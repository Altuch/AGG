#include "Ui/ChatForm.h"
#include "Ui/FormNavigator.h"
#include "Core/ChatHistory.h"
#include "MRIM/MrimProtocol.h"

using namespace Osp::Base;
using namespace Osp::Base::Collection;
using namespace Osp::System;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;

ChatForm::ChatForm(void) :
    pConnection(null), pHistoryArea(null), pInputField(null) {}

ChatForm::~ChatForm(void) {
    if (pConnection != null) pConnection->ClearActiveChatListener();
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

    pHistoryArea = static_cast<EditArea*>(GetControl(L"IDC_HISTORY"));
    if (pHistoryArea != null) {
        pHistoryArea->SetKeypadEnabled(false);
    }

    pInputField = static_cast<EditField*>(GetControl(L"IDC_TEXT"));
    if (pInputField != null) pInputField->AddTextEventListener(*this);

    LoadHistory();

    return E_SUCCESS;
}

result ChatForm::OnTerminating(void) {
    return E_SUCCESS;
}

void ChatForm::RedrawHistory(void) {
    if (pHistoryArea == null || GetParent() == null) return;

    pHistoryArea->RequestRedraw(true);
    pHistoryArea->Draw();
    pHistoryArea->Show();
}

void ChatForm::LoadHistory(void) {
    if (pHistoryArea == null) return;

    String text = ChatHistory::LoadRecentAsText(contactEmail, MAX_LOADED_HISTORY_LINES);
    if (!text.IsEmpty()) pHistoryArea->SetText(text);
}

void ChatForm::AppendLine(const String& sender, const String& text, bool saveToHistory) {
    if (pHistoryArea == null) return;

    pHistoryArea->AppendText(sender + L": " + text + L"\n\n");
    RedrawHistory();

    if (saveToHistory) ChatHistory::Append(contactEmail, sender, text);
}

void ChatForm::ShowSystemLine(const String& text) {
    AppendLine(L"Система", text, false);
}

void ChatForm::SendNudgeNow(void) {
    if (pConnection == null) return;

    pConnection->SendNudge(contactEmail);
    AppendLine(ChatHistory::SELF_LABEL, L"Будильник відправлено!", true);
}

void ChatForm::OnActionPerformed(const Control& source, int actionId) {
    switch (actionId) {
        case ID_BTN_SEND: {
            if (pInputField == null || pConnection == null) break;

            String text = pInputField->GetText();
            text.Trim();
            if (text.IsEmpty()) break;

            pConnection->SendMessageTo(contactEmail, text);
            AppendLine(ChatHistory::SELF_LABEL, text, true);

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
            pMenu->AddItem(L"Розбудити", ID_MENU_NUDGE);
            pMenu->AddItem(L"Профіль", ID_MENU_INFO);
            pMenu->AddItem(L"Очистити історію", ID_MENU_CLEAR_HISTORY);
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
            if (pHistoryArea != null) {
                pHistoryArea->SetText(L"");
                RedrawHistory();
            }
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
        AppendLine(contactName, L"ВАМ НАДІСЛАНО БУДИЛЬНИК!", false);
    } else if (!text.IsEmpty()) {
        AppendLine(contactName, text, false);
    }
}

void ChatForm::OnMessageDeliveryStatus(unsigned long status) {
    switch (status) {
        case Mrim::Delivery::SUCCESS:
            break;
        case Mrim::Delivery::NO_USER:
            ShowSystemLine(L"Користувача не знайдено на сервері.");
            break;
        case Mrim::Delivery::OFFLINE_DISABLED:
            ShowSystemLine(L"Отримувач офлайн (повідомлення не доставлено).");
            break;
        case Mrim::Delivery::OFFLINE_LIMIT:
            ShowSystemLine(L"Перевищено ліміт офлайн-повідомлень.");
            break;
        case Mrim::Delivery::TOO_LARGE:
            ShowSystemLine(L"Повідомлення завелике.");
            break;
        case Mrim::Delivery::INTERNAL_ERROR:
            ShowSystemLine(L"Помилка сервера під час доставки.");
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

    SetTitleText(contactName + L" (друкує...)");
    if (GetParent() != null) {
        RequestRedraw(true);
        Draw();
        Show();
    }
}
