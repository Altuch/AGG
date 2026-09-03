#include "Form1.h"
#include "Settings.h"
#include "ContactListForm.h"
#include <FApp.h>
#include <FBase.h>

using namespace Osp::Ui;
using namespace Osp::Ui::Controls;
using namespace Osp::App;
using namespace Osp::Base;

Form1::Form1(void) : pEditEmail(null), pEditPassword(null), pConnection(null) {}

Form1::~Form1(void) {
    if (pConnection != null) {
        delete pConnection;
        pConnection = null;
    }
}

bool Form1::Initialize(void) {
    Construct(L"IDF_AUTH");
    return true;
}

result Form1::OnInitializing(void) {
    pConnection = new AggConnection();
    pConnection->Construct();

    Button* pBtnLogin = static_cast<Button*>(GetControl(L"IDC_BUTTON_LOGIN"));
    if (pBtnLogin != null) {
        pBtnLogin->SetActionId(ID_BTN_LOGIN);
        pBtnLogin->AddActionEventListener(*this);
    }

    Button* pBtnSignup = static_cast<Button*>(GetControl(L"IDC_BUTTON_SIGNUP"));
    if (pBtnSignup != null) {
        pBtnSignup->SetActionId(ID_BTN_SIGNUP);
        pBtnSignup->AddActionEventListener(*this);
    }

    Button* pBtnSettings = static_cast<Button*>(GetControl(L"IDC_BUTTON_SETTINGS"));
    if (pBtnSettings != null) {
        pBtnSettings->SetActionId(ID_BTN_SETTINGS);
        pBtnSettings->AddActionEventListener(*this);
    }

    pEditEmail = static_cast<EditField*>(GetControl(L"IDC_EDITFIELD_EMAIL"));
    pEditPassword = static_cast<EditField*>(GetControl(L"IDC_EDITFIELD_PASSWORD"));

    AppRegistry* pReg = Application::GetInstance()->GetAppRegistry();
    if (pReg != null) {
        String savedEmail = L"";
        String savedPassword = L"";

        pReg->Get(L"UserEmail", savedEmail);
        pReg->Get(L"UserPassword", savedPassword);

        if (pEditEmail != null && !savedEmail.IsEmpty()) {
            pEditEmail->SetText(savedEmail);
        }
        if (pEditPassword != null && !savedPassword.IsEmpty()) {
            pEditPassword->SetText(savedPassword);
        }
    }

    return E_SUCCESS;
}

result Form1::OnTerminating(void) {
    return E_SUCCESS;
}

void Form1::OnActionPerformed(const Control& source, int actionId) {
    switch(actionId) {
        case ID_BTN_LOGIN: {
            AppLog("Натиснуто Вхід!");

            String email = L"";
            String password = L"";

            if (pEditEmail != null) email = pEditEmail->GetText();
            if (pEditPassword != null) password = pEditPassword->GetText();

            email.Trim();
            password.Trim();

            if (email.IsEmpty() || password.IsEmpty()) {
                MessageBox msgBox;
                msgBox.Construct(L"Увага", L"Будь ласка, введіть e-mail та пароль!", MSGBOX_STYLE_OK);
                int modalResult = 0;
                msgBox.ShowAndWait(modalResult);
                break;
            }

            // Збереження даних у реєстр
            AppRegistry* pReg = Application::GetInstance()->GetAppRegistry();
            if (pReg != null) {
                String dummy;
                if (pReg->Get(L"UserEmail", dummy) == E_SUCCESS) {
                    pReg->Set(L"UserEmail", email);
                } else {
                    pReg->Add(L"UserEmail", email);
                }

                if (pReg->Get(L"UserPassword", dummy) == E_SUCCESS) {
                    pReg->Set(L"UserPassword", password);
                } else {
                    pReg->Add(L"UserPassword", password);
                }
                pReg->Save();
            }

            String serverIp = L"103.71.21.140";
            String serverPortStr = L"2041";
            int serverPort = 2041;

            if (pReg != null) {
                pReg->Get(L"ServerIP", serverIp);
                pReg->Get(L"ServerPort", serverPortStr);
                Integer::Parse(serverPortStr, serverPort);
            }

            // Передаємо логін і пароль у з'єднання
            pConnection->userLogin = email;
            pConnection->userPassword = password;
            pConnection->SetLoginListener(this);

            // Підключаємося до сервера
            pConnection->ConnectToDirectServer(serverIp, serverPort);
            break;
        }

        case ID_BTN_SIGNUP: {
            MessageBox msgBox;
            msgBox.Construct(L"Реєстрація", L"Реєстрація нових акаунтів доступна на офіційному сайті Mail.Ru.", MSGBOX_STYLE_OK);
            int modalResult = 0;
            msgBox.ShowAndWait(modalResult);
            break;
        }

        case ID_BTN_SETTINGS: {
            Settings* pSettingsForm = new Settings();
            pSettingsForm->Initialize();

            Frame* pFrame = Application::GetInstance()->GetAppFrame()->GetFrame();
            if (pFrame != null) {
                pFrame->AddControl(*pSettingsForm);
                pFrame->SetCurrentForm(*pSettingsForm);
                pSettingsForm->Draw();
                pSettingsForm->Show();
            }
            break;
        }
    }
}

void Form1::OnLoginSuccess(void) {
    AppLog("Успішний вхід! Переходимо на контакт-лист...");

    if (pConnection != null) {
        pConnection->SetLoginListener(null);
    }

    ContactListForm* pContactForm = new ContactListForm();
    pContactForm->Initialize(pConnection);

    pConnection = null;

    Frame* pFrame = Application::GetInstance()->GetAppFrame()->GetFrame();
    if (pFrame != null) {
        pFrame->AddControl(*pContactForm);
        pFrame->SetCurrentForm(*pContactForm);
        pContactForm->Draw();
        pContactForm->Show();
<<<<<<< Updated upstream
        pContactForm->AttachContactListener();
=======
        pContactForm->ScheduleAttachContactListener();
>>>>>>> Stashed changes
        pFrame->RemoveControl(*this);
    }
}

void Form1::OnLoginFailed(const String& reason) {
    AppLog("Авторизація не вдалася: %S", reason.GetPointer());

    if (reason.StartsWith(L"Вхід відхилено", 0) || reason.StartsWith(L"Невірний логін", 0)) {
        AppRegistry* pReg = Application::GetInstance()->GetAppRegistry();
        if (pReg != null) {
            pReg->Remove(L"UserPassword");
            pReg->Save();
        }
    }

    MessageBox msgBox;
    String errorMsg = reason.IsEmpty() ? L"Не вдалося авторизуватися. Перевірте з'єднання з інтернетом або налаштування." : reason;
    msgBox.Construct(L"Помилка авторизації", errorMsg, MSGBOX_STYLE_OK);
    int modalResult = 0;
    msgBox.ShowAndWait(modalResult);
}
