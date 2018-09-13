/****************************** Module Header ******************************\
* Module Name: focusact.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* History:
* 11-08-90 DavidPe      Created.
* 02-11-91 JimA         Multi-desktop support.
* 02-13-91 mikeke       Added Revalidation code.
* 06-10-91 DavidPe      Changed to desynchronized model.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

BOOL RemoveEventMessage(PQ pq, DWORD dwQEvent, DWORD dwQEventStop);

/***************************************************************************\
* xxxDeactivate
*
* This routine does the processing for the event posted when the foreground
* thread changes.  Note the difference in order of assignment vs. message
* sending in the focus and active windows.  This is consistent with how
* things are done in Win 3.1.
*
*
* PTHREADINFO pti May not be ptiCurrent if SetForegroundWindow called from
* minmax
*
* History:
* 06-07-91 DavidPe      Created.
\***************************************************************************/

void xxxDeactivate(
    PTHREADINFO pti,            // May not be ptiCurrent
    DWORD tidSetForeground)
{
    PWND pwndLose;
    AAS aas;
    TL tlpwndCapture;
    TL tlpwndChild;
    TL tlpwndLose;
    TL tlpti;
    TL tlptiLose;
    WPARAM wParam;
    PTHREADINFO ptiLose;
    PTHREADINFO ptiCurrent = PtiCurrent();
    BOOL fSetActivateAppBit = FALSE;

    /*
     * If we're not active, we have nothing to deactivate, so just return.
     * If we don't return, we'll send redundant WM_ACTIVATEAPP messages.
     * Micrografx Draw, for example, calls FreeProcInstance() twice when
     * this occurs, thereby crashing.
     */
    if (pti->pq->spwndActive == NULL)
        return;

    /*
     * If pti != ptiCurrent, thread lock pti because we may leave
     * the critical section.
     */
    if (pti != ptiCurrent)
        ThreadLockPti(ptiCurrent, pti, &tlpti);

    /*
     * Prevent an activating WM_ACTIVATEAPP from being sent
     * while we're processing this event.
     */
    if (!(pti->TIF_flags & TIF_INACTIVATEAPPMSG)) {
        pti->TIF_flags |= TIF_INACTIVATEAPPMSG;
        fSetActivateAppBit = TRUE;
    }

    /*
     * Cancel any modes like move/size and menu tracking.
     */
    if (pti->pq->spwndCapture != NULL) {
        ThreadLockAlwaysWithPti(ptiCurrent, pti->pq->spwndCapture, &tlpwndCapture);
        xxxSendMessage(pti->pq->spwndCapture, WM_CANCELMODE, 0, 0);
        ThreadUnlock(&tlpwndCapture);

        /*
         * Set QS_MOUSEMOVE so any sleeping modal loops,
         * like the move/size code, will wake up and figure
         * out that it should abort.
         */
        SetWakeBit(pti, QS_MOUSEMOVE);
    }

    /*
     * See the comments in xxxActivateThisWindow about Harvard Graphics.
     * WinWord's Equation editor does some games when it gets the WM_ACTIVATE
     * so we have to remember to send the WM_ACTIVATEAPP to ptiLose. 22510
     */
    if (pti->pq->spwndActive != NULL) {
        pwndLose = pti->pq->spwndActive;
        ptiLose = GETPTI(pwndLose);

        ThreadLockPti(ptiCurrent, ptiLose, &tlptiLose);
        ThreadLockAlwaysWithPti(ptiCurrent, pwndLose, &tlpwndLose);
        wParam = MAKELONG(WA_INACTIVE, TestWF(pwndLose, WFMINIMIZED));
        if (!xxxSendMessage(pwndLose, WM_NCACTIVATE, WA_INACTIVE, 0)) {
            ThreadUnlock(&tlpwndLose);
            ThreadUnlockPti(ptiCurrent, &tlptiLose);
            goto Exit;
        }
        xxxSendMessage(pwndLose, WM_ACTIVATE, wParam, 0);

        /*
         * Only update the queue's active windows if they weren't
         * changed while we were off calling SendMessage.
         */
        if (pti->pq->spwndActive == pwndLose) {
            Lock(&pti->pq->spwndActivePrev, pti->pq->spwndActive);
            Unlock(&pti->pq->spwndActive);
        }

        /*
         * The flag WFFRAMEON is cleared in the default processing of
         * WM_NCACTIVATE message.
         * We want to clear this flag again here since it might of been
         * set in xxxSendNCPaint.
         * Pbrush calls DrawMenuBar when it gets the WM_ACTIVATE message
         * sent above and this causes xxxSendNCPaint to get called and the
         * WFFRAMEON flag gets reset.
         */
        ClrWF(pwndLose, WFFRAMEON);
        ThreadUnlock(&tlpwndLose);

        /*
         * Revalidate ptiLose because the thread may have gone away
         * when the activation messages were sent above.
         */
        aas.ptiNotify = (ptiLose->TIF_flags & TIF_INCLEANUP) ? NULL : ptiLose;
        ThreadUnlockPti(ptiCurrent, &tlptiLose);
    } else {

        /*
         * Use a non-NULL special value for the test after
         * the xxxActivateApp calls.
         */
        pwndLose = (PWND)-1;
        aas.ptiNotify = pti;
    }

    if (aas.ptiNotify) {
        aas.tidActDeact = tidSetForeground;
        aas.fActivating = FALSE;
        aas.fQueueNotify = FALSE;

        ThreadLockWithPti(ptiCurrent,
                pti->rpdesk->pDeskInfo->spwnd->spwndChild, &tlpwndChild);
        xxxInternalEnumWindow(pti->rpdesk->pDeskInfo->spwnd->spwndChild,
                (WNDENUMPROC_PWND)xxxActivateApp, (LPARAM)&aas, BWL_ENUMLIST);
        ThreadUnlock(&tlpwndChild);
    }

    /*
     * If an app (i.e. Harvard Graphics/Windows Install) tries to
     * reactivate itself during a deactivating WM_ACTIVATEAPP
     * message, force deactivation.
     */
    if (pti->pq->spwndActive == pwndLose) {

        ThreadLockWithPti(ptiCurrent, pwndLose, &tlpwndLose);
        if (!xxxSendMessage(pwndLose, WM_NCACTIVATE, WA_INACTIVE, 0)) {
            ThreadUnlock(&tlpwndLose);
            goto Exit;
        }
        xxxSendMessage(pwndLose, WM_ACTIVATE, WA_INACTIVE, 0);
        ThreadUnlock(&tlpwndLose);

        /*
         * Only update the queue's active windows if they weren't
         * changed while we were off calling SendMessage.
         */
        if (pti->pq->spwndActive == pwndLose) {
            Lock(&pti->pq->spwndActivePrev, pti->pq->spwndActive);
            Unlock(&pti->pq->spwndActive);
        }
    }

    if (pti->pq->spwndFocus != NULL) {
        pwndLose = Unlock(&pti->pq->spwndFocus);
        if (pwndLose != NULL) {
            ThreadLockAlwaysWithPti(ptiCurrent, pwndLose, &tlpwndLose);
            xxxSendMessage(pwndLose, WM_KILLFOCUS, 0, 0);
#ifdef FE_IME
            if (IS_IME_ENABLED()) {
                xxxFocusSetInputContext(pwndLose, FALSE, FALSE);
            }
#endif
            ThreadUnlock(&tlpwndLose);
        }
    }

Exit:
    if (fSetActivateAppBit) {
        pti->TIF_flags &= ~TIF_INACTIVATEAPPMSG;
    }
    if (pti != ptiCurrent)
        ThreadUnlockPti(ptiCurrent, &tlpti);
}


/***************************************************************************\
* xxxSendFocusMessages
*
* Common routine for xxxSetFocus() and xxxActivateWindow() that sends the
* WM_KILLFOCUS and WM_SETFOCUS messages to the windows losing and
* receiving the focus.  This function also sets the local pwndFocus
* to the pwnd receiving the focus.
*
* History:
* 11-08-90 DavidPe      Ported.
* 06-06-91 DavidPe      Rewrote for local pwndFocus/pwndActive in THREADINFO.
\***************************************************************************/

void xxxSendFocusMessages(
    PTHREADINFO pti,
    PWND pwndReceive)
{
    PWND pwndLose;
    TL tlpwndLose;

    CheckLock(pwndReceive);

    /*
     * Remember if this app set the focus to NULL on purpose after it was
     * activated (needed in ActivateThisWindow()).
     */
    pti->pq->QF_flags &= ~QF_FOCUSNULLSINCEACTIVE;
    if (pwndReceive == NULL && pti->pq->spwndActive != NULL)
        pti->pq->QF_flags |= QF_FOCUSNULLSINCEACTIVE;

    pwndLose = pti->pq->spwndFocus;
    ThreadLockWithPti(pti, pwndLose, &tlpwndLose);

    /*
     * We shouldn't be locking a valid pwnd from another queue.
     */
    UserAssert((pwndReceive == NULL)
                    || TestWF(pwndReceive, WFDESTROYED)
                    || (pti->pq == GETPTI(pwndReceive)->pq));
    Lock(&pti->pq->spwndFocus, pwndReceive);

    if (pwndReceive == NULL) {
        if (pwndLose != NULL) {
            /*
             * Tell the client that nobody is gaining focus.
             */
            if (FWINABLE()) {
                xxxWindowEvent(EVENT_OBJECT_FOCUS, NULL, OBJID_CLIENT, INDEXID_OBJECT, 0);
            }
            xxxSendMessage(pwndLose, WM_KILLFOCUS, 0, 0);
#ifdef FE_IME
            if (IS_IME_ENABLED()) {
                xxxFocusSetInputContext(pwndLose, FALSE, FALSE);
            }
#endif
        }
    } else {

        /*
         * Make this thread foreground so its base
         * priority get set higher.
         */
        if (pti->pq == gpqForeground)
            SetForegroundThread(GETPTI(pwndReceive));

        if (pwndLose != NULL) {
            xxxSendMessage(pwndLose, WM_KILLFOCUS, (WPARAM)HWq(pwndReceive), 0);
#ifdef FE_IME
            if (IS_IME_ENABLED()) {
                xxxFocusSetInputContext(pwndLose, FALSE, FALSE);
            }
#endif
        }

        /*
         * Send the WM_SETFOCUS message, but only if the window we're
         * setting the focus to still has the focus!  This allows apps
         * to prevent themselves from losing the focus by catching
         * the WM_NCACTIVATE message and returning FALSE or by calling
         * SetFocus() inside their WM_KILLFOCUS handler.
         */
        if (pwndReceive == pti->pq->spwndFocus) {
#ifdef FE_IME
            if (IS_IME_ENABLED()) {
                xxxFocusSetInputContext(pwndReceive, TRUE, FALSE);
            }
#endif
            /*
             * We have to do this BEFORE sending the WM_SETFOCUS message.
             * The app, upon receiving it, very well may turn around and
             * SetFocus() to a child window.
             */
            if (FWINABLE()) {
                xxxWindowEvent(EVENT_OBJECT_FOCUS, pwndReceive, OBJID_CLIENT,
                        INDEXID_OBJECT, 0);
            }
            xxxSendMessage(pwndReceive, WM_SETFOCUS, (WPARAM)HW(pwndLose), 0);
        }
    }

    ThreadUnlock(&tlpwndLose);
}


/***************************************************************************\
* xxxActivateApp
*
* xxxEnumWindows call-back function to send the WM_ACTIVATEAPP
* message to the appropriate windows.
*
* We search for windows whose pq == HIWORD(lParam).  Once we find
* one, we send a WM_ACTIVATEAPP message to that window.  The wParam
* of the message is FALSE if the app is losing the activation and
* TRUE if the app is gaining the activation.  The lParam is the
* task handle of the app gaining the activation if wParam is FALSE
* and the task handle of the app losing the activation if wParam
* is TRUE.
*
* lParam = (HIWORD) : pq of app that we are searching for
*          (LOWORD) : pq of app that we notify about
*
* fDoActivate = TRUE  : Send activate
*               FALSE : Send deactivate
*
* History:
* 11-08-90 DavidPe      Ported.
* 06-26-91 DavidPe      Changed for desync focus/activation.
\***************************************************************************/

BOOL xxxActivateApp(
    PWND pwnd,
    AAS *paas)
{
    CheckLock(pwnd);

    if (GETPTI(pwnd) == paas->ptiNotify) {

        if (paas->fQueueNotify) {
            QueueNotifyMessage(pwnd, WM_ACTIVATEAPP, paas->fActivating,
                    paas->tidActDeact);
        } else {
            xxxSendMessage(pwnd, WM_ACTIVATEAPP, paas->fActivating,
                    paas->tidActDeact);
        }
    }

    return TRUE;
}


/***************************************************************************\
* FBadWindow
*
*
* History:
* 11-08-90 DavidPe      Ported.
\***************************************************************************/

BOOL FBadWindow(
    PWND pwnd)
{
    return (pwnd == NULL
            || !TestWF(pwnd, WFVISIBLE)
            || TestWF(pwnd, WFDISABLED));
}


void xxxUpdateTray(PWND pwnd)
{
    PWND pwndT;

    CheckLock(pwnd);
    if (!TestWF(pwnd, WFVISIBLE)) {
        return;
    }

    for (pwndT = pwnd; pwndT->spwndOwner; pwndT = pwndT->spwndOwner) {
    }

    // Notify the shell hook about this activation change
    if (    GETPTI(pwndT)->pq == gpqForeground &&
            FDoTray() &&
            (FCallHookTray() || FPostTray(pwndT->head.rpdesk)) &&
            FTopLevel(pwndT) &&
            TestWF(pwndT, WFVISIBLE))
    {
        BOOL        fFirstTry;
        BOOL        fTryAgain;
        PWND        pwndArg;
        TL          tlpwndArg;

        fFirstTry = TRUE;
        do {
            fTryAgain = FALSE;
            if (TestWF(pwndT, WFWIN40COMPAT)) {
                if (TestWF(pwnd, WFWIN40COMPAT) && IsTrayWindow(pwnd)) {
                    pwndArg = pwnd;
                } else {
                    pwndArg = IsTrayWindow(pwndT) ? pwndT : NULL;
                }
            } else {
                if (TestWF(pwndT, WEFTOOLWINDOW)) {
                    pwndArg = NULL;
                } else if (FHas31TrayStyles(pwndT)) {
                    pwndArg = Is31TrayWindow(pwndT) ? pwndT : NULL;
                } else if (fFirstTry && (pwndT = pwndT->spwndLastActive)) {
                    fFirstTry = FALSE;
                    fTryAgain = TRUE;
                } else {
                    return;
                }
            }
        } while (fTryAgain);

        ThreadLock(pwndArg, &tlpwndArg);
        xxxSetTrayWindow(
                (pwndArg) ? pwndArg->head.rpdesk : pwndT->head.rpdesk,
                pwndArg,
                NULL);

        ThreadUnlock(&tlpwndArg);
    }
}

/***************************************************************************\
* xxxActivateThisWindow
*
* This function is the workhorse for window activation.  It will attempt to
* activate the pwnd specified.  The other parameters are defined as:
*
*  fFlags      This is a flag-mask which defines how the routine is called.
*              These flags are defined as follows:
*
*              ATW_MOUSE     This is set if activation is changing due to a
*                            mouse click and not set if some other action
*                            caused this window to be activated.  This bit
*                            determines the value of wParam on the
*                            WM_ACTIVATE message.
*
*              ATW_SETFOCUS  This parameter is set if this routine should
*                            set the focus to NULL.  If we are called from
*                            the xsxSetFocus() function this will not be
*                            set indicating that we shouldn't screw with the
*                            focus.  Normally (if we are not called from
*                            xxxSetFocus), we set the focus to NULL here
*                            and either the app or xxxDefWindowProc() sets
*                            the focus to the appropriate window.  If the
*                            bit is not set, we don't want to do anything
*                            with focus.  The app may still do a call to
*                            xxxSetFocus() when the WM_ACTIVATE comes
*                            through, but it will just be redundant on its
*                            part.
*
*              ATW_ASYNC     This bit is set if we are processing this
*                            routine from an asynchronous activate (i.e.
*                            xxxProcessEventMessage()).  In this case, we
*                            make sure that we are the foreground queue
*                            before determining if we bring the window to
*                            top.
*
* History:
* 11-08-90 DavidPe      Ported.
* 05-01-95 ChrisWil     changed bool-flags to 1 ATW_ type.
\***************************************************************************/

BOOL xxxActivateThisWindow(
    PWND pwnd,
    DWORD tidLoseForeground,
    DWORD fFlags)
{
    PTHREADINFO ptiCurrent = PtiCurrent();
    PWND pwndT, pwndActivePrev, pwndActiveSave;
    TL tlpwndActive;
    TL tlpwndChild;
    TL tlpwndActivePrev;
    WPARAM wParam;
    BOOL fSetActivateAppBit;

    BOOL fMouse = (BOOL)(fFlags & ATW_MOUSE);
    BOOL fSetFocus = (BOOL)(fFlags & ATW_SETFOCUS);
    BOOL fAsync = (BOOL)(fFlags & ATW_ASYNC);

#if DBG
    PQ pqSave = ptiCurrent->pq;
#endif


    CheckLock(pwnd);

    /*
     * If pwnd is NULL, then we can't do anything.
     */
    if ((pwnd == NULL) || (pwnd == PWNDDESKTOP(pwnd))) {
        return FALSE;
    }

    /*
     * Don't activate a window that has been destroyed.
     */
    if (HMIsMarkDestroy(pwnd))
        return FALSE;

    /*
     * We don't activate top-level windows of a different queue.
     */
    if (GETPTI(pwnd)->pq != ptiCurrent->pq) {
        return FALSE;
    }

    pwndActiveSave = ptiCurrent->pq->spwndActive;

    /*
     * Do the change-in-activation if the two-windows are different,
     * and if we're not recursing
     */
    if ((pwnd != pwndActiveSave) && !TestWF(pwnd, WFBEINGACTIVATED)) {

        /*
         * Ask the CBT hook whether it is OK to activate this window.
         */
        {
            CBTACTIVATESTRUCT CbtActivateParams;

            if (IsHooked(ptiCurrent, WHF_CBT)) {

                CbtActivateParams.fMouse     = fMouse;
                CbtActivateParams.hWndActive = HW(pwndActiveSave);

                if (xxxCallHook(HCBT_ACTIVATE,
                        (WPARAM)HWq(pwnd), (LPARAM)&CbtActivateParams, WH_CBT)) {
                    return FALSE;
                }
            }
        }

        ptiCurrent->pq->QF_flags &= ~QF_EVENTDEACTIVATEREMOVED;

        /*
         * If the active window went away but somehow was left referenced
         * in the queue, then we do not want to do any deactivation of
         * that window.
         *
         * Don't thread lock this because the next thing we do with it
         * is just an equality check.
         *
         * A DBG check is placed in xxxDestroyWindow to attempt to
         * catch the situation where we return from the function with
         * the destroyed window set in the active (pq).  If that situation
         * can be detected and solved, then this conditional might be
         * removed: ChrisWil - 08/22/95.
         */
        if (ptiCurrent->pq->spwndActive && TestWF(ptiCurrent->pq->spwndActive, WFDESTROYED)) {
            Lock(&ptiCurrent->pq->spwndActive, NULL);
        } else {
            Lock(&ptiCurrent->pq->spwndActivePrev, ptiCurrent->pq->spwndActive);
        }
        pwndActivePrev = ptiCurrent->pq->spwndActive;

        /*
         * If there was a previously active window,
         * and we're in the foreground then assign
         * gpqForegroundPrev to ourself.
         */
        if ((pwndActivePrev != NULL) && (ptiCurrent->pq == gpqForeground)) {
            gpqForegroundPrev = ptiCurrent->pq;
        }

        /*
         * Deactivate currently active window if possible.
         */
        if (pwndActivePrev != NULL) {
            ThreadLockWithPti(ptiCurrent, pwndActivePrev, &tlpwndActive);

            /*
             * The active window can prevent itself from losing the
             * activation by returning FALSE to this WM_NCACTIVATE message
             */
            wParam = MAKELONG(WA_INACTIVE, TestWF(pwndActivePrev, WFMINIMIZED));
            if (!xxxSendMessage(pwndActivePrev, WM_NCACTIVATE,
                    wParam, (LPARAM)HWq(pwnd))) {
                ThreadUnlock(&tlpwndActive);
                return FALSE;
            }

            xxxSendMessage(pwndActivePrev, WM_ACTIVATE, wParam, (LPARAM)HWq(pwnd));

            ThreadUnlock(&tlpwndActive);
        }

        /*
         * If the activation changed while we were gone, we'd better
         * not send any more messages, since they'd go to the wrong window.
         * (and, they've already been sent anyhow)
         */
        if (ptiCurrent->pq->spwndActivePrev != ptiCurrent->pq->spwndActive ||
                pwndActiveSave != ptiCurrent->pq->spwndActive) {
#if DBG
            if (ptiCurrent->pq->spwndActivePrev == ptiCurrent->pq->spwndActive) {
                RIPMSG0(RIP_WARNING, "xxxActivateThisWindow: ptiCurrent->pq->spwndActive changed in callbacks");
            }
#endif
            return FALSE;
        }

        /*
         * If the window being activated has been destroyed, don't
         * do anything else.  Making it the active window in this
         * case can cause console to hang during shutdown.
         */
        if (HMIsMarkDestroy(pwnd))
            return FALSE;

        /*
         * Before we lock the new pwndActivate, make sure we're still
         *  on the same queue.
         */
        if (GETPTI(pwnd)->pq != ptiCurrent->pq) {
            RIPMSG1(RIP_WARNING, "xxxActivateThisWindow: Queue unattached:%#p", pqSave);
            return FALSE;
        }

        /*
         * This bit, which means the app set the focus to NULL after becoming
         * active, doesn't make sense if the app is just becoming active, so
         * clear it in this case. It is used below in this routine to
         * determine whether to send focus messages (read comment in this
         * routine).
         */
        if (ptiCurrent->pq->spwndActive == NULL)
            ptiCurrent->pq->QF_flags &= ~QF_FOCUSNULLSINCEACTIVE;

        Lock(&ptiCurrent->pq->spwndActive, pwnd);

        /*
         * Tp prevent recursion, set pwnd's WFBEINGACTIVATED bit.
         * Recursion can happen if we have an activation battle with other
         * threads which keep changing ptiCurrent->pq->spwndActive behind our
         * callbacks.
         * WARNING: Do NOT return from this routine without clearing this bit!
         */
        SetWF(pwnd, WFBEINGACTIVATED);

        if (FWINABLE()) {
            xxxWindowEvent(EVENT_SYSTEM_FOREGROUND, pwnd, OBJID_WINDOW, INDEXID_OBJECT, WEF_USEPWNDTHREAD);
        }

        /*
         * Remove all async activates up to the next async deactivate. We
         * do this so that any queued activates don't reset this synchronous
         * activation state we're now setting. Only remove up till the next
         * deactivate because active state is synchronized with reading
         * input from the input queue.
         *
         * For example, an activate event gets put in an apps queue. Before
         * processing it the app calls ActivateWindow(), which is synchronous.
         * You want the ActivateWindow() to win because it is newer
         * information.
         *
         * msmail32 demonstrates this. Minimize msmail. Alt-tab to it. It
         * brings up the password dialog, but it isn't active. It correctly
         * activates the password dialog but then processes an old activate
         * event activating the icon, so the password dialog is not active.
         */
        RemoveEventMessage(ptiCurrent->pq, QEVENT_ACTIVATE, QEVENT_DEACTIVATE);

        xxxMakeWindowForegroundWithState(NULL, 0);

        pwndActivePrev = ptiCurrent->pq->spwndActivePrev;
        ThreadLockWithPti(ptiCurrent, pwndActivePrev, &tlpwndActivePrev);

        if (TEST_PUSIF(PUSIF_PALETTEDISPLAY) && xxxSendMessage(pwnd, WM_QUERYNEWPALETTE, 0, 0)) {
            xxxSendNotifyMessage(PWND_BROADCAST, WM_PALETTEISCHANGING,
                    (WPARAM)HWq(pwnd), 0);
        }

        /*
         * If the window becoming active is not already the top window in the
         * Z-order, then call xxxBringWindowToTop() to do so.
         */

        /*
         * If this isn't a child window, first check to see if the
         * window isn't already 'on top'.  If not, then call
         * xxxBringWindowToTop().
         */
        if (!(fFlags & ATW_NOZORDER) && !TestWF(pwnd, WFCHILD)) {

            /*
             * Look for the first visible child of the desktop.
             * ScottLu changed this to start looking at the desktop
             * window. Since the desktop window was always visible,
             * BringWindowToTop was always called regardless of whether
             * it was needed or not. No one can remember why this
             * change was made, so I'll change it back to the way it
             * was in Windows 3.1. - JerrySh
             */
            pwndT = PWNDDESKTOP(pwnd)->spwndChild;

            while (pwndT && (!TestWF(pwndT, WFVISIBLE))) {
                pwndT = pwndT->spwndNext;
            }

            /*
             * If this activation came from an async call (i.e.
             * xxxProcessEventMessage), we need to check to see
             * if the thread is the foreground-queue.  If not, then
             * we do not want to bring the window to the top.  This
             * is because another window could have already been
             * place on top w/foreground.  Bringing the window to
             * the top in this case would result in a top-level window
             * without activation. - ChrisWil
             *
             * Added a check to see if the previous-active window went
             * invisible during the deactivation time.  This will ensure
             * that we bring the new window to the top.  Otherwise, we
             * could end up skipping over the previous-window from the
             * above tests.  Office95 apps demonstrate this behaviour by
             * turning their windows invisible during the painting of their
             * captionbars.  By the time we use to get here, we failed to
             * bring the new window to top.
             */
            if ((pwnd != pwndT) || (pwndActivePrev && !IsVisible(pwndActivePrev))) {

                if (!(fAsync && (gpqForeground != ptiCurrent->pq))) {
                    DWORD dwFlags;

                    /*
                     * Bring the window to the top.  If we're already
                     * activating the window, don't reactivate it.
                     */
                    dwFlags = SWP_NOSIZE | SWP_NOMOVE;
                    if (pwnd == pwndT)
                        dwFlags |= SWP_NOACTIVATE;

                    xxxSetWindowPos(pwnd, PWND_TOP, 0, 0, 0, 0, dwFlags);
                }
            }
        }

        /*
         * If there was no previous active window, or if the
         * previously active window belonged to another thread
         * send the WM_ACTIVATEAPP messages.  The fActivate == FALSE
         * case is handled in xxxDeactivate when 'hwndActivePrev == NULL'.
         *
         * Harvard Graphics/Windows setup calls SetActiveWindow when it
         * receives a deactivationg WM_ACTIVATEAPP.  The TIF_INACTIVATEAPPMSG
         * prevents an activating WM_ACTIVATEAPP(TRUE) from being sent while
         * deactivation is occuring.
         */
        fSetActivateAppBit = FALSE;
        if (!(ptiCurrent->TIF_flags & TIF_INACTIVATEAPPMSG) &&
                ((pwndActivePrev == NULL) ||
                (GETPTI(pwndActivePrev) != GETPTI(pwnd)))) {
            AAS aas;

            /*
             * First send the deactivating WM_ACTIVATEAPP if there
             * was a previously active window of another thread in
             * the current queue.
             */
            if (pwndActivePrev != NULL) {
                PTHREADINFO ptiPrev = GETPTI(pwndActivePrev);
                TL tlptiPrev;

                /*
                 * Ensure that the other thread can't recurse
                 * and send more WM_ACTIVATEAPP msgs.
                 */
                ptiPrev->TIF_flags |= TIF_INACTIVATEAPPMSG;

                aas.ptiNotify = ptiPrev;
                aas.tidActDeact = TIDq(ptiCurrent);
                aas.fActivating = FALSE;
                aas.fQueueNotify = FALSE;

                ThreadLockPti(ptiCurrent, ptiPrev, &tlptiPrev);
                ThreadLockWithPti(ptiCurrent, pwndActivePrev->head.rpdesk->pDeskInfo->spwnd->spwndChild, &tlpwndChild);
                xxxInternalEnumWindow(pwndActivePrev->head.rpdesk->pDeskInfo->spwnd->spwndChild,
                        (WNDENUMPROC_PWND)xxxActivateApp, (LPARAM)&aas, BWL_ENUMLIST);
                ThreadUnlock(&tlpwndChild);
                ptiPrev->TIF_flags &= ~TIF_INACTIVATEAPPMSG;
                ThreadUnlockPti(ptiCurrent, &tlptiPrev);
            }

            /*
             * This will ensure that the current thread will not
             * send any more WM_ACTIVATEAPP messages until it
             * is done performing its activation.
             */
            ptiCurrent->TIF_flags |= TIF_INACTIVATEAPPMSG;
            fSetActivateAppBit = TRUE;

            aas.ptiNotify = GETPTI(pwnd);
            aas.tidActDeact = tidLoseForeground;
            aas.fActivating = TRUE;
            aas.fQueueNotify = FALSE;

            ThreadLockWithPti(ptiCurrent, ptiCurrent->rpdesk->pDeskInfo->spwnd->spwndChild, &tlpwndChild);
            xxxInternalEnumWindow(ptiCurrent->rpdesk->pDeskInfo->spwnd->spwndChild,
                    (WNDENUMPROC_PWND)xxxActivateApp, (LPARAM)&aas, BWL_ENUMLIST);
            ThreadUnlock(&tlpwndChild);
        }

        /*
         * If this window has already been drawn as active, set the
         * flag so that we don't draw it again.
         */
        if (TestWF(pwnd, WFFRAMEON)) {
            SetWF(pwnd, WFNONCPAINT);
        }

        /*
         * If the window is marked for destruction, don't do
         * the lock because xxxFreeWindow has already been called
         * and a lock here will result in the window locking itself
         * and never being freed.
         */
        if (!HMIsMarkDestroy(pwnd)) {

            /*
             * Set most recently active window in owner/ownee list.
             */
            pwndT = pwnd;
            while (pwndT->spwndOwner != NULL) {
                pwndT = pwndT->spwndOwner;
            }
            Lock(&pwndT->spwndLastActive, pwnd);
        }


        xxxSendMessage(pwnd, WM_NCACTIVATE,
                MAKELONG(GETPTI(pwnd)->pq == gpqForeground,
                ptiCurrent->pq->spwndActive != NULL ?
                TestWF(ptiCurrent->pq->spwndActive, WFMINIMIZED) : 0),
                (LPARAM)HW(pwndActivePrev));

        if (ptiCurrent->pq->spwndActive != NULL) {
            xxxSendMessage(pwnd, WM_ACTIVATE,
                    MAKELONG((fMouse ? WA_CLICKACTIVE : WA_ACTIVE),
                    TestWF(ptiCurrent->pq->spwndActive, WFMINIMIZED)),
                    (LPARAM)HW(pwndActivePrev));
        } else {
            xxxSendMessage(pwnd, WM_ACTIVATE,
                    MAKELONG((fMouse ? WA_CLICKACTIVE : WA_ACTIVE), 0),
                    (LPARAM)HW(pwndActivePrev));
        }

        xxxUpdateTray(pwnd);

        ThreadUnlock(&tlpwndActivePrev);

        ClrWF(pwnd, WFNONCPAINT);

        /*
         * If xxxActivateThisWindow() is called from xxxSetFocus() then
         * fSetFocus is FALSE.  In this case, we don't set the focus since
         * xxxSetFocus() will do that for us.  Otherwise, we set the focus
         * to the newly activated window if the window with the focus is
         * not the new active window or one of its children.  Normally,
         * xxxDefWindowProc() will set the focus.
         */
        ThreadLockWithPti(ptiCurrent, ptiCurrent->pq->spwndActive, &tlpwndActive);

        /*
         * Win3.1 checks spwndFocus != NULL - we check QF_FOCUSNULLSINCEACTIVE,
         * which is the win32 equivalent. On win32, 32 bit apps each have their
         * own focus. If the app is not foreground, most of the time spwndFocus
         * is NULL when the window is being activated and brought to the
         * foreground. It wouldn't go through this code in this case. Win3.1 in
         * effect is checking if the previous active application had an
         * hwndFocus != NULL. Win32 effectively assumes the last window has a
         * non-NULL hwndFocus, so win32 instead checks to see if the focus has
         * been set to NULL since this application became active (meaning, did
         * it purposefully set the focus to NULL). If it did, don't go through
         * this codepath (like win3.1). If it didn't, go through this code path
         * because the previous application had an hwndFocus != NULL
         * (like win3.1). Effectively it is the same check as win3.1, but
         * updated to deal with async input.
         *
         * Case in point: bring up progman, hit f1 (to get win32 help). Click
         * history to get a popup (has the focus in a listbox in the client
         * area). Activate another app, now click on title bar only of history
         * popup. The focus should get set by going through this code path.
         *
         * Alternate case: Ventura Publisher brings up "Special Effects"
         * dialog. If "Bullet" from this dialog was clicked last time the
         * dialog was brought up, sending focus messages here when
         * hwndFocus == NULL, would reset the focus to "None" incorrectly
         * because Ventura does its state setting when it gets the focus
         * messages. The real focus messages it is depending on are the
         * ones that come from the SetFocus() call in DlgSetFocus() in
         * the dialog management code. (In this case, before the dialog
         * comes up, focus == active window. When the dialog comes up
         * and EnableWindow(hwndOwner, FALSE) is called, EnableWindow() calls
         * SetFocus(NULL) (because it is disabling the window that is also
         * the focus window). When the dialog comes up it gets activated via
         * SwpActivate(), but since the focus is NULL vpwin does not expect
         * to go through this code path.)
         *
         * - scottlu
         */
#if 0
// this is what win3.1 does - which won't work for win32

        if (fSetFocus && ptiCurrent->pq->spwndFocus != NULL && ptiCurrent->pq->spwndActive !=
                GetTopLevelWindow(ptiCurrent->pq->spwndFocus))
#else
        if (fSetFocus && !(ptiCurrent->pq->QF_flags & QF_FOCUSNULLSINCEACTIVE) &&
                ptiCurrent->pq->spwndActive != GetTopLevelWindow(ptiCurrent->pq->spwndFocus)) {
#endif

            xxxSendFocusMessages(ptiCurrent,
                    (ptiCurrent->pq->spwndActive != NULL &&
                    TestWF(ptiCurrent->pq->spwndActive, WFMINIMIZED)) ?
                    NULL : ptiCurrent->pq->spwndActive);
        }

        ThreadUnlock(&tlpwndActive);

        /*
         * This flag is examined in the menu loop code so that we exit from
         * menu mode if another window was activated while we were tracking
         * menus.
         */
        ptiCurrent->pq->QF_flags |= QF_ACTIVATIONCHANGE;

        if (gppiScreenSaver == NULL) {

            /*
             * Activation has occurred, update our last idle time counter if
             * we're on the input desktop.
             */
            if (ptiCurrent->rpdesk == grpdeskRitInput) {
                glinp.timeLastInputMessage = NtGetTickCount();
            }

        } else {

            if (GETPTI(pwnd)->ppi != gppiScreenSaver) {
                /*
                 * Activation ocurred by an app other than the screen saver.
                 * Update the idle time counter and mark our screen saver as
                 * active (so it can quit).
                 */

#if 0
// LATER
                if (ptiCurrent->rpdesk != gppiScreenSaver->rpdeskStartup) {
                    /*
                     * Activation is occurring on different desktops, let WinLogon decide
                     * if it wants to switch.
                     */
                }
#endif

                glinp.timeLastInputMessage = NtGetTickCount();
                gppiScreenSaver->W32PF_Flags &= ~W32PF_IDLESCREENSAVER;
                SetForegroundPriorityProcess(gppiScreenSaver, gppiScreenSaver->ptiMainThread, TRUE);
            }
        }

        /*
         * If WM_ACTIVATEAPP messages were sent, it is now
         * safe to allow them to be sent again.
         */
        if (fSetActivateAppBit)
            ptiCurrent->TIF_flags &= ~TIF_INACTIVATEAPPMSG;


    } else {
#if DBG
        if (TestWF(pwnd, WFBEINGACTIVATED)) {
            RIPMSG1(RIP_WARNING, "xxxActivateThisWindow recursing on pwnd %#p\n", pwnd);
        }
#endif
        ptiCurrent->pq->QF_flags &= ~QF_EVENTDEACTIVATEREMOVED;
        if (TEST_PUSIF(PUSIF_PALETTEDISPLAY) && xxxSendMessage(pwnd, WM_QUERYNEWPALETTE, 0, 0)) {
            xxxSendNotifyMessage(PWND_BROADCAST, WM_PALETTEISCHANGING,
                    (WPARAM)HWq(pwnd), 0);
        }
    }

    ClrWF(pwnd, WFBEINGACTIVATED);
    return ptiCurrent->pq->spwndActive == pwnd;
}


/***************************************************************************\
* RemoveEventMessage
*
* Removes events dwQEvent until finding dwQEventStop. Used for removing
* activate and deactivate events.
*
* 04-01-93 ScottLu      Created.
\***************************************************************************/

BOOL RemoveEventMessage(
    PQ pq,
    DWORD dwQEvent,
    DWORD dwQEventStop)
{
    PQMSG pqmsgT;
    PQMSG pqmsgPrev;
    BOOL bRemovedEvent = FALSE;

    /*
     * Remove all events dwQEvent until finding dwQEventStop.
     */
    for (pqmsgT = pq->mlInput.pqmsgWriteLast; pqmsgT != NULL; ) {

        if (pqmsgT->dwQEvent == dwQEventStop)
            return(bRemovedEvent);

        pqmsgPrev = pqmsgT->pqmsgPrev;

        /*
         * If the event is found and is not the one being peeked,
         * delete it.
         */
        if (pqmsgT->dwQEvent == dwQEvent &&
                pqmsgT != (PQMSG)pq->idSysPeek) {
            DelQEntry(&(pq->mlInput), pqmsgT);
            bRemovedEvent = TRUE;
        }
        pqmsgT = pqmsgPrev;
    }
    return(bRemovedEvent);
}


/***************************************************************************\
* CanForceForeground
*
* A process can NOT force a new foreground when:
* -There is a last input owner glinp.ptiLastWoken), and
* -The process didn't get the last hot key, key or mouse click, and
* -There is a thread with foreground priority gptiForeground), and
* -The process doesn't own the foreground thread, and
* -The process doesn't have foreground activation right, and
* -The process was not the last one to do SendInput/JournalPlayBack
* -There is a foreground queue, and
* -The last input owner is not being debugged, and
* -The foreground process is not being debugged, and
* -The last input was not long ago
*
* History:
* 05/12/97  GerardoB    Extracted from xxxSetForegroundWindow
\***************************************************************************/
BOOL CanForceForeground(PPROCESSINFO ppi)
{

    if ((glinp.ptiLastWoken != NULL)
            && (glinp.ptiLastWoken->ppi != ppi)
            && (gptiForeground != NULL)
            && (gptiForeground->ppi != ppi)
            && !(ppi->W32PF_Flags & (W32PF_ALLOWFOREGROUNDACTIVATE | W32PF_ALLOWSETFOREGROUND))
            && (ppi != gppiInputProvider)
            && (gpqForeground != NULL)
            &&
        #if DBG
            /*
             * When attaching the debugger to the foreground app, this function always
             *  returns TRUE. In order to be able to debug anything related to this
             *  function in such case, set this global to TRUE.
             */
               (gfDebugForegroundIgnoreDebugPort
                || (
        #endif
                       (glinp.ptiLastWoken->ppi->Process->DebugPort == NULL)
                    && (gptiForeground->ppi->Process->DebugPort == NULL)
        #if DBG
                ))
        #endif
            && !IsTimeFromLastRITEvent(UP(FOREGROUNDLOCKTIMEOUT))) {

        return FALSE;
    } else {
        return TRUE;
    }

}
/***************************************************************************\
* AllowSetForegroundWindow (5.0 API)
*
* This API is meant to be called by the foreground process to allow another
*  process to take the foreground.
* This is implemented by making a thread in dwProcessId the owner of the last
*  input event. This means that dwProcessId keeps the right to take the foreground
*  until the user generates new input (unless the input is direct to dwProcessId itself).
*
* History:
* 01-28-98 GerardoB      Created.
\***************************************************************************/
BOOL xxxAllowSetForegroundWindow(
    DWORD dwProcessId)
{
    DWORD dwError;
    PEPROCESS pep;
    NTSTATUS Status;
    PPROCESSINFO ppi;
    /*
     * Get the ppi for dwProcessId
     * ASFW_ANY NULLs out the input owner so any process can take the foreground
     */
    if (dwProcessId != ASFW_ANY) {
        LeaveCrit();
        Status = LockProcessByClientId((HANDLE)LongToHandle( dwProcessId ), &pep);
        EnterCrit();
        if (!NT_SUCCESS(Status)) {
            RIPERR0(ERROR_INVALID_PARAMETER, RIP_VERBOSE, "");
            return FALSE;
        }
        ppi = PpiFromProcess(pep);
        if (ppi == NULL) {
            dwError = ERROR_INVALID_PARAMETER;
            goto UnlockAndFail;
        }
    }
    /*
     * Do nothing if the current process cannot force a foreground change.
     * We could have checked this upfront but we didn't since we had to
     *  leave the crit section and the state could have changed.
     */
    if (!CanForceForeground(PpiCurrent())) {
        dwError = ERROR_ACCESS_DENIED;
        goto UnlockAndFail;
    }
    /*
     * Let's make a thread (if any) of this process be the last input owner
     */
    if (dwProcessId != ASFW_ANY) {
        TAGMSG2(DBGTAG_FOREGROUND, "xxxAllowSetForegroundWindow by %#p to %#p", PpiCurrent(), ppi);
        glinp.ptiLastWoken = ppi->ptiList;
        UnlockProcess(pep);
    } else {
        TAGMSG1(DBGTAG_FOREGROUND, "xxxAllowSetForegroundWindow by %#p to ANY", PpiCurrent());
        glinp.ptiLastWoken = NULL;
    }
    return TRUE;

UnlockAndFail:
    if (dwProcessId != ASFW_ANY) {
        UnlockProcess(pep);
    }
    RIPERR0(dwError, RIP_VERBOSE, "");
    return FALSE;
}
/***************************************************************************\
* LockSetForegroundWindow (5.0 API)
*
* This API allows application to prevent any call to SetForegroundWindow.
* This is mainly intended for application implementing their own menus
*  so they can block SFW just like we do for our own menus.
* Certain actions like hitting the ALT key or any foreground change (ie, by a click)
*  will automatically unlock SFW (so apps cannot hose SFW)
*
* History:
* 07-04-98 GerardoB      Created.
\***************************************************************************/
BOOL _LockSetForegroundWindow(
    UINT uLockCode)
{
    DWORD dwError;
    PPROCESSINFO ppiCurrent = PpiCurrent();

    switch (uLockCode) {
        case LSFW_LOCK:
            /*
             * If the caller cannot lock it or already locked, fail the call
             */
            if (CanForceForeground(ppiCurrent) && (gppiLockSFW == NULL)) {
                gppiLockSFW = ppiCurrent;
                TAGMSG1(DBGTAG_FOREGROUND, "_LockSetForegroundWindow locked by %#p", ppiCurrent);
            } else {
                dwError = ERROR_ACCESS_DENIED;
                goto FailIt;
            }
            break;

        case LSFW_UNLOCK:
            /*
             * If the caller didn't lock it, fail the call
             */
            if (ppiCurrent == gppiLockSFW) {
                gppiLockSFW = NULL;
                TAGMSG0(DBGTAG_FOREGROUND, "_LockSetForegroundWindow UNLOCKED");
            } else {
                dwError = ERROR_ACCESS_DENIED;
                goto FailIt;
            }
            break;

        default:
            dwError = ERROR_INVALID_PARAMETER;
            goto FailIt;
    }

    return TRUE;

FailIt:
    RIPERR0(dwError, RIP_VERBOSE, "");
    return FALSE;
}
/***************************************************************************\
* CleanupDecSFWLockCount
*
* Wrapper to be passed to PushW32ThreadLock, which wants an actual function.
* History:
* 10/19/98 GerardoB      Created.
\***************************************************************************/
void CleanupDecSFWLockCount(PVOID pIgnore)
{
    DecSFWLockCount();
    UNREFERENCED_PARAMETER(pIgnore);
}

/***************************************************************************\
* xxxSetForegroundWindow (API)
*
* History:
* 06-07-91 DavidPe      Created.
\***************************************************************************/
BOOL xxxStubSetForegroundWindow(
    PWND pwnd)
{
    return xxxSetForegroundWindow(pwnd, TRUE);
}
BOOL xxxSetForegroundWindow(
    PWND pwnd,
    BOOL fFlash)
{
    BOOL fNiceCall = TRUE;
    BOOL fSyncActivate, fActive;
    DWORD dwFlashFlags;
    PTHREADINFO ptiCurrent = PtiCurrent();
    PWND pwndFlash;
    TL tlpwndFlash;

    CheckLock(pwnd);

    /*
     * If we're trying to set a window on our own thread to the foreground,
     * and we're already in the foreground, treat it just like a call to
     * SetActiveWindow().
     */
    if ((pwnd != NULL) && (GETPTI(pwnd)->pq == gpqForeground)) {
        fSyncActivate = (gpqForeground == ptiCurrent->pq);
        if (fSyncActivate) {
            gppiWantForegroundPriority = ptiCurrent->ppi;
        } else {
            gppiWantForegroundPriority = GETPTI(pwnd)->ppi;
        }

        goto JustActivateIt;
    }
    /*
     * If the foregrond is not locked
     *     and this thread has the right to changethe foreground,
     * then remove the activation right (it's a one-shot deal)
     *      and do it.
     */

    /*
     * Bug 247768 - joejo
     * Add compatibility hack for foreground activation problems.
     *
     * To Fix Winstone99, ignore the foreground lock if the input
     *  provider is making this call. GerardoB.
     *
     */

    if ((!IsForegroundLocked() || (ptiCurrent->ppi == gppiInputProvider))
            && (ptiCurrent->TIF_flags & (TIF_ALLOWFOREGROUNDACTIVATE | TIF_SYSTEMTHREAD | TIF_CSRSSTHREAD)
                || CanForceForeground(ptiCurrent->ppi)
                || GiveUpForeground())) {

        TAGMSG1(DBGTAG_FOREGROUND, "xxxSetForegroundWindow FRemoveForegroundActivate %#p", ptiCurrent);

        FRemoveForegroundActivate(ptiCurrent);
        return xxxSetForegroundWindow2(pwnd, ptiCurrent, 0);
    }
    fNiceCall = FALSE;
    TAGMSG3(DBGTAG_FOREGROUND, "xxxSetForegroundWindow: rude call by %#p to %#p-%#p",
            ptiCurrent, pwnd, (pwnd != NULL ? GETPTI(pwnd) : NULL));
    if (pwnd == NULL) {
        return FALSE;
    }
    /*
     * Notify the user that this pwnd wants to come to the foreground.
     * Try to flash a tray button only; otherwise, flash pwnd
     */
    if (fFlash) {
        pwndFlash = DSW_GetTopLevelCreatorWindow(GetTopLevelWindow(pwnd));
        if (IsTrayWindow(pwndFlash)) {
            dwFlashFlags = FLASHW_TRAY;
        } else {
            pwndFlash = pwnd;
            dwFlashFlags = FLASHW_ALL;
        }
        ThreadLockAlways(pwndFlash, &tlpwndFlash);
        xxxFlashWindow(pwndFlash,
                       MAKELONG(dwFlashFlags | FLASHW_TIMERNOFG, UP(FOREGROUNDFLASHCOUNT)),
                       0);
        ThreadUnlock(&tlpwndFlash);
    }
    /*
     * Activate the window.
     */
    fSyncActivate = (ptiCurrent->pq == GETPTI(pwnd)->pq);

JustActivateIt:

    if (fSyncActivate) {
        fActive = xxxActivateWindow(pwnd, AW_USE);
    } else if (pwnd == GETPTI(pwnd)->pq->spwndActive) {
        fActive = TRUE;
    } else {
        fActive = PostEventMessage(GETPTI(pwnd), GETPTI(pwnd)->pq,
                                QEVENT_ACTIVATE, NULL, 0,
                                0, (LPARAM)HWq(pwnd)) ;
    }

    /*
     * Return FALSE if we failed the set foreground request.
     */
    return fNiceCall && fActive;
}
/***************************************************************************\
* xxxSetForegroundWindow2
*
* History:
* 07-19-91 DavidPe      Created.
\***************************************************************************/

BOOL xxxSetForegroundWindow2(
    PWND pwnd,
    PTHREADINFO pti,
    DWORD fFlags)
{
    PTHREADINFO ptiForegroundOld;
    PTHREADINFO ptiForegroundNew;
    PQ pqForegroundOld, pqForegroundNew, pqCurrent;
    HWND hwnd;
    PQMSG pqmsgDeactivate, pqmsgActivate;
    BOOL bRemovedEvent;
    PTHREADINFO ptiCurrent = PtiCurrent();
    BOOL retval = TRUE;
    UINT uMsg;
    CheckLock(pwnd);

    /*
     * Queue pointers and threadinfo pointers can go away when calling xxx
     * calls. Also, queues can get recalced via AttachThreadInput() during
     * xxx calls - so we want to reference the application becoming foreground.
     * PQs cannot be refcount locked (either thread locked or structure locked)
     * so must (re)calculate them after returning from xxx calls.
     *
     * NOTE: gpqForeground and gpqForegroundPrev are always current and don't
     *       need special handling.
     */

    /*
     * Don't allow the foreground to be set to a window that is not
     * on the current desktop.
     */
    if (pwnd != NULL && (pwnd->head.rpdesk != grpdeskRitInput ||
            HMIsMarkDestroy(pwnd))) {
        return FALSE;
    }

    /*
     * Unlock SetForegroundWindow (if someone had it locked)
     */
    gppiLockSFW = NULL;
    TAGMSG3(DBGTAG_FOREGROUND, "xxxSetForegroundWindow2 by %#p to %#p-%#p",
            ptiCurrent, pwnd, (pwnd != NULL ? GETPTI(pwnd) : NULL));

    /*
     * Calculate who is becoming foreground. Also, remember who we want
     * foreground (for priority setting reasons).
     */
    if ((gptiForeground != NULL) && !(gptiForeground->TIF_flags & TIF_INCLEANUP)) {
        ptiForegroundOld = gptiForeground;
    } else {
        ptiForegroundOld = NULL;
    }
    pqForegroundOld = NULL;
    pqForegroundNew = NULL;
    pqCurrent = NULL;

    gpqForegroundPrev = gpqForeground;

    if (pwnd != NULL) {
        ptiForegroundNew = GETPTI(pwnd);
        UserAssert(ptiForegroundNew->rpdesk == grpdeskRitInput);
        gppiWantForegroundPriority = GETPTI(pwnd)->ppi;
        gpqForeground = GETPTI(pwnd)->pq;
        UserAssert(gpqForeground->cThreads != 0);
        UserAssert(gpqForeground->ptiMouse->rpdesk == grpdeskRitInput);
        // Assert to catch AV in xxxNextWindow doing Alt-Esc: If we have a non-NULL
        // gpqForeground, its kbd input thread better have an rpdesk!  -IanJa
        UserAssert(!gpqForeground || (gpqForeground->ptiKeyboard && gpqForeground->ptiKeyboard->rpdesk));
        SetForegroundThread(GETPTI(pwnd));
    } else {
        ptiForegroundNew = NULL;
        gppiWantForegroundPriority = NULL;
        gpqForeground = NULL;
        SetForegroundThread(NULL);
    }

    /*
     * Are we switching the foreground queue?
     */
    if (gpqForeground != gpqForegroundPrev) {
        TL tlptiForegroundOld;
        TL tlptiForegroundNew;
        TL tlpti;

        ThreadLockPti(ptiCurrent, ptiForegroundOld, &tlptiForegroundOld);
        ThreadLockPti(ptiCurrent, ptiForegroundNew, &tlptiForegroundNew);
        ThreadLockPti(ptiCurrent, pti, &tlpti);

        /*
         * If this call didn't come from the RIT, cancel tracking
         * and other global states.
         */
        if (pti != NULL) {

            /*
             * Clear any visible tracking going on in system.
             */
            xxxCancelTracking();

            /*
             * Remove the clip cursor rectangle - it is a global mode that
             * gets removed when switching.  Also remove any LockWindowUpdate()
             * that's still around.
             */
            zzzClipCursor(NULL);
            LockWindowUpdate2(NULL, TRUE);

            /*
             * Make sure the desktop of the newly activated window is the
             * foreground fullscreen window
             */
            xxxMakeWindowForegroundWithState(NULL, 0);
        }

        /*
         * We've potentially done callbacks. Calculate pqForegroundOld
         * based on our locked local variable ptiForegroundOld.
         */
        pqForegroundOld = NULL;
        if (ptiForegroundOld && !(ptiForegroundOld->TIF_flags & TIF_INCLEANUP)) {
            pqForegroundOld = ptiForegroundOld->pq;
        }

        pqCurrent = NULL;
        if (pti != NULL)
            pqCurrent = pti->pq;

        /*
         * Now allocate message for the deactivation
         */
        pqmsgDeactivate = pqmsgActivate = NULL;

        if ((pqForegroundOld != NULL) && (pqForegroundOld != pqCurrent)) {
            if ((pqmsgDeactivate = AllocQEntry(&pqForegroundOld->mlInput)) ==
                    NULL) {
                retval = FALSE;
                goto Exit;
            }
        }

        /*
         * Do any appropriate deactivation.
         */
        if (pqForegroundOld != NULL) {

            /*
             * If we're already on the foreground queue we'll call
             * xxxDeactivate() directly later in this routine since
             * it'll cause us to leave the critical section.
             */
            if (pqForegroundOld != pqCurrent) {
                StoreQMessage(pqmsgDeactivate, NULL, 0,
                        gptiForeground != NULL ? (WPARAM)gptiForeground->pEThread->Cid.UniqueThread : 0,
                        0, 0, QEVENT_DEACTIVATE, 0);

                /*
                 * If there was an old foreground thread, make it perform
                 * the deactivation.  Otherwise, any thread on the queue
                 * can perform the deactivation.
                 */
                if (ptiForegroundOld != NULL) {
                    SetWakeBit(ptiForegroundOld, QS_EVENTSET);

                    StoreQMessagePti(pqmsgDeactivate, ptiForegroundOld);

                }

                if (pqForegroundOld->spwndActive != NULL) {
                    if (ptiForegroundOld != NULL && FHungApp(ptiForegroundOld, CMSHUNGAPPTIMEOUT)) {
                        TL tlpwnd;
                        ThreadLockAlwaysWithPti(ptiCurrent, pqForegroundOld->spwndActive, &tlpwnd);
                        xxxRedrawHungWindowFrame(pqForegroundOld->spwndActive, FALSE);
                        ThreadUnlock(&tlpwnd);
                    } else {
                        SetHungFlag(pqForegroundOld->spwndActive, WFREDRAWFRAMEIFHUNG);
                    }
                }
            }
        }

        /*
         * We've potentially done callbacks. Calculate pqForegroundNew
         * based on our locked local variable ptiForegroundNew.
         */
        pqForegroundNew = NULL;
        if (ptiForegroundNew && !(ptiForegroundNew->TIF_flags & TIF_INCLEANUP)) {
            pqForegroundNew = ptiForegroundNew->pq;
        }

        /*
         * Update pqCurrent since we may have made an xxx call,
         * and this variable may be invalid.
         */
        pqCurrent = NULL;
        if (pti != NULL) {
            pqCurrent = pti->pq;
        }

        if ((pqForegroundNew != NULL) && (pqForegroundNew != pqCurrent)) {
            pqmsgActivate = AllocQEntry(&pqForegroundNew->mlInput);
            if (pqmsgActivate == NULL) {
                retval = FALSE;
                goto Exit;
            }
        }

        /*
         * Do any appropriate activation.
         */
        if (pqForegroundNew != NULL) {
            /*
             * We're going to activate (synchronously or async with an activate
             * event). We want to remove the last deactivate event if there is
             * one because this is new state. If we don't, then 1> we could
             * synchronously activate and then asynchronously deactivate,
             * thereby processing these events out of order, or 2> we could
             * pile up a chain of deactivate / activate events which would
             * make the titlebar flash alot if the app wasn't responding to
             * input for awhile (in this case, it doesn't matter if we
             * put a redundant activate in the queue, since the app is already
             * active. Remove all deactivate events because this app is
             * setting a state that is not meant to be synchronized with
             * existing queued input.
             *
             * Case: run setup, switch away (it gets deactivate event). setup
             * is not reading messages so it hasn't go it yet. It finally
             * comes up, calls SetForegroundWindow(). It's synchronous,
             * it activates ok and sets foreground. Then the app calls
             * GetMessage() and gets the deactivate. Now it isn't active.
             */
            bRemovedEvent = RemoveEventMessage(pqForegroundNew, QEVENT_DEACTIVATE, (DWORD)-1);

            /*
             * Now do any appropriate activation.  See comment below
             * for special cases.  If we're already on the foreground
             * queue we'll call xxxActivateThisWindow() directly.
             */
            if (pqForegroundNew != pqCurrent) {

                /*
                 * We do the 'pqCurrent == NULL' test to see if we're being
                 * called from the RIT.  In this case we pass NULL for the
                 * HWND which will check to see if there is already an active
                 * window for the thread and redraw its frame as truly active
                 * since it's in the foreground now.  It will also cancel any
                 * global state like LockWindowUpdate() and ClipRect().
                 */
                if ((pqCurrent == NULL) && (!(fFlags & SFW_SWITCH))) {
                    hwnd = NULL;
                } else {
                    hwnd = HW(pwnd);
                }

                if (bRemovedEvent) {
                    pqForegroundNew->QF_flags |= QF_EVENTDEACTIVATEREMOVED;
                }
                /*
                 * MSMail relies on a specific order to how win3.1 does
                 * fast switch alt-tab activation. On win3.1, it essentially
                 * activates the window, then restores it. MsMail gets confused
                 * if it isn't active when it gets restored, so this logic
                 * will make sure msmail gets restore after it gets activated.
                 *
                 * Click on a message line in the in-box, minimize msmail,
                 * alt-tab to it. The same line should have the focus if msmail
                 * got restored after it got activated.
                 *
                 * This is the history behind SFW_ACTIVATERESTORE.
                 */
                if (fFlags & SFW_ACTIVATERESTORE) {
                    uMsg = PEM_ACTIVATE_RESTORE;
                } else {
                    uMsg = 0;
                }

                if (fFlags & SFW_NOZORDER) {
                    uMsg |= PEM_ACTIVATE_NOZORDER;
                }

                StoreQMessage(pqmsgActivate, NULL, uMsg,
                        (fFlags & SFW_STARTUP) ? 0 : (WPARAM)TID(ptiForegroundOld),
                        (LPARAM)hwnd, 0, QEVENT_ACTIVATE, 0);


                /*
                 * Signal the window's thread to perform activation.  We
                 * know that ptiForegroundNew is valid because pqForegroundNew
                 * is not NULL.
                 */

                StoreQMessagePti(pqmsgActivate, ptiForegroundNew);

                SetWakeBit(ptiForegroundNew, QS_EVENTSET);

                if (pqForegroundNew->spwndActive != NULL) {
                    if (FHungApp(ptiForegroundNew, CMSHUNGAPPTIMEOUT)) {
                        TL tlpwnd;
                        ThreadLockAlwaysWithPti(ptiCurrent, pqForegroundNew->spwndActive, &tlpwnd);
                        xxxRedrawHungWindowFrame(pqForegroundNew->spwndActive, TRUE);
                        ThreadUnlock(&tlpwnd);
                    } else {
                        SetHungFlag(pqForegroundNew->spwndActive, WFREDRAWFRAMEIFHUNG);
                    }
                }

            } else {
                if (pwnd != pqCurrent->spwndActive) {
                    if (!(fFlags & SFW_STARTUP)) {
                        retval = xxxActivateThisWindow(pwnd, TID(ptiForegroundOld),
                                ((fFlags & SFW_SETFOCUS) ? 0 : ATW_SETFOCUS));

                        /*
                         * Make sure the mouse is on this window.
                         */
                        if (retval && TestUP(ACTIVEWINDOWTRACKING)) {
                            zzzActiveCursorTracking(pwnd);
                        }
                        goto Exit;
                    }

                } else {

                    /*
                     * If pwnd is already the active window, just make sure
                     * it's drawn active and on top (if requested).
                     */
                    xxxSendMessage(pwnd, WM_NCACTIVATE,
                            TRUE,
                            (LPARAM)HW(pwnd));
                    xxxUpdateTray(pwnd);
                    if (!(fFlags & SFW_NOZORDER)) {
                        xxxSetWindowPos(pwnd, PWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                    }
                }
            }


        } /* if (pqForegroundNew != NULL) */

        /*
         * First update pqForegroundOld and pqCurrent since we may have
         * made an xxx call, and these variables may be invalid.
         */
        pqForegroundOld = NULL;
        if (ptiForegroundOld && !(ptiForegroundOld->TIF_flags & TIF_INCLEANUP)) {
            pqForegroundOld = ptiForegroundOld->pq;
        }

        pqCurrent = NULL;
        if (pti != NULL)
            pqCurrent = pti->pq;

        /*
         * Now check to see if we needed to do any 'local' deactivation.
         * (ie.  were we on the queue that is being deactivated by this
         * SetForegroundWindow() call?)
         */
        if ((pqForegroundOld != NULL) && (pqForegroundOld == pqCurrent)) {
            xxxDeactivate(pti, (pwnd != NULL) ? TIDq(GETPTI(pwnd)) : 0);
        }
Exit:
        ThreadUnlockPti(ptiCurrent, &tlpti);
        ThreadUnlockPti(ptiCurrent, &tlptiForegroundNew);
        ThreadUnlockPti(ptiCurrent, &tlptiForegroundOld);
    }

    return retval;
}
/***************************************************************************\
* FRemoveForegroundActivate
*
* Returns TRUE if the foreground activate right was removed.
*
* 05-12-97 GerardoB     Extracted from FAllowForegroundActivate.
\***************************************************************************/
BOOL FRemoveForegroundActivate(PTHREADINFO pti)
{
    BOOL fRemoved;
    PPROCESSINFO ppi;
    /*
     * W32PF_APPSTARTING gets turned off the first activate this process does.
     * We assume it's ready now for action.
     */
    ppi = pti->ppi;
    if (ppi->W32PF_Flags & W32PF_APPSTARTING) {
        ClearAppStarting(ppi);
    }

    /*
     * Remove the right if present.
     */
    fRemoved =  (pti->TIF_flags & TIF_ALLOWFOREGROUNDACTIVATE);
    if (fRemoved) {
        pti->TIF_flags &= ~TIF_ALLOWFOREGROUNDACTIVATE ;
        TAGMSG1(DBGTAG_FOREGROUND, "FRemoveForegroundActivate clear TIF %#p", pti);
    } else {
        fRemoved = (ppi->W32PF_Flags & W32PF_ALLOWFOREGROUNDACTIVATE);
    }
    if (fRemoved) {
        ppi->W32PF_Flags &= ~W32PF_ALLOWFOREGROUNDACTIVATE;
        TAGMSG1(DBGTAG_FOREGROUND, "FRemoveForegroundActivate clear W32PF %#p", ppi);
    }

    return fRemoved;

}
/***************************************************************************\
* FAllowForegroundActivate
*
* Checks to see if we previously have allowed this process or thread to
* do a foreground activate - meaning, next time it becomes active, whether
* we'll allow it to come to the foreground.  Sometimes processes are granted
* the right to foreground activate themselves, if they aren't foreground,
* like when starting up (there are other cases). Grant this if this process
* is allowed.
*
* 09-08-92 ScottLu      Created.
\***************************************************************************/

BOOL FAllowForegroundActivate(
    PQ pq,
    PWND pwnd)
{
    PTHREADINFO  ptiCurrent = PtiCurrent();
    UserAssert(pwnd != NULL);
    /*
     * Bail if this guy doesn't have the foreground activate right.
     */
    TAGMSG1(DBGTAG_FOREGROUND, "FAllowForegroundActivate FRemoveForegroundActivate %#p", ptiCurrent);
    if (!FRemoveForegroundActivate(ptiCurrent)) {
        return FALSE;
    }
    /*
     * Don't try to foreground activate if:
     *  we're not on the right desktop.
     *  we're already in the foreground
     *  the foreground is locked
     * It'll fail in SetForegroundWindow2() anyway. This way
     * ActivateWindow() will still locally activate the window.
     */
    if ((ptiCurrent->rpdesk != grpdeskRitInput)
            || (gpqForeground == pq)
            || IsForegroundLocked()) {
        TAGMSG0(DBGTAG_FOREGROUND, "FAllowForegroundActivate FALSE due to addtional checks");
        return FALSE;
    }
    /*
     * noactivate windows cannot take the foreground unless explicitly requested.
     * Note that windows passed to this function are expected to be toplevel, which is
     *  where this style has meaning. This might not be the case if AW_SKIP picked an
     *  owner window which is not top level. Since noactivate doesn't apply to the owner
     *  chain, it's OK to ignore this.
     */
    #if DBG
    if (TestwndChild(pwnd)) {
        RIPMSG1(RIP_WARNING, "FAllowForegroundActivate pwnd %#p is not top level", pwnd);
    }
    #endif
    if (TestWF(pwnd, WEFNOACTIVATE)) {
        TAGMSG1(DBGTAG_FOREGROUND, "FAllowForegroundActivate noactivate window:%#p", pwnd);
        return FALSE;
    }

    return TRUE;
}

/***************************************************************************\
* xxxSetFocus (API)
*
* History:
* 11-08-90 DavidPe      Ported.
\***************************************************************************/

PWND xxxSetFocus(
    PWND pwnd)
{
    HWND hwndTemp;
    PTHREADINFO ptiCurrent = PtiCurrent();
    PTHREADINFO ptiActiveKL;
    PWND pwndTemp = NULL;
    TL tlpwndTemp;

    CheckLock(pwnd);
    /*
     * Special case if we are setting the focus to a null window.
     */
    if (pwnd == NULL) {
        if (IsHooked(ptiCurrent, WHF_CBT) && xxxCallHook(HCBT_SETFOCUS, 0,
                (LPARAM)HW(ptiCurrent->pq->spwndFocus), WH_CBT)) {
            return NULL;
        }

        /*
         * Save old focus so that we can return it.
         */
        hwndTemp = HW(ptiCurrent->pq->spwndFocus);
        xxxSendFocusMessages(ptiCurrent, pwnd);
        return RevalidateHwnd(hwndTemp);
    }

    /*
     * We no longer allow inter-thread set focuses.
     */
    if (GETPTI(pwnd)->pq != ptiCurrent->pq) {
        return NULL;
    }

    /*
     * If the window recieving the focus or any of its ancestors is either
     * minimized or disabled, don't set the focus.
     */
    for (pwndTemp = pwnd; pwndTemp != NULL; pwndTemp = pwndTemp->spwndParent) {
        if (TestWF(pwndTemp, WFMINIMIZED) || TestWF(pwndTemp, WFDISABLED)) {

            /*
             * Don't change the focus if going to a minimized or disabled
             * window.
             */
            return NULL;
        }

        if (!TestwndChild(pwndTemp)) {
            break;
        }
    }
    UserAssert(pwndTemp != NULL);

    /*
     * pwndTemp should now be the top level ancestor of pwnd.
     */
    ThreadLockWithPti(ptiCurrent, pwndTemp, &tlpwndTemp);
    if (pwnd != ptiCurrent->pq->spwndFocus) {
        if (IsHooked(ptiCurrent, WHF_CBT) && xxxCallHook(HCBT_SETFOCUS, (WPARAM)HWq(pwnd),
                (LPARAM)HW(ptiCurrent->pq->spwndFocus), WH_CBT)) {
            ThreadUnlock(&tlpwndTemp);
            return NULL;
        }

        /*
         * Activation must follow the focus.  That is, setting the focus to
         * a particualr window means that the top-level parent of this window
         * must be the active window (top-level parent is determined by
         * following the parent chain until you hit a top-level guy).  So,
         * we must activate this top-level parent if it is different than
         * the current active window.
         *
         * Only change activation if top-level parent is not the currently
         * active window.
         */
        if (pwndTemp != ptiCurrent->pq->spwndActive) {

            /*
             * If this app is not in the foreground, see if foreground
             * activation is allowed.
             */
            if (ptiCurrent->pq != gpqForeground && FAllowForegroundActivate(ptiCurrent->pq, pwndTemp)) {
                /*
                 * If the process lost the foreground activation right by giving
                 * focus to a hidden window, then give it the right back. See
                 * bug #401932 for how this might affect an app
                 */
                if (!TestWF(pwndTemp, WFVISIBLE)){
                    ptiCurrent->ppi->W32PF_Flags |= W32PF_ALLOWFOREGROUNDACTIVATE;
                }
                if (!xxxSetForegroundWindow2(pwndTemp, ptiCurrent, SFW_SETFOCUS)) {
                    ThreadUnlock(&tlpwndTemp);
                    return NULL;
                }
            }

            /*
             * This will return FALSE if something goes wrong.
             */
            if (pwndTemp != ptiCurrent->pq->spwndActive) {
                if (!xxxActivateThisWindow(pwndTemp, 0, 0)) {
                    ThreadUnlock(&tlpwndTemp);
                    return NULL;
                }
            }
        }

        /*
         * Save current pwndFocus since we must return this.
         */
        pwndTemp = ptiCurrent->pq->spwndFocus;
        ThreadUnlock(&tlpwndTemp);
        ThreadLockWithPti(ptiCurrent, pwndTemp, &tlpwndTemp);

        /*
         * Change the global pwndFocus and send the WM_{SET/KILL}FOCUS
         * messages.
         */
        xxxSendFocusMessages(ptiCurrent, pwnd);

    } else {
        pwndTemp = ptiCurrent->pq->spwndFocus;
    }

    if (ptiCurrent->pq->spwndFocus) {
        /*
         * For the shell notification hook, we should use the pti->spkl
         * of the window with the focus. This could be a different thread,
         * (or even different process) when the queue is attached. The typical
         * case would be OLE out-of-process server.
         * #352877
         */
        ptiActiveKL = GETPTI(ptiCurrent->pq->spwndFocus);
    } else {
        /*
         * Preserving the NT4 behavior, otherwise.
         */
        ptiActiveKL = ptiCurrent;
    }
    UserAssert(ptiActiveKL);

    /*
     * Update the keyboard icon on the tray if the layout changed during focus change.
     * Before winlogon loads kbd layouts, pti->spkActive is NULL. #99321
     */
    if (ptiActiveKL->spklActive) {
        HKL hklActive = ptiActiveKL->spklActive->hkl;

        if ((gLCIDSentToShell != hklActive) && IsHooked(ptiCurrent, WHF_SHELL)) {
            gLCIDSentToShell = hklActive;
            xxxCallHook(HSHELL_LANGUAGE, (WPARAM)NULL, (LPARAM)hklActive, WH_SHELL);
        }
    }

    hwndTemp = HW(pwndTemp);
    ThreadUnlock(&tlpwndTemp);

    /*
     * Return the pwnd of the window that lost the focus.
     * Return the validated hwndTemp: since we locked/unlocked pwndTemp,
     * it may be gone.
     */
    return RevalidateHwnd(hwndTemp);
}


/***************************************************************************\
* xxxSetActiveWindow (API)
*
*
* History:
* 11-08-90 DavidPe      Created.
\***************************************************************************/

PWND xxxSetActiveWindow(
    PWND pwnd)
{
    HWND hwndActiveOld;
    PTHREADINFO pti;

    CheckLock(pwnd);

    pti = PtiCurrent();

    /*
     * 32 bit apps must call SetForegroundWindow (to be NT 3.1 compatible)
     * but 16 bit apps that are foreground can make other apps foreground.
     * xxxActivateWindow makes sure an app is foreground.
     */
    if (!(pti->TIF_flags & TIF_16BIT) && (pwnd != NULL) && (GETPTI(pwnd)->pq != pti->pq)) {
        return NULL;
    }

    hwndActiveOld = HW(pti->pq->spwndActive);

    xxxActivateWindow(pwnd, AW_USE);

    return RevalidateHwnd(hwndActiveOld);
}


/***************************************************************************\
* xxxActivateWindow
*
* Changes the active window.  Given the pwnd and cmd parameters, changes the
* activation according to the following rules:
*
*  If cmd ==
*      AW_USE  Use the pwnd passed as the new active window.  If this
*              window cannot be activated, return FALSE.
*
*      AW_TRY  Try to use the pwnd passed as the new active window.  If
*              this window cannot be activated activate another window
*              using the rules for AW_SKIP.
*
*      AW_SKIP Activate any other window than pwnd passed.  The order of
*              searching for a candidate is as follows:
*              -   If pwnd is a popup, try its owner
*              -   else scan the top-level window list for the first
*                  window that is not pwnd that can be activated.
*
*      AW_USE2 Same as AW_USE except that the wParam on the WM_ACTIVATE
*              message will be set to 2 rather than the default of 1. This
*              indicates the activation is being changed due to a mouse
*              click.
*
*      AW_TRY2 Same as AW_TRY except that the wParam on the WM_ACTIVATE
*              message will be set to 2 rather than the default of 1. This
*              indicates the activation is being changed due to a mouse
*              click.
*
*      AW_SKIP2 Same as AW_SKIP, but we skip the first check that AW_SKIP
*              performes (the pwndOwner test).  This is used when
*              the pwnd parameter is NULL when this function is called.
*
*  This function returns TRUE if the activation changed and FALSE if
*  it did not change.
*
*  This function calls xxxActivateThisWindow() to actually do the activation.
*
* History:
* 11-08-90 DavidPe      Ported.
\***************************************************************************/

BOOL xxxActivateWindow(
    PWND pwnd,
    UINT cmd)
{
    DWORD fFlags = ATW_SETFOCUS;
    PTHREADINFO ptiCurrent = PtiCurrent();
    TL tlpwnd;
    BOOL fSuccess;
    BOOL fAllowForeground, fSetForegroundRight;

    CheckLock(pwnd);


    if (pwnd != NULL) {

        /*
         * See if this window is OK to activate
         * (Cannot activate child windows).
         */
        if (TestwndChild(pwnd))
            return FALSE;

    } else {
        cmd = AW_SKIP2;
    }

    switch (cmd) {

    case AW_TRY2:
        fFlags |= ATW_MOUSE;

    /*
     *** FALL THRU **
     */
    case AW_TRY:

        /*
         * See if this window is OK to activate.
         */
        if (!FBadWindow(pwnd)) {
            break;
        }

    /*
     * If pwnd can not be activated, drop into the AW_SKIP case.
     */
    case AW_SKIP:

        /*
         * Try the owner of this popup.
         */
        if (TestwndPopup(pwnd) && !FBadWindow(pwnd->spwndOwner)) {
            pwnd = pwnd->spwndOwner;
            break;
        }

        /*
         * fall through
         */

    case AW_SKIP2:

        /*
         * Try the previously active window but don't activate a shell window
         */
        if ((gpqForegroundPrev != NULL)
                && !FBadWindow(gpqForegroundPrev->spwndActivePrev)
                /*
                 * Bug 290129 - joejo
                 *
                 * Test for WFBOTTOMMOST as opposed to WEFTOOLWINDOW to fix
                 * issue with Office2000 assistant and balloon help.
                 */
                && !TestWF(gpqForegroundPrev->spwndActivePrev, WFBOTTOMMOST)) {

            pwnd = gpqForegroundPrev->spwndActivePrev;
            break;
        }

        {
            PWND pwndSave = pwnd;
            DWORD flags = NTW_IGNORETOOLWINDOW;

TryAgain:
            /*
             * Find a new active window from the top-level window list,
             * skip tool windows the first time through.
             */
            pwnd = NextTopWindow(ptiCurrent, pwndSave, (cmd == AW_SKIP ? pwndSave : NULL),
                                 flags);

            if (pwnd) {
                if (!FBadWindow(pwnd->spwndLastActive))
                    pwnd = pwnd->spwndLastActive;
            } else {
                if (flags == NTW_IGNORETOOLWINDOW) {
                    flags = 0;
                    goto TryAgain;
                }
            }
        }


    case AW_USE:
        break;

    case AW_USE2:
        fFlags |= ATW_MOUSE;
        break;

    default:
        return FALSE;
    }

    if (pwnd == NULL)
        return FALSE;

    ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwnd);

    if (GETPTI(pwnd)->pq == ptiCurrent->pq) {
        /*
         * Activation is within this queue. Usually this means just do
         * all the normal message sending. But if this queue isn't the
         * foreground queue, check to see if it is allowed to become
         * foreground.
         */

        /*
         * Sometimes processes are granted the right to foreground
         * activate themselves, if they aren't foreground, like
         * when starting up (there are other cases). Grant this if
         * this process is allowed.
         */

         /*
          * Removed the first clause from the following if statement
          * if (pti->pq == gpqForeground || !FAllowForegroundActivate(pti->pq)) {
          * This fixes the problem where foreground app A activates app B
          * the user switches to app C, then B does something to activate A
          * (like destroy an owned window).  A now comes to the foreground
          * unexpectedly. This clause is not in Win95 code and was added in
          * 3.51 code to fix some test script hang (Bug 7461)
          */

        if (!FAllowForegroundActivate(ptiCurrent->pq, pwnd)) {
            fSuccess = xxxActivateThisWindow(pwnd, 0, fFlags);
            ThreadUnlock(&tlpwnd);
            return fSuccess;
        }

        fAllowForeground = TRUE;
        /*
         * If this thread doesn't have any top-level non-minimized visible windows,
         *  let it keep the right since it's probably not done with activation yet.
         * Bug 274383 - joejo
         */
        fSetForegroundRight = (ptiCurrent->cVisWindows == 0);

    } else {
        /*
         * If the caller is in the foreground, it has the right to change
         * the foreground itself.
         */
        fAllowForeground = (gpqForeground == ptiCurrent->pq)
                                || (gpqForeground == NULL);
        /*
         * Give the right to change the foreground to this thread only if it already
         *  has it, it has more visible windows or this is an explicit request to
         *  activate the given window.
         * When an app destroys/hides the active (foreground) window, we choose a new
         *  active window and will probably hit this code. We don't want to give them the
         *  right to change the foreground in this case since it's us making the activation
         *  (See comments below). We let them keep the right so apps destroying their last
         *  visible window (ie a splash initialization window) can take the foreground again
         *  when they create another window (the main window).
         */
        if (fAllowForeground) {
            fSetForegroundRight = ((ptiCurrent->TIF_flags & TIF_ALLOWFOREGROUNDACTIVATE)
                                        || (ptiCurrent->cVisWindows != 0)
                                        || (cmd == AW_USE));
        } else {
            fSetForegroundRight = FALSE;
        }
    }

    fSuccess = FALSE;
    if (fAllowForeground) {
        /*
         * Hack! Temporarily give this thread a foreground right to make sure
         *  this call succeds.
         */
        ptiCurrent->TIF_flags |= TIF_ALLOWFOREGROUNDACTIVATE;
        TAGMSG1(DBGTAG_FOREGROUND, "xxxActivateWindow temporarly set TIF %#p", ptiCurrent);
        fSuccess = xxxSetForegroundWindow(pwnd, (cmd == AW_USE));

        if (fSetForegroundRight) {
            /*
             * We activated some other app on purpose. If so that means this
             * thread is probably controlling this window and will probably want
             * to set itself active and foreground really soon again (for example,
             * a setup program doing dde to progman). A real live case: wingz -
             * bring up page setup..., options..., ok, ok. Under Win3.1 the
             * activation goes somewhere strange and then wingz calls
             * SetActiveWindow() to bring it back. This'll make sure that works.
             *
             * We used to set this before calling xxxSetForegeroundWindow above.
             * This would cause callers doing an intra-queue activation to
             *  retain their foreground right eventhough it is supposed to be
             *  a one shot deal (that's why FAllowForeground clears the bits).
             * In addtion, xxxSetForegroundWindow might clear the bits (it didnt'
             *  used to); so we do it here, and only if we did an inter-queue
             *  activation
             */
            ptiCurrent->TIF_flags |= TIF_ALLOWFOREGROUNDACTIVATE;
            TAGMSG1(DBGTAG_FOREGROUND, "xxxActivateWindow set TIF %#p", ptiCurrent);
        } else {
            /*
             * Make sure to remove the temporary right.
             */
            ptiCurrent->TIF_flags &= ~TIF_ALLOWFOREGROUNDACTIVATE;
            TAGMSG1(DBGTAG_FOREGROUND, "xxxActivateWindow clear TIF %#p", ptiCurrent);
        }
    }

    ThreadUnlock(&tlpwnd);
    return fSuccess;
}


/***************************************************************************\
* GNT_NextTopScan
*
* Starting at hwnd (or hwndDesktop->hwndChild if hwnd == NULL), find
* the next window owned by hwndOwner.
*
* History:
* 11-08-90 DavidPe      Ported.
* 02-11-91 JimA         Multi-desktop support.
\***************************************************************************/

PWND GNT_NextTopScan(
    PTHREADINFO pti,
    PWND pwnd,
    PWND pwndOwner)
{
    if (pwnd == NULL) {
        UserAssert(pti->rpdesk != NULL &&
                   (pti->rpdesk->dwDTFlags & DF_DESKWNDDESTROYED) == 0);
        pwnd = pti->rpdesk->pDeskInfo->spwnd->spwndChild;
    } else {
        pwnd = pwnd->spwndNext;
    }

    for (; pwnd != NULL; pwnd = pwnd->spwndNext) {
        if (pwnd->spwndOwner == pwndOwner)
            break;
    }

    return pwnd;
}


/***************************************************************************\
* NTW_GetNextTop
*
* <brief description>
*
* History:
* 11-08-90 DavidPe      Ported.
* 02-11-91 JimA         Multi-desktop support.
\***************************************************************************/

PWND NTW_GetNextTop(
    PTHREADINFO pti,
    PWND pwnd)
{
    PWND pwndOwner;

    if (pwnd == NULL) {
        goto ReturnFirst;
    }

    /*
     * First look for any windows owned by this window
     * If that fails, then go up one level to our owner,
     * and look for next window owned by his owner.
     * This results in a depth-first ordering of the windows.
     */

    pwndOwner = pwnd;
    pwnd = NULL;

    do {
        if ((pwnd = GNT_NextTopScan(pti, pwnd, pwndOwner)) != NULL) {
            return pwnd;
        }

        pwnd = pwndOwner;
        if (pwnd != NULL)
            pwndOwner = pwnd->spwndOwner;

    } while (pwnd != NULL);

ReturnFirst:

    /*
     * If no more windows to enumerate, return the first unowned window.
     */
    return GNT_NextTopScan(pti, NULL, NULL);
}


/***************************************************************************\
* NTW_GetPrevTop
*
* <brief description>
*
* History:
* 11-08-90 DavidPe      Ported.
* 02-11-91 JimA         Multi-desktop support.
\***************************************************************************/

PWND NTW_GetPrevTop(
    PTHREADINFO pti,
    PWND pwndCurrent)
{
    PWND pwnd;
    PWND pwndPrev;

    /*
     * Starting from beginning, loop thru the windows, saving the previous
     * one, until we find the window we're currently at.
     */
    pwndPrev = NULL;

    do {
        pwnd = NTW_GetNextTop(pti, pwndPrev);
        if (pwnd == pwndCurrent && pwndPrev != NULL) {
            break;
        }
    } while ((pwndPrev = pwnd) != NULL);

    return pwndPrev;
}


/***************************************************************************\
* NextTopWindow
*
* <brief description>
*
* History:
* 11-08-90 DavidPe      Ported.
* 02-11-91 JimA         Multi-desktop support.
\***************************************************************************/

PWND CheckTopLevelOnly(
    PWND pwnd)
{
    /*
     * fnid == -1 means this is a desktop window - find the first child
     * of this desktop, if it is one.
     */
    while (pwnd != NULL && GETFNID(pwnd) == FNID_DESKTOP) {
        pwnd = pwnd->spwndChild;
    }

    return pwnd;
}


PWND NextTopWindow(
    PTHREADINFO pti,
    PWND        pwnd,
    PWND        pwndSkip,
    DWORD       flags )
{
    BOOL fFoundFirstUnowned;
    PWND pwndPrev;
    PWND pwndStart = pwnd;
    PWND pwndFirstUnowned;

    /*
     * If the search gets to the first unowned window TWICE (See NTW_GetNextTop),
     * we couldn't find a window
     */
    pwndFirstUnowned = GNT_NextTopScan(pti, NULL, NULL);
    fFoundFirstUnowned = FALSE;

    if (pwnd == NULL) {
        pwnd = NTW_GetNextTop(pti, NULL);

        /*
         * Don't allow desktop windows.
         */
        pwnd = pwndStart = CheckTopLevelOnly(pwnd);

        if (pwnd == NULL)
            return NULL;    // No more windows owned by the thread

        goto Loop;
    }

    /*
     * Don't allow desktop windows.
     */
    pwnd = pwndStart = CheckTopLevelOnly(pwnd);
    if (pwnd == NULL)
        return NULL;        // No more windows owned by this thread

    /*
     * Don't allow desktop windows.
     */
    pwndSkip = CheckTopLevelOnly(pwndSkip);



    while (TRUE) {
        pwndPrev = pwnd;
        pwnd = ((flags & NTW_PREVIOUS) ? NTW_GetPrevTop(pti, pwnd) : NTW_GetNextTop(pti, pwnd));

        /*
         * If we've cycled to where we started, couldn't find one: return NULL
         */
        if (pwnd == pwndStart)
            break;

        if (pwnd == pwndFirstUnowned) {
            if (fFoundFirstUnowned) {
                break;
            } else {
                fFoundFirstUnowned = TRUE;
            }
        }

        if (pwnd == NULL)
            break;

        /*
         * If we've cycled over desktops, then return NULL because we'll
         * never hit pwndStart.
         */
        if (PWNDDESKTOP(pwndStart) != PWNDDESKTOP(pwnd))
            break;

        /*
         * going nowhere is a bad sign.
         */
        if (pwndPrev == pwnd) {
            /*
             * This is a temporary fix chosen because its safe.  This case
             * was hit when a window failed the NCCREATE message and fell
             * into xxxFreeWindow and left the critical section after being
             * unlinked.  The app then died and entered cleanup code and
             * tried to destroy this window again.
             */
            break;
        }

Loop:
        if (pwnd == pwndSkip)
            continue;

        /*
         *  If it's visible, not disabled, not a noactivate window
         *   and either we're not ignoringtool windows or it's not a
         *  tool window, then we've got it.
         */
        if (TestWF(pwnd, WFVISIBLE) &&
            !TestWF(pwnd, WFDISABLED) &&
            !TestWF(pwnd, WEFNOACTIVATE) &&
            (!(flags & NTW_IGNORETOOLWINDOW) || !TestWF(pwnd, WEFTOOLWINDOW))) {

            return pwnd;
        }
    }

    return NULL;
}


/***************************************************************************\
* xxxCheckFocus
*
*
* History:
* 11-08-90 DarrinM      Ported.
\***************************************************************************/

void xxxCheckFocus(
    PWND pwnd)
{
    TL tlpwndParent;
    PTHREADINFO pti;

    CheckLock(pwnd);

    pti = PtiCurrent();

    if (pwnd == pti->pq->spwndFocus) {

        /*
         * Set focus to parent of child window.
         */
        if (TestwndChild(pwnd)) {
            ThreadLockWithPti(pti, pwnd->spwndParent, &tlpwndParent);
            xxxSetFocus(pwnd->spwndParent);
            ThreadUnlock(&tlpwndParent);
        } else {
            xxxSetFocus(NULL);
        }
    }

    if (pwnd == pti->pq->caret.spwnd) {
        zzzDestroyCaret();
    }
}


/***************************************************************************\
* SetForegroundThread
*
*
* History:
* 12-xx-91 MarkL    Created.
* 02-12-92 DavidPe  Rewrote as SetForegroundThread().
\***************************************************************************/

VOID SetForegroundThread(
    PTHREADINFO pti)
{
    PKL pklPrev;

    if (pti == gptiForeground)
        return;

    /*
     * The foregorund thread must be on the foreground queue.
     * xxxSendFocusMessages obtains this pti from a window
     *  received as a parameter. If the owner of the window
     *  exited during a callback (in the caller), then the pti
     *  will be gptiRit,which might not be in the foreground queue
     */
    UserAssert((pti == NULL)
                || (pti->pq == gpqForeground)
                || (pti == gptiRit));

    /*
     * If we're changing gptiForeground to another process,
     * change the base priorities of the two processes.  We
     * know that if either 'pti' or 'gptiForeground' is NULL
     * that both aren't NULL due to the first test in this
     * function.
     */
    if ((pti == NULL) || (gptiForeground == NULL) ||
            (pti->ppi != gptiForeground->ppi)) {
        if (gptiForeground != NULL) {
            gptiForeground->ppi->W32PF_Flags &= ~W32PF_FORCEBACKGROUNDPRIORITY;
            SetForegroundPriority(gptiForeground, FALSE);
        }

        if (pti != NULL) {
            SetForegroundPriority(pti, TRUE);
        }
    }

    if (gptiForeground) {
        pklPrev = gptiForeground->spklActive;
    } else {
        pklPrev = NULL;
    }
    gptiForeground = pti;
    if (gptiForeground && gptiForeground->spklActive) {
        ChangeForegroundKeyboardTable(pklPrev, gptiForeground->spklActive);
    }

    /*
     * Clear recent down information in the async key state to prevent
     * spying by apps.
     */
    RtlZeroMemory(gafAsyncKeyStateRecentDown, CBKEYSTATERECENTDOWN);

    /*
     * Update the async key cache index.
     */
    gpsi->dwAsyncKeyCache++;
}

VOID SetForegroundPriorityProcess(
    PPROCESSINFO    ppi,
    PTHREADINFO     pti,
    BOOL            fSetForeground)
{
    PEPROCESS Process;
    UCHAR PriorityClassSave;

    UserAssert(ppi != NULL);

    Process = ppi->Process;
    UserAssert(ppi->Process != NULL);

    if (ppi->W32PF_Flags & W32PF_IDLESCREENSAVER) {
        fSetForeground = FALSE;
        PriorityClassSave = Process->PriorityClass;
        Process->PriorityClass = PROCESS_PRIORITY_CLASS_IDLE;
    }

    /*
     * If we previously delayed setting some process to the background
     * because a screen saver was starting up, do it now.
     */
    if (gppiForegroundOld != NULL) {
        if (gppiForegroundOld == ppi) {
            gppiForegroundOld = NULL;
        } else if (ppi != gppiScreenSaver) {
            PsSetProcessPriorityByClass(gppiForegroundOld->Process, PsProcessPriorityBackground);
            gppiForegroundOld = NULL;
        }
    }

    /*
     * If this app should be background, don't let it go foreground.
     * Foreground apps run at a higher base priority.
     */
    if (ppi->W32PF_Flags & W32PF_FORCEBACKGROUNDPRIORITY) {
        if (pti != NULL && !(pti->TIF_flags & TIF_GLOBALHOOKER)) {
            PsSetProcessPriorityByClass(Process, PsProcessPrioritySpinning);
        }
    } else if (fSetForeground) {
        PsSetProcessPriorityByClass(Process, PsProcessPriorityForeground);
    } else if (pti != NULL && !(pti->TIF_flags & TIF_GLOBALHOOKER)) {
        /*
         * Don't adjust the priority of the current foreground process if
         * the new foreground process is a screen saver.
         */
        if (gppiScreenSaver && gppiScreenSaver != ppi) {
            gppiForegroundOld = ppi;
        } else {
            PsSetProcessPriorityByClass(Process, PsProcessPriorityBackground);
        }
    }

    if (ppi->W32PF_Flags & W32PF_IDLESCREENSAVER) {
        Process->PriorityClass = PriorityClassSave;
    }
}


VOID SetForegroundPriority(
    PTHREADINFO pti,
    BOOL fSetForeground)
{
    UserAssert(pti != NULL);

    /*
     * We don't want to change the priority of system or console threads
     */
    if (pti->TIF_flags & (TIF_SYSTEMTHREAD | TIF_CSRSSTHREAD))
        return;

    SetForegroundPriorityProcess(pti->ppi, pti, fSetForeground);
}
