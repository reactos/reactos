/**************************** Module Header ********************************\
* Module Name: spb.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Save Popup Bits (SPB) support routines.
*
* History:
* 18-Jul-1991 DarrinM   Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* FBitsTouch
*
* This routine checkes to see if the rectangle *lprcDirty in pwndDirty
* invalidates any bits in the SPB structure at *pspb.
*
* pwndDirty "touches" pwndSpb if:
*   1. pwndDirty is visible AND:
*   2. pwndDirty == or descendent of pwndSpb, and pwndSpb is a LOCKUPDATE
*      spb.
*   3. pwndDirty is pwndSpb's parent.  (e.g., drawing in the
*      desktop window, behind a dialog box).
*   4. A parent of pwndDirty is the sibling of pwndSpb, and the parent
*      is lower in the zorder.
*
* History:
* 18-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL FBitsTouch(
    PWND   pwndDirty,
    LPRECT lprcDirty,
    PSPB   pspb,
    DWORD  flags)
{
    PWND    pwndSpb,
            pwndDirtySave;
    int     fSpbLockUpdate;

    /*
     * When no window is passed in, skip all the window-related stuff and
     * go directly to check the rectangle.
     */
    if (pwndDirty == NULL)
        goto ProbablyTouch;

    /*
     * If pwndDirty or its parents are invisible,
     * then it can't invalidate any SPBs
     */
    if (!IsVisible(pwndDirty))
        return FALSE;

    pwndSpb = pspb->spwnd;
    fSpbLockUpdate = pspb->flags & SPB_LOCKUPDATE;
    if (fSpbLockUpdate) {

        /*
         * If the guy is drawing through a locked window via
         * DCX_LOCKWINDOWUPDATE and the spb is a LOCKUPDATE SPB, then
         * don't do any invalidation of the SPB.  Basically we're trying
         * to avoid having the tracking rectangle invalidate the SPB
         * since it's drawn via a WinGetClipPS() ps.
         */
        if (flags & DCX_LOCKWINDOWUPDATE)
            return FALSE;
    }

    /*
     * If pwndDirty is pwndSpb's immediate parent (e.g., drawing in the
     * desktop window behind a dialog box), then we may touch: do the
     * intersection.
     */
    if (pwndDirty == pwndSpb->spwndParent)
        goto ProbablyTouch;

    /*
     * We know that pwndDirty != pwndSpb->spwndParent.
     * Now find the parent of pwndDirty that is a sibling of pwndSpb.
     */
    pwndDirtySave = pwndDirty;

    while (pwndSpb->spwndParent != pwndDirty->spwndParent) {
        pwndDirty = pwndDirty->spwndParent;

        /*
         * If we get to the top of the tree, it's because:
         *  1.  pwndSpb == pwndDesktop
         *  2.  pwndDirty is a parent of pwndSpb
         *  3.  pwndDirty == pwndDesktop
         *  4.  pwndDirty is a child of some other desktop
         *  5.  pwndSpb and pwndDirty aren't siblings
         *
         * In all these cases, pwndDirty can't touch pwndSpb.
         */
        if (pwndDirty == NULL)
            return FALSE;
    }

    /*
     * If pwndSpb is the same as pwndDirty, then it will invalidate
     * only if the SPB is LOCKUPDATE.
     *
     * Non-LOCKUPDATE SPB's can't be invalidated by their
     * own windows, but LOCKUPDATE SPB's can.
     */
    if (pwndDirty == pwndSpb) {
        if (!fSpbLockUpdate)
            return FALSE;

        /*
         * If pwndSpb itself was drawn in, then we can't
         * try subtracting children.
         */
        if (pwndDirtySave == pwndSpb)
            goto ProbablyTouch;

        /*
         * We want to calculate the immediate child of pwndSpb
         * on the path from pwndDirty to pwndSpb, so we can
         * subtract off the rectangles of the children of pwndSpb
         * in case there are intervening windows.
         */
        while (pwndSpb != pwndDirtySave->spwndParent) {
            pwndDirtySave = pwndDirtySave->spwndParent;
        }

        /*
         * The SubtractIntervening loop subtracts the
         * window rects starting from pwndSpb and ending
         * at the window before pwndDirty, so set up
         * our variables appropriately.
         */
        pwndDirty = pwndDirtySave;
        pwndSpb = pwndSpb->spwndChild;

    } else {
        /*
         * Now compare the Z order of pwndDirty and pwndSpb.
         * If pwndDirty is above pwndSpb, then the SPB can't be touched.
         */
        pwndDirtySave = pwndDirty;

        /*
         * Compare the Z order by searching starting at pwndDirty,
         * moving DOWN the Z order list.  If we encounter pwndSpb,
         * then pwndDirty is ABOVE or EQUAL to pwndSpb.
         */
        for ( ; pwndDirty != NULL; pwndDirty = pwndDirty->spwndNext) {
            if (pwndDirty == pwndSpb) {
                return FALSE;
            }
        }
        pwndDirty = pwndDirtySave;

        /*
         * We don't want to subtract the SPB window itself
         */
        pwndSpb = pwndSpb->spwndNext;
    }

    /*
     * Subtract Intervening rectangles.
     * pwndDirty is below pwndSpb.  If there are any intervening
     * windows, subtract their window rects from lprcDirty to see if pwndDirty
     * is obscured.
     */
    while (pwndSpb && pwndSpb != pwndDirty) {
        /*
         * If this window has a region selected, hwndDirty may draw through
         * it even though it has a full rectangle! We can't subtract its
         * rect from the dirty rect in this case.
         */
        if (    TestWF(pwndSpb, WFVISIBLE) &&
                !pwndSpb->hrgnClip &&
                !SubtractRect(lprcDirty, lprcDirty, &pwndSpb->rcWindow)) {

            return FALSE;
        }

        pwndSpb = pwndSpb->spwndNext;
    }

    // fall through
ProbablyTouch:

    /*
     * If the rectangles don't intersect, there is no invalidation.
     * (we make this test relatively late because it's expensive compared
     * to the tests above).
     * Otherwise, *lprcDirty now has the area of bits not obscured
     * by intervening windows.
     */

    return IntersectRect(lprcDirty, lprcDirty, &pspb->rc);
}

/***************************************************************************\
* SpbCheckRect2
*
* Subtracts lprc in pwnd from pspb's region if lprc touches pspb.
*
* Returns FALSE if there is a memory allocation error, or if lprc
* contains psbp's region; otherwise, returns TRUE.
*
* History:
* 18-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL SpbCheckRect2(
    PSPB   pspb,
    PWND   pwnd,
    LPRECT lprc,
    DWORD  flags)
{
    RECT rcTouch = *lprc;

    /*
     * See if lprc touches any saved bits, taking into account what
     * window the drawing is occuring in.
     */
    if (FBitsTouch(pwnd, &rcTouch, pspb, flags)) {

        /*
         * If no SPB region exists, make one for the whole thing
         */
        if (!pspb->hrgn && SetOrCreateRectRgnIndirectPublic(
                &pspb->hrgn, &pspb->rc) == ERROR) {

            goto Error;
        }

        /*
         * Subtract the rectangle that is invalid from the SPB region
         */
        SetRectRgnIndirect(ghrgnSCR, &rcTouch);
        switch (SubtractRgn(pspb->hrgn, pspb->hrgn, ghrgnSCR)) {
        case ERROR:
        case NULLREGION:
            goto Error;

        default:
            break;
        }
    }

    return TRUE;

Error:
    FreeSpb(pspb);
    return FALSE;
}

/***************************************************************************\
* SpbTransfer
*
* Validate the SPB rectangle from a window's update region, after
* subtracting the window's update region from the SPB.
*
* NOTE: Although SpbTransfer calls xxxInternalInvalidate, it doesn't
* specify any flags that will cause immediate updating.  Therefore the
* critsect isn't left and we don't consider this an 'xxx' routine.
* Also, no revalidation is necessary.
*
* History:
* 18-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL SpbTransfer(
    PSPB pspb,
    PWND pwnd,
    BOOL fChildren)
{
    RECT rc;

    /*
     * If the window has an update region...
     */
    if (pwnd->hrgnUpdate != NULL) {

        /*
         * Invalidate its update region rectangle from the SPB
         */
        if (pwnd->hrgnUpdate > HRGN_FULL) {
            GreGetRgnBox(pwnd->hrgnUpdate, &rc);
        } else {
            rc = pwnd->rcWindow;
        }

        /*
         * Intersect the update region bounds with the parent client rects,
         * to make sure we don't invalidate more than we need to.  If
         * nothing to validate, return TRUE (because SPB is probably not empty)
         * These RDW_ flags won't cause the critical section to be left, nor
         * will they provoke WinEvent notifications.
         */
        if (IntersectWithParents(pwnd, &rc)) {
            BEGINATOMICCHECK();

            xxxInternalInvalidate(pwnd,
                                  ghrgnSPB2,
                                  RDW_VALIDATE | RDW_NOCHILDREN);

            ENDATOMICCHECK();

            /*
             * If the SPB vanished, return FALSE.
             */
            if (!SpbCheckRect2(pspb, pwnd, &rc, DCX_WINDOW))
                return FALSE;
        }
    }

    if (fChildren) {
        for (pwnd = pwnd->spwndChild; pwnd != NULL; pwnd = pwnd->spwndNext) {
            if (!SpbTransfer(pspb, pwnd, TRUE)) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

/***************************************************************************\
* CreateSpb
*
* This function, called after the window is created but before it is visible,
* saves the contents of the screen where the window will be drawn in a SPB
* structure, and links the structure into a linked list of SPB structures.
* popup bits. This routine is called from SetWindowPos.
*
* History:
* 18-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

VOID CreateSpb(
    PWND pwnd,
    UINT flags,
    HDC  hdcScreen)
{
    PSPB    pspb;
    int     fSpbLockUpdate;

    /*
     * Non-LOCKWINDOWUPDATE SPBs can only be created for top-level windows.
     *
     * This is because of the way that the display driver RestoreBits function
     * works.  It can put bits down in places that aren't even part of the
     * window's visrgn, and these bits need to be invalidated.  The
     * SetWindowPos() code to handle this case only knows how to invalidate
     * one of windows (i.e., the window's immediate parent), but all levels
     * need to get invalidated.  See also the comments in wmswp.c, near the
     * call to RestoreSpb().
     *
     * For example: the Q&E app brings up a copyright dialog that is a child
     * of its main window.  While this is up, the user alt-f alt-l to execute
     * the file login command, which brings up another dialog that is a child
     * of the desktop.  When the copyright dialog goes away, the display driver
     * restores bits on top of the second dialog.  The SWP code knows to
     * invalidate the bogus stuff in the main window, but not in the desktop.
     *
     * LOCKUPDATE SPBs are fine, because they don't call RestoreBits.
     */
    fSpbLockUpdate = flags & SPB_LOCKUPDATE;
    if (    !fSpbLockUpdate             &&
            pwnd->spwndParent != NULL   &&
            pwnd->spwndParent != PWNDDESKTOP(pwnd)) {

        return;
    }

    /*
     * We go and check all the existing DCs at this point, to handle the
     * case where we're saving an image of a window that has a "dirty"
     * DC, which would eventually invalidate our saved image (but which
     * is really okay).
     */
    if (AnySpbs()) {

        SpbCheck();

    } else {

        PDCE pdce;

        /*
         * Reset the dirty areas of all of the DC's and enable
         * bounds accumulation.  We're creating a SPB now.  This
         * is only done if there are no other SPB's in the list.
         */
        GreLockDisplay(gpDispInfo->hDev);

        for (pdce = gpDispInfo->pdceFirst; pdce != NULL; pdce = pdce->pdceNext) {

            if (pdce->DCX_flags & DCX_LAYERED)
                continue;

            GreGetBounds(pdce->hdc, NULL, GGB_ENABLE_WINMGR);
        }

        GreUnlockDisplay(gpDispInfo->hDev);
    }

    /*
     * Create the save popup bits structure
     */
    pspb = (PSPB)UserAllocPoolWithQuota(sizeof(SPB), TAG_SPB);
    if (!pspb)
        return;

    pspb->spwnd = NULL;
    pspb->rc    = pwnd->rcWindow;

    /*
     * Clip to the screen
     */
    if (!IntersectRect(&pspb->rc, &pspb->rc, &gpDispInfo->rcScreen))
        goto BMError;

    pspb->hrgn  = NULL;
    pspb->hbm   = NULL;
    pspb->flags = flags;
    Lock(&(pspb->spwnd), pwnd);

    if (!fSpbLockUpdate) {

        RECT rc = pspb->rc;

        if (!SYSMET(SAMEDISPLAYFORMAT)) {
            PMONITOR pMonitor = _MonitorFromRect(&pspb->rc, MONITOR_DEFAULTTOPRIMARY);
            RECT rcT;

            /*
             * If the intersection with the monitor isn't the entire visible
             * window rectangle, then bail!  We don't save SPBs for windows
             * that span multiple monitors.  Since we do a lot of work to
             * pin dialogs and menus, there won't be too many of these
             * babies.
             */
            if (SubtractRect(&rcT, &pspb->rc, &pMonitor->rcMonitor) &&
                    GreRectInRegion(gpDispInfo->hrgnScreen, &rcT))
                goto BMError2;

            /*
             * Clip to the window's monitor
             */
            if (!IntersectRect(&pspb->rc, &pspb->rc, &pMonitor->rcMonitor))
                goto BMError2;

            /*
             * dont save bits in a mixed bitdepth situtation
             * we cant create the exactly correct format bitmap
             * in all cases (555/565, and Paletized) so as
             * a cop-out dont save bitmaps at all (on secondaries)
             * in mixed bit-depth.
             *
             * the correct fix is to create a compatible
             * bitmap for the monitor device and directly
             * BitBlt() from/to the device (pMonitor->hdcMonitor)
             * but this involves too much code at this time.
             */
            if (pMonitor != gpDispInfo->pMonitorPrimary)
                goto BMError2;
        }

        /*
         * If this window is a regional window, don't use driver save
         * bits. Because it can only restore an entire rectangle,
         * invalid region is calculated assuming the old vis rgn was
         * rectangular. For regional windows, this would end up always
         * invalidating the area of (rcWindow - hrgnWindow) every
         * time an spb would be used. On the other hand, the invalid
         * area calculated when not using driver save bits is perfect,
         * because the restore blt can be correctly clipped to begin with.
         */
        if ((pwnd->hrgnClip == NULL) &&
            (pspb->ulSaveId = GreSaveScreenBits(gpDispInfo->hDev,
                                                SS_SAVE,
                                                0,
                                                (RECTL *)&rc))) {

            /*
             * Remember that we copied this bitmap into on board memory.
             */
            pspb->flags |= SPB_SAVESCREENBITS;

        } else {
            HBITMAP hbmSave;
            BOOL    bRet;

            /*
             * The following delta byte-aligns the screen bitmap
             */
            int dx = pspb->rc.left & 0x0007;
            int cx = pspb->rc.right - pspb->rc.left;
            int cy = pspb->rc.bottom - pspb->rc.top;

            /*
             * NOTE: we don't care about setting up a visrgn in
             * hdcScreen, because BitBlt ignores it on reads.
             */
            pspb->hbm = GreCreateCompatibleBitmap(hdcScreen, cx + dx, cy);
            if (!pspb->hbm)
                goto BMError2;

            hbmSave = (HBITMAP)GreSelectBitmap(ghdcMem, pspb->hbm);
            if (!hbmSave)
                goto BMError2;

            /*
             * Copy the contents of the screen to the bitmap in the
             * save popup bits structure.  If we ever find we run
             * into problems with the screen access check we can
             * do a bLockDisplay, give this process permission, do
             * the BitBlt and then take away permission.  GDI
             * accesses the screen and that bit only under the
             * display semaphore so it is safe.  Alternatively
             * if it is too hard to change this processes permission
             * here we could do it in GDI by marking the psoSrc
             * readable temporarily while completing the operation
             * and then setting it back to unreadable when done.
             * Or we could just fail it like the CreateCompatibleDC
             * failed and force a redraw.  Basically we can't add
             * 3K of code in GDI to do a BitBlt that just does 1
             * test differently for this 1 place in User.
             *
             */
            bRet = GreBitBlt(ghdcMem,
                             dx,
                             0,
                             cx,
                             cy,
                             hdcScreen,
                             pspb->rc.left,
                             pspb->rc.top,
                             0x00CC0000,
                             0);

            GreSelectBitmap(ghdcMem, hbmSave);

            if (!bRet)
                goto BMError2;

            GreSetBitmapOwner(pspb->hbm, OBJECT_OWNER_PUBLIC);
        }

        /*
         * Mark that the window has an SPB.
         */
        SetWF(pwnd, WFHASSPB);

        /*
         * non-LOCKUPDATE SPBs are not invalidated by
         * drawing in pspb->spwnd, so start the SPB validation
         * loop below at the sibling immediately below us.
         */
        pwnd = pwnd->spwndNext;
    }

    /*
     * Link the new save popup bits structure into the list.
     */
    pspb->pspbNext = gpDispInfo->pspbFirst;
    gpDispInfo->pspbFirst = pspb;

    /*
     * Here we deal with any update regions that may be
     * pending in windows underneath the SPB.
     *
     * For all windows that might affect this SPB:
     *    - Subtract the SPB rect from the update region
     *    - Subtract the window from the SPB
     *
     * Note that we use pspb->spwnd here, in case it has
     * no siblings.
     *
     * ghrgnSPB2 is the region that is used inside of SpbTransfer to
     * validate window update regions. Intersect with the window clipping
     * region, if it exists. Don't want to intersect with the spb rect if
     * a clipping region exists because we'll end up validating more than
     * we want to validate.
     */
    SetRectRgnIndirect(ghrgnSPB2, &pspb->rc);
    if (pspb->spwnd->hrgnClip != NULL) {

        /*
         * If we get an error bail since an error might result in more
         * being validated than we want. Since the below code is only an
         * optimizer, this is ok: the window will remain invalid and will
         * draw, thereby invalidating the SPB like usual.
         */
        if (IntersectRgn(ghrgnSPB2,
                         ghrgnSPB2,
                         pspb->spwnd->hrgnClip) == ERROR) {
            return;
        }
    }

    if (pspb->spwnd->spwndParent == NULL ||
            SpbTransfer(pspb, pspb->spwnd->spwndParent, FALSE)) {

        /*
         * Do the same for the siblings underneath us...
         */
        for ( ; pwnd != NULL; pwnd = pwnd->spwndNext) {
            if (!SpbTransfer(pspb, pwnd, TRUE))
                break;
        }
    }

    return;

BMError2:
    /*
     * Error creating the bitmap: clean up and return.
     */
    if (pspb->hbm)
        GreDeleteObject(pspb->hbm);

    Unlock(&pspb->spwnd);
    // fall-through

BMError:
    UserFreePool(pspb);
}

/***************************************************************************\
* RestoreSpb
*
* Restores the bits associated with pwnd's SPB onto the screen, clipped
* to hrgnUncovered if possible.
*
* Upon return, hrgnUncovered is modified to contain the part of hrgnUncovered
* that must be invalidated by the caller.  FALSE is returned if the area
* to be invalidated is empty.
*
* NOTE: Because the device driver SaveBitmap() function can not clip, this
* function may write bits into an area of the screen larger than the passed-in
* hrgnUncovered.  In this case, the returned invalid region may be larger
* than the passed-in hrgnUncovered.
*
* History:
* 18-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

UINT RestoreSpb(
    PWND pwnd,
    HRGN hrgnUncovered,
    HDC  *phdcScreen)
{
    PSPB pspb;
    UINT uInvalidate;
    HRGN hrgnRestorable;

    /*
     * Note that we DON'T call SpbCheck() here --
     * SpbCheck() is called by zzzBltValidBits().
     */
    pspb = FindSpb(pwnd);

    /*
     * Assume all of hrgnUncovered was restored, and there's nothing
     * for our caller to invalidate.
     */
    uInvalidate = RSPB_NO_INVALIDATE;
    hrgnRestorable = hrgnUncovered;

    /*
     * First determine whether or not there is any area at all to restore.
     * If hrgnUncovered & pspb->hrgn is empty, then all of hrgnUncovered
     * needs to be invalidated, and there's nothing to restore.
     */
    if (pspb->hrgn != NULL) {
        /*
         * At least some of hrgnUncovered needs to be invalidated.
         */
        uInvalidate = RSPB_INVALIDATE;

        /*
         * Calculate the true area of bits to be restored.  If it becomes
         * empty, then just free the SPB without changing hrgnUncovered,
         * which is the area that must be invalidated.
         */
        hrgnRestorable = ghrgnSPB1;
        switch (IntersectRgn(hrgnRestorable, hrgnUncovered, pspb->hrgn)) {
        case ERROR:
        case NULLREGION:
            goto Error;

        default:
            break;
        }
    }

    if (pspb->flags & SPB_SAVESCREENBITS) {

        RECT rc = pspb->rc;

        /*
         * Since the restore frees the onboard memory, clear this
         * bit so FreeSpb() won't try to free it again (regardless of
         * whether we get an error or not)
         */
        pspb->flags &= ~SPB_SAVESCREENBITS;
        if (!(GreSaveScreenBits(gpDispInfo->hDev,
                                SS_RESTORE,
                                pspb->ulSaveId,
                                (RECTL *)&rc))) {
            goto Error;
        }

        /*
         * The SS_RESTORE call will always restore the entire SPB
         * rectangle, part of which may fall outside of hrgnUncovered.
         * The area that must be invalidated by our caller is simply
         * the SPB rectangle minus the area of restorable bits.
         *
         * If this region is not empty, then the SPB was not completely
         * restored, so we must return FALSE.
         */
        SetRectRgnIndirect(ghrgnSPB2, &pspb->rc);
        if (SubtractRgn(hrgnUncovered, ghrgnSPB2, hrgnRestorable) != NULLREGION) {
            uInvalidate = RSPB_INVALIDATE_SSB;
        }
    } else {

        HDC     hdcScreen;
        HBITMAP hbmSave;

        /*
         * In the unlikely event we need a screen DC and one wasn't passed in,
         * get it now.  If we get one, we return the handle in *phdcScreen
         * so that our caller can release it later.
         */
        if (!*phdcScreen) {
            *phdcScreen = gpDispInfo->hdcScreen;
        }

        hdcScreen = *phdcScreen;

        hbmSave = (HBITMAP)GreSelectBitmap(ghdcMem, pspb->hbm);
        if (!hbmSave)
            goto Error;

        /*
         * Be sure to clip to the area of restorable bits.
         */

        GreSelectVisRgn(hdcScreen, hrgnRestorable, SVR_COPYNEW);

        GreBitBlt(hdcScreen,
                  pspb->rc.left, pspb->rc.top,
                  pspb->rc.right - pspb->rc.left,
                  pspb->rc.bottom - pspb->rc.top,
                  ghdcMem,
                  pspb->rc.left & 0x0007,
                  0,
                  SRCCOPY,
                  0);

        GreSelectBitmap(ghdcMem, hbmSave);

        /*
         * Now compute the area to be invalidated for return.
         * This is simply the original hrgnUncovered - hrgnRestorable
         */
        SubtractRgn(hrgnUncovered, hrgnUncovered, hrgnRestorable);
    }

    if (pwnd->hrgnClip == NULL || !IsVisible(pwnd))
        FreeSpb(pspb);

    return uInvalidate;

Error:
    FreeSpb(pspb);
    return RSPB_INVALIDATE;
}



/***************************************************************************\
* LockWindowUpdate2 (API)
*
* Locks gspwndLockUpdate and it's children from updating.  If
* gspwndLockUpdate is NULL, then all windows will be unlocked.  When
* unlocked, the portions of the screen that would have been written to
* are invalidated so they get repainted. TRUE is returned if the routine
* is successful.
*
* If called when another app has something locked, then this function fails.
*
* History:
* 18-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL LockWindowUpdate2(
    PWND pwndLock,
    BOOL fThreadOverride)
{
    PSPB pspb;
    BOOL fInval;
    HRGN hrgn;
    PTHREADINFO  ptiCurrent = PtiCurrent();

    if (    /*
             * If we're full screen right now, fail this call.
             */
            TEST_PUDF(PUDF_LOCKFULLSCREEN)

            ||

            /*
             * If the screen is already locked, and it's being locked
             * by some other app, then fail.  If fThreadOverride is set
             * then we're calling internally and it's okay to cancel
             * someone elses LockUpdate.
             */
            (   gptiLockUpdate != NULL &&
                gptiLockUpdate != PtiCurrent() &&
                !fThreadOverride)) {
    UserAssert(IsWinEventNotifyDeferredOK());

        RIPERR0(ERROR_SCREEN_ALREADY_LOCKED,
                RIP_WARNING,
                "LockWindowUpdate failed because screen is locked by another application.");

        return FALSE;
    }

    if ((pwndLock != NULL) == (gptiLockUpdate != NULL)) {
        if (!fThreadOverride) {
            RIPERR1(ERROR_INVALID_PARAMETER,
                    RIP_WARNING,
                    "LockWindowUpdate failed because it is already %s.",
                    (pwndLock != NULL) ? "locked" : "unlocked");
        }

        return FALSE;
    }

    /*
     * This must be done while holding the screen critsec.
     * Deadlock if we callback during this, so defer WinEvent notifications
     */
    DeferWinEventNotify();
    GreLockDisplay(gpDispInfo->hDev);

    if (pwndLock != NULL) {
        /*
         * We're about to make pwndLock and its siblings invisible:
         * go invalidate any other affected SPBs.
         */
        SpbCheckPwnd(pwndLock);

        CreateSpb(pwndLock, SPB_LOCKUPDATE, NULL);

        Lock(&(gspwndLockUpdate), pwndLock);
        gptiLockUpdate = ptiCurrent;

        zzzInvalidateDCCache(pwndLock, IDC_DEFAULT);

    } else {
        /*
         * Flush any accumulated rectangles and invalidate spbs.
         */
        SpbCheck();

        /*
         * Save this in a local before we set it to NULL
         */
        pwndLock = gspwndLockUpdate;

        gptiLockUpdate = NULL;
        Unlock(&gspwndLockUpdate);

        zzzInvalidateDCCache(pwndLock, IDC_DEFAULT);

        /*
         * Assume SPB doesn't exist, or couldn't be created, and that we
         * must invalidate the entire window.
         */
        fInval = TRUE;
        hrgn = HRGN_FULL;

        /*
         * Find the LOCKUPDATE spb in the list, and if present calculate
         * the area that has been invalidated, if any.
         */
        for (pspb = gpDispInfo->pspbFirst; pspb != NULL; pspb = pspb->pspbNext) {

            if (pspb->flags & SPB_LOCKUPDATE) {

                if (pspb->hrgn == NULL) {

                    /*
                     * If no invalid area, then no invalidation needed.
                     */
                    fInval = FALSE;

                } else {

                    /*
                     * Subtract SPB valid region from SPB rectangle, to
                     * yield invalid region.
                     */
                    hrgn = ghrgnSPB1;
                    SetRectRgnIndirect(hrgn, &pspb->rc);

                    /*
                     * If spb rect minus the spb valid rgn is empty,
                     * then there is nothing to invalidate.
                     */
                    fInval = SubtractRgn(hrgn, hrgn, pspb->hrgn) != NULLREGION;
                }

                FreeSpb(pspb);

                /*
                 * Exit this loop (there can be only one LOCKUPDATE spb)
                 */
                break;
            }
        }

        if (fInval) {
            BEGINATOMICCHECK();
            // want to prevent WinEvent notifies, but this make break asserts
            DeferWinEventNotify();
            xxxInternalInvalidate(PWNDDESKTOP(pwndLock),
                               hrgn,
                               RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
            zzzEndDeferWinEventNotify();
            ENDATOMICCHECK();
        }

        /*
         * Invalidate any other SPBs affected by the fact that this window
         * and its children are being made visible.
         */
        SpbCheckPwnd(pwndLock);
    }

    GreUnlockDisplay(gpDispInfo->hDev);
    zzzEndDeferWinEventNotify();

    return TRUE;
}

/***************************************************************************\
* FindSpb
*
* Returns a pointer to the SPB structure associated with the specified
* window or NULL if there is no associated structure.
*
* History:
* 18-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

PSPB FindSpb(
    PWND pwnd)
{
    PSPB pspb;

    /*
     * Walk through the list of save popup bits looking for a match on
     * window handle.
     */
    for (pspb = gpDispInfo->pspbFirst; pspb != NULL; pspb = pspb->pspbNext) {

        if (pspb->spwnd == pwnd && !(pspb->flags & SPB_LOCKUPDATE))
            break;
    }

    return pspb;
}

/***************************************************************************\
* SpbCheck
*
* Modifies all of the save popup bits structures to reflect changes on the
* screen. This function walks through all of the DC's, and if the DC is
* dirty, then the dirty area is removed from the associated save popup bits
* structure.
*
* History:
* 18-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

VOID SpbCheck(VOID)
{
    PDCE pdce;
    RECT rcBounds;

    if (AnySpbs()) {

        GreLockDisplay(gpDispInfo->hDev);

        /*
         * Walk through all of the DC's, accumulating dirty areas.
         */
        for (pdce = gpDispInfo->pdceFirst; pdce != NULL; pdce = pdce->pdceNext) {

            /*
             * Only check valid cache entries...
             */
            if (pdce->DCX_flags & (DCX_INVALID | DCX_DESTROYTHIS))
                continue;
            
            SpbCheckDce(pdce);
        }

        /*
         * Subtact out DirectDraw dirty rect from all the SPB's. The call to
         * GreGetDirectDrawBounds will also reset the accumulated bounds.
         */
        if (GreGetDirectDrawBounds(gpDispInfo->hDev, &rcBounds)) {
            SpbCheckRect(NULL, &rcBounds, 0);
        }

        GreUnlockDisplay(gpDispInfo->hDev);
    }
}

/***************************************************************************\
* SpbCheckDce
*
* This function retrieves the dirty area of a DC and removes the area from
* the list of SPB structures. The DC is then marked as clean.
*
* History:
* 18-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

VOID SpbCheckDce(
    PDCE pdce)
{
    RECT rc;

    if (pdce->DCX_flags & DCX_LAYERED)
        return;

    /*
     * Query the dirty bounds rectangle.  Doing this clears the bounds
     * as well.
     */
    if (GreGetBounds(pdce->hdc, &rc, 0)) {

        if (pdce->pMonitor != NULL) {
            /*
             * Convert the bounds rect to screen coords.
             */
            OffsetRect(&rc, pdce->pMonitor->rcMonitor.left, 
                    pdce->pMonitor->rcMonitor.top);
        }

        /*
         * Intersect the returned rectangle with the window rectangle
         * in case the guy was drawing outside his window
         */
        if (IntersectRect(&rc, &rc, &(pdce->pwndOrg)->rcWindow))
            SpbCheckRect(pdce->pwndOrg, &rc, pdce->DCX_flags);
    }
}

/***************************************************************************\
* SpbCheckRect
*
* This function removes the passed rectangle from the SPB structures which
* touch it.
*
* History:
* 18-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

VOID SpbCheckRect(
    PWND   pwnd,
    LPRECT lprc,
    DWORD  flags)
{
    PSPB pspb, pspbNext;

    /*
     * If this window isn't visible, we're done.
     */
    if (!IsVisible(pwnd))
        return;

    for (pspb = gpDispInfo->pspbFirst; pspb != NULL; pspb = pspbNext) {

        /*
         * Get the pointer to the next save popup bits structure now
         * in case SpbCheckRect2() frees the current one.
         */
        pspbNext = pspb->pspbNext;

        /*
         * In win3.1 they used to exit the function if this function
         * returned false.  This meant that if one of the spbs was freed
         * the rest of the spbs would not be invalidated.
         */
        SpbCheckRect2(pspb, pwnd, lprc, flags);
    }
}

/***************************************************************************\
* SpbCheckPwnd
*
* This routine checks to see if the window rectangle of PWND affects any SPBs.
* It is called if pwnd or its children are being hidden or shown without
* going through WinSetWindowPos().
*
* Any SPBs for children of pwnd are destroyed.
*
* It must be called while pwnd is still visible.
*
* History:
* 18-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

VOID SpbCheckPwnd(
    PWND pwnd)
{
    PSPB pspb;
    PWND pwndSpb;
    PSPB pspbNext;

    /*
     * First blow away any SPBs owned by this window or its children.
     */
    for (pspb = gpDispInfo->pspbFirst; pspb != NULL; pspb = pspbNext) {

        /*
         * Get pspbNext now in case we free the SPB
         */
        pspbNext = pspb->pspbNext;

        /*
         * If pspb->spwnd is == pwnd or a child of pwnd, then free the SPB
         */
        for (pwndSpb = pspb->spwnd; pwndSpb; pwndSpb = pwndSpb->spwndParent) {

            if (pwnd == pwndSpb)
                FreeSpb(pspb);
        }
    }

    /*
     * Then see if any other SPBs are affected...
     */
    if (gpDispInfo->pspbFirst != NULL) {
        SpbCheckRect(pwnd, &pwnd->rcWindow, 0);
    }
}

/***************************************************************************\
* FreeSpb
*
* This function deletes the bitmap and region assocaited with a save popup
* bits structure and then unlinks and destroys the spb structure itself.
*
* History:
* 18-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

VOID FreeSpb(
    PSPB pspb)
{
    PSPB *ppspb;
    PDCE pdce;

    if (pspb == NULL)
        return;

    /*
     * Delete the bitmap.  If saved in screen memory, make special call.
     */
    if (pspb->flags & SPB_SAVESCREENBITS) {
        GreSaveScreenBits(gpDispInfo->hDev, SS_FREE, pspb->ulSaveId, NULL);
    } else if (pspb->hbm != NULL) {
        GreDeleteObject(pspb->hbm);
    }

    /*
     * Destroy the region.
     */
    if (pspb->hrgn != NULL){
        GreDeleteObject(pspb->hrgn);
    }

    /*
     * Forget that there is an attached SPB.
     */
    if (pspb->spwnd != NULL) {
        ClrWF(pspb->spwnd, WFHASSPB);
        Unlock(&pspb->spwnd);
    }

    /*
     * Unlink the spb.
     */
    ppspb = &gpDispInfo->pspbFirst;
    while (*ppspb != pspb) {
        ppspb = &(*ppspb)->pspbNext;
    }

    *ppspb = pspb->pspbNext;

    /*
     * Free the save popup bits structure.
     */
    UserFreePool(pspb);

    /*
     * If we no longer have any SPBs then turn off window MGR
     * bounds collection.
     */
    if (!AnySpbs()) {

        GreLockDisplay(gpDispInfo->hDev);

        /*
         * Reset the dirty areas of all of the DC's.  NULL means reset.
         */
        for (pdce = gpDispInfo->pdceFirst; pdce != NULL; pdce = pdce->pdceNext) {

            if (pdce->DCX_flags & DCX_LAYERED)
                continue;

            GreGetBounds(pdce->hdc, NULL, GGB_DISABLE_WINMGR);
        }

        GreUnlockDisplay(gpDispInfo->hDev);
    }

}

/***************************************************************************\
* FreeAllSpbs
*
* This function deletes all spb-bitmaps.
*
* History:
* 07-Oct-1995 ChrisWil  Ported from Chicago.
\***************************************************************************/

VOID FreeAllSpbs(void)
{

    while(AnySpbs()) {
        FreeSpb(gpDispInfo->pspbFirst);
    }

    gpDispInfo->pspbFirst = NULL;
}
