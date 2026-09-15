#include "Ui/ProfileForm.h"
#include "Core/Loc.h"
#include "Core/ContactSync.h"
#include "Ui/FormNavigator.h"
#include "Core/AppSettings.h"
#include <FApp.h>
#include <FMedia.h>

using namespace Osp::App;
using namespace Osp::Base;
using namespace Osp::Base::Collection;
using namespace Osp::Graphics;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;

ProfileForm::ProfileForm(void) :
    pConnection(null), pList(null), pIdentityFormat(null), pFieldFormat(null),
    pAvatarBitmap(null), pAvatarLoader(null),
    pReturnTo(null), hasProfile(false), isNotFound(false) {}

ProfileForm::~ProfileForm(void) {
    if (pConnection != null) pConnection->SetProfileListener(null);

    delete pAvatarLoader;
    pAvatarLoader = null;

    delete pAvatarBitmap;
    pAvatarBitmap = null;

    delete pIdentityFormat;
    delete pFieldFormat;
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
    SetTitleText(contactEmail.IsEmpty() ? String(LocString(L"IDS_PROFILE_OWN")) : GetDisplayName());

    SetSoftkeyActionId(SOFTKEY_0, ID_SOFTKEY_BACK);
    AddSoftkeyActionListener(SOFTKEY_0, *this);

    SetOptionkeyActionId(ID_OPTIONKEY_SAVE);
    AddOptionkeyActionListener(*this);
    SetSoftkeyText(SOFTKEY_0, LocString(L"IDS_SK_BACK"));

    pList = static_cast<GroupedList*>(GetControl(L"IDC_PROFILE_LIST"));

    int formW = GetClientAreaBounds().width;
    if (formW <= 0) formW = 480;
    int titleW = formW - 92 - 12;
    if (titleW < 80) titleW = 80;
    int fieldW = formW - 32;
    if (fieldW < 80) fieldW = 80;

    pIdentityFormat = new CustomListItemFormat();
    pIdentityFormat->Construct();

    pIdentityFormat->AddElement(ELEM_AVATAR, Rectangle(15, 14, 68, 68));

    pIdentityFormat->AddElement(
        ELEM_TITLE,
        Rectangle(92, 8, titleW, 44),
        40,
        Osp::Graphics::Color(255, 255, 255),
        Osp::Graphics::Color(255, 255, 255)
    );

    pIdentityFormat->AddElement(
        ELEM_SUB,
        Rectangle(92, 54, titleW, 38),
        30,
        Osp::Graphics::Color(175, 195, 235),
        Osp::Graphics::Color(255, 255, 255)
    );

    pFieldFormat = new CustomListItemFormat();
    pFieldFormat->Construct();
    pFieldFormat->AddElement(
            ELEM_LABEL,
            Rectangle(16, 8, fieldW, 28),
            22,
            Osp::Graphics::Color(60, 165, 240),
            Osp::Graphics::Color(60, 165, 240)
        );

    pFieldFormat->AddElement(
            ELEM_VALUE,
            Rectangle(16, 38, fieldW, 36),
            32,
            Osp::Graphics::Color(255, 255, 255),
            Osp::Graphics::Color(255, 255, 255)
        );

    if (pConnection != null) {
        pConnection->SetProfileListener(this);
        pConnection->RequestProfile(contactEmail);
    }

    RequestAvatar();

    SchedulePopulate();

    return E_SUCCESS;
}

void ProfileForm::RequestAvatar(void) {
    String email = GetDisplayEmail();
    if (email.IsEmpty()) return;

    delete pAvatarLoader;
    pAvatarLoader = new AvatarLoader();
    pAvatarLoader->SetListener(this);

    result r = pAvatarLoader->RequestAvatar(AppSettings::GetAvatarHost(),
                                            AppSettings::GetAvatarPort(),
                                            email,
                                            String(AvatarLoader::TYPE_AVATAR));
    if (IsFailed(r)) {
        delete pAvatarLoader;
        pAvatarLoader = null;
    }
}

void ProfileForm::OnAvatarLoaded(const String& email, Bitmap* pBitmap) {
    if (pBitmap == null) return;
    if (!email.Equals(GetDisplayEmail(), true)) {
        delete pBitmap;
        return;
    }

    delete pAvatarBitmap;
    pAvatarBitmap = pBitmap;
    SchedulePopulate();
}

void ProfileForm::OnAvatarFailed(const String& email) {
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
    return pAvatarBitmap;
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
    pItem->Construct(80);
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

    SetTitleText(contactEmail.IsEmpty() ? String(LocString(L"IDS_PROFILE_OWN")) : GetDisplayName());

    pList->RemoveAllGroups();

    pList->AddGroup(contactEmail.IsEmpty() ? LocString(L"IDS_PROFILE_OWN") : LocString(L"IDS_PROFILE_CONTACT"), null, GROUP_IDENTITY);
    pList->AddItem(GROUP_IDENTITY, *CreateIdentityRow(), 0);

    pList->AddGroup(LocString(L"IDS_PROFILE_DETAILS"), null, GROUP_DETAILS);

    int index = 0;
    if (!hasProfile && !isNotFound) {
        index = AddFieldRow(GROUP_DETAILS, index, LocString(L"IDS_F_STATE"), LocString(L"IDS_PROFILE_LOADING"));
    } else if (isNotFound) {
        index = AddFieldRow(GROUP_DETAILS, index, LocString(L"IDS_F_STATE"), LocString(L"IDS_PROFILE_NOTFOUND"));
    } else {
        String fullName = profile.firstName;
        if (!profile.lastName.IsEmpty()) {
            if (!fullName.IsEmpty()) fullName.Append(L" ");
            fullName.Append(profile.lastName);
        }

        index = AddFieldRow(GROUP_DETAILS, index, LocString(L"IDS_F_NAME"), fullName);
        index = AddFieldRow(GROUP_DETAILS, index, LocString(L"IDS_F_CITY"), profile.location);
        index = AddFieldRow(GROUP_DETAILS, index, LocString(L"IDS_F_BIRTH"), profile.birthday);
        index = AddFieldRow(GROUP_DETAILS, index, LocString(L"IDS_F_ZODIAC"), profile.zodiac);
        index = AddFieldRow(GROUP_DETAILS, index, LocString(L"IDS_F_PHONE"), profile.phone);

        if (profile.sex == L"1") index = AddFieldRow(GROUP_DETAILS, index, LocString(L"IDS_F_SEX"), LocString(L"IDS_SEX_MALE"));
        else if (profile.sex == L"2") index = AddFieldRow(GROUP_DETAILS, index, LocString(L"IDS_F_SEX"), LocString(L"IDS_SEX_FEMALE"));

        if (index == 0) {
            index = AddFieldRow(GROUP_DETAILS, index, LocString(L"IDS_F_STATE"), LocString(L"IDS_PROFILE_NODATA"));
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

        case ID_OPTIONKEY_SAVE: {
            OptionMenu* pMenu = new OptionMenu();
            pMenu->Construct();
            pMenu->AddItem(LocString(L"IDS_MENU_SAVECONTACT"), ID_MENU_SAVE_CONTACT);
            pMenu->AddActionEventListener(*this);
            pMenu->SetShowState(true);
            pMenu->Show();
            break;
        }

        case ID_MENU_SAVE_CONTACT:
            SaveToAddressbook();
            break;

        default:
            break;
    }
}

void ProfileForm::SaveToAddressbook(void) {
    String email = GetDisplayEmail();
    if (email.IsEmpty()) return;

    String firstName = profile.firstName;
    String lastName = profile.lastName;
    if (firstName.IsEmpty() && lastName.IsEmpty()) {
        firstName = profile.nickname.IsEmpty() ? GetDisplayName() : profile.nickname;
    }

    String avatarPath = L"";
    if (pAvatarBitmap != null) {
        avatarPath = ContactSync::AvatarFilePath(email);
        Osp::Media::Image img;
        if (IsFailed(img.Construct())
            || IsFailed(img.EncodeToFile(*pAvatarBitmap, Osp::Media::IMG_FORMAT_JPG, avatarPath, true))) {
            avatarPath = L"";
        }
    }

    bool ok = ContactSync::SaveContact(firstName, lastName, email, profile.phone,
                                       profile.birthday, avatarPath);

    MessageBox msgBox;
    if (ok) {
        msgBox.Construct(LocString(L"IDS_SAVED_TITLE"), LocString(L"IDS_SAVEDCONTACT_OK"), MSGBOX_STYLE_OK);
    } else {
        msgBox.Construct(LocString(L"IDS_ERR_TITLE"), LocString(L"IDS_SAVEDCONTACT_FAIL"), MSGBOX_STYLE_OK);
    }
    int modalResult = 0;
    msgBox.ShowAndWait(modalResult);
}
