/**
 * AGG - клієнт Mail.ru Агента (протокол MRIM) для bada 1.0.
 */

#include "AGG.h"
#include "Core/Loc.h"
#include "Core/AppSettings.h"
#include "Core/MessageRouter.h"
#include "Ui/FormNavigator.h"

using namespace Osp::App;
using namespace Osp::Base;
using namespace Osp::System;
using namespace Osp::Ui::Controls;

AGG::AGG() {}
AGG::~AGG() {}

Application* AGG::CreateInstance(void) {
    return new AGG();
}

void AGG::AttachForcedLogoutHandler(AggConnection* pConnection) {
    if (pConnection == null) return;

    AGG* pApp = static_cast<AGG*>(Application::GetInstance());
    if (pApp != null) pConnection->SetForcedLogoutListener(pApp);
}

bool AGG::OnAppInitializing(AppRegistry& appRegistry) {
    if (!AppSettings::HasSavedCredentials()) {
        FormNavigator::GoToLogin(null, null);
        return true;
    }

    AggConnection* pConnection = new AggConnection();
    pConnection->Construct();
    AttachForcedLogoutHandler(pConnection);

    pConnection->SetCredentials(AppSettings::GetUserEmail(), AppSettings::GetUserPassword());

    FormNavigator::GoToContactList(pConnection, null);
    pConnection->ConnectToServer(AppSettings::GetServerIp(), AppSettings::GetServerPort());

    return true;
}

bool AGG::OnAppTerminating(AppRegistry& appRegistry, bool forcedTermination) {
    return true;
}

void AGG::OnForcedLogout(void) {
    MessageBox msgBox;
    msgBox.Construct(LocString(L"IDS_SESSION_TITLE"),
                     LocString(L"IDS_SESSION_TEXT"),
                     MSGBOX_STYLE_OK);
    int modalResult = 0;
    msgBox.ShowAndWait(modalResult);

    FormNavigator::GoToLogin(AggConnection::GetActive(), FormNavigator::GetCurrentForm());
}

void AGG::OnForeground(void) {
    MessageRouter::SetAppForeground(true);
    AggConnection::NotifyAppForegroundState(true);

    AggConnection* pConn = AggConnection::GetActive();
    if (pConn == null || pConn->GetLogin().IsEmpty()) return;

    String sender = pConn->PeekPendingNotificationSender();
    if (sender.IsEmpty()) return;

    String kind = pConn->PeekPendingNotificationKind();
    pConn->ConsumePendingNotificationSender();

    if (kind == L"BLOG") {
        FormNavigator::GoToMicroblog(pConn, FormNavigator::GetCurrentForm());
        return;
    }

    if (pConn->IsActiveChatWith(sender)) {
        return;
    }

    FormNavigator::GoToChat(pConn, L"", sender, FormNavigator::GetCurrentForm());
}

void AGG::OnBackground(void) {
    MessageRouter::SetAppForeground(false);
    AggConnection::NotifyAppForegroundState(false);
}

void AGG::OnLowMemory(void) {}
void AGG::OnBatteryLevelChanged(BatteryLevel batteryLevel) {}
void AGG::OnScreenOn(void) {}
void AGG::OnScreenOff(void) {}
