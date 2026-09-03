#include "MRIM/MrimUtils.h"
#include <FText.h>

using namespace Osp::Base;
using namespace Osp::Base::Utility;
using namespace Osp::Text;

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
    if (text.IsEmpty()) {
        AppendUL(buffer, 0);
        return;
    }
    ByteBuffer* pTextBytes = Utf8Encoding().GetBytesN(text);
    if (pTextBytes != null) {
        int len = pTextBytes->GetLimit() - 1;
        AppendUL(buffer, len);
        byte* pRawBytes = new byte[len];
        pTextBytes->GetArray(pRawBytes, 0, len);
        buffer.SetArray(pRawBytes, 0, len);
        delete[] pRawBytes;
        delete pTextBytes;
    } else {
        AppendUL(buffer, 0);
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

    ByteBuffer tmpBuf;
    tmpBuf.Construct(len);
    tmpBuf.SetArray(strBytes, 0, len);
    tmpBuf.Flip();

    String resultStr;
    Utf8Encoding().GetString(tmpBuf, resultStr);
    delete[] strBytes;
    return resultStr;
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
