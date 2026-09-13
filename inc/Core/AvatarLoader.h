#ifndef _AVATAR_LOADER_H_
#define _AVATAR_LOADER_H_

#include <FBase.h>
#include <FNet.h>
#include <FGraphics.h>

class IAvatarListener {
public:
    virtual ~IAvatarListener(void) {}
    virtual void OnAvatarLoaded(const Osp::Base::String& email, Osp::Graphics::Bitmap* pBitmap) = 0;
    virtual void OnAvatarFailed(const Osp::Base::String& email) = 0;
};

class AvatarLoader : public Osp::Net::Http::IHttpTransactionEventListener {
public:
    static const wchar_t* TYPE_AVATAR;
    static const wchar_t* TYPE_AVATAR_SMALL;

    AvatarLoader(void);
    virtual ~AvatarLoader(void);

    void SetListener(IAvatarListener* pListener) { pListener_ = pListener; }

    result RequestAvatar(const Osp::Base::String& avatarHost, int avatarPort,
                         const Osp::Base::String& email,
                         const Osp::Base::String& avatarType,
                         int destSize = 90);
    void Cancel(void);

    static Osp::Base::String BuildAvatarPath(const Osp::Base::String& email,
                                              const Osp::Base::String& avatarType);

    virtual void OnTransactionReadyToRead(Osp::Net::Http::HttpSession& httpSession,
                                          Osp::Net::Http::HttpTransaction& httpTransaction,
                                          int availableBodyLen);
    virtual void OnTransactionReadyToWrite(Osp::Net::Http::HttpSession& httpSession,
                                           Osp::Net::Http::HttpTransaction& httpTransaction,
                                           int recommendedChunkSize);
    virtual void OnTransactionHeaderCompleted(Osp::Net::Http::HttpSession& httpSession,
                                              Osp::Net::Http::HttpTransaction& httpTransaction,
                                              int headerLen,
                                              bool bAuthRequired);
    virtual void OnTransactionAborted(Osp::Net::Http::HttpSession& httpSession,
                                      Osp::Net::Http::HttpTransaction& httpTransaction,
                                      result r);
    virtual void OnTransactionCompleted(Osp::Net::Http::HttpSession& httpSession,
                                        Osp::Net::Http::HttpTransaction& httpTransaction);
    virtual void OnTransactionCertVerificationRequiredN(Osp::Net::Http::HttpSession& httpSession,
                                                        Osp::Net::Http::HttpTransaction& httpTransaction,
                                                        Osp::Base::String* pCert);

private:
    static const int MAX_AVATAR_BYTES = 131072;

    void FinishWithFailure(void);
    Osp::Graphics::Bitmap* DecodeAvatar(const Osp::Base::ByteBuffer& jpeg, int destSize);

    IAvatarListener* pListener_;
    Osp::Net::Http::HttpSession* pSession_;
    Osp::Net::Http::HttpTransaction* pTransaction_;
    Osp::Base::ByteBuffer* pBody_;
    Osp::Base::String targetEmail_;
    int targetSize_;
    bool bodyTooLarge_;
    bool completed_;
};

#endif
