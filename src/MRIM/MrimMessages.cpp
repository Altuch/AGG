#include "MRIM/MrimMessages.h"
#include "Core/Loc.h"
#include "MRIM/MrimProtocol.h"
#include "MRIM/MrimUtils.h"
#include "Core/AggConnection.h"

using namespace Osp::Base;

static const int MAX_OFFLINE_BODY_CHARS = 20000;

MrimMessages::MrimMessages(AggConnection* pConn) : pConnection(pConn), pListener(null) {}
MrimMessages::~MrimMessages(void) {}

void MrimMessages::SetListener(IMessageListener* pListener) {
    this->pListener = pListener;
}

static void BuildMessagePayload(ByteBuffer& payload,
                                 unsigned long flags,
                                 const String& to,
                                 const String& text) {
    String cleanTo = MrimUtils::NormalizeEmail(to);

    MrimUtils::AppendUL(payload, flags);
    MrimUtils::AppendLPS(payload, cleanTo);
    MrimUtils::AppendLPSUcs2(payload, text);
    MrimUtils::AppendLPS(payload, L"");
    payload.Flip();
}

void MrimMessages::SendMessageTo(const String& to, const String& text) {
    ByteBuffer payload;
    payload.Construct(8192);
    BuildMessagePayload(payload, 0, to, text);
    pConnection->SendPacket(Mrim::Cmd::MESSAGE, payload);
}

void MrimMessages::SendNudge(const String& to) {
    ByteBuffer payload;
    payload.Construct(1024);
    BuildMessagePayload(payload, Mrim::MsgFlag::ALARM, to, LocString(L"IDS_NUDGE_PAYLOAD"));
    pConnection->SendPacket(Mrim::Cmd::MESSAGE, payload);
}

void MrimMessages::SendTyping(const String& to) {
    ByteBuffer payload;
    payload.Construct(1024);
    BuildMessagePayload(payload, Mrim::MsgFlag::TYPING | Mrim::MsgFlag::NORECV, to, L" ");
    pConnection->SendPacket(Mrim::Cmd::MESSAGE, payload);
}

void MrimMessages::SendMessageRecv(const String& from, unsigned long msgId) {
    String cleanFrom = MrimUtils::NormalizeEmail(from);

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

            String text;
            if (payload.GetRemaining() >= 4) text = MrimUtils::ReadLPSUcs2(payload);

            if (flags & Mrim::MsgFlag::TYPING) {
                if (pListener != null) pListener->OnTypingReceived(sender);
                return true;
            }

            bool isNudge = (flags & Mrim::MsgFlag::ALARM) != 0;
            if (pListener != null && !text.IsEmpty() && text != L" ") {
                pListener->OnMessageReceived(sender, text, isNudge);
            }

            if ((flags & Mrim::MsgFlag::NORECV) == 0) SendMessageRecv(sender, msgId);
            return true;
        }

        case Mrim::Cmd::OFFLINE_MESSAGE_ACK: {
            if (payload.GetRemaining() < 8) return true;

            MrimUtils::ReadUL(payload);
            MrimUtils::ReadUL(payload);

            HandleOfflineMessageEnvelope(MrimUtils::ReadLPS(payload));
            SendOfflineMessageDelete();
            return true;
        }

        case Mrim::Cmd::MESSAGE_STATUS: {
            if (payload.GetRemaining() < 4) return true;

            unsigned long status = MrimUtils::ReadUL(payload);

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

    String sender;
    int fromPos = -1;
    if (!MrimUtils::SafeIndexOf(envelope, String(L"From: "), 0, fromPos)) return;
    if (fromPos >= 0 && fromPos < envLen) {
        int lineEnd = -1;
        MrimUtils::SafeIndexOf(envelope, String(L"\r\n"), fromPos, lineEnd);

        int start = fromPos + 6;
        if (start >= 0 && start <= envLen && lineEnd > start && lineEnd <= envLen) {
            envelope.SubString(start, lineEnd - start, sender);
        }
    }
    sender.Trim();
    if (sender.IsEmpty()) return;

    int bodyStart = -1;
    if (!MrimUtils::SafeIndexOf(envelope, String(L"\r\n\r\n"), 0, bodyStart)) return;
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
