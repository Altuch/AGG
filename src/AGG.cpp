/**
 * Name        : AGG
 * Version     : 
 * Vendor      : 
 * Description : 
 */

#include "AGG.h"
#include "Form1.h"
#include "ContactListForm.h"
#include "AggConnection.h"
#include <FApp.h>
#include <FBase.h>

using namespace Osp::App;
using namespace Osp::Base;
using namespace Osp::System;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;

AGG::AGG() {}
AGG::~AGG() {}

Application* AGG::CreateInstance(void) {
    return new AGG();
}

bool AGG::OnAppInitializing(AppRegistry& appRegistry) {
    Frame* pFrame = GetAppFrame()->GetFrame();

    String savedEmail = L"";
    String savedPassword = L"";
    appRegistry.Get(L"UserEmail", savedEmail);
    appRegistry.Get(L"UserPassword", savedPassword);

    savedEmail.Trim();
    savedPassword.Trim();

    if (!savedEmail.IsEmpty() && !savedPassword.IsEmpty()) {
        AppLog("Автологін: знайдено збережені дані, підключаємося...");

        AggConnection* pConnection = new AggConnection();
        pConnection->Construct();

        ContactListForm* pContactForm = new ContactListForm();
        pContactForm->Initialize(pConnection);

        String serverIp = L"103.71.21.140";
        String serverPortStr = L"2041";
        int serverPort = 2041;

        appRegistry.Get(L"ServerIP", serverIp);
        appRegistry.Get(L"ServerPort", serverPortStr);
        Integer::Parse(serverPortStr, serverPort);

        pConnection->userLogin = savedEmail;
        pConnection->userPassword = savedPassword;
        pConnection->ConnectToDirectServer(serverIp, serverPort);

        pFrame->AddControl(*pContactForm);
        pFrame->SetCurrentForm(*pContactForm);
        pContactForm->Draw();
        pContactForm->Show();
        pContactForm->AttachContactListener();
    } else {
        AppLog("Збережених даних немає: відкриваємо форму авторизації...");

        Form1* pForm1 = new Form1();
        pForm1->Initialize();

        pFrame->AddControl(*pForm1);
        pFrame->SetCurrentForm(*pForm1);
        pForm1->Draw();
        pForm1->Show();
    }

    return true;
}

bool AGG::OnAppTerminating(AppRegistry& appRegistry, bool forcedTermination) {
    return true;
}

void AGG::OnForeground(void) {}
void AGG::OnBackground(void) {}
void AGG::OnLowMemory(void) {}
void AGG::OnBatteryLevelChanged(BatteryLevel batteryLevel) {}
void AGG::OnScreenOn(void) {}
void AGG::OnScreenOff(void) {}
