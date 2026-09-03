#include "ContactListForm.h"
#include "ChatForm.h"
#include "Settings.h"
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
    pSavedGroups(null), pSavedContacts(null) {}

ContactListForm::~ContactListForm(void) {
    if (pConnection != null) pConnection->SetContactListListener(null);
    if (pItemFormat != null) delete pItemFormat;
    if (pSavedGroups != null) { pSavedGroups->RemoveAll(true); delete pSavedGroups; }
    if (pSavedContacts != null) { pSavedContacts->RemoveAll(true); delete pSavedContacts; }
}

result ContactListForm::Initialize(AggConnection* pConn) {
    pConnection = pConn;
    result r = Form::Construct(L"IDF_CONT");
    if (IsFailed(r)) return r;

    return E_SUCCESS;
}

<<<<<<< Updated upstream
=======
void ContactListForm::ScheduleAttachContactListener(void) {
    SendUserEvent(USER_EVENT_ATTACH_LISTENER, null);
}


>>>>>>> Stashed changes
void ContactListForm::AttachContactListener(void) {
    if (pConnection != null) pConnection->SetContactListListener(this);
}

result ContactListForm::OnInitializing(void) {
    SetTitleText(L"Контакти");
    SetOptionkeyActionId(ID_OPTIONKEY_SETTINGS);
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
        pItemFormat->AddElement(1, Rectangle(15, 10, 440, 40));
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

            // Правильна перевірка онлайну (покриває X-Статуси)
            if (pContact->status != 0 && pContact->status != 0x80000001) {
                displayName = L"● " + displayName;
            } else {
                displayName = L"○ " + displayName;
            }

            CustomListItem* pItem = new CustomListItem();
            pItem->Construct(60);
            if (pItemFormat != null) pItem->SetItemFormat(*pItemFormat);
            pItem->SetElement(1, displayName);
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

                    if (pContact->status != 0 && pContact->status != 0x80000001) {
                        displayName = L"● " + displayName;
                    } else {
                        displayName = L"○ " + displayName;
                    }

                    CustomListItem* pItem = new CustomListItem();
                    pItem->Construct(60);
                    if (pItemFormat != null) pItem->SetItemFormat(*pItemFormat);
                    pItem->SetElement(1, displayName);
                    pGroupedList->AddItem(gIdx, *pItem, cIdx);
                }
            }
        }
    }

    // Примусово перемальовуємо екран як в офіційному прикладі
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

    // Викликаємо оновлення списку НАПРЯМУ
    PopulateList();
}

void ContactListForm::OnUserEventReceivedN(long requestId, IList* pArgs) {
    if (requestId == USER_EVENT_ATTACH_LISTENER) {
        AttachContactListener();
    }
    if (pArgs != null) {
        pArgs->RemoveAll(true);
        delete pArgs;
    }
}

void ContactListForm::OnItemStateChanged(const Control& source, int groupIndex, int itemIndex, int itemId, ItemStatus status) {
    if (pSavedContacts == null || itemId < 0 || itemId >= pSavedContacts->GetCount()) return;

    ContactInfo* pTarget = static_cast<ContactInfo*>(pSavedContacts->GetAt(itemId));
    if (pTarget == null) return;

    ChatForm* pChatForm = new ChatForm();
    pChatForm->Initialize(pConnection, pTarget->nickname, pTarget->email);

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
                pFrame->RemoveControl(*this);
            }
            break;
        }
        case ID_SOFTKEY_EXIT: {
            Application::GetInstance()->Terminate();
            break;
        }
    }
}
