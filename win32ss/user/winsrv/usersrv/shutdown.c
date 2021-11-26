/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User API Server DLL
 * FILE:            win32ss/user/winsrv/usersrv/shutdown.c
 * PURPOSE:         Logout/shutdown
 * PROGRAMMERS:
 */

/* INCLUDES *******************************************************************/

#include "usersrv.h"

#include <commctrl.h>
#include <psapi.h>

#include "resource.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

// Those flags (that are used for CsrProcess->ShutdownFlags) are named
// in accordance to the only public one: SHUTDOWN_NORETRY used for the
// SetProcessShutdownParameters API.
#if !defined(SHUTDOWN_SYSTEMCONTEXT) && !defined(SHUTDOWN_OTHERCONTEXT)
#define SHUTDOWN_SYSTEMCONTEXT  CsrShutdownSystem
#define SHUTDOWN_OTHERCONTEXT   CsrShutdownOther
#endif

// The DPRINTs that need to really be removed as soon as everything works.
#define MY_DPRINT  DPRINT1
#define MY_DPRINT2 DPRINT

typedef struct tagNOTIFY_CONTEXT
{
    UINT Msg;
    WPARAM wParam;
    LPARAM lParam;
    // HDESK Desktop;
    // HDESK OldDesktop;
    DWORD StartTime;
    DWORD QueryResult;
    HWND Dlg;
    DWORD EndNowResult;
    BOOL ShowUI;
    HANDLE UIThread;
    HWND WndClient;
    PSHUTDOWN_SETTINGS ShutdownSettings;
} NOTIFY_CONTEXT, *PNOTIFY_CONTEXT;

#define QUERY_RESULT_ABORT    0
#define QUERY_RESULT_CONTINUE 1
#define QUERY_RESULT_TIMEOUT  2
#define QUERY_RESULT_ERROR    3
#define QUERY_RESULT_FORCE    4

typedef void (WINAPI *INITCOMMONCONTROLS_PROC)(void);

typedef struct tagMESSAGE_CONTEXT
{
    HWND Wnd;
    UINT Msg;
    WPARAM wParam;
    LPARAM lParam;
    DWORD Timeout;
} MESSAGE_CONTEXT, *PMESSAGE_CONTEXT;


/* FUNCTIONS ******************************************************************/

static HMODULE hComCtl32Lib = NULL;

static VOID
CallInitCommonControls(VOID)
{
    static BOOL Initialized = FALSE;
    INITCOMMONCONTROLS_PROC InitProc;

    if (Initialized) return;

    hComCtl32Lib = LoadLibraryW(L"COMCTL32.DLL");
    if (hComCtl32Lib == NULL) return;

    InitProc = (INITCOMMONCONTROLS_PROC)GetProcAddress(hComCtl32Lib, "InitCommonControls");
    if (InitProc == NULL) return;

    (*InitProc)();

    Initialized = TRUE;
}

static VOID FASTCALL
UpdateProgressBar(HWND ProgressBar, PNOTIFY_CONTEXT NotifyContext)
{
    DWORD Passed;

    Passed = GetTickCount() - NotifyContext->StartTime;
    Passed -= NotifyContext->ShutdownSettings->HungAppTimeout;
    if (NotifyContext->ShutdownSettings->WaitToKillAppTimeout < Passed)
    {
        Passed = NotifyContext->ShutdownSettings->WaitToKillAppTimeout;
    }
    SendMessageW(ProgressBar, PBM_SETPOS, Passed / 2, 0);
}

static INT_PTR CALLBACK
EndNowDlgProc(HWND Dlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR Result;
    PNOTIFY_CONTEXT NotifyContext;
    HWND ProgressBar;
    DWORD TitleLength;
    int Len;
    LPWSTR Title;

    switch(Msg)
    {
    case WM_INITDIALOG:
        NotifyContext = (PNOTIFY_CONTEXT)lParam;
        NotifyContext->EndNowResult = QUERY_RESULT_ABORT;
        SetWindowLongPtrW(Dlg, DWLP_USER, (LONG_PTR)lParam);
        TitleLength = SendMessageW(NotifyContext->WndClient, WM_GETTEXTLENGTH,
                                   0, 0) +
                      GetWindowTextLengthW(Dlg);
        Title = HeapAlloc(UserServerHeap, 0, (TitleLength + 1) * sizeof(WCHAR));
        if (Title)
        {
            Len = GetWindowTextW(Dlg, Title, TitleLength + 1);
            SendMessageW(NotifyContext->WndClient, WM_GETTEXT,
                         TitleLength + 1 - Len, (LPARAM)(Title + Len));
            SetWindowTextW(Dlg, Title);
            HeapFree(UserServerHeap, 0, Title);
        }
        ProgressBar = GetDlgItem(Dlg, IDC_PROGRESS);
        SendMessageW(ProgressBar, PBM_SETRANGE32, 0,
                     NotifyContext->ShutdownSettings->WaitToKillAppTimeout / 2);
        UpdateProgressBar(ProgressBar, NotifyContext);
        SetTimer(Dlg, 0, 200, NULL);
        Result = FALSE;
        break;

    case WM_TIMER:
        NotifyContext = (PNOTIFY_CONTEXT)GetWindowLongPtrW(Dlg, DWLP_USER);
        ProgressBar = GetDlgItem(Dlg, IDC_PROGRESS);
        UpdateProgressBar(ProgressBar, NotifyContext);
        Result = TRUE;
        break;

    case WM_COMMAND:
        if (BN_CLICKED == HIWORD(wParam) && IDC_END_NOW == LOWORD(wParam))
        {
            NotifyContext = (PNOTIFY_CONTEXT)GetWindowLongPtrW(Dlg, DWLP_USER);
            NotifyContext->EndNowResult = QUERY_RESULT_FORCE;
            MY_DPRINT("Closing progress dlg by hand\n");
            SendMessageW(Dlg, WM_CLOSE, 0, 0);
            Result = TRUE;
        }
        else
        {
            Result = FALSE;
        }
        break;

    case WM_CLOSE:
        MY_DPRINT("WM_CLOSE\n");
        DestroyWindow(Dlg);
        Result = TRUE;
        break;

    case WM_DESTROY:
        MY_DPRINT("WM_DESTROY\n");
        NotifyContext = (PNOTIFY_CONTEXT)GetWindowLongPtrW(Dlg, DWLP_USER);
        NotifyContext->Dlg = NULL;
        KillTimer(Dlg, 0);
        PostQuitMessage(NotifyContext->EndNowResult);
        Result = TRUE;
        break;

    default:
        Result = FALSE;
        break;
    }

    return Result;
}

static DWORD WINAPI
EndNowThreadProc(LPVOID Parameter)
{
    PNOTIFY_CONTEXT NotifyContext = (PNOTIFY_CONTEXT)Parameter;
    MSG Msg;

#if 0
    SetThreadDesktop(NotifyContext->Desktop);
    SwitchDesktop(NotifyContext->Desktop);
#else
    /* For now show the end task dialog in the active desktop */
    NtUserSetInformationThread(NtCurrentThread(),
                               UserThreadUseActiveDesktop,
                               NULL,
                               0);
#endif

    CallInitCommonControls();
    NotifyContext->Dlg = CreateDialogParam(UserServerDllInstance,
                                           MAKEINTRESOURCE(IDD_END_NOW), NULL,
                                           EndNowDlgProc, (LPARAM)NotifyContext);
    if (NotifyContext->Dlg == NULL)
        return 0;

    ShowWindow(NotifyContext->Dlg, SW_SHOWNORMAL);

    while (GetMessageW(&Msg, NULL, 0, 0))
    {
        if (!IsDialogMessage(NotifyContext->Dlg, &Msg))
        {
            TranslateMessage(&Msg);
            DispatchMessageW(&Msg);
        }
    }

    return Msg.wParam;
}

static DWORD WINAPI
SendClientShutdown(LPVOID Parameter)
{
    PMESSAGE_CONTEXT Context = (PMESSAGE_CONTEXT)Parameter;
    DWORD_PTR Result;

    /* If the shutdown is aborted, just notify the process, there is no need to wait */
    if ((Context->wParam & (MCS_QUERYENDSESSION | MCS_ENDSESSION)) == 0)
    {
        DPRINT("Called WM_CLIENTSHUTDOWN with wParam == 0 ...\n");
        SendNotifyMessageW(Context->Wnd, WM_CLIENTSHUTDOWN,
                           Context->wParam, Context->lParam);
        return QUERY_RESULT_CONTINUE;
    }

    if (SendMessageTimeoutW(Context->Wnd, WM_CLIENTSHUTDOWN,
                            Context->wParam, Context->lParam,
                            SMTO_NORMAL, Context->Timeout, &Result))
    {
        DWORD Ret;

        if (Context->wParam & MCS_QUERYENDSESSION)
        {
            /* WM_QUERYENDSESSION case */
            switch (Result)
            {
                case MCSR_DONOTSHUTDOWN:
                    Ret = QUERY_RESULT_ABORT;
                    break;

                case MCSR_GOODFORSHUTDOWN:
                case MCSR_SHUTDOWNFINISHED:
                default:
                    Ret = QUERY_RESULT_CONTINUE;
            }
        }
        else
        {
            /* WM_ENDSESSION case */
            Ret = QUERY_RESULT_CONTINUE;
        }

        DPRINT("SendClientShutdown -- Return == %s\n",
                  Ret == QUERY_RESULT_CONTINUE ? "Continue" : "Abort");
        return Ret;
    }

    DPRINT1("SendClientShutdown -- Error == %s\n",
              GetLastError() == 0 ? "Timeout" : "error");

    return (GetLastError() == 0 ? QUERY_RESULT_TIMEOUT : QUERY_RESULT_ERROR);
}

static BOOL
NotifyTopLevelWindow(HWND Wnd, PNOTIFY_CONTEXT NotifyContext)
{
    MESSAGE_CONTEXT MessageContext;
    DWORD Now, Passed;
    DWORD Timeout, WaitStatus;
    HANDLE MessageThread;
    HANDLE Threads[2];

    SetForegroundWindow(Wnd);

    Now = GetTickCount();
    if (NotifyContext->StartTime == 0)
        NotifyContext->StartTime = Now;

    /*
     * Note: Passed is computed correctly even when GetTickCount()
     * wraps due to unsigned arithmetic.
     */
    Passed = Now - NotifyContext->StartTime;
    MessageContext.Wnd = Wnd;
    MessageContext.Msg = NotifyContext->Msg;
    MessageContext.wParam = NotifyContext->wParam;
    MessageContext.lParam = NotifyContext->lParam;
    MessageContext.Timeout = NotifyContext->ShutdownSettings->HungAppTimeout;
    if (!NotifyContext->ShutdownSettings->AutoEndTasks)
    {
        MessageContext.Timeout += NotifyContext->ShutdownSettings->WaitToKillAppTimeout;
    }
    if (Passed < MessageContext.Timeout)
    {
        MessageContext.Timeout -= Passed;
        MessageThread = CreateThread(NULL, 0, SendClientShutdown,
                                     (LPVOID)&MessageContext, 0, NULL);
        if (MessageThread == NULL)
        {
            NotifyContext->QueryResult = QUERY_RESULT_ERROR;
            return FALSE;
        }
        Timeout = NotifyContext->ShutdownSettings->HungAppTimeout;
        if (Passed < Timeout)
        {
            Timeout -= Passed;
            WaitStatus = WaitForSingleObjectEx(MessageThread, Timeout, FALSE);
        }
        else
        {
            WaitStatus = WAIT_TIMEOUT;
        }
        if (WAIT_TIMEOUT == WaitStatus)
        {
            NotifyContext->WndClient = Wnd;
            if (NotifyContext->UIThread == NULL && NotifyContext->ShowUI)
            {
                NotifyContext->UIThread = CreateThread(NULL, 0,
                                                       EndNowThreadProc,
                                                       (LPVOID)NotifyContext,
                                                       0, NULL);
            }
            Threads[0] = MessageThread;
            Threads[1] = NotifyContext->UIThread;
            WaitStatus = WaitForMultipleObjectsEx(NotifyContext->UIThread == NULL ?
                                                  1 : 2,
                                                  Threads, FALSE, INFINITE,
                                                  FALSE);
            if (WaitStatus == WAIT_OBJECT_0)
            {
                if (!GetExitCodeThread(MessageThread, &NotifyContext->QueryResult))
                {
                    NotifyContext->QueryResult = QUERY_RESULT_ERROR;
                }
            }
            else if (WaitStatus == WAIT_OBJECT_0 + 1)
            {
                if (!GetExitCodeThread(NotifyContext->UIThread,
                                       &NotifyContext->QueryResult))
                {
                    NotifyContext->QueryResult = QUERY_RESULT_ERROR;
                }
            }
            else
            {
                NotifyContext->QueryResult = QUERY_RESULT_ERROR;
            }
            if (WaitStatus != WAIT_OBJECT_0)
            {
                TerminateThread(MessageThread, QUERY_RESULT_TIMEOUT);
            }
        }
        else if (WaitStatus == WAIT_OBJECT_0)
        {
            if (!GetExitCodeThread(MessageThread,
                                   &NotifyContext->QueryResult))
            {
                NotifyContext->QueryResult = QUERY_RESULT_ERROR;
            }
        }
        else
        {
            NotifyContext->QueryResult = QUERY_RESULT_ERROR;
        }
        CloseHandle(MessageThread);
    }
    else
    {
        NotifyContext->QueryResult = QUERY_RESULT_TIMEOUT;
    }

    DPRINT("NotifyContext->QueryResult == %d\n", NotifyContext->QueryResult);
    return (NotifyContext->QueryResult == QUERY_RESULT_CONTINUE);
}

static BOOLEAN
IsConsoleMode(VOID)
{
    return (BOOLEAN)NtUserCallNoParam(NOPARAM_ROUTINE_ISCONSOLEMODE);
}

/************************************************/


static BOOL
ThreadShutdownNotify(IN PCSR_THREAD CsrThread,
                     IN ULONG Flags,
                     IN ULONG Flags2,
                     IN PNOTIFY_CONTEXT Context)
{
    HWND TopWnd = NULL;

    EnumThreadWindows(HandleToUlong(CsrThread->ClientId.UniqueThread),
                      FindTopLevelWnd, (LPARAM)&TopWnd);
    if (TopWnd)
    {
        HWND hWndOwner;

        /*** FOR TESTING PURPOSES ONLY!! ***/
        HWND tmpWnd;
        tmpWnd = TopWnd;
        /***********************************/

        while ((hWndOwner = GetWindow(TopWnd, GW_OWNER)) != NULL)
        {
            MY_DPRINT("GetWindow(TopWnd, GW_OWNER) not returned NULL...\n");
            TopWnd = hWndOwner;
        }
        if (TopWnd != tmpWnd) MY_DPRINT("(TopWnd = %x) != (tmpWnd = %x)\n", TopWnd, tmpWnd);
    }
    else
    {
        return FALSE;
    }

    Context->wParam = Flags2;
    Context->lParam = (0 != (Flags & EWX_CALLER_WINLOGON_LOGOFF) ?
                       ENDSESSION_LOGOFF : 0);

    Context->StartTime = 0;
    Context->UIThread = NULL;
    Context->ShowUI = !IsConsoleMode() && (Flags2 & (MCS_QUERYENDSESSION | MCS_ENDSESSION));
    Context->Dlg = NULL;

#if 0 // Obviously, switching desktops like that from within WINSRV doesn't work...
    {
    BOOL Success;
    Context->OldDesktop = GetThreadDesktop(GetCurrentThreadId());
    // Context->Desktop    = GetThreadDesktop(HandleToUlong(CsrThread->ClientId.UniqueThread));
    Context->Desktop    = GetThreadDesktop(GetWindowThreadProcessId(TopWnd, NULL));
    MY_DPRINT("Last error = %d\n", GetLastError());
    MY_DPRINT("Before switching to desktop 0x%x\n", Context->Desktop);
    Success = SwitchDesktop(Context->Desktop);
    MY_DPRINT("After switching to desktop (Success = %s ; last error = %d); going to notify top-level...\n",
            Success ? "TRUE" : "FALSE", GetLastError());
    }
#endif

    NotifyTopLevelWindow(TopWnd, Context);

/******************************************************************************/
#if 1
    if (Context->UIThread)
    {
        MY_DPRINT("Context->UIThread != NULL\n");
        if (Context->Dlg)
        {
            MY_DPRINT("Sending WM_CLOSE because Dlg is != NULL\n");
            SendMessageW(Context->Dlg, WM_CLOSE, 0, 0);
        }
        else
        {
            MY_DPRINT("Terminating UIThread thread with QUERY_RESULT_ERROR\n");
            TerminateThread(Context->UIThread, QUERY_RESULT_ERROR);
        }
        CloseHandle(Context->UIThread);
        /**/Context->UIThread = NULL;/**/
        /**/Context->Dlg = NULL;/**/
    }
#endif
/******************************************************************************/

#if 0
    MY_DPRINT("Switch back to old desktop 0x%x\n", Context->OldDesktop);
    SwitchDesktop(Context->OldDesktop);
    MY_DPRINT("Switched back ok\n");
#endif

    return TRUE;
}

static ULONG
NotifyUserProcessForShutdown(PCSR_PROCESS CsrProcess,
                             PSHUTDOWN_SETTINGS ShutdownSettings,
                             UINT Flags)
{
    DWORD QueryResult = QUERY_RESULT_CONTINUE;
    PCSR_PROCESS Process;
    PCSR_THREAD Thread;
    PLIST_ENTRY NextEntry;
    NOTIFY_CONTEXT Context;
    BOOL FoundWindows = FALSE;

    /* In case we make a forced shutdown, just kill the process */
    if (Flags & EWX_FORCE)
        return CsrShutdownCsrProcess;

    Context.ShutdownSettings = ShutdownSettings;
    Context.QueryResult = QUERY_RESULT_CONTINUE; // We continue shutdown by default.

    /* Lock the process */
    CsrLockProcessByClientId(CsrProcess->ClientId.UniqueProcess, &Process);

    /* Send first the QUERYENDSESSION messages to all the threads of the process */
    MY_DPRINT2("Sending the QUERYENDSESSION messages...\n");

    NextEntry = CsrProcess->ThreadList.Flink;
    while (NextEntry != &CsrProcess->ThreadList)
    {
        /* Get the current thread entry */
        Thread = CONTAINING_RECORD(NextEntry, CSR_THREAD, Link);

        /* Move to the next entry */
        NextEntry = NextEntry->Flink;

        /* If the thread is being terminated, just skip it */
        if (Thread->Flags & CsrThreadTerminated) continue;

        /* Reference the thread and temporarily unlock the process */
        CsrReferenceThread(Thread);
        CsrUnlockProcess(Process);

        Context.QueryResult = QUERY_RESULT_CONTINUE;
        if (ThreadShutdownNotify(Thread, Flags, MCS_QUERYENDSESSION, &Context))
        {
            FoundWindows = TRUE;
        }

        /* Lock the process again and dereference the thread */
        CsrLockProcessByClientId(CsrProcess->ClientId.UniqueProcess, &Process);
        CsrDereferenceThread(Thread);

        // FIXME: Analyze Context.QueryResult !!
        /**/if (Context.QueryResult == QUERY_RESULT_ABORT) goto Quit;/**/
    }

    if (!FoundWindows)
    {
        /* We looped all threads but no top level window was found so we didn't send any message */
        /* Let the console server run the generic process shutdown handler */
        CsrUnlockProcess(Process);
        return CsrShutdownNonCsrProcess;
    }

    QueryResult = Context.QueryResult;
    MY_DPRINT2("QueryResult = %s\n",
               QueryResult == QUERY_RESULT_ABORT ? "Abort" : "Continue");

    /* Now send the ENDSESSION messages to the threads */
    MY_DPRINT2("Now sending the ENDSESSION messages...\n");

    NextEntry = CsrProcess->ThreadList.Flink;
    while (NextEntry != &CsrProcess->ThreadList)
    {
        /* Get the current thread entry */
        Thread = CONTAINING_RECORD(NextEntry, CSR_THREAD, Link);

        /* Move to the next entry */
        NextEntry = NextEntry->Flink;

        /* If the thread is being terminated, just skip it */
        if (Thread->Flags & CsrThreadTerminated) continue;

        /* Reference the thread and temporarily unlock the process */
        CsrReferenceThread(Thread);
        CsrUnlockProcess(Process);

        Context.QueryResult = QUERY_RESULT_CONTINUE;
        ThreadShutdownNotify(Thread, Flags,
                             (QUERY_RESULT_ABORT != QueryResult) ? MCS_ENDSESSION : 0,
                             &Context);

        /* Lock the process again and dereference the thread */
        CsrLockProcessByClientId(CsrProcess->ClientId.UniqueProcess, &Process);
        CsrDereferenceThread(Thread);
    }

Quit:
    /* Unlock the process */
    CsrUnlockProcess(Process);

#if 0
    if (Context.UIThread)
    {
        if (Context.Dlg)
        {
            SendMessageW(Context.Dlg, WM_CLOSE, 0, 0);
        }
        else
        {
            TerminateThread(Context.UIThread, QUERY_RESULT_ERROR);
        }
        CloseHandle(Context.UIThread);
    }
#endif

    /* Kill the process unless we abort shutdown */
    if (QueryResult == QUERY_RESULT_ABORT)
        return CsrShutdownCancelled;

    return CsrShutdownCsrProcess;
}

static NTSTATUS FASTCALL
UserExitReactOS(PCSR_THREAD CsrThread, UINT Flags)
{
    NTSTATUS Status;
    LUID CallerLuid;

    DWORD ProcessId = HandleToUlong(CsrThread->ClientId.UniqueProcess);
    DWORD ThreadId  = HandleToUlong(CsrThread->ClientId.UniqueThread);

    DPRINT1("SrvExitWindowsEx(ClientId: %lx.%lx, Flags: 0x%x)\n",
            ProcessId, ThreadId, Flags);

    /*
     * Check for flags validity
     */

    if (Flags & EWX_CALLER_WINLOGON)
    {
        /* Only Winlogon can call this */
        if (ProcessId != LogonProcessId)
        {
            DPRINT1("SrvExitWindowsEx call not from Winlogon\n");
            return STATUS_ACCESS_DENIED;
        }
    }

    /* Implicitely add the shutdown flag when we poweroff or reboot */
    if (Flags & (EWX_POWEROFF | EWX_REBOOT))
        Flags |= EWX_SHUTDOWN;

    /*
     * Impersonate and retrieve the caller's LUID so that
     * we can only shutdown processes in its context.
     */
    if (!CsrImpersonateClient(NULL))
        return STATUS_BAD_IMPERSONATION_LEVEL;

    Status = CsrGetProcessLuid(NULL, &CallerLuid);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Unable to get caller LUID, Status = 0x%08x\n", Status);
        goto Quit;
    }

    DPRINT("Caller LUID is: %lx.%lx\n", CallerLuid.HighPart, CallerLuid.LowPart);

    /* Shutdown loop */
    while (TRUE)
    {
        /* Notify Win32k and potentially Winlogon of the shutdown */
        Status = NtUserSetInformationThread(CsrThread->ThreadHandle,
                                            UserThreadInitiateShutdown,
                                            &Flags, sizeof(Flags));
        DPRINT("Win32k says: %lx\n", Status);
        switch (Status)
        {
            /* We cannot wait here, the caller should start a new thread */
            case STATUS_CANT_WAIT:
                DPRINT1("NtUserSetInformationThread returned STATUS_CANT_WAIT\n");
                goto Quit;

            /* Shutdown is in progress */
            case STATUS_PENDING:
                DPRINT1("NtUserSetInformationThread returned STATUS_PENDING\n");
                goto Quit;

            /* Abort */
            case STATUS_RETRY:
            {
                DPRINT1("NtUserSetInformationThread returned STATUS_RETRY\n");
                UNIMPLEMENTED;
                continue;
            }

            default:
            {
                if (!NT_SUCCESS(Status))
                {
                    // FIXME: Use some UserSetLastNTError or SetLastNtError
                    // that we have defined for user32 or win32k usage only...
                    SetLastError(RtlNtStatusToDosError(Status));
                    goto Quit;
                }
            }
        }

        /* All good */
        break;
    }

    /*
     * OK we can continue. Now magic happens:
     *
     * Terminate all Win32 processes, stop if we find one kicking
     * and screaming it doesn't want to die.
     *
     * This function calls the ShutdownProcessCallback callback of
     * each CSR server for each Win32 process.
     */
    Status = CsrShutdownProcesses(&CallerLuid, Flags);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to shutdown processes, Status = 0x%08x\n", Status);
    }

    // FIXME: If Status == STATUS_CANCELLED, call RecordShutdownReason

    /* Tell Win32k and potentially Winlogon that we're done */
    NtUserSetInformationThread(CsrThread->ThreadHandle,
                               UserThreadEndShutdown,
                               &Status, sizeof(Status));

    DPRINT("SrvExitWindowsEx returned 0x%08x\n", Status);

Quit:
    /* We are done */
    CsrRevertToSelf();
    return Status;
}


ULONG
NTAPI
UserClientShutdown(IN PCSR_PROCESS CsrProcess,
                   IN ULONG Flags,
                   IN BOOLEAN FirstPhase)
{
    ULONG result;

    DPRINT("UserClientShutdown(0x%p, 0x%x, %s) - [0x%x, 0x%x], ShutdownFlags: %lu\n",
            CsrProcess, Flags, FirstPhase ? "FirstPhase" : "LastPhase",
            CsrProcess->ClientId.UniqueProcess, CsrProcess->ClientId.UniqueThread,
            CsrProcess->ShutdownFlags);

    /*
     * Check for process validity
     */

    /* Do not kill system processes when a user is logging off */
    if ((Flags & EWX_SHUTDOWN) == EWX_LOGOFF &&
        (CsrProcess->ShutdownFlags & (SHUTDOWN_OTHERCONTEXT | SHUTDOWN_SYSTEMCONTEXT)))
    {
        DPRINT("Do not kill a system process in a logoff request!\n");
        return CsrShutdownNonCsrProcess;
    }

    /* Do not kill Winlogon */
    if (CsrProcess->ClientId.UniqueProcess == UlongToHandle(LogonProcessId))
    {
        DPRINT("Not killing Winlogon; CsrProcess->ShutdownFlags = %lu\n",
               CsrProcess->ShutdownFlags);

        /* Returning CsrShutdownCsrProcess means that we handled this process by doing nothing */
        /* This will mark winlogon as processed so consrv won't be notified again for it */
        CsrDereferenceProcess(CsrProcess);
        return CsrShutdownCsrProcess;
    }

    /* Notify the process for shutdown if needed */
    result = NotifyUserProcessForShutdown(CsrProcess, &ShutdownSettings, Flags);
    if (result == CsrShutdownCancelled || result == CsrShutdownNonCsrProcess)
    {
        if (result == CsrShutdownCancelled)
            DPRINT1("Process 0x%x aborted shutdown\n", CsrProcess->ClientId.UniqueProcess);
        return result;
    }

    /* Terminate this process */
#if DBG
    {
        WCHAR buffer[MAX_PATH];
        if (!GetProcessImageFileNameW(CsrProcess->ProcessHandle, buffer, MAX_PATH))
        {
            DPRINT1("Terminating process %x\n", CsrProcess->ClientId.UniqueProcess);
        }
        else
        {
            DPRINT1("Terminating process %x (%S)\n", CsrProcess->ClientId.UniqueProcess, buffer);
        }
    }
#endif
    NtTerminateProcess(CsrProcess->ProcessHandle, 0);

    WaitForSingleObject(CsrProcess->ProcessHandle, ShutdownSettings.ProcessTerminateTimeout);

    /* We are done */
    CsrDereferenceProcess(CsrProcess);
    return CsrShutdownCsrProcess;
}


/* PUBLIC SERVER APIS *********************************************************/

/* API_NUMBER: UserpExitWindowsEx */
CSR_API(SrvExitWindowsEx)
{
    NTSTATUS Status;
    PUSER_EXIT_REACTOS ExitReactOSRequest = &((PUSER_API_MESSAGE)ApiMessage)->Data.ExitReactOSRequest;

    Status = NtUserSetInformationThread(NtCurrentThread(),
                                        UserThreadUseActiveDesktop,
                                        NULL,
                                        0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to set thread desktop!\n");
        return Status;
    }

    Status = UserExitReactOS(CsrGetClientThread(), ExitReactOSRequest->Flags);
    ExitReactOSRequest->Success   = NT_SUCCESS(Status);
    ExitReactOSRequest->LastError = GetLastError();

    NtUserSetInformationThread(NtCurrentThread(), UserThreadRestoreDesktop, NULL, 0);

    return Status;
}

/* API_NUMBER: UserpEndTask */
CSR_API(SrvEndTask)
{
    PUSER_END_TASK EndTaskRequest = &((PUSER_API_MESSAGE)ApiMessage)->Data.EndTaskRequest;
    NTSTATUS Status;

    // FIXME: This is HACK-plemented!!
    DPRINT1("SrvEndTask is HACKPLEMENTED!!\n");

    Status = NtUserSetInformationThread(NtCurrentThread(),
                                        UserThreadUseActiveDesktop,
                                        NULL,
                                        0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to set thread desktop!\n");
        return Status;
    }

    SendMessageW(EndTaskRequest->WndHandle, WM_CLOSE, 0, 0);
    // PostMessageW(EndTaskRequest->WndHandle, WM_CLOSE, 0, 0);

    if (IsWindow(EndTaskRequest->WndHandle))
    {
        if (EndTaskRequest->Force)
        {
            EndTaskRequest->Success   = DestroyWindow(EndTaskRequest->WndHandle);
            EndTaskRequest->LastError = GetLastError();
        }
        else
        {
            EndTaskRequest->Success = FALSE;
        }
    }
    else
    {
        EndTaskRequest->Success   = TRUE;
        EndTaskRequest->LastError = ERROR_SUCCESS;
    }

    NtUserSetInformationThread(NtCurrentThread(), UserThreadRestoreDesktop, NULL, 0);

    return STATUS_SUCCESS;
}

/* API_NUMBER: UserpRecordShutdownReason */
CSR_API(SrvRecordShutdownReason)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
