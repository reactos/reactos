//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       hifiTime.cxx
//
//  History:    02-Mar-1998     terrylu     Created
//
//  Contents:   HiFiTimer implementation
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_HANDIMPL_HXX_
#define X_HANDIMPL_HXX_
#include "handimpl.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_ACTIVSCP_H_
#define X_ACTIVSCP_H_
#include <activscp.h>
#endif

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include <dispex.h>
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include "mshtmhst.h"       // for IClassFactory3
#endif

#ifndef X_SCROID_H_
#define X_SCROID_H_
#include "scroid.h"         // For SID_ScriptletDispatch
#endif

#ifndef X_HANDLER_H_
#define X_HANDLER_H_
#include "handler.h"
#endif

#ifndef X_MISC_HXX_
#define X_MISC_HXX_
#ifdef _MAC
#include "misc2.hxx"
#else
#include "misc.hxx"
#endif
#endif

#ifndef X_HIFITIME_HXX_
#define X_HIFITIME_HXX_
#include "hifitime.hxx"
#endif

MtDefine(CHiFiUses, Scriptlet, "CHiFiUses")
MtDefine(CHiFiSink, Scriptlet, "CHiFiSink")
MtDefine(CHiFiList__aryHiFi_pv, Scriptlet, "CHiFiDispatch::_aryHiFi::_pv")


///////////////////////////////////////////////////////////////////////////
//
// tearoff tables
//
///////////////////////////////////////////////////////////////////////////

BEGIN_TEAROFF_TABLE(CHiFiUses, IScriptletHandler)
	TEAROFF_METHOD(CHiFiUses, GetNameSpaceObject, getnamespaceobject, (IUnknown **ppunk))
	TEAROFF_METHOD(CHiFiUses, SetScriptNameSpace, setscriptnamespace, (IUnknown *punkNameSpace))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CHiFiUses, IScriptletHandlerConstructor)
    TEAROFF_METHOD(CHiFiUses, Load, load, (WORD wFlags, IScriptletXML *pxmlElement))
	TEAROFF_METHOD(CHiFiUses, Create, create, (IUnknown *punkContext, IUnknown *punkOuter, IUnknown **ppunkHandler))
	TEAROFF_METHOD(CHiFiUses, Register, register, (LPCOLESTR pstrPath))
	TEAROFF_METHOD(CHiFiUses, Unregister, unregister, ())
	TEAROFF_METHOD(CHiFiUses, AddInterfaceTypeInfo, addinterfacetypeinfo, (ICreateTypeLib *ptclib, ICreateTypeInfo *pctiCoclass))
END_TEAROFF_TABLE()


///////////////////////////////////////////////////////////////////////////
//
// misc
//
///////////////////////////////////////////////////////////////////////////

const CBase::CLASSDESC CHiFiUses::s_classdesc =
{
    &CLSID_CHiFiUses,               // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    NULL,                           // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

//+------------------------------------------------------------------------
//
//  Function:   CreatePeerHandler
//
//  Synopsis:   Creates a new instance of scriptoid peer handler
//
//-------------------------------------------------------------------------

HRESULT
CreateHiFiUses(IUnknown *pUnkContext, IUnknown * pUnkOuter, IUnknown ** ppUnk)
{
    HRESULT         hr;
    CHiFiUses      *pHiFi;

    *ppUnk = NULL;

    pHiFi = new CHiFiUses(pUnkContext, pUnkOuter);
    if (!pHiFi)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = pHiFi->Init();
    if (hr)
        goto Cleanup;

    (*ppUnk) = (IUnknown*)pHiFi;

Cleanup:    
    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CHiFiUses constructor
//
//-------------------------------------------------------------------------

CHiFiUses::CHiFiUses(IUnknown *pUnkContext, IUnknown * pUnkOuter) :
  _pContext(pUnkContext),
  _pUnkOuter(pUnkOuter),
  _pHiFiDisp(NULL)
{
}

//+------------------------------------------------------------------------
//
//  Member:     CHiFiUses destructor
//
//-------------------------------------------------------------------------

CHiFiUses::~CHiFiUses()
{
}

//+------------------------------------------------------------------------
//
//  Member:     CHiFiUses::Init
//
//-------------------------------------------------------------------------

HRESULT
CHiFiUses::Init()
{
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CHiFiUses::Passivate
//
//-------------------------------------------------------------------------

void CHiFiUses::Passivate()
{
    if (_pHiFiDisp)
    {
        _pHiFiDisp->Disconnect();
        _pHiFiDisp = NULL;
    }

    super::Passivate();
}

//+------------------------------------------------------------------------
//
//  Member:     CHiFiUses::PrivateQueryInterface
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHiFiUses::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS((IPrivateUnknown *)this, IUnknown)
    QI_TEAROFF(this, IScriptletHandler, _pUnkOuter)
    QI_TEAROFF(this, IScriptletHandlerConstructor, _pUnkOuter)
    default:
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}


HRESULT
CHiFiUses::GetHiFiTimer(IUnknown **ppHiFiTimerServices)
{
    HRESULT     hr = S_OK;

    *ppHiFiTimerServices = NULL;

    if (_pContext)
    {
// BUGBUG: ***TLL***
//        hr = _pContext->QueryInterface(IID_IHiFiTimer, (void **)ppHiFiTimerServices);
_pContext->AddRef();    // Bugbug until above line is implemented, then remove this line.
    }

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHiFiUses::GetNameSpaceObject, per IScriptletHandler
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHiFiUses::GetNameSpaceObject(IUnknown **ppUnk)
{
    if (!ppUnk)
        return E_POINTER;

    if (!_pHiFiDisp)
    {
        // Keep weak-ref in _pPeerDisp, object life time is controlled by caller.
        _pHiFiDisp = new CHiFiDispatch(this);
        if (!_pHiFiDisp)
            return E_OUTOFMEMORY;
    }
    else
    {
        _pHiFiDisp->AddRef();
    }

    *ppUnk = (IUnknown *)_pHiFiDisp;
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CHiFiUses::SetScriptNameSpace, per IScriptletHandler
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHiFiUses::SetScriptNameSpace(IUnknown *punkNameSpace)
{
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CHiFiUses::Create, per IScriptletHandlerConstructor
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHiFiUses::Create(IUnknown *punkContext, IUnknown *punkOuter, IUnknown **ppunkHandler)
{
    HRESULT         hr;
    CHiFiUses      *pHiFi;

    pHiFi = new CHiFiUses(punkContext, punkOuter);
    if (!pHiFi)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = pHiFi->Init();
    if (hr)
        goto Cleanup;

    (*ppunkHandler) = (IUnknown*)pHiFi;

Cleanup:
    if (hr)
        delete pHiFi;

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHiFiDispatch constructor
//
//-------------------------------------------------------------------------

CHiFiDispatch::CHiFiDispatch(CHiFiUses *pHiFiUses)
{
    _ulRefs = 1;
    _pHiFiUses = pHiFiUses;
    _pTimer = 0;
}

//+------------------------------------------------------------------------
//
//  Member:     CHiFiDispatch:: destructor
//
//-------------------------------------------------------------------------

CHiFiDispatch::~CHiFiDispatch()
{
    for (int i = _aryHiFi.Size() - 1; i >= 0; i--)
    {
        delete _aryHiFi[i];
        _aryHiFi.Delete(i);
    }

    ReleaseInterface(_pTimer);
}

//+------------------------------------------------------------------------
//
//  Member:     CHiFiDispatch::Disconnect, helper
//
//-------------------------------------------------------------------------

// Sever connection with handler this breaks circularity.
void
CHiFiDispatch::Disconnect()
{
    _pHiFiUses = NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     CHiFiDispatch::QueryInterface, per IUnknown
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHiFiDispatch::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;

    if ( riid==IID_IUnknown || riid==IID_IDispatch )
    {
        *ppv = (IDispatch*)this;
        AddRef();
        return S_OK;
    }
    else if ( riid==IID_IDispatchEx )
    {
        *ppv = (IDispatchEx*)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//+------------------------------------------------------------------------
//
//  Member:     CHiFiDispatch::AddRef, per IUnknown
//
//-------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CHiFiDispatch::AddRef()
{
    return ++_ulRefs;
}

//+------------------------------------------------------------------------
//
//  Member:     CHiFiDispatch::Release, per IUnknown
//
//-------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CHiFiDispatch::Release() 
{
    if (--_ulRefs == 0)
    {
        delete this;
        return 0;
    }
    
    return _ulRefs;
}

//+------------------------------------------------------------------------
//
//  Member:     CHiFiDispatch::GetTypeInfoCount, per IDispatch
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHiFiDispatch::GetTypeInfoCount(UINT * /* pcTypeInfo */)
{
    return E_NOTIMPL;
}

//+------------------------------------------------------------------------
//
//  Member:     CHiFiDispatch::GetTypeInfo, per IDispatch
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHiFiDispatch::GetTypeInfo(
    UINT            /* uTypeInfo */, 
    LCID            /* lcid */,
    ITypeInfo **    /* ppTypeInfo */)
{
    return E_NOTIMPL;
}
    

//+------------------------------------------------------------------------
//
//  Member:     CPeerDispatch::GetIDsOfNames, per IDispatch
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHiFiDispatch::GetIDsOfNames(
    REFIID      /*riid*/,
    LPOLESTR *  rgszNames,
    UINT        cNames,
    LCID        /*lcid*/,
    DISPID *    pdispid)
{
    HRESULT hr;
    BSTR    bstrName;
    
    if (1 != cNames)
        return E_UNEXPECTED;

    if (!rgszNames || !pdispid)
        return E_POINTER;

    bstrName = SysAllocString(rgszNames[0]);
    if (bstrName)
    {
        hr = GetDispID(bstrName, 0, pdispid);
        SysFreeString(bstrName);
    }
    else 
    {
        hr = E_OUTOFMEMORY;
    }
    
    return hr;
}
    
//+------------------------------------------------------------------------
//
//  Member:     CPeerDispatch::Invoke, per IDispatch
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHiFiDispatch::Invoke(
    DISPID          dispid,
    REFIID          /*riid*/,
    LCID            lcid,
    WORD            wFlags,
    DISPPARAMS *    pDispParams,
    VARIANT *       pvarResult,
    EXCEPINFO *     pExcepInfo,
    UINT *          /* puArgErr */)
{
    return InvokeEx(dispid, lcid, wFlags, pDispParams, pvarResult, pExcepInfo, NULL);
}

//+------------------------------------------------------------------------
//
//  Member:     CHiFiDispatch::GetDispID, helper
//
//-------------------------------------------------------------------------

HRESULT
CHiFiDispatch::GetDispID(
    BSTR        bstrName,
    DWORD       grfdex,
    DISPID *    pdispid)
{
    STRINGCOMPAREFN pfnStrCmp = (grfdex & fdexNameCaseSensitive) ? StrCmpC : StrCmpIC;

    LPTSTR *    ppchName;

    if (!pdispid)
        return E_POINTER;

    for (ppchName = GetNamesTable(); *ppchName; ppchName++)
    {
        if (0 == pfnStrCmp (*ppchName, bstrName))
        {
            (*pdispid) = ppchName - GetNamesTable();
            return S_OK;
        }
    }

    *pdispid = DISPID_UNKNOWN;
    return DISP_E_UNKNOWNNAME;
}

//+------------------------------------------------------------------------
//
//  Member:     CHiFiDispatch::Invoke, helper
//
//-------------------------------------------------------------------------

HRESULT
CHiFiDispatch::InvokeEx(
    DISPID              dispid,
    LCID                lcid,
    WORD                wFlags,
    DISPPARAMS *        pDispParams,
    VARIANT *           pvarRes,
    EXCEPINFO *         pExcepInfo,
    IServiceProvider *  pServiceCaller)
{
    HRESULT         hr = S_OK;
    IUnknown       *pHiFiTimerServices = NULL;

    if (!_pHiFiUses)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    switch (dispid)
    {
    case DISPID_ISAVAILABLE:

        //
        // Is the interface(s) all available from the host?
        //

        if (!(wFlags & DISPATCH_PROPERTYGET))
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        if (pDispParams->cArgs) // no arguments expected
        {
            hr = DISP_E_BADPARAMCOUNT;
            goto Cleanup;
        }

        if (!pvarRes)
        {
            hr = E_POINTER;
            goto Cleanup;
        }

        V_VT(pvarRes) = VT_BOOL;
        hr = _pHiFiUses->GetHiFiTimer(&pHiFiTimerServices);
        V_BOOL(pvarRes) = pHiFiTimerServices ? VB_TRUE : VB_FALSE;

        ReleaseInterface(pHiFiTimerServices);

        break;

    case DISPID_INTERVAL:
        VARIANT     varInterval;

        VariantInit(&varInterval);

        if (!(wFlags & DISPATCH_METHOD))
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        if (2 == pDispParams->cArgs)
        {
            hr = VariantChangeTypeEx(&varInterval, &pDispParams->rgvarg[0], 0, VARIANT_NOVALUEPROP, VT_UI4);
            if (hr || VT_DISPATCH != V_VT(&pDispParams->rgvarg[1]))
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }
        }
        else
        {
            hr = DISP_E_BADPARAMCOUNT;
            goto Cleanup;
        }

        V_VT(pvarRes) = VT_UI4;
        SetHiFiTimer(V_UI4(&varInterval), V_DISPATCH(&pDispParams->rgvarg[1]), &V_UI4(pvarRes));
        break;

    case DISPID_DELETE:
        VARIANT     varCookie;

        VariantInit(&varCookie);

        if (!(wFlags & DISPATCH_METHOD))
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        if (1 == pDispParams->cArgs)
        {
            hr = VariantChangeTypeEx(&varCookie, &pDispParams->rgvarg[0], 0, VARIANT_NOVALUEPROP, VT_UI4);
            if (hr)
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }
        }

        DeleteHiFi(V_UI4(&varCookie));
        break;

    case DISPID_TIME:
        if (!(wFlags & DISPATCH_PROPERTYGET))
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        if (pDispParams->cArgs) // no arguments expected
        {
            hr = DISP_E_BADPARAMCOUNT;
            goto Cleanup;
        }

        if (!pvarRes)
        {
            hr = E_POINTER;
            goto Cleanup;
        }

        hr = GetHiFiCurrentTime(pvarRes);

        break;

    default:
        hr = DISP_E_MEMBERNOTFOUND;
        break;
    }

Cleanup:
    return hr;
}


//+------------------------------------------------------------------------
//
//  Member:     CHiFiDispatch::DeleteMemberByName, per IDispatchEx
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHiFiDispatch::DeleteMemberByName(BSTR /*bstr*/, DWORD /*grfdex*/)
{
    return S_OK;
}
        
//+------------------------------------------------------------------------
//
//  Member:     CHiFiDispatch::DeleteMemberByDispID, per IDispatchEx
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHiFiDispatch::DeleteMemberByDispID(DISPID /*id*/)
{
    return S_OK;
}
        
//+------------------------------------------------------------------------
//
//  Member:     CHiFiDispatch::GetMemberProperties, per IDispatchEx
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHiFiDispatch::GetMemberProperties(DISPID /*dispid*/, DWORD /*grfdexFetch*/, DWORD *pgrfdex)
{
    if ( !pgrfdex )
        return E_POINTER;

    *pgrfdex = 0;

/*
BUGBUG: Need to handle the following.

fdexPropCanGet
fdexPropCannotGet
fdexPropCanPut
fdexPropCannotPut
fdexPropCanPutRef
fdexPropCannotPutRef
fdexPropNoSideEffects
fdexPropDynamicType
fdexPropCanCall
fdexPropCannotCall
fdexPropCanConstruct
fdexPropCannotConstruct
fdexPropCanSourceEvents
fdexPropCannotSourceEvents
*/

    return DISP_E_MEMBERNOTFOUND;
}
        
//+------------------------------------------------------------------------
//
//  Member:     CHiFiDispatch::GetMemberName, per IDispatchEx
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHiFiDispatch::GetMemberName(DISPID dispid, BSTR * pbstrName)
{
    if (!pbstrName)
        return E_POINTER;
        
    if (dispid < 0 || DISPID_COUNT <= dispid)
        return DISP_E_MEMBERNOTFOUND;

    *pbstrName = SysAllocString(GetNamesTable()[dispid]);
    return (*pbstrName) ? S_OK : E_OUTOFMEMORY;
}
        
//+------------------------------------------------------------------------
//
//  Member:     CHiFiDispatch::GetNextDispID, per IDispatchEx
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHiFiDispatch::GetNextDispID(DWORD /*grfdex*/, DISPID /*dispid*/, DISPID * pdispid)
{
    if (!pdispid)
        return E_POINTER;

    // BUGBUG implement
    return S_FALSE;
}
        
//+------------------------------------------------------------------------
//
//  Member:     CHiFiDispatch::GetNameSpaceParent, per IDispatchEx
//
//-------------------------------------------------------------------------

STDMETHODIMP
CHiFiDispatch::GetNameSpaceParent(IUnknown ** ppUnk)
{
    *ppUnk = NULL;
    return S_OK;
}


HRESULT
CHiFiDispatch::SetHiFiTimer(unsigned int interval, IDispatch *pFuncPtr, DWORD *pdwCookie)
{
    HRESULT             hr;
    ITimerService      *pTM = NULL;
    ITimer             *pDrawTimer = NULL;
    IServiceProvider   *pServiceProvider = NULL;
    CVariant            vtimeMin(VT_UI4), vtimeMax(VT_UI4), vtimeInt(VT_UI4);
    HIFIINFO           *pHiFiInfo;

    if (!_pHiFiUses || !_pHiFiUses->_pContext)
    {
        hr = E_FAIL;
        goto cleanup;
    }

    if (!_pTimer)
    {
        hr = _pHiFiUses->_pContext->QueryInterface(IID_IServiceProvider, 
                                                   (void**)&pServiceProvider);
        if (hr)
            goto cleanup;

        hr = pServiceProvider->QueryService(SID_STimerService, IID_ITimerService, (void **)&pTM);
        if (hr)
            goto cleanup;

        hr = pTM->GetNamedTimer(NAMEDTIMER_DRAW, &pDrawTimer);
        if (hr)
            goto cleanup;

        hr = pTM->CreateTimer(pDrawTimer, &_pTimer);
        if (hr)
            goto cleanup;
    }

    _pTimer->GetTime(&vtimeMin);
    V_UI4(&vtimeMax) = 0;
    V_UI4(&vtimeInt) = interval;

    pHiFiInfo = new HIFIINFO(this);
    if (!pHiFiInfo)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    pHiFiInfo->_pSink = new CHiFiSink(pFuncPtr);
    if (!pHiFiInfo->_pSink)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup1;
    }

    hr = _aryHiFi.Append(pHiFiInfo);
    if (hr)
        goto cleanup1;

    hr = _pTimer->Advise(vtimeMin, vtimeMax,
                         vtimeInt, 0,
                         (ITimerSink *)pHiFiInfo->_pSink,
                         &(pHiFiInfo->_dwCookie));
    if (hr)
        goto cleanup1;

    *pdwCookie = pHiFiInfo->_dwCookie;

    hr = S_OK;

cleanup:
    ReleaseInterface(pServiceProvider);
    ReleaseInterface(pDrawTimer);
    ReleaseInterface(pTM);

    RRETURN(hr);

cleanup1:
    delete pHiFiInfo;
    goto cleanup;
}

void
CHiFiDispatch::DeleteHiFi(DWORD dwCookie)
{
    int     i;

    for (i = _aryHiFi.Size() - 1; i >= 0  ; i--)
    {
        if(_aryHiFi[i]->_dwCookie == dwCookie && _aryHiFi[i]->_pSink)
        {
            delete _aryHiFi[i];
            _aryHiFi.Delete(i);
            break;
        }
    }
}

HRESULT
CHiFiDispatch::GetHiFiCurrentTime(VARIANT *pVar)
{
    HRESULT             hr = S_OK;
    ITimerService      *pTM = NULL;
    ITimer             *pDrawTimer = NULL;
    IServiceProvider   *pServiceProvider = NULL;

    if (!_pHiFiUses || !_pHiFiUses->_pContext)
    {
        hr = E_FAIL;
        goto cleanup;
    }

    if (!_pTimer)
    {
        hr = _pHiFiUses->_pContext->QueryInterface(IID_IServiceProvider, 
                                                   (void**)&pServiceProvider);
        if (hr)
            goto cleanup;

        hr = pServiceProvider->QueryService(SID_STimerService, IID_ITimerService, (void **)&pTM);
        if (hr)
            goto cleanup;

        hr = pTM->GetNamedTimer(NAMEDTIMER_DRAW, &pDrawTimer);
        if (hr)
            goto cleanup;
    }

    if (pVar)
    {
        hr = _pTimer ? _pTimer->GetTime(pVar) : pDrawTimer->GetTime(pVar);
    }
    else
    {
        V_VT(pVar) = VT_UI4;
        V_UI4(pVar) = 0;
    }

cleanup:
    ReleaseInterface(pDrawTimer);
    ReleaseInterface(pTM);
    ReleaseInterface(pServiceProvider);

    RRETURN(hr);
}


CHiFiDispatch::HIFIINFO::~HIFIINFO()
{
    if (_pSink)
    {
        if (_dwCookie)
            _pHiFiDispatch->getTimer()->Unadvise(_dwCookie);

        _pSink->Release();
    }
}


CHiFiSink::CHiFiSink(IDispatch *pFuncPtr)
{
    _pFuncPtr = pFuncPtr;
    _pFuncPtr->AddRef();

    _ulRefs = 1;
}


CHiFiSink::~CHiFiSink()
{
    ReleaseInterface(_pFuncPtr);
}


ULONG
CHiFiSink::AddRef()
{
    return ++_ulRefs;
}

ULONG
CHiFiSink::Release()
{
    if ( 0 == --_ulRefs )
    {
        delete this;
        return 0;
    }
    return _ulRefs;
}

//+-------------------------------------------------------------------------
//
//  Member:     QueryInterface
//
//  Synopsis:   IUnknown implementation.
//
//  Arguments:  the usual
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------
HRESULT
CHiFiSink::QueryInterface(REFIID iid, void **ppv)
{
    if ( !ppv )
        RRETURN(E_POINTER);

    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((ITimerSink *)this, IUnknown)
        QI_INHERITS(this, ITimerSink)
        default:
            break;
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    RRETURN(E_NOINTERFACE);
}

//+----------------------------------------------------------------------------
//
//  Method:     OnTimer             [ITimerSink]
//
//  Synopsis:   Takes the accumulated region and invalidates the window
//
//  Arguments:  timeAdvie - the time that the advise was set.
//
//  Returns:    S_OK
//
//-----------------------------------------------------------------------------
HRESULT
CHiFiSink::OnTimer (VARIANT)
{
    HRESULT         hr = S_OK;

    if (_pFuncPtr)
    {
        DISPPARAMS      dp = g_Zero.dispparams;
        IDispatchEx    *pDEX = NULL;

        hr = THR_NOTRACE(_pFuncPtr->QueryInterface(IID_IDispatchEx, (void **)&pDEX));

        //
        // Call the function
        //
        hr = pDEX ? THR(pDEX->InvokeEx(DISPID_VALUE,
                                     0,
                                     DISPATCH_METHOD,
                                     &dp,
                                     NULL,
                                     NULL,
                                     NULL))         :
                    THR(_pFuncPtr->Invoke(DISPID_VALUE,
                                      IID_NULL,
                                      0,
                                      DISPATCH_METHOD,
                                      &dp,
                                      NULL,
                                      NULL,
                                      NULL));

        ReleaseInterface(pDEX);
    }

    RRETURN(hr);
}
