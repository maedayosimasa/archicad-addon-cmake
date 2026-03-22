#include "APIEnvir.h"
#include "ACAPinc.h"
<<<<<<< HEAD
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
=======
#include "RS.hpp"
#include "ExampleDialog.hpp"

// -----------------------------------------------------------------------------
// メニューコマンドハンドラ
// -----------------------------------------------------------------------------
static GSErrCode MenuCommandHandler (const API_MenuParams* /*menuParams*/)
{
    // 1. ダイアログのインスタンス化
    ExampleDialog dialog;

    // 2. ダイアログを表示し、ユーザーが「Search」ボタン（Accept）を押して閉じるのを待つ
    // これにより、重いAPI処理中にダイアログが画面をブロックする（フリーズする）のを防ぎます
    if (dialog.Invoke () == DG::ModalDialog::Accept) {
        
        GS::Array<API_Guid> finalFilteredGuids;
        API_StoryInfo storyInfo;
        BNZeroMemory (&storyInfo, sizeof (API_StoryInfo));
        
        // 現在表示中のフロア情報を取得（フィルタリング用）
        GSErrCode err = ACAPI_ProjectSetting_GetStorySettings (&storyInfo);
        if (err != NoError) return err;

        // 3. ダイアログの selectedTypes 配列（チェックされた要素タイプ）をスキャン
        for (const API_ElemType& type : dialog.selectedTypes) {
            GS::Array<API_Guid> guids;
            
            // 指定されたタイプの要素GUIDリストを全取得
            if (ACAPI_Element_GetElemList (type, &guids) == NoError) {
                for (const auto& guid : guids) {
                    API_Elem_Head header;
                    BNZeroMemory (&header, sizeof (API_Elem_Head));
                    header.guid = guid;
                    
                    // 要素のヘッダー情報を取得し、現在のフロアに存在するかチェック
                    if (ACAPI_Element_GetHeader (&header) == NoError) {
                        if (header.floorInd == storyInfo.actStory) {
                            finalFilteredGuids.Push (guid);
                        }
                    }
                }
            }
        }

        // 4. --- 選択の実行 ---
        // Archicad 28では、モーダルダイアログが閉じた後のこのタイミングで実行するのが最も安全です。
        
        // まず現在の選択を解除
        ACAPI_Selection_DeselectAll ();
        
        if (!finalFilteredGuids.IsEmpty ()) {
            GS::Array<API_Neig> selNeigs;
            for (const auto& guid : finalFilteredGuids) {
                selNeigs.Push (API_Neig (guid));
            }
            
            // フィルタリングされた要素を選択状態にする
            err = ACAPI_Selection_Select (selNeigs, true);
            
            if (err == NoError) {
                ACAPI_WriteReport (GS::UniString::Printf ("Success: Selected %u elements on the current story.", (unsigned int)finalFilteredGuids.GetSize()), true);
            }
        } else {
            ACAPI_WriteReport ("No matching elements found on the current story.", true);
        }
    }

    return NoError;
}

// -----------------------------------------------------------------------------
// Add-on Entry Points (Archicad 28 API Standard)
// -----------------------------------------------------------------------------

/**
 * アドオンの名前と説明をArchicadに登録
 */
API_AddonType CheckEnvironment (API_EnvirParams* envir)
{
    RSGetIndString (&envir->addOnInfo.name, 32000, 1, ACAPI_GetOwnResModule ());
    RSGetIndString (&envir->addOnInfo.description, 32000, 2, ACAPI_GetOwnResModule ());
    
    return APIAddon_Normal;
}

/**
 * メニューなどのインターフェースを登録
 */
GSErrCode RegisterInterface (void)
{
    // リソースID 32500 のメニューを「ツール」メニュー配下に登録
    return ACAPI_MenuItem_RegisterMenu (32500, 0, MenuCode_Tools, MenuFlag_Default);
}

/**
 * アドオン起動時の初期化処理
 */
GSErrCode Initialize (void)
{
    // メニューがクリックされた時の実行関数（コールバック）を紐付け
    return ACAPI_MenuItem_InstallMenuHandler (32500, MenuCommandHandler);
}

/**
 * アドオン終了時のクリーンアップ
 */
GSErrCode FreeData (void)
{
    return NoError;
>>>>>>> f6aad847eb0dc3cdce4896ebd551ec7e964078e9
}