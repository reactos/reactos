#include "ctlspriv.h"
#include "rebar.h"
#include "image.h"

#ifdef DEBUG
int ExprASSERT(int e);
BOOL RBCheckRangePtr(PRB prb, PRBB prbb);
BOOL RBCheckRangeInd(PRB prb, INT_PTR i);
#else
#define ExprASSERT(e)   0
#define RBCheckRangePtr(prb, prbb)  0
#define RBCheckRangeInd(prb, i)     0
#endif

#define RBBUSECHEVRON(prb, prbb)    ((prbb->fStyle & RBBS_USECHEVRON) &&              \
                                    !((prbb)->fStyle & RBBS_FIXEDSIZE) &&          \
                                    ((UINT)(prbb)->cxIdeal > (prbb)->cxMinChild))

#define RBSHOWTEXT(prbb) (!(prbb->fStyle&RBBS_HIDETITLE) && prbb->lpText && prbb->lpText[0])

#define CX_CHEVRON (5 * g_cxEdge + 2)
#define CX_OFFSET (2 * g_cxEdge)
#define RB_GRABWIDTH 5
#define RB_ISVERT(prb)  ((prb)->ci.style & CCS_VERT)
#define RB_ISVERTICALGRIPPER(prb) (RB_ISVERT(prb) && (prb)->ci.style & RBS_VERTICALGRIPPER)
#define RB_GETLASTBAND(prb) ((prb)->cBands ? RBGETBAND(prb, (prb)->cBands -1) : NULL)

#define RBISBANDSTARTOFROW(prbb) (!((prbb)->x) && !((prbb)->fStyle & RBBS_HIDDEN))
#define RBGETBAND(prb, i) (ExprASSERT(RBCheckRangeInd(prb, i)), &(prb)->rbbList[i])
#define RBISSTARTOFROW(prb, i) (RBISBANDSTARTOFROW( RBGETBAND((prb), (i))))
#define RBGETFIRSTBAND(prb) (RBGETBAND(prb, 0))
#define RBBANDTOINDEX(prb, prbb) ((int)((prbb) - (prb)->rbbList))
#define RBBHEADERWIDTH(prbb) ((prbb)->cxMin - ((prbb)->cxMinChild + ((RBBUSECHEVRON(prb, prbb) ? CX_CHEVRON : 0))))
#define RBISBANDVISIBLE(prbb)  (!((prbb)->fStyle & RBBS_HIDDEN))
#define RBROWATMINHEIGHT(prb, pprbb) (!RBGetRowHeightExtra(prb, pprbb, NULL))
#define RBGETBARHEIGHT(prb) (((prb)->cBands && !(prb)->cy) ? RBRecalc(prb) : (prb)->cy)

#define RB_ISVALIDINDEX(prb, i)     ((UINT)i < (prb)->cBands)
#define RB_ISVALIDBAND(prb, prbb)   RB_ISVALIDINDEX(prb, RBBANDTOINDEX(prb, prbb))

#define RB_ANIMSTEPS 10
#define RB_ANIMSTEPTIME 5

void FlipRect(LPRECT prc);

void RBPassBreak(PRB prb, PRBB prbbSrc, PRBB prbbDest);
int RBHitTest(PRB prb, LPRBHITTESTINFO prbht);
BOOL RBSizeBandsToRect(PRB prb, LPRECT prc);
BOOL RBShouldDrawGripper(PRB prb, PRBB prbb);
void RBAutoSize(PRB prb);
void RBSizeBandsToRowHeight(PRB prb);
void RBSizeBandToRowHeight(PRB prb, int i, UINT uRowHeight);
BOOL RBSetBandPos(PRB prb, PRBB prbb, int xLeft);
BOOL RBSetBandPosAnim(PRB prb, PRBB prbb, int xLeft);
PRBB RBGetFirstInRow(PRB prb, PRBB prbbRow);
PRBB RBGetLastInRow(PRB prb, PRBB prbbRow, BOOL fStopAtFixed);
PRBB RBGetPrev(PRB prb, PRBB prbb, UINT uStyleSkip);
PRBB RBGetNext(PRB prb, PRBB prbb, UINT uStyleSkip);
PRBB RBEnumBand(PRB prb, int i, UINT uStyleSkip);
int RBCountBands(PRB prb, UINT uStyleSkip);
BOOL RBMaximizeBand(PRB prb, UINT uBand, BOOL fIdeal, BOOL fAnim);
PRBB RBGetNextVisible(PRB prb, PRBB prbb);
PRBB RBGetPrevVisible(PRB prb, PRBB prbb);
PRBB RBBNextVisible(PRB prb, PRBB prbb);
BOOL  RBShowBand(PRB prb, UINT uBand, BOOL fShow);
void RBGetClientRect(PRB prb, LPRECT prc);
int RBGetRowHeightExtra(PRB prb, PRBB *pprbb, PRBB prbbSkip);
void RBOnBeginDrag(PRB prb, UINT uBand);

#define RBBANDWIDTH(prb, prbb)  _RBBandWidth(prb, prbb->cx)
#ifdef DEBUG
#undef  RBBANDWIDTH
#define RBBANDWIDTH(prb, prbb) \
    ((prbb->fStyle & RBBS_HIDDEN) ? (ExprASSERT(0), -1) : \
    _RBBandWidth(prb, prbb->cx))
#endif

#define RBBANDMINWIDTH(prb, prbb) _RBBandWidth(prb, prbb->cxMin)
#ifdef DEBUG
#undef  RBBANDMINWIDTH
#define RBBANDMINWIDTH(prb, prbb) \
    ((prbb->fStyle & RBBS_HIDDEN) ? (ExprASSERT(0), -1) : \
    _RBBandWidth(prb, prbb->cxMin))
#endif

//***   RBC_* -- commands
#define RBC_QUERY   0
#define RBC_SET     1

#ifdef DEBUG
int ExprASSERT(int e)
{
    ASSERT(e);
    return 0;
}
#endif

HBRUSH g_hDPFRBrush = NULL;

__inline COLORREF RB_GetBkColor(PRB prb)
{
    if (prb->clrBk == CLR_DEFAULT)
        return g_clrBtnFace;
    else
        return prb->clrBk;
}

__inline COLORREF RB_GetTextColor(PRB prb)
{
    if (prb->clrText == CLR_DEFAULT)
        return g_clrBtnText;
    else
        return prb->clrText;
}

__inline COLORREF RBB_GetBkColor(PRB prb, PRBB prbb)
{
    switch(prbb->clrBack)
    {
    case CLR_NONE:
        // CLR_NONE means "use our dad's color"
        return RB_GetBkColor(prb);

    case CLR_DEFAULT:
        return g_clrBtnFace;

    default:
        return prbb->clrBack;
    }
}

__inline COLORREF RBB_GetTextColor(PRB prb, PRBB prbb)
{
    switch (prbb->clrFore)
    {
    case CLR_NONE:
        // CLR_NONE means "use our dad's color"
        return RB_GetTextColor(prb);

    case CLR_DEFAULT:
        return g_clrBtnText;

    default:
        return prbb->clrFore;
    }
}

//
// Our use of CLR_DEFAULT for the band background colors is new for 
// version 5.01.  Since we don't want to confuse apps by returning
// CLR_DEFAULT when they used to see a real colorref, we convert it
// before returning it to them.  If the background color is CLR_NONE, 
// though, we need to return it without conversion (like version 4 did).
// The *_External functions handle these cases.
//
__inline COLORREF RBB_GetTextColor_External(PRB prb, PRBB prbb)
{
    if (prbb->clrFore == CLR_NONE)
        return CLR_NONE;
    else
        return RBB_GetTextColor(prb, prbb);
}

__inline COLORREF RBB_GetBkColor_External(PRB prb, PRBB prbb)
{
    if (prbb->clrBack == CLR_NONE)
        return CLR_NONE;
    else
        return RBB_GetBkColor(prb, prbb);
}


///
//
// Implement MapWindowPoints as if the hwndFrom and hwndTo aren't
// mirrored. This is used when any of the windows (hwndFrom or hwndTo)
// are mirrored. See below. [samera]
//
int TrueMapWindowPoints(HWND hwndFrom, HWND hwndTo, LPPOINT lppt, UINT cPoints)
{
    int dx, dy;
    RECT rcFrom={0,0,0,0}, rcTo={0,0,0,0};

    if (hwndFrom) {
        GetClientRect(hwndFrom, &rcFrom);
        MapWindowPoints(hwndFrom, NULL, (LPPOINT)&rcFrom.left, 2);
    }

    if (hwndTo) {
        GetClientRect(hwndTo, &rcTo);
        MapWindowPoints(hwndTo, NULL, (LPPOINT)&rcTo.left, 2);
    }

    dx = rcFrom.left - rcTo.left;
    dy = rcFrom.top  - rcTo.top;

    /*
     * Map the points
     */
    while (cPoints--) {
        lppt->x += dx;
        lppt->y += dy;
        ++lppt;
    }
    
    return MAKELONG(dx, dy);
}

///
//
// Map a rect to parent should be based on the visual right edge
// for calculating the client coordinates for a RTL mirrored windows.
// This routine should only be used when calculating client
// coordinates in a RTL mirrored window. [samera]
//
BOOL MapRectInRTLMirroredWindow( LPRECT lprc, HWND hwnd)
{
    int iWidth  = lprc->right - lprc->left;
    int iHeight = lprc->bottom- lprc->top;
    RECT rc={0,0,0,0};


    if (hwnd) {
        GetClientRect(hwnd, &rc);
        MapWindowPoints(hwnd, NULL, (LPPOINT)&rc.left, 2);
    }

    lprc->left = rc.right - lprc->right;
    lprc->top  = lprc->top-rc.top;

    lprc->bottom = lprc->top + iHeight;
    lprc->right  = lprc->left + iWidth;

    return TRUE;
}

int _RBBandWidth(PRB prb, int x)
{
    if (prb->ci.style & RBS_BANDBORDERS)
        x += g_cxEdge;
    return x;
}

void RBRealize(PRB prb, HDC hdcParam, BOOL fBackground, BOOL fForceRepaint)
{
    if (prb->hpal)
    {
        HDC hdc = hdcParam ? hdcParam : GetDC(prb->ci.hwnd);

        if (hdc)
        {
            BOOL fRepaint;
            
            SelectPalette(hdc, prb->hpal, fBackground);
            fRepaint = RealizePalette(hdc) || fForceRepaint;

            if (!hdcParam)
                ReleaseDC(prb->ci.hwnd, hdc);

            if (fRepaint)
            {
                InvalidateRect(prb->ci.hwnd, NULL, TRUE);
            }
        }
    }
}


//////////////////////////////////////////////////////////////////
// RBSendNotify
//
// sends a wm_notify of code iCode and packages up all the data for you
// for band uBand
//
//////////////////////////////////////////////////////////////////
LRESULT RBSendNotify(PRB prb, UINT uBand, int iCode)
{
    NMREBAR nm = {0};
    
    nm.uBand = uBand;
    if (uBand != (UINT)-1) {
        nm.dwMask = RBNM_ID | RBNM_STYLE | RBNM_LPARAM;

        nm.wID = RBGETBAND(prb, uBand)->wID;
        nm.fStyle = RBGETBAND(prb, uBand)->fStyle;
        nm.lParam = RBGETBAND(prb, uBand)->lParam;
    }
    return CCSendNotify(&prb->ci, iCode, &nm.hdr);
}


BOOL RBInvalidateRect(PRB prb, RECT* prc)
{
    if (prb->fRedraw) 
    {
        RECT rc;

        if (prc && RB_ISVERT(prb))
        {
            CopyRect(&rc, prc);
            FlipRect(&rc);
            prc = &rc;
        }

        prb->fRefreshPending = FALSE;
        InvalidateRect(prb->ci.hwnd, prc, TRUE);
        return TRUE;
    }
    else 
    {
        prb->fRefreshPending = TRUE;
        return FALSE;
    }
}

LRESULT RebarDragCallback(HWND hwnd, UINT code, WPARAM wp, LPARAM lp)
{
    PRB prb = (PRB)GetWindowPtr(hwnd, 0);
    LRESULT lres;

    switch (code)
    {
    case DPX_DRAGHIT:
        if (lp)
        {
            int iBand;
            RBHITTESTINFO rbht;

            rbht.pt.x = ((POINTL *)lp)->x;
            rbht.pt.y = ((POINTL *)lp)->y;

            MapWindowPoints(NULL, prb->ci.hwnd, &rbht.pt, 1);

            iBand = RBHitTest(prb, &rbht);
            *(DWORD*)wp = rbht.flags;
            lres = (LRESULT)(iBand != -1 ? prb->rbbList[iBand].wID : -1);
        }
        else
            lres = -1;
        break;

    case DPX_GETOBJECT:
        lres = (LRESULT)GetItemObject(&prb->ci, RBN_GETOBJECT, &IID_IDropTarget, (LPNMOBJECTNOTIFY)lp);
        break;

    default:
        lres = -1;
        break;
    }

    return lres;
}

// ----------------------------------------------------------------------------
//
// RBCanBandMove
//
// returns TRUE if the given band can be moved and FALSE if it cannot
//
// ----------------------------------------------------------------------------
BOOL  RBCanBandMove(PRB prb, PRBB prbb)
{
    // If there is only one visible band it cannot move
    if (RBEnumBand(prb, 1, RBBS_HIDDEN) > RB_GETLASTBAND(prb))
        return FALSE;

    ASSERT(!(prbb->fStyle & RBBS_HIDDEN));
        
    if ((prb->ci.style & RBS_FIXEDORDER)
      && (prbb == RBEnumBand(prb, 0, RBBS_HIDDEN)))
        // the first (visible) band in fixed order rebars can't be moved
        return(FALSE);
    
    // fixed size bands can't be moved
    return(!(prbb->fStyle & RBBS_FIXEDSIZE));
}

// ----------------------------------------------------------------------------
//
// RBBCalcMinWidth
//
// calculates minimum width for the given band
//
// ----------------------------------------------------------------------------
void  RBBCalcMinWidth(PRB prb, PRBB prbb)
{
    BOOL fDrawGripper = RBShouldDrawGripper(prb, prbb);
    BOOL fVertical;
    int  cEdge;
    BOOL fEmpty = ((prbb->iImage == -1) && (!RBSHOWTEXT(prbb)));

    if (prbb->fStyle & RBBS_HIDDEN) {
        ASSERT(0);
        return;
    }

    // did the user specify the size explicitly?
    if (prbb->fStyle & RBBS_FIXEDHEADERSIZE)
        return;

    prbb->cxMin = prbb->cxMinChild;

    if (RBBUSECHEVRON(prb, prbb))
        prbb->cxMin += CX_CHEVRON;

    if (!fDrawGripper && fEmpty)
        return;

    fVertical = (prb->ci.style & CCS_VERT);
    if (RB_ISVERTICALGRIPPER(prb)) {
        
        prbb->cxMin += 4 * g_cyEdge;
        prbb->cxMin += max(prb->cyImage, prb->cyFont);
        
    } else {
        cEdge = fVertical ? g_cyEdge : g_cxEdge;

        prbb->cxMin += 2 * cEdge;

        if (fDrawGripper)
        {
            prbb->cxMin += RB_GRABWIDTH * (fVertical ? g_cyBorder : g_cxBorder);
            if (fEmpty)
                return;
        }

        prbb->cxMin += 2 * cEdge;

        if (prbb->iImage != -1)
            prbb->cxMin += (fVertical ? prb->cyImage : prb->cxImage);

        if (RBSHOWTEXT(prbb))
        {
            if (fVertical)
                prbb->cxMin += prb->cyFont;
            else
                prbb->cxMin += prbb->cxText;
            if (prbb->iImage != -1)
                // has both image and text -- add in edge between 'em
                prbb->cxMin += cEdge;
        }
    }
}

BOOL RBShouldDrawGripper(PRB prb, PRBB prbb)
{
    if (prbb->fStyle & RBBS_NOGRIPPER)
        return FALSE;

    if ((prbb->fStyle & RBBS_GRIPPERALWAYS) || RBCanBandMove(prb, prbb))
        return TRUE;
    
    return FALSE;
        
}

// ----------------------------------------------------------------------------
//
// RBBCalcTextExtent
//
// computes the horizontal extent of the given band's title text in the current
// title font for the rebar
//
// returns TRUE if text extent changed, FALSE otherwise
//
// ----------------------------------------------------------------------------
BOOL  RBBCalcTextExtent(PRB prb, PRBB prbb, HDC hdcIn)
{
    HDC     hdc = hdcIn;
    HFONT   hFontOld;
    UINT    cx;

    if (prbb->fStyle & RBBS_HIDDEN)
    {
        ASSERT(0);      // caller should have skipped
        return FALSE;
    }

    if (!RBSHOWTEXT(prbb))
    {
        cx = 0;
    }
    else
    {
        if (!hdcIn && !(hdc = GetDC(prb->ci.hwnd)))
            return FALSE;

        hFontOld = SelectObject(hdc, prb->hFont);

        // for clients >= v5, we draw text with prefix processing (& underlines next char)
        if (prb->ci.iVersion >= 5)
        {
            RECT rc = {0,0,0,0};
            DrawText(hdc, prbb->lpText, lstrlen(prbb->lpText), &rc, DT_CALCRECT);
            cx = RECTWIDTH(rc);
        }
        else
        {
            SIZE size;
            GetTextExtentPoint(hdc, prbb->lpText, lstrlen(prbb->lpText), &size);
            cx = size.cx;
        }
        SelectObject(hdc, hFontOld);

        if (!hdcIn)
            ReleaseDC(prb->ci.hwnd, hdc);
    }

    if (prbb->cxText != cx)
    {
        prbb->cxText = cx;
        RBBCalcMinWidth(prb, prbb);

        return TRUE;
    }

    return FALSE;
}

// ----------------------------------------------------------------------------
//
// RBBGetHeight
//
// returns minimum height for the given band
// TODO: make this a field in the band structure instead of always calling this
//
// ----------------------------------------------------------------------------
UINT  RBBGetHeight(PRB prb, PRBB prbb)
{
    UINT cy = 0;
    BOOL fVertical = (prb->ci.style & CCS_VERT);
    UINT cyCheck, cyBorder;

    cyBorder = (fVertical ? g_cxEdge : g_cyEdge) * 2;

    if (prbb->hwndChild)
    {
        cy = prbb->cyChild;
        if (!(prbb->fStyle & RBBS_CHILDEDGE))
            // add edge to top and bottom of child window
            cy -= cyBorder;
    }

    if (RBSHOWTEXT(prbb) && !fVertical)
    {
        cyCheck = prb->cyFont;

        if (cyCheck > cy)
            cy = cyCheck;
    }

    if (prbb->iImage != -1)
    {
        cyCheck = (fVertical) ? prb->cxImage : prb->cyImage;

        if (cyCheck > cy)
            cy = cyCheck;
    }

    return(cy + cyBorder);
}

// ----------------------------------------------------------------------------
//
// RBGetRowCount
//
// returns the number of rows in the rebar's current configuration
//
// ----------------------------------------------------------------------------
UINT  RBGetRowCount(PRB prb)
{
    UINT i;
    UINT cRows = 0;

    for (i = 0; i < prb->cBands; i++) {
        if (RBGETBAND(prb, i)->fStyle & RBBS_HIDDEN)
            continue;

        if (RBISSTARTOFROW(prb, i))
            cRows++;
    }

    return(cRows);
}

// ----------------------------------------------------------------------------
//
// RBGetLineHeight
//
// returns the height of the line of bands from iStart to iEnd, inclusively
//
// ----------------------------------------------------------------------------
UINT  RBGetLineHeight(PRB prb, UINT iStart, UINT iEnd)
{
    UINT cy = 0;
    PRBB prbb;
    UINT cyBand;

    if (!(prb->ci.style & RBS_VARHEIGHT))
    {
        // for fixed height bars, line height is maximum height of ALL bands
        iStart = 0;
        iEnd = prb->cBands - 1;
    }

    for (prbb = prb->rbbList + iStart; iStart <= iEnd; prbb++, iStart++)
    {
        if (prbb->fStyle & RBBS_HIDDEN)
            continue;

        cyBand = RBBGetHeight(prb, prbb);
        cy = max(cy, cyBand);
    }

    return(cy);
}

// RBRecalcChevron: update & refresh chevron
void RBRecalcChevron(PRB prb, PRBB prbb, BOOL fChevron)
{
    RECT rcChevron;

    if (fChevron)
    {
        rcChevron.right = prbb->x + prbb->cx;
        rcChevron.left = rcChevron.right - CX_CHEVRON;
        rcChevron.top = prbb->y;
        rcChevron.bottom = rcChevron.top + prbb->cy;
    }
    else
        SetRect(&rcChevron, -1, -1, -1, -1);

    if (!EqualRect(&rcChevron, &prbb->rcChevron))
    {
        if (prbb->fChevron)
            RBInvalidateRect(prb, &prbb->rcChevron);

        prbb->fChevron = fChevron;
        CopyRect(&prbb->rcChevron, &rcChevron);

        if (prbb->fChevron)
            RBInvalidateRect(prb, &prbb->rcChevron);
    }
}

// ----------------------------------------------------------------------------
//
// RBResizeChildren
//
// resizes children to fit properly in their respective bands' bounding rects
//
// ----------------------------------------------------------------------------
void  RBResizeChildren(PRB prb)
{
    int     cx, cy, x, y, cxHeading;
    HDWP    hdwp;
    BOOL    fVertical = (prb->ci.style & CCS_VERT);
    PRBB prbb, prbbEnd;

    if (!prb->cBands || !prb->fRedraw)
        return;

    hdwp = BeginDeferWindowPos(prb->cBands);

    prbb = RBGETBAND(prb, 0);
    prbbEnd = RB_GETLASTBAND(prb);

    for ( ; prbb <= prbbEnd ; prbb++)
    {
        NMREBARCHILDSIZE nm;
        BOOL fChevron = FALSE;

        if (prbb->fStyle & RBBS_HIDDEN)
            continue;
        
        if (!prbb->hwndChild)
            continue;

        cxHeading = RBBHEADERWIDTH(prbb);
        x = prbb->x + cxHeading;

        cx = prbb->cx - cxHeading;

        // if we're not giving child ideal size, make space for chevron button
        if ((cx < prbb->cxIdeal) && RBBUSECHEVRON(prb, prbb))
        {
            fChevron = TRUE;
            cx -= CX_CHEVRON;
        }

        if (!(prbb->fStyle & RBBS_FIXEDSIZE)) {
            if (fVertical) {
                PRBB prbbNext = RBBNextVisible(prb, prbb);
                if (prbbNext && !RBISBANDSTARTOFROW(prbbNext))
                    cx -= g_cyEdge * 2;
            } else 
                cx -= CX_OFFSET;
        }

        if (cx < 0)
            cx = 0;
        y = prbb->y;
        cy = prbb->cy;
        if (prbb->cyChild && (prbb->cyChild < (UINT) cy))
        {
            y += (cy - prbb->cyChild) / 2;
            cy = prbb->cyChild;
        }

        nm.rcChild.left = x;
        nm.rcChild.top = y;
        nm.rcChild.right = x + cx;
        nm.rcChild.bottom = y + cy;
        nm.rcBand.left = prbb->x + RBBHEADERWIDTH(prbb);
        nm.rcBand.right = prbb->x + prbb->cx;
        nm.rcBand.top = prbb->y;
        nm.rcBand.bottom = prbb->y + prbb->cy;

        nm.uBand = RBBANDTOINDEX(prb, prbb);
        nm.wID = prbb->wID;
        if (fVertical) {
            FlipRect(&nm.rcChild);
            FlipRect(&nm.rcBand);
        }
        
        CCSendNotify(&prb->ci, RBN_CHILDSIZE, &nm.hdr);

        if (!RB_ISVALIDBAND(prb, prbb)) {
            // somebody responded to notify by nuking bands; bail
            break;
        }

        RBRecalcChevron(prb, prbb, fChevron);

        DeferWindowPos(hdwp, prbb->hwndChild, NULL, nm.rcChild.left, nm.rcChild.top, 
                       RECTWIDTH(nm.rcChild), RECTHEIGHT(nm.rcChild), SWP_NOZORDER);
    }

    EndDeferWindowPos(hdwp);

    //
    //  The SQL 7.0 Enterprise Manager Data Transformation Services MMC Snap-In
    //  (and the Visual Basic Coolbar Sample App, too) is stupid.
    //  It hosts a rebar but doesn't set the WS_CLIPCHILDREN flag,
    //  so when it erases its background, it wipes out the rebar.  So don't
    //  call UpdateWindow() here, or we will paint *first*, then SQL will
    //  erase us by mistake.  We have to leave our paint pending, so that
    //  when SQL erases us by mistake, we will eventually get a WM_PAINT
    //  message afterwards.
    //
#if 0
    UpdateWindow(prb->ci.hwnd);
#endif
}

// ----------------------------------------------------------------------------
//
// RBMoveBand
//
// moves the band from one position to another in the rebar's band array,
// updating the rebar's iCapture field as needed
//
// returns TRUE or FALSE if something moved
// ----------------------------------------------------------------------------
BOOL  RBMoveBand(PRB prb, UINT iFrom, UINT iTo)
{
    RBB rbbMove;
    int iShift;
    BOOL fCaptureChanged = (prb->iCapture == -1);

    if (iFrom != iTo)
    {
        rbbMove = *RBGETBAND(prb, iFrom);
        if (prb->iCapture == (int) iFrom)
        {
            prb->iCapture = (int) iTo;
            fCaptureChanged = TRUE;
        }

        iShift = (iFrom > iTo) ? -1 : 1;

        while (iFrom != iTo)
        {
            if (!fCaptureChanged && (prb->iCapture == (int) (iFrom + iShift)))
            {
                prb->iCapture = (int) iFrom;
                fCaptureChanged = TRUE;
            }

            *RBGETBAND(prb, iFrom) = *RBGETBAND(prb, iFrom + iShift);
            iFrom += iShift;
        }
        *RBGETBAND(prb, iTo) = rbbMove;
        return TRUE;
    }
    return(FALSE);
}

// ----------------------------------------------------------------------------
//
// RBRecalc
//
// recomputes bounding rects for all bands in given rebar
//
// ----------------------------------------------------------------------------
UINT  RBRecalc(PRB prb)
{
    PRBB    prbb = RBGETBAND(prb, 0);
    PRBB    prbbWalk;
    UINT    cHidden;    // # of hidden guys we've seen in current row
    UINT    cxRow;
    UINT    cxMin;
    UINT    i;
    UINT    j;
    UINT    k;
    UINT    iFixed = 0xFFFF;
    int     cy;
    int     y;
    int     x;
    UINT    cxBar;
    RECT    rc;
    HWND    hwndSize;
    BOOL    fNewLine = FALSE;
    BOOL    fChanged;
    BOOL    fVertical = (prb->ci.style & CCS_VERT);
    BOOL    fBandBorders;
    int     iBarWidth;

    if (!prb->cBands)
        return(0);

    if ((prb->ci.style & CCS_NORESIZE) || (prb->ci.style & CCS_NOPARENTALIGN))
        // size based on rebar window itself
        hwndSize = prb->ci.hwnd;
    else if (!(hwndSize = prb->ci.hwndParent))
        // size based on parent window -- if no parent window, bail now
        return(0);

    if (!prb->fRecalc) {
        // defer this recalc
        prb->fRecalcPending = TRUE;
        return 0;
    } else {
        prb->fRecalcPending = FALSE;
    }

    GetClientRect(hwndSize, &rc);

    iBarWidth = (fVertical ? (rc.bottom - rc.top) : (rc.right - rc.left));
    // this can happen because we adjust the client rect, but wedon't change 
    // the getminmaxinfo.
    if (iBarWidth <= 0)
        iBarWidth = 1;

    cxBar = (UINT) iBarWidth;    

    fBandBorders = (prb->ci.style & RBS_BANDBORDERS);

    for (i = 0; i < prb->cBands; i++) {
        prb->rbbList[i].cx = prb->rbbList[i].cxRequest;
    }

    y = 0;
    i = 0;
    // Main Loop -- loop until all bands are calculated
    while (i < prb->cBands)
    {
        TraceMsg(TF_REBAR, "RBRecalc: outer loop i=%d", i);
        
        if (fBandBorders && (y > 0))
            y += g_cyEdge;

ReLoop:
        cxRow = 0;
        cxMin = 0;

        x = 0;
        cHidden = 0;

        // Row Loop -- loop until hard line break is found or soft line break
        // is necessary
        for (j = i, prbbWalk = prbb; j < prb->cBands; j++, prbbWalk++)
        {
            TraceMsg(TF_REBAR, "RBRecalc: inner loop j=%d", j);
            
            if (prbbWalk->fStyle & RBBS_HIDDEN) {
                ++cHidden;
                continue;
            }

            if (j > i + cHidden)
            {
                // not the first band in the row -- check for break style
                if ((prbbWalk->fStyle & RBBS_BREAK) && !(prbbWalk->fStyle & RBBS_FIXEDSIZE))
                    break;

                if (fBandBorders)
                    // add in space for vertical etch on palettized display
                    cxMin += g_cxEdge;
            }

            if (prbbWalk->fStyle & RBBS_FIXEDSIZE)
            {
                // remember location of branding brick
                iFixed = j;
             
                // if this is the first band, the next band cannot have a forced break.
                if (i + cHidden == j) {
                    // if the first index in the row (i) plus the number of hidden items (cHidden) leaves us at this band,
                    // then it's the first visible in this row.
                    PRBB prbbNextVis = RBBNextVisible(prb, prbbWalk);
                    if (prbbNextVis && (prbbNextVis->fStyle & RBBS_BREAK)) {
                        // can't do this unilaterally because on startup
                        // some folks (net meeting) initialize it in reverse order
                        // and we whack off this break bit incorrectly
                        if (prb->fRedraw && IsWindowVisible(prb->ci.hwnd))
                            prbbNextVis->fStyle &= ~RBBS_BREAK;
                    }
                }
                
                prbbWalk->cx = prbbWalk->cxMin;
            }

            if (prbbWalk->cx < prbbWalk->cxMin)
                prbbWalk->cx = prbbWalk->cxMin;

            cxMin += prbbWalk->cxMin; // update running total of min widths

            // read the assert comment below
            if (j > i + cHidden)
            {
                // not the first band in row -- check for need to autobreak
                if (cxMin > cxBar)
                    // autobreak here
                    break;


                if (fBandBorders)
                    // add in space for vertical etch on palettized display
                    cxRow += g_cxEdge;
            }

            cxRow += prbbWalk->cx; // update running total of current widths
        }

        if (!i)
        {
            // first row -- handle proper placement of branding band
            if (iFixed == 0xFFFF)
            {
                // branding band not yet found; look in the remaining bands
                k = j;
                for ( ; j < prb->cBands; j++)
                {
                    if (RBGETBAND(prb, j)->fStyle & RBBS_HIDDEN)
                        continue;

                    if (RBGETBAND(prb, j)->fStyle & RBBS_FIXEDSIZE)
                    {
                        // branding band found; move to 1st row and recompute
                        ASSERT(j != k);                        
                        RBMoveBand(prb, j, k);
                        goto ReLoop;
                    }
                }
                // no branding band found -- reset j and continue on
                j = k;
            }
            else
                // we have a branding band; move it to
                // the rightmost position in the row
                RBMoveBand(prb, iFixed, j - 1);

            TraceMsg(TF_REBAR, "RBRecalc: after brand i=%d", i);            
        }

        // variant:
        // now the current row of bands is from i to j - 1
        // n.b. i (and some following bands) might be hidden

        // assert that j != i because then the above variant won't be true
        ASSERT(j != i);     // BUGBUG RBBS_HIDDEN?

        if (cxRow > cxBar)
        {
            // bands are too long -- shrink bands from right to left
            for (k = i; k < j; k++)
            {
                prbbWalk--;
                if (prbbWalk->fStyle & RBBS_HIDDEN)
                    continue;

                if (prbbWalk->cx > prbbWalk->cxMin)
                {
                    cxRow -= prbbWalk->cx - prbbWalk->cxMin;
                    prbbWalk->cx = prbbWalk->cxMin;
                    if (cxRow <= cxBar)
                    {
                        prbbWalk->cx += cxBar - cxRow;
                        break;
                    }
                }
            }
            TraceMsg(TF_REBAR, "RBRecalc: after shrink i=%d", i);            
        }
        else if (cxRow < cxBar)
        {
            // bands are too short -- grow rightmost non-minimized band
            for (k = j - 1; k >= i; k--)
            {
                ASSERT(k != (UINT)-1);  // catch infinite loop
                prbbWalk--;
                if ((k == i) || 
                    (!(prbbWalk->fStyle & (RBBS_HIDDEN | RBBS_FIXEDSIZE)) &&
                     (prbbWalk->cx > prbb->cxMin)))
                {
                    // the k == i check  means we've made it to the first
                    // band on this row and so he has to get the cx change
                    if (prbbWalk->fStyle & RBBS_HIDDEN) 
                    {
                        ASSERT(k == i);
                        prbbWalk = RBBNextVisible(prb, prbbWalk);
                        if (!prbbWalk)
                            break;
                    }
                    prbbWalk->cx += cxBar - cxRow;
                    break;
                }
            }
            TraceMsg(TF_REBAR, "RBRecalc: after grow i=%d", i);            
        }

        // items from index i to index j-1 (inclusive) WILL fit on one line
        cy = RBGetLineHeight(prb, i, j - 1);

        fChanged = FALSE; // set if any bands on current row changed position

        for ( ; i < j; i++, prbb++)
        {
            if (prbb->fStyle & RBBS_HIDDEN)
                continue;

            // go through row of bands, updating positions and heights,
            // invalidating as needed
            if ((prbb->y != y) || (prbb->x != x) || (prbb->cy != cy))
            {
                TraceMsg(TF_REBAR, "RBRecalc: invalidate i=%d", RBBANDTOINDEX(prb, prbb));
                fChanged = TRUE;
                rc.left = min(prbb->x, x);
                rc.top = min(prbb->y, y);
                rc.right = cxBar;
                rc.bottom = max(prbb->y + prbb->cy, y + cy);
                if (fBandBorders)
                {
                    // acount for etch line that will need to move
                    rc.left -= g_cxEdge;
                    rc.bottom += g_cyEdge/2;
                }
                RBInvalidateRect(prb, &rc);
            }

            prbb->x = x;
            prbb->y = y;
            prbb->cy = cy;

            x += RBBANDWIDTH(prb, prbb);
        }

        // i and prbb now refer to the first band in the next row of bands
        y += cy;
    }

    prb->cy = y;
    return(y);
}

// ----------------------------------------------------------------------------
//
// RBResize
//
// recomputes bounding rects for all bands and then resizes rebar and children
// based on these rects
//
// ----------------------------------------------------------------------------
void  RBResizeNow(PRB prb)
{
    RECT rc;
    BOOL bMirroredWnd=(prb->ci.dwExStyle&RTL_MIRRORED_WINDOW);

    if (!prb || !prb->ci.hwndParent)
        return;

    GetWindowRect(prb->ci.hwnd, &rc);

    //
    // If this is a mirrored window, we don't won't to refect the
    // coordinates since they are coming from the screen coord
    // which they are not mirrored. [samera]
    //
    if (bMirroredWnd)
        MapRectInRTLMirroredWindow(&rc, prb->ci.hwndParent);
    else 
        MapWindowPoints(HWND_DESKTOP, prb->ci.hwndParent, (LPPOINT)&rc, 2);

    RBResizeChildren(prb);

    NewSize(prb->ci.hwnd, prb->cy, prb->ci.style, rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc));

    if (prb->fResizeNotify) 
        CCSendNotify(&prb->ci, RBN_HEIGHTCHANGE, NULL);
    prb->fResizeNotify = FALSE;
    prb->fResizePending = FALSE;
}

void  RBResize(PRB prb, BOOL fForceHeightChange)
{
    UINT cy;

StartOver:
    // lots of the code relies on having cy calculated synchronously with RBResize,
    // but we're going to delay the actual changing of the window
    cy = prb->cy;
    if (prb->fResizing)
    {
        prb->fResizeRecursed = TRUE;
        return;
    }
    prb->fResizing = TRUE;
    
    RBRecalc(prb);    

   // true overrides always
    if (fForceHeightChange || (cy != prb->cy))
        prb->fResizeNotify = TRUE;

    if (prb->fRedraw) {
        RBResizeNow(prb);
    } else 
        prb->fResizePending = TRUE;
        
    prb->fResizing = FALSE;
    
    // we do this to avoid infinite loop...  RBResize can cause NewSize which causes 
    // a notify in which the parent sizes us, which causes us to loop.
    // if the parent does any message pumping during the NewSize, we're in a loop 
    if (prb->fResizeRecursed) {
        prb->fResizeRecursed = FALSE;
        fForceHeightChange = FALSE;
        goto StartOver;
    }     
}

void RBSetRecalc(PRB prb, BOOL fRecalc)
{
    prb->fRecalc = fRecalc;
    if (fRecalc) {
        if (prb->fRecalcPending)
            RBRecalc(prb);
    }
}

BOOL RBSetRedraw(PRB prb, BOOL fRedraw)
{
    BOOL fOld = prb->fRedraw;
    if (prb) {
        prb->fRedraw = BOOLIFY(fRedraw);
        if (fRedraw) {
            // save off prb->fRefreshPending since this can
            // get changed by call to RBResizeNow
            BOOL fRefreshPending = prb->fRefreshPending;

            if (prb->fResizePending)
                RBResizeNow(prb);

            if (fRefreshPending)
                RBInvalidateRect(prb, NULL);
        }
    }
    
    return fOld;
}

BOOL RBAfterSetFont(PRB prb)
{
    TEXTMETRIC tm;
    BOOL fChange = FALSE;
    UINT        i;
    HFONT hOldFont;
    
    HDC hdc = GetDC(prb->ci.hwnd);
    if (!hdc)
        return FALSE;

    hOldFont = SelectObject(hdc, prb->hFont);
    GetTextMetrics(hdc, &tm);

    if (prb->cyFont != (UINT) tm.tmHeight)
    {
        prb->cyFont = tm.tmHeight;
        fChange = TRUE;
    }

    // adjust bands
    for (i = 0; i < prb->cBands; i++)
    {
        if (RBGETBAND(prb, i)->fStyle & RBBS_HIDDEN)
            continue;

        fChange |= RBBCalcTextExtent(prb, RBGETBAND(prb, i), hdc);
    }

    SelectObject(hdc, hOldFont);
    ReleaseDC(prb->ci.hwnd, hdc);

    if (fChange)
    {
        RBResize(prb, FALSE);
        // invalidate, o.w. title doesn't redraw 1st time after font growth
        RBInvalidateRect(prb, NULL);
    }

    return TRUE;
}

BOOL RBOnSetFont(PRB prb, HFONT hFont)
{
    if (prb->fFontCreated) {
        DeleteObject(prb->hFont);
    }
    
    prb->hFont = hFont;
    prb->fFontCreated = FALSE;
    if (!prb->hFont)
        RBSetFont(prb, 0);
    else 
        return RBAfterSetFont(prb);
    
    return TRUE;
}

// ----------------------------------------------------------------------------
//
// RBSetFont
//
// sets the rebar band title font to the current system-wide caption font
//
// ----------------------------------------------------------------------------
BOOL  RBSetFont(PRB prb, WPARAM wParam)
{
    NONCLIENTMETRICS ncm;
    HFONT hOldFont;

    if ((wParam != 0) && (wParam != SPI_SETNONCLIENTMETRICS))
        return(FALSE);

    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    if (!SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0))
        return(FALSE);

    hOldFont = prb->hFont;

    ncm.lfCaptionFont.lfWeight = FW_NORMAL;
    if (!(prb->hFont = CreateFontIndirect(&ncm.lfCaptionFont)))
    {
        prb->hFont = hOldFont;
        return(FALSE);
    }

    prb->fFontCreated = TRUE;
    if (hOldFont)
        DeleteObject(hOldFont);
    
    return RBAfterSetFont(prb);
}

// ----------------------------------------------------------------------------
//
//  Draws a horizontal or vertical dotted line from the given (x,y) location
//  for the given length (c). (From TReeView's TV_DrawDottedLine)
//
// ----------------------------------------------------------------------------

void RBVertMungeGripperRect(PRB prb, LPRECT lprc)
{
    if (RB_ISVERTICALGRIPPER(prb)) {
        OffsetRect(lprc, -lprc->left + lprc->top, -lprc->top + lprc->left);
        lprc->bottom -= g_cyEdge;
    } else {
        FlipRect(lprc);
    }
}

void RBDrawChevron(PRB prb, PRBB prbb, HDC hdc)
{
    RECT rc;
    DWORD dwFlags = prbb->wChevState | DCHF_HORIZONTAL | DCHF_TRANSPARENT;

    CopyRect(&rc, &prbb->rcChevron);
    if (RB_ISVERT(prb))
        FlipRect(&rc);
    else
        dwFlags |= DCHF_TOPALIGN;

    DrawChevron(hdc, &rc, dwFlags);
}

void RBUpdateChevronState(PRB prb, PRBB prbb, WORD wControlState)
{
    if (!prb || !prbb)
        return;

    // if no change in state, bail
    if (!(wControlState ^ prbb->wChevState))
        return;

    prbb->wChevState = wControlState;

    // if active (pushed or hottracked)
    if (!(wControlState & DCHF_INACTIVE)) {
        // then we're now the hot band
        prb->prbbHot = prbb;
    }
    // else if we were the hot band then clear
    else if (prbb == prb->prbbHot) {
        prb->prbbHot = NULL;
    }

    // clear background & repaint
    RBInvalidateRect(prb, &prbb->rcChevron);
    UpdateWindow(prb->ci.hwnd);
}

// ----------------------------------------------------------------------------
//
// RBDrawBand
//
// draws the title icon and title text of the given band into the given DC;
// also the band's chevron
//
// ----------------------------------------------------------------------------
void  RBDrawBand(PRB prb, PRBB prbb, HDC hdc)
{
    int                 xStart, yCenter;
    COLORREF            clrBackSave, clrForeSave;
    int                 iModeSave;
    BOOL                fVertical = RB_ISVERT(prb);
    BOOL                fDrawHorizontal = (!fVertical || RB_ISVERTICALGRIPPER(prb));
    NMCUSTOMDRAW        nmcd;
    LRESULT             dwRet;

    if (prbb->fStyle & RBBS_HIDDEN) {
        ASSERT(0);
        return;
    }

    clrForeSave = SetTextColor(hdc, RBB_GetTextColor(prb, prbb));
    clrBackSave = SetBkColor(hdc, RBB_GetBkColor(prb, prbb));
    if (prbb->hbmBack)
        iModeSave = SetBkMode(hdc, TRANSPARENT);

    nmcd.hdc = hdc;
    nmcd.dwItemSpec = prbb->wID;
    nmcd.uItemState = 0;
    nmcd.lItemlParam = prbb->lParam;
    nmcd.rc.top = prbb->y;
    nmcd.rc.left = prbb->x;
    nmcd.rc.bottom = nmcd.rc.top + prbb->cy;
    nmcd.rc.right = nmcd.rc.left + RBBHEADERWIDTH(prbb);
    if (prb->ci.style & CCS_VERT)
    {
        FlipRect(&nmcd.rc);
    }
#ifdef KEYBOARDCUES
#if 0
    // BUGBUG: Custom draw stuff for UISTATE (stephstm)
    if (CCGetUIState(&(prb->ci), KC_TBD))
        nmcd.uItemState |= CDIS_SHOWKEYBOARDCUES;
#endif
#endif
    dwRet = CICustomDrawNotify(&prb->ci, CDDS_ITEMPREPAINT, &nmcd);

    if (!(dwRet & CDRF_SKIPDEFAULT))
    {
        int cy;
        
        if (RB_ISVERTICALGRIPPER(prb)) {
            cy = RBBHEADERWIDTH(prbb);
            yCenter = prbb->x + (cy / 2);
        } else {
            cy = prbb->cy;
            yCenter = prbb->y + (cy / 2);
        }

        xStart = prbb->x;
        if (RBShouldDrawGripper(prb, prbb))
        {
            RECT rc;
            int  c;
            int dy;

            c = 3 * g_cyBorder;
            xStart += 2 * g_cxBorder;
            dy = g_cxEdge;

            SetRect(&rc, xStart, prbb->y + dy, xStart + c, prbb->y + cy - dy);

            if (fVertical)
            {
                RBVertMungeGripperRect(prb, &rc);
                if (RB_ISVERTICALGRIPPER(prb))
                    xStart = rc.left;
            }

            CCDrawEdge(hdc, &rc, BDR_RAISEDINNER, BF_RECT | BF_MIDDLE, &(prb->clrsc));

            xStart += c;
        }

        xStart += 2 * (fVertical ? g_cyEdge : g_cxEdge);


        if (prbb->iImage != -1)
        { 
            UINT                yStart;
            IMAGELISTDRAWPARAMS imldp = {0};

            yStart = yCenter - ((!fDrawHorizontal ? prb->cxImage : prb->cyImage) / 2);
            imldp.cbSize = sizeof(imldp);
            imldp.himl   = prb->himl;
            imldp.i      = prbb->iImage;
            imldp.hdcDst = hdc;
            imldp.x      = (!fDrawHorizontal ? yStart : xStart);
            imldp.y      = (!fDrawHorizontal ? xStart : yStart);
            imldp.rgbBk  = CLR_DEFAULT;
            imldp.rgbFg  = CLR_DEFAULT;
            imldp.fStyle = ILD_TRANSPARENT;

            ImageList_DrawIndirect(&imldp);
            xStart +=  (fDrawHorizontal ? (prb->cxImage + g_cxEdge) : (prb->cyImage + g_cyEdge));
        }

        if (RBSHOWTEXT(prbb))
        {
            HFONT hFontSave = SelectObject(hdc, prb->hFont);
            RECT rcText;
            
            rcText.left = fDrawHorizontal ? xStart : yCenter - (prbb->cxText / 2);
            rcText.top = fDrawHorizontal ? yCenter - (prb->cyFont / 2) : xStart;
            if (fDrawHorizontal)
                rcText.top -= 1;    // fudge
            rcText.right = rcText.left + prbb->cxText;
            rcText.bottom = rcText.top + prb->cyFont;

            // for clients >= v5, we draw text with prefix processing (& underlines next char)
            if (prb->ci.iVersion >= 5)
            {
                UINT uFormat=0;
#ifdef KEYBOARDCUES
                if (CCGetUIState(&(prb->ci)) & UISF_HIDEACCEL)
                   uFormat= DT_HIDEPREFIX;
#endif
                DrawText(hdc, prbb->lpText, lstrlen(prbb->lpText), &rcText, uFormat);
            }
            else
                TextOut(hdc, rcText.left, rcText.top, prbb->lpText, lstrlen(prbb->lpText));

            SelectObject(hdc, hFontSave);
        }

        // maybe draw chevron
        if (RBBUSECHEVRON(prb, prbb) && prbb->fChevron)
            RBDrawChevron(prb, prbb, hdc);
    }

    if (dwRet & CDRF_NOTIFYPOSTPAINT)
        CICustomDrawNotify(&prb->ci, CDDS_ITEMPOSTPAINT, &nmcd);

    if (prbb->hbmBack)
        SetBkMode(hdc, iModeSave);
    SetTextColor(hdc, clrForeSave);
    SetBkColor(hdc, clrBackSave);

}

// ----------------------------------------------------------------------------
//
// RBPaint
//
// processes WM_PAINT message
//
// ----------------------------------------------------------------------------
void  RBPaint(PRB prb, HDC hdcIn)
{
    HDC         hdc = hdcIn;
    PAINTSTRUCT ps;
    UINT        i;
    NMCUSTOMDRAW    nmcd;

    if (!hdcIn)
        hdc = BeginPaint(prb->ci.hwnd, &ps);
    else
        GetClientRect(prb->ci.hwnd, &ps.rcPaint);

    nmcd.hdc = hdc;
    nmcd.uItemState = 0;
    nmcd.lItemlParam = 0;
    nmcd.rc = ps.rcPaint;
    prb->ci.dwCustom = CICustomDrawNotify(&prb->ci, CDDS_PREPAINT, &nmcd);

    if (!(prb->ci.dwCustom & CDRF_SKIPDEFAULT))
    {
        for (i = 0; i < prb->cBands; i++) {
            if (RBGETBAND(prb, i)->fStyle & RBBS_HIDDEN)
                continue;

            RBDrawBand(prb, RBGETBAND(prb, i), hdc);
        }
    }

    if (prb->ci.dwCustom & CDRF_NOTIFYPOSTPAINT)
        CICustomDrawNotify(&prb->ci, CDDS_POSTPAINT, &nmcd);

    if (!hdcIn)
        EndPaint(prb->ci.hwnd, &ps);
}

// ----------------------------------------------------------------------------
//
// RBTileBlt
//
// Fills the given rectangle with the rebar's background bitmap, tiling if
// necessary
//
// ----------------------------------------------------------------------------
void  RBTileBlt(PRB prb, PRBB prbb, UINT x, UINT y, UINT cx, UINT cy, HDC hdcDst, HDC hdcSrc)
{
    UINT xOff = 0;
    UINT yOff = 0;
    BOOL fxTile, fyTile;
    int cxPart, cyPart;
    int iPixelOffset = 0;

#ifndef WINNT
    // On Win98 BiDi, Bitblt has off-by-one bug in mirroring
    if(IS_DC_RTL_MIRRORED(hdcSrc))
    {
        iPixelOffset = 1;
    }
#endif // WINNT
    if (!(prbb->fStyle & RBBS_FIXEDBMP))
    {
        if (prb->ci.style & CCS_VERT)
        {
            xOff = -prbb->y;
            yOff = -prbb->x;
        }
        else
        {
            xOff = -prbb->x;
            yOff = -prbb->y;
        }
    }

    xOff += x;
    if (xOff >= prbb->cxBmp)
        xOff %= prbb->cxBmp;

    yOff += y;
    if (yOff >= prbb->cyBmp)
        yOff %= prbb->cyBmp;

ReCheck:
    fxTile = ((xOff + cx) > prbb->cxBmp);
    fyTile = ((yOff + cy) > prbb->cyBmp);

    if (!fxTile && !fyTile)
    {
        // no tiling needed -- blt and leave
        BitBlt(hdcDst, x , y, cx, cy, hdcSrc, xOff + iPixelOffset, yOff, SRCCOPY);
        return;
    }

    if (!fxTile)
    {
        // vertically tile
        cyPart = prbb->cyBmp - yOff;
        BitBlt(hdcDst, x, y, cx, cyPart, hdcSrc, xOff + iPixelOffset, yOff, SRCCOPY);
        y += cyPart;
        cy -= cyPart;
        yOff = 0;
        goto ReCheck;
    }

    if (!fyTile)
    {
        // horizontally tile
        cxPart = prbb->cxBmp - xOff;
        BitBlt(hdcDst, x, y, cxPart, cy, hdcSrc, xOff + iPixelOffset, yOff, SRCCOPY);
        x += cxPart;
        cx -= cxPart;
        xOff = 0;
        goto ReCheck;
    }

    // tile both ways
    cyPart = prbb->cyBmp - yOff;
    RBTileBlt(prb, prbb, x, y, cx, cyPart, hdcDst, hdcSrc);
    y += cyPart;
    cy -= cyPart;
    yOff = 0;
    goto ReCheck;
}

// this is using virtual coordinate space (internal always horizontal)
int _RBHitTest(PRB prb, LPRBHITTESTINFO prbht, int x, int y)
{
    BOOL fVert = (prb->ci.style & CCS_VERT);
    int i;
    PRBB prbb = RBGETBAND(prb, 0);
    int  cx;
    RBHITTESTINFO rbht;
    
    if (!prbht)
        prbht = &rbht;

    for (i = 0; i < (int) prb->cBands; i++, prbb++)
    {
        if (prbb->fStyle & RBBS_HIDDEN)
            continue;

        if ((x >= prbb->x) && (y >= prbb->y) &&
            (x <= (prbb->x + prbb->cx)) && (y <= (prbb->y + prbb->cy)))
        {
            cx = RBBHEADERWIDTH(prbb);
            if (x <= (int) (prbb->x + cx))
            {
                prbht->flags = RBHT_CAPTION;
                
                if (RB_ISVERTICALGRIPPER(prb)) {
                    if (y - prbb->y < RB_GRABWIDTH)
                        prbht->flags = RBHT_GRABBER;
                } else {
                    cx = RB_GRABWIDTH * (fVert ? g_cyBorder : g_cxBorder);
                    if (RBShouldDrawGripper(prb, RBGETBAND(prb, i)) &&
                        (x <= (int) (prbb->x + cx)))
                        prbht->flags = RBHT_GRABBER;
                }
            }
            else
            {
                POINT pt;

                pt.x = x;
                pt.y = y;

                if (RBBUSECHEVRON(prb, prbb) && prbb->fChevron && PtInRect(&prbb->rcChevron, pt))
                    prbht->flags = RBHT_CHEVRON;
                else
                    prbht->flags = RBHT_CLIENT;
            }

            prbht->iBand = i;
            return(i);
            break;
        }
    }

    prbht->flags = RBHT_NOWHERE;
    prbht->iBand = -1;
    return(-1);
}

// ----------------------------------------------------------------------------
//
// RBHitTest
//
// returns the index to the band that the given point lies in, or -1 if outside
// of all bands.  Also, sets flags to indicate which part of the band the
// point lies in.
//
// ----------------------------------------------------------------------------
int RBHitTest(PRB prb, LPRBHITTESTINFO prbht)
{
    BOOL fVert = (prb->ci.style & CCS_VERT);
    POINT pt;

    if (fVert)
    {
        pt.x = prbht->pt.y;
        pt.y = prbht->pt.x;
    }
    else
        pt = prbht->pt;
    
    return _RBHitTest(prb, prbht, pt.x, pt.y);
}


// ----------------------------------------------------------------------------
//
// RBEraseBkgnd
//
// processes WM_ERASEBKGND message by drawing band borders, if necessary, and
// filling in the rebar bands with their background color
//
// ----------------------------------------------------------------------------
BOOL  RBEraseBkgnd(PRB prb, HDC hdc, int iBand)
{
    BOOL fVertical = (prb->ci.style & CCS_VERT);
    NMCUSTOMDRAW    nmcd;
    LRESULT         dwItemRet;
    BOOL            fBandBorders;
    RECT            rcClient;
    HDC             hdcMem = NULL;
    UINT            i;
    PRBB            prbb = RBGETBAND(prb, 0);

    nmcd.hdc = hdc;
    nmcd.uItemState = 0;
    nmcd.lItemlParam = 0;
    prb->ci.dwCustom = CICustomDrawNotify(&prb->ci, CDDS_PREERASE, &nmcd);

    if (!(prb->ci.dwCustom & CDRF_SKIPDEFAULT))
    {
        COLORREF clrBk;

        fBandBorders = (prb->ci.style & RBS_BANDBORDERS);
        GetClientRect(prb->ci.hwnd, &rcClient);

        clrBk = RB_GetBkColor(prb);
        if (clrBk != CLR_NONE) {
            FillRectClr(hdc, &rcClient, clrBk);
        }

        for (i = 0; i < prb->cBands; i++, prbb++)
        {
            if (prbb->fStyle & RBBS_HIDDEN)
                continue;

            if (fVertical)
                SetRect(&nmcd.rc, prbb->y, prbb->x, prbb->y + prbb->cy, prbb->x + prbb->cx);
            else
                SetRect(&nmcd.rc, prbb->x, prbb->y, prbb->x + prbb->cx, prbb->y + prbb->cy);

            if (fBandBorders)
            {
                if (prbb->x)
                {
                    // draw etch between bands on same row
                    if (fVertical)
                    {
                        nmcd.rc.right += g_cxEdge / 2;
                        nmcd.rc.top -= g_cyEdge;
                        CCDrawEdge(hdc, &nmcd.rc, EDGE_ETCHED, BF_TOP, &(prb->clrsc));
                        nmcd.rc.right -= g_cxEdge / 2;
                        nmcd.rc.top += g_cyEdge;
                    }
                    else
                    {
                        nmcd.rc.bottom += g_cyEdge / 2;
                        nmcd.rc.left -= g_cxEdge;
                        CCDrawEdge(hdc, &nmcd.rc, EDGE_ETCHED, BF_LEFT, &(prb->clrsc));
                        nmcd.rc.bottom -= g_cyEdge / 2;
                        nmcd.rc.left += g_cxEdge;
                    }
                }
                else
                {
                    // draw etch between rows
                    if (fVertical)
                    {
                        rcClient.right = prbb->y + prbb->cy + g_cxEdge;
                        CCDrawEdge(hdc, &rcClient, EDGE_ETCHED, BF_RIGHT, &(prb->clrsc));
                    }
                    else
                    {
                        rcClient.bottom = prbb->y + prbb->cy + g_cyEdge;
                        CCDrawEdge(hdc, &rcClient, EDGE_ETCHED, BF_BOTTOM, &(prb->clrsc));
                    }
                }
            }

            nmcd.dwItemSpec = prbb->wID;
            nmcd.uItemState = 0;
            dwItemRet = CICustomDrawNotify(&prb->ci, CDDS_ITEMPREERASE, &nmcd);

            if (!(dwItemRet & CDRF_SKIPDEFAULT))
            {
                if (prbb->hbmBack)
                {
                    if (!hdcMem)
                    {
                        if (!(hdcMem = CreateCompatibleDC(hdc)))
                            continue;

                        RBRealize(prb, hdc, TRUE, FALSE);
                    }

                    SelectObject(hdcMem, prbb->hbmBack);

                    RBTileBlt(prb, prbb, nmcd.rc.left, nmcd.rc.top, nmcd.rc.right - nmcd.rc.left,
                            nmcd.rc.bottom - nmcd.rc.top, hdc, hdcMem);
                }
                else
                {
                    // if the color for this band is the same as the 
                    // rebar's default background color, then we
                    // don't need to paint this specially
                    COLORREF clr = RBB_GetBkColor(prb, prbb);
                    if (clr != RB_GetBkColor(prb)) {
                        FillRectClr(hdc, &nmcd.rc, clr);
                    }
                }
            }

            if (dwItemRet & CDRF_NOTIFYPOSTERASE)
                CICustomDrawNotify(&prb->ci, CDDS_ITEMPOSTERASE, &nmcd);
        }

        if (hdcMem)
        {
            DeleteDC(hdcMem);
        }
    }

    if (prb->ci.dwCustom & CDRF_NOTIFYPOSTERASE)
    {
        nmcd.uItemState = 0;
        nmcd.dwItemSpec = 0;
        nmcd.lItemlParam = 0;
        CICustomDrawNotify(&prb->ci, CDDS_POSTERASE, &nmcd);
    }

    return(TRUE);
}

// ----------------------------------------------------------------------------
//
// RBGetBarInfo
//
// retrieves the indicated values from the rebar's internal structure
//
// ----------------------------------------------------------------------------
BOOL  RBGetBarInfo(PRB prb, LPREBARINFO lprbi)
{
    if (!prb || (lprbi->cbSize != sizeof(REBARINFO)))
        return(FALSE);

    if (lprbi->fMask & RBIM_IMAGELIST)
        lprbi->himl = prb->himl;

    return(TRUE);
}


// ----------------------------------------------------------------------------
//
// RBSetBarInfo
//
// sets the indicated values in the rebar's internal structure, recalculating
// and refreshing as needed
//
// ----------------------------------------------------------------------------
BOOL  RBSetBarInfo(PRB prb, LPREBARINFO lprbi)
{
    if (!prb || (lprbi->cbSize != sizeof(REBARINFO)))
        return(FALSE);

    if (lprbi->fMask & RBIM_IMAGELIST)
    {
        HIMAGELIST himl = prb->himl;
        UINT    cxOld, cyOld;

        //todo:validate lprbi->himl
        prb->himl = lprbi->himl;
        cxOld = prb->cxImage;
        cyOld = prb->cyImage;
        ImageList_GetIconSize(prb->himl, (LPINT)&prb->cxImage, (LPINT)&prb->cyImage);
        if ((prb->cxImage != cxOld) || (prb->cyImage != cyOld))
        {
            UINT i;

            for (i = 0; i < prb->cBands; i++) {
                if (RBGETBAND(prb, i)->fStyle & RBBS_HIDDEN)
                    continue;

                RBBCalcMinWidth(prb, RBGETBAND(prb, i));
            }

            RBResize(prb, FALSE);
        }
        else
            RBInvalidateRect(prb, NULL);
        lprbi->himl = himl;
    }

    return(TRUE);
}

// ----------------------------------------------------------------------------
//
// RBGetBandInfo
//
// retrieves the indicated values from the specified band's internal structure
//
// ----------------------------------------------------------------------------
BOOL  RBGetBandInfo(PRB prb, UINT uBand, LPREBARBANDINFO lprbbi)
{
    PRBB prbb;

    if (!prb || (!RB_ISVALIDINDEX(prb, uBand)) || (lprbbi->cbSize > SIZEOF(REBARBANDINFO)))
        return(FALSE);

    prbb = RBGETBAND(prb, uBand);

    if (lprbbi->fMask & RBBIM_SIZE) {
        if (prbb->fStyle & RBBS_FIXEDSIZE)
            lprbbi->cx = prbb->cx;
        else 
            lprbbi->cx = prbb->cxRequest;
    }
    
    if (lprbbi->fMask & RBBIM_HEADERSIZE) 
        lprbbi->cxHeader = RBBHEADERWIDTH(prbb);
    
    if (lprbbi->fMask & RBBIM_IDEALSIZE)
        // HACKHACK: (tjgreen) Subtract the offset we added in SetBandInfo (see 
        // comments there).
        lprbbi->cxIdeal = prbb->cxIdeal ? prbb->cxIdeal - CX_OFFSET : 0;

    if (lprbbi->fMask & RBBIM_STYLE)
        lprbbi->fStyle = prbb->fStyle;

    if (lprbbi->fMask & RBBIM_COLORS)
    {
        lprbbi->clrFore = RBB_GetTextColor_External(prb, prbb);
        lprbbi->clrBack = RBB_GetBkColor_External(prb, prbb);
    }

    if (lprbbi->fMask & RBBIM_TEXT)
    {
        UINT cch = prbb->lpText ? lstrlen(prbb->lpText) : 0;

        if (!lprbbi->cch || !lprbbi->lpText || (lprbbi->cch <= cch))
            lprbbi->cch = cch + 1;
        else if (prbb->lpText)
            lstrcpy(lprbbi->lpText, prbb->lpText);
        else
            // no text -- so just make it an empty string
            lprbbi->lpText[0] = 0;
    }

    if (lprbbi->fMask & RBBIM_IMAGE)
        lprbbi->iImage = prbb->iImage;

    if (lprbbi->fMask & RBBIM_CHILD)
        lprbbi->hwndChild = prbb->hwndChild;

    if (lprbbi->fMask & RBBIM_CHILDSIZE)
    {
        // HACKHACK: (tjgreen) Subtract the offset we added in SetBandInfo (see
        // comments there).
        lprbbi->cxMinChild = prbb->cxMinChild ? prbb->cxMinChild - CX_OFFSET : 0;
        lprbbi->cyMinChild = prbb->cyMinChild;
        
        if (prbb->fStyle & RBBS_VARIABLEHEIGHT) {
            lprbbi->cyIntegral = prbb->cyIntegral;
            lprbbi->cyMaxChild = prbb->cyMaxChild;
            lprbbi->cyChild = prbb->cyChild;
        }
    }

    if (lprbbi->fMask & RBBIM_BACKGROUND)
        lprbbi->hbmBack = prbb->hbmBack;

    if (lprbbi->fMask & RBBIM_ID)
        lprbbi->wID = prbb->wID;

    if (lprbbi->fMask & RBBIM_LPARAM)
        lprbbi->lParam = prbb->lParam;

    return(TRUE);
}

BOOL RBValidateBandInfo(LPREBARBANDINFO *pprbbi, LPREBARBANDINFO prbbi)
{
    BOOL fRet = ((*pprbbi)->cbSize == sizeof(REBARBANDINFO));
    
    if (!fRet) {
        
        if ((*pprbbi)->cbSize < SIZEOF(REBARBANDINFO)) {
            hmemcpy(prbbi, (*pprbbi), (*pprbbi)->cbSize);
            (*pprbbi) = prbbi;
            prbbi->cbSize = SIZEOF(REBARBANDINFO);
            fRet = TRUE;
        }
    }

    return fRet;
}

// ----------------------------------------------------------------------------
//
// RBSetBandInfo
//
// sets the indicated values in the specified band's internal structure,
// recalculating and refreshing as needed
//
// ----------------------------------------------------------------------------
BOOL  RBSetBandInfo(PRB prb, UINT uBand, LPREBARBANDINFO lprbbi, BOOL fAllowRecalc)
{
    PRBB    prbb;
    BOOL    fRefresh = FALSE;
    BOOL    fRecalc  = FALSE;
    BOOL    fRecalcMin = FALSE;
    BOOL    fTextChanged = FALSE;
    REBARBANDINFO rbbi = {0};
    RECT    rc;

    if (!prb || (!RB_ISVALIDINDEX(prb, uBand)) || 
        !RBValidateBandInfo(&lprbbi, &rbbi))
        return(FALSE);

    prbb = RBGETBAND(prb, uBand);

    if (lprbbi->fMask & RBBIM_TEXT)
    {
        if (!lprbbi->lpText || !prbb->lpText || lstrcmp(lprbbi->lpText, prbb->lpText))
        {
            if (lprbbi->lpText != prbb->lpText) {
                Str_Set(&prbb->lpText, lprbbi->lpText);
                fTextChanged = TRUE;
            }
        }
    }

    if (lprbbi->fMask & RBBIM_STYLE)
    {
        UINT fStylePrev = prbb->fStyle;
        UINT fChanged = lprbbi->fStyle ^ fStylePrev;

        prbb->fStyle = lprbbi->fStyle;

        if (fChanged)
            fRecalc = TRUE;

        if ((prbb->fStyle & RBBS_FIXEDSIZE) && !(fStylePrev & RBBS_FIXEDSIZE))
            prbb->cxMin = prbb->cx;
        else if (fChanged & RBBS_FIXEDSIZE)
            fRecalcMin = TRUE;
        
        if (fChanged & RBBS_GRIPPERALWAYS)
            fRecalcMin = TRUE;
        
        if (fChanged & RBBS_HIDDEN) 
            RBShowBand(prb, uBand, !(prbb->fStyle & RBBS_HIDDEN));

        if (fChanged & RBBS_HIDETITLE)
            fTextChanged = TRUE;

        // can't have both of these
        if (prbb->fStyle & RBBS_FIXEDSIZE)
            prbb->fStyle &= ~RBBS_BREAK;
        
    }

    // RBBIM_TEXT does calculations that want to take some RBBIM_STYLE bits
    // into account, so delay those calculations until we grab the style bits.
    //
    if (fTextChanged && !(prbb->fStyle & RBBS_HIDDEN))
    {
        if (RBBCalcTextExtent(prb, prbb, NULL))
            fRecalc = TRUE;
        else
            fRefresh = TRUE;
    }

    if (lprbbi->fMask & RBBIM_IDEALSIZE)
    {
        // HACKHACK: (tjgreen) Add an offset to the width the caller specifies.
        // This offset gets clipped off in RBResizeChildren, so the child window is  
        // rendered with the width specified by caller, and we get a little space on 
        // the toolbar after the buttons.  If caller specifies zero-width, though, 
        // we don't want this extra space, so don't add offset.
        int cxIdeal = lprbbi->cxIdeal ? lprbbi->cxIdeal + CX_OFFSET : 0;
        if (cxIdeal != prbb->cxIdeal) {
            prbb->cxIdeal = cxIdeal;
            fRecalcMin = TRUE;
            fRecalc = TRUE;
        }
    }
    
    if (lprbbi->fMask & RBBIM_SIZE)
    {
        if (prbb->cxRequest != (int) lprbbi->cx)
        {
            fRecalc = TRUE;
            prbb->cxRequest = (int) lprbbi->cx;
        }

        if (prbb->fStyle & RBBS_FIXEDSIZE)
            prbb->cxMin = prbb->cxRequest;
    }
    
    if (lprbbi->fMask & RBBIM_HEADERSIZE)
    {
        if ((lprbbi->cxHeader == -1) ||
            !(prbb->fStyle & RBBS_FIXEDHEADERSIZE) ||
            ((UINT)prbb->cxMin != lprbbi->cxHeader + prbb->cxMinChild)) {

            if (lprbbi->cxHeader == -1) {
                prbb->fStyle &= ~RBBS_FIXEDHEADERSIZE;
                fRecalcMin = TRUE;
            } else {
                prbb->fStyle |= RBBS_FIXEDHEADERSIZE;
                prbb->cxMin = lprbbi->cxHeader + prbb->cxMinChild;
            }

            fRecalc = TRUE;
            fRefresh = TRUE;
        }
    }

    if (lprbbi->fMask & RBBIM_COLORS)
    {
        prbb->clrFore = lprbbi->clrFore;
        prbb->clrBack = lprbbi->clrBack;
        fRefresh = TRUE;
    }

    if ((lprbbi->fMask & RBBIM_IMAGE) && (prbb->iImage != lprbbi->iImage))
    {
        BOOL fToggleBmp = ((prbb->iImage == -1) || (lprbbi->iImage == -1));

        prbb->iImage = lprbbi->iImage;

        if (fToggleBmp)
        {
            fRecalc = TRUE;
            fRecalcMin = TRUE;
        }
        else
            fRefresh = TRUE;
    }

    if (lprbbi->fMask & RBBIM_CHILD &&
        lprbbi->hwndChild != prbb->hwndChild &&
        (NULL == lprbbi->hwndChild || 
         !IsChild(lprbbi->hwndChild, prb->ci.hwnd)))
    {
        if (IsWindow(prbb->hwndChild))
            ShowWindow(prbb->hwndChild, SW_HIDE);        

        prbb->hwndChild = lprbbi->hwndChild;

        if (prbb->hwndChild)
        {
            SetParent(prbb->hwndChild, prb->ci.hwnd);
            ShowWindow(prbb->hwndChild, SW_SHOW);
        }
        fRecalc = TRUE;
    }

    if (lprbbi->fMask & RBBIM_CHILDSIZE)
    {
        UINT cyChildOld = prbb->cyChild;

        if (lprbbi->cyMinChild != -1)
            prbb->cyMinChild = lprbbi->cyMinChild;

        if (prbb->fStyle & RBBS_VARIABLEHEIGHT) {
            
            if (lprbbi->cyIntegral != -1)
                prbb->cyIntegral = lprbbi->cyIntegral;
            
            if (lprbbi->cyMaxChild != -1)
                prbb->cyMaxChild = lprbbi->cyMaxChild;
            
            if (lprbbi->cyChild != -1)
                prbb->cyChild = lprbbi->cyChild;

            if (prbb->cyChild < prbb->cyMinChild)
                prbb->cyChild = prbb->cyMinChild;
            if (prbb->cyChild > prbb->cyMaxChild)
                prbb->cyChild = prbb->cyMaxChild;

            // validate the child size.  cyChild must be cyMinChild plux n*cyIntegral
            if (prbb->cyIntegral) {
                int iExtra;
                iExtra = (prbb->cyChild - prbb->cyMinChild) % prbb->cyIntegral;
                prbb->cyChild -= iExtra;
            }
            
        } else {
            // if we're not in variable height mode, then 
            // the cyChild is the same as cyMinChild.  
            // this is a little peculiar, but done this way for backcompat.
            // cyMinChild came before cyChild
            prbb->cyChild = lprbbi->cyMinChild;
        }

        if (lprbbi->cxMinChild != (UINT)-1) {
            // HACKHACK: (tjgreen) Add an offset to the width the caller specifies.
            // This offset gets clipped off in RBResizeChildren, so the child window is  
            // rendered with the width specified by caller, and we get a little space on 
            // the toolbar after the buttons.  However, if caller specifies zero-width or
            // if the band is fixed size, we don't want this extra space, so don't add offset.
            UINT cxMinChild = lprbbi->cxMinChild;
            if ((lprbbi->cxMinChild != 0) && !(prbb->fStyle & RBBS_FIXEDSIZE))
                cxMinChild += CX_OFFSET;

            if (prbb->cxMinChild != cxMinChild) {
                int cxOldHeaderMin = RBBHEADERWIDTH(prbb);
                
                if (prbb->fStyle & RBBS_FIXEDSIZE)
                    fRecalc = TRUE;
                    
                prbb->cxMinChild = cxMinChild;
                
                if (prbb->fStyle & RBBS_FIXEDHEADERSIZE)
                    prbb->cxMin = cxOldHeaderMin + prbb->cxMinChild;
                
                fRecalcMin = TRUE;
            }
            
            if (cyChildOld != prbb->cyChild) {
                // TODO:  revisit optimization:
                // if (RBBGetHeight(prb, prbb) != (UINT) prbb->cy)
                fRecalc = TRUE;
            }
        }
            
    }

    if (lprbbi->fMask & RBBIM_BACKGROUND)
    {
        DIBSECTION  dib;

        if (lprbbi->hbmBack && !GetObject(lprbbi->hbmBack, sizeof(DIBSECTION), &dib))
            return(FALSE);

        prbb->hbmBack = lprbbi->hbmBack;
        prbb->cxBmp = dib.dsBm.bmWidth;
        prbb->cyBmp = dib.dsBm.bmHeight;
        fRefresh = TRUE;
    }

    if (lprbbi->fMask & RBBIM_ID)
        prbb->wID = lprbbi->wID;

    if (lprbbi->fMask & RBBIM_LPARAM)
        prbb->lParam = lprbbi->lParam;

    if (fRecalcMin && !(prbb->fStyle & RBBS_HIDDEN))
        RBBCalcMinWidth(prb, prbb);

    if (fAllowRecalc) {

        if (fRecalc)
            RBResize(prb, FALSE);
        if (fRefresh || fRecalc)
        {
            // '|| fRecalc' so we catch add/grow of text.
            // testcase: remove title from band; add back; make sure the text
            // shows up (used to just leave old band contents there)
            SetRect(&rc, prbb->x, prbb->y, prbb->x + prbb->cx, prbb->y + prbb->cy);
            RBInvalidateRect(prb, &rc);
        }
    }
    
    return(TRUE);
}

// ----------------------------------------------------------------------------
//
// RBReallocBands
//
// reallocates the array of bands pointed to by prb->rbbList to the given
// number of bands
//
// ----------------------------------------------------------------------------
BOOL  RBReallocBands(PRB prb, UINT cBands)
{
    PRBB rbbList;

    if (!(rbbList = (PRBB) CCLocalReAlloc(prb->rbbList, sizeof(RBB) * cBands)) && cBands)
        return(FALSE);

    prb->rbbList = rbbList;
    return(TRUE);
}

//
// NOTES
//  for now caller does this in two calls (query, set).  eventually we
//  should be able to have it do everything up front.
RBRecalcFirst(int nCmd, PRB prb, PRBB prbbDelHide)
{
    switch (nCmd) {
    case RBC_QUERY:
    {
        BOOL fRecalcFirst;
        // if we're nuking the 1st visible guy,
        // and there are visible guys after us,
        // then we need to recompute stuff
        //
        // for a testcase, start w/:
        //  row1: 'standard buttons' + 'brand'
        //  row2: 'address' + 'links'
        // now hide 'standard buttons', you should end up w/:
        //  row1: 'address' + 'links' + 'brand'
        // if there's a bug, you'll end up w/ (since the break isn't recomputed):
        //  row1: 'brand'
        //  row2: 'address' + 'links'
        // fRecalcFirst = (!uBand && prb->cBands);

        // if brbbDelHide is the first non-hidden band, and there are other non-hidden bands after it, fRecalcFirst = TRUE;
        fRecalcFirst = (RBEnumBand(prb, 0, RBBS_HIDDEN) == prbbDelHide) &&
                       (RBGetNextVisible(prb, prbbDelHide) <= RB_GETLASTBAND(prb));

        return fRecalcFirst;
    }

    case RBC_SET: // set
    {
        PRBB prbb1, prbb2;

        prbb1 = RBEnumBand(prb, 0, RBBS_HIDDEN);
        if ((prbb1->fStyle & RBBS_FIXEDSIZE)
          && (prbb2 = RBEnumBand(prb, 1, RBBS_HIDDEN)) <= RB_GETLASTBAND(prb)) {
            // get rid of line break on NEW first item
            prbb2->fStyle &= ~RBBS_BREAK;
        }

        if (prb->ci.style & RBS_FIXEDORDER) {
            // BUGBUG not sure what this does...
            // this is because the min width is now based on it's movability --
            // and since we are deleting (or hiding) the first item,
            // the new first item becomes immovable
            RBBCalcMinWidth(prb, prbb1);
        }
        return TRUE;
    }
    
    default:
        ASSERT(0);
    }

    return FALSE;
}

// ----------------------------------------------------------------------------
//
// RBShowBand
//
// updates show/hide state for the indicated band in the rebar's band array
// (rbbList).
//
// ----------------------------------------------------------------------------
BOOL  RBShowBand(PRB prb, UINT uBand, BOOL fShow)
{
    PRBB prbb;
    BOOL fRecalcFirst;

    if (!prb || (!RB_ISVALIDINDEX(prb, uBand)))
        return(FALSE);

    prbb = RBGETBAND(prb, uBand);

    // if we're nuking the 1st visible guy,
    // then we need to recompute stuff
    fRecalcFirst = RBRecalcFirst(RBC_QUERY, prb, prbb);

    if (fShow)
    {
        prbb->fStyle &= ~RBBS_HIDDEN;

        if (!RBBCalcTextExtent(prb, prbb, NULL))
            RBBCalcMinWidth(prb, prbb);

        if (prbb->hwndChild)
            ShowWindow(prbb->hwndChild, SW_SHOW);
    }
    else
    {
        prbb->fStyle |= RBBS_HIDDEN;
        if (prbb->hwndChild)
            ShowWindow(prbb->hwndChild, SW_HIDE);        
    }

    if (fRecalcFirst)
        RBRecalcFirst(RBC_SET, prb, NULL);

    RBInvalidateRect(prb, NULL);
    RBResize(prb, FALSE);
    RBAutoSize(prb);

    return(TRUE);
}


// ----------------------------------------------------------------------------
//
// RBDeleteBand
//
// deletes the indicated band from the rebar's band array (rbbList) and
// decrements the rebar's band count (cBands)
//
// ----------------------------------------------------------------------------
BOOL  RBDeleteBand(PRB prb, UINT uBand)
{
    PRBB prbb;
    PRBB prbbStop;
    BOOL fRecalcFirst;
    NMREBAR nm = {0};

    ASSERT(prb);

    // we need to clean up
    //
    // a) captured band and
    // b) hottracked band
    //
    // before we delete this band

    if (prb->iCapture != -1) {
        RBSendNotify(prb, prb->iCapture, RBN_ENDDRAG);
        RBOnBeginDrag(prb, (UINT)-1);
    }

    if (!RB_ISVALIDINDEX(prb, uBand))
        return FALSE;

    prbb = RBGETBAND(prb, uBand);

    // Notify the client of the delete
    RBSendNotify(prb, uBand, RBN_DELETINGBAND);

    nm.dwMask = RBNM_ID;
    nm.wID = RBGETBAND(prb, uBand)->wID;        // Save this

    Str_Set(&prbb->lpText, NULL);

    // don't destroy the hbmBack 'cause it's given to us by app

    // if we're nuking the 1st visible guy,
    // then we need to recompute stuff

    // if this is the first visible guy and there are other visible bands after it, fRecalcFirst = TRUE
    fRecalcFirst = RBRecalcFirst(RBC_QUERY, prb, prbb);

    if (IsWindow(prbb->hwndChild))
        ShowWindow(prbb->hwndChild, SW_HIDE);    
    
    // prbbStop gets the address of the last band
    prbbStop = RB_GETLASTBAND(prb);

    for ( ; prbb < prbbStop; prbb++)
        *prbb = *(prbb + 1);

    prb->cBands--;

    if (prb->uResizeNext >= uBand && prb->uResizeNext > 0) {
        // (defer RBBS_HIDDEN stuff to use of uResizeNext)
        prb->uResizeNext--;
    }


    // Notify the client of the delete
    CCSendNotify(&prb->ci, RBN_DELETEDBAND, &nm.hdr);

    if (fRecalcFirst)
        RBRecalcFirst(RBC_SET, prb, NULL);

    RBReallocBands(prb, prb->cBands);

    RBInvalidateRect(prb, NULL);
    RBResize(prb, FALSE);
    RBAutoSize(prb);
    return(TRUE);
}

// ----------------------------------------------------------------------------
//
// RBInsertBand
//
// inserts a new band at the given position in the rebar's band array (rbbList),
// increments the rebar's band count (cBands), and sets the band's structure
// based on the given REBARBANDINFO structure.
//
// ----------------------------------------------------------------------------
BOOL  RBInsertBand(PRB prb, UINT uBand, LPREBARBANDINFO lprbbi)
{
    PRBB prbb;
    REBARBANDINFO rbbi = {0};

    if (!prb || !RBValidateBandInfo(&lprbbi, &rbbi))
        return(FALSE);
    
    if (uBand == -1)
        uBand = prb->cBands;
    else if (uBand > prb->cBands)
        return(FALSE);

    if (!RBReallocBands(prb, prb->cBands + 1))
        return(FALSE);

    ++prb->cBands;
    MoveMemory(RBGETBAND(prb, uBand + 1), RBGETBAND(prb, uBand), (prb->cBands-1-uBand) * sizeof(prb->rbbList[0]));

    prbb = RBGETBAND(prb, uBand);

    // movememory does not zero init for us...
    ZeroMemory(prbb, SIZEOF(RBB));


    // Init text color
    if (prb->clrText == CLR_NONE)
    {
        // Default to system text color
        prbb->clrFore = CLR_DEFAULT;
    }
    else
    {
        // Default to rebar's custom text color
        prbb->clrFore = CLR_NONE;
    }


    // Init background color
    if (prb->clrBk == CLR_NONE)
    {
        // Default to system background color
        prbb->clrBack = CLR_DEFAULT;
    }
    else
    {
        // Default to rebar's custom background color
        prbb->clrBack = CLR_NONE;
    }

    
    prbb->iImage = -1;
    prbb->cyMaxChild = MAXINT;
    prbb->wChevState = DCHF_INACTIVE;
    
    ASSERT(prbb->fStyle == 0);
    ASSERT(prbb->lpText == NULL);
    ASSERT(prbb->cxText == 0);
    ASSERT(prbb->hwndChild == NULL);
    ASSERT(prbb->cxMinChild == 0);
    ASSERT(prbb->cyMinChild == 0);
    ASSERT(prbb->hbmBack == 0);
    ASSERT(prbb->x == 0);
    ASSERT(prbb->y == 0);
    ASSERT(prbb->cx == 0);
    ASSERT(prbb->cy == 0);
    
    if (!RBSetBandInfo(prb, uBand, lprbbi, FALSE))
    {
        RBDeleteBand(prb, uBand);
        return(FALSE);
    }
    
    if (!(prbb->fStyle & RBBS_HIDDEN)) {
        PRBB prbbFirst = RBEnumBand(prb, 0, RBBS_HIDDEN);
        
        if (!prbb->cxMin)
            RBBCalcMinWidth(prb, prbb);

        if (prbbFirst != prbb) {
            int cxMin = prbbFirst->cxMin;
            RBBCalcMinWidth(prb, prbbFirst);
        }
        RBResize(prb, FALSE);
    }

    RBSizeBandToRowHeight(prb, uBand, (UINT)-1);

    if (RBCountBands(prb, RBBS_HIDDEN) == 1) {
        // typcially, when you insert a band, we put it in a row with another band.
        // thus the total bounding rect doesn't change.  however, on the addition of the first band,
        // the bound rect does change, so we need to autosize as necessary.
        RBAutoSize(prb);
    }

    return(TRUE);
}

#pragma code_seg(CODESEG_INIT)

LRESULT CALLBACK ReBarWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL InitReBarClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    if (!GetClassInfo(hInstance, c_szReBarClass, &wc))
    {
#ifndef WIN32
        extern LRESULT CALLBACK _ReBarWndProc(HWND, UINT, WPARAM, LPARAM);
        wc.lpfnWndProc  = _ReBarWndProc;
#else
        wc.lpfnWndProc  = (WNDPROC) ReBarWndProc;
#endif

        wc.lpszClassName= c_szReBarClass;
        wc.style        = CS_GLOBALCLASS | CS_DBLCLKS;
        wc.cbClsExtra   = 0;
        wc.cbWndExtra   = sizeof(PRB);
        wc.hInstance    = hInstance;   // use DLL instance if in DLL
        wc.hIcon        = NULL;
        wc.hCursor      = NULL;
        wc.hbrBackground= (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszMenuName = NULL;

        if (!RegisterClass(&wc))
            return(FALSE);
    }

    return(TRUE);
}
#pragma code_seg()

// get the first band in the same row as rbbRow
// n.b. we may return an RBBS_HIDDEN band!
PRBB RBGetFirstInRow(PRB prb, PRBB prbbRow)
{
    // n.b. we don't pay attention to hidden here, that's up to caller.
    // in fact we *can't*, since there might be no non-hidden guys left
    // (e.g. when RBDestroy is deleting all the bands), in which case
    // we'd loop forever.
    while (prbbRow > RBGETBAND(prb, 0) && !RBISBANDSTARTOFROW(prbbRow)) {
        RBCheckRangePtr(prb, prbbRow);
        prbbRow--;
    }
    
    return prbbRow;
}

// get the last band in the same row as rbbRow.
// fStopAtFixed says whether to continue over fixed bands or 
// stop at them
// n.b. we may return an RBBS_HIDDEN band!
PRBB RBGetLastInRow(PRB prb, PRBB prbbRow, BOOL fStopAtFixed)
{
    do {
        prbbRow++;
    } while (prbbRow <= RB_GETLASTBAND(prb) && !RBISBANDSTARTOFROW(prbbRow) && 
        (!fStopAtFixed || (prbbRow->fStyle & (RBBS_FIXEDSIZE|RBBS_HIDDEN)) == RBBS_FIXEDSIZE));

    // loop steps to the start of the NEXT line
    prbbRow--;
    
    return prbbRow;
}

#ifdef DEBUG
BOOL RBCheckRangePtr(PRB prb, PRBB prbb)
{
    if (prbb < RBGETBAND(prb, 0)) {
        ASSERT(0);
        return FALSE;
    }

    if (RB_GETLASTBAND(prb) + 1 < prbb) {
        // +1 to allow for "p = first; p < last+1; p++" kinds of loops
        ASSERT(0);
        return FALSE;
    }

    return TRUE;
}

BOOL RBCheckRangeInd(PRB prb, INT_PTR i)
{
    if (i < 0) {
        ASSERT(0);
        return FALSE;
    }

    if ((int) prb->cBands < i) {
        // +1 to allow for "p = first; p < last+1; p++" kinds of loops
        ASSERT(0);
        return FALSE;
    }

    return TRUE;
}
#endif

//***   RBGetPrev, RBGetNext -- get prev (next) band, skipping guys
// of style uStyleSkip (e.g. RBBS_HIDDEN)
PRBB RBGetPrev(PRB prb, PRBB prbb, UINT uStyleSkip)
{
    while (--prbb >= RBGETBAND(prb, 0)) {
        if (prbb->fStyle & uStyleSkip)
            continue;
        break;
    }

    return prbb;
}

// when called with prbb=lastband, returns prbb++
// which is one past the end...
PRBB RBGetNext(PRB prb, PRBB prbb, UINT uStyleSkip)
{
    while (++prbb <= RB_GETLASTBAND(prb)) {
        if (prbb->fStyle & uStyleSkip)
            continue;
        break;
    }

    return prbb;
}

// this returns NULL when it hits the end
PRBB RBBNextVisible(PRB prb, PRBB prbb)
{
    prbb = RBGetNextVisible(prb, prbb);
    if (prbb > RB_GETLASTBAND(prb))
        return NULL;
    
    return prbb;
}

// this returns null when it hits the end
PRBB RBBPrevVisible(PRB prb, PRBB prbb)
{
    prbb = RBGetPrevVisible(prb, prbb);
    if (prbb < prb->rbbList)
        return NULL;
    
    return prbb;
}

//***   RBCountBands -- get count of bands, skipping guys
// of style uStyleSkip (e.g. RBBS_HIDDEN)
int RBCountBands(PRB prb, UINT uStyleSkip)
{
    int i;
    PRBB prbb;

    if (prb->cBands == 0)
        return 0;

    i = 0;
    for (prbb = RBGETBAND(prb, 0); prbb <= RB_GETLASTBAND(prb); prbb++) {
        if (prbb->fStyle & uStyleSkip)
            continue;
        i++;
    }

    return i;
}

//***   RBEnumBand -- get Nth band, skipping guys
// of style uStyleSkip (e.g. RBBS_HIDDEN)
// 'skipping' means don't include in count
PRBB RBEnumBand(PRB prb, int i, UINT uStyleSkip)
{
    PRBB prbb;

    for (prbb = RBGETBAND(prb, 0); prbb <= RB_GETLASTBAND(prb); prbb++) {
        if (prbb->fStyle & uStyleSkip)
            continue;
        if (i-- == 0)
            break;
    }

    // if we found it, this is the band;
    // if we ran out of bands, this is 1 past the end
    return prbb;
}

// returns the minimum x position prbb can be
int RBMinX(PRB prb, PRBB prbb)
{
    int xLimit = 0;

    ASSERT(!(prbb->fStyle & RBBS_HIDDEN));  // o.w. might loop forever
    while (!RBISBANDSTARTOFROW(prbb))
    {
        prbb--;
        if (!(prbb->fStyle & RBBS_HIDDEN))
            xLimit += _RBBandWidth(prb, prbb->cxMin);
    }
    
    return xLimit;
}

int RBMaxX(PRB prb, PRBB prbb)
{
    PRBB    prbbLast = prb->rbbList + prb->cBands;
    int xLimit = 0;
    PRBB prbbWalk;
    for (prbbWalk = prbb; prbbWalk < prbbLast; prbbWalk++) {
        if (prbbWalk->fStyle & RBBS_HIDDEN)
            continue;
        if (RBISBANDSTARTOFROW(prbbWalk))
            break;

        if (prbbWalk != prbb)
            xLimit += _RBBandWidth(prb, prbbWalk->cxMin);
        else 
            xLimit += prbbWalk->cxMin;
    }
    prbbWalk = RBGetPrevVisible(prb, prbbWalk);   // prbbWalk--;
    xLimit = prbbWalk->x + prbbWalk->cx - xLimit;
    return xLimit;
}

PRBB RBGetPrevVisible(PRB prb, PRBB prbb)
{
    return RBGetPrev(prb, prbb, RBBS_HIDDEN);
}

PRBB RBGetNextVisible(PRB prb, PRBB prbb)
{
    return RBGetNext(prb, prbb, RBBS_HIDDEN);
}

BOOL RBMinimizeBand(PRB prb, UINT uBand, BOOL fAnim)
{
    PRBB prbb;

    if (!RB_ISVALIDINDEX(prb, uBand))
        return FALSE;
    prbb=RBGETBAND(prb,uBand);
    if (prbb->fStyle & RBBS_FIXEDSIZE)
        return FALSE;
    
    ASSERT(!(prbb->fStyle & RBBS_HIDDEN));
    if (RBISBANDSTARTOFROW(prbb)) {
        // if it's the start of a row, the way to minimize it is to maximize the next guy
        // if it's on the same row
        prbb = RBGetNextVisible(prb, prbb);
        if (prbb > RB_GETLASTBAND(prb) || RBISBANDSTARTOFROW(prbb)) 
            return FALSE;
        
        return RBMaximizeBand(prb, RBBANDTOINDEX(prb,prbb), FALSE, fAnim);
    }
    if (fAnim)
        return RBSetBandPosAnim(prb, prbb, prbb->x + (prbb->cx - prbb->cxMin));
    else
        return RBSetBandPos(prb, prbb, prbb->x + (prbb->cx - prbb->cxMin));

}


// fIdeal - FALSE == full maximization...  
//          TRUE == go to cxIdeal
// fAnim - TRUE means we were called due to UI action (via RBToggleBand), so animate

BOOL RBMaximizeBand(PRB prb, UINT uBand, BOOL fIdeal, BOOL fAnim)
{
    int x, dx;
    BOOL fChanged = FALSE;
    PRBB prbbMaximize;

    if (!RB_ISVALIDINDEX(prb, uBand))
        return FALSE;

    prbbMaximize = RBGETBAND(prb,uBand);

    if (prbbMaximize->fStyle & RBBS_FIXEDSIZE)
        return FALSE;

    dx = prbbMaximize->cxIdeal + RBBHEADERWIDTH(prbbMaximize) - prbbMaximize->cx;
    
    if (fIdeal && dx > 0) 
    {
        PRBB prbb;
        
        // first move the next guy over if possible.

        prbb = RBBNextVisible(prb, prbbMaximize);
        if (prbb && (!RBISBANDSTARTOFROW(prbb)))
        {
            int dxRbb;

            x = RBMaxX(prb, prbb);
            // dxRbb is the maximum that prbb can move
            dxRbb = x - prbb->x;

            if (dxRbb > dx) {
                // if that's more than enough space, then limit dx
                dxRbb = dx;
            }

            x = prbb->x + dxRbb;
            fChanged |= (fAnim)?RBSetBandPosAnim(prb, prbb, x):RBSetBandPos(prb,prbb,x);
            dx -= dxRbb;
        }

        if (dx) {
            int dxRbb;

            // the one on the right didn't move enough.
            // now move us back
            x = RBMinX(prb, prbbMaximize);
            dxRbb = prbbMaximize->x - x;

            if (dxRbb > dx) {
                x = prbbMaximize->x - dx;
            }
            fChanged |= (fAnim)?RBSetBandPosAnim(prb, prbbMaximize, x):RBSetBandPos(prb, prbbMaximize, x);
        }
        
    } else {    
        x = RBMinX(prb, prbbMaximize);
        fChanged |= (fAnim)?RBSetBandPosAnim(prb, prbbMaximize, x):RBSetBandPos(prb, prbbMaximize, x);
        prbbMaximize = RBBNextVisible(prb, prbbMaximize);
        if (prbbMaximize && !RBISBANDSTARTOFROW(prbbMaximize)) {
            x = RBMaxX(prb, prbbMaximize);
            fChanged |= (fAnim)?RBSetBandPosAnim(prb, prbbMaximize, x):RBSetBandPos(prb, prbbMaximize, x);
        }
    }
            
    return fChanged;
}


// ----------------------------------------------------------------------------
//
// RBToggleBand
//
// switches a band between it's maximized and minimized state, based on where
// the user clicked
//
// ----------------------------------------------------------------------------
void  RBToggleBand(PRB prb, BOOL fAnim)
{
    BOOL fDidSomething = FALSE;

    // try to maximize this band.  if failed (meaning already maximize)
    // then minimize

    if (CCSendNotify(&prb->ci, RBN_MINMAX, NULL))
        return;            

    fDidSomething = RBMaximizeBand(prb, prb->iCapture, TRUE,fAnim);
    if (!fDidSomething)
        fDidSomething = RBMinimizeBand(prb, prb->iCapture,fAnim);

    if (fDidSomething)
        CCPlaySound(TEXT("ShowBand"));
}


// ----------------------------------------------------------------------------
//
// RBSetCursor
//
// sets the cursor to either the move cursor or the arrow cursor, depending
// on whether or not the cursor is on a band's caption
//
// ----------------------------------------------------------------------------
void  RBSetCursor(PRB prb, int x, int y, BOOL fMouseDown)
{

    int             iBand;
    RBHITTESTINFO   rbht;
    rbht.pt.x = x;
    rbht.pt.y = y;
    iBand = RBHitTest(prb, &rbht);
    if (rbht.flags == RBHT_GRABBER)
    {
        if (fMouseDown)
            SetCursor(LoadCursor(HINST_THISDLL, (prb->ci.style & CCS_VERT) ? MAKEINTRESOURCE(IDC_DIVOPENV) : MAKEINTRESOURCE(IDC_DIVOPEN) ));
        else
            SetCursor(LoadCursor(NULL, (prb->ci.style & CCS_VERT) ? IDC_SIZENS : IDC_SIZEWE));
        return;
    }

    if ((fMouseDown) && ((rbht.flags == RBHT_GRABBER) || (rbht.flags == RBHT_CAPTION) && RBShouldDrawGripper(prb, RBGETBAND(prb, iBand))))
    {
        // No longer IE3 compatible, per RichSt
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        return;
    }

    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

// adjust's a band's (prbb) starting location to the given location
BOOL RBSetBandPos(PRB prb, PRBB prbb, int xLeft)
{
    RECT    rc;
    PRBB    prbbPrev;
    int     xRight;
    BOOL    fBandBorders = (prb->ci.style & RBS_BANDBORDERS);
    BOOL    fRight;

    ASSERT(!(prbb->fStyle & RBBS_HIDDEN));
    ASSERT((xLeft >= 0)); // We've got problems if someone is trying to set us negative

    if (prbb->x == xLeft)
        return(FALSE);

    prbbPrev = RBGetPrevVisible(prb, prbb);

    // band has moved within valid range -- adjust band sizes and redraw
    // window
    fRight = (prbb->x < xLeft);

    SetRect(&rc, prbb->x, prbb->y, prbb->x + prbb->cxMin, prbb->y + prbb->cy);
    xRight = prbb->x + prbb->cx;
    prbb->x = xLeft;
    prbb->cx = xRight - xLeft;
    prbb->cxRequest = prbb->cx;

    if (fRight)
    {
        //moving right
        prbbPrev->cx = prbb->x - prbbPrev->x;
        if (fBandBorders)
        {
            prbbPrev->cx -= g_cxEdge;
            rc.left -= g_cxEdge;
        }
        prbbPrev->cxRequest = prbbPrev->cx;

        //check for compacting of following bands

        while (prbb->cx < prbb->cxMin)
        {
            prbb->cx = prbb->cxMin;
            prbb->cxRequest = prbb->cx;
            xLeft += RBBANDWIDTH(prb, prbb);
            prbb = RBGetNextVisible(prb, prbb);   // prbb++;
            xRight = prbb->x + prbb->cx;
            prbb->x = xLeft;
            prbb->cx = xRight - xLeft;
            prbb->cxRequest = prbb->cx;
        }
        rc.right = xLeft + prbb->cxMin;
    }
    else
    {
        //moving left

        //check for compacting of preceding bands
CompactPrevious:
        if (fBandBorders)
            xLeft -= g_cxEdge;
        prbbPrev->cx = xLeft - prbbPrev->x;
        prbbPrev->cxRequest = prbbPrev->cx;
        if (prbbPrev->cx < prbbPrev->cxMin)
        {
            prbbPrev->x = xLeft - prbbPrev->cxMin;
            prbbPrev->cx = prbbPrev->cxMin;
            prbbPrev->cxRequest = prbbPrev->cx;
            xLeft = prbbPrev->x;
            prbbPrev = RBGetPrevVisible(prb, prbbPrev);   // prbbPrev--
            goto CompactPrevious;
        }
        rc.left = xLeft;
    }

    if (fBandBorders)
        rc.bottom += g_cyEdge / 2;

    RBResizeChildren(prb);
    if (RBInvalidateRect(prb, &rc))
        UpdateWindow(prb->ci.hwnd);
    return(TRUE);

}

BOOL RBSetBandPosAnim(PRB prb, PRBB prbb, int xLeft)
{
    int ctr=0,dx, xCur = prbb->x;
    DWORD dwStartTime;

    if (xCur == xLeft)
        return FALSE;

    dwStartTime=GetTickCount();
    dx = (xLeft - xCur)/RB_ANIMSTEPS;

    if (dx != 0)
    {
        if (xCur < xLeft) {
            // move right
            for (; xCur < (xLeft-dx); ctr++,xCur += dx) {
                RBSetBandPos(prb, prbb, xCur);
                // If something caused us to take more than 10 times the time we
                // should be, break out, and let the final RBSetBandPos finish
                if (GetTickCount() > (dwStartTime + 10*RB_ANIMSTEPS*RB_ANIMSTEPTIME))
                    break;

                Sleep(RB_ANIMSTEPTIME);
                // Start slowing us down 80% of the way through
                // Cut speed by 2/3 each time, but never move less than 4 pixels
                if ((ctr >= 4*RB_ANIMSTEPS/5) && (dx >= 4))
                    dx = 2*dx/3; 
            }
        }
        else {
            // move left
            for (; xCur > (xLeft-dx); ctr++, xCur += dx) {
                RBSetBandPos(prb, prbb, xCur);
                if (GetTickCount() > (dwStartTime + 10*RB_ANIMSTEPS*RB_ANIMSTEPTIME))
                    break;
                Sleep(RB_ANIMSTEPTIME);
                if ((ctr >= 4*RB_ANIMSTEPS/5) && (dx <= -4))
                    dx = 2*dx/3;
            }
        }
    }
    RBSetBandPos(prb, prbb, xLeft);
    return TRUE;
}

// ----------------------------------------------------------------------------
//
// RBDragSize
//
// adjust the captured band's starting location to the given location and
// redraw
//
// ----------------------------------------------------------------------------
BOOL RBDragSize(PRB prb, int xLeft)
{
    return RBSetBandPos(prb, RBGETBAND(prb, prb->iCapture), xLeft);
}

void RBOnBeginDrag(PRB prb, UINT uBand)
{
    prb->iCapture = (int)uBand;
    prb->ptLastDragPos.x = -1;
    prb->ptLastDragPos.y = -1;
    if (prb->iCapture == -1) {
       // aborting drag
        prb->fParentDrag = FALSE;
        prb->fFullOnDrag = FALSE;

        // we could have unwrapped rows, in which case, we need to grow bands (but not wrap)
        // to fill the empty space.
        if (prb->ci.style & RBS_AUTOSIZE) {
            RBSizeBandsToRect(prb, NULL);
            RBSizeBandsToRowHeight(prb);
        }
        
    } else {
        prb->fParentDrag = TRUE;
        prb->fFullOnDrag = TRUE;
    }
}

int minmax(int x, int min, int max)
{
    x = max(x, min);
    x = min(x, max);
    return x;
}

// pass the break bit along
void RBPassBreak(PRB prb, PRBB prbbSrc, PRBB prbbDest)
{
    if (prbbSrc->fStyle & RBBS_BREAK) {
        prbbSrc->fStyle &= ~RBBS_BREAK;
        if (prbbDest)
            prbbDest->fStyle |= RBBS_BREAK;
    }
}

void RBGetClientRect(PRB prb, LPRECT prc)
{
    GetClientRect(prb->ci.hwnd, prc);
    if (prb->ci.style & CCS_VERT)
        FlipRect(prc);
}

//tells if prbb is the first band and the next band is fixed.
// if this is true then we need to do a recalc if we move prbb
BOOL RBRecalcIfMove(PRB prb, PRBB prbb)
{
    if (RBEnumBand(prb, 0, RBBS_HIDDEN) == prbb) {
        PRBB prbbNext = RBBNextVisible(prb, prbb);
        if (prbbNext && prbbNext->fStyle & RBBS_FIXEDSIZE)
            return TRUE;
    }
    return FALSE;
}

// find out if the prbb at it's min height could fit within the current window
// if all the others shrunk as much as they could
BOOL RBRoomForBandVert(PRB prb, PRBB prbbSkip)
{
    int yExtra = 0;
    int cBands = prb->cBands;
    int iNewRowHeight = prbbSkip->cyMinChild;
    PRBB prbb = RBGETBAND(prb, 0);
    
    if (prb->ci.style & RBS_BANDBORDERS)
        iNewRowHeight += g_cyEdge;
    
    while (prbb) {
        if (RBISBANDVISIBLE(prbb)) {
            if (RBISBANDSTARTOFROW(prbb)) {
                yExtra += RBGetRowHeightExtra(prb, &prbb, prbbSkip);
                if (yExtra >= iNewRowHeight)
                    return TRUE;
                continue;
            }
        }
        prbb = RBBNextVisible(prb, prbb);
    }
    
    return FALSE;
}

// we should make a new row if prbb isn't the start of the row already
// and we're off the end of the control
//
// poweruser hack of holding the control down will make a new row if you hit the border between lines

BOOL RBMakeNewRow(PRB prb, PRBB prbb, int y)
{
    BOOL fRet = FALSE;
    RECT rc;

    // if we're off the top of the control, move this band to the end (or beginning)
    RBGetClientRect(prb, &rc);
    InflateRect(&rc, 0, -g_cyEdge);

    if (!(prb->ci.style & RBS_FIXEDORDER)) {

        int iOutsideLimit = g_cyEdge * 4; // how far do you have to move outside the bounds of the window to force a new row
        
        if (RBRoomForBandVert(prb, prbb)) {
            iOutsideLimit = -g_cyEdge;
        }
        
        if (y < rc.top - iOutsideLimit) { // top of control
            
            PRBB prbbNext = RBEnumBand(prb, 0, RBBS_HIDDEN);
            if (prbbNext == prbb) 
                prbbNext = RBBNextVisible(prb, prbb);
            fRet |= RBMoveBand(prb, RBBANDTOINDEX(prb, prbb), 0);
            ASSERT(prbbNext <= RB_GETLASTBAND(prb));
            if (prbbNext && !(prbbNext->fStyle & RBBS_BREAK)) {
                prbbNext->fStyle |= RBBS_BREAK;
                fRet = TRUE;
            }
        } else if (y >= rc.bottom + iOutsideLimit) { // move to the end
            if (!(prbb->fStyle & RBBS_BREAK)) {
                prbb->fStyle |= RBBS_BREAK;
                fRet = TRUE;
            }
            fRet |= RBMoveBand(prb, RBBANDTOINDEX(prb, prbb), prb->cBands-1);
        } else {

            // create a new row in the middle
            if (!RBISBANDSTARTOFROW(prbb) && GetAsyncKeyState(VK_CONTROL) < 0) {
                // make sure they're on different rows and on the border
                if (y > prbb->y + prbb->cy && (y < prbb->y + prbb->cy + g_cyEdge)) {

                    PRBB prbbLast = RBGetLastInRow(prb, prbb, FALSE);  // move it right before the first in this row
                    prbb->fStyle |= RBBS_BREAK;
                    RBMoveBand(prb, RBBANDTOINDEX(prb, prbb), RBBANDTOINDEX(prb, prbbLast));
                    fRet = TRUE;
                }
            }
        }

    } else {
        // fixed guys can't move, they can only make a new row
        if (!RBISBANDSTARTOFROW(prbb)) {
            if (y > prbb->y + prbb->cy) {
                prbb->fStyle |= RBBS_BREAK;
                fRet = TRUE;
            }
        }
    }
    
    if (fRet)
        RBResize(prb, FALSE);
    return fRet;
}


// ----------------------------------------------------------------------------
//
// RBDragBand
//
// resizes the currently tracked band based on the user's mouse movement as
// indicated in the given point (x, y)
//
// ----------------------------------------------------------------------------
void RBDragBand(PRB prb, int x, int y)
{
    PRBB prbb = RBGETBAND(prb, prb->iCapture);
    int iHit;
    // Do nothing if the mouse didn't actually move
    // otherwise, multiple WM_MOUSEMOVE messages will be generated by resizing windows
    if (x==prb->ptLastDragPos.x && y==prb->ptLastDragPos.y)
        return;
    else
    {
        prb->ptLastDragPos.x = x;
        prb->ptLastDragPos.y = y;
    }

    if (prb->ci.style & CCS_VERT)
        SWAP(x,y, int);

    if (!prb->fFullOnDrag)
    {
        // don't begin dragging until mouse is moved outside of an edge-thick
        // tolerance border
        if ((y < (prb->ptCapture.y - g_cyEdge)) || (y > (prb->ptCapture.y + g_cyEdge)) ||
            (x < (prb->ptCapture.x - g_cxEdge)) || (x > (prb->ptCapture.x + g_cxEdge))) {

            // did parent abort?
            if (RBSendNotify(prb, prb->iCapture, RBN_BEGINDRAG))
                return;

            if (!RB_ISVALIDBAND(prb, prbb)) {
                // somebody responded to RBN_BEGINDRAG by nuking bands; bail
                return;
            }
            
            prb->fFullOnDrag = TRUE;
        } else
            return;
    }
    
    // bail for right now on fRecalcIfMoved (ie3 did the same thing). nice feature for later
    if (!RBCanBandMove(prb, prbb))
        return;
    
   /* what type of drag operation depends on what we drag hit on.

        if we hit on the band before us, or ourself
          and it's the same row
          and we're not the first band of the row
             then we just to a size move
             
        otherwise if we hit on a band then we do a move 
        
        if we hit outside of any band, we grow to meet the cursor
        
        in all of the above, a band that's hit must be NOT fixed and not hidden
    */
    iHit = _RBHitTest(prb, NULL, x, y);
    
    if (iHit != -1) {
        BOOL fResize = FALSE;
        PRBB prbbPrev = RBBPrevVisible(prb, prbb);
        PRBB prbbHit = RBGETBAND(prb, iHit);
        prbbHit = RBGetPrev(prb, ++prbbHit, RBBS_FIXEDSIZE); // skip over fixed guys

        ASSERT(prbbHit >= prb->rbbList);
        // this should never happen.
        if (prbbHit < prb->rbbList) 
            return;
        
        iHit = RBBANDTOINDEX(prb, prbbHit);
        
        // if we're on the same row ...  and it's us or the previous one
        if (prbbHit->y == prbb->y && (prbbHit == prbb || prbbHit == prbbPrev)) {

            if (x < RB_GRABWIDTH && !(prb->ci.style & RBS_FIXEDORDER)) {
                // special case dragging to the far left. there's no other way to move to first in row
                RBPassBreak(prb, prbbHit, prbb);
                if (RBMoveBand(prb, prb->iCapture, iHit))                
                    fResize = TRUE;

            } else if (!RBISBANDSTARTOFROW(prbb)) {
                // and we're not the first band of the row
                // then just size it
                int xLeft = prb->xStart + (x - prb->ptCapture.x);
                xLeft = minmax(xLeft, RBMinX(prb, prbb), RBMaxX(prb, prbb));
                RBDragSize(prb, xLeft);
            }

        } else if (RBMakeNewRow(prb, prbb, y)) {
        } else {            // otherwise do a move if we're not in a fixed order
            if (!(prb->ci.style & RBS_FIXEDORDER)) {
                if (iHit < RBBANDTOINDEX(prb, prbb)) 
                    iHit++; // +1 because if you hit a band, you're moving to the right of him

                // if one with a break is moving, the next one inherits the break
                RBPassBreak(prb, prbb, RBBNextVisible(prb, prbb));
                RBMoveBand(prb, prb->iCapture, iHit);
            } else {
                if (iHit < RBBANDTOINDEX(prb, prbb))
                    RBPassBreak(prb, prbb, RBBNextVisible(prb, prbb));
            }
            fResize = TRUE;
        }
        if (fResize)
            RBResize(prb, FALSE);        
        
    } else 
        RBMakeNewRow(prb, prbb, y);    
}

HPALETTE RBSetPalette(PRB prb, HPALETTE hpal)
{
    HPALETTE hpalOld = prb->hpal;

    if (hpal != hpalOld) {
        if (!prb->fUserPalette) {
            if (prb->hpal) {
                DeleteObject(prb->hpal);
                prb->hpal = NULL;
            }
        }

        if (hpal) {
            prb->fUserPalette = TRUE;
            prb->hpal = hpal;
        }

        RBInvalidateRect(prb, NULL);
    }
    return hpalOld;
}

// ----------------------------------------------------------------------------
//
// RBDestroy
//
// frees all memory allocated by rebar, including rebar structure
//
// ----------------------------------------------------------------------------
BOOL  RBDestroy(PRB prb)
{
    UINT c = prb->cBands;

    RBSetRedraw(prb, FALSE);
    RBSetRecalc(prb, FALSE);
    
    while (c--)
        RBDeleteBand(prb, c);

    // so that we don't keep trying to autosize
    prb->ci.style &= ~RBS_AUTOSIZE;
    
    ASSERT(!prb->rbbList);

    RBSetPalette(prb, NULL);
    
    if (prb->hFont && prb->fFontCreated) {
        DeleteObject(prb->hFont);
    }

    if ((prb->ci.style & RBS_TOOLTIPS) && IsWindow(prb->hwndToolTips))
    {
        DestroyWindow (prb->hwndToolTips);
        prb->hwndToolTips = NULL;
    }


    // don't destroy the himl 'cause it's given to us by app

    SetWindowPtr(prb->ci.hwnd, 0, 0);


    if (prb->hDragProxy)
        DestroyDragProxy(prb->hDragProxy);

    LocalFree((HLOCAL) prb);
    return(TRUE);
}

// ----------------------------------------------------------------------------
//
// RBInitPaletteHack
//
// this is a hack to use the halftone palette until we have a way of asking
// the client what palette they are using
//
// ----------------------------------------------------------------------------
void  RBInitPaletteHack(PRB prb)
{
    if (!prb->fUserPalette) {
        HDC hdc = CreateCompatibleDC(NULL);
        if (hdc) {
            if (GetDeviceCaps(hdc, BITSPIXEL) <= 8) {
                if (prb->hpal)
                    DeleteObject(prb->hpal);
                prb->hpal = CreateHalftonePalette(hdc);  // this is a hack
            }
            DeleteDC(hdc);
        }
    }
}

LRESULT RBIDToIndex(PRB prb, UINT id)
{
    UINT i;
    REBARBANDINFO   rbbi;

    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask = RBBIM_ID;

    for (i = 0; i < prb->cBands; i++) {

        if (RBGetBandInfo(prb, i, &rbbi)) {

            if (rbbi.wID == (WORD)id)
                return i;
        }
    }

    return -1;
}

LRESULT RBGetRowHeight(PRB prb, UINT uRow)
{
    if (uRow < prb->cBands)
    {
        // move back to start of line
        PRBB prbbFirst = RBGetFirstInRow(prb, RBGETBAND(prb, uRow));
        PRBB prbbLast = RBGetLastInRow(prb, RBGETBAND(prb, uRow), FALSE);

        return RBGetLineHeight(prb, RBBANDTOINDEX(prb, prbbFirst), RBBANDTOINDEX(prb, prbbLast));
    }

    return (LRESULT)-1;
}

// fOneStep == whether to allow  only one cyIntegral or as many as will fit to 
//     fill dy
int RBGrowBand(PRB prb, PRBB prbb, int dy, BOOL fOneStep)
{
    int iDirection = dy / ABS(dy);
    int dyBand = 0; // how much the band changes
    int cyNewHeight;


    if (prbb->cyIntegral &&
        prbb->cyIntegral <= (UINT)ABS(dy)) {
        
        // get the proposed new size
        if (fOneStep)
            dyBand = (prbb->cyIntegral * iDirection);
        else {
            int iNumOfIntegrals;
            
            // don't let it grow more than the max allowed
            if (dy >= 0) {
                if ((int)(prbb->cyMaxChild - prbb->cyChild) < dy) {
                    dy = (int)(prbb->cyMaxChild - prbb->cyChild);
                }
            } else {
                if ((int)(prbb->cyMinChild - prbb->cyChild) > dy) {
                    dy = (int)(prbb->cyMinChild - prbb->cyChild);
                }
            }
            
            iNumOfIntegrals = (dy / (int) prbb->cyIntegral);
            dyBand = (prbb->cyIntegral * iNumOfIntegrals);
                
        }
        
        cyNewHeight = ((int)prbb->cyChild) + dyBand;

        // make sure the new size is legal
        
        if ((int)prbb->cyMinChild <= cyNewHeight && ((UINT)cyNewHeight) <= prbb->cyMaxChild) {
            prbb->cyChild = cyNewHeight;
            RBResize(prb, TRUE);
        } else
            dyBand = 0;
    }
    return dyBand;
}

// returns the delta in size that the rebar is from prc.
// taking into account vertical mode
int RBSizeDifference(PRB prb, LPRECT prc)
{
    int d;

    d = (RB_ISVERT(prb) ? RECTWIDTH(*prc) : RECTHEIGHT(*prc))
        - prb->cy;
    
    return d;
}

// returns how much this row could shrink
int RBGetRowHeightExtra(PRB prb, PRBB *pprbb, PRBB prbbSkip)
{
    // this is the largest minimum child size for the row. 
    // even if something is not at it's min size, if it's smaller than this
    // then it doesn't matter because someone else on that row can't be sized
    int yLimit = 0;
    int yExtra = 0;
    PRBB prbb = *pprbb;
            
    while (prbb) {
        
        if (prbb != prbbSkip) {
            int yMin;
            int yExtraBand = 0;

            // the min height is the cyChild if it's not variable height
            yMin = prbb->cyChild;
            if (prbb->fStyle & RBBS_VARIABLEHEIGHT)
            {
                // if it is variable height, and there's still room to shrink, then cyMinChild is
                // the minimum.  
                if (prbb->cyChild > prbb->cyMinChild + prbb->cyIntegral) {
                    yMin = prbb->cyMinChild;
                    yExtraBand = prbb->cyChild - prbb->cyMinChild;
                }
            }

            if (yMin == yLimit) {
                if (yExtraBand > yExtra)
                    yExtra = yExtraBand;
            } else if (yMin > yLimit) {
                yExtra = yExtraBand;
            }
        }
        
        prbb = RBBNextVisible(prb, prbb);
    }
    
    *pprbb = prbb;
    
    return yExtra;
}

// are allt he bands at the minimum size? 
BOOL RBBandsAtMinHeight(PRB prb)
{
    BOOL fRet = TRUE;
    int cBands = prb->cBands;
    
    PRBB prbb = RBGETBAND(prb, 0);
    while (prbb) {
        if (RBISBANDVISIBLE(prbb)) {
            if (RBISBANDSTARTOFROW(prbb)) {
                fRet = RBROWATMINHEIGHT(prb, &prbb);
                if (!fRet)
                    break;
                continue;
            }
        }
        prbb = RBBNextVisible(prb, prbb);
    }
    
    return fRet;
}

// this is like RBSizeBarToRect except that it resizes theactual bands if they
// are VARIABLEHEIGHT
BOOL RBSizeBandsToRect(PRB prb, LPRECT prc)
{
    int dy;
    int iDirection = 0;
    BOOL fChanged = FALSE;
    BOOL fChangedThisLoop;
    UINT cBands;
    RECT rc;
    BOOL fRedrawOld;
    
    if (prc)
        rc = *prc;
    else {
        GetClientRect(prb->ci.hwnd, &rc);
    }
    
    fRedrawOld = RBSetRedraw(prb, FALSE);

    
    // this is the amount we need to grow by

    do {
        BOOL fOneStep = TRUE;
        
        cBands = prb->cBands;
        fChangedThisLoop = FALSE;

        // if there's only one row, we don't need to iterate through all the rows slowly
        if (RBGetRowCount(prb) == 1)
            fOneStep = FALSE;
        
        dy = RBSizeDifference(prb, &rc);
        
        // ensure that we alway size in the same direction.
        // it's possible to get on the border and flip flop in an infinite
        // loop.  this happens when we size both horizontally and vertically down
        // beyond the minimum.  
        if (iDirection == 0)
            iDirection = dy;
        else if (dy * iDirection < 0)
            break;
        
        while (cBands-- && dy) {
            // when we're resizing the entire rebar,  we want to divvy up
            // the growth among all the bands (rather than give it all to
            // a single guy).  uResizeNext goes round-robin thru the bands.
            PRBB prbb = RBGETBAND(prb, prb->uResizeNext);

            if (prb->uResizeNext == 0) 
                prb->uResizeNext = prb->cBands -1;
            else
                prb->uResizeNext--;
            
            if (prbb->fStyle & RBBS_HIDDEN)
                continue;

            if (prbb->fStyle & RBBS_VARIABLEHEIGHT) {
                int d;
                // if it's a variable height kind of guy, grow/shrink it
                d = RBGrowBand(prb, prbb, dy, fOneStep);
                dy -= d;
                if (d) {
                    fChanged = TRUE;
                    fChangedThisLoop = TRUE;
                    break;
                }
            }
        }

        // if we're shrinking 
        // and we didn't get completely satisfied. we need to overshoot
        // so that no bands hang off the end and get cut off
        if (dy < 0 && !fChangedThisLoop && !RBBandsAtMinHeight(prb)) {
            if (rc.bottom > rc.top) {
                rc.bottom -= 1;
                fChangedThisLoop = TRUE;
            }
        }
        
    } while (fChangedThisLoop);

    RBSetRedraw(prb, fRedrawOld);
    
    return fChanged;
}

void RBSizeBandToRowHeight(PRB prb, int i, UINT uRowHeight)
{
    PRBB prbb = RBGETBAND(prb, i);
    
    if (prbb && prbb->fStyle & RBBS_VARIABLEHEIGHT) {
        if (uRowHeight == (UINT)-1)
            uRowHeight = (UINT) RBGetRowHeight(prb, i);

        if (uRowHeight > prbb->cyChild) {
            RBGrowBand(prb, prbb, (uRowHeight - prbb->cyChild),
                       FALSE);
        }
    }
}

// in the process of sizing, one band in a row of several bands might have
// grow pretty large.  we need to let the other bands have a chance to fill
// the extra space as well
void RBSizeBandsToRowHeight(PRB prb)
{
    UINT i;
    UINT iRowHeight = (UINT)-1;
    
    for (i = 0; i < prb->cBands; i++) {
        PRBB prbb = RBGETBAND(prb, i);

        if (prbb->fStyle & RBBS_HIDDEN)
            continue;
        
        if (RBISBANDSTARTOFROW(prbb))
            iRowHeight = (UINT) RBGetRowHeight(prb, i);

        RBSizeBandToRowHeight(prb, i, iRowHeight);
    }
}

// this will add/remove rebar band breaks to get to the requested size.
// it returns TRUE/FALSE whether something was done or not.
LRESULT RBSizeBarToRect(PRB prb, LPRECT prc)
{
    BOOL fChanged = FALSE;
    RECT rc;
    BOOL fRedrawOld = RBSetRedraw(prb, FALSE);

    if (!prc) {
        GetClientRect(prb->ci.hwnd, &rc);
        prc = &rc;
    }
    
    if (prb->cBands) {
        int c;
        UINT cBands = prb->cBands;
        BOOL fChangedThisLoop = TRUE;
        BOOL fGrowing = TRUE;
        
        // if we're shrinking the rebar, we first want to shrink the bands before we start 
        // removing breaks
        c = RBSizeDifference(prb, prc);
        if (c < 0) 
            fGrowing = FALSE;
        
        if (!fGrowing) {
            fChanged = RBSizeBandsToRect(prb, prc);
            
            if (!RBBandsAtMinHeight(prb)) {
                // if we're shrinking and all the bands are not down to
                // the minimum height, don't try doing any of the breaking stuff
                goto Bail;
            }
        } else if (RB_ISVERT(prb)) {

            // if we're in vertical mode, give preference to 
            // sizing bands before breaking
            fChanged = RBSizeBandsToRect(prb, prc);
        }

        while (fChangedThisLoop && prb->cBands) {

            int cyRowHalf  = (int) RBGetRowHeight(prb, prb->cBands-1) / 2 ;
            REBARBANDINFO   rbbi;
            PRBB prbb;

            fChangedThisLoop = FALSE;

            rbbi.cbSize = sizeof(REBARBANDINFO);
            rbbi.fMask = RBBIM_STYLE;

            c = RBSizeDifference(prb, prc);

            if (c < -cyRowHalf) {

                // we've shrunk the rebar, try to remove breaks
                while (--cBands)
                {
                    prbb = RBGETBAND(prb, cBands);
                    if (prbb->fStyle & RBBS_HIDDEN)
                        continue;

                    if (prbb->fStyle & RBBS_BREAK)
                    {
                        fChanged = TRUE;
                        fChangedThisLoop = TRUE;
                        rbbi.fStyle = prbb->fStyle & ~RBBS_BREAK;
                        RBSetBandInfo(prb, cBands, &rbbi, TRUE);
                        break;
                    }
                }
            } else if (c > cyRowHalf) {

                // we're enlarging the rebar
                while (--cBands)
                {
                    prbb = RBGETBAND(prb, cBands);
                    if (prbb->fStyle & RBBS_HIDDEN)
                        continue;

                    if (!(prbb->fStyle & (RBBS_BREAK | RBBS_FIXEDSIZE)))
                    {
                        // no break here, add it
                        fChanged = TRUE;
                        fChangedThisLoop = TRUE;
                        rbbi.fStyle = (prbb->fStyle | RBBS_BREAK);
                        RBSetBandInfo(prb, cBands, &rbbi, TRUE);
                        break;
                    }
                }
            }
        };

        // if we did as much breaking as we could
        // and we walked all the way down to the 0th band (we start at the Nth band)
        // then we try to grow the bands that are VARIABLEHEIGHT
        // for fGrowing, see comment at top of function
        // 
        // wedo the % because cBands == prb->cBands if we didn't go through
        // any of the breaking loops at all
        if (!(cBands % prb->cBands) && fGrowing) 
            fChanged |= RBSizeBandsToRect(prb, prc);

    }

Bail:
    RBSizeBandsToRowHeight(prb);
    RBSetRedraw(prb, fRedrawOld);
    
    return (LRESULT)fChanged;
}

void RBAutoSize(PRB prb)
{
    NMRBAUTOSIZE nm;
    
    // if this is an internal autosize call, but we're not in autosize mode
    // do nothing
    
    if (!(prb->ci.style & RBS_AUTOSIZE))
        return;
    
    
    GetClientRect(prb->ci.hwnd, &nm.rcTarget);

    nm.fChanged = (BOOL) RBSizeBarToRect(prb, &nm.rcTarget);

    GetClientRect(prb->ci.hwnd, &nm.rcActual);
    CCSendNotify(&prb->ci, RBN_AUTOSIZE, &nm.hdr);
}

LRESULT RBGetBandBorders(PRB prb, int wParam, LPRECT prc)
{
    BOOL fBandBorders = (prb->ci.style & RBS_BANDBORDERS);

    PRBB prbb = &prb->rbbList[wParam];
    prc->left = RBBHEADERWIDTH(prbb);
    
    if (fBandBorders) {
        prc->left += 2*g_cxEdge;
        prc->right = 0;
        prc->top = g_cyEdge/2;
        prc->bottom = g_cyEdge /2;
    }
    if (prb->ci.style & CCS_VERT)
        FlipRect(prc);
    return 0;
}

void RBOnStyleChanged(PRB prb, WPARAM wParam, LPSTYLESTRUCT lpss)
{
    if (wParam == GWL_STYLE)
    {
        DWORD dwChanged;
        
        prb->ci.style = lpss->styleNew;
        
        dwChanged = (lpss->styleOld ^ lpss->styleNew);
        // update to reflect style change
#ifndef WINNT
        TraceMsg(TF_REBAR, "rebar window style changed %x", prb->ci.style);
#endif
        if (dwChanged & CCS_VERT)
        {
            UINT i;
            for (i = 0; i < prb->cBands; i++) {
                if (RBGETBAND(prb, i)->fStyle & RBBS_HIDDEN)
                    continue;

                RBBCalcMinWidth(prb, RBGETBAND(prb, i));
            }
            RBResize(prb, TRUE);
            RBInvalidateRect(prb, NULL);
        }
        
        if (dwChanged & RBS_REGISTERDROP) {
            
            if (prb->ci.style & RBS_REGISTERDROP) {
                ASSERT(!prb->hDragProxy);
                prb->hDragProxy = CreateDragProxy(prb->ci.hwnd, RebarDragCallback, TRUE);
            } else {
                ASSERT(prb->hDragProxy);
                DestroyDragProxy(prb->hDragProxy);
            }
        }
    } else if (wParam == GWL_EXSTYLE) {
        //
        // If the RTL_MIRROR extended style bit had changed, let's
        // repaint the control window
        //
        if ((prb->ci.dwExStyle&RTL_MIRRORED_WINDOW) !=  (lpss->styleNew&RTL_MIRRORED_WINDOW)) {
            RBInvalidateRect(prb, NULL);
        }

        //
        // Save the new ex-style bits
        //
        prb->ci.dwExStyle = lpss->styleNew;
    }
}

void RBOnMouseMove(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, PRB prb)
{
    RelayToToolTips(prb->hwndToolTips, hwnd, uMsg, wParam, lParam);

    if (prb->iCapture != -1)
    {
        // captured band -- mouse is down
        if (hwnd != GetCapture() && !prb->fParentDrag)
        {
            RBSendNotify(prb, prb->iCapture, RBN_ENDDRAG);
            RBOnBeginDrag(prb, (UINT)-1);
        }
        else
            RBDragBand(prb, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    }
    else
    {
        // hottracking
        int iBand;
        PRBB prbb = NULL;
        PRBB prbbHotOld = prb->prbbHot;
        RBHITTESTINFO rbht;

        rbht.pt.x = GET_X_LPARAM(lParam);
        rbht.pt.y = GET_Y_LPARAM(lParam);

        iBand = RBHitTest(prb, &rbht);
        if (iBand != -1)
            prbb = RBGETBAND(prb, iBand);

        if (prbbHotOld && (prbbHotOld->wChevState & DCHF_PUSHED))
            return;

        if (prbb && (rbht.flags & RBHT_CHEVRON))
        {
            SetCapture(hwnd);
            RBUpdateChevronState(prb, prbb, DCHF_HOT);
            if (prbb == prbbHotOld)
                prbbHotOld = NULL;
        }

        if (prbbHotOld)
        {
            CCReleaseCapture(&prb->ci);
            RBUpdateChevronState(prb, prbbHotOld, DCHF_INACTIVE);
        }
    }
}

void RBOnPushChevron(HWND hwnd, PRB prb, PRBB prbb, LPARAM lParamNM)
{
    NMREBARCHEVRON nm;
    nm.uBand = RBBANDTOINDEX(prb, prbb);
    nm.wID = prbb->wID;
    nm.lParam = prbb->lParam;
    nm.lParamNM = lParamNM;
    CopyRect(&nm.rc, &prbb->rcChevron);
    if (RB_ISVERT(prb))
        FlipRect(&nm.rc);
    RBUpdateChevronState(prb, prbb, DCHF_PUSHED);
    CCReleaseCapture(&prb->ci);
    CCSendNotify(&prb->ci, RBN_CHEVRONPUSHED, &nm.hdr);
    RBUpdateChevronState(prb, prb->prbbHot, DCHF_INACTIVE);
}

LRESULT CALLBACK ReBarWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PRB     prb = (PRB) GetWindowPtr(hwnd, 0);
    int     iBand;

    // bail if no prb unless at creation time
    if (!prb && !(uMsg == WM_NCCREATE))
        goto CallDWP;

    switch (uMsg)
    {
    case WM_SETREDRAW:
        if (prb->ci.iVersion >= 5)
            RBSetRecalc(prb, BOOLFROMPTR(wParam));

        return RBSetRedraw(prb, BOOLFROMPTR(wParam));

    case WM_NCCREATE:
#define lpcs ((LPCREATESTRUCT) lParam)
        InitGlobalColors();

        if (!(prb = (PRB) LocalAlloc(LPTR, sizeof(RB))))
            return(0L);

        SetWindowPtr(hwnd, 0, prb);
 
        prb->iCapture = -1;
        prb->clrBk = CLR_NONE;
        prb->clrText = CLR_NONE;

        // Init the dwSize because we block-copy it back to the app
        prb->clrsc.dwSize = sizeof(COLORSCHEME);
        prb->clrsc.clrBtnHighlight = prb->clrsc.clrBtnShadow = CLR_DEFAULT;

        prb->fRedraw = TRUE;
        prb->fRecalc = TRUE;

        // note, zero init memory from above
        CIInitialize(&prb->ci, hwnd, lpcs);

        if (!(prb->ci.style & (CCS_TOP | CCS_NOMOVEY | CCS_BOTTOM)))
        {
            prb->ci.style |= CCS_TOP;
            SetWindowLong(hwnd, GWL_STYLE, prb->ci.style);
        }

        RBSetFont(prb, 0);

        if (lpcs->lpCreateParams)
            RBSetBarInfo(prb, (LPREBARINFO) (lpcs->lpCreateParams));
#undef lpcs
        return TRUE;

    case WM_DESTROY:

        RBDestroy(prb);
        SetWindowPtr(hwnd, 0, 0);
        break;

    case WM_CREATE:
        // Do delayed stuff for speed.
        PostMessage(hwnd, RB_PRIV_DODELAYEDSTUFF, 0, 0);
        goto CallDWP;

    case RB_PRIV_DODELAYEDSTUFF:
        // Delay done stuff for speed:

        if (prb->ci.style & RBS_REGISTERDROP)
            prb->hDragProxy = CreateDragProxy(prb->ci.hwnd, RebarDragCallback, TRUE);
                
        if (prb->ci.style & RBS_TOOLTIPS)
        {
            TOOLINFO ti;
            // don't bother setting the rect because we'll do it below
            // in FlushToolTipsMgr;
            ti.cbSize = sizeof(ti);
            ti.uFlags = TTF_IDISHWND;
            ti.hwnd = hwnd;
            ti.uId = (UINT_PTR)hwnd;
            ti.lpszText = 0;

            prb->hwndToolTips = CreateWindow(c_szSToolTipsClass, NULL,
                    WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    hwnd, NULL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

            SendMessage(prb->hwndToolTips, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO) &ti);
        }
        RBInitPaletteHack(prb);
        break;

    case WM_NCHITTEST:
        {
            RBHITTESTINFO rbht;
            int iBand;
            
            rbht.pt.x = GET_X_LPARAM(lParam);
            rbht.pt.y = GET_Y_LPARAM(lParam);
            ScreenToClient(prb->ci.hwnd, &rbht.pt);

            iBand = RBHitTest(prb, &rbht);
            {
                NMMOUSE nm;
                LRESULT lres;
                
                nm.dwItemSpec = iBand;
                nm.pt = rbht.pt;
                nm.dwHitInfo = rbht.flags;
                
                // send to the parent to give them a chance to override
                lres = CCSendNotify(&prb->ci, NM_NCHITTEST, &nm.hdr);
                if (lres)
                    return lres;
                
            }
        }
        return HTCLIENT;

    case WM_NCCALCSIZE:
        if (prb->ci.style & WS_BORDER)
        {
            InflateRect((LPRECT) lParam, -g_cxEdge, -g_cyEdge);
            break;
        }
        goto CallDWP;

    case WM_NCPAINT:
        if (prb->ci.style & WS_BORDER)
        {
            RECT rc;
            HDC hdc;

            GetWindowRect(hwnd, &rc);
            OffsetRect(&rc, -rc.left, -rc.top);
            hdc = GetWindowDC(hwnd);
            CCDrawEdge(hdc, &rc, EDGE_ETCHED, BF_RECT, &(prb->clrsc));
            ReleaseDC(hwnd, hdc);
            break;
        }
        goto CallDWP;

    case WM_PALETTECHANGED:
        if ((HWND)wParam == hwnd)
            break;

    case WM_QUERYNEWPALETTE:
        // Want to pass FALSE if WM_QUERYNEWPALETTE...
        RBRealize(prb, NULL, uMsg == WM_PALETTECHANGED, uMsg == WM_PALETTECHANGED);
        return TRUE;

    case WM_PAINT:
    case WM_PRINTCLIENT:
        RBPaint(prb, (HDC)wParam);
        break;

    case WM_ERASEBKGND:
        if (RBEraseBkgnd(prb, (HDC) wParam, -1))
            return(TRUE);
        goto CallDWP;

    case WM_SYSCOLORCHANGE:
        RBInitPaletteHack(prb);

        if (prb->hwndToolTips)
            SendMessage(prb->hwndToolTips, uMsg, wParam, lParam);

        InitGlobalColors();
        InvalidateRect(prb->ci.hwnd, NULL, TRUE);

        break;


    case RB_SETPALETTE:
        return (LRESULT)RBSetPalette(prb, (HPALETTE)lParam);

    case RB_GETPALETTE:
        return (LRESULT)prb->hpal;

    case WM_SIZE:
        RBAutoSize(prb);
        RBResize(prb, FALSE);
        break;

    case WM_GETFONT:
        return((LRESULT) (prb ? prb->hFont : NULL));

    case WM_COMMAND:
    case WM_DRAWITEM:
    case WM_MEASUREITEM:
    case WM_VKEYTOITEM:
    case WM_CHARTOITEM:
        SendMessage(prb->ci.hwndParent, uMsg, wParam, lParam);
        break;

    case WM_LBUTTONDBLCLK:  // DBLCLK sent in place of LBUTTONDOWN
    case WM_RBUTTONDOWN:    // right button drags too
    case WM_LBUTTONDOWN:
        {
            RBHITTESTINFO rbht;
            PRBB prbb = NULL;

            rbht.pt.x = GET_X_LPARAM(lParam);
            rbht.pt.y = GET_Y_LPARAM(lParam);

            RelayToToolTips(prb->hwndToolTips, hwnd, uMsg, wParam, lParam);

            iBand = RBHitTest(prb, &rbht);
            if (iBand != -1)
                prbb = RBGETBAND(prb, iBand);

            if (!prbb)
                /* nothing */ ;
            else if (rbht.flags & RBHT_CHEVRON)
            {
                RBOnPushChevron(hwnd, prb, prbb, 0);
            }
            else if (rbht.flags != RBHT_CLIENT && RBShouldDrawGripper(prb, prbb))
            {
                prb->iCapture = iBand;
                prb->ptCapture = rbht.pt;
                if (prb->ci.style & CCS_VERT) 
                    SWAP(prb->ptCapture.x, prb->ptCapture.y, int);
                prb->xStart = prbb->x;
                SetCapture(hwnd);
                prb->fFullOnDrag = FALSE;

                if (uMsg == WM_LBUTTONDBLCLK && (prb->ci.style & RBS_DBLCLKTOGGLE))
                    RBToggleBand(prb,TRUE);
            }
        }
        break;

    case WM_SETCURSOR:
        // Give the parent first crack, if it sets the cursor then
        // leave it at that.  Otherwise if the cursor is over our
        // window then set it to what we want it to be.
        if (!DefWindowProc(hwnd, uMsg, wParam, lParam) && (hwnd == (HWND)wParam))
        {
            POINT   pt;
            GetMessagePosClient(prb->ci.hwnd, &pt);
            RBSetCursor(prb, pt.x, pt.y,  (HIWORD(lParam) == WM_LBUTTONDOWN || HIWORD(lParam) == WM_RBUTTONDOWN));
        }
        return TRUE;

    case WM_MOUSEMOVE:
        RBOnMouseMove(hwnd, uMsg, wParam, lParam, prb);
        break;

    case WM_RBUTTONUP:
        if (!prb->fFullOnDrag && !prb->fParentDrag) {
            CCReleaseCapture(&prb->ci);

            // if we're not doing drag drop, go to def window proc so that
            // wm_contextmenu gets propagated
            RBOnBeginDrag(prb, (UINT)-1);
            goto CallDWP;
        }
        // fall through

    case WM_LBUTTONUP:
        RelayToToolTips(prb->hwndToolTips, hwnd, uMsg, wParam, lParam);

        if (prb->iCapture != -1)
        {
            UINT uiIndex;

            if (!prb->fParentDrag)
                CCReleaseCapture(&prb->ci);
            // if there was no significant mouse motion, treat as a click
            if (!(prb->ci.style & RBS_DBLCLKTOGGLE) && !prb->fFullOnDrag)
                RBToggleBand(prb,TRUE);

            RBGETBAND(prb, prb->iCapture)->fStyle &= ~RBBS_DRAGBREAK;
            CCSendNotify(&prb->ci, RBN_LAYOUTCHANGED, NULL);
            RBSendNotify(prb, prb->iCapture, RBN_ENDDRAG);
            RBOnBeginDrag(prb, (UINT)-1);
            for (uiIndex = 0; uiIndex < prb->cBands; uiIndex++) {
                if (RBGETBAND(prb, uiIndex)->fStyle & RBBS_HIDDEN)
                    continue;

                RBBCalcMinWidth(prb, RBGETBAND(prb, uiIndex));
            }

            RBSizeBandsToRect(prb, NULL);
            RBInvalidateRect(prb, NULL);
        }
        break;

    case WM_WININICHANGE:
        InitGlobalMetrics(wParam);
        if (prb->fFontCreated)
            RBSetFont(prb, wParam);

        if (prb->hwndToolTips)
            SendMessage(prb->hwndToolTips, uMsg, wParam, lParam);

        break;

    case WM_SETFONT:
        RBOnSetFont(prb, (HFONT)wParam);
        break;

    case WM_NOTIFYFORMAT:
        return(CIHandleNotifyFormat(&prb->ci, lParam));

    case WM_NOTIFY:
        // We are just going to pass this on to the real parent
        // Note that -1 is used as the hwndFrom.  This prevents SendNotifyEx
        // from updating the NMHDR structure.
        return(SendNotifyEx(prb->ci.hwndParent, (HWND) -1,
                 ((LPNMHDR) lParam)->code, (LPNMHDR) lParam, prb->ci.bUnicode));

    case WM_STYLECHANGED:
        RBOnStyleChanged(prb, wParam, (LPSTYLESTRUCT)lParam);
        break;

#ifdef KEYBOARDCUES
    case WM_UPDATEUISTATE:
        if (CCOnUIState(&(prb->ci), WM_UPDATEUISTATE, wParam, lParam))
        {
            InvalidateRect(hwnd, NULL, TRUE);
        }
        goto CallDWP;
#endif
#ifdef UNICODE
    case RB_SETBANDINFOA:
    case RB_INSERTBANDA:
        if (EVAL(lParam))
        {
            LPWSTR lpStrings = NULL;
            LPSTR  lpAnsiString;
            int    iResult;

            // lParam starts out pointing to a REBARBANDINFOA, and
            // we secretly change it into a REBARBANDINFOW, and then
            // change it back.

            LPREBARBANDINFOW prbiW = (LPREBARBANDINFOW)lParam;
            LPREBARBANDINFOA prbiA = (LPREBARBANDINFOA)lParam;

            COMPILETIME_ASSERT(sizeof(REBARBANDINFOW) == sizeof(REBARBANDINFOA));

            // BUGBUG - raymondc - Is it safe to modify the incoming
            // REBARBANDINFOA structure?

            lpAnsiString = prbiA->lpText;
            if ((prbiA->fMask & RBBIM_TEXT) && prbiA->lpText) {

                lpStrings = ProduceWFromA(prb->ci.uiCodePage, lpAnsiString);
                if (!lpStrings)
                    return -1;

                // Presto!  Now it's a REBARBANDINFOW!
                prbiW->lpText = lpStrings;
            }

            if (uMsg == RB_INSERTBANDA)
                iResult = RBInsertBand(prb, (UINT) wParam, prbiW);
            else
                iResult = RBSetBandInfo(prb, (UINT) wParam, prbiW, TRUE);

            // Change-o!  Now it's a REBARBANDINFOA!
            prbiA->lpText = lpAnsiString;

            if (lpStrings)
                FreeProducedString(lpStrings);

            return iResult;
        }
#endif

    case RB_INSERTBAND:
        return(RBInsertBand(prb, (UINT) wParam, (LPREBARBANDINFO) lParam));

    case RB_DELETEBAND:
        return(RBDeleteBand(prb, (UINT) wParam));

    case RB_SHOWBAND:
        return(RBShowBand(prb, (UINT) wParam, BOOLFROMPTR(lParam)));

#ifdef UNICODE
    case RB_GETBANDINFOA:
        {
            LPREBARBANDINFOA prbbi = (LPREBARBANDINFOA)lParam;
            LPWSTR pszW = NULL;
            LPSTR  lpAnsiString = prbbi->lpText;
            int    iResult;

            if (prbbi->fMask & RBBIM_TEXT) {
                pszW = LocalAlloc(LPTR, prbbi->cch * sizeof(WCHAR));
                if (!pszW)
                    return 0;
                prbbi->lpText = (LPSTR)pszW;
            }

            iResult = RBGetBandInfo(prb, (UINT)wParam, (LPREBARBANDINFO)lParam);

            if (pszW) {
                ConvertWToAN(prb->ci.uiCodePage, lpAnsiString, prbbi->cch, (LPWSTR)prbbi->lpText, -1);
                prbbi->lpText = lpAnsiString;
                LocalFree(pszW);
            }

            return iResult;
        }
#endif

        // we have getbandinfoold because in ie3, we did not thunk
        // and getbandinfo always return OS native string (dumb)
    case RB_GETBANDINFOOLD:
    case RB_GETBANDINFO:
        return(RBGetBandInfo(prb, (UINT) wParam, (LPREBARBANDINFO) lParam));
            
    case RB_GETTOOLTIPS:
        return (LPARAM)prb->hwndToolTips;
            
    case RB_SETTOOLTIPS:
        prb->hwndToolTips = (HWND)wParam;
        break;
            
    case RB_SETBKCOLOR:
        {
            COLORREF clr = prb->clrBk;
            prb->clrBk = (COLORREF)lParam;
            if (clr != prb->clrBk)
                InvalidateRect(prb->ci.hwnd, NULL, TRUE);
            return clr;
        }
            
    case RB_GETBKCOLOR:
        return prb->clrBk;
            
    case RB_SETTEXTCOLOR:
        {
            COLORREF clr = prb->clrText;
            prb->clrText = (COLORREF)lParam;
            return clr;
        }
            
    case RB_GETTEXTCOLOR:
        return prb->clrText;

    case RB_IDTOINDEX:
        return RBIDToIndex(prb, (UINT) wParam);

    case RB_GETROWCOUNT:
        return(RBGetRowCount(prb));

    case RB_GETROWHEIGHT:
        return RBGetRowHeight(prb, (UINT)wParam);
        
    case RB_GETBANDBORDERS:
        return RBGetBandBorders(prb, (UINT)wParam, (LPRECT)lParam);

    case RB_GETBANDCOUNT:
        return(prb->cBands);

    case RB_SETBANDINFO:
        return(RBSetBandInfo(prb, (UINT) wParam, (LPREBARBANDINFO) lParam, TRUE));

    case RB_GETBARINFO:
        return(RBGetBarInfo(prb, (LPREBARINFO) lParam));

    case RB_SETBARINFO:
        return(RBSetBarInfo(prb, (LPREBARINFO) lParam));

    case RB_SETPARENT:
        {
            HWND hwndOld = prb->ci.hwndParent;
            prb->ci.hwndParent = (HWND) wParam;
            return (LRESULT)hwndOld;
        }
        break;

    case RB_GETRECT:
        if (RB_ISVALIDINDEX(prb, wParam))
        {
            PRBB prbb = RBGETBAND(prb, (int) wParam);
            LPRECT lprc = (LPRECT) lParam;

            lprc->left = prbb->x;
            lprc->top = prbb->y;
            lprc->right = prbb->x + prbb->cx;
            lprc->bottom = prbb->y + prbb->cy;

            return(TRUE);
        }
        break;

    case RB_HITTEST:
        return(RBHitTest(prb, (LPRBHITTESTINFO) lParam));

    case RB_SIZETORECT:
        return RBSizeBarToRect(prb, (LPRECT)lParam);

    case RB_BEGINDRAG:

        if (RB_ISVALIDINDEX(prb, wParam)) {
            // -1 means do it yourself.
            // -2 means use what you had saved before
            if (lParam != (LPARAM)-2) {
                if (lParam == (LPARAM)-1) {
                    GetMessagePosClient(prb->ci.hwnd, &prb->ptCapture);
                } else {
                    prb->ptCapture.x = GET_X_LPARAM(lParam);
                    prb->ptCapture.y = GET_Y_LPARAM(lParam);
                }
                if (prb->ci.style & CCS_VERT) 
                    SWAP(prb->ptCapture.x, prb->ptCapture.y, int);
            }

            prb->xStart = RBGETBAND(prb, (UINT)wParam)->x;

            RBOnBeginDrag(prb, (UINT)wParam);
        }
        break;
        
    case RB_GETBARHEIGHT:
        return RBGETBARHEIGHT(prb);
        
    case RB_ENDDRAG:
        RBOnBeginDrag(prb, (UINT)-1);
        break;
        
    case RB_DRAGMOVE:
        if (prb->iCapture != -1) {
            if (lParam == (LPARAM)-1) {
                lParam = GetMessagePosClient(prb->ci.hwnd, NULL);
            }
            RBDragBand(prb, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        break;
        
    case RB_MINIMIZEBAND:
        RBMinimizeBand(prb, (UINT) wParam,FALSE);
        break;

    case RB_MAXIMIZEBAND:
        RBMaximizeBand(prb, (UINT)wParam, BOOLFROMPTR(lParam),FALSE);
        break;

    case RB_MOVEBAND:
        if (!RB_ISVALIDINDEX(prb,wParam) || !RB_ISVALIDINDEX(prb,lParam))
            break;
        return RBMoveBand(prb, (UINT) wParam, (UINT) lParam);

    case RB_GETDROPTARGET:
        if (!prb->hDragProxy)
            prb->hDragProxy = CreateDragProxy(prb->ci.hwnd, RebarDragCallback, FALSE);

        GetDragProxyTarget(prb->hDragProxy, (IDropTarget**)lParam);
        break;

    case RB_GETCOLORSCHEME:
        {
            LPCOLORSCHEME lpclrsc = (LPCOLORSCHEME) lParam;
            if (lpclrsc) {
                if (lpclrsc->dwSize == sizeof(COLORSCHEME))
                    *lpclrsc = prb->clrsc;
            }
            return (LRESULT) lpclrsc;
        }

    case RB_SETCOLORSCHEME:
        if (lParam) {
            if (((LPCOLORSCHEME) lParam)->dwSize == sizeof(COLORSCHEME)) {
                prb->clrsc.clrBtnHighlight = ((LPCOLORSCHEME) lParam)->clrBtnHighlight;
                prb->clrsc.clrBtnShadow = ((LPCOLORSCHEME) lParam)->clrBtnShadow;        
                InvalidateRect(hwnd, NULL, FALSE);
                if (prb->ci.style & WS_BORDER)
                    CCInvalidateFrame(hwnd);
            }
        }
        break;

    case RB_PUSHCHEVRON:
        if (RB_ISVALIDINDEX(prb, wParam)) {
            PRBB prbb = RBGETBAND(prb, wParam);
            RBOnPushChevron(hwnd, prb, prbb, lParam);
        }
        break;

    default:
        {
            LRESULT lres;
            if (CCWndProc(&prb->ci, uMsg, wParam, lParam, &lres))
                return lres;
        }
        
CallDWP:
        return(DefWindowProc(hwnd, uMsg, wParam, lParam));
    }

    return(0L);
}
