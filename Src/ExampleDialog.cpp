<<<<<<< HEAD
﻿#include "ACAPinc.h"

#include "ExampleDialog.hpp"

#include "UniString.hpp"
#include "BM.hpp"

ExampleDialog::ExampleDialog () :
    DG::ModalDialog (ACAPI_GetOwnResModule (), ID_SEARCH_DIALOG, ACAPI_GetOwnResModule ()),
    resultTree   (GetReference (), ResultTreeID),
    searchButton (GetReference (), SearchButtonID),
    wallCheck    (GetReference (), WallCheckID),
    columnCheck  (GetReference (), ColumnCheckID),
    beamCheck    (GetReference (), BeamCheckID),
    slabCheck    (GetReference (), SlabCheckID),
    winDoorCheck (GetReference (), WinDoorCheckID),
    objectCheck  (GetReference (), ObjectCheckID)
{
    Attach (*this);
    searchButton.Attach (*this);

    wallCheck.Check ();
    columnCheck.Check ();
    beamCheck.Check ();
    slabCheck.Check ();
    winDoorCheck.Check ();
    objectCheck.Check ();
=======
﻿#include "ExampleDialog.hpp"

ExampleDialog::ExampleDialog () :
    DG::ModalDialog (ACAPI_GetOwnResModule (), 32500, ACAPI_GetOwnResModule ()),
    searchButton (GetReference (), 1),
    messageLabel (GetReference (), 2),
    elementList  (GetReference (), 3)
{
    searchButton.Attach (*this);
    elementList.Attach (*this);
    Attach (*this);
>>>>>>> f6aad847eb0dc3cdce4896ebd551ec7e964078e9
}

ExampleDialog::~ExampleDialog ()
{
    searchButton.Detach (*this);
<<<<<<< HEAD
    Detach (*this);
}

void ExampleDialog::PanelOpened (const DG::PanelOpenEvent&)
{
=======
    elementList.Detach (*this);
    Detach (*this);
}

void ExampleDialog::PanelOpened (const DG::PanelOpenEvent& /*ev*/)
{
    elementList.DeleteItem (DG::ListBox::AllItems);
    short listWidth = elementList.GetWidth ();

    elementList.SetTabFieldCount (2);
    
    // タブ1: チェックボックス代わりのテキスト列 (幅30ピクセル)
    elementList.SetTabFieldProperties (1, 0, 30, DG::ListBox::Center, DG::ListBox::EndTruncate, true);
    // タブ2: 要素名列
    elementList.SetTabFieldProperties (2, 30, (short)(listWidth - 30), DG::ListBox::Left, DG::ListBox::EndTruncate, true);

    const GS::Array<GS::UniString> typeNames = { "Walls", "Columns", "Beams", "Slabs", "Windows", "Roofs", "Objects" };
    
    for (short i = 1; i <= (short)typeNames.GetSize (); i++) {
        elementList.InsertItem (i);
        // 初期状態は空のチェックボックス "[   ]" をセット
        elementList.SetTabItemText (i, 1, "[   ]"); 
        elementList.SetTabItemText (i, 2, typeNames[i-1]);
    }
}

void ExampleDialog::ListBoxSelectionChanged (const DG::ListBoxSelectionEvent& ev)
{
    if (ev.GetSource () == &elementList) {
        // クリックされた行を取得
        short listItem = elementList.GetSelectedItem (); 
        
        if (listItem > 0) {
            // 現在の1列目のテキストを取得し、反転させる
            GS::UniString currentMark = elementList.GetTabItemText (listItem, 1);
            if (currentMark == "[ x ]") {
                elementList.SetTabItemText (listItem, 1, "[   ]");
            } else {
                elementList.SetTabItemText (listItem, 1, "[ x ]");
            }
            
            // 選択の青いハイライトを解除し、擬似的にチェックボックスの操作感を出す
            elementList.DeselectItem (listItem);
        }
    }
>>>>>>> f6aad847eb0dc3cdce4896ebd551ec7e964078e9
}

void ExampleDialog::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource () == &searchButton) {
<<<<<<< HEAD
        RefreshElementList ();
    }
}

void ExampleDialog::RefreshElementList ()
{
    resultTree.DeleteItem (DG::TreeView::RootItem);

    struct Filter {
        API_ElemTypeID id;
        DG::CheckBox& cb;
        const char* name;
    };

    Filter filters[] = {
        { API_WallID, wallCheck, "Walls" },
        { API_ColumnID, columnCheck, "Columns" },
        { API_BeamID, beamCheck, "Beams" },
        { API_SlabID, slabCheck, "Slabs" },
        { API_WindowID, winDoorCheck, "Windows" },
        { API_DoorID, winDoorCheck, "Doors" },
        { API_ObjectID, objectCheck, "Objects" }
    };

    for (auto& f : filters) {

        if (!f.cb.IsChecked ())
            continue;

        GS::Array<API_Guid> elemGuids;

        if (ACAPI_Element_GetElemList (f.id, &elemGuids) != NoError)
            continue;

        if (elemGuids.IsEmpty ())
            continue;

        Int32 root = resultTree.InsertItem (DG::TreeView::RootItem, DG::TreeView::BottomItem);

        resultTree.SetItemText (
            root,
            GS::UniString::Printf ("%s (%d)", f.name, (int)elemGuids.GetSize ())
        );

        for (const auto& g : elemGuids) {

            Int32 child = resultTree.InsertItem (root, DG::TreeView::BottomItem);

            resultTree.SetItemText (
                child,
                APIGuid2GSGuid (g).ToUniString ().GetSubstring (0, 8)
            );
        }

        resultTree.ExpandItem (root);
=======
        selectedTypes.Clear ();

        for (short i = 1; i <= elementList.GetItemCount (); i++) {
            // 1列目が "[ x ]" になっているか判定
            if (elementList.GetTabItemText (i, 1) == "[ x ]") {
                GS::UniString typeName = elementList.GetTabItemText (i, 2);
                
                if (typeName == "Walls")   selectedTypes.Push (API_ElemType (API_WallID));
                if (typeName == "Columns") selectedTypes.Push (API_ElemType (API_ColumnID));
                if (typeName == "Beams")   selectedTypes.Push (API_ElemType (API_BeamID));
                if (typeName == "Slabs")   selectedTypes.Push (API_ElemType (API_SlabID));
                if (typeName == "Windows") selectedTypes.Push (API_ElemType (API_WindowID));
                if (typeName == "Roofs")   selectedTypes.Push (API_ElemType (API_RoofID));
                if (typeName == "Objects") selectedTypes.Push (API_ElemType (API_ObjectID));
            }
        }

        PostCloseRequest (DG::ModalDialog::Accept);
>>>>>>> f6aad847eb0dc3cdce4896ebd551ec7e964078e9
    }
}