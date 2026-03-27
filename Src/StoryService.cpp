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

        // ★ AC28対応（nameが無いので仮生成）
        data.name = GS::UniString::Printf("Floor %d", story.index);

        result.Push(data);
    }

    if (storyInfo.data != nullptr)
        BMKillHandle((GSHandle*)&storyInfo.data);

    return result;
}