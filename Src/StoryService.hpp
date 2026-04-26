#pragma once

#include "ACAPinc.h"

struct StoryData {
    short           index;
    GS::UniString   name;
    double          level;
    double          height;
};

class StoryService {
public:
    static GS::Array<StoryData> GetAllStories ();
};