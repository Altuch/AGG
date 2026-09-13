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

    void SetUnreadCountListener(IUnreadCountListener* pListener);
    int GetUnreadCount(const Osp::Base::String& email) const;

    void SetKnownContacts(Osp::Base::Collection::IList* pContactEmails);
    Osp::Base::Collection::IList* GetStrangerEmails(void) const;

    // Notification tap routing: tapping a bada notification only foregrounds
    // the app (it carries no payload), so the router remembers which contact
    // the last notification came from. UI consumes it and opens that chat.
    // Persisted to a file so it also survives an app kill between tap and
    // launch. Emails here are always normalized (lowercase).
    Osp::Base::String PeekPendingNotificationSender(void) const;
    Osp::Base::String ConsumePendingNotificationSender(void);
    bool IsActiveChatWith(const Osp::Base::String& email) const;
    Osp::Base::String GetActiveChatEmail(void) const { return activeChatEmail; }

    static void SetAppForeground(bool foreground);

    virtual void OnMessageReceived(const Osp::Base::String& sender, const Osp::Base::String& text, bool isNudge);
    virtual void OnMessageDeliveryStatus(unsigned long status);
    virtual void OnTypingReceived(const Osp::Base::String& sender);

private:
    class UnreadEntry : public Osp::Base::Object {
    public:
        UnreadEntry(void) : count(0) {}
        virtual ~UnreadEntry(void) {}

        Osp::Base::String email;
        int count;
    };

    void IncrementUnreadCount(const Osp::Base::String& email);
    void ResetUnreadCount(const Osp::Base::String& email);
    void NotifyUnreadChanged(void);

    bool IsKnownContact(const Osp::Base::String& email) const;
    void RememberStranger(const Osp::Base::String& email);
    void ForgetStrangersNowInContacts(void);
    void LoadStrangers(void);
    void SaveStrangers(void);

    void ShowNotification(const Osp::Base::String& sender, const Osp::Base::String& text, bool isNudge);
    void ClearBadge(void);

    void SetPendingNotificationSender(const Osp::Base::String& email);
    void ClearPendingNotificationSender(void);
    void LoadPendingNotificationSender(void);
    void SavePendingNotificationSender(void);

    IMessageListener* pActiveChatListener;
    Osp::Base::String activeChatEmail;
    Osp::Base::String pendingNotificationEmail;

    IUnreadCountListener* pUnreadCountListener;
    Osp::Base::Collection::ArrayList* pUnreadCounts;
    Osp::Base::Collection::ArrayList* pKnownContactEmails;
    Osp::Base::Collection::ArrayList* pStrangerEmails;

    static bool isAppInForeground;
};

#endif
