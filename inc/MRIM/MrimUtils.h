#ifndef _MRIM_UTILS_H_
#define _MRIM_UTILS_H_

#include <FBase.h>

// Кодування/декодування полів протоколу MRIM.
//
// LPS - рядок у форматі "довжина (4 байти) + байти". Для протоколу 1.8
// байти завжди CP1251; UCS2-варіант лишається для полів, які сервер
// може прислати в юнікоді (нікнейми з відповідним прапорцем).
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

    // Пропускає решту запису за маскою полів ("u" - число, "s" - LPS).
    static void SkipFormattedRecord(Osp::Base::ByteBuffer& buffer, const Osp::Base::String& mask, int startIndex);
};

#endif
