#include "ACAPinc.h"
#include "ElementService.hpp"

GS::Array<API_Guid> ElementService::GetElementsByTypeAndStories (
    API_ElemTypeID typeID,
    const GS::Array<short>& stories
)
{
    GS::Array<API_Guid> result;
    GS::Array<API_Guid> allElementGuids;

    API_ElemType type (typeID);
    GSErrCode err = ACAPI_Element_GetElemList (type, &allElementGuids);
    
    if (err != NoError || allElementGuids.IsEmpty()) return result;

    for (const auto& guid : allElementGuids) {
        // 階数チェックを一旦スルーして、見つかったものをすべてリストに入れるテスト
        // これで表示されるなら、原因は「stories（階数）の判定不一致」です。
        if (stories.IsEmpty()) {
             result.Push(guid);
        } else {
            API_Elem_Head header = {};
            header.guid = guid;
            header.type = type;
            if (ACAPI_Element_GetHeader (&header) == NoError) {
                // ここで階数を比較
                for (short s : stories) {
                    if (header.floorInd == s) {
                        result.Push (guid);
                        break;
                    }
                }
            }
        }
    }
    return result;
}