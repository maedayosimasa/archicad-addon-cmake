#include "APIEnvir.h"
#include "ACAPinc.h"
#include "ResourceIds.hpp"
#include "ExampleDialog.hpp"

static GSErrCode MenuCommandHandler (const API_MenuParams* menuParams)
{
    if (menuParams->menuItemRef.menuResID == ID_MENU_STRINGS) { 
        ExampleDialog dialog;
        dialog.Invoke ();
    }
    return NoError;
}

// -----------------------------------------------------------------------------
// エントリポイント関数
// -----------------------------------------------------------------------------

extern "C" {

API_AddonType __stdcall CheckEnvironment (API_EnvirParams* envir)
{
    RSGetIndString (&envir->addOnInfo.name, ID_ADDON_INFO, 1, ACAPI_GetOwnResModule ());
    RSGetIndString (&envir->addOnInfo.description, ID_ADDON_INFO, 2, ACAPI_GetOwnResModule ());

    return APIAddon_Normal;
}

GSErrCode __stdcall RegisterInterface (void)
{
    GSErrCode err = ACAPI_MenuItem_RegisterMenu (ID_MENU_STRINGS, 0, MenuCode_Tools, MenuFlag_Default);
    return err;
}

GSErrCode __stdcall Initialize (void)
{
    GSErrCode err = ACAPI_MenuItem_InstallMenuHandler (ID_MENU_STRINGS, MenuCommandHandler);
    return err;
}

GSErrCode __stdcall FreeData (void)
{
    return NoError;
}

} // extern "C"
