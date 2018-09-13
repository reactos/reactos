//+---------------------------------------------------------------------
//
//   File:      csite.cxx
//
//  Contents:   client-site object for forms kernel
//
//  Classes:    CSite (partial)
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_SELECOBJ_HXX_
#define X_SELECOBJ_HXX_
#include "selecobj.hxx"
#endif

#ifndef X_DOBJ_HXX_
#define X_DOBJ_HXX_
#include "dobj.hxx"
#endif

#ifndef X_RTFTOHTM_HXX_
#define X_RTFTOHTM_HXX_
#include "rtftohtm.hxx"
#endif

#ifndef X_FPRINT_HXX_
#define X_FPRINT_HXX_
#include "fprint.hxx"
#endif

#ifndef X_EFORM_HXX_
#define X_EFORM_HXX_
#include "eform.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_FILTCOL_HXX_
#define X_FILTCOL_HXX_
#include "filtcol.hxx"
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"       // table layout
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef X_OLEDLG_H_
#define X_OLEDLG_H_
#include <oledlg.h>
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"   // needed for EVENTPARAM
#endif

#ifndef X_CUTIL_HXX_
#define X_CUTIL_HXX_
#include "cutil.hxx"
#endif

#ifndef X_OCMM_H_
#define X_OCMM_H_
#include "ocmm.h"
#endif

#ifndef X_FILTER_HXX_
#define X_FILTER_HXX_
#include "filter.hxx"
#endif

#ifndef X_OLEDBERR_H_
#define X_OLEDBERR_H_
#include <oledberr.h>                   // for DB_E_DELETEDROW
#endif

#ifndef X_DISPDEFS_HXX_
#define X_DISPDEFS_HXX_
#include "dispdefs.hxx"
#endif

#define _cxx_
#include "csite.hdl"

ExternTag(tagFormP);
ExternTag(tagMsoCommandTarget);
PerfDbgExtern(tagDocPaint)
DeclareTag(tagLayout, "Layout", "Trace SetPosition/RequestLayout");
DeclareTag(tagLayoutNoShort, "LayoutNoShort", "Prevent RequestLayout short-circuit");

extern "C" const IID IID_IControl;

//+---------------------------------------------------------------------------
//
//  Function:   CompareElementsByZIndex
//
//  Synopsis:   Comparison function used by qsort to compare the zIndex of
//              two elements.
//
//----------------------------------------------------------------------------

#define ELEMENT1_ONTOP 1
#define ELEMENT2_ONTOP -1
#define ELEMENTS_EQUAL 0

int RTCCONV
CompareElementsByZIndex ( const void * pv1, const void * pv2 )
{
    int        i, z1, z2;
    HWND       hwnd1, hwnd2;

    CElement * pElement1 = * (CElement **) pv1;
    CElement * pElement2 = * (CElement **) pv2;

    //
    // Only compare elements which have the same ZParent
    // BUGBUG: For now, since table elements (e.g., TDs, TRs, CAPTIONs) cannot be
    //         positioned, it is Ok if they all end up in the same list - even if
    //         their ZParent-age is different.
    //         THIS MUST BE RE-VISITED ONCE WE SUPPORT POSITIONING ON TABLE ELEMENTS.
    //         (brendand)
    //
    Assert(     pElement1->GetFirstBranch()->ZParent() == pElement2->GetFirstBranch()->ZParent()
           ||   (   pElement1->GetFirstBranch()->ZParent()->Tag() == ETAG_TR
                &&  pElement2->GetFirstBranch()->ZParent()->Tag() == ETAG_TR)
           ||   (   pElement1->GetFirstBranch()->ZParent()->Tag() == ETAG_TABLE
                &&  pElement2->GetFirstBranch()->ZParent()->Tag() == ETAG_TR)
           ||   (   pElement2->GetFirstBranch()->ZParent()->Tag() == ETAG_TABLE
                &&  pElement1->GetFirstBranch()->ZParent()->Tag() == ETAG_TR));

    // Sites with windows are _always_ above sites without windows.

    hwnd1 = pElement1->GetHwnd();
    hwnd2 = pElement2->GetHwnd();

    if ((hwnd1 == NULL) != (hwnd2 == NULL))
    {
        return (hwnd1 != NULL) ? ELEMENT1_ONTOP : ELEMENT2_ONTOP;
    }

    //
    // If one element contains the other, then the containee is on top.
    //
    // Since table cells cannot be positioned, we ignore any case where
    // something is contained inside a table cell. That way they essentially
    // become 'peers' of the cells and can be positioned above or below them.
    //
    // BUGBUG (lylec) -- These elements are already scoped, so there's a
    // possibility we might walk up the wrong tree! This should be fixed
    // when Eric's proposed change is made regarding using Tree Nodes
    // instead of elements in the tree.
    //
    if (pElement1->Tag() != ETAG_TD && pElement2->Tag() != ETAG_TD &&   // Cell
        pElement1->Tag() != ETAG_TH && pElement2->Tag() != ETAG_TH &&   // Header
        pElement1->Tag() != ETAG_TC && pElement2->Tag() != ETAG_TC)     // Caption
    {
        if (pElement1->GetFirstBranch()->SearchBranchToRootForScope(pElement2))
        {
            return ELEMENT1_ONTOP;
        }

        if (pElement2->GetFirstBranch()->SearchBranchToRootForScope(pElement1))
        {
            return ELEMENT2_ONTOP;
        }
    }

    //
    // Only pay attention to the z-index attribute if the element is positioned
    //
    // The higher z-index is on top, which means the higher z-index value
    // is "greater".
    //

    z1 = !pElement1->IsPositionStatic()
              ? pElement1->GetFirstBranch()->GetCascadedzIndex()
              : 0;

    z2 = !pElement2->IsPositionStatic()
              ? pElement2->GetFirstBranch()->GetCascadedzIndex()
              : 0;

    i = z1 - z2;

    if (i == ELEMENTS_EQUAL &&
        pElement1->IsPositionStatic() != pElement2->IsPositionStatic())
    {
        //
        // The non-static element has a z-index of 0, so we must make
        // sure it stays above anything in the flow (static).
        //
        i = (!pElement1->IsPositionStatic()) ? ELEMENT1_ONTOP : ELEMENT2_ONTOP;
    }

    //
    // Make sure that the source indices are up to date before accessing them
    //

    Assert( pElement1->Doc() == pElement2->Doc() );

    //
    // If the zindex is the same, then sort by source order.
    //
    // Later in the source is on top, which means the higher source-index
    // value is "greater".
    //

    if (i == ELEMENTS_EQUAL)
    {
        i = pElement1->GetSourceIndex() - pElement2->GetSourceIndex();
    }

    // Different elements should never be exactly equal.
    //
    // If this assert fires it's likely due to the element collection not
    // having been built yet.
    //

    Assert( i != ELEMENTS_EQUAL || pElement1 == pElement2 );

    return i;
}

//+---------------------------------------------------------------
//
//  Member:     CSite::CSite
//
//  Synopsis:   Normal constructor.
//
//  Arguments:  pParent  Site that's our parent
//
//---------------------------------------------------------------

CSite::CSite(ELEMENT_TAG etag, CDoc *pDoc)
    : CElement(etag, pDoc)
{
    TraceTag((tagCDoc, "constructing CSite"));

#ifdef WIN16
    m_baseOffset = ((BYTE *) (void *) (CBase *)this) - ((BYTE *) this);
    m_ElementOffset = ((BYTE *) (void *) (CElement *)this) - ((BYTE *) this);
#endif

    // We only need to initialize non-zero state because of our redefinition
    // of operator new.

    _fSite           = TRUE;
}


HRESULT
CSite::Init()
{
    HRESULT hr;

    hr = THR( super::Init() );

    if (hr)
        goto Cleanup;

    // Root's layout is created, if required, outside Init().
    if (Tag() != ETAG_ROOT)
    {
        CreateLayout();
    }

    Assert( _fLayoutAlwaysValid );

Cleanup:

    RRETURN( hr );
}

//+------------------------------------------------------------------------
//
//  Member:     CSite::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
CSite::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT hr;

    *ppv = NULL;

    // IE4 shipped the interface IHTMLControlElement with the same GUID as
    // IControl.  Unfortunately, IControl is a forms^3 interface, which is bad.
    // To resolve this problem Trident's GUID for IHTMLControlElement has
    // changed however, the old GUID remembered in the QI for CSite to return
    // IHTMLControlElement.  The only side affect is that using the old GUID
    // will not marshall the interface correctly only the new GUID has the
    // correct marshalling code.  So, the solution is that QI'ing for
    // IID_IControl or IID_IHTMLControlElement will return IHTMLControlElement.

    // For VB page designer we need to emulate IE4 behavior (fail the QI if not a site)
    if(iid == IID_IControl && Doc()->_fVB && !HasLayout())
        RRETURN(E_NOINTERFACE);

    if (iid == IID_IHTMLControlElement || iid == IID_IControl)
    {
        hr = CreateTearOffThunk(this,
                                s_apfnpdIHTMLControlElement,
                                NULL,
                                ppv,
                                (void *)s_ppropdescsInVtblOrderIHTMLControlElement);
        if (hr)
            RRETURN(hr);
    }
    else
    {
        RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    (*(IUnknown **)ppv)->AddRef();
    return S_OK;
}


void
GetBorderColorInfoHelper(
    CTreeNode *     pNodeContext,
    CDocInfo *      pdci,
    CBorderInfo *   pborderinfo)
{
    const CFancyFormat *pFF = pNodeContext->GetFancyFormat();
    const CCharFormat  *pCF = pNodeContext->GetCharFormat();
    int i;
    COLORREF clr, clrHilight, clrLight, clrDark, clrShadow;

    for ( i=0; i<4 ; i++ )
    {
        BOOL  fNeedSysColor = FALSE;
        // Get the base color
        if ( !pFF->_ccvBorderColors[i].IsDefined() )
        {
            clr = pCF->_ccvTextColor.GetColorRef();
            fNeedSysColor = TRUE;
        }
        else
            clr = pFF->_ccvBorderColors[i].GetColorRef();

        // Set up the flat color
        pborderinfo->acrColors[i][1] = clr;

        // Set up the inner and outer colors
        switch ( pborderinfo->abStyles[i] )
        {
        case fmBorderStyleNone:
        case fmBorderStyleDouble:
        case fmBorderStyleSingle:
        case fmBorderStyleDotted:
        case fmBorderStyleDashed:
            pborderinfo->acrColors[i][0] =
            pborderinfo->acrColors[i][2] = clr;
            // Don't need inner/outer colors
            break;

        default:
            {
                // Set up the color variations
                if ( pFF->_ccvBorderColorHilight.IsDefined() && !ISBORDERSIDECLRSETUNIQUE( pFF, i ) )
                    clrHilight = pFF->_ccvBorderColorHilight.GetColorRef();
                else
                {
                    if (fNeedSysColor)
                    {
                        clrHilight = GetSysColorQuick(COLOR_BTNHIGHLIGHT);
                    }
                    else
                        clrHilight = clr;
                }
                if ( pFF->_ccvBorderColorDark.IsDefined() && !ISBORDERSIDECLRSETUNIQUE( pFF, i ) )
                    clrDark = pFF->_ccvBorderColorDark.GetColorRef();
                else
                {
                    if (fNeedSysColor)
                    {
                        clrDark = GetSysColorQuick(COLOR_3DDKSHADOW);
                    }
                    else
                        clrDark = ( ( (clr & 0xff0000)>>1 ) & 0xff0000 ) |
                                  ( ( (clr & 0x00ff00)>>1 ) & 0x00ff00 ) |
                                  ( ( (clr & 0x0000ff)>>1 ) & 0x0000ff );
                }
                if ( pFF->_ccvBorderColorShadow.IsDefined() && !ISBORDERSIDECLRSETUNIQUE( pFF, i ) )
                    clrShadow = pFF->_ccvBorderColorShadow.GetColorRef();
                else
                {
                    if (fNeedSysColor)
                    {
                        clrShadow = GetSysColorQuick(COLOR_BTNSHADOW);
                    }
                    else
                        clrShadow = ( ( (clr & 0xff0000)>>2 ) & 0xff0000 ) |
                                    ( ( (clr & 0x00ff00)>>2 ) & 0x00ff00 ) |
                                    ( ( (clr & 0x0000ff)>>2 ) & 0x0000ff );
                }

                // If the Light color isn't set synthesise a light color 3/4 of clr
                if ( pFF->_ccvBorderColorLight.IsDefined() && !ISBORDERSIDECLRSETUNIQUE( pFF, i ) )
                    clrLight = pFF->_ccvBorderColorLight.GetColorRef();
                else
                {
                    if (fNeedSysColor)
                    {
                        clrLight = GetSysColorQuick(COLOR_BTNFACE);
                    }
                    else
                        clrLight = clrShadow + clrDark;
                }

                if (i==BORDER_TOP || i==BORDER_LEFT)
                {   // Top/left edges
                    if ( pFF->_bBorderSoftEdges || (pborderinfo->wEdges & BF_SOFT))
                    {
                        switch ( pborderinfo->abStyles[i] )
                        {
                        case fmBorderStyleRaisedMono:
                            pborderinfo->acrColors[i][0] =
                            pborderinfo->acrColors[i][2] = clrHilight;
                            break;
                        case fmBorderStyleSunkenMono:
                            pborderinfo->acrColors[i][0] =
                            pborderinfo->acrColors[i][2] = clrDark;
                            break;
                        case fmBorderStyleRaised:
                            pborderinfo->acrColors[i][0] = clrHilight;
                            pborderinfo->acrColors[i][2] = clrLight;
                            break;
                        case fmBorderStyleSunken:
                            pborderinfo->acrColors[i][0] = clrDark;
                            pborderinfo->acrColors[i][2] = clrShadow;
                            break;
                        case fmBorderStyleEtched:
                            pborderinfo->acrColors[i][0] = clrDark;
                            pborderinfo->acrColors[i][2] = clrLight;
                            break;
                        case fmBorderStyleBump:
                            pborderinfo->acrColors[i][0] = clrHilight;
                            pborderinfo->acrColors[i][2] = clrShadow;
                            break;
                        }
                    }
                    else
                    {
                        switch ( pborderinfo->abStyles[i] )
                        {
                        case fmBorderStyleRaisedMono:
                            pborderinfo->acrColors[i][0] =
                            pborderinfo->acrColors[i][2] = clrLight;
                            break;
                        case fmBorderStyleSunkenMono:
                            pborderinfo->acrColors[i][0] =
                            pborderinfo->acrColors[i][2] = clrShadow;
                            break;
                        case fmBorderStyleRaised:
                            pborderinfo->acrColors[i][0] = clrLight;
                            pborderinfo->acrColors[i][2] = clrHilight;
                            break;
                        case fmBorderStyleSunken:
                            pborderinfo->acrColors[i][0] = clrShadow;
                            pborderinfo->acrColors[i][2] = clrDark;
                            break;
                        case fmBorderStyleEtched:
                            pborderinfo->acrColors[i][0] = clrShadow;
                            pborderinfo->acrColors[i][2] = clrHilight;
                            break;
                        case fmBorderStyleBump:
                            pborderinfo->acrColors[i][0] = clrLight;
                            pborderinfo->acrColors[i][2] = clrDark;
                            break;
                        }
                    }
                }
                else
                {   // Bottom/right edges
                    switch ( pborderinfo->abStyles[i] )
                    {
                    case fmBorderStyleRaisedMono:
                        pborderinfo->acrColors[i][0] =
                        pborderinfo->acrColors[i][2] = clrDark;
                        break;
                    case fmBorderStyleSunkenMono:
                        pborderinfo->acrColors[i][0] =
                        pborderinfo->acrColors[i][2] = clrHilight;
                        break;
                    case fmBorderStyleRaised:
                        pborderinfo->acrColors[i][0] = clrDark;
                        pborderinfo->acrColors[i][2] = clrShadow;
                        break;
                    case fmBorderStyleSunken:
                        pborderinfo->acrColors[i][0] = clrHilight;
                        pborderinfo->acrColors[i][2] = clrLight;
                        break;
                    case fmBorderStyleEtched:
                        pborderinfo->acrColors[i][0] = clrHilight;
                        pborderinfo->acrColors[i][2] = clrShadow;
                        break;
                    case fmBorderStyleBump:
                        pborderinfo->acrColors[i][0] = clrDark;
                        pborderinfo->acrColors[i][2] = clrLight;
                        break;
                    }
                }
            }
        }
    }

    if ( pFF->_bBorderSoftEdges )
        pborderinfo->wEdges |= BF_SOFT;
}

DWORD
GetBorderInfoHelper(
    CTreeNode *     pNodeContext,
    CDocInfo *      pdci,
    CBorderInfo *   pborderinfo,
    BOOL            fAll /*=FALSE*/)
{
    Assert(pNodeContext);
    Assert(pborderinfo);

    int   i;
    int   iBorderWidth = 0;
    const CFancyFormat *pFF = pNodeContext->GetFancyFormat();
    const CCharFormat  *pCF = pNodeContext->GetCharFormat();

    Assert( pFF && pCF );

    for (i = BORDER_TOP; i <= BORDER_LEFT; i++)
    {
        if ( pFF->_bBorderStyles[i] != (BYTE)-1 )
            pborderinfo->abStyles[i] = pFF->_bBorderStyles[i];
        if ( !pborderinfo->abStyles[i] )
        {
            pborderinfo->aiWidths[i] = 0;
            continue;
        }

        switch ( pFF->_cuvBorderWidths[i].GetUnitType() )
        {
        case CUnitValue::UNIT_NULLVALUE:
            continue;
        case CUnitValue::UNIT_ENUM:
            {   // Pick up the default border width here.
                CUnitValue cuv;
                cuv.SetValue( (pFF->_cuvBorderWidths[i].GetUnitValue() + 1) * 2, CUnitValue::UNIT_PIXELS );
                iBorderWidth = cuv.GetPixelValue(NULL,
                                                ((i==BORDER_TOP)||(i==BORDER_BOTTOM))
                                                        ? CUnitValue::DIRECTION_CY
                                                        : CUnitValue::DIRECTION_CX,
                                                0, pCF->_yHeight);
            }
            break;
        default:
            iBorderWidth = pFF->_cuvBorderWidths[i].GetPixelValue(NULL,
                                                            ((i==BORDER_TOP)||(i==BORDER_BOTTOM))
                                                                        ? CUnitValue::DIRECTION_CY
                                                                        : CUnitValue::DIRECTION_CX,
                                                            0, pCF->_yHeight);

            // If user sets tiny borderwidth, set smallest width possible (1px) instead of zero (IE5,5865).
            if (!iBorderWidth && pFF->_cuvBorderWidths[i].GetUnitValue() > 0)
                iBorderWidth = 1;
        }
        if (iBorderWidth >= 0)
        {
            pborderinfo->aiWidths[i] = iBorderWidth <= MAX_BORDER_SPACE? iBorderWidth : MAX_BORDER_SPACE;
        }
    }

    // Now pick up the edges if we set the border-style for that edge to "none"
    pborderinfo->wEdges &= ~BF_RECT;

    if ( pborderinfo->aiWidths[BORDER_TOP] )
        pborderinfo->wEdges |= BF_TOP;
    if ( pborderinfo->aiWidths[BORDER_RIGHT] )
        pborderinfo->wEdges |= BF_RIGHT;
    if ( pborderinfo->aiWidths[BORDER_BOTTOM] )
        pborderinfo->wEdges |= BF_BOTTOM;
    if ( pborderinfo->aiWidths[BORDER_LEFT] )
        pborderinfo->wEdges |= BF_LEFT;

    if ( fAll )
    {
        GetBorderColorInfoHelper(pNodeContext,pdci,pborderinfo);
    }

    if (    pborderinfo->wEdges
        ||  pborderinfo->rcSpace.top    > 0
        ||  pborderinfo->rcSpace.bottom > 0
        ||  pborderinfo->rcSpace.left   > 0
        ||  pborderinfo->rcSpace.right  > 0)
    {
        return (    pborderinfo->wEdges & BF_RECT
                &&  pborderinfo->aiWidths[BORDER_TOP]  == pborderinfo->aiWidths[BORDER_BOTTOM]
                &&  pborderinfo->aiWidths[BORDER_LEFT] == pborderinfo->aiWidths[BORDER_RIGHT]
                &&  pborderinfo->aiWidths[BORDER_TOP]  == pborderinfo->aiWidths[BORDER_LEFT]
                        ? DISPNODEBORDER_SIMPLE
                        : DISPNODEBORDER_COMPLEX);
    }
    return DISPNODEBORDER_NONE;
}

//+-------------------------------------------------------------------------
//
//  Function:   DrawBorder
//
//  Synopsis:   This routine is functionally equivalent to the Win95
//              DrawEdge API
//  xyFlat:     positive means, we draw flat inside, outside if negative
//
//--------------------------------------------------------------------------

#define DRAWBORDER_OUTER    0
#define DRAWBORDER_SPACE    1
#define DRAWBORDER_INNER    2
#define DRAWBORDER_TOTAL    3
#define DRAWBORDER_INCRE    4
#define DRAWBORDER_LAST     5

void DrawBorder(CFormDrawInfo *pDI,
        LPRECT lprc,
        CBorderInfo *pborderinfo)
{
    HDC         hdc = pDI->GetDC(TRUE);
    RECT        rc;
    RECT        rcSave; // Original rectangle, maybe adjusted for flat border.
    RECT        rcOrig; // Original rectangle of the element container.
    HBRUSH      hbrOld = NULL;
    COLORREF    crNow = COLORREF_NONE;
    COLORREF    crNew;
    HPEN        hPen;
    // Ordering of the next 2 arrays are top,right,bottom,left.
    BYTE        abCanUseJoin[4];
    int         aiNumBorderLines[4];
    int         aiLineWidths[4][DRAWBORDER_LAST];     // outer, spacer, inner
    UINT        wEdges = pborderinfo->wEdges;
    int         i,j;
    POINT       polygon[8];
    int         xyFlat = abs(pborderinfo->xyFlat);
    BOOL        fdrawCaption = pborderinfo->sizeCaption.cx || pborderinfo->sizeCaption.cy;
    BOOL        fContinuingPoly;
    SIZE        sizeLegend;
    SIZE        sizeRect;
    POINT       ptBrushOrg;
    
    // save legend size
    sizeLegend = pborderinfo->sizeCaption;

    Assert(pborderinfo);
    if (pborderinfo->fNotDraw)
        return;

    // offsetCaption is already transformed in GetBorderInfo
    rc.top    = lprc->top + pDI->DocPixelsFromWindowY(pborderinfo->rcSpace.top)
                             + pborderinfo->offsetCaption;
    rc.bottom = lprc->bottom
                    - pDI->DocPixelsFromWindowY(pborderinfo->rcSpace.bottom);
    rc.left   = lprc->left
                    + pDI->DocPixelsFromWindowX(pborderinfo->rcSpace.left);
    rc.right  = lprc->right
                    - pDI->DocPixelsFromWindowX(pborderinfo->rcSpace.right);

    rcOrig = rc;

    Assert(rc.left <= rc.right);
    Assert(rc.top <= rc.bottom);

    if (pborderinfo->xyFlat < 0)
    {
        InflateRect(&rc, pborderinfo->xyFlat, pborderinfo->xyFlat);
    }

    rcSave = rc;

    hPen = (HPEN)GetStockObject(NULL_PEN);
    SelectObject(hdc, hPen);

    for ( i = 0; i < 4; i++ )
    {
        aiLineWidths[i][DRAWBORDER_OUTER] = pborderinfo->aiWidths[i] - xyFlat;
        aiLineWidths[i][DRAWBORDER_TOTAL] = max(aiLineWidths[i][DRAWBORDER_OUTER], 1);
    }

    sizeRect.cx = rc.right - rc.left;
    sizeRect.cy = rc.bottom - rc.top;

    // set brush origin so that dither patterns are anchored correctly
    ::GetViewportOrgEx(hdc, &ptBrushOrg);
    ptBrushOrg.x += rc.left;
    ptBrushOrg.y += rc.top;

    // validate border size
    // if the broders are too big, truncate the bottom and right parts
    if (aiLineWidths[BORDER_TOP][DRAWBORDER_OUTER] +
        aiLineWidths[BORDER_BOTTOM][DRAWBORDER_OUTER] > sizeRect.cy)
    {
        aiLineWidths[BORDER_TOP][DRAWBORDER_OUTER] =
                        (aiLineWidths[BORDER_TOP][DRAWBORDER_OUTER] > sizeRect.cy) ?
                        sizeRect.cy :
                        aiLineWidths[BORDER_TOP][DRAWBORDER_OUTER];
        aiLineWidths[BORDER_BOTTOM][DRAWBORDER_OUTER] =
                                                sizeRect.cy -
                                                aiLineWidths[BORDER_TOP][DRAWBORDER_OUTER];
    }

    if (aiLineWidths[BORDER_RIGHT][DRAWBORDER_OUTER] +
        aiLineWidths[BORDER_LEFT][DRAWBORDER_OUTER] > sizeRect.cx)
    {
        aiLineWidths[BORDER_LEFT][DRAWBORDER_OUTER] =
                        (aiLineWidths[BORDER_LEFT][DRAWBORDER_OUTER] > sizeRect.cx) ?
                        sizeRect.cx :
                        aiLineWidths[BORDER_LEFT][DRAWBORDER_OUTER];
        aiLineWidths[BORDER_RIGHT][DRAWBORDER_OUTER] =
                                                sizeRect.cx -
                                                aiLineWidths[BORDER_LEFT][DRAWBORDER_OUTER];
    }

    for ( i = 0; i < 4; i++ )
    {
        aiLineWidths[i][DRAWBORDER_TOTAL] = max(aiLineWidths[i][DRAWBORDER_OUTER], 1);
    }

    for ( i = 0; i < 4; i++ )
    {
        switch ( pborderinfo->abStyles[i] )
        {
        case fmBorderStyleNone:
            aiLineWidths[i][DRAWBORDER_TOTAL] = 0;

        case fmBorderStyleSingle:
        case fmBorderStyleRaisedMono:
        case fmBorderStyleSunkenMono:
        case fmBorderStyleDotted:
        case fmBorderStyleDashed:
            aiNumBorderLines[i] = 1;
            aiLineWidths[i][DRAWBORDER_INNER] = 0;
            aiLineWidths[i][DRAWBORDER_SPACE] = 0;
            abCanUseJoin[i] = !!(aiLineWidths[i][DRAWBORDER_OUTER] > 0);
            break;

        case fmBorderStyleRaised:
        case fmBorderStyleSunken:
        case fmBorderStyleEtched:
        case fmBorderStyleBump:
            aiLineWidths[i][DRAWBORDER_SPACE] = 0; // Spacer
            aiNumBorderLines[i] = 3;
            aiLineWidths[i][DRAWBORDER_INNER] = aiLineWidths[i][DRAWBORDER_OUTER] >> 1;
            aiLineWidths[i][DRAWBORDER_OUTER] -= aiLineWidths[i][DRAWBORDER_INNER];
            abCanUseJoin[i] = !!(aiLineWidths[i][DRAWBORDER_TOTAL] > 1);
            break;

        case fmBorderStyleDouble:
            aiNumBorderLines[i] = 3;
            aiLineWidths[i][DRAWBORDER_SPACE] = aiLineWidths[i][DRAWBORDER_OUTER] / 3; // Spacer
            // If this were equal to the line above,
            aiLineWidths[i][DRAWBORDER_INNER] = (aiLineWidths[i][DRAWBORDER_OUTER]
                                                - aiLineWidths[i][DRAWBORDER_SPACE]) / 2;
            // we'd get widths of 3,1,1 instead of 2,1,2
            aiLineWidths[i][DRAWBORDER_OUTER] -= aiLineWidths[i][DRAWBORDER_SPACE]
                                                + aiLineWidths[i][DRAWBORDER_INNER];
            // evaluate border join
            // we don't want to have a joint polygon borders if
            // the distribution does not match
            abCanUseJoin[i] = pborderinfo->acrColors[(i==0)?3:(i-1)][0]
                                    == pborderinfo->acrColors[i][0]
                                && (aiLineWidths[i][DRAWBORDER_TOTAL] > 2)
                                && (aiLineWidths[(i==0)?3:(i-1)][DRAWBORDER_TOTAL] % 3)
                                    == (aiLineWidths[i][DRAWBORDER_TOTAL] % 3);
            break;
        }
        abCanUseJoin[i] = abCanUseJoin[i]
                            && pborderinfo->abStyles[(4 + i - 1) % 4]
                                == pborderinfo->abStyles[i];
        aiLineWidths[i][DRAWBORDER_INCRE] = 0;
    }

    // Loop: draw outer lines (j=0), spacer (j=1), inner lines (j=2)
    for ( j=DRAWBORDER_OUTER; j<=DRAWBORDER_INNER; j++ )
    {
        fContinuingPoly = FALSE;
        if ( j != DRAWBORDER_SPACE ) // if j==1, this line is a spacer only - don't draw lines, just deflate the rect.
        {
            i = 0;
            // We'll work around the border edges CW, starting with the right edge, in an attempt to reduce calls to polygon().
            // we must draw left first to follow Windows standard

            if ( wEdges & BF_LEFT && (j < aiNumBorderLines[BORDER_LEFT]) && aiLineWidths[BORDER_LEFT][j])
            {   // There's a left edge
                // get color brush for left side
                crNew = pborderinfo->acrColors[BORDER_LEFT][j];
                if (crNew != crNow)
                {
                    HBRUSH hbrNew;
                    SelectCachedBrush(hdc, crNew, &hbrNew, &hbrOld, &crNow);
#ifndef WINCE
                    // not supported on WINCE
                    ::UnrealizeObject(hbrNew);
#endif
                    ::SetBrushOrgEx(hdc, POSITIVE_MOD(ptBrushOrg.x,8), POSITIVE_MOD(ptBrushOrg.y,8), NULL);
                }
                
                // build left side polygon

                polygon[i].x = rc.left + aiLineWidths[BORDER_LEFT][j];    // lower right corner
                polygon[i++].y = rcSave.bottom - MulDivQuick(aiLineWidths[BORDER_BOTTOM][DRAWBORDER_TOTAL],
                                                    aiLineWidths[BORDER_LEFT][DRAWBORDER_INCRE]
                                                        + aiLineWidths[BORDER_LEFT][j],
                                                    aiLineWidths[BORDER_LEFT][DRAWBORDER_TOTAL]);
                polygon[i].x = rc.left;                         // lower left corner
                polygon[i++].y = rcSave.bottom - MulDivQuick(aiLineWidths[BORDER_BOTTOM][DRAWBORDER_TOTAL],
                                                    aiLineWidths[BORDER_LEFT][DRAWBORDER_INCRE],
                                                    aiLineWidths[BORDER_LEFT][DRAWBORDER_TOTAL]);

                if ( !(wEdges & BF_TOP) ||
                    ( pborderinfo->acrColors[BORDER_LEFT][j] != pborderinfo->acrColors[BORDER_TOP][j] )
                        || !abCanUseJoin[BORDER_TOP])
                {
                    polygon[i].x = rc.left;                         // upper left corner
                    polygon[i++].y = rcSave.top + MulDivQuick(aiLineWidths[BORDER_TOP][DRAWBORDER_TOTAL],
                                                    aiLineWidths[BORDER_LEFT][DRAWBORDER_INCRE],
                                                    aiLineWidths[BORDER_LEFT][DRAWBORDER_TOTAL]);
                    polygon[i].x = rc.left + aiLineWidths[BORDER_LEFT][j];    // upper right corner
                    polygon[i++].y = rcSave.top + MulDivQuick(aiLineWidths[BORDER_TOP][DRAWBORDER_TOTAL],
                                                    aiLineWidths[BORDER_LEFT][DRAWBORDER_INCRE]
                                                        + aiLineWidths[BORDER_LEFT][j],
                                                    aiLineWidths[BORDER_LEFT][DRAWBORDER_TOTAL]);
                    Polygon( hdc, polygon, i );
                    i = 0;
                }
                else
                    fContinuingPoly = TRUE;
            }
            if ( wEdges & BF_TOP && (j < aiNumBorderLines[BORDER_TOP]) && aiLineWidths[BORDER_TOP][j])
            {   // There's a top edge
                if ( !fContinuingPoly )
                {
                    // get color brush for top side
                    crNew = pborderinfo->acrColors[BORDER_TOP][j];
                    if (crNew != crNow)
                    {
                        HBRUSH hbrNew;
                        SelectCachedBrush(hdc, crNew, &hbrNew, &hbrOld, &crNow);
#ifndef WINCE
                        // not supported on WINCE
                        ::UnrealizeObject(hbrNew);
#endif
                        ::SetBrushOrgEx(hdc, POSITIVE_MOD(ptBrushOrg.x,8), POSITIVE_MOD(ptBrushOrg.y,8), NULL);
                        i = 0;
                    }
                }
                // build top side polygon

                // up left
                polygon[i].x    = rcSave.left + ((wEdges & BF_LEFT) ? 
                                                    MulDivQuick(aiLineWidths[BORDER_LEFT][DRAWBORDER_TOTAL],
                                                        aiLineWidths[BORDER_TOP][DRAWBORDER_INCRE],
                                                        aiLineWidths[BORDER_TOP][DRAWBORDER_TOTAL])
                                                    : 0);
                polygon[i++].y  = rc.top;

                if (fdrawCaption)
                {
                    // shrink legend
                    sizeLegend.cx = sizeLegend.cx
                                        - aiLineWidths[BORDER_LEFT][DRAWBORDER_INCRE];
                    sizeLegend.cy = sizeLegend.cy
                                        - aiLineWidths[BORDER_LEFT][DRAWBORDER_INCRE];

                    polygon[i].x    = rc.left + sizeLegend.cx;
                    polygon[i++].y  = rc.top;
                    polygon[i].x    = rc.left + sizeLegend.cx;
                    polygon[i++].y  = rc.top + aiLineWidths[BORDER_TOP][j];

                    polygon[i].x    = rcSave.left + ((wEdges & BF_LEFT) ? 
                                                        MulDivQuick(aiLineWidths[BORDER_LEFT][DRAWBORDER_TOTAL],
                                                            aiLineWidths[BORDER_TOP][DRAWBORDER_INCRE]
                                                                + aiLineWidths[BORDER_TOP][j],
                                                            aiLineWidths[BORDER_TOP][DRAWBORDER_TOTAL])
                                                        : 0);

                    polygon[i++].y  = rc.top + aiLineWidths[BORDER_TOP][j];

                    Polygon(hdc, polygon, i );
                    i = 0;
                    polygon[i].x    = rc.left + sizeLegend.cy;
                    polygon[i++].y  = rc.top + aiLineWidths[BORDER_TOP][j];
                    polygon[i].x    = rc.left + sizeLegend.cy;
                    polygon[i++].y  = rc.top;
                }

                // upper right
                polygon[i].x    = rcSave.right - ((wEdges & BF_RIGHT) ?
                                                    MulDivQuick(aiLineWidths[BORDER_RIGHT][DRAWBORDER_TOTAL],
                                                        aiLineWidths[BORDER_TOP][DRAWBORDER_INCRE],
                                                        aiLineWidths[BORDER_TOP][DRAWBORDER_TOTAL])
                                                    : 0);
                polygon[i++].y  = rc.top;

                // lower right
                polygon[i].x    = rcSave.right - ((wEdges & BF_RIGHT) ?
                                                    MulDivQuick(aiLineWidths[BORDER_RIGHT][DRAWBORDER_TOTAL],
                                                        aiLineWidths[BORDER_TOP][DRAWBORDER_INCRE]
                                                            + aiLineWidths[BORDER_TOP][j],
                                                        aiLineWidths[BORDER_TOP][DRAWBORDER_TOTAL])
                                                    : 0 );
                polygon[i++].y  = rc.top + aiLineWidths[BORDER_TOP][j];

                if (!fdrawCaption)
                {
                    polygon[i].x    = rcSave.left + MulDivQuick(aiLineWidths[BORDER_LEFT][DRAWBORDER_TOTAL],
                                                    aiLineWidths[BORDER_TOP][DRAWBORDER_INCRE]
                                                        + aiLineWidths[BORDER_TOP][j],
                                                    aiLineWidths[BORDER_TOP][DRAWBORDER_TOTAL]);
                    polygon[i++].y  = rc.top + aiLineWidths[BORDER_TOP][j];
                }

                Polygon(hdc, polygon, i );
                i = 0;
            }

            fContinuingPoly = FALSE;
            i = 0;

            if ( wEdges & BF_RIGHT && (j < aiNumBorderLines[BORDER_RIGHT]) && aiLineWidths[BORDER_RIGHT][j])
            {   // There's a right edge
                // get color brush for right side
                crNew = pborderinfo->acrColors[BORDER_RIGHT][j];
                if (crNew != crNow)
                {
                    HBRUSH hbrNew;
                    SelectCachedBrush(hdc, crNew, &hbrNew, &hbrOld, &crNow);
#ifndef WINCE
                    // not supported on WINCE
                    ::UnrealizeObject(hbrNew);
#endif
                    ::SetBrushOrgEx(hdc, POSITIVE_MOD(ptBrushOrg.x,8), POSITIVE_MOD(ptBrushOrg.y,8), NULL);
                }

                // build right side polygon

                polygon[i].x    = rc.right - aiLineWidths[BORDER_RIGHT][j];     // upper left corner
                polygon[i++].y  =  rcSave.top + MulDivQuick(aiLineWidths[BORDER_TOP][DRAWBORDER_TOTAL],
                                                    aiLineWidths[BORDER_RIGHT][DRAWBORDER_INCRE]
                                                        + aiLineWidths[BORDER_RIGHT][j],
                                                    aiLineWidths[BORDER_RIGHT][DRAWBORDER_TOTAL]);

                if (pborderinfo->abStyles[BORDER_RIGHT] == pborderinfo->abStyles[BORDER_TOP]
                        && aiLineWidths[BORDER_RIGHT][j] == 1)
                {
                    // upper right corner fix: we have to overlap one pixel to avoid holes
                    polygon[i].x    = rc.right - 1;
                    polygon[i].y    = rcSave.top + MulDivQuick(aiLineWidths[BORDER_TOP][DRAWBORDER_TOTAL],
                                                    aiLineWidths[BORDER_RIGHT][DRAWBORDER_INCRE],
                                                    aiLineWidths[BORDER_RIGHT][DRAWBORDER_TOTAL]);

                    polygon[i+1].x    = rc.right;
                    polygon[i+1].y  = polygon[i].y;

                    i = i + 2;
                }
                else
                {
                    polygon[i].x    = rc.right;
                    polygon[i++].y    = rcSave.top + MulDivQuick(aiLineWidths[BORDER_TOP][DRAWBORDER_TOTAL],
                                                    aiLineWidths[BORDER_RIGHT][DRAWBORDER_INCRE],
                                                    aiLineWidths[BORDER_RIGHT][DRAWBORDER_TOTAL]);
                }

                if ( !(wEdges & BF_BOTTOM) ||
                    ( pborderinfo->acrColors[BORDER_RIGHT][j] != pborderinfo->acrColors[BORDER_BOTTOM][j] )
                        || !abCanUseJoin[BORDER_BOTTOM])
                {
                    polygon[i].x    = rc.right;                                     // lower right corner
                    polygon[i++].y  = rcSave.bottom - MulDivQuick(aiLineWidths[BORDER_BOTTOM][DRAWBORDER_TOTAL],
                                                    aiLineWidths[BORDER_RIGHT][DRAWBORDER_INCRE],
                                                    aiLineWidths[BORDER_RIGHT][DRAWBORDER_TOTAL]);

                    polygon[i].x    = rc.right - aiLineWidths[BORDER_RIGHT][j];     // lower left corner
                    polygon[i++].y  = rcSave.bottom - MulDivQuick(aiLineWidths[BORDER_BOTTOM][DRAWBORDER_TOTAL],
                                                    aiLineWidths[BORDER_RIGHT][DRAWBORDER_INCRE]
                                                        + aiLineWidths[BORDER_RIGHT][j],
                                                    aiLineWidths[BORDER_RIGHT][DRAWBORDER_TOTAL]);
                    Polygon( hdc, polygon, i);
                    i = 0;
                }
                else
                    fContinuingPoly = TRUE;
            }
            if ( wEdges & BF_BOTTOM && (j < aiNumBorderLines[BORDER_BOTTOM]) && aiLineWidths[BORDER_BOTTOM][j])
            {   // There's a bottom edge
                if ( !fContinuingPoly )
                {
                    // get color brush for bottom side
                    crNew = pborderinfo->acrColors[BORDER_BOTTOM][j];
                    if (crNew != crNow)
                    {
                        HBRUSH hbrNew;
                        SelectCachedBrush(hdc, crNew, &hbrNew, &hbrOld, &crNow);
#ifndef WINCE
                        // not supported on WINCE
                        ::UnrealizeObject(hbrNew);
#endif
                        ::SetBrushOrgEx(hdc, POSITIVE_MOD(ptBrushOrg.x,8), POSITIVE_MOD(ptBrushOrg.y,8), NULL);
                        i = 0;
                    }
                }

                // build bottom side polygon

                polygon[i].x    = rcSave.right - MulDivQuick(aiLineWidths[BORDER_RIGHT][DRAWBORDER_TOTAL],
                                                    aiLineWidths[BORDER_BOTTOM][DRAWBORDER_INCRE],
                                                    aiLineWidths[BORDER_BOTTOM][DRAWBORDER_TOTAL]);
                polygon[i++].y  = rc.bottom;

                if (pborderinfo->abStyles[BORDER_BOTTOM] == pborderinfo->abStyles[BORDER_LEFT]
                        && aiLineWidths[BORDER_RIGHT][j] == 1)
                {
                    // bottom left
                    polygon[i].x    = rcSave.left + MulDivQuick(aiLineWidths[BORDER_LEFT][DRAWBORDER_TOTAL],
                                                    aiLineWidths[BORDER_BOTTOM][DRAWBORDER_INCRE],
                                                    aiLineWidths[BORDER_BOTTOM][DRAWBORDER_TOTAL]);
                    polygon[i].y    = rc.bottom;

                    // lower left fix, we have to overlap 1 pixel to avoid holes
                    polygon[i+1].x  = polygon[i].x;
                    polygon[i+1].y  = rc.bottom - 1;

                    i = i + 2;
                }
                else
                {
                    polygon[i].x    = rcSave.left + MulDivQuick(aiLineWidths[BORDER_LEFT][DRAWBORDER_TOTAL],
                                                    aiLineWidths[BORDER_BOTTOM][DRAWBORDER_INCRE],
                                                    aiLineWidths[BORDER_BOTTOM][DRAWBORDER_TOTAL]);
                    polygon[i++].y    = rc.bottom;
                }

                // upper left, we have to overlap 1 pixel to avoid holes
                polygon[i].x    = rcSave.left + MulDivQuick(aiLineWidths[BORDER_LEFT][DRAWBORDER_TOTAL],
                                                    aiLineWidths[BORDER_BOTTOM][DRAWBORDER_INCRE]
                                                        + aiLineWidths[BORDER_BOTTOM][j],
                                                    aiLineWidths[BORDER_BOTTOM][DRAWBORDER_TOTAL]);
                polygon[i++].y  = rc.bottom - aiLineWidths[BORDER_BOTTOM][j];
                polygon[i].x    = rcSave.right  - MulDivQuick(aiLineWidths[BORDER_RIGHT][DRAWBORDER_TOTAL],
                                                    aiLineWidths[BORDER_BOTTOM][DRAWBORDER_INCRE]
                                                        + aiLineWidths[BORDER_BOTTOM][j],
                                                    aiLineWidths[BORDER_BOTTOM][DRAWBORDER_TOTAL]);

                polygon[i++].y  = rc.bottom - aiLineWidths[BORDER_BOTTOM][j];

                Polygon(hdc, polygon, i);
                i = 0;
            }

        }


        // Shrink rect for this line or spacer
        rc.top    += aiLineWidths[BORDER_TOP][j];
        rc.right  -= aiLineWidths[BORDER_RIGHT][j];
        rc.bottom -= aiLineWidths[BORDER_BOTTOM][j];
        rc.left   += aiLineWidths[BORDER_LEFT][j];

        // increment border shifts
        aiLineWidths[BORDER_RIGHT][DRAWBORDER_INCRE] += aiLineWidths[BORDER_RIGHT][j];
        aiLineWidths[BORDER_TOP][DRAWBORDER_INCRE] += aiLineWidths[BORDER_TOP][j];
        aiLineWidths[BORDER_BOTTOM][DRAWBORDER_INCRE] += aiLineWidths[BORDER_BOTTOM][j];
        aiLineWidths[BORDER_LEFT][DRAWBORDER_INCRE] += aiLineWidths[BORDER_LEFT][j];
    }

    // Okay, now let's draw the flat border if necessary.
    if ( xyFlat != 0 )
    {
        if (pborderinfo->xyFlat < 0)
        {
            rc = rcOrig;
        }
        rc.right++;
        rc.bottom++;
        xyFlat++;

        if (wEdges & BF_RIGHT)
        {
            crNew = pborderinfo->acrColors[BORDER_RIGHT][1];
            if (crNew != crNow)
            {
                HBRUSH hbrNew;
                SelectCachedBrush(hdc, crNew, &hbrNew, &hbrOld, &crNow);
#ifndef WINCE
                // not supported on WINCE
                ::UnrealizeObject(hbrNew);
#endif
                ::SetBrushOrgEx(hdc, POSITIVE_MOD(ptBrushOrg.x,8), POSITIVE_MOD(ptBrushOrg.y,8), NULL);
            }
            Rectangle(hdc,
                        rc.right - xyFlat,
                        rc.top + ((wEdges & BF_TOP) ? 0 : xyFlat),
                        rc.right,
                        rc.bottom - ((wEdges & BF_BOTTOM) ? 0 : xyFlat)
                        );
        }
        if (wEdges & BF_BOTTOM)
        {
            crNew = pborderinfo->acrColors[BORDER_BOTTOM][1];
            if (crNew != crNow)
            {
                HBRUSH hbrNew;
                SelectCachedBrush(hdc, crNew, &hbrNew, &hbrOld, &crNow);
#ifndef WINCE
                // not supported on WINCE
                ::UnrealizeObject(hbrNew);
#endif
                ::SetBrushOrgEx(hdc, POSITIVE_MOD(ptBrushOrg.x,8), POSITIVE_MOD(ptBrushOrg.y,8), NULL);
            }
            Rectangle(hdc,
                        rc.left + ((wEdges & BF_LEFT) ? 0 : xyFlat),
                        rc.bottom - xyFlat,
                        rc.right - ((wEdges & BF_RIGHT) ? 0 : xyFlat),
                        rc.bottom
                        );
        }

        if (wEdges & BF_TOP)
        {
            crNew = pborderinfo->acrColors[BORDER_TOP][1];
            if (crNew != crNow)
            {
                HBRUSH hbrNew;
                SelectCachedBrush(hdc, crNew, &hbrNew, &hbrOld, &crNow);
#ifndef WINCE
                // not supported on WINCE
                ::UnrealizeObject(hbrNew);
#endif
                ::SetBrushOrgEx(hdc, POSITIVE_MOD(ptBrushOrg.x,8), POSITIVE_MOD(ptBrushOrg.y,8), NULL);
            }
            Rectangle(hdc,
                    rc.left + ((wEdges & BF_LEFT) ? 0 : xyFlat),
                    rc.top,
                    rc.right - ((wEdges & BF_RIGHT) ? 0 : xyFlat),
                    rc.top + xyFlat
                    );
        }
        if (wEdges & BF_LEFT)
        {
            crNew = pborderinfo->acrColors[BORDER_LEFT][1];
            if (crNew != crNow)
            {
                HBRUSH hbrNew;
                SelectCachedBrush(hdc, crNew, &hbrNew, &hbrOld, &crNow);
#ifndef WINCE
                // not supported on WINCE
                ::UnrealizeObject(hbrNew);
#endif
                ::SetBrushOrgEx(hdc, POSITIVE_MOD(ptBrushOrg.x,8), POSITIVE_MOD(ptBrushOrg.y,8), NULL);
            }
            Rectangle(hdc,
                        rc.left,
                        rc.top + ((wEdges & BF_TOP) ? 0 : xyFlat),
                        rc.left + xyFlat,
                        rc.bottom - ((wEdges & BF_BOTTOM) ? 0 : xyFlat)
                        );
        }
    }

    if (hbrOld)
        ReleaseCachedBrush((HBRUSH)SelectObject(hdc, hbrOld));
}

//+----------------------------------------------------------------------------
//
// Function:    CalcImgBgRect
//
// Synopsis:    Finds the rectangle to pass to Tile() to draw correct
//              background image with the attributes specified in the
//              fancy format (repeat-x, repeat-y, etc).
//
//-----------------------------------------------------------------------------

void
CalcBgImgRect(
    CTreeNode          * pNode,
    CFormDrawInfo      * pDI,
    const SIZE         * psizeObj,
    const SIZE         * psizeImg,
          CPoint       * pptBackOrig,
          RECT         * prcBackClip)
{
    // pNode is used to a) extract formats to get the
    // background position values, and b) to get the font height so we
    // can handle em/en/ex units for position.

    const CFancyFormat * pFF = pNode->GetFancyFormat();
    
    // N.B. Per CSS spec, percentages work as follows:
    // (x%, y%) means that the (x%, y%) point in the image is
    // positioned at the (x%, y%) point in the bounded rectangle.

    if (pFF->_cuvBgPosX.GetUnitType() == CUnitValue::UNIT_PERCENT)
    {
        pptBackOrig->x =
                  MulDivQuick(pFF->_cuvBgPosX.GetPercent(),
                              psizeObj->cx - psizeImg->cx,
                              100);
    }
    else
    {
        pptBackOrig->x =
         pFF->_cuvBgPosX.GetPixelValue(pDI, CUnitValue::DIRECTION_CX, 0,
                                       pNode->GetFontHeightInTwips((CUnitValue*)&pFF->_cuvBgPosX));
    }

    if (pFF->_cuvBgPosY.GetUnitType() == CUnitValue::UNIT_PERCENT)
    {
        pptBackOrig->y =
                  MulDivQuick(pFF->_cuvBgPosY.GetPercent(),
                              psizeObj->cy - psizeImg->cy,
                              100);
    }
    else
    {
        pptBackOrig->y =
          pFF->_cuvBgPosY.GetPixelValue(pDI, CUnitValue::DIRECTION_CY, 0,
                                        pNode->GetFontHeightInTwips((CUnitValue*)&pFF->_cuvBgPosY));
    }


    if (pFF->_fBgRepeatX)
    {
        prcBackClip->left  = 0;
        prcBackClip->right = psizeObj->cx;
    }
    else
    {
        prcBackClip->left  = pptBackOrig->x;
        prcBackClip->right = pptBackOrig->x + psizeImg->cx;
    }

    if (pFF->_fBgRepeatY)
    {
        prcBackClip->top    = 0;
        prcBackClip->bottom = psizeObj->cy;
    }
    else
    {
        prcBackClip->top    = pptBackOrig->y;
        prcBackClip->bottom = pptBackOrig->y + psizeImg->cy;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     get_form
//
//  Synopsis:   Exposes the form element of this site.
//
//  Note:
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CSite::get_form(IHTMLFormElement **ppDispForm)
{
    HRESULT        hr = S_OK;
    CFormElement * pForm;

    if (!ppDispForm)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppDispForm = NULL;

    pForm = GetParentForm();
    if (pForm)
    {
        hr = THR_NOTRACE(pForm->QueryInterface(IID_IHTMLFormElement,
                                              (void**)ppDispForm));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN( SetErrorInfoPGet( hr, DISPID_CSite_form));
}

CSite::DRAGINFO::~DRAGINFO()
{
    if (_pBag)
        _pBag->Release();
}

//BUGBUG (terrylu, alexz) pdl parser should be fixed so to avoid doing this
#if 1
HRESULT CSite::focus() { return super::focus(); };
HRESULT CSite::blur() { return super::blur(); };
HRESULT CSite::addFilter(IUnknown* pUnk) { return super::addFilter(pUnk); };
HRESULT CSite::removeFilter(IUnknown* pUnk) { return super::removeFilter(pUnk); };
HRESULT CSite::get_clientHeight(long*p) { return super::get_clientHeight(p); };
HRESULT CSite::get_clientWidth(long*p) { return super::get_clientWidth(p); };
HRESULT CSite::get_clientTop(long*p) { return super::get_clientTop(p); };
HRESULT CSite::get_clientLeft(long*p) { return super::get_clientLeft(p); };
#endif
