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

   // インデックスが0以上（地上）なら +1 して表示、負（地下）ならそのまま表示
    Int32 displayFloorNum = (story.index >= 0) ? (story.index + 1) : story.index;
    
    data.name = GS::UniString::Printf("Floor %d", displayFloorNum);
    // --- ここまで修正 ---

    result.Push(data);
}

    if (storyInfo.data != nullptr)
        BMKillHandle((GSHandle*)&storyInfo.data);

    return result;
}