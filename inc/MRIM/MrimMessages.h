#ifndef _MRIM_MESSAGES_H_
#define _MRIM_MESSAGES_H_

#include <FBase.h>

class AggConnection;

// Отримувач вхідних подій листування. Реалізує MessageRouter (постійно)
// і ChatForm (через router - лише для свого контакту).
class IMessageListener {
public:
    virtual ~IMessageListener(void) {}
    virtual void OnMessageReceived(const Osp::Base::String& sender, const Osp::Base::String& text, bool isNudge) = 0;
    virtual void OnMessageDeliveryStatus(unsigned long status) = 0;
    virtual void OnTypingReceived(const Osp::Base::String& sender) = 0;
};

// Листування: надсилання, розбір вхідних, офлайн-повідомлення.
// Коди команд і прапорці - у MRIM/MrimProtocol.h.
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
    // Підтвердження отримання: без нього сервер вважає повідомлення
    // недоставленим.
    void SendMessageRecv(const Osp::Base::String& from, unsigned long msgId);

    // Сервер чистить чергу офлайн-повідомлень цілком, щойно клієнт
    // надішле цю команду (конкретний id він ігнорує), тож викликати її
    // після кожного отриманого повідомлення безпечно.
    void SendOfflineMessageDelete(void);

    // Офлайн-повідомлення приходить не як звичайне, а "конвертом" на
    // кшталт листа: заголовки From/Date/Content-Type, порожній рядок,
    // далі тіло в base64(UTF-16LE).
    void HandleOfflineMessageEnvelope(const Osp::Base::String& envelope);

    AggConnection* pConnection;
    IMessageListener* pListener;
};

#endif
