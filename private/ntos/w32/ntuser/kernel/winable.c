/**************************** Module Header ********************************\
* Module Name: winable.c
*
* This has the stuff for WinEvents:
*     NotifyWinEvent
*     _SetWinEventHook
*     UnhookWinEventHook
*
* All other additions to USER for Active Accessibility are in WINABLE2.C
* and its helper ASM file, ABLEASM.ASM.
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* History:
* Based on snapshot taken from:
*  \\trango\slmro\proj\win\src\CORE\access\user_40\user32 on 8/29/96
* 08-30-96 IanJa  Ported from Windows '95
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#if DBG
int gnNotifies = 0;
#define DBGVERIFYEVENTHOOK(peh)                                              \
        HMValidateCatHandleNoSecure(PtoH(peh), TYPE_WINEVENTHOOK);           \
        UserAssertMsg1((IsValidTag(peh, TAG_WINEVENT)), "event hook %#p: bad tag", peh); \
        UserAssertMsg1((peh->eventMin <= peh->eventMax), "event hook %#p: bad range", peh)
#define DBGVERIFYNOTIFY(pNotify)                                  \
        UserAssert(pNotify->spEventHook != NULL);                 \
        UserAssert(pNotify->spEventHook->fSync || (pNotify->dwWEFlags & WEF_ASYNC))
#else
#define DBGVERIFYEVENTHOOK(peh)
#define DBGVERIFYNOTIFY(pNotify)
#endif

/*
 * Pending Event Notifications (sync and async)
 */

static NOTIFY   notifyCache;
static BOOL     fNotifyCacheInUse = FALSE;


/*
 * Local to this module
 */
WINEVENTPROC xxxGetEventProc(PEVENTHOOK pEventOrg);
PNOTIFY CreateNotify(PEVENTHOOK peh, DWORD event, PWND pwnd, LONG idObject,
        LONG idChild, PTHREADINFO ptiEvent, DWORD dwTime);


/*****************************************************************************\
*
*  xxxProcessNotifyWinEvent()
*
*  Posts or Sends a WinEvent notification.
*  Post: uses PostEventMesage - does not leave the critical section.
*  Send: makes a callback to user-mode - does leave the critical section.
*
*  If this is a system thread (RIT, Desktop or Console) then synchronously
*  hooked (WINEVENT_INCONTEXT) events are forced to be asynchronous.
*
*  We return the next win event hook in the list.
*
\*****************************************************************************/
PEVENTHOOK
xxxProcessNotifyWinEvent(PNOTIFY pNotify)
{
    WINEVENTPROC   pfn;
    PEVENTHOOK     pEventHook;
    TL             tlpEventHook;
    PTHREADINFO    ptiCurrent = PtiCurrent();

    pEventHook = pNotify->spEventHook;
    DBGVERIFYEVENTHOOK(pEventHook);
    UserAssert(pEventHook->head.cLockObj);

    if (((pNotify->dwWEFlags & (WEF_ASYNC | WEF_POSTED)) == WEF_ASYNC)
        ||
        (ptiCurrent->TIF_flags & (TIF_SYSTEMTHREAD | TIF_CSRSSTHREAD | TIF_INCLEANUP))

        ||
        (!RtlEqualLuid(&GETPTI(pEventHook)->ppi->luidSession, &ptiCurrent->ppi->luidSession) &&
         !(ptiCurrent->TIF_flags & TIF_ALLOWOTHERACCOUNTHOOK))

        ||
        (GETPTI(pEventHook)->ppi != ptiCurrent->ppi &&
         IsRestricted(GETPTI(pEventHook)->pEThread))

#if defined(_WIN64)
        ||
        ((GETPTI(pEventHook)->TIF_flags & TIF_WOW64) != (ptiCurrent->TIF_flags & TIF_WOW64))
#endif
        ) {
        /*
         * POST
         *
         * WinEvent Hook set without WINEVENT_INCONTEXT flag are posted;
         * Events from system threads are posted because there is no user-mode
         *    part to callback to;
         * Console is not permitted to load DLLs, so we must post back to the
         *    hooking application;
         * DLLs can not be loaded cross bit type(32bit to 64bit) on 64bit NT
         *    so we must post(It may be usefull to let the app be aware and
         *    even supply both a 32bit and a 64bit DLL that are aware of each other);
         * Threads in cleanup can't get called back, so turn their
         *    notifications into async ones. (Better late than never).
         *
         * If forcing these events ASYNC is unacceptable, we might consider
         * doing system/console SYNC events like low-level hooks (sync with
         * timeout: but may have to post it if the timeout expires) - IanJa
         */
        PQ  pqReceiver = GETPTI(pEventHook)->pq;
        PEVENTHOOK pEventHookNext = pEventHook->pehNext;

        BEGINATOMICCHECK();

        DBGVERIFYNOTIFY(pNotify);
        pNotify->dwWEFlags |= WEF_POSTED | WEF_ASYNC;
        if (!pqReceiver || (GETPTI(pEventHook) == gptiRit) ||
                pEventHook->fDestroyed ||
                !PostEventMessage(GETPTI(pEventHook), pqReceiver,
                                  QEVENT_NOTIFYWINEVENT,
                                  NULL, 0, 0, (LPARAM)pNotify)) {
            /*
             * If the receiver doesn't have a queue or the
             * post failed (low memory), cleanup what we just
             * created.
             * Note: destroying the notification may destroy pEventHook too.
             */
            RIPMSG2(RIP_WARNING, "failed to post NOTIFY at %#p, time %lx\n",
                      pNotify, pNotify->dwEventTime);
            DestroyNotify(pNotify);
        }

        ENDATOMICCHECK();

        if (pEventHookNext) {
            DBGVERIFYEVENTHOOK(pEventHookNext);
        }
        return pEventHookNext;
    }

    /*
     * Don't call back if the hook has been destroyed (unhooked).
     */
    if (pEventHook->fDestroyed) {
        /*
         * Save the next hook since DestroyNotify may cause pEventHook to
         * be freed by unlocking it.
         */
        pEventHook = pEventHook->pehNext;
        DestroyNotify(pNotify);
        return pEventHook;
    }

    /*
     * CALLBACK
     *
     * This leaves the critical section.
     * We return the next Event Hook in the list so that the caller doesn't
     * have to lock pEventHook.
     */
    UserAssert((pNotify->dwWEFlags & WEF_DEFERNOTIFY) == 0);

    ThreadLockAlways(pEventHook, &tlpEventHook);

    UserAssertMsg1(pNotify->ptiReceiver == NULL,
         "pNotify %#p is already in callback!  Reentrant?", pNotify);
    pNotify->ptiReceiver = ptiCurrent;

    if (!pEventHook->fSync) {
        UserAssert(pEventHook->ihmod == -1);
        pfn = (WINEVENTPROC)pEventHook->offPfn;
    } else {
        pfn = xxxGetEventProc(pEventHook);
    }
    if (pfn) {
        xxxClientCallWinEventProc(pfn, pEventHook, pNotify);
        DBGVERIFYNOTIFY(pNotify);
        DBGVERIFYEVENTHOOK(pEventHook);
        UserAssert(pEventHook->head.cLockObj);
    }

    pNotify->ptiReceiver = NULL;

    /*
     * Save the next item in the list, ThreadUnlock() may destroy pEventHook.
     * DestroyNotify() may also kill the event if it is a zombie (destroyed
     * but being used, waiting for use count to go to 0 before being freed).
     */
    pEventHook = pEventHook->pehNext;
    ThreadUnlock(&tlpEventHook);

    /*
     * We are done with the notification.  Kill it.
     *
     * NOTE that DestroyNotify does not yield, which is why we can hang on
     * to the pehNext field above around this call.
     *
     * NOTE ALSO that DestroyNotify will kill the event it references if the
     * ref count goes down to zero and it was zombied earlier.
     */
    DestroyNotify(pNotify);

    return pEventHook;
}


/****************************************************************************\
* xxxFlushDeferredWindowEvents()
*
* Process notifications that were queued up during DeferWinEventNotify()
\****************************************************************************/
VOID
xxxFlushDeferredWindowEvents()
{
    PNOTIFY pNotify;
    DWORD idCurrentThread = W32GetCurrentTID();

    if (idCurrentThread == 0) {
        RIPMSG0(RIP_ERROR, "processing deferred notifications before we have a pti!");
        // return;
    }

    UserAssert(IsWinEventNotifyDeferredOK());

    pNotify = gpPendingNotifies;
    while (pNotify) {
        if (((pNotify->dwWEFlags & WEF_DEFERNOTIFY) == 0) ||
                (pNotify->idSenderThread != idCurrentThread)) {
            // UserAssert(pNotify->idSenderThread == idCurrentThread); // just testing!
            pNotify = pNotify->pNotifyNext;
        } else {
            /*
             * Clear WEF_DEFERNOTIFY so that if we recurse in the callback
             * we won't try to send this notification again.
             */
            pNotify->dwWEFlags &= ~WEF_DEFERNOTIFY;
#if DBG
            gnDeferredWinEvents--;
#endif
            /*
             * We shouldn't have deferred ASYNC notifications: we should have
             * posted them immediately.
             */
            UserAssert((pNotify->dwWEFlags & WEF_ASYNC) == 0);
            xxxProcessNotifyWinEvent(pNotify);
            /*
             * Start again at the head of the list, in case it munged during
             * the callback.
             */
            pNotify = gpPendingNotifies;
        }
    }
}


/*****************************************************************************\
*
* xxxWindowEvent
*
* Send, Post or Defer a Win Event notification, depending on what Win Event
* hooks are installed and what the context of the caller is.
*
* The caller should test FWINABLE() and only call xxxWindowEvent if it is TRUE,
* that way only costs a few clocks if no Win Event hooks are set.
*
* Caller shouldn't lock pwnd, because xxxWindowEvent() will do it.
*
\*****************************************************************************/
VOID
xxxWindowEvent(
    DWORD   event,
    PWND    pwnd,
    LONG    idObject,
    LONG    idChild,
    DWORD   dwFlags)
{
    PEVENTHOOK peh;
    PEVENTHOOK pehNext;
    PTHREADINFO ptiCurrent, ptiEvent;
    DWORD   dwTime;
    PPROCESSINFO ppiEvent;
    DWORD idEventThread;
    HANDLE hEventProcess;
    PNOTIFY pNotify;
    TL tlpwnd;
    TL tlpti;

    /*
     * Do not bother with CheckLock(pwnd) - we ThreadLock it below.
     */
    UserAssert(FWINABLE());

    /*
     * This thread is in startup, and has not yet had it's pti set up
     * This is pretty rare, but sometimes encountered in stress.
     * Test gptiCurrent to avoid the UserAssert(gptiCurrent) in PtiCurrent()
     */
    if (gptiCurrent == NULL) {
        RIPMSG3(RIP_WARNING, "Ignore WinEvent %lx %#p %lx... no PtiCurrent yet",
                event, pwnd, idObject);
        return;
    }
    ptiCurrent = PtiCurrent();

    /*
     * Don't bother with destroyed windows
     */
    if (pwnd && TestWF(pwnd, WFDESTROYED)) {
        RIPMSG3(RIP_WARNING,
                "Ignore WinEvent %lx %#p %lx... pwnd already destroyed",
                event, pwnd, idObject);
        return;
    }

    /*
     * Under some special circumstances we have to defer
     */
    if (ptiCurrent->TIF_flags & (TIF_DISABLEHOOKS | TIF_INCLEANUP)) {
        dwFlags |= WEF_DEFERNOTIFY;
    }

    /*
     * Determine process and thread issuing the event notification
     */
    if ((dwFlags & WEF_USEPWNDTHREAD) && pwnd) {
        ptiEvent = GETPTI(pwnd);
    } else {
        ptiEvent = ptiCurrent;
    }
    idEventThread = TIDq(ptiEvent);
    ppiEvent = ptiEvent->ppi;
    hEventProcess = ptiEvent->pEThread->Cid.UniqueProcess;

    dwTime = NtGetTickCount();

    ThreadLockWithPti(ptiCurrent, pwnd, &tlpwnd);
    ThreadLockPti(ptiCurrent, ptiEvent, &tlpti);

    /*
     * If we're not deferring the current notification process any pending
     * deferred notifications before proceeding with the current notification
     */
    if (!(dwFlags & WEF_DEFERNOTIFY)) {
        xxxFlushDeferredWindowEvents();
    }

    for (peh = gpWinEventHooks; peh; peh = pehNext) {
        DBGVERIFYEVENTHOOK(peh);
        pehNext = peh->pehNext;

        //
        // Is event in the right range?  And is it for this process/thread?
        // Note that we skip destroyed events.  They will be freed any
        // second now, it's just that yielding may have caused reentrancy.
        //
        // If the caller said to ignore events on his own thread, make sure
        // we skip them.
        //
        if (!peh->fDestroyed                &&
            (peh->eventMin <= event)        &&
            (event <= peh->eventMax)        &&
            (!peh->hEventProcess || (peh->hEventProcess == hEventProcess)) &&
            (!peh->fIgnoreOwnProcess || (ppiEvent != GETPTI(peh)->ppi)) &&
            (!peh->idEventThread || (peh->idEventThread == idEventThread))  &&
            (!peh->fIgnoreOwnThread || (ptiEvent != GETPTI(peh))) &&
            // temp fix from SP3 - best to architect events on a per-desktop
            // basis, with a separate pWinEventHook list per desktop. (IanJa)
            (peh->head.pti->rpdesk == ptiCurrent->rpdesk))
        {
            /*
             * Don't create new notifications for zombie event hooks.
             * When an event is destroyed, it stays as a zombie until the in-use
             * count goes to zero (all it's async and deferred notifies gone)
             */
            if (HMIsMarkDestroy(peh)) {
                break;
            }

            UserAssert(peh->fDestroyed == 0);

            if ((pNotify = CreateNotify(peh, event, pwnd, idObject,
                    idChild, ptiEvent, dwTime)) == NULL) {
                break;
            }
            pNotify->dwWEFlags |= dwFlags;

            /*
             * If it's async, don't defer it: post it straight away.
             */
            if (pNotify->dwWEFlags & WEF_ASYNC) {
                pNotify->dwWEFlags &= ~WEF_DEFERNOTIFY;
            }

            if (pNotify->dwWEFlags & WEF_DEFERNOTIFY) {
#if DBG
                gnDeferredWinEvents++;
#endif
                DBGVERIFYNOTIFY(pNotify);
            } else {
                pehNext = xxxProcessNotifyWinEvent(pNotify);
            }
        }
    }

    ThreadUnlockPti(ptiCurrent, &tlpti);
    ThreadUnlock(&tlpwnd);
}

/****************************************************************************\
*
* CreateNotify()
*
* Gets a pointer to a NOTIFY struct that we can then propagate to our
* event window via Send/PostMessage.  We have to do this since we want to
* (pass on a lot more data then can be packed in the parameters.
*
*  We have one cached struct so we avoid lots of allocs and frees in the
*  most common case of just one outstanding notification.
\****************************************************************************/
PNOTIFY
CreateNotify(PEVENTHOOK pEvent, DWORD event, PWND pwnd, LONG idObject,
    LONG idChild, PTHREADINFO ptiSender, DWORD dwTime)
{
    PNOTIFY pNotify;
    UserAssert(pEvent != NULL);

    //
    // Get a pointer.  From cache if available.
    // IanJa - change this to allocate from zone a la AllocQEntry??
    //
    if (!fNotifyCacheInUse) {
        fNotifyCacheInUse = TRUE;
        pNotify = &notifyCache;
#if DBG
        //
        // Make sure we aren't forgetting to set any fields.
        //
        // DebugFillBuffer(pNotify, sizeof(NOTIFY));
#endif
    } else {
        pNotify = (PNOTIFY)UserAllocPool(sizeof(NOTIFY), TAG_NOTIFY);
        if (!pNotify)
            return NULL;
    }


    /*
     * Fill in the notify block.
     */
    pNotify->spEventHook = NULL;
    Lock(&pNotify->spEventHook, pEvent);
    pNotify->hwnd = HW(pwnd);
    pNotify->event = event;
    pNotify->idObject = idObject;
    pNotify->idChild = idChild;
    pNotify->idSenderThread = TIDq(ptiSender);
    UserAssert(pNotify->idSenderThread != 0);
    pNotify->dwEventTime = dwTime;
    pNotify->dwWEFlags = pEvent->fSync ? 0 : WEF_ASYNC;
    pNotify->pNotifyNext = NULL;
    pNotify->ptiReceiver = NULL;
#if DBG
    gnNotifies++;
#endif

    /*
     * The order of non-deferred notifications doesn't matter; they are here
     * simply for cleanup/in-use tracking. However, deferred notifications must
     * be ordered with most recent at the end, so just order them all that way.
     */
    if (gpPendingNotifies) {
        UserAssert(gpLastPendingNotify);
        UserAssert(gpLastPendingNotify->pNotifyNext == NULL);
        gpLastPendingNotify->pNotifyNext = pNotify;
    } else {
        gpPendingNotifies = pNotify;
    }
    gpLastPendingNotify = pNotify;

    return pNotify;
}


/****************************************************************************\
*
*  RemoveNotify()
*
*  NOTE:  This does NOT yield.
\****************************************************************************/
VOID
RemoveNotify(PNOTIFY *ppNotify)
{
    PNOTIFY pNotifyRemove;

    pNotifyRemove = *ppNotify;

    /*
     * First, get it out of the pending list.
     */
    *ppNotify = pNotifyRemove->pNotifyNext;

#if DBG
    if (pNotifyRemove->dwWEFlags & WEF_DEFERNOTIFY) {
        UserAssert(gnDeferredWinEvents > 0);
        gnDeferredWinEvents--;
    }
#endif
    if (*ppNotify == NULL) {
        /*
         * Removing last notify, so fix up gpLastPendingNotify:
         * If list now empty, there is no last item.
         */
        if (gpPendingNotifies == NULL) {
            gpLastPendingNotify = NULL;
        } else {
            gpLastPendingNotify = CONTAINING_RECORD(ppNotify, NOTIFY, pNotifyNext);
        }
    }
    UserAssert((gpPendingNotifies == 0) || (gpPendingNotifies > (PNOTIFY)100));

    DBGVERIFYEVENTHOOK(pNotifyRemove->spEventHook);

    /*
     * This may cause the win event hook to be freed.
     */
    Unlock(&pNotifyRemove->spEventHook);

    //
    // Now free it.  Either put it back in the cache if it is the cache,
    // or really free it otherwise.
    //
    if (pNotifyRemove == &notifyCache) {
        UserAssert(fNotifyCacheInUse);
        fNotifyCacheInUse = FALSE;
    } else {
        UserFreePool(pNotifyRemove);
    }
#if DBG
    UserAssert(gnNotifies > 0);
    gnNotifies--;
#endif
}


/*****************************************************************************\
*
* DestroyNotify()
*
* NOTE:  This does NOT yield.
*
* This gets the notification out of our pending list and frees the local
* memory it uses.
*
* This function is called
* (1) NORMALLY:   After returning from calling the notify proc
* (2) CLEANUP:    When a thread goes away, we cleanup async notifies it
*     hasn't received, and sync notifies it was in the middle of trying
*     to call (i.e. the event proc faulted).
*
\*****************************************************************************/
VOID
DestroyNotify(PNOTIFY pNotifyDestroy)
{
    PNOTIFY  *ppNotify;
    PNOTIFY  pNotifyT;

    DBGVERIFYNOTIFY(pNotifyDestroy);

    /*
     * Either this notify isn't currently in the process of calling back
     * (which means ptiReceiver is NULL) or the thread destroying it
     * must be the one that was calling back (which means this thread
     * was destroyed during the callback and is cleaning up).
     */
    UserAssert((pNotifyDestroy->ptiReceiver == NULL) ||
            (pNotifyDestroy->ptiReceiver == PtiCurrent()));

    ppNotify = &gpPendingNotifies;
    while (pNotifyT = *ppNotify) {
        if (pNotifyT == pNotifyDestroy) {
            RemoveNotify(ppNotify);
            return;
        } else {
            ppNotify = &pNotifyT->pNotifyNext;
        }
    }
    RIPMSG1(RIP_ERROR, "DestroyNotify %#p - not found", pNotifyDestroy);
}



/***************************************************************************\
* FreeThreadsWinEvents
*
* During 'exit-list' processing this function is called to free any WinEvent
* notifications and WinEvent hooks created by the current thread.
*
* Notifications that remain may be:
*  o  Posted notifications (async)
*  o  Notifications in xxxClientCallWinEventProc (sync)
*  o  Deferred notifications (should be sync only)
* Destroy the sync notifications, because we cannot do callbacks
* while in thread cleanup.
* Leave the posted (async) notifications alone: they're on their way already.
*
* History:
* 11-11-96 IanJa         Created.
\***************************************************************************/

VOID
FreeThreadsWinEvents(PTHREADINFO pti)
{
    PEVENTHOOK peh, pehNext;
    PNOTIFY pn, pnNext;
    DWORD idCurrentThread = W32GetCurrentTID();

    /*
     * Loop through all the notifications
     */
    for (pn = gpPendingNotifies; pn; pn = pnNext) {
        pnNext = pn->pNotifyNext;

        /*
         * Only destroy sync notifications that belong to this thread
         * and are not currently calling back i.e. ptiReceiver must be NULL.
         * Otherwise, when we come back from the callback in
         * xxxProcessNotifyWinEvent we will operate on a freed notify.
         * Also destroy the notification if the receiver is going away
         * or else it gets leaked as long as the sender is alive.
         */
        if ((pn->idSenderThread == idCurrentThread &&
                pn->ptiReceiver == NULL) || (pn->ptiReceiver == pti)) {
            if ((pn->dwWEFlags & WEF_ASYNC) == 0) {
                UserAssert((pn->dwWEFlags & WEF_POSTED) == 0);
                DestroyNotify(pn);
            }
        }
    }

    peh = gpWinEventHooks;
    while (peh) {
        pehNext = peh->pehNext;
        if (GETPTI(peh) == pti) {
            DestroyEventHook(peh);
        }
        peh = pehNext;
    }
    // Async notification not yet processed may still be posted in a queue,
    // pending being read and processed (gnNotifies > 0), although the
    // originating hook has now been unhooked (maybe gpWinEventHooks == NULL)
    // so the following assert is no good:
    // UserAssert(gpWinEventHooks || (!gpWinEventHooks && !gnNotifies));
}


// --------------------------------------------------------------------------
//
//  _SetWinEventHook()
//
//  This installs a win event hook.
//
//
// If hEventProcess set but idEventThread = 0, hook all threads in process.
// If idEventThread set but hEventProcess = NULL, hook single thread only.
// If neither are set, hook everything.
// If both are set ??
//
// --------------------------------------------------------------------------
PEVENTHOOK
_SetWinEventHook(
    DWORD           eventMin,
    DWORD           eventMax,
    HMODULE         hmodWinEventProc,
    PUNICODE_STRING pstrLib,
    WINEVENTPROC    pfnWinEventProc,
    HANDLE          hEventProcess,
    DWORD           idEventThread,
    DWORD           dwFlags)
{
    PEVENTHOOK pEventNew;
    PTHREADINFO ptiCurrent;

    int ihmod;

    ptiCurrent = PtiCurrent();

    //
    // If exiting, fail the call.
    //
    if (ptiCurrent->TIF_flags & TIF_INCLEANUP) {
        RIPMSG1(RIP_ERROR, "SetWinEventHook: Fail call - thread %#p in cleanup", ptiCurrent);
        return NULL;
    }

    /*
     * Check to see if filter proc is valid.
     */
    if (pfnWinEventProc == NULL) {
        RIPERR0(ERROR_INVALID_FILTER_PROC, RIP_VERBOSE, "pfnWinEventProc == NULL");
        return NULL;
    }

    if (eventMin > eventMax) {
        RIPERR0(ERROR_INVALID_HOOK_FILTER, RIP_VERBOSE, "eventMin > eventMax");
        return NULL;
    }

    if (dwFlags & WINEVENT_INCONTEXT) {
        /*
         * WinEventProc to be called in context of hooked thread, so needs a DLL
         */
        if (hmodWinEventProc == NULL) {
            RIPERR0(ERROR_HOOK_NEEDS_HMOD, RIP_VERBOSE, "");
            return NULL;
        } else if (pstrLib == NULL) {
            /*
             * If we got an hmod, we should get a DLL name too!
             */
            RIPERR1(ERROR_DLL_NOT_FOUND, RIP_ERROR,
                    "hmod %#p, but no lib name", hmodWinEventProc);
            return NULL;
        }
        ihmod = GetHmodTableIndex(pstrLib);
        if (ihmod == -1) {
            RIPERR0(ERROR_MOD_NOT_FOUND, RIP_VERBOSE, "");
            return NULL;
        }
    } else {
        ihmod = -1;            // means no DLL is required
        hmodWinEventProc = 0;
    }

    /*
     * Check the thread id, check it is a GUI thread.
     */
    if (idEventThread != 0) {
        PTHREADINFO ptiT;

        ptiT = PtiFromThreadId(idEventThread);
        if ((ptiT == NULL) ||
                !(ptiT->TIF_flags & TIF_GUITHREADINITIALIZED)) {
            RIPERR1(ERROR_INVALID_THREAD_ID, RIP_VERBOSE, "pti %#p", ptiT);
            return NULL;
        }
    }

    //
    // Create the window for async events first.  Creating it might yield,
    // so we want to do this before we've touched our event array.
    //
    // NOTE that USER itself will not pass on window creation/destruction
    // notifications for
    //      * IME windows
    //      * OLE windows
    //      * RPC windows
    //      * Event windows
    //

    //
    // Get a new event.
    //
    pEventNew = (PEVENTHOOK)HMAllocObject(ptiCurrent, NULL,
            TYPE_WINEVENTHOOK, sizeof(EVENTHOOK));
    if (!pEventNew)
        return NULL;

    //
    // Fill in the new event.
    //
    pEventNew->eventMin = (UINT)eventMin;
    pEventNew->eventMax = (UINT)eventMax;

    // pEventNew->f32Bit = ((dwFlags & WINEVENT_32BITCALLER) != 0);
    pEventNew->fIgnoreOwnThread = ((dwFlags & WINEVENT_SKIPOWNTHREAD) != 0);
    pEventNew->fIgnoreOwnProcess = ((dwFlags & WINEVENT_SKIPOWNPROCESS) != 0);
    pEventNew->fDestroyed = FALSE;
    pEventNew->fSync = ((dwFlags & WINEVENT_INCONTEXT) != 0);

    pEventNew->hEventProcess = hEventProcess;
    pEventNew->idEventThread = idEventThread;
    // pEventNew->cInUse = 0;

    pEventNew->ihmod = ihmod;

    /*
     * Add a dependency on this module - meaning, increment a count
     * that simply counts the number of hooks set into this module.
     */
    if (pEventNew->ihmod >= 0) {
        AddHmodDependency(pEventNew->ihmod);
    }

    /*
     * If pfnWinEventProc is in caller's process and no DLL is involved,
     * then pEventNew->offPfn is the actual address.
     */
    pEventNew->offPfn = ((ULONG_PTR)pfnWinEventProc) - ((ULONG_PTR)hmodWinEventProc);

    //
    //
    // Link our event into the master list.
    //
    // Note that we count on USER to not generate any events when installing
    // our hook.  The caller can't handle it yet since he hasn't got back
    // his event handle from this call.
    //
    pEventNew->pehNext = gpWinEventHooks;
    gpWinEventHooks = pEventNew;
    SET_SRVIF(SRVIF_WINEVENTHOOKS);

    return pEventNew;
}

/****************************************************************************\
*  UnhookWinEvent()
*
*  Unhooks a win event hook.  We of course sanity check that this thread is
*  the one which installed the hook.  We have to:  We are going to destroy
*  the IPC window and that must be in context.
*
\****************************************************************************/
BOOL
_UnhookWinEvent(PEVENTHOOK pEventUnhook)
{
    DBGVERIFYEVENTHOOK(pEventUnhook);

    if (HMIsMarkDestroy(pEventUnhook) || (GETPTI(pEventUnhook) != PtiCurrent())) {
        //
        // We do this to avoid someone calling UnhookWinEvent() the first
        // time, then somehow getting control again and calling it a second
        // time before we've managed to free up the event since someone was
        // in the middle of using it at the first UWE call.
        //

        RIPERR0(ERROR_INVALID_HANDLE, RIP_WARNING, "_UnhookWinEvent: Invalid event hook");
        return FALSE;
    }

    //
    // Purge this baby if all notifications are done.
    //      * if there are SYNC ones pending, the caller will clean this up
    //          upon the return from calling the event
    //      * if there are ASYNC ones pending, the receiver will not call
    //          the event and clean it up when he gets it.
    //

    //
    // NOTE that DestroyEventHook() does not yield!
    //
    DestroyEventHook(pEventUnhook);

    return TRUE;
}




/*****************************************************************************\
*
* DestroyEventHook()
*
* NOTE that this does NOT yield
*
* Destroys an event when the ref count has gone down to zero.  It may
* happen
*     * in the event generator's context, after returning from a callback
*         and the ref count dropped to zero, if sync
*     * in the event installer's context, after returning from a callback
*         and the ref count dropped to zero if async
*     * in the event installer's context, if on _UnhookWinEvent() the event
*         was not in use at all
*
\*****************************************************************************/
VOID
DestroyEventHook(PEVENTHOOK pEventDestroy)
{
    PEVENTHOOK *ppEvent;
    PEVENTHOOK pEventT;

    DBGVERIFYEVENTHOOK(pEventDestroy);
    UserAssert(gpWinEventHooks);

    /*
     * Mark this event as destroyed, but don't remove it from the event list
     * until its lock count goes to 0 - we may be traversing the list
     * within xxxWindowEvent, so we mustn't break the link to the next hook.
     */
    pEventDestroy->fDestroyed = TRUE;

    /*
     * If the object is locked, mark it for destroy but don't free it yet.
     */
    if (!HMMarkObjectDestroy(pEventDestroy))
        return;

    /*
     * Remove this from our event list.
     */
    for (ppEvent = &gpWinEventHooks; pEventT = *ppEvent; ppEvent = &pEventT->pehNext) {
        if (pEventT == pEventDestroy) {
            *ppEvent = pEventDestroy->pehNext;
            break;
        }
    }
    UserAssert(pEventT);
    SET_OR_CLEAR_SRVIF(SRVIF_WINEVENTHOOKS, gpWinEventHooks);

    /*
     * Make sure each hooked thread will unload the hook proc DLL
     */
    if (pEventDestroy->ihmod >= 0) {
        RemoveHmodDependency(pEventDestroy->ihmod);
    }

    /*
     * Free this pointer.
     */
    HMFreeObject(pEventDestroy);

    return;
}

/***************************************************************************\
*
* xxxGetEventProc()
*
* For sync events, this gets the address to call.  If 16-bits, then just
* return the installed address.  If 32-bits, we need to load the library
* if not in the same process as the installer.
\***************************************************************************/
WINEVENTPROC
xxxGetEventProc(PEVENTHOOK pEventOrg) {
    PTHREADINFO ptiCurrent;

    UserAssert(pEventOrg);
    UserAssert(pEventOrg->fSync);
    UserAssert(pEventOrg->ihmod >= 0);
    UserAssert(pEventOrg->offPfn != 0);

    CheckLock(pEventOrg);

    /*
     * Make sure the hook is still around before we
     * try and call it.
     */
    if (HMIsMarkDestroy(pEventOrg)) {
        return NULL;
    }

    ptiCurrent = PtiCurrent();

    /*
     * Make sure the DLL for this hook, if any, has been loaded
     * for the current process.
     */
    if ((pEventOrg->ihmod != -1) &&
            (TESTHMODLOADED(ptiCurrent, pEventOrg->ihmod) == 0)) {

        /*
         * Try loading the library, since it isn't loaded in this processes
         * context.  The hook is alrerady locked, so it won't go away while
         * we're loading this library.
         */
        if (xxxLoadHmodIndex(pEventOrg->ihmod, FALSE) == NULL) {
            return NULL;
        }
    }

    /*
     * While we're still inside the critical section make sure the
     * hook hasn't been 'freed'.  If so just return NULL.
     * IanJa - since WinEvent has already been called, you might think that we
     * should pass the event on, but the hooker may not be expecting this after
     * having cancelled the hook!  In any case, seems we may have two ways
     * of detecting that this hook has been removed:
     */

    /*
     * Make sure the hook is still around before we
     * try and call it.
     */
    if (HMIsMarkDestroy(pEventOrg)) {
        return NULL;
    }

    return (WINEVENTPROC)PFNHOOK(pEventOrg);
}
