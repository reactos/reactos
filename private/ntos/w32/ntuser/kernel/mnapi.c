/**************************** Module Header ********************************\
* Module Name: mnapi.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Rarely Used Menu API Functions
*
* History:
* 10-10-90 JimA       Cleanup.
* 03-18-91 IanJa      Window revaliodation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* xxxSetMenu
*
* Sets the given window's menu to the menu specified by the pMenu
* parameter.  If pMenu is Null, the window's current menu is removed (but
* not destroyed).
*
* History:
\***************************************************************************/

BOOL xxxSetMenu(
    PWND  pwnd,
    PMENU pMenu,
    BOOL  fRedraw)
{
    CheckLock(pwnd);
    CheckLock(pMenu);

    if (!TestwndChild(pwnd)) {

        LockWndMenu(pwnd, &pwnd->spmenu, pMenu);

        /*
         * only redraw the frame if the window is non-minimized --
         * even if it's not visible, we need RedrawFrame to recalc the NC size
         *
         * Added a check for (redraw) since the MDISetMenu() only needs to
         * set the menu and not perform any redraws.
         */
        if (!TestWF(pwnd, WFMINIMIZED) && fRedraw)
            xxxRedrawFrame(pwnd);

        return TRUE;
    }

    RIPERR0(ERROR_CHILD_WINDOW_MENU, RIP_VERBOSE, "");
    return FALSE;
}


/***************************************************************************\
* xxxSetSystemMenu
*
* !
*
* History:
\***************************************************************************/

BOOL xxxSetSystemMenu(
    PWND pwnd,
    PMENU pMenu)
{
    CheckLock(pwnd);
    CheckLock(pMenu);

    if (TestWF(pwnd, WFSYSMENU)) {
        PMENU pmenuT = pwnd->spmenuSys;
        if (LockWndMenu(pwnd, &pwnd->spmenuSys, pMenu))
            _DestroyMenu(pmenuT);

        MNPositionSysMenu(pwnd, pMenu);

        return TRUE;
    }

    RIPERR0(ERROR_NO_SYSTEM_MENU, RIP_VERBOSE, "");
    return FALSE;
}


/***************************************************************************\
* xxxSetDialogSystemMenu
*
* !
*
* History:
\***************************************************************************/

BOOL xxxSetDialogSystemMenu(
    PWND pwnd)
{
    PMENU pMenu;

    CheckLock(pwnd);

    pMenu = pwnd->head.rpdesk->spmenuDialogSys;
    if (pMenu == NULL) {
        pMenu = xxxLoadSysDesktopMenu (&pwnd->head.rpdesk->spmenuDialogSys, ID_DIALOGSYSMENU);
    }

    LockWndMenu(pwnd, &pwnd->spmenuSys, pMenu);

    return (pMenu != NULL);
}

/***************************************************************************\
* xxxEndMenu
*
* !
* Revalidation notes:
* o  xxxEndMenu must be called with a valid non-NULL pwnd.
* o  Revalidation is not required in this routine: pwnd is used at the start
*    to obtain pMenuState, and not used again.
*
* History:
\***************************************************************************/

void xxxEndMenu(
    PMENUSTATE pMenuState)
{
    BOOL fMenuStateOwner;
    PPOPUPMENU  ppopup;
    PTHREADINFO ptiCurrent;

    if ((ppopup = pMenuState->pGlobalPopupMenu) == NULL) {

        /*
         * We're not really in menu mode. This can happen
         *  if we are forced out of menu loop too soon; i.e, from
         *  inside xxxMNGetPopup or xxxTrackPopupMenuEx.
         */
         UserAssert(!pMenuState->fInsideMenuLoop && !pMenuState->fMenuStarted);
        return;
    }



    pMenuState->fInsideMenuLoop = FALSE;
    pMenuState->fMenuStarted = FALSE;
    /*
     * Mark the popup as destroyed so people will not use it anymore.
     * This means that root popups can be marked as destroyed before
     * actually being destroyed (nice and confusing).
     */
    ppopup->fDestroyed = TRUE;

    /*
     * Determine if this is the menu loop owner before calling back.
     * Only the owner can destroy the menu windows
     */
   ptiCurrent = PtiCurrent();
   fMenuStateOwner = (ptiCurrent == pMenuState->ptiMenuStateOwner);

    /*
     * Release mouse capture if we got it in xxxStartMenuState
     */
    if (ptiCurrent->pq->spwndCapture == pMenuState->pGlobalPopupMenu->spwndNotify) {
        xxxMNReleaseCapture();
    }

    /*
     * Bail if this is not the menu loop owner
     */
    if (!fMenuStateOwner) {
        RIPMSG1(RIP_WARNING, "xxxEndMenu: Thread %#p doesn't own the menu loop", ptiCurrent);
        return;
    }
    /*
     * If the menu loop is running on a thread different than the thread
     *  that owns spwndNotify, we can have two threads trying to end
     *  this menu at the same time.
     */
    if (pMenuState->fInEndMenu) {
        RIPMSG1(RIP_WARNING, "xxxEndMenu: already in EndMenu. pMenuState:%#p", pMenuState);
        return;
    }
    pMenuState->fInEndMenu = TRUE;

    if (pMenuState->pGlobalPopupMenu->spwndNotify != NULL) {
        if (!pMenuState->pGlobalPopupMenu->fInCancel) {
            xxxMNDismiss(pMenuState);
        }
    } else {
        BOOL    fTrackedPopup = ppopup->fIsTrackPopup;

        /*
         * This should do the same stuff as MenuCancelMenus but not send any
         * messages...
         */
        xxxMNCloseHierarchy(ppopup, pMenuState);

        if (fTrackedPopup) {
            xxxDestroyWindow(ppopup->spwndPopupMenu);
        }

    }

}
