#include "APIEnvir.h"
#include "ACAPinc.h"
#include "RS.hpp"
#include "ExampleDialog.hpp"

// -----------------------------------------------------------------------------
// メニューコマンドハンドラ
// -----------------------------------------------------------------------------
static GSErrCode MenuCommandHandler (const API_MenuParams* /*menuParams*/)
{
// この行を削除（またはコメントアウト）すれば「警告！」は出なくなります
    // ACAPI_WriteReport ("Menu clicked: Opening Dialog...", true);

    ExampleDialog dialog;
    dialog.Invoke ();

    return NoError;
}

// -----------------------------------------------------------------------------
// Archicad エクスポート関数（extern "C" や __ACENV_CALL は一切不要です）
// -----------------------------------------------------------------------------

API_AddonType CheckEnvironment (API_EnvirParams* envir)
{
    // 正しい関数 ACAPI_GetOwnResModule() を使用
    RSGetIndString (&envir->addOnInfo.name, 32000, 1, ACAPI_GetOwnResModule ());
    RSGetIndString (&envir->addOnInfo.description, 32000, 2, ACAPI_GetOwnResModule ());
    
    return APIAddon_Normal;
}

GSErrCode RegisterInterface (void)
{
    // 正しい定数 MenuCode_Tools を使用
    return ACAPI_MenuItem_RegisterMenu (32500, 0, MenuCode_Tools, MenuFlag_Default);
}

GSErrCode Initialize (void)
{
    return ACAPI_MenuItem_InstallMenuHandler (32500, MenuCommandHandler);
}

GSErrCode FreeData (void)
{
    return NoError;
}