#ifndef _APP_SETTINGS_H_
#define _APP_SETTINGS_H_

#include <FBase.h>

class AppSettings {
public:
    static Osp::Base::String GetUserEmail(void);
    static Osp::Base::String GetUserPassword(void);
    static bool HasSavedCredentials(void);
    static void SaveCredentials(const Osp::Base::String& email, const Osp::Base::String& password);
    static void ClearCredentials(void);
    static void ClearPassword(void);

    static Osp::Base::String GetServerIp(void);
    static int GetServerPort(void);
    static void SaveServer(const Osp::Base::String& ip, int port);

    static Osp::Base::String GetAvatarHost(void);
    static int GetAvatarPort(void);
    static void SaveAvatarServer(const Osp::Base::String& host, int port);

    static Osp::Base::String GetDefaultServerIp(void);
    static int GetDefaultServerPort(void);

private:
    static void Put(const Osp::Base::String& key, const Osp::Base::String& value);
    static Osp::Base::String Get(const Osp::Base::String& key, const Osp::Base::String& defaultValue);
    static void Remove(const Osp::Base::String& key);
};

#endif
