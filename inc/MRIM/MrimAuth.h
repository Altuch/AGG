#ifndef _MRIM_AUTH_H_
#define _MRIM_AUTH_H_

#include <FBase.h>
#include <FSecurity.h>

class AggConnection;

class ILoginListener {
public:
    virtual ~ILoginListener(void) {}
    virtual void OnLoginSuccess(void) = 0;
    virtual void OnLoginFailed(const Osp::Base::String& reason) = 0;
};

class MrimAuth {
public:
    MrimAuth(AggConnection* pConn);
    virtual ~MrimAuth(void);

    void SetListener(ILoginListener* pListener);
    void SendLogin3(const Osp::Base::String& login, const Osp::Base::String& password);
    bool ProcessCommand(unsigned long command, Osp::Base::ByteBuffer& payload);
    void NotifyLoginFailed(const Osp::Base::String& reason);

private:
    static bool ComputeMd5(const Osp::Base::String& password, byte digest[16]);

    AggConnection* pConnection;
    ILoginListener* pListener;
};

#endif
