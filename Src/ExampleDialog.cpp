#include "ACAPinc.h"

#include "ExampleDialog.hpp"

#include "UniString.hpp"
#include "BM.hpp"
#include <algorithm>

#include "ElementService.hpp"
#include "StoryService.hpp"


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
    // ★ newで生成（これが正解）
    storyChecks[0] = new DG::CheckBox (GetReference (), Story1CheckID);
    storyChecks[1] = new DG::CheckBox (GetReference (), Story2CheckID);
    storyChecks[2] = new DG::CheckBox (GetReference (), Story3CheckID);
    storyChecks[3] = new DG::CheckBox (GetReference (), Story4CheckID);

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

    // ★delete忘れるとメモリリーク
    for (int i = 0; i < 4; i++) {
        delete storyChecks[i];
    }
}


void ExampleDialog::PanelOpened (const DG::PanelOpenEvent&)
{
    auto stories = StoryService::GetAllStories ();

    Int32 count = std::min ((Int32)stories.GetSize (), 4);

    for (Int32 i = 0; i < count; i++) {
        storyChecks[i]->SetText (stories[i].name);
        storyChecks[i]->Check ();
        storyChecks[i]->Enable ();
    }

    for (Int32 i = count; i < 4; i++) {
        storyChecks[i]->Disable ();
    }
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

    auto filters = GetSelectedFilters ();
    auto stories = GetSelectedStories ();

    for (const auto& f : filters) {

        auto guids = ElementService::GetElementsByTypeAndStories (
            f.id,
            stories
        );

        if (!guids.IsEmpty ()) {
            BuildTree (f.name, guids);
        }
    }
}


GS::Array<Filter> ExampleDialog::GetSelectedFilters ()
{
    GS::Array<Filter> result;

    Filter all[] = {
        { API_WallID, &wallCheck, "Walls" },
        { API_ColumnID, &columnCheck, "Columns" },
        { API_BeamID, &beamCheck, "Beams" },
        { API_SlabID, &slabCheck, "Slabs" },
        { API_WindowID, &winDoorCheck, "Windows" },
        { API_DoorID, &winDoorCheck, "Doors" },
        { API_ObjectID, &objectCheck, "Objects" }
    };

    for (auto& f : all) {
        if (f.cb->IsChecked ()) {
            result.Push (f);
        }
    }

    return result;
}


GS::Array<short> ExampleDialog::GetSelectedStories ()
{
    GS::Array<short> result;

    auto stories = StoryService::GetAllStories ();
    Int32 count = std::min ((Int32)stories.GetSize (), 4);

    for (Int32 i = 0; i < count; i++) {
        if (storyChecks[i]->IsChecked ()) {
            result.Push (stories[i].index);
        }
    }

    return result;
}


void ExampleDialog::BuildTree (const char* name, const GS::Array<API_Guid>& guids)
{
    Int32 root = resultTree.InsertItem (DG::TreeView::RootItem, DG::TreeView::BottomItem);

    resultTree.SetItemText (
        root,
        GS::UniString::Printf ("%s (%d)", name, (int)guids.GetSize ())
    );

    for (const auto& g : guids) {

        Int32 child = resultTree.InsertItem (root, DG::TreeView::BottomItem);

        resultTree.SetItemText (
            child,
            APIGuid2GSGuid (g).ToUniString ().GetSubstring (0, 8)
        );
    }

    resultTree.ExpandItem (root);
}