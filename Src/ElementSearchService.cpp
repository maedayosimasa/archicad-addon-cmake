#include "ExamplePrecompiledHeader.hpp"
#include "ElementSearchService.hpp"
#include "StringConversion.hpp"

GS::Array<ElementInfo> ElementSearchService::SearchElements (API_ElemTypeID typeID, const GS::UniString& label, const GS::Array<short>& stories)
{
    GS::Array<ElementInfo> results;
    GS::Array<API_Guid> guids;
    
    if (ACAPI_Element_GetElemList (typeID, &guids) != NoError) return results;

    for (const auto& g : guids) {
        API_Element element = {};
        element.header.guid = g;
        if (ACAPI_Element_GetHeader (&element.header) != NoError) continue;

        // 階数フィルタ
        bool storyMatch = stories.IsEmpty();
        if (!storyMatch) {
            for (short sIdx : stories) {
                if (element.header.floorInd == sIdx) { storyMatch = true; break; }
            }
        }
        if (!storyMatch) continue;

        // データを構造体に詰める
        ElementInfo info;
        info.guid = g;
        info.typeName = label;
        info.floorInd = element.header.floorInd;
        info.status = "Verified";

        // ここでレイヤー名などを取得する処理を将来追加できる

        results.Push (info);
    }
    return results;
}