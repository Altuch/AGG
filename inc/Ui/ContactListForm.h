#ifndef _CONTACT_LIST_FORM_H_
#define _CONTACT_LIST_FORM_H_

#include <FUi.h>
#include <FBase.h>
#include <FGraphics.h>
#include "Core/AggConnection.h"
#include "Core/MessageRouter.h"
#include "MRIM/MrimContacts.h"

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
    virtual void OnUserEventReceivedN(long requestId, Osp::Base::Collection::IList* pArgs);
    virtual void OnItemStateChanged(const Osp::Ui::Control& source, int groupIndex, int itemIndex, int itemId, Osp::Ui::ItemStatus status);
    virtual void OnItemStateChanged(const Osp::Ui::Control& source, int groupIndex, int itemIndex, int elementId, int itemId, Osp::Ui::ItemStatus status);
    virtual void OnContactListReceived(Osp::Base::Collection::IList* pGroups, Osp::Base::Collection::IList* pContacts);
    virtual void OnUnreadCountChanged(void);
    virtual void OnConnectionStateChanged(bool connected);

private:
    void AttachListeners(void);
    void DetachListeners(void);

    void PopulateList(void);
    void SchedulePopulate(void);
    Osp::Ui::Controls::CustomListItem* CreateRow(const Osp::Base::String& title,
                                                 const Osp::Base::String& email,
                                                 const Osp::Graphics::Bitmap* pIcon);
    int AddStrangersGroup(int groupIndex);
    void PublishKnownContacts(void);
    void ShowStatusMenu(void);

    const Osp::Graphics::Bitmap* GetStatusBitmap(unsigned long status) const;

    static const int ID_STATUS_ONLINE    = 501;
    static const int ID_STATUS_AWAY      = 502;
    static const int ID_STATUS_INVISIBLE = 503;
    static const int ID_STATUS_BUSY      = 504;

    static const int ID_SOFTKEY_PROFILE    = 101;
    static const int ID_SOFTKEY_LOGOUT     = 102;
    static const int ID_OPTIONKEY_MENU     = 103;
    static const int ID_MENU_CHANGE_STATUS = 104;
    static const int ID_MENU_ADD           = 105;
    static const int ID_MENU_SETTINGS      = 109;

    static const long USER_EVENT_POPULATE        = 2001;
    static const long USER_EVENT_ATTACH_LISTENER = 2002;

    static const int ELEM_NAME  = 1;
    static const int ELEM_ICON  = 2;
    static const int ELEM_BADGE = 3;
    static const int ROW_HEIGHT = 60;

    static const int GROUP_MY_STATUS = 0;

    AggConnection* pConnection;
    Osp::Ui::Controls::GroupedList* pGroupedList;
    Osp::Ui::Controls::CustomListItemFormat* pItemFormat;
    Osp::Ui::Controls::ContextMenu* pStatusContextMenu;

    Osp::Base::Collection::ArrayList* pSavedGroups;
    Osp::Base::Collection::ArrayList* pSavedContacts;

    int strangersGroupIndex;

    bool hasCheckedPendingChat;

    Osp::Graphics::Bitmap* pBitmapOnline;
    Osp::Graphics::Bitmap* pBitmapAway;
    Osp::Graphics::Bitmap* pBitmapBusy;
    Osp::Graphics::Bitmap* pBitmapOffline;

    unsigned long myStatus;
};

#endif
