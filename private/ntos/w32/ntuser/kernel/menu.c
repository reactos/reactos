
/**************************** Module Header ********************************\
* Module Name: menu.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Keyboard Accelerator Routines
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***********************************************************************\
* MNGetpItemIndex
*
* 11/19/96  GerardoB  Created
\***********************************************************************/
#if DBG
UINT DBGMNGetpItemIndex(PMENU pmenu, PITEM pitem)
{
    UINT uiPos;
    UserAssert((ULONG_PTR)pitem >= (ULONG_PTR)pmenu->rgItems);
    uiPos = _MNGetpItemIndex(pmenu, pitem);
    UserAssert(uiPos < pmenu->cItems);
    return uiPos;
}
#endif
/**************************************************************************\
* xxxMNDismiss
*
* 12/03/96 GerardoB     Created
\**************************************************************************/
void xxxMNDismiss (PMENUSTATE pMenuState)
{
    xxxMNCancel(pMenuState, 0, 0, 0);
}

/***************************************************************************\
* MNFadeSelection
*
* 2/5/1998   vadimg          created
\***************************************************************************/

BOOL MNFadeSelection(PMENU pmenu, PITEM pitem)
{
    PWND pwnd;
    HDC hdc;
    RECT rc;
    PPOPUPMENU ppopup;

    if (!TestALPHA(SELECTIONFADE))
        return FALSE;

    /*
     * Get the window for the currently active popup menu.
     */
    if ((ppopup = MNGetPopupFromMenu(pmenu, NULL)) == NULL)
        return FALSE;

    if ((pwnd = ppopup->spwndPopupMenu) == NULL)
        return FALSE;

    rc.left = pwnd->rcClient.left + pitem->xItem;
    rc.top = pwnd->rcClient.top + pitem->yItem;
    rc.right = rc.left + pitem->cxItem;
    rc.bottom = rc.top + pitem->cyItem;

    /*
     * Initialize the fade animation and get the DC to draw the selection into.
     */
    if ((hdc = CreateFade(NULL, &rc, CMS_SELECTIONFADE, 0)) == NULL)
        return FALSE;

    /*
     * Read the current menu selection right from the screen, since the menu
     * is still visible and it's always on top. In the worst case we could
     * offset the origin of the DC and call xxxDrawMenuItem, but just reading
     * from the screen is much faster.
     */
    GreBitBlt(hdc, 0, 0, pitem->cxItem, pitem->cyItem, gpDispInfo->hdcScreen,
            rc.left, rc.top, SRCCOPY, 0);

    ShowFade();

    return TRUE;
}

/**************************************************************************\
* xxxMNDismissWithNotify
*
* Generates parameters for WM_COMMAND or WM_SYSCOMMAND message.
*
* 12/03/96 GerardoB     Created
\**************************************************************************/
void xxxMNDismissWithNotify (PMENUSTATE pMenuState, PMENU pmenu, PITEM pitem,
        UINT uPos, LPARAM lParam)
{
    UINT uMsg;
    UINT uCmd;

    if (pMenuState->pGlobalPopupMenu->fIsSysMenu) {
        uMsg = WM_SYSCOMMAND;
        uCmd = pitem->wID;
        /* lParam set by caller */
    } else if (pMenuState->fNotifyByPos) {
        uMsg = WM_MENUCOMMAND;
        uCmd = uPos;
        lParam = (LPARAM)PtoHq(pmenu);
    } else {
        uMsg = WM_COMMAND;
        uCmd = pitem->wID;
        lParam = 0;
    }

    /*
     * The menu is about to go away, see if we want to fade out the selection.
     */
    if (MNFadeSelection(pmenu, pitem)) {
        StartFade();
    }

    /*
     * Dismiss the menu.
     */
    xxxMNCancel(pMenuState, uMsg, uCmd, lParam);
}

/**************************************************************************\
* MNGetpItem
*
* 11/15/96 GerardoB     Created
\**************************************************************************/
PITEM MNGetpItem(PPOPUPMENU ppopup, UINT uIndex)
{
    if ((ppopup == NULL)
            || (uIndex >= ppopup->spmenu->cItems)) {

       return NULL;
    }

    return ppopup->spmenu->rgItems + uIndex;
}
/***************************************************************************\
* MNSetCapture
*
* History:
* 11/18/96 GerardoB  Created
\***************************************************************************/
void xxxMNSetCapture (PPOPUPMENU ppopup)
{
    PTHREADINFO ptiCurrent = PtiCurrent();

    /*
     * Set the capture and lock it so no one will be able to steal it
     *  from us.
     */
    xxxCapture(ptiCurrent, ppopup->spwndNotify, SCREEN_CAPTURE);
    UserAssert (ptiCurrent->pq->spwndCapture == ppopup->spwndNotify);
    ptiCurrent->pq->QF_flags |= QF_CAPTURELOCKED;
    ptiCurrent->pMenuState->fSetCapture = TRUE;
#if DBG
    /*
     * Unless we're in the foreground, this menu mode won't go away
     *  when the user clicks outside the menu. This is because only
     *  the foreground queue capture sees clicks outside its windows.
     */
    if (ptiCurrent->pq != gpqForeground) {
        RIPMSG0(RIP_WARNING, "xxxMNSetCapture: Menu mode is not in foreground queue");
    }
#endif
}
/***************************************************************************\
* MNReleaseCapture
*
* History:
* 11/18/96 GerardoB  Created
\***************************************************************************/
void xxxMNReleaseCapture (void)
{
    PTHREADINFO ptiCurrent = PtiCurrent();

    /*
     * Bail if we didn't set capture
     */
    if ((ptiCurrent->pMenuState == NULL) ||
        (! ptiCurrent->pMenuState->fSetCapture)) {
        return;
    }
    ptiCurrent->pMenuState->fSetCapture = FALSE;

    /*
     * Unlock capture and release it.
     */
    PtiCurrent()->pq->QF_flags &= ~QF_CAPTURELOCKED;
    xxxReleaseCapture();
}
/***************************************************************************\
* MNCheckButtonDownState
*
* History:
* 11/14/96 GerardoB  Created
\***************************************************************************/
void MNCheckButtonDownState (PMENUSTATE pMenuState)
{
    /*
     * Modeless menus don't capture the mouse so when a mouse down
     *  goes off the window, we need to keep watching its state.
     * We also might not see the button up when going on DoDragDrop loop
     */
    UserAssert(pMenuState->fDragAndDrop || pMenuState->fModelessMenu);
    pMenuState->fButtonDown = ((_GetKeyState(pMenuState->vkButtonDown) & 0x8000) != 0);
    if (!pMenuState->fButtonDown) {
        pMenuState->fDragging =
        pMenuState->fIgnoreButtonUp = FALSE;
        UnlockMFMWFPWindow(&pMenuState->uButtonDownHitArea);
    }
}
/***************************************************************************\
* GetMenuStateWindow
*
* This function is called when we need to post a message to the menu loop.
* The actual pwnd is not important since we just want to reach
*  xxxHandleMenuMessages or xxxMenuWindowProc. So we just pick a window that
*  has a good chance to be around as long as we are in menu mode.
*
* History:
* 10/31/96 GerardoB  Created
\***************************************************************************/
PWND GetMenuStateWindow (PMENUSTATE pMenuState)
{
    if (pMenuState == NULL) {
        return NULL;
    } else if (pMenuState->pGlobalPopupMenu->fIsTrackPopup) {
        return pMenuState->pGlobalPopupMenu->spwndPopupMenu;
    } else if (pMenuState->pGlobalPopupMenu->spwndNextPopup != NULL) {
        return pMenuState->pGlobalPopupMenu->spwndNextPopup;
    } else {
        return pMenuState->pGlobalPopupMenu->spwndActivePopup;
    }
}
/***************************************************************************\
* UnlockPopupMenuWindow
*
* This function is called when locking/unlocking a menu into a popup structure.
* It makes sure that pmenu doesn't keep the notification window locked
*  unneccessarily
*
* It unlocks pmenu->spwndNotify if the menu it's not locked into pmenu->spwndNotify
*  itself AND it's currently locked to pwnd.
* It's also unlocked if pmenu->spwndNotify is marked as destroyed
*
* History:
* 10/15/96 GerardoB  Created
\***************************************************************************/
void UnlockPopupMenuWindow(PMENU pmenu, PWND pwnd)
{
    /*
     * Bail if there's nothing to unlock
     */
    if ((pmenu == NULL)
            || (pmenu->spwndNotify == NULL)) {
        return;
    }
    /*
     * if pmenu->spwndNotify owns the menu, bail
     */
    if ((pmenu == pmenu->spwndNotify->spmenu)
            || (pmenu == pmenu->spwndNotify->spmenuSys)) {
        return;
    }
    /*
     * If pwnd doesn't own the menu, and pmenu->spwndNotify is not destroyed, bail
     */
    if ((pwnd != pmenu->spwndNotify)
            && !TestWF(pmenu->spwndNotify, WFDESTROYED)) {
        return;
    }
    /*
     * Unlock it
     */
    Unlock(&pmenu->spwndNotify);
    return;
}
/***************************************************************************\
* LockPopupMenu
*
* Locks a given menu into a popup strucuture and makes the
*  popup's notification window the owner of the menu
*
* History:
* 10/15/96 GerardoB  Created
\***************************************************************************/
PVOID LockPopupMenu(PPOPUPMENU ppopup, PMENU * pspmenu, PMENU pmenu)
{
    /*
     * If you hit this assertion, you're probably not passing the right thing
     */
    UserAssert((pspmenu == &ppopup->spmenu) || (pspmenu == &ppopup->spmenuAlternate));
    Validateppopupmenu(ppopup);
    /*
     * This won't work properly if the popup hasn't locked the notification
     *  window.
     */
    UserAssert(ppopup->spwndNotify != NULL);

    /*
     * When using modeless menus, menus can be shared by several active popups.
     * If the menu has owner draw items, the app better knows how to draw them
     *  correctly. This shouldn't happen with modal menus though
     */
#if DBG
        if ((*pspmenu != NULL)
                && ((*pspmenu)->spwndNotify != NULL)
                && ((*pspmenu)->spwndNotify != ppopup->spwndNotify)) {

            RIPMSG3(RIP_WARNING, "LockPopupMenu: Current Menu %#p shared by %#p and %#p",
                    *pspmenu, (*pspmenu)->spwndNotify, ppopup->spwndNotify);
        }
#endif

    /*
     * Unlock the current's menu spwndNotify if needed
     */
    UnlockPopupMenuWindow(*pspmenu, ppopup->spwndNotify);

    /*
     * Lock the notification window into the menu structure
     */
    if (pmenu != NULL) {

        /*
         * Display a warning if this menu is being shared
         */
#if DBG
        if ((pmenu->spwndNotify != NULL)
                && (pmenu->spwndNotify != ppopup->spwndNotify)
                && (pmenu != pmenu->spwndNotify->head.rpdesk->spmenuDialogSys)) {

            RIPMSG3(RIP_WARNING, "LockPopupMenu: New Menu %#p shared by %#p and %#p",
                    pmenu, pmenu->spwndNotify, ppopup->spwndNotify);
        }
#endif

        /*
         * spwndNotify "owns" this menu now.
         */
        Lock(&pmenu->spwndNotify, ppopup->spwndNotify);
    }

    /*
     * Lock the menu into the popup structure (unlock the previous one)
     */
    return Lock(pspmenu, pmenu);
}
/***************************************************************************\
* UnlockPopupMenu
*
* Unlocks a given menu from a popup strucuture and makes sure that the
*  menu is no longer "owned" by the popup's notification window; if needed
*
* History:
* 10/15/96 GerardoB  Created
\***************************************************************************/
PVOID UnlockPopupMenu(PPOPUPMENU ppopup, PMENU * pspmenu)
{
    /*
     * If you hit this assertion, you're probably not passing the right thing
     */
    UserAssert((pspmenu == &ppopup->spmenu) || (pspmenu == &ppopup->spmenuAlternate));
    /*
     * If nothing is locked, bail.
     */
    if (*pspmenu == NULL) {
        return NULL;
    }

    /*
     * This won't work properly if the popup already unlocked the notification
     *  window. However, this can happen with the root popup if the
     *  notification window gets destroyed while in menu mode.
     */
    UserAssert((ppopup->spwndNotify != NULL) || IsRootPopupMenu(ppopup));

    /*
     * When using modeless menus, menus can be shared by several active
     *  popups/notification windows. If the menu has owner draw items,
     *  the app better knows how to paint them right. It shouldn't
     *  happen with modal menus though.
     */
#if DBG
    if (((*pspmenu)->spwndNotify != NULL)
            && (ppopup->spwndNotify != NULL)
            && (ppopup->spwndNotify != (*pspmenu)->spwndNotify)) {

        RIPMSG3(RIP_WARNING, "UnlockPopupMenu: Menu %#p shared by %#p and %#p",
                *pspmenu, (*pspmenu)->spwndNotify, ppopup->spwndNotify);
    }
#endif

    /*
     * Unlock the menu's spwndNotify if needed
     */
    UnlockPopupMenuWindow(*pspmenu, ppopup->spwndNotify);

    /*
     * Unlock the menu from the popup structure
     */
    return Unlock(pspmenu);
}
/***************************************************************************\
* LockWndMenu
*
* Locks a given menu into a window structure and locks the window into
*  the menu strucuture.
*
* History:
* 10/15/96 GerardoB  Created
\***************************************************************************/
PVOID LockWndMenu(PWND pwnd, PMENU * pspmenu, PMENU pmenu)
{
    /*
     * If you hit this assertion, you're probably not passing the right thing
     */
    UserAssert((pspmenu == &pwnd->spmenu) || (pspmenu == &pwnd->spmenuSys));

    /*
     * If the current menu is owned by this window, unlock it
     */
    if ((*pspmenu != NULL) && ((*pspmenu)->spwndNotify == pwnd)) {
        Unlock(&((*pspmenu)->spwndNotify));
    }

    /*
     * If nobody owns the new menu, make this window the owner
     */
    if ((pmenu != NULL) && (pmenu->spwndNotify == NULL)) {
        Lock(&pmenu->spwndNotify, pwnd);
    }

    /*
     * Lock the menu into the window structure (unlock the previous menu)
     */
    return Lock(pspmenu, pmenu);

}
/***************************************************************************\
* UnlockWndMenu
*
* Unlocks a given menu from a window strucutre and the window from the
*  menu strucuture
*
* History:
* 10/15/96 GerardoB  Created
\***************************************************************************/
PVOID UnlockWndMenu(PWND pwnd, PMENU * pspmenu)
{
    /*
     * If you hit this assertion, you're probably not passing the right thing
     */
    UserAssert((pspmenu == &pwnd->spmenu) || (pspmenu == &pwnd->spmenuSys));

    /*
     * If nothing is locked, bail
     */
    if (*pspmenu == NULL) {
        return NULL;
    }

    /*
     * If this window owns the menu, unlock it from the menu strucutre
     */
    if (pwnd == (*pspmenu)->spwndNotify) {
        Unlock(&((*pspmenu)->spwndNotify));
    }

    /*
     * Unlock the menu from the window structure
     */
    return Unlock(pspmenu);
}


/***************************************************************************\
* MNSetTop
*
*  sets the first visible item in a scrollable menu to the given iNewTop;
*  returns TRUE if iTop was changed; FALSE if iNewTop is already the
*  first visible item
*
* 08/13/96 GerardoB Ported From Memphis.
\***************************************************************************/
BOOL xxxMNSetTop(PPOPUPMENU ppopup, int iNewTop)
{
    PMENU   pMenu = ppopup->spmenu;
    int     dy;

    if (iNewTop < 0) {
        iNewTop = 0;
    } else if (iNewTop > pMenu->iMaxTop) {
        iNewTop = pMenu->iMaxTop;
    }

    /*
     * If no change, done
     */
    if (iNewTop == pMenu->iTop) {
        return FALSE;
    }

#if DBG
    /*
     * We're going to scroll, so validate iMaxTop, cyMax and cyMenu
     */
    UserAssert((pMenu->cyMax == 0) || (pMenu->cyMax >= pMenu->cyMenu));
    if ((UINT)pMenu->iMaxTop < pMenu->cItems) {
        PITEM pitemLast = pMenu->rgItems + pMenu->cItems - 1;
        PITEM pitemMaxTop = pMenu->rgItems + pMenu->iMaxTop;
        UINT uHeight = pitemLast->yItem + pitemLast->cyItem - pitemMaxTop->yItem;
        UserAssert(uHeight <= pMenu->cyMenu);
        /*
         * Let's guess a max item height
         */
        UserAssert(pMenu->cyMenu - uHeight <= 2 * pitemLast->cyItem);
    } else {
        UserAssert((UINT)pMenu->iMaxTop < pMenu->cItems);
    }
#endif


    /*
     * if we've made it this far, the new iTop WILL change -- thus if the
     * current iTop is at the top it won't be after this change -- same goes
     * for iTop at the bottom
     */
    if (pMenu->dwArrowsOn == MSA_ATTOP) {
        pMenu->dwArrowsOn = MSA_ON;
        if (pMenu->hbrBack == NULL) {
            MNDrawArrow(NULL, ppopup, MFMWFP_UPARROW);
        }
    } else if (pMenu->dwArrowsOn == MSA_ATBOTTOM) {
        pMenu->dwArrowsOn = MSA_ON;
        if (pMenu->hbrBack == NULL) {
            MNDrawArrow(NULL, ppopup, MFMWFP_DOWNARROW);
        }
    }

    UserAssert((UINT)iNewTop < pMenu->cItems);
    dy = MNGetToppItem(pMenu)->yItem - (pMenu->rgItems + iNewTop)->yItem;

    if ((dy > 0 ? dy : -dy) > (int)pMenu->cyMenu) {
        xxxInvalidateRect(ppopup->spwndPopupMenu, NULL, TRUE);
    } else {
        xxxScrollWindowEx(ppopup->spwndPopupMenu, 0, dy, NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_ERASE);
    }

    pMenu->iTop = iNewTop;

    if (iNewTop == 0) {
        pMenu->dwArrowsOn = MSA_ATTOP;
        if (pMenu->hbrBack == NULL) {
            MNDrawArrow(NULL, ppopup, MFMWFP_UPARROW);
        }
    } else if (iNewTop == pMenu->iMaxTop) {
        pMenu->dwArrowsOn = MSA_ATBOTTOM;
        if (pMenu->hbrBack == NULL) {
            MNDrawArrow(NULL, ppopup, MFMWFP_DOWNARROW);
        }
    }

    if (pMenu->hbrBack != NULL) {
        MNDrawFullNC(ppopup->spwndPopupMenu, NULL, ppopup);
    }

    return TRUE;

}

/***************************************************************************\
* xxxMNDoScroll
*
*  scrolls a scrollable menu (ppopup) if the given position (uArrow) is one of
*  the menu scroll arrows and sets a timer to auto-scroll when necessary;
*  returns FALSE if the given position was not a menu scroll arrow; returns
*  TRUE otherwise
*
* 08/13/96 GerardoB Ported From Memphis.
\***************************************************************************/
BOOL xxxMNDoScroll(PPOPUPMENU ppopup, UINT uArrow, BOOL fSetTimer)
{
    int iScrollTop = ppopup->spmenu->iTop;

    if (uArrow == MFMWFP_UPARROW) {
        iScrollTop--;
    } else if (uArrow == MFMWFP_DOWNARROW) {
        iScrollTop++;
    } else {
        return FALSE;
    }

    if (!xxxMNSetTop(ppopup, iScrollTop)) {
        if (!fSetTimer) {
            _KillTimer(ppopup->spwndPopupMenu, uArrow);
        }
    } else {
        /*
         * Set this timer just like we do in the scrollbar code:
         * the first time we wait a little longer.
         */
        _SetTimer(ppopup->spwndPopupMenu, uArrow,
                  (fSetTimer ? gpsi->dtScroll : gpsi->dtScroll / 4), NULL);
    }

    return TRUE;
}

/***************************************************************************\
* MNCheckScroll
*
*  checks to see if the given menu (pMenu) can be displayed in it's entirety
*  or if it can't, in which case it sets the menu to be scrollable
*
* 08/13/96 GerardoB Ported From Memphis.
\***************************************************************************/
int MNCheckScroll(PMENU pMenu, PMONITOR pMonitor)
{
    int     i;
    UINT    cyMax;
    PITEM   pItem;

    /*
     * Max height that fits on the monitor
     */
    cyMax = (pMonitor->rcMonitor.bottom - pMonitor->rcMonitor.top);

    /*
     * If the menu has a valid max height, use it
     */
    if ((pMenu->cyMax != 0) && (pMenu->cyMax < cyMax)) {
        cyMax = pMenu->cyMax;
    }

    /*
     * Bail if menu is either empty, multicolumn, or able to fit
     *   without scrolling
     */
    if ((pMenu->rgItems == 0)
            || (pMenu->rgItems->cxItem != pMenu->cxMenu)
            || (pMenu->cyMenu + (2 * SYSMET(CYFIXEDFRAME)) <= cyMax))  {

        pMenu->dwArrowsOn = MSA_OFF;
        pMenu->iTop = 0;
        pMenu->iMaxTop = 0;
        return pMenu->cyMenu;
    }

    /*
     * Max client height
     */
    cyMax -= 2 * (SYSMET(CYFIXEDFRAME) + gcyMenuScrollArrow);

    /*
     * Determine the menu height
     * Find the first item that won't fit.
     */
    pItem = pMenu->rgItems;
    for (i = 0; i < (int)pMenu->cItems; i++, pItem++) {
        if (pItem->yItem > (UINT)cyMax) {
            break;
        }
    }
    if (i != 0) {
        pItem--;
    }
    pMenu->cyMenu = pItem->yItem;

    /*
     * compute the last possible top item when all remaining items are fully
     * visible
     */
    cyMax = 0;
    i = pMenu->cItems - 1;
    pItem = pMenu->rgItems + i;
    for (; i >= 0; i--, pItem--) {
        cyMax += pItem->cyItem;
        if (cyMax > pMenu->cyMenu) {
            break;
        }
    }
    if ((UINT)i != pMenu->cItems - 1) {
        i++;
    }
    pMenu->iMaxTop = i;

    /*
     * Update top item and scroll state
     */
    if (pMenu->iTop > i) {
        pMenu->iTop = i;
    }

    if (pMenu->iTop == i) {
        pMenu->dwArrowsOn = MSA_ATBOTTOM;
    } else if (pMenu->iTop == 0) {
        pMenu->dwArrowsOn = MSA_ATTOP;
    } else {
        pMenu->dwArrowsOn = MSA_ON;
    }

    /*
     * This is funtion is called by MN_SIZEWINDOW which doesn't check
     *  if the scroll bars are present but simply adds (2 * SYSMET(CYFIXEDFRAME))
     *  to calculate the window height. So we add the scrollbars height
     *  here. (I believe MN_SIZEWINDOW is a private-but-publicly-known message)
     */
    return (pMenu->cyMenu + (2 * gcyMenuScrollArrow));
}

/***************************************************************************\
* MNIsPopupItem
*
*
\***************************************************************************/

BOOL MNIsPopupItem(ITEM *lpItem)
{
    return((lpItem) && (lpItem->spSubMenu) &&
        !TestMFS(lpItem, MFS_GRAYED));
}

/***************************************************************************\
* Validateppopupmenu
*
* 05-15-96 GerardoB  Created
\***************************************************************************/
#if DBG
void Validateppopupmenu (PPOPUPMENU ppopupmenu)
{
    UserAssert(ppopupmenu != NULL);
    try {
        UserAssert(!ppopupmenu->fFreed);

        /*
         * If this popup is being destroyed to soon, ppopupmenuRoot can be NULL
         */
         if (ppopupmenu->ppopupmenuRoot != NULL) {
             if (ppopupmenu->ppopupmenuRoot != ppopupmenu) {
                 /*
                  * This must be a hierarchical popup
                  */
                 UserAssert(ppopupmenu->spwndPrevPopup != NULL);
                 UserAssert(!ppopupmenu->fIsMenuBar && !ppopupmenu->fIsTrackPopup);
                 Validateppopupmenu(ppopupmenu->ppopupmenuRoot);
             } else {
                 /*
                  * This must be the root popupmenu
                  */
                 UserAssert(ppopupmenu->spwndPrevPopup == NULL);
                 UserAssert(ppopupmenu->fIsMenuBar || ppopupmenu->fIsTrackPopup);
             }
         }

         /*
          * This can be NULL when called from xxxDeleteThreadInfo
          */
         if (ppopupmenu->spwndPopupMenu != NULL) {
             UserAssert(ppopupmenu->spwndPopupMenu == RevalidateCatHwnd(HW(ppopupmenu->spwndPopupMenu)));
         }

         /*
          * This can be NULL when called from xxxDestroyWindow (spwndNotify)
          *  or from xxxDeleteThreadInfo
          */
         if (ppopupmenu->spwndNotify != NULL) {
             UserAssert(ppopupmenu->spwndNotify == RevalidateCatHwnd(HW(ppopupmenu->spwndNotify)));
         }

    } except (W32ExceptionHandler(FALSE, RIP_ERROR)) {
        RIPMSG1(RIP_ERROR, "Validateppopupmenu: Invalid popup:%#p", ppopupmenu);
    }
}
#endif
/***************************************************************************\
* xxxSwitchToAlternateMenu
*
* Switches to the alternate popupmenu. Returns TRUE if we switch
* else FALSE.
*
* History:
* 05-25-91 Mikehar      Ported from Win3.1
\***************************************************************************/

BOOL xxxMNSwitchToAlternateMenu(
    PPOPUPMENU ppopupmenu)
{
    PMENU pmenuSwap = NULL;
    PMENUSTATE pMenuState;
    TL tlpwndPopupMenu;

    if (!ppopupmenu->fIsMenuBar || !ppopupmenu->spmenuAlternate) {
        /*
         * Do nothing if no menu or not top level menu bar.
         * ppopupmenu->spmenuAlternate can be NULL when an app has
         *  either system menu or menu bar but not both. If that menu
         *  has only one popup that it's not dropped, then hitting
         *  VK_RIGHT or VK_LEFT causes xxxMNKeyDown to end up here.
         * ppopupmenu->fIsMenuBar can be false when you drop the
         *  system menu of an app with no menu bar; then hit VK_RIGHT
         *  on an item that doesn't have a popup and you'll get here
         * There might be some other situations like this; in any case
         *  the assertion got to go
         */
        return FALSE;
    }

    /*
     * If we're getting out of menu mode, do nothing
     */
    if (ppopupmenu->fDestroyed) {
        return FALSE;
    }

    /*
     * Select no items in the current menu.
     */
    ThreadLock(ppopupmenu->spwndPopupMenu, &tlpwndPopupMenu);
    UserAssert(ppopupmenu->spwndPopupMenu != NULL);
    pMenuState = GetpMenuState(ppopupmenu->spwndPopupMenu);
    if (pMenuState == NULL) {
        RIPMSG0(RIP_ERROR, "xxxMNSwitchToAlternateMenu: pMenuState == NULL");
        ThreadUnlock(&tlpwndPopupMenu);
        return FALSE;
    }
    xxxMNSelectItem(ppopupmenu, pMenuState, MFMWFP_NOITEM);


    UserAssert(ppopupmenu->spmenu->spwndNotify == ppopupmenu->spmenuAlternate->spwndNotify);
    Lock(&pmenuSwap, ppopupmenu->spmenuAlternate);
    Lock(&ppopupmenu->spmenuAlternate, ppopupmenu->spmenu);
    Lock(&ppopupmenu->spmenu, pmenuSwap);
    Unlock(&pmenuSwap);

    /*
     * Set this global because it is used in SendMenuSelect()
     */
    if (!TestWF(ppopupmenu->spwndNotify, WFSYSMENU)) {
        pMenuState->fIsSysMenu = FALSE;
    } else if (ppopupmenu->spwndNotify->spmenuSys != NULL) {
        pMenuState->fIsSysMenu = (ppopupmenu->spwndNotify->spmenuSys ==
                ppopupmenu->spmenu);
    } else {
        pMenuState->fIsSysMenu = !!TestMF(ppopupmenu->spmenu, MFSYSMENU);
    }

    ppopupmenu->fIsSysMenu = pMenuState->fIsSysMenu;

    if (FWINABLE()) {
        xxxWindowEvent(EVENT_SYSTEM_MENUEND, ppopupmenu->spwndNotify,
            (ppopupmenu->fIsSysMenu ? OBJID_MENU : OBJID_SYSMENU), INDEXID_CONTAINER, 0);
        xxxWindowEvent(EVENT_SYSTEM_MENUSTART, ppopupmenu->spwndNotify,
            (ppopupmenu->fIsSysMenu ? OBJID_SYSMENU : OBJID_MENU), INDEXID_CONTAINER, 0);
    }

    ThreadUnlock(&tlpwndPopupMenu);

    return TRUE;
}

/***************************************************************************\
* MenuDestroyHandler
*
* cleans up after this menu
*
* History:
* 05-25-91 Mikehar      Ported from Win3.1
\***************************************************************************/

void xxxMNDestroyHandler(
    PPOPUPMENU ppopupmenu)
{
    PITEM pItem;
    TL tlpwndT;

    if (ppopupmenu == NULL) {
        /*
         * This can happen if WM_NCCREATE failed to allocate the ppopupmenu
         * in xxxMenuWindowProc.
         */
        RIPMSG0(RIP_WARNING, "xxxMNDestroyHandler: NULL \"ppopupmenu\"");
        return;
    }

#if DBG
    /*
     * When destroying a desktop's spwndMenu that is not in use (i.e., the
     *  desktop is going away), the ppopupmenu is not exactly valid (i.e.,
     *  we're not in menu mode) but it should be properly NULLed out so
     *  everything should go smoothly
     */
    Validateppopupmenu(ppopupmenu);
#endif

    if (ppopupmenu->spwndNextPopup != NULL) {
        /*
         * We used to send the message to spwndNextPopup here. The message should
         *  go to the current popup so it'll close spwndNextPopup (not to the next
         *  to close its next, if any).
         * I don't see how the current spwndPopupMenu can be NULL but we better
         *  handle it since we never accessed it before. This menu code is tricky...
         */
        PWND pwnd;
        UserAssert(ppopupmenu->spwndPopupMenu != NULL);
        pwnd = (ppopupmenu->spwndPopupMenu != NULL ? ppopupmenu->spwndPopupMenu : ppopupmenu->spwndNextPopup);
        ThreadLockAlways(pwnd, &tlpwndT);
        xxxSendMessage(pwnd, MN_CLOSEHIERARCHY, 0, 0);
        ThreadUnlock(&tlpwndT);
    }

    if ((ppopupmenu->spmenu!=NULL) && MNIsItemSelected(ppopupmenu))
    {
        /*
         * Unset the hilite bit on the hilited item.
         */
        if (ppopupmenu->posSelectedItem < ppopupmenu->spmenu->cItems) {
            /*
             * this extra check saves Ambiente 1.02 -- they have a menu with
             * one item in it.  When that command is chosen, the app proceeds
             * to remove that one item -- leaving us in the oh so strange state
             * of having a valid hMenu with a NULL rgItems.
             */
            pItem = &(ppopupmenu->spmenu->rgItems[ppopupmenu->posSelectedItem]);
            pItem->fState &= ~MFS_HILITE;
        }
    }

    if (ppopupmenu->fShowTimer) {
        _KillTimer(ppopupmenu->spwndPopupMenu, IDSYS_MNSHOW);
    }

    if (ppopupmenu->fHideTimer) {
        _KillTimer(ppopupmenu->spwndPopupMenu, IDSYS_MNHIDE);
    }

    /*
     * Send WM_UNINITMENUPOPUP so the menu owner can clean up.
     */
    if (ppopupmenu->fSendUninit
            && (ppopupmenu->spwndNotify != NULL)) {

        ThreadLockAlways(ppopupmenu->spwndNotify, &tlpwndT);
        xxxSendMessage(ppopupmenu->spwndNotify, WM_UNINITMENUPOPUP,
                       (WPARAM)PtoH(ppopupmenu->spmenu),
                        MAKELONG(0, (ppopupmenu->fIsSysMenu ? MF_SYSMENU: 0)));
        ThreadUnlock(&tlpwndT);
    }

    ppopupmenu->fDestroyed = TRUE;
    if (!ppopupmenu->fDesktopMenu) {

        if (ppopupmenu->spwndPopupMenu != NULL) {
            ((PMENUWND)(ppopupmenu->spwndPopupMenu))->ppopupmenu = NULL;
        }

    }

    if (!ppopupmenu->fDelayedFree) {
        MNFreePopup(ppopupmenu);
    } else if (ppopupmenu->ppopupmenuRoot != NULL) {
        ppopupmenu->ppopupmenuRoot->fFlushDelayedFree = TRUE;
        #if DBG
        {
            /*
             * If this is not the rootpopup,
             * assert that this popup is linked in the delayed free list
             */
            if (!IsRootPopupMenu(ppopupmenu)) {
                BOOL fFound = FALSE;
                PPOPUPMENU ppm = ppopupmenu->ppopupmenuRoot;
                while (ppm->ppmDelayedFree != NULL) {
                    if (ppm->ppmDelayedFree == ppopupmenu) {
                        fFound = TRUE;
                        break;
                    }
                    ppm = ppm->ppmDelayedFree;
                }
                UserAssert(fFound);
            }
        }
        #endif
    } else {
        UserAssertMsg1(FALSE, "Leaking ppopupmenu:%p?", ppopupmenu);
    }

}


/***************************************************************************\
* MenuCharHandler
*
* Handles char messages for the given menu. This procedure is called
* directly if the menu char is for the top level menu bar else it is called
* by the menu window proc on behalf of the window that should process the
* key.
*
* History:
* 05-25-91 Mikehar      Ported from Win3.1
\***************************************************************************/

void xxxMNChar(
    PPOPUPMENU ppopupmenu,
    PMENUSTATE pMenuState,
    UINT character)
{
    PMENU pMenu;
    UINT flags;
    LRESULT result;
    int item;
    INT matchType;
    BOOL    fExecute = FALSE;
    TL tlpwndNotify;

    pMenu = ppopupmenu->spmenu;

    Validateppopupmenu(ppopupmenu);

    /*
     * If this comes in with a NULL pMenu, then we could have problems.
     * This could happen if the xxxMNStartMenuState never gets called
     * because the fInsideMenuLoop is set.
     *
     * This is placed in here temporarily until we can discover why
     * this pMenu isn't set.  We will prevent the system from crashing
     * in the meantime.
     *
     * HACK: ChrisWil
     */
    if (pMenu == NULL) {
        UserAssert(pMenu);
        xxxMNDismiss(pMenuState);
        return;
    }

    /*
     * If we're getting out of menu mode, bail
     */
    if (ppopupmenu->fDestroyed) {
        return;
    }

    item = xxxMNFindChar(pMenu, character,
            ppopupmenu->posSelectedItem, &matchType);
    if (item != MFMWFP_NOITEM) {
        int item1;
        int firstItem = item;

        /*
         * Find first ENABLED menu item with the given mnemonic 'character'
         * !!!  If none found, exit menu loop  !!!
         */
        while (pMenu->rgItems[item].fState & MFS_GRAYED) {
            item = xxxMNFindChar(pMenu, character, item, &matchType);
            if (item == firstItem) {
                xxxMNDismiss(pMenuState);
                return;
            }
        }
        item1 = item;

        /*
         * Find next ENABLED menu item with the given mnemonic 'character'
         * This is to see if we have a DUPLICATE MNEMONIC situation
         */
        do {
            item = xxxMNFindChar(pMenu, character, item, &matchType);
        } while ((pMenu->rgItems[item].fState & MFS_GRAYED) && (item != firstItem));

        if ((firstItem == item) || (item == item1))
            fExecute = TRUE;

        item = item1;
    }

    if ((item == MFMWFP_NOITEM) && ppopupmenu->fIsMenuBar && (character == TEXT(' '))) {

        /*
         * Handle the case of the user cruising through the top level menu bar
         * without any popups dropped.  We need to handle switching to and from
         * the system menu.
         */
        if (ppopupmenu->fIsSysMenu) {

            /*
             * If we are on the system menu and user hits space, bring
             * down thesystem menu.
             */
            item = 0;
            fExecute = TRUE;
        } else if (ppopupmenu->spmenuAlternate != NULL) {

            /*
             * We are not currently on the system menu but one exists.  So
             * switch to it and bring it down.
             */
            item = 0;
            goto SwitchToAlternate;
        }
    }

    if ((item == MFMWFP_NOITEM) && ppopupmenu->fIsMenuBar && ppopupmenu->spmenuAlternate) {

        /*
         * No matching item found on this top level menu (could be either the
         * system menu or the menu bar).  We need to check the other menu.
         */
        item = xxxMNFindChar(ppopupmenu->spmenuAlternate,
                character, 0, &matchType);

        if (item != MFMWFP_NOITEM) {
SwitchToAlternate:
            if (xxxMNSwitchToAlternateMenu(ppopupmenu)) {
                xxxMNChar(ppopupmenu, pMenuState, character);
            }
            return;
        }
    }

    if (item == MFMWFP_NOITEM) {
        flags = (ppopupmenu->fIsSysMenu) ? MF_SYSMENU : 0;

        if (!ppopupmenu->fIsMenuBar) {
            flags |= MF_POPUP;
        }

        ThreadLock(ppopupmenu->spwndNotify, &tlpwndNotify);
        result = xxxSendMessage(ppopupmenu->spwndNotify, WM_MENUCHAR,
                MAKELONG((WORD)character, (WORD)flags),
                (LPARAM)PtoH(ppopupmenu->spmenu));
        ThreadUnlock(&tlpwndNotify);

        switch (HIWORD(result)) {
        case MNC_IGNORE:
            xxxMessageBeep(0);
            /*
             * If we're on the menu bar, cancel menu mode (fall through).
             * We do this because you can really freak out an end user
             *  who accidentally tapped the Alt key (causing us to go
             *  into "invisible" menu mode) and now can't type anything!
             */
            if (flags & MF_POPUP) {
                return;
            }
            /*
             * Fall through.
             */

        case MNC_CLOSE:
            xxxMNDismiss(pMenuState);
            return;

        case MNC_EXECUTE:
            fExecute = TRUE;
            /* fall thru */

        case MNC_SELECT:
            item = (UINT)(short)LOWORD(result);
            if ((WORD) item >= ppopupmenu->spmenu->cItems)
            {
                RIPMSG1(RIP_WARNING, "Invalid item number returned from WM_MENUCHAR %#lx", result);
                return;
            }
            break;
        }
    }

    if (item != MFMWFP_NOITEM) {
        xxxMNSelectItem(ppopupmenu, pMenuState, item);

        if (fExecute)
            xxxMNKeyDown(ppopupmenu, pMenuState, VK_RETURN);
    }
}

/***************************************************************************\
*
*  GetMenuInheritedContextHelpId(PPOPUPMENU  ppopup)
* Given a ppopup, this function will see if that menu has a context help
*  id and return it. If it does not have a context help id, it will look up
*  in the parent menu, parent of the parent etc., all the way to the top
*  top level menu bar till it finds a context help id and returns it. If no
*  context help id is found, it returns a zero.
*
\***************************************************************************/

DWORD GetMenuInheritedContextHelpId(PPOPUPMENU  ppopup)
{
  PWND  pWnd;

  /*
   * If we are already at the menubar, simply return it's ContextHelpId
   */
  UserAssert(ppopup != NULL);
  if (ppopup->fIsMenuBar)
      goto Exit_GMI;

  while(TRUE) {
      UserAssert(ppopup != NULL);

      /*
       * See if the given popup has a context help id.
       */
      if (ppopup->spmenu->dwContextHelpId) {
          /* Found the context Id */
          break;
      }

      /*
       * Get the previous popup menu;
       * Check if the previous menu is the menu bar.
       */
      if (  (ppopup->fHasMenuBar) &&
            (ppopup->spwndPrevPopup == ppopup->spwndNotify)) {

          ppopup = ppopup -> ppopupmenuRoot;
          break;
      } else {
          /*
           * See if this has a valid prevPopup; (it could be TrackPopup menu)
           */
          if ((pWnd = ppopup -> spwndPrevPopup) == NULL) {
              return ((DWORD)0);
          }

          ppopup = ((PMENUWND)pWnd)->ppopupmenu;
      }
  }

Exit_GMI:
  return(ppopup->spmenu->dwContextHelpId);
}

/***************************************************************************\
* void MenuKeyDownHandler(PPOPUPMENU ppopupmenu, UINT key)
* effects: Handles a keydown for the given menu.
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/

void xxxMNKeyDown(
    PPOPUPMENU ppopupmenu,
    PMENUSTATE pMenuState,
    UINT key)
{
    LRESULT dwMDIMenu;
    UINT item;
    BOOL fHierarchyWasDropped = FALSE;
    TL tlpwndT;
    PPOPUPMENU ppopupSave;
    BOOL bFakedKey;
    UINT keyOrig = key;

    /*
     * Blow off keyboard if mouse down.
     */
    if ((pMenuState->fButtonDown) && (key != VK_F1)) {
        /*
         * Check if the user wants to cancel dragging.
         */
        if (pMenuState->fDragging && (key == VK_ESCAPE)) {
            RIPMSG0(RIP_WARNING, "xxxMNKeyDown: ESC while dragging");
            pMenuState->fIgnoreButtonUp = TRUE;
        }

        return;
    }

    switch (key) {
    case VK_MENU:
    case VK_F10:
    {
        /*
         * Modeless don't go away when the menu key is hit. They just
         *  ignore it.
         */
        if (pMenuState->fModelessMenu) {
            return;
        }

        if (gwinOldAppHackoMaticFlags & WOAHACK_CHECKALTKEYSTATE) {

            /*
             * Winoldapp is telling us to put up/down the system menu.  Due to
             * possible race conditions, we need to check the state of the alt
             * key before throwing away the menu.
             */
            if (gwinOldAppHackoMaticFlags & WOAHACK_IGNOREALTKEYDOWN) {
                return;
            }
        }
        xxxMNDismiss(pMenuState);

        /*
         * We're going to exit menu mode but the ALT key is down, so clear
         *  pMenuState->fUnderline to cause xxxMNLoop not to erase the underlines
         */
        if (key == VK_MENU) {
            pMenuState->fUnderline = FALSE;
        }
    }
        return;

    case VK_ESCAPE:

        /*
         * Escape key was hit.  Get out of one level of menus.  If no active
         * popups or we are minimized and there are no active popups below
         * this, we need to get out of menu mode.  Otherwise, we popup up
         * one level in the hierarchy.
         */
        if (ppopupmenu->fIsMenuBar ||
                ppopupmenu == ppopupmenu->ppopupmenuRoot ||
                TestWF(ppopupmenu->ppopupmenuRoot->spwndNotify, WFMINIMIZED)) {
            xxxMNDismiss(pMenuState);
        } else {
            /*
             * Pop back one level of menus.
             */
            if (ppopupmenu->fHasMenuBar &&
                    ppopupmenu->spwndPrevPopup == ppopupmenu->spwndNotify) {

                PPOPUPMENU ppopupmenuRoot = ppopupmenu->ppopupmenuRoot;

                ppopupmenuRoot->fDropNextPopup = FALSE;

#if 0
                /*
                 * We are on a menu bar hierarchy and there is only one popup
                 * visible.  We have to cancel this popup and put focus back on
                 * the menu bar.
                 */
                if (_IsIconic(ppopupmenuRoot->spwndNotify)) {

                    /*
                     * However, if we are iconic there really is no menu
                     * bar so let's make it easier for users and get out
                     * of menu mode completely.
                     */
                    xxxMNDismiss(pMenuState);
                } else
#endif
                    /*
                     * If the popup is closed, a modeless menu won't
                     *  have a window to get the keys. So modeless menu
                     *  cancel the menu at this point. Modal menus go
                     *  to the menu bar.
                     */
                    if (pMenuState->fModelessMenu) {
                        xxxMNDismiss(pMenuState);
                    } else {
                        xxxMNCloseHierarchy(ppopupmenuRoot, pMenuState);
                    }
            } else {
                ThreadLock(ppopupmenu->spwndPrevPopup, &tlpwndT);
                xxxSendMessage(ppopupmenu->spwndPrevPopup, MN_CLOSEHIERARCHY,
                        0, 0);
                ThreadUnlock(&tlpwndT);
            }
        }
        return;

    case VK_UP:
    case VK_DOWN:
        if (ppopupmenu->fIsMenuBar) {

            /*
             * If we are on the top level menu bar, try to open the popup if
             * possible.
             */
            if (xxxMNOpenHierarchy(ppopupmenu, pMenuState) == (PWND)-1)
                return;
        } else {
            item = MNFindNextValidItem(ppopupmenu->spmenu,
                    ppopupmenu->posSelectedItem, (key == VK_UP ? -1 : 1), 0);
            xxxMNSelectItem(ppopupmenu, pMenuState, item);
        }
        return;

    case VK_LEFT:
    case VK_RIGHT:
#ifdef USE_MIRRORING
            bFakedKey = (!!ppopupmenu->fRtoL) ^ (!!TestWF(ppopupmenu->spwndPopupMenu, WEFLAYOUTRTL));
#else
            bFakedKey = ppopupmenu->fRtoL;
#endif
        if (bFakedKey)
            /*
             * turn the keys around, we drew the menu backwards.
             */
            key = (key == VK_LEFT) ? VK_RIGHT : VK_LEFT;
        if (!ppopupmenu->fIsMenuBar && (key == VK_RIGHT) &&
                !ppopupmenu->spwndNextPopup) {
            /*
             * Try to open the hierarchy at this item if there is one.
             */
            if (xxxMNOpenHierarchy(ppopupmenu, pMenuState) == (PWND)-1)
                return;
            if (ppopupmenu->fHierarchyDropped) {
                return;
            }
        }

        if (ppopupmenu->spwndNextPopup) {
            fHierarchyWasDropped = TRUE;
            if ((key == VK_LEFT) && !ppopupmenu->fIsMenuBar) {
                xxxMNCloseHierarchy(ppopupmenu, pMenuState);
                return;
            }
        } else if (ppopupmenu->fDropNextPopup)
            fHierarchyWasDropped = TRUE;

        ppopupSave = ppopupmenu;

        item = MNFindItemInColumn(ppopupmenu->spmenu,
                ppopupmenu->posSelectedItem,
                (key == VK_LEFT ? -1 : 1),
                (ppopupmenu->fHasMenuBar &&
                ppopupmenu == ppopupmenu->ppopupmenuRoot));

        if (item == MFMWFP_NOITEM) {

            /*
             * No valid item found in the given direction so send it up to our
             * parent to handle.
             */
            if (ppopupmenu->fHasMenuBar &&
                    ppopupmenu->spwndPrevPopup == ppopupmenu->spwndNotify) {

                /*
                 * if we turned the key round, then turn it back again.
                 */
                if (bFakedKey)
                    key = (key == VK_LEFT) ? VK_RIGHT : VK_LEFT;
                /*
                 * Go to next/prev item in menu bar since a popup was down and
                 * no item on the popup to go to.
                 */
                xxxMNKeyDown(ppopupmenu->ppopupmenuRoot, pMenuState, key);
                return;
            }

            if (ppopupmenu == ppopupmenu->ppopupmenuRoot) {
                if (!ppopupmenu->fIsMenuBar) {

                    /*
                     * No menu bar associated with this menu so do nothing.
                     */
                    return;
                }
            } else {
                ThreadLock(ppopupmenu->spwndPrevPopup, &tlpwndT);
                xxxSendMessage(ppopupmenu->spwndPrevPopup, WM_KEYDOWN, keyOrig, 0);
                ThreadUnlock(&tlpwndT);
                return;
            }
        }

        if (!ppopupmenu->fIsMenuBar) {
            if (item != MFMWFP_NOITEM) {
                xxxMNSelectItem(ppopupmenu, pMenuState, item);
            }
            return;

        } else {

            /*
             * Special handling if keydown occurred on a menu bar.
             */
            if (item == MFMWFP_NOITEM) {

                if (TestWF(ppopupmenu->spwndNotify, WFSYSMENU)) {
                    PTHREADINFO ptiCurrent = PtiCurrent();
                    PWND    pwndNextMenu;
                    PMENU   pmenuNextMenu, pmenuUse;
                    MDINEXTMENU mnm;
                    TL tlpmenuNextMenu;
                    TL tlpwndNextMenu;

                    mnm.hmenuIn = (HMENU)0;
                    mnm.hmenuNext = (HMENU)0;
                    mnm.hwndNext = (HWND)0;

                    /*
                     * We are in the menu bar and need to go up to the system menu
                     * or go from the system menu to the menu bar.
                     */
                    pmenuNextMenu = ppopupmenu->fIsSysMenu ?
                        _GetSubMenu(ppopupmenu->spmenu, 0) :
                        ppopupmenu->spmenu;
                    mnm.hmenuIn = PtoH(pmenuNextMenu);
                    ThreadLock(ppopupmenu->spwndNotify, &tlpwndT);
                    dwMDIMenu = xxxSendMessage(ppopupmenu->spwndNotify,
                        WM_NEXTMENU, (WPARAM)keyOrig, (LPARAM)&mnm);
                    ThreadUnlock(&tlpwndT);

                    pwndNextMenu = RevalidateHwnd(mnm.hwndNext);
                    if (pwndNextMenu == NULL)
                        goto TryAlternate;

                    /*
                     * If this window belongs to another thread, we cannot
                     *  use it. The menu loop won't get any messages
                     *  directed to that thread.
                     */
                    if (GETPTI(pwndNextMenu) != ptiCurrent) {
                        RIPMSG1(RIP_WARNING, "xxxMNKeyDown: Ignoring mnm.hwndNext bacause it belongs to another thread: %#p", pwndNextMenu);
                        goto TryAlternate;
                    }


                    pmenuNextMenu = RevalidateHmenu(mnm.hmenuNext);
                    if (pmenuNextMenu == NULL)
                        goto TryAlternate;

                    ThreadLock(pmenuNextMenu, &tlpmenuNextMenu);
                    ThreadLock(pwndNextMenu, &tlpwndNextMenu);

                    /*
                     * If the system menu is for a minimized MDI child,
                     * make sure the menu is dropped to give the user a
                     * visual clue that they are in menu mode
                     */
                    if (TestWF(pwndNextMenu, WFMINIMIZED))
                        fHierarchyWasDropped = TRUE;

                    xxxMNSelectItem(ppopupmenu, pMenuState, MFMWFP_NOITEM);

                    pMenuState->fIsSysMenu = TRUE;
                    UnlockPopupMenu(ppopupmenu, &ppopupmenu->spmenuAlternate);
                    ppopupmenu->fToggle = FALSE;
                    /*
                     * GetSystemMenu(pwnd, FALSE) and pwnd->spmenuSys are
                     * NOT equivalent -- GetSystemMenu returns the 1st submenu
                     * of pwnd->spmenuSys -- make up for that here
                     */
                    pmenuUse = (((pwndNextMenu->spmenuSys != NULL)
                                    && (_GetSubMenu(pwndNextMenu->spmenuSys, 0) == pmenuNextMenu))
                               ? pwndNextMenu->spmenuSys
                               : pmenuNextMenu);
                    /*
                     * We're going to change the notification window AND the menu.
                     * LockPopupMenu needs to unlock the current pmenu-spwndNotify
                     *  but also lock the new pmenu-spwndNotify. Since we cannot
                     *  give it the current AND the new pair, we unlock the
                     *  current one first, switch the notification window and
                     *  then call LockPopupMenu to lock the new pmenu-spwndNotify.
                     */
                    UserAssert(IsRootPopupMenu(ppopupmenu));
                    UnlockPopupMenu(ppopupmenu, &ppopupmenu->spmenu);
                    Lock(&ppopupmenu->spwndNotify, pwndNextMenu);
                    Lock(&ppopupmenu->spwndPopupMenu, pwndNextMenu);
                    LockPopupMenu(ppopupmenu, &ppopupmenu->spmenu, pmenuUse);
                    /*
                     * We just switched to a new notification window so
                     *  we need to Adjust capture accordingly
                     */
                    if (!pMenuState->fModelessMenu) {
                        ptiCurrent->pq->QF_flags &= ~QF_CAPTURELOCKED;
                        xxxMNSetCapture(ppopupmenu);
                    }


                    if (!TestWF(pwndNextMenu, WFCHILD) &&
                            ppopupmenu->spmenu != NULL) {

                        /*
                         * This window has a system menu and a main menu bar
                         * Set the alternate menu to the appropriate menu
                         */
                        if (pwndNextMenu->spmenu == ppopupmenu->spmenu) {
                            LockPopupMenu(ppopupmenu, &ppopupmenu->spmenuAlternate,
                                    pwndNextMenu->spmenuSys);
                            pMenuState->fIsSysMenu = FALSE;
                        } else {
                            LockPopupMenu(ppopupmenu, &ppopupmenu->spmenuAlternate,
                                    pwndNextMenu->spmenu);
                        }
                    }

                    ThreadUnlock(&tlpwndNextMenu);
                    ThreadUnlock(&tlpmenuNextMenu);

                    ppopupmenu->fIsSysMenu = pMenuState->fIsSysMenu;

                    item = 0;
                } else
TryAlternate:
                if (xxxMNSwitchToAlternateMenu(ppopupmenu)) {
                        /*
                         * go to first or last menu item int ppopup->hMenu
                         * based on 'key'
                         */
                    int dir = (key == VK_RIGHT) ? 1 : -1;

                    item = MNFindNextValidItem(ppopupmenu->spmenu, MFMWFP_NOITEM, dir, 0);
                }
            }

            if (item != MFMWFP_NOITEM) {
                /*
                 * we found a new menu item to go to
                 * 1) close up the previous menu if it was dropped
                 * 2) select the new menu item to go to
                 * 3) drop the new menu if the previous menu was dropped
                 */

                if (ppopupSave->spwndNextPopup)
                    xxxMNCloseHierarchy(ppopupSave, pMenuState);

                xxxMNSelectItem(ppopupmenu, pMenuState, item);

                if (fHierarchyWasDropped) {
DropHierarchy:
                    if (xxxMNOpenHierarchy(ppopupmenu, pMenuState) == (PWND)-1) {
                        return;
                    }
                }
            }
        }
        return;

    case VK_RETURN:
        {
        BOOL fEnabled;
        PITEM  pItem;

        if (ppopupmenu->posSelectedItem >= ppopupmenu->spmenu->cItems) {
            xxxMNDismiss(pMenuState);
            return;
        }

        pItem = ppopupmenu->spmenu->rgItems + ppopupmenu->posSelectedItem;
        fEnabled = !(pItem->fState & MFS_GRAYED);
        if ((pItem->spSubMenu != NULL) && fEnabled)
            goto DropHierarchy;

        /*
         * If no item is selected, throw away menu and return.
         */
        if (fEnabled) {
            xxxMNDismissWithNotify(pMenuState, ppopupmenu->spmenu, pItem, ppopupmenu->posSelectedItem, 0);
        } else {
            xxxMNDismiss(pMenuState);
        }
        return;
        }

    case VK_F1: /* Provide context sensitive help. */
        {
        PITEM  pItem;

        pItem = ppopupmenu->spmenu->rgItems + ppopupmenu->posSelectedItem;
        ThreadLock(ppopupmenu->spwndNotify, &tlpwndT);
        xxxSendHelpMessage(ppopupmenu->spwndNotify, HELPINFO_MENUITEM, pItem->wID,
                PtoHq(ppopupmenu->spmenu),
                GetMenuInheritedContextHelpId(ppopupmenu));
        ThreadUnlock(&tlpwndT);
        break;
        }

    }
}
/***************************************************************************\
* xxxMNPositionHierarchy
*
* Calculates the x.y postion to drop a hierarchy and returns the direction
*  to be used when animating (PAS_* value)
*
* 11/19/96  GerardoB  Extracted from xxxMNOpenHierarchy
\***************************************************************************/
UINT
xxxMNPositionHierarchy(
        PPOPUPMENU  ppopup,
        PITEM       pitem,
        int         cx,
        int         cy,
        int         *px,
        int         *py,
        PMONITOR    *ppMonitor)
{
    int         x, y;
    UINT        uPAS;
    PMONITOR    pMonitor;

    UserAssert(ppopup->fHierarchyDropped && (ppopup->spwndNextPopup != NULL));

    if (ppopup->fIsMenuBar) {
        /*
         * This is a menu being dropped from the top menu bar.  We need to
         * position it differently than hierarchicals which are dropped from
         * another popup.
         */

        BOOL fIconic = (TestWF(ppopup->spwndPopupMenu, WFMINIMIZED) != 0);
        RECT rcWindow;

        /*
         * Menu bar popups animate down.
         */
        uPAS = PAS_DOWN;

        CopyRect(&rcWindow, &ppopup->spwndPopupMenu->rcWindow);
        if (fIconic && IsTrayWindow(ppopup->spwndPopupMenu)) {
            xxxSendMinRectMessages(ppopup->spwndPopupMenu, &rcWindow);
        }

        /*
         * x position
         */
        if (!SYSMET(MENUDROPALIGNMENT) && !TestMF(ppopup->spmenu,MFRTL)) {
            if (fIconic) {
                x = rcWindow.left;
            } else {
                x = rcWindow.left + pitem->xItem;
            }
        } else {
            ppopup->fDroppedLeft = TRUE;
            if (fIconic) {
                x = rcWindow.right - cx;
            } else {
                x = rcWindow.left + pitem->xItem + pitem->cxItem - cx;
            }
        }

        /*
         * For a menu bar dropdown, pin to the monitor that owns the
         * majority of the menu item.  Otherwise, pin to the monitor that
         * owns the minimized window (the tray rect for min-to-tray dudes).
         */
        if (!fIconic)
        {
            /*
             * Use rcWindow as scratch for the menu bar item rect.  We want
             * to pin this menu on whatever monitor owns most of the menu
             * item clicked on.
             */
            rcWindow.left += pitem->xItem;
            rcWindow.top  += pitem->yItem;
            rcWindow.right = rcWindow.left + pitem->cxItem;
            rcWindow.bottom = rcWindow.top + pitem->cyItem;
        }

        pMonitor = _MonitorFromRect(&rcWindow, MONITOR_DEFAULTTOPRIMARY);

        /*
         * y position
         */
        if (!fIconic) {
            y = rcWindow.bottom;
        } else {
            /*
             * If the window is iconic, pop the menu up.  Since we're
             * minimized, the sysmenu button doesn't really exist.
             */
            y = rcWindow.top - cy;
            if (y < pMonitor->rcMonitor.top) {
                y = rcWindow.bottom;
            }
        }

        /*
         * Make sure the menu doesn't go off right side of monitor
         */
        x = min(x, pMonitor->rcMonitor.right - cx);

#ifdef USE_MIRRORING
        if (TestWF(ppopup->spwndPopupMenu, WEFLAYOUTRTL)) {
            x = ppopup->spwndPopupMenu->rcWindow.right - x + ppopup->spwndPopupMenu->rcWindow.left - cx;
        }
#endif

    } else { /* if (ppopup->fIsMenuBar) */

        /* Now position the hierarchical menu window.
         * We want to overlap by the amount of the frame, to help in the
         * 3D illusion.
         */

        /*
         * By default, hierachical popups animate to the right
         */
        uPAS = PAS_RIGHT;
        x = ppopup->spwndPopupMenu->rcWindow.left + pitem->xItem + pitem->cxItem;

        /* Note that we DO want the selections in the item and its popup to
         * align horizontally.
         */
        y = ppopup->spwndPopupMenu->rcWindow.top + pitem->yItem;
        if (ppopup->spmenu->dwArrowsOn != MSA_OFF) {
            y += gcyMenuScrollArrow - MNGetToppItem(ppopup->spmenu)->yItem;
        }

        /*
         * Try to make sure the menu doesn't go off right side of the
         * monitor.  If it does, drop it left, overlapping the checkmark
         * area.  Unless it would cover the previous menu...
         *
         * Use the monitor that the parent menu is on to keep all hierarchicals
         * in the same place.
         */
        pMonitor = _MonitorFromWindow(
                ppopup->spwndPopupMenu, MONITOR_DEFAULTTOPRIMARY);

#ifdef USE_MIRRORING
        if ((!!ppopup->fDroppedLeft) ^ (!!TestWF(ppopup->spwndPopupMenu, WEFLAYOUTRTL))) {
#else
        if (ppopup->fDroppedLeft) {
#endif
            int xTmp;

            /*
             * If this menu has dropped left, see if our hierarchy can be made
             * to drop to the left also.
             */
            xTmp = ppopup->spwndPopupMenu->rcWindow.left + SYSMET(CXFIXEDFRAME) - cx;
            if (xTmp >= pMonitor->rcMonitor.left) {
                x = xTmp;
                uPAS = PAS_LEFT;
            }
        }

        /*
         * Make sure the menu doesn't go off right side of screen.  Make it drop
         * left if it does.
         */
         if (x + cx > pMonitor->rcMonitor.right) {
             x = ppopup->spwndPopupMenu->rcWindow.left + SYSMET(CXFIXEDFRAME) - cx;
             uPAS = PAS_LEFT;
         }
#ifdef USE_MIRRORING
    if (TestWF(ppopup->spwndPopupMenu, WEFLAYOUTRTL)) {
        uPAS ^= PAS_HORZ;
    }
#endif

    } /* else if (ppopup->fIsMenuBar) */

    /*
     * Does the menu extend beyond bottom of monitor?
     */
    UserAssert(pMonitor);
    if (y + cy > pMonitor->rcMonitor.bottom) {
        y -= cy;

        /*
         * Try to pop above menu bar first
         */
        if (ppopup->fIsMenuBar) {
            y -= SYSMET(CYMENUSIZE);
            if (y >= pMonitor->rcMonitor.top) {
                uPAS = PAS_UP;
            }
        } else {
            /*
             * Account for nonclient frame above & below
             */
            y += pitem->cyItem + 2*SYSMET(CYFIXEDFRAME);
        }

        /*
         * Make sure that starting point is on a monitor, and all of menu shows.
         */
        if (    (y < pMonitor->rcMonitor.top) ||
                (y + cy > pMonitor->rcMonitor.bottom))
            /*
             * Pin it to the bottom.
             */
            y = pMonitor->rcMonitor.bottom - cy;
    }

    /*
     * Make sure Upper Left corner of menu is always visible.
     */
    x = max(x, pMonitor->rcMonitor.left);
    y = max(y, pMonitor->rcMonitor.top);

    /*
     * Propagate position
     */
    *px = x;
    *py = y;
    *ppMonitor = pMonitor;

    /*
     * Return animation direction
     */
    return uPAS;
}


/***************************************************************************\
* xxxCleanupDesktopMenu
*
* History:
* 10/19/98 GerardoB  Extracted from xxxMNCloseHierarchy
\***************************************************************************/
void xxxCleanupDesktopMenu (PWND pwndDeskMenu, PDESKTOP pdesk)
{
    TL tlpwnd;
    CheckLock(pwndDeskMenu);
    /*
     *  Put it on the message window tree so it is out of the way.
     */
    UserAssert(pwndDeskMenu->spwndParent == _GetDesktopWindow());
    ThreadLockAlways(pdesk->spwndMessage, &tlpwnd);
    xxxSetParent(pwndDeskMenu, pdesk->spwndMessage);
    ThreadUnlock(&tlpwnd);

    /*
     * Give ownershipe back to the desktop thread
     */
    pwndDeskMenu->head.pti = pdesk->pDeskInfo->spwnd->head.pti;
    Unlock(&pwndDeskMenu->spwndOwner);
}
/***************************************************************************\
* PWND MenuOpenHierarchyHandler(PPOPUPMENU ppopupmenu)
* effects: Drops one level of the hierarchy at the selection.
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/

PWND xxxMNOpenHierarchy(
    PPOPUPMENU ppopupmenu, PMENUSTATE pMenuState)
{
    PWND        ret = 0;
    PITEM       pItem;
    PWND        pwndHierarchy;
    PPOPUPMENU  ppopupmenuHierarchy;
    LONG        sizeHierarchy;
    int         xLeft;
    int         yTop;
    int         cxPopup, cyPopup;
    TL          tlpwndT;
    TL          tlpwndHierarchy;
    PTHREADINFO ptiCurrent = PtiCurrent();
    PDESKTOP    pdesk = ptiCurrent->rpdesk;
    BOOL        fSendUninit = FALSE;
    HMENU       hmenuInit;
    PMONITOR    pMonitor;


    if (ppopupmenu->posSelectedItem == MFMWFP_NOITEM) {
        /*
         *  No selection so fail.
         */
        return NULL;
    }

    if (ppopupmenu->posSelectedItem >= ppopupmenu->spmenu->cItems)
        return NULL;

    if (ppopupmenu->fHierarchyDropped) {
        if (ppopupmenu->fHideTimer) {
            xxxMNCloseHierarchy(ppopupmenu,pMenuState);
        } else {
        /*
         * Hierarchy already dropped. What are we doing here?
         */
            UserAssert(!ppopupmenu->fHierarchyDropped);
            return NULL;
        }
    }

    if (ppopupmenu->fShowTimer) {
        _KillTimer(ppopupmenu->spwndPopupMenu, IDSYS_MNSHOW);
        ppopupmenu->fShowTimer = FALSE;
    }

    /*
     * Get a pointer to the currently selected item in this menu.
     */
    pItem = &(ppopupmenu->spmenu->rgItems[ppopupmenu->posSelectedItem]);

    if (pItem->spSubMenu == NULL)
        goto Exit;

    /*
     * Send the initmenupopup message.
     */
    if (!ppopupmenu->fNoNotify) {
        ThreadLock(ppopupmenu->spwndNotify, &tlpwndT);
        /*
         * WordPerfect's Grammatik app doesn't know that TRUE means NON-ZERO,
         * not 1.  So we must use 0 & 1 explicitly for fIsSysMenu here
         * -- Win95B B#4947 -- 2/13/95 -- jeffbog
         */
        hmenuInit = PtoHq(pItem->spSubMenu);
        xxxSendMessage(ppopupmenu->spwndNotify, WM_INITMENUPOPUP,
            (WPARAM)hmenuInit, MAKELONG(ppopupmenu->posSelectedItem,
            (ppopupmenu->fIsSysMenu ? 1: 0)));
        ThreadUnlock(&tlpwndT);
        fSendUninit = TRUE;
    }


    /*
     * B#1517
     * Check if we're still in menu loop
     */
    if (!pMenuState->fInsideMenuLoop) {
        RIPMSG0(RIP_WARNING, "Menu loop ended unexpectedly by WM_INITMENUPOPUP");
        ret = (PWND)-1;
        goto Exit;
    }

    /*
     * The WM_INITMENUPOPUP message may have resulted in a change to the
     * menu.  Make sure the selection is still valid.
     */
    if (ppopupmenu->posSelectedItem >= ppopupmenu->spmenu->cItems) {
        /*
         * Selection is out of range, so fail.
         */
        goto Exit;
    }

    /*
     * Get a pointer to the currently selected item in this menu.
     * Bug #17867 - the call can cause this thing to change, so reload it.
     */
    pItem = &(ppopupmenu->spmenu->rgItems[ppopupmenu->posSelectedItem]);

    if (TestMFS(pItem, MFS_GRAYED) || (pItem->spSubMenu == NULL) || (pItem->spSubMenu->cItems == 0)) {
        /*
         * The item is disabled, no longer a popup, or empty so don't drop.
         */
        /*
         * No items in menu.
         */
        goto Exit;
    }

    /*
     * Let's make sure that the current thread is in menu mode and
     *  it uses this pMenuState. Otherwise the window we're about to
     *  create (or set the thread to) will point to a different pMenuState
     */
    UserAssert(ptiCurrent->pMenuState == pMenuState);

    if (ppopupmenu->fIsMenuBar && (pdesk->spwndMenu != NULL) &&
            (!(pdesk->dwDTFlags & DF_MENUINUSE)) &&
            !TestWF(pdesk->spwndMenu, WFVISIBLE)) {

        pdesk->dwDTFlags |= DF_MENUINUSE;

        if (HMPheFromObject(pdesk->spwndMenu)->bFlags & HANDLEF_DESTROY) {
            PPROCESSINFO ppi = pdesk->rpwinstaParent->pTerm->ptiDesktop->ppi;
            PPROCESSINFO ppiSave;
            PWND         pwndMenu;
            DWORD        dwDisableHooks;

            /*
             * the menu window is destroyed -- recreate it
             *
             * clear the desktopMenu flag so that the popup is
             * freed.
             */
            UserAssert(ppopupmenu->fDesktopMenu);

            ppopupmenu->fDesktopMenu = FALSE;
            ppopupmenu->fDelayedFree = FALSE;

            Unlock(&pdesk->spwndMenu);
            ppiSave  = ptiCurrent->ppi;
            ptiCurrent->ppi = ppi;

            /*
             * HACK HACK HACK!!! (adams) In order to create the menu window
             * with the correct desktop, we set the desktop of the current thread
             * to the new desktop. But in so doing we allow hooks on the current
             * thread to also hook this new desktop. This is bad, because we don't
             * want the menu window to be hooked while it is created. So we
             * temporarily disable hooks of the current thread or desktop,
             * and reenable them after switching back to the original desktop.
             */

            dwDisableHooks = ptiCurrent->TIF_flags & TIF_DISABLEHOOKS;
            ptiCurrent->TIF_flags |= TIF_DISABLEHOOKS;

            pwndMenu = xxxCreateWindowEx(
                WS_EX_TOOLWINDOW | WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE,
                (PLARGE_STRING)MENUCLASS,
                NULL,
                WS_POPUP | WS_BORDER,
                0,
                0,
                100,
                100,
                NULL,
                NULL,
                hModuleWin,
                NULL,
                WINVER);

            UserAssert(ptiCurrent->TIF_flags & TIF_DISABLEHOOKS);
            ptiCurrent->TIF_flags = (ptiCurrent->TIF_flags & ~TIF_DISABLEHOOKS) | dwDisableHooks;

            Lock(&(pdesk->spwndMenu), pwndMenu);

            UserAssert(((PMENUWND)pwndMenu)->ppopupmenu != NULL);

            /*
             * Set the desktopMenu flag to mark that this is the popup
             * allocated for pdesk->spwndMenu
             * Unlock spwndPopupMenu to avoid this special case in the common code path.
             */
            ((PMENUWND)pwndMenu)->ppopupmenu->fDesktopMenu = TRUE;
            Unlock(&((PMENUWND)pwndMenu)->ppopupmenu->spwndPopupMenu);

            ptiCurrent->ppi = ppiSave;

            HMChangeOwnerThread(pdesk->spwndMenu, pdesk->rpwinstaParent->pTerm->ptiDesktop);
        } else {
            TL tlpwndDesk;

            ThreadLockAlways(pdesk->spwndMenu, &tlpwndT);
            ThreadLockAlways(pdesk->pDeskInfo->spwnd, &tlpwndDesk);
            xxxSetParent(pdesk->spwndMenu, pdesk->pDeskInfo->spwnd);
            ThreadUnlock(&tlpwndDesk);
            ThreadUnlock(&tlpwndT);

        }


        pwndHierarchy = pdesk->spwndMenu;
        Lock(&pwndHierarchy->spwndOwner, ppopupmenu->spwndNotify);
        pwndHierarchy->head.pti = ptiCurrent;

        /*
         * Make the topmost state match the menu mode
         */
        if ((TestWF(pdesk->spwndMenu, WEFTOPMOST) && pMenuState->fModelessMenu)
                || (!TestWF(pdesk->spwndMenu, WEFTOPMOST) && !pMenuState->fModelessMenu)) {

            ThreadLock(pdesk->spwndMenu, &tlpwndHierarchy);
            xxxSetWindowPos(pdesk->spwndMenu,
              (pMenuState->fModelessMenu ? PWND_NOTOPMOST: PWND_TOPMOST),
              0,0,0,0,
              SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING);
            ThreadUnlock(&tlpwndHierarchy);
        }

        ppopupmenuHierarchy = ((PMENUWND)pwndHierarchy)->ppopupmenu;

        UserAssert(ppopupmenuHierarchy->fDesktopMenu);
        /*
         * clear any leftover data from the last time we used it
         * Assert that we're not zapping any locks here
         */
        UserAssert(ppopupmenuHierarchy->spwndPopupMenu == NULL);
        UserAssert(ppopupmenuHierarchy->spwndNextPopup == NULL);
        UserAssert(ppopupmenuHierarchy->spwndPrevPopup == NULL);
        UserAssert(ppopupmenuHierarchy->spmenu == NULL);
        UserAssert(ppopupmenuHierarchy->spmenuAlternate == NULL);
        UserAssert(ppopupmenuHierarchy->spwndNotify == NULL);
        UserAssert(ppopupmenuHierarchy->spwndActivePopup == NULL);

        RtlZeroMemory((PVOID)ppopupmenuHierarchy, sizeof(POPUPMENU));
        ppopupmenuHierarchy->fDesktopMenu = TRUE;

        ppopupmenuHierarchy->posSelectedItem = MFMWFP_NOITEM;
        Lock(&ppopupmenuHierarchy->spwndPopupMenu, pdesk->spwndMenu);

        Lock(&(ppopupmenuHierarchy->spwndNotify), ppopupmenu->spwndNotify);
        LockPopupMenu(ppopupmenuHierarchy, &ppopupmenuHierarchy->spmenu, pItem->spSubMenu);

    } else {

        ThreadLock(ppopupmenu->spwndNotify, &tlpwndT);

        pwndHierarchy = xxxCreateWindowEx(
                WS_EX_TOOLWINDOW | WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE,
                (PLARGE_STRING)MENUCLASS, NULL,
                WS_POPUP | WS_BORDER, 0, 0, 100, 100, ppopupmenu->spwndNotify,
                NULL, (HANDLE)ppopupmenu->spwndNotify->hModule,
                (LPVOID)pItem->spSubMenu, WINVER);

        ThreadUnlock(&tlpwndT);

        if (!pwndHierarchy)
            goto Exit;

        /*
         * Do this so old apps don't get weird borders on the popups of
         * hierarchical items!
         */
        ClrWF(pwndHierarchy, WFOLDUI);

        ppopupmenuHierarchy = ((PMENUWND)pwndHierarchy)->ppopupmenu;

    }

    /*
     * Mark this as fDelayedFree and link it
     */
    ppopupmenuHierarchy->fDelayedFree = TRUE;
    ppopupmenuHierarchy->ppmDelayedFree = ppopupmenu->ppopupmenuRoot->ppmDelayedFree;
    ppopupmenu->ppopupmenuRoot->ppmDelayedFree = ppopupmenuHierarchy;

#ifdef USE_MIRRORING
    if (TestWF(ppopupmenu->spwndPopupMenu, WEFLAYOUTRTL)) {
        SetWF(pwndHierarchy, WEFLAYOUTRTL);
    } else {
        ClrWF(pwndHierarchy, WEFLAYOUTRTL);
    }
#endif


    Lock(&(ppopupmenuHierarchy->spwndNotify), ppopupmenu->spwndNotify);
#if DBG
    /*
     * We should associate ppopupmenuHierarchy to the same menu we sent the
     *  WM_INITMsENUPOPUP message. Otherwise, the WM_UNINITMENUPOPUP
     *  will go to the wrong window. It would be the app's fault...
     */
    if (!ppopupmenu->fNoNotify && (hmenuInit != PtoHq(pItem->spSubMenu))) {
        RIPMSG2(RIP_WARNING, "xxxMNOpenHierarchy: bad app changed submenu from %#p to %#p",
                              hmenuInit, PtoHq(pItem->spSubMenu));
    }
#endif
    LockPopupMenu(ppopupmenuHierarchy, &ppopupmenuHierarchy->spmenu, pItem->spSubMenu);
    Lock(&(ppopupmenu->spwndNextPopup), pwndHierarchy);
    ppopupmenu->posDropped              = ppopupmenu->posSelectedItem;
    Lock(&(ppopupmenuHierarchy->spwndPrevPopup), ppopupmenu->spwndPopupMenu);
    ppopupmenuHierarchy->ppopupmenuRoot = ppopupmenu->ppopupmenuRoot;
    ppopupmenuHierarchy->fHasMenuBar = ppopupmenu->fHasMenuBar;
    ppopupmenuHierarchy->fIsSysMenu = ppopupmenu->fIsSysMenu;
    ppopupmenuHierarchy->fNoNotify      = ppopupmenu->fNoNotify;
    ppopupmenuHierarchy->fSendUninit = TRUE;
    ppopupmenuHierarchy->fRtoL = ppopupmenu->fRtoL;

    /*
     * The menu window has been created and intialized so if
     *  something fails, the WM_UNINITMENUPOPUP message will
     *  be sent from xxxMNDestroyHandler
     */
    fSendUninit = FALSE;

    /*
     * Set/clear the underline flag
     */
    if (pMenuState->fUnderline) {
        SetMF(ppopupmenuHierarchy->spmenu, MFUNDERLINE);
    } else {
        ClearMF(ppopupmenuHierarchy->spmenu, MFUNDERLINE);
    }

    ppopupmenuHierarchy->fAboutToHide   = FALSE;

    /*
     * Find the size of the menu window and actually size it (wParam = 1)
     */
    ThreadLock(pwndHierarchy, &tlpwndHierarchy);
    sizeHierarchy = (LONG)xxxSendMessage(pwndHierarchy, MN_SIZEWINDOW, MNSW_SIZE, 0);

    if (!sizeHierarchy) {
        /*
         * No size for this menu so zero it and blow off.
         */
        UserAssert(ppopupmenuHierarchy->fDelayedFree);
        if (ppopupmenuHierarchy->fDesktopMenu) {
            xxxMNDestroyHandler(ppopupmenuHierarchy);
            xxxCleanupDesktopMenu(pwndHierarchy, pdesk);
        }

        if (ThreadUnlock(&tlpwndHierarchy)) {
            if (!ppopupmenuHierarchy->fDesktopMenu) {
                xxxDestroyWindow(pwndHierarchy);
            }
        }

        Unlock(&ppopupmenu->spwndNextPopup);
        goto Exit;
    }

    cxPopup = LOWORD(sizeHierarchy) + 2*SYSMET(CXFIXEDFRAME);
    cyPopup = HIWORD(sizeHierarchy) + 2*SYSMET(CYFIXEDFRAME);

    ppopupmenu->fHierarchyDropped = TRUE;

    /*
     * Find out the x,y position to drop the hierarchy and the animation
     *  direction
     */
    ppopupmenuHierarchy->iDropDir = xxxMNPositionHierarchy(
            ppopupmenu, pItem, cxPopup, cyPopup, &xLeft, &yTop, &pMonitor);

    if (ppopupmenu->fIsMenuBar && _GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
        /*
         * If the menu had to be pinned to the bottom of the screen and
         * the mouse button is down, make sure the mouse isn't over the
         * menu rect.
         */
        RECT rc;
        RECT rcParent;
        int xrightdrop;
        int xleftdrop;

        /*
         * Get rect of hierarchical
         */
        CopyOffsetRect(
                &rc,
                &pwndHierarchy->rcWindow,
                xLeft - pwndHierarchy->rcWindow.left,
                yTop  - pwndHierarchy->rcWindow.top);

        /*
         * Get the rect of the menu bar popup item
         */
        rcParent.left = pItem->xItem + ppopupmenu->spwndPopupMenu->rcWindow.left;
        rcParent.top = pItem->yItem + ppopupmenu->spwndPopupMenu->rcWindow.top;
        rcParent.right = rcParent.left + pItem->cxItem;
        rcParent.bottom = rcParent.top + pItem->cyItem;

        if (IntersectRect(&rc, &rc, &rcParent)) {

            /*
             * Oh, oh...  The cursor will sit right on top of a menu item.
             * If the user up clicks, a menu will be accidently selected.
             *
             * Calc x position of hierarchical if we dropped it to the
             * right/left of the menu bar item.
             */
            xrightdrop = ppopupmenu->spwndPopupMenu->rcWindow.left +
                pItem->xItem + pItem->cxItem + cxPopup;

            if (xrightdrop > pMonitor->rcMonitor.right) {
                xrightdrop = 0;
            }

            xleftdrop = ppopupmenu->spwndPopupMenu->rcWindow.left +
                pItem->xItem - cxPopup;

            if (xleftdrop < pMonitor->rcMonitor.left) {
                xleftdrop = 0;
            }

            if (((SYSMET(MENUDROPALIGNMENT) || TestMFT(pItem, MFT_RIGHTORDER))
                  && xleftdrop) || !xrightdrop) {
                xLeft = ppopupmenu->spwndPopupMenu->rcWindow.left +
                    pItem->xItem - cxPopup;
                    ppopupmenuHierarchy->iDropDir = PAS_LEFT;
            } else if (xrightdrop) {
                xLeft = ppopupmenu->spwndPopupMenu->rcWindow.left +
                    pItem->xItem + pItem->cxItem;
                    ppopupmenuHierarchy->iDropDir = PAS_RIGHT;
            }
        }
    }

    /*
     * Take care of fDropNextPopup (menu bar) or fDroppedLeft (popups)
     * Set animation flag
     */
    if (ppopupmenu->fIsMenuBar) {
        /*
         * Only the first popup being dropped off the menu bar
         * is animated.
         */
        if (!ppopupmenu->fDropNextPopup) {
            ppopupmenuHierarchy->iDropDir |= PAS_OUT;
        }

        /*
         * Propagate right-to-left direction.
         */
        if (ppopupmenu->fDroppedLeft || (ppopupmenuHierarchy->iDropDir == PAS_LEFT)) {
            ppopupmenuHierarchy->fDroppedLeft = TRUE;
        }
        /*
         * Once a popup is dropped from the menu bar, moving to the next
         *  item on the menu bar should drop the popup.
         */
        ppopupmenu->fDropNextPopup = TRUE;
    } else {
        /*
         * Submenus always animate.
         */
        ppopupmenuHierarchy->iDropDir |= PAS_OUT;

        /*
         * Is this popup a lefty?
         */
        if (ppopupmenuHierarchy->iDropDir == PAS_LEFT) {
            ppopupmenuHierarchy->fDroppedLeft = TRUE;
        }
    }

    /*
     * The previous active dude must be visible
     */
    UserAssert((ppopupmenu->ppopupmenuRoot->spwndActivePopup == NULL)
            || TestWF(ppopupmenu->ppopupmenuRoot->spwndActivePopup, WFVISIBLE));

    /*
     * This is the new active popup
     */
    Lock(&(ppopupmenu->ppopupmenuRoot->spwndActivePopup), pwndHierarchy);

    /*
     * Paint the owner window before the popup menu comes up so that
     * the proper bits are saved.
     */
    if (ppopupmenuHierarchy->spwndNotify != NULL) {
        ThreadLockAlways(ppopupmenuHierarchy->spwndNotify, &tlpwndT);
        xxxUpdateWindow(ppopupmenuHierarchy->spwndNotify);
        ThreadUnlock(&tlpwndT);
    }

    /*
     * If this is a drag and drop menu, then we need to register the window
     *  as a drop target.
     */
    if (pMenuState->fDragAndDrop) {
        if (!NT_SUCCESS(xxxClientRegisterDragDrop(HW(pwndHierarchy)))) {
            RIPMSG1(RIP_ERROR, "xxxMNOpenHierarchy: xxxClientRegisterDragDrop failed:%#p", pwndHierarchy);
        }
    }

    /*
     * Show the window. Modeless menus are not topmost and get activated.
     *  Modal menus are topmost but don't get activated.
     */
    PlayEventSound(USER_SOUND_MENUPOPUP);

    xxxSetWindowPos(pwndHierarchy,
                    (pMenuState->fModelessMenu ? PWND_TOP : PWND_TOPMOST),
                    xLeft, yTop, 0, 0,
                    SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOOWNERZORDER
                    | (pMenuState->fModelessMenu ? 0 : SWP_NOACTIVATE));

    if (FWINABLE()) {
        xxxWindowEvent(EVENT_SYSTEM_MENUPOPUPSTART, pwndHierarchy, OBJID_CLIENT, INDEXID_CONTAINER, 0);
    }

    /*
     * Select the first item IFF we're in keyboard mode.  This fixes a
     * surprising number of compatibility problems with keyboard macros,
     * scripts, etc.
     */
    if (pMenuState->mnFocus == KEYBDHOLD) {
        xxxSendMessage(pwndHierarchy, MN_SELECTITEM, 0, 0L);
    }

    /*
     * This is needed so that popup menus are properly drawn on sys
     * modal dialog boxes.
     */
    xxxUpdateWindow(pwndHierarchy);

    ret = pwndHierarchy;
    ThreadUnlock(&tlpwndHierarchy);

Exit:
    /*
     * send matching WM_UNINITMENUPOPUP if needed (i.e, something
     *  failed).
     */
    if (fSendUninit
            && (ppopupmenu->spwndNotify != NULL)) {

        ThreadLockAlways(ppopupmenu->spwndNotify, &tlpwndT);
        xxxSendMessage(ppopupmenu->spwndNotify, WM_UNINITMENUPOPUP,
            (WPARAM)hmenuInit,
             MAKELONG(0, (ppopupmenu->fIsSysMenu ? MF_SYSMENU : 0)));
        ThreadUnlock(&tlpwndT);
    }

    return ret;
}

/***************************************************************************\
*
*  MNHideNextHierarchy()
*
*  Closes any submenu coming off of this popup.
*
\***************************************************************************/
BOOL xxxMNHideNextHierarchy(PPOPUPMENU ppopup)
{
    if (ppopup->spwndNextPopup != NULL) {
        TL tlpwndT;

        ThreadLockAlways(ppopup->spwndNextPopup, &tlpwndT);
        if (ppopup->spwndNextPopup != ppopup->spwndActivePopup)
            xxxSendMessage(ppopup->spwndNextPopup, MN_CLOSEHIERARCHY, 0, 0L);

        xxxSendMessage(ppopup->spwndNextPopup, MN_SELECTITEM, (WPARAM)-1, 0L);
        ThreadUnlock(&tlpwndT);
        return TRUE;
    }
    return FALSE;
}


/***************************************************************************\
* void MenuCloseHierarchyHandler(PPOPUPMENU ppopupmenu)
* effects: Close all hierarchies from this window down.
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/

void xxxMNCloseHierarchy(
    PPOPUPMENU ppopupmenu, PMENUSTATE pMenuState)
{
    TL           tlpwndNext;
    TL           tlpwnd;
    TL           tlpopup;
    PTHREADINFO  ptiCurrent = PtiCurrent();
    PDESKTOP     pdesk;
    PWND         pwndNext;

    Validateppopupmenu(ppopupmenu);

    /*
     * Terminate any animation
     */
    MNAnimate(pMenuState, FALSE);

    /*
     * If a hierarchy exists, close all childen below us.  Do it in reversed
     * order so savebits work out.
     */
    if  (!ppopupmenu->fHierarchyDropped) {
        /*
         * Assert that there's no next or it might not get closed
         */
        UserAssert(ppopupmenu->spwndNextPopup == NULL);
        return;
    }

    if (ppopupmenu->fHideTimer)
    {
        _KillTimer(ppopupmenu->spwndPopupMenu, IDSYS_MNHIDE);
        ppopupmenu->fHideTimer = FALSE;
    }

    pwndNext = ppopupmenu->spwndNextPopup;
    if (pwndNext != NULL) {

        ThreadLockAlways(pwndNext, &tlpwndNext);
        xxxSendMessage(pwndNext, MN_CLOSEHIERARCHY, 0, 0);

        /*
         * If modeless menu, activate the this popup since we're about
         *  to destroy the current active one. We want to keep activation
         *  on a menu window so we can get the keys. Also, modeless menus
         *  are canceled when a non-menu window is activated in their queue
         */
        if (pMenuState->fModelessMenu
                && pMenuState->fInsideMenuLoop
                && !ppopupmenu->fIsMenuBar) {

            ThreadLockAlways(ppopupmenu->spwndPopupMenu, &tlpwnd);
            xxxActivateThisWindow(ppopupmenu->spwndPopupMenu, 0, 0);
            ThreadUnlock(&tlpwnd);
        }

        if (FWINABLE()) {
            xxxWindowEvent(EVENT_SYSTEM_MENUPOPUPEND, pwndNext, OBJID_CLIENT, INDEXID_CONTAINER, 0);
        }

        /*
         * If the current thread is not in the right pdesk, then that could
         *  be the cause of the stuck menu bug.
         * In other words, are we nuking this menu out of context?
         */
        UserAssert(ptiCurrent->pMenuState != NULL);
        pdesk = ptiCurrent->rpdesk;

        if (pwndNext == pdesk->spwndMenu) {
            PPOPUPMENU ppopupmenuReal;

            UserAssert(pdesk->dwDTFlags & DF_MENUINUSE);

            /*
             * If this is our precreated real popup window,
             * initialize ourselves and just hide.
             */
            xxxShowWindow(pwndNext, SW_HIDE | TEST_PUDF(PUDF_ANIMATE));

            /*
             * Its possible that during Logoff the above xxxShowWindow
             * won't get prossessed and because this window is a special
             * window that is owned by they desktop we have to manually mark
             * it as invisible.
             */
            if (TestWF(pwndNext, WFVISIBLE)) {
                SetVisible(pwndNext, SV_UNSET);
            }

#ifdef HAVE_MN_GETPPOPUPMENU
            ppopupmenuReal = (PPOPUPMENU)xxxSendMessage(pwndNext,
                    MN_GETPPOPUPMENU,0, 0L);
#else
            ppopupmenuReal = ((PMENUWND)pwndNext)->ppopupmenu;
#endif
            UserAssert(ppopupmenuReal->fDesktopMenu == TRUE);

            /*
             * We don't want this window to be a drop target anymore.
             * Non cached menu windows revoke it on WM_FINALDESTROY.
             */
            if (pMenuState->fDragAndDrop) {
                if (!NT_SUCCESS(xxxClientRevokeDragDrop(HW(pwndNext)))) {
                    RIPMSG1(RIP_ERROR, "xxxMNCloseHierarchy: xxxClientRevokeRegisterDragDrop failed:%#p", pwndNext);
                }
            }

            if (ppopupmenuReal != NULL) {
                xxxMNDestroyHandler(ppopupmenuReal);
                /*
                 * We used to clear the popup contents here but the popup might be
                 *  still in use if this is happening during a callback. So we let
                 *  MNFreePopup do that. If it didn't happen during the call above,
                 *  it'll happen when MNFlushDestroyedPopups is executed.
                 */
            }

            xxxCleanupDesktopMenu(pwndNext, pdesk);

            ThreadUnlock(&tlpwndNext);
        } else  if (ThreadUnlock(&tlpwndNext)) {
            /*
             * We know this is not the current thread's desktop menu window.
             * Let's assert that it's not the menu window of another desktop.
             */
            UserAssert(pwndNext != pwndNext->head.rpdesk->spwndMenu);
            xxxDestroyWindow(pwndNext);
        }

        Unlock(&ppopupmenu->spwndNextPopup);
        ppopupmenu->fHierarchyDropped = FALSE;

    }

    if (ppopupmenu->fIsMenuBar) {
        Unlock(&ppopupmenu->spwndActivePopup);
    } else {
        Lock(&(ppopupmenu->ppopupmenuRoot->spwndActivePopup),
                ppopupmenu->spwndPopupMenu);
    }

    if (pMenuState->fInsideMenuLoop &&
            (ppopupmenu->posSelectedItem != MFMWFP_NOITEM)) {
        /*
         * Send a menu select as if this item had just been selected.  This
         * allows people to easily update their menu status bars when a
         * hierarchy from this item has been closed.
         */
        PWND pwnd = ppopupmenu->ppopupmenuRoot->spwndNotify;
        if (pwnd) {
            ThreadLockAlways(pwnd, &tlpwnd);
            ThreadLockAlways(ppopupmenu->spwndPopupMenu, &tlpopup);
            xxxSendMenuSelect(pwnd, ppopupmenu->spwndPopupMenu,
                    ppopupmenu->spmenu, ppopupmenu->posSelectedItem);
            ThreadUnlock(&tlpopup);
            ThreadUnlock(&tlpwnd);
        }
    }

}

/***************************************************************************\
*
*  MNDoubleClick()
*
*  If an item isn't a hierarchical, then the double-click works just like
*  single click did.  Otherwise, we traverse the submenu hierarchy to find
*  a valid default element.  If we reach a submenu that has no valid default
*  subitems and it itself has a valid ID, that becomes the valid default
*  element.
*
*  Note:   This function does not remove the double click message
*          from the message queue, so the caller must do so.
*
*  BOGUS
*  How about opening the hierarchies if we don't find anything?
*
*  Returns TRUE if handled.
*
\***************************************************************************/
BOOL xxxMNDoubleClick(PMENUSTATE pMenuState, PPOPUPMENU ppopup, int idxItem)
{
    PMENU  pMenu;
    PITEM  pItem;
    MSG   msg;
    UINT uPos;

    /*
     * This code to swallow double clicks isn't executed!  MNLoop will
     * swallow all double clicks for us.  Swallow the up button for the
     * double dude instead.  Word will not be happy if they get a spurious
     * WM_LBUTTONUP on the menu bar if their code to close the MDI child
     * doesn't swallow it soon enough.
     */

    /*
     * Eat the click.
     */
    if (xxxPeekMessage(&msg, NULL, 0, 0, PM_NOYIELD)) {
        if ((msg.message == WM_LBUTTONUP) ||
            (msg.message == WM_NCLBUTTONUP)) {
           xxxPeekMessage(&msg, NULL, msg.message, msg.message, PM_REMOVE);
        }
#if DBG
        else if (msg.message == WM_LBUTTONDBLCLK ||
            msg.message == WM_NCLBUTTONDBLCLK)
        {
            UserAssertMsg0(FALSE, "xxxMNDoubleClick found a double click");
        }
#endif
    }

    /*
     * Get current item.
     */
    pMenu = ppopup->spmenu;
    if ((pMenu==NULL) || ((UINT)idxItem >= pMenu->cItems)) {
        xxxMNDoScroll(ppopup, ppopup->posSelectedItem, FALSE);
        goto Done;
    }

    pItem = pMenu->rgItems + idxItem;
    uPos = idxItem;

    /*
     * Do nothing if item is disabled.
     */
    if (pItem->fState & MFS_GRAYED)
        goto Done;

    /*
     * Traverse the hierarchy down as far as possible.
     */
    do
    {
        if (pItem->spSubMenu != NULL) {
            /*
             * The item is a popup menu, so continue traversing.
             */
            pMenu = pItem->spSubMenu;
            idxItem = (UINT)_GetMenuDefaultItem(pMenu, MF_BYPOSITION, 0);

            if (idxItem != -1) {
                pItem = pMenu->rgItems + idxItem;
                uPos = idxItem;
                continue;
            } else /* if (lpItem->wID == -1) How do we know this popup has an ID? */
                break;
        }

        /*
         * We've found a leaf node of some kind, either a MFS_DEFAULT popup
         * with a valid cmd ID that has no valid MFS_DEFAULT children, or
         * a real cmd with MFS_DEFAULT style.
         *
         * Exit menu mode and send command ID.
         */

        /*
         * For old apps we need to generate a WM_MENUSELECT message first.
         * Old apps, esp. Word 6.0, can't handle double-clicks on maximized
         * child sys menus because they never get a WM_MENUSELECT for the
         * item, unlike with normal keyboard/mouse choosing.  We need to
         * fake it so they won't fault.  Several VB apps have a similar
         * problem.
         */
        if (!TestWF(ppopup->ppopupmenuRoot->spwndNotify, WFWIN40COMPAT))
        {
            TL tlpwndNotify, tlpopup;

            ThreadLock(ppopup->ppopupmenuRoot->spwndNotify, &tlpwndNotify);
            ThreadLock(ppopup->spwndPopupMenu, &tlpopup);
            xxxSendMenuSelect(ppopup->ppopupmenuRoot->spwndNotify,
                    ppopup->spwndPopupMenu, pMenu, idxItem);
            ThreadUnlock(&tlpopup);
            ThreadUnlock(&tlpwndNotify);
        }

        xxxMNDismissWithNotify(pMenuState, pMenu, pItem, uPos, 0);
        return TRUE;
    }
    while (TRUE);

Done:
    return(FALSE);
}


/***************************************************************************\
* UINT MenuSelectItemHandler(PPOPUPMENU ppopupmenu, int itemPos)
*
* Unselects the old selection, selects the item at itemPos and highlights it.
*
* MFMWFP_NOITEM if no item is to be selected.
*
* Returns the item flags of the item being selected.
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/

PITEM xxxMNSelectItem(
    PPOPUPMENU ppopupmenu,
    PMENUSTATE pMenuState,
    UINT itemPos)
{
    PITEM pItem = NULL;
    TL tlpwndNotify;
    TL tlpwndPopup;
    TL tlpmenu;
    PWND pwndNotify;
    PMENU pmenu;

    if (ppopupmenu->posSelectedItem == itemPos) {

        /*
         * If this item is already selectected, just return its flags.
         */
        if ((itemPos != MFMWFP_NOITEM) && (itemPos < ppopupmenu->spmenu->cItems)) {
            return &(ppopupmenu->spmenu->rgItems[itemPos]);
        }
        return NULL;
    }

    /*
     * Terminate any animation
     */
    MNAnimate(pMenuState, FALSE);

    if (ppopupmenu->fShowTimer) {
        _KillTimer(ppopupmenu->spwndPopupMenu, IDSYS_MNSHOW);
        ppopupmenu->fShowTimer = FALSE;
    }

    ThreadLock(pmenu = ppopupmenu->spmenu, &tlpmenu);
    ThreadLock(pwndNotify = ppopupmenu->spwndNotify, &tlpwndNotify);

    if (ppopupmenu->fAboutToHide)
    {
        PPOPUPMENU ppopupPrev = ((PMENUWND)(ppopupmenu->spwndPrevPopup))->ppopupmenu;

        _KillTimer(ppopupPrev->spwndPopupMenu, IDSYS_MNHIDE);
        ppopupPrev->fHideTimer = FALSE;
        if (ppopupPrev->fShowTimer)
        {
            _KillTimer(ppopupPrev->spwndPopupMenu, IDSYS_MNSHOW);
            ppopupPrev->fShowTimer = FALSE;
        }

        if (ppopupPrev->posSelectedItem != ppopupPrev->posDropped)
        {
            TL tlpmenuPopupMenuPrev;
            ThreadLock(ppopupPrev->spmenu, &tlpmenuPopupMenuPrev);
            if (ppopupPrev->posSelectedItem != MFMWFP_NOITEM) {
                xxxMNInvertItem(ppopupPrev, ppopupPrev->spmenu,
                        ppopupPrev->posSelectedItem, ppopupPrev->spwndNotify, FALSE);
            }

            ppopupPrev->posSelectedItem = ppopupPrev->posDropped;

            xxxMNInvertItem(ppopupPrev, ppopupPrev->spmenu,
                        ppopupPrev->posDropped, ppopupPrev->spwndNotify, TRUE);
            ThreadUnlock(&tlpmenuPopupMenuPrev);
        }

        ppopupmenu->fAboutToHide = FALSE;
        Lock(&ppopupmenu->ppopupmenuRoot->spwndActivePopup, ppopupmenu->spwndPopupMenu);
    }

    if (MNIsItemSelected(ppopupmenu)) {
        /*
         * Something else is selected so we need to unselect it.
         */
        if (ppopupmenu->spwndNextPopup) {
            if (ppopupmenu->fIsMenuBar) {
                xxxMNCloseHierarchy(ppopupmenu, pMenuState);
            } else {
                MNSetTimerToCloseHierarchy(ppopupmenu);
            }
        }

        goto DeselectItem;
    } else if (MNIsScrollArrowSelected(ppopupmenu)) {
            _KillTimer(ppopupmenu->spwndPopupMenu, ppopupmenu->posSelectedItem);
DeselectItem:

            xxxMNInvertItem(ppopupmenu, pmenu,
                    ppopupmenu->posSelectedItem, pwndNotify, FALSE);
    }

    ppopupmenu->posSelectedItem = itemPos;

    if (itemPos != MFMWFP_NOITEM) {
        /*
         * If an item is selected, no autodismiss plus this means
         *  that the mouse is on the menu
         */
        pMenuState->fAboutToAutoDismiss =
        pMenuState->fMouseOffMenu = FALSE;

        if (pMenuState->fButtonDown) {
            xxxMNDoScroll(ppopupmenu, itemPos, TRUE);
        }

        pItem = xxxMNInvertItem(ppopupmenu, pmenu,
                itemPos, pwndNotify, TRUE);
        ThreadUnlock(&tlpwndNotify);
        ThreadUnlock(&tlpmenu);
        return pItem;

    } else if (FWINABLE()) {
        /*
         * Notify that nothing is now focused in this menu.
         */
        xxxWindowEvent(EVENT_OBJECT_FOCUS, ppopupmenu->spwndPopupMenu,
               ((ppopupmenu->spwndNotify != ppopupmenu->spwndPopupMenu) ? OBJID_CLIENT :
               (ppopupmenu->fIsSysMenu ? OBJID_SYSMENU : OBJID_MENU)), 0, 0);
    }

    ThreadUnlock(&tlpwndNotify);
    ThreadUnlock(&tlpmenu);

    if (ppopupmenu->spwndPrevPopup != NULL) {
        PPOPUPMENU pp;

        /*
         * Get the popupMenu data for the previous menu
         * Use the root popupMenu if the previous menu is the menu bar
         */
        if (ppopupmenu->fHasMenuBar && (ppopupmenu->spwndPrevPopup ==
                ppopupmenu->spwndNotify)) {
            pp = ppopupmenu->ppopupmenuRoot;
        } else {
#ifdef HAVE_MN_GETPPOPUPMENU
            TL tlpwndPrevPopup;
            ThreadLock(ppopupmenu->spwndPrevPopup, &tlpwndPrevPopup);
            pp = (PPOPUPMENU)xxxSendMessage(ppopupmenu->spwndPrevPopup,
                    MN_GETPPOPUPMENU, 0, 0L);
            ThreadUnlock(&tlpwndPrevPopup);
#else
            pp = ((PMENUWND)ppopupmenu->spwndPrevPopup)->ppopupmenu;
#endif
        }

        /*
         * Generate a WM_MENUSELECT for the previous menu to re-establish
         * it's current item as the SELECTED item
         */
        ThreadLock(pp->spwndNotify, &tlpwndNotify);
        ThreadLock(pp->spwndPopupMenu, &tlpwndPopup);
        xxxSendMenuSelect(pp->spwndNotify, pp->spwndPopupMenu, pp->spmenu, pp->posSelectedItem);
        ThreadUnlock(&tlpwndPopup);
        ThreadUnlock(&tlpwndNotify);
    }

    return NULL;
}

/***************************************************************************\
*
*  MNItemHitTest()
*
*  Given a hMenu and a point in screen coordinates, returns the position
*  of the item the point is in.  Returns -1 if no item exists there.
*
\***************************************************************************/
UINT MNItemHitTest(PMENU pMenu, PWND pwnd, POINT pt)
{
    PITEM  pItem;
    UINT    iItem;
    RECT    rect;

    PTHREADINFO ptiCurrent = PtiCurrent();

    if (pMenu->cItems == 0)
        return MFMWFP_NOITEM;


    /*
     * This point is screen-relative.  Menu bar coordinates relative
     * to the window.  But popup menu coordinates are relative to the client.
     */
    if (TestMF(pMenu, MFISPOPUP)) {

        /*
         * Bail if it's outside rcWindow
         */
        CopyInflateRect(&rect, &(pwnd->rcWindow),
                -SYSMET(CXFIXEDFRAME), -SYSMET(CYFIXEDFRAME));

        if (!PtInRect(&rect, pt)) {
            return MFMWFP_NOITEM;
        }

        /* ScreenToClient */
#ifdef USE_MIRRORING
        if (TestWF(pwnd, WEFLAYOUTRTL)) {
            pt.x = pwnd->rcClient.right - pt.x;
        } else
#endif
        {
            pt.x -= pwnd->rcClient.left;
        }
        pt.y -= pwnd->rcClient.top;

        /*
         * If on the non client area, then it's on the scroll arrows
         */
        if (pt.y < 0) {
            return MFMWFP_UPARROW;
        } else if (pt.y > (int)pMenu->cyMenu) {
            return MFMWFP_DOWNARROW;
        }

    } else {
        /* ScreenToWindow */
#ifdef USE_MIRRORING
        if (TestWF(pwnd, WEFLAYOUTRTL) &&
            (
             (ptiCurrent->pq->codeCapture == SCREEN_CAPTURE) || (ptiCurrent->pq->codeCapture == NO_CAP_SYS)
            )
           ) {
            pt.x = pwnd->rcWindow.right - pt.x;
        } else
#endif
        {
            pt.x -= pwnd->rcWindow.left;
        }
        pt.y -= pwnd->rcWindow.top;
    }

    /*
     * Step through all the items in the menu.
     * If scrollable menu
     */
    if (pMenu->dwArrowsOn != MSA_OFF) {
        UserAssert(TestMF(pMenu, MFISPOPUP));
        pItem = MNGetToppItem(pMenu);
        rect.left = rect.top = 0;
        rect.right = pItem->cxItem;
        rect.bottom = pItem->cyItem;
        for (iItem = pMenu->iTop; (iItem < (int)pMenu->cItems) && (rect.top < (int)pMenu->cyMenu); iItem++) {

            if (PtInRect(&rect, pt)) {
                return iItem;
            }

            pItem++;
            rect.top = rect.bottom;
            rect.bottom += pItem->cyItem;
        }
    } else {
        /*
         * No scroll bars.
         */
        for (iItem = 0, pItem = pMenu->rgItems; iItem < pMenu->cItems; iItem++, pItem++) {
            /* Is the mouse inside this item's rectangle? */
            rect.left       = pItem->xItem;
            rect.top        = pItem->yItem;
            rect.right      = pItem->xItem + pItem->cxItem;
            rect.bottom     = pItem->yItem + pItem->cyItem;

            if (PtInRect(&rect, pt)) {
                return(iItem);
            }
        }
    }


    return(MFMWFP_NOITEM);
}
/***************************************************************************\
* LockMFMWFPWindow
*
* This function is called when we need to save the return value of
*  xxxMNFindWindowFromPoint.
*
* History:
* 11/14/96  GerardoB  Created
\***************************************************************************/
void LockMFMWFPWindow (PULONG_PTR puHitArea, ULONG_PTR uNewHitArea)
{
    /*
     * Bail if there is nothing to do.
     */
    if (*puHitArea == uNewHitArea) {
        return;
    }

    /*
     * Unlock current hit area
     */
    UnlockMFMWFPWindow(puHitArea);

    /*
     * Lock new hit area
     */
    if (IsMFMWFPWindow(uNewHitArea)) {
        Lock(puHitArea, (PWND)uNewHitArea);
    } else {
        *puHitArea = uNewHitArea;
    }
}
/***************************************************************************\
* UnlockMFMWFPWindow
*
* You must called this if you ever called LockMFMWFPWindow
*
* History:
* 11/14/96  GerardoB  Created
\***************************************************************************/
void UnlockMFMWFPWindow (PULONG_PTR puHitArea)
{
    if (IsMFMWFPWindow(*puHitArea)) {
        Unlock(puHitArea);
    } else {
        *puHitArea = MFMWFP_OFFMENU;
    }
}
/***************************************************************************\
* IsMFMWFPWindow
*
* Test whether or not the return value of xxxMNFindWindowFromPoint is
*  a window. Not that uHitArea could be an HWND or a PWND.
*
* History:
*   10-02-96 GerardoB   Created
\***************************************************************************/
BOOL IsMFMWFPWindow (ULONG_PTR uHitArea)
{
    switch(uHitArea) {
        case MFMWFP_OFFMENU:
        case MFMWFP_NOITEM:
        case MFMWFP_ALTMENU:
            return FALSE;

        default:
            return TRUE;
    }
}
/***************************************************************************\
* LONG MenuFindMenuWindowFromPoint(
*         PPOPUPMENU ppopupmenu, PUINT pIndex, POINT screenPt)
*
* effects: Determines in which window the point lies.
*
* Returns
*   - PWND of the hierarchical menu the point is on,
*   - MFMWFP_ALTMENU if point lies on the alternate popup menu.
*   - MFMWFP_NOITEM if there is no item at that point on the menu or the
*      point lies on the menu bar.
*   - MFMWFP_OFFMENU if point lies elsewhere.
*
* Returns in pIndex
*   - the index of the item hit,
*   - MFMWFP_NOITEM if there is no item at that point on the menu or
*      point lies on the menu bar.
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
*   8-11-92 Sanfords added MFMWFP_ constants
\***************************************************************************/

LONG_PTR xxxMNFindWindowFromPoint(
    PPOPUPMENU ppopupmenu,
    PUINT pIndex,
    POINTS screenPt)
{
    POINT pt;
    RECT rect;
    LONG_PTR longHit;
    UINT itemHit;
    PWND pwnd;
    TL tlpwndT;
#ifdef USE_MIRRORING
    int cx;
#endif


    *pIndex = 0;

    if (ppopupmenu->spwndNextPopup) {

        /*
         * Check if this point is on any of our children before checking if it
         * is on ourselves.
         */
        ThreadLockAlways(ppopupmenu->spwndNextPopup, &tlpwndT);
        longHit = xxxSendMessage(ppopupmenu->spwndNextPopup,
                MN_FINDMENUWINDOWFROMPOINT, (WPARAM)&itemHit,
                MAKELONG(screenPt.x, screenPt.y));
        ThreadUnlock(&tlpwndT);

        /*
         * If return value is an hwnd, convert to pwnd.
         */
        if (IsMFMWFPWindow(longHit)) {
            longHit = (LONG_PTR)RevalidateHwnd((HWND)longHit);
        }

        if (longHit) {

            /*
             * Hit occurred on one of our children.
             */

            *pIndex = itemHit;
            return longHit;
        }
    }

    if (ppopupmenu->fIsMenuBar) {
        int cBorders;

         /*
          * Check if this point is on the menu bar
          */
        pwnd = ppopupmenu->spwndNotify;
        if (pwnd == NULL) {
            return MFMWFP_OFFMENU;
        }

        pt.x = screenPt.x;
        pt.y = screenPt.y;

        if (ppopupmenu->fIsSysMenu) {

            if (!_HasCaptionIcon(pwnd)) {
                /*
                 * no system menu rect to click in if it doesn't have an icon
                 */
                return(0L);
            }

            /*
             * Check if this is a click on the system menu icon.
             */
            if (TestWF(pwnd, WFMINIMIZED)) {

                /*
                 * If the window is minimized, then check if there was a hit in
                 * the client area of the icon's window.
                 */

/*
 * Mikehar 5/27
 * Don't know how this ever worked. If we are the system menu of an icon
 * we want to punt the menus if the click occurs ANYWHERE outside of the
 * menu.
 * Johnc 03-Jun-1992 the next 4 lines were commented out for Mike's
 * problem above but that made clicking on a minimized window with
 * the system menu already up, bring down the menu and put it right
 * up again (bug 10951) because the mnloop wouldn't swallow the mouse
 * down click message.  The problem Mike mentions no longer shows up.
 */

                if (PtInRect(&(pwnd->rcWindow), pt)) {
                    return MFMWFP_NOITEM;
                }

                /*
                 * It's an iconic window, so can't be hitting anywhere else.
                 */
                return MFMWFP_OFFMENU;
            }

            /*
             * Check if we are hitting on the system menu rectangle on the top
             * left of windows.
             */
            rect.top = rect.left = 0;
            rect.right  = SYSMET(CXSIZE);
            rect.bottom = SYSMET(CYSIZE);

            cBorders = GetWindowBorders(pwnd->style, pwnd->ExStyle, TRUE, FALSE);

            OffsetRect(&rect, pwnd->rcWindow.left + cBorders*SYSMET(CXBORDER),
                pwnd->rcWindow.top + cBorders*SYSMET(CYBORDER));
#ifdef USE_MIRRORING
            //Mirror the rect because the buttons in the left hand side of the window if it mirrored
            if (TestWF(pwnd, WEFLAYOUTRTL)) {
                cx         = rect.right - rect.left;
                rect.right = pwnd->rcWindow.right - (rect.left - pwnd->rcWindow.left);
                rect.left  = rect.right - cx;
            }
#endif
            if (PtInRect(&rect, pt)) {
                *pIndex = 0;
                return(MFMWFP_NOITEM);
            }
            /*
             * Check if we hit in the alternate menu if available.
             */
            if (ppopupmenu->spmenuAlternate) {
                itemHit = MNItemHitTest(ppopupmenu->spmenuAlternate, pwnd, pt);
                if (itemHit != MFMWFP_NOITEM) {
                    *pIndex = itemHit;
                    return MFMWFP_ALTMENU;
                }
            }
            return MFMWFP_OFFMENU;
        } else {
            if (TestWF(ppopupmenu->spwndNotify, WFMINIMIZED)) {

                /*
                 * If we are minimized, we can't hit on the main menu bar.
                 */
                return MFMWFP_OFFMENU;
            }
        }
    } else {
        pwnd = ppopupmenu->spwndPopupMenu;

        /*
         * else this is a popup window and we need to check if we are hitting
         * anywhere on this popup window.
         */
        pt.x = screenPt.x;
        pt.y = screenPt.y;
        if (!PtInRect(&pwnd->rcWindow, pt)) {

            /*
             * Point completely outside the popup menu window so return 0.
             */
            return MFMWFP_OFFMENU;
        }
    }

    pt.x = screenPt.x;
    pt.y = screenPt.y;

    itemHit = MNItemHitTest(ppopupmenu->spmenu, pwnd, pt);

    if (ppopupmenu->fIsMenuBar) {

        /*
         * If hit is on menu bar but no item is there, treat it as if the user
         * hit nothing.
         */
        if (itemHit == MFMWFP_NOITEM) {

            /*
             * Check if we hit in the alternate menu if available.
             */
            if (ppopupmenu->spmenuAlternate) {
                itemHit = MNItemHitTest(ppopupmenu->spmenuAlternate, pwnd, pt);
                if (itemHit != MFMWFP_NOITEM) {
                    *pIndex = itemHit;
                    return MFMWFP_ALTMENU;
                }
            }
            return MFMWFP_OFFMENU;
        }

        *pIndex = itemHit;
        return MFMWFP_NOITEM;
    } else {

        /*
         * If hit is on popup menu but no item is there, itemHit
         * will be MFMWFP_NOITEM
         */
        *pIndex = itemHit;
        return (LONG_PTR)pwnd;
    }
    return MFMWFP_OFFMENU;
}

/***************************************************************************\
*void MenuCancelMenus(PPOPUPMENU ppopupmenu,
*                                UINT cmd, BOOL fSend)
* Should only be sent to the top most ppopupmenu/menu window in the
* hierarchy.
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/

void xxxMNCancel(
    PMENUSTATE pMenuState,
    UINT uMsg,
    UINT cmd,
    LPARAM lParam)
{
    PPOPUPMENU ppopupmenu = pMenuState->pGlobalPopupMenu;
    BOOL fSynchronous   = ppopupmenu->fSynchronous;
    BOOL fTrackFlagsSet = ppopupmenu->fIsTrackPopup;
    BOOL fIsSysMenu     = ppopupmenu->fIsSysMenu;
    BOOL fIsMenuBar     = ppopupmenu->fIsMenuBar;
    BOOL fNotify        = !ppopupmenu->fNoNotify;
    PWND pwndT;
    TL tlpwndT;
    TL tlpwndPopupMenu;

    Validateppopupmenu(ppopupmenu);

    pMenuState->fInsideMenuLoop = FALSE;
    pMenuState->fButtonDown = FALSE;
    /*
     * Mark the popup as destroyed so people will not use it anymore.
     * This means that root popups can be marked as destroyed before
     * actually being destroyed (nice and confusing).
     */
    ppopupmenu->fDestroyed = TRUE;

    /*
     * Only the menu loop owner can destroy the menu windows (i.e, xxxMNCloseHierarchy)
     */
    if (PtiCurrent() != pMenuState->ptiMenuStateOwner) {
        RIPMSG1(RIP_WARNING, "xxxMNCancel: Thread %#p doesn't own the menu loop", PtiCurrent());
        return;
    }

    /*
     * If the menu loop is running on a thread different than the thread
     *  that owns spwndNotify, we can have two threads trying to cancel
     *  this popup at the same time.
     */
    if (ppopupmenu->fInCancel) {
        RIPMSG1(RIP_WARNING, "xxxMNCancel: already in cancel. ppopupmenu:%#p", ppopupmenu);
        return;
    }
    ppopupmenu->fInCancel = TRUE;

    ThreadLock(ppopupmenu->spwndPopupMenu, &tlpwndPopupMenu);

    /*
     * Close all hierarchies from this point down.
     */
    xxxMNCloseHierarchy(ppopupmenu, pMenuState);

    /*
     * Unselect any items on this top level window
     */
    xxxMNSelectItem(ppopupmenu, pMenuState, MFMWFP_NOITEM);

    pMenuState->fMenuStarted = FALSE;

    pwndT = ppopupmenu->spwndNotify;

    ThreadLock(pwndT, &tlpwndT);

    xxxMNReleaseCapture();

    if (fTrackFlagsSet) {
        /*
         * Send a POPUPEND so people watching see them paired
         */
        if (FWINABLE()) {
            xxxWindowEvent(EVENT_SYSTEM_MENUPOPUPEND,
                    ppopupmenu->spwndPopupMenu, OBJID_CLIENT, 0, 0);
        }

        UserAssert(ppopupmenu->spwndPopupMenu != ppopupmenu->spwndPopupMenu->head.rpdesk->spwndMenu);
        xxxDestroyWindow(ppopupmenu->spwndPopupMenu);
    }

    if (pwndT == NULL) {
        ThreadUnlock(&tlpwndT);
        ThreadUnlock(&tlpwndPopupMenu);
        return;
    }

    /*
     * SMS_NOMENU hack so we can send MenuSelect messages with
     * (loword(lparam) = -1) when
     * the menu pops back up for the CBT people. In 3.0, all WM_MENUSELECT
     * messages went through the message filter so go through the function
     * SendMenuSelect. We need to do this in 3.1 since WordDefect for Windows
     * depends on this.
     */
    xxxSendMenuSelect(pwndT, NULL, SMS_NOMENU, MFMWFP_NOITEM);

    if (FWINABLE()) {
        xxxWindowEvent(EVENT_SYSTEM_MENUEND, pwndT, (fIsSysMenu ?
            OBJID_SYSMENU : (fIsMenuBar ? OBJID_MENU : OBJID_WINDOW)),
            INDEXID_CONTAINER, 0);
    }

    if (fNotify) {
    /*
     * Notify app we are exiting the menu loop.  Mainly for WinOldApp 386.
     * wParam is 1 if a TrackPopupMenu else 0.
     */
        xxxSendMessage(pwndT, WM_EXITMENULOOP,
            ((fTrackFlagsSet && !fIsSysMenu)? 1 : 0), 0);
    }

    if (uMsg != 0) {
        PlayEventSound(USER_SOUND_MENUCOMMAND);
        pMenuState->cmdLast = cmd;
        if (!fSynchronous) {
            if (!fIsSysMenu && fTrackFlagsSet && !TestWF(pwndT, WFWIN31COMPAT)) {
                xxxSendMessage(pwndT, uMsg, cmd, lParam);
            } else {
                _PostMessage(pwndT, uMsg, cmd, lParam);
            }
        }
    } else
        pMenuState->cmdLast = 0;

    ThreadUnlock(&tlpwndT);

    ThreadUnlock(&tlpwndPopupMenu);

}
/***************************************************************************\
* void MenuButtonDownHandler(PPOPUPMENU ppopupmenu, int posItemHit)
* effects: Handles a mouse down on the menu associated with ppopupmenu at
* item index posItemHit.  posItemHit could be MFMWFP_NOITEM if user hit on a
* menu where no item exists.
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/

void xxxMNButtonDown(
    PPOPUPMENU ppopupmenu,
    PMENUSTATE pMenuState,
    UINT posItemHit, BOOL fClick)
{
    PITEM  pItem;
    BOOL   fOpenHierarchy;

    /*
     * A different item was hit than is currently selected, so select it
     * and drop its menu if available.  Make sure we toggle click state.
     */
    if (ppopupmenu->posSelectedItem != posItemHit) {
        /*
         * We are clicking on a new item, not moving the mouse over to it.
         * So reset cancel toggle state.  We don't want button up from
         * this button down to cancel.
         */
        if (fClick) {
            fOpenHierarchy = TRUE;
            ppopupmenu->fToggle = FALSE;
        }
        else
        {
            fOpenHierarchy = (ppopupmenu->fDropNextPopup != 0);
        }


        /*
         * If the item has a popup and isn't disabled, open it.  Note that
         * selecting this item will cancel any hierarchies associated with
         * the previously selected item.
         */
        pItem = xxxMNSelectItem(ppopupmenu, pMenuState, posItemHit);
        if (MNIsPopupItem(pItem) && fOpenHierarchy) {
            /* Punt if menu was destroyed. */
            if (xxxMNOpenHierarchy(ppopupmenu, pMenuState) == (PWND)-1) {
                return;
            }
        }
    } else {
        /*
         * We are moving over to the already-selected item.  If we are
         * clicking for real, reset cancel toggle state.  We want button
         * up to cancel if on same item.  Otherwise, do nothing if just
         * moving...
         */
        if (fClick) {
            ppopupmenu->fToggle = TRUE;
        }

        if (!xxxMNHideNextHierarchy(ppopupmenu) && fClick && xxxMNOpenHierarchy(ppopupmenu, pMenuState))
            ppopupmenu->fToggle = FALSE;
    }

    if (fClick) {
        pMenuState->fButtonDown = TRUE;
        xxxMNDoScroll(ppopupmenu, posItemHit, TRUE);
    }
}

/***************************************************************************\
* MNSetTimerToAutoDissmiss
*
* History:
*  11/14/96 GerardoB  Created
\***************************************************************************/
void MNSetTimerToAutoDismiss(PMENUSTATE pMenuState, PWND pwnd)
{
    if (pMenuState->fAutoDismiss && !pMenuState->fAboutToAutoDismiss) {
        if (_SetTimer(pwnd, IDSYS_MNAUTODISMISS, 16 * gdtMNDropDown, NULL)) {
            pMenuState->fAboutToAutoDismiss = TRUE;
        } else {
            RIPMSG0(RIP_WARNING, "xxxMNMouseMove: Failed to set autodismiss timer");
        }
    }
}
/***************************************************************************\
* void MenuMouseMoveHandler(PPOPUPMENU ppopupmenu, POINT screenPt)
* Handles a mouse move to the given point.
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/

void xxxMNMouseMove(
    PPOPUPMENU ppopup,
    PMENUSTATE pMenuState,
    POINTS ptScreen)
{
    LONG_PTR cmdHitArea;
    UINT uFlags;
    UINT cmdItem;
    PWND pwnd;
    TL tlpwndT;


    if (!IsRootPopupMenu(ppopup)) {
        RIPMSG0(RIP_ERROR,
            "MenuMouseMoveHandler() called for a non top most menu");
        return;
    }

    /*
     * Ignore mouse moves that aren't really moves.  MSTEST jiggles
     * the mouse for some reason.  And windows coming and going will
     * force mouse moves, to reset the cursor.
     */
    if ((ptScreen.x == pMenuState->ptMouseLast.x) && (ptScreen.y == pMenuState->ptMouseLast.y))
        return;

    pMenuState->ptMouseLast.x = ptScreen.x;
    pMenuState->ptMouseLast.y = ptScreen.y;

    /*
     * Find out where this mouse move occurred.
     */
    cmdHitArea = xxxMNFindWindowFromPoint(ppopup, &cmdItem, ptScreen);

    /*
     * If coming from an IDropTarget call out, remember the hit test
     */
    if (pMenuState->fInDoDragDrop) {
        xxxMNUpdateDraggingInfo(pMenuState, cmdHitArea, cmdItem);
    }

    if (pMenuState->mnFocus == KEYBDHOLD) {
        /*
         * Ignore mouse moves when in keyboard mode if the mouse isn't over any
         * menu at all.  Also ignore mouse moves if over minimized window,
         * because we pretend that its entire window is like system menu.
         */
        if ((cmdHitArea == MFMWFP_OFFMENU) ||
            ((cmdHitArea == MFMWFP_NOITEM) && TestWF(ppopup->spwndNotify, WFMINIMIZED))) {
            return;
        }

        pMenuState->mnFocus = MOUSEHOLD;
    }

    if (cmdHitArea == MFMWFP_ALTMENU) {
        /*
         * User clicked in the other menu so switch to it ONLY IF
         * MOUSE IS DOWN.  Usability testing proves that people frequently
         * get kicked into the system menu accidentally when browsing the
         * menu bar.  We support the Win3.1 behavior when the mouse is
         * down however.
         */
        if (pMenuState->fButtonDown) {
            xxxMNSwitchToAlternateMenu(ppopup);
            cmdHitArea = MFMWFP_NOITEM;
        } else
            goto OverNothing;
    }

    if (cmdHitArea == MFMWFP_NOITEM) {
        /*
         * Mouse move occurred to an item in the main menu bar. If the item
         * is different than the one already selected, close up the current
         * one, select the new one and drop its menu. But if the item is the
         * same as the one currently selected, we need to pull up any popups
         * if needed and just keep the current level visible.  Hey, this is
         * the same as a mousedown so lets do that instead.
         */
        xxxMNButtonDown(ppopup, pMenuState, cmdItem, FALSE);
        return;
    } else if (cmdHitArea != 0) {
        /* This is a popup window we moved onto. */
        pwnd = (PWND)(cmdHitArea);
        ThreadLock(pwnd, &tlpwndT);

        UserAssert(TestWF(pwnd, WFVISIBLE));

        /*
         * Modeless menus don't capture the mouse, so track it to know
         *  when it leaves the popup.
         */
        ppopup = ((PMENUWND)pwnd)->ppopupmenu;
        if (pMenuState->fModelessMenu
                && !pMenuState->fInDoDragDrop
                && !ppopup->fTrackMouseEvent) {

            TRACKMOUSEEVENT tme;

            /* tme.cbSize = sizeof(TRACKMOUSEEVENT); Not checked on kernel side */
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = PtoH(pwnd);
            TrackMouseEvent(&tme);
            ppopup->fTrackMouseEvent = TRUE;

            /*
             * We just entered this window so make sure the cursor
             *  is properly set.
             */
            xxxSendMessage(pwnd, WM_SETCURSOR, (WPARAM)HWq(pwnd), MAKELONG(MSGF_MENU, 0));

        }

        /*
         * Select the item.
         */
        uFlags = (UINT)xxxSendMessage(pwnd, MN_SELECTITEM, (WPARAM)cmdItem, 0L);
        if ((uFlags & MF_POPUP) && !(uFlags & MFS_GRAYED)) {
           /*
            * User moved back onto an item with a hierarchy.  Hide the
            * the dropped popup.
            */
           if (!xxxSendMessage(pwnd, MN_SETTIMERTOOPENHIERARCHY, 0, 0L)) {
                xxxMNHideNextHierarchy(ppopup);
           }
        }
        ThreadUnlock(&tlpwndT);
    } else
OverNothing:
    {
        /* We moved off all menu windows... */
        if (ppopup->spwndActivePopup != NULL) {
            pwnd = ppopup->spwndActivePopup;

            ThreadLock(pwnd, &tlpwndT);
            xxxSendMessage(pwnd, MN_SELECTITEM, MFMWFP_NOITEM, 0L);
            MNSetTimerToAutoDismiss(pMenuState, pwnd);
            ThreadUnlock(&tlpwndT);
        } else {
            xxxMNSelectItem(ppopup, pMenuState, MFMWFP_NOITEM);
        }

    }
}


/***************************************************************************\
* void MenuButtonUpHandler(PPOPUPMENU ppopupmenu, int posItemHit)
* effects: Handles a mouse button up at the given point.
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/

void xxxMNButtonUp(
    PPOPUPMENU ppopup,
    PMENUSTATE pMenuState,
    UINT posItemHit,
    LPARAM lParam)
{
    PITEM pItem;

    if (!pMenuState->fButtonDown) {

        /*
         * Ignore if button was never down...  Really shouldn't happen...
         */
        return;
    }

    if (posItemHit == MFMWFP_NOITEM) {
        RIPMSG0(RIP_WARNING, "button up on no item");
        goto ExitButtonUp;
    }

    if (ppopup->posSelectedItem != posItemHit) {
        goto ExitButtonUp;
    }

    if (ppopup->fIsMenuBar) {

        /*
         * Handle button up in menubar specially.
         */
        if (ppopup->fHierarchyDropped) {
            if (!ppopup->fToggle) {
                goto ExitButtonUp;
            } else {
                /*
                 * Cancel menu now.
                 */
                ppopup->fToggle = FALSE;
                xxxMNDismiss(pMenuState);
                return;
            }
        }
    } else if (ppopup->fShowTimer) {
        ppopup->fToggle = FALSE;

        /*
         * Open hierarchy on popup
         */
        xxxMNOpenHierarchy(ppopup, pMenuState);

        goto ExitButtonUp;
    }

    /*
     * If nothing is selected, get out.  This occurs mainly on unbalanced
     * multicolumn menus where one of the columns isn't completely full.
     */
    if (ppopup->posSelectedItem == MFMWFP_NOITEM)
        goto ExitButtonUp;

    if (ppopup->posSelectedItem >= ppopup->spmenu->cItems)
        goto ExitButtonUp;

    /*
     * Get a pointer to the currently selected item in this menu.
     */
    pItem = &(ppopup->spmenu->rgItems[ppopup->posSelectedItem]);

    /*
     * Kick out of menu mode if user clicked on a non-separator, enabled,
     * non-hierarchical item.
     *
     * BOGUS
     * Why doesn't MFS_GRAYED check work for separators now?  Find out later.
     */
    if (!(pItem->fType & MFT_SEPARATOR)
            && !(pItem->fState & MFS_GRAYED)
            && (pItem->spSubMenu == NULL)) {

        xxxMNDismissWithNotify(pMenuState, ppopup->spmenu, pItem,
                               ppopup->posSelectedItem, lParam);
        return;
    }

ExitButtonUp:
    pMenuState->fButtonDown =
    pMenuState->fButtonAlwaysDown = FALSE;
}


/***************************************************************************\
*UINT MenuSetTimerToOpenHierarchy(PPOPUPMENU ppopupmenu)
* Given the current selection, set a timer to show this hierarchy if
* valid else return 0. If a timer should be set but couldn't return -1.
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
\***************************************************************************/
UINT MNSetTimerToOpenHierarchy(
    PPOPUPMENU ppopup)
{
    PITEM pItem;

    /*
     * No selection so fail
     */
    if (ppopup->posSelectedItem == MFMWFP_NOITEM)
        return(0);

    if (ppopup->posSelectedItem >= ppopup->spmenu->cItems)
        return(0);

    /*
     * Is item an enabled popup?
     * Get a pointer to the currently selected item in this menu.
     */
    pItem = ppopup->spmenu->rgItems + ppopup->posSelectedItem;
    if ((pItem->spSubMenu == NULL) || (pItem->fState & MFS_GRAYED))
        return(0);

    if (ppopup->fShowTimer
        || (ppopup->fHierarchyDropped
            && (ppopup->posSelectedItem == ppopup->posDropped))) {

        /*
         * A timer is already set or the hierarchy is already opened.
         */
        return 1;
    }

    if (!_SetTimer(ppopup->spwndPopupMenu, IDSYS_MNSHOW, gdtMNDropDown, NULL))
        return (UINT)-1;

    ppopup->fShowTimer = TRUE;

    return 1;
}



/***************************************************************************\
* MNSetTimerToCloseHierarchy
*
\***************************************************************************/

UINT MNSetTimerToCloseHierarchy(PPOPUPMENU ppopup)
{

    if (!ppopup->fHierarchyDropped)
        return(0);

    if (ppopup->fHideTimer)
        return(1);

    if (!_SetTimer(ppopup->spwndPopupMenu, IDSYS_MNHIDE, gdtMNDropDown, NULL))
        return((UINT) -1);

    ppopup->fHideTimer = TRUE;

    ppopup = ((PMENUWND)(ppopup->spwndNextPopup))->ppopupmenu;
    ppopup->fAboutToHide = TRUE;

    return(1);
}


/***************************************************************************\
* xxxCallHandleMenuMessages
*
* Modeless menus don't have a modal loop so we don't see the messages until
*  they are dispatched to xxxMenuWindowProc. So we call this function to
*  process the message just like we would in the modal case, only that
*  the message has already been pulled out of the queue.
* This is also calledfrom xxxScanSysQueue to pass mouse messages on the menu
*  bar or from xxxMNDragOver to upadate the mouse position when being draged over.
*
* History:
* 10/25/96 GerardoB  Created
\***************************************************************************/
BOOL xxxCallHandleMenuMessages(PMENUSTATE pMenuState, PWND pwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL fHandled;
    MSG msg;

    CheckLock(pwnd);

    UserAssert(pMenuState->fModelessMenu || pMenuState->fInDoDragDrop);

    /*
     * Since modeless menus don't capture the mouse, then we need to
     *  keep checking on the mouse button when the mouse is off the
     *  menu.
     * Note that we do not set fMouseOffMenu if fInDoDragDrop is set
     */
    if (pMenuState->fMouseOffMenu && pMenuState->fButtonDown) {
        UserAssert(!pMenuState->fInDoDragDrop && pMenuState->fModelessMenu);
        MNCheckButtonDownState(pMenuState);
    }

    /*
     * Setup the msg structure
     */
    msg.hwnd = HW(pwnd);
    msg.message = message;
    msg.wParam = wParam;

    /*
     * xxxHandleMenuMessages expects screen coordinates
     */
    if ((message >= WM_MOUSEFIRST) && (message <= WM_MOUSELAST)) {
        msg.lParam = MAKELONG(GET_X_LPARAM(lParam) + pwnd->rcClient.left,
                              GET_Y_LPARAM(lParam) + pwnd->rcClient.top);
    } else {
        msg.lParam = lParam;
    }

    /*
     * Not used by xxxHandleMenuMessages
     */
    msg.time = 0;
    msg.pt.x = msg.pt.x = 0;


    UserAssert(pMenuState->pGlobalPopupMenu != NULL);

    pMenuState->fInCallHandleMenuMessages = TRUE;
    fHandled = xxxHandleMenuMessages(&msg, pMenuState, pMenuState->pGlobalPopupMenu);
    pMenuState->fInCallHandleMenuMessages = FALSE;

    /*
     * If the message was handled and this is a modeless menu,
     *  check to see if it's time to go.
     */
    if (fHandled
            && pMenuState->fModelessMenu
            && ExitMenuLoop (pMenuState, pMenuState->pGlobalPopupMenu)) {

        xxxEndMenuLoop (pMenuState, pMenuState->pGlobalPopupMenu);
        xxxMNEndMenuState(TRUE);
    }

    return fHandled;
}
/***************************************************************************\
*
* History:
*  05-25-91 Mikehar Ported from Win3.1
*  08-12-96 jparsons Catch NULL lParam on WM_CREATE [51986]
\***************************************************************************/

LRESULT xxxMenuWindowProc(
    PWND pwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    BOOL fIsRecursedMenu;
    LRESULT lRet;
    PAINTSTRUCT ps;
    PPOPUPMENU ppopupmenu;
    PMENUSTATE pMenuState;
    PMENU      pmenu;
    PITEM      pItem;
    TL tlpmenu;
    TL tlpwndNotify;
    PDESKTOP pdesk = pwnd->head.rpdesk;
    POINT ptOrg;
    HDC hdcAni;

    CheckLock(pwnd);

    VALIDATECLASSANDSIZE(pwnd, message, wParam, lParam, FNID_MENU, WM_NCCREATE);

    /*
     * If we're not in menu mode or this window is just being created,
     *  there are only few messages we care about.
     */
    pMenuState = GetpMenuState(pwnd);
    ppopupmenu = ((PMENUWND)pwnd)->ppopupmenu;
    pmenu = (ppopupmenu != NULL ? ppopupmenu->spmenu : NULL);
    if ((pMenuState == NULL) || (pmenu == NULL)) {
        switch (message) {
            case WM_NCCREATE:
            case WM_FINALDESTROY:
                break;

            case MN_SETHMENU:
                if (ppopupmenu != NULL) {
                    break;
                } else {
                    return 0;
                }

            default:
                goto CallDWP;
        }
    } else {
        /*
         * TPM_RECURSE support: make sure we grab the proper pMenuState.
         */
        fIsRecursedMenu = ((ppopupmenu->ppopupmenuRoot != NULL)
                            && IsRecursedMenuState(pMenuState, ppopupmenu));
        if (fIsRecursedMenu) {
            while (IsRecursedMenuState(pMenuState, ppopupmenu)
                    && (pMenuState->pmnsPrev != NULL)) {
                pMenuState = pMenuState->pmnsPrev;
            }
            UserAssert(pMenuState->pGlobalPopupMenu == ppopupmenu->ppopupmenuRoot);
        }

        Validateppopupmenu(ppopupmenu);

        /*
         * If this is a modeless menu, give xxxHandleMenuMessages the first
         *  shot at the message
         */
        if (pMenuState->fModelessMenu && !pMenuState->fInCallHandleMenuMessages) {
            /*
             * If this is a recursed menu, we don't want to process any
             *  input for it until the current menu goes away.
             */
            if (fIsRecursedMenu) {
                if (((message >= WM_MOUSEFIRST) && (message <= WM_MOUSELAST))
                        || ((message >= WM_KEYFIRST) && (message <= WM_KEYLAST))
                        || ((message >= WM_NCMOUSEFIRST) && (message <= WM_NCMOUSELAST))) {

                    goto CallDWP;
                }
            } else {
                if (xxxCallHandleMenuMessages(pMenuState, pwnd, message, wParam, lParam)) {
                    return 0;
                }
            }
        }
    } /* else of if ((pMenuState == NULL) || (ppopupmenu == NULL)) */

    switch (message) {
    case WM_NCCREATE:
        /*
         * Ignore evil messages to prevent leaks.
         * Use RIP_ERROR for a while to make sure to see if we're getting here
         */
        if (((PMENUWND)pwnd)->ppopupmenu != NULL) {
            RIPMSG1(RIP_ERROR, "xxxMenuWindowProc: evil WM_NCCREATE. already initialized. pwnd:%p", pwnd);
            return FALSE;
        }
        ppopupmenu = MNAllocPopup(TRUE);
        if (ppopupmenu == NULL) {
            return FALSE;
        }

        ((PMENUWND)pwnd)->ppopupmenu = ppopupmenu;
        ppopupmenu->posSelectedItem = MFMWFP_NOITEM;
        Lock(&(ppopupmenu->spwndPopupMenu), pwnd);
        return TRUE;

    case WM_NCCALCSIZE:
        xxxDefWindowProc(pwnd, message, wParam, lParam);
        if (pmenu->dwArrowsOn != MSA_OFF) {
            InflateRect((PRECT)lParam, 0, -gcyMenuScrollArrow);
        }
        break;

    case WM_ERASEBKGND:
        if (pmenu->hbrBack != NULL) {
            MNEraseBackground ((HDC) wParam, pmenu,
                    0, 0,
                    pwnd->rcClient.right - pwnd->rcClient.left,
                    pwnd->rcClient.bottom - pwnd->rcClient.top);
            return TRUE;
        } else {
            goto CallDWP;
        }
        break;

    case WM_PRINT:
         /*
          * default processing of WM_PRINT does not handle custom non-
          * client painting -- which scrollable menus have -- so take
          * care of drawing nonclient area and then let DefWindowProc
          * handle the rest
          */
        if ((lParam & PRF_NONCLIENT) && (pmenu->dwArrowsOn != MSA_OFF)) {

            MNDrawFullNC(pwnd, (HDC)wParam, ppopupmenu);
            GreGetWindowOrg((HDC)wParam, &ptOrg);
            GreSetWindowOrg((HDC)wParam,
                  ptOrg.x - MNXBORDER,
                  ptOrg.y - MNYBORDER - gcyMenuScrollArrow,
                  NULL);
            xxxDefWindowProc(pwnd, message, wParam, lParam & ~PRF_NONCLIENT);
            GreSetWindowOrg((HDC)wParam, ptOrg.x, ptOrg.y, NULL);

        } else {
            goto CallDWP;
        }
        break;

    case WM_WINDOWPOSCHANGING:
        if (!(((LPWINDOWPOS)lParam)->flags & SWP_SHOWWINDOW))
            goto CallDWP;

        if (!TestEffectUP(MENUANIMATION) || !(ppopupmenu->iDropDir & PAS_OUT)
            || (GetAppCompatFlags2(VER40) & GACF2_ANIMATIONOFF)) {
NoAnimation:
            ppopupmenu->iDropDir &= ~PAS_OUT;
            goto CallDWP;
        }

        /*
         * Create the animation bitmap.
         */
        pMenuState->cxAni = pwnd->rcWindow.right - pwnd->rcWindow.left;
        pMenuState->cyAni = pwnd->rcWindow.bottom - pwnd->rcWindow.top;

        if (TestALPHA(MENUFADE)) {
            if ((hdcAni = CreateFade(pwnd, NULL, CMS_MENUFADE,
                    FADE_SHOW | FADE_MENU)) == NULL) {
                goto NoAnimation;
            }
        } else {

            if (!MNCreateAnimationBitmap(pMenuState, pMenuState->cxAni,
                    pMenuState->cyAni)) {
                goto NoAnimation;
            }

            /*
             * We shouldn't be animating at this time.
             */
            UserAssert(pMenuState->hdcWndAni == NULL);

            /*
             * This window must be the active popup
             */
            UserAssert(pMenuState->pGlobalPopupMenu->spwndActivePopup == pwnd);

            /*
             * Initialize animation info
             */
            pMenuState->hdcWndAni = _GetDCEx(pwnd, HRGN_FULL, DCX_WINDOW | DCX_USESTYLE | DCX_INTERSECTRGN);
            pMenuState->iAniDropDir = ppopupmenu->iDropDir;
            pMenuState->ixAni = (pMenuState->iAniDropDir & PAS_HORZ) ? 0 : pMenuState->cxAni;
            pMenuState->iyAni = (pMenuState->iAniDropDir & PAS_VERT) ? 0 : pMenuState->cyAni;
            hdcAni = pMenuState->hdcAni;
        }

        /*
         * MFWINDOWDC is used by MNEraseBackground to determine where the
         *  brush org should be set.
         */
        SetMF(pmenu, MFWINDOWDC);

        xxxSendMessage(pwnd, WM_PRINT, (WPARAM)hdcAni, PRF_CLIENT | PRF_NONCLIENT | PRF_ERASEBKGND);

        ClearMF(pmenu, MFWINDOWDC);

        /*
         *If owner draw, we just passed hdcAni to the client side.
         *  The app might have deleted it (??); no blue screen seems to
         *  happen but only painting on that DC will fail from
         *  now on. I won't waste time handling this unless it turns
         *  out to be a problem (it's unlikely an app would do so).
         */
        UserAssert(GreValidateServerHandle(hdcAni, DC_TYPE));

        /*
         * While the window is still hidden, load the first fade animation
         * frame to avoid flicker when the window is actually shown.
         *
         * There would still be flicker with slide animations, though. It
         * could be fixed by using the window region, similar to
         * AnimateWindow. For now, too many functions would become xxx, so
         * let's not do it, unless it becomes a big issue.
         */
        if (TestFadeFlags(FADE_MENU)) {
            ShowFade();
        }
        goto CallDWP;

    case WM_WINDOWPOSCHANGED:
        if (!(((LPWINDOWPOS)lParam)->flags & SWP_SHOWWINDOW))
            goto CallDWP;

        /*
         * If not animating, nothing else to do here.
         */
        if (!(ppopupmenu->iDropDir & PAS_OUT))
            goto CallDWP;

        /*
         * Start the animation cycle now.
         */
        if (TestFadeFlags(FADE_MENU)) {
            StartFade();
        } else {
            pMenuState->dwAniStartTime = NtGetTickCount();
            _SetTimer(pwnd, IDSYS_MNANIMATE, 1, NULL);
        }
        ppopupmenu->iDropDir &= ~PAS_OUT;
        goto CallDWP;

    case WM_NCPAINT:

        if (ppopupmenu->iDropDir & PAS_OUT) {

            /*
             * When animating, validate itself to ensure no further drawing
             * that is not related to the animation.
             */
            xxxValidateRect(pwnd, NULL);
        } else {

            /*
             * If we have scroll bars, draw them
             */
            if (pmenu->dwArrowsOn != MSA_OFF) {

                HDC hdc = _GetDCEx(pwnd, (HRGN)wParam,
                        DCX_USESTYLE | DCX_WINDOW | DCX_INTERSECTRGN | DCX_NODELETERGN | DCX_LOCKWINDOWUPDATE);
                MNDrawFullNC(pwnd, hdc, ppopupmenu);
                _ReleaseDC(hdc);
            } else {
                goto CallDWP;
            }
        }
        break;

    case WM_PRINTCLIENT:
        ThreadLock(pmenu, &tlpmenu);
        xxxMenuDraw((HDC)wParam, pmenu);
        ThreadUnlock(&tlpmenu);
        break;

      case WM_FINALDESTROY:
        /*
         * If we're animating, we must haved been killed in a rude way....
         */
        UserAssert((pMenuState == NULL) || (pMenuState->hdcWndAni == NULL));

        /*
         * If this is a drag and drop menu, then call RevokeDragDrop.
         */
        if ((pMenuState != NULL) && pMenuState->fDragAndDrop) {
            if (!SUCCEEDED(xxxClientRevokeDragDrop(HW(pwnd)))) {
                RIPMSG1(RIP_ERROR, "xxxMenuWindowProc: xxxClientRevokeRegisterDragDrop failed:%#p", pwnd);
            }
        }

        xxxMNDestroyHandler(ppopupmenu);
        return 0;


      case WM_PAINT:
        ThreadLock(pmenu, &tlpmenu);
        xxxBeginPaint(pwnd, &ps);
        xxxMenuDraw(ps.hdc, pmenu);
        xxxEndPaint(pwnd, &ps);
        ThreadUnlock(&tlpmenu);
        break;

    case WM_CHAR:
    case WM_SYSCHAR:
        xxxMNChar(ppopupmenu, pMenuState, (UINT)wParam);
        break;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        xxxMNKeyDown(ppopupmenu, pMenuState, (UINT)wParam);
        break;

    case WM_TIMER:
        switch (wParam) {
            case IDSYS_MNSHOW:
                /*
                 * Open the window and kill the show timer.
                 *
                 * Cancel any toggle state we might have.  We don't
                 * want to dismiss this on button up if shown from
                 * button down.
                 */
                ppopupmenu->fToggle = FALSE;
                xxxMNOpenHierarchy(ppopupmenu, pMenuState);
                break;

            case IDSYS_MNHIDE:
                ppopupmenu->fToggle = FALSE;
                xxxMNCloseHierarchy(ppopupmenu,pMenuState);
                break;

            case IDSYS_MNUP:
            case IDSYS_MNDOWN:
                if (pMenuState->fButtonDown) {
                    xxxMNDoScroll(ppopupmenu, (UINT)wParam, FALSE);
                } else {
                    _KillTimer(pwnd, (UINT)wParam);
                }
                break;

            case IDSYS_MNANIMATE:
                if (pMenuState->hdcWndAni != NULL) {
                    MNAnimate(pMenuState, TRUE);
                } else {
                    /*
                     * This timer shouldn't be set. Left over in msg queue?
                     */
                    UserAssert(pMenuState->hdcWndAni != NULL);
                }
                break;

            case IDSYS_MNAUTODISMISS:
                /*
                 * This is a one shot timer, so kill it.
                 * Dismiss the popup if the flag hasn't been reset.
                 */
                _KillTimer(pwnd, IDSYS_MNAUTODISMISS);
                if (pMenuState->fAboutToAutoDismiss) {
                    goto EndMenu;
                }
        }
        break;

    /*
     * Menu messages.
     */
    case MN_SETHMENU:

         /*
          * wParam - new hmenu to associate with this menu window
          * Don't let them set the spmenu to NULL of we have to deal with
          *  that all over. Use RIP_ERROR for a while to make sure this is OK
          */
        if (wParam != 0) {
            if ((wParam = (WPARAM)ValidateHmenu((HMENU)wParam)) == 0) {
                break;
            }
            LockPopupMenu(ppopupmenu, &(ppopupmenu->spmenu), (PMENU)wParam);
        } else {
            RIPMSG1(RIP_ERROR, "xxxMenuWindowProc: MN_SETHMENU ignoring NULL wParam. pwnd:%p", pwnd);
        }
        break;

    case MN_GETHMENU:

        /*
         * returns the hmenu associated with this menu window
         */
        return (LRESULT)PtoH(pmenu);

    case MN_SIZEWINDOW:
        {

            /*
             * Computes the size of the menu associated with this window and resizes
             * it if needed.  Size is returned x in loword, y in highword.  wParam
             * is 0 to just return new size.  wParam is non zero if we should also resize
             * window.
             * When called by xxxMNUpdateShownMenu, we might need to redraw the
             *  frame (i.e, the scrollbars). So we check for MNSW_DRAWFRAME in wParam.
             *  If some app is sending this message and that bit is set, then we'll
             *  do some extra work, but I think everything should be cool.
             */
            int         cx, cy;
            PMONITOR    pMonitor;

            /*
             * Call menucomputeHelper directly since this is the entry point for
             * non toplevel menu bars.
             */
            if (pmenu == NULL)
                break;

            ThreadLockAlways(pmenu, &tlpmenu);
            ThreadLock(ppopupmenu->spwndNotify, &tlpwndNotify);
            UserAssert(pmenu->spwndNotify == ppopupmenu->spwndNotify);
            xxxMNCompute(pmenu, ppopupmenu->spwndNotify, 0, 0, 0, 0);
            ThreadUnlock(&tlpwndNotify);
            ThreadUnlock(&tlpmenu);

            pMonitor = _MonitorFromWindow(pwnd, MONITOR_DEFAULTTOPRIMARY);
            cx = pmenu->cxMenu;
            cy = MNCheckScroll(pmenu, pMonitor);

            /*
             * Size the window?
             */
            if (wParam != 0) {
                LONG    lPos;
                int     x, y;
                DWORD   dwFlags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER;

                /*
                 * Need to redraw the frame?
                 */
                if (wParam & MNSW_DRAWFRAME) {
                    dwFlags |= SWP_DRAWFRAME;
                }

                /*
                 * If the window is visible, it's being resized while
                 *  shown. So make sure that it still fits on the screen
                 *  (i.e, move it to the best pos).
                 */
                if (TestWF(pwnd, WFVISIBLE)) {
                    lPos = FindBestPos(
                            pwnd->rcWindow.left,
                            pwnd->rcWindow.top,
                            cx,
                            cy,
                            NULL,
                            0,
                            ppopupmenu,
                            pMonitor);

                    x = GET_X_LPARAM(lPos);
                    y = GET_Y_LPARAM(lPos);
                } else {
                    dwFlags |= SWP_NOMOVE;
                }

                xxxSetWindowPos(
                        pwnd,
                        PWND_TOP,
                        x,
                        y,
                        cx + 2*SYSMET(CXFIXEDFRAME),    /* For shadow */
                        cy + 2*SYSMET(CYFIXEDFRAME),    /* For shadow */
                        dwFlags);

            }

            return MAKELONG(cx, cy);
        }

    case MN_OPENHIERARCHY:
        {
            PWND pwndT;
            /*
             * Opens one level of the hierarchy at the selected item, if
             * present. Return 0 if error, else hwnd of opened hierarchy.
             */
            pwndT = xxxMNOpenHierarchy(ppopupmenu, pMenuState);
            return (LRESULT)HW(pwndT);
        }

    case MN_CLOSEHIERARCHY:
        xxxMNCloseHierarchy(ppopupmenu, pMenuState);
        break;

    case MN_SELECTITEM:
        /*
         * wParam - the item to select. Must be a valid
         * Returns the item flags of the wParam (0 if failure)
         */
        if ((wParam >= pmenu->cItems) && (wParam < MFMWFP_MINVALID)) {
            UserAssertMsg1(FALSE, "Bad wParam %x for MN_SELECTITEM", wParam);
            break;
        }

        pItem = xxxMNSelectItem(ppopupmenu, pMenuState, (UINT)wParam);
        if (pItem != NULL) {
            return((LONG)(DWORD)(WORD)(pItem->fState |
                ((pItem->spSubMenu != NULL) ? MF_POPUP : 0)));
        }

        break;

    case MN_SELECTFIRSTVALIDITEM: {
        UINT item;

        item = MNFindNextValidItem(pmenu, -1, 1, TRUE);
        xxxSendMessage(pwnd, MN_SELECTITEM, item, 0L);
        return (LONG)item;
      }

    case MN_CANCELMENUS:

        /*
         * Cancels all menus, unselects everything, destroys windows, and cleans
         * everything up for this hierarchy.  wParam is the command to send and
         * lParam says if it is valid or not.
         */
        xxxMNCancel(pMenuState, (UINT)wParam, (BOOL)LOWORD(lParam), 0);
        break;

    case MN_FINDMENUWINDOWFROMPOINT:
        /*
         * lParam is point to search for from this hierarchy down.
         * returns MFMWFP_* value or a pwnd.
         */
        lRet = xxxMNFindWindowFromPoint(ppopupmenu, (PUINT)wParam, MAKEPOINTS(lParam));

        /*
         * Convert return value to a handle.
         */
        if (IsMFMWFPWindow(lRet)) {
            return (LRESULT)HW((PWND)lRet);
        } else {
            return lRet;
        }


    case MN_SHOWPOPUPWINDOW:
        /*
         * Forces the dropped down popup to be visible and if modeless, also active.
         */
        PlayEventSound(USER_SOUND_MENUPOPUP);
        xxxShowWindow(pwnd, (pMenuState->fModelessMenu ? SW_SHOW : SW_SHOWNOACTIVATE));
        break;

    case MN_ACTIVATEPOPUP:
        /*
         * Activates a popup. This messages is posted in response to WM_ACTIVATEAPP
         *  or WM_ACTIVATE
         */
        UserAssert(pMenuState->fModelessMenu);
        xxxActivateThisWindow(pwnd, 0, 0);
        break;

    case MN_ENDMENU:
        /*
         * End the menu. This message is posted to avoid ending the menu
         *  at randmom moments. By posting the message, the request is
         *  queued after any pending/current processing.
         */
EndMenu:
        xxxEndMenuLoop(pMenuState, pMenuState->pGlobalPopupMenu);
        if (pMenuState->fModelessMenu) {
            UserAssert(!pMenuState->fInCallHandleMenuMessages);
            xxxMNEndMenuState(TRUE);
        }
        return 0;

     case MN_DODRAGDROP:
        /*
         * Let the app know that the user is dragging.
         */
        if (pMenuState->fDragging
                && (ppopupmenu->spwndNotify != NULL)
                && IsMFMWFPWindow(pMenuState->uButtonDownHitArea)) {
            /*
             * Get the pmenu that contains the item being dragged
             */
             pmenu = (((PMENUWND)pMenuState->uButtonDownHitArea)->ppopupmenu)->spmenu;
            /*
             * If this is a modal menu, release the capture lock so
             *  DoDragDrop (if called) can get it.
             */
            if (!pMenuState->fModelessMenu) {
                UserAssert(PtiCurrent()->pq->QF_flags & QF_CAPTURELOCKED);
                PtiCurrent()->pq->QF_flags &= ~QF_CAPTURELOCKED;
            }

            LockMenuState(pMenuState);
            ThreadLockAlways(ppopupmenu->spwndNotify, &tlpwndNotify);

            /*
             * Give them a chance to call DoDragDrop
             */
            pMenuState->fInDoDragDrop = TRUE;
            lRet = xxxSendMessage(ppopupmenu->spwndNotify, WM_MENUDRAG,
                                  pMenuState->uButtonDownIndex, (LPARAM)PtoH(pmenu));
            pMenuState->fInDoDragDrop = FALSE;

            if (lRet == MND_ENDMENU) {
                /*
                 * Go away.
                 */
                ThreadUnlock(&tlpwndNotify);
                if (!xxxUnlockMenuState(pMenuState)) {
                    goto EndMenu;
                } else {
                    return 0;
                }
                break;
             } else {
                 /*
                  * If the user starts dragging, we always
                  *  ignore the following button up
                  */
                 pMenuState->fIgnoreButtonUp = TRUE;
             }

            /*
             * Check the button state since we might have not seen the button up
             * If so, this will cancel the dragging state
             */
            MNCheckButtonDownState(pMenuState);

            /*
             * If this is a modal menu, make sure we recover capture
             */
            if (!pMenuState->fModelessMenu) {
                xxxMNSetCapture(ppopupmenu);
            }

            ThreadUnlock(&tlpwndNotify);
            xxxUnlockMenuState(pMenuState);
        }
        return 0;

    case MN_BUTTONDOWN:

        /*
         * wParam is position (index) of item the button was clicked on.
         * Must be a valid
         */
        if ((wParam >= pmenu->cItems) && (wParam < MFMWFP_MINVALID)) {
            UserAssertMsg1(FALSE, "Bad wParam %x for MN_BUTTONDOWN", wParam);
            break;
        }
        xxxMNButtonDown(ppopupmenu, pMenuState, (UINT)wParam, TRUE);
        break;

    case MN_MOUSEMOVE:

        /*
         * lParam is mouse move coordinate wrt screen.
         */
        xxxMNMouseMove(ppopupmenu, pMenuState, MAKEPOINTS(lParam));
        break;

    case MN_BUTTONUP:

        /*
         * wParam is position (index) of item the button was up clicked on.
         */
        if ((wParam >= pmenu->cItems) && (wParam < MFMWFP_MINVALID)) {
            UserAssertMsg1(FALSE, "Bad wParam %x for MN_BUTTONUP", wParam);
            break;
        }
        xxxMNButtonUp(ppopupmenu, pMenuState, (UINT)wParam, lParam);
        break;

    case MN_SETTIMERTOOPENHIERARCHY:

        /*
         * Given the current selection, set a timer to show this hierarchy if
         * valid else return 0.
         */
        return (LONG)(WORD)MNSetTimerToOpenHierarchy(ppopupmenu);

    case MN_DBLCLK:
            //
            // User double-clicked on item.  wParamLo is the item.
            //
        xxxMNDoubleClick(pMenuState, ppopupmenu, (int)wParam);
        break;

    case WM_MOUSELEAVE:
        UserAssert(pMenuState->fModelessMenu);
        /*
         * If we're in DoDragDrop loop, we don't track the mouse
         *  when it goes off the menu window
         */
        pMenuState->fMouseOffMenu = !pMenuState->fInDoDragDrop;
        ppopupmenu->fTrackMouseEvent = FALSE;
        /*
         * See if we need to set the timer to autodismiss
         */
        MNSetTimerToAutoDismiss(pMenuState, pwnd);
        /*
         * If we left the active popup, remove the selection
         */
        if (ppopupmenu->spwndPopupMenu == pMenuState->pGlobalPopupMenu->spwndActivePopup) {
            xxxMNSelectItem(ppopupmenu, pMenuState, MFMWFP_NOITEM);
        }
        break;

    case WM_ACTIVATEAPP:
        if (pMenuState->fModelessMenu
                && (pwnd == pMenuState->pGlobalPopupMenu->spwndActivePopup)) {
            /*
             * If the application is getting activated,  we post a message
             *  to let the dust settle and then re-activate spwndPopupActive
             */
            if (wParam) {
                _PostMessage(pwnd, MN_ACTIVATEPOPUP, 0, 0);
                /*
                 * If we're not in the foregruond queue, we want to keep
                 *  the frame off.
                 * This flag will also tell us that if we lose activation
                 *  while coming to the foregrund (later), we don't want
                 *  to dismiss the menu.
                 */
                 pMenuState->fActiveNoForeground = (gpqForeground != PtiCurrent()->pq);
            }

            /*
             * Make the notification window frame show that we're active/inactive.
             * If the application is inactive but the user moves the mouse
             *  over the menu, then we can get this message when the first
             *  window in the app gets activated (i.e., the move causes a popup to
             *  be closed/opened). So turn on the frame only if we're in
             *  the foreground.
             */
            if (ppopupmenu->spwndNotify != NULL) {
                ThreadLockAlways(ppopupmenu->spwndNotify, &tlpwndNotify);
                xxxDWP_DoNCActivate(ppopupmenu->spwndNotify,
                                    ((wParam && !pMenuState->fActiveNoForeground) ? NCA_ACTIVE : NCA_FORCEFRAMEOFF),
                                    HRGN_FULL);
                ThreadUnlock(&tlpwndNotify);
            }
        }
        break;

     case WM_ACTIVATE:
         if (pMenuState->fModelessMenu) {
             /*
              * If activation is NOT going to a menu window or
              *  it's going to a recursed menu, bail
              */
             if ((LOWORD(wParam) == WA_INACTIVE)
                    && !pMenuState->fInCallHandleMenuMessages
                    && !pMenuState->pGlobalPopupMenu->fInCancel) {

                 lParam = (LPARAM)RevalidateHwnd((HWND)lParam);
                 if ((lParam != 0)
                     && ((GETFNID((PWND)lParam) != FNID_MENU)
                         || IsRecursedMenuState(pMenuState, ((PMENUWND)lParam)->ppopupmenu))) {
                     /*
                      * If we're just coming to the foreground, then
                      *  activate the popup later and stay up.
                      */
                     if (pMenuState->fActiveNoForeground
                            && (gpqForeground == PtiCurrent()->pq)) {

                         pMenuState->fActiveNoForeground = FALSE;
                         _PostMessage(pwnd, MN_ACTIVATEPOPUP, 0, 0);
                     } else {
                         /*
                          * Since the menu window is active, ending the menu
                          *  now would set a new active window, messing the
                          *  current activation that sent us this message.
                          *  so end the menu later.
                          */
                         _PostMessage(pwnd, MN_ENDMENU, 0, 0);
                         break;
                     }
                 }
             }
             goto CallDWP;
         } /* if (pMenuState->fModelessMenu) */

       /*
        * We must make sure that the menu window does not get activated.
        * Powerpoint 2.00e activates it deliberately and this causes problems.
        * We try to activate the previously active window in such a case.
        * Fix for Bug #13961 --SANKAR-- 09/26/91--
        */
       /*
        * In Win32, wParam has other information in the hi 16bits, so to
        * prevent infinite recursion, we need to mask off those bits
        * Fix for NT bug #13086 -- 23-Jun-1992 JonPa
        *
        */

       if (LOWORD(wParam)) {
            TL tlpwnd;
            /*
             * This is a super bogus hack. Let's start failing this for 5.0 apps.
             */
            if (Is500Compat(PtiCurrent()->dwExpWinVer)) {
                RIPMSG1(RIP_ERROR, "xxxMenuWindowProc: Menu window activated:%#p", pwnd);
                _PostMessage(pwnd, MN_ENDMENU, 0, 0);
                break;
            }

#if 0
           /*
            * Activate the previously active wnd
            */
           xxxActivateWindow(pwnd, AW_SKIP2);
#else
            /*
             * Try the previously active window.
             */
            if ((gpqForegroundPrev != NULL) &&
                    !FBadWindow(gpqForegroundPrev->spwndActivePrev) &&
                    !ISAMENU(gpqForegroundPrev->spwndActivePrev)) {
                pwnd = gpqForegroundPrev->spwndActivePrev;
            } else {

                /*
                 * Find a new active window from the top-level window list.
                 * Bug 78131: Make sure we don't loop for ever. This is a pretty
                 *  unusual scenario (in addtion, normally we should not hit this code path)
                 *  So let's use a counter to rule out the possibility that another
                 *  weird window configuration is going to make us loop for ever
                 */
                PWND pwndMenu = pwnd;
                UINT uCounter = 0;
                do {
                    pwnd = NextTopWindow(PtiCurrent(), pwnd, NULL, 0);
                    if (pwnd && !FBadWindow(pwnd->spwndLastActive) &&
                        !ISAMENU(pwnd->spwndLastActive)) {
                        pwnd = pwnd->spwndLastActive;
                        uCounter = 0;
                        break;
                    }
                } while ((pwnd != NULL) && (uCounter++ < 255));
                /*
                 * If we couldn't find a window, just bail.
                 */
                if (uCounter != 0) {
                    RIPMSG0(RIP_ERROR, "xxxMenuWindowProc: couldn't fix active window");
                    _PostMessage(pwndMenu, MN_ENDMENU, 0, 0);
                    break;
                }
            }

            if (pwnd != NULL) {
                PTHREADINFO pti = PtiCurrent();
                ThreadLockAlwaysWithPti(pti, pwnd, &tlpwnd);

                /*
                 * If GETPTI(pwnd) isn't pqCurrent this is a AW_SKIP* activation
                 * we'll want to a do a xxxSetForegroundWindow().
                 */
                if (GETPTI(pwnd)->pq != pti->pq) {

                    /*
                     * Only allow this if we're on the current foreground queue.
                     */
                    if (gpqForeground == pti->pq) {
                        xxxSetForegroundWindow(pwnd, FALSE);
                    }
                } else {
                    xxxActivateThisWindow(pwnd, 0, ATW_SETFOCUS);
                }

                ThreadUnlock(&tlpwnd);
            }
#endif
       }
       break;

     case WM_SIZE:
     case WM_MOVE:
       /*
        * When a popup has been sized/moved, we need to make
        *  sure any dropped hierarchy is moved accordingly.
        */
       if (ppopupmenu->spwndNextPopup != NULL) {
           pItem = MNGetpItem(ppopupmenu, ppopupmenu->posDropped);
           if (pItem != NULL) {
               int      x, y;
               PMONITOR pMonitorDummy;

               /*
                * If the dropped hierarchy needs to be recomputed, do it
                */
#define pmenuNext (((PMENUWND)ppopupmenu->spwndNextPopup)->ppopupmenu->spmenu)
              if (pmenuNext->cxMenu == 0) {
                  xxxSendMessage(ppopupmenu->spwndNextPopup, MN_SIZEWINDOW, MNSW_RETURNSIZE, 0L);
              }

              /*
               * Find out the new position
               */
              xxxMNPositionHierarchy(ppopupmenu, pItem,
                                     pmenuNext->cxMenu + (2 * SYSMET(CXFIXEDFRAME)),
                                     pmenuNext->cyMenu + (2 * SYSMET(CXFIXEDFRAME)),
                                     &x, &y, &pMonitorDummy);

              /*
               * Move it
               */
              ThreadLockAlways(ppopupmenu->spwndNextPopup, &tlpwndNotify);
              xxxSetWindowPos(ppopupmenu->spwndNextPopup, NULL,
                              x, y, 0, 0,
                              SWP_NOSIZE | SWP_NOZORDER | SWP_NOSENDCHANGING);
              ThreadUnlock(&tlpwndNotify);
#undef pmenuNext
           } /* if (pItem != NULL) */
       } /* if (ppopupmenu->spwndNextPopup != NULL) */
       break;

     case WM_NCHITTEST:
        /*
         * Since modeless menus don't capture the mouse, we
         *  process this message to make sure that we always receive
         *  a mouse move when the mouse in our window.
         * This also causes us to receive the WM_MOUSELEAVE only when
         *  the mouse leaves the window and not just the  client area.
         */
        if (pMenuState->fModelessMenu) {
            ptOrg.x = GET_X_LPARAM(lParam);
            ptOrg.y = GET_Y_LPARAM(lParam);
            if (PtInRect(&pwnd->rcWindow, ptOrg)) {
                return HTCLIENT;
            } else {
                return HTNOWHERE;
            }
        } else {
            goto CallDWP;
        }


    default:
CallDWP:
        return xxxDefWindowProc(pwnd, message, wParam, lParam);
    }

    return 0;
}
