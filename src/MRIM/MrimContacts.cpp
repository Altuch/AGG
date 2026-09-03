#include "MRIM/MrimContacts.h"
#include "MRIM/MrimUtils.h"
#include "AggConnection.h"

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
        case 0x1015: { // MRIM_CS_USER_INFO
            while (payload.GetRemaining() >= 8) {
                String key = MrimUtils::ReadLPS(payload);
                String val = MrimUtils::ReadLPS(payload);
                if (key == L"MRIM.NICKNAME") {
                    pConnection->userNickname = val;
                    AppLog("Знайдено нікнейм: %S", val.GetPointer());
                }
            }
            return true;
        }
        case 0x1037: { // MRIM_CS_CONTACT_LIST2
            ParseContactList2(payload);
            return true;
        }
    }
    return false;
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

    for (unsigned long i = 0; i < groupsCount; i++) {
        GroupInfo* pGroup = new GroupInfo();
        pGroup->flags = MrimUtils::ReadUL(payload);
        pGroup->name = MrimUtils::ReadLPS(payload);
        pCachedGroups->Add(*pGroup);
    }

    while (payload.GetRemaining() >= 8) {
        ContactInfo* pContact = new ContactInfo();
        pContact->flags = MrimUtils::ReadUL(payload);
        pContact->groupId = MrimUtils::ReadUL(payload);
        pContact->email = MrimUtils::ReadLPS(payload);
        pContact->nickname = MrimUtils::ReadLPS(payload);

        unsigned long isAuth = MrimUtils::ReadUL(payload);
        (void)isAuth;

        pContact->status = MrimUtils::ReadUL(payload);

        if (contactMask.GetLength() >= 7 && payload.GetRemaining() >= 4) {
            String phone = MrimUtils::ReadLPS(payload);
            (void)phone;
        }

        pCachedContacts->Add(*pContact);
    }

    if (this->pListener != null) {
        this->pListener->OnContactListReceived(pCachedGroups, pCachedContacts);
    }
}
