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
        case 0x100f: { // MRIM_CS_USER_STATUS
            ParseUserStatus(payload);
            return true;
        }
    }
    return false;
}

// Фіксований формат пакета (немає "старого"/"нового" варіанту, як
// вважалося раніше — саме звідси й був краш на пристрої, дивись
// DebugInfo/crashinfo.txt): status, xstatus, title, desc, email,
// clientCaps, client. Усі читання вже безпечні (MrimUtils перевіряє
// GetRemaining() і ніколи не читає за межі буфера).
void MrimContacts::ParseUserStatus(ByteBuffer& payload) {
    unsigned long status = MrimUtils::ReadUL(payload);
    MrimUtils::ReadLPS(payload);       // xstatus (CP1251) - поки не показуємо в UI
    MrimUtils::ReadLPSUcs2(payload);   // title (UCS2)
    MrimUtils::ReadLPSUcs2(payload);   // desc (UCS2)
    String email = MrimUtils::ReadLPS(payload); // CP1251
    MrimUtils::ReadUL(payload);        // clientCaps
    MrimUtils::ReadLPS(payload);       // client (CP1251)

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
    // groupMask/contactMask - не просто прапорці "є/немає", а форматні
    // рядки: кожен символ описує ще одне (маска-залежне) поле в кінці
    // запису групи/контакту. 's' = LPS-рядок, будь-що інше = DWORD.
    String groupMask = MrimUtils::ReadLPS(payload);
    String contactMask = MrimUtils::ReadLPS(payload);

    for (unsigned long i = 0; i < groupsCount; i++) {
        if (payload.GetRemaining() < 8) break;
        GroupInfo* pGroup = new GroupInfo();
        pGroup->flags = MrimUtils::ReadUL(payload);
        pGroup->name = MrimUtils::ReadLPSUcs2(payload);
        MrimUtils::SkipFormattedRecord(payload, groupMask, 2);
        pCachedGroups->Add(*pGroup);
    }

    while (payload.GetRemaining() >= 8) {
        ContactInfo* pContact = new ContactInfo();
        pContact->flags = MrimUtils::ReadUL(payload);
        pContact->groupId = MrimUtils::ReadUL(payload);
        pContact->email = MrimUtils::ReadLPS(payload);       // CP1251
        pContact->nickname = MrimUtils::ReadLPSUcs2(payload); // UCS2

        MrimUtils::ReadUL(payload); // serverFlags (не використовується)
        pContact->status = MrimUtils::ReadUL(payload);

        MrimUtils::ReadLPS(payload);       // phone (CP1251)
        MrimUtils::ReadLPS(payload);       // xstatus (CP1251)
        MrimUtils::ReadLPSUcs2(payload);   // xstatus title (UCS2)
        MrimUtils::ReadLPSUcs2(payload);   // xstatus desc (UCS2)
        MrimUtils::ReadUL(payload);        // featFlags
        MrimUtils::ReadLPS(payload);       // client (CP1251)
        MrimUtils::ReadUL(payload);        // blog post id, hi DWORD
        MrimUtils::ReadUL(payload);        // blog post id, lo DWORD
        MrimUtils::ReadUL(payload);        // blog post timestamp
        MrimUtils::ReadLPSUcs2(payload);   // blog post text (UCS2)

        MrimUtils::SkipFormattedRecord(payload, contactMask, 16);

        pCachedContacts->Add(*pContact);
    }

    if (this->pListener != null) {
        this->pListener->OnContactListReceived(pCachedGroups, pCachedContacts);
    }
}
