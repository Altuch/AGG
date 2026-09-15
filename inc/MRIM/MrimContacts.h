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
    ContactInfo(void) : flags(0), groupId(0), status(0), blogTime(0) {}
    virtual ~ContactInfo(void) {}

    unsigned long flags;
    unsigned long groupId;
    Osp::Base::String email;
    Osp::Base::String nickname;
    unsigned long status;
    unsigned long blogTime;
    Osp::Base::String blogText;
};

class IContactListListener {
public:
    virtual ~IContactListListener(void) {}
    virtual void OnContactListReceived(Osp::Base::Collection::IList* pGroups, Osp::Base::Collection::IList* pContacts) = 0;
};

class IMicroblogListener {
public:
    virtual ~IMicroblogListener(void) {}
    virtual void OnMicroblogChanged(void) = 0;
};

class MrimContacts {
public:
    MrimContacts(AggConnection* pConn);
    virtual ~MrimContacts(void);

    void SetListener(IContactListListener* pListener);
    void SetMicroblogListener(IMicroblogListener* pListener);
    bool ProcessCommand(unsigned long command, Osp::Base::ByteBuffer& payload);
    Osp::Base::Collection::IList* GetCachedContacts(void);

    void SendAddContact(const Osp::Base::String& email, const Osp::Base::String& nickname, unsigned long groupIdx);
    void SendAddGroup(const Osp::Base::String& name);
    void SendModifyContact(unsigned long contactIdx, const Osp::Base::String& email,
                           const Osp::Base::String& nickname, unsigned long groupIdx, unsigned long flags);
    void SendAuthorize(const Osp::Base::String& email);

private:
    AggConnection* pConnection;
    IContactListListener* pListener;
    IMicroblogListener* pMicroblogListener;

    Osp::Base::Collection::ArrayList* pCachedGroups;
    Osp::Base::Collection::ArrayList* pCachedContacts;

    void ParseContactList2(Osp::Base::ByteBuffer& payload);
    void ParseUserStatus(Osp::Base::ByteBuffer& payload);
    void ParseUserBlogStatus(Osp::Base::ByteBuffer& payload);
    void ParseAddContactAck(Osp::Base::ByteBuffer& payload);
    void NotifyMicroblogChanged(void);
    void ClearCache(void);
};

#endif
