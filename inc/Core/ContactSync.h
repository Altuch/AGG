#ifndef _CONTACT_SYNC_H_
#define _CONTACT_SYNC_H_

#include <FBase.h>
#include <FBaseColIList.h>
#include <FGraphics.h>
#include "MRIM/MrimProfile.h"

class AggConnection;

class ContactSync {
public:
    static bool SyncSubset(Osp::Base::Collection::IList* pSubset);
    static bool FinishSync(Osp::Base::Collection::IList* pFullRoster, AggConnection* pConn);
    static bool SaveContact(const Osp::Base::String& firstName,
                            const Osp::Base::String& lastName,
                            const Osp::Base::String& email,
                            const Osp::Base::String& phone,
                            const Osp::Base::String& birthday,
                            const Osp::Base::String& avatarPath);
    static Osp::Base::String AvatarFilePath(const Osp::Base::String& email);

    static void OnDetailProfile(const Osp::Base::String& email, const ProfileInfo& info, bool found);
    static void OnDetailAvatar(const Osp::Base::String& email, Osp::Graphics::Bitmap* pBitmap);

private:
    ContactSync(void);
};

#endif
