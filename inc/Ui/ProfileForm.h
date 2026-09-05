#ifndef _PROFILE_FORM_H_
#define _PROFILE_FORM_H_

#include <FUi.h>
#include <FBase.h>
#include "Core/AggConnection.h"
#include "MRIM/MrimProfile.h"

// Перегляд власного профілю (IDF_PROFILE). Лише перегляд: сервер
// (mrimsu/mrim-server) уміє повертати анкету через MRIM_CS_WP_REQUEST,
// але не редагувати її - на цьому сервері поля, найімовірніше, просто
// порожні, бо їх нема звідки взяти.
//
// Реалізована як один нередагований EditArea (як історія чату), а не
// набір Label-ів - найпростіший і вже перевірений на цьому SDK спосіб
// показати текст, що не влазить в один рядок.
class ProfileForm : public Osp::Ui::Controls::Form,
                     public Osp::Ui::IActionEventListener,
                     public IProfileListener
{
public:
    ProfileForm(void);
    virtual ~ProfileForm(void);

    result Initialize(AggConnection* pConn);

    virtual result OnInitializing(void);
    virtual result OnTerminating(void);
    virtual void OnActionPerformed(const Osp::Ui::Control& source, int actionId);

    // --- IProfileListener ---
    virtual void OnProfileReceived(const ProfileInfo& info);
    virtual void OnProfileNotFound(void);

private:
    void ShowText(const Osp::Base::String& text);
    // Додає рядок "мітка: значення" лише якщо значення не порожнє -
    // сервер, найімовірніше, поверне анкету майже порожньою.
    void AppendField(Osp::Base::String& out, const Osp::Base::String& label, const Osp::Base::String& value) const;

    static const int ID_SOFTKEY_BACK = 401;

    AggConnection* pConnection;
    Osp::Ui::Controls::EditArea* pProfileText;
};

#endif
