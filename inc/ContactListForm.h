#ifndef _CONTACT_LIST_FORM_H_
#define _CONTACT_LIST_FORM_H_

#include <FUi.h>
#include <FBase.h>
#include <FGraphics.h>
#include "AggConnection.h"
#include "MRIM/MrimContacts.h"
#include "MessageRouter.h"

class ContactListForm : public Osp::Ui::Controls::Form,
                        public Osp::Ui::IGroupedItemEventListener,
                        public Osp::Ui::IActionEventListener,
                        public IContactListListener,
                        public IUnreadCountListener,
                        public IConnectionStateListener
{
public:
    ContactListForm(void);
    virtual ~ContactListForm(void);

    result Initialize(AggConnection* pConn);
    void ScheduleAttachContactListener(void);
    virtual result OnInitializing(void);
    virtual result OnTerminating(void);

    virtual void OnActionPerformed(const Osp::Ui::Control& source, int actionId);

    virtual void OnItemStateChanged(const Osp::Ui::Control& source, int groupIndex, int itemIndex, int itemId, Osp::Ui::ItemStatus status);
    virtual void OnItemStateChanged(const Osp::Ui::Control& source, int groupIndex, int itemIndex, int elementId, int itemId, Osp::Ui::ItemStatus status);

    virtual void OnUserEventReceivedN(long requestId, Osp::Base::Collection::IList* pArgs);
    virtual void OnContactListReceived(Osp::Base::Collection::IList* pGroups, Osp::Base::Collection::IList* pContacts);
    virtual void OnUnreadCountChanged(void);
    virtual void OnConnectionStateChanged(bool connected);

private:
    void PopulateList(void);
    void DetachListeners(void);
    const Osp::Graphics::Bitmap* GetStatusBitmap(unsigned long status) const;

    static const int ID_OPTIONKEY_SETTINGS = 101;
    static const int ID_SOFTKEY_EXIT       = 102;
    static const int ID_OPTIONKEY_MENU        = 103;
    static const int ID_MENU_CHANGE_STATUS    = 104;
    static const int ID_MENU_LOGOUT           = 105;
    static const int ID_STATUS_ONLINE         = 106;
    static const int ID_STATUS_AWAY           = 107;
    static const int ID_STATUS_INVISIBLE      = 108;
    static const long USER_EVENT_CONTACTS_READY = 2001;
    static const long USER_EVENT_ATTACH_LISTENER = 2002;

    void AttachContactListener(void);

    AggConnection* pConnection;
    Osp::Ui::Controls::GroupedList* pGroupedList;
    Osp::Ui::Controls::CustomListItemFormat* pItemFormat;

    Osp::Base::Collection::ArrayList* pSavedGroups;
    Osp::Base::Collection::ArrayList* pSavedContacts;
    int strangersGroupIndex;

    Osp::Graphics::Bitmap* pBitmapOnline;
    Osp::Graphics::Bitmap* pBitmapAway;
    Osp::Graphics::Bitmap* pBitmapDnd;
    Osp::Graphics::Bitmap* pBitmapOffline;
};

#endif
