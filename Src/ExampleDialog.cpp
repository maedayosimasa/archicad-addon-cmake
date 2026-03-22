#include "ACAPinc.h"

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
}

ExampleDialog::~ExampleDialog ()
{
    searchButton.Detach (*this);
    Detach (*this);
}

void ExampleDialog::PanelOpened (const DG::PanelOpenEvent&)
{
}

void ExampleDialog::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource () == &searchButton) {
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
    }
}