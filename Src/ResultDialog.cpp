#include "ExamplePrecompiledHeader.hpp"
#include "APIEnvir.h"
#include "ACAPinc.h"
#include "ResultDialog.hpp"

ResultDialog::ResultDialog (const GS::Array<ElementInfo>& data) :
    DG::ModalDialog (ACAPI_GetOwnResModule (), ID_RESULT_DIALOG, ACAPI_GetOwnResModule ()),
    resultList  (GetReference (), ResultListID),
    closeButton (GetReference (), CloseButtonID),
    displayData (data)
{
    Attach (*this);
    resultList.Attach (*this);
    closeButton.Attach (*this);
}

ResultDialog::~ResultDialog ()
{
    Detach (*this);
    resultList.Detach (*this);
    closeButton.Detach (*this);
}

void ResultDialog::PanelOpened (const DG::PanelOpenEvent&)
{
    resultList.DisableDraw ();
    
    GSResModule resModule = ACAPI_GetOwnResModule ();
    this->SetTitle (RSGetIndString (32502, 3, resModule));
    closeButton.SetText (RSGetIndString (32502, 2, resModule));

    // カラム設定
    resultList.SetTabFieldCount (4);
    resultList.SetTabFieldProperties (1, 0,   220, DG::ListBox::Left, DG::ListBox::EndTruncate, true);
    resultList.SetTabFieldProperties (2, 225, 350, DG::ListBox::Left, DG::ListBox::EndTruncate, true);
    resultList.SetTabFieldProperties (3, 355, 450, DG::ListBox::Left, DG::ListBox::EndTruncate, true);
    resultList.SetTabFieldProperties (4, 455, 780, DG::ListBox::Left, DG::ListBox::EndTruncate, true);

    resultList.SetHeaderItemText (1, "GUID");
    resultList.SetHeaderItemText (2, "タイプ");
    resultList.SetHeaderItemText (3, "フロア");
    resultList.SetHeaderItemText (4, "要素ID / カテゴリ");

    for (const auto& info : displayData) {
        resultList.InsertItem (DG::ListBox::BottomItem);
        short row = (short)resultList.GetItemCount ();
        
        resultList.SetTabItemText (row, 1, APIGuid2GSGuid (info.guid).ToUniString ());
        resultList.SetTabItemText (row, 2, info.typeName);
        
        Int32 displayFloor = (info.floorInd >= 0) ? (info.floorInd + 1) : info.floorInd;
        resultList.SetTabItemText (row, 3, GS::UniString::Printf("%d 階", displayFloor));
        resultList.SetTabItemText (row, 4, info.status);
    }

    resultList.EnableDraw ();
}

void ResultDialog::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource () == &closeButton) {
        this->PostCloseRequest (DG::ModalDialog::Accept); 
    }
}
