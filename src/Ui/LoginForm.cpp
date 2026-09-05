#include "Ui/LoginForm.h"
#include "Ui/FormNavigator.h"
#include "Core/AppSettings.h"
#include "AGG.h"

using namespace Osp::Base;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;

LoginForm::LoginForm(void) :
    pEditEmail(null), pEditPassword(null), pConnection(null) {}

LoginForm::~LoginForm(void) {
    // Не null лише якщо вхід так і не відбувся - тоді з'єднання наше.
    delete pConnection;
    pConnection = null;
}

result LoginForm::Initialize(AggConnection* pExisting) {
    if (pExisting != null) {
        // Повернення сюди після виходу: чистимо стан сеансу й слухачів,
        // але лишаємо сам об'єкт - сокет перестворить ConnectToServer().
        pExisting->Reset();
        pConnection = pExisting;
    }
    return Construct(L"IDF_AUTH");
}

result LoginForm::OnInitializing(void) {
    if (pConnection == null) {
        pConnection = new AggConnection();
        pConnection->Construct();
        AGG::AttachForcedLogoutHandler(pConnection);
    }

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

    // Підставляємо збережене, щоб не набирати вручну.
    String savedEmail = AppSettings::GetUserEmail();
    String savedPassword = AppSettings::GetUserPassword();
    if (pEditEmail != null && !savedEmail.IsEmpty()) pEditEmail->SetText(savedEmail);
    if (pEditPassword != null && !savedPassword.IsEmpty()) pEditPassword->SetText(savedPassword);

    return E_SUCCESS;
}

result LoginForm::OnTerminating(void) {
    return E_SUCCESS;
}

void LoginForm::OnActionPerformed(const Control& source, int actionId) {
    switch (actionId) {
        case ID_BTN_LOGIN: {
            if (pConnection == null) break;

            String email = (pEditEmail != null) ? pEditEmail->GetText() : String(L"");
            String password = (pEditPassword != null) ? pEditPassword->GetText() : String(L"");
            email.Trim();
            password.Trim();

            if (email.IsEmpty() || password.IsEmpty()) {
                MessageBox msgBox;
                msgBox.Construct(L"Увага", L"Будь ласка, введіть e-mail та пароль!", MSGBOX_STYLE_OK);
                int modalResult = 0;
                msgBox.ShowAndWait(modalResult);
                break;
            }

            AppSettings::SaveCredentials(email, password);

            pConnection->SetCredentials(email, password);
            pConnection->SetLoginListener(this);
            pConnection->ConnectToServer(AppSettings::GetServerIp(), AppSettings::GetServerPort());
            break;
        }

        case ID_BTN_SIGNUP: {
            MessageBox msgBox;
            msgBox.Construct(L"Реєстрація",
                             L"Реєстрація нових акаунтів доступна на офіційному сайті Mail.Ru.",
                             MSGBOX_STYLE_OK);
            int modalResult = 0;
            msgBox.ShowAndWait(modalResult);
            break;
        }

        case ID_BTN_SETTINGS: {
            // null - цю форму не прибираємо: налаштування лягають зверху,
            // а "Назад" повертає сюди ж.
            FormNavigator::GoToSettings(null, null);
            break;
        }

        default:
            break;
    }
}

void LoginForm::OnLoginSuccess(void) {
    AppLog("Успішний вхід! Переходимо на список контактів...");

    AggConnection* pConn = pConnection;
    pConn->SetLoginListener(null);

    // Володіння з'єднанням переходить далі - інакше наш деструктор
    // (спрацює при видаленні форми) знищив би живий сокет.
    pConnection = null;

    FormNavigator::GoToContactList(pConn, this);
}

void LoginForm::OnLoginFailed(const String& reason) {
    AppLog("Авторизація не вдалася: %S", reason.GetPointer());

    // Пароль явно відхилено - прибираємо його, щоб автологін не бився
    // об ту саму стіну при кожному запуску.
    if (reason.StartsWith(L"Вхід відхилено", 0) || reason.StartsWith(L"Невірний логін", 0)) {
        AppSettings::ClearPassword();
    }

    MessageBox msgBox;
    String errorMsg = reason.IsEmpty()
        ? String(L"Не вдалося авторизуватися. Перевірте з'єднання з інтернетом або налаштування.")
        : reason;
    msgBox.Construct(L"Помилка авторизації", errorMsg, MSGBOX_STYLE_OK);
    int modalResult = 0;
    msgBox.ShowAndWait(modalResult);
}
