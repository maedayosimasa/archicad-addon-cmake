#include "APIEnvir.h"
#include "ACAPinc.h"
#include "ResourceIds.hpp"
#include "ExampleDialog.hpp"

#ifndef AD_CALL
#define AD_CALL __stdcall
#endif

// メニューコマンド実行時の処理
static GSErrCode AD_CALL MenuCommandHandler (const API_MenuParams* menuParams)
{
    if (menuParams->menuItemRef.itemIndex == 1) {
        ExampleDialog dialog;
        dialog.Invoke ();
    }
    return NoError;
}

extern "C" {

    // アドオンの読み込み準備
    API_AddonType AD_CALL CheckEnvironment (API_EnvirParams* envir)
    {
        // 余計な関数は不要です。UniStringへの読み込み（&付き）のみ行います。
        RSGetIndString (&envir->addOnInfo.name, ID_ADDON_INFO, 1, ACAPI_GetOwnResModule ());
        RSGetIndString (&envir->addOnInfo.description, ID_ADDON_INFO, 2, ACAPI_GetOwnResModule ());

        return APIAddon_Normal;
    }

    // メニューの登録
    GSErrCode AD_CALL RegisterInterface (void)
    {
        // AC28公式テンプレートに則り、「MenuCode_Tools」を使用します
        return ACAPI_MenuItem_RegisterMenu (ID_ADDON_MENU, 0, MenuCode_Tools, MenuFlag_Default);
    }

    // 初期化（ハンドラーの設置）
    GSErrCode AD_CALL Initialize (void)
    {
        return ACAPI_MenuItem_InstallMenuHandler (ID_ADDON_MENU, MenuCommandHandler);
    }

    // 終了処理
    GSErrCode AD_CALL FreeData (void)
    {
        return NoError;
    }

} // extern "C"