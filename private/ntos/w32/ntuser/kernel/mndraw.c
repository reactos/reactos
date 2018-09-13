/**************************** Module Header ********************************\
* Module Name: mndraw.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Menu Painting Routines
*
* History:
* 10-10-90 JimA       Cleanup.
* 03-18-91 IanJa      Window revalidation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


#define SRCSTENCIL          0x00B8074AL

#define MENU_STRLEN 255
/***************************************************************************\
* MNIsCachedBmpOnly
*
* 04/02/97 GerardoB Created
\***************************************************************************/
__inline BOOL MNIsCachedBmpOnly (PITEM pItem)
{
    return TestMFS(pItem, MFS_CACHEDBMP) && (pItem->lpstr == NULL);
}
/***************************************************************************\
* MNDrawHilite
*
* Don't draw the hilite if:
*   The insertion bar is on (MFS_GAPDROP)
*   or this is a cached bitmap (close, min, max, etc) with no text
*
*
* 08/12/96 GerardoB Ported From Memphis.
\***************************************************************************/
BOOL mnDrawHilite (PITEM pItem)
{
    return TestMFS(pItem, MFS_HILITE)
            && !TestMFS(pItem, MFS_GAPDROP)
            && !MNIsCachedBmpOnly(pItem);
}

/***************************************************************************\
* MNDrawMenu3DHotTracking
*
* 03/10/97  yutakas  created
* 04/07/97  vadimg   ported from Memphis
\***************************************************************************/
void MNDrawMenu3DHotTracking(HDC hdc, PMENU pMenu, PITEM pItem)
{
    HBRUSH hbrTopLeft, hbrBottomRight;
    BOOL fDrawEdge;

    if (pItem->hbmp && TestMFS(pItem, MFS_CACHEDBMP))
        return;

    fDrawEdge = FALSE;

    if (!TestMF(pMenu, MFISPOPUP)) {
        if (TestMFS(pItem, MFS_HILITE)) {
            hbrTopLeft = SYSHBR(3DSHADOW);
            hbrBottomRight = SYSHBR(3DHILIGHT);
            SetMFS(pItem, MFS_HOTTRACKDRAWN);
            fDrawEdge = TRUE;
        } else if (TestMFS(pItem, MFS_HOTTRACK)) {
            hbrTopLeft = SYSHBR(3DHILIGHT);
            hbrBottomRight = SYSHBR(3DSHADOW);
            SetMFS(pItem, MFS_HOTTRACKDRAWN);
            fDrawEdge = TRUE;
        } else if (TestMFS(pItem, MFS_HOTTRACKDRAWN)) {
            hbrTopLeft = SYSHBR(MENU);
            hbrBottomRight = SYSHBR(MENU);
            ClearMFS(pItem, MFS_HOTTRACKDRAWN);
            fDrawEdge = TRUE;
        }
    }

    if (fDrawEdge) {
        int x = pItem->xItem, y = pItem->yItem;
        int cx = pItem->cxItem, cy = pItem->cyItem;
        HBRUSH hbrOld = GreSelectBrush(hdc, hbrTopLeft);
        GrePatBlt(hdc, x, y, cx - CXMENU3DEDGE, CYMENU3DEDGE, PATCOPY);
        GrePatBlt(hdc, x, y, CXMENU3DEDGE, cy - CYMENU3DEDGE, PATCOPY);
        GreSelectBrush(hdc, hbrBottomRight);
        GrePatBlt(hdc, x, y + cy - CYMENU3DEDGE, cx - CXMENU3DEDGE, CYMENU3DEDGE, PATCOPY);
        GrePatBlt(hdc, x + cx - CYMENU3DEDGE, y, CXMENU3DEDGE, cy, PATCOPY);
        GreSelectBrush(hdc, hbrOld);
    }
}
/***************************************************************************\
* MNDrawArrow
*
*  redraws the specified arrow (uArrow) in a scrollable menu (ppopup) to reflect
*  it's current state of enabled or disabled, drawing it in HOTLIGHT if
*  fOn is TRUE
*
* 08/12/96 GerardoB Ported From Memphis.
\***************************************************************************/
void MNDrawArrow(HDC hdcIn, PPOPUPMENU ppopup, UINT uArrow)
{
    PWND    pwnd = ppopup->spwndPopupMenu;
    HDC     hdc;
    int     x, y;
    DWORD   dwBmp;
    DWORD   dwAtCheck;
    DWORD   dwState;

    if (ppopup->spmenu->dwArrowsOn == MSA_OFF) {
        return;
    }

    if (hdcIn == NULL) {
        hdc = _GetDCEx(pwnd, NULL, DCX_USESTYLE | DCX_WINDOW | DCX_LOCKWINDOWUPDATE);
    } else {
        hdc = hdcIn;
    }

    x = SYSMET(CXFIXEDFRAME);
    if (!TestMF(ppopup->spmenu, MNS_NOCHECK)) {
       /*
        * Win9x:  x += MNByteAlignItem(oemInfo.bm[OBI_MENUCHECK].cx);
        */
        x += gpsi->oembmi[OBI_MENUCHECK].cx;
    } else {
        x += SYSMET(CXEDGE) * 2;
    }

    if (uArrow == MFMWFP_UPARROW) {
        y = SYSMET(CXFIXEDFRAME);
        dwBmp = OBI_MENUARROWUP;
        dwAtCheck = MSA_ATTOP;
        dwState = DFCS_MENUARROWUP;
    } else {
        y = pwnd->rcWindow.bottom - pwnd->rcWindow.top - SYSMET(CYFIXEDFRAME) - gcyMenuScrollArrow;
        dwBmp = OBI_MENUARROWDOWN;
        dwAtCheck = MSA_ATBOTTOM;
        dwState = DFCS_MENUARROWDOWN;
    }

    if (ppopup->spmenu->dwArrowsOn == dwAtCheck) {
        /*
         * go 2 ahead to inactive state bitmap
         */
        dwBmp += 2;
        dwState |= DFCS_INACTIVE;
    }

    if (ppopup->spmenu->hbrBack != NULL) {
        /*
         * for menus with background brushes, we can't do a straight blt
         * of the scroll arrows 'cause the background wouldn't be right;
         * need to call DrawFrameControl with DFCS_TRANSPARENT instead
         */
        RECT rc;
        rc.top = y;
        rc.left = x;
        rc.right  = x + gpsi->oembmi[OBI_MENUARROWUP].cx;
        rc.bottom = y + gpsi->oembmi[OBI_MENUARROWUP].cy;
        DrawFrameControl(hdc, &rc, DFC_MENU, dwState | DFCS_TRANSPARENT);
    } else {
        BitBltSysBmp(hdc, x, y, dwBmp);
        BitBltSysBmp(hdc, x, y, dwBmp);
    }

    if (hdcIn == NULL) {
        _ReleaseDC(hdc);
    }
}
/***************************************************************************\
*  MNDrawFullNC
*
*  Performs the custom nonclient painting needed for scrollable menus.
*  Assumes that the given menu is a scrollable menu.
*
* History:
*  08-14-96 GerardoB - Ported from Memphis
\***************************************************************************/
void MNDrawFullNC(PWND pwnd, HDC hdcIn, PPOPUPMENU ppopup)
{
    RECT rc;
    HDC hdc;
    HBRUSH hbrOld;
    int yTop, yBottom;
    POINT ptOrg;
    if (hdcIn == NULL) {
        hdc = _GetDCEx(pwnd, NULL, DCX_USESTYLE | DCX_WINDOW | DCX_LOCKWINDOWUPDATE);
    } else {
        hdc = hdcIn;
    }

    rc.left = rc.top = 0;
    rc.right = pwnd->rcWindow.right - pwnd->rcWindow.left;
    rc.bottom = pwnd->rcWindow.bottom - pwnd->rcWindow.top;
    DrawEdge(hdc, &rc, EDGE_RAISED, (BF_RECT | BF_ADJUST));
    DrawFrame(hdc, &rc, 1, DF_3DFACE);
    InflateRect(&rc, -SYSMET(CXBORDER), -SYSMET(CYBORDER));

    yTop = rc.top;
    yBottom = rc.bottom - gcyMenuScrollArrow;

    GreGetBrushOrg(hdc, &ptOrg);
    if (ppopup->spmenu->hbrBack != NULL) {
        GreSetBrushOrg(hdc, 0,
                -(int)MNGetToppItem(ppopup->spmenu)->yItem, NULL);
        hbrOld = GreSelectBrush(hdc, ppopup->spmenu->hbrBack);
    } else {
        hbrOld = GreSelectBrush(hdc, SYSHBR(MENU));
    }

    rc.right -= rc.left;
    GrePatBlt(hdc, rc.left, yTop, rc.right, gcyMenuScrollArrow, PATCOPY);
    MNDrawArrow(hdc, ppopup, MFMWFP_UPARROW);
    GrePatBlt(hdc, rc.left, yBottom, rc.right, gcyMenuScrollArrow, PATCOPY);
    MNDrawArrow(hdc, ppopup, MFMWFP_DOWNARROW);

    GreSetBrushOrg(hdc, ptOrg.x, ptOrg.y, NULL);
    GreSelectBrush(hdc, hbrOld);

    if (hdcIn == NULL) {
        _ReleaseDC(hdc);
    }
}
/***************************************************************************\
* MNEraseBackground
*
* Erases the background making sure that the background pattern (i.e, watermark)
*  aligns with the pattern in the nonclient area.
*
* History:
*  08-23-96 GerardoB - Created
\***************************************************************************/
void MNEraseBackground (HDC hdc, PMENU pmenu, int x, int y, int cx, int cy)
{
    BOOL fSetOrg;
    HBRUSH hbrOld;
    POINT ptOrg;

    UserAssert(pmenu->hbrBack != NULL);

    fSetOrg = TRUE;
    GreGetBrushOrg(hdc, &ptOrg);
    /*
     * If we have scrollbars
     */
    if (pmenu->dwArrowsOn != MSA_OFF) {
        /*
         * If not drawing on the client area only
         */
        if (TestMF(pmenu, MFWINDOWDC)) {
            ptOrg.x = 0;
            ptOrg.y = -(int)MNGetToppItem(pmenu)->yItem;
        } else {
            ptOrg.x = -MNXBORDER;
            ptOrg.y = -MNYBORDER - gcyMenuScrollArrow - MNGetToppItem(pmenu)->yItem;
        }
    } else {
        if (TestMF(pmenu, MFWINDOWDC)) {
            ptOrg.x = MNXBORDER;
            ptOrg.y = MNYBORDER;
        } else {
            fSetOrg = FALSE;
        }
    }

    if (fSetOrg) {
        GreSetBrushOrg(hdc, ptOrg.x, ptOrg.y, &ptOrg);
    }
    hbrOld = GreSelectBrush(hdc, pmenu->hbrBack);

    GrePatBlt(hdc, x, y, cx, cy, PATCOPY);

    if (fSetOrg) {
        GreSetBrushOrg(hdc, ptOrg.x, ptOrg.y, NULL);
    }
    GreSelectBrush(hdc, hbrOld);
}

/***************************************************************************\
* MNAnimate
*
*  If fIterate is TRUE, then perform the next iteration in the menu animation
*  sequence.  If fIterate is FALSE, terminate the animation sequence.
*
* History:
*  07-23-96 GerardoB - fixed up for 5.0
\***************************************************************************/
void MNAnimate(PMENUSTATE pMenuState, BOOL fIterate)
{
    DWORD   dwTimeElapsed;
    int     x, y, xOff, yOff, xLast, yLast;

    if (TestFadeFlags(FADE_MENU)) {
        if (!fIterate) {
            StopFade();
        }
        return;
    }

    /*
     * If we're not animating, bail
     */
    if (pMenuState->hdcWndAni == NULL) {
        return;
    }

    /*
     * The active popup must be visible. It's supposed to be the
     *  window we're animating
     */
    UserAssert(TestWF(pMenuState->pGlobalPopupMenu->spwndActivePopup, WFVISIBLE));

    /*
     * End animation if asked to do so, if it's taking too long
     *  or someone is waiting for the critical section
     */
    dwTimeElapsed = NtGetTickCount() - pMenuState->dwAniStartTime;
    if (!fIterate
            || (dwTimeElapsed > CMS_QANIMATION)
            || (ExGetExclusiveWaiterCount(gpresUser) > 0)
            || (ExGetSharedWaiterCount(gpresUser) > 0)) {

        GreBitBlt(pMenuState->hdcWndAni, 0, 0, pMenuState->cxAni, pMenuState->cyAni, pMenuState->hdcAni,
            0, 0, SRCCOPY | NOMIRRORBITMAP, 0xFFFFFF);

        goto AnimationCompleted;
    }

    /*
     * Remember current animation point and calculate new one
     */
    xLast = pMenuState->ixAni;
    yLast = pMenuState->iyAni;
    if (pMenuState->iAniDropDir & PAS_HORZ) {
        pMenuState->ixAni = MultDiv(gcxMenuFontChar, dwTimeElapsed, CMS_QANIMATION / 20);
        if (pMenuState->ixAni > pMenuState->cxAni) {
            pMenuState->ixAni = pMenuState->cxAni;
        }
    }

    if (pMenuState->iAniDropDir & PAS_VERT) {
        pMenuState->iyAni = MultDiv(gcyMenuFontChar, dwTimeElapsed, CMS_QANIMATION / 10);
        if (pMenuState->iyAni > pMenuState->cyAni) {
            pMenuState->iyAni = pMenuState->cyAni;
        }
    }

     /*
      * if no change -- bail out
      */
    if ((pMenuState->ixAni == xLast) && (pMenuState->iyAni == yLast)) {
        return;
    }

    /*
     * Calculate source and dest coordinates
     */
    if (pMenuState->iAniDropDir & PAS_LEFT) {
        x = pMenuState->cxAni - pMenuState->ixAni;
        xOff = 0;
    } else {
        xOff = pMenuState->cxAni - pMenuState->ixAni;
        x = 0;
    }

    if (pMenuState->iAniDropDir & PAS_UP) {
        y = pMenuState->cyAni - pMenuState->iyAni;
        yOff = 0;
    } else {
        yOff = pMenuState->cyAni - pMenuState->iyAni;
        y = 0;
    }

    /*
     * Do it
     */
    GreBitBlt(pMenuState->hdcWndAni, x, y, pMenuState->ixAni, pMenuState->iyAni,
              pMenuState->hdcAni, xOff, yOff, SRCCOPY | NOMIRRORBITMAP, 0xFFFFFF);

    /*
     * Check if we're done
     */
    if ((pMenuState->cxAni == pMenuState->ixAni)
            && (pMenuState->cyAni == pMenuState->iyAni)) {

AnimationCompleted:

        MNDestroyAnimationBitmap(pMenuState);
        _ReleaseDC(pMenuState->hdcWndAni);
        pMenuState->hdcWndAni = NULL;
        _KillTimer(pMenuState->pGlobalPopupMenu->spwndActivePopup, IDSYS_MNANIMATE);
    }

}

/***************************************************************************\
* DrawMenuItemCheckMark() -
*
* Draws the proper check mark for the given item.  Note that ownerdraw
* items should NOT be passed to this procedure, otherwise we'd draw a
* checkmark for them when they are already going to take care of it.
*
* Returns TRUE is a bitmap was drawn (or at least we attempted to draw it).
*
* History:
\***************************************************************************/
BOOL DrawMenuItemCheckMark(HDC hdc, PITEM pItem, int xPos)
{
    int     yCenter;
    HBITMAP hbm;
    DWORD   textColorSave;
    DWORD   bkColorSave;
    BOOL    fChecked;
    POEMBITMAPINFO  pOem;
    BOOL fRet = TRUE;
    DWORD dwFlags = BC_INVERT;

    UserAssert(hdc != ghdcMem2);
    pOem = gpsi->oembmi + OBI_MENUCHECK;
    yCenter = pItem->cyItem - pOem->cy;
    if (yCenter < 0)
        yCenter = 0;
    yCenter /= 2;

    fChecked = TestMFS(pItem, MFS_CHECKED);

    if (hbm = (fChecked) ? pItem->hbmpChecked : pItem->hbmpUnchecked) {
        HBITMAP hbmSave;

        // Use the app supplied bitmaps.
        if (hbmSave = GreSelectBitmap(ghdcMem2, hbm)) {

            textColorSave = GreSetTextColor(hdc, 0x00000000L);
            bkColorSave   = GreSetBkColor  (hdc, 0x00FFFFFFL);

            if (TestMFT(pItem, MFT_RIGHTORDER))
                xPos = pItem->cxItem - pOem->cx;

            GreBitBlt(hdc,
                      xPos,
                      yCenter,
                      pOem->cx,
                      pOem->cy,
                      ghdcMem2,
                      0,
                      0,
                      SRCSTENCIL,
                      0x00FFFFFF);

            GreSetTextColor(hdc, textColorSave);
            GreSetBkColor(hdc, bkColorSave);

            GreSelectBitmap(ghdcMem2, hbmSave);
        }

    } else if (fChecked) {

        if (TestMFT(pItem, MFT_RADIOCHECK))
            pOem = gpsi->oembmi + OBI_MENUBULLET;

        if (TestMFT(pItem, MFT_RIGHTORDER))
            xPos = pItem->cxItem - pOem->cx;

#ifdef USE_MIRRORING
        // 389917: Mirror active menu's check mark if hdc is mirroed.
        if ((GreGetLayout(hdc) & LAYOUT_RTL) && (hdc != gpDispInfo->hdcGray)) {
            dwFlags |= BC_NOMIRROR;
        }
#endif
        BltColor(hdc,
                 NULL,
                 HDCBITS(),
                 xPos,
                 yCenter,
                 pOem->cx,
                 pOem->cy,
                 pOem->x,
                 pOem->y,
                 dwFlags);
    } else {
        fRet = FALSE;
    }

    return fRet;
}


/***************************************************************************\
* xxxDrawItemtUnderline
*
* Draws or hides an underline for a menu item
*
* 07/23/96  vadimg      separated into a separate routine
\***************************************************************************/

void xxxDrawItemUnderline(PITEM pItem, HDC hdc, int xLeft, int yTop,
        LPWSTR pszMenu, LONG lResLo)
{
    int   cx;
    PTHREADINFO ptiCurrent = PtiCurrentShared();
    //
    // LOWORD of result is 0xFFFF if there is no underlined character.
    // Therefore ulX must be valid or be UNDERLINE_RECALC because the item
    // or menu mode changed.
    //
    // Bail out if there isn't one.
    //
    if (lResLo == 0xFFFF)
        return;

    //
    // For proportional fonts, or if an LPK is installed, find starting
    // point of underline
    //
    if ((pItem->ulX == UNDERLINE_RECALC) || (PpiCurrent()->dwLpkEntryPoints & LPK_INSTALLED)) {
        if (lResLo != 0) {
            SIZE size;

            if (CALL_LPK(ptiCurrent)) {
                xxxClientGetTextExtentPointW(hdc, pszMenu, lResLo, &size);
            } else {
                GreGetTextExtentW(hdc, pszMenu, lResLo, &size, GGTE_WIN3_EXTENT);
            }
            pItem->ulX = size.cx - gcxMenuFontOverhang;
        } else
            pItem->ulX = 0;
    }

    xLeft += pItem->ulX;

    //
    // Adjust for proportional font when setting the length of the underline
    // and height of text.
    //
    // Calculate underline width.
    if (!pItem->ulWidth) {
        SIZE size;

        if (CALL_LPK(ptiCurrent)) {
            xxxClientGetTextExtentPointW(hdc, pszMenu + lResLo, 1, &size);
        } else {
            GreGetTextExtentW(hdc, pszMenu + lResLo, 1, &size, GGTE_WIN3_EXTENT);
        }
        pItem->ulWidth = size.cx - gcxMenuFontOverhang;
    }
    cx = pItem->ulWidth;

    // Get ascent of text (units above baseline) so that underline can be drawn
    // below the text
    yTop += gcyMenuFontAscent;

    // Proper brush should be selected into dc.
    GrePatBlt(hdc, xLeft, yTop, pItem->ulWidth, SYSMET(CYBORDER), PATCOPY);
}

/***************************************************************************\
*
*  xxxDrawMenuItemText()
*
*  Draws menu text with underline.
*
\***************************************************************************/
void xxxDrawMenuItemText(PITEM pItem, HDC hdc, int xLeft, int yTop,
    LPWSTR  lpsz, int cch, BOOL fShowUnderlines)
{
    LONG  result;
    WCHAR  szMenu[MENU_STRLEN], *pchOut;
    PTHREADINFO ptiCurrent = PtiCurrentShared();
    TL tl;

    if (cch > MENU_STRLEN) {
        pchOut = (WCHAR*)UserAllocPool((cch+1) * sizeof(WCHAR), TAG_RTL);
        if (pchOut == NULL)
            return;
        ThreadLockPool(ptiCurrent, pchOut, &tl);
    } else {
        pchOut = szMenu;
    }
    result = GetPrefixCount(lpsz, cch, pchOut, cch);

    if (CALL_LPK(ptiCurrent)) {
        xxxClientExtTextOutW(hdc, xLeft, yTop, 0, NULL, pchOut, cch - HIWORD(result), NULL);
    } else {
        GreExtTextOutW(hdc, xLeft, yTop, 0, NULL, pchOut, cch - HIWORD(result), NULL);
    }

    if (fShowUnderlines || TEST_BOOL_ACCF(ACCF_KEYBOARDPREF) || TestEffectInvertUP(KEYBOARDCUES)
                        || (GetAppCompatFlags2(VER40) & GACF2_KCOFF)) {
        if (CALL_LPK(ptiCurrent)) {
            xxxPSMTextOut(hdc, xLeft, yTop, lpsz, cch, DT_PREFIXONLY);
        } else{
            xxxDrawItemUnderline(pItem, hdc, xLeft, yTop, pchOut, LOWORD(result));
        }
    }
    if (pchOut != szMenu) {
        ThreadUnlockAndFreePool(ptiCurrent, &tl);
    }
}

/***************************************************************************\
* xxxSendMenuDrawItemMessage
*
* Sends a WM_DRAWITEM message to the owner of the menu (pMenuState->hwndMenu).
* All state is determined in this routine so HILITE state must be properly
* set before entering this routine..
*
* Revalidation notes:
*  This routine must be called with a valid and non-NULL pwnd.
*  Revalidation is not required in this routine: no windows are used after
*  potentially leaving the critsect.
*
* History:
\***************************************************************************/

void xxxSendMenuDrawItemMessage(
    HDC hdc,
    UINT itemAction,
    PMENU pmenu,
    PITEM pItem,
    BOOL fBitmap,
    int iOffset)
{
    DRAWITEMSTRUCT dis;
    TL tlpwndNotify;
    int y;

    CheckLock(pmenu);

    dis.CtlType = ODT_MENU;
    dis.CtlID = 0;

    dis.itemID = pItem->wID;

    dis.itemAction = itemAction;
    dis.itemState   =
         ((pItem->fState & MF_GRAYED)       ? ODS_GRAYED    : 0) |
         ((pItem->fState & MFS_DEFAULT)     ? ODS_DEFAULT   : 0) |
         ((pItem->fState & MFS_CHECKED)     ? ODS_CHECKED   : 0) |
         ((pItem->fState & MFS_DISABLED)    ? ODS_DISABLED  : 0) |
         (mnDrawHilite(pItem)               ? ODS_SELECTED  : 0) |
         ((pItem->fState & MFS_HOTTRACK)    ? ODS_HOTLIGHT  : 0) |
         (TestMF(pmenu, MFINACTIVE)         ? ODS_INACTIVE  : 0) |
         (!TestMF(pmenu, MFUNDERLINE)       ? ODS_NOACCEL : 0);

    dis.hwndItem = (HWND)PtoH(pmenu);
    dis.hDC = hdc;

    y = pItem->yItem;
    if (fBitmap) {
        y = (pItem->cyItem - pItem->cyBmp) / 2;
    }

    dis.rcItem.left     = iOffset + pItem->xItem;
    dis.rcItem.top      = y;
    dis.rcItem.right    = iOffset + pItem->xItem + (fBitmap ? pItem->cxBmp : pItem->cxItem);
    dis.rcItem.bottom   = y + (fBitmap ? pItem->cyBmp : pItem->cyItem);
    dis.itemData = pItem->dwItemData;

    if (pmenu->spwndNotify != NULL) {
        ThreadLockAlways(pmenu->spwndNotify, &tlpwndNotify);
        xxxSendMessage(pmenu->spwndNotify, WM_DRAWITEM, 0, (LPARAM)&dis);
        ThreadUnlock(&tlpwndNotify);
    }

}

/***************************************************************************\
* CalcbfExtra
*
* History:
* 08-09-96 GerardoB  Made into an inline function (code from xxxMenuDraw)
\***************************************************************************/
__inline UINT CalcbfExtra (void)
{
if ((SYSRGB(3DHILIGHT) == SYSRGB(MENU)) && (SYSRGB(3DSHADOW) == SYSRGB(MENU)))
        return BF_FLAT | BF_MONO;
    else
        return 0;
}
/***************************************************************************\
* MNDrawInsertionBar
*
* History:
* 11/21/96 GerardoB  Created
\***************************************************************************/
void MNDrawInsertionBar (HDC hdc, PITEM pItem)
{
    BOOL fTop;
    POLYPATBLT PolyData [3], *ppd;

    /*
     * If no insertion bar for this item, bail
     */
    fTop = TestMFS(pItem, MFS_TOPGAPDROP);
    if (!fTop && !TestMFS(pItem, (MFS_BOTTOMGAPDROP))) {
        return;
    }

    /*
     * Vertical line on the left
     */
    ppd = PolyData;
    ppd->x = pItem->xItem + SYSMET(CXDRAG);
    ppd->cx = SYSMET(CXDRAG);
    ppd->cy = SYSMET(CYDRAG);
    if (fTop) {
        ppd->y = pItem->yItem;
    } else {
        ppd->y = pItem->yItem + pItem->cyItem - ppd->cy;
    }
    ppd->BrClr.hbr = SYSHBR(HIGHLIGHT);

    /*
     * Horizontal line in the middle
     */
    ppd++;
    ppd->x = pItem->xItem + (2 * SYSMET(CXDRAG));
    ppd->cx = pItem->cxItem - (4 * SYSMET(CXDRAG));
    ppd->cy = SYSMET(CYDRAG) / 2;
    if (fTop) {
        ppd->y = pItem->yItem;
    } else {
        ppd->y = pItem->yItem + pItem->cyItem - ppd->cy;
    }
    ppd->BrClr.hbr = PolyData->BrClr.hbr;

    /*
     * Vertical line on the right
     */
    ppd++;
    ppd->x = pItem->xItem + pItem->cxItem - (2 * SYSMET(CXDRAG));
    ppd->cx = PolyData->cx;
    ppd->cy = PolyData->cy;
    ppd->y = PolyData->y;
    ppd->BrClr.hbr = PolyData->BrClr.hbr;

    GrePolyPatBlt(hdc, PATCOPY, PolyData, 3, PPB_BRUSH);

}
/***************************************************************************\
* xxxDrawMenuItem
*
* !
*
* History:
\***************************************************************************/

void xxxDrawMenuItem(
    HDC hdc,
    PMENU pMenu,
    PITEM pItem,
    DWORD dwFlags)
{
    BOOL fHilite;
    HFONT   hfnOld;
    int     tcExtra;
    UINT    uFlags;
    int     iBkSave;
    hfnOld = NULL;
    uFlags = DST_COMPLEX;


    CheckLock(pMenu);

    /*
     * If the insertion bar is on (MFS_GAPDROP), don't draw the item hilited
     */
    fHilite = mnDrawHilite(pItem);


    if (TestMFS(pItem, MFS_DEFAULT))
    {
        if (ghMenuFontDef != NULL)
            hfnOld = GreSelectFont(hdc, ghMenuFontDef);
        else
        {
            uFlags |= DSS_DEFAULT;
            tcExtra = GreGetTextCharacterExtra(hdc);
            GreSetTextCharacterExtra(hdc, tcExtra + 1 + (gcxMenuFontChar / gpsi->cxSysFontChar));
        }
    }

    if (TestMFT(pItem, MFT_OWNERDRAW)) {

        /*
         * If ownerdraw, just set the default menu colors since the app is
         * responsible for handling the rest.
         */
        GreSetTextColor(hdc, SYSRGB(MENUTEXT));
        GreSetBkColor(hdc, SYSRGB(MENU));

        /*
         * Send drawitem message since this is an ownerdraw item.
         */
        xxxSendMenuDrawItemMessage(hdc,
                (UINT)((dwFlags & DMI_INVERT) ? ODA_SELECT : ODA_DRAWENTIRE),
                pMenu, pItem,FALSE,0);

        // Draw the hierarchical arrow for the cascade menu.
        if (TestMF(pMenu, MFISPOPUP) && (pItem->spSubMenu != NULL))
        {
            POEMBITMAPINFO pOem;
            HBRUSH hbr = fHilite ? SYSHBR(HIGHLIGHTTEXT) : SYSHBR(MENUTEXT);

            pOem = gpsi->oembmi + (TestMFT(pItem, MFT_RIGHTORDER)
                                           ? OBI_MENUARROW_L : OBI_MENUARROW);

            // This item has a hierarchical popup associated with it. Draw the
            // bitmap dealy to signify its presence. Note we check if fPopup is set
            // so that this isn't drawn for toplevel menus that have popups.

            BltColor(hdc,
                     hbr,
                     HDCBITS(),
                     TestMFT(pItem, MFT_RIGHTORDER)
                         ? pItem->xItem + pOem->cx :
                           pItem->xItem + pItem->cxItem - pOem->cx,
                     pItem->yItem + max((INT)(pItem->cyItem - 2 - pOem->cy) / 2,
                                        0),
                     pOem->cx,
                     pOem->cy,
                     pOem->x,
                     pOem->y,
                     BC_INVERT);
        }
    } else {
        int         icolBack;
        int         icolFore;
        GRAYMENU    gm;

        //
        // Setup colors and state
        //
        if (fHilite && TestMF(pMenu, MFISPOPUP)) {
            icolBack = COLOR_HIGHLIGHT;
            icolFore = COLOR_HIGHLIGHTTEXT;
        } else {
            icolBack = COLOR_MENU;
            icolFore = COLOR_MENUTEXT;
        }

        // B#4157 - Lotus doesn't like it if we draw
        // its disabled menu items in GRAY, t-arthb
        // MAKE SURE MF_GRAYED stays 0x0001 NOT 0x0003 for this to fix

        /*
         * System bitmaps are already grayed so don't draw them disabled
         *  if the menu is inactive
         */
        if (!MNIsCachedBmpOnly(pItem)
                    && (TestMFS(pItem, MF_GRAYED) || TestMF(pMenu, MFINACTIVE))) {
            //
            // Only do embossing if menu color is same as 3D color.  The
            // emboss uses 3D hilight & 3D shadow, which doesn't look cool
            // on a different background.
            //
            if ((icolBack == COLOR_HIGHLIGHT) ||
                (SYSRGB(MENU) != SYSRGB(3DFACE)) || SYSMET(SLOWMACHINE)) {
                //
                // If gray text won't show up on background, then dither.
                //
                if (SYSRGB(GRAYTEXT) == gpsi->argbSystem[icolBack])
                    uFlags |= DSS_UNION;
                else
                    icolFore = COLOR_GRAYTEXT;
            } else
            {
                if ((SYSRGB(3DSHADOW) == SYSRGB(MENU)) && (SYSRGB(3DHILIGHT) == SYSRGB(MENU)))
                    uFlags |= DSS_UNION;
                else
                    uFlags |= TestMF(pMenu, MFINACTIVE) ? DSS_INACTIVE : DSS_DISABLED;
            }
        }

        GreSetBkColor(hdc, gpsi->argbSystem[icolBack]);
        GreSetTextColor(hdc, gpsi->argbSystem[icolFore]);
        if (((dwFlags & DMI_INVERT) && (pMenu->hbrBack == NULL))
                || fHilite) {

            POLYPATBLT PolyData;

            /*
             * Only fill the background if we're being called on behalf of
             * MNInvertItem. This is so that we don't waste time filling
             * the unselected rect when the menu is first pulled down.
             * If the menu has a background brush and we were called by
             * MNInvertItem, that function will have already taken care of
             * filling the background
             */

            PolyData.x         = pItem->xItem;
            PolyData.y         = pItem->yItem;
            PolyData.cx        = pItem->cxItem;
            PolyData.cy        = pItem->cyItem;
            PolyData.BrClr.hbr = SYSHBRUSH(icolBack);

            GrePolyPatBlt(hdc, PATCOPY, &PolyData, 1, PPB_BRUSH);
        }

        if (pMenu->hbrBack != NULL) {
            iBkSave = GreSetBkMode(hdc, TRANSPARENT);
        }
        GreSelectBrush(hdc, SYSHBRUSH(icolFore));

        //
        // Draw the image
        //
        gm.pItem   = pItem;
        gm.pMenu   = pMenu;

        xxxDrawState(hdc,
            SYSHBRUSH(icolFore),
            (LPARAM)(PGRAYMENU)&gm,
            pItem->xItem,
            pItem->yItem,
            pItem->cxItem,
            pItem->cyItem,
            uFlags);

        MNDrawMenu3DHotTracking(hdc, pMenu, pItem);
    }

    /*
     * Draw the drop insertion bar, if any
     */
    MNDrawInsertionBar (hdc, pItem);


    if (pMenu->hbrBack != NULL)
        GreSetBkMode(hdc, iBkSave);

    if (TestMFS(pItem, MFS_DEFAULT))
    {
        if (hfnOld)
            GreSelectFont(hdc, hfnOld);
        else
            GreSetTextCharacterExtra(hdc, tcExtra);
    }
}

extern void SetupFakeMDIAppStuff(PMENU lpMenu, PITEM lpItem);

/***************************************************************************\
*
*  xxxRealDrawMenuItem()
*
*  Callback from xxxDrawState() to draw the menu item, either normally or into
*  an offscreen bitmp.  We don't know where we're drawing, and shouldn't
*  have to.
*
\***************************************************************************/
BOOL CALLBACK xxxRealDrawMenuItem(
    HDC hdc,
    PGRAYMENU pGray,
    int cx,
    int cy)
{
    PMENU  pMenu;
    PITEM  pItem;
    BOOL    fPopup;
    int     cch;
    int     xLeft;
    int     yTop;
    int     tp;
    int     rp;
    LPWSTR   lpsz;
    int     cyTemp;
    int     xHilite = 0;
    int     yHilite = 0;
    TL     tlpwndChild;
    PTHREADINFO  ptiCurrent = PtiCurrent();
    BOOL fCheckDrawn = FALSE;
    int     xFarLeft;
    //
    // BOGUS
    // Use cx and cy instead of lpItem->cxItem, lpItem->cyItem to
    // speed stuff up.
    //
    pMenu = pGray->pMenu;
    CheckLock(pMenu);
    pItem = pGray->pItem;
    fPopup = TestMF(pMenu, MFISPOPUP);

    if (fPopup) {
        xLeft = MNLEFTMARGIN;
        if (TestMF(pMenu, MNS_NOCHECK)) {
            xLeft += MNXSPACE;
        } else {
            fCheckDrawn = DrawMenuItemCheckMark(hdc, pItem, xLeft);
            if (!TestMF(pMenu, MNS_CHECKORBMP)
                    || ((pItem->hbmp == NULL) || fCheckDrawn)) {

                xLeft += TestMFT(pItem, MFT_RIGHTORDER)
                            ? 0 : (gpsi->oembmi[OBI_MENUCHECK].cx + MNXSPACE);
            }
        }
    } else {
        xLeft = 0;

        if (TestMFS(pItem, MFS_HILITE)) {
            xHilite = CXMENU3DEDGE;
            yHilite = CYMENU3DEDGE;
        }
    }

    /*
     * If there is not bitmap or we don't to draw it, go draw the text
     */
    if ((pItem->hbmp == NULL)
        || (fCheckDrawn
            && TestMF(pMenu, MNS_CHECKORBMP))) {
        goto RealDrawMenuItemText;
    }

    /*
     * Draw the bitmap
     */
    if (TestMFS(pItem, MFS_CACHEDBMP)) {
        if (pItem->hbmp == HBMMENU_SYSTEM) {
            /*
             * Drawing app icon (system menu)
             */
            PWND  pwndChild;
            PICON pIcon = NULL;
            UINT cyUse, cxUse;

AintNothingLikeTheRealMDIThing:
            if (!(pItem->dwItemData))
                SetupFakeMDIAppStuff(pMenu, pItem);

            pwndChild = HMValidateHandleNoRip((HWND)(pItem->dwItemData),TYPE_WINDOW);
            if (!pwndChild)
            {
                //
                // Oops, child window isn't valid anymore.  Go find
                // the new one.
                //
                if (pItem->dwItemData)
                {
                    pItem->dwItemData = 0;
                    goto AintNothingLikeTheRealMDIThing;
                }

                pIcon = NULL;
            }
            else {
                ThreadLock(pwndChild, &tlpwndChild);
                pIcon = xxxGetWindowSmIcon(pwndChild, FALSE);
                ThreadUnlock(&tlpwndChild);
            }


            if (!pIcon)
                pIcon = SYSICO(WINLOGO);

            cyUse = cy - SYSMET(CYEDGE);
            cxUse = cx - (SYSMET(CXEDGE) * 2);
            /*
             * If this is a popup, make sure that no weird
             *  width/height stretch takes place.
             */
            if (fPopup && (cyUse < cxUse)) {
                cxUse = cyUse;
            }

            _DrawIconEx(hdc, xLeft + (SYSMET(CXEDGE) * 2),
                  SYSMET(CYBORDER), pIcon, cxUse,
                  cyUse, 0, SYSHBR(MENU), DI_NORMAL);

        } else {
            /*
             * This is a cached bitmap
             */
            UINT wBmp;
            int xBmpLeft = xLeft;
            int y;
            POEMBITMAPINFO pOem;

            switch ((ULONG_PTR)pItem->hbmp) {
                case (ULONG_PTR)HBMMENU_MBAR_RESTORE:
                    wBmp = OBI_RESTORE_MBAR;
                    goto DrawSysBmp;

                case (ULONG_PTR)HBMMENU_MBAR_MINIMIZE:
                    wBmp = OBI_REDUCE_MBAR;
                    xBmpLeft += SYSMET(CXEDGE);
                    goto DrawSysBmp;

                case (ULONG_PTR)HBMMENU_MBAR_CLOSE:
                    wBmp = OBI_CLOSE_MBAR;
                    goto DrawSysBmp;

                case (ULONG_PTR)HBMMENU_MBAR_CLOSE_D:
                    wBmp = OBI_CLOSE_MBAR_I;
                    goto DrawSysBmp2;

                case (ULONG_PTR)HBMMENU_MBAR_MINIMIZE_D:
                    wBmp = OBI_REDUCE_MBAR_I;
                    xBmpLeft += SYSMET(CXEDGE);
                    goto DrawSysBmp2;

DrawSysBmp:
                    /*
                     * Select proper bitmap based on the item state
                     */
                    if (TestMFS(pItem, MFS_HILITE)) {
                        wBmp += DOBI_PUSHED;
                    }

DrawSysBmp2:
                    BitBltSysBmp(hdc, xBmpLeft, SYSMET(CYEDGE), wBmp);
                    break;

                default:
                    UserAssert((pItem->hbmp >= HBMMENU_POPUPFIRST)
                                && (pItem->hbmp <= HBMMENU_POPUPLAST));

                    wBmp = OBI_POPUPFIRST + HandleToUlong(pItem->hbmp) - HandleToUlong(HBMMENU_POPUPFIRST);
                    UserAssert(wBmp < OBI_COUNT);

                    pOem = gpsi->oembmi + wBmp;
                    y = (pItem->cyItem - pOem->cy) / 2;
                    if (y < 0) {
                        y = 0;
                    }
                    BltColor(hdc, NULL, HDCBITS(), xLeft, y,
                             pOem->cx, pOem->cy, pOem->x, pOem->y, BC_INVERT);
                    break;
            }

        } /* if (pItem->hbmp == HBMMENU_SYSTEM) */


    } else if (pItem->hbmp == HBMMENU_CALLBACK) {
        /*
         * Owner draw bitmap
         */
        xxxSendMenuDrawItemMessage(hdc,ODA_DRAWENTIRE, pMenu, pItem, TRUE, xLeft);

    } else {
        /*
         * Drawing a regular bitmap.
         */

        int dx, dy;
        HBITMAP hbmSave;

        //
        // Is this the zero'th item in a menu bar that's not all
        // bitmaps?  Hmm, sounds like it could be a fake MDI dude.
        // If it is, use the windows icon instead
        //
        /*
         * Let's fail this for > 4.0 apps so we can get rid of
         *  this horrible hack someday. The HBMMENU_ constants
         *  have been made public so people can use them freely.
         *
         * Note: even if the app is marked as 4.0, he could be
         * recompiled and may utilizes the new feature in NT5 menu.
         * So just in case, we have to check both dwItemData and lpstr
         * so that the menu could have bitmap, dwItemData and a menu string.
         *
         */
        if (ptiCurrent->dwExpWinVer <= VER40) {
            if (pItem->dwItemData && pItem->lpstr == NULL)
                goto AintNothingLikeTheRealMDIThing;
            else if (!fPopup &&
                    (pItem == pMenu->rgItems) &&
                    (pMenu->cItems > 1) &&
                    !(pMenu->rgItems[1].hbmp) &&
                    (pItem->spSubMenu)) {
                RIPMSG0(RIP_WARNING, "Fake MDI detected, using window icon in place of bitmap");
                goto AintNothingLikeTheRealMDIThing;
            }
        }

        UserAssert(hdc != ghdcMem2);

        dx = pItem->cxBmp;

        if (fPopup) {
            dy = pItem->cyBmp;

            //
            // Center bitmap in middle of item area
            //
            cyTemp = (pItem->cyItem - dy);
            if (cyTemp > 0)
                cyTemp = cyTemp / 2;
            else
                cyTemp = 0;
        } else {
            dy = max(pItem->cyBmp, SYSMET(CYMENUSIZE));
            cyTemp = 0;
            if (pItem->lpstr != NULL) {
                xLeft += gcxMenuFontChar;
            }
        }

        if (hbmSave = GreSelectBitmap(ghdcMem2, pItem->hbmp)) {
            BITMAP  bmp;
            //
            // Draw the bitmap leaving some room on the left for the
            // optional check mark if we are in a popup menu. (as opposed
            // to a top level menu bar).
            //
            // We can do cool stuff with monochrome bitmap itmes
            // by merging with the current colors.
            //
            // If the item is selected and the bitmap isn't monochrome,
            // we just invert the thing when we draw it.  We can't do
            // anything more clever unless we want to convert to
            // monochrome.
            //
            GreExtGetObjectW(pItem->hbmp, sizeof(bmp), (LPSTR)&bmp);
            GreBitBlt(hdc, xLeft + xHilite, cyTemp + xHilite, dx, dy, ghdcMem2, 0, 0,
                (bmp.bmPlanes * bmp.bmBitsPixel == 1)   ?
                SRCSTENCIL                              :
                (mnDrawHilite(pItem) ? NOTSRCCOPY : SRCCOPY),
                0x00FFFFFF);
            GreSelectBitmap(ghdcMem2, hbmSave);
        } else {
            RIPMSG3(RIP_WARNING, "Menu 0x%08X, item 0x%08X: Tried to draw invalid bitmap 0x%08X", pMenu, pItem, pItem->hbmp) ;
        }
    }


RealDrawMenuItemText:
    if (pItem->lpstr != NULL) {
        /*
         * We want the text in all popup menu items to be aligned
         *  if an alignment offset is available.
         */
        if (fPopup && (pMenu->cxTextAlign != 0)) {
            xLeft = pMenu->cxTextAlign;
        } else if (pItem->hbmp != NULL) {
            xLeft += pItem->cxBmp + SYSMET(CXEDGE);
        }

        // This item is a text string item. Display it.
        yTop = gcyMenuFontExternLeading;

        cyTemp = pItem->cyItem - (gcyMenuFontChar + gcyMenuFontExternLeading + SYSMET(CYBORDER));
        if (cyTemp > 0)
            yTop += (cyTemp / 2);

        if (!fPopup && (pItem->hbmp == NULL)) {
            xLeft += gcxMenuFontChar;
        }

        lpsz = TextPointer(pItem->lpstr);
        if (lpsz!=NULL) {
            cch = pItem->cch;

            // Even though we no longer support any funky handling of the
            // help prefix character, we still need to eat it up if we run
            // across it so that the menu item is drawn correctly
            if ((*lpsz == CH_HELPPREFIX) && !fPopup) {
                // Skip help prefix character.
                lpsz++;
                cch--;
            }

            // tp will contain the character position of the \t indicator
            // in the menu string.  This is where we add a tab to the string.
            //
            // rp will contain the character position of the \a indicator
            // in the string.  All text following this is right aligned.
            tp = FindCharPosition(lpsz, TEXT('\t'));
            rp = FindCharPosition(lpsz, TEXT('\t') - 1);

            xFarLeft = pItem->cxItem - (gpsi->oembmi[OBI_MENUCHECK].cx + MNXSPACE);

            if (rp && (rp != cch)) {
                // Display all the text up to the \a
                if (TestMFT(pItem, MFT_RIGHTORDER) && fPopup) {
                    SIZE extent;

                    xxxPSMGetTextExtent(hdc, lpsz, rp, &extent);
                    xLeft = xFarLeft - extent.cx;
                }
                xxxDrawMenuItemText(pItem, hdc, xLeft + xHilite, yTop + xHilite, lpsz, rp,
                        TestMF(pMenu, MFUNDERLINE));

                // Do we also have a tab beyond the \a ??
                if (tp > rp + 1) {
                    SIZE extent;

                    if (TestMFT(pItem, MFT_RIGHTORDER) && fPopup) {
                        xLeft = xFarLeft - pItem->dxTab ;
                    } else {
                        xxxPSMGetTextExtent(hdc, lpsz + rp + 1,
                                (UINT)(tp - rp - 1), &extent);
                        xLeft = (int)(pItem->dxTab - extent.cx);
                    }
                    //
                    // lotus in Hebrew make their menus by putting the
                    // accelerator on the left and the text on the right
                    //
                    xxxPSMTextOut(hdc, xLeft, yTop, (LPWSTR)(lpsz + rp + 1), tp - rp - 1,
                                  TestMF(pMenu, MFUNDERLINE) ? 0 : DT_HIDEPREFIX);
                }
             } else if (tp && rp == cch) {
                // Display text up to the tab position
                if (TestMFT(pItem, MFT_RIGHTORDER)) {
                    SIZE extent;

                    xxxPSMGetTextExtent(hdc, lpsz, tp, &extent);
                    xLeft = xFarLeft - extent.cx;
                    if (!fPopup && (pItem->hbmp == NULL)) {
                        xLeft += gcxMenuFontChar;
                    }
                }
                xxxDrawMenuItemText(pItem, hdc, xLeft + xHilite, yTop + xHilite, lpsz, tp,
                        TestMF(pMenu, MFUNDERLINE));
             }

            // Any text left to display (like after the tab) ??
            if (tp < cch - 1) {
                if (TestMFT(pItem, MFT_RIGHTORDER) && fPopup) {
                    SIZE extent;

                    xxxPSMGetTextExtent(hdc, lpsz + tp + 1, (int)cch - tp - 1, &extent);
                    xLeft = pItem->cxItem - pItem->dxTab - extent.cx;
                } else {
                    xLeft = pItem->dxTab + gcxMenuFontChar;
                }
                xxxPSMTextOut(hdc, xLeft, yTop, lpsz + tp + 1, cch - tp - 1,
                              TestMF(pMenu, MFUNDERLINE) ? 0 : DT_HIDEPREFIX);
            }
        }
    }

    //
    // Draw the hierarchical arrow for the cascade menu.
    //
    if (fPopup && (pItem->spSubMenu != NULL)) {
        POEMBITMAPINFO pOem;

        pOem = gpsi->oembmi + (TestMFT(pItem, MFT_RIGHTORDER)
                               ? OBI_MENUARROW_L : OBI_MENUARROW);

        // This item has a hierarchical popup associated with it. Draw the
        // bitmap dealy to signify its presence. Note we check if fPopup is set
        // so that this isn't drawn for toplevel menus that have popups.

        BltColor(hdc,
                 NULL,
                 HDCBITS(),
                 TestMFT(pItem, MFT_RIGHTORDER)
                 ? pOem->cx :
                   pItem->cxItem - pOem->cx,
                 max((INT)(pItem->cyItem - 2 - pOem->cy) / 2, 0),
                 pOem->cx,
                 pOem->cy,
                 pOem->x,
                 pOem->y,
                 BC_INVERT);
    }

    return(TRUE);
}

/***************************************************************************\
* xxxMenuBarDraw
*
* History:
* 11-Mar-1992 mikeke   From win31
\***************************************************************************/

int xxxMenuBarDraw(
    PWND pwnd,
    HDC hdc,
    int cxFrame,
    int cyFrame)
{
    UINT cxMenuMax;
    int yTop;
    PMENU pMenu;
    BOOL fClipped = FALSE;
    TL tlpmenu;
    HRGN hrgnVisSave;
    HBRUSH hbrT;
    CheckLock(pwnd);

    /*
     * Lock the menu so we can poke around
     */
    pMenu = (PMENU)pwnd->spmenu;
    if (pMenu == NULL)
        return SYSMET(CYBORDER);

    /*
     * NT5 menus are drawn inactive when the window is not active.
     */
    if (TestwndFrameOn(pwnd) || (GetAppCompatFlags2(VER40) & GACF2_ACTIVEMENUS)) {
        ClearMF(pMenu, MFINACTIVE);
    } else {
        SetMF(pMenu, MFINACTIVE);
    }

    ThreadLock(pMenu, &tlpmenu);

    yTop = cyFrame;
    yTop += GetCaptionHeight(pwnd);


    /*
     * Calculate maximum available horizontal real estate
     */
    cxMenuMax = (pwnd->rcWindow.right - pwnd->rcWindow.left) - cxFrame * 2;

    /*
     * If the menu has switched windows, or if either count is 0,
     * then we need to recompute the menu width.
     */
    if (pwnd != pMenu->spwndNotify ||
            pMenu->cxMenu == 0 ||
            pMenu->cyMenu == 0) {

        xxxMenuBarCompute(pMenu, pwnd, yTop, cxFrame, cxMenuMax);
    }

    /*
     * If the menu rectangle is wider than allowed, or the
     * bottom would overlap the size border, we need to clip.
     */
    if (pMenu->cxMenu > cxMenuMax ||
            (int)(yTop + pMenu->cyMenu) > (int)((pwnd->rcWindow.bottom - pwnd->rcWindow.top)
            - cyFrame)) {

        /*
         * Lock the display while we're playing around with visrgns.  Make
         * a local copy of the saved-visrgn so it can be restored in case
         * we make a callback (i.e. WM_DRAWITEM).
         */
        GreLockDisplay(gpDispInfo->hDev);

        fClipped = TRUE;

        #ifdef LATER
        // mikeke don't use the visrgn here if possible
        hrgnVisSave = GreCreateRectRgn(
             pwnd->rcWindow.left + cxFrame,
             pwnd->rcWindow.top,
             pwnd->rcWindow.left + cxFrame + cxMenuMax,
             pwnd->rcWindow.bottom - cyFrame);
        GreExtSelectClipRgn(hdc, hrgnVisSave, RGN_COPY);
        GreSetMetaRgn(hdc);
        GreDeleteObject(hrgnVisSave);
        #else
        hrgnVisSave = CreateEmptyRgn();
        GreCopyVisRgn(hdc,hrgnVisSave);
        GreIntersectVisRect(hdc, pwnd->rcWindow.left + cxFrame,
                              pwnd->rcWindow.top,
                              pwnd->rcWindow.left + cxFrame + cxMenuMax,
                              pwnd->rcWindow.bottom - cyFrame);

        #endif

        GreUnlockDisplay(gpDispInfo->hDev);
    }

    {
        POLYPATBLT PolyData[2];

        PolyData[0].x         = cxFrame;
        PolyData[0].y         = yTop;
        PolyData[0].cx        = pMenu->cxMenu;
        PolyData[0].cy        = pMenu->cyMenu;
        PolyData[0].BrClr.hbr = (pMenu->hbrBack) ? pMenu->hbrBack : SYSHBR(MENU);

        PolyData[1].x         = cxFrame;
        PolyData[1].y         = yTop + pMenu->cyMenu;
        PolyData[1].cx        = pMenu->cxMenu;
        PolyData[1].cy        = SYSMET(CYBORDER);
        PolyData[1].BrClr.hbr = (TestWF(pwnd, WEFEDGEMASK) && !TestWF(pwnd, WFOLDUI))? SYSHBR(3DFACE) : SYSHBR(WINDOWFRAME);

        GrePolyPatBlt(hdc,PATCOPY,&PolyData[0],2,PPB_BRUSH);

        //
        // Draw menu background in MENU color
        //
        //hbrT = GreSelectBrush(hdc, SYSHBR(MENU));
        //GrePatBlt(hdc, cxFrame, yTop, pMenu->cxMenu, pMenu->cyMenu, PATCOPY);

        //
        // Draw border under menu in proper BORDER color
        //
        //GreSelectBrush(hdc, (TestWF(pwnd, WEFEDGEMASK) && !TestWF(pwnd, WFOLDUI))? SYSHBR(3DFACE) : SYSHBR(WINDOWFRAME));
        //GrePatBlt(hdc, cxFrame, yTop + pMenu->cyMenu, pMenu->cxMenu, SYSMET(CYBORDER), PATCOPY);


    }

    /*
     * Finally, draw the menu itself.
     */

    hbrT = GreSelectBrush(hdc, (TestWF(pwnd, WEFEDGEMASK) && !TestWF(pwnd, WFOLDUI))? SYSHBR(3DFACE) : SYSHBR(WINDOWFRAME));
    xxxMenuDraw(hdc, pMenu);
    GreSelectBrush(hdc, hbrT);

    if (fClipped) {
        #ifdef LATER
        // mikeke don't use the visrgn here if possible
        hrgnVisSave = CreateEmptyRgn();
        GreExtSelectClipRgn(hdc, hrgnVisSave, RGN_COPY);
        GreSetMetaRgn(hdc);
        GreDeleteObject(hrgnVisSave);
        #else
        GreLockDisplay(gpDispInfo->hDev);
        GreSelectVisRgn(hdc, hrgnVisSave, SVR_DELETEOLD);
        GreUnlockDisplay(gpDispInfo->hDev);
        #endif
    }

    ThreadUnlock(&tlpmenu);
    return(pMenu->cyMenu + SYSMET(CYBORDER));
}
/***************************************************************************\
* xxxMenuDraw
*
* Draws the menu
*
* Revalidation notes:
*  This routine must be called with a valid and non-NULL pwnd.
*
* History:
\***************************************************************************/

void xxxMenuDraw(
    HDC hdc,
    PMENU pmenu)
{
    PITEM pItem;
    UINT i, cy;
    RECT rcItem;
    HFONT       hFontOld;
    UINT        bfExtra;
    PTHREADINFO ptiCurrent = PtiCurrent();
    UINT        oldAlign;
    int         iBkSave;
    POINT       ptOrg;
    CheckLock(pmenu);

    if (pmenu == NULL) {
        RIPERR0(ERROR_INVALID_HANDLE,
                RIP_WARNING,
                "Invalid menu handle \"pmenu\" (NULL) to xxxMenuDraw");

        return;
    }

    GreGetViewportOrg(hdc, &ptOrg);
    hFontOld = GreSelectFont(hdc, ghMenuFont);

    oldAlign = GreGetTextAlign(hdc);
    if (pmenu->rgItems && TestMFT(pmenu->rgItems, MFT_RIGHTORDER))
        GreSetTextAlign(hdc, oldAlign | TA_RTLREADING);

    bfExtra = CalcbfExtra();

    if (pmenu->hbrBack != NULL) {
        iBkSave = GreSetBkMode(hdc, TRANSPARENT);
    }

    if (pmenu->dwArrowsOn != MSA_OFF) {
        pItem = MNGetToppItem(pmenu);
        GreSetViewportOrg(hdc, ptOrg.x, ptOrg.y - ((int)pItem->yItem), NULL);
        i = pmenu->iTop;
    } else {
        pItem = (PITEM)pmenu->rgItems;
        i = 0;
    }

    cy = 0;
    for (; i < pmenu->cItems; i++, pItem++) {
        if (TestMFT(pItem, MFT_MENUBARBREAK) &&
                TestMF(pmenu, MFISPOPUP)) {

            //
            // Draw a vertical etch.  This is done by calling DrawEdge(),
            // sunken, with BF_LEFT | BF_RIGHT.
            //
            if(TestMFT(pItem, MFT_RIGHTORDER) && i) {
                //
                // going backwards, so the correct place is just before the
                // _previous_ item.
                //
                PITEM pi;

                pi            = pItem - 1;
                rcItem.left   = pi->xItem - SYSMET(CXFIXEDFRAME);
                rcItem.top    = 0;
                rcItem.right  = pi->xItem - SYSMET(CXBORDER);
                rcItem.bottom = pmenu->cyMenu;
            } else {
                rcItem.left     = pItem->xItem - SYSMET(CXFIXEDFRAME);
                rcItem.top      = 0;
                rcItem.right    = pItem->xItem - SYSMET(CXBORDER);
                rcItem.bottom   = pmenu->cyMenu;
            }

            DrawEdge(hdc, &rcItem, BDR_SUNKENOUTER, BF_LEFT | BF_RIGHT | bfExtra);
        }
        /*
         * If this is a separator, draw it and return.
         * If version is less than 4.0  don't test the MFT_OWNERDRAW
         * flag. Bug 21922; App MaxEda has both separator and Ownerdraw
         * flags on. In 3.51 we didn't test the OwnerDraw flag
         */
        if (TestMFT(pItem, MFT_SEPARATOR)
                && (!TestMFT(pItem, MFT_OWNERDRAW)
                    || (LOWORD(ptiCurrent->dwExpWinVer) < VER40))) {

            /*
             * Draw a horizontal etch.
             */
            int yT = pItem->yItem + (pItem->cyItem / 2) - SYSMET(CYBORDER);
            RECT rcItem;

            rcItem.left     = pItem->xItem + 1;
            rcItem.top       = yT;
            rcItem.right    = pItem->xItem + pItem->cxItem - 1;
            rcItem.bottom   = yT + SYSMET(CYEDGE);

            DrawEdge(hdc, &rcItem, BDR_SUNKENOUTER, BF_TOP | BF_BOTTOM | bfExtra);
            /*
             * Draw drop insertion bar, if any.
             */
            MNDrawInsertionBar (hdc, pItem);

        } else {
            xxxDrawMenuItem(hdc, pmenu, pItem, 0);
        }

        if (pmenu->dwArrowsOn != MSA_OFF) {
            cy += pItem->cyItem;
            if (cy > pmenu->cyMenu) {
                /*
                 * this is a scrollable menu and the item just drawn falls below
                 * the bottom of the visible menu -- no need to draw any further
                 */
                break;
            }
        }
    } /* for (; i < pmenu->cItems; i++, pItem++) */

    if (pmenu->hbrBack != NULL) {
        GreSetBkMode(hdc, iBkSave);
    }
    GreSetViewportOrg(hdc, ptOrg.x, ptOrg.y, NULL);
    GreSetTextAlign(hdc, oldAlign);
    GreSelectFont(hdc, hFontOld);

}


/***************************************************************************\
* xxxDrawMenuBar
*
* Forces redraw of the menu bar
*
* History:
\***************************************************************************/

BOOL xxxDrawMenuBar(
    PWND pwnd)
{
    CheckLock(pwnd);

    if (!TestwndChild(pwnd)) {
        xxxRedrawFrame(pwnd);
    }

    return TRUE;
}


/***************************************************************************\
* xxxMenuInvert
*
* Invert menu item
*
* Revalidation notes:
*  This routine must be called with a valid and non-NULL pwnd.
*
*  fOn - TRUE if the item is selected thus it needs to be inverted
*  fNotify - TRUE if the parent should be notified (as appropriate), FALSE
*            if we are just redrawing the selected item.
*
* History:
\***************************************************************************/
PITEM xxxMNInvertItem(
    PPOPUPMENU ppopupmenu,
    PMENU pmenu,
    int itemNumber,
    PWND pwndNotify,
    BOOL fOn)
{
    PITEM pItem = NULL;
    HDC hdc;
    int y, iNewTop;
    RECT rcItem;
    BOOL fSysMenuIcon = FALSE;
    PMENU pmenusys;
    BOOL fClipped = FALSE;
    HFONT   hOldFont;
    HRGN hrgnVisSave;
    PWND pwnd;
    POINT ptOrg;
    TL tlpwnd;
    UINT oldAlign;

    CheckLock(pmenu);
    CheckLock(pwndNotify);

    /*
     * If we are in the middle of trying to get out of menu mode, hMenu
     * and/or pwndNotify will be NULL, so just bail out now.
     */
    if ((pmenu == NULL) || (pwndNotify == NULL)) {
        return NULL;
    }


    /*
     * If ppopupmenu is NULL, we're not in menu mode (i.e, call from
     *  HiliteMenuItem).
     */
    if (ppopupmenu == NULL) {
        pwnd = pwndNotify;
    } else {
        pwnd = ppopupmenu->spwndPopupMenu;
    }

    if (pwnd != pwndNotify) {
        ThreadLock(pwnd, &tlpwnd);
    }


    if (itemNumber < 0) {

        if (ppopupmenu != NULL) {
            if ((itemNumber == MFMWFP_UPARROW) || (itemNumber == MFMWFP_DOWNARROW)) {
                MNDrawArrow(NULL, ppopupmenu, itemNumber);
            }
        }

        xxxSendMenuSelect(pwndNotify, pwnd, pmenu, itemNumber);
        goto SeeYa;
    }

    if (!TestMF(pmenu, MFISPOPUP)) {
        pmenusys = xxxGetSysMenuHandle(pwndNotify);
        if (pmenu == pmenusys) {
            MNPositionSysMenu(pwndNotify, pmenusys);
            fSysMenuIcon = TRUE;
        }
    }

    if ((UINT)itemNumber >= pmenu->cItems)
        goto SeeYa;

    pItem = &pmenu->rgItems[itemNumber];

    if (!TestMF(pmenu, MFISPOPUP) && TestWF(pwndNotify, WFMINIMIZED)) {

        /*
         * Skip inverting top level menus if the window is iconic.
         */
        goto SeeYa;
    }

    /*
     * Is this a separator?
     */
    if (TestMFT(pItem, MFT_SEPARATOR)) {
        goto SendSelectMsg;
    }

    if ((BOOL)TestMFS(pItem, MFS_HILITE) == (BOOL)fOn) {

        /*
         * Item's state isn't really changing.  Just return.
         */
        goto SeeYa;
    }

    if (fOn && (ppopupmenu != NULL) && (pmenu->dwArrowsOn != MSA_OFF)) {
        /*
         * when selecting an item, ensure that the item is fully visible
         * BUGBUG -- for mouse use, partially visible should be acceptable
         *        -- can we get mouse info down this far ?
         */

        if (itemNumber < pmenu->iTop) {
            iNewTop = itemNumber;
            goto NewTop;
        } else {
            PITEM pWalk = MNGetToppItem(pmenu);
            int dy = pItem->yItem + pItem->cyItem - pWalk->yItem - pmenu->cyMenu;
            iNewTop = pmenu->iTop;
            while ((dy > 0) && (iNewTop < (int)pmenu->cItems)) {
                dy -= pWalk->cyItem;
                pWalk++;
                iNewTop++;
            }
            if (iNewTop >= (int)pmenu->cItems) {
                iNewTop = pmenu->cItems;
            }
NewTop:
            if (xxxMNSetTop(ppopupmenu, iNewTop)) {
                xxxUpdateWindow(pwnd);
            }
        }
    }

    rcItem.left     = pItem->xItem;
    rcItem.top      = pItem->yItem;
    rcItem.right    = pItem->xItem + pItem->cxItem;
    rcItem.bottom   = pItem->yItem + pItem->cyItem;

    y = pItem->cyItem;

    if (TestMF(pmenu, MFISPOPUP)) {
        hdc = _GetDC(pwnd);
    } else {
        hdc = _GetWindowDC(pwnd);
        if ( TestWF(pwnd, WFSIZEBOX) && !fSysMenuIcon) {

            /*
             * If the window is small enough that some of the menu bar has been
             * obscured by the frame, we don't want to draw on the bottom of the
             * sizing frame.  Note that we don't want to do this if we are
             * inverting the system menu icon since that will be clipped to the
             * window rect.  (otherwise we end up with only half the sys menu
             * icon inverted)
             */
            int xMenuMax = (pwnd->rcWindow.right - pwnd->rcWindow.left) - SYSMET(CXSIZEFRAME);

            if (rcItem.right > xMenuMax ||
                    rcItem.bottom > ((pwnd->rcWindow.bottom -
                    pwnd->rcWindow.top) - SYSMET(CYSIZEFRAME))) {

                /*
                 * Lock the display while we're playing around with visrgns.
                 * Make a local copy of the visrgn, so that it can be
                 * properly restored on potential callbacks (i.e. WM_DRAWITEM).
                 */
                GreLockDisplay(gpDispInfo->hDev);

                fClipped = TRUE;

                #ifdef LATER
                // mikeke - don't use the visrgn here if possible
                hrgnVisSave = GreCreateRectRgn(
                    pwnd->rcWindow.left + rcItem.left,
                    pwnd->rcWindow.top + rcItem.top,
                    pwnd->rcWindow.left + xMenuMax,
                    pwnd->rcWindow.bottom - SYSMET(CYSIZEFRAME));
                GreExtSelectClipRgn(hdc, hrgnVisSave, RGN_COPY);
                GreSetMetaRgn(hdc);
                GreDeleteObject(hrgnVisSave);
                #else
                hrgnVisSave = CreateEmptyRgn();
                GreCopyVisRgn(hdc,hrgnVisSave);
                GreIntersectVisRect(hdc,
                        pwnd->rcWindow.left + rcItem.left,
                        pwnd->rcWindow.top + rcItem.top,
                        pwnd->rcWindow.left + xMenuMax,
                        pwnd->rcWindow.bottom - SYSMET(CYSIZEFRAME));
                #endif

                GreUnlockDisplay(gpDispInfo->hDev);
            }
        }
    }

    oldAlign = GreGetTextAlign(hdc);
    if (pItem && TestMFT(pItem, MFT_RIGHTORDER))
        GreSetTextAlign(hdc, oldAlign | TA_RTLREADING);

    hOldFont = GreSelectFont(hdc, ghMenuFont);
    GreGetViewportOrg(hdc, &ptOrg);

    if (fOn) {
        SetMFS(pItem, MFS_HILITE);
    } else {
        ClearMFS(pItem, MFS_HILITE);
    }

    if (!fSysMenuIcon
        && ((pItem->hbmp != HBMMENU_SYSTEM)
            || (TestMF(pmenu, MFISPOPUP)))) {

        if (pmenu->dwArrowsOn != MSA_OFF) {
            GreSetViewportOrg(hdc, ptOrg.x, ptOrg.y - ((int)MNGetToppItem(pmenu)->yItem), NULL);
        }

        if ((pmenu->hbrBack != NULL)
                && !mnDrawHilite(pItem)
                && !TestMFT(pItem, MFT_OWNERDRAW)) {

            /*
             * fill the background here so xxxDrawMenuItem doesn't have to fool
             * around with hbrBack
             */
            int iBkSave = GreSetBkMode(hdc, TRANSPARENT);
            MNEraseBackground (hdc, pmenu,
                    pItem->xItem, pItem->yItem,
                    pItem->cxItem, pItem->cyItem);
            GreSetBkMode(hdc, iBkSave);
        }

        xxxDrawMenuItem(hdc, pmenu, pItem, DMI_INVERT);
    }


    if (fClipped) {
        GreLockDisplay(gpDispInfo->hDev);
        GreSelectVisRgn(hdc, hrgnVisSave, SVR_DELETEOLD);
        GreUnlockDisplay(gpDispInfo->hDev);
    }

    GreSelectFont(hdc, hOldFont);
    GreSetViewportOrg(hdc, ptOrg.x, ptOrg.y, NULL);
    GreSetTextAlign(hdc, oldAlign);
    _ReleaseDC(hdc);

SendSelectMsg:
    /*
     * send select msg only if we are selecting an item.
     */
    if (fOn) {
        xxxSendMenuSelect(pwndNotify, pwnd, pmenu, itemNumber);
    }

SeeYa:
    if (pwnd != pwndNotify) {
        ThreadUnlock(&tlpwnd);
    }

    return(pItem);
}

/***************************************************************************\
*
*  xxxDrawMenuBarTemp()
*
*  This is so the control panel can let us do the work -- and make their
*  preview windows that much more accurate. The only reason I put the hwnd in
*  here is because, in the low level menu routines, we assume that an hwnd is
*  associated with the hmenu -- I didn't want to slow that code down by adding
*  NULL checks.
*
*  The SYSMET(CYMENU) with regard to the given font is returned -- this
*  way control panel can say, "The user wants this menu font (hfont) with this
*  menu height (lprc)", and we can respond "this is the height we ended up
*  using."
*
*  NOTE: It's OK to overrite lprc because this function receives a pointer
*        to the rectangle captured in NtUserDrawMenuBarTemp.
*
* History:
* 20-Sep-95     BradG       Ported from Win95 (inctlpan.c)
\***************************************************************************/

int xxxDrawMenuBarTemp(
    PWND    pwnd,
    HDC     hdc,
    LPRECT  lprc,
    PMENU   pMenu,
    HFONT   hfont)
{
    int          cyMenu;
    HFONT        hOldFont;
    HFONT        hFontSave;
    int          cxCharSave;
    int          cxOverhangSave;
    int          cyCharSave;
    int          cyLeadingSave;
    int          cyAscentSave;
    int          cySizeSave;
    PWND        pwndNotify;
    TL          tlpwndNotify;

    hFontSave       = ghMenuFont;
    cxCharSave      = gcxMenuFontChar;
    cxOverhangSave  = gcxMenuFontOverhang;
    cyCharSave      = gcyMenuFontChar;
    cyLeadingSave   = gcyMenuFontExternLeading;
    cyAscentSave    = gcyMenuFontAscent;
    cySizeSave      = SYSMET(CYMENUSIZE);

    CheckLock(pwnd);
    CheckLock(pMenu);

    ThreadLock(pMenu->spwndNotify, &tlpwndNotify);
    pwndNotify = pMenu->spwndNotify;

    cyMenu = lprc->bottom - lprc->top;

    if (hfont) {
        TEXTMETRIC  textMetrics;

        /*
         *  Compute the new menu font info if needed
         */
        ghMenuFont = hfont;
        hOldFont = GreSelectFont(HDCBITS(), ghMenuFont);
        gcxMenuFontChar = GetCharDimensions(
                HDCBITS(), &textMetrics, &gcyMenuFontChar);

        gcxMenuFontOverhang = textMetrics.tmOverhang;
        GreSelectFont(HDCBITS(), hOldFont);

#if 0   // FYI: #254237 removing KOR hack
        if (gSystemCPCharSet == HANGUL_CHARSET) {
            gcyMenuFontChar -= textMetrics.tmInternalLeading - 2;
        }
#endif

        gcyMenuFontExternLeading = textMetrics.tmExternalLeading;

        gcyMenuFontAscent = textMetrics.tmAscent + SYSMET(CYBORDER);
    }

    cyMenu -= SYSMET(CYBORDER);
    cyMenu = max(cyMenu, (gcyMenuFontChar + gcyMenuFontExternLeading + SYSMET(CYEDGE)));
    SYSMET(CYMENUSIZE) = cyMenu;
    SYSMET(CYMENU) = cyMenu + SYSMET(CYBORDER);

    /*
     *  Compute the dimensons of the menu (hope that we don't leave the
     *  USER critical section)
     */
    xxxMenuBarCompute(pMenu, pwnd, lprc->top, lprc->left, lprc->right);

    /*
     *  Draw menu border in menu color
     */
    lprc->bottom = lprc->top + pMenu->cyMenu;
    FillRect(hdc, lprc, SYSHBR(MENU));

    /*
     *  Finally, draw the menu itself.
     */
    xxxMenuDraw(hdc, pMenu);

    /*
     *  Restore the old state
     */
    ghMenuFont              = hFontSave;
    gcxMenuFontChar          = cxCharSave;
    gcxMenuFontOverhang      = cxOverhangSave;
    gcyMenuFontChar          = cyCharSave;
    gcyMenuFontExternLeading = cyLeadingSave;
    gcyMenuFontAscent        = cyAscentSave;
    SYSMET(CYMENUSIZE)      = cySizeSave;

    cyMenu = SYSMET(CYMENU);
    SYSMET(CYMENU) = cySizeSave + SYSMET(CYBORDER);

    Lock(&pMenu->spwndNotify, pwndNotify);
    ThreadUnlock(&tlpwndNotify);

    return cyMenu;
}

/***************************************************************************\
* DrawBarUnderlines
*
* Description: Shows or hides all underlines on a menu bar.
*
* History:
* 07/23/96  vadimg      created
\***************************************************************************/
void xxxDrawMenuBarUnderlines(PWND pwnd, BOOL fShow)
{
    HDC hdc;
    PMENU pmenu;
    PITEM pitem;
    ULONG i, yTop, cyTemp;
    LPWSTR psz;
    WCHAR  szMenu[MENU_STRLEN], *pchOut;
    LONG result;
    HBRUSH hbr;
    TL tlmenu;
    PTHREADINFO ptiCurrent = PtiCurrentShared();
    int xLeft;
    LPWSTR lpsz;
    SIZE extent;

    CheckLock(pwnd);

    /*
     * Bail if menu underlines are always on.
     */
    if (TEST_BOOL_ACCF(ACCF_KEYBOARDPREF) || TestEffectInvertUP(KEYBOARDCUES)
        || (GetAppCompatFlags2(VER40) & GACF2_KCOFF)) {
        return;
    }

    // if there is no menu, bail out right away

    pwnd = GetTopLevelWindow(pwnd);
    if (pwnd == NULL || !TestWF(pwnd, WFMPRESENT))
        return;
    /*
     * We don't clear WFMPRESENT when the menu is unlocked so make sure we have one
     */
    pmenu = pwnd->spmenu;
    if (pmenu == NULL) {
        return;
    }

    /*
     * set/clear the underline state. There are cases when the
     *  menu loop doesn't remove the keys from the queue; so after
     *  exiting we might get here but nothing needs to be drawn
     */
    if (fShow) {
        if (TestMF(pmenu, MFUNDERLINE)) {
            return;
        }
        hbr = SYSHBR(MENUTEXT);
        SetMF(pmenu, MFUNDERLINE);
    } else {
        if (!TestMF(pmenu, MFUNDERLINE)) {
            return;
        }
        if (pmenu->hbrBack != NULL) {
            hbr = pmenu->hbrBack;
        } else {
            hbr = SYSHBR(MENU);
        }
        ClearMF(pmenu, MFUNDERLINE);
    }

    pitem = (PITEM)pmenu->rgItems;

    hdc = _GetDCEx(pwnd, NULL, DCX_WINDOW | DCX_USESTYLE | DCX_CACHE);

    // select in the correct brush and font

    GreSelectFont(hdc, ghMenuFont);

    ThreadLock(pmenu, &tlmenu);
    for (i = 0; i < pmenu->cItems; i++, pitem++) {
        if (((psz = TextPointer(pitem->lpstr)) == NULL)
                && !TestMFT(pitem, MFT_OWNERDRAW)) {
            continue;
        }

        if (TestMFT(pitem, MFT_OWNERDRAW)) {
            GreSetViewportOrg(hdc, 0, 0, NULL);
        } else {
            GreSetViewportOrg(hdc, pitem->xItem, pitem->yItem, NULL);
        }

        // this funky xLeft and yTop calculation stolen from RealDrawMenuItem

        yTop = gcyMenuFontExternLeading;
        cyTemp = pitem->cyItem - (gcyMenuFontChar + gcyMenuFontExternLeading +
                SYSMET(CYBORDER));
        if (cyTemp > 0) {
            yTop += (cyTemp / 2);
        }

        if (fShow && TestMFS(pitem, MFS_HOTTRACK)) {
            GreSelectBrush(hdc, SYSHBR(HOTLIGHT));
        } else {
            GreSelectBrush(hdc, hbr);
        }

        if (TestMFT(pitem, MFT_OWNERDRAW)) {
            xxxSendMenuDrawItemMessage(hdc, ODA_DRAWENTIRE, pmenu, pitem, FALSE, 0);
        } else {
            TL tl;

            if (pitem->cch > MENU_STRLEN) {
                pchOut = (WCHAR*)UserAllocPool((pitem->cch+1) * sizeof(WCHAR), TAG_RTL);
                if (pchOut == NULL)
                    return;
                ThreadLockPool(ptiCurrent, pchOut, &tl);
            } else {
                pchOut = szMenu;
            }
            
            xLeft = gcxMenuFontChar;

            if ( TestMFT(pitem, MFT_RIGHTORDER) && 
                 ((lpsz  = TextPointer(pitem->lpstr)) != NULL))
            {
                xxxPSMGetTextExtent(hdc, lpsz, pitem->cch, &extent);
                xLeft += (pitem->cxItem - (gpsi->oembmi[OBI_MENUCHECK].cx + MNXSPACE) - extent.cx);
            }

            if (CALL_LPK(ptiCurrent)) {
                if (!fShow) {
                    //Becuase PSMTextOut does not use PatBlt it uses ExtTextOut.
                    GreSetTextColor(hdc, SYSRGB(MENU));
                }
                xxxPSMTextOut(hdc, xLeft, yTop, psz, pitem->cch, DT_PREFIXONLY);

            } else {
                result = GetPrefixCount(psz, pitem->cch, pchOut, pitem->cch);
                xxxDrawItemUnderline(pitem, hdc, xLeft, yTop, pchOut,
                    LOWORD(result));
            }
            if (pchOut != szMenu) {
                    ThreadUnlockAndFreePool(ptiCurrent, &tl);
            }
        }
    }
    ThreadUnlock(&tlmenu);

    _ReleaseDC(hdc);
}
