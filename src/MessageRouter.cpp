#include "MessageRouter.h"
#include "MRIM/MrimUtils.h"
#include <FIo.h>
#include <FApp.h>

using namespace Osp::Base;
using namespace Osp::Io;
using namespace Osp::Base::Collection;
using namespace Osp::App;

bool MessageRouter::isAppInForeground = true;

MessageRouter::MessageRouter(void) : pActiveChatListener(null), pUnreadCountListener(null) {
    pUnreadCounts = new ArrayList();
    pUnreadCounts->Construct();

    pKnownContactEmails = new ArrayList();
        pKnownContactEmails->Construct();

        pStrangerEmails = new ArrayList();
        pStrangerEmails->Construct();
        LoadStrangers();
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

void MessageRouter::SetUnreadCountListener(IUnreadCountListener* pListener) {
    pUnreadCountListener = pListener;
}

void MessageRouter::IncrementUnreadCount(const String& email) {
    if (pUnreadCounts == null) return;
    bool found = false;
    for (int i = 0; i < pUnreadCounts->GetCount(); i++) {
        UnreadEntry* pEntry = static_cast<UnreadEntry*>(pUnreadCounts->GetAt(i));
        if (pEntry != null && pEntry->email.Equals(email, true)) {
            pEntry->count++;
            found = true;
            break;
        }
    }
    if (!found) {
    UnreadEntry* pNew = new UnreadEntry();
    	        pNew->email = email;
    	        pNew->count = 1;
    	        pUnreadCounts->Add(*pNew);
    }
    if (pUnreadCountListener != null) pUnreadCountListener->OnUnreadCountChanged();
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
    if (pUnreadCountListener != null) pUnreadCountListener->OnUnreadCountChanged();
}

int MessageRouter::GetUnreadCount(const String& email) const {
    if (pUnreadCounts == null) return 0;

    String cleanEmail = email;
    cleanEmail.Trim();
    cleanEmail.ToLower();

    for (int i = 0; i < pUnreadCounts->GetCount(); i++) {
        UnreadEntry* pEntry = static_cast<UnreadEntry*>(pUnreadCounts->GetAt(i));
        if (pEntry != null && pEntry->email.Equals(cleanEmail, true)) {
            return pEntry->count;
        }
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

    // Якщо когось із "невідомих" відтоді додали в контакти - більше не
    // невідомий.
    PruneKnownStrangers();
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
    for (int i = 0; i < pStrangerEmails->GetCount(); i++) {
        String* pExisting = static_cast<String*>(pStrangerEmails->GetAt(i));
        if (pExisting != null && pExisting->Equals(email, true)) return; // вже знаємо
    }

    pStrangerEmails->Add(*(new String(email)));
    SaveStrangers();

    if (pUnreadCountListener != null) pUnreadCountListener->OnUnreadCountChanged();
}

void MessageRouter::PruneKnownStrangers(void) {
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
    result r = file.Construct(L"/Home/strangers.dat", L"r");
    if (IsFailed(r)) return; // файлу ще немає - це нормально

    while (true) {
        String line;
        r = file.Read(line);
        if (IsFailed(r)) break; // E_END_OF_FILE

        line.Trim();
        if (line.IsEmpty()) continue;
        pStrangerEmails->Add(*(new String(line)));
    }
}

void MessageRouter::SaveStrangers(void) {
    if (pStrangerEmails == null) return;

    File file;
    result r = file.Construct(L"/Home/strangers.dat", L"w");
    if (IsFailed(r)) return;

    for (int i = 0; i < pStrangerEmails->GetCount(); i++) {
        String* pEmail = static_cast<String*>(pStrangerEmails->GetAt(i));
        if (pEmail == null) continue;
        String line = *pEmail + L"\n";
        file.Write(line);
    }
}

void MessageRouter::SetAppForeground(bool foreground) {
    isAppInForeground = foreground;
}

void MessageRouter::SetActiveChat(IMessageListener* pListener, const String& email) {
    pActiveChatListener = pListener;
    activeChatEmail = email;
    activeChatEmail.Trim();
    activeChatEmail.ToLower();

    ClearBadge();
    ResetUnreadCount(activeChatEmail);
}

void MessageRouter::ClearActiveChat(void) {
    pActiveChatListener = null;
    activeChatEmail = L"";
}

void MessageRouter::SaveMessageToHistory(const String& email, const String& sender, const String& text) {
    if (email.IsEmpty()) return;
    String path = MrimUtils::GetHistoryFilePath(email);

    File file;
    result r = file.Construct(path, L"a+");
    if (IsFailed(r)) {
        r = file.Construct(path, L"w");
        if (IsFailed(r)) return;
    }

    String line = sender + L"\t" + text + L"\n";
    file.Write(line);
}

void MessageRouter::ShowNotification(const String& sender, const String& text, bool isNudge) {
    NotificationManager notiMgr;
    result r = notiMgr.Construct();
    if (IsFailed(r)) return;

    String message = sender + L": " + (isNudge ? String(L"🔔 Будильник!") : text);
    if (message.GetLength() > 100) {
        String truncated;
        message.SubString(0, 100, truncated);
        message = truncated + L"...";
    }

    int badgeNumber = notiMgr.GetBadgeNumber();
    if (badgeNumber < 0) badgeNumber = 0;
    badgeNumber++;

    notiMgr.Notify(message, badgeNumber);
}

void MessageRouter::ClearBadge(void) {
    NotificationManager notiMgr;
    result r = notiMgr.Construct();
    if (IsFailed(r)) return;

    notiMgr.Notify(0);
}

void MessageRouter::OnMessageReceived(const String& sender, const String& text, bool isNudge) {
    String cleanSender = sender;
    cleanSender.Trim();
    cleanSender.ToLower();
    if (cleanSender.IsEmpty()) return;

    String displayText = isNudge ? L"🔔 ВАМ НАДІСЛАНО БУДИЛЬНИК!" : text;
    if (!isNudge && text.IsEmpty()) return;

    SaveMessageToHistory(cleanSender, cleanSender, displayText);

    if (!IsKnownContact(cleanSender)) {
        RememberStranger(cleanSender);
    }

    bool isActiveChat = !activeChatEmail.IsEmpty() && cleanSender.Equals(activeChatEmail, true);

    if (isActiveChat && pActiveChatListener != null) {
        pActiveChatListener->OnMessageReceived(sender, text, isNudge);
    }

    if (!isActiveChat || !isAppInForeground) {
        IncrementUnreadCount(cleanSender);
        ShowNotification(cleanSender, displayText, isNudge);
    }
}

void MessageRouter::OnMessageDeliveryStatus(unsigned long status) {
    if (pActiveChatListener != null) {
        pActiveChatListener->OnMessageDeliveryStatus(status);
    }
}

void MessageRouter::OnTypingReceived(const String& sender) {
    if (pActiveChatListener != null) {
        pActiveChatListener->OnTypingReceived(sender);
    }
}
