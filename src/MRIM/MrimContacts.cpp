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
    if (this->pListener != null && pCachedGroups != null && pCachedContacts != null)
        this->pListener->OnContactListReceived(pCachedGroups, pCachedContacts);
}

bool MrimContacts::ProcessCommand(unsigned long command, ByteBuffer& payload) {
    switch (command) {
        case Mrim::Cmd::USER_INFO: {
            while (payload.GetRemaining() >= 8) {
                String key = MrimUtils::ReadLPS(payload);
                String val = MrimUtils::ReadLPSUcs2(payload);
                if (key == L"MRIM.NICKNAME")
                    pConnection->SetNickname(val);
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
        case Mrim::Cmd::ADD_CONTACT_ACK: {
            ParseAddContactAck(payload);
            return true;
        }
        case Mrim::Cmd::MODIFY_CONTACT_ACK: {
            unsigned long err = MrimUtils::ReadUL(payload);
            if (err == Mrim::ContactError::SUCCESS && pListener != null && pCachedGroups != null)
                pListener->OnContactListReceived(pCachedGroups, pCachedContacts);
            return true;
        }
        case Mrim::Cmd::AUTHORIZE_ACK: {
            String email = MrimUtils::ReadLPS(payload);
            if (pCachedContacts != null) {
                for (int i = 0; i < pCachedContacts->GetCount(); i++) {
                    ContactInfo* p = static_cast<ContactInfo*>(pCachedContacts->GetAt(i));
                    if (p == null) continue;
                    String e = p->email; e.ToLower();
                    String eq = email;   eq.ToLower();
                    if (e.Equals(eq, true)) { p->flags |= Mrim::ContactFlag::AUTHORIZED; break; }
                }
                if (pListener != null)
                    pListener->OnContactListReceived(pCachedGroups, pCachedContacts);
            }
            return true;
        }
    }
    return false;
}

void MrimContacts::ParseUserStatus(ByteBuffer& payload) {
    if (payload.GetRemaining() < 4) return;
    unsigned long status = MrimUtils::ReadUL(payload);

    int afterStatus = payload.GetPosition();
    String xstatusType;
    String xTitle;
    String xDesc;
    String email;
    bool isExtended = false;

    if (payload.GetRemaining() >= 4) {
        int before = payload.GetPosition();
        xstatusType = MrimUtils::ReadLPS(payload);
        int titlePos = payload.GetPosition();
        xTitle = MrimUtils::ReadLPSUcs2(payload);
        if (payload.GetPosition() == titlePos && payload.GetRemaining() < 4) {
            payload.SetPosition(before);
        } else {
            xDesc = MrimUtils::ReadLPSUcs2(payload);
            if (payload.GetRemaining() >= 4) {
                String candidate = MrimUtils::ReadLPS(payload);
                int atIdx = -1;
                if (!candidate.IsEmpty()
                    && MrimUtils::SafeIndexOfChar(candidate, L'@', atIdx)
                    && atIdx >= 0) {
                    email = candidate;
                    isExtended = true;
                    if (payload.GetRemaining() >= 4) MrimUtils::ReadUL(payload);
                    if (payload.GetRemaining() >= 4) MrimUtils::ReadLPS(payload);
                } else {
                    payload.SetPosition(afterStatus);
                }
            } else {
                payload.SetPosition(afterStatus);
            }
        }
    }

    if (!isExtended) {
        payload.SetPosition(afterStatus);
        if (payload.GetRemaining() < 4) return;
        email = MrimUtils::ReadLPS(payload);
    }

    email.Trim();
    email.ToLower();
    if (email.IsEmpty() || pCachedContacts == null) return;

    for (int i = 0; i < pCachedContacts->GetCount(); i++) {
        ContactInfo* p = static_cast<ContactInfo*>(pCachedContacts->GetAt(i));
        if (p == null) continue;
        String e = p->email; e.Trim(); e.ToLower();
        if (e.Equals(email, true)) { p->status = status; break; }
    }

    if (pListener != null)
        pListener->OnContactListReceived(pCachedGroups, pCachedContacts);
}

void MrimContacts::ParseContactList2(ByteBuffer& payload) {
    if (MrimUtils::ReadUL(payload) != 0) return;

    ClearCache();
    pCachedGroups   = new ArrayList(); pCachedGroups->Construct();
    pCachedContacts = new ArrayList(); pCachedContacts->Construct();

    unsigned long groupsCount = MrimUtils::ReadUL(payload);
    String groupMask   = MrimUtils::ReadLPS(payload);
    String contactMask = MrimUtils::ReadLPS(payload);
    int cmLen = contactMask.GetLength();

    for (unsigned long i = 0; i < groupsCount && i < 20; i++) {
        if (payload.GetRemaining() < 8) break;
        GroupInfo* pGroup = new GroupInfo();
        pGroup->flags = MrimUtils::ReadUL(payload);
        pGroup->name  = (pGroup->flags & Mrim::ContactFlag::UNICODE_NICKNAME)
                        ? MrimUtils::ReadLPSUcs2(payload)
                        : MrimUtils::ReadLPS(payload);
        MrimUtils::SkipFormattedRecord(payload, groupMask, 2);
        pCachedGroups->Add(*pGroup);
    }

    while (payload.GetRemaining() >= 8) {
        ContactInfo* pContact = new ContactInfo();

        pContact->flags   = MrimUtils::ReadUL(payload);
        pContact->groupId = MrimUtils::ReadUL(payload);
        pContact->email   = MrimUtils::ReadLPS(payload);
        pContact->nickname = (pContact->flags & Mrim::ContactFlag::UNICODE_NICKNAME)
                             ? MrimUtils::ReadLPSUcs2(payload)
                             : MrimUtils::ReadLPS(payload);
        MrimUtils::ReadUL(payload);
        pContact->status = MrimUtils::ReadUL(payload);

        bool isUnicode = ((pContact->flags & Mrim::ContactFlag::UNICODE_NICKNAME) != 0);

        if (cmLen >= 7)
            MrimUtils::ReadLPS(payload);

        if (cmLen >= 12) {
            MrimUtils::ReadLPS(payload);
            if (isUnicode) {
                MrimUtils::ReadLPSUcs2(payload);
                MrimUtils::ReadLPSUcs2(payload); 
            } else {
                MrimUtils::ReadLPS(payload);
                MrimUtils::ReadLPS(payload);
            }
            MrimUtils::ReadUL(payload);
            MrimUtils::ReadLPS(payload); 
        }

        if (cmLen >= 18) {
            MrimUtils::ReadUL(payload);
            MrimUtils::ReadUL(payload); 
            MrimUtils::ReadUL(payload);
            if (isUnicode) {
                MrimUtils::ReadLPSUcs2(payload);
                MrimUtils::ReadLPSUcs2(payload);
                MrimUtils::ReadLPSUcs2(payload);
            } else {
                MrimUtils::ReadLPS(payload);
                MrimUtils::ReadLPS(payload);
                MrimUtils::ReadLPS(payload);
            }
        }

        MrimUtils::SkipFormattedRecord(payload, contactMask, 18);
        pCachedContacts->Add(*pContact);
    }

    if (pListener != null)
        pListener->OnContactListReceived(pCachedGroups, pCachedContacts);
}

void MrimContacts::ParseAddContactAck(ByteBuffer& payload) {
    unsigned long err = MrimUtils::ReadUL(payload);
    MrimUtils::ReadUL(payload);
    if (err == Mrim::ContactError::SUCCESS && pListener != null && pCachedGroups != null)
        pListener->OnContactListReceived(pCachedGroups, pCachedContacts);
}

void MrimContacts::SendAddContact(const String& email, const String& nickname, unsigned long groupIdx) {
    // Nickname keeps its case; only the address is normalized.
    String cleanEmail = MrimUtils::NormalizeEmail(email);
    if (cleanEmail.IsEmpty()) return;
    ByteBuffer payload;
    payload.Construct(1024);
    MrimUtils::AppendUL(payload, 0x00000000);
    MrimUtils::AppendUL(payload, groupIdx);
    MrimUtils::AppendLPS(payload, cleanEmail);
    MrimUtils::AppendLPSUcs2(payload, nickname.IsEmpty() ? cleanEmail : nickname);
    MrimUtils::AppendUL(payload, 0);
    MrimUtils::AppendLPS(payload, L"");
    payload.Flip();
    pConnection->SendPacket(Mrim::Cmd::ADD_CONTACT, payload);
}

void MrimContacts::SendAddGroup(const String& name) {
    ByteBuffer payload;
    payload.Construct(128);
    MrimUtils::AppendUL(payload, Mrim::ContactFlag::FL_GROUP);
    MrimUtils::AppendUL(payload, 0);
    MrimUtils::AppendLPS(payload, name);
    MrimUtils::AppendLPS(payload, L"");
    MrimUtils::AppendLPS(payload, L"");
    MrimUtils::AppendLPS(payload, L"");
    payload.Flip();
    pConnection->SendPacket(Mrim::Cmd::ADD_CONTACT, payload);
}

void MrimContacts::SendModifyContact(unsigned long contactIdx, const String& email,
                                     const String& nickname, unsigned long groupIdx,
                                     unsigned long flags) {
    String cleanEmail = MrimUtils::NormalizeEmail(email);
    if (cleanEmail.IsEmpty()) return;
    ByteBuffer payload;
    payload.Construct(1024);
    MrimUtils::AppendUL(payload, contactIdx);
    MrimUtils::AppendUL(payload, flags);
    MrimUtils::AppendUL(payload, groupIdx);
    MrimUtils::AppendLPS(payload, cleanEmail);
    MrimUtils::AppendLPSUcs2(payload, nickname); 
    MrimUtils::AppendLPS(payload, L"");
    payload.Flip();
    pConnection->SendPacket(Mrim::Cmd::MODIFY_CONTACT, payload);
}

void MrimContacts::SendAuthorize(const String& email) {
    String cleanEmail = MrimUtils::NormalizeEmail(email);
    if (cleanEmail.IsEmpty()) return;
    ByteBuffer payload;
    payload.Construct(128);
    MrimUtils::AppendLPS(payload, cleanEmail);
    payload.Flip();
    pConnection->SendPacket(Mrim::Cmd::AUTHORIZE, payload);
}
