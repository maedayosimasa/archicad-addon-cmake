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

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include <queue>

#pragma comment(lib, "ws2_32.lib")

static const GSResID AddOnInfoID = ID_ADDON_INFO;
static const short AddOnMenuID = ID_ADDON_MENU;
static const Int32 AddOnCommandID = 1;

static HANDLE serverProcessHandle = NULL;
static std::mutex commandMutex;
static std::queue<std::string> commandQueue;
static bool isProcessingCommand = false;

struct ElementTypeInfo {
    API_ElemTypeID id;
    const char* key;
};

static const ElementTypeInfo SupportedTypes[] = {
    { API_WallID, "Wall" },
    { API_ColumnID, "Column" },
    { API_BeamID, "Beam" },
    { API_SlabID, "Slab" },
    { API_WindowID, "Window" },
    { API_DoorID, "Door" },
    { API_ZoneID, "Zone" },
    { API_RoofID, "Roof" },
    { API_ObjectID, "Object" },
    { API_MorphID, "Morph" },
    { API_MeshID, "Mesh" }
};

void SendDataToPython (const GS::UniString& jsonStr)
{
    SOCKET connectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL, hints;
    ZeroMemory (&hints, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    if (getaddrinfo ("127.0.0.1", "5000", &hints, &result) != 0) {
        ACAPI_WriteReport ("Failed to get addrinfo for 127.0.0.1:5000", true);
        return;
    }
    connectSocket = socket (result->ai_family, result->ai_socktype, result->ai_protocol);
    if (connectSocket == INVALID_SOCKET) {
        ACAPI_WriteReport ("Failed to create socket for connecting to Python server", true);
        freeaddrinfo (result);
        return;
    }
    if (connect (connectSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        ACAPI_WriteReport ("Failed to connect to Python server at 127.0.0.1:5000. Is the server running?", true);
        closesocket (connectSocket);
        freeaddrinfo (result);
        return;
    }
    freeaddrinfo (result);
    char* utf8Data = jsonStr.CopyUTF8();
    if (utf8Data != nullptr) {
        send (connectSocket, utf8Data, (int)strlen(utf8Data), 0);
        delete[] utf8Data;
    }
    closesocket (connectSocket);
}

static GS::UniString Escape (const GS::UniString& input)
{
    GS::UniString output = input;
    output.ReplaceAll ("\"", "\\\"");
    output.ReplaceAll ("\n", " ");
    output.ReplaceAll ("\r", " ");
    return output;
}

void ExportElementsToPython (const GS::Array<ElementInfo>& elements)
{
    GS::UniString jsonStr = "{\"elements\": [";
    for (UInt32 i = 0; i < elements.GetSize(); i++) {
        if (i > 0) jsonStr.Append (",");
        const auto& info = elements[i];
        int displayFloor = (info.floorInd >= 0) ? (info.floorInd + 1) : (int)info.floorInd;
        jsonStr.Append ("{");
        jsonStr.Append ("\"guid\": \""); jsonStr.Append (APIGuid2GSGuid(info.guid).ToUniString()); jsonStr.Append ("\",");
        jsonStr.Append ("\"type\": \""); jsonStr.Append (info.typeName); jsonStr.Append ("\",");
        jsonStr.Append ("\"floor\": "); jsonStr.Append (GS::UniString::Printf("%d", displayFloor)); jsonStr.Append (",");
        jsonStr.Append ("\"elementID\": \""); jsonStr.Append (Escape(info.elementID)); jsonStr.Append ("\",");
        jsonStr.Append ("\"categoryID\": \""); jsonStr.Append (Escape(info.categoryID)); jsonStr.Append ("\",");
        jsonStr.Append ("\"structuralFunction\": \""); jsonStr.Append (Escape(info.structuralFunction)); jsonStr.Append ("\",");
        jsonStr.Append ("\"position\": \""); jsonStr.Append (Escape(info.position)); jsonStr.Append ("\",");
        jsonStr.Append ("\"width\": "); jsonStr.Append (GS::UniString::Printf("%.2f", info.width)); jsonStr.Append (",");
        jsonStr.Append ("\"height\": "); jsonStr.Append (GS::UniString::Printf("%.2f", info.height));
        jsonStr.Append ("}");
    }
    jsonStr.Append ("]}");
    SendDataToPython (jsonStr);
}

void ExportConfigurationToPython ()
{
    GS::UniString jsonStr = "{\"type\": \"project_config\", \"stories\": [";
    GS::Array<StoryData> stories = StoryService::GetAllStories();
    for (UInt32 i = 0; i < stories.GetSize(); i++) {
        if (i > 0) jsonStr.Append (",");
        jsonStr.Append ("{");
        jsonStr.Append ("\"index\": "); jsonStr.Append (GS::UniString::Printf("%d", stories[i].index)); jsonStr.Append (",");
        jsonStr.Append ("\"name\": \""); jsonStr.Append (Escape(stories[i].name)); jsonStr.Append ("\",");
        jsonStr.Append ("\"level\": "); jsonStr.Append (GS::UniString::Printf("%.2f", stories[i].level)); jsonStr.Append (",");
        jsonStr.Append ("\"height\": "); jsonStr.Append (GS::UniString::Printf("%.2f", stories[i].height)); jsonStr.Append ("}");
    }
    jsonStr.Append ("], \"elementTypes\": {");
    bool first = true;
    for (int i = 0; i < sizeof(SupportedTypes)/sizeof(SupportedTypes[0]); i++) {
        GS::Array<ElementInfo> elements = ElementService::GetElementsByTypeAndStories(SupportedTypes[i].id, {}); // 全フロアで取得してカウント
        if (elements.IsEmpty()) continue;

        if (!first) jsonStr.Append (",");
        jsonStr.Append ("\""); jsonStr.Append (SupportedTypes[i].key); jsonStr.Append ("\": ");
        jsonStr.Append (GS::UniString::Printf("%u", elements.GetSize()));
        first = false;
    }
    jsonStr.Append ("}}");
    SendDataToPython (jsonStr);
}

void DoSearch (const std::string& jsonCommand)
{
    GS::Array<short> selectedStories;
    std::string cmd = jsonCommand;
    
    ACAPI_WriteReport(GS::UniString("Search command received: ") + GS::UniString(cmd.c_str()), false);

    // ストーリー（階）の抽出ロジック
    size_t sPos = cmd.find("\"stories\":");
    if (sPos != std::string::npos) {
        size_t startBracket = cmd.find("[", sPos);
        size_t endBracket = cmd.find("]", startBracket);
        if (startBracket != std::string::npos && endBracket != std::string::npos) {
            std::string sList = cmd.substr(startBracket + 1, endBracket - startBracket - 1);
            size_t start = 0, next;
            while ((next = sList.find(",", start)) != std::string::npos) {
                std::string val = sList.substr(start, next - start);
                try { selectedStories.Push((short)std::stoi(val)); } catch(...) {}
                start = next + 1;
            }
            if (start < sList.length()) {
                std::string val = sList.substr(start);
                try { selectedStories.Push((short)std::stoi(val)); } catch(...) {}
            }
        }
    }

    if (selectedStories.IsEmpty()) {
        GS::Array<StoryData> all = StoryService::GetAllStories();
        for (const auto& s : all) selectedStories.Push(s.index);
    }

    GS::Array<ElementInfo> results;
    for (const auto& typeInfo : SupportedTypes) {
        // キー（"Wall"等）が含まれており、かつ値が true であるかを確認
        std::string keyPattern = std::string("\"") + typeInfo.key + "\"";
        size_t kPos = cmd.find(keyPattern);
        if (kPos != std::string::npos) {
            size_t colonPos = cmd.find(":", kPos);
            if (colonPos != std::string::npos) {
                // ":" の直後から次の "," または "}" までの間に "true" があるか探す
                size_t nextDelimiter = cmd.find_first_of(",}", colonPos);
                std::string valuePart = cmd.substr(colonPos + 1, nextDelimiter - colonPos - 1);
                if (valuePart.find("true") != std::string::npos) {
                    GS::Array<ElementInfo> found = ElementService::GetElementsByTypeAndStories(typeInfo.id, selectedStories);
                    for (const auto& e : found) {
                        ElementInfo info = e;
                        info.typeName = typeInfo.key;
                        results.Push(info);
                        if (results.GetSize() >= 1000) break; // 1000件で打ち切り
                    }
                }
            }
        }
        if (results.GetSize() >= 1000) break;
    }
    
    ACAPI_WriteReport(GS::UniString::Printf("Search found %u elements.", (UInt32)results.GetSize()), false);
    ExportElementsToPython(results);
}

static void MyIdleCallBack (void)
{
    if (isProcessingCommand) return;
    std::string cmd = "";
    {
        std::lock_guard<std::mutex> lock(commandMutex);
        if (!commandQueue.empty()) {
            cmd = commandQueue.front();
            commandQueue.pop();
        }
    }
    if (!cmd.empty()) {
        isProcessingCommand = true;
        DoSearch(cmd);
        isProcessingCommand = false;
    }
}

void StartCommandListener ()
{
    std::thread ([] () {
        SOCKET listenSocket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSocket == INVALID_SOCKET) return;
        sockaddr_in service;
        service.sin_family = AF_INET;
        service.sin_addr.s_addr = inet_addr ("127.0.0.1");
        service.sin_port = htons (5001);
        if (bind (listenSocket, (SOCKADDR*) &service, sizeof (service)) == SOCKET_ERROR) { closesocket (listenSocket); return; }
        listen (listenSocket, 1);
        while (true) {
            SOCKET acceptSocket = accept (listenSocket, NULL, NULL);
            if (acceptSocket != INVALID_SOCKET) {
                char buffer[16384];
                int bytesReceived = recv (acceptSocket, buffer, 16384, 0);
                if (bytesReceived > 0) {
                    std::string cmd (buffer, bytesReceived);
                    std::lock_guard<std::mutex> lock(commandMutex);
                    commandQueue.push(cmd);
                }
                closesocket (acceptSocket);
            }
        }
    }).detach ();
}

GSErrCode MenuCommandHandler (const API_MenuParams *menuParams)
{
    if (menuParams->menuItemRef.menuResID == AddOnMenuID && menuParams->menuItemRef.itemIndex == AddOnCommandID) {
        ExampleDialog dialog;
        dialog.Invoke ();
    }
    return NoError;
}

API_AddonType CheckEnvironment (API_EnvirParams* envir)
{
    RSGetIndString (&envir->addOnInfo.name, AddOnInfoID, 1, ACAPI_GetOwnResModule ());
    RSGetIndString (&envir->addOnInfo.description, AddOnInfoID, 2, ACAPI_GetOwnResModule ());
    return APIAddon_Normal;
}

GSErrCode RegisterInterface (void)
{
    return ACAPI_MenuItem_RegisterMenu (AddOnMenuID, 0, MenuCode_Tools, MenuFlag_Default);
}

GSErrCode Initialize (void)
{
    WSADATA wsaData;
    WSAStartup (MAKEWORD (2, 2), &wsaData);
    ACAPI_KeepInMemory (true);
    DGRegisterIdleCallBack (MyIdleCallBack);
    StartCommandListener ();
    IO::Location addOnLoc;
    if (ACAPI_GetOwnLocation (&addOnLoc) == NoError) {
        IO::Location searchLoc = addOnLoc;
        GS::UniString scriptPathStr;
        bool found = false;

        // アドオンの場所から最大4階層上まで server.py を探す
        for (int i = 0; i < 5; i++) {
            searchLoc.DeleteLastLocalName ();
            IO::Location testLoc = searchLoc;
            testLoc.AppendToLocal (IO::Name ("server.py"));
            bool exists = false;
            if (IO::fileSystem.Contains (testLoc, &exists) == NoError && exists) {
                testLoc.ToPath (&scriptPathStr);
                found = true;
                break;
            }
        }

        if (found) {
            std::wstring scriptPath = (const wchar_t*)scriptPathStr.ToUStr().Get();
            GS::UniString projectPathStr;
            searchLoc.ToPath (&projectPathStr);
            std::wstring projectPath = (const wchar_t*)projectPathStr.ToUStr().Get();

            auto startServer = [&](const std::wstring& pythonCmd) {
                std::wstring cmdWStr = pythonCmd + L" \"" + scriptPath + L"\"";
                
                std::vector<wchar_t> cmdBuf(cmdWStr.begin(), cmdWStr.end());
                cmdBuf.push_back(0);
                
                STARTUPINFO si = { sizeof(si) };
                si.cb = sizeof(si);
                si.dwFlags = STARTF_USESHOWWINDOW;
                si.wShowWindow = SW_SHOWNORMAL; // 通常状態で起動（GUIを表示可能にする）
                
                PROCESS_INFORMATION pi;
                // CREATE_NO_WINDOW でコンソールを隠しつつ、プロセスは通常状態で実行
                if (CreateProcess (NULL, cmdBuf.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, projectPath.c_str(), &si, &pi)) {
                    serverProcessHandle = pi.hProcess;
                    CloseHandle (pi.hThread);
                    return true;
                }
                return false;
            };

            if (startServer(L"python")) {
                ACAPI_WriteReport("Python Server started successfully with 'python'.", false);
            } else if (startServer(L"py -3")) {
                ACAPI_WriteReport("Python Server started successfully with 'py -3'.", false);
            } else {
                ACAPI_WriteReport("Failed to execute python command. Please check if python is in PATH.", true);
            }
        } else {
            ACAPI_WriteReport("Could not find 'server.py' in any parent directories of the Add-On.", true);
        }
    }
    return ACAPI_MenuItem_InstallMenuHandler (AddOnMenuID, MenuCommandHandler);
}

GSErrCode FreeData (void)
{
    DGRegisterIdleCallBack (nullptr);
    if (serverProcessHandle != NULL) { TerminateProcess (serverProcessHandle, 0); CloseHandle (serverProcessHandle); }
    WSACleanup ();
    return NoError;
}