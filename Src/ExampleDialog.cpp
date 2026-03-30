#include "ExamplePrecompiledHeader.hpp"

// Archicad API 標準ヘッダー
#include "ACAPinc.h"

// プロジェクト固有ヘッダー
#include "ExampleDialog.hpp"
#include "ElementSearchService.hpp" // 検索サービス（ElementInfoを返すもの）
#include "StoryService.hpp"
#include "ResourceIds.hpp"
#include "StringConversion.hpp" // APIGuidToString 用

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
    // Observerの接続
    resultList.Attach (*static_cast<DG::ListBoxObserver*> (this)); 
}

ExampleDialog::~ExampleDialog ()
{
    Detach (*this);
    searchButton.Detach (*this);
    resultList.Detach (*static_cast<DG::ListBoxObserver*> (this));
}

// ======================================================
// パネル初期設定 (PanelOpened)
// ======================================================
void ExampleDialog::PanelOpened (const DG::PanelOpenEvent&)
{
    resultList.DisableDraw ();
    
    // カラムの設定（4列）
    resultList.SetTabFieldCount (4);
    
    // 各列の幅と属性を定義
    resultList.SetTabFieldProperties (1, 0,   180, DG::ListBox::Left, DG::ListBox::EndTruncate, true);
    resultList.SetTabFieldProperties (2, 185, 350, DG::ListBox::Left, DG::ListBox::EndTruncate, true);
    resultList.SetTabFieldProperties (3, 355, 450, DG::ListBox::Left, DG::ListBox::EndTruncate, true);
    resultList.SetTabFieldProperties (4, 455, 580, DG::ListBox::Left, DG::ListBox::EndTruncate, true);

    // --- 修正箇所：エラーの出た SetHeaderType を削除し、テキストセットのみにする ---
    resultList.SetHeaderItemText (1, "GUID");
    resultList.SetHeaderItemText (2, "Element Type");
    resultList.SetHeaderItemText (3, "Story No.");
    resultList.SetHeaderItemText (4, "Status");

    resultList.EnableDraw ();
}
// ======================================================
// ボタンクリックイベント (司令塔)
// ======================================================
void ExampleDialog::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource () == &searchButton) {
        // リストの表示更新
        RefreshElementList ();
    }
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

    // 2. 選択されている階数を取得 (StoryService経由)
    auto stories = GetSelectedStories ();
    Int32 totalFound = 0;

    // 3. 各ツールごとに検索して追加
    // SearchAndAddがサービス(ElementSearchService)を呼び出す
    if (wallCheck.IsChecked ())    SearchAndAdd (API_WallID,   "Wall",    stories, totalFound);
    if (columnCheck.IsChecked ())  SearchAndAdd (API_ColumnID, "Column",  stories, totalFound);
    if (beamCheck.IsChecked ())    SearchAndAdd (API_BeamID,   "Beam",    stories, totalFound);
    if (slabCheck.IsChecked ())    SearchAndAdd (API_SlabID,   "Slab",    stories, totalFound);

    // 4. 反映
    resultList.EnableDraw ();
    resultList.Redraw ();

    // 5. 結果を通知
    ACAPI_WriteReport (GS::UniString::Printf ("Search Complete: %d items added.", totalFound), false);
}

// ======================================================
// 要素検索とリスト追加 (サービス連携版)
// ======================================================
void ExampleDialog::SearchAndAdd (API_ElemTypeID typeID, const char* label, const GS::Array<short>& stories, Int32& totalCount)
{
    // 1. サービス(ElementSearchService)に検索を依頼
    // サービス側で階数判定(stories比較)まで完了したデータが返ってくる
    GS::Array<ElementInfo> foundElements = ElementSearchService::SearchElements (typeID, GS::UniString (label), stories);

    // 2. 受け取った結果をリストボックスに書き込む（UI操作に特化）
    for (const auto& info : foundElements) {
        resultList.InsertItem (DG::ListBox::BottomItem);
        short currentRow = (short) resultList.GetItemCount ();

        resultList.SetTabItemText (currentRow, 1, APIGuidToString (info.guid));
        resultList.SetTabItemText (currentRow, 2, info.typeName);
        
        // 階数表示ロジック（地上階は+1、地下階はそのまま）
        Int32 displayFloor = (info.floorInd >= 0) ? (info.floorInd + 1) : info.floorInd;
        resultList.SetTabItemText (currentRow, 3, GS::UniString::Printf ("Floor %d", displayFloor));
        
        resultList.SetTabItemText (currentRow, 4, info.status);

        // 管理用GUID配列に保存（後で要素を選択する際に使用）
        itemGuids.Push (info.guid);
        totalCount++;
    }
}

// ======================================================
// 補助関数：UIから選択された階数インデックスを取得
// ======================================================
GS::Array<short> ExampleDialog::GetSelectedStories ()
{
    GS::Array<short> result;
    // 全階層の情報を取得
    auto allStories = StoryService::GetAllStories ();
    DG::CheckBox* storyChecks[] = { &storyCheck1, &storyCheck2, &storyCheck3, &storyCheck4 };

    for (int i = 0; i < 4; i++) {
        // UIのチェックが入っている階の「index」を収集
        if (i < (int)allStories.GetSize () && storyChecks[i]->IsChecked ()) {
            result.Push (allStories[i].index);
        }
    }

    // もし1つもチェックされていない場合は、便宜上「全階層」を対象とする
    if (result.IsEmpty ()) {
        for (const auto& s : allStories) result.Push (s.index);
    }
    return result;
}