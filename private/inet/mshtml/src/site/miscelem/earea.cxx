//=-----------------------------------------------------------=
//
// File:        earea.cxx
//
// Contents:    Area element class
//
// Classes:     CAreaElement
//
//=-----------------------------------------------------------=


#include "headers.hxx"

#ifndef X_MATH_H_
#define X_MATH_H_
#include "math.h"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"   // For AnchorPropertyPage
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_EAREA_HXX_
#define X_EAREA_HXX_
#include "earea.hxx"
#endif

#ifndef X_IMGELEM_HXX_
#define X_IMGELEM_HXX_
#include "imgelem.hxx"
#endif

#ifndef X_CUTIL_HXX_
#define X_CUTIL_HXX_
#include "cutil.hxx"
#endif

#ifndef X_EMAP_HXX_
#define X_EMAP_HXX_
#include "emap.hxx"
#endif

#ifndef X_JSPROT_HXX_
#define X_JSPROT_HXX_
#include "jsprot.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#define _cxx_
#include "area.hdl"

MtDefine(CAreaElement, Elements, "CAreaElement")
MtDefine(CImgAreaStub, Elements, "CImgAreaStub")

ExternTag(tagMsoCommandTarget);

#ifndef NO_PROPERTY_PAGE
const CLSID * CAreaElement::s_apclsidPages[] =
{
    // Browse-time pages
    NULL,
    // Edit-time pages
#if DBG==1    
    &CLSID_CCDGenericPropertyPage,
#endif // DBG==1    
    NULL
};
#endif // NO_PROPERTY_PAGE


const CElement::CLASSDESC CAreaElement::s_classdesc =
{
    {
    &CLSID_HTMLAreaElement,             // _pclsid
    0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
    s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
    s_acpi,                             // _pcpi
    ELEMENTDESC_NOLAYOUT,               // _dwFlags
    &IID_IHTMLAreaElement,              // _piidDispinterface
    &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLAreaElement,   // _pfnTearOff
    NULL,                               // _pAccelsDesign
    NULL                                // _pAccelsRun
};


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      Creation and Initialization
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

//=-----------------------------------------------------------------------=
//
// Function:    CreateElement
//
// Synopsis:    Creates an instance of the given element's class
//
// Arguments:   CHtmTag *pht - struct for tag creation info.
//              CElement *pElementParent - The parent of the element
//              CElement **ppElement - Return ptr to element in *ppElement.
//
//=-----------------------------------------------------------------------=
HRESULT
CAreaElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    HRESULT hr = S_OK;

    Assert(pht->Is(ETAG_AREA));
    Assert(ppElement);

    *ppElement = new CAreaElement(pDoc);

    if (!*ppElement)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    
Cleanup:
    RRETURN(hr);
}

//=-----------------------------------------------------------------------=
//
// Function:    Init2()
//
// Synopsis:    Overridden to set netscape events (copied from
//              CAnchorElement::Init2()).
//
//=-----------------------------------------------------------------------=

HRESULT
CAreaElement::Init2(CInit2Context * pContext)
{
    HRESULT hr = S_OK;
    CDoc *  pDoc = Doc();

    Assert(pDoc);
    if (pDoc)
    {
        // Set default shape and parse the coords string if needed
        if (!_fShapeSet)
        {
            // No shape attribute is specified in the HTML
            _nShapeType = SHAPE_TYPE_RECT;
            _fShapeSet = TRUE;
            if (!_strCoords.IsNull())
            {
                hr = ParseCoords();
            }
        }

    }

    RRETURN(super::Init2(pContext));
}

//=-----------------------------------------------------------------------=
//
// Function:    GetcoordsHelper
//
// Synopsis:    Gets the coordinates attribute
//
// Arguments:   BSTR *bstrCOORDS - Pointer to BSTR for COORDS
//
//=-----------------------------------------------------------------------=
HRESULT
CAreaElement::GetcoordsHelper(CStr *pstrCOORDS)
{
    HRESULT hr = S_OK;
    TCHAR   achTemp[1024];
    int     c;
    int     nOffset;
    POINT  *ppt;
    BOOL    fFirst;

    Assert(pstrCOORDS);

    *achTemp = 0;
    switch(_nShapeType)
    {
    case SHAPE_TYPE_RECT:
        hr = THR(Format(0,
                &achTemp,
                ARRAY_SIZE(achTemp),
                _T("<0d>,<1d>,<2d>,<3d>"),
                (long)_coords.Rect.left,
                (long)_coords.Rect.top,
                (long)_coords.Rect.right,
                (long)_coords.Rect.bottom));
        if (hr)
            goto Cleanup;
        hr = THR(pstrCOORDS->Set(achTemp));
        break;

    case SHAPE_TYPE_CIRCLE:
        hr = THR(Format(0,
                &achTemp,
                ARRAY_SIZE(achTemp),
                _T("<0d>,<1d>,<2d>"),
                (long)_coords.Circle.lx,
                (long)_coords.Circle.ly,
                (long)_coords.Circle.lradius));
        if (hr)
            goto Cleanup;
        hr = THR(pstrCOORDS->Set(achTemp));
        break;

    case SHAPE_TYPE_POLY:
        hr = THR(pstrCOORDS->Set(_T("")));
        if (hr)
            goto Cleanup;
        fFirst = TRUE;
        for(c = _ptList.Size(), ppt = _ptList; c > 0; c--, ppt++)
        {
            if (fFirst)
            {
                fFirst = FALSE;
                nOffset = 0;
            }
            else
            {
                achTemp[0] = _T(',');
                nOffset = 1;
            }
            hr = THR(Format(0,
                    &(achTemp[nOffset]),
                    ARRAY_SIZE(achTemp)-nOffset,
                    _T("<0d>,<1d>"),
                    ppt->x,
                    ppt->y));
            if (hr)
                goto Cleanup;
            hr = THR(pstrCOORDS->Append(achTemp));
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN ( hr );
}

//=-----------------------------------------------------------------------=
//
// Function:    SetcoordsHelper
//
// Synopsis:    Sets the coordinates attribute
//
// Arguments:   CStr pstrCOORDS - String containing COORDS
//
//=-----------------------------------------------------------------------=
HRESULT
CAreaElement::SetcoordsHelper(CStr *pstrCOORDS)
{
    if (!pstrCOORDS->Length())
        RRETURN(S_OK);

    // Copy the buffer for tokenizing
    _strCoords.Set(*pstrCOORDS);

    // Don;t parse this yet if we don't know the shape
    if (!_fShapeSet)
    {
        RRETURN(S_OK);
    }
    RRETURN(ParseCoords());
}

//=-----------------------------------------------------------------------=
//
// Function:    ParseCoords
//
// Synopsis:    Parses the input string and sets the coordinates attribute.
//
//=-----------------------------------------------------------------------=
HRESULT
CAreaElement::ParseCoords()
{
    POINT pt;
    TCHAR *pch;

    Assert(_fShapeSet);
    Assert(_strCoords.Length());

    //
    // Grab the first token.  If _tcstok returns NULL,
    // we want to keep processing, because this means they
    // gave us an empty coordinate string.  Right now,
    // missing values are set to 0.
    //

    pch = _tcstok(_strCoords, DELIMS);

    switch(_nShapeType)
    {
    case SHAPE_TYPE_RECT:
        NextNum(&_coords.Rect.left, &pch);
        NextNum(&_coords.Rect.top, &pch);
        NextNum(&_coords.Rect.right, &pch);
        NextNum(&_coords.Rect.bottom, &pch);

        UpdateRectangle();
        break;

    case SHAPE_TYPE_CIRCLE:
        NextNum(&_coords.Circle.lx, &pch);
        NextNum(&_coords.Circle.ly, &pch);
        NextNum(&_coords.Circle.lradius, &pch);

        break;

    case SHAPE_TYPE_POLY:
        if(_ptList.Size())
        {
            _ptList.DeleteMultiple(0, _ptList.Size() - 1);
        }
        while(pch)
        {
            NextNum(&pt.x, &pch);
            NextNum(&pt.y, &pch);
           _ptList.AppendIndirect(&pt);
        }
        if (_ptList.Size())
        {
            // We don't store the same point as first and last
            if((_ptList[0].x == _ptList[_ptList.Size() - 1].x) &&
               (_ptList[0].y == _ptList[_ptList.Size() - 1].y))
            {
                _ptList.Delete(_ptList.Size() - 1);
            }
        }
        UpdatePolygon();
        break;
    }

    _strCoords.Free();
    RRETURN(S_OK);
}


//=-----------------------------------------------------------------------=
//
// Function:    GetshapeHelper
//
// Synopsis:    Gets the Shape attribute
//
// Arguments:   BSTR *bstrSHAPE - Pointer to BSTR for SHAPE
//
//=-----------------------------------------------------------------------=
HRESULT
CAreaElement::GetshapeHelper(CStr *pstrSHAPE)
{
    HRESULT hr = S_OK;
    Assert(pstrSHAPE);

    switch(_nShapeType)
    {
    case SHAPE_TYPE_RECT:
        hr = THR ( pstrSHAPE -> Set (_T("RECT")) );
        break;

    case SHAPE_TYPE_CIRCLE:
        hr = THR ( pstrSHAPE -> Set (_T("CIRCLE")) );
        break;

    case SHAPE_TYPE_POLY:
        hr = THR ( pstrSHAPE -> Set (_T("POLY")) );
        break;
    }


    RRETURN ( hr );
}


//=-----------------------------------------------------------------------=
//
// Function:    SetshapeHelper
//
// Synopsis:    Sets the shape attribute
//
// Arguments:   BSTR bstrSHAPE - BSTR containing SHAPE
//
//=-----------------------------------------------------------------------=
HRESULT
CAreaElement::SetshapeHelper(CStr *pstrSHAPE)
{
    HRESULT hr = S_OK;

    if (!*pstrSHAPE)
    {
        _nShapeType = SHAPE_TYPE_RECT;
    }
    else if (!StrCmpIC(*pstrSHAPE, _T("CIRC")) ||
         !StrCmpIC(*pstrSHAPE, _T("CIRCLE")))
    {
        _nShapeType = SHAPE_TYPE_CIRCLE;
    }
    else if (!StrCmpIC(*pstrSHAPE, _T("POLY")) ||
         !StrCmpIC(*pstrSHAPE, _T("POLYGON")))
    {
        _nShapeType = SHAPE_TYPE_POLY;
    }
    else
    {
        _nShapeType = SHAPE_TYPE_RECT;
    }

    // Parse the coords string if needed
    if (!_fShapeSet)
    {
        _fShapeSet = TRUE;
        if (!_strCoords.IsNull())
        {
            hr = ParseCoords();
        }
    }


    RRETURN(hr);
}



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      End Creation and Initialization
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      Destructor
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

void CAreaElement::Passivate()
{
    if(_nShapeType == SHAPE_TYPE_POLY && _coords.Polygon.hPoly)
    {
        DeleteObject(_coords.Polygon.hPoly);
        _coords.Polygon.hPoly = NULL;
    }

    super::Passivate();
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      End Destructor
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      Drawing related Code
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////



//=-----------------------------------------------------------------------=
//
// Function:    Draw
//
// Synopsis:    Performs the drawing of the Area, given a DC
//
// Arguments:   CFormDrawInfo * pDI - draw info
//              CElement *      pImg- Associated image
//
//=-----------------------------------------------------------------------=

HRESULT
CAreaElement::Draw(CFormDrawInfo * pDI, CElement * pImg)
{
    CRect   rcFocus, rcImg;
    LONG    xOff, yOff;
    HDC     hdc  = pDI->GetDC(TRUE);

    Assert(pImg);
    Assert(pImg->Doc());
    Assert(pImg->Doc()->_pInPlace);

    DYNCAST(CImgElement, pImg)->_pImage->GetRectImg(&rcImg);
    xOff = rcImg.left;
    yOff = rcImg.top;

    // Should not come here in browse mode
    Assert(pImg->IsEditable(TRUE));

    // BUGBUG (MohanB) Use functions in shape.cxx (like DrawPoly)

    switch(_nShapeType)
    {
    case SHAPE_TYPE_RECT:
        rcFocus.left    = _coords.Rect.left     + xOff;
        rcFocus.top     = _coords.Rect.top      + yOff;
        rcFocus.right   = _coords.Rect.right    + xOff;
        rcFocus.bottom  = _coords.Rect.bottom   + yOff;
        Rectangle(hdc, rcFocus.left, rcFocus.top,
            rcFocus.right, rcFocus.bottom);
        break;

    case SHAPE_TYPE_CIRCLE:
        Ellipse(hdc,
            _coords.Circle.lx - _coords.Circle.lradius + xOff,
            _coords.Circle.ly - _coords.Circle.lradius + yOff,
            _coords.Circle.lx + _coords.Circle.lradius + xOff,
            _coords.Circle.ly + _coords.Circle.lradius + yOff);

        break;

    case SHAPE_TYPE_POLY:
        POINT *ppt;
        UINT c;

        // Do we have enough points to draw a polygon ?
        if (_ptList.Size() < 2)
            break;

        MoveToEx(hdc, _ptList[0].x + xOff, _ptList[0].y + yOff, (POINT *)NULL);
        for(c = _ptList.Size(), ppt = &(_ptList[1]);
            c > 1;                  // c > 1, because we MoveTo'd the first pt
            ppt++, c--)
        {
            LineTo(hdc, ppt->x + xOff, ppt->y + yOff);
        }
        //
        // If there are only 2 points in the polygon, we don't want to draw
        // the same line twice and end up with nothing!
        //

        if(_ptList.Size() != 2)
        {
            LineTo(hdc, _ptList[0].x + xOff, _ptList[0].y + yOff);

        }
        break;

    default:
        Assert(FALSE && "Invalid Shape");

        break;
    }

    return S_OK;
}

//=-----------------------------------------------------------------------=
//
// Function:    GetBoundingRect
//
// Synopsis:    Returns the bounding rectangle for the area.
//
//=-----------------------------------------------------------------------=
void
CAreaElement::GetBoundingRect(RECT *prc)
{
    int i;

    // BUGBUG (MohanB) Use functions in shape.cxx (like GetPolyBoundingRect)

    switch(_nShapeType)
    {
    case SHAPE_TYPE_RECT:
        *prc = _coords.Rect;
        break;
    case SHAPE_TYPE_CIRCLE:
        prc->left = _coords.Circle.lx - _coords.Circle.lradius;
        prc->top = _coords.Circle.ly - _coords.Circle.lradius;
        prc->right = _coords.Circle.lx + _coords.Circle.lradius;
        prc->bottom = _coords.Circle.ly + _coords.Circle.lradius;
        break;
    case SHAPE_TYPE_POLY:
        if (_ptList.Size() == 0)
            break;
        prc->left = prc->right = _ptList[0].x;
        prc->top = prc->bottom = _ptList[0].y;
        for (i = _ptList.Size() - 1; i > 0; i--)
        {
            if (_ptList[i].x < prc->left)
            {
                prc->left = _ptList[i].x;
            }
            else if (_ptList[i].x > prc->right)
            {
                prc->right = _ptList[i].x;
            }
            if (_ptList[i].y < prc->top)
            {
                prc->top = _ptList[i].y;
            }
            else if (_ptList[i].y > prc->bottom)
            {
                prc->bottom = _ptList[i].y;
            }
        }
        break;
    }
}


//=-----------------------------------------------------------------------=
//
// Function:    UpdatePolygon
//
// Synopsis:    Updates the internal polygon region.
//
// Notes:       Should only be called if the region is actually a
//              SHAPE_TYPE_POLY.
//
//=-----------------------------------------------------------------------=
HRESULT
CAreaElement::UpdatePolygon()
{
    HRGN hNew;

    Assert(_nShapeType == SHAPE_TYPE_POLY);

    hNew = CreatePolygonRgn(_ptList, _ptList.Size(), ALTERNATE);

    if(hNew == NULL)
    {
        return E_OUTOFMEMORY;
    }
    else
    {
        if (_coords.Polygon.hPoly)
            DeleteObject(_coords.Polygon.hPoly);
        _coords.Polygon.hPoly = hNew;

        return S_OK;
    }
}


//=-----------------------------------------------------------------------=
//
// Function:    UpdateRectangle
//
// Synopsis:    Updates the rectangle coordinates.
//
// Notes:       Should only be called if the region is actually a
//              SHAPE_TYPE_RECT.
//
//=-----------------------------------------------------------------------=
HRESULT
CAreaElement::UpdateRectangle()
{
    LONG ltemp;

    Assert(_nShapeType == SHAPE_TYPE_RECT);

    if(_coords.Rect.left > _coords.Rect.right)
    {
        ltemp = _coords.Rect.left;
        _coords.Rect.left = _coords.Rect.right;
        _coords.Rect.right = ltemp;
    }

    if(_coords.Rect.top > _coords.Rect.bottom)
    {
        ltemp = _coords.Rect.top;
        _coords.Rect.top = _coords.Rect.bottom;
        _coords.Rect.bottom = ltemp;
    }

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//      End Modification/Update Code
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Helpers

HRESULT
CAreaElement::InsertIntoElemTree ( CMapElement * pMap, long lItemIndex )
{
    HRESULT    hr = S_OK;
    CTreeNode *pNodeAdjacentTo;
    Where      adjacencyHow;

    if (lItemIndex <= 0)
    {
        pNodeAdjacentTo = pMap->GetFirstBranch();
        adjacencyHow = AfterBegin;
    }
    else
    {
        CChildIterator ci(pMap);
        CTreeNode * pNode;
        LONG lIndex = 0;

        pNodeAdjacentTo = NULL;
        adjacencyHow = AfterEnd;

        while ((pNode = ci.NextChild()) != NULL)
        {
            if (pNode->Tag() == ETAG_AREA)
            {
                if (lItemIndex-1 == lIndex)
                {
                    if (ci.NextChild())
                    {
                        pNodeAdjacentTo = pNode;
                    }
                    break;
                }

                lIndex += 1;
            }
        }

        if (pNodeAdjacentTo == NULL)
        {
            pNodeAdjacentTo = pMap->GetFirstBranch();
            adjacencyHow = BeforeEnd;
        }
    }

    Assert(pNodeAdjacentTo);

    hr = THR( pNodeAdjacentTo->Element()->InsertAdjacent( adjacencyHow, this ) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}

HRESULT
CAreaElement::RemoveFromElemTree()
{
    HRESULT hr;

    hr = THR( RemoveOuter() );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}


HRESULT
CAreaElement::focus()
{
    HRESULT hr = S_OK;
    CNotification   nf;
    CMarkup *       pMarkup = GetMarkup();

    // we could have been created and not added to the tree yet
    if (!pMarkup || !pMarkup->GetElementClient())
        goto Cleanup;

    nf.AreaFocus(pMarkup->GetElementClient(), this);
    pMarkup->Notify(&nf);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CAreaElement::blur()
{
    HRESULT         hr = S_OK;
    CImgElement *   pImg;
    long            lIndex;
    CDoc *          pDoc = Doc();
    
    Assert(pDoc);

    // don't blur if the img is not current, this area is not the _pArea
    // or the frame in which this object is, does not currently have the focus
    
    // CHROME
    // If Chrome hosted there is no valid HWND so ::GetFocus() cannot be used.
    // Instead use CServer::GetFocus() which handles the windowless case
    if (!pDoc->IsChromeHosted())
    {
        if (!pDoc->_pInPlace ||
            ::GetFocus() != pDoc->_pInPlace->_hwnd ||
            pDoc->_pElemCurrent->Tag() != ETAG_IMG)
            goto Cleanup;
    }
    else
    {
        if (!pDoc->_pInPlace ||
            !pDoc->GetFocus() ||
            pDoc->_pElemCurrent->Tag() != ETAG_IMG)
            goto Cleanup;
    }

    //
    // Search for this area in the current element's map.
    //

    pImg = DYNCAST(CImgElement, pDoc->_pElemCurrent);
    pImg->EnsureMap();
    if (!pImg->GetMap())
        goto Cleanup;

    if (!OK(pImg->GetMap()->SearchArea(this, &lIndex)))
        goto Cleanup;
    
    // make the body the current site. Become current handles all the event firing
    hr = THR(pImg->blur());

Cleanup:
    RRETURN(hr);
}

void
CAreaElement::Notify(CNotification *pNF)
{
    super::Notify(pNF);
    switch (pNF->Type())
    {
    case NTYPE_BASE_URL_CHANGE: 
        _fBaseUrlChanged = TRUE;
        OnPropertyChange( DISPID_CAreaElement_href, ((PROPERTYDESC *)&s_propdescCAreaElementhref)->GetdwFlags());
        break;
    case NTYPE_ELEMENT_QUERYMNEMONICTARGET:
        {
            FOCUS_ITEM          fi;
            CElement *          pElem       = NULL;
            CImgElement *       pImg        = NULL;
            int                 i, c;
            CCollectionCache*   pCollectionCache;

            fi.pElement = NULL;
            fi.lSubDivision = 0;

            if (!IsInMarkup())
                goto CleanupGetTarget;

            // Search the document's collection for the first image that has this AREA as
            // a subdivision.
            if (S_OK != THR(GetMarkup()->EnsureCollectionCache(CMarkup::IMAGES_COLLECTION)))
                goto CleanupGetTarget;

            pCollectionCache = GetMarkup()->CollectionCache();

            // get size of collection
            c = pCollectionCache->SizeAry(CMarkup::IMAGES_COLLECTION);

            for (i = 0; i < c; i++)
            {
                if (S_OK != THR(pCollectionCache->GetIntoAry(CMarkup::IMAGES_COLLECTION, i, &pElem)))
                    goto CleanupGetTarget;

                if (pElem->Tag() != ETAG_IMG)
                    continue;

                pImg = DYNCAST(CImgElement, pElem);
                if (!pImg->EnsureAndGetMap())
                    continue;

                if (OK(pImg->GetMap()->SearchArea(this, &fi.lSubDivision)))
                {
                    fi.pElement = pImg;
                    break;
                }
            }
        CleanupGetTarget:
            *(FOCUS_ITEM *)pNF->DataAsPtr() = fi;
        }
        break;
    }
}

HRESULT
CAreaElement::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    switch (dispid)
    {
    case DISPID_CAreaElement_href:
        _fOMSetHasOccurred = !_fBaseUrlChanged;
        _fBaseUrlChanged = FALSE;
        break;

    case DISPID_CElement_tabIndex:
        {
            CNotification   nf;
            CMarkup *       pMarkup = GetMarkup();

            // we could have been created and not added to the tree yet
            if (!pMarkup || !pMarkup->GetElementClient())
                break;

            nf.AreaTabindexChange(pMarkup->GetElementClient(), this);
            pMarkup->Notify(&nf);
        }
        break;
    }
    
    return super::OnPropertyChange( dispid, dwFlags );
}


// URL accessors - CHyperlink overrides

HRESULT
CAreaElement::SetUrl(BSTR bstrUrl)
{
    return (s_propdescCAreaElementhref.b.SetUrlProperty(bstrUrl,
                this,
                (CVoid *)(void *)GetAttrArray()));
}


LPCTSTR
CAreaElement::GetUrl() const
{
    return GetAAhref();
}


LPCTSTR
CAreaElement::GetTarget() const
{
    return GetAAtarget();
}


HRESULT
CAreaElement::GetUrlTitle(CStr *pstr)
{
    pstr->Set(GetAAalt());
    if (pstr->Length() == 0)
        pstr->Set(GetAAtitle());
    return S_OK;
}


HRESULT
CAreaElement::ClickAction (CMessage *pmsg)
{
    HRESULT hr = super::ClickAction(pmsg);

    // Do not want this to bubble, so..
    if (S_FALSE == hr)
        hr = S_OK;

    RRETURN(hr);
}



