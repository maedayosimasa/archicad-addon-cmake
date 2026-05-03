#pragma warning(disable : 4819)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "APIEnvir.h"
#include "ACAPinc.h"
#include "ResourceIds.hpp"
#include "DGModule.hpp"
#include "DG.h"
#include "Location.hpp"
#include "FileSystem.hpp"
#include "ExampleDialog.hpp"
#include "StoryService.hpp"
#include "ElementService.hpp"
#include "HashTable.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include <queue>
#include <atomic>
#include <cmath>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <map>
#include <deque>
#include <algorithm>
#include <cctype>

#pragma comment(lib, "ws2_32.lib")

// --- [DEBUG]: 外部ログ出力関数の実装 ---
void WriteExternalLog(const std::string& message) {
    static std::mutex logMutex;
    std::lock_guard<std::mutex> lock(logMutex);
    try {
        std::ofstream logFile("C:\\AC28_AddOnProject\\archicad-addon-pyQt\\server_log.txt", std::ios_base::app);
        if (logFile.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            struct tm buf;
            localtime_s(&buf, &in_time_t);
            logFile << "[" << std::put_time(&buf, "%Y-%m-%d %H:%M:%S") << "] " << message << std::endl;
            logFile.close();
        }
    } catch (...) {}
}

// --- データ構造定義 ---
struct ChangeData {
    std::string guid;
    std::string group; 
    std::string propId;
    std::string propName;
    std::string propGroup;
    std::string specialType;
    std::string value;
    UInt32 modiStamp = 0;
};

struct CommandBatch {
    std::string type;
    std::vector<ChangeData> changes;
    std::string rawJson;
};

static const GSResID AddOnInfoID = ID_ADDON_INFO;
static const short AddOnMenuID = ID_ADDON_MENU;
static HANDLE serverProcessHandle = NULL;

// --- [RE-DESIGN]: 完全アトミック制御フラグ ---
static std::mutex commandMutex;
static std::deque<CommandBatch> commandQueue;
static std::atomic<bool> isExecuting(false);
static std::atomic<bool> dispatchScheduled(false);

static std::thread listenerThread;
static SOCKET listenSocket = INVALID_SOCKET;
static std::atomic<bool> stopListener(false);

static std::mutex sendMutex;
static std::deque<std::string> sendQueue;
static std::thread senderThread;
static std::atomic<bool> stopSender(false);

static constexpr GSType EventLoopDispatcherCmdID = 'EVLP';
static constexpr Int32 EventLoopDispatcherCmdVersion = 1;
static const API_ModulID OwnModuleID = { 11, 11 };

struct ElementTypeInfo { API_ElemTypeID id; const char* key; };
static const ElementTypeInfo SupportedTypes[] = {
    { API_WallID, "Wall" }, { API_ColumnID, "Column" }, { API_BeamID, "Beam" },
    { API_SlabID, "Slab" }, { API_WindowID, "Window" }, { API_DoorID, "Door" },
    { API_ZoneID, "Zone" }, { API_RoofID, "Roof" }, { API_ObjectID, "Object" },
    { API_MorphID, "Morph" }, { API_MeshID, "Mesh" }
};

// --- 関数前方宣言 ---
void ProcessOneCommand();
void ExecuteBatch(const CommandBatch& batch);
void DoSearch(const std::string& json);
void ExportDefs(const std::string& json);
void ExportValues(const std::string& json);
GSErrCode EventLoopCommandHandler(GSHandle params, GSPtr resultData, bool silentMode);
void StartPythonServer();
void EnqueueProjectConfiguration();
static void LogLoadedAddOnBinary();

static void ScheduleProcessOneCommand() {
    const GSErrCode err = ACAPI_AddOnAddOnCommunication_CallFromEventLoop(
        &OwnModuleID,
        EventLoopDispatcherCmdID,
        EventLoopDispatcherCmdVersion,
        nullptr,
        true,
        nullptr
    );
    if (err != NoError) {
        WriteExternalLog("Failed to schedule command processing: " + std::to_string(err));
        dispatchScheduled.store(false);
    }
}

static GS::UniString Escape(const GS::UniString& input) {
    GS::UniString out = input; out.ReplaceAll("\"", "\\\""); out.ReplaceAll("\n", " "); out.ReplaceAll("\r", " "); return out;
}

static bool IsValidNumber(const std::string& s) {
    if (s.empty()) return false;
    char* end = nullptr;
    std::strtod(s.c_str(), &end);
    return end != s.c_str() && *end == '\0';
}

static std::string ToLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [] (unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

static bool ContainsInsensitive(const std::string& text, const std::string& needle) {
    return ToLowerAscii(text).find(ToLowerAscii(needle)) != std::string::npos;
}

static bool IsElementIdDefinition(const GS::UniString& propName, const GS::UniString& groupName) {
    const std::string nameUtf8 = std::string((const char*)propName.ToCStr(CC_UTF8));
    const std::string groupUtf8 = std::string((const char*)groupName.ToCStr(CC_UTF8));

    const bool nameMatches =
        propName == "ID" ||
        propName == "Element ID" ||
        ContainsInsensitive(nameUtf8, "element id") ||
        ContainsInsensitive(nameUtf8, "id");
    const bool groupMatches =
        groupName == "ID and Categories" ||
        groupName == "IDとカテゴリ" ||
        ContainsInsensitive(groupUtf8, "categories");

    return nameMatches && groupMatches;
}

static std::string GetV(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return "";
    size_t colonPos = json.find(":", keyPos + searchKey.length());
    if (colonPos == std::string::npos) return "";
    size_t startQuote = json.find("\"", colonPos);
    if (startQuote == std::string::npos) return "";
    startQuote++; 
    size_t endQuote = json.find("\"", startQuote);
    if (endQuote == std::string::npos) return "";
    return json.substr(startQuote, endQuote - startQuote);
}

static GS::Array<API_Guid> ExtractGuids(const std::string& json, const std::string& key) {
    GS::Array<API_Guid> guids;
    size_t pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos) return guids;
    pos = json.find("[", pos); if (pos == std::string::npos) return guids;
    size_t endArr = json.find("]", pos); if (endArr == std::string::npos) return guids;
    size_t current = pos;
    while (current < endArr) {
        size_t s = json.find("\"", current); if (s == std::string::npos || s > endArr) break;
        size_t e = json.find("\"", s + 1); if (e == std::string::npos || e > endArr) break;
        std::string gStr = json.substr(s + 1, e - s - 1);
        if (gStr.length() == 36) {
            try { guids.Push(GSGuid2APIGuid(GS::Guid(gStr.c_str()))); } catch(...) {}
        }
        current = e + 1;
    }
    return guids;
}

static std::vector<std::string> ExtractStringArray(const std::string& json, const std::string& key) {
    std::vector<std::string> values;
    size_t pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos) return values;
    pos = json.find("[", pos);
    if (pos == std::string::npos) return values;
    size_t endArr = json.find("]", pos);
    if (endArr == std::string::npos) return values;

    size_t current = pos;
    while (current < endArr) {
        size_t s = json.find("\"", current);
        if (s == std::string::npos || s > endArr) break;
        size_t e = json.find("\"", s + 1);
        if (e == std::string::npos || e > endArr) break;
        values.push_back(json.substr(s + 1, e - s - 1));
        current = e + 1;
    }
    return values;
}

// --- [通信レイヤー]: 非同期送信 ---
void EnqueueResult(const std::string& json) {
    std::lock_guard<std::mutex> lock(sendMutex);
    sendQueue.push_back(json);
}

void SenderLoop() {
    while (!stopSender) {
        std::string msg;
        {
            std::lock_guard<std::mutex> lock(sendMutex);
            if (!sendQueue.empty()) { msg = sendQueue.front(); sendQueue.pop_front(); }
        }
        if (!msg.empty()) {
            SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (s != INVALID_SOCKET) {
                sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons(5000); addr.sin_addr.s_addr = inet_addr("127.0.0.1");
                if (connect(s, (sockaddr*)&addr, sizeof(addr)) == 0) {
                    send(s, msg.c_str(), (int)msg.length(), 0);
                    WriteExternalLog("Sent message to Python: " + msg.substr(0, (std::min<size_t>)(msg.size(), 120)));
                } else {
                    WriteExternalLog("Failed to connect to Python receiver; retrying");
                    std::lock_guard<std::mutex> lock(sendMutex);
                    sendQueue.push_front(msg);
                }
                closesocket(s);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(msg.empty() ? 50 : 200));
    }
}

void EnqueueProjectConfiguration() {
    GS::UniString out = "{\"type\":\"project_config\",\"stories\":[";
    GS::Array<StoryData> stories = StoryService::GetAllStories();
    for (UInt32 i = 0; i < stories.GetSize(); i++) {
        if (i > 0) out.Append(",");
        out.Append("{\"index\":"); out.Append(GS::UniString::Printf("%d", stories[i].index));
        out.Append(",\"name\":\""); out.Append(Escape(stories[i].name)); out.Append("\"}");
    }
    out.Append("],\"elementTypes\":{");
    bool first = true;
    for (const auto& t : SupportedTypes) {
        GS::Array<ElementInfo> e = ElementService::GetElementsByTypeAndStories(t.id, {});
        if (e.IsEmpty()) continue;
        if (!first) out.Append(",");
        out.Append("\""); out.Append(t.key); out.Append("\":"); out.Append(GS::UniString::Printf("%u", (UInt32)e.GetSize()));
        first = false;
    }
    out.Append("}}");
    EnqueueResult(std::string((const char*)out.ToCStr(CC_UTF8)));
}

// --- [APIレイヤー]: メインスレッド安全実行 (EventLoopディスパッチャ) ---
void EnqueueCommand(const CommandBatch& cmd) {
    if (cmd.type == "apply_changes" && cmd.changes.size() > 1000) {
        WriteExternalLog("Splitting large batch: " + std::to_string(cmd.changes.size()));
        for (size_t i = 0; i < cmd.changes.size(); i += 1000) {
            CommandBatch subBatch;
            subBatch.type = "apply_changes";
            size_t end = (std::min)(i + 1000, cmd.changes.size());
            subBatch.changes.assign(cmd.changes.begin() + i, cmd.changes.begin() + end);
            {
                std::lock_guard<std::mutex> lock(commandMutex);
                commandQueue.push_back(subBatch);
            }
        }
    } else {
        std::lock_guard<std::mutex> lock(commandMutex);
        commandQueue.push_back(cmd);
    }
    bool expected = false;
    if (dispatchScheduled.compare_exchange_strong(expected, true)) {
        ScheduleProcessOneCommand();
    }
}

void ProcessOneCommand() {
    dispatchScheduled.store(false);
    bool expected = false;
    if (!isExecuting.compare_exchange_strong(expected, true)) return;
    CommandBatch batch;
    {
        std::lock_guard<std::mutex> lock(commandMutex);
        if (commandQueue.empty()) { isExecuting.store(false); return; }
        batch = std::move(commandQueue.front());
        commandQueue.pop_front();
    }
    WriteExternalLog("EXEC START: " + batch.type + " (rem: " + std::to_string(commandQueue.size()) + ")");
    try { ExecuteBatch(batch); } catch (...) { WriteExternalLog("CRITICAL: Exception in ExecuteBatch"); }
    WriteExternalLog("EXEC END: " + batch.type);
    isExecuting.store(false);
    {
        std::lock_guard<std::mutex> lock(commandMutex);
        if (!commandQueue.empty()) {
            bool exp = false;
            if (dispatchScheduled.compare_exchange_strong(exp, true)) {
                ScheduleProcessOneCommand();
            }
        }
    }
}

// --- [内部処理]: 要素更新ロジック ---
static bool ApplyElementChanges_Internal(const std::string& guidStr, const std::vector<ChangeData>& changes) {
    API_Guid eg;
    try { eg = GSGuid2APIGuid(GS::Guid(guidStr.c_str())); } catch(...) { return false; }
    API_Element element = {};
    API_Element mask = {};
    BNZeroMemory(&element, sizeof(API_Element));
    ACAPI_ELEMENT_MASK_CLEAR(mask);
    element.header.guid = eg;
    if (ACAPI_Element_Get(&element) != NoError) return false;

    if (!changes.empty() && changes[0].modiStamp != 0) {
        if (element.header.modiStamp != changes[0].modiStamp) {
            WriteExternalLog("CONFLICT: modiStamp mismatch for " + guidStr);
            return false;
        }
    }

    API_Element updateElem = element;
    bool elementChanged = false;
    GS::HashTable<API_Guid, ChangeData> propMap;
    GS::HashTable<API_Guid, API_PropertyDefinition> resolvedDefMap;
    GS::Array<API_PropertyDefinition> propDefsToUpdate;
    GS::Array<API_Guid> classItemsToAdd;
    bool hasElementInfoStringChange = false;
    GS::UniString elementInfoStringValue;
    GS::Array<API_PropertyDefinition> elementScopedDefinitions;
    const bool hasElementScopedDefinitions =
        ACAPI_Element_GetPropertyDefinitions(eg, API_PropertyDefinitionFilter_All, elementScopedDefinitions) == NoError;

    for (const auto& data : changes) {
        const bool isElementIdPropertyByName =
            (data.propId == "builtin:element_id") ||
            (data.specialType == "element_id") ||
            (((data.propName == "ID" || data.propName == "Element ID" || ContainsInsensitive(data.propName, "element id") || ContainsInsensitive(data.propName, "id")) &&
              (data.propGroup == "ID and Categories" || data.propGroup == "IDとカテゴリ" || ContainsInsensitive(data.propGroup, "categories"))));

        if (isElementIdPropertyByName) {
            hasElementInfoStringChange = true;
            elementInfoStringValue = GS::UniString(data.value.c_str(), CC_UTF8);
            WriteExternalLog("Detected Element ID change by metadata for " + guidStr + " -> " + data.value + " [name=" + data.propName + ", group=" + data.propGroup + ", special=" + data.specialType + "]");
            continue;
        }

        if (data.group == "element" || data.group == "parameter") {
            if (data.propId == "builtin:Layer") {
                updateElem.header.layer = ACAPI_CreateAttributeIndex((Int32)std::stoi(data.value));
                ACAPI_ELEMENT_MASK_SET(mask, API_Elem_Head, layer);
                elementChanged = true;
            } else {
                switch (updateElem.header.type.typeID) {
                    case API_ColumnID:
                        if (data.propId == "builtin:Height" && IsValidNumber(data.value)) {
                            updateElem.column.height = std::stod(data.value);
                            ACAPI_ELEMENT_MASK_SET(mask, API_ColumnType, height);
                            elementChanged = true;
                        }
                        break;
                    case API_WallID:
                        if (data.propId == "builtin:Thickness" && IsValidNumber(data.value)) {
                            updateElem.wall.thickness = std::stod(data.value);
                            ACAPI_ELEMENT_MASK_SET(mask, API_WallType, thickness);
                            elementChanged = true;
                        }
                        break;
                    default: break;
                }
            }
            continue;
        }
        GS::Guid guidObj;
        try { guidObj = GS::Guid(data.propId.c_str()); } catch(...) { continue; }
        if (guidObj.IsNull()) continue;
        API_Guid apiGuid = GSGuid2APIGuid(guidObj);
        API_PropertyDefinition pDef = {}; pDef.guid = apiGuid;
        bool hasResolvedDefinition = false;
        if (ACAPI_Property_GetPropertyDefinition(pDef) == NoError) {
            hasResolvedDefinition = true;
        } else if (hasElementScopedDefinitions) {
            for (const auto& def : elementScopedDefinitions) {
                if (def.guid == apiGuid) {
                    pDef = def;
                    hasResolvedDefinition = true;
                    WriteExternalLog("Resolved property definition from element scope for " + guidStr + " propId=" + data.propId);
                    break;
                }
            }
        }

        if (hasResolvedDefinition) {
            API_PropertyGroup propGroup = {};
            propGroup.guid = pDef.groupGuid;
            ACAPI_Property_GetPropertyGroup(propGroup);

            const bool isElementIdProperty = IsElementIdDefinition(pDef.name, propGroup.name);

            if (isElementIdProperty) {
                hasElementInfoStringChange = true;
                elementInfoStringValue = GS::UniString(data.value.c_str(), CC_UTF8);
                WriteExternalLog("Detected Element ID change for " + guidStr + " -> " + data.value);
                continue;
            }

            if (!propMap.ContainsKey(apiGuid)) {
                propMap.Add(apiGuid, data);
                resolvedDefMap.Add(apiGuid, pDef);
                propDefsToUpdate.Push(pDef);
                WriteExternalLog(
                    "Queued property update for " + guidStr +
                    " propId=" + data.propId +
                    " name=" + std::string((const char*)pDef.name.ToCStr(CC_UTF8)) +
                    " editable=" + std::to_string(pDef.canValueBeEditable ? 1 : 0) +
                    " valueType=" + std::to_string((int)pDef.valueType) +
                    " collectionType=" + std::to_string((int)pDef.collectionType)
                );
            }
            continue;
        }
        WriteExternalLog("Could not resolve property/classification definition for " + guidStr + " propId=" + data.propId);
        API_ClassificationItem cItem = {}; cItem.guid = apiGuid;
        if (ACAPI_Classification_GetClassificationItem(cItem) == NoError) {
            if (!classItemsToAdd.Contains(apiGuid)) classItemsToAdd.Push(apiGuid);
            continue;
        }
    }

    GSErrCode undoErr = ACAPI_CallUndoableCommand("Sync Element", [&]() -> GSErrCode {
        bool opError = false;
        bool anyApplied = false;
        if (elementChanged && ACAPI_Element_Change(&updateElem, &mask, nullptr, 0, true) != NoError) opError = true;
        else if (elementChanged) anyApplied = true;

        if (hasElementInfoStringChange) {
            const GSErrCode infoErr = ACAPI_Element_ChangeElementInfoString(&eg, &elementInfoStringValue);
            if (infoErr != NoError) {
                WriteExternalLog("Failed to change Element ID for " + guidStr + " err=" + std::to_string(infoErr));
                opError = true;
            } else {
                anyApplied = true;
            }
        }

        if (!classItemsToAdd.IsEmpty()) {
            GS::Array<GS::Pair<API_Guid, API_Guid>> existing;
            ACAPI_Element_GetClassificationItems(eg, existing);
            for (const auto& newItemGuid : classItemsToAdd) {
                API_ClassificationSystem system = {};
                if (ACAPI_Classification_GetClassificationItemSystem(newItemGuid, system) == NoError) {
                    for (const auto& pair : existing) if (pair.first == system.guid) ACAPI_Element_RemoveClassificationItem(eg, pair.second);
                    if (ACAPI_Element_AddClassificationItem(eg, newItemGuid) != NoError) opError = true;
                    else anyApplied = true;
                }
            }
        }
        if (!propDefsToUpdate.IsEmpty()) {
            GS::Array<API_Property> properties;
            if (ACAPI_Element_GetPropertyValues(eg, propDefsToUpdate, properties) == NoError) {
                GS::Array<API_Property> propsToSet;
                for (auto& prop : properties) {
                    ChangeData* dPtr = propMap.GetPtr(prop.definition.guid);
                    if (dPtr == nullptr) continue;
                    API_PropertyDefinition def = {};
                    API_PropertyDefinition* resolvedDef = resolvedDefMap.GetPtr(prop.definition.guid);
                    if (resolvedDef != nullptr) {
                        def = *resolvedDef;
                    } else {
                        def.guid = prop.definition.guid;
                        ACAPI_Property_GetPropertyDefinition(def);
                    }
                    if (prop.status == API_Property_NotAvailable) {
                        WriteExternalLog("Skip property update: not available for guid=" + guidStr + " propId=" + dPtr->propId);
                        continue;
                    }
                    if (def.defaultValue.hasExpression) {
                        WriteExternalLog("Skip property update: expression-backed for guid=" + guidStr + " propId=" + dPtr->propId);
                        continue;
                    }
                    if (!def.canValueBeEditable) {
                        WriteExternalLog("Skip property update: not editable for guid=" + guidStr + " propId=" + dPtr->propId);
                        continue;
                    }
                    prop.isDefault = false; prop.status = API_Property_HasValue;
                    bool valueSet = false;
                    if (def.valueType == API_PropertyStringValueType) {
                        prop.value.singleVariant.variant.type = API_PropertyStringValueType;
                        prop.value.singleVariant.variant.uniStringValue = GS::UniString(dPtr->value.c_str(), CC_UTF8);
                        valueSet = true;
                    } else if (def.valueType == API_PropertyIntegerValueType) {
                        if (IsValidNumber(dPtr->value)) {
                            prop.value.singleVariant.variant.type = API_PropertyIntegerValueType;
                            prop.value.singleVariant.variant.intValue = (Int32)std::stol(dPtr->value);
                            valueSet = true;
                        } else if (def.collectionType == API_PropertySingleChoiceEnumerationCollectionType) {
                            GS::UniString target(dPtr->value.c_str(), CC_UTF8);
                            for (const auto& ev : def.possibleEnumValues) {
                                if (ev.displayVariant.uniStringValue == target) {
                                    prop.value.singleVariant.variant = ev.keyVariant; valueSet = true; break;
                                }
                            }
                        }
                    } else if (def.valueType == API_PropertyRealValueType && IsValidNumber(dPtr->value)) {
                        prop.value.singleVariant.variant.type = API_PropertyRealValueType;
                        prop.value.singleVariant.variant.doubleValue = std::stod(dPtr->value);
                        valueSet = true;
                    } else if (def.valueType == API_PropertyBooleanValueType) {
                        prop.value.singleVariant.variant.type = API_PropertyBooleanValueType;
                        prop.value.singleVariant.variant.boolValue = (dPtr->value == "True" || dPtr->value == "true" || dPtr->value == "1");
                        valueSet = true;
                    }
                    if (valueSet) {
                        propsToSet.Push(prop);
                        WriteExternalLog("Prepared property update for guid=" + guidStr + " propId=" + dPtr->propId + " value=" + dPtr->value);
                    } else {
                        WriteExternalLog("Skip property update: unsupported value conversion for guid=" + guidStr + " propId=" + dPtr->propId + " value=" + dPtr->value);
                    }
                }
                if (!propsToSet.IsEmpty()) {
                    const GSErrCode setErr = ACAPI_Element_SetProperties(eg, propsToSet);
                    if (setErr != NoError) {
                        WriteExternalLog("ACAPI_Element_SetProperties failed for " + guidStr + " err=" + std::to_string(setErr));
                        opError = true;
                    } else {
                        anyApplied = true;
                    }
                } else {
                    WriteExternalLog("No prepared property values to set for " + guidStr);
                }
            } else {
                WriteExternalLog("ACAPI_Element_GetPropertyValues failed for " + guidStr);
            }
        }
        if (!anyApplied && !opError) {
            WriteExternalLog("No applicable changes found for " + guidStr);
            opError = true;
        }
        return opError ? APIERR_GENERAL : NoError;
    });
    return (undoErr == NoError);
}

void ExecuteBatch(const CommandBatch& batch) {
    if (batch.type == "apply_changes") {
        if (batch.changes.empty()) return;
        std::map<std::string, std::vector<ChangeData>> elementMap;
        for (const auto& c : batch.changes) elementMap[c.guid].push_back(c);
        bool allSuccess = true;
        for (auto const& [guid, elemChanges] : elementMap) {
            if (!ApplyElementChanges_Internal(guid, elemChanges)) allSuccess = false;
        }
        EnqueueResult(allSuccess ? "{\"type\":\"sync_complete\",\"status\":\"success\"}" : "{\"type\":\"sync_complete\",\"status\":\"failed\"}");
    } else if (batch.type == "other") {
        const std::string& cmd = batch.rawJson;
        if (cmd.find("\"ready\"") != std::string::npos) {
            EnqueueProjectConfiguration();
        } 
        else if (cmd.find("\"get_elements\"") != std::string::npos) DoSearch(cmd);
        else if (cmd.find("\"get_definitions\"") != std::string::npos) ExportDefs(cmd);
        else if (cmd.find("\"get_values\"") != std::string::npos) ExportValues(cmd);
        else if (cmd.find("\"select_elements\"") != std::string::npos) {
            GS::Array<API_Guid> guids = ExtractGuids(cmd, "guids");
            if (!guids.IsEmpty()) {
                GS::Array<API_Neig> selNeigs;
                for (const auto& guid : guids) {
                    API_Neig neig;
                    if (ACAPI_Selection_SetSelectedElementNeig(&guid, &neig) == NoError) {
                        selNeigs.Push(neig);
                    }
                }
                if (!selNeigs.IsEmpty()) ACAPI_Selection_Select(selNeigs, true);
            }
        }
    }
}

void DoSearch(const std::string& json) {
    WriteExternalLog("DoSearch request: " + json);
    GS::Array<short> stories;
    size_t sPos = json.find("\"stories\":");
    if (sPos != std::string::npos) {
        size_t s = json.find("[", sPos);
        size_t e = json.find("]", (s != std::string::npos) ? s : 0);
        if (s != std::string::npos && e != std::string::npos) {
            std::string sl = json.substr(s + 1, e - s - 1);
            size_t p = 0, n;
            while ((n = sl.find(",", p)) != std::string::npos) { 
                try { stories.Push((short)std::stoi(sl.substr(p, n - p))); } catch(...) {}
                p = n + 1; 
            }
            if (p < sl.length()) try { stories.Push((short)std::stoi(sl.substr(p))); } catch(...) {}
        }
    }

    GS::Array<API_Guid> results;
    for (const auto& t : SupportedTypes) {
        if (json.find("\"" + std::string(t.key) + "\":true") != std::string::npos) {
            GS::Array<API_Guid> allElementGuids;
            API_ElemType type(t.id);
            if (ACAPI_Element_GetElemList(type, &allElementGuids) != NoError) {
                continue;
            }

            for (const auto& guid : allElementGuids) {
                API_Elem_Head header = {};
                header.guid = guid;
                header.type = type;
                if (ACAPI_Element_GetHeader(&header) != NoError) {
                    continue;
                }

                bool isMatch = stories.IsEmpty();
                for (short s : stories) {
                    if (header.floorInd == s) {
                        isMatch = true;
                        break;
                    }
                }

                if (isMatch) {
                    results.Push(guid);
                    if (results.GetSize() >= 1000) {
                        break;
                    }
                }
            }
        }
        if (results.GetSize() >= 1000) {
            break;
        }
    }

    WriteExternalLog("DoSearch result count: " + std::to_string(results.GetSize()));
    GS::UniString out = "{\"type\":\"elements\",\"elements\":[";
    for (UInt32 i = 0; i < results.GetSize(); i++) {
        if (i > 0) out.Append(",");
        out.Append("{\"guid\":\""); out.Append(APIGuid2GSGuid(results[i]).ToUniString()); out.Append("\"}");
    }
    out.Append("]}");
    EnqueueResult(std::string((const char*)out.ToCStr(CC_UTF8)));
}

void ExportDefs(const std::string& json) {
    GS::Array<API_Guid> guids = ExtractGuids(json, "guids");
    if (guids.IsEmpty()) return;
    GS::Array<API_PropertyDefinition> defs;
    ACAPI_Element_GetPropertyDefinitions(guids[0], API_PropertyDefinitionFilter_All, defs);
    GS::UniString out = "{\"type\":\"property_definitions\",\"definitions\":[";
    out.Append("{\"guid\":\"builtin:element_id\",");
    out.Append("\"name\":\"ID\",");
    out.Append("\"group\":\"IDとカテゴリ\",");
    out.Append("\"editable\":true,");
    out.Append("\"valueType\":1,");
    out.Append("\"collectionType\":0}");
    for (UInt32 i = 0; i < defs.GetSize(); i++) {
        API_PropertyGroup grp = {}; grp.guid = defs[i].groupGuid; ACAPI_Property_GetPropertyGroup(grp);
        if (IsElementIdDefinition(defs[i].name, grp.name)) {
            continue;
        }
        out.Append(",");
        out.Append("{\"guid\":\""); out.Append(APIGuid2GSGuid(defs[i].guid).ToUniString()); out.Append("\",");
        out.Append("\"name\":\""); out.Append(Escape(defs[i].name)); out.Append("\",");
        out.Append("\"group\":\""); out.Append(Escape(grp.name)); out.Append("\",");
        out.Append("\"editable\":"); out.Append(defs[i].canValueBeEditable ? "true" : "false"); out.Append(",");
        out.Append("\"valueType\":"); out.Append(GS::UniString::Printf("%d", (int)defs[i].valueType)); out.Append(",");
        out.Append("\"collectionType\":"); out.Append(GS::UniString::Printf("%d", (int)defs[i].collectionType));
        if (defs[i].collectionType == API_PropertySingleChoiceEnumerationCollectionType || defs[i].collectionType == API_PropertyMultipleChoiceEnumerationCollectionType) {
            out.Append(",\"enums\":[");
            for (UInt32 j = 0; j < defs[i].possibleEnumValues.GetSize(); j++) {
                if (j > 0) out.Append(",");
                out.Append("\""); out.Append(Escape(defs[i].possibleEnumValues[j].displayVariant.uniStringValue)); out.Append("\"");
            }
            out.Append("]");
        }
        out.Append("}");
    }
    out.Append("]}"); EnqueueResult(std::string((const char*)out.ToCStr(CC_UTF8)));
}

void ExportValues(const std::string& json) {
    GS::Array<API_Guid> guids = ExtractGuids(json, "guids");
    std::vector<std::string> propGuidStrings = ExtractStringArray(json, "propGuids");
    if (guids.IsEmpty() || propGuidStrings.empty()) return;
    GS::Array<API_PropertyDefinition> propDefs;
    bool includeElementId = false;
    for (const auto& propGuidString : propGuidStrings) {
        if (propGuidString == "builtin:element_id") {
            includeElementId = true;
            continue;
        }
        try {
            API_Guid pg = GSGuid2APIGuid(GS::Guid(propGuidString.c_str()));
            API_PropertyDefinition d = {}; d.guid = pg;
            if (ACAPI_Property_GetPropertyDefinition(d) == NoError) propDefs.Push(d);
        } catch (...) {}
    }
    GS::UniString out = "{\"type\":\"property_values\",\"values\":[";
    for (UInt32 i = 0; i < guids.GetSize(); i++) {
        if (i > 0) out.Append(",");
        API_Elem_Head header = {}; header.guid = guids[i]; ACAPI_Element_GetHeader(&header);
        out.Append("{\"guid\":\""); out.Append(APIGuid2GSGuid(guids[i]).ToUniString()); out.Append("\",");
        out.Append("\"modiStamp\":"); out.Append(GS::UniString::Printf("%u", header.modiStamp)); out.Append(",");
        out.Append("\"props\":{");
        bool firstProp = true;
        if (includeElementId) {
            GS::UniString elementIdValue;
            if (ACAPI_Element_GetElementInfoString(&guids[i], &elementIdValue) == NoError) {
                out.Append("\"builtin:element_id\":\"");
                out.Append(Escape(elementIdValue));
                out.Append("\"");
                firstProp = false;
            }
        }
        GS::Array<API_Property> props;
        if (!propDefs.IsEmpty() && ACAPI_Element_GetPropertyValues(guids[i], propDefs, props) == NoError) {
            for (UInt32 j = 0; j < props.GetSize(); j++) {
                if (!firstProp) out.Append(",");
                out.Append("\""); out.Append(APIGuid2GSGuid(props[j].definition.guid).ToUniString()); out.Append("\":");
                GS::UniString valStr;
                if (ACAPI_Property_GetPropertyValueString(props[j], &valStr) == NoError) {
                    out.Append("\""); out.Append(Escape(valStr)); out.Append("\"");
                } else { out.Append("\"---\""); }
                firstProp = false;
            }
        }
        out.Append("}}");
    }
    out.Append("]}"); EnqueueResult(std::string((const char*)out.ToCStr(CC_UTF8)));
}

void StartCommandListener() {
    stopListener = false;
    listenerThread = std::thread([]() {
        listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSocket == INVALID_SOCKET) {
            WriteExternalLog("Failed to create listener socket");
            return;
        }
        sockaddr_in svc{}; svc.sin_family = AF_INET; svc.sin_addr.s_addr = INADDR_ANY; svc.sin_port = htons(5001);
        if (bind(listenSocket, (SOCKADDR*)&svc, sizeof(svc)) == SOCKET_ERROR) {
            WriteExternalLog("Failed to bind listener socket");
            closesocket(listenSocket);
            listenSocket = INVALID_SOCKET;
            return;
        }
        listen(listenSocket, 5);
        WriteExternalLog("Command listener started on port 5001");
        while (!stopListener) {
            fd_set r; FD_ZERO(&r); 
            if (listenSocket != INVALID_SOCKET) {
                FD_SET(listenSocket, &r); timeval t{1, 0};
                if (select(0, &r, NULL, NULL, &t) > 0) {
                    SOCKET as = accept(listenSocket, NULL, NULL);
                    if (as != INVALID_SOCKET) {
                        char buf[65536]; int br = recv(as, buf, sizeof(buf) - 1, 0);
                        if (br > 0) { buf[br] = '\0'; 
                            CommandBatch b;
                            if (std::string(buf).find("\"apply_changes\"") != std::string::npos) {
                                b.type = "apply_changes";
                                size_t pos = std::string(buf).find("\"changes\":[");
                                if (pos != std::string::npos) {
                                    pos += 11;
                                    while (true) {
                                        size_t s = std::string(buf).find("{", pos); if (s == std::string::npos) break;
                                        size_t e = std::string(buf).find("}", s); if (e == std::string::npos) break;
                                        std::string obj = std::string(buf).substr(s, e - s + 1);
                                        ChangeData d; 
                                        d.guid = GetV(obj, "guid"); d.group = GetV(obj, "group"); d.propId = GetV(obj, "propId"); d.propName = GetV(obj, "propName"); d.propGroup = GetV(obj, "propGroup"); d.specialType = GetV(obj, "specialType"); d.value = GetV(obj, "value");
                                        std::string stampStr = GetV(obj, "modiStamp");
                                        if (!stampStr.empty()) d.modiStamp = (UInt32)std::stoul(stampStr);
                                        if (!d.guid.empty()) b.changes.push_back(d);
                                        pos = e + 1;
                                    }
                                }
                            } else { b.type = "other"; b.rawJson = buf; }
                            if (b.type == "apply_changes") {
                                WriteExternalLog("Received apply_changes count: " + std::to_string(b.changes.size()));
                                if (!b.changes.empty()) {
                                    const ChangeData& first = b.changes.front();
                                    WriteExternalLog("First apply_change: guid=" + first.guid + ", propId=" + first.propId + ", propName=" + first.propName + ", propGroup=" + first.propGroup + ", specialType=" + first.specialType + ", value=" + first.value);
                                }
                            } else {
                                WriteExternalLog("Received command: " + b.rawJson.substr(0, (std::min<size_t>)(b.rawJson.size(), 120)));
                            }
                            EnqueueCommand(b);
                        }
                        closesocket(as);
                    }
                }
            }
        }
        WriteExternalLog("Command listener stopped");
    });
}

void StartPythonServer() {
    if (serverProcessHandle) {
        DWORD exit; if (GetExitCodeProcess(serverProcessHandle, &exit) && exit == STILL_ACTIVE) return;
        CloseHandle(serverProcessHandle); serverProcessHandle = NULL;
    }
    IO::Location loc;
    if (ACAPI_GetOwnLocation(&loc) == NoError) {
        for (int i = 0; i < 5; i++) {
            loc.DeleteLastLocalName(); IO::Location test = loc; test.AppendToLocal(IO::Name("server.py"));
            bool ex; if (IO::fileSystem.Contains(test, &ex) == NoError && ex) {
                GS::UniString p; test.ToPath(&p);
                std::wstring c = L"python \"" + std::wstring((const wchar_t*)p.ToUStr().Get()) + L"\"";
                STARTUPINFO si = { sizeof(si) }; PROCESS_INFORMATION pi;
                if (CreateProcess(NULL, (wchar_t*)c.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                    serverProcessHandle = pi.hProcess;
                    CloseHandle(pi.hThread);
                    WriteExternalLog("Started Python server process");
                    break;
                }
            }
        }
    }
}

static void LogLoadedAddOnBinary() {
    IO::Location ownLocation;
    if (ACAPI_GetOwnLocation(&ownLocation) == NoError) {
        GS::UniString pathString;
        ownLocation.ToPath(&pathString);
        WriteExternalLog("Loaded Add-On binary: " + std::string((const char*)pathString.ToCStr(CC_UTF8)));
    } else {
        WriteExternalLog("Loaded Add-On binary: <unavailable>");
    }
    WriteExternalLog(std::string("Add-On build stamp: ") + __DATE__ + " " + __TIME__);
}

void ExportElementsToPython () {
	GS::Array<API_Guid> elemGuids;
	if (ACAPI_Element_GetElemList (API_ZombieElemID, &elemGuids) != NoError) return;
	GS::UniString jsonStr = "{\"type\":\"elements\",\"elements\":[";
	bool first = true;
	for (const auto& guid : elemGuids) {
		API_Element element = {}; element.header.guid = guid;
		if (ACAPI_Element_GetHeader (&element.header) == NoError) {
			if (!first) jsonStr += ",";
			GS::UniString typeName; ACAPI_Element_GetElemTypeName (element.header.type, typeName);
			jsonStr += "{\"guid\":\"" + APIGuid2GSGuid (guid).ToUniString () + "\",\"type\":\"" + Escape (typeName) + "\",\"floor\":" + GS::UniString::Printf ("%d", element.header.floorInd) + "}";
			first = false;
		}
	}
	jsonStr += "]}"; EnqueueResult(std::string((const char*)jsonStr.ToCStr(CC_UTF8)));
}

GSErrCode __stdcall MenuCommandHandler (const API_MenuParams*) { ExampleDialog::GetInstance ().Show (); ExampleDialog::GetInstance ().BringToFront (); return NoError; }
GSErrCode EventLoopCommandHandler(GSHandle, GSPtr, bool) { ProcessOneCommand(); return NoError; }

extern "C" {
    API_AddonType CheckEnvironment(API_EnvirParams* envir) { RSGetIndString(&envir->addOnInfo.name, AddOnInfoID, 1, ACAPI_GetOwnResModule()); RSGetIndString(&envir->addOnInfo.description, AddOnInfoID, 2, ACAPI_GetOwnResModule()); return APIAddon_Normal; }
    GSErrCode RegisterInterface(void) {
        GSErrCode err = ACAPI_MenuItem_RegisterMenu(AddOnMenuID, 0, MenuCode_UserDef, MenuFlag_Default);
        if (err == NoError) err = ACAPI_AddOnAddOnCommunication_RegisterSupportedService(EventLoopDispatcherCmdID, EventLoopDispatcherCmdVersion);
        return err;
    }
    GSErrCode Initialize(void) {
        WSAData wsa; WSAStartup(MAKEWORD(2,2), &wsa); ACAPI_KeepInMemory(true); stopSender = false; senderThread = std::thread(SenderLoop); LogLoadedAddOnBinary(); StartCommandListener(); StartPythonServer();
        GSErrCode err = ACAPI_MenuItem_InstallMenuHandler(AddOnMenuID, MenuCommandHandler);
        if (err == NoError) err = ACAPI_AddOnIntegration_InstallModulCommandHandler(EventLoopDispatcherCmdID, EventLoopDispatcherCmdVersion, EventLoopCommandHandler);
        if (err == NoError) {
            ACAPI_RegisterModelessWindow(
                ExampleDialog::PaletteRefId(),
                ExampleDialog::PaletteAPIControlCallBack,
                API_PalEnabled_FloorPlan + API_PalEnabled_Section + API_PalEnabled_Elevation +
                API_PalEnabled_InteriorElevation + API_PalEnabled_3D +
                API_PalEnabled_Detail + API_PalEnabled_Worksheet + API_PalEnabled_Layout +
                API_PalEnabled_DocumentFrom3D,
                GSGuid2APIGuid(ExampleDialog::PaletteGuid())
            );
        }
        return err;
    }
    GSErrCode FreeData(void) {
        WriteExternalLog("FreeData start");
        stopListener = true;
        stopSender = true;
        ACAPI_UnregisterModelessWindow (ExampleDialog::PaletteRefId ());

        if (listenSocket != INVALID_SOCKET) {
            shutdown(listenSocket, SD_BOTH);
            closesocket(listenSocket);
            listenSocket = INVALID_SOCKET;
        }

        if (listenerThread.joinable()) listenerThread.join();
        if (senderThread.joinable()) senderThread.join();

        if (serverProcessHandle) {
            DWORD exitCode = 0;
            if (GetExitCodeProcess(serverProcessHandle, &exitCode) && exitCode == STILL_ACTIVE) {
                TerminateProcess(serverProcessHandle, 0);
            }
            CloseHandle(serverProcessHandle);
            serverProcessHandle = NULL;
        }

        WSACleanup();
        WriteExternalLog("FreeData end");
        return NoError;
    }
}
