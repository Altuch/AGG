#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <FUi.h>
#include <FApp.h>

class AggConnection;

class Settings : public Osp::Ui::Controls::Form,
                 public Osp::Ui::IActionEventListener
{
public:
    Settings(void);
    virtual ~Settings(void);
    bool Initialize(AggConnection* pConn = null);
    virtual result OnInitializing(void);
    virtual void OnActionPerformed(const Osp::Ui::Control& source, int actionId);

    void ResetToDefault(void);

private:
    void ReturnFromSettings(void);
    static const int ID_SOFTKEY_BACK = 201;
    static const int ID_SOFTKEY_SAVE = 202;
    static const int ID_BTN_DEFAULT  = 203;

    Osp::Ui::Controls::EditField* pEditIp;
    Osp::Ui::Controls::EditField* pEditPort;
    AggConnection* pConnection;
};

#endif
