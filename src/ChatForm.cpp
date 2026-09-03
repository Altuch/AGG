#include "ChatForm.h"
#include "ContactListForm.h"
#include <FApp.h>
#include <FGraphics.h>

// Константи статусів доставки протоколу MRIM
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

ChatForm::ChatForm(void) :
    pConnection(null),
    pCustomListHistory(null),
    pItemFormat(null),
    pEditInput(null),
    pBtnNudge(null) {}

ChatForm::~ChatForm(void) {
    if (pConnection != null) {
        pConnection->SetMessageListener(null);
    }
    if (pItemFormat != null) {
        delete pItemFormat;
        pItemFormat = null;
    }
}

result ChatForm::Initialize(AggConnection* pConn, const String& name, const String& email) {
    pConnection = pConn;
    contactName = name.IsEmpty() ? email : name;
    contactEmail = email;
    contactEmail.Trim();
    contactEmail.ToLower(); // Примусовий чистий lowercase

    if (pConnection != null) {
        pConnection->SetMessageListener(this);
    }

    return Form::Construct(L"IDF_CHAT");
}

result ChatForm::OnInitializing(void) {
    SetTitleText(contactName);

    SetSoftkeyActionId(SOFTKEY_0, ID_SOFTKEY_BACK);
    AddSoftkeyActionListener(SOFTKEY_0, *this);

    SetSoftkeyActionId(SOFTKEY_1, ID_BTN_SEND);
    AddSoftkeyActionListener(SOFTKEY_1, *this);

    pCustomListHistory = static_cast<CustomList*>(GetControl(L"IDC_HISTORY"));
    if (pCustomListHistory != null) {
        int listWidth = pCustomListHistory->GetWidth();
        if (listWidth <= 0) listWidth = 460;

        pItemFormat = new CustomListItemFormat();
        pItemFormat->Construct();
        pItemFormat->AddElement(1, Rectangle(10, 5, listWidth - 20, 25));
        pItemFormat->AddElement(2, Rectangle(10, 30, listWidth - 20, 45));
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

    return E_SUCCESS;
}

result ChatForm::OnTerminating(void) {
    return E_SUCCESS;
}

void ChatForm::AppendMessageToChat(const String& senderTitle, const String& text, bool isIncoming) {
    if (pCustomListHistory == null || pItemFormat == null) return;

    CustomListItem* pItem = new CustomListItem();
    pItem->Construct(80);
    pItem->SetItemFormat(*pItemFormat);

    String header = isIncoming ? senderTitle : L"Ви";

    pItem->SetElement(1, header);
    pItem->SetElement(2, text);

    pCustomListHistory->AddItem(*pItem);
    pCustomListHistory->ScrollToBottom();

    if (GetParent() != null) {
        pCustomListHistory->RequestRedraw(true);
        pCustomListHistory->Draw();
        pCustomListHistory->Show();
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
            if (pConnection == null) break;

            pConnection->SendNudge(contactEmail);
            AppendMessageToChat(L"Ви", L"🔔 Будильник відправлено!", false);
            break;
        }

        case ID_SOFTKEY_BACK: {
            if (pConnection != null) {
                pConnection->SetMessageListener(null);
            }

            ContactListForm* pContactForm = new ContactListForm();
            pContactForm->Initialize(pConnection);

            if (pFrame != null) {
                pFrame->AddControl(*pContactForm);
                pFrame->SetCurrentForm(*pContactForm);
                pContactForm->Draw();
                pContactForm->Show();
                pContactForm->AttachContactListener();
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
            AppendMessageToChat(contactName, L"🔔 ВАМ НАДІСЛАНО БУДИЛЬНИК!", true);
        } else if (!text.IsEmpty()) {
            AppendMessageToChat(contactName, text, true);
        }
    }
}

void ChatForm::OnMessageDeliveryStatus(unsigned long status) {
    if (status == MRIM_DELIVERY_STATUS_OK) {
        return; // Доставлено успішно — попереджень не потрібно
    } else if (status == MRIM_DELIVERY_STATUS_USER_NOT_FOUND) {
        AppendMessageToChat(L"Система", L"⚠ Користувача не знайдено на сервері.", true);
    } else if (status == MRIM_DELIVERY_STATUS_NO_OFFLINE) {
        AppendMessageToChat(L"Система", L"⚠ Отримувач офлайн (повідомлення не доставлено).", true);
    } else if (status == MRIM_DELIVERY_STATUS_OFFLINE_LIMIT) {
        AppendMessageToChat(L"Система", L"⚠ Перевищено ліміт офлайн-повідомлень.", true);
    } else if (status == MRIM_DELIVERY_STATUS_SERVER_ERROR) {
        AppendMessageToChat(L"Система", L"⚠ Помилка сервера під час доставки.", true);
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
