/**
 * AGG - клієнт Mail.ru Агента (протокол MRIM) для bada 1.0.
 */

#include "AGG.h"
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
        AppLog("Збережених даних немає: відкриваємо форму авторизації...");
        FormNavigator::GoToLogin(null, null);
        return true;
    }

    AppLog("Автологін: знайдено збережені дані, підключаємося...");

    AggConnection* pConnection = new AggConnection();
    pConnection->Construct();
    AttachForcedLogoutHandler(pConnection);

    pConnection->SetCredentials(AppSettings::GetUserEmail(), AppSettings::GetUserPassword());

    // Список контактів показуємо одразу: він сам підхопить дані, коли
    // сервер їх надішле.
    FormNavigator::GoToContactList(pConnection, null);
    pConnection->ConnectToServer(AppSettings::GetServerIp(), AppSettings::GetServerPort());

    return true;
}

bool AGG::OnAppTerminating(AppRegistry& appRegistry, bool forcedTermination) {
    return true;
}

void AGG::OnForcedLogout(void) {
    MessageBox msgBox;
    msgBox.Construct(L"Сеанс завершено",
                     L"Ви увійшли в цей акаунт з іншого пристрою. MRIM дозволяє лише один активний сеанс - увійдіть знову, коли будете готові.",
                     MSGBOX_STYLE_OK);
    int modalResult = 0;
    msgBox.ShowAndWait(modalResult);

    // Те саме з'єднання передаємо формі входу - вона його скине й
    // перевикористає, тож зайвого об'єкта з живим сокетом не лишиться.
    FormNavigator::GoToLogin(AggConnection::GetActive(), FormNavigator::GetCurrentForm());
}

void AGG::OnForeground(void) {
    MessageRouter::SetAppForeground(true);
    AggConnection::NotifyAppForegroundState(true);
}

void AGG::OnBackground(void) {
    // Згорнутий застосунок = "Відійшов" для контактів; сповіщення при
    // цьому лишаються потрібними, тож router теж має знати.
    MessageRouter::SetAppForeground(false);
    AggConnection::NotifyAppForegroundState(false);
}

void AGG::OnLowMemory(void) {}
void AGG::OnBatteryLevelChanged(BatteryLevel batteryLevel) {}
void AGG::OnScreenOn(void) {}
void AGG::OnScreenOff(void) {}
