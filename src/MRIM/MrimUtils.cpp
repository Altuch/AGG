#include "MRIM/MrimUtils.h"

using namespace Osp::Base;
using namespace Osp::Base::Utility;

static mchar Cp1251ByteToUnicode(byte b) {
    if (b < 0x80) return (mchar)b;
    if (b >= 0xC0 && b <= 0xFF) return (mchar)(0x0410 + (b - 0xC0)); // А-Я, а-я
    switch (b) {
        case 0xA8: return 0x0401; // Ё
        case 0xB8: return 0x0451; // ё
        case 0xAA: return 0x0404; // Є
        case 0xBA: return 0x0454; // є
        case 0xAF: return 0x0407; // Ї
        case 0xBF: return 0x0457; // ї
        case 0xB2: return 0x0406; // І
        case 0xB3: return 0x0456; // і
        case 0xA5: return 0x0490; // Ґ
        case 0xB4: return 0x0491; // ґ
        case 0xA1: return 0x040E; // Ў
        case 0xA2: return 0x045E; // ў
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
        case 0x0401: return 0xA8; // Ё
        case 0x0451: return 0xB8; // ё
        case 0x0404: return 0xAA; // Є
        case 0x0454: return 0xBA; // є
        case 0x0407: return 0xAF; // Ї
        case 0x0457: return 0xBF; // ї
        case 0x0406: return 0xB2; // І
        case 0x0456: return 0xB3; // і
        case 0x0490: return 0xA5; // Ґ
        case 0x0491: return 0xB4; // ґ
        case 0x040E: return 0xA1; // Ў
        case 0x045E: return 0xA2; // ў
        case 0x0402: return 0x90; // Ђ
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
    AppendUL(buffer, 0xDEADBEEF);
    AppendUL(buffer, 0x00010008);
    AppendUL(buffer, 0x0000000A);
    AppendUL(buffer, command);
    AppendUL(buffer, dataLen);
    AppendUL(buffer, 0);
    AppendUL(buffer, 0);
    byte zeros[16] = {0};
    buffer.SetArray(zeros, 0, 16);
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
    if (len == 0 || len > 4096 || (unsigned long)buffer.GetRemaining() < len) return String(L"");

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
    if (len == 0 || len > 8192 || (unsigned long)buffer.GetRemaining() < len) return String(L"");

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

void MrimUtils::SkipFormattedRecord(ByteBuffer& buffer, const String& mask, int startIndex) {
    int maskLen = mask.GetLength();
    for (int i = startIndex; i < maskLen; i++) {
        mchar c;
        mask.GetCharAt(i, c);
        if (c == L's') {
            unsigned long len = ReadUL(buffer);
            if (len > 0 && (unsigned long)buffer.GetRemaining() >= len) {
                buffer.SetPosition(buffer.GetPosition() + len);
            }
        } else {
            ReadUL(buffer);
        }
    }
}

String MrimUtils::GetHistoryFilePath(const String& email) {
    String safeName = email;
    safeName.Trim();
    safeName.ToLower();
    safeName.Replace(L"@", L"_");
    safeName.Replace(L".", L"_");
    return L"/Home/hist_" + safeName + L".dat";
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
