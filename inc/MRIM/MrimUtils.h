#ifndef _MRIM_UTILS_H_
#define _MRIM_UTILS_H_

#include <FBase.h>

class MrimUtils {
public:
    static void AppendUL(Osp::Base::ByteBuffer& buffer, unsigned long value);

    static void AppendLPS(Osp::Base::ByteBuffer& buffer, const Osp::Base::String& text);
    static Osp::Base::String ReadLPS(Osp::Base::ByteBuffer& buffer);

    static void AppendRawBytes(Osp::Base::ByteBuffer& buffer, const byte* data, int len);

    static void AppendLPSUcs2(Osp::Base::ByteBuffer& buffer, const Osp::Base::String& text);
    static Osp::Base::String ReadLPSUcs2(Osp::Base::ByteBuffer& buffer);

    static unsigned long ReadUL(Osp::Base::ByteBuffer& buffer);
    static Osp::Base::String Base64DecodeUtf16LEToString(const Osp::Base::String& base64Text);
    static void BuildHeader(Osp::Base::ByteBuffer& buffer, unsigned long command, unsigned long dataLen);
    static bool IsValidIpAddress(const Osp::Base::String& ip);

    static bool IsValidHost(const Osp::Base::String& host);

    static void SkipFormattedRecord(Osp::Base::ByteBuffer& buffer, const Osp::Base::String& mask, int startIndex);

    static bool SafeIndexOf(const Osp::Base::String& text, const Osp::Base::String& pattern, int startIndex, int& pos);
    static bool SafeIndexOfChar(const Osp::Base::String& text, mchar ch, int& pos);

    static Osp::Base::String NormalizeEmail(const Osp::Base::String& email);
};

#endif
