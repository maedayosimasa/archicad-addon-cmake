/* ResultDialog.hpp */
#ifndef RESULTDIALOG_HPP
#define RESULTDIALOG_HPP

#include "ACAPinc.h"
#include "DGModule.hpp"
#include "DGDialog.hpp"
#include "DGListBox.hpp"
#include "DGButton.hpp"
#include "ResourceIds.hpp"
#include "ElementInfo.hpp"

class ResultDialog : public DG::ModalDialog,
                     public DG::PanelObserver,
                     public DG::ButtonItemObserver,
                     public DG::ListBoxObserver
{
private:
    DG::MultiSelListBox    resultList;
    DG::Button             closeButton;
    GS::Array<ElementInfo> displayData;

public:
    ResultDialog (const GS::Array<ElementInfo>& data);
    virtual ~ResultDialog ();

    virtual void PanelOpened (const DG::PanelOpenEvent& ev) override;
    virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override;
};

#endif
