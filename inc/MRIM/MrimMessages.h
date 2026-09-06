#ifndef _MRIM_MESSAGES_H_
#define _MRIM_MESSAGES_H_

#include <FBase.h>

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

    bool ProcessCommand(unsigned long command, Osp::Base::ByteBuffer& payload);

private:
    void SendMessageRecv(const Osp::Base::String& from, unsigned long msgId);

    void SendOfflineMessageDelete(void);

    void HandleOfflineMessageEnvelope(const Osp::Base::String& envelope);

    AggConnection* pConnection;
    IMessageListener* pListener;
};

#endif
