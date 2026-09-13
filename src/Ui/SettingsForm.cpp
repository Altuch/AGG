#include "Ui/SettingsForm.h"
#include "Ui/FormNavigator.h"
#include "Core/AppSettings.h"
#include "MRIM/MrimUtils.h"

using namespace Osp::Base;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;

SettingsForm::SettingsForm(void) :
    pEditIp(null), pEditPort(null), pEditAvatarIp(null), pEditAvatarPort(null),
    pConnection(null), pReturnTo(null) {}

SettingsForm::~SettingsForm(void) {}

result SettingsForm::Initialize(AggConnection* pConn, Form* pReturn) {
    pConnection = pConn;
    pReturnTo = pReturn;
    return Construct(L"IDF_SETTINGS");
}

result SettingsForm::OnInitializing(void) {
    pEditIp = static_cast<EditField*>(GetControl(L"IDC_EDIT_IP"));
    pEditPort = static_cast<EditField*>(GetControl(L"IDC_EDIT_PORT"));
    pEditAvatarIp = static_cast<EditField*>(GetControl(L"IDC_AVATAR_IP"));
    pEditAvatarPort = static_cast<EditField*>(GetControl(L"IDC_AVATAR_PORT"));

    SetSoftkeyActionId(SOFTKEY_0, ID_SOFTKEY_BACK);
    AddSoftkeyActionListener(SOFTKEY_0, *this);

    SetSoftkeyActionId(SOFTKEY_1, ID_SOFTKEY_SAVE);
    AddSoftkeyActionListener(SOFTKEY_1, *this);

    Button* pBtnDefault = static_cast<Button*>(GetControl(L"IDC_BUTTON_DEFAULT"));
    if (pBtnDefault != null) {
        pBtnDefault->SetActionId(ID_BTN_DEFAULT);
        pBtnDefault->AddActionEventListener(*this);
    }

    if (pEditIp != null) pEditIp->SetText(AppSettings::GetServerIp());
    if (pEditPort != null) pEditPort->SetText(Integer::ToString(AppSettings::GetServerPort()));
    if (pEditAvatarIp != null) pEditAvatarIp->SetText(AppSettings::GetAvatarHost());
    if (pEditAvatarPort != null) pEditAvatarPort->SetText(Integer::ToString(AppSettings::GetAvatarPort()));

    return E_SUCCESS;
}

static bool ReadPortField(EditField* pField, int& outPort, const wchar_t* pErrTitle, const wchar_t* pErrText) {
    String portStr = (pField != null) ? pField->GetText() : String(L"");
    portStr.Trim();

    outPort = 0;
    if (IsFailed(Integer::Parse(portStr, outPort)) || outPort <= 0 || outPort > 65535) {
        MessageBox msgBox;
        msgBox.Construct(pErrTitle, pErrText, MSGBOX_STYLE_OK);
        int modalResult = 0;
        msgBox.ShowAndWait(modalResult);
        return false;
    }
    return true;
}

bool SettingsForm::ReadAndValidate(String& outIp, int& outPort,
                                   String& outAvatarIp, int& outAvatarPort) {
    outIp = (pEditIp != null) ? pEditIp->GetText() : AppSettings::GetServerIp();
    outIp.Trim();

    if (!MrimUtils::IsValidHost(outIp)) {
        MessageBox msgBox;
        msgBox.Construct(L"Помилка", L"Некоректна адреса сервера!\nПриклад: 103.71.21.140 або mrim.su", MSGBOX_STYLE_OK);
        int modalResult = 0;
        msgBox.ShowAndWait(modalResult);
        return false;
    }

    if (!ReadPortField(pEditPort, outPort, L"Помилка", L"Некоректний порт!\nВведіть число від 1 до 65535.")) return false;

    outAvatarIp = (pEditAvatarIp != null) ? pEditAvatarIp->GetText() : AppSettings::GetAvatarHost();
    outAvatarIp.Trim();
    if (!outAvatarIp.IsEmpty() && !MrimUtils::IsValidHost(outAvatarIp)) {
        MessageBox msgBox;
        msgBox.Construct(L"Помилка", L"Некоректна адреса сервера аватарок!\nЗалиште порожнім, щоб використовувати адресу MRIM.", MSGBOX_STYLE_OK);
        int modalResult = 0;
        msgBox.ShowAndWait(modalResult);
        return false;
    }

    if (pEditAvatarPort != null) {
        if (!ReadPortField(pEditAvatarPort, outAvatarPort, L"Помилка", L"Некоректний порт аватарок!\nВведіть число від 1 до 65535.")) return false;
    } else {
        outAvatarPort = AppSettings::GetAvatarPort();
    }

    return true;
}

void SettingsForm::ApplyDefaults(void) {
    String ip = AppSettings::GetDefaultServerIp();
    int port = AppSettings::GetDefaultServerPort();

    AppSettings::SaveServer(ip, port);
    AppSettings::SaveAvatarServer(L"", 8081);

    if (pEditIp != null) {
        pEditIp->SetText(ip);
        pEditIp->RequestRedraw(true);
    }
    if (pEditPort != null) {
        pEditPort->SetText(Integer::ToString(port));
        pEditPort->RequestRedraw(true);
    }
    if (pEditAvatarIp != null) {
        pEditAvatarIp->SetText(AppSettings::GetAvatarHost());
        pEditAvatarIp->RequestRedraw(true);
    }
    if (pEditAvatarPort != null) {
        pEditAvatarPort->SetText(Integer::ToString(AppSettings::GetAvatarPort()));
        pEditAvatarPort->RequestRedraw(true);
    }

    MessageBox msgBox;
    msgBox.Construct(L"Налаштування", L"Параметри скинуто до значень за замовчуванням!", MSGBOX_STYLE_OK);
    int modalResult = 0;
    msgBox.ShowAndWait(modalResult);
}

void SettingsForm::Leave(void) {
    if (pReturnTo != null) {
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
            String avatarIp;
            int avatarPort = 8081;
            if (!ReadAndValidate(ip, port, avatarIp, avatarPort)) break;

            AppSettings::SaveServer(ip, port);
            AppSettings::SaveAvatarServer(avatarIp, avatarPort);

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
