#include "MRIM/MrimContacts.h"
#include "MRIM/MrimProtocol.h"
#include "MRIM/MrimUtils.h"
#include "Core/AggConnection.h"

using namespace Osp::Base;
using namespace Osp::Base::Collection;

MrimContacts::MrimContacts(AggConnection* pConn) :
    pConnection(pConn),
    pListener(null),
    pCachedGroups(null),
    pCachedContacts(null) {}

MrimContacts::~MrimContacts(void) {
    ClearCache();
}

void MrimContacts::ClearCache(void) {
    if (pCachedGroups != null) {
        pCachedGroups->RemoveAll(true);
        delete pCachedGroups;
        pCachedGroups = null;
    }
    if (pCachedContacts != null) {
        pCachedContacts->RemoveAll(true);
        delete pCachedContacts;
        pCachedContacts = null;
    }
}

void MrimContacts::SetListener(IContactListListener* pListener) {
    this->pListener = pListener;

    // Якщо кеш є, передаємо його слухачу напряму
    if (this->pListener != null && pCachedGroups != null && pCachedContacts != null) {
        AppLog("MrimContacts: Відновлюємо список контактів з кешу напряму!");
        this->pListener->OnContactListReceived(pCachedGroups, pCachedContacts);
    }
}

bool MrimContacts::ProcessCommand(unsigned long command, ByteBuffer& payload) {
    switch (command) {
        case Mrim::Cmd::USER_INFO: {
            while (payload.GetRemaining() >= 8) {
                String key = MrimUtils::ReadLPS(payload);
                String val = MrimUtils::ReadLPS(payload);
                if (key == L"MRIM.NICKNAME") {
                    pConnection->SetNickname(val);
                    AppLog("Знайдено нікнейм: %S", val.GetPointer());
                }
            }
            return true;
        }
        case Mrim::Cmd::CONTACT_LIST2: {
            ParseContactList2(payload);
            return true;
        }
        case Mrim::Cmd::USER_STATUS: {
            ParseUserStatus(payload);
            return true;
        }
    }
    return false;
}

void MrimContacts::ParseUserStatus(ByteBuffer& payload) {
    unsigned long status = MrimUtils::ReadUL(payload);
    String str1 = MrimUtils::ReadLPS(payload);

    String email;
    int atIndex = -1;
    str1.IndexOf(L"@", 0, atIndex);

    if (atIndex >= 0) {
        email = str1;
    } else {
        MrimUtils::ReadLPS(payload);   // xstatus title
        MrimUtils::ReadLPS(payload);   // xstatus desc
        email = MrimUtils::ReadLPS(payload);
        MrimUtils::ReadUL(payload);    // clientCaps
        MrimUtils::ReadLPS(payload);   // useragent
    }

    email.Trim();
    email.ToLower();
    if (email.IsEmpty() || pCachedContacts == null) return;

    AppLog("MrimContacts: Контакт %S змінив статус на 0x%X", email.GetPointer(), status);

    for (int i = 0; i < pCachedContacts->GetCount(); i++) {
        ContactInfo* pContact = static_cast<ContactInfo*>(pCachedContacts->GetAt(i));
        if (pContact == null) continue;

        String cEmail = pContact->email;
        cEmail.Trim();
        cEmail.ToLower();

        if (cEmail.Equals(email, true)) {
            pContact->status = status;
            break;
        }
    }

    if (pListener != null) {
        pListener->OnContactListReceived(pCachedGroups, pCachedContacts);
    }
}

void MrimContacts::ParseContactList2(ByteBuffer& payload) {
    unsigned long errorCode = MrimUtils::ReadUL(payload);
    if (errorCode != 0) return;

    ClearCache();

    pCachedGroups = new ArrayList();
    pCachedGroups->Construct();

    pCachedContacts = new ArrayList();
    pCachedContacts->Construct();

    unsigned long groupsCount = MrimUtils::ReadUL(payload);
    String groupMask = MrimUtils::ReadLPS(payload);
    String contactMask = MrimUtils::ReadLPS(payload);
    int contactMaskLen = contactMask.GetLength();

    for (unsigned long i = 0; i < groupsCount; i++) {
        if (payload.GetRemaining() < 8) break;
        GroupInfo* pGroup = new GroupInfo();
        pGroup->flags = MrimUtils::ReadUL(payload);
        pGroup->name = (pGroup->flags & Mrim::ContactFlag::UNICODE_NICKNAME)
                     ? MrimUtils::ReadLPSUcs2(payload)
                     : MrimUtils::ReadLPS(payload);
        MrimUtils::SkipFormattedRecord(payload, groupMask, 2);
        pCachedGroups->Add(*pGroup);
    }

    while (payload.GetRemaining() >= 8) {
        ContactInfo* pContact = new ContactInfo();
        pContact->flags = MrimUtils::ReadUL(payload);
        pContact->groupId = MrimUtils::ReadUL(payload);
        pContact->email = MrimUtils::ReadLPS(payload); // CP1251
        pContact->nickname = (pContact->flags & Mrim::ContactFlag::UNICODE_NICKNAME)
                            ? MrimUtils::ReadLPSUcs2(payload)
                            : MrimUtils::ReadLPS(payload);

        MrimUtils::ReadUL(payload); // "чи авторизований" (0/1) - не використовується
        pContact->status = MrimUtils::ReadUL(payload);

        if (contactMaskLen >= 7) {
            MrimUtils::ReadLPS(payload); // телефон (CP1251)
        }
        if (contactMaskLen >= 12) {
            MrimUtils::ReadLPS(payload); // xstatus (CP1251)
            MrimUtils::ReadLPS(payload); // заголовок xstatus (CP1251)
            MrimUtils::ReadLPS(payload); // опис xstatus (CP1251)
            MrimUtils::ReadUL(payload);  // маска функцій
            MrimUtils::ReadLPS(payload); // useragent (CP1251)
        }
        if (contactMaskLen >= 19) {
            MrimUtils::ReadUL(payload);  // айді мікроблог-поста, частина 1
            MrimUtils::ReadUL(payload);  // айді мікроблог-поста, частина 2
            MrimUtils::ReadUL(payload);  // unix-time поста
            MrimUtils::ReadLPS(payload); // текст поста
            MrimUtils::ReadLPS(payload); // зарезервовано
            MrimUtils::ReadLPS(payload); // "ReplyTo" (?)
        }

        MrimUtils::SkipFormattedRecord(payload, contactMask, 19);

        pCachedContacts->Add(*pContact);
    }

    if (this->pListener != null) {
        this->pListener->OnContactListReceived(pCachedGroups, pCachedContacts);
    }
}
