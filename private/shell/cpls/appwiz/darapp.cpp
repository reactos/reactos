//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 
//
// File: instapp.cpp
//
// Installed applications 
//
// History:
//         1-18-97  by dli
//------------------------------------------------------------------------
#include "priv.h"

// Do not build this file if on Win9X or NT4
#ifndef DOWNLEVEL_PLATFORM

#include "darapp.h"
#include "util.h"   
#include "appwizid.h"

// constructor
CDarwinPublishedApp::CDarwinPublishedApp(MANAGEDAPPLICATION * pma) : _cRef(1)
{
    DllAddRef();

    TraceAddRef(CDarwinPublishedApp, _cRef);
    
    hmemcpy(&_ma, pma, SIZEOF(_ma));
    _dwAction  |= APPACTION_INSTALL;
}


// destructor
CDarwinPublishedApp::~CDarwinPublishedApp()
{
    ClearManagedApplication(&_ma);
    DllRelease();
}



// IShellApps::GetAppInfo
STDMETHODIMP CDarwinPublishedApp::GetAppInfo(PAPPINFODATA pai)
{
    if (pai->cbSize != SIZEOF(APPINFODATA))
        return E_FAIL;

    DWORD dwInfoFlags = pai->dwMask;
    pai->dwMask = 0;
    
    if (dwInfoFlags & AIM_DISPLAYNAME)
    {
        if (SUCCEEDED(SHStrDupW(_ma.pszPackageName, &pai->pszDisplayName)))
            pai->dwMask |= AIM_DISPLAYNAME;
    }

    if ((dwInfoFlags & AIM_PUBLISHER) && _ma.pszPublisher && _ma.pszPublisher[0])
    {
        if (SUCCEEDED(SHStrDupW(_ma.pszPublisher, &pai->pszPublisher)))
            pai->dwMask |= AIM_PUBLISHER;
    }

    if ((dwInfoFlags & AIM_SUPPORTURL) && _ma.pszSupportUrl && _ma.pszSupportUrl[0])
    {
        if (SUCCEEDED(SHStrDupW(_ma.pszSupportUrl, &pai->pszSupportUrl)))
            pai->dwMask |= AIM_SUPPORTURL;
    }

    if ((dwInfoFlags & AIM_VERSION) && (_ma.dwVersionHi != 0))
    {
        pai->pszVersion = (LPWSTR)SHAlloc(SIZEOF(WCHAR) * MAX_PATH);
        wsprintf(pai->pszVersion, L"%d.%d.%d.%d", HIWORD(_ma.dwVersionHi), LOWORD(_ma.dwVersionHi), HIWORD(_ma.dwVersionLo), LOWORD(_ma.dwVersionLo));
        pai->dwMask |= AIM_VERSION;
    }

    
    // BUGBUG: don't know how to retrieve other infomation, need to talk to the Darwin guys about it
    TraceMsg(TF_GENERAL, "(DarPubApp) GetAppInfo with %x but got %x", dwInfoFlags, pai->dwMask);
    
    return S_OK;
}

// IShellApps::GetPossibleActions
STDMETHODIMP CDarwinPublishedApp::GetPossibleActions(DWORD * pdwActions)
{
    ASSERT(pdwActions);
    *pdwActions = _dwAction;
    return S_OK;
}
        
// IShellApps::GetSlowAppInfo
STDMETHODIMP CDarwinPublishedApp::GetSlowAppInfo(PSLOWAPPINFO psai)
{
    return E_NOTIMPL;
}

// IShellApps::GetSlowAppInfo
STDMETHODIMP CDarwinPublishedApp::GetCachedSlowAppInfo(PSLOWAPPINFO psai)
{
    return E_NOTIMPL;
}

// IShellApps::IsInstalled
STDMETHODIMP CDarwinPublishedApp::IsInstalled()
{
    return _ma.bInstalled ? S_OK : S_FALSE;
}

// IPublishedApps::Install
STDMETHODIMP CDarwinPublishedApp::Install(LPSYSTEMTIME pftInstall)
{
    INSTALLDATA id;
    id.Type = APPNAME;
    id.Spec.AppName.Name = _ma.pszPackageName;
    id.Spec.AppName.GPOId = _ma.GpoId;
    LONG lRet = InstallApplication(&id);

    HRESULT hres = HRESULT_FROM_WIN32(lRet);
    // Tell the users what is wrong with this install. 
    if (FAILED(hres))
        _ARPErrorMessageBox(lRet);
    else
        _ma.bInstalled = TRUE;
    
    return hres;
}

// IPublishedApps::GetPublishedTime
STDMETHODIMP CDarwinPublishedApp::GetPublishedAppInfo(PPUBAPPINFO ppai)
{
    if (ppai->cbSize != SIZEOF(PUBAPPINFO))
        return E_FAIL;

    DWORD dwInfoFlags = ppai->dwMask;
    ppai->dwMask = 0;
    
    if ((dwInfoFlags & PAI_SOURCE) && _ma.pszPolicyName && _ma.pszPolicyName[0])
    {
        if (SUCCEEDED(SHStrDupW(_ma.pszPolicyName, &ppai->pszSource)))
            ppai->dwMask |= PAI_SOURCE;
    }
    return S_OK;
}

// IPublishedApps::GetAssignedTime
STDMETHODIMP CDarwinPublishedApp::Unschedule(void)
{
    return E_NOTIMPL;
}

// IPublishedApp::QueryInterface
HRESULT CDarwinPublishedApp::QueryInterface(REFIID riid, LPVOID * ppvOut)
{ 
    static const QITAB qit[] = {
        QITABENT(CDarwinPublishedApp, IPublishedApp),                  // IID_IPublishedApp
        QITABENTMULTI(CDarwinPublishedApp, IShellApp, IPublishedApp),  // IID_IShellApp
        { 0 },
    };

    return QISearch(this, qit, riid, ppvOut);
}

// IPublishedApp::AddRef
ULONG CDarwinPublishedApp::AddRef()
{
    InterlockedIncrement(&_cRef);
    TraceAddRef(CDarwinPublishedApp, _cRef);
    return _cRef;
}

// IPublishedApp::Release
ULONG CDarwinPublishedApp::Release()
{
    InterlockedDecrement(&_cRef);
    TraceRelease(CDarwinPublishedApp, _cRef);
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

#endif //DOWNLEVEL_PLATFORM
