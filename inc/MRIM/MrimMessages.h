#ifndef _MRIM_MESSAGES_H_
#define _MRIM_MESSAGES_H_

#include <FBase.h>

#define MRIM_MSG_FLAG_OFFLINE   0x00000001
#define MRIM_MSG_FLAG_NORECV    0x00000004
#define MRIM_MSG_FLAG_AUTHORIZE 0x00000008
#define MRIM_MSG_FLAG_SYSTEM    0x00000040
#define MRIM_MSG_FLAG_RTF       0x00000080
#define MRIM_MSG_FLAG_CONTACT   0x00000200
#define MRIM_MSG_FLAG_TYPING    0x00000400
#define MRIM_MSG_FLAG_MULTICAST 0x00001000
#define MRIM_MSG_FLAG_ALARM     0x00004000
#define MRIM_MSG_FLAG_FLASH     0x00008000

class AggConnection;

class IMessageListener {
public:
    virtual ~IMessageListener(void) {}
    virtual void OnMessageReceived(const Osp::Base::String& sender, const Osp::Base::String& text, bool isNudge) = 0;
    virtual void OnMessageDeliveryStatus(unsigned long status) = 0;
    virtual void OnTypingReceived(const Osp::Base::String& sender) = 0;
};

class MrimMessages {
public:
    MrimMessages(AggConnection* pConn);
    virtual ~MrimMessages(void);

    void SetListener(IMessageListener* pListener);

    void SendMessageTo(const Osp::Base::String& to, const Osp::Base::String& text);
    void SendNudge(const Osp::Base::String& to);
    void SendTyping(const Osp::Base::String& to);
    void SendMessageRecv(const Osp::Base::String& from, unsigned long msgId);

    bool ProcessCommand(unsigned long command, Osp::Base::ByteBuffer& payload);

private:
    AggConnection* pConnection;
    IMessageListener* pListener;
};

#endif
