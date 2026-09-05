#ifndef _LOGIN_FORM_H_
#define _LOGIN_FORM_H_

#include <FUi.h>
#include <FBase.h>
#include "Core/AggConnection.h"

class LoginForm : public Osp::Ui::Controls::Form,
                  public Osp::Ui::IActionEventListener,
                  public ILoginListener
{
public:
    LoginForm(void);
    virtual ~LoginForm(void);

    result Initialize(AggConnection* pExisting = null);

    virtual result OnInitializing(void);
    virtual result OnTerminating(void);
    virtual void OnActionPerformed(const Osp::Ui::Control& source, int actionId);

    virtual void OnLoginSuccess(void);
    virtual void OnLoginFailed(const Osp::Base::String& reason);

private:
    static const int ID_BTN_LOGIN    = 101;
    static const int ID_BTN_SIGNUP   = 102;
    static const int ID_BTN_SETTINGS = 103;

    Osp::Ui::Controls::EditField* pEditEmail;
    Osp::Ui::Controls::EditField* pEditPassword;
    AggConnection* pConnection;
};

#endif
