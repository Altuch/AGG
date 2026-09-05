#ifndef _CHAT_HISTORY_H_
#define _CHAT_HISTORY_H_

#include <FBase.h>

class ChatHistory {
public:
    static Osp::Base::String GetFilePath(const Osp::Base::String& email);

    static void Append(const Osp::Base::String& email,
                       const Osp::Base::String& sender,
                       const Osp::Base::String& text);

    static Osp::Base::String LoadRecentAsText(const Osp::Base::String& email, int maxLines);

    static void Clear(const Osp::Base::String& email);

    static const wchar_t* SELF_LABEL;

private:
    static const int MAX_RING = 200;
};

#endif
