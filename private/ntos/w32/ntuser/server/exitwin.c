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

#define BEGIN_LPC_RECV(API)                                                 \
    P##API##MSG a = (P##API##MSG)&m->u.ApiMessageData;                      \
    PCSR_THREAD pcsrt;                                                      \
    PTEB Teb = NtCurrentTeb();                                              \
    NTSTATUS Status = STATUS_SUCCESS;                                       \
    UNREFERENCED_PARAMETER(ReplyStatus);                                    \
                                                                            \
    Teb->LastErrorValue = 0;                                                \
    pcsrt = CSR_SERVER_QUERYCLIENTTHREAD();

#define END_LPC_RECV()                                                  \
    a->dwLastError = Teb->LastErrorValue;                               \
    return Status;

#define CCHMSGMAX   256
#define CCHBODYMAX  512

#define CSR_THREAD_SHUTDOWNSKIP 0x00000008

BOOL WowExitTask(PCSR_THREAD pcsrt);
NTSTATUS UserClientShutdown(PCSR_PROCESS pcsrp, ULONG dwFlags, BOOLEAN fFirstPass);

/***************************************************************************\
* _ExitWindowsEx
*
* Determines whether shutdown is allowed, and if so calls CSR to start
* shutting down processes. If this succeeds all the way through, tell winlogon
* so it'll either logoff or reboot the system. Shuts down the processes in
* the caller's sid.
*
* History
* 07-23-92 ScottLu      Created.
\***************************************************************************/

NTSTATUS _ExitWindowsEx(
    PCSR_THREAD pcsrt,
    UINT dwFlags,
    DWORD dwReserved)
{
    LUID luidCaller;
    NTSTATUS Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(dwReserved);

    if ((dwFlags & EWX_REBOOT) || (dwFlags & EWX_POWEROFF)) {
        dwFlags |= EWX_SHUTDOWN;
    }

    //
    // Only winlogon gets to set the high flags:
    //

    if ( ( dwFlags & ( ~ ( EWX_VALID ) ) ) != 0 )
    {
        if ( HandleToUlong(pcsrt->ClientId.UniqueProcess) != gIdLogon )
        {
            KdPrint(( "Process %x tried to call ExitWindowsEx with flags %x\n",
                        pcsrt->ClientId.UniqueProcess, dwFlags ));

            return STATUS_ACCESS_DENIED ;
        }
    }

    /*
     * Find out the callers sid. Only want to shutdown processes in the
     * callers sid.
     */
    if (!CsrImpersonateClient(NULL)) {
        return STATUS_BAD_IMPERSONATION_LEVEL;
    }

    Status = CsrGetProcessLuid(NULL, &luidCaller);
    if (!NT_SUCCESS(Status)) {
        CsrRevertToSelf();
        return Status;
    }

    /*
     * Loop until we can do the shutdown; if we cannot do it,
     *  we'll go to fastexit and bail.
     */
    while (TRUE) {

        LARGE_INTEGER li;

        Status = NtUserSetInformationThread(pcsrt->ThreadHandle,
                                            UserThreadInitiateShutdown,
                                            &dwFlags, sizeof(dwFlags));

        switch (Status) {
        case STATUS_PENDING:
            /*
             * The logoff/shutdown is in progress and nothing
             * more needs to be done.
             */
            goto fastexit;

        case STATUS_RETRY:
            /*
             * Another logoff/shutdown is in progress and we need
             * to cancel it so we can do an override.
             *
             * if someone else is trying to cancel shutdown, exit
             */
            EnterCrit();
            li.QuadPart  = 0;
            if (NtWaitForSingleObject(gheventCancel, FALSE, &li) == WAIT_OBJECT_0) {
                Status = STATUS_PENDING;
                LeaveCrit();
                goto fastexit;
            }
            /*
             * If no one will set gheventCancelled, don't wait.
             */
            if (gdwThreadEndSession == 0) {
                LeaveCrit();
                continue;
            }

            NtClearEvent(gheventCancelled);
            NtSetEvent(gheventCancel, NULL);
            LeaveCrit();
            /*
             * Wait for the other guy to be cancelled
             */
            NtWaitForSingleObject(gheventCancelled, FALSE, NULL);

            EnterCrit();
            /*
             * This signals that we are no longer trying to cancel a
             * shutdown
             */
            NtClearEvent(gheventCancel);
            /*
             * If someone managed to start a shutdown again, exit
             *  Can this happen? Let's assert to check it out.
             */
            if (gdwThreadEndSession != 0) {
                UserAssert(gdwThreadEndSession == 0);
                Status = STATUS_PENDING;
                LeaveCrit();
                goto fastexit;
            }
            LeaveCrit();
            continue;

        case STATUS_CANT_WAIT:

            /*
             * There is no notify window and the calling thread has
             * windows that prevent this request from succeeding.
             * The client handles this by starting another thread
             * to recall ExitWindowsEx.
             */
            goto fastexit;

        default:
            if (!NT_SUCCESS(Status)) {
                SetLastError(RtlNtStatusToDosError(Status));
                goto fastexit;
            }
        }
        break;
    }

    /*
     * This thread is doing the shutdown
     */
    EnterCrit();
    UserAssert(gdwThreadEndSession == 0);
    gdwThreadEndSession = HandleToUlong(pcsrt->ClientId.UniqueThread);
    LeaveCrit();

    /*
     * Call csr to loop through the processes shutting them down.
     */
    Status = CsrShutdownProcesses(&luidCaller, dwFlags);
    /*
     * Tell win32k.sys we're done.
     */
    NtUserSetInformationThread(pcsrt->ThreadHandle, UserThreadEndShutdown, &Status, sizeof(Status));

    EnterCrit();
    gdwThreadEndSession = 0;
    NtSetEvent(gheventCancelled, NULL);
    LeaveCrit();

fastexit:
    CsrRevertToSelf();

    return Status;
}

/***************************************************************************\
* UserClientShutdown
*
* This gets called from CSR. If we recognize the application (i.e., it has a
* top level window), then send queryend/end session messages to it. Otherwise
* say we don't recognize it.
*
* 07-23-92 ScottLu      Created.
\***************************************************************************/

NTSTATUS UserClientShutdown(
    PCSR_PROCESS pcsrp,
    ULONG dwFlags,
    BOOLEAN fFirstPass)
{
    PLIST_ENTRY ListHead, ListNext;
    PCSR_PROCESS Process;
    PCSR_THREAD Thread;
    USERTHREAD_SHUTDOWN_INFORMATION ShutdownInfo;
    BOOL fNoMsgs;
    BOOL fNoMsgsEver = TRUE;
    BOOL Forced = FALSE;
    BOOL bDoBlock;
    BOOL fNoRetry;
    DWORD cmd = 0, dwClientFlags;
    NTSTATUS Status;
    NTSTATUS TerminateStatus = STATUS_ACCESS_DENIED;
    UINT cThreads;
    BOOL fSendEndSession = FALSE;

#if DBG
    DWORD dwLocalThreadEndSession = gdwThreadEndSession;
#endif

    /*
     * If this is a logoff and the process does not belong to
     * the account doing the logoff and is not LocalSystem,
     * do not send end-session messages.  Console will notify
     * the app of the logoff.
     */
    if (!(dwFlags & EWX_SHUTDOWN) && (pcsrp->ShutdownFlags & SHUTDOWN_OTHERCONTEXT)) {
        Status = SHUTDOWN_UNKNOWN_PROCESS;
        goto CleanupAndExit;
    }

    /*
     * Calculate whether to allow exit and force-exit this process before
     * we unlock pcsrp.
     */
    fNoRetry = (pcsrp->ShutdownFlags & SHUTDOWN_NORETRY) ||
            (dwFlags & EWX_FORCE);

    /*
     * Setup flags for WM_CLIENTSHUTDOWN
     * -Assume the process is going to OK the WM_QUERYENDSESSION (WMCS_EXIT)
     * -NT's shutdown always starts with a logoff.
     * -Shutdown or logoff? (WMCS_SHUTDOWN)
     * -Should display dialog for hung apps? (WMCS_NODLGIFHUNG)
     * -is this process in the context being logged off? (WMCS_CONTEXTLOGOFF)
     */
    dwClientFlags = WMCS_EXIT | (fNoRetry ? WMCS_NORETRY : 0);
    //}

    /*
     * Check the flags originally passed by the ExitWindows caller to see if we're
     *  really just logging off.
     */
    if (!(dwFlags & (EWX_WINLOGON_OLD_REBOOT | EWX_WINLOGON_OLD_SHUTDOWN))) {
        dwClientFlags |= WMCS_LOGOFF;
    }

    if (dwFlags & EWX_FORCEIFHUNG) {
        dwClientFlags |= WMCS_NODLGIFHUNG;
    }
    if (!(pcsrp->ShutdownFlags & (SHUTDOWN_SYSTEMCONTEXT | SHUTDOWN_OTHERCONTEXT))) {
        dwClientFlags |= WMCS_CONTEXTLOGOFF;
    }


    /*
     * Lock the process while we walk the thread list.  We know
     * that the process is valid and therefore do not need to
     * check the return status.
     */
    CsrLockProcessByClientId(pcsrp->ClientId.UniqueProcess, &Process);

    ShutdownInfo.StatusShutdown = SHUTDOWN_UNKNOWN_PROCESS;

    /*
     * Go through the thread list and mark them as not
     * shutdown yet.
     */
    ListHead = &pcsrp->ThreadList;
    ListNext = ListHead->Flink;
    while (ListNext != ListHead) {
        Thread = CONTAINING_RECORD( ListNext, CSR_THREAD, Link );
        Thread->Flags &= ~CSR_THREAD_SHUTDOWNSKIP;
        ListNext = ListNext->Flink;
    }

    /*
     * Perform the proper shutdown operation on each thread.  Keep
     * a count of the number of gui threads found.
     */
    cThreads = 0;
    ShutdownInfo.drdRestore.pdeskRestore = NULL;
    ShutdownInfo.drdRestore.hdeskNew = NULL;
    while (TRUE) {
        ListNext = ListHead->Flink;
        while (ListNext != ListHead) {
            Thread = CONTAINING_RECORD( ListNext, CSR_THREAD, Link );
            /*
             * Skip the thread doing the shutdown.  Assume that it's
             * ready.
             * gdwThreadEndSession shouldn't change while the shutdown
             *  is in progress; so this should be thread safe.
             */
            UserAssert(gdwThreadEndSession == dwLocalThreadEndSession);
            if (HandleToUlong(Thread->ClientId.UniqueThread) == gdwThreadEndSession) {
                Thread->Flags |= CSR_THREAD_SHUTDOWNSKIP;
            }

            if (!(Thread->Flags &
                    (CSR_THREAD_DESTROYED | CSR_THREAD_SHUTDOWNSKIP))) {
                break;
            }
            ListNext = ListNext->Flink;
        }
        if (ListNext == ListHead)
            break;

        Thread->Flags |= CSR_THREAD_SHUTDOWNSKIP;
        ShutdownInfo.dwFlags = dwClientFlags;

        Status = NtUserQueryInformationThread(Thread->ThreadHandle,
                UserThreadShutdownInformation, &ShutdownInfo, sizeof(ShutdownInfo), NULL);

        if (!NT_SUCCESS(Status))
            continue;
        if (ShutdownInfo.StatusShutdown == SHUTDOWN_UNKNOWN_PROCESS)
            continue;
        if (ShutdownInfo.StatusShutdown == SHUTDOWN_KNOWN_PROCESS) {
            CsrUnlockProcess(Process);
            Status = SHUTDOWN_KNOWN_PROCESS;
            goto RestoreDesktop;
        }

        /*
         * If this process is not in the account being logged off and it
         * is not on the windowstation being logged off, don't send
         * the end session messages.
         */
        if (!(dwClientFlags & WMCS_CONTEXTLOGOFF) && (ShutdownInfo.hwndDesktop == NULL)) {
            /*
             * This process is not in the context being logged off.  Do
             * not terminate it and let console send an event to the process.
             */
            ShutdownInfo.StatusShutdown = SHUTDOWN_UNKNOWN_PROCESS;
            continue;
        }

        /*
         * Shut down this process.
         */
        cThreads++;

        if (ISTS()) {
            Forced = (dwFlags & EWX_FORCE);
            fNoMsgs =  (pcsrp->ShutdownFlags & SHUTDOWN_NORETRY) ||
                       !(ShutdownInfo.dwFlags & USER_THREAD_GUI);
            fNoMsgsEver &= fNoMsgs;
            if (Forced && (!(dwFlags & EWX_NONOTIFY) || (gSessionId != 0)))  {
                dwClientFlags &= ~WMCS_LOGOFF; // WinStation Reset or Shutdown. Don't do this for console session.
            }

            if (fNoMsgs || Forced) {
                BoostHardError((ULONG_PTR)Thread->ClientId.UniqueProcess, BHE_FORCE);
            }
            bDoBlock = (fNoMsgs == FALSE);

        } else {
            if (fNoRetry || !(ShutdownInfo.dwFlags & USER_THREAD_GUI)) {

                /*
                 * Dispose of any hard errors.
                 */
                BoostHardError((ULONG_PTR)Thread->ClientId.UniqueProcess, BHE_FORCE);
                bDoBlock = FALSE;
            } else {
                bDoBlock = TRUE;
            }
        }

        if (bDoBlock) {
            CsrReferenceThread(Thread);
            CsrUnlockProcess(Process);

            /*
             * There are problems in changing shutdown to send all the
             * QUERYENDSESSIONs at once before doing any ENDSESSIONs, like
             * Windows does. The whole machine needs to be modal if you do this.
             * If it isn't modal, then you have this problem. Imagine app 1 and 2.
             * 1 gets the queryendsession, no problem. 2 gets it and brings up a
             * dialog. Now being a simple user, you decide you need to change the
             * document in app 1. Now you switch back to app 2, hit ok, and
             * everything goes away - including app 1 without saving its changes.
             * Also, apps expect that once they've received the QUERYENDSESSION,
             * they are not going to get anything else of any particular interest
             * (unless it is a WM_ENDSESSION with FALSE) We had bugs pre 511 where
             * apps were blowing up because of this.
             * If this change is made, the entire system must be modal
             * while this is going on. - ScottLu 6/30/94
             */
            cmd = ThreadShutdownNotify(dwClientFlags | WMCS_QUERYEND, (ULONG_PTR)Thread, 0);

            CsrLockProcessByClientId(pcsrp->ClientId.UniqueProcess, &Process);
            CsrDereferenceThread(Thread);

            /*
             * If shutdown has been cancelled, let csr know about it.
             */
            switch (cmd) {
            case TSN_USERSAYSCANCEL:
            case TSN_APPSAYSNOTOK:
                if (!Forced) {
                    dwClientFlags &= ~WMCS_EXIT;
                }

                /*
                 * Fall through.
                 */
            case TSN_APPSAYSOK:
                fSendEndSession = TRUE;
                break;

            case TSN_USERSAYSKILL:
                /*
                 * Since we cannot just kill one thread, the whole process
                 *  is going down. Hence, there is no point on continuing
                 *  checking other threads. Also, the user wants it killed
                 *  so we won't waste any time sending more messages
                 */
                dwClientFlags |= WMCS_EXIT;
                goto KillIt;

            case TSN_NOWINDOW:
                /*
                 * Did this process have a window?
                 * If this is the second pass we terminate the process even if it did
                 * not have any windows in case the app was just starting up.
                 * WOW hits this often when because it takes so long to start up.
                 * Logon (with WOW auto-starting) then logoff WOW won't die but will
                 * lock some files open so you can't logon next time.
                 */
                if (fFirstPass) {
                    cThreads--;
                }
                break;
            }
        }
    }

    /*
     * If end session message need to be sent, do it now.
     */
    if (fSendEndSession) {

        /*
         * Go through the thread list and mark them as not
         * shutdown yet.
         */
        ListNext = ListHead->Flink;
        while (ListNext != ListHead) {
            Thread = CONTAINING_RECORD( ListNext, CSR_THREAD, Link );
            Thread->Flags &= ~CSR_THREAD_SHUTDOWNSKIP;
            ListNext = ListNext->Flink;
        }

        /*
         * Perform the proper shutdown operation on each thread.
         */
        while (TRUE) {
            ListHead = &pcsrp->ThreadList;
            ListNext = ListHead->Flink;
            while (ListNext != ListHead) {
                Thread = CONTAINING_RECORD( ListNext, CSR_THREAD, Link );
                if (!(Thread->Flags &
                        (CSR_THREAD_DESTROYED | CSR_THREAD_SHUTDOWNSKIP))) {
                    break;
                }
                ListNext = ListNext->Flink;
            }
            if (ListNext == ListHead)
                break;

            Thread->Flags |= CSR_THREAD_SHUTDOWNSKIP;
            ShutdownInfo.dwFlags = dwClientFlags;

            Status = NtUserQueryInformationThread(Thread->ThreadHandle,
                    UserThreadShutdownInformation, &ShutdownInfo, sizeof(ShutdownInfo), NULL);

            if (!NT_SUCCESS(Status))
                continue;

            if (ShutdownInfo.StatusShutdown == SHUTDOWN_UNKNOWN_PROCESS ||
                    !(ShutdownInfo.dwFlags & USER_THREAD_GUI))
                continue;

            /*
             * Send the end session messages to the thread.
             */
            CsrReferenceThread(Thread);
            CsrUnlockProcess(Process);

            /*
             * If the user says kill it, the user wants it to go away now
             * no matter what. If the user didn't say kill, then call again
             * because we need to send WM_ENDSESSION messages.
             */
            ThreadShutdownNotify(dwClientFlags, (ULONG_PTR)Thread, 0);

            CsrLockProcessByClientId(pcsrp->ClientId.UniqueProcess, &Process);
            CsrDereferenceThread(Thread);
        }
    }

KillIt:
    CsrUnlockProcess(Process);

    if (ISTS()) {
        bDoBlock = (!fNoMsgsEver && !(dwClientFlags & WMCS_EXIT));
    } else {
        bDoBlock = (!fNoRetry && !(dwClientFlags & WMCS_EXIT));
    }

    if (bDoBlock) {
        Status = SHUTDOWN_CANCEL;
        goto RestoreDesktop;
    }

    /*
     * Set the final shutdown status according to the number of gui
     * threads found.  If the count is zero, we have an unknown process.
     */
    if (cThreads == 0)
        ShutdownInfo.StatusShutdown = SHUTDOWN_UNKNOWN_PROCESS;
    else
        ShutdownInfo.StatusShutdown = SHUTDOWN_KNOWN_PROCESS;

    if (ShutdownInfo.StatusShutdown == SHUTDOWN_UNKNOWN_PROCESS ||
            !(dwClientFlags & WMCS_CONTEXTLOGOFF)) {

        /*
         * This process is not in the context being logged off.  Do
         * not terminate it and let console send an event to the process.
         */
        Status = SHUTDOWN_UNKNOWN_PROCESS;

        if (ShutdownInfo.drdRestore.hdeskNew) {
            goto RestoreDesktop;
        }
        goto CleanupAndExit;
    }

    /*
     * Calling ExitProcess() in the app's context will not always work
     * because the app may have .dll termination deadlocks: so the thread
     * will hang with the rest of the process. To ensure apps go away,
     * we terminate the process with NtTerminateProcess().
     *
     * Pass this special value, DBG_TERMINATE_PROCESS, which tells
     * NtTerminateProcess() to return failure if it can't terminate the
     * process because the app is being debugged.
     */

    if (ISTS()) {
        NTSTATUS ExitStatus;
        HANDLE DebugPort;

        ExitStatus = DBG_TERMINATE_PROCESS;
        if (NT_SUCCESS(NtQueryInformationProcess(NtCurrentProcess(),
                                                 ProcessDebugPort,
                                                 &DebugPort,
                                                 sizeof(HANDLE),
                                                 NULL)) &&
            (DebugPort != NULL)) {
            // Csr is being debugged - go ahead and kill the process
            ExitStatus = 0;
        }
        TerminateStatus = NtTerminateProcess(pcsrp->ProcessHandle, ExitStatus);
    } else {
        TerminateStatus = NtTerminateProcess(pcsrp->ProcessHandle, DBG_TERMINATE_PROCESS);
    }

    pcsrp->Flags |= CSR_PROCESS_TERMINATED;


    /*
     * Let csr know we know about this process - meaning it was our
     * responsibility to shut it down.
     */
    Status = SHUTDOWN_KNOWN_PROCESS;

RestoreDesktop:

    /*
     * Release the desktop that was used.
     */
    {
        USERTHREAD_USEDESKTOPINFO utudi;
        utudi.hThread = NULL;
        RtlCopyMemory(&(utudi.drdRestore), &(ShutdownInfo.drdRestore), sizeof(DESKRESTOREDATA));

        NtUserSetInformationThread(NtCurrentThread(), UserThreadUseDesktop,
                &utudi, sizeof(utudi));
    }

    /*
     * Now that we're done with the process handle, derefence the csr
     * process structure.
     */
    if (Status != SHUTDOWN_UNKNOWN_PROCESS) {

        /*
		 * If TerminateProcess returned STATUS_ACCESS_DENIED, then the process
		 * is being debugged and it wasn't terminated.Otherwise we need to wait
		 * anyway since TerminateProcess might return failure when the process
		 * is going away (ie STATUS_PROCESS_IS_TERMINATING).If termination
		 * indeed fail, something is wrong anyway so waiting a bit won't
		 * hurt much.
         * If we wait give the process whatever exit timeout value configured
		 * in the registry, but no less  than the 5 second Hung App timeout.

         */
        if (TerminateStatus != STATUS_ACCESS_DENIED) {
            LARGE_INTEGER li;

            li.QuadPart = (LONGLONG)-10000 * gdwProcessTerminateTimeout;
            TerminateStatus = NtWaitForSingleObject(pcsrp->ProcessHandle,
                                           FALSE,
                                           &li);
            if (TerminateStatus != STATUS_WAIT_0) {
                RIPMSG2(RIP_WARNING,
                        "UserClientShutdown: wait for process %x failed with status %x",
                        pcsrp->ClientId.UniqueProcess, TerminateStatus);
            }
        }

        CsrDereferenceProcess(pcsrp);
    }


CleanupAndExit:

    return Status;
}

/***************************************************************************\
* WMCSCallback
*
* This function is passed to SendMessageCallback when sending the
*  WM_CLIENTSHUTDOWN message. It propagates the return value back
*  if ThreadShutdownNotify is still waiting for it; otherwise,
*  it just frees the memory.
*
* 03-04-97 GerardoB     Created.
\***************************************************************************/
VOID CALLBACK WMCSCallback(HWND hwnd, UINT uMsg, ULONG_PTR dwData, LRESULT lResult)
{
    PWMCSDATA pwmcsd = (PWMCSDATA)dwData;
    if (pwmcsd->dwFlags & WMCSD_IGNORE) {
        LocalFree(pwmcsd);
        return;
    }

    pwmcsd->dwFlags |= WMCSD_REPLY;
    pwmcsd->dwRet = (DWORD)lResult;

    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(uMsg);
}
/***************************************************************************\
* GetInputWindow
*
* We assume a thread is waiting for input if it's not hung, the (main)
*  window is disabled and it owns an enabled popup.
*
* 03-06-97 GerardoB     Created.
\***************************************************************************/
HWND GetInputWindow (PCSR_THREAD pcsrt, HWND hwnd)
{
    DWORD dwTimeout;
    HWND hwndPopup;
    /*
     * Ask the kernel if the thread is hung
     */
    dwTimeout = gCmsHungAppTimeout;
    NtUserQueryInformationThread(pcsrt->ThreadHandle,
       UserThreadHungStatus, &dwTimeout, sizeof(dwTimeout), NULL);

    /*
     * If not hung and disabled, see if it owns an enabled popup
     */
    if (!dwTimeout && !IsWindowEnabled(hwnd)) {
        hwndPopup = GetWindow(hwnd, GW_ENABLEDPOPUP);
        if ((hwndPopup != NULL) && (hwndPopup != hwnd)) {
            return hwndPopup;
        }
    }

    return NULL;
}
/***************************************************************************\
* GetApplicationText
*
* Gets the text that identifies the given window or thread
*
* 08-01-97 GerardoB     Created.
\***************************************************************************/
void GetApplicationText (HWND hwnd, HANDLE hThread, WCHAR *pwcText, UINT uLen)
{
    /*
     * GetWindowText doesn't call the hwnd's proc; otherwise, we could
     *  get blocked here for good.
     */
    GetWindowText(hwnd, pwcText, uLen);
    if (*pwcText == 0) {
        /*
         * We couldn't get window title; let's try thread name
         */
        NtUserQueryInformationThread(hThread, UserThreadTaskName,
                                     pwcText, uLen * sizeof(WCHAR), NULL);
    }
}
/***************************************************************************\
* ThreadShutdownNotify
*
* This function notifies a given thread that it's time (or about time)
*  to go away. This is called from _EndTask to post the WM_CLOSE message
*  or from UserClientShutdown to send the WM_QUERYENDSESSION and
*  WM_ENDSESSION messages. If the thread fails to respond, then the
*  "End Application" dialog is brought up. This function is also called
*  from Console to display that dialog too.
*
* 03-07-97 GerardoB     Created to replace SendShutdownMessages,
*                        MySendEndSessionMessages and DoEndTaskDialog
\***************************************************************************/

DWORD ThreadShutdownNotify(DWORD dwClientFlags, ULONG_PTR dwThread, LPARAM lParam)
{
    HWND hwnd, hwndOwner;
    PWMCSDATA pwmcsd = NULL;
    HWND hwndDlg;
    ENDDLGPARAMS edp;
    DWORD dwRet, dwRealTimeout, dwTimeout, dwStartTiming, dwCmd;
    MSG msg;
    PCSR_THREAD pcsrt;
    HANDLE hThread;
    BOOL    fEndTaskNow = FALSE;

#define ESMH_CANCELEVENT     0
#define ESMH_THREAD          1
#define ESMH_HANDLECOUNT     2
    HANDLE ahandles[ESMH_HANDLECOUNT];

    /*
     * If this is console, just set up the wait loop and
     *   bring the dialog up right away. Otherwise, find
     *   the notification window, notify it and go wait.
     */
    if (dwClientFlags & WMCS_CONSOLE) {
        hThread = (HANDLE)dwThread;
        dwRealTimeout = 0;
        goto SetupWaitLoop;
    } else {
        pcsrt = (PCSR_THREAD)dwThread;
        hThread = pcsrt->ThreadHandle;
        hwnd = (HWND)lParam;
    }

    /*
     * If no window was provided,
     *  find a top-level window owned by the thread
     */
    if (hwnd == NULL) {
        EnumThreadWindows(HandleToUlong(pcsrt->ClientId.UniqueThread),
                            &FindWindowFromThread, (LPARAM)&hwnd);
    }
    if (hwnd == NULL) {
        return TSN_NOWINDOW;
    }
    /*
     * Find the root owner (we'll guess this is the "main" window)
     */
    while ((hwndOwner = GetWindow(hwnd, GW_OWNER)) != NULL) {
        hwnd = hwndOwner;
    }

#if defined(FE_IME)
    /*
     * If this is a console window, then just returns TSN_APPSAYSOK.
     * In this routine:
     * Normally windows NT environment, hwnd never point to console window.
     * However, In ConIme process, its owner window point to console window.
     */
    if (!(dwClientFlags & WMCS_ENDTASK)) {
        if ((HANDLE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE) == ghModuleWin) {
            return TSN_APPSAYSOK;
        }
    }
#endif

    /*
     * If this is an EndTask request but the window is disabled,
     *   then we want to bring the dialog up right way (the app
     *   is probably waiting for input).
     * Otherwise, we bring the window to the foreground, send/post
     *  the request and wait
     */


    /*
     * Bug 296188 - joejo
     *
     * Make sure we respond to the user ASAP when they
     * attempt to shutdown an application that we know is hung.
     */

    if ((dwClientFlags & WMCS_ENDTASK)) {

        dwTimeout = gCmsHungAppTimeout;
        NtUserQueryInformationThread(pcsrt->ThreadHandle, UserThreadHungStatus, &dwTimeout, sizeof(dwTimeout), NULL);

        if (!IsWindowEnabled(hwnd) || dwTimeout){
            dwRealTimeout = 0;
            fEndTaskNow = TRUE;
        }


    }

    if (!fEndTaskNow) {

        SetForegroundWindow(hwnd);
        dwRealTimeout = gCmsHungAppTimeout;
        if (dwClientFlags & WMCS_ENDTASK) {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
        } else {
            /*
             * If the shutdown was canceled, we don't need to wait.
             *  (we're just sending the WM_ENDSESSION(FALSE))
             */
            if (!(dwClientFlags & (WMCS_QUERYEND | WMCS_EXIT))) {
                SendNotifyMessage(hwnd, WM_CLIENTSHUTDOWN, dwClientFlags, 0);
                return TSN_APPSAYSOK;
            }
            /*
             * Allocate callback data. If out of memory, kill it.
             */
            pwmcsd = (PWMCSDATA)LocalAlloc(LPTR, sizeof(WMCSDATA));
            if (pwmcsd == NULL) {
                return TSN_USERSAYSKILL;
            }

            SendMessageCallback(hwnd, WM_CLIENTSHUTDOWN, dwClientFlags, 0,
                                WMCSCallback, (ULONG_PTR)pwmcsd);
        }
    }

SetupWaitLoop:
    /*
     * This tells us if the hwndDlg is valid. This is set/cleared by
     *  EndTaskDlgProc
     */
    ZeroMemory(&edp, sizeof(edp));
    edp.dwFlags = EDPF_NODLG;
    /*
     * Loop until the hwnd replies, the request is canceled
     *  or the thread goes away. It time out, bring up the
     *  dialog and wait until the user tells us what to do.
     */
    *(ahandles + ESMH_CANCELEVENT) = gheventCancel;
    *(ahandles + ESMH_THREAD) = hThread;
    dwStartTiming = GetTickCount();
    dwCmd = 0;
    while (dwCmd == 0) {
        /*
         * Calculate how long we have to wait.
         */
        dwTimeout = dwRealTimeout;
        if ((dwTimeout != 0) && (dwTimeout != INFINITE)) {
            dwTimeout -= (GetTickCount() - dwStartTiming);
            if ((int)dwTimeout < 0) {
                dwTimeout = 0;
            }
        }

        dwRet = MsgWaitForMultipleObjects(ESMH_HANDLECOUNT, ahandles, FALSE, dwTimeout, QS_ALLINPUT);

        switch (dwRet) {
            case WAIT_OBJECT_0 + ESMH_CANCELEVENT:
                /*
                 * The request has been canceled.
                 */
                dwCmd = TSN_USERSAYSCANCEL;
                break;

            case WAIT_OBJECT_0 + ESMH_THREAD:
                /*
                 * The thread is gone.
                 */
                dwCmd = TSN_APPSAYSOK;
                break;

            case WAIT_OBJECT_0 + ESMH_HANDLECOUNT:
                /*
                 * We got some input; process it.
                 */
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                    if ((edp.dwFlags & EDPF_NODLG)
                            || !IsDialogMessage(hwndDlg, &msg)) {

                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }
                /*
                 * If we got a reply to the message, act on it
                 */
                if ((pwmcsd != NULL) && (pwmcsd->dwFlags & WMCSD_REPLY)) {

                    switch (pwmcsd->dwRet) {
                        default:
                            /*
                             * If the message was not processed (the thread
                             *  exited) or someone processed it and returned
                             *  a bogus value, just shut them down.
                             * Fall through
                             */
                        case WMCSR_ALLOWSHUTDOWN:
                            /*
                             * We're going to nuke this app, so get rid of
                             *  any pending harderror boxes he might have
                             */
                            BoostHardError((ULONG_PTR)pcsrt->ClientId.UniqueProcess, BHE_FORCE);
                            /*
                             * Fall through.
                             */
                        case WMCSR_DONE:
                            dwCmd = TSN_APPSAYSOK;
                            break;

                        case WMCSR_CANCEL:
                            dwCmd = TSN_APPSAYSNOTOK;
                            break;
                    }
                }
                /*
                 * Else if the dialog is still up, keep waiting for the user
                 *  to tell us what to do
                 */
                else if (!(edp.dwFlags & EDPF_NODLG)) {
                    break;
                }
                /*
                 * Else if the user dismissed the dialog, act on his response
                 */
                else if (edp.dwFlags & EDPF_RESPONSE) {
                    switch(edp.dwRet) {
                        case IDC_ENDNOW:
                            /*
                             * The user wants us to kill it
                             */
                            dwCmd = TSN_USERSAYSKILL;
                            break;

                        /* case IDCANCEL: */
                        default:
                            dwCmd = TSN_USERSAYSCANCEL;
                            break;
                    }
                }
                break;

            case WAIT_TIMEOUT:

                if (dwClientFlags & WMCS_NORETRY) {

                    /*
                     * We come here only for Terminal Server case. We return
                     * TSN_APPSAYSOK as Terminal Server 4 did in this case.
                     */
                    UserAssert(ISTS());

                    dwCmd = TSN_APPSAYSOK;
                    break;
                }


                /*
                 * Once we time out, we bring up the dialog and let
                 *  its timer take over.
                 */
                dwRealTimeout = INFINITE;
                /*
                 * Check if the windows app is waiting for input;
                 *  if not, we assume it is hung for EndTask;
                 *   otherwise we enter a wait mode that brings the
                 *   dialog up just to provide some (waiting) feedback
                 * Console just gets the dialog right away
                 */
                if (!(dwClientFlags & WMCS_CONSOLE)) {
                    if (BoostHardError((ULONG_PTR)pcsrt->ClientId.UniqueProcess, BHE_TEST)
                           || (GetInputWindow(pcsrt, hwnd) != NULL)) {

                        edp.dwFlags |= EDPF_INPUT;
                    } else {
                        /*
                         * EWX_FORCEIFHUNG support.
                         * Also, if this is an ExitWindows call and the process is not in
                         * the context being logged off, we won't kill it.
                         * So don't bother asking the user what to do
                         */
                        if ((dwClientFlags & WMCS_NODLGIFHUNG)
                                || (!(dwClientFlags & WMCS_ENDTASK)
                                        && !(dwClientFlags & WMCS_CONTEXTLOGOFF))) {

                            dwCmd = TSN_USERSAYSKILL;
                            break;
                        }
                        /*
                         * Hung or Wait?
                         */
                        if (dwClientFlags & WMCS_ENDTASK) {
                            edp.dwFlags |= EDPF_HUNG;
                        } else {
                            edp.dwFlags |= EDPF_WAIT;
                        }
                    }
                }

                /*
                 * If the registry says no dialog, then tell the caller
                 *  the user wants to kill the app.
                 */
                if (gfAutoEndTask) {
                    dwCmd = TSN_USERSAYSKILL;
                    break;
                }
                /*
                 * Setup the parameters needed by EndTaskDlgProc
                 */
                edp.dwRet = 0;
                edp.dwClientFlags = dwClientFlags;
                if (dwClientFlags & WMCS_CONSOLE) {
                    edp.pcsrt = NULL;
                    edp.lParam = lParam;
                } else {
                    edp.pcsrt = pcsrt;
                    edp.lParam = (LPARAM)hwnd;
                }

                hwndDlg = CreateDialogParam (ghModuleWin, MAKEINTRESOURCE(IDD_ENDTASK),
                                        NULL, EndTaskDlgProc, (LPARAM)(&edp));
                /*
                 * If we cannot ask the user, then kill the app.
                 */
                if (hwndDlg == NULL) {
                    edp.dwFlags |= EDPF_NODLG;
                    dwCmd = TSN_USERSAYSKILL;
                    break;
                }
                break;

            default:
                /*
                 * Unexpected return; something is wrong. Kill the app.
                 */
                UserAssert(dwRet != dwRet);
                dwCmd = TSN_USERSAYSKILL;
                break;
        } /* switch (dwRet) */
    } /* while (dwCmd == 0) */


    /*
     * If the dialog is up, nuke it.
     */
    if (!(edp.dwFlags & EDPF_NODLG)) {
        DestroyWindow(hwndDlg);
    }
    /*
     * Make sure pwmcsd is freed or marked to be freed by WMCSCallback
     *  when the reply comes
     */
    if (pwmcsd != NULL) {
        if (pwmcsd->dwFlags & WMCSD_REPLY) {
            LocalFree(pwmcsd);
        } else {
            pwmcsd->dwFlags |= WMCSD_IGNORE;
        }
    }
#if DBG
    /*
     * If Canceling, let's the name the app that doesn't let us log off
     */
    if ((dwClientFlags & WMCS_EXIT) && (dwCmd == TSN_APPSAYSNOTOK)) {
        WCHAR achTitle[CCHBODYMAX];
        WCHAR *pwcText;
        UserAssert(!(dwClientFlags & WMCS_CONSOLE));
        pwcText = achTitle;
        *(achTitle + CCHBODYMAX - 1) = (WCHAR)0;
        GetApplicationText(hwnd, hThread, pwcText, CCHBODYMAX - 1);
        KdPrint(("Log off canceled by pid:%#lx tid:%#lx - '%ws'.\n",
                    HandleToUlong(pcsrt->ClientId.UniqueProcess),
                    HandleToUlong(pcsrt->ClientId.UniqueThread),
                    pwcText));
    }
#endif
    /*
     * If we're killing this dude, clean any hard errors.
     * Also if wow takes care of it, then our caller doesn't need to
     */
    if ((dwCmd == TSN_USERSAYSKILL) && !(dwClientFlags & WMCS_CONSOLE)) {

        BoostHardError((ULONG_PTR)pcsrt->ClientId.UniqueProcess, BHE_FORCE);

        if (!(pcsrt->Flags & CSR_THREAD_DESTROYED) && WowExitTask(pcsrt)) {
            dwCmd = TSN_APPSAYSOK;
        }
    }

    return dwCmd;
}

/***************************************************************************\
* SetEndTaskDlgStatus
*
* Displays the appropiate message and shows the dialog
*
* 03-11-97 GerardoB     Created
\***************************************************************************/
void SetEndTaskDlgStatus(ENDDLGPARAMS *pedp, HWND hwndDlg, UINT uStrId, BOOL fInit)
{
    BOOL f, fIsWaiting, fWasWaiting;
    WCHAR *pwcText;

    fWasWaiting = (pedp->uStrId == STR_ENDTASK_WAIT);
    fIsWaiting = (pedp->dwFlags & EDPF_WAIT);
    /*
     * Store the current message id, load it and show it
     */
    pedp->uStrId = uStrId;
    pwcText = ServerLoadString(ghModuleWin, uStrId, NULL, &f);
    if (pwcText != NULL) {
        SetDlgItemText(hwndDlg, IDC_STATUSMSG, pwcText);
        LocalFree(pwcText);
    }
    /*
     * If we haven't decided that the app is hung, set a
     *  timer to keep an eye on it.
     */
    if (!(pedp->dwFlags & EDPF_HUNG) && !(pedp->dwClientFlags & WMCS_CONSOLE)) {
        SetTimer(hwndDlg, IDT_CHECKAPPSTATE, gCmsHungAppTimeout, NULL);
    }
    /*
     * If initializing or switching to/from the wait mode,
     *  set the proper status for IDC_STATUSCANCEL, IDCANCEL,
     *  IDC_ENDNOW and  the start/stop the progress bar.
     *  Invalidate paint if/as needed
     */
    if (fInit || (fIsWaiting ^ fWasWaiting)) {
        RECT rc;
        HWND hwndStatusCancel = GetDlgItem(hwndDlg, IDC_STATUSCANCEL);
        HWND hwndCancelButton = GetDlgItem(hwndDlg, IDCANCEL);
        HWND hwndEndButton = GetDlgItem(hwndDlg, IDC_ENDNOW);
        DWORD dwSwpFlags;
        /*
         * If on wait mode, we hide the cancel button and its
         *  explanatory text. The End button will be moved to
         *  the cancel button position.
         */
        dwSwpFlags = ((fIsWaiting ? SWP_HIDEWINDOW : SWP_SHOWWINDOW)
                                | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOMOVE
                                | SWP_NOZORDER | SWP_NOSENDCHANGING
                                | SWP_NOACTIVATE);
        /*
         * If we're hiding the cancel button, give focus/def id to
         *  the End button.
         * Note that DM_SETDEIF won't do the right thing unless
         *  both Cancel/End buttons are visible.
         */
        if (fIsWaiting) {
            SendMessage(hwndDlg, DM_SETDEFID, IDC_ENDNOW, 0);
            SetFocus(hwndEndButton);
        }
        SetWindowPos(hwndStatusCancel, NULL, 0, 0, 0, 0, dwSwpFlags);
        SetWindowPos(hwndCancelButton, NULL, 0, 0, 0, 0, dwSwpFlags);
        /*
         * If the cancel button is visible, give it focus/def id.
         */
        if (!fIsWaiting) {
            SendMessage(hwndDlg, DM_SETDEFID, IDCANCEL, 0);
            SetFocus(hwndCancelButton);
        }
        /*
         * Initialize progress bar (first time around)
         */
        if (fIsWaiting && (pedp->hbrProgress == NULL)) {
            int iMagic;
            /*
             * Initialize progress bar stuff.
             * The size and location calculations below were made up
             *  to make it look good(?).
             * We need that location on dialog coordiantes since the
             *  progress bar is painted on the dialog's WM_PAINT.
             */
            GetClientRect(hwndStatusCancel, &pedp->rcBar);
            iMagic = (pedp->rcBar.bottom - pedp->rcBar.top) / 4;
            InflateRect(&pedp->rcBar, 0, -iMagic + GetSystemMetrics(SM_CYEDGE));
            pedp->rcBar.right -= (5 * iMagic);
            OffsetRect(&pedp->rcBar, 0, -iMagic);
            MapWindowPoints(hwndStatusCancel, hwndDlg, (LPPOINT)&pedp->rcBar, 2);
            /*
             * Calculate drawing rectangle and dimensions.
             *  We kind of make it look like comctrl's progress bar.
             */
            pedp->rcProgress = pedp->rcBar;
            InflateRect(&pedp->rcProgress, -GetSystemMetrics(SM_CXEDGE), -GetSystemMetrics(SM_CYEDGE));
            pedp->iProgressStop = pedp->rcProgress.right;
            pedp->iProgressWidth = ((2 * (pedp->rcProgress.bottom - pedp->rcProgress.top)) / 3);

            pedp->rcProgress.right = pedp->rcProgress.left + pedp->iProgressWidth - 1;

            pedp->hbrProgress = CreateSolidBrush(GetSysColor(COLOR_ACTIVECAPTION));
            /*
             * Remember the End button position.
             */
            GetWindowRect(hwndEndButton, &pedp->rcEndButton);
            MapWindowPoints(NULL, hwndDlg, (LPPOINT)&pedp->rcEndButton, 2);
        }
        /*
         * Start/Stop progress bar and set End button position
         */
        if (fIsWaiting) {
            RECT rcEndButton;
            UINT uTimeout = (gdwHungToKillCount * gCmsHungAppTimeout)
                            / ((pedp->iProgressStop - pedp->rcProgress.left) / pedp->iProgressWidth);
            SetTimer(hwndDlg, IDT_PROGRESS, uTimeout, NULL);
            /*
             * The Cancel and the End buttons might have different widths when
             *  localized. So make sure we position it inside the dialog and
             *  to the right end of the dialog.
             */
            GetWindowRect(hwndCancelButton, &rc);
            GetWindowRect(hwndEndButton, &rcEndButton);
            rc.left = rc.right - (rcEndButton.right - rcEndButton.left);
            MapWindowPoints(NULL, hwndDlg, (LPPOINT)&rc, 2);
        } else if (fWasWaiting) {
            KillTimer(hwndDlg, IDT_PROGRESS);
            rc = pedp->rcEndButton;
        }
        /*
         * Move the End button if needed
         */
        if (fIsWaiting || fWasWaiting) {
            SetWindowPos(hwndEndButton, NULL, rc.left, rc.top, 0, 0,
                            SWP_NOREDRAW | SWP_NOSIZE | SWP_NOACTIVATE
                            | SWP_NOZORDER | SWP_NOSENDCHANGING);
        }
        /*
         * Make sure we repaint if needed
         */
        if (!fInit) {
            InvalidateRect(hwndDlg, NULL, TRUE);
        }
    } /* if (fInit || (fIsWaiting ^ fWasWaiting)) */
    /*
     * If initializing or in hung mode, make sure the user can
     * see the dialog; only bring it to the foreground on
     * initialization (no rude focus stealing)
     */
    if (fInit || (pedp->dwFlags & EDPF_HUNG)) {
        SetWindowPos(hwndDlg, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW
                     | SWP_NOACTIVATE | SWP_NOSENDCHANGING);

        if (fInit) {
            SetForegroundWindow(hwndDlg);
        }
    }
}
/***************************************************************************\
* EndTaskDlgProc
*
* This is the dialog procedure for the dialog that comes up when an app is
* not responding.
*
* 03-06-97 GerardoB     Rewrote it once again. New template though
* 07-23-92 ScottLu      Rewrote it, but used same dialog template.
* 04-28-92 JonPa        Created.
\***************************************************************************/

INT_PTR APIENTRY EndTaskDlgProc(
    HWND hwndDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    ENDDLGPARAMS* pedp;
    WCHAR achTitle[CCHBODYMAX];
    WCHAR *pwcText, *pwcTemp;
    UINT uLen;
    UINT uStrId;
    PAINTSTRUCT ps;
    HDC hdc, hdcMem;
    BOOL fIsInput, fWasInput;
#ifdef USE_MIRRORING
    int iOldLayout;
#endif

    switch (msg) {
    case WM_INITDIALOG:
        /*
         * Save parameters
         */
        pedp = (ENDDLGPARAMS*)lParam;
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (ULONG_PTR)pedp);
        /*
         * This tells the caller that the dialog is up
         */
        pedp->dwFlags &= ~EDPF_NODLG;
        /*
         * Build the dialog title making sure that
         *  we end up with a NULL terminated string.
         */
        *(achTitle + CCHBODYMAX - 1) = (WCHAR)0;
        uLen = GetWindowText(hwndDlg, achTitle, CCHBODYMAX - 1);
        pwcText = achTitle + uLen;
        uLen = CCHBODYMAX - 1 - uLen;
        /*
         * Console provides the title; we figure it out for windows apps.
         */
        if (pedp->dwClientFlags & WMCS_CONSOLE) {
            pwcTemp = (WCHAR *)pedp->lParam;
            while (uLen-- && (*pwcText++ = *pwcTemp++));
        } else {
            GetApplicationText((HWND)pedp->lParam, pedp->pcsrt->ThreadHandle, pwcText, uLen);
        }

        SetWindowText(hwndDlg, achTitle);
        /*
         * Get the app's icon: first look for atomIconProperty
         * if not available, try the class icon.
         * else, use the default icon.
         */
        pedp->hIcon = (HICON)GetProp((HWND)pedp->lParam, ICON_PROP_NAME);

        if (pedp->hIcon == NULL) {

            pedp->hIcon = (HICON)GetClassLongPtr((HWND)pedp->lParam, GCLP_HICON);

            if (pedp->hIcon == NULL) {

                if (pedp->dwClientFlags & WMCS_CONSOLE) {
                    pedp->hIcon = LoadIcon(ghModuleWin, MAKEINTRESOURCE(IDI_CONSOLE));
                }
                else {
                    pedp->hIcon = LoadIcon(NULL, IDI_APPLICATION);
                }
            }
        }

        /*
         * Figure out what message the caller wants initially
         */
        if (pedp->dwClientFlags & WMCS_CONSOLE) {
            uStrId = STR_ENDTASK_CONSOLE;
        } else if (pedp->dwFlags & EDPF_INPUT) {
            uStrId = STR_ENDTASK_INPUT;
        } else if (pedp->dwFlags & EDPF_WAIT) {
            uStrId = STR_ENDTASK_WAIT;
        } else {
            uStrId = STR_ENDTASK_HUNG;
        }
        /*
         * Display the message, set the focus and show the dialog
         */
        SetEndTaskDlgStatus(pedp, hwndDlg, uStrId, TRUE);
        return FALSE;


    case WM_PAINT:
        pedp = (ENDDLGPARAMS*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
        if ((pedp == NULL) || (pedp->hIcon == NULL)) {
            break;
        }
        /*
         * Draw the icon
         */
        hdc = BeginPaint(hwndDlg, &ps);
#ifdef USE_MIRRORING
        iOldLayout = GetLayout(hdc);
        if (iOldLayout != GDI_ERROR) {
            SetLayout(hdc, iOldLayout|LAYOUT_BITMAPORIENTATIONPRESERVED);
        }
#endif
        DrawIcon(hdc, ETD_XICON, ETD_YICON, pedp->hIcon);
#ifdef USE_MIRRORING
        if (iOldLayout != GDI_ERROR) {
            SetLayout(hdc, iOldLayout);
        }
#endif
        /*
         * If waiting, draw the progress bar;
         *  else draw the warning sign
         */
        if (pedp->dwFlags & EDPF_WAIT) {
            RECT rc;
            /*
             * Draw a client-edge-looking border.
             */
            rc = pedp->rcBar;
            DrawEdge(hdc, &rc, BDR_SUNKENOUTER, BF_RECT | BF_ADJUST);
            InflateRect(&rc, -1, -1);
            /*
             * Draw the blocks up to current position
             */
            rc.right = rc.left + pedp->iProgressWidth - 1;
            while (rc.left < pedp->rcProgress.left) {
                if (rc.right > pedp->iProgressStop) {
                    rc.right = pedp->iProgressStop;
                    if (rc.left >= rc.right) {
                        break;
                    }
                }
                FillRect(hdc, &rc, pedp->hbrProgress);
                rc.left += pedp->iProgressWidth;
                rc.right += pedp->iProgressWidth;
            }
        } else {
            /*
             * Load the bitmap the first time around and
             *  figure out where it goes
            */
            if (pedp->hbmpWarning == NULL) {
                BITMAP bmp;
                pedp->hbmpWarning = LoadBitmap(ghModuleWin, MAKEINTRESOURCE(IDB_WARNING));
                if (GetObject(pedp->hbmpWarning, sizeof(bmp), &bmp)) {
                    pedp->rcWarning.left = ETD_XICON;
                    pedp->rcWarning.top = ETD_XICON + 32 - bmp.bmHeight;
                    pedp->rcWarning.right = bmp.bmWidth;
                    pedp->rcWarning.bottom = bmp.bmHeight;
                }
            }
            /*
             * Blit it
             */
            hdcMem = CreateCompatibleDC(hdc);
            SelectObject(hdcMem, pedp->hbmpWarning);
            GdiTransparentBlt(hdc, pedp->rcWarning.left, pedp->rcWarning.top,
                   pedp->rcWarning.right, pedp->rcWarning.bottom,
                   hdcMem, 0, 0, pedp->rcWarning.right, pedp->rcWarning.bottom, RGB(255, 0, 255));
            DeleteDC(hdcMem);
        }

        EndPaint(hwndDlg, &ps);
        return TRUE;

    case WM_TIMER:
        pedp = (ENDDLGPARAMS*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
        if (pedp == NULL) {
            return TRUE;
        }
        switch (wParam) {
        case IDT_CHECKAPPSTATE:
            pedp->dwCheckTimerCount++;
            /*
             * Check if the app has switched from/to a waiting-for-input
             *  mode. If so, update the dialog and wait a little longer
             */
            fIsInput = (BoostHardError((ULONG_PTR)pedp->pcsrt->ClientId.UniqueProcess, BHE_TEST)
                        || (GetInputWindow(pedp->pcsrt, (HWND)pedp->lParam) != NULL));
            fWasInput = (pedp->dwFlags & EDPF_INPUT);
            if (fIsInput ^ fWasInput) {
                UINT uProgress;
                pedp->dwFlags &= ~(EDPF_INPUT | EDPF_WAIT);
                pedp->dwFlags |= (fIsInput ? EDPF_INPUT : EDPF_WAIT);
                SetEndTaskDlgStatus(pedp, hwndDlg,
                                    (fIsInput ? STR_ENDTASK_INPUT : STR_ENDTASK_WAIT),
                                     FALSE);
                pedp->dwCheckTimerCount /= 2;
                uProgress = pedp->rcProgress.left - pedp->rcBar.left - GetSystemMetrics(SM_CXEDGE);
                uProgress /= 2;
                pedp->rcProgress.left -= uProgress;
                pedp->rcProgress.right -= uProgress;
            }
            /*
             * Is it time to declare it hung?
             */
            if (pedp->dwCheckTimerCount >= gdwHungToKillCount) {
                KillTimer(hwndDlg, IDT_CHECKAPPSTATE);
                pedp->dwFlags &= ~(EDPF_INPUT | EDPF_WAIT);
                pedp->dwFlags |= EDPF_HUNG;
                SetEndTaskDlgStatus(pedp, hwndDlg, STR_ENDTASK_HUNG, FALSE);
            }
        break;

        case IDT_PROGRESS:
            /*
             * Draw the next block in the progress bar.
             */
            if (pedp->rcProgress.right >= pedp->iProgressStop) {
                pedp->rcProgress.right = pedp->iProgressStop;
                if (pedp->rcProgress.left >= pedp->rcProgress.right) {
                    break;
                }
            }
            hdc = GetDC(hwndDlg);
            FillRect(hdc, &pedp->rcProgress, pedp->hbrProgress);
            ReleaseDC(hwndDlg, hdc);
            pedp->rcProgress.left += pedp->iProgressWidth;
            pedp->rcProgress.right += pedp->iProgressWidth;
        break;
        }
        return TRUE;


    case WM_NCACTIVATE:
        /*
         * Make sure we're uncovered when active and not covering the app
         *  when inactive
         */
        pedp = (ENDDLGPARAMS*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
        if (pedp != NULL) {
            HWND hwnd;
            if (wParam) {
                hwnd = HWND_TOPMOST;
            } else if (pedp->dwClientFlags & WMCS_CONSOLE) {
                hwnd = HWND_TOP;
            } else {
                hwnd = (HWND)pedp->lParam;
            }
            SetWindowPos(hwndDlg, hwnd,
                         0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        }
        break;


    case WM_COMMAND:
        /*
         * The user has made a choice, we're done.
         */
        pedp = (ENDDLGPARAMS*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
        if (pedp != NULL) {
            pedp->dwRet = (DWORD)wParam;
        }
        DestroyWindow(hwndDlg);
        break;


    case WM_DESTROY:
        /*
         * We're dead. Make sure the caller knows we're history
         */
        pedp = (ENDDLGPARAMS*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
        if (pedp != NULL) {
            pedp->dwFlags |= (EDPF_NODLG | EDPF_RESPONSE);
            if (pedp->hbmpWarning != NULL) {
                DeleteObject(pedp->hbmpWarning);
            }
            if (pedp->hbrProgress != NULL) {
                DeleteObject(pedp->hbrProgress);
            }
        }
        break;
    }

    return FALSE;
}

/***************************************************************************\
* _EndTask
*
* This routine is called from the task manager to end an application - for
* gui apps, either a win32 app or a win16 app. Note: Multiple console
* processes can live in a single console window. We'll pass these requests
* for destruction to console.
*
* 07-25-92 ScottLu      Created.
\***************************************************************************/

BOOL _EndTask(
    HWND hwnd,
    BOOL fShutdown,
    BOOL fMeanKill)
{
    BOOL fRet = TRUE;
    PCSR_THREAD pcsrt = CSR_SERVER_QUERYCLIENTTHREAD();
    PCSR_THREAD pcsrtKill;
    DWORD dwThreadId;
    DWORD dwProcessId;
    LPWSTR lpszMsg;
    BOOL fAllocated;
    DWORD dwCmd;
    USERTHREAD_USEDESKTOPINFO utudi;
    NTSTATUS Status;

    /*
     * Note: fShutdown isn't used for anything in this routine!
     * They are still there because I haven't removed them: the old endtask
     * code relied on them.
     */
    UNREFERENCED_PARAMETER(fShutdown);

    /*
     * Set the current work thread to a desktop so we can
     *  go safely into win32k.sys
     */
    utudi.hThread = pcsrt->ThreadHandle;
    utudi.drdRestore.pdeskRestore = NULL;

    Status = NtUserSetInformationThread(NtCurrentThread(),
            UserThreadUseDesktop, &utudi, sizeof(utudi));
    if (!NT_SUCCESS(Status)) {
        /*
         * We were unable to get the thread's desktop. Game over
         */
        return TRUE;
    }


    /*
     * Get the process and thread that owns hwnd.
     */
    dwThreadId = GetWindowThreadProcessId(hwnd, &dwProcessId);
    if (dwThreadId == 0) {
        goto RestoreDesktop;
    }

    /*
     * Don't allow destruction of winlogon
     */
    if (dwProcessId == gIdLogon) {
        goto RestoreDesktop;
    }

    /*
     * If this is a console window, then just send the close message to
     * it, and let console clean up the processes in it.
     */
    if ((HANDLE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE) == ghModuleWin) {
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        goto RestoreDesktop;
    }

    /*
     * Find the CSR_THREAD for the window.
     */
    CsrLockThreadByClientId((HANDLE)LongToHandle( dwThreadId ), &pcsrtKill);
    if (pcsrtKill == NULL) {
        goto RestoreDesktop;
    }
    CsrReferenceThread(pcsrtKill);
    CsrUnlockThread(pcsrtKill);

    /*
     * If this is a WOW app, then shutdown just this wow application.
     */
    if (!fMeanKill) {
        /*
         * Find out what to do now - did the user cancel or the app cancel,
         * etc? Only allow cancelling if we are not forcing the app to
         * exit.
         */
        dwCmd = ThreadShutdownNotify(WMCS_ENDTASK, (ULONG_PTR)pcsrtKill, (LPARAM)hwnd);

        switch (dwCmd) {
        case TSN_APPSAYSNOTOK:
            /*
             * App says not ok - this'll let taskman bring up the "are you sure?"
             * dialog to the user.
             */
            CsrDereferenceThread(pcsrtKill);
            fRet = FALSE;
            goto RestoreDesktop;

        case TSN_USERSAYSCANCEL:
            /*
             * User hit cancel on the timeout dialog - so the user really meant
             * it. Let taskman know everything is ok by returning TRUE.
             */
            CsrDereferenceThread(pcsrtKill);
            goto RestoreDesktop;
        }
    }

    /*
     * Kill the application now.  If the thread has not been destroyed,
     * nuke the task.  If WowExitTask returns that the thread is not
     * a WOW task, terminate the process.
     */
    if (!(pcsrtKill->Flags & CSR_THREAD_DESTROYED) && !WowExitTask(pcsrtKill)) {

        BOOL bDoBlock;

        /*
         * Calling ExitProcess() in the app's context will not always work
         * because the app may have .dll termination deadlocks: so the thread
         * will hang with the rest of the process. To ensure apps go away,
         * we terminate the process with NtTerminateProcess().
         *
         * Pass this special value, DBG_TERMINATE_PROCESS, which tells
         * NtTerminateProcess() to return failure if it can't terminate the
         * process because the app is being debugged.
         */
        if (ISTS()) {
            NTSTATUS ExitStatus;
            HANDLE DebugPort;

            ExitStatus = DBG_TERMINATE_PROCESS;
            if (NT_SUCCESS(NtQueryInformationProcess(NtCurrentProcess(),
                                                     ProcessDebugPort,
                                                     &DebugPort,
                                                     sizeof(HANDLE),
                                                     NULL)) &&
                (DebugPort != NULL)) {
                // Csr is being debugged - go ahead and kill the process
                ExitStatus = 0;
            }
            if (!NT_SUCCESS(NtTerminateProcess(pcsrtKill->Process->ProcessHandle,
                    ExitStatus))) {

                bDoBlock = TRUE;
            } else {
                bDoBlock = FALSE;
            }
        } else {
            if (!NT_SUCCESS(NtTerminateProcess(pcsrtKill->Process->ProcessHandle,
                    DBG_TERMINATE_PROCESS))) {
                bDoBlock = TRUE;
            } else {
                bDoBlock = FALSE;
            }
        }

        if (bDoBlock) {

            /*
             * If the app is being debugged, don't close it - because that can
             * cause a hang to the NtTerminateProcess() call.
             */
            lpszMsg = ServerLoadString(ghModuleWin, STR_APPDEBUGGED,
                    NULL, &fAllocated);
            if (lpszMsg) {
                MessageBoxEx(NULL, lpszMsg, NULL, MB_OK | MB_SETFOREGROUND, 0);
                LocalFree(lpszMsg);
            }
        } else {
            pcsrtKill->Process->Flags |= CSR_PROCESS_TERMINATED;
        }
    }
    CsrDereferenceThread(pcsrtKill);

RestoreDesktop:
    utudi.hThread = NULL;
    Status = NtUserSetInformationThread(NtCurrentThread(),
            UserThreadUseDesktop, &utudi, sizeof(utudi));
    UserAssert(NT_SUCCESS(Status));

    return fRet;
}

/***************************************************************************\
* WowExitTask
*
* Calls wow back to make sure a specific task has exited.  Returns
* TRUE if the thread is a WOW task, FALSE if not.
*
* 08-02-92 ScottLu      Created.
\***************************************************************************/

BOOL WowExitTask(
    PCSR_THREAD pcsrt)
{
    HANDLE ahandle[2];
    USERTHREAD_WOW_INFORMATION WowInfo;
    NTSTATUS Status;

    ahandle[1] = gheventCancel;

    /*
     * Query task id and exit function.
     */
    Status = NtUserQueryInformationThread(pcsrt->ThreadHandle,
            UserThreadWOWInformation, &WowInfo, sizeof(WowInfo), NULL);
    if (!NT_SUCCESS(Status))
        return FALSE;

    /*
     * If no task id was returned, it is not a WOW task
     */
    if (WowInfo.hTaskWow == 0)
        return FALSE;

    /*
     * Try to make it exit itself. This will work most of the time.
     * If this doesn't work, terminate this process.
     */
    ahandle[0] = InternalCreateCallbackThread(pcsrt->Process->ProcessHandle,
                                              (ULONG_PTR)WowInfo.lpfnWowExitTask,
                                              (ULONG_PTR)WowInfo.hTaskWow);
    if (ahandle[0] == NULL) {
        NtTerminateProcess(pcsrt->Process->ProcessHandle, 0);
        pcsrt->Process->Flags |= CSR_PROCESS_TERMINATED;
        goto Exit;
    }

    WaitForMultipleObjects(2, ahandle, FALSE, INFINITE);
    NtClose(ahandle[0]);

Exit:
    return TRUE;
}

/***************************************************************************\
* InternalWaitCancel
*
* Console calls this to wait for objects or shutdown to be cancelled
*
* 29-Oct-1992 mikeke    Created
\***************************************************************************/

DWORD InternalWaitCancel(
    HANDLE handle,
    DWORD dwMilliseconds)
{
    HANDLE ahandle[2];

    ahandle[0] = handle;
    ahandle[1] = gheventCancel;

    return WaitForMultipleObjects(2, ahandle, FALSE, dwMilliseconds);
}


/***************************************************************************\
* InternalCreateCallbackThread
*
* This routine creates a remote thread in the context of a given process.
* It is used to call the console control routine, as well as ExitProcess when
* forcing an exit. Returns a thread handle.
*
* 07-28-92 ScottLu      Created.
\***************************************************************************/

HANDLE InternalCreateCallbackThread(
    HANDLE hProcess,
    ULONG_PTR lpfn,
    ULONG_PTR dwData)
{
    LONG BasePriority;
    HANDLE hThread, hToken;
    PTOKEN_DEFAULT_DACL lpDaclDefault;
    TOKEN_DEFAULT_DACL daclDefault;
    ULONG cbDacl;
    SECURITY_ATTRIBUTES attrThread;
    SECURITY_DESCRIPTOR sd;
    DWORD idThread;
    NTSTATUS Status;

    hThread = NULL;

    Status = NtOpenProcessToken(hProcess, TOKEN_QUERY, &hToken);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("NtOpenProcessToken failed, status = %x\n", Status));
        return NULL;
    }

    cbDacl = 0;
    NtQueryInformationToken(hToken,
            TokenDefaultDacl,
            &daclDefault,
            sizeof(daclDefault),
            &cbDacl);

    lpDaclDefault = (PTOKEN_DEFAULT_DACL)LocalAlloc(LMEM_FIXED, cbDacl);
    if (lpDaclDefault == NULL) {
        KdPrint(("LocalAlloc failed for lpDaclDefault"));
        goto closeexit;
    }

    Status = NtQueryInformationToken(hToken,
            TokenDefaultDacl,
            lpDaclDefault,
            cbDacl,
            &cbDacl);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("NtQueryInformationToken failed, status = %x\n", Status));
        goto freeexit;
    }

    if (!NT_SUCCESS(RtlCreateSecurityDescriptor(&sd,
            SECURITY_DESCRIPTOR_REVISION1))) {
        UserAssert(FALSE);
        goto freeexit;
    }

    RtlSetDaclSecurityDescriptor(&sd, TRUE, lpDaclDefault->DefaultDacl, TRUE);

    attrThread.nLength = sizeof(attrThread);
    attrThread.lpSecurityDescriptor = &sd;
    attrThread.bInheritHandle = FALSE;

    GetLastError();
    hThread = CreateRemoteThread(hProcess,
        &attrThread,
        0L,
        (LPTHREAD_START_ROUTINE)lpfn,
        (LPVOID)dwData,
        0,
        &idThread);

    if (hThread != NULL) {
        BasePriority = THREAD_PRIORITY_HIGHEST;
        NtSetInformationThread(hThread,
                               ThreadBasePriority,
                               &BasePriority,
                               sizeof(LONG));
    }

freeexit:
    LocalFree((HANDLE)lpDaclDefault);

closeexit:
    NtClose(hToken);

    return hThread;
}

ULONG
SrvExitWindowsEx(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    BEGIN_LPC_RECV(EXITWINDOWSEX);

    Status = _ExitWindowsEx(pcsrt, a->uFlags, a->dwReserved);
    a->fSuccess = NT_SUCCESS(Status);

    END_LPC_RECV();
}

ULONG
SrvEndTask(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    PENDTASKMSG petm = (PENDTASKMSG)&m->u.ApiMessageData;
    PCSR_THREAD pcsrt;
    PTEB Teb = NtCurrentTeb();

    Teb->LastErrorValue = 0;
    pcsrt = CSR_SERVER_QUERYCLIENTTHREAD();
    /*
     * Don't block the client so it can respond to messages while we
     *  process this request -- we might bring up the End Application
     *  dialog or the hwnd being shutdown might request some user action.
     */
    if (pcsrt->Process->ClientPort != NULL) {
        m->ReturnValue = STATUS_SUCCESS;
        petm->dwLastError = 0;
        petm->fSuccess = TRUE;
        NtReplyPort(pcsrt->Process->ClientPort, (PPORT_MESSAGE)m);
        *ReplyStatus = CsrServerReplied;
    }

    petm->fSuccess = _EndTask(petm->hwnd, petm->fShutdown, petm->fForce);

    petm->dwLastError = Teb->LastErrorValue;
    return STATUS_SUCCESS;
}

/***************************************************************************\
* IsPrivileged
*
* Check to see if the client has the specified privileges
*
* History:
* 01-02-91 JimA       Created.
\***************************************************************************/

BOOL IsPrivileged(
    PPRIVILEGE_SET ppSet)
{
    HANDLE hToken;
    NTSTATUS Status;
    BOOLEAN bResult = FALSE;
    UNICODE_STRING strSubSystem;

    /*
     * Impersonate the client
     */
    if (!CsrImpersonateClient(NULL))
        return FALSE;

    /*
     * Open the client's token
     */
    RtlInitUnicodeString(&strSubSystem, L"USER32");
    if (NT_SUCCESS(Status = NtOpenThreadToken(NtCurrentThread(), TOKEN_QUERY,
            (BOOLEAN)TRUE, &hToken))) {

        /*
         * Perform the check
         */
        Status = NtPrivilegeCheck(hToken, ppSet, &bResult);
        NtPrivilegeObjectAuditAlarm(&strSubSystem, NULL, hToken,
                0, ppSet, bResult);
        NtClose(hToken);
        if (!bResult) {
            SetLastError(ERROR_ACCESS_DENIED);
        }
    }
    CsrRevertToSelf();
    if (!NT_SUCCESS(Status))
        SetLastError(RtlNtStatusToDosError(Status));

    /*
     * Return result of privilege check
     */
    return (BOOL)(bResult && NT_SUCCESS(Status));
}

/***************************************************************************\
* _RegisterServicesProcess
*
* Register the services process.
*
* History:
* 05-05-95 BradG         Created.
\***************************************************************************/

ULONG
SrvRegisterServicesProcess(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    PRIVILEGE_SET psTcb = { 1, PRIVILEGE_SET_ALL_NECESSARY,
        { SE_TCB_PRIVILEGE, 0 }
    };

    BEGIN_LPC_RECV(REGISTERSERVICESPROCESS);

    /*
     * Allow only one services process and then only if it has TCB
     * privilege.
     */
    EnterCrit();
    if ((gdwServicesProcessId != 0) || !IsPrivileged(&psTcb)) {
        SetLastError(ERROR_ACCESS_DENIED);
        a->fSuccess = FALSE;
    } else {
        gdwServicesProcessId = a->dwProcessId;
        a->fSuccess = TRUE;
    }
    LeaveCrit();

    END_LPC_RECV();
}

#if defined(FE_IME)
/***************************************************************************\
* IsImeWindow
*
* return TRUE if it's IME window
*
* History:
* 06-05-96 KazuM         Created.
\***************************************************************************/
BOOL
IsImeWindow(
    HWND hwnd
    )
{
    int num;
    WCHAR ClassName[16];
    DWORD ClassStyle;

    num = GetClassName(hwnd, ClassName, sizeof(ClassName)/sizeof(WCHAR)-1);
    if (num == 0)
        return FALSE;

    ClassName[num] = L'\0';
    if (wcsncmp(ClassName, L"IME", 3) == 0)
        return TRUE;

    ClassStyle = GetClassLong(hwnd, GCL_STYLE);
    if (ClassStyle & CS_IME)
        return TRUE;
    else
        return FALSE;
}
#endif
