#ifndef EXAMPLE_DIALOG_HPP
#define EXAMPLE_DIALOG_HPP

#include "ACAPinc.h"
#include "DGModule.hpp"
#include "ResourceIds.hpp"

class ExampleDialog : public DG::ModalDialog,
                      public DG::PanelObserver,
                      public DG::ButtonItemObserver
{
private:
    // ボタン
    DG::Button      searchButton;
    
    // ツール別チェックボックス
    DG::CheckBox    wallCheck;
    DG::CheckBox    columnCheck;
    DG::CheckBox    beamCheck;
    DG::CheckBox    slabCheck;
    DG::CheckBox    winDoorCheck;
    DG::CheckBox    objectCheck;

    // 階数別チェックボックス
    DG::CheckBox    storyCheck1;
    DG::CheckBox    storyCheck2;
    DG::CheckBox    storyCheck3;
    DG::CheckBox    storyCheck4;

    // 内部補助関数
    GS::Array<short> GetSelectedStories ();

public:
    ExampleDialog ();
    ~ExampleDialog ();

    virtual void PanelOpened (const DG::PanelOpenEvent& ev) override;
    virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override;
};

#endif