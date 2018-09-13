//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       collect.cxx
//
//  Contents:   CCollectionCache, CElementCollectionBase and CElementCollection
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_ATOMTBL_HXX_
#define X_ATOMTBL_HXX_
#include "atomtbl.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include <dispex.h>
#endif

#ifndef X_TCELL_HXX_
#define X_TCELL_HXX_
#include "tcell.hxx"
#endif

#ifndef X_CLRNGPRS_HXX_
#define X_CLRNGPRS_HXX_
#include <clrngprs.hxx>
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include <frame.hxx>
#endif

#ifndef X_WINDOW_HXX_
#define X_WIDNOW_HXX_
#include <window.hxx>
#endif

#define _cxx_
#include "collect.hdl"

MtDefine(CElementCollection, Tree, "CElementCollection")
MtDefine(CCollectionCache, Tree, "CCollectionCache")

MtDefine(CElementAryCacheItem, CCollectionCache, "CElementAryCacheItem")
MtDefine(CElementAryCacheItem_aryElements_pv, CElementAryCacheItem, "CElementAryCacheItem::_aryElements._pv")

MtDefine(CCollectionCacheItem, CCollectionCacheItem, "CCollectionCacheItem")
MtDefine(CCollectionCache_aryItems_pv, CCollectionCache, "CCollectionCache::_aryItems::_pv")
MtDefine(CCollectionCache_aryItems_pary, CCollectionCache, "CCollectionCache::_aryItems[i].pary")
MtDefine(CCollectionCache_aryItems_pary_pv, CCollectionCache, "CCollectionCache::_aryItems[i].pary->_pv")
MtDefine(CCollectionCacheGetNewEnum_pary, Tree, "CCollectionCache::GetNewEnum pary")
MtDefine(CCollectionCacheGetNewEnum_pary_pv, Tree, "CCollectionCache::GetNewEnum pary->_pv")

//====================================================================
//
//  Class CElementCollectionBase, CElementCollection methods
//
//===================================================================

//+------------------------------------------------------------------------
//
//  Member:     s_classdesc
//
//  Synopsis:   class descriptor
//
//-------------------------------------------------------------------------

const CBase::CLASSDESC CElementCollectionBase::s_classdesc =
{
    0,                              // _pclsid
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
//  Member:     s_classdesc
//
//  Synopsis:   class descriptor
//
//-------------------------------------------------------------------------

const CBase::CLASSDESC CElementCollection::s_classdesc =
{
    0,                              // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLElementCollection,    // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};


//+------------------------------------------------------------------------
//
//  Member:     CElementCollectionBase
//
//  Synopsis:   constructor
//
//-------------------------------------------------------------------------

CElementCollectionBase::CElementCollectionBase(
        CCollectionCache *pCollectionCache,
        long lIndex)
    : super(), _pCollectionCache(pCollectionCache), _lIndex(lIndex)
{
      // Tell the base to live longer than us
     _pCollectionCache->GetBase()->SubAddRef();
}

//+------------------------------------------------------------------------
//
//  Member:     ~CElementCollectionBase
//
//  Synopsis:   destructor
//
//-------------------------------------------------------------------------

CElementCollectionBase::~CElementCollectionBase()
{
    // release subobject count
    _pCollectionCache->GetBase()->SubRelease();
}

//+------------------------------------------------------------------------
//
//  Member:     ~CElementCollection
//
//  Synopsis:   destructor
//
//-------------------------------------------------------------------------

CElementCollection::~CElementCollection()
{
    _pCollectionCache->ClearDisp(_lIndex);
}

//+------------------------------------------------------------------------
//
//  Member:     PrivateQueryInterface
//
//  Synopsis:   vanilla implementation
//
//-------------------------------------------------------------------------

HRESULT
CElementCollection::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IHTMLElementCollection *)this, IUnknown)
        QI_INHERITS((IHTMLElementCollection *)this, IDispatch)
        QI_INHERITS(this, IDispatchEx)
        QI_TEAROFF(this, IHTMLElementCollection2, NULL)

        default:
            if (iid == CLSID_CElementCollection)
            {
                *ppv = this;
                return S_OK;
            }

            if (iid == IID_IHTMLElementCollection)
                *ppv = (IHTMLElementCollection *)this;
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *) *ppv)->AddRef();

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CElementCollectionBase::GetTypeInfoCount, IDispatch
//
//----------------------------------------------------------------------------

STDMETHODIMP
CElementCollectionBase::GetTypeInfoCount (UINT FAR* pctinfo)
{
    return super::GetTypeInfoCount(pctinfo);
}


//+---------------------------------------------------------------------------
//
//  Member:     CElementCollectionBase::GetTypeInfo, IDispatch
//
//----------------------------------------------------------------------------

STDMETHODIMP
CElementCollectionBase::GetTypeInfo(
                UINT itinfo,
                LCID lcid,
                ITypeInfo ** pptinfo)
{
    return super::GetTypeInfo(itinfo, lcid, pptinfo);
}


//+---------------------------------------------------------------------------
//
//  Member:     CElementCollection::GetIDsOfNames, IDispatch
//
//----------------------------------------------------------------------------

STDMETHODIMP
CElementCollectionBase::GetIDsOfNames(
                REFIID                riid,
                LPOLESTR *            rgszNames,
                UINT                  cNames,
                LCID                  lcid,
                DISPID FAR*           rgdispid)
{
    RRETURN(THR_NOTRACE(GetDispID(rgszNames[0], fdexFromGetIdsOfNames, rgdispid)));
}


HRESULT
CElementCollectionBase::Invoke(
    DISPID          dispidMember,
    REFIID,
    LCID            lcid,
    WORD            wFlags,
    DISPPARAMS *    pdispparams,
    VARIANT *       pvarResult,
    EXCEPINFO *     pexcepinfo,
    UINT *)
{
    return InvokeEx(dispidMember,
                    lcid,
                    wFlags,
                    pdispparams,
                    pvarResult,
                    pexcepinfo,
                    NULL);
}
//+---------------------------------------------------------------------------
//
//  Member:     CElementCollectionBase::Invoke, IDispatch
//
//  Synopsis:   Return the value of the property.
//
//  Note:       If the dispidMember passed to Invoke is DISPID_UNKNOWN we'll
//              return VT_EMPTY.  This allow VBScript/JavaScript to test
//              isnull( ) or comparision to null.
//----------------------------------------------------------------------------
HRESULT
CElementCollectionBase::InvokeEx(
                DISPID          dispidMember,
                LCID            lcid,
                WORD            wFlags,
                DISPPARAMS *    pdispparams,
                VARIANT *       pvarResult,
                EXCEPINFO *     pexcepinfo,
                IServiceProvider *pSrvProvider)
{
    HRESULT             hr;

    hr = THR(_pCollectionCache->EnsureAry(_lIndex));
    if (hr)
        goto Cleanup;

    hr = DispatchInvokeCollection(this,
                                  &super::InvokeEx,
                                  _pCollectionCache,
                                  _lIndex,
                                  dispidMember,
                                  IID_NULL,
                                  lcid,
                                  wFlags,
                                  pdispparams,
                                  pvarResult,
                                  pexcepinfo,
                                  NULL,
                                  pSrvProvider);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     CElementCollectionBase::GetDispID, IDispatchEx
//
//  Synopsis:   defer to cache
//
//----------------------------------------------------------------------------

STDMETHODIMP
CElementCollectionBase::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT     hr;

    hr = THR(_pCollectionCache->EnsureAry(_lIndex));
    if (hr)
        goto Cleanup;

    hr = DispatchGetDispIDCollection(this,
                                     &super::GetDispID,
                                     _pCollectionCache,
                                     _lIndex,
                                     bstrName,
                                     grfdex,
                                     pid);

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CElementCollection::DeleteMember, IDispatchEx
//
//--------------------------------------------------------------------------

HRESULT CElementCollectionBase::DeleteMemberByName(BSTR bstr,DWORD grfdex)
{
    return E_NOTIMPL;
}

HRESULT CElementCollectionBase::DeleteMemberByDispID(DISPID id)
{
    return E_NOTIMPL;
}



//+-------------------------------------------------------------------------
//
//  Method:     CElementCollection::GetMemberProperties, IDispatchEx
//
//--------------------------------------------------------------------------

HRESULT
CElementCollectionBase::GetMemberProperties(
                DISPID id,
                DWORD grfdexFetch,
                DWORD *pgrfdex)
{
    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CElementCollectionBase::GetNextDispID, IDispatchEx
//
//----------------------------------------------------------------------------
STDMETHODIMP
CElementCollectionBase::GetNextDispID(
                DWORD grfdex,
                DISPID id,
                DISPID *prgid)
{
    HRESULT     hr;

    hr = THR(_pCollectionCache->EnsureAry(_lIndex));
    if (hr)
        goto Cleanup;

    hr = DispatchGetNextDispIDCollection(this,
                                         &super::GetNextDispID,
                                         _pCollectionCache,
                                         _lIndex,
                                         grfdex,
                                         id,
                                         prgid);

Cleanup:
    RRETURN1(hr, S_FALSE);
}

STDMETHODIMP
CElementCollectionBase::GetMemberName(
                DISPID id,
                BSTR *pbstrName)
{
    HRESULT     hr;

    hr = THR(_pCollectionCache->EnsureAry(_lIndex));
    if (hr)
        goto Cleanup;

    hr = DispatchGetMemberNameCollection(this,
                                         &super::GetMemberName,
                                         _pCollectionCache,
                                         _lIndex,
                                         id,
                                         pbstrName);

Cleanup:
    RRETURN(hr);
}

HRESULT
CElementCollectionBase::GetNameSpaceParent(IUnknown **ppunk)
{
    HRESULT     hr;

    if (!ppunk)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppunk = NULL;

    hr = S_OK;

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CloseErrorInfo
//
//  Synopsis:   defer to the collection cache
//
//-------------------------------------------------------------------------

HRESULT
CElementCollectionBase::CloseErrorInfo(HRESULT hr)
{
    return _pCollectionCache->CloseErrorInfo(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     get_length
//
//  Synopsis:   collection object model, defers to Cache Helper
//
//-------------------------------------------------------------------------

HRESULT
CElementCollection::get_length(long * plSize)
{
    RRETURN(SetErrorInfo(_pCollectionCache->GetLength(_lIndex, plSize)));
}

//+------------------------------------------------------------------------
//
//  Member:     put_length
//
//  Synopsis:   collection object model, defers to Cache Helper
//
//-------------------------------------------------------------------------

HRESULT
CElementCollection::put_length(long lSize)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}


//+------------------------------------------------------------------------
//
//  Member:     item
//
//  Synopsis:   collection object model
//
//-------------------------------------------------------------------------

HRESULT
CElementCollection::item(VARIANTARG var1, VARIANTARG var2, IDispatch** ppResult)
{
    RRETURN(SetErrorInfo(_pCollectionCache->Item(_lIndex, var1, var2, ppResult)));
}


//+------------------------------------------------------------------------
//
//  Member:     tags
//
//  Synopsis:   collection object model, this always returns a collection
//              and is named based on the tag, and searched based on tagname
//
//-------------------------------------------------------------------------

HRESULT
CElementCollection::tags(VARIANT var1, IDispatch ** ppdisp)
{
    RRETURN(SetErrorInfo(_pCollectionCache->Tags(_lIndex, var1, ppdisp)));
}

HRESULT
CElementCollection::Tags(LPCTSTR szTagName, IDispatch ** ppdisp)
{
    RRETURN(_pCollectionCache->Tags(_lIndex, szTagName, ppdisp));
}

//+------------------------------------------------------------------------
//
//  Member:     urns
//
//  Synopsis:   collection object model, this always returns a collection
//              and is named based on the urn, and searched based on urn
//
//-------------------------------------------------------------------------

HRESULT
CElementCollection::urns(VARIANT var1, IDispatch ** ppdisp)
{
    RRETURN(SetErrorInfo(_pCollectionCache->Urns(_lIndex, var1, ppdisp)));
}

//+------------------------------------------------------------------------
//
//  Member:     Get_newEnum
//
//  Synopsis:   collection object model
//
//-------------------------------------------------------------------------

HRESULT
CElementCollection::get__newEnum(IUnknown ** ppEnum)
{
    RRETURN(SetErrorInfo(_pCollectionCache->GetNewEnum(_lIndex, ppEnum)));
}


//+------------------------------------------------------------------------
//
//  Member:     toString
//
//  Synopsis:   This is impplemented on all objects
//
//-------------------------------------------------------------------------

HRESULT
CElementCollection::toString(BSTR* String)
{
    RRETURN(super::toString(String));
};



//====================================================================
//
//  Class CCollectionCache Methods:
//
//====================================================================

//+------------------------------------------------------------------------
//
//  Member:     ~CCollectionCache
//
//  Synopsis:   Constructor
//
//-------------------------------------------------------------------------

CCollectionCache::CCollectionCache(
        CBase * pBase,
        CDoc *pDoc,
        PFN_CVOID_ENSURE pfnEnsure /* = NULL */,
        PFN_CVOID_CREATECOL pfnCreation /* = NULL */,
        PFN_CVOID_REMOVEOBJECT pfnRemove /* = NULL */,    
        PFN_CVOID_ADDNEWOBJECT pfnAddNewObject /* = NULL */ )
        :   _pBase(pBase), 
            _pfnEnsure(pfnEnsure),     
            _pfnRemoveObject(pfnRemove), 
            _lReservedSize(0),
            _pfnAddNewObject(pfnAddNewObject),
            _pDoc(pDoc),   
            _pfnCreateCollection(pfnCreation),
            _aryItems(Mt(CCollectionCache_aryItems_pv))
{
    Assert(pBase);  // Required.

}

//+------------------------------------------------------------------------
//
//  Member:     ~CCollectionCache
//
//  Synopsis:   Destructor
//
//-------------------------------------------------------------------------

CCollectionCache::~CCollectionCache()
{
    CacheItem * pce = _aryItems;
    UINT cSize = _aryItems.Size();

    for (; cSize--; ++pce)
    {
        delete pce->_pCacheItem;
    }
}


//+------------------------------------------------------------------------
//
//  Member:     Init
//
//  Synopsis:   Setup the cache.  This call is required if part of the
//              cache is to be reserved.
//
//-------------------------------------------------------------------------

HRESULT
CCollectionCache::InitReservedCacheItems(long lReservedSize, long lFromIndex /*= 0*/,
                       long lIdentityIndex  /*= -1*/)
{
    HRESULT hr = E_INVALIDARG;
    CacheItem * pce = NULL;
    long l;

    Assert ( lReservedSize > 0 );
    Assert ( lFromIndex <= lReservedSize && lFromIndex >= 0 );
    Assert ( lIdentityIndex == -1 || (lIdentityIndex >= 0 && lIdentityIndex < _lReservedSize ));

    // Clear the reserved part of the cache.

    hr = THR(_aryItems.EnsureSize(lReservedSize));
    if (hr)
        goto Cleanup;

    pce = _aryItems;

    memset(_aryItems, 0, lReservedSize * sizeof(CacheItem));
    _aryItems.SetSize(lReservedSize);

    // Reserved items always use the CCollectionCacheItem
    for ( l = lFromIndex, pce = _aryItems+lFromIndex  ; l < lReservedSize ; l++, ++pce)
    {
        pce->Init();
        pce->_pCacheItem = new CElementAryCacheItem ();
        if (pce->_pCacheItem == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        MemSetName((pce->_pCacheItem, "CacheItem"));
    }
    
    if (lIdentityIndex != -1 ) 
        _aryItems[lIdentityIndex].fIdentity = TRUE;

Cleanup:
    if ( hr )
    {
        for ( l = _aryItems.Size() ; l >= 0 ; l-- )
        {
            delete pce->_pCacheItem;
        }
        _aryItems.SetSize(0);
    }
    else
    {
        _lReservedSize = lReservedSize;
    }
    RRETURN(hr);
}

HRESULT
CCollectionCache::InitCacheItem(long lCacheIndex, CCollectionCacheItem *pCacheItem)
{
    Assert ( pCacheItem );
    Assert ( lCacheIndex >= 0 && lCacheIndex < _aryItems.Size() );
    Assert ( !_aryItems [ lCacheIndex ]._pCacheItem ); // better not be initialized all ready

    _aryItems [ lCacheIndex ].Init();
    _aryItems [ lCacheIndex ]._pCacheItem = pCacheItem;

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     UnloadContents
//
//  Synopsis : called when the contents of the cache need to be freed up
//      but the cache itself needs to stick around
//
//+------------------------------------------------------------------------
HRESULT
CCollectionCache::UnloadContents()
{
    long l;

    for (l = _aryItems.Size()-1; l>=0; l--)
    {
        _aryItems[l]._lCollectionVersion = 0;

        if (_aryItems[l]._pCacheItem)
            _aryItems[l]._pCacheItem->DeleteContents();
    }

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CreateCollectionHelper
//
//  Synopsis:   Wrapper for private member, if no CreateCollection function is
//      provided, default to creating a CElementCollectionBase. If a create function
//      IS provided, then the base object has a derived collection that it wants
//
//      This returns IDispatch because that is the form that the callers need
//---------------------------------------------------------------------------
HRESULT
CCollectionCache::CreateCollectionHelper(IDispatch ** ppIEC, long lIndex)
{
    HRESULT hr = S_OK;

    *ppIEC = NULL;

    if (_pfnCreateCollection)
    {
#ifdef WIN16
        hr = THR((*_pfnCreateCollection)((CVoid *)(void *)((BYTE *) _pBase - _pBase->m_baseOffset), ppIEC, lIndex));
#else
        hr = THR( CALL_METHOD( (CVoid *)(void *)_pBase, _pfnCreateCollection, (ppIEC, lIndex)));
#endif
    }
    else
    {
        CElementCollection *    pobj;

        pobj = new CElementCollection(this, lIndex);
        if (!pobj)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(pobj->QueryInterface(IID_IDispatch, (void **) ppIEC));
        pobj->Release();
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     GetIntoAry
//
//  Synopsis:   Return the element at the specified index.
//
//-------------------------------------------------------------------------

HRESULT
CCollectionCache::GetIntoAry(
        long lIndex,
        long lIndexElement,
        CElement ** ppElem)
{
    if (lIndexElement >= _aryItems[lIndex]._pCacheItem->Length())
        RRETURN(DISP_E_MEMBERNOTFOUND);

    if (lIndexElement < 0)
        RRETURN(E_INVALIDARG);

    *ppElem = _aryItems[lIndex]._pCacheItem->GetAt (lIndexElement);

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     GetIntoAry
//
//  Synopsis:   Return the index of the specified element.
//
//-------------------------------------------------------------------------

HRESULT
CCollectionCache::GetIntoAry(
        long lIndex,
        CElement * pElement,
        long * plIndexElement)
{
    CCollectionCacheItem    *pCacheItem = _aryItems[lIndex]._pCacheItem;
    CElement                *pElem;
    long                    lElemIndex = 0;
 
    Assert ( plIndexElement );

    *plIndexElement = -1;

    // Using GetNext() should be more efficient than using GetAt();
    pCacheItem->MoveTo( 0 );
    do
    {
        pElem = pCacheItem->GetNext();
        if ( pElem && pElem == pElement )
        {
            *plIndexElement = lElemIndex;
            break;
        }
        lElemIndex++;
    } while ( pElem );


    RRETURN(*plIndexElement == -1 ? DISP_E_MEMBERNOTFOUND : S_OK);
}

//+------------------------------------------------------------------------
//
//  Member:     GetIntoAry
//
//  Synopsis:   Return the element with a given name
//
//  Returns:    S_OK, if it found the element.  *ppNode is set
//              S_FALSE, if multiple elements w/ name were found.
//                  *ppNode is set to the first element in list.
//              Other errors.
//-------------------------------------------------------------------------

HRESULT
CCollectionCache::GetIntoAry(
        long lCollectionIndex,
        LPCTSTR Name,
        BOOL fTagName,
        CElement ** ppElem,
        long iStartFrom /* = 0*/,
        BOOL fCaseSensitive /* = FALSE */)
{
    HRESULT                 hr = S_OK;
    CElement *              pElemFirstMatched = NULL;
    CElement *              pElemLastMatched = NULL;
    CElement *              pElem;
    CCollectionCacheItem *  pCacheItem = _aryItems[lCollectionIndex]._pCacheItem;
    BOOL                    fLastOne = FALSE;

    Assert(ppElem);
    *ppElem = NULL;

    if (iStartFrom == -1)
    {
        iStartFrom = 0;
        fLastOne = TRUE;
    }

    for(pCacheItem->MoveTo( iStartFrom );;)
    {
        pElem = pCacheItem->GetNext();
        if ( !pElem )
            break;
        if ( CompareName( pElem, Name, fTagName, fCaseSensitive) )
        {
            if ( !pElemFirstMatched )
                pElemFirstMatched = pElem;
            pElemLastMatched = pElem;
        }
    }

    if ( !pElemLastMatched )
    {
        hr = DISP_E_MEMBERNOTFOUND;

        if (pCacheItem->IsRangeSyntaxSupported())
        {
            CCellRangeParser    cellRangeParser(Name);
            if(!cellRangeParser.Failed())
            {
                hr = S_FALSE;   // for allowing expando and properties/methods on the collection (TABLE_CELL_COLLECTION)
            }
        }

        goto Cleanup;
    }

    // A collection can be marked to always return the last matching name,
    // rather than the default first matching name.
    if ( DoGetLastMatchedName ( lCollectionIndex ) )
    {
        *ppElem = pElemLastMatched;
    }
    else
    {
        // The iStartFrom has higher precedence on which element we really
        // return first or last in the collection.
        *ppElem = fLastOne ? pElemLastMatched : pElemFirstMatched;
    }

    // return S_FALSE if we have more than one element that matced
    hr = (pElemFirstMatched == pElemLastMatched ) ? S_OK : S_FALSE;

Cleanup:
    RRETURN1(hr, S_FALSE);
}

// Invalid smart collections - one using the _fIsValid technique
void 
CCollectionCache::InvalidateAllSmartCollections(void)
{
    long l;
    for ( l = _aryItems.Size()-1 ; l >= 0 ; l-- )
    {
		_aryItems[l]._fIsValid = FALSE;
    }
}


// Invalidate "dump" collections - ones using old-style version management
void 
CCollectionCache::Invalidate(void)
{
    long l;
    for ( l = _aryItems.Size()-1 ; l >= 0 ; l-- )
    {
        _aryItems[l]._lCollectionVersion  = 0;
    }
}

//+------------------------------------------------------------------------
//
//  Member:     EnsureAry
//
//  Synopsis:   Make sure this index is ready for access.
//
//-------------------------------------------------------------------------

HRESULT
CCollectionCache::EnsureAry(long lIndex)
{
    HRESULT hr = S_OK;

    if (_pfnEnsure && lIndex < _lReservedSize)
    {
        // Ensure the reserved part of the collection
#ifdef WIN16
        hr = THR((*_pfnEnsure)((CVoid *)(void *)((BYTE *) _pBase - _pBase->m_baseOffset), 
            lIndex,
            &_aryItems[lIndex]._lCollectionVersion));
#else
        hr = THR( CALL_METHOD( (CVoid *)(void *)_pBase, 
            _pfnEnsure, 
            (lIndex,&_aryItems[lIndex]._lCollectionVersion)));
#endif
        if (hr)
            goto Cleanup;
    }


    // Ensuring a dynamic collection, need to make sure all its dependent collections
    // are ensured
    if (lIndex >= _lReservedSize)
    {
        // Ensure the collection we're based upon
        // note that this is a recursove call
        hr = THR(EnsureAry(_aryItems[lIndex].sIndex));
        if (hr)
            goto Cleanup;

        // If we're a different version than the collection we're based upon, rebuild ourselves now
        if ( _aryItems[lIndex]._lCollectionVersion != 
            _aryItems[_aryItems[lIndex].sIndex]._lCollectionVersion )
        {
            switch (  _aryItems[lIndex].Type )
            {
            case CacheType_Tag:
                // Rebuild based on name
                hr = THR(BuildNamedArray(
                    _aryItems[lIndex].sIndex,
                    _aryItems[lIndex].cstrName,
                    TRUE,
                    _aryItems[lIndex]._pCacheItem,
                    0,
                    _aryItems[lIndex].fIsCaseSensitive));
                break;

            case CacheType_Named:
                // Rebuild based on tag name
                hr = THR(BuildNamedArray(
                    _aryItems[lIndex].sIndex,
                    _aryItems[lIndex].cstrName,
                    FALSE,
                    _aryItems[lIndex]._pCacheItem,
                    0,
                    _aryItems[lIndex].fIsCaseSensitive));
                break;

            case CacheType_Children:
            case CacheType_DOMChildNodes:
                // Rebuild all children
                hr = THR(BuildChildArray(
                    _aryItems[lIndex].sIndex,
                    _aryItems[lIndex].pElementBase,
                    _aryItems[lIndex]._pCacheItem,
                    FALSE));
                break;

            case CacheType_AllChildren:
                // Rebuild all children
                hr = THR(BuildChildArray(
                    _aryItems[lIndex].sIndex,
                    _aryItems[lIndex].pElementBase,
                    _aryItems[lIndex]._pCacheItem,
                    TRUE));
                break;

            case CacheType_CellRange:
                // Rebuild cells acollection
                hr = THR(BuildCellRangeArray(
                    _aryItems[lIndex].sIndex,
                    _aryItems[lIndex].cstrName,
                    &(_aryItems[lIndex].rectCellRange),
                    _aryItems[lIndex]._pCacheItem));
                break;

            case CacheType_FreeEntry:
                // Free collection waiting to be reused
                break;

            case CacheType_Urn:
                hr = THR(BuildNamedArray(
                    _aryItems[lIndex].sIndex,
                    _aryItems[lIndex].cstrName,
                    TRUE,
                    _aryItems[lIndex]._pCacheItem,
                    0,
                    _aryItems[lIndex].fIsCaseSensitive,
                    TRUE));
                break;

            default:
                Assert(0);
                break;
            }
        }
        _aryItems[lIndex]._lCollectionVersion = 
            _aryItems[_aryItems[lIndex].sIndex]._lCollectionVersion;
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     ClearDisp
//
//  Synopsis:   Clear the reference to this dispatch ptr out of the cache.
//
//              At this point,  any reference that the disp (collection)
//                may have (due to being a named collection) on other collections
//
//-------------------------------------------------------------------------

HRESULT
CCollectionCache::ClearDisp(long lIndex)
{
    Assert ( lIndex >= 0 && lIndex <_aryItems.Size() );

    // Find the collection in the cache & clear it down
    _aryItems[lIndex].pdisp = NULL;
    // Mark it as free
    _aryItems[lIndex].Type = CacheType_FreeEntry;

    if ( lIndex >= _lReservedSize )
    {
        if (_aryItems[lIndex]._pCacheItem)
        {
            short sDepend;

            delete _aryItems[lIndex]._pCacheItem;
            _aryItems[lIndex]._pCacheItem = NULL;

            sDepend = _aryItems[lIndex].sIndex;
            if (sDepend >= _lReservedSize)
            {
                _aryItems[sDepend].pdisp->Release();
            }
        }
        _aryItems[lIndex].cstrName.Free();
    }
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////}


//+------------------------------------------------------------------------
//
//  Member:     GetDisp
//
//  Synopsis:   Get a dispatch ptr from the RESERVED part of the cache.
//              If fIdentity is not set, then everything works as planned.
//              If set, then we QI the base Object and return that
//
//-------------------------------------------------------------------------

HRESULT
CCollectionCache::GetDisp(long lIndex, IDispatch ** ppdisp)
{
    CacheItem * pce;

    Assert((lIndex >= 0) && (lIndex < _aryItems.Size()));

    HRESULT hr = S_OK;

    *ppdisp = NULL;

    pce = &_aryItems[lIndex];

    // if not identity and there is a colllection, addref and return it
    if (!pce->fIdentity)
    {
        if (pce->pdisp)
            pce->pdisp->AddRef();
        else
        {
            IDispatch * pdisp;

            hr = THR(CreateCollectionHelper(&pdisp, lIndex));
            if (hr)
                goto Cleanup;

            //We're not dependant on any other collection
            pce = &_aryItems[lIndex];
            pce->pdisp = pdisp;
            pce->sIndex = -1;
        }

        *ppdisp = pce->pdisp;
    }
    else
        hr = THR_NOTRACE(DYNCAST(CElement, _pBase)->QueryInterface(
                    IID_IDispatch,
                    (void**) ppdisp));

Cleanup:
    RRETURN(hr);
}

// Find pElement in the lIndex base Collection
HRESULT 
CCollectionCache::CreateChildrenCollection(long lCollectionIndex, 
    CElement *pElement, 
    IDispatch **ppDisp,
    BOOL fAllChildren,
    BOOL fDOMCollection)
{
    CacheItem *             pce;
    long                    lSize = _aryItems.Size(),l;
    HRESULT                 hr = S_OK;
    CollCacheType Type = fAllChildren ? 
            CacheType_AllChildren :
            (fDOMCollection) ? CacheType_DOMChildNodes
                             : CacheType_Children;

    Assert (ppDisp);
    *ppDisp = NULL;

    hr = THR(EnsureAry(lCollectionIndex));
    if (hr)
        goto Cleanup;

    // Try and locate an exiting collection
    pce = &_aryItems[_lReservedSize];

    // Return this named collection if it already exists.
    for (l = _lReservedSize; l < lSize; ++l, ++pce)
    {
        if ( pce->Type == Type && 
             pElement == pce->pElementBase )
        {
            pce->pdisp->AddRef();
            *ppDisp = pce->pdisp;
            goto Cleanup;
        }
    }

    // Didn't find it, create a new collection

    hr = THR(GetFreeIndex(&l));  // always returns Idx from non-reserved part of cache
    if (hr)
        goto Cleanup;

    hr = THR(CreateCollectionHelper(ppDisp, l));
    if (hr)
        goto Cleanup;

    pce = &_aryItems[l];
    pce->Init();

    Assert (!pce->_pCacheItem);
    pce->_pCacheItem = new CElementAryCacheItem();
    if ( !pce->_pCacheItem )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(BuildChildArray (lCollectionIndex, pElement, pce->_pCacheItem, fAllChildren ));
    if ( hr )
        goto Cleanup;

    pce->pElementBase = pElement;
    pce->pdisp = *ppDisp;

    pce->sIndex = lCollectionIndex;       // Remember the index we depend on.
    pce->Type = Type;
    
Cleanup:
    RRETURN(hr);
}



//+------------------------------------------------------------------------
//
//  Member:     GetDisp
//
//  Synopsis:   Get a dispatch ptr from the NON-RESERVED part of the cache.
//              N.B. non-reserved can never be identity collecitons,
//
//-------------------------------------------------------------------------

HRESULT
CCollectionCache::GetDisp(
        long lIndex,
        LPCTSTR Name,
        CollCacheType  CacheType,
        IDispatch ** ppdisp,
        BOOL fCaseSensitive /*= FALSE */,
        RECT *pRect /* = NULL */,
        BOOL fAlwaysCollection /* = FALSE */)
{
    CPtrAry<CElement *> *   paryNamed = NULL;
    long                    lSize = _aryItems.Size();
    long                    l;
    HRESULT                 hr = S_OK;
    CacheItem             * pce;
    CRect                   rectCellRange(CRect::CRECT_EMPTY);

    // named arrays are always built into an AryCacheItem
    CElementAryCacheItem           aryItem;

    Assert(CacheType == CacheType_Tag || 
           CacheType == CacheType_Named || 
           CacheType == CacheType_CellRange ||
           CacheType == CacheType_Urn);

    typedef int ( *COMPAREFN)( LPCTSTR, LPCTSTR );
    COMPAREFN CompareFn = fCaseSensitive ? FormsStringCmp : FormsStringICmp;

    *ppdisp = NULL;

    pce = &_aryItems[_lReservedSize];

    // Return this named collection if it already exists.
    for (l = _lReservedSize; l < lSize; ++l, ++pce)
    {
        if(pce->Type == CacheType && lIndex == pce->sIndex && 
             pce->fIsCaseSensitive == (unsigned)fCaseSensitive &&
                            !CompareFn(Name, (BSTR) pce->cstrName))
        {
            pce->pdisp->AddRef();
            *ppdisp = pce->pdisp;
            goto Cleanup;
        }
    }

    // Build the list
    if(CacheType != CacheType_CellRange)
        hr = THR(BuildNamedArray(lIndex, Name, CacheType == CacheType_Tag, 
            &aryItem, 0, fCaseSensitive, CacheType == CacheType_Urn ));
    else
    {           
        if(!pRect)
            // Mark the rect as empty
            rectCellRange.right = -1;
        else
            // Use the passed in rect
            rectCellRange = *pRect;
        hr = THR(BuildCellRangeArray(lIndex, Name, &rectCellRange, &aryItem));
    }

    if (hr)
        goto Cleanup;

    // Return based on what the list of named elements looks like.
    if (!aryItem.Length() && !((CacheType == CacheType_Tag) || (CacheType == CacheType_Urn) || fAlwaysCollection))
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Error;
    }
    // The tags method ALWAYS should return a collection (else case)
    else if (aryItem.Length() == 1 && !((CacheType == CacheType_Tag) || (CacheType == CacheType_Urn) || fAlwaysCollection))
    {
        CElement *pElem = aryItem.GetAt ( 0 );
        hr = THR(pElem->QueryInterface(IID_IDispatch, (void **) ppdisp));
        // Keep the ppdisp around we'll return that and just release the array.
        goto Cleanup2;
    }
    else
    {
        CElementAryCacheItem *pAryItem;

        hr = THR(GetFreeIndex(&l));  // always returns Idx from non-reserved part of cache
        if (hr)
            goto Error;

        hr = THR(CreateCollectionHelper(ppdisp, l ));
        if (hr)
            goto Error;

        pce = &_aryItems[l];
        pce->Init();

        hr = THR(pce->cstrName.Set(Name));
        if (hr)
            goto Error;

        Assert (!pce->_pCacheItem);
        pce->_pCacheItem = new CElementAryCacheItem();
        if ( !pce->_pCacheItem )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        // Copy the array. 
        // For perf reasons assume that the destination collection is a 
        // ary cache - which it is for now, by design
        //
        pAryItem = DYNCAST(CElementAryCacheItem, pce->_pCacheItem);
        pAryItem->CopyAry ( &aryItem ); // this just copies the ptrarray _pv across

        pce->pdisp = *ppdisp;
        pce->sIndex = lIndex;       // Remember the index we depend on.
        pce->Type = CacheType;
        pce->fIsCaseSensitive = fCaseSensitive;

        // Save the range for the cell range type cache so we do not need to parse
        //  the name later
        if(CacheType == CacheType_CellRange)
            pce->rectCellRange = rectCellRange;

        // The collection this named collection was built from is now
        // used to rebuild (ensure) this collection. so we need to
        // put a reference on it so that it will not go away and its
        // location re-assigned by another call to GetFreeIndex.
        // The matching Release() will be done in the dtor
        // although it is not necessary to addref the reserved collections
        //  it is done anyhow, simply for consistency.  This addref
        // only needs to be done for non-reserved collections

        if (lIndex >= _lReservedSize)
        {
            _aryItems[lIndex].pdisp->AddRef();
        }
    }

Cleanup:
    RRETURN(hr);

Error:
    ClearInterface(ppdisp);

Cleanup2:
    delete paryNamed;
    goto Cleanup;
}


//+------------------------------------------------------------------------
//
//  Member:     GetDisp
//
//  Synopsis:   Get a dispatch ptr on an element from the cache.
//
//-------------------------------------------------------------------------

HRESULT
CCollectionCache::GetDisp(long lIndex, long lIndexElement, IDispatch ** ppdisp)
{
    CElement *pElem;

    Assert((lIndex >= 0) && (lIndex < _aryItems.Size()));

    *ppdisp = NULL;

    if (lIndexElement >= _aryItems[lIndex]._pCacheItem->Length())
        RRETURN(DISP_E_MEMBERNOTFOUND);

    if (lIndexElement<0)
        RRETURN(E_INVALIDARG);

    pElem = _aryItems [ lIndex ]._pCacheItem->GetAt( lIndexElement );
    Assert(pElem );

    RRETURN(THR(pElem->QueryInterface(IID_IDispatch, (void **)ppdisp)));
}


//+------------------------------------------------------------------------
//
//  Member:     GetDisp
//
//  Synopsis:   Get a dispatch ptr on an element from the cache.
//      Return the nth element that mathces the name
//
//-------------------------------------------------------------------------

HRESULT
CCollectionCache::GetDisp(
        long lIndex,
        LPCTSTR Name,
        long lNthElement,
        IDispatch ** ppdisp,
        BOOL fCaseSensitive )
{
    long                    lSize,l;
    HRESULT                 hr = DISP_E_MEMBERNOTFOUND;
    CCollectionCacheItem *  pItem;
    CElement *              pElem;

    Assert((lIndex >= 0) && (lIndex < _aryItems.Size()));
    Assert(ppdisp);

    pItem = _aryItems[lIndex]._pCacheItem;

    *ppdisp = NULL;

    // if lIndexElement is too large, just pretend we
    //  didn't find it rather then erroring out
    if (lNthElement < 0 )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    lSize = pItem->Length();

    if (lNthElement >= lSize)
        goto Cleanup;

    for (l = 0; l < lSize; ++l)
    {
        pElem = pItem->GetAt(l);
        Assert(pElem);
        if (CompareName(pElem, Name, FALSE, fCaseSensitive) && !lNthElement--)
            RRETURN(THR(pElem->QueryInterface(IID_IDispatch, (void **) ppdisp)));
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     GetFreeIndex
//
//  Synopsis:   Get a free slot from the NON-RESERVED part of the cache.
//
//-------------------------------------------------------------------------

HRESULT
CCollectionCache::GetFreeIndex(long * plIndex)
{
    CacheItem * pce;
    long    lSize = _aryItems.Size();
    long    l;
    HRESULT hr = S_OK;

    pce = &_aryItems[_lReservedSize];

    // Look for a free slot in the non-reserved part of the cache.
    for (l = _lReservedSize; l < lSize; ++l, ++pce)
    {
        if (pce->Type == CacheType_FreeEntry)
        {
            Assert (!pce->pdisp);
            *plIndex = l;
            goto Cleanup;
        }
    }

    // If we failed to find a free slot then grow the cache by one.
    hr = THR(_aryItems.EnsureSize(l + 1));
    if (hr)
        goto Cleanup;
    _aryItems.SetSize(l + 1);

    pce = &_aryItems[l];

    memset(pce, 0, sizeof(CacheItem));
    pce->fOKToDelete = TRUE;

    *plIndex = lSize;

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CompareName
//
//  Synopsis:   Compares a bstr with an element, either by name rules or
//              as a tagname.
//
//-------------------------------------------------------------------------


BOOL
CCollectionCache::CompareName(CElement * pElement, LPCTSTR Name, 
                              BOOL fTagName, 
                              BOOL fCaseSensitive /* = FALSE */ )
{
    BOOL    fCompare;
    typedef int ( *COMPAREFN)( LPCTSTR, LPCTSTR );
    COMPAREFN CompareFn = fCaseSensitive ? FormsStringCmp : FormsStringICmp;

    if (fTagName)
    {
        fCompare = !CompareFn(Name, pElement->TagName());
    }
    else if ( pElement->IsNamed() )
    {
        BOOL fHasName;
        LPCTSTR pchId = pElement->GetAAname();

        // do we have a name
        fHasName = !!(pchId);

        fCompare = pchId ? !CompareFn(Name, pchId) : FALSE;
        if (!fCompare)
        {
            pchId = pElement->GetAAid();
            fCompare = pchId ? !CompareFn(Name, pchId) : FALSE;
        }

        if (!fCompare)
        {
            pchId = pElement->GetAAuniqueName();
            fCompare = pchId ? !CompareFn(Name, pchId) : FALSE;
        }
    }
	else
		fCompare = FALSE;

    return fCompare;
}

HRESULT 
CCollectionCache::BuildChildArray( 
    long lCollectionIndex, // Index of collection on which this child collection is based
    CElement* pRootElement,
    CCollectionCacheItem *pIntoCacheItem,
    BOOL fAll )
{
    long                    lSize;
    HRESULT                 hr = S_OK;
    long                    lSourceIndex;
    CCollectionCacheItem *  pFromCacheItem;
    CElement *              pElem;
    CTreeNode *             pNode;

    Assert(lCollectionIndex >=0 && lCollectionIndex < _aryItems.Size());
    Assert(pIntoCacheItem );
    Assert(pRootElement);

    pFromCacheItem = _aryItems[lCollectionIndex]._pCacheItem;

    // 
    // BUGBUG rgardner - about this fn & the "assert(lCollectionIndex==0)"
    // As a result this fn is not very generic. This assert also assumes that
    // we're item 0 in the all collection. However, this is currently the only situation
    // this fn is called, and making the assumption optimizes the code
    //

    Assert(lCollectionIndex==0); 

    pIntoCacheItem->ResetContents();

    // Didn't find it, create a new collection
    lSourceIndex = pRootElement->GetSourceIndex();
    // If we are outside the tree return
    if(lSourceIndex < 0)
        goto Cleanup;

    lSize = pFromCacheItem->Length();

    if (lSourceIndex >= lSize)
    {
        // This should never happen
        // No match - Return error 
        Assert(0);
        hr = E_UNEXPECTED;
        goto Cleanup;
    }
    
   
    // Now locate all the immediate children of the element and add them to the array
    for (;;)
    {
        pElem = pFromCacheItem->GetAt(++lSourceIndex) ;
        if ( !pElem )
            break;
        pNode = pElem->GetFirstBranch();
        Assert(pNode);
        // optimize search to spot when we go outside scope of element
        if (!pNode->SearchBranchToRootForScope ( pRootElement ))
        {
            // outside scope of element
            break;
        }
        // If the fall flag is on it means all direct descendants
        // Otherwise it means only immediate children
        if ( fAll || ( pNode->Parent() && 
            pNode->Parent()->Element() == pRootElement ))
        {
            pIntoCacheItem->AppendElement(pElem);
        }
    }
Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     BuildNamedArray
//
//  Synopsis:   Fills in an array based on names found in the array of the
//              given index.
//              If we are building a named array on the ELEMENT_COLLECTION
//              we check if we have already created a collection of all
//              named elements, if not, we build it, if yes, we use this
//              collection to access all named elements
//
//  Result:     S_OK
//              E_OUTOFMEMORY
//
//  Note: It is now the semantics of this function to always return a
//        named array (albeit with a size of 0) instead of returning
//        DISPID_MEMBERNOTFOUND.  This allows for Tags to return an empty
//        collection.
//-------------------------------------------------------------------------

HRESULT
CCollectionCache::BuildNamedArray(
        long lCollectionIndex,
        LPCTSTR Name,
        BOOL fTagName,
        CCollectionCacheItem *pIntoCacheItem,
        long iStartFrom,
        BOOL fCaseSensitive,
        BOOL fUrn /* = FALSE */)
{
    HRESULT                 hr = S_OK;
    CElement *              pElem;
    CCollectionCacheItem *  pFromCacheItem;
    BOOL                    fAddElement;

    Assert ( lCollectionIndex >=0 && lCollectionIndex < _aryItems.Size() );
    Assert ( pIntoCacheItem );

    pFromCacheItem = _aryItems[lCollectionIndex]._pCacheItem;
    pIntoCacheItem->ResetContents(); 


    for ( pFromCacheItem->MoveTo( iStartFrom ) ;; ) 
    {
        pElem = pFromCacheItem->GetNext();
        if ( !pElem )
            break;

        if (fUrn)
        {
            // 'Name' is the Urn we are looking for.  Check if this element has the requested Urn
            fAddElement = pElem->HasPeerWithUrn(Name);
        }
        else if ( CompareName( pElem, Name, fTagName, fCaseSensitive) )
        {
            fAddElement = TRUE;
        }
        else
            fAddElement = FALSE;

        if (fAddElement)
        {
            hr = THR(pIntoCacheItem->AppendElement(pElem));
            if (hr)
                goto Cleanup;
        }
    } 

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     BuildCellRangeArray
//
//  Synopsis:   Fills in an array based on range of cells found in the array of the
//              given index.
//              We first check if we have already created a collection of all
//              cells, if not, we build it, if yes, we use this collection to access 
//              all cells
//
//  Result:     S_OK
//              E_OUTOFMEMORY
//
//  Note: It is now the semantics of this function to always return a
//        named array (albeit with a size of 0) instead of returning
//        DISPID_MEMBERNOTFOUND.  This allows us to return an empty
//        collection.
//-------------------------------------------------------------------------

HRESULT
CCollectionCache::BuildCellRangeArray(long lCollectionIndex, LPCTSTR szRange, 
                             RECT *pRect,  CCollectionCacheItem *pIntoCacheItem )
{
    CCollectionCacheItem *  pFromCacheItem;
    HRESULT                 hr = S_OK;
    CTableCell            * pCell;
    CElement *              pElem;

    Assert(pRect);
    Assert(lCollectionIndex >=0 && lCollectionIndex < _aryItems.Size());
    Assert(pIntoCacheItem );

    pFromCacheItem = _aryItems[lCollectionIndex]._pCacheItem;

    if(pRect->right == -1)
    {
        // The rect is empty, parse it from the string
        CCellRangeParser        cellRangeParser(szRange);
        if(cellRangeParser.Failed())
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        cellRangeParser.GetRangeRect(pRect);
    }

    pIntoCacheItem->ResetContents();

    // Build a list of cells in the specified range
    for ( pFromCacheItem->MoveTo(0);; )
    {
        pElem = pFromCacheItem->GetNext();
        if ( !pElem )
            break;
        if (pElem->Tag() != ETAG_TD && pElem->Tag() != ETAG_TH)
            break;
        pCell = DYNCAST(CTableCell, pElem);
        if (pCell->IsInRange(pRect))
        {
            hr = THR(pIntoCacheItem->AppendElement(pElem));
            if (hr)
                goto Cleanup;
        }
    }
Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CCollectionCache::GetIDsOfNames
//
//  Synopsis:   Helper to support the following syntax
//
//                  Document.TextBox1.Text = "abc"
//
//----------------------------------------------------------------------------

HRESULT
CCollectionCache::GetIDsOfNames(
        long        lCollectionIndex,
        REFIID      iid,
        TCHAR **    rgszNames,
        UINT        cNames,
        LCID        lcid,
        DISPID *    rgdispid)
{
    // Call GetDispID with CaseSensitive flag set to 0
    RRETURN(GetDispID(lCollectionIndex,
                      rgszNames[0], 
                      fdexFromGetIdsOfNames,
                      rgdispid));
}

//+---------------------------------------------------------------------------
//
//  Member:     CCollectionCache::Invoke
//
//  Synopsis:   Helper to support the following syntax
//
//                  Document.TextBox1.Text = "abc"
//
//----------------------------------------------------------------------------

HRESULT
CCollectionCache::Invoke(
        long            lCollectionIndex,
        DISPID          dispid,
        REFIID          iid,
        LCID            lcid,
        WORD            wFlags,
        DISPPARAMS *    pdispparams,
        VARIANT *       pvarResult,
        EXCEPINFO *     pexcepinfo,
        UINT *          puArgErr,
        RETCOLLECT_KIND returnCollection /* = RETCOLLECT_ALL */  )
{
    HRESULT     hr=E_FAIL;
    IDispatch * pDisp = NULL;
    LPCTSTR pch = NULL;

    hr = THR(EnsureAry(lCollectionIndex));
    if (hr)
        goto Cleanup;


    // Is the dispid a collection ordinal index?
    if (IsOrdinalCollectionMember ( lCollectionIndex, dispid ))
    {
        if ( wFlags & DISPATCH_PROPERTYPUT )
        {
            if ( !_pfnAddNewObject )
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }
            if ( ! (pdispparams && pdispparams->cArgs == 1) )
            {
                // No result type we need one for the get to return.
                hr = DISP_E_MEMBERNOTFOUND;
                goto Cleanup;
            }
            // Only allow VARIANT of type IDispatch to be put
            if ( pdispparams->rgvarg[0].vt == VT_NULL )
            {
                // the options collection is special. it allows
                // options[n] = NULL to be specified. in this case
                // map the invoke to a delete on that appropriate index
                if (_aryItems[lCollectionIndex].fSettableNULL)
                {
                    hr = THR(Remove(lCollectionIndex, 
                                    dispid - GetOrdinalMemberMin(lCollectionIndex)));
                    // Like Nav - silently ignore the put if its's outside the current range
                    if ( hr == E_INVALIDARG )
                        hr = S_OK;
                }
                else
                {
                    hr = E_INVALIDARG;
                }

                goto Cleanup;
            }
            else if ( pdispparams->rgvarg[0].vt != VT_DISPATCH )
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }
            // All OK, let the collection cache validate the Put
#ifdef WIN16
            hr = THR ( (*_pfnAddNewObject)( (CVoid *)((void *)((BYTE *) _pBase - _pBase->m_baseOffset)),
                    lCollectionIndex, 
                    V_DISPATCH ( pdispparams->rgvarg ), 
                    dispid - GetOrdinalMemberMin(lCollectionIndex) ));
#else
            hr = THR ( CALL_METHOD( (CVoid *)(void *)_pBase, _pfnAddNewObject, ( 
                    lCollectionIndex, 
                    V_DISPATCH ( pdispparams->rgvarg ), 
                    dispid - GetOrdinalMemberMin(lCollectionIndex) )));
#endif

            if ( hr )
                goto Cleanup;
        }
        else if ( wFlags & DISPATCH_PROPERTYGET )
        {
            VARIANTARG      v1, v2;
            long lIdx = dispid - GetOrdinalMemberMin(lCollectionIndex);

            if ( ! ( (lIdx >= 0) && (lIdx < SizeAry(lCollectionIndex)) ) )
            {
                hr = S_OK;
                if ( pvarResult )
                {
                    VariantClear(pvarResult);
                    pvarResult->vt = VT_NULL;
                    hr = S_OK;
                    goto Cleanup;
                }
            }

            v1.vt = VT_I4;
            v1.lVal = lIdx;

            // Always get the item by index.
            v2.vt = VT_ERROR;

            if (pvarResult)
            {
                hr = Item(lCollectionIndex, v1, v2, &(pvarResult->pdispVal));
                if (!hr)
                {
                    if (!(pvarResult->pdispVal))
                    {
                        hr = E_FAIL;        // use super::Invoke
                    }
                    else
                    {
                        pvarResult->vt = VT_DISPATCH;
                    }
                }
            }
        }
    }
    else if(IsNamedCollectionMember(lCollectionIndex, dispid))
    {
        BOOL        fCaseSensitive;
        long        lOffset;

        lOffset = GetNamedMemberOffset(lCollectionIndex, dispid, &fCaseSensitive);

        hr = THR(GetAtomTable()->GetNameFromAtom(dispid - lOffset, &pch));
        if (hr)
            goto Cleanup;

        if ( returnCollection == RETCOLLECT_ALL && !DoGetLastMatchedName(lCollectionIndex))
        {
            // GetDisp can return a disp ptr to a collection if name matches more than one item
            hr = THR_NOTRACE(GetDisp(
                lCollectionIndex,
                pch,
                CacheType_Named,
                &pDisp,
                fCaseSensitive));
            if (FAILED(hr))
            {
                hr = THR_NOTRACE(GetDisp(
                    lCollectionIndex,
                    pch,
                    CacheType_CellRange,
                    &pDisp,
                    fCaseSensitive));
            }
        }
        else 
        {
            CElement * pElementTemp;
            long lIndex = (returnCollection == RETCOLLECT_LASTITEM) ? -1 : 0;

            // Here GetIntoAry will only return the first/last item that matches the name
            hr = THR_NOTRACE(GetIntoAry(
                    lCollectionIndex,
                    pch,
                    FALSE,
                    &pElementTemp,
                    lIndex,
                    fCaseSensitive));
            if (hr && hr != S_FALSE)
                goto Cleanup;

            hr = THR(pElementTemp->QueryInterface(IID_IDispatch, (void **)&pDisp));
            if (hr)
                goto Cleanup;
        }

        if(hr)
            goto Cleanup;
        //
        //  Special handling for controls accessed as members
        //

        if (wFlags == DISPATCH_PROPERTYGET ||
            wFlags == (DISPATCH_METHOD | DISPATCH_PROPERTYGET))
        {
            if (!pvarResult)
            {
                hr = E_POINTER;
                goto Cleanup;
            }

            // cArgs==1 when Doc.foo(0) is used and =0 when Doc.foo.count
            //  this is only an issue when there are multiple occurances
            //  of foo, and a collection is supposed to be returned by
            //  document.foo
            if (pdispparams->cArgs > 1)
            {
                hr = DISP_E_BADPARAMCOUNT;
                goto Cleanup;
            }
            else  if (pdispparams->cArgs == 1)
            {
                hr = THR(pDisp->Invoke(
                        DISPID_VALUE,
                        iid,
                        lcid,
                        wFlags,
                        pdispparams,
                        pvarResult,
                        pexcepinfo,
                        puArgErr));
            }
            else
            {
                V_VT(pvarResult) = VT_DISPATCH;
                V_DISPATCH(pvarResult) = pDisp;
                pDisp->AddRef();
            }
        }
        else if (wFlags == DISPATCH_PROPERTYPUT ||
            wFlags == DISPATCH_PROPERTYPUTREF)
        {
            if (pdispparams->cArgs != 1)
            {
                hr = DISP_E_BADPARAMCOUNT;
                goto Cleanup;
            }

            hr = THR(pDisp->Invoke(
                    DISPID_VALUE,
                    iid,
                    lcid,
                    wFlags,
                    pdispparams,
                    pvarResult,
                    pexcepinfo,
                    puArgErr));
        }
        else
        {
            // Any other kind of invocation is not valid.
            hr = DISP_E_MEMBERNOTFOUND;
        }
    }
    else
    {
        Assert(FALSE);
    }
Cleanup:
    ReleaseInterface(pDisp);
    RRETURN_NOTRACE(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CCollectionCache::GetDispID, IDispatchEx
//
//  Synopsis:   Support insuring if the name is an index into the collection
//              or passing off to property/expando support.
//
//  Note:       If the name is not known index, property or expando in the
//              collection the returned dispid is DISPID_UNKNOWN with a result
//              of S_OK.  Then on the invoke if DISPID_UNKNOWN is encountered
//              we'll return VT_EMPTY.  This allow VBScript/JavaScript to test
//              isnull( ) or comparision to null.
//
//----------------------------------------------------------------------------

HRESULT
CCollectionCache::GetDispID(long lIndex, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT     hr = E_FAIL;
    long        lIdx = 0;
    long        lAtom;
    CElement * pNode;

    // Could the name be an index.  We check for index first, index for a
    // collection object takes precidence over expando.  As a result if an
    // expando "5" exist then the collection grows to have an item 5 we'll find
    // item and not the expando "5".  If the collection item 5 is removed then
    // we'd find the expando 5.  In addtion we'll allow [""] to index item 0 of
    // the collection.

    // Insure the string is really a number.
    hr = ttol_with_error(bstrName, &lIdx);
    if (!hr)
    {
        // Try to map name to a named element in the collection.
        // Ignore it if we're not promoting ordinals
        if ( !CanPromoteOrdinals (lIndex ) )
        {
            hr = DISP_E_UNKNOWNNAME;
            goto Cleanup;
        }

        if ( _pfnAddNewObject )
        {
            // The presence of _pfnAddNewObject indicates that the collection
            // allows setting to arbitrary indices. Expando on the collection
            // is not allowed.
            *pid = GetOrdinalMemberMin(lIndex)+lIdx;
            if (*pid > GetOrdinalMemberMax(lIndex) )
            {
                hr = DISP_E_UNKNOWNNAME;
            }
            goto Cleanup;
        }
        else
        {
            // Without a _pfnAddNewObject, the collection only supports
            // access to ordinals in the current range. Other accesses
            // become expando.
            hr = THR(EnsureAry(lIndex));
            if (hr)
                goto Cleanup;

            if ( (lIdx >= 0) && (lIdx < SizeAry(lIndex)) )
            {
                *pid = GetOrdinalMemberMin(lIndex)+lIdx;
                if (*pid > GetOrdinalMemberMax(lIndex) )
                {
                    hr = DISP_E_UNKNOWNNAME;
                }
            }
            else
            {
                hr = DISP_E_UNKNOWNNAME;
            }
        }
    }
    else
    {
        long        lMax;
        BOOL        fCaseSensitive;

        // If we don't promote named items - nothing more to do
        if ( !CanPromoteNames(lIndex)  )
        {
            hr = DISP_E_UNKNOWNNAME;
            goto Cleanup;
        }

        hr = THR(EnsureAry(lIndex));
        if (hr)
            goto Cleanup;

        Assert ( _aryItems[lIndex].dispidMin != 0 );
        Assert ( _aryItems[lIndex].dispidMax != 0 );

        fCaseSensitive = ( grfdex & fdexNameCaseSensitive ) != 0;
        //
        // Search the collection for the given name
        //

        hr = THR_NOTRACE(GetIntoAry(
                lIndex,
                bstrName,
                FALSE,
                &pNode, 
                0,
                fCaseSensitive));
        if (FAILED(hr))
        {
            hr = DISP_E_UNKNOWNNAME;
            goto Cleanup;
        }

        //
        // Since we found the element in the elements collection,
        // update atom table.
        //
        Assert(bstrName);
        hr = THR(GetAtomTable()->AddNameToAtomTable(bstrName, &lAtom));
        if (hr)
            goto Cleanup;
        //
        // lAtom is the index into the atom table.  Offset this by
        // base.
        //
        if(fCaseSensitive)
        {
            lAtom += GetSensitiveNamedMemberMin(lIndex);
            lMax = GetSensitiveNamedMemberMax(lIndex);
        }
        else
        {
            lAtom += GetNotSensitiveNamedMemberMin(lIndex);
            lMax = GetNotSensitiveNamedMemberMax(lIndex);
        }

        *pid = lAtom;
        if (*pid > lMax)
        {
            hr = DISP_E_UNKNOWNNAME;
        }
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CCollectionCache::GetNextDispID, IDispatchEx
//
//  Synopsis:   Supports enumerating through all properties, attributes,
//              expandos, and indexes (numbers and names) of a collection.
//
//----------------------------------------------------------------------------
HRESULT
CCollectionCache::GetNextDispID(
                long lIndex,
                DWORD grfdex,
                DISPID id,
                DISPID *prgid)
{
    HRESULT     hr = S_FALSE;
    long        lItmIndex;

    // Enumerating the indexes then enumerate each index of the collection.

    if (IsOrdinalCollectionMember (lIndex, id))
    {
        lItmIndex = id - GetOrdinalMemberMin(lIndex) + 1;

        // Make sure our collection is up-to-date.
        hr = THR(EnsureAry(lIndex));
        if (hr)
            goto Error;

        // Is the number within range for an item in the collection?
        if (lItmIndex < 0 || (lItmIndex >= SizeAry(lIndex)))
            goto Error;

        *prgid = GetOrdinalMemberMin(lIndex) + lItmIndex;

        // Is the index too large?
        if (*prgid > GetOrdinalMemberMax(lIndex) )
            goto Error;
    }

Cleanup:
    RRETURN1(hr, S_FALSE);

Error:
    hr = S_FALSE;
    *prgid = DISPID_UNKNOWN;
    goto Cleanup;
}

HRESULT
CCollectionCache::GetMemberName(
                long lIndex,
                DISPID id,
                BSTR *pbstrName)
{
    HRESULT     hr = S_FALSE;
    long        lItmIndex;
    TCHAR       ach[20];
    CElement *  pElem;
    LPCTSTR     peName;

    if (IsOrdinalCollectionMember (lIndex, id))
    {
        lItmIndex = id - GetOrdinalMemberMin(lIndex);

        // Make sure our collection is up-to-date.
        hr = THR(EnsureAry(lIndex));
        if (hr)
            goto Error;

        // Is the number within range for an item in the collection?
        if (lItmIndex < 0 || (lItmIndex >= SizeAry(lIndex)))
            goto Error;

        // If this fails then we've got real problems the collection
        // size and elements it points to is VERY BAD.

        if ( !_aryItems[lIndex].fDontPromoteNames )
        {
            THR(GetIntoAry(lIndex, lItmIndex, &pElem));
            Assert(pElem);
            peName = pElem->GetIdentifier();
        }
        else
        {
            peName = NULL;
        }
        // If the element doesn't have name associated with it then
        // return the index number.
        if (!peName || !_tcslen(peName))
        {
            // Make a string out of the index.
            hr = Format(0, ach, ARRAY_SIZE(ach), _T("<0d>"), lItmIndex);
            if (hr)
                goto Cleanup;

            peName = ach;
        }

        hr = FormsAllocString(peName, pbstrName);
    }

Cleanup:
    return hr;

Error:
    hr = S_FALSE;
    goto Cleanup;
}

//+------------------------------------------------------------------------
//
//  Member:     GetLength
//
//  Synopsis:   collection object model helper
//
//-------------------------------------------------------------------------

HRESULT
CCollectionCache::GetLength(long lCollection, long * plSize)
{
    HRESULT hr;

    // Make sure our collection is up-to-date.
    hr = THR(EnsureAry(lCollection));
    if (hr)
        goto Cleanup;

    // Get its current size.
    *plSize = SizeAry(lCollection);

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     Item
//
//  Synopsis:   collection object model
//
//              we handle the following parameter cases:
//                  0 params            : by index = 0
//                  1 params bstr       : by name, index = 0
//                  1 params #          : by index
//                  2 params bstr, #    : by name, index
//                  2 params #, bstr    : by index, ignoring bstr
//
//-------------------------------------------------------------------------

HRESULT
CCollectionCache::Item(long lCollection, VARIANTARG var1, VARIANTARG var2, IDispatch** ppResult)
{
    VARIANT *   pvarName = NULL;
    VARIANT *   pvarOne  = NULL;
    VARIANT *   pvarIndex = NULL;
    long        lIndex = 0;
    HRESULT     hr=E_INVALIDARG;

    if (!ppResult)
        goto Cleanup;

    *ppResult = NULL;

    pvarOne = (V_VT(&var1) == (VT_BYREF | VT_VARIANT)) ?
            V_VARIANTREF(&var1) : &var1;

    if ((V_VT(pvarOne)==VT_BSTR) || V_VT(pvarOne)==(VT_BYREF|VT_BSTR))
    {
        pvarName = (V_VT(pvarOne) & VT_BYREF) ?
            V_VARIANTREF(pvarOne) : pvarOne;

        if ((V_VT(&var2) != VT_ERROR ) &&
            (V_VT(&var2) != VT_EMPTY ))
        {
            pvarIndex = &var2;
        }
    }
    else if ((V_VT(&var1) != VT_ERROR )&&
             (V_VT(&var1) != VT_EMPTY ))
    {
        pvarIndex = &var1;
    }

    if (pvarIndex)
    {
        VARIANT varNum;

         VariantInit(&varNum);
         hr = THR(VariantChangeTypeSpecial(&varNum,pvarIndex,VT_I4));
         if (hr)
            goto Cleanup;

         lIndex = V_I4(&varNum);
    }

    // Make sure our collection is up-to-date.
    hr = THR(EnsureAry(lCollection));
    if (hr)
        goto Cleanup;

    // Get a collection or element of the specified object.
    if (pvarName)
    {
        BSTR    Name = V_BSTR(pvarName);

        if (pvarIndex)
        {
            hr = THR(GetDisp(lCollection,
                             Name,
                             lIndex,
                             ppResult,
                             FALSE));   // BUBUG rgardner - shouldn't ignore case
            if (hr)
                goto Cleanup;
        }
        else
        {
            hr = THR(GetDisp(lCollection,
                             Name,
                             CacheType_Named,
                             ppResult,
                             FALSE));   // BUBUG rgardner - shouldn't ignore case
            if (FAILED(hr))
            {
                HRESULT hrSave = hr;    // save error code, and see if it a cell range
                hr = THR_NOTRACE(GetDisp(
                    lCollection,
                    Name,
                    CacheType_CellRange,
                    ppResult,
                    FALSE));            // BUBUG rgardner - shouldn't ignore case
                if (hr)
                {
                    hr = hrSave;        // restore error code
                    goto Cleanup;
                }
            }
        }
    }
    else
    {
        hr = THR(GetDisp(lCollection,
                         lIndex,
                         ppResult));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    // If we didn't find anything, make sure to just return NULL.
    if (hr == DISP_E_MEMBERNOTFOUND)
    {
        hr = S_OK;
    }

    RRETURN(hr);
}

HRESULT
CCollectionCache::GetElemAt(long lCollection, long *plCurrIndex, IDispatch **ppCurrNode)
{
    HRESULT hr;

    // Make sure our collection is up-to-date.
    hr = THR(EnsureAry(lCollection));
    if (hr)
        goto Cleanup;

    *ppCurrNode = NULL;

    if (*plCurrIndex < 0)
    {
        *plCurrIndex = 0;
        goto Cleanup;
    }
    else if (*plCurrIndex > SizeAry(lCollection) - 1)
    {
        *plCurrIndex = SizeAry(lCollection) - 1;
        goto Cleanup;
    }

    hr = THR(GetDisp(lCollection, *plCurrIndex, ppCurrNode));

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     Tags
//
//  Synopsis:   collection object model, this always returns a collection
//              and is named based on the tag, and searched based on tagname
//
//-------------------------------------------------------------------------

HRESULT
CCollectionCache::Tags(long lCollection, VARIANT var1, IDispatch ** ppdisp)
{
    VARIANT *   pvarName = NULL;
    HRESULT     hr=E_INVALIDARG;

    if (!ppdisp)
        goto Cleanup;

    *ppdisp = NULL;

    pvarName = (V_VT(&var1) == (VT_BYREF | VT_VARIANT)) ?
        V_VARIANTREF(&var1) : &var1;

    if ((V_VT(pvarName)==VT_BSTR) || V_VT(pvarName)==(VT_BYREF|VT_BSTR))
    {
        pvarName = (V_VT(pvarName)&VT_BYREF) ?
            V_VARIANTREF(pvarName) : pvarName;
    }
    else
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    hr = THR(Tags(lCollection, V_BSTR(pvarName), ppdisp));

Cleanup:
    RRETURN(hr);
}

HRESULT
CCollectionCache::Tags(long lCollection, LPCTSTR szTagName, IDispatch ** ppdisp)
{
    HRESULT hr;

    // Make sure our collection is up-to-date.
    hr = THR(EnsureAry(lCollection));
    if (hr)
        goto Cleanup;

    // Get a collection of the specified tags.
    hr = THR(GetDisp( lCollection,
                      szTagName,
                      CacheType_Tag,
                      (IDispatch**)ppdisp,
                      FALSE)); // Case sensitivity ignored for TagName
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}



//+------------------------------------------------------------------------
//
//  Member:     Urns
//
//  Synopsis:   collection object model, this always returns a collection
//              and is named based on the urn, and searched based on urn
//
//-------------------------------------------------------------------------

HRESULT
CCollectionCache::Urns(long lCollection, VARIANT var1, IDispatch ** ppdisp)
{
    VARIANT *   pvarName = NULL;
    HRESULT     hr=E_INVALIDARG;

    if (!ppdisp)
        goto Cleanup;

    *ppdisp = NULL;

    pvarName = (V_VT(&var1) == (VT_BYREF | VT_VARIANT)) ?
        V_VARIANTREF(&var1) : &var1;

    if ((V_VT(pvarName)==VT_BSTR) || V_VT(pvarName)==(VT_BYREF|VT_BSTR))
    {
        pvarName = (V_VT(pvarName)&VT_BYREF) ?
            V_VARIANTREF(pvarName) : pvarName;
    }
    else
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    // Make sure our collection is up-to-date.
    hr = THR(EnsureAry(lCollection));
    if (hr)
        goto Cleanup;

    // Get a collection of the elements with the specified urn
    hr = THR(GetDisp( lCollection,
                      V_BSTR(pvarName),
                      CacheType_Urn,
                      (IDispatch**)ppdisp,
                      FALSE)); // Case sensitivity ignored for Urn
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     GetNewEnum
//
//  Synopsis:   collection object model
//
//-------------------------------------------------------------------------

HRESULT
CCollectionCache::GetNewEnum(long lCollection, IUnknown ** ppEnum)
{
    CPtrAry<LPUNKNOWN> *    pary = NULL;
    long                    lSize;
    long                    l;
    HRESULT                 hr=E_INVALIDARG;

    if (!ppEnum)
        goto Cleanup;

    *ppEnum = NULL;

    // Make sure our collection is up-to-date.
    hr = THR(EnsureAry(lCollection));
    if (hr)
        goto Cleanup;

    pary = new(Mt(CCollectionCacheGetNewEnum_pary)) CPtrAry<LPUNKNOWN>(Mt(CCollectionCacheGetNewEnum_pary_pv));
    if (!pary)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    lSize = SizeAry(lCollection);

    hr = THR(pary->EnsureSize(lSize));
    if (hr)
        goto Error;

    // Now make a snapshot of our collection.
    for (l = 0; l < lSize; ++l)
    {
        IDispatch * pdisp;

        hr = THR(GetDisp(lCollection, l, &pdisp));
        if (hr)
            goto Error;

        Verify(!pary->Append(pdisp));
    }

    // Turn the snapshot into an enumerator.
    hr = THR(pary->EnumVARIANT(VT_DISPATCH, (IEnumVARIANT **) ppEnum, FALSE, TRUE));
    if (hr)
        goto Error;

Cleanup:
    RRETURN(hr);

Error:
    pary->ReleaseAll();
    goto Cleanup;
}

//+------------------------------------------------------------------------
//
//  Member:     Remove
//
//  Synopsis:   remove the item in the collection at the given index
//
//-------------------------------------------------------------------------

HRESULT
CCollectionCache::Remove(long lCollection, long lItemIndex)
{
    HRESULT hr;

    if ((lItemIndex < 0) || (lItemIndex >= SizeAry(lCollection)))
        hr = E_INVALIDARG;
    else
    {
        if (_pfnRemoveObject)
#ifdef WIN16
            hr = THR ((*_pfnRemoveObject)((CVoid *)(void *)((BYTE *) _pBase - _pBase->m_baseOffset), lCollection,
                                                                        lItemIndex));
#else
            hr = THR ( CALL_METHOD( (CVoid *)(void *)_pBase, _pfnRemoveObject, 
                                    (lCollection, lItemIndex)));
#endif
        else
            hr = CTL_E_METHODNOTAPPLICABLE;
    }

    RRETURN( hr);
}

////////////////////////////////////////////////////////////////////////////////
//
//  Automation helper routines used by collection classes.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT
DispatchInvokeCollection(CBase *             pThis,
                         InvokeExPROC        SuperInvokeFunction,
                         CCollectionCache *  pCollectionCache,
                         long                lCollectionIdx,
                         DISPID              dispidMember,
                         REFIID              riid,
                         LCID                lcid,
                         WORD                wFlags,
                         DISPPARAMS *        pdispparams,
                         VARIANT *           pvarResult,
                         EXCEPINFO *         pexcepinfo,
                         UINT *              puArgErr,
                         IServiceProvider *  pSrvProvider,
                         RETCOLLECT_KIND     returnCollection /* = RETCOLLECT_ALL */)
{
    HRESULT hr = DISP_E_MEMBERNOTFOUND;
    IHTMLWindow2   *pOmWindow = NULL;

    if (pCollectionCache && pCollectionCache->IsDISPIDInCollection ( lCollectionIdx, dispidMember ) )
    {
        // Note that CCollectionCache::Invokedoesn't need a punkCaller
        hr = pCollectionCache->Invoke(lCollectionIdx,
                                      dispidMember,
                                      riid,
                                      lcid,
                                      wFlags,
                                      pdispparams,
                                      pvarResult,
                                      pexcepinfo,
                                      puArgErr,
                                      returnCollection );
    }

    // for IE3 compat, name access of IFrames should return the om window
    if (lCollectionIdx == CMarkup::NAVDOCUMENT_COLLECTION &&
        !hr &&
        pvarResult &&
        V_VT(pvarResult) == VT_DISPATCH &&
        V_DISPATCH(pvarResult))
    {
        CElement       * pElem     = NULL;

        hr = THR_NOTRACE(V_DISPATCH(pvarResult)->QueryInterface(CLSID_CElement, (void**)&pElem));
        if (hr)
        {
            // what we have may be a collection pointer, so just mask the error
            // and return like we used to.
            hr = S_OK;
            goto Cleanup;
        }

        // For IE3.0 compatibility return named IFRAMEs as elements from the
        // document.  The window IDispatch'd object returned must be a 
        // security object (WindowProxy).
        if (pElem->Tag() == ETAG_IFRAME)
        {
            IHTMLWindow2   *pIFrameWindow = NULL;
            CDoc           *pDoc = pElem->Doc();

            // the above QI does NOT addref. so we need to do it here before
            // the release in the variantClear
            CElement::CLock lock(pElem);

            VariantClear(pvarResult);

            hr = THR(DYNCAST(CFrameSite, pElem)->GetOmWindow(&pOmWindow));
            if (hr)
                goto Cleanup;

            hr = pDoc->EnsureOmWindow();
            if (hr)
                goto Cleanup;

            hr = pDoc->_pOmWindow->SecureObject(pOmWindow, &pIFrameWindow);
            if (hr)
                goto Cleanup;

            V_DISPATCH(pvarResult) = (IDispatch *)pIFrameWindow;
            V_VT(pvarResult)       = VT_DISPATCH;
        }
    }

    // If above didn't work then try to get the property/expando.
    if (hr && pThis)
    {
        hr = THR_NOTRACE(CALL_METHOD( pThis, SuperInvokeFunction, (dispidMember,
                                                       lcid,
                                                       wFlags,
                                                       pdispparams,
                                                       pvarResult,
                                                       pexcepinfo,
                                                       pSrvProvider)));
    }

Cleanup:
    ReleaseInterface(pOmWindow);
    RRETURN(hr);
}



HRESULT
DispatchGetDispIDCollection(CBase *             pThis,
                            GetDispIDPROC       SuperGetDispIDFunction,
                            CCollectionCache *  pCollectionCache,
                            long                lCollectionIdx,
                            BSTR                bstrName,
                            DWORD               grfdex,
                            DISPID    *         pid)
{
    HRESULT     hr;

    Assert(pThis);
    Assert(pCollectionCache);

    hr = THR_NOTRACE(pCollectionCache->GetDispID(lCollectionIdx,
                                                 bstrName,
                                                 grfdex,
                                                 pid));

    // The collectionCache GetIDsOfNamesEx will return S_OK w/ DISPID_UNKNOW
    // if the name isn't found, catastrophic errors are of course returned.
    if (hr || (!hr && *pid == DISPID_UNKNOWN))
    {
        hr = THR_NOTRACE(CALL_METHOD( pThis, SuperGetDispIDFunction, (bstrName, grfdex, pid)));
    }

    RRETURN(hr);
}


HRESULT
DispatchGetNextDispIDCollection(CBase *             pThis,
                                GetNextDispIDPROC   SuperGetNextDispIDFunction,
                                CCollectionCache *  pCollectionCache,
                                long                lCollectionIdx,
                                DWORD               grfdex,
                                DISPID              id,
                                DISPID *            pid)
{
    HRESULT hr = S_FALSE;

    Assert(pThis);
    Assert(pCollectionCache);

    // Are we enumerating the collection indexes?
    if ( !pCollectionCache->IsOrdinalCollectionMember ( lCollectionIdx, id ) )
    {
        // No, so continue enumerating regular attributes, properties, and
        // expandos.
        hr = THR(CALL_METHOD( pThis, SuperGetNextDispIDFunction, (grfdex,
                                                      id,
                                                      pid)));

        // Have we reached the end of the properties, attributes and expandos
        // for the collection?
        if (hr)
        {
            // Yes, so only return a DISPID_UNKNOWN if the collection has no
            // items otherwise return the index 0 of the collection.
            if (pCollectionCache->SizeAry(lCollectionIdx) > 0)
            {
                *pid = pCollectionCache->GetOrdinalMemberMin ( lCollectionIdx );
                hr = S_OK;
            }
        }
    }
    // If we didn't or we're enumerating in the collection index range then go
    // right to the collection cache.
    else
    {
        hr = THR(pCollectionCache->GetNextDispID(lCollectionIdx,
                                                 grfdex,
                                                 id,
                                                 pid));
    }

    RRETURN1(hr, S_FALSE);
}

HRESULT
DispatchGetMemberNameCollection(CBase *                 pThis,
                                GetGetMemberNamePROC    SuperGetMemberNameFunction,
                                CCollectionCache *      pCollectionCache,
                                long                    lCollectionIdx,
                                DISPID                  id,
                                BSTR *                  pbstrName)
{
    if (!pbstrName)
        return E_INVALIDARG;

    *pbstrName = NULL;

    Assert(pThis);
    Assert(pCollectionCache);

    // Are we enumerating the collection indexes?
    if ( !pCollectionCache->IsOrdinalCollectionMember ( lCollectionIdx, id ) )
    {
        // No, so continue enumerating regular attributes, properties, and
        // expandos.
        CALL_METHOD( pThis, SuperGetMemberNameFunction, (id, pbstrName));
    }
    // If we didn't or we're enumerating in the collection index range then go
    // right to the collection cache.
    else
    {
        pCollectionCache->GetMemberName(lCollectionIdx,
                                        id,
                                        pbstrName);
    }

    return *pbstrName ? S_OK : DISP_E_MEMBERNOTFOUND;
}

// returns the correct offset of given dispid in given collection and the 
// case sensetivity flag (if requested)


long
CCollectionCache::GetNamedMemberOffset(long lCollectionIndex, DISPID dispid, 
                             BOOL *pfCaseSensitive /* =  NULL */)
{
    LONG        lOffset;
    BOOL        fSensitive;

    Assert(IsNamedCollectionMember(lCollectionIndex, dispid));

    // Check to see wich half of the dispid space the value goes
    if(IsSensitiveNamedCollectionMember(lCollectionIndex, dispid))
    {
        lOffset = GetSensitiveNamedMemberMin(lCollectionIndex);
        fSensitive = TRUE;
    }
    else
    {
        lOffset = GetNotSensitiveNamedMemberMin(lCollectionIndex);
        fSensitive = FALSE;
    }

    // return the sensitivity flag if required
    if(pfCaseSensitive != NULL)
        *pfCaseSensitive = fSensitive;

    return lOffset;
}


CAtomTable * 
CCollectionCache::GetAtomTable (BOOL *pfExpando)
{   
    Assert(_pDoc);

    if (pfExpando) 
        *pfExpando = _pDoc->_fExpando; 

    return _pDoc->GetAtomTable(); 
}
