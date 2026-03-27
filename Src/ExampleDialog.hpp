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

struct Filter {
    API_ElemTypeID id;
    DG::CheckBox* cb;
    const char* name;
};

class ExampleDialog : public DG::ModalDialog,
                      public DG::PanelObserver,
                      public DG::ButtonItemObserver
{
private:
    DG::SingleSelTreeView resultTree;

    DG::Button      searchButton;
    DG::CheckBox    wallCheck;
    DG::CheckBox    columnCheck;
    DG::CheckBox    beamCheck;
    DG::CheckBox    slabCheck;
    DG::CheckBox    winDoorCheck;
    DG::CheckBox    objectCheck;

    // ★ポインタに変更（重要）
    DG::CheckBox* storyChecks[4];

public:
    ExampleDialog ();
    virtual ~ExampleDialog ();

    void PanelOpened (const DG::PanelOpenEvent& ev) override;
    void ButtonClicked (const DG::ButtonClickEvent& ev) override;

private:
    void RefreshElementList ();
    GS::Array<Filter> GetSelectedFilters ();
    GS::Array<short> GetSelectedStories ();
    void BuildTree (const char* name, const GS::Array<API_Guid>& guids);
};

#endif