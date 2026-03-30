#ifndef RESULT_DIALOG_HPP
#define RESULT_DIALOG_HPP

// Archicad SDKの標準的なダイアログ用インクルード
#include "ACAPinc.h"
#include "DGModule.hpp"
#include "DGListBox.hpp"
#include "DGButton.hpp"
#include "ElementInfo.hpp"

// DG:: 名前空間を使用して定義
class ResultDialog : public DG::ModalDialog,
                     public DG::PanelObserver,
                     public DG::ListBoxObserver,
                    public DG::ButtonItemObserver // ButtonItemObserver に修正
{
private:
    enum {
        ResultListID = 1,  // GRCでのID
        CloseButtonID = 2
    };

    DG::MultiSelListBox resultList;
    DG::Button          closeButton;
    const GS::Array<ElementInfo>& displayData; // 検索ダイアログから渡されたデータ

public:
    ResultDialog (const GS::Array<ElementInfo>& data);
    ~ResultDialog ();

    virtual void PanelOpened (const DG::PanelOpenEvent& ev) override;
    // ★追加：ボタンクリックのイベントハンドラ
    virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override;
};

#endif