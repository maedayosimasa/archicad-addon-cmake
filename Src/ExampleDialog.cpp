#include "ExamplePrecompiledHeader.hpp"
#include "ExampleDialog.hpp"
#include "ElementService.hpp"
#include "StoryService.hpp"

// ======================================================
// コンストラクタ：初期化リスト（:のあとの部分）でGRCと確実に紐付けます
// ======================================================
ExampleDialog::ExampleDialog () :
    DG::ModalDialog (ACAPI_GetOwnResModule (), ID_SEARCH_DIALOG, ACAPI_GetOwnResModule ()),
    resultTree   (GetReference (), ResultTreeID),  // ← 最重要：実体を紐付け
    searchButton (GetReference (), SearchButtonID),
    wallCheck    (GetReference (), WallCheckID),
    columnCheck  (GetReference (), ColumnCheckID),
    beamCheck    (GetReference (), BeamCheckID),
    slabCheck    (GetReference (), SlabCheckID),
    winDoorCheck (GetReference (), WinDoorCheckID),
    objectCheck  (GetReference (), ObjectCheckID),
    storyCheck1  (GetReference (), Story1CheckID), // 階数チェックも初期化
    storyCheck2  (GetReference (), Story2CheckID),
    storyCheck3  (GetReference (), Story3CheckID),
    storyCheck4  (GetReference (), Story4CheckID)
{
    Attach (*this);
    searchButton.Attach (*this);
    
    // 診断用：もし紐付けに失敗していればレポートを出す
    if (!resultTree.IsValid ()) {
        ACAPI_WriteReport ("Error: TreeView binding failed! Check GRC ID.", true);
    }
}

// ======================================================
// デストラクタ：newしていないので delete は一切不要です
// ======================================================
ExampleDialog::~ExampleDialog ()
{
    searchButton.Detach (*this);
    Detach (*this);
}

// ======================================================
// パネルが開いた時
// ======================================================
void ExampleDialog::PanelOpened (const DG::PanelOpenEvent&)
{
    auto stories = StoryService::GetAllStories ();
    // 処理しやすくするためにポインタの配列にまとめる（宣言のみ）
    DG::CheckBox* storyChecks[] = { &storyCheck1, &storyCheck2, &storyCheck3, &storyCheck4 };

    for (int i = 0; i < 4; i++) {
        if (i < (int)stories.GetSize ()) {
            storyChecks[i]->SetText (stories[i].name);
            storyChecks[i]->Enable ();
        } else {
            storyChecks[i]->Disable ();
        }
    }
}

// ======================================================
// ボタンクリック
// ======================================================
void ExampleDialog::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource () == &searchButton) {
        RefreshElementList ();
    }
}

// ======================================================
// リストの更新
// ======================================================
void ExampleDialog::RefreshElementList ()
{
    // 描画を一時停止（大量データ追加時の画面のチラつきと処理落ちを防ぐため）
    resultTree.DisableDraw ();

    // ★ もし DeleteAllItems() でエラーになる場合はこちらを使用
    resultTree.DeleteItem (DG::TreeView::AllItems); 

    auto stories = GetSelectedStories ();
    Int32 totalFound = 0;

    if (wallCheck.IsChecked ())    SearchAndAdd (API_WallID,   "Walls",   stories, totalFound);
    if (columnCheck.IsChecked ())  SearchAndAdd (API_ColumnID, "Columns", stories, totalFound);
    if (beamCheck.IsChecked ())    SearchAndAdd (API_BeamID,   "Beams",   stories, totalFound);
    if (slabCheck.IsChecked ())    SearchAndAdd (API_SlabID,   "Slabs",   stories, totalFound);
    if (winDoorCheck.IsChecked ()) SearchAndAdd (API_WindowID, "Windows", stories, totalFound);
    if (objectCheck.IsChecked ())  SearchAndAdd (API_ObjectID, "Objects", stories, totalFound);

    // 描画を再開して強制アップデート
    resultTree.EnableDraw ();
    resultTree.RedrawItem (DG::TreeView::RootItem);

    // GS::UniString reportMsg = GS::UniString::Printf ("Found: %d elements", (int)totalFound);
    // ACAPI_WriteReport (reportMsg, true);
}

// ======================================================
// 要素検索とツリー追加
// ======================================================
void ExampleDialog::SearchAndAdd (API_ElemTypeID typeID, const char* label, const GS::Array<short>& stories, Int32& totalCount)
{
    auto guids = ElementService::GetElementsByTypeAndStories (typeID, stories);
    
    if (!guids.IsEmpty ()) {
        // [修正] アロー演算子(->) ではなく ドット(.) を使用
        Int32 root = resultTree.AppendItem (DG::TreeView::RootItem);
        
        if (root != DG::TreeView::NoItem) {
            resultTree.SetItemText (root, GS::UniString::Printf ("%s (%d)", label, (int)guids.GetSize ()));

            for (const auto& g : guids) {
                Int32 child = resultTree.InsertItem (root, DG::TreeView::BottomItem);
                if (child != DG::TreeView::NoItem) {
                    resultTree.SetItemText (child, APIGuid2GSGuid(g).ToUniString());
                }
            }
            resultTree.ExpandItem (root);
        }
        totalCount += (Int32)guids.GetSize ();
    }
}

// ======================================================
// 選択階数の取得
// ======================================================
GS::Array<short> ExampleDialog::GetSelectedStories ()
{
    GS::Array<short> result;
    auto stories = StoryService::GetAllStories ();
    DG::CheckBox* storyChecks[] = { &storyCheck1, &storyCheck2, &storyCheck3, &storyCheck4 };

    for (int i = 0; i < 4; i++) {
        if (i < (int)stories.GetSize () && storyChecks[i]->IsChecked ()) {
            result.Push (stories[i].index);
        }
    }
    if (result.IsEmpty ()) {
        for (const auto& s : stories) result.Push (s.index);
    }
    return result;
}