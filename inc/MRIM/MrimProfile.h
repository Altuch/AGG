#ifndef _MRIM_PROFILE_H_
#define _MRIM_PROFILE_H_

#include <FBase.h>

class AggConnection;

// Анкета користувача - те, що повертає MRIM_CS_ANKETA_INFO для одного
// рядка результату пошуку. Сервер (mrimsu/mrim-server) не вміє
// редагувати ці поля, лише повертати вже наявні - тож у AGG це
// виключно перегляд.
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
};

class IProfileListener {
public:
    virtual ~IProfileListener(void) {}
    virtual void OnProfileReceived(const ProfileInfo& info) = 0;
    virtual void OnProfileNotFound(void) = 0;
};

// Анкета (MRIM_CS_WP_REQUEST / MRIM_CS_ANKETA_INFO): пошук за логіном,
// щоб дістати власний профіль. Формат відповіді сервер оголошує сам -
// спершу шле NAME кожного поля, потім значення в тому самому порядку,
// тож розбір іде за оголошеною схемою, а не за фіксованими офсетами.
class MrimProfile {
public:
    MrimProfile(AggConnection* pConn);
    virtual ~MrimProfile(void);

    void SetListener(IProfileListener* pListener);

    // login - повний, "ім'я@домен"; розбивається на два поля пошуку.
    void RequestOwnProfile(const Osp::Base::String& login);

    bool ProcessCommand(unsigned long command, Osp::Base::ByteBuffer& payload);

private:
    AggConnection* pConnection;
    IProfileListener* pListener;
};

#endif
