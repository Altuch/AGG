#ifndef _MRIM_UTILS_H_
#define _MRIM_UTILS_H_

#include <FBase.h>

class MrimUtils {
public:
    static void AppendUL(Osp::Base::ByteBuffer& buffer, unsigned long value);

    static void AppendLPS(Osp::Base::ByteBuffer& buffer, const Osp::Base::String& text);
    static Osp::Base::String ReadLPS(Osp::Base::ByteBuffer& buffer);

    static void AppendLPSUcs2(Osp::Base::ByteBuffer& buffer, const Osp::Base::String& text);
    static Osp::Base::String ReadLPSUcs2(Osp::Base::ByteBuffer& buffer);

    static unsigned long ReadUL(Osp::Base::ByteBuffer& buffer);
    static Osp::Base::String Base64DecodeUtf16LEToString(const Osp::Base::String& base64Text);
    static void BuildHeader(Osp::Base::ByteBuffer& buffer, unsigned long command, unsigned long dataLen);
    static bool IsValidIpAddress(const Osp::Base::String& ip);

    static void SkipFormattedRecord(Osp::Base::ByteBuffer& buffer, const Osp::Base::String& mask, int startIndex);
    static Osp::Base::String GetHistoryFilePath(const Osp::Base::String& email);
};

#endif
