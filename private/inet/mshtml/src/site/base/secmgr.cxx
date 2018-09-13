//+------------------------------------------------------------------------
//
//  File:       secmgr.cxx
//
//  Contents:   Security manager call implementation
//
//  Classes:    (part of) CDoc, CSecurityMgrSite
//
//  History:    05-07-97  AnandRa   Created
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_SAFETY_HXX_
#define X_SAFETY_HXX_
#include "safety.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_ACTIVSCP_H_
#define X_ACTIVSCP_H_
#include <activscp.h>
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_FPRINT_HXX_
#define X_FPRINT_HXX_
#include "fprint.hxx"
#endif

ExternTag(tagCDoc);

const GUID GUID_CUSTOM_CONFIRMOBJECTSAFETY = 
	{ 0x10200490, 0xfa38, 0x11d0, { 0xac, 0xe, 0x0, 0xa0, 0xc9, 0xf, 0xff, 0xc0 }};

// Custom policy used to query before the scriptlet runtime creates an 
// interface handler. The semantics of this call are similar to the standard
// policy URLACTION_ACTIVEX_RUN. e.g. Input is the CLSID of the handler. Output
// is the policy in a DWORD.
static const GUID GUID_CUSTOM_CONFIRMINTERFACEHANDLER = 
	{ 0x02990d50, 0xcd96, 0x11d1, { 0xa3, 0x75, 0x0, 0x60, 0x8, 0xc3, 0xfb, 0xfc }};

// automation handler
static const GUID CLSID_DexHandlerConstructor =
    { 0xc195d550, 0xa068, 0x11d1, { 0x89, 0xb6, 0x0, 0x60, 0x8, 0xc3, 0xfb, 0xfc }};
// event handler
static const GUID CLSID_CpcHandlerConstructor =
    { 0x3af5262e, 0x4e6e, 0x11d1, { 0xbf, 0xa5, 0x00, 0xc0, 0x4f, 0xc3, 0x0c, 0x45}};

static const GUID* knownScriptletHandlers[] = {
	&CLSID_CPeerHandler,
	&CLSID_DexHandlerConstructor,
	&CLSID_CpcHandlerConstructor,
	&CLSID_CHiFiUses,
	&CLSID_CCSSFilterHandler,
    &CLSID_CSvrOMUses,
};

//+------------------------------------------------------------------------
//
//  Member:     CDoc::EnsureSecurityManager
//
//  Synopsis:   Verify that the security manager is created
//
//-------------------------------------------------------------------------

HRESULT
CDoc::EnsureSecurityManager()
{
    TraceTag((tagCDoc, "CDoc::EnsureSecurityManager"));

    HRESULT hr = S_OK;
    CDoc *  pDoc;

    if (_pSecurityMgr)
        goto Cleanup;

    //
    // If we're ensuring the security manager for some reason, we
    // will be using the url down the road, so ensure that too.
    //

    Assert(!!GetDocumentSecurityUrl());

    //
    // Get the root doc because all frames in a frameset need to
    // use the same security manager.  This is for answers that are
    // persisted on a per instance basis.
    //

    pDoc = GetRootDoc();
    if (pDoc != this)
    {
        hr = THR(pDoc->EnsureSecurityManager());
        if (hr)
            goto Cleanup;

        _pSecurityMgr = pDoc->_pSecurityMgr;
        Assert(_pSecurityMgr);
        _pSecurityMgr->AddRef();
    }
    else
    {
        hr = THR(CoInternetCreateSecurityManager(NULL, &_pSecurityMgr, 0));
        if (hr)
            goto Cleanup;

        hr = THR(_pSecurityMgr->SetSecuritySite(&_SecuritySite));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CDoc::AllowClipboardAccess(TCHAR *pchCmdId, BOOL *pfAllow)
{
    if (    StrCmpICW(pchCmdId, _T("paste")) == 0
        ||  StrCmpICW(pchCmdId, _T("copy")) == 0
        ||  StrCmpICW(pchCmdId, _T("cut")) == 0)
    {
        RRETURN(ProcessURLAction(URLACTION_SCRIPT_PASTE, pfAllow));
    }

    *pfAllow = TRUE;

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::ProcessURLAction
//
//  Synopsis:   Query the security manager for a given action
//              and return the response.  This is basically a true/false
//              or allow/disallow return value.
//
//  Arguments:  dwAction [in]  URLACTION_* from urlmon.h
//              pfReturn [out] TRUE on allow, FALSE on any other result
//              fNoQuery [in]  TRUE means don't query
//              pfQuery  [out] TRUE if query was required
//              pchURL   [in]  The acting URL or NULL for the doc itself
//              pbArg    [in]  pbContext for IInternetSecurityManager::PUA
//              cbArg    [in]  cbContext for IInternetSecurityManager::PUA
//
//-------------------------------------------------------------------------

HRESULT
CDoc::ProcessURLAction(
    DWORD dwAction,
    BOOL *pfReturn,
    DWORD dwPuaf,
    DWORD *pdwPolicy,
    LPCTSTR pchURL,
    BYTE *pbArg,
    DWORD cbArg,
    BOOL fDisableNoShowElement)
{
    HRESULT hr = S_OK;
    DWORD   dwPolicy = 0;
    DWORD   dwMask = 0;
    ULONG   cDie = _cDie;

    *pfReturn = FALSE;

    hr = THR(EnsureSecurityManager());
    if (hr)
        goto Error;

    //
    // Check the action for special known guids.  In these cases
    // we want the min of the security settings and the _dwLoadf
    // that the host provided.
    //

    switch (dwAction)
    {
    case URLACTION_SCRIPT_RUN:
        dwMask = DLCTL_NO_SCRIPTS;
        break;

    case URLACTION_ACTIVEX_RUN:
        dwMask = DLCTL_NO_RUNACTIVEXCTLS;
        break;

    case URLACTION_HTML_JAVA_RUN:
        dwMask = DLCTL_NO_JAVA;
        break;

    case URLACTION_JAVA_PERMISSIONS:
        dwMask = DLCTL_NO_JAVA;
        break;
    }

    if (dwMask && (_dwLoadf & dwMask))
    {
        // 69976: If we are not running scripts only because we are printing,
        // don't disable noshow elements NOSCRIPT and NOEMBED.
        if (   !fDisableNoShowElement
            || dwMask != DLCTL_NO_SCRIPTS
            || !DontRunScripts())
        {
            goto Error;
        }
    }

    if (!pchURL)
    {
        pchURL = GetDocumentSecurityUrl();
    }

    if (_fTrustedDoc)
    {
        dwPuaf |= PUAF_TRUSTED;
    }
    
    hr = THR(_pSecurityMgr->ProcessUrlAction(
        pchURL,
        dwAction,
        (BYTE *)&dwPolicy,
        sizeof(DWORD),
        pbArg,
        cbArg,
        dwPuaf,
        0));

    if (hr == S_FALSE)
        hr = S_OK;

    if (_cDie != cDie)
        hr = E_ABORT;

    if (hr)
        goto Error;

    if (pdwPolicy)
        *pdwPolicy = dwPolicy;

    if (dwAction != URLACTION_JAVA_PERMISSIONS)
    {
        *pfReturn = (GetUrlPolicyPermissions(dwPolicy) == URLPOLICY_ALLOW);
    }
    else
    {
        // query was for URL_ACTION_JAVA_PERMISSIONS
        *pfReturn = (dwPolicy != URLPOLICY_JAVA_PROHIBIT);
    }

Cleanup:
    TraceTag((tagCDoc, "CDoc::ProcessURLAction, Action=0x%x URL=%s Allowed=%d", dwAction, pchURL, *pfReturn));

    RRETURN(hr);

Error:
    *pfReturn = FALSE;

    goto Cleanup;
}


//+------------------------------------------------------------------------
//
//  Member:     CDoc::GetSecurityID
//
//  Synopsis:   Retrieve a security ID from a url from the system sec mgr.
//
//-------------------------------------------------------------------------

HRESULT
CDoc::GetSecurityID(BYTE *pbSID, DWORD *pcb, LPCTSTR pchURL)
{
    HRESULT hr;
    DWORD   dwSize;
    TCHAR   achURL[pdlUrlLen];

    hr = THR(EnsureSecurityManager());
    if (hr)
        goto Cleanup;

    if (!pchURL)
    {
        pchURL = GetDocumentSecurityUrl();
    }

    // Unescape the URL.
    hr = THR(CoInternetParseUrl(
            pchURL,
            PARSE_ENCODE,
            0,
            achURL,
            ARRAY_SIZE(achURL),
            &dwSize,
            0));
    if (hr)
        goto Cleanup;

    hr = THR(_pSecurityMgr->GetSecurityId(
            achURL,
            pbSID,
            pcb,
            (DWORD_PTR)(TCHAR *)_cstrSetDomain));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

void
CDoc::UpdateSecurityID()
{
    HRESULT hr;

    if (_pOmWindow)
    {
        BYTE    abSID[MAX_SIZE_SECURITY_ID];
        DWORD   cbSID = ARRAY_SIZE(abSID);

        hr = THR(GetSecurityID(abSID, &cbSID));
        if (hr)
            return;

        hr = THR(_pOmWindow->Init(_pOmWindow->_pWindow, abSID, cbSID));
        if (hr)
            return;

        if ((TLS(windowInfo.paryWindowTbl)->FindProxy(_pOmWindow->_pWindow, abSID, cbSID, _fTrustedDoc, NULL)))
        {
            TLS(windowInfo.paryWindowTbl)->DeleteProxyEntry(_pOmWindow);

            THR(TLS(windowInfo.paryWindowTbl)->AddTuple(
                    _pOmWindow->_pWindow,
                    abSID,
                    cbSID,
                    _fTrustedDoc,
                    _pOmWindow));
        }
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::AccessAllowed
//
//  Synopsis:   checks if access should be allowed from/to pchUrl to/from 
//              current doc's url.
//
//-------------------------------------------------------------------------

BOOL
CDoc::AccessAllowed(LPCTSTR pchUrl)
{
    HRESULT hr = S_OK;
    BYTE    abSID1[MAX_SIZE_SECURITY_ID];
    BYTE    abSID2[MAX_SIZE_SECURITY_ID];
    DWORD   cbSID1 = ARRAY_SIZE(abSID1);
    DWORD   cbSID2 = ARRAY_SIZE(abSID2);
    BOOL    fAccessAllowed = FALSE;

    hr = THR_NOTRACE(GetSecurityID(abSID1, &cbSID1, pchUrl));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(GetSecurityID(abSID2, &cbSID2)); // get SID of doc itself
    if (hr)
        goto Cleanup;

    fAccessAllowed = (cbSID1 == cbSID2 && 0 == memcmp(abSID1, abSID2, cbSID1));

Cleanup:
    return fAccessAllowed;
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::GetFrameZone
//
//  Synopsis:   Pass out the zone of the current doc depending on variant
//              passed in.
//
//  Notes:      If the pvar is VT_EMPTY, nothing has been filled out yet.
//              If it is VT_UI4, it contains the current zone.  We have to
//              check zones with this and update it to mixed if appropriate.
//              If it is VT_NULL, the zone is mixed.
//
//-------------------------------------------------------------------------

HRESULT
CDoc::GetFrameZone(VARIANT *pvar)
{
    HRESULT hr = S_OK;
    DWORD   dwZone;

    if (V_VT(pvar) == VT_NULL)
        goto Cleanup;

    hr = THR(EnsureSecurityManager());
    if (hr)
        goto Cleanup;

    hr = THR(_pSecurityMgr->MapUrlToZone(GetDocumentSecurityUrl(), &dwZone, 0));
    if (hr)
        goto Cleanup;

    if (V_VT(pvar) == VT_EMPTY)
    {
        V_VT(pvar) = VT_UI4;
        V_UI4(pvar) = dwZone;
    }
    else if (V_VT(pvar) == VT_UI4)
    {
        if (V_UI4(pvar) != dwZone)
        {
            V_VT(pvar) = VT_NULL;
        }
    }
    else
    {
        Assert(0 && "Unexpected value in variant");
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CDoc::HostGetSecurityId
//
//  Synopsis:   Retrieve a security ID from a url from the system sec mgr.
//
//-------------------------------------------------------------------------

HRESULT
CDoc::HostGetSecurityId(BYTE *pbSID, DWORD *pcb, LPCWSTR pwszDomain)
{
    HRESULT hr;

    hr = THR(EnsureSecurityManager());
    if (hr)
        goto Cleanup;

    hr = THR(_pSecurityMgr->GetSecurityId(
            GetDocumentSecurityUrl(),
            pbSID,
            pcb,
            (DWORD_PTR)pwszDomain));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CDoc::ProcessUrlAction
//
//  Synopsis:   per IInternetHostSecurityManager
//
//-------------------------------------------------------------------------

HRESULT
CDoc::HostProcessUrlAction(
    DWORD dwAction,
    BYTE *pPolicy,
    DWORD cbPolicy,
    BYTE *pContext,
    DWORD cbContext,
    DWORD dwFlags,
    DWORD dwReserved)
{
    HRESULT hr;

    hr = THR(EnsureSecurityManager());
    if (hr)
        goto Cleanup;

    if (_fTrustedDoc)
    {
        dwFlags |= PUAF_TRUSTED;
    }
    
    hr = THR(_pSecurityMgr->ProcessUrlAction(
            GetDocumentSecurityUrl(),
            dwAction,
            pPolicy,
            cbPolicy,
            pContext,
            cbContext,
            dwFlags,
            dwReserved));
    if (!OK(hr))
        goto Cleanup;

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+------------------------------------------------------------------------
//
//  Member:     CDoc::QueryCustomPolicy
//
//  Synopsis:   per IInternetHostSecurityManager
//
//-------------------------------------------------------------------------

HRESULT
CDoc::HostQueryCustomPolicy(
    REFGUID guidKey,
    BYTE **ppPolicy,
    DWORD *pcbPolicy,
    BYTE *pContext,
    DWORD cbContext,
    DWORD dwReserved)
{
    HRESULT         hr;
    IActiveScript * pScript = NULL;
    BYTE            bPolicy = (BYTE)URLPOLICY_DISALLOW;

    hr = THR(EnsureSecurityManager());
    if (hr)
        goto Cleanup;

    //
    // Forward all other custom policies to the real security
    // manager.
    //
    
    hr = THR_NOTRACE(_pSecurityMgr->QueryCustomPolicy(
            GetDocumentSecurityUrl(),
            guidKey,
            ppPolicy,
            pcbPolicy,
            pContext,
            cbContext,
            dwReserved));
    if (hr != INET_E_DEFAULT_ACTION &&
        hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
        goto Cleanup;
        
    if (guidKey == GUID_CUSTOM_CONFIRMOBJECTSAFETY)
    {
        CONFIRMSAFETY * pconfirm;
        DWORD           dwAction;
        BOOL            fSafe;
        SAFETYOPERATION safety;
        const IID *     piid;
        
        //
        // This is a special guid meaning that some embedded object
        // within is itself trying to create another object.  This might
        // just be some activex obj or a script engine.  We will need
        // to run through our IObjectSafety code on this object.  We
        // get the clsid and the IUnknown of the object passed in from
        // the context.
        //
        
        if (cbContext != sizeof(CONFIRMSAFETY))
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        if (!ppPolicy || !pcbPolicy)
        {
            hr = E_POINTER;
            goto Cleanup;
        }
        
        pconfirm = (CONFIRMSAFETY *)pContext;
        if (!pconfirm->pUnk)
            goto Cleanup;
            
        //
        // Check for script engine.  If so, then we need to use a different
        // action and IObjectSafety has slightly different restrictions.
        //

        if (OK(THR_NOTRACE(pconfirm->pUnk->QueryInterface(
                IID_IActiveScript,
                (void **)&pScript))) && 
            pScript)
        {
            dwAction = URLACTION_SCRIPT_OVERRIDE_SAFETY;
            safety = SAFETY_SCRIPTENGINE;
            piid = &IID_IActiveScript;
        }
        else
        {
            dwAction = URLACTION_ACTIVEX_OVERRIDE_SCRIPT_SAFETY;
            safety = SAFETY_SCRIPT;
            piid = &IID_IDispatch;
        }
        
        hr = THR(ProcessURLAction(dwAction, &fSafe));
        if (hr)
            goto Cleanup;

        if (!fSafe)
        {
            fSafe = IsSafeTo(
                        safety, 
                        *piid, 
                        pconfirm->clsid, 
                        pconfirm->pUnk, 
                        this);
        }

        if (fSafe)
            bPolicy = (BYTE)URLPOLICY_ALLOW;

        hr = S_OK;
        goto ReturnPolicy;
    }
    else if (guidKey == GUID_CUSTOM_CONFIRMINTERFACEHANDLER)
    {
        if (cbContext == sizeof(GUID))
        {
            CLSID  *pHandlerCLSID = (CLSID *)pContext;

            for (int i = 0; i < ARRAY_SIZE(knownScriptletHandlers); i++)
                if (*pHandlerCLSID == *knownScriptletHandlers[i])
                {
                    bPolicy = (BYTE)URLPOLICY_ALLOW;
                    break;
                }
        }

        hr = S_OK;
        goto ReturnPolicy;
    }
    
Cleanup:
    ReleaseInterface(pScript);
    RRETURN(hr);

ReturnPolicy:
    *pcbPolicy = sizeof(DWORD);
    *ppPolicy = (BYTE *)CoTaskMemAlloc(sizeof(DWORD));
    if (!*ppPolicy)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    *(DWORD *)*ppPolicy = (BYTE)bPolicy;

    goto Cleanup;
}



IMPLEMENT_SUBOBJECT_IUNKNOWN(CSecurityMgrSite, CDoc, Doc, _SecuritySite)


//+------------------------------------------------------------------------
//
//  Member:     CSecurityMgrSite::QueryInterface
//
//  Synopsis:   per IUnknown
//
//-------------------------------------------------------------------------

HRESULT
CSecurityMgrSite::QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (riid == IID_IUnknown || riid == IID_IInternetSecurityMgrSite)
    {
        *ppv = (IInternetSecurityMgrSite *)this;
    }
    else if (riid == IID_IServiceProvider)
    {
        *ppv = (IServiceProvider *)this;
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    RRETURN(E_NOINTERFACE);
}


//+------------------------------------------------------------------------
//
//  Member:     CSecurityMgrSite::GetWindow
//
//  Synopsis:   Return parent window for use in ui
//
//-------------------------------------------------------------------------

HRESULT
CSecurityMgrSite::GetWindow(HWND *phwnd)
{
    HRESULT         hr = S_OK;
    IOleWindow *    pOleWindow = NULL;
    
    if (Doc()->_dwLoadf & DLCTL_SILENT)
    {
        *phwnd = (HWND)INVALID_HANDLE_VALUE;
        hr = S_FALSE;
    }
    else
    {
        hr = THR(Doc()->GetWindow(phwnd));
        if (hr)
        {
            if (Doc()->_pClientSite &&
                S_OK == Doc()->_pClientSite->QueryInterface(IID_IOleWindow, (void **)&pOleWindow))
            {
                hr = THR(pOleWindow->GetWindow(phwnd));
            }
        }
    }

    ReleaseInterface(pOleWindow);  
    RRETURN1(hr, S_FALSE);
}


//+------------------------------------------------------------------------
//
//  Member:     CSecurityMgrSite::EnableModeless
//
//  Synopsis:   Called before & after displaying any ui
//
//-------------------------------------------------------------------------

HRESULT
CSecurityMgrSite::EnableModeless(BOOL fEnable)
{
    CDoEnableModeless   dem(Doc(), FALSE);

    if (fEnable)
    {
        dem.EnableModeless(TRUE);
    }
    else
    {
        dem.DisableModeless();

        // Return an explicit failure here if we couldn't do it.
        // This is needed to ensure that the count does not go
        // out of sync.
        if (!dem._fCallEnableModeless)
            return E_FAIL;
    }
    
    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     CSecurityMgrSite::QueryService
//
//  Synopsis:   per IServiceProvider
//
//-------------------------------------------------------------------------

HRESULT
CSecurityMgrSite::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    RRETURN(Doc()->QueryService(guidService, riid, ppv));
}
