#include "ExampleDialog.hpp"
#include "ACAPinc.h"
#include "ResourceIds.hpp"

ExampleDialog::ExampleDialog () :
    // リソースモジュールを明示的に指定
    DG::ModalDialog (ACAPI_GetOwnResModule (), ID_ADDON_DLG, ACAPI_GetOwnResModule ()),
    searchButton (GetReference (), SearchButtonId),
    messageLabel (GetReference (), LabelId),
    wallCheckBox (GetReference (), WallCheckId),
    columnCheckBox (GetReference (), ColumnCheckId),
    beamCheckBox (GetReference (), BeamCheckId),
    slabCheckBox (GetReference (), SlabCheckId),
    roofCheckBox (GetReference (), RoofCheckId),
    objectCheckBox (GetReference (), ObjectCheckId),
    windowCheckBox (GetReference (), WindowCheckId),
    doorCheckBox (GetReference (), DoorCheckId)
{
    // 検索ボタンのみオブザーバーを接続
    searchButton.Attach (*this);
    
    // デフォルトのチェック状態を設定
    wallCheckBox.Check ();
    slabCheckBox.Check ();
}

ExampleDialog::~ExampleDialog ()
{
    // 確実にデタッチ
    searchButton.Detach (*this);
}

// カウント処理の修正
USize ExampleDialog::CountElements (const API_ElemTypeID& typeID)
{
    GS::Array<API_Guid> guids;
    // Archicad 26以降、API_ElemTypeID をそのまま渡す形式でOK（API_ElemType 構造体を使用する場合もあり）
    GSErrCode err = ACAPI_Element_GetElemList (typeID, &guids);
    if (err == NoError) {
        return guids.GetSize ();
    }
    return 0;
}

void ExampleDialog::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource () == &searchButton) {
        GS::UniString resultText = "--- Search Results ---\n";
        USize totalCount = 0;

        // ラムダ関数の引数型を API_ElemTypeID に最適化
        auto AddToResult = [&](const char* name, bool isChecked, API_ElemTypeID type) {
            if (isChecked) {
                USize count = CountElements (type);
                // %u ではなく GS::UniString::Printf の型安全な指定を使用
                resultText += GS::UniString::Printf ("%s: %u\n", name, (unsigned int)count);
                totalCount += count;
            }
        };

        // 各要素の集計
        AddToResult ("Walls",   wallCheckBox.IsChecked (), API_WallID);
        AddToResult ("Columns", columnCheckBox.IsChecked (), API_ColumnID);
        AddToResult ("Beams",   beamCheckBox.IsChecked (), API_BeamID);
        AddToResult ("Slabs",   slabCheckBox.IsChecked (), API_SlabID);
        AddToResult ("Roofs",   roofCheckBox.IsChecked (), API_RoofID);
        AddToResult ("Objects", objectCheckBox.IsChecked (), API_ObjectID);
        AddToResult ("Windows", windowCheckBox.IsChecked (), API_WindowID);
        AddToResult ("Doors",   doorCheckBox.IsChecked (), API_DoorID);

        resultText += "----------------------\n";
        resultText += GS::UniString::Printf ("Total: %u items", (unsigned int)totalCount);
        
        // 結果を表示
        messageLabel.SetText (resultText);
    }
}