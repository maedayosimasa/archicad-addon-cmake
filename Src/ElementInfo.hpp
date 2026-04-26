#ifndef ELEMENT_INFO_HPP
#define ELEMENT_INFO_HPP

#include "APIEnvir.h"
#include "ACAPinc.h"

struct ElementInfo {
    API_Guid        guid;
    GS::UniString   typeName;
    short           floorInd;
    GS::UniString   elementID;
    GS::UniString   categoryID;
    GS::UniString   structuralFunction;
    GS::UniString   position;
    double          width;
    double          height;
};

#endif