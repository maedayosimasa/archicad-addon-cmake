#include "ExampleDialog.hpp"
#include "ACAPinc.h"

ExampleDialog::ExampleDialog () :
    DG::ModalDialog (ACAPI_GetOwnResModule (), ID_ADDON_DLG, ACAPI_GetOwnResModule ()),
    searchButton (GetReference (), 4),
    treeView     (GetReference (), 5),
    listBox      (GetReference (), 6)
{
    Attach (*this);
    searchButton.Attach (*this);

    // ツリーの初期化
    Int32 rootId = treeView.AppendItem (DG::TreeView::RootItem);
    treeView.SetItemText (rootId, "01_Archicad Objects");
    
    Int32 subId = treeView.AppendItem (rootId);
    treeView.SetItemText (subId, "Furniture Category");
    
    treeView.ExpandItem (rootId);
}

ExampleDialog::~ExampleDialog ()
{
    searchButton.Detach (*this);
    Detach (*this);
}

void ExampleDialog::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource () == &searchButton) {
        short newItemId = listBox.AppendItem ();
        listBox.SetTabItemText (newItemId, 1, "Searching...");
    }
}