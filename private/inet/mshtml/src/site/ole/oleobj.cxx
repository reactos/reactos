//+------------------------------------------------------------------------
//
//  File:       oleobj.cxx
//
//  Contents:   COleSite::CObject implementation.
//
//
//	History:	
//				5-22-95		kfl		converted WCHAR to TCHAR
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

IMPLEMENT_FORMS_SUBOBJECT_IUNKNOWN(COleSite::CObject, COleSite, _Object)

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CObject::GetDispatch
//
//  Synopsis:   Fetch the IDispatch pointer of the embedded control.
//
//  Returns:    NULL if not available.
//
//----------------------------------------------------------------------------

IDispatch *
COleSite::CObject::GetDispatch()
{
    if (IsMyParentAlive())
    {
        return MyCOleSite()->_pDisp;
    }
    else
    {
        return NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CObject::QueryInterface, IUnknown
//
//  Synopsis:   As per IUnknown
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CObject::QueryInterface(REFIID iid, void **ppv)
{
    if (iid == IID_IDispatch || iid == IID_IUnknown)
    {
        *ppv = (IDispatch *)this;
    }
    else
    {
        *ppv = 0;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CObject::GetTypeInfoCount, IDispatch
//
//  Synopsis:   As per IDispatch
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CObject::GetTypeInfoCount(UINT * pctinfo)
{
    IDispatch *pDisp = GetDispatch();

    RRETURN(!pDisp ? E_FAIL : pDisp->GetTypeInfoCount(pctinfo));
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CObject::GetIDsOfNames, IDispatch
//
//  Synopsis:   As per IDispatch
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CObject::GetIDsOfNames(
        REFIID          iid,
        LPTSTR *        rgszNames,
        UINT            cNames,
        LCID            lcid,
        DISPID FAR*     rgdispid)
{
    IDispatch *pDisp = GetDispatch();
    RRETURN(!pDisp ? E_FAIL : pDisp->GetIDsOfNames(
            iid,
            rgszNames,
            cNames,
            lcid,
            rgdispid));
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CObject::Invoke, IDispatch
//
//  Synopsis:   As per IDispatch
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CObject::Invoke(
        DISPID          dispidMember,
        REFIID          iid,
        LCID            lcid,
        WORD            wFlags,
        DISPPARAMS *    pdispparams,
        VARIANT *       pvarResult,
        EXCEPINFO *     pexcepinfo,
        UINT *          puArgErr)
{
    IDispatch *pDisp = GetDispatch();
    RRETURN(!pDisp ? E_FAIL : pDisp->Invoke(
            dispidMember,
            iid,
            lcid,
            wFlags,
            pdispparams,
            pvarResult,
            pexcepinfo,
            puArgErr));
}

STDMETHODIMP
COleSite::CObject::GetTypeInfo(
        UINT         itinfo,
        LCID         lcid,
        ITypeInfo ** pptinfo)
{
    IDispatch *pDisp = GetDispatch();
    RRETURN(!pDisp ? E_FAIL : pDisp->GetTypeInfo(
            itinfo,
            lcid,
            pptinfo));
}
