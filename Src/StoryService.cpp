
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
        
        // 階高の計算（最終階以外は次の階との差分）
        if (i < storyCount - 1) {
            data.height = (*storyInfo.data)[i + 1].level - story.level;
        } else {
            data.height = 0.0; // 最終階の階高は0または規定値
        }

        // 表示用の名前を生成
        Int32 displayFloorNum = (story.index >= 0) ? (story.index + 1) : story.index;
        GS::UniString storyName (story.uName);
        data.name = GS::UniString::Printf("%d Floor (%s)", displayFloorNum, (const char*)storyName.ToCStr());

        result.Push(data);
    }

    if (storyInfo.data != nullptr)
        BMKillHandle((GSHandle*)&storyInfo.data);

    return result;
}