#include "AggConnection.h"
#include "MRIM/MrimUtils.h"
#include <FText.h>

using namespace Osp::Net;
using namespace Osp::Net::Sockets;
using namespace Osp::Base;
using namespace Osp::Base::Runtime;
using namespace Osp::Base::Utility;
using namespace Osp::Text;

AggConnection::AggConnection(void) :
    pSocket(null), pTxBuffer(null), pRxBuffer(null), pPingTimer(null),
    isRedirected(false), isTimerStarted(false), pingIntervalMsec(0),
    pAuthMgr(null), pContactMgr(null), pMessageMgr(null)
{
    pAuthMgr = new MrimAuth(this);
    pContactMgr = new MrimContacts(this);
    pMessageMgr = new MrimMessages(this);
}

AggConnection::~AggConnection(void) {
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
    delete pAuthMgr;
    delete pContactMgr;
    delete pMessageMgr;
}

result AggConnection::Construct(void) {
    pTxBuffer = new ByteBuffer();
    pTxBuffer->Construct(4096);

    pRxBuffer = new ByteBuffer();
    pRxBuffer->Construct(8192);

    result r = InitSocket();
    if (IsFailed(r)) return r;

    pPingTimer = new Timer();
    pPingTimer->Construct(*this);
    return r;
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
    if (isRedirected && pAuthMgr != null) {
        pAuthMgr->NotifyLoginFailed(L"З'єднання розірвано. Перевірте мережу.");
    }
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
    if (isTimerStarted) {
        SendPing();
        pPingTimer->Start(pingIntervalMsec);
    }
}

void AggConnection::OnSocketReadyToReceive(Socket& socket) {
    unsigned long buflen = 0;

    // Блискуча знахідка з документації bada: дізнаємося точний розмір даних!
    socket.Ioctl(NET_SOCKET_FIONREAD, buflen);
    if (buflen == 0) return;

    ByteBuffer tempBuf;
    tempBuf.Construct(buflen + 1);

    result r = socket.Receive(tempBuf);
    if (IsFailed(r) || tempBuf.GetPosition() == 0) return;

    tempBuf.Flip();

    // Правильно дописуємо дані в кінець (TCP фрагментація)
    pRxBuffer->SetArray(tempBuf.GetPointer(), 0, tempBuf.GetLimit());
    pRxBuffer->Flip(); // Готуємо буфер до читання

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

            // Перевіряємо, чи отримано весь пакет повністю
            if (static_cast<unsigned long>(pRxBuffer->GetRemaining()) < 24 + dataLen) {
                pRxBuffer->SetPosition(packetStartPos);
                break; // Чекаємо наступного OnSocketReadyToReceive
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

            // Переходимо до наступного пакета
            pRxBuffer->SetPosition((packetStartPos + 44) + dataLen);
        }
    }

    // ВАЖЛИВО: Зсуває недочитані (фрагментовані) дані на початок для склеювання
    pRxBuffer->Compact();
}
