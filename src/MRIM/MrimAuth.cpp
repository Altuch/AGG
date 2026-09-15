#include "MRIM/MrimAuth.h"
#include "Core/Loc.h"
#include "MRIM/MrimProtocol.h"
#include "MRIM/MrimUtils.h"
#include "Core/AggConnection.h"
#include <FLclLocaleManager.h>
#include <FLclLocale.h>

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

String MrimAuth::GetClientVersion(void) {
    Osp::App::Application* pApp = Osp::App::Application::GetInstance();
    String version = (pApp != null) ? pApp->GetAppVersion() : String(L"");
    version.Trim();
    if (version.IsEmpty()) version = L"0.1";
    return version;
}

String MrimAuth::GetClientLocale(void) {    Osp::Locales::LocaleManager localeManager;
    if (IsFailed(localeManager.Construct())) return String(L"ru");
    String lang = localeManager.GetSystemLocale().GetLanguageCodeString();
    if (lang.StartsWith(L"uk", 0)) return String(L"uk");
    if (lang.StartsWith(L"en", 0)) return String(L"en");
    if (lang.StartsWith(L"ru", 0)) return String(L"ru");
    return String(L"ru");
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
        NotifyLoginFailed(LocString(L"IDS_AUTH_MD5"));
        return;
    }

    ByteBuffer payload;
    payload.Construct(1024);

    MrimUtils::AppendLPS(payload, login);
    MrimUtils::AppendRawBytes(payload, md5Digest, 16);
    MrimUtils::AppendUL(payload, 0x000007FF);
    String clientVersion = GetClientVersion();
    MrimUtils::AppendLPS(payload, L"client=\"AGG\" version=\"" + clientVersion + L"\" build=\"1\"");
    MrimUtils::AppendLPS(payload, GetClientLocale());
    MrimUtils::AppendUL(payload, 0x00000010);
    MrimUtils::AppendUL(payload, 0x00000001);
    MrimUtils::AppendLPS(payload, L"geo-list");
    MrimUtils::AppendLPS(payload, L"AGG " + clientVersion + L";");

    payload.Flip();
    pConnection->SendPacket(Mrim::Cmd::LOGIN3, payload);
}

bool MrimAuth::ProcessCommand(unsigned long command, ByteBuffer& payload) {
    switch (command) {
        case Mrim::Cmd::LOGIN_ACK: {
            if (pConnection != null) pConnection->NotifyLoggedIn();

            if (pConnection != null) pConnection->ChangeStatus(Mrim::Status::ONLINE);
            if (pListener != null) pListener->OnLoginSuccess();
            return true;
        }

        case Mrim::Cmd::LOGIN_REJ: {
            String reason = MrimUtils::ReadLPS(payload);

            NotifyLoginFailed(reason.IsEmpty()
                ? String(LocString(L"IDS_AUTH_REJECTED"))
                : String(LocString(L"IDS_AUTH_REJECT_PREFIX")) + reason);
            return true;
        }

        default:
            return false;
    }
}
