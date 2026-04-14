#ifndef ELEMENT_INFO_HPP
#define ELEMENT_INFO_HPP

#include "APIEnvir.h"
#include "ACAPinc.h"

struct ElementInfo {
    API_Guid        guid;
    GS::UniString   typeName;
    short           floorInd;
    GS::UniString   status;
    
    // 今後増やしたい項目はここに足していく
    // GS::UniString   layerName;
    // double          area;
};

#endif