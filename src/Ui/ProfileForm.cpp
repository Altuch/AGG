#include "Ui/ProfileForm.h"
#include "Ui/FormNavigator.h"
#include "MRIM/MrimProtocol.h"
#include <FApp.h>

using namespace Osp::App;
using namespace Osp::Base;
using namespace Osp::Base::Collection;
using namespace Osp::Graphics;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;

ProfileForm::ProfileForm(void) :
    pConnection(null), pList(null), pIdentityFormat(null), pFieldFormat(null),
    pBitmapOnline(null), pBitmapAway(null), pBitmapBusy(null), pBitmapOffline(null),
    pReturnTo(null), hasProfile(false), isNotFound(false) {}

ProfileForm::~ProfileForm(void) {
    if (pConnection != null) pConnection->SetProfileListener(null);

    delete pIdentityFormat;
    delete pFieldFormat;

    delete pBitmapOnline;
    delete pBitmapAway;
    delete pBitmapBusy;
    delete pBitmapOffline;
}

result ProfileForm::Initialize(AggConnection* pConn,
                               const String& contactName,
                               const String& contactEmail,
                               Form* pReturn) {
    pConnection = pConn;
    this->contactName = contactName;
    this->contactEmail = contactEmail;
    pReturnTo = pReturn;
    return Construct(L"IDF_PROFILE");
}

result ProfileForm::OnInitializing(void) {
    SetTitleText(contactEmail.IsEmpty() ? String(L"Мій профіль") : GetDisplayName());

    SetSoftkeyActionId(SOFTKEY_0, ID_SOFTKEY_BACK);
    AddSoftkeyActionListener(SOFTKEY_0, *this);

    pList = static_cast<GroupedList*>(GetControl(L"IDC_PROFILE_LIST"));

    pIdentityFormat = new CustomListItemFormat();
    pIdentityFormat->Construct();
    pIdentityFormat->AddElement(ELEM_AVATAR, Rectangle(15, 20, 56, 56));
    pIdentityFormat->AddElement(ELEM_TITLE,  Rectangle(85, 18, 375, 36));
    pIdentityFormat->AddElement(ELEM_SUB,    Rectangle(85, 56, 375, 30));

    pFieldFormat = new CustomListItemFormat();
    pFieldFormat->Construct();
    pFieldFormat->AddElement(ELEM_LABEL, Rectangle(15, 8, 190, 40));
    pFieldFormat->AddElement(ELEM_VALUE, Rectangle(210, 8, 255, 40));

    AppResource* pRes = Application::GetInstance()->GetAppResource();
    if (pRes != null) {
        pBitmapOnline  = pRes->GetBitmapN(L"Statuses/Online.png");
        pBitmapAway    = pRes->GetBitmapN(L"Statuses/Away.png");
        pBitmapBusy    = pRes->GetBitmapN(L"Statuses/Busy.png");
        pBitmapOffline = pRes->GetBitmapN(L"Statuses/Offline.png");
    }

    if (pConnection != null) {
        pConnection->SetProfileListener(this);
        pConnection->RequestProfile(contactEmail);
    }

    SchedulePopulate();

    return E_SUCCESS;
}

result ProfileForm::OnTerminating(void) {
    return E_SUCCESS;
}

void ProfileForm::SchedulePopulate(void) {
    SendUserEvent(USER_EVENT_POPULATE, null);
}

void ProfileForm::OnUserEventReceivedN(long requestId, IList* pArgs) {
    if (requestId == USER_EVENT_POPULATE) PopulateList();

    if (pArgs != null) {
        pArgs->RemoveAll(true);
        delete pArgs;
    }
}

String ProfileForm::GetDisplayName(void) const {
    if (!contactEmail.IsEmpty()) {
        if (!contactName.IsEmpty()) return contactName;
        return profile.nickname.IsEmpty() ? contactEmail : profile.nickname;
    }

    String own = (pConnection != null) ? pConnection->GetNickname() : String(L"");
    if (own.IsEmpty()) own = profile.nickname;
    if (own.IsEmpty() && pConnection != null) own = pConnection->GetLogin();
    return own;
}

String ProfileForm::GetDisplayEmail(void) const {
    if (!contactEmail.IsEmpty()) return contactEmail;
    return (pConnection != null) ? pConnection->GetLogin() : String(L"");
}

const Bitmap* ProfileForm::GetAvatarBitmap(void) const {
    if (!profile.status.IsEmpty()) {
        int status = 0;
        if (!IsFailed(Integer::Parse(profile.status, status))) {
            if (status == (int)Mrim::Status::AWAY) return pBitmapAway;
            if (status == (int)Mrim::Status::XSTATUS) return pBitmapBusy;
            if (status == (int)Mrim::Status::OFFLINE) return pBitmapOffline;
            return pBitmapOnline;
        }
    }

    if (contactEmail.IsEmpty()) return pBitmapOnline;

    return null;
}


CustomListItem* ProfileForm::CreateIdentityRow(void) const {
    CustomListItem* pItem = new CustomListItem();
    pItem->Construct(IDENTITY_ROW_HEIGHT);
    pItem->SetItemFormat(*pIdentityFormat);

    const Bitmap* pAvatar = GetAvatarBitmap();
    if (pAvatar != null) pItem->SetElement(ELEM_AVATAR, *pAvatar, null);

    pItem->SetElement(ELEM_TITLE, GetDisplayName());
    pItem->SetElement(ELEM_SUB, GetDisplayEmail());
    return pItem;
}

CustomListItem* ProfileForm::CreateFieldRow(const String& label, const String& value) const {
    CustomListItem* pItem = new CustomListItem();
    pItem->Construct(FIELD_ROW_HEIGHT);
    pItem->SetItemFormat(*pFieldFormat);
    pItem->SetElement(ELEM_LABEL, label);
    pItem->SetElement(ELEM_VALUE, value);
    return pItem;
}

int ProfileForm::AddFieldRow(int group, int index, const String& label, const String& value) {
    if (value.IsEmpty()) return index;
    pList->AddItem(group, *CreateFieldRow(label, value), index);
    return index + 1;
}

void ProfileForm::PopulateList(void) {
    if (pList == null) return;

    SetTitleText(contactEmail.IsEmpty() ? String(L"Мій профіль") : GetDisplayName());

    pList->RemoveAllGroups();

    pList->AddGroup(contactEmail.IsEmpty() ? L"Мій профіль" : L"Контакт", null, GROUP_IDENTITY);
    pList->AddItem(GROUP_IDENTITY, *CreateIdentityRow(), 0);

    pList->AddGroup(L"Анкета", null, GROUP_DETAILS);

    int index = 0;
    if (!hasProfile && !isNotFound) {
        index = AddFieldRow(GROUP_DETAILS, index, L"Стан", L"Завантаження...");
    } else if (isNotFound) {
        index = AddFieldRow(GROUP_DETAILS, index, L"Стан", L"Анкету не знайдено");
    } else {
        String fullName = profile.firstName;
        if (!profile.lastName.IsEmpty()) {
            if (!fullName.IsEmpty()) fullName.Append(L" ");
            fullName.Append(profile.lastName);
        }

        index = AddFieldRow(GROUP_DETAILS, index, L"Ім'я", fullName);
        index = AddFieldRow(GROUP_DETAILS, index, L"Місто", profile.location);
        index = AddFieldRow(GROUP_DETAILS, index, L"Народження", profile.birthday);
        index = AddFieldRow(GROUP_DETAILS, index, L"Зодіак", profile.zodiac);
        index = AddFieldRow(GROUP_DETAILS, index, L"Телефон", profile.phone);

        if (profile.sex == L"1") index = AddFieldRow(GROUP_DETAILS, index, L"Стать", L"Чоловіча");
        else if (profile.sex == L"2") index = AddFieldRow(GROUP_DETAILS, index, L"Стать", L"Жіноча");

        if (index == 0) {
            index = AddFieldRow(GROUP_DETAILS, index, L"Стан", L"Сервер не має даних");
        }
    }

    if (GetParent() != null) {
        pList->RequestRedraw(true);
        pList->Draw();
        pList->Show();
    }
}

void ProfileForm::OnProfileReceived(const ProfileInfo& info) {
    profile.username  = info.username;
    profile.nickname  = info.nickname;
    profile.domain    = info.domain;
    profile.firstName = info.firstName;
    profile.lastName  = info.lastName;
    profile.location  = info.location;
    profile.birthday  = info.birthday;
    profile.zodiac    = info.zodiac;
    profile.phone     = info.phone;
    profile.sex       = info.sex;
    profile.status    = info.status;

    hasProfile = true;
    isNotFound = false;

    SchedulePopulate();
}

void ProfileForm::OnProfileNotFound(void) {
    hasProfile = false;
    isNotFound = true;
    SchedulePopulate();
}

void ProfileForm::Leave(void) {
    if (pConnection != null) pConnection->SetProfileListener(null);

    if (pReturnTo != null) {
        FormNavigator::Back(pReturnTo, this);
        return;
    }

    AggConnection* pConn = pConnection;
    pConnection = null;
    FormNavigator::GoToContactList(pConn, this);
}

void ProfileForm::OnActionPerformed(const Control& source, int actionId) {
    switch (actionId) {
        case ID_SOFTKEY_BACK:
            Leave();
            break;

        default:
            break;
    }
}
