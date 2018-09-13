//+------------------------------------------------------------------------
//
//  File:       grab.cxx
//
//  Contents:   Grab handle utilities
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifdef UNIX
extern "C" COLORREF MwGetTrueRGBValue(COLORREF crColor);
#endif

template < class T > void swap ( T & a, T & b ) { T t = a; a = b; b = t; }

struct HTCDSI
{
    short   htc;
    short   dsi;
};

static const HTCDSI s_aHtcDsi[] =
{
    {HTC_TOPLEFTHANDLE,     DSI_NOTOPHANDLES    | DSI_NOLEFTHANDLES     },
    {HTC_TOPHANDLE,         DSI_NOTOPHANDLES                            },
    {HTC_TOPRIGHTHANDLE,    DSI_NOTOPHANDLES    | DSI_NORIGHTHANDLES    },
    {HTC_LEFTHANDLE,                              DSI_NOLEFTHANDLES     },
    {HTC_RIGHTHANDLE,                             DSI_NORIGHTHANDLES    },
    {HTC_BOTTOMLEFTHANDLE,  DSI_NOBOTTOMHANDLES | DSI_NOLEFTHANDLES     },
    {HTC_BOTTOMHANDLE,      DSI_NOBOTTOMHANDLES                         },
    {HTC_BOTTOMRIGHTHANDLE, DSI_NOBOTTOMHANDLES | DSI_NORIGHTHANDLES    },
};

//+------------------------------------------------------------------------
//
//  Function:   ColorDiff
//
//  Synopsis:   Computes the color difference amongst two colors
//
//-------------------------------------------------------------------------
DWORD ColorDiff (COLORREF c1, COLORREF c2)
{
#ifdef UNIX
    if ( CColorValue(c1).IsMotifColor() ) {
        c1 = MwGetTrueRGBValue( c1 );
    }

    if ( CColorValue(c2).IsMotifColor() ) {
        c2 = MwGetTrueRGBValue( c2 );
    }
#endif

#define __squareit(n) ((DWORD)((n)*(n)))
    return (__squareit ((INT)GetRValue (c1) - (INT)GetRValue (c2)) +
            __squareit ((INT)GetGValue (c1) - (INT)GetGValue (c2)) +
            __squareit ((INT)GetBValue (c1) - (INT)GetBValue (c2)));
#undef __squareit
}

//+------------------------------------------------------------------------
//
//  Function:   PatBltRectH & PatBltRectV
//
//  Synopsis:   PatBlts the top/bottom and left/right.
//
//-------------------------------------------------------------------------
static void
PatBltRectH(HDC hDC, RECT * prc, int cThick, DWORD dwRop)
{
    PatBlt(
            hDC,
            prc->left,
            prc->top,
            prc->right - prc->left,
            cThick,
            dwRop);

    PatBlt(
            hDC,
            prc->left,
            prc->bottom - cThick,
            prc->right - prc->left,
            cThick,
            dwRop);
}

static void
PatBltRectV(HDC hDC, RECT * prc, int cThick, DWORD dwRop)
{
    PatBlt(
            hDC,
            prc->left,
            prc->top + cThick,
            cThick,
            (prc->bottom - prc->top) - (2 * cThick),
            dwRop);

    PatBlt(
            hDC,
            prc->right - cThick,
            prc->top + cThick,
            cThick,
            (prc->bottom - prc->top) - (2 * cThick),
            dwRop);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetGrabRect
//
//  Synopsis:   Compute grab rect for a given area.
//
//  Notes:      These diagrams show the output grab rect for handles and
//              borders.
//
//              -----   -----   -----               -------------
//              |   |   |   |   |   |               |           |
//              | TL|   | T |   |TR |               |     T     |
//              ----|-----------|----           ----|-----------|----
//                  |           |               |   |           |   |
//              ----| Input     |----           |   | Input     |   |
//              |   |           |   |           |   |           |   |
//              |  L|   RECT    |R  |           |  L|   RECT    |R  |
//              ----|           |----           |   |           |   |
//                  |           |               |   |           |   |
//              ----|-----------|----           ----|-----------|----
//              | BL|   | B |   |BR |               |     B     |
//              |   |   |   |   |   |               |           |
//              -----   -----   -----               -------------
//
//----------------------------------------------------------------------------

static void
GetGrabRect(HTC htc, RECT * prcOut, RECT * prcIn, LONG cGrabSize)
{
    switch (htc)
    {
    case HTC_TOPLEFTHANDLE:
    case HTC_LEFTHANDLE:
    case HTC_BOTTOMLEFTHANDLE:
    case HTC_GRPTOPLEFTHANDLE:
    case HTC_GRPLEFTHANDLE:
    case HTC_GRPBOTTOMLEFTHANDLE:
    case HTC_LEFTBORDER:
    case HTC_GRPLEFTBORDER:
        prcOut->left = prcIn->left - cGrabSize;
        prcOut->right = prcIn->left;
        break;

    case HTC_TOPHANDLE:
    case HTC_BOTTOMHANDLE:
    case HTC_GRPTOPHANDLE:
    case HTC_GRPBOTTOMHANDLE:
        prcOut->left = ((prcIn->left + prcIn->right) - cGrabSize) / 2;
        prcOut->right = prcOut->left + cGrabSize;
        break;

    case HTC_TOPRIGHTHANDLE:
    case HTC_RIGHTHANDLE:
    case HTC_BOTTOMRIGHTHANDLE:
    case HTC_GRPTOPRIGHTHANDLE:
    case HTC_GRPRIGHTHANDLE:
    case HTC_GRPBOTTOMRIGHTHANDLE:
    case HTC_RIGHTBORDER:
    case HTC_GRPRIGHTBORDER:
        prcOut->left = prcIn->right;
        prcOut->right = prcOut->left + cGrabSize;
        break;

    case HTC_TOPBORDER:
    case HTC_BOTTOMBORDER:
    case HTC_GRPTOPBORDER:
    case HTC_GRPBOTTOMBORDER:
        prcOut->left = prcIn->left;
        prcOut->right = prcIn->right;
        break;

    default:
        Assert(FALSE && "Unsupported HTC_ value in GetHandleRegion");
        return;
    }

    switch (htc)
    {
    case HTC_TOPLEFTHANDLE:
    case HTC_TOPHANDLE:
    case HTC_TOPRIGHTHANDLE:
    case HTC_GRPTOPLEFTHANDLE:
    case HTC_GRPTOPHANDLE:
    case HTC_GRPTOPRIGHTHANDLE:
    case HTC_TOPBORDER:
    case HTC_GRPTOPBORDER:
        prcOut->top = prcIn->top - cGrabSize;
        prcOut->bottom = prcIn->top;
        break;

    case HTC_LEFTHANDLE:
    case HTC_RIGHTHANDLE:
    case HTC_GRPLEFTHANDLE:
    case HTC_GRPRIGHTHANDLE:
        prcOut->top = ((prcIn->top + prcIn->bottom) - cGrabSize) / 2;
        prcOut->bottom = prcOut->top + cGrabSize;
        break;

    case HTC_BOTTOMLEFTHANDLE:
    case HTC_BOTTOMHANDLE:
    case HTC_BOTTOMRIGHTHANDLE:
    case HTC_GRPBOTTOMLEFTHANDLE:
    case HTC_GRPBOTTOMHANDLE:
    case HTC_GRPBOTTOMRIGHTHANDLE:
    case HTC_BOTTOMBORDER:
    case HTC_GRPBOTTOMBORDER:
        prcOut->top = prcIn->bottom;
        prcOut->bottom = prcOut->top + cGrabSize;
        break;

    case HTC_LEFTBORDER:
    case HTC_RIGHTBORDER:
    case HTC_GRPLEFTBORDER:
    case HTC_GRPRIGHTBORDER:
        prcOut->top = prcIn->top;
        prcOut->bottom = prcIn->bottom;
        break;

    default:
        Assert(FALSE && "Unsupported HTC_ value in GetHandleRegion");
        return;
    }

    if (prcOut->left > prcOut->right)
    {
        swap(prcOut->left, prcOut->right);
    }
    if (prcOut->top > prcOut->bottom)
    {
        swap(prcOut->top, prcOut->bottom);
    }
}


//+---------------------------------------------------------------------------
//
//  Global helpers.
//
//----------------------------------------------------------------------------

void
PatBltRect(HDC hDC, RECT * prc, int cThick, DWORD dwRop)
{
    PatBltRectH(hDC, prc, cThick, dwRop);

    PatBltRectV(hDC, prc, cThick, dwRop);
}

void
DrawDefaultFeedbackRect(HDC hDC, RECT * prc)
{
#ifdef NEVER
    HBRUSH  hbrOld = NULL;
    HBRUSH  hbr = GetCachedBmpBrush(IDR_FEEDBACKRECTBMP);

    hbrOld = (HBRUSH) SelectObject(hDC, hbr);
    if (!hbrOld)
        goto Cleanup;

    PatBltRect(hDC, prc, FEEDBACKRECTSIZE, PATINVERT);

Cleanup:
    if (hbrOld)
        SelectObject(hDC, hbrOld);
#endif        
}


//+------------------------------------------------------------------------
//
//  Member:     CLayout::DrawGrabBorders
//
//  Synopsis:   Draws grab borders around the givin rect.
//
//  BUBUG - this function exists here because it uses some of the static
//          functions declared in this file.
//
//-------------------------------------------------------------------------
void
CLayout::DrawGrabBorders(HDC hdc, RECT *prc, BOOL fHatch)
{
    DWORD       dwRop;
    HBRUSH      hbrOld = NULL;
    HBRUSH      hbr;
    RECT        rc = *prc;
    LONG        cGrabSize = (_fGrabInside) ? -GRABSIZE : GRABSIZE;
    POINT       pt ;

    GetViewportOrgEx (hdc, &pt) ;

    if ((Doc()->_pOptionSettings->crBack() & RGB(0xff, 0xff, 0xff))
             == RGB(0x00, 0x00, 0x00))
    {
        dwRop = DST_PAT_NOT_OR;
    }
    else
    {
        dwRop = DST_PAT_AND;
    }

    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkColor(hdc, RGB(255, 255, 255));

    InflateRect(&rc, cGrabSize, cGrabSize);

    //
    // In the code below, before each select of a brush into the dc, we do
    // an UnrealizeObject followed by a SetBrushOrgEx.  This assures that
    // the brush pattern is properly aligned in that face of scrolling,
    // resizing or dragging the form containing the "bordered" object.
    //

    if (fHatch)
    {
        hbr = GetCachedBmpBrush(IDR_HATCHBMP);
        if (!hbr)
            goto Cleanup;

        // Brush alignment code.
#ifndef WINCE
        // not supported on WINCE
        UnrealizeObject(hbr);
#endif
        SetBrushOrgEx(hdc, POSITIVE_MOD(pt.x,8)+POSITIVE_MOD(rc.left,8),
                           POSITIVE_MOD(pt.y,8)+POSITIVE_MOD(rc.top,8), NULL);

        hbrOld = (HBRUSH) SelectObject(hdc, hbr);
        if (!hbrOld)
            goto Cleanup;

        PatBltRect(hdc, &rc, cGrabSize, dwRop);
    }
    else if (GetFirstBranch()->IsAbsolute())
    {
        // For a site with position=absolute, draw an extra rectangle around it when selected
        HPEN hpenOld;

        InflateRect(&rc, -cGrabSize / 2, -cGrabSize / 2);

        hbrOld = (HBRUSH) SelectObject(hdc, GetStockObject(WHITE_BRUSH));
        if (!hbrOld)
            goto Cleanup;

        PatBltRect(hdc, &rc, cGrabSize / 2, PATCOPY);

        hpenOld = (HPEN) SelectObject(hdc, GetStockObject(BLACK_PEN));
        if (!hpenOld)
            goto Cleanup;

        MoveToEx(hdc, rc.left, rc.top, (GDIPOINT *)NULL);
        LineTo(hdc, rc.left, rc.bottom - 1);
        LineTo(hdc, rc.right - 1, rc.bottom - 1);
        LineTo(hdc, rc.right - 1, rc.top);
        LineTo(hdc, rc.left, rc.top);

        if (hpenOld)
            SelectObject(hdc, hpenOld);
     }

Cleanup:
    if (hbrOld)
        SelectObject(hdc, hbrOld);
}


//+------------------------------------------------------------------------
//
//  Member:     CLayout::DrawGrabHandles
//
//  Synopsis:   Draws grab handles around the givin rect.
//
//  BUBUG - this function exists here because it uses some of the static
//          functions declared in this file.
//
//-------------------------------------------------------------------------

void
CLayout::DrawGrabHandles(HDC hdc, RECT *prc, DWORD dwInfo)
{
    int         typeBrush = WHITE_BRUSH;
    int         typePen = BLACK_PEN;
    HBRUSH      hbr = (HBRUSH) GetStockObject(typeBrush);
    HPEN        hpen = (HPEN) GetStockObject(typePen);
    int         i;
    LONG        cGrabSize = (_fGrabInside) ? -GRABSIZE : GRABSIZE;

    Assert(hbr && hpen);

    if (ElementOwner()->IsLocked()) // don't draw grab handles when site is locked.
        return;

    // Load the brush and pen into the DC.
    hbr = (HBRUSH) SelectObject(hdc, hbr);
    hpen = (HPEN) SelectObject(hdc, hpen);

    // Draw each grab handle.
    for (i = 0; i < ARRAY_SIZE(s_aHtcDsi); ++i)
    {
        RECT    rc;

        // Skip any sides requested.
        if (dwInfo & s_aHtcDsi[i].dsi)
            continue;

        // Get the grab rect for this handle.
        GetGrabRect((HTC) s_aHtcDsi[i].htc, &rc, prc, cGrabSize);

        // Draw it.
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

#ifdef NEVER
        // Draw a cross if it is requested.
        // BUGBUG: Determine if this is the actual behaviour.
        if (dwInfo & DSI_LOCKED)
        {
            // This will draw a cross using the outside brush.
            MoveToEx(hdc, rc.left, rc.right, NULL);
            LineTo(hdc, rc.right, rc.bottom);
            MoveToEx(hdc, rc.right, rc.top, NULL);
            LineTo(hdc, rc.left, rc.bottom);
        }
#endif
    }

    // Restore the old brush and pen.
    hbr = (HBRUSH) SelectObject(hdc, hbr);
    hpen = (HPEN) SelectObject(hdc, hpen);
}


//
//  BUBUG - this function exists here because it uses some of the static
//          functions declared in this file.
void
CLayout::DrawSelectInfo(HDC hdc, RECT *prc, DWORD dwInfo)
{

    // For the purposes of hit testing and grab handle drawing, we treat <button> tags like
    // any other intrinsic even though it can have children.
    BOOL fContainer = (_fCanHaveChildren && !ElementOwner()->_fActsLikeButton);

    //
    // marka BUGBUG - HTMLArea's _fCanHaveChildren = FALSE, _fActsLikeButton = FALSE
    //
#ifdef  NEVER
    if ( ElementOwner()->_etag == ETAG_HTMLAREA )
        fContainer = FALSE ;
#endif
        
    //
    // marka - New fix for BUG # 9543. We only draw the UI Active 
    // iff we are in a Control ( ! Container).
    // We REMOVE the UI Active border for Tables and DIVs. 
    //
    //
    // this is a little unreadable. What we want to say is: 
    // if we are a Control ( !Container) or we are Site Selected ( !( dwInfo & DSI_UIACTIVE ) )
    //
    if(  ( ! fContainer ) ||  !( dwInfo & DSI_UIACTIVE ) )
    {
        DrawGrabBorders(hdc, prc, dwInfo & DSI_UIACTIVE);
        DrawGrabHandles(hdc, prc, dwInfo);
    }
}

//
//  BUBUG - this function exists here because it uses some of the static
//          functions declared in this file.
HTC
CLayout::HitTestSelect(POINT pt, RECT *prc)
{
    RECT    rc;
    int     htc;
    int     htcFirst, htcLast;
    LONG    cGrabSize;
    BOOL fContainer = (_fCanHaveChildren && !ElementOwner()->_fActsLikeButton);

#ifdef  NEVER
    if ( ElementOwner()->_etag == ETAG_HTMLAREA )
        fContainer = FALSE ; // marka see above BUGBUG
#endif
        
    Assert(ElementOwner()->IsEditable(TRUE));

    if (_fAdorned ||  ElementOwner()->HasCurrency()  )
    {
        cGrabSize = (_fGrabInside) ? -GRABSIZE : GRABSIZE;
        //
        // For locked sites, we do not want to hit test for resize handles
        //

        //
        // marka - For layouts that are UI Active, and Containers
        // we don't want to test for Resize Handles. See comment above in DrawSelectInfo
        // for Bug #9543
        //
        if ( (ElementOwner()->IsLocked()) || ( ElementOwner()->HasCurrency() && fContainer ) )
        {
           htcFirst = HTC_GRPTOPBORDER;
           htcLast = HTC_GRPRIGHTBORDER;
        }
        else
        {
            //
            // Start at bottom right because the bottom right handle
            // is preferred over others when handles overlap.
            //
            htcFirst = HTC_TOPBORDER;
            htcLast = HTC_BOTTOMRIGHTHANDLE;
        }
    }
    else
    {
        cGrabSize = -HITTESTSIZE;
        //
        // For inactive sites, we do not want to hit test for resize handles
        //
        htcFirst = HTC_GRPTOPBORDER;
        htcLast = HTC_GRPRIGHTBORDER;
    }

    for (htc = htcLast; htc >= htcFirst; htc--)
    {
        GetGrabRect((HTC) htc, &rc, prc, cGrabSize);
        if (PtInRect(&rc, pt))
            return (HTC) htc;
    }

    //
    // For elements marked as not ui-activatable and for 
    // intrinsics and OCs that aren't selected or current,
    // return HTC_TOPBORDER any time pt is within prc
    // We do this because we want the first click to select,
    // and not ui-activate for these elements.
    //
    if ((ElementOwner()->NoUIActivate() || 
        (!fContainer && !_fAdorned && !ElementOwner()->HasCurrency() )) && 
        PtInRect(prc, pt))
    {
        return HTC_TOPBORDER;
    }

    return HTC_NO;
}
