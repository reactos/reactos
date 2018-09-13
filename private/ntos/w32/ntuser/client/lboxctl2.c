/***************************************************************************\
*
*  LBOXCTL2.C -
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
*      List box handling routines
*
* 18-Dec-1990 ianja    Ported from Win 3.0 sources
* 14-Feb-1991 mikeke   Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define LB_KEYDOWN WM_USER+1
#define NOMODIFIER  0  /* No modifier is down */
#define SHIFTDOWN   1  /* Shift alone */
#define CTLDOWN     2  /* Ctl alone */
#define SHCTLDOWN   (SHIFTDOWN + CTLDOWN)  /* Ctrl + Shift */

/*
 * Variables for incremental type search support
 */
#define MAX_TYPESEARCH  256

BOOL LBGetDC(PLBIV plb);
void LBReleaseDC(PLBIV plb);

/***************************************************************************\
*
*  LBInvalidateRect()
*
*  If the listbox is visible, invalidates a rectangle in the listbox.
*  If the listbox is not visible, sets the defer update flag for the listbox
*
\***************************************************************************/
BOOL xxxLBInvalidateRect(PLBIV plb, LPRECT lprc, BOOL fErase)
{
    CheckLock(plb->spwnd);

    if (IsLBoxVisible(plb)) {
        NtUserInvalidateRect(HWq(plb->spwnd), lprc, fErase);
        return(TRUE);
    }

    if (!plb->fRedraw)
        plb->fDeferUpdate = TRUE;

    return(FALSE);
}

/***************************************************************************\
*
*  LBGetBrush()
*
*  Gets background brush & colors for listbox.
*
\***************************************************************************/
HBRUSH xxxLBGetBrush(PLBIV plb, HBRUSH *phbrOld)
{
    HBRUSH  hbr;
    HBRUSH  hbrOld;
    TL tlpwndParent;

    CheckLock(plb->spwnd);

    SetBkMode(plb->hdc, OPAQUE);

    //
    // Get brush & colors
    //
    if ((plb->spwnd->spwndParent == NULL) ||
        (REBASEPWND(plb->spwnd, spwndParent) == _GetDesktopWindow())) {
        ThreadLock(plb->spwndParent, &tlpwndParent);
        hbr = GetControlColor(HW(plb->spwndParent), HWq(plb->spwnd),
                              plb->hdc, WM_CTLCOLORLISTBOX);
        ThreadUnlock(&tlpwndParent);
    } else
        hbr = GetControlBrush(HWq(plb->spwnd), plb->hdc, WM_CTLCOLORLISTBOX);

    //
    // Select brush into dc
    //
    if (hbr != NULL) {
        hbrOld = SelectObject(plb->hdc, hbr);
        if (phbrOld)
            *phbrOld = hbrOld;
    }

    return(hbr);
}


/***************************************************************************\
*
*  LBInitDC()
*
*  Initializes dc for listbox
*
\***************************************************************************/
void LBInitDC(PLBIV plb)
{
    RECT    rc;

    // Set font
    if (plb->hFont)
        SelectObject(plb->hdc, plb->hFont);

    // Set clipping area
    _GetClientRect(plb->spwnd, &rc);
    IntersectClipRect(plb->hdc, rc.left, rc.top, rc.right, rc.bottom);

    OffsetWindowOrgEx(plb->hdc, plb->xOrigin, 0, NULL);
}


/***************************************************************************\
* LBGetDC
*
* Returns a DC which can be used by a list box even if parentDC is in effect
*
* History:
\***************************************************************************/

BOOL LBGetDC(
    PLBIV plb)
{
    if (plb->hdc)
        return(FALSE);

    plb->hdc = NtUserGetDC(HWq(plb->spwnd));

    LBInitDC(plb);

    return TRUE;
}

/***************************************************************************\
*
*  LBTermDC()
*
*  Cleans up when done with listbox dc.
*
\***************************************************************************/
void LBTermDC(PLBIV plb)
{
    if (plb->hFont)
        SelectObject(plb->hdc, ghFontSys);
}



/***************************************************************************\
* LBReleaseDC
*
* History:
\***************************************************************************/

void LBReleaseDC(
    PLBIV plb)
{
    LBTermDC(plb);
    NtUserReleaseDC(HWq(plb->spwnd), plb->hdc);
    plb->hdc = NULL;
}


/***************************************************************************\
* LBGetItemRect
*
* Return the rectangle that the item will be drawn in with respect to the
* listbox window.  Returns TRUE if any portion of the item's rectangle
* is visible (ie. in the listbox client rect) else returns FALSE.
*
* History:
\***************************************************************************/

BOOL LBGetItemRect(
    PLBIV plb,
    INT sItem,
    LPRECT lprc)
{
    INT sTmp;
    int clientbottom;

    /*
     * Always allow an item number of 0 so that we can draw the caret which
     * indicates the listbox has the focus even though it is empty.

     * FreeHand 3.1 passes in -1 as the itemNumber and expects
     * a non-null rectangle. So we check for -1 specifically.
     * BUGTAG: Fix for Bug #540 --Win95B-- SANKAR -- 2/20/95 --
     */

    if (sItem && (sItem != -1) && ((UINT)sItem >= (UINT)plb->cMac))
    {
        SetRectEmpty(lprc);
        RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
        return (LB_ERR);
    }

    _GetClientRect(plb->spwnd, lprc);

    if (plb->fMultiColumn) {

        /*
         * itemHeight * sItem mod number ItemsPerColumn (itemsPerColumn)
         */
        lprc->top = plb->cyChar * (sItem % plb->itemsPerColumn);
        lprc->bottom = lprc->top + plb->cyChar  /*+(plb->OwnerDraw ? 0 : 1)*/;

        UserAssert(plb->itemsPerColumn);

        if (plb->fRightAlign) {
            lprc->right = lprc->right - plb->cxColumn *
                 ((sItem / plb->itemsPerColumn) - (plb->iTop / plb->itemsPerColumn));

            lprc->left = lprc->right - plb->cxColumn;
        } else {
            /*
             * Remember, this is integer division here...
             */
            lprc->left += plb->cxColumn *
                      ((sItem / plb->itemsPerColumn) - (plb->iTop / plb->itemsPerColumn));

            lprc->right = lprc->left + plb->cxColumn;
        }
    } else if (plb->OwnerDraw == OWNERDRAWVAR) {

        /*
         * Var height owner draw
         */
        lprc->right += plb->xOrigin;
        clientbottom = lprc->bottom;

        if (sItem >= plb->iTop) {
            for (sTmp = plb->iTop; sTmp < sItem; sTmp++) {
                lprc->top = lprc->top + LBGetVariableHeightItemHeight(plb, sTmp);
            }

            /*
             * If item number is 0, it may be we are asking for the rect
             * associated with a nonexistant item so that we can draw a caret
             * indicating focus on an empty listbox.
             */
            lprc->bottom = lprc->top + (sItem < plb->cMac ? LBGetVariableHeightItemHeight(plb, sItem) : plb->cyChar);
            return (lprc->top < clientbottom);
        } else {

            /*
             * Item we want the rect of is before plb->iTop.  Thus, negative
             * offsets for the rect and it is never visible.
             */
            for (sTmp = sItem; sTmp < plb->iTop; sTmp++) {
                lprc->top = lprc->top - LBGetVariableHeightItemHeight(plb, sTmp);
            }
            lprc->bottom = lprc->top + LBGetVariableHeightItemHeight(plb, sItem);
            return FALSE;
        }
    } else {

        /*
         * For fixed height listboxes
         */
        if (plb->fRightAlign && !(plb->fMultiColumn || plb->OwnerDraw) && plb->fHorzBar)
            lprc->right += plb->xOrigin + (plb->xRightOrigin - plb->xOrigin);
        else
            lprc->right += plb->xOrigin;
        lprc->top = (sItem - plb->iTop) * plb->cyChar;
        lprc->bottom = lprc->top + plb->cyChar;
    }

    return (sItem >= plb->iTop) &&
            (sItem < (plb->iTop + CItemInWindow(plb, TRUE)));
}


/***************************************************************************\
*
*  LBPrintCallback
*
*  Called back from DrawState()
*
\***************************************************************************/
BOOL CALLBACK LBPrintCallback(HDC hdc, LPWSTR lpstr, PLBIV plb, int cx, int cy)
{
    int     xStart;
    UINT    cLen;
    RECT    rc;
    UINT    oldAlign;

    if (!lpstr) {
        return FALSE;
    }

    if (plb->fMultiColumn)
        xStart = 0;
    else
        xStart = 2;

    if (plb->fRightAlign) {
        oldAlign = SetTextAlign(hdc, TA_RIGHT | GetTextAlign(hdc));
        xStart = cx - xStart;
    }

    cLen = wcslen(lpstr);

    if (plb->fUseTabStops) {
        TabTextOut(hdc, xStart, 0, lpstr, cLen,
            (plb->iTabPixelPositions ? plb->iTabPixelPositions[0] : 0),
            (plb->iTabPixelPositions ? (LPINT)&plb->iTabPixelPositions[1] : NULL),
            plb->fRightAlign ? cx : 0, TRUE, GetTextCharset(plb->hdc));
    } else {
        rc.left     = 0;
        rc.top      = 0;
        rc.right    = cx;
        rc.bottom   = cy;

        if (plb->wMultiple)
            ExtTextOut(hdc, xStart, 0, ETO_OPAQUE, &rc, lpstr, cLen, NULL);
        else if (plb->fMultiColumn)
            ExtTextOut(hdc, xStart, 0, ETO_CLIPPED, &rc, lpstr, cLen, NULL);
        else {
            ExtTextOut(hdc, xStart, 0, 0, NULL, lpstr, cLen, NULL);

            /*
             * When the listbox is in the incremental search mode and the item
             * is highlighted (so we only draw in the current item), draw the
             * caret for search indication.
             */
            if ((plb->iTypeSearch != 0) && (plb->OwnerDraw == 0) &&
                    (GetBkColor(hdc) == SYSRGB(HIGHLIGHT))) {
                SIZE size;
                GetTextExtentPointW(hdc, lpstr, plb->iTypeSearch, &size);
                PatBlt(hdc, xStart + size.cx - 1, 1, 1, cy - 2, DSTINVERT);
            }
        }
    }

    if (plb->fRightAlign)
        SetTextAlign(hdc, oldAlign);

    return(TRUE);
}


/***************************************************************************\
* xxxLBDrawLBItem
*
* History:
\***************************************************************************/

void xxxLBDrawLBItem(
    PLBIV plb,
    INT sItem,
    LPRECT lprect,
    BOOL fHilite,
    HBRUSH hbr)
{
    LPWSTR lpstr;
    DWORD rgbSave;
    DWORD rgbBkSave;
    UINT    uFlags;
    HDC     hdc = plb->hdc;
    UINT  oldAlign;

    CheckLock(plb->spwnd);

    /*
     * If the item is selected, then fill with highlight color
     */
    if (fHilite) {
        FillRect(hdc, lprect, SYSHBR(HIGHLIGHT));
        rgbBkSave = SetBkColor(hdc, SYSRGB(HIGHLIGHT));
        rgbSave = SetTextColor(hdc, SYSRGB(HIGHLIGHTTEXT));
    } else {

        /*
         * If fUseTabStops, we must fill the background, because later we use
         * LBTabTheTextOutForWimps(), which fills the background only partially
         * Fix for Bug #1509 -- 01/25/91 -- SANKAR --
         */
        if ((hbr != NULL) && ((sItem == plb->iSelBase) || (plb->fUseTabStops))) {
            FillRect(hdc, lprect, hbr);
        }
    }

    uFlags = DST_COMPLEX;
    lpstr = GetLpszItem(plb, sItem);

    if (TestWF(plb->spwnd, WFDISABLED)) {
        if ((COLORREF)SYSRGB(GRAYTEXT) != GetBkColor(hdc))
            SetTextColor(hdc, SYSRGB(GRAYTEXT));
        else
            uFlags |= DSS_UNION;
    }

    if (plb->fRightAlign)
        uFlags |= DSS_RIGHT;

    if (plb->fRtoLReading)
        oldAlign = SetTextAlign(hdc, TA_RTLREADING | GetTextAlign(hdc));

    DrawState(hdc, SYSHBR(WINDOWTEXT),
        (DRAWSTATEPROC)LBPrintCallback,
        (LPARAM)lpstr,
        (WPARAM)plb,
        lprect->left,
        lprect->top,
        lprect->right-lprect->left,
        lprect->bottom-lprect->top,
        uFlags);

    if (plb->fRtoLReading)
        SetTextAlign(hdc, oldAlign);

    if (fHilite) {
        SetTextColor(hdc, rgbSave);
        SetBkColor(hdc, rgbBkSave);
    }
}


/***************************************************************************\
*
* LBSetCaret()
*
\***************************************************************************/
void xxxLBSetCaret(PLBIV plb, BOOL fSetCaret)
{
    RECT    rc;
    BOOL    fNewDC;

    if (plb->fCaret && ((BOOL) plb->fCaretOn != !!fSetCaret)) {
        if (IsLBoxVisible(plb)) {
            /* Turn the caret (located at plb->iSelBase) on */
            fNewDC = LBGetDC(plb);

            LBGetItemRect(plb, plb->iSelBase, &rc);

            if (fNewDC) {
                SetBkColor(plb->hdc, SYSRGB(WINDOW));
                SetTextColor(plb->hdc, SYSRGB(WINDOWTEXT));
            }

            if (plb->OwnerDraw) {
                /* Fill in the drawitem struct */
                UINT itemState = (fSetCaret) ? ODS_FOCUS : 0;

                if (IsSelected(plb, plb->iSelBase, HILITEONLY))
                    itemState |= ODS_SELECTED;

                xxxLBoxDrawItem(plb, plb->iSelBase, ODA_FOCUS, itemState, &rc);
            } else if (!TestWF(plb->spwnd, WEFPUIFOCUSHIDDEN)) {
                COLORREF crBk = SetBkColor(plb->hdc, SYSRGB(WINDOW));
                COLORREF crText = SetTextColor(plb->hdc, SYSRGB(WINDOWTEXT));

                DrawFocusRect(plb->hdc, &rc);

                SetBkColor(plb->hdc, crBk);
                SetTextColor(plb->hdc, crText);
            }

            if (fNewDC)
                LBReleaseDC(plb);
        }
        plb->fCaretOn = !!fSetCaret;
    }
}


/***************************************************************************\
* IsSelected
*
* History:
* 16-Apr-1992 beng      The NODATA listbox case
\***************************************************************************/

BOOL IsSelected(
    PLBIV plb,
    INT sItem,
    UINT wOpFlags)
{
    LPBYTE lp;

    if ((sItem >= plb->cMac) || (sItem < 0)) {
        RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
//        return LB_ERR;
        return(FALSE);
    }

    if (plb->wMultiple == SINGLESEL) {
        return (sItem == plb->iSel);
    }

    lp = plb->rgpch + sItem +
             (plb->cMac * (plb->fHasStrings
                                ? sizeof(LBItem)
                                : (plb->fHasData
                                    ? sizeof(LBODItem)
                                    : 0)));
    sItem = *lp;

    if (wOpFlags == HILITEONLY) {
        sItem >>= 4;
    } else {
        sItem &= 0x0F;  /* SELONLY */
    }

    return sItem;
}


/***************************************************************************\
* CItemInWindow
*
* Returns the number of items which can fit in a list box.  It
* includes the partially visible one at the bottom if fPartial is TRUE. For
* var height ownerdraw, return the number of items visible starting at iTop
* and going to the bottom of the client rect.
*
* History:
\***************************************************************************/

INT CItemInWindow(
    PLBIV plb,
    BOOL fPartial)
{
    RECT rect;

    if (plb->OwnerDraw == OWNERDRAWVAR) {
        return CItemInWindowVarOwnerDraw(plb, fPartial);
    }

    if (plb->fMultiColumn) {
        return plb->itemsPerColumn * (plb->numberOfColumns + (fPartial ? 1 : 0));
    }

    _GetClientRect(plb->spwnd, &rect);

    /*
     * fPartial must be considered only if the listbox height is not an
     * integral multiple of character height.
     * A part of the fix for Bug #3727 -- 01/14/91 -- SANKAR --
     */
    UserAssert(plb->cyChar);
    return (INT)((rect.bottom / plb->cyChar) +
            ((rect.bottom % plb->cyChar)? (fPartial ? 1 : 0) : 0));
}


/***************************************************************************\
* xxxLBoxCtlScroll
*
* Handles vertical scrolling of the listbox
*
* History:
\***************************************************************************/

void xxxLBoxCtlScroll(
    PLBIV plb,
    INT cmd,
    int yAmt)
{
    INT iTopNew;
    INT cItemPageScroll;
    DWORD dwTime = 0;

    CheckLock(plb->spwnd);

    if (plb->fMultiColumn) {

        /*
         * Don't allow vertical scrolling on a multicolumn list box.  Needed
         * in case app sends WM_VSCROLL messages to the listbox.
         */
        return;
    }

    cItemPageScroll = plb->cItemFullMax;

    if (cItemPageScroll > 1)
        cItemPageScroll--;

    if (plb->cMac) {
        iTopNew = plb->iTop;
        switch (cmd) {
        case SB_LINEUP:
            dwTime = yAmt;
            iTopNew--;
            break;

        case SB_LINEDOWN:
            dwTime = yAmt;
            iTopNew++;
            break;

        case SB_PAGEUP:
            if (plb->OwnerDraw == OWNERDRAWVAR) {
                iTopNew = LBPage(plb, plb->iTop, FALSE);
            } else {
                iTopNew -= cItemPageScroll;
            }
            break;

        case SB_PAGEDOWN:
            if (plb->OwnerDraw == OWNERDRAWVAR) {
                iTopNew = LBPage(plb, plb->iTop, TRUE);
            } else {
                iTopNew += cItemPageScroll;
            }
            break;

        case SB_THUMBTRACK:
        case SB_THUMBPOSITION: {

            /*
             * If the listbox contains more than 0xFFFF items
             * it means that the scrolbar can return a position
             * that cannot fit in a WORD (16 bits), so use
             * GetScrollInfo (which is slower) in this case.
             */
            if (plb->cMac < 0xFFFF) {
                iTopNew = yAmt;
            } else {
                SCROLLINFO si;

                si.cbSize   = sizeof(SCROLLINFO);
                si.fMask    = SIF_TRACKPOS;

                GetScrollInfo( HWq(plb->spwnd), SB_VERT, &si);

                iTopNew = si.nTrackPos;
            }
            break;
        }
        case SB_TOP:
            iTopNew = 0;
            break;

        case SB_BOTTOM:
            iTopNew = plb->cMac - 1;
            break;

        case SB_ENDSCROLL:
            plb->fSmoothScroll = TRUE;
            xxxLBSetCaret(plb, FALSE);
            xxxLBShowHideScrollBars(plb);
            xxxLBSetCaret(plb, TRUE);
            return;
        }

        xxxNewITopEx(plb, iTopNew, dwTime);
    }
}

/***************************************************************************\
* LBGetScrollFlags
*
\***************************************************************************/

DWORD LBGetScrollFlags(PLBIV plb, DWORD dwTime)
{
    DWORD dwFlags;

    if (GetAppCompatFlags(NULL) & GACF_NOSMOOTHSCROLLING)
        goto NoSmoothScrolling;

    if (dwTime != 0) {
        dwFlags = MAKELONG(SW_SCROLLWINDOW | SW_SMOOTHSCROLL | SW_SCROLLCHILDREN, dwTime);
    } else if (TEST_EffectPUSIF(PUSIF_LISTBOXSMOOTHSCROLLING) && plb->fSmoothScroll) {
        dwFlags = SW_SCROLLWINDOW | SW_SMOOTHSCROLL | SW_SCROLLCHILDREN;
        plb->fSmoothScroll = FALSE;
    } else {
NoSmoothScrolling:
        dwFlags = SW_SCROLLWINDOW | SW_INVALIDATE | SW_ERASE | SW_SCROLLCHILDREN;
    }

    return dwFlags;
}

/***************************************************************************\
* xxxLBoxCtlHScroll
*
* Supports horizontal scrolling of listboxes
*
* History:
\***************************************************************************/

void xxxLBoxCtlHScroll(
    PLBIV plb,
    INT cmd,
    int xAmt)
{
    int newOrigin = plb->xOrigin;
    int oldOrigin = plb->xOrigin;
    int windowWidth;
    RECT rc;
    DWORD dwTime = 0;

    CheckLock(plb->spwnd);

    /*
     * Update the window so that we don't run into problems with invalid
     * regions during the horizontal scroll.
     */
    if (plb->fMultiColumn) {

        /*
         * Handle multicolumn scrolling in a separate segment
         */
        xxxLBoxCtlHScrollMultiColumn(plb, cmd, xAmt);
        return;
    }

    _GetClientRect(plb->spwnd, &rc);
    windowWidth = rc.right;

    if (plb->cMac) {

        switch (cmd) {
        case SB_LINEUP:
            dwTime = xAmt;
            newOrigin -= plb->cxChar;
            break;

        case SB_LINEDOWN:
            dwTime = xAmt;
            newOrigin += plb->cxChar;
            break;

        case SB_PAGEUP:
            newOrigin -= (windowWidth / 3) * 2;
            break;

        case SB_PAGEDOWN:
            newOrigin += (windowWidth / 3) * 2;
            break;

        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            newOrigin = xAmt;
            break;

        case SB_TOP:
            newOrigin = 0;
            break;

        case SB_BOTTOM:
            newOrigin = plb->maxWidth;
            break;

        case SB_ENDSCROLL:
            plb->fSmoothScroll = TRUE;
            xxxLBSetCaret(plb, FALSE);
            xxxLBShowHideScrollBars(plb);
            xxxLBSetCaret(plb, TRUE);
            return;
        }

        xxxLBSetCaret(plb, FALSE);
        plb->xOrigin = newOrigin;
        plb->xOrigin = xxxSetLBScrollParms(plb, SB_HORZ);

        if ((cmd == SB_BOTTOM) && plb->fRightAlign) {
            /*
             * so we know where to draw from.
             */
            plb->xRightOrigin = plb->xOrigin;
        }

        if(oldOrigin != plb->xOrigin)  {
            HWND hwnd = HWq(plb->spwnd);
            DWORD dwFlags;

            dwFlags = LBGetScrollFlags(plb, dwTime);
            ScrollWindowEx(hwnd, oldOrigin-plb->xOrigin,
                0, NULL, &rc, NULL, NULL, dwFlags);
            UpdateWindow(hwnd);
        }

        xxxLBSetCaret(plb, TRUE);
    } else {
        // this is a less-than-ideal fix for ImageMind ScreenSaver (Win95
        // B#8252) but it works and it doesn't hurt anybody -- JEFFBOG 10/28/94
        xxxSetLBScrollParms(plb, SB_HORZ);
    }
}


/***************************************************************************\
* xxxLBoxCtlPaint
*
* History:
\***************************************************************************/

void xxxLBPaint(
    PLBIV plb,
    HDC hdc,
    LPRECT lprcBounds)
{
    INT i;
    RECT rect;
    RECT    scratchRect;
    BOOL    fHilite;
    INT iLastItem;
    HBRUSH hbrSave = NULL;
    HBRUSH hbrControl;
    BOOL fCaretOn;
    RECT    rcBounds;
    HDC     hdcSave;

    CheckLock(plb->spwnd);

    if (lprcBounds == NULL) {
        lprcBounds = &rcBounds;
        _GetClientRect(plb->spwnd, lprcBounds);
    }

    hdcSave = plb->hdc;
    plb->hdc = hdc;

    // Initialize dc.
    LBInitDC(plb);

    // Turn caret off
    if (fCaretOn = plb->fCaretOn)
        xxxLBSetCaret(plb, FALSE);

    hbrSave = NULL;
    hbrControl = xxxLBGetBrush(plb, &hbrSave);

    // Get listbox's client
    _GetClientRect(plb->spwnd, &rect);

    // Adjust width of client rect for scrolled amount
    // fix for #140, t-arthb
    if (plb->fRightAlign && !(plb->fMultiColumn || plb->OwnerDraw) && plb->fHorzBar)
        rect.right += plb->xOrigin + (plb->xRightOrigin - plb->xOrigin);
    else
        rect.right += plb->xOrigin;

    // Get the index of the last item visible on the screen. This is also
    // valid for var height ownerdraw.
    iLastItem = plb->iTop + CItemInWindow(plb,TRUE);
    iLastItem = min(iLastItem, plb->cMac - 1);

    // Fill in the background of the listbox if it's an empty listbox
    // or if we're doing a control print
    if (iLastItem == -1)
        FillRect(plb->hdc, &rect, hbrControl);


    // Allow AnimateWindow() catch the apps that do not use our DC when
    // drawing the list box
    SetBoundsRect(plb->hdc, NULL, DCB_RESET | DCB_ENABLE);

    for (i = plb->iTop; i <= iLastItem; i++) {

        /*
         * Note that rect contains the clientrect from when we did the
         * _GetClientRect so the width is correct.  We just need to adjust
         * the top and bottom of the rectangle to the item of interest.
         */
        rect.bottom = rect.top + plb->cyChar;

        if ((UINT)i < (UINT)plb->cMac) {

            /*
             * If var height, get the rectangle for the item.
             */
            if (plb->OwnerDraw == OWNERDRAWVAR || plb->fMultiColumn) {
                LBGetItemRect(plb, i, &rect);
            }

            if (IntersectRect(&scratchRect, lprcBounds, &rect)) {
                fHilite = !plb->fNoSel && IsSelected(plb, i, HILITEONLY);

                if (plb->OwnerDraw) {

                    /*
                     * Fill in the drawitem struct
                     */
                    xxxLBoxDrawItem(plb, i, ODA_DRAWENTIRE,
                            (UINT)(fHilite ? ODS_SELECTED : 0), &rect);
                } else {
                    xxxLBDrawLBItem(plb, i, &rect, fHilite, hbrControl);
                }
            }
        }
        rect.top = rect.bottom;
    }

    if (hbrSave != NULL)
        SelectObject(hdc, hbrSave);

    if (fCaretOn)
        xxxLBSetCaret(plb, TRUE);

    LBTermDC(plb);

    plb->hdc = hdcSave;
}


/***************************************************************************\
* ISelFromPt
*
* In the loword, returns the closest item number the pt is on. The high
* word is 0 if the point is within bounds of the listbox client rect and is
* 1 if it is outside the bounds.  This will allow us to make the invertrect
* disappear if the mouse is outside the listbox yet we can still show the
* outline around the item that would be selected if the mouse is brought back
* in bounds...
*
* History:
\***************************************************************************/

BOOL ISelFromPt(
    PLBIV plb,
    POINT pt,
    LPDWORD piItem)
{
    RECT rect;
    int y;
    UINT mouseHighWord = 0;
    INT sItem;
    INT sTmp;

    _GetClientRect(plb->spwnd, &rect);

    if (pt.y < 0) {

        /*
         * Mouse is out of bounds above listbox
         */
        *piItem = plb->iTop;
        return TRUE;
    } else if ((y = pt.y) > rect.bottom) {
        y = rect.bottom;
        mouseHighWord = 1;
    }

    if (pt.x < 0 || pt.x > rect.right)
        mouseHighWord = 1;

    /*
     * Now just need to check if y mouse coordinate intersects item's rectangle
     */
    if (plb->OwnerDraw != OWNERDRAWVAR) {
        if (plb->fMultiColumn) {
            if (y < plb->itemsPerColumn * plb->cyChar) {
                if (plb->fRightAlign)
                    sItem = plb->iTop + (INT)((y / plb->cyChar) +
                            ((rect.right - pt.x) / plb->cxColumn) * plb->itemsPerColumn);
                else
                    sItem = plb->iTop + (INT)((y / plb->cyChar) +
                            (pt.x / plb->cxColumn) * plb->itemsPerColumn);

            } else {

                /*
                 * User clicked in blank space at the bottom of a column.
                 * Just select the last item in the column.
                 */
                mouseHighWord = 1;
                sItem = plb->iTop + (plb->itemsPerColumn - 1) +
                        (INT)((pt.x / plb->cxColumn) * plb->itemsPerColumn);
            }
        } else {
            sItem = plb->iTop + (INT)(y / plb->cyChar);
        }
    } else {

        /*
         * VarHeightOwnerdraw so we gotta do this the hardway...   Set the x
         * coordinate of the mouse down point to be inside the listbox client
         * rectangle since we no longer care about it.  This lets us use the
         * point in rect calls.
         */
        pt.x = 8;
        pt.y = y;
        for (sTmp = plb->iTop; sTmp < plb->cMac; sTmp++) {
            (void)LBGetItemRect(plb, sTmp, &rect);
            if (PtInRect(&rect, pt)) {
                *piItem = sTmp;
                return mouseHighWord;
            }
        }

        /*
         * Point was at the empty area at the bottom of a not full listbox
         */
        *piItem = plb->cMac - 1;
        return mouseHighWord;
    }

    /*
     * Check if user clicked on the blank area at the bottom of a not full list.
     * Assumes > 0 items in the listbox.
     */
    if (sItem > plb->cMac - 1) {
        mouseHighWord = 1;
        sItem = plb->cMac - 1;
    }

    *piItem = sItem;
    return mouseHighWord;
}


/***************************************************************************\
* SetSelected
*
* This is used for button initiated changes of selection state.
*
*  fSelected : TRUE  if the item is to be set as selected, FALSE otherwise
*
*  wOpFlags : HILITEONLY = Modify only the Display state (hi-nibble)
*             SELONLY    = Modify only the Selection state (lo-nibble)
*             HILITEANDSEL = Modify both of them;
*
* History:
* 16-Apr-1992 beng      The NODATA listbox case
\***************************************************************************/

void SetSelected(
    PLBIV plb,
    INT iSel,
    BOOL fSelected,
    UINT wOpFlags)
{
    LPSTR lp;
    BYTE cMask;
    BYTE cSelStatus;

    if (iSel < 0 || iSel >= plb->cMac)
        return;

    if (plb->wMultiple == SINGLESEL) {
        if (fSelected)
            plb->iSel = iSel;
    } else {
        cSelStatus = (BYTE)fSelected;
        switch (wOpFlags) {
        case HILITEONLY:

            /*
             * Mask out lo-nibble
             */
            cSelStatus = (BYTE)(cSelStatus << 4);
            cMask = 0x0F;
            break;
        case SELONLY:

            /*
             * Mask out hi-nibble
             */
            cMask = 0xF0;
            break;
        case HILITEANDSEL:

            /*
             * Mask the byte fully
             */
            cSelStatus |= (cSelStatus << 4);
            cMask = 0;
            break;
        }
        lp = (LPSTR)(plb->rgpch) + iSel +
                (plb->cMac * (plb->fHasStrings
                                ? sizeof(LBItem)
                                : (plb->fHasData ? sizeof(LBODItem) : 0)));

        *lp = (*lp & cMask) | cSelStatus;
    }
}


/***************************************************************************\
* LastFullVisible
*
* Returns the last fully visible item in the listbox. This is valid
* for ownerdraw var height and fixed height listboxes.
*
* History:
\***************************************************************************/

INT LastFullVisible(
    PLBIV plb)
{
    INT iLastItem;

    if (plb->OwnerDraw == OWNERDRAWVAR || plb->fMultiColumn) {
        iLastItem = plb->iTop + CItemInWindow(plb, FALSE) - 1;
        iLastItem = max(iLastItem, plb->iTop);
    } else {
        iLastItem = min(plb->iTop + plb->cItemFullMax - 1, plb->cMac - 1);
    }
    return iLastItem;
}


/***************************************************************************\
* xxxInvertLBItem
*
* History:
\***************************************************************************/

void xxxInvertLBItem(
    PLBIV plb,
    INT i,
    BOOL fHilite)  /* The new selection state of the item */
{
    RECT rect;
    BOOL fCaretOn;
    HBRUSH hbrControl;
    BOOL    fNewDC;

    CheckLock(plb->spwnd);

    // Skip if item isn't showing.
    if (plb->fNoSel || (i < plb->iTop) || (i >= (plb->iTop + CItemInWindow(plb, TRUE))))
        return;

    if (IsLBoxVisible(plb)) {
        LBGetItemRect(plb, i, &rect);

        /*
         * Only turn off the caret if it is on.  This avoids annoying caret
         * flicker when nesting xxxCaretOns and xxxCaretOffs.
         */
        if (fCaretOn = plb->fCaretOn) {
            xxxLBSetCaret(plb, FALSE);
        }

        fNewDC = LBGetDC(plb);

        hbrControl = xxxLBGetBrush(plb, NULL);

        if (!plb->OwnerDraw) {
            if (!fHilite) {
                FillRect(plb->hdc, &rect, hbrControl);
                hbrControl = NULL;
            }

            xxxLBDrawLBItem(plb, i, &rect, fHilite, hbrControl);
        } else {

            /*
             * We are ownerdraw so fill in the drawitem struct and send off
             * to the owner.
             */
            xxxLBoxDrawItem(plb, i, ODA_SELECT,
                    (UINT)(fHilite ? ODS_SELECTED : 0), &rect);
        }

        if (fNewDC)
            LBReleaseDC(plb);

        /*
         * Turn the caret back on only if it was originally on.
         */
        if (fCaretOn) {
            xxxLBSetCaret(plb, TRUE);
        }
    }
}


/***************************************************************************\
* xxxResetWorld
*
* Resets everyone's selection and hilite state except items in the
* range sStItem to sEndItem (Both inclusive).
*
* History:
\***************************************************************************/

void xxxResetWorld(
    PLBIV plb,
    INT iStart,
    INT iEnd,
    BOOL fSelect)
{
    INT i;
    INT iLastInWindow;
    BOOL fCaretOn;

    CheckLock(plb->spwnd);

    /*
     * If iStart and iEnd are not in correct order we swap them
     */

    if (iStart > iEnd) {
        i = iStart;
        iStart = iEnd;
        iEnd = i;
    }

    if (plb->wMultiple == SINGLESEL) {
        if (plb->iSel != -1 && ((plb->iSel < iStart) || (plb->iSel > iEnd))) {
            xxxInvertLBItem(plb, plb->iSel, fSelect);
            plb->iSel = -1;
        }
        return;
    }

    iLastInWindow = plb->iTop + CItemInWindow(plb, TRUE);

    if (fCaretOn = plb->fCaretOn)
        xxxLBSetCaret(plb, FALSE);

    for (i = 0; i < plb->cMac; i++) {
        if (i == iStart)
            // skip range to be preserved
            i = iEnd;
        else {
            if ((plb->iTop <= i) && (i <= iLastInWindow) &&
                (fSelect != IsSelected(plb, i, HILITEONLY)))
                // Only invert the item if it is visible and present Selection
                // state is different from what is required.
                xxxInvertLBItem(plb, i, fSelect);

            // Set all items outside of preserved range to unselected
            SetSelected(plb, i, fSelect, HILITEANDSEL);
        }
    }

    if (fCaretOn)
        xxxLBSetCaret(plb, TRUE);

}


/***************************************************************************\
* xxxNotifyOwner
*
* History:
\***************************************************************************/

void xxxNotifyOwner(
    PLBIV plb,
    INT sEvt)
{
    TL tlpwndParent;

    CheckLock(plb->spwnd);

    ThreadLock(plb->spwndParent, &tlpwndParent);
    SendMessage(HW(plb->spwndParent), WM_COMMAND,
            MAKELONG(PTR_TO_ID(plb->spwnd->spmenu), sEvt), (LPARAM)HWq(plb->spwnd));
    ThreadUnlock(&tlpwndParent);
}


/***************************************************************************\
* xxxSetISelBase
*
* History:
\***************************************************************************/

void xxxSetISelBase(
    PLBIV plb,
    INT sItem)
{
    CheckLock(plb->spwnd);

    xxxLBSetCaret(plb, FALSE);
    plb->iSelBase = sItem;
    xxxLBSetCaret(plb, TRUE);

    if (FWINABLE()) {
        xxxInsureVisible(plb, plb->iSelBase, FALSE);
        if (_IsWindowVisible(plb->spwnd)) {
            LBEvent(plb, EVENT_OBJECT_FOCUS, sItem);
        }
    }
}


/***************************************************************************\
* xxxTrackMouse
*
* History:
\***************************************************************************/

void xxxTrackMouse(
    PLBIV plb,
    UINT wMsg,
    POINT pt)
{
    INT iSelFromPt;
    INT iSelTemp;
    BOOL mousetemp;
    BOOL fMouseInRect;
    RECT rcClient;
    UINT wModifiers = 0;
    BOOL fSelected;
    UINT uEvent = 0;
    INT trackPtRetn;
    HWND hwnd = HWq(plb->spwnd);
    TL tlpwndEdit;
    TL tlpwndParent;

    CheckLock(plb->spwnd);

    /*
     * Optimization:  do nothing if mouse not captured
     */
    if ((wMsg != WM_LBUTTONDOWN) && (wMsg != WM_LBUTTONDBLCLK)) {
        if (!plb->fCaptured) {
            return;
        }
        /*
         * If we are processing a WM_MOUSEMOVE but the mouse has not moved from
         * the previous point, then we may be dealing with a mouse "jiggle" sent
         * from the kernel (see zzzInvalidateDCCache).  If we process this, we will
         * snap the listbox selection back to where the mouse cursor is pointing,
         * even if the user has not touched the mouse.  FritzS: NT5 bug 220722.
         *  Some apps (like MSMoney98) rely on this, so added the bLastRITWasKeyboard
         * check.  MCostea #244450
         */
        if ((wMsg == WM_MOUSEMOVE) && RtlEqualMemory(&pt, &(plb->ptPrev), sizeof(POINT))
            && gpsi->bLastRITWasKeyboard) {
                RIPMSG0(RIP_WARNING, "xxxTrackMouse ignoring WM_MOUSEMOVE with no mouse movement");
                return;
        }
    }

    mousetemp = ISelFromPt(plb, pt, &iSelFromPt);

    /*
     * If we allow the user to cancel his selection then fMouseInRect is true if
     * the mouse is in the listbox client area otherwise it is false.  If we
     * don't allow the user to cancel his selection, then fMouseInRect will
     * always be true.  This allows us to implement cancelable selection
     * listboxes ie.  The selection reverts to the origional one if the user
     * releases the mouse outside of the listbox.
     */
    fMouseInRect = !mousetemp || !plb->pcbox;

    _GetClientRect(plb->spwnd, &rcClient);

    switch (wMsg) {
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
        /*
         * We want to divert mouse clicks.  If the user clicks outside
         * of a dropped down listbox, we want to popup it up, using
         * the current selection.
         */
        if (plb->fCaptured) {
            /*
             * If plb->pcbox is NULL, this is a listbox that
             * received a WM_LBUTTONDOWN again w/o receiving
             * a WM_LBUTTONUP for the previous WM_LBUTTONDOWN
             * bug
             */
            if (plb->pcbox && mousetemp) {
                _ClientToScreen(plb->spwnd, &pt);

                if (!PtInRect(&plb->spwnd->rcWindow, pt)) {
                    /*
                     * Cancel selection if clicked outside of combo;
                     * Accept if clicked on combo button or item.
                     */
                    xxxCBHideListBoxWindow(plb->pcbox, TRUE, FALSE);
                } else if (!PtInRect(&plb->spwnd->rcClient, pt)) {
                    /*
                     * Let it pass through.  Save, restore capture in
                     * case user is clicking on scrollbar.
                     */
                    plb->fCaptured = FALSE;
                    NtUserReleaseCapture();

                    SendMessageWorker(plb->spwnd, WM_NCLBUTTONDOWN,
                        FindNCHit(plb->spwnd, POINTTOPOINTS(pt)),
                        MAKELONG(pt.x, pt.y), FALSE);

                    NtUserSetCapture(hwnd);
                    plb->fCaptured = TRUE;
                }

                break;
            }

            plb->fCaptured = FALSE;
            NtUserReleaseCapture();
        }

        if (plb->pcbox) {

            /*
             * If this listbox is in a combo box, set the focus to the combo
             * box window so that the edit control/static text is also
             * activated
             */
            ThreadLock(plb->pcbox->spwndEdit, &tlpwndEdit);
            NtUserSetFocus(HWq(plb->pcbox->spwndEdit));
            ThreadUnlock(&tlpwndEdit);
        } else {

            /*
             * Get the focus if the listbox is clicked in and we don't
             * already have the focus.  If we don't have the focus after
             * this, run away...
             */
            NtUserSetFocus(hwnd);
            if (!plb->fCaret)
                return;
        }

        if (plb->fAddSelMode) {

            /*
             * If it is in "Add" mode, quit it using shift f8 key...
             * However, since we can't send shift key state, we have to turn
             * this off directly...
             */

            /*
             *SendMessage(HW(plb->spwnd),WM_KEYDOWN, (UINT)VK_F8, 0L);
             */

            /*
             * Switch off the Caret blinking
             */
            NtUserKillTimer(hwnd, IDSYS_CARET);

            /*
             * Make sure the caret does not vanish
             */
            xxxLBSetCaret(plb, TRUE);
            plb->fAddSelMode = FALSE;
        }

        if (!plb->cMac) {

            /*
             * Don't even bother handling the mouse if no items in the
             * listbox since the code below assumes >0 items in the
             * listbox.  We will just get the focus (the statement above) if
             * we don't already have it.
             */
            break;
        }

        if (mousetemp) {

            /*
             * Mouse down occurred in a empty spot.  Just ignore it.
             */
            break;
        }

        plb->fDoubleClick = (wMsg == WM_LBUTTONDBLCLK);

        if (!plb->fDoubleClick) {

            /*
             * This hack put in for the shell.  Tell the shell where in the
             * listbox the user clicked and at what item number.  The shell
             * can return 0 to continue normal mouse tracking or TRUE to
             * abort mouse tracking.
             */
            ThreadLock(plb->spwndParent, &tlpwndParent);
            trackPtRetn = (INT)SendMessage(HW(plb->spwndParent), WM_LBTRACKPOINT,
                    (DWORD)iSelFromPt, MAKELONG(pt.x+plb->xOrigin, pt.y));
            ThreadUnlock(&tlpwndParent);
            if (trackPtRetn) {
                if (trackPtRetn == 2) {

                    /*
                     * Ignore double clicks
                     */
                    NtUserCallNoParam(SFI__RESETDBLCLK);
                }
                return;
            }
        }

        if (plb->pcbox) {

            /*
             * Save the last selection if this is a combo box.  So that it
             * can be restored if user decides to cancel the selection by up
             * clicking outside the listbox.
             */
            plb->iLastSelection = plb->iSel;
        }

        /*
         * Save for timer
         */
        plb->ptPrev = pt;

        plb->fMouseDown = TRUE;
        NtUserSetCapture(hwnd);
        plb->fCaptured = TRUE;

        if (plb->fDoubleClick) {

            /*
             * Double click.  Fake a button up and exit
             */
            xxxTrackMouse(plb, WM_LBUTTONUP, pt);
            return;
        }

        /*
         * Set the system timer so that we can autoscroll if the mouse is
         * outside the bounds of the listbox rectangle
         */
        NtUserSetTimer(hwnd, IDSYS_SCROLL, gpsi->dtScroll, NULL);



        /*
         * If extended multiselection listbox, are any modifier key pressed?
         */
        if (plb->wMultiple == EXTENDEDSEL) {
            if (GetKeyState(VK_SHIFT) < 0)
                wModifiers = SHIFTDOWN;
            if (GetKeyState(VK_CONTROL) < 0)
                wModifiers += CTLDOWN;

            /*
             * Please Note that (SHIFTDOWN + CTLDOWN) == (SHCTLDOWN)
             */
        }


        switch (wModifiers) {
        case NOMODIFIER:
MouseMoveHandler:
            if (plb->iSelBase != iSelFromPt) {
                xxxLBSetCaret(plb, FALSE);
            }

            /*
             * We only look at the mouse if the point it is pointing to is
             * not selected.  Since we are not in ExtendedSelMode, anywhere
             * the mouse points, we have to set the selection to that item.
             * Hence, if the item isn't selected, it means the mouse never
             * pointed to it before so we can select it.  We ignore already
             * selected items so that we avoid flashing the inverted
             * selection rectangle.  Also, we could get WM_SYSTIMER simulated
             * mouse moves which would cause flashing otherwise...
             */

            iSelTemp = (fMouseInRect ? iSelFromPt : -1);

            /*
             * If the LB is either SingleSel or Extended multisel, clear all
             * old selections except the new one being made.
             */
            if (plb->wMultiple != MULTIPLESEL) {
                xxxResetWorld(plb, iSelTemp, iSelTemp, FALSE);
                /*
                 * This will be TRUE if iSelTemp isn't -1 (like below)
                 * and also if it is but there is a current selection.
                 */
                if ((iSelTemp == -1) && (plb->iSel != -1)) {
                    uEvent = EVENT_OBJECT_SELECTIONREMOVE;
                }
            }

            fSelected = IsSelected(plb, iSelTemp, HILITEONLY);
            if (iSelTemp != -1) {

                /*
                 * If it is MULTIPLESEL, then toggle; For others, only if
                 * not selected already, select it.
                 */
                if (((plb->wMultiple == MULTIPLESEL) && (wMsg != WM_LBUTTONDBLCLK)) || !fSelected) {
                    SetSelected(plb, iSelTemp, !fSelected, HILITEANDSEL);

                    /*
                     * And invert it
                     */
                    xxxInvertLBItem(plb, iSelTemp, !fSelected);
                    fSelected = !fSelected;  /* Set the new state */
                    if (plb->wMultiple == MULTIPLESEL) {
                        uEvent = (fSelected ? EVENT_OBJECT_SELECTIONADD :
                                EVENT_OBJECT_SELECTIONREMOVE);
                    } else {
                        uEvent = EVENT_OBJECT_SELECTION;
                    }
                }
            }

            /*
             * We have to set iSel in case this is a multisel lb.
             */
            plb->iSel = iSelTemp;

            /*
             * Set the new anchor point
             */
            plb->iMouseDown = iSelFromPt;
            plb->iLastMouseMove = iSelFromPt;
            plb->fNewItemState = fSelected;

            break;
        case SHIFTDOWN:

            /*
             * This is so that we can handle click and drag for multisel
             * listboxes using Shift modifier key .
             */
            plb->iLastMouseMove = plb->iSel = iSelFromPt;



            /*
             * Check if an anchor point already exists
             */
            if (plb->iMouseDown == -1) {
                plb->iMouseDown = iSelFromPt;

                /*
                 * Reset all the previous selections
                 */
                xxxResetWorld(plb, plb->iMouseDown, plb->iMouseDown, FALSE);

                /*
                 * Select the current position
                 */
                SetSelected(plb, plb->iMouseDown, TRUE, HILITEANDSEL);
                xxxInvertLBItem(plb, plb->iMouseDown, TRUE);
                /*
                 * We are changing the selction to this item only
                 */
                uEvent = EVENT_OBJECT_SELECTION;
            } else {

                /*
                 * Reset all the previous selections
                 */
                xxxResetWorld(plb, plb->iMouseDown, plb->iMouseDown, FALSE);

                /*
                 * Select all items from anchor point upto current click pt
                 */
                xxxAlterHilite(plb, plb->iMouseDown, iSelFromPt, HILITE, HILITEONLY, FALSE);
                uEvent = EVENT_OBJECT_SELECTIONWITHIN;
            }
            plb->fNewItemState = (UINT)TRUE;
            break;

        case CTLDOWN:

            /*
             * This is so that we can handle click and drag for multisel
             * listboxes using Control modifier key.
             */

            /*
             * Reset the anchor point to the current point
             */
            plb->iMouseDown = plb->iLastMouseMove = plb->iSel = iSelFromPt;

            /*
             * The state we will be setting items to
             */
            plb->fNewItemState = (UINT)!IsSelected(plb, iSelFromPt, (UINT)HILITEONLY);

            /*
             * Toggle the current point
             */
            SetSelected(plb, iSelFromPt, plb->fNewItemState, HILITEANDSEL);
            xxxInvertLBItem(plb, iSelFromPt, plb->fNewItemState);
            uEvent = (plb->fNewItemState ? EVENT_OBJECT_SELECTIONADD :
                    EVENT_OBJECT_SELECTIONREMOVE);
            break;

        case SHCTLDOWN:

            /*
             * This is so that we can handle click and drag for multisel
             * listboxes using Shift and Control modifier keys.
             */

            /*
             * Preserve all the previous selections
             */

            /*
             * Deselect only the selection connected with the last
             * anchor point; If the last anchor point is associated with a
             * de-selection, then do not do it
             */
            if (plb->fNewItemState) {
                xxxAlterHilite(plb, plb->iMouseDown, plb->iLastMouseMove, FALSE, HILITEANDSEL, FALSE);
            }
            plb->iLastMouseMove = plb->iSel = iSelFromPt;

            /*
             * Check if an anchor point already exists
             */
            if (plb->iMouseDown == -1) {

                /*
                 * No existing anchor point; Make the current pt as anchor
                 */
                plb->iMouseDown = iSelFromPt;
            }

            /*
             * If one exists preserve the most recent anchor point
             */

            /*
             * The state we will be setting items to
             */
            plb->fNewItemState = (UINT)IsSelected(plb, plb->iMouseDown, HILITEONLY);

            /*
             * Select all items from anchor point upto current click pt
             */
            xxxAlterHilite(plb, plb->iMouseDown, iSelFromPt, plb->fNewItemState, HILITEONLY, FALSE);
            uEvent = EVENT_OBJECT_SELECTIONWITHIN;
            break;
        }

        /*
         * Set the new base point (the outline frame caret).  We do the check
         * first to avoid flashing the caret unnecessarly.
         */
        if (plb->iSelBase != iSelFromPt) {

            /*
             * Since xxxSetISelBase always turns on the caret, we don't need to
             * do it here...
             */
            xxxSetISelBase(plb, iSelFromPt);
        }

        /*
         * SetISelBase will change the focus and send a focus event.
         * Then we send the selection event.
         */
        if (FWINABLE() && uEvent) {
            LBEvent(plb, uEvent, iSelFromPt);
        }
        if (wMsg == WM_LBUTTONDOWN && TestWF(plb->spwnd, WEFDRAGOBJECT)) {
            if (NtUserDragDetect(hwnd, pt)) {

                /*
                 * User is trying to drag object...
                 */

                /*
                 *  Fake an up click so that the item is selected...
                 */
                xxxTrackMouse(plb, WM_LBUTTONUP, pt);

                /*
                 * Notify parent
                 * #ifndef WIN16 (32-bit Windows), plb->iSelBase gets
                 * zero-extended to LONG wParam automatically by the compiler.
                 */
                ThreadLock(plb->spwndParent, &tlpwndParent);
                SendMessage(HW(plb->spwndParent), WM_BEGINDRAG, plb->iSelBase,
                        (LPARAM)hwnd);
                ThreadUnlock(&tlpwndParent);
            } else {
                xxxTrackMouse(plb, WM_LBUTTONUP, pt);
            }
            return;
        }
        break;

    case WM_MOUSEMOVE: {
        int dist;
        int iTimer;

        /*
         * Save for timer.
         */
        plb->ptPrev = pt;
        /*
         * Autoscroll listbox if mouse button is held down and mouse is
         * moved outside of the listbox
         */
        if (plb->fMouseDown) {
            if (plb->fMultiColumn) {
                if ((pt.x < 0) || (pt.x >= rcClient.right - 1)) {
                    /*
                     * Reset timer interval based on distance from listbox.
                     * use a longer default interval because each multicolumn
                     * scrolling increment is larger
                     */
                    dist = pt.x < 0 ? -pt.x : (pt.x - rcClient.right + 1);
                    iTimer = ((gpsi->dtScroll * 3) / 2) - ((WORD) dist << 4);

                    if (plb->fRightAlign)
                        xxxLBoxCtlHScrollMultiColumn(plb, (pt.x < 0 ? SB_LINEDOWN : SB_LINEUP), 0);
                    else
                        xxxLBoxCtlHScrollMultiColumn(plb, (pt.x < 0 ? SB_LINEUP : SB_LINEDOWN), 0);

                    goto SetTimerAndSel;
                }
            } else if ((pt.y < 0) || (pt.y >= rcClient.bottom - 1)) {
                /*
                 * Reset timer interval based on distance from listbox.
                 */
                dist = pt.y < 0 ? -pt.y : (pt.y - rcClient.bottom + 1);
                iTimer = gpsi->dtScroll - ((WORD) dist << 4);

                xxxLBoxCtlScroll(plb, (pt.y < 0 ? SB_LINEUP : SB_LINEDOWN), 0);
SetTimerAndSel:
                NtUserSetTimer(hwnd, IDSYS_SCROLL, max(iTimer, 1), NULL);
                ISelFromPt(plb, pt, &iSelFromPt);
            }
        } else {
            /*
             * Ignore if not in client since we don't autoscroll
             */
            if (!PtInRect(&rcClient, pt))
                break;
        }

        switch (plb->wMultiple) {
        case SINGLESEL:

            /*
             * If it is a single selection or plain multisel list box
             */
            goto MouseMoveHandler;
            break;

        case MULTIPLESEL:
        case EXTENDEDSEL:

            /*
             * Handle mouse movement with extended selection of items
             */
            if (plb->iSelBase != iSelFromPt) {
                xxxSetISelBase(plb, iSelFromPt);

                /*
                 * If this is an extended Multi sel list box, then
                 * adjust the display of the range due to the mouse move
                 */
                if (plb->wMultiple == EXTENDEDSEL) {
                    xxxLBBlockHilite(plb, iSelFromPt, FALSE);
                    if (FWINABLE())
                        LBEvent(plb, EVENT_OBJECT_SELECTIONWITHIN, iSelFromPt);
                }
                plb->iLastMouseMove = iSelFromPt;
            }
            break;
        }
        break;
    }
    case WM_LBUTTONUP:
        if (plb->fMouseDown)
            xxxLBButtonUp(plb, LBUP_RELEASECAPTURE | LBUP_NOTIFY |
                (mousetemp ? LBUP_RESETSELECTION : 0) |
                (fMouseInRect ? LBUP_SUCCESS : 0));
    }
}

/***************************************************************************\
*
*  LBButtonUp()
*
*  Called in response to both WM_CAPTURECHANGED and WM_LBUTTONUP.
*
\***************************************************************************/
void xxxLBButtonUp(PLBIV plb, UINT uFlags)
{

    CheckLock(plb->spwnd);

    /*
     * If the list box is an Extended listbox, then change the select status
     * of all items between the anchor and the last mouse position to the
     * newItemState
     */
    if (plb->wMultiple == EXTENDEDSEL)
        xxxAlterHilite(plb, plb->iMouseDown, plb->iLastMouseMove,
            plb->fNewItemState, SELONLY, FALSE);

    /*
     * This is a combo box and user upclicked outside the listbox
     * so we want to restore the original selection.
     */
    if (plb->pcbox && (uFlags & LBUP_RESETSELECTION)) {
        int iSelOld;

        iSelOld = plb->iSel;

        if (iSelOld >= 0)
            xxxInvertLBItem(plb, plb->iSel, FALSE);

        plb->iSel = plb->iLastSelection;
        xxxInvertLBItem(plb, plb->iSel, TRUE);

        /*
         * Note that we always send selection events before we tell the
         * app.  This is on purpose--the app may turn around and select
         * something else when notified.  In which case our event would
         * be out of order.
         */
        if (FWINABLE())
            LBEvent(plb, EVENT_OBJECT_SELECTION, plb->iSel);

        /*
         * On win-95 and NT4 the check used to be !(uFlags & LBUP_NOTIFY) which
         * is a bug because we would notify even when the lb is not LBUP_NOTIFY
         */
        if ((uFlags & LBUP_NOTIFY) && plb->fNotify && (iSelOld != plb->iSel))
            xxxNotifyOwner(plb, LBN_SELCHANGE);
    }

    NtUserKillTimer(HWq(plb->spwnd), IDSYS_SCROLL);
    plb->fMouseDown = FALSE;
    if (plb->fCaptured) {
        plb->fCaptured = FALSE;
        if (uFlags & LBUP_RELEASECAPTURE)
            NtUserReleaseCapture();
    }
    /*
     * Don't scroll item as long as any part of it is visible
     */
    if (plb->iSelBase < plb->iTop ||
        plb->iSelBase > plb->iTop + CItemInWindow(plb, TRUE))
        xxxInsureVisible(plb, plb->iSelBase, FALSE);

    if (plb->fNotify) {
        if (uFlags & LBUP_NOTIFY)  {
            if (uFlags & LBUP_SUCCESS) {
                /*
                 * ArtMaster needs this SELCHANGE notification now!
                 */
                if ((plb->fDoubleClick) && !TestWF(plb->spwnd, WFWIN31COMPAT))
                    xxxNotifyOwner(plb, LBN_SELCHANGE);

                /*
                 * Notify owner of click or double click on selection
                 */
                xxxNotifyOwner(plb, (plb->fDoubleClick) ? LBN_DBLCLK : LBN_SELCHANGE);
            } else {
                /*
                 * Notify owner that the attempted selection was cancelled.
                 */
                xxxNotifyOwner(plb, LBN_SELCANCEL);
            }
        } else if (uFlags & LBUP_SELCHANGE) {
            /*
             * Did we do some semi-selecting with mouse moves, then hit Enter?
             * If so, we need to make sure the app knows that something was
             * really truly selected.
             */
            UserAssert(TestWF(plb->spwnd, WFWIN40COMPAT));
            if (plb->iLastSelection != plb->iSel)
                xxxNotifyOwner(plb, LBN_SELCHANGE);

        }
    }

}


/***************************************************************************\
* IncrementISel
*
* History:
\***************************************************************************/

INT IncrementISel(
    PLBIV plb,
    INT iSel,
    INT sInc)
{

    /*
     * Assumes cMac > 0, return iSel+sInc in range [0..cmac).
     */
    iSel += sInc;
    if (iSel < 0) {
        return 0;
    } else if (iSel >= plb->cMac) {
        return plb->cMac - 1;
    }
    return iSel;
}


/***************************************************************************\
* NewITop
*
\***************************************************************************/

void xxxNewITop(PLBIV plb, INT iTopNew)
{
    xxxNewITopEx(plb, iTopNew, 0);
}


/***************************************************************************\
* xxxNewITopEx
*
* History:
\***************************************************************************/

void xxxNewITopEx(
    PLBIV plb,
    INT iTopNew,
    DWORD dwTime)
{
    int     iTopOld;
    BOOL fCaretOn;
    BOOL fMulti = plb->fMultiColumn;

    CheckLock(plb->spwnd);

    // Always try to turn off caret whether or not redraw is on
    if (fCaretOn = plb->fCaretOn)
        xxxLBSetCaret(plb, FALSE);

    iTopOld = (fMulti) ? (plb->iTop / plb->itemsPerColumn) : plb->iTop;
    plb->iTop = iTopNew;
    iTopNew = xxxSetLBScrollParms(plb, (fMulti) ? SB_HORZ : SB_VERT);
    plb->iTop = (fMulti) ? (iTopNew * plb->itemsPerColumn) : iTopNew;

    if (!IsLBoxVisible(plb)) {
        return;
    }

    if (iTopNew != iTopOld) {
        int     xAmt, yAmt;
        RECT    rc;
        DWORD   dwFlags;

        _GetClientRect(plb->spwnd, &rc);

        if (fMulti) {
            yAmt = 0;
            if (abs(iTopNew - iTopOld) > plb->numberOfColumns)
                // Handle scrolling a large number of columns properly so that
                // we don't overflow the size of a rect.
                xAmt = 32000;
            else {
                xAmt = (iTopOld - iTopNew) * plb->cxColumn;
                if (plb->fRightAlign)
                    xAmt = -xAmt;
            }
        } else {
            xAmt = 0;
            if (plb->OwnerDraw == OWNERDRAWVAR) {
                //
                // Have to fake iTopOld for OWNERDRAWVAR listboxes so that
                // the scrolling amount calculations work properly.
                //
                plb->iTop = iTopOld;
                yAmt = LBCalcVarITopScrollAmt(plb, iTopOld, iTopNew);
                plb->iTop = iTopNew;
            } else if (abs(iTopNew - iTopOld) > plb->cItemFullMax)
                yAmt = 32000;
            else
                yAmt = (iTopOld - iTopNew) * plb->cyChar;
        }

        dwFlags = LBGetScrollFlags(plb, dwTime);
        ScrollWindowEx(HWq(plb->spwnd), xAmt, yAmt, NULL, &rc, NULL,
                NULL, dwFlags);
        UpdateWindow(HWq(plb->spwnd));
    }

    // Note that although we turn off the caret regardless of redraw, we
    // only turn it on if redraw is true. Slimy thing to fixup many
    // caret related bugs...
    if (fCaretOn)
        // Turn the caret back on only if we turned it off. This avoids
        // annoying caret flicker.
        xxxLBSetCaret(plb, TRUE);
}


/***************************************************************************\
* xxxInsureVisible
*
* History:
\***************************************************************************/

void xxxInsureVisible(
    PLBIV plb,
    INT iSel,
    BOOL fPartial)  /* It is ok for the item to be partially visible */
{
    INT sLastVisibleItem;

    CheckLock(plb->spwnd);

    if (iSel < plb->iTop) {
        xxxNewITop(plb, iSel);
    } else {
        if (fPartial) {

            /*
             * 1 must be subtracted to get the last visible item
             * A part of the fix for Bug #3727 -- 01/14/91 -- SANKAR
             */
            sLastVisibleItem = plb->iTop + CItemInWindow(plb, TRUE) - (INT)1;
        } else {
            sLastVisibleItem = LastFullVisible(plb);
        }

        if (plb->OwnerDraw != OWNERDRAWVAR) {
            if (iSel > sLastVisibleItem) {
                if (plb->fMultiColumn) {
                    xxxNewITop(plb,
                        ((iSel / plb->itemsPerColumn) -
                        max(plb->numberOfColumns-1,0)) * plb->itemsPerColumn);
                } else {
                    xxxNewITop(plb, (INT)max(0, iSel - sLastVisibleItem + plb->iTop));
                }
            }
        } else if (iSel > sLastVisibleItem)
            xxxNewITop(plb, LBPage(plb, iSel, FALSE));
    }
}

/***************************************************************************\
* xxxLBoxCaretBlinker
*
* Timer callback function toggles Caret
* Since it is a callback, it is APIENTRY
*
* History:
\***************************************************************************/

VOID xxxLBoxCaretBlinker(
    HWND hwnd,
    UINT wMsg,
    UINT_PTR nIDEvent,
    DWORD dwTime)
{
    PWND pwnd;
    PLBIV plb;

    /*
     * Standard parameters for a timer callback function that aren't used.
     * Mentioned here to avoid compiler warnings
     */
    UNREFERENCED_PARAMETER(wMsg);
    UNREFERENCED_PARAMETER(nIDEvent);
    UNREFERENCED_PARAMETER(dwTime);

    pwnd = ValidateHwnd(hwnd);
    plb = ((PLBWND)pwnd)->pLBIV;

    /*
     * leave caret on, don't blink it off (prevents rapid blinks?)
     */
    if (ISREMOTESESSION() && plb->fCaretOn) {
        return;
    }

    /*
     * Check if the Caret is ON, if so, switch it OFF
     */
    xxxLBSetCaret(plb, !plb->fCaretOn);
    return;
}


/***************************************************************************\
* xxxLBoxCtlKeyInput
*
* If msg == LB_KEYDOWN, vKey is the number of the item to go to,
* otherwise it is the virtual key.
*
* History:
\***************************************************************************/

void xxxLBoxCtlKeyInput(
    PLBIV plb,
    UINT msg,
    UINT vKey)
{
    INT i;
    INT iNewISel;
    INT cItemPageScroll;
    PCBOX pcbox;
    BOOL fDropDownComboBox;
    BOOL fExtendedUIComboBoxClosed;
    BOOL hScrollBar = TestWF(plb->spwnd, WFHSCROLL);
    UINT wModifiers = 0;
    BOOL fSelectKey = FALSE;  /* assume it is a navigation key */
    UINT uEvent = 0;
    HWND hwnd = HWq(plb->spwnd);
    TL tlpwndParent;
    TL tlpwnd;

    CheckLock(plb->spwnd);

    pcbox = plb->pcbox;

    /*
     * Is this a dropdown style combo box/listbox ?
     */
    fDropDownComboBox = pcbox && (pcbox->CBoxStyle & SDROPPABLE);

    /*
     *Is this an extended ui combo box which is closed?
     */
    fExtendedUIComboBoxClosed = fDropDownComboBox && pcbox->fExtendedUI &&
                              !pcbox->fLBoxVisible;

    if (plb->fMouseDown || (!plb->cMac && vKey != VK_F4)) {

        /*
         * Ignore keyboard input if we are in the middle of a mouse down deal or
         * if there are no items in the listbox. Note that we let F4's go
         * through for combo boxes so that the use can pop up and down empty
         * combo boxes.
         */
        return;
    }

    /*
     * Modifiers are considered only in EXTENDED sel list boxes.
     */
    if (plb->wMultiple == EXTENDEDSEL) {

        /*
         * If multiselection listbox, are any modifiers used ?
         */
        if (GetKeyState(VK_SHIFT) < 0)
            wModifiers = SHIFTDOWN;
        if (GetKeyState(VK_CONTROL) < 0)
            wModifiers += CTLDOWN;

        /*
         * Please Note that (SHIFTDOWN + CTLDOWN) == (SHCTLDOWN)
         */
    }

    if (msg == LB_KEYDOWN) {

        /*
         * This is a listbox "go to specified item" message which means we want
         * to go to a particular item number (given by vKey) directly.  ie.  the
         * user has typed a character and we want to go to the item which
         * starts with that character.
         */
        iNewISel = (INT)vKey;
        goto TrackKeyDown;
    }

    cItemPageScroll = plb->cItemFullMax;

    if (cItemPageScroll > 1)
        cItemPageScroll--;

    if (plb->fWantKeyboardInput) {

        /*
         * Note: msg must not be LB_KEYDOWN here or we'll be in trouble...
         */
        ThreadLock(plb->spwndParent, &tlpwndParent);
        iNewISel = (INT)SendMessage(HW(plb->spwndParent), WM_VKEYTOITEM,
                MAKELONG(vKey, plb->iSelBase), (LPARAM)hwnd);
        ThreadUnlock(&tlpwndParent);

        if (iNewISel == -2) {

            /*
             * Don't move the selection...
             */
            return;
        }
        if (iNewISel != -1) {

            /*
             * Jump directly to the item provided by the app
             */
            goto TrackKeyDown;
        }

        /*
         * else do default processing of the character.
         */
    }

    switch (vKey) {
    // LATER IanJa: not language independent!!!
    // We could use VkKeyScan() to find out which is the '\' key
    // This is VK_OEM_5 '\|' for US English only.
    // Germans, Italians etc. have to type CTRL+^ (etc) for this.
    // This is documented as File Manager behaviour for 3.0, but apparently
    // not for 3.1., although functionality remains. We should still fix it,
    // although German (etc?) '\' is generated with AltGr (Ctrl-Alt) (???)
    case VERKEY_BACKSLASH:  /* '\' character for US English */

        /*
         * Check if this is CONTROL-\ ; If so Deselect all items
         */
        if ((wModifiers & CTLDOWN) && (plb->wMultiple != SINGLESEL)) {
            xxxLBSetCaret(plb, FALSE);
            xxxResetWorld(plb, plb->iSelBase, plb->iSelBase, FALSE);

            /*
             * And select the current item
             */
            SetSelected(plb, plb->iSelBase, TRUE, HILITEANDSEL);
            xxxInvertLBItem(plb, plb->iSelBase, TRUE);
            uEvent = EVENT_OBJECT_SELECTION;
            goto CaretOnAndNotify;
        }
        return;
        break;

    case VK_DIVIDE:     /* NumPad '/' character on enhanced keyboard */
    // LATER IanJa: not language independent!!!
    // We could use VkKeyScan() to find out which is the '/' key
    // This is VK_OEM_2 '/?' for US English only.
    // Germans, Italians etc. have to type CTRL+# (etc) for this.
    case VERKEY_SLASH:  /* '/' character */

        /*
         * Check if this is CONTROL-/ ; If so select all items
         */
        if ((wModifiers & CTLDOWN) && (plb->wMultiple != SINGLESEL)) {
            xxxLBSetCaret(plb, FALSE);
            xxxResetWorld(plb, -1, -1, TRUE);

            uEvent = EVENT_OBJECT_SELECTIONWITHIN;

CaretOnAndNotify:
            xxxLBSetCaret(plb, TRUE);
            if (FWINABLE()) {
                LBEvent(plb, uEvent, plb->iSelBase);
            }
            xxxNotifyOwner(plb, LBN_SELCHANGE);
        }
        return;
        break;

    case VK_F8:

        /*
         * The "Add" mode is possible only in Multiselection listboxes...  Get
         * into it via SHIFT-F8...  (Yes, sometimes these UI people are sillier
         * than your "typical dumb user"...)
         */
        if (plb->wMultiple != SINGLESEL && wModifiers == SHIFTDOWN) {

            /*
             * We have to make the caret blink! Do something...
             */
            if (plb->fAddSelMode) {

                /*
                 * Switch off the Caret blinking
                 */
                NtUserKillTimer(hwnd, IDSYS_CARET);

                /*
                 * Make sure the caret does not vanish
                 */
                xxxLBSetCaret(plb, TRUE);
            } else {

                /*
                 * Create a timer to make the caret blink
                 */
                NtUserSetTimer(hwnd, IDSYS_CARET, gpsi->dtCaretBlink,
                        xxxLBoxCaretBlinker);
            }

            /*
             * Toggle the Add mode flag
             */
            plb->fAddSelMode = (UINT)!plb->fAddSelMode;
        }
        return;
    case VK_SPACE:  /* Selection key is space */
        i = 0;
        fSelectKey = TRUE;
        break;

    case VK_PRIOR:
        if (fExtendedUIComboBoxClosed) {

            /*
             * Disable movement keys for TandyT.
             */
            return;
        }

        if (plb->OwnerDraw == OWNERDRAWVAR) {
            i = LBPage(plb, plb->iSelBase, FALSE) - plb->iSelBase;
        } else {
            i = -cItemPageScroll;
        }
        break;

    case VK_NEXT:
        if (fExtendedUIComboBoxClosed) {

            /*
             * Disable movement keys for TandyT.
             */
            return;
        }

        if (plb->OwnerDraw == OWNERDRAWVAR) {
            i = LBPage(plb, plb->iSelBase, TRUE) - plb->iSelBase;
        } else {
            i = cItemPageScroll;
        }
        break;

    case VK_HOME:
        if (fExtendedUIComboBoxClosed) {

            /*
             * Disable movement keys for TandyT.
             */
            return;
        }

        i = (INT_MIN/2)+1;  /* A very big negative number */
        break;

    case VK_END:
        if (fExtendedUIComboBoxClosed) {

            /*
             * Disable movement keys for TandyT.
             */
            return;
        }

        i = (INT_MAX/2)-1;  /* A very big positive number */
        break;

    case VK_LEFT:
        if (plb->fMultiColumn) {
            if (plb->fRightAlign
#ifdef USE_MIRRORING
                                 ^ (!!TestWF(plb->spwnd, WEFLAYOUTRTL))

#endif
               )
                goto ReallyRight;
ReallyLeft:
            if (plb->iSelBase / plb->itemsPerColumn == 0) {
                i = 0;
            } else {
                i = -plb->itemsPerColumn;
            }
            break;
        }

        if (hScrollBar) {
            goto HandleHScrolling;
        } else {

            /*
             * Fall through and handle this as if the up arrow was pressed.
             */

            vKey = VK_UP;
        }

        /*
         * Fall through
         */

    case VK_UP:
        if (fExtendedUIComboBoxClosed)
            // Disable movement keys for TandyT.
            return;

        i = -1;
        break;

    case VK_RIGHT:
        if (plb->fMultiColumn) {
            if (plb->fRightAlign
#ifdef USE_MIRRORING
                                 ^ (!!TestWF(plb->spwnd, WEFLAYOUTRTL))

#endif
               )
                goto ReallyLeft;
ReallyRight:
            if (plb->iSelBase / plb->itemsPerColumn == plb->cMac / plb->itemsPerColumn) {
                i = 0;
            } else {
                i = plb->itemsPerColumn;
            }
            break;
        }
        if (hScrollBar) {
HandleHScrolling:
            PostMessage(hwnd, WM_HSCROLL,
                    (vKey == VK_RIGHT ? SB_LINEDOWN : SB_LINEUP), 0L);
            return;
        } else {

            /*
             * Fall through and handle this as if the down arrow was
             * pressed.
             */
            vKey = VK_DOWN;
        }

        /*
         * Fall through
         */

    case VK_DOWN:
        if (fExtendedUIComboBoxClosed) {

            /*
             * If the combo box is closed, down arrow should open it.
             */
            if (!pcbox->fLBoxVisible) {

                /*
                 * If the listbox isn't visible, just show it
                 */
                ThreadLock(pcbox->spwnd, &tlpwnd);
                xxxCBShowListBoxWindow(pcbox, TRUE);
                ThreadUnlock(&tlpwnd);
            }
            return;
        }
        i = 1;
        break;

    case VK_ESCAPE:
    case VK_RETURN:
        if (!fDropDownComboBox || !pcbox->fLBoxVisible)
            return;

        // |  If this is a dropped listbox for a combobox and the ENTER  |
        // |  key is pressed, close up the listbox, so FALLTHRU          |
        // V                                                             V

    case VK_F4:
        if (fDropDownComboBox && !pcbox->fExtendedUI) {

            /*
             * If we are a dropdown combo box/listbox we want to process
             * this key.  BUT for TandtT, we don't do anything on VK_F4 if we
             * are in extended ui mode.
             */
            ThreadLock(pcbox->spwnd, &tlpwnd);
            if (!pcbox->fLBoxVisible) {

                /*
                 * If the listbox isn't visible, just show it
                 */
                xxxCBShowListBoxWindow(pcbox, (vKey != VK_ESCAPE));
            } else {

                /*
                 * Ok, the listbox is visible.  So hide the listbox window.
                 */
                xxxCBHideListBoxWindow(pcbox, TRUE, (vKey != VK_ESCAPE));
            }
            ThreadUnlock(&tlpwnd);
        }

        /*
         * Fall through to the return
         */

    default:
        return;
    }

    /*
     * Find out what the new selection should be
     */
    iNewISel = IncrementISel(plb, plb->iSelBase, i);


    if (plb->wMultiple == SINGLESEL) {
        if (plb->iSel == iNewISel) {

            /*
             * If we are single selection and the keystroke is moving us to an
             * item which is already selected, we don't have to do anything...
             */
            return;
        }

        uEvent = EVENT_OBJECT_SELECTION;

        plb->iTypeSearch = 0;
        if ((vKey == VK_UP || vKey == VK_DOWN) &&
                !IsSelected(plb, plb->iSelBase, HILITEONLY)) {

            /*
             * If the caret is on an unselected item and the user just hits the
             * up or down arrow key (ie. with no shift or ctrl modifications),
             * then we will just select the item the cursor is at. This is
             * needed for proper behavior in combo boxes but do we always want
             * to run this code??? Note that this is only used in single
             * selection list boxes since it doesn't make sense in the
             * multiselection case. Note that an LB_KEYDOWN message must not be
             * checked here because the vKey will be an item number not a
             * VK_and we will goof. Thus, trackkeydown label is below this to
             * fix a bug caused by it being above this...
             */
            iNewISel = (plb->iSelBase == -1) ? 0 : plb->iSelBase;
        }
    }

TrackKeyDown:

    xxxSetISelBase(plb, iNewISel);

    xxxLBSetCaret(plb, FALSE);

    if (wModifiers & SHIFTDOWN) {
        // Check if iMouseDown is un-initialised
        if (plb->iMouseDown == -1)
            plb->iMouseDown = iNewISel;
        if (plb->iLastMouseMove == -1)
            plb->iLastMouseMove = iNewISel;

        // Check if we are in ADD mode
        if (plb->fAddSelMode) {
            /* Preserve all the pre-existing selections except the
             * ones connected with the last anchor point; If the last
             * Preserve all the previous selections
            */
            /* Deselect only the selection connected with the last
             * anchor point; If the last anchor point is associated
             * with de-selection, then do not do it
            */

            if (!plb->fNewItemState)
                plb->iLastMouseMove = plb->iMouseDown;

            /* We haven't done anything here because, LBBlockHilite()
             * will take care of wiping out the selection between
             * Anchor point and iLastMouseMove and select the block
             * between anchor point and current cursor location
            */
        } else {
            /* We are not in ADD mode */
            /* Remove all selections except between the anchor point
             * and last mouse move because it will be taken care of in
             * LBBlockHilite
            */
            xxxResetWorld(plb, plb->iMouseDown, plb->iLastMouseMove, FALSE);
        }

        uEvent = EVENT_OBJECT_SELECTIONWITHIN;

        /* LBBlockHilite takes care to deselect the block between
         * the anchor point and iLastMouseMove and select the block
         * between the anchor point and the current cursor location
        */
        /* Toggle all items to the same selection state as the item
         * item at the anchor point) from the anchor point to the
         * current cursor location.
        */
        plb->fNewItemState = IsSelected(plb, plb->iMouseDown, SELONLY);
        xxxLBBlockHilite(plb, iNewISel, TRUE);

        plb->iLastMouseMove = iNewISel;
        /* Preserve the existing anchor point */
    } else {
        /* Check if this is in ADD mode */
        if ((plb->fAddSelMode) || (plb->wMultiple == MULTIPLESEL)) {
            /* Preserve all pre-exisiting selections */
            if (fSelectKey) {
                /* Toggle the selection state of the current item */
                plb->fNewItemState = !IsSelected(plb, iNewISel, SELONLY);
                SetSelected(plb, iNewISel, plb->fNewItemState, HILITEANDSEL);

                xxxInvertLBItem(plb, iNewISel, plb->fNewItemState);

                /* Set the anchor point at the current location */
                plb->iLastMouseMove = plb->iMouseDown = iNewISel;
                uEvent = (plb->fNewItemState ? EVENT_OBJECT_SELECTIONADD :
                        EVENT_OBJECT_SELECTIONREMOVE);
            }
        } else {
            /* We are NOT in ADD mode */
            /* Remove all existing selections except iNewISel, to
             * avoid flickering.
            */
            xxxResetWorld(plb, iNewISel, iNewISel, FALSE);

            /* Select the current item */
            SetSelected(plb, iNewISel, TRUE, HILITEANDSEL);
            xxxInvertLBItem(plb, iNewISel, TRUE);

            /* Set the anchor point at the current location */
            plb->iLastMouseMove = plb->iMouseDown = iNewISel;
            uEvent = EVENT_OBJECT_SELECTION;
        }
    }

    /*
     * Move the cursor to the new location
     */
    xxxInsureVisible(plb, iNewISel, FALSE);
    xxxLBShowHideScrollBars(plb);

    xxxLBSetCaret(plb, TRUE);

    if (FWINABLE() && uEvent) {
        LBEvent(plb, uEvent, iNewISel);
    }

    /*
     * Should we notify our parent?
     */
    if (plb->fNotify) {
        if (fDropDownComboBox && pcbox->fLBoxVisible) {

            /*
             * If we are in a drop down combo box/listbox and the listbox is
             * visible, we need to set the fKeyboardSelInListBox bit so that the
             * combo box code knows not to hide the listbox since the selchange
             * message is caused by the user keyboarding through...
             */
            pcbox->fKeyboardSelInListBox = TRUE;
            plb->iLastSelection = iNewISel;
        }
        xxxNotifyOwner(plb, LBN_SELCHANGE);
    }
}


/***************************************************************************\
* Compare
*
* Is lpstr1 equal/prefix/less-than/greater-than lsprst2 (case-insensitive) ?
*
* LATER IanJa: this assume a longer string is never a prefix of a longer one.
* Also assumes that removing 1 or more characters from the end of a string will
* give a string tahs sort before the original.  These assumptions are not valid
* for all languages.  We nedd better support from NLS. (Consider French
* accents, Spanish c/ch, ligatures, German sharp-s/SS, etc.)
*
* History:
\***************************************************************************/

INT Compare(
    LPCWSTR pwsz1,
    LPCWSTR pwsz2,
    DWORD dwLocaleId)
{
    UINT len1 = wcslen(pwsz1);
    UINT len2 = wcslen(pwsz2);
    INT result;

    /*
     * CompareStringW returns:
     *    1 = pwsz1  <  pwsz2
     *    2 = pwsz1  == pwsz2
     *    3 = pwsz1  >  pwsz2
     */
    result = CompareStringW((LCID)dwLocaleId, NORM_IGNORECASE,
            pwsz1, min(len1,len2), pwsz2, min(len1, len2));

    if (result == CSTR_LESS_THAN) {
       return LT;
    } else if (result == CSTR_EQUAL) {
        if (len1 == len2) {
            return EQ;
        } else if (len1 < len2) {
            /*
             * LATER IanJa: should not assume shorter string is a prefix
             * Spanish "c" and "ch", ligatures, German sharp-s/SS etc.
             */
            return PREFIX;
        }
    }
    return GT;
}

/***************************************************************************\
* xxxFindString
*
* Scans for a string in the listbox prefixed by or equal to lpstr.
* For OWNERDRAW listboxes without strings and without the sort style, we
* try to match the long app supplied values.
*
* History:
* 16-Apr-1992 beng      The NODATA case
\***************************************************************************/

INT xxxFindString(
    PLBIV plb,
    LPWSTR lpstr,
    INT sStart,
    INT code,
    BOOL fWrap)
{
    /*
     * Search for a prefix match (case-insensitive equal/prefix)
     * sStart == -1 means start from beginning, else start looking at sStart+1
     * assumes cMac > 0.
     */
    INT sInd;  /* index of string */
    INT sStop;          /* index to stop searching at */
    lpLBItem pRg;
    TL tlpwndParent;
    INT sortResult;

/*
 * Owner-Draw version of pRg
 */
#define pODRg ((lpLBODItem)pRg)
    COMPAREITEMSTRUCT cis;
    LPWSTR listboxString;

    CheckLock(plb->spwnd);

    if (plb->fHasStrings && (!lpstr || !*lpstr))
        return LB_ERR;

    if (!plb->fHasData) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "FindString called on NODATA lb");
        return LB_ERR;
    }

    if ((sInd = sStart + 1) >= plb->cMac)
        sInd = (fWrap ? 0 : plb->cMac - 1);

    sStop = (fWrap ? sInd : 0);

    /*
     * If at end and no wrap, stop right away
     */
    if (((sStart >= plb->cMac - 1) && !fWrap) || (plb->cMac < 1)) {
        return LB_ERR;
    }

    /*
     * Apps could pass in an invalid sStart like -2 and we would blow up.
     * Win 3.1 would not so we need to fixup sInd to be zero
     */
    if (sInd < 0)
        sInd = 0;

    pRg = (lpLBItem)(plb->rgpch);

    do {
        if (plb->fHasStrings) {

            /*
             * Searching for string matches.
             */
            listboxString = (LPWSTR)((LPBYTE)plb->hStrings + pRg[sInd].offsz);

            if (code == PREFIX &&
                listboxString &&
                *lpstr != TEXT('[') &&
                *listboxString == TEXT('[')) {

                /*
                 * If we are looking for a prefix string and the first items
                 * in this string are [- then we ignore them.  This is so
                 * that in a directory listbox, the user can goto drives
                 * by selecting the drive letter.
                 */
                listboxString++;
                if (*listboxString == TEXT('-'))
                    listboxString++;
            }

            if (Compare(lpstr, listboxString, plb->dwLocaleId) <= code) {
               goto FoundIt;
            }

        } else {
            if (plb->fSort) {

                /*
                 * Send compare item messages to the parent for sorting
                 */
                cis.CtlType = ODT_LISTBOX;
                cis.CtlID = PtrToUlong(plb->spwnd->spmenu);
                cis.hwndItem = HWq(plb->spwnd);
                cis.itemID1 = (UINT)-1;
                cis.itemData1 = (ULONG_PTR)lpstr;
                cis.itemID2 = (UINT)sInd;
                cis.itemData2 = pODRg[sInd].itemData;
                cis.dwLocaleId = plb->dwLocaleId;

                ThreadLock(plb->spwndParent, &tlpwndParent);
                sortResult = (INT)SendMessage(HW(plb->spwndParent), WM_COMPAREITEM,
                        cis.CtlID, (LPARAM)&cis);
                ThreadUnlock(&tlpwndParent);


                if (sortResult == -1) {
                   sortResult = LT;
                } else if (sortResult == 1) {
                   sortResult = GT;
                } else {
                   sortResult = EQ;
                }

                if (sortResult <= code) {
                    goto FoundIt;
                }
            } else {

                /*
                 * Searching for app supplied long data matches.
                 */
                if ((ULONG_PTR)lpstr == pODRg[sInd].itemData)
                    goto FoundIt;
            }
        }

        /*
         * Wrap round to beginning of list
         */
        if (++sInd == plb->cMac)
            sInd = 0;
    } while (sInd != sStop);

    sInd = -1;

FoundIt:
    return sInd;
}


/***************************************************************************\
* xxxLBoxCtlCharInput
*
* History:
\***************************************************************************/

void xxxLBoxCtlCharInput(
    PLBIV plb,
    UINT  inputChar,
    BOOL  fAnsi)
{
    INT iSel;
    BOOL fControl;
    TL tlpwndParent;

    CheckLock(plb->spwnd);

    if (plb->cMac == 0 || plb->fMouseDown) {

        /*
         * Get out if we are in the middle of mouse routines or if we have no
         * items in the listbox, we just return without doing anything.
         */
        return;
    }

    fControl = (GetKeyState(VK_CONTROL) < 0);

    switch (inputChar) {
    case VK_ESCAPE:
        plb->iTypeSearch = 0;
        if (plb->pszTypeSearch)
            plb->pszTypeSearch[0] = 0;
        break;

    case VK_BACK:
        if (plb->iTypeSearch) {
            plb->pszTypeSearch[plb->iTypeSearch--] = 0;
            if (plb->fSort) {
                iSel = -1;
                goto TypeSearch;
            }
        }
        break;

    case VK_SPACE:
        if (plb->fAddSelMode || plb->wMultiple == MULTIPLESEL)
            break;
        /* Otherwise, for single/extended selection listboxes not in add
         * selection mode, let the  space go thru as a type search character
         * FALL THRU
         */

    default:

        /*
         * Move selection to first item beginning with the character the
         * user typed.  We don't want do this if we are using owner draw.
         */

        if (fAnsi && IS_DBCS_ENABLED() && IsDBCSLeadByteEx(THREAD_CODEPAGE(), (BYTE)inputChar)) {
            WCHAR wch;
            LPWSTR lpwstr = &wch;

            inputChar = DbcsCombine(HWq(plb->spwnd), (BYTE)inputChar);
            RIPMSG1(RIP_VERBOSE, "xxxLBoxCtlCharInput: combined DBCS. 0x%04x", inputChar);

            if (inputChar == 0) {
                RIPMSG1(RIP_WARNING, "xxxLBoxCtlCharInput: cannot combine two DBCS. LB=0x%02x",
                        inputChar);
                break;
            }
            // If it is DBCS, let's ignore the ctrl status.
            fControl = FALSE;

            // Convert DBCS to UNICODE.
            // Note: Leading byte is in the low byte, trailing byte is in high byte.
            // Let's assume Little Endian CPUs only, so inputChar can directly be
            // input for MBSToWCSEx as an ANSI string.
            if (MBToWCSEx(THREAD_CODEPAGE(), (LPCSTR)&inputChar, 2, &lpwstr, 1, FALSE) == 0) {
                RIPMSG1(RIP_WARNING, "xxxLBoxCtlCharInput: cannot convert 0x%04x to UNICODE.",
                        inputChar);
                break;
            }
            inputChar = wch;
        }

        if (plb->fHasStrings) {
            // Incremental Type Search processing
            //
            // update szTypeSearch string and then move to the first item from
            // the current selection whose prefix matches szTypeSearch
            //
            // the szTypeSearch will continue to grow until a "long enough"
            // gap between key entries is encountered -- at which point any
            // more searching will start over

            /*
             * Undo CONTROL-char to char
             */
            if (fControl && inputChar < 0x20)
                inputChar += 0x40;

            if (plb->iTypeSearch == MAX_TYPESEARCH) {
                NtUserMessageBeep(0);
                break;
            }
            iSel = -1;

            if (plb->pszTypeSearch == NULL)
                plb->pszTypeSearch = (LPWSTR)UserLocalAlloc(HEAP_ZERO_MEMORY, sizeof(WCHAR) * (MAX_TYPESEARCH + 1));

            if (plb->pszTypeSearch == NULL) {
                NtUserMessageBeep(0);
                break;
            }

            plb->pszTypeSearch[plb->iTypeSearch++] = (WCHAR) inputChar;
            plb->pszTypeSearch[plb->iTypeSearch]   = 0;

TypeSearch:
            if (plb->fSort) {
                // Set timer to determine when to kill incremental searching
                NtUserSetTimer(HWq(plb->spwnd), IDSYS_LBSEARCH,
                               gpsi->dtLBSearch, NULL);
            } else {
                // If this is not a sorted listbox, no incremental search.
                plb->iTypeSearch = 0;
                iSel = plb->iSelBase;
            }


            /*
             * Search for the item beginning with the given character starting
             * at iSel+1.  We will wrap the search to the beginning of the
             * listbox if we don't find the item.   If SHIFT is down and we are
             * a multiselection lb, then the item's state will be set to
             * plb->fNewItemState according to the current mode.
             */
            iSel = xxxFindString(plb, plb->pszTypeSearch, iSel, PREFIX, TRUE);
            if (iSel == -1) {
                // no match found -- check for prefix match
                // (i.e. "p" find FIRST item that starts with 'p',
                //       "pp" find NEXT item that starts with 'p')
                if(plb->iTypeSearch)
                {
                    plb->iTypeSearch--;
                    if ((plb->iTypeSearch == 1) && (plb->pszTypeSearch[0] == plb->pszTypeSearch[1]))
                    {
                        plb->pszTypeSearch[1] = 0;
                        iSel = xxxFindString(plb, plb->pszTypeSearch, plb->iSelBase, PREFIX, TRUE);
                    }
                }
            }
            // if match is found -- select it
            if (iSel != -1)
            {
CtlKeyInput:
                xxxLBoxCtlKeyInput(plb, LB_KEYDOWN, iSel);

            }
        } else {
            if (plb->spwndParent != NULL) {
                ThreadLock(plb->spwndParent, &tlpwndParent);
                iSel = (INT)SendMessageWorker(plb->spwndParent, WM_CHARTOITEM,
                        MAKELONG(inputChar, plb->iSelBase), (LPARAM)HWq(plb->spwnd), fAnsi);
                ThreadUnlock(&tlpwndParent);
            } else
                iSel = -1;

            if (iSel != -1 && iSel != -2)
                goto CtlKeyInput;

        }
        break;
    }
}


/***************************************************************************\
* LBoxGetSelItems
*
* effects: For multiselection listboxes, this returns the total number of
* selection items in the listbox if fCountOnly is true.  or it fills an array
* (lParam) with the items numbers of the first wParam selected items.
*
* History:
\***************************************************************************/

int LBoxGetSelItems(
    PLBIV plb,
    BOOL fCountOnly,
    int wParam,
    LPINT lParam)
{
    int i;
    int itemsselected = 0;

    if (plb->wMultiple == SINGLESEL)
        return LB_ERR;

    for (i = 0; i < plb->cMac; i++) {
        if (IsSelected(plb, i, SELONLY)) {
            if (!fCountOnly) {
                if (itemsselected < wParam)
                    *lParam++ = i;
                else {

                    /*
                     * That's all the items we can fit in the array.
                     */
                    return itemsselected;
                }
            }
            itemsselected++;
        }
    }

    return itemsselected;
}


/***************************************************************************\
* xxxLBSetRedraw
*
* Handle WM_SETREDRAW message
*
* History:
\***************************************************************************/

void xxxLBSetRedraw(
    PLBIV plb,
    BOOL fRedraw)
{
    CheckLock(plb->spwnd);

    if (fRedraw)
        fRedraw = TRUE;

    if (plb->fRedraw != (UINT)fRedraw) {
        plb->fRedraw = !!fRedraw;

        if (fRedraw) {
            xxxLBSetCaret(plb, TRUE);
            xxxLBShowHideScrollBars(plb);

            if (plb->fDeferUpdate) {
                plb->fDeferUpdate = FALSE;
                RedrawWindow(HWq(plb->spwnd), NULL, NULL,
                        RDW_INVALIDATE | RDW_ERASE |
                        RDW_FRAME | RDW_ALLCHILDREN);
            }
        }
    }
}

/***************************************************************************\
* xxxLBSelRange
*
* Selects the range of items between i and j, inclusive.
*
* History:
\***************************************************************************/

void xxxLBSelRange(
    PLBIV plb,
    int iStart,
    int iEnd,
    BOOL fnewstate)
{
    DWORD temp;
    RECT rc;

    CheckLock(plb->spwnd);

    if (iStart > iEnd) {
        temp = iEnd;
        iEnd = iStart;
        iStart = temp;
    }

    /*
     * We don't want to loop through items that don't exist.
     */
    iEnd = min(plb->cMac, iEnd);
    iStart = max(iStart, 0);
    if (iStart > iEnd)
        return;


    /*
     * iEnd could be equal to MAXINT which is why we test temp and iEnd
     * as DWORDs.
     */
    for (temp = iStart; temp <= (DWORD)iEnd; temp++) {

        if (IsSelected(plb, temp, SELONLY) != fnewstate) {
            SetSelected(plb, temp, fnewstate, HILITEANDSEL);
            LBGetItemRect(plb, temp, &rc);

            xxxLBInvalidateRect(plb, (LPRECT)&rc, FALSE);
        }

    }
    UserAssert(plb->wMultiple);
    if (FWINABLE()) {
        LBEvent(plb, EVENT_OBJECT_SELECTIONWITHIN, iStart);
    }
}


/***************************************************************************\
* xxxLBSetCurSel
*
* History:
\***************************************************************************/

int xxxLBSetCurSel(
    PLBIV plb,
    int iSel)
{
    CheckLock(plb->spwnd);

    if (!(plb->wMultiple || iSel < -1 || iSel >= plb->cMac)) {
        xxxLBSetCaret(plb, FALSE);
        if (plb->iSel != -1) {

            /*
             * This prevents scrolling when iSel == -1
             */
            if (iSel != -1)
                xxxInsureVisible(plb, iSel, FALSE);

            /*
             * Turn off old selection
             */
            xxxInvertLBItem(plb, plb->iSel, FALSE);
        }

        if (iSel != -1) {
            xxxInsureVisible(plb, iSel, FALSE);
            plb->iSelBase = plb->iSel = iSel;

            /*
             * Highlight new selection
             */
            xxxInvertLBItem(plb, plb->iSel, TRUE);
        } else {
            plb->iSel = -1;
            if (plb->cMac)
                plb->iSelBase = min(plb->iSelBase, plb->cMac-1);
            else
                plb->iSelBase = 0;
        }

        if (FWINABLE()) {
            /*
             * Send both focus and selection events
             */
            if (_IsWindowVisible(plb->spwnd)) {
                LBEvent(plb, EVENT_OBJECT_FOCUS, plb->iSelBase);
                LBEvent(plb, EVENT_OBJECT_SELECTION, plb->iSel);
            }
        }

        xxxLBSetCaret(plb, TRUE);
        return plb->iSel;
    }

    return LB_ERR;
}


/***************************************************************************\
* LBSetItemData
*
* Makes the item at index contain the data given.
*
* History:
* 16-Apr-1992 beng      The NODATA listbox case
\***************************************************************************/

int LBSetItemData(
    PLBIV plb,
    int index,
    LONG_PTR data)
{
    LPSTR lpItemText;

    /*
     * v-ronaar: fix bug #25865, don't allow negative indices!
     */
    if ((index != -1) && ((UINT) index >= (UINT) plb->cMac)) {
        RIPERR1(ERROR_INVALID_INDEX, RIP_WARNING, "LBSetItemData with invalid index %x", index);
        return LB_ERR;
    }

    /*
     * No-data listboxes just ignore all LB_SETITEMDATA calls
     */
    if (!plb->fHasData) {
        return TRUE;
    }

    lpItemText = (LPSTR)plb->rgpch;

    if (index == -1) {

        /*
         * index == -1 means set the data to all the items
         */
        if (plb->fHasStrings) {
            for (index = 0; index < plb->cMac; index++) {

                ((lpLBItem)lpItemText)->itemData = data;
                lpItemText += sizeof(LBItem);
            }
        } else {
            for (index = 0; index < plb->cMac; index++) {

                ((lpLBODItem)lpItemText)->itemData = data;
                lpItemText += sizeof(LBODItem);
            }
        }
        return TRUE;
    }

    if (plb->fHasStrings) {

        lpItemText = (LPSTR)(lpItemText + (index * sizeof(LBItem)));
        ((lpLBItem)lpItemText)->itemData = data;
    } else {

        lpItemText = (LPSTR)(lpItemText + (index * sizeof(LBODItem)));
        ((lpLBODItem)lpItemText)->itemData = data;
    }
    return TRUE;
}

/***************************************************************************\
* xxxCheckRedraw
*
* History:
\***************************************************************************/

void xxxCheckRedraw(
    PLBIV plb,
    BOOL fConditional,
    INT sItem)
{
    CheckLock(plb->spwnd);

    if (fConditional && plb->cMac &&
            (sItem > (plb->iTop + CItemInWindow(plb, TRUE))))
        return;

    /*
     * Don't do anything if the parent is not visible.
     */
    xxxLBInvalidateRect(plb, (LPRECT)NULL, TRUE);
}


/***************************************************************************\
* xxxCaretDestroy
*
* History:
\***************************************************************************/

void xxxCaretDestroy(
    PLBIV plb)
{
    CheckLock(plb->spwnd);

    /*
     * We're losing the focus.  Act like up clicks are happening so we release
     * capture, set the current selection, notify the parent, etc.
     */
    if (plb->fCaptured)

        /*
         * If we have the capture and we lost the focus, that means we already
         * changed the selection and we have to notify also the parent about
         * this. So we need to add also the LBUP_SUCCESS flag in this case.
         */

        xxxLBButtonUp(plb, LBUP_RELEASECAPTURE | LBUP_NOTIFY |
            (plb->fMouseDown ? LBUP_SUCCESS : 0));

    if (plb->fAddSelMode) {

        /*
         * Switch off the Caret blinking
         */
        NtUserKillTimer(HWq(plb->spwnd), IDSYS_CARET);

        /*
         * Make sure the caret goes away
         */
        xxxLBSetCaret(plb, FALSE);
        plb->fAddSelMode = FALSE;
    }

    plb->fCaret = FALSE;
}


/***************************************************************************\
* xxxLbSetSel
*
* History:
\***************************************************************************/

LONG xxxLBSetSel(
    PLBIV plb,
    BOOL fSelect,  /* New state to set selection to */
    INT iSel)
{
    INT sItem;
    RECT rc;
    UINT uEvent = 0;

    CheckLock(plb->spwnd);

    /*
    * Bug 17656. WinZip's accelerator key for 'DeSelect All' sends a LB_SETSEL
    * message with lparam = 0x0000ffff instead of 0xffffffff(-1). If iSel
    * is equal to  0x0000ffff and there are less than 0xffff elements in the
    * list we set iSel equal to 0xffffffff.
    */
    if ((iSel == (UINT)0xffff) && (iSel >= plb->cMac)) {
        iSel = -1;
        RIPMSG0(RIP_WARNING, "Sign extending iSel=0xffff to 0xffffffff");
    }


    if ((plb->wMultiple == SINGLESEL) || (iSel != -1 && iSel >= plb->cMac)) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING,
                "xxxLBSetSel:Invalid iSel or SINGLESEL listbox");
        return LB_ERR;
    }

    xxxLBSetCaret(plb, FALSE);

    if (iSel == -1/*(INT)0xffff*/) {

        /*
         * Set/clear selection from all items if -1
         */
        for (sItem = 0; sItem < plb->cMac; sItem++) {
            if (IsSelected(plb, sItem, SELONLY) != fSelect) {
                SetSelected(plb, sItem, fSelect, HILITEANDSEL);
                if (LBGetItemRect(plb, sItem, &rc)) {
                    xxxLBInvalidateRect(plb, &rc, FALSE);
                }
            }
        }
        xxxLBSetCaret(plb, TRUE);
        uEvent = EVENT_OBJECT_SELECTIONWITHIN;
    } else {
        if (fSelect) {

            /*
             * Check if the item if fully hidden and scroll it into view if it
             * is.  Note that we don't want to scroll partially visible items
             * into full view because this breaks the shell...
             */
            xxxInsureVisible(plb, iSel, TRUE);
            plb->iSelBase = plb->iSel = iSel;

            plb->iMouseDown = plb->iLastMouseMove = iSel;
            uEvent = EVENT_OBJECT_FOCUS;
        } else {
            uEvent = EVENT_OBJECT_SELECTIONREMOVE;
        }
        SetSelected(plb, iSel, fSelect, HILITEANDSEL);

        /*
         * Note that we set the caret on bit directly so that we avoid flicker
         * when drawing this item.  ie.  We turn on the caret, redraw the item and
         * turn it back on again.
         */
        if (!fSelect && plb->iSelBase != iSel) {
            xxxLBSetCaret(plb, TRUE);
        } else if (plb->fCaret) {
            plb->fCaretOn = TRUE;
        }

        if (LBGetItemRect(plb, iSel, &rc)) {
            xxxLBInvalidateRect(plb, &rc, FALSE);
        }
    }

    if (FWINABLE() && _IsWindowVisible(plb->spwnd)) {
        if (uEvent == EVENT_OBJECT_FOCUS) {
            LBEvent(plb, uEvent, plb->iSelBase);
            uEvent = EVENT_OBJECT_SELECTION;
        }
        LBEvent(plb, uEvent, iSel);
    }

    return 0;
}


/***************************************************************************\
* xxxLBoxDrawItem
*
* This fills the draw item struct with some constant data for the given
* item.  The caller will only have to modify a small part of this data
* for specific needs.
*
* History:
* 16-Apr-1992 beng      The NODATA case
\***************************************************************************/

void xxxLBoxDrawItem(
    PLBIV plb,
    INT item,
    UINT itemAction,
    UINT itemState,
    LPRECT lprect)
{
    DRAWITEMSTRUCT dis;
    TL tlpwndParent;

    CheckLock(plb->spwnd);

    /*
     * Fill the DRAWITEMSTRUCT with the unchanging constants
     */

    dis.CtlType = ODT_LISTBOX;
    dis.CtlID = PtrToUlong(plb->spwnd->spmenu);

    /*
     * Use -1 if an invalid item number is being used.  This is so that the app
     * can detect if it should draw the caret (which indicates the lb has the
     * focus) in an empty listbox
     */
    dis.itemID = (UINT)(item < plb->cMac ? item : -1);
    dis.itemAction = itemAction;
    dis.hwndItem = HWq(plb->spwnd);
    dis.hDC = plb->hdc;
    dis.itemState = itemState |
            (UINT)(TestWF(plb->spwnd, WFDISABLED) ? ODS_DISABLED : 0);

    if (TestWF(plb->spwnd, WEFPUIFOCUSHIDDEN)) {
        dis.itemState |= ODS_NOFOCUSRECT;
    }
    if (TestWF(plb->spwnd, WEFPUIACCELHIDDEN)) {
        dis.itemState |= ODS_NOACCEL;
    }

    /*
     * Set the app supplied data
     */
    if (!plb->cMac || !plb->fHasData) {

        /*
         * If no strings or no items, just use 0 for data.  This is so that we
         * can display a caret when there are no items in the listbox.
         *
         * Lazy-eval listboxes of course have no data to pass - only itemID.
         */
        dis.itemData = 0L;
    } else {
        dis.itemData = LBGetItemData(plb, item);
    }

    CopyRect(&dis.rcItem, lprect);

    /*
     * Set the window origin to the horizontal scroll position.  This is so that
     * text can always be drawn at 0,0 and the view region will only start at
     * the horizontal scroll offset. We pass this as wParam
     */
    /*
     * Note:  Only pass the itemID in wParam for 3.1 or newer apps.  We break
     * ccMail otherwise.
     */

    ThreadLock(plb->spwndParent, &tlpwndParent);
    SendMessage(HW(plb->spwndParent), WM_DRAWITEM,
            TestWF(plb->spwndParent, WFWIN31COMPAT) ? dis.CtlID : 0,
            (LPARAM)&dis);
    ThreadUnlock(&tlpwndParent);
}


/***************************************************************************\
* xxxLBBlockHilite
*
*       In Extended selection mode for multiselection listboxes, when
*   mouse is draged to a new position, the range being marked should be
*   properly sized(parts of which will be highlighted/dehighlighted).
*   NOTE: This routine assumes that iSelFromPt and LasMouseMove are not
*          equal because only in that case this needs to be called;
*   NOTE: This routine calculates the region whose display attribute is to
*          be changed in an optimised way. Instead of de-highlighting the
*          the old range completely and highlight the new range, it omits
*          the regions that overlap and repaints only the non-pverlapping
*          area.
*   fKeyBoard = TRUE if this is called for Keyboard interface
*                FALSE if called from Mouse interface routines
*
* History:
\***************************************************************************/

void xxxLBBlockHilite(
    PLBIV plb,
    INT iSelFromPt,
    BOOL fKeyBoard)
{
    INT sCurPosOffset;
    INT sLastPosOffset;
    INT sHiliteOrSel;
    BOOL fUseSelStatus;
    BOOL DeHiliteStatus;

    CheckLock(plb->spwnd);

    if (fKeyBoard) {

        /*
         * Set both Hilite and Selection states
         */
        sHiliteOrSel = HILITEANDSEL;

        /*
         * Do not use the Selection state while de-hiliting
         */
        fUseSelStatus = FALSE;
        DeHiliteStatus = FALSE;
    } else {

        /*
         * Set/Reset only the Hilite state
         */
        sHiliteOrSel = HILITEONLY;

        /*
         * Use the selection state for de-hilighting
         */
        fUseSelStatus = TRUE;
        DeHiliteStatus = plb->fNewItemState;
    }



    /*
     * The idea of the routine is to :
     *          1.  De-hilite the old range (iMouseDown to iLastMouseDown)  and
     *          2.  Hilite the new range (iMouseDwon to iSelFromPt)
     */

    /*
     * Offset of current mouse position from the anchor point
     */
    sCurPosOffset = plb->iMouseDown - iSelFromPt;

    /*
     * Offset of last mouse position from the anchor point
     */
    sLastPosOffset = plb->iMouseDown - plb->iLastMouseMove;

    /*
     * Check if both current position and last position lie on the same
     * side of the anchor point.
     */
    if ((sCurPosOffset * sLastPosOffset) >= 0) {

        /*
         * Yes they are on the same side; So, highlight/dehighlight only
         * the difference.
         */
        if (abs(sCurPosOffset) > abs(sLastPosOffset)) {
            xxxAlterHilite(plb, plb->iLastMouseMove, iSelFromPt,
                    plb->fNewItemState, sHiliteOrSel, FALSE);
        } else {
            xxxAlterHilite(plb, iSelFromPt, plb->iLastMouseMove, DeHiliteStatus,
                    sHiliteOrSel, fUseSelStatus);
        }
    } else {
        xxxAlterHilite(plb, plb->iMouseDown, plb->iLastMouseMove,
                DeHiliteStatus, sHiliteOrSel, fUseSelStatus);
        xxxAlterHilite(plb, plb->iMouseDown, iSelFromPt,
                plb->fNewItemState, sHiliteOrSel, FALSE);
    }
}


/***************************************************************************\
* xxxAlterHilite
*
* Changes the hilite state of (i..j] (ie. excludes i, includes j in case
* you've forgotten this silly notation) to fHilite. It inverts this changes
* the hilite state.
*
*  OpFlags:  HILITEONLY      Only change the display state of the items
*            SELONLY         Only Change the selection state of the items
*            HILITEANDSELECT Do both.
*  fHilite:
*            HILITE/TRUE
*            DEHILITE/FALSE
*  fSelStatus:
*            if TRUE, use the selection state of the item to hilite/dehilite
*            if FALSE, use the fHilite parameter to hilite/dehilite
*
* History:
\***************************************************************************/

void xxxAlterHilite(
    PLBIV plb,
    INT i,
    INT j,
    BOOL fHilite,
    INT OpFlags,
    BOOL fSelStatus)
{
    INT low;
    INT high;
    INT sLastInWindow;
    BOOL fCaretOn;
    BOOL fSelected;

    CheckLock(plb->spwnd);

    sLastInWindow = plb->iTop + CItemInWindow(plb, TRUE);
    sLastInWindow = min(sLastInWindow, plb->cMac - 1);
    high = max(i, j) + 1;

    if (fCaretOn = plb->fCaretOn) {
        xxxLBSetCaret(plb, FALSE);
    }

    for (low = min(i, j); low < high; low++) {
        if (low != i) {
            if (OpFlags & HILITEONLY) {
                if (fSelStatus) {
                    fSelected = IsSelected(plb, low, SELONLY);
                } else {
                    fSelected = fHilite;
                }
                if (IsSelected(plb, low, HILITEONLY) != fSelected) {
                    if (plb->iTop <= low && low <= sLastInWindow) {

                        /*
                         * Invert the item only if it is visible
                         */
                        xxxInvertLBItem(plb, low, fSelected);
                    }
                    SetSelected(plb, low, fSelected, HILITEONLY);
                }
            }

            if (OpFlags & SELONLY) {
                SetSelected(plb, low, fHilite, SELONLY);
            }
        }
    }

    if (fCaretOn) {
        xxxLBSetCaret(plb, TRUE);
    }
}
