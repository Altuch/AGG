#include "AggConnection.h"
#include "MRIM/MrimUtils.h"
#include "MessageRouter.h"
#include "Form1.h"
#include <FText.h>
#include <FUi.h>
#include <FApp.h>

using namespace Osp::Net;
using namespace Osp::Net::Sockets;
using namespace Osp::Base;
using namespace Osp::Base::Collection;
using namespace Osp::Base::Runtime;
using namespace Osp::Base::Utility;
using namespace Osp::Text;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;
using namespace Osp::App;

AggConnection* AggConnection::pActiveInstance = null;

AggConnection::AggConnection(void) :
    pSocket(null), pTxBuffer(null), pRxBuffer(null), pPingTimer(null),
    isRedirected(false), isTimerStarted(false), pingIntervalMsec(0),
    lastServerPort(0), hasLoggedInOnce(false), isReconnecting(false),
    isForceLoggedOut(false),
    reconnectAttempt(0), pReconnectTimer(null), pConnectionStateListener(null),
    pAuthMgr(null), pContactMgr(null), pMessageMgr(null), pMessageRouter(null)
{
    pAuthMgr = new MrimAuth(this);
    pContactMgr = new MrimContacts(this);
    pMessageMgr = new MrimMessages(this);
}

AggConnection::~AggConnection(void) {
    if (pActiveInstance == this) pActiveInstance = null;

    if (pSocket != null) {
        pSocket->RemoveSocketListener(*this);
        pSocket->Close();
        delete pSocket;
    }
    delete pTxBuffer;
    delete pRxBuffer;
    if (pPingTimer != null) {
        pPingTimer->Cancel();
        delete pPingTimer;
    }
    if (pReconnectTimer != null) {
        pReconnectTimer->Cancel();
        delete pReconnectTimer;
    }
    delete pAuthMgr;
    delete pContactMgr;
    delete pMessageMgr;
    delete pMessageRouter;
}

void AggConnection::NotifyAppForegroundState(bool foreground) {
    if (pActiveInstance == null || !pActiveInstance->hasLoggedInOnce) return;
    pActiveInstance->ChangeStatus(foreground ? 0x1 : 0x2);
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
    if (pMessageMgr != null) {
        pMessageMgr->SetListener(pMessageRouter);
    }

    return r;
}

void AggConnection::SetActiveChatListener(IMessageListener* pListener, const String& email) {
    if (pMessageRouter != null) pMessageRouter->SetActiveChat(pListener, email);
}

void AggConnection::ClearActiveChatListener(void) {
    if (pMessageRouter != null) pMessageRouter->ClearActiveChat();
}

int AggConnection::GetUnreadCount(const String& email) const {
    return pMessageRouter != null ? pMessageRouter->GetUnreadCount(email) : 0;
}

void AggConnection::SetContactListVisibleListener(IUnreadCountListener* pListener) {
    if (pMessageRouter != null) pMessageRouter->SetUnreadCountListener(pListener);
}

void AggConnection::SetKnownContactEmails(IList* pEmails) {
    if (pMessageRouter != null) pMessageRouter->SetKnownContacts(pEmails);
}

IList* AggConnection::GetStrangerEmails(void) const {
    return pMessageRouter != null ? pMessageRouter->GetStrangerEmails() : null;
}

void AggConnection::ChangeStatus(unsigned long status) {
    ByteBuffer payload;
    payload.Construct(16);
    MrimUtils::AppendUL(payload, status);
    payload.Flip();
    SendPacket(0x1022, payload);
}

result AggConnection::InitSocket(void) {
    if (pSocket != null) {
        pSocket->RemoveSocketListener(*this);
        pSocket->Close();
        delete pSocket;
        pSocket = null;
    }
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

result AggConnection::ConnectToRedirector(const String& serverIp) {
    isRedirected = false;
    result r = InitSocket();
    if (IsFailed(r)) return r;

    if (!MrimUtils::IsValidIpAddress(serverIp)) return E_INVALID_ARG;

    Ip4Address peerAddr(serverIp);
    NetEndPoint peerEndPoint(peerAddr, 2042);
    return pSocket->Connect(peerEndPoint);
}

result AggConnection::ConnectToDirectServer(const String& serverIp, int port) {
    isRedirected = true;
    lastServerIp = serverIp;
    lastServerPort = port;

    result r = InitSocket();
    if (IsFailed(r)) return r;

    if (!MrimUtils::IsValidIpAddress(serverIp)) return E_INVALID_ARG;

    Ip4Address peerAddr(serverIp);
    NetEndPoint peerEndPoint(peerAddr, port);
    return pSocket->Connect(peerEndPoint);
}

void AggConnection::OnSocketConnected(Socket& socket) {
    if (isRedirected) SendHello();
}

void AggConnection::OnSocketClosed(Socket& socket, NetSocketClosedReason reason) {
    if (isTimerStarted && pPingTimer != null) {
        pPingTimer->Cancel();
        isTimerStarted = false;
    }

    if (isForceLoggedOut) {
        return;
    }

    if (!hasLoggedInOnce) {
        if (isRedirected && pAuthMgr != null) {
            pAuthMgr->NotifyLoginFailed(L"З'єднання розірвано. Перевірте мережу.");
        }
        return;
    }

    bool wasAlreadyReconnecting = isReconnecting;
    isReconnecting = true;
    if (!wasAlreadyReconnecting && pConnectionStateListener != null) {
        pConnectionStateListener->OnConnectionStateChanged(false);
    }
    ScheduleReconnect();
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

void AggConnection::ScheduleReconnect(void) {
    if (pReconnectTimer == null) return;
    reconnectAttempt++;

    int delayMsec = 3000;
    if (reconnectAttempt == 2) delayMsec = 6000;
    else if (reconnectAttempt == 3) delayMsec = 12000;
    else if (reconnectAttempt >= 4) delayMsec = 20000;

    pReconnectTimer->Start(delayMsec);
}

void AggConnection::AttemptReconnect(void) {
    if (lastServerIp.IsEmpty()) return;
    AppLog("Спроба перепідключення #%d...", reconnectAttempt);
    ConnectToDirectServer(lastServerIp, lastServerPort);
}

void AggConnection::HandleForcedLogout(void) {
    isForceLoggedOut = true;

    Frame* pFrame = Application::GetInstance()->GetAppFrame()->GetFrame();
    if (pFrame == null) return;

    MessageBox msgBox;
    msgBox.Construct(
        L"Вихід із застосунку",
        L"Ви увійшли в цей акаунт з іншого пристрою. MRIM дозволяє лише один активний сеанс - увійдіть знову, коли будете готові.",
        MSGBOX_STYLE_OK);
    int modalResult = 0;
    msgBox.ShowAndWait(modalResult);

    Form* pOldForm = pFrame->GetCurrentForm();

    Form1* pForm1 = new Form1();
    pForm1->Initialize();

    pFrame->AddControl(*pForm1);
    pFrame->SetCurrentForm(*pForm1);
    pForm1->Draw();
    pForm1->Show();

    if (pOldForm != null) pFrame->RemoveControl(*pOldForm);
}

void AggConnection::OnSocketReadyToSend(Socket& socket) {}
void AggConnection::OnSocketAccept(Socket& socket) {}

void AggConnection::SendPacket(unsigned long command, ByteBuffer& payload) {
    if (pSocket == null) return;
    pTxBuffer->Clear();
    MrimUtils::BuildHeader(*pTxBuffer, command, payload.GetLimit());
    if (payload.GetLimit() > 0) {
        pTxBuffer->SetArray(payload.GetPointer(), 0, payload.GetLimit());
    }
    pTxBuffer->Flip();
    pSocket->Send(*pTxBuffer);
}

void AggConnection::SendHello(void) {
    ByteBuffer emptyPayload;
    emptyPayload.Construct(1);
    emptyPayload.SetLimit(0);
    emptyPayload.Flip();
    SendPacket(0x1001, emptyPayload);
}

void AggConnection::SendPing(void) {
    ByteBuffer emptyPayload;
    emptyPayload.Construct(1);
    emptyPayload.SetLimit(0);
    emptyPayload.Flip();
    SendPacket(0x1006, emptyPayload);
}

void AggConnection::OnTimerExpired(Timer& timer) {
    if (&timer == pReconnectTimer) {
        AttemptReconnect();
        return;
    }
    if (isTimerStarted) {
        SendPing();
        pPingTimer->Start(pingIntervalMsec);
    }
}

void AggConnection::OnSocketReadyToReceive(Socket& socket) {
    unsigned long buflen = 0;

    socket.Ioctl(NET_SOCKET_FIONREAD, buflen);
    if (buflen == 0) return;

    ByteBuffer tempBuf;
    tempBuf.Construct(buflen + 1);

    result r = socket.Receive(tempBuf);
    if (IsFailed(r) || tempBuf.GetPosition() == 0) return;

    tempBuf.Flip();

    pRxBuffer->SetArray(tempBuf.GetPointer(), 0, tempBuf.GetLimit());
    pRxBuffer->Flip();

    if (!isRedirected) {
        String redirectAddress;
        AsciiEncoding().GetString(*pRxBuffer, redirectAddress);
        redirectAddress.Trim();

        StringTokenizer strTok(redirectAddress, L":");
        if (strTok.GetTokenCount() >= 2) {
            String newIp, newPortStr;
            strTok.GetNextToken(newIp);
            strTok.GetNextToken(newPortStr);

            int newPort = 2041;
            Integer::Parse(newPortStr, newPort);
            ConnectToDirectServer(newIp, newPort);
        }
    } else {
        while (pRxBuffer->GetRemaining() >= 44) {
            int packetStartPos = pRxBuffer->GetPosition();
            unsigned long magic = MrimUtils::ReadUL(*pRxBuffer);
            if (magic != 0xDEADBEEF) {
                pRxBuffer->SetPosition(packetStartPos + 1);
                continue;
            }

            pRxBuffer->SetPosition(packetStartPos + 12);
            unsigned long command = MrimUtils::ReadUL(*pRxBuffer);
            unsigned long dataLen = MrimUtils::ReadUL(*pRxBuffer);

            if (static_cast<unsigned long>(pRxBuffer->GetRemaining()) < 24 + dataLen) {
                pRxBuffer->SetPosition(packetStartPos);
                break;
            }

            pRxBuffer->SetPosition(packetStartPos + 44);
            ByteBuffer payload;
            payload.Construct(dataLen > 0 ? dataLen : 1);

            if (dataLen > 0) {
                byte* temp = new byte[dataLen];
                pRxBuffer->GetArray(temp, 0, dataLen);
                payload.SetArray(temp, 0, dataLen);
                delete[] temp;
            }
            payload.SetPosition(0);
            payload.SetLimit(dataLen);

            bool handled = false;
            if (pAuthMgr) handled = pAuthMgr->ProcessCommand(command, payload);
            if (!handled && pContactMgr) {
                payload.SetPosition(0);
                handled = pContactMgr->ProcessCommand(command, payload);
            }
            if (!handled && pMessageMgr) {
                payload.SetPosition(0);
                handled = pMessageMgr->ProcessCommand(command, payload);
            }

            if (!handled && command == 0x1002) { // HELLO_ACK
                payload.SetPosition(0);
                pingIntervalMsec = MrimUtils::ReadUL(payload) * 1000;
                if (pPingTimer) {
                    if (isTimerStarted) pPingTimer->Cancel();
                    pPingTimer->Start(pingIntervalMsec);
                    isTimerStarted = true;
                }
                if (pAuthMgr) pAuthMgr->SendLogin2(userLogin, userPassword);
            }

            if (!handled && command == 0x1013) {
                handled = true;
                HandleForcedLogout();
            }

            pRxBuffer->SetPosition((packetStartPos + 44) + dataLen);
        }
    }
    pRxBuffer->Compact();
}
