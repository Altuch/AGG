#ifndef _AGG_H_
#define _AGG_H_

#include <FApp.h>
#include <FBase.h>
#include <FSystem.h>
#include <FUi.h>

#include "Core/AggConnection.h"

class AGG : public Osp::App::Application,
            public Osp::System::IScreenEventListener,
            public IForcedLogoutListener
{
public:
    static Osp::App::Application* CreateInstance(void);

    static void AttachForcedLogoutHandler(AggConnection* pConnection);

    AGG();
    ~AGG();

    bool OnAppInitializing(Osp::App::AppRegistry& appRegistry);
    bool OnAppTerminating(Osp::App::AppRegistry& appRegistry, bool forcedTermination = false);

    void OnForeground(void);
    void OnBackground(void);
    void OnLowMemory(void);
    void OnBatteryLevelChanged(Osp::System::BatteryLevel batteryLevel);
    void OnScreenOn(void);
    void OnScreenOff(void);

    virtual void OnForcedLogout(void);
};

#endif
