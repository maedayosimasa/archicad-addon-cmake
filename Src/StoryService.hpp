#pragma once

#include "ACAPinc.h"

struct StoryData {
    short           index;
    GS::UniString   name;
};

class StoryService {
public:
    static GS::Array<StoryData> GetAllStories ();
};