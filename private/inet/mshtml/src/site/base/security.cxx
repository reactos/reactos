//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       security.cxx
//
//  Contents:   Implementation of the security proxy for COmWindow2
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include "dispex.h"
#endif

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_IMGELEM_HXX_
#define X_IMGELEM_HXX_
#include "imgelem.hxx"
#endif

#ifndef X_EOPTION_HXX_
#define X_EOPTION_HXX_
#include "eoption.hxx"
#endif

#ifndef X_JSPROT_HXX_
#define X_JSPROT_HXX_
#include "jsprot.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include <uwininet.h>
#endif

#ifndef X_HISTORY_H_
#define X_HISTORY_H_
#include "history.h"
#endif

#ifndef X_WINABLE_H_
#define X_WINABLE_H_
#include "winable.h"
#endif

#ifndef X_EVENTOBJ_HXX_
#define X_EVENTOBJ_HXX_
#include "eventobj.hxx"
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_OLEACC_H
#define X_OLEACC_H
#include <oleacc.h>
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include <shell.h>
#endif

#ifndef X_SHLOBJP_H_
#define X_SHLOBJP_H_
#include <shlobjp.h>
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_HTIFRAME_H_
#define X_HTIFRAME_H_
#include <htiframe.h>
#endif

#define WINDOWDEAD() (RPC_E_SERVER_DIED_DNE == hr || RPC_E_DISCONNECTED == hr)

DeclareTag(tagSecurity, "Security", "Security methods")
MtDefine(COmWindowProxy, ObjectModel, "COmWindowProxy")
MtDefine(COmWindowProxy_pbSID, COmWindowProxy, "COmWindowProxy::_pbSID")
MtDefine(CAryWindowTbl, ObjectModel, "CAryWindowTbl")
MtDefine(CAryWindowTbl_pv, CAryWindowTbl, "CAryWindowTbl::_pv")
MtDefine(CAryWindowTblAddTuple_pbSID, CAryWindowTbl, "CAryWindowTbl::_pv::_pbSID")

BEGIN_TEAROFF_TABLE(COmWindowProxy, IMarshal)
    TEAROFF_METHOD(COmWindowProxy, GetUnmarshalClass, getunmarshalclass, (REFIID riid,void *pvInterface,DWORD dwDestContext,void *pvDestContext,DWORD mshlflags,CLSID *pCid))
    TEAROFF_METHOD(COmWindowProxy, GetMarshalSizeMax, getmarshalsizemax, (REFIID riid,void *pvInterface,DWORD dwDestContext,void *pvDestContext,DWORD mshlflags,DWORD *pSize))
    TEAROFF_METHOD(COmWindowProxy, MarshalInterface, marshalinterface, (IStream *pistm,REFIID riid,void *pvInterface,DWORD dwDestContext,void *pvDestContext,DWORD mshlflags))
    TEAROFF_METHOD(COmWindowProxy, UnmarshalInterface, unmarshalinterface, (IStream *pistm,REFIID riid,void ** ppvObj))
    TEAROFF_METHOD(COmWindowProxy, ReleaseMarshalData, releasemarshaldata, (IStream *pStm))
    TEAROFF_METHOD(COmWindowProxy, DisconnectObject, disconnectobject, (DWORD dwReserved))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_(COmLocationProxy, IDispatchEx)
    TEAROFF_METHOD(COmLocationProxy, GetTypeInfoCount, gettypeinfocount, (UINT *pcTinfo))
    TEAROFF_METHOD(COmLocationProxy, GetTypeInfo, gettypeinfo, (UINT itinfo, ULONG lcid, ITypeInfo ** ppTI))
    TEAROFF_METHOD(COmLocationProxy, GetIDsOfNames, getidsofnames, (REFIID riid,
                                   LPOLESTR *prgpsz,
                                   UINT cpsz,
                                   LCID lcid,
                                   DISPID *prgid))
    TEAROFF_METHOD(COmLocationProxy, Invoke, invoke, (DISPID dispidMember,
                            REFIID riid,
                            LCID lcid,
                            WORD wFlags,
                            DISPPARAMS * pdispparams,
                            VARIANT * pvarResult,
                            EXCEPINFO * pexcepinfo,
                            UINT * puArgErr))
    TEAROFF_METHOD(COmLocationProxy, GetDispID, getdispid, (BSTR bstrName,
                               DWORD grfdex,
                               DISPID *pid))
    TEAROFF_METHOD(COmLocationProxy, InvokeEx, invokeex, (DISPID id,
                        LCID lcid,
                        WORD wFlags,
                        DISPPARAMS *pdp,
                        VARIANT *pvarRes,
                        EXCEPINFO *pei,
                        IServiceProvider *pSrvProvider)) 
    TEAROFF_METHOD(COmLocationProxy, DeleteMemberByName, deletememberbyname, (BSTR bstr,DWORD grfdex))
    TEAROFF_METHOD(COmLocationProxy, DeleteMemberByDispID, deletememberbydispid, (DISPID id))    
    TEAROFF_METHOD(COmLocationProxy, GetMemberProperties, getmemberproperties, (DISPID id,
                                         DWORD grfdexFetch,
                                         DWORD *pgrfdex))
    TEAROFF_METHOD(COmLocationProxy, GetMemberName, getmembername, (DISPID id,
                                   BSTR *pbstrName))
    TEAROFF_METHOD(COmLocationProxy, GetNextDispID, getnextdispid, (DWORD grfdex,
                                   DISPID id,
                                   DISPID *pid))
    TEAROFF_METHOD(COmLocationProxy, GetNameSpaceParent, getnamespaceparent, (IUnknown **ppunk))
END_TEAROFF_TABLE()


BEGIN_TEAROFF_TABLE_(COmWindowProxy, IServiceProvider)
    TEAROFF_METHOD(COmWindowProxy, QueryService, queryservice, (REFGUID guidService, REFIID riid, void **ppvObject))
END_TEAROFF_TABLE()

BYTE byOnErrorParamTypes[4] = {VT_BSTR, VT_BSTR, VT_I4, 0};

HRESULT GetCallerURL(CStr &cstr, CBase *pBase, IServiceProvider *pSP);
HRESULT GetCallerCommandTarget (CBase *pBase, IServiceProvider *pSP, BOOL fFirstScriptSite, IOleCommandTarget **ppCommandTarget);
HRESULT WrapSpecialUrl(TCHAR *pchURL, CStr *pcstr, CStr &cstrBaseURL, BOOL fNotPrivate, BOOL fIgnoreUrlScheme);
HRESULT GetCallerSecurityState(SSL_SECURITY_STATE *pSecState, CBase *pBase, IServiceProvider * pSP);

extern void  ProcessValueEntities(TCHAR *pch, ULONG *pcch);

#ifndef WIN16
STDAPI HlinkFindFrame(LPCWSTR pszFrameName, LPUNKNOWN *ppunk);
#endif

//+-------------------------------------------------------------------------
//
//  Member:     Free
//
//  Synopsis:   Clear out the contents of a WINDOWTBL structure
//
//--------------------------------------------------------------------------

void
WINDOWTBL::Free()
{
    ClearInterface(&pWindow);
    delete [] pbSID;
    pbSID = NULL;
    cbSID = 0;
}


//+-------------------------------------------------------------------------
//
//  Function:   EnsureWindowInfo
//
//  Synopsis:   Ensures that a thread local window table exists
//
//--------------------------------------------------------------------------

HRESULT 
EnsureWindowInfo()
{
    if (!TLS(windowInfo.paryWindowTbl))
    {
        TLS(windowInfo.paryWindowTbl) = new CAryWindowTbl;
        if (!TLS(windowInfo.paryWindowTbl))
            RRETURN(E_OUTOFMEMORY);
    }

    if (!TLS(windowInfo.pSecMgr))
    {
        IInternetSecurityManager *  pSecMgr = NULL;
        HRESULT                     hr;

        hr = THR(CoInternetCreateSecurityManager(NULL, &pSecMgr, 0));
        if (hr)
            RRETURN(hr);

        TLS(windowInfo.pSecMgr) = pSecMgr;  // Implicit addref/release
    }
    
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:   DeinitWindowInfo
//
//  Synopsis:   Clear out the thread local window table
//
//--------------------------------------------------------------------------

void 
DeinitWindowInfo(THREADSTATE *pts)
{
    WINDOWTBL * pwindowtbl;
    long        i;
    
    if (pts->windowInfo.paryWindowTbl)
    {
        for (i = pts->windowInfo.paryWindowTbl->Size(), pwindowtbl=*(pts->windowInfo.paryWindowTbl);
             i > 0;
             i--, pwindowtbl++)
        {
            pwindowtbl->Free();
        }
        delete pts->windowInfo.paryWindowTbl;
        pts->windowInfo.paryWindowTbl = NULL;
    }

    ClearInterface(&(pts->windowInfo.pSecMgr));
}


//+-------------------------------------------------------------------------
//
//  Member:     CAryWindowTbl::FindProxy
//
//  Synopsis:   Search in window table and return proxy given
//              a window and a string.
//
//--------------------------------------------------------------------------

HRESULT
CAryWindowTbl::FindProxy(
    IHTMLWindow2 *pWindow, 
    BYTE *pbSID, 
    DWORD cbSID, 
    BOOL fTrust,
    IHTMLWindow2 **ppProxy)
{
    WINDOWTBL * pwindowtbl;
    long        i;

    Assert(pWindow);
    
    for (i = Size(), pwindowtbl = *this;
         i > 0;
         i--, pwindowtbl++)
    {
        // Should we QI for IUnknown here?
        if (IsSameObject(pWindow, pwindowtbl->pWindow))
        {
            if ((!pbSID && pwindowtbl->pbSID) ||
                (pbSID && !pwindowtbl->pbSID))
                continue;

            // Trust status must match for this comparison to succeed.
            if (fTrust != pwindowtbl->fTrust)
                continue;
                
            if ((!pbSID && !pwindowtbl->pbSID) ||
                (cbSID == pwindowtbl->cbSID &&
                 !memcmp(pbSID, pwindowtbl->pbSID, cbSID)))
            {
                //
                // Comparision succeeded, return this proxy.
                // This is a weak ref.
                //

                if (ppProxy)
                {
                    *ppProxy = pwindowtbl->pProxy;
                }
                return S_OK;
            }
        }
    }
    
    RRETURN(E_FAIL);
}


//+-------------------------------------------------------------------------
//
//  Member:     CAryWindowTbl::AddTuple
//
//  Synopsis:   Search in window table and return proxy given
//              a window and a string.
//
//--------------------------------------------------------------------------


HRESULT
CAryWindowTbl::AddTuple(
    IHTMLWindow2 *pWindow, 
    BYTE *pbSID,
    DWORD cbSID,
    BOOL fTrust,
    IHTMLWindow2 *pProxy)
{
    WINDOWTBL   windowtbl;
    WINDOWTBL * pwindowtbl;
    HRESULT     hr = S_OK;
    BYTE *      pbSIDTmp = NULL;
    
#if DBG == 1
    //
    // We better not be adding a proxy for an already existing tuple.
    //

    if (!(FindProxy(pWindow, pbSID, cbSID, fTrust, NULL)))
    {
        Assert(0 && "Adding security proxy twice");
    }
#endif

    windowtbl.pWindow = pWindow;
    pWindow->AddRef();

    //
    // Weak ref.  The proxy deletes itself from this array upon 
    // its destruction.
    //
    
    windowtbl.pProxy = pProxy;
    windowtbl.cbSID = cbSID;
    windowtbl.fTrust = fTrust;
    
    pbSIDTmp = new(Mt(CAryWindowTblAddTuple_pbSID)) BYTE[cbSID];
    if (!pbSIDTmp)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    memcpy(pbSIDTmp, pbSID, cbSID);
    windowtbl.pbSID = pbSIDTmp;
    
    hr = THR(TLS(windowInfo.paryWindowTbl)->AppendIndirect(&windowtbl, &pwindowtbl));
    if (hr)
        goto Error;

    MemSetName(( *(GetThreadState()->windowInfo.paryWindowTbl), "paryWindowTbl c=%d", (TLS(windowInfo.paryWindowTbl))->Size()));

Cleanup:
    RRETURN(hr);

Error:
    delete [] pbSIDTmp;
    pWindow->Release();
    goto Cleanup;
}


//+-------------------------------------------------------------------------
//
//  Member:     CAryWindowTbl::DeleteProxyEntry
//
//  Synopsis:   Search in window table for the given proxy
//              and delete its entry.  Every proxy should appear
//              only once in the window table.
//
//--------------------------------------------------------------------------

void 
CAryWindowTbl::DeleteProxyEntry(IHTMLWindow2 *pProxy)
{
    WINDOWTBL * pwindowtbl;
    long        i;
    
    Assert(pProxy);
    
    for (i = Size(), pwindowtbl = *this;
         i > 0;
         i--, pwindowtbl++)
    {
        if (pProxy == pwindowtbl->pProxy)
        {
            pwindowtbl->Free();
            Delete(Size() - i);
            break;
        }
    }

#if DBG == 1
    //
    // In debug mode, just ensure that this proxy is not appearing
    // anywhere else in the table.
    //

    for (i = Size(), pwindowtbl = *this;
         i > 0;
         i--, pwindowtbl++)
    {
        // Should we QI for IUnknown here?
        Assert(pProxy != pwindowtbl->pProxy);
    }
#endif
}


//+-------------------------------------------------------------------------
//
//  Function:   GetSIDOfDispatch
//
//  Synopsis:   Retrieve the host name given an IHTMLDispatch *
//
//--------------------------------------------------------------------------

HRESULT 
GetSIDOfDispatch(IDispatch *pDisp, BYTE *pbSID, DWORD *pcbSID)
{
    HRESULT         hr;
    CVariant        VarUrl;
    CVariant        VarDomain;
    TCHAR           ach[pdlUrlLen];
    DWORD           dwSize;

    // call invoke DISPID_SECURITYCTX off pDisp to get SID
    hr = THR_NOTRACE(GetDispProp(
            pDisp,
            DISPID_SECURITYCTX,
            LOCALE_SYSTEM_DEFAULT,
            &VarUrl,
            NULL,
            FALSE));
    if (hr) 
        goto Cleanup;

    if (V_VT(&VarUrl) != VT_BSTR || !V_BSTR(&VarUrl))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (!OK(THR_NOTRACE(GetDispProp(
            pDisp,
            DISPID_SECURITYDOMAIN,
            LOCALE_SYSTEM_DEFAULT,
            &VarDomain,
            NULL,
            FALSE))))
    {
        VariantClear(&VarDomain);
    }

    hr = THR(EnsureWindowInfo());
    if (hr)
        goto Cleanup;

    hr = THR(CoInternetParseUrl(
            V_BSTR(&VarUrl), 
            PARSE_ENCODE, 
            0, 
            ach, 
            ARRAY_SIZE(ach), 
            &dwSize, 
            0));
    if (hr)
        goto Cleanup;
        
    hr = THR(TLS(windowInfo.pSecMgr)->GetSecurityId(
            ach, 
            pbSID, 
            pcbSID,
            (DWORD_PTR)(V_VT(&VarDomain) == VT_BSTR ? V_BSTR(&VarDomain) : NULL)));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:   CreateSecurityProxy
//
//  Synopsis:   Creates a new security proxy for marshalling across
//              thread & process boundaries.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CreateSecurityProxy(
        IUnknown * pUnkOuter,
        IUnknown **ppUnk)
{
    COmWindowProxy *    pProxy;
    
    if (pUnkOuter)
        RRETURN(CLASS_E_NOAGGREGATION);

    pProxy = new COmWindowProxy();
    if (!pProxy)
        RRETURN(E_OUTOFMEMORY);

    Verify(!pProxy->QueryInterface(IID_IUnknown, (void **)ppUnk));
    pProxy->Release();
    
    return S_OK;
}


const CONNECTION_POINT_INFO COmWindowProxy::s_acpi[] =
{
    CPI_ENTRY(IID_IDispatch, DISPID_A_EVENTSINK)
    CPI_ENTRY(DIID_HTMLWindowEvents, DISPID_A_EVENTSINK)
    CPI_ENTRY(IID_ITridentEventSink, DISPID_A_EVENTSINK)
    CPI_ENTRY(DIID_HTMLWindowEvents2, DISPID_A_EVENTSINK)
    CPI_ENTRY_NULL
};


const CBase::CLASSDESC COmWindowProxy::s_classdesc =
{
    &CLSID_HTMLWindowProxy,         // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    s_acpi,                         // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLWindow2,              // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};




//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::COmWindowProxy
//
//  Synopsis:   ctor
//
//--------------------------------------------------------------------------

COmWindowProxy::COmWindowProxy() : super()
{
#ifdef WIN16
    m_baseOffset = ((BYTE *) (void *) (CBase *)this) - ((BYTE *) this);
#endif
    _pWindow = NULL;
    IncrementObjectCount(&_dwObjCnt);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::Passivate
//
//  Synopsis:   1st phase destructor
//
//--------------------------------------------------------------------------

void
COmWindowProxy::Passivate()
{
    //
    // Go through the cache and delete the entry for this proxy
    //
    
    if (TLS(windowInfo.paryWindowTbl))
    {
        TLS(windowInfo.paryWindowTbl)->DeleteProxyEntry((IHTMLWindow2 *)this);
    }

    GWKillMethodCall (this, NULL, 0);
    
    ClearInterface(&_pWindow);
    delete [] _pbSID;
    _pbSID = NULL;
    _cbSID = 0;
    super::Passivate();
    DecrementObjectCount(&_dwObjCnt);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::Init
//
//  Synopsis:   Initializer
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::Init(IHTMLWindow2 *pWindow, BYTE *pbSID, DWORD cbSID)
{
    HRESULT hr = S_OK;

    Assert(pbSID && cbSID);

    delete [] _pbSID;
    
    _pbSID = new(Mt(COmWindowProxy_pbSID)) BYTE[cbSID];
    if (!_pbSID)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    _cbSID = cbSID;
    memcpy(_pbSID, pbSID, cbSID);
    
    if (_pWindow != pWindow)
        ReplaceInterface(&_pWindow, pWindow);

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::SecureObject
//
//  Synopsis:   Wrap the correct proxy around this object if
//              necessary.
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::SecureObject(VARIANT *pvarIn, VARIANT *pvarOut, IServiceProvider *pSrvProvider, BOOL fSecurityCheck)
{
    IHTMLWindow2 *      pWindow = NULL;
    IOleCommandTarget * pCommandTarget = NULL;
    HRESULT             hr = E_FAIL;

    if (!pvarOut)
    {
        hr = S_OK;
        goto Cleanup;
    }
    
    // No need for a security check if this is a trusted context.
    if (_fTrustedDoc)
        fSecurityCheck = FALSE;

    if (V_VT(pvarIn) == VT_UNKNOWN && V_UNKNOWN(pvarIn))
    {
        hr = THR_NOTRACE(V_UNKNOWN(pvarIn)->QueryInterface(
                IID_IHTMLWindow2, (void **)&pWindow));
    }
    else if (V_VT(pvarIn) == VT_DISPATCH && V_DISPATCH(pvarIn))
    {
        hr = THR_NOTRACE(V_DISPATCH(pvarIn)->QueryInterface(
                IID_IHTMLWindow2, (void **)&pWindow));
    }

    if (hr)
    {
        //
        // Object being retrieved is not a window object.
        // Just proceed normally.
        //

        VariantCopy(pvarOut, pvarIn);
        hr = S_OK;
    }
    else
    {
        IHTMLWindow2 *  pWindowOut;

        if (fSecurityCheck)
        {
            BYTE        abSID[MAX_SIZE_SECURITY_ID];
            DWORD       cbSID = ARRAY_SIZE(abSID);
            CVariant    varCallerSID;

            hr = THR(GetCallerCommandTarget(this, pSrvProvider, FALSE, &pCommandTarget));
            if (FAILED(hr))
                goto Cleanup;

            if (hr == S_FALSE && pSrvProvider==NULL)
            {
                // the ONLY way for this to happen is if the call has come in on the VTable
                // rather than through script.  But lets be paranoid about that.
                // To work properly, and to be consistent with
                // the Java VM, if we don't have a securityProvicer in the AA, or passed in
                // then assume trusted.
                 if (AA_IDX_UNKNOWN == FindAAIndex (DISPID_INTERNAL_INVOKECONTEXT,CAttrValue::AA_Internal))
                    hr = S_OK;      // succeed anyhow, else leave the hr=S_FALSE
            }
            else if (pCommandTarget)
            {
                hr = THR(pCommandTarget->Exec(
                        &CGID_ScriptSite,
                        CMDID_SCRIPTSITE_SID,
                        0,
                        NULL,
                        &varCallerSID));
                if (hr)
                    goto Cleanup;

                Assert(V_VT(&varCallerSID) == VT_BSTR);
                Assert(FormsStringLen(V_BSTR(&varCallerSID)) == MAX_SIZE_SECURITY_ID);

                memset(abSID, 0, cbSID);
                hr = THR(GetSIDOfDispatch(_pWindow, abSID, &cbSID));
                if (hr)
                    goto Cleanup;

                if (memcmp(abSID, V_BSTR(&varCallerSID), MAX_SIZE_SECURITY_ID))
                {
                    DWORD dwPolicy = 0;
                    DWORD dwContext = 0;
                    CStr cstrCallerUrl;

                    hr = THR(GetCallerURL(cstrCallerUrl, this, pSrvProvider));
                    if (hr)
                        goto Cleanup;

                    if (    !SUCCEEDED(ZoneCheckUrlEx(cstrCallerUrl, &dwPolicy, sizeof(dwPolicy), &dwContext, sizeof(dwContext),
                                      URLACTION_HTML_SUBFRAME_NAVIGATE, 0, NULL))
                        ||  GetUrlPolicyPermissions(dwPolicy) != URLPOLICY_ALLOW)
                    {
                        hr = E_ACCESSDENIED;
                        goto Cleanup;
                    }
                }
            }
        }

        if (hr)
            goto Cleanup;

        Assert(pWindow);
        hr = THR(SecureObject(pWindow, &pWindowOut));
        if (hr)
            goto Cleanup;

        V_VT(pvarOut) = VT_DISPATCH;
        V_DISPATCH(pvarOut) = pWindowOut;
    }
    
Cleanup:
    ReleaseInterface(pCommandTarget);
    ReleaseInterface(pWindow);
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::SecureWindow
//
//  Synopsis:   Wrap the correct proxy around this object if
//              necessary.  Check in cache if a proxy already exists
//              for this combination.
//
//--------------------------------------------------------------------------
HRESULT
COmWindowProxy::SecureObject(
    IHTMLWindow2 *pWindowIn, 
    IHTMLWindow2 **ppWindowOut)
{
    IHTMLWindow2 *      pWindow = NULL;
    HRESULT             hr = S_OK;
    COmWindowProxy *    pProxy = NULL;
    COmWindowProxy *    pProxyIn = NULL;

    if (!pWindowIn)
    {
        *ppWindowOut = NULL;
        goto Cleanup;
    }

    //
    // First if pWindowIn is itself a proxy, delve all the way
    // through to find the real IHTMLWindow2 that it's bound to
    //

    hr = THR_NOTRACE(pWindowIn->QueryInterface(
            CLSID_HTMLWindowProxy, (void **)&pProxyIn));
    if (!hr)
    {
        //
        // No need to further delve down because with this check
        // we're asserting that a proxy to a proxy can never exist.
        // Remember that pProxyIn is a weak ref.
        //
        
        pWindowIn = pProxyIn->_pWindow;
    }

    //
    // Create a new proxy with this window's security context for the 
    // new window.  Test the cache to see if we already have a proxy 
    // created for this combination first.
    //

    hr = THR(EnsureWindowInfo());
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(TLS(windowInfo.paryWindowTbl)->FindProxy(
            pWindowIn, 
            _pbSID,
            _cbSID,
            _fTrustedDoc,
            &pWindow));
    if (!hr)
    {
        //
        // We found an entry, just return this one.
        //

        *ppWindowOut = pWindow;
        pWindow->AddRef();
    }
    else
    {
        //
        // No entry in cache for this tuple, so create a new proxy
        // and add to cache
        //
        
        pProxy = new COmWindowProxy;
        if (!pProxy)
            RRETURN(E_OUTOFMEMORY);

        hr = THR(pProxy->Init(pWindowIn, _pbSID, _cbSID));
        if (hr)
            goto Cleanup;

        // Set the trusted attribute for this new proxy.  If this proxy is
        // for a trusted doc, the new one should be too.
        pProxy->_fTrustedDoc = _fTrustedDoc;

        // Implicit AddRef/Release for pProxy
        *ppWindowOut = (IHTMLWindow2 *)pProxy;

        hr = THR(TLS(windowInfo.paryWindowTbl)->AddTuple(
                pWindowIn, 
                _pbSID, 
                _cbSID,
                _fTrustedDoc,
                *ppWindowOut));
        if (hr)
            goto Cleanup;
    }
    
Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::AccessAllowed
//
//  Synopsis:   Tell if access is allowed based on urls.
//
//  Returns:    TRUE if allowed, FALSE otherwise.
//
//  Notes:      Access is allowed if the second tier domain is the
//              same.  I.e. www.usc.edu and ftp.usc.edu can access
//              each other.  However, www.usc.com and www.usc.edu 
//              cannot.  Neither can www.stanford.edu and www.usc.edu.
//
//--------------------------------------------------------------------------

BOOL
COmWindowProxy::AccessAllowed()
{
    HRESULT hr;
    BYTE    abSID[MAX_SIZE_SECURITY_ID];
    DWORD   cbSID = ARRAY_SIZE(abSID);
    
    if (_fTrusted || _fTrustedDoc)
        return TRUE;

    hr = THR(GetSIDOfDispatch(_pWindow, abSID, &cbSID));
    if (hr)
        return FALSE;
        
    return (cbSID == _cbSID && !memcmp(abSID, _pbSID, cbSID));
}

// Same as above but use passed in IDispatch *
//
BOOL
COmWindowProxy::AccessAllowed(IDispatch *pDisp)
{
    HRESULT hr;
    CStr    cstr;
    BYTE    abSID[MAX_SIZE_SECURITY_ID];
    DWORD   cbSID = ARRAY_SIZE(abSID);

    hr = THR(GetSIDOfDispatch(pDisp, abSID, &cbSID));
    if (hr)
        return FALSE;
        
    return (cbSID == _cbSID && !memcmp(abSID, _pbSID, cbSID));
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::Window
//
//--------------------------------------------------------------------------

COmWindow2 *
COmWindowProxy::Window()
{
    COmWindow2 *    pWindow = NULL;

    IGNORE_HR(_pWindow->QueryInterface(CLSID_HTMLWindow2, (void**)&pWindow));

    return pWindow;
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::FireEvent
//
//  Synopsis:   CBase doesn't allow an EVENTPARAM, which we need.
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::FireEvent(
    DISPID dispidEvent, 
    DISPID dispidProp, 
    LPCTSTR pchEventType,
    BYTE * pbTypes, 
    ...)
{
    va_list         valParms;
    HRESULT         hr;
    COmWindow2     *pWindow = Window();
    IDispatch      *pEventObj = NULL;

    // Creating this causes it to be added to a linked list on the
    // doc. Even though this looks like param is not used, DON'T REMOVE
    // THIS CODE!!
    EVENTPARAM      param(pWindow->_pDoc, TRUE);
    param.SetType(pchEventType);

    va_start(valParms, pbTypes);

    // Get the eventObject.
    IGNORE_HR(pWindow->get_event((IHTMLEventObj **)&pEventObj));
    
    hr = THR(FireEventV(dispidEvent, 
                        dispidProp,
                        pEventObj,
                        NULL,
                        pbTypes,
                        valParms));
    // Any attachEvents?  Need to fire on window itself.
    hr = THR(pWindow->FireAttachEventV(
                dispidEvent, 
                dispidProp,
                pEventObj,
                NULL, 
                pWindow, 
                pbTypes, 
                valParms));

    ReleaseInterface(pEventObj);

    va_end(valParms);



    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::Fire_onerror
//
//  Synopsis:   Fires the onerror event and returns S_FALSE if ther was not script
//              or the script returned S_FALSE requesting default processing
//
//       NOTE:  While this event handler returns TRUE to tell the browser to take no 
//                further action, most Form and form element event handles return false to 
//                prevent the browser from performing some action such as submitting a form.
//              This inconsistency can be confusing.
//--------------------------------------------------------------------------

BOOL 
COmWindowProxy::Fire_onerror(BSTR bstrMessage, BSTR bstrUrl,
                             long lLine, long lCharacter, long lCode,
                             BOOL fWindow)
{
    HRESULT         hr;
    VARIANT_BOOL    fRet = VB_FALSE;
    COmWindow2     *pWindow = Window();
    CDoc *          pDoc = pWindow->_pDoc;
    EVENTPARAM      param(pDoc, TRUE);

    param.SetType(_T("error"));
    param.errorParams.pchErrorMessage = bstrMessage;
    param.errorParams.pchErrorUrl = bstrUrl;
    param.errorParams.lErrorLine = lLine;
    param.errorParams.lErrorCharacter = lCharacter;
    param.errorParams.lErrorCode = lCode;

    if (fWindow)
    {
        hr = THR(pWindow->FireCancelableEvent(DISPID_EVMETH_ONERROR, DISPID_EVPROP_ONERROR, NULL, &fRet,
            byOnErrorParamTypes, bstrMessage, bstrUrl, lLine));
        if (hr)
            goto Cleanup;

        hr = pDoc->ShowErrorDialog(&fRet);
    }
    else
        hr = THR(FireCancelableEvent(DISPID_EVMETH_ONERROR, DISPID_EVPROP_ONERROR, NULL, &fRet,
            byOnErrorParamTypes, bstrMessage, bstrUrl, lLine));

    if (hr)
        goto Cleanup;

    if (    (fRet != VB_TRUE)
        &&  (V_VT(&param.varReturnValue) == VT_BOOL)
        &&  (V_BOOL(&param.varReturnValue) == VB_TRUE))
        fRet = VB_TRUE;

Cleanup:  
    return (fRet == VB_TRUE);
}

    

//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::Fire_onscroll
//
//  Synopsis:
//
//--------------------------------------------------------------------------

void
COmWindowProxy::Fire_onscroll()
{
    FireEvent(DISPID_EVMETH_ONSCROLL, DISPID_EVPROP_ONSCROLL, _T("scroll"), (BYTE *) VTS_NONE);

    Window()->Fire_onscroll();
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::Fire_onload, Fire_onunload
//
//  Synopsis:   Fires the onload/onunload events of the window
//      these all call the CBase::FireEvent variant that takes the CBase *
//
//--------------------------------------------------------------------------

void
COmWindowProxy::Fire_onload()
{
    COmWindow2     *pWindow = Window();

	FireEvent(DISPID_EVMETH_ONLOAD, DISPID_EVPROP_ONLOAD, _T("load"), (BYTE *) VTS_NONE);

    pWindow->Fire_onload();

    ClearInterface(&(pWindow->_pDoc->_pXMLHistoryUserData));
    pWindow->_pDoc->_cstrHistoryUserData.Free();
    ClearInterface(&(pWindow->_pDoc->_pShortcutUserData));

    // fire for accessibility, if its enabled.
    Window()->_pDoc->FireAccesibilityEvents(DISPID_EVMETH_ONLOAD, OBJID_WINDOW);
}

void
COmWindowProxy::Fire_onunload()
{
	FireEvent(DISPID_EVMETH_ONUNLOAD, DISPID_EVPROP_ONUNLOAD, _T("unload"), (BYTE *) VTS_NONE);

    // fire for window connection points 
    Window()->Fire_onunload();

    // fire for accessibility, if its enabled
    Window()->_pDoc->FireAccesibilityEvents(DISPID_EVMETH_ONUNLOAD, OBJID_WINDOW);
}

BOOL 
COmWindowProxy::Fire_onhelp(BYTE * pbTypes, ...)
{
    va_list         valParms;
    COmWindow2     *pWindow = Window();
    EVENTPARAM      param(pWindow->_pDoc, TRUE);
    BOOL            fRet = TRUE;
    VARIANT_BOOL    vb;
    CVariant        Var;
    IDispatch      *pEventObj = NULL;

    param.SetType(_T("help"));

    va_start(valParms, pbTypes);

    // Get the eventObject.
    IGNORE_HR(pWindow->get_event((IHTMLEventObj **)&pEventObj));
    
    IGNORE_HR(FireEventV(
                DISPID_EVMETH_ONHELP, 
                DISPID_EVPROP_ONHELP,
                pEventObj,
                &Var, 
                pbTypes, 
                valParms));

    ReleaseInterface(pEventObj);

    va_end(valParms);


    vb = (V_VT(&Var) == VT_BOOL) ? V_BOOL(&Var) : VB_TRUE;

    fRet = !param.IsCancelled() && VB_TRUE == vb;

    return (fRet && Window()->Fire_onhelp());
}

void 
COmWindowProxy::Fire_onresize()
{
    FireEvent( DISPID_EVMETH_ONRESIZE, DISPID_EVPROP_ONRESIZE, _T("resize"), (BYTE *) VTS_NONE);

    Window()->Fire_onresize();
}

//
// For the alpha compiler, we include this hack.  Please repair this.
//
static HRESULT
FireHack_onbeforeunload(COmWindowProxy * pProxy, CDoc * pDoc, VARIANT & varRetval, BYTE * pbTypes, ...)
{
    HRESULT			hr;
    COmWindow2     *pWindow = pProxy->Window();
	va_list			valParms;
    IDispatch      *pEventObj = NULL;

    va_start(valParms, pbTypes);

    // Get the eventObject.
    IGNORE_HR(pWindow->get_event((IHTMLEventObj **)&pEventObj));
    
    hr = THR(pProxy->FireEventV(DISPID_EVMETH_ONBEFOREUNLOAD,
                      DISPID_EVPROP_ONBEFOREUNLOAD,
                      pEventObj,
                      &varRetval,
                      (BYTE *) VTS_NONE,
                      valParms));

	// Any attachEvents?  Need to fire on window itself.
    hr = THR(pWindow->FireAttachEventV(
                        DISPID_EVMETH_ONBEFOREUNLOAD, 
                        DISPID_EVPROP_ONBEFOREUNLOAD,
                        pEventObj, NULL, pWindow, pbTypes, valParms));

    ReleaseInterface(pEventObj);

    va_end(valParms);

    RRETURN(hr);
}

BOOL
COmWindowProxy::Fire_onbeforeunload()
{
    BOOL fRetval = TRUE;
    HRESULT hr = S_OK;
    COmWindow2 *pOmWindow2 = Window();
    CDoc *pDoc = pOmWindow2 ? pOmWindow2->_pDoc : NULL;
    if (pDoc)
    {
        CVariant    varRetval;
        CVariant  * pvarString = NULL;
        EVENTPARAM  param(pDoc, TRUE);

        param.SetType(_T("beforeunload"));

        hr = THR(FireHack_onbeforeunload(this, pDoc, varRetval, (BYTE *)VTS_NONE));
    
        // if we have an event.retValue use that rather then the varRetval
        if (S_FALSE != param.varReturnValue.CoerceVariantArg(VT_BSTR))
        {
            pvarString = &param.varReturnValue;
        }
        else if (S_FALSE != varRetval.CoerceVariantArg(VT_BSTR))
        {
            pvarString = &varRetval;
        }

        // if we have a return string, show it
        if (!hr && pvarString)
        {
            int iResult = 0;
            TCHAR *pstr;
            Format(FMT_OUT_ALLOC,
                   &pstr,
                   64,
                   _T("<0i>\n\n<2s>\n\n<3i>"),
                   GetResourceHInst(),
                   IDS_ONBEFOREUNLOAD_PREAMBLE,
                   V_BSTR(pvarString),
                   GetResourceHInst(),
                   IDS_ONBEFOREUNLOAD_POSTAMBLE);
            pDoc->ShowMessageEx(&iResult,
                          MB_OKCANCEL | MB_ICONWARNING | MB_SETFOREGROUND,
                          NULL,
                          0,
                          pstr);
            delete pstr;
            if (iResult == IDCANCEL)
                fRetval = FALSE;
        }
        if (fRetval && pDoc->_fHasOleSite)
        {
            CNotification   nf;
            
            nf.BeforeUnload(pDoc->PrimaryRoot(), &fRetval);
            pDoc->BroadcastNotify(&nf);
        }
    }
    if (fRetval)
        fRetval = Window()->Fire_onbeforeunload();
    return fRetval;
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::Fire_onfocus, Fire_onblur
//
//  Synopsis:   Fires the onfocus/onblur events of the window
//
//--------------------------------------------------------------------------

void COmWindowProxy::Fire_onfocus(DWORD_PTR dwContext)
{                   
    CDoc::CLock LockForm(Window()->_pDoc, FORMLOCK_CURRENT);

    FireEvent(DISPID_EVMETH_ONFOCUS, DISPID_EVPROP_ONFOCUS, _T("focus"), (BYTE *) VTS_NONE);

    // fire for window connection points 
    Window()->Fire_onfocus();

    // fire for accessibility, if its enabled
    Window()->_pDoc->FireAccesibilityEvents(DISPID_EVMETH_ONFOCUS, OBJID_WINDOW);
}

void COmWindowProxy::Fire_onblur(DWORD_PTR dwContext)
{
    CDoc::CLock LockForm(Window()->_pDoc, FORMLOCK_CURRENT);

    Window()->_pDoc->_fModalDialogInOnblur = (BOOL)dwContext;
    FireEvent(DISPID_EVMETH_ONBLUR, DISPID_EVPROP_ONBLUR, _T("blur"), (BYTE *) VTS_NONE);

    // fire for window connection points 
    Window()->Fire_onblur();

    // fire for accessibility, if its enabled
    Window()->_pDoc->FireAccesibilityEvents(DISPID_EVMETH_ONBLUR, OBJID_WINDOW);
    Window()->_pDoc->_fModalDialogInOnblur = FALSE;
}

void COmWindowProxy::Fire_onbeforeprint()
{
    FireEvent(DISPID_EVMETH_ONBEFOREPRINT, DISPID_EVPROP_ONBEFOREPRINT, _T("beforeprint"), (BYTE *) VTS_NONE);

    Window()->Fire_onbeforeprint();
}

void COmWindowProxy::Fire_onafterprint()
{
    FireEvent(DISPID_EVMETH_ONAFTERPRINT, DISPID_EVPROP_ONAFTERPRINT, _T("afterprint"), (BYTE *) VTS_NONE);

    Window()->Fire_onafterprint();
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::CanMarshallIID
//
//  Synopsis:   Return TRUE if this iid can be marshalled.
//              Keep in sync with QI below.
//
//--------------------------------------------------------------------------

BOOL
COmWindowProxy::CanMarshalIID(REFIID riid)
{
    return (riid == IID_IUnknown ||
            riid == IID_IDispatch ||
            riid == IID_IHTMLFramesCollection2 ||
            riid == IID_IHTMLWindow2 ||
            riid == IID_IHTMLWindow3 ||
            riid == IID_IDispatchEx ||
            riid == IID_IObjectIdentity ||
            riid == IID_IServiceProvider) ? TRUE : FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::IsOleProxy
//
//  Returns:    BOOL      True if _pWindow is actually an OLE proxy.
//
//  Notes:      It performs this check by QI'ing for IClientSecurity.
//              Only OLE proxies implement this interface.
//
//----------------------------------------------------------------------------

BOOL
COmWindowProxy::IsOleProxy()
{
    BOOL                fRet = FALSE;

#ifndef WIN16       //BUGWIN16 deal with this for cross process support
    IClientSecurity *   pCL;

    if (OK(_pWindow->QueryInterface(IID_IClientSecurity, (void **)&pCL)))
    {
        ReleaseInterface(pCL);

        // Only proxy objects should support this interface.
        fRet = TRUE;
    }
#endif

    return fRet;
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::PrivateQueryInterface
//
//  Synopsis:   Per IPrivateUnknown
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::PrivateQueryInterface(REFIID iid, LPVOID * ppv)
{
    void *  pvObject = NULL;
    HRESULT hr = S_OK;
    
    *ppv = NULL;
    
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_INHERITS(this, IDispatchEx)
        QI_INHERITS(this, IHTMLWindow2)
        QI_INHERITS(this, IHTMLWindow3)
        QI_INHERITS2(this, IHTMLFramesCollection2, IHTMLWindow2)
        QI_INHERITS2(this, IDispatch, IHTMLWindow2)
        QI_TEAROFF(this, IMarshal, NULL)
        QI_TEAROFF(this, IObjectIdentity, NULL)
        QI_TEAROFF(this, IServiceProvider, NULL);

        // NOTE: Any new tearoffs or inheritance classes in the QI need to have
        //       the appropriate IID placed in the COmWindowProxy::CanMarshallIID
        //       function.  This allows the marshaller/unmarshaller to actually
        //       support this object being created if any of the above interface
        //       are asked for first.  Once the object is marshalled all
        //       subsequent QI's are to the same marshalled object.

        QI_CASE(IConnectionPointContainer)
            if (iid == IID_IConnectionPointContainer)
            {
                if (IsOleProxy())
                {
                    // IConnectionPointerContainer interface is to the real
                    // window object not this marshalled proxy.  The IUknown
                    // will be this objects (for identity). 
                    goto DelegateToWindow;
                }
                else
                {
                    *((IConnectionPointContainer **)ppv) =
                            new CConnectionPointContainer(this, NULL);

                    if (!*ppv)
                        RRETURN(E_OUTOFMEMORY);
                }
            }
            break;
            
        QI_CASE(IProvideMultipleClassInfo)
        QI_CASE(IProvideClassInfo)
        QI_CASE(IProvideClassInfo2)
            if (iid == IID_IProvideMultipleClassInfo ||
                iid == IID_IProvideClassInfo ||
                iid == IID_IProvideClassInfo2)
            {
DelegateToWindow:
                //
                // For these cases, just delegate on down to the real window
                // with our IUnknown.
                //
                hr = THR_NOTRACE(_pWindow->QueryInterface(iid, &pvObject));
                if (hr)
                    RRETURN(hr);

                hr = THR(CreateTearOffThunk(
                        pvObject, 
                        *(void **)pvObject,
                        NULL,
                        ppv,
                        (IUnknown *)(IPrivateUnknown *)this,
                        *(void **)(IUnknown *)(IPrivateUnknown *)this,
                        1,      // Call QI on object 2.
                        NULL));

                ((IUnknown *)pvObject)->Release();
                if (!*ppv)
                    RRETURN(E_OUTOFMEMORY);
            }
            break;
        
        default:
            if (DispNonDualDIID(iid))
            {
                *ppv = (IHTMLWindow2 *)this;
            }
            else if (iid == CLSID_HTMLWindowProxy)
            {
                *ppv = this; // Weak ref.
                return S_OK;
            }
            break;
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        DbgTrackItf(iid, "COmWindowProxy", FALSE, ppv);
        return S_OK;
    }

    return E_NOINTERFACE;
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::GetTypeInfoCount
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::GetTypeInfoCount(UINT FAR* pctinfo)
{
    TraceTag((tagSecurity, "GetTypeInfoCount"));

    RRETURN(_pWindow->GetTypeInfoCount(pctinfo));
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::GetTypeInfo
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo ** pptinfo)
{
    TraceTag((tagSecurity, "GetTypeInfo"));

    RRETURN(_pWindow->GetTypeInfo(itinfo, lcid, pptinfo));
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::GetIDsOfNames
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::GetIDsOfNames(
    REFIID                riid,
    LPOLESTR *            rgszNames,
    UINT                  cNames,
    LCID                  lcid,
    DISPID FAR*           rgdispid)
{
    HRESULT hr;
    
    hr = THR(_pWindow->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid));

#ifndef WIN16
    if (OK(hr))
        goto Cleanup;

    //
    // Catch the case where the thread has already gone down.  Remember
    // that _pWindow may be a marshalled pointer to a window object.
    // In this case we want to try our own GetIDsOfNames to come up with
    // plausible responses.
    //
    
    if (WINDOWDEAD())
    {
        if (OK(super::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid)))
        {
            hr = S_OK;
            goto Cleanup;
        }
    }
Cleanup:
#endif //!WIN16    
    RRETURN(hr);
}

HRESULT
COmWindowProxy::Invoke(
    DISPID          dispid,
    REFIID,
    LCID            lcid,
    WORD            wFlags,
    DISPPARAMS *    pdispparams,
    VARIANT *       pvarResult,
    EXCEPINFO *     pexcepinfo,
    UINT *)
{
    return InvokeEx(dispid,
                    lcid,
                    wFlags,
                    pdispparams,
                    pvarResult,
                    pexcepinfo,
                    NULL);
}
//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::InvokeEx
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::InvokeEx(DISPID dispidMember,
                         LCID lcid,
                         WORD wFlags,
                         DISPPARAMS *pdispparams,
                         VARIANT *pvarResult,
                         EXCEPINFO *pexcepinfo,
                         IServiceProvider *pSrvProvider)
{
    TraceTag((tagSecurity, "Invoke dispid=0x%x", dispidMember));

    HRESULT     hr;
    CVariant    Var;
    
    //
    // Look for known negative dispids that we can answer
    // or forward safely.
    //

    switch (dispidMember)
    {
    case DISPID_WINDOWOBJECT:
        //
        // Return a ptr to the real window object.
        //

        V_VT(pvarResult) = VT_DISPATCH;
        V_DISPATCH(pvarResult) = _pWindow;
        _pWindow->AddRef();
        hr = S_OK;
        goto Cleanup;

    case DISPID_LOCATIONOBJECT:
    case DISPID_NAVIGATOROBJECT:
    case DISPID_HISTORYOBJECT:
    case DISPID_SECURITYCTX:
    case DISPID_SECURITYDOMAIN:
        //
        // Forward these on safely.
        //

        // BUGBUG rgardner need to QI for Ex2 & Invoke ??
        hr = THR(_pWindow->Invoke(dispidMember,
                                  IID_NULL,
                                  lcid,
                                  wFlags,
                                  pdispparams,
                                  pvarResult,
                                  pexcepinfo,
                                  NULL));
        goto Cleanup;

    default:
        break;
    }

    //
    // If dispid is within range for named or indexed frames
    // pass through and secure the returned object.
    //

    // Note that we don't try & verify which collection it's in - we just
    // want to know if its in any collection at all
    if ( dispidMember >= CMarkup::FRAME_COLLECTION_MIN_DISPID && 
        dispidMember <=  CMarkup::FRAME_COLLECTION_MAX_DISPID )
    {
        hr = THR(_pWindow->Invoke(dispidMember,
                                  IID_NULL,
                                  lcid,
                                  wFlags,
                                  pdispparams,
                                  &Var,
                                  pexcepinfo,
                                  NULL));
        if (!hr)
        {
            hr = THR(SecureObject(&Var, pvarResult, pSrvProvider, TRUE)); // fSecurityCheck = TRUE
        }
        goto Cleanup;
    }
    
    //
    // Now try automation based invoke as long as the dispid
    // is not an expando or omwindow method dispid.
    //

    // also, disable getting in super::InvokeEx when DISPID_COmWindow2_showModalDialog:
    // this will make us to invoke _pWindow directly so caller chain will be available there.

    if (!IsExpandoDISPID(dispidMember) && 
        dispidMember < DISPID_OMWINDOWMETHODS &&
        dispidMember != DISPID_COmWindow2_showModalDialog)
    {
        hr = THR(super::InvokeEx(dispidMember,
                                 lcid,
                                 wFlags,
                                 pdispparams,
                                 pvarResult,
                                 pexcepinfo,
                                 pSrvProvider));

        // if above failed, allow the dialogTop\Left\Width\Height properties to go through else bailout
        // Note that these std dispids are defined only for the dlg objeect and not the window object,
        // so if they fall in this range, it must be an invoke for the dialog properties
        if (!(hr && dispidMember >= STDPROPID_XOBJ_LEFT && dispidMember <= STDPROPID_XOBJ_HEIGHT))
            goto Cleanup;
    }
    
    //
    // If automation invoke also failed, then only allow the invoke to
    // pass through if the security allows it.
    // Is this too stringent?
    //

    if (AccessAllowed())
    {
        IDispatchEx *pDEX2=NULL;
        hr = THR(_pWindow->QueryInterface ( IID_IDispatchEx, (void**)&pDEX2 ));
        if ( hr )
            goto Cleanup;

        Assert(pDEX2);
        hr = THR(pDEX2->InvokeEx(dispidMember,
                                 lcid,
                                 wFlags,
                                 pdispparams,
                                 &Var,
                                 pexcepinfo,
                                 pSrvProvider));

        ReleaseInterface(pDEX2);

        if (!hr)
        {
            hr = THR(SecureObject(&Var, pvarResult, NULL, FALSE));  // fSecurityCheck = FALSE
            if (hr)
                goto Cleanup;
        }
    }
    else
    {
        hr = SetErrorInfo(E_ACCESSDENIED);
    }
    
Cleanup:
    // for IE3 backward compat, map the below old dispids to their corresponding new
    // ones, on failure.
    if (hr && hr != E_ACCESSDENIED)
    {
        switch(dispidMember)
        {
        case 1:             dispidMember = DISPID_COmWindow2_length;
                            break;

        case 0x60020000:    dispidMember = DISPID_COmWindow2_name;
                            break;

        case 0x60020003:    dispidMember = DISPID_COmWindow2_item;
                            break;

        case 0x60020006:    dispidMember = DISPID_COmWindow2_location;
                            break;

        case 0x60020007:    dispidMember = DISPID_COmWindow2_frames;
                            break;

        default:
                            RRETURN_NOTRACE(hr);
        }

        hr = THR(super::InvokeEx(dispidMember,
                                 lcid,
                                 wFlags,
                                 pdispparams,
                                 pvarResult,
                                 pexcepinfo,
                                 pSrvProvider));
    }

    RRETURN_NOTRACE(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::GetDispID
//
//  Synopsis:   Per IDispatchEx
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT         hr;
    IDispatchEx *  pDispEx2 = NULL;
    BOOL            fDenied = FALSE;

    // Cache the IDispatchEx ptr?
    hr = THR(_pWindow->QueryInterface(IID_IDispatchEx, (void **)&pDispEx2));
    if (hr)
        goto Cleanup;
        
    if ((grfdex & fdexNameEnsure) && !AccessAllowed())
    {
        //
        // If access is not allowed, don't even allow creation of an
        // expando on the remote side.
        //
        
        grfdex &= ~fdexNameEnsure;
        fDenied = TRUE;
    }

    hr = THR_NOTRACE(pDispEx2->GetDispID(bstrName, grfdex, pid));

    if (fDenied && DISP_E_UNKNOWNNAME == hr)
    {
        hr = E_ACCESSDENIED;
    }

Cleanup:
#ifndef WIN16
    if (WINDOWDEAD())
    {
        //
        // This means the thread on the remote side has gone down.
        // Try to get out a plausible answer from CBase.  Otherwise
        // just bail.  Of course, never create an expando in such a
        // situation.
        //
        
        grfdex &= ~fdexNameEnsure;
        if (OK(super::GetDispID(bstrName, grfdex, pid)))
        {
            hr = S_OK;
        }
    }
#endif //!WIN16
    ReleaseInterface(pDispEx2);
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBase::DeleteMember, IDispatchEx
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::DeleteMemberByName(BSTR bstr,DWORD grfdex)
{
    return E_NOTIMPL;
}

HRESULT
COmWindowProxy::DeleteMemberByDispID(DISPID id)
{
    return E_NOTIMPL;
}



//+-------------------------------------------------------------------------
//
//  Method:     CBase::GetMemberProperties, IDispatchEx
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::GetMemberProperties(
                DISPID id,
                DWORD grfdexFetch,
                DWORD *pgrfdex)
{
    return E_NOTIMPL;
}


HRESULT
COmWindowProxy::GetMemberName (DISPID id, BSTR *pbstrName)
{
    HRESULT         hr;
    IDispatchEx *  pDispEx2 = NULL;
    
    // Cache the IDispatchEx ptr?
    hr = THR(_pWindow->QueryInterface(IID_IDispatchEx, (void **)&pDispEx2));
    if (hr)
        goto Cleanup;
        
    hr = THR(pDispEx2->GetMemberName(id, pbstrName));

Cleanup:
    ReleaseInterface(pDispEx2);
    RRETURN1(hr,S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::GetNextDispID
//
//  Synopsis:   Per IDispatchEx
//
//--------------------------------------------------------------------------
HRESULT
COmWindowProxy::GetNextDispID(DWORD grfdex,
                              DISPID id,
                              DISPID *prgid)
{
    HRESULT         hr;
    IDispatchEx *  pDispEx2 = NULL;
    
    // Cache the IDispatchEx ptr?
    hr = THR(_pWindow->QueryInterface(IID_IDispatchEx, (void **)&pDispEx2));
    if (hr)
        goto Cleanup;
        
    hr = THR(pDispEx2->GetNextDispID(grfdex, id, prgid));

Cleanup:
    ReleaseInterface(pDispEx2);
    RRETURN1(hr,S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::GetNameSpaceParent
//
//  Synopsis:   Per IDispatchEx
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::GetNameSpaceParent(IUnknown **ppunk)
{
    HRESULT         hr;
    IDispatchEx *  pDEX = NULL;

    if (!ppunk)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppunk = NULL;

    hr = THR(_pWindow->QueryInterface(IID_IDispatchEx, (void**) &pDEX));
    if (hr)
        goto Cleanup;
        
    hr = THR(pDEX->GetNameSpaceParent(ppunk));

Cleanup:
    ReleaseInterface (pDEX);
    RRETURN(hr);
}


#define IMPLEMENT_SECURE_OBJECTGET(meth)            \
    HRESULT hr = S_OK;                              \
                                                    \
    *ppOut = NULL;                                  \
    hr = THR(_pWindow->meth(&pReal));               \
    if (hr)                                         \
        goto Cleanup;                               \
    if (AccessAllowed())                            \
    {                                               \
        *ppOut = pReal;                             \
        if (pReal)                                  \
            pReal->AddRef();                        \
        hr = S_OK;                                  \
    }                                               \
    else                                            \
    {                                               \
        hr = E_ACCESSDENIED;                        \
    }                                               \
Cleanup:                                            \
    ReleaseInterface(pReal);                        \

#define IMPLEMENT_SECURE_PROPGET(meth)              \
    HRESULT hr = S_OK;                              \
                                                    \
    *ppOut = NULL;                                  \
    hr = THR(_pWindow->meth(ppOut));                \
    if (hr)                                         \
        goto Cleanup;                               \
    if (!AccessAllowed())                           \
    {                                               \
        hr = E_ACCESSDENIED;                        \
    }                                               \

#define IMPLEMENT_SECURE_PROPGET3(meth)             \
    HRESULT hr;                                     \
    if (AccessAllowed())                            \
    {                                               \
        IHTMLWindow3 *pWin3 = NULL;                 \
        hr = THR(_pWindow->QueryInterface(IID_IHTMLWindow3, (void **)&pWin3));  \
        if(hr)                                      \
            goto Cleanup;                           \
        hr = THR(pWin3->meth);                      \
        ReleaseInterface(pWin3);                    \
    }                                               \
    else                                            \
        hr = E_ACCESSDENIED;                        \
Cleanup:                                            \

#define IMPLEMENT_SECURE_WINDOWGET(meth)            \
    HRESULT             hr = S_OK;                  \
    IHTMLWindow2 *      pWindow = NULL;             \
                                                    \
    *ppOut = NULL;                                  \
    hr = THR(_pWindow->meth(&pWindow));             \
    if (hr)                                         \
        goto Cleanup;                               \
    hr = THR(SecureObject(pWindow, ppOut));         \
    if (hr)                                         \
        goto Cleanup;                               \
Cleanup:                                            \
    ReleaseInterface(pWindow);                      \
    

#define IMPLEMENT_SECURE_METHOD(meth)               \
    HRESULT hr = S_OK;                              \
                                                    \
    if (AccessAllowed())                            \
    {                                               \
        hr = THR(_pWindow->meth);                   \
        if (hr)                                     \
            goto Cleanup;                           \
    }                                               \
    else                                            \
    {                                               \
        hr = E_ACCESSDENIED;                        \
    }                                               \
Cleanup:                                            \


#define IMPLEMENT_SECURE_METHOD3(meth)              \
    HRESULT hr = S_OK;                              \
    if (AccessAllowed())                            \
    {                                               \
        IHTMLWindow3 *pWin3 ;                       \
        hr = THR(_pWindow->QueryInterface(IID_IHTMLWindow3, (void **)&pWin3)); \
        if(hr)                                      \
            goto Cleanup;                           \
        hr = THR(pWin3->meth);                      \
        ReleaseInterface(pWin3);                    \
    }                                               \
    else                                            \
        hr = E_ACCESSDENIED;                        \
Cleanup:;

//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::get_document
//
//  Synopsis:   Per IOmWindow
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::get_document(IHTMLDocument2 **ppOut)
{
    IHTMLDocument2 *    pReal = NULL;

    IMPLEMENT_SECURE_OBJECTGET(get_document)
    RRETURN(SetErrorInfo(hr));    
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::get_frames
//
//  Synopsis:   Per IOmWindow
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::get_frames(IHTMLFramesCollection2 ** ppOut)
{
    IHTMLFramesCollection2 *    pColl = NULL;
    HRESULT                     hr = S_OK;
    IHTMLWindow2 *              pWindow = NULL;
    IHTMLWindow2 *              pWindowOut = NULL;
    
    *ppOut = NULL;

    hr = THR(_pWindow->get_frames(&pColl));
    if (hr)
        goto Cleanup;

    hr = THR(pColl->QueryInterface(
            IID_IHTMLWindow2, (void **)&pWindow));
    if (hr)
        goto Cleanup;
        
    hr = THR(SecureObject(pWindow, &pWindowOut));
    if (hr)
        goto Cleanup;

    *ppOut = pWindowOut;
    
Cleanup:
    ReleaseInterface(pWindow);
    ReleaseInterface(pColl);
    RRETURN(SetErrorInfo(hr));    
}


//+----------------------------------------------------------------------------------
//
//  Member:     item
//
//  Synopsis:   object model implementation
//
//              we handle the following parameter cases:
//                  0 params:               returns IDispatch of this
//                  1 param as number N:    returns IDispatch of om window of
//                                          frame # N, or fails if doc is not with
//                                          frameset
//                  1 param as string "foo" returns the om window of the frame
//                                          element that has NAME="foo"
//
//-----------------------------------------------------------------------------------

HRESULT
COmWindowProxy::item(VARIANTARG *pvarArg1, VARIANTARG * pvarRes)
{
    HRESULT     hr = S_OK;
    CVariant    Var;

    hr = THR(_pWindow->item(pvarArg1, &Var));
    if (hr)
        goto Cleanup;

    hr = THR(SecureObject(&Var, pvarRes, NULL, TRUE));  // fSecurityCheck = TRUE
    if (hr)
        goto Cleanup;
        
Cleanup:
    RRETURN(SetErrorInfo(hr));    
}


//+------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::Get_newEnum
//
//  Synopsis:   collection object model
//
//-------------------------------------------------------------------------

HRESULT
COmWindowProxy::get__newEnum(IUnknown **ppOut)
{
    IUnknown *          pReal = NULL;

    IMPLEMENT_SECURE_OBJECTGET(get__newEnum)
    RRETURN(SetErrorInfo(hr));    
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::get_event
//
//  Synopsis:   Per IOmWindow
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::get_event(IHTMLEventObj **ppOut)
{
    IHTMLEventObj * pReal = NULL;

    IMPLEMENT_SECURE_OBJECTGET(get_event)
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::setTimeout
//
//  Synopsis:  Runs <Code> after <msec> milliseconds and returns a bstr <TimeoutID>
//      in case ClearTimeout is called.
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::setTimeout(
    BSTR strCode, 
    LONG lMSec, 
    VARIANT *language, 
    LONG * pTimerID)
{
    IMPLEMENT_SECURE_METHOD(setTimeout(strCode, lMSec, language, pTimerID))
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::setTimeout (w/ VARIANT pCode)
//
//  Synopsis:  Runs <Code> after <msec> milliseconds and returns a bstr <TimeoutID>
//      in case ClearTimeout is called.
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::setTimeout(
    VARIANT *pCode, 
    LONG lMSec, 
    VARIANT *language, 
    LONG * pTimerID)
{
    IMPLEMENT_SECURE_METHOD3(setTimeout(pCode, lMSec, language, pTimerID))
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::clearTimeout
//
//  Synopsis:
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::clearTimeout(LONG timerID)
{
    IMPLEMENT_SECURE_METHOD(clearTimeout(timerID))
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::setInterval
//
//  Synopsis:  Runs <Code> every <msec> milliseconds 
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::setInterval(
    BSTR strCode, 
    LONG lMSec, 
    VARIANT *language, 
    LONG * pTimerID)
{
    IMPLEMENT_SECURE_METHOD(setInterval(strCode, lMSec, language, pTimerID))
    RRETURN(SetErrorInfo(hr));    
}

//+---------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::setInterval (w/ VARIANT pCode)
//
//  Synopsis:  Runs <Code> after <msec> milliseconds and returns a bstr <TimeoutID>
//      in case ClearTimeout is called.
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::setInterval(
    VARIANT *pCode, 
    LONG lMSec, 
    VARIANT *language, 
    LONG * pTimerID)
{
    IMPLEMENT_SECURE_METHOD3(setInterval(pCode, lMSec, language, pTimerID))
    RRETURN(SetErrorInfo(hr));    
}

//+---------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::clearInterval
//
//  Synopsis:   deletes the period timer set by setInterval
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::clearInterval(LONG timerID)
{
    IMPLEMENT_SECURE_METHOD(clearInterval(timerID))
    RRETURN(SetErrorInfo(hr));    
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::get_screen
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::get_screen(IHTMLScreen **ppOut)
{
    IHTMLScreen *   pReal = NULL;
    
    IMPLEMENT_SECURE_OBJECTGET(get_screen)
    RRETURN(SetErrorInfo(hr));    
}

//+---------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::showModalDialog
//
//  Synopsis:   Interface method to bring up a HTMLdialog given a url
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::showModalDialog(
    BSTR         bstrUrl,
    VARIANT *    pvarArgIn,
    VARIANT *    pvarOptions,
    VARIANT *    pvarArgOut)
{
    IMPLEMENT_SECURE_METHOD(showModalDialog(bstrUrl, pvarArgIn, pvarOptions, pvarArgOut))
    RRETURN(SetErrorInfo(hr));    
}


//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::alert
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------

HRESULT
COmWindowProxy::alert(BSTR message)
{
    IMPLEMENT_SECURE_METHOD(alert(message))
    RRETURN(SetErrorInfo(hr));    
}

//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::confirm
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------

HRESULT
COmWindowProxy::confirm(BSTR message, VARIANT_BOOL *pConfirmed)
{
    IMPLEMENT_SECURE_METHOD(confirm(message, pConfirmed))
    RRETURN(SetErrorInfo(hr));    
}


//+-------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_length
//
//  Synopsis:   object model implementation
//
//              returns number of frames in frameset of document;
//              fails if the doc does not contain frameset
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::get_length(long *ppOut)
{
    RRETURN(SetErrorInfo(_pWindow->get_length(ppOut)));
}



//+---------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::showHelp
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::showHelp(BSTR helpURL, VARIANT helpArg, BSTR features)
{
    IMPLEMENT_SECURE_METHOD(showHelp(helpURL, helpArg, features))
    RRETURN(SetErrorInfo(hr));    
}


//+------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::prompt
//
//  Synopsis:   Show a prompt dialog
//
//-------------------------------------------------------------------------

HRESULT
COmWindowProxy::prompt(BSTR message, BSTR defstr, VARIANT *retval)
{
    IMPLEMENT_SECURE_METHOD(prompt(message, defstr, retval))
    RRETURN(SetErrorInfo(hr));    
}


HRESULT
COmWindowProxy::toString(BSTR* String)
{
    IMPLEMENT_SECURE_METHOD(toString(String))
    RRETURN(SetErrorInfo(hr));    
};


//+------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_defaultStatus
//
//  Synopsis:   Retrieve the default status property
//
//-------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: get_defaultStatus(BSTR *ppOut)
{
    IMPLEMENT_SECURE_PROPGET(get_defaultStatus)

Cleanup:    
    RRETURN(SetErrorInfo(hr));    
}


//+-------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_status
//
//  Synopsis:   Retrieve the status property
//
//--------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: get_status(BSTR *ppOut)
{
    IMPLEMENT_SECURE_PROPGET(get_status)

Cleanup:    
    RRETURN(SetErrorInfo(hr));    
}


//+-------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::put_defaultStatus
//
//  Synopsis:   Set the default status property
//
//--------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: put_defaultStatus(BSTR bstr)
{
    RRETURN(SetErrorInfo(_pWindow->put_defaultStatus(bstr)));
}


//+------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::put_status
//
//  Synopsis:   Set the default status property
//
//-------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: put_status(BSTR bstr)
{
    RRETURN(SetErrorInfo(_pWindow->put_status(bstr)));
}


//+------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_Image
//
//  Synopsis:   Retrieve the image element factory
//
//-------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: get_Image(IHTMLImageElementFactory **ppOut)
{
    IHTMLImageElementFactory *  pReal = NULL;

    IMPLEMENT_SECURE_OBJECTGET(get_Image)
    RRETURN(SetErrorInfo(hr));    
}


//+------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_location
//
//  Synopsis:   Retrieve the location object
//-------------------------------------------------------------------------

HRESULT 
COmWindowProxy::get_location(IHTMLLocation **ppOut)
{
    HRESULT         hr = S_OK;

    if (!_pWindow)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    if (!ppOut)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    hr = THR(_Location.QueryInterface(IID_IHTMLLocation, (void **)ppOut));
    
Cleanup:
    RRETURN(SetErrorInfo(hr));    
}


//+-------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_history
//
//  Synopsis:   Retrieve the history object
//
//--------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: get_history(IOmHistory **ppOut)
{
    IOmHistory *    pReal = NULL;

    IMPLEMENT_SECURE_OBJECTGET(get_history)
    RRETURN(SetErrorInfo(hr));    
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::close
//
//  Synopsis:   Close this windows
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: close()
{
    HRESULT hr;

    hr = THR(_pWindow->close());
#ifndef WIN16
    if (WINDOWDEAD())
    {
        hr = S_OK;
    }
#endif //!WIN16
    RRETURN(SetErrorInfo(hr));    
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::put_opener
//
//  Synopsis:   Set the opener (which window opened me) property
//
//  Notes:      This might allow the inner window to contain a
//              as it's opener a window with a bad security id.
//              However, when retrieving in get_opener, we check
//              for this and wrap it correctly.
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: put_opener(VARIANT v)
{
    RRETURN(SetErrorInfo(_pWindow->put_opener(v)));
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_opener
//
//  Synopsis:   Retrieve the opener (which window opened me) property
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: get_opener(VARIANT *pvarOut)
{
    CVariant            Real;
    HRESULT             hr;
    IHTMLWindow2 *      pWindow = NULL;

    V_VT(pvarOut) = VT_EMPTY;
    
    hr = THR(_pWindow->get_opener(&Real));
    if (hr)
        goto Cleanup;

    hr = E_FAIL;
    
    if (V_VT(&Real) == VT_UNKNOWN)
    {
        hr = THR_NOTRACE(V_UNKNOWN(&Real)->QueryInterface(
                IID_IHTMLWindow2, (void **)&pWindow));
    }
    else if (V_VT(&Real) == VT_DISPATCH)
    {
        hr = THR_NOTRACE(V_DISPATCH(&Real)->QueryInterface(
                IID_IHTMLWindow2, (void **)&pWindow));
    }

    if (hr)
    {
        //
        // Object being retrieved is not a window object.
        // Allow if the security allows it.
        //

        if (AccessAllowed())
        {
            hr = THR(VariantCopy(pvarOut, &Real));
        }
        else
        {
            hr = SetErrorInfo(E_ACCESSDENIED);
        }
    }
    else
    {
        //
        // Object is a window object, so wrap it.
        // See note above in put_opener.
        //
        
        IHTMLWindow2 *  pWindowOut;
        
        Assert(pWindow);
        hr = THR(SecureObject(pWindow, &pWindowOut));
        if (hr)
            goto Cleanup;

        V_VT(pvarOut) = VT_DISPATCH;
        V_DISPATCH(pvarOut) = pWindowOut;
    }

Cleanup:
    ReleaseInterface(pWindow);
    RRETURN(SetErrorInfo(hr));
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_navigator
//
//  Synopsis:   Get the navigator object
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: get_navigator(IOmNavigator **ppOut)
{
    IOmNavigator *  pReal = NULL;

    IMPLEMENT_SECURE_OBJECTGET(get_navigator)
    RRETURN(SetErrorInfo(hr));    
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_clientInformation
//
//  Synopsis:   Get the navigator object though the client alias
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: get_clientInformation(IOmNavigator **ppOut)
{
    IOmNavigator *  pReal = NULL;

    IMPLEMENT_SECURE_OBJECTGET(get_clientInformation)
    RRETURN(SetErrorInfo(hr));    
}

//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::put_name
//
//  Synopsis:   Set the name property
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: put_name(BSTR bstr)
{
    RRETURN(SetErrorInfo(_pWindow->put_name(bstr)));
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_name
//
//  Synopsis:   Get the name property
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: get_name(BSTR *ppOut)
{
    IMPLEMENT_SECURE_PROPGET(get_name)

Cleanup:    
    RRETURN(SetErrorInfo(hr));    
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_screenTop
//
//  Synopsis:   Get the name property
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy::get_screenTop(long *ppOut)
{
    IMPLEMENT_SECURE_PROPGET3(get_screenTop(ppOut));
    RRETURN(SetErrorInfo(hr));
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_screenLeft
//
//  Synopsis:   Get the name property
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy::get_screenLeft(long *ppOut)
{
    IMPLEMENT_SECURE_PROPGET3(get_screenLeft(ppOut));
    RRETURN(SetErrorInfo(hr));
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::get_clipboardData
//
//  Synopsis:   Per IOmWindow
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::get_clipboardData(IHTMLDataTransfer **ppOut)
{
    IMPLEMENT_SECURE_PROPGET3(get_clipboardData(ppOut));
    RRETURN(SetErrorInfo(hr));
}

//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::createModelessDialog
//
//  Synopsis:   interface method, does what the name says.
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy::showModelessDialog(BSTR strUrl, 
                                   VARIANT * pvarArgIn, 
                                   VARIANT * pvarOptions, 
                                   IHTMLWindow2 ** ppOut)
{
    HRESULT        hr = S_OK;
    IHTMLWindow3 * pWin3 = NULL;
    IHTMLWindow2 * pWindow=NULL;

    if (!ppOut)
        return E_POINTER;

    *ppOut = NULL;

    if (AccessAllowed())
    {
        hr = THR(_pWindow->QueryInterface(IID_IHTMLWindow3, (void **)&pWin3));
        if(hr)
            goto Cleanup;

        hr = THR(pWin3->showModelessDialog(strUrl, pvarArgIn, pvarOptions, &pWindow));
        if (hr)
            goto Cleanup;

        hr = THR(SecureObject(pWindow, ppOut));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pWindow);
    ReleaseInterface(pWin3);
    RRETURN(SetErrorInfo(hr));
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::attachEvent
//
//  Synopsis:   Attach the event
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy::attachEvent(BSTR event, IDispatch* pDisp, VARIANT_BOOL *pResult)
{
    IMPLEMENT_SECURE_METHOD3(attachEvent(event, pDisp, pResult));
    RRETURN(SetErrorInfo(hr));
}
        

//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::detachEvent
//
//  Synopsis:   Detach the event
//
//---------------------------------------------------------------------------

HRESULT
COmWindowProxy::detachEvent(BSTR event, IDispatch* pDisp)
{
    IMPLEMENT_SECURE_METHOD3(detachEvent(event, pDisp));
    RRETURN(SetErrorInfo(hr));
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_parent
//
//  Synopsis:   Retrieve the parent window of this window
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy::get_parent(IHTMLWindow2 **ppOut)
{
    IMPLEMENT_SECURE_WINDOWGET(get_parent)
    RRETURN(SetErrorInfo(hr));    
}

//+---------------------------------------------------------------------------
//
//  Helper:     GetFullyExpandedUrl
//
//  Synopsis:   Helper method,
//              gets the fully expanded url from the calling document. 
//----------------------------------------------------------------------------

HRESULT GetFullyExpandedUrl(CBase *pBase, BSTR bstrUrl, BSTR *pbstrFullUrl, IServiceProvider *pSP = NULL)
{
    HRESULT         hr = S_OK;
    CStr            cstrBaseURL;
    CStr            cstrSpecialURL;
    TCHAR           achBuf[pdlUrlLen];
    DWORD           cchBuf;
    TCHAR *         pchFinalUrl = NULL;
    TCHAR           achUrl[pdlUrlLen];
    ULONG           len;
    SSL_SECURITY_STATE sssCaller;

    if (!pbstrFullUrl)
        return E_INVALIDARG;

    // Get the URL of the caller
    hr = THR(GetCallerURL(cstrBaseURL, pBase, pSP));
    if (hr)
        goto Cleanup;

    hr = THR(GetCallerSecurityState(&sssCaller, pBase, pSP));
    if (hr)
        goto Cleanup;
        
    //
    // Expand the URL before we pass it on.
    //

    len = bstrUrl ? SysStringLen(bstrUrl) : 0;

    if (len)
    {
        if (len > pdlUrlLen-1)
            len = pdlUrlLen-1;

        _tcsncpy(achUrl, bstrUrl, len);
        ProcessValueEntities(achUrl, &len);
    }

    achUrl[len] = _T('\0');

    if (cstrBaseURL.Length())
    {
        hr = THR(CoInternetCombineUrl(
                cstrBaseURL, 
                achUrl,
                URL_ESCAPE_SPACES_ONLY | URL_BROWSER_MODE, 
                achBuf, 
                ARRAY_SIZE(achBuf), 
                &cchBuf, 0));
        if (hr)
            goto Cleanup;

        if (*achUrl)
        {
            hr = THR(WrapSpecialUrl(
                    achBuf, 
                    &cstrSpecialURL, 
                    cstrBaseURL,
                    sssCaller <= SSL_SECURITY_MIXED,
                    FALSE));
            if (hr)
                goto Cleanup;
        }
        else
        {
            // Construct special URL for shdocvw, the \1 for the first character
            // signals that the URL is really empty.  The url after the base is
            // the caller base url used for security.
            hr = cstrSpecialURL.Set(sssCaller <= SSL_SECURITY_MIXED ? _T("\1\1") : _T("\1"));
            if (hr)
                goto Cleanup;

            hr = cstrSpecialURL.Append(achBuf);
            if (hr)
                goto Cleanup;
        }

        pchFinalUrl = cstrSpecialURL;
    }
    else
    {
        pchFinalUrl = achUrl;
    }

    // Change the URL to the fully expanded URL.
    hr = THR(FormsAllocString(pchFinalUrl, pbstrFullUrl));

Cleanup:
    return hr;
}

// Determine if access is allowed to the named frame (by comparing the SID of the proxy with the SID
// of the direct parent of the target frame).
//
BOOL 
COmWindowProxy::AccessAllowedToNamedFrame(LPCTSTR pchTarget)
{
    HRESULT              hr;
    IUnknown           * pUnkTarget = NULL;
    ITargetFrame2      * pTargetFrame = NULL;
    IWebBrowser        * pWB = NULL;
    IDispatch          * pDispDoc = NULL;
    BOOL                 fAccessAllowed = TRUE;

    if (!pchTarget || !pchTarget[0])
        goto Cleanup;

    // Obtain a target frame pointer for the source window
    hr = THR(QueryService(IID_ITargetFrame2, IID_ITargetFrame2, (void**)&pTargetFrame));
    if (hr || !pTargetFrame)
        goto Cleanup;

    // Find the target frame using the source window as context
    hr = THR(pTargetFrame->FindFrame(pchTarget, FINDFRAME_JUSTTESTEXISTENCE, &pUnkTarget));
    if (hr || !pUnkTarget)
        goto Cleanup;

    // Perform a cross-domain check on the pUnkTarget we've received.
    //

    // QI for IWebBrowser to determine frameness.
    //
    hr = THR(pUnkTarget->QueryInterface(IID_IWebBrowser,(void**)&pWB));
    if (hr || !pWB)
        goto Cleanup;

    // Get the IDispatch of the container.  This is the containing Trident if a frameset.
    //
    hr = THR_NOTRACE(pWB->get_Container(&pDispDoc));
    if (hr || !pDispDoc)
        goto Cleanup;

    fAccessAllowed = AccessAllowed(pDispDoc);

Cleanup:
    ReleaseInterface(pWB);
    ReleaseInterface(pDispDoc);
    ReleaseInterface(pTargetFrame);
    ReleaseInterface(pUnkTarget);

    return fAccessAllowed;
}

//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::open
//
//  Synopsis:   Open a new window
//
//---------------------------------------------------------------------------

HRESULT
COmWindowProxy::open(BSTR bstrUrl,
                     BSTR bstrName,
                     BSTR bstrFeatures,
                     VARIANT_BOOL vbReplace,
                     IHTMLWindow2** ppWindow)
{
    HRESULT         hr = S_OK;
    IHTMLWindow2 *  pWindow = NULL;
    BSTR            bstrFullUrl = NULL;
    
    hr = THR(GetFullyExpandedUrl(this, bstrUrl, &bstrFullUrl));
    if (hr)
        goto Cleanup;

    // If a frame name was given
    if (!_fTrustedDoc && bstrName && bstrName[0]
        && StrCmpIC(bstrName, _T("_self")) != 0
        && StrCmpIC(bstrName, _T("_parent")) != 0
        && StrCmpIC(bstrName, _T("_blank")) != 0
        && StrCmpIC(bstrName, _T("_top")) != 0
        && StrCmpIC(bstrName, _T("_main")) != 0
        )
    {
        // Determine if access is allowed to it
        if (!AccessAllowedToNamedFrame(bstrName))
        {
            CStr cstrCallerUrl;
            DWORD dwPolicy = 0;
            DWORD dwContext = 0;

            hr = THR(GetCallerURL(cstrCallerUrl, this, NULL));
            if (hr)
                goto Cleanup;

            if (    !SUCCEEDED(ZoneCheckUrlEx(cstrCallerUrl, &dwPolicy, sizeof(dwPolicy), &dwContext, sizeof(dwContext),
                              URLACTION_HTML_SUBFRAME_NAVIGATE, 0, NULL))
                ||  GetUrlPolicyPermissions(dwPolicy) != URLPOLICY_ALLOW)
            {
                hr = E_ACCESSDENIED;
                goto Cleanup;
            }
        }
    }

    *ppWindow = NULL;
    hr = THR(_pWindow->open(
            bstrFullUrl, 
            bstrName, 
            bstrFeatures, 
            vbReplace, 
            &pWindow));
    if (hr)
    {
        if (hr == S_FALSE)  // valid return value
            hr = S_OK;
        else
            goto Cleanup;
    }
    
    if ( pWindow )
    {
        hr = THR(SecureObject(pWindow, ppWindow));
        if (hr)
        {
            AssertSz(FALSE, "Window.open proxy: returning a secure object pointer failed");
            goto Cleanup;
        }
    }
    
Cleanup:
    ReleaseInterface(pWindow);
    FormsFreeString(bstrFullUrl);
    RRETURN(SetErrorInfo(hr));    
}

//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::print
//
//  Synopsis:   Send the print cmd to this window
//
//---------------------------------------------------------------------------

HRESULT
COmWindowProxy::print()
{
    IMPLEMENT_SECURE_METHOD3(print())
    RRETURN(SetErrorInfo(hr));    
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_self
//
//  Synopsis:   Retrieve this window
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy::get_self(IHTMLWindow2**ppOut)
{
    IMPLEMENT_SECURE_WINDOWGET(get_self)
    RRETURN(SetErrorInfo(hr));    
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_top
//
//  Synopsis:   Retrieve the topmost window from this window
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: get_top(IHTMLWindow2**ppOut)
{
    IMPLEMENT_SECURE_WINDOWGET(get_top)
    RRETURN(SetErrorInfo(hr));    
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_window
//
//  Synopsis:   Retrieve this window
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: get_window(IHTMLWindow2**ppOut)
{
    IMPLEMENT_SECURE_WINDOWGET(get_window)
    RRETURN(SetErrorInfo(hr));    
}

//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::execScript
//
//  Synopsis:   object model method implementation
//
//
//-----------------------------------------------------------------------------------

HRESULT
COmWindowProxy::execScript(BSTR bstrCode, BSTR bstrLanguage, VARIANT * pvarRet)
{
    IMPLEMENT_SECURE_METHOD(execScript(bstrCode, bstrLanguage, pvarRet))
    RRETURN(SetErrorInfo(hr));    
}

//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::navigate
//
//  Synopsis:   Navigate to this url
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: navigate(BSTR url)
{
    HRESULT         hr = S_OK;
    BSTR            bstrFullUrl = NULL;
    
    hr = THR(GetFullyExpandedUrl(this, url, &bstrFullUrl));
    if (hr)
        goto Cleanup;

    hr = THR(_pWindow->navigate(bstrFullUrl));

Cleanup:
    FormsFreeString(bstrFullUrl);
    RRETURN(SetErrorInfo(hr));    
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_Option
//
//  Synopsis:   Retrieve the option element factory
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: get_Option(IHTMLOptionElementFactory **ppOut)
{
    IHTMLOptionElementFactory * pReal = NULL;

    IMPLEMENT_SECURE_OBJECTGET(get_Option)
    RRETURN(SetErrorInfo(hr));    
}

//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::focus
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------

HRESULT
COmWindowProxy::focus()
{
    HRESULT hr;
    hr = THR(_pWindow->focus());
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::blur
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------

HRESULT
COmWindowProxy::blur()
{
    HRESULT hr;
    hr = THR(_pWindow->blur());
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::focus
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------

HRESULT
COmWindowProxy::get_closed(VARIANT_BOOL *p)
{
    HRESULT hr;
    
    //
    // No security check for the closed property.
    //

    hr = THR(_pWindow->get_closed(p));
#ifndef WIN16
    if (WINDOWDEAD())
    {
        *p = VB_TRUE;
        hr = S_OK;
    }
#endif //!WIN16
    RRETURN(SetErrorInfo(hr));    
}


//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::scroll
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------

HRESULT
COmWindowProxy::scroll(long x, long y)
{
    IMPLEMENT_SECURE_METHOD(scroll(x, y))
    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::scrollBy
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------
HRESULT
COmWindowProxy::scrollBy(long x, long y)
{
    IMPLEMENT_SECURE_METHOD(scrollBy(x, y))
    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::scrollTo
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------
HRESULT
COmWindowProxy::scrollTo(long x, long y)
{
    // This method performs exactly the same action as scroll
    IMPLEMENT_SECURE_METHOD(scrollTo(x, y))
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::moveBy
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------
HRESULT
COmWindowProxy::moveBy(long x, long y)
{
    IMPLEMENT_SECURE_METHOD(moveBy(x, y))
    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::moveTo
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------
HRESULT
COmWindowProxy::moveTo(long x, long y)
{
    // This method performs exactly the same action as scroll
    IMPLEMENT_SECURE_METHOD(moveTo(x, y))
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::resizeBy
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------
HRESULT
COmWindowProxy::resizeBy(long x, long y)
{
    // This method performs exactly the same action as scroll
    IMPLEMENT_SECURE_METHOD(resizeBy(x, y))
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::resizeTo
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------
HRESULT
COmWindowProxy::resizeTo(long x, long y)
{
    // This method performs exactly the same action as scroll
    IMPLEMENT_SECURE_METHOD(resizeTo(x, y))
    RRETURN(SetErrorInfo(hr));
}



//+------------------------------------------------------------------------
//
//  Method: COmWindowProxy : IsEqualObject
//
//  Synopsis; IObjectIdentity method implementation
//
//  Returns : S_OK if objects are equal, E_FAIL otherwise
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
COmWindowProxy::IsEqualObject( IUnknown * pUnk)
{
    HRESULT             hr = E_POINTER;
    IHTMLWindow2      * pWindow = NULL;
    IHTMLWindow2      * pThisWindow = NULL;
    IServiceProvider  * pISP1 = NULL;
    IServiceProvider  * pISP2 = NULL;

    if (!pUnk)
        goto Cleanup;

    hr = THR_NOTRACE(_pWindow->QueryInterface(IID_IServiceProvider, (void**)&pISP1));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(pISP1->QueryService(SID_SHTMLWindow2, 
                                         IID_IHTMLWindow2,
                                         (void**)&pThisWindow));
    if (hr) 
        goto Cleanup;


    hr = THR_NOTRACE(pUnk->QueryInterface(IID_IServiceProvider, (void**)&pISP2));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(pISP2->QueryService(SID_SHTMLWindow2, 
                                         IID_IHTMLWindow2,
                                         (void**)&pWindow));
    if (hr)
        goto Cleanup;

    // are the objects the same
    hr = IsSameObject(pThisWindow, pWindow) ? S_OK : S_FALSE;

Cleanup:
    ReleaseInterface(pThisWindow);
    ReleaseInterface(pWindow);
    ReleaseInterface(pISP1);
    ReleaseInterface(pISP2);
    RRETURN1(hr, S_FALSE);
}



//+-------------------------------------------------------------------------
//
//  Method : COmWindowProxy
//
//  Synopsis : IServiceProvider methoid Implementaion.
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    HRESULT            hr = E_POINTER;
    IServiceProvider * pISP = NULL;

    if (!ppvObject)
        goto Cleanup;

    *ppvObject = NULL;

    hr = THR_NOTRACE(_pWindow->QueryInterface(IID_IServiceProvider, (void**)&pISP));
    if (!hr)
        hr = THR_NOTRACE(pISP->QueryService(guidService, riid, ppvObject));

    ReleaseInterface(pISP);

Cleanup:
    RRETURN1(hr, E_NOINTERFACE);
}

//+---------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::ValidateMarshalParams
//
//  Synopsis:   Validates the standard set parameters that are passed into most
//              of the IMarshal methods
//
//----------------------------------------------------------------------------

HRESULT 
COmWindowProxy::ValidateMarshalParams(
    REFIID riid,
    void *pvInterface,
    DWORD dwDestContext,
    void *pvDestContext,
    DWORD mshlflags)
{
    HRESULT hr = NOERROR;
 
    if (CanMarshalIID(riid))
    {
        if ((dwDestContext != MSHCTX_INPROC && 
             dwDestContext != MSHCTX_LOCAL && 
             dwDestContext != MSHCTX_NOSHAREDMEM) ||
            (mshlflags != MSHLFLAGS_NORMAL && 
             mshlflags != MSHLFLAGS_TABLESTRONG))
        {
            Assert(0 && "Invalid argument");
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::GetUnmarshalClass
//
//  Synopsis:
//
//----------------------------------------------------------------------------

STDMETHODIMP
COmWindowProxy::GetUnmarshalClass(
    REFIID riid,
    void *pvInterface,
    DWORD dwDestContext, 
    void *pvDestContext, 
    DWORD mshlflags, 
    CLSID *pclsid)
{
    HRESULT hr;

    hr = THR(ValidateMarshalParams(
            riid, 
            pvInterface, 
            dwDestContext,
            pvDestContext, 
            mshlflags));
    if (!hr)
    {
        *pclsid = CLSID_HTMLWindowProxy;
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::GetMarshalSizeMax
//
//  Synopsis:
//
//----------------------------------------------------------------------------

STDMETHODIMP
COmWindowProxy::GetMarshalSizeMax(
    REFIID riid,
    void *pvInterface,
    DWORD dwDestContext,
    void *pvDestContext,
    DWORD mshlflags,
    DWORD *pdwSize)
{
#ifdef WIN16
    return E_FAIL;
#else
    HRESULT hr;

    if (!pdwSize)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(ValidateMarshalParams(
            riid, 
            pvInterface, 
            dwDestContext,
            pvDestContext, 
            mshlflags));
    if (hr) 
        goto Cleanup;

    //
    // Size is the _cbSID + sizeof(DWORD) +
    // size of marshalling _pWindow
    //

    hr = THR(CoGetMarshalSizeMax(
            pdwSize,
            riid,
            _pWindow,
            dwDestContext,
            pvDestContext,
            mshlflags));
    if (hr)
        goto Cleanup;
        
    (*pdwSize) += sizeof(DWORD) + _cbSID;

Cleanup:
    RRETURN(hr);
#endif //ndef WIN16
}


//+---------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::MarshalInterface
//
//  Synopsis:   Write data of current proxy into a stream to read
//              out on the other side.
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::MarshalInterface(
    IStream *pStm,
    REFIID riid,
    void *pvInterface, 
    DWORD dwDestContext,
    void *pvDestContext, 
    DWORD mshlflags)
{
    HRESULT hr;

    hr = THR(ValidateMarshalParams(
            riid, 
            pvInterface, 
            dwDestContext,
            pvDestContext, 
            mshlflags));
    if (hr)
        goto Cleanup;

    //
    // Call the real worker
    //

    hr = THR(MarshalInterface(
            TRUE,
            pStm,
            riid,
            pvInterface,
            dwDestContext,
            pvDestContext,
            mshlflags));
    
Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::MarshalInterface
//
//  Synopsis:   Actual worker of marshalling.  Takes an additional 
//              BOOL to signify what is actually getting marshalled,
//              the window proxy or location proxy.
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::MarshalInterface(
    BOOL fWindow,
    IStream *pStm,
    REFIID riid,
    void *pvInterface, 
    DWORD dwDestContext,
    void *pvDestContext, 
    DWORD mshlflags)
{
    HRESULT hr;

    //  Marshal _pWindow
    hr = THR(CoMarshalInterface(
            pStm,
            IID_IHTMLWindow2,
            _pWindow,
            dwDestContext,
            pvDestContext,
            mshlflags));
    if (hr)
        goto Cleanup;

    //  Write _pbSID
    hr = THR(pStm->Write(&_cbSID, sizeof(DWORD), NULL));
    if (hr)
        goto Cleanup;

    hr = THR(pStm->Write(_pbSID, _cbSID, NULL));
    if (hr)
        goto Cleanup;

    hr = THR(pStm->Write(&fWindow, sizeof(BOOL), NULL));
    if (hr)
        goto Cleanup;
        
    hr = THR(pStm->Write(&_fTrustedDoc, sizeof(BOOL), NULL));
    if (hr)
        goto Cleanup;
        
Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::UnmarshalInterface
//
//  Synopsis:   Unmarshals the interface out of a stream
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::UnmarshalInterface(
    IStream *pStm,
    REFIID riid,
    void ** ppvObj)
{
    HRESULT         hr = S_OK;
    IHTMLWindow2 *  pWindow = NULL;
    IHTMLWindow2 *  pProxy = NULL;
    BOOL            fWindow = FALSE;
    BOOL            fTrustedDoc = FALSE;
    BYTE            abSID[MAX_SIZE_SECURITY_ID];
    DWORD           cbSID;
    
    if (!ppvObj)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppvObj = NULL;
    if (!CanMarshalIID(riid))
    {
        hr = E_NOINTERFACE;
        goto Cleanup;
    }

    hr = THR(CoUnmarshalInterface(pStm, IID_IHTMLWindow2, (void **)&pWindow));
    if (hr)
        goto Cleanup;

    //  Read abSID
    hr = THR(pStm->Read(&cbSID, sizeof(DWORD), NULL));
    if (hr)
        goto Cleanup;

    hr = THR(pStm->Read(abSID, cbSID, NULL));
    if (hr)
        goto Cleanup;

    hr = THR(pStm->Read(&fWindow, sizeof(BOOL), NULL));
    if (hr)
        goto Cleanup;
        
    hr = THR(pStm->Read(&fTrustedDoc, sizeof(BOOL), NULL));
    if (hr)
        goto Cleanup;
        
    //
    // Initialize myself with these values.  This is so the SecureObject
    // call below will know what to do.
    //
    
    _fTrustedDoc = fTrustedDoc;
    
    hr = THR(Init(pWindow, abSID, cbSID));
    if (hr)
        goto Cleanup;

    //
    // Finally call SecureObject to return the right proxy.  This will
    // look in thread local storage to see if a security proxy for this
    // window already exists.  If so, it'll return that one, otherwise
    // it will create a new one with cstrSID as the security
    // context.  When OLE releases this object it will just disappear.
    //

    hr = THR(SecureObject(pWindow, &pProxy));
    if (hr)
        goto Cleanup;

    if (fWindow)
    {
        hr = THR_NOTRACE(pProxy->QueryInterface(riid, ppvObj));
    }
    else
    {
        COmWindowProxy *    pCProxy;

        Verify(!pProxy->QueryInterface(CLSID_HTMLWindowProxy, (void **)&pCProxy));
        hr = THR_NOTRACE(pCProxy->_Location.QueryInterface(riid, ppvObj));
    }
    if (hr)
        goto Cleanup;
        
Cleanup:
    ReleaseInterface(pWindow);
    ReleaseInterface(pProxy);
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::ReleaseMarshalData
//
//  Synopsis:   Free up any data used while marshalling
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::ReleaseMarshalData(IStream *pStm)
{
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::DisconnectObject
//
//  Synopsis:   Unmarshals the interface out of a stream
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::DisconnectObject(DWORD dwReserved)
{
    return S_OK;
}


class CGetLocation
{
public:
    CGetLocation(COmWindowProxy *pWindowProxy)
        { _pLoc = NULL; pWindowProxy->_pWindow->get_location(&_pLoc); }
    ~CGetLocation()
        { ReleaseInterface(_pLoc); }

    IHTMLLocation * _pLoc;
};

//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::put_offscreenBuffering
//
//  Synopsis:   Set the name property
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy::put_offscreenBuffering(VARIANT var)
{
    RRETURN(SetErrorInfo(_pWindow->put_offscreenBuffering(var)));
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_offscreenBuffering
//
//  Synopsis:   Get the name property
//
//---------------------------------------------------------------------------
HRESULT 
COmWindowProxy::get_offscreenBuffering(VARIANT *ppOut)
{
    // can't use IMPLEMENT_SECURE_PROPGET(get_offscreenBuffering) because of ppOut = NULL; line
    HRESULT hr = S_OK;                              
                                                    
    hr = THR(_pWindow->get_offscreenBuffering(ppOut));                
    if (hr)                                         
        goto Cleanup;                               
    if (!AccessAllowed())                           
    {                                               
        hr = E_ACCESSDENIED;          
        goto Cleanup;
    }                                               

Cleanup:    
    RRETURN(SetErrorInfo(hr));    
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_external
//
//  Synopsis:   Get IDispatch object associated with the IDocHostUIHandler
//
//---------------------------------------------------------------------------
HRESULT 
COmWindowProxy::get_external(IDispatch **ppOut)
{
    IDispatch * pReal = NULL;

    IMPLEMENT_SECURE_OBJECTGET(get_external)
    RRETURN(SetErrorInfo(hr));    
}

#ifndef UNIX
//
// IEUNIX: Due to vtable pointer counts (I believe), the size is 8, not 4
//

StartupAssert(sizeof(void *) == sizeof(COmLocationProxy));
#endif

BEGIN_TEAROFF_TABLE_NAMED(COmLocationProxy, s_apfnLocationVTable)
    TEAROFF_METHOD(COmLocationProxy, QueryInterface, queryinterface, (REFIID riid, void **ppv))
    TEAROFF_METHOD_(COmLocationProxy, AddRef, addref, ULONG, ())
    TEAROFF_METHOD_(COmLocationProxy, Release, release, ULONG, ())
    TEAROFF_METHOD_NULL
    TEAROFF_METHOD_NULL
    TEAROFF_METHOD(COmLocationProxy, MarshalInterface, marshalinterface, (IStream *pistm,REFIID riid,void *pvInterface,DWORD dwDestContext,void *pvDestContext,DWORD mshlflags))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_(COmLocationProxy, IObjectIdentity)
    TEAROFF_METHOD(CCOmLocationProxy, IsEqualObject, isequalobject, (IUnknown*))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_(COmLocationProxy, IServiceProvider)
    TEAROFF_METHOD(COmLocationProxy, QueryService, queryservice, (REFGUID guidService, REFIID riid, void **ppvObject))
END_TEAROFF_TABLE()

//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::QueryInterface
//
//  Synopsis:   Per IUnknown
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::QueryInterface(REFIID iid, LPVOID * ppv)
{
    HRESULT         hr = S_OK;
    
    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS2(this, IUnknown, IHTMLLocation)
        QI_TEAROFF(this, IDispatchEx, NULL)
        QI_TEAROFF(this, IObjectIdentity, NULL)
        QI_TEAROFF(this, IServiceProvider, NULL)
        QI_INHERITS(this, IHTMLLocation)
        QI_INHERITS2(this, IDispatch, IHTMLLocation)
        QI_CASE(IMarshal)
            if (iid == IID_IMarshal)
            {
                IMarshal *  pMarshal = NULL;
                
                hr = THR(MyWindowProxy()->QueryInterface(
                        IID_IMarshal,
                        (void **)&pMarshal));
                if (hr)
                    RRETURN(hr);
                    
                hr = THR(CreateTearOffThunk(
                        pMarshal,
                        *(void **)pMarshal,
                        NULL,
                        ppv,
                        (IHTMLLocation *)this,
                        (void *)s_apfnLocationVTable,
                        QI_MASK + ADDREF_MASK + RELEASE_MASK + METHOD_MASK(5), NULL));
                ReleaseInterface(pMarshal);
                if (hr)
                    RRETURN(hr);
            }
            break;
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::GetTypeInfoCount
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::GetTypeInfoCount(UINT FAR* pctinfo)
{
    TraceTag((tagSecurity, "GetTypeInfoCount"));
    CGetLocation    cg(MyWindowProxy());
    
    if (!cg._pLoc)
        RRETURN(E_FAIL);
    
    RRETURN(cg._pLoc->GetTypeInfoCount(pctinfo));
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::GetTypeInfo
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo ** pptinfo)
{
    TraceTag((tagSecurity, "GetTypeInfo"));

    CGetLocation    cg(MyWindowProxy());
    
    if (!cg._pLoc)
        RRETURN(E_FAIL);
    

    RRETURN(cg._pLoc->GetTypeInfo(itinfo, lcid, pptinfo));
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::GetIDsOfNames
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::GetIDsOfNames(
    REFIID                riid,
    LPOLESTR *            rgszNames,
    UINT                  cNames,
    LCID                  lcid,
    DISPID FAR*           rgdispid)
{
    CGetLocation    cg(MyWindowProxy());
    
    if (!cg._pLoc)
        RRETURN(DISP_E_UNKNOWNNAME);
    
    RRETURN(cg._pLoc->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid));
}

HRESULT
COmLocationProxy::Invoke(DISPID dispid,
                         REFIID riid,
                         LCID lcid,
                         WORD wFlags,
                         DISPPARAMS *pdispparams,
                         VARIANT *pvarResult,
                         EXCEPINFO *pexcepinfo,
                         UINT *puArgErr)
{
    return InvokeEx(dispid,
                    lcid,
                    wFlags,
                    pdispparams,
                    pvarResult,
                    pexcepinfo,
                    NULL);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::Invoke
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::InvokeEx(DISPID dispidMember,
                           LCID lcid,
                           WORD wFlags,
                           DISPPARAMS *pdispparams,
                           VARIANT *pvarResult,
                           EXCEPINFO *pexcepinfo,
                           IServiceProvider *pSrvProvider)
{
    TraceTag((tagSecurity, "Invoke dispid=0x%x", dispidMember));

    HRESULT         hr;
    CGetLocation    cg(MyWindowProxy());
    BSTR            bstrOld = NULL, bstrNew = NULL;
    CStr            cstrSpecialURL;

    if (!cg._pLoc)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // If Access is allowed or if the invoke is for doing
    // a put on the href property, act as a pass through.
    //

    if (MyWindowProxy()->AccessAllowed() ||
        (!dispidMember && 
            wFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF)) ||
        (dispidMember == DISPID_COmLocation_replace))
    {
        //
        // If this is a put_href, just expand the url correctly with
        // the base url of the caller.
        //


        if ( (!dispidMember || 
              dispidMember == DISPID_COmLocation_replace ||
              dispidMember == DISPID_COmLocation_assign ||
              dispidMember == DISPID_COmLocation_reload) &&
            pdispparams->cArgs==1 && 
            pdispparams->rgvarg[0].vt == VT_BSTR && 
            pSrvProvider )
        {
            bstrOld = V_BSTR(&pdispparams->rgvarg[0]);

            if (bstrOld)
            {
                hr = THR(GetFullyExpandedUrl(NULL, bstrOld, &bstrNew, pSrvProvider));
                if (hr)
                    goto Cleanup;

                V_BSTR(&pdispparams->rgvarg[0]) = bstrNew;
            }
        }

        hr = THR(cg._pLoc->Invoke(
                    dispidMember,
                    IID_NULL,
                    lcid,
                    wFlags,
                    pdispparams,
                    pvarResult,
                    pexcepinfo,
                    NULL));
        if (hr)
            hr = MyWindowProxy()->SetErrorInfo(hr);
    }
    else
    {
        //
        // Deny access otherwise.  
        // CONSIDER: Is this too stringent?
        //

        hr = MyWindowProxy()->SetErrorInfo(E_ACCESSDENIED);
    }
    
Cleanup:
    if (bstrOld || bstrNew)
    {
        // replace the original arg - supposed top be [in] parameter only
        V_BSTR(&pdispparams->rgvarg[0]) = bstrOld;
    }
    FormsFreeString(bstrNew);
    RRETURN_NOTRACE(hr);
}


HRESULT
COmLocationProxy::GetMemberName (DISPID id, BSTR *pbstrName)
{
    HRESULT         hr;
    IDispatchEx *  pDispEx2 = NULL;
    CGetLocation    cg(MyWindowProxy());
    
    if (!cg._pLoc)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    hr = THR(cg._pLoc->QueryInterface(IID_IDispatchEx, (void **)&pDispEx2));
    if (hr)
        goto Cleanup;
        
    hr = THR_NOTRACE(pDispEx2->GetMemberName(id, pbstrName));
            
Cleanup:
    ReleaseInterface(pDispEx2);
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::GetDispID
//
//  Synopsis:   Per IDispatchEx
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT         hr;
    IDispatchEx *  pDispEx2 = NULL;
    CGetLocation    cg(MyWindowProxy());
    
    if (!cg._pLoc)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    hr = THR(cg._pLoc->QueryInterface(IID_IDispatchEx, (void **)&pDispEx2));
    if (hr)
        goto Cleanup;
        
    hr = THR_NOTRACE(pDispEx2->GetDispID(bstrName, grfdex, pid));
            
Cleanup:
    ReleaseInterface(pDispEx2);
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBase::DeleteMember, IDispatchEx
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::DeleteMemberByName(BSTR bstr,DWORD grfdex)
{
    return E_NOTIMPL;
}

HRESULT
COmLocationProxy::DeleteMemberByDispID(DISPID id)
{
    return E_NOTIMPL;
}

//+-------------------------------------------------------------------------
//
//  Method:     CBase::GetMemberProperties, IDispatchEx
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::GetMemberProperties(
                DISPID id,
                DWORD grfdexFetch,
                DWORD *pgrfdex)
{
    return E_NOTIMPL;
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::GetNextDispID
//
//  Synopsis:   Per IDispatchEx
//
//--------------------------------------------------------------------------
HRESULT
COmLocationProxy::GetNextDispID(DWORD grfdex,
                     DISPID id,
                     DISPID *prgid)
{
    HRESULT         hr;
    IDispatchEx *  pDispEx2 = NULL;
    CGetLocation    cg(MyWindowProxy());
    
    if (!cg._pLoc)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    // Cache the IDispatchEx ptr?
    hr = THR(cg._pLoc->QueryInterface(IID_IDispatchEx, (void **)&pDispEx2));
    if (hr)
        goto Cleanup;
        
    hr = THR(pDispEx2->GetNextDispID(grfdex, id, prgid));

Cleanup:
    ReleaseInterface(pDispEx2);
    RRETURN1(hr,S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::GetNameSpaceParent
//
//  Synopsis:   Per IDispatchEx
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::GetNameSpaceParent(IUnknown **ppunk)
{
    HRESULT         hr;
    IDispatchEx *  pDEX = NULL;
    CGetLocation    cg(MyWindowProxy());
    
    if (!cg._pLoc)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (!ppunk)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppunk = NULL;

    Verify (!THR(cg._pLoc->QueryInterface(IID_IDispatchEx, (void**) &pDEX)));

    hr = THR(pDEX->GetNameSpaceParent(ppunk));

Cleanup:
    ReleaseInterface (pDEX);
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::MarshalInterface
//
//  Synopsis:   Write data of current proxy into a stream to read
//              out on the other side.
//
//----------------------------------------------------------------------------

HRESULT
COmLocationProxy::MarshalInterface(
    IStream *pStm,
    REFIID riid,
    void *pvInterface, 
    DWORD dwDestContext,
    void *pvDestContext, 
    DWORD mshlflags)
{
    HRESULT hr;

    hr = THR(MyWindowProxy()->ValidateMarshalParams(
            riid, 
            pvInterface, 
            dwDestContext,
            pvDestContext, 
            mshlflags));
    if (hr)
        goto Cleanup;

    //
    // Call the real worker
    //

    hr = THR(MyWindowProxy()->MarshalInterface(
            FALSE,
            pStm,
            riid,
            pvInterface,
            dwDestContext,
            pvDestContext,
            mshlflags));
    
Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Method: COmLocationProxy : IsEqualObject
//
//  Synopsis; IObjectIdentity method implementation
//
//  Returns : S_OK if objects are equal, E_FAIL otherwise
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
COmLocationProxy::IsEqualObject( IUnknown * pUnk)
{
    HRESULT          hr = E_POINTER;
    IServiceProvider *pISP = NULL;
    IHTMLLocation    *pShdcvwLoc = NULL;
    CGetLocation     cg(MyWindowProxy());

    if (!pUnk || !cg._pLoc)
        goto Cleanup;

    hr = THR_NOTRACE(pUnk->QueryInterface(IID_IServiceProvider, (void**)&pISP));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(pISP->QueryService(SID_SOmLocation, 
                                        IID_IHTMLLocation,
                                        (void**)&pShdcvwLoc));
    if (hr)
        goto Cleanup;

    // are the unknowns equal?
    hr = (IsSameObject(pShdcvwLoc, cg._pLoc)) ? S_OK : S_FALSE;

Cleanup:
    ReleaseInterface(pShdcvwLoc);
    ReleaseInterface(pISP);
    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method : COmLocationProxy
//
//  Synopsis : IServiceProvider methoid Implementaion.
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    HRESULT hr = E_POINTER;

    if (!ppvObject)
        goto Cleanup;

    *ppvObject = NULL;

    if ( guidService == SID_SOmLocation)
    {
        CGetLocation    cg(MyWindowProxy());

        if (!cg._pLoc)
        {
            hr = E_FAIL;
        }
        else
        {
            hr = THR_NOTRACE(cg._pLoc->QueryInterface(riid, ppvObject));
        }
    }
    else
        hr = E_NOINTERFACE;

Cleanup:
    RRETURN1(hr, E_NOINTERFACE);
}

//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::put_href
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::put_href(BSTR bstr)
{
    CGetLocation    cg(MyWindowProxy());
    HRESULT         hr;
    
    if (!cg._pLoc)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    // Expand the URL using the base HREF of the caller

    //
    // Always allow puts on the href to go thru for netscape compat.
    //
    
    hr = THR(cg._pLoc->put_href(bstr));
    
Cleanup:
    RRETURN(MyWindowProxy()->SetErrorInfo(hr));
}


#define IMPLEMENT_SECURE_LOCATION_METH(meth)        \
    HRESULT         hr = S_OK;                      \
    CGetLocation    cg(MyWindowProxy());            \
                                                    \
    if (!cg._pLoc)                                  \
    {                                               \
        hr = E_FAIL;                                \
        goto Cleanup;                               \
    }                                               \
    hr = MyWindowProxy()->AccessAllowed() ?         \
            cg._pLoc->meth(bstr) :                  \
            E_ACCESSDENIED;                         \
Cleanup:                                            \
    return MyWindowProxy()->SetErrorInfo(hr);       \


  
//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::get_href
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::get_href(BSTR *bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(get_href)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::put_protocol
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::put_protocol(BSTR bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(put_protocol)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::get_protocol
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::get_protocol(BSTR *bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(get_protocol)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::put_host
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::put_host(BSTR bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(put_host)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::get_host
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::get_host(BSTR *bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(get_host)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::put_hostname
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::put_hostname(BSTR bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(put_hostname)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::get_hostname
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::get_hostname(BSTR *bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(get_hostname)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::put_port
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::put_port(BSTR bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(put_port)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::get_port
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::get_port(BSTR *bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(get_port)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::get_pathname
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::get_pathname(BSTR *bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(get_pathname)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::put_pathname
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::put_pathname(BSTR bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(put_pathname)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::get_search
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::get_search(BSTR *bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(get_search)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::put_search
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::put_search(BSTR bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(put_search)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::put_hash
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::put_hash(BSTR bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(put_hash)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::get_hash
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::get_hash(BSTR *bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(get_hash)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::reload
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::reload(VARIANT_BOOL flag)
{
    CGetLocation    cg(MyWindowProxy());
    HRESULT         hr = S_OK;
    
    if (!cg._pLoc)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = MyWindowProxy()->AccessAllowed() ? 
            cg._pLoc->reload(flag) : E_ACCESSDENIED;

Cleanup:
    RRETURN(MyWindowProxy()->SetErrorInfo(hr));
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::replace
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::replace(BSTR bstr)
{
    CGetLocation    cg(MyWindowProxy());
    HRESULT         hr = S_OK;
    
    if (!cg._pLoc)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(cg._pLoc->replace(bstr));

Cleanup:
    RRETURN(MyWindowProxy()->SetErrorInfo(hr));
}

//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::assign
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::assign(BSTR bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(assign)
}

//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::toString
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::toString(BSTR * bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(toString)
}



