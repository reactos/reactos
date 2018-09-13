//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994
//
//  File:       reload.cpp ( based on WebCheck's downld.cxx )
//
//  Contents:   Implmentation of Office9 Thicket Save API
//
//----------------------------------------------------------------------------

#include "priv.h"

//#include "headers.hxx"
#include "reload.h"

#include <exdisp.h>
#include <exdispid.h>
#include <htiface.h>
#include <mshtmdid.h>
#include <mshtmcid.h>
#include <mshtmhst.h>
#include <optary.h>                 // needed for IHtmlLoadOptions
#include <wininet.h>
#include <winineti.h>
#include <shlguid.h>
#include <shlobj.h>
#include "intshcut.h"               // IUniformResourceLocator

#undef DEFINE_STRING_CONSTANTS
#pragma warning( disable : 4207 ) 
#include "htmlstr.h"
#pragma warning( default : 4207 )

// Disable perf warning for typecasts to the native bool type
// Useful for NT64 where pointers cannot be cast to "BOOL"
#pragma warning( disable : 4800 )

//MtDefine(CUrlDownload, Utilities, "CUrlDownload")

#define TF_THISMODULE   TF_DOWNLD

LRESULT UrlDownloadWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);


CLIPFORMAT g_cfHTML=CF_NULL;

// User-Agent strings
const WCHAR c_szUserAgent95[] = L"Mozilla/4.0 (compatible; MSIE 4.01; MSIECrawler; Windows 95)";
const WCHAR c_szUserAgentNT[] = L"Mozilla/4.0 (compatible; MSIE 4.01; MSIECrawler; Windows NT)";

// Refresh header for http-equiv (client-pull)
const WCHAR c_wszRefresh[] = L"Refresh";

const int  MAX_CLIENT_PULL_NUM = 4;     // max # redirections
const int  MAX_CLIENT_PULL_TIMEOUT = 6; // max timeout we'll follow

// Function also present in shdocvw\basesb.cpp and in mshtml
BOOL DLParseRefreshContent(LPWSTR pwzContent, UINT * puiDelay, LPWSTR pwzUrlBuf, UINT cchUrlBuf);

const WCHAR c_wszHeadVerb[] = L"HEAD";

const WCHAR c_szUserAgentPrefix[] = L"User-Agent: ";
const WCHAR c_szAcceptLanguagePrefix[] = L"Accept-Language: ";

#define WM_URLDL_CLEAN      (WM_USER + 0x1010)
#define WM_URLDL_ONDLCOMPLETE (WM_USER + 0x1012)
#define WM_URLDL_CLIENTPULL (WM_USER+0x1013)

#define PROP_CODEPAGE       3

const PROPSPEC c_rgPropRead[] = {
    { PRSPEC_PROPID, PID_INTSITE_SUBSCRIPTION},
    { PRSPEC_PROPID, PID_INTSITE_FLAGS},
    { PRSPEC_PROPID, PID_INTSITE_TRACKING},
    { PRSPEC_PROPID, PID_INTSITE_CODEPAGE},
};

//---------------------------------------------------------------
// CUrlDownload class
CUrlDownload::CUrlDownload( CThicketProgress* ptp, HRESULT *phr, UINT cpDL )
{
    // Maintain global count of objects
    //DllAddRef();

    m_ptp = ptp;
    m_phr = phr;
    m_cpDL = cpDL;
    m_dwProgMax = 0;

    m_cRef = 1;
    
    m_pDocument = NULL;
    m_dwConnectionCookie = 0;
    m_pwszURL = NULL;
    m_pScript = NULL;
    m_fAdviseOn = FALSE;
    m_pCP = NULL;
    m_pDocument = NULL;
    m_pPersistMk = NULL;
    m_pOleCmdTarget = NULL;
    m_pwszClientPullURL = NULL;
    m_fWaitingForReadyState = FALSE;
    m_fFormSubmitted = FALSE;
    m_fBrowserValid = FALSE;
    m_hwndMe = NULL;

    // find the HTML clipboard format
    if (!g_cfHTML)
    {
        g_cfHTML = RegisterClipboardFormat(CFSTR_MIME_HTML);
    }

    // find out if we need to set the "RESYNCHRONIZE" flag
    INTERNET_CACHE_CONFIG_INFOA CacheConfigInfo;
    DWORD dwBufSize = sizeof(CacheConfigInfo);

    if (GetUrlCacheConfigInfoA(&CacheConfigInfo, &dwBufSize, CACHE_CONFIG_SYNC_MODE_FC))
    {
        if ((WININET_SYNC_MODE_ONCE_PER_SESSION == CacheConfigInfo.dwSyncMode) ||
             (WININET_SYNC_MODE_NEVER == CacheConfigInfo.dwSyncMode) ||
             (WININET_SYNC_MODE_AUTOMATIC == CacheConfigInfo.dwSyncMode))
        {
            m_fSetResync = FALSE;
        }
        else
        {
            m_fSetResync = TRUE;
        }
    }
    else
        ASSERT(FALSE);

    m_lBindFlags = DLCTL_SILENT | DLCTL_NO_SCRIPTS | DLCTL_NO_FRAMEDOWNLOAD | 
        DLCTL_NO_JAVA | DLCTL_NO_RUNACTIVEXCTLS | DLCTL_NO_DLACTIVEXCTLS;
    if (m_fSetResync)
        m_lBindFlags |= DLCTL_RESYNCHRONIZE;

    // register our window class if necessary
    WNDCLASS wc;

    wc.style = 0;
    wc.lpfnWndProc = UrlDownloadWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = g_hinst;
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = (HBRUSH)NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = URLDL_WNDCLASS;

    SHRegisterClass(&wc);

}

CUrlDownload::~CUrlDownload()
{
    // Maintain global count of objects
    //DllRelease();

    CleanUp();
}

void CUrlDownload::CleanUpBrowser()
{
    SAFERELEASE(m_pScript);

    if (m_fAdviseOn)
    {
        UnAdviseMe();
    }
    SAFERELEASE(m_pCP);
    SAFERELEASE(m_pDocument);
    SAFERELEASE(m_pPersistMk);
    SAFERELEASE(m_pOleCmdTarget);
    SAFELOCALFREE(m_pwszClientPullURL);
}

void CUrlDownload::CleanUp()
{
    CleanUpBrowser();
    SAFELOCALFREE(m_pwszURL);

    if (m_hwndMe)
    {
        SetWindowLongPtr(m_hwndMe, GWLP_USERDATA, 0);
        DestroyWindow(m_hwndMe);
        m_hwndMe = NULL;
    }
}

LRESULT UrlDownloadWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    CUrlDownload *pThis = (CUrlDownload *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    // Validate pThis
#ifdef DEBUG
    if (pThis && IsBadWritePtr(pThis, sizeof(*pThis)))
    {
        ASSERT(FALSE);
    }
#endif

    switch (Msg)
    {
    case WM_CREATE :
        {
            LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;

            if (!pcs || !(pcs->lpCreateParams))
            {
                return -1;
            }
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) pcs->lpCreateParams);
            return 0;
        }

    case WM_URLDL_CLIENTPULL :
    case WM_URLDL_ONDLCOMPLETE :
        if (pThis)
            pThis->HandleMessage(hWnd, Msg, wParam, lParam);
        break;

    default:
        return DefWindowProc(hWnd, Msg, wParam, lParam);
    }
    return 0;
}

HRESULT CUrlDownload::CreateMyWindow()
{
    // Create our callback window
    if (NULL == m_hwndMe)
    {
//      TraceMsg(TF_THISMODULE, "Creating MeWnd, this=0x%08x", (DWORD)this);
        m_hwndMe = CreateWindow(URLDL_WNDCLASS, TEXT("CUrlDownloadWnd"), WS_OVERLAPPED,
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    NULL, NULL, g_hinst, (LPVOID)this);

        if (NULL == m_hwndMe)
        {
            return E_FAIL;
        }
    }
    return S_OK;
}

HRESULT CUrlDownload::BeginDownloadURL2(
    LPCWSTR     pwszURL,        // URL
    BDUMethod   iMethod,        // download method
    BDUOptions  iOptions,       // download options
    LPTSTR      pszLocalFile,   // Local file to download to instead of cache
    DWORD       dwMaxSize       // Max size in bytes; will abort if exceeded
)
{
    HRESULT hr = S_OK;

    // Param validation
    ASSERT(pwszURL);
    ASSERT(!(iOptions & BDU2_NEEDSTREAM));
    ASSERT(!pszLocalFile);

    if (pszLocalFile)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CreateMyWindow();

        // Clean up some old stuff
        SAFERELEASE(m_pScript);

        m_fBrowserValid = FALSE;

        m_iMethod = iMethod;
        m_iOptions = iOptions;

        m_dwMaxSize = dwMaxSize;

        SAFELOCALFREE(m_pwszClientPullURL);
        m_iNumClientPull = 0;

        // Save URL
        SAFELOCALFREE(m_pwszURL);
        m_pwszURL = StrDupW(pwszURL);

        // Determine how to download this URL
        hr = BeginDownloadWithBrowser(pwszURL);
    }

    if (FAILED(hr))
    {
        OnDownloadComplete(BDU2_ERROR_GENERAL);
    }

    return (hr);
}

//
// Looks up the Url in the url history object and if its not CP_ACP
// inserts an IHTMLLoadOptions object that contains the codepage
// into the bind context
//


HRESULT InsertCodepageIntoBindCtx(UINT codepage, IBindCtx * pbc)
{
    HRESULT hr = S_OK;

    if (pbc == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (codepage != CP_ACP)
        {
            DWORD dwcp = codepage;
            //
            // We got a codepage that wasn't the ansi one create an
            // HTMLLoadOptions object and set the code page in it.
            //
            IHtmlLoadOptions *phlo = NULL;
            hr = CoCreateInstance(CLSID_HTMLLoadOptions, NULL, 
                CLSCTX_INPROC_SERVER, IID_IHtmlLoadOptions, (void**)&phlo);

            if (SUCCEEDED(hr) && phlo)
            {
                hr = phlo->SetOption(HTMLLOADOPTION_CODEPAGE, &dwcp,
                    sizeof(dwcp));

                if (SUCCEEDED(hr))
                {
                    //
                    // Insert the option into the bindctx
                    //
                    pbc->RegisterObjectParam(L"__HTMLLOADOPTIONS", phlo);
                }
                phlo->Release();
            }
        }
    }
    return hr; // no return  - may return S_FALSE
}

HRESULT CUrlDownload::BeginDownloadWithBrowser(LPCWSTR pwszURL)
{
    HRESULT hr;

    // Get browser and hook up sink
    // (no-op if we're already set up)
    hr = GetBrowser();

    if (SUCCEEDED(hr))
    {
        // browse to the required URL
        LPMONIKER           pURLMoniker = NULL;
        IBindCtx           *pbc = NULL;

        // create a URL moniker from the canonicalized path
        hr=CreateURLMoniker(NULL, pwszURL, &pURLMoniker);

        // create an empty bind context so that Urlmon will call Trident's
        //  QueryService on the proper thread so that Trident can delegate
        //  it to use properly.
        hr=CreateBindCtx(0, &pbc);

        if (SUCCEEDED(hr))
        {
            //
            // Looks up the Url in the url history object and if its not CP_ACP
            // inserts an IHTMLLoadOptions object that contains the codepage
            // into the bind context. This is done so that TRIDENT is seeded
            // with the correct codepage.
            //
            InsertCodepageIntoBindCtx(m_cpDL, pbc);

            hr = m_pPersistMk->Load(FALSE, pURLMoniker, pbc, 0);
            if (SUCCEEDED(hr)) m_fWaitingForReadyState = TRUE;
        }

        // clean up junk
        if (pURLMoniker)
            pURLMoniker->Release();

        if (pbc)
            pbc->Release();

        if (SUCCEEDED(hr))
        {
            m_fBrowserValid = TRUE;
        }
        else
        {
            CleanUpBrowser();
        }
    }

    return (hr);
}

HRESULT CUrlDownload::OnDownloadComplete(int iError)
{
    PostMessage(m_hwndMe, WM_URLDL_ONDLCOMPLETE, (WPARAM)iError, 0);
    return S_OK;
}

BOOL CUrlDownload::HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_URLDL_CLIENTPULL :
        {
            HRESULT hr = S_OK;

            // Ask our parent if we should do this
            if (m_pwszClientPullURL)
            {
                if (m_iNumClientPull >= MAX_CLIENT_PULL_NUM)
                    hr = E_FAIL;
            }

            if (SUCCEEDED(hr))
            {
                // Download this new url. Don't give "downloadcomplete" for first one
                // Save member vars since they get reset in BDU2
                int iNumClientPull = m_iNumClientPull;
                LPWSTR pszNewURL = m_pwszClientPullURL;

                m_pwszClientPullURL = NULL;
                hr = BeginDownloadURL2(pszNewURL, m_iMethod, m_iOptions, NULL, m_dwMaxSize);
                SAFELOCALFREE(pszNewURL);
                if (SUCCEEDED(hr))
                {
                    m_iNumClientPull = iNumClientPull + 1;
                }
            }
        }
        break;

    case WM_URLDL_ONDLCOMPLETE :
        ASSERT(m_phr);
        *m_phr = S_OK;
        return TRUE;

    default:
        break;

    }
    return TRUE;
}

HRESULT CUrlDownload::AbortDownload(int iErrorCode /* =-1 */)
{
    HRESULT hr=S_FALSE;
    BOOL    fAborted=FALSE;

    if (m_fBrowserValid)
    {
        ASSERT(m_pOleCmdTarget);
        if (m_pOleCmdTarget)
        {
            m_pOleCmdTarget->Exec(NULL, OLECMDID_STOP, 0, NULL, NULL);
        }

        SAFELOCALFREE(m_pwszClientPullURL);

        fAborted=TRUE;
        m_fBrowserValid = FALSE;
    }

    return hr; // no return  - may return S_FALSE
}

// Loads browser, creates sink and hooks it up to DIID_DWebBrowserEvents
HRESULT CUrlDownload::GetBrowser()
{
    HRESULT hr = S_OK;

    if (m_fAdviseOn)
        return (hr);

    if (NULL == m_pDocument)
    {
        ASSERT(!m_pPersistMk);
        ASSERT(!m_pCP);

        hr = CoCreateInstance(CLSID_HTMLDocument, NULL,
                    CLSCTX_INPROC, IID_IHTMLDocument2, (void **)&m_pDocument);

        if (SUCCEEDED(hr)) // setting design mode faults Trident && SUCCEEDED(hr = m_pDocument->put_designMode( (BSTR)c_bstr_ON )))
        {
            IOleObject *pOleObj;

            hr = m_pDocument->QueryInterface(IID_IOleObject, (void **)&pOleObj);
            if (SUCCEEDED(hr))
            {
                pOleObj->SetClientSite((IOleClientSite *)this);
                pOleObj->Release();
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = m_pDocument->QueryInterface(IID_IPersistMoniker, (void**)&m_pPersistMk);
        }

        if (SUCCEEDED(hr))
        {
            hr = m_pDocument->QueryInterface(IID_IOleCommandTarget, (void**)&m_pOleCmdTarget);
        }

        ASSERT(SUCCEEDED(hr));
    }

    // At this point we have m_pDocument and m_pPersistMk

    // Find our connection point if necessary
    if (NULL == m_pCP && SUCCEEDED(hr))
    {
        IConnectionPointContainer *pCPCont=NULL;
        hr = m_pDocument->QueryInterface(IID_IConnectionPointContainer,
                (void **)&pCPCont);

        if (SUCCEEDED(hr))
        {
            hr = pCPCont->FindConnectionPoint(IID_IPropertyNotifySink, &m_pCP);
            pCPCont->Release();
            pCPCont = NULL;
        }
    }

    // And hook it up to us
    if (SUCCEEDED(hr))
    {
        // create sink
        IPropertyNotifySink *pSink = (IPropertyNotifySink *)this;

        hr = m_pCP->Advise(pSink, &m_dwConnectionCookie);
        if (SUCCEEDED(hr))
        {
            m_fAdviseOn = TRUE;
        }
    }

    return (hr);
}

void CUrlDownload::UnAdviseMe()
{
    if (m_fAdviseOn)
    {
        m_pCP->Unadvise(m_dwConnectionCookie);
        m_fAdviseOn = FALSE;
    }
}

void CUrlDownload::DestroyBrowser()
{
    CleanUpBrowser();
}

void CUrlDownload::DoneDownloading()
{
    // Don't send any more messages to the parent

    AbortDownload();

    CleanUp();
}

HRESULT CUrlDownload::GetScript(IHTMLWindow2 **ppWin)
{
    HRESULT hr = E_FAIL;
    IDispatch *pDisp=NULL;

    ASSERT(ppWin);
    *ppWin=NULL;

    if (!m_fBrowserValid)
    {
        return (E_FAIL);
    }

    *ppWin = NULL;

    if (m_pScript)
    {
        m_pScript->AddRef();
        *ppWin = m_pScript;
        return S_OK;
    }

    if (m_pDocument)
    {
        hr = m_pDocument->get_Script(&pDisp);
        if (!pDisp) hr=E_NOINTERFACE;
    }

    if (SUCCEEDED(hr))
    {
        hr = pDisp->QueryInterface(IID_IHTMLWindow2, (void **)ppWin);
        if (*ppWin == NULL) hr = E_NOINTERFACE;
        pDisp->Release();
    }

    // Save this so future GetScript() calls much faster
    ASSERT(!m_pScript);
    if (SUCCEEDED(hr))
    {
        m_pScript = *ppWin;
        m_pScript->AddRef();
    }

    return (hr);
}


// Returns pointer to '.' or pointer to null-terminator or query '?'
LPWSTR                  // ptr to period or to null-term or '?'
URLFindExtensionW(
    LPCWSTR pszURL,
    int *piLen)         // length including period
{
    LPCWSTR pszDot;

    ASSERT(pszURL);

    for (pszDot = NULL; *pszURL && *pszURL!=TEXT('?'); pszURL++)
    {
        switch (*pszURL) {
        case TEXT('.'):
            pszDot = pszURL;         // remember the last dot
            break;
        case TEXT('/'):
            pszDot = NULL;       // forget last dot, it was in a directory
            break;
        }
    }

    if (piLen)
    {
        if (pszDot)
            *piLen = (int) (pszURL-pszDot);
        else
            *piLen = 0;
    }

    // if we found the extension, return ptr to the dot, else
    // ptr to end of the string (NULL extension) (cast->non const)
    return pszDot ? (LPWSTR)pszDot : (LPWSTR)pszURL;
}

// Returns TRUE if this appears to be an HTML URL
BOOL CUrlDownload::IsHTMLURL(LPCWSTR lpURL)
{
    LPWSTR pwch;
    int iLen;

    pwch = URLFindExtensionW(lpURL, &iLen);

    if (*pwch && iLen)
    {
        // We found an extension. Check it out.
        if ((iLen<4 || iLen>5) ||
            (iLen == 5 &&
                (StrCmpNIW(pwch, L".html", 5))) ||
            (iLen == 4 &&
                (StrCmpNIW(pwch, L".htm", 4) &&
                 StrCmpNIW(pwch, L".htt", 4) &&
                 StrCmpNIW(pwch, L".asp", 4) &&
                 StrCmpNIW(pwch, L".htx", 4)
                                            )))
        {
            // extension < 3 chars, or extension > 4 chars, or
            // extension not one of the above
            return FALSE;
        }
    }
    else
        return FALSE;       // no extension. Can't assume it's HTML.

    return TRUE;
}

// Returns TRUE if this is a URL we should try to download (http:)
BOOL CUrlDownload::IsValidURL(LPCWSTR lpURL)
{
    // Check protocol
//  HKEY hk;

#if 0
    BOOL fValidProtocol = FALSE;

    // Always accept http or https
    if (!StrCmpNIW(lpURL, L"http", 4))
        fValidProtocol = TRUE;

    if (!fValidProtocol &&
        (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT("PROTOCOLS\\Handler"), 0, KEY_QUERY_VALUE, &hk)))
    {
        // Crack out protocol
        DWORD dwData=0, cbData = sizeof(DWORD);
        int i;
        char ch[16];

        // we know we are 7-bit
        for (i=0; i<ARRAYSIZE(ch) && lpURL[i] != L':' && lpURL[i]; i++)
            ch[i] = (char) (lpURL[i]);

        if (i<ARRAYSIZE(ch))
        {
            ch[i] = '\0';

            // We have protocol
            if (NO_ERROR == SHGetValue(hk, ch, TEXT("SupportsNoUI"), NULL, &dwData, &cbData))
            {
                if (dwData != 0)
                    fValidProtocol = TRUE;  // Passed test
            }
        }

        RegCloseKey(hk);
    }
#endif

    // See if this protocol will give us something for the cache
    BOOL fUsesCache=FALSE;
    DWORD dwBufSize=0;
    CoInternetQueryInfo(lpURL, QUERY_USES_CACHE, 0,
        &fUsesCache, sizeof(fUsesCache), &dwBufSize, 0);

    if (!fUsesCache || (S_FALSE == ::IsValidURL(NULL, lpURL, 0)))
        return FALSE;

    return TRUE;
}

HRESULT CUrlDownload::GetRealURL(LPWSTR *ppwszURL)
{
    *ppwszURL = NULL;

    if (!m_fBrowserValid)
    {
        if (m_pwszURL)
            *ppwszURL = StrDupW(m_pwszURL);
    }
    else
    {
        // Get the real URL from the browser in case we were redirected
        // We could optimize to do this only once
        ITargetContainer *pTarget=NULL;
        LPWSTR pwszThisUrl=NULL;

        if (m_pDocument)
        {
            m_pDocument->QueryInterface(IID_ITargetContainer, (void **)&pTarget);

            if (pTarget)
            {
                pTarget->GetFrameUrl(&pwszThisUrl);
                pTarget->Release();
            }
        }

        if (pwszThisUrl)
        {
            if (m_pwszURL) SAFELOCALFREE(m_pwszURL);
            m_pwszURL = StrDupW(pwszThisUrl);
            *ppwszURL = StrDupW(pwszThisUrl);
            CoTaskMemFree(pwszThisUrl);
        }
        else if (m_pwszURL)
        {
            *ppwszURL = StrDupW(m_pwszURL);
        }
    }

    return (*ppwszURL) ? S_OK : E_OUTOFMEMORY;
}


HRESULT CUrlDownload::GetDocument(IHTMLDocument2 **ppDoc)
{
    HRESULT hr;

    if (!m_fBrowserValid)
    {
        *ppDoc = NULL;
        return (E_FAIL);
    }

    *ppDoc = m_pDocument;
    if (m_pDocument)
    {
        m_pDocument->AddRef();
        hr = S_OK;
    }
    else
        hr = E_NOINTERFACE;

    return (hr);
}

    


//
// IUnknown of CUrlDownload
//
STDMETHODIMP CUrlDownload::QueryInterface(REFIID riid, void ** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv=NULL;

    // Validate requested interface
    if (IID_IOleClientSite == riid)
        *ppv=(IOleClientSite *)this;
    else if (IID_IPropertyNotifySink == riid)
        *ppv=(IPropertyNotifySink *)this;
    else if (IID_IOleCommandTarget == riid)
        *ppv=(IOleCommandTarget *)this;
    else if (IID_IDispatch == riid)
        *ppv=(IDispatch *)this;
    else if (IID_IServiceProvider == riid)
        *ppv = (IServiceProvider *)this;
    else if (IID_IAuthenticate == riid)
        *ppv = (IAuthenticate *)this;
    else if (IID_IInternetSecurityManager == riid)
        *ppv = (IInternetSecurityManager *)this;
    else if ((IID_IUnknown == riid) ||
             (IID_IHlinkFrame == riid))
        *ppv = (IHlinkFrame *)this;
    else
    {
        // DBGIID("CUrlDownload::QueryInterface() failing", riid);
    }

    // Addref through the interface
    if (NULL != *ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CUrlDownload::AddRef(void)
{
//  TraceMsg(TF_THISMODULE, "CUrlDownload addref to %d", m_cRef+1);
    return ++m_cRef;
}


STDMETHODIMP_(ULONG) CUrlDownload::Release(void)
{
//  TraceMsg(TF_THISMODULE, "CUrlDownload release - %d", m_cRef-1);
    if( 0L != --m_cRef )
        return m_cRef;

    delete this;
    return 0L;
}

STDMETHODIMP CUrlDownload::GetTypeInfoCount(UINT *pctinfo)
{
    return E_NOTIMPL;
}

STDMETHODIMP CUrlDownload::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{
    return E_NOTIMPL;
}

STDMETHODIMP CUrlDownload::GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
{
    return E_NOTIMPL;
}

STDMETHODIMP CUrlDownload::Invoke(DISPID dispidMember, 
                                  REFIID riid, 
                                  LCID lcid, 
                                  WORD wFlags,
                                  DISPPARAMS *pdispparams, 
                                  VARIANT *pvarResult,
                                  EXCEPINFO *pexcepinfo, 
                                  UINT *puArgErr)
{
    if (!pvarResult)
        return E_INVALIDARG;

    ASSERT(V_VT(pvarResult)== VT_EMPTY);

    if (wFlags == DISPATCH_PROPERTYGET)
    {
        switch (dispidMember)
        {
        case DISPID_AMBIENT_DLCONTROL :
            //TraceMsg(TF_THISMODULE, "Returning DLCONTROL ambient property 0x%08x", m_lBindFlags);
            pvarResult->vt = VT_I4;
            pvarResult->lVal = m_lBindFlags;
            break;

        case DISPID_AMBIENT_USERAGENT:
            CHAR    szUserAgent[MAX_PATH];  // URLMON says the max length of the UA string is MAX_PATH
            DWORD   dwSize;

            dwSize = MAX_PATH;
            szUserAgent[0] = '\0';

            pvarResult->vt = VT_BSTR;

            if ( ObtainUserAgentString( 0, szUserAgent, &dwSize ) == S_OK )
            {
                UINT cch = lstrlenA( szUserAgent );

                // Allocates size + 1
                pvarResult->bstrVal = SysAllocStringLen( 0, cch );
                if( pvarResult->bstrVal )
                {
                    if( !MultiByteToWideChar( CP_ACP, 0, szUserAgent, -1, pvarResult->bstrVal, cch + 1 ) )
                    {
                        SysFreeString( pvarResult->bstrVal );
                        pvarResult->bstrVal = 0;
                    }
                }
            }
            break;

        case DISPID_AMBIENT_USERMODE:
            pvarResult->vt = VT_BOOL;
            pvarResult->boolVal = VARIANT_FALSE; // put it in design mode
            break;

        default:
            return DISP_E_MEMBERNOTFOUND;
        }
        return S_OK;
    }

    return DISP_E_MEMBERNOTFOUND;
}

// IPropertyNotifySink

STDMETHODIMP CUrlDownload::OnChanged(DISPID dispID)
{
    if ((DISPID_READYSTATE == dispID) ||
        (DISPID_UNKNOWN == dispID))
    {
        // Find out if we're done
        if (m_fWaitingForReadyState)
        {
            VARIANT     varState;
            DISPPARAMS  dp;

            VariantInit(&varState);

            if (SUCCEEDED(m_pDocument->Invoke(DISPID_READYSTATE, 
                                              IID_NULL, 
                                              GetUserDefaultLCID(), 
                                              DISPATCH_PROPERTYGET, 
                                              &dp, 
                                              &varState, NULL, NULL)) &&
                V_VT(&varState)==VT_I4 && 
                V_I4(&varState)== READYSTATE_COMPLETE)
            {
                m_fWaitingForReadyState = FALSE;
                // Successful download. See if a client-pull is waiting.
                if (m_pwszClientPullURL)
                    PostMessage(m_hwndMe, WM_URLDL_CLIENTPULL, 0, 0);
                else
                    OnDownloadComplete(BDU2_ERROR_NONE);
            }
        }
    }

    return S_OK;
}

STDMETHODIMP CUrlDownload::OnRequestEdit(DISPID dispID)
{
    return S_OK;
}

// IOleCommandTarget
STDMETHODIMP CUrlDownload::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds,
                                    OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    return OLECMDERR_E_UNKNOWNGROUP;
}

STDMETHODIMP CUrlDownload::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
                                DWORD nCmdexecopt, VARIANTARG *pvarargIn,
                                VARIANTARG *pvarargOut)
{
    HRESULT hres = OLECMDERR_E_NOTSUPPORTED;

    if (pguidCmdGroup == NULL) 
    {
        switch(nCmdID) 
        {
        case OLECMDID_SETPROGRESSPOS:
        {
            hres = S_OK;
            VARIANT     varBytes;
            
            if (m_pOleCmdTarget)
            {
                varBytes.vt=VT_EMPTY;
                m_pOleCmdTarget->Exec(&CGID_MSHTML, IDM_GETBYTESDOWNLOADED, 0, NULL, &varBytes);

                if (varBytes.vt == VT_I4)
                {
                    DWORD dwBytes = (DWORD) varBytes.lVal;

                    //TraceMsg(TF_THISMODULE, "%d bytes on page so far (mshtml)", dwBytes);

                    ProgressBytes(dwBytes);

                    // for this mutant version, we also want to keep mom and dad up to date.
                    LONG lPos;

                    // we use 0..50 so that the progress meter won't max out
                    // when only the download phase is finished and we still have
                    // packaging work to do.
                    if (pvarargIn && m_dwProgMax)
                        lPos = (pvarargIn->lVal * 25) / m_dwProgMax;
                    else
                        lPos = 0;

                    if (m_ptp)
                        m_ptp->SetPercent( lPos );
                    hres = S_OK;
                }
            }
        }
            break;

        case OLECMDID_SETPROGRESSMAX:
        {
            if (pvarargIn && pvarargIn->vt == VT_I4)
                m_dwProgMax = pvarargIn->lVal;
            hres = S_OK;
        }
            break;

        //
        // The containee has found an http-equiv meta tag; handle it
        // appropriately (client pull)
        //
        case OLECMDID_HTTPEQUIV_DONE:
            hres = S_OK;
            break;

        case OLECMDID_HTTPEQUIV:
            {
                LPWSTR  pwszEquivString = pvarargIn? pvarargIn->bstrVal : NULL;
                BOOL    fHasHeader = (bool) pwszEquivString;

                if (pvarargIn && pvarargIn->vt != VT_BSTR)
                    return OLECMDERR_E_NOTSUPPORTED;

                if (!fHasHeader || StrCmpNIW(c_wszRefresh, pwszEquivString, lstrlenW(c_wszRefresh)) == 0)
                {
                    // Hit.  Now do the right thing for this header
                    // We pass both the header and a pointer to the first char after
                    // ':', which is usually the delimiter handlers will look for.

                    LPWSTR pwszColon = fHasHeader ? StrChrW(pwszEquivString, ':') : NULL;
      
                    // Enforce the : at the end of the header
                    if (fHasHeader && !pwszColon)
                    {
                        return OLECMDERR_E_NOTSUPPORTED;
                    }
             
                    hres = HandleRefresh(pwszEquivString, pwszColon ? pwszColon+1:NULL,
                                         (nCmdID == OLECMDID_HTTPEQUIV_DONE));
                }
            }

            // if we return OLECMDERR_E_NOTSUPPORTED, we don't handle
            // client pull
            break;
        }
    }

    return hres;
}

// The basic operation was lifted from shdocvw\basesb.cpp
HRESULT CUrlDownload::HandleRefresh(LPWSTR pwszEquivString, LPWSTR pwszContent, BOOL fDone)
{
    unsigned int uiTimeout = 0;
    WCHAR        awch[INTERNET_MAX_URL_LENGTH];

    if (fDone)
    {
        return S_OK;    // fDone means we don't process this
    }

    // NSCompat: we only honor the first successfully parsed Refresh
    if (m_pwszClientPullURL)
        return S_OK;

    if (!pwszContent ||
        !DLParseRefreshContent(pwszContent, &uiTimeout, awch, INTERNET_MAX_URL_LENGTH))
    {
        return OLECMDERR_E_NOTSUPPORTED;   // cannot handle refresh w/o timeout
    }
    
    if (!awch[0])
    {
        return S_OK;
    }

    if (m_iNumClientPull >= MAX_CLIENT_PULL_NUM)
    {
        return S_OK;
    }

    //TraceMsg(TF_THISMODULE, "CUrlDownload client pull (refresh=%d) url=%ws", uiTimeout, awch);
    if (uiTimeout > MAX_CLIENT_PULL_TIMEOUT)
    {
        return S_OK;
    }

    m_pwszClientPullURL = StrDupW(awch);

    // If we can't copy the URL, don't set the timer or else we'll
    // keep reloading the same page.

    if (m_pwszClientPullURL == NULL)
        return OLECMDERR_E_NOTSUPPORTED;

    return S_OK;
}



HRESULT CUrlDownload::SetDLCTL(long lFlags)
{
//  TraceMsg(TF_THISMODULE, "CUrlDownload: SetDLCTL %04x", lFlags);
    m_lBindFlags = lFlags | DLCTL_SILENT;
    if (m_fSetResync)
        m_lBindFlags |= DLCTL_RESYNCHRONIZE;

    return S_OK;
}

#define INET_E_AGENT_BIND_IN_PROGRESS 0x800C0FFF

HRESULT CUrlDownload::ProgressBytes(DWORD dwBytes)
{
    if (m_dwMaxSize > 0 && dwBytes > m_dwMaxSize)
    {
        //TraceMsg(TF_THISMODULE, "CUrlDownload MaxSize exceeded aborting. %d of %d bytes", dwBytes, m_dwMaxSize);

        AbortDownload(BDU2_ERROR_MAXSIZE);
        return E_ABORT;
    }

    return S_OK;
}

//---------------------------------------------------------------
// IServiceProvider
STDMETHODIMP CUrlDownload::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    if ((SID_SHlinkFrame == guidService && IID_IHlinkFrame == riid) ||
        (IID_IAuthenticate == guidService && IID_IAuthenticate == riid) ||
        (SID_SInternetSecurityManager == guidService && IID_IInternetSecurityManager == riid))
    {
        return QueryInterface(riid, ppvObject);
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
}

//---------------------------------------------------------------
// IAuthenticate
STDMETHODIMP CUrlDownload::Authenticate(HWND *phwnd, LPWSTR *ppszUsername, LPWSTR *ppszPassword)
{
    HRESULT hr;
    ASSERT(phwnd && ppszUsername && ppszPassword);
    
    *phwnd = (HWND)-1;
    *ppszUsername = NULL;
    *ppszPassword = NULL;

    hr = E_NOTIMPL;

    //TraceMsg(TF_THISMODULE, "CUrlDownload::Authenticate returning hr=%08x", hr);

    return (hr);
}

//---------------------------------------------------------------
// IHlinkFrame
STDMETHODIMP CUrlDownload::SetBrowseContext(IHlinkBrowseContext *pihlbc)
{
    return E_NOTIMPL;
}
STDMETHODIMP CUrlDownload::GetBrowseContext(IHlinkBrowseContext **ppihlbc)
{
    return E_NOTIMPL;
}
STDMETHODIMP CUrlDownload::Navigate(DWORD grfHLNF, LPBC pbc, IBindStatusCallback *pibsc, IHlink *pihlNavigate)
{
    // We should only get a call through IHlinkFrame->Navigate()
    // when the webcrawler has submitted a form for authentication.
    // Bail out if that's not the case.
    if (!m_fFormSubmitted)
    {
        return E_NOTIMPL;
    }

    // Our timer has already been started. If this fails, OnDownloadComplete will get
    //  called when we time out.

    // We don't support a wide variety of parameters.
    ASSERT(grfHLNF == 0);
    ASSERT(pbc);
    ASSERT(pibsc);
    ASSERT(pihlNavigate);

    // Get the moniker from IHlink
    HRESULT hr;
    IMoniker *pmk = NULL;
    hr = pihlNavigate->GetMonikerReference(HLINKGETREF_ABSOLUTE, &pmk, NULL);
    if (SUCCEEDED(hr))
    {
        // Load the URL with the post data.
        // BUGBUG: What if we get redirected to something other than HTML? (beta 2)
        hr = m_pPersistMk->Load(FALSE, pmk, pbc, 0);
        SAFERELEASE(pmk);
        if (SUCCEEDED(hr))
        {
            m_fBrowserValid = TRUE;
            // Need to wait again.
            m_fWaitingForReadyState = TRUE;
        }
    }
    return (hr);
}
STDMETHODIMP CUrlDownload::OnNavigate(DWORD grfHLNF, IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName, DWORD dwreserved)
{
    return E_NOTIMPL;
}
STDMETHODIMP CUrlDownload::UpdateHlink(ULONG uHLID, IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName)
{
    return E_NOTIMPL;
}

//---------------------------------------------------------------------
// IInternetSecurityManager interface
// Used to override security to allow form submits, for form auth sites
HRESULT CUrlDownload::SetSecuritySite(IInternetSecurityMgrSite *pSite)
{
    return E_NOTIMPL;
}

HRESULT CUrlDownload::GetSecuritySite(IInternetSecurityMgrSite **ppSite)
{
    return E_NOTIMPL;
}

HRESULT CUrlDownload::MapUrlToZone(LPCWSTR pwszUrl, DWORD *pdwZone, DWORD dwFlags)
{
    return INET_E_DEFAULT_ACTION;
}

HRESULT CUrlDownload::GetSecurityId(LPCWSTR pwszUrl, BYTE *pbSecurityId, DWORD *pcbSecurityId, DWORD_PTR dwReserved)
{
    return INET_E_DEFAULT_ACTION;
}

HRESULT CUrlDownload::ProcessUrlAction(LPCWSTR pwszUrl, DWORD dwAction, BYTE __RPC_FAR *pPolicy, DWORD cbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved)
{
    if ((dwAction == URLACTION_HTML_SUBMIT_FORMS_TO) ||
        (dwAction == URLACTION_HTML_SUBMIT_FORMS_FROM))
    {
        return S_OK;
    }
    
    return INET_E_DEFAULT_ACTION;
}

HRESULT CUrlDownload::QueryCustomPolicy(LPCWSTR pwszUrl, REFGUID guidKey, BYTE **ppPolicy, DWORD *pcbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwReserved)
{
    return INET_E_DEFAULT_ACTION;
}

HRESULT CUrlDownload::SetZoneMapping(DWORD dwZone, LPCWSTR lpszPattern, DWORD dwFlags)
{
    return INET_E_DEFAULT_ACTION;
}

HRESULT CUrlDownload::GetZoneMappings(DWORD dwZone, IEnumString **ppenumString, DWORD dwFlags)
{
    return INET_E_DEFAULT_ACTION;
}

//
// IOleClientSite
//
STDMETHODIMP CUrlDownload:: SaveObject(void)
{
    return E_NOTIMPL;
}

STDMETHODIMP CUrlDownload:: GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
    return E_NOTIMPL;
}

STDMETHODIMP CUrlDownload:: GetContainer(IOleContainer **ppContainer)
{
    return E_NOTIMPL;
}

STDMETHODIMP CUrlDownload:: ShowObject(void)
{
    return E_NOTIMPL;
}

STDMETHODIMP CUrlDownload:: OnShowWindow(BOOL fShow)
{
    return E_NOTIMPL;
}

STDMETHODIMP CUrlDownload:: RequestNewObjectLayout(void)
{
    return E_NOTIMPL;
}



// ParseRefreshContent was lifted in its entirety from shdocvw\basesb.cpp
BOOL DLParseRefreshContent(LPWSTR pwzContent,
    UINT * puiDelay, LPWSTR pwzUrlBuf, UINT cchUrlBuf)
{
    // We are parsing the following string:
    //
    //  [ws]* [0-9]+ [ws]* ; [ws]* url [ws]* = [ws]* { ' | " } [any]* { ' | " }
    //
    // Netscape insists that the string begins with a delay.  If not, it
    // ignores the entire directive.  There can be more than one URL mentioned,
    // and the last one wins.  An empty URL is treated the same as not having
    // a URL at all.  An empty URL which follows a non-empty URL resets
    // the previous URL.

    enum { PRC_START, PRC_DIG, PRC_DIG_WS, PRC_SEMI, PRC_SEMI_URL,
        PRC_SEMI_URL_EQL, PRC_SEMI_URL_EQL_ANY };
    #define ISSPACE(ch) (((ch) == 32) || ((unsigned)((ch) - 9)) <= 13 - 9)

    UINT uiState = PRC_START;
    UINT uiDelay = 0;
    LPWSTR pwz = pwzContent;
    LPWSTR pwzUrl = NULL;
    UINT   cchUrl = 0;
    WCHAR  wch,  wchDel = 0;

    *pwzUrlBuf = 0;

    do
    {
        wch = *pwz;

        switch (uiState)
        {
            case PRC_START:
                if (wch >= TEXT('0') && wch <= TEXT('9'))
                {
                    uiState = PRC_DIG;
                    uiDelay = wch - TEXT('0');
                }
                else if (!ISSPACE(wch))
                    goto done;
                break;

            case PRC_DIG:
                if (wch >= TEXT('0') && wch <= TEXT('9'))
                    uiDelay = uiDelay * 10 + wch - TEXT('0');
                else if (ISSPACE(wch))
                    uiState = PRC_DIG_WS;
                else if (wch == TEXT(';'))
                    uiState = PRC_SEMI;
                else
                    goto done;
                break;

            case PRC_DIG_WS:
                if (wch == TEXT(';'))
                    uiState = PRC_SEMI;
                else if (!ISSPACE(wch))
                    goto done;
                break;

            case PRC_SEMI:
                if (    (wch == TEXT('u') || wch == TEXT('U'))
                    &&  (pwz[1] == TEXT('r') || pwz[1] == TEXT('R'))
                    &&  (pwz[2] == TEXT('l') || pwz[2] == TEXT('L')))
                {
                    uiState = PRC_SEMI_URL;
                    pwz += 2;
                }
                else if (!ISSPACE(wch) && wch != TEXT(';'))
                    goto done;
                break;

            case PRC_SEMI_URL:
                if (wch == TEXT('='))
                {
                    uiState = PRC_SEMI_URL_EQL;
                    *pwzUrlBuf = 0;
                }
                else if (wch == TEXT(';'))
                    uiState = PRC_SEMI;
                else if (!ISSPACE(wch))
                    goto done;
                break;

            case PRC_SEMI_URL_EQL:
                if (wch == TEXT(';'))
                    uiState = PRC_SEMI;
                else if (!ISSPACE(wch))
                {
                    uiState = PRC_SEMI_URL_EQL_ANY;

                    pwzUrl = pwzUrlBuf;
                    cchUrl = cchUrlBuf;

                    if (wch == TEXT('\'')|| wch == TEXT('\"'))
                        wchDel = wch;
                    else
                    {
                        wchDel = 0;
                        *pwzUrl++ = wch;
                        cchUrl--;
                    }
                }
                break;
                        
            case PRC_SEMI_URL_EQL_ANY:
                if (    !wch
                    ||  ( wchDel && wch == wchDel)
                    ||  (!wchDel && wch == L';'))
                {
                    *pwzUrl = 0;
                    uiState = wch == TEXT(';') ? PRC_SEMI : PRC_DIG_WS;
                }
                else if (cchUrl > 1)
                {
                    *pwzUrl++ = wch;
                    cchUrl--;
                }
                break;
        }

        ++pwz;

    } while (wch);

done:

    *puiDelay = uiDelay;

    return(uiState >= PRC_DIG);
} // ParseRefreshContent
