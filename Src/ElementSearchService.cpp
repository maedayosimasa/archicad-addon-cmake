#include "ExamplePrecompiledHeader.hpp"
#include "ElementSearchService.hpp"

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

        // ==========================================
        // Element ID の取得
        // ==========================================
        GS::UniString elementID = "-";
        
        API_ElementMemo memo = {};
        if (ACAPI_Element_GetMemo (g, &memo, APIMemoMask_ElemInfoString) == NoError) {
            if (memo.elemInfoString != nullptr) {
                elementID = *memo.elemInfoString;
            }
            ACAPI_DisposeElemMemoHdls (&memo);
        }

        // ==========================================
        // カテゴリのID を取得する処理
        // ==========================================
        GS::UniString catStr = "-";
        
        API_ElemCategory category = {};
        category.categoryID = API_ElemCategory_StructuralFunction;
        API_ElemCategoryValue catValue = {};
        if (ACAPI_Category_GetCategoryValue (g, category, &catValue) == NoError && catValue.name[0] != 0) {
            catStr = GS::UniString(catValue.name);
        } else {
            // 構造機能が空の場合は「位置」カテゴリを試す
            category.categoryID = API_ElemCategory_Position;
            if (ACAPI_Category_GetCategoryValue (g, category, &catValue) == NoError && catValue.name[0] != 0) {
                catStr = GS::UniString(catValue.name);
            }
        }

        // 結合して 4列目(Status) に表示する
        info.status = GS::UniString::Printf ("ID: %T | Cat: %T", elementID.ToPrintf (), catStr.ToPrintf ());

        results.Push (info);
    }
    return results;
}