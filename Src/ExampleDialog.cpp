#include "ExamplePrecompiledHeader.hpp"
#include "ACAPinc.h"
#include "ExampleDialog.hpp"
#include "ResultDialog.hpp"
#include "ElementSearchService.hpp"
#include "StoryService.hpp"
#include "ResourceIds.hpp"

ExampleDialog::ExampleDialog () :
    DG::ModalDialog (ACAPI_GetOwnResModule (), ID_SEARCH_DIALOG, ACAPI_GetOwnResModule ()),
    searchButton (GetReference (), SearchButtonID),
    wallCheck    (GetReference (), WallCheckID),
    columnCheck  (GetReference (), ColumnCheckID),
    beamCheck    (GetReference (), BeamCheckID),
    slabCheck    (GetReference (), SlabCheckID),
    winDoorCheck (GetReference (), WinDoorCheckID),
    objectCheck  (GetReference (), ObjectCheckID),
    storyCheck1  (GetReference (), StoryB1CheckID),
    storyCheck2  (GetReference (), Story1CheckID),
    storyCheck3  (GetReference (), Story2CheckID),
    storyCheck4  (GetReference (), Story3CheckID)
{
    Attach (*this);
    searchButton.Attach (*this);
}

ExampleDialog::~ExampleDialog ()
{
    Detach (*this);
    searchButton.Detach (*this);
}

void ExampleDialog::PanelOpened (const DG::PanelOpenEvent&)
{
    // プログラムから日本語をセット (STR# 32501 から取得)
    GSResModule resModule = ACAPI_GetOwnResModule ();
    this->SetTitle (RSGetIndString (32501, 12, resModule));
    searchButton.SetText (RSGetIndString (32501, 1, resModule));
    wallCheck.SetText (RSGetIndString (32501, 2, resModule));
    columnCheck.SetText (RSGetIndString (32501, 3, resModule));
    beamCheck.SetText (RSGetIndString (32501, 4, resModule));
    slabCheck.SetText (RSGetIndString (32501, 5, resModule));
    winDoorCheck.SetText (RSGetIndString (32501, 6, resModule));
    objectCheck.SetText (RSGetIndString (32501, 7, resModule));
    storyCheck1.SetText (RSGetIndString (32501, 8, resModule));
    storyCheck2.SetText (RSGetIndString (32501, 9, resModule));
    storyCheck3.SetText (RSGetIndString (32501, 10, resModule));
    storyCheck4.SetText (RSGetIndString (32501, 11, resModule));
}

void ExampleDialog::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource () == &searchButton) {
        auto stories = GetSelectedStories ();
        GS::Array<ElementInfo> allFound;

        if (wallCheck.IsChecked ()) allFound.Append (ElementSearchService::SearchElements (API_WallID, "Wall", stories));
        if (columnCheck.IsChecked ()) allFound.Append (ElementSearchService::SearchElements (API_ColumnID, "Column", stories));
        if (beamCheck.IsChecked ()) allFound.Append (ElementSearchService::SearchElements (API_BeamID, "Beam", stories));
        if (slabCheck.IsChecked ()) allFound.Append (ElementSearchService::SearchElements (API_SlabID, "Slab", stories));
        if (winDoorCheck.IsChecked ()) {
            allFound.Append (ElementSearchService::SearchElements (API_WindowID, "Window", stories));
            allFound.Append (ElementSearchService::SearchElements (API_DoorID, "Door", stories));
        }
        if (objectCheck.IsChecked ()) allFound.Append (ElementSearchService::SearchElements (API_ObjectID, "Object", stories));

        if (allFound.IsEmpty ()) {
            DG::ErrorAlert ("検索結果", "要素が見つかりませんでした。", "OK");
            return;
        }

        ResultDialog resDialog (allFound);
        resDialog.Invoke (); 
    }
}

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
