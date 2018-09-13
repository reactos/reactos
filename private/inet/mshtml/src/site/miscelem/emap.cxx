//=-----------------------------------------------------------=
//
// File:        emap.cxx
//
// Contents:    Map element class
//
// Classes:     CMapElement
//              CAreasCollection
//
//=-----------------------------------------------------------=

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_EAREA_HXX_
#define X_EAREA_HXX_
#include "earea.hxx"
#endif

#ifndef X_EMAP_HXX_
#define X_EMAP_HXX_
#include "emap.hxx"
#endif

#ifndef X_CSIMUTIL_HXX_
#define X_CSIMUTIL_HXX_
#include "csimutil.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#define _cxx_
#include "map.hdl"

MtDefine(CMapElement, Elements, "CMapElement")
MtDefine(CAreasCollection, Tree, "CAreasCollection")
MtDefine(BldMapAreasCol, PerfPigs, "Build CMapElement::AREAS_COLLECTION")

const CElement::CLASSDESC CMapElement::s_classdesc =
{
    {
        &CLSID_HTMLMapElement,              // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLMapElement,               // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnIHTMLMapElement,          // _pfnTearOff

    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};

CMapElement::CMapElement(CDoc *pDoc)
    : CElement(ETAG_MAP, pDoc)
{
#ifdef WIN16
    m_baseOffset = ((BYTE *) (void *) (CBase *)this) - ((BYTE *) this);
    m_ElementOffset = ((BYTE *) (void *) (CElement *)this) - ((BYTE *) this);
#endif
}

//=-----------------------------------------------------------------------=
//
// Function:    CreateElement
//
// Synopsis:    Creates an instance of the given element's class
//
// Arguments:   CHtmTag *pst - struct for creation info
//              CElement *pElementParent - Parent of new element
//              CElement **ppElement - Return ptr to element in *ppElement.
//
//=-----------------------------------------------------------------------=
HRESULT
CMapElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(pht->Is(ETAG_MAP));
    Assert(ppElement);

    *ppElement = new CMapElement(pDoc);

    return *ppElement ? S_OK : E_OUTOFMEMORY;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      Destructor, and pasivate
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

CMapElement::~CMapElement()
{
    delete _pCollectionCache;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      Notification
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

void
CMapElement::Notify(CNotification *pNF)
{
    CDoc * pDoc;

    super::Notify(pNF);

    switch (pNF->Type())
    {
        case NTYPE_ELEMENT_ENTERTREE:
        {
            pDoc = Doc();
            _pMapNext = pDoc->_pMapHead;
            pDoc->_pMapHead = this;
            break;
        }

        case NTYPE_ELEMENT_EXITTREE_1:
        {
            CMapElement ** ppMap, *pMap;
            pDoc = Doc();
            for (ppMap = &pDoc->_pMapHead; (pMap = *ppMap) != NULL; ppMap = &pMap->_pMapNext)
            {
                if (pMap == this)
                {
                    *ppMap = _pMapNext;
                    break;
                }
            }
            AssertSz(pMap == this, "Can't find CMapElement in CDoc::_pMapHead list");
            break;
        }
    }
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      Containment Checking
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

//=-----------------------------------------------------------------------=
//
// Function:    GetAreaContaining
//
// Synopsis:    Gives the area in the map that contains the given point,
//                  if there is an area that contains it.  If the point
//                  is not contained within an area, it sets it to NULL.
//                  If an area is found, returns S_OK, if not, E_FAIL.
//
// Arguments:   POINT pt - The point for which to check containment.
//              CAreaElement **parea - Area (if any) containing the point
//                  is stored in *parea.
//
//=-----------------------------------------------------------------------=

HRESULT
CMapElement::GetAreaContaining(POINT pt, long *plIndex)
{
    CChildIterator ci(this, NULL, CHILDITERATOR_DEEP);
    CTreeNode * pNode;
    CAreaElement * pArea;
    LONG lIndex = 0;

    while ((pNode = ci.NextChild()) != NULL)
    {
        if (pNode->Tag() == ETAG_AREA)
        {
            pArea = DYNCAST(CAreaElement, pNode->Element());

            if (Contains(pt, pArea->_coords, pArea->_nShapeType))
            {
                *plIndex = lIndex;
                return S_OK;
            }

            lIndex += 1;
        }
    }

    *plIndex = -1;
    return S_OK;
}


//=-----------------------------------------------------------------------=
//
// Function:    GetAreaContaining
//
// Synopsis:    Gives the area in the map that contains the given point,
//                  if there is an area that contains it.  If the point
//                  is not contained within an area, it sets it to NULL.
//                  If an area is found, returns S_OK, if not, E_FAIL.
//
//=-----------------------------------------------------------------------=

HRESULT
CMapElement::GetAreaContaining(long lIndex, CAreaElement **ppArea)
{
    CChildIterator ci(this, NULL, CHILDITERATOR_DEEP);
    CTreeNode * pNode;

    *ppArea = NULL;
    while (lIndex >= 0 && (pNode = ci.NextChild()) != NULL)
    {
        if (pNode->Tag() == ETAG_AREA)
        {
            if (lIndex == 0)
            {
                *ppArea = DYNCAST(CAreaElement, pNode->Element());
                return S_OK;
            }

            lIndex -= 1;
        }
    }

    return E_FAIL;
}

LONG
CMapElement::GetAreaCount()
{
    CChildIterator ci(this, NULL, CHILDITERATOR_DEEP);
    CTreeNode * pNode;
    LONG lCount = 0;

    while ((pNode = ci.NextChild()) != NULL)
    {
        if (pNode->Tag() == ETAG_AREA)
        {
            lCount += 1;
        }
    }

    return lCount;
}

HRESULT
CMapElement::GetAreaTabs(long *pTabs, long c)
{
    CChildIterator ci(this, NULL, CHILDITERATOR_DEEP);
    CTreeNode * pNode;

    while ((pNode = ci.NextChild()) != NULL)
    {
        if (pNode->Tag() == ETAG_AREA)
        {
            if (c == 0)
            {
                AssertSz(0, "Requesting more tabs than there are AREA elements");
                return E_FAIL;
            }

            *pTabs++ = DYNCAST(CAreaElement, pNode->Element())->GetAAtabIndex();
            c -= 1;
        }
    }

    return S_OK;
}

//=-----------------------------------------------------------------------=
//
// Function:    CMapElement::SearchArea
//
// Synopsis:    Search for the given area, returning it's index
//
//=-----------------------------------------------------------------------=

HRESULT
CMapElement::SearchArea(CAreaElement *pAreaFind, long *plIndex)
{
    CChildIterator ci(this, NULL, CHILDITERATOR_DEEP);
    CTreeNode * pNode;
    CAreaElement * pArea;
    LONG lIndex = 0;

    while ((pNode = ci.NextChild()) != NULL)
    {
        if (pNode->Tag() == ETAG_AREA)
        {
            pArea = DYNCAST(CAreaElement, pNode->Element());

            if (pArea == pAreaFind)
            {
                *plIndex = lIndex;
                return S_OK;
            }
        }

        lIndex += 1;
    }

    *plIndex = 0;
    return E_FAIL;
}


//=-----------------------------------------------------------------------=
//
// Function:    GetBoundingRect
//
// Synopsis:    Returns the bounding rectangle for the map, computed as
//              union of the bounding rectangles of the <AREA>s in it.
//
//=-----------------------------------------------------------------------=
void
CMapElement::GetBoundingRect(RECT *prc)
{
    CChildIterator ci(this, NULL, CHILDITERATOR_DEEP);
    CTreeNode * pNode;
    RECT rcArea, rcTemp;

    SetRectEmpty(prc);

    while ((pNode = ci.NextChild()) != NULL)
    {
        if (pNode->Tag() == ETAG_AREA)
        {
            DYNCAST(CAreaElement, pNode->Element())->GetBoundingRect(&rcArea);
            CopyRect(&rcTemp, prc);
            UnionRect(prc, &rcTemp, &rcArea);
        }
    }
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      Drawing Related Code
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

//=------------------------------------------------------------------------=
//
// Function:    Draw
//
// Synopsis:    Refreshes all the areas on the map
//
// Arguments:   HDC hDC - The DC to draw into
//              RECT rc - The rectangle of the image site calling me
//
//=------------------------------------------------------------------------=
HRESULT
CMapElement::Draw(CFormDrawInfo * pDI, CElement * pImg)
{
    CChildIterator  ci(this, NULL, CHILDITERATOR_DEEP);
    CTreeNode *     pNode;
    HPEN            hpenOld;
    HBRUSH          hbrOld;
    int             nROPOld;
    HDC             hdc = pDI->GetDC(TRUE);
    
    // Should come here only in edit mode
    Assert(pImg && pImg->IsEditable(TRUE));

    nROPOld = SetROP2(hdc, R2_XORPEN);
    hpenOld = (HPEN)SelectObject(hdc, GetStockObject(WHITE_PEN));
    hbrOld  = (HBRUSH)SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    
    while ((pNode = ci.NextChild()) != NULL)
    {
        if (pNode->Tag() == ETAG_AREA)
        {
            DYNCAST(CAreaElement, pNode->Element())->Draw(pDI, pImg);
        }
    }

    SelectObject(hdc, hpenOld);
    SelectObject(hdc, hbrOld);
    SetROP2(hdc, nROPOld);

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      Area Collection Code
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
HRESULT
CMapElement::EnsureCollectionCache()
{
    HRESULT hr = S_OK;

    if ( ! _pCollectionCache )
    {
        _pCollectionCache =
            new CCollectionCache(
                this,
                GetDocPtr(),
                ENSURE_METHOD(CMapElement, EnsureAreaCollection, ensureareacollection),
                CREATECOL_METHOD(CMapElement, CreateAreaCollection, createareacollection),
                NULL,
                ADDNEWOBJECT_METHOD(CMapElement, AddNewArea, addnewarea));

        if (!_pCollectionCache)
            goto MemoryError;

        hr = THR(_pCollectionCache->InitReservedCacheItems(1));
        if (hr)
            goto Cleanup;

    }

    hr = THR(_pCollectionCache->EnsureAry(AREA_ELEMENT_COLLECTION));

Cleanup:
    if (hr && _pCollectionCache)
    {
        delete _pCollectionCache;
        _pCollectionCache = NULL;
    }    
    RRETURN(SetErrorInfo(hr));

MemoryError:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

HRESULT BUGCALL
CMapElement::CreateAreaCollection(IDispatch ** ppIEC, long lIndex)
{
    HRESULT             hr = S_OK;
    CAreasCollection *  pobj;

    pobj = new CAreasCollection(_pCollectionCache, lIndex);
    if (!pobj)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pobj->QueryInterface(IID_IDispatch, (void **) ppIEC));
    pobj->Release();
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

HRESULT BUGCALL
CMapElement::EnsureAreaCollection(long lIndex, long * plCollectionVersion)
{
    CTreeNode *     pNode;
    CDoc *          pDoc = Doc();
    HRESULT         hr = S_OK;

    // Nothing to do so get out.
    if (*plCollectionVersion == pDoc->GetDocTreeVersion())
        return S_OK;

    MtAdd(Mt(BldMapAreasCol), +1, 0);

    // Reset this collection.
    _pCollectionCache->ResetAry(AREA_ELEMENT_COLLECTION);

    if(IsInMarkup())
    {
        CChildIterator  ci(this, NULL, CHILDITERATOR_DEEP);
        while ((pNode = ci.NextChild()) != NULL)
        {
            if (pNode->Tag() == ETAG_AREA)
            {
                hr = THR(_pCollectionCache->SetIntoAry(AREA_ELEMENT_COLLECTION, pNode->Element()));
                if (hr)
                    goto Error;
            }
        }
    }
    
    *plCollectionVersion = pDoc->GetDocTreeVersion();

Cleanup:
    RRETURN(hr);

Error:
    _pCollectionCache->ResetAry(AREA_ELEMENT_COLLECTION);
    goto Cleanup;
}

//+------------------------------------------------------------------------
//
//  Member:     AddAreaHelper
//
//  Synopsis:   Add area to collection
//
//-------------------------------------------------------------------------
HRESULT
CMapElement::AddAreaHelper(CAreaElement * pArea, long lItemIndex)
{
    HRESULT         hr;

    if (lItemIndex == -1)
        lItemIndex = GetAreaCount(); // append

    // insert the area into the element tree, with pMap as parent at the
    // position lItemIndex
    hr = THR(pArea->InsertIntoElemTree(this, lItemIndex));
    if (hr)
        goto Cleanup;
        
Cleanup:
    RRETURN (SetErrorInfo(hr));
}


//+------------------------------------------------------------------------
//
//  Member:     RemoveAreaHelper
//
//  Synopsis:   remove the area at the given index 
//
//-------------------------------------------------------------------------

HRESULT
CMapElement::RemoveAreaHelper(long lItemIndex)
{
    HRESULT         hr;
    CAreaElement *  pArea;
    
    Assert (lItemIndex >= 0);
    
    hr = THR(GetAreaContaining(lItemIndex, &pArea));
    if (hr)
    {
        // Silently ignore out-of-bounds condition
        hr = S_OK;
        goto Cleanup;
    }

    // remove the area from the element tree
    hr = THR(pArea->RemoveFromElemTree());
    if (hr)
        goto Cleanup;
        
Cleanup:
    RRETURN (SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:     CMapElement::AddNewArea
//
// Supports adding area element to the areas collection via
// JScript array access e.g.
// areas [ 7 ] = new Area();
//----------------------------------------------------------------------------

HRESULT BUGCALL
CMapElement::AddNewArea(long lIndex, IDispatch *pObject, long index)
{
    HRESULT             hr = S_OK;
    CAreaElement *      pArea;
    IUnknown *          pUnk;
    long                lDummy;
    CElement *          pElement = NULL;
    long                lAreaCount;

    if (index < -1)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Make sure that pObject is an <AREA> element
    hr = THR(pObject->QueryInterface(IID_IHTMLAreaElement, (void**)&pUnk));
    ReleaseInterface(pUnk);
    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    lAreaCount = GetAreaCount();

    if (index == -1)
        index = lAreaCount; // append

    // index is the ordinal position to add/replace
    // If it exists, replace the existing element.
    // If not extend the options array with default elements
    // up to index-1, then add the new element
    // Verify that pObject is an IOptionElement

    if (index < lAreaCount)
    {
        // remove the current element at 'index'
        hr = THR(RemoveAreaHelper(index));
        if (hr)
            goto Cleanup;

        lAreaCount -= 1;
    }
    else
    {
        CDoc *  pDoc = Doc();

        // pad with dummy elements till index - 1

        for (lDummy = index - lAreaCount; lDummy > 0; --lDummy)
        {
            hr = THR(pDoc->CreateElement(ETAG_AREA, &pElement));
            if (hr)
                goto Cleanup;

            pArea = DYNCAST(CAreaElement, pElement);
            // insert the dummy element
            hr = THR(AddAreaHelper(pArea, lDummy));
            if (hr)
                goto Cleanup;
                
            CElement::ClearPtr(&pElement);
           
            lAreaCount += 1;
        }
    }

    // insert the new element at 'index'
    Verify(S_OK == THR(pObject->QueryInterface(CLSID_CElement, (void **)&pArea)));

    // Bail out if the element is already in the tree - #25130
    if (pArea->IsInMarkup())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }


    hr = THR(AddAreaHelper(pArea, index));
    if (hr)
        goto Cleanup;

Cleanup:

    CElement::ClearPtr(&pElement);
        
    RRETURN(hr);
}


HRESULT
CMapElement::get_areas(IHTMLAreasCollection ** ppElemCol)
{
    HRESULT hr;

    if (!ppElemCol)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    hr = EnsureCollectionCache(); // Ensures AREAS
    if(hr)
        goto Cleanup;

    hr = THR(_pCollectionCache->GetDisp(0, (IDispatch**)ppElemCol));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//          Class CAreasCollection method implementations
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


//+------------------------------------------------------------------------
//
//  Member:     s_classdesc
//
//  Synopsis:   class descriptor
//
//-------------------------------------------------------------------------

const CBase::CLASSDESC CAreasCollection::s_classdesc =
{
    NULL,                           // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLAreasCollection,      // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

//+------------------------------------------------------------------------
//
//  Member:     ~CAreasCollection
//
//  Synopsis:   destructor
//
//-------------------------------------------------------------------------

CAreasCollection::~CAreasCollection()
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
CAreasCollection::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IHTMLAreasCollection *)this, IUnknown)
        QI_INHERITS((IHTMLAreasCollection *)this, IDispatch)
        QI_INHERITS(this, IDispatchEx)
        QI_TEAROFF(this, IHTMLAreasCollection2, NULL)

        default:
            if (iid == IID_IHTMLAreasCollection)
                *ppv = (IHTMLAreasCollection *)this;
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *) *ppv)->AddRef();

    return S_OK;
}



//+------------------------------------------------------------------------
//
//  Member:     get_length
//
//  Synopsis:   collection object model, defers to Cache Helper
//
//-------------------------------------------------------------------------

HRESULT
CAreasCollection::get_length(long * plSize)
{
    RRETURN(SetErrorInfo(_pCollectionCache->GetLength(_lIndex, plSize)));
}


//+-------------------------------------------------------------------------
//
//  Method:     CAreasCollection::putt_length
//
//  Synopsis:   Sets length (i.e. the number of entries). Truncates or
//              expands (by padding with dummy elements) the array as needed.
//
//--------------------------------------------------------------------------

HRESULT
CAreasCollection::put_length(long lLengthNew)
{
    HRESULT         hr = S_OK;
    long            l, lLengthOld;
    CAreaElement *  pArea;
    CMapElement *   pMap;
    CElement *      pElement = NULL;

    if (lLengthNew < 0)
    {
        hr =E_INVALIDARG;
        goto Cleanup;
    }

#ifdef WIN16
    // we store it as the original ptr and a DYNCAST just messes
    // things up so cast to void * and then back to what we want.
    pMap = (CMapElement *)(void *)_pCollectionCache->GetBase();
#else
    pMap = DYNCAST(CMapElement, _pCollectionCache->GetBase());
#endif
    if (!pMap)
    {
        hr = E_UNEXPECTED; 
        goto Cleanup;
    }

    lLengthOld = pMap->GetAreaCount();
    
    if (lLengthNew == lLengthOld)
        goto Cleanup;

    if (lLengthNew < lLengthOld)
    {
        // truncate the array
        for (l = lLengthOld-1; l >= lLengthNew; l--)
        {
            hr = THR(pMap->RemoveAreaHelper(l));
            if (hr)
                goto Cleanup;
        }
    }
    else
    {
        CDoc *  pDoc = pMap->Doc();

        // pad the array
        for (l = lLengthOld; l < lLengthNew; l++)
        {
            hr = THR(pDoc->CreateElement(ETAG_AREA, &pElement));
            if (hr)
                goto Cleanup;

            pArea = DYNCAST(CAreaElement, pElement);
            // insert the dummy element
            hr = THR(pMap->AddAreaHelper(pArea, l));
            if (hr)
                goto Cleanup;

            CElement::ClearPtr(&pElement);
        }
    }
Cleanup:
    CElement::ClearPtr(&pElement);
    
    RRETURN(SetErrorInfo(hr));
}


//+------------------------------------------------------------------------
//
//  Member:     item
//
//  Synopsis:   collection object model
//
//-------------------------------------------------------------------------

HRESULT
CAreasCollection::item(VARIANTARG var1, VARIANTARG var2, IDispatch** ppResult)
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
CAreasCollection::tags(VARIANT var1, IDispatch ** ppdisp)
{
    RRETURN(SetErrorInfo(_pCollectionCache->Tags(_lIndex, var1, ppdisp)));
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
CAreasCollection::urns(VARIANT var1, IDispatch ** ppdisp)
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
CAreasCollection::get__newEnum(IUnknown ** ppEnum)
{
    RRETURN(SetErrorInfo(_pCollectionCache->GetNewEnum(_lIndex, ppEnum)));
}


//+------------------------------------------------------------------------
//
//  Member:     Add
//
//  Synopsis:   Add item to collection...
//
//-------------------------------------------------------------------------
HRESULT
CAreasCollection::add(IHTMLElement * pIElement, VARIANT varIndex)
{
    HRESULT         hr;
    CMapElement *   pMap;
    CAreaElement *  pArea;
    long            lItemIndex;
    IUnknown *      pUnk = NULL;

    if (!pIElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Make sure this is an AREA element
    hr = THR(pIElement->QueryInterface(IID_IHTMLAreaElement, (void**)&pUnk));
    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(pUnk->QueryInterface(CLSID_CElement, (void**)&pArea));
    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Bail out if the element is already in the tree - #25130
    if (pArea->IsInMarkup())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

#ifdef WIN16
    // we store it as the original ptr and a DYNCAST just messes
    // things up so cast to void * and then back to what we want.
    pMap = (CMapElement *)(void *)_pCollectionCache->GetBase();
#else
    pMap = DYNCAST(CMapElement, _pCollectionCache->GetBase());
#endif
    if (!pMap)
    {
        hr = E_UNEXPECTED; 
        goto Cleanup;
    }

    hr = THR(VARIANTARGToIndex(&varIndex, &lItemIndex));
    if (hr)
        goto Cleanup;
    
    if (lItemIndex < -1 || lItemIndex > pMap->GetAreaCount())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(pMap->AddAreaHelper(pArea, lItemIndex));
    if (hr)
        goto Cleanup;
        
Cleanup:
    ReleaseInterface(pUnk);
    RRETURN (SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     remove
//
//  Synopsis:   remove the item in the collection at the given index 
//
//-------------------------------------------------------------------------

HRESULT
CAreasCollection::remove(long lItemIndex)
{
    HRESULT         hr;
    CMapElement *   pMap; 

#ifdef WIN16
    // we store it as the original ptr and a DYNCAST just messes
    // things up so cast to void * and then back to what we want.
    pMap = (CMapElement *)(void *)_pCollectionCache->GetBase();
#else
    pMap = DYNCAST(CMapElement, _pCollectionCache->GetBase());
#endif
    if (!pMap)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    if (lItemIndex < 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(pMap->RemoveAreaHelper(lItemIndex));
        
Cleanup:
    RRETURN (SetErrorInfo(hr));
}


