#ifndef _SETTINGS_FORM_H_
#define _SETTINGS_FORM_H_

#include <FUi.h>
#include <FBase.h>

class AggConnection;

class SettingsForm : public Osp::Ui::Controls::Form,
                     public Osp::Ui::IActionEventListener
{
public:
    SettingsForm(void);
    virtual ~SettingsForm(void);

    result Initialize(AggConnection* pConnection = null,
                      Osp::Ui::Controls::Form* pReturnTo = null);

    virtual result OnInitializing(void);
    virtual result OnTerminating(void);
    virtual void OnActionPerformed(const Osp::Ui::Control& source, int actionId);
    virtual void OnUserEventReceivedN(long requestId, Osp::Base::Collection::IList* pArgs);

private:
    void Leave(void);
    void ApplyDefaults(void);
    bool ReadAndValidate(Osp::Base::String& outIp, int& outPort,
                         Osp::Base::String& outAvatarIp, int& outAvatarPort);
    void BeginRosterSync(void);
    void ProcessSyncChunk(void);
    void FinishRosterSync(void);
    void ShowSyncPopup(int done, int total);
    void HideSyncPopup(void);
    void UpdateSortButtonText(void);
    void ShowSortMenu(void);
    static Osp::Base::String GetSortModeName(int mode);

    static const int ID_SOFTKEY_BACK = 201;
    static const int ID_SOFTKEY_SAVE = 202;
    static const int ID_BTN_DEFAULT  = 203;
    static const int ID_BTN_SYNC     = 204;
    static const int ID_BTN_SORT     = 205;

    static const int ID_SORT_AS_RECEIVED   = 206;
    static const int ID_SORT_AZ            = 207;
    static const int ID_SORT_ONLINE_FIRST  = 208;
    static const int ID_SORT_OFFLINE_FIRST = 209;

    static const long USER_EVENT_SYNC = 301;

    Osp::Ui::Controls::EditField* pEditIp;
    Osp::Ui::Controls::EditField* pEditPort;
    Osp::Ui::Controls::EditField* pEditAvatarIp;
    Osp::Ui::Controls::EditField* pEditAvatarPort;
    Osp::Ui::Controls::Button* pBtnSort;
    Osp::Ui::Controls::ContextMenu* pSortMenu;

    int syncIndex_;
    int syncTotal_;
    bool syncOk_;
    Osp::Ui::Controls::Popup* pSyncPopup;
    Osp::Ui::Controls::Label* pSyncLabel;

    AggConnection* pConnection;
    Osp::Ui::Controls::Form* pReturnTo;
};

#endif
