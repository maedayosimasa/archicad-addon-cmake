#pragma once

#include "ACAPinc.h"

class ElementService {
public:
    static GS::Array<API_Guid> GetElementsByTypeAndStories (
        API_ElemTypeID type,
        const GS::Array<short>& stories
    );
};