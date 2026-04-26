#ifndef EXAMPLE_DIALOG_HPP
#define EXAMPLE_DIALOG_HPP

#include "DGModule.hpp"

class ExampleDialog : public DG::ModalDialog,
                      public DG::PanelObserver,
                      public DG::ButtonItemObserver
{
public:
    ExampleDialog ();
    ~ExampleDialog ();

private:
    virtual void PanelOpened (const DG::PanelOpenEvent& ev) override;
    virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override;

    DG::Button searchButton;
    DG::Button cancelButton;
    DG::Button getConfigButton;
};

#endif
