/*****************************************************************************
 *
 *    ftpgto.cpp - Global timeouts
 *
 *    Global timeouts are managed by a separate worker thread, whose job
 *    it is to hang around and perform delayed actions on request.
 *
 *    All requests are for FTP_SESSION_TIME_OUT milliseconds.  If nothing happens
 *    for an additional FTP_SESSION_TIME_OUT milliseconds, the worker thread is
 *    terminated.
 *
 *****************************************************************************/

#include "priv.h"
#include "dbgmem.h"
#include "util.h"

#define MS_PER_SECOND               1000
#define SECONDS_PER_MINUTE          60
#define FTP_SESSION_TIME_OUT        (10 * SECONDS_PER_MINUTE * MS_PER_SECOND)    // Survive 10 minutes in cache


BOOL g_fBackgroundThreadStarted; // Has the background thread started?
HANDLE g_hthWorker;             // Background worker thread
HANDLE g_hFlushDelayedActionsEvent = NULL; // Do we want to flush the delayed actions?

/*****************************************************************************
 *
 *    Global Timeout Info
 *
 *    We must allocate separate information to track timeouts.  Stashing
 *    the information into a buffer provided by the caller opens race
 *    conditions, if the caller frees the memory before we are ready.
 *
 *    dwTrigger is 0 if the timeout is being dispatched.  This avoids
 *    race conditions where one thread triggers a timeout manually
 *    while it is in progress.
 *
 *****************************************************************************/

struct GLOBALTIMEOUTINFO g_gti = { // Anchor of global timeout info list
    &g_gti,
    &g_gti,
    0, 0, 0
};



/*****************************************************************************
 *    TriggerDelayedAction
 *
 *    Unlink the node and dispatch the timeout procedure.
 *****************************************************************************/
void TriggerDelayedAction(LPGLOBALTIMEOUTINFO * phgti)
{
    LPGLOBALTIMEOUTINFO hgti = *phgti;

    *phgti = NULL;
    if (hgti)
    {
        ENTERCRITICAL;
        if (hgti->dwTrigger)
        {
            // Unlink the node
            hgti->hgtiPrev->hgtiNext = hgti->hgtiNext;
            hgti->hgtiNext->hgtiPrev = hgti->hgtiPrev;

            hgti->dwTrigger = 0;

            // Do the callback
            if (hgti->pfn)
                hgti->pfn(hgti->pvRef);
            LEAVECRITICAL;

            TraceMsg(TF_BKGD_THREAD, "TriggerDelayedAction(%#08lx) Freeing=%#08lx", phgti, hgti);
            DEBUG_CODE(memset(hgti, 0xFE, (UINT) LocalSize((HLOCAL)hgti)));

            LocalFree((LPVOID) hgti);
        }
        else
        {
            LEAVECRITICAL;
        }
    }
}


/*****************************************************************************
 *    FtpDelayedActionWorkerThread
 *
 *    This is the procedure that runs on the worker thread.  It waits
 *    for something to do, and if enough time elapses with nothing
 *    to do, it terminates.
 *
 *    Be extremely mindful of race conditions.  They are oft subtle
 *    and quick to anger.
 *****************************************************************************/
DWORD FtpDelayedActionWorkerThread(LPVOID pv)
{
    FTPDebugMemLeak(DML_TYPE_THREAD | DML_BEGIN);

    // Tell the caller we started so they can continue.
    g_fBackgroundThreadStarted = TRUE;
    for (;;) 
    {
        DWORD msWait;

        // Determine how long we need to wait.  The critical section
        // is necessary to ensure we don't collide with SetDelayedAction.
        ENTERCRITICAL;
        if (g_gti.hgtiNext == &g_gti)
        {
            // Queue is empty
            msWait = FTP_SESSION_TIME_OUT;
        }
        else
        {
            msWait = g_gti.hgtiNext->dwTrigger - GetTickCount();
        }
        LEAVECRITICAL;

        //  If a new delayed action gets added, no matter, because
        //  we will wake up from the sleep before the delayed action
        //  is due.
        ASSERTNONCRITICAL;
        if ((int)msWait > 0)
        {
            TraceMsg(TF_BKGD_THREAD, "FtpDelayedActionWorkerThread: Sleep(%d)", msWait);
            WaitForMultipleObjects(1, &g_hFlushDelayedActionsEvent, FALSE, msWait);
            TraceMsg(TF_BKGD_THREAD, "FtpDelayedActionWorkerThread: Sleep finished");
        }
        ENTERCRITICALNOASSERT;
        if ((g_gti.hgtiNext != &g_gti) && g_gti.hgtiNext && (g_gti.hgtiNext->phgtiOwner))
        {    // Queue has work
#pragma message("$$ BUGBUG -- Race condition")
            LEAVECRITICAL;
            TraceMsg(TF_BKGD_THREAD, "FtpDelayedActionWorkerThread: Dispatching");
            TriggerDelayedAction(g_gti.hgtiNext->phgtiOwner);
        }
        else
        {
            CloseHandle(InterlockedExchangePointer(&g_hthWorker, NULL));
            CloseHandle(InterlockedExchangePointer(&g_hFlushDelayedActionsEvent, NULL));
            LEAVECRITICALNOASSERT;
            TraceMsg(TF_BKGD_THREAD, "FtpDelayedActionWorkerThread: ExitThread");
            ExitThread(0);
        }
    }

    AssertMsg(0, TEXT("FtpDelayedActionWorkerThread() We should never get here or we are exiting the for loop incorrectly."));
    FTPDebugMemLeak(DML_TYPE_THREAD | DML_END);
    return 0;
}


/*****************************************************************************
 *    SetDelayedAction
 *
 *    If there is a previous action, it is triggered.  (Not cancelled.)
 *
 *    In principle, we could've allocated into a private pointer, then
 *    stuffed the pointer in at the last minute, avoiding the need to
 *    take the critical section so aggressively.  But that would tend
 *    to open race conditions in the callers.  BUGBUG -- So?  I should
 *    fix the bugs instead of hacking around them like this.
 *****************************************************************************/
STDMETHODIMP SetDelayedAction(DELAYEDACTIONPROC pfn, LPVOID pvRef, LPGLOBALTIMEOUTINFO * phgti)
{
    TriggerDelayedAction(phgti);
    ENTERCRITICAL;
    if (!g_hthWorker)
    {
        DWORD dwThid;

        g_hFlushDelayedActionsEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (g_hFlushDelayedActionsEvent)
        {
            g_fBackgroundThreadStarted = FALSE;
            g_hthWorker = CreateThread(0, 0, FtpDelayedActionWorkerThread, 0, 0, &dwThid);
            if (g_hthWorker)
            {
                // We need to wait until the thread starts up
                // before we return. Otherwise, we may return to the
                // caller and they may free our COM object
                // which will unload our DLL.  The thread won't
                // start if we are in PROCESS_DLL_DETACH and we
                // spin waiting for them to start and stop.
                TraceMsg(TF_BKGD_THREAD, "SetDelayedAction: Thread created, waiting for it to start.");
                while (FALSE == g_fBackgroundThreadStarted)
                    Sleep(0);
                TraceMsg(TF_BKGD_THREAD, "SetDelayedAction: Thread started.");
            }
            else
            {
                CloseHandle(g_hFlushDelayedActionsEvent);
                g_hFlushDelayedActionsEvent = NULL;
            }
        }
    }

    if (g_hthWorker && EVAL(*phgti = (LPGLOBALTIMEOUTINFO) LocalAlloc(LPTR, sizeof(GLOBALTIMEOUTINFO))))
    {
        LPGLOBALTIMEOUTINFO hgti = *phgti;

        // Insert the node at the end (i.e., before the head)
        hgti->hgtiPrev = g_gti.hgtiPrev;
        g_gti.hgtiPrev->hgtiNext = hgti;

        g_gti.hgtiPrev = hgti;
        hgti->hgtiNext = &g_gti;

        // The "|1" ensures that dwTrigger is not zero
        hgti->dwTrigger = (GetTickCount() + FTP_SESSION_TIME_OUT) | 1;

        hgti->pfn = pfn;
        hgti->pvRef = pvRef;
        hgti->phgtiOwner = phgti;

        //  Note that there is no need to signal the worker thread that
        //  there is new work to do, because he will always wake up on
        //  his own before the requisite time has elapsed.
        //
        //  This optimization relies on the fact that the worker thread
        //  idle time is less than or equal to our delayed action time.
        LEAVECRITICAL;
    }
    else
    {
        // Unable to create worker thread or alloc memory
        LEAVECRITICAL;
    }
    return S_OK;
}


HRESULT PurgeDelayedActions(void)
{
    HRESULT hr = E_FAIL;

    if (g_hFlushDelayedActionsEvent)
    {
        LPGLOBALTIMEOUTINFO hgti = g_gti.hgtiNext;

        // We need to set all the times to zero so all waiting
        // items will not be delayed.
        ENTERCRITICAL;
        while (hgti != &g_gti)
        {
            hgti->dwTrigger = (GetTickCount() - 3);    // Don't Delay...
            hgti = hgti->hgtiNext;  // Next...
        }
        LEAVECRITICAL;

        if (SetEvent(g_hFlushDelayedActionsEvent))
        {
            // We can't be in a critical section or our background
            // thread can't come alive.
            ASSERTNONCRITICAL;

            TraceMsg(TF_BKGD_THREAD, "PurgeDelayedActions: Waiting for thread to stop.");
            // Now just wait for the thread to finish.  Someone may kill
            // the thread so let's make sure we don't keep sleeping
            // if the thread died.
            while (g_hthWorker && (WAIT_TIMEOUT == WaitForSingleObject(g_hthWorker, 0)))
                Sleep(0);

            TraceMsg(TF_BKGD_THREAD, "PurgeDelayedActions: Thread stopped.");
            // Sleep 0.1 seconds in order to give enough time for caller
            // to call CloseHandle(), LEAVECRITICAL, ExitThread(0).
            // I would much prefer to call WaitForSingleObject() on
            // the thread handle but I can't do that in PROCESS_DLL_DETACH.
            Sleep(100);
            hr = S_OK;
        }
    }

    return hr;
}


BOOL AreOutstandingDelayedActions(void)
{
    return (g_gti.hgtiNext != &g_gti);
}
