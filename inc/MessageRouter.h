#ifndef _MESSAGE_ROUTER_H_
#define _MESSAGE_ROUTER_H_

#include <FBase.h>
#include "MRIM/MrimMessages.h"

class IUnreadCountListener {
public:
    virtual ~IUnreadCountListener(void) {}
    virtual void OnUnreadCountChanged(void) = 0;
};

class MessageRouter : public IMessageListener {
public:
    MessageRouter(void);
    virtual ~MessageRouter(void);
    void SetActiveChat(IMessageListener* pListener, const Osp::Base::String& email);
    void ClearActiveChat(void);

    virtual void OnMessageReceived(const Osp::Base::String& sender, const Osp::Base::String& text, bool isNudge);
    virtual void OnMessageDeliveryStatus(unsigned long status);
    virtual void OnTypingReceived(const Osp::Base::String& sender);

    static void SetAppForeground(bool foreground);
    int GetUnreadCount(const Osp::Base::String& email) const;
    void SetUnreadCountListener(IUnreadCountListener* pListener);

    void SetKnownContacts(Osp::Base::Collection::IList* pContactEmails);
    Osp::Base::Collection::IList* GetStrangerEmails(void) const;

private:
    class UnreadEntry : public Osp::Base::Object {
    public:
        UnreadEntry(void) : count(0) {}
        Osp::Base::String email;
        int count;
    };

    void IncrementUnreadCount(const Osp::Base::String& email);
    void ResetUnreadCount(const Osp::Base::String& email);

    bool IsKnownContact(const Osp::Base::String& email) const;
    void RememberStranger(const Osp::Base::String& email);
    void PruneKnownStrangers(void);
    void LoadStrangers(void);
    void SaveStrangers(void);

    static void SaveMessageToHistory(const Osp::Base::String& email, const Osp::Base::String& sender, const Osp::Base::String& text);
    void ShowNotification(const Osp::Base::String& sender, const Osp::Base::String& text, bool isNudge);
    void ClearBadge(void);

    IMessageListener* pActiveChatListener;
    Osp::Base::String activeChatEmail;
    Osp::Base::Collection::ArrayList* pUnreadCounts;
    IUnreadCountListener* pUnreadCountListener;
    Osp::Base::Collection::ArrayList* pKnownContactEmails;
    Osp::Base::Collection::ArrayList* pStrangerEmails;

    static bool isAppInForeground;
};

#endif
