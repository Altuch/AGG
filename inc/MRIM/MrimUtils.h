#ifndef _MRIM_UTILS_H_
#define _MRIM_UTILS_H_

#include <FBase.h>

class MrimUtils {
public:
    static void AppendUL(Osp::Base::ByteBuffer& buffer, unsigned long value);
    static void AppendLPS(Osp::Base::ByteBuffer& buffer, const Osp::Base::String& text);
    static unsigned long ReadUL(Osp::Base::ByteBuffer& buffer);
    static Osp::Base::String ReadLPS(Osp::Base::ByteBuffer& buffer);
    static void BuildHeader(Osp::Base::ByteBuffer& buffer, unsigned long command, unsigned long dataLen);
    static bool IsValidIpAddress(const Osp::Base::String& ip);
};

#endif
