/**************************** Module Header ********************************\
* Module Name: exitwin.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* NT: Logoff user
* DOS: Exit windows
*
* History:
* 07-23-92 ScottLu      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define OPTIONMASK (EWX_SHUTDOWN | EWX_REBOOT | EWX_FORCE)

/*
 * Globals local to this file only
 */
PWINDOWSTATION  gpwinstaLogoff;
DWORD           gdwLocks;
DWORD           gdwShutdownFlags;
HANDLE          gpidEndSession;

extern PSECURITY_DESCRIPTOR gpsdInitWinSta;

/*
 * Called by ExitWindowsEx() to check whether the thread is permitted to logoff
 * If it is, and this is WinLogon calling, then also save any of the user's
 * setting that have not yet been stored in the profile.
 */
BOOL PrepareForLogoff(
    UINT uFlags)
{
    PTHREADINFO ptiCurrent = PtiCurrent();

    CheckCritIn();

    if (ptiCurrent->TIF_flags & TIF_RESTRICTED) {
        PW32JOB pW32Job;

        pW32Job = ptiCurrent->ppi->pW32Job;

        UserAssert(pW32Job != NULL);

        if (pW32Job->restrictions & JOB_OBJECT_UILIMIT_EXITWINDOWS) {
            // Not permitted to ExitWindows.
            return FALSE;
        }
    }

    /*
     * There are no restrictions, or the restriction do not deny shutdown:
     * The caller is about to ExitWindowsEx via CSR, so save the volatile
     * elements of the User preferences in their profile
     */
    if (ptiCurrent->pEThread->Cid.UniqueProcess == gpidLogon) {
        /*
         * Save the current user's NumLock state
         */
        TL tlName;
        PUNICODE_STRING pProfileUserName = CreateProfileUserName(&tlName);
        RegisterPerUserKeyboardIndicators(pProfileUserName);
        FreeProfileUserName(pProfileUserName, &tlName);
    }

    return TRUE;
    UNREFERENCED_PARAMETER(uFlags);
}

/*
 * Bug 294204 - joejo
 * Added an extra parameter to NotifyLogon so that we could
 * tell people that the logon/logoff was cancelled.
 */
BOOL NotifyLogon(
    PWINDOWSTATION pwinsta,
    PLUID pluidCaller,
    DWORD dwFlags,
    NTSTATUS StatusCode)
{
    BOOL fNotified = FALSE;
    DWORD dwllParam;
    DWORD dwStatus;

    if (!(dwFlags & EWX_NONOTIFY)) {

        if (dwFlags & EWX_CANCELED) {
            dwllParam = LOGON_LOGOFFCANCELED;
            dwStatus = StatusCode;
        } else {
            dwllParam = LOGON_LOGOFF;
            dwStatus = dwFlags;
        }

        if (dwFlags & EWX_SHUTDOWN) {
            /*
             * Post the message to the global logon notify window
             */
            if (gspwndLogonNotify != NULL) {
                _PostMessage(gspwndLogonNotify, WM_LOGONNOTIFY,
                             dwllParam, (LONG)dwStatus);
                fNotified = TRUE;
            }
        } else {
            if (gspwndLogonNotify != NULL &&
                    (RtlEqualLuid(&pwinsta->luidUser, pluidCaller) ||
                     RtlEqualLuid(&luidSystem, pluidCaller))) {
                _PostMessage(gspwndLogonNotify, WM_LOGONNOTIFY, dwllParam,
                        (LONG)dwStatus);
                fNotified = TRUE;
            }
        }
    }
    return fNotified;
}

NTSTATUS InitiateShutdown(
    PETHREAD Thread,
    PULONG lpdwFlags)
{
    static PRIVILEGE_SET psShutdown = {
        1, PRIVILEGE_SET_ALL_NECESSARY, { SE_SHUTDOWN_PRIVILEGE, 0 }
    };
    PEPROCESS Process;
    LUID luidCaller;
    PPROCESSINFO ppi;
    PWINDOWSTATION pwinsta;
    HWINSTA hwinsta;
    PTHREADINFO ptiClient;
    NTSTATUS Status;
    DWORD dwFlags;

    /*
     * Find out the callers sid. Only want to shutdown processes in the
     * callers sid.
     */
    Process = THREAD_TO_PROCESS(Thread);
    ptiClient = PtiFromThread(Thread);
    Status = GetProcessLuid(Thread, &luidCaller);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    /*
     * Set the system flag if the caller is a system process.
     * Winlogon uses this to determine in which context to perform
     * a shutdown operation.
     */
    dwFlags = *lpdwFlags;
    if (RtlEqualLuid(&luidCaller, &luidSystem)) {
        dwFlags |= EWX_SYSTEM_CALLER;
    } else {
        dwFlags &= ~EWX_SYSTEM_CALLER;
    }

    /*
     * Find a windowstation.  If the process does not have one
     * assigned, use the standard one.
     */
    ppi = PpiFromProcess(Process);
    if (ppi == NULL) {
        /*
         * We ran into a case where the thread was terminated and had already
         * been cleaned up by USER.  Thus, the ppi and ptiClient was NULL.
         */
        return STATUS_INVALID_HANDLE;
    }
    pwinsta = ppi->rpwinsta;
    hwinsta = ppi->hwinsta;
    /*
     * If we're not being called by Winlogon, validate the call and
     * notify the logon process to do the actual shutdown.
     */
    if (Thread->Cid.UniqueProcess != gpidLogon) {
        dwFlags &= ~EWX_WINLOGON_CALLER;
        *lpdwFlags = dwFlags;

        if (pwinsta == NULL) {
#ifndef LATER
            return STATUS_INVALID_HANDLE;
#else
            hwinsta = ppi->pOpenObjectTable[HI_WINDOWSTATION].h;
            if (hwinsta == NULL) {
                return STATUS_INVALID_HANDLE;
            }
            pwinsta = (PWINDOWSTATION)ppi->pOpenObjectTable[HI_WINDOWSTATION].phead;
#endif
        }

        /*
         * Check security first - does this thread have access?
         */
        if (!RtlAreAllAccessesGranted(ppi->amwinsta, WINSTA_EXITWINDOWS)) {
            return STATUS_ACCESS_DENIED;
        }

        /*
         * If the client requested shutdown, reboot, or poweroff they must have
         * the shutdown privilege.
         */
        if (dwFlags & EWX_SHUTDOWN) {
            if (!IsPrivileged(&psShutdown) ) {
                return STATUS_PRIVILEGE_NOT_HELD;
            }
        } else {

            /*
             * If this is a non-IO windowstation and we are not shutting down,
             * fail the call.
             */
            if (pwinsta->dwWSF_Flags & WSF_NOIO) {
                return STATUS_INVALID_DEVICE_REQUEST;
            }
        }
    }

    /*
     * Is there a shutdown already in progress?
     */
    if (gdwThreadEndSession != 0) {
        DWORD dwNew;

        /*
         * If the current shutdown in another sid and is not being done by
         * winlogon, override it.
         */
        if (!RtlEqualLuid(&luidCaller, &gpwinstaLogoff->luidEndSession) &&
                (gpidEndSession != gpidLogon)) {
            return STATUS_RETRY;
        }

        /*
         * Calculate new flags
         */
        dwNew = dwFlags & OPTIONMASK & (~gdwShutdownFlags);

        /*
         * Should we override the other shutdown?  Make sure
         * winlogon does not recurse.
         */
        if (dwNew && HandleToUlong(PsGetCurrentThread()->Cid.UniqueThread) !=
                gdwThreadEndSession) {
            /*
             * Only one windowstation can be logged off at a time.
             */
            if (!(dwFlags & EWX_SHUTDOWN) &&
                    pwinsta != gpwinstaLogoff) {
                return STATUS_DEVICE_BUSY;
            }

            /*
             * Set the new flags
             */
            gdwShutdownFlags = dwFlags;

            if (dwNew & EWX_FORCE) {
                return STATUS_RETRY;
            } else {
                return STATUS_PENDING;
            }
        } else {
            /*
             * Don't override
             */
            return STATUS_PENDING;
        }
    }

    /*
     * If the caller is not winlogon, signal winlogon to start
     * the real shutdown.
     */
    if (Thread->Cid.UniqueProcess != gpidLogon) {
        if (dwFlags & EWX_NOTIFY) {
            if (ptiClient && ptiClient->TIF_flags & TIF_16BIT)
                gptiShutdownNotify = ptiClient;
            dwFlags &= ~EWX_NOTIFY;
            *lpdwFlags = dwFlags;
        }

        if (NotifyLogon(pwinsta, &luidCaller, dwFlags, STATUS_SUCCESS))
            return STATUS_PENDING;
        else if (ptiClient && ptiClient->cWindows)
            return STATUS_CANT_WAIT;
    }

    /*
     * Mark this thread as the one that is currently processing
     * exit windows, and set the global saying someone is exiting
     */
    dwFlags |= EWX_WINLOGON_CALLER;
    *lpdwFlags = dwFlags;
    gdwShutdownFlags = dwFlags;

    gdwThreadEndSession = HandleToUlong(PsGetCurrentThread()->Cid.UniqueThread);
    gpidEndSession = PsGetCurrentThread()->Cid.UniqueProcess;
    gpwinstaLogoff = pwinsta;
    pwinsta->luidEndSession = luidCaller;

    /*
     * Lock the windowstation to prevent apps from starting
     * while we're doing shutdown processing.
     */
    gdwLocks = pwinsta->dwWSF_Flags & (WSF_SWITCHLOCK | WSF_OPENLOCK);
    pwinsta->dwWSF_Flags |= (WSF_OPENLOCK | WSF_SHUTDOWN);

    /*
     * Set the flag WSF_REALSHUTDOWN if we are not doing just a
     * logoff
     */
    if (dwFlags &
        (EWX_WINLOGON_OLD_SHUTDOWN | EWX_WINLOGON_OLD_REBOOT |
         EWX_SHUTDOWN | EWX_REBOOT)) {

        pwinsta->dwWSF_Flags |= WSF_REALSHUTDOWN;
    }

    return STATUS_SUCCESS;
}

NTSTATUS EndShutdown(
    PETHREAD Thread,
    NTSTATUS StatusShutdown)
{
    PWINDOWSTATION pwinsta = gpwinstaLogoff;
    PDESKTOP pdesk;
    LUID luidCaller;
    UserAssert(gpwinstaLogoff);

    gpwinstaLogoff = NULL;
    gpidEndSession = NULL;
    gdwThreadEndSession = 0;
    pwinsta->dwWSF_Flags &= ~WSF_SHUTDOWN;

    if (!NT_SUCCESS(GetProcessLuid(Thread, &luidCaller))) {
        luidCaller = RtlConvertUlongToLuid(0);     // null luid
    }

    if (!NT_SUCCESS(StatusShutdown)) {

        /*
         * We need to notify the process that called ExitWindows that
         * the logoff was aborted.
         */
        if (gptiShutdownNotify) {
            _PostThreadMessage(gptiShutdownNotify, WM_ENDSESSION, FALSE, 0);
            gptiShutdownNotify = NULL;
        }

        /*
         * Reset the windowstation lock flags so apps can start
         * again.
         */
        pwinsta->dwWSF_Flags =
                (pwinsta->dwWSF_Flags & ~WSF_OPENLOCK) |
                gdwLocks;

        /*
         * Bug 294204 - joejo
         * Tell winlogon that we we cancelled shutdown/logoff.
         */
        NotifyLogon(pwinsta, &luidCaller, gdwShutdownFlags | EWX_CANCELED, StatusShutdown);

        return STATUS_SUCCESS;
    }

    gptiShutdownNotify = NULL;

    /*
     * If logoff is occuring for the user set by winlogon, perform
     * the normal logoff cleanup.  Otherwise, clear the open lock
     * and continue.
     */
    if (((pwinsta->luidUser.LowPart != 0) || (pwinsta->luidUser.HighPart != 0)) &&
            RtlEqualLuid(&pwinsta->luidUser, &luidCaller)) {

        /*
         * Zero out the free blocks in all desktop heaps.
         */
        for (pdesk = pwinsta->rpdeskList; pdesk != NULL; pdesk = pdesk->rpdeskNext) {
            RtlZeroHeap(Win32HeapGetHandle(pdesk->pheapDesktop), 0);
        }

        /*
         * Logoff/shutdown was successful. In case this is a logoff, remove
         * everything from the clipboard so the next logged on user can't get
         * at this stuff.
         */
        ForceEmptyClipboard(pwinsta);

        /*
         * Destroy all non-pinned atoms in the global atom table.  User can't
         * create pinned atoms.  Currently only the OLE atoms are pinned.
         */
        RtlEmptyAtomTable(pwinsta->pGlobalAtomTable, FALSE);

        // this code path is hit only on logoff and also on shutdown
        // We do not want to unload fonts twice when we attempt shutdown
        // so we mark that the fonts have been unloaded at a logoff time

        if (TEST_PUDF(PUDF_FONTSARELOADED)) {
            LeaveCrit();
            GreRemoveAllButPermanentFonts();
            EnterCrit();
            CLEAR_PUDF(PUDF_FONTSARELOADED);
        }
    } else {
        pwinsta->dwWSF_Flags &= ~WSF_OPENLOCK;
    }

    /*
     * Tell winlogon that we successfully shutdown/logged off.
     */
    NotifyLogon(pwinsta, &luidCaller, gdwShutdownFlags, STATUS_SUCCESS);

    return STATUS_SUCCESS;
}

/***************************************************************************\
* xxxClientShutdown2
*
* Called by xxxClientShutdown
\***************************************************************************/

LONG xxxClientShutdown2(
    PBWL pbwl,
    UINT msg,
    WPARAM wParam)
{
    HWND *phwnd;
    PWND pwnd;
    TL tlpwnd;
    BOOL fEnd;
    PTHREADINFO ptiCurrent = PtiCurrent();
    BOOL fDestroyTimers;
    LPARAM lParam;

    /*
     * Make sure we don't send this window any more WM_TIMER
     * messages if the session is ending. This was causing
     * AfterDark to fault when it freed some memory on the
     * WM_ENDSESSION and then tried to reference it on the
     * WM_TIMER.
     * LATER GerardoB: Do we still need to do this??
     * Do this horrible thing only if the process is in the
     * context being logged off.
     * Perhaps someday we should post a WM_CLOSE so the app
     * gets a better chance to clean up (if this process is in
     * the context being logged off, winsrv is going to call
     * TerminateProcess soon after this).
     */
     fDestroyTimers = (wParam & WMCS_EXIT) && (wParam & WMCS_CONTEXTLOGOFF);

     /*
      * fLogOff and fEndSession parameters (WM_ENDSESSION only)
      */
     lParam = wParam & ENDSESSION_LOGOFF;
     wParam &= WMCS_EXIT;

    /*
     * Now enumerate these windows and send the WM_QUERYENDSESSION or
     * WM_ENDSESSION messages.
     */
    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {
        if ((pwnd = RevalidateHwnd(*phwnd)) == NULL)
            continue;

        ThreadLockAlways(pwnd, &tlpwnd);

        /*
         * Send the message.
         */
        switch (msg) {
        case WM_QUERYENDSESSION:

            /*
             * Windows does not send the WM_QUERYENDSESSION to the app
             * that called ExitWindows
             */
            if (ptiCurrent == gptiShutdownNotify) {
                fEnd = TRUE;
            } else {
                fEnd = (xxxSendMessage(pwnd, WM_QUERYENDSESSION, FALSE, lParam) != 0);
                if (!fEnd) {
                    RIPMSG2(RIP_WARNING, "xxxClientShutdown2: pwnd:%p canceled shutdown. lParam:%p",
                            pwnd, lParam);
                }
            }
            break;

        case WM_ENDSESSION:
            xxxSendMessage(pwnd, WM_ENDSESSION, wParam, lParam);
            fEnd = TRUE;

            if (fDestroyTimers) {
                DestroyWindowsTimers(pwnd);
            }

            break;
        }

        ThreadUnlock(&tlpwnd);

        if (!fEnd)
            return WMCSR_CANCEL;
    }

    return WMCSR_ALLOWSHUTDOWN;
}
/***************************************************************************\
* xxxClientShutdown
*
* This is the processing that occurs when an application receives a
* WM_CLIENTSHUTDOWN message.
*
* 10-01-92 ScottLu      Created.
\***************************************************************************/
LONG xxxClientShutdown(
    PWND pwnd,
    WPARAM wParam)
{
    PBWL pbwl;
    PTHREADINFO ptiT;
    LONG lRet;

    /*
     * Build a list of windows first.
     */
    ptiT = GETPTI(pwnd);

    if ((pbwl = BuildHwndList(ptiT->rpdesk->pDeskInfo->spwnd->spwndChild,
            BWL_ENUMLIST, ptiT)) == NULL) {
        /*
         * Can't allocate memory to notify this thread's windows of shutdown.
         * Can't do more than kill the app
         */
        return WMCSR_ALLOWSHUTDOWN;
    }

    if (wParam & WMCS_QUERYEND) {
        lRet = xxxClientShutdown2(pbwl, WM_QUERYENDSESSION, wParam);
    } else {
        xxxClientShutdown2(pbwl, WM_ENDSESSION, wParam);
        lRet = WMCSR_DONE;
    }

    FreeHwndList(pbwl);
    return lRet;
}

/***************************************************************************\
* xxxRegisterUserHungAppHandlers
*
* This routine simply records the WOW callback address for notification of
* "hung" wow apps.
*
* History:
* 01-Apr-1992 jonpa      Created.
* Added saving and duping of wowexc event handle
\***************************************************************************/

BOOL xxxRegisterUserHungAppHandlers(
    PFNW32ET pfnW32EndTask,
    HANDLE   hEventWowExec)
{
    BOOL   bRetVal;
    PPROCESSINFO    ppi;
    PWOWPROCESSINFO pwpi;

    //
    //  Allocate the per wow process info stuff
    //  ensuring the memory is Zero init.
    //
    pwpi = (PWOWPROCESSINFO) UserAllocPoolWithQuotaZInit(
            sizeof(WOWPROCESSINFO), TAG_WOWPROCESSINFO);

    if (!pwpi)
        return FALSE;

    //
    // Reference the WowExec event for kernel access
    //
    bRetVal = NT_SUCCESS(ObReferenceObjectByHandle(
                 hEventWowExec,
                 EVENT_ALL_ACCESS,
                 *ExEventObjectType,
                 UserMode,
                 &pwpi->pEventWowExec,
                 NULL
                 ));

    //
    //  if sucess then intialize the pwpi, ppi structs
    //  else free allocated memory
    //
    if (bRetVal) {
        pwpi->hEventWowExecClient = hEventWowExec;
        pwpi->lpfnWowExitTask = pfnW32EndTask;
        ppi = PpiCurrent();
        ppi->pwpi = pwpi;

        // add to the list, order doesn't matter
        pwpi->pwpiNext = gpwpiFirstWow;
        gpwpiFirstWow  = pwpi;

        }
    else {
        UserFreePool(pwpi);
        }

   return bRetVal;
}
