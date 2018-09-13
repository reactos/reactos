/**************************** Module Header ********************************\
* Module Name: lboxctl1.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* List Box Handling Routines
*
* History:
* ??-???-???? ianja    Ported from Win 3.0 sources
* 14-Feb-1991 mikeke   Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

INT xxxLBBinarySearchString(PLBIV plb,LPWSTR lpstr);

/***************************************************************************\
*
*  SetLBScrollParms()
*
*  Sets the scroll range, page, and position
*
\***************************************************************************/

int xxxSetLBScrollParms(PLBIV plb, int nCtl)
{
    int         iPos;
    int         cItems;
    UINT        iPage;
    SCROLLINFO  si;
    BOOL        fNoScroll = FALSE;
    PSCROLLPOS  psp;
    BOOL        fCacheInitialized;
    int         iReturn;

    if (nCtl == SB_VERT) {
        iPos = plb->iTop;
        cItems = plb->cMac;
        iPage = plb->cItemFullMax;
        if (!plb->fVertBar)
            fNoScroll = TRUE;
        psp = &plb->VPos;
        fCacheInitialized = plb->fVertInitialized;
    } else {
        if (plb->fMultiColumn) {
            iPos   = plb->iTop / plb->itemsPerColumn;
            cItems = plb->cMac ? ((plb->cMac - 1) / plb->itemsPerColumn) + 1 : 0;
            iPage = plb->numberOfColumns;
            if (plb->fRightAlign && cItems)
                iPos = cItems - iPos - 1;
        } else {
            iPos = plb->xOrigin;
            cItems = plb->maxWidth;
            iPage = plb->spwnd->rcClient.right - plb->spwnd->rcClient.left;
        }

        if (!plb->fHorzBar)
            fNoScroll = TRUE;
        psp = &plb->HPos;
        fCacheInitialized = plb->fHorzInitialized;
    }

    if (cItems)
        cItems--;

    if (fNoScroll) {
        // Limit page to 0, posMax + 1
        iPage = max(min((int)iPage, cItems + 1), 0);

        // Limit pos to 0, posMax - (page - 1).
        return(max(min(iPos, cItems - ((iPage) ? (int)(iPage - 1) : 0)), 0));
    } else {
        si.fMask    = SIF_ALL;
        if (plb->fDisableNoScroll)
            si.fMask |= SIF_DISABLENOSCROLL;

        /*
         * If the scrollbar is already where we want it, do nothing.
         */
        if (fCacheInitialized) {
            if (psp->fMask == si.fMask &&
                    psp->cItems == cItems && psp->iPage == iPage &&
                    psp->iPos == iPos)
                return psp->iReturn;
        } else if (nCtl == SB_VERT) {
            plb->fVertInitialized = TRUE;
        } else {
            plb->fHorzInitialized = TRUE;
        }

        si.cbSize   = sizeof(SCROLLINFO);
        si.nMin     = 0;
        si.nMax     = cItems;
        si.nPage    = iPage;

        if (plb->fMultiColumn && plb->fRightAlign)
            si.nPos =  (iPos+1) > (int)iPage ? iPos - iPage + 1 : 0;
        else
            si.nPos = iPos;

        iReturn = NtUserSetScrollInfo(HWq(plb->spwnd), nCtl, &si, plb->fRedraw);
        if (plb->fMultiColumn && plb->fRightAlign)
            iReturn = cItems - (iReturn + iPage - 1);

        /*
         * Update the position cache
         */
        psp->fMask = si.fMask;
        psp->cItems = cItems;
        psp->iPage = iPage;
        psp->iPos = iPos;
        psp->iReturn = iReturn;

        return iReturn;
    }
}

/***************************************************************************\
* xxxLBShowHideScrollBars
*
* History:
\***************************************************************************/

void xxxLBShowHideScrollBars(
    PLBIV plb)
{
    BOOL fVertDone = FALSE;
    BOOL fHorzDone = FALSE;

    // Don't do anything if there are no scrollbars or if parents
    // are invisible.
    if ((!plb->fHorzBar && !plb->fVertBar) || !plb->fRedraw)
        return;

    //
    // Adjust iTop if necessary but DO NOT REDRAW PERIOD.  We never did
    // in 3.1.  There's a potential bug:
    //      If someone doesn't have redraw off and inserts an item in the
    // same position as the caret, we'll tell them to draw before they may
    // have called LB_SETITEMDATA for their item.  This is because we turn
    // the caret off & on inside of NewITop(), even if the item isn't
    // changing.
    //      So we just want to reflect the position/scroll changes.
    // CheckRedraw() will _really_ redraw the visual changes later if
    // redraw isn't off.
    //

    if (!plb->fFromInsert) {
        xxxNewITop(plb, plb->iTop);
        fVertDone = TRUE;
    }

    if (!plb->fMultiColumn) {
        if (!plb->fFromInsert) {
            fHorzDone = TRUE;
            xxxLBoxCtlHScroll(plb, SB_THUMBPOSITION, plb->xOrigin);
        }

        if (!fVertDone)
            xxxSetLBScrollParms(plb, SB_VERT);
    }
    if (!fHorzDone)
        xxxSetLBScrollParms(plb, SB_HORZ);
}

/***************************************************************************\
* LBGetItemData
*
* returns the long value associated with listbox items. -1 if error
*
* History:
* 16-Apr-1992 beng      The NODATA listbox case
\***************************************************************************/

LONG_PTR LBGetItemData(
    PLBIV plb,
    INT sItem)
{
    LONG_PTR buffer;
    LPBYTE lpItem;

    if (sItem < 0 || sItem >= plb->cMac) {
        RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
        return LB_ERR;
    }

    // No-data listboxes always return 0L
    //
    if (!plb->fHasData) {
        return 0L;
    }

    lpItem = (plb->rgpch +
            (sItem * (plb->fHasStrings ? sizeof(LBItem) : sizeof(LBODItem))));
    buffer = (plb->fHasStrings ? ((lpLBItem)lpItem)->itemData : ((lpLBODItem)lpItem)->itemData);
    return buffer;
}


/***************************************************************************\
* LBGetText
*
* Copies the text associated with index to lpbuffer and returns its length.
* If fLengthOnly, just return the length of the text without doing a copy.
*
* Waring: for size only querries lpbuffer is the count of ANSI characters
*
* Returns count of chars
*
* History:
\***************************************************************************/

INT LBGetText(
    PLBIV plb,
    BOOL fLengthOnly,
    BOOL fAnsi,
    INT index,
    LPWSTR lpbuffer)
{
    LPWSTR lpItemText;
    INT cchText;

    if (index < 0 || index >= plb->cMac) {
        RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
        return LB_ERR;
    }

    if (!plb->fHasStrings && plb->OwnerDraw) {

        /*
         * Owner draw without strings so we must copy the app supplied DWORD
         * value.
         */
        cchText = sizeof(ULONG_PTR);

        if (!fLengthOnly) {
            LONG_PTR UNALIGNED *p = (LONG_PTR UNALIGNED *)lpbuffer;
            *p = LBGetItemData(plb, index);
        }
    } else {
        lpItemText = GetLpszItem(plb, index);
        if (!lpItemText)
            return LB_ERR;

        /*
         * These are strings so we are copying the text and we must include
         * the terminating 0 when doing the RtlMoveMemory.
         */
        cchText = wcslen(lpItemText);

        if (fLengthOnly) {
            if (fAnsi)
                RtlUnicodeToMultiByteSize(&cchText, lpItemText, cchText*sizeof(WCHAR));
        } else {
            if (fAnsi) {
#ifdef FE_SB // LBGetText()
                cchText = WCSToMB(lpItemText, cchText+1, &((LPSTR)lpbuffer), (cchText+1)*sizeof(WORD), FALSE);
                /*
                 * Here.. cchText contains null-terminate char, subtract it... Because, we pass cchText+1 to
                 * above Unicode->Ansi convertsion to make sure the string is terminated with null.
                 */
                cchText--;
#else
                WCSToMB(lpItemText, cchText+1, &((LPSTR)lpbuffer), cchText+1, FALSE);
#endif // FE_SB
            } else {
                RtlCopyMemory(lpbuffer, lpItemText, (cchText+1)*sizeof(WCHAR));
            }
        }

    }

    return cchText;
}

/***************************************************************************\
* GrowMem
*
* History:
* 16-Apr-1992 beng      NODATA listboxes
* 23-Jul-1996 jparsons  Added numItems parameter for LB_INITSTORAGE support
\***************************************************************************/

BOOL GrowMem(
    PLBIV plb,
    INT   numItems)

{
    LONG cb;
    HANDLE hMem;

    /*
     * Allocate memory for pointers to the strings.
     */
    cb = (plb->cMax + numItems) *
            (plb->fHasStrings ? sizeof(LBItem)
                              : (plb->fHasData ? sizeof(LBODItem)
                                              : 0));

    /*
     * If multiple selection list box (MULTIPLESEL or EXTENDEDSEL), then
     * allocate an extra byte per item to keep track of it's selection state.
     */
    if (plb->wMultiple != SINGLESEL) {
        cb += (plb->cMax + numItems);
    }

    /*
     * Extra bytes for each item so that we can store its height.
     */
    if (plb->OwnerDraw == OWNERDRAWVAR) {
        cb += (plb->cMax + numItems);
    }

    /*
     * Don't allocate more than 2G of memory
     */
    if (cb > MAXLONG)
        return FALSE;

    if (plb->rgpch == NULL) {
        if ((plb->rgpch = UserLocalAlloc(HEAP_ZERO_MEMORY, (LONG)cb)) == NULL)
            return FALSE;
    } else {
        if ((hMem = UserLocalReAlloc(plb->rgpch, (LONG)cb, HEAP_ZERO_MEMORY)) == NULL)
            return FALSE;
        plb->rgpch = hMem;
    }

    plb->cMax += numItems;

    return TRUE;
}

/***************************************************************************\
* xxxLBInitStorage
*
* History:
* 23-Jul-1996 jparsons  Added support for pre-allocation
\***************************************************************************/
LONG xxxLBInitStorage(PLBIV plb, BOOL fAnsi, INT cItems, INT cb)
{
    HANDLE hMem;
    INT    cbChunk;

    /*
     * if the app is talking ANSI, then adjust for the worst case in unicode
     * where each single ansi byte translates to one 16 bit unicode value
     */
    if (fAnsi) {
        cb *= sizeof(WCHAR) ;
    } /* if */

    /*
     * Fail if either of the parameters look bad.
     */
    if ((cItems < 0) || (cb < 0)) {
        xxxNotifyOwner(plb, LBN_ERRSPACE);
        return LB_ERRSPACE;
    } /* if */

    /*
     * try to grow the pointer array (if necessary) accounting for the free space
     * already available.
     */
    cItems -= plb->cMax - plb->cMac ;
    if ((cItems > 0) && !GrowMem(plb, cItems)) {
        xxxNotifyOwner(plb, LBN_ERRSPACE);
        return LB_ERRSPACE;
    } /* if */

    /*
     * now grow the string space if necessary
     */
    if (plb->fHasStrings) {
        if ((cbChunk = (plb->ichAlloc + cb)) > plb->cchStrings) {

            /*
             * Round up to the nearest 256 byte chunk.
             */
            cbChunk = (cbChunk & ~0xff) + 0x100;
            if (!(hMem = UserLocalReAlloc(plb->hStrings, (LONG)cbChunk, 0))) {
                xxxNotifyOwner(plb, LBN_ERRSPACE);
                return LB_ERRSPACE;
            }
            plb->hStrings = hMem;
            plb->cchStrings = cbChunk;
        } /* if */
    } /* if */

    /*
     * return the number of items that can be stored
     */
    return plb->cMax ;
}

/***************************************************************************\
* xxxInsertString
*
* Insert an item at a specified position.
*
* History:
* 16-Apr-1992 beng      NODATA listboxes
\***************************************************************************/

INT xxxLBInsertItem(
    PLBIV plb,

    /*
     * For owner draw listboxes without LBS_HASSTRINGS style, this is not a
     * string but rather a 4 byte value we will store for the app.
     */
    LPWSTR lpsz,
    INT index,
    UINT wFlags)
{
    MEASUREITEMSTRUCT measureItemStruct;
    INT cbString;
    INT cbChunk;
    PBYTE lp;
    PBYTE lpT;
    PBYTE lpHeightStart;
    LONG cbItem;     /* sizeof the Item in rgpch */
    HANDLE hMem;
    TL tlpwndParent;

    CheckLock(plb->spwnd);

    if (wFlags & LBI_ADD)
        index = (plb->fSort) ? xxxLBBinarySearchString(plb, lpsz) : -1;

    if (!plb->rgpch) {
        if (index != 0 && index != -1) {
            RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
            return LB_ERR;
        }

        plb->iSel = -1;
        plb->iSelBase = 0;
        plb->cMax = 0;
        plb->cMac = 0;
        plb->iTop = 0;
        plb->rgpch = UserLocalAlloc(HEAP_ZERO_MEMORY, 0L);
        if (!plb->rgpch)
            return LB_ERR;
    }

    if (index == -1) {
        index = plb->cMac;
    }

    if (index > plb->cMac || plb->cMac >= MAXLONG) {
        RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
        return LB_ERR;
    }

    if (plb->fHasStrings) {

        /*
         * we must store the string in the hStrings memory block.
         */
        cbString = (wcslen(lpsz) + 1)*sizeof(WCHAR);  /* include 0 terminator */

        if ((cbChunk = (plb->ichAlloc + cbString)) > plb->cchStrings) {

            /*
             * Round up to the nearest 256 byte chunk.
             */
            cbChunk = (cbChunk & ~0xff) + 0x100;
            if (!(hMem = UserLocalReAlloc(plb->hStrings, (LONG)cbChunk,
                    0))) {
                xxxNotifyOwner(plb, LBN_ERRSPACE);
                return LB_ERRSPACE;
            }
            plb->hStrings = hMem;

            plb->cchStrings = cbChunk;
        }

        /*
         * Note difference between Win 95 code with placement of new string
         */
        if (wFlags & UPPERCASE)
            CharUpperBuffW((LPWSTR)lpsz, cbString / sizeof(WCHAR));
        else if (wFlags & LOWERCASE)
            CharLowerBuffW((LPWSTR)lpsz, cbString / sizeof(WCHAR));

        lp = (PBYTE)(plb->hStrings);
        RtlMoveMemory(lp + plb->ichAlloc, lpsz, cbString);
    }

    /*
     * Now expand the pointer array.
     */
    if (plb->cMac >= plb->cMax) {
        if (!GrowMem(plb, CITEMSALLOC)) {
            xxxNotifyOwner(plb, LBN_ERRSPACE);
            return LB_ERRSPACE;
        }
    }

    lpHeightStart = lpT = lp = plb->rgpch;

    /*
     * Now calculate how much room we must make for the string pointer (lpsz).
     * If we are ownerdraw without LBS_HASSTRINGS, then a single DWORD
     * (LBODItem.itemData) stored for each item, but if we have strings with
     * each item then a LONG string offset (LBItem.offsz) is also stored.
     */
    cbItem = (plb->fHasStrings ? sizeof(LBItem)
                               : (plb->fHasData ? sizeof(LBODItem):0));
    cbChunk = (plb->cMac - index) * cbItem;

    if (plb->wMultiple != SINGLESEL) {

        /*
         * Extra bytes were allocated for selection flag for each item
         */
        cbChunk += plb->cMac;
    }

    if (plb->OwnerDraw == OWNERDRAWVAR) {

        /*
         * Extra bytes were allocated for each item's height
         */
        cbChunk += plb->cMac;
    }

    /*
     * First, make room for the 2 byte pointer to the string or the 4 byte app
     * supplied value.
     */
    lpT += (index * cbItem);
    RtlMoveMemory(lpT + cbItem, lpT, cbChunk);
    if (!plb->fHasStrings && plb->OwnerDraw) {
        if (plb->fHasData) {
            /*
             * Ownerdraw so just save the DWORD value
             */
            lpLBODItem p = (lpLBODItem)lpT;
            p->itemData = (ULONG_PTR)lpsz;
        }
    } else {
        lpLBItem p = ((lpLBItem)lpT);

        /*
         * Save the start of the string.  Let the item data field be 0
         */
        p->offsz = (LONG)(plb->ichAlloc);
        p->itemData = 0;
        plb->ichAlloc += cbString;
    }

    /*
     * Now if Multiple Selection lbox, we have to insert a selection status
     * byte.  If var height ownerdraw, then we also have to move up the height
     * bytes.
     */
    if (plb->wMultiple != SINGLESEL) {
        lpT = lp + ((plb->cMac + 1) * cbItem) + index;
        RtlMoveMemory(lpT + 1, lpT, plb->cMac - index +
                (plb->OwnerDraw == OWNERDRAWVAR ? plb->cMac : 0));
        *lpT = 0;  /* fSelected = FALSE */
    }

    /*
     * Increment count of items in the listbox now before we send a message to
     * the app.
     */
    plb->cMac++;

    /*
     * If varheight ownerdraw, we much insert an extra byte for the item's
     * height.
     */
    if (plb->OwnerDraw == OWNERDRAWVAR) {

        /*
         * Variable height owner draw so we need to get the height of each item.
         */
        lpHeightStart += (plb->cMac * cbItem) + index +
                (plb->wMultiple ? plb->cMac : 0);

        RtlMoveMemory(lpHeightStart + 1, lpHeightStart, plb->cMac - 1 - index);

        /*
         * Query for item height only if we are var height owner draw.
         */
        measureItemStruct.CtlType = ODT_LISTBOX;
        measureItemStruct.CtlID = PtrToUlong(plb->spwnd->spmenu);
        measureItemStruct.itemID = index;

        /*
         * System font height is default height
         */
        measureItemStruct.itemHeight = (UINT)gpsi->cySysFontChar;
        measureItemStruct.itemData = (ULONG_PTR)lpsz;

        /*
         * If "has strings" then add the special thunk bit so the client data
         * will be thunked to a client side address.  LB_DIR sends a string
         * even if the listbox is not HASSTRINGS so we need to special
         * thunk this case.  HP Dashboard for windows send LB_DIR to a non
         * HASSTRINGS listbox needs the server string converted to client.
         * WOW needs to know about this situation as well so we mark the
         * previously uninitialized itemWidth as FLAT.
         */
        if (plb->fHasStrings || (wFlags & MSGFLAG_SPECIAL_THUNK)) {
            measureItemStruct.itemWidth = MIFLAG_FLAT;
        }

        ThreadLock(plb->spwndParent, &tlpwndParent);
        SendMessage(HW(plb->spwndParent),
                WM_MEASUREITEM,
                measureItemStruct.CtlID,
                (LPARAM)&measureItemStruct);
        ThreadUnlock(&tlpwndParent);
        *lpHeightStart = (BYTE)measureItemStruct.itemHeight;
    }


    /*
     * If the item was inserted above the current selection then move
     * the selection down one as well.
     */
    if ((plb->wMultiple == SINGLESEL) && (plb->iSel >= index))
        plb->iSel++;

    if (plb->OwnerDraw == OWNERDRAWVAR)
        LBSetCItemFullMax(plb);

    /*
     * Check if scroll bars need to be shown/hidden
     */
    plb->fFromInsert = TRUE;
    xxxLBShowHideScrollBars(plb);
    if (plb->fHorzBar && plb->fRightAlign && !(plb->fMultiColumn || plb->OwnerDraw)) {
        /*
         * origin to right
         */
        xxxLBoxCtlHScroll(plb, SB_BOTTOM, 0);
    }
    plb->fFromInsert = FALSE;

    xxxCheckRedraw(plb, TRUE, index);

    if (FWINABLE()) {
        LBEvent(plb, EVENT_OBJECT_CREATE, index);
    }

    return index;
}


/***************************************************************************\
* LBlstrcmpi
*
* This is a version of lstrcmpi() specifically used for listboxes
* This gives more weight to '[' characters than alpha-numerics;
* The US version of lstrcmpi() and lstrcmp() are similar as far as
* non-alphanumerals are concerned; All non-alphanumerals get sorted
* before alphanumerals; This means that subdirectory strings that start
* with '[' will get sorted before; But we don't want that; So, this
* function takes care of it;
*
* History:
\***************************************************************************/

INT LBlstrcmpi(
    LPWSTR lpStr1,
    LPWSTR lpStr2,
    DWORD dwLocaleId)
{

    /*
     * NOTE: This function is written so as to reduce the number of calls
     * made to the costly IsCharAlphaNumeric() function because that might
     * load a language module; It 'traps' the most frequently occurring cases
     * like both strings starting with '[' or both strings NOT starting with '['
     * first and only in abosolutely necessary cases calls IsCharAlphaNumeric();
     */
    if (*lpStr1 == TEXT('[')) {
        if (*lpStr2 == TEXT('[')) {
            goto LBL_End;
        }
        if (IsCharAlphaNumeric(*lpStr2)) {
            return 1;
        }
    }

    if ((*lpStr2 == TEXT('[')) && IsCharAlphaNumeric(*lpStr1)) {
        return -1;
    }

LBL_End:
    if ((GetClientInfo()->dwTIFlags & TIF_16BIT) &&
        dwLocaleId == MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)) {
        /*
         * This is how Windows95 does, bug #4199
         */
        return (*pfnWowIlstrcmp)(lpStr1, lpStr2);
    }
    return (INT)CompareStringW((LCID)dwLocaleId, NORM_IGNORECASE,
            lpStr1, -1, lpStr2, -1 ) - 2;
}


/***************************************************************************\
* xxxLBBinarySearchString
*
* Does a binary search of the items in the SORTED listbox to find
* out where this item should be inserted.  Handles both HasStrings and item
* long WM_COMPAREITEM cases.
*
* History:
*    27 April 1992  GregoryW
*          Modified to support sorting based on current list box locale.
\***************************************************************************/

INT xxxLBBinarySearchString(
    PLBIV plb,
    LPWSTR lpstr)
{
    BYTE *FAR *lprgpch;
    INT sortResult;
    COMPAREITEMSTRUCT cis;
    LPWSTR pszLBBase;
    LPWSTR pszLB;
    INT itemhigh;
    INT itemnew = 0;
    INT itemlow = 0;
    TL tlpwndParent;

    CheckLock(plb->spwnd);

    if (!plb->cMac)
        return 0;

    lprgpch = (BYTE *FAR *)(plb->rgpch);
    if (plb->fHasStrings) {
        pszLBBase = plb->hStrings;
    }

    itemhigh = plb->cMac - 1;
    while (itemlow <= itemhigh) {
        itemnew = (itemhigh + itemlow) / 2;

        if (plb->fHasStrings) {

            /*
             * Searching for string matches.
             */
            pszLB = (LPWSTR)((LPSTR)pszLBBase + ((lpLBItem)lprgpch)[itemnew].offsz);
            sortResult = LBlstrcmpi(pszLB, lpstr, plb->dwLocaleId);
        } else {

            /*
             * Send compare item messages to the parent for sorting
             */
            cis.CtlType = ODT_LISTBOX;
            cis.CtlID = PtrToUlong(plb->spwnd->spmenu);
            cis.hwndItem = HWq(plb->spwnd);
            cis.itemID1 = itemnew;
            cis.itemData1 = ((lpLBODItem)lprgpch)[itemnew].itemData;
            cis.itemID2 = (UINT)-1;
            cis.itemData2 = (ULONG_PTR)lpstr;
            cis.dwLocaleId = plb->dwLocaleId;
            ThreadLock(plb->spwndParent, &tlpwndParent);
            sortResult = (INT)SendMessage(HW(plb->spwndParent), WM_COMPAREITEM,
                    cis.CtlID, (LPARAM)&cis);
            ThreadUnlock(&tlpwndParent);
        }

        if (sortResult < 0) {
            itemlow = itemnew + 1;
        } else if (sortResult > 0) {
            itemhigh = itemnew - 1;
        } else {
            itemlow = itemnew;
            goto FoundIt;
        }
    }

FoundIt:

    return max(0, itemlow);
}

/***************************************************************************\
* xxxLBResetContent
*
* History:
\***************************************************************************/

BOOL xxxLBResetContent(
    PLBIV plb)
{
    if (!plb->cMac)
        return TRUE;

    xxxLBoxDoDeleteItems(plb);

    if (plb->rgpch != NULL) {
        UserLocalFree(plb->rgpch);
        plb->rgpch = NULL;
    }

    if (plb->hStrings != NULL) {
        UserLocalFree(plb->hStrings);
        plb->hStrings = NULL;
    }

    InitHStrings(plb);

    if (TestWF(plb->spwnd, WFWIN31COMPAT))
        xxxCheckRedraw(plb, FALSE, 0);
    else if (IsVisible(plb->spwnd))
        NtUserInvalidateRect(HWq(plb->spwnd), NULL, TRUE);

    plb->iSelBase =  0;
    plb->iTop =  0;
    plb->cMac =  0;
    plb->cMax =  0;
    plb->xOrigin =  0;
    plb->iLastSelection =  0;
    plb->iSel = -1;

    xxxLBShowHideScrollBars(plb);
    return TRUE;
}


/***************************************************************************\
* xxxLBoxCtlDelete
*
* History:
* 16-Apr-1992 beng      NODATA listboxes
\***************************************************************************/

INT xxxLBoxCtlDelete(
    PLBIV plb,
    INT sItem)  /* Item number to delete */
{
    LONG cb;
    LPBYTE lp;
    LPBYTE lpT;
    RECT rc;
    int cbItem;    /* size of Item in rgpch */
    LPWSTR lpString;
    PBYTE pbStrings;
    INT cbStringLen;
    LPBYTE itemNumbers;
    INT sTmp;
    TL tlpwnd;

    CheckLock(plb->spwnd);

    if (sItem < 0 || sItem >= plb->cMac) {
        RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
        return LB_ERR;
    }

    if (FWINABLE()) {
        LBEvent(plb, EVENT_OBJECT_DESTROY, sItem);
    }

    if (plb->cMac == 1) {

        /*
         * When the item count is 0, we send a resetcontent message so that we
         * can reclaim our string space this way.
         */
        SendMessageWorker(plb->spwnd, LB_RESETCONTENT, 0, 0, FALSE);
        goto FinishUpDelete;
    }

    /*
     * Get the rectangle associated with the last item in the listbox.  If it is
     * visible, we need to invalidate it.  When we delete an item, everything
     * scrolls up to replace the item deleted so we must make sure we erase the
     * old image of the last item in the listbox.
     */
    if (LBGetItemRect(plb, (INT)(plb->cMac - 1), &rc)) {
        xxxLBInvalidateRect(plb, &rc, TRUE);
    }

    // 3.1 and earlier used to only send WM_DELETEITEMs if it was an ownerdraw
    // listbox.  4.0 and above will send WM_DELETEITEMs for every item that has
    // nonzero item data.
    if (TestWF(plb->spwnd, WFWIN40COMPAT) || (plb->OwnerDraw && plb->fHasData)) {
        xxxLBoxDeleteItem(plb, sItem);
    }

    plb->cMac--;

    cbItem = (plb->fHasStrings ? sizeof(LBItem)
                               : (plb->fHasData ? sizeof(LBODItem): 0));
    cb = ((plb->cMac - sItem) * cbItem);

    /*
     * Byte for the selection status of the item.
     */
    if (plb->wMultiple != SINGLESEL) {
        cb += (plb->cMac + 1);
    }

    if (plb->OwnerDraw == OWNERDRAWVAR) {

        /*
         * One byte for the height of the item.
         */
        cb += (plb->cMac + 1);
    }

    /*
     * Might be nodata and singlesel, for instance.
     * but what out for the case where cItem == cMac (and cb == 0).
     */
    if ((cb != 0) || plb->fHasStrings) {
        lp = plb->rgpch;

        lpT = (lp + (sItem * cbItem));

        if (plb->fHasStrings) {
            /*
             * If we has strings with each item, then we want to compact the string
             * heap so that we can recover the space occupied by the string of the
             * deleted item.
             */
            /*
             * Get the string which we will be deleting
             */
            pbStrings = (PBYTE)(plb->hStrings);
            lpString = (LPTSTR)(pbStrings + ((lpLBItem)lpT)->offsz);
            cbStringLen = (wcslen(lpString) + 1) * sizeof(WCHAR);  /* include null terminator */

            /*
             * Now compact the string array
             */
            plb->ichAlloc = plb->ichAlloc - cbStringLen;

            RtlMoveMemory(lpString, (PBYTE)lpString + cbStringLen,
                    plb->ichAlloc + (pbStrings - (LPBYTE)lpString));

            /*
             * We have to update the string pointers in plb->rgpch since all the
             * string after the deleted string have been moved down stringLength
             * bytes.  Note that we have to explicitly check all items in the list
             * box if the string was allocated after the deleted item since the
             * LB_SORT style allows a lower item number to have a string allocated
             * at the end of the string heap for example.
             */
            itemNumbers = lp;
            for (sTmp = 0; sTmp <= plb->cMac; sTmp++) {
                lpLBItem p =(lpLBItem)itemNumbers;
                if ( (LPTSTR)(p->offsz + pbStrings) > lpString ) {
                    p->offsz -= cbStringLen;
                }
                p++;
                itemNumbers=(LPBYTE)p;
            }
        }

        /*
         * Now compact the pointers to the strings (or the long app supplied values
         * if ownerdraw without strings).
         */
        RtlMoveMemory(lpT, lpT + cbItem, cb);

        /*
         * Compress the multiselection bytes
         */
        if (plb->wMultiple != SINGLESEL) {
            lpT = (lp + (plb->cMac * cbItem) + sItem);
            RtlMoveMemory(lpT, lpT + 1, plb->cMac - sItem +
                    (plb->OwnerDraw == OWNERDRAWVAR ? plb->cMac + 1 : 0));
        }

        if (plb->OwnerDraw == OWNERDRAWVAR) {
            /*
             * Compress the height bytes
             */
            lpT = (lp + (plb->cMac * cbItem) + (plb->wMultiple ? plb->cMac : 0)
                    + sItem);
            RtlMoveMemory(lpT, lpT + 1, plb->cMac - sItem);
        }

    }

    if (plb->wMultiple == SINGLESEL) {
        if (plb->iSel == sItem) {
            plb->iSel = -1;

            if (plb->pcbox != NULL) {
                ThreadLock(plb->pcbox->spwnd, &tlpwnd);
                xxxCBInternalUpdateEditWindow(plb->pcbox, NULL);
                ThreadUnlock(&tlpwnd);
            }
        } else if (plb->iSel > sItem)
            plb->iSel--;
    }

    if ((plb->iMouseDown != -1) && (sItem <= plb->iMouseDown))
        plb->iMouseDown = -1;

    if (plb->iSelBase && sItem == plb->iSelBase)
        plb->iSelBase--;

    if (plb->cMac) {
        plb->iSelBase = min(plb->iSelBase, plb->cMac - 1);
    } else {
        plb->iSelBase = 0;
    }

    if ((plb->wMultiple == EXTENDEDSEL) && (plb->iSel == -1))
        plb->iSel = plb->iSelBase;

    if (plb->OwnerDraw == OWNERDRAWVAR)
        LBSetCItemFullMax(plb);

    /*
     * We always set a new iTop.  The iTop won't change if it doesn't need to
     * but it will change if:  1.  The iTop was deleted or 2.  We need to change
     * the iTop so that we fill the listbox.
     */
    xxxInsureVisible(plb, plb->iTop, FALSE);

FinishUpDelete:

    /*
     * Check if scroll bars need to be shown/hidden
     */
    plb->fFromInsert = TRUE;
    xxxLBShowHideScrollBars(plb);
    plb->fFromInsert = FALSE;

    xxxCheckRedraw(plb, TRUE, sItem);
    xxxInsureVisible(plb, plb->iSelBase, FALSE);

    return plb->cMac;
}

/***************************************************************************\
* xxxLBoxDeleteItem
*
* Sends a WM_DELETEITEM message to the owner of an ownerdraw listbox
*
* History:
\***************************************************************************/

void xxxLBoxDeleteItem(
    PLBIV plb,
    INT sItem)
{
    DELETEITEMSTRUCT dis;
    TL tlpwndParent;

    CheckLock(plb->spwnd);
    if (plb->spwnd == NULL)
        return;

    /*
     * Bug 262122 - joejo
     * No need to send message if no data!
     */
    if (!plb->fHasData) {
        return;
    }

    /*
     * Fill the DELETEITEMSTRUCT
     */
    dis.CtlType = ODT_LISTBOX;
    dis.CtlID = PtrToUlong(plb->spwnd->spmenu);
    dis.itemID = sItem;
    dis.hwndItem = HWq(plb->spwnd);

    /*
     * Bug 262122 - joejo
     * Fixed in 93 so that ItemData was passed. For some reason, not
     * merged in.
     */
    dis.itemData = LBGetItemData(plb, sItem);

    if (plb->spwndParent != NULL) {
        ThreadLock(plb->spwndParent, &tlpwndParent);
        SendMessage(HWq(plb->spwndParent), WM_DELETEITEM, dis.CtlID,
                (LPARAM)&dis);
        ThreadUnlock(&tlpwndParent);
    }
}

/**************************************************************************\
* xxxLBSetCount
*
* Sets the number of items in a lazy-eval (fNoData) listbox.
*
* Calling SetCount scorches any existing selection state.  To preserve
* selection state, call Insert/DeleteItem instead.
*
* History
* 16-Apr-1992 beng      Created
\**************************************************************************/

INT xxxLBSetCount(
    PLBIV plb,
    INT cItems)
{
    UINT  cbRequired;
    BOOL    fRedraw;

    CheckLock(plb->spwnd);

    /*
     * SetCount is only valid on lazy-eval ("nodata") listboxes.
     * All other lboxen must add their items one at a time, although
     * they may SetCount(0) via RESETCONTENT.
     */
    if (plb->fHasStrings || plb->fHasData) {
        RIPERR0(ERROR_SETCOUNT_ON_BAD_LB, RIP_VERBOSE, "");
        return LB_ERR;
    }

    if (cItems == 0) {
        SendMessageWorker(plb->spwnd, LB_RESETCONTENT, 0, 0, FALSE);
        return 0;
    }

    // If redraw isn't turned off, turn it off now
    if (fRedraw = plb->fRedraw)
        xxxLBSetRedraw(plb, FALSE);

    cbRequired = LBCalcAllocNeeded(plb, cItems);

    /*
     * Reset selection and position
     */
    plb->iSelBase = 0;
    plb->iTop = 0;
    plb->cMax = 0;
    plb->xOrigin = 0;
    plb->iLastSelection = 0;
    plb->iSel = -1;

    if (cbRequired != 0) { // Only if record instance data required

        /*
         * If listbox was previously empty, prepare for the
         * realloc-based alloc strategy ahead.
         */
        if (plb->rgpch == NULL) {
            plb->rgpch = UserLocalAlloc(HEAP_ZERO_MEMORY, 0L);
            plb->cMax = 0;

            if (plb->rgpch == NULL) {
                xxxNotifyOwner(plb, LBN_ERRSPACE);
                return LB_ERRSPACE;
            }
        }

        /*
         * rgpch might not have enough room for the new record instance
         * data, so check and realloc as necessary.
         */
        if (cItems >= plb->cMax) {
            INT    cMaxNew;
            UINT   cbNew;
            HANDLE hmemNew;

            /*
             * Since GrowMem presumes a one-item-at-a-time add schema,
             * SetCount can't use it.  Too bad.
             */
            cMaxNew = cItems+CITEMSALLOC;
            cbNew = LBCalcAllocNeeded(plb, cMaxNew);
            hmemNew = UserLocalReAlloc(plb->rgpch, cbNew, HEAP_ZERO_MEMORY);

            if (hmemNew == NULL) {
                xxxNotifyOwner(plb, LBN_ERRSPACE);
                return LB_ERRSPACE;
            }

            plb->rgpch = hmemNew;
            plb->cMax = cMaxNew;
        }

        /*
         * Reset the item instance data (multisel annotations)
         */
        RtlZeroMemory(plb->rgpch, cbRequired);
    }

    plb->cMac = cItems;

    // Turn redraw back on
    if (fRedraw)
        xxxLBSetRedraw(plb, TRUE);

    xxxLBInvalidateRect(plb, NULL, TRUE);
// Not In Chicago -- FritzS
//    NtUserSetScrollPos(plb->spwnd, SB_HORZ, 0, plb->fRedraw);
//    NtUserSetScrollPos(plb->spwnd, SB_VERT, 0, plb->fRedraw);
    xxxLBShowHideScrollBars(plb); // takes care of fRedraw

    return 0;
}

/**************************************************************************\
* LBCalcAllocNeeded
*
* Calculate the number of bytes needed in rgpch to accommodate a given
* number of items.
*
* History
* 16-Apr-1992 beng      Created
\**************************************************************************/

UINT LBCalcAllocNeeded(
    PLBIV plb,
    INT cItems)
{
    UINT cb;

    /*
     * Allocate memory for pointers to the strings.
     */
    cb = cItems * (plb->fHasStrings ? sizeof(LBItem)
                                    : (plb->fHasData ? sizeof(LBODItem)
                                                    : 0));

    /*
     * If multiple selection list box (MULTIPLESEL or EXTENDEDSEL), then
     * allocate an extra byte per item to keep track of it's selection state.
     */
    if (plb->wMultiple != SINGLESEL) {
        cb += cItems;
    }

    /*
     * Extra bytes for each item so that we can store its height.
     */
    if (plb->OwnerDraw == OWNERDRAWVAR) {
        cb += cItems;
    }

    return cb;
}
