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

// Втрата/відновлення зв'язку - для індикатора на видимому екрані.
class IConnectionStateListener {
public:
    virtual ~IConnectionStateListener(void) {}
    virtual void OnConnectionStateChanged(bool connected) = 0;
};

// Сервер обірвав сеанс, бо з цим акаунтом увійшли з іншого пристрою
// (MRIM тримає лише один активний сеанс). Слухача ставить застосунок,
// а не окремий екран: вибити можуть з будь-якого місця.
class IForcedLogoutListener {
public:
    virtual ~IForcedLogoutListener(void) {}
    virtual void OnForcedLogout(void) = 0;
};

// З'єднання з MRIM-сервером: сокет, збірка/розбір пакетів, ping,
// автоматичне перепідключення. Форми звертаються сюди через фасадні
// методи й ніколи не працюють із сокетом напряму.
//
// Клас навмисно НЕ знає про форми: про події, що вимагають зміни
// екрана, він лише повідомляє слухачів.
class AggConnection : public Osp::Net::Sockets::ISocketEventListener,
                      public Osp::Base::Runtime::ITimerEventListener
{
public:
    AggConnection(void);
    virtual ~AggConnection(void);

    result Construct(void);

    // Повертає з'єднання у стан "щойно створене", але придатне до
    // повторного входу: гасить таймери, скидає слухачів, прапорці
    // сеансу й буфер прийому. Сокет лишається як є (див. реалізацію) -
    // його перестворить наступний ConnectToServer(). Дозволяє після
    // примусового виходу повернутись на форму входу тим самим
    // об'єктом, не створюючи новий.
    void Reset(void);

    void SetCredentials(const Osp::Base::String& login, const Osp::Base::String& password);
    Osp::Base::String GetLogin(void) const { return userLogin; }
    Osp::Base::String GetNickname(void) const { return userNickname; }
    void SetNickname(const Osp::Base::String& nickname) { userNickname = nickname; }

    result ConnectToServer(const Osp::Base::String& serverIp, int port);
    void SendPacket(unsigned long command, Osp::Base::ByteBuffer& payload);

    // --- Слухачі ---
    void SetLoginListener(ILoginListener* pListener) { if (pAuthMgr != null) pAuthMgr->SetListener(pListener); }
    void SetContactListListener(IContactListListener* pListener) { if (pContactMgr != null) pContactMgr->SetListener(pListener); }
    void SetConnectionStateListener(IConnectionStateListener* pListener) { pConnectionStateListener = pListener; }
    void SetForcedLogoutListener(IForcedLogoutListener* pListener) { pForcedLogoutListener = pListener; }

    void SetActiveChatListener(IMessageListener* pListener, const Osp::Base::String& email);
    void ClearActiveChatListener(void);
    void SetContactListVisibleListener(IUnreadCountListener* pListener);

    // --- Анкета (перегляд власного профілю) ---
    void SetProfileListener(IProfileListener* pListener) { if (pProfileMgr != null) pProfileMgr->SetListener(pListener); }
    void RequestOwnProfile(void) { if (pProfileMgr != null) pProfileMgr->RequestOwnProfile(userLogin); }

    // --- Надсилання ---
    void SendMessageTo(const Osp::Base::String& to, const Osp::Base::String& text) { if (pMessageMgr != null) pMessageMgr->SendMessageTo(to, text); }
    void SendNudge(const Osp::Base::String& to) { if (pMessageMgr != null) pMessageMgr->SendNudge(to); }
    void SendTyping(const Osp::Base::String& to) { if (pMessageMgr != null) pMessageMgr->SendTyping(to); }
    void ChangeStatus(unsigned long status);

    // --- Непрочитані та невідомі відправники ---
    int GetUnreadCount(const Osp::Base::String& email) const;
    void SetKnownContactEmails(Osp::Base::Collection::IList* pEmails);
    Osp::Base::Collection::IList* GetStrangerEmails(void) const;

    // Викликає MrimAuth після КОЖНОГО успішного входу - і першого, і
    // після автоматичного перепідключення.
    void NotifyLoggedIn(void);

    // AGG::OnForeground()/OnBackground(). У кожен момент активне лише
    // одне з'єднання, але хуки рівня Application не мають на нього
    // вказівника - тому "поточний" екземпляр відстежується статично.
    static void NotifyAppForegroundState(bool foreground);
    static AggConnection* GetActive(void) { return pActiveInstance; }

    // --- ISocketEventListener ---
    virtual void OnSocketConnected(Osp::Net::Sockets::Socket& socket);
    virtual void OnSocketClosed(Osp::Net::Sockets::Socket& socket, Osp::Net::Sockets::NetSocketClosedReason reason);
    virtual void OnSocketReadyToReceive(Osp::Net::Sockets::Socket& socket);
    virtual void OnSocketReadyToSend(Osp::Net::Sockets::Socket& socket);
    virtual void OnSocketAccept(Osp::Net::Sockets::Socket& socket);

    // --- ITimerEventListener ---
    virtual void OnTimerExpired(Osp::Base::Runtime::Timer& timer);

private:
    result InitSocket(void);
    void CloseSocket(void);
    void SendHello(void);
    void SendPing(void);

    void DispatchPacket(unsigned long command, Osp::Base::ByteBuffer& payload);
    void ProcessInbox(void);

    // Wi-Fi на Wave II зникає посеред роботи. Розрив ДО першого входу -
    // це помилка авторизації (без автоповтору), розрив уже залогіненого
    // сеансу - привід тихо перепідключитись із наростаючою паузою.
    void ScheduleReconnect(void);
    void AttemptReconnect(void);
    void StopTimers(void);

    Osp::Net::Sockets::Socket* pSocket;
    Osp::Base::ByteBuffer* pTxBuffer;
    Osp::Base::ByteBuffer* pRxBuffer;

    Osp::Base::Runtime::Timer* pPingTimer;
    Osp::Base::Runtime::Timer* pReconnectTimer;
    bool isPingTimerStarted;
    int pingIntervalMsec;

    Osp::Base::String userLogin;
    Osp::Base::String userPassword;
    Osp::Base::String userNickname;

    Osp::Base::String lastServerIp;
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
