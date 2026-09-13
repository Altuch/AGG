#include "Core/AvatarLoader.h"
#include "MRIM/MrimUtils.h"
#include <FMedia.h>

using namespace Osp::Base;
using namespace Osp::Base::Collection;
using namespace Osp::Graphics;
using namespace Osp::Media;
using namespace Osp::Net::Http;

const wchar_t* AvatarLoader::TYPE_AVATAR       = L"_mrimavatar";
const wchar_t* AvatarLoader::TYPE_AVATAR_SMALL = L"_mrimavatarsmall";

AvatarLoader::AvatarLoader(void) :
    pListener_(null),
    pSession_(null),
    pTransaction_(null),
    pBody_(null),
    targetSize_(90),
    bodyTooLarge_(false),
    completed_(false) {
}

AvatarLoader::~AvatarLoader(void) {
    Cancel();
}

void AvatarLoader::Cancel(void) {
    completed_ = true;
    delete pTransaction_;
    pTransaction_ = null;
    delete pSession_;
    pSession_ = null;
    delete pBody_;
    pBody_ = null;
}

String AvatarLoader::BuildAvatarPath(const String& email, const String& avatarType) {
    String clean = MrimUtils::NormalizeEmail(email);

    int atPos = -1;
    if (!clean.IsEmpty()) clean.IndexOf(L"@", 0, atPos);
    if (atPos <= 0) return String(L"");

    String login;
    String domain;
    clean.SubString(0, atPos, login);
    clean.SubString(atPos + 1, domain);
    if (login.IsEmpty() || domain.IsEmpty()) return String(L"");

    String shortDomain = domain;    int dotPos = -1;
    domain.IndexOf(L".", 0, dotPos);
    if (dotPos > 0) domain.SubString(0, dotPos, shortDomain);

    String type = avatarType.IsEmpty() ? String(TYPE_AVATAR) : avatarType;
    return String(L"/") + shortDomain + String(L"/") + login + String(L"/") + type;
}

result AvatarLoader::RequestAvatar(const String& avatarHost, int avatarPort,
                                   const String& email,
                                   const String& avatarType,
                                   int destSize) {
    Cancel();

    targetEmail_ = MrimUtils::NormalizeEmail(email);
    if (targetEmail_.IsEmpty()) return E_INVALID_ARG;

    targetSize_ = (destSize > 0 && destSize <= 180) ? destSize : 90;

    String host = avatarHost;
    host.Trim();
    if (host.IsEmpty() || avatarPort <= 0 || avatarPort > 65535) return E_INVALID_ARG;

    String path = BuildAvatarPath(targetEmail_, avatarType);
    if (path.IsEmpty()) return E_INVALID_ARG;

    String baseUrl;
    baseUrl.Append(L"http://");
    baseUrl.Append(host);
    baseUrl.Append(L":");
    baseUrl.Append(Integer::ToString(avatarPort));
    String fullUrl = baseUrl + path;

    bodyTooLarge_ = false;
    completed_ = false;

    pBody_ = new ByteBuffer();
    result r = pBody_->Construct(MAX_AVATAR_BYTES);
    if (IsFailed(r)) {
        delete pBody_;
        pBody_ = null;
        return r;
    }
    pBody_->Clear();

    pSession_ = new HttpSession();
    r = pSession_->Construct(NET_HTTP_SESSION_MODE_NORMAL, null, baseUrl, null);
    if (IsFailed(r)) {
        Cancel();
        return r;
    }

    pTransaction_ = pSession_->OpenTransactionN();
    if (pTransaction_ == null) {
        Cancel();
        return E_SYSTEM;
    }
    pTransaction_->AddHttpTransactionListener(*this);

    HttpRequest* pRequest = pTransaction_->GetRequest();
    if (pRequest == null) {
        Cancel();
        return E_SYSTEM;
    }
    pRequest->SetMethod(NET_HTTP_METHOD_GET);
    pRequest->SetUri(fullUrl);

    return pTransaction_->Submit();
}

void AvatarLoader::OnTransactionReadyToRead(HttpSession& httpSession, HttpTransaction& httpTransaction,
                                            int availableBodyLen) {
    if (completed_ || bodyTooLarge_ || pBody_ == null) return;

    HttpResponse* pResponse = httpTransaction.GetResponse();
    if (pResponse == null) return;

    ByteBuffer* pChunk = pResponse->ReadBodyN();
    if (pChunk != null) {
        int chunkLen = pChunk->GetLimit();
        int freeSpace = MAX_AVATAR_BYTES - pBody_->GetPosition();
        if (chunkLen > 0 && chunkLen <= freeSpace) {
            pBody_->SetArray(pChunk->GetPointer(), 0, chunkLen);
        } else if (chunkLen > freeSpace) {
            bodyTooLarge_ = true;
        }
        delete pChunk;
    }
}

void AvatarLoader::OnTransactionReadyToWrite(HttpSession& httpSession, HttpTransaction& httpTransaction,
                                              int recommendedChunkSize) {
}

void AvatarLoader::OnTransactionHeaderCompleted(HttpSession& httpSession, HttpTransaction& httpTransaction,
                                                 int headerLen, bool bAuthRequired) {
}

void AvatarLoader::OnTransactionAborted(HttpSession& httpSession, HttpTransaction& httpTransaction,
                                        result r) {
    FinishWithFailure();
}

void AvatarLoader::OnTransactionCompleted(HttpSession& httpSession, HttpTransaction& httpTransaction) {
    if (completed_) return;
    completed_ = true;

    Bitmap* pBitmap = null;
    if (!bodyTooLarge_ && pBody_ != null && pBody_->GetPosition() > 0) {
        pBody_->Flip();
        pBitmap = DecodeAvatar(*pBody_, targetSize_);
    }

    IAvatarListener* pListener = pListener_;
    String email = targetEmail_;
    pListener_ = null;

    if (pBitmap != null && pListener != null) {
        pListener->OnAvatarLoaded(email, pBitmap);
    } else {
        delete pBitmap;
        if (pListener != null) pListener->OnAvatarFailed(email);
    }
}

void AvatarLoader::OnTransactionCertVerificationRequiredN(HttpSession& httpSession,
                                                           HttpTransaction& httpTransaction,
                                                           String* pCert) {
    if (pCert != null) delete pCert;
}

void AvatarLoader::FinishWithFailure(void) {
    if (completed_) return;
    completed_ = true;

    IAvatarListener* pListener = pListener_;
    String email = targetEmail_;
    pListener_ = null;

    if (pListener != null) pListener->OnAvatarFailed(email);
}

Bitmap* AvatarLoader::DecodeAvatar(const ByteBuffer& jpeg, int destSize) {
    int size = (destSize > 0 && destSize <= 180) ? destSize : 90;
    Image img;
    if (IsFailed(img.Construct())) return null;

    return img.DecodeN(jpeg, IMG_FORMAT_JPG, BITMAP_PIXEL_FORMAT_ARGB8888, size, size);
}
