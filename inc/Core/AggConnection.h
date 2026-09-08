#ifndef _AGG_CONNECTION_H_
#define _AGG_CONNECTION_H_

#include <FNet.h>
#include <FBase.h>

#include "MRIM/MrimAuth.h"
#include "MRIM/MrimContacts.h"
#include "MRIM/MrimMessages.h"
#include "MRIM/MrimProfile.h"

class MessageRouter;
class IUnreadCountListener;

class IConnectionStateListener {
public:
    virtual ~IConnectionStateListener(void) {}
    virtual void OnConnectionStateChanged(bool connected) = 0;
};

class IForcedLogoutListener {
public:
    virtual ~IForcedLogoutListener(void) {}
    virtual void OnForcedLogout(void) = 0;
};

class AggConnection : public Osp::Net::Sockets::ISocketEventListener,
                      public Osp::Base::Runtime::ITimerEventListener,
                      public Osp::Net::IDnsEventListener
{
public:
    AggConnection(void);
    virtual ~AggConnection(void);

    result Construct(void);

    void Reset(void);

    void SetCredentials(const Osp::Base::String& login, const Osp::Base::String& password);
    Osp::Base::String GetLogin(void) const { return userLogin; }
    Osp::Base::String GetNickname(void) const { return userNickname; }
    void SetNickname(const Osp::Base::String& nickname) { userNickname = nickname; }

    result ConnectToServer(const Osp::Base::String& serverHost, int port);
    void SendPacket(unsigned long command, Osp::Base::ByteBuffer& payload);

    void SetLoginListener(ILoginListener* pListener) { if (pAuthMgr != null) pAuthMgr->SetListener(pListener); }
    void SetContactListListener(IContactListListener* pListener) { if (pContactMgr != null) pContactMgr->SetListener(pListener); }
    void SetConnectionStateListener(IConnectionStateListener* pListener) { pConnectionStateListener = pListener; }
    void SetForcedLogoutListener(IForcedLogoutListener* pListener) { pForcedLogoutListener = pListener; }

    void SetActiveChatListener(IMessageListener* pListener, const Osp::Base::String& email);
    void ClearActiveChatListener(void);
    void SetContactListVisibleListener(IUnreadCountListener* pListener);

    void SetProfileListener(IProfileListener* pListener) { if (pProfileMgr != null) pProfileMgr->SetListener(pListener); }
    void RequestProfile(const Osp::Base::String& login) { if (pProfileMgr != null) pProfileMgr->RequestProfileFor(login.IsEmpty() ? userLogin : login); }

    void SendMessageTo(const Osp::Base::String& to, const Osp::Base::String& text) { if (pMessageMgr != null) pMessageMgr->SendMessageTo(to, text); }
    void SendNudge(const Osp::Base::String& to) { if (pMessageMgr != null) pMessageMgr->SendNudge(to); }
    void SendTyping(const Osp::Base::String& to) { if (pMessageMgr != null) pMessageMgr->SendTyping(to); }
    void ChangeStatus(unsigned long status);

    int GetUnreadCount(const Osp::Base::String& email) const;
    void SetKnownContactEmails(Osp::Base::Collection::IList* pEmails);
    Osp::Base::Collection::IList* GetStrangerEmails(void) const;

    void NotifyLoggedIn(void);

    static void NotifyAppForegroundState(bool foreground);
    static AggConnection* GetActive(void) { return pActiveInstance; }

    virtual void OnSocketConnected(Osp::Net::Sockets::Socket& socket);
    virtual void OnSocketClosed(Osp::Net::Sockets::Socket& socket, Osp::Net::Sockets::NetSocketClosedReason reason);
    virtual void OnSocketReadyToReceive(Osp::Net::Sockets::Socket& socket);
    virtual void OnSocketReadyToSend(Osp::Net::Sockets::Socket& socket);
    virtual void OnSocketAccept(Osp::Net::Sockets::Socket& socket);

    virtual void OnTimerExpired(Osp::Base::Runtime::Timer& timer);

    virtual void OnDnsResolutionCompletedN(Osp::Net::IpHostEntry* pIpHostEntry, result r);

private:
    result InitSocket(void);
    void CloseSocket(void);
    void SendHello(void);
    result ConnectToAddress(const Osp::Net::IpAddress& address, int port);
    void SendPing(void);

    void DispatchPacket(unsigned long command, Osp::Base::ByteBuffer& payload);
    void ProcessInbox(void);

    void ScheduleReconnect(void);
    void AttemptReconnect(void);
    void StopTimers(void);

    Osp::Net::Sockets::Socket* pSocket;
    Osp::Net::Dns* pDns;
    Osp::Base::ByteBuffer* pTxBuffer;
    Osp::Base::ByteBuffer* pRxBuffer;

    Osp::Base::Runtime::Timer* pPingTimer;
    Osp::Base::Runtime::Timer* pReconnectTimer;
    bool isPingTimerStarted;
    int pingIntervalMsec;

    Osp::Base::String userLogin;
    Osp::Base::String userPassword;
    Osp::Base::String userNickname;

    Osp::Base::String lastServerHost;
    int lastServerPort;

    bool hasLoggedInOnce;
    bool isReconnecting;
    bool isForceLoggedOut;
    int reconnectAttempt;

    IConnectionStateListener* pConnectionStateListener;
    IForcedLogoutListener* pForcedLogoutListener;

    MrimAuth* pAuthMgr;
    MrimContacts* pContactMgr;
    MrimMessages* pMessageMgr;
    MrimProfile* pProfileMgr;
    MessageRouter* pMessageRouter;

    static AggConnection* pActiveInstance;
};

#endif
