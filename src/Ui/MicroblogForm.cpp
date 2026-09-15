#include "Ui/MicroblogForm.h"
#include "Core/Loc.h"
#include "Ui/FormNavigator.h"
#include "Core/AppSettings.h"
#include "MRIM/MrimProtocol.h"
#include <time.h>

using namespace Osp::Base;
using namespace Osp::Base::Collection;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;

MicroblogForm::MicroblogForm(void) :
    pConnection(null), pList(null), pRowFormats(null), rowSeq_(0),
    pAvatarLoader(null), pAvatarCache(null), pAvatarQueue(null),
    avatarLoading_(false), avatarSize_(68), pMeasureFont_(null) {}

MicroblogForm::~MicroblogForm(void) {
    if (pConnection != null) {
        pConnection->SetMicroblogListener(null);
        pConnection->SetBlogViewOpen(false);
    }
    delete pMeasureFont_;
    pMeasureFont_ = null;
    delete pAvatarLoader;
    pAvatarLoader = null;
    if (pAvatarCache != null) {
        pAvatarCache->RemoveAll(true);
        delete pAvatarCache;
    }
    if (pAvatarQueue != null) {
        pAvatarQueue->RemoveAll(true);
        delete pAvatarQueue;
    }
    if (pRowFormats != null) {
        pRowFormats->RemoveAll(true);
        delete pRowFormats;
    }
}

result MicroblogForm::Initialize(AggConnection* pConn) {
    pConnection = pConn;
    return Construct(L"IDF_MICROBLOG");
}

result MicroblogForm::OnInitializing(void) {
    SetTitleText(LocString(L"IDS_BLOG_TITLE"));

    SetSoftkeyActionId(SOFTKEY_0, ID_SOFTKEY_BACK);
    AddSoftkeyActionListener(SOFTKEY_0, *this);
    SetSoftkeyText(SOFTKEY_0, LocString(L"IDS_SK_BACK"));

    pRowFormats = new ArrayList();
    if (pRowFormats == null) return E_OUT_OF_MEMORY;
    pRowFormats->Construct();

    pMeasureFont_ = new Osp::Graphics::Font();
    if (pMeasureFont_ == null) return E_OUT_OF_MEMORY;
    if (IsFailed(pMeasureFont_->Construct(Osp::Graphics::FONT_STYLE_PLAIN, 28))) {
        delete pMeasureFont_;
        pMeasureFont_ = null;
    }

    Osp::Graphics::Rectangle client = GetClientAreaBounds();
    pList = new CustomList();
    if (pList == null) return E_OUT_OF_MEMORY;
    if (IsFailed(pList->Construct(Osp::Graphics::Rectangle(0, 0, client.width, client.height), CUSTOM_LIST_STYLE_NORMAL))) {
        delete pList;
        pList = null;
        return E_SYSTEM;
    }
    AddControl(*pList);
    pList->SetShowState(true);
    pList->Show();

    if (pConnection != null) {
        pConnection->SetMicroblogListener(this);
        pConnection->SetBlogViewOpen(true);
    }
    SchedulePopulate();

    return E_SUCCESS;
}

result MicroblogForm::OnTerminating(void) {
    return E_SUCCESS;
}

void MicroblogForm::SchedulePopulate(void) {
    SendUserEvent(USER_EVENT_POPULATE, null);
}

void MicroblogForm::OnUserEventReceivedN(long requestId, IList* pArgs) {
    if (requestId == USER_EVENT_POPULATE) PopulateList();

    if (pArgs != null) {
        pArgs->RemoveAll(true);
        delete pArgs;
    }
}

void MicroblogForm::OnMicroblogChanged(void) {
    SchedulePopulate();
}

static void AppendTwoDigits(String& out, int v) {
    if (v < 10) out.Append(L"0");
    out.Append(Integer::ToString(v));
}

String MicroblogForm::FormatBlogTime(unsigned long unixTime) {
    String out;
    if (unixTime == 0) return out;

    time_t t = (time_t)unixTime;
    struct tm* ptm = localtime(&t);
    if (ptm == null) return out;

    AppendTwoDigits(out, ptm->tm_mday);
    out.Append(L".");
    AppendTwoDigits(out, ptm->tm_mon + 1);
    out.Append(L".");
    out.Append(Integer::ToString(ptm->tm_year + 1900));
    out.Append(L" ");
    AppendTwoDigits(out, ptm->tm_hour);
    out.Append(L":");
    AppendTwoDigits(out, ptm->tm_min);
    return out;
}

void MicroblogForm::AddPostRow(const String& email, const String& nick,
                               const String& dateText, const String& text) {
    if (pList == null || pRowFormats == null) return;

    bool withAvatar = !email.IsEmpty();
    int av = withAvatar ? avatarSize_ : 0;

    int w = GetClientAreaBounds().width;
    if (w <= 0) w = 480;
    int titleX = withAvatar ? (12 + av + 8) : 12;
    int titleW = w - titleX - 12;
    if (titleW < 60) titleW = 60;
    int bodyW = w - 24;
    if (bodyW < 60) bodyW = 60;

    int bodyY = withAvatar ? (8 + av + 8) : 76;
    int bodyChars = CalibratedCharsPerLine(bodyW);
    if (bodyChars < 10) bodyChars = 10;

    int newlines = 0;
    for (int i = 0; i < text.GetLength(); i++) {
        mchar c = 0;
        text.GetCharAt(i, c);
        if (c == L'\n') newlines++;
    }

    int lines = (text.GetLength() + bodyChars - 1) / bodyChars + newlines;
    if (text.GetLength() > bodyChars) lines += 1;
    if (lines < 1) lines = 1;
    if (lines > 30) lines = 30;
    int h = bodyY + lines * 36 + 8;
    int minH = withAvatar ? (8 + av + 8) : 76;
    if (h < minH) h = minH;

    CustomListItemFormat* pFmt = new CustomListItemFormat();
    pFmt->Construct();
    if (withAvatar) {
        pFmt->AddElement(ELEM_AVATAR, Osp::Graphics::Rectangle(12, 8, av, av));
    }
    pFmt->AddElement(ELEM_TITLE, Osp::Graphics::Rectangle(titleX, 4, titleW, 30),
                     28, Osp::Graphics::Color(60, 165, 240), Osp::Graphics::Color(60, 165, 240));
    pFmt->AddElement(ELEM_DATE, Osp::Graphics::Rectangle(titleX, 36, titleW, 24),
                     22, Osp::Graphics::Color(150, 150, 150), Osp::Graphics::Color(150, 150, 150));
    pFmt->AddElement(ELEM_BODY, Osp::Graphics::Rectangle(12, bodyY, bodyW, h - bodyY - 8),
                     28, Osp::Graphics::Color(255, 255, 255), Osp::Graphics::Color(255, 255, 255));
    pRowFormats->Add(*pFmt);

    const Osp::Graphics::Bitmap* pAvatar = withAvatar ? FindAvatar(email) : null;
    CustomListItem* pItem = new CustomListItem();
    pItem->Construct(h);
    pItem->SetItemFormat(*pFmt);
    if (pAvatar != null) pItem->SetElement(ELEM_AVATAR, *pAvatar, null);
    pItem->SetElement(ELEM_TITLE, nick);
    pItem->SetElement(ELEM_DATE, dateText);
    pItem->SetElement(ELEM_BODY, text);
    pList->AddItem(*pItem, rowSeq_++);
}

void MicroblogForm::PopulateList(void) {
    if (pList == null) return;

    pList->RemoveAllItems();
    if (pRowFormats != null) pRowFormats->RemoveAll(true);

    int w = GetClientAreaBounds().width;
    avatarSize_ = (w > 0 && w < 400) ? 45 : 68;

    IList* pContacts = (pConnection != null) ? pConnection->GetRosterContacts() : null;

    ArrayList order;
    order.Construct();
    if (pContacts != null) {
        for (int i = 0; i < pContacts->GetCount(); i++) {
            ContactInfo* p = static_cast<ContactInfo*>(pContacts->GetAt(i));
            if (p == null || p->blogText.IsEmpty()) continue;
            order.Add(*(new Integer(i)));
        }
    }

    ArrayList sorted;
    sorted.Construct();
    while (order.GetCount() > 0) {
        int best = 0;
        for (int j = 1; j < order.GetCount(); j++) {
            Integer* pBestIdx = static_cast<Integer*>(order.GetAt(best));
            Integer* pCandIdx = static_cast<Integer*>(order.GetAt(j));
            int bestIdx = (pBestIdx != null) ? pBestIdx->ToInt() : -1;
            int candIdx = (pCandIdx != null) ? pCandIdx->ToInt() : -1;
            ContactInfo* pBest = (pContacts != null && bestIdx >= 0 && bestIdx < pContacts->GetCount())
                ? static_cast<ContactInfo*>(pContacts->GetAt(bestIdx)) : null;
            ContactInfo* pCand = (pContacts != null && candIdx >= 0 && candIdx < pContacts->GetCount())
                ? static_cast<ContactInfo*>(pContacts->GetAt(candIdx)) : null;
            unsigned long bestTime = (pBest != null) ? pBest->blogTime : 0;
            unsigned long candTime = (pCand != null) ? pCand->blogTime : 0;
            if (candTime > bestTime) best = j;
        }
        Integer* pWinner = static_cast<Integer*>(order.GetAt(best));
        int winnerIdx = (pWinner != null) ? pWinner->ToInt() : -1;
        order.RemoveAt(best, true);
        sorted.Add(*(new Integer(winnerIdx)));
    }
    order.RemoveAll(true);

    if (sorted.GetCount() == 0) {
        AddPostRow(L"", L"", L"", LocString(L"IDS_BLOG_EMPTY"));
    } else {
        for (int k = 0; k < sorted.GetCount(); k++) {
            Integer* pIdx = static_cast<Integer*>(sorted.GetAt(k));
            int idx = (pIdx != null) ? pIdx->ToInt() : -1;
            if (pContacts == null || idx < 0 || idx >= pContacts->GetCount()) continue;
            ContactInfo* p = static_cast<ContactInfo*>(pContacts->GetAt(idx));
            if (p == null || p->blogText.IsEmpty()) continue;

            String name = p->nickname.IsEmpty() ? p->email : p->nickname;
            AddPostRow(p->email, name, FormatBlogTime(p->blogTime), p->blogText);
            EnqueueAvatar(p->email);
        }
    }
    sorted.RemoveAll(true);

    if (GetParent() != null) {
        pList->RequestRedraw(true);
        pList->Draw();
        pList->Show();
    }
}

const Osp::Graphics::Bitmap* MicroblogForm::FindAvatar(const String& email) const {
    if (pAvatarCache == null || email.IsEmpty()) return null;

    String clean = email;
    clean.Trim();
    clean.ToLower();
    for (int i = 0; i < pAvatarCache->GetCount(); i++) {
        AvatarEntry* pEntry = static_cast<AvatarEntry*>(pAvatarCache->GetAt(i));
        if (pEntry != null && pEntry->email.Equals(clean, true)) return pEntry->pBitmap;
    }
    return null;
}

void MicroblogForm::EnqueueAvatar(const String& email) {
    String clean = email;
    clean.Trim();
    clean.ToLower();
    if (clean.IsEmpty() || FindAvatar(clean) != null) return;

    if (pAvatarQueue == null) {
        pAvatarQueue = new ArrayList();
        if (pAvatarQueue == null) return;
        pAvatarQueue->Construct();
    }
    for (int i = 0; i < pAvatarQueue->GetCount(); i++) {
        String* pQueued = static_cast<String*>(pAvatarQueue->GetAt(i));
        if (pQueued != null && pQueued->Equals(clean, true)) return;
    }
    pAvatarQueue->Add(*(new String(clean)));
    PumpAvatarQueue();
}

void MicroblogForm::PumpAvatarQueue(void) {
    if (avatarLoading_) return;
    if (pAvatarQueue == null || pAvatarQueue->GetCount() == 0) return;
    if (pConnection == null) return;

    if (pAvatarLoader == null) {
        pAvatarLoader = new AvatarLoader();
        if (pAvatarLoader == null) return;
        pAvatarLoader->SetListener(this);
    }

    String* pEmail = static_cast<String*>(pAvatarQueue->GetAt(0));
    currentAvatarEmail_ = (pEmail != null) ? *pEmail : String(L"");
    pAvatarQueue->RemoveAt(0, true);
    if (currentAvatarEmail_.IsEmpty()) {
        PumpAvatarQueue();
        return;
    }

    avatarLoading_ = true;
    int destSize = (avatarSize_ > 0) ? avatarSize_ : 68;
    String avatarType = (destSize > 45) ? String(AvatarLoader::TYPE_AVATAR)
                                        : String(AvatarLoader::TYPE_AVATAR_SMALL);
    result r = pAvatarLoader->RequestAvatar(AppSettings::GetAvatarHost(),
                                            AppSettings::GetAvatarPort(),
                                            currentAvatarEmail_,
                                            avatarType,
                                            destSize);
    if (IsFailed(r)) {
        avatarLoading_ = false;
        PumpAvatarQueue();
    }
}

void MicroblogForm::OnAvatarLoaded(const String& email, Osp::Graphics::Bitmap* pBitmap) {
    avatarLoading_ = false;
    if (pBitmap != null && !email.IsEmpty()) {
        if (pAvatarCache == null) {
            pAvatarCache = new ArrayList();
            if (pAvatarCache != null) pAvatarCache->Construct();
        }
        if (pAvatarCache != null) {
            AvatarEntry* pEntry = new AvatarEntry();
            pEntry->email = email;
            pEntry->pBitmap = pBitmap;
            pBitmap = null;
            pAvatarCache->Add(*pEntry);
            SchedulePopulate();
        }
    }
    delete pBitmap;
    PumpAvatarQueue();
}

void MicroblogForm::OnAvatarFailed(const String& email) {
    avatarLoading_ = false;
    PumpAvatarQueue();
}

int MicroblogForm::CalibratedCharsPerLine(int textW) const {
    if (pMeasureFont_ != null && textW > 0) {
        static const wchar_t* pSample =
            L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 ";
        String sample(pSample);
        Osp::Graphics::Dimension dim;
        if (!IsFailed(pMeasureFont_->GetTextExtent(sample, sample.GetLength(), dim))
            && dim.width > 0) {
            int avg = dim.width / sample.GetLength();
            if (avg > 0) {
                int cpl = textW / avg;
                return (cpl < 10) ? 10 : cpl;
            }
        }
    }
    int fallback = textW / 16;
    return (fallback < 10) ? 10 : fallback;
}

void MicroblogForm::OnActionPerformed(const Control& source, int actionId) {
    switch (actionId) {
        case ID_SOFTKEY_BACK: {
            AggConnection* pConn = pConnection;
            if (pConn != null) {
                pConn->SetMicroblogListener(null);
                pConn->SetBlogViewOpen(false);
            }

            pConnection = null;

            FormNavigator::GoToContactList(pConn, this);
            break;
        }

        default:
            break;
    }
}
