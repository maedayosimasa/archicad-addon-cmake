#include "ExampleDialog.hpp"

ExampleDialog::ExampleDialog () :
    DG::ModalDialog (ACAPI_GetOwnResModule (), 32500, ACAPI_GetOwnResModule ()),
    searchButton (GetReference (), 1),
    messageLabel (GetReference (), 2),
    elementList  (GetReference (), 3)
{
    searchButton.Attach (*this);
    Attach (*this);
}

ExampleDialog::~ExampleDialog ()
{
    searchButton.Detach (*this);
    Detach (*this);
}

void ExampleDialog::PanelOpened (const DG::PanelOpenEvent& /*ev*/)
{
    elementList.DeleteItem (DG::ListBox::AllItems);

    short listWidth = elementList.GetWidth ();
    elementList.SetTabFieldCount (1);
    elementList.SetTabFieldBeginEndPosition (1, 0, listWidth);

    // --- 項目の挿入（窓と屋根を追加） ---
    elementList.InsertItem (1);
    elementList.SetTabItemText (1, 1, "Walls");

    elementList.InsertItem (2);
    elementList.SetTabItemText (2, 1, "Columns");

    elementList.InsertItem (3);
    elementList.SetTabItemText (3, 1, "Beams");

    elementList.InsertItem (4);
    elementList.SetTabItemText (4, 1, "Slabs");

    elementList.InsertItem (5);
    elementList.SetTabItemText (5, 1, "Windows"); // 追加

    elementList.InsertItem (6);
    elementList.SetTabItemText (6, 1, "Roofs");   // 追加

    elementList.InsertItem (7);
    elementList.SetTabItemText (7, 1, "Objects");

    elementList.SelectItem (1);
    messageLabel.SetText ("Select types and click Search.");
}

void ExampleDialog::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource () == &searchButton) {
        short selected = elementList.GetSelectedItem ();
        if (selected <= 0) return;

        GS::UniString typeName = elementList.GetTabItemText (selected, 1);
        API_ElemType type (API_ZombieElemID);

        // --- 要素タイプの判定ロジックを更新 ---
        if (typeName == "Walls")   type = API_ElemType (API_WallID);
        if (typeName == "Columns") type = API_ElemType (API_ColumnID);
        if (typeName == "Beams")   type = API_ElemType (API_BeamID);
        if (typeName == "Slabs")   type = API_ElemType (API_SlabID);
        if (typeName == "Windows") type = API_ElemType (API_WindowID); // 追加
        if (typeName == "Roofs")   type = API_ElemType (API_RoofID);   // 追加
        if (typeName == "Objects") type = API_ElemType (API_ObjectID);

        GS::Array<API_Guid> elementGuids;
        if (ACAPI_Element_GetElemList (type, &elementGuids) == NoError) {
            USize count = elementGuids.GetSize ();
            
            // 以前の画像 のように結果を表示
            messageLabel.SetText (GS::UniString::Printf ("Found %u %s", (unsigned int)count, typeName.ToCStr ().Get ()));

            ACAPI_Selection_DeselectAll ();
            if (count > 0) {
                GS::Array<API_Neig> selNeigs;
                for (const auto& guid : elementGuids) {
                    selNeigs.Push (API_Neig (guid));
                }
                ACAPI_Selection_Select (selNeigs, true);
            }
        }
    }
}