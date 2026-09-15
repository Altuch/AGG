#include "Core/ChatHistory.h"
#include "Core/Loc.h"
#include <FIo.h>

using namespace Osp::Base;
using namespace Osp::Base::Collection;
using namespace Osp::Io;

Osp::Base::String ChatHistory::GetSelfLabel(void) {
    return LocString(L"IDS_SELF_LABEL");
}

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

int ChatHistory::LoadRecentPairs(const String& email, int maxLines, IList* pOut) {
    if (email.IsEmpty() || pOut == null) return 0;

    File file;
    if (IsFailed(file.Construct(GetFilePath(email), L"r"))) {
        return 0;
    }

    if (maxLines <= 0 || maxLines > MAX_RING) maxLines = MAX_RING;

    String ring[MAX_RING];
    int count = 0;
    int next = 0;

    while (true) {
        String line;
        if (IsFailed(file.Read(line))) break;

        line.Trim();
        if (line.IsEmpty()) continue;

        ring[next] = line;
        next = (next + 1) % maxLines;
        if (count < maxLines) count++;
    }

    int start = (count < maxLines) ? 0 : next;

    for (int i = 0; i < count; i++) {
        String& line = ring[(start + i) % maxLines];
        pOut->Add(*(new String(line)));
    }

    return count;
}
