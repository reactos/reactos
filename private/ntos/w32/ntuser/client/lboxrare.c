/**************************** Module Header ********************************\
* Module Name: lboxrare.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Infrequently Used List Box Routines
*
* History:
* ??-???-???? ianja    Ported from Win 3.0 sources
* 14-Feb-1991 mikeke   Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

extern LOOKASIDE ListboxLookaside;

/***************************************************************************\
* LBSetCItemFullMax
*
* History:
* 03-04-92 JimA             Ported from Win 3.1 sources.
\***************************************************************************/

void LBSetCItemFullMax(
    PLBIV plb)
{
    if (plb->OwnerDraw != OWNERDRAWVAR) {
        plb->cItemFullMax = CItemInWindow(plb, FALSE);
    } else if (plb->cMac < 2) {
        plb->cItemFullMax = 1;
    } else {
        int     height;
        RECT    rect;
        int     i;
        int     j = 0;

        _GetClientRect(plb->spwnd, &rect);
        height = rect.bottom;

        plb->cItemFullMax = 0;
        for (i = plb->cMac - 1; i >= 0; i--, j++) {
            height -= LBGetVariableHeightItemHeight(plb, i);

            if (height < 0) {
                plb->cItemFullMax = j;
                break;
            }
        }
        if (!plb->cItemFullMax)
            plb->cItemFullMax = j;
    }
}

/***************************************************************************\
* xxxCreateLBox
*
* History:
* 16-Apr-1992 beng      Added LBS_NODATA
\***************************************************************************/

LONG xxxLBCreate(
    PLBIV plb, PWND pwnd, LPCREATESTRUCT lpcs)
{
    UINT style;
    MEASUREITEMSTRUCT measureItemStruct;
    TL tlpwndParent;
    HDC hdc;

    /*
     * Once we make it here, nobody can change the ownerdraw style bits
     * by calling SetWindowLong. The window style must match the flags in plb
     *
     */
    plb->fInitialized = TRUE;

    style = pwnd->style;

    /*
     * Compatibility hack.
     */
    if (pwnd->spwndParent == NULL)
        Lock(&(plb->spwndParent), _GetDesktopWindow());
    else
        Lock(&(plb->spwndParent), REBASEPWND(pwnd, spwndParent));

    /*
     * Break out the style bits
     */
    plb->fRedraw = ((style & LBS_NOREDRAW) == 0);
    plb->fDeferUpdate = FALSE;
    plb->fNotify = (UINT)((style & LBS_NOTIFY) != 0);
    plb->fVertBar = ((style & WS_VSCROLL) != 0);
    plb->fHorzBar = ((style & WS_HSCROLL) != 0);

    if (!TestWF(pwnd, WFWIN40COMPAT)) {
        // for 3.x apps, if either scroll bar was specified, the app got BOTH
        if (plb->fVertBar || plb->fHorzBar)
            plb->fVertBar = plb->fHorzBar = TRUE;
    }

    plb->fRtoLReading = (TestWF(pwnd, WEFRTLREADING) != 0);
    plb->fRightAlign  = (TestWF(pwnd, WEFRIGHT) != 0);
    plb->fDisableNoScroll = ((style & LBS_DISABLENOSCROLL) != 0);

    plb->fSmoothScroll = TRUE;

    /*
     * LBS_NOSEL gets priority over any other selection style.  Next highest
     * priority goes to LBS_EXTENDEDSEL. Then LBS_MULTIPLESEL.
     */
    if (TestWF(pwnd, WFWIN40COMPAT) && (style & LBS_NOSEL)) {
        plb->wMultiple = SINGLESEL;
        plb->fNoSel = TRUE;
    } else if (style & LBS_EXTENDEDSEL) {
        plb->wMultiple = EXTENDEDSEL;
    } else {
        plb->wMultiple = (UINT)((style & LBS_MULTIPLESEL) ? MULTIPLESEL : SINGLESEL);
    }

    plb->fNoIntegralHeight = ((style & LBS_NOINTEGRALHEIGHT) != 0);
    plb->fWantKeyboardInput = ((style & LBS_WANTKEYBOARDINPUT) != 0);
    plb->fUseTabStops = ((style & LBS_USETABSTOPS) != 0);
    if (plb->fUseTabStops) {

        /*
         * Set tab stops every <default> dialog units.
         */
        LBSetTabStops(plb, 0, NULL);
    }
    plb->fMultiColumn = ((style & LBS_MULTICOLUMN) != 0);
    plb->fHasStrings = TRUE;
    plb->iLastSelection = -1;
    plb->iMouseDown = -1;  /* Anchor point for multi selection */
    plb->iLastMouseMove = -1;

    /*
     * Get ownerdraw style bits
     */
    if ((style & LBS_OWNERDRAWFIXED)) {
        plb->OwnerDraw = OWNERDRAWFIXED;
    } else if ((style & LBS_OWNERDRAWVARIABLE) && !plb->fMultiColumn) {
        plb->OwnerDraw = OWNERDRAWVAR;

        /*
         * Integral height makes no sense with var height owner draw
         */
        plb->fNoIntegralHeight = TRUE;
    }

    if (plb->OwnerDraw && !(style & LBS_HASSTRINGS)) {

        /*
         * If owner draw, do they want the listbox to maintain strings?
         */
        plb->fHasStrings = FALSE;
    }

    /*
     * If user specifies sort and not hasstrings, then we will send
     * WM_COMPAREITEM messages to the parent.
     */
    plb->fSort = ((style & LBS_SORT) != 0);

    /*
     * "No data" lazy-eval listbox mandates certain other style settings
     */
    plb->fHasData = TRUE;

    if (style & LBS_NODATA) {
        if (plb->OwnerDraw != OWNERDRAWFIXED || plb->fSort || plb->fHasStrings) {
            RIPERR0(ERROR_INVALID_FLAGS, RIP_WARNING,
                 "NODATA listbox must be OWNERDRAWFIXED, w/o SORT or HASSTRINGS");
        } else {
            plb->fHasData = FALSE;
        }
    }

    plb->dwLocaleId = GetThreadLocale();

    /*
     * Check if this is part of a combo box
     */
    if ((style & LBS_COMBOBOX) != 0) {

        /*
         * Get the pcbox structure contained in the parent window's extra data
         * pointer.  Check cbwndExtra to ensure compatibility with SQL windows.
         */
        if (plb->spwndParent->cbwndExtra != 0)
            plb->pcbox = ((PCOMBOWND)(plb->spwndParent))->pcbox;
    }

    /*
     * No need to set these to 0 since that was done for us when we Alloced
     * the PLBIV.
     */

    /*
     * plb->rgpch       = (PBYTE)0;
     */

    /*
     * plb->iSelBase    = plb->iTop = 0;
     */

    /*
     * plb->fMouseDown  = FALSE;
     */

    /*
     * plb->fCaret      = FALSE;
     */

    /*
     * plb->fCaretOn    = FALSE;
     */

    /*
     * plb->maxWidth    = 0;
     */

    plb->iSel = -1;

    plb->hdc        = NULL;

    /*
     * Set the keyboard state so that when the user keyboard clicks he selects
     * an item.
     */
    plb->fNewItemState = TRUE;

    InitHStrings(plb);

    if (plb->fHasStrings && plb->hStrings == NULL) {
        return -1L;
    }

    hdc = NtUserGetDC(HWq(pwnd));
    plb->cxChar = GdiGetCharDimensions(hdc, NULL, &plb->cyChar);
    NtUserReleaseDC(HWq(pwnd), hdc);

    if (plb->cxChar == 0) {
        RIPMSG0(RIP_WARNING, "xxxLBCreate: GdiGetCharDimensions failed");
        plb->cxChar = gpsi->cxSysFontChar;
        plb->cyChar = gpsi->cySysFontChar;
    }

    if (plb->OwnerDraw == OWNERDRAWFIXED) {

        /*
         * Query for item height only if we are fixed height owner draw.  Note
         * that we don't care about an item's width for listboxes.
         */
        measureItemStruct.CtlType = ODT_LISTBOX;
        measureItemStruct.CtlID = PtrToUlong(pwnd->spmenu);

        /*
         * System font height is default height
         */
        measureItemStruct.itemHeight = plb->cyChar;
        measureItemStruct.itemWidth = 0;
        measureItemStruct.itemData = 0;

        /*
         * IanJa: #ifndef WIN16 (32-bit Windows), plb->id gets extended
         * to LONG wParam automatically by the compiler
         */
        ThreadLock(plb->spwndParent, &tlpwndParent);
        SendMessage(HW(plb->spwndParent), WM_MEASUREITEM,
                measureItemStruct.CtlID,
                (LPARAM)&measureItemStruct);
        ThreadUnlock(&tlpwndParent);

        /*
         * Use default height if given 0.  This prevents any possible future
         * div-by-zero errors.
         */
        if (measureItemStruct.itemHeight)
            plb->cyChar = measureItemStruct.itemHeight;


        if (plb->fMultiColumn) {

            /*
             * Get default column width from measure items struct if we are a
             * multicolumn listbox.
             */
            plb->cxColumn = measureItemStruct.itemWidth;
        }
    } else if (plb->OwnerDraw == OWNERDRAWVAR)
        plb->cyChar = 0;


    if (plb->fMultiColumn) {

        /*
         * Set these default values till we get the WM_SIZE message and we
         * calculate them properly.  This is because some people create a
         * 0 width/height listbox and size it later.  We don't want to have
         * problems with invalid values in these fields
         */
        if (plb->cxColumn <= 0)
            plb->cxColumn = 15 * plb->cxChar;
        plb->numberOfColumns = plb->itemsPerColumn = 1;
    }

    LBSetCItemFullMax(plb);

    // Don't do this for 4.0 apps.  It'll make everyone's lives easier and
    // fix the anomaly that a combo & list created the same width end up
    // different when all is done.
    // B#1520

    if (!TestWF(pwnd, WFWIN40COMPAT)) {
        plb->fIgnoreSizeMsg = TRUE;
        NtUserMoveWindow(HWq(pwnd),
             lpcs->x - SYSMET(CXBORDER),
             lpcs->y - SYSMET(CYBORDER),
             lpcs->cx + SYSMET(CXEDGE),
             lpcs->cy + SYSMET(CYEDGE),
             FALSE);
        plb->fIgnoreSizeMsg = FALSE;
    }

    if (!plb->fNoIntegralHeight) {

        /*
         * Send a message to ourselves to resize the listbox to an integral
         * height.  We need to do it this way because at create time we are all
         * mucked up with window rects etc...
         * IanJa: #ifndef WIN16 (32-bit Windows), wParam 0 gets extended
         * to wParam 0L automatically by the compiler.
         */
        PostMessage(HWq(pwnd), WM_SIZE, 0, 0L);
    }

    return 1L;
}

/***************************************************************************\
* xxxLBoxDoDeleteItems
*
* Send DELETEITEM message for all the items in the ownerdraw listbox.
*
* History:
* 16-Apr-1992 beng          Nodata case
\***************************************************************************/

void xxxLBoxDoDeleteItems(
    PLBIV plb)
{
    INT sItem;

    CheckLock(plb->spwnd);

    /*
     * Send WM_DELETEITEM message for ownerdraw listboxes which are
     * being deleted.  (NODATA listboxes don't send such, though.)
     */
    if (plb->OwnerDraw && plb->cMac && plb->fHasData) {
        for (sItem = plb->cMac - 1; sItem >= 0; sItem--) {
            xxxLBoxDeleteItem(plb, sItem);
        }
    }
}


/***************************************************************************\
* xxxDestroyLBox
*
* History:
\***************************************************************************/

void xxxDestroyLBox(
    PLBIV pLBIV,
    PWND pwnd)
{
    PWND pwndParent;

    CheckLock(pwnd);

    if (pLBIV != NULL) {
        CheckLock(pLBIV->spwnd);

        /*
         * If ownerdraw, send deleteitem messages to parent
         */
        xxxLBoxDoDeleteItems(pLBIV);

        if (pLBIV->rgpch != NULL) {
            UserLocalFree(pLBIV->rgpch);
            pLBIV->rgpch = NULL;
        }

        if (pLBIV->hStrings != NULL) {
            UserLocalFree(pLBIV->hStrings);
            pLBIV->hStrings = NULL;
        }

        if (pLBIV->iTabPixelPositions != NULL) {
            UserLocalFree((HANDLE)pLBIV->iTabPixelPositions);
            pLBIV->iTabPixelPositions = NULL;
        }

        Unlock(&pLBIV->spwnd);
        Unlock(&pLBIV->spwndParent);

        if (pLBIV->pszTypeSearch) {
            UserLocalFree(pLBIV->pszTypeSearch);
        }

        FreeLookasideEntry(&ListboxLookaside, pLBIV);
    }

    /*
     * Set the window's fnid status so that we can ignore rogue messages
     */
    NtUserSetWindowFNID(HWq(pwnd), FNID_CLEANEDUP_BIT);

    /*
     * If we're part of a combo box, let it know we're gone
     */
    pwndParent = REBASEPWND(pwnd, spwndParent);
    if (pwndParent && GETFNID(pwndParent) == FNID_COMBOBOX) {
        ComboBoxWndProcWorker(pwndParent, WM_PARENTNOTIFY,
                MAKELONG(WM_DESTROY, PTR_TO_ID(pwnd->spmenu)), (LPARAM)HWq(pwnd), FALSE);
    }
}


/***************************************************************************\
* xxxLBSetFont
*
* History:
\***************************************************************************/

void xxxLBSetFont(
    PLBIV plb,
    HANDLE hFont,
    BOOL fRedraw)
{
    HDC     hdc;
    HANDLE  hOldFont = NULL;
    int     iHeight;

    CheckLock(plb->spwnd);

    plb->hFont = hFont;

    hdc = NtUserGetDC(HWq(plb->spwnd));

    if (hFont) {
        hOldFont = SelectObject(hdc, hFont);
        if (!hOldFont) {
            plb->hFont = NULL;
        }
    }

    plb->cxChar = GdiGetCharDimensions(hdc, NULL, &iHeight);
    if (plb->cxChar == 0) {
        RIPMSG0(RIP_WARNING, "xxxLBSetFont: GdiGetCharDimensions failed");
        plb->cxChar = gpsi->cxSysFontChar;
        iHeight = gpsi->cySysFontChar;
    }


    if (!plb->OwnerDraw && (plb->cyChar != iHeight)) {

        /*
         * We don't want to mess up the cyChar height for owner draw listboxes
         * so don't do this.
         */
        plb->cyChar = iHeight;

        /*
         * Only resize the listbox for 4.0 dudes, or combo dropdowns.
         * Macromedia Director 4.0 GP-faults otherwise.
         */
        if (!plb->fNoIntegralHeight &&
                (plb->pcbox || TestWF(plb->spwnd, WFWIN40COMPAT))) {
            xxxLBSize(plb,
                plb->spwnd->rcClient.right  - plb->spwnd->rcClient.left,
                plb->spwnd->rcClient.bottom - plb->spwnd->rcClient.top);
        }
    }

    if (hOldFont) {
        SelectObject(hdc, hOldFont);
    }

    /*
     * IanJa: was ReleaseDC(hwnd, hdc);
     */
    NtUserReleaseDC(HWq(plb->spwnd), hdc);

    if (plb->fMultiColumn) {
        LBCalcItemRowsAndColumns(plb);
    }

    LBSetCItemFullMax(plb);

    if (fRedraw)
        xxxCheckRedraw(plb, FALSE, 0);
}


/***************************************************************************\
* xxxLBSize
*
* History:
\***************************************************************************/

void xxxLBSize(
    PLBIV plb,
    INT cx,
    INT cy)
{
    RECT rc;
    int iTopOld;
    BOOL fSizedSave;

    CheckLock(plb->spwnd);

    if (!plb->fNoIntegralHeight) {
        int cBdrs = GetWindowBorders(plb->spwnd->style, plb->spwnd->ExStyle, TRUE, TRUE);

        CopyInflateRect(&rc, &plb->spwnd->rcWindow, 0, -cBdrs * SYSMET(CYBORDER));

        // Size the listbox to fit an integral # of items in its client
        if ((rc.bottom - rc.top) % plb->cyChar) {
            int iItems = (rc.bottom - rc.top);

            // B#2285 - If its a 3.1 app its SetWindowPos needs
            // to be window based dimensions not Client !
            // this crunches Money into using a scroll bar

            if ( ! TestWF( plb->spwnd, WFWIN40COMPAT ) )
                iItems += (cBdrs * SYSMET(CYEDGE)); // so add it back in

            iItems /= plb->cyChar;

            NtUserSetWindowPos(HWq(plb->spwnd), HWND_TOP, 0, 0, rc.right - rc.left,
                    iItems * plb->cyChar + (SYSMET(CYEDGE) * cBdrs),
                    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);

            /*
             * Changing the size causes us to recurse.  Upon return
             * the state is where it should be and nothing further
             * needs to be done.
             */
            return;
        }
    }

    if (plb->fMultiColumn) {

        /*
         * Compute the number of DISPLAYABLE rows and columns in the listbox
         */
        LBCalcItemRowsAndColumns(plb);
    } else {

        /*
         * Adjust the current horizontal position to eliminate as much
         * empty space as possible from the right side of the items.
         */
        _GetClientRect(plb->spwnd, &rc);
        if ((plb->maxWidth - plb->xOrigin) < (rc.right - rc.left))
            plb->xOrigin = max(0, plb->maxWidth - (rc.right - rc.left));
    }

    LBSetCItemFullMax(plb);

    /*
     * Adjust the top item in the listbox to eliminate as much empty space
     * after the last item as possible
     * (fix for bugs #8490 & #3836)
     */
    iTopOld = plb->iTop;
    fSizedSave = plb->fSized;
    plb->fSized = FALSE;
    xxxNewITop(plb, plb->iTop);

    /*
     * If changing the top item index caused a resize, there is no
     * more work to be done here.
     */
    if (plb->fSized)
        return;
    plb->fSized = fSizedSave;

    if (IsLBoxVisible(plb)) {
        /*
         * This code no longer blows because it's fixed right!!!  We could
         * optimize the fMultiColumn case with some more code to figure out
         * if we really need to invalidate the whole thing but note that some
         * 3.0 apps depend on this extra invalidation (AMIPRO 2.0, bug 14620)
         *
         * For 3.1 apps, we blow off the invalidaterect in the case where
         * cx and cy are 0 because this happens during the processing of
         * the posted WM_SIZE message when we are created which would otherwise
         * cause us to flash.
         */
        if ((plb->fMultiColumn && !(cx == 0 && cy == 0)) ||
                plb->iTop != iTopOld)
            NtUserInvalidateRect(HWq(plb->spwnd), NULL, TRUE);
        else if (plb->iSelBase >= 0) {

            /*
             * Invalidate the item with the caret so that if the listbox
             * grows horizontally, we redraw it properly.
             */
            LBGetItemRect(plb, plb->iSelBase, &rc);
            NtUserInvalidateRect(HWq(plb->spwnd), &rc, FALSE);
        }
    } else if (!plb->fRedraw)
        plb->fDeferUpdate = TRUE;

    /*
     * Send "fake" scroll bar messages to update the scroll positions since we
     * changed size.
     */
    if (TestWF(plb->spwnd, WFVSCROLL)) {
        xxxLBoxCtlScroll(plb, SB_ENDSCROLL, 0);
    }

    /*
     * We count on this to call LBShowHideScrollBars except when plb->cMac == 0!
     */
    xxxLBoxCtlHScroll(plb, SB_ENDSCROLL, 0);

    /*
     * Show/hide scroll bars depending on how much stuff is visible...
     *
     * Note:  Now we only call this guy when cMac == 0, because it is
     * called inside the LBoxCtlHScroll with SB_ENDSCROLL otherwise.
     */
    if (plb->cMac == 0)
        xxxLBShowHideScrollBars(plb);
}


/***************************************************************************\
* LBSetTabStops
*
* Sets the tab stops for this listbox. Returns TRUE if successful else FALSE.
*
* History:
\***************************************************************************/

BOOL LBSetTabStops(
    PLBIV plb,
    INT count,
    LPINT lptabstops)
{
    PINT ptabs;

    if (!plb->fUseTabStops) {
        RIPERR0(ERROR_LB_WITHOUT_TABSTOPS, RIP_VERBOSE, "");
        return FALSE;
    }

    if (count) {
        /*
         * Allocate memory for the tab stops.  The first byte in the
         * plb->iTabPixelPositions array will contain a count of the number
         * of tab stop positions we have.
         */
        ptabs = (LPINT)UserLocalAlloc(HEAP_ZERO_MEMORY, (count + 1) * sizeof(int));
        if (ptabs == NULL)
            return FALSE;

        if (plb->iTabPixelPositions != NULL)
            UserLocalFree(plb->iTabPixelPositions);
        plb->iTabPixelPositions = ptabs;

        /*
         * Set the count of tab stops
         */
        *ptabs++ = count;

        for (; count > 0; count--) {

            /*
             * Convert the dialog unit tabstops into pixel position tab stops.
             */
            *ptabs++ = MultDiv(*lptabstops, plb->cxChar, 4);
            lptabstops++;
        }
    } else {

        /*
         * Set default 8 system font ave char width tabs.  So free the memory
         * associated with the tab stop list.
         */
        if (plb->iTabPixelPositions != NULL) {
            UserLocalFree((HANDLE)plb->iTabPixelPositions);
            plb->iTabPixelPositions = NULL;
        }
    }

    return TRUE;
}


/***************************************************************************\
* InitHStrings
*
* History:
\***************************************************************************/

void InitHStrings(
    PLBIV plb)
{
    if (plb->fHasStrings) {
        plb->ichAlloc = 0;
        plb->cchStrings = 0;
        plb->hStrings = UserLocalAlloc(0, 0L);
    }
}


/***************************************************************************\
* LBDropObjectHandler
*
* Handles a WM_DROPITEM message on this listbox
*
* History:
\***************************************************************************/

void LBDropObjectHandler(
    PLBIV plb,
    PDROPSTRUCT pds)
{
    LONG mouseSel;

    if (ISelFromPt(plb, pds->ptDrop, &mouseSel)) {

        /*
         * User dropped in empty space at bottom of listbox
         */
        pds->dwControlData = (DWORD)-1L;
    } else {
        pds->dwControlData = mouseSel;
    }
}


/***************************************************************************\
* LBGetSetItemHeightHandler()
*
* Sets/Gets the height associated with each item.  For non ownerdraw
* and fixed height ownerdraw, the item number is ignored.
*
* History:
\***************************************************************************/

int LBGetSetItemHeightHandler(
    PLBIV plb,
    UINT message,
    int item,
    UINT height)
{
    if (message == LB_GETITEMHEIGHT) {
        /*
         * All items are same height for non ownerdraw and for fixed height
         * ownerdraw.
         */
        if (plb->OwnerDraw != OWNERDRAWVAR)
            return plb->cyChar;

        if (plb->cMac && item >= plb->cMac) {
            RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
            return LB_ERR;
        }

        return (int)LBGetVariableHeightItemHeight(plb, (INT)item);
    }

    if (!height || height > 255) {
        RIPERR1(ERROR_INVALID_PARAMETER,
                RIP_WARNING,
                "Invalid parameter \"height\" (%ld) to LBGetSetItemHeightHandler",
                height);

        return LB_ERR;
    }

    if (plb->OwnerDraw != OWNERDRAWVAR)
        plb->cyChar = height;
    else {
        if (item < 0 || item >= plb->cMac) {
            RIPERR1(ERROR_INVALID_PARAMETER,
                    RIP_WARNING,
                    "Invalid parameter \"item\" (%ld) to LBGetSetItemHeightHandler",
                    item);

            return LB_ERR;
        }

        LBSetVariableHeightItemHeight(plb, (INT)item, (INT)height);
    }

    if (plb->fMultiColumn)
        LBCalcItemRowsAndColumns(plb);

    LBSetCItemFullMax(plb);

    return(0);
}

/*****************************************************************************\
*
* LBEvent()
*
* This is for item focus & selection events in listboxes.
*
\*****************************************************************************/
void LBEvent(PLBIV plb, UINT uEvent, int iItem)
{
    UserAssert(FWINABLE());

    switch (uEvent) {
        case EVENT_OBJECT_SELECTIONREMOVE:
            if (plb->wMultiple != SINGLESEL) {
                break;
            }
            iItem = -1;
            //
            // FALL THRU
            //

        case EVENT_OBJECT_SELECTIONADD:
            if (plb->wMultiple == MULTIPLESEL) {
                uEvent = EVENT_OBJECT_SELECTION;
            }
            break;

        case EVENT_OBJECT_SELECTIONWITHIN:
            iItem = -1;
            break;
    }

    NotifyWinEvent(uEvent, HW(plb->spwnd), OBJID_CLIENT, iItem+1);
}
