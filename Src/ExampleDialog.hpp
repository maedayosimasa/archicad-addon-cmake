#ifndef EXAMPLE_DIALOG_HPP
#define EXAMPLE_DIALOG_HPP

#include "ACAPinc.h"

#include "DGDialog.hpp"
#include "DGPanel.hpp"
#include "DGItem.hpp"
#include "DGButton.hpp"
#include "DGCheckItem.hpp"
#include "DGTreeView.hpp"

#include "ResourceIds.hpp"

class ExampleDialog : public DG::ModalDialog,
                      public DG::PanelObserver,
                      public DG::ButtonItemObserver
{
private:
    // ✔ TreeView修正済
    DG::SingleSelTreeView resultTree;

    DG::Button      searchButton;
    DG::CheckBox    wallCheck;
    DG::CheckBox    columnCheck;
    DG::CheckBox    beamCheck;
    DG::CheckBox    slabCheck;
    DG::CheckBox    winDoorCheck;
    DG::CheckBox    objectCheck;

public:
    ExampleDialog ();
    virtual ~ExampleDialog ();

    void PanelOpened (const DG::PanelOpenEvent& ev) override;
    void ButtonClicked (const DG::ButtonClickEvent& ev) override;

private:
    void RefreshElementList ();
};

#endif