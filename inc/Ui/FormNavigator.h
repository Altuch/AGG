#ifndef _FORM_NAVIGATOR_H_
#define _FORM_NAVIGATOR_H_

#include <FUi.h>
#include <FBase.h>

class AggConnection;

class FormNavigator {
public:
    static void GoToLogin(AggConnection* pConnection, Osp::Ui::Controls::Form* pFrom);
    static void GoToContactList(AggConnection* pConnection, Osp::Ui::Controls::Form* pFrom);
    static void GoToChat(AggConnection* pConnection,
                         const Osp::Base::String& contactName,
                         const Osp::Base::String& contactEmail,
                         Osp::Ui::Controls::Form* pFrom);
    static void GoToSettings(AggConnection* pConnection, Osp::Ui::Controls::Form* pFrom);

    static void GoToProfile(AggConnection* pConnection,
                            const Osp::Base::String& contactName,
                            const Osp::Base::String& contactEmail,
                            Osp::Ui::Controls::Form* pFrom);

    static void Back(Osp::Ui::Controls::Form* pTarget, Osp::Ui::Controls::Form* pFrom);

    static Osp::Ui::Controls::Form* GetCurrentForm(void);

private:
    static void Show(Osp::Ui::Controls::Form* pNewForm, Osp::Ui::Controls::Form* pFrom);
};

#endif
