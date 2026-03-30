#ifndef ELEMENT_SERVICE_HPP
#define ELEMENT_SERVICE_HPP

#include "ACAPinc.h"

// 検索結果をまとめる箱
struct ElementInfo {
    API_Guid guid;
    short    floorInd;
    // 将来、レイヤー名や面積が必要になったらここに足す
};

class ElementService {
public:
    // 戻り値を API_Guid の配列から、ElementInfo の配列に変更
    static GS::Array<ElementInfo> GetElementsByTypeAndStories (API_ElemTypeID typeID, const GS::Array<short>& stories);
};

#endif