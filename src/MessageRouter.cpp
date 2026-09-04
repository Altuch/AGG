#include "MessageRouter.h"
#include "MRIM/MrimUtils.h"
#include <FIo.h>
#include <FText.h>
#include <FApp.h>

using namespace Osp::Base;
using namespace Osp::Io;
using namespace Osp::Text;
using namespace Osp::App;

MessageRouter::MessageRouter(void) : pActiveChatListener(null) {}
MessageRouter::~MessageRouter(void) {}

void MessageRouter::SetActiveChat(IMessageListener* pListener, const String& email) {
    pActiveChatListener = pListener;
    activeChatEmail = email;
    activeChatEmail.Trim();
    activeChatEmail.ToLower();
    ClearBadge();
}

void MessageRouter::ClearActiveChat(void) {
    pActiveChatListener = null;
    activeChatEmail = L"";
}

void MessageRouter::SaveMessageToHistory(const String& email, const String& sender, const String& text) {
    if (email.IsEmpty()) return;
    String path = MrimUtils::GetHistoryFilePath(email);

    File file;
    // У bada немає атомарного "додати або створити" - пробуємо дописати,
    // а якщо файлу ще немає, створюємо новий.
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

    // Зберігаємо в історію НЕЗАЛЕЖНО від того, чи відкритий зараз цей чат.
    SaveMessageToHistory(cleanSender, cleanSender, displayText);

    bool isActiveChat = !activeChatEmail.IsEmpty() && cleanSender.Equals(activeChatEmail, true);

    if (isActiveChat && pActiveChatListener != null) {
        // Чат відкритий зараз - хай ChatForm сам покаже повідомлення наживо.
        pActiveChatListener->OnMessageReceived(sender, text, isNudge);
    } else {
        // Чат не відкритий (або взагалі жоден чат не активний) - сповіщення.
        ShowNotification(cleanSender, displayText, isNudge);
    }
}

void MessageRouter::OnMessageDeliveryStatus(unsigned long status) {
    // Статус доставки стосується лише повідомлення, щойно відправленого з
    // відкритого чату - сповіщати про нього деінде немає сенсу.
    if (pActiveChatListener != null) {
        pActiveChatListener->OnMessageDeliveryStatus(status);
    }
}

void MessageRouter::OnTypingReceived(const String& sender) {
    if (pActiveChatListener != null) {
        pActiveChatListener->OnTypingReceived(sender);
    }
}
