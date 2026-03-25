#include "APIEnvir.h"
#include "ACAPinc.h"
#include "ResourceIds.hpp"
#include "ExampleDialog.hpp"

static GSErrCode MenuCommandHandler (const API_MenuParams* menuParams)
{
    if (menuParams->menuItemRef.menuResID == ID_MENU_STRINGS) {
        ExampleDialog dialog; // 名前を統一
        dialog.Invoke ();
    }
    return NoError;
}

extern "C" {
    API_AddonType __stdcall CheckEnvironment (API_EnvirParams* envir) {
        RSGetIndString (&envir->addOnInfo.name, 32500, 1, ACAPI_GetOwnResModule ());
        RSGetIndString (&envir->addOnInfo.description, 32500, 2, ACAPI_GetOwnResModule ());
        return APIAddon_Normal;
    }

    GSErrCode __stdcall RegisterInterface (void) {
        return ACAPI_MenuItem_RegisterMenu (ID_MENU_STRINGS, 0, MenuCode_Tools, MenuFlag_Default);
    }

    GSErrCode __stdcall Initialize (void) {
        return ACAPI_MenuItem_InstallMenuHandler (ID_MENU_STRINGS, MenuCommandHandler);
    }

    GSErrCode __stdcall FreeData (void) {
        return NoError;
    }
}