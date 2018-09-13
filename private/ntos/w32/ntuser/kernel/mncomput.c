/**************************** Module Header ********************************\
* Module Name: mncomput.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Menu Layout Calculation Routines
*
* History:
* 10-10-90 JimA       Cleanup.
* 03-18-91 IanJa      Window revalidation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


DWORD MNRecalcTabStrings(HDC, PMENU, UINT, UINT, DWORD, DWORD);

/***************************************************************************\
* xxxMNGetBitmapSize
*
* returns TRUE if measureitem was sent and FALSE if not
*
* History:
*  07-23-96 GerardoB - Added header & fixed up for 5.0
\***************************************************************************/
BOOL xxxMNGetBitmapSize(LPITEM pItem, PWND pwndNotify)
{
    MEASUREITEMSTRUCT mis;

    if (pItem->cxBmp != MNIS_MEASUREBMP) {
        return FALSE;
    }

    // Send a measure item message to the owner
    mis.CtlType = ODT_MENU;
    mis.CtlID = 0;
    mis.itemID  = pItem->wID;
    mis.itemWidth = 0;
// After scrollable menus
//    mis32.itemHeight= gcyMenuFontChar;
    mis.itemHeight= (UINT)gpsi->cySysFontChar;
    mis.itemData = pItem->dwItemData;

    xxxSendMessage(pwndNotify, WM_MEASUREITEM, 0, (LPARAM)&mis);

    pItem->cxBmp = mis.itemWidth;
    pItem->cyBmp = mis.itemHeight;

    return TRUE;
}

/***************************************************************************\
* xxxItemSize
*
* Calc the dimensions of bitmaps and strings. Loword of returned
* value contains width, high word contains height of item.
*
* History:
*  07-23-96 GerardoB - fixed up for 5.0
\***************************************************************************/
BOOL xxxMNItemSize(
    PMENU pMenu,
    PWND pwndNotify,
    HDC hdc,
    PITEM pItem,
    BOOL fPopup,
    LPPOINT lppt)
{
    BITMAP bmp;
    int width = 0;
    int height = 0;
    DWORD xRightJustify;
    LPWSTR lpMenuString;
    HFONT               hfnOld;
    int                 tcExtra;

    UNREFERENCED_PARAMETER(pMenu);

    CheckLock(pMenu);
    CheckLock(pwndNotify);

    if (!fPopup) {

        /*
         * Save off the height of the top menu bar since we will used this often
         * if the pItem is not in a popup.  (ie.  it is in the top level menu bar)
         */
        height = SYSMET(CYMENUSIZE);
    }

    hfnOld = NULL;
    if (TestMFS(pItem, MFS_DEFAULT)) {
        if (ghMenuFontDef)
            hfnOld = GreSelectFont(hdc, ghMenuFontDef);
        else {
            tcExtra = GreGetTextCharacterExtra(hdc);
            GreSetTextCharacterExtra(hdc, tcExtra + 1 + (gcxMenuFontChar / gpsi->cxSysFontChar));
        }
    }

    /*
     * Compute bitmap dimensions if needed
     */
    if (pItem->hbmp != NULL)  {
        if (pItem->hbmp == HBMMENU_CALLBACK) {
            xxxMNGetBitmapSize(pItem, pwndNotify);
        } else if (pItem->cxBmp == MNIS_MEASUREBMP) {
            if (TestMFS(pItem, MFS_CACHEDBMP)) {
                pItem->cxBmp = SYSMET(CXMENUSIZE);
                pItem->cyBmp = SYSMET(CYMENUSIZE);
                if (pItem->hbmp == HBMMENU_SYSTEM) {
                    pItem->cxBmp += SYSMET(CXEDGE);
                    /*
                     * Chicago/Memphis only stretch the width,
                     * not the height.  NT Bug 124779.  FritzS
                     */
                 //  pItem->cyBmp += SYSMET(CXEDGE);
                }
            } else {
                if (GreExtGetObjectW(pItem->hbmp, sizeof(BITMAP), (LPSTR)&bmp)) {
                    pItem->cxBmp = bmp.bmWidth;
                    pItem->cyBmp = bmp.bmHeight;
                } else {
                    /*
                     * If the bitmap is not useable, this is as good a default
                     * as any.
                     */
                    pItem->cxBmp = SYSMET(CXMENUSIZE);
                    pItem->cyBmp = SYSMET(CYMENUSIZE);
                }
            }
        }
        width = pItem->cxBmp;
        /*
         * Remember the max bitmap width to align the text in all items.
         */
        pMenu->cxTextAlign = max(pMenu->cxTextAlign, (DWORD)width);
        /*
         * In menu bars, we force the item to be at least CYMNSIZE.
         * Fixes many, many problems w/ apps that fake own MDI.
         */
        if (fPopup) {
            height = pItem->cyBmp;
        } else {
            height = max((int)pItem->cyBmp, height);
        }
    } else if (TestMFT(pItem, MFT_OWNERDRAW)) {
        // This is an ownerdraw item -- the width and height are stored in
        // cxBmp and cyBmp
        xxxMNGetBitmapSize(pItem, pwndNotify);
        width = pItem->cxBmp;
        //
        // Ignore height with menu bar now--that's set by user.
        //
        if (fPopup) {
            height = pItem->cyBmp;
            // If this item has a popup (hierarchical) menu associated with it, then
            // reserve room for the bitmap that tells the user that a hierarchical
            // menu exists here.
            // B#2966, t-arthb

            UserAssert(fPopup == (TestMF(pMenu, MFISPOPUP) != 0));

            width = width + (gcxMenuFontChar << 1);
        }
    }

    if ((pItem->lpstr != NULL) && (!TestMFT(pItem, MFT_OWNERDRAW)) ) {
        SIZE size;

        /*
         * This menu item contains a string
         */

        /*
         * We want to keep the menu bar height if this isn't a popup.
         */
        if (fPopup) {
            /* The thickness of mnemonic underscore is CYBORDER and the gap
             * between the characters and the underscore is another CYBORDER
             */
            height = max(height, gcyMenuFontChar + gcyMenuFontExternLeading + SYSMET(CYEDGE));
        }

        lpMenuString = TextPointer(pItem->lpstr);
        xRightJustify = FindCharPosition(lpMenuString, TEXT('\t'));

        xxxPSMGetTextExtent(hdc, lpMenuString, xRightJustify, &size);

        if (width) {
            width += MNXSPACE + size.cx;
        } else {
            width =  size.cx;
        }
    }

    if (fPopup && !TestMFT(pItem, MFT_OWNERDRAW)) {
        /*
         *  Add on space for checkmark, then horz spacing for default & disabled
         *   plus some left margin.
         */
        if (TestMF(pMenu, MNS_CHECKORBMP) || !TestMF(pMenu, MNS_NOCHECK)) {
            width += gpsi->oembmi[OBI_MENUCHECK].cx;
        }
        width += MNXSPACE + MNLEFTMARGIN + 2;
        height += 2;
    }

    if (TestMFS(pItem, MFS_DEFAULT)) {
        if (hfnOld)
            GreSelectFont(hdc, hfnOld);
        else
            GreSetTextCharacterExtra(hdc, tcExtra);
    }

    /*
     * Loword contains width, high word contains height of item.
     */
    lppt->x = width;
    lppt->y = height;

    return(TestMFT(pItem, MFT_OWNERDRAW));
}
/***************************************************************************\
* xxxMenuCompute2
*
* !
*
* History:
\***************************************************************************/

int xxxMNCompute(
    PMENU pMenu,
    PWND pwndNotify,
    DWORD yMenuTop,
    DWORD xMenuLeft,
    DWORD cxMax,
    LPDWORD lpdwMenuHeight)
{
    UINT         cItem;
    DWORD        cxItem;
    DWORD        cyItem;
    DWORD        cyItemKeep;
    DWORD        yPopupTop;
    INT          cMaxWidth;
    DWORD        cMaxHeight;
    UINT         cItemBegCol;
    DWORD        temp;
    int          ret;
    PITEM        pCurItem;
    POINT        ptMNItemSize;
    BOOL         fOwnerDrawItems;
    BOOL         fMenuBreak;
    LPWSTR       lpsz;
    BOOL         fPopupMenu;
    DWORD        menuHeight = 0;
    HDC          hdc;
    HFONT        hOldFont;
    PTHREADINFO  ptiCurrent = PtiCurrent();
    BOOL         fStringAndBitmapItems;

    CheckLock(pMenu);
    CheckLock(pwndNotify);

    /*
     * Whoever computes the menu, becomes the owner.
     */
    if (pwndNotify != pMenu->spwndNotify) {
        Lock(&pMenu->spwndNotify, pwndNotify);
    }


    if (lpdwMenuHeight != NULL)
        menuHeight = *lpdwMenuHeight;

    /*
     * Empty menus have a height of zero.
     */
    ret = 0;
    if (pMenu->cItems == 0)
        return ret;

    hdc = _GetDCEx(NULL, NULL, DCX_WINDOW | DCX_CACHE);
    hOldFont = GreSelectFont(hdc, ghMenuFont);

    /*
     * Try to make a non-multirow menu first.
     */
    pMenu->fFlags &= (~MFMULTIROW);

    fPopupMenu = TestMF(pMenu, MFISPOPUP);

    if (fPopupMenu) {

        /*
         * Reset the menu bar height to 0 if this is a popup since we are
         * being called from menu.c MN_SIZEWINDOW.
         */
        menuHeight = 0;
    } else if (pwndNotify != NULL) {
        pMenu->cxMenu = cxMax;
    }

    /*
     * Initialize the computing variables.
     */
    cMaxWidth = cyItemKeep = 0L;
    cItemBegCol = 0;

    cyItem = yPopupTop = yMenuTop;
    cxItem = xMenuLeft;

    pCurItem = (PITEM)&pMenu->rgItems[0];
    /*
     * cxTextAlign is used to align the text in all items; this is useful
     *  in popup menus that mix text only items with bitmap-text items. It's
     *  set to the max bitmap width plus some spacing.
     * Do this for new menus wich use string AND bitmaps on the same item
     */
    fStringAndBitmapItems = FALSE;
    pMenu->cxTextAlign = 0;

    /*
     * Process each item in the menu.
     */
    fOwnerDrawItems = FALSE;
    for (cItem = 0; cItem < pMenu->cItems; cItem++) {

        /*
         * If it's not a separator, find the dimensions of the object.
         */
        if (TestMFT(pCurItem, MFT_SEPARATOR) &&
                ( !TestMFT(pCurItem, MFT_OWNERDRAW) ||
                  (LOWORD(ptiCurrent->dwExpWinVer) < VER40)) ) {
            /*
            * If version is less than 4.0  don't test the MFT_OWNERDRAW
            * flag. Bug 21922; App MaxEda has both separator and Ownerdraw
            * flags on. In 3.51 we didn't test the OwnerDraw flag
            */

            //
            // This is a separator.  It's drawn as wide as the menu area,
            // leaving some space above and below.  Since the menu area is
            // the max of the items' widths, we set our width to 0 so as not
            // to affect the result.
            //
            pCurItem->cxItem = 0;
            pCurItem->cyItem = SYSMET(CYMENUSIZE) / 2;


        } else {
            /*
             * Are we using NT5 strings and bitmaps?
             */
            fStringAndBitmapItems |= ((pCurItem->hbmp != NULL) && (pCurItem->lpstr != NULL));
            /*
             * Get the item's X and Y dimensions.
             */
            if (xxxMNItemSize(pMenu, pwndNotify, hdc, pCurItem, fPopupMenu, &ptMNItemSize))
                fOwnerDrawItems = TRUE;

            pCurItem->cxItem = ptMNItemSize.x;
            pCurItem->cyItem = ptMNItemSize.y;
            if (!fPopupMenu && ((pCurItem->hbmp == NULL) || (pCurItem->lpstr != NULL))) {
                pCurItem->cxItem += gcxMenuFontChar * 2;
            }
        }

        if (menuHeight != 0)
            pCurItem->cyItem = menuHeight;

        /*
         * If this is the first item, initialize cMaxHeight.
         */
        if (cItem == 0)
            cMaxHeight = pCurItem->cyItem;

        /*
         * Is this a Pull-Down menu?
         */
        if (fPopupMenu) {

            /*
             * If this item has a break or is the last item...
             */
            if ((fMenuBreak = TestMFT(pCurItem, MFT_BREAK)) ||
                pMenu->cItems == cItem + (UINT)1) {

                /*
                 * Keep cMaxWidth around if this is not the last item.
                 */
                temp = cMaxWidth;
                if (pMenu->cItems == cItem + (UINT)1) {
                    if ((int)(pCurItem->cxItem) > cMaxWidth)
                        temp = pCurItem->cxItem;
                }

                /*
                 * Get new width of string from RecalcTabStrings.
                 */
                temp = MNRecalcTabStrings(hdc, pMenu, cItemBegCol,
                        (UINT)(cItem + (fMenuBreak ? 0 : 1)), temp, cxItem);

                /*
                 * If this item has a break, account for it.
                 */
                if (fMenuBreak) {
                    //
                    // Add on space for the etch and a border on either side.
                    // NOTE:  For old apps that do weird stuff with owner
                    // draw, keep 'em happy by adding on the same amount
                    // of space we did in 3.1.
                    //
                    if (fOwnerDrawItems && !TestWF(pwndNotify, WFWIN40COMPAT))
                        cxItem = temp + SYSMET(CXBORDER);
                    else
                        cxItem = temp + 2 * SYSMET(CXEDGE);

                    /*
                     * Reset the cMaxWidth to the current item.
                     */
                    cMaxWidth = pCurItem->cxItem;

                    /*
                     * Start at the top.
                     */
                    cyItem = yPopupTop;

                    /*
                     * Save the item where this column begins.
                     */
                    cItemBegCol = cItem;

                    /*
                     * If this item is also the last item, recalc for this
                     * column.
                     */
                    if (pMenu->cItems == (UINT)(cItem + 1)) {
                        temp = MNRecalcTabStrings(hdc, pMenu, cItem,
                                (UINT)(cItem + 1), cMaxWidth, cxItem);
                    }
                }

                /*
                 * If this is the last entry, supply the width.
                 */
                if (pMenu->cItems == cItem + (UINT)1)
                    pMenu->cxMenu = temp;
            }

            pCurItem->xItem = cxItem;
            pCurItem->yItem = cyItem;

            cyItem += pCurItem->cyItem;

            if (cyItemKeep < cyItem)
                cyItemKeep = cyItem;

        } else {

            /*
             * This a Top Level menu, not a Pull-Down.
             */

            /*
             * Adjust right aligned items before testing for multirow
             */
            if (pCurItem->lpstr != NULL) {
                lpsz = TextPointer(pCurItem->lpstr);
                if ((lpsz != NULL) && (*lpsz == CH_HELPPREFIX)) {
                    pCurItem->cxItem -= gcxMenuFontChar;
                }
            }


            /*
             * If this is a new line or a menu break.
             */
            if ((TestMFT(pCurItem, MFT_BREAK)) ||
                    (((cxItem + pCurItem->cxItem + gcxMenuFontChar) >
                    pMenu->cxMenu) && (cItem != 0))) {
                cyItem += cMaxHeight;

                cxItem = xMenuLeft;
                cMaxHeight = pCurItem->cyItem;
                pMenu->fFlags |= MFMULTIROW;
            }

            pCurItem->yItem = cyItem;

            pCurItem->xItem = cxItem;
            cxItem += pCurItem->cxItem;
        }

        if (cMaxWidth < (int)(pCurItem->cxItem))
            cMaxWidth = pCurItem->cxItem;

        if (cMaxHeight != pCurItem->cyItem) {
            if (cMaxHeight < pCurItem->cyItem)
                cMaxHeight = pCurItem->cyItem;

            if (!fPopupMenu)
                menuHeight = cMaxHeight;
        }

        if (!fPopupMenu)
            cyItemKeep = cyItem + cMaxHeight;

        pCurItem++;

    } /* of for loop */
    /*
     * Determine where the strings should be drawn so they are aligned
     * The alignment is only for popup (vertical) menus (see xxxRealDrawMenuItem)
     * The actual space depends on the MNS_NOCHECK and MNS_CHECKORBMP styles
     * Multicolumn menus don't get aligment (now that we have scrollbars, multicolum is out)
     */
    if (!fStringAndBitmapItems || (cItemBegCol != 0)) {
        pMenu->cxTextAlign = 0;
    } else if (TestMF(pMenu, MNS_NOCHECK)) {
        pMenu->cxTextAlign += MNXSPACE;
    } else if (TestMF(pMenu, MNS_CHECKORBMP)) {
        pMenu->cxTextAlign = max(pMenu->cxTextAlign, (UINT)gpsi->oembmi[OBI_MENUCHECK].cx);
        pMenu->cxTextAlign += MNXSPACE;
    } else {
        pMenu->cxTextAlign += gpsi->oembmi[OBI_MENUCHECK].cx + MNXSPACE;
    }
    /*
     * Add the left margin
     */
    if (pMenu->cxTextAlign != 0) {
        pMenu->cxTextAlign += MNLEFTMARGIN;
    }


    if (cItemBegCol && pMenu->cItems &&
        TestMFT(((PITEM)&pMenu->rgItems[0]), MFT_RIGHTJUSTIFY)) {
        //
        // multi-column, if we are in RtoL mode, reverse the columns
        //
        pCurItem = (PITEM)&pMenu->rgItems[0];

        for (cItem = 0; cItem < pMenu->cItems; cItem++) {
            pCurItem->xItem = pMenu->cxMenu -
                              (pCurItem->xItem + pCurItem->cxItem);
            pCurItem++;
        }
    }

    GreSelectFont(hdc, hOldFont);
    _ReleaseDC(hdc);

    pMenu->cyMenu = cyItemKeep - yMenuTop;
    ret = pMenu->cyMenu;

    if (lpdwMenuHeight != NULL)
        *lpdwMenuHeight = menuHeight;

    return ret;
}

/***************************************************************************\
* MBC_RightJustifyMenu
*
* !
*
* History:
\***************************************************************************/

void MBC_RightJustifyMenu(
    PMENU pMenu)
{
    PITEM pItem;
    int cItem;
    int iFirstRJItem = MFMWFP_NOITEM;
    DWORD xMenuPos;
    DWORD  yPos;
    DWORD  xPosStart;
    DWORD  xPosEnd;
    int    cItemEnd;
    int    cItemStart;
    BOOL   bIsWin95;

    //
    // need to compensate for MDI menus.  need to do all here as Win31/Hebrew did
    // this.  also screwed up computation, anything non-text was not moved.
    //
    if (pMenu->cItems == 0)
        return;

    pItem = (PITEM)&pMenu->rgItems[0];
    cItemStart = 0;

    if (TestMF(pMenu,MFRTL)) {
        bIsWin95 = TestWF(pMenu->spwndNotify, WFWIN40COMPAT);

        while( cItemStart < (int)pMenu->cItems ) {
            if (bIsWin95) {  //(not time critical)
                //
                // deal with fake MDI dude.
                //
                if (!cItemStart && IsMDIItem(pItem))
                    goto StillFindStart;
                else
                    break;
            }

            if( TestMFT(pItem, MFT_BITMAP) ) {
                if ( pItem->hbmp > (HBITMAP)HBMMENU_MAX )
                    break;
                else
                    goto StillFindStart;
            }

            if (!TestMFT(pItem, MFT_OWNERDRAW) )
                break;

StillFindStart:
            cItemStart++;
            pItem = pMenu->rgItems + cItemStart;
        }
        //
        // anything before cItems should be left where it is! Now need to find
        // the last item to fool with.
        //
        cItemEnd = pMenu->cItems - 1;
        pItem   = pMenu->rgItems + cItemEnd;

        while( cItemEnd > cItemStart ) {
            if (bIsWin95) {
                //
                // fake mdi dudes
                //
                if (IsMDIItem(pItem))
                    goto StillFindEnd;
                else
                    break;
            }
            if ( !TestMFT(pItem, MFT_BITMAP) && !TestMFT(pItem, MFT_OWNERDRAW) )
                break;
StillFindEnd:
            cItemEnd --;
            pItem = pMenu->rgItems + cItemEnd;
        }

        yPos      = pMenu->rgItems[0].yItem;
        xMenuPos  = pMenu->cxMenu ;
        xPosStart = xMenuPos;              // for 2nd row onward
        xPosEnd   = pMenu->rgItems[cItemStart].xItem ;

        for (cItem = pMenu->cItems-1; cItem > cItemEnd; cItem--) {
            //
            // force any MDI dudes back to the top line again.
            //
            pItem        = pMenu->rgItems + cItem;
            xMenuPos      =
            pItem->xItem = xMenuPos - pItem->cxItem;
            pItem->yItem = yPos;
        }

        for( cItem = cItemStart; cItem <= cItemEnd; cItem++ ) {
            pItem = pMenu->rgItems + cItem;
            if (xMenuPos - pItem->cxItem > xPosEnd) {
                xMenuPos -= pItem->cxItem;
            } else {
                xMenuPos = xPosStart - pItem->cxItem;
                yPos     += pItem->cyItem;
                xPosEnd  = 0;
            }
            pItem->xItem = xMenuPos;
            pItem->yItem = yPos;
        }
    } else {
        // B#4055
        // Use signed arithmetic so comparison cItem >= iFirstRJItem won't
        // cause underflow.
        for (cItem = 0; cItem < (int)pMenu->cItems; cItem++) {
            // Find the first item which is right justified.
            if (TestMFT((pMenu->rgItems + cItem), MFT_RIGHTJUSTIFY)) {
                iFirstRJItem = cItem;
                xMenuPos = pMenu->cxMenu + pMenu->rgItems[0].xItem;
                for (cItem = (int)pMenu->cItems - 1; cItem >= iFirstRJItem; cItem--) {
                    pItem = pMenu->rgItems + cItem;
                    xMenuPos -= pItem->cxItem;
                    if (pItem->xItem < xMenuPos)
                        pItem->xItem = xMenuPos;
                }
                return;
            }
        }
    }
}

/***************************************************************************\
* xxxMenuBarCompute
*
* returns the height of the menubar menu. yMenuTop, xMenuLeft, and
* cxMax are used when computing the height/width of top level menu bars in
* windows.
*
*
* History:
\***************************************************************************/

int xxxMenuBarCompute(
    PMENU pMenu,
    PWND pwndNotify,
    DWORD yMenuTop,
    DWORD xMenuLeft,
    int cxMax)
{
    int size;
    /* menuHeight is set by MNCompute when dealing with a top level menu and
     * not all items in the menu bar have the same height.  Thus, by setting
     * menuHeight, MNCompute is called a second time to set every item to the
     * same height.  The actual value stored in menuHeight is the maximum
     * height of all the menu bar items
     */
    DWORD menuHeight = 0;

    CheckLock(pwndNotify);
    CheckLock(pMenu);

    size = xxxMNCompute(pMenu, pwndNotify, yMenuTop, xMenuLeft, cxMax, &menuHeight);

    if (!TestMF(pMenu, MFISPOPUP)) {
        if (menuHeight != 0) {

            /*
             * Add a border for the multi-row case.
             */
            size = xxxMNCompute(pMenu, pwndNotify, yMenuTop, xMenuLeft,
                    cxMax, &menuHeight);
        }

        /*
         * Right justification of HELP items is only needed on top level
         * menus.
         */
        MBC_RightJustifyMenu(pMenu);
    }

    /*
     * There's an extra border underneath the menu bar, if it's not empty!
     */
    return(size ? size + SYSMET(CYBORDER) : size);
}

/***************************************************************************\
* xxxRecomputeMenu
*
* !
*
* History:
\***************************************************************************/

void xxxMNRecomputeBarIfNeeded(
    PWND pwndNotify,
    PMENU pMenu)
{
    int cxFrame;
    int cyFrame;

    UserAssert(!TestMF(pMenu, MFISPOPUP));

    CheckLock(pwndNotify);
    CheckLock(pMenu);

    if (!TestMF(pMenu, MFSYSMENU)
        && ((pMenu->spwndNotify != pwndNotify) || !pMenu->cxMenu || !pMenu->cyMenu)) {
        int cBorders;

        cBorders = GetWindowBorders(pwndNotify->style, pwndNotify->ExStyle, TRUE, FALSE);
        cxFrame = cBorders * SYSMET(CXBORDER);
        cyFrame = cBorders * SYSMET(CYBORDER);

        cyFrame += GetCaptionHeight(pwndNotify);

        // The width passed in this call was larger by cxFrame;
        // Fix for Bug #11466 - Fixed by SANKAR - 01/06/92 --
        xxxMenuBarCompute(pMenu, pwndNotify, cyFrame, cxFrame,
                (pwndNotify->rcWindow.right - pwndNotify->rcWindow.left) - cxFrame * 2);
    }
}

/***************************************************************************\
* RecalcTabStrings
*
* !
*
* History:
*   10-11-90 JimA       Translated from ASM
\***************************************************************************/

DWORD MNRecalcTabStrings(
    HDC hdc,
    PMENU pMenu,
    UINT iBeg,
    UINT iEnd,
    DWORD xTab,
    DWORD hCount)
{
    UINT i;
    UINT    cOwnerDraw;
    int adx;
    int     maxWidth = 0;
    int     cx;
    PITEM pItem;
    CheckLock(pMenu);

    xTab += hCount;

    if ((iBeg >= pMenu->cItems) || (iBeg > iEnd))
        goto SeeYa;

    cOwnerDraw = 0;

    for (i = iBeg, pItem = pMenu->rgItems + iBeg; i < iEnd; pItem++, i++) {
        adx = 0;

        /*
         * Subtract hCount to make dxTab relative to start of column for
         * multi-column menus.
         */

        pItem->dxTab = xTab - hCount;

        // Skip non-string or empty string items
        if ((pItem->lpstr != NULL) && !TestMFT(pItem, MFT_OWNERDRAW)) {
            LPWSTR   lpString = TextPointer(pItem->lpstr);
            int     tp;
            SIZE size;

            // Are there any tabs?
            tp = FindCharPosition(lpString, TEXT('\t'));
            if (tp < (int) pItem->cch) {
                PTHREADINFO ptiCurrent = PtiCurrentShared();

                if (CALL_LPK(ptiCurrent)) {
                    xxxClientGetTextExtentPointW(hdc, lpString + tp + 1,
                          pItem->cch - tp - 1, &size);
                } else {
                    GreGetTextExtentW(hdc, lpString + tp + 1,
                          pItem->cch - tp - 1, &size, GGTE_WIN3_EXTENT);
                }
                adx = gcxMenuFontChar + size.cx;
            }
        } else if (TestMFT(pItem, MFT_OWNERDRAW))
            cOwnerDraw++;

        adx += xTab;

        if (adx > maxWidth)
            maxWidth = adx;

    }

    /*
     * Add on space for hierarchical arrow.  So basically, popup menu items
     * can have 4 columns:
     *      (1) Checkmark
     *      (2) Text
     *      (3) Tabbed text for accel
     *      (4) Hierarchical arrow
     *
     * But, we only do this if at least one item isn't ownerdraw
     *  and if there's at least one submenu in the popup.
     */
    if (cOwnerDraw != (iEnd - iBeg)) {
        maxWidth += gcxMenuFontChar + gpsi->oembmi[OBI_MENUCHECK].cx;
    }

    cx = maxWidth - hCount;

    for (i = iBeg, pItem = pMenu->rgItems + iBeg; i < iEnd; pItem++, i++)
        pItem->cxItem = cx;

SeeYa:
    return(maxWidth);
}
/***************************************************************************\
* GetMenuPwnd
*
* This function is used by xxxGetMenuItemRect and xxxMenuItemFromPoint
*  which expect a pointer to the  menu window for popup menus.
* In 4.0, apps had to go the extra mile to find the menu window; but this
*  is bogus since menu windows are an internally thing not directly exposed
*  to applications.
*
* 08/19/97  GerardoB    Created
\***************************************************************************/
PWND GetMenuPwnd (PWND pwnd, PMENU pmenu)
{
    if (TestMF(pmenu, MFISPOPUP)) {
        if ((pwnd == NULL) || (GETFNID(pwnd) != FNID_MENU)) {
            PPOPUPMENU ppopup = MNGetPopupFromMenu(pmenu, NULL);
            if (ppopup != NULL) {
                UserAssert(ppopup->spmenu == pmenu);
                pwnd = ppopup->spwndPopupMenu;
            }
        }
    }
    return pwnd;
}
// ============================================================================
//
//  GetMenuItemRect()
//
// ============================================================================
BOOL xxxGetMenuItemRect(PWND pwnd, PMENU pMenu, UINT uIndex, LPRECT lprcScreen)
{
    PITEM  pItem;
    int     dx, dy;

    CheckLock(pwnd);
    CheckLock(pMenu);

    SetRectEmpty(lprcScreen);

    if (uIndex >= pMenu->cItems)
        return(FALSE);

    /*
     * Raid #315084: Compatiblity with NT4/Win95/98
     *
     * WordPerfect does a long complex way to calc the menu rect
     * by calling this API. It calls GetMenuItemRect() with the app's
     * window.
     */
    if (pwnd == NULL || TestWF(pwnd, WFWIN50COMPAT)) {
        pwnd = GetMenuPwnd(pwnd, pMenu);
    }

    /*
     * If no pwnd, no go.
     * IMPORTANT: for MFISPOPUP we might get a different pwnd but we don't lock
     *   it because we won't call back
     */
    if (pwnd == NULL) {
        return FALSE;
    }

    if (TestMF(pMenu, MFISPOPUP)) {
        dx = pwnd->rcClient.left;
        dy = pwnd->rcClient.top;
    } else {
        xxxMNRecomputeBarIfNeeded(pwnd, pMenu);

        dx = pwnd->rcWindow.left;
        dy = pwnd->rcWindow.top;
    }

    if (uIndex >= pMenu->cItems)
        return(FALSE);

    pItem = pMenu->rgItems + uIndex;

    lprcScreen->right   = pItem->cxItem;
    lprcScreen->bottom  = pItem->cyItem;

    OffsetRect(lprcScreen, dx + pItem->xItem, dy + pItem->yItem);
    return(TRUE);
}

/***************************************************************************\
*
*  MenuItemFromPoint()
*
\***************************************************************************/
UINT MNItemHitTest(PMENU pMenu, PWND pwnd, POINT pt);
int xxxMenuItemFromPoint(PWND pwnd, PMENU pMenu, POINT ptScreen)
{
    CheckLock(pwnd);
    CheckLock(pMenu);

    /*
     * If no pwnd, no go.
     * IMPORTANT: for MFISPOPUP we might get a different pwnd but we don't lock
     *   it because we won't call back
     */
    pwnd = GetMenuPwnd(pwnd, pMenu);
    if (pwnd == NULL) {
        return MFMWFP_NOITEM;
    }

    if (!TestMF(pMenu, MFISPOPUP)) {
        xxxMNRecomputeBarIfNeeded(pwnd, pMenu);
    }

    return(MNItemHitTest(pMenu, pwnd, ptScreen));
}


PMENU MakeMenuRtoL(PMENU pMenu, BOOL bRtoL)
{
    PITEM  pItem;
    int    cItem;

    if (bRtoL)
        SetMF(pMenu,MFRTL);
    else
        ClearMF(pMenu,MFRTL);

    for (cItem = 0; cItem < (int)pMenu->cItems; cItem++) {
        pItem = pMenu->rgItems + cItem;
        if (bRtoL) {
            SetMFT(pItem, MFT_RIGHTJUSTIFY);
            SetMFT(pItem, MFT_RIGHTORDER);
        } else {
            ClearMFT(pItem, MFT_RIGHTJUSTIFY);
            ClearMFT(pItem, MFT_RIGHTORDER);
        }
        if (pItem->spSubMenu)
            MakeMenuRtoL(pItem->spSubMenu, bRtoL);
    }
    return pMenu;
}
