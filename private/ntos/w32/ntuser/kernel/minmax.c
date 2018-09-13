/****************************** Module Header ******************************\
* Module Name: minmax.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
*  Window Minimize/Maximize Routines
*
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * How long we want animation to last, in milliseconds
 */
#define CMS_ANIMATION       250
#define DX_GAP      (SYSMET(CXMINSPACING) - SYSMET(CXMINIMIZED))
#define DY_GAP      (SYSMET(CYMINSPACING) - SYSMET(CYMINIMIZED))


/***************************************************************************\
* xxxInitSendValidateMinMaxInfo()
*
* Routine which initializes the minmax array, sends WM_GETMINMAXINFO to
* the caller, and validates the results.
*
* Returns FALSE is the window went away in the middle.
*
\***************************************************************************/

void
xxxInitSendValidateMinMaxInfo(PWND pwnd, LPMINMAXINFO lpmmi)
{
    PTHREADINFO     ptiCurrent;
    PMONITOR        pMonitorReal;
    PMONITOR        pMonitorPrimary;
    TL              tlpMonitorReal;
    TL              tlpMonitorPrimary;
    CHECKPOINT *    pcp;
    RECT            rcParent;
    int             cBorders;
    int             xMin, yMin;

    CheckLock(pwnd);

    ptiCurrent = PtiCurrent();

    /*
     * FILL IN THE MINMAXINFO WE THINK IS APPROPRIATE
     */

    /*
     * Minimized Size
     */
    lpmmi->ptReserved.x = SYSMET(CXMINIMIZED);
    lpmmi->ptReserved.y = SYSMET(CYMINIMIZED);

    /*
     * Maximized Position and Size
     * Figure out where the window would be maximized within its parent.
     */
    pMonitorPrimary = GetPrimaryMonitor();
    if (pwnd->spwndParent == PWNDDESKTOP(pwnd)) {
        /* What monitor is the window really going to maximize to? */
        pMonitorReal = _MonitorFromWindow(pwnd, MONITOR_DEFAULTTOPRIMARY);

        /* Send dimensions based on the primary only. */
        rcParent = pMonitorPrimary->rcMonitor;
    } else {
        pMonitorReal = NULL;
        _GetClientRect(pwnd->spwndParent, &rcParent);
    }

    cBorders = GetWindowBorders(pwnd->style, pwnd->ExStyle, TRUE, FALSE);

    InflateRect(&rcParent,
                cBorders * SYSMET(CXBORDER),
                cBorders * SYSMET(CYBORDER));

    rcParent.right -= rcParent.left;
    rcParent.bottom -= rcParent.top;

    /* rcParent.right, bottom are width and height now. */
    lpmmi->ptMaxSize.x = rcParent.right;
    lpmmi->ptMaxSize.y = rcParent.bottom;

    pcp = (CHECKPOINT *)_GetProp(pwnd, PROP_CHECKPOINT, PROPF_INTERNAL);
    if (pcp && pcp->fMaxInitialized) {
        /*
         * Note:  For top level windows, we will fix this point up after
         * the fact if it has gotten out of date because the size border
         * changed.
         */
        lpmmi->ptMaxPosition = pcp->ptMax;
    } else {
        lpmmi->ptMaxPosition = *((LPPOINT)&rcParent.left);
    }

    /*
     * Normal minimum tracking size
     * Only enforce min tracking size for windows with captions
     */
    xMin = cBorders*SYSMET(CXEDGE);
    yMin = cBorders*SYSMET(CYEDGE);

    if (TestWF(pwnd, WFCAPTION) && !TestWF(pwnd, WEFTOOLWINDOW)) {
        lpmmi->ptMinTrackSize.x = SYSMET(CXMINTRACK);
        lpmmi->ptMinTrackSize.y = SYSMET(CYMINTRACK);
    } else {
        lpmmi->ptMinTrackSize.x = max(SYSMET(CXEDGE), xMin);
        lpmmi->ptMinTrackSize.y = max(SYSMET(CYEDGE), yMin);
    }

    /*
     * Normal maximum tracking size
     */
    lpmmi->ptMaxTrackSize.x = SYSMET(CXMAXTRACK);
    lpmmi->ptMaxTrackSize.y = SYSMET(CYMAXTRACK);

    /*
     * SEND THE WM_GETMINMAXINFO MESSAGE
     */

    ThreadLockWithPti(ptiCurrent, pMonitorReal, &tlpMonitorReal);
    ThreadLockAlwaysWithPti(ptiCurrent, pMonitorPrimary, &tlpMonitorPrimary);
    xxxSendMessage(pwnd, WM_GETMINMAXINFO, 0, (LPARAM)lpmmi);

    /*
     * VALIDATE THE MINMAXINFO
     */

    /*
     * Minimized Size (this is read only)
     */
    lpmmi->ptReserved.x = SYSMET(CXMINIMIZED);
    lpmmi->ptReserved.y = SYSMET(CYMINIMIZED);

    /*
     * Maximized Postion and Size (only for top level windows)
     */
    if (pwnd->spwndParent == PWNDDESKTOP(pwnd)) {
        LPRECT  lprcRealMax;

        GetMonitorMaxArea(pwnd, pMonitorReal, &lprcRealMax);

        /*
         * Is the window a TRUE maximized dude, or somebody like the DOS box
         * who can maximize but not take up the entire screen?
         *
         * Is the window really maximizeable?
         */
        if ((lpmmi->ptMaxSize.x >= (pMonitorPrimary->rcMonitor.right - pMonitorPrimary->rcMonitor.left)) &&
            (lpmmi->ptMaxSize.y >= (pMonitorPrimary->rcMonitor.bottom - pMonitorPrimary->rcMonitor.top))) {

            SetWF(pwnd, WFREALLYMAXIMIZABLE);

            /*
             * Need to reload the checkpoint here, since it might have gotten
             * blown away while we were in the xxxSendMessage call above.
             */
            pcp = (CHECKPOINT *)_GetProp(pwnd, PROP_CHECKPOINT, PROPF_INTERNAL);

            if (    pcp &&
                    pcp->fMaxInitialized &&
                    TestWF(pwnd, WFSIZEBOX) &&
                    (lpmmi->ptMaxPosition.x != rcParent.left) &&
                    (pcp->ptMax.x == lpmmi->ptMaxPosition.x)) {

                /*
                 * If this window has a weird maximize point that doesn't jibe
                 * with what we'd expect and it has a checkpoint, fix up the
                 * checkpoint.  It means that somebody's WINDOWPLACEMENT
                 * got out of date when the size border changed dimensions.
                 */
                pcp->fMaxInitialized = FALSE;

                lpmmi->ptMaxPosition.y += (rcParent.left - lpmmi->ptMaxPosition.x);
                lpmmi->ptMaxPosition.x = rcParent.left;
            }

            /*
             * Transfer the maximum size over to the monitor we are REALLY
             * moving to.  And fix up guys going fullscreen.  A whole bunch
             * of Consumer titles + Word '95 and XL '95 move their caption
             * above the top of the monitor when going fullscreen.  Detect
             * these guys now, and let them take up the monitor.
             */
            if (    lpmmi->ptMaxPosition.y + SYSMET(CYCAPTION) <=
                        pMonitorPrimary->rcMonitor.top
                    &&
                    lpmmi->ptMaxPosition.y + lpmmi->ptMaxSize.y >=
                        pMonitorPrimary->rcMonitor.bottom) {

                lprcRealMax = &pMonitorReal->rcMonitor;
            }

            /*
             * Compensate for the difference between the primary monitor
             * and the monitor we are actually on.
             */
            lpmmi->ptMaxSize.x = lpmmi->ptMaxSize.x -
                (pMonitorPrimary->rcMonitor.right - pMonitorPrimary->rcMonitor.left) +
                (lprcRealMax->right - lprcRealMax->left);

            lpmmi->ptMaxSize.y = lpmmi->ptMaxSize.y -
                (pMonitorPrimary->rcMonitor.bottom - pMonitorPrimary->rcMonitor.top) +
                (lprcRealMax->bottom - lprcRealMax->top);
        } else {
            ClrWF(pwnd, WFREALLYMAXIMIZABLE);
        }

        /*
         * Now transfer the max position over to the monitor we are REALLY
         * moving to.
         */
        lpmmi->ptMaxPosition.x += lprcRealMax->left;
        lpmmi->ptMaxPosition.y += lprcRealMax->top;
    }

    ThreadUnlock(&tlpMonitorPrimary);
    ThreadUnlock(&tlpMonitorReal);

    /*
     * Normal minimum tracking size.
     */

    /*
     * WFCAPTION == WFBORDER | WFDLGFRAME; So, when we want to test for the
     * presence of CAPTION, we must test for both the bits. Otherwise we
     * might mistake WFBORDER or WFDLGFRAME to be a CAPTION.
     *
     *
     * We must not allow a window to be sized smaller than the border
     * thickness -- SANKAR -- 06/12/91 --
     */
    if (TestWF(pwnd, WFCPRESENT)) {

        /*
         * NOTE THAT IF YOU CHANGE THE SPACING OF STUFF IN THE CAPTION,
         * YOU NEED TO KEEP THE FOLLOWING IN SSYNC:
         *      (1) Default CXMINTRACK, CYMINTRACK in inctlpan.c
         *      (2) The default minimum right below
         *      (3) Hit testing
         *
         * The minimum size should be space for:
         *      * The borders
         *      * The buttons
         *      * Margins
         *      * 4 chars of text
         *      * Caption icon
         */
        yMin = SYSMET(CYMINTRACK);

        /*
         * Min track size is determined by the number of buttons in
         * the caption.
         */
        if (TestWF(pwnd, WEFTOOLWINDOW)) {

            /*
             * Add in space for close button.
             */
            if (TestWF(pwnd, WFSYSMENU))
                xMin += SYSMET(CXSMSIZE);

            /*
             * DON'T add in space for 2 characters--breaks
             * MFC toolbar stuff.  They want to make vertical undocked
             * toolbars narrower than what that would produce.
             */
            xMin += (2 * SYSMET(CXEDGE));

        } else {

            if (TestWF(pwnd, WFSYSMENU)) {

                /*
                 * Add in space for min/max/close buttons.  Otherwise,
                 * if it's a contexthelp window, then add in space
                 * for help/close buttons.
                 */
                if (TestWF(pwnd, (WFMINBOX | WFMAXBOX)))
                    xMin += 3 * SYSMET(CXSIZE);
                else if (TestWF(pwnd, WEFCONTEXTHELP))
                    xMin += 2 * SYSMET(CXSIZE);


                /*
                 * Add in space for system menu icon.
                 */
                if (_HasCaptionIcon(pwnd))
                    xMin += SYSMET(CYSIZE);
            }

            /*
             * Add in space for 4 characters and margins.
             */
            xMin += 4 * gcxCaptionFontChar + 2 * SYSMET(CXEDGE);
        }
    }

    lpmmi->ptMinTrackSize.x = max(lpmmi->ptMinTrackSize.x, xMin);
    lpmmi->ptMinTrackSize.y = max(lpmmi->ptMinTrackSize.y, yMin);
}



/***************************************************************************\
* ParkIcon
*
* Called when minimizing a window.  This parks the minwnd in the position
* given in the checkpoint or calculates a new position for it.
*
* LauraBu 10/15/92
* We now let the user specify two things that affect parking and arranging:
*     (1) The corner to start arranging from
*     (2) The direction to move in first
* MCostea 11/13/98  #246397
*   Add sanity check for the number of tries.  If the metrics are messed up
*   and pwnd has a lot of siblings, the for-ever loop would make us timeout
*
\***************************************************************************/

VOID ParkIcon(
    PWND        pwnd,
    PCHECKPOINT pcp)
{
    RECT        rcTest;
    RECT        rcT;
    UINT        xIconPositions;
    UINT        xIconT;
    PWND        pwndTest;
    PWND        pwndParent;
    int         xOrg;
    int         yOrg;
    int         dx;
    int         dy;
    int         dxSlot;
    int         dySlot;
    int         iteration;
    BOOL        fHorizontal;
    PCHECKPOINT pncp;

    /*
     * Put these into local vars immediately.  The compiler is too dumb to
     * know that we're using a constant offset into a constant address, and
     * thus a resulting constant address.
     */
    dxSlot = SYSMET(CXMINSPACING);
    dySlot = SYSMET(CYMINSPACING);

    if (IsTrayWindow(pwnd)) {

        pcp->fMinInitialized = TRUE;
        pcp->ptMin.x         = WHERE_NOONE_CAN_SEE_ME;
        pcp->ptMin.y         = WHERE_NOONE_CAN_SEE_ME;

        return;
    }

    /* We need to adjust the client rectangle for scrollbars, just like we
     * do in ArrangeIconicWindows().  If one thing is clear, it is that
     * parking and arranging must follow the same principles.  This is to
     * avoid the user arranging some windows, creating a new one, and parking
     * it in a place not consistent with the arrangement of the others.
     */
    pwndParent = pwnd->spwndParent;
    GetRealClientRect(pwndParent, &rcT, GRC_SCROLLS, NULL);

    /*
     * Get gravity & move vars.  We want gaps to start on the sides that
     * we begin arranging from.
     *
     * Horizontal gravity
     */
    if (SYSMET(ARRANGE) & ARW_STARTRIGHT) {

        /*
         * Starting on right side
         */
        rcTest.left = xOrg = rcT.right - dxSlot;
        dx = -dxSlot;

    } else {

        /*
         * Starting on left
         */
        rcTest.left = xOrg = rcT.left + DX_GAP;
        dx = dxSlot;
    }

    /*
     * Vertical gravity
     */
    if (SYSMET(ARRANGE) & ARW_STARTTOP) {

        /*
         * Starting on top side
         */
        rcTest.top = yOrg = rcT.top + DY_GAP;
        dy = dySlot;

    } else {

        /*
         * Starting on bottom
         */
        rcTest.top = yOrg = rcT.bottom - dySlot;
        dy = -dySlot;
    }

    /*
     * Get arrangement direction.  Note that ARW_HORIZONTAL is 0, so we
     * can't test for it.
     */
    fHorizontal = ((SYSMET(ARRANGE) & ARW_DOWN) ? FALSE : TRUE);

    if (fHorizontal)
        xIconPositions = xIconT = max(1, (rcT.right / dxSlot));
    else
        xIconPositions = xIconT = max(1, (rcT.bottom / dySlot));

    /*
     * BOGUS
     * LauraBu 10/15/92
     * What happens if the parent is scrolled over horizontally or
     * vertically?  Just like when you drop an object...
     */
    iteration = 0;
    while (iteration < 5000) {

        /*
         * Make a rectangle representing this position, in screen coords
         */
        rcTest.right = rcTest.left + dxSlot;
        rcTest.bottom = rcTest.top + dySlot;

        /*
         * Look for intersections with existing iconic windows
         */
        for (pwndTest = pwndParent->spwndChild; pwndTest; pwndTest = pwndTest->spwndNext) {

            if (!TestWF(pwndTest, WFVISIBLE))
                    continue;

            if (pwndTest == pwnd)
                    continue;

            if (!TestWF(pwndTest, WFMINIMIZED)) {

                /*
                 * This is a non-minimized window.  See if it has a checkpoint
                 * and find out where it would be if it were minimized.  We
                 * will try not to park an icon in this spot.
                 */
                pncp = (PCHECKPOINT)_GetProp(pwndTest,
                                             PROP_CHECKPOINT,
                                             PROPF_INTERNAL);

                if (!pncp || !pncp->fDragged || !pncp->fMinInitialized)
                    continue;

                /*
                 * Get parent coordinates of minimized window pos.
                 */
                rcT.right   = rcT.left = pncp->ptMin.x;
                rcT.right  += dxSlot;
                rcT.bottom  = rcT.top  = pncp->ptMin.y;
                rcT.bottom += dySlot;

            } else {

                /*
                 * Get parent coordinates of currently minimized window
                 */
                GetRect(pwndTest, &rcT, GRECT_WINDOW | GRECT_PARENTCOORDS);
            }

            iteration++;
            /*
             * Get out of loop if they overlap
             */
            if (IntersectRect(&rcT, &rcT, &rcTest))
                break;
        }

        /*
         * Found a position that doesn't overlap, so get out of search loop
         */
        if (!pwndTest)
            break;

        /*
         * Else setup to process the next position
         */
        if (--xIconT == 0) {

            /*
             * Setup next pass
             */
            xIconT = xIconPositions;

            if (fHorizontal) {
                rcTest.left = xOrg;
                rcTest.top += dy;
            } else {
                rcTest.left += dx;
                rcTest.top = yOrg;
            }

        } else {

            /*
             * Same pass.
             */
            if (fHorizontal)
                rcTest.left += dx;
            else
                rcTest.top += dy;
        }
    }

    /*
     * Note that rcTest is in parent coordinates already.
     */
    pcp->fMinInitialized = TRUE;
    pcp->ptMin.x         = rcTest.left;
    pcp->ptMin.y         = rcTest.top;
}

/***************************************************************************\
* xxxAnimateCaption
*
*
\***************************************************************************/

ULONG_PTR SaveScreen(PWND pwnd, ULONG iMode, ULONG_PTR iSave, int x, int y, int cx, int cy)
{
    RECT rc;

    /*
     * x and y are in the DC coordinates, make the screen in the
     * (meta hdev) coordinates for the call to Gre/driver.
     */
    rc.left = x + pwnd->rcWindow.left;
    rc.right = x + cx;
    rc.top = y + pwnd->rcWindow.top;
    rc.bottom = y + cy;

    if (IntersectRect(&rc, &rc, &gpDispInfo->rcScreen)) {
        return GreSaveScreenBits(gpDispInfo->hDev, iMode, iSave, (RECTL*)&rc);
    } else {
        return 0;
    }
}

VOID xxxAnimateCaption(
    PWND   pwnd,
    HDC    hdc,
    LPRECT lprcStart,
    LPRECT lprcEnd)
{
    DWORD        dwTimeStart;
    DWORD        iTimeElapsed;
    int          iLeftStart;
    int          iTopStart;
    int          cxStart;
    int          dLeft;
    int          dTop;
    int          dcx;
    int          iLeft;
    int          iTop;
    int          cx;
    int          iLeftNew;
    int          iTopNew;
    int          cxNew;
    int          cBorders;
    HBITMAP      hbmpOld;
    RECT         rc;
    int          cy;
    HDC          hdcMem;
    ULONG_PTR     uSave;
    PWND         pwndOrg;

    CheckLock(pwnd);

    if ((pwndOrg = _WindowFromDC(hdc)) == NULL) {
        RIPMSG0(RIP_WARNING, "SaveScreen: invalid DC passed in");
        return;
    }

    cy = SYSMET(CYCAPTION) - 1;

    /*
     *  kurtp: 29-Jan-1997
     *
     *  We don't do anything when animating the caption,
     *  because we couldn't get the desired effect at the
     *  client.  If we do use it then the
     *  cache gets a bunch of bitmaps (size: 2xCaption by CXScreen)
     *  that are never re-used.  This slows down clients
     *  because the GreBitBlts always generate new bitmaps
     *  and the cache is displaced by the new bitmaps (yuk!).
     */

    if (gbRemoteSession)
        return;

    if ((hdcMem = GreCreateCompatibleDC(ghdcMem)) == NULL)
        return;

    /*
     * If the caption strip doesn't exist, then attempt to recreate it.  This
     * might be necessary if the user does a mode-switch during low memory
     * and is not able to recreate the surface.  When the memory becomes
     * available, we'll attempt to recreate it here.
     */
    if (ghbmCaption == NULL) {
        ghbmCaption = CreateCaptionStrip();
    }

    hbmpOld = GreSelectBitmap(hdcMem, ghbmCaption);

    /*
     * initialize start values
     */
    iTopStart  = lprcStart->top;
    iLeftStart = lprcStart->left;
    cxStart    = lprcStart->right - iLeftStart;

    /*
     * initialize delta values to the destination dimensions
     */
    dLeft  = lprcEnd->left;
    dTop   = lprcEnd->top;
    dcx    = lprcEnd->right - dLeft;

    /*
     * adjust for window borders as appropriate
     */
    cBorders = GetWindowBorders(pwnd->style,
                                pwnd->ExStyle,
                                TRUE,
                                FALSE);

    if ((lprcStart->bottom - iTopStart) > SYSMET(CYCAPTION)) {

        iLeftStart += cBorders;
        iTopStart  += cBorders;
        cxStart    -= 2*cBorders;
    }

    if ((lprcEnd->bottom - dTop) > SYSMET(CYCAPTION)) {

        dLeft += cBorders;
        dTop  += cBorders;
        dcx   -= 2*cBorders;
    }

    /*
     * initialize step values
     */
    iLeft = iLeftStart;
    iTop  = iTopStart;
    cx    = cxStart;

    /*
     * initialize off screen bitmap with caption drawing and first saved rect
     */
    rc.left   = 0;
    rc.top    = cy;
    rc.right  = max(cxStart, dcx);
    rc.bottom = cy * 2;

    xxxDrawCaptionTemp(pwnd,
                       hdcMem,
                       &rc,
                       NULL,
                       NULL,
                       NULL,
                       DC_ACTIVE | DC_ICON | DC_TEXT |
                       (TestALPHA(GRADIENTCAPTIONS) ? DC_GRADIENT : 0));

    if ((uSave = SaveScreen(pwndOrg, SS_SAVE, 0,iLeft, iTop, cx, cy)) == 0) {
        if (!GreBitBlt(hdcMem,
                  0,
                  0,
                  cx,
                  cy,
                  hdc,
                  iLeft,
                  iTop,
                  SRCCOPY,
                  0)) {
            goto Cleanup;
        }
    }

    /*
     * compute delta values by subtracting source dimensions
     */
    dLeft -= iLeftStart;
    dTop  -= iTopStart;
    dcx   -= cxStart;

    /*
     * blt and time first caption on screen
     * WARNING: If you use *lpSystemTickCount here,
     * the compiler may not generate code to do a DWORD fetch;
     */
    dwTimeStart = NtGetTickCount();
    GreBitBlt(hdc,
              iLeft,
              iTop,
              cx,
              cy,
              hdcMem,
              0,
              cy,
              SRCCOPY,
              0);

    iTimeElapsed = (NtGetTickCount() - dwTimeStart);

    while (LOWORD(iTimeElapsed) <= CMS_ANIMATION) {

        iLeftNew = iLeftStart + MultDiv(dLeft, LOWORD(iTimeElapsed), CMS_ANIMATION);
        iTopNew  = iTopStart  + MultDiv(dTop,  LOWORD(iTimeElapsed), CMS_ANIMATION);
        cxNew    = cxStart    + MultDiv(dcx,   LOWORD(iTimeElapsed), CMS_ANIMATION);

        /*
         * Delay before next frame
         */
        UserSleep(1);

        /*
         * restore saved rect
         */
        if (uSave != 0) {
            SaveScreen(pwndOrg, SS_RESTORE, uSave, iLeft, iTop, cx, cy);
        } else {
            GreBitBlt(hdc,
                      iLeft,
                      iTop,
                      cx,
                      cy,
                      hdcMem,
                      0,
                      0,
                      SRCCOPY,
                      0);
        }

        iLeft = iLeftNew;
        iTop  = iTopNew;
        cx    = cxNew;

        /*
         * save new rect offscreen and then draw over it onscreen.
         */
        if (uSave != 0) {
            uSave = SaveScreen(pwndOrg, SS_SAVE, 0, iLeft, iTop, cx, cy);
        } else {
            GreBitBlt(hdcMem,
                      0,
                      0,
                      cx,
                      cy,
                      hdc,
                      iLeft,
                      iTop,
                      SRCCOPY,
                      0);
        }
        GreBitBlt(hdc,
                  iLeft,
                  iTop,
                  cx,
                  cy,
                  hdcMem,
                  0,
                  cy,
                  SRCCOPY,
                  0);

        /*
         * update elapsed time
         * WARNING: If you use *lpSystemTickCount here,
         * the compiler may not generate code to do a DWORD fetch;
         */
        iTimeElapsed = (NtGetTickCount() - dwTimeStart);
    }

    /*
     * restore saved rect
     */
    if (uSave != 0) {
        SaveScreen(pwndOrg, SS_RESTORE, uSave, iLeft, iTop, cx, cy);
    } else {
        GreBitBlt(hdc,
                  iLeft,
                  iTop,
                  cx,
                  cy,
                  hdcMem,
                  0,
                  0,
                  SRCCOPY,
                  0);
    }

Cleanup:
    GreSelectBitmap(hdcMem, hbmpOld);
    GreDeleteDC(hdcMem);
}

#if 0 // DISABLE OLD ANIMATION FOR M7
/***************************************************************************\
* DrawWireFrame
*
* Draws wire frame trapezoid
*
*
\***************************************************************************/

VOID DrawWireFrame(
    HDC    hdc,
    LPRECT prcFront,
    LPRECT prcBack)
{
    RECT rcFront;
    RECT rcBack;
    RECT rcT;
    HRGN hrgnSave;
    BOOL fClip;

    /*
     * Save these locally
     */
    CopyRect(&rcFront, prcFront);
    CopyRect(&rcBack, prcBack);

    /*
     * Front face
     */
    GreMoveTo(hdc, rcFront.left, rcFront.top);
    GreLineTo(hdc, rcFront.left, rcFront.bottom);
    GreLineTo(hdc, rcFront.right, rcFront.bottom);
    GreLineTo(hdc, rcFront.right, rcFront.top);
    GreLineTo(hdc, rcFront.left, rcFront.top);

    /*
     * Exclude front face from clipping area, only if back face isn't
     * entirely within interior.  We need variable because SaveClipRgn()
     * can return NULL.
     */
    fClip = (EqualRect(&rcFront, &rcBack)            ||
             !IntersectRect(&rcT, &rcFront, &rcBack) ||
             !EqualRect(&rcT, &rcBack));

    if (fClip) {

        hrgnSave = GreSaveClipRgn(hdc);

        GreExcludeClipRect(hdc,
                           rcFront.left,
                           rcFront.top,
                           rcFront.right,
                           rcFront.bottom);
    }

    /*
     * Edges
     */
    GreMoveTo(hdc, rcBack.left, rcBack.top);
    LineTo(hdc, rcFront.left, rcFront.top);

    GreMoveTo(hdc, rcBack.right, rcBack.top);
    GreLineTo(hdc, rcFront.right, rcFront.top);

    GreMoveTo(hdc, rcBack.right, rcBack.bottom);
    GreLineTo(hdc, rcFront.right, rcFront.bottom);

    GreMoveTo(hdc, rcBack.left, rcBack.bottom);
    GreLineTo(hdc, rcFront.left, rcFront.bottom);

    /*
     * Back face
     */
    MoveTo(hdc, rcBack.left, rcBack.top);
    LineTo(hdc, rcBack.left, rcBack.bottom);
    LineTo(hdc, rcBack.right, rcBack.bottom);
    LineTo(hdc, rcBack.right, rcBack.top);
    LineTo(hdc, rcBack.left, rcBack.top);

    if (fClip)
        GreRestoreClipRgn(hdc, hrgnSave);
}

/***************************************************************************\
* AnimateFrame
*
* Draws wire frame 3D trapezoid
*
*
\***************************************************************************/

VOID AnimateFrame(
    HDC    hdc,
    LPRECT prcStart,
    LPRECT prcEnd,
    BOOL   fGrowing)
{
    RECT  rcBack;
    RECT  rcFront;
    RECT  rcT;
    HPEN  hpen;
    int   nMode;
    int   iTrans;
    int   nTrans;
    DWORD dwTimeStart;
    DWORD dwTimeCur;

    /*
     * Get pen for drawing lines
     */
    hpen = GreSelectPen(hdc, GetStockObject(WHITE_PEN));
    nMode = GreSetROP2(hdc, R2_XORPEN);

    /*
     * Save these locally
     */
    if (fGrowing) {

        CopyRect(&rcBack, prcStart);
        CopyRect(&rcFront, prcStart);

    } else {

       /*
        * Initial is trapezoid entire way from small to big.  We're going
        * to shrink it from the front face.
        */
       CopyRect(&rcFront, prcStart);
       CopyRect(&rcBack, prcEnd);
    }

    /*
     * Offset left & top edges of rects, due to way that lines work.
     */
    rcFront.left -= 1;
    rcFront.top  -= 1;
    rcBack.left  -= 1;
    rcBack.top   -= 1;

    /*
     * Get tick count.  We'll draw then check how much time elapsed.  From
     * that we can calculate how many more transitions to draw.  For the first
     * We basically want whole animation to last 3/4 of a second, or 750
     * milliseconds.
     *
     * WARNING: If you use *lpSystemTickCount here,
     * the compiler may not generate code to do a DWORD fetch;
     */
    dwTimeStart = GetSystemMsecCount();

    DrawWireFrame(hdc, &rcFront, &rcBack);

    /*
     * WARNING: If you use *lpSystemTickCount here,
     * the compiler may not generate code to do a DWORD fetch;
     */
    dwTimeCur = GetSystemMsecCount();

    /*
     * Get rough estimate for how much time it took.
     */
    if (dwTimeCur == dwTimeStart)
        nTrans = CMS_ANIMATION / 55;
    else
        nTrans = CMS_ANIMATION / ((int)(dwTimeCur - dwTimeStart));

    iTrans = 1;
    while (iTrans <= nTrans) {

        /*
         * Grow the trapezoid out or shrink it in.  Fortunately, prcStart
         * and prcEnd are already set up for us.
         */
        rcT.left = prcStart->left +
            MultDiv(prcEnd->left - prcStart->left, iTrans, nTrans);
        rcT.top = prcStart->top +
            MultDiv(prcEnd->top - prcStart->top, iTrans, nTrans);
        rcT.right = prcStart->right +
            MultDiv(prcEnd->right - prcStart->right, iTrans, nTrans);
        rcT.bottom = prcStart->bottom +
            MultDiv(prcEnd->bottom - prcStart->bottom, iTrans, nTrans);

        /*
         * Undraw old and draw new
         */
        DrawWireFrame(hdc, &rcFront, &rcBack);
        CopyRect(&rcFront, &rcT);
        DrawWireFrame(hdc, &rcFront, &rcBack);

        /*
         * Check the time.  How many more transitions left?
         *  iTrans / nTrans AS (dwTimeCur-dwTimeStart) / 750
         *
         * WARNING: If you use *lpSystemTickCount here,
         * the compiler may not generate code to do a DWORD fetch;
         */
        dwTimeCur = GetSystemMsecCount();
        iTrans = MultDiv(nTrans,
                         (int)(dwTimeCur - dwTimeStart),
                         CMS_ANIMATION);
    }

    /*
     * Undraw wire frame
     */
    DrawWireFrame(hdc, &rcFront, &rcBack);

    /*
     * Clean up
     */
    GreSetROP2(hdc, nMode);
    hpen = GreSelectPen(hdc, hpen);
}
#endif // END DISABLE OLD ANIMATION FOR M7

/***************************************************************************\
* xxxDrawAnimatedRects
*
* General routine, like PlaySoundEvent(), that calls other routines for
* various animation effects.  Currently used for changing state from/to
* minimized.
*
\***************************************************************************/

BOOL xxxDrawAnimatedRects(
    PWND   pwndClip,
    int    idAnimation,
    LPRECT lprcStart,
    LPRECT lprcEnd)
{
    HDC   hdc;
    POINT rgPt[4];
    RECT  rcClip;
    HRGN  hrgn;
    PWND  pwndAnimate = NULL;
    int   iPt;

    CheckLock(pwndClip);

    /*
     * Get rects into variables
     */
    CopyRect((LPRECT)&rgPt[0], lprcStart);
    CopyRect((LPRECT)&rgPt[2], lprcEnd);

    /*
     * DISABLE OLD ANIMATION FOR M7
     */
    if (idAnimation != IDANI_CAPTION)
        return TRUE;

    pwndAnimate = pwndClip;
    if (!pwndAnimate || pwndAnimate == PWNDDESKTOP(pwndAnimate))
        return FALSE;

    pwndClip = pwndClip->spwndParent;
    if (!pwndClip) {
        RIPMSG0(RIP_WARNING, "xxxDrawAnimatedRects: pwndClip->spwndParent is NULL");
    } else if (pwndClip == PWNDDESKTOP(pwndClip)) {
        pwndClip = NULL;
    }

    /*
     * NOTE:
     * We do NOT need to do LockWindowUpdate().  We never yield within this
     * function!  Anything that was invalid will stay invalid, etc.  So our
     * XOR drawing won't leave remnants around.
     *
     * WIN32NT may need to take display critical section or do LWU().
     *
     * Get clipping area
     * Neat feature:
     *      NULL window means whole screen, don't clip out children
     *      hwndDesktop means working area, don't clip out children
     */
    if (pwndClip == NULL) {
        pwndClip = _GetDesktopWindow();
        CopyRect(&rcClip, &pwndClip->rcClient);
        if ((hrgn = GreCreateRectRgnIndirect(&rcClip)) == NULL) {
            hrgn = HRGN_FULL;
        }

        /*
         * Get drawing DC
         */
        hdc = _GetDCEx(pwndClip,
                       hrgn,
                       DCX_WINDOW           |
                           DCX_CACHE        |
                           DCX_INTERSECTRGN |
                           DCX_LOCKWINDOWUPDATE);
    } else {

        hdc = _GetDCEx(pwndClip,
                       HRGN_FULL,
                       DCX_WINDOW | DCX_USESTYLE | DCX_INTERSECTRGN);

        /*
         * We now have a window DC.  We need to convert client coords
         * to window coords.
         */
        for (iPt = 0; iPt < 4; iPt++) {

            rgPt[iPt].x += (pwndClip->rcClient.left - pwndClip->rcWindow.left);
            rgPt[iPt].y += (pwndClip->rcClient.top - pwndClip->rcWindow.top);
        }
    }

    /*
     * Get drawing DC:
     * Unclipped if desktop, clipped otherwise.
     * Note that ReleaseDC() will free the region if needed.
     */
    if (idAnimation == IDANI_CAPTION) {
        CheckLock(pwndAnimate);
        xxxAnimateCaption(pwndAnimate, hdc, (LPRECT)&rgPt[0], (LPRECT)&rgPt[2]);
    }

/*
 * DISABLE OLD ANIMATION FOR M7
 */
#if 0
    else {
        AnimateFrame(hdc,
                     (LPRECT)&rgPt[0],
                     (LPRECT)&rgPt[2],
                     (idAnimation == IDANI_OPEN));
    }
#endif
/*
 * END DISABLE OLD ANIMATION FOR M7
 */

    /*
     * Clean up
     */
    _ReleaseDC(hdc);

    return TRUE;
}


/***************************************************************************\
* CalcMinZOrder
*
*
* Compute the Z-order of a window to be minimized.
*
* The strategy is to find the bottom-most sibling of pwndMinimize that
* shares the same owner, and insert ourself behind that.  We must also
* take into account that a TOPMOST window should stay among other TOPMOST,
* and vice versa.
*
* We must make sure never to insert after a bottom-most window.
*
* This code works for child windows too, since they don't have owners
* and never have WEFTOPMOST set.
*
* If NULL is returned, the window shouldn't be Z-ordered.
*
\***************************************************************************/

PWND CalcMinZOrder(
    PWND pwndMinimize)
{
    BYTE bTopmost;
    PWND pwndAfter;
    PWND pwnd;

    bTopmost = TestWF(pwndMinimize, WEFTOPMOST);
    pwndAfter = NULL;

    for (pwnd = pwndMinimize->spwndNext; pwnd && !TestWF(pwnd, WFBOTTOMMOST); pwnd = pwnd->spwndNext) {

        /*
         * If we've enumerated a window that isn't the same topmost-wise
         * as pwndMinimize, we've gone as far as we can.
         */
        if (TestWF(pwnd, WEFTOPMOST) != bTopmost)
            break;

        if (pwnd->spwndOwner == pwndMinimize->spwndOwner)
            pwndAfter = pwnd;
    }

    return pwndAfter;
}

/***************************************************************************\
* xxxActivateOnMinimize
*
* Activate the previously active window, provided that window still exists
* and is a NORMAL window (not bottomost, minimized, disabled, or invisible).
* If it's not NORMAL, then activate the first non WS_EX_TOPMOST window
* that's normal. Return TRUE when no activation is needed or the activation
* has been done in this function. Return FALSE if failed to find a window
* to activate.
*
\***************************************************************************/

BOOL xxxActivateOnMinimize(PWND pwnd)
{
    PTHREADINFO ptiCurrent = PtiCurrent();
    PWND pwndStart, pwndFirstTool, pwndT;
    BOOL fTryTopmost = TRUE;
    BOOL fPrevCheck = (ptiCurrent->pq->spwndActivePrev != NULL);
    TL tlpwndT;

    /*
     * We should always have a last-topmost window.
     */
    pwndStart = GetLastTopMostWindow();
    if (pwndStart) {
        pwndStart = pwndStart->spwndNext;
    } else {
        pwndStart = pwnd->spwndParent->spwndChild;
    }

    UserAssert(HIBYTE(WFMINIMIZED) == HIBYTE(WFVISIBLE));
    UserAssert(HIBYTE(WFVISIBLE) == HIBYTE(WFDISABLED));

SearchAgain:

    pwndT = (fPrevCheck ? ptiCurrent->pq->spwndActivePrev : pwndStart);
    pwndFirstTool = NULL;

    for ( ; pwndT ; pwndT = pwndT->spwndNext) {

TryThisWindow:

        /*
         * Use the first nonminimized, visible, nondisabled, and
         * nonbottommost window
         */
        if (!HMIsMarkDestroy(pwndT) &&
            !TestWF(pwndT, WEFNOACTIVATE) &&
            (TestWF(pwndT, WFVISIBLE | WFDISABLED) == LOBYTE(WFVISIBLE)) &&
            (!TestWF(pwndT, WFMINIMIZED) || GetFullScreen(pwndT) == FULLSCREEN)) {

            if (TestWF(pwndT, WEFTOOLWINDOW)) {
                if (!pwndFirstTool) {
                    pwndFirstTool = pwndT;
                }
            } else {
                break;
            }
        }

        if (fPrevCheck) {
            fPrevCheck = FALSE;
            pwndT = pwndStart;
            goto TryThisWindow;
        }
    }

    if (!pwndT) {

        if (fTryTopmost) {

            fTryTopmost = FALSE;
            if (pwndStart != NULL) {
                pwndStart = pwndStart->spwndParent->spwndChild;
            } else {
                PWND pwndDesktop = _GetDesktopWindow();
                pwndStart = (pwndDesktop != NULL) ? pwndDesktop->spwndChild : NULL;
            }
            goto SearchAgain;
        }

        pwndT = pwndFirstTool;
    }

    if (pwndT) {
        ThreadLockAlwaysWithPti(ptiCurrent, pwndT, &tlpwndT);
        xxxSetForegroundWindow(pwndT, FALSE);
        ThreadUnlock(&tlpwndT);
    } else {
        return FALSE;
    }

    return TRUE;
}



/***************************************************************************\
* xxxMinMaximize
*
* cmd = SW_MINIMIZE, SW_SHOWMINNOACTIVE, SW_SHOWMINIZED,
*     SW_SHOWMAXIMIZED, SW_SHOWNOACTIVE, SW_NORMAL
*
* If MINMAX_KEEPHIDDEN is set in dwFlags, keep it hidden, otherwise show it.
*    This is always cleared, except in the case we call it from
*    createwindow(), where the wnd is iconic, but hidden.  we
*    need to call this func, to set it up correctly so that when
*    the app shows the wnd, it is displayed correctly.
*
* When changing state, we always add on SWP_STATECHANGE.  This lets
* SetWindowPos() know to always send WM_WINDOWPOSCHANGING/CHANGED messages
* even if the new size is the same as the old size.  This is because
* apps watch the WM_SIZE wParam field to see when they are changing state.
* If SWP doesn't send WM_WINDOWPOSCHANGED, then they won't get a WM_SIZE
* message at all.
*
* Furthermore, when changing state to/from maximized, if we are really
* maximizing and are in multiple monitor mode, we want to set the window's
* region so that it can't draw outside of the monitor.  Otherwise, it
* will spill over onto another.  The borders are really annoying.
*
\***************************************************************************/

PWND xxxMinMaximize(
    PWND pwnd,
    UINT cmd,
    DWORD dwFlags)
{
    RECT        rc;
    RECT        rcWindow;
    RECT        rcRestore;
    BOOL        fShow = FALSE;
    BOOL        fSetFocus = FALSE;
    BOOL        fShowOwned = FALSE;
    BOOL        fSendActivate = FALSE;
    BOOL        fMaxStateChanging = FALSE;
    int         idAnimation = 0;
    BOOL        fFlushPalette = FALSE;
    UINT        swpFlags = 0;
    HWND        hwndAfter = NULL;
    PWND        pwndT;
    PCHECKPOINT pcp;
    PTHREADINFO ptiCurrent;
    TL          tlpwndParent;
    TL          tlpwndT;
    PSMWP       psmwp;
    BOOL        fIsTrayWindowNow = FALSE;
    NTSTATUS    Status;
    MINMAXINFO  mmi;
    UINT        uEvent = 0;
#if defined(USE_MIRRORING)
    PWND    pwndParent = pwnd->spwndParent;
    BOOL    bMirroredParent=FALSE;
#endif

    CheckLock(pwnd);

    /*
     * Get window rect, in parent client coordinates.
     */
    GetRect(pwnd, &rcWindow, GRECT_WINDOW | GRECT_PARENTCOORDS);

    /*
     * If this is NULL, we're out of memory, so punt now.
     */
    pcp = CkptRestore(pwnd, &rcWindow);
    if (!pcp)
        goto Exit;

#if defined(USE_MIRRORING)
    /*
     * If this top-level window is placed in a mirrored desktop,
     * its coordinates should be mirrored here so that xxxAnimateCaptions
     * works properly, however we shouldn't change the actual screen coordinates
     * of the window. This is why I do it after CkptRestore(...). [samera]
     */
    if (TestWF(pwndParent,WEFLAYOUTRTL) &&
            (!TestWF(pwnd,WFCHILD))) {
        int iLeft = rcWindow.left;
        rcWindow.left  = pwndParent->rcWindow.right - rcWindow.right;
        rcWindow.right = pwndParent->rcWindow.right - iLeft;
        bMirroredParent = TRUE;
    }
#endif


    /*
     * Save the previous restore size.
     */
    CopyRect(&rcRestore, &pcp->rcNormal);

    /*
     * First ask the CBT hook if we can do this operation.
     */
    if (    IsHooked(PtiCurrent(), WHF_CBT) &&
            xxxCallHook(HCBT_MINMAX, (WPARAM)HWq(pwnd), (DWORD)cmd, WH_CBT)) {

        goto Exit;
    }

    /*
     * If another MDI window is being maximized, and we want to restore this
     * one to its previous state, we can't change the zorder or the
     * activation.  We'd screw things up that way.  BTW, this SW_ value is
     * internal.
     */
    if (cmd == SW_MDIRESTORE) {

        swpFlags |= SWP_NOZORDER | SWP_NOACTIVATE;

        cmd = (pcp->fWasMinimizedBeforeMaximized ?
                SW_SHOWMINIMIZED : SW_SHOWNORMAL);
    }

    ptiCurrent = PtiCurrent();

    switch (cmd) {
    case SW_MINIMIZE:        // Bottom of zorder, make top-level active
    case SW_SHOWMINNOACTIVE: // Bottom of zorder, don't change activation

        if (gpqForeground && gpqForeground->spwndActive)
            swpFlags |= SWP_NOACTIVATE;

        if ((pwndT = CalcMinZOrder(pwnd)) == NULL) {
            swpFlags |= SWP_NOZORDER;
        } else {
            hwndAfter = PtoHq(pwndT);
        }


        /*
         * FALL THRU
         */

    case SW_SHOWMINIMIZED:   // Top of zorder, make active

        /*
         * Force a show.
         */
        fShow = TRUE;

        /*
         * If already minimized, then don't change the existing
         * parking spot.
         */
        if (TestWF(pwnd, WFMINIMIZED)) {

            /*
             * If we're already minimized and we're properly visible
             * or not visible, don't do anything
             */
            if (TestWF(pwnd, WFVISIBLE))
                return NULL;

            swpFlags |= SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE;

            goto Showit;
        }

        /*
         * We're becoming minimized although we currently are not.  So
         * we want to draw the transition animation, and ALWAYS send
         * sizing messages.
         */
        idAnimation = IDANI_CLOSE;

        if (!pcp->fDragged)
            pcp->fMinInitialized = FALSE;

        if (!pcp->fMinInitialized)
            ParkIcon(pwnd, pcp);

        rc.left   = pcp->ptMin.x;
        rc.top    = pcp->ptMin.y;
        rc.right  = pcp->ptMin.x + SYSMET(CXMINIMIZED);
        rc.bottom = pcp->ptMin.y + SYSMET(CYMINIMIZED);

        xxxShowOwnedWindows(pwnd, SW_PARENTCLOSING, NULL);

        pwndT = ptiCurrent->pq->spwndFocus;

        while (pwndT) {

            /*
             * if we or any child has the focus, punt it away
             */
            if (pwndT != pwnd) {
                pwndT = pwndT->spwndParent;
                continue;
            }

            ThreadLockAlwaysWithPti(ptiCurrent, pwndT, &tlpwndT);

            if (TestwndChild(pwnd)) {

                ThreadLockWithPti(ptiCurrent, pwnd->spwndParent, &tlpwndParent);
                xxxSetFocus(pwnd->spwndParent);
                ThreadUnlock(&tlpwndParent);

            } else {
                xxxSetFocus(NULL);
            }

            ThreadUnlock(&tlpwndT);
            break;
        }

        /*
         * Save the maximized state so that we can restore the window maxed
         */
        if (TestWF(pwnd, WFMAXIMIZED)) {
            pcp->fWasMaximizedBeforeMinimized = TRUE;
            fMaxStateChanging = TRUE;
        } else{
            pcp->fWasMaximizedBeforeMinimized = FALSE;
        }

        if (!TestWF(pwnd, WFWIN40COMPAT))
            fIsTrayWindowNow = IsTrayWindow(pwnd);

        /*
         * Decrement the visible-windows count only if the
         * window is visible.  If the window is marked for
         * destruction, we will not decrement for that as
         * well. Let SetMinimize take care of this.
         */
        SetMinimize(pwnd, SMIN_SET);
        ClrWF(pwnd, WFMAXIMIZED);

        uEvent = EVENT_SYSTEM_MINIMIZESTART;

        if (!TestWF(pwnd, WFWIN40COMPAT))
            fIsTrayWindowNow = (fIsTrayWindowNow != IsTrayWindow(pwnd));

        /*
         * The children of this window are now no longer visible.
         * Ensure that they no longer have any update regions...
         */
        for (pwndT = pwnd->spwndChild; pwndT; pwndT = pwndT->spwndNext)
            ClrFTrueVis(pwndT);

        /*
         * B#2919
         * Ensure that the client area gets recomputed, and make
         * sure that no bits are copied when the size is changed.  And
         * make sure that WM_SIZE messages get sent, even if our client
         * size is staying the same.
         */
        swpFlags |= (SWP_DRAWFRAME | SWP_NOCOPYBITS | SWP_STATECHANGE);

        /*
         * We are going minimized, so we want to give palette focus to
         * another app.
         */
        if (pwnd->spwndParent == PWNDDESKTOP(pwnd)) {
            fFlushPalette = (BOOL)TestWF(pwnd, WFHASPALETTE);
        }

        break;

    case SW_SHOWNOACTIVATE:
        if (gpqForeground && gpqForeground->spwndActive)
            swpFlags |= SWP_NOACTIVATE;

        /*
         * FALL THRU
         */

    case SW_RESTORE:

        /*
         * If restoring a minimized window that was maximized before
         * being minimized, go back to being maximized.
         */
        if (TestWF(pwnd, WFMINIMIZED) && pcp->fWasMaximizedBeforeMinimized)
            cmd = SW_SHOWMAXIMIZED;
        else
            cmd = SW_NORMAL;

        /*
         * FALL THRU
         */

    case SW_NORMAL:
    case SW_SHOWMAXIMIZED:

        if (cmd == SW_SHOWMAXIMIZED) {

            /*
             * If already maximized and visible, we have nothing to do
             * Otherwise, for the DOSbox, still set fMaxStateChanging
             * to TRUE so we recalc the monitor region if need be.
             * That way WinOldAp can change its "changing from maxed to
             * maxed with new bigger font" code to work right.
             */
            if (TestWF(pwnd, WFMAXIMIZED)) {
                if (TestWF(pwnd, WFVISIBLE)) {
                    return NULL;
                }
            } else {
                /*
                 * We're changing from normal to maximized, so always
                 * send WM_SIZE.
                 */
                swpFlags |= SWP_STATECHANGE;
            }
            fMaxStateChanging = TRUE;

            /*
             * If calling from CreateWindow, don't let the thing become
             * activated by the SWP call below.  Acitvation will happen
             * on the ShowWindow done by CreateWindow or the app.
             */
            if (dwFlags & MINMAX_KEEPHIDDEN)
                swpFlags |= SWP_NOACTIVATE;

            /*
             * This is for MDI's auto-restore behaviour (craigc)
             */
            if (TestWF(pwnd, WFMINIMIZED))
                pcp->fWasMinimizedBeforeMaximized = TRUE;

            xxxInitSendValidateMinMaxInfo(pwnd, &mmi);

        } else {

            /*
             * We're changing state from non-normal to normal.  Make
             * sure WM_SIZE gets sents.
             */
            UserAssert(HIBYTE(WFMINIMIZED) == HIBYTE(WFMAXIMIZED));
            if (TestWF(pwnd, WFMINIMIZED | WFMAXIMIZED)) {
                swpFlags |= SWP_STATECHANGE;
            }
            if (TestWF(pwnd, WFMAXIMIZED)) {
                fMaxStateChanging = TRUE;
            }
        }

        /*
         * If currently minimized, show windows' popups
         */
        if (TestWF(pwnd, WFMINIMIZED)) {

            /*
             * Send WM_QUERYOPEN to make sure this guy should unminimize
             */
            if (!xxxSendMessage(pwnd, WM_QUERYOPEN, 0, 0L))
                return NULL;

            idAnimation = IDANI_OPEN;
            fShowOwned  = TRUE;
            fSetFocus   = TRUE;

            /*
             * JEFFBOG B#2868
             * Condition added before setting fSendActivate prevents
             * WM_ACTIVATE message from reaching a child window.  Might
             * be backwards compatibility problems if a pre 3.1 app
             * relies on WM_ACTIVATE reaching a child.
             */
            if (!TestWF(pwnd, WFCHILD))
                fSendActivate = TRUE;

            swpFlags |= SWP_NOCOPYBITS;
        } else {
            idAnimation = IDANI_CAPTION;
        }

        if (cmd == SW_SHOWMAXIMIZED) {
            rc.left     = mmi.ptMaxPosition.x;
            rc.top      = mmi.ptMaxPosition.y;
            rc.right    = rc.left + mmi.ptMaxSize.x;
            rc.bottom   = rc.top + mmi.ptMaxSize.y;

            SetWF(pwnd, WFMAXIMIZED);

        } else {
            CopyRect(&rc, &rcRestore);
            ClrWF(pwnd, WFMAXIMIZED);
        }

        /*
         * We do this TestWF again since we left the critical section
         * above and someone might have already 'un-minimized us'.
         */
        if (TestWF(pwnd, WFMINIMIZED)) {

            if (!TestWF(pwnd, WFWIN40COMPAT))
                fIsTrayWindowNow = IsTrayWindow(pwnd);

            /*
             * Mark it as minimized and adjust cVisWindows.
             */
            SetMinimize(pwnd, SMIN_CLEAR);

            uEvent = EVENT_SYSTEM_MINIMIZEEND;

            /*
             * if we're unminimizing a window that is now
             * not seen in maximized/restore mode then remove him
             * from the tray
             */
            if (!TestWF(pwnd, WFWIN40COMPAT)             &&
                (fIsTrayWindowNow != IsTrayWindow(pwnd)) &&
                FDoTray()) {

                HWND hw = HWq(pwnd);

                if (FCallHookTray()) {
                    xxxCallHook(HSHELL_WINDOWDESTROYED,
                                (WPARAM)hw,
                                (LPARAM)0,
                                WH_SHELL);
                }

                /*
                 * NT specific code.  Post the window-destroyed message
                 * to the shell.
                 */
                if (FPostTray(pwnd->head.rpdesk))
                    PostShellHookMessages(HSHELL_WINDOWDESTROYED, (LPARAM)hw);
            }

            fIsTrayWindowNow = FALSE;

            /*
             * If we're un-minimizing a visible top-level window, cVisWindows
             * was zero, and we're either activating a window or showing
             * the currently active window, set ourselves into the
             * foreground.  If the window isn't currently visible
             * then we can rely on SetWindowPos() to do the right
             * thing for us.
             */
            if (!TestwndChild(pwnd)                 &&
                TestWF(pwnd, WFVISIBLE)             &&
                (GETPTI(pwnd)->cVisWindows == 1)    &&
                (GETPTI(pwnd)->pq != gpqForeground) &&
                (!(swpFlags & SWP_NOACTIVATE)
                    || (GETPTI(pwnd)->pq->spwndActive == pwnd))) {

                xxxSetForegroundWindow2(pwnd, GETPTI(pwnd), SFW_STARTUP);
            }
        }

        /*
         * Ensure that client area gets recomputed, and that
         * the frame gets redrawn to reflect the new state.
         */
        swpFlags |= SWP_DRAWFRAME;
        break;
    }

    /*
     * For the iconic case, we need to also show the window because it
     * might not be visible yet.
     */

Showit:

    if (!(dwFlags & MINMAX_KEEPHIDDEN)) {

        if (TestWF(pwnd, WFVISIBLE)) {

            if (fShow)
                swpFlags |= SWP_SHOWWINDOW;

            /* if we're full screening a DOS BOX then don't draw
             * the animation 'cause it looks bad.
             * overloaded WFFULLSCREEN bit for MDI child windows --
             * use it to indicate to not animate size change.
             */
            if (IsVisible(pwnd)            &&
                (dwFlags & MINMAX_ANIMATE) &&
                idAnimation                &&
                (!TestWF(pwnd, WFCHILD) || !TestWF(pwnd, WFNOANIMATE))) {

#if defined(USE_MIRRORING)
                /*
                 * If this top-level window is placed in a mirrored desktop,
                 * its coordinates should be mirrored here so that xxxAnimateCaptions
                 * works properly, however we shouldn't change the actual screen coordinates
                 * of the window. This is why I do it here and restore it afterwards before
                 * doing the _DeferWindowPos(...). [samera]
                 */
                 RECT rcT;
                 if (bMirroredParent) {
                     int iLeft = rc.left;
                     rcT = rc;
                     rc.left  = pwndParent->rcWindow.right - rc.right;
                     rc.right = pwndParent->rcWindow.right - iLeft;
                 }
#endif

                if ((idAnimation != IDANI_CAPTION) && IsTrayWindow(pwnd)) {

                    RECT rcMin;

                    SetRectEmpty(&rcMin);
#if 0 // Win95 call.
                    CallHook(HSHELL_GETMINRECT, (WPARAM)HW16(hwnd), (LPARAM)(LPRECT)&rcMin, WH_SHELL);
#else
                    xxxSendMinRectMessages(pwnd, &rcMin);
#endif

                    if (!IsRectEmpty(&rcMin)) {

                        if (idAnimation == IDANI_CLOSE) {

                            xxxDrawAnimatedRects(pwnd,
                                                  IDANI_CAPTION,
                                                  &rcWindow,
                                                  &rcMin);

                        } else {

                            xxxDrawAnimatedRects(pwnd,
                                                  IDANI_CAPTION,
                                                  &rcMin,
                                                  &rc);
                        }
                    }

                } else {
                    xxxDrawAnimatedRects(pwnd, IDANI_CAPTION, &rcWindow, &rc);
                }

#if defined(USE_MIRRORING)
                    /*
                     * Restore the original rect, after doing the animation
                     */
                     if (bMirroredParent) {
                         rc = rcT;
                     }
#endif
            }

        } else {
            swpFlags |= SWP_SHOWWINDOW;
        }
    }

    /*
     * hack for VB - we add their window in when their minimizing.
     */
    if (!TestWF(pwnd, WFWIN40COMPAT) && fIsTrayWindowNow && FDoTray()) {

        HWND hw = HWq(pwnd);

        if (FCallHookTray()) {
            xxxCallHook(HSHELL_WINDOWCREATED,
                        (WPARAM)hw,
                        (LPARAM)0,
                        WH_SHELL);
        }

        /*
         * NT specific code.  Post the window-created message
         * to the shell.
         */
        if (FPostTray(pwnd->head.rpdesk))
            PostShellHookMessages(HSHELL_WINDOWCREATED, (LPARAM)hw);
    }

    /*
     * BACKWARD COMPATIBILITY HACK:
     *
     * Because SetWindowPos() won't honor sizing, moving and SWP_SHOWWINDOW
     * at the same time in version 3.0 or below, we call DeferWindowPos()
     * directly here.
     */
    if (psmwp = InternalBeginDeferWindowPos(1)) {

        psmwp = _DeferWindowPos(psmwp,
                                pwnd,
                                ((hwndAfter != NULL) ? RevalidateHwnd(hwndAfter) : NULL),
                                rc.left, rc.top,
                                rc.right - rc.left,
                                rc.bottom - rc.top,
                                swpFlags);

        if (psmwp) {

            /*
             * HACK FOR MULTIPLE MONITOR TRUE MAXIMIZATION CLIPPING
             *      On a multiple monitor system, we would like the
             *      borders not to spill over onto another monitor when a
             *      window 'really' maximizes.  The only way to get this
             *      to work right is to set a rectangular region, namely
             *      a copy of the monitor region, on the window.  We can
             *      only do this if the window isn't currently regional.
             *
             *  Going to maximized:     Add the monitor region
             *  Coming from maximized:  Remove the monitor region
             */
            if (fMaxStateChanging && gpDispInfo->cMonitors > 1) {
                if (    TestWF(pwnd, WFMAXIMIZED) &&
                        pwnd->spwndParent == PWNDDESKTOP(pwnd)) {

                    psmwp->acvr[0].hrgnClip = HRGN_MONITOR;

                } else if (TestWF(pwnd, WFMAXFAKEREGIONAL)) {
                    UserAssert(pwnd->hrgnClip);
                    psmwp->acvr[0].hrgnClip = HRGN_FULL;
                }
            }

            xxxEndDeferWindowPosEx(psmwp, FALSE);
        }
    }

    if (FWINABLE() && uEvent) {
        xxxWindowEvent(uEvent, pwnd, OBJID_WINDOW, 0, WEF_USEPWNDTHREAD);
    }

    /*
     * COMPATIBILITY HACK:
     * Borland's OBEX expects a WM_PAINT message when it starts running
     * minimized and initializes all it's data during that message.
     * So, we generate a bogus WM_PAINT message here.
     * Also, Visionware's XServer can not handle getting a WM_PAINT msg, as it
     * would always get a WM_PAINTICON msg in 3.1, so make sure the logic is here
     * to generate the correct message.
     */
    if((cmd == SW_SHOWMINIMIZED)      &&
       (!TestWF(pwnd, WFWIN40COMPAT)) &&
        TestWF(pwnd, WFVISIBLE)       &&
        TestWF(pwnd, WFTOPLEVEL)) {

        if (pwnd->pcls->spicn)
            _PostMessage(pwnd, WM_PAINTICON, (WPARAM)TRUE, 0L);
        else
            _PostMessage(pwnd, WM_PAINT, 0, 0L);
    }

    if (fShowOwned)
        xxxShowOwnedWindows(pwnd, SW_PARENTOPENING, NULL);

    if ((cmd == SW_MINIMIZE) && (pwnd->spwndParent == PWNDDESKTOP(pwnd))) {
        if (!xxxActivateOnMinimize(pwnd)) {
            xxxActivateWindow(pwnd, AW_SKIP);
        }

        {
            PEPROCESS p;

            if (gptiForeground && ptiCurrent->ppi != gptiForeground->ppi && !(ptiCurrent->TIF_flags & TIF_SYSTEMTHREAD)) {

                p = THREAD_TO_PROCESS(ptiCurrent->pEThread);
                KeAttachProcess(&p->Pcb);
                Status = MmAdjustWorkingSetSize((SIZE_T)-1, (SIZE_T)-1, FALSE);
                KeDetachProcess();

                if (!NT_SUCCESS(Status)) {
                    RIPMSG1(RIP_ERROR, "Error adjusting working set, status = %x\n", Status);
                }
            }
        }

        /*
         * If any app is starting, restore its right to foreground activate
         * (activate and come on top of everything else) because we just
         * minimized what we were working on.
         */
        RestoreForegroundActivate();
    }

    /*
     * If going from iconic, insure the focus is in the window.
     */
    if (fSetFocus)
        xxxSetFocus(pwnd);

    /*
     * This was added for 1.03 compatibility reasons.  If apps watch
     * WM_ACTIVATE to set their focus, sending this message will appear
     * as if the window just got activated (like in 1.03).  Before this
     * was added, opening an iconic window never sent this message since
     * it was already active (but HIWORD(lParam) != 0).
     */
    if (fSendActivate)
        xxxSendMessage(pwnd, WM_ACTIVATE, WA_ACTIVE, 0);

    /*
     * Flush the palette.  We do this on a minimize of a palette app.
     */
    if (fFlushPalette)
        xxxFlushPalette(pwnd);

Exit:
    return NULL;
}

/***************************************************************************\
* xxxMinimizeHungWindow
*
* 10/31/96      vadimg      created
\***************************************************************************/

void xxxMinimizeHungWindow(PWND pwnd)
{
    RECT rcMin;
    HRGN hrgnHung;


    CheckLock(pwnd);

    /*
     * If the window is already minimized or not visible don't do anything.
     */
   if (TestWF(pwnd, WFMINIMIZED) || !TestWF(pwnd, WFVISIBLE))
       return;

    /*
     * Animate the caption to the minimized state.
     */
    if (TEST_PUDF(PUDF_ANIMATE)) {
        SetRectEmpty(&rcMin);
        xxxSendMinRectMessages(pwnd, &rcMin);
        if (!IsRectEmpty(&rcMin)) {
            xxxDrawAnimatedRects(pwnd, IDANI_CAPTION, &pwnd->rcWindow, &rcMin);
        }
    }

    /*
     * Reset the visible bit on the window itself and ownees. At the same
     * time calculate how much needs to be repainted. We must invalidate
     * the DC cache to make sure that the visible regions get recalculated.
     */
    SetVisible(pwnd, SV_UNSET);
    hrgnHung = GreCreateRectRgnIndirect(&pwnd->rcWindow);
    xxxShowOwnedWindows(pwnd, SW_PARENTCLOSING, hrgnHung);
    zzzInvalidateDCCache(pwnd, IDC_DEFAULT);
    xxxRedrawWindow(NULL, NULL, hrgnHung, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
    GreDeleteObject(hrgnHung);

    /*
     * Deal with activating some other window for top-level windows.
     */
    if (pwnd->spwndParent == PWNDDESKTOP(pwnd)) {
        xxxActivateOnMinimize(pwnd);
    }
    PostEventMessage(GETPTI(pwnd), GETPTI(pwnd)->pq, QEVENT_HUNGTHREAD, pwnd, 0, 0, 0);
}
