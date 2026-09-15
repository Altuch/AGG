#include "Ui/ChatForm.h"
#include "Core/Loc.h"
#include "Ui/FormNavigator.h"
#include "Core/ChatHistory.h"
#include "MRIM/MrimProtocol.h"

using namespace Osp::Base;
using namespace Osp::Base::Collection;
using namespace Osp::System;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;

ChatForm::ChatForm(void) :
    pConnection(null), pInputField(null), pHistoryList(null), pRowFormats(null), rowSeq_(0),
    pMeasureFont_(null) {}

ChatForm::~ChatForm(void) {
    if (pConnection != null) pConnection->ClearActiveChatListener();
    if (pRowFormats != null) {
        pRowFormats->RemoveAll(true);
        delete pRowFormats;
    }
    delete pMeasureFont_;
    pMeasureFont_ = null;
}

result ChatForm::Initialize(AggConnection* pConn, const String& name, const String& email) {
    pConnection = pConn;

    contactEmail = email;
    contactEmail.Trim();
    contactEmail.ToLower();

    contactName = name.IsEmpty() ? contactEmail : name;

    if (pConnection != null) pConnection->SetActiveChatListener(this, contactEmail);

    return Construct(L"IDF_CHAT");
}

result ChatForm::OnInitializing(void) {
    SetTitleText(contactName);

    SetSoftkeyActionId(SOFTKEY_0, ID_SOFTKEY_BACK);
    AddSoftkeyActionListener(SOFTKEY_0, *this);

    SetSoftkeyActionId(SOFTKEY_1, ID_BTN_SEND);
    AddSoftkeyActionListener(SOFTKEY_1, *this);

    SetOptionkeyActionId(ID_OPTIONKEY_CHAT);
    AddOptionkeyActionListener(*this);

    EditArea* pHistArea = static_cast<EditArea*>(GetControl(L"IDC_HISTORY"));
    if (pHistArea != null) pHistArea->SetShowState(false);

    pInputField = static_cast<EditField*>(GetControl(L"IDC_TEXT"));
    if (pInputField != null) pInputField->AddTextEventListener(*this);
    SetSoftkeyText(SOFTKEY_0, LocString(L"IDS_SK_BACK"));
    SetSoftkeyText(SOFTKEY_1, LocString(L"IDS_SK_SEND"));
    if (pInputField != null) pInputField->SetGuideText(LocString(L"IDS_GUIDE_MESSAGE"));

    pRowFormats = new ArrayList();
    pRowFormats->Construct();

    pMeasureFont_ = new Osp::Graphics::Font();
    if (pMeasureFont_ != null) {
        if (IsFailed(pMeasureFont_->Construct(Osp::Graphics::FONT_STYLE_PLAIN, 28))) {
            delete pMeasureFont_;
            pMeasureFont_ = null;
        }
    }

    Osp::Graphics::Rectangle client = GetClientAreaBounds();
    int listH = client.height;
    if (pInputField != null) {
        int ix = 0, iy = 0, iw = 0, ih = 0;
        pInputField->GetBounds(ix, iy, iw, ih);
        if (iy > 0) listH = iy;
    }
    pHistoryList = new CustomList();
    pHistoryList->Construct(Osp::Graphics::Rectangle(0, 0, client.width, listH), CUSTOM_LIST_STYLE_NORMAL);
    AddControl(*pHistoryList);
    pHistoryList->SetShowState(true);
    pHistoryList->Show();

    LoadHistory();

    return E_SUCCESS;
}

result ChatForm::OnTerminating(void) {
    return E_SUCCESS;
}

void ChatForm::AppendRow(const String& nick, const String& text, int rowKind) {
    if (pHistoryList == null || pRowFormats == null) return;
    if (nick.IsEmpty() && text.IsEmpty()) return;

    Osp::Graphics::Color nickColor(150, 150, 150);
    if (rowKind == ROW_INCOMING) nickColor = Osp::Graphics::Color(235, 20, 20);
    else if (rowKind == ROW_OWN) nickColor = Osp::Graphics::Color(30, 144, 255);

    int w = GetClientAreaBounds().width;
    if (w <= 0) w = 480;
    int textW = w - 24;
    int charsPerLine = CalibratedCharsPerLine(textW);
    if (charsPerLine < 10) charsPerLine = 10;

    static const int LINES_PER_ROW = 8;
    int chunkChars = LINES_PER_ROW * charsPerLine;
    int len = text.GetLength();

    ArrayList srcLines;
    srcLines.Construct();
    if (len == 0) {
        srcLines.Add(*(new String(L"")));
    } else {
        int start = 0;
        while (start < len) {
            int nl = -1;
            text.IndexOf(L"\n", start, nl);
            String line;
            if (nl < 0) {
                text.SubString(start, line);
                srcLines.Add(*(new String(line)));
                break;
            }
            text.SubString(start, nl - start, line);
            srcLines.Add(*(new String(line)));
            start = nl + 1;
            if (start >= len) {
                srcLines.Add(*(new String(L"")));
                break;
            }
        }
    }

    bool first = true;
    String cur;
    int curLines = 0;
    for (int i = 0; i < srcLines.GetCount(); i++) {
        String* pSrc = static_cast<String*>(srcLines.GetAt(i));
        String src = (pSrc != null) ? *pSrc : String(L"");

        while (src.GetLength() > chunkChars) {
            if (curLines > 0) {
                AppendRowChunk(first ? nick : String(L""), cur, nickColor, charsPerLine);
                first = false;
                cur = L"";
                curLines = 0;
            }
            String piece;
            src.SubString(0, chunkChars, piece);
            AppendRowChunk(first ? nick : String(L""), piece, nickColor, charsPerLine);
            first = false;
            String rest;
            src.SubString(chunkChars, rest);
            src = rest;
        }

        int need = src.GetLength() / charsPerLine + 1;
        if (curLines + need > LINES_PER_ROW && curLines > 0) {
            AppendRowChunk(first ? nick : String(L""), cur, nickColor, charsPerLine);
            first = false;
            cur = L"";
            curLines = 0;
        }
        if (!cur.IsEmpty() || curLines > 0) cur.Append(L"\n");
        cur.Append(src);
        curLines += need;
    }
    if (curLines > 0 || first) {
        AppendRowChunk(first ? nick : String(L""), cur, nickColor, charsPerLine);
    }
    srcLines.RemoveAll(true);

    pHistoryList->ScrollToBottom();

    if (GetParent() != null) {
        pHistoryList->RequestRedraw(true);
        pHistoryList->Draw();
        pHistoryList->Show();
    }
}

void ChatForm::AppendRowChunk(const String& nick, const String& chunk,
                              const Osp::Graphics::Color& nickColor,
                              int charsPerLine) {

    int newlines = 0;
    for (int i = 0; i < chunk.GetLength(); i++) {
        mchar c = 0;
        chunk.GetCharAt(i, c);
        if (c == L'\n') newlines++;
    }
    int lines = chunk.GetLength() / charsPerLine + 1 + newlines;
    if (lines < 1) lines = 1;
    if (lines > 12) lines = 12;
    int h = 44 + lines * 36;

    int w = GetClientAreaBounds().width;
    if (w <= 0) w = 480;
    int textW = w - 24;

    CustomListItemFormat* pFmt = new CustomListItemFormat();
    pFmt->Construct();
    pFmt->AddElement(ELEM_NICK, Osp::Graphics::Rectangle(12, 4, textW, 32), 30, nickColor, nickColor);
    pFmt->AddElement(ELEM_BODY, Osp::Graphics::Rectangle(12, 40, textW, h - 48),
                     28, Osp::Graphics::Color(255, 255, 255), Osp::Graphics::Color(255, 255, 255));
    pRowFormats->Add(*pFmt);

    CustomListItem* pItem = new CustomListItem();
    pItem->Construct(h);
    pItem->SetItemFormat(*pFmt);
    pItem->SetElement(ELEM_NICK, nick);
    pItem->SetElement(ELEM_BODY, chunk);
    pHistoryList->AddItem(*pItem, rowSeq_++);
}

void ChatForm::LoadHistory(void) {
    if (pHistoryList == null) return;

    ArrayList pairs;
    pairs.Construct();
    ChatHistory::LoadRecentPairs(contactEmail, MAX_LOADED_HISTORY_LINES, &pairs);
    for (int i = 0; i < pairs.GetCount(); i++) {
        String* pLine = static_cast<String*>(pairs.GetAt(i));
        if (pLine == null || pLine->IsEmpty()) continue;

        int tab = -1;
        pLine->IndexOf(L"\t", 0, tab);
        if (tab <= 0) continue;

        String sender;
        String body;
        pLine->SubString(0, tab, sender);
        pLine->SubString(tab + 1, body);
        body.Trim();
        if (body.IsEmpty()) continue;

        // Router saves incoming senders as the (lowercase) email, own
        // messages under the display label — direction follows from that.
        if (sender.Equals(contactEmail, true)) {
            AppendRow(contactName, body, ROW_INCOMING);
        } else {
            AppendRow(sender, body, ROW_OWN);
        }
    }
    pairs.RemoveAll(true);
}

int ChatForm::CalibratedCharsPerLine(int textW) const {
    if (pMeasureFont_ != null && textW > 0) {
        String sample(L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 ");
        Osp::Graphics::Dimension dim;
        if (!IsFailed(pMeasureFont_->GetTextExtent(sample, sample.GetLength(), dim))
            && dim.width > 0) {
            int avg = dim.width / sample.GetLength();
            if (avg > 0) {
                int cpl = textW / avg;
                return (cpl < 10) ? 10 : cpl;
            }
        }
    }
    int fallback = textW / 16;
    return (fallback < 10) ? 10 : fallback;
}

void ChatForm::ClearHistoryView(void) {
    if (pHistoryList == null) return;

    pHistoryList->RemoveAllItems();
    if (pRowFormats != null) pRowFormats->RemoveAll(true);

    if (GetParent() != null) {
        pHistoryList->RequestRedraw(true);
        pHistoryList->Draw();
        pHistoryList->Show();
    }
}

void ChatForm::AppendLine(const String& sender, const String& text, bool saveToHistory, int rowKind) {
    if (pHistoryList == null) return;

    AppendRow(sender, text, rowKind);

    if (saveToHistory) ChatHistory::Append(contactEmail, sender, text);
}

void ChatForm::ShowSystemLine(const String& text) {
    AppendLine(LocString(L"IDS_SYS_SENDER"), text, false, ROW_SYSTEM);
}

void ChatForm::SendNudgeNow(void) {
    if (pConnection == null) return;

    pConnection->SendNudge(contactEmail);
    AppendLine(ChatHistory::GetSelfLabel(), LocString(L"IDS_NUDGE_SENT"), true, ROW_OWN);
}

void ChatForm::OnActionPerformed(const Control& source, int actionId) {
    switch (actionId) {
        case ID_BTN_SEND: {
            if (pInputField == null || pConnection == null) break;

            String text = pInputField->GetText();
            text.Trim();
            if (text.IsEmpty()) break;

            pConnection->SendMessageTo(contactEmail, text);
            AppendLine(ChatHistory::GetSelfLabel(), text, true, ROW_OWN);

            pInputField->SetText(L"");
            if (GetParent() != null) {
                pInputField->RequestRedraw(true);
                pInputField->Draw();
                pInputField->Show();
            }
            break;
        }

        case ID_OPTIONKEY_CHAT: {
            OptionMenu* pMenu = new OptionMenu();
            pMenu->Construct();
            pMenu->AddItem(LocString(L"IDS_MENU_NUDGE"), ID_MENU_NUDGE);
            pMenu->AddItem(LocString(L"IDS_MENU_PROFILE"), ID_MENU_INFO);
            pMenu->AddItem(LocString(L"IDS_MENU_CLEAR"), ID_MENU_CLEAR_HISTORY);
            pMenu->AddActionEventListener(*this);
            pMenu->SetShowState(true);
            pMenu->Show();
            break;
        }

        case ID_MENU_NUDGE: {
            SendNudgeNow();
            break;
        }

        case ID_MENU_INFO: {
            FormNavigator::GoToProfile(pConnection, contactName, contactEmail, null);
            break;
        }

        case ID_MENU_CLEAR_HISTORY: {
            ClearHistoryView();
            ChatHistory::Clear(contactEmail);
            break;
        }

        case ID_SOFTKEY_BACK: {
            AggConnection* pConn = pConnection;
            if (pConn != null) pConn->ClearActiveChatListener();

            pConnection = null;

            FormNavigator::GoToContactList(pConn, this);
            break;
        }

        default:
            break;
    }
}

void ChatForm::OnTextValueChanged(const Control& source) {
    if (pConnection != null && !contactEmail.IsEmpty()) pConnection->SendTyping(contactEmail);
}

void ChatForm::OnMessageReceived(const String& sender, const String& text, bool isNudge) {
    String cleanSender = sender;
    cleanSender.Trim();
    cleanSender.ToLower();
    if (!cleanSender.Equals(contactEmail, true)) return;

    if (isNudge) {
        Vibrator vibrator;
        vibrator.Construct();
        vibrator.Start(1000, 100);
        AppendRow(contactName, LocString(L"IDS_NUDGE_RECEIVED"), ROW_INCOMING);
    } else if (!text.IsEmpty()) {
        AppendRow(contactName, text, ROW_INCOMING);
    }
}

void ChatForm::OnMessageDeliveryStatus(unsigned long status) {
    switch (status) {
        case Mrim::Delivery::SUCCESS:
            break;
        case Mrim::Delivery::NO_USER:
            ShowSystemLine(LocString(L"IDS_DLV_NO_USER"));
            break;
        case Mrim::Delivery::OFFLINE_DISABLED:
            ShowSystemLine(LocString(L"IDS_DLV_OFFLINE"));
            break;
        case Mrim::Delivery::OFFLINE_LIMIT:
            ShowSystemLine(LocString(L"IDS_DLV_LIMIT"));
            break;
        case Mrim::Delivery::TOO_LARGE:
            ShowSystemLine(LocString(L"IDS_DLV_TOOLARGE"));
            break;
        case Mrim::Delivery::INTERNAL_ERROR:
            ShowSystemLine(LocString(L"IDS_DLV_INTERNAL"));
            break;
        default:
            break;
    }
}

void ChatForm::OnTypingReceived(const String& sender) {
    String cleanSender = sender;
    cleanSender.Trim();
    cleanSender.ToLower();
    if (!cleanSender.Equals(contactEmail, true)) return;

    SetTitleText(contactName + LocString(L"IDS_TYPING_SUFFIX"));
    if (GetParent() != null) {
        RequestRedraw(true);
        Draw();
        Show();
    }
}
