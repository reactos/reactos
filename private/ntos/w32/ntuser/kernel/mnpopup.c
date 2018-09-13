/**************************** Module Header ********************************\
* Module Name: mnpopup.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Popup Menu Support
*
* History:
*  10-10-90 JimA    Cleanup.
*  03-18-91 IanJa   Window revalidation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define RECT_ONLEFT     0
#define RECT_ONTOP      1
#define RECT_ONRIGHT    2
#define RECT_ONBOTTOM   3
#define RECT_ORG        4

BOOL TryRect(
        UINT        wRect,
        int         x,
        int         y,
        int         cx,
        int         cy,
        LPRECT      prcExclude,
        LPPOINT     ppt,
        PMONITOR    pMonitor);

/***************************************************************************\
* xxxTrackPopupMenuEx (API)
*
* Process a popup menu
*
* Revalidation Notes:
* o  if pwndOwner is always the owner of the popup menu windows, then we don't
*    really have to revalidate it: when it is destroyed the popup menu windows
*    are destroyed first because it owns them - this is detected in MenuWndProc
*    so we would only have to test pMenuState->fSabotaged.
* o  pMenuState->fSabotaged must be cleared before this top-level routine
*    returns, to be ready for next time menus are processed (unless we are
*    currently inside xxxMenuLoop())
* o  pMenuState->fSabotaged should be FALSE when we enter this routine.
* o  xxxMenuLoop always returns with pMenuState->fSabotaged clear.  Use
*    a UserAssert to verify this.
*
* History:
\***************************************************************************/

int xxxTrackPopupMenuEx(
    PMENU       pMenu,
    UINT        dwFlags,
    int         x,
    int         y,
    PWND        pwndOwner,
    CONST TPMPARAMS *lpTpm)
{
    PMENUSTATE      pMenuState;
    PWND            pwndHierarchy;
    PPOPUPMENU      ppopupMenuHierarchy;
    LONG            sizeHierarchy;
    int             cxPopup,
                    cyPopup;
    BOOL            fSync;
    int             cmd;
    BOOL            fButtonDown;
    TL              tlpwndHierarchy;
    TL              tlpwndT;
    RECT            rcExclude;
    PTHREADINFO     ptiCurrent,
                    ptiOwner;
    PMONITOR        pMonitor;
    POINT           pt;

    CheckLock(pMenu);
    CheckLock(pwndOwner);

    /*
     * Capture the things we care about in case lpTpm goes away.
     */
    if (lpTpm != NULL) {
        if (lpTpm->cbSize != sizeof(TPMPARAMS)) {
            RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "TrackPopupMenuEx: cbSize is invalid");
            return(FALSE);
        }
        rcExclude = lpTpm->rcExclude;
    }

    ptiCurrent = PtiCurrent();
    ptiOwner = GETPTI(pwndOwner);

    /*
     * Win95 compatibility: pwndOwner must be owned by ptiCurrent.
     */
    if (ptiCurrent != ptiOwner) {
        RIPMSG0(RIP_WARNING, "xxxTrackPopupMenuEx: pwndOwner not owned by ptiCurrent");
        return FALSE;
    }

    UserAssert(pMenu != NULL);
    if (ptiCurrent->pMenuState != NULL) {

        if (dwFlags & TPM_RECURSE) {
            /*
             * Only allow recursion if:
             *  -The current menu mode is not about to exit
             *  -Both menus notify the same window
             *  -Only one thread is involved in the current menu mode
             * This will prevent us from getting into some random
             *  scenarios we don't want to deal with
             */
           ppopupMenuHierarchy = ptiCurrent->pMenuState->pGlobalPopupMenu;
           pwndHierarchy = ppopupMenuHierarchy->spwndNotify;
           if (ExitMenuLoop(ptiCurrent->pMenuState, ppopupMenuHierarchy)
                || (pwndHierarchy == NULL)
                || (pwndHierarchy != pwndOwner)
                || (ptiCurrent->pMenuState->ptiMenuStateOwner != GETPTI(pwndHierarchy))) {

               RIPMSG0(RIP_WARNING, "xxxTrackPopupMenuEx: Failing TPM_RECURSE request");
               return FALSE;
           }
           /*
            * Terminate any animation
            */
            MNAnimate(ptiCurrent->pMenuState, FALSE);
           /*
            * Cancel pending show timer if any. ie, the app wants to
            *  pop up a context menu on a popup before we drop it.
            */
           ppopupMenuHierarchy = ((ppopupMenuHierarchy->spwndActivePopup != NULL)
                                  ? ((PMENUWND)(ppopupMenuHierarchy->spwndActivePopup))->ppopupmenu
                                  : NULL);
           if ((ppopupMenuHierarchy != NULL) && ppopupMenuHierarchy->fShowTimer) {

                _KillTimer(ppopupMenuHierarchy->spwndPopupMenu, IDSYS_MNSHOW);
                ppopupMenuHierarchy->fShowTimer = FALSE;
           }
           /*
            * If we're currently on a modal menu, let's unlock the capture
            *  so the recursive menu can get it.
            */
           if (!ptiCurrent->pMenuState->fModelessMenu) {
               ptiCurrent->pq->QF_flags &= ~QF_CAPTURELOCKED;
           }
        } else {
            /*
             * Allow only one guy to have a popup menu up at a time...
             */
            RIPERR0(ERROR_POPUP_ALREADY_ACTIVE, RIP_VERBOSE, "");
            return FALSE;
       }
   }

    // Is button down?

    if (dwFlags & TPM_RIGHTBUTTON)
    {
        fButtonDown = (_GetKeyState(VK_RBUTTON) & 0x8000) != 0;
    } else {
        fButtonDown = (_GetKeyState(VK_LBUTTON) & 0x8000) != 0;
    }

    /*
     * Create the menu window.
     */
    pwndHierarchy = xxxCreateWindowEx(
            WS_EX_TOOLWINDOW | WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE,
            (PLARGE_STRING)MENUCLASS,
            NULL,
            WS_POPUP | WS_BORDER,
            x, y, 100, 100,
            TestMF(pMenu, MNS_MODELESS) ? pwndOwner : NULL,
            NULL, (HANDLE)pwndOwner->hModule,
            NULL,
            WINVER);

    if (pwndHierarchy == NULL) {
        return FALSE;
    }

#ifdef USE_MIRRORING
    if (TestWF(pwndOwner, WEFLAYOUTRTL))
        SetWF(pwndHierarchy, WEFLAYOUTRTL);
#endif

    //
    // Do this so that old apps don't get weird borders on tracked popups due
    // to the app hack used in CreateWindowEx32.
    //
    ClrWF(pwndHierarchy, WFOLDUI);

    ThreadLockAlways(pwndHierarchy, &tlpwndHierarchy);

#ifdef HAVE_MN_GETPPOPUPMENU
    ppopupMenuHierarchy = (PPOPUPMENU)xxxSendMessage(pwndHierarchy,
                                                MN_GETPPOPUPMENU, 0, 0);
#else
    ppopupMenuHierarchy = ((PMENUWND)pwndHierarchy)->ppopupmenu;
#endif


    ppopupMenuHierarchy->fDelayedFree = TRUE;
    Lock(&(ppopupMenuHierarchy->spwndNotify), pwndOwner);
    LockPopupMenu(ppopupMenuHierarchy, &ppopupMenuHierarchy->spmenu, pMenu);
    Lock(&(ppopupMenuHierarchy->spwndActivePopup), pwndHierarchy);
    ppopupMenuHierarchy->ppopupmenuRoot = ppopupMenuHierarchy;
    ppopupMenuHierarchy->fIsTrackPopup  = TRUE;
    ppopupMenuHierarchy->fFirstClick = fButtonDown;
    ppopupMenuHierarchy->fRightButton   = ((dwFlags & TPM_RIGHTBUTTON) != 0);
    if (SYSMET(MENUDROPALIGNMENT) || TestMF(pMenu, MFRTL)) {
       //
       // popup's below this one need to follow the same direction as
       // the other menu's on the desktop.
       //
       ppopupMenuHierarchy->fDroppedLeft = TRUE;
    }
    ppopupMenuHierarchy->fNoNotify      = ((dwFlags & TPM_NONOTIFY) != 0);

    if (fSync = (dwFlags & TPM_RETURNCMD))
        ppopupMenuHierarchy->fSynchronous = TRUE;

    ppopupMenuHierarchy->fIsSysMenu =  ((dwFlags & TPM_SYSMENU) != 0);

    // Set the GlobalPopupMenu variable so that EndMenu works for popupmenus so
    // that WinWart II people can continue to abuse undocumented functions.
    // This is nulled out in MNCancel.
    /*
     * This is actually needed for cleanup in case this thread ends
     *  execution before we can free the popup. (See xxxDestroyThreadInfo)
     *
     * Note that one thread might own pwndOwner and another one might call
     *  TrackPopupMenu (pretty normal if the two threads are attached). So
     *  properly setting (and initializing) pMenuState is a must here.
     */
    pMenuState = xxxMNAllocMenuState(ptiCurrent, ptiOwner, ppopupMenuHierarchy);
    if (pMenuState == NULL) {
        /*
         * Get out. The app never knew we were here so don't notify it
         */
        dwFlags |= TPM_NONOTIFY;
        goto AbortTrackPopupMenuEx;
    }

    /*
     * Notify the app we are entering menu mode.  wParam is 1 since this is a
     * TrackPopupMenu.
     */

    if (!ppopupMenuHierarchy->fNoNotify)
        xxxSendMessage(pwndOwner, WM_ENTERMENULOOP,
            (ppopupMenuHierarchy->fIsSysMenu ? FALSE : TRUE), 0);

    /*
     * Send off the WM_INITMENU, set ourselves up for menu mode etc...
     */
    if (!xxxMNStartMenu(ppopupMenuHierarchy, MOUSEHOLD)) {
        /*
         * ppopupMenuHierarchy has been destroyed already; let's bail
         */
        goto AbortTrackPopupMenuEx;
    }

    /*
     * If drag and drop, register the window as a target.
     */
    if (pMenuState->fDragAndDrop) {
        if (!SUCCEEDED(xxxClientRegisterDragDrop(HW(pwndHierarchy)))) {
            RIPMSG0(RIP_ERROR, "xxxTrackPopupMenuEx: xxxClientRegisterDragDrop failed");
        }
    }

    if (!ppopupMenuHierarchy->fNoNotify) {
        ThreadLock(ppopupMenuHierarchy->spwndNotify, &tlpwndT);
        xxxSendMessage(ppopupMenuHierarchy->spwndNotify, WM_INITMENUPOPUP,
            (WPARAM)PtoHq(pMenu), MAKELONG(0, (ppopupMenuHierarchy->fIsSysMenu ? 1: 0)));
        ThreadUnlock(&tlpwndT);
        ppopupMenuHierarchy->fSendUninit = TRUE;
    }

    /*
     * Size the menu window if needed...
     */
    sizeHierarchy = (LONG)xxxSendMessage(pwndHierarchy, MN_SIZEWINDOW, MNSW_SIZE, 0);

    if (!sizeHierarchy) {

AbortTrackPopupMenuEx:
        if (FWINABLE()) {
            xxxWindowEvent(EVENT_SYSTEM_MENUEND, pwndOwner, OBJID_WINDOW, INDEXID_CONTAINER, 0);
        }
        /*
         * Release the mouse capture we set when we called StartMenuState...
         */
        xxxMNReleaseCapture();

        /* Notify the app we have exited menu mode.  wParam is 1 for real
         * tracked popups, not sys menu.  Check wFlags since ppopupHierarchy
         * will be gone.
         */
        if (!(dwFlags & TPM_NONOTIFY))
            xxxSendMessage(pwndOwner, WM_EXITMENULOOP, ((dwFlags & TPM_SYSMENU) ?
                FALSE : TRUE), 0L);

        /*
         * Make sure we return failure
         */
        fSync = TRUE;
        cmd = FALSE;
        goto CleanupTrackPopupMenuEx;
    }

    if (glinp.dwFlags & LINP_KEYBOARD) {
        pMenuState->fUnderline = TRUE;
        SetMF(pMenu, MFUNDERLINE);
    }

    //
    // Setup popup window dimensions
    //
    cxPopup = LOWORD(sizeHierarchy) + 2*SYSMET(CXFIXEDFRAME);
    cyPopup = HIWORD(sizeHierarchy) + 2*SYSMET(CYFIXEDFRAME);

    //
    // Calculate the monitor BEFORE we adjust the point.  Otherwise, we might
    // move the point offscreen.  In which case, we will end up pinning the
    // popup to the primary display, which is wrong.
    //
    pt.x = x;
    pt.y = y;
    pMonitor = _MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

    //
    // Horizontal alignment
    //
#ifdef USE_MIRRORING
    if (TestWF(pwndOwner, WEFLAYOUTRTL) && !(dwFlags & TPM_CENTERALIGN)) {
        dwFlags = dwFlags ^ TPM_RIGHTALIGN;
    }
#endif
    if (dwFlags & TPM_RIGHTALIGN) {
#if DBG
        if (dwFlags & TPM_CENTERALIGN) {
            RIPMSG0(RIP_WARNING, "TrackPopupMenuEx:  TPM_CENTERALIGN ignored");
        }
#endif // DBG

        x -= cxPopup;
        ppopupMenuHierarchy->iDropDir = PAS_LEFT;
    } else if (dwFlags & TPM_CENTERALIGN) {
        x -= (cxPopup / 2);
    } else {
        ppopupMenuHierarchy->iDropDir = (ppopupMenuHierarchy->fDroppedLeft ? PAS_LEFT : PAS_RIGHT);
    }

    //
    // Vertical alignment
    //
    if (dwFlags & TPM_BOTTOMALIGN) {
#if DBG
        if (dwFlags & TPM_VCENTERALIGN) {
            RIPMSG0(RIP_WARNING, "TrackPopupMenuEx:  TPM_VCENTERALIGN ignored");
        }
#endif // DBG

        y -= cyPopup;
        ppopupMenuHierarchy->iDropDir |= PAS_UP;
    } else if (dwFlags & TPM_VCENTERALIGN) {
        y -= (cyPopup / 2);
    } else {
        ppopupMenuHierarchy->iDropDir |= PAS_DOWN;
    }
    /*
     * If the caller provided an animation direction, use that instead
     */
    if (dwFlags & TPM_ANIMATIONBITS) {
        ppopupMenuHierarchy->iDropDir = ((dwFlags >> TPM_FIRSTANIBITPOS) & (PAS_VERT | PAS_HORZ));
    }
    //
    // Get coords to move to.
    //
    sizeHierarchy = FindBestPos(
            x,
            y,
            cxPopup,
            cyPopup,
            ((lpTpm != NULL) ? &rcExclude : NULL),
            dwFlags,
            ppopupMenuHierarchy,
            pMonitor);

#ifdef USE_MIRRORING
    if (TestWF(pwndOwner, WEFLAYOUTRTL) && (ppopupMenuHierarchy->iDropDir & PAS_HORZ)) {
        ppopupMenuHierarchy->iDropDir ^= PAS_HORZ;
    }
#endif

    /*
     * If we have an animation direction and the caller wants animation,
     *  set the bit to get it going.
     */
    if ((ppopupMenuHierarchy->iDropDir != 0) && !(dwFlags & TPM_NOANIMATION)) {
        ppopupMenuHierarchy->iDropDir |= PAS_OUT;
    }

    /*
     * Show the window. Modeless menus are not topmost and get activated.
     *  Modal menus are topmost but don't get activated.
     */
    PlayEventSound(USER_SOUND_MENUPOPUP);
    xxxSetWindowPos(pwndHierarchy,
            (pMenuState->fModelessMenu ? PWND_TOP : PWND_TOPMOST),
            GET_X_LPARAM(sizeHierarchy), GET_Y_LPARAM(sizeHierarchy), 0, 0,
            SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOOWNERZORDER
            | (pMenuState->fModelessMenu ? 0 : SWP_NOACTIVATE));

    if (FWINABLE()) {
        xxxWindowEvent(EVENT_SYSTEM_MENUPOPUPSTART, pwndHierarchy, OBJID_CLIENT, INDEXID_CONTAINER, 0);
    }
    //
    // We need to return TRUE for compatibility w/ async TrackPopupMenu().
    // It is conceivable that a menu ID could have ID 0, in which case just
    // returning the cmd chosen would return FALSE instead of TRUE.
    //

    //
    // If mouse is in client of popup, act like clicked down
    //
    pMenuState->fButtonDown = fButtonDown;

    cmd = xxxMNLoop(ppopupMenuHierarchy, pMenuState, 0, FALSE);

    /*
     * If this is a modeless menu, return without clenning up because
     *  the menu is up.
     */
    if (pMenuState->fModelessMenu) {
        ThreadUnlock(&tlpwndHierarchy);
        goto ReturnCmdOrTrue;
    }

CleanupTrackPopupMenuEx:

    if (ThreadUnlock(&tlpwndHierarchy)) {
        if (!TestWF(pwndHierarchy, WFDESTROYED)) {
            xxxDestroyWindow(pwndHierarchy);
        }
    }

    if (pMenuState != NULL) {
        xxxMNEndMenuState (TRUE);
    }

    /*
     * Capture must be unlocked if no menu is active.
     */
    UserAssert(!(ptiCurrent->pq->QF_flags & QF_CAPTURELOCKED)
            || ((ptiCurrent->pMenuState != NULL)
                && !ptiCurrent->pMenuState->fModelessMenu));


ReturnCmdOrTrue:
    return(fSync ? cmd : TRUE);
}

/***************************************************************************\
*
* FindBestPos()
*
* Gets best point to move popup menu window to, given exclusion area and
* screen real estate.  Note that for our purposes, we consider center
* alignment to be the same as left/top alignment.
*
* We try to pin the menu to a particular monitor, to avoid having it
* cross.
*
* We try four possibilities if the original position fails.  The order of
* these is determined by the alignment and "try" flags.  Basically, we try
* to move the rectangle out of the exclusion area by sliding it horizontally
* or vertically without going offscreen.  If we can't, then we know that
* sliding it in both dimensions will also fail.  So we use the original
* point, clipping on screen.
*
* Take the example of a top-left justified popup, which should be moved
* horizontally before vertically.  We'll try the original point.  Then
* we'll try to left-justify with the right edge of the exclude rect.  Then
* we'll try to top-justify with the bottom edge of the exclude rect.  Then
* we'll try to right-justify with the left edge of the exclude rect.  Then
* we'll try to bottom-justify with the top edge of the exclude rect.
* Finally, we'll use the original pos.
*
\***************************************************************************/

LONG
FindBestPos(
        int         x,
        int         y,
        int         cx,
        int         cy,
        LPRECT      prcExclude,
        UINT        wFlags,
        PPOPUPMENU  ppopupmenu,
        PMONITOR    pMonitor)
{
    int iRect;
    int iT;
    UINT awRect[4];
    POINT ptT;
    RECT rcExclude;
    //
    // Clip our coords on screen first.  We use the same algorithm to clip
    // as in Win3.1 for dudes with no exclude rect.
    //

    if (prcExclude!=NULL) {
        // Clip exclude rect to monitor!
        CopyRect(&rcExclude, prcExclude);
        IntersectRect(&rcExclude, &rcExclude, &pMonitor->rcMonitor);
    } else {
        SetRect(&rcExclude, x, y, x, y);
    }


    /*
     * Make sure popup fits completely on the screen
     * At least the x,y point will be on the screen.
     */
    if (x + cx > pMonitor->rcMonitor.right) {
        if ((wFlags & TPM_CENTERALIGN)
                || (x - cx < pMonitor->rcMonitor.left)
                || (x >= pMonitor->rcMonitor.right)) {
            x = pMonitor->rcMonitor.right - cx;
        } else {
            x -= cx;
        }
        if (ppopupmenu->iDropDir & PAS_HORZ) {
            COPY_FLAG(ppopupmenu->iDropDir, PAS_LEFT, PAS_HORZ);
        }
    }

    if (x < pMonitor->rcMonitor.left) {
        x += cx;
        if ((wFlags & TPM_CENTERALIGN)
                || (x >= pMonitor->rcMonitor.right)
                || (x < pMonitor->rcMonitor.left)) {
            x = pMonitor->rcMonitor.left;
        }
        if (ppopupmenu->iDropDir & PAS_HORZ) {
            COPY_FLAG(ppopupmenu->iDropDir, PAS_RIGHT, PAS_HORZ);
        }
    }


    if (y + cy > pMonitor->rcMonitor.bottom) {
        if ((wFlags & TPM_VCENTERALIGN)
                || (y - cy < pMonitor->rcMonitor.top)
                || (y >= pMonitor->rcMonitor.bottom)) {
            y = pMonitor->rcMonitor.bottom - cy;
        } else {
            y -= cy;
        }
        if (ppopupmenu->iDropDir & PAS_VERT) {
            COPY_FLAG(ppopupmenu->iDropDir, PAS_UP, PAS_VERT);
        }
    }

    if (y < pMonitor->rcMonitor.top) {
        y += cy;
        if ((wFlags & TPM_VCENTERALIGN)
                || (y >= pMonitor->rcMonitor.bottom)
                || (y < pMonitor->rcMonitor.top)) {
            y = pMonitor->rcMonitor.top;
        }
        if (ppopupmenu->iDropDir & PAS_VERT) {
            COPY_FLAG(ppopupmenu->iDropDir, PAS_DOWN, PAS_VERT);
        }
    }

    //
    // Try first point
    //
    if (TryRect(RECT_ORG, x, y, cx, cy, &rcExclude, &ptT, pMonitor))
        goto FOUND;

    //
    // Sort possibilities.  Get offset of horizontal rects.
    //
    iRect = (wFlags & TPM_VERTICAL) ? 2 : 0;

    //
    // Sort horizontally.  Note that we treat TPM_CENTERALIGN like
    // TPM_LEFTALIGN.
    //
    //
    // If we're right-aligned, try to right-align on left side first.
    // Otherwise, try to left-align on right side first.
    //
    iT = (wFlags & TPM_RIGHTALIGN) ? 0 : 2;

    awRect[0 + iRect] = RECT_ONLEFT + iT;
    awRect[1 + iRect] = RECT_ONRIGHT - iT;

    //
    // Sort vertically.  Note that we treat TPM_VCENTERALIGN like
    // TPM_TOPALIGN.
    //
    // If we're bottom-aligned, try to bottom-align with top of rect
    // first.  Otherwise, try to top-align with bottom of exclusion first.
    //
    iT = (wFlags & TPM_BOTTOMALIGN) ? 0 : 2;

    awRect[2 - iRect] = RECT_ONTOP + iT;
    awRect[3 - iRect] = RECT_ONBOTTOM - iT;

    //
    // Loop through sorted alternatives.  Note that TryRect fails immediately
    // if an exclusion coordinate is too close to screen edge.
    //

    for (iRect = 0; iRect < 4; iRect++) {
        if (TryRect(awRect[iRect], x, y, cx, cy, &rcExclude, &ptT, pMonitor)) {
            switch (awRect[iRect])
            {
                case RECT_ONTOP:
                    ppopupmenu->iDropDir = PAS_UP;
                    break;
                case RECT_ONLEFT:
                    ppopupmenu->iDropDir = PAS_LEFT;
                    break;
                case RECT_ONBOTTOM:
                    ppopupmenu->iDropDir = PAS_DOWN;
                    break;
                case RECT_ONRIGHT:
                    ppopupmenu->iDropDir = PAS_RIGHT;
                    break;
            }

            x = ptT.x;
            y = ptT.y;
            break;
        }
    }

FOUND:
    return MAKELONG(x, y);
}



/***************************************************************************\
*
*  TryRect()
*
*  Tries to fit rect on screen without covering exclusion area.  Returns
*  TRUE if success.
*
\***************************************************************************/

BOOL
TryRect(
        UINT        wRect,
        int         x,
        int         y,
        int         cx,
        int         cy,
        LPRECT      prcExclude,
        LPPOINT     ppt,
        PMONITOR    pMonitor)
{
    RECT rcTry;

    switch (wRect) {
        case RECT_ONRIGHT:
            x = prcExclude->right;
            if (x + cx > pMonitor->rcMonitor.right)
                return FALSE;
            break;

        case RECT_ONBOTTOM:
            y = prcExclude->bottom;
            if (y + cy > pMonitor->rcMonitor.bottom)
                return FALSE;
            break;

        case RECT_ONLEFT:
            x = prcExclude->left - cx;
            if (x < pMonitor->rcMonitor.left)
                return FALSE;
            break;

        case RECT_ONTOP:
            y = prcExclude->top - cy;
            if (y < pMonitor->rcMonitor.top)
                return FALSE;
            break;

        //
        // case RECT_ORG:
        //      NOP;
        //      break;
        //
    }

    ppt->x = x;
    ppt->y = y;

    rcTry.left      = x;
    rcTry.top       = y;
    rcTry.right     = x + cx;
    rcTry.bottom    = y + cy;

    return(!IntersectRect(&rcTry, &rcTry, prcExclude));
}
