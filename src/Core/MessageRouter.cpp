#include "Core/MessageRouter.h"
#include "Core/ChatHistory.h"
#include "MRIM/MrimUtils.h"
#include <FIo.h>
#include <FApp.h>

using namespace Osp::App;
using namespace Osp::Base;
using namespace Osp::Base::Collection;
using namespace Osp::Io;

bool MessageRouter::isAppInForeground = true;

static const wchar_t* STRANGERS_FILE = L"/Home/strangers.dat";
static const wchar_t* PENDING_NOTI_FILE = L"/Home/pending_noti.dat";
static const int NOTIFICATION_MAX_LENGTH = 100;

MessageRouter::MessageRouter(void) :
    pActiveChatListener(null),
    pUnreadCountListener(null),
    pUnreadCounts(null),
    pKnownContactEmails(null),
    pStrangerEmails(null)
{
    pUnreadCounts = new ArrayList();
    pUnreadCounts->Construct();

    pKnownContactEmails = new ArrayList();
    pKnownContactEmails->Construct();

    pStrangerEmails = new ArrayList();
    pStrangerEmails->Construct();
    LoadStrangers();
    LoadPendingNotificationSender();
}

MessageRouter::~MessageRouter(void) {
    if (pUnreadCounts != null) {
        pUnreadCounts->RemoveAll(true);
        delete pUnreadCounts;
    }
    if (pKnownContactEmails != null) {
        pKnownContactEmails->RemoveAll(true);
        delete pKnownContactEmails;
    }
    if (pStrangerEmails != null) {
        pStrangerEmails->RemoveAll(true);
        delete pStrangerEmails;
    }
}

void MessageRouter::SetAppForeground(bool foreground) {
    isAppInForeground = foreground;
}

void MessageRouter::SetUnreadCountListener(IUnreadCountListener* pListener) {
    pUnreadCountListener = pListener;
}

void MessageRouter::NotifyUnreadChanged(void) {
    if (pUnreadCountListener != null) pUnreadCountListener->OnUnreadCountChanged();
}

void MessageRouter::SetActiveChat(IMessageListener* pListener, const String& email) {
    pActiveChatListener = pListener;

    activeChatEmail = MrimUtils::NormalizeEmail(email);

    ClearBadge();
    ResetUnreadCount(activeChatEmail);
    // Chat with this contact is now open — its notification is handled.
    if (!pendingNotificationEmail.IsEmpty()
        && pendingNotificationEmail.Equals(activeChatEmail, true)) {
        ClearPendingNotificationSender();
    }
}

void MessageRouter::ClearActiveChat(void) {
    pActiveChatListener = null;
    activeChatEmail = L"";
}

void MessageRouter::IncrementUnreadCount(const String& email) {
    if (pUnreadCounts == null) return;

    for (int i = 0; i < pUnreadCounts->GetCount(); i++) {
        UnreadEntry* pEntry = static_cast<UnreadEntry*>(pUnreadCounts->GetAt(i));
        if (pEntry != null && pEntry->email.Equals(email, true)) {
            pEntry->count++;
            NotifyUnreadChanged();
            return;
        }
    }

    UnreadEntry* pEntry = new UnreadEntry();
    pEntry->email = email;
    pEntry->count = 1;
    pUnreadCounts->Add(*pEntry);

    NotifyUnreadChanged();
}

void MessageRouter::ResetUnreadCount(const String& email) {
    if (pUnreadCounts == null) return;

    for (int i = 0; i < pUnreadCounts->GetCount(); i++) {
        UnreadEntry* pEntry = static_cast<UnreadEntry*>(pUnreadCounts->GetAt(i));
        if (pEntry != null && pEntry->email.Equals(email, true)) {
            pEntry->count = 0;
            break;
        }
    }

    NotifyUnreadChanged();
}

int MessageRouter::GetUnreadCount(const String& email) const {
    if (pUnreadCounts == null) return 0;

    String cleanEmail = email;
    cleanEmail.Trim();
    cleanEmail.ToLower();

    for (int i = 0; i < pUnreadCounts->GetCount(); i++) {
        UnreadEntry* pEntry = static_cast<UnreadEntry*>(pUnreadCounts->GetAt(i));
        if (pEntry != null && pEntry->email.Equals(cleanEmail, true)) return pEntry->count;
    }
    return 0;
}

void MessageRouter::SetKnownContacts(IList* pContactEmails) {
    if (pKnownContactEmails == null) return;

    pKnownContactEmails->RemoveAll(true);
    if (pContactEmails == null) return;

    for (int i = 0; i < pContactEmails->GetCount(); i++) {
        String* pEmail = static_cast<String*>(pContactEmails->GetAt(i));
        if (pEmail == null) continue;

        String clean = *pEmail;
        clean.Trim();
        clean.ToLower();
        pKnownContactEmails->Add(*(new String(clean)));
    }

    ForgetStrangersNowInContacts();
}

IList* MessageRouter::GetStrangerEmails(void) const {
    return pStrangerEmails;
}

bool MessageRouter::IsKnownContact(const String& email) const {
    if (pKnownContactEmails == null) return false;

    for (int i = 0; i < pKnownContactEmails->GetCount(); i++) {
        String* pKnown = static_cast<String*>(pKnownContactEmails->GetAt(i));
        if (pKnown != null && pKnown->Equals(email, true)) return true;
    }
    return false;
}

void MessageRouter::RememberStranger(const String& email) {
    if (pStrangerEmails == null) return;

    String clean = MrimUtils::NormalizeEmail(email);
    if (clean.IsEmpty()) return;

    for (int i = 0; i < pStrangerEmails->GetCount(); i++) {
        String* pExisting = static_cast<String*>(pStrangerEmails->GetAt(i));
        if (pExisting != null && pExisting->Equals(clean, true)) return; // уже знаємо
    }

    pStrangerEmails->Add(*(new String(clean)));
    SaveStrangers();
    NotifyUnreadChanged();
}

void MessageRouter::ForgetStrangersNowInContacts(void) {
    if (pStrangerEmails == null || pKnownContactEmails == null) return;

    bool changed = false;
    for (int i = pStrangerEmails->GetCount() - 1; i >= 0; i--) {
        String* pStranger = static_cast<String*>(pStrangerEmails->GetAt(i));
        if (pStranger == null) continue;

        if (IsKnownContact(*pStranger)) {
            pStrangerEmails->RemoveAt(i, true);
            changed = true;
        }
    }

    if (changed) SaveStrangers();
}

void MessageRouter::LoadStrangers(void) {
    if (pStrangerEmails == null) return;

    File file;
    if (IsFailed(file.Construct(STRANGERS_FILE, L"r"))) return;

    while (true) {
        String line;
        if (IsFailed(file.Read(line))) break;

        line.Trim();
        if (line.IsEmpty()) continue;
        line.ToLower();
        pStrangerEmails->Add(*(new String(line)));
    }
}

void MessageRouter::SaveStrangers(void) {
    if (pStrangerEmails == null) return;

    File file;
    if (IsFailed(file.Construct(STRANGERS_FILE, L"w"))) return;

    for (int i = 0; i < pStrangerEmails->GetCount(); i++) {
        String* pEmail = static_cast<String*>(pStrangerEmails->GetAt(i));
        if (pEmail == null) continue;
        file.Write(*pEmail + L"\n");
    }
}

void MessageRouter::ShowNotification(const String& sender, const String& text, bool isNudge) {
    NotificationManager notiMgr;
    if (IsFailed(notiMgr.Construct())) return;

    String message = sender + L": " + (isNudge ? String(L"🔔 Будильник!") : text);
    if (message.GetLength() > NOTIFICATION_MAX_LENGTH) {
        String shortened;
        message.SubString(0, NOTIFICATION_MAX_LENGTH, shortened);
        message = shortened + L"...";
    }

    int badge = notiMgr.GetBadgeNumber();
    if (badge < 0) badge = 0;

    notiMgr.Notify(message, badge + 1);
}

void MessageRouter::ClearBadge(void) {
    NotificationManager notiMgr;
    if (IsFailed(notiMgr.Construct())) return;
    notiMgr.Notify(0);
}

String MessageRouter::PeekPendingNotificationSender(void) const {
    return pendingNotificationEmail;
}

String MessageRouter::ConsumePendingNotificationSender(void) {
    String sender = pendingNotificationEmail;
    ClearPendingNotificationSender();
    return sender;
}

bool MessageRouter::IsActiveChatWith(const String& email) const {
    if (activeChatEmail.IsEmpty()) return false;
    String clean = MrimUtils::NormalizeEmail(email);
    return !clean.IsEmpty() && clean.Equals(activeChatEmail, true);
}

void MessageRouter::SetPendingNotificationSender(const String& email) {
    pendingNotificationEmail = MrimUtils::NormalizeEmail(email);
    SavePendingNotificationSender();
}

void MessageRouter::ClearPendingNotificationSender(void) {
    pendingNotificationEmail = L"";
    File::Remove(PENDING_NOTI_FILE);
}

void MessageRouter::LoadPendingNotificationSender(void) {
    pendingNotificationEmail = L"";

    File file;
    if (IsFailed(file.Construct(PENDING_NOTI_FILE, L"r"))) return;

    String line;
    if (IsFailed(file.Read(line))) return;

    pendingNotificationEmail = MrimUtils::NormalizeEmail(line);
}

void MessageRouter::SavePendingNotificationSender(void) {
    if (pendingNotificationEmail.IsEmpty()) {
        File::Remove(PENDING_NOTI_FILE);
        return;
    }

    File file;
    if (IsFailed(file.Construct(PENDING_NOTI_FILE, L"w"))) return;
    file.Write(pendingNotificationEmail);
}

void MessageRouter::OnMessageReceived(const String& sender, const String& text, bool isNudge) {
    String cleanSender = MrimUtils::NormalizeEmail(sender);
    if (cleanSender.IsEmpty()) return;
    if (!isNudge && text.IsEmpty()) return;

    String displayText = isNudge ? String(L"ВАМ НАДІСЛАНО БУДИЛЬНИК!") : text;

    ChatHistory::Append(cleanSender, cleanSender, displayText);

    if (!IsKnownContact(cleanSender)) RememberStranger(cleanSender);

    bool isActiveChat = !activeChatEmail.IsEmpty() && cleanSender.Equals(activeChatEmail, true);

    if (isActiveChat && pActiveChatListener != null) {
        pActiveChatListener->OnMessageReceived(sender, text, isNudge);
    }

    if (!isActiveChat || !isAppInForeground) {
        IncrementUnreadCount(cleanSender);
        SetPendingNotificationSender(cleanSender);
        ShowNotification(cleanSender, displayText, isNudge);
    }
}

void MessageRouter::OnMessageDeliveryStatus(unsigned long status) {
    if (pActiveChatListener != null) pActiveChatListener->OnMessageDeliveryStatus(status);
}

void MessageRouter::OnTypingReceived(const String& sender) {
    if (pActiveChatListener != null) pActiveChatListener->OnTypingReceived(sender);
}
