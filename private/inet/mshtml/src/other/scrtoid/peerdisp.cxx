#include <headers.hxx>

#ifndef X_HANDIMPL_HXX_
#define X_HANDIMPL_HXX_
#include "handimpl.hxx"
#endif

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include <dispex.h>
#endif

#ifndef X_MISC_HXX_
#define X_MISC_HXX_
#ifdef _MAC
#include "misc2.hxx"
#else
#include "misc.hxx"
#endif
#endif

#ifndef X_MSHTMEXT_H_
#define X_MSHTMEXT_H_
#include "mshtmext.h"
#endif

#ifndef X_PEERHAND_HXX_
#define X_PEERHAND_HXX_
#include "peerhand.hxx"
#endif

#ifndef X_PEERDISP_HXX_
#define X_PEERDISP_HXX_
#include "peerdisp.hxx"
#endif

#ifndef X_ELEMENT_H_
#define X_ELEMENT_H_
#include "element.h"
#endif

#ifndef X_EVENTOBJ_H_
#define X_EVENTOBJ_H_
#include "eventobj.h"
#endif



//+------------------------------------------------------------------------
//
//  Member:     CPeerDispatch constructor
//
//-------------------------------------------------------------------------

CPeerDispatch::CPeerDispatch(CPeerHandler *pPeerHandler)
{
    _ulRefs = 1;
    _pPeerHandler = pPeerHandler;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerDispatch:: destructor
//
//-------------------------------------------------------------------------

CPeerDispatch::~CPeerDispatch()
{
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerDispatch::Disconnect, helper
//
//-------------------------------------------------------------------------

// Sever connection with handler this breaks circularity.
void
CPeerDispatch::Disconnect()
{
    _pPeerHandler = NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerDispatch::QueryInterface, per IUnknown
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerDispatch::QueryInterface(REFIID riid, void** ppv)
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
//  Member:     CPeerDispatch::AddRef, per IUnknown
//
//-------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CPeerDispatch::AddRef()
{
    return ++_ulRefs;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerDispatch::Release, per IUnknown
//
//-------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CPeerDispatch::Release() 
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
//  Member:     CPeerDispatch::GetTypeInfoCount, per IDispatch
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerDispatch::GetTypeInfoCount(UINT * /* pcTypeInfo */)
{
    return E_NOTIMPL;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerDispatch::GetTypeInfo, per IDispatch
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerDispatch::GetTypeInfo(
    UINT            /* uTypeInfo */, 
    LCID            /* lcid */,
    ITypeInfo **    /* ppTypeInfo */)
{
    return E_NOTIMPL;
}
    
//+------------------------------------------------------------------------
//
//  Member:     CPeerDispatch::GetElementDispatchEx, helper
//
//-------------------------------------------------------------------------

HRESULT
CPeerDispatch::GetElementDispatchEx(IDispatchEx ** ppDisp)
{
    HRESULT         hr = E_NOINTERFACE;
    IHTMLElement *  pElement = NULL;

    // Are we sited yet?  Note: We might not be when running inline script.
    if (_pPeerHandler->GetSite())
    {
        hr = _pPeerHandler->GetSite()->GetElement(&pElement);
        if (hr)
            goto Cleanup;

        hr = pElement->QueryInterface(IID_IDispatchEx, (void**)ppDisp);
    }

Cleanup:
    ReleaseInterface (pElement);

    return hr;
} 

//+------------------------------------------------------------------------
//
//  Member:     CPeerDispatch::GetIDsOfNames, per IDispatch
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerDispatch::GetIDsOfNames(
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
CPeerDispatch::Invoke(
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
//  Member:     CPeerDispatch::InternalGetDispID, helper
//
//-------------------------------------------------------------------------

HRESULT
CPeerDispatch::InternalGetDispID(
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
            (*pdispid) = ppchName - GetNamesTable() + DISPID_PEERHAND_BASE;
            return S_OK;
        }
    }

    *pdispid = DISPID_UNKNOWN;
    return DISP_E_UNKNOWNNAME;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerDispatch::InternalInvoke, helper
//
//-------------------------------------------------------------------------

HRESULT
CPeerDispatch::InternalInvoke(
    DISPID              dispid,
    WORD                wFlags,
    DISPPARAMS *        pDispParams,
    VARIANT *           pvarRes,
    EXCEPINFO *         pei)
{
    HRESULT         hr = S_OK;

    if (!_pPeerHandler)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    dispid -= DISPID_PEERHAND_BASE;
    switch (dispid)
    {
    case DISPID_PEERELEMENT:

        //
        // get element this peer is attached to
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

        V_VT(pvarRes) = VT_DISPATCH;
        hr = _pPeerHandler->GetSite()->GetElement((IHTMLElement**)&V_DISPATCH(pvarRes));
        if (hr)
            goto Cleanup;

        break;

    case DISPID_CREATEEVENTOBJECT:

        //
        // create event object
        //

        if (!(wFlags & (DISPATCH_PROPERTYGET | DISPATCH_METHOD)))
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

        if (!_pPeerHandler->_pPeerSiteOM)
        {
            hr = E_UNEXPECTED;
            goto Cleanup;
        }

        V_VT(pvarRes) = VT_DISPATCH;
        hr = _pPeerHandler->_pPeerSiteOM->CreateEventObject((IHTMLEventObj**)&V_DISPATCH(pvarRes));

        break;

    case DISPID_FIREEVENT:

        //
        // raise event
        //

        {
            IHTMLEventObj * pEventObj = NULL;
            DWORD           dwCookie;
            BSTR            bstrEvent;
            IDispatch *     pDispEventObj = NULL;

            if (!(wFlags & DISPATCH_METHOD))
            {
                hr = DISP_E_MEMBERNOTFOUND;
                goto Cleanup;
            }

            if (1 == pDispParams->cArgs)
            {
                if (VT_BSTR != V_VT(&pDispParams->rgvarg[0]))
                {
                    hr = E_INVALIDARG;
                    goto Cleanup;
                }
                bstrEvent = V_BSTR(&pDispParams->rgvarg[0]);
            }
            else if (2 == pDispParams->cArgs)
            {
                if (VT_BSTR     != V_VT(&pDispParams->rgvarg[1]) ||
                    VT_DISPATCH != V_VT(&pDispParams->rgvarg[0]))
                {
                    hr = E_INVALIDARG;
                    goto Cleanup;
                }
                bstrEvent     = V_BSTR    (&pDispParams->rgvarg[1]);
                pDispEventObj = V_DISPATCH(&pDispParams->rgvarg[0]);
            }
            else
            {
                hr = DISP_E_BADPARAMCOUNT;
                goto Cleanup;
            }

            if (!_pPeerHandler->_pPeerSiteOM)
            {
                hr = E_UNEXPECTED;
                goto Cleanup;
            }

            hr = _pPeerHandler->_pPeerSiteOM->GetEventCookie (bstrEvent, &dwCookie);
            if (hr)
                goto LocalCleanup;

            if (pDispEventObj)
            {
                hr = pDispEventObj->QueryInterface(IID_IHTMLEventObj, (void**)&pEventObj);
                if (hr)
                    goto LocalCleanup;
            }

            hr = _pPeerHandler->_pPeerSiteOM->FireEvent (dwCookie, pEventObj);

        LocalCleanup:
            if (DISP_E_MEMBERNOTFOUND == hr)
            {
                // don't allow DISP_E_MEMBERNOTFOUND to fall through as this has special
                // meaning to callers to try more invokes and may cause recursive loop. Also DISP_E_UNKNOWNNAME
                // is more appropriate.
                hr = DISP_E_UNKNOWNNAME;
            }

            ReleaseInterface (pEventObj);
        }

        break;

    case DISPID_ATTACHNOTIFICATION:

        //
        // attach notification handlers
        //

        if (!(wFlags & DISPATCH_METHOD))
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        if (1 != pDispParams->cArgs)
        {
            hr = DISP_E_BADPARAMCOUNT;
            goto Cleanup;
        }

        if (VT_DISPATCH != V_VT(&pDispParams->rgvarg[0]))
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        _pPeerHandler->_pDispNotification = V_DISPATCH(&pDispParams->rgvarg[0]);
        _pPeerHandler->_pDispNotification->AddRef();

        break;

    case DISPID_CANTAKEFOCUS:
        if (wFlags & DISPATCH_PROPERTYGET)
        {
            if (!pvarRes)
            {
                hr = E_POINTER;
                goto Cleanup;
            }

            V_VT(pvarRes) = VT_BOOL;
            V_BOOL(pvarRes) = (_pPeerHandler->_fCanTakeFocus) ? 
                VARIANT_TRUE : VARIANT_FALSE;
        }
        else if (wFlags & DISPATCH_PROPERTYPUT)
        {
            if (pDispParams->cArgs > 1)
            {
                hr = DISP_E_BADPARAMCOUNT;
                goto Cleanup;
            }

            if (VT_BOOL != V_VT(&pDispParams->rgvarg[0]))
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }

            _pPeerHandler->_fCanTakeFocus = 
                (V_BOOL(&pDispParams->rgvarg[0]) == VARIANT_TRUE) ? TRUE : FALSE;
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        break;
        
    default:

        //
        // error case
        //

        hr = DISP_E_MEMBERNOTFOUND;

        break;
    }

Cleanup:

    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerDispatch::GetDispID, per IDispatchEx
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerDispatch::GetDispID(BSTR bstrName, DWORD grfdex, DISPID * pdispid)
{
    HRESULT         hr = S_OK;
    IDispatchEx *   pElement = NULL;
    IDispatchEx *   pWindow = NULL;

    //
    // check our own names
    //

    hr = InternalGetDispID(bstrName, grfdex, pdispid);

    if (DISP_E_UNKNOWNNAME != hr)   // S_OK or error other then DISP_E_UNKNOWNNAME
        goto Cleanup;               // then get out

    //
    // ask the element and then window (element.document.parentWindow)
    //

    hr = GetElementDispatchEx(&pElement);
    if (hr)
        goto Cleanup;

    hr = pElement->GetDispID(bstrName, grfdex, pdispid);
    if (DISP_E_UNKNOWNNAME != hr)   // if S_OK or error other then DISP_E_UNKNOWNNAME
        goto Cleanup;               // get out

    hr = GetWindow (pElement, &pWindow);
    if (hr)
        goto Cleanup;

    hr = pWindow->GetDispID(bstrName, grfdex, pdispid);


Cleanup:
    ReleaseInterface(pElement);
    ReleaseInterface(pWindow);

    return hr;
}
        
//+------------------------------------------------------------------------
//
//  Member:     CPeerDispatch::InvokeEx, per IDispatchEx
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerDispatch::InvokeEx(
    DISPID              dispid,
    LCID                lcid,
    WORD                wFlags,
    DISPPARAMS *        pDispParams,
    VARIANT *           pvarRes,
    EXCEPINFO *         pExcepInfo,
    IServiceProvider *  pServiceCaller)
{
    HRESULT         hr;
    IDispatchEx *   pElement = NULL;
    IDispatchEx *   pWindow = NULL;

    //
    // respond to our own dispids
    //

    hr = InternalInvoke(dispid, wFlags, pDispParams, pvarRes, pExcepInfo);
    if (DISP_E_MEMBERNOTFOUND != hr)    // if S_OK or error other then DISP_E_MEMBERNOTFOUND
        goto Cleanup;                   // get out

    //
    // ask the element and then window (element.document.parentWindow)
    //

    hr = GetElementDispatchEx(&pElement);
    if (hr)
        goto Cleanup;

    hr = pElement->InvokeEx(dispid, lcid, wFlags, pDispParams, pvarRes, pExcepInfo, pServiceCaller);
    if (DISP_E_MEMBERNOTFOUND != hr)    // if S_OK or error other then DISP_E_MEMBERNOTFOUND
        goto Cleanup;                   // get out

    hr = GetWindow (pElement, &pWindow);
    if (hr)
        goto Cleanup;

    hr = pWindow->InvokeEx(dispid, lcid, wFlags, pDispParams, pvarRes, pExcepInfo, pServiceCaller);

Cleanup:
    ReleaseInterface(pElement);
    ReleaseInterface(pWindow);

    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerDispatch::DeleteMemberByName, per IDispatchEx
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerDispatch::DeleteMemberByName(BSTR /*bstr*/, DWORD /*grfdex*/)
{
    return S_OK;
}
        
//+------------------------------------------------------------------------
//
//  Member:     CPeerDispatch::DeleteMemberByDispID, per IDispatchEx
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerDispatch::DeleteMemberByDispID(DISPID /*id*/)
{
    return S_OK;
}
        
//+------------------------------------------------------------------------
//
//  Member:     CPeerDispatch::GetMemberProperties, per IDispatchEx
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerDispatch::GetMemberProperties(DISPID /*dispid*/, DWORD /*grfdexFetch*/, DWORD *pgrfdex)
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
//  Member:     CPeerDispatch::GetMemberName, per IDispatchEx
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerDispatch::GetMemberName(DISPID dispid, BSTR * pbstrName)
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
//  Member:     CPeerDispatch::GetNextDispID, per IDispatchEx
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerDispatch::GetNextDispID(DWORD /*grfdex*/, DISPID /*dispid*/, DISPID * pdispid)
{
    if (!pdispid)
        return E_POINTER;

    // BUGBUG implement
    return S_FALSE;
}
        
//+------------------------------------------------------------------------
//
//  Member:     CPeerDispatch::GetNameSpaceParent, per IDispatchEx
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPeerDispatch::GetNameSpaceParent(IUnknown ** ppUnk)
{
    if (!ppUnk)
        return E_POINTER;

    return _pPeerHandler->GetSite()->GetElement((IHTMLElement**)ppUnk);
}
