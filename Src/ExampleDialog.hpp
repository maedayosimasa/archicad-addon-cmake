#ifndef EXAMPLEDIALOG_HPP
#define EXAMPLEDIALOG_HPP

#include "DGModule.hpp"
#include "ACAPinc.h" // API_ElemTypeID のために必要

class ExampleDialog : public DG::ModalDialog,
                      public DG::PanelObserver,
                      public DG::ButtonItemObserver,
                      public DG::CompoundItemObserver
{
private:
    enum {
        DialogId = 32502,
        SearchButtonId = 1,
        LabelId = 2,
        WallCheckId = 3,
        ColumnCheckId = 4,
        BeamCheckId = 5,
        SlabCheckId = 6,
        RoofCheckId = 7,
        ObjectCheckId = 8,
        WindowCheckId = 9,
        DoorCheckId = 10
    };

    DG::Button      searchButton;
    DG::LeftText    messageLabel;
    DG::CheckBox    wallCheckBox;
    DG::CheckBox    columnCheckBox;
    DG::CheckBox    beamCheckBox;
    DG::CheckBox    slabCheckBox;
    DG::CheckBox    roofCheckBox;
    DG::CheckBox    objectCheckBox;
    DG::CheckBox    windowCheckBox;
    DG::CheckBox    doorCheckBox;

    // ここを修正：引数に const と & を付ける
  USize CountElements (const API_ElemTypeID& typeID);

public:
    ExampleDialog ();
    virtual ~ExampleDialog ();

    virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override;
};

#endif