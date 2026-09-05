#include "Ui/ContactListForm.h"
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
    pConnection(null), pGroupedList(null), pItemFormat(null),
    pSavedGroups(null), pSavedContacts(null), strangersGroupIndex(-1),
    pBitmapOnline(null), pBitmapAway(null), pBitmapBusy(null), pBitmapOffline(null) {}

ContactListForm::~ContactListForm(void) {
    DetachListeners();

    delete pItemFormat;
    if (pSavedGroups != null) { pSavedGroups->RemoveAll(true); delete pSavedGroups; }
    if (pSavedContacts != null) { pSavedContacts->RemoveAll(true); delete pSavedContacts; }

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
    SetTitleText(L"Контакти");

    SetOptionkeyActionId(ID_OPTIONKEY_MENU);
    AddOptionkeyActionListener(*this);

    // SOFTKEY_0 підписана в IDF_CONT як "Settings" і поки веде прямо в
    // налаштування (те саме є пунктом меню OptionKey).
    SetSoftkeyActionId(SOFTKEY_0, ID_SOFTKEY_PROFILE);
    AddSoftkeyActionListener(SOFTKEY_0, *this);

    SetSoftkeyActionId(SOFTKEY_1, ID_SOFTKEY_EXIT);
    AddSoftkeyActionListener(SOFTKEY_1, *this);

    pGroupedList = static_cast<GroupedList*>(GetControl(L"IDC_CONTACT_LIST"));
    if (pGroupedList != null) {
        pGroupedList->AddGroupedItemEventListener(*this);

        pItemFormat = new CustomListItemFormat();
        pItemFormat->Construct();
        pItemFormat->AddElement(ELEM_ICON,  Rectangle(15, 14, 32, 32));
        pItemFormat->AddElement(ELEM_NAME,  Rectangle(60, 10, 340, 40));
        pItemFormat->AddElement(ELEM_BADGE, Rectangle(405, 10, 50, 40));
    }

    // Іконок може не бути в ресурсах - тоді GetBitmapN поверне null, і
    // рядки просто намалюються без значка статусу.
    AppResource* pRes = Application::GetInstance()->GetAppResource();
    if (pRes != null) {
        pBitmapOnline  = pRes->GetBitmapN(L"Statuses/Online.png");
        pBitmapAway    = pRes->GetBitmapN(L"Statuses/Away.png");
        pBitmapBusy    = pRes->GetBitmapN(L"Statuses/Busy.png");
        pBitmapOffline = pRes->GetBitmapN(L"Statuses/Offline.png");
    }

    // Кеш міг приїхати ще до створення форми (повернення з чату).
    if (pSavedContacts != null) PopulateList();

    return E_SUCCESS;
}

result ContactListForm::OnTerminating(void) {
    return E_SUCCESS;
}

// --- Слухачі ---

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
    // Не малюємо просто з місця виклику: сюди потрапляють і з обробки
    // вхідного пакета (зміна статусу контакту, нове повідомлення), а
    // перемальовування в цей момент валилось з "control has not been
    // constructed yet". Наступний оберт циклу подій - безпечний.
    SendUserEvent(USER_EVENT_POPULATE, null);
}

void ContactListForm::OnUserEventReceivedN(long requestId, IList* pArgs) {
    if (requestId == USER_EVENT_ATTACH_LISTENER) {
        AttachListeners();
    } else if (requestId == USER_EVENT_POPULATE) {
        PopulateList();
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
    SetTitleText(connected ? L"Контакти" : L"Контакти (з'єднання...)");

    if (GetParent() != null) {
        RequestRedraw(true);
        Draw();
        Show();
    }
}

// --- Дані ---

void ContactListForm::PublishKnownContacts(void) {
    if (pConnection == null || pSavedContacts == null) return;

    // MessageRouter копіює рядки собі, тож локальний список можна
    // звільнити одразу після виклику.
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

    // Копіюємо: список у MrimContacts переживе не всі перемальовування.
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
        pSavedContacts->Add(*pCopy);
    }

    PublishKnownContacts();
    SchedulePopulate();
}

// --- Малювання списку ---

const Bitmap* ContactListForm::GetStatusBitmap(unsigned long status) const {
    if (status == Mrim::Status::OFFLINE || status == Mrim::Status::INVISIBLE) return pBitmapOffline;
    if (status == Mrim::Status::AWAY) return pBitmapAway;
    if (status == Mrim::Status::XSTATUS) return pBitmapBusy;
    return pBitmapOnline; // ONLINE та інші ненульові коди
}

CustomListItem* ContactListForm::CreateRow(const String& title, const String& email, const Bitmap* pIcon) {
    CustomListItem* pItem = new CustomListItem();
    pItem->Construct(ROW_HEIGHT);
    if (pItemFormat != null) pItem->SetItemFormat(*pItemFormat);

    // Bitmap-варіант SetElement у цьому SDK не має значення за
    // замовчуванням для другого (focused) параметра - null передаємо явно.
    if (pIcon != null) pItem->SetElement(ELEM_ICON, *pIcon, null);
    pItem->SetElement(ELEM_NAME, title);

    int unread = (pConnection != null) ? pConnection->GetUnreadCount(email) : 0;
    if (unread > 0) pItem->SetElement(ELEM_BADGE, Integer::ToString(unread));

    return pItem;
}

int ContactListForm::AddStrangersGroup(int groupIndex) {
    IList* pStrangers = (pConnection != null) ? pConnection->GetStrangerEmails() : null;
    if (pStrangers == null || pStrangers->GetCount() == 0) return -1;

    pGroupedList->AddGroup(L"Невідомі", null, groupIndex);

    for (int i = 0; i < pStrangers->GetCount(); i++) {
        String* pEmail = static_cast<String*>(pStrangers->GetAt(i));
        if (pEmail == null) continue;

        // Без іконки статусу: на невідомих ми не підписані, тож
        // MRIM_CS_USER_STATUS для них не приходить.
        pGroupedList->AddItem(groupIndex, *CreateRow(*pEmail, *pEmail, null), i);
    }

    return groupIndex;
}

void ContactListForm::PopulateList(void) {
    if (pGroupedList == null || pSavedContacts == null) return;

    pGroupedList->RemoveAllGroups();
    strangersGroupIndex = -1;

    int groupCount = (pSavedGroups != null) ? pSavedGroups->GetCount() : 0;
    int contactCount = pSavedContacts->GetCount();

    // Групи мають існувати до додавання елементів у них.
    int listedGroups = 0;
    if (groupCount > 0) {
        for (int g = 0; g < groupCount; g++) {
            GroupInfo* pGroup = static_cast<GroupInfo*>(pSavedGroups->GetAt(g));
            String name = (pGroup != null && !pGroup->name.IsEmpty())
                        ? pGroup->name
                        : (L"Група " + Integer::ToString(g + 1));
            pGroupedList->AddGroup(name, null, g);
        }
        listedGroups = groupCount;
    } else if (contactCount > 0) {
        pGroupedList->AddGroup(L"Всі контакти", null, 0);
        listedGroups = 1;
    }

    // Один прохід по контактах. Раніше для КОЖНОЇ групи перебирався
    // весь список контактів - O(групи × контакти) на кожне вхідне
    // повідомлення; тепер O(групи + контакти).
    for (int c = 0; c < contactCount; c++) {
        ContactInfo* pContact = static_cast<ContactInfo*>(pSavedContacts->GetAt(c));
        if (pContact == null || listedGroups == 0) continue;

        // Контакт із групою поза списком показуємо в першій, інакше він
        // просто зник би з екрана.
        int group = (pContact->groupId < static_cast<unsigned long>(listedGroups))
                  ? static_cast<int>(pContact->groupId)
                  : 0;

        String title = pContact->nickname.IsEmpty() ? pContact->email : pContact->nickname;
        pGroupedList->AddItem(group, *CreateRow(title, pContact->email, GetStatusBitmap(pContact->status)), c);
    }

    strangersGroupIndex = AddStrangersGroup(listedGroups);

    if (GetParent() != null) {
        pGroupedList->RequestRedraw(true);
        pGroupedList->Draw();
        pGroupedList->Show();
    }
}

// --- Взаємодія ---

void ContactListForm::OnItemStateChanged(const Control& source, int groupIndex, int itemIndex, int itemId, ItemStatus status) {
    String targetName;
    String targetEmail;

    if (strangersGroupIndex >= 0 && groupIndex == strangersGroupIndex) {
        // У групі "Невідомі" itemId - індекс у списку невідомих,
        // а не у pSavedContacts.
        IList* pStrangers = (pConnection != null) ? pConnection->GetStrangerEmails() : null;
        if (pStrangers == null || itemId < 0 || itemId >= pStrangers->GetCount()) return;

        String* pEmail = static_cast<String*>(pStrangers->GetAt(itemId));
        if (pEmail == null) return;

        targetEmail = *pEmail;
        targetName = *pEmail; // нікнейма для невідомого немає
    } else {
        if (pSavedContacts == null || itemId < 0 || itemId >= pSavedContacts->GetCount()) return;

        ContactInfo* pContact = static_cast<ContactInfo*>(pSavedContacts->GetAt(itemId));
        if (pContact == null) return;

        targetName = pContact->nickname;
        targetEmail = pContact->email;
    }

    // Знімаємо слухачів ДО створення чату: ChatForm::Initialize()
    // зсередини скидає лічильник непрочитаних цього контакту, а це б
    // смикнуло наш же OnUnreadCountChanged і поставило в чергу подію
    // для форми, яку за мить приберуть.
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
            pMenu->AddItem(L"Змінити статус", ID_MENU_CHANGE_STATUS);
            pMenu->AddItem(L"Вийти з акаунту", ID_MENU_LOGOUT);
            pMenu->AddItem(L"Налаштування", ID_MENU_SETTINGS);
            pMenu->AddActionEventListener(*this);
            pMenu->SetShowState(true);
            pMenu->Show();
            break;
        }

        case ID_MENU_CHANGE_STATUS: {
            // Протокол 1.8: статус - одне число, без xstatus-полів.
            // "Не турбувати" (0x4) сервер приймає лише з 1.15+, тому
            // тут його не пропонуємо.
            OptionMenu* pMenu = new OptionMenu();
            pMenu->Construct();
            pMenu->AddItem(L"Онлайн", ID_STATUS_ONLINE);
            pMenu->AddItem(L"Відійшов", ID_STATUS_AWAY);
            pMenu->AddItem(L"Невидимка", ID_STATUS_INVISIBLE);
            pMenu->AddActionEventListener(*this);
            pMenu->SetShowState(true);
            pMenu->Show();
            break;
        }

        case ID_STATUS_ONLINE:
            if (pConnection != null) pConnection->ChangeStatus(Mrim::Status::ONLINE);
            break;

        case ID_STATUS_AWAY:
            if (pConnection != null) pConnection->ChangeStatus(Mrim::Status::AWAY);
            break;

        case ID_STATUS_INVISIBLE:
            if (pConnection != null) pConnection->ChangeStatus(Mrim::Status::INVISIBLE);
            break;

        case ID_MENU_SETTINGS: {
            AggConnection* pConn = pConnection;
            DetachListeners();
            FormNavigator::GoToSettings(pConn, this);
            break;
        }

        case ID_MENU_LOGOUT: {
            // Забуваємо дані входу, щоб автологін не підхопив їх знову.
            AppSettings::ClearCredentials();

            AggConnection* pConn = pConnection;
            DetachListeners();

            // З'єднання не знищуємо: форма входу перевикористає його
            // (Reset() всередині Initialize), і ми не лишаємо
            // осиротілий об'єкт із живим сокетом.
            FormNavigator::GoToLogin(pConn, this);
            break;
        }

        case ID_SOFTKEY_EXIT:
            Application::GetInstance()->Terminate();
            break;

        default:
            break;
    }
}
