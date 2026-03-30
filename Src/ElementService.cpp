#include "ElementService.hpp"

GS::Array<ElementInfo> ElementService::GetElementsByTypeAndStories (
    API_ElemTypeID typeID,
    const GS::Array<short>& stories
)
{
    GS::Array<ElementInfo> result;
    GS::Array<API_Guid> allElementGuids;

    API_ElemType type (typeID);
    GSErrCode err = ACAPI_Element_GetElemList (type, &allElementGuids);
    
    if (err != NoError || allElementGuids.IsEmpty()) return result;

    for (const auto& guid : allElementGuids) {
        API_Elem_Head header = {};
        header.guid = guid;
        header.type = type;

        if (ACAPI_Element_GetHeader (&header) == NoError) {
            // 階数チェック
            bool isMatch = stories.IsEmpty();
            if (!isMatch) {
                for (short s : stories) {
                    if (header.floorInd == s) {
                        isMatch = true;
                        break;
                    }
                }
            }

            if (isMatch) {
                // GUIDだけでなく、取得済みの階数情報も一緒に返す！
                ElementInfo info;
                info.guid = guid;
                info.floorInd = header.floorInd;
                
                result.Push (info);
            }
        }
    }
    return result;
}