#ifndef _MRIM_UTILS_H_
#define _MRIM_UTILS_H_

#include <FBase.h>

class MrimUtils {
public:
    static void AppendUL(Osp::Base::ByteBuffer& buffer, unsigned long value);

    // LPS-рядок у кодуванні CP1251 (Windows-1251) — так MRIM передає
    // логін/пароль/email/телефон/xstatus/client. Саме це "класичний" LPS.
    static void AppendLPS(Osp::Base::ByteBuffer& buffer, const Osp::Base::String& text);
    static Osp::Base::String ReadLPS(Osp::Base::ByteBuffer& buffer);

    // LPS-рядок у кодуванні UCS2 (UTF-16LE без BOM, довжина в байтах) —
    // так MRIM передає нікнейми/імена, текст повідомлень, xstatus title/desc.
    static void AppendLPSUcs2(Osp::Base::ByteBuffer& buffer, const Osp::Base::String& text);
    static Osp::Base::String ReadLPSUcs2(Osp::Base::ByteBuffer& buffer);

    static unsigned long ReadUL(Osp::Base::ByteBuffer& buffer);
    static void BuildHeader(Osp::Base::ByteBuffer& buffer, unsigned long command, unsigned long dataLen);
    static bool IsValidIpAddress(const Osp::Base::String& ip);

    // Пропускає одне поле "форматного" запису MRIM_CS_CONTACT_LIST2 /
    // групи: 's' у форматному рядку (mask) означає LPS-рядок (пропускаємо
    // за довжиною), будь-який інший символ — простий DWORD.
    static void SkipFormattedRecord(Osp::Base::ByteBuffer& buffer, const Osp::Base::String& mask, int startIndex);
};

#endif
