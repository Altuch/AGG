#include "Core/AggConnection.h"
#include "Core/MessageRouter.h"
#include "MRIM/MrimProtocol.h"
#include "MRIM/MrimUtils.h"

using namespace Osp::Base;
using namespace Osp::Base::Collection;
using namespace Osp::Base::Runtime;
using namespace Osp::Net;
using namespace Osp::Net::Sockets;

AggConnection* AggConnection::pActiveInstance = null;

static const int RECONNECT_DELAYS_MSEC[] = { 3000, 6000, 12000, 20000 };
static const int RECONNECT_DELAY_COUNT = 4;

AggConnection::AggConnection(void) :
    pSocket(null), pDns(null), pTxBuffer(null), pRxBuffer(null),
    pPingTimer(null), pReconnectTimer(null),
    isPingTimerStarted(false), pingIntervalMsec(0),
    lastServerPort(0),
    hasLoggedInOnce(false), isReconnecting(false), isForceLoggedOut(false),
    reconnectAttempt(0),
    pConnectionStateListener(null), pForcedLogoutListener(null),
    pAuthMgr(null), pContactMgr(null), pMessageMgr(null), pProfileMgr(null), pMessageRouter(null)
{
    pAuthMgr = new MrimAuth(this);
    pContactMgr = new MrimContacts(this);
    pMessageMgr = new MrimMessages(this);
    pProfileMgr = new MrimProfile(this);
}

AggConnection::~AggConnection(void) {
    if (pActiveInstance == this) pActiveInstance = null;

    StopTimers();
    CloseSocket();
    delete pDns;

    delete pTxBuffer;
    delete pRxBuffer;
    delete pPingTimer;
    delete pReconnectTimer;
    delete pAuthMgr;
    delete pContactMgr;
    delete pMessageMgr;
    delete pProfileMgr;
    delete pMessageRouter;
}

result AggConnection::Construct(void) {
    pActiveInstance = this;

    pTxBuffer = new ByteBuffer();
    pTxBuffer->Construct(4096);

    pRxBuffer = new ByteBuffer();
    pRxBuffer->Construct(8192);

    result r = InitSocket();
    if (IsFailed(r)) return r;

    pPingTimer = new Timer();
    pPingTimer->Construct(*this);

    pReconnectTimer = new Timer();
    pReconnectTimer->Construct(*this);

    pMessageRouter = new MessageRouter();
    if (pMessageMgr != null) pMessageMgr->SetListener(pMessageRouter);

    return r;
}

void AggConnection::Reset(void) {
    StopTimers();

    SetLoginListener(null);
    SetContactListListener(null);
    SetConnectionStateListener(null);
    SetContactListVisibleListener(null);
    ClearActiveChatListener();

    hasLoggedInOnce = false;
    isReconnecting = false;
    reconnectAttempt = 0;
    pingIntervalMsec = 0;

    if (pRxBuffer != null) pRxBuffer->Clear();
}

void AggConnection::SetCredentials(const String& login, const String& password) {
    userLogin = login;
    userPassword = password;
}

void AggConnection::SetActiveChatListener(IMessageListener* pListener, const String& email) {
    if (pMessageRouter != null) pMessageRouter->SetActiveChat(pListener, email);
}

void AggConnection::ClearActiveChatListener(void) {
    if (pMessageRouter != null) pMessageRouter->ClearActiveChat();
}

void AggConnection::SetContactListVisibleListener(IUnreadCountListener* pListener) {
    if (pMessageRouter != null) pMessageRouter->SetUnreadCountListener(pListener);
}

int AggConnection::GetUnreadCount(const String& email) const {
    return (pMessageRouter != null) ? pMessageRouter->GetUnreadCount(email) : 0;
}

void AggConnection::SetKnownContactEmails(IList* pEmails) {
    if (pMessageRouter != null) pMessageRouter->SetKnownContacts(pEmails);
}

IList* AggConnection::GetStrangerEmails(void) const {
    return (pMessageRouter != null) ? pMessageRouter->GetStrangerEmails() : null;
}

void AggConnection::CloseSocket(void) {
    if (pSocket == null) return;

    pSocket->RemoveSocketListener(*this);
    pSocket->Close();
    delete pSocket;
    pSocket = null;
}

result AggConnection::InitSocket(void) {
    CloseSocket();

    pSocket = new Socket();
    result r = pSocket->Construct(NET_SOCKET_AF_IPV4, NET_SOCKET_TYPE_STREAM, NET_SOCKET_PROTOCOL_TCP);
    if (IsFailed(r)) return r;

    pSocket->AddSocketListener(*this);
    pSocket->AsyncSelectByListener(NET_SOCKET_EVENT_CONNECT |
                                   NET_SOCKET_EVENT_READ |
                                   NET_SOCKET_EVENT_WRITE |
                                   NET_SOCKET_EVENT_CLOSE);
    return r;
}

result AggConnection::ConnectToServer(const String& serverHost, int port) {
    if (!MrimUtils::IsValidHost(serverHost)) return E_INVALID_ARG;

    lastServerHost = serverHost;
    lastServerPort = port;

    isForceLoggedOut = false;

    if (MrimUtils::IsValidIpAddress(serverHost)) {
        Ip4Address peerAddr(serverHost);
        return ConnectToAddress(peerAddr, port);
    }

    delete pDns;
    pDns = new Dns();
    result r = pDns->Construct(*this);
    if (IsFailed(r)) return r;

    return pDns->GetHostByName(serverHost);
}

result AggConnection::ConnectToAddress(const IpAddress& address, int port) {
    result r = InitSocket();
    if (IsFailed(r)) return r;

    NetEndPoint peerEndPoint(address, (unsigned short)port);
    return pSocket->Connect(peerEndPoint);
}

void AggConnection::OnDnsResolutionCompletedN(IpHostEntry* pIpHostEntry, result r) {
    if (IsFailed(r) || pIpHostEntry == null) {
        delete pIpHostEntry;
        AppLog("DNS: не вдалося визначити адресу '%S'.", lastServerHost.GetPointer());
        if (pAuthMgr != null) pAuthMgr->NotifyLoginFailed(L"Не вдалося визначити адресу сервера.");
        return;
    }

    IList* pAddresses = pIpHostEntry->GetAddressList();
    if (pAddresses == null || pAddresses->GetCount() == 0) {
        delete pIpHostEntry;
        AppLog("DNS: жодної адреси для '%S'.", lastServerHost.GetPointer());
        if (pAuthMgr != null) pAuthMgr->NotifyLoginFailed(L"DNS не повернув жодної адреси сервера.");
        return;
    }

    IpAddress* pAddr = static_cast<IpAddress*>(pAddresses->GetAt(0));
    if (pAddr != null) {
        AppLog("DNS: '%S' -> %S", lastServerHost.GetPointer(), pAddr->ToString().GetPointer());
        ConnectToAddress(*pAddr, lastServerPort);
    }

    delete pIpHostEntry;
}

void AggConnection::SendPacket(unsigned long command, ByteBuffer& payload) {
    if (pSocket == null || pTxBuffer == null) return;

    pTxBuffer->Clear();
    MrimUtils::BuildHeader(*pTxBuffer, command, payload.GetLimit());
    if (payload.GetLimit() > 0) {
        pTxBuffer->SetArray(payload.GetPointer(), 0, payload.GetLimit());
    }
    pTxBuffer->Flip();

    pSocket->Send(*pTxBuffer);
}

void AggConnection::SendHello(void) {
    ByteBuffer empty;
    empty.Construct(1);
    empty.SetLimit(0);
    empty.Flip();
    SendPacket(Mrim::Cmd::HELLO, empty);
}

void AggConnection::SendPing(void) {
    ByteBuffer empty;
    empty.Construct(1);
    empty.SetLimit(0);
    empty.Flip();
    SendPacket(Mrim::Cmd::PING, empty);
}

void AggConnection::ChangeStatus(unsigned long status) {
    ByteBuffer payload;
    payload.Construct(16);
    MrimUtils::AppendUL(payload, status);
    payload.Flip();
    SendPacket(Mrim::Cmd::CHANGE_STATUS, payload);
}

void AggConnection::OnSocketConnected(Socket& socket) {
    SendHello();
}

void AggConnection::OnSocketClosed(Socket& socket, NetSocketClosedReason reason) {
    StopTimers();

    if (isForceLoggedOut) {
        return;
    }

    if (!hasLoggedInOnce) {
        if (pAuthMgr != null) {
            pAuthMgr->NotifyLoginFailed(L"З'єднання розірвано. Перевірте мережу.");
        }
        return;
    }

    bool wasReconnecting = isReconnecting;
    isReconnecting = true;
    if (!wasReconnecting && pConnectionStateListener != null) {
        pConnectionStateListener->OnConnectionStateChanged(false);
    }
    ScheduleReconnect();
}

void AggConnection::OnSocketReadyToSend(Socket& socket) {}
void AggConnection::OnSocketAccept(Socket& socket) {}

void AggConnection::OnSocketReadyToReceive(Socket& socket) {
    if (pRxBuffer == null) return;

    unsigned long available = 0;
    socket.Ioctl(NET_SOCKET_FIONREAD, available);
    if (available == 0) return;

    ByteBuffer chunk;
    chunk.Construct(available + 1);

    if (IsFailed(socket.Receive(chunk)) || chunk.GetPosition() == 0) return;
    chunk.Flip();

    pRxBuffer->SetArray(chunk.GetPointer(), 0, chunk.GetLimit());
    pRxBuffer->Flip();

    ProcessInbox();

    pRxBuffer->Compact();
}

void AggConnection::ProcessInbox(void) {
    while (pRxBuffer->GetRemaining() >= Mrim::HEADER_SIZE) {
        int packetStart = pRxBuffer->GetPosition();

        if (MrimUtils::ReadUL(*pRxBuffer) != Mrim::MAGIC) {
            pRxBuffer->SetPosition(packetStart + 1);
            continue;
        }

        pRxBuffer->SetPosition(packetStart + 12);
        unsigned long command = MrimUtils::ReadUL(*pRxBuffer);
        unsigned long dataLen = MrimUtils::ReadUL(*pRxBuffer);

        if (static_cast<unsigned long>(pRxBuffer->GetRemaining()) < 24 + dataLen) {
            pRxBuffer->SetPosition(packetStart);
            break;
        }

        pRxBuffer->SetPosition(packetStart + Mrim::HEADER_SIZE);

        ByteBuffer payload;
        payload.Construct(dataLen > 0 ? dataLen : 1);
        if (dataLen > 0) {
            byte* pTemp = new byte[dataLen];
            pRxBuffer->GetArray(pTemp, 0, dataLen);
            payload.SetArray(pTemp, 0, dataLen);
            delete[] pTemp;
        }
        payload.SetPosition(0);
        payload.SetLimit(dataLen);

        DispatchPacket(command, payload);

        pRxBuffer->SetPosition(packetStart + Mrim::HEADER_SIZE + dataLen);
    }
}

void AggConnection::DispatchPacket(unsigned long command, ByteBuffer& payload) {
    if (pAuthMgr != null && pAuthMgr->ProcessCommand(command, payload)) return;

    payload.SetPosition(0);
    if (pContactMgr != null && pContactMgr->ProcessCommand(command, payload)) return;

    payload.SetPosition(0);
    if (pMessageMgr != null && pMessageMgr->ProcessCommand(command, payload)) return;

    payload.SetPosition(0);
    if (pProfileMgr != null && pProfileMgr->ProcessCommand(command, payload)) return;

    payload.SetPosition(0);
    switch (command) {
        case Mrim::Cmd::HELLO_ACK: {
            pingIntervalMsec = MrimUtils::ReadUL(payload) * 1000;
            if (pPingTimer != null && pingIntervalMsec > 0) {
                if (isPingTimerStarted) pPingTimer->Cancel();
                pPingTimer->Start(pingIntervalMsec);
                isPingTimerStarted = true;
            }
            if (pAuthMgr != null) pAuthMgr->SendLogin2(userLogin, userPassword);
            break;
        }

        case Mrim::Cmd::LOGOUT: {
            isForceLoggedOut = true;
            StopTimers();
            if (pForcedLogoutListener != null) pForcedLogoutListener->OnForcedLogout();
            break;
        }

        default:
            break;
    }
}

void AggConnection::NotifyLoggedIn(void) {
    hasLoggedInOnce = true;

    bool wasReconnecting = isReconnecting;
    isReconnecting = false;
    reconnectAttempt = 0;

    if (wasReconnecting && pConnectionStateListener != null) {
        pConnectionStateListener->OnConnectionStateChanged(true);
    }
}

void AggConnection::NotifyAppForegroundState(bool foreground) {
    if (pActiveInstance == null || !pActiveInstance->hasLoggedInOnce) return;
    pActiveInstance->ChangeStatus(foreground ? Mrim::Status::ONLINE : Mrim::Status::AWAY);
}

void AggConnection::StopTimers(void) {
    if (pPingTimer != null && isPingTimerStarted) pPingTimer->Cancel();
    isPingTimerStarted = false;
    if (pReconnectTimer != null) pReconnectTimer->Cancel();
}

void AggConnection::ScheduleReconnect(void) {
    if (pReconnectTimer == null) return;

    int index = reconnectAttempt;
    if (index >= RECONNECT_DELAY_COUNT) index = RECONNECT_DELAY_COUNT - 1;
    reconnectAttempt++;

    pReconnectTimer->Start(RECONNECT_DELAYS_MSEC[index]);
}

void AggConnection::AttemptReconnect(void) {
    if (lastServerHost.IsEmpty()) return;
    AppLog("Спроба перепідключення #%d...", reconnectAttempt);
    ConnectToServer(lastServerHost, lastServerPort);
}

void AggConnection::OnTimerExpired(Timer& timer) {
    if (&timer == pReconnectTimer) {
        AttemptReconnect();
        return;
    }

    if (isPingTimerStarted && pPingTimer != null) {
        SendPing();
        pPingTimer->Start(pingIntervalMsec);
    }
}
