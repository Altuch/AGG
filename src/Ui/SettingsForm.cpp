#include "Ui/SettingsForm.h"
#include "Core/Loc.h"
#include "Core/AggConnection.h"
#include "Core/ContactSync.h"
#include "Ui/FormNavigator.h"
#include "Core/AppSettings.h"
#include "MRIM/MrimUtils.h"
#include "MRIM/MrimContacts.h"

using namespace Osp::Base;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;

SettingsForm::SettingsForm(void) :
    pEditIp(null), pEditPort(null), pEditAvatarIp(null), pEditAvatarPort(null), pBtnSort(null), pSortMenu(null),
    syncIndex_(0), syncTotal_(0), syncOk_(true), pSyncPopup(null), pSyncLabel(null),
    pConnection(null), pReturnTo(null) {}

SettingsForm::~SettingsForm(void) {
    if (pSortMenu != null) {
        delete pSortMenu;
        pSortMenu = null;
    }
    if (pSyncPopup != null) {
        delete pSyncPopup;
        pSyncPopup = null;
    }
    pSyncLabel = null;
}

result SettingsForm::OnTerminating(void) {
    if (pSortMenu != null) {
        delete pSortMenu;
        pSortMenu = null;
    }
    if (pSyncPopup != null) {
        delete pSyncPopup;
        pSyncPopup = null;
    }
    pSyncLabel = null;
    return E_SUCCESS;
}

void SettingsForm::OnUserEventReceivedN(long requestId, Osp::Base::Collection::IList* pArgs) {
    if (requestId == USER_EVENT_SYNC) {
        ProcessSyncChunk();
    }
    if (pArgs != null) {
        pArgs->RemoveAll(true);
        delete pArgs;
    }
}

result SettingsForm::Initialize(AggConnection* pConn, Form* pReturn) {
    pConnection = pConn;
    pReturnTo = pReturn;
    return Construct(L"IDF_SETTINGS");
}

result SettingsForm::OnInitializing(void) {
    pEditIp = static_cast<EditField*>(GetControl(L"IDC_EDIT_IP"));
    pEditPort = static_cast<EditField*>(GetControl(L"IDC_EDIT_PORT"));
    pEditAvatarIp = static_cast<EditField*>(GetControl(L"IDC_AVATAR_IP"));
    pEditAvatarPort = static_cast<EditField*>(GetControl(L"IDC_AVATAR_PORT"));

    SetSoftkeyActionId(SOFTKEY_0, ID_SOFTKEY_BACK);
    AddSoftkeyActionListener(SOFTKEY_0, *this);

    SetSoftkeyActionId(SOFTKEY_1, ID_SOFTKEY_SAVE);
    AddSoftkeyActionListener(SOFTKEY_1, *this);

    Button* pBtnDefault = static_cast<Button*>(GetControl(L"IDC_BUTTON_DEFAULT"));
    if (pBtnDefault != null) {
        pBtnDefault->SetActionId(ID_BTN_DEFAULT);
        pBtnDefault->AddActionEventListener(*this);
    }
    Button* pBtnSync = static_cast<Button*>(GetControl(L"IDC_BUTTON_SYNC"));
    if (pBtnSync != null) {
        pBtnSync->SetActionId(ID_BTN_SYNC);
        pBtnSync->AddActionEventListener(*this);
    }
    pBtnSort = static_cast<Button*>(GetControl(L"IDC_BUTTON_SORT"));
    if (pBtnSort != null) {
        pBtnSort->SetActionId(ID_BTN_SORT);
        pBtnSort->AddActionEventListener(*this);
    }
    UpdateSortButtonText();
    SetTitleText(LocString(L"IDS_SETTINGS_TITLE"));
    SetSoftkeyText(SOFTKEY_0, LocString(L"IDS_SK_BACK"));
    SetSoftkeyText(SOFTKEY_1, LocString(L"IDS_SK_SAVE"));
    if (pBtnDefault != null) pBtnDefault->SetText(LocString(L"IDS_BTN_DEFAULTS"));
    if (pEditIp != null) pEditIp->SetGuideText(LocString(L"IDS_GUIDE_SERVER_IP"));
    if (pEditPort != null) pEditPort->SetGuideText(LocString(L"IDS_GUIDE_SERVER_PORT"));
    if (pEditAvatarIp != null) pEditAvatarIp->SetGuideText(LocString(L"IDS_GUIDE_AVATAR_IP"));
    if (pEditAvatarPort != null) pEditAvatarPort->SetGuideText(LocString(L"IDS_GUIDE_AVATAR_PORT"));

    if (pEditIp != null) pEditIp->SetText(AppSettings::GetServerIp());
    if (pEditPort != null) pEditPort->SetText(Integer::ToString(AppSettings::GetServerPort()));
    if (pEditAvatarIp != null) pEditAvatarIp->SetText(AppSettings::GetAvatarHost());
    if (pEditAvatarPort != null) pEditAvatarPort->SetText(Integer::ToString(AppSettings::GetAvatarPort()));

    return E_SUCCESS;
}

static bool ReadPortField(EditField* pField, int& outPort, const String& errTitle, const String& errText) {
    String portStr = (pField != null) ? pField->GetText() : String(L"");
    portStr.Trim();

    outPort = 0;
    if (IsFailed(Integer::Parse(portStr, outPort)) || outPort <= 0 || outPort > 65535) {
        MessageBox msgBox;
        msgBox.Construct(errTitle, errText, MSGBOX_STYLE_OK);
        int modalResult = 0;
        msgBox.ShowAndWait(modalResult);
        return false;
    }
    return true;
}

bool SettingsForm::ReadAndValidate(String& outIp, int& outPort,
                                   String& outAvatarIp, int& outAvatarPort) {
    outIp = (pEditIp != null) ? pEditIp->GetText() : AppSettings::GetServerIp();
    outIp.Trim();

    if (!MrimUtils::IsValidHost(outIp)) {
        MessageBox msgBox;
        msgBox.Construct(LocString(L"IDS_ERR_TITLE"), LocString(L"IDS_ERR_HOST"), MSGBOX_STYLE_OK);
        int modalResult = 0;
        msgBox.ShowAndWait(modalResult);
        return false;
    }

    if (!ReadPortField(pEditPort, outPort, LocString(L"IDS_ERR_TITLE"), LocString(L"IDS_ERR_PORT"))) return false;

    outAvatarIp = (pEditAvatarIp != null) ? pEditAvatarIp->GetText() : AppSettings::GetAvatarHost();
    outAvatarIp.Trim();
    if (!outAvatarIp.IsEmpty() && !MrimUtils::IsValidHost(outAvatarIp)) {
        MessageBox msgBox;
        msgBox.Construct(LocString(L"IDS_ERR_TITLE"), LocString(L"IDS_ERR_AVATAR_HOST"), MSGBOX_STYLE_OK);
        int modalResult = 0;
        msgBox.ShowAndWait(modalResult);
        return false;
    }

    if (pEditAvatarPort != null) {
        if (!ReadPortField(pEditAvatarPort, outAvatarPort, LocString(L"IDS_ERR_TITLE"), LocString(L"IDS_ERR_AVATAR_PORT"))) return false;
    } else {
        outAvatarPort = AppSettings::GetAvatarPort();
    }

    return true;
}

static void RedrawIfVisible(Osp::Ui::Control* pControl) {
    if (pControl != null && pControl->IsVisible()) pControl->RequestRedraw(true);
}

void SettingsForm::ApplyDefaults(void) {
    String ip = AppSettings::GetDefaultServerIp();
    int port = AppSettings::GetDefaultServerPort();

    AppSettings::SaveServer(ip, port);
    AppSettings::SaveAvatarServer(L"", 8081);

    if (pEditIp != null) {
        pEditIp->SetText(ip);
        RedrawIfVisible(pEditIp);
    }
    if (pEditPort != null) {
        pEditPort->SetText(Integer::ToString(port));
        RedrawIfVisible(pEditPort);
    }
    if (pEditAvatarIp != null) {
        pEditAvatarIp->SetText(AppSettings::GetAvatarHost());
        RedrawIfVisible(pEditAvatarIp);
    }
    if (pEditAvatarPort != null) {
        pEditAvatarPort->SetText(Integer::ToString(AppSettings::GetAvatarPort()));
        RedrawIfVisible(pEditAvatarPort);
    }

    MessageBox msgBox;
    msgBox.Construct(LocString(L"IDS_DEFAULTS_TITLE"), LocString(L"IDS_DEFAULTS_TEXT"), MSGBOX_STYLE_OK);
    int modalResult = 0;
    msgBox.ShowAndWait(modalResult);
}

String SettingsForm::GetSortModeName(int mode) {
    switch (mode) {
        case 1:  return LocString(L"IDS_SORT_AZ");
        case 2:  return LocString(L"IDS_SORT_ONLINE_FIRST");
        case 3:  return LocString(L"IDS_SORT_OFFLINE_FIRST");
        default: return LocString(L"IDS_SORT_AS_RECEIVED");
    }
}

void SettingsForm::UpdateSortButtonText(void) {
    if (pBtnSort != null) {
        pBtnSort->SetText(LocString(L"IDS_SORT_PREFIX") + GetSortModeName(AppSettings::GetContactSortMode()));
        RedrawIfVisible(pBtnSort);
    }
}

void SettingsForm::ShowSortMenu(void) {
    if (pSortMenu != null) {
        delete pSortMenu;
        pSortMenu = null;
    }

    int anchorY = GetHeight() - 160;
    if (anchorY < 0) anchorY = 0;
    Osp::Graphics::Point anchorPos(20, anchorY);

    pSortMenu = new Osp::Ui::Controls::ContextMenu();
    pSortMenu->Construct(anchorPos, Osp::Ui::Controls::CONTEXT_MENU_STYLE_LIST);
    pSortMenu->AddActionEventListener(*this);

    int current = AppSettings::GetContactSortMode();
    pSortMenu->AddItem((current == 0 ? String(L"• ") : String(L"")) + GetSortModeName(0), ID_SORT_AS_RECEIVED);
    pSortMenu->AddItem((current == 1 ? String(L"• ") : String(L"")) + GetSortModeName(1), ID_SORT_AZ);
    pSortMenu->AddItem((current == 2 ? String(L"• ") : String(L"")) + GetSortModeName(2), ID_SORT_ONLINE_FIRST);
    pSortMenu->AddItem((current == 3 ? String(L"• ") : String(L"")) + GetSortModeName(3), ID_SORT_OFFLINE_FIRST);

    pSortMenu->SetShowState(true);
    pSortMenu->Show();
}

void SettingsForm::Leave(void) {
    if (pReturnTo != null) {
        FormNavigator::Back(pReturnTo, this);
        return;
    }

    if (pConnection != null) {
        FormNavigator::GoToContactList(pConnection, this);
    } else {
        FormNavigator::GoToLogin(null, this);
    }
}

void SettingsForm::OnActionPerformed(const Control& source, int actionId) {
    switch (actionId) {
        case ID_SOFTKEY_SAVE: {
            String ip;
            int port = 0;
            String avatarIp;
            int avatarPort = 8081;
            if (!ReadAndValidate(ip, port, avatarIp, avatarPort)) break;

            AppSettings::SaveServer(ip, port);
            AppSettings::SaveAvatarServer(avatarIp, avatarPort);

            MessageBox msgBox;
            msgBox.Construct(LocString(L"IDS_SAVED_TITLE"), LocString(L"IDS_SAVED_TEXT"), MSGBOX_STYLE_OK);
            int modalResult = 0;
            msgBox.ShowAndWait(modalResult);

            Leave();
            break;
        }

        case ID_SOFTKEY_BACK: {
            Leave();
            break;
        }

        case ID_BTN_DEFAULT: {
            ApplyDefaults();
            break;
        }

        case ID_BTN_SYNC: {
            BeginRosterSync();
            break;
        }

        case ID_BTN_SORT: {
            ShowSortMenu();
            break;
        }

        case ID_SORT_AS_RECEIVED:
        case ID_SORT_AZ:
        case ID_SORT_ONLINE_FIRST:
        case ID_SORT_OFFLINE_FIRST: {
            int mode = actionId - ID_SORT_AS_RECEIVED;
            AppSettings::SaveContactSortMode(mode);
            UpdateSortButtonText();
            break;
        }

        default:
            break;
    }
}

static Osp::Base::Collection::IList* GetSyncRoster(AggConnection* pConn) {
    if (pConn == null) return null;
    Osp::Base::Collection::IList* pRoster = pConn->GetRosterContacts();
    if (pRoster == null || pRoster->GetCount() == 0) return null;
    return pRoster;
}

void SettingsForm::BeginRosterSync(void) {
    Osp::Base::Collection::IList* pRoster = GetSyncRoster(pConnection);
    if (pRoster == null) {
        MessageBox msgBox;
        msgBox.Construct(LocString(L"IDS_ERR_TITLE"), LocString(L"IDS_SYNC_FAIL"), MSGBOX_STYLE_OK);
        int modalResult = 0;
        msgBox.ShowAndWait(modalResult);
        return;
    }

    syncIndex_ = 0;
    syncTotal_ = pRoster->GetCount();
    syncOk_ = true;
    ShowSyncPopup(0, syncTotal_);
    SendUserEvent(USER_EVENT_SYNC, null);
}

void SettingsForm::ProcessSyncChunk(void) {
    Osp::Base::Collection::IList* pRoster = GetSyncRoster(pConnection);
    if (pRoster == null || syncIndex_ >= pRoster->GetCount()) {
        FinishRosterSync();
        return;
    }

    Osp::Base::Collection::ArrayList slice;
    slice.Construct();
    for (int i = 0; i < 15 && syncIndex_ < pRoster->GetCount(); i++, syncIndex_++) {
        ContactInfo* pContact = static_cast<ContactInfo*>(pRoster->GetAt(syncIndex_));
        if (pContact != null) slice.Add(*pContact);
    }
    if (!ContactSync::SyncSubset(&slice)) syncOk_ = false;
    slice.RemoveAll(false);

    if (syncIndex_ < pRoster->GetCount()) {
        ShowSyncPopup(syncIndex_, syncTotal_);
        SendUserEvent(USER_EVENT_SYNC, null);
    } else {
        FinishRosterSync();
    }
}

void SettingsForm::FinishRosterSync(void) {
    Osp::Base::Collection::IList* pRoster = GetSyncRoster(pConnection);
    if (pRoster != null && !ContactSync::FinishSync(pRoster, pConnection)) {
        syncOk_ = false;
    }
    HideSyncPopup();

    MessageBox msgBox;
    if (syncOk_) {
        msgBox.Construct(LocString(L"IDS_SAVED_TITLE"), LocString(L"IDS_SYNC_DONE"), MSGBOX_STYLE_OK);
    } else {
        msgBox.Construct(LocString(L"IDS_ERR_TITLE"), LocString(L"IDS_SYNC_FAIL"), MSGBOX_STYLE_OK);
    }
    int modalResult = 0;
    msgBox.ShowAndWait(modalResult);
}

void SettingsForm::ShowSyncPopup(int done, int total) {
    if (pSyncPopup == null) {
        int w = GetWidth() - 60;
        if (w < 200) w = 200;
        if (w > 420) w = 420;
        pSyncPopup = new Popup();
        pSyncPopup->Construct(true, Osp::Graphics::Dimension(w, 150));
        pSyncPopup->SetTitleText(LocString(L"IDS_SYNC_PROGRESS"));
        pSyncLabel = new Label();
        pSyncLabel->Construct(Osp::Graphics::Rectangle(20, 65, w - 40, 45), L"");
        pSyncPopup->AddControl(*pSyncLabel);
        pSyncPopup->SetShowState(true);
        pSyncPopup->Show();
    }
    if (pSyncLabel != null) {
        String text = Integer::ToString(done) + L"/" + Integer::ToString(total);
        pSyncLabel->SetText(text);
        RedrawIfVisible(pSyncLabel);
    }
}

void SettingsForm::HideSyncPopup(void) {
    if (pSyncPopup != null) {
        pSyncPopup->SetShowState(false);
        delete pSyncPopup;
        pSyncPopup = null;
        pSyncLabel = null;
    }
}
