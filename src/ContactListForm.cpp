#include "ContactListForm.h"
#include "ChatForm.h"
#include "Settings.h"
#include "Form1.h"
#include <FApp.h>
#include <FGraphics.h>

using namespace Osp::Ui;
using namespace Osp::Ui::Controls;
using namespace Osp::App;
using namespace Osp::Base;
using namespace Osp::Base::Collection;
using namespace Osp::Graphics;

ContactListForm::ContactListForm(void) :
    pConnection(null), pGroupedList(null), pItemFormat(null),
    pSavedGroups(null), pSavedContacts(null), strangersGroupIndex(-1),
    pBitmapOnline(null), pBitmapAway(null), pBitmapDnd(null), pBitmapOffline(null) {}

ContactListForm::~ContactListForm(void) {
	DetachListeners();
    if (pItemFormat != null) delete pItemFormat;
    if (pSavedGroups != null) { pSavedGroups->RemoveAll(true); delete pSavedGroups; }
    if (pSavedContacts != null) { pSavedContacts->RemoveAll(true); delete pSavedContacts; }
    if (pBitmapOnline != null) delete pBitmapOnline;
    if (pBitmapAway != null) delete pBitmapAway;
    if (pBitmapDnd != null) delete pBitmapDnd;
    if (pBitmapOffline != null) delete pBitmapOffline;
}

const Bitmap* ContactListForm::GetStatusBitmap(unsigned long status) const {
    if (status == 0 || status == 0x80000001) return pBitmapOffline;
    if (status == 0x2) return pBitmapAway;
    if (status == 0x4) return pBitmapDnd;
    return pBitmapOnline;
}

result ContactListForm::Initialize(AggConnection* pConn) {
    pConnection = pConn;
    result r = Form::Construct(L"IDF_CONT");
    if (IsFailed(r)) return r;

    return E_SUCCESS;
}

void ContactListForm::ScheduleAttachContactListener(void) {
    SendUserEvent(USER_EVENT_ATTACH_LISTENER, null);
}

void ContactListForm::AttachContactListener(void) {
	if (pConnection == null) return;
	pConnection->SetContactListListener(this);
	pConnection->SetContactListVisibleListener(this);
	pConnection->SetConnectionStateListener(this);
}

void ContactListForm::DetachListeners(void) {
    if (pConnection == null) return;
    pConnection->SetContactListListener(null);
    pConnection->SetContactListVisibleListener(null);
    pConnection->SetConnectionStateListener(this);
}

void ContactListForm::OnUnreadCountChanged(void) {
	SendUserEvent(USER_EVENT_CONTACTS_READY, null);
	}

void ContactListForm::OnConnectionStateChanged(bool connected) {
    SetTitleText(connected ? L"Контакти" : L"Контакти (з'єднання...)");
    if (GetParent() != null) {
        RequestRedraw(true);
        Draw();
        Show();
    }
}

result ContactListForm::OnInitializing(void) {
    SetTitleText(L"Контакти");
    SetOptionkeyActionId(ID_OPTIONKEY_MENU);
    AddOptionkeyActionListener(*this);

    SetSoftkeyActionId(SOFTKEY_0, ID_OPTIONKEY_SETTINGS);
    AddSoftkeyActionListener(SOFTKEY_0, *this);

    SetSoftkeyActionId(SOFTKEY_1, ID_SOFTKEY_EXIT);
    AddSoftkeyActionListener(SOFTKEY_1, *this);

    pGroupedList = static_cast<GroupedList*>(GetControl(L"IDC_CONTACT_LIST"));
    if (pGroupedList == null) pGroupedList = static_cast<GroupedList*>(GetControl(L"IDC_Contact_LIST"));

    if (pGroupedList != null) {
        pGroupedList->AddGroupedItemEventListener(*this);
        pItemFormat = new CustomListItemFormat();
        pItemFormat->Construct();
        pItemFormat->AddElement(2, Rectangle(15, 14, 32, 32));
        pItemFormat->AddElement(1, Rectangle(60, 10, 340, 40));
        pItemFormat->AddElement(3, Rectangle(405, 10, 50, 40));
    }

    AppResource* pAppResource = Application::GetInstance()->GetAppResource();
    if (pAppResource != null) {
        pBitmapOnline  = pAppResource->GetBitmapN(L"Statuses/Online.png");
        pBitmapAway    = pAppResource->GetBitmapN(L"Statuses/Away.png");
        pBitmapDnd     = pAppResource->GetBitmapN(L"Statuses/Busy.png");
        pBitmapOffline = pAppResource->GetBitmapN(L"Statuses/Offline.png");
    }

    if (pSavedGroups != null && pSavedContacts != null) PopulateList();
    return E_SUCCESS;
}

result ContactListForm::OnTerminating(void) { return E_SUCCESS; }

void ContactListForm::PopulateList(void) {
    if (pGroupedList == null || pSavedGroups == null || pSavedContacts == null) return;

    pGroupedList->RemoveAllItems();

    int groupsCount = pSavedGroups->GetCount();
    int contactsCount = pSavedContacts->GetCount();

    if (groupsCount == 0 && contactsCount > 0) {
        pGroupedList->AddGroup(L"Всі контакти", null, 0);
        for (int cIdx = 0; cIdx < contactsCount; cIdx++) {
            ContactInfo* pContact = static_cast<ContactInfo*>(pSavedContacts->GetAt(cIdx));
            if (pContact == null) continue;

            String displayName = pContact->nickname.IsEmpty() ? pContact->email : pContact->nickname;

            CustomListItem* pItem = new CustomListItem();
            pItem->Construct(60);
            if (pItemFormat != null) pItem->SetItemFormat(*pItemFormat);
            const Bitmap* pStatusBitmap = GetStatusBitmap(pContact->status);
            if (pStatusBitmap != null) pItem->SetElement(2, *pStatusBitmap, null);
            pItem->SetElement(1, displayName);
            int unreadCount = (pConnection != null) ? pConnection->GetUnreadCount(pContact->email) : 0;
            if (unreadCount > 0) pItem->SetElement(3, Integer::ToString(unreadCount));
            pGroupedList->AddItem(0, *pItem, cIdx);
        }
    } else {
        for (int gIdx = 0; gIdx < groupsCount; gIdx++) {
            GroupInfo* pGroup = static_cast<GroupInfo*>(pSavedGroups->GetAt(gIdx));
            if (pGroup == null) continue;

            String groupName = pGroup->name.IsEmpty() ? (L"Група " + Integer::ToString(gIdx + 1)) : pGroup->name;
            pGroupedList->AddGroup(groupName, null, gIdx);

            for (int cIdx = 0; cIdx < contactsCount; cIdx++) {
                ContactInfo* pContact = static_cast<ContactInfo*>(pSavedContacts->GetAt(cIdx));
                if (pContact == null) continue;

                bool inThisGroup = false;
                if (pContact->groupId == static_cast<unsigned long>(gIdx)) inThisGroup = true;
                else if (gIdx == 0 && pContact->groupId >= static_cast<unsigned long>(groupsCount)) inThisGroup = true;

                if (inThisGroup) {
                    String displayName = pContact->nickname.IsEmpty() ? pContact->email : pContact->nickname;

                    CustomListItem* pItem = new CustomListItem();
                    pItem->Construct(60);
                    if (pItemFormat != null) pItem->SetItemFormat(*pItemFormat);
                    const Bitmap* pStatusBitmap = GetStatusBitmap(pContact->status);
                    if (pStatusBitmap != null) pItem->SetElement(2, *pStatusBitmap, null);
                    pItem->SetElement(1, displayName);
                    int unreadCount = (pConnection != null) ? pConnection->GetUnreadCount(pContact->email) : 0;
                    if (unreadCount > 0) pItem->SetElement(3, Integer::ToString(unreadCount));
                    pGroupedList->AddItem(gIdx, *pItem, cIdx);
                }
            }
        }
    }
    IList* pStrangers = (pConnection != null) ? pConnection->GetStrangerEmails() : null;
        if (pStrangers != null && pStrangers->GetCount() > 0) {
            int nextGroupIndex = (groupsCount == 0 && contactsCount > 0) ? 1 : groupsCount;
            strangersGroupIndex = nextGroupIndex;
            pGroupedList->AddGroup(L"Невідомі", null, nextGroupIndex);

            for (int sIdx = 0; sIdx < pStrangers->GetCount(); sIdx++) {
                String* pEmail = static_cast<String*>(pStrangers->GetAt(sIdx));
                if (pEmail == null) continue;

                CustomListItem* pItem = new CustomListItem();
                pItem->Construct(60);
                if (pItemFormat != null) pItem->SetItemFormat(*pItemFormat);
                // Немає статус-іконки - для невідомих контактів ми не
                // отримуємо MRIM_CS_USER_STATUS (ми ж на них не підписані).
                pItem->SetElement(1, *pEmail);
                int unreadCount = pConnection->GetUnreadCount(*pEmail);
                if (unreadCount > 0) pItem->SetElement(3, Integer::ToString(unreadCount));
                pGroupedList->AddItem(nextGroupIndex, *pItem, sIdx);
            }
        }
    if (GetParent() != null) {
        pGroupedList->RequestRedraw(true);
        pGroupedList->Draw();
        pGroupedList->Show();
    }
}

void ContactListForm::OnContactListReceived(IList* pGroups, IList* pContacts) {
    if (pGroups == null || pContacts == null) return;

    if (pSavedGroups == null) {
        pSavedGroups = new ArrayList();
        pSavedGroups->Construct();
    } else pSavedGroups->RemoveAll(true);

    if (pSavedContacts == null) {
        pSavedContacts = new ArrayList();
        pSavedContacts->Construct();
    } else pSavedContacts->RemoveAll(true);

    for (int i = 0; i < pGroups->GetCount(); i++) {
        GroupInfo* pG = static_cast<GroupInfo*>(pGroups->GetAt(i));
        GroupInfo* pGNew = new GroupInfo();
        pGNew->name = pG->name;
        pGNew->flags = pG->flags;
        pSavedGroups->Add(*pGNew);
    }

    for (int i = 0; i < pContacts->GetCount(); i++) {
        ContactInfo* pC = static_cast<ContactInfo*>(pContacts->GetAt(i));
        ContactInfo* pCNew = new ContactInfo();
        pCNew->email = pC->email;
        pCNew->nickname = pC->nickname;
        pCNew->status = pC->status;
        pCNew->groupId = pC->groupId;
        pCNew->flags = pC->flags;
        pSavedContacts->Add(*pCNew);
    }
    if (pConnection != null) {
            ArrayList knownEmails;
            knownEmails.Construct();
            for (int i = 0; i < pSavedContacts->GetCount(); i++) {
                ContactInfo* pC = static_cast<ContactInfo*>(pSavedContacts->GetAt(i));
                if (pC != null) knownEmails.Add(*(new String(pC->email)));
            }
            pConnection->SetKnownContactEmails(&knownEmails);
            knownEmails.RemoveAll(true);
        }
    SendUserEvent(USER_EVENT_CONTACTS_READY, null);
}

void ContactListForm::OnUserEventReceivedN(long requestId, IList* pArgs) {
    if (requestId == USER_EVENT_ATTACH_LISTENER) {
        AttachContactListener();
    } else if (requestId == USER_EVENT_CONTACTS_READY) {
            PopulateList();
    }
    if (pArgs != null) {
        pArgs->RemoveAll(true);
        delete pArgs;
    }
}

void ContactListForm::OnItemStateChanged(const Control& source, int groupIndex, int itemIndex, int itemId, ItemStatus status) {
	String targetName;
	String targetEmail;

	if (strangersGroupIndex >= 0 && groupIndex == strangersGroupIndex) {
	        // Тап по групі "Невідомі" - itemId тут індекс у СПИСКУ НЕВІДОМИХ
	        // (окремий простір індексів від pSavedContacts, див. PopulateList).
	        IList* pStrangers = (pConnection != null) ? pConnection->GetStrangerEmails() : null;
	        if (pStrangers == null || itemId < 0 || itemId >= pStrangers->GetCount()) return;
	        String* pEmail = static_cast<String*>(pStrangers->GetAt(itemId));
	        if (pEmail == null) return;
	        targetEmail = *pEmail;
	        targetName = *pEmail; // немає нікнейму - показуємо сам email
	    } else {
	        if (pSavedContacts == null || itemId < 0 || itemId >= pSavedContacts->GetCount())
	return;
	        ContactInfo* pTarget = static_cast<ContactInfo*>(pSavedContacts->GetAt(itemId));
	        if (pTarget == null) return;
	        targetName = pTarget->nickname;
	        targetEmail = pTarget->email;
	    }

    ContactInfo* pTarget = static_cast<ContactInfo*>(pSavedContacts->GetAt(itemId));
    if (pTarget == null) return;

    DetachListeners();

    ChatForm* pChatForm = new ChatForm();
    pChatForm->Initialize(pConnection, targetName, targetEmail);

    Frame* pFrame = Application::GetInstance()->GetAppFrame()->GetFrame();
    if (pFrame != null) {
        pFrame->AddControl(*pChatForm);
        pFrame->SetCurrentForm(*pChatForm);
        pChatForm->Draw();
        pChatForm->Show();
        pFrame->RemoveControl(*this);
    }
}

void ContactListForm::OnItemStateChanged(const Control& source, int groupIndex, int itemIndex, int elementId, int itemId, ItemStatus status) {
    OnItemStateChanged(source, groupIndex, itemIndex, itemId, status);
}

void ContactListForm::OnActionPerformed(const Control& source, int actionId) {
    Frame* pFrame = Application::GetInstance()->GetAppFrame()->GetFrame();
    switch (actionId) {
        case ID_OPTIONKEY_SETTINGS: {
            Settings* pSettings = new Settings();
            pSettings->Initialize(pConnection);
            if (pFrame != null) {
                pFrame->AddControl(*pSettings);
                pFrame->SetCurrentForm(*pSettings);
                pSettings->Draw();
                pSettings->Show();
                DetachListeners();
                pFrame->RemoveControl(*this);
            }
            break;
        }
        case ID_SOFTKEY_EXIT: {
            Application::GetInstance()->Terminate();
            break;
        }

        case ID_OPTIONKEY_MENU: {
            OptionMenu* pMenu = new OptionMenu();
            pMenu->Construct();
            pMenu->AddItem(L"Змінити статус", ID_MENU_CHANGE_STATUS);
            pMenu->AddItem(L"Вийти з акаунту", ID_MENU_LOGOUT);
            pMenu->AddItem(L"Налаштування", ID_OPTIONKEY_SETTINGS);
            pMenu->AddActionEventListener(*this);
            pMenu->SetShowState(true);
            pMenu->Show();
            break;
        }

        case ID_MENU_CHANGE_STATUS: {
            OptionMenu* pStatusMenu = new OptionMenu();
            pStatusMenu->Construct();
            pStatusMenu->AddItem(L"Онлайн", ID_STATUS_ONLINE);
            pStatusMenu->AddItem(L"Відійшов", ID_STATUS_AWAY);
            pStatusMenu->AddItem(L"Невидимка", ID_STATUS_INVISIBLE);
            pStatusMenu->AddActionEventListener(*this);
            pStatusMenu->SetShowState(true);
            pStatusMenu->Show();
            break;
        }

        case ID_STATUS_ONLINE: {
            if (pConnection != null) pConnection->ChangeStatus(0x1);
            break;
        }
        case ID_STATUS_AWAY: {
            if (pConnection != null) pConnection->ChangeStatus(0x2);
            break;
        }
        case ID_STATUS_INVISIBLE: {
            if (pConnection != null) pConnection->ChangeStatus(0x80000001);
            break;
        }

        case ID_MENU_LOGOUT: {
            AppRegistry* pReg = Application::GetInstance()->GetAppRegistry();
            if (pReg != null) {
                pReg->Remove(L"UserEmail");
                pReg->Remove(L"UserPassword");
                pReg->Save();
            }
            DetachListeners();
            if (pConnection != null) {
                delete pConnection;
                pConnection = null;
            }

            Form1* pForm1 = new Form1();
            pForm1->Initialize();

            if (pFrame != null) {
                pFrame->AddControl(*pForm1);
                pFrame->SetCurrentForm(*pForm1);
                pForm1->Draw();
                pForm1->Show();
                pFrame->RemoveControl(*this);
            }
            break;
        }
    }
}
