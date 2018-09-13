//+------------------------------------------------------------------------
//
//  File:       OLEXOBJ.CXX
//
//  Contents:   X-Object Implementation
//
//  Classes:    COleSite
//
//  Notes:      The Ole X object (attempts to) aggregate OLE controls and add
//              to them properties (and methods) specific to the container.
//
//              This XObject delegates the IDispatch implementation to the
//              control for properties and methods it doesn't know about.
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_TYPENAV_HXX_
#define X_TYPENAV_HXX_
#include <typenav.hxx>
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#if defined(_M_ALPHA)
#ifndef X_TEAROFF_HXX_
#define X_TEAROFF_HXX_
#include "tearoff.hxx"
#endif
#endif

HRESULT
InvokeDispatchWithNoThis (
    IDispatch *         pDisp,
    DISPID              dispidMember,
    LCID                lcid,
    WORD                wFlags,
    DISPPARAMS *        pdispparams,
    VARIANT *           pvarResult,
    EXCEPINFO *         pexcepinfo)
{
    HRESULT         hr;
    BOOL            fThis = FALSE;
    VARIANTARG *    rgOldVarg = NULL;
    DISPID     *    rgdispidOldNamedArgs = NULL;

    // Any invoke call from a script engine might have the named argument
    // DISPID_THIS.  If so then we'll not include this argument in the
    // list of parameters because oleaut doesn't know how to deal with this
    // argument.
    if (pdispparams->cNamedArgs && (pdispparams->rgdispidNamedArgs[0] == DISPID_THIS))
    {
        fThis = TRUE;
    
        pdispparams->cNamedArgs--;
        pdispparams->cArgs--;

        rgOldVarg = pdispparams->rgvarg;
        rgdispidOldNamedArgs = pdispparams->rgdispidNamedArgs;

        pdispparams->rgvarg++;
        pdispparams->rgdispidNamedArgs++;

        if (pdispparams->cNamedArgs == 0)
            pdispparams->rgdispidNamedArgs = NULL;

        if (pdispparams->cArgs == 0)
            pdispparams->rgvarg = NULL;
    }

    hr = THR_NOTRACE(pDisp->Invoke(
            dispidMember,
            IID_NULL,
            lcid,
            wFlags,
            pdispparams,
            pvarResult,
            pexcepinfo,
            NULL));


    // restore the named DISPID_THIS argument.
    if (fThis)
    {
        pdispparams->cNamedArgs++;
        pdispparams->cArgs++;

        pdispparams->rgvarg = rgOldVarg;
        pdispparams->rgdispidNamedArgs = rgdispidOldNamedArgs;
    }

    RRETURN1(hr, DISP_E_MEMBERNOTFOUND);
}

//+---------------------------------------------------------------
//
//  Member:     COleSite::IE3XObjInvoke, IDispatch
//
// Synopsis:    supports IE3 xobj props by dispid
//
//---------------------------------------------------------------

HRESULT
COleSite::IE3XObjInvoke(
    DISPID       dispidMember,
    REFIID       riid,
    LCID         lcid,
    WORD         wFlags,
    DISPPARAMS * pdispparams,
    VARIANT *    pvarResult,
    EXCEPINFO *  pexcepinfo,
    UINT *       puArgErr)
{
    HRESULT hr = DISP_E_MEMBERNOTFOUND;

    if (STDPROPID_IE3XOBJ_OBJECTALIGN == dispidMember)
    {
        htmlControlAlign    htmlAlign;
        CVariant            varAlign;

        if (wFlags & DISPATCH_PROPERTYGET)
        {
            if (!pvarResult)
            {
                hr = E_POINTER;
                goto Cleanup;
            }

            htmlAlign = GetAAalign();

            V_VT(pvarResult) = VT_I4;
            switch (htmlAlign)
            {
                case htmlControlAlignTop:       V_I4(pvarResult) = 1;   break;
                case htmlControlAlignBottom:    V_I4(pvarResult) = 2;   break;
                case htmlControlAlignLeft:      V_I4(pvarResult) = 3;   break;
                case htmlControlAlignRight:     V_I4(pvarResult) = 4;   break;
                default:                        V_I4(pvarResult) = 0;   break;
            }
        }
        else if (wFlags & DISPATCH_PROPERTYPUT)
        {
            if (!pdispparams || !pdispparams->rgvarg || 1 != pdispparams->cArgs)
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }

            hr = THR(VariantChangeTypeSpecial(&varAlign, pdispparams->rgvarg, VT_I4));
            if (hr)
                goto Cleanup;

            if (V_I4(&varAlign) < 0 || 4 < V_I4(&varAlign))
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }

            Assert (VT_I4 == V_VT(&varAlign));

            switch (V_I4(&varAlign))
            {
                case 1:    htmlAlign = htmlControlAlignTop;    break;
                case 2:    htmlAlign = htmlControlAlignBottom; break;
                case 3:    htmlAlign = htmlControlAlignLeft;   break;
                case 4:    htmlAlign = htmlControlAlignRight;  break;
                default:   Assert (0); htmlAlign = htmlControlAlignBottom;  break;
            }

            hr = THR(SetAAalign(htmlAlign));
            if (hr)
                goto Cleanup;
        }
        else
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        hr = S_OK;
    }

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------
//
//  Member:     COleSite::ContextThunk_InvokeEx, IDispatch
//
// Synopsis:    Provides access to properties and members of the control
//
// Arguments:   [dispidMember] - Member id to invoke
//              [riid]         - Interface ID being accessed
//              [wFlags]       - Flags describing context of call
//              [pdispparams]  - Structure containing arguments
//              [pvarResult]   - Place to put result
//              [pexcepinfo]   - Pointer to exception information struct
//              [puArgErr]     - Indicates which argument is incorrect
//
//---------------------------------------------------------------

#ifdef USE_STACK_SPEW
#pragma check_stack(off)
#endif

HRESULT
COleSite::ContextThunk_InvokeEx(
        DISPID       dispidMember,
        LCID         lcid,
        WORD         wFlags,
        DISPPARAMS * pdispparams,
        VARIANT *    pvarResult,
        EXCEPINFO *  pexcepinfo,
        IServiceProvider *pSrvProvider)
{
    IUnknown * pUnkContext;

    // Magic macro which pulls context out of nowhere (actually eax)
    CONTEXTTHUNK_SETCONTEXT

    return ContextInvokeEx(
            dispidMember,
            lcid,
            wFlags,
            pdispparams,
            pvarResult,
            pexcepinfo,
            pSrvProvider,
            pUnkContext ? pUnkContext : (IUnknown*)this);
}
#ifdef USE_STACK_SPEW
#pragma check_stack(on)
#endif


//+---------------------------------------------------------------
//
//  Member:     COleSite::ContextInvokeEx
//
// Synopsis:    Provides access to properties and members of the control
//
// Arguments:   [dispidMember] - Member id to invoke
//              [riid]         - Interface ID being accessed
//              [wFlags]       - Flags describing context of call
//              [pdispparams]  - Structure containing arguments
//              [pvarResult]   - Place to put result
//              [pexcepinfo]   - Pointer to exception information struct
//              [puArgErr]     - Indicates which argument is incorrect
//              [pUnkContext]  - IUnknown to pass all calls to
//
//---------------------------------------------------------------

HRESULT
COleSite::ContextInvokeEx(
        DISPID       dispidMember,
        LCID         lcid,
        WORD         wFlags,
        DISPPARAMS * pdispparams,
        VARIANT *    pvarResult,
        EXCEPINFO *  pexcepinfo,
        IServiceProvider *pSrvProvider,
        IUnknown *   pUnkContext)
{
    HRESULT         hr = DISP_E_MEMBERNOTFOUND;
    IDispatchEx *   pDispEx = NULL;
    
    if (IllegalSiteCall(0))
        RRETURN(E_UNEXPECTED);

    CacheDispatch();

    hr = THR_NOTRACE(IE3XObjInvoke(
        dispidMember,
        IID_NULL,
        lcid,
        wFlags,
        pdispparams,
        pvarResult,
        pexcepinfo,
        NULL));
    if (DISP_E_MEMBERNOTFOUND != hr) // if S_OK or error other then DISP_E_MEMBERNOTFOUND
        goto Cleanup;

    // Don't pass on a property get for DISPID_VALUE to CBase. CBase will always
    // return a default [object] string.

    if (!((wFlags & DISPATCH_PROPERTYGET) && dispidMember == DISPID_VALUE))
    {
        // Invoke any HTML element properties OR expando
        hr = THR_NOTRACE(super::ContextInvokeEx(
                    dispidMember,
                    lcid,
                    wFlags,
                    pdispparams,
                    pvarResult,
                    pexcepinfo,
                    pSrvProvider,
                    pUnkContext));
    }

    // If we're NOT an XObject DISPID AND we have a _pDisp AND it is safe to
    // delegate to the underlying object...
    if (hr == DISP_E_MEMBERNOTFOUND &&
        ((ULONG)dispidMember < DISPID_XOBJ_MIN || DISPID_XOBJ_MAX < (ULONG)dispidMember ) &&
        _pDisp &&
        IsSafeToScript())
    {
        if (!AccessAllowed(_pDisp))
        {
            hr = E_ACCESSDENIED;
            goto Cleanup;
        }

        //
        // Try IDispatchEx2 first.
        //

        if (OK(THR_NOTRACE(QueryControlInterface(
                IID_IDispatchEx, 
                (void **)&pDispEx))))
        {
            hr = THR_NOTRACE(pDispEx->InvokeEx(
                    dispidMember,
                    lcid,
                    wFlags,
                    pdispparams,
                    pvarResult,
                    pexcepinfo,
                    pSrvProvider));
        }
        else
        {
            hr = THR_NOTRACE(InvokeDispatchWithNoThis (
                _pDisp,
                dispidMember,
                lcid,
                wFlags,
                pdispparams,
                pvarResult,
                pexcepinfo));
        }

#if 0
// Some activex controls like the IHammer controls will erroneously get an fp error
// w/o clearing the fp status codes.  This results in the script engines returning
// a fp error where none actually occured.  To bullet proof IE4 from from other
// errant apps like this we'll clear all exception flags prior to invoking an
// engine call.
#if (defined(_X86_))
#if DBG==1
__asm {
        fstsw	ax                  ;; Get error flags
        test	al,0DH              ;; See if any errors (Overflow, zero divide, invalid errs)
        jz      NoFPError           ;; No errors.
       }

    Assert(!"OleSite InvokeEx: Floating point status has errors.");
#endif  // DBG==1

    __asm { fnclex }                // Clear fp exception flags.
#endif // _X86_
#endif // 0
    }

#if 0
#if (defined(_X86_))
#if DBG==1
NoFPError:
#endif  // DBG==1
#endif // _X86_
#endif // 0

    // If we didn't find the default property on the OBJECT, retuin the
    // default object->string conversion
    if ( DISPID_NOT_FOUND(hr) && (wFlags & DISPATCH_PROPERTYGET) 
        && dispidMember == DISPID_VALUE && pvarResult &&
        pdispparams->cArgs == 0)
    {
        V_VT(pvarResult) = VT_BSTR;
        hr = THR(FormsAllocString ( _T("[object]"),&V_BSTR(pvarResult) ) );
    }

Cleanup:
    ReleaseInterface(pDispEx);
    RRETURN1(hr, DISP_E_MEMBERNOTFOUND);
}


//+-------------------------------------------------------------------------
//
//  Method:     COleSite::GetDispID, IDispatchEx
//
//  Synopsis:   First try GetDispID, then try expando version.
//
//--------------------------------------------------------------------------

HRESULT
COleSite::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT hr;
    INSTANTCLASSINFO * pici;

    if (IllegalSiteCall(0))
        RRETURN (E_UNEXPECTED);

    //
    // try to resolve the name using the element
    //

    hr = THR_NOTRACE(super::GetDispID(
        bstrName,
        grfdex & (~fdexNameEnsure),     // (don't allow it to create new expandos just yet)
        pid));

    if (S_OK == hr)
    {
        hr = THR(RemapActivexExpandoDispid(pid));
        goto Cleanup;
    }
    else if (DISP_E_MEMBERNOTFOUND != hr && DISP_E_UNKNOWNNAME != hr)   // if S_OK or error other than
    {                                                                   // DISP_E_MEMBERNOTFOUND or DISP_E_UNKNOWNNAME
        goto Cleanup;
    }

    //
    // try to resolve the name using dispatch interfaces the control exposes
    //

    CacheDispatch();
    if (_pDisp)
    {
        pici = GetInstantClassInfo();

        if (pici && pici->IsDispatchEx2())
        {
            // try to resolve the name using IDispatchEx

            IDispatchEx *   pDispEx = NULL;

            hr = THR(_pDisp->QueryInterface(IID_IDispatchEx, (void **)&pDispEx));

            if (S_OK == hr && pDispEx)
            {
                hr = THR_NOTRACE(pDispEx->GetDispID(bstrName, grfdex, pid));

                ReleaseInterface(pDispEx);
            }
	    }
        else
        {
            // try to resolve the name using IDispatch

            hr = THR_NOTRACE(_pDisp->GetIDsOfNames(IID_NULL, &bstrName, 1, 0, pid));
        }

        if (S_OK == hr)     // don't check for DISP_E_MEMBERNOTFOUND or DISP_E_UNKNOWNNAME here - 
        {                   // the control might not follow conventions strictly enough
            goto Cleanup;   // done
        }
    }

    //
    // neither element nor control supports the name; so try to ensure expando
    //

    hr = THR_NOTRACE(GetExpandoDispID(bstrName, pid, grfdex));

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::RemapActivexExpandoDispid
//
//----------------------------------------------------------------------------

HRESULT
COleSite::RemapActivexExpandoDispid(DISPID * pid)
{
    HRESULT     hr = S_OK;

    if (IsExpandoDispid (*pid))
    {
        *pid = (*pid - DISPID_EXPANDO_BASE) + DISPID_ACTIVEX_EXPANDO_BASE;

        // Too many activeX expandos?
        if (*pid > DISPID_ACTIVEX_EXPANDO_MAX)
        {
            // Don't allow it.
            *pid = DISPID_UNKNOWN;
            hr = DISP_E_UNKNOWNNAME;
        }
    }

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::GetNextDispID (IDispatchEx)
//
//  Synopsis:   Enumerates through all properties and html attributes.
//
//----------------------------------------------------------------------------

HRESULT
COleSite::GetNextDispID(
                DWORD grfdex,
                DISPID id,
                DISPID *prgid)
{
    HRESULT     hr;
    BSTR        bstr = NULL;

    CacheDispatch();

    hr = THR(GetInternalNextDispID(grfdex, id, prgid, &bstr, _pDisp));
    SysFreeString(bstr);
    RRETURN1(hr, S_FALSE);
}

HRESULT
COleSite::GetMemberName(DISPID id, BSTR *pbstrName)
{
    if (!pbstrName)
        return E_INVALIDARG;

    *pbstrName = NULL;
    
    if (super::GetMemberName(id, pbstrName))
    {
        CacheDispatch();
        if (_pDisp)
        {
            UINT            cNames = 0;
            ITypeInfo      *pTI = NULL;
            CTypeInfoNav    tin;
                    
            INSTANTCLASSINFO * pici = GetInstantClassInfo();

            if (pici && pici->IsDispatchEx2())
            {
                IDispatchEx *pDispEx = NULL;

                HRESULT hr = THR(_pDisp->QueryInterface(IID_IDispatchEx, (void **)&pDispEx));
                if (S_OK == hr && pDispEx)
                {
                    hr = THR(pDispEx->GetMemberName(id, pbstrName));
                    ReleaseInterface(pDispEx);
                    goto Cleanup;
                }
            }

            if (tin.InitIDispatch(_pDisp, &pTI, 0))
                goto Cleanup;

            if (pTI->GetNames(id, pbstrName, 1, &cNames))
                goto Cleanup;
            
            Assert(cNames == 1);
        }
    }

Cleanup:
    return *pbstrName ? S_OK : DISP_E_MEMBERNOTFOUND;
}

HRESULT
COleSite::GetNameSpaceParent(IUnknown **ppunk)
{
    HRESULT     hr;

    if (!ppunk)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppunk = NULL;

    // BUGBUG (alexz) (terrylu) this needs to consider control's name space additionally
    // to implementation provided by super

    hr = THR(super::GetNameSpaceParent(ppunk));

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite:InterfaceSupportsErrorInfo, ISupportErrorInfo
//
//  Synopsis:   Return true if given interface supports error info.
//
//  Arguments:  iid the interface
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::InterfaceSupportsErrorInfo(REFIID iid)
{
    HRESULT             hr = S_FALSE;
    ISupportErrorInfo * psei;

    hr = THR(super::InterfaceSupportsErrorInfo(iid));
    if (S_OK == hr)
        goto Cleanup;

    // S_FALSE means that the interface is not supported by the x-object.
    // Try the aggregated control.

    if (OK(THR_NOTRACE(QueryControlInterface(
            IID_ISupportErrorInfo, (void**) &psei))))
    {
        hr = THR(psei->InterfaceSupportsErrorInfo(iid));
        ReleaseInterface(psei);
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}



//+---------------------------------------------------------------------------
//
//  Member:     COleSite::GetPages, ISpecifyPropertyPagse
//
//  Synopsis:   Return property pages supported
//
//  Arguments:  [pPages] -- return Pages here
//
//  Returns:    HRESULT
//
//  Notes:      Since the CSite aggregates the control, the set of pages
//              returned is obtained by appending the XObject property page to
//              the list of pages that the control says it supports.
//
//----------------------------------------------------------------------------

HRESULT
COleSite::GetPages(CAUUID * pPages)
{
#ifdef NO_PROPERTY_PAGE
    pPages->pElems = NULL;
    pPages->cElems = 0;
    return S_OK;
#else
    HRESULT hr;

    hr = THR(AddPages(
            _pUnkCtrl,
            BaseDesc()->_apclsidPages,
            pPages));
    RRETURN(hr);
#endif // NO_PROPERTY_PAGE
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite:GetClassInfo, IProvideMultipleClassInfo
//
//  Synopsis:   Returns the control's coclass typeinfo.
//
//  Arguments:  ppTI    Resulting typeinfo.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::GetClassInfo(ITypeInfo ** ppTI)
{
    RRETURN(THR(super::GetClassInfo(ppTI)));
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite:GetGUID, IProvideMultipleClassInfo
//
//  Synopsis:   Returns some type of requested guid
//
//  Arguments:  dwGuidKind      The type of guid requested
//              pGUID           Resultant
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::GetGUID(DWORD dwGuidKind, GUID *pGUID)
{
    RRETURN(THR(super::GetGUID(dwGuidKind, pGUID)));
}


//---------------------------------------------------------------------------
//
//  Member:     COleSite::GetMultiTypeInfoCount
//
//  Synopsis:   per IProvideMultipleClassInfo
//
//---------------------------------------------------------------------------


HRESULT
COleSite::GetMultiTypeInfoCount(ULONG *pc)
{
    RRETURN(GetAggMultiTypeInfoCount(pc, _pUnkCtrl));
}


//---------------------------------------------------------------------------
//
//  Member:     COleSite::GetInfoOfIndex
//
//  Synopsis:   per IProvideMultipleClassInfo
//
//---------------------------------------------------------------------------

HRESULT
COleSite::GetInfoOfIndex(
    ULONG       iTI,
    DWORD       dwFlags,
    ITypeInfo** ppTICoClass,
    DWORD*      pdwTIFlags,
    ULONG*      pcdispidReserved,
    IID*        piidPrimary,
    IID*        piidSource)
{
    RRETURN(GetAggInfoOfIndex(
        iTI,
        dwFlags,
        ppTICoClass,
        pdwTIFlags,
        pcdispidReserved,
        piidPrimary,
        piidSource,
        _pUnkCtrl));
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::GetBaseHref
//
//  Synopsis:   Returns the base href for this object tag.
//				Helper used by CPluginSite and CObjectElement.
//
//----------------------------------------------------------------------------

HRESULT
COleSite::GetBaseHref(BSTR *pbstr)
{
    HRESULT hr;
    TCHAR * pchUrl = NULL;

    *pbstr = NULL;
    
    hr = THR(Doc()->GetBaseUrl(&pchUrl, this));
    if (hr)
        goto Cleanup;
        
    hr = THR(FormsAllocString(pchUrl, pbstr));
    if (hr)
        goto Cleanup;
        
Cleanup:
    RRETURN(hr);
}


//+--------------------------------------------------------------------------
//
//  Member:     COleSite::attachEvent
//
//  Synopsis:   Attach the event
//
//---------------------------------------------------------------------------

HRESULT 
COleSite::attachEvent(BSTR bstrEvent, IDispatch* pDisp, VARIANT_BOOL *pResult)
{
    DISPID      dispid;
    HRESULT     hr = S_OK;
    ITypeInfo * pTIEvent;

    if (!bstrEvent || !pDisp)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    //
    // First see if the event coming in is an event on the ocx.
    // If so, use it's dispid.  Otherwise just use the base implementation
    //

    EnsurePrivateSink();
    pTIEvent = GetClassInfo()->_pTypeInfoEvents;
    if (pTIEvent)
    {
        hr = THR_NOTRACE(pTIEvent->GetIDsOfNames(
                &bstrEvent,
                1,
                &dispid));
        if (!hr)
        {
            hr = THR(AddDispatchObjectMultiple(
                    dispid,
                    pDisp,
                    CAttrValue::AA_AttachEvent));
            goto Cleanup;
        }
    }
    
    hr = THR(super::attachEvent(bstrEvent, pDisp, pResult));

Cleanup:
    if (pResult)
    {
        *pResult = hr ? VARIANT_FALSE : VARIANT_TRUE;
    }

    RRETURN(SetErrorInfo(hr));
}
        

//+--------------------------------------------------------------------------
//
//  Member:     COleSite::detachEvent
//
//  Synopsis:   Detach the event
//
//---------------------------------------------------------------------------

HRESULT
COleSite::detachEvent(BSTR bstrEvent, IDispatch* pDisp)
{
    DISPID      dispid;
    HRESULT     hr = S_OK;
    ITypeInfo * pTIEvent;
    AAINDEX     aaidx = AA_IDX_UNKNOWN;
    IDispatch * pThisDisp = NULL;
    
    if (!bstrEvent || !pDisp)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    //
    // First see if the event coming in is an event on the ocx.
    // If so, use it's dispid.  Otherwise just use the base implementation
    //

    pTIEvent = GetClassInfo()->_pTypeInfoEvents;
    if (pTIEvent)
    {
        hr = THR_NOTRACE(pTIEvent->GetIDsOfNames(
                &bstrEvent,
                1,
                &dispid));
        if (!hr)
        {
            // Find event that has this function pointer.
            for (;;)
            {
                aaidx = FindNextAAIndex(dispid, CAttrValue::AA_AttachEvent, aaidx);
                if (aaidx == AA_IDX_UNKNOWN)
                    break;

                ClearInterface(&pThisDisp);
                if (GetDispatchObjectAt(aaidx, &pThisDisp))
                    continue;

                if (IsSameObject(pDisp, pThisDisp))
                    break;
            };

            // Found item to delete?
            if (aaidx != AA_IDX_UNKNOWN)
            {
                DeleteAt(aaidx);
            }
            goto Cleanup;
        }
    }
    
    hr = THR(super::detachEvent(bstrEvent, pDisp));

Cleanup:
    ReleaseInterface(pThisDisp);
    RRETURN(SetErrorInfo(hr));
}

