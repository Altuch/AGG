#include "MRIM/MrimMessages.h"
#include "MRIM/MrimProtocol.h"
#include "MRIM/MrimUtils.h"
#include "Core/AggConnection.h"

using namespace Osp::Base;

// Стеля на тіло офлайн-повідомлення (символів base64). Сервер обмежує
// повідомлення ~5000 символами, тобто base64(UTF-16LE) від нього -
// близько 13400; запас узятий свідомо, аби похибка розбору вище не
// призвела до спроби виділити щось несосвітенне.
static const int MAX_OFFLINE_BODY_CHARS = 20000;

MrimMessages::MrimMessages(AggConnection* pConn) : pConnection(pConn), pListener(null) {}
MrimMessages::~MrimMessages(void) {}

void MrimMessages::SetListener(IMessageListener* pListener) {
    this->pListener = pListener;
}

// Тіло MRIM_CS_MESSAGE однакове для звичайного повідомлення, будильника
// і "друкує...": прапорці, адресат, текст, RTF (у нас завжди порожній).
static void BuildMessagePayload(ByteBuffer& payload,
                                unsigned long flags,
                                const String& to,
                                const String& text) {
    String cleanTo = to;
    cleanTo.Trim();
    cleanTo.ToLower();

    MrimUtils::AppendUL(payload, flags);
    MrimUtils::AppendLPS(payload, cleanTo);
    MrimUtils::AppendLPS(payload, text);
    MrimUtils::AppendLPS(payload, L"");
    payload.Flip();
}

void MrimMessages::SendMessageTo(const String& to, const String& text) {
    ByteBuffer payload;
    payload.Construct(2048);
    BuildMessagePayload(payload, 0, to, text);
    pConnection->SendPacket(Mrim::Cmd::MESSAGE, payload);
}

void MrimMessages::SendNudge(const String& to) {
    ByteBuffer payload;
    payload.Construct(1024);
    BuildMessagePayload(payload, Mrim::MsgFlag::ALARM, to, L"Вам надіслано будильник!");
    pConnection->SendPacket(Mrim::Cmd::MESSAGE, payload);
}

void MrimMessages::SendTyping(const String& to) {
    ByteBuffer payload;
    payload.Construct(512);
    // NORECV обов'язковий: інакше сервер шле статус доставки на кожне
    // натискання клавіші. Текст - рівно один пробіл, як вимагає протокол.
    BuildMessagePayload(payload, Mrim::MsgFlag::TYPING | Mrim::MsgFlag::NORECV, to, L" ");
    pConnection->SendPacket(Mrim::Cmd::MESSAGE, payload);
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

    pConnection->SendPacket(Mrim::Cmd::MESSAGE_RECV, payload);
}

void MrimMessages::SendOfflineMessageDelete(void) {
    ByteBuffer payload;
    payload.Construct(8);
    // За протоколом тут 64-бітний id, але сервер його не дивиться -
    // просто чистить чергу цілком, тож нулів достатньо.
    MrimUtils::AppendUL(payload, 0);
    MrimUtils::AppendUL(payload, 0);
    payload.Flip();

    pConnection->SendPacket(Mrim::Cmd::OFFLINE_MESSAGE_DELETE, payload);
}

bool MrimMessages::ProcessCommand(unsigned long command, ByteBuffer& payload) {
    switch (command) {
        case Mrim::Cmd::MESSAGE_ACK: {
            if (payload.GetRemaining() < 8) return true;

            unsigned long msgId = MrimUtils::ReadUL(payload);
            unsigned long flags = MrimUtils::ReadUL(payload);

            String sender;
            if (payload.GetRemaining() >= 4) sender = MrimUtils::ReadLPS(payload);

            // Протокол 1.8 - текст у CP1251 (UTF-16LE лише з 1.16).
            String text;
            if (payload.GetRemaining() >= 4) text = MrimUtils::ReadLPS(payload);

            if (flags & Mrim::MsgFlag::TYPING) {
                if (pListener != null) pListener->OnTypingReceived(sender);
                return true;
            }

            bool isNudge = (flags & Mrim::MsgFlag::ALARM) != 0;
            if (pListener != null && !text.IsEmpty() && text != L" ") {
                pListener->OnMessageReceived(sender, text, isNudge);
            }

            // Без підтвердження сервер вважатиме повідомлення недоставленим.
            if ((flags & Mrim::MsgFlag::NORECV) == 0) SendMessageRecv(sender, msgId);
            return true;
        }

        case Mrim::Cmd::OFFLINE_MESSAGE_ACK: {
            if (payload.GetRemaining() < 8) return true;

            // 64-бітний id: молодше слово, потім старше. Нам не
            // потрібен - видалення все одно чистить чергу цілком.
            MrimUtils::ReadUL(payload);
            MrimUtils::ReadUL(payload);

            HandleOfflineMessageEnvelope(MrimUtils::ReadLPS(payload));
            SendOfflineMessageDelete();
            return true;
        }

        case Mrim::Cmd::MESSAGE_STATUS: {
            if (payload.GetRemaining() < 4) return true;

            unsigned long status = MrimUtils::ReadUL(payload);
            AppLog("MRIM_CS_MESSAGE_STATUS: 0x%X", status);

            if (pListener != null) pListener->OnMessageDeliveryStatus(status);
            return true;
        }

        default:
            return false;
    }
}

void MrimMessages::HandleOfflineMessageEnvelope(const String& envelope) {
    int envLen = envelope.GetLength();
    if (envLen <= 0) return;

    // Кожен обчислений індекс тримаємо в межах рядка ДО виклику
    // SubString: цей клас, на відміну від наших читачів, меж сам не
    // перевіряє, і вихід за них раніше валив застосунок.
    String sender;
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

    // Порожній рядок відділяє заголовки від тіла.
    int bodyStart = -1;
    envelope.IndexOf(L"\r\n\r\n", 0, bodyStart);
    if (bodyStart < 0) return;

    bodyStart += 4;
    if (bodyStart >= envLen) return;

    String body;
    envelope.SubString(bodyStart, body);
    body.Trim();
    if (body.IsEmpty()) return;

    if (body.GetLength() > MAX_OFFLINE_BODY_CHARS) {
        String shortened;
        body.SubString(0, MAX_OFFLINE_BODY_CHARS, shortened);
        body = shortened;
    }

    String text = MrimUtils::Base64DecodeUtf16LEToString(body);
    if (text.IsEmpty()) return;

    if (pListener != null) pListener->OnMessageReceived(sender, text, false);
}
