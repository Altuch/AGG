#include "MRIM/MrimMessages.h"
#include "MRIM/MrimUtils.h"
#include "AggConnection.h"

using namespace Osp::Base;

MrimMessages::MrimMessages(AggConnection* pConn) : pConnection(pConn), pListener(null) {}
MrimMessages::~MrimMessages(void) {}

void MrimMessages::SetListener(IMessageListener* pListener) {
    this->pListener = pListener;
}

// 1. Відправка звичайного текстового повідомлення
void MrimMessages::SendMessageTo(const String& to, const String& text) {
    String cleanTo = to;
    cleanTo.Trim();
    cleanTo.ToLower();

    ByteBuffer payload;
    payload.Construct(2048);
    MrimUtils::AppendUL(payload, 0); // Звичайне повідомлення (Plain text)
    MrimUtils::AppendLPS(payload, cleanTo);      // адресат - CP1251
    MrimUtils::AppendLPSUcs2(payload, text);     // текст повідомлення - UCS2
    MrimUtils::AppendLPS(payload, L""); // RTF порожній
    payload.Flip();

    pConnection->SendPacket(0x1008, payload); // MRIM_CS_MESSAGE
}

// 2. Відправка Будильника (Nudge)
void MrimMessages::SendNudge(const String& to) {
    String cleanTo = to;
    cleanTo.Trim();
    cleanTo.ToLower();

    ByteBuffer payload;
    payload.Construct(1024);
    MrimUtils::AppendUL(payload, MRIM_MSG_FLAG_ALARM); // 0x4000
    MrimUtils::AppendLPS(payload, cleanTo);
    MrimUtils::AppendLPSUcs2(payload, L"Вам надіслано будильник!");
    MrimUtils::AppendLPS(payload, L"");
    payload.Flip();

    pConnection->SendPacket(0x1008, payload);
}

// 3. Відправка статусу «набирає повідомлення...»
void MrimMessages::SendTyping(const String& to) {
    String cleanTo = to;
    cleanTo.Trim();
    cleanTo.ToLower();

    ByteBuffer payload;
    payload.Construct(512);
    // КРИТИЧНО: 0x0400 (TYPING) | 0x0004 (NORECV) = 0x0404
    // Це забороняє серверу генерувати статус доставки і помилку 0x8006
    MrimUtils::AppendUL(payload, MRIM_MSG_FLAG_TYPING | MRIM_MSG_FLAG_NORECV);
    MrimUtils::AppendLPS(payload, cleanTo);
    MrimUtils::AppendLPSUcs2(payload, L" "); // За специфікацією протоколу - 1 пробіл, UCS2
    MrimUtils::AppendLPS(payload, L"");
    payload.Flip();

    pConnection->SendPacket(0x1008, payload);
}

// 4. Підтвердження отримання повідомлення серверу
void MrimMessages::SendMessageRecv(const String& from, unsigned long msgId) {
    String cleanFrom = from;
    cleanFrom.Trim();
    cleanFrom.ToLower();

    ByteBuffer payload;
    payload.Construct(1024);
    MrimUtils::AppendUL(payload, msgId);
    MrimUtils::AppendLPS(payload, cleanFrom);
    payload.Flip();

    pConnection->SendPacket(0x1011, payload); // MRIM_CS_MESSAGE_RECV
}

// 5. Розбір вхідних пакетів
bool MrimMessages::ProcessCommand(unsigned long command, ByteBuffer& payload) {
    switch (command) {
        case 0x1009: { // MRIM_CS_MESSAGE_ACK (Вхідне повідомлення)
            if (payload.GetRemaining() < 8) return true;

            unsigned long msgId = MrimUtils::ReadUL(payload);
            unsigned long flags = MrimUtils::ReadUL(payload);

            String sender = L"";
            if (payload.GetRemaining() >= 4) {
                sender = MrimUtils::ReadLPS(payload);
            }

            // За замовчуванням текст повідомлення йде в UCS2; прапорець
            // MRIM_MSG_FLAG_OLD означає старий, CP1251-кодований формат.
            String text = L"";
            if (payload.GetRemaining() >= 4) {
                text = (flags & MRIM_MSG_FLAG_OLD)
                     ? MrimUtils::ReadLPS(payload)
                     : MrimUtils::ReadLPSUcs2(payload);
            }

            String rtf = L"";
            if (payload.GetRemaining() >= 4) {
                rtf = MrimUtils::ReadLPS(payload);
            }
            (void)rtf;

            // Обробка сповіщення "друкує..."
            if (flags & MRIM_MSG_FLAG_TYPING) {
                if (this->pListener != null) {
                    this->pListener->OnTypingReceived(sender);
                }
                return true;
            }

            bool isNudge = (flags & MRIM_MSG_FLAG_ALARM) != 0;

            if (this->pListener != null && !text.IsEmpty() && text != L" ") {
                this->pListener->OnMessageReceived(sender, text, isNudge);
            }

            // Якщо не стоїть прапорець NORECV — шлемо підтвердження отримання
            if ((flags & MRIM_MSG_FLAG_NORECV) == 0) {
                SendMessageRecv(sender, msgId);
            }

            return true;
        }

        case 0x1012: { // MRIM_CS_MESSAGE_STATUS
            if (payload.GetRemaining() < 4) return true;

            unsigned long status = MrimUtils::ReadUL(payload);
            AppLog("MRIM_CS_MESSAGE_STATUS: 0x%X", status);

            if (this->pListener != null) {
                this->pListener->OnMessageDeliveryStatus(status);
            }
            return true;
        }
    }
    return false;
}
