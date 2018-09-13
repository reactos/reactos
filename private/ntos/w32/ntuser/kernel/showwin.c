/****************************** Module Header ******************************\
* Module Name: showwin.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Contains the xxxShowWindow API and related functions.
*
* History:
* 10-20-90 darrinm      Created.
* 02-04-91 IanJa        Window handle revalidation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* _ShowWindowAsync
*
* This queues a show window event in another thread's queue. Used mainly from
* within taskmgr, so that taskmgr doesn't hang waiting on hung apps.
*
* 04-23-93 ScottLu      Created.
\***************************************************************************/

BOOL _ShowWindowAsync(PWND pwnd, int cmdShow, UINT uWPFlags)
{

    return PostEventMessage(
            GETPTI(pwnd),
            GETPTI(pwnd)->pq,
            QEVENT_SHOWWINDOW,
            NULL,
            uWPFlags,
            (WPARAM)HWq(pwnd),
            cmdShow | TEST_PUDF(PUDF_ANIMATE));
}

/***************************************************************************\
* xxxShowWindow (API)
*
* This function changes the "state" of a window based upon the cmdShow
* parameter.  The action taken is:
*
* SW_HIDE             0  Hide the window and pass avtivation to someone else
*
* SW_SHOWNORMAL       1  Show a window in its most recent "normal"
* SW_RESTORE             size and position.  This will "restore" a iconic
*                        or zoomed window.  This is compatible with 1.03
*                        SHOW_OPENWINDOW.  This will also activate the window.
*
* SW_SHOWMINIMIZED    2  Show the window as iconic and make it active.
*
* SW_SHOWMAXIMIZED    3  Show the window as maximized and make it active.
*
* SW_SHOWNOACTIVATE   4  Same as SW_SHOWNORMAL except that it doesn't change
*                        the activation (currently active window stays active).
*
* All the above are compatible with 1.03 ShowWindow parameters.  Now here are
* the new ones:
*
* SW_SHOW             5  Show the window in its current state (iconic, etc.)
*                        That is, if the window is iconic when hidden, it will
*                        still be iconic. This will activate the window.
*                        (This is one we don't have today)
*
* SW_MINIMIZE         6  minimize the window, activate the toplevel open window
*
* SW_SHOWMINNOACTIVE  7  show the icon, don't change activation.
*
* SW_SHOWNA           8  Same as SW_SHOW except that it doesn't change
*                        the activation.
*
* SW_SHOWDEFAULT      10 Use value obtained from STARTUPINFO.
*
* History:
* 10-20-90 darrinm      Ported from Win 3.0 sources.
* 04-16-91 JimA         Added SW_SHOWDEFAULT support.
\***************************************************************************/

/*
 * cmdShow now has fAnimate as the lower bit in the upper word.  This puts it in the
 * MINMAX_ANIMATE position for calling MinMaximize.
 */

BOOL xxxShowWindow(
    PWND pwnd,
    DWORD cmdShowAnimate)
{
    BOOL fVisOld, fVisNew;
    UINT swpFlags = SWP_NOMOVE | SWP_NOSIZE;
    PTHREADINFO pti;
    BOOL bFirstMain = FALSE;
    int cmdShow = LOWORD(cmdShowAnimate);

    CheckLock(pwnd);

    fVisOld = TestWF(pwnd, WFVISIBLE);
    pti = PtiCurrent();

    /*
     * See if this is the first "main" top level
     * window being created by this application - if show, assume it
     * is showing with the SW_SHOWDEFAULT command.
     *
     * Checks for:
     * - cmdShow is a "default" show command
     * - we haven't done startupinfo yet (we only use it once)
     * - this is not a child (it is a top level window)
     * - this has a titlebar (indicator of the main window)
     * - it isn't owned (indicator of the main window)
     */
    if ((pti->ppi->usi.dwFlags & STARTF_USESHOWWINDOW) &&
            !TestwndChild(pwnd) &&
            (TestWF(pwnd, WFBORDERMASK) == (BYTE)LOBYTE(WFCAPTION)) &&
            (pwnd->spwndOwner == NULL)) {

        bFirstMain = TRUE;

        switch (cmdShow) {
        case SW_SHOWNORMAL:
        case SW_SHOW:

            /*
             * Then assume default!
             */
            cmdShow = SW_SHOWDEFAULT;
            break;
        }
    }

    /*
     * If this application specified SW_SHOWDEFAULT, then we get the
     * real SW_* command from the application's STARTUPINFO structure
     * (STARTUPINFO is passed to CreateProcess() when this application
     * was launched).
     */
    if (cmdShow == SW_SHOWDEFAULT) {

        /*
         * Call the client to get the SW_* command from the STARTUPINFO
         * for this process.
         */
        if (pti->ppi->usi.dwFlags & STARTF_USESHOWWINDOW) {

            bFirstMain = TRUE;

            cmdShow = pti->ppi->usi.wShowWindow;

            /*
             * The following code was removed in 3.51
             *
             * switch (cmdShow) {
             * case SW_SHOWMINIMIZED:
             * case SW_MINIMIZE:
             *
             *      *
             *      * If the default show was "minimized", then make sure it doesn't
             *      * become active.  Minimized is effectively "background".
             *      *
             *     cmdShow = SW_SHOWMINNOACTIVE;
             *     break;
             * }
             *
             */
        }
    }


    /*
     * This is in case someone said SW_SHOWDEFAULT but has no startupinfo.
     * Or in case cmdShow inside of STARTUPINFO is SW_SHOWDEFAULT.
     */
    if (cmdShow == SW_SHOWDEFAULT)
        cmdShow = SW_SHOWNORMAL;

    /*
     * Turn off startup info.  We turn this off after the first call to
     * ShowWindow.  If we don't apps can be started by progman with
     * the start info being minimized and then be restored and then
     * call ShowWindow(SW_SHOW) and the app would minimize again.
     * Notepad had that problem 2985.
     */
    if (bFirstMain) {
        pti->ppi->usi.dwFlags &=
                ~(STARTF_USESHOWWINDOW | STARTF_USESIZE | STARTF_USEPOSITION);
    }


    /*
     * Take care of all the OLD show commands with columns & iconslot.
     */
    if (cmdShow & 0xFF00) {
        if ((cmdShow & 0xFF80) == (int)0xFF80)
            cmdShow = SW_SHOWMINNOACTIVE;
        else
            cmdShow = SW_SHOW;
    }

    /*
     * Change to new fullscreen if needed and in same desktop
     */
    if ((GetFullScreen(pwnd) != WINDOWED)
            && (pwnd->head.rpdesk == grpdeskRitInput)) {
        if ((cmdShow == SW_SHOWNORMAL) ||
            (cmdShow == SW_RESTORE) ||
            (cmdShow == SW_MAXIMIZE) ||
            (cmdShow == SW_SHOWMAXIMIZED)) {
            cmdShow = SW_SHOWMINIMIZED;

            if (GetFullScreen(pwnd) == FULLSCREENMIN) {
                SetFullScreen(pwnd, FULLSCREEN);
            }

            if (gpqForeground != NULL &&
                gpqForeground->spwndActive == pwnd) {
                xxxMakeWindowForegroundWithState(NULL, 0);
            }
        }
    }

    switch (cmdShow) {
    case SW_SHOWNOACTIVATE:
    case SW_SHOWNORMAL:
    case SW_RESTORE:

        /*
         * If min/max, let xxxMinMaximize() do all the work.
         */
        if (TestWF(pwnd, WFMINIMIZED) || TestWF(pwnd, WFMAXIMIZED)) {
            xxxMinMaximize(pwnd, (UINT)cmdShow, cmdShowAnimate & MINMAX_ANIMATE);
            return fVisOld;

        } else {

            /*
             * Ignore if the window is already visible.
             */
            if (fVisOld) {
                return fVisOld;
            }

            swpFlags |= SWP_SHOWWINDOW;
            if (   cmdShow == SW_SHOWNOACTIVATE) {
                swpFlags |= SWP_NOZORDER;
#ifdef NEVER
                /*
                 * This is what win3.1 does. On NT, since each "queue" has
                 * its own active window, there is often no active window.
                 * In this case, win3.1 turns a SHOWNOACTIVATE into a "SHOW
                 * with activate". Since win3.1 almost always has an active
                 * window, this almost never happens. So on NT, we're not
                 * going to do this check - that way we'll be more compatible
                 * with win3.1 because we'll usally not activate (like win3.1).
                 * With this check, this causes FoxPro 2.5 for Windows to not
                 * properly activate its command window when first coming up.
                 */
                if (pti->pq->spwndActive != NULL)
                    swpFlags |= SWP_NOACTIVATE;
#else
                swpFlags |= SWP_NOACTIVATE;
#endif
            }
        }
        break;

    case SW_FORCEMINIMIZE:
        xxxMinimizeHungWindow(pwnd);
        return fVisOld;

    case SW_SHOWMINNOACTIVE:
    case SW_SHOWMINIMIZED:
    case SW_SHOWMAXIMIZED:
    case SW_MINIMIZE:
        xxxMinMaximize(pwnd, (UINT)cmdShow, cmdShowAnimate & MINMAX_ANIMATE);
        return fVisOld;

    case SW_SHOWNA:
        swpFlags |= SWP_SHOWWINDOW | SWP_NOACTIVATE;


        /*
         * LATER removed this to be compatible with SHOWNOACTIVATE
         * if (pti->pq->spwndActive != NULL)
         *     swpFlags |= SWP_NOACTIVATE;
         */
        break;

    case SW_SHOW:

        /*
         * Don't bother if it is already visible.
         */
        if (fVisOld)
            return fVisOld;

        swpFlags |= SWP_SHOWWINDOW;
        UserAssert(cmdShow != SW_SHOWNOACTIVATE);
        break;

    case SW_HIDE:

        /*
         * Don't bother if it is already hidden.
         */
        if (!fVisOld)
            return fVisOld;

        swpFlags |= SWP_HIDEWINDOW;
        if (pwnd != pti->pq->spwndActive)
            swpFlags |= (SWP_NOACTIVATE | SWP_NOZORDER);
        break;

    default:
        RIPERR0(ERROR_INVALID_SHOWWIN_COMMAND, RIP_VERBOSE, "");
        return fVisOld;
    }

    /*
     * If we're changing from visible to hidden or vise-versa, send
     * WM_SHOWWINDOW.
     */
    fVisNew = !(cmdShow == SW_HIDE);
    if (fVisNew != fVisOld) {
        xxxSendMessage(pwnd, WM_SHOWWINDOW, fVisNew, 0L);
        if (!TestWF(pwnd, WFWIN31COMPAT)) {
            xxxSendMessage(pwnd, WM_SETVISIBLE, fVisNew, 0L);
        }
    }

    if (!TestwndChild(pwnd)) {
        if (TestCF(pwnd, CFSAVEBITS)) {

            /*
             * Activate to prevent discarding saved bits???
             */
            if (cmdShow == SW_SHOW || cmdShow == SW_SHOWNORMAL) {
                xxxActivateWindow(pwnd, AW_USE);
                swpFlags |= SWP_NOZORDER | SWP_NOACTIVATE;
            }
        }
    } else {

        /*
         * Children can't get activation...
         */
        swpFlags |= (SWP_NOACTIVATE | SWP_NOZORDER);
    }

    /*
     * If our parent is hidden, don't bother to call xxxSetWindowPos.
     */
    if (_FChildVisible(pwnd)) {
        xxxSetWindowPos(pwnd, (PWND)NULL, 0, 0, 0, 0, swpFlags);
    } else {
        if (cmdShow == SW_HIDE)
            SetVisible(pwnd, SV_UNSET);
        else
            SetVisible(pwnd, SV_SET);
    }

    /*
     * Send size and move messages AFTER repainting
     */
    if (TestWF(pwnd, WFSENDSIZEMOVE)) {
        ClrWF(pwnd, WFSENDSIZEMOVE);
        if (TestWF(pwnd, WFMINIMIZED)) {
            xxxSendSizeMessage(pwnd, SIZE_MINIMIZED);
        } else if (TestWF(pwnd, WFMAXIMIZED)) {
            xxxSendSizeMessage(pwnd, SIZE_MAXIMIZED);
        } else {
            xxxSendSizeMessage(pwnd, SIZE_RESTORED);
        }

        xxxSendMessage(pwnd, WM_MOVE, 0,
                (pwnd->spwndParent == PWNDDESKTOP(pwnd)) ?
                MAKELONG(pwnd->rcClient.left, pwnd->rcClient.top) :
                MAKELONG(
                    pwnd->rcClient.left - pwnd->spwndParent->rcClient.left,
                    pwnd->rcClient.top - pwnd->spwndParent->rcClient. top));
    }

    /*
     * If hiding and is active-foreground window, activate someone else.
     * If hiding a active window make someone active.
     */
    if (cmdShow == SW_HIDE) {
        if ((pwnd == pti->pq->spwndActive) && (pti->pq == gpqForeground)) {
            xxxActivateWindow(pwnd, AW_SKIP);
        } else {
            xxxCheckFocus(pwnd);
        }
    }

    return fVisOld;
}

/***************************************************************************\
* xxxShowOwnedWindows
*
* xxxShowOwnedWindows is used to hide or show associated popups for the
* following reasons:
*
*     1. Window going iconic
*     2. Popup window being hidden
*     3. Iconic window being opened
*     4. Popup window being shown
*     5. Window being zoomed or unzoomed
*
* For cases 1 and 2, all popups associated with that window are hidden,
* and the WFHIDDENPOPUP bit is set.  This bit is used to differentiate
* between windows hidded by xxxShowOwnedWindows and those hidden by the
* application.
*
* For cases 3 and 4, all popups associated with that window that have the
* WFHIDDENPOPUP bit set are shown.
*
* For case 5, all popups associated with any window BUT the supplied
* window are hidden or shown.  In this case as well, the SW_OTHERZOOM
* or SW_OTHERUNZOOM message is send to all tiled windows to notify them
* that they are being covered or uncovered by the zoomed window.
*
* In all cases, the WM_SHOWWINDOW message is sent to the window to hide or
* show it.
*
* This routine works by simply enumerating all popup windows checking to see
* if the owner of the popup matches the pwndOwner parameter, and taking the
* appropriate action.
*
* We will eventually want 3 separate hide bits: one each for other zoom/unzoom,
* owner iconic/open, owner hide/show.  Right now, there is only one bit, so
* we show windows sometimes when we shouldn't
*
* History:
* 10-20-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

void xxxShowOwnedWindows(
    PWND pwndOwner,
    UINT cmdShow,
    HRGN hrgnHung)
{
    BOOL fShow;
    int cmdZoom;
    HWND *phwnd;
    PBWL pbwl;
    PWND pwnd, pwndTopOwner;
    TL tlpwnd;

    CheckLock(pwndOwner);

    /*
     * Not interested in child windows
     */
    if (TestwndChild(pwndOwner))
        return;

    if ((pbwl = BuildHwndList(PWNDDESKTOP(pwndOwner)->spwndChild, BWL_ENUMLIST, NULL)) == NULL)
        return;

    /*
     * NOTE: The following code assumes the values of SW_* are 1, 2, 3, and 4
     */
    fShow = (cmdShow >= SW_PARENTOPENING);

    cmdZoom = 0;
    if (cmdShow == SW_OTHERZOOM)
        cmdZoom = SIZEZOOMHIDE;

    if (cmdShow == SW_OTHERUNZOOM)
        cmdZoom = SIZEZOOMSHOW;

    /*
     * If zoom/unzoom, then open/close all popups owned by all other
     * windows.  Otherwise, open/close popups owned by pwndOwner.
     */
    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {

        /*
         * Lock the window before we play with it.
         * If the window handle is invalid, skip it
         */
        if ((pwnd = RevalidateHwnd(*phwnd)) == NULL)
            continue;

        /*
         * Kanji windows can't be owned, so skip it.
         */
        if (TestCF(pwnd, CFKANJIWINDOW))
            continue;

        /*
         * If same as window passed in, skip it.
         */
        if (pwnd == pwndOwner)
            continue;

        /*
         * Find ultimate owner of popup, but only go up as far as pwndOwner.
         */
        if ((pwndTopOwner = pwnd->spwndOwner) != NULL) {

            /*
             * The TestwndHI is needed since if it has an icon, pwndOwner
             * is invalid.
             */
            while (!TestwndHI(pwndTopOwner) && pwndTopOwner != pwndOwner &&
                    pwndTopOwner->spwndOwner != NULL)
                pwndTopOwner = pwndTopOwner->spwndOwner;
        }

        /*
         * Zoom/Unzoom case.
         */
        if (cmdZoom != 0) {

            /*
             * If no parent, or parents are the same, skip.
             */
            if (pwndTopOwner == NULL || pwndTopOwner == pwndOwner)
                continue;

            /*
             * If owner is iconic, then this window should stay hidden,
             * UNLESS the minimized window is disabled, in which case we'd
             * better show the window.
             */
            if (   cmdShow == SW_OTHERUNZOOM
                && pwndTopOwner != NULL
                && TestWF(pwndTopOwner, WFMINIMIZED)
                && !TestWF(pwndTopOwner, WFDISABLED)
               )
                continue;
        } else {
            /*
             * Hide/Iconize/Show/Open case.
             */
            /*
             * If parents aren't the same, skip.
             */
            if (pwndTopOwner != pwndOwner)
                continue;
        }

        /*
         * Hide or show if:
         * Showing & this is a hidden popup
         *   OR
         * Hiding & this is a visible window
         */
        if ((fShow && TestWF(pwnd, WFHIDDENPOPUP)) ||
                (!fShow && TestWF(pwnd, WFVISIBLE))) {
            /*
             * For hung minimization, just set the HIDDENPOPUP bit, clear
             * the visible bit and add the window rect to the region to
             * be repainted.
             */
            if (hrgnHung != NULL) {
                HRGN hrgn = GreCreateRectRgnIndirect(&pwnd->rcWindow);
                UnionRgn(hrgnHung, hrgnHung, hrgn);
                GreDeleteObject(hrgn);

                UserAssert(!fShow);

                SetWF(pwnd, WFHIDDENPOPUP);
                SetVisible(pwnd, SV_UNSET);
            } else {
                ThreadLockAlways(pwnd, &tlpwnd);
                xxxSendMessage(pwnd, WM_SHOWWINDOW, fShow, (LONG)cmdShow);
                ThreadUnlock(&tlpwnd);
            }
        }
    }

    /*
     * Free the window list.
     */
    FreeHwndList(pbwl);
}


/***************************************************************************\
* xxxShowOwnedPopups (API)
*
* This routine is accessable to the user.  It will either show or
* hide all popup windows owned by the window handle specified.  If
* fShow if TRUE, all hidden popups will be shown.  If it is FALSE, all
* visible popups will be hidden.
*
* History:
* 10-20-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

BOOL xxxShowOwnedPopups(
    PWND pwndOwner,
    BOOL fShow)
{
    CheckLock(pwndOwner);

    xxxShowOwnedWindows(pwndOwner,
            (UINT)(fShow ? SW_PARENTOPENING : SW_PARENTCLOSING), NULL);
    return TRUE;
}

