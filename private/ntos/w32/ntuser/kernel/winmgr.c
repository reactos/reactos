/****************************** Module Header ******************************\
* Module Name: winmgr.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Core Window Manager APIs and support routines.
*
* History:
* 24-Sep-1990 darrinm   Generated stubs.
* 22-Jan-1991 IanJa     Handle revalidation added
* 19-Feb-1991 JimA      Added enum access checks
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* xxxFlashWindow (API)
*
* New for 5.0: HIWORD(dwFlags) contains the number of times the window should be
*              flashed. LOWORD(dwFlags) contains the FLASHW_ bits.
*
* History:
* 27-Nov-1990 DarrinM   Ported.
* 15-Nov-1997 MCostea   Added dwTimeout and windowing the maximised cmd
\***************************************************************************/
BOOL xxxFlashWindow(
    PWND pwnd,
    DWORD dwFlags,
    DWORD dwTimeout)
{

    BOOL fStatePrev = FALSE;
    BOOL fFlashOn;
    DWORD dwState;

    CheckLock(pwnd);
    /*
     * Get the previous state. If not available (FLASHW_STOP) then
     *  initialize on/off based on frame
     */
    dwState = GetFlashWindowState(pwnd);
    if (dwState == FLASHW_DONE) {
        /*
         * We just need to clean up and to set the activation correctly
         */
        dwState |= FLASHW_KILLTIMER;
        dwFlags = FLASHW_STOP;
        goto flash;
    }
    if (dwState == FLASHW_STOP) {
#if defined(_X86_)
        /*
         * If there is a fullscreen cmd window, switch it to window mode
         * so that the user gets a chance to see the flashing one
         */
        if (gbFullScreen == FULLSCREEN) {
            _PostMessage(gspwndFullScreen, WM_USER + 6, (WPARAM)WINDOWED, (LPARAM)0);
        }
#endif // _X86_
        if (TestWF(pwnd, WFFRAMEON)) {
            dwState = FLASHW_ON | FLASHW_STARTON;
        }
    } else if (dwFlags == FLASHW_TIMERCALL) {
        dwFlags = dwState;
    }
    dwFlags &= FLASHW_CALLERBITS;
    fStatePrev = (dwState & FLASHW_ON);
    /*
     * Later5.0 Gerardob
     * Not sure why we do this check but it used to be here.
     */
    if (pwnd == gspwndAltTab) {
        return fStatePrev;
    }
    /*
     * Check if we're waiting to come to the foreground to stop.
     */
    if (dwState & FLASHW_FLASHNOFG) {
        if (gpqForeground == GETPTI(pwnd)->pq)
            dwFlags = FLASHW_STOP;
    }

flash:
    /*
     * Figure out new state
     */
    if (dwFlags != FLASHW_STOP) {
        fFlashOn =  !fStatePrev;
    } else {
        fFlashOn = (gpqForeground != NULL) && (gpqForeground->spwndActive == pwnd);
    }
    /*
     * Flash'em
     */
    if ((dwFlags == FLASHW_STOP) || (dwFlags & FLASHW_CAPTION)) {
        xxxSendMessage(pwnd, WM_NCACTIVATE, fFlashOn, 0L);
    }
    if ((dwFlags == FLASHW_STOP) || (dwFlags & FLASHW_TRAY)) {
        if (IsTrayWindow(pwnd)) {
            HWND hw = HWq(pwnd);
            BOOL fShellFlash;
            if (dwState & FLASHW_DONE) {
                /*
                 * If the window is not the active one when we're done flashing,
                 * let the tray icon remain activated.  The Shell  is going to
                 * take care to restore it at the when the window gets activated
                 */
                fShellFlash = !fFlashOn;
            } else {
                fShellFlash = (dwFlags == FLASHW_STOP ? FALSE : fFlashOn);
            }
            xxxCallHook(HSHELL_REDRAW, (WPARAM) hw, (LPARAM) fShellFlash, WH_SHELL);
            PostShellHookMessages(fShellFlash? HSHELL_FLASH:HSHELL_REDRAW, (LPARAM)hw);
        }
    }
    /*
     *  If we're to continue, check count, set timer and store
     *   state as appropriate. Otherwise, kill timer and remove
     *   state
     */
    if (dwFlags != FLASHW_STOP) {
        /*
         * If counting, decrement count when we complete a cycle
         */
        if (HIWORD(dwFlags) != 0) {
            dwState |= FLASHW_COUNTING;
            if (!(fFlashOn ^ !!(dwState & FLASHW_STARTON))) {
                dwFlags -= MAKELONG(0,1);
            }
            /*
             * Make sure we have a timer going.
             */
            if (!(dwState & FLASHW_KILLTIMER)) {
                dwFlags |= FLASHW_TIMER;
            }
        }
        /*
         * Set a timer if needed.
         */
        if (dwFlags & FLASHW_TIMER) {
            dwState |= FLASHW_KILLTIMER;
            InternalSetTimer(pwnd,
                             IDSYS_FLASHWND,
                             dwTimeout ? dwTimeout : gpsi->dtCaretBlink,
                             xxxSystemTimerProc,
                             TMRF_SYSTEM);
        }
        /*
         * Remember on/off state, propagate public flags
         *  and count then save the state
         */
        if (dwState & FLASHW_COUNTING &&
            HIWORD(dwFlags) == 0) {
            dwState = FLASHW_DONE;
        }
        else {
            SET_OR_CLEAR_FLAG(dwState, FLASHW_ON, fFlashOn);
            COPY_FLAG(dwState, dwFlags, FLASHW_CALLERBITS & ~FLASHW_TIMER);
        }
        SetFlashWindowState(pwnd, dwState);

    } else {
        /*
         * We're done.
         */
        if (dwState & FLASHW_KILLTIMER) {
            _KillSystemTimer(pwnd, IDSYS_FLASHWND);
        }
        RemoveFlashWindowState(pwnd);
    }

    return fStatePrev;
}

/***************************************************************************\
* xxxEnableWindow (API)
*
*
* History:
* 12-Nov-1990 DarrinM   Ported.
\***************************************************************************/

BOOL xxxEnableWindow(
    PWND pwnd,
    BOOL fEnable)
{
    BOOL fOldState, fChange;

    CheckLock(pwnd);
    UserAssert(IsWinEventNotifyDeferredOK());

    fOldState = TestWF(pwnd, WFDISABLED);

    if (!fEnable) {
        fChange = !TestWF(pwnd, WFDISABLED);

        xxxSendMessage(pwnd, WM_CANCELMODE, 0, 0);

        if (pwnd == PtiCurrent()->pq->spwndFocus) {
                xxxSetFocus(NULL);
        }
        SetWF(pwnd, WFDISABLED);

    } else {
        fChange = TestWF(pwnd, WFDISABLED);
        ClrWF(pwnd, WFDISABLED);
    }

    if (fChange) {
        if (FWINABLE()) {
            xxxWindowEvent(EVENT_OBJECT_STATECHANGE, pwnd, OBJID_WINDOW,
                    INDEXID_CONTAINER, 0);
        }
        xxxSendMessage(pwnd, WM_ENABLE, fEnable, 0L);
    }

    return fOldState;
}

/***************************************************************************\
* xxxDoSend
*
* The following code is REALLY BOGUS!!!! Basically it prevents an
* app from hooking the WM_GET/SETTEXT messages if they're going to
* be called from another app.
*
* History:
* 04-Mar-1992 JimA  Ported from Win 3.1 sources.
\***************************************************************************/

LRESULT xxxDoSend(
    PWND  pwnd,
    UINT  message,
    WPARAM wParam,
    LPARAM lParam)
{
    /*
     * We compare PROCESSINFO sturctures here so multi-threaded
     * app can do what the want.
     */
    if (GETPTI(pwnd)->ppi == PtiCurrent()->ppi) {
        return xxxSendMessage(pwnd, message, wParam, lParam);
    } else {
        return xxxDefWindowProc(pwnd, message, wParam, lParam);
    }
}

/***************************************************************************\
* xxxGetWindowText (API)
*
*
* History:
* 09-Nov-1990 DarrinM   Wrote.
\***************************************************************************/

int xxxGetWindowText(
    PWND   pwnd,
    LPTSTR psz,
    int    cchMax)
{
    LARGE_UNICODE_STRING str;
    UINT nRet, nLen;

    CheckLock(pwnd);

    if (cchMax) {
        /*
         * Initialize string empty, in case xxxSendMessage aborts validation
         * If a bogus value was returned, rely on str.Length
         */
        str.bAnsi         = FALSE;
        str.MaximumLength = cchMax * sizeof(WCHAR);
        str.Buffer        = psz;
        str.Length        = 0;

        *psz = TEXT('\0');

        nRet = (UINT)xxxDoSend(pwnd, WM_GETTEXT, cchMax, (LPARAM)&str);
        nLen = str.Length / sizeof(WCHAR);
        return (nRet > nLen) ? nLen : nRet;
    }

    return 0;
}

/***************************************************************************\
* xxxSetParent (API)
*
* Change a windows parent to a new window.  These steps are taken:
*
* 1. The window is hidden (if visible),
* 2. Its coordinates are mapped into the new parent's space such that the
*    window's screen-relative position is unchanged.
* 3. The window is unlinked from its old parent and relinked to the new.
* 4. xxxSetWindowPos is used to move the window to its new position.
* 5. The window is shown again (if originally visible)
*
* NOTE: If you have a child window and set its parent to be NULL (the
* desktop), the WS_CHILD style isn't removed from the window. This bug has
* been in windows since 2.x. It turns out the apps group depends on this for
* their combo boxes to work.  Basically, you end up with a top level window
* that never gets activated (our activation code blows it off due to the
* WS_CHILD bit).
*
* History:
* 12-Nov-1990 DarrinM   Ported.
* 19-Feb-1991 JimA      Added enum access check
\***************************************************************************/

PWND xxxSetParent(
    PWND pwnd,
    PWND pwndNewParent)
{
    POINT pt;
    BOOL  fVisible;
    PWND  pwndOldParent;
    TL    tlpwndOldParent;
    TL    tlpwndNewParent;
    PVOID pvRet;
    PWND  pwndDesktop;
    PWND  pwndT;
    int flags = SWP_NOZORDER | SWP_NOSIZE;

    CheckLock(pwnd);
    CheckLock(pwndNewParent);

    if (!ValidateParentDepth(pwnd, pwndNewParent)) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "Exceeded nested children limit");
        return NULL;
    }

    pwndDesktop = PWNDDESKTOP(pwnd);

    /*
     * In 1.0x, an app's parent was null, but now it is pwndDesktop.
     * Need to remember to lock pwndNewParent because we're reassigning
     * it here.
     */
    if (pwndNewParent == NULL)
        pwndNewParent = pwndDesktop;

    /*
     * Don't ever change the parent of the desktop.
     */
    if ((pwnd == pwndDesktop) || (pwnd == PWNDMESSAGE(pwnd))) {
        RIPERR0(ERROR_ACCESS_DENIED,
                RIP_WARNING,
                "Access denied: can't change parent of the desktop");

        return NULL;
    }

    /*
     * Don't let the window become its own parent, grandparent, etc.
     */
    for (pwndT = pwndNewParent; pwndT != NULL; pwndT = pwndT->spwndParent) {

        if (pwnd == pwndT) {
            RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING,
                  "Attempting to creating a parent-child relationship loop");
            return NULL;
        }
    }

    /*
     * We still need pwndNewParent across callbacks...  and even though
     * it was passed in, it may have been reassigned above.
     */
    ThreadLock(pwndNewParent, &tlpwndNewParent);

    /*
     * Make the thing disappear from original parent.
     */
    fVisible = xxxShowWindow(pwnd, MAKELONG(SW_HIDE, TEST_PUDF(PUDF_ANIMATE)));

    /*
     * Ensure that the window being changed and the new parent
     * are not in a destroyed state.
     *
     * IMPORTANT: After this check, do not leave the critical section
     * until the window links have been rearranged.
     */
    if (TestWF(pwnd, WFDESTROYED) || TestWF(pwndNewParent, WFDESTROYED)) {
        ThreadUnlock(&tlpwndNewParent);
        return NULL;
    }

    pwndOldParent = pwnd->spwndParent;
    ThreadLock(pwndOldParent, &tlpwndOldParent);

#ifdef USE_MIRRORING
    if (TestWF(pwndOldParent, WEFLAYOUTRTL)) {
        pt.x = pwnd->rcWindow.right;
    } else
#endif
    {
        pt.x = pwnd->rcWindow.left;
    }
    pt.y = pwnd->rcWindow.top;
    _ScreenToClient(pwndOldParent, &pt);

    UnlinkWindow(pwnd, pwndOldParent);
    Lock(&pwnd->spwndParent, pwndNewParent);

    if (pwndNewParent == PWNDDESKTOP(pwnd) && !TestWF(pwnd, WEFTOPMOST)) {

        /*
         * Make sure a child who's owner is topmost inherits the topmost
         * bit. - win31 bug 7568
         */
        if (TestWF(pwnd, WFCHILD) &&
            (pwnd->spwndOwner) &&
            TestWF(pwnd->spwndOwner, WEFTOPMOST)) {

            SetWF(pwnd, WEFTOPMOST);
        }

        /*
         * BACKWARD COMPATIBILITY HACK ALERT
         *
         * All top level windows must be WS_CLIPSIBLINGs bit set.
         * The SDM ComboBox() code calls SetParent() with a listbox
         * window that does not have this set.  This causes problems
         * with InternalInvalidate2() because it does not subtract off
         * the window from the desktop's update region.
         *
         * We must invalidate the DC cache here, too, because if there is
         * a cache entry lying around, its clipping region will be incorrect.
         */
        if ((pwndNewParent == _GetDesktopWindow()) &&
            !TestWF(pwnd, WFCLIPSIBLINGS)) {

            SetWF(pwnd, WFCLIPSIBLINGS);
            zzzInvalidateDCCache(pwnd, IDC_DEFAULT);
        }

        /*
         * This is a top level window but it isn't a topmost window so we
         * have to link it below all topmost windows.
         */
        LinkWindow(pwnd,
                   CalcForegroundInsertAfter(pwnd),
                   pwndNewParent);
    } else {

        /*
         * If this is a child window or if this is a TOPMOST window, we can
         * link at the head of the parent chain.
         */
        LinkWindow(pwnd, NULL, pwndNewParent);
    }

    /*
     * If we're a child window, do any necessary attaching and
     * detaching.
     */
    if (TestwndChild(pwnd)) {

        /*
         * Make sure we're not a WFCHILD window that got SetParent()'ed
         * to the desktop.
         */
        if ((pwnd->spwndParent != PWNDDESKTOP(pwnd)) &&
            GETPTI(pwnd) != GETPTI(pwndOldParent)) {

            zzzAttachThreadInput(GETPTI(pwnd), GETPTI(pwndOldParent), FALSE);
        }

        /*
         * If the new parent window is on a different thread, and also
         * isn't the desktop window, attach ourselves appropriately.
         */
        if (pwndNewParent != PWNDDESKTOP(pwnd) &&
            GETPTI(pwnd) != GETPTI(pwndNewParent)) {

            zzzAttachThreadInput(GETPTI(pwnd), GETPTI(pwndNewParent), TRUE);
        }
    }

    if (pwndNewParent == PWNDMESSAGE(pwnd) || pwndOldParent == PWNDMESSAGE(pwnd))
        flags |= SWP_NOACTIVATE;

    if (FWINABLE()) {
        xxxWindowEvent(EVENT_OBJECT_PARENTCHANGE, pwnd, OBJID_WINDOW,
                INDEXID_CONTAINER, WEF_USEPWNDTHREAD);
    }

    /*
     * We mustn't return an invalid pwndOldParent
     */
    xxxSetWindowPos(pwnd, NULL, pt.x, pt.y, 0, 0, flags);

    if (fVisible) {
        xxxShowWindow(pwnd, MAKELONG(SW_SHOWNORMAL, TEST_PUDF(PUDF_ANIMATE)));
    }

    /*
     * returns pwndOldParent if still valid, else NULL.
     */
    pvRet = ThreadUnlock(&tlpwndOldParent);
    ThreadUnlock(&tlpwndNewParent);

    return pvRet;
}

/***************************************************************************\
* xxxFindWindowEx (API)
*
* Searches for a window among top level windows. The keys used are pszClass,
* (the class name) and/or pszName, (the window title name). Either can be
* NULL.
*
* History:
* 06-Jun-1994 JohnL     Converted xxxFindWindow to xxxFindWindowEx
* 10-Nov-1992 mikeke    Added 16bit and 32bit only flag
* 24-Sep-1990 DarrinM   Generated stubs.
* 02-Jun-1991 ScottLu   Ported from Win3.
* 19-Feb-1991 JimA      Added enum access check
\***************************************************************************/

#define CCHMAXNAME 80

PWND _FindWindowEx(
    PWND   pwndParent,
    PWND   pwndChild,
    LPCWSTR ccxlpszClass,
    LPCWSTR ccxlpszName,
    DWORD  dwType)
{
    /*
     * Note that the Class and Name pointers are client-side addresses.
     */

    PBWL    pbwl;
    HWND    *phwnd;
    PWND    pwnd;
    WORD    atomClass = 0;
    LPCWSTR  lpName;
    BOOL    fTryMessage = FALSE;

    if (ccxlpszClass != NULL) {

        atomClass = FindClassAtom(ccxlpszClass);

        if (atomClass == 0) {
            return NULL;
        }
    }

    /*
     * Setup parent window
     */
    if (!pwndParent) {
        pwndParent = _GetDesktopWindow();
        /*
         * If we are starting from the root and no child window
         * was specified, then check the message window tree too
         * in case we don't find it on the desktop tree.
         */

        if (!pwndChild)
            fTryMessage = TRUE;
    }

TryAgain:
    /*
     * Setup first child
     */
    if (!pwndChild) {
        pwndChild = pwndParent->spwndChild;
    } else {
        if (pwndChild->spwndParent != pwndParent) {
            RIPMSG0(RIP_WARNING,
                 "FindWindowEx: Child window doesn't have proper parent");
            return NULL;
        }

        pwndChild = pwndChild->spwndNext;
    }

    /*
     * Generate a list of top level windows.
     */
    if ((pbwl = BuildHwndList(pwndChild, BWL_ENUMLIST, NULL)) == NULL) {
        return NULL;
    }

    /*
     * Set pwnd to NULL in case the window list is empty.
     */
    pwnd = NULL;

    try {
        for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {

            /*
             * Validate this hwnd since we left the critsec earlier (below
             * in the loop we send a message!
             */
            if ((pwnd = RevalidateHwnd(*phwnd)) == NULL)
                continue;

            /*
             * make sure this window is of the right type
             */
            if (dwType != FW_BOTH) {
                if (((dwType == FW_16BIT) && !(GETPTI(pwnd)->TIF_flags & TIF_16BIT)) ||
                    ((dwType == FW_32BIT) && (GETPTI(pwnd)->TIF_flags & TIF_16BIT)))
                    continue;
            }

            /*
             * If the class is specified and doesn't match, skip this window
             */
            if (!atomClass || (atomClass == pwnd->pcls->atomClassName)) {
                if (!ccxlpszName)
                    break;

                if (pwnd->strName.Length) {
                    lpName = pwnd->strName.Buffer;
                } else {
                    lpName = szNull;
                }

                /*
                 * Is the text the same? If so, return with this window!
                 */
                if (_wcsicmp(ccxlpszName, lpName) == 0)
                    break;
            }

            /*
             * The window did not match.
             */
            pwnd = NULL;
        }
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        pwnd = NULL;
    }

    FreeHwndList(pbwl);

    if (!pwnd && fTryMessage) {
        fTryMessage = FALSE;
        pwndParent = _GetMessageWindow();
        pwndChild = NULL;
        goto TryAgain;
    }

    return ((*phwnd == (HWND)1) ? NULL : pwnd);
}

/***************************************************************************\
* UpdateCheckpoint
*
* Checkpoints the current window size/position/state and returns a pointer
* to the structure.
*
* History:
\***************************************************************************/

PCHECKPOINT UpdateCheckpoint(
    PWND pwnd)
{
    RECT rc;

    GetRect(pwnd, &rc, GRECT_WINDOW | GRECT_PARENTCOORDS);
    return CkptRestore(pwnd, &rc);
}

/***************************************************************************\
* GetWindowPlacement
*
* History:
* 02-Mar-1992 MikeKe    From Win 3.1
\***************************************************************************/

BOOL _GetWindowPlacement(
    PWND             pwnd,
    PWINDOWPLACEMENT pwp)
{
    CHECKPOINT * pcp;

    /*
     * this will set the normal or the minimize point in the checkpoint,
     * so that all elements will be up to date.
     */
    pcp = UpdateCheckpoint(pwnd);

    if (!pcp)
        return FALSE;

    if (TestWF(pwnd, WFMINIMIZED)) {
        pwp->showCmd = SW_SHOWMINIMIZED;
    } else if (TestWF(pwnd, WFMAXIMIZED)) {
        pwp->showCmd = SW_SHOWMAXIMIZED;
    } else {
        pwp->showCmd = SW_SHOWNORMAL;
    }

    CopyRect(&pwp->rcNormalPosition, &pcp->rcNormal);

    if (pcp->fMinInitialized) {
        pwp->ptMinPosition = pcp->ptMin;
    } else {
        pwp->ptMinPosition.x = pwp->ptMinPosition.y = -1;
    }

    /*
     * We never ever save the position of "normal" maximized windows.  Other
     * wise, when the size border changes dimensions, the max pos would be
     * invalid, and you would never be able to reset it.
     */
    if (pcp->fMaxInitialized && !TestWF(pwnd, WFREALLYMAXIMIZABLE)) {
        pwp->ptMaxPosition = pcp->ptMax;
    } else {
        pwp->ptMaxPosition.x = pwp->ptMaxPosition.y = -1;
    }

    if ((pwnd->spwndParent == PWNDDESKTOP(pwnd)) &&
            !TestWF(pwnd, WEFTOOLWINDOW)) {

        PMONITOR    pMonitor;

        pMonitor = _MonitorFromRect(&pwp->rcNormalPosition, MONITOR_DEFAULTTOPRIMARY);

        /*
         * Convert min, normal positions to be relative to the working area.
         * The max pos already is (always is saved that way).
         *
         * working area, except for maximized position, which is always
         * working area relative.
         */
        if (pcp->fMinInitialized) {
            pwp->ptMinPosition.x -= (pMonitor->rcWork.left - pMonitor->rcMonitor.left);
            pwp->ptMinPosition.y -= (pMonitor->rcWork.top - pMonitor->rcMonitor.top);
        }

        OffsetRect(&pwp->rcNormalPosition,
            pMonitor->rcMonitor.left - pMonitor->rcWork.left,
            pMonitor->rcMonitor.top - pMonitor->rcWork.top);
    }

    pwp->flags = 0;

    /*
     * B#3276
     * Don't allow WPF_SETMINPOSITION on top-level windows.
     */
    if (TestwndChild(pwnd) && pcp->fDragged)
        pwp->flags |= WPF_SETMINPOSITION;

    if (pcp->fWasMaximizedBeforeMinimized || TestWF(pwnd, WFMAXIMIZED))
        pwp->flags |= WPF_RESTORETOMAXIMIZED;

    pwp->length = sizeof(WINDOWPLACEMENT);

    return TRUE;
}

/***************************************************************************\
* CheckPlacementBounds
*
* History:
* 02-Mar-1992 MikeKe    From Win 3.1
\***************************************************************************/

VOID CheckPlacementBounds(
    LPRECT      lprc,
    LPPOINT     ptMin,
    LPPOINT     ptMax,
    PMONITOR    pMonitor)
{
    int xIcon;
    int yIcon;
    int sTop;
    int sBottom;
    int sLeft;
    int sRight;

    /*
     * Check Normal Window Placement
     */

    /*
     * Possible values for these sign variables are :
     * -1 : less than the minimum for that dimension
     *  0 : within the range for that dimension
     *  1 : more than the maximum for that dimension
     */
    sTop = (lprc->top < pMonitor->rcWork.top) ? -1 :
        ((lprc->top > pMonitor->rcWork.bottom) ? 1 : 0);

    sBottom = (lprc->bottom < pMonitor->rcWork.top) ? -1 :
        ((lprc->bottom > pMonitor->rcWork.bottom) ? 1 : 0);

    sLeft = (lprc->left < pMonitor->rcWork.left) ? -1 :
        ((lprc->left > pMonitor->rcWork.right) ? 1 : 0);

    sRight = (lprc->right < pMonitor->rcWork.left) ? -1 :
        ((lprc->right > pMonitor->rcWork.right) ? 1 : 0);

    if ((sTop * sBottom > 0) || (sLeft * sRight > 0)) {

        /*
         * Window is TOTALLY outside monitor bounds.  The resolution and/or
         * configuration of monitors probably changed since the last time
         * we ran this app.
         *
         * Slide it FULLY onto the monitor at the nearest position.
         */
        int size;

        if (sTop < 0) {
            lprc->bottom -= lprc->top;
            lprc->top     = pMonitor->rcWork.top;
        } else if (sBottom > 0) {
            size = lprc->bottom - lprc->top;
            lprc->top    = max(pMonitor->rcWork.bottom - size, pMonitor->rcWork.top);
            lprc->bottom = lprc->top + size;
        }

        if (sLeft < 0) {
            lprc->right -= lprc->left;
            lprc->left   = pMonitor->rcWork.left;
        } else if (sRight > 0) {
            size = lprc->right - lprc->left;
            lprc->left  = max(pMonitor->rcWork.right - size, pMonitor->rcWork.left);
            lprc->right = lprc->left + size;
        }
    }

    /*
     * Check Iconic Window Placement
     */
    if (ptMin->x != -1) {

        xIcon = SYSMET(CXMINSPACING);
        yIcon = SYSMET(CYMINSPACING);

        sTop = (ptMin->y < pMonitor->rcWork.top) ? -1 :
            ((ptMin->y > pMonitor->rcWork.bottom) ? 1 : 0);

        sBottom = (ptMin->y + yIcon < pMonitor->rcWork.top) ? -1 :
            ((ptMin->y + yIcon > pMonitor->rcWork.bottom) ? 1 : 0);

        sLeft = (ptMin->x < pMonitor->rcWork.left) ? -1 :
            ((ptMin->x > pMonitor->rcWork.right) ? 1 : 0);

        sRight = (ptMin->x + xIcon < pMonitor->rcWork.left) ? -1 :
            ((ptMin->x + xIcon > pMonitor->rcWork.right) ? 1 : 0);

        /*
         * Icon is TOTALLY outside monitor bounds; repark it.
         */
        if ((sTop * sBottom > 0) || (sLeft * sRight > 0))
            ptMin->x = ptMin->y = -1;
    }

    /*
     * Check Maximized Window Placement
     */
    if (ptMax->x != -1 &&
        (ptMax->x + pMonitor->rcWork.left >= pMonitor->rcWork.right ||
         ptMax->y + pMonitor->rcWork.top >= pMonitor->rcWork.bottom)) {

        /*
         * window is TOTALLY below beyond maximum dimensions; zero the
         * position so that the window will at least be clipped to the
         * monitor.
         */
        ptMax->x = 0;
        ptMax->y = 0;
    }
}

/***************************************************************************\
* WPUpdateCheckPointSettings
*
* History:
* 02/23/98  GerardoB    Extracted from xxxSetWindowPlacement
\***************************************************************************/
void WPUpdateCheckPointSettings (PWND pwnd, UINT uWPFlags)
{
    CHECKPOINT *    pcp;

    UserAssert(TestWF(pwnd, WFMINIMIZED));
    if (pcp = UpdateCheckpoint(pwnd)) {

        /*
         * Save settings in the checkpoint struct
         */
        if (uWPFlags & WPF_SETMINPOSITION)
            pcp->fDragged = TRUE;

        if (uWPFlags & WPF_RESTORETOMAXIMIZED) {
            pcp->fWasMaximizedBeforeMinimized = TRUE;
        } else {
            pcp->fWasMaximizedBeforeMinimized = FALSE;
        }
    }
}
/***************************************************************************\
* xxxSetWindowPlacement
*
* History:
* 02-Mar-1992 MikeKe    From Win 3.1
\***************************************************************************/

BOOL xxxSetWindowPlacement(
    PWND             pwnd,
    PWINDOWPLACEMENT pwp)
{
    CHECKPOINT *    pcp;
    PMONITOR        pMonitor;
    RECT            rc;
    POINT           ptMin;
    POINT           ptMax;
    BOOL            fMin;
    BOOL            fMax;
    UINT            uSWPFlags;
    BOOL            fRealAsync;

    CheckLock(pwnd);

    CopyRect(&rc, &pwp->rcNormalPosition);
    if (pwnd->spwndParent == PWNDDESKTOP(pwnd)) {
        pMonitor = _MonitorFromRect(&rc, MONITOR_DEFAULTTOPRIMARY);
    }

    ptMin = pwp->ptMinPosition;
    fMin  = ((ptMin.x != -1) && (ptMin.y != -1));

    ptMax = pwp->ptMaxPosition;
    fMax  = ((ptMax.x != -1) && (ptMax.y != -1));

    /*
     * Convert back to working rectangle coordinates
     */
    if (    pwnd->spwndParent == PWNDDESKTOP(pwnd) &&
            !TestWF(pwnd, WEFTOOLWINDOW)) {

        OffsetRect(
                &rc,
                pMonitor->rcWork.left - pMonitor->rcMonitor.left,
                pMonitor->rcWork.top - pMonitor->rcMonitor.top);

        if (fMin) {
            ptMin.x += pMonitor->rcWork.left - pMonitor->rcMonitor.left;
            ptMin.y += pMonitor->rcWork.top - pMonitor->rcMonitor.top;
        }

        CheckPlacementBounds(&rc, &ptMin, &ptMax, pMonitor);
    }

    if (pcp = UpdateCheckpoint(pwnd)) {

        /*
         * Save settings in the checkpoint struct
         */
        CopyRect(&pcp->rcNormal, &rc);

        pcp->ptMin                        = ptMin;
        pcp->fMinInitialized              = fMin;
        pcp->fDragged                     = (pwp->flags & WPF_SETMINPOSITION) ?
                                                TRUE : FALSE;
        pcp->ptMax                        = ptMax;
        pcp->fMaxInitialized              = fMax;
        pcp->fWasMaximizedBeforeMinimized = FALSE;
    }

    /*
     * WPF_ASYNCWINDOWPLACEMENT new for NT5.
     */
    uSWPFlags = SWP_NOZORDER | SWP_NOACTIVATE
                | ((pwp->flags & WPF_ASYNCWINDOWPLACEMENT) ? SWP_ASYNCWINDOWPOS : 0);

    if (TestWF(pwnd, WFMINIMIZED)) {

        if ((!pcp || pcp->fDragged) && fMin) {
            xxxSetWindowPos(pwnd,
                            PWND_TOP,
                            ptMin.x,
                            ptMin.y,
                            0,
                            0,
                            SWP_NOSIZE | uSWPFlags);
        }

    } else if (TestWF(pwnd, WFMAXIMIZED)) {

        if (pcp != NULL) {
            if (TestWF(pwnd, WFREALLYMAXIMIZABLE))
                pcp->fMaxInitialized = FALSE;

            if (pcp->fMaxInitialized) {
                if (pwnd->spwndParent == PWNDDESKTOP(pwnd)) {
                    ptMax.x += pMonitor->rcWork.left;
                    ptMax.y += pMonitor->rcWork.top;
                }

                xxxSetWindowPos(pwnd,
                                PWND_TOP,
                                ptMax.x,
                                ptMax.y,
                                0,
                                0,
                                SWP_NOSIZE | uSWPFlags);
            }
        }


    } else {

        xxxSetWindowPos(pwnd,
                        PWND_TOP,
                        rc.left,
                        rc.top,
                        rc.right - rc.left,
                        rc.bottom - rc.top,
                        uSWPFlags);
    }
    /*
     * xxxSetWindowPos is only assync when the window's thread is on a
     *  different queue than the current thread's. See AsyncWindowPos.
     */
    fRealAsync = (pwp->flags & WPF_ASYNCWINDOWPLACEMENT)
                    && (GETPTI(pwnd)->pq != PtiCurrent()->pq);

    if (fRealAsync) {
        _ShowWindowAsync(pwnd, pwp->showCmd, pwp->flags);
    } else {
        xxxShowWindow(pwnd, MAKELONG(pwp->showCmd, TEST_PUDF(PUDF_ANIMATE)));
    }

    if (TestWF(pwnd, WFMINIMIZED) && !fRealAsync) {
        WPUpdateCheckPointSettings(pwnd, pwp->flags);
    }

    return TRUE;
}

/***************************************************************************\
* xxxSetInternalWindowPos
*
* Sets a window to the size, position and state it was most recently
* in.  Side effect (possibly bug): shows and activates the window as well.
*
* History:
* 28-Mar-1991 DavidPe   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL xxxSetInternalWindowPos(
    PWND    pwnd,
    UINT    cmdShow,
    LPRECT  lprcWin,
    LPPOINT lpptMin)
{
    CHECKPOINT *    pcp;
    PMONITOR        pMonitor;

    CheckLock(pwnd);

    if ((pcp = UpdateCheckpoint(pwnd)) == NULL) {
        return FALSE;
    }

    if (lprcWin) {

        pcp->rcNormal = *lprcWin;
        if (pwnd->spwndParent == PWNDDESKTOP(pwnd)) {
            pMonitor = _MonitorFromRect(lprcWin, MONITOR_DEFAULTTOPRIMARY);
            OffsetRect(
                    &pcp->rcNormal,
                    pMonitor->rcWork.left - pMonitor->rcMonitor.left,
                    pMonitor->rcWork.top - pMonitor->rcMonitor.top);
        }
    }

    if (lpptMin && (lpptMin->x != -1)) {

        pcp->ptMin = *lpptMin;
        if (pwnd->spwndParent == PWNDDESKTOP(pwnd)) {
            pMonitor = _MonitorFromRect(&pcp->rcNormal, MONITOR_DEFAULTTOPRIMARY);
            pcp->ptMin.x += pMonitor->rcWork.left - pMonitor->rcMonitor.left;
            pcp->ptMin.y += pMonitor->rcWork.top - pMonitor->rcMonitor.top;
        }

        pcp->fDragged = TRUE;
        pcp->fMinInitialized = TRUE;

    } else {
        pcp->fMinInitialized = FALSE;
        pcp->fDragged = FALSE;
    }

    if (TestWF(pwnd, WFMINIMIZED)) {

        /*
         * need to move the icon
         */
        if (pcp->fMinInitialized) {
            xxxSetWindowPos(pwnd,
                            PWND_TOP,
                            pcp->ptMin.x,
                            pcp->ptMin.y,
                            0,
                            0,
                            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

    } else if (!TestWF(pwnd, WFMAXIMIZED) && lprcWin) {
        /*
         * need to set the size and the position
         */
        xxxSetWindowPos(pwnd,
                        NULL,
                        lprcWin->left,
                        lprcWin->top,
                        lprcWin->right - lprcWin->left,
                        lprcWin->bottom - lprcWin->top,
                        SWP_NOZORDER);
    }

    xxxShowWindow(pwnd, MAKELONG(cmdShow, TEST_PUDF(PUDF_ANIMATE)));

    return TRUE;
}

/***************************************************************************\
* _GetDesktopWindow (API)
*
* History:
* 07-Nov-1990 DarrinM   Implemented.
\***************************************************************************/

PWND _GetDesktopWindow(VOID)
{
    PTHREADINFO  pti = PtiCurrent();
    PDESKTOPINFO pdi;

    if (pti == NULL)
        return NULL;

    pdi = pti->pDeskInfo;

    return pdi == NULL ? NULL : pdi->spwnd;
}

/***************************************************************************\
* _GetDesktopWindow (API)
*
* History:
* 07-Nov-1990 DarrinM   Implemented.
\***************************************************************************/

PWND _GetMessageWindow(VOID)
{
    PTHREADINFO  pti = PtiCurrent();
    PDESKTOP pdi;

    if (pti == NULL)
        return NULL;

    pdi = pti->rpdesk;

    return pdi == NULL ? NULL : pdi->spwndMessage;
}

/**************************************************************************\
* TestWindowProcess
*
* History:
* 14-Nov-1994 JimA      Created.
\**************************************************************************/

BOOL TestWindowProcess(
    PWND pwnd)
{
    return (PpiCurrent() == GETPTI(pwnd)->ppi);
}

/***************************************************************************\
* ValidateDepth
*
* The function conveniently simulates recursion by utilizing the fact
* that from any sibling in the Next chain we can correctly get to the
* parent window and that two siblings in the Next chain cannot have
* different parents.
*
* 12-Mar-1997   vadimg      created
\***************************************************************************/

#define NESTED_WINDOW_LIMIT 100

BOOL ValidateParentDepth(PWND pwnd, PWND pwndParent)
{
    UINT cDepth = 1, cDepthMax;
    PWND pwndStop;

    /*
     * Calculate the depth of the parent chain.
     */
    while (pwndParent != NULL) {
        pwndParent = pwndParent->spwndParent;
        cDepth++;
    }

    cDepthMax = cDepth;

    /*
     * When pwnd is NULL, it means that we want to add one more
     * level to the existing depth of pwndParent.
     */
    if (pwnd == NULL || pwnd->spwndChild == NULL) {
        goto Exit;
    } else {
        pwndStop = pwnd->spwndParent;
    }

Restart:
    if (pwnd->spwndChild != NULL) {
        pwnd = pwnd->spwndChild;
        cDepth++;
    } else if (pwnd->spwndNext != NULL) {
        pwnd = pwnd->spwndNext;
    } else {
        if (cDepth > cDepthMax) {
            cDepthMax = cDepth;
        }

        /*
         * Find a parent with siblings and recurse on them. Terminate
         * when we reach the parent of the original pwnd.
         */
        do {
            pwnd = pwnd->spwndParent;
            cDepth--;

            if (pwnd == pwndStop)
                goto Exit;

        } while (pwnd->spwndNext == NULL);

        pwnd = pwnd->spwndNext;
    }
    goto Restart;

Exit:
    return (cDepthMax <= NESTED_WINDOW_LIMIT);
}

/***************************************************************************\
* ValidateOwnerDepth
*
* pwndOwner is the new intended owner, we basically add 1 to the current
* nested owner chain depth. We assume that the actual window does not have
* any ownees. In reality, it can through SetWindowLong, but finding the
* maximum depth of the ownee chain is really tricky - just look in swp.c.
*
* 12-Mar-1997   vadimg      created
\***************************************************************************/

BOOL ValidateOwnerDepth(PWND pwnd, PWND pwndOwner)
{
    UINT cDepth = 1;

    while (pwndOwner != NULL) {

        /*
         * Do not allow loops in the owner chain.
         */
        if (pwndOwner == pwnd) {
            return FALSE;
        }

        pwndOwner = pwndOwner->spwndOwner;
        cDepth++;
    }

    return (cDepth <= NESTED_WINDOW_LIMIT);
}
