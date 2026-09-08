#ifndef _PROFILE_FORM_H_
#define _PROFILE_FORM_H_

#include <FUi.h>
#include <FBase.h>
#include <FGraphics.h>
#include "Core/AggConnection.h"
#include "MRIM/MrimProfile.h"

class ProfileForm : public Osp::Ui::Controls::Form,
                     public Osp::Ui::IActionEventListener,
                     public IProfileListener
{
public:
    ProfileForm(void);
    virtual ~ProfileForm(void);

    result Initialize(AggConnection* pConn,
                      const Osp::Base::String& contactName,
                      const Osp::Base::String& contactEmail,
                      Osp::Ui::Controls::Form* pReturnTo);

    virtual result OnInitializing(void);
    virtual result OnTerminating(void);
    virtual void OnActionPerformed(const Osp::Ui::Control& source, int actionId);
    virtual void OnUserEventReceivedN(long requestId, Osp::Base::Collection::IList* pArgs);

    virtual void OnProfileReceived(const ProfileInfo& info);
    virtual void OnProfileNotFound(void);

private:
    void SchedulePopulate(void);
    void PopulateList(void);

    Osp::Ui::Controls::CustomListItem* CreateIdentityRow(void) const;
    Osp::Ui::Controls::CustomListItem* CreateFieldRow(const Osp::Base::String& label,
                                                      const Osp::Base::String& value) const;

    int AddFieldRow(int group, int index, const Osp::Base::String& label,
                    const Osp::Base::String& value);

    Osp::Base::String GetDisplayName(void) const;
    Osp::Base::String GetDisplayEmail(void) const;
    const Osp::Graphics::Bitmap* GetAvatarBitmap(void) const;
    void Leave(void);

    static const int ID_SOFTKEY_BACK = 401;
    static const long USER_EVENT_POPULATE = 4001;

    static const int GROUP_IDENTITY = 0;
    static const int GROUP_DETAILS  = 1;

    static const int ELEM_AVATAR = 1;
    static const int ELEM_TITLE  = 2;
    static const int ELEM_SUB    = 3;
    static const int ELEM_LABEL = 1;
    static const int ELEM_VALUE = 2;

    static const int IDENTITY_ROW_HEIGHT = 96;
    static const int FIELD_ROW_HEIGHT    = 56;

    AggConnection* pConnection;
    Osp::Ui::Controls::GroupedList* pList;
    Osp::Ui::Controls::CustomListItemFormat* pIdentityFormat;
    Osp::Ui::Controls::CustomListItemFormat* pFieldFormat;

    Osp::Graphics::Bitmap* pBitmapOnline;
    Osp::Graphics::Bitmap* pBitmapAway;
    Osp::Graphics::Bitmap* pBitmapBusy;
    Osp::Graphics::Bitmap* pBitmapOffline;

    Osp::Base::String contactName;
    Osp::Base::String contactEmail;
    Osp::Ui::Controls::Form* pReturnTo;

    ProfileInfo profile;
    bool hasProfile;
    bool isNotFound;
};

#endif
