#include "MRIM/MrimAuth.h"
#include "MRIM/MrimProtocol.h"
#include "MRIM/MrimUtils.h"
#include "Core/AggConnection.h"

using namespace Osp::Base;
using namespace Osp::Security::Crypto;

MrimAuth::MrimAuth(AggConnection* pConn) : pConnection(pConn), pListener(null) {}
MrimAuth::~MrimAuth(void) {}

void MrimAuth::SetListener(ILoginListener* pListener) {
    this->pListener = pListener;
}

void MrimAuth::NotifyLoginFailed(const String& reason) {
    if (pListener != null) pListener->OnLoginFailed(reason);
}

bool MrimAuth::ComputeMd5(const String& password, byte digest[16]) {
    int strLen = password.GetLength();

    ByteBuffer inputBuf;
    if (IsFailed(inputBuf.Construct(strLen > 0 ? strLen : 1))) {
        return false;
    }

    for (int i = 0; i < strLen; i++) {
        mchar ch;
        password.GetCharAt(i, ch);
        byte b;
        if (ch < 0x80) {
            b = (byte)ch;
        } else if (ch >= 0x0410 && ch <= 0x044F) {
            b = (byte)(0xC0 + (ch - 0x0410));
        } else {
            switch (ch) {
                case 0x0401: b = 0xA8; break;
                case 0x0451: b = 0xB8; break;
                case 0x0404: b = 0xAA; break;
                case 0x0454: b = 0xBA; break;
                case 0x0407: b = 0xAF; break;
                case 0x0457: b = 0xBF; break;
                case 0x0406: b = 0xB2; break;
                case 0x0456: b = 0xB3; break;
                case 0x0490: b = 0xA5; break;
                case 0x0491: b = 0xB4; break;
                case 0x040E: b = 0xA1; break;
                case 0x045E: b = 0xA2; break;
                case 0x0402: b = 0x90; break;
                case 0x00A0: b = 0xA0; break;
                default:     b = '?';  break;
            }
        }
        inputBuf.SetByte(b);
    }
    inputBuf.Flip();

    Md5Hash md5;
    ByteBuffer* pHash = md5.GetHashN(inputBuf);
    if (pHash == null) {
        return false;
    }

    bool ok = false;
    if (pHash->GetRemaining() == 16) {
        pHash->GetArray(digest, 0, 16);
        ok = true;
    }
    delete pHash;
    return ok;
}

void MrimAuth::SendLogin3(const String& login, const String& password) {
    byte md5Digest[16];
    if (!ComputeMd5(password, md5Digest)) {
        NotifyLoginFailed(L"Внутрішня помилка: не вдалося обчислити MD5.");
        return;
    }

    ByteBuffer payload;
    payload.Construct(512);

    MrimUtils::AppendLPS(payload, login);
    MrimUtils::AppendRawBytes(payload, md5Digest, 16);
    MrimUtils::AppendUL(payload, 0x000007FF);
    MrimUtils::AppendLPS(payload, L"client=\"magent\" version=\"5.8\" build=\"4133\"");
    MrimUtils::AppendLPS(payload, L"ru");
    MrimUtils::AppendUL(payload, 0x00000010);
    MrimUtils::AppendUL(payload, 0x00000001);
    MrimUtils::AppendLPS(payload, L"geo-list");
    MrimUtils::AppendLPS(payload, L"MRA 5.8 (build 4133);");

    payload.Flip();
    pConnection->SendPacket(Mrim::Cmd::LOGIN3, payload);
}

bool MrimAuth::ProcessCommand(unsigned long command, ByteBuffer& payload) {
    switch (command) {
        case Mrim::Cmd::LOGIN_ACK: {
            if (pConnection != null) pConnection->NotifyLoggedIn();
            if (pListener != null) pListener->OnLoginSuccess();
            return true;
        }

        case Mrim::Cmd::LOGIN_REJ: {
            String reason = MrimUtils::ReadLPS(payload);

            NotifyLoginFailed(reason.IsEmpty()
                ? String(L"Невірний логін або пароль!")
                : String(L"Вхід відхилено сервером: ") + reason);
            return true;
        }

        default:
            return false;
    }
}
