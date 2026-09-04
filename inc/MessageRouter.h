#ifndef _MESSAGE_ROUTER_H_
#define _MESSAGE_ROUTER_H_

#include <FBase.h>
#include "MRIM/MrimMessages.h"

class MessageRouter : public IMessageListener {
public:
    MessageRouter(void);
    virtual ~MessageRouter(void);

    void SetActiveChat(IMessageListener* pListener, const Osp::Base::String& email);
    void ClearActiveChat(void);

    virtual void OnMessageReceived(const Osp::Base::String& sender, const Osp::Base::String& text, bool isNudge);
    virtual void OnMessageDeliveryStatus(unsigned long status);
    virtual void OnTypingReceived(const Osp::Base::String& sender);

private:
    static void SaveMessageToHistory(const Osp::Base::String& email, const Osp::Base::String& sender, const Osp::Base::String& text);
    void ShowNotification(const Osp::Base::String& sender, const Osp::Base::String& text, bool isNudge);
    void ClearBadge(void);

    IMessageListener* pActiveChatListener;
    Osp::Base::String activeChatEmail;
};

#endif
