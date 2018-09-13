/****************************** Module Header ******************************\
* Module Name: fullscr.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains all the fullscreen code for the USERSRV.DLL.
*
* History:
* 12-Dec-1991 mikeke   Created
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* We can only have one fullscreen window at a time so this information can
* be store globally
*
* We partially use busy waiting to set the state of the hardware.
* The problem is that while we are in the middle of a fullscreen switch,
* we leave the critical section !  So someone else could come in and change
* the state of the fullscreen stuff.
* In order to keep the system from getting confused about the state of the
* device, we actually "post" the request.
*
* What we do with external requests for switching, is that we will do busy
* waiting on these state variables.
* So an app won't be able to request a fullscreen switch while one is under
* way.  This is a way to make the system completely reentrant for state
* switches.
*
* The state variables themselves can only be touched while owning the
* critical section.  We are guaranteed that we will not busy wait forever
* since the switch operations (although long) will eventually finish.
*
* 20-Mar-1996 andreva  Created
\***************************************************************************/

LONG TraceFullscreenSwitch;

#define NOSWITCHER ((HANDLE)-1)

HANDLE idSwitcher = NOSWITCHER;
BOOL   fRedoFullScreenSwitch;
BOOL   fGdiEnabled = TRUE;
POINT  gptCursorFullScreen;

extern ULONG InitSafeBootMode;  // imported from NTOS (init.c), must use a pointer to reference the data

void SetVDMCursorBounds(LPRECT lprc);


VOID UserSetDelayedChangeBroadcastForAllDesktops(PDESKTOP pCurrentDesktop);


VOID UserSetDelayedChangeBroadcastForAllDesktops(
    PDESKTOP pCurrentDesktop)
{
    PWINDOWSTATION pwinsta;
    PDESKTOP       pdesk;

    /*
     * Get a pointer to the windowstation so we can change display
     * setting for all of its destops.
     */
    if ((pwinsta = grpWinStaList) == NULL) {
        RIPMSG0(RIP_WARNING, "UserSaveCurrentModeForAllDesktops - No interactive WindowStation!!!\n");
        return;
    }

    /*
     * Walk all the desktops of the winstation and, for each of them,
     * just set its delayed Broadcast indicator to TRUE so that
     * next switch to that destop will force Display Settings change
     * messages to be broadcasted to windows of that desktop.
     */
    pdesk = pwinsta->rpdeskList;

    while (pdesk != NULL) {
        if (pdesk != pCurrentDesktop) {
            pdesk->dwDTFlags |= DF_NEWDISPLAYSETTINGS;
        }
        pdesk = pdesk->rpdeskNext;
    }
}


/***************************************************************************\
* FullScreenCleanup
*
* This is called during thread cleanup, we test to see if we died during a
* full screen switch and switch back to the GDI desktop if we did.
*
* NOTE:
* All the variables touched here are guaranteed to be touched under
* the CritSect.
*
* 12-Dec-1991 mikeke   Created
\***************************************************************************/

void FullScreenCleanup()
{
    if (PsGetCurrentThread()->Cid.UniqueThread == idSwitcher) {

        /*
         * correct the full screen state
         */

        if (fGdiEnabled) {

            TRACE_SWITCH(("Switching: FullScreenCleanup: Gdi Enabled\n"));

            /*
             * gdi is enabled, we are switching away from gdi the only thing we
             * could have done so far is locking the screen so unlock it.
             */
            CLEAR_PUDF(PUDF_LOCKFULLSCREEN);
            LockWindowUpdate2(NULL, TRUE);

        } else {

            /*
             * GDI is not enabled .  This means we were switching from a full
             * screen to another fullscreen or back to GDI.  Or we could have
             * disabled gdi and sent a message to the new full screen which
             * never got completed.
             *
             * In any case this probably means the fullscreen guy is gone so
             * we will switch back to gdi.
             *
             * delete any left over saved screen state stuff
             * set the fullscreen to nothing and then send a message that will
             * cause us to switch back to the gdi desktop
             */
            TL tlpwndT;

            TRACE_SWITCH(("Switching: FullScreenCleanup: Gdi Disabled\n"));

            Unlock(&gspwndFullScreen);
            gbFullScreen = FULLSCREEN;

            ThreadLock(grpdeskRitInput->pDeskInfo->spwnd, &tlpwndT);
            xxxSendNotifyMessage(
                                grpdeskRitInput->pDeskInfo->spwnd, WM_FULLSCREEN,
                                GDIFULLSCREEN, (LPARAM)HW(grpdeskRitInput->pDeskInfo->spwnd));
            ThreadUnlock(&tlpwndT);
        }

        idSwitcher = NOSWITCHER;
        fRedoFullScreenSwitch = FALSE;
    }
}

/***************************************************************************\
* xxxMakeWindowForegroundWithState
*
* Syncs the screen graphics mode with the mode of the specified (foreground)
* window
*
* We make sure only one thread is going through this code by checking
* idSwitcher.  If idSwticher is non-null someone is allready in this code
*
* 12-Dec-1991 mikeke   Created
\***************************************************************************/

BOOL xxxMakeWindowForegroundWithState(
                                     PWND pwnd,
                                     BYTE NewState)
{
    PWND pwndNewFG;
    TL tlpwndNewFG;

    TRACE_SWITCH(("Switching: xxxMakeWindowForeground: Enter\n"));
    TRACE_SWITCH(("\t \t pwnd     = %08lx\n", pwnd));
    TRACE_SWITCH(("\t \t NewState = %d\n", NewState));

    CheckLock(pwnd);
    UserAssert(IsWinEventNotifyDeferredOK());

    /*
     * If we should switch to a specific window save that window
     */

    if (pwnd != NULL) {

        if (NewState == GDIFULLSCREEN) {
            Lock(&gspwndShouldBeForeground, pwnd);
        }

        /*
         * Change to the new state
         */

        SetFullScreen(pwnd, NewState);

        if (NewState == FULLSCREEN &&
            (gpqForeground == NULL ||
             gpqForeground->spwndActive != pwnd)) {

            SetFullScreen(pwnd, FULLSCREENMIN);
        }
    }

    //
    // Since we leave the critical section during the switch, some other
    // thread could come into this routine and request a switch.  The global
    // will be reset, and we will use the loop to perform the next switch.
    //

    if (idSwitcher != NOSWITCHER) {
        fRedoFullScreenSwitch = TRUE;
        TRACE_SWITCH(("Switching: xxxMakeWindowForeground was posted: Exit\n"));

        return TRUE;
    }

    UserAssert(!fRedoFullScreenSwitch);
    idSwitcher = PsGetCurrentThread()->Cid.UniqueThread;

    /*
     * We loop, switching full screens until all states have stabilized
     */

    while (TRUE) {
        /*
         * figure out who should be foreground
         */
        fRedoFullScreenSwitch = FALSE;

        if (gspwndShouldBeForeground != NULL) {
            pwndNewFG = gspwndShouldBeForeground;
            Unlock(&gspwndShouldBeForeground);
        } else {
            if (gpqForeground != NULL &&
                gpqForeground->spwndActive != NULL) {

                pwndNewFG = gpqForeground->spwndActive;

                if (GetFullScreen(pwndNewFG) == WINDOWED ||
                    GetFullScreen(pwndNewFG) == FULLSCREENMIN) {

                    pwndNewFG = PWNDDESKTOP(pwndNewFG);
                }
            } else {
                /*
                 * No active window, switch to current desktop
                 */
                pwndNewFG = grpdeskRitInput->pDeskInfo->spwnd;
            }
        }

        /*
         * We don't need to switch if the right window is already foreground
         */
        if (pwndNewFG == gspwndFullScreen) {
            break;
        }

        ThreadLock(pwndNewFG, &tlpwndNewFG);

        {
            BYTE bStateNew = GetFullScreen(pwndNewFG);
            TL tlpwndOldFG;
            PWND pwndOldFG = gspwndFullScreen;
            BYTE bStateOld = gbFullScreen;

            ThreadLock(pwndOldFG, &tlpwndOldFG);

            Lock(&gspwndFullScreen, pwndNewFG);
            gbFullScreen = bStateNew;

            UserAssert(!HMIsMarkDestroy(gspwndFullScreen));

            /*
             * If the old screen was GDIFULLSCREEN and we are switching to
             * GDIFULLSCREEN then just repaint
             */
            
            /*
             * BUG 231647: For remote sessions it can happen that
             * pwndOldFG is NULL but the display is enabled therefore a
             * call to DrvEnableMDEV would confuse the Drv* code. The way
             * this happens is when gspwndFullScreen was the desktop window
             * of a desktop that got destroyed after we switched away from it.
             */
            
            if ((pwndOldFG != NULL || gbRemoteSession) &&
                bStateOld == GDIFULLSCREEN &&
                bStateNew == GDIFULLSCREEN) {

                xxxRedrawWindow(pwndNewFG, NULL, NULL,
                                RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE | RDW_ERASENOW);

                ThreadUnlock(&tlpwndOldFG);

            } else {

                /*
                 * tell old 'foreground' window it is losing control of the screen
                 */
                if (pwndOldFG != NULL) {
                    switch (bStateOld) {
                    case FULLSCREEN:
                        if (GetFullScreen(pwndOldFG) == FULLSCREEN) {
                            SetFullScreen(pwndOldFG, FULLSCREENMIN);
                        }
                        xxxSendMessage(pwndOldFG, WM_FULLSCREEN, FALSE, 0);
                        xxxCapture(GETPTI(pwndOldFG), NULL, FULLSCREEN_CAPTURE);
                        SetVDMCursorBounds(NULL);
                        break;

                    case GDIFULLSCREEN:
                        /*
                         * Lock out other windows from drawing while we are fullscreen
                         */
                        LockWindowUpdate2(pwndOldFG, TRUE);
                        SET_PUDF(PUDF_LOCKFULLSCREEN);

                        UserAssert(fGdiEnabled == TRUE);

                        if (!DrvDisableMDEV(gpDispInfo->pmdev, TRUE)) {
                            /*
                             * Restore the previous state before bailing.
                             */
                            CLEAR_PUDF(PUDF_LOCKFULLSCREEN);
                            LockWindowUpdate2(NULL, TRUE);

                            Lock(&gspwndFullScreen, pwndOldFG);
                            gbFullScreen = bStateOld;

                            ThreadUnlock(&tlpwndOldFG);
                            ThreadUnlock(&tlpwndNewFG);

                            idSwitcher = NOSWITCHER;

                            return FALSE;
                        }

                        gptCursorFullScreen = gpsi->ptCursor;
                        fGdiEnabled = FALSE;
                        break;

                    default:
                        RIPMSG0(RIP_ERROR, "xxxDoFullScreenSwitch: bad screen state");
                        break;

                    }
                }

                ThreadUnlock(&tlpwndOldFG);

                switch (bStateNew) {
                case FULLSCREEN:
                    xxxCapture(GETPTI(pwndNewFG), pwndNewFG, FULLSCREEN_CAPTURE);
                    xxxSendMessage(pwndNewFG, WM_FULLSCREEN, TRUE, 0);
                    break;

                case GDIFULLSCREEN:
                    if (ISTS() && pwndOldFG != NULL) {
                        UserAssert(fGdiEnabled == FALSE);
                    }

                    DrvEnableMDEV(gpDispInfo->pmdev, TRUE);
                    fGdiEnabled = TRUE;

                    /*
                     * Return the cursor to it's old state. Reset the screen saver mouse
                     * position or it'll go away by accident.
                     */
                    gpqCursor = NULL;
                    gpcurPhysCurrent = NULL;
                    gpcurLogCurrent = NULL;
                    SetPointer(FALSE);
                    gptSSCursor = gptCursorFullScreen;

                    /*
                     * No need to DeferWinEventNotify() - we use only globals,
                     * then make an xxx call below.
                     */
                    zzzInternalSetCursorPos(gptCursorFullScreen.x,
                                            gptCursorFullScreen.y
                                           );

                    CLEAR_PUDF(PUDF_LOCKFULLSCREEN);
                    LockWindowUpdate2(NULL, TRUE);

                    xxxRedrawWindow(pwndNewFG, NULL, NULL,
                                    RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE | RDW_ERASENOW);
                    break;

                default:
                    RIPMSG0(RIP_ERROR, "xxxDoFullScreenSwitch: bad screen state\n");
                    break;
                }
            }
        }

        ThreadUnlock(&tlpwndNewFG);

        if (!fRedoFullScreenSwitch) {
            break;
        }
    }

    TRACE_SWITCH(("Switching: xxxMakeWindowForeground: Exit\n"));

    idSwitcher = NOSWITCHER;
    return TRUE;
}

/***************************************************************************\
* MonitorFromHdev
\***************************************************************************/

PMONITOR MonitorFromHdev(HANDLE hdev)
{
    PMONITOR pMonitor;

    for (pMonitor = gpDispInfo->pMonitorFirst; pMonitor != NULL;
            pMonitor = pMonitor->pMonitorNext) {
        if (pMonitor->hDev == hdev) {
            return pMonitor;
        }
    }
    return NULL;
}

/***************************************************************************\
* HdevFromMonitor
\***************************************************************************/

ULONG HdevFromMonitor(PMONITOR pMonitor)
{
    PMDEV pmdev = gpDispInfo->pmdev;
    ULONG i;

    for (i = 0; i < pmdev->chdev; i++) {
        if (pmdev->Dev[i].hdev == pMonitor->hDev) {
            return i;
        }
    }
    return -1;
}

/***************************************************************************\
* CreateMonitor
\***************************************************************************/

PMONITOR CreateMonitor(void)
{
    PMONITOR pMonitor;

    pMonitor = (PMONITOR)HMAllocObject(NULL, NULL, TYPE_MONITOR, sizeof(MONITOR));

    if (pMonitor == NULL) {
        RIPERR0(ERROR_NOT_ENOUGH_MEMORY, RIP_WARNING, "CreateMonitor failed");
    }

    return pMonitor;
}

/***************************************************************************\
* CreateCachedMonitor
\***************************************************************************/

PMONITOR CreateCachedMonitor(VOID)
{
    if (gpMonitorCached == NULL) {
        gpMonitorCached = CreateMonitor();
    }

    return gpMonitorCached;
}

/***************************************************************************\
* SetMonitorData
\***************************************************************************/

PMONITOR SetMonitorData(PMONITOR pMonitor, ULONG iDev)
{
    PMDEV pmdev = gpDispInfo->pmdev;
    HDEV hdev = pmdev->Dev[iDev].hdev;
    BOOL fVisible = TRUE;
    BOOL fPrimary = FALSE;
    HDC hdcTmp;

    UserAssert(iDev < pmdev->chdev);

    if (hdcTmp = GreCreateDisplayDC(hdev, DCTYPE_DIRECT, FALSE)) {
        if (GreGetDeviceCaps(hdcTmp, CAPS1) & C1_MIRROR_DEVICE) {
            fVisible = FALSE;
        }
        GreDeleteDC(hdcTmp);
    }

    if (fVisible && (pmdev->Dev[iDev].rect.top == 0) &&
            (pmdev->Dev[iDev].rect.left == 0)) {
        fPrimary = TRUE;
    }

    if (pMonitor == NULL) {
        if (fPrimary) {
            UserAssert(gpMonitorCached != NULL);
            pMonitor = gpMonitorCached;
            gpMonitorCached = NULL;
        } else {
            pMonitor = CreateMonitor();
        }
    }

    if (pMonitor == NULL) {
        return NULL;
    }

    SET_OR_CLEAR_FLAG(pMonitor->dwMONFlags, MONF_VISIBLE, fVisible);

    /*
     * When the monitor rect is changing, size the work area so the same
     * amount as before is clipped off each edge.
     */
    if (!EqualRect(&pMonitor->rcMonitor, &pmdev->Dev[iDev].rect)) {
        pMonitor->rcWork.left = pmdev->Dev[iDev].rect.left -
                (pMonitor->rcMonitor.left - pMonitor->rcWork.left);
        pMonitor->rcWork.top = pmdev->Dev[iDev].rect.top -
                (pMonitor->rcMonitor.top - pMonitor->rcWork.top);
        pMonitor->rcWork.right = pmdev->Dev[iDev].rect.right -
                (pMonitor->rcMonitor.right - pMonitor->rcWork.right);
        pMonitor->rcWork.bottom = pmdev->Dev[iDev].rect.bottom -
                (pMonitor->rcMonitor.bottom - pMonitor->rcWork.bottom);
    }
    pMonitor->rcMonitor = pmdev->Dev[iDev].rect;
    pMonitor->hDev = hdev;

    if (fPrimary) {
        gpDispInfo->pMonitorPrimary = pMonitor;
    }

    return pMonitor;
}

/***************************************************************************\
*
* Is this still TRUE ?
*
* When a window becomes FULLSCREEN, it is minimized and
* treated like any other minimized window.  Whenever the
* minimized window is restored, by double clicking, menu
* or keyboard, it remains minimized and the application
* is given control of the screen device.
*
* 12-Dec-1991 mikeke   Created
\***************************************************************************/


/***************************************************************************\
* UpdateUserScreen
*
* Updates USER information associated with the screen
*
* History:
* 28-Sep-1996 adams     Created.
\***************************************************************************/

BOOL UpdateUserScreen(void)
{
    PMDEV           pmdev = gpDispInfo->pmdev;
    ULONG           i;
    PMONITOR        pMonitor;
    TEXTMETRIC      tm;
    int             iRgn;
    PWINDOWSTATION  pwinsta;
    PDESKTOP        pdesk;
    HRGN            hrgn;
    BOOL            fPaletteDisplay;
    RECT            rc;
    PMONITOR pMonitorNext = gpDispInfo->pMonitorFirst;
    PMONITOR *ppMonitorLast = &gpDispInfo->pMonitorFirst;


    TRACE_INIT(("UpdateUserScreen\n"));

    UserAssert(gpDispInfo->hdcScreen);
    UserAssert(gpMonitorCached != NULL);

    /*
     * Keep HMONITOR for the hdev that is the same.  Delete the
     * monitors that weren't found in the new hdev list.
     */
    while (pMonitorNext != NULL) {
        pMonitor = pMonitorNext;
        pMonitorNext = pMonitor->pMonitorNext;

        if ((i = HdevFromMonitor(pMonitor)) == -1) {
            DestroyMonitor(pMonitor);
        } else {
            SetMonitorData(pMonitor, i);
            ppMonitorLast = &pMonitor->pMonitorNext;
        }
    }

    /*
     * Create monitors for the hdevs that aren't yet on the monitor list.
     */
    for (i = 0; i < pmdev->chdev; i++) {
        if ((pMonitor = MonitorFromHdev(pmdev->Dev[i].hdev)) == NULL) {

            /*
             * Try to create a new monitor.
             */
            pMonitor = SetMonitorData(NULL, i);

            if (pMonitor != NULL) {
                *ppMonitorLast = pMonitor;
                ppMonitorLast = &pMonitor->pMonitorNext;
            }
        }
    }

    UserAssert(gpDispInfo->pMonitorFirst != NULL);
    UserAssert(gpDispInfo->pMonitorPrimary != NULL);

    /*
     * For now, all monitors have the same display format.
     */
    SYSMET(SAMEDISPLAYFORMAT) = (pmdev->ulFlags & MDEV_MISMATCH_COLORDEPTH) ? FALSE : TRUE;
    fPaletteDisplay = GreGetDeviceCaps(gpDispInfo->hdcScreen, RASTERCAPS) & RC_PALETTE;
    gpDispInfo->fAnyPalette = !!fPaletteDisplay;

    /*
     * Determine the coordinates of the virtual desktop.
     * Compute cMonitors as the number of visible monitors.
     */
    SetRectEmpty(&rc);

    gpDispInfo->cMonitors = 0;
    for (pMonitor = gpDispInfo->pMonitorFirst;
        pMonitor;
        pMonitor = pMonitor->pMonitorNext) {

        /*
         * Only visible monitors contribute to the desktop area
         */
        if (pMonitor->dwMONFlags & MONF_VISIBLE) {
            rc.left = min(rc.left, pMonitor->rcMonitor.left);
            rc.top = min(rc.top, pMonitor->rcMonitor.top);
            rc.right = max(rc.right, pMonitor->rcMonitor.right);
            rc.bottom = max(rc.bottom, pMonitor->rcMonitor.bottom);

            gpDispInfo->cMonitors++;
        }

        if (SYSMET(SAMEDISPLAYFORMAT)) {
            SET_OR_CLEAR_FLAG(pMonitor->dwMONFlags, MONF_PALETTEDISPLAY, fPaletteDisplay);
        } else {
            if (GreIsPaletteDisplay(pMonitor->hDev)) {
                pMonitor->dwMONFlags |= MONF_PALETTEDISPLAY;
                gpDispInfo->fAnyPalette = TRUE;
            }
        }
    }
    UserAssert(gpDispInfo->pMonitorPrimary != NULL);
    gpDispInfo->rcScreen = rc;

    /*
     * Update system metrics
     */
    SYSMET(CXSCREEN)        = gpDispInfo->pMonitorPrimary->rcMonitor.right;
    SYSMET(CYSCREEN)        = gpDispInfo->pMonitorPrimary->rcMonitor.bottom;
    SYSMET(XVIRTUALSCREEN)  = gpDispInfo->rcScreen.left;
    SYSMET(YVIRTUALSCREEN)  = gpDispInfo->rcScreen.top;
    SYSMET(CXVIRTUALSCREEN) = gpDispInfo->rcScreen.right - gpDispInfo->rcScreen.left;
    SYSMET(CYVIRTUALSCREEN) = gpDispInfo->rcScreen.bottom - gpDispInfo->rcScreen.top;
    SYSMET(CXMAXTRACK)      = SYSMET(CXVIRTUALSCREEN) + (2 * (SYSMET(CXSIZEFRAME) + SYSMET(CXEDGE)));
    SYSMET(CYMAXTRACK)      = SYSMET(CYVIRTUALSCREEN) + (2 * (SYSMET(CYSIZEFRAME) + SYSMET(CYEDGE)));
    SYSMET(CMONITORS)       = gpDispInfo->cMonitors;

    /*
     * Bug 281219: flush out the mouse move points if a mode change occured
     */
    RtlZeroMemory(gaptMouse, MAX_MOUSEPOINTS * sizeof(MOUSEMOVEPOINT));

    SetDesktopMetrics();

    gpDispInfo->dmLogPixels = (WORD)GreGetDeviceCaps(gpDispInfo->hdcScreen, LOGPIXELSY);

    UserAssert(gpDispInfo->dmLogPixels != 0);

    /*
     * Get per-monitor or sum of monitor information, including:
     *     The desktop region.
     *     The region of each monitor.
     *     Min bit counts - Not for NT SP2.
     *     Same color format - Not for NT SP2.
     */

    SetOrCreateRectRgnIndirectPublic(&gpDispInfo->hrgnScreen, PZERO(RECT));

    if (gpDispInfo->hrgnScreen) {

        for (pMonitor = gpDispInfo->pMonitorFirst;
            pMonitor;
            pMonitor = pMonitor->pMonitorNext) {

            /*
             *  We want to set up hrgnMonitor for all monitors, visible or not
             */
            if (SetOrCreateRectRgnIndirectPublic(&pMonitor->hrgnMonitor,
                                                 &pMonitor->rcMonitor)) {

                /*
                 *  But we want only visible monitors to contribute to hrgnScreen
                 */
                if (pMonitor->dwMONFlags & MONF_VISIBLE) {
                    iRgn = UnionRgn(gpDispInfo->hrgnScreen,
                                    gpDispInfo->hrgnScreen,
                                    pMonitor->hrgnMonitor);
                }

            }
        }

        gpDispInfo->fDesktopIsRect = (iRgn == SIMPLEREGION);
    }


    /*
     * Reset the window region of desktop windows.
     */
    hrgn = (gpDispInfo->fDesktopIsRect) ? NULL : gpDispInfo->hrgnScreen;
    for (pwinsta = grpWinStaList; pwinsta; pwinsta = pwinsta->rpwinstaNext) {
        for (pdesk = pwinsta->rpdeskList; pdesk; pdesk = pdesk->rpdeskNext) {
            if (pdesk->pDispInfo == gpDispInfo) {
                pdesk->pDeskInfo->spwnd->hrgnClip = hrgn;
            }
        }
    }

    /*
     * Updated information stored in gpsi.
     */
    gpsi->Planes        = (BYTE)GreGetDeviceCaps(gpDispInfo->hdcScreen, PLANES);
    gpsi->BitsPixel     = (BYTE)GreGetDeviceCaps(gpDispInfo->hdcScreen, BITSPIXEL);
    gpsi->BitCount      = gpsi->Planes * gpsi->BitsPixel;
    gpDispInfo->BitCountMax = gpsi->BitCount;
    SET_OR_CLEAR_PUSIF(PUSIF_PALETTEDISPLAY, fPaletteDisplay);
    gpsi->dmLogPixels   = gpDispInfo->dmLogPixels;
    gpsi->rcScreen      = gpDispInfo->rcScreen;
    gpsi->cxSysFontChar = GetCharDimensions(HDCBITS(), &tm, &gpsi->cySysFontChar);
    gpsi->tmSysFont     = tm;

    EnforceColorDependentSettings();

#if DBG
    VerifyVisibleMonitorCount();
#endif

    return TRUE;
}


/**************************************************************************\
* InitUserScreen
*
* Initializes user variables at startup.
*
* BUGBUG (adams): This function needs to handle failures.
* BUGBUG (adams): Where do these variables get inited for
*     DISPLAYINFOs other than gpDispInfo?
*
* 12-Jan-1994 andreva       Created
* 23-Jan-1995 ChrisWil      ChangeDisplaySettings work.
\**************************************************************************/

BOOL
InitUserScreen()
{
    int i;
    TL tlName;
    PUNICODE_STRING pProfileUserName = CreateProfileUserName(&tlName);
    BOOL fSuccess = TRUE;

    TRACE_INIT(("UserInit: Initialize Screen\n"));

    /*
     * Create screen and memory dcs.
     */
    gpDispInfo->hdcScreen = GreCreateDisplayDC(gpDispInfo->hDev, DCTYPE_DIRECT, FALSE);

    if (gpDispInfo->hdcScreen == NULL) {
        RIPMSG0(RIP_WARNING, "Fail to create gpDispInfo->hdcScreen");
        fSuccess = FALSE;
        goto Exit;
    }

    GreSelectFont(gpDispInfo->hdcScreen, GreGetStockObject(SYSTEM_FONT));
    GreSetDCOwner(gpDispInfo->hdcScreen, OBJECT_OWNER_PUBLIC);

    HDCBITS() = GreCreateCompatibleDC(gpDispInfo->hdcScreen);

    if (HDCBITS() == NULL) {
        RIPMSG0(RIP_WARNING, "Fail to create HDCBITS()");
        fSuccess = FALSE;
        goto Exit;
    }

    GreSelectFont(HDCBITS(), GreGetStockObject(SYSTEM_FONT));
    GreSetDCOwner(HDCBITS(), OBJECT_OWNER_PUBLIC);

    ghdcMem = GreCreateCompatibleDC(gpDispInfo->hdcScreen);
    fSuccess &= !!ghdcMem;

    ghdcMem2 = GreCreateCompatibleDC(gpDispInfo->hdcScreen);
    fSuccess &= !!ghdcMem2;

    if (!fSuccess) {
        RIPMSG0(RIP_WARNING, "Fail to create ghdcMem or ghdcMem2");
        goto Exit;
    }

    GreSetDCOwner(ghdcMem, OBJECT_OWNER_PUBLIC);
    GreSetDCOwner(ghdcMem2, OBJECT_OWNER_PUBLIC);

    if (CreateCachedMonitor() == NULL) {
        fSuccess = FALSE;
        goto Exit;
    }

    if (!UpdateUserScreen()) {
        RIPMSG0(RIP_WARNING, "UpdateUserScreen failed");
        fSuccess = FALSE;
        goto Exit;
    }

    /*
     * Do some initialization so we create the system colors.
     */

    /*
     * Set the window sizing border width to something reasonable.
     */
    gpsi->gclBorder = 1;

    /*
     * Init InternalInvalidate globals
     */
    ghrgnInv0 = CreateEmptyRgnPublic();    // For InternalInvalidate()
    fSuccess &= !!ghrgnInv0;

    ghrgnInv1 = CreateEmptyRgnPublic();    // For InternalInvalidate()
    fSuccess &= !!ghrgnInv1;

    ghrgnInv2 = CreateEmptyRgnPublic();    // For InternalInvalidate()
    fSuccess &= !!ghrgnInv2;

    /*
     * Initialize SPB globals
     */
    ghrgnSPB1 = CreateEmptyRgnPublic();
    fSuccess &= !!ghrgnSPB1;

    ghrgnSPB2 = CreateEmptyRgnPublic();
    fSuccess &= !!ghrgnSPB2;

    ghrgnSCR  = CreateEmptyRgnPublic();
    fSuccess &= !!ghrgnSCR;

    /*
     * Initialize ScrollWindow/ScrollDC globals
     */
    ghrgnSW        = CreateEmptyRgnPublic();
    fSuccess &= !!ghrgnSW;

    ghrgnScrl1     = CreateEmptyRgnPublic();
    fSuccess &= !!ghrgnScrl1;

    ghrgnScrl2     = CreateEmptyRgnPublic();
    fSuccess &= !!ghrgnScrl2;

    ghrgnScrlVis   = CreateEmptyRgnPublic();
    fSuccess &= !!ghrgnScrlVis;

    ghrgnScrlSrc   = CreateEmptyRgnPublic();
    fSuccess &= !!ghrgnScrlSrc;

    ghrgnScrlDst   = CreateEmptyRgnPublic();
    fSuccess &= !!ghrgnScrlDst;

    ghrgnScrlValid = CreateEmptyRgnPublic();
    fSuccess &= !!ghrgnScrlValid;

    /*
     * Initialize SetWindowPos()
     */
    ghrgnInvalidSum = CreateEmptyRgnPublic();
    fSuccess &= !!ghrgnInvalidSum;

    ghrgnVisNew     = CreateEmptyRgnPublic();
    fSuccess &= !!ghrgnVisNew;

    ghrgnSWP1       = CreateEmptyRgnPublic();
    fSuccess &= !!ghrgnSWP1;

    ghrgnValid      = CreateEmptyRgnPublic();
    fSuccess &= !!ghrgnValid;

    ghrgnValidSum   = CreateEmptyRgnPublic();
    fSuccess &= !!ghrgnValidSum;

    ghrgnInvalid    = CreateEmptyRgnPublic();
    fSuccess &= !!ghrgnInvalid;

    /*
     * Initialize DC cache
     */
    ghrgnGDC = CreateEmptyRgnPublic();
    fSuccess &= !!ghrgnGDC;

    for (i = 0; i < DCE_SIZE_CACHEINIT; i++) {
        fSuccess &= !!CreateCacheDC(NULL, DCX_INVALID | DCX_CACHE, NULL);
    }

    if (!fSuccess) {
        RIPMSG0(RIP_WARNING, "CreateCacheDC failed");
        goto Exit;
    }

    /*
     * Let engine know that the display must be secure.
     */

    GreMarkDCUnreadable(gpDispInfo->hdcScreen);

    /*
     * LATER mikeke - if ghfontsys is changed anywhere but here
     * we need to fix SetNCFont()
     */
    ghFontSys = (HFONT)GreGetStockObject(SYSTEM_FONT);

#if DBG
    SYSMET(DEBUG) = TRUE;
#else
    SYSMET(DEBUG) = FALSE;
#endif

    SYSMET(CLEANBOOT) = **((PULONG *)&InitSafeBootMode);

    SYSMET(SLOWMACHINE) = 0;

    /*
     * Initialize system colors from registry.
     */
    xxxODI_ColorInit(pProfileUserName);

    /*
     * Paint the screen background.
     */
    FillRect(gpDispInfo->hdcScreen, &gpDispInfo->rcScreen, SYSHBR(DESKTOP));

    UserAssert(fSuccess);

Exit:
    FreeProfileUserName(pProfileUserName, &tlName);

    return fSuccess;
}


/***************************************************************************\
* xxxResetSharedDesktops
*
* Resets the attributes for other desktops which share the DISPINFO that
* was just changed.  We need to resize all visrgns of the other desktops
* so that clipping is allright.
*
* NOTE:  For now, we have to change all the desktop even though we keep
* track of the devmode on a per desktop basis, because we can switch
* back to a desktop that has a different resolution and paint it before
* we can change the resolution again.
* There is also an issue with CDS_FULLSCREEN where we currently loose track
* of whether or not the desktop settings need to be reset or not. [andreva]
*
* 19-Feb-1996 ChrisWil Created.
\***************************************************************************/

VOID ResetSharedDesktops(
                        PDISPLAYINFO pDIChanged,
                        PDESKTOP     pdeskChanged)
{
    PWINDOWSTATION pwinsta = _GetProcessWindowStation(NULL);
    PDESKTOP       pdesk;
    HRGN           hrgn;
    POINT          pt;
    PRECT          prc;
    UserAssert(IsWinEventNotifyDeferredOK());

    /*
     * If this is CSRSS doing the dynamic resolution change then use
     * WinSta0 since the process windowstation is NULL for CSRSS.
     */
    if (gbRemoteSession && pwinsta == NULL && PsGetCurrentProcess() == gpepCSRSS) {
        pwinsta = grpWinStaList;
    }

    if (pwinsta == NULL) {

        if (PtiCurrent()->TIF_flags & (TIF_CSRSSTHREAD|TIF_SYSTEMTHREAD))
        {
            pwinsta =  grpdeskRitInput->rpwinstaParent;
        }
        else
        {
            TRACE_SWITCH(("ResetSharedDesktops - NULL window station !\n"));
            return;
        }
    }

    for (pdesk = pwinsta->rpdeskList; pdesk; pdesk = pdesk->rpdeskNext) {

        /*
         * Make sure this is a shared DISPINFO.
         */
        if (pdesk->pDispInfo == pDIChanged) {

#if 0
            /*
             * This is the preferable method to set the desktop-window.
             * However, this causes synchronization problems where we
             * leave the critical-section allowing other apps to call
             * ChangeDisplaySettings() and thus mucking up the works.
             *
             * By calculating the vis-rgn ourselves, we can assure that
             * the clipping is current for the desktop even when we leave
             * the section.
             */
            {
                TL tlpwnd;

                ThreadLockAlways(pdesk->pDeskInfo->spwnd, &tlpwnd);
                xxxSetWindowPos(pdesk->pDeskInfo->spwnd,
                                PWND_TOP,
                                pDIChanged->rcScreen.left,
                                pDIChanged->rcScreen.top,
                                pDIChanged->rcScreen.right - pDIChanged->rcScreen.left,
                                pDIChanged->rcScreen.bottom - pDIChanged->rcScreen.top,
                                SWP_NOZORDER | SWP_NOACTIVATE);
                ThreadUnlock(&tlpwnd);
            }
#else
            CopyRect(&pdesk->pDeskInfo->spwnd->rcWindow, &pDIChanged->rcScreen);
            CopyRect(&pdesk->pDeskInfo->spwnd->rcClient, &pDIChanged->rcScreen);
#endif
        }
    }

    /*
     * Recalc the desktop visrgn.  Since the hdcScreen is shared amoungts
     * all the
     */
    hrgn = CreateEmptyRgn();
    CalcVisRgn(&hrgn,
               pdeskChanged->pDeskInfo->spwnd,
               pdeskChanged->pDeskInfo->spwnd,
               DCX_WINDOW);

    GreSelectVisRgn(pDIChanged->hdcScreen, hrgn, SVR_DELETEOLD);

    /*
     * Invalidate all DCE's visrgns.
     */
    zzzInvalidateDCCache(pdeskChanged->pDeskInfo->spwnd, 0);

    /*
     * Position mouse so that it is within the new visrgn, once we
     * recalc it.
     */
    if (grpdeskRitInput->pDispInfo == pDIChanged) {
        prc = &pDIChanged->pMonitorPrimary->rcMonitor;
        pt.x = (prc->right - prc->left) / 2;
        pt.y = (prc->bottom - prc->top) / 2;

        /*
         * Remember new mouse pos. Makes sure we don't wake the screensaver.
         */
        gptSSCursor = pt;
        zzzInternalSetCursorPos(pt.x, pt.y);
    }
}

/***************************************************************************\
* DestroyMonitorDCs
*
* 03/03/1998      vadimg      created
\***************************************************************************/

void DestroyMonitorDCs(void)
{
    PDCE pdce;
    PDCE *ppdce;

    /*
     * Scan the DC cache to find any monitor DC's that need to be destroyed.
     */
    for (ppdce = &gpDispInfo->pdceFirst; *ppdce != NULL; ) {

        pdce = *ppdce;

        if (pdce->pMonitor != NULL) {
            DestroyCacheDC(ppdce, pdce->hdc);
        }

        /*
         * Step to the next DC.  If the DC was deleted, there
         * is no need to calculate address of the next entry.
         */
        if (pdce == *ppdce)
            ppdce = &pdce->pdceNext;
    }
}

/***************************************************************************\
* ResetSystemColors
*
* Reset all system colors to make sure magic colors are reset and
* solid system colors are indeed solid after a mode change.
\***************************************************************************/

VOID ResetSystemColors(VOID)
{
    INT         i;
    INT         colorIndex[COLOR_MAX];
    COLORREF    colorValue[COLOR_MAX];

    for (i = 0; i < COLOR_MAX; i++) {
        colorIndex[i] = i;
        colorValue[i] = gpsi->argbSystem[i];
    }

    BEGINATOMICCHECK();
    xxxSetSysColors(NULL, i, colorIndex, colorValue, SSCF_FORCESOLIDCOLOR |
            SSCF_SETMAGICCOLORS);
    ENDATOMICCHECK();
}

/***************************************************************************\
* xxxResetDisplayDevice
*
* Resets the user-globals with the new hdev settings.
*
* 19-Feb-1996 ChrisWil Created.
\***************************************************************************/

VOID xxxResetDisplayDevice(
                          PDESKTOP     pdesk,
                          PDISPLAYINFO pDI,
                          DWORD        CDS_Flags)
{
    WORD            wOldBpp;
    PMONITORRECTS   pmr = NULL;
    TL              tlPool;
    PTHREADINFO     ptiCurrent = PtiCurrent();

    /*
     * BUGBUG: Correctly update work area for each monitor.
     */
    wOldBpp = gpsi->BitCount;

    if (!(CDS_Flags & CDS_FULLSCREEN)) {

        pmr = SnapshotMonitorRects();

        if (pmr) {
            ThreadLockPool(ptiCurrent, pmr, &tlPool);
        }
    }

    /*
     * Cleanup any monitor specific DCs we gave out.
     */
    DestroyMonitorDCs();

    UpdateUserScreen();
    ResetSharedDesktops(pDI, pdesk);

    ResetSystemColors();

    if (ghbmCaption) {
        GreDeleteObject(ghbmCaption);
        ghbmCaption = CreateCaptionStrip();
    }

    zzzClipCursor(&pDI->rcScreen);

    /*
     * Adjust window positions to fit new resolutions and
     * positions of monitors.
     */
    if (pmr) {
        xxxDesktopRecalc(pmr);
        ThreadUnlockAndFreePool(PtiCurrent(), &tlPool);
    }

    /*
     * Change the wallpaper metrics.
     * Don't reload the wallpaper when in cleanup since that may
     * go to the client for and we can't do that.
     */
    if (!(ptiCurrent->TIF_flags & TIF_INCLEANUP) && ghbmWallpaper) {
        TL tlName;
        PUNICODE_STRING pProfileUserName = CreateProfileUserName(&tlName);
        xxxSetDeskWallpaper(pProfileUserName, SETWALLPAPER_METRICS);
        FreeProfileUserName(pProfileUserName, &tlName);
    }

    /*
     * Recreate cached bitmaps.
     */
    CreateBitmapStrip();

    /*
     * Broadcast that the display has changed resolution.
     * Also broadcast a color-change if we were not in fullscreen, and a
     * color-change took effect.
     */
    if (!(CDS_Flags & CDS_FULLSCREEN) && (gpsi->BitCount != wOldBpp)) {
       xxxBroadcastDisplaySettingsChange(pdesk, TRUE);

    } else {
       xxxBroadcastDisplaySettingsChange(pdesk, FALSE);
    }

    /*
     * If the user performed a CTL-ESC, it is possible that the
     * tray-window is then in the menu-loop.  We want to clear this
     * out so that we don't leave improper menu positioning.
     */
    if (gpqForeground && gpqForeground->spwndCapture)
        QueueNotifyMessage(gpqForeground->spwndCapture, WM_CANCELMODE, 0, 0l);
}

/***************************************************************************\
* TrackFullscreenMode
*
* Remember the process going into the fullscreen mode, so that
* the mode can be restored if the process doesn't clean up upon
* exit. If some other mode change, clear the global since that
* means we're definitely out of the fullscreen mode.
*
* 1/12/1999        vadimg      created
\***************************************************************************/

VOID TrackFullscreenMode(DWORD dwFlags)
{
    if (dwFlags & CDS_FULLSCREEN) {
        gppiFullscreen = PtiCurrent()->ppi;
    } else {
        gppiFullscreen = NULL;
    }
}

/***************************************************************************\
* NtUserChangeDisplaySettings
*
* ChangeDisplaySettings API
*
* 01-Sep-1995 andreva  Created
* 19-Feb-1996 ChrisWil Implemented Dynamic-Resolution changes.
\***************************************************************************/




LONG
xxxUserChangeDisplaySettings(
                            IN PUNICODE_STRING pstrDeviceName,
                            IN LPDEVMODEW      pDevMode,
                            IN HWND            hwnd,
                            IN PDESKTOP        pdesk,
                            IN DWORD           dwFlags,
                            IN PVOID           lParam,
                            IN MODE            PreviousMode)
{
    BOOL     bSwitchMode;
    PDESKTOP pdesktop;
    LONG     status;
    PMDEV    pmdev;

    /*
     * BUGBUG: Has lParam been properly captured?
     */

    lParam;

    TRACE_INIT(("ChangeDisplaySettings - Entering\n"));
    TRACE_SWITCH(("ChangeDisplaySettings - Entering\n"));

    TRACE_INIT(("    Flags -"));

    if (dwFlags & CDS_UPDATEREGISTRY) TRACE_INIT((" CDS_UPDATEREGISTRY - "));
    if (dwFlags & CDS_TEST) TRACE_INIT((" CDS_TEST - "));
    if (dwFlags & CDS_FULLSCREEN) TRACE_INIT((" CDS_FULLSCREEN - "));
    if (dwFlags & CDS_GLOBAL) TRACE_INIT((" CDS_GLOBAL - "));
    if (dwFlags & CDS_SET_PRIMARY) TRACE_INIT((" CDS_SET_PRIMARY - "));
    if (dwFlags & CDS_RESET) TRACE_INIT((" CDS_RESET - "));
    if (dwFlags & CDS_NORESET) TRACE_INIT((" CDS_NORESET - "));
    if (dwFlags & CDS_VIDEOPARAMETERS) TRACE_INIT((" CDS_VIDEOPARAMETERS - "));
    TRACE_INIT(("\n"));

#if ((DISP_CHANGE_SUCCESSFUL != GRE_DISP_CHANGE_SUCCESSFUL)  ||  \
     (DISP_CHANGE_RESTART    != GRE_DISP_CHANGE_RESTART)     ||  \
     (DISP_CHANGE_FAILED     != GRE_DISP_CHANGE_FAILED)      ||  \
     (DISP_CHANGE_BADMODE    != GRE_DISP_CHANGE_BADMODE)     ||  \
     (DISP_CHANGE_NOTUPDATED != GRE_DISP_CHANGE_NOTUPDATED)  ||  \
     (DISP_CHANGE_BADFLAGS   != GRE_DISP_CHANGE_BADFLAGS)    ||  \
     (DISP_CHANGE_BADPARAM   != GRE_DISP_CHANGE_BADPARAM))
    #error "inconsistent header files"
#endif

    /*
     * Perform Error Checking to verify flag combinations are valid.
     */
    if (dwFlags & (~CDS_VALID)) {

        //BUGBUG
        //RIPMSG0(RIP_ERROR, "ChangeDisplaySettings: invalid flags specified\n");
        return DISP_CHANGE_BADFLAGS;
    }

    /*
     * CDS_GLOBAL and CDS_NORESET can only be specified if UPDAREREGISTRY
     * is specified.
     */

    if ((dwFlags & (CDS_GLOBAL | CDS_NORESET))  &&
        (!(dwFlags & CDS_UPDATEREGISTRY))) {

        //BUGBUG
        //RIPMSG0(RIP_ERROR, "ChangeDisplaySettings: invalid registry flags specified\n");
        return DISP_CHANGE_BADFLAGS;
    }

    if ((dwFlags & CDS_NORESET)  &&
        (dwFlags & CDS_RESET)) {

        //BUGBUG
        //RIPMSG0(RIP_ERROR, "ChangeDisplaySettings: RESET and NORESET can not be put together\n");
        return DISP_CHANGE_BADFLAGS;
    }

    if ((dwFlags & CDS_EXCLUSIVE) && (dwFlags & CDS_FULLSCREEN) && (dwFlags & CDS_RESET)) {

        //BUGBUG
        //RIPMSG0(RIP_ERROR, "ChangeDisplaySettings: invalid flags specified\n");
        return DISP_CHANGE_BADFLAGS;
    }

    if (hwnd) {
        return DISP_CHANGE_BADPARAM;
    }

    /*
     * For now, don't allow dynamic-resolution changes to occur
     * when GDI is in fullscreen.  We should almost never hit this
     * condition, except for rare stress cases.
     */


    /*
     * Allow mode change if this is a CSRSS of a remote
     * session. This means we are changing display settings when
     * reconnecting a session with a diferent resolution.
     */
    if (TEST_PUDF(PUDF_LOCKFULLSCREEN)) {
        if (!(ISCSRSS() && gbRemoteSession)) {
            return DISP_CHANGE_FAILED;
        }
    }


    /*
     * If the modeset is being done on a non-active desktop, we don't want
     * it too happen.
     *
     * PtiCurrent()->rpdesk can be NULL !!! (in the case of thread shutdown).
     */

    if (pdesk) {
        pdesktop = pdesk;
    } else {
        pdesktop = PtiCurrent()->rpdesk;
    }

    if (pdesktop != grpdeskRitInput) {
        RIPMSG0(RIP_WARNING, "ChangeDisplaySettings on wrong desktop pdesk\n");
        return DISP_CHANGE_FAILED;
    }

    bSwitchMode = !(dwFlags & (CDS_NORESET | CDS_TEST));

    /*
     * Turn off cursor and free the spb's prior to calling the mode-change.
     * This will make sure off-screen memory is cleaned up for gdi.
     * while mucking with the resolution changes.
     */
    if (bSwitchMode) {

        if (CreateCachedMonitor() == NULL) {
            return DISP_CHANGE_FAILED;
        }

        SetPointer(FALSE);
        FreeAllSpbs();
    }

    /*
     * Before calling gdi to change the mode, we should kill the fade sprite.  This is
     * so that we won't keep pointers to gdi sprites during the mode change because
     * the sprites could be reallocated.
     */
    
    if (gfade.hbm != NULL) {
        StopFade();
    }

    /*
     * Similarly, we should kill the sprites associated with the drag rect (if any
     * exist) before the mode change.
     */
    
    bSetDevDragRect(gpDispInfo->hDev, NULL, NULL);

    /*
     * Lets capture our parameters.  They are both required.
     *
     * If the input string is not NULL, then we are trying to affect another
     * Device. The device name is the same as for EnumDisplaySettings.
     */

    status = DrvChangeDisplaySettings(pstrDeviceName,
                                      gpDispInfo->pMonitorPrimary->hDev,
                                      pDevMode,
                                      LongToPtr( pdesktop->dwDesktopId ),
                                      PreviousMode,
                                      (dwFlags & CDS_UPDATEREGISTRY),
                                      bSwitchMode,
                                      gpDispInfo->pmdev,
                                      &pmdev,
                                      (dwFlags & CDS_RAWMODE) ? GRE_RAWMODE : GRE_DEFAULT,
                                      (dwFlags & CDS_TRYCLOSEST));


    if (bSwitchMode) {

        /*
         * If the caller wanted a reset, but the mode is identical, just
         * reset the current mode.
         */

        if (status == GRE_DISP_CHANGE_NO_CHANGE) {

            TrackFullscreenMode(dwFlags);

            if (pmdev != NULL) {
                GreFreePool(pmdev);
            }

            if (dwFlags & CDS_RESET) {

                if (DrvDisableMDEV(gpDispInfo->pmdev, TRUE)) {
                    DrvEnableMDEV(gpDispInfo->pmdev, TRUE);
                }

                xxxUserResetDisplayDevice();
            }

            status = DISP_CHANGE_SUCCESSFUL;

        } else if (status == DISP_CHANGE_SUCCESSFUL) {

            TrackFullscreenMode(dwFlags);

            /*
             * ChangeDisplaySettings automatically destroys the old MDEV.
             * We only have to delete it here.
             */
            GreFreePool(gpDispInfo->pmdev);
            gpDispInfo->pmdev = pmdev;
            xxxResetDisplayDevice(pdesktop, gpDispInfo, dwFlags);

            /*
             * set delayed change indicator for
             * currently background desktops.
             */
            UserSetDelayedChangeBroadcastForAllDesktops(pdesktop);

        } else if (status < DISP_CHANGE_SUCCESSFUL) {
            UserAssert(pmdev == NULL);
            xxxUserResetDisplayDevice();
        }

        /*
         * Inline so we can specify which desktop this should happen on.
         * xxxRedrawScreen();
         */
        xxxInternalInvalidate(pdesktop->pDeskInfo->spwnd,
                              HRGN_FULL, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);

        /*
         * Bring back the cursor-shape.
         */
        SetPointer(TRUE);
        zzzUpdateCursorImage();
    }

    /*
     * TV-Out Support
     */

    if (NT_SUCCESS(status) && (dwFlags & CDS_VIDEOPARAMETERS))
    {
        if (lParam == NULL) {
            status = DISP_CHANGE_BADPARAM;
        } else {
            status = DrvSetVideoParameters(pstrDeviceName,
                                           gpDispInfo->pMonitorPrimary->hDev,
                                           PreviousMode,
                                           lParam);
        }
    }

    TRACE_INIT(("ChangeDisplaySettings - Leaving, Status = %d\n", status));

    return status;
}


/***************************************************************************\
* xxxbFullscreenSwitch
*
* Switch in and out of fullscreen console mode
*
* 15-Apr-1997 andreva  Created
\***************************************************************************/

BOOL
xxxbFullscreenSwitch(
                    BOOL bFullscreenSwitch,
                    HWND hwnd)
{

    PWND pwnd;
    TL   tlpwnd;
    BOOL bStat = TRUE;

    pwnd = ValidateHwnd(hwnd);

    if (!pwnd) {
        return DISP_CHANGE_BADPARAM;
    }

    /*
     * Lock the PWND, if it is provided
     */

    ThreadLock(pwnd, &tlpwnd);

    /*
     * We don't want our mode switch to be posted on the looping thread.
     * So let's loop until the system has settled down and no mode switch
     * is currently occuring.
     */

    while (idSwitcher != NOSWITCHER) {
        /*
         * Make sure we aren't blocking anyone who's sending us a message.
         * They can have idSwitcher and never release it because they are
         * waiting on us to process the sent message. And we're waiting on
         * idSwitcher, hence a deadlock.
         */
        xxxSleepThread(0, 1, FALSE);
    }

    /*
     * If there is a window, we want to check the state of the window.
     * For most calls, we want to ensure we are in windowed mode.
     * However, for Console, we want to make sure we are in fullscreen mode.
     * So differentiate between the two.  We will check if the TEXTMODE
     * flag is passed in the DEVMODE.
     */

    if (bFullscreenSwitch) {

        if (GetFullScreen(pwnd) != FULLSCREEN) {

            xxxShowWindow(pwnd, SW_SHOWMINIMIZED | TEST_PUDF(PUDF_ANIMATE));

            xxxUpdateWindow(pwnd);
        }

        if (!xxxMakeWindowForegroundWithState(pwnd, FULLSCREEN)) {
            goto FullscreenSwitchFailed;
        }

        if ((idSwitcher != NOSWITCHER) ||
            (gbFullScreen != FULLSCREEN)) {
            goto FullscreenSwitchFailed;
        }

    } else {

        /*
         * For the console windows, we want to call with WINDOWED
         */

        if (!xxxMakeWindowForegroundWithState(pwnd, WINDOWED)) {
            goto FullscreenSwitchFailed;
        }

        if ((idSwitcher != NOSWITCHER) ||
            (gbFullScreen != GDIFULLSCREEN)) {

            FullscreenSwitchFailed:
            TRACE_INIT(("ChangeDisplaySettings: Can not switch out of fullscreen\n"));
            bStat = FALSE;
        }
    }

    ThreadUnlock(&tlpwnd);

    return bStat;
}


NTSTATUS
RemoteRedrawRectangle(
    WORD Left,
    WORD Top,
    WORD Right,
    WORD Bottom)
{
    TL   tlpwnd;
    RECT rcl;

    CheckCritIn();

    TRACE_HYDAPI(("RemoteRedrawRectangle\n"));

    UserAssert(ISCSRSS());
    /*
     * If xxxRemoteStopScreenUpdates has not been called,
     * then just repaint the current foreground window.
     */
    if (gspdeskShouldBeForeground == NULL) {
        if (gspwndFullScreen) {

            ThreadLock(gspwndFullScreen, &tlpwnd);

            rcl.left   = Left;
            rcl.top    = Top;
            rcl.right  = Right;
            rcl.bottom = Bottom;

            vDrvInvalidateRect(gpDispInfo->hDev, &rcl);

            xxxRedrawWindow(gspwndFullScreen, &rcl, NULL,
                            RDW_INVALIDATE | RDW_ALLCHILDREN |
                            RDW_ERASE | RDW_ERASENOW);
            ThreadUnlock(&tlpwnd);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RemoteRedrawScreen(
    VOID)
{
    TL             tlpdesk;
    PWINDOWSTATION pwinsta;
    DWORD          dwFlagsPrev;
    PTHREADINFO    ptiCurrent;

    TRACE_HYDAPI(("RemoteRedrawScreen\n"));

    CheckCritIn();

    if (!gbFreezeScreenUpdates)
        return STATUS_SUCCESS;

    ptiCurrent = PtiCurrentShared();

    gbFreezeScreenUpdates = FALSE;

    /*
     * Switch back to the previous desktop
     */
    if (gspdeskShouldBeForeground == NULL) {
        RIPMSG0(RIP_WARNING, "RemoteRedrawScreen called with no gspdeskShouldBeForeground");
        return STATUS_SUCCESS;
    }

    /*
     * Clear desktop switch lock
     * (WSF_SWITCHLOCK must be clear for xxxSwitchDesktop to succeed)
     */
    gbDesktopLocked = FALSE;

    pwinsta = gspdeskShouldBeForeground->rpwinstaParent;

    dwFlagsPrev = pwinsta->dwWSF_Flags;
    pwinsta->dwWSF_Flags &= ~WSF_SWITCHLOCK;

    /*
     * Switch back to previous desktop
     */
    if (!(gspdeskShouldBeForeground->dwDTFlags & DF_DESTROYED)) {
        ThreadLockDesktop(ptiCurrent, gspdeskShouldBeForeground, &tlpdesk, LDLT_FN_CTXREDRAWSCREEN);
        xxxSwitchDesktop(pwinsta, gspdeskShouldBeForeground, FALSE);
        ThreadUnlockDesktop(ptiCurrent, &tlpdesk, LDUT_FN_CTXREDRAWSCREEN);
        
        /*
         * Enable screen updates
         */
        DrvEnableMDEV(gpDispInfo->pmdev, TRUE);
    }
    pwinsta->dwWSF_Flags = dwFlagsPrev;
    LockDesktop(&gspdeskShouldBeForeground, NULL, LDL_DESKSHOULDBEFOREGROUND2, 0 );

    return STATUS_SUCCESS;
}

NTSTATUS
RemoteDisableScreen(
    VOID)
{
    TL             tlpdesk;
    DWORD          dwFlagsPrev;
    PTHREADINFO    ptiCurrent;
    PWINDOWSTATION pwinsta;
    NTSTATUS       Status = STATUS_SUCCESS;

    CheckCritIn();

    TRACE_HYDAPI(("RemoteDisableScreen\n"));


    ptiCurrent = PtiCurrentShared();

    if (grpdeskRitInput != gspdeskDisconnect &&
        gspdeskDisconnect != NULL) {

        pwinsta = gspdeskDisconnect->rpwinstaParent;

        /*
         * Save current desktop
         */
        UserAssert(grpdeskRitInput == pwinsta->pdeskCurrent);

        LockDesktop(&gspdeskShouldBeForeground,
                    grpdeskRitInput,
                    LDL_DESKSHOULDBEFOREGROUND3, 0);

        /*
         * Set desktop switch lock
         * (WSF_SWITCHLOCK must be clear for xxxSwitchDesktop to succeed)
         */
        dwFlagsPrev = pwinsta->dwWSF_Flags;
        pwinsta->dwWSF_Flags &= ~WSF_SWITCHLOCK;
        gbDesktopLocked = TRUE;

        /*
         * Switch to Disconnected desktop
         */
        ThreadLockDesktop(ptiCurrent, gspdeskDisconnect, &tlpdesk, LDLT_FN_CTXDISABLESCREEN);
        xxxSwitchDesktop(pwinsta, gspdeskDisconnect, FALSE);
        ThreadUnlockDesktop(ptiCurrent, &tlpdesk, LDUT_FN_CTXDISABLESCREEN);

        pwinsta->dwWSF_Flags = dwFlagsPrev;

        /*
         * Disable screen updates
         */
        DrvDisableMDEV(gpDispInfo->pmdev, TRUE);

    } else if (gspdeskDisconnect != NULL) {
        /*
         * For some reason the disconnected desktop was the current desktop.
         * Now prevent switching from it.
         */
        gbDesktopLocked = TRUE;
    }

    return Status;
}

VOID
xxxBroadcastDisplaySettingsChange(
    PDESKTOP pdesk,
    BOOL     bBroadcastColorChange)
{

    /*
     * Broadcast that the display has changed resolution.  We are going
     * to specify the desktop for the changing-desktop.  That way we
     * don't get confused as to what desktop to broadcast to.
     */
    xxxBroadcastMessage(pdesk->pDeskInfo->spwnd,
                       WM_DISPLAYCHANGE,
                       gpsi->BitCount,
                       MAKELONG(SYSMET(CXSCREEN), SYSMET(CYSCREEN)),
                       BMSG_SENDNOTIFYMSG,
                       NULL);

    /*
     * Broadcast a color-change if requested to do so.
     */

    if (bBroadcastColorChange){
#if 1 // We might want to remove this call, since color-change seems
       // to provide apps the notification.  Need to review
       // chriswil - 06/11/96

       xxxBroadcastMessage(pdesk->pDeskInfo->spwnd,
                          WM_SETTINGCHANGE,
                          0,
                          0,
                          BMSG_SENDNOTIFYMSG,
                          NULL);
#endif

       xxxBroadcastMessage(pdesk->pDeskInfo->spwnd,
                          WM_SYSCOLORCHANGE,
                          0,
                          0,
                          BMSG_SENDNOTIFYMSG,
                          NULL);
    }

}
