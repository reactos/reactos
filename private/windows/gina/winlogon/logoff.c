/****************************** Module Header ******************************\
* Module Name: logoff.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Implements functions to allow a user to logoff the system.
*
* History:
* 12-05-91 Davidc       Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <ras.h>
#include <raserror.h>

//
// Private prototypes
//

HANDLE
ExecLogoffThread(
    PTERMINAL pTerm,
    DWORD     Flags
    );

DWORD
LogoffThreadProc(
    LPVOID Parameter
    );

DWORD
KillComProcesses(
    LPVOID Parameter
    );

INT_PTR WINAPI
ShutdownWaitDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );

BOOL WINAPI
ShutdownDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );

BOOL
DeleteNetworkConnections(
    PTERMINAL pTerm
    );


BOOL
DeleteRasConnections(
    PTERMINAL pTerm
    );

INT_PTR WINAPI
DeleteNetConnectionsDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );

typedef struct _LOGOFF_THREAD_STARTUP {
    PTERMINAL   pTerminal ;
    HANDLE      SyncEvent ;
    DWORD       Flags ;
} LOGOFF_THREAD_STARTUP, * PLOGOFF_THREAD_STARTUP ;


BOOL    ExitWindowsInProgress = FALSE ;
BOOL    SystemProcessShutdown = FALSE ;
HANDLE  LogoffSem ;
HMODULE hRasApi;
RTL_CRITICAL_SECTION LogoffLock ;
ULONG LogoffWaiter ;
BOOL LogoffInProgress ;

#ifndef _WIN64
extern  HWND g_hwndAppDesktopThread;
#endif

BOOL
LogoffLockInit(
    VOID
    )
{
    return NT_SUCCESS( RtlInitializeCriticalSection( &LogoffLock ) );
}

VOID
LogoffLockBegin(
    VOID
    )
{
    RtlEnterCriticalSection( &LogoffLock );

    LogoffInProgress = TRUE ;

    RtlLeaveCriticalSection( &LogoffLock );
    
}

VOID
LogoffLockEnd(
    VOID
    )
{
    BOOL DoRelease = FALSE ;
    ULONG Count ;

    RtlEnterCriticalSection( &LogoffLock );

    LogoffInProgress = FALSE ;

    if ( LogoffWaiter )
    {
        DoRelease = TRUE ;
    }

    RtlLeaveCriticalSection( &LogoffLock );

    if ( DoRelease )
    {
        ReleaseSemaphore( LogoffSem, 1, &Count );
    }
}


VOID
LogoffLockTest(
    VOID
    )
{
    RtlEnterCriticalSection( &LogoffLock );

    if ( LogoffInProgress )
    {
        if ( LogoffSem == NULL )
        {
            LogoffSem = CreateSemaphore( NULL, 1, 64, NULL );

        }

        LogoffWaiter++ ;

        RtlLeaveCriticalSection( &LogoffLock );

        WaitForSingleObject( LogoffSem, 3600 * 1000 );

        RtlEnterCriticalSection( &LogoffLock );

        LogoffWaiter-- ;
    }

    RtlLeaveCriticalSection( &LogoffLock );
}




/***************************************************************************\
* FUNCTION: InitiateLogOff
*
* PURPOSE:  Starts the procedure of logging off the user.
*
* RETURNS:  DLG_SUCCESS - logoff was initiated successfully.
*           DLG_FAILURE - failed to initiate logoff.
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

int
InitiateLogoff(
    PTERMINAL pTerm,
    LONG      Flags
    )
{
    BOOL IgnoreResult;
    HANDLE ThreadHandle;
    HANDLE Handle;
    PUSER_PROCESS_DATA UserProcessData;
    DWORD   Result = 0 ;
    PWINDOWSTATION pWS = pTerm->pWinStaWinlogon;
    LOGOFF_THREAD_STARTUP Startup ;

    //
    // mark the terminal has having done a log off, therefore we can disable auto logon next time in
    //

    pTerm->IgnoreAutoLogon = TRUE;


    //
    // If this is a shutdown operation, call ExitWindowsEx from
    // another thread.
    //

    if (Flags & (EWX_SHUTDOWN | EWX_REBOOT | EWX_POWEROFF)) {

        //
        // Exec a user thread to call ExitWindows
        //

        ThreadHandle = ExecLogoffThread(pTerm, Flags);

        if (ThreadHandle == NULL) {

            DebugLog((DEB_ERROR, "Unable to create logoff thread"));
            return(DLG_FAILURE);

        } else {

            //
            // We don't need the thread handle
            //

            IgnoreResult = CloseHandle(ThreadHandle);
            ASSERT(IgnoreResult);
        }
        Result = DLG_SUCCESS;

    } else {

        //
        // Switch the thread to user context.  We don't want
        // to start another thread to perform logoffs in
        // case the system is out of memory and unable to
        // create any more threads.
        //

        UserProcessData = &pWS->UserProcessData;
        Handle = ImpersonateUser(UserProcessData, GetCurrentThread());

        if (Handle == NULL) {

            DebugLog((DEB_ERROR, "Failed to set user context on thread!"));

            Result = DLG_FAILURE ;

        } else {

            //
            // Let the thread run
            //

            if ((pTerm->UserLoggedOn) &&
                (pTerm->LastGinaRet != WLX_SAS_ACTION_FORCE_LOGOFF) )
            {
                SetActiveDesktop(pTerm, Desktop_Application);
            }

            Startup.Flags = Flags ;
            Startup.pTerminal = pTerm ;
            Startup.SyncEvent = NULL ;
            Result = LogoffThreadProc( &Startup );

        }

        RevertToSelf();

    }

    //
    // ExitWindowsEx will cause one or more desktop switches to occur,
    // so we must invalidate our current desktop.
    //

    if ( (Flags & EWX_WINLOGON_API_SHUTDOWN) == 0 )
    {
        pWS->PreviousDesktop = pWS->ActiveDesktop;
        pWS->ActiveDesktop = -1;
    }

    //
    // The reboot thread is off and running. We're finished.
    //

    return (Result);
}


/***************************************************************************\
* FUNCTION: ExecLogoffThread
*
* PURPOSE:  Creates a user thread that calls ExitWindowsEx with the
*           passed flags.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   05-05-92 Davidc       Created.
*
\***************************************************************************/

HANDLE
ExecLogoffThread(
    PTERMINAL pTerm,
    DWORD Flags
    )
{
    HANDLE ThreadHandle;
    DWORD ThreadId;
    LOGOFF_THREAD_STARTUP Startup ;

    Startup.Flags = Flags ;
    Startup.pTerminal = pTerm ;
    Startup.SyncEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

    if ( Startup.SyncEvent == NULL )
    {
        return NULL ;
    }

    if ( Flags & EWX_SYSTEM_CALLER )
    {
        DebugLog(( DEB_TRACE, "Starting system thread for Logoff, flags = %x\n", Flags ));
        ThreadHandle = CreateThread(
                            NULL,
                            0,
                            LogoffThreadProc,
                            &Startup,
                            0,
                            &ThreadId );
    }
    else 
    {
        DebugLog(( DEB_TRACE, "Starting user thread for Logoff, flags = %x\n", Flags ));
        ThreadHandle = ExecUserThread(
                            pTerm,
                            LogoffThreadProc,
                            &Startup,
                            0,          // Thread creation flags
                            &ThreadId);

    }


    if (ThreadHandle == NULL) {
        DebugLog((DEB_ERROR, "Failed to exec a user logoff thread"));
    }
    else 
    {
        WaitForSingleObjectEx( Startup.SyncEvent, INFINITE, FALSE );
    }

    CloseHandle( Startup.SyncEvent );

    return (ThreadHandle);
}


/***************************************************************************\
* FUNCTION: LogoffThreadProc
*
* PURPOSE:  The logoff thread procedure. Calls ExitWindowsEx with passed flags.
*
* RETURNS:  Thread termination code is result of ExitWindowsEx call.
*
* HISTORY:
*
*   05-05-92 Davidc       Created.
*
\***************************************************************************/

DWORD
LogoffThreadProc(
    LPVOID Parameter
    )
{
    DWORD LogoffFlags ;
    PTERMINAL pTerm ;
    BOOL Result = FALSE;
    PLOGOFF_THREAD_STARTUP Startup ;

    Startup = (PLOGOFF_THREAD_STARTUP) Parameter ;

    LogoffFlags = Startup->Flags ;
    pTerm = Startup->pTerminal ;

    if ( Startup->SyncEvent )
    {
        SetEvent( Startup->SyncEvent );
    }



    //
    // If this logoff is a result of the InitiateSystemShutdown API,
    //  put up a dialog warning the user.
    //

    if ( LogoffFlags & EWX_WINLOGON_API_SHUTDOWN ) {

        Result = ShutdownThread( &LogoffFlags );


    } else {
        if ( !ExitWindowsInProgress )
        {
            Result = TRUE;
        }
        else 
        {
            Result = FALSE;
        }
        if ( pTerm->UserLoggedOn )
        {
            LogoffFlags &= ~(EWX_SHUTDOWN | EWX_REBOOT | EWX_POWEROFF);

        }
    }


    if ( Result ) {

        //
        // Enable shutdown privilege if we need it
        //

        if (LogoffFlags & (EWX_SHUTDOWN | EWX_REBOOT | EWX_POWEROFF))
        {

            if ( LogoffFlags & EWX_WINLOGON_API_SHUTDOWN )
            {
                //
                // Turn off the flags for this call.  They are already in
                // the other bits, so it will come through correctly.
                //


            }
            else
            {

                Result = EnablePrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE);


                if (!Result) {
                    DebugLog((DEB_ERROR, "Logoff thread failed to enable shutdown privilege!\n"));
                }
            }
        }

        //
        // Call ExitWindowsEx with the passed flags
        //

        if (Result) {

            while ( ExitWindowsInProgress )
            {
                //
                // If another thread is doing an ExitWindows, we would corrupt the flags.  This
                // can happen if (a) a remote shutdown is happening during a logoff, (b) a system
                // process has died, (c) someone uses c-a-d and the API at the same time to shutdown.
                //

                DebugLog(( DEB_TRACE, "Spinning while exit windows in progress\n" ));

                Sleep( 1000 );

            }

            //
            // Check to see if the logoff processing is going on in a different thread
            //

            LogoffLockTest();


            DebugLog((DEB_TRACE, "Calling ExitWindowsEx(%#x, 0)\n", LogoffFlags));

            //
            // Set global flag indicating an ExitWindows is in progress.
            //

            ExitWindowsInProgress = TRUE ;

            Result = ExitWindowsEx(LogoffFlags, 0);

            if (!Result) {
                DebugLog((DEB_ERROR, "Logoff thread call to ExitWindowsEx failed, error = %d\n", GetLastError()));
            }
        }
    }

    return(Result ? DLG_SUCCESS : DLG_FAILURE);
}


/***************************************************************************\
* FUNCTION: RebootMachine
*
* PURPOSE:  Calls NtShutdown(Reboot) in current user's context.
*
* RETURNS:  Should never return
*
* HISTORY:
*
*   05-09-92 Davidc       Created.
*
\***************************************************************************/

VOID
RebootMachine(
    PTERMINAL pTerm
    )
{
    NTSTATUS Status;
    BOOL EnableResult, IgnoreResult;
    HANDLE UserHandle;
    PWINDOWSTATION pWS = pTerm->pWinStaWinlogon;

    //
    // Call windows to have it clear all data from video memory
    //

    // GdiEraseMemory();

    DebugLog(( DEB_TRACE, "Rebooting machine\n" ));

    //
    // Impersonate the user for the shutdown call
    //

    UserHandle = ImpersonateUser( &pWS->UserProcessData, NULL );
    ASSERT(UserHandle != NULL);

    //
    // Enable the shutdown privilege
    // This should always succeed - we are either system or a user who
    // successfully passed the privilege check in ExitWindowsEx.
    //

    EnableResult = EnablePrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE);
    ASSERT(EnableResult);


    //
    // Do the final system shutdown pass (reboot)
    //

    if (g_IsTerminalServer) {

        if (!gpfnWinStationShutdownSystem( SERVERNAME_CURRENT, WSD_SHUTDOWN | WSD_REBOOT )) {
            DebugLog((DEB_ERROR, "gpfnWinStationShutdownSystem, ERROR 0x%x", GetLastError()));
        }
        
        if (g_Console) {

            Status = NtShutdownSystem(ShutdownReboot);
        }

    } else {

        Status = NtShutdownSystem(ShutdownReboot);
    }

    DebugLog((DEB_ERROR, "NtShutdownSystem failed, status = 0x%lx", Status));
    ASSERT(NT_SUCCESS(Status)); // Should never get here

    //
    // We may get here if system is screwed up.
    // Try and clean up so they can at least log on again.
    //

    IgnoreResult = StopImpersonating(UserHandle);
    ASSERT(IgnoreResult);
}

/***************************************************************************\
* FUNCTION: PowerdownMachine
*
* PURPOSE:  Calls NtShutdownSystem(ShutdownPowerOff) in current user's context.
*
* RETURNS:  Should never return
*
* HISTORY:
*
*   08-09-93 TakaoK       Created.
*
\***************************************************************************/

VOID
PowerdownMachine(
    PTERMINAL pTerm
    )
{
    NTSTATUS Status;
    BOOL EnableResult, IgnoreResult;
    HANDLE UserHandle;
    PWINDOWSTATION pWS = pTerm->pWinStaWinlogon;

    DebugLog(( DEB_TRACE, "Powering down machine\n" ));
    //
    // Impersonate the user for the shutdown call
    //

    UserHandle = ImpersonateUser( &pWS->UserProcessData, NULL );
    ASSERT(UserHandle != NULL);

    //
    // Enable the shutdown privilege
    // This should always succeed - we are either system or a user who
    // successfully passed the privilege check in ExitWindowsEx.
    //

    EnableResult = EnablePrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE);
    ASSERT(EnableResult);

    //
    // Do the final system shutdown and powerdown pass
    //

    if (g_IsTerminalServer) {

        if (!gpfnWinStationShutdownSystem( SERVERNAME_CURRENT, WSD_SHUTDOWN | WSD_POWEROFF )) {
            DebugLog((DEB_ERROR, "gpfnWinStationShutdownSystem, ERROR 0x%x", GetLastError()));
        }

        if( g_Console ) {

            Status = NtShutdownSystem(ShutdownPowerOff);
        }

    } else {
        Status = NtShutdownSystem(ShutdownPowerOff);
    }


    DebugLog((DEB_ERROR, "NtPowerdownSystem failed, status = 0x%lx", Status));
    ASSERT(NT_SUCCESS(Status)); // Should never get here

    //
    // We may get here if system is screwed up.
    // Try and clean up so they can at least log on again.
    //

    IgnoreResult = StopImpersonating(UserHandle);
    ASSERT(IgnoreResult);
}


/***************************************************************************\
* FUNCTION: ShutdownMachine
*
* PURPOSE:  Shutsdown and optionally reboots or powers off the machine.
*
*           The shutdown is always done in the logged on user's context.
*           If no user is logged on then the shutdown happens in system context.
*
* RETURNS:  FALSE if something went wrong, otherwise it never returns.
*
* HISTORY:
*
*   05-09-92 Davidc       Created.
*   10-04-93 Johannec     Add poweroff option.
*
\***************************************************************************/

extern PTERMINAL pShutDownTerm;

BOOL
ShutdownMachine(
    PTERMINAL pTerm,
    int Flags
    )
{
    int Result;
    HANDLE FoundDialogHandle;
    HANDLE LoadedDialogHandle = NULL;
    BOOL Success ;
    NTSTATUS Status;
    HANDLE UserHandle;

    ASSERT(pTerm == g_pTerminals);

    SetProcessWindowStation(pTerm->pWinStaWinlogon->hwinsta);

    //
    // I don't know what this does, but the power management guys
    // said to call it.
    //

    SetThreadExecutionState( ES_SYSTEM_REQUIRED | ES_CONTINUOUS );

    pShutDownTerm = pTerm;

    StatusMessage (TRUE, 0, IDS_STATUS_STOPPING_WFP);
    SfcTerminateWatcherThread();

    //
    // Preload the shutdown dialog so we don't have to fetch it after
    // the filesystem has been shutdown
    //

    FoundDialogHandle = FindResource(NULL,
                                (LPTSTR) MAKEINTRESOURCE(IDD_SHUTDOWN),
                                (LPTSTR) MAKEINTRESOURCE(RT_DIALOG));
    if (FoundDialogHandle == NULL) {
        DebugLog((DEB_ERROR, "Failed to find shutdown dialog resource\n"));
    } else {
        LoadedDialogHandle = LoadResource(NULL, FoundDialogHandle);
        if (LoadedDialogHandle == NULL) {
            DebugLog((DEB_ERROR, "Failed to load shutdown dialog resource\n"));
        }
    }

    //
    // Send the shutdown notification
    //

    WlWalkNotifyList( pTerm, WL_NOTIFY_SHUTDOWN );

    //
    // Notify the GINA of shutdown here.
    //


#if DBG
    if (TEST_FLAG(GinaBreakFlags, BREAK_SHUTDOWN))
    {
        DebugLog((DEB_TRACE, "About to call WlxShutdown(%#x)\n",
                                    pTerm->Gina.pGinaContext));

        DebugBreak();
    }
#endif

    WlxSetTimeout(pTerm, 120);

    if (g_IsTerminalServer) {

        //
        // Shutdown the WinStations
        //
        gpfnWinStationShutdownSystem( SERVERNAME_CURRENT, WSD_LOGOFF );

    }


    (void) pTerm->Gina.pWlxShutdown(pTerm->Gina.pGinaContext, Flags);

    //
    // If we haven't shut down already (via the Remote shutdown path), then
    // we  start it here, and wait for it to complete.  Otherwise, skip straight
    // down to the cool stuff.
    //
    if (pTerm->WinlogonState != Winsta_Shutdown)
    {
        //
        // Call windows to do the windows part of shutdown
        // We make this a force operation so it is guaranteed to work
        // and can not be interrupted.
        //

        DebugLog(( DEB_TRACE, "Starting shutdown\n" ));

        Result = InitiateLogoff(pTerm, EWX_SHUTDOWN | EWX_FORCE |
                           ((Flags == WLX_SAS_ACTION_SHUTDOWN_REBOOT) ? EWX_REBOOT : 0) |
                           ((Flags == WLX_SAS_ACTION_SHUTDOWN_POWER_OFF) ? EWX_POWEROFF : 0) );

        ASSERT(Result == DLG_SUCCESS);


        //
        // Put up a dialog box to wait for the shutdown notification
        // from windows and make the first NtShutdownSystem call.
        //

        WlxSetTimeout(pTerm, TIMEOUT_NONE);

        Result = WlxDialogBoxParam( pTerm,
                                    g_hInstance,
                                    (LPTSTR)IDD_SHUTDOWN_WAIT,
                                    NULL,
                                    ShutdownWaitDlgProc,
                                    (LPARAM)pTerm);


    }
    else
    {
        //
        // If we're here, it means that we were shut down from the remote path,
        // so user has cleaned up, now we have to call NtShutdown to flush out
        // mm, io, etc.
        //

        DebugLog(( DEB_TRACE, "Shutting down kernel\n" ));

        EnablePrivilege( SE_SHUTDOWN_PRIVILEGE, TRUE );

        if (Flags == WLX_SAS_ACTION_SHUTDOWN_POWER_OFF)
        {
            NtShutdownSystem(ShutdownPowerOff);

        }
        else if (Flags == WLX_SAS_ACTION_SHUTDOWN_REBOOT)
        {
            NtShutdownSystem(ShutdownReboot);

        }
        else
        {
            NtShutdownSystem(ShutdownNoReboot);
        }

        EnablePrivilege( SE_SHUTDOWN_PRIVILEGE, FALSE );
    }


    //
    // It's the notification we were waiting for.
    // Do any final processing required and make the first
    // call to NtShutdownSystem.
    //

    //
    // Impersonate the user for the shutdown call
    //

    UserHandle = ImpersonateUser( &pTerm->pWinStaWinlogon->UserProcessData, NULL );
    ASSERT(UserHandle != NULL);

    //
    // Enable the shutdown privilege
    // This should always succeed - we are either system or a user who
    // successfully passed the privilege check in ExitWindowsEx.
    //

    Success = EnablePrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE);
    ASSERT(Success);

    //
    // Do the first pass at system shutdown (no reboot yet)
    //

    WaitForSystemProcesses();

    //
    // For Hydra, have the Terminal Server do the actual shutdown
    //
    if (g_IsTerminalServer) {

        SHUTDOWN_ACTION Action;
        ULONG ShutdownFlags;

        if (pTerm->LogoffFlags & EWX_POWEROFF) {

            ShutdownFlags = WSD_SHUTDOWN | WSD_POWEROFF;
            Action = ShutdownPowerOff;

        } else if (pTerm->LogoffFlags & EWX_REBOOT) {

            ShutdownFlags = WSD_SHUTDOWN | WSD_REBOOT;
            Action = ShutdownReboot;

        } else {

            ShutdownFlags = WSD_SHUTDOWN;
            Action = ShutdownNoReboot;
        }

        if (!gpfnWinStationShutdownSystem( SERVERNAME_CURRENT, ShutdownFlags )) {
            DebugLog((DEB_ERROR, "gpfnWinStationShutdownSystem, ERROR 0x%x", GetLastError()));
        }

        if( g_Console ) {

            Status = NtShutdownSystem(Action);
        }

    } else {

        //
        // LogoffFlags may not be set correctly for no-user-logged-on case,
        // so check both the LogoffFlags *and* the LastGina status passed in.
        //

        if (pTerm->LogoffFlags & EWX_POWEROFF)
        {
            Status = NtShutdownSystem(ShutdownPowerOff);
        }
        else if (pTerm->LogoffFlags & EWX_REBOOT)
        {
            Status = NtShutdownSystem(ShutdownReboot);
        }
        else if (Flags == WLX_SAS_ACTION_SHUTDOWN_POWER_OFF)
        {
            Status = NtShutdownSystem(ShutdownPowerOff);

        }
        else if (Flags == WLX_SAS_ACTION_SHUTDOWN_REBOOT)
        {
            Status = NtShutdownSystem(ShutdownReboot);

        }
        else
        {
            Status = NtShutdownSystem(ShutdownNoReboot);
        }

    }

    ASSERT(NT_SUCCESS(Status));

    //
    // Revert to ourself
    //

    Success = StopImpersonating(UserHandle);
    ASSERT(Success);

    //
    // We've finished system shutdown, we're done
    //

    //
    // if machine has powerdown capability and user want to turn it off, then
    // we down the system power.
    //
    if ( Flags == WLX_SAS_ACTION_SHUTDOWN_POWER_OFF)
    {
        PowerdownMachine(pTerm);

    }



    //
    // If they got past that dialog it means they want to reboot
    //

    RebootMachine(pTerm);

    ASSERT(!"RebootMachine failed");  // Should never get here

    return(FALSE);
}


/***************************************************************************\
* FUNCTION: ShutdownWaitDlgProc
*
* PURPOSE:  Processes messages while we wait for windows to notify us of
*           a successful shutdown. When notification is received, do any
*           final processing and make the first call to NtShutdownSystem.
*
* RETURNS:
*   DLG_FAILURE     - the dialog could not be displayed
*   DLG_SHUTDOWN()  - the system has been shutdown, reboot wasn't requested
*
* HISTORY:
*
*   10-14-92 Davidc       Created.
*   10-04-93 Johannec     Added Power off option.
*
\***************************************************************************/

INT_PTR WINAPI
ShutdownWaitDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PTERMINAL pTerm = (PTERMINAL)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    BOOL Success;

    switch (message) {

        case WM_INITDIALOG:
            SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);

            MoveWindow (hDlg, -20, -20, 1, 1, TRUE);

            //
            // Send ourselves a message so we can hide without the
            // dialog code trying to force us to be visible
            //

            PostMessage(hDlg, WM_HIDEOURSELVES, 0, 0);
            return(TRUE);


        case WM_HIDEOURSELVES:
            ShowWindow(hDlg, SW_HIDE);

            //
            // Put up the please wait UI
            //

            StatusMessage (FALSE, STATUSMSG_OPTION_SETFOREGROUND, IDS_STATUS_SAVING_DATA);
            return(TRUE);


        case WLX_WM_SAS:
            if (wParam != WLX_SAS_TYPE_USER_LOGOFF)
            {
                return(TRUE);
            }

            UpdateWindow(hDlg);

            //
            // Look at the public shutdown/reboot flags to determine what windows
            // has actually done. We may receive other logoff notifications here
            // but they will be only logoffs - the only place that winlogon actually
            // calls ExitWindowsEx to do a shutdown/reboot is right here. So wait
            // for the real shutdown/reboot notification.
            //

            RemoveStatusMessage( TRUE );

            EndDialog(hDlg, LogoffFlagsToWlxCode(pTerm->LogoffFlags) );

            return TRUE ;
        }


    // We didn't process this message
    return FALSE;
}


/***************************************************************************\
* FUNCTION: ShutdownDlgProc
*
* PURPOSE:  Processes messages for the shutdown dialog - the one that says
*           it's safe to turn off the machine.
*
* RETURNS:  DLG_SUCCESS if the user hits the restart button.
*
* HISTORY:
*
*   03-19-92 Davidc       Created.
*
\***************************************************************************/

BOOL WINAPI
ShutdownDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PTERMINAL pTerm = (PTERMINAL)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (message) {

    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
        SetupSystemMenu(hDlg);
        CentreWindow(hDlg);
        return(TRUE);

    case WM_COMMAND:
        EndDialog(hDlg, DLG_SUCCESS);
        return(TRUE);
    }

    // We didn't process this message
    return FALSE;
}


/***************************************************************************\
* FUNCTION: LogOff
*
* PURPOSE:  Handles the post-user-application part of user logoff. This
*           routine is called after all the user apps have been closed down
*           It saves the user's profile, deletes network connections
*           and reboots/shutsdown the machine if that was requested.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL
Logoff(
    PTERMINAL pTerm,
    int LoggedOnResult
    )
{
    NTSTATUS Status;
    LUID luidNone = { 0, 0 };
    PWINDOWSTATION pWS = pTerm->pWinStaWinlogon;
    HANDLE LogoffThread ;
    DWORD Tid ;

    DebugLog((DEB_TRACE, "In Logoff()\n"));

    LogoffLockBegin();

    SetActiveDesktop(pTerm, Desktop_Application);
    WlWalkNotifyList( pTerm, WL_NOTIFY_LOGOFF );
    RemoveStatusMessage (TRUE);
    SetActiveDesktop(pTerm, Desktop_Winlogon);

    //
    // Terminate the application desktop thread first.
    //
#ifndef _WIN64
    if (!g_IsTerminalServer || g_Console) {
        SendMessage(g_hwndAppDesktopThread, WM_NOTIFY,
                    (WPARAM)g_hwndAppDesktopThread, (LPARAM)g_hwndAppDesktopThread);
    }
#endif

    //
    // We expect to be at the winlogon desktop in all cases
    //

    // ASSERT(OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED) == pTerm->hdeskWinlogon);


    //
    // Delete the user's network connections
    // Make sure we do this before deleting the user's profile
    //

    DeleteNetworkConnections(pTerm);



    //
    // Remove any Messages Aliases added by the user.
    //
    DeleteMsgAliases();


    //
    // Play the user's logoff sound
    //

    StatusMessage (TRUE, 0, IDS_STATUS_PLAY_LOGOFF_SOUND);

    try
    {

        HANDLE uh;
        BOOL fBeep;

        // We AREN'T impersonating the user by default, so we MUST do so
        // otherwise we end up playing the default rather than the user
        // specified sound.

        uh = ImpersonateUser(&pWS->UserProcessData, NULL);

        if (OpenIniFileUserMapping(pTerm))
        {
            //
            // Whenever a user logs out, have WINMM.DLL check if there
            // were any sound events added to the [SOUNDS] section of
            // CONTROL.INI by a non-regstry-aware app.  If there are,
            // migrate those schemes to their new home.  If there aren't,
            // this is very quick.
            //

            MigrateSoundEvents();

            if (!SystemParametersInfo(SPI_GETBEEP, 0, &fBeep, FALSE)) {
                // Failed to get hold of beep setting.  Should we be
                // noisy or quiet?  We have to choose one value...
                fBeep = TRUE;
            }

            if (fBeep) {

                //
                // Play synchronous
                //
                PlaySound( (LPCTSTR) SND_ALIAS_SYSTEMEXIT,
                           NULL,
                           SND_ALIAS_ID | SND_SYNC | SND_NODEFAULT );
            }

            CloseIniFileUserMapping(pTerm);
        }

        __try { WinmmLogoff(); }
        __except (EXCEPTION_EXECUTE_HANDLER) { NOTHING; }

        StopImpersonating(uh);

    }
    except ( EXCEPTION_EXECUTE_HANDLER )
    {
        //
        // these are delay loaded.  They might blow.  If so, press on anyway.
        //

        NOTHING ;
    }

    //
    // Call user to close the registry key for the NLS cache.
    //

    SetWindowStationUser(pTerm->pWinStaWinlogon->hwinsta, &luidNone, NULL, 0);

    //
    // Close the IniFileMapping that happened at logon time (LogonAttempt()).
    //

    CloseIniFileUserMapping(pTerm);


    //
    // Save the user profile, this unloads the user's key in the registry
    //

    StatusMessage (FALSE, 0, IDS_STATUS_SAVE_PROFILE);
    SaveUserProfile(pTerm->pWinStaWinlogon);

    //
    // Delete any remaining RAS connections.  Make sure to do this after
    // the user profile gets copied up to the
    //

    DeleteRasConnections(pTerm);

    //
    // Don't do repaints at this point
    //
    pTerm->MuGlobals.fLogoffInProgress = TRUE;

    //
    // If the user logged off themselves (rather than a system logoff)
    // and wanted to reboot then do it now.
    //

    LogoffLockEnd();

    //
    // Create a thread to do another log off in case any sens or scripts invoked
    // a process through COM.
    //
    LogoffThread = ExecUserThread(
                        pTerm,
                        KillComProcesses,
                        pTerm,
                        0,
                        &Tid );

    if ( LogoffThread )
    {
        WaitForSingleObject( LogoffThread, 15 * 60 * 1000 );

        CloseHandle( LogoffThread );
    }

    //
    // Set up security info for new user (system) - this clears out
    // the stuff for the old user.
    //

    SecurityChangeUser(pTerm, NULL, NULL, g_WinlogonSid, FALSE);

    if (IsShutdown(LoggedOnResult) && (!(pTerm->LogoffFlags & EWX_WINLOGON_OLD_SYSTEM)))
    {
        ShutdownMachine(pTerm, LoggedOnResult);

        ASSERT(!"ShutdownMachine failed"); // Should never return
    }



    return(TRUE);
}

DWORD
KillComProcesses(
    PVOID Parameter
    )
{
    PTERMINAL pTerm = (PTERMINAL) Parameter ;

    DebugLog(( DEB_TRACE, "ExitWindowsEx called to shut down COM processes\n" ));

    ExitWindowsEx(
            EWX_FORCE |
            EWX_LOGOFF |
            EWX_NONOTIFY |
            EWX_WINLOGON_CALLER,
            0 );

    return 0 ;
            
}


/***************************************************************************\
* FUNCTION: DeleteNetworkConnections
*
* PURPOSE:  Calls WNetNukeConnections in the client context to delete
*           any connections they may have had.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   04-15-92 Davidc       Created.
*
\***************************************************************************/

BOOL
DeleteNetworkConnections(
    PTERMINAL    pTerm
    )
{
    HANDLE ImpersonationHandle;
    DWORD WNetResult;
    BOOL Result = FALSE; // Default is failure
    TCHAR szMprDll[] = TEXT("mpr.dll");
    CHAR szWNetNukeConn[]     = "WNetClearConnections";
    CHAR szWNetOpenEnum[]     = "WNetOpenEnumW";
    CHAR szWNetEnumResource[] = "WNetEnumResourceW";
    CHAR szWNetCloseEnum[]    = "WNetCloseEnum";
    PWNETNUKECONN  lpfnWNetNukeConn        = NULL;
    PWNETOPENENUM  lpfnWNetOpenEnum        = NULL;
    PWNETENUMRESOURCE lpfnWNetEnumResource = NULL;
    PWNETCLOSEENUM lpfnWNetCloseEnum       = NULL;
    HANDLE hEnum;
    BOOL bConnectionsExist = TRUE;
    NETRESOURCE NetRes;
    DWORD dwNumEntries = 1;
    DWORD dwEntrySize = sizeof (NETRESOURCE);
    HINSTANCE hMPR = NULL;

    //
    // Impersonate the user
    //

    ImpersonationHandle = ImpersonateUser(&pTerm->pWinStaWinlogon->UserProcessData, NULL);

    if (ImpersonationHandle == NULL) {
        DebugLog((DEB_ERROR, "DeleteNetworkConnections : Failed to impersonate user\n"));
        return(FALSE);
    }


    //
    // Load mpr if it wasn't already loaded.
    //

    if (!(hMPR = LoadLibrary(szMprDll))) {
       DebugLog((DEB_ERROR, "DeleteNetworkConnections : Failed to load mpr.dll\n"));
       goto DNCExit;
    }

    //
    // Get the function pointers
    //

    lpfnWNetOpenEnum = (PWNETOPENENUM) GetProcAddress(hMPR,
                                                      (LPSTR)szWNetOpenEnum);
    lpfnWNetEnumResource = (PWNETENUMRESOURCE) GetProcAddress(hMPR,
                                                      (LPSTR)szWNetEnumResource);
    lpfnWNetCloseEnum = (PWNETCLOSEENUM) GetProcAddress(hMPR,
                                                      (LPSTR)szWNetCloseEnum);
    lpfnWNetNukeConn = (PWNETNUKECONN) GetProcAddress(hMPR,
                                                      (LPSTR)szWNetNukeConn);

    //
    // Check for NULL return values
    //

    if ( !lpfnWNetOpenEnum || !lpfnWNetEnumResource ||
         !lpfnWNetCloseEnum || !lpfnWNetNukeConn ) {
        DebugLog((DEB_ERROR, "DeleteNetworkConnections : Received a NULL pointer from GetProcAddress\n"));
        goto DNCExit;
    }

    //
    // Check for at least one network connection
    //

    if ( (*lpfnWNetOpenEnum)(RESOURCE_CONNECTED, RESOURCETYPE_ANY,
                             0, NULL, &hEnum) == NO_ERROR) {

        if ((*lpfnWNetEnumResource)(hEnum, &dwNumEntries, &NetRes,
                                    &dwEntrySize) == ERROR_NO_MORE_ITEMS) {
            bConnectionsExist = FALSE;
        }

        (*lpfnWNetCloseEnum)(hEnum);
    }

    //
    // If we don't have any connections, then we can exit.
    //

    if (!bConnectionsExist) {
        goto DNCExit;
    }

    StatusMessage (FALSE, 0, IDS_STATUS_CLOSE_NET);

    //
    // Delete the network connections.
    //

    WNetResult = 0;

    WNetResult = (*lpfnWNetNukeConn)(NULL);

    if (WNetResult != 0 && WNetResult != ERROR_CAN_NOT_COMPLETE) {
        DebugLog((DEB_ERROR, "DeleteNetworkConnections : WNetNukeConnections failed, error = %d\n", WNetResult));
    }

    Result = (WNetResult == ERROR_SUCCESS);

DNCExit:

    //
    // Unload mpr.dll
    //

    if ( hMPR ) {
        FreeLibrary(hMPR);
    }

    //
    // Revert to being 'ourself'
    //

    if (!StopImpersonating(ImpersonationHandle)) {
        DebugLog((DEB_ERROR, "DeleteNetworkConnections : Failed to revert to self\n"));
    }

    return(Result);
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteRasConnections
//
//  Synopsis:   Delete RAS connections during logoff.
//
//  Arguments:  (none)
//
//  History:    5-10-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
DeleteRasConnections(
    PTERMINAL    pTerm
    )
{
    HANDLE  ImpersonationHandle;
    SC_HANDLE hServiceMgr, hService;
    SERVICE_STATUS status;
    RASCONN rasconn;
    LPRASCONN lprasconn = &rasconn;
    DWORD i, dwErr, dwcb, dwc;
    BOOL bRet;
    PRASENUMCONNECTIONSW pRasEnumConnectionsW;
    PRASHANGUPW pRasHangUpW;
    BOOL FreeThatRasConn = FALSE;
    PWINDOWSTATION pWS = pTerm->pWinStaWinlogon;


    if ( GetProfileInt( WINLOGON, KEEP_RAS_AFTER_LOGOFF, 0 ) )
    {
        return( TRUE );
    }

    //
    // Determine whether the rasman service is running.
    //

    hServiceMgr = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

    if ( hServiceMgr == NULL )
    {
        DebugLog((DEB_ERROR, "Unable to object service controller, %d\n", GetLastError()));
        return( FALSE );
    }

    hService = OpenService(
                     hServiceMgr,
                     RASMAN_SERVICE_NAME,
                     SERVICE_QUERY_STATUS);

    if (hService == NULL)
    {
        CloseServiceHandle(hServiceMgr);
        DebugLog((DEB_TRACE, "rasman not started, nothing to tear down\n"));
        return( TRUE );
    }

    bRet = QueryServiceStatus( hService, &status );

    CloseServiceHandle(hService);

    CloseServiceHandle(hServiceMgr);

    if (! bRet )
    {
        //
        // Service in bad state, get out of here...
        //

        return( TRUE );
    }

    if (status.dwCurrentState != SERVICE_RUNNING)
    {
        //
        // Service is not running
        //

        return( TRUE );

    }

    //
    // Load the RASAPI DLL so we can make it do stuff
    //

    if ( !hRasApi )
    {
        hRasApi = LoadLibrary( RASAPI32 );
        if ( !hRasApi )
        {
            return( FALSE );
        }
    }

    pRasEnumConnectionsW = (PRASENUMCONNECTIONSW) GetProcAddress(
                                hRasApi,
                                "RasEnumConnectionsW" );

    pRasHangUpW = (PRASHANGUPW) GetProcAddress(
                                hRasApi,
                                "RasHangUpW" );

    if ( (!pRasEnumConnectionsW) ||
         (!pRasHangUpW) )
    {
        return( FALSE );
    }



    //
    // Impersonate the user
    //

    ImpersonationHandle = ImpersonateUser(&pWS->UserProcessData, NULL);

    if (ImpersonationHandle == NULL) {
        DebugLog((DEB_ERROR, "DeleteNetworkConnections : Failed to impersonate user\n"));
        return(FALSE);
    }

    //
    // Enumerate the current RAS connections.
    //


    lprasconn->dwSize = sizeof (RASCONN);

    dwcb = sizeof (RASCONN);

    dwc = 1;

    dwErr = pRasEnumConnectionsW(lprasconn, &dwcb, &dwc);

    if (dwErr == ERROR_BUFFER_TOO_SMALL)
    {
        lprasconn = LocalAlloc(LPTR, dwcb);

        FreeThatRasConn = TRUE;

        if ( !lprasconn )
        {
            return( FALSE );
        }

        lprasconn->dwSize = sizeof (RASCONN);

        dwErr = pRasEnumConnectionsW(lprasconn, &dwcb, &dwc);
        if (dwErr)
        {
            if ( FreeThatRasConn )
            {
                LocalFree( lprasconn );
            }

            return( FALSE );
        }
    }
    else if (dwErr)
    {
        return( FALSE );
    }

    //
    // cycle through the connections, and kill them
    //


    for (i = 0; i < dwc; i++)
    {
        DebugLog((DEB_TRACE, "Hanging up connection to %ws\n",
                lprasconn[i].szEntryName));

        (VOID) pRasHangUpW( lprasconn[i].hrasconn );
    }

    if ( FreeThatRasConn )
    {
        LocalFree( lprasconn );
    }

    //
    // Revert to being 'ourself'
    //

    if (!StopImpersonating(ImpersonationHandle)) {
        DebugLog((DEB_ERROR, "DeleteNetworkConnections : Failed to revert to self\n"));
    }

    return( TRUE );
}


