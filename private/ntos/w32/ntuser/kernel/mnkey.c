/**************************** Module Header ********************************\
* Module Name: mnkey.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Menu Keyboard Handling Routines
*
* History:
* 10-10-90 JimA       Cleanup.
* 03-18-91 IanJa      Window revalidation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

int xxxClientFindMnemChar(
    PUNICODE_STRING pstrSrc,
    WCHAR ch,
    BOOL fFirst,
    BOOL fPrefix);

/* MenuSwitch commands */
#define CMDSWITCH   1
#define CMDQUERY    2

/***************************************************************************\
* FindNextValidMenuItem
*
* !
*
* History:
\***************************************************************************/

UINT MNFindNextValidItem(
    PMENU pMenu,
    int i,
    int dir,
    UINT flags)
{
    int iStart;
    BOOL cont = TRUE;
    int cItems = pMenu->cItems;
    PITEM pItem;

    if ((i < 0) && (dir > 0))
        // going forward from beginning -- stop after last menu item
        i = iStart = cItems;
    else if ((i >= cItems) && (dir < 0))
        // going backward from end -- stop after first menu item
        i = iStart = -1;
    else
        iStart = i;

    if (!cItems)
        return(MFMWFP_NOITEM);

    // b#8997 - if we have these conditions and enter
    // loop will go blistic ( infin )
    // fix: jump over code and come loop to i == iStart will now stop us
    if ( ( i == 0 ) && ( cItems == 1 ) && ( dir > 0 ) )
    {
        dir = 0;
        goto artsquickndirtybugfix;
    }

    //
    // Loop thru menu items til (1) we find a valid item
    //                       or (2) we make it back to the start item (iStart)
    while (TRUE) {
        i += dir;

        if ((i == iStart) || (dir == 0))
            // we made it back to start item -- return NOT FOUND
            return MFMWFP_NOITEM;

        // keep 'i' in the range: 0 <= i < cItems
        if (i >= cItems) {
            i = -1;
            continue;
        }
        else if (i < 0) {
            i = cItems;
            continue;
        }

artsquickndirtybugfix:
        pItem = pMenu->rgItems + i;

        // skip ownerdraw - seperators even though there not NULL
        if (TestMFT(pItem, MFT_SEPARATOR)) {
            //
            // Skip non-separator (if asked) empty items.  With hot-tracking,
            // it is acceptable for them to be selected.  In truth, it was possible
            // in Win3.1 too, but less likely.
            //
            if (!(flags & MNF_DONTSKIPSEPARATORS)) {
                continue;
            }
        } else if ((pItem->hbmp >= HBMMENU_MBARFIRST) && (pItem->hbmp <= HBMMENU_MBARLAST)) {
            /*
             * Skip close & minimize & restore buttons
             */
            continue;
        }

        // return index of found item
        return(i);
    }

    //
    // We should never get here!
    //
    UserAssert(FALSE);
}

/***************************************************************************\
* MKF_FindMenuItemInColumn
*
* Finds closest item in the pull-down menu's next "column".
*
* History:
\***************************************************************************/

UINT MNFindItemInColumn(
    PMENU pMenu,
    UINT idxB,
    int dir,
    BOOL fRoot)
{
    int dxMin;
    int dyMin;
    int dxMax;
    int dyMax;
    int xB;
    int yB;
    UINT idxE;
    UINT idxR;
    UINT cItems;
    PITEM pItem;

    cItems = pMenu->cItems;
    idxR = MFMWFP_NOITEM;
    idxE = MNFindNextValidItem(pMenu, MFMWFP_NOITEM, dir, 0);
    if (idxE == -1)
        goto End;

    dxMin = dyMin = 20000;

    if (idxB >= pMenu->cItems)
        return idxR;

    pItem = &pMenu->rgItems[idxB];
    xB = pItem->xItem;
    yB = pItem->yItem;

    while (cItems-- > 0 &&
            (idxB = MNFindNextValidItem(pMenu, idxB, dir, 0)) != idxE) {
        pItem = &pMenu->rgItems[idxB];
        dxMax = xB - pItem->xItem;
        dyMax = yB - pItem->yItem;

        if (dxMax < 0)
            dxMax = (-dxMax);
        if (dyMax < 0)
            dyMax = (-dyMax);

        // See if this item is nearer than the last item found
        // --------------------------------------------------------
        // (fRoot || dxMax) -- this condition means that if it's
        // not the actual menu bar menu that we're dealing with,
        // then the item below/above (same X value as) the selected
        // item is not a valid one to move to
        if ((dyMax < dyMin) && (fRoot || dxMax) && dxMax <= dxMin) {
            dxMin = dxMax;
            dyMin = dyMax;
            idxR = idxB;
        }
    }

End:
    return idxR;
}

/***************************************************************************\
* MKF_FindMenuChar
*
* Translates Virtual cursor key movements into pseudo-ascii values.  Maps a
* character to an item number.
*
* History:
\***************************************************************************/

UINT xxxMNFindChar(
    PMENU pMenu,
    UINT ch,
    int idxC,
    LPINT lpr)       /* Put match type here */
{
    int idxFirst = MFMWFP_NOITEM;
    int idxB;
    int idxF;
    int rT;
    LPWSTR lpstr;
    PITEM pItem;

    if (ch == 0)
        return 0;

    /*
     * First time thru go for the very first menu.
     */
    idxF = MFMWFP_NOITEM;
    rT = 0;
    idxB = idxC;

    if (idxB < 0)
//    if (idxB & 0x8000)
        idxB = MNFindNextValidItem(pMenu, pMenu->cItems, MFMWFP_NOITEM, MNF_DONTSKIPSEPARATORS);

    do {
        INT idxPrev;

        idxPrev = idxC;
        idxC = MNFindNextValidItem(pMenu, idxC, 1, MNF_DONTSKIPSEPARATORS);
        if (idxC == MFMWFP_NOITEM || idxC == idxFirst)
            break;
        if (idxFirst == MFMWFP_NOITEM)
            idxFirst = idxC;

        pItem = &pMenu->rgItems[idxC];

        if (pItem->lpstr != NULL) {
            if (pItem->cch != 0) {
                UNICODE_STRING strMnem;

                lpstr = TextPointer(pItem->lpstr);
                if (*lpstr == CH_HELPPREFIX) {

                    /*
                     * Skip help prefix if it is there so that we can mnemonic
                     * to the first character of a right justified string.
                     */
                    lpstr++;
                }

                RtlInitUnicodeString(&strMnem, lpstr);
                if (((rT = (UINT)xxxClientFindMnemChar(&strMnem,
                        (WCHAR)ch, TRUE, TRUE)) == 0x0080) &&
                        (idxF == MFMWFP_NOITEM))
                    idxF = idxC;
            }
        }
        if (idxC == idxPrev) {
            break;  // no progress - break inf. loop
        }
    } while (rT != 1 && idxB != idxC);

    *lpr = rT;

    if (rT == 1)
        return idxC;

    return idxF;
}


/***************************************************************************\
* xxxMenuKeyFilter
*
* !
*
* Revalidation notes:
* o Routine assumes it is called with pMenuState->hwndMenu non-NULL and valid.
* o If one or more of the popup menu windows is unexpectedly destroyed, this is
*   detected in xxxMenuWndProc(), which sets pMenuState->fSabotaged and calls
*   xxxKillMenuState().  Therefore, if we return from an xxxRoutine with
*   pMenuState->fSabotaged set, we must abort immediately.
* o If pMenuState->hwndMenu is unexpectedly destroyed, we abort only if we
*   need to use the corresponding pwndMenu.
* o pMenuState->hwndMenu may be supplied as a parameter to various routines
*   (eg:  xxxNextItem), whether valid or not.
* o Any label preceded with xxx (eg: xxxMKF_UnlockAndExit) may be reached with
*   pMenuState->hwndMenu invalid.
* o If this routine is not called while in xxxMenuLoop(), then it must
*   clear pMenuState->fSabotaged before returning.
*
* History:
\***************************************************************************/

void xxxMNKeyFilter(
    PPOPUPMENU ppopupMenu,
    PMENUSTATE pMenuState,
    UINT ch)
{
    BOOL fLocalInsideMenuLoop = pMenuState->fInsideMenuLoop;

    if (pMenuState->fButtonDown) {

        /*
         * Ignore keystrokes while the mouse is pressed (except ESC).
         */
        return;
    }

    if (!pMenuState->fInsideMenuLoop) {

        /*
         * Need to send the WM_INITMENU message before we pull down the menu.
         */
        if (!xxxMNStartMenu(ppopupMenu, KEYBDHOLD)) {
            return;
        }
        pMenuState->fInsideMenuLoop = TRUE;
    }


    switch (ch) {
    case 0:

        /*
         * If we get a WM_KEYDOWN alt key and then a KEYUP alt key, we need to
         * activate the first item on the menu.  ie.  user hits and releases alt
         * key so just select first item.  USER sends us a SC_KEYMENU with
         * lParam 0 when the user does this.
         */
        xxxMNSelectItem(ppopupMenu, pMenuState, 0);
        break;

    case MENUCHILDSYSMENU:
        if (!TestwndChild(ppopupMenu->spwndNotify)) {

            /*
             * Change made to fix MDI problem: child window gets a keymenu,
             * and pops up sysmenu of frame when maximized.  Need to act like
             * MENUCHAR if hwndMenu is a top-level.
             */
            goto MenuCharHandler;
        }

        /*
         * else fall through.
         */

    case MENUSYSMENU:
        if (!TestWF(ppopupMenu->spwndNotify, WFSYSMENU)) {
            xxxMessageBeep(0);
            goto MenuCancel;
        }

        /*
         * Popup any hierarchies we have.
         */
        xxxMNCloseHierarchy(ppopupMenu, pMenuState);
        if (!ppopupMenu->fIsSysMenu && ppopupMenu->spmenuAlternate)
            xxxMNSwitchToAlternateMenu(ppopupMenu);
        if (!ppopupMenu->fIsSysMenu) {
            /*
             * If no system menu, get out.
             */
            goto MenuCancel;
        }

        MNPositionSysMenu(ppopupMenu->spwndPopupMenu, ppopupMenu->spmenu);
        xxxMNSelectItem(ppopupMenu, pMenuState, 0);
        xxxMNOpenHierarchy(ppopupMenu, pMenuState);
        ppopupMenu->fToggle = FALSE;
        break;


    default:

        /*
         * Handle ALT-Character sequences for items on top level menu bar.
         * Note that fInsideMenuLoop may be set to false on return from this
         * function if the app decides to return 1 to the WM_MENUCHAR message.
         * We detect this and not enter MenuLoop if fInsideMenuLoop is reset
         * to false.
         */
MenuCharHandler:
        xxxMNChar(ppopupMenu, pMenuState, ch);
        if (ppopupMenu->posSelectedItem == MFMWFP_NOITEM) {
            /*
             * No selection found.
             */
            goto MenuCancel;
        }
        break;
    }

    if (!fLocalInsideMenuLoop && pMenuState->fInsideMenuLoop) {
        xxxMNLoop(ppopupMenu, pMenuState, 0, FALSE);
    }

    return;


MenuCancel:
    pMenuState->fModelessMenu = FALSE;
    if (!ppopupMenu->fInCancel) {
        xxxMNDismiss(pMenuState);
    }
    UserAssert(!pMenuState->fInsideMenuLoop && !pMenuState->fMenuStarted);
    return;
}
