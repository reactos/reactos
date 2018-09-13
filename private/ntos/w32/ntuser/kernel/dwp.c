/****************************** Module Header ******************************\
* Module Name: dwp.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains xxxDefWindowProc and related functions.
*
* History:
* 10-22-90 DarrinM      Created stubs.
* 13-Feb-1991 mikeke    Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
*
*  DWP_DrawItem()
*
*  Does default WM_DRAWITEM handling.
*
\***************************************************************************/

void DWP_DrawItem(
    LPDRAWITEMSTRUCT lpdis)
{
    if (lpdis->CtlType == ODT_LISTBOX) {
        /*
         * Default OwnerDraw Listbox Item Drawing
         */
        if (   (lpdis->itemAction == ODA_FOCUS)
            || (   lpdis->itemAction == ODA_DRAWENTIRE
                && lpdis->itemState & ODS_FOCUS)
           ) {
            ClientFrame(lpdis->hDC, &lpdis->rcItem, gpsi->hbrGray, PATINVERT);
        }
    }
}


/***************************************************************************\
* xxxDWP_SetRedraw
*
*   This routine sets/resets the VISIBLE flag for windows who don't want any
*   redrawing.  Although a fast way of preventing paints, it is the apps
*   responsibility to reset this flag once they need painting.  Otherwise,
*   the window will be rendered transparent (could leave turds on the
*   screen).
*
*
* History:
* 07-24-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

void xxxDWP_SetRedraw(
    PWND pwnd,
    BOOL fRedraw)
{
    CheckLock(pwnd);
    UserAssert(IsWinEventNotifyDeferredOK());

    if (fRedraw) {
        if (!TestWF(pwnd, WFVISIBLE)) {
            SetVisible(pwnd, SV_SET);

            /*
             * We made this window visible - if it is behind any SPBs,
             * then we need to go invalidate them.
             *
             * We do this AFTER we make the window visible, so that
             * SpbCheckHwnd won't ignore it.
             */
            if (AnySpbs())
                SpbCheckPwnd(pwnd);

            /*
             * Now we need to invalidate/recalculate any affected cache entries
             * This call must be AFTER the window state change
             * No need to DeferWinEventNotify() since pwnd is threadlocked.
             */
            zzzInvalidateDCCache(pwnd, IDC_DEFAULT);

            /*
             * Because 3.1 sometimes doesn't draw window frames when 3.0 did,
             * we need to ensure that the frame gets drawn if the window is
             * later invalidated after a WM_SETREDRAW(TRUE)
             */
            SetWF(pwnd, WFSENDNCPAINT);
        }
    } else {
        if (TestWF(pwnd, WFVISIBLE)) {

            /*
             * Invalidate any SPBs.
             *
             * We do this BEFORE we make the window invisible, so
             * SpbCheckHwnd() won't ignore it.
             */
            if (AnySpbs())
                SpbCheckPwnd(pwnd);

            /*
             * Clear WFVISIBLE and delete any update regions lying around.
             */
            SetVisible(pwnd, SV_UNSET | (TestWF(pwnd, WFWIN31COMPAT) ? SV_CLRFTRUEVIS : 0));

            /*
             * Now we need to invalidate/recalc affected cache entries
             * This call must be AFTER the window state change
             * No need to DeferWinEventNotify() since we're about to return.
             */
            zzzInvalidateDCCache(pwnd, IDC_DEFAULT);
        }
    }
}


/***************************************************************************\
* DWP_GetEnabledPopup
*
* History:
* 10-28-90 MikeHar Ported from Windows.
\***************************************************************************/

PWND DWP_GetEnabledPopup(
    PWND pwndStart)
{
    PWND pwndT, pwnd;
    PTHREADINFO ptiStart;

    ptiStart = GETPTI(pwndStart);
    pwnd = pwndStart->spwndNext;

#ifdef SYSMODALWINDOWS
    if (gspwndSysModal)
        return NULL;
#endif

    /*
     * The user clicked on a window that is disabled. That window is pwndStart.
     * This loop is designed to evaluate what application this window is
     * associated with, and activate that "application", by finding what window
     * associated with that application can be activated. This is done by
     * enumerating top level windows, searching for a top level enabled
     * and visible ownee associated with this application.
     */
    while (pwnd != pwndStart) {
        if (pwnd == NULL) {

        /*
         * Warning! Win 3.1 had PWNDDESKTOP(pwndStart)->spwndChild
         * which could loop forever if pwndStart was a child window
         */
            pwnd = pwndStart->spwndParent->spwndChild;
            continue;
        }

        /*
         * We have two cases we need to watch out for here.  The first is when
         * applications call AssociateThreadInput() to tie two threads
         * together to share input state.  If the threads own the same queue,
         * then associate them together: this way, when two threads call
         * AttachThreadInput(), one created the main window, one created the
         * dialog window, when you click on the main window, they'll both
         * come to the top (rather than beeping).  In this case we want to
         * compare queues.  When Control Panel starts Setup in the Network
         * applet is one type of example of attached input.
         *
         * The second case is WOW apps.  All wow apps have the same queue
         * so to retain Win 3.1 compatibility, we want to treat each app
         * as an individual task (Win 3.1 tests hqs), so we will compare
         * PTI's for WOW apps.
         *
         * To see this case start 16 bit notepad and 16 bit write.  Do file
         * open on write and then give notepad the focus now click on write's
         * main window and the write file open dialog should activate.
         *
         * Another related case is powerpnt.  This case is interesting because
         * it tests that we do not find another window to activate when nested
         * windows are up and you click on a owner's owner.  Run Powerpnt, do
         * Edit-Insert-Picture and Object-Recolor Picture will bring up a
         * dialog with combos, drop down one of the color combo and then click
         * on powerpnt's main window - focus should stay with the dialogs
         * combo and it should stay dropped down.
         */
        if (((ptiStart->TIF_flags & TIF_16BIT) && (GETPTI(pwnd) == ptiStart)) ||
                (!(ptiStart->TIF_flags & TIF_16BIT) && (GETPTI(pwnd)->pq == ptiStart->pq))) {

            if (!TestWF(pwnd, WFDISABLED) && TestWF(pwnd, WFVISIBLE)) {
                pwndT = pwnd->spwndOwner;

                /*
                 * If this window is the parent of a popup window,
                 * bring up only one.
                 */
                while (pwndT) {
                    if (pwndT == pwndStart)
                        return pwnd;

                    pwndT = pwndT->spwndOwner;
                }

                /*
                 * Win9x continues looping only if pwnd is WEFTOPMOST. NT4 just returns, like Win3.1
                 * As soon as we find a window on the queue, we stop. So if the queue owns
                 * multiple top level unowned windows, then this code will probably not find
                 * the enabled popup. Note that owned windows are supposed to be on top of the
                 * owner, usally right on top of it (ie, pwnd->spwndNext == pwnd->spwndOwner)
                 * so this code used to find any other top level unowned windows before the enabled
                 * popup and bail. Odd.
                 * So let's continue looping. Hopefully this won't cause any compatibility problems
                 */
                // return NULL;
            }
        }
        pwnd = pwnd->spwndNext;
    }

    return NULL;
}
/***************************************************************************\
* xxxDWP_ProcessVirtKey
*
* History:
* 10-28-90 MikeHar      Ported from Windows.
\***************************************************************************/

void xxxDWP_ProcessVirtKey(
    UINT wKey)
{
    PTHREADINFO pti;
    TL tlpwndActive;

    pti = PtiCurrent();
    if (pti->pq->spwndActive == NULL)
        return;

    switch (wKey) {

    case VK_F4:
        if (TestCF(pti->pq->spwndActive, CFNOCLOSE))
            break;

        /*
         * Don't change the focus if the child window has it.
         */
        if (pti->pq->spwndFocus == NULL ||
                GetTopLevelWindow(pti->pq->spwndFocus) !=
                pti->pq->spwndActive) {
            ThreadLockAlwaysWithPti(pti, pti->pq->spwndActive, &tlpwndActive);
            xxxSetFocus(pti->pq->spwndActive);
            ThreadUnlock(&tlpwndActive);
        }
        _PostMessage(pti->pq->spwndActive, WM_SYSCOMMAND, SC_CLOSE, 0L);
        break;

    case VK_TAB:
        /*
         * If alt-tab is reserved by console, don't bring up the alt-tab
         * window.
         */
        if (GETPTI(pti->pq->spwndActive)->fsReserveKeys & CONSOLE_ALTTAB)
            break;

    case VK_ESCAPE:
    case VK_F6:
        ThreadLockAlwaysWithPti(pti, pti->pq->spwndActive, &tlpwndActive);
        xxxSendMessage(pti->pq->spwndActive, WM_SYSCOMMAND,
                (UINT)(_GetKeyState(VK_SHIFT) < 0 ? SC_NEXTWINDOW : SC_PREVWINDOW),
                        (LONG)(DWORD)(WORD)wKey);
        ThreadUnlock(&tlpwndActive);
       break;
    }
}


/***************************************************************************\
* xxxDWP_Paint
*
* Handle WM_PAINT and WM_PAINTICON messages.
*
* History:
* 07-24-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

void xxxDWP_Paint(
    PWND pwnd)
{
    PAINTSTRUCT ps;

    CheckLock(pwnd);

    /*
     * Bad handling of a WM_PAINT message, the application called
     * BeginPaint/EndPaint and is now calling DefWindowProc for the same
     * WM_PAINT message. Just return so we don't get full drag problems.
     * (Word and Excel do this).
     *
     * Added the check for empty-client-rects.  ObjectVision has a problem
     * with empty-windows being invalidated during a full-drag.  They used
     * to get blocked at the STARTPAINT and couldn't get through to
     * xxxBeginPaint to validate their update-rgn.
     *
     * i.e.
     *      a) Parent has a child-window with an empty rect.  On a full
     *         drag of the parent, we process SetWindowPos() to paint
     *         the new position.
     *
     *      b) During the parents processing of WM_PAINT, it calls
     *         GetUpdateRect() on the empty-child, which sets the STARTPAINT
     *         on its window.
     *
     *      c) On return to the parent WM_PAINT handler, it calls
     *         UpdateWindow() on the child, and used to get blocked here
     *         because the STARTPAINT bit was set.  The Child never gets
     *         updated, causing an infinite loop.
     *
     *      *) By checking for an empty-rectangle, we will let it through
     *         to validate.
     *
     */
    if (TestWF(pwnd, WFSTARTPAINT) && !IsRectEmpty(&(pwnd->rcClient))) {
        return;
    }

    if (xxxBeginPaint(pwnd, &ps)) {
        xxxEndPaint(pwnd, &ps);
    }
}


/***************************************************************************\
* xxxDWP_EraseBkgnd
*
*
* History:
* 07-24-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

BOOL xxxDWP_EraseBkgnd(
    PWND pwnd,
    UINT msg,
    HDC  hdc)
{
    HBRUSH hbr;

    CheckLock(pwnd);

    switch (msg) {
    case WM_ICONERASEBKGND:
        //
        // Old compatibility:  Many hack apps use this to paint the
        // desktop wallpaper.  We never send WM_ICONERASEBKGND anymore
        // because we don't have client areas in our minimized windows.
        //
        if (!TestWF(pwnd, WFCHILD)) {
            xxxInternalPaintDesktop(pwnd, hdc, TRUE);
        } else {
            return FALSE;
        }
        break;

    case WM_ERASEBKGND:
        if (hbr = pwnd->pcls->hbrBackground) {
            // Convert sys colors to proper brush
            if (hbr <= (HBRUSH)COLOR_MAX)
                hbr = SYSHBRUSH((ULONG_PTR)hbr - 1);

            /*
             * Remove call to UnrealizeObject.  GDI handles this
             * for brushes on NT.
             *
             * if (hbr != SYSHBR(DESKTOP))
             *     GreUnrealizeObject(hbr);
             */

            xxxFillWindow(pwnd, pwnd, hdc, hbr);
        } else {
            return FALSE;
        }
    }
    return TRUE;
}


/***************************************************************************\
* xxxDWP_SetCursorInfo
*
*
* History:
* 26-Apr-1994 mikeke    Created
\***************************************************************************/

/***************************************************************************\
* xxxDWP_SetCursor
*
*
* History:
* 07-24-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

BOOL xxxDWP_SetCursor(
    PWND pwnd,
    HWND hwndHit,
    int codeHT,
    UINT msg)
{
    PWND pwndParent, pwndPopup, pwndHit;
    PCURSOR pcur;
    LRESULT lt;
    TL tlpwndParent;
    TL tlpwndPopup;

    CheckLock(pwnd);
    UserAssert(IsWinEventNotifyDeferredOK());

    /*
     * wParam  == pwndHit == pwnd that cursor is over
     * lParamL == ht  == Hit test area code (result of WM_NCHITTEST)
     * lParamH == msg     == Mouse message number
     */
    if (msg)
    {
        switch (codeHT)
        {
        case HTLEFT:
        case HTRIGHT:
            pcur = SYSCUR(SIZEWE);
            break;
        case HTTOP:
        case HTBOTTOM:
            pcur = SYSCUR(SIZENS);
            break;
        case HTTOPLEFT:
        case HTBOTTOMRIGHT:
            pcur = SYSCUR(SIZENWSE);
            break;
        case HTTOPRIGHT:
        case HTBOTTOMLEFT:
            pcur = SYSCUR(SIZENESW);
            break;

        default:
            goto NotSize;
        }

        pwndHit = RevalidateHwnd(hwndHit);
        if (pwndHit == NULL)
            return FALSE;

        if (TestWF(pwndHit, WFSYSMENU)) {
            TL tlpwndHit;
            DWORD dwState;

            ThreadLockAlways(pwndHit, &tlpwndHit);
            dwState = _GetMenuState(
                    xxxGetSysMenu(pwndHit, TRUE), SC_SIZE, MF_BYCOMMAND);
            ThreadUnlock(&tlpwndHit);

            if ((dwState != (DWORD) -1) && (dwState & MFS_GRAYED))
                goto UseNormalCursor;
        }
        /*
         * No need to DeferWinEventNotify() - we're about to return
         */
        zzzSetCursor(pcur);
        return TRUE;
    }

NotSize:

    pwndParent = GetChildParent(pwnd);

    /*
     * Some windows (like the list boxes of comboboxes), are marked with
     * the child bit but are actually child of the desktop (can happen
     * if you call SetParent()). Make this special case check for
     * the desktop here.
     */
    if (pwndParent == PWNDDESKTOP(pwnd))
        pwndParent = NULL;

    if (pwndParent != NULL) {
        ThreadLockAlways(pwndParent, &tlpwndParent);
        lt = xxxSendMessage(pwndParent, WM_SETCURSOR, (WPARAM)hwndHit,
            MAKELONG(codeHT, msg));
        ThreadUnlock(&tlpwndParent);
        if (lt != 0)
            return TRUE;
    }

    if (msg == 0) {
        /*
         * No need to DeferWinEventNotify() - we're about to return
         */
        zzzSetCursor(SYSCUR(ARROW));

    } else {
        pwndHit = RevalidateHwnd(hwndHit);
        if (pwndHit == NULL)
            return FALSE;

        switch (codeHT) {
        case HTCLIENT:
            if (pwndHit->pcls->spcur != NULL) {
                /*
                 * No need to DeferWinEventNotify() - we're about to return
                 */
                zzzSetCursor(pwndHit->pcls->spcur);
            }
            break;

        case HTERROR:
            switch (msg) {
            case WM_MOUSEMOVE:
                if (TestUP(ACTIVEWINDOWTRACKING)) {
                    xxxActiveWindowTracking(pwnd, WM_SETCURSOR, codeHT);
                }
                break;

            case WM_LBUTTONDOWN:
                if ((pwndPopup = DWP_GetEnabledPopup(pwnd)) != NULL) {
                    if (pwndPopup != PWNDDESKTOP(pwnd)->spwndChild) {
                        PWND pwndActiveOld;

                        pwndActiveOld = PtiCurrent()->pq->spwndActive;

                        ThreadLockAlways(pwndPopup, &tlpwndPopup);

                        xxxSetWindowPos(pwnd, NULL, 0, 0, 0, 0,
                                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

                        xxxSetActiveWindow(pwndPopup);

                        ThreadUnlock(&tlpwndPopup);

                        if (pwndActiveOld != PtiCurrent()->pq->spwndActive)
                            break;

                        /*
                         *** ELSE FALL THRU **
                         */
                    }
                }

                /*
                 *** FALL THRU **
                 */

            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_XBUTTONDOWN:
                {
                    PWND pwndDlg;

                    pwndDlg = DWP_GetEnabledPopup(pwnd);

                    if (pwndDlg != NULL) {

                        ThreadLockAlways(pwndDlg, &tlpwndPopup);

                        xxxFlashWindow(pwndDlg,
                                       MAKELONG(FLASHW_ALL, UP(FOREGROUNDFLASHCOUNT)),
                                       (gpsi->dtCaretBlink >> 3));

                        ThreadUnlock(&tlpwndPopup);
                    }

                    xxxMessageBeep(0);

                    break;
                }
            }

            /*
             *** FALL THRU **
             */

        default:
UseNormalCursor:
            /*
             * No need to DeferWinEventNotify() - we're about to return
             */
            zzzSetCursor(SYSCUR(ARROW));
            break;
        }
    }

    return FALSE;
}


/***************************************************************************\
* xxxDWP_NCMouse
*
*
* History:
* 07-24-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

void xxxDWP_NCMouse(
    PWND pwnd,
    UINT msg,
    UINT ht,
    LPARAM lParam)
{
    UINT cmd;

    CheckLock(pwnd);

    cmd = 0;
    switch (msg) {
    case WM_NCLBUTTONDOWN:

        switch (ht) {
        case HTZOOM:
        case HTREDUCE:
        case HTCLOSE:
        case HTHELP:
            cmd = xxxTrackCaptionButton(pwnd, ht);
            break;

        default:
            // Change into a MV/SZ command
            if (ht >= HTSIZEFIRST && ht <= HTSIZELAST)
                cmd = SC_SIZE + (ht - HTSIZEFIRST + WMSZ_SIZEFIRST);
            break;
        }

        if (cmd != 0) {
            //
            // For SysCommands on system menu, don't do if menu item is
            // disabled.
            //
            if (   cmd != SC_CONTEXTHELP
                && TestWF(pwnd, WFSYSMENU)
                && !TestwndChild(pwnd)
               ) {
                if (_GetMenuState(xxxGetSysMenu(pwnd, TRUE), cmd & 0xFFF0,
                    MF_BYCOMMAND) & MFS_GRAYED)
                    break;
            }

            xxxSendMessage(pwnd, WM_SYSCOMMAND, cmd, lParam);
            break;
        }
        // FALL THRU

    case WM_NCLBUTTONUP:
    case WM_NCLBUTTONDBLCLK:
        xxxHandleNCMouseGuys(pwnd, msg, ht, lParam);
        break;
    }
}

/***************************************************************************\
*
* History:
* 09-Mar-1992 mikeke   From win3.1
\***************************************************************************/

UINT AreNonClientAreasToBePainted(
    PWND pwnd)
{
    WORD wRetValue = 0;

    /*
     * Check if Active and Inactive captions have same color
     */
    if (SYSRGB(ACTIVECAPTION) != SYSRGB(INACTIVECAPTION) ||
            SYSRGB(CAPTIONTEXT) != SYSRGB(INACTIVECAPTIONTEXT)) {
        wRetValue = DC_CAPTION;
    }

    /*
     * We want to repaint the borders if we're not minimized and
     * we have a sizing border and the active/inactive colors are
     * different.
     */
    if (!TestWF(pwnd, WFMINIMIZED) && TestWF(pwnd, WFSIZEBOX) &&
        (SYSRGB(ACTIVEBORDER) != SYSRGB(INACTIVEBORDER))) {
        // We need to redraw the sizing border.
        wRetValue |= DC_FRAME;
    }

    return wRetValue;
}

/***************************************************************************\
*
* History:
* 09-Mar-1992 mikeke   From win3.1
* 07-Aug-1996 vadimg   Added menu grayout and underline code
\***************************************************************************/

VOID xxxDWP_DoNCActivate(
    PWND pwnd,
    DWORD dwFlags,
    HRGN hrgnClip)
{
    UINT wFlags = DC_CAPTION;

    CheckLock(pwnd);

   /*
    * Later5.0 Gerardob. Since activation must follow focus, modeless
    *  menu windows are activated so they can receive keyboard input;
    *  however, we want the notification frame on, even when inactive.
    * (so it looks just like regular menus).
    * There are other scenarios when we want focus and activation on
    *  different parent-child chains so we should consider allowing this.
    */
    if ((dwFlags & NCA_ACTIVE)
            || (!(dwFlags & NCA_FORCEFRAMEOFF)
                && IsModelessMenuNotificationWindow(pwnd))) {

        SetWF(pwnd, WFFRAMEON);
        wFlags |= DC_ACTIVE;
    } else {

        ClrWF(pwnd, WFFRAMEON);
    }

    if (TestWF(pwnd, WFVISIBLE) && !TestWF(pwnd, WFNONCPAINT)) {

        HDC  hdc;
        WORD wBorderOrCap = (WORD)AreNonClientAreasToBePainted(pwnd);

        if (wBorderOrCap) {

            /*
             * Validate and Copy the region for our use.  Since we
             * hand this off to GetWindowDC() we won't have to delete
             * the region (done in ReleaseDC()).  Regardless, the region
             * passed in from the user is its responsibility to delete.
             */
            hrgnClip = UserValidateCopyRgn(hrgnClip);

            if (hdc = _GetDCEx(pwnd, hrgnClip, DCX_WINDOW | DCX_USESTYLE)) {
                 /*
                  * Draw the menu for grayout and underlines
                  */
                if (TestWF(pwnd, WFMPRESENT)) {
                    int cxFrame, cyFrame;
                    cxFrame = cyFrame = GetWindowBorders(pwnd->style,
                         pwnd->ExStyle, TRUE, FALSE);
                    cxFrame *= SYSMET(CXBORDER);
                    cyFrame *= SYSMET(CYBORDER);
                    xxxMenuBarDraw(pwnd, hdc, cxFrame, cyFrame);
                }
                xxxDrawCaptionBar(pwnd, hdc, wBorderOrCap | wFlags);
                _ReleaseDC(hdc);
            } else {
                GreDeleteObject(hrgnClip);
            }
        }
    }
}

/***************************************************************************\
*
* History:
* 09-Mar-1992 mikeke   From win3.1
\***************************************************************************/

BOOL xxxRedrawTitle(
    PWND pwnd, UINT wFlags)
{
    BOOL fDrawn = TRUE;

    CheckLock(pwnd);

    if (TestWF(pwnd, WFVISIBLE)) {

        if (TestWF(pwnd, WFBORDERMASK) == (BYTE)LOBYTE(WFCAPTION)) {

            HDC hdc = _GetWindowDC(pwnd);
            if (TestwndFrameOn(pwnd))
                wFlags |= DC_ACTIVE;
            xxxDrawCaptionBar(pwnd, hdc, wFlags);
            _ReleaseDC(hdc);
        }
        else
            fDrawn = FALSE;
    }

    if ( IsTrayWindow(pwnd) && (wFlags & (DC_ICON | DC_TEXT)) ) {
        HWND hw = HWq(pwnd);
        xxxCallHook(HSHELL_REDRAW, (WPARAM)hw, 0L, WH_SHELL);
        PostShellHookMessages(HSHELL_REDRAW, (LPARAM)hw);

    }
    return(fDrawn);
}

/***************************************************************************\
*
* History:
* 09-Mar-1992 mikeke   From win3.1
\***************************************************************************/

void xxxDWP_DoCancelMode(
    PWND pwnd)
{
    PTHREADINFO pti = PtiCurrent();
    PWND pwndCapture = pti->pq->spwndCapture;
    PMENUSTATE pMenuState;

    /*
     * If the below menu lines are changed in any way, then SQLWin
     * won't work if in design mode you drop some text, double click on
     * it, then try to use the heirarchical menus.
     */
    pMenuState = GetpMenuState(pwnd);
    if ((pMenuState != NULL)
            && (pwnd == pMenuState->pGlobalPopupMenu->spwndNotify)
            && !pMenuState->fModelessMenu) {

        xxxEndMenu(pMenuState);
    }

    if (pwndCapture == pwnd) {
        PSBTRACK pSBTrack = PWNDTOPSBTRACK(pwnd);
        if (pSBTrack && (pSBTrack->xxxpfnSB != NULL))
            xxxEndScroll(pwnd, TRUE);

        if (pti->pmsd != NULL) {
            pti->pmsd->fTrackCancelled = TRUE;
            pti->TIF_flags &= ~TIF_MOVESIZETRACKING;

            /*
             * Also clip the cursor back to the whole screen
             * so we don't get confused in xxxMoveSize.
             * This fix bug 64166.
             */
            zzzClipCursor((LPRECT)NULL);
        }

        /*
         * If the capture is still set, just release at this point.
         */
        xxxReleaseCapture();
    }
}

BOOL xxxDWPPrint(
    PWND   pwnd,
    HDC    hdc,
    LPARAM lParam)
{
    POINT  pt;
    int    iDC;
    LPRECT lprc;
    PWND   pwndSave = pwnd;
    LPARAM lParamSave = lParam;
    BOOL   fNotVisible;
    PBWL   pbwl;
    HWND   *phwnd;
    TL     tlpwnd;
#ifdef USE_MIRRORING
    DWORD  dwOldLayout;
    BOOL   bMirrorDC;
#endif
    int    iMirrorOffset=0;

    CheckLock(pwnd);

    if ((lParam & PRF_CHECKVISIBLE) && !_IsWindowVisible(pwnd))
        return(FALSE);

#ifdef USE_MIRRORING
    bMirrorDC = (TestWF(pwnd, WEFLAYOUTRTL) && !MIRRORED_HDC(hdc));
    /*
     * If the DC will be mirrored then we shift the clip rect by one
     * to include the right most pixel
     */
    if (bMirrorDC)
        iMirrorOffset=1;
#endif

    if (lParam & PRF_NONCLIENT) {

        /*
         * draw non-client area first
         */
        if (fNotVisible = !TestWF(pwnd, WFVISIBLE))
            SetVisible(pwnd, SV_SET);

        SetWF(pwnd, WFMENUDRAW);

#ifdef USE_MIRRORING
        if (bMirrorDC) {
            dwOldLayout = GreSetLayout(hdc, pwnd->rcWindow.right - pwnd->rcWindow.left, LAYOUT_RTL);
            xxxDrawWindowFrame(pwnd, hdc, FALSE, TestWF(pwnd, WFFRAMEON));
            GreSetLayout(hdc, pwnd->rcWindow.right - pwnd->rcWindow.left, dwOldLayout);
        } else
#endif
        {
            xxxDrawWindowFrame(pwnd, hdc, FALSE, TestWF(pwnd, WFFRAMEON));
        }

        ClrWF(pwnd, WFMENUDRAW);

        if (fNotVisible)
            SetVisible(pwnd, SV_UNSET);
    }

    if (lParam & PRF_CLIENT) {

        /*
         * draw client area second
         */
        iDC = GreSaveDC(hdc);
        GreGetWindowOrg(hdc, &pt);

        if (lParam & PRF_NONCLIENT) {
            int xBorders, yBorders;

            /*
             * adjust for non-client area
             */
            xBorders = pwnd->rcClient.left - pwnd->rcWindow.left;
            yBorders = pwnd->rcClient.top - pwnd->rcWindow.top;
            GreSetWindowOrg(hdc, pt.x - xBorders, pt.y - yBorders, NULL);
        }

        lprc = &pwnd->rcClient;
        GreIntersectClipRect(hdc, iMirrorOffset, 0, lprc->right - lprc->left + iMirrorOffset, lprc->bottom - lprc->top);

#ifdef USE_MIRRORING
        if (bMirrorDC) {
            dwOldLayout = GreSetLayout(hdc, pwnd->rcClient.right - pwnd->rcClient.left, LAYOUT_RTL);
            if (lParam & PRF_ERASEBKGND)
                xxxSendMessage(pwnd, WM_ERASEBKGND, (WPARAM) hdc, 0L);
            xxxSendMessage(pwnd, WM_PRINTCLIENT, (WPARAM) hdc, lParam);
            GreSetLayout(hdc, pwnd->rcClient.right - pwnd->rcClient.left, dwOldLayout);
        } else
#endif
        {

            if (lParam & PRF_ERASEBKGND)
                xxxSendMessage(pwnd, WM_ERASEBKGND, (WPARAM) hdc, 0L);
            xxxSendMessage(pwnd, WM_PRINTCLIENT, (WPARAM) hdc, lParam);
        }

        GreRestoreDC(hdc, iDC);

        pt.x += pwnd->rcWindow.left;
        pt.y += pwnd->rcWindow.top;

        if (lParam & PRF_CHILDREN) {

            /*
             * when drawing children, always include nonclient area
             */
            lParam |= PRF_NONCLIENT | PRF_ERASEBKGND;

            lParam &= ~PRF_CHECKVISIBLE;

            /*
             * draw children last
             */
            pbwl = BuildHwndList(pwnd->spwndChild, BWL_ENUMLIST, NULL);
            if (pbwl != NULL) {
                for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {
                    if ((pwnd = RevalidateHwnd(*phwnd)) == NULL)
                        continue;

                    if (TestWF(pwnd, WFVISIBLE)) {
                        lprc = &pwnd->rcWindow;
                        iDC = GreSaveDC(hdc);
                        GreSetWindowOrg(hdc, pt.x - lprc->left, pt.y - lprc->top, NULL);
                        if (!TestCF(pwnd, CFPARENTDC)) {
                            GreIntersectClipRect(hdc, 0, 0, lprc->right - lprc->left, lprc->bottom - lprc->top);
                        }
                        ThreadLockAlways(pwnd, &tlpwnd);
                        xxxSendMessage(pwnd, WM_PRINT, (WPARAM) hdc, lParam);
                        ThreadUnlock(&tlpwnd);
                        GreRestoreDC(hdc, iDC);
                    }
                }
                FreeHwndList(pbwl);
            }
        }

        if (lParam & PRF_OWNED) {
            pbwl = BuildHwndList((PWNDDESKTOP(pwnd))->spwndChild, BWL_ENUMLIST, NULL);
            if (pbwl != NULL) {
                for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {

                    if ((pwnd = RevalidateHwnd(*phwnd)) == NULL)
                        continue;

                    if ((pwnd->spwndOwner == pwndSave) && TestWF(pwnd, WFVISIBLE)) {
                        iDC = GreSaveDC(hdc);
                        GreSetWindowOrg(hdc, pt.x - pwnd->rcWindow.left, pt.y - pwnd->rcWindow.top, NULL);
                        ThreadLockAlways(pwnd, &tlpwnd);
                        xxxSendMessage(pwnd, WM_PRINT, (WPARAM) hdc, lParamSave);
                        ThreadUnlock(&tlpwnd);
                        GreRestoreDC(hdc, iDC);
                    }
                }
                FreeHwndList(pbwl);
            }
        }
    }

    return TRUE;
}



/***************************************************************************\
*
*  DWP_GetIcon()
*
*  Gets the small or big icon for a window.  For small icons, if we created
*  the thing, we don't let the app see it.
*
\***************************************************************************/

HICON DWP_GetIcon(
    PWND pwnd,
    UINT uType)
{
    HICON   hicoTemp;

    if (uType < ICON_SMALL || uType > ICON_BIG)
    {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "WM_GETICON: Invalid wParam value (0x%X)", uType);
        return (HICON)NULL;
    }

    /*
     *  Get the icon from the window
     */
    hicoTemp = (HICON)_GetProp(pwnd,
                              MAKEINTATOM(uType == ICON_SMALL ? gpsi->atomIconSmProp : gpsi->atomIconProp),
                              PROPF_INTERNAL);

    /*
     *  If it's a USER created small icon don't return it.
     */
    if (uType == ICON_SMALL && hicoTemp) {
        PCURSOR pcurTemp;

        pcurTemp = (PCURSOR)HMValidateHandleNoRip((HCURSOR)hicoTemp, TYPE_CURSOR);
        if (pcurTemp != NULL && (pcurTemp->CURSORF_flags & CURSORF_SECRET)) {
            hicoTemp = (HICON)NULL;
        }
    }

    return hicoTemp;
}


/***************************************************************************\
*
*  DestroyWindowSmIcon()
*
*  Destroys the small icon of a window if we've created a cached one.
*  This is  because it's called in winrare.c when the caption height
*  changes.
*
\***************************************************************************/

BOOL DestroyWindowSmIcon(
    PWND pwnd)
{
    HCURSOR hcursor;
    PCURSOR pcursor;

    //
    // Get the small icon property first...
    //
    hcursor = (HCURSOR)_GetProp(pwnd, MAKEINTATOM(gpsi->atomIconSmProp), PROPF_INTERNAL);
    if (hcursor == NULL)
        return FALSE;

    pcursor = (PCURSOR)HMValidateHandleNoRip(hcursor, TYPE_CURSOR);
    if (pcursor == NULL)
        return FALSE;

    //
    // Remove it if it's a secretly created one
    //

    if (pcursor->CURSORF_flags & CURSORF_SECRET)
    {
        ClrWF(pwnd, WFSMQUERYDRAGICON);
        InternalRemoveProp(pwnd, MAKEINTATOM(gpsi->atomIconSmProp), PROPF_INTERNAL);
        _DestroyCursor(pcursor, CURSOR_ALWAYSDESTROY);
        return(TRUE);
    }
    else
        return(FALSE);
}


/***************************************************************************\
*
*  xxxDWP_SetIcon()
*
*  Sets the small or big icon for a window, and returns back the previous
*  one.
*
\***************************************************************************/

HICON xxxDWP_SetIcon(
    PWND   pwnd,
    WPARAM wType,
    HICON  hicoNew)
{
    HICON   hIcon;
    HICON   hIconSm;
    HICON   hOld;
    BOOL    fRedraw;

    CheckLock(pwnd);

#if DBG
    if (hicoNew && !IS_PTR(hicoNew)) {
        RIPMSG1(RIP_WARNING, "WM_SETICON: Icon handle missing HIWORD (0x%08X)", hicoNew);
    }
#endif

    if (wType < ICON_SMALL || wType > ICON_RECREATE)
    {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "WM_SETICON: Invalid wParam value (0x%0X)", wType);
        return (HICON)NULL;
    }

    /*
     *  Regenerate small icons if requested.
     */
    if (wType == ICON_RECREATE) {
        xxxRecreateSmallIcons(pwnd);
        return 0L;
    }

    /*
     *  Save old icon
     */
    hIcon = (HICON)_GetProp(pwnd, MAKEINTATOM(gpsi->atomIconProp), PROPF_INTERNAL);
    hIconSm = (HICON)_GetProp(pwnd, MAKEINTATOM(gpsi->atomIconSmProp), PROPF_INTERNAL);
    hOld = ((wType == ICON_SMALL) ? hIconSm : hIcon);

    /*
     * Only update the icons if they have changed
     */
    if (hOld != hicoNew)
    {
        PCURSOR pcursor;
        BOOL fWasCache = FALSE;

        fRedraw = TRUE;

        /*
         *  Always remove the small icon because it is either being replaced or
         *  will be recreated if the big icon is being set.
         */
        pcursor = (PCURSOR)HMValidateHandleNoRip(hIconSm, TYPE_CURSOR);
        if (pcursor && (pcursor->CURSORF_flags & CURSORF_SECRET)) {
            fWasCache = TRUE;
            _DestroyCursor(pcursor, CURSOR_ALWAYSDESTROY);
        }

        if (wType == ICON_SMALL) {
            /*
             *  Apps never see the icons that USER creates behind their backs
             *  from big icons.
             */
            if (fWasCache)
                hOld = NULL;

            hIconSm = hicoNew;
        } else {
            if (fWasCache) {
                /*
                 * Force us to recalc the small icon to match the new big icon
                 */
                hIconSm = NULL;
            } else if (hIconSm) {
                /*
                 * Redraw of the caption isn't needed because the small icon
                 * didn't change.
                 */
                fRedraw = FALSE;
            }

            hIcon = hicoNew;
        }


        /*
         *  Store the icons off the window as properties
         */
        InternalSetProp(pwnd, MAKEINTATOM(gpsi->atomIconProp), (HANDLE)hIcon, PROPF_INTERNAL | PROPF_NOPOOL);
        InternalSetProp(pwnd, MAKEINTATOM(gpsi->atomIconSmProp), (HANDLE)hIconSm, PROPF_INTERNAL | PROPF_NOPOOL);

        /*
         *  Create the small icon if it doesn't exist.
         */
        if (hIcon && !hIconSm)
            xxxCreateWindowSmIcon(pwnd, hIcon, TRUE);

        /*
         * Redraw caption if the small icon has changed
         */
        if (fRedraw)
            xxxRedrawTitle(pwnd, DC_ICON);
    }
    return hOld;
}

// --------------------------------------------------------------------------
//
//  CreateWindowSmIcon()
//
//  Makes a per-window small icon copy of a big icon.
//
// --------------------------------------------------------------------------
HICON xxxCreateWindowSmIcon(
    PWND pwnd,
    HICON hIconBig,
    BOOL fNotQueryDrag)
{
    HICON   hIconSm = NULL;
    PCURSOR pcurs = NULL,pcursBig;

    CheckLock(pwnd);
    UserAssert(hIconBig);

    pcursBig = (PCURSOR)HMValidateHandleNoRip(hIconBig, TYPE_CURSOR);

    if (pcursBig) {
        pcurs = xxxClientCopyImage(PtoHq(pcursBig),
                        pcursBig->rt == PTR_TO_ID(RT_ICON) ? IMAGE_ICON : IMAGE_CURSOR,
                        SYSMET(CXSMICON),
                        SYSMET(CYSMICON),
                        LR_DEFAULTCOLOR | (fNotQueryDrag ? LR_COPYFROMRESOURCE : 0));
        if (pcurs != NULL)
            hIconSm = PtoHq(pcurs);
    }
    if (hIconSm) {
        pcurs->CURSORF_flags |= CURSORF_SECRET;
        InternalSetProp(pwnd, MAKEINTATOM(gpsi->atomIconSmProp), (HANDLE)hIconSm, PROPF_INTERNAL | PROPF_NOPOOL);
        if (!fNotQueryDrag)
            SetWF(pwnd, WFSMQUERYDRAGICON);
    }

    return(hIconSm);
}


/***************************************************************************\
* xxxDefWindowProc (API)
*
* History:
* 10-23-90 MikeHar Ported from WaWaWaWindows.
* 12-07-90 IanJa   CTLCOLOR handling round right way
\***************************************************************************/

LRESULT xxxDefWindowProc(
    PWND pwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    LRESULT     lt;
    PWND        pwndT;
    TL          tlpwndParent;
    TL          tlpwndT;
    int         icolBack;
    int         icolFore;
    int         i;

    CheckLock(pwnd);

    if (pwnd == (PWND)-1) {
        return 0;
    }

    if (message > WM_USER) {
        return 0;
    }

    /*
     * Important:  If you add cases to the switch statement below,
     *             and those messages can originate on the client
     *             side, add the messages to server.c's gawDefWindowMsgs
     *             array or else the client will short-circuit the call
     *             and return 0.
     */

    switch (message) {
    case WM_CLIENTSHUTDOWN:
        return xxxClientShutdown(pwnd, wParam);

    case WM_NCACTIVATE:
        xxxDWP_DoNCActivate(pwnd, (LOWORD(wParam) ? NCA_ACTIVE : 0), (HRGN)lParam);
        return (LONG)TRUE;

    case WM_NCHITTEST:
        return FindNCHit(pwnd, (LONG)lParam);

    case WM_NCCALCSIZE:

        /*
         * wParam = fCalcValidRects
         * lParam = LPRECT rgrc[3]:
         *        lprc[0] = rcWindowNew = New window rectangle
         *    if fCalcValidRects:
         *        lprc[1] = rcWindowOld = Old window rectangle
         *        lprc[2] = rcClientOld = Old client rectangle
         *
         * On return:
         *        rgrc[0] = rcClientNew = New client rect
         *    if fCalcValidRects:
         *        rgrc[1] = rcValidDst  = Destination valid rectangle
         *        rgrc[2] = rcValidSrc  = Source valid rectangle
         */
        xxxCalcClientRect(pwnd, (LPRECT)lParam, FALSE);
        break;

    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONUP:
    case WM_NCLBUTTONDBLCLK:
        xxxDWP_NCMouse(pwnd, message, (UINT)wParam, lParam);
        break;

    case WM_CANCELMODE:
        {
            /*
             * Terminate any modes the system might
             * be in, such as scrollbar tracking, menu mode,
             * button capture, etc.
             */
            xxxDWP_DoCancelMode(pwnd);
        }
        break;

    case WM_NCCREATE:
        if (TestWF(pwnd, (WFHSCROLL | WFVSCROLL))) {
            if (_InitPwSB(pwnd) == NULL)
                return (LONG)FALSE;
        }

#ifdef FE_SB // xxxDefWindowProc()
        /*
         * If CREATESTRUCTEX.strName contains resource id, we don't
         * need to call DefSetText(). because it is a numeric number,
         * it does not need Ansi <-> Unicode translation.
         */
        if (lParam) {
            PLARGE_STRING pstr = &((PCREATESTRUCTEX)lParam)->strName;

            if (pwnd->head.rpdesk == NULL || pstr == NULL || pstr->Buffer == NULL) {
                pwnd->strName.Length = 0;
                return TRUE;
            }

            if ((pstr->bAnsi && (pstr->Length >= sizeof(BYTE)) &&
                                (*(PBYTE)(pstr->Buffer) == 0xff)) ||
                (!pstr->bAnsi && (pstr->Length >= sizeof(WCHAR)) &&
                                (*(PWCHAR)(pstr->Buffer) == 0xffff))) {
               /*
                * This is Resource ID, we just return here with TRUE.
                */
               return (LONG)TRUE;
            }
        }
#endif // FE_SB

        SetWF(pwnd, WFTITLESET);

        return (LONG)DefSetText(pwnd, &((PCREATESTRUCTEX)lParam)->strName);

    case WM_PRINT:
            return((LRESULT)xxxDWPPrint(pwnd, (HDC) wParam, lParam));

    case WM_NCPAINT:
        {
            HDC hdc;
            /*
             * Force the drawing of the menu.
             */
            SetWF(pwnd, WFMENUDRAW);

            /*
             * Get a window DC intersected with hrgnClip,
             * but make sure that hrgnClip doesn't get deleted.
             */
            hdc = _GetDCEx(pwnd,
                           (HRGN)wParam,
                           DCX_USESTYLE         |
                               DCX_WINDOW       |
                               DCX_INTERSECTRGN |
                               DCX_NODELETERGN  |
                               DCX_LOCKWINDOWUPDATE);

            xxxDrawWindowFrame(pwnd,
                               hdc,
                               FALSE,
                               (TestWF(pwnd, WFFRAMEON) &&
                                   (GETPTI(pwnd)->pq == gpqForeground)));

            _ReleaseDC(hdc);
            ClrWF(pwnd, WFMENUDRAW);
        }
        break;

    case WM_ISACTIVEICON:
        return TestWF(pwnd, WFFRAMEON) != 0;

    case WM_SETTEXT:
        /*
         * At one time we added an optimization to do nothing if the new
         * text was the same as the old text but found that QCcase does not work
         * because it calls SetWindowText not to change the text but
         * cause the title bar to redraw after it had added the sysmenu
         * through SetWindowLong
         */
        if (lt = DefSetText(pwnd, (PLARGE_STRING)lParam)) {
            /*
             * Text was set, so redraw title bar
             */
            xxxRedrawTitle(pwnd, DC_TEXT);
            if (FWINABLE()) {
                xxxWindowEvent(EVENT_OBJECT_NAMECHANGE, pwnd, OBJID_WINDOW, INDEXID_CONTAINER, 0);
            }
        }
        return lt;

    case WM_GETTEXT:
        if (wParam != 0) {
            PLARGE_STRING pstr = (PLARGE_STRING)lParam;

            if (pwnd->strName.Length) {
                if (pstr->bAnsi) {
                    i = WCSToMB(pwnd->strName.Buffer,
                            pwnd->strName.Length / sizeof(WCHAR),
                            (LPSTR *)&pstr->Buffer, pstr->MaximumLength - 1, FALSE);
                    ((LPSTR)pstr->Buffer)[i] = 0;
                    pstr->Length = i;
                } else {
                    i = TextCopy(&pwnd->strName, pstr->Buffer, (UINT)wParam);
                    pstr->Length = i * sizeof(WCHAR);
                }
                return i;
            }

            /*
             * else Null terminate the text buffer since there is no text.
             */
            if (pstr->bAnsi) {
                *(LPSTR)pstr->Buffer = 0;
            } else {
                *(LPWSTR)pstr->Buffer = 0;
            }
        }
        return 0L;

    case WM_GETTEXTLENGTH:
        if (pwnd->strName.Length) {
            UINT cch;
            if (lParam) {
                RtlUnicodeToMultiByteSize(&cch,
                                          pwnd->strName.Buffer,
                                          pwnd->strName.Length);
            } else {
                cch = pwnd->strName.Length / sizeof(WCHAR);
            }
            return cch;
        }
        return 0L;

    case WM_CLOSE:
        xxxDestroyWindow(pwnd);
        break;

    case WM_PAINT:
    case WM_PAINTICON:
        xxxDWP_Paint(pwnd);
        break;

    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
        return (LONG)xxxDWP_EraseBkgnd(pwnd, message, (HDC)wParam);

    case WM_SYNCPAINT:

        /*
         * Clear our sync-paint pending flag.
         */
        ClrWF(pwnd, WFSYNCPAINTPENDING);

        /*
         * This message is sent when SetWindowPos() is trying
         * to get the screen looking nice after window rearrangement,
         * and one of the windows involved is of another task.
         * This message avoids lots of inter-app message traffic
         * by switching to the other task and continuing the
         * recursion there.
         *
         * wParam         = flags
         * LOWORD(lParam) = hrgnClip
         * HIWORD(lParam) = pwndSkip  (not used; always NULL)
         *
         * pwndSkip is now always NULL.
         *
         * NOTE: THIS MESSAGE IS FOR INTERNAL USE ONLY! ITS BEHAVIOR
         * IS DIFFERENT IN 3.1 THAN IN 3.0!!
         */
        xxxInternalDoSyncPaint(pwnd, (DWORD)wParam);
        break;

    case WM_QUERYOPEN:
    case WM_QUERYENDSESSION:
    case WM_DEVICECHANGE:
    case WM_POWERBROADCAST:
        return (LONG)TRUE;

    // Default handling for WM_CONTEXTMENU support
    case WM_RBUTTONUP:
#ifdef USE_MIRRORING
        if (TestWF(pwnd, WEFLAYOUTRTL)) {
            lParam = MAKELPARAM(pwnd->rcClient.right - GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) + pwnd->rcClient.top);
        } else
#endif
        {
                        lParam = MAKELONG(GET_X_LPARAM(lParam) + pwnd->rcClient.left, GET_Y_LPARAM(lParam) + pwnd->rcClient.top);
        }
        xxxSendMessage(pwnd, WM_CONTEXTMENU, (WPARAM) HWq(pwnd), lParam);
        break;

    case WM_NCRBUTTONDOWN:
        {
            int         nHit;
            MSG         msg;
            LONG        spt;
            PTHREADINFO pti = PtiCurrent();

            nHit = FindNCHit(pwnd, (LONG)lParam);
            if (nHit == HTVSCROLL || nHit == HTHSCROLL) {
                if (!_IsDescendant(pti->pq->spwndActive, pwnd)) {
                    break;
                }
            } else if (nHit == HTCAPTION || nHit == HTSYSMENU) {
                if (pwnd != pti->pq->spwndActive) {
                    break;
                }
            } else {
                break;
            }

            xxxSetCapture(pwnd);

            while (TRUE)
            {
                if (xxxPeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE))
                {
                    if (msg.message == WM_RBUTTONUP)
                    {
                        xxxReleaseCapture();
                        spt = POINTTOPOINTS(msg.pt);
                        nHit = FindNCHit(pwnd, spt);
                        if ((nHit == HTCAPTION) || (nHit == HTSYSMENU) ||
                            (nHit == HTVSCROLL) || (nHit == HTHSCROLL)) {
                            xxxSendMessage(pwnd, WM_CONTEXTMENU, (WPARAM) HWq(pwnd), spt);
                        }
                        break;
                    }
                }
                if (pwnd != pti->pq->spwndCapture)
                // Someone else grabbed the capture.  Bail out.
                    break;
//                xxxWaitMessage();
                if (!xxxSleepThread(QS_MOUSE, 0, TRUE))
                    break;
            }
        }
        break;

    /*
     * Default handling for WM_APPCOMMAND support
     */
    case WM_NCXBUTTONUP:
    case WM_XBUTTONUP:
        {
            WORD cmd;
            WORD keystate;
            LPARAM lParamAppCommand;

            cmd = 0;
            switch (GET_XBUTTON_WPARAM(wParam)) {
            case XBUTTON1:
                cmd = APPCOMMAND_BROWSER_BACKWARD;
                break;

            case XBUTTON2:
                cmd = APPCOMMAND_BROWSER_FORWARD;
                break;

            default:
                break;
            }

            if (cmd == 0) {
                break;
            }

            cmd |= FAPPCOMMAND_MOUSE;
            if (message == WM_XBUTTONUP) {
                keystate = GET_KEYSTATE_WPARAM(wParam);
            } else {
                keystate = (WORD)GetMouseKeyFlags(PtiCurrent()->pq);
            }

            lParamAppCommand = MAKELPARAM(keystate, cmd);
            xxxSendMessage(pwnd, WM_APPCOMMAND, (WPARAM) HWq(pwnd), lParamAppCommand);
            break;
        }

    case WM_MOUSEWHEEL:
        if (TestwndChild(pwnd)) {
            ThreadLockAlways(pwnd->spwndParent, &tlpwndParent);
            xxxSendMessage(pwnd->spwndParent, WM_MOUSEWHEEL, wParam, lParam);
            ThreadUnlock(&tlpwndParent);
        }
        break;

    case WM_CONTEXTMENU:
        {
            int nHit;

            nHit = FindNCHit(pwnd, (LONG)lParam);

            /*
             * Put up a context menu if we clicked on a scroll bar
             */
            if ((nHit == HTVSCROLL) || (nHit == HTHSCROLL)) {
                if (_IsDescendant(PtiCurrent()->pq->spwndActive, pwnd)) {
                    xxxDoScrollMenu(pwnd, NULL, nHit - HTHSCROLL, lParam);
                }
                break;
            }

            if (TestwndChild(pwnd)) {
                ThreadLockAlways(pwnd->spwndParent, &tlpwndParent);
                xxxSendMessage(pwnd->spwndParent, WM_CONTEXTMENU, (WPARAM) HWq(pwnd), lParam);
                ThreadUnlock(&tlpwndParent);
            } else {
                /*
                 *  Do default context menu if right clicked on caption
                 */
                if (pwnd == PtiCurrent()->pq->spwndActive)
                {
                    if (nHit == HTCAPTION)
                        goto DoTheDefaultThang;
                    else if (nHit == HTSYSMENU)
                    {
                        i = SC_CLOSE;
                        goto DoTheSysMenuThang;
                    }

                    /*
                     *  If this was generated by the keyboard (apps key), then simulate a shift-f10
                     *  for old apps so they get a crack at putting up their context menu.
                     */
                    if (lParam == KEYBOARD_MENU && !TestWF(pwnd, WFWIN40COMPAT))
                        xxxSimulateShiftF10();
                }
            }
        }
        break;

    case WM_APPCOMMAND:
        /*
         * Bubble the message to the parent
         */
        if (TestwndChild(pwnd)) {
            ThreadLockAlways(pwnd->spwndParent, &tlpwndParent);
            lt = xxxSendMessage(pwnd->spwndParent, WM_APPCOMMAND, wParam, lParam);
            ThreadUnlock(&tlpwndParent);
            return lt;
        } else if (pwnd != PWNDDESKTOP(pwnd) ) {
            /*
             * Notify listeners on the SHELLHOOK that a WM_APPCOMMAND message was not handled
             * We also post this message to the shell queue so they don't need to load themselves 
             * into every process with a hook.
             * We don't bother about the desktop since csrss services it and it doesn't accept 
             * shell hooks so there is no point.
             */
            if (IsHooked(PtiCurrent(), WHF_SHELL)) 
                xxxCallHook(HSHELL_APPCOMMAND, wParam, lParam, WH_SHELL);
            
            /*
             * Still post messages to the shell window even if there isn't a hook installed 
             */
            PostShellHookMessages(HSHELL_APPCOMMAND, lParam);
        }
        break;

    case WM_KEYF1:
        xxxSendHelpMessage(pwnd, HELPINFO_WINDOW,
                (int) (TestwndChild(pwnd) ? PTR_TO_ID(pwnd->spmenu) : 0),
                HWq(pwnd), GetContextHelpId(pwnd));
        break;

    case WM_SYSCOMMAND:
        xxxSysCommand(pwnd, (UINT)wParam, lParam);
        break;

    case WM_KEYDOWN:
        if (wParam == VK_F10) {
            PtiCurrent()->pq->QF_flags |= QF_FF10STATUS;
HandleF10:
         /*
          *  Generate a WM_CONTEXTMENU for new apps for shift-f10.
          */
             if (_GetKeyState(VK_SHIFT) < 0 && TestWF(pwnd, WFWIN40COMPAT)) {
                 xxxSendMessage(pwnd, WM_CONTEXTMENU, (WPARAM)HWq(pwnd), KEYBOARD_MENU);
             }
        }
        break;

    case WM_HELP:
        // If this window is a child window, Help message must be passed on
        // to it's parent; Else, this must be passed on to the owner window.
        pwndT = (TestwndChild(pwnd)? pwnd->spwndParent : pwnd->spwndOwner);
        if (pwndT && (pwndT != _GetDesktopWindow())) {
            ThreadLockAlways(pwndT, &tlpwndT);
            lt = xxxSendMessage(pwndT, WM_HELP, wParam, lParam);
            ThreadUnlock(&tlpwndT);
            return lt;
        }
        return 0L;

    case WM_SYSKEYDOWN:
        {
            PTHREADINFO pti = PtiCurrent();

            /*
             * Is the ALT key down?
             */
            if (HIWORD(lParam) & SYS_ALTERNATE) {
                /*
                 * Toggle QF_FMENUSTATUS iff this is NOT a repeat KEYDOWN
                 * message; Only if the prev key state was 0, then this is the
                 * first KEYDOWN message and then we consider toggling menu
                 * status; Fix for Bugs #4531 & #4566 --SANKAR-- 10-02-89.
                 */
                if ((HIWORD(lParam) & SYS_PREVKEYSTATE) == 0) {

                    /*
                     * Don't have to lock pwndActive because it's
                     * processing this key.
                     */
                    if ((wParam == VK_MENU) &&
                            !(pti->pq->QF_flags & QF_FMENUSTATUS)) {
                        pti->pq->QF_flags |= QF_FMENUSTATUS;
                        xxxDrawMenuBarUnderlines(pwnd, TRUE);
                    } else {
                        pti->pq->QF_flags &= ~(QF_FMENUSTATUS|QF_FMENUSTATUSBREAK);
                    }
                }

                pti->pq->QF_flags &= ~QF_FF10STATUS;

                xxxDWP_ProcessVirtKey((UINT)wParam);

            } else {
                if (wParam == VK_F10) {
                    pti->pq->QF_flags |= QF_FF10STATUS;
                    goto HandleF10;
                }
            }
        }
        break;

    case WM_SYSKEYUP:
    case WM_KEYUP:
        {
            PTHREADINFO pti = PtiCurrent();

            /*
             * press and release F10 or ALT.  Send this only to top-level windows,
             * otherwise MDI gets confused.  The fix in which DefMDIChildProc()
             * passed up the message was insufficient in the case a child window
             * of the MDI child had the focus.
             * Also make sure the sys-menu activation wasn't broken by a mouse
             * up or down when the Alt was down (QF_MENUSTATUSBREAK).
             */
            if ((wParam == VK_MENU && !(pti->pq->QF_flags & QF_TABSWITCHING) && ((pti->pq->QF_flags &
                    (QF_FMENUSTATUS | QF_FMENUSTATUSBREAK)) == QF_FMENUSTATUS)) ||
                    (wParam == VK_F10 && (pti->pq->QF_flags & QF_FF10STATUS ))) {
                pwndT = GetTopLevelWindow(pwnd);
                if (gspwndFullScreen != pwndT) {

                    ThreadLockWithPti(pti, pwndT, &tlpwndT);
                    /*
                     * Draw the underlines for F10. This was already down for ALT
                     *  when the key went down.
                     */
                    if (wParam == VK_F10) {
                        xxxDrawMenuBarUnderlines(pwnd, TRUE);
                    }
                    xxxSendMessage(pwndT, WM_SYSCOMMAND, SC_KEYMENU, 0);
                    ThreadUnlock(&tlpwndT);
                }
            }

            /*
             * Turn off bit for tab-switching.  This is set in the _KeyEvent()
             * routine when it's been determined we're doing switching.  This
             * is necessary for cases where the ALT-KEY is release before the
             * TAB-KEY.  In which case, the FMENUSTATUS bit would be cleared
             * by the ALT-KEY-UP and would have forced us into a syscommand
             * loop.  This guarentees that we don't enter that condition.
             */
            if (wParam == VK_MENU) {
                pti->pq->QF_flags &= ~QF_TABSWITCHING;
                xxxDrawMenuBarUnderlines(pwnd, FALSE);
            }

            pti->pq->QF_flags &= ~(QF_FMENUSTATUS | QF_FMENUSTATUSBREAK | QF_FF10STATUS);
        }
        break;

    case WM_SYSCHAR:
        {
            PTHREADINFO pti = PtiCurrent();

            /*
             * If syskey is down and we have a char...
             */
            pti->pq->QF_flags &= ~(QF_FMENUSTATUS | QF_FMENUSTATUSBREAK);

            if (wParam == VK_RETURN && TestWF(pwnd, WFMINIMIZED)) {

                /*
                 * If the window is iconic and user hits RETURN, we want to
                 * restore this window.
                 */
                _PostMessage(pwnd, WM_SYSCOMMAND, SC_RESTORE, 0L);
                break;
            }

            if ((HIWORD(lParam) & SYS_ALTERNATE) && wParam) {
                if (wParam == VK_TAB || wParam == VK_ESCAPE)
                    break;

                /*
                 * Send ALT-SPACE only to top-level windows.
                 */
                if ((wParam == MENUSYSMENU) && (TestwndChild(pwnd))) {
                    ThreadLockAlwaysWithPti(pti, pwnd->spwndParent, &tlpwndParent);
                    xxxSendMessage(pwnd->spwndParent, message, wParam, lParam);
                    ThreadUnlock(&tlpwndParent);
                } else {
                    xxxSendMessage(pwnd, WM_SYSCOMMAND, SC_KEYMENU, (DWORD)wParam);
                }
            } else {

                /*
                 * Ctrl-Esc produces a WM_SYSCHAR, But should not beep;
                 */
                if (wParam != VK_ESCAPE)
                    xxxMessageBeep(0);
            }
        }
        break;

    case WM_CHARTOITEM:
    case WM_VKEYTOITEM:

        /*
         * Do default processing for keystrokes into owner draw listboxes.
         */
        return -1L;

    case WM_ACTIVATE:
        if (wParam)
            xxxSetFocus(pwnd);
        break;

    case WM_INPUTLANGCHANGEREQUEST:
    {
        PWND pwndFocus = PtiCurrent()->pq->spwndFocus;

        /*
         * #115190
         * Dialog does not forward I.L.Reqest to the focused window.
         * (Memphis compatible issue)
         */
        if (pwndFocus && (pwndFocus != pwnd) &&
                pwnd->pcls->atomClassName != gpsi->atomSysClass[ICLS_DIALOG]) {
            /*
             * pass message to focus'ed window. Old app, pass on to
             * focus'ed window which may be ML aware.  (edit class
             * for example).
             */
            ThreadLockAlways(pwndFocus, &tlpwndT);
            xxxSendMessage(pwndFocus, message, wParam, lParam);
            ThreadUnlock(&tlpwndT);
        } else if (!xxxActivateKeyboardLayout(_GetProcessWindowStation(NULL),
                (HKL)lParam, KLF_SETFORPROCESS, pwnd)) {
            RIPERR1(ERROR_INVALID_KEYBOARD_HANDLE, RIP_WARNING, "WM_INPUTLANGCHANGEREQUEST: Invalid keyboard handle (0x%08lx)", lParam);
        }
        break;
    }

    case WM_INPUTLANGCHANGE:
    {
        PBWL pbwl;
        HWND *phwnd;
        TL tlpwnd;

        pbwl = BuildHwndList(pwnd->spwndChild, BWL_ENUMLIST, NULL);
        if (pbwl == NULL)
            return 0;

        for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {
            /*
             * Make sure this hwnd is still around.
             */
            if ((pwnd = RevalidateHwnd(*phwnd)) == NULL)
                continue;

            ThreadLockAlways(pwnd, &tlpwnd);
            RIPMSG1(RIP_VERBOSE, "WM_INPUTLANGCHANGE: Sending message to pwnd %#p", pwnd);
            xxxSendMessage(pwnd, message, wParam, lParam);
            ThreadUnlock(&tlpwnd);
        }
        FreeHwndList(pbwl);

        break;
    }

    case WM_SETREDRAW:
        xxxDWP_SetRedraw(pwnd, wParam != 0);
        break;

    case WM_WINDOWPOSCHANGING:
        {
            /*
             * If the window's size is changing, adjust the passed-in size
             */
            WINDOWPOS *ppos = ((WINDOWPOS *)lParam);
            if (!(ppos->flags & SWP_NOSIZE)) {
                xxxAdjustSize(pwnd, &ppos->cx, &ppos->cy);
            }
        }
        break;

    case WM_WINDOWPOSCHANGED:
        xxxHandleWindowPosChanged(pwnd, (PWINDOWPOS)lParam);
        break;

    case WM_CTLCOLORSCROLLBAR:
        if (gpsi->BitCount < 8 ||
                SYSRGB(3DHILIGHT) != SYSRGB(SCROLLBAR) ||
                SYSRGB(3DHILIGHT) == SYSRGB(WINDOW))
        {
            /*
             * Remove call to UnrealizeObject.  GDI handles this
             * for brushes on NT.
             *
             * GreUnrealizeObject(ghbrGray);
             */

            GreSetBkColor((HDC)wParam, SYSRGB(3DHILIGHT));
            GreSetTextColor((HDC)wParam, SYSRGB(3DFACE));
            return((LRESULT)gpsi->hbrGray);
        }

        icolBack = COLOR_3DHILIGHT;
        icolFore = COLOR_BTNTEXT;
        goto SetColor;

    case WM_CTLCOLORBTN:
        if (TestWF(pwnd, WFWIN40COMPAT)) {
            icolBack = COLOR_3DFACE;
            icolFore = COLOR_BTNTEXT;
        } else {
            goto ColorDefault;
        }
        goto SetColor;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORMSGBOX:
        // We want static controls in dialogs to have the 3D
        // background color, but statics in windows to inherit
        // their parents' background.
        if (TestWF(pwnd, WFWIN40COMPAT)
           ) {
            icolBack = COLOR_3DFACE;
            icolFore = COLOR_WINDOWTEXT;
            goto SetColor;
        }
        // ELSE FALL THRU...

    case WM_CTLCOLOR:              // here for WOW only
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLOREDIT:
ColorDefault:
        icolBack = COLOR_WINDOW;
        icolFore = COLOR_WINDOWTEXT;

SetColor:
        GreSetBkColor((HDC)wParam, gpsi->argbSystem[icolBack]);
        GreSetTextColor((HDC)wParam, gpsi->argbSystem[icolFore]);
        return (LRESULT)(SYSHBRUSH(icolBack));

    case WM_SETCURSOR:

        /*
         * wParam  == pwndHit == pwnd that cursor is over
         * lParamL == ht  == Hit test area code (result of WM_NCHITTEST)
         * lParamH == msg     == Mouse message number
         */
        return (LONG)xxxDWP_SetCursor(pwnd, (HWND)wParam, (int)(SHORT)lParam,
                HIWORD(lParam));

    case WM_MOUSEACTIVATE:
        pwndT = GetChildParent(pwnd);
        if (pwndT != NULL) {
            ThreadLockAlways(pwndT, &tlpwndT);
            lt = xxxSendMessage(pwndT, WM_MOUSEACTIVATE, wParam, lParam);
            ThreadUnlock(&tlpwndT);
            if (lt != 0)
                return lt;
        }

        /*
         * Moving, sizing or minimizing? Activate AFTER we take action.
         * If the user LEFT clicked in the title bar, don't activate now:
         */
        return  (   (LOWORD(lParam) == HTCAPTION)
                 && (HIWORD(lParam) == WM_LBUTTONDOWN)
                )
              ? (LONG)MA_NOACTIVATE
              : (LONG)MA_ACTIVATE;

    case WM_SHOWWINDOW:

        /*
         * If we are being called because our owner window is being shown,
         * hidden, minimized, or un-minimized, then we must hide or show
         * show ourself as appropriate.
         *
         * This behavior occurs for popup windows or owned windows only.
         * It's not designed for use with child windows.
         */
        if (LOWORD(lParam) != 0 && (TestwndPopup(pwnd) || pwnd->spwndOwner)) {

            /*
             * The WFHIDDENPOPUP flag is an internal flag that indicates
             * that the window was hidden because its owner was hidden.
             * This way we only show windows that were hidden by this code,
             * not intentionally by the application.
             *
             * Go ahead and hide or show this window, but only if:
             *
             * a) we need to be hidden, or
             * b) we need to be shown, and we were hidden by
             *    an earlier WM_SHOWWINDOW message
             */
            if ((!wParam && TestWF(pwnd, WFVISIBLE)) ||
                    (wParam && !TestWF(pwnd, WFVISIBLE) &&
                    TestWF(pwnd, WFHIDDENPOPUP))) {

                /*
                 * Remember that we were hidden by WM_SHOWWINDOW processing
                 */
                ClrWF(pwnd, WFHIDDENPOPUP);
                if (!wParam)
                    SetWF(pwnd, WFHIDDENPOPUP);

                xxxShowWindow(
                        pwnd,
                        (wParam ? SW_SHOWNOACTIVATE : SW_HIDE) | TEST_PUDF(PUDF_ANIMATE));
            }
        }
        break;

    case WM_SYSMENU:
        if (   !TestWF(pwnd, WFDISABLED)
            && (   (GETPTI(pwnd)->pq == gpqForeground)
                || xxxSetForegroundWindow(pwnd, FALSE))
           )
        {
            PMENU pMenu;
            TL tpmenu;
DoTheDefaultThang:
            if (TestWF(pwnd, WFMAXIMIZED) || TestWF(pwnd, WFMINIMIZED))
                i = SC_RESTORE;
            else
                i = SC_MAXIMIZE;

DoTheSysMenuThang:
            if ((pMenu = xxxGetSysMenu(pwnd, TRUE)) != NULL)
            {
                _SetMenuDefaultItem(pMenu, i, MF_BYCOMMAND);

                // Tell the shell we are bringing it up the system menu
                PostShellHookMessages(HSHELL_SYSMENU, (LPARAM)HWq(pwnd));

                ThreadLockAlways(pMenu, &tpmenu);
                if (lParam == 0xFFFFFFFF)
                {
                    // this is a keyboard generated WM_SYSMENU
                    if (FDoTray())
                    {
                        TPMPARAMS tpm;

                        tpm.cbSize = sizeof(TPMPARAMS);

                        if (xxxSendMinRectMessages(pwnd, &tpm.rcExclude)) {
                            xxxTrackPopupMenuEx(pMenu, TPM_SYSMENU | TPM_VERTICAL,
                                tpm.rcExclude.left, tpm.rcExclude.top, pwnd, &tpm);
                        }
                    }
                }
                else
                {
                    xxxTrackPopupMenuEx(pMenu, TPM_RIGHTBUTTON | TPM_SYSMENU,
                        GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), pwnd, NULL);
                }
                ThreadUnlock(&tpmenu);
            }
        }
        break;

    case WM_DRAWITEM:
        DWP_DrawItem((LPDRAWITEMSTRUCT)lParam);
        break;

    case WM_GETHOTKEY:
        return (LONG)DWP_GetHotKey(pwnd);
        break;

    case WM_SETHOTKEY:
        return (LONG)DWP_SetHotKey(pwnd, (DWORD)wParam);
        break;

    case WM_GETICON:
        return (LRESULT)DWP_GetIcon(pwnd, (BOOL)wParam);

    case WM_SETICON:
        return (LRESULT)xxxDWP_SetIcon(pwnd, wParam, (HICON)lParam);

    case WM_COPYGLOBALDATA:
        /*
         * This message is used to thunk WM_DROPFILES messages along
         * with other things.  If we end up here with it, directly
         * call the client back to finish processing of this message.
         * This assumes that the ultimate destination of the
         * WM_DROPFILES message is in the client side's process context.
         */
        return(SfnCOPYGLOBALDATA(NULL, 0, wParam, lParam, 0, 0, 0, NULL));

    case WM_QUERYDROPOBJECT:
        /*
         * if the app has registered interest in drops, return TRUE
         */
        return (LRESULT)(TestWF(pwnd, WEFACCEPTFILES) ? TRUE : FALSE);

    case WM_DROPOBJECT:
        return DO_DROPFILE;

    case WM_ACCESS_WINDOW:
        if (ValidateHwnd((HWND)wParam)) {
            // SECURITY: set ACL for this window to no-access
            return TRUE;
        }
        return FALSE;

    case WM_NOTIFYFORMAT:
        if(lParam == NF_QUERY)
            return(TestWF(pwnd, WFANSICREATOR) ? NFR_ANSI : NFR_UNICODE);
        break;

    case WM_CHANGEUISTATE:
        {
            WORD wAction = LOWORD(wParam);
            WORD wFlags = HIWORD(wParam);
            BOOL bRealChange = FALSE;

            if (wFlags & ~UISF_VALID || wAction > UIS_LASTVALID ||
                lParam || !TEST_KbdCuesPUSIF) {
                    return 0;
            }

            if (wAction == UIS_INITIALIZE) {
                if (gpsi->bLastRITWasKeyboard) {
                    wAction = UIS_CLEAR;
                } else {
                    wAction = UIS_SET;
                }
                wFlags = UISF_HIDEFOCUS | UISF_HIDEACCEL;
                wParam = MAKEWPARAM(wAction, wFlags);
            }

            UserAssert(wAction == UIS_SET || wAction == UIS_CLEAR);
            /*
             * If the state is not going to change, there's nothing to do here
             */
            if (wFlags & UISF_HIDEFOCUS) {
                bRealChange = (!!TestWF(pwnd, WEFPUIFOCUSHIDDEN)) ^ (wAction == UIS_SET);
            }
            if (wFlags & UISF_HIDEACCEL) {
                bRealChange |= (!!TestWF(pwnd, WEFPUIACCELHIDDEN)) ^ (wAction == UIS_SET);
            }

            if (!bRealChange) {
                break;
            }

            /*
             * Children pass this message up
             * Top level windows update their children's state and
             * send down to their imediate children WM_UPDATEUISTATE.
             */
            if (TestwndChild(pwnd)) {
                ThreadLockAlways(pwnd->spwndParent, &tlpwndParent);
                lt = xxxSendMessage(pwnd->spwndParent, WM_CHANGEUISTATE, wParam, lParam);
                ThreadUnlock(&tlpwndParent);
                return lt;
            } else {
                return xxxSendMessage(pwnd, WM_UPDATEUISTATE, wParam, lParam);
            }

        }
        break;

    case WM_QUERYUISTATE:
        return (TestWF(pwnd, WEFPUIFOCUSHIDDEN) ? UISF_HIDEFOCUS : 0) |
               (TestWF(pwnd, WEFPUIACCELHIDDEN) ? UISF_HIDEACCEL : 0);
        break;

    case WM_UPDATEUISTATE:
        {
            WORD wAction = LOWORD(wParam);
            WORD wFlags = HIWORD(wParam);

            if (wFlags & ~UISF_VALID || wAction > UIS_LASTVALID ||
                lParam || !TEST_KbdCuesPUSIF) {
                    return 0;
            }

            switch (wAction) {
                case UIS_INITIALIZE:
                    /*
                     * UISTATE: UIS_INITIALIZE sets the UIState bits
                     * based on the last input type
                     */
                    if (!gpsi->bLastRITWasKeyboard) {
                        SetWF(pwnd, WEFPUIFOCUSHIDDEN);
                        SetWF(pwnd, WEFPUIACCELHIDDEN);
                        wParam = MAKEWPARAM(UIS_SET, UISF_HIDEACCEL | UISF_HIDEFOCUS);
                    } else {
                        ClrWF(pwnd, WEFPUIFOCUSHIDDEN);
                        ClrWF(pwnd, WEFPUIACCELHIDDEN);
                        wParam = MAKEWPARAM(UIS_CLEAR, UISF_HIDEACCEL | UISF_HIDEFOCUS);
                    }
                    break;

                case UIS_SET:
                    if (wFlags & UISF_HIDEACCEL) {
                        SetWF(pwnd, WEFPUIACCELHIDDEN);
                    }
                    if (wFlags & UISF_HIDEFOCUS) {
                        SetWF(pwnd, WEFPUIFOCUSHIDDEN);
                    }
                    break;

                case UIS_CLEAR:
                    if (wFlags & UISF_HIDEACCEL) {
                        ClrWF(pwnd, WEFPUIACCELHIDDEN);
                    }
                    if (wFlags & UISF_HIDEFOCUS) {
                        ClrWF(pwnd, WEFPUIFOCUSHIDDEN);
                    }
                    break;

                default:
                    break;
             }

            /*
             * Send it down to its immediate children if any
             */
             if (pwnd->spwndChild) {

                PBWL pbwl;
                HWND *phwnd;
                TL tlpwnd;

                pbwl = BuildHwndList(pwnd->spwndChild, BWL_ENUMLIST, NULL);
                if (pbwl == NULL)
                    return 0;

                for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {
                    /*
                     * Make sure this hwnd is still around.
                     */
                    if ((pwnd = RevalidateHwnd(*phwnd)) == NULL)
                        continue;

                    ThreadLockAlways(pwnd, &tlpwnd);
                    xxxSendMessage(pwnd, message, wParam, lParam);
                    ThreadUnlock(&tlpwnd);
                }
                FreeHwndList(pbwl);
            }
        }
        break;

#ifdef PENWIN20
    // LATER mikeke
    default:
        // BOGUS
        // 32-bit ize DefPenWindowProc
        //
        // call DefPenWindowProc if penwin is loaded
        if (   (message >= WM_HANDHELDFIRST)
            && (message <= WM_HANDHELDLAST)
           ) {
            if (lpfnHandHeld != NULL)
                return (*lpfnHandHeld)(HW16(pwnd), message, wParamLo, lParam);
        } else if (   (message >= WM_PENWINFIRST)
                   && (message <= WM_PENWINLAST)
                  ) {
            if (SYSMET(PENWINDOWS))
                return DefPenWindowProc(pwnd, message, wParamLo, lParam);
        }

#endif // PENWIN20
    }

    return 0;
}
