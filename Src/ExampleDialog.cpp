#include "ExamplePrecompiledHeader.hpp"

// Archicad API 標準ヘッダー
#include "ACAPinc.h"

// プロジェクト固有ヘッダー
#include "ExampleDialog.hpp"
#include "ResultDialog.hpp"         // 結果ダイアログ (32502)
#include "ElementSearchService.hpp" // 検索サービス
#include "StoryService.hpp"
#include "ResourceIds.hpp"

// ======================================================
// コンストラクタ
// ======================================================
ExampleDialog::ExampleDialog () :
    DG::ModalDialog (ACAPI_GetOwnResModule (), 32501, ACAPI_GetOwnResModule ()),
    searchButton (GetReference (), SearchButtonID),
    wallCheck    (GetReference (), WallCheckID),
    columnCheck  (GetReference (), ColumnCheckID),
    beamCheck    (GetReference (), BeamCheckID),
    slabCheck    (GetReference (), SlabCheckID),
    winDoorCheck (GetReference (), WinDoorCheckID),
    objectCheck  (GetReference (), ObjectCheckID),
    storyCheck1  (GetReference (), Story1CheckID),
    storyCheck2  (GetReference (), Story2CheckID),
    storyCheck3  (GetReference (), Story3CheckID),
    storyCheck4  (GetReference (), Story4CheckID)
{
    Attach (*this);
    searchButton.Attach (*this);
    // ※ resultList の Attach はここから削除しました
}

// ======================================================
// デストラクタ
// ======================================================
ExampleDialog::~ExampleDialog ()
{
    Detach (*this);
    searchButton.Detach (*this);
    // ※ resultList の Detach も削除しました
}

// ======================================================
// パネル初期設定 (PanelOpened)
// ※ 32501にはリストがないので、中身は空でOKです
// ======================================================
void ExampleDialog::PanelOpened (const DG::PanelOpenEvent&)
{
    // 何もする必要はありません
}

// ======================================================
// ボタンクリックイベント (司令塔)
// ======================================================
void ExampleDialog::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource () == &searchButton) {
        auto stories = GetSelectedStories ();
        GS::Array<ElementInfo> allFound; // 全ツールの結果を格納する配列

        // 各ツールのチェック状態を確認し、結果を Append (追加) していく
        if (wallCheck.IsChecked ()) {
            allFound.Append (ElementSearchService::SearchElements (API_WallID, "Wall", stories));
        }
        if (columnCheck.IsChecked ()) {
            allFound.Append (ElementSearchService::SearchElements (API_ColumnID, "Column", stories));
        }
        if (beamCheck.IsChecked ()) {
            allFound.Append (ElementSearchService::SearchElements (API_BeamID, "Beam", stories));
        }
        if (slabCheck.IsChecked ()) {
            allFound.Append (ElementSearchService::SearchElements (API_SlabID, "Slab", stories));
        }
        if (winDoorCheck.IsChecked ()) {
            // 窓とドアを両方検索
            allFound.Append (ElementSearchService::SearchElements (API_WindowID, "Window", stories));
            allFound.Append (ElementSearchService::SearchElements (API_DoorID, "Door", stories));
        }
        if (objectCheck.IsChecked ()) {
            allFound.Append (ElementSearchService::SearchElements (API_ObjectID, "Object", stories));
        }

        if (allFound.IsEmpty ()) {
            DG::ErrorAlert ("Search results", "No elements found.", "OK");
            return;
        }

        // ==========================================================
        // ★ここが超重要★
        // 検索で集めたデータ (allFound) を 32502番のダイアログに渡して開く
        // ==========================================================
        ResultDialog resDialog (allFound);
        resDialog.Invoke (); 
    }
}

// ======================================================
// 補助関数：UIから選択された階数インデックスを取得
// ======================================================
GS::Array<short> ExampleDialog::GetSelectedStories ()
{
    GS::Array<short> result;
    auto allStories = StoryService::GetAllStories ();
    DG::CheckBox* storyChecks[] = { &storyCheck1, &storyCheck2, &storyCheck3, &storyCheck4 };

    for (int i = 0; i < 4; i++) {
        if (i < (int)allStories.GetSize () && storyChecks[i]->IsChecked ()) {
            result.Push (allStories[i].index);
        }
    }

    if (result.IsEmpty ()) {
        for (const auto& s : allStories) result.Push (s.index);
    }
    return result;
}

// ======================================================
// ※ RefreshElementList() と SearchAndAdd() は
//    このクラスではもう使わないため、完全に削除しました。
// ======================================================