/****************************** Module Header ******************************\
* Module Name: paint.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the APIs used to begin and end window painting.
*
* History:
* 27-Oct-1990 DarrinM   Created.
* 12-Feb-1991 IanJa     HWND revalidation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* xxxFillWindow (not an API)
*
* pwndBrush - The brush is aligned with with client rect of this window.
*             It is usually either pwndPaint or pwndPaint's parent.
*
* pwndPaint - The window to paint.
* hdc       - The DC to paint in.
* hbr       - The brush to use.
*
* Returns TRUE if successful, FALSE if not.
*
* History:
* 15-Nov-1990 DarrinM   Ported from Win 3.0 sources.
* 21-Jan-1991 IanJa     Prefix '_' denoting exported function.
\***************************************************************************/

BOOL xxxFillWindow(
    PWND   pwndBrush,
    PWND   pwndPaint,
    HDC    hdc,
    HBRUSH hbr)
{
    RECT rc;

    CheckLock(pwndBrush);
    CheckLock(pwndPaint);

    /*
     * If there is no pwndBrush (sometimes the parent), use pwndPaint.
     */
    if (pwndBrush == NULL)
        pwndBrush = pwndPaint;

    if (UT_GetParentDCClipBox(pwndPaint, hdc, &rc))
        return xxxPaintRect(pwndBrush, pwndPaint, hdc, hbr, &rc);

    return TRUE;
}

/***************************************************************************\
* xxxPaintRect
*
* pwndBrush - The brush is aligned with with client rect of this window.
*             It is usually either pwndPaint or pwndPaint's parent.
*
* pwndPaint - The window to paint in.
* hdc       - The DC to paint in.
* hbr       - The brush to use.
* lprc      - The rectangle to paint.
*
* History:
* 15-Nov-1990 DarrinM   Ported from Win 3.0 sources.
* 21-Jan-1991 IanJa     Prefix '_' denoting exported function.
\***************************************************************************/

BOOL xxxPaintRect(
    PWND   pwndBrush,
    PWND   pwndPaint,
    HDC    hdc,
    HBRUSH hbr,
    LPRECT lprc)
{
    POINT ptOrg;

    CheckLock(pwndBrush);
    CheckLock(pwndPaint);

    if (pwndBrush == NULL) {
        pwndBrush = PtiCurrent()->rpdesk->pDeskInfo->spwnd;
    }

    if (pwndBrush == PWNDDESKTOP(pwndBrush)) {
        GreSetBrushOrg(
                hdc,
                0,
                0,
                &ptOrg);
    } else {
        GreSetBrushOrg(
                hdc,
                pwndBrush->rcClient.left - pwndPaint->rcClient.left,
                pwndBrush->rcClient.top - pwndPaint->rcClient.top,
                &ptOrg);
    }

    /*
     * If hbr < CTLCOLOR_MAX, it isn't really a brush but is one of our
     * special color values.  Translate it to the appropriate WM_CTLCOLOR
     * message and send it off to get back a real brush.  The translation
     * process assumes the CTLCOLOR*** and WM_CTLCOLOR*** values map directly.
     */
    if (hbr < (HBRUSH)CTLCOLOR_MAX) {
        hbr = xxxGetControlColor(pwndBrush,
                                 pwndPaint,
                                 hdc,
                                 HandleToUlong(hbr) + WM_CTLCOLORMSGBOX);
    }

    FillRect(hdc, lprc, hbr);

    GreSetBrushOrg(hdc, ptOrg.x, ptOrg.y, NULL);


    return TRUE;
}

/***************************************************************************\
* DeleteMaybeSpecialRgn
*
* Deletes a GDI region, making sure it is not a special region.
*
* History:
* 26-Feb-1992 MikeKe    from win3.1
\***************************************************************************/

VOID DeleteMaybeSpecialRgn(
    HRGN hrgn)
{
    if (hrgn > HRGN_SPECIAL_LAST) {
        GreDeleteObject(hrgn);
    }
}



/***************************************************************************\
* GetNCUpdateRgn
*
* Gets the update region which includes the non-client area.
*
* History:
* 26-Feb-1992 MikeKe    From win3.1
\***************************************************************************/

HRGN GetNCUpdateRgn(
    PWND pwnd,
    BOOL fValidateFrame)
{
    HRGN hrgnUpdate;

    if (pwnd->hrgnUpdate > HRGN_FULL) {

        /*
         * We must make a copy of our update region, because
         * it could change if we send a message, and we want to
         * make sure the whole thing is used for drawing our
         * frame and background.  We can't use a global
         * temporary region, because more than one app may
         * be calling this routine.
         */
        hrgnUpdate = CreateEmptyRgnPublic();

        if (hrgnUpdate == NULL) {
            hrgnUpdate = HRGN_FULL;
        } else if (CopyRgn(hrgnUpdate, pwnd->hrgnUpdate) == ERROR) {
            GreDeleteObject(hrgnUpdate);
            hrgnUpdate = HRGN_FULL;
        }

        if (fValidateFrame) {

            /*
             * Now that we've taken care of any frame drawing,
             * intersect the update region with the window's
             * client area.  Otherwise, apps that do ValidateRects()
             * to draw themselves (e.g., WinWord) won't ever
             * subtract off the part of the update region that
             * overlaps the frame but not the client.
             */
            CalcWindowRgn(pwnd, ghrgnInv2, TRUE);

            switch (IntersectRgn(pwnd->hrgnUpdate,
                                 pwnd->hrgnUpdate,
                                 ghrgnInv2)) {
            case ERROR:
                /*
                 * If an error occured, we can't leave things as
                 * they are: invalidate the whole window and let
                 * BeginPaint() take care of it.
                 */
                GreDeleteObject(pwnd->hrgnUpdate);
                pwnd->hrgnUpdate = HRGN_FULL;
                break;

            case NULLREGION:
                /*
                 * There is nothing in the client area to repaint.
                 * Blow the region away, and decrement the paint count
                 * if possible.
                 */
                GreDeleteObject(pwnd->hrgnUpdate);
                pwnd->hrgnUpdate = NULL;
                ClrWF(pwnd, WFUPDATEDIRTY);
                if (!TestWF(pwnd, WFINTERNALPAINT))
                    DecPaintCount(pwnd);
                break;
            }
        }

    } else {
        hrgnUpdate = pwnd->hrgnUpdate;
    }

    return hrgnUpdate;
}

/***************************************************************************\
* xxxSendNCPaint
*
* Sends a WM_NCPAINT message to a window.
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

VOID xxxSendNCPaint(
    PWND pwnd,
    HRGN hrgnUpdate)
{
    CheckLock(pwnd);

    /*
     * Clear the WFSENDNCPAINT bit...
     */
    ClrWF(pwnd, WFSENDNCPAINT);

    /*
     * If the window is active, but its FRAMEON bit hasn't
     * been set yet, set it and make sure that the entire frame
     * gets redrawn when we send the NCPAINT.
     */
    if ((pwnd == PtiCurrent()->pq->spwndActive) && !TestWF(pwnd, WFFRAMEON)) {
        SetWF(pwnd, WFFRAMEON);
        hrgnUpdate = HRGN_FULL;
        ClrWF(pwnd, WFNONCPAINT);
    }

    /*
     * If PixieHack() has set the WM_NCPAINT bit, we must be sure
     * to send with hrgnClip == HRGN_FULL.  (see PixieHack() in wmupdate.c)
     */
    if (TestWF(pwnd, WFPIXIEHACK)) {
        ClrWF(pwnd, WFPIXIEHACK);
        hrgnUpdate = HRGN_FULL;
    }

    if (hrgnUpdate)
        xxxSendMessage(pwnd, WM_NCPAINT, (WPARAM)hrgnUpdate, 0L);
}



/***************************************************************************\
* xxxSendChildNCPaint
*
* Sends WM_NCPAINT message to the immediate children of a window.
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

VOID xxxSendChildNCPaint(
    PWND pwnd)
{
    TL tlpwnd;

    CheckLock(pwnd);

    ThreadLockNever(&tlpwnd);
    pwnd = pwnd->spwndChild;
    while (pwnd != NULL) {
        if ((pwnd->hrgnUpdate == NULL) && TestWF(pwnd, WFSENDNCPAINT)) {
            ThreadLockExchangeAlways(pwnd, &tlpwnd);
            xxxSendNCPaint(pwnd, HRGN_FULL);
        }

        pwnd = pwnd->spwndNext;
    }

    ThreadUnlock(&tlpwnd);
}

/***************************************************************************\
* xxxBeginPaint
*
* Revalidation Note:
* We MUST return NULL if the window is deleted during xxxBeginPaint because
* its DCs are released upon deletion, and we shouldn't return a *released* DC!
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

HDC xxxBeginPaint(
    PWND          pwnd,
    LPPAINTSTRUCT lpps)
{
    HRGN hrgnUpdate;
    HDC  hdc;
    BOOL fSendEraseBkgnd;

    CheckLock(pwnd);
    UserAssert(IsWinEventNotifyDeferredOK());

    if (TEST_PUDF(PUDF_DRAGGINGFULLWINDOW))
        SetWF(pwnd, WFSTARTPAINT);

    /*
     * We're processing a WM_PAINT message: clear this flag.
     */
    ClrWF(pwnd, WFPAINTNOTPROCESSED);

    /*
     * If this bit gets set while we are drawing the frame we will need
     * to redraw it.
     *
     * If necessary, send our WM_NCPAINT message now.
     *
     * please heed these notes
     *
     * We have to send this message BEFORE we diddle hwnd->hrgnUpdate,
     * because an app may call ValidateRect or InvalidateRect in its
     * handler, and it expects what it does to affect what gets drawn
     * in the later WM_PAINT.
     *
     * It is possible to get an invalidate when we leave the critical
     * section below, therefore we loop until UPDATEDIRTY is clear
     * meaning there were no additional invalidates.
     */
    if (TestWF(pwnd, WFSENDNCPAINT)) {

        do {
            ClrWF(pwnd, WFUPDATEDIRTY);
            hrgnUpdate = GetNCUpdateRgn(pwnd, FALSE);
            xxxSendNCPaint(pwnd, hrgnUpdate);
            DeleteMaybeSpecialRgn(hrgnUpdate);
        } while (TestWF(pwnd, WFUPDATEDIRTY));

    } else {
        ClrWF(pwnd, WFUPDATEDIRTY);
    }

    /*
     * Hide the caret if needed.  Do this before we get the DC so
     * that if HideCaret() gets and releases a DC we will be able
     * to reuse it later here.
     * No need to DeferWinEventNotify() since pwnd is locked.
     */
    if (pwnd == PtiCurrent()->pq->caret.spwnd)
        zzzInternalHideCaret();

    /*
     * Send the check for sending an WM_ERASEBKGND to the
     * window.
     */
    if (fSendEraseBkgnd = TestWF(pwnd, WFSENDERASEBKGND)) {
        ClrWF(pwnd, WFERASEBKGND);
        ClrWF(pwnd, WFSENDERASEBKGND);
    }

    /*
     * Validate the entire window.
     */
    if (NEEDSPAINT(pwnd))
        DecPaintCount(pwnd);

    ClrWF(pwnd, WFINTERNALPAINT);

    hrgnUpdate = pwnd->hrgnUpdate;
    pwnd->hrgnUpdate = NULL;

    if (TestWF(pwnd, WFDONTVALIDATE)) {

        if (ghrgnUpdateSave == NULL) {
            ghrgnUpdateSave = CreateEmptyRgn();
        }

        if (ghrgnUpdateSave != NULL) {
            UnionRgn(ghrgnUpdateSave, ghrgnUpdateSave, hrgnUpdate);
            gnUpdateSave++;
        }
    }

    /*
     * Clear these flags for backward compatibility
     */
    lpps->fIncUpdate =
    lpps->fRestore   = FALSE;

    lpps->hdc =
    hdc       = _GetDCEx(pwnd,
                         hrgnUpdate,
                         DCX_USESTYLE | DCX_INTERSECTRGN);

    if (UT_GetParentDCClipBox(pwnd, hdc, &lpps->rcPaint)) {

        /*
         * If necessary, erase our background, and possibly deal with
         * our children's frames and backgrounds.
         */
        if (fSendEraseBkgnd)
            xxxSendEraseBkgnd(pwnd, hdc, hrgnUpdate);
    }

    /*
     * Now that we're completely erased, see if there are any children
     * that couldn't draw their own frames because their update regions
     * got deleted.
     */
    xxxSendChildNCPaint(pwnd);

    /*
     * The erase and frame operation has occured. Clear the WFREDRAWIFHUNG
     * bit here. We don't want to clear it until we know the erase and
     * frame has occured, so we know we always have a consistent looking
     * window.
     */
    ClearHungFlag(pwnd, WFREDRAWIFHUNG);

    lpps->fErase = (TestWF(pwnd, WFERASEBKGND) != 0);

    return hdc;
}

/***************************************************************************\
* xxxEndPaint (API)
*
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

BOOL xxxEndPaint(
    PWND          pwnd,
    LPPAINTSTRUCT lpps)
{
    CheckLock(pwnd);

    ReleaseCacheDC(lpps->hdc, TRUE);

    if (TestWF(pwnd, WFDONTVALIDATE)) {

        if (ghrgnUpdateSave != NULL) {

            InternalInvalidate3(pwnd,
                                ghrgnUpdateSave,
                                RDW_INVALIDATE | RDW_ERASE);

            if (--gnUpdateSave == 0) {
                GreDeleteObject(ghrgnUpdateSave);
                ghrgnUpdateSave = NULL;
            }
        }

        ClrWF(pwnd, WFDONTVALIDATE);
    }

    ClrWF(pwnd, WFWMPAINTSENT);

    /*
     * This used to check that the update-region was empty before
     * doing the clear.  However, this caused a problem with WOW
     * amipro/approach hanging.  They were invalidating rects in
     * their WM_PAINT handler, and allowing the defwindowproc to
     * perform the validation for them.  Since we were blocking
     * the BeginPaint in this case, it sent them into a infinite
     * loop (see bug 19036).
     */
    ClrWF(pwnd, WFSTARTPAINT);

    /*
     * Reshow the caret if needed, but AFTER we've released the DC.
     * This way ShowCaret() can reuse the DC we just released.
     */
    if (pwnd == PtiCurrent()->pq->caret.spwnd)
        zzzInternalShowCaret();

    return TRUE;
}

/***************************************************************************\
* InternalDoPaint
*
* Return a window equal to or below pwnd, created by the current thread,
* which needs painting.
*
* pwnd       - Window to start searching from. Search is depth first.
* ptiCurrent - The current thread.
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

PWND InternalDoPaint(
    PWND        pwnd,
    PTHREADINFO ptiCurrent)
{
    PWND pwndT;

    /*
     * Enumerate all windows, top-down, looking for one that
     * needs repainting.  Skip windows of other tasks.
     */
    for ( ; pwnd != NULL; pwnd = pwnd->spwndNext) {

        if (GETPTI(pwnd) == ptiCurrent) {

            if (NEEDSPAINT(pwnd)) {

                /*
                 * If this window is transparent, we don't want to
                 * send it a WM_PAINT until all its siblings below it
                 * have been repainted.  If we find an unpainted sibling
                 * below, return it instead.
                 */
                if (TestWF(pwnd, WEFTRANSPARENT)) {

                    pwndT = pwnd;
                    while ((pwndT = pwndT->spwndNext) != NULL) {

                        /*
                         * Make sure sibling window belongs to same app
                         */
                        if ((GETPTI(pwndT) == ptiCurrent) && NEEDSPAINT(pwndT)) {

                            if (TestWF(pwndT, WEFTRANSPARENT))
                                continue;

                            return pwndT;
                        }
                    }
                }

                return pwnd;
            }
        }

        if (pwnd->spwndChild &&
            (pwndT = InternalDoPaint(pwnd->spwndChild, ptiCurrent))) {

            return pwndT;
        }
    }

    return pwnd;
}

/***************************************************************************\
* DoPaint
*
* Looks at all the desktops for the window needing a paint and places a
* WM_PAINT in its queue.
*
* History:
* 16-Jul-91 DarrinM     Ported from Win 3.1 sources.
\***************************************************************************/

BOOL DoPaint(
    PWND  pwndFilter,
    LPMSG lpMsg)
{
    PWND        pwnd;
    PWND        pwndT;
    PTHREADINFO ptiCurrent = PtiCurrent();


#if 0 // CHRISWIL: WIN95 SPECIFIC

    /*
     * If there is a system modal up and it is attached to another task,
     * DON'T do paints.  We don't want to return a message for a window in
     * another task!
     */
    if (hwndSysModal && (hwndSysModal->hq != hqCurrent)) {

        /*
         * Poke this guy so he wakes up at some point in the future,
         * otherwise he may never wake up to realize he should paint.
         * Causes hangs - e.g. Photoshop installation program
         *   PostThreadMessage32(Lpq(hqCurrent)->idThread, WM_NULL, 0, 0, 0);
         */
        return FALSE;
    }

#endif

    /*
     * If this is a system thread, then walk the windowstation desktop-list
     * to find the window which needs painting.  For other threads, we
     * reference off the thread-desktop.
     */
    if (ptiCurrent->TIF_flags & TIF_SYSTEMTHREAD) {

        PWINDOWSTATION pwinsta;
        PDESKTOP       pdesk;

        if ((pwinsta = ptiCurrent->pwinsta) == NULL) {
            RIPMSG0(RIP_ERROR, "DoPaint: SYSTEMTHREAD does not have (pwinsta)");
            return FALSE;
        }

        pwnd = NULL;
        for(pdesk = pwinsta->rpdeskList; pdesk; pdesk = pdesk->rpdeskNext) {

            if (pwnd = InternalDoPaint(pdesk->pDeskInfo->spwnd, ptiCurrent))
                break;
        }

    } else {
        pwnd = InternalDoPaint(ptiCurrent->rpdesk->pDeskInfo->spwnd,
                               ptiCurrent);
    }

    if (pwnd != NULL) {

        if (!CheckPwndFilter(pwnd, pwndFilter))
            return FALSE;

        /*
         * We're returning a WM_PAINT message, so clear WFINTERNALPAINT so
         * it won't get sent again later.
         */
        if (TestWF(pwnd, WFINTERNALPAINT)) {

            ClrWF(pwnd, WFINTERNALPAINT);

            /*
             * If there is no update region, then no more paint for this
             * window.
             */
            if (pwnd->hrgnUpdate == NULL)
                DecPaintCount(pwnd);
        }

        /*
         * Set the STARTPAINT so that any other calls to BeginPaint while
         * painting is begin performed, will prevent painting on those
         * windows.
         *
         * Clear the UPDATEDIRTY since some apps (DBFast) don't call
         * GetUpdateRect, BeginPaint/EndPaint.
         */
        ClrWF(pwnd, WFSTARTPAINT);
        ClrWF(pwnd, WFUPDATEDIRTY);

        /*
         * If we get an invalidate between now and the time the app calls
         * BeginPaint() and the windows parent is not CLIPCHILDREN, then
         * the parent will paint in the wrong order.  So we are going to
         * cause the child to paint again.  Look in beginpaint and internal
         * invalidate for other parts of this fix.
         *
         * Set a flag to signify that we are in the bad zone.
         *
         * Must go up the parent links to make sure all parents have
         * WFCLIPCHILDREN set otherwise set the WFWMPAINTSENT flag.
         * This is to fix Excel spreadsheet and fulldrag. The speadsheet
         * parent window (class XLDESK) has WFCLIPCHILDREN set but it's
         * parent (class XLMAIN) doesn't. So the main window erases  the
         * background after the child window paints.
         *
         * JOHANNEC : 27-Jul-1994
         */
        
        /*
         * NT Bug 400167: As we walk up the tree, we need to stop short of
         * desktop windows and mother desktop windows.  We can't do a test
         * for WFCLIPCHILDREN on the mother desktop window's parent because
         * it doesn't exist.  This means that no desktop window will get 
         * WFWMPAINTSENT set, but the message window will be able to get 
         * WFWMPAINTSENT set.
         */
        
        pwndT = pwnd;
        while (pwndT && (GETFNID(pwndT) != FNID_DESKTOP)) {

            if (!TestWF(pwndT->spwndParent, WFCLIPCHILDREN)) {
                SetWF(pwnd, WFWMPAINTSENT);
                break;
            }

            pwndT = pwndT->spwndParent;
        }

        /*
         * If the top level "tiled" owner/parent of this window is iconed,
         * send a WM_PAINTICON rather than a WM_PAINT.  The wParam
         * is TRUE if this is the tiled window and FALSE if it is a
         * child/owned popup of the minimized window.
         *
         * BACKWARD COMPATIBILITY HACK
         *
         * 3.0 sent WM_PAINTICON with wParam == TRUE for no apparent
         * reason.  Lotus Notes 2.1 depends on this for some reason
         * to properly change its icon when new mail arrives.
         */
        if (!TestWF(pwnd, WFWIN40COMPAT) &&
            TestWF(pwnd, WFMINIMIZED)    &&
            (pwnd->pcls->spicn != NULL)) {

            StoreMessage(lpMsg, pwnd, WM_PAINTICON, (DWORD)TRUE, 0L, 0L);

        } else {

            StoreMessage(lpMsg, pwnd, WM_PAINT, 0, 0L, 0L);
        }

        return TRUE;
    }

    return FALSE;
}

/***************************************************************************\
* xxxSimpleDoSyncPaint
*
* Process the sync-paint for this window.  This can send either a NCPAINT
* or an ERASEBKGND.  This assumes no recursion and flags == 0.
*
* History:
* 26-Oct-1993 MikeKe    Created
\***************************************************************************/

VOID xxxSimpleDoSyncPaint(
    PWND pwnd)
{
    HRGN  hrgnUpdate;
    DWORD flags = 0;

    CheckLock(pwnd);

    /*
     * Since we're taking care of the frame drawing, we can consider
     * this WM_PAINT message processed.
     */
    ClrWF(pwnd, WFPAINTNOTPROCESSED);

    /*
     * Make copies of these flags, because their state might
     * change after we send a message, and we don't want
     * to "lose" them.
     */
    if (TestWF(pwnd, WFSENDNCPAINT))
        flags |= DSP_FRAME;

    if (TestWF(pwnd, WFSENDERASEBKGND))
        flags |= DSP_ERASE;

    if (flags & (DSP_ERASE | DSP_FRAME)) {

        if (!TestWF(pwnd, WFVISIBLE)) {

            /*
             * If there is no update region, just clear the bits.
             */
            ClrWF(pwnd, WFSENDNCPAINT);
            ClrWF(pwnd, WFSENDERASEBKGND);
            ClrWF(pwnd, WFPIXIEHACK);
            ClrWF(pwnd, WFERASEBKGND);
            ClearHungFlag(pwnd, WFREDRAWIFHUNG);

        } else {

            PTHREADINFO ptiCurrent = PtiCurrent();

            /*
             * If there is no update region, we don't have to
             * do any erasing, but we may need to send an NCPAINT.
             */
            if (pwnd->hrgnUpdate == NULL) {
                ClrWF(pwnd, WFSENDERASEBKGND);
                ClrWF(pwnd, WFERASEBKGND);
                flags &= ~DSP_ERASE;
            }

            /*
             * Only mess with windows owned by the current thread.
             * NOTE: This means that WM_NCPAINT and WM_ERASEBKGND are
             *       only sent intra-thread.
             */
            if (GETPTI(pwnd) == ptiCurrent) {

                hrgnUpdate = GetNCUpdateRgn(pwnd, TRUE);

                if (flags & DSP_FRAME) {

                    /*
                     * If the message got sent before we got here then do
                     * nothing.
                     */
                    if (TestWF(pwnd, WFSENDNCPAINT))
                        xxxSendNCPaint(pwnd, hrgnUpdate);
                }

                if (flags & DSP_ERASE) {

                    if (TestWF(pwnd, WFSENDNCPAINT)) {
                        /*
                         * If we got another invalidate during the NCPAINT
                         * callback get the new update region
                         */
                        DeleteMaybeSpecialRgn(hrgnUpdate);
                        hrgnUpdate = GetNCUpdateRgn(pwnd, FALSE);
                    }

                    /*
                     * If the message got sent before we got here
                     * (e.g.: an UpdateWindow() inside WM_NCPAINT handler,
                     * for example), don't do anything.
                     *
                     * WINPROJ.EXE (version 1.0) calls UpdateWindow() in
                     * the WM_NCPAINT handlers for its subclassed listboxes
                     * in the open dialog.
                     */
                    if (TestWF(pwnd, WFSENDERASEBKGND)) {
                        ClrWF(pwnd, WFSENDERASEBKGND);
                        ClrWF(pwnd, WFERASEBKGND);
                        xxxSendEraseBkgnd(pwnd, NULL, hrgnUpdate);
                    }

                    /*
                     * The erase and frame operation has occured. Clear the
                     * WFREDRAWIFHUNG bit here. We don't want to clear it until we
                     * know the erase and frame has occured, so we know we always
                     * have a consistent looking window.
                     */
                    ClearHungFlag(pwnd, WFREDRAWIFHUNG);
                }

                DeleteMaybeSpecialRgn(hrgnUpdate);

            } else if (!TestwndChild(pwnd)                         &&
                       (pwnd != grpdeskRitInput->pDeskInfo->spwnd) &&
                       FHungApp(GETPTI(pwnd), CMSHUNGAPPTIMEOUT)   &&
                       TestWF(pwnd, WFREDRAWIFHUNG)) {

                ClearHungFlag(pwnd, WFREDRAWIFHUNG);
                xxxRedrawHungWindow(pwnd, NULL);
            }
        }
    }
}

/***************************************************************************\
* xxxInternalDoSyncPaint
*
* Mostly the same functionality as the old xxxDoSyncPaint.
*
*
* This function is called to erase the background of a window, and
* possibly frame and erase the children too.
*
* WM_SYNCPAINT(wParam)/DoSyncPaint(flags) values:
*
* DSP_ERASE               - Erase background
* DSP_FRAME               - Draw child frames
* DSP_ENUMCLIPPEDCHILDREN - Recurse if children are clipped
* DSP_NOCHECKPARENTS      - Don't check
*
*
* Normally, only the DSP_ENUMCLIPPEDCHILDREN bit of flags is
* significant on entry.  If DSP_WM_SYNCPAINT is set, then hrgnUpdate
* and the rest of the flags bits are valid.
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

VOID xxxInternalDoSyncPaint(
    PWND  pwnd,
    DWORD flags)
{
    CheckLock(pwnd);

    /*
     * Do the paint for this window.
     */
    xxxSimpleDoSyncPaint(pwnd);

    /*
     * Normally we like to enumerate all of this window's children and have
     * them erase their backgrounds synchronously.  However, this is a bad
     * thing to do if the window is NOT CLIPCHLIDREN.  Here's the scenario
     * we want to to avoid:
     *
     * 1) Window 'A' is invalidated
     * 2) 'A' erases itself (or not, doesn't matter)
     * 3) 'A's children are enumerated and they erase themselves.
     * 4) 'A' paints over its children (remember, 'A' isn't CLIPCHILDREN)
     * 5) 'A's children paint but their backgrounds aren't their ERASEBKND
     *    color (because 'A' painted over them) and everything looks like
     *    dirt.
     */
    if ((flags & DSP_ALLCHILDREN) ||
        ((flags & DSP_ENUMCLIPPEDCHILDREN) && TestWF(pwnd, WFCLIPCHILDREN))) {

        TL   tlpwnd;
        PBWL pbwl;
        HWND *phwnd;

        if (pbwl = BuildHwndList(pwnd->spwndChild, BWL_ENUMLIST, NULL)) {

            PTHREADINFO ptiCurrent = PtiCurrent();
            HWND        hwnd;

            /*
             * If the client dies during a callback, the hwnd list
             * will be freed in xxxDestroyThreadInfo.
             */
            for (phwnd = pbwl->rghwnd; (hwnd = *phwnd) != (HWND)1; phwnd++) {

                if (hwnd == NULL)
                    continue;

                if ((pwnd = (PWND)RevalidateHwnd(hwnd)) == NULL)
                    continue;

                /*
                 * Note: testing if a window is a child automatically
                 * excludes the desktop window.
                 */
                if (TestWF(pwnd, WFCHILD) && (ptiCurrent != GETPTI(pwnd))) {

                    /*
                     * Don't cause any more intertask sendmessages cause it
                     * does bad things to cbt's windowproc hooks.  (Due to
                     * SetParent allowing child windows in the topwindow
                     * hierarchy.
                     */
                    continue;
                }

                /*
                 * Note that we pass only certain bits down as we recurse:
                 * the other bits pertain to the current window only.
                 */
                ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwnd);
                xxxInternalDoSyncPaint(pwnd, flags);
                ThreadUnlock(&tlpwnd);
            }

            FreeHwndList(pbwl);
        }
    }
}

/***************************************************************************\
* DoQueuedSyncPaint
*
* Queues WM_SYNCPAINT messages for top level windows not of the specified
* thread.
*
* History:
\***************************************************************************/

VOID DoQueuedSyncPaint(
    PWND        pwnd,
    DWORD       flags,
    PTHREADINFO pti)
{
    PTHREADINFO ptiPwnd = GETPTI(pwnd);

    if ((ptiPwnd != pti)          &&
        TestWF(pwnd, WFSENDNCPAINT)    &&
        TestWF(pwnd, WFSENDERASEBKGND) &&
        TestWF(pwnd, WFVISIBLE)) {

        PSMS psms = ptiPwnd->psmsReceiveList;

        /*
         * If this window already has a WM_SYNCPAINT queue'd up, then there's
         * no need to send another one.  Also protects our heap from getting
         * chewed up.
         */
        while (psms != NULL) {

            if ((psms->message == WM_SYNCPAINT) && (psms->spwnd == pwnd)) {
                break;
            }

            psms = psms->psmsReceiveNext;
        }

        if (psms == NULL) {
            /*
             * This will give this message the semantics of a notify
             * message (sendmessage no wait), without calling back
             * the WH_CALLWNDPROC hook. We don't want to do that
             * because that'll let all these processes with invalid
             * windows to process paint messages before they process
             * "synchronous" erasing or framing needs.
             *
             * Hi word of wParam must be zero or wow will drop it
             *
             * LATER mikeke
             * Do we need to send down the flags with DWP_ERASE and DSP_FRAME
             * in it?
             */
            UserAssert(HIWORD(flags) == 0);
            QueueNotifyMessage(pwnd, WM_SYNCPAINT, flags, 0);

            /*
             * Set our syncpaint-pending flag, since we queued one up.  This
             * will be used to check when we validate-parents for windows
             * without clipchildren.
             */
            SetWF(pwnd, WFSYNCPAINTPENDING);
        }

        /*
         * If we posted a WM_SYNCPAINT for a top-level window that is not
         * of the current thread we're done; we'll pick up the children
         * when we process the message for real.  If we're the desktop
         * however make sure we get all it children.
         */
        if (pwnd != PWNDDESKTOP(pwnd))
            return;
    }

    /*
     * Normally we like to enumerate all of this window's children and have
     * them erase their backgrounds synchronously.  However, this is a bad
     * thing to do if the window is NOT CLIPCHLIDREN.  Here's the scenario
     * we want to to avoid:
     *
     * 1.  Window 'A' is invalidated
     * 2.  'A' erases itself (or not, doesn't matter)
     * 3.  'A's children are enumerated and they erase themselves.
     * 4.  'A' paints over its children (remember, 'A' isn't CLIPCHILDREN)
     * 5.  'A's children paint but their backgrounds aren't their ERASEBKND
     *    color (because 'A' painted over them) and everything looks like
     *    dirt.
     */
    if ((flags & DSP_ALLCHILDREN) ||
        ((flags & DSP_ENUMCLIPPEDCHILDREN) && TestWF(pwnd, WFCLIPCHILDREN))) {

        PWND pwndT;

        for (pwndT = pwnd->spwndChild; pwndT; pwndT = pwndT->spwndNext) {

            /*
             * Don't cause any more intertask sendmessages cause it does
             * bad things to cbt's windowproc hooks.  (Due to SetParent
             * allowing child windows in the topwindow hierarchy.
             * The child bit also catches the desktop window; we want to
             */
            if (TestWF(pwndT, WFCHILD) && (pti != GETPTI(pwndT)))
                continue;

            /*
             * Note that we pass only certain bits down as we recurse:
             * the other bits pertain to the current window only.
             */
            DoQueuedSyncPaint(pwndT, flags, pti);
        }
    }
}

/***************************************************************************\
* xxxDoSyncPaint
*
* This funstion is only called for the initial syncpaint so we always
* queue syncpaints to other threads in this funtion.
*
* History:
\***************************************************************************/

VOID xxxDoSyncPaint(
    PWND  pwnd,
    DWORD flags)
{
    CheckLock(pwnd);

    /*
     * If any of our non-clipchildren parents have an update region, don't
     * do anything.  This way we won't redraw our background or frame out
     * of order, only to have it get obliterated when our parent erases his
     * background.
     */
    if (ParentNeedsPaint(pwnd))
        return;

    /*
     * First of all if we are going to be queueing any WM_SYNCPAINT messages
     * to windows of another thread do it first while the window's update
     * regions are still in sync.  This way there is no chance the update
     * region will be incorrect (through window movement during callbacks of
     * the WM_ERASEBKGND|WM_NCPAINT messages).
     */
    DoQueuedSyncPaint(pwnd, flags, PtiCurrent());
    xxxInternalDoSyncPaint(pwnd, flags);
}

/***************************************************************************\
* ParentNeedsPaint
*
* Return a non-zero PWND if a non-CLIPCHILDREN parent requires a WM_PAINT
* message.
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

PWND ParentNeedsPaint(
    PWND pwnd)
{
    while ((pwnd = pwnd->spwndParent) != NULL) {

        if (TestWF(pwnd, WFCLIPCHILDREN))
            break;

        if (NEEDSPAINT(pwnd))
            return pwnd;
    }

    return NULL;
}

/***************************************************************************\
* xxxSendEraseBkgnd
*
* Sends a WM_ERASEBKGROUND event to the window.  This contains the paintDC
* with the update-region selected into it.  If there's no update region
* then we prevent this event from making it to the window.
*
* History:
* 16-Jul-1991 DarrinM   Ported from Win 3.1 sources.
* 15-Dec-1996 ChrisWil  Merged Chicago Functionality (no erase on min).
\***************************************************************************/

BOOL xxxSendEraseBkgnd(
    PWND pwnd,
    HDC  hdcBeginPaint,
    HRGN hrgnUpdate)
{
    PTHREADINFO ptiCurrent;
    BOOL        fErased;
    HDC         hdc;

    CheckLock(pwnd);

    /*
     * For minimized dudes in win3.1, we would've sent an
     * WM_ICONERASEBKGND and cleared the erase bit.  Now that min
     * windows in 4.0 are all nonclient, don't bother erasing at
     * all.  Pretend like we did.
     *
     * NOTE:
     * For < 4.0 windows, we may have to send a fake WM_ICONERASEKBGND
     * to keep 'em happy.  Saves time not to though.  Getting a DC and
     * sending the message ain't speedy.
     */
    if ((hrgnUpdate == NULL) || TestWF(pwnd, WFMINIMIZED))
        return FALSE;

    /*
     * If a DC to use was not passed in, get one.
     * We want one clipped to this window's update region.
     */
    if (hdcBeginPaint == NULL) {

       hdc = _GetDCEx(pwnd,
                      hrgnUpdate,
                      DCX_USESTYLE | DCX_INTERSECTRGN | DCX_NODELETERGN);
    } else {

        hdc = hdcBeginPaint;
    }

    /*
     * If we're send the WM_ERASEBKGND to another process
     * we need to change the DC owner.
     *
     * We'd like to change the owner to pwnd->pti->idProcess, but
     * GDI won't let us assign ownership back to ourselves later.
     */
    ptiCurrent = PtiCurrent();

    if (GETPTI(pwnd)->ppi != ptiCurrent->ppi)
        GreSetDCOwner(hdc, OBJECT_OWNER_PUBLIC);

    /*
     * Send the event to the window.  This contains the DC clipped to
     * the update-region.
     */
    fErased = (BOOL)xxxSendMessage(pwnd, WM_ERASEBKGND, (WPARAM)hdc, 0L);

    /*
     * If we've changed the DC owner, change it back to
     * the current process.
     */
    if (GETPTI(pwnd)->ppi != ptiCurrent->ppi)
        GreSetDCOwner(hdc, OBJECT_OWNER_CURRENT);

    /*
     * If the WM_ERASEBKGND message did not erase the
     * background, then set this flag to let BeginPaint()
     * know to ask the caller to do it via the fErase
     * flag in the PAINTSTRUCT.
     */
    if (!fErased) {
        SetWF(pwnd, WFERASEBKGND);
        if (!TestWF(pwnd, WFWIN31COMPAT))
            SetWF(pwnd, WFSENDERASEBKGND);
    }

    /*
     * If we got a cache DC in this routine, release it.
     */
    if (hdcBeginPaint == NULL) {
        ReleaseCacheDC(hdc, TRUE);
    }

    return fErased;
}

/***************************************************************************\
* IncPaintCount
*
* EFFECTS:
* If cPaintsReady changes from 0 to 1, the QS_PAINT bit is set for
* associated queue and we wake up task so repaint will occur.
*
* IMPLEMENTATION:
* Get the queue handle from the window handle, bump the paint count, and
* if paint count is one, Set the wakebit.
*
* History:
* 17-Jul-1991 DarrinM   Translated Win 3.1 ASM code.
\***************************************************************************/

VOID IncPaintCount(
    PWND pwnd)
{
    PTHREADINFO pti = GETPTI(pwnd);

    if (pti->cPaintsReady++ == 0)
        SetWakeBit(pti, QS_PAINT);
}

/***************************************************************************\
* DecPaintCount
*
* EFFECTS:
* If cPaintsReady changes from 1 to 0, the QS_PAINT bit is cleared so
* that no more paints will occur.
*
* IMPLEMENTATION:
* Get the queue handle from the window handle, decrement the paint count,
* and if paint count is zero, clear the wakebit.
*
* History:
* 17-Jul-1991 DarrinM   Translated Win 3.1 ASM code.
\***************************************************************************/

VOID DecPaintCount(
    PWND pwnd)
{
    PTHREADINFO pti = GETPTI(pwnd);

    if (--pti->cPaintsReady == 0) {
        pti->pcti->fsWakeBits   &= ~QS_PAINT;
        pti->pcti->fsChangeBits &= ~QS_PAINT;
    }
}

/***************************************************************************\
* UT_GetParentDCClipBox
*
* Return rectangle coordinates of the parent clip-rect.  If the window
* isn't using a parentDC for drawing then return normal clipbox.
*
* History:
* 31-Oct-1990 DarrinM   Ported from Win 3.0 sources.
\***************************************************************************/

int UT_GetParentDCClipBox(
    PWND   pwnd,
    HDC    hdc,
    LPRECT lprc)
{
    RECT rc;

    if (GreGetClipBox(hdc, lprc, TRUE) == NULLREGION)
        return FALSE;

    if ((pwnd == NULL) || !TestCF(pwnd, CFPARENTDC))
        return TRUE;

    GetRect(pwnd, &rc, GRECT_CLIENT | GRECT_CLIENTCOORDS);

    return IntersectRect(lprc, lprc, &rc);
}

/***************************************************************************\
* UserRedrawDesktop
*
* Redraw the desktop and its children.  This is called from GDI for DCI
* related unlocks, so that all visrgns are recalculated for the apps.
*
* History:
* 08-Jan-1996 ChrisWil  Created.
\***************************************************************************/

VOID UserRedrawDesktop(VOID)
{
    TL   tlpwnd;
    PWND pwndDesk;

    EnterCrit();

    pwndDesk = PtiCurrent()->rpdesk->pDeskInfo->spwnd;

    ThreadLockAlways(pwndDesk, &tlpwnd);

    xxxInternalInvalidate(pwndDesk,
                          HRGN_FULL,
                          RDW_INVALIDATE |
                              RDW_ERASE  |
                              RDW_FRAME  |
                              RDW_ALLCHILDREN);

    ThreadUnlock(&tlpwnd);

    LeaveCrit();
}
