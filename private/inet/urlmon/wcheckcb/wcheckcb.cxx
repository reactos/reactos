#include <urlmon.h>
#include <htmlguid.h>
#include <ocidl.h>
#include "..\\inc\\urlint.h"
#include "..\\inc\\wcheckcb.h"
#include "..\\inc\\debug.h"
#include "..\\download\\cdl.h"
#include "Status.h"

// {5DFE9E81-46E4-11d0-94E8-00AA0059CE02}
const CLSID CLSID_ControlRefreshCallback = {
                                0x5dfe9e81, 0x46e4, 0x11d0, 
                                0x94, 0xe8, 0x0, 0xaa, 0x0, 
                                0x59, 0xce, 0x2
                                };

const static TCHAR *g_pszUpdateInfo = TEXT("UpdateInfo");
const static TCHAR *g_pszNewControlInCache = TEXT("NewControlInCache");

#define CLSID_MAX_LENGTH 50
#define CONTENT_MAX_LENGTH 1024

/******************************************************************************
    Constructor, Destructor and helper methods
******************************************************************************/

CControlRefreshCallback::CControlRefreshCallback()
{
    DllAddRef();
    m_cRef = 1;
    m_clsidCtrl = CLSID_NULL;
    m_wszURL[0] = '\0';
}

CControlRefreshCallback::~CControlRefreshCallback()
{
    Assert(m_cRef == 0);
    DllRelease();
}

// Give this object information such as the clsid and url of the control
// it's dealing with 
STDMETHODIMP CControlRefreshCallback::SetInfo(
                                        REFCLSID rclsidControl,
                                        LPCWSTR lpwszURL)
{
    m_clsidCtrl = rclsidControl;

    // copy wide strings
#ifdef UNICODE
    lstrcpyW(m_wszURL, lpwszURL);   // only works in Win32 mode
                                    // fails on Win95
#else
    INT i = 0;
    for(; lpwszURL[i]; i++)
        m_wszURL[i] = lpwszURL[i];
    m_wszURL[i] = '\0';
#endif

    Assert(lstrlenW(m_wszURL) == lstrlenW(lpwszURL));

    return S_OK;
}

/*
HRESULT CControlRefreshCallback::UpdateControlInCacheFlag(
                                               SCODE scReason) const
{
    // update flag in registry so that IE initiates a new
    // download when the control is visited.

    LONG lResult = ERROR_SUCCESS;
    TCHAR szKey[MAX_PATH];
    LPOLESTR pwcsClsid = NULL;
    BOOL fChanged = (scReason == S_OK);

    lstrcpy(szKey, TEXT("CLSID\\"));

    if (SUCCEEDED(::StringFromCLSID(m_clsidCtrl, &pwcsClsid)))
    {
        HKEY hKey = NULL;
        int nLen = lstrlen(szKey);

        if (WideCharToMultiByte(
                           CP_ACP, 0, pwcsClsid, -1, szKey + nLen, 
                           MAX_PATH - nLen, NULL, NULL) > 0)
        {
            lstrcat(szKey, "\\");
            lstrcat(szKey, g_pszUpdateInfo);
            lResult = RegOpenKeyEx(
                              HKEY_CLASSES_ROOT, szKey, 0, 
                              KEY_ALL_ACCESS, &hKey); 

            if (lResult == ERROR_SUCCESS)
            {
                DWORD dwKeySet = 0;
                DWORD dwSize = sizeof(DWORD);
                lResult = RegQueryValueEx(
                                    hKey, g_pszNewControlInCache, NULL, NULL,
                                    (LPBYTE)&dwKeySet, &dwSize);

                if (lResult != ERROR_SUCCESS || (dwKeySet == 0 && fChanged))
                {
                    dwKeySet = (fChanged ? 1 : 0);
                    lResult = RegSetValueEx(
                                    hKey, g_pszNewControlInCache, 0, 
                                    REG_DWORD, (LPBYTE)&dwKeySet, sizeof(DWORD));
                }
                RegCloseKey(hKey);
            }
        }

        delete pwcsClsid;
    }

    return (lResult == ERROR_SUCCESS ? S_OK : HRESULT_FROM_WIN32(lResult));
}
*/

HRESULT CControlRefreshCallback::DownloadControl() const
{
    HRESULT hr = S_OK;
    CSilentCodeDLSink *pscdls = NULL;
    LPBC pbc = NULL;

    Assert(lstrlenW(m_wszURL) > 0);
    Assert(m_clsidCtrl != CLSID_NULL);

    pscdls = new CSilentCodeDLSink;
    if (pscdls == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = CreateBindCtx(0, &pbc);
    if (SUCCEEDED(hr))
        hr = RegisterBindStatusCallback(pbc, pscdls, NULL, 0);

    if (FAILED(hr))
        goto Exit;

    hr = AsyncGetClassBits(
                       m_clsidCtrl, NULL, NULL, 
                       (DWORD)-1, (DWORD)-1, m_wszURL,
                       pbc, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER, 
                       NULL, IID_IClassFactory, 0);

    if (hr == MK_S_ASYNCHRONOUS)
        hr = pscdls->WaitTillNotified();

    RevokeBindStatusCallback(pbc, pscdls);

Exit:

    if (pbc)
        pbc->Release();

    if (pscdls)
        pscdls->Release();

    return hr;
}

/******************************************************************************
    IUnknown Methods
******************************************************************************/

STDMETHODIMP CControlRefreshCallback::QueryInterface(
                                                REFIID iid, 
                                                void** ppvObject)
{
    *ppvObject = NULL;

    if (iid == IID_IUnknown)
    {
        *ppvObject = (void*)this;
    }
    else if (iid == IID_IPersistStream)
    {
        *ppvObject = (void*)(IPersistStream*)this;
    }
    else if (iid == IID_IWebCheckAdviseSink)
    {
        *ppvObject = (void*)(IWebCheckAdviseSink*)this;
    }

    if (*ppvObject) 
    {
        ((LPUNKNOWN)*ppvObject)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CControlRefreshCallback::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CControlRefreshCallback::Release()
{
    if (--m_cRef)
        return m_cRef;

    delete this;
    return 0;   
}


/******************************************************************************
    IPersistStream Methods
******************************************************************************/

STDMETHODIMP CControlRefreshCallback::GetClassID(CLSID* pClsssID)
{
    *pClsssID = CLSID_ControlRefreshCallback;
    return S_OK;
}

STDMETHODIMP CControlRefreshCallback::IsDirty(void)
{
    Assert(m_clsidCtrl != CLSID_NULL);
    return (m_clsidCtrl != CLSID_NULL ? S_OK : S_FALSE);
}

STDMETHODIMP CControlRefreshCallback::Load(IStream* pStm)
{
    DWORD dwLen = 0;

    ULONG cb = sizeof(CLSID);
    ULONG cbRead = 0;
    pStm->Read((void*)&m_clsidCtrl, cb, &cbRead);
    Assert(cb == cbRead);
    if (cb != cbRead)
        goto Exit;

    cb = sizeof(DWORD);
    cbRead = 0;
    pStm->Read((void*)&dwLen, cb, &cbRead);
    Assert(cb == cbRead);
    if (cb != cbRead)
        goto Exit;

    cb = dwLen * sizeof(WCHAR);
    cbRead = 0;
    pStm->Read((void*)&m_wszURL, cb, &cbRead);
    Assert(cb == cbRead);

    Assert((DWORD)lstrlenW(m_wszURL) == dwLen - 1);

Exit:

    return (cbRead == cb ? S_OK : E_FAIL);
}

STDMETHODIMP CControlRefreshCallback::Save(IStream* pStm, BOOL fClearDirty)
{
    DWORD dwLen = lstrlenW(m_wszURL) + 1;    // add 1 for NULL char

    ULONG cb = sizeof(CLSID);
    ULONG cbSaved = 0;
    pStm->Write((void*)&m_clsidCtrl, cb, &cbSaved);
    Assert(cb == cbSaved);
    if (cb != cbSaved)
        goto Exit;

    cb = sizeof(DWORD);
    cbSaved = 0;
    pStm->Write((void*)&dwLen, cb, &cbSaved);
    Assert(cb == cbSaved);
    if (cb != cbSaved)
        goto Exit;

    cb = dwLen * sizeof(WCHAR);
    cbSaved = 0;
    pStm->Write((void*)m_wszURL, cb, &cbSaved);
    Assert(cb == cbSaved);

Exit:

    return (cbSaved == cb ? S_OK : E_FAIL);
}

STDMETHODIMP CControlRefreshCallback::GetSizeMax(ULARGE_INTEGER* pcbSize)
{
    pcbSize->QuadPart = sizeof(CLSID) + sizeof(DWORD) + sizeof(m_wszURL);
    return S_OK;
}


/******************************************************************************
    IWebCheckAdviseSink Methods
******************************************************************************/

STDMETHODIMP CControlRefreshCallback::UpdateBegin(
                                              long lCookie, 
                                              SCODE scReason, 
                                              BSTR lpURL)
{
    return S_OK;
}

// scReason -- S_OK means changed, S_FALSE means no changes
STDMETHODIMP CControlRefreshCallback::UpdateEnd(
                                            long lCookie, 
                                            SCODE scReason)
{
    Assert(m_clsidCtrl != CLSID_NULL);
    LONG lResult = ERROR_SUCCESS;

    if (scReason == S_OK)
    {
        DownloadControl();
//        UpdateControlInCacheFlag(scReason);
    }

    return S_OK;
}

STDMETHODIMP CControlRefreshCallback::UpdateProgress(
                                                 long lCookie, 
                                                 long lCurrent, 
                                                 long lMax)
{
    return S_OK;
}

