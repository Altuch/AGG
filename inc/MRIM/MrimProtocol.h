#ifndef _MRIM_PROTOCOL_H_
#define _MRIM_PROTOCOL_H_

namespace Mrim {

static const unsigned long MAGIC        = 0xDEADBEEF;
static const int           HEADER_SIZE  = 44;

static const unsigned long PROTO_VERSION = 0x00010016;

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
    static const unsigned long ADD_CONTACT            = 0x1019;
    static const unsigned long ADD_CONTACT_ACK        = 0x101A;
    static const unsigned long MODIFY_CONTACT         = 0x101B;
    static const unsigned long MODIFY_CONTACT_ACK     = 0x101C;
    static const unsigned long AUTHORIZE              = 0x1020;
    static const unsigned long AUTHORIZE_ACK          = 0x1021;
    static const unsigned long CHANGE_STATUS          = 0x1022;
    static const unsigned long OFFLINE_MESSAGE_ACK    = 0x101D;
    static const unsigned long OFFLINE_MESSAGE_DELETE = 0x101E;
    static const unsigned long CONTACT_LIST2          = 0x1037;
    static const unsigned long LOGIN2                 = 0x1038;
    static const unsigned long LOGIN3                 = 0x1078;
    static const unsigned long ANKETA_INFO            = 0x1028;
    static const unsigned long WP_REQUEST             = 0x1029;
}

namespace Anketa {
    namespace SearchField {
        static const unsigned long USER   = 0;
        static const unsigned long DOMAIN = 1;
    }
    namespace Status {
        static const unsigned long NO_USER  = 0x0;
        static const unsigned long OK       = 0x1;
        static const unsigned long DB_ERROR = 0x2;
        static const unsigned long LIMIT    = 0x3;
    }
}

namespace Status {
    // Full set for MRIM 1.22 (Renaissance src/servers/mrim/globals.js).
    // NOTE: there is no separate DND/BUSY — custom "busy" states are
    // XSTATUS (0x4) with xstatusType/Title/Description attached.
    // INVISIBLE (0x80000001) is shown to others as OFFLINE, unless they
    // are in the contact's ALWAYS_VISIBLE list.
    static const unsigned long OFFLINE   = 0x00000000;
    static const unsigned long ONLINE    = 0x00000001;
    static const unsigned long AWAY      = 0x00000002;
    static const unsigned long XSTATUS   = 0x00000004;
    static const unsigned long INVISIBLE = 0x80000001;
}

// Feature flags advertised in LOGIN3 and CHANGE_STATUS (xstatus features).
// Must stay in sync with MrimAuth::SendLogin3 (0x7FF).
namespace Features {
    static const unsigned long DEFAULT = 0x000007FF;
}

namespace MsgFlag {
    static const unsigned long OFFLINE   = 0x00000001;
    static const unsigned long NORECV    = 0x00000004;
    static const unsigned long AUTHORIZE = 0x00000008;
    static const unsigned long SYSTEM    = 0x00000040;
    static const unsigned long RTF       = 0x00000080;
    static const unsigned long CONTACT   = 0x00000200;
    static const unsigned long TYPING    = 0x00000400;
    static const unsigned long MULTICAST = 0x00001000;
    static const unsigned long ALARM     = 0x00004000;
    static const unsigned long FLASH     = 0x00008000;
}

namespace Delivery {
    static const unsigned long SUCCESS          = 0x0000;
    static const unsigned long NO_USER          = 0x8001;
    static const unsigned long INTERNAL_ERROR   = 0x8003;
    static const unsigned long OFFLINE_LIMIT    = 0x8004;
    static const unsigned long TOO_LARGE        = 0x8005;
    static const unsigned long OFFLINE_DISABLED = 0x8006;
}

namespace ContactFlag {
    static const unsigned long INVISIBLE_ALWAYS  = 0x00000004; // "always invisible for"
    static const unsigned long VISIBLE_ALWAYS    = 0x00000008; // "always visible for"
    static const unsigned long IGNORED           = 0x00000010; // in ignore list
    static const unsigned long AUTHORIZED        = 0x00000040; // authorized (rarely used)
    static const unsigned long CONFERENCE        = 0x00000080; // conference (not in Renaissance)
    static const unsigned long UNICODE_NICKNAME  = 0x00000200; // nickname in Unicode
    static const unsigned long PHONE             = 0x00100000; // contact is a phone number
    // ADD_CONTACT request flags
    static const unsigned long FL_DELETE         = 0x00000001;
    static const unsigned long FL_GROUP          = 0x00000002;
    static const unsigned long FL_NOT_IN_LIST    = 0x00000020;
}

namespace ContactError {
    static const unsigned long SUCCESS           = 0x0;
    static const unsigned long FAILURE           = 0x1;
    static const unsigned long INTERNAL_ERROR    = 0x2;
    static const unsigned long NO_SUCH_USER      = 0x3;
    static const unsigned long INVALID_DATA      = 0x4;
    static const unsigned long ALREADY_EXISTS    = 0x5;
    static const unsigned long GROUP_LIMIT       = 0x6;
}

}

#endif
