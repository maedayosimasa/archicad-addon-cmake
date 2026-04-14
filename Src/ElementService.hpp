#ifndef ELEMENT_SERVICE_HPP
#define ELEMENT_SERVICE_HPP

#include "ACAPinc.h"
#include "ElementInfo.hpp"

class ElementService {
public:
    // 戻り値を API_Guid の配列から、ElementInfo の配列に変更
    static GS::Array<ElementInfo> GetElementsByTypeAndStories (API_ElemTypeID typeID, const GS::Array<short>& stories);
};

#endif