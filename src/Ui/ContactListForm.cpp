#include "Ui/ContactListForm.h"
#include "Core/Loc.h"
#include "Ui/FormNavigator.h"
#include "Core/AppSettings.h"
#include "MRIM/MrimProtocol.h"
#include <FApp.h>

using namespace Osp::App;
using namespace Osp::Base;
using namespace Osp::Base::Collection;
using namespace Osp::Graphics;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;

ContactListForm::ContactListForm(void) :
    pConnection(null),
    pGroupedList(null),
    pItemFormat(null),
    pStatusContextMenu(null),
    pSavedGroups(null),
    pSavedContacts(null),
    strangersGroupIndex(-1),
    hasCheckedPendingChat(false),
    populatePending_(false),
    pBitmapOnline(null),
    pBitmapAway(null),
    pBitmapBusy(null),
    pBitmapOffline(null),
    myStatus(Mrim::Status::ONLINE)
{
}

ContactListForm::~ContactListForm(void) {
    DetachListeners();

    delete pItemFormat;
    if (pSavedGroups != null) { pSavedGroups->RemoveAll(true); delete pSavedGroups; }
    if (pSavedContacts != null) { pSavedContacts->RemoveAll(true); delete pSavedContacts; }
    if (pStatusContextMenu != null) {
            delete pStatusContextMenu;
            pStatusContextMenu = null;
    }

    delete pBitmapOnline;
    delete pBitmapAway;
    delete pBitmapBusy;
    delete pBitmapOffline;
}

result ContactListForm::Initialize(AggConnection* pConn) {
    pConnection = pConn;
    return Construct(L"IDF_CONT");
}

result ContactListForm::OnInitializing(void) {
    SetTitleText(LocString(L"IDS_CONTACTS"));

    SetOptionkeyActionId(ID_OPTIONKEY_MENU);
    AddOptionkeyActionListener(*this);

    SetSoftkeyActionId(SOFTKEY_0, ID_SOFTKEY_PROFILE);
    AddSoftkeyActionListener(SOFTKEY_0, *this);

    SetSoftkeyActionId(SOFTKEY_1, ID_SOFTKEY_LOGOUT);
    AddSoftkeyActionListener(SOFTKEY_1, *this);
    SetSoftkeyText(SOFTKEY_0, LocString(L"IDS_SK_PROFILE"));
    SetSoftkeyText(SOFTKEY_1, LocString(L"IDS_SK_SIGNOUT"));

    pGroupedList = static_cast<GroupedList*>(GetControl(L"IDC_CONTACT_LIST"));
    if (pGroupedList != null) {
        pGroupedList->AddGroupedItemEventListener(*this);

        pItemFormat = new CustomListItemFormat();
        pItemFormat->Construct();
        pItemFormat->AddElement(ELEM_ICON,  Rectangle(15, 14, 32, 32));
        pItemFormat->AddElement(ELEM_NAME,  Rectangle(60, 10, 340, 40));
        pItemFormat->AddElement(ELEM_BADGE, Rectangle(405, 10, 50, 40));
    }

    AppResource* pRes = Application::GetInstance()->GetAppResource();
    if (pRes != null) {
        pBitmapOnline  = pRes->GetBitmapN(L"Statuses/Online.png");
        pBitmapAway    = pRes->GetBitmapN(L"Statuses/Away.png");
        pBitmapBusy    = pRes->GetBitmapN(L"Statuses/Busy.png");
        pBitmapOffline = pRes->GetBitmapN(L"Statuses/Offline.png");
    }

    if (pSavedContacts != null) PopulateList();

    return E_SUCCESS;
}

result ContactListForm::OnTerminating(void) {
    return E_SUCCESS;
}

void ContactListForm::ScheduleAttachContactListener(void) {
    SendUserEvent(USER_EVENT_ATTACH_LISTENER, null);
}

void ContactListForm::AttachListeners(void) {
    if (pConnection == null) return;
    pConnection->SetContactListListener(this);
    pConnection->SetContactListVisibleListener(this);
    pConnection->SetConnectionStateListener(this);
}

void ContactListForm::DetachListeners(void) {
    if (pConnection == null) return;
    pConnection->SetContactListListener(null);
    pConnection->SetContactListVisibleListener(null);
    pConnection->SetConnectionStateListener(null);
}

void ContactListForm::SchedulePopulate(void) {
    if (populatePending_) return;
    populatePending_ = true;
    SendUserEvent(USER_EVENT_POPULATE, null);
}

void ContactListForm::OnUserEventReceivedN(long requestId, IList* pArgs) {
    if (requestId == USER_EVENT_ATTACH_LISTENER) {
        AttachListeners();
    } else if (requestId == USER_EVENT_POPULATE) {
        populatePending_ = false;
        PopulateList();
    } else if (requestId == USER_EVENT_GOTO_BLOG) {
        AggConnection* pConn = pConnection;
        DetachListeners();
        FormNavigator::GoToMicroblog(pConn, this);
    }

    if (pArgs != null) {
        pArgs->RemoveAll(true);
        delete pArgs;
    }
}

void ContactListForm::OnUnreadCountChanged(void) {
    SchedulePopulate();
}

void ContactListForm::OnConnectionStateChanged(bool connected) {
    SetTitleText(connected ? LocString(L"IDS_CONTACTS") : LocString(L"IDS_CONTACTS_CONNECTING"));

    if (GetParent() != null) {
        RequestRedraw(true);
        Draw();
        Show();
    }
}

void ContactListForm::PublishKnownContacts(void) {
    if (pConnection == null || pSavedContacts == null) return;

    ArrayList emails;
    emails.Construct();

    for (int i = 0; i < pSavedContacts->GetCount(); i++) {
        ContactInfo* pContact = static_cast<ContactInfo*>(pSavedContacts->GetAt(i));
        if (pContact != null) emails.Add(*(new String(pContact->email)));
    }

    pConnection->SetKnownContactEmails(&emails);
    emails.RemoveAll(true);
}

void ContactListForm::OnContactListReceived(IList* pGroups, IList* pContacts) {
    if (pGroups == null || pContacts == null) return;

    if (pSavedGroups == null) {
        pSavedGroups = new ArrayList();
        pSavedGroups->Construct();
    } else {
        pSavedGroups->RemoveAll(true);
    }

    if (pSavedContacts == null) {
        pSavedContacts = new ArrayList();
        pSavedContacts->Construct();
    } else {
        pSavedContacts->RemoveAll(true);
    }

    for (int i = 0; i < pGroups->GetCount(); i++) {
        GroupInfo* pSrc = static_cast<GroupInfo*>(pGroups->GetAt(i));
        if (pSrc == null) continue;

        GroupInfo* pCopy = new GroupInfo();
        pCopy->name = pSrc->name;
        pCopy->flags = pSrc->flags;
        pSavedGroups->Add(*pCopy);
    }

    for (int i = 0; i < pContacts->GetCount(); i++) {
        ContactInfo* pSrc = static_cast<ContactInfo*>(pContacts->GetAt(i));
        if (pSrc == null) continue;

        ContactInfo* pCopy = new ContactInfo();
        pCopy->email = pSrc->email;
        pCopy->nickname = pSrc->nickname;
        pCopy->status = pSrc->status;
        pCopy->groupId = pSrc->groupId;
        pCopy->flags = pSrc->flags;
        pCopy->blogTime = pSrc->blogTime;
        pCopy->blogText = pSrc->blogText;
        pSavedContacts->Add(*pCopy);
    }

    PublishKnownContacts();

    if (!hasCheckedPendingChat) {
        hasCheckedPendingChat = true;
        if (pConnection != null) {
            String pendingKind = pConnection->PeekPendingNotificationKind();
            String pending = pConnection->ConsumePendingNotificationSender();
            if (!pending.IsEmpty() && !pConnection->IsActiveChatWith(pending)) {
                AggConnection* pConn = pConnection;
                DetachListeners();
                if (pendingKind == L"BLOG") {
                    FormNavigator::GoToMicroblog(pConn, this);
                    return;
                }
                String name = pending;
                for (int i = 0; i < pSavedContacts->GetCount(); i++) {
                    ContactInfo* pContact = static_cast<ContactInfo*>(pSavedContacts->GetAt(i));
                    if (pContact == null) continue;
                    String e = pContact->email;
                    if (!e.IsEmpty() && e.Equals(pending, true) && !pContact->nickname.IsEmpty()) {
                        name = pContact->nickname;
                        break;
                    }
                }
                FormNavigator::GoToChat(pConn, name, pending, this);
                return;
            }
        }
    }

    SchedulePopulate();
}

const Bitmap* ContactListForm::GetStatusBitmap(unsigned long status) const {
    if (status == Mrim::Status::OFFLINE || status == Mrim::Status::INVISIBLE) return pBitmapOffline;
    if (status == Mrim::Status::AWAY) return pBitmapAway;
    if (status == Mrim::Status::XSTATUS) return pBitmapBusy;
    return pBitmapOnline;
}

CustomListItem* ContactListForm::CreateRow(const String& title, const String& email, const Bitmap* pIcon) {
    CustomListItem* pItem = new CustomListItem();
    pItem->Construct(ROW_HEIGHT);
    if (pItemFormat != null) pItem->SetItemFormat(*pItemFormat);

    if (pIcon != null) pItem->SetElement(ELEM_ICON, *pIcon, null);
    pItem->SetElement(ELEM_NAME, title);

    int unread = (pConnection != null) ? pConnection->GetUnreadCount(email) : 0;
    if (unread > 0) pItem->SetElement(ELEM_BADGE, Integer::ToString(unread));

    return pItem;
}

int ContactListForm::AddStrangersGroup(int groupIndex) {
    IList* pStrangers = (pConnection != null) ? pConnection->GetStrangerEmails() : null;
    if (pStrangers == null || pStrangers->GetCount() == 0) return -1;

    pGroupedList->AddGroup(LocString(L"IDS_GROUP_STRANGERS"), null, groupIndex);

    for (int i = 0; i < pStrangers->GetCount(); i++) {
        String* pEmail = static_cast<String*>(pStrangers->GetAt(i));
        if (pEmail == null) continue;

        pGroupedList->AddItem(groupIndex, *CreateRow(*pEmail, *pEmail, null), i);
    }

    return groupIndex;
}

void ContactListForm::PopulateList(void) {
    if (pGroupedList == null || pSavedContacts == null) return;

    pGroupedList->RemoveAllGroups();
    strangersGroupIndex = -1;

    pGroupedList->AddGroup(LocString(L"IDS_GROUP_ME"), null, GROUP_MY_STATUS);
    String myLogin = (pConnection != null) ? pConnection->GetLogin() : String(L"");
    String myName = (pConnection != null) ? pConnection->GetNickname() : String(L"");
    if (myName.IsEmpty()) myName = myLogin;
    pGroupedList->AddItem(GROUP_MY_STATUS, *CreateRow(myName, myLogin, GetStatusBitmap(myStatus)), 0);

    int groupCount = (pSavedGroups != null) ? pSavedGroups->GetCount() : 0;
    int contactCount = pSavedContacts->GetCount();

    int listedGroups = 0;
    if (groupCount > 0) {
        for (int g = 0; g < groupCount; g++) {
            GroupInfo* pGroup = static_cast<GroupInfo*>(pSavedGroups->GetAt(g));
            String name = (pGroup != null && !pGroup->name.IsEmpty())
                        ? pGroup->name
                        : (LocString(L"IDS_GROUP_PREFIX") + Integer::ToString(g + 1));
            pGroupedList->AddGroup(name, null, g + 1);
        }
        listedGroups = groupCount;
    } else if (contactCount > 0) {
        pGroupedList->AddGroup(LocString(L"IDS_GROUP_ALL"), null, 1);
        listedGroups = 1;
    }

    ArrayList* pOrder = BuildContactOrder();
    for (int k = 0; k < pOrder->GetCount(); k++) {
        Integer* pIdx = static_cast<Integer*>(pOrder->GetAt(k));
        int c = (pIdx != null) ? pIdx->ToInt() : -1;
        if (c < 0 || c >= contactCount) continue;

        ContactInfo* pContact = static_cast<ContactInfo*>(pSavedContacts->GetAt(c));
        if (pContact == null || listedGroups == 0) continue;

        int group = (pContact->groupId < static_cast<unsigned long>(listedGroups))
                  ? static_cast<int>(pContact->groupId)
                  : 0;

        String title = pContact->nickname.IsEmpty() ? pContact->email : pContact->nickname;
        pGroupedList->AddItem(group + 1, *CreateRow(title, pContact->email, GetStatusBitmap(pContact->status)), c);
    }

    strangersGroupIndex = AddStrangersGroup(listedGroups + 1);

    if (GetParent() != null) {
        pGroupedList->RequestRedraw(true);
        pGroupedList->Draw();
        pGroupedList->Show();
    }

    pOrder->RemoveAll(true);
    delete pOrder;
}

bool ContactListForm::IsOnlineStatus(unsigned long status) {
    return status != Mrim::Status::OFFLINE && status != Mrim::Status::INVISIBLE;
}

int ContactListForm::CompareContacts(const ContactInfo* pA, const ContactInfo* pB, int mode) {
    if (pA == null) return (pB == null) ? 0 : 1;
    if (pB == null) return -1;

    if (mode == SORT_ONLINE_FIRST || mode == SORT_OFFLINE_FIRST) {
        bool aOn = IsOnlineStatus(pA->status);
        bool bOn = IsOnlineStatus(pB->status);
        if (aOn != bOn) {
            bool onlineFirst = (mode == SORT_ONLINE_FIRST);
            if (aOn) return onlineFirst ? -1 : 1;
            return onlineFirst ? 1 : -1;
        }
    }

    String a = pA->nickname.IsEmpty() ? pA->email : pA->nickname;
    String b = pB->nickname.IsEmpty() ? pB->email : pB->nickname;
    a.ToLower();
    b.ToLower();
    return a.CompareTo(b);
}

ArrayList* ContactListForm::BuildContactOrder(void) const {
    ArrayList* pOrder = new ArrayList();
    pOrder->Construct();
    if (pSavedContacts == null) return pOrder;

    int count = pSavedContacts->GetCount();
    int mode = AppSettings::GetContactSortMode();

    for (int i = 0; i < count; i++) pOrder->Add(*(new Integer(i)));
    if (mode == SORT_AS_RECEIVED || count < 2) return pOrder;

    ArrayList rest;
    rest.Construct();
    for (int i = 0; i < pOrder->GetCount(); i++) {
        Integer* pIdx = static_cast<Integer*>(pOrder->GetAt(i));
        rest.Add(*(new Integer(pIdx != null ? pIdx->ToInt() : 0)));
    }
    pOrder->RemoveAll(true);

    while (rest.GetCount() > 0) {
        int best = 0;
        for (int j = 1; j < rest.GetCount(); j++) {
            Integer* pBestIdx = static_cast<Integer*>(rest.GetAt(best));
            Integer* pCandIdx = static_cast<Integer*>(rest.GetAt(j));
            const ContactInfo* pBest = (pBestIdx != null && pBestIdx->ToInt() >= 0 && pBestIdx->ToInt() < count)
                ? static_cast<const ContactInfo*>(pSavedContacts->GetAt(pBestIdx->ToInt())) : null;
            const ContactInfo* pCand = (pCandIdx != null && pCandIdx->ToInt() >= 0 && pCandIdx->ToInt() < count)
                ? static_cast<const ContactInfo*>(pSavedContacts->GetAt(pCandIdx->ToInt())) : null;
            if (CompareContacts(pCand, pBest, mode) < 0) best = j;
        }
        Integer* pWinner = static_cast<Integer*>(rest.GetAt(best));
        int winnerIdx = (pWinner != null) ? pWinner->ToInt() : 0;
        rest.RemoveAt(best, true);
        pOrder->Add(*(new Integer(winnerIdx)));
    }
    return pOrder;
}

void ContactListForm::OnItemStateChanged(const Control& source, int groupIndex, int itemIndex, int itemId, ItemStatus status) {
    if (groupIndex == GROUP_MY_STATUS) {
        FormNavigator::GoToProfile(pConnection, L"", L"", null);
        return;
    }

    String targetName;
    String targetEmail;

    if (strangersGroupIndex >= 0 && groupIndex == strangersGroupIndex) {
        IList* pStrangers = (pConnection != null) ? pConnection->GetStrangerEmails() : null;
        if (pStrangers == null || itemId < 0 || itemId >= pStrangers->GetCount()) return;

        String* pEmail = static_cast<String*>(pStrangers->GetAt(itemId));
        if (pEmail == null) return;

        targetEmail = *pEmail;
        targetName = *pEmail;
    } else {
        if (pSavedContacts == null || itemId < 0 || itemId >= pSavedContacts->GetCount()) return;

        ContactInfo* pContact = static_cast<ContactInfo*>(pSavedContacts->GetAt(itemId));
        if (pContact == null) return;

        targetName = pContact->nickname;
        targetEmail = pContact->email;
    }

    AggConnection* pConn = pConnection;
    DetachListeners();

    FormNavigator::GoToChat(pConn, targetName, targetEmail, this);
}

void ContactListForm::OnItemStateChanged(const Control& source, int groupIndex, int itemIndex, int elementId, int itemId, ItemStatus status) {
    OnItemStateChanged(source, groupIndex, itemIndex, itemId, status);
}

void ContactListForm::OnActionPerformed(const Control& source, int actionId) {
    switch (actionId) {
        case ID_OPTIONKEY_MENU: {
            OptionMenu* pMenu = new OptionMenu();
            pMenu->Construct();
            pMenu->AddItem(LocString(L"IDS_MENU_STATUS"), ID_MENU_CHANGE_STATUS);
            pMenu->AddItem(LocString(L"IDS_MENU_BLOG"), ID_MENU_ADD);
            pMenu->AddItem(LocString(L"IDS_MENU_SETTINGS"), ID_MENU_SETTINGS);
            pMenu->AddActionEventListener(*this);
            pMenu->SetShowState(true);
            pMenu->Show();
            break;
        }

        case ID_MENU_CHANGE_STATUS: {
        	if (pStatusContextMenu != null) {
        		delete pStatusContextMenu;
        	    pStatusContextMenu = null;
        	}
        	int screenHeight = GetClientAreaBounds().height > 0 ? GetClientAreaBounds().height : 800;
            Osp::Graphics::Point anchorPos(20, screenHeight - 80);

            pStatusContextMenu = new Osp::Ui::Controls::ContextMenu();
            pStatusContextMenu->Construct(anchorPos, Osp::Ui::Controls::CONTEXT_MENU_STYLE_LIST);
            pStatusContextMenu->AddActionEventListener(*this);

            pStatusContextMenu->AddItem(LocString(L"IDS_STATUS_ONLINE"), ID_STATUS_ONLINE);
            pStatusContextMenu->AddItem(LocString(L"IDS_STATUS_AWAY"), ID_STATUS_AWAY);
            pStatusContextMenu->AddItem(LocString(L"IDS_STATUS_BUSY"), ID_STATUS_BUSY);
            pStatusContextMenu->AddItem(LocString(L"IDS_STATUS_INVISIBLE"), ID_STATUS_INVISIBLE);

            pStatusContextMenu->SetShowState(true);
            pStatusContextMenu->Show();
            break;
        }

        case ID_STATUS_ONLINE:
            myStatus = Mrim::Status::ONLINE;
            if (pConnection != null) pConnection->ChangeStatus(Mrim::Status::ONLINE);
            SchedulePopulate();
            break;

        case ID_STATUS_AWAY:
            myStatus = Mrim::Status::AWAY;
            if (pConnection != null) pConnection->ChangeStatus(Mrim::Status::AWAY);
            SchedulePopulate();
            break;

        case ID_STATUS_BUSY:
            myStatus = Mrim::Status::XSTATUS;
            if (pConnection != null) pConnection->ChangeStatus(Mrim::Status::XSTATUS);
            SchedulePopulate();
            break;

        case ID_STATUS_INVISIBLE:
            myStatus = Mrim::Status::INVISIBLE;
            if (pConnection != null) pConnection->ChangeStatus(Mrim::Status::INVISIBLE);
            SchedulePopulate();
            break;

        case ID_MENU_SETTINGS: {
            AggConnection* pConn = pConnection;
            DetachListeners();
            FormNavigator::GoToSettings(pConn, this);
            break;
        }

        case ID_SOFTKEY_PROFILE: {
            FormNavigator::GoToProfile(pConnection, L"", L"", null);
            break;
        }

        case ID_MENU_ADD: {
            SendUserEvent(USER_EVENT_GOTO_BLOG, null);
            break;
        }

        case ID_SOFTKEY_LOGOUT: {
            AppSettings::ClearCredentials();

            AggConnection* pConn = pConnection;
            if (pConn != null) pConn->ConsumePendingNotificationSender();
            DetachListeners();

            FormNavigator::GoToLogin(pConn, this);
            break;
        }

        default:
            break;
    }
}
