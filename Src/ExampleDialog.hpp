#ifndef EXAMPLE_DIALOG_HPP
#define EXAMPLE_DIALOG_HPP

#include "ACAPinc.h"
#include "DGDialog.hpp"
#include "DGButton.hpp"
#include "DGCheckItem.hpp"
#include "DGTreeView.hpp"

class ExampleDialog : public DG::ModalDialog,
                      public DG::PanelObserver,
                      public DG::ButtonItemObserver,
                      public DG::ListBoxObserver,    // ← 【重要】必ず public で継承する
                      public DG::CheckItemObserver
{
private:
    // 【重要】ポインタ（*）ではなく、すべて実体として宣言します。
    // これにより、Dialogが作られた瞬間にメモリが確保され、GRCと強固に結びつきます。
    DG::MultiSelListBox    resultList;

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
    GS::Array<API_Guid> GetSelectedElements ();
    GS::Array<API_Guid> itemGuids; // 検索結果のGUIDを一時保存する配列
};

#endif