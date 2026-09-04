#include "MRIM/MrimMessages.h"
#include "MRIM/MrimUtils.h"
#include "AggConnection.h"

using namespace Osp::Base;

MrimMessages::MrimMessages(AggConnection* pConn) : pConnection(pConn), pListener(null) {}
MrimMessages::~MrimMessages(void) {}

void MrimMessages::SetListener(IMessageListener* pListener) {
    this->pListener = pListener;
}

void MrimMessages::SendMessageTo(const String& to, const String& text) {
    String cleanTo = to;
    cleanTo.Trim();
    cleanTo.ToLower();

    ByteBuffer payload;
    payload.Construct(2048);
    MrimUtils::AppendUL(payload, 0);
    MrimUtils::AppendLPS(payload, cleanTo);
    MrimUtils::AppendLPS(payload, text);
    MrimUtils::AppendLPS(payload, L"");
    payload.Flip();

    pConnection->SendPacket(0x1008, payload);
}

void MrimMessages::SendNudge(const String& to) {
    String cleanTo = to;
    cleanTo.Trim();
    cleanTo.ToLower();

    ByteBuffer payload;
    payload.Construct(1024);
    MrimUtils::AppendUL(payload, MRIM_MSG_FLAG_ALARM); // 0x4000
    MrimUtils::AppendLPS(payload, cleanTo);
    MrimUtils::AppendLPS(payload, L"Вам надіслано будильник!");
    MrimUtils::AppendLPS(payload, L"");
    payload.Flip();

    pConnection->SendPacket(0x1008, payload);
}

void MrimMessages::SendTyping(const String& to) {
    String cleanTo = to;
    cleanTo.Trim();
    cleanTo.ToLower();

    ByteBuffer payload;
    payload.Construct(512);
    MrimUtils::AppendUL(payload, MRIM_MSG_FLAG_TYPING | MRIM_MSG_FLAG_NORECV);
    MrimUtils::AppendLPS(payload, cleanTo);
    MrimUtils::AppendLPS(payload, L" ");
    MrimUtils::AppendLPS(payload, L"");
    payload.Flip();

    pConnection->SendPacket(0x1008, payload);
}

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

bool MrimMessages::ProcessCommand(unsigned long command, ByteBuffer& payload) {
    switch (command) {
        case 0x1009: {
            if (payload.GetRemaining() < 8) return true;

            unsigned long msgId = MrimUtils::ReadUL(payload);
            unsigned long flags = MrimUtils::ReadUL(payload);

            String sender = L"";
            if (payload.GetRemaining() >= 4) {
                sender = MrimUtils::ReadLPS(payload);
            }

            // AGG заявляє протокол MRIM 1.8 - текст повідомлення тут CP1251
            // (UTF-16LE в MRIM з'являється лише з 1.16).
            String text = L"";
            if (payload.GetRemaining() >= 4) {
                text = MrimUtils::ReadLPS(payload);
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
        case 0x101D: { // MRIM_CS_OFFLINE_MESSAGE_ACK - офлайн-повідомлення, надіслані при вході
                    if (payload.GetRemaining() < 8) return true;
                    MrimUtils::ReadUL(payload); // id (64-біт, low) - не потрібен, див. HandleOfflineMessageEnvelope
                    MrimUtils::ReadUL(payload); // id (64-біт, high)

                    String envelope = MrimUtils::ReadLPS(payload);
                    HandleOfflineMessageEnvelope(envelope);

                    // Одразу підтверджуємо - сервер на 0x101E чистить ВСЮ чергу
                    // офлайн-повідомлень незалежно від id, тож повторний виклик
                    // після кожного повідомлення нешкідливий.
                    SendOfflineMessageDelete();
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

void MrimMessages::SendOfflineMessageDelete(void) {
    ByteBuffer payload;
    payload.Construct(8);
    // 64-бітний id за протоколом, але сервер (mrim-server) його
    // ігнорує й чистить чергу цілком - нулі тут абсолютно достатньо.
    MrimUtils::AppendUL(payload, 0);
    MrimUtils::AppendUL(payload, 0);
    payload.Flip();

    pConnection->SendPacket(0x101E, payload); // MRIM_CS_OFFLINE_MESSAGE_DELETE
}

void MrimMessages::HandleOfflineMessageEnvelope(const String& envelope) {
	int envLen = envelope.GetLength();
	if (envLen <= 0) return;
    String sender = L"";
    int fromPos = -1;
    envelope.IndexOf(L"From: ", 0, fromPos);
    if (fromPos >= 0 && fromPos < envLen) {
        int lineEnd = -1;
        envelope.IndexOf(L"\r\n", fromPos, lineEnd);
        int start = fromPos + 6; // довжина "From: "
                if (start >= 0 && start <= envLen && lineEnd > start && lineEnd <= envLen) {
                    envelope.SubString(start, lineEnd - start, sender);
                }
    }
    sender.Trim();
    if (sender.IsEmpty()) return;

    int bodyStart = -1;
    envelope.IndexOf(L"\r\n\r\n", 0, bodyStart);
    if (bodyStart < 0) return;
    bodyStart += 4;
    if (bodyStart < 0 || bodyStart > envLen) return;

    String base64Body;
    if (bodyStart < envLen) {
    	envelope.SubString(bodyStart, base64Body);
    }
    base64Body.Trim();
    if (base64Body.IsEmpty()) return;

    if (base64Body.GetLength() > 20000) {
            String truncated;
            base64Body.SubString(0, 20000, truncated);
            base64Body = truncated;
    }

    String text = MrimUtils::Base64DecodeUtf16LEToString(base64Body);
    if (text.IsEmpty()) return;

    if (this->pListener != null) {
        this->pListener->OnMessageReceived(sender, text, false);
    }
}
