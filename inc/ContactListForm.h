#ifndef _CONTACT_LIST_FORM_H_
#define _CONTACT_LIST_FORM_H_

#include <FUi.h>
#include <FBase.h>
#include "AggConnection.h"
#include "MRIM/MrimContacts.h"

class ContactListForm : public Osp::Ui::Controls::Form,
                        public Osp::Ui::IGroupedItemEventListener,
                        public Osp::Ui::IActionEventListener,
                        public IContactListListener
{
public:
    ContactListForm(void);
    virtual ~ContactListForm(void);

    result Initialize(AggConnection* pConn);
    // Реєструє форму як слухача списку контактів. Кеш може прийти
    // синхронно (одразу під час виклику) і спричинити перемальовування,
    // тож викликати це треба ЛИШЕ після AddControl/SetCurrentForm/Show,
    // інакше control ще не приєднаний до Frame і RequestRedraw/Show впаде.
    void AttachContactListener(void);
    virtual result OnInitializing(void);
    virtual result OnTerminating(void);

    virtual void OnActionPerformed(const Osp::Ui::Control& source, int actionId);

    // Слухачі списку контактів
    virtual void OnItemStateChanged(const Osp::Ui::Control& source, int groupIndex, int itemIndex, int itemId, Osp::Ui::ItemStatus status);
    virtual void OnItemStateChanged(const Osp::Ui::Control& source, int groupIndex, int itemIndex, int elementId, int itemId, Osp::Ui::ItemStatus status);

    virtual void OnUserEventReceivedN(long requestId, Osp::Base::Collection::IList* pArgs);
    virtual void OnContactListReceived(Osp::Base::Collection::IList* pGroups, Osp::Base::Collection::IList* pContacts);

private:
    void PopulateList(void);

    static const int ID_OPTIONKEY_SETTINGS = 101;
    static const int ID_SOFTKEY_EXIT       = 102;
    static const long USER_EVENT_CONTACTS_READY = 2001;

    AggConnection* pConnection;
    Osp::Ui::Controls::GroupedList* pGroupedList;
    Osp::Ui::Controls::CustomListItemFormat* pItemFormat;

    Osp::Base::Collection::ArrayList* pSavedGroups;
    Osp::Base::Collection::ArrayList* pSavedContacts;
};

#endif
