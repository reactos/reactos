//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       history.cxx
//
//  Contents:   Implementation of COmLocation, COmHistory and COmNavigator objects.
//
//  Synopsis:   COmWindow2 uses instances of this class to provide expando properties
//              to the browser impelemented window.location, window.history, and 
//              window.navigator objects. 
//
//              The typelib for those objects are in mshtml.dll which we use via the
//              standard CBase mechanisms and the CLASSDESC specifiers.
//              The browser's implementation of those objects are the only interface
//              to those objects that is ever exposed externally.  The browser delegates
//              to us for the few calls that require our expando support.
//
//              Instances of these classes are also used to provide minimal implementation
//              for non-browser scenarios like Athena
//
//----------------------------------------------------------------------------


#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#define _cxx_
#include "history.hdl"

MtDefine(COmLocation, ObjectModel, "COmLocation")
MtDefine(COmLocationGetUrlComponent, Utilities, "COmLocation::GetUrlComponent")
MtDefine(COmHistory, ObjectModel, "COmHistory")
MtDefine(COmNavigator, ObjectModel, "COmNavigator")
MtDefine(COpsProfile, ObjectModel, "COpsProfile")
MtDefine(CPlugins, ObjectModel, "CPlugins")
MtDefine(CMimeTypes, ObjectModel, "CMimeTypes")

//+-------------------------------------------------------------------------
//
//  COmLocation - implementation for the window.location object
//
//--------------------------------------------------------------------------

COmLocation::COmLocation(COmWindow2 *pWindow)
{
    Assert(pWindow);
    _pWindow = pWindow;
}

ULONG COmLocation::PrivateAddRef(void)
{
    return _pWindow->SubAddRef();
}

ULONG COmLocation::PrivateRelease(void)
{
    return _pWindow->SubRelease();
}

HRESULT
COmLocation::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IObjectIdentity, NULL)
        QI_TEAROFF(this, IHTMLLocation, NULL)
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    (*(IUnknown **)ppv)->AddRef();
    return S_OK;
}

const COmLocation::CLASSDESC COmLocation::s_classdesc =
{
    &CLSID_HTMLLocation,                 // _pclsid
    0,                                   // _idrBase
#ifndef NO_PROPERTY_PAGE
    0,                                   // _apClsidPages
#endif // NO_PROPERTY_PAGE
    0,                                   // _pcpi
    0,                                   // _dwFlags
    &IID_IHTMLLocation,                  // _piidDispinterface
    &s_apHdlDescs,                       // _apHdlDesc
};

HRESULT
COmLocation::GetUrlComponent(BSTR *pstrComp, URLCOMP_ID ucid, TCHAR **ppchUrl, DWORD dwFlags)
{
    HRESULT  hr = S_OK;
    TCHAR   cBuf[pdlUrlLen];
    TCHAR  * pchNewUrl = cBuf;

    // make sure we have at least one place to return a value
    Assert(!(pstrComp && ppchUrl));
    if (!pstrComp && !ppchUrl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (ppchUrl)
        *ppchUrl = NULL;
    else
        *pstrComp = NULL;

    // get the expanded string 
    hr = THR(_pWindow->_pDoc->ExpandUrl(_pWindow->_pDoc->_cstrUrl, ARRAY_SIZE(cBuf), pchNewUrl, NULL));
    if (hr || !*pchNewUrl)
        goto Cleanup;

    // if asking for whole thing, just set return param
    if (ucid == URLCOMP_WHOLE)
    {
        if (ppchUrl)
        {
            *ppchUrl = pchNewUrl;
            pchNewUrl = NULL;          // to avoid cleanup
        }
        else
        {
            hr = THR(FormsAllocString(pchNewUrl, pstrComp));
        }
    }
    else
    {
        // we want a piece, so split it up.
        CStr cstrComponent; 

        hr = THR(GetUrlComponentHelper(pchNewUrl, &cstrComponent, dwFlags, ucid));
        if (hr)
            goto Cleanup;

        if (ppchUrl)
        {
            if (cstrComponent)
                hr = THR(MemAllocString(Mt(COmLocationGetUrlComponent),
                        cstrComponent, ppchUrl));
            else
                *ppchUrl = NULL;
        }
        else
        {
            hr = THR(cstrComponent.AllocBSTR(pstrComp));
        }
    }

Cleanup:
    RRETURN (hr);
}

//+-----------------------------------------------------------
//
//  Member  : SetUrlComponenet
//
//  Synopsis    : field the various component setting requests
//
//-----------------------------------------------------------

HRESULT
COmLocation::SetUrlComponent(const BSTR bstrComp, URLCOMP_ID ucid)
{
    HRESULT hr = S_OK;
    TCHAR achUrl[pdlUrlLen];
    TCHAR *pchOldUrl = NULL;

    // if set_href, just set it
    if (ucid == URLCOMP_WHOLE)
    {
        hr = THR(_pWindow->_pDoc->FollowHyperlink(bstrComp));
    }
    else
    {
        // get the old url
        hr = THR(GetUrlComponent(NULL, URLCOMP_WHOLE, &pchOldUrl, ICU_DECODE));

        if (hr || !pchOldUrl)
            goto Cleanup;

        // expand it if necessary
        if ((ucid != URLCOMP_HASH) && (ucid != URLCOMP_SEARCH))
        {
            // and set the appropriate component
            hr = THR(SetUrlComponentHelper(pchOldUrl,
                                           achUrl,
                                           ARRAY_SIZE(achUrl),
                                           &bstrComp,
                                           ucid));
        }
        else
        {
            hr = THR(ShortCutSetUrlHelper(pchOldUrl,
                                   achUrl,
                                   ARRAY_SIZE(achUrl),
                                   &bstrComp,
                                   ucid));
        }
        if (hr)
            goto Cleanup;

        hr = THR(_pWindow->_pDoc->FollowHyperlink(bstrComp));

        IGNORE_HR(_pWindow->_pDoc->Fire_PropertyChangeHelper(DISPID_CDoc_location,0));
    }
                                            
Cleanup:
    if (pchOldUrl)
        MemFreeString(pchOldUrl);

    RRETURN(hr);
}

HRESULT COmLocation::put_href(BSTR v)
{
    RRETURN(SetErrorInfo(SetUrlComponent(v, URLCOMP_WHOLE)));
}

HRESULT COmLocation::get_href(BSTR *p)
{
    RRETURN(SetErrorInfo(GetUrlComponent(p, URLCOMP_WHOLE, NULL, ICU_DECODE)));
}

HRESULT COmLocation::put_protocol(BSTR v)
{
    RRETURN(SetErrorInfo(SetUrlComponent(v, URLCOMP_PROTOCOL)));
}

HRESULT COmLocation::get_protocol(BSTR *p)
{
    RRETURN(SetErrorInfo(GetUrlComponent(p, URLCOMP_PROTOCOL, NULL, ICU_DECODE)));
}

HRESULT COmLocation::put_host(BSTR v)
{
    RRETURN(SetErrorInfo(SetUrlComponent(v, URLCOMP_HOST)));
}

HRESULT COmLocation::get_host(BSTR *p)
{
    RRETURN(SetErrorInfo(GetUrlComponent(p, URLCOMP_HOST, NULL, ICU_DECODE)));
}

HRESULT COmLocation::put_hostname(BSTR v)
{
    RRETURN(SetErrorInfo(SetUrlComponent(v, URLCOMP_HOSTNAME)));
}

HRESULT COmLocation::get_hostname(BSTR *p)
{
    RRETURN(SetErrorInfo(GetUrlComponent(p, URLCOMP_HOSTNAME, NULL, ICU_DECODE)));
}

HRESULT COmLocation::put_port(BSTR v)
{
    RRETURN(SetErrorInfo(SetUrlComponent(v, URLCOMP_PORT)));
}

HRESULT COmLocation::get_port(BSTR *p)
{
    RRETURN(SetErrorInfo(GetUrlComponent(p, URLCOMP_PORT, NULL, ICU_DECODE)));
}

HRESULT COmLocation::put_pathname(BSTR v)
{
    RRETURN(SetErrorInfo(SetUrlComponent(v, URLCOMP_PATHNAME)));
}

HRESULT COmLocation::get_pathname(BSTR *p)
{
    RRETURN(SetErrorInfo(GetUrlComponent(p, URLCOMP_PATHNAME, NULL, ICU_DECODE)));
}

HRESULT COmLocation::put_search(BSTR v)
{
    RRETURN(SetErrorInfo(SetUrlComponent(v, URLCOMP_SEARCH)));
}

HRESULT COmLocation::get_search(BSTR *p)
{
    RRETURN(SetErrorInfo(GetUrlComponent(p, URLCOMP_SEARCH, NULL, ICU_DECODE)));
}

HRESULT COmLocation::put_hash(BSTR v)
{
    RRETURN(SetErrorInfo(SetUrlComponent(v, URLCOMP_HASH)));
}

HRESULT COmLocation::get_hash(BSTR *p)
{
    RRETURN(SetErrorInfo(GetUrlComponent(p, URLCOMP_HASH, NULL, ICU_DECODE)));
}

HRESULT COmLocation::reload(VARIANT_BOOL flag)
{
    LONG lOleCmdidf;

    if (flag)
        lOleCmdidf = OLECMDIDF_REFRESH_COMPLETELY|OLECMDIDF_REFRESH_CLEARUSERINPUT;
    else
        lOleCmdidf = OLECMDIDF_REFRESH_NO_CACHE|OLECMDIDF_REFRESH_CLEARUSERINPUT;

    RRETURN(SetErrorInfo(_pWindow->_pDoc->ExecRefresh(lOleCmdidf)));
}

HRESULT COmLocation::replace(BSTR bstr)
{
    // NOP
    RRETURN(S_OK);
}

HRESULT COmLocation::assign(BSTR bstr)
{
    RRETURN(put_href(bstr));
}

HRESULT COmLocation::toString(BSTR * pbstr)
{
    RRETURN(get_href(pbstr));
}

//+------------------------------------------------------------------------
//
//  Method: COmLocation : IsEqualObject
//
//  Synopsis; IObjectIdentity method implementation
//
//  Returns : S_OK if objects are equal, E_FAIL otherwise
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
COmLocation::IsEqualObject( IUnknown * pUnk)
{
    HRESULT          hr = E_POINTER;
    IHTMLLocation  * pLoc = NULL;

    if (!pUnk)
        goto Cleanup;

    // if we're hosted this returns the host's imple, otherwise it is a ptr 
    // to us.
    hr = THR(_pWindow->get_location(&pLoc));
    if (!pLoc || hr)
        goto Cleanup;

    // are the unknowns equal?
    hr = (IsSameObject(pUnk, pLoc)) ? S_OK : S_FALSE;

Cleanup:
    ReleaseInterface(pLoc);
    RRETURN1(hr, S_FALSE);
}



//+-------------------------------------------------------------------------
//
//  COmHistory - implementation for the window.history object
//
//--------------------------------------------------------------------------

COmHistory::COmHistory(COmWindow2 *pWindow)
{
    Assert(pWindow);
    _pWindow = pWindow;
}

ULONG COmHistory::PrivateAddRef(void)
{
    return _pWindow->SubAddRef();
}

ULONG COmHistory::PrivateRelease(void)
{
    return _pWindow->SubRelease();
}

HRESULT COmHistory::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT hr=S_OK;

    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IObjectIdentity, NULL)

    default:
        if (iid == IID_IOmHistory)
            hr = THR(CreateTearOffThunk(this, COmHistory::s_apfnIOmHistory, NULL, ppv));
    }

    if (!hr)
    {
        if (*ppv)
            (*(IUnknown **)ppv)->AddRef();
        else
            hr = E_NOINTERFACE;
    }
    RRETURN(hr);
}

const COmHistory::CLASSDESC COmHistory::s_classdesc =
{
    &CLSID_HTMLHistory,                  // _pclsid
    0,                                   // _idrBase
#ifndef NO_PROPERTY_PAGE
    0,                                   // _apClsidPages
#endif // NO_PROPERTY_PAGE
    0,                                   // _pcpi
    0,                                   // _dwFlags
    &IID_IOmHistory,                     // _piidDispinterface
    &s_apHdlDescs,                       // _apHdlDesc
};

// CHROME
// Get the containters OmHistory interface if we are Chrome hosted and
// have a client site.
// NOTE: If we are not Chrome hosted this function returns E_FAIL.

HRESULT COmHistory::GetChromeSiteHistory(IOmHistory **ppiChromeSiteHistory)
{
    HRESULT hr = E_FAIL;

    if ((NULL != _pWindow) &&
        (NULL != _pWindow->_pDoc) &&
        (_pWindow->_pDoc->IsChromeHosted()) &&
        (NULL != _pWindow->_pDoc->_pClientSite))
    {
        hr = THR(_pWindow->_pDoc->_pClientSite->QueryInterface(IID_IOmHistory, 
                                                               (VOID **)ppiChromeSiteHistory));
    }
    
    return hr;
}

HRESULT COmHistory::get_length(short *p)
{
    // CHROME
    // Forward history calls to the chrome site, yet keep the 
    // stock history object's aggregation state intact.
    // NOTE: GetChromeSiteHistory will fail if not Chrome hosted
    IOmHistory *piChromeSiteHistory = NULL;
    HRESULT hr = THR(GetChromeSiteHistory(&piChromeSiteHistory));
    if (SUCCEEDED(hr))
    {
        hr = THR(piChromeSiteHistory->get_length(p));
        piChromeSiteHistory->Release();
        RRETURN(hr);
    }

    if (p)
        *p = 0;

    RRETURN(S_OK);
}

HRESULT COmHistory::back(VARIANT *pvargdistance)
{
    // CHROME
    // Forward history calls to the chrome site, yet keep the 
    // stock history object's aggregation state intact.
    // NOTE: GetChromeSiteHistory will fail if not Chrome hosted
    IOmHistory *piChromeSiteHistory = NULL;
    HRESULT hr = THR(GetChromeSiteHistory(&piChromeSiteHistory));
    if (SUCCEEDED(hr))
    {
        hr = THR(piChromeSiteHistory->back(pvargdistance));
        piChromeSiteHistory->Release();
        RRETURN(hr);
    }

    RRETURN(S_OK);
}

HRESULT COmHistory::forward(VARIANT *pvargdistance)
{
    // CHROME
    // Forward history calls to the chrome site, yet keep the 
    // stock history object's aggregation state intact.
    // NOTE: GetChromeSiteHistory will fail if not Chrome hosted
    IOmHistory *piChromeSiteHistory = NULL;
    HRESULT hr = THR(GetChromeSiteHistory(&piChromeSiteHistory));
    if (SUCCEEDED(hr))
    {
        hr = THR(piChromeSiteHistory->forward(pvargdistance));
        piChromeSiteHistory->Release();
        RRETURN(hr);
    }

    RRETURN(S_OK);
}

HRESULT COmHistory::go(VARIANT *pvargdistance)
{
    // CHROME
    // Forward history calls to the chrome site, yet keep the 
    // stock history object's aggregation state intact.
    // NOTE: GetChromeSiteHistory will fail if not Chrome hosted
    IOmHistory *piChromeSiteHistory = NULL;
    HRESULT hr = THR(GetChromeSiteHistory(&piChromeSiteHistory));
    if (SUCCEEDED(hr))
    {
        hr = THR(piChromeSiteHistory->go(pvargdistance));
        piChromeSiteHistory->Release();
        RRETURN(hr);
    }

    RRETURN(S_OK);
}

//+------------------------------------------------------------------------
//
//  Method: COmhistory: IsEqualObject
//
//  Synopsis; IObjectIdentity method implementation
//
//  Returns : S_OK if objects are equal, E_FAIL otherwise
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
COmHistory::IsEqualObject( IUnknown * pUnk)
{
    HRESULT      hr = E_POINTER;
    IOmHistory   * pHist= NULL;

    if (!pUnk)
        goto Cleanup;

    // if we're hosted this returns the host's imple, otherwise it is a ptr 
    // to us.
    hr = THR(_pWindow->get_history(&pHist));
    if (!pHist || hr)
        goto Cleanup;

    // are the unknowns equal?
    hr = (IsSameObject(pUnk, pHist)) ? S_OK : S_FALSE;

Cleanup:
    ReleaseInterface(pHist);
    RRETURN1(hr, S_FALSE);
}



//+-------------------------------------------------------------------------
//
//  COmNavigator - implementation for the window.navigator object
//
//--------------------------------------------------------------------------

COmNavigator::COmNavigator(COmWindow2 *pWindow)
{
    Assert(pWindow);
    _pWindow = pWindow;
    
    _pPluginsCollection = NULL;
    _pMimeTypesCollection = NULL;
    _pOpsProfile = NULL;
}

COmNavigator::~COmNavigator()
{
    super::Passivate();
    delete _pPluginsCollection;
    delete _pMimeTypesCollection;
    delete _pOpsProfile;
}

ULONG COmNavigator::PrivateAddRef(void)
{
    return _pWindow->SubAddRef();
}

ULONG COmNavigator::PrivateRelease(void)
{
    return _pWindow->SubRelease();
}

HRESULT
COmNavigator::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT hr=S_OK;

    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IObjectIdentity, NULL)

    default:
        if (iid == IID_IOmNavigator)
            hr = THR(CreateTearOffThunk(this, COmNavigator::s_apfnIOmNavigator, NULL, ppv));
    }
    
    if (!hr)
    {
        if (*ppv)
            (*(IUnknown **)ppv)->AddRef();
        else
            hr = E_NOINTERFACE;
    }
    RRETURN(hr);
}

const COmNavigator::CLASSDESC COmNavigator::s_classdesc =
{
    &CLSID_HTMLNavigator,                // _pclsid
    0,                                   // _idrBase
#ifndef NO_PROPERTY_PAGE
    0,                                   // _apClsidPages
#endif // NO_PROPERTY_PAGE
    0,                                   // _pcpi
    0,                                   // _dwFlags
    &IID_IOmNavigator,                   // _piidDispinterface
    &s_apHdlDescs,                       // _apHdlDesc
};

void DeinitUserAgentString(THREADSTATE *pts)
{
    pts->cstrUserAgent.Free();
}

HRESULT EnsureUserAgentString()
{
    HRESULT hr = S_OK;
    TCHAR   szUserAgent[MAX_PATH];  // URLMON says the max length of the UA string is MAX_PATH
    DWORD   dwSize = MAX_PATH;

    szUserAgent[0] = '\0';

    if (!TLS(cstrUserAgent))
    {
        hr = ObtainUserAgentStringW(0, szUserAgent, &dwSize);
        if (hr)
            goto Cleanup;

        hr = (TLS(cstrUserAgent)).Set(szUserAgent);
    }

Cleanup:
    RRETURN(hr);
}

HRESULT COmNavigator::get_appCodeName(BSTR *p)
{
    HRESULT hr;
    hr = THR(EnsureUserAgentString());
    if (hr)
        goto Cleanup;

    Assert(!!TLS(cstrUserAgent));
    hr = THR(FormsAllocStringLen(TLS(cstrUserAgent), 7, p));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT COmNavigator::get_appName(BSTR *p)
{
    HRESULT hr;
    // BUGBUG (sramani): Need to replace hard coded string with value from registry when available.
    hr = THR(FormsAllocString(_T("Microsoft Internet Explorer"), p));
    RRETURN(SetErrorInfo(hr));
}

HRESULT COmNavigator::get_appVersion(BSTR *p)
{
    HRESULT hr;
    hr = THR(EnsureUserAgentString());
    if (hr)
        goto Cleanup;

    Assert(!!TLS(cstrUserAgent));
    hr = THR(FormsAllocString(TLS(cstrUserAgent) + 8, p));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT COmNavigator::get_userAgent(BSTR *p)
{
    HRESULT hr;

    hr = THR(EnsureUserAgentString());
    if (hr)
        goto Cleanup;

    Assert(!!TLS(cstrUserAgent));
    (TLS(cstrUserAgent)).AllocBSTR(p);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT COmNavigator::get_cookieEnabled(VARIANT_BOOL *p)
{
    HRESULT hr = S_OK;
    BOOL    fAllowed;

    if (p)
    {
        hr = THR(_pWindow->_pDoc->ProcessURLAction(URLACTION_COOKIES, &fAllowed));
        if (!hr)
            *p = fAllowed ? VARIANT_TRUE : VARIANT_FALSE; 
    }
    else
        hr = E_POINTER;

    RRETURN(SetErrorInfo(hr));
}

HRESULT 
COmNavigator::javaEnabled(VARIANT_BOOL *enabled)
{
    HRESULT hr;
    BOOL    fAllowed;

    hr = THR(_pWindow->_pDoc->ProcessURLAction(URLACTION_JAVA_PERMISSIONS, &fAllowed));
    if (hr)
        goto Cleanup;    
    
    if (enabled)
    {
        *enabled = fAllowed ?  VARIANT_TRUE : VARIANT_FALSE;
    }

Cleanup:
    RRETURN(hr);
}

HRESULT 
COmNavigator::taintEnabled(VARIANT_BOOL *penabled)
{
    HRESULT hr = S_OK;

    if(penabled != NULL)
    {
        *penabled = VB_FALSE;
    }
    else
    {
        hr = E_POINTER;
    }

    RRETURN(hr);
}


HRESULT COmNavigator::toString(BSTR * pbstr)
{
    RRETURN(super::toString(pbstr));
}

//+-----------------------------------------------------------------
//
//  members : get_mimeTypes
//
//  synopsis : IHTMLELement implementaion to return the mimetypes collection
//
//-------------------------------------------------------------------

HRESULT
COmNavigator::get_mimeTypes(IHTMLMimeTypesCollection **ppMimeTypes)
{
    HRESULT     hr;

    if (ppMimeTypes == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppMimeTypes = NULL;

    if(_pMimeTypesCollection == NULL)
    {
        // create the collection
        _pMimeTypesCollection = new CMimeTypes();
        if (_pMimeTypesCollection == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    hr = THR(_pMimeTypesCollection->QueryInterface(IID_IHTMLMimeTypesCollection,
        (VOID **)ppMimeTypes)); 

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+-----------------------------------------------------------------
//
//  members : get_plugins
//
//  synopsis : IHTMLELement implementaion to return the filter collection
//
//-------------------------------------------------------------------

HRESULT
COmNavigator::get_plugins(IHTMLPluginsCollection **ppPlugins)
{
    HRESULT     hr;

    if (ppPlugins == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppPlugins = NULL;

    //Get existing Plugins Collection or create a new one
    if (_pPluginsCollection == NULL)
    {
        _pPluginsCollection = new CPlugins();
        if (_pPluginsCollection == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    hr = THR_NOTRACE(_pPluginsCollection->QueryInterface(IID_IHTMLPluginsCollection,
        (VOID **)ppPlugins));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
COmNavigator::get_userProfile(IHTMLOpsProfile **ppOpsProfile)
{
    return  get_opsProfile(ppOpsProfile);
}

//+-----------------------------------------------------------------
//
//  members : get_opsProfile
//
//  synopsis : IHTMLOpsProfile implementaion to return the profile object.
//
//-------------------------------------------------------------------

HRESULT
COmNavigator::get_opsProfile(IHTMLOpsProfile **ppOpsProfile)
{
    HRESULT     hr;

    if (ppOpsProfile == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppOpsProfile = NULL;

    //Get existing opsProfile object or create a new one

    if (_pOpsProfile == NULL)
    {
        _pOpsProfile = new COpsProfile();
        if (_pOpsProfile == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    hr = THR_NOTRACE(_pOpsProfile->QueryInterface(IID_IHTMLOpsProfile,
        (VOID **)ppOpsProfile));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT COmNavigator::get_cpuClass(BSTR *p)
{
    HRESULT hr = S_OK; // For Now
    if(!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    SYSTEM_INFO SysInfo;
    ::GetSystemInfo(&SysInfo);
    switch(SysInfo.wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_INTEL:
        *p = SysAllocString(_T("x86"));
        break;
    case PROCESSOR_ARCHITECTURE_ALPHA:
        *p = SysAllocString(_T("Alpha"));
        break;
    default:
        *p = SysAllocString(_T("Other"));
        break;
    }
    
    if(*p == NULL)
        hr = E_OUTOFMEMORY;

Cleanup:
    RRETURN(hr);
}


extern IMultiLanguage *g_pMultiLanguage;

HRESULT COmNavigator::get_systemLanguage(BSTR *p)
{
    HRESULT hr;
    LCID    lcid;
    
    if(!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *p = NULL;

    hr = THR(EnsureMultiLanguage());
    if (hr)
        goto Cleanup;

    lcid = ::GetSystemDefaultLCID();
    hr = THR(g_pMultiLanguage->GetRfc1766FromLcid(lcid, p));

Cleanup:
    RRETURN(hr);
}

HRESULT COmNavigator::get_browserLanguage(BSTR *p)
{
    *p = NULL;
    RRETURN(E_NOTIMPL);
}

HRESULT COmNavigator::get_userLanguage(BSTR *p)
{
    HRESULT hr;
    LCID    lcid;
    
    if(!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *p = NULL;

    hr = THR(EnsureMultiLanguage());
    if (hr)
        goto Cleanup;

    lcid = ::GetUserDefaultLCID();
    hr = THR(g_pMultiLanguage->GetRfc1766FromLcid(lcid, p));

Cleanup:
    RRETURN(hr);
}

HRESULT COmNavigator::get_platform(BSTR *p)
{
    HRESULT hr = S_OK; 

    // Nav compatability item, returns the following in Nav:-
    // Win32,Win16,Unix,Motorola,Max68k,MacPPC
    TCHAR *pszPlatform =
#ifdef WIN16
        _T("Win16");
#else
#ifdef WINCE
        _T("WinCE");    // Invented - obviously not a Nav compat issue!
#else
        _T("Win32");
#endif // WINCE
#endif // WIN16

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *p = NULL;

    hr = THR(FormsAllocString ( pszPlatform, p ));
Cleanup:
    RRETURN(hr);
}

HRESULT COmNavigator::get_appMinorVersion(BSTR *p)
{
    HKEY hkInetSettings;
    long lResult;
    HRESULT hr = S_FALSE;
    
    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *p = NULL;

    lResult = RegOpenKey(HKEY_LOCAL_MACHINE,
                        _T("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"),
                        &hkInetSettings );

    if( ERROR_SUCCESS == lResult )
    {
        DWORD dwType;
        DWORD size = pdlUrlLen;
        BYTE  buffer[pdlUrlLen];

        // If this is bigger than MAX_URL_STRING the registry is probably hosed.
        lResult = RegQueryValueEx( hkInetSettings, _T("MinorVersion"), 0, &dwType, buffer, &size );
        
        RegCloseKey(hkInetSettings);

        if( ERROR_SUCCESS == lResult && dwType == REG_SZ )
        {
            // Just figure out the real length since 'size' is ANSI bytes required.
            *p = SysAllocString( (LPCTSTR)buffer );
            hr = *p ? S_OK : E_OUTOFMEMORY;
        }
    }

    if ( hr )
    {
        *p = SysAllocString ( L"0" );
        hr = *p ? S_OK : E_OUTOFMEMORY;
    }

Cleanup:
    RRETURN(hr);
}

HRESULT COmNavigator::get_connectionSpeed(long *p)
{
    *p = NULL;
    RRETURN(E_NOTIMPL);
}

extern BOOL IsGlobalOffline();

HRESULT COmNavigator::get_onLine(VARIANT_BOOL *p)
{
    HRESULT hr = S_OK;
    
    if(!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *p = (IsGlobalOffline()) ? VB_FALSE : VB_TRUE;

Cleanup:        
     RRETURN(hr);
}


//+-----------------------------------------------------------------
//
//  CPlugins implementation.
//
//-------------------------------------------------------------------

const CBase::CLASSDESC CPlugins::s_classdesc =
{
    &CLSID_CPlugins,                    // _pclsid
    0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                               // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                               // _pcpi
    0,                                  // _dwFlags
    &IID_IHTMLPluginsCollection,        // _piidDispinterface
    &s_apHdlDescs                       // _apHdlDesc
};

HRESULT
CPlugins::get_length(LONG *pLen)
{
    HRESULT hr = S_OK;
    if(pLen != NULL)
        *pLen = 0;
    else
        hr =E_POINTER;

    RRETURN(hr);
}

HRESULT
CPlugins::refresh(VARIANT_BOOL fReload)
{
    return S_OK;
}



//+---------------------------------------------------------------
//
//  Member  : CPlugins::PrivateQueryInterface
//
//  Sysnopsis : Vanilla implementation for this class
//
//----------------------------------------------------------------

HRESULT
CPlugins::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IObjectIdentity, NULL)
        default:
        {
            if (iid == IID_IHTMLPluginsCollection)
            {
                HRESULT hr = THR(CreateTearOffThunk(this, s_apfnIHTMLPluginsCollection, NULL, ppv));
                if (hr)
                    RRETURN(hr);
            }
        }
    }

    if (*ppv)
    {
        (*(IUnknown**)ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


const CBase::CLASSDESC CMimeTypes::s_classdesc =
{
    &CLSID_CMimeTypes,                  // _pclsid
    0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                               // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                               // _pcpi
    0,                                  // _dwFlags
    &IID_IHTMLMimeTypesCollection,      // _piidDispinterface
    &s_apHdlDescs                       // _apHdlDesc
};



HRESULT
CMimeTypes::get_length(LONG *pLen)
{
    HRESULT hr = S_OK;
    if(pLen != NULL)
        *pLen = 0;
    else
        hr = E_POINTER;

    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member  : CMimeTypes::PrivateQueryInterface
//
//  Sysnopsis : Vanilla implementation for this class
//
//----------------------------------------------------------------

HRESULT
CMimeTypes::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        default:
        {
            if ( iid == IID_IHTMLMimeTypesCollection )
            {
                HRESULT hr = THR(CreateTearOffThunk(this, s_apfnIHTMLMimeTypesCollection, NULL, ppv));
                if (hr)
                    RRETURN(hr);
            }
        }
    }

    if (*ppv)
    {
        (*(IUnknown**)ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//+-----------------------------------------------------------------
//
//  COpsProfile implementation.
//
//-------------------------------------------------------------------

const CBase::CLASSDESC COpsProfile::s_classdesc =
{
    &CLSID_COpsProfile,                 // _pclsid
    0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                               // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                               // _pcpi
    0,                                  // _dwFlags
    &IID_IHTMLOpsProfile,               // _piidDispinterface
    &s_apHdlDescs                       // _apHdlDesc
};


HRESULT
COpsProfile::getAttribute(BSTR name, BSTR *value)
{
    HRESULT hr = S_OK;
    // Should never get called. 
    RRETURN(hr);
}

HRESULT
COpsProfile::setAttribute(BSTR name, BSTR value, VARIANT prefs, VARIANT_BOOL *p)
{
    HRESULT hr = S_OK;

    if (p != NULL)
    {
        *p = VB_FALSE;
    }
    else
    {
        hr = E_POINTER;
    }        
                
    return hr;
}

HRESULT 
COpsProfile::addReadRequest(BSTR name, VARIANT reserved, VARIANT_BOOL *p)
{
    HRESULT hr = S_OK;

    if (p != NULL)
    {
        *p = VB_FALSE;
    }
    else
    {
        hr = E_POINTER;
    }        
                
    return hr;
}


HRESULT 
COpsProfile::addRequest(BSTR name, VARIANT reserved, VARIANT_BOOL *p)
{                
    return addReadRequest(name,reserved,p);
}

HRESULT 
COpsProfile::clearRequest()
{
    return S_OK;
}

HRESULT
COpsProfile::doRequest(VARIANT usage, VARIANT fname, 
                       VARIANT domain, VARIANT path, VARIANT expire,
                       VARIANT reserved)
{
    return S_OK;
}

HRESULT
COpsProfile::doReadRequest(VARIANT usage, VARIANT fname, 
                           VARIANT domain, VARIANT path, VARIANT expire,
                           VARIANT reserved)
{
    return S_OK;
}

HRESULT 
COpsProfile::commitChanges(VARIANT_BOOL *p)
{
    HRESULT hr = S_OK;

    if (p != NULL)
    {
        *p = VB_FALSE;
    }
    else
    {
        hr = E_POINTER;
    }        
                
    return hr;
}

HRESULT 
COpsProfile::doWriteRequest(VARIANT_BOOL *p)
{
    return commitChanges(p);
}



//+---------------------------------------------------------------
//
//  Member  : COpsProfile::PrivateQueryInterface
//
//  Sysnopsis : Vanilla implementation for this class
//
//----------------------------------------------------------------

HRESULT
COpsProfile::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IObjectIdentity, NULL)
        default:
        {
            if (iid == IID_IHTMLOpsProfile)
            {
                HRESULT hr = THR(CreateTearOffThunk(this, s_apfnIHTMLOpsProfile, NULL, ppv));
                if (hr)
                    RRETURN(hr);
            }
        }
    }

    if (*ppv)
    {
        (*(IUnknown**)ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


