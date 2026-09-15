#ifndef _LOC_H_
#define _LOC_H_

#include <FBase.h>
#include <FApp.h>

inline Osp::Base::String LocString(const wchar_t* pId) {
    Osp::Base::String value;
    Osp::App::Application* pApp = Osp::App::Application::GetInstance();
    if (pApp != null) {
        Osp::App::AppResource* pRes = pApp->GetAppResource();
        if (pRes != null) pRes->GetString(pId, value);
    }
    return value;
}

#endif
