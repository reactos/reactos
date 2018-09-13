/****************************** Module Header ******************************\
* Module Name: taskman.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the core functions of the input sub-system
*
* History:
* 02-27-91 MikeHar      Created.
* 02-23-92 MattFe       rewrote sleeptask
* 09-07-93 DaveHart     Per-process nonpreemptive scheduler for
*                       multiple WOW VDM support.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/*
 * WakeWowTask
 *
 * if needed the wowtask is woken by setting its event
 * It is assumed that if any wow task is currently scheduled
 * that it is a waste of time to wake the specified wow task
 * since rescheduling will occur when the currently scheduled
 * wow task enters xxxSleepTask
 *
 */
VOID
WakeWowTask(
   PTHREADINFO pti
   )
{
   PWOWPROCESSINFO pwpi;

   pwpi = pti->ppi->pwpi;
   if (pwpi && !pwpi->ptiScheduled) {
       KeSetEvent(pti->pEventQueueServer, EVENT_INCREMENT, FALSE);
       }
}




/***************************************************************************\
* InsertTask
*
* This function removes a task from its old location and inserts
* in the proper prioritized location
*
* Find a place for this task such that it must be inserted
* after any task with greater or equal priority and must be
* before any task with higher priorty.  The higher the priority
* the less urgent the task.
*
* History:
* 19-Nov-1993 mikeke    Created
\***************************************************************************/

void InsertTask(
    PPROCESSINFO ppi,
    PTDB ptdbNew)
{
    PTDB *pptdb;
    PTDB ptdb;
    int nPriority;
    PWOWPROCESSINFO pwpi = ppi->pwpi;

    CheckCritIn();

    UserAssert(pwpi != NULL);

    pptdb = &pwpi->ptdbHead;
    nPriority = ptdbNew->nPriority;

    while ((ptdb = *pptdb) != NULL) {
        /*
         * Remove it from it's old location
         */
        if (ptdb == ptdbNew) {
            *pptdb = ptdbNew->ptdbNext;

            /*
             * continue to search for the place to insert it
             */
            while ((ptdb = *pptdb) != NULL) {
                if (nPriority < ptdb->nPriority) {
                    break;
                }

                pptdb = &(ptdb->ptdbNext);
            }
            break;
        }

        /*
         * if this is the place to insert continue to search for the
         * place to delete it from
         */
        if (nPriority < ptdb->nPriority) {
            do {
                if (ptdb->ptdbNext == ptdbNew) {
                    ptdb->ptdbNext = ptdbNew->ptdbNext;
                    break;
                }
                ptdb = ptdb->ptdbNext;
            } while (ptdb != NULL);
            break;
        }

        pptdb = &(ptdb->ptdbNext);
    }

    /*
     * insert the new task
     */
    ptdbNew->ptdbNext = *pptdb;
    *pptdb = ptdbNew;
}


/***************************************************************************\
* DestroyTask()
*
* History:
* 02-27-91 MikeHar      Created.
\***************************************************************************/

void DestroyTask(
    PPROCESSINFO ppi,
    PTHREADINFO ptiToRemove)
{
    PTDB ptdbToRemove = ptiToRemove->ptdb;
    PTDB ptdb;
    PTDB* pptdb;
    PWOWPROCESSINFO pwpi = ppi->pwpi;

    // try to catch #150446
    CheckCritIn();
    BEGINATOMICCHECK();

    UserAssert(pwpi != NULL);

    if (ptdbToRemove != NULL) {

        if (ptdbToRemove->TDB_Flags & TDBF_SETUP) {
              /*
               * This means that the WoW app was a setup app (checked in SetAppCompatFlags).
               * If so, the shell needs to be notified so it can clean up potential problems
               * caused by bad calls to DDE, etc.   FritzS
               */
            PDESKTOPINFO pdeskinfo = GETDESKINFO(ptiToRemove);
            if (pdeskinfo->spwndShell) {
                _PostMessage(pdeskinfo->spwndShell, DTM_SETUPAPPRAN, 0, 0);
            }
        }
        /*
         * Remove the WOW per thread info
         */
        if (ptdbToRemove->pwti) {
            PWOWTHREADINFO *ppwti = &gpwtiFirst;
            while (*ppwti != ptdbToRemove->pwti && (*ppwti)->pwtiNext != NULL) {
                ppwti = &((*ppwti)->pwtiNext);
            }
            if (*ppwti == ptdbToRemove->pwti) {
                 *ppwti = ptdbToRemove->pwti->pwtiNext;
            }
            CLOSE_PSEUDO_EVENT(&ptdbToRemove->pwti->pIdleEvent);
            UserFreePool(ptdbToRemove->pwti);
        }

        gpsi->nEvents -= ptdbToRemove->nEvents;

        /*
         * remove it from any lists
         */
        pptdb = &pwpi->ptdbHead;
        while ((ptdb = *pptdb) != NULL) {
            /*
             * Remove it from it's old location
             */
            if (ptdb == ptdbToRemove) {
                *pptdb = ptdb->ptdbNext;
                UserFreePool(ptdbToRemove);
                UserAssert(ptiToRemove->ptdb == ptdbToRemove);
                ptiToRemove->ptdb = NULL;
                break;
            }
            pptdb = &(ptdb->ptdbNext);
        }
        UserAssert(ptdb == ptdbToRemove);  // #150446 check that we actually found it
    }
    ENDATOMICCHECK(); // #150446

    /*
     * If the task being destroyed is the active task, make nobody active.
     * We will go through this code path for 32-bit threads that die while
     * Win16 threads are waiting for a SendMessage reply from them.
     */
    if (pwpi->ptiScheduled == ptiToRemove) {
        pwpi->ptiScheduled = NULL;
        ExitWowCritSect(ptiToRemove, pwpi);

        /*
         * If this active task was locked, remove lock so next guy can
         * run.
         */
        pwpi->nTaskLock = 0;


        /*
         * Wake next task with events, or wowexec to run the scheduler
         */
        if (pwpi->ptdbHead != NULL) {
            PTDB ptdb;

            for (ptdb = pwpi->ptdbHead; ptdb; ptdb = ptdb->ptdbNext) {
                if (ptdb->nEvents > 0) {
                    KeSetEvent(ptdb->pti->pEventQueueServer,
                               EVENT_INCREMENT, FALSE);
                    break;
                }
            }

            if (!ptdb) {
                KeSetEvent(pwpi->pEventWowExec, EVENT_INCREMENT, FALSE);
            }
        }
    }
    UserAssert(ptiToRemove != pwpi->CSOwningThread);

}




/***************************************************************************\
* xxxSleepTask
*
* This function puts this task to sleep and wakes the next (if any)
* deserving task.
*
* BOOL   fInputIdle  - app is going idle, may do idle hooks
* HANDLE hEvent      - if nonzero, WowExec's event (client side) for
*                      virtual HW Interrupt HotPath.
* History:
* 02-27-91 MikeHar      Created.
* 02-23-91 MattFe       rewrote
* 12-17-93 Jonle        add wowexec hotpath for VirtualInterrupts
\***************************************************************************/

BOOL xxxSleepTask(
    BOOL   fInputIdle,
    HANDLE hEvent)
{
    PTDB ptdb;
    PTHREADINFO     pti;
    PPROCESSINFO    ppi;
    PWOWPROCESSINFO pwpi;
    PSMS            psms;
    NTSTATUS Status;
    int    nHandles;
    BOOLEAN bWaitedAtLeastOnce;

    /*
     * !!!
     * ClearSendMessages assumes that this function does NOT leave the
     * critical section when called with fInputIdle==FALSE and from a
     * 32bit thread!
     */

    CheckCritIn();

    pti  = PtiCurrent();
    ppi  = pti->ppi;
    pwpi = ppi->pwpi;

    /*
     *  If this task has received a message from outside of the current
     *  wow scheduler and hasn't yet replied to the message, the scheduler
     *  will deadlock because the send\receive lock counts are updated
     *  in ReplyMessage and not in receive message. Check for this
     *  condition and do the DirectedSchedukeTask that normally occurs
     *  in ReplyMessage. 16-Feb-1995 Jonle
     */
    psms = pti->psmsCurrent;
    if (psms && psms->ptiReceiver == pti &&
            psms->ptiSender && !(psms->flags & SMF_REPLY) &&
            psms->flags & (SMF_RECEIVERBUSY | SMF_RECEIVEDMESSAGE) &&
            psms->ptiSender->TIF_flags & TIF_16BIT &&
            (pwpi != psms->ptiSender->ppi->pwpi || !(pti->TIF_flags & TIF_16BIT)) ) {
        DirectedScheduleTask(psms->ptiReceiver, psms->ptiSender, FALSE, psms);
    }


    /*
     * return immediately if we are not 16 bit (don't have a pwpi)
     */
    if (!(pti->TIF_flags & TIF_16BIT)) {
        return FALSE;
    }


    /*
     *  Deschedule the current task
     */
    if (pti == pwpi->ptiScheduled) {
        ExitWowCritSect(pti, pwpi);
        if (!pwpi->nTaskLock) {
            pwpi->ptiScheduled = NULL;
            }
        }
    UserAssert(pti != pwpi->CSOwningThread);


    /*
     *  If this is wowexec calling on WowWaitForMsgAndEvent
     *  set up the WakeMask for all messages , and check for wake
     *  bits set since the last time. Reinsert wowexec, at the end
     *  of the list so other 16 bit tasks will be scheduled first.
     */
    if (pwpi->hEventWowExecClient == hEvent) {
        InsertTask(ppi, pti->ptdb);
        pti->pcti->fsWakeMask = QS_ALLINPUT | QS_EVENT;
        if (pti->pcti->fsChangeBits & pti->pcti->fsWakeMask) {
            pti->ptdb->nEvents++;
            gpsi->nEvents++;
        }
    }


    bWaitedAtLeastOnce = FALSE;

    do {

        /*
         * If nobody is Active look for the highest priority task with
         * some events pending. if MsgWaitForMultiple call don't
         * reschedule self
         */

        if (pwpi->ptiScheduled == NULL) {
rescan:
            if (pwpi->nRecvLock >= pwpi->nSendLock) {
                for (ptdb = pwpi->ptdbHead; ptdb; ptdb = ptdb->ptdbNext) {
                    if (ptdb->nEvents > 0 &&
                            !(hEvent == HEVENT_REMOVEME && ptdb->pti == pti)) {
                        pwpi->ptiScheduled = ptdb->pti;
                        break;
                    }
                }

                if (bWaitedAtLeastOnce) {
                    //
                    // If not first entry into sleep task avoid waiting
                    // more than needed, if the curr task is now scheduled.
                    //
                    if (pwpi->ptiScheduled == pti) {
                        break;
                    }

                } else {
                    //
                    // On the first entry into sleep task input is going
                    // idle if no tasks are ready to run. Call the idle
                    // hook if there is one.
                    //
                    if (fInputIdle &&
                            pwpi->ptiScheduled == NULL &&
                            IsHooked(pti, WHF_FOREGROUNDIDLE)) {

                        /*
                         * Make this the active task so that no other
                         * task will become active while we're calling
                         * the hook.
                         */
                        pwpi->ptiScheduled = pti;
                        xxxCallHook(HC_ACTION, 0, 0, WH_FOREGROUNDIDLE);

                        /*
                         * Reset state so that no tasks are active.  We
                         * then need to rescan the task list to see if
                         * a task was scheduled during the call to the
                         * hook.  Clear the input idle flag to ensure
                         * that the hook won't be called again if there
                         * are no tasks ready to run.
                         */
                        pwpi->ptiScheduled = NULL;
                        fInputIdle = FALSE;
                        goto rescan;
                    }
                }
            }


            /*
             * If there is a task ready, wake it up.
             */
            if (pwpi->ptiScheduled != NULL) {
                KeSetEvent(pwpi->ptiScheduled->pEventQueueServer,
                           EVENT_INCREMENT,
                           FALSE
                           );

            /*
             *  There is no one to wake up, but we may have to wake
             *  wowexec to service virtual hardware interrupts
             */
            } else if (ppi->W32PF_Flags & W32PF_WAKEWOWEXEC) {
                if (pwpi->hEventWowExecClient == hEvent) {
                    pwpi->ptiScheduled = pti;
                    ppi->W32PF_Flags &= ~W32PF_WAKEWOWEXEC;
                    InsertTask(ppi, pti->ptdb);
                    EnterWowCritSect(pti, pwpi);
                    UserAssert(pti == pwpi->ptiScheduled);
                    return TRUE;
                } else {
                    KeSetEvent(pwpi->pEventWowExec, EVENT_INCREMENT, FALSE);
                }
            } else if ((pti->TIF_flags & TIF_SHAREDWOW) && !bWaitedAtLeastOnce) {
                if (pwpi->hEventWowExecClient == hEvent) {
                    /*
                     * We have to call zzzWakeInputIdle only if this will
                     * awake WowExec's thread and not other thread. Bug 44060.
                     */
                    zzzWakeInputIdle(pti); // need to DeferWinEventNotify() ?? IANJA ??
                }
            }

        } else if (pwpi->nTaskLock > 0 && pwpi->ptiScheduled == pti
                   && pti->ptdb->nEvents > 0) {
             KeSetEvent(pwpi->ptiScheduled->pEventQueueServer,
                    EVENT_INCREMENT, FALSE);
        }
        /*
         *  return if we are a 32 bit thread, or if we were called by
         *  MsgWaitForMultiple to exit the wow scheduler
         */
        if (!(pti->TIF_flags & TIF_16BIT)) {
            return FALSE;
        } else if (hEvent == HEVENT_REMOVEME) {
            InsertTask(ppi, pti->ptdb);
            KeClearEvent(pti->pEventQueueServer);
            return FALSE;
        }

        if (pti->apEvent == NULL) {
            pti->apEvent = UserAllocPoolNonPaged(POLL_EVENT_CNT * sizeof(PKEVENT), TAG_EVENT);
            if (pti->apEvent == NULL)
                return FALSE;
        }

        /*
         * Wait for input to this thread.
         */
        pti->apEvent[IEV_TASK] = pti->pEventQueueServer;

        /*
         * Add the WowExec, handle for virtual hw interrupts
         */
        if (pwpi->hEventWowExecClient == hEvent) {
            pti->apEvent[IEV_WOWEXEC] = pwpi->pEventWowExec;
            nHandles = 2;
        } else {
            nHandles = 1;
        }

        CheckForClientDeath();
        LeaveCrit();

        Status = KeWaitForMultipleObjects(nHandles,
                                          &pti->apEvent[IEV_TASK],
                                          WaitAny,
                                          WrUserRequest,
                                          UserMode,
                                          TRUE,
                                          NULL,
                                          NULL);

        CheckForClientDeath();

        EnterCrit();

        bWaitedAtLeastOnce = TRUE;

        // remember if we woke up for wowexec
        if (Status == STATUS_WAIT_1) {
            ppi->W32PF_Flags |= W32PF_WAKEWOWEXEC;
        } else if (Status == STATUS_USER_APC) {

            /*
             * An alert was received.  This should only occur when the
             * thread has been terminated.
             * ClientDeliverUserApc() delivers User-mode APCs by calling back
             * to the client and immediately returning without doing anything:
             * KeUserModeCallback will automatically deliver any pending APCs.
             */
            UserAssert(PsIsThreadTerminating(PsGetCurrentThread()));
            ClientDeliverUserApc();
        }

    } while (pwpi->ptiScheduled != pti);


    /*
     * We are the Active Task, reduce number of Events
     * Place ourselves at the far end of tasks in the same priority
     * so that next time we sleep someone else will run.
     */
    pti->ptdb->nEvents--;
    gpsi->nEvents--;
    UserAssert(gpsi->nEvents >= 0);

    InsertTask(ppi, pti->ptdb);

    ppi->W32PF_Flags &= ~W32PF_WAKEWOWEXEC;

    EnterWowCritSect(pti, pwpi);
    UserAssert(pti == pwpi->ptiScheduled);



    return FALSE;
}



/***************************************************************************\
* xxxUserYield
*
* Does exactly what Win3.1 UserYield does.
*
* History:
* 10-19-92 Scottlu      Created.
\***************************************************************************/

BOOL xxxUserYield(
    PTHREADINFO pti)
{
    PPROCESSINFO ppi = pti->ppi;

    /*
     * Deal with any pending messages. Only call it this first time if
     * this is the current running 16 bit app. In the case when starting
     * up a 16 bit app, the starter calls UserYield() to yield to the new
     * task, but at this time ppi->ptiScheduled is set to the new task.
     * Receiving messages at this point would be bad!
     */
    if (pti->TIF_flags & TIF_16BIT) {
        if (pti == ppi->pwpi->ptiScheduled) {
            xxxReceiveMessages(pti);
        }
    } else {
        xxxReceiveMessages(pti);
    }

    /*
     * If we are a 16 bit task
     * Mark our task so it comes back some time.  Also, remove it and
     * re-add it to the list so that we are the last task of our priority
     * to run.
     */
    if ((pti->TIF_flags & TIF_16BIT) && (pti->ptdb != NULL)) {
       if (pti->ptdb->nEvents == 0) {
            pti->ptdb->nEvents++;
            gpsi->nEvents++;
        }
        InsertTask(ppi, pti->ptdb);

        /*
         * Sleep.  Return right away if there are no higher priority tasks
         * in need of running.
         */
        xxxSleepTask(TRUE, NULL);

        /*
         * Deal with any that arrived since we weren't executing.
         */
        xxxReceiveMessages(pti);
    }


    return TRUE;
}


/***************************************************************************\
* DirectedScheduleTask
*
* History:
* 25-Jun-1992 mikeke    Created.
\***************************************************************************/

VOID DirectedScheduleTask(
     PTHREADINFO ptiOld,
     PTHREADINFO ptiNew,
     BOOL bSendMsg,
     PSMS psms
     )
{
    PWOWPROCESSINFO pwpiOld;
    PWOWPROCESSINFO pwpiNew;

    CheckCritIn();

    pwpiOld  = ptiOld->ppi->pwpi;
    pwpiNew  = ptiNew->ppi->pwpi;


    /*
     * If old task is 16 bit, reinsert the task in its wow scheduler list
     * so that it is lowest in priority. Note that ptiOld is always the
     * same as pwpiOld->ptiScheduled except when called from ReceiverDied.
     */
    if (ptiOld->TIF_flags & TIF_16BIT) {

        if (pwpiOld->ptiScheduled == ptiOld) {
            ptiOld->ptdb->nEvents++;
            gpsi->nEvents++;
            InsertTask(ptiOld->ppi, ptiOld->ptdb);
            }


        // Update the Send\Recv counts for interprocess scheduling in SleepTask

        if (pwpiOld != pwpiNew || !(ptiNew->TIF_flags & TIF_16BIT)) {
            if (bSendMsg) {
                pwpiOld->nSendLock++;
                psms->flags |= SMF_WOWSEND;
                }
            else if (pwpiOld->nRecvLock && psms->flags & SMF_WOWRECEIVE) {
                pwpiOld->nRecvLock--;
                psms->flags &= ~SMF_WOWRECEIVE;
                }
            }

        }


    /*
     *  If the new task is 16 bit, reinsert into the wow scheduler list
     *  so that it will run, if its a sendmsg raise priority of the receiver.
     *  If its a reply and the sender is waiting for this psms or the sender
     *  has a message to reply to raise priority of the sender.
     */
    if (ptiNew->TIF_flags & TIF_16BIT) {
        BOOL bRaisePriority;

        ptiNew->ptdb->nEvents++;
        gpsi->nEvents++;
        bRaisePriority = bSendMsg || psms == ptiNew->psmsSent;

        if (bRaisePriority) {
            ptiNew->ptdb->nPriority--;
            }

        InsertTask(ptiNew->ppi, ptiNew->ptdb);

        if (bRaisePriority) {
            ptiNew->ptdb->nPriority++;
            WakeWowTask(ptiNew);
            }


        // Update the Send\Recv counts for interprocess scheduling in SleepTask

        if (pwpiOld != pwpiNew || !(ptiOld->TIF_flags & TIF_16BIT)) {
            if (bSendMsg) {
                pwpiNew->nRecvLock++;
                psms->flags |= SMF_WOWRECEIVE;
                }
            else if (pwpiNew->nSendLock && psms->flags & SMF_WOWSEND) {
                pwpiNew->nSendLock--;
                psms->flags &= ~SMF_WOWSEND;
                }
            }

        }
}




/***************************************************************************\
* xxxDirectedYield
*
* History:
* 09-17-92 JimA         Created.
\***************************************************************************/

void xxxDirectedYield(
    DWORD dwThreadId)
{
    PTHREADINFO ptiOld;
    PTHREADINFO ptiNew;

    CheckCritIn();

    ptiOld = PtiCurrent();
    if (!(ptiOld->TIF_flags & TIF_16BIT) || !ptiOld->ppi->pwpi) {
         RIPMSG0(RIP_ERROR, "DirectedYield called from 32 bit thread!");
         return;
         }

    /*
     *  If the old task is 16 bit, reinsert the task in its wow
     *  scheduler list so that it is lowest in priority.
     */
    ptiOld->ptdb->nEvents++;
    gpsi->nEvents++;
    InsertTask(ptiOld->ppi, ptiOld->ptdb);

    /*
     * -1 supports Win 3.1 OldYield mechanics
     */
    if (dwThreadId != DY_OLDYIELD) {

        ptiNew = PtiFromThreadId(dwThreadId);
        if (ptiNew == NULL)
            return;

        if (ptiNew->TIF_flags & TIF_16BIT) {
            ptiNew->ptdb->nEvents++;
            gpsi->nEvents++;
            ptiNew->ptdb->nPriority--;
            InsertTask(ptiNew->ppi, ptiNew->ptdb);
            ptiNew->ptdb->nPriority++;
        }
    }

    xxxSleepTask(TRUE, NULL);
}


/***************************************************************************\
* CurrentTaskLock
*
* Lock the current 16 bit task into the 16 bit scheduler
*
* Parameter:
*   hlck    if NULL, lock the current 16 bit task and return a lock handle
*           if valid lock handle, unlock the current task
*
* History:
* 13-Apr-1992 jonpa      Created.
\***************************************************************************/
#if 0 /* WOW is not using this but they might some day */
DWORD CurrentTaskLock(
    DWORD hlck)
{
    PWOWPROCESSINFO pwpi = PpiCurrent()->pwpi;

    if (!pwpi)
        return 0;

    if (hlck == 0) {
        pwpi->nTaskLock++;
        return ~(DWORD)pwpi->ptiScheduled->ptdb;
    } else if ((~hlck) == (DWORD)pwpi->ptiScheduled->ptdb) {
        pwpi->nTaskLock--;
    }

    return 0;
}
#endif
