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
    if (IsFailed(inputBuf.Construct(strLen > 0 ? strLen * 3 + 1 : 1))) {
        return false;
    }

    for (int i = 0; i < strLen; i++) {
        mchar ch;
        password.GetCharAt(i, ch);
        if (ch < 0x80) {
            inputBuf.SetByte((byte)ch);
        } else if (ch < 0x800) {
            inputBuf.SetByte((byte)(0xC0 | (ch >> 6)));
            inputBuf.SetByte((byte)(0x80 | (ch & 0x3F)));
        } else {
            inputBuf.SetByte((byte)(0xE0 | (ch >> 12)));
            inputBuf.SetByte((byte)(0x80 | ((ch >> 6) & 0x3F)));
            inputBuf.SetByte((byte)(0x80 | (ch & 0x3F)));
        }
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
    payload.Construct(1024);

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
            // Renaissance processLoginThree (LOGIN3 path), unlike processLogin
            // (LOGIN2 path), never broadcasts our presence to contacts after
            // login — nobody learns we went offline->online until we change
            // status manually. Original MRA announces itself, so do it here
            // (also re-announces after every reconnect, same LOGIN_ACK flow).
            if (pConnection != null) pConnection->ChangeStatus(Mrim::Status::ONLINE);
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
