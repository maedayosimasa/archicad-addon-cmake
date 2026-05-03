#pragma warning(disable : 4819)
#include "ExamplePrecompiledHeader.hpp"
#include "ACAPinc.h"
#include "ExampleDialog.hpp"
#include "ResourceIds.hpp"

extern void StartPythonServer ();
extern void EnqueueProjectConfiguration ();

const GS::Guid& ExampleDialog::PaletteGuid ()
{
    static GS::Guid guid ("A832A582-EBF4-4DD9-927D-6129AE29E4B0");
    return guid;
}

Int32 ExampleDialog::PaletteRefId ()
{
    static Int32 refId (GS::CalculateHashValue (PaletteGuid ()));
    return refId;
}

GSErrCode ExampleDialog::PaletteAPIControlCallBack (Int32 referenceID, API_PaletteMessageID messageID, GS::IntPtr param)
{
    if (referenceID != PaletteRefId ()) {
        return NoError;
    }

    ExampleDialog& palette = GetInstance ();

    switch (messageID) {
        case APIPalMsg_OpenPalette:
        case APIPalMsg_HidePalette_End:
            if (!palette.IsVisible ()) {
                palette.Show ();
            }
            break;

        case APIPalMsg_ClosePalette:
        case APIPalMsg_HidePalette_Begin:
            if (palette.IsVisible ()) {
                palette.Hide ();
            }
            break;

        case APIPalMsg_IsPaletteVisible:
            (*reinterpret_cast<bool*> (param)) = palette.IsVisible ();
            break;

        default:
            break;
    }

    return NoError;
}

ExampleDialog& ExampleDialog::GetInstance ()
{
    static ExampleDialog instance;
    return instance;
}

ExampleDialog::ExampleDialog () :
    DG::Palette (ACAPI_GetOwnResModule (), ID_SEARCH_DIALOG, ACAPI_GetOwnResModule (), PaletteGuid ()),
    searchButton (GetReference (), SearchButtonID),
    cancelButton (GetReference (), CancelButtonID),
    getConfigButton (GetReference (), GetConfigButtonID)
{
    Attach (*this);
    searchButton.Attach (*this);
    cancelButton.Attach (*this);
    getConfigButton.Attach (*this);
    BeginEventProcessing ();
}

ExampleDialog::~ExampleDialog ()
{
    EndEventProcessing ();
    Detach (*this);
    searchButton.Detach (*this);
    cancelButton.Detach (*this);
    getConfigButton.Detach (*this);
}

void ExampleDialog::PanelOpened (const DG::PanelOpenEvent&)
{
    GSResModule res = ACAPI_GetOwnResModule ();
    this->SetTitle (RSGetIndString (ID_ADDON_DLG, 1, res));
    searchButton.SetText (RSGetIndString (ID_ADDON_DLG, 2, res));
    cancelButton.SetText (RSGetIndString (ID_ADDON_DLG, 3, res));
    getConfigButton.SetText (RSGetIndString (ID_ADDON_DLG, 4, res));
}

void ExampleDialog::PanelCloseRequested (const DG::PanelCloseRequestEvent&, bool* accepted)
{
    Hide ();
    if (accepted != nullptr) {
        *accepted = false;
    }
}

void ExampleDialog::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource () == &searchButton) {
        // [INFO]: 検索処理はSocket経由で自動化されました
    } else if (ev.GetSource () == &cancelButton) {
        Hide ();
    } else if (ev.GetSource () == &getConfigButton) {
        ACAPI_WriteReport("Configuration request triggered.", true);
        StartPythonServer ();
        EnqueueProjectConfiguration ();
        Hide ();
    }
}

