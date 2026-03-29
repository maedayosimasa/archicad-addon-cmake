#include "APIEnvir.h"
#include "ACAPinc.h"
#include "ResourceIds.hpp"
#include "ExampleDialog.hpp"

static GSErrCode MenuCommandHandler (const API_MenuParams* menuParams)
{
    // メニューIDの判定を ID_MENU_STRINGS (32500) と比較
    if (menuParams->menuItemRef.menuResID == 32500) { 
        ExampleDialog dialog;
        dialog.Invoke ();
    }
    return NoError;
}

extern "C" {
    API_AddonType __stdcall CheckEnvironment (API_EnvirParams* envir) {
        // 【修正】名前と説明は通常 STR# 32000 から取得します
        RSGetIndString (&envir->addOnInfo.name, 32000, 1, ACAPI_GetOwnResModule ());
        RSGetIndString (&envir->addOnInfo.description, 32000, 2, ACAPI_GetOwnResModule ());
        return APIAddon_Normal;
    }

    GSErrCode __stdcall RegisterInterface (void) {
        // メニューをツールメニューに登録
        return ACAPI_MenuItem_RegisterMenu (32500, 0, MenuCode_Tools, MenuFlag_Default);
    }

    GSErrCode __stdcall Initialize (void) {
        // ハンドラをインストール
        return ACAPI_MenuItem_InstallMenuHandler (32500, MenuCommandHandler);
    }

    GSErrCode __stdcall FreeData (void) {
        return NoError;
    }
}