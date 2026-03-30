#include "ExamplePrecompiledHeader.hpp" // もしあれば
#include "APIEnvir.h"         // 1. 環境設定
#include "ACAPinc.h"          // 2. API基本
#include "ResultDialog.hpp"   // 3. 自身のヘッダー
#include "StringConversion.hpp"

ResultDialog::ResultDialog (const GS::Array<ElementInfo>& data) :
    // GRCの新しいリソースID（例: 32502）を指定
    DG::ModalDialog (ACAPI_GetOwnResModule (), 32502, ACAPI_GetOwnResModule ()),
    resultList  (GetReference (), ResultListID),
    closeButton (GetReference (), CloseButtonID),
    displayData (data)
{
    Attach (*this);
    resultList.Attach (*this);
    closeButton.Attach (*this); // ★ここ：ボタンのイベントを監視
}

ResultDialog::~ResultDialog ()
{
    Detach (*this);
    resultList.Detach (*this);
    closeButton.Detach (*this); // ★ここ：後始末も忘れずに
}

void ResultDialog::PanelOpened (const DG::PanelOpenEvent&)
{
    resultList.DisableDraw ();
    
    // カラム設定（テーブル表示用）
    resultList.SetTabFieldCount (4);
    resultList.SetTabFieldProperties (1, 0,   180, DG::ListBox::Left, DG::ListBox::EndTruncate, true);
    resultList.SetTabFieldProperties (2, 185, 350, DG::ListBox::Left, DG::ListBox::EndTruncate, true);
    resultList.SetTabFieldProperties (3, 355, 450, DG::ListBox::Left, DG::ListBox::EndTruncate, true);
    resultList.SetTabFieldProperties (4, 455, 580, DG::ListBox::Left, DG::ListBox::EndTruncate, true);

    resultList.SetHeaderItemText (1, "GUID");
    resultList.SetHeaderItemText (2, "Type");
    resultList.SetHeaderItemText (3, "Story");
    resultList.SetHeaderItemText (4, "Status");

    // データの流し込み
    for (const auto& info : displayData) {
        resultList.InsertItem (DG::ListBox::BottomItem);
        short row = (short)resultList.GetItemCount ();
        
        resultList.SetTabItemText (row, 1, APIGuidToString(info.guid));
        resultList.SetTabItemText (row, 2, info.typeName);
        
        Int32 displayFloor = (info.floorInd >= 0) ? (info.floorInd + 1) : info.floorInd;
        resultList.SetTabItemText (row, 3, GS::UniString::Printf("Floor %d", displayFloor));
        resultList.SetTabItemText (row, 4, info.status);
    }

    resultList.EnableDraw ();
}

// ★追加：ボタンがクリックされた時の処理
// ResultDialog.cpp

void ResultDialog::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource () == &closeButton) {
        // Archicad 28 ModalDialog を閉じるための正しいメソッド
        // Accept (OK相当) または Cancel を引数に取ります
        this->PostCloseRequest (DG::ModalDialog::Accept); 
    }
}