#include "Ui/LoginForm.h"
#include "Core/Loc.h"
#include "Ui/FormNavigator.h"
#include "Core/AppSettings.h"
#include "MRIM/MrimUtils.h"
#include "AGG.h"

using namespace Osp::Base;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;

LoginForm::LoginForm(void) :
    pEditEmail(null), pEditPassword(null), pConnection(null) {}

LoginForm::~LoginForm(void) {
    delete pConnection;
    pConnection = null;
}

result LoginForm::Initialize(AggConnection* pExisting) {
    if (pExisting != null) {
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
    if (pBtnLogin != null) pBtnLogin->SetText(LocString(L"IDS_BTN_LOGIN"));
    if (pBtnSignup != null) pBtnSignup->SetText(LocString(L"IDS_BTN_SIGNUP"));
    if (pBtnSettings != null) pBtnSettings->SetText(LocString(L"IDS_BTN_SETTINGS"));
    if (pEditEmail != null) pEditEmail->SetGuideText(LocString(L"IDS_GUIDE_EMAIL"));
    if (pEditPassword != null) pEditPassword->SetGuideText(LocString(L"IDS_GUIDE_PASSWORD"));
    Label* pLblHint = static_cast<Label*>(GetControl(L"IDC_LABEL"));
    if (pLblHint != null) pLblHint->SetText(LocString(L"IDS_LOGIN_HINT"));

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
            email = MrimUtils::NormalizeEmail(email);
            password.Trim();

            if (email.IsEmpty() || password.IsEmpty()) {
                MessageBox msgBox;
                msgBox.Construct(LocString(L"IDS_LOGIN_WARN_TITLE"), LocString(L"IDS_LOGIN_WARN_EMPTY"), MSGBOX_STYLE_OK);
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
            msgBox.Construct(LocString(L"IDS_SIGNUP_TITLE"),
                             LocString(L"IDS_SIGNUP_TEXT"),
                             MSGBOX_STYLE_OK);
            int modalResult = 0;
            msgBox.ShowAndWait(modalResult);
            break;
        }

        case ID_BTN_SETTINGS: {
            FormNavigator::GoToSettings(null, null);
            break;
        }

        default:
            break;
    }
}

void LoginForm::OnLoginSuccess(void) {
    AggConnection* pConn = pConnection;
    pConn->SetLoginListener(null);

    pConnection = null;

    String pendingKind = pConn->PeekPendingNotificationKind();
    String pending = pConn->ConsumePendingNotificationSender();
    if (!pending.IsEmpty()) {
        if (pendingKind == L"BLOG") {
            FormNavigator::GoToMicroblog(pConn, this);
        } else {
            FormNavigator::GoToChat(pConn, L"", pending, this);
        }
    } else {
        FormNavigator::GoToContactList(pConn, this);
    }
}

void LoginForm::OnLoginFailed(const String& reason) {
    if (reason.StartsWith(LocString(L"IDS_AUTH_REJECT_PREFIX"), 0) || reason.StartsWith(LocString(L"IDS_AUTH_REJECTED"), 0)) {
        AppSettings::ClearPassword();
    }

    MessageBox msgBox;
    String errorMsg = reason.IsEmpty()
        ? String(LocString(L"IDS_LOGIN_FAIL_DEFAULT"))
        : reason;
    msgBox.Construct(LocString(L"IDS_LOGIN_FAIL_TITLE"), errorMsg, MSGBOX_STYLE_OK);
    int modalResult = 0;
    msgBox.ShowAndWait(modalResult);
}
