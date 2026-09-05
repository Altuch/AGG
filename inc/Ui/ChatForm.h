#ifndef _CHAT_FORM_H_
#define _CHAT_FORM_H_

#include <FUi.h>
#include <FBase.h>
#include <FSystem.h>
#include "Core/AggConnection.h"

class ChatForm : public Osp::Ui::Controls::Form,
                 public Osp::Ui::IActionEventListener,
                 public Osp::Ui::ITextEventListener,
                 public IMessageListener
{
public:
    ChatForm(void);
    virtual ~ChatForm(void);

    result Initialize(AggConnection* pConn,
                      const Osp::Base::String& name,
                      const Osp::Base::String& email);

    virtual result OnInitializing(void);
    virtual result OnTerminating(void);

    virtual void OnActionPerformed(const Osp::Ui::Control& source, int actionId);

    virtual void OnMessageReceived(const Osp::Base::String& sender, const Osp::Base::String& text, bool isNudge);
    virtual void OnMessageDeliveryStatus(unsigned long status);
    virtual void OnTypingReceived(const Osp::Base::String& sender);

    virtual void OnTextValueChanged(const Osp::Ui::Control& source);
    virtual void OnTextValueChangeCanceled(const Osp::Ui::Control& source) {}

private:
    void AppendLine(const Osp::Base::String& sender,
                    const Osp::Base::String& text,
                    bool saveToHistory);
    void ShowSystemLine(const Osp::Base::String& text);
    void SendNudgeNow(void);
    void LoadHistory(void);
    void RedrawHistory(void);

    static const int ID_SOFTKEY_BACK       = 301;
    static const int ID_BTN_SEND           = 302;
    static const int ID_OPTIONKEY_CHAT     = 303;
    static const int ID_MENU_NUDGE         = 304;
    static const int ID_MENU_INFO          = 305;
    static const int ID_MENU_CLEAR_HISTORY = 306;

    static const int MAX_LOADED_HISTORY_LINES = 200;

    AggConnection* pConnection;
    Osp::Base::String contactName;
    Osp::Base::String contactEmail;

    Osp::Ui::Controls::EditArea* pHistoryArea;
    Osp::Ui::Controls::EditField* pInputField;
};

#endif
