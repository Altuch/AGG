#include "MRIM/MrimUtils.h"
#include "MRIM/MrimProtocol.h"

using namespace Osp::Base;
using namespace Osp::Base::Utility;

static mchar Cp1251ByteToUnicode(byte b) {
    if (b < 0x80) return (mchar)b;
    if (b >= 0xC0 && b <= 0xFF) return (mchar)(0x0410 + (b - 0xC0)); // А-Я, а-я
    switch (b) {
        case 0xA8: return 0x0401;
        case 0xB8: return 0x0451;
        case 0xAA: return 0x0404;
        case 0xBA: return 0x0454;
        case 0xAF: return 0x0407;
        case 0xBF: return 0x0457;
        case 0xB2: return 0x0406;
        case 0xB3: return 0x0456;
        case 0xA5: return 0x0490;
        case 0xB4: return 0x0491;
        case 0xA1: return 0x040E;
        case 0xA2: return 0x045E;
        case 0x90: return 0x0402;
        case 0x93: return 0x201C;
        case 0x94: return 0x201D;
        case 0x96: return 0x2013;
        case 0x97: return 0x2014;
        case 0xA0: return 0x00A0;
        default:   return (mchar)b;
    }
}

static byte UnicodeToCp1251Byte(mchar ch) {
    if (ch < 0x80) return (byte)ch;
    if (ch >= 0x0410 && ch <= 0x044F) return (byte)(0xC0 + (ch - 0x0410));
    switch (ch) {
        case 0x0401: return 0xA8;
        case 0x0451: return 0xB8;
        case 0x0404: return 0xAA;
        case 0x0454: return 0xBA;
        case 0x0407: return 0xAF;
        case 0x0457: return 0xBF;
        case 0x0406: return 0xB2;
        case 0x0456: return 0xB3;
        case 0x0490: return 0xA5;
        case 0x0491: return 0xB4;
        case 0x040E: return 0xA1;
        case 0x045E: return 0xA2;
        case 0x0402: return 0x90;
        case 0x00A0: return 0xA0;
        default:     return '?';
    }
}

void MrimUtils::AppendUL(ByteBuffer& buffer, unsigned long value) {
    byte data[4];
    data[0] = (byte)(value & 0xFF);
    data[1] = (byte)((value >> 8) & 0xFF);
    data[2] = (byte)((value >> 16) & 0xFF);
    data[3] = (byte)((value >> 24) & 0xFF);
    buffer.SetArray(data, 0, 4);
}

void MrimUtils::BuildHeader(ByteBuffer& buffer, unsigned long command, unsigned long dataLen) {
    AppendUL(buffer, Mrim::MAGIC);
    AppendUL(buffer, Mrim::PROTO_VERSION);
    AppendUL(buffer, 0x0000000A);
    AppendUL(buffer, command);
    AppendUL(buffer, dataLen);
    AppendUL(buffer, 0);
    AppendUL(buffer, 0);
    byte reserved[16] = {0};
    buffer.SetArray(reserved, 0, 16);
}

void MrimUtils::AppendLPS(ByteBuffer& buffer, const String& text) {
    int strLen = text.GetLength();
    if (strLen == 0) {
        AppendUL(buffer, 0);
        return;
    }
    AppendUL(buffer, strLen);
    for (int i = 0; i < strLen; i++) {
        mchar ch;
        text.GetCharAt(i, ch);
        byte b = UnicodeToCp1251Byte(ch);
        buffer.SetByte(b);
    }
}

void MrimUtils::AppendRawBytes(ByteBuffer& buffer, const byte* data, int len) {
    AppendUL(buffer, (unsigned long)len);
    if (len > 0 && data != null) {
        buffer.SetArray(data, 0, len);
    }
}

void MrimUtils::AppendLPSUcs2(ByteBuffer& buffer, const String& text) {
    int strLen = text.GetLength();
    if (strLen == 0) {
        AppendUL(buffer, 0);
        return;
    }
    AppendUL(buffer, strLen * 2);
    for (int i = 0; i < strLen; i++) {
        mchar ch;
        text.GetCharAt(i, ch);
        buffer.SetByte((byte)(ch & 0xFF));
        buffer.SetByte((byte)((ch >> 8) & 0xFF));
    }
}

unsigned long MrimUtils::ReadUL(ByteBuffer& buffer) {
    if (buffer.GetRemaining() < 4) return 0;
    byte b[4];
    buffer.GetArray(b, 0, 4);
    return (unsigned long)(b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24));
}

String MrimUtils::ReadLPS(ByteBuffer& buffer) {
    if (buffer.GetRemaining() < 4) return String(L"");
    unsigned long len = ReadUL(buffer);
    if (len == 0 || len > 20000 || (unsigned long)buffer.GetRemaining() < len) return String(L"");

    byte* strBytes = new byte[len];
    buffer.GetArray(strBytes, 0, len);

    String resultStr;
    for (unsigned long i = 0; i < len; i++) {
        resultStr.Append(Cp1251ByteToUnicode(strBytes[i]));
    }
    delete[] strBytes;
    return resultStr;
}

String MrimUtils::ReadLPSUcs2(ByteBuffer& buffer) {
    if (buffer.GetRemaining() < 4) return String(L"");
    unsigned long len = ReadUL(buffer);
    if (len == 0 || len > 20000 || (unsigned long)buffer.GetRemaining() < len) return String(L"");

    byte* strBytes = new byte[len];
    buffer.GetArray(strBytes, 0, len);

    String resultStr;
    unsigned long charCount = len / 2;
    for (unsigned long i = 0; i < charCount; i++) {
        mchar ch = (mchar)(strBytes[i * 2] | (strBytes[i * 2 + 1] << 8));
        resultStr.Append(ch);
    }
    delete[] strBytes;
    return resultStr;
}

static int Base64CharValue(mchar c) {
    if (c >= L'A' && c <= L'Z') return c - L'A';
    if (c >= L'a' && c <= L'z') return c - L'a' + 26;
    if (c >= L'0' && c <= L'9') return c - L'0' + 52;
    if (c == L'+') return 62;
    if (c == L'/') return 63;
    return -1;
}

String MrimUtils::Base64DecodeUtf16LEToString(const String& base64Text) {
    int len = base64Text.GetLength();
    if (len <= 0 || len > 20000) return String(L"");

    byte* raw = new byte[(len / 4 + 1) * 3];
    int rawLen = 0;

    int bits = 0;
    int bitCount = 0;
    for (int i = 0; i < len; i++) {
        mchar ch;
        base64Text.GetCharAt(i, ch);
        int val = Base64CharValue(ch);
        if (val < 0) continue;

        bits = (bits << 6) | val;
        bitCount += 6;
        if (bitCount >= 8) {
            bitCount -= 8;
            raw[rawLen++] = (byte)((bits >> bitCount) & 0xFF);
        }
    }

    String result;
    int charCount = rawLen / 2;
    for (int i = 0; i < charCount; i++) {
        mchar ch = (mchar)(raw[i * 2] | (raw[i * 2 + 1] << 8));
        result.Append(ch);
    }

    delete[] raw;
    return result;
}

void MrimUtils::SkipFormattedRecord(ByteBuffer& buffer, const String& mask, int startIndex) {
    int maskLen = mask.GetLength();
    if (maskLen <= 0 || startIndex < 0 || startIndex >= maskLen) return;
    for (int i = startIndex; i < maskLen; i++) {
        if (buffer.GetRemaining() < 4) return;
        mchar c;
        mask.GetCharAt(i, c);
        if (c == L's') {
            unsigned long len = ReadUL(buffer);
            if (len > 0 && len <= 20000 && (unsigned long)buffer.GetRemaining() >= len) {
                buffer.SetPosition(buffer.GetPosition() + len);
            } else if (len != 0) {
                return;
            }
        } else {
            ReadUL(buffer);
        }
    }
}

bool MrimUtils::SafeIndexOf(const String& text, const String& pattern, int startIndex, int& pos) {
    pos = -1;
    if (text.IsEmpty() || pattern.IsEmpty()) return false;
    int textLen = text.GetLength();
    if (startIndex < 0 || startIndex >= textLen) return false;
    text.IndexOf(pattern, startIndex, pos);
    return pos >= 0;
}

bool MrimUtils::SafeIndexOfChar(const String& text, mchar ch, int& pos) {
    pos = -1;
    if (text.IsEmpty()) return false;
    String pattern;
    pattern.Append(ch);
    text.IndexOf(pattern, 0, pos);
    return pos >= 0;
}

String MrimUtils::NormalizeEmail(const String& email) {
    String clean = email;
    clean.Trim();
    clean.ToLower();
    return clean;
}


bool MrimUtils::IsValidIpAddress(const String& ip) {
    String trimmedIp = ip;
    trimmedIp.Trim();
    if (trimmedIp.IsEmpty()) return false;

    StringTokenizer strTok(trimmedIp, L".");
    if (strTok.GetTokenCount() != 4) return false;

    while (strTok.HasMoreTokens()) {
        String token;
        strTok.GetNextToken(token);
        token.Trim();
        if (token.IsEmpty() || token.GetLength() > 3) return false;

        for (int i = 0; i < token.GetLength(); i++) {
            mchar c;
            token.GetCharAt(i, c);
            if (c < L'0' || c > L'9') return false;
        }

        int val = -1;
        if (IsFailed(Integer::Parse(token, val))) return false;
        if (val < 0 || val > 255) return false;
    }
    return true;
}

bool MrimUtils::IsValidHost(const String& host) {
    String trimmed = host;
    trimmed.Trim();
    if (trimmed.IsEmpty() || trimmed.GetLength() > 253) return false;

    if (IsValidIpAddress(trimmed)) return true;

    int len = trimmed.GetLength();
    for (int i = 0; i < len; i++) {
        mchar c;
        trimmed.GetCharAt(i, c);
        bool ok = (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z')
                || (c >= L'0' && c <= L'9') || c == L'.' || c == L'-';
        if (!ok) return false;
    }

    mchar first, last;
    trimmed.GetCharAt(0, first);
    trimmed.GetCharAt(len - 1, last);
    return first != L'.' && first != L'-' && last != L'.' && last != L'-';
}
