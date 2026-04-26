#ifndef ELEMENT_SERVICE_HPP
#define ELEMENT_SERVICE_HPP

#include "ACAPinc.h"
#include "ElementInfo.hpp"

class ElementService {
public:
    static GS::Array<ElementInfo> GetElementsByTypeAndStories (API_ElemTypeID typeID, const GS::Array<short>& stories);
    static bool CheckElementPresence (API_ElemTypeID typeID);
};

#endif