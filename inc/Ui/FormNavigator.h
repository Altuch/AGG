#ifndef _FORM_NAVIGATOR_H_
#define _FORM_NAVIGATOR_H_

#include <FUi.h>
#include <FBase.h>

class AggConnection;

// Єдине місце, де застосунок перемикає екрани.
//
// Раніше послідовність AddControl -> SetCurrentForm -> Draw -> Show ->
// RemoveControl(стара форма) була переписана вісім разів; порядок у ній
// має значення (стару форму можна прибирати ЛИШЕ після того, як нова
// стала поточною), і кожна копія була нагодою помилитись.
class FormNavigator {
public:
    // pFrom - форма, з якої йдемо; вона прибирається з Frame (а отже й
    // руйнується) ПІСЛЯ того, як нова вже показана. null - лишити
    // поточну форму на місці (нова просто лягає зверху).
    //
    // УВАГА: після виклику з непорожнім pFrom об'єкт pFrom знищено -
    // не звертайтесь до його полів (у т.ч. неявно через this).
    static void GoToLogin(AggConnection* pConnection, Osp::Ui::Controls::Form* pFrom);
    static void GoToContactList(AggConnection* pConnection, Osp::Ui::Controls::Form* pFrom);
    static void GoToChat(AggConnection* pConnection,
                         const Osp::Base::String& contactName,
                         const Osp::Base::String& contactEmail,
                         Osp::Ui::Controls::Form* pFrom);
    static void GoToSettings(AggConnection* pConnection, Osp::Ui::Controls::Form* pFrom);
    static void GoToProfile(AggConnection* pConnection, Osp::Ui::Controls::Form* pFrom);

    // Повернутись до форми, яка лишилась під поточною (її не треба
    // створювати заново), і прибрати pFrom.
    static void Back(Osp::Ui::Controls::Form* pTarget, Osp::Ui::Controls::Form* pFrom);

    static Osp::Ui::Controls::Form* GetCurrentForm(void);

private:
    static void Show(Osp::Ui::Controls::Form* pNewForm, Osp::Ui::Controls::Form* pFrom);
};

#endif
