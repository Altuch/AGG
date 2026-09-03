#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <FUi.h>
#include <FApp.h>

class Settings : public Osp::Ui::Controls::Form,
                 public Osp::Ui::IActionEventListener
{
public:
    Settings(void);
    virtual ~Settings(void);
    bool Initialize(void);
    virtual result OnInitializing(void);
    virtual void OnActionPerformed(const Osp::Ui::Control& source, int actionId);

    void ResetToDefault(void);

private:
    static const int ID_SOFTKEY_BACK = 201;
    static const int ID_SOFTKEY_SAVE = 202;
    static const int ID_BTN_DEFAULT  = 203;

    Osp::Ui::Controls::EditField* pEditIp;
    Osp::Ui::Controls::EditField* pEditPort;
};

#endif
