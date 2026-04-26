#ifndef ELEMENT_SEARCH_SERVICE_HPP
#define ELEMENT_SEARCH_SERVICE_HPP

#include "ElementInfo.hpp"

class ElementSearchService {
public:
    // 特定のタイプ（壁など）と指定階層から要素を検索してリストを返す
    static GS::Array<ElementInfo> SearchElements (API_ElemTypeID typeID, 
                                                  const GS::UniString& label, 
                                                  const GS::Array<short>& stories);
};

#endif