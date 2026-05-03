#include "ExamplePrecompiledHeader.hpp"
#include "StoryService.hpp"

GS::Array<StoryData> StoryService::GetAllStories ()
{
    GS::Array<StoryData> result;

    API_StoryInfo storyInfo;
    BNZeroMemory(&storyInfo, sizeof(API_StoryInfo));

    GSErrCode err = ACAPI_ProjectSetting_GetStorySettings(&storyInfo);
    if (err != NoError) {
        DBPrintf("[StoryService] GetStorySettings failed: %d\n", err);
        return result;
    }

    Int32 storyCount = storyInfo.lastStory - storyInfo.firstStory + 1;

    for (Int32 i = 0; i < storyCount; i++) {
        const API_StoryType& story = (*storyInfo.data)[i];

        StoryData data;
        data.index = story.index;
        data.level = story.level;

        // 階高
        if (i < storyCount - 1)
            data.height = (*storyInfo.data)[i + 1].level - story.level;
        else
            data.height = 0.0;

        // 表示階番号
        Int32 displayFloorNum = (story.index >= 0) ? (story.index + 1) : story.index;

        GS::UniString storyName(story.uName);

        data.name = GS::UniString::Printf(
            "%d Floor (%T)",
            displayFloorNum,
            storyName.ToPrintf()
        );

        result.Push(data);
    }

    if (storyInfo.data != nullptr)
        BMKillHandle(reinterpret_cast<GSHandle*>(&storyInfo.data));

    return result;
}