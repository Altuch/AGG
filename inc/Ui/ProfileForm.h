#ifndef _PROFILE_FORM_H_
#define _PROFILE_FORM_H_

#include <FUi.h>
#include <FBase.h>
#include "Core/AggConnection.h"
#include "MRIM/MrimProfile.h"

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

    virtual void OnProfileReceived(const ProfileInfo& info);
    virtual void OnProfileNotFound(void);

private:
    void ShowText(const Osp::Base::String& text);
    void AppendField(Osp::Base::String& out, const Osp::Base::String& label, const Osp::Base::String& value) const;

    static const int ID_SOFTKEY_BACK = 401;

    AggConnection* pConnection;
    Osp::Ui::Controls::EditArea* pProfileText;
};

#endif
