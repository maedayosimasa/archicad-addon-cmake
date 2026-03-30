#include "ExamplePrecompiledHeader.hpp"

// Archicad API 標準ヘッダー
#include "ACAPinc.h"

// プロジェクト固有ヘッダー
#include "ExampleDialog.hpp"
#include "ElementService.hpp"
#include "StoryService.hpp"
#include "ResourceIds.hpp"

// ======================================================
// コンストラクタ / デストラクタ
// ======================================================
ExampleDialog::ExampleDialog () :
    DG::ModalDialog (ACAPI_GetOwnResModule (), 32501, ACAPI_GetOwnResModule ()),
    resultList   (GetReference (), ResultTreeID),
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
    resultList.Attach (*static_cast<DG::ListBoxObserver*> (this)); 
}

ExampleDialog::~ExampleDialog ()
{
    Detach (*this);
    searchButton.Detach (*this);
    resultList.Detach (*static_cast<DG::ListBoxObserver*> (this));
}

// ======================================================
// パネル初期設定
// ======================================================
void ExampleDialog::PanelOpened (const DG::PanelOpenEvent&)
{
    resultList.DisableDraw ();
    
    // カラムの設定（4列）
    resultList.SetTabFieldCount (4);
    
    // 各列の幅と属性を定義（のヘッダー反映用）
    resultList.SetTabFieldProperties (1, 0,   180, DG::ListBox::Left, DG::ListBox::EndTruncate, true);
    resultList.SetTabFieldProperties (2, 185, 350, DG::ListBox::Left, DG::ListBox::EndTruncate, true);
    resultList.SetTabFieldProperties (3, 355, 450, DG::ListBox::Left, DG::ListBox::EndTruncate, true);
    resultList.SetTabFieldProperties (4, 455, 580, DG::ListBox::Left, DG::ListBox::EndTruncate, true);

    resultList.SetHeaderItemText (1, "GUID");
    resultList.SetHeaderItemText (2, "Element Type");
    resultList.SetHeaderItemText (3, "Story No.");
    resultList.SetHeaderItemText (4, "Status");

    resultList.EnableDraw ();
}

// ======================================================
// リスト更新処理
// ======================================================
void ExampleDialog::RefreshElementList ()
{
    resultList.DisableDraw ();
    
    // 1. リストを空にする
    resultList.DeleteItem (DG::ListBox::AllItems); 
    itemGuids.Clear (); 

    // 2. 選択されている階数を取得
    auto stories = GetSelectedStories ();
    Int32 totalFound = 0;

    // 3. 各ツールごとに検索して追加（SearchAndAddの中でInsertItemが行われる）
    if (wallCheck.IsChecked ())    SearchAndAdd (API_WallID,   "Wall",    stories, totalFound);
    if (columnCheck.IsChecked ())  SearchAndAdd (API_ColumnID, "Column",  stories, totalFound);
    if (beamCheck.IsChecked ())    SearchAndAdd (API_BeamID,   "Beam",    stories, totalFound);
    if (slabCheck.IsChecked ())    SearchAndAdd (API_SlabID,   "Slab",    stories, totalFound);

    // 4. 反映
    resultList.EnableDraw ();
    resultList.Redraw ();

    // 5. 結果をレポートパレットに表示（正しく動いているか確認するため）
    ACAPI_WriteReport (GS::UniString::Printf ("Build List: %d items added.", totalFound), false);
}
/// ======================================================
// 要素検索とリスト追加 (修正版)
// ======================================================
void ExampleDialog::SearchAndAdd (API_ElemTypeID typeID, const char* label, const GS::Array<short>& stories, Int32& totalCount)
{
    GS::Array<API_Guid> guids;
    if (ACAPI_Element_GetElemList (typeID, &guids) != NoError) return;

    for (const auto& g : guids) {
        API_Element element = {};
        element.header.guid = g;
        if (ACAPI_Element_GetHeader (&element.header) != NoError) continue;

        // 階数フィルタ
        bool storyMatch = stories.IsEmpty();
        if (!storyMatch) {
            for (short sIdx : stories) {
                if (element.header.floorInd == sIdx) { storyMatch = true; break; }
            }
        }
        if (!storyMatch) continue;

        // --- 修正の要：物理的に行を増やし、その行番号を特定する ---
        resultList.InsertItem (DG::ListBox::BottomItem); // 末尾に行を挿入
        short currentRow = (short) resultList.GetItemCount (); // 追加された最新の行番号を取得

        // currentRow（1, 2, 3...）に対して値をセットすることで「書き足し」を実現
        resultList.SetTabItemText (currentRow, 1, APIGuidToString(g));
        resultList.SetTabItemText (currentRow, 2, GS::UniString(label));
        resultList.SetTabItemText (currentRow, 3, GS::UniString::Printf("%d", element.header.floorInd + 1));
        resultList.SetTabItemText (currentRow, 4, "Verified");

        itemGuids.Push (g);
        totalCount++;
    }
}
// ======================================================
// 補助関数：階数取得 (二重定義を解消)
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
    // 何もチェックされていない場合は全階を対象とする
    if (result.IsEmpty ()) {
        for (const auto& s : stories) result.Push (s.index);
    }
    return result;
}

// ======================================================
// 補助関数：選択要素取得 (二重定義を解消)
// ======================================================
GS::Array<API_Guid> ExampleDialog::GetSelectedElements ()
{
    GS::Array<API_Guid> selectedGuids;
    API_SelectionInfo selectionInfo;
    GS::Array<API_Neig> selNeigs;

    if (ACAPI_Selection_Get (&selectionInfo, &selNeigs, false) == NoError) {
        for (const auto& neig : selNeigs) {
            selectedGuids.Push (neig.guid);
        }
    }
    return selectedGuids;
}

// --- この関数が丸ごと抜けている、または名前が間違っている可能性があります ---
void ExampleDialog::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource () == &searchButton) {
        resultList.DisableDraw ();
        
        // 1. 全データを消去してリセット
        resultList.DeleteItem (DG::ListBox::AllItems); 
        itemGuids.Clear ();
        
        auto stories = GetSelectedStories ();
        Int32 total = 0;

        // 2. 各ツールを検索（SearchAndAddの中でDeleteItemを呼んではいけない）
        if (wallCheck.IsChecked ())    SearchAndAdd (API_WallID,   "Wall",   stories, total);
        if (columnCheck.IsChecked ())  SearchAndAdd (API_ColumnID, "Column", stories, total);
        if (beamCheck.IsChecked ())    SearchAndAdd (API_BeamID,   "Beam",   stories, total);
        if (slabCheck.IsChecked ())    SearchAndAdd (API_SlabID,   "Slab",   stories, total);

        resultList.EnableDraw ();
        resultList.Redraw (); // 強制再描画
        
        // デバッグレポート出力
        ACAPI_WriteReport (GS::UniString::Printf ("Search Complete: %d items added.", total), false);
    }
}