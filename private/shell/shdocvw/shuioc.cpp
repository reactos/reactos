//
// Shell UI Control Class (CShellUIHelper)
//
// Sample code : shell\docs\shuiod.htm
//
// This is the source code of the ShellUI control class. You can put an instance
// of ShellUI control on any HTML pages then
// 
// The key thing is it always bring up some UI and never alter the system
// silently (without the User's interaction). By doing this, we can expose
// shell features as much as we want without worrying about the security
// problem (which automation interface has).
// 
// This control also allow us to provide many configuration type UI (such as
// "Cutsomize your IE page") as well as rich web-view on some folders
// (especially control panel ;-) without paying the cost of data-binding. 
// 
#include "priv.h"
#include "sccls.h"
#include <fsmenu.h>     // needed for favorite.h
#ifndef UNIX
#include <webcheck.h>
#else
#include <subsmgr.h>
#endif
#include "favorite.h"
#include "caggunk.h"
#include "resource.h"
#include "channel.h"
#include "chanmgr.h"
#include "chanmgrp.h"
#include "iforms.h"
#include "dspsprt.h"
#include "impexp.h" // needed for RunImportExportWizard()
#include "iforms.h"
//#include "cobjsafe.h" // CObjectSafety
#include "shvocx.h" // WrapSpecialUrl()

#include <mluisupp.h>

#define REG_DESKCOMP_SCHEME                 TEXT("Software\\Microsoft\\Internet Explorer\\Desktop\\Scheme")
#define REG_VAL_SCHEME_DISPLAY              TEXT("Display")
#define REG_VAL_GENERAL_WALLPAPER           TEXT("Wallpaper")
#define REG_VAL_GENERAL_TILEWALLPAPER       TEXT("TileWallpaper")
#define REG_DESKCOMP_GENERAL                TEXT("Software\\Microsoft\\Internet Explorer\\Desktop%sGeneral")

STDAPI SHAddSubscribeFavorite (HWND hwnd, LPCWSTR pwszURL, LPCWSTR pwszName, DWORD dwFlags,
                               SUBSCRIPTIONTYPE subsType, SUBSCRIPTIONINFO* pInfo);

// move it to shdocvw.h
UINT IE_ErrorMsgBox(IShellBrowser* psb,
                    HWND hwndOwner, HRESULT hrError, LPCWSTR szError, LPCTSTR pszURLparam,
                    UINT idResource, UINT wFlags);

BOOL UpdateAllDesktopSubscriptions();
#define DM_SHUIOC   DM_TRACE

LONG GetSearchFormatString(DWORD dwIndex, LPTSTR psz, DWORD cbpsz);

HRESULT TargetQueryService(IUnknown *punk, REFIID riid, void **ppvObj);

EXTERN_C const SA_BSTRGUID s_sstrSearch;
EXTERN_C const SA_BSTRGUID s_sstrFailureUrl;

class CShellUIHelper :
        public CAggregatedUnknown,
        public IObjectWithSite,
        public IObjectSafety,
        public IShellUIHelper,  // dual, IDispatch
        public IDispatchEx,
        protected CImpIDispatch
{
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj) { return CAggregatedUnknown::QueryInterface(riid, ppvObj);};
    STDMETHODIMP_(ULONG) AddRef(void) { return CAggregatedUnknown::AddRef();};
    STDMETHODIMP_(ULONG) Release(void) { return CAggregatedUnknown::Release();};

    // IObjectWithSite
    STDMETHODIMP SetSite(IUnknown *pUnkSite);
    STDMETHODIMP GetSite(REFIID riid, void **ppvSite);

    // IObjectSafety
    STDMETHOD(GetInterfaceSafetyOptions)(REFIID riid, DWORD *pdwSupportedOptions, 
        DWORD *pdwEnabledOptions);
    STDMETHOD(SetInterfaceSafetyOptions)(REFIID riid, DWORD dwOptionSetMask, 
        DWORD dwEnabledOptions);


    // IDispatch
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);
    STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo);
    STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames,
                               LCID lcid, DISPID * rgdispid);
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, 
                        DISPPARAMS * pdispparams, VARIANT * pvarResult, 
                        EXCEPINFO * pexcepinfo, UINT * puArgErr);

    // IDispatchEx
    STDMETHODIMP GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid);
    STDMETHODIMP InvokeEx(DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
                          VARIANT *pvarRes, EXCEPINFO *pei, 
                          IServiceProvider *pspCaller);
    STDMETHODIMP DeleteMemberByName(BSTR bstr, DWORD grfdex);           
    STDMETHODIMP DeleteMemberByDispID(DISPID id);           
    STDMETHODIMP GetMemberProperties(DISPID id, DWORD grfdexFetch, DWORD *pgrfdex);           
    STDMETHODIMP GetMemberName(DISPID id, BSTR *pbstrName);
    STDMETHODIMP GetNextDispID(DWORD grfdex, DISPID id, DISPID *pid);
    STDMETHODIMP GetNameSpaceParent(IUnknown **ppunk);

    // IShellUIHelper
    STDMETHODIMP Execute();
    STDMETHODIMP ResetSafeMode();
    STDMETHODIMP ResetFirstBootMode();
    STDMETHODIMP RefreshOfflineDesktop();
    STDMETHODIMP AddFavorite(BSTR strURL, VARIANT *Title);
    STDMETHODIMP AddChannel(BSTR bstrURL);
    STDMETHODIMP AddDesktopComponent(BSTR strURL, BSTR strType, 
                        VARIANT *Left, VARIANT *Top, 
                        VARIANT *Width, VARIANT *Height);
    STDMETHODIMP IsSubscribed(BSTR bstrURL, VARIANT_BOOL* pBool);
    STDMETHODIMP NavigateAndFind(BSTR URL, BSTR strQuery, VARIANT* varTargetFrame);
    STDMETHODIMP ImportExportFavorites(VARIANT_BOOL fImport, BSTR strImpExpPath);
    STDMETHODIMP AutoCompleteSaveForm(VARIANT *Form);
    STDMETHODIMP AutoScan(BSTR strSearch, BSTR strFailureUrl, VARIANT* pvarTargetFrame);
    STDMETHODIMP AutoCompleteAttach(VARIANT *Form);
    STDMETHODIMP ShowBrowserUI(BSTR bstrName, VARIANT *pvarIn, VARIANT *pvarOut);

    HRESULT v_InternalQueryInterface(REFIID riid, LPVOID * ppvObj);

    CShellUIHelper(IUnknown* punkAgg);
    ~CShellUIHelper();

    inline IDispatch *GetExternalDispatch()
    {
        return _pExternalDispEx ? _pExternalDispEx : _pExternalDisp;
    }

    void SetExternalDispatch(IDispatch *pExternalDisp)
    {
        ATOMICRELEASE(_pExternalDisp);
        ATOMICRELEASE(_pExternalDispEx);

        //  If we were passed an IDispatch to delegate to then we need to
        //  see if it can do IDispatchEx so we can support it as well,
        //  otherwise we just fall back to good ole IDispatch.
        if (pExternalDisp)
        {
            if (FAILED(pExternalDisp->QueryInterface(IID_IDispatchEx, 
                                      (void **)&_pExternalDispEx)))
            {
                _pExternalDisp = pExternalDisp;
                _pExternalDisp->AddRef();
            }
        }
    }

    STDMETHODIMP ShowChannel(IChannelMgrPriv *pChMgrPriv, LPWSTR pwszURL, HWND hwnd);
    HWND _GetOwnerWindow();
    HRESULT _ConnectToTopLevelConnectionPoint(BOOL fConnect, IUnknown* punk);
    STDMETHODIMP _DoFindOnPage(IDispatch* pdisp);

    friend HRESULT CShellUIHelper_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
    friend HRESULT CShellUIHelper_CreateInstance2(IUnknown** ppunk, REFIID riid, 
                                                 IUnknown *pSite, IDispatch *pExternalDisp);

    DWORD               _dwSafety;
    // Cached pointers, hwnd
    IUnknown*           _punkSite;  // site pointer
    IDispatchEx*        _pExternalDispEx;
    IDispatch*          _pExternalDisp;
    DWORD               _dwcpCookie;
    BOOL                _fWaitingToFindText;
    BSTR                _bstrQuery;
    VOID *              _pvIntelliForms;
};

STDAPI CShellUIHelper_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    HRESULT hres = E_OUTOFMEMORY;
    CShellUIHelper* psuo = new CShellUIHelper(punkOuter);
    if (psuo)
    {
        *ppunk = psuo->_GetInner();
        hres = S_OK;
    }
    return hres;
}

HRESULT CShellUIHelper_CreateInstance2(IUnknown** ppunk, REFIID riid, 
                                      IUnknown *pSite, IDispatch *pExternalDisp)
{
    HRESULT hres = E_OUTOFMEMORY;
    CShellUIHelper* psuo = new CShellUIHelper(NULL);
    
    if (psuo)
    {
        hres = psuo->QueryInterface(riid, (void **)ppunk);
        psuo->Release();

        if (SUCCEEDED(hres))
        {
            psuo->SetSite(pSite);
            psuo->SetExternalDispatch(pExternalDisp);
        }
    }

    return hres;
}

CShellUIHelper::CShellUIHelper(IUnknown* punkAgg) : CAggregatedUnknown(punkAgg), CImpIDispatch(&IID_IShellUIHelper)
{
    DllAddRef();
    _fWaitingToFindText = FALSE;
    _bstrQuery = NULL;
}

CShellUIHelper::~CShellUIHelper()
{

    ReleaseIntelliForms(_pvIntelliForms);

    if (_punkSite)
        SetSite(NULL);  // In case the parent did not clean it up.
    if (_bstrQuery)
        SysFreeString(_bstrQuery);
    ATOMICRELEASE(_pExternalDisp);
    ATOMICRELEASE(_pExternalDispEx);

    DllRelease();
}

HRESULT CShellUIHelper::v_InternalQueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IDispatch) ||
        IsEqualIID(riid, IID_IShellUIHelper))
    {
        *ppvObj = SAFECAST(this, IShellUIHelper *);
    }
    else if (IsEqualIID(riid, IID_IObjectWithSite))
    {
        *ppvObj = SAFECAST(this, IObjectWithSite *);
    }
    else if (IsEqualIID(riid, IID_IObjectSafety))
    {
        *ppvObj = SAFECAST(this, IObjectSafety *);
    }
    else if (IsEqualIID(riid, IID_IDispatchEx))
    {
        *ppvObj = SAFECAST(this, IDispatchEx *);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

HWND CShellUIHelper::_GetOwnerWindow()
{
    HWND hwnd;
    HRESULT hres;

    // this deals with NULL _punkSite and zeros hwnd on failure
    IUnknown_GetWindow(_punkSite, &hwnd);

    if (!hwnd)
    {
        //
        //  We get to this point if we are instantiated like this
        //  in jscript:
        //      foo = new ActiveXObject("Shell.UIControl");
        //  or vbscript:
        //      set foo = CreateObject("Shell.UIControl");
        //
        if (_punkSite)
        {
            IServiceProvider *pSP = NULL;
            IOleWindow *pOleWindow = NULL;
            hres = _punkSite->QueryInterface(IID_IServiceProvider, (void **)&pSP);

            if (SUCCEEDED(hres))
            {
                ASSERT(pSP);

                hres = pSP->QueryService(SID_SContainerDispatch, IID_IOleWindow, 
                                         (void **)&pOleWindow);
                if (SUCCEEDED(hres))
                {
                    pOleWindow->GetWindow(&hwnd);
                    pOleWindow->Release();
                }
                pSP->Release();
            }
        }
        else
        {
            //  It's either this or the functions we call should take NULL for HWNDs.
            hwnd = GetDesktopWindow();
        }
    }

    return hwnd;
}

HRESULT CShellUIHelper::_ConnectToTopLevelConnectionPoint(BOOL fConnect, IUnknown* punk)
{
    HRESULT hr = E_INVALIDARG;
    IConnectionPointContainer* pcpContainer;
    IServiceProvider*          psp;
    IServiceProvider*          psp2;

    ASSERT(punk);
    if (punk)
    {
        hr = punk->QueryInterface(IID_IServiceProvider, (void**)&psp);

        if (SUCCEEDED(hr))
        {
            hr = psp->QueryService(SID_STopLevelBrowser, IID_IServiceProvider, (void**) &psp2);

            if (SUCCEEDED(hr))
            {
                hr = psp2->QueryService(SID_SWebBrowserApp, IID_IConnectionPointContainer, (void **)&pcpContainer);

                if (SUCCEEDED(hr))
                {
                    //to avoid ambiguous reference
                    IDispatch* pdispThis;
                    this->QueryInterface(IID_IDispatch, (void **)&pdispThis);
                    ASSERT(pdispThis);
                
                    hr = ConnectToConnectionPoint(pdispThis, DIID_DWebBrowserEvents2, fConnect,
                                                  pcpContainer, &_dwcpCookie, NULL);
                    pcpContainer->Release();
                    pdispThis->Release();
                }
                psp2->Release();
            }
            psp->Release();
        }
    }
    
    return hr;
}

HRESULT CShellUIHelper::SetSite(IUnknown *punkSite)
{
    if (!_punkSite)
    {
        _ConnectToTopLevelConnectionPoint(TRUE, punkSite);
    } 
    else
    {
        ASSERT(punkSite == NULL);   //if we've already got _punkSite, we'd better be releasing the site

        _ConnectToTopLevelConnectionPoint(FALSE, _punkSite);
        ATOMICRELEASE(_punkSite);
    }
    
    _punkSite = punkSite;

    if (_punkSite)
        _punkSite->AddRef();

    return S_OK;
}

HRESULT CShellUIHelper::GetSite(REFIID riid, void **ppvSite)
{
    TraceMsg(DM_SHUIOC, "SHUO::GetSite called");

    if (_punkSite) 
        return _punkSite->QueryInterface(riid, ppvSite);

    *ppvSite = NULL;
    return E_FAIL;
}

STDMETHODIMP CShellUIHelper::GetInterfaceSafetyOptions(REFIID riid, DWORD *pdwSupportedOptions, 
                                             DWORD *pdwEnabledOptions)
{
    HRESULT hr = S_OK;

    if (!pdwSupportedOptions || !pdwEnabledOptions)
        return E_POINTER;

    if (IID_IDispatch == riid)
    {
        *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
        *pdwEnabledOptions = _dwSafety & INTERFACESAFE_FOR_UNTRUSTED_CALLER;
    }
    else
    {
        *pdwSupportedOptions = 0;
        *pdwEnabledOptions = 0;
        hr = E_NOINTERFACE;
    }

    return hr;
}


STDMETHODIMP CShellUIHelper::SetInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, 
                                             DWORD dwEnabledOptions)
{
    HRESULT hr = S_OK;
    
    if (IID_IDispatch == riid)
        _dwSafety = dwOptionSetMask & dwEnabledOptions;
    else
        hr = E_NOINTERFACE;

    return hr;
}


struct SHUI_STRMAP 
{
    LPCTSTR psz;
    int id;
};

int _MapStringToId(LPCTSTR pszStr, const SHUI_STRMAP* const psmap, int cel, int idDefault)
{
    if (pszStr)
    {
        for (int i=0; i<cel ; i++) 
        {
            if (StrCmpI(psmap[i].psz, pszStr) == 0) 
            {
                return psmap[i].id;
            }
        }
    }
    return idDefault;
}


LPCTSTR OptionalVariantToStr(VARIANT *pvar, LPTSTR pszBuf, UINT cchBuf)
{
    if (pvar->vt == VT_BSTR && pvar->bstrVal)
    {
        SHUnicodeToTChar(pvar->bstrVal, pszBuf, cchBuf);
        return pszBuf;
    }
    *pszBuf = 0;
    return NULL;
}

int OptionalVariantToInt(VARIANT *pvar, int iDefault)
{
    VARIANT v;
    VariantInit(&v);
    if (SUCCEEDED(VariantChangeType(&v, pvar, 0, VT_I4)))
    {
        iDefault = v.lVal;
        // VariantClear(&v);   // not needed, VT_I4 has no allocs
    }
    return iDefault;
}

//------------------------------------------------------------------------
STDMETHODIMP CShellUIHelper::AddFavorite(/* [in] */ BSTR strURL, /* [in][optional] */ VARIANT *Title)
{
    HRESULT hres = S_OK;
    LPITEMIDLIST pidl;
    BSTR bstrTemp = NULL;

    if (IsSpecialUrl(strURL))
    {
        bstrTemp = SysAllocString(strURL);
        if (bstrTemp)
        {
            hres = WrapSpecialUrl(&bstrTemp);
            if (SUCCEEDED(hres))
                strURL = bstrTemp;
        }
        else
            hres = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hres))
        hres = IECreateFromPath(strURL, &pidl);
    if (SUCCEEDED(hres)) 
    {
        TCHAR szTitle[MAX_PATH];
        hres = ::AddToFavorites(_GetOwnerWindow(), pidl, OptionalVariantToStr(Title, szTitle, ARRAYSIZE(szTitle)), TRUE, NULL, NULL);
        ILFree(pidl);
    }
    if (bstrTemp)
        SysFreeString(bstrTemp);
    return hres;
}

//------------------------------------------------------------------------

STDMETHODIMP CShellUIHelper::ShowChannel(IChannelMgrPriv *pChMgrPriv, LPWSTR pwszURL, HWND hwnd)
{
    HRESULT hres = E_FAIL;
    IServiceProvider *pSP1 = NULL,
                     *pSP2 = NULL;
    IWebBrowser2 *pWebBrowser2 = NULL;

    if (_punkSite)
    {
        hres = _punkSite->QueryInterface(IID_IServiceProvider, (void **)&pSP1);
        if (SUCCEEDED(hres))
        {
            ASSERT(pSP1);
            hres = pSP1->QueryService(SID_STopLevelBrowser,
                                      IID_IServiceProvider,
                                      (void**)&pSP2);
            if (SUCCEEDED(hres))
            {
                ASSERT(pSP2);
                hres = pSP2->QueryService(SID_SWebBrowserApp,
                                          IID_IWebBrowser2,
                                          (void**)&pWebBrowser2);
                ASSERT((SUCCEEDED(hres) && pWebBrowser2) || FAILED(hres));
                pSP2->Release();
            }
            pSP1->Release();
        }
    }

    if (FAILED(hres))
    {
#ifndef UNIX
        hres = CoCreateInstance(CLSID_InternetExplorer, NULL, CLSCTX_LOCAL_SERVER,
                              IID_IWebBrowser2, (void**)&pWebBrowser2);
#else
        hres = CoCreateInternetExplorer( IID_IWebBrowser2,
                                         CLSCTX_LOCAL_SERVER,
                                         (LPVOID*) &pWebBrowser2 );
#endif
    }

    if (SUCCEEDED(hres))
    {
        ASSERT(pWebBrowser2);
        hres = pChMgrPriv->ShowChannel(pWebBrowser2, pwszURL, hwnd);
        pWebBrowser2->Release();
    }
    
    return hres;
}

STDMETHODIMP CShellUIHelper::AddChannel(BSTR bstrURL)
{
    HRESULT hres;
    IChannelMgrPriv *pChMgrPriv;

    TCHAR szURL[MAX_URL_STRING];
    HWND hwnd;

    if (!bstrURL)
    {
        return E_INVALIDARG;
    }
    
    hwnd = _GetOwnerWindow();

    //
    //  As long as the underlying functions choke on NULL HWNDs then we may as well
    //  bail early.
    //
    if (hwnd)
    {
        if (!SHIsRestricted2W(hwnd, REST_NoChannelUI, NULL, 0) && !SHIsRestricted2W(hwnd, REST_NoAddingChannels, NULL, 0))
        {
                StrCpyNW(szURL, bstrURL, ARRAYSIZE(szURL));

            hres = JITCoCreateInstance(CLSID_ChannelMgr, 
                                    NULL,
                                    CLSCTX_INPROC_SERVER, 
                                    IID_IChannelMgrPriv, 
                                    (void **)&pChMgrPriv,
                                    hwnd,
                                    FIEF_FLAG_FORCE_JITUI);

            if (S_OK == hres)
            {
                ASSERT(pChMgrPriv);
            
                hres = pChMgrPriv->AddAndSubscribe(hwnd, bstrURL, NULL);
                if (hres == S_OK)
                {
                    hres = ShowChannel(pChMgrPriv, bstrURL, hwnd);
                }
                else if (FAILED(hres))
                {
                    IE_ErrorMsgBox(NULL, hwnd, hres, NULL, szURL, IDS_CHANNEL_UNAVAILABLE, MB_OK| MB_ICONSTOP);
                    hres = S_FALSE;
                }
                pChMgrPriv->Release();
            }
            else if (SUCCEEDED(hres))
            {
                hres = S_FALSE; // FAIL silently for now - throw dialog for indicating that reboot required if needed
            } else
            {
                IE_ErrorMsgBox(NULL, hwnd, hres, NULL, szURL, IDS_FAV_UNABLETOCREATE, MB_OK| MB_ICONSTOP);
                hres = S_FALSE;
            }
        }
        else
        {
            hres = S_FALSE;  // Failure code results in a script error.
        }
    }
    else    //  !hwnd
    {
        hres = E_FAIL;
    }
    return hres;
}

STDMETHODIMP GetHTMLDoc2(IUnknown *punk, IHTMLDocument2 **ppHtmlDoc)
{
    IServiceProvider   *pSP = NULL;
    IOleClientSite     *pClientSite = NULL;
    IOleContainer      *pContainer = NULL;
    HRESULT             hr = E_NOINTERFACE;

    *ppHtmlDoc = NULL;

    //  The window.external, jscript "new ActiveXObject" and the <OBJECT> tag
    //  don't take us down the same road.
    
    if (punk != NULL)
    {
        hr = punk->QueryInterface(IID_IOleClientSite, (void **)&pClientSite);
    }
    
    if (SUCCEEDED(hr))
    {
        //  <OBJECT> tag path

        ASSERT(pClientSite);

        hr = pClientSite->GetContainer(&pContainer);
        if (SUCCEEDED(hr))
        {
            ASSERT(pContainer);

            hr = pContainer->QueryInterface(IID_IHTMLDocument2, (void **)ppHtmlDoc);
            pContainer->Release();
        }
        
        if (FAILED(hr))
        {
            IWebBrowser2   *pWebBrowser2 = NULL;
            IDispatch      *pDispatch = NULL;

            //  window.external path
            hr = pClientSite->QueryInterface(IID_IServiceProvider, (void **)&pSP);
            if (SUCCEEDED(hr))
            {
                hr = pSP->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, 
                                       (void **)&pWebBrowser2);

                if (SUCCEEDED(hr))
                {
                    hr = pWebBrowser2->get_Document(&pDispatch);
                    if (SUCCEEDED(hr))
                    {
                        hr = pDispatch->QueryInterface(IID_IHTMLDocument2, (void **)ppHtmlDoc);
                        pDispatch->Release();
                    }
                    pWebBrowser2->Release();
                }
                pSP->Release();
            }
        }

        pClientSite->Release();
    }
    else
    {
        //  jscript path
        hr = punk->QueryInterface(IID_IServiceProvider, (void **)&pSP);

        if (SUCCEEDED(hr))
        {
            ASSERT(pSP);

            hr = pSP->QueryService(SID_SContainerDispatch, IID_IHTMLDocument2,
                                   (void **)ppHtmlDoc);
            pSP->Release();
        }
    }

    ASSERT(FAILED(hr) || (*ppHtmlDoc));
    return hr;
}

//A lot like GetHTMLDoc2 above, but only cares about window.external
STDMETHODIMP GetTopLevelBrowser(IUnknown *punk, IWebBrowser2 **ppwb2)
{
    IServiceProvider   *pSP = NULL;
    IOleClientSite     *pClientSite = NULL;
    HRESULT             hr = E_NOINTERFACE;

    *ppwb2 = NULL;

    hr = punk->QueryInterface(IID_IOleClientSite, (void **)&pClientSite);
    if (SUCCEEDED(hr))
    {
        ASSERT(pClientSite);

        hr = pClientSite->QueryInterface(IID_IServiceProvider, (void **)&pSP);
        if (SUCCEEDED(hr))
        {
            hr = pSP->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, (void **)ppwb2);
            pSP->Release();
        }

        pClientSite->Release();
    }

    ASSERT(FAILED(hr) || (*ppwb2));
    return hr;
}


STDMETHODIMP ZoneCheck(IUnknown *punkSite, BSTR bstrReqUrl)
{
    HRESULT hr = E_ACCESSDENIED;

    //  Return S_FALSE if we don't have a host site since we have no way of doing a 
    //  security check.  This is as far as VB 5.0 apps get.
    if (!punkSite)
        return S_FALSE;

    //  1)  Get an IHTMLDocument2 pointer
    //  2)  Get URL from doc
    //  3)  Create security manager
    //  4)  Check if doc URL zone is local, if so everything's S_OK
    //  5)  Otherwise, get and compare doc URL SID to requested URL SID

    IHTMLDocument2 *pHtmlDoc;
    if (SUCCEEDED(GetHTMLDoc2(punkSite, &pHtmlDoc)))
    {
        ASSERT(pHtmlDoc);
        BSTR bstrDocUrl;
        if (SUCCEEDED(pHtmlDoc->get_URL(&bstrDocUrl)))
        {
            ASSERT(bstrDocUrl);
            IInternetSecurityManager *pSecMgr;

            if (SUCCEEDED(CoCreateInstance(CLSID_InternetSecurityManager, 
                                           NULL, 
                                           CLSCTX_INPROC_SERVER,
                                           IID_IInternetSecurityManager, 
                                           (void **)&pSecMgr)))
            {
                ASSERT(pSecMgr);
                DWORD dwZoneID = URLZONE_UNTRUSTED;
                if (SUCCEEDED(pSecMgr->MapUrlToZone(bstrDocUrl, &dwZoneID, 0)))
                {
                    if (dwZoneID == URLZONE_LOCAL_MACHINE)
                        hr = S_OK;
                }
                if (hr != S_OK && bstrReqUrl)
                {
                    BYTE reqSid[MAX_SIZE_SECURITY_ID], docSid[MAX_SIZE_SECURITY_ID];
                    DWORD cbReqSid = ARRAYSIZE(reqSid);
                    DWORD cbDocSid = ARRAYSIZE(docSid);

                    if (   SUCCEEDED(pSecMgr->GetSecurityId(bstrReqUrl, reqSid, &cbReqSid, 0))
                        && SUCCEEDED(pSecMgr->GetSecurityId(bstrDocUrl, docSid, &cbDocSid, 0))
                        && (cbReqSid == cbDocSid)
                        && (memcmp(reqSid, docSid, cbReqSid) == 0))
                    {

                        hr = S_OK;
                    }
                }
                pSecMgr->Release();
            }
            SysFreeString(bstrDocUrl);
        }
        pHtmlDoc->Release();
    }
    else
    {
        //  If we don't have an IHTMLDocument2 we aren't running in a browser that supports
        //  our OM.  We shouldn't block in this case since we could potentially
        //  get here from other hosts (VB, WHS, etc.).
        hr = S_FALSE;
    }

    return hr;
}


STDMETHODIMP CShellUIHelper::IsSubscribed(BSTR bstrURL, VARIANT_BOOL* pBool)
{
    HRESULT hr;

    if (!bstrURL || !pBool)
    {
        return E_INVALIDARG;
    }

    hr = ZoneCheck(_punkSite, bstrURL);

    if (SUCCEEDED(hr))
    {
        ISubscriptionMgr *pSubscriptionMgr;

        hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER,
                              IID_ISubscriptionMgr, (void**)&pSubscriptionMgr);
        if (SUCCEEDED(hr))
        {
            ASSERT(pSubscriptionMgr);

            BOOL tmpBool;
            
            hr = pSubscriptionMgr->IsSubscribed(bstrURL, &tmpBool);
            *pBool = tmpBool ? VARIANT_TRUE : VARIANT_FALSE;
            
            pSubscriptionMgr->Release();
        }
    }

    return hr;
}

/****************************************************************************
 *
 *  AddDesktopComponentA - Adds a component to the desktop
 *
 *  ENTRY:
 *      hwnd - the parent for all UI
 *      pszUrlA - the URL of the component
 *      iCompType - one of COMP_TYPE_*
 *      iLeft, iTop, iWidth, iHeight - dimensions of the component
 *      dwFlags - additional flags
 *
 *  RETURNS:
 *      TRUE on success
 *      
 ****************************************************************************/
BOOL AddDesktopComponentW(HWND hwnd, LPCWSTR pszUrl, int iCompType,
                                    int iLeft, int iTop, int iWidth, int iHeight,
                                    DWORD dwFlags)
{
    COMPONENT Comp;
    BOOL    fRet = FALSE;
    HRESULT hres;

    Comp.dwSize = sizeof(Comp);

    //
    // Build the pcomp structure.
    //
    Comp.dwID = -1;
    Comp.iComponentType = iCompType;
    Comp.fChecked = TRUE;
    Comp.fDirty = FALSE;
    Comp.fNoScroll = FALSE;
    Comp.dwSize = SIZEOF(Comp);
    Comp.cpPos.dwSize = SIZEOF(COMPPOS);
    Comp.cpPos.iLeft = iLeft;
    Comp.cpPos.iTop = iTop;
    Comp.cpPos.dwWidth = iWidth;
    Comp.cpPos.dwHeight = iHeight;
    Comp.cpPos.izIndex = COMPONENT_TOP;
    Comp.cpPos.fCanResize = TRUE;
    Comp.cpPos.fCanResizeX = TRUE;
    Comp.cpPos.fCanResizeY = TRUE;
    Comp.cpPos.iPreferredLeftPercent = 0;
    Comp.cpPos.iPreferredTopPercent = 0;
    StrCpyNW(Comp.wszSource, pszUrl, ARRAYSIZE(Comp.wszSource));
    StrCpyNW(Comp.wszSubscribedURL, pszUrl, ARRAYSIZE(Comp.wszSource));
    StrCpyNW(Comp.wszFriendlyName, pszUrl, ARRAYSIZE(Comp.wszFriendlyName));
    Comp.dwCurItemState = IS_NORMAL;

    IActiveDesktop * piad;

    //
    // Add it to the system.
    //
    hres = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER, IID_IActiveDesktop, (LPVOID*)&piad);
    if (SUCCEEDED(hres))
    {
        dwFlags |= DTI_ADDUI_POSITIONITEM;
        piad->AddDesktopItemWithUI(hwnd, &Comp, dwFlags);
        piad->Release();
        fRet = TRUE;
    } 

    return fRet;
}

//------------------------------------------------------------------------

STDMETHODIMP CShellUIHelper::AddDesktopComponent(BSTR strURL, BSTR strType, 
            /* [optional, in] */ VARIANT *Left,
            /* [optional, in] */ VARIANT *Top,
            /* [optional, in] */ VARIANT *Width,
            /* [optional, in] */ VARIANT *Height)
{
#ifndef DISABLE_ACTIVEDESKTOP_FOR_UNIX

    HRESULT hres;
    int iType;

    if (StrCmpIW(strType, L"image")==0) 
    {
        iType = COMP_TYPE_PICTURE;
    } 
    else if (StrCmpIW(strType, L"website")==0) 
    {
        iType = COMP_TYPE_WEBSITE;
    }
    else
    {
        iType = 0;
    }

    if (iType) 
    {
        AddDesktopComponentW(_GetOwnerWindow(), strURL, iType,
                             OptionalVariantToInt(Left, -1),
                             OptionalVariantToInt(Top, -1),
                             OptionalVariantToInt(Width, -1),
                             OptionalVariantToInt(Height, -1),
                             DTI_ADDUI_DISPSUBWIZARD);
        hres = S_OK;
    }
    else 
    {
        hres = E_INVALIDARG;
    }
    return hres;
#else // #ifndef DISABLE_ACTIVEDESKTOP_FOR_UNIX

return E_INVALIDARG;

#endif // #ifndef DISABLE_ACTIVEDESKTOP_FOR_UNIX
}
void RemoveDefaultWallpaper();
STDMETHODIMP CShellUIHelper::ResetFirstBootMode()
{
#ifndef DISABLE_ACTIVEDESKTOP_FOR_UNIX

    if (MLShellMessageBox(
                        _GetOwnerWindow(),
                        MAKEINTRESOURCE(IDS_CONFIRM_RESETFLAG),
                        MAKEINTRESOURCE(IDS_TITLE),
                        MB_YESNO | MB_ICONQUESTION) == IDYES)
    {
        RemoveDefaultWallpaper();
        return S_OK;
    }

#endif // #ifndef DISABLE_ACTIVEDESKTOP_FOR_UNIX
    return S_FALSE;
}

// Little helper function used to change the safemode state
void SetSafeMode(DWORD dwFlags)
{
    IActiveDesktopP * piadp;

    HRESULT hres = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER, IID_IActiveDesktopP, (LPVOID*)&piadp);
    if (SUCCEEDED(hres))
    {
        piadp->SetSafeMode(dwFlags);
        piadp->Release();
    }
}

STDMETHODIMP CShellUIHelper::ResetSafeMode()
{
#ifndef DISABLE_ACTIVEDESKTOP_FOR_UNIX

    if ((ZoneCheck(_punkSite, NULL) == S_OK) || (MLShellMessageBox(
                       _GetOwnerWindow(),
                       MAKEINTRESOURCE(IDS_CONFIRM_RESET_SAFEMODE),
                       MAKEINTRESOURCE(IDS_TITLE),
                       MB_YESNO | MB_ICONQUESTION) == IDYES))
    {
        SetSafeMode(SSM_CLEAR | SSM_REFRESH);
        return S_OK;
    }

#endif // #ifndef DISABLE_ACTIVEDESKTOP_FOR_UNIX
    return S_FALSE;
}

STDMETHODIMP CShellUIHelper::RefreshOfflineDesktop()
{
#ifndef DISABLE_ACTIVEDESKTOP_FOR_UNIX
    IADesktopP2 * piad;

    HRESULT hres = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER, IID_IADesktopP2, (LPVOID*)&piad);
    if (SUCCEEDED(hres))
    {
        piad->UpdateAllDesktopSubscriptions();
        piad->Release();
    }
#endif // #ifndef DISABLE_ACTIVEDESKTOP_FOR_UNIX
    return S_OK;
}

STDMETHODIMP CShellUIHelper::GetTypeInfoCount(UINT * pctinfo)
{ 
    return E_NOTIMPL;
}

STDMETHODIMP CShellUIHelper::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{ 
    return E_NOTIMPL;
}

STDMETHODIMP CShellUIHelper::GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, 
                                      UINT cNames, LCID lcid, DISPID * rgdispid)
{ 
    HRESULT hr = CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);

    if (FAILED(hr))
    {
        IDispatch *pDisp = GetExternalDispatch();
        if (pDisp)
        {
            //  not one of ours, so let's see if it's our alternate external dispatch
            hr = pDisp->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
            if (SUCCEEDED(hr))
            {
                if (rgdispid[0] > 0)
                {
                    //  Offset there dispid
                    rgdispid[0] += DISPID_SHELLUIHELPERLAST;
                }
            }
        }
    }

    return hr;
}

HRESULT CShellUIHelper::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
        DISPPARAMS *pdispparams, VARIANT * pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr)
{
    HRESULT hr = DISP_E_MEMBERNOTFOUND;
    
    if ((dispidMember > 0) && (dispidMember <= DISPID_SHELLUIHELPERLAST))
    {
        hr = CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, 
                                   pvarResult, pexcepinfo, puArgErr);
    }
    else if (_fWaitingToFindText && (dispidMember == DISPID_DOCUMENTCOMPLETE))
    {
        ASSERT(pdispparams->rgvarg[1].vt == VT_DISPATCH);

        _fWaitingToFindText = FALSE;
        hr = _DoFindOnPage(pdispparams->rgvarg[1].pdispVal);
    }
    else
    {
        IDispatch *pDisp = GetExternalDispatch();
        if (pDisp)
        {
            if (dispidMember > 0)
            {
                //  Fixup the offset we added in GetIDsOfNames
                dispidMember -= DISPID_SHELLUIHELPERLAST;
            }
            hr = pDisp->Invoke(dispidMember, riid, lcid, wFlags, pdispparams, 
                                        pvarResult, pexcepinfo, puArgErr);
        }
    }

    return hr;                                   
}

STDMETHODIMP CShellUIHelper::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT hr;
    LCID lcid = GetSystemDefaultLCID();

    //  First see if it's ours
    hr = CImpIDispatch::GetIDsOfNames(IID_NULL, &bstrName, 1, lcid, pid);

    if (FAILED(hr))
    {
        //  otherwise try external IDispatchEx 
        if (_pExternalDispEx)
        {
            hr = _pExternalDispEx->GetDispID(bstrName, grfdex, pid);
        }
        //  finally try the external IDispatch
        else if (_pExternalDisp)
        {
            hr = _pExternalDisp->GetIDsOfNames(IID_NULL, &bstrName, 1, lcid, pid);
        }

        if (SUCCEEDED(hr) && (*pid > 0))
        {
            *pid += DISPID_SHELLUIHELPERLAST;
        }
    }

    return hr;
}

STDMETHODIMP CShellUIHelper::InvokeEx(DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
                                 VARIANT *pvarRes, EXCEPINFO *pei, 
                                 IServiceProvider *pspCaller)
{
    HRESULT hr = DISP_E_MEMBERNOTFOUND;
    UINT ArgErr;    //  inetsdk says this isn't used here

    //  First see if it's ours
    if ((id > 0) && (id <= DISPID_SHELLUIHELPERLAST))
    {
        hr = CImpIDispatch::Invoke(id, IID_NULL, lcid, wFlags, pdp, 
                                   pvarRes, pei, &ArgErr);
    }
    else
    {
        if (id > 0)
        {
            id -= DISPID_SHELLUIHELPERLAST;
        }
        //  otherwise try external IDispatchEx 
        if (_pExternalDispEx)
        {
            hr = _pExternalDispEx->InvokeEx(id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);
        }
        //  finally try the external IDispatch
        else if (_pExternalDisp)
        {
            hr = _pExternalDisp->Invoke(id, IID_NULL, lcid, wFlags, pdp, 
                                        pvarRes, pei, &ArgErr);
        }
    }
    
    return hr;
}

STDMETHODIMP CShellUIHelper::DeleteMemberByName(BSTR bstr, DWORD grfdex)
{
    HRESULT hr = E_NOTIMPL;
    
    if (_pExternalDispEx)
    {
        hr = _pExternalDispEx->DeleteMemberByName(bstr, grfdex);
    }

    return hr;
}
        
STDMETHODIMP CShellUIHelper::DeleteMemberByDispID(DISPID id)
{
    HRESULT hr = E_NOTIMPL;
    
    if (_pExternalDispEx)
    {
        hr = _pExternalDispEx->DeleteMemberByDispID(id);
    }

    return hr;
}
        
STDMETHODIMP CShellUIHelper::GetMemberProperties(DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    HRESULT hr = E_NOTIMPL;
    
    if (_pExternalDispEx)
    {
        hr = _pExternalDispEx->GetMemberProperties(id, grfdexFetch, pgrfdex);
    }

    return hr;
}
        
STDMETHODIMP CShellUIHelper::GetMemberName(DISPID id, BSTR *pbstrName)
{
    HRESULT hr = E_NOTIMPL;
    
    if (_pExternalDispEx)
    {
        hr = _pExternalDispEx->GetMemberName(id, pbstrName);
    }

    return hr;
}

STDMETHODIMP CShellUIHelper::GetNextDispID(DWORD grfdex, DISPID id, DISPID *pid)
{
    HRESULT hr = E_NOTIMPL;
    
    if (_pExternalDispEx)
    {
        hr = _pExternalDispEx->GetNextDispID(grfdex, id, pid);
    }

    return hr;
}
        
STDMETHODIMP CShellUIHelper::GetNameSpaceParent(IUnknown **ppunk)
{
    HRESULT hr = E_NOTIMPL;
    
    if (_pExternalDispEx)
    {
        hr = _pExternalDispEx->GetNameSpaceParent(ppunk);
    }

    return hr;
}

int GetIntFromReg(HKEY    hKey,
                  LPCTSTR lpszSubkey,
                  LPCTSTR lpszNameValue,
                  int     iDefault)
{
    TCHAR szValue[20];
    DWORD dwSizeofValueBuff = SIZEOF(szValue);
    int iRetValue = iDefault;
    DWORD dwType;

    if ((SHGetValue(hKey, lpszSubkey, lpszNameValue, &dwType,(LPBYTE)szValue,
                   &dwSizeofValueBuff) == ERROR_SUCCESS) && dwSizeofValueBuff)
    {
        if (dwType == REG_SZ)
        {
            iRetValue = (int)StrToInt(szValue);
        }
    }

    return iRetValue;
}

void GetRegLocation(LPTSTR lpszResult, DWORD cchResult, LPCTSTR lpszKey, LPCTSTR lpszScheme)
{
    TCHAR szSubkey[MAX_PATH];
    DWORD dwDataLength = sizeof(szSubkey) - 2 * sizeof(TCHAR);
    DWORD dwType;

    StrCpyN(szSubkey, TEXT("\\"), ARRAYSIZE(szSubkey));
    if (lpszScheme)
        StrCatBuff(szSubkey, lpszScheme, ARRAYSIZE(szSubkey));
    else
        SHGetValue(HKEY_CURRENT_USER, REG_DESKCOMP_SCHEME, REG_VAL_SCHEME_DISPLAY, &dwType,
            (LPBYTE)(szSubkey) + sizeof(TCHAR), &dwDataLength);
    if (szSubkey[1])
        StrCatBuff(szSubkey, TEXT("\\"), ARRAYSIZE(szSubkey));

    wnsprintf(lpszResult, cchResult, lpszKey, szSubkey);
}

#define c_szWallpaper  REG_VAL_GENERAL_WALLPAPER
void RemoveDefaultWallpaper()
{
    // Read the Wallpaper from the Old location.
    TCHAR   szOldWallpaper[MAX_PATH];
    DWORD dwType;
    DWORD dwSize = SIZEOF(szOldWallpaper);
    if (SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_DESKTOP, c_szWallpaper, &dwType, szOldWallpaper, &dwSize) != ERROR_SUCCESS)
        szOldWallpaper[0] = TEXT('\0');

    // Read the wallpaper style
    DWORD dwWallpaperStyle = GetIntFromReg(HKEY_CURRENT_USER, REGSTR_PATH_DESKTOP, REG_VAL_GENERAL_TILEWALLPAPER, WPSTYLE_TILE);

    TCHAR szDeskcomp[MAX_PATH];
    GetRegLocation(szDeskcomp, ARRAYSIZE(szDeskcomp), REG_DESKCOMP_GENERAL, NULL);

    // Set the old wallpaper into the new location.
    SHSetValue(HKEY_CURRENT_USER, szDeskcomp,
        c_szWallpaper, REG_SZ, (LPBYTE)szOldWallpaper, SIZEOF(szOldWallpaper));


//  98/08/14 vtan: This used to write out a REG_DWORD. It should've
//  written out a REG_SZ.

    TCHAR   szWallpaperStyle[2];

    (TCHAR*)StrCpyN(szWallpaperStyle, TEXT("0"), sizeof(szWallpaperStyle));
    szWallpaperStyle[0] += static_cast<TCHAR>(dwWallpaperStyle & WPSTYLE_MAX);

    // Set the old wallpaper style into the new location.
    SHSetValue(HKEY_CURRENT_USER, szDeskcomp,
        REG_VAL_GENERAL_TILEWALLPAPER, REG_SZ, (LPBYTE)szWallpaperStyle, SIZEOF(szWallpaperStyle));

//  98/08/14 vtan #196226: Moved the create instance of IActiveDesktop
//  to here from entry of the function. When the instance is created the
//  registry information is cached in the member variable data. The
//  registry information for the wallpaper is then changed behind the
//  object instance and ApplyChanges() is called on STALE information.
//  By deferring the object instantiation to after the registry changes
//  the changes are applied correctly.

    IActiveDesktop * piad;
    HRESULT hres = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER, IID_IActiveDesktop, (LPVOID*)&piad);
    if (SUCCEEDED(hres))
    {
        piad->ApplyChanges(AD_APPLY_FORCE | AD_APPLY_HTMLGEN | AD_APPLY_REFRESH);
        piad->Release();
    }
}

HRESULT GetTargetFrame(IUnknown* _punkSite, BSTR bstrFrame, IWebBrowser2** ppunkTargetFrame)
{
    IWebBrowser2* pwb2;
    HRESULT hr = GetTopLevelBrowser(_punkSite, &pwb2);

    if (SUCCEEDED(hr))
    {
        LPTARGETFRAME2 pOurTargetFrame = NULL;
        IUnknown *punkTargetFrame = NULL;

        // See if there is an existing frame with the specified target name
        hr = TargetQueryService(pwb2, IID_ITargetFrame2, (LPVOID *) &pOurTargetFrame);
        if (SUCCEEDED(hr))
        {
            hr = pOurTargetFrame->FindFrame(bstrFrame, FINDFRAME_JUSTTESTEXISTENCE, &punkTargetFrame);
            if (SUCCEEDED(hr) && punkTargetFrame)
            {
                // yes, we found a frame with that name.  QI for the automation
                // interface on that frame.
                hr = punkTargetFrame->QueryInterface(IID_IWebBrowser2, (LPVOID *)ppunkTargetFrame);
                punkTargetFrame->Release();
            }
            pOurTargetFrame->Release();
        }
        pwb2->Release();
    }
    return hr;
}

// NavigateAndFind
// 1. navigate the specified target frame to the specified url
// 2. set _fWaitingToFindText so that on DocumentComplete, 
//    mimic the find dialog and select/highlight the specified text
STDMETHODIMP CShellUIHelper::NavigateAndFind(BSTR URL, BSTR strQuery, VARIANT* varTargetFrame)
{
    HRESULT hr;
    IWebBrowser2* pwb2;
    BSTR bstrFrameSrc = NULL;

    if (_bstrQuery)
        SysFreeString(_bstrQuery);
    _bstrQuery = SysAllocString(strQuery);
     
    BSTR bstrTemp = NULL;
    if (IsSpecialUrl(URL))
    {
        bstrTemp = SysAllocString(URL);
        if (bstrTemp)
        {
            hr = WrapSpecialUrl(&bstrTemp);
            if (SUCCEEDED(hr))
                URL = bstrTemp;
        }
        else
            hr = E_OUTOFMEMORY;

        // Security:  Don't allow javascript on one web page
        //            to automatically execute in the zone
        //            of the target frame URL.
        if (SUCCEEDED(hr) && varTargetFrame)
        {
            hr = GetTargetFrame(_punkSite, varTargetFrame->bstrVal, &pwb2);
            if (SUCCEEDED(hr))
            {
                hr = pwb2->get_LocationURL(&bstrFrameSrc);

                if (SUCCEEDED(hr))
                {
                    IHTMLDocument2 *pHtmlDoc;
                    if (SUCCEEDED(GetHTMLDoc2(_punkSite, &pHtmlDoc)))
                    {
                        BSTR bstrDocUrl;
                        if (SUCCEEDED(pHtmlDoc->get_URL(&bstrDocUrl)))
                        {
                            hr = AccessAllowed(bstrDocUrl, bstrFrameSrc) ? S_OK : E_ACCESSDENIED;
                            SysFreeString(bstrDocUrl);
                        }
                        pHtmlDoc->Release();
                    }
                    SysFreeString(bstrFrameSrc);
                }
                pwb2->Release();
                pwb2 = NULL;
            }
        }
    }

    if (SUCCEEDED(hr))
        hr = GetTopLevelBrowser(_punkSite, &pwb2);

    if (SUCCEEDED(hr))
    {
        _fWaitingToFindText = TRUE;
        pwb2->Navigate(URL, PVAREMPTY, varTargetFrame, PVAREMPTY, PVAREMPTY);
        pwb2->Release();
    }

    if (bstrTemp)
        SysFreeString(bstrTemp);
        
    //script runtime error protection
    return SUCCEEDED(hr) ? S_OK : S_FALSE;
}


// AutoScan
//
// Takes the search string and tries to navigate to www.%s.com, www.%s.org etc.  If all of
// these fail then we navigate to pvarTargetFrame.
//
STDMETHODIMP CShellUIHelper::AutoScan
(
    BSTR strSearch,             // String to autoscan
    BSTR strFailureUrl,         // url to display is search fails
    VARIANT* pvarTargetFrame    // [optional] target frame
)
{
    HRESULT hr = E_FAIL;
    IWebBrowser2* pwb2;

    // Don't bother autoscanning if there are extended characters in the search string
    if (!HasExtendedChar(strSearch))
    {
        //first, check to see if the url is trying to spoof /1 security
        BSTR bstrTemp = NULL;
        if (IsSpecialUrl(strFailureUrl))
        {
            bstrTemp = SysAllocString(strFailureUrl);
            if (bstrTemp)
            {
                hr = WrapSpecialUrl(&bstrTemp);
                if (SUCCEEDED(hr))
                    strFailureUrl = bstrTemp;
            }
            else
                hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = S_OK;
        }

        if (SUCCEEDED(hr))
        {
            LPWSTR pszTarget = (pvarTargetFrame && pvarTargetFrame->vt == VT_BSTR && pvarTargetFrame->bstrVal) ?
                                    pvarTargetFrame->bstrVal : L"_main";

            hr = GetTargetFrame(_punkSite, pszTarget, &pwb2);
            if (SUCCEEDED(hr))
            {
                // We don't want to navigate to strSearch.  Start searching using the 
                // first autoscan substitution.
                WCHAR szFormat[MAX_PATH];
                WCHAR szUrl[MAX_URL_STRING];
                if (GetSearchFormatString(1, szFormat, sizeof(szFormat)) != ERROR_SUCCESS)
                {
                    hr = E_FAIL;
                }
                else
                {
                    wnsprintf(szUrl, ARRAYSIZE(szUrl), szFormat, strSearch);
                    BSTRBLOB bstrBlob;
                    bstrBlob.cbSize = lstrlenW(szUrl) * sizeof(WCHAR);
                    bstrBlob.pData = (BYTE*)szUrl;
                    BSTR bstrUrl = (BSTR)bstrBlob.pData;

                    // Save the original search string for autoscanning.  Normally
                    // this come from the addressbar, but in our case we store it as
                    // a property
                    VARIANT v;
                    VariantInit (&v);

                    v.vt = VT_BSTR;
                    v.bstrVal = strSearch;
                    pwb2->PutProperty((BSTR)s_sstrSearch.wsz, v);

                    // Save the error page in case the scan fails
                    v.vt = VT_BSTR;
                    v.bstrVal = strFailureUrl;
                    pwb2->PutProperty((BSTR)s_sstrFailureUrl.wsz, v);

                    // Navigate with autosearch enabled
                    VARIANT vFlags;
                    vFlags.vt = VT_I4;
                    vFlags.lVal = navAllowAutosearch;

                    pwb2->Navigate(bstrUrl, &vFlags, pvarTargetFrame, PVAREMPTY, PVAREMPTY);
                }
                pwb2->Release();
            }
        }
        if (bstrTemp)
            SysFreeString(bstrTemp);
    }

    //script runtime error protection
    return SUCCEEDED(hr) ? S_OK : S_FALSE;
}

typedef HRESULT (*PFNSHOWBROWSERUI)(IUnknown *punkSite, HWND hwnd, VARIANT *pvarIn, VARIANT *pvarOut);

typedef void (*PFNOPENLANGUAGEDIALOG)(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow);

HRESULT ShowLanguageDialog(IUnknown *punkSite, HWND hwnd, VARIANT *pvarIn, VARIANT *pvarOut)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hInstInetcpl = LoadLibrary(TEXT("inetcpl.cpl"));

    if (hInstInetcpl)
    {
        
        PFNOPENLANGUAGEDIALOG pfnOpenLanguageDialog = 
            (PFNOPENLANGUAGEDIALOG)GetProcAddress( hInstInetcpl, "OpenLanguageDialog" );

        if (pfnOpenLanguageDialog)
        {
            pfnOpenLanguageDialog(hwnd, NULL, NULL, SW_SHOW);
            hr = S_OK;
        }

        FreeLibrary(hInstInetcpl);
    }

    return hr;
}

HRESULT ShowOrganizeFavorites(IUnknown *punkSite, HWND hwnd, VARIANT *pvarIn, VARIANT *pvarOut)
{
    return DoOrganizeFavDlgW(hwnd, NULL) ? S_OK : E_FAIL;
}

struct BROWSERUI_MAP
{
    LPWSTR pwszName;
    PFNSHOWBROWSERUI pfnShowBrowserUI;
};

BROWSERUI_MAP s_browserUIMap[] =
{
    { L"LanguageDialog",        ShowLanguageDialog      },
    { L"OrganizeFavorites",     ShowOrganizeFavorites   }
};

STDMETHODIMP CShellUIHelper::ShowBrowserUI(BSTR bstrName, VARIANT *pvarIn, VARIANT *pvarOut)
{
    HRESULT hr = E_FAIL;

    for (int i = 0; i < ARRAYSIZE(s_browserUIMap); i++)
    {
        if (pvarOut)
        {
            VariantInit(pvarOut);
        }
        
        if (0 == StrCmpIW(s_browserUIMap[i].pwszName, bstrName))
        {
            hr = s_browserUIMap[i].pfnShowBrowserUI(_punkSite, _GetOwnerWindow(), pvarIn, pvarOut);
        }
    }

    return S_OK == hr ? S_OK : S_FALSE;
}


// the find dialog does the following:
// 
// rng = document.body.createTextRange();
// if (rng.findText("Find this text"))
//     rng.select();
STDMETHODIMP CShellUIHelper::_DoFindOnPage(IDispatch* pdisp)
{
    HRESULT           hr;
    IWebBrowser2*     pwb2;
    IDispatch*        pdispDocument;
    IHTMLDocument2*   pDocument;
    IHTMLElement*     pBody;
    IHTMLBodyElement* pBodyElement;
    IHTMLTxtRange*    pRange;

    ASSERT(pdisp);
    if (!pdisp)
        return E_FAIL;

    hr = pdisp->QueryInterface(IID_IWebBrowser2, (void**)&pwb2);

    if (SUCCEEDED(hr))
    {
        hr = pwb2->get_Document(&pdispDocument);

        if (SUCCEEDED(hr) && (NULL != pdispDocument))
        {
            hr = pdispDocument->QueryInterface(IID_IHTMLDocument2, (void**)&pDocument);
            
            if (SUCCEEDED(hr))
            {
                hr = pDocument->get_body(&pBody);
                
                if (SUCCEEDED(hr) && (NULL != pBody))
                {
                    hr = pBody->QueryInterface(IID_IHTMLBodyElement, (void**)&pBodyElement);

                    if (SUCCEEDED(hr))
                    {
                        hr = pBodyElement->createTextRange(&pRange);

                        if (SUCCEEDED(hr) && (NULL != pRange))
                        {
                            VARIANT_BOOL vbfFoundText;
                            
                            hr = pRange->findText(_bstrQuery, 1000000, 0, &vbfFoundText);

                            if (SUCCEEDED(hr) && (vbfFoundText == VARIANT_TRUE))
                            {
                                hr = pRange->select();
                            }
                            
                            pRange->Release();
                        }
                        pBodyElement->Release();
                    }
                    pBody->Release();
                }
                pDocument->Release();
            }
            pdispDocument->Release();
        }
        pwb2->Release();
    }
    return hr;
}

//
// Launch the favorites import/export wizard
//
STDMETHODIMP CShellUIHelper::ImportExportFavorites(VARIANT_BOOL fImport, BSTR strImpExpPath) 
{
    //don't allow to import/export to folders other than Favorites from OM
    DoImportOrExport(fImport==VARIANT_TRUE, NULL, (LPCWSTR)strImpExpPath, TRUE);
    return S_OK;
}

//
// Save the form data via intelliforms
//
STDMETHODIMP CShellUIHelper::AutoCompleteSaveForm(VARIANT *Form)
{
    HRESULT hrRet = S_FALSE;

    IHTMLDocument2 *pDoc2=NULL;

    GetHTMLDoc2(_punkSite, &pDoc2);
    
    if (pDoc2)
    {
        hrRet = IntelliFormsSaveForm(pDoc2, Form);
        pDoc2->Release();
    }

    return hrRet;
}

//
// Attach intelliforms to this document
//
STDMETHODIMP CShellUIHelper::AutoCompleteAttach(VARIANT *Reserved)
{
    HRESULT hr=E_FAIL;

    if (_pvIntelliForms == NULL)
    {
        IHTMLDocument2 *pDoc2=NULL;

        GetHTMLDoc2(_punkSite, &pDoc2);

        if (pDoc2)
        {
            hr = S_OK;
            AttachIntelliForms(NULL, _GetOwnerWindow(), pDoc2, &_pvIntelliForms);
            pDoc2->Release();
        }
    }

    return SUCCEEDED(hr) ? S_OK : S_FALSE;
}
