#ifndef _MRIM_PROFILE_H_
#define _MRIM_PROFILE_H_

#include <FBase.h>
#include <FBaseColArrayList.h>

class AggConnection;

class ProfileInfo : public Osp::Base::Object {
public:
    ProfileInfo(void) {}
    virtual ~ProfileInfo(void) {}

    Osp::Base::String username;
    Osp::Base::String nickname;
    Osp::Base::String domain;
    Osp::Base::String firstName;
    Osp::Base::String lastName;
    Osp::Base::String location;
    Osp::Base::String birthday;
    Osp::Base::String zodiac;
    Osp::Base::String phone;
    Osp::Base::String sex;
    Osp::Base::String status;
};

class IProfileListener {
public:
    virtual ~IProfileListener(void) {}
    virtual void OnProfileReceived(const ProfileInfo& info) = 0;
    virtual void OnProfileNotFound(void) = 0;
};

class MrimProfile {
public:
    MrimProfile(AggConnection* pConn);
    virtual ~MrimProfile(void);

    void SetListener(IProfileListener* pListener);

    void RequestProfileFor(const Osp::Base::String& login);
    void RequestProfileFor(const Osp::Base::String& login, IProfileListener* pListener);

    bool ProcessCommand(unsigned long command, Osp::Base::ByteBuffer& payload);

private:
    static Osp::Base::String NormalizeKey(const Osp::Base::String& user, const Osp::Base::String& domain);

    AggConnection* pConnection;
    IProfileListener* pListener;
    Osp::Base::Collection::ArrayList* pPending;
};

#endif
