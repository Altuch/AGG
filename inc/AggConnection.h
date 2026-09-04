#ifndef _AGG_CONNECTION_H_
#define _AGG_CONNECTION_H_

#include <FNet.h>
#include <FBase.h>

#include "MRIM/MrimAuth.h"
#include "MRIM/MrimContacts.h"
#include "MRIM/MrimMessages.h"

class MessageRouter;


class AggConnection : public Osp::Net::Sockets::ISocketEventListener,
                      public Osp::Base::Runtime::ITimerEventListener
{
public:
    AggConnection(void);
    virtual ~AggConnection(void);
    result Construct(void);

    Osp::Base::String userLogin;
    Osp::Base::String userPassword;
    Osp::Base::String userNickname;

    result ConnectToRedirector(const Osp::Base::String& serverIp);
    result ConnectToDirectServer(const Osp::Base::String& serverIp, int port);

    void SendPacket(unsigned long command, Osp::Base::ByteBuffer& payload);

    void SetLoginListener(ILoginListener* pListener) { if (pAuthMgr) pAuthMgr->SetListener(pListener); }
    void SetContactListListener(IContactListListener* pListener) { if (pContactMgr) pContactMgr->SetListener(pListener); }
    void SetActiveChatListener(IMessageListener* pListener, const Osp::Base::String& email);
    void ClearActiveChatListener(void);
    void SendMessageTo(const Osp::Base::String& to, const Osp::Base::String& text) { if (pMessageMgr) pMessageMgr->SendMessageTo(to, text); }
    void SendNudge(const Osp::Base::String& to) { if (pMessageMgr) pMessageMgr->SendNudge(to); }
    void SendTyping(const Osp::Base::String& to) { if (pMessageMgr) pMessageMgr->SendTyping(to); }

    virtual void OnSocketConnected(Osp::Net::Sockets::Socket& socket);
    virtual void OnSocketClosed(Osp::Net::Sockets::Socket& socket, Osp::Net::Sockets::NetSocketClosedReason reason);
    virtual void OnSocketReadyToReceive(Osp::Net::Sockets::Socket& socket);
    virtual void OnSocketReadyToSend(Osp::Net::Sockets::Socket& socket);
    virtual void OnSocketAccept(Osp::Net::Sockets::Socket& socket);

    virtual void OnTimerExpired(Osp::Base::Runtime::Timer& timer);

private:
    result InitSocket(void);
    void SendHello(void);
    void SendPing(void);

    Osp::Net::Sockets::Socket* pSocket;
    Osp::Base::ByteBuffer* pTxBuffer;
    Osp::Base::ByteBuffer* pRxBuffer;

    Osp::Base::Runtime::Timer* pPingTimer;
    bool isRedirected;
    bool isTimerStarted;
    int pingIntervalMsec;

    MrimAuth* pAuthMgr;
    MrimContacts* pContactMgr;
    MrimMessages* pMessageMgr;
    MessageRouter* pMessageRouter;
};

#endif
