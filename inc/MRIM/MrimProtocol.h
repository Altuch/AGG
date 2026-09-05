#ifndef _MRIM_PROTOCOL_H_
#define _MRIM_PROTOCOL_H_

// Єдине місце для всіх констант протоколу MRIM.
//
// Значення звірені з реальним сервером mrimsu/mrim-server
// (src/servers/mrim/globals.js) та документацією mrimsu/mrim-docs.
// Раніше ці числа були розкидані шістнадцятковими літералами по
// шести файлах - через це, наприклад, коди статусів дублювались у
// трьох місцях і легко розходились.

namespace Mrim {

// --- Заголовок пакета (44 байти) ---
static const unsigned long MAGIC        = 0xDEADBEEF;
static const int           HEADER_SIZE  = 44;

// AGG заявляє протокол 1.8. Це НЕ косметика: від версії залежить
// формат даних на дроті - для <= 1.15 рядки в CP1251 і короткі
// записи контактів, UTF-16LE з'являється лише з 1.16.
static const unsigned long PROTO_VERSION = 0x00010008; // 1.8

// --- Команди клієнт -> сервер ---
namespace Cmd {
    static const unsigned long HELLO                  = 0x1001;
    static const unsigned long HELLO_ACK              = 0x1002;
    static const unsigned long LOGIN_ACK              = 0x1004;
    static const unsigned long LOGIN_REJ              = 0x1005;
    static const unsigned long PING                   = 0x1006;
    static const unsigned long MESSAGE                = 0x1008;
    static const unsigned long MESSAGE_ACK            = 0x1009;
    static const unsigned long USER_STATUS            = 0x100F;
    static const unsigned long MESSAGE_RECV           = 0x1011;
    static const unsigned long MESSAGE_STATUS         = 0x1012;
    static const unsigned long LOGOUT                 = 0x1013;
    static const unsigned long USER_INFO              = 0x1015;
    static const unsigned long OFFLINE_MESSAGE_ACK    = 0x101D;
    static const unsigned long OFFLINE_MESSAGE_DELETE = 0x101E;
    static const unsigned long CHANGE_STATUS          = 0x1022;
    static const unsigned long CONTACT_LIST2          = 0x1037;
    static const unsigned long LOGIN2                 = 0x1038;
    static const unsigned long ANKETA_INFO            = 0x1028; // S->C: результат пошуку/профіль
    static const unsigned long WP_REQUEST             = 0x1029; // C->S: пошук за анкетою
}

// Пошук анкети (MRIM_CS_WP_REQUEST) і відповідь (MRIM_CS_ANKETA_INFO).
// Звірено з mrim-server (processors/anketa.js): сервер не вміє
// РЕДАГУВАТИ ці поля - лише шукати/повертати, тож профіль у AGG
// показуємо, а не редагуємо.
namespace Anketa {
    // Поле пошуку - лише ідентифікатор власного логіна нам і треба.
    namespace SearchField {
        static const unsigned long USER   = 0;
        static const unsigned long DOMAIN = 1;
    }
    namespace Status {
        static const unsigned long NO_USER  = 0x0;
        static const unsigned long OK       = 0x1;
        static const unsigned long DB_ERROR = 0x2;
        static const unsigned long LIMIT    = 0x3; // забагато запитів
    }
}

// --- Статуси користувача ---
// 0x4 (xstatus) сервер приймає лише з протоколу 1.15+, тож AGG його
// не надсилає - лише розпізнає у чужих статусах.
namespace Status {
    static const unsigned long OFFLINE   = 0x00000000;
    static const unsigned long ONLINE    = 0x00000001;
    static const unsigned long AWAY      = 0x00000002;
    static const unsigned long XSTATUS   = 0x00000004;
    static const unsigned long INVISIBLE = 0x80000001;
}

// --- Прапорці повідомлень ---
namespace MsgFlag {
    static const unsigned long OFFLINE   = 0x00000001;
    static const unsigned long NORECV    = 0x00000004;
    static const unsigned long AUTHORIZE = 0x00000008;
    static const unsigned long SYSTEM    = 0x00000040;
    static const unsigned long RTF       = 0x00000080;
    static const unsigned long CONTACT   = 0x00000200;
    static const unsigned long TYPING    = 0x00000400; // у сервера зветься NOTIFY
    static const unsigned long MULTICAST = 0x00001000;
    static const unsigned long ALARM     = 0x00004000; // у сервера зветься WAKEUP
    static const unsigned long FLASH     = 0x00008000;
}

// --- Коди доставки (MRIM_CS_MESSAGE_STATUS) ---
namespace Delivery {
    static const unsigned long SUCCESS          = 0x0000;
    static const unsigned long NO_USER          = 0x8001;
    static const unsigned long INTERNAL_ERROR   = 0x8003;
    static const unsigned long OFFLINE_LIMIT    = 0x8004;
    static const unsigned long TOO_LARGE        = 0x8005;
    static const unsigned long OFFLINE_DISABLED = 0x8006;
}

// --- Прапорці записів списку контактів ---
namespace ContactFlag {
    static const unsigned long UNICODE_NICKNAME = 0x00000200;
}

} // namespace Mrim

#endif
