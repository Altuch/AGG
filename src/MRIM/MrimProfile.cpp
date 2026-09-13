#include "MRIM/MrimProfile.h"
#include "MRIM/MrimProtocol.h"
#include "MRIM/MrimUtils.h"
#include "Core/AggConnection.h"

using namespace Osp::Base;

MrimProfile::MrimProfile(AggConnection* pConn) : pConnection(pConn), pListener(null) {}
MrimProfile::~MrimProfile(void) {}

void MrimProfile::SetListener(IProfileListener* pListener) {
    this->pListener = pListener;
}

void MrimProfile::RequestProfileFor(const Osp::Base::String& login) {
    String user = login;
    String domain;

    int atPos = -1;
    if (!login.IsEmpty()) login.IndexOf(L"@", 0, atPos);
    if (atPos >= 0) {
        login.SubString(0, atPos, user);
        login.SubString(atPos + 1, domain);
    }
    user.Trim();
    user.ToLower();
    domain.Trim();
    domain.ToLower();

    ByteBuffer payload;
    payload.Construct(256);
    MrimUtils::AppendUL(payload, Mrim::Anketa::SearchField::USER);
    MrimUtils::AppendLPS(payload, user);
    if (!domain.IsEmpty()) {
        MrimUtils::AppendUL(payload, Mrim::Anketa::SearchField::DOMAIN);
        MrimUtils::AppendLPS(payload, domain);
    }
    payload.Flip();

    pConnection->SendPacket(Mrim::Cmd::WP_REQUEST, payload);
}

bool MrimProfile::ProcessCommand(unsigned long command, ByteBuffer& payload) {
    if (command != Mrim::Cmd::ANKETA_INFO) return false;

    if (payload.GetRemaining() < 16) return true;

    unsigned long status = MrimUtils::ReadUL(payload);
    unsigned long fieldCount = MrimUtils::ReadUL(payload);
    unsigned long rowCount = MrimUtils::ReadUL(payload);
    MrimUtils::ReadUL(payload);

    if (status != Mrim::Anketa::Status::OK || rowCount == 0 || fieldCount == 0) {
        if (pListener != null) pListener->OnProfileNotFound();
        return true;
    }

    if (fieldCount > 32) fieldCount = 32;
    String fieldNames[32];
    for (unsigned long i = 0; i < fieldCount; i++) {
        fieldNames[i] = MrimUtils::ReadLPS(payload); 
    }

    ProfileInfo info;
    for (unsigned long row = 0; row < rowCount; row++) {
        for (unsigned long i = 0; i < fieldCount; i++) {
            const String& name = fieldNames[i];
            bool isAsciiField = (name == L"Username" || name == L"Domain"
                || name == L"Phone" || name == L"Birthday" || name == L"Zodiac"
                || name == L"Sex" || name == L"mrim_status" || name == L"Status");
            String value = isAsciiField
                ? MrimUtils::ReadLPS(payload)
                : MrimUtils::ReadLPSUcs2(payload);
            if (row != 0) continue;

            if (name == L"Username") info.username = value;
            else if (name == L"Nickname") info.nickname = value;
            else if (name == L"Domain") info.domain = value;
            else if (name == L"FirstName") info.firstName = value;
            else if (name == L"LastName") info.lastName = value;
            else if (name == L"Location") info.location = value;
            else if (name == L"Birthday") info.birthday = value;
            else if (name == L"Zodiac") info.zodiac = value;
            else if (name == L"Phone") info.phone = value;
            else if (name == L"Sex") info.sex = value;
            else if (name == L"mrim_status" || name == L"Status") info.status = value;
        }
    }

    if (pListener != null) pListener->OnProfileReceived(info);
    return true;
}
