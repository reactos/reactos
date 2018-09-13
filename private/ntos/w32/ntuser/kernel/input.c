/****************************** Module Header ******************************\
* Module Name: input.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the core functions of the input sub-system
*
* History:
* 10-18-90 DavidPe      Created.
* 02-14-91 mikeke       Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

//#define MARKPATH
#ifdef MARKPATH
BOOL gfMarkPath;
#endif

#if DEBUGTAGS

int  gnSysPeekSearch;

void CheckPtiSysPeek(int where, PQ pq, ULONG_PTR newIdSysPeek)
{
    PTHREADINFO ptiCurrent = PtiCurrent();
    DWORD       dwRip;

    dwRip = (newIdSysPeek > 1) ? RIP_THERESMORE : 0;
    TAGMSG5(DBGTAG_SysPeek | dwRip,
            "%d pti %#p sets id %#p to pq %#p ; old id %#p",
            where, ptiCurrent, newIdSysPeek, pq, pq->idSysPeek);

    if (newIdSysPeek > 1) {
        PQMSG pqmsg = (PQMSG)newIdSysPeek;
        TAGMSG5(DBGTAG_SysPeek | RIP_NONAME,
                "-> msg %lx hwnd %#p w %#p l %#p pti %#p",
                pqmsg->msg.message, pqmsg->msg.hwnd,  pqmsg->msg.wParam,
                pqmsg->msg.lParam,  pqmsg->pti);
    }
}

void CheckSysLock(int where, PQ pq, PTHREADINFO ptiSysLock)
{
    PTHREADINFO ptiCurrent = PtiCurrent();
    TAGMSG5(DBGTAG_SysPeek,
            "%d pti %#p sets ptiSL %#p to pq %#p ; old ptiSL %#p\n",
            where, ptiCurrent, ptiSysLock, pq, pq->ptiSysLock);
}
#endif //DEBUGTAGS

#if DBG
BOOL gfLogPlayback;

LPCSTR aszMouse[] = {
    "WM_MOUSEMOVE",
    "WM_LBUTTONDOWN",
    "WM_LBUTTONUP",
    "WM_LBUTTONDBLCLK",
    "WM_RBUTTONDOWN",
    "WM_RBUTTONUP",
    "WM_RBUTTONDBLCLK",
    "WM_MBUTTONDOWN",
    "WM_MBUTTONUP",
    "WM_MBUTTONDBLCLK"
    "WM_MOUSEWHEEL",
    "WM_XBUTTONDOWN",
    "WM_XBUTTONUP",
    "WM_XBUTTONDBLCLK",
};
LPCSTR aszKey[] = {
    "WM_KEYDOWN",
    "WM_KEYUP",
    "WM_CHAR",
    "WM_DEADCHAR",
    "WM_SYSKEYDOWN",
    "WM_SYSKEYUP",
    "WM_SYSCHAR",
    "WM_SYSDEADCHAR"
};
#endif  // DBG

#define CANCEL_ACTIVESTATE  0
#define CANCEL_FOCUSSTATE   1
#define CANCEL_CAPTURESTATE 2

#define KEYSTATESIZE    (CBKEYSTATE + CBKEYSTATERECENTDOWN)


/*
 * xxxGetNextSysMsg return values
 */
#define PQMSG_PLAYBACK       ((PQMSG)1)

BOOL xxxScanSysQueue(PTHREADINFO ptiCurrent, LPMSG lpMsg, PWND pwndFilter,
        UINT msgMinFilter, UINT msgMaxFilter, DWORD flags, DWORD fsReason);
BOOL xxxReadPostMessage(PTHREADINFO pti, LPMSG lpMsg, PWND pwndFilter,
        UINT msgMin, UINT msgMax, BOOL fRemoveMsg);
void CleanEventMessage(PQMSG pqmsg);

/***************************************************************************\
* xxxWaitMessage (API)
*
* This API will block until an input message is received on
* the current queue.
*
* History:
* 10-25-90 DavidPe      Created.
\***************************************************************************/

BOOL xxxWaitMessage(VOID)
{
    return xxxSleepThread(QS_ALLINPUT | QS_EVENT, 0, TRUE);
}


/***************************************************************************\
* CheckProcessBackground/Foreground
*
* This checks to see if the process is at the right priority. If CSPINS is
* greater than CSPINBACKGROUND and the process isn't at the background
* priority, put it there. If it is less than this and should be foreground
* and isn't put it there.
*
* Need to put foreground spinning apps in the background (make it the same
* priority as all other background apps) so that bad apps can still communicate
* with apps in the background via dde, for example. There are other cases
* where spinning foreground apps affect the server, the printer spooler, and
* mstest scenarios. On Win3.1, calling PeekMessage() involves making a trip
* through the scheduler, forcing other apps to run. Processes run with
* priority on NT, where the foreground process gets foreground priority for
* greater responsiveness.
*
* If an app calls peek/getmessage without idling, count how many times this
* happens - if it happens CSPINBACKGROUND or more times, make the process
* background. This handles most of the win3.1 app compatibility spinning
* cases. If there is no priority contention, the app continues to run at
* full speed (no performance scenarios should be adversely affected by this).
*
* This solves these cases:
*
* - high speed timer not allowing app to go idle
* - post/peek loop (receiving a WM_ENTERIDLE, and posting a msg, for example)
* - peek no remove loop (winword "idle" state, most dde loops, as examples)
*
* But doesn't protect against these sort of cases:
*
* - app calls getmessage, then goes into a tight loop
* - non-gui threads in tight cpu loops
*
* 02-08-93 ScottLu      Created.
\***************************************************************************/

void CheckProcessForeground(
    PTHREADINFO pti)
{
    PTHREADINFO ptiT;

    /*
     * Check to see if we need to move this process into foreground
     * priority.
     */
    pti->pClientInfo->cSpins = 0;
    pti->TIF_flags &= ~TIF_SPINNING;
    pti->pClientInfo->dwTIFlags = pti->TIF_flags;

    if (pti->ppi->W32PF_Flags & W32PF_FORCEBACKGROUNDPRIORITY) {
        /*
         * See if any thread of this process is spinning. If none
         * are, we can remove the force to background.
         */
        for (ptiT = pti->ppi->ptiList; ptiT != NULL; ptiT = ptiT->ptiSibling) {
            if (ptiT->TIF_flags & TIF_SPINNING)
                return;
        }

        pti->ppi->W32PF_Flags &= ~W32PF_FORCEBACKGROUNDPRIORITY;
        if (pti->ppi == gppiWantForegroundPriority) {
            SetForegroundPriority(pti, TRUE);
        }
    }
}

/***************************************************************************\
* xxxInternalGetMessage
*
* This routine is the worker for both xxxGetMessage() and xxxPeekMessage()
* and is modelled after its win3.1 counterpart. From Win3.1:
*
* Get msg from the app queue or sys queue if there is one that matches
* hwndFilter and matches msgMin/msgMax. If no messages in either queue, check
* the QS_PAINT and QS_TIMER bits, call DoPaint or DoTimer to Post the
* appropriate message to the application queue, and then Read that message.
* Otherwise, if in GetMessage, Sleep until a wake bit is set indicating there
* is something we need to do.  If in PeekMessage, return to caller. Before
* reading messages from the queues, check to see if the QS_SENDMESSAGE bit
* is set, and if so, call ReceiveMessage().
*
* New for NT5: HIWORD(flags) contains a wake mask provided by the caller.
*   This mask is passed to CalcWakeMask to be combined with the mask generated
*   from msgMin and MsgMax. The default mask includes QS_SENDMESSAGE
*   now; we won't call xxxReceiveMessages (directly) unless this bit is set;
*   however, to avoid potentail deadlocks and maintain NT4 compatibility as
*   much as possible, we fail the call if QS_SENDMESSAGE is set in fsWakeBits
*   but not requested by the caller. The same applies to QS_EVENT which we would
*   always process in NT4.
*
*
* 10-19-92 ScottLu      Created.
\***************************************************************************/

BOOL xxxInternalGetMessage(
    LPMSG lpMsg,
    HWND hwndFilter,
    UINT msgMin,
    UINT msgMax,
    UINT flags,
    BOOL fGetMessage)
{
    UINT fsWakeBits;
    UINT fsWakeMask;
    UINT fsRemoveBits;
    PTHREADINFO ptiCurrent;
    PW32PROCESS W32Process;
    PWND pwndFilter;
    BOOL fLockPwndFilter;
    TL tlpwndFilter;
    BOOL fRemove;
    BOOL fExit;
    PQ pq;
#ifdef MARKPATH
    DWORD pathTaken = 0;

#define PATHTAKEN(x)  pathTaken  |= x
#define DUMPPATHTAKEN() if (gfMarkPath) DbgPrint("xxxInternalGetMessage path:%08x\n", pathTaken)
#else
#define PATHTAKEN(x)
#define DUMPPATHTAKEN()
#endif

    CheckCritIn();
    UserAssert(IsWinEventNotifyDeferredOK());

    ptiCurrent = PtiCurrent();

    /*
     * PeekMessage accepts NULL, 0x0000FFFF, and -1 as valid HWNDs.
     * If hwndFilter is invalid we can't just return FALSE because that will
     * hose existing badly behaved apps who might attempt to dispatch
     * the random contents of pmsg.
     */
    if ((hwndFilter == (HWND)-1) || (hwndFilter == (HWND)0x0000FFFF)) {
        hwndFilter = (HWND)1;
    }

    if ((hwndFilter != NULL) && (hwndFilter != (HWND)1)) {
        if ((pwndFilter = ValidateHwnd(hwndFilter)) == NULL) {
            lpMsg->hwnd = NULL;
            lpMsg->message = WM_NULL;
            PATHTAKEN(1);
            DUMPPATHTAKEN();
            if (fGetMessage)
                return -1;
            else
                return 0;
        }

        ThreadLockAlwaysWithPti(ptiCurrent, pwndFilter, &tlpwndFilter);
        fLockPwndFilter = TRUE;

    } else {
        pwndFilter = (PWND)hwndFilter;
        fLockPwndFilter = FALSE;
    }

    /*
     * Add one to our spin count. At this end of this routine we'll check
     * to see if the spin count gets >= CSPINBACKGROUND. If so we'll put this
     * process into the background.
     */
    ptiCurrent->pClientInfo->cSpins++;

    /*
     * Check to see if the startglass is on, and if so turn it off and update.
     */
    W32Process = W32GetCurrentProcess();
    if (W32Process->W32PF_Flags & W32PF_STARTGLASS) {

        /*
         * This app is no longer in "starting" mode. Recalc when to hide
         * the app starting cursor.
         */
        W32Process->W32PF_Flags &= ~W32PF_STARTGLASS;
        /*
         * Don't need DeferWinEventNotify() - xxxDoSysExpunge below doesn't
         */
        zzzCalcStartCursorHide(NULL, 0);
    }

    /*
     * Next check to see if any .dlls need freeing in
     * the context of this client (used for windows hooks).
     */
    if (ptiCurrent->ppi->cSysExpunge != gcSysExpunge) {
        ptiCurrent->ppi->cSysExpunge = gcSysExpunge;
        if (ptiCurrent->ppi->dwhmodLibLoadedMask & gdwSysExpungeMask)
            xxxDoSysExpunge(ptiCurrent);
    }

    /*
     * Set up BOOL fRemove local variable from for ReadMessage()
     */
    fRemove = flags & PM_REMOVE;

    /*
     * Unlock the system queue if it's owned by us.
     */
    /*
     * If we're currently processing a message, unlock the input queue
     * because the sender, who is blocked, might be the owner, and in order
     * to reply, the receiver may need to read keyboard / mouse input.
     */
    /*
     * If this thread has the input queue locked and the last message removed
     * is the last message we looked at, then unlock - we're ready for anyone
     * to get the next message.
     */
    pq = ptiCurrent->pq;
    if (   (ptiCurrent->psmsCurrent != NULL)
        || (pq->ptiSysLock == ptiCurrent && pq->idSysLock == ptiCurrent->idLast)
       ) {
        CheckSysLock(1, pq, NULL);
        pq->ptiSysLock = NULL;
        PATHTAKEN(2);
    } else if (pq->ptiSysLock
                && (pq->ptiSysLock->cVisWindows == 0)
                && (PhkFirstGlobalValid(ptiCurrent, WH_JOURNALPLAYBACK) != NULL)) {
        /*
         * If the thread that has the system queue lock has no windows visible
         * (can happen if it just hid its last window), don't expect it to call
         * GetMessage() again! - unlock the system queue. --- ScottLu
         * This condition creates a hole by which a second thread attached to
         * the same queue as thread 1 can alter pq->idSysPeek during a callback
         * made by thread 1 so that thread 1 will delete the wrong message
         * (losing keystrokes - causing Shift to appear be stuck down editing a
         * Graph5 caption embedded in Word32 document #5032.  However, MSTEST
         * requires this hole, so allow it if Journal Playback is occurring
         * #8850 (yes, a hack)  Chicago also has this behavior.  --- IanJa
         */
        CheckSysLock(2, pq, NULL);
        pq->ptiSysLock = NULL;
        PATHTAKEN(3);
    }
    if (pq->ptiSysLock != ptiCurrent) {
        ptiCurrent->pcti->CTIF_flags &= ~CTIF_SYSQUEUELOCKED;
    }

    /*
     * If msgMax == 0 then msgMax = -1: that makes our range checking only
     * have to deal with msgMin < msgMax.
     */
    if (msgMax == 0)
        msgMax--;

    /*
     * Compute the QS* mask that corresponds to the message range
     *  and the wake mask filter (HIWORD(flags))
     */
    fsWakeMask = CalcWakeMask(msgMin, msgMax, HIWORD(flags));
    ptiCurrent->fsChangeBitsRemoved = 0;

    /*
     * If we can yield and one or more events were skipped,
     * set the wakebits for event
     */
    if (!(flags & PM_NOYIELD) && ptiCurrent->TIF_flags & TIF_DELAYEDEVENT) {
        ptiCurrent->pcti->fsWakeBits |= QS_EVENT;
        ptiCurrent->pcti->fsChangeBits |= QS_EVENT;
        ptiCurrent->TIF_flags &= ~TIF_DELAYEDEVENT;
        ptiCurrent->pClientInfo->dwTIFlags = ptiCurrent->TIF_flags;
    }

    while (TRUE) {

        /*
         * Restore any wake bits saved while journalling
         */
        ptiCurrent->pcti->fsWakeBits |= ptiCurrent->pcti->fsWakeBitsJournal;

        /*
         * If we need to recalc queue attachments, do it here. Do it on the
         * right desktop or else the queues will get created in the wrong
         * heap.
         */
        if (ptiCurrent->rpdesk == gpdeskRecalcQueueAttach) {
            gpdeskRecalcQueueAttach = NULL;

            if (ptiCurrent->rpdesk != NULL && !FJOURNALRECORD() && !FJOURNALPLAYBACK()) {
                /*
                 * No need to DeferWinEventNotify(): a call to
                 * xxxReceiveMessages is made just below
                 */
                zzzReattachThreads(FALSE);
                PATHTAKEN(4);
            }
        }

        /*
         * Remember what change bits we're clearing. This is important to
         * fix a bug in the input model: If an app receives a sent message
         * from within SleepThread(), then does PostMessage() (which sets
         * QS_POSTMESSAGE), then does a PeekMessage(...) for some different
         * posted message (clears QS_POSTMESSAGE in fsChangeBits), then returns
         * back into SleepThread(), it won't wake up to retrieve that newly
         * posted message because the change bits are cleared.
         *
         * What we do is remember the change bits that are being cleared.
         * Then, when we return to SleepThread(), we put these remembered
         * bits back into the change bits that also have corresponding
         * bits in the wakebits (so we don't set changebits that represent
         * input that isn't there anymore). This way, the app will retrieve
         * the newly posted message refered to earlier.
         * - scottlu
         *
         * New for NT5: Since QS_SENDMESSAGE was never set it fsWakeMask before (NT4),
         *  it was never cleared from fsChangeBits. For compatibility, we won't clear
         *  it now even if specified in fsWakeMask; hence we won't affect any one
         *  checking for QS_SENDMESSAGE in pcti->fsChangeBits.
         */
        fsRemoveBits = fsWakeMask & ~QS_SENDMESSAGE;
        ptiCurrent->fsChangeBitsRemoved |= ptiCurrent->pcti->fsChangeBits & fsRemoveBits;

        /*
         * Clear the change bits that we're looking at, in order to detect
         * incoming events that may occur the last time we checked the wake
         * bits.
         */
        ptiCurrent->pcti->fsChangeBits &= ~fsRemoveBits;

        /*
         * Check for sent messages. Check the the actual wake bits (i.e, from pcti)
         *  so we know for real.
         */
        if (ptiCurrent->pcti->fsWakeBits & fsWakeMask & QS_SENDMESSAGE) {
            xxxReceiveMessages(ptiCurrent);
        } else if (ptiCurrent->pcti->fsWakeBits & QS_SENDMESSAGE) {
            RIPMSG2(RIP_WARNING, "xxxInternalGetMessage:(1st test) sendmsgs pending. Bits:%#lx Mask:%#lx",
                        ptiCurrent->pcti->fsWakeBits, fsWakeMask);
            goto NoMessages;
        }

        /*
         * Check to see if we have any input we want.
         */
        if ((ptiCurrent->pcti->fsWakeBits & fsWakeMask) == 0) {
            PATHTAKEN(8);
            goto NoMessages;
        }
        fsWakeBits = ptiCurrent->pcti->fsWakeBits;

        /*
         * If the queue lock is != NULL (ptiSysLock) and it is this thread that
         * locked it, then go get the message from the system queue. This is
         * to prevent messages posted after a PeekMessage/no-remove from being
         * seen before the original message from the system queue. (Aldus
         * Pagemaker requires this) (bobgu 8/5/87).
         */
        if (ptiCurrent->pq->ptiSysLock == ptiCurrent &&
                (ptiCurrent->pq->QF_flags & QF_LOCKNOREMOVE)) {
            /*
             * Does the caller want mouse / keyboard?
             */
            if (fsWakeBits & fsWakeMask & (QS_INPUT | QS_EVENT)) {

                /*
                 * It should never get here during exit.
                 */
                UserAssert(gbExitInProgress == FALSE);

                if (xxxScanSysQueue(ptiCurrent, lpMsg, pwndFilter,
                        msgMin, msgMax, flags,
                        fsWakeBits & fsWakeMask & (QS_INPUT | QS_EVENT))) {

                    PATHTAKEN(0x10);
                    break;
                }
            } else if (fsWakeBits & QS_EVENT) {
                RIPMSG2(RIP_WARNING,
                    "xxxInternalGetMessage:(1st test)events pending. Bits:%#lx Mask:%#lx",
                    fsWakeBits, fsWakeMask);
                goto NoMessages;
            }
        }

        /*
         * See if there's a message in the application queue.
         */
        if (fsWakeBits & fsWakeMask & QS_POSTMESSAGE) {
            if (xxxReadPostMessage(ptiCurrent, lpMsg, pwndFilter,
                    msgMin, msgMax, fRemove)) {
                PATHTAKEN(0x20);
                break;
            }
        }

        /*
         * Time to scan the raw input queue for input. First check to see
         * if the caller wants mouse / keyboard input.
         */
        if (fsWakeBits & fsWakeMask & (QS_INPUT | QS_EVENT)) {

            /*
             * It should never get here during exit.
             */
            UserAssert(gbExitInProgress == FALSE);

            if (xxxScanSysQueue(ptiCurrent, lpMsg, pwndFilter,
                    msgMin, msgMax, flags,
                    fsWakeBits & fsWakeMask & (QS_INPUT | QS_EVENT))) {
                PATHTAKEN(0x40);
                break;
            }
        } else if (fsWakeBits & QS_EVENT) {
            RIPMSG2(RIP_WARNING, "xxxInternalGetMessage:(2nd test)events pending. Bits:%#lx Mask:%#lx",
                        fsWakeBits, fsWakeMask);
            goto NoMessages;
        }

        /*
         * Check for sent messages. Check the the actual wake bits (i.e, from pcti)
         *  so we know for real.
         */
        if (ptiCurrent->pcti->fsWakeBits & fsWakeMask & QS_SENDMESSAGE) {
            xxxReceiveMessages(ptiCurrent);
        } else if (ptiCurrent->pcti->fsWakeBits & QS_SENDMESSAGE) {
            RIPMSG2(RIP_WARNING, "xxxInternalGetMessage:(2nd test)sendmsgs pending. Bits:%#lx Mask:%#lx",
                        ptiCurrent->pcti->fsWakeBits, fsWakeMask);
            goto NoMessages;
        }

        /*
         * Get new input bits.
         */
        if ((ptiCurrent->pcti->fsWakeBits & fsWakeMask) == 0) {
            PATHTAKEN(0x80);
            goto NoMessages;
        }
        fsWakeBits = ptiCurrent->pcti->fsWakeBits;

        /*
         * Does the caller want paint messages? If so, try to find a paint.
         */
        if (fsWakeBits & fsWakeMask & QS_PAINT) {
            if (DoPaint(pwndFilter, lpMsg)) {
                PATHTAKEN(0x100);
                break;
            }
        }

        /*
         * We must yield for 16 bit apps before checking timers or an app
         * that has a fast timer could chew up all the time and never let
         * anyone else run.
         *
         * NOTE: This could cause PeekMessage() to yield TWICE, if the user
         * is filtering with a window handle. If the DoTimer() call fails
         * then we end up yielding again.
         */
        if (!(flags & PM_NOYIELD)) {
            /*
             * This is the point where windows would yield.  Here we wait to wake
             * up any threads waiting for this thread to hit "idle state".
             */
            zzzWakeInputIdle(ptiCurrent);

            /*
             * Yield and receive pending messages.
             */
            xxxUserYield(ptiCurrent);

            /*
             * Check new input buts and receive pending messages.
             */
            if (ptiCurrent->pcti->fsWakeBits & fsWakeMask & QS_SENDMESSAGE) {
                xxxReceiveMessages(ptiCurrent);
            } else if (ptiCurrent->pcti->fsWakeBits & QS_SENDMESSAGE) {
                RIPMSG2(RIP_WARNING, "xxxInternalGetMessage:(3rd test) sendmsgs pending. Bits:%#lx Mask:%#lx",
                            ptiCurrent->pcti->fsWakeBits, fsWakeMask);
                goto NoMessages;
            }

            if ((ptiCurrent->pcti->fsWakeBits & fsWakeMask) == 0) {

                PATHTAKEN(0x200);
                goto NoMessages;
            }
            fsWakeBits = ptiCurrent->pcti->fsWakeBits;
        }

        /*
         * Does the app want timer messages, and if there one pending?
         */
        if (fsWakeBits & fsWakeMask & QS_TIMER) {
            if (DoTimer(pwndFilter)) {
                /*
                 * DoTimer() posted the message into the app's queue,
                 * so start over and we'll grab it from there.
                 */
                 PATHTAKEN(0x400);
                 continue;
            }
        }

NoMessages:
        /*
         * Looks like we have no input. If we're being called from GetMessage()
         * then go to sleep until we find something.
         */
        if (!fGetMessage) {
            /*
             * This is one last check for pending sent messages. It also
             * yields. Win3.1 does this.
             */
            if (!(flags & PM_NOYIELD)) {
                /*
                 * This is the point where windows yields. Here we wait to wake
                 * up any threads waiting for this thread to hit "idle state".
                 */
                zzzWakeInputIdle(ptiCurrent);

                /*
                 * Yield and receive pending messages.
                 */
                xxxUserYield(ptiCurrent);
            }
            PATHTAKEN(0x800);
            goto FalseExit;
        }

        /*
         * This is a getmessage not a peekmessage, so sleep. When we sleep,
         * zzzWakeInputIdle() is called to wake up any apps waiting on this
         * app to go idle.
         */
        if (!xxxSleepThread(fsWakeMask, 0, TRUE))
            goto FalseExit;
    } /* while (TRUE) */

    /*
     * If we're here then we have input for this queue. Call the
     * GetMessage() hook with this input.
     */
    if (IsHooked(ptiCurrent, WHF_GETMESSAGE))
        xxxCallHook(HC_ACTION, flags, (LPARAM)lpMsg, WH_GETMESSAGE);

    /*
     * If called from PeekMessage(), return TRUE.
     */
    if (!fGetMessage) {
        PATHTAKEN(0x1000);
        goto TrueExit;
    }

    /*
     * Being called from GetMessage(): return FALSE if the message is WM_QUIT,
     * TRUE otherwise.
     */
    if (lpMsg->message == WM_QUIT) {
        PATHTAKEN(0x2000);
        goto FalseExit;
    }

    /*
     * Fall through to TrueExit...
     */

TrueExit:
    /*
     * Update timeLastRead. We use this for hung app calculations.
     */
    SET_TIME_LAST_READ(ptiCurrent);
    fExit = TRUE;
    PATHTAKEN(0x4000);
    goto Exit;

FalseExit:
    fExit = FALSE;

Exit:
    if (fLockPwndFilter)
        ThreadUnlock(&tlpwndFilter);

    /*
     * see CheckProcessBackground() comment above
     * Check to see if we need to move this process into background
     * priority.
     */
    if (ptiCurrent->pClientInfo->cSpins >= CSPINBACKGROUND) {

        ptiCurrent->pClientInfo->cSpins = 0;

        if (!(ptiCurrent->TIF_flags & TIF_SPINNING)) {

            ptiCurrent->TIF_flags |= TIF_SPINNING;
            ptiCurrent->pClientInfo->dwTIFlags = ptiCurrent->TIF_flags;

            if (!(ptiCurrent->ppi->W32PF_Flags & W32PF_FORCEBACKGROUNDPRIORITY)) {

                ptiCurrent->ppi->W32PF_Flags |= W32PF_FORCEBACKGROUNDPRIORITY;

                if (ptiCurrent->ppi == gppiWantForegroundPriority) {
                    SetForegroundPriority(ptiCurrent, FALSE);
                }
            }
        }

        /*
         * For spinning Message loops, we need to take the 16bit-thread out
         * of the scheduler temporarily so that other processes can get a chance
         * to run.  This is appearent in OLE operations where a 16bit foreground
         * thread starts an OLE activation on a 32bit process.  The 32bit process
         * gets starved of CPU while the 16bit thread spins.
         */
        if (ptiCurrent->TIF_flags & TIF_16BIT) {

            /*
             * Take the 16bit thread out of the scheduler.  This wakes any
             * other 16bit thread needing time, and takes the current thread
             * out.  We will do a brief sleep so that apps can respond in time.
             * When done, we will reschedule the thread.  The zzzWakeInputIdle()
             * should have been called in the no-messages section, so we have
             * already set the Idle-Event.
             */
            xxxSleepTask(FALSE, HEVENT_REMOVEME);

            LeaveCrit();
            ZwYieldExecution();
            CheckForClientDeath();
            EnterCrit();

            xxxDirectedYield(DY_OLDYIELD);
        }
    }

    PATHTAKEN(0x8000);
    DUMPPATHTAKEN();
    return fExit;
}
#undef PATHTAKEN
#undef DUMPPATHTAKEN

/***************************************************************************\
* xxxDispatchMessage (API)
*
* Calls the appropriate window procedure or function with pmsg.
*
* History:
* 10-25-90 DavidPe      Created.
\***************************************************************************/

LRESULT xxxDispatchMessage(
    LPMSG pmsg)
{
    LRESULT lRet;
    PWND pwnd;
    WNDPROC_PWND lpfnWndProc;
    TL tlpwnd;

    pwnd = NULL;
    if (pmsg->hwnd != NULL) {
        if ((pwnd = ValidateHwnd(pmsg->hwnd)) == NULL)
            return 0;
    }

    /*
     * If this is a synchronous-only message (takes a pointer in wParam or
     * lParam), then don't allow this message to go through since those
     * parameters have not been thunked, and are pointing into outer-space
     * (which would case exceptions to occur).
     *
     * (This api is only called in the context of a message loop, and you
     * don't get synchronous-only messages in a message loop).
     */
    if (TESTSYNCONLYMESSAGE(pmsg->message, pmsg->wParam)) {
        /*
         * Fail if 32 bit app is calling.
         */
        if (!(PtiCurrent()->TIF_flags & TIF_16BIT)) {
            RIPERR1(ERROR_MESSAGE_SYNC_ONLY, RIP_WARNING, "xxxDispatchMessage: Sync only message 0x%lX",
                    pmsg->message);
            return 0;
        }

        /*
         * For wow apps, allow it to go through (for compatibility). Change
         * the message id so our code doesn't understand the message - wow
         * will get the message and strip out this bit before dispatching
         * the message to the application.
         */
        pmsg->message |= MSGFLAG_WOW_RESERVED;
    }

    ThreadLock(pwnd, &tlpwnd);

    /*
     * Is this a timer?  If there's a proc address, call it,
     * otherwise send it to the wndproc.
     */
    if ((pmsg->message == WM_TIMER) || (pmsg->message == WM_SYSTIMER)) {
        if (pmsg->lParam != 0) {

            /*
             * System timers must be executed on the server's context.
             */
            if (pmsg->message == WM_SYSTIMER) {

                /*
                 * Verify that it's a valid timer proc. If so,
                 * don't leave the critsect to call server-side procs
                 * and pass a PWND, not HWND.
                 */
                PTIMER ptmr;
                lRet = 0;
                for (ptmr = gptmrFirst; ptmr != NULL; ptmr = ptmr->ptmrNext) {
                    if ((pmsg->lParam == (LPARAM)ptmr->pfn) && (ptmr->flags & TMRF_SYSTEM)) {
                        ptmr->pfn(pwnd, WM_SYSTIMER, (UINT)pmsg->wParam,
                                  NtGetTickCount());
                        break;
                    }
                }
                goto Exit;
            } else {
                /*
                 * WM_TIMER is the same for Unicode/ANSI.
                 */
                PTHREADINFO ptiCurrent = PtiCurrent();

                if (ptiCurrent->TIF_flags & TIF_SYSTEMTHREAD) {
                    lRet = 0;
                    goto Exit;
                }

                lRet = CallClientProcA(pwnd, WM_TIMER,
                       pmsg->wParam, NtGetTickCount(), pmsg->lParam);

                goto Exit;
            }
        }
    }

    /*
     * Check to see if pwnd is NULL AFTER the timer check.  Apps can set
     * timers with NULL hwnd's, that's totally legal.  But NULL hwnd messages
     * don't get dispatched, so check here after the timer case but before
     * dispatching - if it's NULL, just return 0.
     */
    if (pwnd == NULL) {
        lRet = 0;
        goto Exit;
    }

    /*
     * If we're dispatching a WM_PAINT message, set a flag to be used to
     * determine whether it was processed properly.
     */
    if (pmsg->message == WM_PAINT)
        SetWF(pwnd, WFPAINTNOTPROCESSED);

    /*
     * If this window's proc is meant to be executed from the server side
     * we'll just stay inside the semaphore and call it directly.  Note
     * how we don't convert the pwnd into an hwnd before calling the proc.
     */
    if (TestWF(pwnd, WFSERVERSIDEPROC)) {
        ULONG_PTR fnMessageType;

        fnMessageType = pmsg->message >= WM_USER ? (ULONG_PTR)SfnDWORD :
                (ULONG_PTR)gapfnScSendMessage[MessageTable[pmsg->message].iFunction];

        /*
         * Convert the WM_CHAR from ANSI to UNICODE if the source was ANSI
         */
        if (fnMessageType == (ULONG_PTR)SfnINWPARAMCHAR && TestWF(pwnd, WFANSIPROC)) {
            UserAssert(PtiCurrent() == GETPTI(pwnd)); // use receiver's codepage
            RtlMBMessageWParamCharToWCS(pmsg->message, &pmsg->wParam);
        }

        lRet = pwnd->lpfnWndProc(pwnd, pmsg->message, pmsg->wParam,
                pmsg->lParam);
        goto Exit;
    }

    /*
     * Cool people dereference any window structure members before they
     * leave the critsect.
     */
    lpfnWndProc = pwnd->lpfnWndProc;

    {
        /*
         * If we're dispatching the message to an ANSI wndproc we need to
         * convert the character messages from Unicode to Ansi.
         */
        if (TestWF(pwnd, WFANSIPROC)) {
            UserAssert(PtiCurrent() == GETPTI(pwnd)); // use receiver's codepage
            RtlWCSMessageWParamCharToMB(pmsg->message, &pmsg->wParam);
            lRet = CallClientProcA(pwnd, pmsg->message,
                    pmsg->wParam, pmsg->lParam, (ULONG_PTR)lpfnWndProc);
        } else {
            lRet = CallClientProcW(pwnd, pmsg->message,
                    pmsg->wParam, pmsg->lParam, (ULONG_PTR)lpfnWndProc);
        }
    }

    /*
     * If we dispatched a WM_PAINT message and it wasn't properly
     * processed, do the drawing here.
     */
    if (pmsg->message == WM_PAINT && RevalidateHwnd(pmsg->hwnd) &&
            TestWF(pwnd, WFPAINTNOTPROCESSED)) {
        //RIPMSG0(RIP_WARNING,
        //    "Missing BeginPaint or GetUpdateRect/Rgn(fErase == TRUE) in WM_PAINT");
        ClrWF(pwnd, WFWMPAINTSENT);
        xxxSimpleDoSyncPaint(pwnd);
    }

Exit:
    ThreadUnlock(&tlpwnd);
    return lRet;
}

/***************************************************************************\
* AdjustForCoalescing
*
* If message is in the coalesce message range, and it's message and hwnd
* equals the last message in the queue, then coalesce these two messages
* by simple deleting the last one.
*
* 11-12-92 ScottLu      Created.
\***************************************************************************/

void AdjustForCoalescing(
    PMLIST pml,
    HWND hwnd,
    UINT message)
{
    /*
     * First see if this message is in that range.
     */
    if (!CheckMsgFilter(message, WM_COALESCE_FIRST, WM_COALESCE_LAST) &&
            (message != WM_TIMECHANGE))
        return;

    if (pml->pqmsgWriteLast == NULL)
        return;

    if (pml->pqmsgWriteLast->msg.message != message)
        return;

    if (pml->pqmsgWriteLast->msg.hwnd != hwnd)
        return;

    /*
     * The message and hwnd are the same, so delete this message and
     * the new one will added later.
     */
    DelQEntry(pml, pml->pqmsgWriteLast);
}

/***************************************************************************\
* _PostMessage (API)
*
* Writes a message to the message queue for pwnd.  If pwnd == -1, the message
* is broadcasted to all windows.
*
* History:
* 11-06-90 DavidPe      Created.
\***************************************************************************/

BOOL _PostMessage(
    PWND pwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PQMSG pqmsg;
    BOOL fPwndUnlock;
    BOOL fRet;
    DWORD dwPostCode;
    TL tlpwnd;
    PTHREADINFO pti;

    /*
     * First check to see if this message takes DWORDs only. If it does not,
     * fail the post. Cannot allow an app to post a message with pointers or
     * handles in it - this can cause the server to fault and cause other
     * problems - such as causing apps in separate address spaces to fault.
     * (or even an app in the same address space to fault!)
     */
    if (TESTSYNCONLYMESSAGE(message, wParam)) {
        RIPERR1(ERROR_MESSAGE_SYNC_ONLY,
                RIP_WARNING,
                "Invalid parameter \"message\" (%ld) to _PostMessage",
                message);

        return FALSE;
    }

    /*
     * Is this a BroadcastMsg()?
     */
    if (pwnd == PWND_BROADCAST) {
        xxxBroadcastMessage(NULL, message, wParam, lParam, BMSG_POSTMSG, NULL);
        return TRUE;
    }

    pti = PtiCurrent();

    /*
     * Is this posting to the current thread info?
     */
    if (pwnd == NULL) {
        return _PostThreadMessage(pti, message, wParam, lParam);
    }

    fPwndUnlock = FALSE;
    if (message >= WM_DDE_FIRST && message <= WM_DDE_LAST) {
        ThreadLockAlwaysWithPti(pti, pwnd, &tlpwnd);
        dwPostCode = xxxDDETrackPostHook(&message, pwnd, wParam, &lParam, FALSE);

        if (dwPostCode != DO_POST) {
            ThreadUnlock(&tlpwnd);
            return (BOOL)dwPostCode;
        }

        fPwndUnlock = TRUE;
    }

    pti = GETPTI(pwnd);

    /*
     * Check to see if this message is in the multimedia coalescing range.
     * If so, see if it can be coalesced with the previous message.
     */
    AdjustForCoalescing(&pti->mlPost, HWq(pwnd), message);

    /*
     * Allocate a key state update event if needed.
     */
    if (message >= WM_KEYFIRST && message <= WM_KEYLAST) {
        PostUpdateKeyStateEvent(pti->pq);
    }

    /*
     * Put this message on the 'post' list.
     */
    fRet = FALSE;
    if ((pqmsg = AllocQEntry(&pti->mlPost)) != NULL) {
        /*
         * Set the QS_POSTMESSAGE bit so the thread knows it has a message.
         */
        StoreQMessage(pqmsg, pwnd, message, wParam, lParam, 0, 0, 0);
        SetWakeBit(pti, QS_POSTMESSAGE | QS_ALLPOSTMESSAGE);

        /*
         * If it's a hotkey, set the QS_HOTKEY bit since we have a separate
         * bit for those messages.
         */
        if (message == WM_HOTKEY)
            SetWakeBit(pti, QS_HOTKEY);

        fRet = TRUE;
    } else {
        RIPMSG1(RIP_WARNING, "_PostMessage: Failed to alloc Q entry: target pti=0x%p",
                pti);
    }

    /*
     * Are we posting to the thread currently reading from the input queue?
     * If so, update idSysLock with this pqmsg so that the input queue will
     * not be unlocked until this message is read.
     */
    if (pti == pti->pq->ptiSysLock)
        pti->pq->idSysLock = (ULONG_PTR)pqmsg;

    if (fPwndUnlock)
        ThreadUnlock(&tlpwnd);

    return fRet;
}
/***************************************************************************\
* _PostQuitMessage (API)
*
* Writes a message to the message queue for pwnd.  If pwnd == -1, the message
* is broadcasted to all windows.
*
* History:
* 11-06-90 DavidPe      Created.
* 05-16-91 mikeke       Changed to return BOOL
\***************************************************************************/
BOOL IPostQuitMessage(PTHREADINFO pti, int nExitCode)
{
    pti->cQuit = 1;
    pti->exitCode = nExitCode;
    SetWakeBit(pti, QS_POSTMESSAGE | QS_ALLPOSTMESSAGE);
    return TRUE;
}
BOOL _PostQuitMessage(int nExitCode)
{
    return IPostQuitMessage(PtiCurrent(), nExitCode);
}
/***************************************************************************\
* _PostThreadMessage (API)
*
* Given a thread ID, the function will post the specified message to this
* thread with pmsg->hwnd == NULL..
*
* History:
* 11-21-90 DavidPe      Created.
\***************************************************************************/

BOOL _PostThreadMessage(
    PTHREADINFO pti,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam)
{
    PQMSG       pqmsg;

    if ((pti == NULL)                                ||
        !(pti->TIF_flags & TIF_GUITHREADINITIALIZED) ||
        (pti->TIF_flags & TIF_INCLEANUP)) {

        RIPERR0(ERROR_INVALID_THREAD_ID, RIP_VERBOSE, "");
        return FALSE;
    }

    /*
     * First check to see if this message takes DWORDs only. If it does not,
     * fail the post. Cannot allow an app to post a message with pointers or
     * handles in it - this can cause the server to fault and cause other
     * problems - such as causing apps in separate address spaces to fault.
     * (or even an app in the same address space to fault!)
     */
    if (TESTSYNCONLYMESSAGE(message, wParam)) {
        RIPERR1(ERROR_MESSAGE_SYNC_ONLY,
                RIP_WARNING,
                "Invalid parameter \"message\" (%ld) to _PostThreadMessage",
                message);

        return FALSE;
    }

    /*
     * Check to see if this message is in the multimedia coalescing range.
     * If so, see if it can be coalesced with the previous message.
     */
    AdjustForCoalescing(&pti->mlPost, NULL, message);

    /*
     * Put this message on the 'post' list.
     */
    if ((pqmsg = AllocQEntry(&pti->mlPost)) == NULL) {
        RIPMSG1(RIP_WARNING, "_PostThreadMessage: Failed to alloc Q entry: Target pti=0x%p",
                pti);
        return FALSE;
    }

    /*
     * Set the QS_POSTMESSAGE bit so the thread knows it has a message.
     */
    StoreQMessage(pqmsg, NULL, message, wParam, lParam, 0, 0, 0);
    SetWakeBit(pti, QS_POSTMESSAGE | QS_ALLPOSTMESSAGE);

    /*
     * If it's a hotkey, set the QS_HOTKEY bit since we have a separate
     * bit for those messages.
     */
    if (message == WM_HOTKEY)
        SetWakeBit(pti, QS_HOTKEY);

    /*
     * Are we posting to the thread currently reading from the input queue?
     * If so, update idSysLock with this pqmsg so that the input queue will
     * not be unlocked until this message is read.
     */
    if (pti == pti->pq->ptiSysLock)
        pti->pq->idSysLock = (ULONG_PTR)pqmsg;

    return TRUE;
}


/***************************************************************************\
* _GetMessagePos (API)
*
* This API returns the cursor position when the last message was read from
* the current message queue.
*
* History:
* 11-19-90 DavidPe      Created.
\***************************************************************************/

DWORD _GetMessagePos(VOID)
{
    PTHREADINFO pti;

    pti = PtiCurrent();

    return MAKELONG((SHORT)pti->ptLast.x, (SHORT)pti->ptLast.y);
}



#ifdef SYSMODALWINDOWS
/***************************************************************************\
* _SetSysModalWindow (API)
*
* History:
* 01-25-91 DavidPe      Created stub.
\***************************************************************************/

PWND APIENTRY _SetSysModalWindow(
    PWND pwnd)
{
    pwnd;
    return NULL;
}


/***************************************************************************\
* _GetSysModalWindow (API)
*
* History:
* 01-25-91 DavidPe      Created stub.
\***************************************************************************/

PWND APIENTRY _GetSysModalWindow(VOID)
{
    return NULL;
}
#endif //LATER

/***************************************************************************\
* PostMove
*
* This routine gets called when it is detected that the QF_MOUSEMOVED bit
* is set in a particular queue.
*
* 11-03-92 ScottLu      Created.
\***************************************************************************/

VOID PostMove(
    PQ pq)
{
#ifdef REDIRECTION
    POINT pt;
#endif // REDIRECTION

    CheckCritIn();

    /*
     * set gdwMouseMoveTimeStamp to 0 after posting the move so
     * subsequent calls to SetFMouseMove doesn't use the same value
     * of gdwMouseMoveTimeStamp. Bug 74508.
     */
    if (gdwMouseMoveTimeStamp == 0) {
        gdwMouseMoveTimeStamp = NtGetTickCount();
    }

#ifdef REDIRECTION

    PopMouseMove(pq, &pt);

    PostInputMessage(pq, NULL, WM_MOUSEMOVE, 0,
            MAKELONG((SHORT)pt.x, (SHORT)pt.y),
            gdwMouseMoveTimeStamp, gdwMouseMoveExtraInfo);
#else
    PostInputMessage(pq, NULL, WM_MOUSEMOVE, 0,
            MAKELONG((SHORT)gpsi->ptCursor.x, (SHORT)gpsi->ptCursor.y),
            gdwMouseMoveTimeStamp, gdwMouseMoveExtraInfo);
#endif // REDIRECTION

    gdwMouseMoveTimeStamp = 0;

    pq->QF_flags &= ~QF_MOUSEMOVED;
}

#ifdef REDIRECTION

typedef struct tagQMOUSEMOVE {
    PQ    pq;
    POINT pt;
} QMOUSEMOVE;

#define MAX_QMOUSEMOVE  16

QMOUSEMOVE gqMouseMove[MAX_QMOUSEMOVE];

int gnLastMouseMove;

VOID PushMouseMove(
    PQ    pq,
    POINT pt)
{
    int ind;

    CheckCritIn();

    UserAssert(gnLastMouseMove < MAX_QMOUSEMOVE - 1);

    for (ind = 0; ind < gnLastMouseMove; ind++) {
        if (pq == gqMouseMove[ind].pq) {

            gqMouseMove[ind].pt = pt;
            return;
        }
    }

    gqMouseMove[gnLastMouseMove].pq = pq;
    gqMouseMove[gnLastMouseMove].pt = pt;

    gnLastMouseMove++;
}

VOID PopMouseMove(
    PQ     pq,
    POINT* ppt)
{
    int ind;

    CheckCritIn();

    for (ind = 0; ind < gnLastMouseMove; ind++) {
        if (pq == gqMouseMove[ind].pq) {
            *ppt = gqMouseMove[ind].pt;

            RtlMoveMemory(&gqMouseMove[ind],
                          &gqMouseMove[ind + 1],
                          (gnLastMouseMove - ind - 1) * sizeof(QMOUSEMOVE));

            gnLastMouseMove--;

            return;
        }
    }
    UserAssert(0);
}
#endif // REDIRECTION

/***************************************************************************\
* zzzSetFMouseMoved
*
* Send a mouse move through the system. This usually occurs when doing
* window management to be sure that the mouse shape accurately reflects
* the part of the window it is currently over (window managment may have
* changed this).
*
* 11-02-92 ScottLu      Created.
\***************************************************************************/

VOID zzzSetFMouseMoved()
{
    PWND pwnd;
    PWND pwndOldCursor;
    PQ   pq;

#ifdef REDIRECTION
    PWND  pwndStart;
    POINT ptMouse = gpsi->ptCursor;
#endif // REDIRECTION

    /*
     * Need to first figure out what queue this mouse event is in. Do NOT
     * check for mouse capture here !! Talk to scottlu.
     */
    if ((pwnd = gspwndScreenCapture) == NULL) {

#ifdef REDIRECTION
        /*
         * Call the speed hit test hook
         */
        pwndStart = xxxCallSpeedHitTestHook(&ptMouse);
#endif // REDIRECTION

        if ((pwnd = gspwndMouseOwner) == NULL) {
            if ((pwnd = gspwndInternalCapture) == NULL) {

                UserAssert(grpdeskRitInput != NULL);

#ifdef REDIRECTION
                if (pwndStart == NULL) {
                    pwndStart = grpdeskRitInput->pDeskInfo->spwnd;
                }
                pwnd = SpeedHitTest(pwndStart, ptMouse);
#else
                pwnd = SpeedHitTest(grpdeskRitInput->pDeskInfo->spwnd, gpsi->ptCursor);
#endif // REDIRECTION

            }
        }
    }

    if (pwnd == NULL)
        return;

    /*
     * This is apparently needed by the attach/unattach code for some
     * reason. I'd like to get rid of it - scottlu.
     */
    pwndOldCursor = Lock(&gspwndCursor, pwnd);

    /*
     * If we're giving a mouse move to a new queue, be sure the cursor
     * image represents what this queue thinks it should be.
     */
    pq = GETPTI(pwnd)->pq;

    /*
     * Protect pq by deferring WinEvent notification
     */
    DeferWinEventNotify();

    if (pq != gpqCursor) {
        /*
         * If the old queue had the mouse captured, let him know that
         * the mouse moved first. Need this to fix tooltips in
         * WordPerfect Office. Do the same for mouse tracking.
         */
        if (gpqCursor != NULL) {

            if (gpqCursor->spwndCapture != NULL) {
                gpqCursor->QF_flags |= QF_MOUSEMOVED;
                SetWakeBit(GETPTI(gpqCursor->spwndCapture), QS_MOUSEMOVE);

#ifdef REDIRECTION
                PushMouseMove(gpqCursor, ptMouse);
#endif // REDIRECTION

            }

            if ((pwndOldCursor != NULL) && (PtoHq(pwndOldCursor) != PtoHq(pwnd))) {
                PDESKTOP pdesk = GETPDESK(pwndOldCursor);
                if (pdesk->dwDTFlags & DF_MOUSEMOVETRK) {
                    PTHREADINFO pti = GETPTI(pdesk->spwndTrack);
                    PostEventMessage(pti, pti->pq, QEVENT_CANCELMOUSEMOVETRK,
                                     pdesk->spwndTrack, pdesk->dwDTFlags, pdesk->htEx,
                                     DF_MOUSEMOVETRK);
                    pdesk->dwDTFlags &= ~DF_MOUSEMOVETRK;
                }
            }
        }

        /*
         * First re-assign gpqCursor so any zzzSetCursor() calls
         * will only take effect if done by the thread that
         * owns the window the mouse is currently over.
         */
        gpqCursor = pq;

        /*
         * Call zzzUpdateCursorImage() so the new gpqCursor's
         * notion of the current cursor is represented.
         */
        zzzUpdateCursorImage();

    }

    /*
     * Set the mouse moved bit for this queue so we know later to post
     * a move message to this queue.
     */
    pq->QF_flags |= QF_MOUSEMOVED;

#ifdef REDIRECTION
    PushMouseMove(pq, ptMouse);
#endif // REDIRECTION


    /*
     * Reassign mouse input to this thread - this indicates which thread
     * to wake up when new input comes in.
     */
    pq->ptiMouse = GETPTI(pwnd);

    /*
     * Wake some thread within this queue to process this mouse event.
     */
    WakeSomeone(pq, WM_MOUSEMOVE, NULL);

    /*
     * We're possibly generating a fake mouse move message - it has no
     * extra info associated with it - so 0 it out.
     */
    gdwMouseMoveExtraInfo = 0;

    zzzEndDeferWinEventNotify();
}

/***************************************************************************\
* CancelForegroundActivate
*
* This routine cancels the foreground activate that we allow apps starting
* up to have. This means that if you make a request to start an app,
* if this routine is called before the app becomes foreground, it won't
* become foreground. This routine gets called if the user down clicks or
* makes a keydown event, with the idea being that if the user did this,
* the user is using some other app and doesn't want the newly starting
* application to appear on top and force itself into the foreground.
*
* 09-15-92 ScottLu      Created.
\***************************************************************************/

void CancelForegroundActivate()
{
    PPROCESSINFO ppiT;

    if (TEST_PUDF(PUDF_ALLOWFOREGROUNDACTIVATE)) {

        for (ppiT = gppiStarting; ppiT != NULL; ppiT = ppiT->ppiNext) {
            /*
             * Don't cancel activation if the app is being debugged - if
             * the debugger stops the application before it has created and
             * activated its first window, the app will come up behind all
             * others - not what you want when being debugged.
             */
            if (!ppiT->Process->DebugPort) {
                ppiT->W32PF_Flags &= ~W32PF_ALLOWFOREGROUNDACTIVATE;
                TAGMSG1(DBGTAG_FOREGROUND, "CancelForegroundActivate clear W32PF %#p", ppiT);
            }
        }

        CLEAR_PUDF(PUDF_ALLOWFOREGROUNDACTIVATE);
        TAGMSG0(DBGTAG_FOREGROUND, "CancelForegroundActivate clear PUDF");
    }
}

/***************************************************************************\
* RestoreForegroundActivate
*
* This routine re-enables an app's right to foreground activate (activate and
* come on top) if it is starting up. This is called when we minimize or when
* the last window of a thread goes away, for example.
*
* 01-26-93 ScottLu      Created.
\***************************************************************************/

void RestoreForegroundActivate()
{
    PPROCESSINFO ppiT;

    for (ppiT = gppiStarting; ppiT != NULL; ppiT = ppiT->ppiNext) {
        if (ppiT->W32PF_Flags & W32PF_APPSTARTING) {
            ppiT->W32PF_Flags |= W32PF_ALLOWFOREGROUNDACTIVATE;
            TAGMSG1(DBGTAG_FOREGROUND, "RestoreForegroundActivate set W32PF %#p", ppiT);
            SET_PUDF(PUDF_ALLOWFOREGROUNDACTIVATE);
            TAGMSG0(DBGTAG_FOREGROUND, "RestoreForegroundActivate set PUDF");
        }
    }
}


/***************************************************************************\
* PostInputMessage
*
* Puts a message on the 'input' linked-list of message for the specified
* queue.
*
* History:
* 10-25-90 DavidPe      Created.
* 01-21-92 DavidPe      Rewrote to deal with OOM errors gracefully.
\***************************************************************************/

void PostInputMessage(
    PQ    pq,
    PWND  pwnd,
    UINT  message,
    WPARAM wParam,
    LPARAM lParam,
    DWORD time,
    ULONG_PTR dwExtraInfo)
{
    PQMSG pqmsgInput, pqmsgPrev;
    short sWheelDelta;

    /*
     * Grab the last written message before we start allocing new ones,
     * so we're sure to point to the correct message.
     */
    pqmsgPrev = pq->mlInput.pqmsgWriteLast;

    /*
     * Allocate a key state update event if needed.
     */
    if (pq->QF_flags & QF_UPDATEKEYSTATE) {
        PostUpdateKeyStateEvent(pq);
    }

    /*
     * We want to coalesce sequential WM_MOUSEMOVE and WM_MOUSEWHEEL.
     * WM_MOUSEMOVEs are coalesced by just storing the most recent
     * event over the last one.
     * WM_MOUSEWHEELs also add up the wheel rolls.
     */
    if (pqmsgPrev != NULL &&
        pqmsgPrev->msg.message == message &&
        (message == WM_MOUSEMOVE || message == WM_MOUSEWHEEL)) {

        if (message == WM_MOUSEWHEEL) {
            sWheelDelta = (short)HIWORD(wParam) + (short)HIWORD(pqmsgPrev->msg.wParam);

#if 0
            /*
             * LATER: We can't remove a wheel message with zero delta
             * unless we know it hasn't been peeked. Ideally,
             * we would check idsyspeek for this, but we're too close
             * to ship and idsyspeek is too fragile. Consider also
             * checking to see if mouse move messages have been peeked.
             */

            if (sWheelDelta == 0) {
                if ((PQMSG)pq->idSysPeek == pqmsgPrev) {
                    RIPMSG0(RIP_VERBOSE,
                            "Coalescing of mouse wheel messages causing "
                            "idSysPeek to be reset to 0");

                    pq->idSysPeek = 0;
                }

                DelQEntry(&pq->mlInput, pqmsgPrev);
                return;
            }
#endif

            wParam = MAKEWPARAM(LOWORD(wParam), sWheelDelta);
        }

        StoreQMessage(pqmsgPrev, pwnd, message, wParam, lParam, time, 0, dwExtraInfo);
        WakeSomeone(pq, message, pqmsgPrev);

        return;
    }

    /*
     * Fill in pqmsgInput.
     */
    pqmsgInput = AllocQEntry(&pq->mlInput);
    if (pqmsgInput != NULL) {
        StoreQMessage(pqmsgInput, pwnd, message, wParam, lParam, time, 0, dwExtraInfo);
        WakeSomeone(pq, message, pqmsgInput);
    }
}

/***************************************************************************\
* WakeSomeone
*
* Figures out which thread to wake up based on the queue and message.
* If the queue pointer is NULL, figures out a likely queue.
*
* 10-23-92 ScottLu      Created.
\***************************************************************************/

void WakeSomeone(
    PQ pq,
    UINT message,
    PQMSG pqmsg)
{
    BOOL fSetLastWoken = FALSE;
    PTHREADINFO ptiT;

    /*
     * Set the appropriate wakebits for this queue.
     */
    ptiT = NULL;
    switch (message) {

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        /*
         * Don't change input ownership if the user is holding down
         *  a modifier key. When doing a ctrl-drag operation for example,
         *  the ctrl key must be down when the user drops the object (ie, mouse up).
         *  On mouse up the RIT gives input ownership to the target; but since
         *  ctrl is down, on the next repeat key we used to give input ownership
         *  to the focus window (usually the drag source). Hence the target
         *  would lose owenerhip and couldn't take the foreground.
         */
        if (pqmsg != NULL) {
            switch (pqmsg->msg.wParam) {
                case VK_SHIFT:
                case VK_CONTROL:
                case VK_MENU:
                    if (TestKeyStateDown(pq, pqmsg->msg.wParam)) {
                        break;
                    }
                    /* Fall through */

                default:
                    fSetLastWoken = TRUE;
                    break;
            }
        } else {
            fSetLastWoken = TRUE;
        }
        /* fall through */

    case WM_SYSCHAR:
    case WM_CHAR:
        /* Freelance graphics seems to pass in WM_SYSCHARs and WM_CHARs into
         * the journal playback hook, so we need to set an input bit for
         * this case since that is what win3.1 does. VB2 "learning" demo does
         * the same, as does Excel intro.
         *
         * On win3.1, the WM_CHAR would by default set the QS_MOUSEBUTTON bit.
         * On NT, the WM_CHAR sets the QS_KEY bit. This is because
         * ScanSysQueue() calls TransferWakeBit() with the QS_KEY bit when
         * a WM_CHAR message is passed in. By using the QS_KEY bit on NT,
         * we're more compatible with what win3.1 wants to be.
         *
         * This fixes a case where the mouse was over progman, the WM_CHAR
         * would come in via journal playback, wakesomeone would be called,
         * and set the mouse bit in progman. Progman would then get into
         * ScanSysQueue(), callback the journal playback hook, get the WM_CHAR,
         * and do it again, looping. This caught VB2 in a loop.
         */

        CancelForegroundActivate();

        /* fall through */

    case WM_KEYUP:
    case WM_SYSKEYUP:
    case WM_MOUSEWHEEL:
        /*
         * Win3.1 first looks at what thread has the active status. This
         * means that we don't depend on the thread owning ptiKeyboard
         * to wake up and process this key in order to give it to the
         * active window, which is potentially newly active. Case in
         * point: excel bringing up CBT, cbt has an error, brings up
         * a message box: since excel is filtering for CBT messages only,
         * ptiKeyboard never gets reassigned to CBT so CBT doesn't get
         * any key messages and appears hung.
         */
        ptiT = pq->ptiKeyboard;
        if (pq->spwndActive != NULL)
            ptiT = GETPTI(pq->spwndActive);

        SetWakeBit(ptiT, message == WM_MOUSEWHEEL ? QS_MOUSEBUTTON : QS_KEY);
        break;

    case WM_MOUSEMOVE:
        /*
         * Make sure we wake up the thread with the capture, if there is
         * one. This fixes PC Tools screen capture program, which sets
         * capture and then loops trying to remove messages from the
         * queue.
         */
        if (pq->spwndCapture != NULL)
            ptiT = GETPTI(pq->spwndCapture);
        else
            ptiT = pq->ptiMouse;
        SetWakeBit(ptiT, QS_MOUSEMOVE);
        break;


    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONDBLCLK:
            fSetLastWoken = TRUE;

        /* fall through */

    default:
        /*
         * The default case in Win3.1 for this is QS_MOUSEBUTTON.
         */

        CancelForegroundActivate();

        /* fall through */

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_XBUTTONUP:
        /*
         * Make sure we wake up the thread with the capture, if there is
         * one. This fixes PC Tools screen capture program, which sets
         * capture and then loops trying to remove messages from the
         * queue.
         */
        if (pq->spwndCapture != NULL &&
                message >= WM_MOUSEFIRST && message <= WM_MOUSELAST)
            ptiT = GETPTI(pq->spwndCapture);
        else
            ptiT = pq->ptiMouse;
        SetWakeBit(ptiT, QS_MOUSEBUTTON);
        break;
    }

    /*
     * If a messaged was passed in, remember in it who we woke up for this
     * message. We do this so each message is ownership marked. This way
     * we can merge/unmerge message streams when zzzAttachThreadInput() is
     * called.
     */
    if (ptiT != NULL) {
        if (pqmsg != NULL) {

            StoreQMessagePti(pqmsg, ptiT);

            UserAssert(!(ptiT->TIF_flags & TIF_INCLEANUP));
        }

        /*
         * Remember who got the last key/click down.
         */
        if (fSetLastWoken) {
            glinp.ptiLastWoken = ptiT;
        }
    }

}


/***************************************************************************\
* PostUpdateKeyStateEvent
*
* This routine posts an event which updates the local thread's keystate
* table. This makes sure the thread's key state is always up-to-date.
*
* An example is: control-esc from cmd to taskman.
* Control goes to cmd, but taskman is activated. Control state is still down
* in cmd - switch back to cmd, start typing, nothing appears because it thinks
* the control state is still down.
*
* As events go into a particular queue (queue A), the async key state table is
* updated. As long as transition events are put into queue A, the key
* state at the logical "end of the queue" is up-to-date with the async key
* state. As soon as the user posts transition events (up/down msgs) into queue
* B, the queue A's end-of-queue key state is out of date with the user. If
* the user then again added messages to queue A, when those messages are read
* the thread specific key state would not be updated correctly unless we
* did some synchronization (which this routine helps to do).
*
* As soon as transition events change queues, we go mark all the other queues
* with the QF_UPDATEKEYSTATE flag. Before any input event is posted into
* a queue, this flag is checked, and if set, this routine is called. This
* routine makes a copy of the async key state, and a copy of the bits
* representing the keys that have changed since the last update (we need to
* keep track of which keys have changed so that any state set by the
* app with SetKeyboardState() doesn't get wiped out). We take this data
* and post a new event of the type QEVENT_UPDATEKEYSTATE, which points to this
* key state and transition information. When this message is read out of the
* queue, this key state copy is copied into the thread specific key state
* table for those keys that have changed, and the copy is deallocated.
*
* This ensures all queues are input-synchronized with key transitions no matter
* where they occur. The side affect of this is that an application may suddenly
* have a key be up without seeing the up message. If this causes any problems
* we may have to generate false transition messages (this could have more nasty
* side affects as well, so it needs to be considered closely before being
* implemented.)
*
* 06-07-91 ScottLu      Created.
\***************************************************************************/

void PostUpdateKeyStateEvent(
    PQ pq)
{
    BYTE *pb;
    PQMSG pqmsg;

    if (!(pq->QF_flags & QF_UPDATEKEYSTATE))
        return;

    /*
     * Exclude the RIT - it's queue is never read, so don't waste memory
     */
    if (pq->ptiKeyboard == gptiRit) {
        return;
    }

    /*
     * If there's no mousebutton or keystroke input pending, process the
     * UpdateKeyState Event now: thus saving memory allocation and giving
     * applications the correct KeyState immediately.
     * NOTE: There may be event/activation msgs in pq->mlInput that don't
     * affect keystate, so I'd like to just test QS_KEY | QS_MOUSEBUTTON
     * specifically, instead of the cMsgss.  However, sometimes there are
     * keystroke or mousebutton msgs in the q without those bits set! - IanJa
     */
    if (pq->mlInput.cMsgs == 0) {
        ProcessUpdateKeyStateEvent(pq, gafAsyncKeyState, pq->afKeyRecentDown);
        goto SyncQueue;
    }
#if DBG
    else if ((!pq->ptiKeyboard || !(pq->ptiKeyboard->pcti->fsWakeBits & QS_KEY)) &&
             (!pq->ptiMouse    || !(pq->ptiMouse->pcti->fsWakeBits & QS_MOUSEBUTTON))) {
        /*
         * See if there are any key or mousebutton messages that aren't
         * indicated by QS_KEY or QS_MOUSEBUTTON bits.
         */
        PQMSG pqmsgT;
        for (pqmsgT = pq->mlInput.pqmsgRead; pqmsgT; pqmsgT = pqmsgT->pqmsgNext) {
            if (pqmsgT->msg.message >= WM_KEYFIRST && pqmsgT->msg.message <= WM_KEYLAST) {
                TAGMSG1(DBGTAG_InputWithoutQS,
                        "PostUpdateKeyStateEvent() pushing in front of a keystroke: Q %#p", pq);
            } else if (pqmsgT->msg.message >= WM_LBUTTONDOWN && pqmsgT->msg.message <= WM_XBUTTONDBLCLK) {
                TAGMSG1(DBGTAG_InputWithoutQS,
                        "PostUpdateKeyStateEvent() pushing in front of a mousebutton: Q %#p", pq);
            }
        }
    }
#endif

    UserAssert(pq->mlInput.pqmsgWriteLast != NULL);

    /*
     * If the last input message is an UPDATEKEYSTATE event, coalesce with it.
     * (Prevents big memory leaks on apps that don't read input messages)
     */
    pqmsg = pq->mlInput.pqmsgWriteLast;
    if (pqmsg->dwQEvent == QEVENT_UPDATEKEYSTATE) {
        int i;
        DWORD *pdw;

        pb = (PBYTE)(pqmsg->msg.wParam);
        pdw = (DWORD *) (pb + CBKEYSTATE);

        /*
         * Copy in the new key state
         */
        RtlCopyMemory(pb, gafAsyncKeyState, CBKEYSTATE);

        /*
         * Or in the recent key down state (DWORD at a time)
         */
#if (CBKEYSTATERECENTDOWN % 4) != 0
#error "CBKEYSTATERECENTDOWN assumed to be an integral number of DWORDs"
#endif
        for (i = 0; i < CBKEYSTATERECENTDOWN / sizeof(*pdw); i++) {
            *pdw++ |= ((DWORD *)(pq->afKeyRecentDown))[i];
        }

        /*
         * Set QS_EVENTSET as in PostEventMessage, although this is
         * usually, but not always already set
         */
        SetWakeBit(pq->ptiKeyboard, QS_EVENTSET);
        goto SyncQueue;
    }

    /*
     * Make a copy of the async key state buffer, point to it, and add an
     * event to the end of the input queue.
     */
    if ((pb = UserAllocPool(KEYSTATESIZE, TAG_KBDSTATE)) == NULL) {
        return;
    }

    RtlCopyMemory(pb, gafAsyncKeyState, CBKEYSTATE);
    RtlCopyMemory(pb + CBKEYSTATE, pq->afKeyRecentDown, CBKEYSTATERECENTDOWN);

    if (!PostEventMessage(pq->ptiKeyboard, pq, QEVENT_UPDATEKEYSTATE,
                          NULL, 0 , (WPARAM)pb, 0)) {
        UserFreePool(pb);
        return;
    }

    /*
     * The key state of the queue is input-synchronized with the user.  Erase
     * all 'recent down' flags.
     */
SyncQueue:
    RtlZeroMemory(pq->afKeyRecentDown, CBKEYSTATERECENTDOWN);
    pq->QF_flags &= ~QF_UPDATEKEYSTATE;
}


/***************************************************************************\
* ProcessUpdateKeyStateEvent
*
* This is part two of the above routine, called when the QEVENT_UPDATEKEYSTATE
* message is read out of the input queue.
*
* 06-07-91 ScottLu      Created.
\***************************************************************************/

void ProcessUpdateKeyStateEvent(
    PQ pq,
    CONST PBYTE pbKeyState,
    CONST PBYTE pbRecentDown)
{
    int i, j;
    BYTE *pbChange;
    int vk;

    pbChange = pbRecentDown;
    for (i = 0; i < CBKEYSTATERECENTDOWN; i++, pbChange++) {

        /*
         * Find some keys that have changed.
         */
        if (*pbChange == 0)
            continue;

        /*
         * Some keys have changed in this byte.  find out which key it is.
         */
        for (j = 0; j < 8; j++) {

            /*
             * Convert our counts to a virtual key index and check to see
             * if this key has changed.
             */
            vk = (i << 3) + j;
            if (!TestKeyRecentDownBit(pbRecentDown, vk))
                continue;

            /*
             * This key has changed.  Update it's state in the thread key
             * state table.
             */

            if (TestKeyDownBit(pbKeyState, vk)) {
                SetKeyStateDown(pq, vk);
            } else {
                ClearKeyStateDown(pq, vk);
            }

            if (TestKeyToggleBit(pbKeyState, vk)) {
                SetKeyStateToggle(pq, vk);
            } else {
                ClearKeyStateToggle(pq, vk);
            }
        }
    }

    /*
     * Update the key cache index.
     */
    gpsi->dwKeyCache++;

    /*
     * All updated.  Free the key state table if it was posted as an Event Message
     */
    if (pbKeyState != gafAsyncKeyState) {
        UserFreePool(pbKeyState);
    }
}


/***************************************************************************\
* PostEventMessage
*
*
* History:
* 03-04-91 DavidPe      Created.
\***************************************************************************/

BOOL PostEventMessage(
    PTHREADINFO pti,
    PQ    pq,
    DWORD dwQEvent,
    PWND  pwnd,
    UINT  message,
    WPARAM wParam,
    LPARAM lParam)
{
    PQMSG pqmsgEvent;

    CheckCritIn();

    /*
     * If the thread is in cleanup, then it's possible the queue has
     * already been removed for this thread.  If this is the case, then
     * we should fail to post the event to a dying thread.
     */
    if (pti && (pti->TIF_flags & TIF_INCLEANUP))
        return FALSE;

    if ((pqmsgEvent = AllocQEntry(&pq->mlInput)) == NULL)
        return FALSE;

    StoreQMessage(pqmsgEvent, pwnd, message, wParam, lParam, 0, dwQEvent, 0);

    StoreQMessagePti(pqmsgEvent, pti);

    /*
     * Let this thread know it has an event message to process.
     */
    if (pti == NULL) {
        UserAssert(pti);
        SetWakeBit(pq->ptiMouse, QS_EVENTSET);
        SetWakeBit(pq->ptiKeyboard, QS_EVENTSET);
    } else {
        SetWakeBit(pti, QS_EVENTSET);
    }

    return TRUE;
}

/***************************************************************************\
* CheckOnTop
*
* Usually windows come to the top and activate all at once. Occasionally, a
* starting app will create a window, pause for awhile, then make itself
* visible. During that pause, if the user clicks down, the window won't be
* allowed to activate (because of our foreground activation model). But this
* still leaves the new window on top of the active window. When this click
* happens, we get here: if this window is active and is not on top, bring it
* to the top.
*
* Case in point: start winquote, click down. The window
* you clicked on is active, but winquote is on top.
*
* This rarely does anything because 99.99% of the time the active
* window is already where it should be - on top. Note that
* CalcForegroundInsertAfter() takes into account owner-based zordering.
*
*
* NOTE: the following was the original function written.  However, this
*       function has been disabled for now.  in WinWord and Excel especially,
*       this tends to cause savebits to be blown-away on mouse-activation of
*       its dialog-boxes.  This could be a problem with GW_HWNDPREV and/or
*       CalcForground not being the same, which causes a SetWindowPos to be
*       called, resulting in the freeing of the SPB.  This also solves a
*       problem with the ComboBoxes in MsMoney hiding/freeing the dropdown
*       listbox on activation as well.
*
* We need this for ActiveWindowTracking support. xxxBringWindowToTop used to be a call
*  to SetWindowPos but now it's gone.
*
* It returns TRUE if the window was brought the top; if no z-order change
*  took place, it returns FALSE.
*
* 05-20-93 ScottLu      Created.
* 10-17-94 ChrisWil     Made into stub-macro.
* 05-30-96 GerardoB     Brought it back to live for AWT
\***************************************************************************/
BOOL CheckOnTop(PTHREADINFO pti, PWND pwndTop, UINT message)
{
    if (pwndTop != pti->pq->spwndActive)
        return FALSE;

    switch (message) {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
            if (   TestWF(pwndTop, WEFTOPMOST)
                    || (_GetWindow(pwndTop, GW_HWNDPREV) != CalcForegroundInsertAfter(pwndTop))) {

                 return xxxSetWindowPos(pwndTop, PWND_TOP, 0, 0, 0, 0,
                        SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
             }
             break;
    }

    return FALSE;
}


#define MA_PASSTHRU     0
#define MA_SKIP         1
#define MA_REHITTEST    2

/***************************************************************************\
* zzzActiveCursorTracking
*
* If active window tracking is enabled, activation follows
*  the mouse. If the mouse is NOT on the active window
* (i.e., it was activated by a keyboard operation),
*  activation will change as soon as the mouse moves.
* So we have to make sure the mouse is on the active window.
*
* History
* 12/07/96  GerardoB  Created
\***************************************************************************/
void zzzActiveCursorTracking (PWND pwnd)
{
    BOOL fVisible;
    POINT pt;

    /*
     * If the last input event wasn't from the keyboard, bail
     * The user is probably moving the mouse.
     */
    if (!(glinp.dwFlags & LINP_KEYBOARD)) {
        return;
    }
    /*
     * If we're already there, bail.
     */
    if (PtInRect((LPRECT)&pwnd->rcWindow, gptCursorAsync)) {
        return;
    }
    /*
     * If the window the mouse is on is not "active-trackable", then
     *  we can leave the mouse right where it is
     */
     if ((gspwndCursor != NULL) && (GetActiveTrackPwnd(gspwndCursor, NULL) == NULL)) {
         return;
     }
     /*
      * If this window doesn't have a point visible in the screen, bail
      */
     pt.x = pwnd->rcWindow.left + ((pwnd->rcWindow.right  - pwnd->rcWindow.left) / 2);
     pt.y = pwnd->rcWindow.top  + ((pwnd->rcWindow.bottom - pwnd->rcWindow.top)  / 2);
     BoundCursor(&pt);
     if (!PtInRect((LPRECT)&pwnd->rcWindow, pt)) {
         return;
     }
    /*
     * We need to make sure that this window is marked as visible or someone
     *  else will be waken up to update the cursor (and might
     *  activate itself because of the active tracking).
     *
     * Later5.0 GerardoB: If the window is still not visible when
     *  it wakes up, then we're out of luck.
     */
    fVisible = TestWF(pwnd, WFVISIBLE);
    if (!fVisible) {
        SetVisible(pwnd, SV_SET);
    }

    /*
     * Move the cursor to the center of this window
     */
    zzzInternalSetCursorPos(pt.x, pt.y);
    /*
     * Restore visible bit.
     */
    if (!fVisible) {
        SetVisible(pwnd, SV_UNSET);
    }
}
/***************************************************************************\
* GetActiveTrackPwnd
*
* History
* 12/07/96  GerardoB  Extracted from xxxActiveWindowTracking.
\***************************************************************************/
PWND GetActiveTrackPwnd(PWND pwnd, Q **ppq)
{
    PWND pwndActivate;
    Q *pq;

    CheckCritIn();
    pwndActivate = pwnd;

    /*
     * Find the top parent
     */
    while (TestwndChild(pwndActivate)) {
        pwndActivate = pwndActivate->spwndParent;
    }

    /*
     * If disabled, get a enabled popup owned by it.
     */
    if (TestWF(pwndActivate, WFDISABLED)) {
        /*
         * This is what we do elsewhere when someone clicks on a
         *  disabled non-active window. It might be cheaper to check
         *  pwnd->spwndLastActive first (we might need to walk up
         *  the owner chain though, as this is where we set spwndLastAcitve
         *  when activating a new window. see xxxActivateThisWindow).
         * But let's do the same here; this should be fixed/improved
         *  in DWP_GetEnabledPopup anyway. There might be a reason
         *  why we don't grab spwndLastActive if OK.... perhaps it has
         *  something to do with nested owner windows
         */
         pwndActivate = DWP_GetEnabledPopup(pwndActivate);
    }

    /*
     * Bail if we didn't find a visible window
     */
    if ((pwndActivate == NULL) || !TestWF(pwndActivate, WFVISIBLE)) {
        return NULL;
    }

    /*
     * If already active in the foreground queue, nothing to do
     * Don't activate the modeless menu notification window (it would
     *  dismiss the menu)
     */
    pq = GETPTI(pwndActivate)->pq;
    if ((pq == gpqForeground)
            && ((pwndActivate == pq->spwndActive)
                || IsModelessMenuNotificationWindow(pwndActivate))) {

        return NULL;
    }

    /*
     * Don't activate the shell window.
     */
    if (pwndActivate == pwndActivate->head.rpdesk->pDeskInfo->spwndShell) {
        return NULL;
    }

    /*
     * Return the queue if requested
     */
    if (ppq != NULL) {
        *ppq = pq;
    }

    return pwndActivate;
}
/***************************************************************************\
* xxxActivateWindowTracking
*
* Activates a window without z-ordering it to the top
*
* 06/05/96  GerardoB  Created
\***************************************************************************/
int xxxActiveWindowTracking(
    PWND pwnd,
    UINT uMsg,
    int iHitTest)
{

    BOOL fSuccess;
    int iRet;
    PWND pwndActivate;
    Q *pq;
    TL tlpwndActivate;

    CheckLock(pwnd);
    UserAssert(TestUP(ACTIVEWINDOWTRACKING));

    /*
     * If the mouse hasn't been long enough on this queue, bail.
     */
    pq = GETPTI(pwnd)->pq;
    if (!(pq->QF_flags & QF_ACTIVEWNDTRACKING)) {
        return MA_PASSTHRU;
    }
    pq->QF_flags &= ~QF_ACTIVEWNDTRACKING;

    /*
     * If the foreground is locked, bail
     */
    if (IsForegroundLocked()) {
        return MA_PASSTHRU;
    }

    /*
     * Get the window we need to activate. If none, bail.
     */
    pwndActivate = GetActiveTrackPwnd(pwnd, &pq);
    if (pwndActivate == NULL) {
        return MA_PASSTHRU;
    }

    /*
     * Lock if needed because we're about to callback
     */
    if (pwnd != pwndActivate) {
        ThreadLockAlways(pwndActivate, &tlpwndActivate);
    }

    /*
     * Let's ask if it's OK to do this
     *
     * This message is supposed to go to the window the mouse is on.
     * This could be a child window which might return MA_NOACTIVATE*.
     * For mouse clicks (which is what we want to emulate here)
     *  xxxButtonEvent calls xxxSetForegroundWindow2 so their
     *  pwndActivate gets brought to the foreground regardless.
     * So we send the message to pwndActivate instead.
     */
    iRet = (int)xxxSendMessage(pwndActivate, WM_MOUSEACTIVATE,
            (WPARAM)(HWq(pwndActivate)), MAKELONG((SHORT)iHitTest, uMsg));


    switch (iRet) {
        case MA_ACTIVATE:
        case MA_ACTIVATEANDEAT:
            if (pq == gpqForeground) {
                fSuccess = xxxActivateThisWindow(pwndActivate, 0,
                        (TestUP(ACTIVEWNDTRKZORDER) ? 0 : ATW_NOZORDER));
            } else {
                fSuccess = xxxSetForegroundWindow2(pwndActivate, NULL,
                        SFW_SWITCH | (TestUP(ACTIVEWNDTRKZORDER) ? 0 : SFW_NOZORDER));
            }

            /*
             * Eat the message if activation failed.
             */
            if (!fSuccess) {
                iRet = MA_SKIP;
            } else if (iRet == MA_ACTIVATEANDEAT) {
               iRet = MA_SKIP;
            }
            break;

        case MA_NOACTIVATEANDEAT:
            iRet = MA_SKIP;
            break;


        case MA_NOACTIVATE:
        default:
            iRet = MA_PASSTHRU;
            break;
    }

    if (pwnd != pwndActivate) {
        ThreadUnlock(&tlpwndActivate);
    }

    return iRet;

}
/***************************************************************************\
* xxxMouseActivate
*
* This is where activation due to mouse clicks occurs.
*
* IMPLEMENTATION:
*     The message is sent to the specified window.  In xxxDefWindowProc, the
*     message is sent to the window's parent.  The receiving window may
*            a) process the message,
*            b) skip the message totally, or
*            c) re-hit test message
*
*     A WM_SETCURSOR message is also sent through the system to set the cursor.
*
* History:
* 11-22-90 DavidPe      Ported.
\***************************************************************************/

int xxxMouseActivate(
    PTHREADINFO pti,
    PWND pwnd,
    UINT message,
    WPARAM wParam,
    LPPOINT lppt,
    int ht)
{
    UINT x, y;
    PWND pwndTop;
    int result;
    TL tlpwndTop;
    BOOL fSend;

    CheckLock(pwnd);

    UserAssert(_GETPDESK(pwnd) != NULL);

    /*
     * No mouse activation if the mouse is captured. Must check for the capture
     * ONLY here. 123W depends on it - create a graph, select Rearrange..
     * flip horizontal, click outside the graph. If this code checks for
     * anything beside just capture, 123w will get the below messages and
     * get confused.
     */
    if (pti->pq->spwndCapture != NULL) {
        return MA_PASSTHRU;
    }

    result = MA_PASSTHRU;

    pwndTop = pwnd;
    ThreadLockWithPti(pti, pwndTop, &tlpwndTop);

        /*
         * B#1404
         * Don't send WM_PARENTNOTIFY messages if the child has
         * WS_EX_NOPARENTNOTIFY style.
         *
         * Unfortunately, this breaks people who create controls in
         * MDI children, like WinMail.  They don't get WM_PARENTNOTIFY
         * messages, which don't get passed to DefMDIChildProc(), which
         * then can't update the active MDI child.  Grrr.
         */

    fSend = (!TestWF(pwnd, WFWIN40COMPAT) || !TestWF(pwnd, WEFNOPARENTNOTIFY));

    /*
     * If it's a buttondown event, send WM_PARENTNOTIFY.
     */
    switch (message) {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_XBUTTONDOWN:
        while (TestwndChild(pwndTop)) {
            pwndTop = pwndTop->spwndParent;

            if (fSend) {
                ThreadUnlock(&tlpwndTop);
                ThreadLockWithPti(pti, pwndTop, &tlpwndTop);
                x = (UINT)(lppt->x - pwndTop->rcClient.left);
                y = (UINT)(lppt->y - pwndTop->rcClient.top);

                /* Get the xbutton from the hiword of wParam */
                UserAssert(message == WM_XBUTTONDOWN || HIWORD(wParam) == 0);
                UserAssert(LOWORD(wParam) == 0);
                xxxSendMessage(pwndTop, WM_PARENTNOTIFY, (WPARAM)(message | wParam), MAKELPARAM(x, y));
            }
        }

        if (!fSend) {
            ThreadUnlock(&tlpwndTop);
            ThreadLockAlwaysWithPti(pti, pwndTop, &tlpwndTop);
        }

        /*
         * NOTE: We break out of this loop with pwndTop locked.
         */
        break;
    }

    /*
     * The mouse was moved onto this window: make it foreground
     */
    if (TestUP(ACTIVEWINDOWTRACKING) && (message == WM_MOUSEMOVE)) {
        result = xxxActiveWindowTracking(pwnd, WM_MOUSEMOVE, ht);
    }

    /*
     * Are we hitting an inactive top-level window WHICH ISN'T THE DESKTOP(!)?
     *
     * craigc 7-14-89 hitting either inactive top level or any child window,
     * to be compatible with 2.X.  Apps apparently needs this message.
     */
    else if ((pti->pq->spwndActive != pwnd || pti->pq->QF_flags & QF_EVENTDEACTIVATEREMOVED) &&
            (pwndTop != PWNDDESKTOP(pwndTop))) {
        switch (message) {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:

            /*
             * Send the MOUSEACTIVATE message.
             */
            result = (int)xxxSendMessage(pwnd, WM_MOUSEACTIVATE,
                    (WPARAM)(HW(pwndTop)), MAKELONG((SHORT)ht, message));

            switch (result) {

            case 0:
            case MA_ACTIVATE:
            case MA_ACTIVATEANDEAT:

                /*
                 * If activation fails, swallow the message.
                 */
                if ((pwndTop != pti->pq->spwndActive ||
                        pti->pq->QF_flags & QF_EVENTDEACTIVATEREMOVED) &&
                        !xxxActivateWindow(pwndTop,
                          (UINT)((pti->pq->codeCapture == NO_CAP_CLIENT) ?
                          AW_TRY2 : AW_TRY))) {
                    result = MA_SKIP;
                } else if (TestWF(pwndTop, WFDISABLED)) {
#ifdef NEVER

                    /*
                     * Althoug this is what win3 does, it is braindead: it
                     * can easily cause infinite loops.  Returning "rehittest"
                     * means process this event over again - nothing causes
                     * anything different to happen, and we get an infinite
                     * loop.  This case never gets executed on win3 because if
                     * the window is disabled, it got the HTERROR hittest
                     * code.  This can only be done on Win32 where input is
                     * assigned to a window BEFORE process occurs to pull
                     * it out of the queue.
                     */
                    result = MA_REHITTEST;
#endif

                    /*
                     * Someone clicked on a window before it was disabled...
                     * Since it is disabled now, don't send this message to
                     * it: instead eat it.
                     */
                    result = MA_SKIP;
                } else if (result == MA_ACTIVATEANDEAT) {
                    result = MA_SKIP;
                } else {
                    result = MA_PASSTHRU;
                    goto ItsActiveJustCheckOnTop;
                }
                break;

            case MA_NOACTIVATEANDEAT:
                result = MA_SKIP;
                break;
            }
        }
    } else {
ItsActiveJustCheckOnTop:
        /*
         * Make sure this active window is on top (see comment
         * in CheckOnTop).
         */
        if (TestUP(ACTIVEWINDOWTRACKING)) {
            if (CheckOnTop(pti, pwndTop, message)) {
                /*
                 * The window was z-ordered to the top.
                 * If it is a console window, skip the message
                 *  so it won't go into "selecting" mode
                 * Hard error boxes are created by csrss as well
                 * If we have topmost console windows someday, this
                 *  will need to change
                 */
                 if ((ht == HTCLIENT)
                        && (GETPTI(pwndTop)->TIF_flags & TIF_CSRSSTHREAD)
                        && !(TestWF(pwndTop, WEFTOPMOST))) {

                     RIPMSG2(RIP_WARNING, "xxxMouseActivate: Skipping msg %#lx for pwnd %#p",
                            message, pwndTop);
                     result = MA_SKIP;
                 }
            }
        } /* if (TestUP(ACTIVEWINDOWTRACKING)) */
    }

    /*
     * Now set the cursor shape.
     */
    if (pti->pq->spwndCapture == NULL) {
        xxxSendMessage(pwnd, WM_SETCURSOR, (WPARAM)HW(pwnd),
                MAKELONG((SHORT)ht, message));
    }

    ThreadUnlock(&tlpwndTop);
    return result;
}

/***************************************************************************\
*  ResetMouseHover()
*
*  Resets mouse hover state information.
*
*  11/03/95    francish    created.
*  09/04/97    GerardoB    Rewritten to use per desktop tracking
\***************************************************************************/

void ResetMouseHover(PDESKTOP pdesk, POINT pt)
{
    /*
     * Reset the timer and hover rect
     */
    InternalSetTimer(pdesk->spwndTrack, IDSYS_MOUSEHOVER,
                     pdesk->dwMouseHoverTime,
                    xxxSystemTimerProc, TMRF_SYSTEM);

    SetRect(&pdesk->rcMouseHover,
            pt.x - gcxMouseHover / 2,
            pt.y - gcyMouseHover / 2,
            pt.x + gcxMouseHover / 2,
            pt.y + gcyMouseHover / 2);

}

/***************************************************************************\
*  QueryTrackMouseEvent()
*
*  Fills in a TRACKMOUSEEVENT structure describing current tracking state.
*
*  11/03/95    francish    created.
*  09/04/97    GerardoB    Rewritten to use per desktop tracking
\***************************************************************************/

BOOL QueryTrackMouseEvent(
    LPTRACKMOUSEEVENT lpTME)
{
    PTHREADINFO ptiCurrent = PtiCurrent();
    PDESKTOP pdesk = ptiCurrent->rpdesk;

    /*
     * initialize the struct
     */
    RtlZeroMemory(lpTME, sizeof(*lpTME));
    lpTME->cbSize = sizeof(*lpTME);
    /*
     * Bail if not tracking any mouse event
     *  or if the current thread is not in spwndTrack's queue
     */
    if (!(pdesk->dwDTFlags & DF_TRACKMOUSEEVENT)
            || (ptiCurrent->pq != GETPTI(pdesk->spwndTrack)->pq)) {
        return TRUE;
    }
    /*
     * fill in the requested information
     */
    if (pdesk->htEx != HTCLIENT) {
        lpTME->dwFlags |= TME_NONCLIENT;
    }
    if (pdesk->dwDTFlags & DF_TRACKMOUSELEAVE) {
        lpTME->dwFlags |= TME_LEAVE;
    }
    if (pdesk->dwDTFlags & DF_TRACKMOUSEHOVER) {
        lpTME->dwFlags |= TME_HOVER;
        lpTME->dwHoverTime = pdesk->dwMouseHoverTime;
    }

    lpTME->hwndTrack = HWq(pdesk->spwndTrack);

    return TRUE;
}

/***************************************************************************\
*  TrackMouseEvent()
*
*  API for requesting extended mouse notifications (hover, leave, ...)
*
*  11/03/95    francish    created.
*  09/04/97    GerardoB    Rewritten to use per desktop tracking
\***************************************************************************/
BOOL TrackMouseEvent(
    LPTRACKMOUSEEVENT lpTME)
{
    PDESKTOP pdesk = PtiCurrent()->rpdesk;
    PWND     pwnd;

    /*
     * Validate hwndTrack
     */
    pwnd = ValidateHwnd(lpTME->hwndTrack);
    if (pwnd == NULL) {
        return FALSE;
    }
    /*
     * If we're not tracking this window or not in correct hittest, bail
     */
    if ((pwnd != pdesk->spwndTrack)
            || (!!(lpTME->dwFlags & TME_NONCLIENT) ^ (pdesk->htEx != HTCLIENT))) {

        if ((lpTME->dwFlags & TME_LEAVE) && !(lpTME->dwFlags & TME_CANCEL)) {
            _PostMessage(pwnd,
                         ((lpTME->dwFlags & TME_NONCLIENT) ? WM_NCMOUSELEAVE : WM_MOUSELEAVE),
                         0, 0);
        }
        return TRUE;
    }

    /*
     * Process cancel request
     */
    if (lpTME->dwFlags & TME_CANCEL) {
        if (lpTME->dwFlags & TME_LEAVE) {
            pdesk->dwDTFlags &= ~DF_TRACKMOUSELEAVE;
        }
        if (lpTME->dwFlags & TME_HOVER) {
            if (pdesk->dwDTFlags & DF_TRACKMOUSEHOVER) {
                _KillSystemTimer(pwnd, IDSYS_MOUSEHOVER);
                pdesk->dwDTFlags &= ~DF_TRACKMOUSEHOVER;
            }
        }
        return TRUE;
    }

    /*
     * Track mouse leave
     */
    if (lpTME->dwFlags & TME_LEAVE) {
        pdesk->dwDTFlags |= DF_TRACKMOUSELEAVE;
    }
    /*
     * Track mouse hover
     */
    if (lpTME->dwFlags & TME_HOVER) {
        pdesk->dwDTFlags |= DF_TRACKMOUSEHOVER;

        pdesk->dwMouseHoverTime = lpTME->dwHoverTime;
        if ((pdesk->dwMouseHoverTime == 0) || (pdesk->dwMouseHoverTime == HOVER_DEFAULT)) {
            pdesk->dwMouseHoverTime = gdtMouseHover;
        }

        ResetMouseHover(pdesk, GETPTI(pwnd)->ptLast);
    }

    return TRUE;
}

/***************************************************************************\
* xxxGetNextSysMsg
*
* Returns the queue pointer of the next system message or
*  NULL             - no more messages (may be a journal playback delay)
*  PQMSG_PLAYBACK   - got a journal playback message
* (Anything else is a real pointer)
*
* 10-23-92 ScottLu      Created.
\***************************************************************************/

PQMSG xxxGetNextSysMsg(
    PTHREADINFO pti,
    PQMSG pqmsgPrev,
    PQMSG pqmsg)
{
    DWORD dt;
    PMLIST pml;
    PQMSG pqmsgT;

    /*
     * If there is a journal playback hook, call it to get the next message.
     */
    if ((PhkFirstGlobalValid(pti, WH_JOURNALPLAYBACK) != NULL)
            && (pti->rpdesk == grpdeskRitInput)) {
        /*
         * We can't search through journal messages: we only get the current
         * journal message. So if the caller has already called us once
         * before, then exit with no messages.
         */
        if (pqmsgPrev != 0)
            return NULL;

        /*
         * Tell the journal playback hook that we're done
         * with this message now.
         */
        dt = xxxCallJournalPlaybackHook(pqmsg);
        if (dt == 0xFFFFFFFF)
            return NULL;

        /*
         * If dt == 0, then we don't need to wait: set the right wake
         * bits and return this message.
         */
        if (dt == 0) {
            WakeSomeone(pti->pq, pqmsg->msg.message, NULL);
            return PQMSG_PLAYBACK;
        } else {
            /*
             * There is logically no more input in the "queue", so clear the
             * bits so that we will sleep when GetMessage is called.
             */
            pti->pcti->fsWakeBits &= ~QS_INPUT;
            pti->pcti->fsChangeBits &= ~QS_INPUT;

            /*
             * Need to wait before processing this next message. Set
             * a journal timer.
             */
            SetJournalTimer(dt, pqmsg->msg.message);

            return NULL;
        }
    }

    /*
     * No journalling going on... return next message in system queue.
     */

    /*
     * Queue up a mouse move if the mouse has moved.
     */
    if (pti->pq->QF_flags & QF_MOUSEMOVED) {
        PostMove(pti->pq);
    }

    /*
     * If no messages in the input queue, return with 0.
     */
    pml = &pti->pq->mlInput;
    if (pml->cMsgs == 0)
        return NULL;

    /*
     * If this is the first call to xxxGetNextSysMsg(), return the
     * first message.
     */
    if (pqmsgPrev == NULL || pti->pq->idSysPeek <= (ULONG_PTR)PQMSG_PLAYBACK) {
        pqmsgT = pml->pqmsgRead;
    } else {
        /*
         * Otherwise return the next message in the queue. Index with
         * idSysPeek, because that is updated by recursive calls through
         * this code.
         */
        pqmsgT = ((PQMSG)(pti->pq->idSysPeek))->pqmsgNext;
    }

    /*
     * Fill in the structure passed, and return the pointer to the
     * current message in the message list. This will become the new
     * pq->idSysPeek.
     */
    if (pqmsgT != NULL)
        *pqmsg = *pqmsgT;
    return pqmsgT;
}

/***************************************************************************\
* UpdateKeyState
*
* Updates queue key state tables.
*
* 11-11-92 ScottLu      Created.
\***************************************************************************/

void UpdateKeyState(
    PQ pq,
    UINT vk,
    BOOL fDown)
{
    if (vk != 0) {
        /*
         * If we're going down, toggle only if the key isn't
         * already down.
         */
        if (fDown && !TestKeyStateDown(pq, vk)) {
            if (TestKeyStateToggle(pq, vk)) {
                ClearKeyStateToggle(pq, vk);
            } else {
                SetKeyStateToggle(pq, vk);
            }
        }

        /*
         * Now set/clear the key down state.
         */
        if (fDown) {
            SetKeyStateDown(pq, vk);
        } else {
            ClearKeyStateDown(pq, vk);
        }

        /*
         * If this is one of the keys we cache, update the key cache index.
         */
        if (vk < CVKKEYCACHE) {
            gpsi->dwKeyCache++;
        }
    }
}

/***************************************************************************\
* EqualMsg
*
* This routine is called in case that idSysPeek points to a message
* and we are trying to remove a different message
*
* 04-25-96 CLupu      Created.
\***************************************************************************/

BOOL EqualMsg(PQMSG pqmsg1, PQMSG pqmsg2)
{
    if (pqmsg1->msg.hwnd    != pqmsg2->msg.hwnd ||
        pqmsg1->msg.message != pqmsg2->msg.message)
        return FALSE;

    /*
     * This might be a coalesced WM_MOUSEMOVE
     */
    if (pqmsg1->msg.message == WM_MOUSEMOVE)
        return TRUE;

    if (pqmsg1->pti      != pqmsg2->pti ||
        pqmsg1->msg.time != pqmsg2->msg.time)
        return FALSE;

    return TRUE;
}

/***************************************************************************\
* xxxSkipSysMsg
*
* This routine "skips" an input message: either by calling the journal
* hooks if we're journalling or by "skipping" the message in the input
* queue. Internal keystate tables are updated as well.
*
* 10-23-92 ScottLu      Created.
\***************************************************************************/

void xxxSkipSysMsg(
    PTHREADINFO pti,
    PQMSG pqmsg)
{
    PQMSG pqmsgT;
    BOOL  fDown;
    BYTE  vk;
    PHOOK phook;

    /*
     * If idSysPeek is 0, then the pqmsg that we were looking at has been
     * deleted, probably because of a callout from ScanSysQueue, and that
     * callout then called PeekMessage(fRemove == TRUE), and then returned.
     */
    if (pti->pq->idSysPeek == 0)
        return;

    phook = PhkFirstGlobalValid(pti, WH_JOURNALPLAYBACK);
    if (phook != NULL) {
        /*
         * Tell the journal playback hook that we're done
         * with this message now.
         */
        phook->flags |= HF_NEEDHC_SKIP;
    } else {
        phook = PhkFirstGlobalValid(pti, WH_JOURNALRECORD);
        if (phook != NULL) {
            /*
             * We've processed a new message: tell the journal record
             * hook what the message is.
             */
            xxxCallJournalRecordHook(pqmsg);
        }

        /*
         * If idSysPeek is 0 now, it means we've been recursed into yet
         * again. This would confuse a journalling app, but it would confuse
         * us more because we'd fault. Return if idSysPeek is 0.
         */
        if ((pqmsgT = (PQMSG)pti->pq->idSysPeek) == NULL)
            return;

        /*
         * Delete this message from the input queue. Make sure pqmsgT isn't
         * 1: this could happen if an app unhooked a journal record hook
         * during a callback from xxxScanSysQueue.
         */
        if (pqmsgT != PQMSG_PLAYBACK) {
            /*
             * There are cases when idSysPeek points to a different message
             * than the one we are trying to remove. This can happen if
             * two threads enters in xxxScanSysQueue, sets the idSysPeek and
             * after this their queues got redistributed. The first thread
             * will have the idSysPeek preserved but the second one has to
             * search the queue for its message. - ask CLupu
             */
            if (!EqualMsg(pqmsgT, pqmsg)) {

                PQMSG pqmsgS;

#if DEBUGTAGS
                if (IsDbgTagEnabled(DBGTAG_SysPeek)) {
                    gnSysPeekSearch++;
                }
#endif

                TAGMSG0(DBGTAG_SysPeek | RIP_THERESMORE,              "Different message than idSysPeek\n");
                TAGMSG2(DBGTAG_SysPeek | RIP_NONAME | RIP_THERESMORE, "pqmsg   = %#p  idSysPeek = %#p",  pqmsg,              pqmsgT);
                TAGMSG2(DBGTAG_SysPeek | RIP_NONAME | RIP_THERESMORE, "pti     = %#p  pti       = %#p",  pqmsg->pti,         pqmsgT->pti);
                TAGMSG2(DBGTAG_SysPeek | RIP_NONAME | RIP_THERESMORE, "msg     = %08lx  msg       = %08lx",  pqmsg->msg.message, pqmsgT->msg.message);
                TAGMSG2(DBGTAG_SysPeek | RIP_NONAME | RIP_THERESMORE, "hwnd    = %#p  hwnd      = %#p",  pqmsg->msg.hwnd,    pqmsgT->msg.hwnd);
                TAGMSG2(DBGTAG_SysPeek | RIP_NONAME | RIP_THERESMORE, "wParam  = %#p  wParam    = %#p",  pqmsg->msg.wParam,  pqmsgT->msg.wParam);
                TAGMSG2(DBGTAG_SysPeek | RIP_NONAME | RIP_THERESMORE, "lParam  = %#p  lParam    = %#p",  pqmsg->msg.lParam,  pqmsgT->msg.lParam);
                TAGMSG2(DBGTAG_SysPeek | RIP_NONAME | RIP_THERESMORE, "time    = %08lx  time      = %08lx",  pqmsg->msg.time,    pqmsgT->msg.time);
                TAGMSG2(DBGTAG_SysPeek | RIP_NONAME | RIP_THERESMORE, "Extra   = %08lx  Extra     = %08lx",  pqmsg->ExtraInfo,   pqmsgT->ExtraInfo);
                TAGMSG1(DBGTAG_SysPeek | RIP_NONAME,                  "\npqmsgT  = %#p", pqmsgT);

                /*
                 * Begin to search for this message
                 */
                pqmsgS = pti->pq->mlInput.pqmsgRead;

                while (pqmsgS != NULL) {
                    if (EqualMsg(pqmsgS, pqmsg)) {
                        TAGMSG2(DBGTAG_SysPeek | RIP_THERESMORE,
                                "Deleting pqmsg %#p, pti %#p",
                                pqmsgS, pqmsgS->pti);

                        TAGMSG4(DBGTAG_SysPeek | RIP_NONAME,
                                "m %04lx, w %#p, l %#p, t %lx",
                                pqmsgS->msg.message, pqmsgS->msg.hwnd,
                                pqmsgS->msg.lParam, pqmsgS->msg.time);

                        pqmsgT = pqmsgS;
                        break;
                    }
                    pqmsgS = pqmsgS->pqmsgNext;
                }
                if (pqmsgS == NULL) {
                    TAGMSG0(DBGTAG_SysPeek, "Didn't find a matching message. No message removed.");
                    return;
                }
            }

            if (pqmsgT == (PQMSG)pti->pq->idSysPeek) {
                /*
                 * We'll remove this message from the input queue
                 * so set idSysPeek to 0.
                 */
                CheckPtiSysPeek(1, pti->pq, 0);
                pti->pq->idSysPeek = 0;
            }
            DelQEntry(&pti->pq->mlInput, pqmsgT);
        }
    }

    fDown = TRUE;
    vk = 0;

    switch (pqmsg->msg.message) {
    case WM_MOUSEMOVE:
    case WM_QUEUESYNC:
    default:
        /*
         * No state change.
         */
        break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        fDown = FALSE;

        /*
         * Fall through.
         */
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        vk = LOBYTE(LOWORD(pqmsg->msg.wParam));
        break;

    case WM_LBUTTONUP:
        fDown = FALSE;

        /*
         * Fall through.
         */
    case WM_LBUTTONDOWN:
        vk = VK_LBUTTON;
        break;

    case WM_RBUTTONUP:
        fDown = FALSE;

        /*
         * Fall through.
         */
    case WM_RBUTTONDOWN:
        vk = VK_RBUTTON;
        break;

    case WM_MBUTTONUP:
        fDown = FALSE;

        /*
         * Fall through.
         */
    case WM_MBUTTONDOWN:
        vk = VK_MBUTTON;
        break;

    case WM_XBUTTONUP:
        fDown = FALSE;

        /*
         * Fall through.
         */
    case WM_XBUTTONDOWN:
        UserAssert(GET_XBUTTON_WPARAM(pqmsg->msg.wParam) == XBUTTON1 ||
                   GET_XBUTTON_WPARAM(pqmsg->msg.wParam) == XBUTTON2);

        switch (GET_XBUTTON_WPARAM(pqmsg->msg.wParam)) {
        case XBUTTON1:
            vk = VK_XBUTTON1;
            break;

        case XBUTTON2:
            vk = VK_XBUTTON2;
            break;
        }

        break;
    }

    /*
     * Set toggle and down bits appropriately.
     */
    if ((vk == VK_SHIFT) || (vk == VK_MENU) || (vk == VK_CONTROL)) {
        BYTE vkHanded, vkOtherHand;
        /*
         * Convert this virtual key into a differentiated (Left/Right) key
         * depending on the extended key bit.
         */
        vkHanded = (vk - VK_SHIFT) * 2 + VK_LSHIFT +
                ((pqmsg->msg.lParam & EXTENDED_BIT) ? 1 : 0);
        vkOtherHand = vkHanded ^ 1;

        if (vk == VK_SHIFT) {
            /*
             * Clear extended bit for r.h. Shift, since it isn't really
             * extended (bit was set to indicate right-handed)
             */
            pqmsg->msg.lParam &= ~EXTENDED_BIT;
        }

        /*
         * Update the key state for the differentiated (Left/Right) key.
         */
        UpdateKeyState(pti->pq, vkHanded, fDown);

        /*
         * Update key state for the undifferentiated (logical) key.
         */
        if (fDown || !TestKeyStateDown(pti->pq, vkOtherHand)) {
            UpdateKeyState(pti->pq, vk, fDown);
        }
    } else {
        UpdateKeyState(pti->pq, vk, fDown);
    }
}



#if DBG
/***************************************************************************\
* LogPlayback
*
*
* History:
* 02-13-95 JimA             Created.
\***************************************************************************/

void LogPlayback(
    PWND pwnd,
    PMSG lpmsg)
{
    static PWND pwndM = NULL, pwndK = NULL;
    LPCSTR lpszMsg;
    CHAR achBuf[20];

    if ((lpmsg->message >= WM_MOUSEFIRST) && (lpmsg->message <= WM_MOUSELAST)) {
        lpszMsg = aszMouse[lpmsg->message - WM_MOUSEFIRST];
        if (pwnd != pwndM) {
            DbgPrint("*** Mouse input to window \"%ws\" of class \"%s\"\n",
                    pwnd->strName.Length ? pwnd->strName.Buffer : L"",
                    pwnd->pcls->lpszAnsiClassName);
            pwndM = pwnd;
        }
    } else if ((lpmsg->message >= WM_KEYFIRST) && (lpmsg->message <= WM_KEYLAST)) {
        lpszMsg = aszKey[lpmsg->message - WM_KEYFIRST];
        if (pwnd != pwndK) {
            DbgPrint("*** Kbd input to window \"%ws\" of class \"%s\"\n",
                    pwnd->strName.Length ? pwnd->strName.Buffer : L"",
                    pwnd->pcls->lpszAnsiClassName);
            pwndK = pwnd;
        }
    } else if (lpmsg->message == WM_QUEUESYNC) {
        lpszMsg = "WM_QUEUESYNC";
    } else {
        sprintf(achBuf, "0x%4x", lpmsg->message);
        lpszMsg = achBuf;
    }
    DbgPrint("msg = %s, wP = %x, lP = %x\n", lpszMsg,
            lpmsg->wParam, lpmsg->lParam);
}
#endif  // DBG

/***************************************************************************\
*
*  GetMouseKeyFlags()
*
*  Computes MOST of the MK_ flags given a Q.
*  Does not compute MK_MOUSEENTER.
*
\***************************************************************************/

UINT GetMouseKeyFlags(
    PQ pq)
{
    UINT wParam = 0;

    if (TestKeyStateDown(pq, VK_LBUTTON))
        wParam |= MK_LBUTTON;
    if (TestKeyStateDown(pq, VK_RBUTTON))
        wParam |= MK_RBUTTON;
    if (TestKeyStateDown(pq, VK_MBUTTON))
        wParam |= MK_MBUTTON;
    if (TestKeyStateDown(pq, VK_XBUTTON1))
        wParam |= MK_XBUTTON1;
    if (TestKeyStateDown(pq, VK_XBUTTON2))
        wParam |= MK_XBUTTON2;
    if (TestKeyStateDown(pq, VK_SHIFT))
        wParam |= MK_SHIFT;
    if (TestKeyStateDown(pq, VK_CONTROL))
        wParam |= MK_CONTROL;

    return wParam;
}

/***************************************************************************\
* xxxScanSysQueue
*
* This routine looks at the hardware message, determines what
* window it will be in, determines what the input message will
* be, and then checks the destination window against hwndFilter,
* and the input message against msgMinFilter and msgMaxFilter.
*
* It also updates various input synchronized states like keystate info.
*
* This is almost verbatim from Win3.1.
*
* 10-20-92 ScottLu      Created.
\***************************************************************************/

BOOL xxxScanSysQueue(
    PTHREADINFO ptiCurrent,
    LPMSG lpMsg,
    PWND pwndFilter,
    UINT msgMinFilter,
    UINT msgMaxFilter,
    DWORD flags,
    DWORD fsReason)
{
    QMSG qmsg;
    HWND hwnd;
    PWND pwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
    PTHREADINFO ptiKeyWake, ptiMouseWake, ptiEventWake;
    POINT pt, ptScreen;
    UINT codeMouseDown;
    BOOL fMouseHookCalled;
    BOOL fKbdHookCalled;
    BOOL fOtherApp;
    int part;
    MOUSEHOOKSTRUCTEX mhs;
    PWND pwndT;
    BOOL fPrevDown;
    BOOL fDown;
    BOOL fAlt;
    TL tlpwnd;
    TL tlpwndT;
    BOOL fRemove = (flags & PM_REMOVE);
#ifdef FE_IME
    DWORD dwImmRet = 0;
#endif
#ifdef MARKPATH
    DWORD pathTaken = 0;
    DWORD pathTaken2 = 0;
    DWORD pathTaken3 = 0;

#define PATHTAKEN(x)  pathTaken  |= x
#define PATHTAKEN2(x) pathTaken2 |= x
#define PATHTAKEN3(x) pathTaken3 |= x
#define DUMPPATHTAKEN() if (gfMarkPath) DbgPrint("xxxScanSysQueue path:%08x %08x %08x\n", pathTaken, pathTaken2, pathTaken3)
#define DUMPSUBPATHTAKEN(p, x) if (gfMarkPath && p & x) { DbgPrint("  %08x %08x %08x\n", pathTaken, pathTaken2, pathTaken3); pathTaken = pathTaken2 = pathTaken3 = 0; }
#else
#define PATHTAKEN(x)
#define PATHTAKEN2(x)
#define PATHTAKEN3(x)
#define DUMPPATHTAKEN()
#define DUMPSUBPATHTAKEN(p, x)
#endif

    UserAssert(IsWinEventNotifyDeferredOK());
    UserAssert((fsReason & ~(QS_EVENT | QS_INPUT)) == 0 &&
               (fsReason & (QS_EVENT | QS_INPUT)) != 0);

    /*
     * If we are looking at a peeked message currently (recursion into this
     * routine) and the only reason we got here was because of an event
     * message (an app was filtering for a non-input message), then just
     * return so we don't screw up idSysPeek. If we do enter this code
     * idSysPeek will get set back to 0, and when we return back into
     * the previous xxxScanSysQueue(), SkipSysMsg() will do nothing, so the
     * message won't get removed. (MS Publisher 2.0 does this).
     */
    if (fsReason == QS_EVENT) {
        if (ptiCurrent->pq->idSysPeek != 0) {
            PATHTAKEN(1);
            DUMPPATHTAKEN();
            return FALSE;
        }
    }

    fDown = FALSE;
    fMouseHookCalled = FALSE;
    fKbdHookCalled = FALSE;

    /*
     * Lock the queue if it's currently unlocked.
     */
    if (ptiCurrent->pq->ptiSysLock == NULL) {
        CheckSysLock(3, ptiCurrent->pq, ptiCurrent);
        ptiCurrent->pq->ptiSysLock = ptiCurrent;
        ptiCurrent->pcti->CTIF_flags |= CTIF_SYSQUEUELOCKED;
    }

    /*
     * Flag to tell if locker was removing messages. If not, then next time
     * Get/PeekMessage is called, the input message list is scanned before the
     * post msg list.
     *
     * Under Win3.1, this flag only gets modified for key and mouse messages.
     * Since under NT ScanSysQueue() can be called to execute event messages,
     * we make this check to be compatible.
     */
    if (fsReason & QS_INPUT) {
        if (fRemove) {
            PATHTAKEN(2);
            ptiCurrent->pq->QF_flags &= ~QF_LOCKNOREMOVE;
        } else {
            PATHTAKEN(4);
            ptiCurrent->pq->QF_flags |= QF_LOCKNOREMOVE;
        }
    }

    /*
     * Return FALSE if the current thread is not the one that lock this queue.
     */
    if (ptiCurrent->pq->ptiSysLock != ptiCurrent) {
        PATHTAKEN(8);
        DUMPPATHTAKEN();
        return FALSE;
    }

    ptiEventWake = ptiKeyWake = ptiMouseWake = NULL;

    /*
     * Initialize the thread lock structure here so we can unlock/lock in
     * the main loop.
     */
    pwnd = NULL;
    ThreadLockWithPti(ptiCurrent, pwnd, &tlpwnd);

RestartScan:
    CheckPtiSysPeek(2, ptiCurrent->pq, 0);
    ptiCurrent->pq->idSysPeek = 0;

ContinueScan:
    while (TRUE) {
        ULONG_PTR idSysPeek;

        DUMPSUBPATHTAKEN(pathTaken, 0xf0);
        /*
         * Store idSysPeek in a local which forces pq to be reloaded
         * in case it changed during the xxx call (the compiler can
         * evaluate the LValue at any time)
         */
        idSysPeek = (ULONG_PTR)xxxGetNextSysMsg(ptiCurrent,
                (PQMSG)ptiCurrent->pq->idSysPeek, &qmsg);
        CheckPtiSysPeek(3, ptiCurrent->pq, idSysPeek);
        ptiCurrent->pq->idSysPeek = idSysPeek;

        if (ptiCurrent->pq->idSysPeek == 0) {
            /*
             * If we are only looking for event messages and we didn't
             * find any, then clear the QS_EVENT bit
             */
            if (fsReason == QS_EVENT)
                ClearWakeBit(ptiCurrent, QS_EVENT, FALSE);
            PATHTAKEN(0x10);
            goto NoMessages;
        }

        /*
         * pwnd should be locked for the duration of this routine.
         * For most messages right out of GetNextSysMsg, this is
         * NULL.
         */
        ThreadUnlock(&tlpwnd);
        pwnd = RevalidateHwnd(qmsg.msg.hwnd);
        ThreadLockWithPti(ptiCurrent, pwnd, &tlpwnd);

        /*
         * See if this is an event message. If so, execute it regardless
         * of message and window filters, but only if it is the first element
         * of the input queue.
         */
        if (qmsg.dwQEvent != 0) {
            PTHREADINFO pti;

            PATHTAKEN(0x20);
            /*
             * Most event messages can be executed out of order relative to
             * its place in the queue. There are some examples were this is
             * not allowed, and we check that here. For example, we would not
             * want a keystate synchronization event to be processed before
             * the keystrokes that came before it in the queue!
             *
             * We need to have most event messages be able to get processed
             * out of order because apps can be filtering for message ranges
             * that don't include input (like dde) - those scenarios still
             * need to process events such as deactivate event messages even
             * if there is input in the input queue.
             */
            switch (qmsg.dwQEvent) {
            case QEVENT_UPDATEKEYSTATE:
                /*
                 * If the message is not the next message in the queue, don't
                 * process it.
                 */
                if (ptiCurrent->pq->idSysPeek !=
                        (ULONG_PTR)ptiCurrent->pq->mlInput.pqmsgRead) {
                    PATHTAKEN(0x40);
                    continue;
                }
                break;
            }

            /*
             * If this event isn't for this thread, wake the thread it is
             * for.  A NULL qmsg.hti means that any thread can process
             * the event.
             */
            if (qmsg.pti != NULL && (pti = qmsg.pti) != ptiCurrent) {

                /*
                 * If somehow this event message got into the wrong queue,
                 * then ignore it.
                 */
                UserAssert(pti->pq == ptiCurrent->pq);
                if (pti->pq != ptiCurrent->pq) {
                    CleanEventMessage((PQMSG)ptiCurrent->pq->idSysPeek);
                    DelQEntry(&ptiCurrent->pq->mlInput,
                            (PQMSG)ptiCurrent->pq->idSysPeek);
                    PATHTAKEN(0x80);
                    goto RestartScan;
                }

                /*
                 * If ptiEventWake is already set, it means we've already
                 * found a thread to wake for event.
                 */
                if (ptiEventWake == NULL)
                    ptiEventWake = pti;

                /*
                 * Clear idSysPeek so that the targeted thread
                 * can always get it.  Look at the test at the
                 * start of this routine for more info.
                 */
                CheckPtiSysPeek(4, ptiCurrent->pq, 0);
                ptiCurrent->pq->idSysPeek = 0;
                PATHTAKEN(0x100);
                goto NoMessages;
            }

            /*
             * If this is called with PM_NOYIELD from a 16-bit app, skip
             * processing any event that can generate activation messages.  An
             * example is printing from PageMaker 5.0.  Bug #12662.
             */
            if ((flags & PM_NOYIELD) && (ptiCurrent->TIF_flags & TIF_16BIT)) {
                PATHTAKEN(0x200);
                switch (qmsg.dwQEvent) {

                /*
                 * The following events are safe to process if no yield
                 * is to occur.
                 */
                case QEVENT_UPDATEKEYSTATE:
                case QEVENT_ASYNCSENDMSG:
                    break;

                /*
                 * Skip all other events.
                 */
                default:
                    ptiCurrent->TIF_flags |= TIF_DELAYEDEVENT;
                    ptiCurrent->pClientInfo->dwTIFlags = ptiCurrent->TIF_flags;
                    PATHTAKEN(0x400);
                    goto ContinueScan;
                }
            }

            /*
             * Delete this before it gets processed so there are no
             * recursion problems.
             */
            DelQEntry(&ptiCurrent->pq->mlInput,
                    (PQMSG)ptiCurrent->pq->idSysPeek);

            /*
             * Clear idSysPeek before processing any events messages, because
             * they may recurse and want to use idSysPeek.
             */
            CheckPtiSysPeek(5, ptiCurrent->pq, 0);
            ptiCurrent->pq->idSysPeek = 0;
            xxxProcessEventMessage(ptiCurrent, &qmsg);

            /*
             * Restart the scan from the start so we start with 0 in
             * pq->idSysPeek (since that message is now gone!).
             */
            PATHTAKEN(0x800);
            goto RestartScan;
        }

        /*
         * If the reason we called was just to process event messages, don't
         * enumerate any other mouse or key messages!
         */
        if (fsReason == QS_EVENT) {
            PATHTAKEN(0x1000);
            continue;
        }

        switch (message = qmsg.msg.message) {
        case WM_QUEUESYNC:
            PATHTAKEN(0x2000);
            /*
             * This message is for CBT. Its parameters should already be
             * set up correctly.
             */
            wParam = 0;
            lParam = qmsg.msg.lParam;

            /*
             * Check if this is intended for the current app. Use the mouse
             * bit for WM_QUEUESYNC.
             */
            if (pwnd != NULL && GETPTI(pwnd) != ptiCurrent) {
                /*
                 * If this other app isn't going to read from this
                 * queue, then skip this message. This can happen with
                 * WM_QUEUESYNC if the app passed a window handle
                 * to the wrong queue. This isn't likely to happen in
                 * this case because WM_QUEUESYNCs come in while journalling,
                 * which has all threads sharing the same queue.
                 */
                if (GETPTI(pwnd)->pq != ptiCurrent->pq) {
                    PATHTAKEN(0x4000);
                    goto SkipMessage;
                }

                if (ptiMouseWake == NULL)
                    ptiMouseWake = GETPTI(pwnd);
                PATHTAKEN(0x8000);
                goto NoMessages;
            }

            if (!CheckMsgFilter(message, msgMinFilter, msgMaxFilter)) {
                PATHTAKEN(0x10000);
                goto NoMessages;
            }

            /*
             * Eat the message.
             */
            if (fRemove) {
                xxxSkipSysMsg(ptiCurrent, &qmsg);
            }

            /*
             * !!HARDWARE HOOK!! goes here.
             */

            /*
             * Return the message.
             */
            PATHTAKEN(0x20000);
            goto ReturnMessage;
            break;

        /*
         * Mouse message or generic hardware messages
         * Key messages are handled in case statements
         * further down in this switch.
         */
        default:
ReprocessMsg:
            DUMPSUBPATHTAKEN(pathTaken, 0x40000);
            PATHTAKEN(0x40000);
            /*
             * !!GENERIC HARDWARE MESSAGE!! support goes here.
             */

            /*
             * Take the mouse position out of the message.
             */
            pt.x = (int)(short)LOWORD(qmsg.msg.lParam);
            pt.y = (int)(short)HIWORD(qmsg.msg.lParam);

            /*
             * Assume we have a capture.
             */
            part = HTCLIENT;

            /*
             * We have a special global we use for when we're full screen.
             * All mouse input will go to this window.
             */
            if (gspwndScreenCapture != NULL) {
                /*
                 * Change the mouse coordinates to full screen.
                 */
                pwnd = gspwndScreenCapture;
                lParam = MAKELONG((WORD)qmsg.msg.pt.x,
                        (WORD)qmsg.msg.pt.y);
                PATHTAKEN(0x80000);
            } else if ((pwnd = ptiCurrent->pq->spwndCapture) == NULL) {

                PATHTAKEN(0x100000);
                /*
                 * We don't have the capture. Figure out which window owns
                 * this message.
                 *
                 * NOTE: Use gptiRit and not ptiCurrent to get the desktop
                 * window because if ptiCurrent is the thread that created
                 * the main desktop, it's associated desktop is the logon
                 * desktop - don't want to hittest against the logon desktop
                 * while switched into the main desktop!
                 */
                pwndT = gptiRit->rpdesk->pDeskInfo->spwnd;

                ThreadLockWithPti(ptiCurrent, pwndT, &tlpwndT);

                hwnd = xxxWindowHitTest(pwndT, pt, &part, WHT_IGNOREDISABLED);
                ThreadUnlock(&tlpwndT);

                if ((pwnd = RevalidateHwnd(hwnd)) == NULL) {
                    pwnd = ptiCurrent->rpdesk->pDeskInfo->spwnd;
                    PATHTAKEN(0x200000);
                }

                if (part == HTCLIENT) {
                    /*
                     * Part of the client... normal mouse message.
                     * NO_CAP_CLIENT means "not captured, in client area
                     * of window".
                     */
                    ptiCurrent->pq->codeCapture = NO_CAP_CLIENT;
                    PATHTAKEN(0x400000);
                } else {
                    /*
                     * Not part of the client... must be an NCMOUSEMESSAGE.
                     * NO_CAP_SYS is a creative name by raor which means
                     * "not captured, in system area of window."
                     */
                    ptiCurrent->pq->codeCapture = NO_CAP_SYS;
                    PATHTAKEN(0x800000);
                }
            }

            /*
             * We've reassigned pwnd, so lock it.
             */
            ThreadLockExchange(pwnd, &tlpwnd);

            if (fOtherApp = (GETPTI(pwnd) != ptiCurrent)) {

                PATHTAKEN(0x1000000);
                /*
                 * If this other app isn't going to read from this
                 * queue, then skip this message. This can happen if
                 * the RIT queues up a message thinking it goes to
                 * a particular hwnd, but then by the time GetMessage()
                 * is called for that thread, it doesn't go to that hwnd
                 * (like in the case of mouse messages, window rearrangement
                 * happens which changes which hwnd the mouse hits on).
                 */
                if (GETPTI(pwnd)->pq != ptiCurrent->pq) {
                    zzzSetCursor(SYSCUR(ARROW));
                    PATHTAKEN(0x2000000);
                    goto SkipMessage;
                }

                /*
                 * If we haven't already found a message that is intended
                 * for another app, remember that we have one.
                 */
                if (ptiMouseWake == NULL) {
                    ptiMouseWake = GETPTI(pwnd);
                    PATHTAKEN(0x4000000);
                }
            }

            /*
             * Map mouse coordinates based on hit test area code.
             */
            ptScreen = pt;
            switch (ptiCurrent->pq->codeCapture) {
            case CLIENT_CAPTURE:
            case NO_CAP_CLIENT:
#ifdef USE_MIRRORING
                //Screen To Client
                if (TestWF(pwnd, WEFLAYOUTRTL)) {
                    pt.x = pwnd->rcClient.right - pt.x;
                } else
#endif
                {
                    pt.x -= pwnd->rcClient.left;
                }
                pt.y -= pwnd->rcClient.top;
                PATHTAKEN2(2);
                break;

            case WINDOW_CAPTURE:
#ifdef USE_MIRRORING
                //Screen To Window
                if (TestWF(pwnd, WEFLAYOUTRTL)) {
                    pt.x = pwnd->rcWindow.right - pt.x;
                } else
#endif
                {
                    pt.x -= pwnd->rcWindow.left;
                }
                pt.y -= pwnd->rcWindow.top;
                PATHTAKEN2(4);
                break;
            }

            /*
             * Track mouse moves when it moves to a different window or
             * a different hit-test area for hot-tracking, tooltips,
             * active window tracking and TrackMouseEvent.
             * Mouse clicks reset tracking state too.
             * Do it only if the message is for the current thread;
             *  otherwise, the hit test code (part) is not valid
             *  (it's always HTCLIENT; see xxxWindowHitTest2).
             *  Tracking will take place when that thread wakes up
             * We also don't do it if this thread is not on pqCursor;
             *  that would be the case for a slow app that gets the
             *  input message when the mouse has already left its queue
             */
             if (!fOtherApp && (ptiCurrent->pq == gpqCursor)) {
                 BOOL fNewpwndTrack = (ptiCurrent->rpdesk->spwndTrack != pwnd);
                 int htEx = FindNCHitEx(pwnd, part, pt);
                 if ((message != WM_MOUSEMOVE)
                        || fNewpwndTrack
                        || (ptiCurrent->rpdesk->htEx != htEx)) {

                     xxxTrackMouseMove(pwnd, htEx, message);
                     ValidateThreadLocks(NULL, ptiCurrent->ptl, (ULONG_PTR)&tlpwnd, TRUE);
                 }

                 /*
                  * Reset mouse hovering if needed.
                  *
                  */
                 if (!fNewpwndTrack && (ptiCurrent->rpdesk->dwDTFlags & DF_TRACKMOUSEHOVER)) {
                     if ((message != WM_MOUSEMOVE)
                            || !PtInRect(&ptiCurrent->rpdesk->rcMouseHover, ptScreen)) {

                         ResetMouseHover(ptiCurrent->rpdesk, ptScreen);
                     }
                 } else {
                     /*
                      * Hover must be canceled.
                      */
                     UserAssert(!(ptiCurrent->rpdesk->dwDTFlags & DF_TRACKMOUSEHOVER));
                 }

             } /* if (!fOtherApp.... */

            /*
             * Now see if it matches the window handle filter. If not,
             * get the next message.
             */
            if (!CheckPwndFilter(pwnd, pwndFilter)) {
                PATHTAKEN(0x8000000);
                continue;
            }

            /*
             * See if we need to map to a double click.
             */
            codeMouseDown = 0;
            switch (message) {
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_XBUTTONDOWN:
                if (TestCF(pwnd, CFDBLCLKS) ||
                        ptiCurrent->pq->codeCapture == NO_CAP_SYS ||
                        IsMenuStarted(ptiCurrent)) {
                    codeMouseDown++;
                    PATHTAKEN(0x10000000);
                    if (qmsg.msg.time <= ptiCurrent->pq->timeDblClk &&
                            (!gbClientDoubleClickSupport) &&
                            HW(pwnd) == ptiCurrent->pq->hwndDblClk &&
                            message == ptiCurrent->pq->msgDblClk &&
                            (message != WM_XBUTTONDOWN ||
                              GET_XBUTTON_WPARAM(qmsg.msg.wParam) == ptiCurrent->pq->xbtnDblClk)) {
                        RECT rcDblClk = {
                            ptiCurrent->pq->ptDblClk.x - SYSMET(CXDOUBLECLK) / 2,
                            ptiCurrent->pq->ptDblClk.y - SYSMET(CYDOUBLECLK) / 2,
                            ptiCurrent->pq->ptDblClk.x + SYSMET(CXDOUBLECLK) / 2,
                            ptiCurrent->pq->ptDblClk.y + SYSMET(CYDOUBLECLK) / 2
                        };
                        if (PtInRect(&rcDblClk, qmsg.msg.pt)) {
                            message += (WM_LBUTTONDBLCLK - WM_LBUTTONDOWN);
                            codeMouseDown++;
                            PATHTAKEN(0x20000000);
                        }
                    }
                }

            // FALL THROUGH!!!

            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_MBUTTONUP:
            case WM_XBUTTONUP:
                /*
                 * Note that the mouse button went up or down if we were
                 * in menu status mode of alt-key down
                 */

                PATHTAKEN(0x40000000);
                if (ptiCurrent->pq->QF_flags & QF_FMENUSTATUS) {
                    ptiCurrent->pq->QF_flags |= QF_FMENUSTATUSBREAK;
                    PATHTAKEN(0x80000000);
                }
            }

            /*
             * Map message number based on hit test area code.
             */
            if (ptiCurrent->pq->codeCapture == NO_CAP_SYS) {
                message += (UINT)(WM_NCMOUSEMOVE - WM_MOUSEMOVE);
                wParam = (UINT)part;
                PATHTAKEN2(1);
            }

            /*
             * Message number has been mapped: see if it fits the filter.
             * If not, get the next message.
             */
            if (!CheckMsgFilter(message, msgMinFilter, msgMaxFilter)) {
                PATHTAKEN2(8);
                continue;
            }

             /*
             * If message is for another app but it fits our filter, then
             * we should stop looking for messages: this will ensure that
             * we don't keep looking and find and process a message that
             * occured later than the one that should be processed by the
             * other guy.
             */
            if (fOtherApp) {
                PATHTAKEN2(0x10);
                goto NoMessages;
            }

            /*
             * If we're doing full drag, the mouse messages should go to
             * the xxxMoveSize PeekMessage loop. So we get the next message.
             * This can happen when an application does a PeekMessage in
             * response to a message sent inside the movesize dragging loop.
             * This causes the dragging loop to not get the WM_LBUTTONUP
             * message and dragging continues after the button is up
             * (fix for Micrografx Draw). -johannec
             */
            if (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST &&
                    ptiCurrent->TIF_flags & TIF_MOVESIZETRACKING) {
                PATHTAKEN2(0x20);
                continue;
            }

            if (FWINABLE() && (ptiCurrent->TIF_flags & TIF_MSGPOSCHANGED)) {
                ptiCurrent->TIF_flags &= ~TIF_MSGPOSCHANGED;
                xxxWindowEvent(EVENT_OBJECT_LOCATIONCHANGE, NULL,
                    OBJID_CURSOR, INDEXID_CONTAINER, TRUE);
                ValidateThreadLocks(NULL, ptiCurrent->ptl, (ULONG_PTR)&tlpwnd, TRUE);
            }

            /*
             * Let us call the mouse hook to find out if this click is
             * permitted by it.
             *
             * We want to inform the mouse hook before we test for
             * HTNOWHERE and HTERROR; Otherwise, the mouse hook won't
             * get these messages (sankar 12/10/91).
             */
            if (IsHooked(ptiCurrent, WHF_MOUSE)) {
                fMouseHookCalled = TRUE;
                mhs.pt = qmsg.msg.pt;
                mhs.hwnd = HW(pwnd);
                mhs.wHitTestCode = (UINT)part;
                mhs.dwExtraInfo = qmsg.ExtraInfo;
                UserAssert(LOWORD(qmsg.msg.wParam) == 0);
                mhs.mouseData = (DWORD)qmsg.msg.wParam;

                if (xxxCallMouseHook(message, &mhs, fRemove)) {
                    /*
                     * Not allowed by mouse hook; so skip it.
                     */
                    PATHTAKEN2(0x40);
                    goto SkipMessage;
                }
                PATHTAKEN2(0x80);
                ValidateThreadLocks(NULL, ptiCurrent->ptl, (ULONG_PTR)&tlpwnd, TRUE);
            }

            /*
             * If a HTERROR or HTNOWHERE occured, send the window the
             * WM_SETCURSOR message so it can beep or whatever. Then skip
             * the message and try the next one.
             */
            switch (part) {
            case HTERROR:
            case HTNOWHERE:
                /*
                 * Now set the cursor shape.
                 */
                xxxSendMessage(pwnd, WM_SETCURSOR, (WPARAM)HW(pwnd),
                        MAKELONG(part, qmsg.msg.message));

                /*
                 * Skip the message.
                 */
                PATHTAKEN2(0x100);
                goto SkipMessage;
                break;
            }

            if (fRemove) {
                PATHTAKEN2(0x200);
                /*
                 * Since the processing of a down click may cause the next
                 * message to be interpreted as a double click, we only want
                 * to do the double click setup if we're actually going to
                 * remove the message.  Otherwise, the next time we read the
                 * same message it would be interpreted as a double click.
                 */
                switch (codeMouseDown) {
                case 1:
                    /*
                     * Down clock: set up for later possible double click.
                     */
                    ptiCurrent->pq->msgDblClk = qmsg.msg.message;

                    /*
                     * Note that even if the following assertion were not true,
                     * we could still put bogus data in ptiCurrent->pq->xbtnDblClk
                     * when the message is not WM_XBUTTONDOWN, since when we check
                     * for dblclick we compare the message number before the xbtnDblClk.
                     */
                    UserAssert(qmsg.msg.message == WM_XBUTTONDOWN || GET_XBUTTON_WPARAM(qmsg.msg.wParam) == 0);
                    ptiCurrent->pq->xbtnDblClk = GET_XBUTTON_WPARAM(qmsg.msg.wParam);

                    ptiCurrent->pq->timeDblClk = qmsg.msg.time + gdtDblClk;
                    ptiCurrent->pq->hwndDblClk = HW(pwnd);
                    ptiCurrent->pq->ptDblClk = qmsg.msg.pt;
                    PATHTAKEN2(0x400);
                    break;

                case 2:
                    /*
                     * Double click: finish processing.
                     */
                    ptiCurrent->pq->timeDblClk = 0L;
                    PATHTAKEN2(0x800);
                    break;

                default:
                    PATHTAKEN2(0x1000);
                    break;
                }

                /*
                 * Set mouse cursor and allow app to activate window
                 * only if we're removing the message.
                 */
                switch (xxxMouseActivate(ptiCurrent, pwnd,
                        qmsg.msg.message, qmsg.msg.wParam, &qmsg.msg.pt, part)) {
SkipMessage:
                case MA_SKIP:
                    DUMPSUBPATHTAKEN(pathTaken2, 0x2000);
                    PATHTAKEN2(0x2000);
                    xxxSkipSysMsg(ptiCurrent, &qmsg);

                    /*
                     * Inform the CBT hook that we skipped a mouse click.
                     */
                    if (fMouseHookCalled) {
                        if (IsHooked(ptiCurrent, WHF_CBT)) {
                            xxxCallHook(HCBT_CLICKSKIPPED, message,
                                    (LPARAM)&mhs, WH_CBT);
                            PATHTAKEN2(0x4000);
                        }
                        fMouseHookCalled = FALSE;
                    }

                    /*
                     * Inform the CBT hook that we skipped a key
                     */
                    if (fKbdHookCalled) {
                        if (IsHooked(ptiCurrent, WHF_CBT)) {
                            xxxCallHook(HCBT_KEYSKIPPED, wParam, lParam,
                                    WH_CBT);
                            PATHTAKEN2(0x8000);
                        }
                        fKbdHookCalled = FALSE;
                    }

                    /*
                     * If we aren't removing messages, don't reset idSysPeek
                     * otherwise we will go into an infinite loop if
                     * the keyboard hook says to ignore the message.
                     * (bobgu 4/7/87).
                     */
                    if (!fRemove) {
                        PATHTAKEN2(0x10000);
                        goto ContinueScan;
                    } else {
                        PATHTAKEN2(0x20000);
                        goto RestartScan;
                    }
                    break;

                case MA_REHITTEST:
                    /*
                     * Reprocess the message.
                     */
                    PATHTAKEN2(0x40000);
                    goto ReprocessMsg;
                }
            }

            /*
             * Eat the message from the input queue (and set the keystate
             * table).
             */
            PATHTAKEN2(0x80000);
            if (fRemove) {
                xxxSkipSysMsg(ptiCurrent, &qmsg);
            }

            if (fRemove && fMouseHookCalled && IsHooked(ptiCurrent, WHF_CBT)) {
                xxxCallHook(HCBT_CLICKSKIPPED, message,
                        (LPARAM)&mhs, WH_CBT);
            }
            fMouseHookCalled = FALSE;

            lParam = MAKELONG((short)pt.x, (short)pt.y);

            /*
             * Calculate virtual key state bitmask for wParam.
             */
            if (message >= WM_MOUSEFIRST) {
                /*
                 * This is a USER mouse message. Calculate the bit mask for the
                 * virtual key state.
                 */
                wParam = GetMouseKeyFlags(ptiCurrent->pq);
                PATHTAKEN2(0x100000);
            }

            if (    (WM_NCXBUTTONFIRST <= message && message <= WM_NCXBUTTONLAST) ||
                    (WM_XBUTTONFIRST <= message && message <= WM_XBUTTONLAST)) {

                /*
                 * The hiword of wParam is assigned the xbutton number when
                 * the message is queued.
                 */
                UserAssert(LOWORD(qmsg.msg.wParam) == 0);
                UserAssert(HIWORD(wParam) == 0);
                wParam |= qmsg.msg.wParam;
            }

            PATHTAKEN2(0x200000);

            /*
             * If this app has a modeles menu bar,
             *  then the menu code should get the first shot at messages on the menu
             * Note that this assumes that xxxHandleMenuMessages
             *  doens't need any of the stuff set after ReturnMessage
             */
            if ((part == HTMENU)
                    && fRemove
                    && (ptiCurrent->pMenuState != NULL)
                    && ptiCurrent->pMenuState->fModelessMenu
                    && (ptiCurrent->pMenuState->pGlobalPopupMenu != NULL)
                    && (ptiCurrent->pMenuState->pGlobalPopupMenu->fIsMenuBar)) {

                if (xxxCallHandleMenuMessages(ptiCurrent->pMenuState, pwnd, message, wParam, lParam)) {
                    goto RestartScan;
                }
            }

            goto ReturnMessage;
            break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            fDown = TRUE;

            /*
             * If we are sending keyboard input to an app that has been
             * spinning then boost it back up.  If we don't you use spinning
             * apps like Write or Project and do two builds in the
             * background.  Note the app will also be unboosted again shortly
             * after you stop typing by the old logic. #11188
             */
            if (ptiCurrent->TIF_flags & TIF_SPINNING)
                CheckProcessForeground(ptiCurrent);

            /*
             * Apps doing journal playback sometimes put trash in the hiword
             * of wParam... zero it out here.
             */
            wParam = qmsg.msg.wParam & 0xFF;

            /*
             * Clear QF_FMENUSTATUS if a key other than Alt it hit
             * since this means the break of the Alt wouldn't be a
             * menu key anymore.
             */
            if (wParam != VK_MENU)
                ptiCurrent->pq->QF_flags &= ~(QF_FMENUSTATUS|QF_FMENUSTATUSBREAK);

            /*
             * Check for keyboard language toggle.  Build the key state
             * here for use during key up processing (where the layout
             * switching takes place.  This code is skipped if layout
             * switching via the keyboard is disabled.
             */
            if (gLangToggle[0].bVkey && (gLangToggleKeyState < KLT_NONE)) {
                DWORD i;
                BYTE scancode = LOBYTE(HIWORD(qmsg.msg.lParam));
                BYTE vkey = LOBYTE(qmsg.msg.wParam);

                for (i = 0; i < LANGTOGGLEKEYS_SIZE; i++) {
                    if (gLangToggle[i].bScan) {
                        if (gLangToggle[i].bScan == scancode) {
                            gLangToggleKeyState |= gLangToggle[i].iBitPosition;
                            break;
                        }
                    } else {
                        if (gLangToggle[i].bVkey == vkey) {
                            gLangToggleKeyState |= gLangToggle[i].iBitPosition;
                            break;
                        }
                    }
                }

                if (i == LANGTOGGLEKEYS_SIZE) {
                    gLangToggleKeyState = KLT_NONE;   // not a language toggle combination
                }
            }

            /*
             * Check if it is the PrintScrn key.
             */
            fAlt = TestKeyStateDown(ptiCurrent->pq, VK_MENU);
            if (wParam == VK_SNAPSHOT &&
                ((fAlt && !(ptiCurrent->fsReserveKeys & CONSOLE_ALTPRTSC)) ||
                 (!fAlt && !(ptiCurrent->fsReserveKeys & CONSOLE_PRTSC)))) {

                /*
                 * Remove this message from the input queue.
                 */
                PATHTAKEN2(0x400000);
                xxxSkipSysMsg(ptiCurrent, &qmsg);

                /*
                 * PRINTSCREEN          -> Snap the whole screen.
                 * ALT-PRINTSCREEN      -> Snap the current window.
                 */
                pwndT = ptiCurrent->pq->spwndActive;

                /*
                 * check also the scan code to see if we got here
                 * through keybd_event(VK_SNAPSHOT, ...
                 * the scan code is in lParam bits 16-23
                 */
                if (!fAlt && ((qmsg.msg.lParam & 0x00FF0000) != 0x00010000)) {
                    pwndT = ptiCurrent->rpdesk->pDeskInfo->spwnd;
                }

                if (pwndT != NULL) {
                    ThreadLockAlwaysWithPti(ptiCurrent, pwndT, &tlpwndT);
                    xxxSnapWindow(pwndT);
                    ThreadUnlock(&tlpwndT);
                }

                PATHTAKEN2(0x800000);
                goto RestartScan;
            }

            /*
             * Check for hot keys being hit if any are defined.
             */
            if (gcHotKey != 0 && (!gfEnableHexNumpad || (gfInNumpadHexInput & NUMPAD_HEXMODE_HL) == 0)) {
                UINT key;
                key = (UINT)wParam;

                if (TestKeyStateDown(ptiCurrent->pq, VK_MENU))
                    key |= 0x0400;

                if (TestKeyStateDown(ptiCurrent->pq, VK_CONTROL))
                    key |= 0x0200;

                if (TestKeyStateDown(ptiCurrent->pq, VK_SHIFT))
                    key |= 0x0100;

                pwndT = HotKeyToWindow(key);

                if (pwndT != NULL) {
                    /*
                     * VK_PACKET shouldn't be a hot key.
                     */
                    UserAssert((key & 0xff) != VK_PACKET);

                    _PostMessage(ptiCurrent->pq->spwndActive, WM_SYSCOMMAND,
                                (WPARAM)SC_HOTKEY, (LPARAM)HWq(pwndT));

                    /*
                     * Remove this message from the input queue.
                     */
                    xxxSkipSysMsg(ptiCurrent, &qmsg);
                    PATHTAKEN2(0x1000000);
                    goto RestartScan;
                }

                PATHTAKEN2(0x2000000);
            }

#if DBG
            else if (gfInNumpadHexInput & NUMPAD_HEXMODE_HL) {
                RIPMSG0(RIP_VERBOSE, "xxxScanSysQueue: gfInNumpadHexInput is true, so we skipped hotkey.");
            }
#endif

            if (wParam == VK_PACKET) {
                /*
                 * Save the character in thread's cache for TranslateMessage
                 */
                ptiCurrent->wchInjected = HIWORD(qmsg.msg.wParam);
                qmsg.msg.wParam = wParam;
                UserAssert(qmsg.msg.wParam == VK_PACKET);
            }

            /*
             * Fall through.
             */

        case WM_SYSKEYUP:
        case WM_KEYUP:
            wParam = qmsg.msg.wParam & 0xFF;
            if (wParam == VK_PACKET) {
                qmsg.msg.wParam = wParam;
            }

            /*
             * Special processing for thai locale toggle using grave accent key
             * Remove key message irrespective of fDown otherwise it will
             * generate WM_CHAR message
             */
            if (gbGraveKeyToggle &&
                //
                // In case of mstsc.exe, should not eat Grave Accent key message.
                // TS client must send Grave Accent key message to server side.
                //
                !(GetAppImeCompatFlags(NULL) & IMECOMPAT_HYDRACLIENT) &&
                LOBYTE(HIWORD(qmsg.msg.lParam)) == SCANCODE_THAI_LAYOUT_TOGGLE &&
                fRemove &&
                !TestKeyStateDown(ptiCurrent->pq, VK_SHIFT)   &&
                !TestKeyStateDown(ptiCurrent->pq, VK_MENU)    &&
                !TestKeyStateDown(ptiCurrent->pq, VK_CONTROL) &&
                !TestKeyStateDown(ptiCurrent->pq, VK_LWIN)    &&
                !TestKeyStateDown(ptiCurrent->pq, VK_RWIN)){

                if ((pwnd = ptiCurrent->pq->spwndFocus) == NULL){
                    pwnd = ptiCurrent->pq->spwndActive;
                }

                /*
                 * Post message only on WM_KEYUP
                 */
                if (!fDown && pwnd){
                    PTHREADINFO     ptiToggle = GETPTI(pwnd);
                    PKL             pkl = ptiToggle->spklActive;

                    if (pkl && (pkl = HKLtoPKL(ptiToggle, (HKL)HKL_NEXT))) {
                        _PostMessage(
                            pwnd,
                            WM_INPUTLANGCHANGEREQUEST,
                            (WPARAM)(((pkl->dwFontSigs & gSystemFS) ? INPUTLANGCHANGE_SYSCHARSET : 0) | INPUTLANGCHANGE_FORWARD),
                            (LPARAM)pkl->hkl
                            );
                    }
                }
                /*
                 * eat Accent Grave's key msgs
                 */
                xxxSkipSysMsg(ptiCurrent, &qmsg);
                goto RestartScan;
            }

            /*
             * Process keyboard toggle keys only if this is
             * a break event and fRemove == TRUE.  Some apps,
             * for instance Word 95, call PeekMessage with
             * PM_NOREMOVE followed by a call with PM_REMOVE.
             * We only want to process this once.  Skip all
             * of this is layout switching via the keyboard
             * is disabled.
             */
            if (!fDown && fRemove && gLangToggle[0].bVkey) {
                BOOL bDropToggle = FALSE;
                DWORD dwDirection = 0;
                PKL pkl;
                PTHREADINFO ptiToggle;
                // PWND pwndTop;
                BOOL bArabicSwitchPresent = FALSE;
                LCID lcid;

                ZwQueryDefaultLocale(FALSE, &lcid);

                pwnd = ptiCurrent->pq->spwndFocus;
                if (pwnd == NULL) {
                    pwnd = ptiCurrent->pq->spwndActive;
                    if (!pwnd) {
                        goto NoLayoutSwitch;
                    }
                }

                ptiToggle = GETPTI(pwnd);
                pkl = ptiToggle->spklActive;
                UserAssert(ptiToggle->spklActive != NULL);

                /*
                 * Check for Arabic toggle context
                 */
                if (gLangToggleKeyState < KLT_NONE && PRIMARYLANGID(lcid) == LANG_ARABIC){
                    PKL pkl_next = HKLtoPKL (ptiToggle, (HKL)HKL_NEXT);

                    /*
                     * test if there are exactly two pkl's and at least one
                     * of them is arabic
                     */
                    if (pkl && pkl_next &&
                        pkl->hkl != pkl_next->hkl && pkl_next == HKLtoPKL(ptiToggle, (HKL)HKL_PREV) &&
                        (PRIMARYLANGID(HandleToUlong(pkl->hkl)) == LANG_ARABIC || PRIMARYLANGID(HandleToUlong(pkl_next->hkl)) == LANG_ARABIC)){
                        bArabicSwitchPresent = TRUE;
                    }
                }

                /*
                 * NT has always had Alt LShift going forward (down) the list,
                 * and Alt RShift going backwards. Windows '95 is different.
                 */
                switch (gLangToggleKeyState) {
                case KLT_ALTLEFTSHIFT:
                   bDropToggle = TRUE;
                   dwDirection = INPUTLANGCHANGE_FORWARD;
                   if (!bArabicSwitchPresent || PRIMARYLANGID(HandleToUlong(pkl->hkl)) == LANG_ARABIC){
                       pkl = HKLtoPKL(ptiToggle, (HKL)HKL_NEXT);
                   }
                   break;

                case KLT_ALTRIGHTSHIFT:
                   bDropToggle = TRUE;
                   dwDirection = INPUTLANGCHANGE_BACKWARD;
                   if (!bArabicSwitchPresent || PRIMARYLANGID(HandleToUlong(pkl->hkl)) != LANG_ARABIC){
                       pkl = HKLtoPKL(ptiToggle, (HKL)HKL_PREV);
                   }
                   break;

                case KLT_ALTBOTHSHIFTS:
                   pkl = gspklBaseLayout;
                   break;

                default:
                   goto NoLayoutSwitch;
                   break;
                }

                if (pkl == NULL) {
                    pkl = GETPTI(pwnd)->spklActive;
                }

                /*
                 * If these two are not NULL, then winlogon hasn't loaded
                 * any keyboard layouts yet: but nobody should be getting
                 * input yet, so Assert but check pkl anyway. #99321
                 */
                UserAssert(gspklBaseLayout != NULL);
                UserAssert(pkl);
                if (pkl) {
                    /*
                     * Not a very satisfactory window to post to, but it's hard
                     * to figure out a better window. Just do as Memphis does.
                     * Note: The following went up too high, bypassing Word
                     * when using wordmail - IanJa bug #64744.
                     *    if ((pwndTop = GetTopLevelWindow(pwnd)) != NULL) {
                     *       pwnd = pwndTop;
                     *    }
                     */
                    _PostMessage(pwnd, WM_INPUTLANGCHANGEREQUEST,
                            (DWORD)(((pkl->dwFontSigs & gSystemFS) ? INPUTLANGCHANGE_SYSCHARSET : 0) | dwDirection),
                            (LPARAM)pkl->hkl);
                }

NoLayoutSwitch:

                if (bDropToggle) {
                    /*
                     * Clear this key from the key state so that multiple key
                     * presses will work (i.e., Alt+Shft+Shft).  We don't do
                     * this when both shift keys are pressed simultaneously to
                     * avoid two activates.
                     */
                    DWORD i;
                    BYTE scancode = LOBYTE(HIWORD(qmsg.msg.lParam));
                    BYTE vkey = LOBYTE(qmsg.msg.wParam);

                    for (i = 0; i < LANGTOGGLEKEYS_SIZE; i++) {
                        if (gLangToggle[i].bScan) {
                            if (gLangToggle[i].bScan == scancode) {
                                gLangToggleKeyState &= ~(gLangToggle[i].iBitPosition);
                            }
                        } else {
                            if (gLangToggle[i].bVkey == vkey) {
                                gLangToggleKeyState &= ~(gLangToggle[i].iBitPosition);
                            }
                        }
                    }
                } else {
                    gLangToggleKeyState = 0;
                }
            }

            /*
             * Convert F10 to syskey for new apps.
             */
            if (wParam == VK_F10)
                message |= (WM_SYSKEYDOWN - WM_KEYDOWN);

            if (TestKeyStateDown(ptiCurrent->pq, VK_CONTROL) &&
                    wParam == VK_ESCAPE) {
                message |= (WM_SYSKEYDOWN - WM_KEYDOWN);
            }

            /*
             * Clear the 'simulated keystroke' bit for all applications except
             * console so it can pass it to 16-bit vdms. VDM keyboards need to
             * distinguish between AltGr (where Ctrl keystroke is simulated)
             * and a real Ctrl+Alt. Check TIF_CSRSSTHREAD for the console
             * input thread because it lives in the server. This is a cheap
             * way to check for it.
             */
            if (!(ptiCurrent->TIF_flags & TIF_CSRSSTHREAD))
                qmsg.msg.lParam &= ~FAKE_KEYSTROKE;
            PATHTAKEN2(0x4000000);

            /*
             * Fall through.
             */

            /*
             * Some apps want to be able to feed WM_CHAR messages through
             * the playback hook. Why? Because they want to be able to
             * convert a string of characters info key messages
             * and feed them to themselves or other apps. Unfortunately,
             * there are no machine independent virtual key codes for
             * some characters (for example '$'), so they need to send
             * those through as WM_CHARs. (6/10/87).
             */

        case WM_CHAR:
            wParam = qmsg.msg.wParam & 0xFF;

            /*
             * Assign the input to the focus window. If there is no focus
             * window, assign it to the active window as a SYS message.
             */
            pwnd = ptiCurrent->pq->spwndFocus;
            if (ptiCurrent->pq->spwndFocus == NULL) {
                if ((pwnd = ptiCurrent->pq->spwndActive) != NULL) {
                    if (CheckMsgFilter(message, WM_KEYDOWN, WM_DEADCHAR)) {
                        message += (WM_SYSKEYDOWN - WM_KEYDOWN);
                        PATHTAKEN2(0x8000000);
                    }
                } else {
                    PATHTAKEN2(0x10000000);
                    goto SkipMessage;
                }
            }

            /*
             * If there is no active window or focus window, eat this
             * message.
             */
            if (pwnd == NULL) {
                PATHTAKEN2(0x20000000);
                goto SkipMessage;
            }

            ThreadUnlock(&tlpwnd);
            ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwnd);

            /*
             * Check if this is intended for the current app.
             */
            if (fOtherApp = (GETPTI(pwnd) != ptiCurrent)) {
                PWND pwndModalLoop;

                /*
                 * If this other app isn't going to read from this
                 * queue, then skip this message. This can happen if
                 * the RIT queues up a message thinking it goes to
                 * a particular hwnd, but then by the time GetMessage()
                 * is called for that thread, it doesn't go to that hwnd
                 * (like in the case of mouse messages, window rearrangement
                 * happens which changes which hwnd the mouse hits on).
                 */
                if (GETPTI(pwnd)->pq != ptiCurrent->pq) {
                    PATHTAKEN2(0x40000000);
                    goto SkipMessage;
                }

                /*
                 * If the current thread is in the menu or movesize loop
                 *  then we need to give it the input
                 */
                if (IsInsideMenuLoop(ptiCurrent)) {
                    pwndModalLoop = ptiCurrent->pMenuState->pGlobalPopupMenu->spwndNotify;
                } else if (ptiCurrent->pmsd != NULL) {
                    pwndModalLoop = ptiCurrent->pmsd->spwnd;
                    RIPMSG0(RIP_WARNING, "xxxScanSysQueue: returning key to movesize loop");
                } else {
                    pwndModalLoop = NULL;
                }

                /*
                 * If we're switching windows, lock the new one
                 */
                if (pwndModalLoop != NULL) {
                    pwnd = pwndModalLoop;
                    fOtherApp = (GETPTI(pwnd) != ptiCurrent);
                    ThreadUnlock(&tlpwnd);
                    ThreadLockWithPti(ptiCurrent, pwnd, &tlpwnd);
                    PATHTAKEN2(0x80000000);
                }

                /*
                 * If not for us, then remember who it is for.
                 */
                if (ptiKeyWake == NULL) {
                    PATHTAKEN3(1);
                    ptiKeyWake = GETPTI(pwnd);
                }
            }

            /*
             * See if this thing matches our filter.
             */
            if (!CheckMsgFilter(message, msgMinFilter, msgMaxFilter) ||
                    !CheckPwndFilter(pwnd, pwndFilter)) {
                PATHTAKEN3(2);
                continue;
            }

            /*
             * This message matches our filter. If it is not for us then
             * stop searching to make sure the real owner processes this
             * message first.
             */
            if (fOtherApp) {
                PATHTAKEN3(4);
                goto NoMessages;
            }

            /*
             * Generate some special messages if we are removing and we are
             * not inside the menu loop.
             */
            if (fRemove && !IsInsideMenuLoop(ptiCurrent)) {
                /*
                 * Generate a WM_CONTEXTMENU for the VK_APPS key
                 */
                if ((wParam == VK_APPS) && (message == WM_KEYUP)) {
                    _PostMessage(pwnd, WM_CONTEXTMENU, (WPARAM)PtoH(pwnd), KEYBOARD_MENU);
                }

                /*
                 * If this is a WM_KEYDOWN message for F1 key then we must generate
                 * the WM_KEYF1 message.
                 */
                if ((wParam == VK_F1) && (message == WM_KEYDOWN)) {
                    _PostMessage(pwnd, WM_KEYF1, 0, 0);
                }
            }

            /*
             * If one Shift key is released while the other Shift key is held
             * down, this keystroke is normally skipped, presumably to prevent
             * applications from thinking that the shift condition no longer
             * applies.
             */
            if (wParam == VK_SHIFT) {
                BYTE vkHanded, vkOtherHand;

                if (qmsg.msg.lParam & EXTENDED_BIT) {
                    vkHanded = VK_RSHIFT;
                } else {
                    vkHanded = VK_LSHIFT;
                }
                vkOtherHand = vkHanded ^ 1;

                if (!fDown && TestKeyStateDown(ptiCurrent->pq, vkOtherHand)) {
                    /*
                     * Unlike normal apps, Console MUST be sent a Shift break
                     * even when the other Shift key is still down, since it
                     * has to be passed on to VDM, which maintains it's own
                     * state. Check TIF_CSRSSTHREAD for the console input
                     * thread because it lives in the server. This is a cheap
                     * way to check for it.
                     */
                    if ((ptiCurrent->TIF_flags & TIF_CSRSSTHREAD) == 0) {
                        /*
                         * We ignore this key event, so we must update
                         * it's key state whether fRemove is TRUE or not.
                         * (ignoring an key event is same as removing it)
                         */
                        qmsg.msg.wParam = vkHanded;
                        xxxSkipSysMsg(ptiCurrent, &qmsg);
                        PATHTAKEN3(8);
                        goto RestartScan;
                    }
                    PATHTAKEN3(0x10);
                }
            }

            /*
             * Get the previous up/down state of the key here since
             * SkipSysMsg() sets the key state table and destroys
             * the previous state info.
             */
            fPrevDown = FALSE;
            if (TestKeyStateDown(ptiCurrent->pq, wParam))
                fPrevDown = TRUE;

            /*
             * Eat the message from the input queue and set the keystate
             * table.
             */
            PATHTAKEN3(0x20);
            if (fRemove) {
                xxxSkipSysMsg(ptiCurrent, &qmsg);
            }

            /*
             * This gets us the LOWORD of lParam, the repeat count,
             * the bit in the hi byte indicating whether this is an extended
             * key, and the scan code.  We also need to re-get the wParam in
             * case xxxSkipSysMsg called a hook which modified the message.
             * AfterDark's password protection does this.
             */
            lParam = qmsg.msg.lParam;
            wParam = qmsg.msg.wParam;

            /*
             * Indicate if it was previously down.
             */
            if (fPrevDown)
                lParam |= 0x40000000;           // KF_REPEAT

            /*
             * Set the transition bit.
             */
            switch (message) {
            case WM_KEYUP:
            case WM_SYSKEYUP:
                lParam |= 0x80000000;           // KF_UP
                break;
            }

            /*
             * Set the alt key down bit.
             */
            if (TestKeyStateDown(ptiCurrent->pq, VK_MENU)) {
                lParam |= 0x20000000;           // KF_ALTDOWN
            }

            /*
             * Set the menu state flag.
             */
            if (IsMenuStarted(ptiCurrent)) {
                lParam |= 0x10000000;           // KF_MENUMODE
            }

            /*
             * Set the dialog state flag.
             */
            if (ptiCurrent->pq->QF_flags & QF_DIALOGACTIVE) {
                lParam |= 0x08000000;           // KF_DLGMODE
            }

            /*
             * 0x80000000 is set if up, clear if down
             * 0x40000000 is previous up/down state of key
             * 0x20000000 is whether the alt key is down
             * 0x10000000 is whether currently in menumode.
             * 0x08000000 is whether in dialog mode
             * 0x04000000 is not used
             * 0x02000000 is not used
             * 0x01000000 is whether this is an extended keyboard key
             *
             * Low word is repeat count, low byte hiword is scan code,
             * hi byte hiword is all these bits.
             */

            /*
             * Callback the client IME before calling the keyboard hook.
             * If the vkey is one of the IME hotkeys, the vkey will not
             * be passed to the keyboard hook.
             * If IME needs this vkey, VK_PROCESSKEY will be put into the
             * application queue instead of real vkey.
             */
            UserAssert(ptiCurrent != NULL);
            if (gpImeHotKeyListHeader != NULL &&
                    fRemove &&
                    !IsMenuStarted(ptiCurrent) &&
                    !(ptiCurrent->TIF_flags & TIF_DISABLEIME) &&
                    pwnd != NULL) {

                WPARAM wParamTemp = wParam;

                if (wParam == VK_PACKET) {
                    wParamTemp = MAKEWPARAM(wParam, ptiCurrent->wchInjected);
                }

                /*
                 * xxxImmProcessKey also checks the registered IME hotkeys.
                 */
                dwImmRet = xxxImmProcessKey( ptiCurrent->pq,
                                             pwnd,
                                             message,
                                             wParamTemp,
                                             lParam);
                if ( dwImmRet & (IPHK_HOTKEY | IPHK_SKIPTHISKEY) ) {
                    dwImmRet = 0;
                    goto SkipMessage;
                }
            }

            /*
             * If we are removing the message, call the keyboard hook
             * with HC_ACTION, otherwise call the hook with HC_NOREMOVE
             * to let it know that the message is not being removed.
             */
            if (IsHooked(ptiCurrent, WHF_KEYBOARD)) {
                fKbdHookCalled = TRUE;
                if (xxxCallHook(fRemove ? HC_ACTION : HC_NOREMOVE,
                        wParam, lParam, WH_KEYBOARD)) {
                    PATHTAKEN3(0x40);
                    goto SkipMessage;
                }
            }

            if (fKbdHookCalled && fRemove && IsHooked(ptiCurrent, WHF_CBT)) {
                xxxCallHook(HCBT_KEYSKIPPED, wParam, lParam, WH_CBT);
                PATHTAKEN3(0x80);
            }

            fKbdHookCalled = FALSE;
            PATHTAKEN3(0x100);
            goto ReturnMessage;

        case WM_MOUSEWHEEL:
            /*
             * If we are sending keyboard input to an app that has been
             * spinning then boost it back up.  If we don't you use spinning
             * apps like Write or Project and do two builds in the
             * background.  Note the app will also be unboosted again shortly
             * after you stop typing by the old logic. #11188
             */
            if (ptiCurrent->TIF_flags & TIF_SPINNING)
                CheckProcessForeground(ptiCurrent);

            /*
             * Assign the input to the focus window. If there is no focus
             * window, or we are in a menu loop, eat this message.
             */
            pwnd = ptiCurrent->pq->spwndFocus;
            if (pwnd == NULL || IsInsideMenuLoop(ptiCurrent)) {
                PATHTAKEN2(0x20000000);
                goto SkipMessage;
            }

            ThreadUnlock(&tlpwnd);
            ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwnd);

            /*
             * Check if this is intended for the current app.
             */
            if (fOtherApp = (GETPTI(pwnd) != ptiCurrent)) {

                /*
                 * If this other app isn't going to read from this
                 * queue, then skip this message. This can happen if
                 * the RIT queues up a message thinking it goes to
                 * a particular hwnd, but then by the time GetMessage()
                 * is called for that thread, it doesn't go to that hwnd
                 * (like in the case of mouse messages, window rearrangement
                 * happens which changes which hwnd the mouse hits on).
                 */
                if (GETPTI(pwnd)->pq != ptiCurrent->pq) {
                    PATHTAKEN2(0x40000000);
                    goto SkipMessage;
                }

                /*
                 * If not for us, then remember who it is for.
                 */
                if (ptiKeyWake == NULL) {
                    PATHTAKEN3(1);
                    ptiKeyWake = GETPTI(pwnd);
                }
            }

            /*
             * See if this thing matches our filter.
             * NOTE: We need to check whether the caller is filtering
             * for all mouse messages - if so, we assume the caller
             * wants mouse wheel messages too.
             */
            if (    !CheckMsgFilter(WM_MOUSEWHEEL, msgMinFilter, msgMaxFilter) ||
                    !CheckPwndFilter(pwnd, pwndFilter)) {
                PATHTAKEN3(2);
                continue;
            }

            /*
             * This message matches our filter. If it is not for us then
             * stop searching to make sure the real owner processes this
             * message first.
             */
            if (fOtherApp) {
                PATHTAKEN3(4);
                goto NoMessages;
            }

            /*
             * Eat the message from the input queue and set the keystate
             * table.
             */
            PATHTAKEN3(0x20);
            if (fRemove) {
                xxxSkipSysMsg(ptiCurrent, &qmsg);
            }

            wParam = GetMouseKeyFlags(ptiCurrent->pq);
            UserAssert(LOWORD(qmsg.msg.wParam) == 0);
            UserAssert(HIWORD(wParam) == 0);
            wParam |= qmsg.msg.wParam;
            lParam = qmsg.msg.lParam;

            /*
             * If we are removing the message, call the mouse hook
             * with HC_ACTION, otherwise call the hook with HC_NOREM
             * to let it know that the message is not being removed.
             */
            if (IsHooked(ptiCurrent, WHF_MOUSE)) {
                fMouseHookCalled = TRUE;
                mhs.pt = qmsg.msg.pt;
                mhs.hwnd = HW(pwnd);
                mhs.wHitTestCode = HTNOWHERE;
                mhs.dwExtraInfo = qmsg.ExtraInfo;
                mhs.mouseData = (DWORD)qmsg.msg.wParam;
                if (xxxCallMouseHook(message, &mhs, fRemove)) {
                    /*
                     * Not allowed by mouse hook; so skip it.
                     */
                    PATHTAKEN3(0x40);
                    goto SkipMessage;
                }
            }

            if (fMouseHookCalled && fRemove && IsHooked(ptiCurrent, WHF_CBT)) {
                /*
                 * CONSIDER: Add new HCBT_ constant for the mouse wheel?
                 */
                xxxCallHook(HCBT_CLICKSKIPPED, message, (LPARAM)&mhs, WH_CBT);
                PATHTAKEN3(0x80);
            }

            fMouseHookCalled = FALSE;
            PATHTAKEN3(0x100);
            goto ReturnMessage;
        } /* End of switch (message = qmsg.msg.message) */
    } /* End of the GetNextSysMsg() loop */

ReturnMessage:
    if (!RtlEqualMemory(&ptiCurrent->ptLast, &qmsg.msg.pt, sizeof(POINT))) {
        ptiCurrent->TIF_flags |= TIF_MSGPOSCHANGED;
    }
    ptiCurrent->ptLast = qmsg.msg.pt;
    ptiCurrent->timeLast = qmsg.msg.time;
    ptiCurrent->pq->ExtraInfo = qmsg.ExtraInfo;

    /*
     * idSysLock value of 1 indicates that the message came from the input
     * queue.
     */
    ptiCurrent->idLast = ptiCurrent->pq->idSysLock = 1;

    /*
     * Now see if our input bit is set for this input. If it isn't, set ours
     * and clear the guy who had it previously.
     */
    TransferWakeBit(ptiCurrent, message);

    /*
     * Clear the input bits if no messages in the input queue.
     */
    ClearWakeBit(ptiCurrent, QS_MOUSE | QS_KEY | QS_EVENT | QS_TRANSFER, TRUE);

    /*
     * Get the message and split.
     */
    lpMsg->hwnd = HW(pwnd);
    lpMsg->message = message;

    /*
     * If the IME claims that it needs this vkey, replace it
     * with VK_PROCESSKEY. The real vkey has been saved in
     * the input context in the client side.
     */
    lpMsg->wParam = (dwImmRet & IPHK_PROCESSBYIME) ? VK_PROCESSKEY : wParam;

    lpMsg->lParam = lParam;
    lpMsg->time = qmsg.msg.time;
    lpMsg->pt = qmsg.msg.pt;

#if DBG
    if (gfLogPlayback && ptiCurrent->pq->idSysPeek == (LONG_PTR)PQMSG_PLAYBACK)
        LogPlayback(pwnd, lpMsg);
#endif  // DBG

    ThreadUnlock(&tlpwnd);

    PATHTAKEN3(0x200);
    DUMPPATHTAKEN();
    return TRUE;

NoMessages:
    /*
     * The message was for another app, or none were found that fit the
     * filter.
     */

    /*
     * Unlock the system queue.
     */
    ptiCurrent->pq->idSysLock  = 0;
    CheckSysLock(4, ptiCurrent->pq, NULL);
    ptiCurrent->pq->ptiSysLock = NULL;
    ptiCurrent->pcti->CTIF_flags &= ~CTIF_SYSQUEUELOCKED;

    /*
     * Wake up someone else if we found a message for him.  QS_TRANSFER
     * signifies that the thread was woken due to input transfer
     * from another thread, rather than from a real input event.
     */
    if (ptiKeyWake != NULL || ptiMouseWake != NULL || ptiEventWake != NULL) {
        PATHTAKEN3(0x400);
        if (ptiKeyWake != NULL) {
            SetWakeBit(ptiKeyWake, QS_KEY | QS_TRANSFER);
            ClearWakeBit(ptiCurrent, QS_KEY | QS_TRANSFER, FALSE);
            PATHTAKEN3(0x800);
        }

        if (ptiMouseWake != NULL) {
            SetWakeBit(ptiMouseWake, QS_MOUSE | QS_TRANSFER);
            ClearWakeBit(ptiCurrent, QS_MOUSE | QS_TRANSFER, FALSE);
            PATHTAKEN3(0x1000);
        }

        if (ptiEventWake != NULL) {
            SetWakeBit(ptiEventWake, QS_EVENTSET);
            ClearWakeBit(ptiCurrent, QS_EVENT, FALSE);
            PATHTAKEN3(0x2000);
        } else if (FJOURNALPLAYBACK()) {

            /*
             * If journal playback is occuring, clear the input bits.  This will
             * help prevent a race condition between two threads that call
             * WaitMessage/PeekMessage.  This can occur when embedding an OLE
             * object.  An example is inserting a Word object into an Excel
             * spreadsheet.
             * Also clear change bits else this thread might not xxxSleepThread.
             */
            ptiCurrent->pcti->fsWakeBitsJournal |= (ptiCurrent->pcti->fsWakeBits &
                    (QS_MOUSE | QS_KEY | QS_TRANSFER));
            ClearWakeBit(ptiCurrent, QS_MOUSE | QS_KEY | QS_TRANSFER, FALSE);
            ptiCurrent->pcti->fsChangeBits &= ~(QS_MOUSE | QS_KEY | QS_TRANSFER);
        }
    } else {
        /*
         * Clear the input bits if no messages in the input queue.
         */
        ptiCurrent->pcti->fsWakeBitsJournal = 0;
        ClearWakeBit(ptiCurrent, QS_MOUSE | QS_KEY | QS_EVENT |
                QS_TRANSFER, TRUE);
        PATHTAKEN3(0x4000);
    }

    ThreadUnlock(&tlpwnd);

    PATHTAKEN3(0x8000);
    DUMPPATHTAKEN();
    return FALSE;
}
#undef PATHTAKEN
#undef PATHTAKEN2
#undef PATHTAKEN3
#undef DUMPPATHTAKEN
#undef DUMPSUBPATHTAKEN


/***************************************************************************\
* IdleTimerProc
*
* This will start the screen saver app
*
* History:
* 09-06-91  mikeke      Created.
* 03-26-92  DavidPe     Changed to be run from hungapp timer on RIT.
\***************************************************************************/

VOID IdleTimerProc(VOID)
{

    CheckCritIn();

    if (    (TestAsyncKeyStateDown(VK_LBUTTON)) ||
            (TestAsyncKeyStateDown(VK_RBUTTON)) ||
            (TestAsyncKeyStateDown(VK_MBUTTON)) ||
            (TestAsyncKeyStateDown(VK_XBUTTON1)) ||
            (TestAsyncKeyStateDown(VK_XBUTTON2))) {

        return;
    }


    if (giScreenSaveTimeOutMs > 0) {

        if (IsTimeFromLastInput((DWORD)(giScreenSaveTimeOutMs))) {

            if (gppiScreenSaver != NULL) {

                if (!(gppiScreenSaver->W32PF_Flags & W32PF_IDLESCREENSAVER)) {
                    /*
                     * Bump the priority of the screen saver down to idle.
                     */
                    gppiScreenSaver->W32PF_Flags |= W32PF_IDLESCREENSAVER;
                    SetForegroundPriorityProcess(gppiScreenSaver, gppiScreenSaver->ptiMainThread, TRUE);
                }
            } else {
                /*
                 * Tell the system that it needs to bring up a screen saver.
                 *
                 * Carefull with the case when the active window is hung. If this
                 * is the case the screen saver won't be started by winlogon because
                 * DefWindowProc won't call StartScreenSaver(FALSE).
                 */
                if ((gpqForeground != NULL) &&
                    (gpqForeground->spwndActive != NULL) &&
                    !FHungApp(GETPTI(gpqForeground->spwndActive), CMSHUNGAPPTIMEOUT)) {

                    /*
                     * Tell winlogon to start the screen saver if we have a secure
                     * screen saver. In case we do have a secure one, the next PostMessage
                     * will be ignored in winlogon.
                     */
                    StartScreenSaver(TRUE);
                    _PostMessage(gpqForeground->spwndActive, WM_SYSCOMMAND, SC_SCREENSAVE, 0L);
                } else {
                    StartScreenSaver(FALSE);
                }
            }
        }
    }

    if ((giLowPowerTimeOutMs > 0) && ((glinp.dwFlags & LINP_LOWPOWER) == 0)) {
        if (IsTimeFromLastInput((DWORD)(giLowPowerTimeOutMs))) {
            if ((gpqForeground != NULL) && (gpqForeground->spwndActive != NULL)) {
                _PostMessage(gpqForeground->spwndActive, WM_SYSCOMMAND, SC_MONITORPOWER, LOWPOWER_PHASE);
            }
        }
    }

    if ((giPowerOffTimeOutMs > 0) && ((glinp.dwFlags & LINP_POWEROFF) == 0)) {
        if (IsTimeFromLastInput((DWORD)(giPowerOffTimeOutMs))) {
            if ((gpqForeground != NULL) && (gpqForeground->spwndActive != NULL)) {
                _PostMessage(gpqForeground->spwndActive, WM_SYSCOMMAND, SC_MONITORPOWER, POWEROFF_PHASE);
            }
        }
    }

}

/***************************************************************************\
* zzzWakeInputIdle
*
* The calling thread is going "idle". Wake up any thread waiting for this.
*
* 09-24-91 ScottLu      Created.
\***************************************************************************/

void zzzWakeInputIdle(
    PTHREADINFO pti)
{
    PW32PROCESS W32Process = W32GetCurrentProcess();

    /*
     * clear out the TIF_FIRSTIDLE since here we are
     */
    pti->TIF_flags &= ~TIF_FIRSTIDLE;


    /*
     * Shared Wow Apps use the per thread idle event for synchronization.
     * Separate Wow VDMs use the regular mechanism.
     */
    if (pti->TIF_flags & TIF_SHAREDWOW) {
        UserAssert(pti->TIF_flags & TIF_16BIT);
        if (pti->ptdb->pwti) {
            SET_PSEUDO_EVENT(&pti->ptdb->pwti->pIdleEvent);
        }
    } else {
        /*
         * If the main thread is NULL, set it to this queue: it is calling
         * GetMessage().
         */
        if (pti->ppi->ptiMainThread == NULL)
            pti->ppi->ptiMainThread = pti;

        /*
         * Wake up anyone waiting on this event.
         */
        if (pti->ppi->ptiMainThread == pti) {
            SET_PSEUDO_EVENT(&W32Process->InputIdleEvent);
        }
    }

    /*
     * Check to see if the startglass is on, and if so turn it off and update.
     */
    if (W32Process->W32PF_Flags & W32PF_STARTGLASS) {
        /*
         * This app is no longer in "starting" mode. Recalc when to hide
         * the app starting cursor.
         */
        W32Process->W32PF_Flags &= ~W32PF_STARTGLASS;
        zzzCalcStartCursorHide(NULL, 0);
    }
}

void SleepInputIdle(
    PTHREADINFO pti)
{
    PW32PROCESS W32Process;

    /*
     * Shared Wow Apps use the per thread idle event for synchronization.
     * Separate Wow VDMs use the regular mechanism.
     */
    if (pti->TIF_flags & TIF_SHAREDWOW) {
        UserAssert(pti->TIF_flags & TIF_16BIT);
        if (pti->ptdb->pwti) {
            RESET_PSEUDO_EVENT(&pti->ptdb->pwti->pIdleEvent);
        }
    } else {
        /*
         * If the main thread is NULL, set it to this queue: it is calling
         * GetMessage().
         */
        if (pti->ppi->ptiMainThread == NULL)
            pti->ppi->ptiMainThread = pti;

        /*
         * Put to sleep up anyone waiting on this event.
         */
        if (pti->ppi->ptiMainThread == pti) {
            W32Process = W32GetCurrentProcess();
            RESET_PSEUDO_EVENT(&W32Process->InputIdleEvent);
        }
    }
}

/***************************************************************************\
* zzzRecalcThreadAttachment
* zzzRecalc2
* zzzAddAttachment
* CheckAttachment
*
* Runs through all the attachinfo fields for all threads and calculates
* which threads share which queues. Puts calculated result in pqAttach
* field in each threadinfo structure. This is a difficult problem
* whose only solution in iterative. The basic algorithm is:
*
* 0. Find next unattached thread and attach a queue to it. If none, stop.
* 1. Loop through all threads: If thread X assigned to this queue or any
*    of X's attach requests assigned to this queue, assign X and all X's
*    attachments to this queue. Remember if we ever attach a 16 bit thread.
* 2. If thread X is a 16 bit thread and we've already attached another
*    16 bit thread, assign X and all X's attachments to this queue.
* 3. If any change found in 1-2, goto 1
* 4. Goto 0
*
* 12-11-92 ScottLu      Created.
* 01-Oct-1993 mikeke    Fixed to work with MWOWs
\***************************************************************************/

void zzzAddAttachment(
    PTHREADINFO pti,
    PQ pqAttach,
    LPBOOL pfChanged)
{
    if (pti->pqAttach != pqAttach) {
        /*
         * LATER
         * !!! This is totally screwed up,  The only reason that this thing
         * could be non null is because two threads are going through
         * zzzAttachThreadInput() at the same time.  No one can predict
         * what kind of problems are going to be caused by that.
         * We leave the critical section in one place where we send
         * WM_CANCELMODE below.  We should figure out how to remove
         * the sendmessage.
         *
         * If there already is a queue there, as there may be, destroy it.
         * Note that zzzDestroyQueue() will only get rid of the queue if the
         * thread reference count goes to 0.
         */
        PQ pqDestroy = pti->pqAttach;
        pti->pqAttach = pqAttach;
        if (pqDestroy != NULL)
            zzzDestroyQueue(pqDestroy, pti);
        pqAttach->cThreads++;
        *pfChanged = TRUE;
    }
}

void zzzRecalc2(
    PQ pqAttach)
{
    PATTACHINFO pai;
    PTHREADINFO pti;
    BOOL fChanged;
    PLIST_ENTRY pHead, pEntry;

    /*
     * Defer Win Event notifications so we can traverse PtiList with impunity
     * #bug number from shiflet
     */
    DeferWinEventNotify();
    BEGINATOMICCHECK();

    /*
     * Keep adding attachments until everything that should be attached to this
     * queue is attached
     */
    do {
        fChanged = FALSE;

        /*
         * If a thread is attached to this Q attach all of it's attachments
         * and MWOW buddies if they aren't already attached.
         */
        pHead = &PtiCurrent()->rpdesk->PtiList;
        for (pEntry = pHead->Flink; pEntry != pHead; pEntry = pEntry->Flink) {
            pti = CONTAINING_RECORD(pEntry, THREADINFO, PtiLink);

            if (pti->pqAttach == pqAttach) {
                /*
                 * check each of the attachments to see if this thread is attached
                 * to any other threads
                 */
                for (pai = gpai; pai != NULL; pai = pai->paiNext) {
                    /*
                     * if they weren't attached already, attach them
                     */
                    if (pai->pti1 == pti || pai->pti2 == pti) {
                        zzzAddAttachment((pai->pti1 == pti) ? pai->pti2 : pai->pti1,
                                pqAttach, &fChanged);
                    }
                }

                /*
                 * If this is a 16bit thread attach to all other threads in
                 * it's MWOW
                 */
                if (pti->TIF_flags & TIF_16BIT) {
                    PTHREADINFO ptiAttach;
                    PLIST_ENTRY pHeadAttach, pEntryAttach;

                    pHeadAttach = &pti->rpdesk->PtiList;
                    for (pEntryAttach = pHeadAttach->Flink;
                            pEntryAttach != pHeadAttach;
                            pEntryAttach = pEntryAttach->Flink) {
                        ptiAttach = CONTAINING_RECORD(pEntryAttach, THREADINFO, PtiLink);

                        if (ptiAttach->TIF_flags & TIF_16BIT &&
                            ptiAttach->ppi == pti->ppi) {
                            zzzAddAttachment(ptiAttach, pqAttach, &fChanged);
                        }
                    }
                }
            }
        }
    } while (fChanged);
    ENDATOMICCHECK();
    zzzEndDeferWinEventNotify();
}


void zzzRecalcThreadAttachment()
{
    PTHREADINFO pti;
    PLIST_ENTRY pHead, pEntry;

    /*
     * Win Event notifications must be defered so we can traverse PtiList with impunity
     */
    UserAssert(IsWinEventNotifyDeferred());

    /*
     * For all threads, start an attach queue if a thread hasn't been
     * attached yet.
     */
    pHead = &PtiCurrent()->rpdesk->PtiList;
    for (pEntry = pHead->Flink; pEntry != pHead; pEntry = pEntry->Flink) {
        pti = CONTAINING_RECORD(pEntry, THREADINFO, PtiLink);

        /*
         * Assert: We should not leave the critsect from xxxCreateThreadInfo
         * with the new thread in the rpdesk->PtiList but not yet with a queue.
         */
        UserAssert(pti->pq != NULL);

        if (pti->pqAttach == NULL) {

            /*
             * Allocate a new queue for this thread if more than
             * one thread references it.
             */
            if (pti->pq->cThreads > 1) {
                pti->pqAttach = AllocQueue(NULL, NULL);

                if (pti->pqAttach == NULL) {
                    RIPMSG0(RIP_WARNING, "zzzRecalcThreadAttachment: AllocQueue failed");
                    break;
                }

                pti->pqAttach->cThreads++;
            } else {
                pti->pqAttach = pti->pq;
            }

            /*
             * Attach every thread that is directly or indirectly attached
             * to this thread.
             */
            zzzRecalc2(pti->pqAttach);
        }
    }
}


/***************************************************************************\
* RedistributeInput
*
* This routine takes a input stream from the queue being left, and
* redistributes it. This effectively filters out the messages destined
* to the thread that left the queue.
*
* 12-10-92 ScottLu      Created.
\***************************************************************************/

void RedistributeInput(
    PQMSG pqmsgS,
    PQ    pqRedist)
{
    PTHREADINFO ptiSave;
    PTHREADINFO ptiT;
    PQMSG *ppqmsgD;
    PQMSG pqmsgT;
    PMLIST pmlInput;

    /*
     * Since the thread attaching or unattaching may have left a queue
     * shared by other threads, the messages we are going to requeue
     * may have multiple destinations. On top of this, once we find
     * a home queue for a message, it needs to be inserted in the
     * list ordered by its time stamp (older messages go at the end).
     */

    /*
     * Loop through a given dest's messages to find where to insert
     * the source messages, based on message time stamp. Be sure
     * to deal with empty message lists (meaning, check for NULL).
     */

    ptiT = NULL;
    ppqmsgD = NULL;
    pmlInput = NULL;

    while (pqmsgS != NULL) {

        /*
         * Find out where this message should go.
         */
        ptiSave = ptiT;
        ptiT = pqmsgS->pti;

        /*
         * Get rid of some event messages.
         *
         * QEVENT_UPDATEKEYSTATE: key state already up to date
         */
        if (pqmsgS->dwQEvent == QEVENT_UPDATEKEYSTATE) {
            ptiT = NULL;
        }

        if (ptiT == NULL) {
            /*
             * Unlink it. pqmsgS should be the first in the list
             */

            UserAssert(!pqmsgS->pqmsgPrev);
            if (pqmsgS->pqmsgNext != NULL) {
                pqmsgS->pqmsgNext->pqmsgPrev = NULL;
            }

            pqmsgT = pqmsgS;
            pqmsgS = pqmsgS->pqmsgNext;

            /*
             * Clean it / free it.
             */
            CleanEventMessage(pqmsgT);
            FreeQEntry(pqmsgT);

            ptiT = ptiSave;
            continue;
        }

        /*
         * Point to the pointer that points to the first message
         * that this message should go to, so that pointer is easy to
         * update, no matter where it is.
         */
        if (ppqmsgD == NULL || ptiSave != ptiT) {

            /*
             * If the source is younger than the last message in the
             * destination, go to the end.  Otherwise, start at the
             * head of the desination list and find a place to insert
             * the message.
             */
            if (ptiT->pq->mlInput.pqmsgWriteLast != NULL &&
                    pqmsgS->msg.time >= ptiT->pq->mlInput.pqmsgWriteLast->msg.time) {
                ppqmsgD = &ptiT->pq->mlInput.pqmsgWriteLast->pqmsgNext;
            } else {
                ppqmsgD = &ptiT->pq->mlInput.pqmsgRead;
            }

            pmlInput = &ptiT->pq->mlInput;
        }

        /*
         * If we're not at the end of the destination AND the destination
         * message time is younger than the source time, go on to
         * the next message.
         */
        while (*ppqmsgD != NULL && ((*ppqmsgD)->msg.time <= pqmsgS->msg.time)) {
            ppqmsgD = &((*ppqmsgD)->pqmsgNext);
        }

        /*
         * Link in the source before the dest message. Update
         * it's next and prev pointers. Update the dest prev
         * pointer.
         */
        pqmsgT = pqmsgS;
        pqmsgS = pqmsgS->pqmsgNext;
        pqmsgT->pqmsgNext = *ppqmsgD;

        if (*ppqmsgD != NULL) {
            pqmsgT->pqmsgPrev = (*ppqmsgD)->pqmsgPrev;
            (*ppqmsgD)->pqmsgPrev = pqmsgT;
        } else {
            pqmsgT->pqmsgPrev = pmlInput->pqmsgWriteLast;
            pmlInput->pqmsgWriteLast = pqmsgT;
        }
        *ppqmsgD = pqmsgT;
        ppqmsgD = &pqmsgT->pqmsgNext;
        pmlInput->cMsgs++;

        /*
         * If the thread has an event message, make sure it's going to wake
         * up to process it. The QS_EVENT flag might not be set if the thread
         * previously found an event message for another thread and passed
         * control over to him.
         */
        if (pqmsgT->dwQEvent != 0 && !(ptiT->pcti->fsWakeBits & QS_EVENT)) {
            SetWakeBit(ptiT, QS_EVENTSET);
        }

        /*
         * Preserve the 'idSysPeek' from the old queue.
         * Carefull if the redistributed queue is the same as ptiT->pq.
         */
        if (pqmsgT == (PQMSG)(pqRedist->idSysPeek) && (pqRedist != ptiT->pq)) {

            if (ptiT->pq->idSysPeek == 0) {
                CheckPtiSysPeek(6, ptiT->pq, pqRedist->idSysPeek);
                ptiT->pq->idSysPeek = pqRedist->idSysPeek;
            }

#if DEBUGTAGS
            else {
                TAGMSG2(DBGTAG_SysPeek,
                        "idSysPeek %#p already set in pq %#p",
                        ptiT->pq->idSysPeek, ptiT->pq);

            }
#endif
            /*
             * Set the 'idSysPeek' of this queue to 0 since
             * we moved the idSysPeek to other queue
             */
            CheckPtiSysPeek(7, pqRedist, 0);
            pqRedist->idSysPeek = 0;

            /*
             * Preserve also 'ptiSysLock'.
             * Set ptiSysLock to the ptiT->pq only if it points to
             * that queue.
             */
            if (ptiT->pq->ptiSysLock == NULL &&
                pqRedist->ptiSysLock != NULL &&
                pqRedist->ptiSysLock->pq == ptiT->pq) {

                CheckSysLock(4, ptiT->pq, pqRedist->ptiSysLock);
                ptiT->pq->ptiSysLock = pqRedist->ptiSysLock;

                CheckSysLock(5, pqRedist, NULL);
                pqRedist->ptiSysLock = NULL;
            }
#if DEBUGTAGS
            else {
                TAGMSG2(DBGTAG_SysPeek,
                        "ptiSysLock %#p already set in pq %#p\n",
                        ptiT->pq->ptiSysLock, ptiT->pq);
            }
#endif
        }

        /*
         * Don't want the prev pointer on our message list to point
         * to this message which is on a different list (doesn't
         * really matter because we're about to link it anyway,
         * but completeness shouldn't hurt).
         */
        if (pqmsgS != NULL) {
            pqmsgS->pqmsgPrev = NULL;
        }
    }
}

/***************************************************************************\
* CancelInputState
*
* This routine takes a queue and "cancels" input state in it - i.e., if the
* app thinks it is active, make it think it is not active, etc.
*
* 12-10-92 ScottLu      Created.
\***************************************************************************/

VOID CancelInputState(
    PTHREADINFO pti,
    DWORD cmd)
{
    PTHREADINFO ptiCurrent = PtiCurrent();
    PWND pwndT;
    TL tlpwndT;
    TL tlpwndChild;
    AAS aas;

    /*
     * In all cases, do not leave do any send messages or any callbacks!
     * This is because this code is called from
     * SetWindowsHook(WH_JOURNALPLAYBACK | WH_JOURNALRECORD). No app currently
     * calling this routine expects to be called before this routine returns.
     * (If you do callback before it returns, you'll break at least Access
     * for Windows). - scottlu
     */
    switch (cmd) {
    case CANCEL_ACTIVESTATE:
        /*
         * Active state.
         */
        pwndT = pti->pq->spwndActive;
        ThreadLockWithPti(ptiCurrent, pwndT, &tlpwndT);

        QueueNotifyMessage(pwndT, WM_NCACTIVATE, FALSE, 0);
        QueueNotifyMessage(pwndT, WM_ACTIVATE,
                MAKELONG(WA_INACTIVE, TestWF(pwndT, WFMINIMIZED)),
                0);

        if (pwndT == pti->pq->spwndActive)
            Unlock(&pti->pq->spwndActive);

        aas.ptiNotify = GETPTI(pwndT);
        aas.tidActDeact = TIDq(GETPTI(pwndT));
        aas.fActivating = FALSE;
        aas.fQueueNotify = TRUE;

        /*
         * Even though this in an xxx call, it does NOT leave any critical
         * sections (because fQueueNotify is TRUE).
         */
        ThreadLockWithPti(ptiCurrent, GETPTI(pwndT)->rpdesk->pDeskInfo->spwnd->spwndChild, &tlpwndChild);
        xxxInternalEnumWindow(GETPTI(pwndT)->rpdesk->pDeskInfo->spwnd->spwndChild,
                (WNDENUMPROC_PWND)xxxActivateApp, (LPARAM)&aas, BWL_ENUMLIST);
        ThreadUnlock(&tlpwndChild);

        ThreadUnlock(&tlpwndT);
        break;

    case CANCEL_FOCUSSTATE:
        /*
         * Focus state.
         */
        pwndT = pti->pq->spwndFocus;
        ThreadLockWithPti(ptiCurrent, pwndT, &tlpwndT);

        QueueNotifyMessage(pwndT, WM_KILLFOCUS, 0, 0);
#ifdef FE_IME
        if (IS_IME_ENABLED()) {
            /*
             * Even though this in an xxx call, it does NOT leave any
             * critical section (because fQueueMsg is TRUE).
             */
            xxxFocusSetInputContext(pwndT, FALSE, TRUE);
        }
#endif
        if (pwndT == pti->pq->spwndFocus)
            Unlock(&pti->pq->spwndFocus);

        ThreadUnlock(&tlpwndT);
        break;

    case CANCEL_CAPTURESTATE:
        /*
         * Capture state.
         */

        /*
         * We shouldn't be nuking the capture of a modal menu mode.
         */
        UserAssert((pti->pMenuState == NULL)
                    || pti->pMenuState->fModelessMenu
                    || pti->pMenuState->fInDoDragDrop);

        pti->pq->QF_flags &= ~QF_CAPTURELOCKED;
        pwndT = pti->pq->spwndCapture;
        ThreadLockWithPti(ptiCurrent, pwndT, &tlpwndT);

        QueueNotifyMessage(pwndT, WM_CANCELMODE, 0, 0);
        if (pwndT == pti->pq->spwndCapture)
            UnlockCaptureWindow(pti->pq);

        ThreadUnlock(&tlpwndT);
        break;
    }

    return;
}

/***************************************************************************\
* DBGValidateQueueStates
*
* Verifies that all queues point to stuff owned by a thread attached to
*  the queue.
*
* 07/29/97 GerardoB     Created
\***************************************************************************/
#if DBG
#define VALIDATEQSPWND(spwnd) \
        if (pq-> ## spwnd != NULL) { \
            ptiwnd = GETPTI(pq-> ## spwnd); \
            fDestroyedOK = (TestWF(pq-> ## spwnd, WFDESTROYED) && (ptiwnd == gptiRit)); \
            UserAssert((pti->rpdesk == ptiwnd->rpdesk) || fDestroyedOK); \
            UserAssert((pti == ptiwnd) \
                        || (fAttached && (pq == ptiwnd->pq)) \
                        || fDestroyedOK); \
        }


void DBGValidateQueueStates (PDESKTOP pdesk)
{
    BOOL fAttached, fDestroyedOK;
    PQ pq;
    PLIST_ENTRY pHead, pEntry;
    PTHREADINFO pti, ptiwnd;
    DWORD dwInForeground = 0;

    UserAssert((gpqForeground == NULL)
                || ((gpqForeground->ptiMouse->rpdesk == grpdeskRitInput)
                    && (gpqForeground->cThreads != 0)));

    if (pdesk == NULL) {
        RIPMSG0(RIP_WARNING, "DBGValidateQueueStates: Null pdesk parameter");
        return;
    }
    pHead = &pdesk->PtiList;
    for (pEntry = pHead->Flink; pEntry != pHead; pEntry = pEntry->Flink) {

        pti = CONTAINING_RECORD(pEntry, THREADINFO, PtiLink);
        pq = pti->pq;
        if (pq == NULL) {
            RIPMSG2(RIP_WARNING, "DBGValidateQueueStates: Null pq. pti:%#p. pdesk:%#p", pti, pdesk);
            continue;
        }
        /*
         * The queue should have a non-null cThreads excepting when it is
         * QF_INDESTROY
         */
        if (!(pq->QF_flags & QF_INDESTROY)) {
            UserAssert(pq->cThreads != 0);
        }
        fAttached = (pq->cThreads > 1);
        if (pti->pq == gpqForeground) {
            dwInForeground++;
        }
        /*
         * pti's
         */
        UserAssert((pti == pq->ptiMouse)
                    || (fAttached && (pq == pq->ptiMouse->pq)));
        UserAssert(pti->rpdesk == pq->ptiMouse->rpdesk);
        UserAssert((pti == pq->ptiKeyboard)
                    || (fAttached && (pq == pq->ptiKeyboard->pq)));
        UserAssert(pti->rpdesk == pq->ptiKeyboard->rpdesk);
        if (pq->ptiSysLock != NULL) {
            UserAssert((pti == pq->ptiSysLock)
                        || (fAttached && (pq == pq->ptiSysLock->pq)));
        }
        /*
         * pwnd's.
         */
        VALIDATEQSPWND(spwndActive);
        VALIDATEQSPWND(spwndFocus);
        VALIDATEQSPWND(spwndCapture);
        VALIDATEQSPWND(spwndActivePrev);
    }

    UserAssert((gpqForeground == NULL)
                || (dwInForeground == 0)
                || (gpqForeground->cThreads == dwInForeground));
}

/***************************************************************************\
* DBGValidateQueue
*
* Verifies that queue is readable and fields are valid.
*
* 02-Sep-1999 JerrySh   Created.
\***************************************************************************/
void DBGValidateQueue(PQ pq)
{
    if (pq != NULL) {
        Q q = *pq;
        UserAssert(q.spwndActive == HtoP(PtoH(q.spwndActive)));
        UserAssert(q.spwndFocus == HtoP(PtoH(q.spwndFocus)));
        UserAssert(q.spwndCapture == HtoP(PtoH(q.spwndCapture)));
        UserAssert(q.spwndActivePrev == HtoP(PtoH(q.spwndActivePrev)));
        UserAssert(q.spcurCurrent == HtoP(PtoH(q.spcurCurrent)));
    }
}
#endif /* DBG */
/***************************************************************************\
* zzzAttachThreadInput (API)
* zzzReattachThreads
* zzzAttachToQueue
* CheckTransferState
*
* Attaches a given thread to another input queue, either by attaching to
* a queue (referenced by another thread id), or detaching from one.
*
* 12-09-92  ScottLu     Created.
\***************************************************************************/

#define CTS_DONOTHING 0
#define CTS_CANCELOLD 1
#define CTS_TRANSFER  2

DWORD CheckTransferState(
    PTHREADINFO pti,
    PQ pqAttach,
    LONG offset,
    BOOL fJoiningForeground)
{
    PWND pwndOld, pwndNew, pwndForegroundState;

    /*
     * return 0: do nothing.
     * return 1: cancel the old state.
     * return 2: transfer the old state to the new state
     */
    pwndOld = *(PWND *)(((BYTE *)pti->pq) + offset);
    pwndNew = *(PWND *)(((BYTE *)pqAttach) + offset);

    /*
     * Make sure the old state even exists, and that the old state is
     * owned by this thread. If not, nothing happens.
     */
    if (pwndOld == NULL || GETPTI(pwndOld) != pti)
        return CTS_DONOTHING;

    /*
     * If the new state already exists, cancel the old state.
     */
    if (pwndNew != NULL)
        return CTS_CANCELOLD;

    /*
     * Transfer this old state if this thread is not joining the foreground.
     */
    if (gpqForeground == NULL || !fJoiningForeground)
        return CTS_TRANSFER;

    /*
     * We're joining the foreground - only transfer the old state if we own
     * that foreground state or if there is no foreground state.
     */
    pwndForegroundState = *(PWND *)(((BYTE *)gpqForeground) + offset);
    if (pwndForegroundState == NULL || pwndOld == pwndForegroundState)
        return CTS_TRANSFER;

    /*
     * We're joining the foreground but we didn't set that foreground state.
     * Don't allow the transfer of that state.
     */
    return CTS_CANCELOLD;
}

void zzzAttachToQueue(
    PTHREADINFO pti,
    PQ   pqAttach,
    PQ   pqJournal,
    BOOL fJoiningForeground)
{
    PQMSG pqmsgT;
    PQ pqDestroy;

    /*
     * Check active state.
     */
    switch (CheckTransferState(pti, pqAttach,
            FIELD_OFFSET(Q, spwndActive), fJoiningForeground)) {
    case CTS_CANCELOLD:
        CancelInputState(pti, CANCEL_ACTIVESTATE);
        break;

    case CTS_TRANSFER:
        Lock(&pqAttach->spwndActive, pti->pq->spwndActive);
        Unlock(&pti->pq->spwndActive);

        /*
         * The caret usually follows the focus window, which follows
         * the active window...
         */
        if (pti->pq->caret.spwnd != NULL) {

            if (GETPTI(pti->pq->caret.spwnd) == pti) {
                /*
                 * Just copy the entire caret structure... that way we
                 * don't need to deal with locking/unlocking the spwnd.
                 */
                if (pqAttach->caret.spwnd == NULL) {
                    pqAttach->caret = pti->pq->caret;
                    pti->pq->caret.spwnd = NULL;
                }
            }
        }
        break;
    }

    /*
     * Check focus state.
     */
    switch (CheckTransferState(pti, pqAttach,
            FIELD_OFFSET(Q, spwndFocus), fJoiningForeground)) {
    case CTS_CANCELOLD:
        CancelInputState(pti, CANCEL_FOCUSSTATE);
        break;

    case CTS_TRANSFER:
        Lock(&pqAttach->spwndFocus, pti->pq->spwndFocus);
        Unlock(&pti->pq->spwndFocus);
        break;
    }

    /*
     * Check capture state.
     */
    switch (CheckTransferState(pti, pqAttach,
            FIELD_OFFSET(Q, spwndCapture), fJoiningForeground)) {
    case CTS_CANCELOLD:
        CancelInputState(pti, CANCEL_CAPTURESTATE);
        break;

    case CTS_TRANSFER:
        LockCaptureWindow(pqAttach, pti->pq->spwndCapture);
        UnlockCaptureWindow(pti->pq);
        pqAttach->codeCapture = pti->pq->codeCapture;
        pqAttach->QF_flags ^= ((pqAttach->QF_flags ^ pti->pq->QF_flags) & QF_CAPTURELOCKED);
        break;

#if DBG
    case CTS_DONOTHING:
        /*
         * We should always transfer the capture state of a thread
         *  in modal menu mode
         */
        UserAssert((pti->pMenuState == NULL)
                    || ExitMenuLoop(pti->pMenuState, pti->pMenuState->pGlobalPopupMenu)
                    || pti->pMenuState->fModelessMenu
                    || pti->pMenuState->fInDoDragDrop);

        break;
#endif
    }

    /*
     * Check spwndActivePrev state.  This check has some considerations.
     * If the CTS_TRANSFER is returned, it usually means there was no
     * prev-active in the attach-queue, and it will use the first
     * window it encounters.  Since we walk the thread-list, a out-of-zorder
     * window could be chosen.  So, to counter this, we'll check the
     * attach-queue-next-prev against the thread-previous window to see
     * if it is truly the next-zorder window.
     */
    switch (CheckTransferState(pti, pqAttach,
            FIELD_OFFSET(Q, spwndActivePrev), fJoiningForeground)) {
    case CTS_TRANSFER:
        Lock(&pqAttach->spwndActivePrev, pti->pq->spwndActivePrev);
        Unlock(&pti->pq->spwndActivePrev);
        break;

    case CTS_CANCELOLD:

        /*
         * Check to see if the previous window is what we would expect it
         * to be.
         */
        if (pqAttach->spwndActive &&
            (pqAttach->spwndActivePrev && pti->pq->spwndActivePrev) &&
            (pqAttach->spwndActive->spwndNext == pti->pq->spwndActivePrev)) {

            Lock(&pqAttach->spwndActivePrev, pti->pq->spwndActivePrev);
            Unlock(&pti->pq->spwndActivePrev);
        }
        break;
    }

    if (pti == pti->pq->ptiSysLock) {
        /*
         * Preserve any of the flags we might have already been set on pqAttach.
         * Note that these flags might have been set on a previous call
        *   to this function that received the same pqAttach
         */
        pqAttach->QF_flags ^= ((pqAttach->QF_flags ^ pti->pq->QF_flags)
                                & ~(QF_CAPTURELOCKED));

        /*
         *  Fix for 29967 "Start menu disappears when clicked  and Office
         *  taskbar has focus!". Win95 uses a global counter instead of this
         *  flag. In NT when we click on Office taskbar and then on the Start
         *  Menu, MSoffice calls zzzAttachThreadInput() which changes the Start
         *  Menu's queue and the new queue has the QF_ACTIVATIONCHANGE flag on.
         *  Inside the xxxMNLoop we test if this flag is on and if it is we
         *  exit the menu.
         */
        if (!IsInsideMenuLoop(pti)) {
            pqAttach->QF_flags &= ~QF_ACTIVATIONCHANGE;
        }

        /*
         * Unlock the queue since pti is moving to anotther queue.
         */
        /* CheckSysLock(6, pq, NULL); what number? */
        pti->pq->ptiSysLock = NULL;
    }

    if (gspwndCursor != NULL && pti == GETPTI(gspwndCursor)) {
        LockQCursor(pqAttach, pti->pq->spcurCurrent);
    }

    /*
     * Each thread has its own cursor level, which is a count of the number
     * of times that app has called show/hide cursor. This gets added into
     * the queue's count for a completely accurate count every time this
     * queue recalculation is done.
     */
    /*
     * LATER
     * We need to adjust the iCursorLevel of the old queue to reflect
     * the fact that a thread is departing.
     * FritzS
     */
    pqAttach->iCursorLevel += pti->iCursorLevel;

    /*
     * Pump up the new queue with the right input variables.
     */
    pqAttach->ptiMouse    = pti;
    pqAttach->ptiKeyboard = pti;

    pqDestroy = pti->pq;

    /*
     * Don't increment the thread count here because we already incremented
     * it when we put it in pti->pqAttach. Since we're moving it from pqAttach
     * to pq, we don't mess with the reference count.
     */
    pti->pq = pqAttach;

    /*
     * If the thread is using the journal queue, leave the message list
     * alone.  Otherwise, redistribute the messages.
     */
    if (pqDestroy != pqJournal) {

        /*
         * Remember the current message list so it can get redistributed taking
         * into account ptiAttach's new queue.
         */
        pqmsgT = pqDestroy->mlInput.pqmsgRead;
        pqDestroy->mlInput.pqmsgRead      = NULL;
        pqDestroy->mlInput.pqmsgWriteLast = NULL;
        pqDestroy->mlInput.cMsgs          = 0;

        /*
         * Now redistribute the input messages from the old queue they go into the
         * right queues.
         *
         * Preserve the 'idSysPeek' when redistributing the queue
         */
        RedistributeInput(pqmsgT, pqDestroy);

        /*
         * Officially attach the new queue to this thread. Note that zzzDestroyQueue()
         * doesn't actually destroy anything until the thread reference count goes
         * to 0.
         */
        zzzDestroyQueue(pqDestroy, pti);

    } else {
        UserAssert(pqDestroy->cThreads);
        pqDestroy->cThreads--;
    }
}

BOOL zzzReattachThreads(
    BOOL fJournalAttach)
{
    PTHREADINFO ptiCurrent = PtiCurrent();
    PTHREADINFO pti;
    PQ          pqForegroundPrevNew;
    PQ          pqForegroundNew;
    PQ          pqAttach;
    PQ          pqJournal;
    PLIST_ENTRY pHead, pEntry;
    BOOL        bHadAnActiveForegroundWindow;

    /*
     * In all cases, do not leave do any send messages or any callbacks!
     * This is because this code is called from
     * SetWindowsHook(WH_JOURNALPLAYBACK | WH_JOURNALRECORD). No app currently
     * calling this routine expects to be called before this routine returns.
     * (If you do callback before it returns, you'll break at least Access
     * for Windows). - scottlu
     */

#if DBG
    DBGValidateQueueStates(ptiCurrent->rpdesk);
#endif

    /*
     * Defer Win Event notifications so we can traverse PtiList with impunity
     * Also, we don't want to callback while we're half way attached
     */
    DeferWinEventNotify();
    BEGINATOMICCHECK();

    /*
     * Don't recalc attach info if this is a journal attach, because
     * the journal attach code has already done this for us.
     */
    if (!fJournalAttach) {

        /*
         * Now recalculate all the different queue groups, based on the
         * attach requests. This fills in the pqAttach of each thread info
         * with the new queue this thread belongs to. Always takes into
         * account all attachment requests.
         */
        zzzRecalcThreadAttachment();

        /*
         * Make a guess about which queue is the journal queue.
         */
        pqJournal = gpqForeground;
        if (pqJournal == NULL)
            pqJournal = ptiCurrent->pq;

        /*
         * If the queue is only used by one thread, perform normal processing.
         */
        if (pqJournal->cThreads == 1) {
            pqJournal = NULL;
        } else {

            /*
             * Lock the queue to ensure that it stays valid
             * until we have redistributed the input.
             */
            (pqJournal->cLockCount)++;
        }
    } else {
        pqJournal = NULL;
    }

    /*
     * What will be the new foreground queue?
     */
    pqForegroundNew = NULL;

    /*
     * Remember if there is a foreground window so we don't force one
     * at the end if there wasn't one before the attach
     */
    if (gpqForeground != NULL && gpqForeground->spwndActive != NULL) {
        bHadAnActiveForegroundWindow = TRUE;
        pqForegroundNew = GETPTI(gpqForeground->spwndActive)->pqAttach;
    } else {
        bHadAnActiveForegroundWindow = FALSE;
    }

    pqForegroundPrevNew = NULL;
    if (gpqForegroundPrev != NULL && gpqForegroundPrev->spwndActivePrev != NULL) {
        pqForegroundPrevNew = GETPTI(gpqForegroundPrev->spwndActivePrev)->pqAttach;
    }


    pHead = &ptiCurrent->rpdesk->PtiList;
    for (pEntry = pHead->Flink; pEntry != pHead; pEntry = pEntry->Flink) {
        pti = CONTAINING_RECORD(pEntry, THREADINFO, PtiLink);

        if (pti->pqAttach == pti->pq) {
            pti->pqAttach = NULL;
        } else if(pti->pqAttach != NULL) {
            /*
             * It is crucial that we NULL out pqAttach for this queue once
             * we have it in a local variable because the NULL-ness of this
             * field is checked in attach operations.
             */
            pqAttach = pti->pqAttach;
            pti->pqAttach = NULL;

            zzzAttachToQueue(pti, pqAttach, pqJournal, pqForegroundNew == pqAttach);
        }

    }

    /*
     * If we are doing a journal detach, redistribute the input messages
     * from the old queue.
     */
    if (pqJournal != NULL) {
        PQMSG pqmsgRedist;

        UserAssert(pqJournal->cLockCount);
        (pqJournal->cLockCount)--;
        pqmsgRedist = pqJournal->mlInput.pqmsgRead;

        pqJournal->mlInput.pqmsgRead      = NULL;
        pqJournal->mlInput.pqmsgWriteLast = NULL;
        pqJournal->mlInput.cMsgs          = 0;
        RedistributeInput(pqmsgRedist, pqJournal);

        /*
         * Only destroy the queue if it is no longer is use.
         */
        if (pqJournal->cThreads == 0) {
            pqJournal->cThreads = 1;    // prevent underflow
            zzzDestroyQueue(pqJournal, pti); // DeferWinEventNotify() ?? IANJA ??
        } else {
            /*
             * Make sure that this queue doesn't point to a pti
             *  no longer attached to it.
             * Hopefully we'll go to zzzDestroyQueue only once
             * Increment cThreads so the queue won't be destroyed
             *  but we'll simply reassign the pti fields.
             */
            if ((pqJournal->ptiMouse != NULL)
                    && (pqJournal != pqJournal->ptiMouse->pq)) {
                pqJournal->cThreads++;
                zzzDestroyQueue(pqJournal, pqJournal->ptiMouse);
            }
            if ((pqJournal->ptiKeyboard != NULL)
                    && (pqJournal != pqJournal->ptiKeyboard->pq)) {
                pqJournal->cThreads++;
                zzzDestroyQueue(pqJournal, pqJournal->ptiKeyboard);
            }
        }
    }

#if DBG
    DBGValidateQueueStates(ptiCurrent->rpdesk);
#endif

    /*
     * If the current thread is not on the active desktop, do not
     * change the global foreground state.
     */
    if (PtiCurrent()->rpdesk != grpdeskRitInput) {
        EXITATOMICCHECK();
        zzzEndDeferWinEventNotify();
        return TRUE;
    }

    /*
     * We're done attaching. gptiForeground hasn't changed... but
     * gpqForeground has!  Try not to leave NULL as the foreground.
     */
#if DBG
    DBGValidateQueue(pqForegroundNew);
#endif
    gpqForeground = pqForegroundNew;
    // So we can Alt-Esc xxxNextWindow without an AV
    // If we have a non-NULL gpqForeground, its kbd input thread better have an rpdesk!
    UserAssert(!gpqForeground || (gpqForeground->ptiKeyboard && gpqForeground->ptiKeyboard->rpdesk));
    gpqForegroundPrev = pqForegroundPrevNew;

    ENDATOMICCHECK();
    zzzEndDeferWinEventNotify();

    if ((gpqForeground == NULL) && (bHadAnActiveForegroundWindow))  {
        PWND pwndNewForeground;
        PTHREADINFO pti = PtiCurrent();

        pwndNewForeground = _GetNextQueueWindow(pti->rpdesk->pDeskInfo->spwnd->spwndChild, 0, FALSE);

        /*
         * Don't use xxxSetForegroundWindow2 because we must not leave
         * the critical section.  There is no currently active foreground
         * so all that is needed is to post an activate event to the
         * new foreground queue.
         */
        if (pwndNewForeground != NULL) {
            PostEventMessage(GETPTI(pwndNewForeground),
                    GETPTI(pwndNewForeground)->pq, QEVENT_ACTIVATE, NULL, 0,
                    0, (LPARAM)HWq(pwndNewForeground));
        }
    }
    zzzSetFMouseMoved();
    UserAssert((gpqForeground == NULL) || (gpqForeground->ptiMouse->rpdesk == grpdeskRitInput));

    return TRUE;
}

BOOL zzzAttachThreadInput(
    PTHREADINFO ptiAttach,
    PTHREADINFO ptiAttachTo,
    BOOL fAttach)
{
    CheckCritIn();

    /*
     * Attaching to yourself doesn't make any sense.
     */
    if (ptiAttach == ptiAttachTo)
        return FALSE;

#if defined(FE_IME)
    /*
     * For console IME issue
     *
     * Console IME do attach to console input thread message queue.
     * So needs share message queue for synchronize a key state.
     */
    if (IS_IME_ENABLED()) {
        PTHREADINFO ptiConsoleIME;
        PTHREADINFO ptiConsoleInput;

        if ( ((ptiConsoleIME   = PtiFromThreadId(ptiAttach->rpdesk->dwConsoleIMEThreadId)) != NULL) &&
             ((ptiConsoleInput = PtiFromThreadId(ptiAttach->rpdesk->dwConsoleThreadId)) != NULL)    &&
             (ptiAttach == ptiConsoleIME)     &&
             (ptiAttachTo == ptiConsoleInput) &&
             (ptiConsoleIME->TIF_flags & TIF_DONTATTACHQUEUE)
           )
        {
            goto SkipCheck;
        }
    }
#endif
    /*
     * Will this thread allow attaching? Shell threads and system threads
     * won't allow attaching.
     */
    if (ptiAttachTo->TIF_flags & TIF_DONTATTACHQUEUE)
        return FALSE;
    if (ptiAttach->TIF_flags & TIF_DONTATTACHQUEUE)
        return FALSE;

#if defined(FE_IME)
SkipCheck:
#endif
    /*
     * Don't allow attaching across desktops, either.
     */
    if (ptiAttachTo->rpdesk != ptiAttach->rpdesk)
        return FALSE;

    /*
     * If attaching, make a new attachinfo structure for this thread.
     * If not attaching, remove an existing attach reference.
     */
    if (fAttach) {
        PATTACHINFO pai;

         /*
         * Alloc a new attachinfo struct, fill it in, link it in.
         */
        if ((pai = (PATTACHINFO)UserAllocPool(sizeof(ATTACHINFO), TAG_ATTACHINFO)) == NULL)
            return FALSE;

        pai->pti1 = ptiAttach;
        pai->pti2 = ptiAttachTo;;
        pai->paiNext = gpai;
        gpai = pai;
    } else {
        PATTACHINFO *ppai;
        BOOL fFound = FALSE;

        /*
         * Search for this attachinfo struct. If we can't find it, fail.
         * If we do find it, unlink it and free it.
         */
        for (ppai = &gpai; (*ppai) != NULL; ppai = &(*ppai)->paiNext) {
            if (((*ppai)->pti2 == ptiAttachTo) && ((*ppai)->pti1 == ptiAttach)) {
                PATTACHINFO paiKill = *ppai;
                fFound = TRUE;
                *ppai = (*ppai)->paiNext;
                UserFreePool((HLOCAL)paiKill);
                break;
            }
        }

        /*
         * If we couldn't find this reference, then fail.
         */
        if (!fFound) {
            return FALSE;
        }
    }

    /*
     * Now do the actual reattachment work for all threads - unless we're
     * journalling. If we did by mistake do attachment while journalling
     * was occuring, journalling would be hosed because journalling requires
     * all threads to be attached - but it is also treated as a special
     * case so it doesn't affect the ATTACHINFO structures. Therefore
     * recalcing attach info based on ATTACHINFO structures would break
     * the attachment required for journalling.
     */
    if (!FJOURNALRECORD() && !FJOURNALPLAYBACK())
        return zzzReattachThreads(FALSE);

    return TRUE;
}

/***************************************************************************\
* _SetMessageExtraInfo (API)
*
* History:
* 1-May-1995 FritzS
\***************************************************************************/

LONG_PTR _SetMessageExtraInfo(LONG_PTR lData)
{
    LONG_PTR lRet;
    PTHREADINFO pti = PtiCurrent();

    lRet = pti->pq->ExtraInfo;
    pti->pq->ExtraInfo = lData;
    return lRet;
}
