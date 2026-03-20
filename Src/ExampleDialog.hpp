#pragma once

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
                      public DG::ButtonItemObserver,
                      public DG::ListBoxObserver
{
private:
    DG::Button             searchButton;
    DG::LeftText           messageLabel;
    
    // 【修正】保護されたベースクラスではなく SingleSelListBox を使用します
    DG::SingleSelListBox   elementList;  

public:
    GS::Array<API_ElemType> selectedTypes;

    ExampleDialog ();
    virtual ~ExampleDialog ();

    virtual void PanelOpened (const DG::PanelOpenEvent& ev) override;
    virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override;
    virtual void ListBoxSelectionChanged (const DG::ListBoxSelectionEvent& ev) override;
};