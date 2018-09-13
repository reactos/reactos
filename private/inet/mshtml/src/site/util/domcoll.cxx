#include "headers.hxx"

#ifndef X_DOM_HXX_
#define X_DOM_HXX_
#include "dom.hxx"
#endif

#ifndef X_COLLBASE_HXX_
#define X_COLLBASE_HXX_
#include "collbase.hxx"
#endif

#ifndef X_DOMCOLL_HXX_
#define X_DOMCOLL_HXX_
#include "domcoll.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#define _cxx_
#include "domcoll.hdl"

MtDefine(CAttrCollectionator, ObjectModel, "CAttrCollectionator")
MtDefine(CAttrCollectionator_aryAttributes_pv, CAttrCollectionator, "CAttrCollectionator::::aryAttributes::pv")

MtDefine(CDOMChildrenCollection, ObjectModel, "CDOMChildrenCollection")

//+---------------------------------------------------------------
//
//  Member  : CAttrCollectionator::~CAttrCollectionator
//
//----------------------------------------------------------------

CAttrCollectionator::~CAttrCollectionator()
{
    _aryAttributes.ReleaseAll();
    Assert(_pElemColl);
    _pElemColl->Release();
}


//+----------------------------------------------------------------
//
//  member : classdesc
//
//+----------------------------------------------------------------

const CBase::CLASSDESC CAttrCollectionator::s_classdesc =
{
    &CLSID_HTMLAttributeCollection,   // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLAttributeCollection,  // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};


HRESULT
CAttrCollectionator::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IHTMLAttributeCollection, NULL)
    }

    if (*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

HRESULT
CAttrCollectionator::get_length(long *pLength)
{
    HRESULT hr = S_OK;

    if (!pLength)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    *pLength = _aryAttributes.Size();

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CAttrCollectionator::EnsureCollection()
{
    HRESULT hr = S_OK;
    Assert(_pElemColl);
    CAttribute *pAttr;
    IDispatch *pDisp;
    const PROPERTYDESC * const * ppPropdesc = _pElemColl->GetPropDescArray();
    long ulength = _pElemColl->GetPropDescCount();
    long i;

    if (_aryAttributes.Size())
        goto Cleanup;

    for (i = 0; i < ulength; i++)
    {
        pAttr = new CAttribute(ppPropdesc + i, _pElemColl);
        if (!pAttr)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(pAttr->QueryInterface(IID_IDispatch, (void **)&pDisp));
        pAttr->Release();
        if (hr)
            goto Cleanup;

        hr = _aryAttributes.Append(pDisp);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    if (hr)
        _aryAttributes.ReleaseAll();

    return hr;
}

HRESULT
CAttrCollectionator::item(VARIANT *pvarName, IDispatch **ppdisp)
{
    HRESULT   hr;
    CVariant  varArg;
    VARIANT   varDispatch;
    long      lIndex = 0;

    if (!ppdisp)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if ((V_VT(pvarName) != VT_ERROR) && (V_VT(pvarName) != VT_EMPTY))
    {
        // first attempt ordinal access...
        hr = THR(varArg.CoerceVariantArg(pvarName, VT_I4));
        if (hr==S_OK)
            lIndex = V_I4(&varArg);
        else
        {
            // not a number, try named access
            hr = THR_NOTRACE(varArg.CoerceVariantArg(pvarName, VT_BSTR));
            if (hr)
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }
            else
            {
                // find the attribute with this name
                lIndex = FindByName((LPCTSTR)V_BSTR(&varArg));
            }
        }
    }

    hr = THR(GetItem(lIndex, &varDispatch));
    if(hr == S_FALSE)
        hr = E_INVALIDARG;
    else
    {
        Assert(V_VT(&varDispatch) == VT_DISPATCH);
        *ppdisp = V_DISPATCH(&varDispatch);
    }
 
Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CAttrCollectionator::get__newEnum(IUnknown ** ppEnum)
{
    HRESULT hr = S_OK;

    if (!ppEnum)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppEnum = NULL;

    hr = THR(_aryAttributes.EnumVARIANT(VT_DISPATCH,
                                (IEnumVARIANT**)ppEnum,
                                FALSE,
                                FALSE));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

long 
CAttrCollectionator::FindByName(LPCTSTR pszName, BOOL fCaseSensitive)
{
    long  lIdx = 0;

    Assert(_pElemColl);
    _pElemColl->FindPropDescForName(pszName, fCaseSensitive, &lIdx);

    return lIdx;
}

LPCTSTR 
CAttrCollectionator::GetName(long lIdx)
{
    Assert(_pElemColl);
    const PROPERTYDESC * const *ppPropdesc = _pElemColl->GetPropDescArray();
    return (ppPropdesc) ? ppPropdesc[lIdx]->pstrName : NULL;
}

HRESULT 
CAttrCollectionator::GetItem(long lIndex, VARIANT *pvar)
{
    HRESULT hr = S_OK;

    if (lIndex < 0 || lIndex >= _aryAttributes.Size())
    {
        hr = S_FALSE;
		if(pvar)
			V_DISPATCH(pvar) = NULL;
        goto Cleanup;
    }

    if(!pvar)
    {
        // No ppDisp, caller wanted only to check for correct range
        hr = S_OK;
        goto Cleanup;
    }

    V_VT(pvar) = VT_DISPATCH;
    Assert(_aryAttributes[lIndex]);
    V_DISPATCH(pvar) = _aryAttributes[lIndex];
    _aryAttributes[lIndex]->AddRef();

Cleanup:
    RRETURN1(hr, S_FALSE);
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//      CDOMChildrenCollection
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+----------------------------------------------------------------
//
//  member : classdesc
//
//+----------------------------------------------------------------
const CBase::CLASSDESC CDOMChildrenCollection::s_classdesc =
{
    &CLSID_DOMChildrenCollection,   // _pclsid
    0,                                      // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                                   // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                                   // _pcpi
    0,                                      // _dwFlags
    &IID_IHTMLDOMChildrenCollection,    // _piidDispinterface
    &s_apHdlDescs                           // _apHdlDesc
};


//+---------------------------------------------------------------
//
//  Member  : CDOMChildrenCollection::PrivateQueryInterface
//
//  Sysnopsis : Vanilla implementation for this class
//
//----------------------------------------------------------------
HRESULT
CDOMChildrenCollection::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        default:
        {
            const CLASSDESC *pclassdesc = BaseDesc();

            if (pclassdesc &&
                pclassdesc->_piidDispinterface &&
                (iid == *pclassdesc->_piidDispinterface))
            {
                HRESULT hr = THR(CreateTearOffThunk(this, s_apfnIHTMLDOMChildrenCollection, NULL, ppv));
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

//+------------------------------------------------------------------------
//
//  Member:     item
//
//  Synopsis:   collection object model
//
//-------------------------------------------------------------------------

HRESULT
CDOMChildrenCollection::item(long lIndex, IDispatch** ppResult)
{
    HRESULT     hr;
    VARIANT     varDispatch;

    if ( !ppResult )
    {
        RRETURN(E_POINTER);
    }

    hr = THR(GetItem(lIndex, &varDispatch));
    if (hr)
        goto Cleanup;

    Assert(V_VT(&varDispatch) == VT_DISPATCH);
    *ppResult = V_DISPATCH(&varDispatch);

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     get_length
//
//  Synopsis:   collection object model, defers to Cache Helper
//
//-------------------------------------------------------------------------

HRESULT
CDOMChildrenCollection::get_length(long * plSize)
{
    HRESULT hr;
    if (!plSize)
        hr = E_INVALIDARG;
    else
    {
           *plSize = GetLength();
        hr = S_OK;
    }
    RRETURN(SetErrorInfo(hr));
}


//+------------------------------------------------------------------------
//
//  Member:     Get_newEnum
//
//  Synopsis:   collection object model
//
//-------------------------------------------------------------------------

HRESULT
CDOMChildrenCollection::get__newEnum(IUnknown ** ppEnum)
{
    HRESULT hr=S_OK;
    RRETURN(SetErrorInfo(hr));
}

HRESULT 
CDOMChildrenCollection::GetItem (long lIndex, VARIANT *pvar)
{
    HRESULT     hr = S_OK;
    IDispatch **ppDisp;

    if ( lIndex < 0  )
        return E_INVALIDARG;

    if ( pvar )
        V_DISPATCH(pvar) = NULL;

    if ( _fIsElement )
    {
        // Pass through the NULL parameter correctly
        if (pvar)
            ppDisp = &V_DISPATCH(pvar);
        else
            ppDisp = NULL;

        hr = THR(DYNCAST(CElement,_pOwner)->DOMWalkChildren(lIndex, NULL, ppDisp ));
    }
    else
        hr = E_INVALIDARG;

    if (!hr && pvar)
        V_VT(pvar) = VT_DISPATCH;

    RRETURN(hr);
}

HRESULT 
CDOMChildrenCollection::IsValidIndex ( long lIndex )
{
    return (lIndex >= 0 && lIndex < GetLength()) ? S_OK : S_FALSE;
}


long 
CDOMChildrenCollection::GetLength ( void )
{
    long lCount = 0;

    if ( _fIsElement )
        DYNCAST(CElement,_pOwner)->DOMWalkChildren ( -1, &lCount, NULL );

    return lCount;
}


