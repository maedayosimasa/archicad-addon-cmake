#include "ACAPinc.h"
#include "ElementService.hpp"

GS::Array<API_Guid> ElementService::GetElementsByTypeAndStories (
    API_ElemTypeID type,
    const GS::Array<short>& stories
)
{
    GS::Array<API_Guid> result;
    GS::Array<API_Guid> all;

    if (ACAPI_Element_GetElemList (type, &all) != NoError)
        return result;

    for (const auto& guid : all) {

        API_Element elem = {};
        elem.header.guid = guid;

        if (ACAPI_Element_Get (&elem) == NoError) {

            for (short s : stories) {
                if (elem.header.floorInd == s) {
                    result.Push (guid);
                    break;
                }
            }
        }
    }

    return result;
}