//=================================================================
//
//   File:      omrect.cxx
//
//  Contents:   COMRect and COMRectCollection classes
//
//=================================================================

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_OMRECT_HXX_
#define X_OMRECT_HXX_
#include "omrect.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#define _cxx_
#include "omrect.hdl"

MtDefine(COMRect, ObjectModel, "COMRect")
MtDefine(COMRectCollection, ObjectModel, "COMRectCollections")
MtDefine(COMRectCollection_aryRects_pv, COMRectCollection, "COMRectCollection::::aryRects::pv")

//+----------------------------------------------------------------
//
//  member : classdesc
//
//+----------------------------------------------------------------

const CBase::CLASSDESC COMRect::s_classdesc =
{
    NULL,                           // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLRect,                 // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};


//+---------------------------------------------------------------
//
//  Member  : COMRect::COMRect(RECT *)
//
//  Sysnopsis : Constructs an COMRect instance given a rect pointer
//
//----------------------------------------------------------------

COMRect::COMRect(RECT *pRect)
{
    Assert(pRect);
    CopyRect(&_Rect, pRect);
}


//+---------------------------------------------------------------
//
//  Member  : COMRect::PrivateQueryInterface
//
//  Sysnopsis : Vanilla implementation for this class
//
//----------------------------------------------------------------

HRESULT
COMRect::PrivateQueryInterface(REFIID iid, void **ppv)
{
    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_INHERITS(this, IHTMLRect)
    }

    if (*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}



//+---------------------------------------------------------------
//
//  Member  : COMRect::get_left
//
//  Sysnopsis : Returns the "left" property of the rect
//
//----------------------------------------------------------------

HRESULT
COMRect::get_left(long * plVal)
{
	HRESULT hr = S_OK;
    
	if(!plVal)
	{
        hr = E_POINTER;
		goto Cleanup;
	}

    *plVal = _Rect.left;

Cleanup:
	RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------
//
//  Member  : COMRect::put_left
//
//  Sysnopsis : Sets the "left" property of the rect
//
//----------------------------------------------------------------

HRESULT
COMRect::put_left(long lVal)
{
    _Rect.left = lVal;
    return SetErrorInfo(S_OK);
}

//+---------------------------------------------------------------
//
//  Member  : COMRect::get_top
//
//  Sysnopsis : Returns the "top" property of the rect
//
//----------------------------------------------------------------

HRESULT
COMRect::get_top(long * plVal)
{
	HRESULT hr = S_OK;
    
	if(!plVal)
	{
        hr = E_POINTER;
		goto Cleanup;
	}

    *plVal = _Rect.top;
    
Cleanup:
	RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------
//
//  Member  : COMRect::put_top
//
//  Sysnopsis : Sets the "top" property of the rect
//
//----------------------------------------------------------------

HRESULT
COMRect::put_top(long lVal)
{
    _Rect.top = lVal;
    return SetErrorInfo(S_OK);
}


//+---------------------------------------------------------------
//
//  Member  : COMRect::get_right
//
//  Sysnopsis : Returns the "right" property of the rect
//
//----------------------------------------------------------------

HRESULT
COMRect::get_right(long * plVal)
{
	HRESULT hr = S_OK;
    
	if(!plVal)
	{
        hr = E_POINTER;
		goto Cleanup;
	}

    *plVal = _Rect.right;
    
Cleanup:
	RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------
//
//  Member  : COMRect::put_right
//
//  Sysnopsis : Sets the "right" property of the rect
//
//----------------------------------------------------------------

HRESULT
COMRect::put_right(long lVal)
{
    _Rect.right = lVal;
    return SetErrorInfo(S_OK);
}


//+---------------------------------------------------------------
//
//  Member  : COMRect::get_bottom
//
//  Sysnopsis : Returns the "bottom" property of the rect
//
//----------------------------------------------------------------

HRESULT
COMRect::get_bottom(long * plVal)
{
	HRESULT hr = S_OK;
    
	if(!plVal)
	{
        hr = E_POINTER;
		goto Cleanup;
	}

    *plVal = _Rect.bottom;
    
Cleanup:
	RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------
//
//  Member  : COMRect::put_bottom
//
//  Sysnopsis : Sets the "bottom" property of the rect
//
//----------------------------------------------------------------

HRESULT
COMRect::put_bottom(long lVal)
{
    _Rect.bottom = lVal;
    return SetErrorInfo(S_OK);
}


//
//============================================================================
//


//+---------------------------------------------------------------
//
//  Member  : COMRectCollection::~COMRectCollection
//
//----------------------------------------------------------------

COMRectCollection::~COMRectCollection()
{
    _aryRects.ReleaseAll();
}


//+----------------------------------------------------------------
//
//  member : classdesc
//
//+----------------------------------------------------------------

const CBase::CLASSDESC COMRectCollection::s_classdesc =
{
    NULL,                           // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLRectCollection,      // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

//+---------------------------------------------------------------
//
//  Member  : CFontNameOptions::PrivateQueryInterface
//
//  Sysnopsis : Vanilla implementation for this class
//
//----------------------------------------------------------------

HRESULT
COMRectCollection::PrivateQueryInterface(REFIID iid, void **ppv)
{
    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_INHERITS(this, IHTMLRectCollection)
    }

    if (*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}



//+---------------------------------------------------------------
//
//  Member  : COMRectCollection::length
//
//  Sysnopsis : Returns number of elements in the collection
//
//----------------------------------------------------------------

HRESULT
COMRectCollection::get_length(long * pLength)
{
    HRESULT hr = S_OK;

    if (!pLength)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    // Simply return the array size
    *pLength = _aryRects.Size();

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}


//+---------------------------------------------------------------
//
//  Member  : COMRectCollection::item
//
//  Sysnopsis : Returns rect at given index
//----------------------------------------------------------------

HRESULT
COMRectCollection::item(VARIANT * pvarIndex, VARIANT * pvarRet)
{
    HRESULT   hr = S_OK;
    CVariant  varArg;
    long      lIndex;

    if (!pvarRet)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // ECMA rule - return empty for access beyond array bounds
    V_VT(pvarRet) = VT_EMPTY;

    // first attempt ordinal access...
    hr = THR(varArg.CoerceVariantArg(pvarIndex, VT_I4));
    if (hr==S_OK)
    {
        lIndex = V_I4(&varArg);
    }
    else
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(GetItem(lIndex, pvarRet));
    if(hr)
    {
        if(hr == S_FALSE)
            hr = E_INVALIDARG;
        goto Cleanup;
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}



//+---------------------------------------------------------------
//
//  Member  : COMRectCollection::_newEnum
//
//  Sysnopsis :
//
//----------------------------------------------------------------

HRESULT
COMRectCollection::get__newEnum(IUnknown ** ppEnum)
{
    HRESULT hr = S_OK;

    if (!ppEnum)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppEnum = NULL;

    hr = THR(_aryRects.EnumVARIANT(VT_DISPATCH,
                                (IEnumVARIANT**)ppEnum,
                                FALSE,
                                FALSE));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+---------------------------------------------------------------
//
//  Member  : COMRectCollection::SetRects
//
//  Sysnopsis : Get the rects from given array of rects and save
//              them into the member array.
//
//----------------------------------------------------------------

HRESULT 
COMRectCollection::SetRects(CDataAry<RECT> *pSrcRect)
{
    HRESULT   hr = S_OK;
    int       nNumRects;
    COMRect * pOMRect;

    Assert(pSrcRect);
    nNumRects = pSrcRect->Size();
    Assert(nNumRects > 0);

    _aryRects.Grow(nNumRects);

    for(int i = 0; i < nNumRects; i++)
    {
        pOMRect = new COMRect(&(*pSrcRect)[i]);
        if (!pOMRect)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        _aryRects[i] = pOMRect;
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member  : COMRectCollection::FindByName
//
//  Sysnopsis : This function seraches for an item having given
//                name in the collection and returns its index.
//                It returns -1 if item is not found.
//              We do not support named items for rectangle 
//               collection so we always return a -1
//----------------------------------------------------------------
long 
COMRectCollection::FindByName(LPCTSTR pszName, BOOL fCaseSensitive)
{
    return -1;
}

//+---------------------------------------------------------------
//
//  Member  : COMRectCollection::GetName
//
//  Sysnopsis : This virtual function returns the name of given item.
//              We do not support named collection access.
//----------------------------------------------------------------

LPCTSTR 
COMRectCollection::GetName(long lIdx)
{
    return NULL;
}


//+---------------------------------------------------------------
//
//  Member  : COMRectCollection::GetItem
//
//  Sysnopsis : Returns the item that has given order in the collection.
//              If the index is out of range returns S_FALSE.
//              If ppDisp is NULL only checks that range and returns
//               S_OK if index is in range, S_FALSE if out
//
//----------------------------------------------------------------

HRESULT 
COMRectCollection::GetItem( long lIndex, VARIANT *pvar )
{
    HRESULT hr;
    COMRect * pRect;

    if (lIndex < 0 || lIndex >= _aryRects.Size())
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

    V_DISPATCH(pvar) = NULL;

    pRect = _aryRects[lIndex];
    Assert(pRect);
    hr = THR(pRect->PrivateQueryInterface(IID_IDispatch, (void **) &V_DISPATCH(pvar)));
    if (hr)
        goto Cleanup;

    V_VT(pvar) = VT_DISPATCH;
Cleanup:
    RRETURN1(hr, S_FALSE);
}

