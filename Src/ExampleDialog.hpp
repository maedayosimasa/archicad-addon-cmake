<<<<<<< HEAD
﻿#ifndef EXAMPLE_DIALOG_HPP
#define EXAMPLE_DIALOG_HPP

#include "ACAPinc.h"

=======
﻿#pragma once

#include "DG.h"
>>>>>>> f6aad847eb0dc3cdce4896ebd551ec7e964078e9
#include "DGDialog.hpp"
#include "DGPanel.hpp"
#include "DGItem.hpp"
#include "DGButton.hpp"
<<<<<<< HEAD
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
=======
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
>>>>>>> f6aad847eb0dc3cdce4896ebd551ec7e964078e9
