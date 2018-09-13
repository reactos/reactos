/****************************** Module Header ******************************\
* Module Name: capture.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* History:
* 08-Nov-1990 DavidPe   Created.
* 01-Feb-1991 MikeKe    Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* xxxSetCapture (API)
*
* This function sets the capture window for the current queue.
*
* History:
* 08-Nov-1990 DavidPe   Created.
\***************************************************************************/

PWND xxxSetCapture(
    PWND pwnd)
{
    PQ   pq;
    PWND pwndCaptureOld;
    HWND hwndCaptureOld;
    PTHREADINFO  ptiCurrent = PtiCurrent();

    pq = (PQ)PtiCurrent()->pq;

    /*
     * If the capture is locked, bail
     */
    if (pq->QF_flags & QF_CAPTURELOCKED) {
        RIPMSG2(RIP_WARNING, "xxxSetCapture(%#p): Capture is locked. pq:%#p", pwnd, pq);
        return NULL;
    }

    /*
     * Don't allow the app to set capture to a window
     * from another queue.
     */
    if ((pwnd != NULL) && GETPTI(pwnd)->pq != pq)
        return NULL;

    /*
     * If full screen capture don't allow any other capture
     */
    if (gspwndScreenCapture)
        return NULL;

    pwndCaptureOld = pq->spwndCapture;
    hwndCaptureOld = HW(pwndCaptureOld);

    xxxCapture(ptiCurrent, pwnd, CLIENT_CAPTURE);

    if (hwndCaptureOld != NULL) {

        if (RevalidateHwnd(hwndCaptureOld))
            return pwndCaptureOld;
    }

    return NULL;
}

/***************************************************************************\
* xxxReleaseCapture (API)
*
* This function release the capture for the current queue.
*
* History:
* 08-Nov-1990 DavidPe   Created.
* 16-May-1991 MikeKe    Changed to return BOOL
\***************************************************************************/

BOOL xxxReleaseCapture(VOID)
{
    PTHREADINFO ptiCurrent = PtiCurrent();
    /*
     * If the capture is locked, bail
     */
    if (ptiCurrent->pq->QF_flags & QF_CAPTURELOCKED) {
        RIPMSG0(RIP_WARNING, "xxxReleaseCapture: Capture is locked");
        return FALSE;
    }

    /*
     * If we're releasing the capture from a window during tracking,
     * cancel tracking first.
     */
    if (ptiCurrent->pmsd != NULL) {

        /*
         * Only remove the tracking rectangle if it's
         * been made visible.
         */
        if (ptiCurrent->TIF_flags & TIF_TRACKRECTVISIBLE) {

            bSetDevDragRect(gpDispInfo->hDev, NULL, NULL);

            if (!(ptiCurrent->pmsd->fDragFullWindows))
                xxxDrawDragRect(ptiCurrent->pmsd, NULL, DDR_ENDCANCEL);

            ptiCurrent->TIF_flags &= ~(TIF_TRACKRECTVISIBLE | TIF_MOVESIZETRACKING);
        }
    }

    xxxCapture(ptiCurrent, NULL, NO_CAP_CLIENT);

    return TRUE;
}

/***************************************************************************\
* xxxCapture
*
* This is the workhorse routine of capture setting and releasing.
*
* History:
* 13-Nov-1990 DavidPe   Created.
\***************************************************************************/

VOID xxxCapture(
    PTHREADINFO pti,
    PWND        pwnd,
    UINT        code)
{
    CheckLock(pwnd);
    UserAssert(IsWinEventNotifyDeferredOK());

    if ((gspwndScreenCapture == NULL) ||
        (code == FULLSCREEN_CAPTURE) ||
        ((pwnd == NULL) && (code == NO_CAP_CLIENT) && (pti->pq != GETPTI(gspwndScreenCapture)->pq))) {

        PQ   pq;
        PWND pwndCaptureOld = NULL;

        if (code == FULLSCREEN_CAPTURE) {
            if (pwnd) {

                Lock(&gspwndScreenCapture, pwnd);

                /*
                 * We're going full screen so clear the mouse owner
                 */
                Unlock(&gspwndMouseOwner);

            } else {

                Unlock(&gspwndScreenCapture);
            }
        }

        /*
         * Internal capture works like Win 3.1 capture unlike the NT capture
         * which can be lost if the user clicks down on another application
         */
        if (code == CLIENT_CAPTURE_INTERNAL) {
            Lock(&gspwndInternalCapture, pwnd);
            code = CLIENT_CAPTURE;
        }

        /*
         * Free the internal capture if the app (thread) that did the internal
         * capture is freeing the capture.
         */
        if ((code == NO_CAP_CLIENT) &&
            gspwndInternalCapture   &&
            (pti == GETPTI(gspwndInternalCapture))) {

            Unlock(&gspwndInternalCapture);
        }

        if ((pq = pti->pq) != NULL) {
            PDESKTOP pdesk = pti->rpdesk;

            UserAssert(!(pq->QF_flags & QF_CAPTURELOCKED));

            /*
             * If someone is tracking mouse events in the client area and
             *  we're setting or releasing an internal capture mode (!= CLIENT_CAPTURE),
             *  then cancel tracking -- because we're either taking or relinquishing
             *  control over the mouse.
             */
            if ((pdesk->dwDTFlags & DF_TRACKMOUSEEVENT)
                    && (pdesk->htEx == HTCLIENT)
                    && ((pdesk->spwndTrack == pwnd)
                            && (code != CLIENT_CAPTURE)
                         || ((pdesk->spwndTrack == pq->spwndCapture)
                             && (pq->codeCapture != CLIENT_CAPTURE)))) {

                BEGINATOMICCHECK();
                xxxCancelMouseMoveTracking(pdesk->dwDTFlags, pdesk->spwndTrack,
                                           pdesk->htEx, DF_TRACKMOUSEEVENT);
                ENDATOMICCHECK();

            }

            pwndCaptureOld = pq->spwndCapture;
            LockCaptureWindow(pq, pwnd);
            pq->codeCapture = code;
        } else {
            /*
             * A thread without a queue?
             */
            UserAssert(pti->pq != NULL);
        }

        /*
         * If there was a capture window and we're releasing it, post
         * a WM_MOUSEMOVE to the window we're over so they can know about
         * the current mouse position.
         * Defer WinEvent notifications to protect pwndCaptureOld
         */
        DeferWinEventNotify();
        
        if (pwnd == NULL && pwndCaptureOld != NULL) {
#ifdef REDIRECTION
            if (!IsGlobalHooked(pti, WHF_FROM_WH(WH_HITTEST)))
#endif
                zzzSetFMouseMoved();
        }
        
        if (FWINABLE()) {
            if (pwndCaptureOld) {
                zzzWindowEvent(EVENT_SYSTEM_CAPTUREEND, pwndCaptureOld, OBJID_WINDOW,
                        INDEXID_CONTAINER, WEF_USEPWNDTHREAD);
            }

            if (pwnd) {
                zzzWindowEvent(EVENT_SYSTEM_CAPTURESTART, pwnd, OBJID_WINDOW,
                        INDEXID_CONTAINER, WEF_USEPWNDTHREAD);
            }
        }

        /*
         * New for win95 - send  WM_CAPTURECHANGED.
         *
         * The FNID_DELETED_BIT is set in xxxFreeWindow which means we
         * DON'T want to send the message.
         */
        if (pwndCaptureOld                        &&
            TestWF(pwndCaptureOld, WFWIN40COMPAT) &&
            !(pwndCaptureOld->fnid & FNID_DELETED_BIT)) {

            TL tlpwnd;

            /*
             * If we are in menu mode and just set capture,
             *  don't let them take it from us during this
             *  callback.
             */
            if ((pti->pMenuState != NULL) && (pwnd != NULL)) {
                pq->QF_flags |= QF_CAPTURELOCKED;
            }

            ThreadLockAlways(pwndCaptureOld, &tlpwnd);
            zzzEndDeferWinEventNotify();
            xxxSendMessageCallback(pwndCaptureOld,
                    WM_CAPTURECHANGED,
                    FALSE,
                    (LPARAM)HW(pwnd),
                    NULL,
                    0,
                    FALSE);
            /* The thread's queue may have changed during the callback,
             * so we need to refresh the local. Bug #377795
             */
            pq = pti->pq;
            UserAssert(pq != NULL);
            ThreadUnlock(&tlpwnd);

            /*
             * Release the temporary lock, if any
             */
            pq->QF_flags &= ~QF_CAPTURELOCKED;
        } else {
            zzzEndDeferWinEventNotify();
        }
    }
}
