#include "Settings.h"
#include "Form1.h"
#include "ContactListForm.h"
#include "MRIM/MrimUtils.h"

using namespace Osp::Ui;
using namespace Osp::Ui::Controls;
using namespace Osp::App;
using namespace Osp::Base;
using namespace Osp::Base::Utility;

Settings::Settings(void) : pEditIp(null), pEditPort(null), pConnection(null) {}
Settings::~Settings(void) {}

bool Settings::Initialize(AggConnection* pConn) {
    pConnection = pConn;
    Construct(L"IDF_SETTINGS");
    return true;
}

result Settings::OnInitializing(void) {
    pEditIp = static_cast<EditField*>(GetControl(L"IDC_EDIT_IP"));
    pEditPort = static_cast<EditField*>(GetControl(L"IDC_EDIT_PORT"));

    SetSoftkeyText(SOFTKEY_0, L"Назад");
    SetSoftkeyActionId(SOFTKEY_0, ID_SOFTKEY_BACK);
    AddSoftkeyActionListener(SOFTKEY_0, *this);

    SetSoftkeyText(SOFTKEY_1, L"Зберегти");
    SetSoftkeyActionId(SOFTKEY_1, ID_SOFTKEY_SAVE);
    AddSoftkeyActionListener(SOFTKEY_1, *this);

    Button* pBtnDefault = static_cast<Button*>(GetControl(L"IDC_BUTTON_DEFAULT"));
    if (pBtnDefault != null) {
        pBtnDefault->SetActionId(ID_BTN_DEFAULT);
        pBtnDefault->AddActionEventListener(*this);
    }

    AppRegistry* pReg = Application::GetInstance()->GetAppRegistry();
    String savedIp = L"103.71.21.140";
    String savedPort = L"2041";

    if (pReg != null) {
        pReg->Get(L"ServerIP", savedIp);
        pReg->Get(L"ServerPort", savedPort);
    }

    if (pEditIp != null) pEditIp->SetText(savedIp);
    if (pEditPort != null) pEditPort->SetText(savedPort);

    return E_SUCCESS;
}

void Settings::ResetToDefault(void) {
    String defaultIp = L"103.71.21.140";
    String defaultPort = L"2041";

    if (pEditIp != null) {
        pEditIp->SetText(defaultIp);
        pEditIp->RequestRedraw(true);
    }
    if (pEditPort != null) {
        pEditPort->SetText(defaultPort);
        pEditPort->RequestRedraw(true);
    }

    AppRegistry* pReg = Application::GetInstance()->GetAppRegistry();
    if (pReg != null) {
        String dummy;
        if (pReg->Get(L"ServerIP", dummy) == E_SUCCESS) {
            pReg->Set(L"ServerIP", defaultIp);
        } else {
            pReg->Add(L"ServerIP", defaultIp);
        }

        if (pReg->Get(L"ServerPort", dummy) == E_SUCCESS) {
            pReg->Set(L"ServerPort", defaultPort);
        } else {
            pReg->Add(L"ServerPort", defaultPort);
        }
        pReg->Save();
    }

    MessageBox msgBox;
    msgBox.Construct(L"Налаштування", L"Параметри скинуто до значень за замовчуванням!", MSGBOX_STYLE_OK);
    int modalResult = 0;
    msgBox.ShowAndWait(modalResult);
}

void Settings::ReturnFromSettings(void) {
    Frame* pFrame = Application::GetInstance()->GetAppFrame()->GetFrame();
    if (pFrame == null) return;

    if (pConnection != null) {
        ContactListForm* pContactForm = new ContactListForm();
        pContactForm->Initialize(pConnection);

        pFrame->AddControl(*pContactForm);
        pFrame->SetCurrentForm(*pContactForm);
        pContactForm->Draw();
        pContactForm->Show();
        pContactForm->ScheduleAttachContactListener();
        pFrame->RemoveControl(*this);
    } else {
        Form1* pForm1 = new Form1();
        pForm1->Initialize();

        pFrame->AddControl(*pForm1);
        pFrame->SetCurrentForm(*pForm1);
        pForm1->Draw();
        pForm1->Show();
        pFrame->RemoveControl(*this);
    }
}

void Settings::OnActionPerformed(const Control& source, int actionId) {
    switch (actionId) {
        case ID_SOFTKEY_SAVE: {
            String ip = (pEditIp != null) ? pEditIp->GetText() : L"103.71.21.140";
            String portStr = (pEditPort != null) ? pEditPort->GetText() : L"2041";

            ip.Trim();
            portStr.Trim();

            if (!MrimUtils::IsValidIpAddress(ip)) {
                MessageBox msgBox;
                msgBox.Construct(L"Помилка", L"Некоректний формат IP-адреси!\nПриклад: 103.71.21.140", MSGBOX_STYLE_OK);
                int modalResult = 0;
                msgBox.ShowAndWait(modalResult);
                break;
            }

            int testPort = 0;
            result rPort = Integer::Parse(portStr, testPort);
            if (IsFailed(rPort) || testPort <= 0 || testPort > 65535) {
                MessageBox msgBox;
                msgBox.Construct(L"Помилка", L"Некоректний порт!\nВведіть число від 1 до 65535.", MSGBOX_STYLE_OK);
                int modalResult = 0;
                msgBox.ShowAndWait(modalResult);
                break;
            }

            AppRegistry* pReg = Application::GetInstance()->GetAppRegistry();
            if (pReg != null) {
                String dummy;
                if (pReg->Get(L"ServerIP", dummy) == E_SUCCESS) {
                    pReg->Set(L"ServerIP", ip);
                } else {
                    pReg->Add(L"ServerIP", ip);
                }

                if (pReg->Get(L"ServerPort", dummy) == E_SUCCESS) {
                    pReg->Set(L"ServerPort", portStr);
                } else {
                    pReg->Add(L"ServerPort", portStr);
                }
                pReg->Save();
            }

            MessageBox msgBox;
            msgBox.Construct(L"Успіх", L"Налаштування сервера успішно збережено!", MSGBOX_STYLE_OK);
            int modalResult = 0;
            msgBox.ShowAndWait(modalResult);

            ReturnFromSettings();
            break;
        }

        case ID_SOFTKEY_BACK: {
        	ReturnFromSettings();
            break;
        }

        case ID_BTN_DEFAULT: {
            ResetToDefault();
            break;
        }
    }
}
