#include "Core/ChatHistory.h"
#include <FIo.h>

using namespace Osp::Base;
using namespace Osp::Io;

const wchar_t* ChatHistory::SELF_LABEL = L"Ви";

String ChatHistory::GetFilePath(const String& email) {
    String safeName = email;
    safeName.Trim();
    safeName.ToLower();
    safeName.Replace(L"@", L"_");
    safeName.Replace(L".", L"_");
    return L"/Home/hist_" + safeName + L".dat";
}

void ChatHistory::Append(const String& email, const String& sender, const String& text) {
    if (email.IsEmpty()) return;

    File file;
    String path = GetFilePath(email);
    result r = file.Construct(path, L"a+");
    if (IsFailed(r)) {
        r = file.Construct(path, L"w");
        if (IsFailed(r)) return;
    }

    file.Write(sender + L"\t" + text + L"\n");
}

String ChatHistory::LoadRecentAsText(const String& email, int maxLines) {
    if (email.IsEmpty()) return String(L"");

    File file;
    if (IsFailed(file.Construct(GetFilePath(email), L"r"))) {
        return String(L"");
    }

    if (maxLines <= 0 || maxLines > MAX_RING) maxLines = MAX_RING;

    String ring[MAX_RING];
    int count = 0;
    int next = 0;

    while (true) {
        String line;
        if (IsFailed(file.Read(line))) break; // E_END_OF_FILE

        line.Trim();
        if (line.IsEmpty()) continue;

        ring[next] = line;
        next = (next + 1) % maxLines;
        if (count < maxLines) count++;
    }

    int start = (count < maxLines) ? 0 : next;

    String text;
    for (int i = 0; i < count; i++) {
        String& line = ring[(start + i) % maxLines];

        int tabPos = -1;
        line.IndexOf(L"\t", 0, tabPos);
        if (tabPos < 0) continue;

        String sender;
        String body;
        line.SubString(0, tabPos, sender);
        line.SubString(tabPos + 1, body);

        text.Append(sender);
        text.Append(L": ");
        text.Append(body);
        text.Append(L"\n\n");
    }

    return text;
}

void ChatHistory::Clear(const String& email) {
    if (email.IsEmpty()) return;
    File::Remove(GetFilePath(email));
}
