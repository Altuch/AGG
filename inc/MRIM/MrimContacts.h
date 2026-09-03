#ifndef _MRIM_CONTACTS_H_
#define _MRIM_CONTACTS_H_

#include <FBase.h>

class AggConnection;

class GroupInfo : public Osp::Base::Object {
public:
    GroupInfo(void) : flags(0) {}
    virtual ~GroupInfo(void) {}

    unsigned long flags;
    Osp::Base::String name;
};

class ContactInfo : public Osp::Base::Object {
public:
    ContactInfo(void) : flags(0), groupId(0), status(0) {}
    virtual ~ContactInfo(void) {}

    unsigned long flags;
    unsigned long groupId;
    Osp::Base::String email;
    Osp::Base::String nickname;
    unsigned long status;
};

class IContactListListener {
public:
    virtual ~IContactListListener(void) {}
    virtual void OnContactListReceived(Osp::Base::Collection::IList* pGroups, Osp::Base::Collection::IList* pContacts) = 0;
};

class MrimContacts {
public:
    MrimContacts(AggConnection* pConn);
    virtual ~MrimContacts(void);

    void SetListener(IContactListListener* pListener);
    bool ProcessCommand(unsigned long command, Osp::Base::ByteBuffer& payload);

private:
    AggConnection* pConnection;
    IContactListListener* pListener;

    Osp::Base::Collection::ArrayList* pCachedGroups;
    Osp::Base::Collection::ArrayList* pCachedContacts;

    void ParseContactList2(Osp::Base::ByteBuffer& payload);
    void ParseUserStatus(Osp::Base::ByteBuffer& payload);
    void ClearCache(void);
};

#endif
