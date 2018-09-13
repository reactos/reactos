//
// ident.cpp - implementation of CIdentity class
//
#include "private.h"
#include "multiusr.h"
#include "multiui.h"
#include "strconst.h"
#include "resource.h"
#include "mluisup.h"

extern HINSTANCE g_hInst;
BOOL        g_fReleasedMutex = true;


//
// Constructor / destructor
//
CUserIdentityManager::CUserIdentityManager()
{
    m_cRef = 1;
    m_fWndRegistered = FALSE;
    m_hwnd = NULL;
    m_pAdviseRegistry = NULL;
    InitializeCriticalSection(&m_rCritSect);
    DllAddRef();
}

CUserIdentityManager::~CUserIdentityManager()
{
    if (m_pAdviseRegistry)
        m_pAdviseRegistry->Release();
    DeleteCriticalSection(&m_rCritSect);
    DllRelease();
}


//
// IUnknown members
//
STDMETHODIMP CUserIdentityManager::QueryInterface(
    REFIID riid, void **ppv)
{
    if (NULL == ppv)
    {
        return E_INVALIDARG;
    }
    
    *ppv=NULL;

    // Validate requested interface
    if(IID_IUnknown == riid)
    {
        *ppv = (IUserIdentityManager *)this;
    }
    else if(IID_IUserIdentityManager == riid)
    {
        *ppv = (IUserIdentityManager *)this;
    }
    else if(IID_IConnectionPoint == riid)
    {
        *ppv = (IConnectionPoint *)this;
    }
    else if(IID_IPrivateIdentityManager == riid)
    {
        *ppv = (IPrivateIdentityManager *)this;
    }

    // Addref through the interface
    if( NULL != *ppv ) {
        ((LPUNKNOWN)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CUserIdentityManager::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CUserIdentityManager::Release()
{
    if( 0L != --m_cRef )
        return m_cRef;

    delete this;
    return 0L;
}


STDMETHODIMP CUserIdentityManager::CreateIdentity(WCHAR *pszName, IUserIdentity **ppIdentity)
{
    CUserIdentity *pIdentity;
    HRESULT  hr;
    TCHAR   szName[CCH_IDENTITY_NAME_MAX_LENGTH+1];

    *ppIdentity = NULL;

    if (MU_IdentitiesDisabled())
        return E_IDENTITIES_DISABLED;

    if (WideCharToMultiByte(CP_ACP, 0, pszName, -1, szName, CCH_IDENTITY_NAME_MAX_LENGTH, NULL, NULL) == 0)
        return GetLastError();

    if (MU_UsernameExists(szName))
        return E_IDENTITY_EXISTS;

    pIdentity = new CUserIdentity;
    
    Assert(pIdentity);

    if (!pIdentity)
        return E_OUTOFMEMORY;

    hr = pIdentity->SetName(pszName);
    
    if (SUCCEEDED(hr))
        *ppIdentity = pIdentity;
    else
        pIdentity->Release();

    PostMessage(HWND_BROADCAST, WM_IDENTITY_INFO_CHANGED, 0, IIC_IDENTITY_ADDED);

    return hr;
}

STDMETHODIMP CUserIdentityManager::ConfirmPassword(GUID *uidCookie, WCHAR *pszPassword)
{
    TCHAR           szPwd[CCH_USERPASSWORD_MAX_LENGTH+1];
    HRESULT         hr = E_FAIL;
    USERINFO        userInfo;

    if (WideCharToMultiByte(CP_ACP, 0, pszPassword, -1, szPwd, CCH_USERPASSWORD_MAX_LENGTH, NULL, NULL) == 0)
        return E_FAIL;

    if (MU_GetUserInfo(uidCookie, &userInfo))
    {
        if (userInfo.fPasswordValid)
        {
            if (!userInfo.fUsePassword)
                userInfo.szPassword[0] = 0;

            if(lstrcmp(szPwd, userInfo.szPassword) == 0)
                hr = S_OK;
            else
                hr = E_FAIL;
        }
        else
        {
            hr = E_FAIL;
        }
    }

    // Slow down dictionary attacks by waiting one second whenever an incorrect
    // password is provided.
    if (hr == E_FAIL)
        Sleep(1000);

    return hr;
}

STDMETHODIMP CUserIdentityManager::DestroyIdentity(GUID *uidCookie)
{
    if (MU_IdentitiesDisabled())
        return E_IDENTITIES_DISABLED;
    
    return MU_DeleteUser(uidCookie);
}

STDMETHODIMP CUserIdentityManager::EnumIdentities(IEnumUserIdentity **ppEnumIdentity)
{
    CEnumUserIdentity   *pEnumIdentity;

    *ppEnumIdentity = NULL;

    pEnumIdentity = new CEnumUserIdentity;

    if(!pEnumIdentity)
        return E_OUTOFMEMORY;

    *ppEnumIdentity = pEnumIdentity;

    return S_OK;
}

STDMETHODIMP CUserIdentityManager::ManageIdentities(HWND hwndParent, DWORD dwFlags)
{
    TCHAR    szUsername[CCH_USERNAME_MAX_LENGTH+1];

    if (MU_IdentitiesDisabled())
        return E_IDENTITIES_DISABLED;
    
    *szUsername = 0;

    MU_ManageUsers(hwndParent, szUsername, dwFlags);
    
    // if the user created a new user and said they want to switch to them now,
    // we should do so.
    if (*szUsername)
    {
        BOOL        fGotUser;
        USERINFO    rUser;
        GUID        uidUserID;
        HRESULT     hr;

        fGotUser = MU_GetUserInfo(NULL, &rUser);
        if (!fGotUser)
        {
            *rUser.szUsername = 0;
            ZeroMemory(&rUser.uidUserID, sizeof(GUID));
        }
        MU_UsernameToUserId(szUsername, &uidUserID);

        if (FAILED(hr = _SwitchToUser(&rUser.uidUserID, &uidUserID)))
        {
            SetForegroundWindow(hwndParent);
            
            if (hr != E_USER_CANCELLED)
                MU_ShowErrorMessage(hwndParent, idsSwitchCancelled, idsSwitchCancelCaption);
        }
    }
    return S_OK;
}

STDMETHODIMP CUserIdentityManager::_PersistChangingIdentities()
{
    HRESULT hr = E_FAIL;
    HKEY hKeyIdentities = NULL;

    if (ERROR_SUCCESS != RegOpenKey(HKEY_CURRENT_USER, c_szRegRoot, &hKeyIdentities))
    {
        goto exit;
    }

    if (ERROR_SUCCESS != RegSetValueEx(hKeyIdentities, c_szOutgoingID, 0, REG_BINARY, (LPBYTE)&g_uidOldUserId, sizeof(GUID)))
    {
        goto exit;
    }
    
    if (ERROR_SUCCESS != RegSetValueEx(hKeyIdentities, c_szIncomingID, 0, REG_BINARY, (LPBYTE)&g_uidNewUserId, sizeof(GUID)))
    {
        goto exit;
    }
    
    if (ERROR_SUCCESS != RegSetValueEx(hKeyIdentities, c_szChanging, 0, REG_BINARY, (LPBYTE)&g_fNotifyComplete, sizeof(g_fNotifyComplete)))
    {
        goto exit;
    }
    

    hr = S_OK;
exit:
    if (hKeyIdentities)
    {
        RegCloseKey(hKeyIdentities);
    }
    
    return hr;
}

STDMETHODIMP CUserIdentityManager::_LoadChangingIdentities()
{
    HRESULT hr = E_FAIL;
    HKEY hKeyIdentities = NULL;
    DWORD dwType, dwSize;

    if (ERROR_SUCCESS != RegOpenKey(HKEY_CURRENT_USER, c_szRegRoot, &hKeyIdentities))
    {
        goto exit;
    }

    dwType = REG_BINARY;
    dwSize = sizeof(GUID);
    if (ERROR_SUCCESS != RegQueryValueEx(hKeyIdentities, c_szOutgoingID, 0, &dwType, (LPBYTE)&g_uidOldUserId, &dwSize))
    {
        goto exit;
    }
    
    dwSize = sizeof(GUID);
    if (ERROR_SUCCESS != RegQueryValueEx(hKeyIdentities, c_szIncomingID, 0, &dwType, (LPBYTE)&g_uidNewUserId, &dwSize))
    {
        goto exit;
    }

    dwSize = sizeof(g_fNotifyComplete);
    if (ERROR_SUCCESS != RegQueryValueEx(hKeyIdentities, c_szChanging, 0, &dwType, (LPBYTE)&g_fNotifyComplete, &dwSize))
    {
        goto exit;
    }


    hr = S_OK;
exit:
    if (FAILED(hr))
    {
        g_uidOldUserId = GUID_NULL;
        g_uidNewUserId = GUID_NULL;
        g_fNotifyComplete = TRUE;
    }
    
    if (hKeyIdentities)
    {
        RegCloseKey(hKeyIdentities);
    }
    
    return hr;
}

STDMETHODIMP CUserIdentityManager::_ClearChangingIdentities()
{
    HRESULT hr = E_FAIL;
    HKEY hKeyIdentities = NULL;

    if (ERROR_SUCCESS != RegOpenKey(HKEY_CURRENT_USER, c_szRegRoot, &hKeyIdentities))
    {
        goto exit;
    }

    RegDeleteValue(hKeyIdentities, c_szChanging);
    RegDeleteValue(hKeyIdentities, c_szIncomingID);
    RegDeleteValue(hKeyIdentities, c_szOutgoingID);    

    hr = S_OK;
    
exit:
    if (hKeyIdentities)
    {
        RegCloseKey(hKeyIdentities);
    }
    
    return hr;

}

STDMETHODIMP CUserIdentityManager::Logon(HWND hwndParent, DWORD dwFlags, IUserIdentity **ppIdentity)
{
    CUserIdentity *pIdentity;
    HRESULT     hr = E_FAIL;
    USERINFO    rUser;
    GUID        uidUserID, uidNewUserID;
    BOOL        fGotUser;
    TCHAR       szOldUsername[CCH_USERNAME_MAX_LENGTH+1], szLogoffName[CCH_USERNAME_MAX_LENGTH+1];
    TCHAR       szRes[MAX_PATH];

    // if identities are disabled, always return the default identity.
    // if they are forcing the UI, return an error, otherwise succeed and 
    // send the message back that identities are disabled.
    if (MU_IdentitiesDisabled())
    {
        if ( !!(dwFlags & UIL_FORCE_UI))
            return E_IDENTITIES_DISABLED;

        hr = GetIdentityByCookie((GUID *)&UID_GIBC_DEFAULT_USER, ppIdentity);
        
        return (SUCCEEDED(hr) ? S_IDENTITIES_DISABLED : hr);
    }

    if (!g_hMutex)
        return E_UNEXPECTED;

    _LoadChangingIdentities();
    
    if (g_uidOldUserId != GUID_NULL || g_uidNewUserId != GUID_NULL)
    {
        // we are in the middle of a switch
        if (!g_fNotifyComplete)
        {
            // and we are not done checking to see if a switch is ok.
            if ( !!(dwFlags & UIL_FORCE_UI))    //if its a force ui, then just fail.
                return E_IDENTITY_CHANGING;    

            //otherwise, we need to do something here, but since they could be
            //calling Login from the notifier proc, this could create a deadlock,
            //but returning either the old or the new could be wrong.  Return the
            //same error here unless we can come up with a better solution.
            return E_IDENTITY_CHANGING;
        }
    }

    DWORD dwWaitResult;
    dwWaitResult = WaitForSingleObject(g_hMutex, 5000); 
    g_fReleasedMutex = false;
    if (dwWaitResult == WAIT_TIMEOUT)
    {
        char    szMsg[255], szTitle[63];

        // someone else seems to have a login dialog up.  Notify the user
        // about this problem and bail.
        if (!!(dwFlags & UIL_FORCE_UI))
        {
            MLLoadStringA(idsSwitchInProgressSwitch, szMsg, ARRAYSIZE(szMsg));
            MLLoadStringA(idsSwitchIdentities, szTitle, ARRAYSIZE(szTitle));
        }
        else
        {
            MLLoadStringA(idsSwitchInProgressLaunch, szMsg, ARRAYSIZE(szMsg));
            MLLoadStringA(idsIdentityLogin, szTitle, ARRAYSIZE(szTitle));
        }

        MessageBox(hwndParent, szMsg, szTitle, MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);

        return E_UNEXPECTED;
    }

    *ppIdentity = NULL;
    fGotUser = MU_GetUserInfo(NULL, &rUser);
    if (!fGotUser)
    {
        *rUser.szUsername = 0;
        ZeroMemory(&rUser.uidUserID, sizeof(GUID));
    }
    lstrcpy(szOldUsername, rUser.szUsername);

    // if we don't have to do the UI and there is a current 
    // user, then just return that identity
    if (!(dwFlags & UIL_FORCE_UI) && fGotUser)
    {
        pIdentity = new CUserIdentity;
        
        if (!pIdentity)
            hr = E_OUTOFMEMORY;

        if (pIdentity && SUCCEEDED(hr = pIdentity->InitFromUsername(rUser.szUsername)))
            *ppIdentity = pIdentity;
    }
    else
    {
        if (0 == *rUser.szUsername)
        {
            GUID    uidStart;

            MU_GetLoginOption(&uidStart);
            if (GUID_NULL != uidStart)
            {
                MU_GetUserInfo(&uidStart, &rUser);
                rUser.uidUserID = GUID_NULL;
        
            }
        }

        if (MU_Login(hwndParent, dwFlags, rUser.szUsername))
        {
            MLLoadStringA(idsLogoff, szLogoffName, sizeof(szLogoffName));
            if (lstrcmp(szLogoffName, rUser.szUsername) == 0)
            {
                MLLoadStringA(idsConfirmLogoff, szRes, sizeof(szRes));

                if (MessageBox(hwndParent, szRes, szLogoffName, MB_YESNO) == IDYES)
                {
                    ReleaseMutex(g_hMutex);
                    g_fReleasedMutex = true;
                    Logoff(hwndParent);
                }
            }
            else
            {
                pIdentity = new CUserIdentity;
                if (pIdentity)
                {
                    hr = pIdentity->InitFromUsername(rUser.szUsername);

                    if (SUCCEEDED(hr))
                    {
                        pIdentity->GetCookie(&uidNewUserID);

                        hr = _SwitchToUser(&rUser.uidUserID, &uidNewUserID);
                        *ppIdentity = pIdentity;
                    }

                    if (FAILED(hr))
                    {
                        UINT    iMsgId = idsSwitchCancelled;

                        pIdentity->Release();
                        *ppIdentity = NULL;

                        SetForegroundWindow(hwndParent);
                    
                        // could switch on some error codes to set iMsgId to 
                        // other error messages.  For now, skip showing the
                        // message if a user did the cancelling
                        if (hr != E_USER_CANCELLED)
                            MU_ShowErrorMessage(hwndParent, iMsgId, idsSwitchCancelCaption);
                    }
                }
            }
        }
        else
            hr = E_USER_CANCELLED;
    }

    if (!g_fReleasedMutex)
        ReleaseMutex(g_hMutex);

    return hr;
}


STDMETHODIMP CUserIdentityManager::Logoff(HWND hwndParent)
{
    GUID        uidToID = GUID_NULL;
    HRESULT     hr;
    USERINFO    rUser;
    BOOL        fGotUser;

    if (!g_hMutex)
        return E_UNEXPECTED;

    DWORD dwWaitResult;
    dwWaitResult = WaitForSingleObject(g_hMutex, INFINITE);  
    
    if (dwWaitResult != WAIT_OBJECT_0)
        return E_UNEXPECTED;

    fGotUser = MU_GetUserInfo(NULL, &rUser);
    if (!fGotUser)
        rUser.uidUserID = GUID_NULL;

    // switch to the null user
    hr = _SwitchToUser(&rUser.uidUserID, &uidToID);

    if (FAILED(hr))
    {
        UINT    iMsgId = idsLogoutCancelled;

        SetForegroundWindow(hwndParent);
        
        // could switch on some error codes to set iMsgId to 
        // other error messages.  For now, skip showing the
        // message if a user did the cancelling
        if (hr != E_USER_CANCELLED)
            MU_ShowErrorMessage(hwndParent, iMsgId, idsSwitchCancelCaption);
    }

    ReleaseMutex(g_hMutex);

    return hr;
}

STDMETHODIMP CUserIdentityManager::_SwitchToUser(GUID *puidFromUser, GUID *puidToUser)
{
    TCHAR   szUsername[CCH_USERNAME_MAX_LENGTH+1] = "";
    HRESULT hr;

    // switching to the same user is automatically OK.
    if (*puidFromUser == *puidToUser)
        return S_OK;

    // Set up the from and to users
    g_uidOldUserId = *puidFromUser;
    g_uidNewUserId = *puidToUser;
    g_fNotifyComplete = FALSE;
    _PersistChangingIdentities();
    if (*puidToUser != GUID_NULL)
        MU_UserIdToUsername(puidToUser, szUsername, CCH_USERNAME_MAX_LENGTH);
        
    // Notify window's that a switch is coming
    if (SUCCEEDED(hr = _QueryProcessesCanSwitch()))
    {
        if (SUCCEEDED(hr = MU_SwitchToUser(szUsername)))
        {
            if(!g_fReleasedMutex)
            {
                g_fReleasedMutex = true;
                g_fNotifyComplete = true;
                ReleaseMutex(g_hMutex);
            }
            _NotifyIdentitiesSwitched();
        }
    }
    g_fNotifyComplete = TRUE;

    // clear these back out again
    g_uidOldUserId = GUID_NULL;
    g_uidNewUserId = GUID_NULL;
    _ClearChangingIdentities();
    
    return hr;
}

STDMETHODIMP CUserIdentityManager::GetIdentityByCookie(GUID *uidCookie, IUserIdentity **ppIdentity)
{
    CUserIdentity *pIdentity;
    HRESULT hr = E_IDENTITY_NOT_FOUND;
    GUID        uidUserCookie = *uidCookie;

    *ppIdentity = NULL;

    if (MU_IdentitiesDisabled())
    {
        // if disabled, they can only get the default identity. 
        // if asking for the current, they will get the defalt.
        // if asking for default by the constant or the default's guid, then succeed.
        // otherwise return an error.
        if (!MU_GetDefaultUserID(&uidUserCookie ))
            return E_IDENTITY_NOT_FOUND;
        
        if (UID_GIBC_CURRENT_USER == uidUserCookie)
            uidUserCookie = UID_GIBC_DEFAULT_USER;

        if (!(uidUserCookie == uidUserCookie || UID_GIBC_DEFAULT_USER == uidUserCookie))
            return E_IDENTITIES_DISABLED;
    }


    if (uidUserCookie  == UID_GIBC_DEFAULT_USER)
    {
        if (!MU_GetDefaultUserID(&uidUserCookie ))
            return E_IDENTITY_NOT_FOUND;
    }
    else if (uidUserCookie  == UID_GIBC_CURRENT_USER)
    {
        if (!MU_GetCurrentUserID(&uidUserCookie ))
            return E_NO_CURRENT_IDENTITY;
    }
    else if (uidUserCookie  == UID_GIBC_OUTGOING_USER)
    {
        _LoadChangingIdentities();
        if (g_uidOldUserId == GUID_NULL)
            return E_IDENTITY_NOT_FOUND;
        else
            uidUserCookie = g_uidOldUserId;
    }
    else if (uidUserCookie  == UID_GIBC_INCOMING_USER)
    {
        _LoadChangingIdentities();
        if (g_uidNewUserId == GUID_NULL)
            return E_IDENTITY_NOT_FOUND;
        else
            uidUserCookie = g_uidNewUserId;
    }

    pIdentity = new CUserIdentity;
    if (pIdentity)
    {
        hr = pIdentity->InitFromCookie(&uidUserCookie);

        if (SUCCEEDED(hr))
            *ppIdentity = pIdentity;
        else
        {
            // Cleanup
            delete pIdentity;
        }
    }

    return hr;
}

STDMETHODIMP CUserIdentityManager::GetConnectionInterface(IID *pIID)
{
    return E_NOTIMPL;
}

STDMETHODIMP CUserIdentityManager::GetConnectionPointContainer(IConnectionPointContainer **ppCPC)
{
    *ppCPC = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CUserIdentityManager::Advise(IUnknown *pUnkSink, DWORD *pdwCookie)
{
    HRESULT hr;
    EnterCriticalSection(&m_rCritSect);

    AddRef();

    if (!m_pAdviseRegistry)
        m_pAdviseRegistry = new CNotifierList;
    Assert(m_pAdviseRegistry);

    if (m_pAdviseRegistry)
    {
        if (!m_fWndRegistered)
            _CreateWindowClass();

        hr = m_pAdviseRegistry->Add(pUnkSink, pdwCookie);
    }
    else
        hr = E_OUTOFMEMORY;

    LeaveCriticalSection(&m_rCritSect);    
    return hr;
}

STDMETHODIMP CUserIdentityManager::Unadvise(DWORD dwCookie)
{
    HRESULT hr;

    EnterCriticalSection(&m_rCritSect);
    if (m_pAdviseRegistry)
    {
        hr = m_pAdviseRegistry->RemoveCookie(dwCookie);
    }
    else
        hr = E_FAIL;

    LeaveCriticalSection(&m_rCritSect);    

    Release();
    
    return hr;
}
        
STDMETHODIMP CUserIdentityManager::EnumConnections(IEnumConnections **ppEnum)
{
    *ppEnum = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CUserIdentityManager::QuerySwitchIdentities()
{
    HRESULT    hr = S_OK;
    DWORD    dwLength, dwIndex;

    if (!m_pAdviseRegistry)
        return S_OK;

    TraceCall("Identity - CUserIdentityManager::QuerySwitchIdentities");

    dwLength = m_pAdviseRegistry->GetLength();

    for (dwIndex = 0; dwIndex < dwLength; dwIndex++)
    {
        IUnknown    *punk;
        IIdentityChangeNotify    *pICNotify;
        if (SUCCEEDED(m_pAdviseRegistry->GetAtIndex(dwIndex, &punk)) && punk)
        {
            if (SUCCEEDED(punk->QueryInterface(IID_IIdentityChangeNotify, (void **)&pICNotify)) && pICNotify)
            {
                if (FAILED(hr = pICNotify->QuerySwitchIdentities()))
                {
                    punk->Release();
                    pICNotify->Release();
                    goto exit;
                }
                pICNotify->Release();
            }
            punk->Release();
        }
    }
exit:
    return hr;
}

STDMETHODIMP CUserIdentityManager::NotifySwitchIdentities()
{
    HRESULT    hr = S_OK;
    DWORD    dwLength, dwIndex;

    if (!m_pAdviseRegistry)
        return S_OK;

    TraceCall("Identity - CUserIdentityManager::NotifySwitchIdentities");

    dwLength = m_pAdviseRegistry->GetLength();

    for (dwIndex = 0; dwIndex < dwLength; dwIndex++)
    {
        IUnknown    *punk;
        IIdentityChangeNotify    *pICNotify;
        if (SUCCEEDED(m_pAdviseRegistry->GetAtIndex(dwIndex, &punk)) && punk)
        {
            if (SUCCEEDED(punk->QueryInterface(IID_IIdentityChangeNotify, (void **)&pICNotify)) && pICNotify)
            {
                if (FAILED(hr = pICNotify->SwitchIdentities()))
                {
                    punk->Release();
                    pICNotify->Release();
                    goto exit;
                }
                pICNotify->Release();
            }
            punk->Release();
        }
    }
exit:
    return hr;
}

STDMETHODIMP CUserIdentityManager::_QueryProcessesCanSwitch()
{
    HWND    hWnd, hNextWnd = NULL;
    LRESULT lResult;
    HWND   *prghwnd = NULL;
    DWORD   chwnd = 0, cAllocHwnd = 0, dw;
    HRESULT hr;

    TraceCall("Identity - CUserIdentityManager::_QueryProcessesCanSwitch");

    cAllocHwnd = 10;
    if (!MemAlloc((LPVOID*)(&prghwnd), cAllocHwnd * sizeof(HWND)))
        return E_OUTOFMEMORY;

    hWnd = GetTopWindow(NULL);
    while (hWnd)
    {
        hNextWnd = GetNextWindow(hWnd, GW_HWNDNEXT);
        
        if (!IsWindowVisible(hWnd))
        {
            TCHAR   szWndClassName[255];

            GetClassName( hWnd,  szWndClassName, sizeof(szWndClassName));
            
            if (lstrcmp(szWndClassName, c_szNotifyWindowClass) == 0)
            {
                if (chwnd == cAllocHwnd)
                {
                    cAllocHwnd += 10;
                    if (!MemRealloc((LPVOID*)(&prghwnd), cAllocHwnd * sizeof(HWND)))
                        goto exit;
                }
                prghwnd[chwnd++] = hWnd;
            }
        }

        hWnd = hNextWnd;
    }
    
    hr = S_OK;
    for (dw = 0; dw < chwnd; dw++)
    {
        if (IsWindow(prghwnd[dw]))
        {
            lResult = SendMessage(prghwnd[dw], WM_QUERY_IDENTITY_CHANGE, 0, 0);
            if(FAILED((HRESULT)lResult))
            {
                hr = (HRESULT)lResult;
                goto exit;
            }
        }
    }
exit:
    SafeMemFree(prghwnd);
    return hr;
}

STDMETHODIMP CUserIdentityManager::_NotifyIdentitiesSwitched()
{
    HWND    hWnd, hNextWnd = NULL;
    LRESULT lResult;
    HWND   *prghwnd = NULL;
    DWORD   chwnd = 0, cAllocHwnd = 0, dw;

    TraceCall("Identity - CUserIdentityManager::_NotifyIdentitiesSwitched");

    cAllocHwnd = 10;
    if (!MemAlloc((LPVOID*)(&prghwnd), cAllocHwnd * sizeof(HWND)))
        return E_OUTOFMEMORY;

    hWnd = GetTopWindow(NULL);
    while (hWnd)
    {
        hNextWnd = GetNextWindow(hWnd, GW_HWNDNEXT);
        
        if (!IsWindowVisible(hWnd))
        {
            TCHAR   szWndClassName[255];

            GetClassName( hWnd,  szWndClassName, sizeof(szWndClassName));
            
            if (lstrcmp(szWndClassName, c_szNotifyWindowClass) == 0)
            {
                if (chwnd == cAllocHwnd)
                {
                    cAllocHwnd += 10;
                    if (!MemRealloc((LPVOID*)(&prghwnd), cAllocHwnd * sizeof(HWND)))
                        goto exit;
                }
                prghwnd[chwnd++] = hWnd;
            }
        }

        hWnd = hNextWnd;
    }
    
    for (dw = 0; dw < chwnd; dw++)
    {
        DWORD_PTR dwResult;
        if (IsWindow(prghwnd[dw]))
//            lResult = PostMessage(prghwnd[dw], WM_IDENTITY_CHANGED, 0, 0);    //Raid 48054
            SendMessageTimeout(prghwnd[dw], WM_IDENTITY_CHANGED, 0, 0, SMTO_ABORTIFHUNG | SMTO_NORMAL, 1500, &dwResult);
    }
exit:
    SafeMemFree(prghwnd);
    return S_OK;
}

STDMETHODIMP CUserIdentityManager::_CreateWindowClass()
{
    WNDCLASS wc;    
        
    if( !m_fWndRegistered)            /*set up window class and register it */
    {
        wc.lpszClassName    = c_szNotifyWindowClass;
        wc.hInstance        = g_hInst;
        wc.lpfnWndProc      = CUserIdentityManager::WndProc;
        wc.hCursor          = NULL;
        wc.hIcon            = NULL;
        wc.lpszMenuName     = NULL;
        wc.hbrBackground    = NULL;
        wc.style            = CS_DBLCLKS;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;

        if( !RegisterClass( &wc ) )
            return E_FAIL;
        m_fWndRegistered = TRUE;
    }

    return S_OK;
}


LRESULT CALLBACK CUserIdentityManager::WndProc( HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam )
{
    CNotifierList *pList = NULL;
    HRESULT  hr;

    switch(messg)
    {
        case WM_CREATE:
            LPCREATESTRUCT  pcs;

            pcs = (LPCREATESTRUCT)lParam;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LRESULT)pcs->lpCreateParams);
            return( DefWindowProc( hWnd, messg, wParam, lParam ) );
            break;

        case WM_QUERY_IDENTITY_CHANGE:
        case WM_IDENTITY_CHANGED:
        case WM_IDENTITY_INFO_CHANGED:
            pList = (CNotifierList *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
            if (pList)
            {
                hr = pList->SendNotification(messg, (DWORD)lParam);
                return hr;
            }
            break;

        case WM_CLOSE:
            SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
            return( DefWindowProc( hWnd, messg, wParam, lParam ) );
            break;

        default:
            return( DefWindowProc( hWnd, messg, wParam, lParam ) );
 
    }
    return 0;
}
    
    
