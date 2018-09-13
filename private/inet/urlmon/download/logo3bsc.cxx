#include <cdlpch.h>
#include <winineti.h>
#include <shellapi.h>

// Globals

#ifdef LOGO3_SUPPORT_AUTOINSTALL
CList<Logo3CodeDLBSC *, Logo3CodeDLBSC *>     g_CDLList;
CMutexSem                                     g_mxsCDL;
UINT                                          g_Timer;
#endif

Logo3CodeDLBSC::Logo3CodeDLBSC(CSoftDist *pSoftDist,
                               IBindStatusCallback *pClientBSC,
                               LPSTR szCodeBase, LPWSTR wzDistUnit)
: _cRef(1)
, _pIBinding(NULL)
, _pClientBSC(pClientBSC)
, _bPrecacheOnly(FALSE)
, _pbc(NULL)
, _pSoftDist(pSoftDist)
#ifdef LOGO3_SUPPORT_AUTOINSTALL
, _uiTimer(0)
, _hProc(INVALID_HANDLE_VALUE)
#endif
{
    DWORD                 len = 0;

    _szCodeBase = new char[lstrlen(szCodeBase) + 1];
    if (_szCodeBase)
    {
        lstrcpy(_szCodeBase, szCodeBase);
    }

    if (_pClientBSC)
    {
        _pClientBSC->AddRef();
    }

    if (_pSoftDist)
    {
        _pSoftDist->AddRef();
    }
    
    len = WideCharToMultiByte(CP_ACP,0, wzDistUnit, -1, NULL, 0, NULL, NULL);
    _szDistUnit = new TCHAR[len];
    if (_szDistUnit)
    {
        WideCharToMultiByte(CP_ACP, 0, wzDistUnit , -1, _szDistUnit,
                            len, NULL, NULL);
    }
}

Logo3CodeDLBSC::~Logo3CodeDLBSC()
{

    if (_pClientBSC)
    {
        _pClientBSC->Release();
    }
    
    if (_pbc)
    {
        _pbc->Release();
    }

    if (_pSoftDist)
    {
        _pSoftDist->Release();
    }

    delete [] _szCodeBase;
    delete [] _szDistUnit;
}

/*
 *
 * IUnknown Methods
 *
 */

STDMETHODIMP Logo3CodeDLBSC::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT          hr = E_NOINTERFACE;

    *ppv = NULL;
    if (riid == IID_IUnknown || riid == IID_IBindStatusCallback)
    {
        *ppv = (IBindStatusCallback *)this;
    }
    if (*ppv != NULL)
    {
        ((IUnknown *)*ppv)->AddRef();
        hr = S_OK;
    }

    return hr;
}

STDMETHODIMP_(ULONG) Logo3CodeDLBSC::AddRef()
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) Logo3CodeDLBSC::Release()
{
    if (--_cRef)
    {
        return _cRef;
    }
    delete this;

    return 0;
}

/*
 *
 * IBindStatusCallback Methods
 *
 */

STDMETHODIMP Logo3CodeDLBSC::OnStartBinding(DWORD grfBSCOption, IBinding *pib)
{
    HRESULT                  hr = S_OK;

    if (_pIBinding != NULL)
    {
        _pIBinding->Release();
    }
    _pIBinding = pib;

    if (_pIBinding != NULL)
    {
        _pIBinding->AddRef();
    }

    if (_pClientBSC != NULL)
    {
        hr =  _pClientBSC->OnStartBinding(grfBSCOption, pib);
    }

    return hr;
}

STDMETHODIMP Logo3CodeDLBSC::OnStopBinding(HRESULT hr, LPCWSTR szError)
{
#ifdef LOGO3_SUPPORT_AUTOINSTALL
    CLock                             lck(g_mxsCDL); // Mutex this method
#endif
    DWORD                             dwSize = 0;
    LPINTERNET_CACHE_ENTRY_INFO       lpCacheEntryInfo = NULL;
    SHELLEXECUTEINFO                  sei;
    TCHAR                             achShortName[MAX_PATH];
    HANDLE                            hFile = INVALID_HANDLE_VALUE;
    LPWSTR                            pwzUrl = NULL;

#ifdef LOGO3_SUPPORT_AUTOINSTALL
    // These vars are used for trust verification.
    // AS TODO: Get these values from QI'ing and QS'ing from the
    // client BindStatusCallback. For now, just use NULL values.

    IInternetHostSecurityManager     *pHostSecurityManager = NULL;
    HWND                              hWnd = (HWND)INVALID_HANDLE_VALUE;
    PJAVA_TRUST                       pJavaTrust = NULL;
#endif

    AddRef();  // RevokeBindStatusCallback() will destroy us too soon.

    if (_pIBinding != NULL)
    {
        _pIBinding->Release();
        _pIBinding = NULL;
    }
    
#ifdef LOGO3_SUPPORT_AUTOINSTALL
    lpCacheEntryInfo = (LPINTERNET_CACHE_ENTRY_INFO)achBuffer;
    dwSize = MAX_CACHE_ENTRY_INFO_SIZE;
    GetUrlCacheEntryInfo(_szCodeBase, lpCacheEntryInfo, &dwSize);
    GetShortPathName(lpCacheEntryInfo->lpszLocalFileName, achShortName,
                     MAX_PATH);
    // Do trust verification

    if (!_bPrecacheOnly)
    {
        if ( (hFile = CreateFile(achShortName, GENERIC_READ, FILE_SHARE_READ,
                                 NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0))
                                 == INVALID_HANDLE_VALUE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }                             
                                 
        Ansi2Unicode(_szCodeBase, &pwzUrl);
        hr = _wvt.VerifyTrust(hFile, hWnd, &pJavaTrust, pwzUrl,
                              pHostSecurityManager);
                              
        SAFEDELETE(pJavaTrust);  
        SAFEDELETE(pwzUrl);
        CloseHandle(hFile);
    }
#endif


#ifdef LOGO3_SUPPORT_AUTOINSTALL
    if (_bPrecacheOnly)
    {
#endif

     
    if (SUCCEEDED(hr))
    {
        // Mark all downloads of the same group to be downloaded
        hr = _pSoftDist->Logo3DownloadNext();
    }
    else
    {
        // Get last download's group and search for another one to download
        hr = _pSoftDist->Logo3DownloadRedundant();
    }

    if ((FAILED(hr) && hr != E_PENDING) || hr == S_FALSE)
    {
        _pClientBSC->OnStopBinding(hr, szError);
        RecordPrecacheValue(hr);
    }

#ifdef LOGO3_SUPPORT_AUTOINSTALL
    }
    else
    {
        sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
        sei.hwnd = NULL;
        sei.lpVerb = NULL; // "Open" is the default verb
        sei.lpFile = achShortName;
        sei.lpParameters = NULL;
        sei.lpDirectory = NULL;
        sei.nShow = SW_SHOWDEFAULT;
        sei.lpIDList = NULL;
        sei.lpClass = NULL;
        sei.hkeyClass = 0;
        sei.dwHotKey = 0;
        sei.hIcon = INVALID_HANDLE_VALUE;
        sei.cbSize = sizeof(sei);
    
        if (!ShellExecuteEx(&sei)) {
            goto Exit;
        }
    
        _hProc = sei.hProcess;
        if (!g_Timer) {
            g_Timer = SetTimer(NULL, (UINT)this, TIMEOUT_INTERVAL, TimeOutProc);
        }
        g_CDLList.AddTail(this);
        AddRef();
    }
#endif

    Assert(_pbc);
    RevokeBindStatusCallback(_pbc, this);
    
    _pbc->Release();
    _pbc = NULL;

    Release();
    
    return hr;
}

STDMETHODIMP Logo3CodeDLBSC::OnObjectAvailable(REFIID riid, IUnknown *punk)
{
    HRESULT                  hr = S_OK;

    if (_pClientBSC)
    {
        hr = _pClientBSC->OnObjectAvailable(riid, punk);
    }

    return hr;
}

STDMETHODIMP Logo3CodeDLBSC::GetPriority(LONG *pnPriority)
{
    HRESULT                  hr = S_OK;

    if (_pClientBSC)
    {
        hr = _pClientBSC->GetPriority(pnPriority);
    }

    return hr;
}

STDMETHODIMP Logo3CodeDLBSC::OnLowResource(DWORD dwReserved)
{
    HRESULT                  hr = S_OK;

    if (_pClientBSC)
    {
        hr = _pClientBSC->OnLowResource(dwReserved);
    }

    return hr;
}  

STDMETHODIMP Logo3CodeDLBSC::OnProgress(ULONG ulProgress, ULONG ulProgressMax,
                                    ULONG ulStatusCode,
                                    LPCWSTR szStatusText)
{
    HRESULT                  hr = S_OK;

    if (_pClientBSC)
    {
        hr = _pClientBSC->OnProgress(ulProgress, ulProgressMax,
                                     ulStatusCode, szStatusText);
    }

    return hr;
}

STDMETHODIMP Logo3CodeDLBSC::GetBindInfo(DWORD *pgrfBINDF, BINDINFO *pbindInfo)
{
    HRESULT                  hr = S_OK;

    *pgrfBINDF |= BINDF_ASYNCHRONOUS;
    if (_pClientBSC)
    {
        hr = _pClientBSC->GetBindInfo(pgrfBINDF, pbindInfo);
        if (*pgrfBINDF & BINDF_SILENTOPERATION)
        {
            _bPrecacheOnly = TRUE;
        }
    }

    return S_OK;
}

STDMETHODIMP Logo3CodeDLBSC::OnDataAvailable(DWORD grfBSCF, DWORD dwSize,
                                         FORMATETC *pformatetc,
                                         STGMEDIUM *pstgmed)
{
    HRESULT                  hr = S_OK;

    if (_pClientBSC)
    {
        hr = _pClientBSC->OnDataAvailable(grfBSCF, dwSize, pformatetc,
                                          pstgmed);
    }
    return S_OK;
}

void Logo3CodeDLBSC::SetBindCtx(IBindCtx *pbc)
{
    _pbc = pbc;

    if (_pbc)
    {
        _pbc->AddRef();
    }
}

STDMETHODIMP Logo3CodeDLBSC::RecordPrecacheValue(HRESULT hr)
{
    HRESULT                           hrRet = S_OK;
    DWORD                             lResult = 0;
    HKEY                              hkeyLogo3 = 0;
    HKEY                              hkeyVersion = 0;
    char                              achBuffer[MAX_CACHE_ENTRY_INFO_SIZE];
    static const char                *szAvailableVersion = "AvailableVersion";
    static const char                *szPrecache = "Precache";

    if (_szDistUnit != NULL)
    {
        wsprintf(achBuffer, "%s\\%s", REGSTR_PATH_LOGO3_SETTINGS, _szDistUnit);
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, achBuffer, 0, KEY_ALL_ACCESS,
                         &hkeyLogo3) == ERROR_SUCCESS)
        {
            if (RegOpenKeyEx(hkeyLogo3, szAvailableVersion,
                             0, KEY_ALL_ACCESS, &hkeyVersion) != ERROR_SUCCESS)
            {
                if ((lResult = RegCreateKey(hkeyLogo3, szAvailableVersion,
                                            &hkeyVersion)) != ERROR_SUCCESS)
                {
                    hrRet = HRESULT_FROM_WIN32(lResult);
                    goto Exit;
                }
            }
        
            // record result of caching bits.
        
            HRESULT hrRecord = hr;
            
            if (FAILED(hr))
            {
                hrRecord = ERROR_IO_INCOMPLETE;
            }
            
            lResult = ::RegSetValueEx(hkeyVersion, szPrecache, NULL, REG_DWORD, 
                                      (unsigned char *)&hrRecord, sizeof(DWORD));

            if (lResult != ERROR_SUCCESS)
            {
                hrRet = HRESULT_FROM_WIN32(lResult);
                goto Exit;
            }
        }
    }
    else {
        hrRet = E_INVALIDARG;
    }

Exit:

    if (hkeyLogo3)
    {
        RegCloseKey(hkeyLogo3);
    }

    if (hkeyVersion)
    {
        RegCloseKey(hkeyVersion);
    }

    return hrRet;
}

#ifdef LOGO3_SUPPORT_AUTOINSTALL

void Logo3CodeDLBSC::TimeOut()
{
    CLock                             lck(g_mxsCDL); // Mutex req for CDLList
    LISTPOSITION                      pos = NULL;

    if (WaitForSingleObjectEx(_hProc, 0, FALSE) == WAIT_OBJECT_0)
    {
        Assert(_pClientBSC);
        _pClientBSC->OnStopBinding(S_OK, NULL);

        // Remove self from code download list
        pos = g_CDLList.Find(this);
        Assert(pos);
        g_CDLList.RemoveAt(pos);
        Release();

        // If no more code downloads, kill the timer
        if (!g_CDLList.GetCount()) {
            KillTimer(NULL, g_Timer);
            g_Timer = 0;
        }
    }
}


void CALLBACK TimeOutProc(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    LISTPOSITION                    pos = NULL;
    Logo3CodeDLBSC                 *bsc = NULL;

    lpos = g_CDLList.GetHeadPosition();
    while (lpos) {
        bsc = g_CDLList.GetNext(pos);
        Assert(bsc);
        bsc->TimeOut();
    }
}

#endif
