#ifndef _CHAT_FORM_H_
#define _CHAT_FORM_H_

#include <FUi.h>
#include <FBase.h>
#include <FSystem.h>
#include "AggConnection.h"

class ChatForm : public Osp::Ui::Controls::Form,
                 public Osp::Ui::IActionEventListener,
                 public Osp::Ui::ITextEventListener,
                 public IMessageListener
{
public:
    ChatForm(void);
    virtual ~ChatForm(void);

    result Initialize(AggConnection* pConn, const Osp::Base::String& name, const Osp::Base::String& email);
    virtual result OnInitializing(void);
    virtual result OnTerminating(void);

    virtual void OnActionPerformed(const Osp::Ui::Control& source, int actionId);
    virtual void OnUserEventReceivedN(long requestId, Osp::Base::Collection::IList* pArgs);

    // Слухачі MRIM-повідомлень (тепер оновлюватимуть UI напряму)
    virtual void OnMessageReceived(const Osp::Base::String& sender, const Osp::Base::String& text, bool isNudge);
    virtual void OnMessageDeliveryStatus(unsigned long status);
    virtual void OnTypingReceived(const Osp::Base::String& sender);

    virtual void OnTextValueChanged(const Osp::Ui::Control& source);
    virtual void OnTextValueChangeCanceled(const Osp::Ui::Control& source) {}

private:
    void AppendMessageToChat(const Osp::Base::String& senderTitle, const Osp::Base::String& text, bool isIncoming, bool saveToHistory = true);
        void SendNudgeNow(void);

    Osp::Base::String GetHistoryFilePath(void);
    void SaveMessageToHistory(const Osp::Base::String& sender, const Osp::Base::String& text);
    void LoadHistory(void);

    static const int ID_SOFTKEY_BACK       = 301;
    static const int ID_BTN_SEND           = 302;
    static const int ID_BTN_NUDGE          = 303;
    static const int ID_OPTIONKEY_CHAT     = 304;
    static const int ID_MENU_NUDGE         = 305;
    static const int ID_MENU_INFO          = 306;
    static const int ID_MENU_CLEAR_HISTORY = 307;

    AggConnection* pConnection;
    Osp::Base::String contactName;
    Osp::Base::String contactEmail;

    Osp::Ui::Controls::CustomList* pCustomListHistory;
    Osp::Ui::Controls::CustomListItemFormat* pItemFormat;
    Osp::Ui::Controls::EditField* pEditInput;
    Osp::Ui::Controls::Button* pBtnNudge;
};

#endif
