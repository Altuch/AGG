#include "Core/AppSettings.h"
#include <FApp.h>

using namespace Osp::App;
using namespace Osp::Base;

static const wchar_t* KEY_EMAIL       = L"UserEmail";
static const wchar_t* KEY_PASSWORD    = L"UserPassword";
static const wchar_t* KEY_SERVER_IP   = L"ServerIP";
static const wchar_t* KEY_SERVER_PORT = L"ServerPort";
static const wchar_t* KEY_AVATAR_HOST = L"AvatarHost";
static const wchar_t* KEY_AVATAR_PORT = L"AvatarPort";
static const wchar_t* KEY_SORT_MODE = L"ContactSortMode";

static const wchar_t* DEFAULT_IP   = L"proto.mrim.su";
static const int      DEFAULT_PORT = 2041;

String AppSettings::GetDefaultServerIp(void) {
    return String(DEFAULT_IP);
}

int AppSettings::GetDefaultServerPort(void) {
    return DEFAULT_PORT;
}

String AppSettings::Get(const String& key, const String& defaultValue) {
    AppRegistry* pReg = Application::GetInstance()->GetAppRegistry();
    if (pReg == null) return defaultValue;

    String value;
    if (IsFailed(pReg->Get(key, value))) return defaultValue;

    value.Trim();
    return value.IsEmpty() ? defaultValue : value;
}

void AppSettings::Put(const String& key, const String& value) {
    AppRegistry* pReg = Application::GetInstance()->GetAppRegistry();
    if (pReg == null) return;

    String existing;
    if (pReg->Get(key, existing) == E_SUCCESS) {
        pReg->Set(key, value);
    } else {
        pReg->Add(key, value);
    }
    pReg->Save();
}

void AppSettings::Remove(const String& key) {
    AppRegistry* pReg = Application::GetInstance()->GetAppRegistry();
    if (pReg == null) return;

    pReg->Remove(key);
    pReg->Save();
}

String AppSettings::GetUserEmail(void) {
    return Get(KEY_EMAIL, L"");
}

String AppSettings::GetUserPassword(void) {
    return Get(KEY_PASSWORD, L"");
}

bool AppSettings::HasSavedCredentials(void) {
    return !GetUserEmail().IsEmpty() && !GetUserPassword().IsEmpty();
}

void AppSettings::SaveCredentials(const String& email, const String& password) {
    Put(KEY_EMAIL, email);
    Put(KEY_PASSWORD, password);
}

void AppSettings::ClearCredentials(void) {
    Remove(KEY_EMAIL);
    Remove(KEY_PASSWORD);
}

void AppSettings::ClearPassword(void) {
    Remove(KEY_PASSWORD);
}

String AppSettings::GetServerIp(void) {
    return Get(KEY_SERVER_IP, DEFAULT_IP);
}

int AppSettings::GetServerPort(void) {
    String portStr = Get(KEY_SERVER_PORT, L"");
    if (portStr.IsEmpty()) return DEFAULT_PORT;

    int port = DEFAULT_PORT;
    if (IsFailed(Integer::Parse(portStr, port))) return DEFAULT_PORT;
    if (port <= 0 || port > 65535) return DEFAULT_PORT;
    return port;
}

void AppSettings::SaveServer(const String& ip, int port) {
    Put(KEY_SERVER_IP, ip);
    Put(KEY_SERVER_PORT, Integer::ToString(port));
}

String AppSettings::GetAvatarHost(void) {
    return Get(KEY_AVATAR_HOST, GetServerIp());
}

int AppSettings::GetAvatarPort(void) {
    String portStr = Get(KEY_AVATAR_PORT, L"");
    if (portStr.IsEmpty()) return 8081;

    int port = 8081;
    if (IsFailed(Integer::Parse(portStr, port))) return 8081;
    if (port <= 0 || port > 65535) return 8081;
    return port;
}

void AppSettings::SaveAvatarServer(const String& host, int port) {
    Put(KEY_AVATAR_HOST, host);
    Put(KEY_AVATAR_PORT, Integer::ToString(port));
}

int AppSettings::GetContactSortMode(void) {
    String modeStr = Get(KEY_SORT_MODE, L"");
    if (modeStr.IsEmpty()) return 0;

    int mode = 0;
    if (IsFailed(Integer::Parse(modeStr, mode))) return 0;
    if (mode < 0 || mode > 3) return 0;
    return mode;
}

void AppSettings::SaveContactSortMode(int mode) {
    if (mode < 0 || mode > 3) mode = 0;
    Put(KEY_SORT_MODE, Integer::ToString(mode));
}
