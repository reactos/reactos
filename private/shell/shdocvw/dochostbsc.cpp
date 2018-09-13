#include "priv.h"
#include "iehelpid.h"
#include "bindcb.h"
#include "winlist.h"
#include "droptgt.h"
#include <mshtml.h>     // CLSID_HTMLDocument
#include "resource.h"
#include <htmlhelp.h>
#include <prsht.h>
#include <inetcpl.h>
#include <optary.h>
#include "shdocfl.h"

#ifdef FEATURE_PICS
#include <shlwapi.h>
#include <ratings.h>
#endif

#include "dochost.h"

#include <mluisupp.h>

#define THISCLASS CDocObjectHost
#define SUPERCLASS CDocHostUIHandler

#define BSCMSG(psz, i, j)       TraceMsg(TF_SHDBINDING, "shd TR-BSC::%s %x %x", psz, i, j)
#define BSCMSG3(psz, i, j, k)   TraceMsg(0, "shd TR-BSC::%s %x %x %x", psz, i, j, k)
#define BSCMSG4(psz, i, j, k, l)        TraceMsg(0, "shd TR-BSC::%s %x %x %x %x", psz, i, j, k, l)
#define BSCMSGS(psz, sz)        TraceMsg(0, "shd TR-BSC::%s %s", psz, sz)
#define CHAINMSG(psz, x)        TraceMsg(0, "shd CHAIN::%s %x", psz, x)
#define PERFMSG(psz, x)         TraceMsg(TF_SHDPERF, "PERF::%s %d msec", psz, x)
#define OPENMSG(psz)            TraceMsg(TF_SHDBINDING, "shd OPENING %s", psz)

#define DM_DOCCP        0
#define DM_DEBUGTFRAME  0
#define DM_SELFASC      TF_SHDBINDING
#define DM_SSL              0
#define DM_PICS         0

#define DO_SEARCH_ON_STATUSCODE(x) ((x == 0) || (x == HTTP_STATUS_BAD_GATEWAY) || (x == HTTP_STATUS_GATEWAY_TIMEOUT))

const static c_aidRes[] = {
    IDI_STATE_NORMAL,          // 0
    IDI_STATE_FINDINGRESOURCE, // BINDSTATUS_FINDINGRESOURCE
    IDI_STATE_FINDINGRESOURCE, // BINDSTATUS_CONNECTING
    IDI_STATE_FINDINGRESOURCE, // BINDSTATUS_REDIRECTING
    IDI_STATE_DOWNLOADINGDATA, // BINDSTATUS_BEGINDOWNLOADDATA
    IDI_STATE_DOWNLOADINGDATA, // BINDSTATUS_DOWNLOADINGDATA
    IDI_STATE_DOWNLOADINGDATA, // BINDSTATUS_ENDDOWNLOADDATA
    IDI_STATE_DOWNLOADINGDATA, // BINDSTATUS_BEGINDOWNLOADCOMPONENTS
    IDI_STATE_DOWNLOADINGDATA, // BINDSTATUS_INSTALLINGCOMPONENTS
    IDI_STATE_DOWNLOADINGDATA, // BINDSTATUS_ENDDOWNLOADCOMPONENTS
    IDI_STATE_SENDINGREQUEST,  // BINDSTATUS_USINGCACHEDCOPY
    IDI_STATE_SENDINGREQUEST,  // BINDSTATUS_SENDINGREQUEST
    IDI_STATE_DOWNLOADINGDATA, // BINDSTATUS_CLASSIDAVAILABLE
};

extern HICON g_ahiconState[IDI_STATE_LAST-IDI_STATE_FIRST+1];


#define SEARCHPREFIX        L"? "
#define SEARCHPREFIXSIZE    sizeof(SEARCHPREFIX)
#define SEARCHPREFIXLENGTH  2

//
// Put the most common errors first in c_aErrorUrls.
//

struct
{
    DWORD   dwError;
    LPCTSTR pszUrl;
} c_aErrorUrls[] = { {404, TEXT("http_404.htm")}, 
                     {ERRORPAGE_DNS, TEXT("dnserror.htm")},
                     {ERRORPAGE_NAVCANCEL, TEXT("navcancl.htm")},
                     {ERRORPAGE_SYNTAX, TEXT("syntax.htm")},
                     {400, TEXT("http_400.htm")},
                     {403, TEXT("http_403.htm")},
                     {405, TEXT("http_gen.htm")},
                     {406, TEXT("http_406.htm")},
                     {408, TEXT("servbusy.htm")},
                     {409, TEXT("servbusy.htm")},
                     {410, TEXT("http_410.htm")},
                     {500, TEXT("http_500.htm")},
                     {501, TEXT("http_501.htm")},
                     {505, TEXT("http_501.htm")},
                     {ERRORPAGE_OFFCANCEL, TEXT("offcancl.htm")},
                     {ERRORPAGE_CHANNELNOTINCACHE, TEXT("cacheerr.htm")},
                   };

//
// Determine if there is an internal error page for the given http error.
//

BOOL IsErrorHandled(DWORD dwError)
{
    BOOL fRet = FALSE;

    for (int i = 0; i < ARRAYSIZE(c_aErrorUrls); i++)
    {
        if (dwError == c_aErrorUrls[i].dwError)
        {
            fRet = TRUE;
            break;
        }
    }

    return fRet;
}


const SA_BSTRGUID s_sstrSearchIndex = {
    38 * SIZEOF(WCHAR),
    L"{265b75c0-4158-11d0-90f6-00c04fd497ea}"
};

//extern const SA_BSTRGUID s_sstrSearchFlags;
const SA_BSTRGUID s_sstrSearchFlags = {
    38 * SIZEOF(WCHAR),
    L"{265b75c1-4158-11d0-90f6-00c04fd497ea}"
};

EXTERN_C const SA_BSTRGUID s_sstrSearch = {
    38 * SIZEOF(WCHAR),
    L"{118D6040-8494-11d2-BBFE-0060977B464C}"
};

EXTERN_C const SA_BSTRGUID s_sstrFailureUrl = {
    38 * SIZEOF(WCHAR),
    L"{04AED800-8494-11d2-BBFE-0060977B464C}"
};


//
// Clears that parameters set by window.external.AutoScan()
//
HRESULT _ClearSearchString(IServiceProvider* psp)
{
    HRESULT hr = E_FAIL;

    if (psp == NULL)
        return hr;

    IWebBrowser2 *pWB2 = NULL;
    hr = psp->QueryService(SID_SHlinkFrame, IID_IWebBrowser2, (LPVOID*)&pWB2);
    if (pWB2 && SUCCEEDED(hr))
    {
        VARIANT v;
        VariantInit(&v);
        v.vt = VT_EMPTY;

        hr = pWB2->PutProperty((BSTR)s_sstrSearch.wsz, v);
        hr = pWB2->PutProperty((BSTR)s_sstrFailureUrl.wsz, v);
        pWB2->Release();
    }
    return hr;
}

//
// Gets the string that was entered in the addressbar
//
HRESULT _GetSearchString(IServiceProvider* psp, VARIANT* pvarSearch)
{
    HRESULT hr = E_FAIL;

    if (psp == NULL)
        return hr;

    VariantInit(pvarSearch);
    IDockingWindow* psct = NULL;
    IOleCommandTarget* poct;

    hr = psp->QueryService(SID_SExplorerToolbar, IID_IDockingWindow, (LPVOID*)&psct);
    if (SUCCEEDED(hr))
    {
        hr = psct->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&poct);
        if (SUCCEEDED(hr)) 
        {
            // NULL is the first parameter so our ErrorMsgBox
            // doesn't call EnableModelessSB()
            // If we don't, our pdoh members may be freed 
            // by the time we return.
            hr = poct->Exec(&CGID_Explorer, SBCMDID_GETUSERADDRESSBARTEXT, 0, NULL, pvarSearch);
            poct->Release();
        }
        psct->Release();
    }
    return hr;
}

//
// Get page that should be displayed if the AutoScan fails
//
HRESULT _GetScanFailureUrl(IServiceProvider* psp, VARIANT* pvarFailureUrl)
{
    HRESULT hr = E_FAIL;

    if (psp == NULL)
        return hr;

    //
    // See if a default failure page is stored as a property of the page
    //
    IWebBrowser2 *pWB2 = NULL;
    hr = psp->QueryService(SID_SHlinkFrame, IID_IWebBrowser2, (LPVOID*)&pWB2);
    if (pWB2 && SUCCEEDED(hr))
    {
        VARIANT v;
        VariantInit(&v);
        v.vt = VT_I4;

        hr = pWB2->GetProperty((BSTR)s_sstrFailureUrl.wsz, pvarFailureUrl);
        pWB2->Release();
    }
    return hr;
}

HRESULT _GetSearchInfo(IServiceProvider *psp, LPDWORD pdwIndex, LPBOOL pfAllowSearch, LPBOOL pfContinueSearch, LPBOOL pfSentToEngine, VARIANT* pvarUrl)
{
    HRESULT hr = E_FAIL;
    DWORD   dwFlags = 0;

    if (psp) {
        IWebBrowser2 *pWB2 = NULL;
        hr = psp->QueryService(SID_SHlinkFrame, IID_IWebBrowser2, (LPVOID*)&pWB2);
        if (pWB2 && SUCCEEDED(hr)) {
            VARIANT v;
            VariantInit (&v);

            if (pdwIndex) {
                v.vt = VT_I4;
                if (SUCCEEDED(pWB2->GetProperty((BSTR)s_sstrSearchIndex.wsz, &v))) {
                    if (v.vt == VT_I4)
                        *pdwIndex = v.lVal;
                }
            }
            if (pfAllowSearch || pfContinueSearch || pfSentToEngine) {
                v.vt = VT_I4;
                if (SUCCEEDED(pWB2->GetProperty((BSTR)s_sstrSearchFlags.wsz, &v))) {
                    if (v.vt == VT_I4)
                        dwFlags = v.lVal;
                }
            }

            //
            // If we have a search string property, and the index is zero, we start
            // with the second autoscan index.  This is because the first index should
            // have already been tried (see window.external.AutoScan()).
            //
            if (pvarUrl)
            {
                pvarUrl->vt = VT_EMPTY;
                if (SUCCEEDED(pWB2->GetProperty((BSTR)s_sstrSearch.wsz, pvarUrl)) &&
                    pvarUrl->vt == VT_BSTR && *pdwIndex == 0)
                {
                    *pdwIndex = 2;
                }
            }

            if (pfAllowSearch)
                *pfAllowSearch = ((dwFlags & 0x01) ? TRUE : FALSE);
            if (pfContinueSearch)
                *pfContinueSearch = ((dwFlags & 0x02) ? TRUE : FALSE);
            if (pfSentToEngine)
                *pfSentToEngine = ((dwFlags & 0x04) ? TRUE : FALSE);

            pWB2->Release();
        }
    }
    return hr;
}

HRESULT CDocObjectHost::CDOHBindStatusCallback::_SetSearchInfo(CDocObjectHost *pdoh, DWORD dwIndex, BOOL fAllowSearch, BOOL fContinueSearch, BOOL fSentToEngine)
{
    HRESULT hr = E_FAIL;
    DWORD   dwFlags = 0;

    dwFlags = (fAllowSearch ? 0x01 : 0) +
              (fContinueSearch ? 0x02 : 0) +
              (fSentToEngine ? 0x04 : 0);

    if (pdoh->_psp)
    {
        IWebBrowser2 *pWB2 = NULL;
        hr = pdoh->_psp->QueryService(SID_SHlinkFrame, IID_IWebBrowser2, (LPVOID*)&pWB2);
        if (pWB2 && SUCCEEDED(hr))
        {
            VARIANT v;
            VariantInit (&v);

            v.vt = VT_I4;
            v.lVal = dwIndex;
            pWB2->PutProperty((BSTR)s_sstrSearchIndex.wsz, v);

            v.vt = VT_I4;
            v.lVal = dwFlags;
            pWB2->PutProperty((BSTR)s_sstrSearchFlags.wsz, v);

            pWB2->Release();
        }
    }

    // If we are done, clear any parameters set by window.external.AutosScan().
    if (!fContinueSearch)
    {
        _ClearSearchString(pdoh->_psp);
    }
    TraceMsg(TF_SHDNAVIGATE, "::HFNS_SetSearchInfo() hr = %X, index = %d, allow = %d, cont = %d, sent = %d", hr, dwIndex, fAllowSearch, fContinueSearch, fSentToEngine);
    
    return hr;
}

BOOL IsAutoSearchEnabled()
{
    // First see if autosearch is enabled (defaults enabled if key is missing)
    DWORD dwType;
    DWORD dwValue;
    DWORD cbValue = sizeof(dwValue);
    BOOL fRet = (SHRegGetUSValue(REGSTR_PATH_MAIN, L"AutoSearch", &dwType, &dwValue, &cbValue, FALSE, NULL, 0) != ERROR_SUCCESS ||
                 dwValue != 0);
    return fRet;
}

//
// Gets the prefix/postfix to use for autoscanning (www.%s.com, etc)
//
LONG GetSearchFormatString(DWORD dwIndex, LPTSTR psz, DWORD cbpsz) 
{
    TCHAR szValue[10];
    DWORD dwType;

    wnsprintf(szValue, ARRAYSIZE(szValue), TEXT("%d"), dwIndex);
    return SHRegGetUSValue(REGSTR_PATH_SEARCHSTRINGS, szValue, &dwType, (LPVOID)psz, &cbpsz, FALSE, NULL, 0);
}


// dwSearchForExtensions : 0     do not search
// dwSearchForExtensions : 1     search through list of exts.
// dwSearchForExtensions : 2     move on to autosearch

// 0 = never ask, never search
// 1 = always ask
// 2 = never ask, always search

HRESULT GetSearchKeys(LPDWORD pdwSearchForExtensions, LPDWORD pdwDo404Search)
{
/*
    DWORD   dwDefault, dwType = REG_DWORD;
    DWORD   cbDword = SIZEOF(DWORD);

    ASSERT(pdwSearchForExtensions);
    ASSERT(pdwDo404Search);
    
    dwDefault = SCAN_SUFFIXES;
    SHRegGetUSValue(REGSTR_PATH_MAIN, REGSTR_VAL_AUTONAVIGATE, &dwType, (LPVOID) pdwSearchForExtensions, &cbDword, FALSE, &dwDefault, SIZEOF(dwDefault));

    ASSERT(cbDword == SIZEOF(DWORD));

    dwDefault = PROMPTSEARCH;
    SHRegGetUSValue(REGSTR_PATH_MAIN, REGSTR_VAL_AUTOSEARCH, &dwType, (LPVOID) pdwDo404Search, &cbDword, FALSE, &dwDefault, SIZEOF(dwDefault));
*/
    DWORD dwType;
    DWORD dwAutoSearch;
    DWORD cb = sizeof(dwAutoSearch);
    if (SHRegGetUSValue(REGSTR_PATH_MAIN, L"AutoSearch", &dwType, &dwAutoSearch, &cb, FALSE, NULL, 0) != ERROR_SUCCESS)
    {
        // Default to "display results in search pane and go to most likely site"
        dwAutoSearch = 3;
    }

    if (dwAutoSearch == 0)
    {
        *pdwSearchForExtensions = NO_SUFFIXES;
        *pdwDo404Search = NEVERSEARCH;
    }
    else
    {
        *pdwSearchForExtensions = SCAN_SUFFIXES;
        *pdwDo404Search = ALWAYSSEARCH;
    }

    return S_OK;
} // GetSearchKeys




//
// Map error codes to error urls.
//

int EUIndexFromError(DWORD dwError)
{
    for (int i = 0; i < ARRAYSIZE(c_aErrorUrls); i++)
    {
        if (dwError == c_aErrorUrls[i].dwError)
            break;
    }

    ASSERT(i < ARRAYSIZE(c_aErrorUrls));

    return i;
}

//
// IsErrorUrl determines if the given url is an internal error page url.  
//


BOOL IsErrorUrl(LPCWSTR pwszDisplayName)
{
    BOOL fRet = FALSE;
    TCHAR szDisplayName[MAX_URL_STRING];
    UnicodeToTChar(pwszDisplayName, szDisplayName, ARRAYSIZE(szDisplayName));

    //
    // First check if the prefix matches.
    //

    if (0 == StrCmpN(szDisplayName, TEXT("res://"), 6))
    {
        int iResStart;

        // find the resource name part of the URL
        // use the fact that the DLL path will be using
        // '\' as delimiters while the URL in general
        // uses '/'

        iResStart = 6;
        while (szDisplayName[iResStart] != TEXT('/') &&
               szDisplayName[iResStart] != TEXT('\0'))
        {
            iResStart++;
        }
        iResStart++;    // get off the '/'

        //
        // Check each url in order.
        //
        for (int i = 0; i < ARRAYSIZE(c_aErrorUrls); i++)
        {
            if (0 == StrCmpN(szDisplayName + iResStart, c_aErrorUrls[i].pszUrl,
                             lstrlen(c_aErrorUrls[i].pszUrl)))
            {
                fRet = TRUE;
                break;
            }
        }
    }

    return fRet;
}

//
// When an http error occurs the server generally returns a page.  The
// threshold value this function returns is used to determine if the
// server page is displayed (if the size of the returned page is greater than
// the threshold) or if an internal error page is shown (if the returned page
// is smaller than the threshold).
//

DWORD _GetErrorThreshold(DWORD dwError)
{
    DWORD dwRet;

    TCHAR  szValue[11]; //Should be large enough to hold max dword 4294967295
    DWORD cbValue = ARRAYSIZE(szValue);
    DWORD cbdwRet = sizeof(dwRet);
    DWORD dwType  = REG_DWORD;

    wnsprintf(szValue, ARRAYSIZE(szValue), TEXT("%d"), dwError);

    if (ERROR_SUCCESS != SHRegGetUSValue(REGSTR_PATH_THRESHOLDS, szValue,
                                          &dwType, (LPVOID)&dwRet, &cbdwRet,
                                          FALSE, NULL, 0))
    {
        dwRet = 512; // hard coded default size if all else fails.
    }

    return dwRet;
}

void CDocObjectHost::CDOHBindStatusCallback::_RegisterObjectParam(IBindCtx* pbc)
{
    // pbc->RegisterObjectParam(L"BindStatusCallback", this);
    HRESULT hres = RegisterBindStatusCallback(pbc, this, 0, 0);
    BSCMSG3(TEXT("_RegisterObjectParam returned"), hres, this, pbc);
}

void CDocObjectHost::CDOHBindStatusCallback::_RevokeObjectParam(IBindCtx* pbc)
{
    // pbc->RevokeObjectParam(L"BindStatusCallback");
    HRESULT hres = RevokeBindStatusCallback(pbc, this);
    AssertMsg(SUCCEEDED(hres), TEXT("URLMON bug??? RevokeBindStatusCallback failed %x"), hres);
    BSCMSG3(TEXT("_RevokeObjectParam returned"), hres, this, pbc);
}

CDocObjectHost::CDOHBindStatusCallback::~CDOHBindStatusCallback()
{
    TraceMsg(DM_DEBUGTFRAME, "dtor CDocObjectHost::CBSC %x", this);

    if (_pib) {
        AssertMsg(0, TEXT("CBSC::~ _pib is %x (this=%x)"), _pib, this);
    }
    ATOMICRELEASE(_pib);

    if (_pbc) {
        AssertMsg(0, TEXT("CBSC::~ _pbc is %x (this=%x)"), _pbc, this);
    }
    ATOMICRELEASE(_pbc);

    if (_psvPrev) {
        AssertMsg(0, TEXT("CBSC::~ _psvPrev is %x (this=%x)"), _psvPrev, this);
    }

    ATOMICRELEASE(_psvPrev);
    ATOMICRELEASE(_pbscChained);
    ATOMICRELEASE(_pnegotiateChained);

    if (_hszPostData)
    {
        GlobalFree(_hszPostData);
        _hszPostData = NULL;
    }
    if (_pszHeaders)
    {
        LocalFree(_pszHeaders);
        _pszHeaders = NULL;
    }
    if (_pszRedirectedURL)
    {
        LocalFree(_pszRedirectedURL);
        _pszRedirectedURL = NULL;
    }
}

HRESULT CDocObjectHost::CDOHBindStatusCallback::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IBindStatusCallback) || 
        IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = SAFECAST(this, IBindStatusCallback*);
    }
    else if (IsEqualIID(riid, IID_IHttpNegotiate))
    {
        *ppvObj = SAFECAST(this, IHttpNegotiate*);
    }
    else if (IsEqualIID(riid, IID_IAuthenticate))
    {
        *ppvObj = SAFECAST(this, IAuthenticate*);
    }
    else if (IsEqualIID(riid, IID_IServiceProvider))
    {
        *ppvObj = SAFECAST(this, IServiceProvider*);
    }
    else if (IsEqualIID(riid, IID_IHttpSecurity))
    {
        *ppvObj = SAFECAST(this, IHttpSecurity*);
    }
    else if (IsEqualIID(riid, IID_IWindowForBindingUI))
    {
        *ppvObj = SAFECAST(this, IWindowForBindingUI*);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}

ULONG CDocObjectHost::CDOHBindStatusCallback::AddRef(void)
{
    CDocObjectHost* pdoh = IToClass(CDocObjectHost, _bsc, this);
    return pdoh->AddRef();
}

ULONG CDocObjectHost::CDOHBindStatusCallback::Release(void)
{
    CDocObjectHost* pdoh = IToClass(CDocObjectHost, _bsc, this);
    return pdoh->Release();
}

void SetBindfFlagsBasedOnAmbient(BOOL fAmbientOffline, DWORD *grfBindf);

HRESULT CDocObjectHost::CDOHBindStatusCallback::GetBindInfo(
     DWORD* grfBINDF,
     BINDINFO *pbindinfo)
{
    if ( !grfBINDF || !pbindinfo || !pbindinfo->cbSize )
        return E_INVALIDARG;

    DWORD dwConnectedStateFlags = 0;
    CDocObjectHost* pdoh = IToClass(CDocObjectHost, _bsc, this);
    BSCMSG(TEXT("GetBindInfo"), 0, 0);

    *grfBINDF = BINDF_ASYNCHRONOUS;

    // Delegation is valid ONLY for the ::GetBindInfo() method
    if (_pbscChained) {
        CHAINMSG("GetBindInfo", grfBINDF);
        _pbscChained->GetBindInfo(grfBINDF, pbindinfo);
        
        pdoh->_uiCP = pbindinfo->dwCodePage;
        
        // As far as offline mode is concerned, we want the latest
        // info. Over-rule what the delegated IBSC returned

        SetBindfFlagsBasedOnAmbient(_bFrameIsOffline, grfBINDF);

        if(_bFrameIsSilent)
            *grfBINDF |= BINDF_NO_UI;  
        else
            *grfBINDF &= ~BINDF_NO_UI;
           
    }
    else
    {
        // fill out the BINDINFO struct
        *grfBINDF = 0;
        BuildBindInfo(grfBINDF,pbindinfo,_hszPostData,_cbPostData,
            _bFrameIsOffline, _bFrameIsSilent, FALSE, /* bHyperlink */
            (IBindStatusCallback *) this);

        // HTTP headers are added by the callback to our
        // IHttpNegotiate::BeginningTransaction() method

    }

    // Remember it to perform modeless download for POST case.
    _dwBindVerb = pbindinfo->dwBindVerb;

    return S_OK;
}

// *** IAuthenticate ***
HRESULT CDocObjectHost::CDOHBindStatusCallback::Authenticate(
    HWND *phwnd,
    LPWSTR *pszUsername,
    LPWSTR *pszPassword)
{
    CDocObjectHost* pdoh = IToClass(CDocObjectHost, _bsc, this);

    if (!phwnd || !pszUsername || !pszPassword)
        return E_POINTER;



    if(!_bFrameIsSilent){
        if (pdoh->_psb) {
            pdoh->_psb->GetWindow(phwnd);
        } else {
            *phwnd = pdoh->_hwnd;
        }
    }else{
        *phwnd = NULL;
    }

    *pszUsername = NULL;
    *pszPassword = NULL;
    // If we're a frame in the active desktop, then find out
    // the user name and password are stored with the subscription
    // and use it
    if(_IsDesktopItem(pdoh))
    {
        // Get the URL
        LPOLESTR pszURL;
        HRESULT hres;
        hres = pdoh->_GetCurrentPageW(&pszURL, TRUE);
        if(SUCCEEDED(hres))
        {
            IActiveDesktop *pActiveDesk;
         
            hres = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER, IID_IActiveDesktop, (LPVOID*)&pActiveDesk);
            
            if(SUCCEEDED(hres))
            {
                // Get the subscribed URL for this
                COMPONENT Component;

                Component.dwSize = SIZEOF(Component);
                Component.wszSubscribedURL[0] = TEXT('\0');
                hres = pActiveDesk->GetDesktopItemBySource(pszURL, &Component, 0);
                if(SUCCEEDED(hres) && Component.wszSubscribedURL[0])
                {
                    // We have a non null subscribed URL
                    // Gotta find the user name and password 
                    // associated with this subscription
                    ISubscriptionMgr *pSubsMgr;

                    hres = CoCreateInstance(CLSID_SubscriptionMgr, NULL,
                                            CLSCTX_INPROC_SERVER, 
                                    IID_ISubscriptionMgr, (LPVOID*)&pSubsMgr);

                    if(SUCCEEDED(hres))
                    {
                        SUBSCRIPTIONINFO SubInfo;
                        SubInfo.cbSize = sizeof(SUBSCRIPTIONINFO);
                        SubInfo.fUpdateFlags = (SUBSINFO_NEEDPASSWORD | SUBSINFO_TYPE 
                                                 | SUBSINFO_USER | SUBSINFO_PASSWORD);
                        SubInfo.bstrUserName = NULL;
                        SubInfo.bstrPassword = NULL;
                        hres = pSubsMgr->GetSubscriptionInfo(Component.wszSubscribedURL, &SubInfo);
                        if(SUCCEEDED(hres) && SubInfo.bNeedPassword)
                        {
                            if((SubInfo.bstrUserName) && (SubInfo.bstrPassword))
                            {
                                // Copy  user name and password
                                SHStrDupW(SubInfo.bstrPassword, pszPassword);
                                SHStrDupW(SubInfo.bstrUserName, pszUsername);
                            }
                        
                        }   
                        if(SubInfo.bstrPassword)
                            SysFreeString(SubInfo.bstrPassword);
                        if(SubInfo.bstrUserName)
                            SysFreeString(SubInfo.bstrUserName);
                        pSubsMgr->Release();
                    }
                }
                pActiveDesk->Release();
            }

            OleFree(pszURL);
        }
        
    }
    
    return S_OK;
}

// *** IServiceProvider ***
HRESULT CDocObjectHost::CDOHBindStatusCallback::QueryService(REFGUID guidService,
                            REFIID riid, void **ppvObj)
{
    HRESULT hres = E_FAIL;
    *ppvObj = NULL;

    if (IsEqualGUID(guidService, IID_IAuthenticate)) {
        return QueryInterface(riid, ppvObj);
    }
    else if (IsEqualGUID(guidService, IID_ITargetFrame2))
    {
        return IToClass(CDocObjectHost, _bsc, this)->QueryService(
                    guidService,
                    riid,
                    ppvObj);
    }
    else if (_pbscChained)
    {
        // Has a delegating IBindStatusCallback.
        IServiceProvider* psp;
        hres = _pbscChained->QueryInterface(IID_IServiceProvider, (LPVOID*)&psp);
        if (SUCCEEDED(hres)) {
            // It supports ServiceProvider, just delegate.
            hres = psp->QueryService(guidService, riid, ppvObj);
            psp->Release();
        } else if (IsEqualGUID(guidService, riid)) {
            // It does not supports ServiceProvide, try QI.
            hres = _pbscChained->QueryInterface(riid, ppvObj);
        }
    }

    return hres;
}

HRESULT CDocObjectHost::CDOHBindStatusCallback::OnStartBinding(
            DWORD grfBSCOption, IBinding *pib)
{

    BSCMSG(TEXT("OnStartBinding"), _pib, pib);
    CDocObjectHost* pdoh = IToClass(CDocObjectHost, _bsc, this);

    _fBinding = TRUE;
    _fDocWriteAbort = FALSE;
    _fBoundToMSHTML = FALSE;
    ASSERT(pdoh->_pocthf);
    pdoh->_fWriteHistory = FALSE;       // should we write history on this bind?
    pdoh->_fSelectHistory = FALSE;      // should we select history on this bind?

    if (pdoh->_pocthf)
    {
        MSOCMD rgCmd[2] = {{SBCMDID_WRITEHIST,0},{SBCMDID_SELECTHISTPIDL,0}};

        pdoh->_pocthf->QueryStatus(&CGID_Explorer, 2, rgCmd, NULL);
        if (rgCmd[0].cmdf & MSOCMDF_ENABLED) pdoh->_fWriteHistory = TRUE;
        if (rgCmd[1].cmdf & MSOCMDF_ENABLED) pdoh->_fSelectHistory = TRUE;
    }

    // ASSERT(_pib==NULL);
    ATOMICRELEASE(_pib);

    _pib = pib;
    if (_pib) {
        _pib->AddRef();
    }

#ifndef NO_DELEGATION
    if (_pbscChained) {
        CHAINMSG("OnStartBinding", grfBSCOption);
    _pbscChained->OnStartBinding(grfBSCOption, pib);
    }
#endif

    pdoh->_fShowProgressCtl = TRUE;
    pdoh->_PlaceProgressBar(TRUE);

    return S_OK;
}

HRESULT CDocObjectHost::CDOHBindStatusCallback::GetPriority(LONG *pnPriority)
{
    BSCMSG(TEXT("GetPriority"), 0, 0);
    *pnPriority = NORMAL_PRIORITY_CLASS;
#ifndef NO_DELEGATION
    if (_pbscChained) {
        _pbscChained->GetPriority(pnPriority);
    }
#endif
    return S_OK;
}

void CDocObjectHost::CDOHBindStatusCallback::_Redirect(LPCWSTR pwzNew)
{
    LPITEMIDLIST pidlNew;
    WCHAR wszPath[MAX_URL_STRING] = TEXT("");
    CDocObjectHost* pdoh = IToClass(CDocObjectHost, _bsc, this);
    LPOLESTR pwszCurrent = NULL;
    BOOL fAllow = FALSE;

    if (SUCCEEDED(IECreateFromPath(pwzNew, &pidlNew))) {
        TraceMsg(TF_SHDNAVIGATE, "CDOH::CBSC::_Redirect calling NotifyRedirect(%s)", pwzNew);
        pdoh->_pwb->NotifyRedirect(pdoh->_psv, pidlNew, NULL);
        // Save te redirected URL
        if (_pszRedirectedURL)
           LocalFree( _pszRedirectedURL );
        _pszRedirectedURL = StrDup(pwzNew);

        // We need to account for a bookmark that might appear
        // in the redirected URL.
        if(IEILGetFragment(pidlNew, wszPath, SIZECHARS(wszPath))) {
            LocalFree((LPVOID) pdoh->_pszLocation);
            pdoh->_pszLocation = StrDup(wszPath);
        }

        ILFree(pidlNew);
    }

    AddUrlToUrlHistoryStg(pwzNew, NULL, pdoh->_psb, FALSE,
                                NULL, NULL, NULL);

    // Security:  Release the pre-created object and start over for
    // server-side redirects.  The only security check for the
    // document reference occurs when someone tries to obtain it.
    // Therefore, we want to orphan the reference if x-domain, so the
    // client will need to obtain a new reference to the redirected
    // document.
    if (SUCCEEDED(pdoh->_GetCurrentPageW(&pwszCurrent, TRUE)))
    {
        fAllow = AccessAllowed(pwszCurrent, pwzNew);
        OleFree(pwszCurrent);
    }

    if (!fAllow)
        pdoh->_ReleasePendingObject(FALSE);
}


//
// In this function, we get the codepage for the current URL. If that's not
// CP_ACP, we pass it to Trident via IBindCtx*.
//
void CDocObjectHost::CDOHBindStatusCallback::_CheckForCodePageAndShortcut(void)
{
    CDocObjectHost* pdoh = IToClass(CDocObjectHost, _bsc, this);
    LPWSTR pwszURL;
    HRESULT hres = pdoh->_GetCurrentPageW(&pwszURL, TRUE);

    

    if (SUCCEEDED(hres)) {
        UINT codepage = CP_ACP;
        IOleCommandTarget *pcmdt;
        VARIANT varShortCutPath = {0};
        BOOL fHasShortcut = FALSE;
        hres = pdoh->QueryService(SID_SHlinkFrame, IID_IOleCommandTarget, (void **)&pcmdt);
        if(S_OK == hres) 
        {
           ASSERT(pcmdt);
           hres = pcmdt->Exec(&CGID_Explorer, SBCMDID_GETSHORTCUTPATH, 0, NULL, &varShortCutPath);

           //
           // App Compat:  Imagineer Technical returns S_OK for the above Exec
           // but of course doesn't set the output parameter.
           //
           if((S_OK) == hres && VT_BSTR == varShortCutPath.vt && varShortCutPath.bstrVal)
           {
               fHasShortcut = TRUE;
           }
           pcmdt->Release();
        }
        if(UrlHitsNetW(pwszURL))
        {
            // Don't do this for File: files - we can live
            // with getting the code page late for file: even
            // if it slows down file: display somewhat if the
            // trident parser needs to restarted
            AddUrlToUrlHistoryStg(pwszURL, NULL, pdoh->_psb, FALSE,
                                NULL, NULL, &codepage);
        }
        TraceMsg(DM_DOCCP, "CDOH::CBSC::_CheckForCodePageAndShortcut codepage=%d", codepage);

        if ((codepage != CP_ACP || fHasShortcut) && _pbc) {
            // Here is where we pass the codepage to Trident.
            IHtmlLoadOptions *phlo;
            HRESULT hres = CoCreateInstance(CLSID_HTMLLoadOptions,
                NULL, CLSCTX_INPROC_SERVER,
                IID_IHtmlLoadOptions, (void**)&phlo);

            if (SUCCEEDED(hres) && phlo)
            {
                if(codepage != CP_ACP)
                {
                    hres = phlo->SetOption(HTMLLOADOPTION_CODEPAGE, &codepage, sizeof(codepage));
                }
                if (SUCCEEDED(hres))
                {
                    if(fHasShortcut)
                    {
                        // deliberately ignore failures here
                        phlo->SetOption(HTMLLOADOPTION_INETSHORTCUTPATH, varShortCutPath.bstrVal, 
                                                (lstrlenW(varShortCutPath.bstrVal) + 1)*sizeof(WCHAR));
                    }
                    _pbc->RegisterObjectParam(L"__HTMLLOADOPTIONS", phlo);
                }
                phlo->Release();
            } else {
                TraceMsg(DM_WARNING, "DOH::_CheckForCodePagecut CoCreateInst failed (%x)", hres);
            }
        }
        VariantClear(&varShortCutPath);
        OleFree(pwszURL);
    }
}

#ifdef BETA1_DIALMON_HACK
extern void IndicateWinsockActivity();
#endif // BETA1_DIALMON_HACK


HRESULT CDocObjectHost::CDOHBindStatusCallback::OnProgress(
     ULONG ulProgress,
     ULONG ulProgressMax,
     ULONG ulStatusCode,
     LPCWSTR pwzStatusText)
{
    HRESULT hr = S_OK;
    
    TraceMsg(TF_SHDPROGRESS, "DOH::BSC::OnProgress (%d of %d) ulStatus=%x",
             ulProgress, ulProgressMax, ulStatusCode);

        //BUGBUG JEFFWE 4/15/96 Beta 1 Hack - every once in a while, send message
        //BUGBUG to the hidden window that detects inactivity so that it doesn't
        //BUGBUG think we are inactive during a long download
#ifdef BETA1_DIALMON_HACK
        IndicateWinsockActivity();
#endif


    CDocObjectHost* pdoh = IToClass(CDocObjectHost, _bsc, this);
#ifdef DEBUG
    if (pwzStatusText) {
        char szStatusText[MAX_PATH];    // OK with MAX_PATH
        UnicodeToAnsi(pwzStatusText, szStatusText, ARRAYSIZE(szStatusText));
        TraceMsg(TF_SHDPROGRESS, "DOH::BSC::OnProgress pszStatus=%s", szStatusText);
    }
#endif

    if (pdoh->_psb)
    {
        // we may be switching between multiple proxy/server hosts, so don't prevent
        //  showing them when they change
        if (_bindst != ulStatusCode ||
            ulStatusCode == BINDSTATUS_FINDINGRESOURCE)
        {
            UINT idRes = IDI_STATE_NORMAL;
            _bindst = ulStatusCode;

            if (_bindst < ARRAYSIZE(c_aidRes)) 
                idRes = c_aidRes[_bindst];

            pdoh->_psb->SendControlMsg(FCW_STATUS, SB_SETICON, STATUS_PANE_NAVIGATION, 
                                        (LPARAM)g_ahiconState[idRes-IDI_STATE_FIRST], NULL);

            TCHAR szStatusText[MAX_PATH];        // OK with MAX_PATH
            if (pwzStatusText) {
                StrCpyN(szStatusText, pwzStatusText, ARRAYSIZE(szStatusText));
            } else {
                szStatusText[0] = TEXT('\0');
            }

            //
            // This if-block will open the safe open dialog for OLE Object
            // and DocObject.
            //
            if (_bindst == BINDSTATUS_CLASSIDAVAILABLE) {
                TraceMsg(TF_SHDPROGRESS, "DOH::BSC::OnProgress got CLSID=%ws", szStatusText);
                CLSID clsid;
                // WORK-AROUND: CLSIDFromString does not take LPCOLESTR correctly.
                HRESULT hresT = CLSIDFromString((LPOLESTR)pwzStatusText, &clsid);
                if (SUCCEEDED(hresT)) {
#ifdef DEBUG
                    if (IsEqualGUID(clsid, CLSID_NULL)) {
                        TraceMsg(DM_WARNING, "DOH::SBC::OnProgress Got CLSID_NULL");
                    }
#endif
                    //
                    //  Notice that we don't want to use BROWSERFLAG_MSHTML,
                    // which includes other types of MSHMTL CLSIDs.
                    // In this case, we just want to deal with HTMLDocument.
                    // (We allow XMLViewer docobj and *.MHT and *.MHTML too!)
                    BOOL fIsHTML = (IsEqualGUID(clsid, CLSID_HTMLDocument) || 
                                    IsEqualGUID(clsid, CLSID_XMLViewerDocObj) ||
                                    IsEqualGUID(clsid, CLSID_MHTMLDocument));
                    BOOL fAbortDesktopComponent = FALSE;

                    if(!fIsHTML)
                    {
                        //Check if we are a desktop component.
                        if (_IsDesktopItem(pdoh))
                        {
                            //Because this is NOT html, then don't show it!
                            fAbortDesktopComponent = TRUE;
                        }
                    }

                    if(fAbortDesktopComponent)
                    {
                        AbortBinding();
                        hr = E_ABORT;
                    }
                    else
                    {
                        _fBoundToMSHTML = fIsHTML; // Remember this and suppress redundant
                                               // AddUrl to history

                        //  There is an interval of time between OnProgress and OnObjectAvailable
                        //  in which the om might be required.
                        if (fIsHTML && pdoh->_punkPending == NULL)
                        {
                            pdoh->_CreatePendingDocObject(FALSE);
                        }
                        if (pdoh->_punkPending)
                        {
                            IPersist *pip;

                            hresT = pdoh->_punkPending->QueryInterface(IID_IPersist, (LPVOID *) &pip);
                            if (SUCCEEDED(hresT))
                            {
                                CLSID clsidPending;

                                hresT = pip->GetClassID(&clsidPending);
                                if (SUCCEEDED(hresT) && IsEqualGUID(clsid, clsidPending))
                                {
                                    _pbc->RegisterObjectParam(L"__PrecreatedObject", pdoh->_punkPending);
                                }
                                pip->Release();
                            }
                        }

                        hresT = pdoh->_MayHaveVirus(clsid);
                        if (hresT == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
                            hr = E_ABORT;
                            AbortBinding();
                        }

                    }
                } else {
                    TraceMsg(DM_ERROR, "DOH::BSC::OnProgress CLSIDFromString failed %x", hresT);
                }

                //
                //  Notice that URLMON will call IPersistMoniker::Load right
                // after we return from this notification. Therefore, this
                // is the latest moment we have a chance to pass the code
                // page to Trident.
                //              
                _CheckForCodePageAndShortcut();
            } else if (_bindst == BINDSTATUS_CACHEFILENAMEAVAILABLE) {
                TraceMsg(DM_SELFASC, "DOH::OnProgress got BINDSTATUS_CACHEFILENAMEAVAILABLE");
                _fSelfAssociated = IsAssociatedWithIE(pwzStatusText);
            } else if (_bindst == BINDSTATUS_CONTENTDISPOSITIONATTACH)
            {
                TCHAR szURL[MAX_URL_STRING];
                TCHAR * pszURL = szURL;
                HRESULT hresT;
        
                hresT = pdoh->_GetCurrentPage(szURL, ARRAYSIZE(szURL), TRUE);
                if (SUCCEEDED(hresT)) 
                {
                    UINT uRet;

                    if (_pszRedirectedURL && lstrlen(_pszRedirectedURL))
                        pszURL = _pszRedirectedURL;

                    uRet = OpenSafeOpenDialog(pdoh->_hwnd, DLG_SAFEOPEN, NULL, pszURL, NULL, NULL, NULL, pdoh->_uiCP);
            
                    switch(uRet) 
                    {
                        case IDOK:
                            //
                            // Set this flag to avoid poppping this dialog box twice.
                            // 
                            pdoh->_fConfirmed = TRUE;
                            break;  // continue download
                        case IDD_SAVEAS:
                            CDownLoad_OpenUI(pdoh->_pmkCur, _pbc, FALSE, TRUE, NULL, NULL, NULL, NULL, NULL, _pszRedirectedURL, pdoh->_uiCP);
                            // fall thru to AbortBinding
                        case IDCANCEL:
                            pdoh->_CancelPendingNavigation(FALSE);
                            AbortBinding();
                            break;
                    }
                }
            }
            
            if ((_bindst >= BINDSTATUS_FINDINGRESOURCE
                && _bindst <= BINDSTATUS_SENDINGREQUEST) ||
                _bindst == BINDSTATUS_PROXYDETECTING)
            {
                TCHAR szTemplate[MAX_PATH];              // OK with MAX_PATH
                UINT idResource = IDS_BINDSTATUS+_bindst;

                if ( _bindst == BINDSTATUS_PROXYDETECTING ) {
                    idResource = IDS_BINDSTATUS_PROXYDETECTING;
                }

                // If we are connecting over proxy, don't say "web site found".
                if (fOnProxy() && idResource == IDS_BINDSTATUS_SEND) {
                    idResource = IDS_BINDSTATUS_CON;
                    TCHAR szUrl[MAX_URL_STRING];
                    pdoh->_GetCurrentPage(szUrl, SIZECHARS(szUrl));
                    DWORD cchStatusText = SIZECHARS(szStatusText);

                    UrlGetPart(szUrl, szStatusText, &cchStatusText, URL_PART_HOSTNAME, 0);
                }

                if (MLLoadString(idResource, szTemplate, ARRAYSIZE(szTemplate)))
                {
                    BSCMSGS("OnProgress szTemplate=", szTemplate);
                    TCHAR szMessage[MAX_PATH];          // OK with MAX_PATH
                    BOOL fSuccess = wnsprintf(szMessage, ARRAYSIZE(szMessage), szTemplate, szStatusText);
                    //BOOL fSuccess = _FormatMessage(szTemplate, szMessage, ARRAYSIZE(szMessage), szStatusText);

                    if (fSuccess)
                    {
                        
                        BSCMSGS("OnProgress szMessage=", szMessage);
                        pdoh->_SetStatusText(szMessage);
                    }
                }
            }
        }

        DWORD dwState = 0;

        switch (ulStatusCode)
        {
        case BINDSTATUS_REDIRECTING:
            // they're redirecting.  treat this as a rename.
            _Redirect(pwzStatusText);
            break;
        
        case BINDSTATUS_FINDINGRESOURCE:
            dwState = PROGRESS_FINDING;
            ASSERT(!ulProgressMax);
            break;

        case BINDSTATUS_SENDINGREQUEST:
            dwState = PROGRESS_SENDING;
            ASSERT(!ulProgressMax);
            break;

        }

        if(dwState)
            pdoh->_OnSetProgressPos(ulProgress, dwState);

        if (BINDSTATUS_BEGINDOWNLOADDATA == ulStatusCode)
            _cbContentLength = ulProgress;
    }

#ifndef NO_DELEGATION
    if (_pbscChained) {
        _pbscChained->OnProgress(ulProgress, ulProgressMax, ulStatusCode, pwzStatusText);
    }
#endif
    return hr;
}

HRESULT CDocObjectHost::CDOHBindStatusCallback::OnDataAvailable(
            /* [in] */ DWORD grfBSC,
            /* [in] */ DWORD dwSize,
            /* [in] */ FORMATETC *pformatetc,
            /* [in] */ STGMEDIUM *pstgmed)
{
    BSCMSG(TEXT("OnDataAvailable (grf,pstg)"), grfBSC, pstgmed);
    CDocObjectHost* pdoh = IToClass(CDocObjectHost, _bsc, this);
#ifndef NO_DELEGATION
    if (_pbscChained) {
        _pbscChained->OnDataAvailable(grfBSC, dwSize, pformatetc, pstgmed);
    }
#endif
    return S_OK;
}

void CDocObjectHost::CDOHBindStatusCallback::_UpdateSSLIcon(void)
{
    CDocObjectHost* pdoh = IToClass(CDocObjectHost, _bsc, this);
    ASSERT(_pib);
    //
    //  if we have already been set by our object, we dont 
    //  want to override it.
    if (_pib  && !pdoh->_fSetSecureLock) 
    {
        pdoh->_eSecureLock = SECURELOCK_SET_UNSECURE;

        IWinInetInfo* pwinet;
        HRESULT hresT = _pib->QueryInterface(IID_IWinInetInfo, (LPVOID*)&pwinet);
        if (SUCCEEDED(hresT)) {
            DWORD dwOptions = 0;
            DWORD cbSize = SIZEOF(dwOptions);
            
            hresT = pwinet->QueryOption(INTERNET_OPTION_SECURITY_FLAGS,
                                (LPVOID)&dwOptions, &cbSize);
            TraceMsg(DM_SSL, "pwinet->QueryOptions hres=%x dwOptions=%x", hresT, dwOptions);

            if (SUCCEEDED(hresT))
            {
                LPWSTR pwzUrl;

                pdoh->_fSetSecureLock = TRUE;
                if(dwOptions & SECURITY_FLAG_SECURE)
                {
                    pdoh->_dwSecurityStatus = dwOptions;
                    if (pdoh->_dwSecurityStatus & SECURITY_FLAG_40BIT)
                        pdoh->_eSecureLock = SECURELOCK_SET_SECURE40BIT;
                    else if (pdoh->_dwSecurityStatus & SECURITY_FLAG_128BIT)
                        pdoh->_eSecureLock = SECURELOCK_SET_SECURE128BIT;
                    else if (pdoh->_dwSecurityStatus & SECURITY_FLAG_FORTEZZA)
                        pdoh->_eSecureLock = SECURELOCK_SET_FORTEZZA;
                    else if (pdoh->_dwSecurityStatus & SECURITY_FLAG_56BIT)
                        pdoh->_eSecureLock = SECURELOCK_SET_SECURE56BIT;
                }
                else if (SUCCEEDED(_GetRequestFlagFromPIB(_pib, &dwOptions)) && 
                    (dwOptions & INTERNET_REQFLAG_FROM_CACHE) && 
                    SUCCEEDED(pdoh->_GetCurrentPageW(&pwzUrl, TRUE)))
                {
                    // 
                    //  when secure pages are cached, they lose their
                    //  security context, but should still be displayed
                    //  as secure.  therefore we use the UnknownBit level
                    //  of security.
                    //
                    if(URL_SCHEME_HTTPS == GetUrlSchemeW(pwzUrl))
                        pdoh->_eSecureLock = SECURELOCK_SET_SECUREUNKNOWNBIT;

                    OleFree(pwzUrl);
                }
            }
            else 
                pdoh->_dwSecurityStatus = 0;

            //  we will update the browser when we are activated

            pwinet->Release();
        } else {
            TraceMsg(DM_SSL, "QI to IWinInetInfo failed");
        }
        TraceMsg(DM_SSL, "[%X] UpdateSslIcon() setting _eSecureLock = %d", pdoh, pdoh->_eSecureLock);
    }
    else
        TraceMsg(DM_SSL, "[%X] UpdateSslIcon() already set _eSecureLock = %d", pdoh, pdoh->_eSecureLock);
}

HRESULT CDocObjectHost::CDOHBindStatusCallback::OnObjectAvailable(
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown *punk)
{
    BSCMSG(TEXT("OnObjectAvailable (riid,punk)"), riid, punk);
    CDocObjectHost* pdoh = IToClass(CDocObjectHost, _bsc, this);

#ifdef DEBUG
    extern DWORD g_dwPerf;
    PERFMSG(TEXT("OnObjectAvailable called"), GetCurrentTime()-g_dwPerf);
    g_dwPerf = GetCurrentTime();
#endif
    
    {
    //  Suppress writing to history if this request is secured and consequently
    //  cannot be written to cache
        DWORD dwOptions;

        if (SUCCEEDED(_GetRequestFlagFromPIB(_pib, &dwOptions)) && (dwOptions & INTERNET_REQFLAG_CACHE_WRITE_DISABLED)) {
            pdoh->_fWriteHistory = FALSE;
        }
    }


    //  If we get this far, DocObject has been inited by UrlMon or
    //  in process of retrieving pending object via IOleCommandTarget::Exec()
    if (pdoh->_punkPending)
    {
        pdoh->_fPendingNeedsInit = 0;
    }

    //
    //  When this notification is called first time, we should ask
    // the browser to activate us (which causes BindToObject).
    //
    if (pdoh->_pole==NULL && punk)
    {
        HRESULT hresT = punk->QueryInterface(IID_IOleObject, (LPVOID*)&(pdoh->_pole));
        if (SUCCEEDED(hresT)) {
            IOleDocument* pmsod = NULL;

            pdoh->_OnBound(S_OK);

            hresT = (pdoh->_fDontInPlaceNavigate() ? E_NOINTERFACE : punk->QueryInterface(IID_IOleDocument, (LPVOID*)&pmsod));
            if (SUCCEEDED(hresT)) {
                pmsod->Release();       // We don't use it at this point.

                // Case 1: DocObject
                OPENMSG(TEXT("OnObjectAvailable ASYNC DocObject"));

                ASSERT(pdoh->_psb);

#ifdef FEATURE_PICS
                BOOL fSupportsPICS = FALSE;
                if (pdoh->_fbPicsWaitFlags) {
                    VARIANTARG v;
                    v.vt = VT_UNKNOWN;
                    v.byref = (LPVOID)(IOleCommandTarget *)&pdoh->_ctPics;

                    hresT = IUnknown_Exec(pdoh->_pole, &CGID_ShellDocView, SHDVID_CANSUPPORTPICS, 0, &v, NULL);
                    if (hresT == S_OK) {
                        BSCMSG(TEXT("OnObjectAvailable - obj supports PICS"), 0, 0);
                        fSupportsPICS = TRUE;
                    }
                    else {
                        BSCMSG(TEXT("OnObjectAvailable - obj either doesn't support IOleCommandTarget or doesn't support PICS"), hresT, 0);
                    }
                }
#endif
                BSCMSG(TEXT("OnObjectAvailable calling pdoh->_Navigate"), 0, 0);

                pdoh->_SetUpTransitionCapability();
                
                _UpdateSSLIcon();
#ifdef FEATURE_PICS
                /* If we can't get labels out of the document (or don't need
                 * to, because we already got one from a bureau or HTTP header),
                 * see if we can complete PICS checking now.
                 */
                if (!fSupportsPICS) {
                    pdoh->_fbPicsWaitFlags &= ~(PICS_WAIT_FOR_INDOC | PICS_WAIT_FOR_END);   /* no indoc ratings */
                    if (!pdoh->_fbPicsWaitFlags) {
                        TraceMsg(DM_PICS, "OnObjectAvailable calling _HandlePicsChecksComplete");
                        pdoh->_HandlePicsChecksComplete();
                    }
                }
#endif
            } else {
                // Case 2: OLE object

                OPENMSG(TEXT("OnDataAvailable ASYNC OLE Object"));
                pdoh->_ActivateOleObject();
                
                // We need to tell the browser not to add this one to the
                // browse history.
                // We also want to close the browser window if this is the first
                // download - that's why we pass TRUE - to treat it like a code
                // download
                pdoh->_CancelPendingNavigation((pdoh->_dwAppHack&BROWSERFLAG_DONTAUTOCLOSE)?FALSE:TRUE);

                //
                // If this is the very first page, we should draw the background.
                //
                pdoh->_fDrawBackground = TRUE;
                //If the following assert is hit, then that means that we are
                // going to invalidate the desktop window (which is not
                // intended here)
                //
                ASSERT(pdoh->_hwnd);
                InvalidateRect(pdoh->_hwnd, NULL, TRUE);
            }

        } else {
            _fBoundToNoOleObject = TRUE;
        }
    }

#ifndef NO_DELEGATION
    if (_pbscChained) {
        _pbscChained->OnObjectAvailable(riid, punk);
    }
#endif
    return S_OK;
}

HRESULT CDocObjectHost::CDOHBindStatusCallback::OnLowResource(DWORD reserved)
{
    BSCMSG(TEXT("OnLowResource"), 0, 0);

#ifndef NO_DELEGATION
    if (_pbscChained) {
        _pbscChained->OnLowResource(reserved);
    }
#endif
    return S_OK;
}

HRESULT CDocObjectHost::CDOHBindStatusCallback::BeginningTransaction(LPCWSTR szURL, LPCWSTR szHeaders,
                DWORD dwReserved, LPWSTR __RPC_FAR * ppwzAdditionalHeaders)
{
    HRESULT hres;

#ifndef NO_DELEGATION
    if (_pnegotiateChained) {
        hres = _pnegotiateChained->BeginningTransaction(szURL, szHeaders, dwReserved, ppwzAdditionalHeaders);
    }
    else
    {
#endif
        //  Here we pass headers to URLMon

        hres=BuildAdditionalHeaders((LPCTSTR) _pszHeaders,(LPCWSTR *) ppwzAdditionalHeaders);

#ifndef NO_DELEGATION
    }
#endif
    return hres;
}


const WCHAR g_wszPicsLabel[] = L"\r\nPICS-Label:";

HRESULT CDocObjectHost::CDOHBindStatusCallback::OnResponse(DWORD dwResponseCode, LPCWSTR szResponseHeaders,
                            LPCWSTR szRequestHeaders,
                            LPWSTR *pszAdditionalRequestHeaders)
{
#ifndef NO_DELEGATION
    if (_pnegotiateChained) {
        _pnegotiateChained->OnResponse(dwResponseCode, szResponseHeaders, szRequestHeaders, pszAdditionalRequestHeaders);
    }
    else
    {
#endif

#ifndef NO_DELEGATION
    }
#endif

#ifdef FEATURE_PICS
    /* CODEWORK: For next release, all response headers should be handled
     * generically through _OnHttpEquiv, and rating labels should be
     * processed there instead of through a private IOleCommandTarget
     * interface with Trident.
     */

    /* NOTE: We still need to check for the PICS label header here, even
     * if we chained to Trident or whoever above.
     */
    CDocObjectHost* pdoh = IToClass(CDocObjectHost, _bsc, this);
    if (pdoh->_fbPicsWaitFlags & PICS_WAIT_FOR_INDOC) {
        LPCWSTR pwszPicsLabel = StrStrW(szResponseHeaders, g_wszPicsLabel);
        if (pwszPicsLabel != NULL) {
            pdoh->_dwPicsLabelSource=PICS_LABEL_FROM_HEADER;
            pwszPicsLabel += ARRAYSIZE(g_wszPicsLabel); /* skip \r\n and label name */
            LPCWSTR pwszPicsLabelEnd = StrChrW(pwszPicsLabel, L'\r');
            if (pwszPicsLabelEnd == NULL) {
                // NOTE: lstrlenW doesn't work on Win95, so we do this manually.
                for (pwszPicsLabelEnd = pwszPicsLabel;
                     *pwszPicsLabelEnd;
                     pwszPicsLabelEnd++)
                    ;
            }
            if (pwszPicsLabel && (pwszPicsLabelEnd > pwszPicsLabel))
            {
                WCHAR* pszLabel = new WCHAR[((int)(pwszPicsLabelEnd - pwszPicsLabel)) + 1];

                if (pszLabel)
                {
                    //
                    // pwszPicsLabel may not be NULL terminated so use memcpy to
                    // move it.  Memory allocated by new is zero filled so
                    // pszLabel doesn't have to have L'\0' appeneded.
                    //
                    memcpy(pszLabel, pwszPicsLabel,
                           ((int)(pwszPicsLabelEnd - pwszPicsLabel)) * sizeof(WCHAR));
                    pdoh->_HandleInDocumentLabel(pszLabel);

                    delete pszLabel;
                }
            }
        }
        else
        {
            pdoh->_dwPicsLabelSource=PICS_LABEL_FROM_PAGE;
        }
    }
#endif

    return S_OK;
}

HRESULT CDocObjectHost::CDOHBindStatusCallback::GetWindow(REFGUID rguidReason, HWND* phwnd)
{
    CDocObjectHost* pdoh = IToClass(CDocObjectHost, _bsc, this);

    if (!phwnd)
        return E_POINTER;

    if (pdoh->_psb) {
        pdoh->_psb->GetWindow(phwnd);
    } else {
        *phwnd = pdoh->_hwnd;
    }

    return S_OK;
}

HRESULT CDocObjectHost::CDOHBindStatusCallback::OnSecurityProblem(DWORD dwProblem)
{
    // force UI - return S_FALSE for all problems
    return S_FALSE;
}


HRESULT CDocObjectHost::CDOHBindStatusCallback::_HandleSelfAssociate(void)
{
    CDocObjectHost* pdoh = IToClass(CDocObjectHost, _bsc, this);
    HRESULT hres;
    IPersistMoniker* ppmk;
    hres = pdoh->_CoCreateHTMLDocument(IID_IPersistMoniker, (LPVOID*)&ppmk);

    if (SUCCEEDED(hres)) {
        BIND_OPTS bindopts;
        bindopts.cbStruct = sizeof(BIND_OPTS);
        hres = _pbc->GetBindOptions(&bindopts);
        if (SUCCEEDED(hres)) {
            hres = ppmk->Load(FALSE, pdoh->_pmkCur, _pbc, bindopts.grfMode);
            if (SUCCEEDED(hres)) {
                ASSERT(NULL==pdoh->_pole);
                hres = ppmk->QueryInterface(IID_IOleObject, (LPVOID*)&pdoh->_pole);
                if (SUCCEEDED(hres)) {
                    pdoh->_InitOleObject();

                    TraceMsg(DM_SELFASC, "DOH::_HandleSelfAssociate self-association is working");
                    pdoh->_Navigate();
                    pdoh->_SetUpTransitionCapability();
                    _UpdateSSLIcon();
                } else {
                    TraceMsg(DM_WARNING, "DOH::_HandleSelfAssociate ppmk->QI(IOleObject) failed (%x)", hres);
                }
            } else {
                TraceMsg(DM_WARNING, "DOH::_HandleSelfAssociate ppmk->Load failed (%x)", hres);
            }
        } else {
            TraceMsg(DM_WARNING, "DOH::_HandleSelfAssociate _pbc->GetBindOptions failed (%x)", hres);
        }
        ppmk->Release();
    } else {
        TraceMsg(DM_WARNING, "DOH::_HandleSelfAssociate CoCreateInst failed (%x)", hres);
    }

    return hres;
}


#define BUG_EXEC_ON_FAILURE     // BUGBUG nash:31526

HRESULT CDocObjectHost::CDOHBindStatusCallback::OnStopBinding(HRESULT hrError,
            LPCWSTR szError)
{
    BSCMSG(TEXT("OnStopBinding"), this, hrError);
    _fBinding = FALSE;
    CDocObjectHost* pdoh = IToClass(CDocObjectHost, _bsc, this);
    LPWSTR   pwzHeaders = NULL;
    BOOL     fShouldDisplayError = TRUE;
    DWORD    dwStatusCode = 0;       // We use 0 to mean no status yet
    DWORD    dwStatusCodeSize = sizeof(dwStatusCode);
    BOOL     bSuppressUI = FALSE;
    BOOL     fAsyncDownload = FALSE;

    //
    //  this is to protect against urlmons behavior of returning 
    //  an async error and sync error on the same call.
    if (pdoh->_fSyncBindToObject && FAILED(hrError))
    {
        pdoh->_hrOnStopBinding = hrError;
        return S_OK;
    }

    // if aborting to let Document.Write work...pretend everything is cool
    if (_fDocWriteAbort && hrError == E_ABORT) hrError = S_OK;

    // Why not use the cached value?
    // pdoh->_GetOfflineSilent(0, &bSuppressUI);
    bSuppressUI = (_bFrameIsSilent || _IsDesktopItem(pdoh)) ? TRUE : FALSE;


    _bindst = 0;    // go back to the normal state

    if (_pbc && pdoh->_punkPending)
    {
        _pbc->RevokeObjectParam(L"__PrecreatedObject");
    }

    if (!_pbc) {
        ASSERT(0);
        return S_OK;
    }

    // NOTES: Guard against last Release by _RevokeObjectParam
    AddRef();

    if (pdoh->_pwb)
        pdoh->_pwb->SetNavigateState(BNS_NORMAL);

    if (pdoh->_psb) {   // paranoia
        pdoh->_psb->SetStatusTextSB(NULL);
    }

    BSCMSG("OnStopBinding calling _RevokeObjectParam", this, _pbc);
    _RevokeObjectParam(_pbc);
    _pbc->RevokeObjectParam(WSZGUID_OPID_DocObjClientSite);

    //
    //  If the error code is a mapped error code (by URLMON), get the
    // real error code from IBinding for display purpose.
    //
    HRESULT hrDisplay = hrError;    // assume they are the same

#define ENABLE_WHEN_GETBINDRESULT_STARTS_WORKING
#ifdef ENABLE_WHEN_GETBINDRESULT_STARTS_WORKING
    if (hrError>=INET_E_ERROR_FIRST && hrError<=INET_E_ERROR_LAST) {
        //
        //  We come here when _pib==NULL, if URLMON synchronously fails
        // (such as a bad protocol).
        //
        // ASSERT(_pib);
        //
        if (_pib) {
            CLSID clsid;
            LPWSTR pwszError = NULL;

            HRESULT hresT=_pib->GetBindResult(&clsid, (DWORD *)&hrDisplay, &pwszError, NULL);
            TraceMsg(TF_SHDBINDING, "DOH::OnStopBinding called GetBindResult %x->%x (%x)", hrError, hrDisplay, hresT);
            if (SUCCEEDED(hresT)) {
                //
                // BUGBUG: URLMON returns a native Win32 error.
                //
                if (hrDisplay && SUCCEEDED(hrDisplay)) {
                    hrDisplay = HRESULT_FROM_WIN32(hrDisplay);
                }

                //
                // URLMON is not supposed to return 0 as the error code,
                //  which causes a silly "successfully done" error msgbox. 
                //
                AssertMsg(hrDisplay != S_OK, TEXT("Call JohannP if you see this assert."));

                if (pwszError) {
                    OleFree(pwszError);
                }
            }
        }
    }
#endif

    TraceMsg(TF_SHDBINDING, "DOH::BSC::OnStopBinding binding failed %x (hrDisplay=%x)", hrError, hrDisplay);

    //
    // HACK: If the object is associated with IE/Shell itself, but has
    //  no CLSID, we'll force MSHTML.
    //
    if (_fSelfAssociated && (hrError==MK_E_INVALIDEXTENSION || hrError==REGDB_E_CLASSNOTREG)) {
        hrError = _HandleSelfAssociate();
    }

    if (_pib) {

        //  we dont need to do the expiry stuff here anymore.
        //  now mshtml should be doing it through the IPersistHistory

        // get the expire info
        // The HTTP rules for expiration are
        // Expires: 0               expire immediately
        // if Expires: <= Date:     expire immediately
        // if Expires: bad format   expire immediately

        IWinInetHttpInfo *phi;
        if (SUCCEEDED(_pib->QueryInterface(IID_IWinInetHttpInfo, (LPVOID*)&phi))) {
            BYTE  abBuffer[256]; // We don't care about this data, just
            DWORD cbBuffer=sizeof(abBuffer); // whether it exists or not

            if (phi->QueryInfo(HTTP_QUERY_LAST_MODIFIED, &abBuffer, &cbBuffer, NULL, 0) == S_OK)
                pdoh->_fhasLastModified = TRUE;

            if (phi->QueryInfo(HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwStatusCode, &dwStatusCodeSize, NULL, 0) != S_OK) {
                dwStatusCode = 0;       // failed to get status code
                dwStatusCodeSize = 0;   // failed to get status code
            }

            // This code will decide if we should display a popup error;
            // essentially, it detects if we can reasonably assume that
            // HTML was returned in the error case; if so, we believe that
            // it is an error page, so we let that display rather than a
            // popup.

            if (dwStatusCode) {
                 // We got a status code; let's see if we have a
                 // content-type.

                 // HTTP retcode 204 is a "succeeded, do nothing" retcode
                 // So we should always suppress the popup; further, it is
                 // spec'd to NEVER have content, so we do this before checking
                 // for content-type.
                 // So is 100
                 // BUGBUG: 100 is not in wininet.h
                 if (dwStatusCode == HTTP_STATUS_NO_CONTENT)
                     fShouldDisplayError = FALSE;

                 // BUGBUG: what is max header size?
                 CHAR  szContentType[1024];
                 DWORD dwContentTypeSize = sizeof(szContentType);

                 // This code handles a bug in URLMON where it tells us 
                 // INET_E_DATA_NOT_AVAILABLE when in fact the
                 // data _was_ available.  We don't want any future 
                 // errors affected by this, so we restrict this
                 // hack to less than 600, and ONLY for the 
                 // INET_E_DATA_NOT_AVAILABLE case.

                 if (hrError == INET_E_DATA_NOT_AVAILABLE && 
                     dwStatusCode < 600 &&
                     phi->QueryInfo(HTTP_QUERY_CONTENT_TYPE, &szContentType, 
                                    &dwContentTypeSize, NULL, 0) == S_OK)
                     fShouldDisplayError = FALSE;

                //
                // Handle http errors.
                //

                if (dwStatusCode >= 400 && dwStatusCode <= 599)
                {
                    _HandleHttpErrors(dwStatusCode, _cbContentLength, pdoh);
                }
            }

            phi->Release();
        }

        ATOMICRELEASE(_pib);
    }

    ATOMICRELEASE(_psvPrev);

    //
    //  If the object does not support IOleObject, treat it as if we failed
    // to bind.
    //
    if (_fBoundToNoOleObject) {
        ASSERT(SUCCEEDED(hrError));
        hrError = MK_E_INVALIDEXTENSION;
    }

    // bugbug:: need to handle navigation in successful proxy response but w/
    // 404 error.  tonyci 13nov96.  for autosearching & autosuffixing

    if (FAILED(hrError)) 
    {
        BOOL fAddToMRU = FALSE;
        pdoh->_fDrawBackground = TRUE;

        TCHAR szURL[MAX_URL_STRING+1];
        szURL[0] = TEXT('\0');

        if (pdoh->_pmkCur)
            pdoh->_GetCurrentPage(szURL,ARRAYSIZE(szURL));

        TraceMsg(TF_SHDBINDING, "DOH::OnStopBinding hrError=%x", hrError);
        
        pdoh->_OnSetProgressPos(0, PROGRESS_RESET);

        switch(hrError) {
        //
        //  If pmk->BindToObject is failed because of "binding", we should
        // offer an option to download it as a file.
        //

#ifdef BUG_EXEC_ON_FAILURE
        case INET_E_CANNOT_INSTANTIATE_OBJECT:
            TraceMsg(TF_SHDBINDING, "DOH::OnStopBinding IDS_ERR_OLESVR");
            _SetSearchInfo(pdoh, 0, FALSE, FALSE, FALSE);  // reset info
            goto Lexec;
        case INET_E_CANNOT_LOAD_DATA:
            TraceMsg(TF_SHDBINDING, "DOH::OnStopBinding IDS_ERR_LOAD");
            _SetSearchInfo(pdoh, 0, FALSE, FALSE, FALSE);  // reset info
            goto Lexec;
#else
        case INET_E_CANNOT_INSTANTIATE_OBJECT:
            _SetSearchInfo(pdoh, 0, FALSE, FALSE, FALSE);  // reset info
            if (MLShellMessageBox(pdoh->_hwnd,
                            MAKEINTRESOURCE(IDS_ERR_OLESVR),
                            MAKEINTRESOURCE(IDS_TITLE),
                            MB_YESNO|MB_ICONERROR,
                            szURL) == IDYES) {
                CDownLoad_OpenUI(pdoh->_pmkCur, _pbc, FALSE, TRUE, NULL, NULL, NULL, NULL, NULL, _pszRedirectedURL, pdoh->_uiCP);
            }
            break;

        case INET_E_CANNOT_LOAD_DATA:
            _SetSearchInfo(pdoh, 0, FALSE, FALSE, FALSE);  // reset info
            // e.g. click on .xls link when doc already open/modified/locked
            // and say 'cancel'
            if (MLShellMessageBox(pdoh->_hwnd,
                            MAKEINTRESOURCE(IDS_ERR_LOAD),
                            MAKEINTRESOURCE(IDS_TITLE),
                            MB_YESNO|MB_ICONERROR,
                            szURL) == IDYES) {
                CDownLoad_OpenUI(pdoh->_pmkCur, _pbc, FALSE, TRUE, NULL, NULL, NULL, NULL, _pszRedirectedURL, pdoh->_uiCP);
            }
            break;
#endif

        //
        // NOTES: According to JohannP, URLMON will give us
        //  REGDB_E_CLASSNOTREG. I'll leave MK_E_INVALIDEXTENSION
        //  to be compatible with old URLMON (which is harmless).
        //
        case MK_E_INVALIDEXTENSION:
        case REGDB_E_CLASSNOTREG:
            _SetSearchInfo(pdoh, 0, FALSE, FALSE, FALSE);  // reset info

#ifdef BUG_EXEC_ON_FAILURE
    Lexec: // BUGBUG nash:31526
            // for various instantiation errors:
            // - for ie3 we suppress messages and force a ShellExec as a
            // 2nd try, pretty much always
            // - for ie4 we should be more selective (BUGBUG nash:31526)
#endif

#ifdef FEATURE_PICS
            /* For data types that don't have a CLSID, we never get a chance
             * to block in the CLASSIDAVAILABLE OnProgress notification, so
             * we have to block here.  However, avoid blocking documents such
             * as HTML which we want to download completely so we can get
             * ratings strings out of them.
             */
            if (!pdoh->_fPicsBlockLate && (pdoh->_fbPicsWaitFlags || !pdoh->_fPicsAccessAllowed)) {
                pdoh->_fbPicsWaitFlags &= ~(PICS_WAIT_FOR_INDOC | PICS_WAIT_FOR_END);   /* make sure we don't expect indoc ratings */
                TraceMsg(DM_PICS, "OnStopBinding calling _PicsBlockingDialog, waitflags now %x", (DWORD)pdoh->_fbPicsWaitFlags);
                if (pdoh->_PicsBlockingDialog(szURL) != IDOK) {
                    TraceMsg(DM_PICS, "OnStopBinding, PICS canceled, calling _CancelPendingNavigation");
                    pdoh->_CancelPendingNavigation(FALSE);
                    break;
                }
            }
#endif

            BeginningTransaction (NULL, NULL, 0, &pwzHeaders);

            if (_dwBindVerb==BINDVERB_POST) {
                // This is a POST. Do it use the same moniker (modeless)
                //
                // Notes: The ownership of the data in pbinfo will be transfered
                //  to CDownLoad_OpenUIPost. Therefore, we should not call
                //  ReleaseBindInfo(pbinfo) here.
                //
                DWORD    grfBINDF;
                // The BINDINFO can not be on the stack since it will be freed by the
                // download thread.
                BINDINFO *pbinfo = (BINDINFO *) LocalAlloc( LPTR, SIZEOF(BINDINFO) );
                if (!pbinfo)
                    return E_OUTOFMEMORY;

                pbinfo->cbSize = SIZEOF(BINDINFO);
                GetBindInfo(&grfBINDF, pbinfo);

                // If our POST was really a redirected POST, it will have
                // turned into a GET.  In this case, we need to release
                // ownership of the data and pretend like the whole thing
                // was a GET to start with.

                if (pbinfo->dwBindVerb==BINDVERB_GET)
                {
                    WCHAR  wszUrl[INTERNET_MAX_URL_LENGTH];
                    ASSERT(_pszRedirectedURL);
                    SHTCharToUnicode(_pszRedirectedURL, wszUrl, ARRAYSIZE(wszUrl));
                    CDownLoad_OpenUIURL(wszUrl, NULL, pwzHeaders, 
                                         FALSE /* fSync */, FALSE /* fSaveAs */, pdoh->_fCalledMayOpenSafeDlg,
                                         NULL, NULL, NULL, _pszRedirectedURL, pdoh->_uiCP);
                    pwzHeaders = NULL;   // ownership is to CDownload now
                    ReleaseBindInfo(pbinfo); // This one is OK since we did not pass the pbinfo
                    LocalFree(pbinfo);       // and we can free it 
                }
                else {

                    ASSERT(pbinfo->dwBindVerb==BINDVERB_POST);

                    // Collect the headers associated with this xact
                    CDownLoad_OpenUI(pdoh->_pmkCur, _pbc, FALSE /* fSync */, FALSE /* fSaveAs */, pdoh->_fCalledMayOpenSafeDlg /* fSafe */, pwzHeaders, BINDVERB_POST, grfBINDF, pbinfo, _pszRedirectedURL, pdoh->_uiCP);
                    pwzHeaders = NULL;   // ownership is to CDownload now
                    TraceMsg(TF_SHDBINDING, "DOH::OnStopBinding just called CDownLoad_OpenUIPost");
                    // NOTE: t-gpease 8-18-97
                    // Do not ReleaseBindInfo(pinfo) because it is used by the download thread.
                    // The thread is responsible for releasing it.
                }

            } else {
                // Otherwise, spawn another thread and get it there.

                // NOTE: If UnBind gets called then pdoh->_pmkCur will be NULL
                // and URLMON is most likely returning a bogus error code.  So
                // we'll check the pointer and prevent from blowing up.

                if (pdoh->_pmkCur)
                {
                    BOOL fSafe = pdoh->_fCalledMayOpenSafeDlg;
                    IBrowserService* pbs;
                    if (PathIsFilePath(szURL) && 
                        SUCCEEDED(pdoh->QueryService(SID_STopFrameBrowser, IID_IBrowserService, (LPVOID *)&pbs))) {
                        DWORD dwFlags;
                        if (SUCCEEDED(pbs->GetFlags(&dwFlags)) && (dwFlags & BSF_NOLOCALFILEWARNING)) {
                            fSafe = TRUE;
                        }
                        pbs->Release();
                    }

                    CDownLoad_OpenUI(pdoh->_pmkCur, pdoh->_pbcCur, FALSE, FALSE, fSafe, pwzHeaders, NULL, NULL, NULL, _pszRedirectedURL, pdoh->_uiCP);
                    pwzHeaders = NULL;   // ownership is to CDownload now
                    fAsyncDownload = TRUE;
                }
            }
            if (pwzHeaders)
                CoTaskMemFree(pwzHeaders);
            break;

        // URLMON failed to bind because it didn't know what to do with
        // with this URL.  Lets check and see if the Shell should handle
        // it via a helper app (news:, mailto:, telnet:, etc.)
        case INET_E_UNKNOWN_PROTOCOL:
            _SetSearchInfo(pdoh, 0, FALSE, FALSE, FALSE);  // reset info
            {
                // If we've been redirected, use that URL
                //
                if (_pszRedirectedURL)
                    StrCpyN(szURL, _pszRedirectedURL, ARRAYSIZE(szURL));

                // Here we check to see if it is a URL we really want to shellexecute
                // so it is handled by helper apps.....else it really is an error
                if (ShouldShellExecURL(szURL))
                {
                    // We can add this to the address bar MRU 
                    fAddToMRU = TRUE;

                    // We need to decode this before passing it on to someone.

                    TCHAR szDecodedURL[INTERNET_MAX_URL_LENGTH];
                    DWORD cchDecodedURL = ARRAYSIZE(szDecodedURL);

                    // REVIEW: NT 319480 IE 54850 - need to append _pszLocation back to pszBadProtoURL...
                    //
                    // I assume the string was escaped when it came from urlmon, so we need
                    // to append it before PrepareURLForExternalApp.
                    //
                    // Note: if the url had been redirected above, _pszLocation has been updated
                    // to the new redirected URL, so we still want to append it.
                    //
                    if (pdoh->_pszLocation)
                        StrCatBuff(szURL, pdoh->_pszLocation, ARRAYSIZE(szURL));

                    PrepareURLForExternalApp(szURL, szDecodedURL, &cchDecodedURL);

                    // PathQuoteSpaces(szDecodedURL);

                    SHELLEXECUTEINFO sei = {0};

                    sei.cbSize = sizeof(sei);
                    sei.lpFile = szDecodedURL;
                    sei.nShow  = SW_SHOWNORMAL;

                    if (!ShellExecuteEx(&sei))
                    {
                        if(!bSuppressUI){
                            IE_ErrorMsgBox(pdoh->_psb, pdoh->_hwnd, hrDisplay, szError,
                                            szDecodedURL, IDS_CANTSHELLEX, MB_OK | MB_ICONSTOP );
                        }
                    }

                    //
                    //  We want to close the browser window if this is the
                    // very first navigation. 
                    //
                    fAsyncDownload = TRUE;
                } else {
                    if(!bSuppressUI){
                        _NavigateToErrorPage(ERRORPAGE_SYNTAX, pdoh, FALSE);
                    }
                }
                break;
            }

        case E_ABORT:
        case HRESULT_FROM_WIN32(ERROR_CANCELLED):
            _SetSearchInfo(pdoh, 0, FALSE, FALSE, FALSE);  // reset info
            break;

#ifdef BUG_EXEC_ON_FAILURE
        case E_NOINTERFACE: // BUGBUG nash:31526
            TraceMsg(TF_SHDBINDING, "DOH::OnStopBinding E_NOINTERFACE");
            goto Lexec;
#endif

        case INET_E_RESOURCE_NOT_FOUND:
        case INET_E_DATA_NOT_AVAILABLE:
            if (_HandleFailedNavigationSearch(&fShouldDisplayError, dwStatusCode, pdoh, hrDisplay, (LPTSTR) &szURL, szError, _pib) != S_OK)
                fShouldDisplayError = TRUE;
            // intentional fallthrough to default to popup if needed

        case INET_E_DOWNLOAD_FAILURE:
            if(IsGlobalOffline())
            {
                fShouldDisplayError = FALSE;
                break; 
            }
            // otherwise fall through to do default handling
        default:
            {
                if (fShouldDisplayError) {
                    _SetSearchInfo(pdoh, 0, FALSE, FALSE, FALSE);  // reset info
                    if (!bSuppressUI) {
                        //
                        // If we're in a frame try to navigate in place.  This
                        // won't work if we're in a synchronous call
                        // (_fSetTarget).
                        //
                        BOOL fNavigateInPlace = pdoh->_fHaveParentSite && !pdoh->_fSetTarget;
                        _NavigateToErrorPage(ERRORPAGE_DNS, pdoh, fNavigateInPlace);
                    }
                }
            }
            break;

        }

        // Tell addressbar to not add this to its mru
        if (!fAddToMRU)
        {
            _DontAddToMRU(pdoh);
        }

        //
        // Prepare for the case where the container keep us visible
        // after hitting this code (Explorer does, IE doesn't).
        //
        pdoh->_fDrawBackground = TRUE;

        // In the case of quickly jumping to another link, we end up with
        // a _hwnd being NULL and we were invalidating the desktop. So,
        // I check for NULL here before calling InvalidateRect.
        if(pdoh->_hwnd)
            InvalidateRect(pdoh->_hwnd, NULL, TRUE);

        //
        //  Tell the browser to cancel the pending navigation only
        // if it has not been canceled by the browser itself.
        //
        if (!pdoh->_fCanceledByBrowser) {
            pdoh->_CancelPendingNavigation(fAsyncDownload);
        } else {
            TraceMsg(TF_SHDNAVIGATE|TF_SHDPROGRESS, 
                "DOH::::OnStopBinding not calling _CancelPendingNav");
        }
    }
    else
    {
        BOOL      bDidNavigate = FALSE;

        //  Might have redirected to mailto: or some other protocol handled by
        //  plugable protocol that does some magic (eg launch mail program) and
        //  reports OnStopBinding w/o going through OnObjectAvailable!
        if (NULL == pdoh->_pole && !pdoh->_fCanceledByBrowser)
            pdoh->_CancelPendingNavigation(FALSE);

        // It is still possible that our Proxy failed to find the server but
        // gave us HTML.  If this is the case, and the user has "find sites"
        // set, we should go ahead and start trying to do our automatic
        // navigation stuff.

        if (dwStatusCode && DO_SEARCH_ON_STATUSCODE(dwStatusCode)) {
            if (_HandleFailedNavigationSearch(&fShouldDisplayError, dwStatusCode, pdoh, hrDisplay, NULL, szError, _pib) == S_OK) {
                bDidNavigate = TRUE;
            }
            // Note, since the Proxy will have given us HTML in this case,
            // we will never display an error dialog.
        }

        if (!bDidNavigate) {
            _SetSearchInfo(pdoh, 0, FALSE, FALSE, FALSE);  // reset info

            //  We can suppress this redundant call to Add to History if DocObject
            //  is MSHTML, since it will report readystate
            if (!_fBoundToMSHTML && pdoh->_pmkCur)
            {
                TCHAR szUrl[MAX_URL_STRING+1];
                SHSTRW strwUrl;

                pdoh->_GetCurrentPage(szUrl,ARRAYSIZE(szUrl));

                if (pdoh->_pszLocation)
                    StrCatBuff(szUrl, pdoh->_pszLocation, ARRAYSIZE(szUrl));

                if(!bSuppressUI && SUCCEEDED(strwUrl.SetStr(szUrl)))
                    AddUrlToUrlHistoryStg((LPWSTR)strwUrl, 
                                    NULL, 
                                    pdoh->_pwb,
                                    pdoh->_fWriteHistory,
                                    pdoh->_fSelectHistory ? pdoh->_pocthf:NULL,
                                    pdoh->get_punkSFHistory(), NULL);
            }
        } // if !bDidNavigate
    } // if failed(hrerror) ... else

    // Released here because we may need it for OpenUI() w/ POST verb
    ATOMICRELEASE(_pbc);

    if (_pbscChained) {
#ifndef NO_DELEGATION
        CHAINMSG("OnStopBinding", hrError);
        _pbscChained->OnStopBinding(hrError, szError);
#endif
    }

    ATOMICRELEASE(_pbscChained);
    ATOMICRELEASE(_pnegotiateChained);
    pdoh->_ResetStatusBar();

    ATOMICRELEASE(pdoh->_pbcCur);

    if (_pszHeaders)
    {
        LocalFree(_pszHeaders);
        _pszHeaders = NULL;
    }
    if (_hszPostData)
    {
        GlobalFree(_hszPostData);
        _hszPostData = NULL;
    }

    // NOTES: Guard against last Release by _RevokeObjectParam
    Release();

    return S_OK;
}

void CDocObjectHost::CDOHBindStatusCallback::AbortBinding(void)
{
    TraceMsg(TF_SHDPROGRESS, "CDOH::CBSC::AbortBinding called _pib=%x", _pib);

    if (_pib)
    {
        TraceMsg(0, "sdv TR AbortBinding Calling _pib->Abort");
        //
        // Notes: OnStopBinding(E_ABORT) will be called from _pib->Abort
        //
        HRESULT hresT = _pib->Abort();
        TraceMsg(TF_SHDBINDING, "sdv TR AbortBinding Called _pib->Abort (%x)", hresT);

        // URLMon may call our OnStopBinding asynchronously.
        ATOMICRELEASE(_pib);

        
        CDocObjectHost* pdoh = IToClass(CDocObjectHost, _bsc, this);
        if(pdoh->_dwProgressPos)
        {
            pdoh->_ResetStatusBar();
            pdoh->_OnSetProgressPos(0, PROGRESS_RESET);
        }
    }
}

//
// NavigatesToErrorPage cancels the pending navigation and and navigates to
// an internal error page.
//

void CDocObjectHost::CDOHBindStatusCallback::_NavigateToErrorPage(DWORD dwError, CDocObjectHost* pdoh, BOOL fInPlace)
{
    ASSERT(IsErrorHandled(dwError));
    ASSERT(pdoh);

    // Security:  Release the pre-created object because we don't want
    // anyone to have access to the OM of the navigated error document
    // if they obtained the reference before the error navigation.
    // Releasing the reference prevents a parent window from getting keys
    // to the My Computer zone.

    pdoh->_ReleaseOleObject(FALSE);
    pdoh->_ReleasePendingObject(FALSE);

    //
    // pdoh->_pmkCur can be NULL if this is a "DNS" error and Unbind has already
    // been called.
    //

    if (pdoh->_pmkCur)
    {
        //
        // Save the url the user attempted to navigate to.  It will be used
        // to refresh the page.
        //

        if (pdoh->_pwszRefreshUrl)
        {
            OleFree(pdoh->_pwszRefreshUrl);
            pdoh->_pwszRefreshUrl = NULL;
        }

        pdoh->_pmkCur->GetDisplayName(pdoh->_pbcCur, NULL,
                                      &pdoh->_pwszRefreshUrl);
    }

    if ((NULL == pdoh->_pwszRefreshUrl) || !IsErrorUrl(pdoh->_pwszRefreshUrl))
    {

        //
        // Build the error page url.
        //

        TCHAR szErrorUrl[MAX_URL_STRING];

        if (fInPlace)
        {
            HRESULT hr;

            hr = MLBuildResURLWrap(TEXT("shdoclc.dll"),
                                   HINST_THISDLL,
                                   ML_CROSSCODEPAGE,
                                   (TCHAR *)c_aErrorUrls[EUIndexFromError(dwError)].pszUrl,
                                   szErrorUrl,
                                   ARRAYSIZE(szErrorUrl),
                                   TEXT("shdocvw.dll"));
            if (SUCCEEDED(hr))
            {
                //
                // Navigate to the error page.
                //

                IMoniker* pIMoniker;

                if (SUCCEEDED(MonikerFromString(szErrorUrl, &pIMoniker)))
                {
                    ASSERT(pIMoniker);
#ifdef DEBUG
                    pdoh->_fFriendlyError = TRUE;
#endif

                    pdoh->SetTarget(pIMoniker, pdoh->_uiCP, NULL, NULL, NULL, 0);

                    pIMoniker->Release();
                }
            }
        }
        else
        {
            const WCHAR* const pszFmt = L"#%s";
            HRESULT hr;

            hr = MLBuildResURLWrap(TEXT("shdoclc.dll"),
                                   HINST_THISDLL,
                                   ML_CROSSCODEPAGE,
                                   (TCHAR *)c_aErrorUrls[EUIndexFromError(dwError)].pszUrl,
                                   szErrorUrl,
                                   ARRAYSIZE(szErrorUrl),
                                   TEXT("shdocvw.dll"));
            if (SUCCEEDED(hr))
            {
                int nLenWritten;

                // append the #<refresh URL>
                nLenWritten = lstrlen(szErrorUrl);
                wnsprintf(szErrorUrl + nLenWritten,
                          ARRAYSIZE(szErrorUrl) - nLenWritten,
                          pszFmt,
                          pdoh->_pwszRefreshUrl ? pdoh->_pwszRefreshUrl : L"");

                //
                // Cancel the server page and display the internal page instead.
                //

                if (!pdoh->_fCanceledByBrowser)
                    pdoh->_CancelPendingNavigation(FALSE);

                pdoh->_DoAsyncNavigation(szErrorUrl);
                pdoh->_fCanceledByBrowser = TRUE;
            }
        }
    }
    return;
}

//
// Check if the user turned off friendly http errors.  Default is yes.
//

BOOL CDocObjectHost::CDOHBindStatusCallback::_DisplayFriendlyHttpErrors()
{
    BOOL fRet;

    DWORD dwType = REG_SZ;
    TCHAR  szYesOrNo[20];
    DWORD  cbSize = sizeof(szYesOrNo);

    if (ERROR_SUCCESS == SHRegGetUSValue(REGSTR_PATH_MAIN,
                                         REGSTR_VAL_HTTP_ERRORS, &dwType,
                                         (LPVOID)szYesOrNo, &cbSize, FALSE,
                                         NULL, 0))
    {
        fRet = StrCmpI(szYesOrNo, L"no");
    }
    else
    {
        fRet = TRUE;
    }

    return fRet;
}

//
// Error handler
//

void CDocObjectHost::CDOHBindStatusCallback::_HandleHttpErrors(DWORD dwError, DWORD cbContentLength, CDocObjectHost* pdoh)
{
    // Tell addressbar to not add this to its mru
    _DontAddToMRU(pdoh);

    // Tell history to not add this url
    pdoh->_fWriteHistory = FALSE;

    if (IsErrorHandled(dwError))
    {
        //
        //  On a 4XX error display an internal page if the server returned a
        //  page smaller than the threshold value.  If the page is larger than
        //  the threshold, display it.
        //
        // If the content length is zero assume the server didn't send the
        // length.  In this case take the conservative approach and don't 
        // show our page.
        //

        if (cbContentLength != 0 &&
            cbContentLength <= _GetErrorThreshold(dwError))
        {
            if (_DisplayFriendlyHttpErrors())
                _NavigateToErrorPage(dwError, pdoh, TRUE);
        }
    }

    return;
}

//
// Informs the address bar to not put this page in its mru
//
void CDocObjectHost::CDOHBindStatusCallback::_DontAddToMRU(CDocObjectHost* pdoh)
{
    IDockingWindow* pdw = NULL;
    IOleCommandTarget* poct;

    if (pdoh->_psp &&
        SUCCEEDED(pdoh->_psp->QueryService(SID_SExplorerToolbar, IID_IDockingWindow, (LPVOID*)&pdw)))
    {
        if (SUCCEEDED(pdw->QueryInterface(IID_IOleCommandTarget, (LPVOID*)&poct)))
        {
            // Get the URL we were navigating to
            LPWSTR pszUrl;
            if (pdoh->_pmkCur &&
                SUCCEEDED(pdoh->_pmkCur->GetDisplayName(pdoh->_pbcCur, NULL, &pszUrl)))
            {
                // Copy url to stack allocated bstr
                SA_BSTR bstr;
                StrCpyNW(bstr.wsz, pszUrl, ARRAYSIZE(bstr.wsz));
                bstr.cb = lstrlenW(bstr.wsz) * sizeof(WCHAR);

                VARIANT varURL = {0};
                varURL.vt      = VT_BSTR;
                varURL.bstrVal = bstr.wsz;
                poct->Exec(&CGID_Explorer, SBCMDID_ERRORPAGE, 0, &varURL, NULL);

                OleFree(pszUrl);
            }

            poct->Release();
        }
        pdw->Release();
    }
}

//
// Tells the addressbar that we are autosearching so that it can update
// the pending url in its mru
//
void CDocObjectHost::CDOHBindStatusCallback::_UpdateMRU(CDocObjectHost* pdoh, LPCWSTR pszUrl)
{
    IDockingWindow* pdw = NULL;
    IOleCommandTarget* poct;

    if (pdoh->_psp &&
        SUCCEEDED(pdoh->_psp->QueryService(SID_SExplorerToolbar, IID_IDockingWindow, (LPVOID*)&pdw)))
    {
        if (SUCCEEDED(pdw->QueryInterface(IID_IOleCommandTarget, (LPVOID*)&poct)))
        {
            // Copy url to stack allocated bstr
            SA_BSTR bstr;
            StrCpyNW(bstr.wsz, pszUrl, ARRAYSIZE(bstr.wsz));
            bstr.cb = lstrlenW(bstr.wsz) * sizeof(WCHAR);

            VARIANT varURL = {0};
            varURL.vt      = VT_BSTR;
            varURL.bstrVal = bstr.wsz;
            poct->Exec(&CGID_Explorer, SBCMDID_AUTOSEARCHING, 0, &varURL, NULL);

            poct->Release();
        }
        pdw->Release();
    }
}

//
//  S_OK means we successfully did a navigation
//  S_FALSE means that we did everything ok, but did not navigate
//  E_* means some internal api failed.
//

HRESULT CDocObjectHost::CDOHBindStatusCallback::_HandleFailedNavigationSearch(LPBOOL pfShouldDisplayError, DWORD dwStatusCode, CDocObjectHost *pdoh, HRESULT hrDisplay, TCHAR *szURL, LPCWSTR szError, IBinding *pib)
{
    DWORD                dwSearchForExtensions = NO_SUFFIXES;
    DWORD                dwDo404Search = PROMPTSEARCH;
    BOOL                 bAskUser = TRUE;  // rely on init
    BOOL                 bDoSearch = FALSE;  // rely on init
    HRESULT              hres = S_FALSE;
    BOOL                 bSuppressUI = FALSE;
    BOOL                 bFrameIsOffline = FALSE;
    BOOL                 bPrepareForSearch = FALSE;
    DWORD                dwSuffixIndex = 0;
    BOOL                 bAllowSearch = FALSE;
    BOOL                 bContinueSearch = FALSE;
    BOOL                 bSentToEngine = FALSE;
    BOOL                 bOnProxy = FALSE;  
    TCHAR                szSearchFormatStr[MAX_SEARCH_FORMAT_STRING];

#define SAFETRACE(psz)      (psz ? psz : TEXT(""))

    TraceMsg(TF_SHDNAVIGATE, "DOH::BSC::_HFNS() entered status = %d, url = %s, pib = %X", dwStatusCode, SAFETRACE(szURL) , pib);
    if (FAILED(GetSearchKeys(&dwSearchForExtensions, &dwDo404Search)))
    {
        return E_FAIL;
    }
    
    TraceMsg(TF_SHDNAVIGATE, "DOH::BSC::_HFNS() dwSearch = %d, do404 = %d", dwSearchForExtensions, dwDo404Search);

    // Get any persistent information from the last request
    VARIANT varURL = {0};
    _GetSearchInfo(pdoh->_psp, &dwSuffixIndex, &bAllowSearch, &bContinueSearch, &bSentToEngine, &varURL);

    // See if window.external.autoscan() was called
    BOOL fAutoScan = (varURL.vt == VT_BSTR);

    TraceMsg(TF_SHDNAVIGATE, "DOH::BSC::_HFNS() index = %d, allow = %d, cont = %d, sent = %d", dwSuffixIndex, bAllowSearch, bContinueSearch, bSentToEngine);

    // Why not use the cached value?
    // pdoh->_GetOfflineSilent(&bFrameIsOffline, &bSuppressUI);
    bFrameIsOffline = _bFrameIsOffline ? TRUE : FALSE;
    bSuppressUI = (_bFrameIsSilent || _IsDesktopItem(pdoh)) ? TRUE : FALSE;

    // if we are at the end of the extension list, turn off extensions
    BOOL fAutoSearching = FALSE;
    if (dwSearchForExtensions)
    {
        if (dwSuffixIndex == 0 && IsAutoSearchEnabled())
        {
            StrCpyN(szSearchFormatStr, L"? %s", ARRAYSIZE(szSearchFormatStr));
            fAutoSearching = TRUE;
        }
        else if (GetSearchFormatString(dwSuffixIndex, szSearchFormatStr, sizeof(szSearchFormatStr)) != ERROR_SUCCESS)
        {
            dwSearchForExtensions = DONE_SUFFIXES;
            StrCpyN(szSearchFormatStr, TEXT("%s"), ARRAYSIZE(szSearchFormatStr));
        }
    }
    else
    {
        dwSearchForExtensions = DONE_SUFFIXES;
    }

    // don't try a 404 srch if we are still trying suffixes
    if (dwSearchForExtensions == SCAN_SUFFIXES)
        dwDo404Search = NEVERSEARCH;

    {
        DWORD dwOptions;

        if (SUCCEEDED(_GetRequestFlagFromPIB(pib, &dwOptions)))
        {
            if (dwOptions & INTERNET_REQFLAG_VIA_PROXY)
            {
                bOnProxy = TRUE;
            }
        }
        else
        {
            TraceMsg(TF_SHDNAVIGATE, "DOH::BSC::_HFNS() QI to IWinInetInfo failed");
        }
    }

    TraceMsg(TF_SHDNAVIGATE, "DOH::BSC::_HFNS() search = %d, do404 = %d, onproxy = %d, szSearch = %s", dwSearchForExtensions, dwDo404Search, bOnProxy, SAFETRACE(szSearchFormatStr));

    // Prepare to do an automatic search if the navigation failed
    // and we think a search might be valuable.

    // These cases are:
    //   (1) if the previous navigation was search-generated (bContinue)
    //   (2) the user allows searching (bAllow)
    //   (3) we are searching for extensions or autosearching
    //   (4) this is a status code we allow searching for
    //   (5) if over proxy, continue searching even on 404

    // Note: 404 is special; it is the case that most servers return this if
    // the documnet is not there, but Proxies also return this if the server
    // was not found - a conditon which normally makes us search.  This means
    // that a 404 over proxy actually causes a search to occur, which is not
    // what we want.
    // BUGBUG: Is there any way I can tell the difference?

    bPrepareForSearch = ((bContinueSearch || (bAllowSearch)) &&
                 (fAutoScan || SHOULD_DO_SEARCH(dwSearchForExtensions, dwDo404Search)) &&
                 DO_SEARCH_ON_STATUSCODE(dwStatusCode) &&
                 (!bOnProxy || (dwStatusCode == HTTP_STATUS_NOT_FOUND)));

    if (bPrepareForSearch)
    {
        TraceMsg(TF_SHDNAVIGATE, "DOH::BSC::_HFNS() Preparing for Search...");

        HRESULT hr = S_OK;

        // If we don't have the url we are searching, get it from the addressbar
        if (!fAutoScan)
            hr = _GetSearchString(pdoh->_psp, &varURL);

        // If we have completed the autoscan, see if there is a special error page that
        // we should display.
        VARIANT varScanFailure = {0};
        if (SUCCEEDED(hr) &&
            dwSearchForExtensions == DONE_SUFFIXES &&
            SUCCEEDED(_GetScanFailureUrl(pdoh->_psp, &varScanFailure)))
        {
            bDoSearch = TRUE;
        }

        else if (SUCCEEDED(hr) &&
            (dwSearchForExtensions == SCAN_SUFFIXES ||  dwDo404Search == ALWAYSSEARCH)) 
        {
            bDoSearch = TRUE;
        } 
        else 
        {
            bDoSearch = FALSE; 
        }
        bAskUser = FALSE;

        TraceMsg(TF_SHDNAVIGATE, "DOH::BSC::_HFNS() typedurl = %s, ask = %d, dosearch = %d", varURL.bstrVal, bAskUser, bDoSearch);


// bugbug: don't prompt user if there is an extension, since we are going to
// not scan anyway.

        if (bDoSearch)
        {
            PARSEDURL pu;

            pu.cbSize = SIZEOF(PARSEDURL);
            if (ParseURL(varURL.bstrVal, &pu) == URL_E_INVALID_SYNTAX)
            {
                // only if this is not a valid URL, should we try to do this searching

                // but try to avoid the case of typos like http;//something.something.com
                // The malformed URL case
                if (!fAutoSearching &&
                    (//StrChrI(varURL.bstrVal, L'.') || 
                     StrChrI(varURL.bstrVal, L'/') ||
                     StrChrI(varURL.bstrVal, L' '))
                    )
                {
                    bAskUser = FALSE;
                    bDoSearch = FALSE;
                }
            }
            else
            {
                bAskUser = FALSE;
                bDoSearch = FALSE;
            }
        }

        TCHAR szT[MAX_URL_STRING + SEARCHPREFIXLENGTH];
        DWORD cchT = SIZECHARS(szT);

        // BUGBUG bug 35354 has been resolved "not repro" because the dialog below
        // currently cannot ever be displayed (there is no way for bAskUser to
        // be true in the following conditional). If that changes, then that bug
        // needs to get fixed.

        if (bAskUser)
        {
            PrepareURLForDisplay(varURL.bstrVal, szT, &cchT);

            // If we ask the user, make sure we don't display another
            // error dialog.
            *pfShouldDisplayError = FALSE;

            // S_OK means we handled any popups; if we return an error,
            // the caller may display an error dialog
            hres = S_OK;

            if (!bSuppressUI && IDYES == IE_ErrorMsgBox(NULL, pdoh->_hwnd, hrDisplay, szError, szT, IDS_CANTFINDURL, MB_YESNO|MB_ICONSTOP))
            {
                bDoSearch = TRUE;
            }
            else
            {
               _SetSearchInfo(pdoh, 0, FALSE, FALSE, FALSE);  // reset info
                bDoSearch = FALSE;
            }
        }

        if (bDoSearch)
        {
            if (dwSearchForExtensions && dwSearchForExtensions != DONE_SUFFIXES)
            {
                wnsprintf(szT, ARRAYSIZE(szT), szSearchFormatStr, varURL.bstrVal);
                if (!fAutoSearching)
                {
                    _ValidateURL(szT, UQF_DEFAULT);
                }
                _UpdateMRU(pdoh, szT);
            } 
            else if (NULL != varScanFailure.bstrVal)
            {
                StrCpyN(szT, varScanFailure.bstrVal, ARRAYSIZE(szT));
                _ValidateURL(szT, UQF_DEFAULT);
                _DontAddToMRU(pdoh);
            }
            else if (dwDo404Search)
            {
                // add the search prefix
                StrCpyN(szT, TEXT("? "), ARRAYSIZE(szT));
                StrCatBuff(szT, varURL.bstrVal, ARRAYSIZE(szT));
                _DontAddToMRU(pdoh);
            }
            else
            {
                ASSERT(0);
            }

            if (dwSearchForExtensions && dwSearchForExtensions != DONE_SUFFIXES)
                _SetSearchInfo(pdoh, ++dwSuffixIndex, FALSE, TRUE, FALSE);
            else if (dwDo404Search)
                _SetSearchInfo(pdoh, dwSuffixIndex, FALSE, FALSE, TRUE);
            else
                _SetSearchInfo(pdoh, 0, FALSE, FALSE, FALSE);

            if (!pdoh->_fCanceledByBrowser) 
                pdoh->_CancelPendingNavigation(FALSE);

            TraceMsg(TF_SHDNAVIGATE, "DOH::BSC::_HFNS() Doing search on %s", szT);

            DWORD cchT = SIZECHARS(szT);
            ParseURLFromOutsideSource(szT, szT, &cchT, NULL);
            pdoh->_DoAsyncNavigation(szT);
            pdoh->_fCanceledByBrowser = TRUE;
            *pfShouldDisplayError = FALSE;  // Don't display another dialog

            hres = S_OK;  // we did a navigate
        } 
    }
    else if (bSentToEngine && !bSuppressUI)
    {
        *pfShouldDisplayError = FALSE;
        _SetSearchInfo(pdoh, 0, FALSE, FALSE, FALSE);
        IE_ErrorMsgBox(NULL, pdoh->_hwnd, hrDisplay, szError, szURL, IDS_CANTFINDSEARCH, MB_OK|MB_ICONSTOP);
        hres = S_OK;
    }

    VariantClear(&varURL);

    return hres;

} // _HandleFailedNavigationSearch()


