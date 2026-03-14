#ifndef EXAMPLE_DIALOG_HPP
#define EXAMPLE_DIALOG_HPP

#include "DGModule.hpp"
#include "DGTreeView.hpp"
#include "DGListBox.hpp"
#include "ResourceIds.hpp"

// 設計図：ここには「何があるか」だけを書く
class ExampleDialog : public DG::ModalDialog,
                      public DG::PanelObserver,
                      public DG::ButtonItemObserver
{
private:
    DG::Button              searchButton;
    DG::SingleSelTreeView   treeView;     // [5]
    DG::SingleSelListBox    listBox;      // [6]

public:
    ExampleDialog ();
    virtual ~ExampleDialog ();

    virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override;
};

#endif