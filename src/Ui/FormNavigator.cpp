#include "Ui/FormNavigator.h"
#include "Ui/LoginForm.h"
#include "Ui/ContactListForm.h"
#include "Ui/ChatForm.h"
#include "Ui/SettingsForm.h"
#include "Ui/ProfileForm.h"
#include "Ui/MicroblogForm.h"
#include <FApp.h>

using namespace Osp::App;
using namespace Osp::Base;
using namespace Osp::Ui::Controls;

static Frame* GetFrame(void) {
    Application* pApp = Application::GetInstance();
    return (pApp != null && pApp->GetAppFrame() != null) ? pApp->GetAppFrame()->GetFrame() : null;
}

Form* FormNavigator::GetCurrentForm(void) {
    Frame* pFrame = GetFrame();
    return (pFrame != null) ? pFrame->GetCurrentForm() : null;
}

void FormNavigator::Show(Form* pNewForm, Form* pFrom) {
    if (pNewForm == null) return;

    Frame* pFrame = GetFrame();
    if (pFrame == null) {
        delete pNewForm;
        return;
    }

    pFrame->AddControl(*pNewForm);
    pFrame->SetCurrentForm(*pNewForm);
    pNewForm->Draw();
    pNewForm->Show();

    if (pFrom != null) pFrame->RemoveControl(*pFrom);
}

void FormNavigator::GoToLogin(AggConnection* pConnection, Form* pFrom) {
    LoginForm* pForm = new LoginForm();
    pForm->Initialize(pConnection);
    Show(pForm, pFrom);
}

void FormNavigator::GoToContactList(AggConnection* pConnection, Form* pFrom) {
    ContactListForm* pForm = new ContactListForm();
    if (IsFailed(pForm->Initialize(pConnection))) {
        delete pForm;
        return;
    }
    Show(pForm, pFrom);
    pForm->ScheduleAttachContactListener();
}

void FormNavigator::GoToChat(AggConnection* pConnection,
                             const String& contactName,
                             const String& contactEmail,
                             Form* pFrom) {
    ChatForm* pForm = new ChatForm();
    pForm->Initialize(pConnection, contactName, contactEmail);
    Show(pForm, pFrom);
}

void FormNavigator::GoToSettings(AggConnection* pConnection, Form* pFrom) {
    Form* pReturnTo = (pFrom == null) ? GetCurrentForm() : null;

    SettingsForm* pForm = new SettingsForm();
    pForm->Initialize(pConnection, pReturnTo);
    Show(pForm, pFrom);
}

void FormNavigator::GoToMicroblog(AggConnection* pConnection, Form* pFrom) {
    MicroblogForm* pForm = new MicroblogForm();
    if (IsFailed(pForm->Initialize(pConnection))) {
        delete pForm;
        return;
    }
    Show(pForm, pFrom);
}

void FormNavigator::GoToProfile(AggConnection* pConnection,
                                const String& contactName,
                                const String& contactEmail,
                                Form* pFrom) {

    Form* pReturnTo = (pFrom == null) ? GetCurrentForm() : null;

    ProfileForm* pForm = new ProfileForm();
    pForm->Initialize(pConnection, contactName, contactEmail, pReturnTo);
    Show(pForm, pFrom);
}

void FormNavigator::Back(Form* pTarget, Form* pFrom) {
    if (pTarget == null) return;

    Frame* pFrame = GetFrame();
    if (pFrame == null) return;

    pFrame->SetCurrentForm(*pTarget);
    pTarget->Draw();
    pTarget->Show();

    if (pFrom != null) pFrame->RemoveControl(*pFrom);
}
