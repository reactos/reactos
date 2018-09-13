/****************************** Module Header ******************************\
* Module Name: timers.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the user timer APIs and support routines.
*
* History:
* 12-Nov-1990 DarrinM   Created.
* 08-Apr-1992 DarrinM   Switched to PM/Win3-like ScanTimers model.
\***************************************************************************/

#define _TIMERS 1      // uses a LARGE_INTEGER
#include "precomp.h"
#pragma hdrstop


/*
 * Make sure that if we return a timer id that it is a WORD value. This
 * will ensure that WOW doesn't need to handle-translate return values
 * from SetTimer().
 *
 * Start with a large number so that FindTimer() doesn't find a timer we
 * calculated with a low cTimerId if the app happens to pass in NULL pwnd
 * and a low id (like 1).
 */
#define TIMERID_MAX   0x7FFF
#define TIMERID_MIN   0x100

#define ELAPSED_MAX  0x7FFFFFFF

#define SYSRIT_TIMER  (TMRF_SYSTEM | TMRF_RIT)

WORD cTimerId = TIMERID_MAX;

/***************************************************************************\
* _SetTimer (API)
*
* This API will start the specified timer.
*
* History:
* 15-Nov-1990 DavidPe   Created.
\***************************************************************************/

UINT_PTR _SetTimer(
    PWND         pwnd,
    UINT_PTR     nIDEvent,
    UINT         dwElapse,
    TIMERPROC_PWND pTimerFunc)
{
    /*
     * Prevent apps from setting a Timer with a window proc to another app
     */
    if (pwnd && (PpiCurrent() != GETPTI(pwnd)->ppi)) {

        RIPERR1(ERROR_ACCESS_DENIED,
                RIP_WARNING,
                "Calling SetTimer with window of another process %lX",
                pwnd);

        return 0;
    }

    return InternalSetTimer(pwnd, nIDEvent, dwElapse, pTimerFunc, 0);
}

/***************************************************************************\
* _SetSystemTimer
*
* This API will start start a system timer which will generate WM_SYSTIMER
* messages rather than WM_TIMER
*
* History:
* 15-Nov-1990 DavidPe   Created.
* 21-Jan-1991 IanJa     Prefix '_' denotes export function (not API)
\***************************************************************************/

UINT_PTR _SetSystemTimer(
    PWND         pwnd,
    UINT_PTR     nIDEvent,
    DWORD        dwElapse,
    TIMERPROC_PWND pTimerFunc)
{
    /*
     * Prevent apps from setting a Timer with a window proc to another app
     */
    if (pwnd && PpiCurrent() != GETPTI(pwnd)->ppi) {

        RIPERR1(ERROR_ACCESS_DENIED,
                RIP_WARNING,
                "Calling SetSystemTimer with window of another process %lX",
                pwnd);

        return 0;
    }

    return InternalSetTimer(pwnd, nIDEvent, dwElapse, pTimerFunc, TMRF_SYSTEM);
}

/***************************************************************************\
* FreeTimer
*
* This function does the actual unlinking and freeing of the timer structure.
* I pulled it out of FindTimer() so it could be shared with DestroyQueues-
* Timers.
* Sets *pptmr to point to the next TIMER struct (NULL if none)
*
* History:
* 15-Feb-1991 DarrinM   Pulled from FindTimer().
\***************************************************************************/

VOID FreeTimer(
    PTIMER ptmr) {

    CheckCritIn();

    /*
     * Mark it for destruction.  If it the object is locked it can't
     * be freed right now.
     */
    if (!HMMarkObjectDestroy((PVOID)ptmr))
        return;

    /*
     * If this timer was just about to be processed, decrement
     * the ready-count since we're blowing it off.
     */
    if (ptmr->flags & TMRF_READY)
        DecTimerCount(ptmr->pti);

    /*
     * Unlock the window
     */
    Unlock(&ptmr->spwnd);

    /*
     * Unlink this timer
     */
    if (ptmr->ptmrPrev) {
        ptmr->ptmrPrev->ptmrNext = ptmr->ptmrNext;
    } else {
        gptmrFirst = ptmr->ptmrNext;
    }

    if (ptmr->ptmrNext) {
        ptmr->ptmrNext->ptmrPrev = ptmr->ptmrPrev;
    }

    /*
     * Free up the TIMER structure.
     */
    HMFreeObject((PVOID)ptmr);
}


/***************************************************************************\
* FindTimer
*
* This function will find a timer that matches the parameters.  We also
* deal with killing timers here since it's easier to remove items from
* the list while we're scanning it.
*
* History:
* 15-Nov-1990 DavidPe   Created.
\***************************************************************************/

PTIMER FindTimer(
    PWND pwnd,
    UINT_PTR nID,
    UINT flags,
    BOOL fKill)
{
    PTIMER ptmr;

    ptmr = gptmrFirst;

    while (ptmr != NULL) {

        /*
         * Is this the timer we're looking for?
         */
        if ((ptmr->spwnd == pwnd) &&
            (ptmr->nID == nID)    &&
            (ptmr->flags & SYSRIT_TIMER) == (flags & SYSRIT_TIMER)) {

            /*
             * Are we being called from KillTimer()? If so, destroy the
             * timer.  return != 0 because *pptmr is gone.
             */
            if (fKill) {
                FreeTimer(ptmr);
                return (PTIMER)TRUE;
            }

            /*
             * Found the timer, break out of the loop.
             */
            break;
        }

        /*
         * No, try the next one.
         */
        ptmr = ptmr->ptmrNext;
    }

    return ptmr;
}

/***************************************************************************\
* InternalSetTimer
*
* This is the guts of SetTimer that actually gets things going.
*
* NOTE (darrinm): Technically there is a bit of latency (the time it takes
* between SetTimer's NtSetEvent and when the RIT wakes up and calls ScanTimers)
* between when SetTimer is called and when the counter starts counting down.
* This is uncool but it should be a very short amount of time because the RIT
* is high-priority.  If it becomes a problem I know how to fix it.
*
* History:
* 15-Nov-1990 DavidPe      Created.
\***************************************************************************/

UINT_PTR InternalSetTimer(
    PWND         pwnd,
    UINT_PTR     nIDEvent,
    UINT         dwElapse,
    TIMERPROC_PWND pTimerFunc,
    UINT         flags)
{
    LARGE_INTEGER liT = {1, 0};
    PTIMER        ptmr;
    PTHREADINFO   ptiCurrent;

    CheckCritIn();

    /*
     * Assert if someone tries to set a timer after InitiateWin32kCleanup
     * killed the RIT.
     */
    UserAssert(gptiRit != NULL);
    
    /*
     * 1.0 compatibility weirdness. Also, don't allow negative elapse times
     * because this'll cause ScanTimers() to generate negative elapse times
     * between timers.
     */
    if ((dwElapse == 0) || (dwElapse > ELAPSED_MAX))
        dwElapse = 1;

    /*
     * Attempt to first locate the timer, then create a new one
     * if one isn't found.
     */
    if ((ptmr = FindTimer(pwnd, nIDEvent, flags, FALSE)) == NULL) {

        /*
         * Not found.  Create a new one.
         */
        ptmr = (PTIMER)HMAllocObject(NULL, NULL, TYPE_TIMER, sizeof(TIMER));
        if (ptmr == NULL) {
            return 0;
        }

        ptmr->spwnd = NULL;

        if (pwnd == NULL) {

            WORD timerIdInitial = cTimerId;

            /*
             * Pick a unique, unused timer ID.
             */
            do {

                if (--cTimerId <= TIMERID_MIN)
                    cTimerId = TIMERID_MAX;

                if (cTimerId == timerIdInitial) {

                    /*
                     * Flat out of timers bud.
                     */
                    HMFreeObject(ptmr);
                    return 0;
                }

            } while (FindTimer(NULL, cTimerId, flags, FALSE) != NULL);

            ptmr->nID = (UINT)cTimerId;

        } else {
            ptmr->nID = nIDEvent;
        }

        /*
         * Link the new timer into the front of the list.
         * Handily this works even when gptmrFirst is NULL.
         */
        ptmr->ptmrNext = gptmrFirst;
        ptmr->ptmrPrev = NULL;
        if (gptmrFirst)
            gptmrFirst->ptmrPrev = ptmr;
        gptmrFirst = ptmr;

    } else {

        /*
         * If this timer was just about to be processed,
         * decrement cTimersReady since we're resetting it.
         */
        if (ptmr->flags & TMRF_READY)
            DecTimerCount(ptmr->pti);
    }

    /*
     * If pwnd is NULL, create a unique id by
     * using the timer handle.  RIT timers are 'owned' by the RIT pti
     * so they are not deleted when the creating pti dies.
     *
     * We used to record the pti as the pti of the window if one was
     * specified.  This is not what Win 3.1 does and it broke 10862
     * where some merge app was setting the timer on winword's window
     * it it still expected to get the messages not winword.
     *
     * MS Visual C NT was counting on this bug in the NT 3.1 so if
     * a thread sets a timer for a window in another thread in the
     * same process the timer goes off in the thread of the window.
     * You can see this by doing a build in msvcnt and the files being
     * compiled do not show up.
     */
    ptiCurrent = (PTHREADINFO)(W32GetCurrentThread()); /*
                                                        * This will be NULL
                                                        * for a non-GUI thread.
                                                        */

    if (pwnd == NULL) {

        if (flags & TMRF_RIT) {
            ptmr->pti = gptiRit;
        } else {
            ptmr->pti = ptiCurrent;
            UserAssert(ptiCurrent);
        }

    } else {

        /*
         * As enforced in the API wrappers.  We shouldn't get here
         * any other way for an app timer.
         *
         * Always use pti of the window when TMRF_PTIWINDOW is passed in.
         */
        if ((ptiCurrent->TIF_flags & TIF_16BIT) && !(flags & TMRF_PTIWINDOW)) {
            ptmr->pti = ptiCurrent;
            UserAssert(ptiCurrent);
        } else {
            ptmr->pti = GETPTI(pwnd);
        }
    }

    /*
     * Initialize the timer-struct.
     *
     * NOTE: The ptiOptCreator is used to identify a JOURNAL-timer.  We
     *       want to allow these timers to be destroyed when the creator
     *       thread goes away.  For other threads that create timers across
     *       threads, we do not want to destroy these timers when the
     *       creator goes away.  Currently, we're only checking for a
     *       TMRF_RIT.  However, in the future we might want to add this
     *       same check for TMRF_SYSTEM.
     */
    Lock(&(ptmr->spwnd), pwnd);

    ptmr->cmsCountdown  = ptmr->cmsRate = dwElapse;
    ptmr->flags         = flags | TMRF_INIT;
    ptmr->pfn           = pTimerFunc;
    ptmr->ptiOptCreator = (flags & TMRF_RIT ? ptiCurrent : NULL);

    /*
     * Force the RIT to scan timers.
     *
     * N.B. The following code sets the raw input thread timer to expire
     *      at the absolute time 1 which is very far into the past. This
     *      causes the timer to immediately expire before the set timer
     *      call returns.
     */
    if (ptiCurrent == gptiRit) {
        /*
         * Don't let RIT timer loop reset the master timer - we already have.
         */
        gbMasterTimerSet = TRUE;
    }

    UserAssert(gptmrMaster);
    KeSetTimer(gptmrMaster, liT, NULL);

    /*
     * Windows 3.1 returns the timer ID if non-zero, otherwise it returns 1.
     */
    return (ptmr->nID == 0 ? 1 : ptmr->nID);
}

/***************************************************************************\
* _KillTimer (API)
*
* This API will stop a timer from sending WM_TIMER messages.
*
* History:
* 15-Nov-1990 DavidPe   Created.
\***************************************************************************/

BOOL _KillTimer(
    PWND pwnd,
    UINT_PTR nIDEvent)
{
    return KillTimer2(pwnd, nIDEvent, FALSE);
}

/***************************************************************************\
* _KillSystemTimer
*
* This API will stop a system timer from sending WM_SYSTIMER messages.
*
* History:
* 15-Nov-1990 DavidPe   Created.
* 21-Jan-1991 IanJa     Prefix '_' denotes export function (not API)
\***************************************************************************/

BOOL _KillSystemTimer(
    PWND pwnd,
    UINT_PTR nIDEvent)
{
    return KillTimer2(pwnd, nIDEvent, TRUE);
}

/***************************************************************************\
* KillTimer2
*
* This is the guts of KillTimer that actually kills the timer.
*
* History:
* 15-Nov-1990 DavidPe       Created.
\***************************************************************************/

BOOL KillTimer2(
    PWND pwnd,
    UINT_PTR nIDEvent,
    BOOL fSystemTimer)
{
    /*
     * Call FindTimer() with fKill == TRUE.  This will
     * basically delete the timer.
     */
    return (FindTimer(pwnd,
                      nIDEvent,
                      (fSystemTimer ? TMRF_SYSTEM : 0),
                      TRUE) != NULL);
}

/***************************************************************************\
* DestroyQueuesTimers
*
* This function scans through all the timers and destroys any that are
* associated with the specified queue.
*
* History:
* 15-Feb-1991 DarrinM   Created.
\***************************************************************************/

VOID DestroyThreadsTimers(
    PTHREADINFO pti)
{
    PTIMER ptmr;

    ptmr = gptmrFirst;

    while (ptmr != NULL) {

        /*
         * Is this one of the timers we're looking for?  If so, destroy it.
         */
        if (ptmr->pti == pti || ptmr->ptiOptCreator == pti) {
            PTIMER ptmrNext = ptmr->ptmrNext;
            FreeTimer(ptmr);
            ptmr = ptmrNext;
        } else {
            ptmr = ptmr->ptmrNext;
        }
    }
}

/***************************************************************************\
* DestroyWindowsTimers
*
* This function scans through all the timers and destroys any that are
* associated with the specified window.
*
* History:
* 04-Jun-1991 DarrinM       Created.
\***************************************************************************/

VOID DestroyWindowsTimers(
    PWND pwnd)
{
    PTIMER ptmr;

    ptmr = gptmrFirst;

    while (ptmr != NULL) {

        /*
         * Is this one of the timers we're looking for?  If so, destroy it.
         */
        if (ptmr->spwnd == pwnd) {
            PTIMER ptmrNext = ptmr->ptmrNext;
            FreeTimer(ptmr);
            ptmr = ptmrNext;
        } else {
            ptmr = ptmr->ptmrNext;
        }
    }
}

/***************************************************************************\
* DoTimer
*
* This function gets called from xxxPeekMessage() if the QS_TIMER bit is
* set.  If this timer is okay with the filter specified the appropriate
* WM_*TIMER message will be placed in 'pmsg' and the timer will be reset.
*
* History:
* 15-Nov-1990 DavidPe   Created.
* 27-NOv-1991 DavidPe   Changed to move 'found' timers to end of list.
\***************************************************************************/

BOOL DoTimer(
    PWND pwndFilter)
{
    PTHREADINFO pti;
    PTIMER      ptmr;
    PTIMER      ptmrNext;
    PQMSG       pqmsg;

    CheckCritIn();

    pti = PtiCurrent();

    /*
     * Search for a timer that belongs to this queue.
     */
    ptmr = gptmrFirst;

    while (ptmr != NULL) {

        /*
         * Has this timer gone off and is it one we're looking for?
         */
        if ((ptmr->flags & TMRF_READY) &&
            (ptmr->pti == pti)         &&
            CheckPwndFilter(ptmr->spwnd, pwndFilter)) {

            /*
             * We found an appropriate timer. Put it in the app's queue and
             * return success.
             */
            if ((pqmsg = AllocQEntry(&pti->mlPost)) != NULL) {

                /*
                 * Store the message and set the QS_POSTMESSAGE bit so the
                 * thread knows it has a message.
                 */
                StoreQMessage(pqmsg,
                              ptmr->spwnd,
                              (UINT)((ptmr->flags & TMRF_SYSTEM) ?
                                      WM_SYSTIMER : WM_TIMER),
                              (WPARAM)ptmr->nID,
                              (LPARAM)ptmr->pfn,
                              0, 0, 0);
#ifdef REDIRECTION
                StoreQMessagePti(pqmsg, pti);
#endif // REDIRECTION
                SetWakeBit(pti, QS_POSTMESSAGE | QS_ALLPOSTMESSAGE);
            }

            /*
             * Reset this timer.
             */
            ptmr->flags &= ~TMRF_READY;
            DecTimerCount(ptmr->pti);

            /*
             * If there are other timers in the system move this timer
             * to the end of the list so other timers in for this queue
             * get a chance to go off.
             */
            ptmrNext = ptmr->ptmrNext;
            if (ptmrNext != NULL) {

                /*
                 * Remove ptmr from its place in the list.
                 */
                if (ptmr->ptmrPrev) {
                    ptmr->ptmrPrev->ptmrNext = ptmr->ptmrNext;
                } else
                    gptmrFirst = ptmr->ptmrNext;

                ptmrNext->ptmrPrev = ptmr->ptmrPrev;

                /*
                 * Move to the last TIMER of the list.
                 */
                while (ptmrNext->ptmrNext != NULL)
                    ptmrNext = ptmrNext->ptmrNext;

                /*
                 * Insert this timer at the end.
                 */
                ptmrNext->ptmrNext = ptmr;
                ptmr->ptmrPrev = ptmrNext;
                ptmr->ptmrNext     = NULL;
            }

            return TRUE;
        }

        ptmr = ptmr->ptmrNext;
    }

    return FALSE;
}

/***************************************************************************\
* DecTimerCount
*
* This routine decrements cTimersReady and clears QS_TIMER if the count
* goes down to zero.
*
* History:
* 21-Jan-1991 DavidPe   Created.
\***************************************************************************/

VOID DecTimerCount(
    PTHREADINFO pti)
{
    CheckCritIn();

    if (--pti->cTimersReady == 0)
        pti->pcti->fsWakeBits &= ~QS_TIMER;
}

/***************************************************************************\
* JournalTimer
*
*
* History:
* 04-Mar-1991 DavidPe       Created.
\***************************************************************************/

VOID JournalTimer(
    PWND  pwnd,
    UINT  message,
    UINT_PTR nID,
    LPARAM lParam)
{
    PTHREADINFO pti;

    DBG_UNREFERENCED_PARAMETER(pwnd);
    DBG_UNREFERENCED_PARAMETER(message);
    DBG_UNREFERENCED_PARAMETER(nID);

    /*
     * We've already entered the critical section.
     */
    if (pti = ((PTIMER)lParam)->ptiOptCreator)
        WakeSomeone(pti->pq, pti->pq->msgJournal, NULL);

    return;
}

/***************************************************************************\
* SetJournalTimer
*
* Sets an NT timer that goes off in 'dt' milliseconds and will wake
* up 'pti' at that time.  This is used in journal playback code to
* simulate the timing in which events were originally given to the system.
*
* History:
* 04-Mar-1991 DavidPe       Created.
\***************************************************************************/

void SetJournalTimer(
    DWORD dt,
    UINT  msgJournal)
{
    static UINT_PTR idJournal = 0;

    PtiCurrent()->pq->msgJournal = msgJournal;

    /*
     * Remember idJournal - because TMRF_ONESHOT timers stay in the timer
     * list - by remembering the idJournal, we always reuse the same timer
     * rather than creating new ones always.
     */
    idJournal = InternalSetTimer(NULL,
                                 idJournal,
                                 dt,
                                 JournalTimer,
                                 TMRF_RIT | TMRF_ONESHOT);
}

/***************************************************************************\
* StartTimers
*
* Prime the timer pump by starting the cursor restoration timer.
*
* History:
* 02-Apr-1992 DarrinM   Created.
\***************************************************************************/

UINT_PTR StartTimers(VOID)
{
    /*
     * Let GDI know that it can start settings timers on the RIT.
     */
    GreStartTimers();

    /*
     * TMRF_RIT timers are called directly from ScanTimers -- no nasty
     * thread switching for these boys.
     */
    return InternalSetTimer(NULL, 0, 1000, xxxHungAppDemon, TMRF_RIT);
}


/***************************************************************************\
* TimersProc
*
* Deal with the timers. Called from RawInputThread.
*
* History:
* 11-11-1996 CLupu   Created.
\***************************************************************************/

VOID TimersProc(
    VOID)
{
    INT            dmsSinceLast;
    LARGE_INTEGER  liT;
    PTIMER         ptmr;
    DWORD          cmsCur;

    /*
     * Calculate how long it was since the last time we
     * processed timers so we can subtract that much time
     * from each timer's countdown value.
     */
    EnterCrit();

    cmsCur = NtGetTickCount();
    dmsSinceLast = ComputePastTickDelta(cmsCur, gcmsLastTimer);
    gcmsLastTimer = cmsCur;

    /*
     * dmsNextTimer is the time delta before the next
     * timer should go off.  As we loop through the
     * timers below this will shrink to the smallest
     * cmsCountdown value in the list.
     */
    gdmsNextTimer = 0x7FFFFFFF;
    ptmr = gptmrFirst;
    gbMasterTimerSet = FALSE;
    while (ptmr != NULL) {

        /*
         * ONESHOT timers go to a WAITING state after
         * they go off. This allows us to leave them
         * in the list but keep them from going off
         * over and over.
         */
        if (ptmr->flags & TMRF_WAITING) {
            ptmr = ptmr->ptmrNext;
            continue;
        }

        /*
         * The first time we encounter a timer we don't
         * want to set it off, we just want to use it to
         * compute the shortest countdown value.
         */
        if (ptmr->flags & TMRF_INIT) {
            ptmr->flags &= ~TMRF_INIT;

        } else {
            /*
             * If this timer is going off, wake up its
             * owner.
             */
            ptmr->cmsCountdown -= dmsSinceLast;
            if (ptmr->cmsCountdown <= 0) {
                ptmr->cmsCountdown = ptmr->cmsRate;

                /*
                 * If the timer's owner hasn't handled the
                 * last time it went off yet, throw this event
                 * away.
                 */
                if (!(ptmr->flags & TMRF_READY)) {
                    /*
                     * A ONESHOT timer goes into a WAITING state
                     * until SetTimer is called again to reset it.
                     */
                    if (ptmr->flags & TMRF_ONESHOT)
                        ptmr->flags |= TMRF_WAITING;

                    /*
                     * RIT timers have the distinction of being
                     * called directly and executing serially with
                     * with incoming timer events.
                     * NOTE: RIT timers get called while we're
                     * inside the critical section.
                     */
                    if (ptmr->flags & TMRF_RIT) {
                        TL tlTimer;

                        ThreadLock(ptmr, &tlTimer);
                        /*
                         * May set gbMasterTimerSet
                         */
                        (ptmr->pfn)(NULL,
                                    WM_SYSTIMER,
                                    ptmr->nID,
                                    (LPARAM)ptmr);

                        if (HMIsMarkDestroy(ptmr)) {
                            ptmr = ptmr->ptmrNext;
                            ThreadUnlock(&tlTimer);
                            continue;
                        }
                        ThreadUnlock(&tlTimer);

                    } else {
                        ptmr->flags |= TMRF_READY;
                        ptmr->pti->cTimersReady++;
                        SetWakeBit(ptmr->pti, QS_TIMER);
                    }
                }
            }
        }

        /*
         * Remember the shortest time left of the timers.
         */
        if (ptmr->cmsCountdown < gdmsNextTimer)
            gdmsNextTimer = ptmr->cmsCountdown;

        /*
         * Advance to the next timer structure.
         */
        ptmr = ptmr->ptmrNext;
    }

    if (!gbMasterTimerSet) {
        /*
         * Time in NT should be negative to specify a relative
         * time. It's also in hundred nanosecond units so multiply
         * by 10000  to get the right value from milliseconds.
         */
        liT.QuadPart = Int32x32To64(-10000, gdmsNextTimer);
        KeSetTimer(gptmrMaster, liT, NULL);
    }

    LeaveCrit();
}

/***************************************************************************\
*  xxxSystemTimerProc()
*
*  11/15/96 GerardoB  Created
\***************************************************************************/
VOID xxxSystemTimerProc(PWND pwnd, UINT msg, UINT_PTR id, LPARAM lParam)
{
    CheckLock(pwnd);
    UNREFERENCED_PARAMETER(msg);
    UNREFERENCED_PARAMETER(id);
    UNREFERENCED_PARAMETER(lParam);

    switch (id) {
        case IDSYS_LAYER: {
            PDCE pdce;
            
            for (pdce = gpDispInfo->pdceFirst; pdce != NULL; pdce = pdce->pdceNext) {

                if (pdce->DCX_flags & (DCX_INVALID | DCX_DESTROYTHIS))
                    continue;

                if ((pdce->DCX_flags & DCX_LAYERED) && (pdce->DCX_flags & DCX_INUSE)) {
                    UpdateLayeredSprite(pdce);
                }
            }
        }
        return;

        case IDSYS_FADE:
            AnimateFade();
            return;

        case IDSYS_FLASHWND:
            xxxFlashWindow(pwnd, FLASHW_TIMERCALL, 0);
            return;

        case IDSYS_WNDTRACKING: {
            /*
             * If the active track window hasn't changed,
             *  it's time to active it.
             * spwndTrack can be NULL if it got destroyed but we haven't
             *  destroyed the timer.yet
             */
            PTHREADINFO pti = GETPTI(pwnd);
            UserAssert(TestUP(ACTIVEWINDOWTRACKING));

            if ((pti->rpdesk->spwndTrack != NULL)
                    && (pwnd == GetActiveTrackPwnd(pti->rpdesk->spwndTrack, NULL))) {

                pti->pq->QF_flags |= (QF_ACTIVEWNDTRACKING | QF_MOUSEMOVED);

#ifdef REDIRECTION
                /*
                 * Should we call the hit test hook here ?
                 */
                PushMouseMove(pti->pq, gpsi->ptCursor);
#endif // REDIRECTION
                
                SetWakeBit(pti, QS_MOUSEMOVE);
            }
        }
        break;

        case IDSYS_MOUSEHOVER: {
            PTHREADINFO pti = GETPTI(pwnd);
            PDESKTOP pdesk = pti->rpdesk;
            /*
             * If hover hasn't been canceled, the mouse is still on
             *  this window and the point is still on the rect, then
             *  it's hover time!
             */
            if ((pdesk->dwDTFlags & DF_TRACKMOUSEHOVER)
                    && (HWq(pwnd) == HWq(pdesk->spwndTrack)
                    && PtInRect(&pdesk->rcMouseHover, gpsi->ptCursor))) {

                UINT message;
                WPARAM wParam;
                POINT pt = gpsi->ptCursor;

                if (pdesk->htEx == HTCLIENT) {
                    message = WM_MOUSEHOVER;
                    wParam = (WPARAM)GetMouseKeyFlags(pti->pq);
#ifdef USE_MIRRORING
                    if (TestWF(pwnd, WEFLAYOUTRTL)) {
                        pt.x = pwnd->rcClient.right - pt.x - 1;
                    } else
#endif
                    {
                        pt.x -= pwnd->rcClient.left;
                    }
                    pt.y -= pwnd->rcClient.top;
                } else {
                    message = WM_NCMOUSEHOVER;
                    /*
                     * Map the extended hit test code to a public one.
                     */
                    wParam = (WPARAM)LOWORD(pdesk->htEx);
                    if ((wParam >= HTEXMENUFIRST) && (wParam <= HTEXMENULAST)) {
                        wParam = (WPARAM)HTMENU;
                    } else if ((wParam >= HTEXSCROLLFIRST) && (wParam <= HTEXSCROLLLAST)) {
                        wParam = (WPARAM)(HIWORD(pdesk->htEx) ? HTVSCROLL : HTHSCROLL);
                    }
                }

                _PostMessage(pwnd, message, wParam, MAKELPARAM(pt.x, pt.y));

                pdesk->dwDTFlags &= ~DF_TRACKMOUSEHOVER;
                break;
            }
        }
        return;


        default:
            RIPMSG1(RIP_ERROR, "xxxSystemTimerProc: unexpected id:%#lx", id);
            break;
    }

    /*
     * if we fell through, the timer got to go
     */
    _KillSystemTimer(pwnd, id);
    return;
}

