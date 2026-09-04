#include "ChatForm.h"
#include "ContactListForm.h"
#include "MRIM/MrimUtils.h"
#include <FApp.h>
#include <FGraphics.h>
#include <FIo.h>

#define MRIM_DELIVERY_STATUS_OK             0x0000
#define MRIM_DELIVERY_STATUS_USER_NOT_FOUND 0x8001
#define MRIM_DELIVERY_STATUS_SERVER_ERROR   0x8003
#define MRIM_DELIVERY_STATUS_OFFLINE_LIMIT  0x8004
#define MRIM_DELIVERY_STATUS_TOO_LARGE      0x8005
#define MRIM_DELIVERY_STATUS_NO_OFFLINE     0x8006

using namespace Osp::Ui;
using namespace Osp::Ui::Controls;
using namespace Osp::App;
using namespace Osp::Base;
using namespace Osp::Base::Collection;
using namespace Osp::Graphics;
using namespace Osp::System;
using namespace Osp::Io;

ChatForm::ChatForm(void) :
    pConnection(null),
    pEditAreaHistory(null),
    pEditInput(null),
    pBtnNudge(null) {}

ChatForm::~ChatForm(void) {
    if (pConnection != null) {
        pConnection->ClearActiveChatListener();
    }
}

result ChatForm::Initialize(AggConnection* pConn, const String& name, const String& email) {
    pConnection = pConn;
    contactName = name.IsEmpty() ? email : name;
    contactEmail = email;
    contactEmail.Trim();
    contactEmail.ToLower();

    if (pConnection != null) {
        pConnection->SetActiveChatListener(this, contactEmail);
    }

    return Form::Construct(L"IDF_CHAT");
}

result ChatForm::OnInitializing(void) {
    SetTitleText(contactName);

    SetSoftkeyActionId(SOFTKEY_0, ID_SOFTKEY_BACK);
    AddSoftkeyActionListener(SOFTKEY_0, *this);

    SetSoftkeyActionId(SOFTKEY_1, ID_BTN_SEND);
    AddSoftkeyActionListener(SOFTKEY_1, *this);

    SetOptionkeyActionId(ID_OPTIONKEY_CHAT);
    AddOptionkeyActionListener(*this);

    pEditAreaHistory = static_cast<EditArea*>(GetControl(L"IDC_HISTORY"));
    if (pEditAreaHistory != null) {
        pEditAreaHistory->SetKeypadEnabled(false);
    }

    pEditInput = static_cast<EditField*>(GetControl(L"IDC_TEXT"));
    if (pEditInput != null) {
        pEditInput->AddTextEventListener(*this);
    }

    pBtnNudge = static_cast<Button*>(GetControl(L"IDC_BUTTON_NUDGE"));
    if (pBtnNudge != null) {
        pBtnNudge->SetActionId(ID_BTN_NUDGE);
        pBtnNudge->AddActionEventListener(*this);
    }

    LoadHistory();

    return E_SUCCESS;
}

result ChatForm::OnTerminating(void) {
    return E_SUCCESS;
}

void ChatForm::AppendMessageToChat(const String& senderTitle, const String& text, bool isIncoming, bool saveToHistory) {
    if (pEditAreaHistory == null) return;

    String header = isIncoming ? senderTitle : L"Ви";

    String entry = header + L": " + text + L"\n\n";
    pEditAreaHistory->AppendText(entry);

    if (GetParent() != null) {
        pEditAreaHistory->RequestRedraw(true);
        pEditAreaHistory->Draw();
        pEditAreaHistory->Show();
    }

    if (saveToHistory) {
        SaveMessageToHistory(header, text);
    }
}

void ChatForm::SendNudgeNow(void) {
    if (pConnection == null) return;
    pConnection->SendNudge(contactEmail);
    AppendMessageToChat(L"Ви", L"🔔 Будильник відправлено!", false);
}

String ChatForm::GetHistoryFilePath(void) {
    return MrimUtils::GetHistoryFilePath(contactEmail);
}

void ChatForm::SaveMessageToHistory(const String& sender, const String& text) {
    if (contactEmail.IsEmpty()) return;
    String path = GetHistoryFilePath();

    File file;
    result r = file.Construct(path, L"a+");
    if (IsFailed(r)) {
        r = file.Construct(path, L"w");
        if (IsFailed(r)) return;
    }

    String line = sender + L"\t" + text + L"\n";
    file.Write(line);
}

void ChatForm::LoadHistory(void) {
    if (contactEmail.IsEmpty()) return;
    String path = GetHistoryFilePath();

    File file;
    result r = file.Construct(path, L"r");
    if (IsFailed(r)) return;

    String ringBuffer[MAX_LOADED_HISTORY_LINES];
    int ringCount = 0;
    int ringNext = 0;

    while (true) {
        String line;
        r = file.Read(line);
        if (IsFailed(r)) break; // E_END_OF_FILE

        line.Trim();
        if (line.IsEmpty()) continue;

        ringBuffer[ringNext] = line;
        ringNext = (ringNext + 1) % MAX_LOADED_HISTORY_LINES;
        if (ringCount < MAX_LOADED_HISTORY_LINES) ringCount++;
    }

    int startIndex = (ringCount < MAX_LOADED_HISTORY_LINES) ? 0 : ringNext;
    for (int i = 0; i < ringCount; i++) {
        String& line = ringBuffer[(startIndex + i) % MAX_LOADED_HISTORY_LINES];

        int tabPos = -1;
        line.IndexOf(L"\t", 0, tabPos);
        if (tabPos < 0) continue;

        String sender;
        String text;
        line.SubString(0, tabPos, sender);
        line.SubString(tabPos + 1, text);

        bool isIncoming = !sender.Equals(L"Ви", true);
        AppendMessageToChat(sender, text, isIncoming, false);
    }
}

void ChatForm::OnActionPerformed(const Control& source, int actionId) {
    Frame* pFrame = Application::GetInstance()->GetAppFrame()->GetFrame();

    switch (actionId) {
        case ID_BTN_SEND: {
            if (pEditInput == null || pConnection == null) break;

            String text = pEditInput->GetText();
            text.Trim();
            if (text.IsEmpty()) break;

            pConnection->SendMessageTo(contactEmail, text);
            AppendMessageToChat(L"Ви", text, false);

            pEditInput->SetText(L"");
            if (GetParent() != null) {
                pEditInput->RequestRedraw(true);
                pEditInput->Draw();
                pEditInput->Show();
            }
            break;
        }

        case ID_BTN_NUDGE: {
            SendNudgeNow();
            break;
        }

        case ID_OPTIONKEY_CHAT: {
            OptionMenu* pMenu = new OptionMenu();
            pMenu->Construct();
            pMenu->AddItem(L"Розбудити", ID_MENU_NUDGE);
            pMenu->AddItem(L"Інформація про контакт", ID_MENU_INFO);
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
            MessageBox infoBox;
            infoBox.Construct(L"Інформація про контакт", contactName + L"\n" + contactEmail, MSGBOX_STYLE_OK, 0);
            int modalResult = 0;
            infoBox.ShowAndWait(modalResult);
            break;
        }

        case ID_MENU_CLEAR_HISTORY: {
            if (pEditAreaHistory != null) {
                pEditAreaHistory->SetText(L"");
                if (GetParent() != null) {
                    pEditAreaHistory->RequestRedraw(true);
                    pEditAreaHistory->Draw();
                    pEditAreaHistory->Show();
                }
            }
            File::Remove(GetHistoryFilePath());
            break;
        }

        case ID_SOFTKEY_BACK: {
            if (pConnection != null) {
                pConnection->ClearActiveChatListener();
            }

            ContactListForm* pContactForm = new ContactListForm();
            pContactForm->Initialize(pConnection);

            if (pFrame != null) {
                pFrame->AddControl(*pContactForm);
                pFrame->SetCurrentForm(*pContactForm);
                pContactForm->Draw();
                pContactForm->Show();
                pContactForm->ScheduleAttachContactListener();
                pFrame->RemoveControl(*this);
            }
            break;
        }
    }
}

void ChatForm::OnTextValueChanged(const Control& source) {
    if (pConnection != null && !contactEmail.IsEmpty()) {
        pConnection->SendTyping(contactEmail);
    }
}

void ChatForm::OnMessageReceived(const String& sender, const String& text, bool isNudge) {
    String cleanSender = sender;
    cleanSender.Trim();
    cleanSender.ToLower();

    if (cleanSender.Equals(contactEmail, true)) {
        if (isNudge) {
            Vibrator vibrator;
            vibrator.Construct();
            vibrator.Start(1000, 100);
            AppendMessageToChat(contactName, L"🔔 ВАМ НАДІСЛАНО БУДИЛЬНИК!", true, false);
        } else if (!text.IsEmpty()) {
            AppendMessageToChat(contactName, text, true, false);
        }
    }
}

void ChatForm::OnMessageDeliveryStatus(unsigned long status) {
    if (status == MRIM_DELIVERY_STATUS_OK) {
        return;
    } else if (status == MRIM_DELIVERY_STATUS_USER_NOT_FOUND) {
        AppendMessageToChat(L"Система", L"⚠ Користувача не знайдено на сервері.", true, false);
    } else if (status == MRIM_DELIVERY_STATUS_NO_OFFLINE) {
        AppendMessageToChat(L"Система", L"⚠ Отримувач офлайн (повідомлення не доставлено).", true, false);
    } else if (status == MRIM_DELIVERY_STATUS_OFFLINE_LIMIT) {
        AppendMessageToChat(L"Система", L"⚠ Перевищено ліміт офлайн-повідомлень.", true, false);
    } else if (status == MRIM_DELIVERY_STATUS_SERVER_ERROR) {
        AppendMessageToChat(L"Система", L"⚠ Помилка сервера під час доставки.", true, false);
    }
}

void ChatForm::OnTypingReceived(const String& sender) {
    String cleanSender = sender;
    cleanSender.Trim();
    cleanSender.ToLower();

    if (cleanSender.Equals(contactEmail, true)) {
        SetTitleText(contactName + L" (друкує...)");

        if (GetParent() != null) {
            RequestRedraw(true);
            Draw();
            Show();
        }
    }
}

void ChatForm::OnUserEventReceivedN(long requestId, IList* pArgs) {
    if (pArgs != null) {
        pArgs->RemoveAll(true);
        delete pArgs;
    }
}
