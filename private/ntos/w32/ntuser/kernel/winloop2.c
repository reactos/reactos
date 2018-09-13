/*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*/

#include "precomp.h"
#pragma hdrstop

// ----------------------------------------------------------------------------
//
//  IsVSlick() -
//
//  TRUE if window is positioned at +100,+100 from bottom right of screen --
//  probably VSlick -- which has two Tray Windows, one is unowned but off the
//  screen....we want the owned one since its on the screen
//
// ----------------------------------------------------------------------------
BOOL IsVSlick(PWND pwnd)
{
    if (gpDispInfo->cMonitors == 1 &&
        ((unsigned) pwnd->rcWindow.left > (unsigned) gpDispInfo->rcScreen.right ) &&
        ((unsigned) pwnd->rcWindow.top  > (unsigned) gpDispInfo->rcScreen.bottom) &&
        (pwnd->rcWindow.top == (gpDispInfo->rcScreen.bottom+100)) &&
        (pwnd->rcWindow.left == (gpDispInfo->rcScreen.right+100)))
    {
        // MUST BE THE ONE AND ONLY V-SLICK
        return(TRUE);
    }

    return(FALSE);
}

// ----------------------------------------------------------------------------
//
//  Is31TrayWindow() -
//
//  extra grilling required for 3.1 and earlier apps before letting 'em in the
//  tray -- trust me, you DON'T want to change this code. -- JEFFBOG 11/10/94
//
// ----------------------------------------------------------------------------
BOOL Is31TrayWindow(PWND pwnd)
{
    PWND pwnd2;

    if (!(pwnd2 = pwnd->spwndOwner))
        return (!IsVSlick(pwnd)); // unowned -- do we want you?

    if (TestWF(pwnd2, WEFTOOLWINDOW))
        return(FALSE); // owned by a tool window -- we don't want

    return((FHas31TrayStyles(pwnd2) ? (IsVSlick(pwnd2)) : TRUE));
}


// ----------------------------------------------------------------------------
//
//  IsTrayWindow() -
//
//  TRUE if the window passes all the necessary checks -- making it a window
//  that should appear in the tray.
//
// ----------------------------------------------------------------------------
BOOL IsTrayWindow(PWND pwnd)
{
    if ((pwnd==NULL) || !(FDoTray() && (FCallHookTray() || FPostTray(pwnd->head.rpdesk))) ||
            !FTopLevel(pwnd))
        return(FALSE);

    // Check for WS_EX_APPWINDOW or WS_EX_TOOLWINDOW "overriding" bits
    if (TestWF(pwnd, WEFAPPWINDOW))
        return(TRUE);

    if (TestWF(pwnd, WEFTOOLWINDOW))
        return(FALSE);

    if (TestWF(pwnd, WEFNOACTIVATE)) {
        return FALSE;
    }

    if (TestWF(pwnd, WFWIN40COMPAT)) {
        if (pwnd->spwndOwner == NULL)
            return(TRUE);
        if (TestWF(pwnd->spwndOwner, WFWIN40COMPAT))
            return(FALSE);
        // if this window is owned by a 3.1 window, check it like a 3.1 window
    }

    if (!FHas31TrayStyles(pwnd))
        return(FALSE);

    return(Is31TrayWindow(pwnd));
}

/***************************************************************************\
* xxxSetTrayWindow
*
* History:
* 11-Dec-1996 adams     Created.
\***************************************************************************/

void xxxSetTrayWindow(PDESKTOP pdesk, PWND pwnd, PMONITOR pMonitor)
{
    HWND hwnd;

    CheckLock(pMonitor);

    if (pwnd == STW_SAME) {
        pwnd = pdesk->spwndTray;
        hwnd = PtoH(pwnd);
    } else {
        CheckLock(pwnd);
        hwnd = PtoH(pwnd);
        Lock(&(pdesk->spwndTray), pwnd);
    }

    if (!pMonitor) {
        if (pwnd) {
            pMonitor = _MonitorFromWindow(pwnd, MONITOR_DEFAULTTOPRIMARY);
        } else {
            pMonitor = GetPrimaryMonitor();
        }
    }

    if ( FPostTray(pdesk)) {
        PostShellHookMessages(
                pMonitor->cFullScreen ?
                        HSHELL_RUDEAPPACTIVATED : HSHELL_WINDOWACTIVATED,
                (LPARAM) hwnd);
    }

    if ( FCallHookTray() ) {
        xxxCallHook(
                HSHELL_WINDOWACTIVATED,
                (WPARAM) hwnd,
                (pMonitor->cFullScreen ? 1 : 0),
                WH_SHELL);
    }
}



/***************************************************************************\
* xxxAddFullScreen
*
* Adds an app to the fullscreen list and moves the tray if it is
* the first fullscreen app.
*
* History:
* 27-Feb-1997 adams     Commented.
\***************************************************************************/

BOOL xxxAddFullScreen(PWND pwnd, PMONITOR pMonitor)
{
    BOOL    fYielded;

    PDESKTOP pdesk = pwnd->head.rpdesk;

    CheckLock(pwnd);
    CheckLock(pMonitor);

    if (pdesk == NULL)
        return FALSE;

    fYielded = FALSE;
    if (!TestWF(pwnd, WFFULLSCREEN) && FCallTray(pdesk))
    {
        SetWF(pwnd, WFFULLSCREEN);

        if (pMonitor->cFullScreen++ == 0) {
            xxxSetTrayWindow(pdesk, STW_SAME, pMonitor);
            fYielded = TRUE;
        }

        pwnd = pwnd->spwndOwner;
        if (    pwnd &&
                !TestWF(pwnd, WFCHILD) &&
                pwnd->rcWindow.right == 0 &&
                pwnd->rcWindow.left == 0 &&
                !TestWF(pwnd, WFVISIBLE)) {

            TL tlpwnd;
            ThreadLock(pwnd, &tlpwnd);
            if (xxxAddFullScreen(pwnd, pMonitor)) {
                fYielded = TRUE;
            }

            ThreadUnlock(&tlpwnd);
        }
    }

    return fYielded;
}



/***************************************************************************\
* xxxRemoveFullScreen
*
* Adds an app to the fullscreen list and moves the tray if there
* are no more fullscreen apps.
*
* History:
* 27-Feb-1997 adams     Commented.
\***************************************************************************/

BOOL xxxRemoveFullScreen(PWND pwnd, PMONITOR pMonitor)
{
    PDESKTOP pdesk = pwnd->head.rpdesk;
    BOOL    fYielded;

    CheckLock(pwnd);
    CheckLock(pMonitor);

    if (pdesk == NULL)
        return FALSE;

    fYielded = FALSE;
    if (TestWF(pwnd, WFFULLSCREEN) && FCallTray(pdesk)) {
        ClrWF(pwnd, WFFULLSCREEN);

        if (--pMonitor->cFullScreen == 0) {
            xxxSetTrayWindow(pdesk, STW_SAME, pMonitor);
            fYielded = TRUE;
        }

        /*
         * (adams): Remove this assertion temporarily while I work on
         * a fix for the problem.
         *
         * UserAssert(pMonitor->cFullScreen >= 0);
         */
    }

    return fYielded;
}
