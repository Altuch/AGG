#include "Ui/SettingsForm.h"
#include "Ui/FormNavigator.h"
#include "Core/AppSettings.h"
#include "MRIM/MrimUtils.h"

using namespace Osp::Base;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;

SettingsForm::SettingsForm(void) :
    pEditIp(null), pEditPort(null), pConnection(null), pReturnTo(null) {}

SettingsForm::~SettingsForm(void) {}

result SettingsForm::Initialize(AggConnection* pConn, Form* pReturn) {
    pConnection = pConn;
    pReturnTo = pReturn;
    return Construct(L"IDF_SETTINGS");
}

result SettingsForm::OnInitializing(void) {
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

    if (pEditIp != null) pEditIp->SetText(AppSettings::GetServerIp());
    if (pEditPort != null) pEditPort->SetText(Integer::ToString(AppSettings::GetServerPort()));

    return E_SUCCESS;
}

bool SettingsForm::ReadAndValidate(String& outIp, int& outPort) {
    outIp = (pEditIp != null) ? pEditIp->GetText() : AppSettings::GetDefaultServerIp();
    String portStr = (pEditPort != null) ? pEditPort->GetText() : String(L"");
    outIp.Trim();
    portStr.Trim();

    if (!MrimUtils::IsValidIpAddress(outIp)) {
        MessageBox msgBox;
        msgBox.Construct(L"Помилка", L"Некоректний формат IP-адреси!\nПриклад: 103.71.21.140", MSGBOX_STYLE_OK);
        int modalResult = 0;
        msgBox.ShowAndWait(modalResult);
        return false;
    }

    outPort = 0;
    if (IsFailed(Integer::Parse(portStr, outPort)) || outPort <= 0 || outPort > 65535) {
        MessageBox msgBox;
        msgBox.Construct(L"Помилка", L"Некоректний порт!\nВведіть число від 1 до 65535.", MSGBOX_STYLE_OK);
        int modalResult = 0;
        msgBox.ShowAndWait(modalResult);
        return false;
    }

    return true;
}

void SettingsForm::ApplyDefaults(void) {
    String ip = AppSettings::GetDefaultServerIp();
    int port = AppSettings::GetDefaultServerPort();

    AppSettings::SaveServer(ip, port);

    if (pEditIp != null) {
        pEditIp->SetText(ip);
        pEditIp->RequestRedraw(true);
    }
    if (pEditPort != null) {
        pEditPort->SetText(Integer::ToString(port));
        pEditPort->RequestRedraw(true);
    }

    MessageBox msgBox;
    msgBox.Construct(L"Налаштування", L"Параметри скинуто до значень за замовчуванням!", MSGBOX_STYLE_OK);
    int modalResult = 0;
    msgBox.ShowAndWait(modalResult);
}

void SettingsForm::Leave(void) {
    if (pReturnTo != null) {
        // Форма, з якої прийшли, ще жива під нами - просто повертаємось
        // до неї. Раніше тут створювалась ЩЕ ОДНА форма входу, і стара
        // лишалась висіти у Frame назавжди.
        FormNavigator::Back(pReturnTo, this);
        return;
    }

    if (pConnection != null) {
        FormNavigator::GoToContactList(pConnection, this);
    } else {
        FormNavigator::GoToLogin(null, this);
    }
}

void SettingsForm::OnActionPerformed(const Control& source, int actionId) {
    switch (actionId) {
        case ID_SOFTKEY_SAVE: {
            String ip;
            int port = 0;
            if (!ReadAndValidate(ip, port)) break;

            AppSettings::SaveServer(ip, port);

            MessageBox msgBox;
            msgBox.Construct(L"Успіх", L"Налаштування сервера успішно збережено!", MSGBOX_STYLE_OK);
            int modalResult = 0;
            msgBox.ShowAndWait(modalResult);

            Leave();
            break;
        }

        case ID_SOFTKEY_BACK: {
            Leave();
            break;
        }

        case ID_BTN_DEFAULT: {
            ApplyDefaults();
            break;
        }

        default:
            break;
    }
}
