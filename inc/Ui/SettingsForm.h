#ifndef _SETTINGS_FORM_H_
#define _SETTINGS_FORM_H_

#include <FUi.h>
#include <FBase.h>

class AggConnection;

class SettingsForm : public Osp::Ui::Controls::Form,
                     public Osp::Ui::IActionEventListener
{
public:
    SettingsForm(void);
    virtual ~SettingsForm(void);

    result Initialize(AggConnection* pConnection = null,
                      Osp::Ui::Controls::Form* pReturnTo = null);

    virtual result OnInitializing(void);
    virtual void OnActionPerformed(const Osp::Ui::Control& source, int actionId);

private:
    void Leave(void);
    void ApplyDefaults(void);

    bool ReadAndValidate(Osp::Base::String& outIp, int& outPort);

    static const int ID_SOFTKEY_BACK = 201;
    static const int ID_SOFTKEY_SAVE = 202;
    static const int ID_BTN_DEFAULT  = 203;

    Osp::Ui::Controls::EditField* pEditIp;
    Osp::Ui::Controls::EditField* pEditPort;

    AggConnection* pConnection;
    Osp::Ui::Controls::Form* pReturnTo;
};

#endif
