#ifndef _MICROBLOG_FORM_H_
#define _MICROBLOG_FORM_H_

#include <FUi.h>
#include <FBase.h>
#include "Core/AggConnection.h"
#include "Core/AvatarLoader.h"

class MicroblogForm : public Osp::Ui::Controls::Form,
                      public Osp::Ui::IActionEventListener,
                      public IMicroblogListener,
                      public IAvatarListener
{
public:
    MicroblogForm(void);
    virtual ~MicroblogForm(void);

    result Initialize(AggConnection* pConn);

    virtual result OnInitializing(void);
    virtual result OnTerminating(void);
    virtual void OnActionPerformed(const Osp::Ui::Control& source, int actionId);
    virtual void OnUserEventReceivedN(long requestId, Osp::Base::Collection::IList* pArgs);

    virtual void OnMicroblogChanged(void);

    virtual void OnAvatarLoaded(const Osp::Base::String& email, Osp::Graphics::Bitmap* pBitmap);
    virtual void OnAvatarFailed(const Osp::Base::String& email);

private:
    class AvatarEntry : public Osp::Base::Object {
    public:
        AvatarEntry(void) : pBitmap(null) {}
        virtual ~AvatarEntry(void) { delete pBitmap; }

        Osp::Base::String email;
        Osp::Graphics::Bitmap* pBitmap;
    };

    void SchedulePopulate(void);
    void PopulateList(void);
    void AddPostRow(const Osp::Base::String& email,
                    const Osp::Base::String& nick,
                    const Osp::Base::String& dateText,
                    const Osp::Base::String& text);

    const Osp::Graphics::Bitmap* FindAvatar(const Osp::Base::String& email) const;
    void EnqueueAvatar(const Osp::Base::String& email);
    void PumpAvatarQueue(void);
    int CalibratedCharsPerLine(int textW) const;

    static Osp::Base::String FormatBlogTime(unsigned long unixTime);

    static const int ID_SOFTKEY_BACK = 601;
    static const long USER_EVENT_POPULATE = 6001;

    static const int ELEM_AVATAR = 1;
    static const int ELEM_TITLE  = 2;
    static const int ELEM_DATE   = 3;
    static const int ELEM_BODY   = 4;

    AggConnection* pConnection;
    Osp::Ui::Controls::CustomList* pList;
    Osp::Base::Collection::ArrayList* pRowFormats;
    int rowSeq_;

    AvatarLoader* pAvatarLoader;
    Osp::Base::Collection::ArrayList* pAvatarCache;
    Osp::Base::Collection::ArrayList* pAvatarQueue;
    Osp::Base::String currentAvatarEmail_;
    bool avatarLoading_;
    int avatarSize_;
    Osp::Graphics::Font* pMeasureFont_;
};

#endif
