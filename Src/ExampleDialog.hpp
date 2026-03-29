#ifndef EXAMPLE_DIALOG_HPP
#define EXAMPLE_DIALOG_HPP

#include "ACAPinc.h"
#include "DGDialog.hpp"
#include "DGButton.hpp"
#include "DGCheckItem.hpp"
#include "DGTreeView.hpp"
#include "ResourceIds.hpp"

class ExampleDialog : public DG::ModalDialog,
                      public DG::PanelObserver,
                      public DG::ButtonItemObserver
{
private:
    // 【重要】ポインタ（*）ではなく、すべて実体として宣言します。
    // これにより、Dialogが作られた瞬間にメモリが確保され、GRCと強固に結びつきます。
    DG::MultiSelTreeView resultTree;

    DG::Button      searchButton;
    DG::CheckBox    wallCheck;
    DG::CheckBox    columnCheck;
    DG::CheckBox    beamCheck;
    DG::CheckBox    slabCheck;
    DG::CheckBox    winDoorCheck;
    DG::CheckBox    objectCheck;

    // newを避けるため、配列ではなく個別の変数として宣言します
    DG::CheckBox    storyCheck1;
    DG::CheckBox    storyCheck2;
    DG::CheckBox    storyCheck3;
    DG::CheckBox    storyCheck4;

public:
    ExampleDialog ();
    virtual ~ExampleDialog ();

    virtual void PanelOpened (const DG::PanelOpenEvent& ev) override;
    virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override;

private:
    void RefreshElementList ();
    void SearchAndAdd (API_ElemTypeID typeID, const char* label, const GS::Array<short>& stories, Int32& totalCount);
    GS::Array<short> GetSelectedStories ();
};

#endif