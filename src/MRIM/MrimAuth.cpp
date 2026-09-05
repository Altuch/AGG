#include "MRIM/MrimAuth.h"
#include "MRIM/MrimProtocol.h"
#include "MRIM/MrimUtils.h"
#include "Core/AggConnection.h"

using namespace Osp::Base;

MrimAuth::MrimAuth(AggConnection* pConn) : pConnection(pConn), pListener(null) {}
MrimAuth::~MrimAuth(void) {}

void MrimAuth::SetListener(ILoginListener* pListener) {
    this->pListener = pListener;
}

void MrimAuth::NotifyLoginFailed(const String& reason) {
    if (pListener != null) pListener->OnLoginFailed(reason);
}

void MrimAuth::SendLogin2(const String& login, const String& password) {
    ByteBuffer payload;
    payload.Construct(1024);

    MrimUtils::AppendLPS(payload, login);
    MrimUtils::AppendLPS(payload, password);
    MrimUtils::AppendUL(payload, Mrim::Status::ONLINE);
    MrimUtils::AppendLPS(payload, L"STATUS_ONLINE");
    MrimUtils::AppendLPS(payload, L"Онлайн");
    MrimUtils::AppendLPS(payload, L"");
    MrimUtils::AppendUL(payload, 0x0000FF03); // маска підтримуваних можливостей
    MrimUtils::AppendLPS(payload, L"client=\"magent\" version=\"5.0\" build=\"2094\"");
    MrimUtils::AppendLPS(payload, L"MRA 5.0 (build 2094);");
    payload.Flip();

    pConnection->SendPacket(Mrim::Cmd::LOGIN2, payload);
    AppLog("Sent LOGIN2 packet.");
}

bool MrimAuth::ProcessCommand(unsigned long command, ByteBuffer& payload) {
    switch (command) {
        case Mrim::Cmd::LOGIN_ACK: {
            AppLog("MRIM_CS_LOGIN_ACK - вхід успішний.");

            // З'єднання має дізнатись про це завжди: після
            // автоматичного перепідключення слухача входу вже немає
            // (форма входу знялась після першого успіху), але скинути
            // лічильник повторних спроб усе одно потрібно.
            if (pConnection != null) pConnection->NotifyLoggedIn();

            if (pListener != null) pListener->OnLoginSuccess();
            return true;
        }

        case Mrim::Cmd::LOGIN_REJ: {
            String reason = MrimUtils::ReadLPS(payload);
            AppLog("MRIM_CS_LOGIN_REJ: %S", reason.GetPointer());

            NotifyLoginFailed(reason.IsEmpty()
                ? String(L"Невірний логін або пароль!")
                : String(L"Вхід відхилено сервером: ") + reason);
            return true;
        }

        default:
            return false;
    }
}
