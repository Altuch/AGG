#ifndef _FORM1_H_
#define _FORM1_H_

#include <FUi.h>
#include <FUiControls.h>
#include "AggConnection.h"

class Form1 : public Osp::Ui::Controls::Form,
              public Osp::Ui::IActionEventListener,
              public ILoginListener
{
public:
    Form1(void);
    virtual ~Form1(void);
    bool Initialize(void);

public:
    virtual void OnLoginSuccess(void);
    virtual void OnLoginFailed(const Osp::Base::String& reason);
    virtual result OnInitializing(void);
    virtual result OnTerminating(void);
    virtual void OnActionPerformed(const Osp::Ui::Control& source, int actionId);

private:
    static const int ID_BTN_LOGIN = 101;
    static const int ID_BTN_SIGNUP = 102;
    static const int ID_BTN_SETTINGS = 103;

    Osp::Ui::Controls::EditField* pEditEmail;
    Osp::Ui::Controls::EditField* pEditPassword;

    AggConnection* pConnection;
};

#endif
