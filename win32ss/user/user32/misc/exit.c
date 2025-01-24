/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            win32ss/user/user32/misc/exit.c
 * PURPOSE:         Shutdown related functions
 * PROGRAMMER:      Eric Kohl
 */

#include <user32.h>

/*
 * Sequence of events:
 *
 * - App (usually explorer) calls ExitWindowsEx()
 * - ExitWindowsEx() sends a message to CSRSS
 * - CSRSS impersonates the caller and sends a message to a hidden Winlogon window
 * - Winlogon checks if the caller has the required privileges
 * - Winlogon enters pending log-out state
 * - Winlogon impersonates the interactive user and calls ExitWindowsEx() again,
 *   passing some special internal flags
 * - CSRSS loops over all processes of the interactive user (sorted by their
 *   SetProcessShutdownParameters() level), sending WM_QUERYENDSESSION and
 *   WM_ENDSESSION messages to its top-level windows. If the messages aren't
 *   processed within the timeout period (registry key HKCU\Control Panel\Desktop\HungAppTimeout)
 *   CSRSS will put up a dialog box asking if the process should be terminated.
 *   Using the registry key HKCU\Control Panel\Desktop\AutoEndTask you can
 *   specify that the dialog box shouldn't be shown and CSRSS should just
 *   terminate the thread. If the the WM_ENDSESSION message is processed
 *   but the thread doesn't terminate within the timeout specified by
 *   HKCU\Control Panel\Desktop\WaitToKillAppTimeout CSRSS will terminate
 *   the thread. When all the top-level windows have been destroyed CSRSS
 *   will terminate the process.
 *   If the process is a console process, CSRSS will send a CTRL_LOGOFF_EVENT
 *   to the console control handler on logoff. No event is sent on shutdown.
 *   If the handler doesn't respond in time the same activities as for GUI
 *   apps (i.e. display dialog box etc) take place. This also happens if
 *   the handler returns TRUE.
 * - This ends the processing for the first ExitWindowsEx() call from Winlogon.
 *   Execution continues in Winlogon, which calls ExitWindowsEx() again to
 *   terminate COM processes in the interactive user's session.
 * - Winlogon stops impersonating the interactive user (whose processes are
 *   all dead by now). and enters log-out state
 * - If the ExitWindowsEx() request was for a logoff, Winlogon sends a SAS
 *   event (to display the "press ctrl+alt+del") to the GINA. Winlogon then
 *   waits for the GINA to send a SAS event to login.
 * - If the ExitWindowsEx() request was for shutdown/restart, Winlogon calls
 *   ExitWindowsEx() again in the system process context.
 * - CSRSS goes through the motions of sending WM_QUERYENDSESSION/WM_ENDSESSION
 *   to GUI processes running in the system process context but won't display
 *   dialog boxes or kill threads/processes. Same for console processes,
 *   using the CTRL_SHUTDOWN_EVENT. The Service Control Manager is one of
 *   these console processes and has a special timeout value WaitToKillServiceTimeout.
 * - After CSRSS has finished its pass notifying processes that system is shutting down,
 *   Winlogon finishes the shutdown process by calling the executive subsystem
 *   function NtShutdownSystem.
 */

typedef struct
{
    UINT  uFlags;
    DWORD dwReserved;
} EXIT_REACTOS_DATA, *PEXIT_REACTOS_DATA;

static BOOL
ExitWindowsWorker(UINT uFlags,
                  DWORD dwReserved,
                  BOOL bCalledFromThread);

static DWORD
WINAPI
ExitWindowsThread(LPVOID Param)
{
    DWORD dwExitCode;
    PEXIT_REACTOS_DATA ExitData = (PEXIT_REACTOS_DATA)Param;

    /* Do the exit asynchronously */
    if (ExitWindowsWorker(ExitData->uFlags, ExitData->dwReserved, TRUE))
        dwExitCode = ERROR_SUCCESS;
    else
        dwExitCode = GetLastError();

    ExitThread(dwExitCode);
    return ERROR_SUCCESS;
}

static BOOL
ExitWindowsWorker(UINT uFlags,
                  DWORD dwReserved,
                  BOOL bCalledFromThread)
{
    EXIT_REACTOS_DATA ExitData;
    HANDLE hExitThread;
    DWORD ExitCode;
    MSG msg;

    USER_API_MESSAGE ApiMessage;
    PUSER_EXIT_REACTOS ExitReactOSRequest = &ApiMessage.Data.ExitReactOSRequest;

    /*
     * 1- FIXME: Call NtUserCallOneParam(uFlags, ONEPARAM_ROUTINE_PREPAREFORLOGOFF);
     *    If success we can continue, otherwise we must fail.
     */

    /*
     * 2- Send the Exit request to CSRSS (and to Win32k indirectly).
     *    We can shutdown synchronously or asynchronously.
     */

    // ExitReactOSRequest->LastError = ERROR_SUCCESS;
    ExitReactOSRequest->Flags = uFlags;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(USERSRV_SERVERDLL_INDEX, UserpExitWindowsEx),
                        sizeof(*ExitReactOSRequest));

    /* Set the last error accordingly */
    if (NT_SUCCESS(ApiMessage.Status) || ApiMessage.Status == STATUS_CANT_WAIT)
    {
        if (ExitReactOSRequest->LastError != ERROR_SUCCESS)
            UserSetLastError(ExitReactOSRequest->LastError);
    }
    else
    {
        UserSetLastNTError(ApiMessage.Status);
        ExitReactOSRequest->Success = FALSE;
    }

    /*
     * In case CSR call succeeded and we did a synchronous exit
     * (STATUS_CANT_WAIT is considered as a non-success status),
     * return the real state of the operation now.
     */
    if (NT_SUCCESS(ApiMessage.Status))
        return ExitReactOSRequest->Success;

    /*
     * In case something failed: we have a non-success status and:
     * - either we were doing a synchronous exit (Status != STATUS_CANT_WAIT), or
     * - we were doing an asynchronous exit because we were called recursively via
     *   another thread but we failed to exit,
     * then bail out immediately, otherwise we would enter an infinite loop of exit requests.
     *
     * On the contrary if we need to do an asynchronous exit (Status == STATUS_CANT_WAIT
     * and not called recursively via another thread), then continue and do the exit.
     */
    if (ApiMessage.Status != STATUS_CANT_WAIT || bCalledFromThread)
    {
        UserSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    /*
     * 3- Win32k wants us to perform an asynchronous exit. Run the request in a thread.
     * (ApiMessage.Status == STATUS_CANT_WAIT and not already called from a thread)
     */
    ExitData.uFlags     = uFlags;
    ExitData.dwReserved = dwReserved;
    hExitThread = CreateThread(NULL, 0, ExitWindowsThread, &ExitData, 0, NULL);
    if (hExitThread == NULL)
        return FALSE;

    /* Pump and discard any input events sent to the app(s) */
    while (MsgWaitForMultipleObjectsEx(1, &hExitThread, INFINITE, QS_ALLINPUT, 0))
    {
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
            DispatchMessageW(&msg);
    }

    /* Finally, return to caller */
    if (!GetExitCodeThread(hExitThread, &ExitCode))
        ExitCode = GetLastError();

    CloseHandle(hExitThread);

    if (ExitCode != ERROR_SUCCESS)
        UserSetLastError(ExitCode);

    return (ExitCode == ERROR_SUCCESS);
}

/*
 * @implemented
 */
BOOL WINAPI
ExitWindowsEx(UINT uFlags,
              DWORD dwReserved)
{
    /*
     * FIXME:
     * 1- Calling the Exit worker must be done under certain conditions.
     *    We may also need to warn the user if there are other people logged
     *    on this computer (see https://pve.proxmox.com/wiki/Windows_2003_guest_best_practices )
     * 2- Call SrvRecordShutdownReason.
     */

    return ExitWindowsWorker(uFlags, dwReserved, FALSE);

    /* FIXME: Call SrvRecordShutdownReason if we failed */
}

/*
 * @implemented
 */
BOOL
WINAPI
EndTask(HWND hWnd,
        BOOL fShutDown,
        BOOL fForce)
{
    USER_API_MESSAGE ApiMessage;
    PUSER_END_TASK EndTaskRequest = &ApiMessage.Data.EndTaskRequest;

    UNREFERENCED_PARAMETER(fShutDown);

    // EndTaskRequest->LastError = ERROR_SUCCESS;
    EndTaskRequest->WndHandle = hWnd;
    EndTaskRequest->Force = fForce;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(USERSRV_SERVERDLL_INDEX, UserpEndTask),
                        sizeof(*EndTaskRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        UserSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    if (EndTaskRequest->LastError != ERROR_SUCCESS)
        UserSetLastError(EndTaskRequest->LastError);

    return EndTaskRequest->Success;
}

/* EOF */
