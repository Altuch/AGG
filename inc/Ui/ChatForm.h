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
    static const int ROW_INCOMING = 0;
    static const int ROW_OWN      = 1;
    static const int ROW_SYSTEM   = 2;

    void AppendLine(const Osp::Base::String& sender,
                    const Osp::Base::String& text,
                    bool saveToHistory,
                    int rowKind);
    void AppendRow(const Osp::Base::String& nick,
                   const Osp::Base::String& text,
                   int rowKind);
    void AppendRowChunk(const Osp::Base::String& nick,
                        const Osp::Base::String& chunk,
                        const Osp::Graphics::Color& nickColor,
                        int charsPerLine);
    int CalibratedCharsPerLine(int textW) const;
    void ShowSystemLine(const Osp::Base::String& text);
    void SendNudgeNow(void);
    void LoadHistory(void);
    void ClearHistoryView(void);

    static const int ID_SOFTKEY_BACK       = 301;
    static const int ID_BTN_SEND           = 302;
    static const int ID_OPTIONKEY_CHAT     = 303;
    static const int ID_MENU_NUDGE         = 304;
    static const int ID_MENU_INFO          = 305;
    static const int ID_MENU_CLEAR_HISTORY = 306;

    static const int MAX_LOADED_HISTORY_LINES = 200;

    static const int ELEM_NICK = 1;
    static const int ELEM_BODY = 2;

    AggConnection* pConnection;
    Osp::Base::String contactName;
    Osp::Base::String contactEmail;

    Osp::Ui::Controls::EditField* pInputField;
    Osp::Ui::Controls::CustomList* pHistoryList;
    Osp::Base::Collection::ArrayList* pRowFormats;
    int rowSeq_;
    Osp::Graphics::Font* pMeasureFont_;
};

#endif
