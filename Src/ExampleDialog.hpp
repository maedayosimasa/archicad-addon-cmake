#ifndef EXAMPLEDIALOG_HPP
#define EXAMPLEDIALOG_HPP

#include "DG.h"
#include "DGDialog.hpp"
#include "DGPanel.hpp"
#include "DGItem.hpp"
#include "DGButton.hpp"
#include "DGStaticItem.hpp"
#include "DGListBox.hpp"
#include "ACAPinc.h"

class ExampleDialog : public DG::ModalDialog,
                      public DG::PanelObserver,
                      public DG::ButtonItemObserver
{
private:
    DG::Button              searchButton;
    DG::LeftText            messageLabel;
    DG::SingleSelListBox    elementList;

public:
    ExampleDialog ();
    virtual ~ExampleDialog ();

    virtual void PanelOpened (const DG::PanelOpenEvent& ev) override;
    // ButtonClickEvent に戻して整合性をとります
    virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override;
};

#endif