#include "Core/ContactSync.h"
#include "Core/AggConnection.h"
#include "Core/AppSettings.h"
#include "Core/AvatarLoader.h"
#include "MRIM/MrimContacts.h"
#include <FBaseUtilStringTokenizer.h>
#include <FMedia.h>
#include <FSocial.h>

using namespace Osp::Base;
using namespace Osp::Base::Collection;
using namespace Osp::Base::Utility;
using namespace Osp::Graphics;
using namespace Osp::Media;
using namespace Osp::Social;

static const wchar_t* SYNC_CATEGORY_NAME = L"MRIM";
static const wchar_t* SYNC_IM_SERVICE = L"MRIM";

static void FreeList(IList* pList) {
    if (pList == null) return;
    pList->RemoveAll(true);
    delete pList;
}

static bool HasEmail(Contact* pContact, const String& email) {
    if (pContact == null || email.IsEmpty()) return false;
    IList* pEmails = pContact->GetValuesN(CONTACT_MPROPERTY_ID_EMAILS);
    if (pEmails == null) return false;
    bool found = false;
    for (int i = 0; i < pEmails->GetCount(); i++) {
        Email* pEntry = static_cast<Email*>(pEmails->GetAt(i));
        if (pEntry != null && pEntry->GetEmail().Equals(email, true)) {
            found = true;
            break;
        }
    }
    pEmails->RemoveAll(false);
    delete pEmails;
    return found;
}

static bool IsRosterEmail(IList* pRoster, const String& email) {
    if (pRoster == null || email.IsEmpty()) return false;
    for (int i = 0; i < pRoster->GetCount(); i++) {
        ContactInfo* pContact = static_cast<ContactInfo*>(pRoster->GetAt(i));
        if (pContact != null && pContact->email.Equals(email, true)) return true;
    }
    return false;
}

static bool IsMemberId(ArrayList& memberIds, RecordId recordId) {
    for (int i = 0; i < memberIds.GetCount(); i++) {
        LongLong* pId = static_cast<LongLong*>(memberIds.GetAt(i));
        if (pId != null && pId->ToLongLong() == recordId) return true;
    }
    return false;
}

static Category* FindCategory(IList* pCatList, const String& name) {
    if (pCatList == null) return null;
    for (int i = 0; i < pCatList->GetCount(); i++) {
        Category* pCand = static_cast<Category*>(pCatList->GetAt(i));
        if (pCand != null && pCand->GetName().Equals(name, true)) return pCand;
    }
    return null;
}

static String RosterTitle(const ContactInfo* pContact, const String& email) {
    if (pContact != null && !pContact->nickname.IsEmpty()) return pContact->nickname;
    return email;
}

class DetailProfileListener : public IProfileListener {
public:
    virtual void OnProfileReceived(const ProfileInfo& info) {
        String key = info.username;
        key.Trim();
        key.ToLower();
        String dom = info.domain;
        dom.Trim();
        dom.ToLower();
        if (!dom.IsEmpty()) {
            key.Append(L"@");
            key.Append(dom);
        }
        ContactSync::OnDetailProfile(key, info, true);
    }
    virtual void OnProfileNotFound(void) {
        ContactSync::OnDetailProfile(L"", ProfileInfo(), false);
    }
};

class DetailAvatarListener : public IAvatarListener {
public:
    virtual void OnAvatarLoaded(const String& email, Bitmap* pBitmap) {
        ContactSync::OnDetailAvatar(email, pBitmap);
    }
    virtual void OnAvatarFailed(const String& email) {
        ContactSync::OnDetailAvatar(email, null);
    }
};

static AggConnection* s_conn = null;
static ArrayList* s_jobs = null;
static String s_currentEmail;
static AvatarLoader* s_avatarLoader = null;
static DetailProfileListener s_profileListener;
static DetailAvatarListener s_avatarListener;

static void PumpDetailQueue(void) {
    if (s_avatarLoader != null) return;
    if (s_conn == null || s_jobs == null) return;
    if (s_jobs->GetCount() == 0) {
        s_currentEmail = L"";
        return;
    }
    String* pHead = static_cast<String*>(s_jobs->GetAt(0));
    String email = (pHead != null) ? *pHead : String(L"");
    s_jobs->RemoveAt(0, true);
    if (email.IsEmpty()) {
        PumpDetailQueue();
        return;
    }
    s_currentEmail = email;
    s_conn->RequestProfileFor(email, &s_profileListener);
}

static void DetailAdvance(void) {
    delete s_avatarLoader;
    s_avatarLoader = null;
    s_currentEmail = L"";
    PumpDetailQueue();
}

static void StartDetailPass(AggConnection* pConn, IList* pRoster) {
    if (s_jobs != null) {
        s_jobs->RemoveAll(true);
        delete s_jobs;
        s_jobs = null;
    }
    delete s_avatarLoader;
    s_avatarLoader = null;
    s_conn = pConn;
    s_currentEmail = L"";
    if (pConn == null || pRoster == null) return;
    s_jobs = new ArrayList();
    s_jobs->Construct();
    for (int i = 0; i < pRoster->GetCount(); i++) {
        ContactInfo* pContact = static_cast<ContactInfo*>(pRoster->GetAt(i));
        if (pContact == null || pContact->email.IsEmpty()) continue;
        String email = pContact->email;
        email.Trim();
        email.ToLower();
        if (!email.IsEmpty()) s_jobs->Add(*(new String(email)));
    }
    PumpDetailQueue();
}

static bool ParseBirthday(const String& text, DateTime& outDate) {
    String t = text;
    t.Trim();
    if (t.IsEmpty()) return false;
    StringTokenizer toks(t, L".");
    String parts[3];
    int n = 0;
    while (toks.HasMoreTokens() && n < 3) {
        toks.GetNextToken(parts[n]);
        parts[n].Trim();
        n++;
    }
    if (n != 3) return false;
    int d = 0, m = 0, y = 0;
    if (IsFailed(Integer::Parse(parts[0], d))) return false;
    if (IsFailed(Integer::Parse(parts[1], m))) return false;
    if (IsFailed(Integer::Parse(parts[2], y))) return false;
    if (d < 1 || d > 31 || m < 1 || m > 12 || y < 1900 || y > 2100) return false;
    DateTime tmp;
    if (IsFailed(tmp.SetValue(y, m, d, 12, 0, 0))) return false;
    outDate.SetValue(tmp);
    return true;
}

String ContactSync::AvatarFilePath(const String& email) {
    String safe = email;
    safe.Trim();
    safe.ToLower();
    safe.Replace(L"@", L"_");
    safe.Replace(L".", L"_");
    return L"/Home/agg_av_" + safe + L".jpg";
}

void ContactSync::OnDetailProfile(const String& email, const ProfileInfo& info, bool found) {
    if (found && !email.Equals(s_currentEmail, true)) return;
    String current = s_currentEmail;
    if (found && !current.IsEmpty()) {
        Addressbook book;
        if (!IsFailed(book.Construct())) {
            IList* pFound = book.SearchContactsByEmailN(current);
            Contact* pExisting = null;
            if (pFound != null) {
                for (int i = 0; i < pFound->GetCount(); i++) {
                    Contact* pCand = static_cast<Contact*>(pFound->GetAt(i));
                    if (HasEmail(pCand, current)) {
                        pExisting = pCand;
                        break;
                    }
                }
            }
            bool wantAvatar = false;
            if (pExisting != null) {
                bool changed = false;
                String curFirst, curLast;
                pExisting->GetValue(CONTACT_PROPERTY_ID_FIRST_NAME, curFirst);
                pExisting->GetValue(CONTACT_PROPERTY_ID_LAST_NAME, curLast);
                if (curFirst.IsEmpty() && !info.firstName.IsEmpty()) {
                    pExisting->SetValue(CONTACT_PROPERTY_ID_FIRST_NAME, info.firstName);
                    changed = true;
                }
                if (curLast.IsEmpty() && !info.lastName.IsEmpty()) {
                    pExisting->SetValue(CONTACT_PROPERTY_ID_LAST_NAME, info.lastName);
                    changed = true;
                }
                DateTime birth;
                if (ParseBirthday(info.birthday, birth)) {
                    pExisting->SetValue(CONTACT_PROPERTY_ID_BIRTHDAY, birth);
                    changed = true;
                }
                if (!info.phone.IsEmpty()) {
                    IList* pPhones = pExisting->GetValuesN(CONTACT_MPROPERTY_ID_PHONE_NUMBERS);
                    bool hasPhones = (pPhones != null && pPhones->GetCount() > 0);
                    if (pPhones != null) {
                        pPhones->RemoveAll(false);
                        delete pPhones;
                    }
                    if (!hasPhones) {
                        PhoneNumber pn(PHONENUMBER_TYPE_MOBILE, info.phone);
                        pExisting->AddPhoneNumber(pn);
                        changed = true;
                    }
                }
                if (changed) book.UpdateContact(*pExisting);

                Bitmap* pThumb = pExisting->GetThumbnailN();
                wantAvatar = (pThumb == null);
                delete pThumb;
            }
            FreeList(pFound);
            if (wantAvatar) {
                s_avatarLoader = new AvatarLoader();
                s_avatarLoader->SetListener(&s_avatarListener);
                result r = s_avatarLoader->RequestAvatar(AppSettings::GetAvatarHost(),
                                                         AppSettings::GetAvatarPort(),
                                                         current,
                                                         String(AvatarLoader::TYPE_AVATAR), 90);
                if (IsFailed(r)) {
                    delete s_avatarLoader;
                    s_avatarLoader = null;
                    DetailAdvance();
                }
                return;
            }
        }
    }
    DetailAdvance();
}

void ContactSync::OnDetailAvatar(const String& email, Bitmap* pBitmap) {
    delete s_avatarLoader;
    s_avatarLoader = null;
    String current = s_currentEmail;
    if (pBitmap != null && !current.IsEmpty() && email.Equals(current, true)) {
        String path = AvatarFilePath(current);
        Image img;
        if (!IsFailed(img.Construct())
            && !IsFailed(img.EncodeToFile(*pBitmap, IMG_FORMAT_JPG, path, true))) {
            Addressbook book;
            if (!IsFailed(book.Construct())) {
                IList* pFound = book.SearchContactsByEmailN(current);
                Contact* pExisting = null;
                if (pFound != null) {
                    for (int i = 0; i < pFound->GetCount(); i++) {
                        Contact* pCand = static_cast<Contact*>(pFound->GetAt(i));
                        if (HasEmail(pCand, current)) {
                            pExisting = pCand;
                            break;
                        }
                    }
                }
                if (pExisting != null) {
                    pExisting->SetThumbnail(path);
                    book.UpdateContact(*pExisting);
                }
                FreeList(pFound);
            }
        }
    }
    delete pBitmap;
    DetailAdvance();
}

static Category* EnsureCategory(Addressbook& book, IList*& pCatList) {
    pCatList = book.GetAllCategoriesN();
    Category* pAggCat = FindCategory(pCatList, String(SYNC_CATEGORY_NAME));
    if (pAggCat != null) return pAggCat;

    Category localCat;
    localCat.SetName(SYNC_CATEGORY_NAME);
    if (IsFailed(book.AddCategory(localCat))) {
        FreeList(pCatList);
        pCatList = null;
        return null;
    }
    FreeList(pCatList);
    pCatList = book.GetAllCategoriesN();
    return FindCategory(pCatList, String(SYNC_CATEGORY_NAME));
}

static bool UpsertOne(Addressbook& book, Category* pAggCat, ArrayList& memberIds,
                      const String& email, const String& title, bool& membersChanged) {
    bool ok = true;
    IList* pFound = book.SearchContactsByEmailN(email);
    Contact* pExisting = null;
    if (pFound != null) {
        for (int i = 0; i < pFound->GetCount(); i++) {
            Contact* pCand = static_cast<Contact*>(pFound->GetAt(i));
            if (HasEmail(pCand, email)) {
                pExisting = pCand;
                break;
            }
        }
    }

    if (pExisting == null) {
        Contact deviceContact;
        deviceContact.SetValue(CONTACT_PROPERTY_ID_FIRST_NAME, title);
        Email addr(EMAIL_TYPE_PERSONAL, email);
        deviceContact.AddEmail(addr);
        ImAddress im(SYNC_IM_SERVICE, email);
        deviceContact.AddImAddress(im);
        if (IsFailed(book.AddContact(deviceContact))) {
            ok = false;
        } else if (!IsFailed(pAggCat->AddMember(deviceContact.GetRecordId()))) {
            membersChanged = true;
        }
        } else {
            if (IsMemberId(memberIds, pExisting->GetRecordId())) {
                String curName;
                pExisting->GetValue(CONTACT_PROPERTY_ID_FIRST_NAME, curName);
                if (!curName.Equals(title, false)) {
                    pExisting->SetValue(CONTACT_PROPERTY_ID_FIRST_NAME, title);
                    if (IsFailed(book.UpdateContact(*pExisting))) ok = false;
                }
            }
            if (!IsMemberId(memberIds, pExisting->GetRecordId())
                && !IsFailed(pAggCat->AddMember(pExisting->GetRecordId()))) {
                membersChanged = true;
            }
        }
    FreeList(pFound);
    return ok;
}

bool ContactSync::SyncSubset(IList* pSubset) {
    if (pSubset == null || pSubset->GetCount() == 0) return true;

    Addressbook book;
    if (IsFailed(book.Construct())) return false;

    IList* pCatList = null;
    Category* pAggCat = EnsureCategory(book, pCatList);
    if (pAggCat == null) return false;

    ArrayList memberIds;
    memberIds.Construct();
    int page = 1;
    while (true) {
        IList* pMembers = book.GetContactsInN(*pAggCat, page, 50);
        if (pMembers == null || pMembers->GetCount() == 0) {
            FreeList(pMembers);
            break;
        }
        for (int i = 0; i < pMembers->GetCount(); i++) {
            Contact* pDev = static_cast<Contact*>(pMembers->GetAt(i));
            if (pDev != null) memberIds.Add(*(new LongLong(pDev->GetRecordId())));
        }
        bool lastPage = (pMembers->GetCount() < 50);
        FreeList(pMembers);
        if (lastPage) break;
        page++;
    }

    bool ok = true;
    bool membersChanged = false;
    for (int c = 0; c < pSubset->GetCount(); c++) {
        ContactInfo* pContact = static_cast<ContactInfo*>(pSubset->GetAt(c));
        if (pContact == null || pContact->email.IsEmpty()) continue;

        String email = pContact->email;
        email.Trim();
        email.ToLower();
        if (email.IsEmpty()) continue;
        if (!UpsertOne(book, pAggCat, memberIds, email, RosterTitle(pContact, email), membersChanged)) {
            ok = false;
        }
    }
    memberIds.RemoveAll(true);

    if (membersChanged) {
        if (IsFailed(book.UpdateCategory(*pAggCat))) ok = false;
    }
    FreeList(pCatList);

    return ok;
}

bool ContactSync::FinishSync(IList* pFullRoster, AggConnection* pConn) {
    if (pFullRoster == null || pFullRoster->GetCount() == 0) return true;

    Addressbook book;
    if (IsFailed(book.Construct())) return false;

    IList* pCatList = null;
    Category* pAggCat = EnsureCategory(book, pCatList);
    if (pAggCat == null) return false;

    bool ok = true;

    ArrayList staleIds;
    staleIds.Construct();
    int page = 1;
    while (true) {
        IList* pMembers = book.GetContactsInN(*pAggCat, page, 50);
        if (pMembers == null || pMembers->GetCount() == 0) {
            FreeList(pMembers);
            break;
        }
        for (int i = 0; i < pMembers->GetCount(); i++) {
            Contact* pDev = static_cast<Contact*>(pMembers->GetAt(i));
            if (pDev == null) continue;
            IList* pEmails = pDev->GetValuesN(CONTACT_MPROPERTY_ID_EMAILS);
            bool inRoster = false;
            if (pEmails != null) {
                for (int e = 0; e < pEmails->GetCount(); e++) {
                    Email* pEntry = static_cast<Email*>(pEmails->GetAt(e));
                    if (pEntry != null && IsRosterEmail(pFullRoster, pEntry->GetEmail())) {
                        inRoster = true;
                        break;
                    }
                }
                pEmails->RemoveAll(false);
                delete pEmails;
            }
            if (!inRoster) staleIds.Add(*(new LongLong(pDev->GetRecordId())));
        }
        bool lastPage = (pMembers->GetCount() < 50);
        FreeList(pMembers);
        if (lastPage) break;
        page++;
    }

    for (int i = 0; i < staleIds.GetCount(); i++) {
        LongLong* pId = static_cast<LongLong*>(staleIds.GetAt(i));
        if (pId != null && IsFailed(book.RemoveContact(pId->ToLongLong()))) ok = false;
    }
    staleIds.RemoveAll(true);    FreeList(pCatList);

    StartDetailPass(pConn, pFullRoster);

    return ok;
}

bool ContactSync::SaveContact(const String& firstName, const String& lastName,
                              const String& email, const String& phone,
                              const String& birthday, const String& avatarPath) {
    if (email.IsEmpty()) return false;

    String cleanEmail = email;
    cleanEmail.Trim();
    cleanEmail.ToLower();
    if (cleanEmail.IsEmpty()) return false;

    Addressbook book;
    if (IsFailed(book.Construct())) return false;

    IList* pCatList = book.GetAllCategoriesN();
    Category* pAggCat = FindCategory(pCatList, String(SYNC_CATEGORY_NAME));

    if (pAggCat == null) {
        Category localCat;
        localCat.SetName(SYNC_CATEGORY_NAME);
        if (IsFailed(book.AddCategory(localCat))) {
            FreeList(pCatList);
            return false;
        }
        FreeList(pCatList);
        pCatList = book.GetAllCategoriesN();
        pAggCat = FindCategory(pCatList, String(SYNC_CATEGORY_NAME));
        if (pAggCat == null) {
            FreeList(pCatList);
            return false;
        }
    }

    bool ok = true;
    bool membersChanged = false;

    IList* pFound = book.SearchContactsByEmailN(cleanEmail);
    Contact* pExisting = null;
    if (pFound != null) {
        for (int i = 0; i < pFound->GetCount(); i++) {
            Contact* pCand = static_cast<Contact*>(pFound->GetAt(i));
            if (HasEmail(pCand, cleanEmail)) {
                pExisting = pCand;
                break;
            }
        }
    }

    Contact* pDev = null;
    Contact deviceContact;
    if (pExisting == null) {
        if (!firstName.IsEmpty()) deviceContact.SetValue(CONTACT_PROPERTY_ID_FIRST_NAME, firstName);
        if (!lastName.IsEmpty()) deviceContact.SetValue(CONTACT_PROPERTY_ID_LAST_NAME, lastName);
        Email addr(EMAIL_TYPE_PERSONAL, cleanEmail);
        deviceContact.AddEmail(addr);
        if (!phone.IsEmpty()) {
            PhoneNumber phoneNumber(PHONENUMBER_TYPE_MOBILE, phone);
            deviceContact.AddPhoneNumber(phoneNumber);
        }
        ImAddress im(SYNC_IM_SERVICE, cleanEmail);
        deviceContact.AddImAddress(im);
        if (IsFailed(book.AddContact(deviceContact))) {
            ok = false;
        } else {
            pDev = &deviceContact;
            if (!IsFailed(pAggCat->AddMember(deviceContact.GetRecordId()))) {
                membersChanged = true;
            }
        }
    } else {
        pDev = pExisting;
        if (!phone.IsEmpty()) {
            IList* pPhones = pExisting->GetValuesN(CONTACT_MPROPERTY_ID_PHONE_NUMBERS);
            bool hasPhones = (pPhones != null && pPhones->GetCount() > 0);
            if (pPhones != null) {
                pPhones->RemoveAll(false);
                delete pPhones;
            }
            if (!hasPhones) {
                PhoneNumber phoneNumber(PHONENUMBER_TYPE_MOBILE, phone);
                pExisting->AddPhoneNumber(phoneNumber);
                book.UpdateContact(*pExisting);
            }
        }
        if (!IsFailed(pAggCat->AddMember(pExisting->GetRecordId()))) {
            membersChanged = true;
        }
    }

    if (ok && pDev != null) {
        bool touched = false;
        DateTime birth;
        if (ParseBirthday(birthday, birth)) {
            pDev->SetValue(CONTACT_PROPERTY_ID_BIRTHDAY, birth);
            touched = true;
        }
        if (!avatarPath.IsEmpty()) {
            pDev->SetThumbnail(avatarPath);
            touched = true;
        }
        if (touched) {
            if (IsFailed(book.UpdateContact(*pDev))) ok = false;
        }
    }
    FreeList(pFound);

    if (membersChanged) {
        if (IsFailed(book.UpdateCategory(*pAggCat))) ok = false;
    }
    FreeList(pCatList);

    return ok;
}
