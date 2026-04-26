#pragma warning(disable : 4819)
#include "ExamplePrecompiledHeader.hpp"
#include "ACAPinc.h"
#include "ExampleDialog.hpp"
#include "ResourceIds.hpp"

// External function from AddOnMain.cpp
extern void ExportElementsToPython (const GS::Array<API_Guid>& elemGuids);
extern void ExportConfigurationToPython ();

ExampleDialog::ExampleDialog () :
    DG::ModalDialog (ACAPI_GetOwnResModule (), ID_SEARCH_DIALOG, ACAPI_GetOwnResModule ()),
    searchButton (GetReference (), SearchButtonID),
    cancelButton (GetReference (), CancelButtonID),
    getConfigButton (GetReference (), GetConfigButtonID)
{
    Attach (*this);
    searchButton.Attach (*this);
    cancelButton.Attach (*this);
    getConfigButton.Attach (*this);
}

ExampleDialog::~ExampleDialog ()
{
    Detach (*this);
    searchButton.Detach (*this);
    cancelButton.Detach (*this);
    getConfigButton.Detach (*this);
}

void ExampleDialog::PanelOpened (const DG::PanelOpenEvent&)
{
    GSResModule resModule = ACAPI_GetOwnResModule ();
    this->SetTitle (RSGetIndString (ID_ADDON_DLG, 1, resModule));
    searchButton.SetText (RSGetIndString (ID_ADDON_DLG, 2, resModule));
    cancelButton.SetText (RSGetIndString (ID_ADDON_DLG, 3, resModule));
    getConfigButton.SetText (RSGetIndString (ID_ADDON_DLG, 4, resModule));
}

void ExampleDialog::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource () == &searchButton) {
        ACAPI_WriteReport("Searching elements...", false);
        // Implement simple search if needed
    } else if (ev.GetSource () == &cancelButton) {
        PostCloseRequest (DG::ModalDialog::Cancel);
    } else if (ev.GetSource () == &getConfigButton) {
        ACAPI_WriteReport("Getting configuration and sending to PyQt...", false);
        ExportConfigurationToPython ();
    }
}
