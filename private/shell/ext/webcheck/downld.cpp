#include "private.h"
#include <exdisp.h>
#include <exdispid.h>
#include <htiface.h>
#include <mshtmdid.h>
#include <mshtmcid.h>
#include <mshtmhst.h>
#include <optary.h>                 // needed for IHtmlLoadOptions

#include "downld.h"

#define TF_THISMODULE   TF_DOWNLD

// CUrlDownload is a single threaded object. We can assume we are always on a single thread.

long g_lRegisteredWnd = 0;
LRESULT UrlDownloadWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

CLIPFORMAT g_cfHTML=CF_NULL;

// User-Agent strings
const WCHAR c_wszUserAgentAppend[] = L"; MSIECrawler)";

// Refresh header for http-equiv (client-pull)
const WCHAR c_wszRefresh[] = L"Refresh";

const int  MAX_CLIENT_PULL_NUM = 4;     // max # redirections
const int  MAX_CLIENT_PULL_TIMEOUT = 6; // max timeout we'll follow

// Function also present in shdocvw\basesb.cpp and in mshtml
BOOL ParseRefreshContent(LPWSTR pwzContent, UINT * puiDelay, LPWSTR pwzUrlBuf, UINT cchUrlBuf);

const WCHAR c_wszHeadVerb[] = L"HEAD";

const WCHAR c_szUserAgentPrefix[] = L"User-Agent: ";
const WCHAR c_szAcceptLanguagePrefix[] = L"Accept-Language: ";

#define WM_URLDL_CLEAN      (WM_USER + 0x1010)
#define WM_URLDL_ONDLCOMPLETE (WM_USER + 0x1012)
#define WM_URLDL_CLIENTPULL (WM_USER+0x1013)

#define SAFE_RELEASE_BSC() \
if (m_pCbsc) { \
m_pCbsc->SetParent(NULL); \
m_pCbsc->Release(); \
m_pCbsc = NULL; \
} else

//---------------------------------------------------------------
// CUrlDownload class
CUrlDownload::CUrlDownload(CUrlDownloadSink *pParent, UINT iID /* =0 */)
{
    DWORD cbData;

    // Maintain global count of objects
    DllAddRef();

    m_iID = iID;
    m_pParent = pParent;

    m_cRef = 1;
    
    ASSERT(m_pDocument==NULL && m_dwConnectionCookie==0 && m_pwszURL == NULL);

    // Get the timeout value (stored in seconds)
    cbData = sizeof(m_nTimeout);
    if (NO_ERROR != SHGetValue(HKEY_CURRENT_USER, c_szRegKey, TEXT("Timeout"), NULL, &m_nTimeout, &cbData))
    {
        // Default to 120 seconds
        m_nTimeout = 120;
    }

    // find the HTML clipboard format
    if (!g_cfHTML)
    {
        g_cfHTML = (CLIPFORMAT) RegisterClipboardFormat(CFSTR_MIME_HTML);
        TraceMsg(TF_THISMODULE, "ClipFormat for HTML = %d", (int)g_cfHTML);
    }

    // find out if we need to set the "RESYNCHRONIZE" flag
    INTERNET_CACHE_CONFIG_INFOA CacheConfigInfo;
    DWORD dwBufSize = sizeof(CacheConfigInfo);
    CacheConfigInfo.dwStructSize = sizeof(CacheConfigInfo);

    if (GetUrlCacheConfigInfoA(&CacheConfigInfo, &dwBufSize, CACHE_CONFIG_SYNC_MODE_FC))
    {
        if ((WININET_SYNC_MODE_ONCE_PER_SESSION == CacheConfigInfo.dwSyncMode) ||
             (WININET_SYNC_MODE_ALWAYS == CacheConfigInfo.dwSyncMode) ||
             (WININET_SYNC_MODE_AUTOMATIC == CacheConfigInfo.dwSyncMode))
        {
            m_fSetResync = FALSE;
        }
        else
        {
            m_fSetResync = TRUE;
            DBG("Browser session update='never', setting RESYNCHRONIZE");
        }
    }
    else
        DBG_WARN("GetUrlCacheConfigInfo failed! Not setting Resync.");

    m_lBindFlags = DLCTL_SILENT | DLCTL_NO_SCRIPTS | 
        DLCTL_NO_JAVA | DLCTL_NO_RUNACTIVEXCTLS | DLCTL_NO_DLACTIVEXCTLS;
    if (m_fSetResync)
        m_lBindFlags |= DLCTL_RESYNCHRONIZE;

    // register our window class if necessary
    if (!g_lRegisteredWnd)
    {
        g_lRegisteredWnd++;

        WNDCLASS wc;

        wc.style = 0;
        wc.lpfnWndProc = UrlDownloadWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = g_hInst;
        wc.hIcon = NULL;
        wc.hCursor = NULL;
        wc.hbrBackground = (HBRUSH)NULL;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = URLDL_WNDCLASS;

        RegisterClass(&wc);
    }
}

CUrlDownload::~CUrlDownload()
{
    // Maintain global count of objects
    DllRelease();

    CleanUp();
    DBG("Destroyed CUrlDownload object");
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
    SAFE_RELEASE_BSC();
    SAFELOCALFREE(m_pwszURL);
    SAFELOCALFREE(m_pstLastModified);
    SAFERELEASE(m_pStm);
    SAFELOCALFREE(m_pwszUserAgent);

    if (m_hwndMe)
    {
        SetWindowLongPtr(m_hwndMe, GWLP_USERDATA, 0);
        DestroyWindow(m_hwndMe);
        m_hwndMe = NULL;
    }
}

LRESULT UrlDownloadWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    CUrlDownload *pThis = (CUrlDownload*) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    // Validate pThis
#ifdef DEBUG
    if (pThis && IsBadWritePtr(pThis, sizeof(*pThis)))
    {
        TraceMsg(TF_THISMODULE,
            "Invalid 'this' in UrlDownloadWndProc (0x%08x) - already destroyed?", pThis);
    }
#endif

    switch (Msg)
    {
    case WM_CREATE :
        {
            LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;

            if (!pcs || !(pcs->lpCreateParams))
            {
                DBG_WARN("Invalid param UrlDownloadWndProc Create");
                return -1;
            }
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) pcs->lpCreateParams);
            return 0;
        }

    case WM_URLDL_CLIENTPULL :
    case WM_URLDL_ONDLCOMPLETE :
    case WM_TIMER :
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
        m_hwndMe = CreateWindow(URLDL_WNDCLASS, TEXT("YO"), WS_OVERLAPPED,
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    NULL, NULL, g_hInst, (LPVOID)this);

        if (NULL == m_hwndMe)
        {
            DBG_WARN("CUrlDownload CreateWindow(UrlDl WndClass) failed");
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
    ASSERT(!(iOptions & BDU2_NEEDSTREAM) || (iMethod == BDU2_URLMON));
    ASSERT(!pszLocalFile || (iMethod == BDU2_URLMON));

    if (pszLocalFile && iMethod != BDU2_URLMON)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CreateMyWindow();

        // Clean up some old stuff
        if (m_pCbsc)
        {
            if (m_fbscValid)
                m_pCbsc->Abort();
            SAFE_RELEASE_BSC();
        }
        SAFERELEASE(m_pScript);
        SAFERELEASE(m_pStm);

        m_fbscValid = m_fBrowserValid = FALSE;

        m_iMethod = iMethod;
        m_iOptions = iOptions;

        m_dwMaxSize = dwMaxSize;

        SAFELOCALFREE(m_pwszClientPullURL);
        m_iNumClientPull = 0;

        // Save URL
        SAFELOCALFREE(m_pwszURL);
        m_pwszURL = StrDupW(pwszURL);

        SAFELOCALFREE(m_pstLastModified);
        m_dwResponseCode = 0;

        if ((iOptions & BDU2_FAIL_IF_NOT_HTML) && IsNonHtmlUrl(pwszURL))
        {
            // Hey, this isn't an HTML url! Don't even try to download it.
            OnDownloadComplete(BDU2_ERROR_NOT_HTML);
        }
        else
        {
            // Determine how to download this URL
            if ((iMethod == BDU2_BROWSER) ||
                ((iMethod == BDU2_SMART) && IsHtmlUrl(pwszURL)))
            {
                hr = BeginDownloadWithBrowser(pwszURL);
            }
            else
            {
                hr = BeginDownloadWithUrlMon(pwszURL, pszLocalFile, NULL);
            }
        }
    }

    if (FAILED(hr))
    {
        DBG("BeginDownloadURL2 : error HRESULT - calling OnDownloadComplete w/Error");
        OnDownloadComplete(BDU2_ERROR_GENERAL);
    }

    return hr;
}

//
// Looks up the Url in the url history object and if its not CP_ACP
// inserts an IHTMLLoadOptions object that contains the codepage
// into the bind context
//
HRESULT InsertHistoricalCodepageIntoBindCtx(LPCWSTR pwszURL, IBindCtx * pbc)
{
    HRESULT hr = S_OK;

    if (pwszURL == NULL || pbc == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        //
        // Get the codepage from the intsite database. This is the codepage
        // the user set when last visiting this url.
        //
        PROPVARIANT propCodepage = {0};
        propCodepage.vt = VT_UI4;

        TCHAR szURL[INTERNET_MAX_URL_LENGTH];
        MyOleStrToStrN(szURL, INTERNET_MAX_URL_LENGTH, pwszURL);
        hr = IntSiteHelper(szURL, &c_rgPropRead[PROP_CODEPAGE], 
            &propCodepage, 1, FALSE);

        if (SUCCEEDED(hr) && propCodepage.lVal != CP_ACP)
        {
            //
            // We got a codepage that wasn't the ansi one create an
            // HTMLLoadOptions object and set the code page in it.
            //
            IHtmlLoadOptions *phlo = NULL;
            hr = CoCreateInstance(CLSID_HTMLLoadOptions, NULL, 
                CLSCTX_INPROC_SERVER, IID_IHtmlLoadOptions, (void**)&phlo);

            if (SUCCEEDED(hr) && phlo)
            {
                hr = phlo->SetOption(HTMLLOADOPTION_CODEPAGE, &propCodepage.lVal,
                    sizeof(propCodepage.lVal));

                if (SUCCEEDED(hr))
                {
                    //
                    // Insert the option into the bindctx
                    //
                    pbc->RegisterObjectParam(L"__HTMLLOADOPTIONS", phlo);
                    TraceMsg(TF_THISMODULE,
                        "InsertHistoricalCodepageIntoBindCtx codepage=%d",
                        propCodepage.lVal);
                }
                phlo->Release();
            }
        }
    }
    return hr;
}

LPCWSTR CUrlDownload::GetUserAgent()
{
    if (m_pwszUserAgent)
    {
        return m_pwszUserAgent;
    }

    // Get default User-Agent string from urlmon
    CHAR chUA[1024];
    DWORD dwBufLen;

    // Assume that UrlMkGetSessionOption always succeeds (82160).
    chUA[0] = 0;
    UrlMkGetSessionOption(URLMON_OPTION_USERAGENT, chUA, sizeof(chUA), &dwBufLen, 0);
    
    // Append "MSIECrawler"
    int iLenUA, iLenNew;

    iLenUA = lstrlenA(chUA);
    iLenNew = iLenUA + ARRAYSIZE(c_wszUserAgentAppend);

    ASSERT(iLenUA == (int)(dwBufLen-1));

    if (iLenUA > 0)
    {
        m_pwszUserAgent = (LPWSTR) LocalAlloc(LMEM_FIXED, sizeof(WCHAR)*iLenNew);

        if (m_pwszUserAgent)
        {
            LPWSTR pwszAppend = m_pwszUserAgent+iLenUA-1;
            m_pwszUserAgent[0] = L'\0';
            SHAnsiToUnicode(chUA, m_pwszUserAgent, iLenNew);
            // find the closing parenthesis and append string there
            if (*pwszAppend != L')')
            {
                DBG("GetUserAgent: Last Char in UA isn't closing paren");
                pwszAppend = StrRChrW(m_pwszUserAgent, m_pwszUserAgent+iLenUA, L')');
            }
            if (pwszAppend)
            {
                StrCpyW(pwszAppend, c_wszUserAgentAppend);
            }
            else
            {
                LocalFree(m_pwszUserAgent);
                m_pwszUserAgent = NULL;
            }
        }
    }

    return m_pwszUserAgent;
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
        if (FAILED(hr)) DBG_WARN("CreateURLMoniker failed");

        // create an empty bind context so that Urlmon will call Trident's
        //  QueryService on the proper thread so that Trident can delegate
        //  it to use properly.
        hr=CreateBindCtx(0, &pbc);
        if (FAILED(hr)) DBG_WARN("CreateBindCtx failed");

        if (SUCCEEDED(hr))
        {
            //
            // Looks up the Url in the url history object and if its not CP_ACP
            // inserts an IHTMLLoadOptions object that contains the codepage
            // into the bind context. This is done so that TRIDENT is seeded
            // with the correct codepage.
            //
            InsertHistoricalCodepageIntoBindCtx(pwszURL, pbc);

            hr = m_pPersistMk->Load(FALSE, pURLMoniker, pbc, 0);
            if (SUCCEEDED(hr)) m_fWaitingForReadyState = TRUE;
            if (FAILED(hr)) DBG_WARN("PersistMoniker::Load failed");
        }

        // clean up junk
        if (pURLMoniker)
            pURLMoniker->Release();

        if (pbc)
            pbc->Release();

        if (SUCCEEDED(hr))
        {
            m_fBrowserValid = TRUE;
            StartTimer();       // Start our timeout
        }
        else
        {
            DBG("Error binding with Browser's IPersistMoniker");
            CleanUpBrowser();
        }
    }

    TraceMsg(TF_THISMODULE,
        "CUrlDownload::BeginDownloadWithBrowser (hr=0x%08x)", (long)hr);

    return hr;
}

HRESULT CUrlDownload::OnDownloadComplete(int iError)
{
    PostMessage(m_hwndMe, WM_URLDL_ONDLCOMPLETE, (WPARAM)iError, 0);
    StopTimer();
    return S_OK;
}

BOOL CUrlDownload::HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_URLDL_CLIENTPULL :
        {
            HRESULT hr = E_FAIL;

            // Ask our parent if we should do this
            if (m_pwszClientPullURL)
            {
                if (m_pParent && (m_iNumClientPull < MAX_CLIENT_PULL_NUM))
                    hr = m_pParent->OnClientPull(m_iID, m_pwszURL, m_pwszClientPullURL);

                TraceMsgA(TF_THISMODULE, "CUrlDownload %s executing client pull to %ws",
                    SUCCEEDED(hr) ? "is" : "**not**", m_pwszClientPullURL);
            }

            if (SUCCEEDED(hr))
            {
                // Download this new url. Don't give "downloadcomplete" for first one
                // Save member vars since they get reset in BDU2
                int iNumClientPull = m_iNumClientPull;
                LPWSTR pszNewURL = m_pwszClientPullURL;

                m_pwszClientPullURL = NULL;
                hr = BeginDownloadURL2(pszNewURL, m_iMethod, m_iOptions, NULL, m_dwMaxSize);
                MemFree(pszNewURL);
                if (SUCCEEDED(hr))
                {
                    m_iNumClientPull = iNumClientPull + 1;
                }
            }
        }
        break;

    case WM_URLDL_ONDLCOMPLETE :
        if (m_pParent)
            m_pParent->OnDownloadComplete(m_iID, (int)wParam);
        return TRUE;

    case WM_TIMER :
#ifdef DEBUG
        DBG_WARN("CUrlDownload ERROR - TIMING OUT");
        ASSERT_MSG(!m_fBrowserValid, "MSHTML caused us to time out");
        ASSERT_MSG(!m_fbscValid, "UrlMon download timed out");
#endif
        StopTimer();
        AbortDownload(BDU2_ERROR_TIMEOUT);
        return TRUE;
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

    if (m_fbscValid)
    {
        ASSERT(m_pCbsc);
        if (m_pCbsc)
        {
            hr = m_pCbsc->Abort();
            fAborted=TRUE;
            SAFE_RELEASE_BSC();
        }
        m_fbscValid=FALSE;
    }

    if (fAborted && m_pParent)
    {
        OnDownloadComplete((iErrorCode==-1) ? BDU2_ERROR_ABORT : iErrorCode);
    }

    return hr;
}

// Loads browser, creates sink and hooks it up to sinks
HRESULT CUrlDownload::GetBrowser()
{
    HRESULT hr = S_OK;

    if (m_fAdviseOn)
        return hr;

    if (NULL == m_pDocument)
    {
        ASSERT(!m_pPersistMk);
        ASSERT(!m_pCP);

        hr = CoCreateInstance(CLSID_HTMLDocument, NULL,
                    CLSCTX_INPROC, IID_IHTMLDocument2, (void **)&m_pDocument);

        DBG("Created new CLSID_HTMLDocument");

        if (SUCCEEDED(hr))
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
    }

    // At this point we have m_pDocument and m_pPersistMk

    // Get DownloadNotify sink hooked up
    IDownloadNotify *pNotify=NULL;
    BOOL            fNotifySet=FALSE;

    if (SUCCEEDED(hr) && SUCCEEDED(m_pParent->GetDownloadNotify(&pNotify)) && pNotify)
    {
        IOleCommandTarget *pTarget=NULL;

        if (SUCCEEDED(m_pDocument->QueryInterface(IID_IOleCommandTarget, (void **)&pTarget)) && pTarget)
        {
            VARIANTARG varIn;

            varIn.vt = VT_UNKNOWN;
            varIn.punkVal = (IUnknown *)pNotify;
            if (SUCCEEDED(pTarget->Exec(&CGID_DownloadHost, DWNHCMDID_SETDOWNLOADNOTIFY, 0,
                                        &varIn, NULL)))
            {
                fNotifySet=TRUE;
            }

            pTarget->Release();
        }

        if (!fNotifySet)
        {
            DBG_WARN("IDownloadNotify provided, but couldn't set callback!");
        }

        pNotify->Release();
    }

    if (!fNotifySet && (m_iOptions & BDU2_DOWNLOADNOTIFY_REQUIRED))
    {
        DBG_WARN("Couldn't set notify, parent requires it. CUrlDownload failing MSHTML download.");
        hr = E_FAIL;
    }

    // Get PropertyNotifySink hooked up
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

    if (FAILED(hr)) DBG_WARN("CUrlDownload::GetBrowser returning failure");
    return hr;
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
    LeaveMeAlone();

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
        DBG("m_fBrowserValid FALSE, GetScript returning failure");
        return E_FAIL;
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
#ifdef DEBUG
        if (FAILED(hr)) DBG_WARN("CUrlDownload::GetScript:  get_Script failed");
#endif
    }

    if (SUCCEEDED(hr))
    {
        hr = pDisp->QueryInterface(IID_IHTMLWindow2, (void **)ppWin);
        if (*ppWin == NULL) hr = E_NOINTERFACE;
        pDisp->Release();
#ifdef DEBUG
        if (FAILED(hr)) DBG_WARN("CUrlDownload::GetScript:  QI IOmWindow2 failed");
#endif
    }

    // Save this so future GetScript() calls much faster
    ASSERT(!m_pScript);
    if (SUCCEEDED(hr))
    {
        m_pScript = *ppWin;
        m_pScript->AddRef();
    }

    return hr;
}

// static member function
// Strips off anchor from URL (# not after ?)
// S_FALSE : Unchanged
// S_OK    : Removed anchor
HRESULT CUrlDownload::StripAnchor(LPWSTR lpURL)
{
    if (!lpURL) return E_POINTER;

    while (*lpURL)
    {
        if (*lpURL == L'?')
            return S_FALSE;
        if (*lpURL == L'#')
        {
            *lpURL = L'\0';
            return S_OK;
        }
        lpURL ++;
    }
    return S_FALSE;
}

// Returns pointer to '.' or pointer to null-terminator or query '?'
LPWSTR                  // ptr to period or to null-term or '?'
URLFindExtensionW(
    LPCWSTR pszURL,
    int *piLen)         // length including period
{
    LPCWSTR pszDot;

    for (pszDot = NULL; *pszURL && *pszURL!='?'; pszURL++)
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
BOOL CUrlDownload::IsHtmlUrl(LPCWSTR lpURL)
{
    LPWSTR pwch;
    int iLen;

    pwch = URLFindExtensionW(lpURL, &iLen);

    if (*pwch && iLen)
    {
        pwch ++; iLen --;

        // We found an extension. Check it out.
        if ((iLen == 4 &&
                (!MyAsciiCmpNIW(pwch, L"html", 4))) ||
            (iLen == 3 &&
                (!MyAsciiCmpNIW(pwch, L"htm", 3) ||
                 !MyAsciiCmpNIW(pwch, L"htt", 3) ||
                 !MyAsciiCmpNIW(pwch, L"asp", 3) ||
                 !MyAsciiCmpNIW(pwch, L"htx", 3)
                                            )))
        {
            // known HTML extension
            return TRUE;
        }
    }

    return FALSE;
}

// Returns TRUE if this appears NOT to be an HTML URL
BOOL CUrlDownload::IsNonHtmlUrl(LPCWSTR lpURL)
{
    LPWSTR pwch;
    int iLen;

    pwch = URLFindExtensionW(lpURL, &iLen);

    if (*pwch && iLen)
    {
        pwch ++; iLen --;

        // We found an extension. Check it out.
        if ((iLen==3) &&
                (!MyAsciiCmpNIW(pwch, L"bmp", 3) ||
                 !MyAsciiCmpNIW(pwch, L"cab", 3) ||
                 !MyAsciiCmpNIW(pwch, L"cdf", 3) ||
                 !MyAsciiCmpNIW(pwch, L"jpg", 3) ||
                 !MyAsciiCmpNIW(pwch, L"exe", 3) ||
                 !MyAsciiCmpNIW(pwch, L"zip", 3) ||
                 !MyAsciiCmpNIW(pwch, L"doc", 3) ||
                 !MyAsciiCmpNIW(pwch, L"gif", 3)
                                            ))
        {
            // known non-HTML extension
            return TRUE;
        }
    }

    return FALSE;
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
#ifdef DEBUG
            if (!fValidProtocol)
                TraceMsgA(TF_THISMODULE, "IsValidUrl failing url protocol=%s url=%ws", ch, lpURL);
#endif
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
            if (m_pwszURL) MemFree(m_pwszURL);
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
        DBG("GetDocument failing, m_fBrowserValid FALSE");
        *ppDoc = NULL;
        return E_FAIL;
    }

    *ppDoc = m_pDocument;
    if (m_pDocument)
    {
        m_pDocument->AddRef();
        hr = S_OK;
    }
    else
        hr = E_NOINTERFACE;

    return hr;
}

HRESULT CUrlDownload::GetStream(IStream **ppStm)
{
    if (!m_pStm)
    {
        DBG("Stream not available, CUrlDownload::GetStream failing");
        *ppStm = NULL;
        return E_FAIL;
    }

    *ppStm = m_pStm;
    (*ppStm)->AddRef();

    return S_OK;
}

HRESULT CUrlDownload::GetLastModified(SYSTEMTIME *pstLastModified)
{
    if (NULL == pstLastModified)
        return E_INVALIDARG;

    if (NULL == m_pstLastModified)
        return E_FAIL;

    CopyMemory(pstLastModified, m_pstLastModified, sizeof(SYSTEMTIME));

    return S_OK;
}

HRESULT CUrlDownload::GetResponseCode(DWORD *pdwResponseCode)
{
    if (m_dwResponseCode == 0)
        return E_FAIL;

    *pdwResponseCode = m_dwResponseCode;

    return S_OK;
}
    
// Start or extend timer
void CUrlDownload::StartTimer()
{
    if (m_hwndMe)
    {
        if (!m_iTimerID)
        {
            m_iTimerID = 1;
            DBG("CUrlDownload Creating new timeout timer");
        }

        m_iTimerID = SetTimer(m_hwndMe, 1, 1000 * m_nTimeout, NULL);
    }
}

void CUrlDownload::StopTimer()
{
    if (m_hwndMe && m_iTimerID)
    {
        DBG("CUrlDownload destroying timeout timer");
        KillTimer(m_hwndMe, m_iTimerID);
        m_iTimerID = 0;
    }
}

//
// IUnknown of CUrlDownload
//
STDMETHODIMP CUrlDownload::QueryInterface(REFIID riid, void ** ppv)
{
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
    return ++m_cRef;
}


STDMETHODIMP_(ULONG) CUrlDownload::Release(void)
{
    if (0L != --m_cRef)
        return 1L;

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

STDMETHODIMP CUrlDownload::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
            DISPPARAMS *pdispparams, VARIANT *pvarResult,
            EXCEPINFO *pexcepinfo, UINT *puArgErr)
{
    if (!pvarResult)
        return E_INVALIDARG;

    ASSERT(pvarResult->vt == VT_EMPTY);

    if (wFlags == DISPATCH_PROPERTYGET)
    {
        HRESULT hr = DISP_E_MEMBERNOTFOUND;
        
        switch (dispidMember)
        {
        case DISPID_AMBIENT_DLCONTROL :
            TraceMsg(TF_THISMODULE, "Returning DLCONTROL ambient property 0x%08x", m_lBindFlags);
            pvarResult->vt = VT_I4;
            pvarResult->lVal = m_lBindFlags;
            hr = S_OK;
            break;
        case DISPID_AMBIENT_USERAGENT:
            DBG("Returning User Agent ambient property");
            pvarResult->bstrVal = SysAllocString(GetUserAgent());
            if (pvarResult->bstrVal != NULL)
            {
                pvarResult->vt = VT_BSTR;
                hr = S_OK;
            }
            break;
        }
        return hr;
    }

    return DISP_E_MEMBERNOTFOUND;
}

// IPropertyNotifySink

STDMETHODIMP CUrlDownload::OnChanged(DISPID dispID)
{
    // We've received a notification, extend our timer if it's currently running
    if (m_iTimerID)
        StartTimer();

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

                    TraceMsg(TF_THISMODULE, "%d bytes on page so far (mshtml)", dwBytes);

                    ProgressBytes(dwBytes);
                }
            }

            // 14032: If dialmon is around, tell it that something is going on
            IndicateDialmonActivity();

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
                BOOL    fHasHeader = (pwszEquivString!=NULL);

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

    if ((hres == OLECMDERR_E_NOTSUPPORTED) && m_pParent)
    {
        hres = m_pParent->OnOleCommandTargetExec(pguidCmdGroup, nCmdID, nCmdexecopt,
                                                    pvarargIn, pvarargOut);
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
        !ParseRefreshContent(pwszContent, &uiTimeout, awch, INTERNET_MAX_URL_LENGTH))
    {
        return OLECMDERR_E_NOTSUPPORTED;   // cannot handle refresh w/o timeout
    }
    
    if (!awch[0])
    {
        DBG("CUrlDownload ignoring client-pull directive with no url");
        return S_OK;
    }

    if (m_iNumClientPull >= MAX_CLIENT_PULL_NUM)
    {
        DBG("Max # client pulls exceeded; ignoring client pull directive");
        return S_OK;
    }

    TraceMsg(TF_THISMODULE, "CUrlDownload client pull (refresh=%d) url=%ws", uiTimeout, awch);
    if (uiTimeout > MAX_CLIENT_PULL_TIMEOUT)
    {
        DBG("Ignoring client-pull directive with large timeout");
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

//==============================================================================
//  UrlMon download code
//==============================================================================
HRESULT CUrlDownload::BeginDownloadWithUrlMon(
    LPCWSTR     pwszURL,
    LPTSTR      pszLocalFile,
    IEnumFORMATETC *pEFE)
{
    IStream*    pstm = NULL;
    IMoniker*   pmk = NULL;
    IBindCtx*   pbc = NULL;
    HRESULT hr;

    hr = CreateURLMoniker(NULL, pwszURL, &pmk);
    if (FAILED(hr))
    {
        DBG_WARN("CreateURLMoniker failed");
        goto LErrExit;
    }

    SAFE_RELEASE_BSC();

    m_pCbsc = new CUrlDownload_BSC(m_iMethod, m_iOptions, pszLocalFile);
    if (m_pCbsc == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto LErrExit;
    }

    hr = CreateBindCtx(0, &pbc);
    if (FAILED(hr))
        goto LErrExit;

    if (pEFE)
    {
        hr = RegisterFormatEnumerator(pbc, pEFE, 0);
        if (FAILED(hr))
            DBG_WARN("RegisterFormatEnumerator failed (continuing download)");
    }

    hr = RegisterBindStatusCallback(pbc,
            (IBindStatusCallback *)m_pCbsc,
            0,
            0L);
    if (FAILED(hr))
        goto LErrExit;

    m_pCbsc->SetParent(this);
    m_fbscValid = TRUE;
    m_hrStatus = INET_E_AGENT_BIND_IN_PROGRESS;
    StartTimer();       // Start our timeout
    hr = pmk->BindToStorage(pbc, 0, IID_IStream, (void**)&pstm);

    if (m_hrStatus != INET_E_AGENT_BIND_IN_PROGRESS)
    {
        // Synchronous success or failure. Call OnDownloadComplete.
        // We can't do it in OnStopBinding because Urlmon returns hrStatus=S_OK...
        //   even if it fails.
        if (FAILED(hr) || FAILED(m_hrStatus))
            OnDownloadComplete(BDU2_ERROR_GENERAL);
        else
            OnDownloadComplete(BDU2_ERROR_NONE);

        DBG("Synchronous bind; OnDownloadComplete called");
    }

    m_hrStatus = S_OK;      // need this so we get OnDownloadComplete (asynch OnStopBinding)
    hr = S_OK;              // need this so we don't get extra OnDownloadComplete (BDU2)

    // Bind has started (and maybe completed), release stuff we don't need
    pmk->Release();
    pbc->Release();

    if (pstm)
        pstm->Release();

    return hr;

LErrExit:
    DBG_WARN("Error in CUrlDownload::BeginDownloadWithUrlMon");
    if (pbc) pbc->Release();
    if (pmk) pmk->Release();
    if (pstm) pstm->Release();
    SAFERELEASE(m_pCbsc);

    return hr;
} // CUrlDownload::BeginDownloadWithUrlMon

void CUrlDownload::BSC_OnStartBinding()
{
    DBG("BSC_OnStartBinding");
}

// We only get this call if we're not downloading with the browser.
void CUrlDownload::BSC_OnStopBinding(HRESULT hrStatus, IStream *pStm)
{
    TraceMsg(TF_THISMODULE, "BSC_OnStopBinding (hrStatus=0x%08x)", (long)hrStatus);
    ASSERT(m_pCbsc);

// It is ok to not have stream when we requested it (robots.txt)
//  ASSERT(( pStm &&  (m_iOptions & BDU2_NEEDSTREAM)) ||
//         (!pStm && !(m_iOptions & BDU2_NEEDSTREAM)));
    ASSERT(!pStm || (m_iOptions & BDU2_NEEDSTREAM));
    ASSERT(!m_pStm);

    // Save stream for caller if they requested it
    // We keep it until the release it (ReleaseStream) or nav to another url
    if (pStm && (m_iOptions & BDU2_NEEDSTREAM))
    {
        if (m_pStm) m_pStm->Release();
        m_pStm = pStm;
        m_pStm->AddRef();
    }

    // Send OnDownloadComplete, stop the timer
    if (m_iMethod == BDU2_HEADONLY && m_pstLastModified)
        hrStatus = S_OK;        // We got what we came for (hrStatus will be E_ABORT)

    if (m_hrStatus != INET_E_AGENT_BIND_IN_PROGRESS)
        OnDownloadComplete(SUCCEEDED(hrStatus) ? BDU2_ERROR_NONE : BDU2_ERROR_GENERAL);
    else
    {
        DBG("Not calling OnDownloadComplete; synchronous bind");
        m_hrStatus = hrStatus;
    }

    m_fbscValid = FALSE;
    SAFE_RELEASE_BSC();
}

void CUrlDownload::BSC_OnProgress(ULONG ulProgress, ULONG ulProgressMax)
{
    // extend our timer
    if (m_iTimerID)
        StartTimer();
}

void CUrlDownload::BSC_FoundLastModified(SYSTEMTIME *pstLastModified)
{
    DBG("Received last modified time");

    SAFELOCALFREE(m_pstLastModified);

    m_pstLastModified = (SYSTEMTIME *)MemAlloc(LMEM_FIXED, sizeof(SYSTEMTIME));

    if (m_pstLastModified)
    {
        CopyMemory(m_pstLastModified, pstLastModified, sizeof(SYSTEMTIME));
    }
}

void CUrlDownload::BSC_FoundMimeType(CLIPFORMAT cf)
{
    TraceMsg(TF_THISMODULE, "FoundMimeType %d", (int)cf);

    BOOL fAbort = FALSE, fBrowser=FALSE;
    HRESULT hr=S_OK;

    // Abort if not html if necessary.
    if ((m_iOptions & BDU2_FAIL_IF_NOT_HTML) && (cf != g_cfHTML))
    {
        DBG("Aborting non-HTML download");
        fAbort = TRUE;
        OnDownloadComplete(BDU2_ERROR_NOT_HTML);
    }

    // Abort the UrlMon download if necessary. Fire off
    //  a browser download if necessary.
    if (((m_iMethod == BDU2_SMART) || (m_iMethod == BDU2_SNIFF)) && (cf == g_cfHTML))
    {
        // Switch into the browser.
        ASSERT(m_pwszURL);
        if (m_pwszURL &&
            (m_dwResponseCode != 401))      // Don't bother if it's auth failure
        {
            DBG("Switching UrlMon download into browser");
            hr = BeginDownloadWithBrowser(m_pwszURL);
            if (SUCCEEDED(hr))
                fBrowser = TRUE;
        }
    }

    if (fAbort || fBrowser)
    {
        // Disconnect the BSC so that we don't get any more notifications.
        // If we're switching into the browser, don't abort the UrlMon
        //  download to help avoid getting multiple GET requests. We do
        //  disconnect the BSC but still maintain a ref to it so we abort
        //  it if necessary.
        ASSERT(m_pCbsc);
        if (m_pCbsc)
        {
            m_pCbsc->SetParent(NULL);  // We don't want OnStopBinding

            if (fAbort)
            {
                m_pCbsc->Abort();
                m_pCbsc->Release();
                m_pCbsc=NULL;
                m_fbscValid = FALSE;
            }
        }
    }
}

// Returns content for Accept-Language header
LPCWSTR CUrlDownload::GetAcceptLanguages()
{
    if (0 == m_iLangStatus)
    {
        DWORD cchLang = ARRAYSIZE(m_achLang);

        if (SUCCEEDED(::GetAcceptLanguagesW(m_achLang, &cchLang)))
        {
            m_iLangStatus = 1;
        }
        else
        {
            m_iLangStatus = 2;
        }
    }

    if (1 == m_iLangStatus)
    {
        return m_achLang;
    }
    
    return NULL;
}

HRESULT CUrlDownload::ProgressBytes(DWORD dwBytes)
{
    if (m_dwMaxSize > 0 && dwBytes > m_dwMaxSize)
    {
        TraceMsg(TF_THISMODULE, "CUrlDownload MaxSize exceeded aborting. %d of %d bytes", dwBytes, m_dwMaxSize);

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

    if (m_pParent)
        hr = m_pParent->OnAuthenticate(phwnd, ppszUsername, ppszPassword);
    else
        hr = E_NOTIMPL;

    TraceMsg(TF_THISMODULE, "CUrlDownload::Authenticate returning hr=%08x", hr);

    return hr;
}

//---------------------------------------------------------------
// IHlinkFrame
STDMETHODIMP CUrlDownload::SetBrowseContext(IHlinkBrowseContext *pihlbc)
{
    DBG_WARN("CUrlDownload::SetBrowseContext() not implemented");
    return E_NOTIMPL;
}
STDMETHODIMP CUrlDownload::GetBrowseContext(IHlinkBrowseContext **ppihlbc)
{
    DBG_WARN("CUrlDownload::GetBrowseContext() not implemented");
    return E_NOTIMPL;
}
STDMETHODIMP CUrlDownload::Navigate(DWORD grfHLNF, LPBC pbc, IBindStatusCallback *pibsc, IHlink *pihlNavigate)
{
    // We should only get a call through IHlinkFrame->Navigate()
    // when the webcrawler has submitted a form for authentication.
    // Bail out if that's not the case.
    if (!m_fFormSubmitted)
    {
        DBG_WARN("CUrlDownload::Navigate() without a form submission!!!");
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
            StartTimer();       // Start our timeout
            // Need to wait again.
            m_fWaitingForReadyState = TRUE;
            DBG("CUrlDownload::Navigate (IHLinkFrame) succeeded");
        }
    }
    return hr;
}
STDMETHODIMP CUrlDownload::OnNavigate(DWORD grfHLNF, IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName, DWORD dwreserved)
{
    DBG_WARN("CUrlDownload::OnNavigate() not implemented");
    return E_NOTIMPL;
}
STDMETHODIMP CUrlDownload::UpdateHlink(ULONG uHLID, IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName)
{
    DBG_WARN("CUrlDownload::UpdateHlink() not implemented");
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


//---------------------------------------------------------------
// CUrlDownload_BSC class
//---------------------------------------------------------------

CUrlDownload_BSC::CUrlDownload_BSC(
    BDUMethod   iMethod,
    BDUOptions  iOptions,
    LPTSTR      pszLocalFile)
{
    // Maintain global count of objects
    DllAddRef();

    m_cRef = 1;

    m_iMethod = iMethod;
    m_iOptions = iOptions;

    if (NULL != pszLocalFile)
    {
        m_pszLocalFileDest = StrDup(pszLocalFile);
        if (m_iMethod != BDU2_URLMON)
        {
            DBG_WARN("CUrlDownload_BSC changing method to URLMON (local file specified)");
            m_iMethod = BDU2_URLMON;
        }
    }
}

CUrlDownload_BSC::~CUrlDownload_BSC()
{
    // Maintain global count of objects
    DllRelease();

    ASSERT(!m_pBinding);
    SAFERELEASE(m_pstm);
    SAFELOCALFREE(m_pszLocalFileDest);
    SAFELOCALFREE(m_pwszLocalFileSrc);
}

void CUrlDownload_BSC::SetParent(CUrlDownload *pUrlDownload)
{
    m_pParent = pUrlDownload;
}

HRESULT CUrlDownload_BSC::Abort()
{
    if (m_pBinding)
    {
        return m_pBinding->Abort();
    }
    return S_FALSE;
}

STDMETHODIMP CUrlDownload_BSC::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;

    if (riid==IID_IUnknown || riid==IID_IBindStatusCallback)
    {
        *ppv = (IBindStatusCallback *)this;
        AddRef();
        return S_OK;
    }
    if (riid==IID_IHttpNegotiate)
    {
        *ppv = (IHttpNegotiate *)this;
        AddRef();
        return S_OK;
    }
    if (riid==IID_IAuthenticate)
    {
        *ppv = (IAuthenticate *)this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

//---------------------------------------------------------------
// IAuthenticate
STDMETHODIMP CUrlDownload_BSC::Authenticate(HWND *phwnd, LPWSTR *ppszUsername, LPWSTR *ppszPassword)
{   //copied from CUrlDownload::Authenticate (to whom we pass off anyway)
    HRESULT hr;
    ASSERT(phwnd && ppszUsername && ppszPassword);
    
    *phwnd = (HWND)-1;
    *ppszUsername = NULL;
    *ppszPassword = NULL;

    // Only try this once. If Urlmon asks again, fail it and flag an error.
    if (m_fTriedAuthenticate)
    {
        if (m_pParent)
        {
            m_pParent->m_dwResponseCode = 401;
            DBG("CUrlDownload_BSC::Authenticate called twice. Faking 401 response");
        }

        return E_FAIL;
    }

    m_fTriedAuthenticate = TRUE;

    if (m_pParent)
        hr = m_pParent->Authenticate(phwnd, ppszUsername, ppszPassword);
    else
        hr = E_NOTIMPL;

    if (FAILED(hr) && m_pParent)
    {
        m_pParent->m_dwResponseCode = 401;
        DBG("CUrlDownload_BSC::Authenticate called; no username/pass. Faking 401 response");
    }

    TraceMsg(TF_THISMODULE, "CUrlDownload_BSC::Authenticate returning hr=%08x", hr);

    return hr;
}

STDMETHODIMP CUrlDownload_BSC::OnStartBinding(
    DWORD dwReserved,
    IBinding* pbinding)
{
    m_fSentMimeType = FALSE;
    if (m_pBinding != NULL)
        m_pBinding->Release();
    m_pBinding = pbinding;
    if (m_pBinding != NULL)
    {
        m_pBinding->AddRef();
    }
    if (m_pParent)
        m_pParent->BSC_OnStartBinding();
    return S_OK;
}

// ---------------------------------------------------------------------------
// %%Function: CUrlDownload_BSC::GetPriority
// ---------------------------------------------------------------------------
 STDMETHODIMP
CUrlDownload_BSC::GetPriority(LONG* pnPriority)
{
    return E_NOTIMPL;
}

// ---------------------------------------------------------------------------
// %%Function: CUrlDownload_BSC::OnLowResource
// ---------------------------------------------------------------------------
 STDMETHODIMP
CUrlDownload_BSC::OnLowResource(DWORD dwReserved)
{
    return E_NOTIMPL;
}

// ---------------------------------------------------------------------------
// %%Function: CUrlDownload_BSC::OnProgress
// ---------------------------------------------------------------------------
 STDMETHODIMP
CUrlDownload_BSC::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
//  TraceMsg(TF_THISMODULE, "cbsc::OnProgress %d of %d : msg %ws", ulProgress, ulProgressMax, szStatusText);

    /*
    if (ulStatusCode==BINDSTATUS_USINGCACHEDCOPY)
    */
    if (ulStatusCode == BINDSTATUS_REDIRECTING)
    {
        DBG("CUrlDownload_BSC::OnProgress getting redirected url");
        TraceMsg(TF_THISMODULE, "New url=%ws", szStatusText);
        if (m_pParent)
        {
            if (m_pParent->m_pwszURL) MemFree(m_pParent->m_pwszURL);
            m_pParent->m_pwszURL = StrDupW(szStatusText);
        }
    }

    if ((ulStatusCode == BINDSTATUS_CACHEFILENAMEAVAILABLE) && m_pszLocalFileDest)
    {
        ASSERT(!m_pwszLocalFileSrc);
        DBG("CUrlDownload_BSC::OnProgress Getting local file name");
        if (!m_pwszLocalFileSrc)
            m_pwszLocalFileSrc = StrDupW(szStatusText);
    }

    if (m_pParent)
        m_pParent->BSC_OnProgress(ulProgress, ulProgressMax);

    // 14032: If dialmon is around, tell it that something is going on
    IndicateDialmonActivity();

    return S_OK;
}

STDMETHODIMP CUrlDownload_BSC::OnStopBinding(
    HRESULT     hrStatus,
    LPCWSTR     pszError)
{
#ifdef DEBUG
    if (hrStatus && (hrStatus != E_ABORT))
        TraceMsg(TF_THISMODULE,
            "cbsc: File download Failed hr=%08x.", (int)hrStatus);
#endif

    if (m_pParent)
        m_pParent->BSC_OnStopBinding(hrStatus, (m_iOptions&BDU2_NEEDSTREAM) ? m_pstm : NULL);

    // We should have neither or both of these
    ASSERT(!m_pwszLocalFileSrc == !m_pszLocalFileDest);

    if (m_pwszLocalFileSrc && m_pszLocalFileDest)
    {
        // Copy or move file from cache file to file/directory requested
        // We have a LPWSTR source name and an LPTSTR destination
        TCHAR szSrc[MAX_PATH];
        TCHAR szDest[MAX_PATH];
        LPTSTR pszSrcFileName, pszDest=NULL;

        MyOleStrToStrN(szSrc, MAX_PATH, m_pwszLocalFileSrc);

        // Combine paths to find destination filename if necessary
        if (PathIsDirectory(m_pszLocalFileDest))
        {
            pszSrcFileName = PathFindFileName(szSrc);
            if (pszSrcFileName)
            {
                PathCombine(szDest, m_pszLocalFileDest, pszSrcFileName);
                pszDest = szDest;
            }
        }
        else
        {
            pszDest = m_pszLocalFileDest;
        }

        if (pszDest)
        {
            TraceMsg(TF_THISMODULE, "Copying file\n%s\n to file \n%s", szSrc, pszDest);
            CopyFile(szSrc, pszDest, FALSE);
        }
        else
            DBG_WARN("Unable to get dest path for local file");
    }

    SAFERELEASE(m_pstm);
    SAFERELEASE(m_pBinding);

    return S_OK;
}

STDMETHODIMP CUrlDownload_BSC::GetBindInfo(
    DWORD       *pgrfBINDF,
    BINDINFO    *pbindInfo)
{
    if ( !pgrfBINDF || !pbindInfo || !pbindInfo->cbSize )
        return E_INVALIDARG;

    *pgrfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_NO_UI;
    if (m_pszLocalFileDest)
        *pgrfBINDF |= BINDF_NEEDFILE;
    if (m_pParent && m_pParent->m_fSetResync)
        *pgrfBINDF |= BINDF_RESYNCHRONIZE;
    if (m_pParent && (m_pParent->m_lBindFlags & DLCTL_FORCEOFFLINE))
        *pgrfBINDF |= BINDF_OFFLINEOPERATION;

    // clear BINDINFO but keep its size
    DWORD cbSize = pbindInfo->cbSize;
    ZeroMemory( pbindInfo, cbSize );
    pbindInfo->cbSize = cbSize;

    pbindInfo->dwBindVerb = BINDVERB_GET;

    if (m_iMethod == BDU2_HEADONLY)
    {
        LPWSTR pwszVerb = (LPWSTR) CoTaskMemAlloc(sizeof(c_wszHeadVerb));
        if (pwszVerb)
        {
            CopyMemory(pwszVerb, c_wszHeadVerb, sizeof(c_wszHeadVerb));
            pbindInfo->dwBindVerb = BINDVERB_CUSTOM;
            pbindInfo->szCustomVerb = pwszVerb;
            DBG("Using 'HEAD' custom bind verb.");
        }
        else
        {
            DBG_WARN("MemAlloc failure CUrlDownload_BSC::GetBindInfo");
            return E_OUTOFMEMORY;
        }
    }

    return S_OK;
}

STDMETHODIMP CUrlDownload_BSC::OnDataAvailable(
    DWORD grfBSCF,
    DWORD dwSize,
    FORMATETC* pfmtetc,
    STGMEDIUM* pstgmed)
{
    TraceMsg(TF_THISMODULE, "%d bytes on page so far (urlmon)", dwSize);

    if (m_pParent)
        if (FAILED(m_pParent->ProgressBytes(dwSize)))
            return S_OK;

        // Get the Stream passed if we want a local file (to lock the file)
    // We just ignore any data in any case
    if (BSCF_FIRSTDATANOTIFICATION & grfBSCF)
    {
        if (!m_pstm && (pstgmed->tymed==TYMED_ISTREAM) &&
            (m_pszLocalFileDest || (m_iOptions & BDU2_NEEDSTREAM)))
        {
            m_pstm = pstgmed->pstm;
            if (m_pstm)
                m_pstm->AddRef();
        }
    }

    if (!m_fSentMimeType && pfmtetc && m_pParent)
    {
        m_pParent->BSC_FoundMimeType(pfmtetc->cfFormat);
        m_fSentMimeType = TRUE;
    }

    if (BSCF_LASTDATANOTIFICATION & grfBSCF)
    {
        DBG("cbsc: LastDataNotification");
    }

    return S_OK;
}  // CUrlDownload_BSC::OnDataAvailable

STDMETHODIMP CUrlDownload_BSC::OnObjectAvailable(REFIID riid, IUnknown* punk)
{
    return E_NOTIMPL;
}

STDMETHODIMP CUrlDownload_BSC::BeginningTransaction(
        LPCWSTR szURL,      LPCWSTR szHeaders,
        DWORD dwReserved,   LPWSTR *pszAdditionalHeaders)
{
    // Add User-Agent and Accept-Language headers
    DBG("CUrlDownload_BSC::BeginningTransaction returning headers");

    LPCWSTR pwszAcceptLanguage;
    int iUAlen=0, iALlen=0;     // in chars, with \r\n, without null-term
    LPWSTR pwsz;
    LPCWSTR pwszUA = m_pParent ? m_pParent->GetUserAgent() : NULL;
    
    pwszAcceptLanguage = (m_pParent) ? m_pParent->GetAcceptLanguages() : NULL;

    if (pwszUA)
    {
        iUAlen = ARRAYSIZE(c_szUserAgentPrefix) + lstrlenW(pwszUA) + 1;
    }
    
    if (pwszAcceptLanguage)
    {
        iALlen = ARRAYSIZE(c_szAcceptLanguagePrefix) + lstrlenW(pwszAcceptLanguage)+1;
    }

    if (iUAlen || iALlen)
    {
        pwsz = (WCHAR *)CoTaskMemAlloc((iUAlen + iALlen + 1) * sizeof(WCHAR));

        if (pwsz)
        {
            pwsz[0] = L'\0';
            
            if (iUAlen)
            {
                StrCpyW(pwsz, c_szUserAgentPrefix);
                StrCatW(pwsz, pwszUA);
                StrCatW(pwsz, L"\r\n");
            }

            if (iALlen)
            {
                StrCatW(pwsz, c_szAcceptLanguagePrefix);
                StrCatW(pwsz, pwszAcceptLanguage);
                StrCatW(pwsz, L"\r\n");
            }

            ASSERT(lstrlenW(pwsz) == (iUAlen + iALlen));

            *pszAdditionalHeaders = pwsz;

            return S_OK;
        }
    }

    return E_OUTOFMEMORY;
}
    
STDMETHODIMP CUrlDownload_BSC::OnResponse(
        DWORD   dwResponseCode,     LPCWSTR szResponseHeaders, 
        LPCWSTR szRequestHeaders,   LPWSTR *pszAdditionalRequestHeaders)
{
    TraceMsg(TF_THISMODULE, "CUrlDownload_BSC::OnResponse - %d", dwResponseCode);

    // If we sent a "HEAD" request, Urlmon will hang expecting data.
    // Abort it here.
    if (m_iMethod == BDU2_HEADONLY)
    {
        // First get the Last-Modified date from Urlmon
        IWinInetHttpInfo    *pInfo;

        if (m_pParent
            && SUCCEEDED(m_pBinding->QueryInterface(IID_IWinInetHttpInfo, (void **)&pInfo)
            && pInfo))
        {
            SYSTEMTIME  st;
            DWORD       dwSize = sizeof(st), dwZero=0;

            if (SUCCEEDED(pInfo->QueryInfo(HTTP_QUERY_FLAG_SYSTEMTIME | HTTP_QUERY_LAST_MODIFIED,
                                           (LPVOID) &st, &dwSize, &dwZero, 0)))
            {
                m_pParent->BSC_FoundLastModified(&st);
            }

            pInfo->Release();
        }
        Abort();    // BUGBUG return E_ABORT and handle abort internally
    }

    if (m_pParent)
        m_pParent->m_dwResponseCode = dwResponseCode;
    else
        DBG_WARN("CUrlDownload_BSC::OnResponse - Parent already NULL");

    return S_OK;
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
    DBG("CUrlDownload::GetMoniker returning failure");
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
BOOL ParseRefreshContent(LPWSTR pwzContent,
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
