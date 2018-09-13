/****************************** Module Header ******************************\
* Module Name: hungapp.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
*
* History:
* 03-10-92 DavidPe      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* SetHungFlag
*
* Sets the specified redraw-if-hung flag in the window and adds the
* window to the list of windows to redraw if hung.
* Windows that are not top-level get the bit set, but aren't added to the list
*
* 08-23-93  JimA        Created.
\***************************************************************************/
#define CHRLINCR 10

VOID SetHungFlag(
    PWND pwnd,
    WORD wFlag)
{
    /*
     * If the window has no hung redraw bits set and it's a top-level
     * window, add it to the redraw list.
     */
    if (!TestWF(pwnd, WFANYHUNGREDRAW) && pwnd->spwndParent == PWNDDESKTOP(pwnd)) {
        /*
         * Add pwnd to the Hung Redraw Volatile Window Pointer List.
         */
        VWPLAdd(&gpvwplHungRedraw, pwnd, CHRLINCR);
    }

    SetWF(pwnd, wFlag);
}


/***************************************************************************\
* ClearHungFlag
*
* Clears the specified redraw-if-hung flag in the window and if no other
* redraw-if-hung flags remain, remove the window from list of windows
* to be redrawn if hung.
* Many windows have WFREDRAW* bits set, but aren't in the list (only those
* that were top-level were added).
*
* 08-23-93  JimA        Created.
\***************************************************************************/

VOID ClearHungFlag(
    PWND pwnd,
    WORD wFlag)
{
    BOOL fInRedrawList = TestWF(pwnd, WFANYHUNGREDRAW);

    ClrWF(pwnd, wFlag);
    if (!TestWF(pwnd, WFANYHUNGREDRAW) && fInRedrawList) {
        /*
         * Remove the window from the redraw list and possibly compact it.
         */
        VWPLRemove(&gpvwplHungRedraw, pwnd);
    }
}


/***************************************************************************\
* FHungApp
*
*
* 02-28-92  DavidPe     Created.
\***************************************************************************/

BOOL FHungApp(
    PTHREADINFO pti,
    DWORD dwTimeFromLastRead)
{

    /*
     * An app is considered hung if it isn't waiting for input, isn't in
     * startup processing, and hasn't called PeekMessage() within the
     * specified timeout.
     */
    if (((NtGetTickCount() - GET_TIME_LAST_READ(pti)) > dwTimeFromLastRead) &&
            !((pti->pcti->fsWakeMask & QS_INPUT) && (pti->pEThread->Tcb.FreezeCount == 0)) &&
            !(pti->ppi->W32PF_Flags & W32PF_APPSTARTING)) {
        return TRUE;
    }

    return FALSE;
}


/***************************************************************************\
* xxxRedrawHungWindowFrame
*
*
* 02-28-92  DavidPe     Created.
\***************************************************************************/

VOID xxxRedrawHungWindowFrame(
    PWND pwnd,
    BOOL fActive)
{
    HDC hdc;
    UINT wFlags = DC_NC | DC_NOSENDMSG;

    CheckLock(pwnd);

    if (fActive)
        wFlags |= DC_ACTIVE;

    hdc = _GetDCEx(pwnd, NULL, DCX_USESTYLE | DCX_WINDOW);
    xxxDrawCaptionBar(pwnd, hdc, wFlags);
    _ReleaseDC(hdc);
}


/***************************************************************************\
* xxxRedrawHungWindow
*
* If the hrgnFullDrag is NULL, redraw the hung window's entire update region,
* otherwise, only redraw the intersection of the window's update region
* with the FullDrag region.
*
* 02-28-92  DavidPe     Created.
\***************************************************************************/

VOID xxxRedrawHungWindow(
    PWND pwnd,
    HRGN hrgnFullDrag)
{
    HDC     hdc;
    HBRUSH  hbr;
    HRGN    hrgnUpdate;
    RECT    rc;
    TL tlpwnd; // should remove (IanJa)
    UINT    flags;
    W32PID  sid;
    DWORD   dwColor;
    PWND    pwndDesk;
    TL      tlpwndDesk;

    CheckCritIn();
    CheckLock(pwnd);

    if (pwnd->hrgnUpdate == NULL) {
        return;
    }

#ifdef HUNGAPP_GHOSTING

    /*
     * Don't bother doing anything here when the window isn't even visible.
     */
    if (!TestWF(pwnd, WFVISIBLE)) {
        return;
    }

    /*
     * This function can be called from the full-drag code to quick redraw
     * windows that aren't hung. In that case check if that thread is hung.
     */
    if ((hrgnFullDrag == NULL) || (hrgnFullDrag != NULL &&
            FHungApp(GETPTI(pwnd), CMSHUNGAPPTIMEOUT))) {
        SignalGhost(pwnd);
    }

#endif // HUNGAPP_GHOSTING

    /*
     * First calculate hrgnUpdate.
     */
    if (pwnd->hrgnUpdate > HRGN_FULL) {
        hrgnUpdate = CreateEmptyRgn();
        if (hrgnUpdate == NULL) {
            hrgnUpdate = HRGN_FULL;

        } else if (CopyRgn(hrgnUpdate, pwnd->hrgnUpdate) == ERROR) {
            GreDeleteObject(hrgnUpdate);
            hrgnUpdate = HRGN_FULL;
        }

    } else {

        /*
         * For our purposes, we need a real hrgnUpdate, so try and
         * create one if even if the entire window needs updating.
         */
        CopyRect(&rc, &pwnd->rcWindow);
        hrgnUpdate = GreCreateRectRgnIndirect(&rc);
        if (hrgnUpdate == NULL) {
            hrgnUpdate = HRGN_FULL;
        }
    }

    /*
     * If we're redrawing because we're full dragging and if the window's
     * update region does not intersect with the Full drag
     * update region, don't erase the hung window again. This is to prevent
     * flickering when a window has been invalidated by another window doing
     * full drag and hasn't received the paint message yet.
     * This way, only if there is a new region that has been invalidated will
     * we redraw the hung window.
     */
    if (hrgnFullDrag && hrgnUpdate != HRGN_FULL &&
            IntersectRgn(hrgnUpdate, hrgnUpdate, hrgnFullDrag) == NULLREGION) {
        GreDeleteObject(hrgnUpdate);
        return;
    }

    ThreadLock(pwnd, &tlpwnd); // should remove (IanJa)

    hdc = _GetDCEx(pwnd, hrgnUpdate, DCX_USESTYLE | DCX_WINDOW |
            DCX_INTERSECTRGN | DCX_NODELETERGN | DCX_LOCKWINDOWUPDATE);
    xxxDrawWindowFrame(pwnd, hdc, TRUE, TestwndFrameOn(pwnd));
    _ReleaseDC(hdc);

    CopyRect(&rc, &pwnd->rcWindow);
    xxxCalcClientRect(pwnd, &rc, TRUE);
    SetRectRgnIndirect(ghrgnInv2, &rc);

    if (hrgnUpdate > HRGN_FULL) {
        switch (IntersectRgn(hrgnUpdate, hrgnUpdate, ghrgnInv2)) {

        case ERROR:
            GreDeleteObject(hrgnUpdate);
            hrgnUpdate = HRGN_FULL;
            break;

        case NULLREGION:
            /*
             * There is nothing in the client area to repaint.
             * Blow the region away, and decrement the paint count
             * if possible.
             */
            GreDeleteObject(hrgnUpdate);
            hrgnUpdate = NULL;
            break;
        }
    }

    /*
     * Erase the rest of the window.
     * When pwnd isn't WFCLIPCHILDREN, make sure valid children bits
     * don't get overwritten if the child is in the middle of BeginPaint
     * or just completed it's painting and it's hrgnUpdate is NULL.
     */
    if (hrgnUpdate != NULL && !TestWF(pwnd, WFCLIPCHILDREN)) {
        RECT rcT;
        PWND pwndT;

        if (hrgnUpdate == HRGN_FULL) {
            rc = pwnd->rcWindow;
        } else {
            GreGetRgnBox(hrgnUpdate, &rc);
        }

        for (pwndT = pwnd->spwndChild; pwndT != NULL;
                pwndT = pwndT->spwndNext) {

            if (TestWF(pwndT, WFVISIBLE) &&
                    (TestWF(pwndT, WFSTARTPAINT) || pwndT->hrgnUpdate == NULL) &&
                    IntersectRect(&rcT, &rc, &pwndT->rcWindow)) {

                /*
                 * This invalidate call won't leave the critial section. In
                 * reality the entire xxxRedrawHungWindow must not leave
                 * the critical section.
                 */
                BEGINATOMICCHECK();
                xxxInternalInvalidate(pwndT, hrgnUpdate, RDW_INVALIDATE |
                        RDW_FRAME | RDW_ERASE | RDW_ALLCHILDREN);
                ENDATOMICCHECK();
            }
        }
    }

    /*
     * Get a window dc so that the menu and scroll bar areas are erased
     * appropriately. But make sure it is clipped so that the children
     * get clipped out correctly! If we don't do this, this we could erase
     * children that aren't invalid.
     *
     * Note: DCX_WINDOW and DCX_USESTYLE will never clip out children.
     * Need to pass the clipping styles in directly, instead of passing
     * DCX_USESTYLE.
     */
    flags = DCX_INTERSECTRGN | DCX_WINDOW | DCX_CACHE;
    if (TestWF(pwnd, WFCLIPSIBLINGS))
        flags |= DCX_CLIPSIBLINGS;
    if (TestWF(pwnd, WFCLIPCHILDREN))
        flags |= DCX_CLIPCHILDREN;

    hdc = _GetDCEx(pwnd, hrgnUpdate, flags);

    if (pwnd == pwnd->head.rpdesk->pDeskInfo->spwndBkGnd) {
        pwndDesk = PWNDDESKTOP(pwnd);
        ThreadLock(pwndDesk, &tlpwndDesk);
        xxxInternalPaintDesktop(PWNDDESKTOP(pwnd), hdc, TRUE);
        ThreadUnlock(&tlpwndDesk);

    } else {

         rc = pwnd->rcWindow;

         OffsetRect(&rc, -pwnd->rcWindow.left, -pwnd->rcWindow.top);

         /*
          * Erase the rest of the window using the window' class background
          * brush.
          */
         if ((hbr = pwnd->pcls->hbrBackground) != NULL) {
             if (hbr <= (HBRUSH)COLOR_ENDCOLORS + 1)
                 hbr = SYSHBRUSH((ULONG_PTR)hbr - 1);
         } else {
             /*
              * Use the window brush for windows and 3.x dialogs,
              * Use the COLOR3D brush for 4.x dialogs.
              */
             if (TestWF(pwnd, WFDIALOGWINDOW) && TestWF(pwnd, WFWIN40COMPAT))
                 hbr = SYSHBR(3DFACE);
             else
                 hbr = SYSHBR(WINDOW);
         }

        /*
         * If the window's class background brush is public, use it.
         */
        sid = (W32PID)GreGetObjectOwner((HOBJ)hbr, BRUSH_TYPE);
        if (sid == (W32PID)(ULONG_PTR)GetCurrentProcessId() || sid == OBJECT_OWNER_PUBLIC) {

            FillRect(hdc, &rc, hbr);

        } else {

            /*
             * The window's class background brush is not public.
             * We get its color and set the color of our own public brush and use
             * that for the background brush.
             */

            /*
             * If the window is a console window, get the console background brush.
             * This brush will be different than the console class brush if the user
             * changed the console background color.
             */
            if (gatomConsoleClass == pwnd->pcls->atomClassName) {

                dwColor = _GetWindowLong(pwnd, GWL_CONSOLE_BKCOLOR);

            } else {

                if ((dwColor = GreGetBrushColor(hbr)) == -1)
                    dwColor = GreGetBrushColor(SYSHBR(WINDOW));
            }

            GreSetSolidBrush(ghbrHungApp, dwColor);

            FillRect(hdc, &rc, ghbrHungApp);
        }
    }
    _ReleaseDC(hdc);

    /*
     * The window has been erased and framed. It only did this because the
     * app hasn't done it yet:
     *
     * - the app hasn't erased and frame yet.
     * - the app is in the middle of erasing and framing.
     *
     * The app could not of completed erasing and framing, because the
     * WFREDRAWIFHUNG bit is cleared when this successfully completes.
     *
     * Given that the app may be in the middle of erasing and framing, we
     * need to set both the erase and frame bits *again* so it erasing and
     * frames over again (if we don't, it never will). If the app hasn't
     * done any erasing/framing yet, this is a nop.
     */
    SetWF(pwnd, WFSENDNCPAINT);
    SetWF(pwnd, WFSENDERASEBKGND);

    /*
     * Always set WFUPDATEDIRTY: we don't want the app to draw, then stop
     * and have the hung app thread draw, and then allow the app to validate
     * itself: Mark the update region dirty - cannot be validated until the
     * app calls a painting function and acknowledges the update region.
     */
    SetWF(pwnd, WFUPDATEDIRTY);

#ifdef WIN95DOESTHIS
    /*
     * Go through all the children and redraw hung ones too.
     */
    if (hrgnFullDrag != NULL) {
        PWND    pwndT;
        TL      tlpwndT;

        pwndT = pwnd->spwndChild;
        ThreadLockNever(&tlpwndT);
        while (pwndT) {

            if (    TestWF(pwndT, WFREDRAWIFHUNG) &&
                    FHungApp(GETPTI(pwndT), CMSHUNGAPPTIMEOUT)) {

                ClearHungFlag(pwndT, WFREDRAWIFHUNG);
                ThreadLockExchangeAlways(pwndT, &tlpwndT);
                xxxRedrawHungWindow(pwndT, NULL);
            }

            if (TestWF(pwndT, WFDESTROYED)) {
                break;
            }

            pwndT = pwndT->spwndNext;
        }

        ThreadUnlock(&tlpwndT);
    }
#endif

    ThreadUnlock(&tlpwnd); // should remove (IanJa)
}


/***************************************************************************\
* xxxHungAppDemon
*
* NOTE: RIT timers (like this one) get called while inside an EnterCrit block.
*
* We keep a list of redraw-if-hung windows in a list that remains in a
* single page to avoid touching the windows themselves each time through
* this routine.  Touching the windows causes a bunch of unnecessary paging
* and in effect keeps all of the pages that contain top-level windows
* resident at all times; this is very wasteful.
*
* 02-28-92  DavidPe     Created.
\***************************************************************************/

VOID xxxHungAppDemon(
    PWND pwnd,
    UINT message,
    UINT_PTR nID,
    LPARAM lParam)
{
    TL      tlpwnd;
#if DBG
    PWND    pwndT;
#endif
    DWORD nPwndHungRedraw;
    PWND  pwndHungRedraw;



    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(nID);

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(pwnd);
    CheckLock(pwnd);

    /*
     * See if we should start the screen saver.
     */
    IdleTimerProc();

    /*
     * If it is time to hide the app starting cursor, do it.
     */
    if (NtGetTickCount() >= gtimeStartCursorHide) {
        /*
         * No need to DeferWinEventNotify()
         */
        zzzCalcStartCursorHide(NULL, 0);
    }

    /*
     * Now check to see if there are any top-level
     * windows that need redrawing.
     */
    if (grpdeskRitInput == NULL || grpdeskRitInput->pDeskInfo->spwnd == NULL)
        return;

    /*
     * Walk down the list of redraw-if-hung windows.  Loop
     * until we hit the end of the array or find a NULL.
     */
    nPwndHungRedraw = 0;
    pwndHungRedraw = NULL;
    while (pwndHungRedraw = VWPLNext(gpvwplHungRedraw, pwndHungRedraw, &nPwndHungRedraw)) {
        /*
         * See if the app is hung.  If so, do the appropriate
         * redrawing.
         */
        if (FHungApp(GETPTI(pwndHungRedraw), CMSHUNGAPPTIMEOUT)) {
            ThreadLock(pwndHungRedraw, &tlpwnd);
            if (TestWF(pwndHungRedraw, WFREDRAWFRAMEIFHUNG)) {

                /*
                 * WFREDRAWFRAMEIFHUNG will be cleared in the process
                 * of drawing the frame, no need to clear it here.
                 */
                xxxRedrawHungWindowFrame(pwndHungRedraw, TestwndFrameOn(pwndHungRedraw));
            }

            if (TestWF(pwndHungRedraw, WFREDRAWIFHUNG)) {
                ClearHungFlag(pwndHungRedraw, WFREDRAWIFHUNG);
                xxxRedrawHungWindow(pwndHungRedraw, NULL);
            }
            #if DBG
                pwndT =
            #endif

            ThreadUnlock(&tlpwnd);
        }
    }

    return;
}
