/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Shutdown.c

Abstract:

    This module contains the server side implementation for the Win32 remote
    shutdown APIs, that is:

        - BaseInitiateShutdown
        - BaseInitiateShutdownEx
        - BaseAbortShutdown

Author:

    Dave Chalmers (davidc) 29-Apr-1992

Notes:


Revision History:

    19-Oct-1993     Danl
        Removed HackExtraThread which was only here to work around a bug in
        the UserSrv.  The text describing the reason for this workaround is
        as follows:
        HACKHACK - Work around bug in UserSrv that causes ExitWindowsEx to
            fail (error 5) when called by a process which doesn't
            have any threads which have called User APIs.  Remove
            after UserSrv is fixed.  See NTBUG 11601.
            Workaround is to create a thread which will make one
            API call then sleep forever.
    21-May-1999     Dragos C. Sambotin (dragoss)
        Moved Shutdown functionality from winreg to winlogon. Added a new RPC 
        interface for the shutdown func.

--*/


#include "precomp.h"
#pragma hdrstop
#include "shutinit.h"

#define RPC_NO_WINDOWS_H
#include <rpc.h>

// // // // // //

//
// Shutdown Dialog Return Codes:
//

#define SHUTDOWN_SUCCESS        0
#define SHUTDOWN_USER_LOGOFF    1
#define SHUTDOWN_DESKTOP_SWITCH 2
#define SHUTDOWN_CANCELLED      3

//
// System Shutdown globals
//

RTL_CRITICAL_SECTION ShutdownCriticalSection; // Protect global shutdown data

//
// Set when a thread has a shutdown 'in progress'
// (Protected by critical section)
//

BOOL ShutdownInProgress;

//
// Set when a thread wants to interrupt the shutdown
// (Protected by critical section)
//

BOOL AbortShutdown;


//
// Data for shutdown UI - this is protected by the ShutdownInProgress flag.
// i.e. only the current shutdown thread manipulates this data.
//

LARGE_INTEGER ShutdownTime;
DWORD ShutdownDelayInSeconds;
PTCH ShutdownMessage;
DWORD ExitWindowsFlags;
DWORD GinaCode;
PTSTR UserName;
PTSTR UserDomain;
BOOL AllowLogonDuringShutdown = TRUE ;
ActiveDesktops  ShutdownDesktop;
WINDOWPLACEMENT ShutdownWindowPlacement;
BOOL ShutdownGetPlacement = FALSE;
BOOL ShutdownHasBegun = FALSE;
PSID NetworkSid ;

//
// Data captured during initialization
//

PTERMINAL pShutDownTerm;
PUNICODE_STRING ShutdownBuiltinName ;
PUNICODE_STRING ShutdownSystemName ;
HANDLE ShutdownNameWait ;



//
// Private prototypes
//


DWORD
InitializeShutdownData(
    BOOLEAN NoClientName,
    PUNICODE_STRING lpMessage,
    DWORD dwTimeout,
    BOOL bForceAppsClosed,
    BOOL bRebootAfterShutdown
    );

ULONG
BaseInitiateShutdownEx(
    PREGISTRY_SERVER_NAME ServerName,
    PREG_UNICODE_STRING lpMessage OPTIONAL,
    DWORD dwTimeout,
    BOOLEAN bForceAppsClosed,
    BOOLEAN bRebootAfterShutdown,
    DWORD dwReason
    );

VOID
FreeShutdownData(
    VOID
    );

INT_PTR WINAPI
ShutdownApiDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );

BOOL
UpdateTimeToShutdown(
    HWND    hDlg
    );

VOID
CentreWindow(
    HWND    hwnd
    );

DWORD
TestClientPrivilege(
    VOID
    );

DWORD
GetClientId(
    PTSTR *UserName,
    PTSTR *UserDomain
    );

VOID
DeleteClientId(
    PTSTR UserName,
    PTSTR UserDomain
    );

BOOL
InsertClientId(
    HWND hDlg,
    int ControlId,
    PTSTR UserName,
    PTSTR UserDomain
    );


//+---------------------------------------------------------------------------
//
//  Function:   ShutdownGetSystemName
//
//  Synopsis:   Loops getting the locallized name for system
//              (NT_AUTHORITY\SYSTEM) from the LSA as soon as it initializes
//
//  Arguments:  [Ignored] --
//              [Timeout] --
//
//  History:    3-04-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
ShutdownGetSystemName(
    PVOID Ignored,
    BOOLEAN Timeout
    )
{

    NTSTATUS Status ;

    if ( ShutdownSystemName )
    {
        return ;
    }

    while ( TRUE )
    {
        Status = LsaGetUserName( &ShutdownSystemName, &ShutdownBuiltinName );

        if ( NT_SUCCESS( Status ) )
        {
            break;
        }

        Sleep( 15 * 1000 );

    }



    return ;
}


BOOL
InitializeShutdownModule(
    VOID
    )
/*++

Routine Description:

    Does any initializtion required for this module.

Arguments:

    pTerm - Pointer to global data defined in WinMain

Return Value:

    Returns TRUE on success, FALSE on failure.

--*/
{
    NTSTATUS Status;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY ;

    //
    // Initialize global variables
    //

    ShutdownInProgress = FALSE;

    //
    // Initialize critical section to protect globals
    //

    Status = RtlInitializeCriticalSection(&ShutdownCriticalSection);

#if DBG
    if (!NT_SUCCESS(Status)) {
        DbgPrint("InitializeShutdownModule: Failed to initialize critical section\n");
    }
#endif

    Status = RtlAllocateAndInitializeSid( &NtAuthority,
                                          1, SECURITY_NETWORK_RID,
                                          0, 0, 0, 0, 0, 0, 0,
                                          &NetworkSid );

    ShutdownNameWait = SetTimerQueueTimer(
                            NULL,
                            ShutdownGetSystemName,
                            NULL,
                            10 * 1000,
                            INFINITE,
                            FALSE );


    return(NT_SUCCESS(Status));
}


ULONG
StartSystemShutdown(
    IN BOOLEAN NoClientName,
    IN PUNICODE_STRING lpMessage OPTIONAL,
    IN DWORD dwTimeout,
    IN BOOLEAN bForceAppsClosed,
    IN BOOLEAN bRebootAfterShutdown
    )
{
    NTSTATUS Status;
    DWORD Error;

    //
    // Enter the critical section so we can look at our globals
    //

    Status = RtlEnterCriticalSection(&ShutdownCriticalSection);
    if (!NT_SUCCESS(Status)) {
        return(RtlNtStatusToDosError(Status));
    }

    //
    // Set up our global shutdown data.
    // Fail if a shutdown is already in progress
    //

    if ( ShutdownInProgress ||
         ShutdownHasBegun ) {

        Error = ERROR_SHUTDOWN_IN_PROGRESS;

    } else {

        //
        // Set up our globals for the shutdown thread to use.
        //

        Error = InitializeShutdownData(NoClientName,
                                       lpMessage,
                                       dwTimeout,
                                       bForceAppsClosed,
                                       bRebootAfterShutdown
                                      );
        if (Error == ERROR_SUCCESS) {
            ShutdownInProgress = TRUE;
            AbortShutdown = FALSE;
        }
    }

    //
    // Leave the critical section
    //

    Status = RtlLeaveCriticalSection(&ShutdownCriticalSection);
    if (Error == ERROR_SUCCESS) {
        if (!NT_SUCCESS(Status)) {
            Error = RtlNtStatusToDosError(Status);
        }
    } else {
        ASSERT(NT_SUCCESS(Status));
    }



    //
    // Create a thread to handle the shutdown (UI and calling ExitWindows)
    // The thread will handle resetting our shutdown data and globals.
    //

    if (Error == ERROR_SUCCESS) {
        int Result;

        //
        // Have winlogon create us a thread running on the user's desktop.
        //
        // The thread will do a call back to ShutdownThread()
        //

        pShutDownTerm->LogoffFlags = EWX_WINLOGON_API_SHUTDOWN | ExitWindowsFlags;
        Result = InitiateLogoff( pShutDownTerm,
                                 EWX_WINLOGON_API_SHUTDOWN | ExitWindowsFlags );


        if (Result != DLG_SUCCESS ) {
            Error = GetLastError();
            KdPrint(("InitiateSystemShutdown : Failed to create shutdown thread. Error = %d\n", Error));
            FreeShutdownData();
            ShutdownInProgress = FALSE; // Atomic operation
        }
    }

    return(Error);

}

ULONG
BaseInitiateShutdown(
    IN PREGISTRY_SERVER_NAME ServerName,
    IN PREG_UNICODE_STRING lpMessage OPTIONAL,
    IN DWORD dwTimeout,
    IN BOOLEAN bForceAppsClosed,
    IN BOOLEAN bRebootAfterShutdown
    )
/*++

Routine Description:

    Initiates the shutdown of this machine.

Arguments:

    ServerName - Name of machine this server code is running on. (Ignored)

    lpMessage - message to display during shutdown timeout period.

    dwTimeout - number of seconds to delay before shutting down

    bForceAppsClosed - Normally applications may prevent system shutdown.
              - If this true, all applications are terminated unconditionally.

    bRebootAfterShutdown - TRUE if the system should reboot. FALSE if it should
              - be left in a shutdown state.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    //
    // pass through to BaseInitiateShutdownEx, but pass a reason code of 0xFF (unknown)
    //


    return BaseInitiateShutdownEx (ServerName,
                                        lpMessage,
                                        dwTimeout,
                                        bForceAppsClosed,
                                        bRebootAfterShutdown,
                                        REASON_UNKNOWN);
                                         
}

ULONG
BaseInitiateShutdownEx(
    IN PREGISTRY_SERVER_NAME ServerName,
    IN PREG_UNICODE_STRING lpMessage OPTIONAL,
    IN DWORD dwTimeout,
    IN BOOLEAN bForceAppsClosed,
    IN BOOLEAN bRebootAfterShutdown,
    IN DWORD dwReason
    )
/*++

Routine Description:

    Initiates the shutdown of this machine.

Arguments:

    ServerName - Name of machine this server code is running on. (Ignored)

    lpMessage - message to display during shutdown timeout period.

    dwTimeout - number of seconds to delay before shutting down

    bForceAppsClosed - Normally applications may prevent system shutdown.
              - If this true, all applications are terminated unconditionally.

    bRebootAfterShutdown - TRUE if the system should reboot. FALSE if it should
              - be left in a shutdown state.

    dwReason    - Reason for initiating the shutdown.  This reason is logged
                        in the eventlog #6006 event.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    DWORD Error ;
    ULONG rc ;
    HKEY  hKey ;



    Error = TestClientPrivilege();
    if (Error != ERROR_SUCCESS) {
        return(Error);
    }

    //
    // Write the reason code to the registry
    //


    rc = RegCreateKeyExW (HKEY_LOCAL_MACHINE, REGSTR_PATH_RELIABILITY, 0, NULL, REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS, NULL, &hKey, NULL);

    if (rc == ERROR_SUCCESS) {
        RegSetValueExW (hKey, REGSTR_VAL_SHUTDOWNREASON, 0, REG_DWORD, (UCHAR *)(&dwReason), sizeof(DWORD));   
    }

    RegCloseKey (hKey);

    ExitWindowsFlags = 0 ;

    return StartSystemShutdown( FALSE,
                                (PUNICODE_STRING)lpMessage,
                                dwTimeout,
                                bForceAppsClosed,
                                bRebootAfterShutdown );

    UNREFERENCED_PARAMETER(ServerName);    
}

NTSTATUS
LocalInitiateSystemShutdown(
    PUNICODE_STRING Message,
    ULONG Timeout,
    BOOLEAN bForceAppsClosed,
    BOOLEAN bRebootAfterShutdown
    )
{
    LUID PrivilegeRequired;
    PRIVILEGE_SET PrivilegeSet;
    UNICODE_STRING SubSystemName;   // LATER this should be global
    HANDLE Token ;
    NTSTATUS Status ;
    PUNICODE_STRING Name ;
    PUNICODE_STRING Domain ;


    RtlInitUnicodeString(&SubSystemName, L"Win32 SystemShutdown module");

    Status = NtOpenProcessToken( NtCurrentProcess(),
                                 MAXIMUM_ALLOWED,
                                 &Token );

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }

    PrivilegeRequired = RtlConvertLongToLuid( SE_SHUTDOWN_PRIVILEGE );

    PrivilegeSet.PrivilegeCount = 1;
    PrivilegeSet.Control = PRIVILEGE_SET_ALL_NECESSARY;
    PrivilegeSet.Privilege[0].Luid = PrivilegeRequired;
    PrivilegeSet.Privilege[0].Attributes = 0;

    Status = NtPrivilegeObjectAuditAlarm(
                                    &SubSystemName,
                                    NULL,
                                    Token,
                                    0,
                                    &PrivilegeSet,
                                    TRUE );

    NtClose( Token );

    //
    // Get our name.  Note:  we will leak this memory, but
    // we're shutting down, so who cares?
    //

    if ( ShutdownSystemName )
    {
        Name = ShutdownSystemName;
        Domain = ShutdownBuiltinName ;
        Status = STATUS_SUCCESS ;

    }
    else
    {
        Status = LsaGetUserName(
                        &Name, &Domain );

    }


    if ( NT_SUCCESS( Status ) )
    {
        UserName = LocalAlloc( LMEM_FIXED, Name->Length + 2 );

        if ( UserName )
        {
            CopyMemory( UserName, Name->Buffer, Name->Length );
            UserName[ Name->Length / 2 ] = L'\0';
        }

        UserDomain = LocalAlloc( LMEM_FIXED, Domain->Length + 2 );

        if ( UserDomain )
        {
            CopyMemory( UserDomain, Domain->Buffer, Domain->Length );

            UserDomain[ Domain->Length / 2 ]= L'\0';
        }
    }
    else
    {
        UserName = AllocAndDuplicateString( L"" );
        UserDomain = AllocAndDuplicateString( L"" );
    }

    ExitWindowsFlags = EWX_SYSTEM_CALLER ;

    SystemProcessShutdown = TRUE ;

    return StartSystemShutdown(
                         TRUE,
                         Message,
                         Timeout,
                         bForceAppsClosed,
                         bRebootAfterShutdown );



}


DWORD
InitializeShutdownData(
    BOOLEAN NoClientName,
    PUNICODE_STRING lpMessage,
    DWORD dwTimeout,
    BOOL bForceAppsClosed,
    BOOL bRebootAfterShutdown
    )
/*++

Routine Description:

    Stores the passed shutdown parameters in our global data.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    NTSTATUS Status;
    LARGE_INTEGER TimeNow;
    LARGE_INTEGER Delay;
    DWORD Error;

    //
    // Set the shutdown time
    //

    ShutdownDelayInSeconds = dwTimeout;

    Status = NtQuerySystemTime(&TimeNow);
    if (!NT_SUCCESS(Status)) {
        return(RtlNtStatusToDosError(Status));
    }

    Delay = RtlEnlargedUnsignedMultiply(dwTimeout, 10000000);   // Delay in 100ns

    ShutdownTime.QuadPart = TimeNow.QuadPart + Delay.QuadPart;


    //
    // Set the shutdown flags
    //
    // We set the EWX_WINLOGON_OLD_xxx and EWX_xxx both since this message
    // originates from the winlogon process.  When these flags actually bubble
    // back to the active dialog box, winlogon expects the EWX_WINLOGON_OLD_xxx
    // to indicate the 'real' request.
    //

    ExitWindowsFlags |= EWX_LOGOFF | EWX_SHUTDOWN | EWX_WINLOGON_OLD_SHUTDOWN;
    ExitWindowsFlags |= bForceAppsClosed ? EWX_FORCE : 0;
    ExitWindowsFlags |= bRebootAfterShutdown ?
                        (EWX_REBOOT | EWX_WINLOGON_OLD_REBOOT) : 0;

    if (bRebootAfterShutdown)
    {
        GinaCode = WLX_SAS_ACTION_SHUTDOWN_REBOOT;
    }
    else
    {
        GinaCode = WLX_SAS_ACTION_SHUTDOWN;
    }


    //
    // Store the caller's username and domain.
    //

    if ( !NoClientName )
    {
        Error = GetClientId(&UserName, &UserDomain);
        if (Error != ERROR_SUCCESS) {
            return(Error);
        }
    }

    //
    // Set the shutdown message
    //

    if (lpMessage != NULL) {

        //
        // Copy the message into a global buffer
        //

        USHORT Bytes = lpMessage->Length + (USHORT)sizeof(UNICODE_NULL);

        ShutdownMessage = (PTCH)LocalAlloc(LPTR, Bytes);
        if (ShutdownMessage == NULL) {
            DeleteClientId(UserName, UserDomain);
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        RtlMoveMemory(ShutdownMessage, lpMessage->Buffer, lpMessage->Length);
        ShutdownMessage[lpMessage->Length / sizeof(WCHAR)] = 0; // Null terminate

    } else {
        ShutdownMessage = NULL;
    }


    return(ERROR_SUCCESS);
}



VOID
FreeShutdownData(
    VOID
    )
/*++

Routine Description:

    Frees up any memory allocated to store the shutdown data

Return Value:

    None.

--*/

{
    if (ShutdownMessage != NULL) {
        LocalFree(ShutdownMessage);
        ShutdownMessage = NULL;
    }

    DeleteClientId(UserName, UserDomain);
    UserName = NULL;
    UserDomain = NULL;
}



BOOLEAN
ShutdownThread(
    PULONG Flags
    )
/*++

Routine Description:

    Handles the display of a shutdown dialog and coordinating with the
    AbortShutdown API.

Arguments:

    None

Return Value:

    TRUE - system should be shut down
    FALSE - shutdown was aborted

--*/
{
    NTSTATUS Status;
    DWORD Error;
    BOOL DoShutdown = TRUE;
    HDESK hdesk;
    BOOL CloseDesktopHandle;
    DWORD Result;
    BOOL Locked;
    BOOL Success;

    //
    // Quick check so we don't get into thorny race conditions.
    //

    if ( ShutdownDelayInSeconds == 0 )
    {

        FreeShutdownData();

        RtlEnterCriticalSection( &ShutdownCriticalSection );

        ShutdownInProgress = FALSE ;

        RtlLeaveCriticalSection( &ShutdownCriticalSection );


        pShutDownTerm->LastGinaRet = GinaCode;

        ShutdownHasBegun = TRUE;

        if ( pShutDownTerm->UserLoggedOn )
        {
            //
            // If a user is logged on, do the logoff first.  The correct bits are
            // already set in the EWX_WINLOGON_OLD_XXX bits, so the correct behavior
            // will be preserved.  If no one is logged on, we need the EWX_SHUTDOWN flag
            // to go all the way through.
            //

            *Flags &= ~(EWX_SHUTDOWN | EWX_REBOOT | EWX_POWEROFF);
        }

        return( TRUE );

    }


    hdesk = GetActiveDesktop(pShutDownTerm,
                             &CloseDesktopHandle,
                             &Locked);

    while (hdesk != NULL)
    {
        DebugLog((DEB_TRACE, "Starting shutdown dialog on desktop %x\n", hdesk));

        if (Locked)
        {
            UnlockWindowStation(pShutDownTerm->pWinStaWinlogon->hwinsta);
        }

        Success = SetThreadDesktop(hdesk);
        if (!Success)
        {
            DebugLog((DEB_TRACE, "Unable to set desktop, %d\n", GetLastError()));
        }

        if (Locked)
        {
            LockWindowStation(pShutDownTerm->pWinStaWinlogon->hwinsta);
        }

        ShutdownDesktop = pShutDownTerm->pWinStaWinlogon->ActiveDesktop;


        //
        // Push the timeout past the shutdown delay, so that we can
        // catch the messages we want, without stomping on the timeout
        // structures.
        //
        Result = (DWORD)DialogBoxParam( GetModuleHandle(NULL),
                                        MAKEINTRESOURCE( IDD_SYSTEM_SHUTDOWN ),
                                        NULL,
                                        ShutdownApiDlgProc,
                                        (LPARAM) 0 );

        DebugLog((DEB_TRACE, "Shutdown Dialog Returned %d\n", Result ));



        if (CloseDesktopHandle)
        {
            CloseDesktop( hdesk );
        }

        if ((Result == SHUTDOWN_SUCCESS) ||
            (Result == SHUTDOWN_CANCELLED) )
        {
            break;
        }

        //
        // Trickier ones:
        //

        if (Result == SHUTDOWN_USER_LOGOFF)
        {
            if (!AllowLogonDuringShutdown)
            {
                break;
            }

        }

        ShutdownGetPlacement = TRUE;

        hdesk = GetActiveDesktop(pShutDownTerm,
                                 &CloseDesktopHandle,
                                 &Locked);

        DebugLog((DEB_TRACE, "Switching to current desktop and restarting dialog\n"));

    }

    //
    // The shutdown has either completed or been cancelled
    // Reset our globals.
    //
    // Note we need to reset the shutdown-in-progress flag before
    // entering the non-abortable part of shutdown so that anyone
    // trying to abort from here on in will get a failure return code.
    //

    FreeShutdownData();


    Status = RtlEnterCriticalSection(&ShutdownCriticalSection);
    Error = RtlNtStatusToDosError(Status);

    if (Error == ERROR_SUCCESS) {

        //
        // Reset the global shutdown-in-progress flag
        // and check for an abort request.
        //

        if (AbortShutdown) {
            DoShutdown = FALSE;
        }

        ShutdownInProgress = FALSE;

        //
        // Leave the critical section
        //

        Status = RtlLeaveCriticalSection(&ShutdownCriticalSection);
        if (!NT_SUCCESS(Status)) {
            Error = RtlNtStatusToDosError(Status);
        }
    }

    //
    // If DoShutdown, update the last gina ret so that
    // the shutdown code will know what to do:
    //

    if ( DoShutdown )
    {
        pShutDownTerm->LastGinaRet = GinaCode;

        ShutdownHasBegun = TRUE;

        if ( SystemProcessShutdown )
        {
            //
            // This is the panic shutdown when a system process has terminated.
            // Just shut down immediately.
            //

            RevertToSelf();

            Result = EnablePrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE);

            NtShutdownSystem( ShutdownReboot );

            //
            // This shouldn't return.  We should have enabled the privilege (we're a
            // system thread in this case), and this should have rebooted the machine.
            // if it didn't, then we press on to the rest of the shutdown path.
            //

        }
    }



    //
    // Tell the caller if he should shut down.
    //

    if ( DoShutdown )
    {
        if ( pShutDownTerm->UserLoggedOn )
        {
            //
            // If a user is logged on, do the logoff first.  The correct bits are
            // already set in the EWX_WINLOGON_OLD_XXX bits, so the correct behavior
            // will be preserved.  If no one is logged on, we need the EWX_SHUTDOWN flag
            // to go all the way through.
            //

            *Flags &= ~(EWX_SHUTDOWN | EWX_REBOOT | EWX_POWEROFF);
        }
    }

    return (DoShutdown != 0);

}



INT_PTR WINAPI
ShutdownApiDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
/*++

Routine Description:

    Processes messages for the shutdown dialog

    Dialog returns ERROR_SUCCESS if shutdown should proceed,
    ERROR_OPERATION_ABORTED if shutdown should be cancelled.

--*/
{
    HMENU hMenu;

    switch (message) {

    case WM_INITDIALOG:

        //
        // Add the caller's id to the main message text
        //

        InsertClientId(hDlg, IDD_SYSTEM_MESSAGE, UserName, UserDomain);

        //
        // Setup the client's message
        //

        SetDlgItemText(hDlg, IDD_MESSAGE, ShutdownMessage);

        //
        // Remove the close item from the system menu
        //

        hMenu = GetSystemMenu(hDlg, FALSE);
        DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);

        //
        // Position ourselves
        //

        if ( ShutdownGetPlacement )
        {
            SetWindowPlacement( hDlg, &ShutdownWindowPlacement );
        }
        else
        {
            CentreWindow(hDlg);
        }

        SetWindowPos( hDlg, HWND_TOPMOST, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW );


        //
        // Start the timer
        //

        SetTimer(hDlg, 0, 1000, NULL);  // 1 second timer

        //
        // Check if it's over before we've even started
        //

        if (UpdateTimeToShutdown(hDlg)) {

            // It's already time to shutdown
            EndDialog(hDlg, SHUTDOWN_SUCCESS);
        }

        //
        // Let everyone know what state we're in
        //

        pShutDownTerm->ShutdownStarted = TRUE;

        return(TRUE);


    case WM_TIMER:

        //
        // Check for abort flag
        //

        if (AbortShutdown) {
            pShutDownTerm->ShutdownStarted = FALSE;
            EndDialog(hDlg, SHUTDOWN_CANCELLED);
            return(TRUE);
        }

        if ( pShutDownTerm->pWinStaWinlogon->ActiveDesktop != ShutdownDesktop )
        {
            GetWindowPlacement( hDlg, &ShutdownWindowPlacement );
            EndDialog( hDlg, SHUTDOWN_DESKTOP_SWITCH );
            return( TRUE );
        }

        //
        // Update the time delay and check if our time's up
        //

        if (!UpdateTimeToShutdown(hDlg)) {

            //
            // Keep waiting
            //

            return(TRUE);
        }

        //
        // Shutdown time has arrived. Drop through...
        //

    case WLX_WM_SAS:


        DebugLog((DEB_TRACE, "Sas message received?  wParam = %d\n", wParam ));
        if ((wParam == WLX_SAS_TYPE_SCRNSVR_TIMEOUT) &&
            (message == WLX_WM_SAS)) {

           //
           // Don't end the dialog if it's just a screen saver timeout
           //

           return(TRUE);

        } else if ((wParam == WLX_SAS_TYPE_CTRL_ALT_DEL) &&
                   (message == WLX_WM_SAS)) {

           //
           // Also don't end the dialog if it's a Ctrl-Alt-Del
           //

           Sleep (1000);
           return(TRUE);

        } else {

           //
           // If the user logs off, preempt the timeout, restore the state
           //

           EndDialog(hDlg, SHUTDOWN_SUCCESS);

           return(TRUE);

        }

    }

    // We didn't process this message
    return FALSE;

    UNREFERENCED_PARAMETER(lParam);
}




BOOL
UpdateTimeToShutdown(
    HWND    hDlg
    )
/*++

Routine Description:

    Updates the display of the time to system shutdown.

Returns:

    TRUE if shutdown time has arrived, otherwise FALSE

--*/
{
    NTSTATUS Status;
    BOOLEAN Success;
    LARGE_INTEGER TimeNow;
    ULONG ElapsedSecondsNow;
    ULONG ElapsedSecondsAtShutdown;
    ULONG SecondsRemaining;
    ULONG DaysRemaining;
    ULONG HoursRemaining;
    ULONG MinutesRemaining;
    TCHAR Message[40];

    //
    // Set the shutdown time
    //

    Status = NtQuerySystemTime(&TimeNow);
    ASSERT(NT_SUCCESS(Status));

    if (TimeNow.QuadPart >= ShutdownTime.QuadPart)
    {
        return(TRUE);
    }

    Success = RtlTimeToSecondsSince1980(&TimeNow, &ElapsedSecondsNow);
    ASSERT(Success);

    Success = RtlTimeToSecondsSince1980(&ShutdownTime, &ElapsedSecondsAtShutdown);
    ASSERT(Success);

    SecondsRemaining = ElapsedSecondsAtShutdown - ElapsedSecondsNow;

    //
    // Convert the seconds remaining to a string
    //

    MinutesRemaining = SecondsRemaining / 60;
    HoursRemaining = MinutesRemaining / 60;
    DaysRemaining = HoursRemaining / 24;

    SecondsRemaining = SecondsRemaining % 60;
    MinutesRemaining = MinutesRemaining % 60;
    HoursRemaining = HoursRemaining % 24;

    if (DaysRemaining > 0) {
        wsprintf(Message, TEXT("%d days"), DaysRemaining);
    } else {
        wsprintf(Message, TEXT("%02d:%02d:%02d"), HoursRemaining, MinutesRemaining, SecondsRemaining);
    }

    SetDlgItemText(hDlg, IDD_TIMER, Message);

    return(FALSE);
}



ULONG
BaseAbortShutdown(
    IN PREGISTRY_SERVER_NAME ServerName
    )
/*++

Routine Description:

    Aborts a pending shutdown of this machine.

Arguments:

    ServerName - Name of machine this server code is running on. (Ignored)

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    NTSTATUS Status;
    DWORD Error;

    //
    // Check the caller has the appropriate privilege
    //

    Error = TestClientPrivilege();
    if (Error != ERROR_SUCCESS) {
        return(Error);
    }

    //
    // Enter the critical section so we can look at our globals
    //

    Status = RtlEnterCriticalSection(&ShutdownCriticalSection);
    if (!NT_SUCCESS(Status)) {
        return(RtlNtStatusToDosError(Status));
    }


    //
    // If a shutdown is in progress, set the abort flag
    //

    if (ShutdownInProgress) {
        AbortShutdown = TRUE;
        Error = ERROR_SUCCESS;
    } else
    {
        if ( ShutdownHasBegun )
        {
            Error = ERROR_SHUTDOWN_IN_PROGRESS;
        }
        else
        {
            Error = ERROR_NO_SHUTDOWN_IN_PROGRESS;
        }
    }

    //
    // Leave the critical section
    //

    Status = RtlLeaveCriticalSection(&ShutdownCriticalSection);
    if (Error == ERROR_SUCCESS) {
        if (!NT_SUCCESS(Status)) {
            Error = RtlNtStatusToDosError(Status);
        }
    } else {
        ASSERT(NT_SUCCESS(Status));
    }

    return(Error);

    UNREFERENCED_PARAMETER(ServerName);
}



DWORD
TestClientPrivilege(
    VOID
    )
/*++

Routine Description:

    Checks if the client has the privilege to perform the requested shutdown.

Arguments:

    None

Return Value:

    ERROR_SUCCESS if the client has the appropriate privilege.

    ERROR_ACCESS_DENIED - client does not have the required privilege

--*/
{
    NTSTATUS Status, IgnoreStatus;
    BOOL LocalConnection;
    LUID PrivilegeRequired;
    PRIVILEGE_SET PrivilegeSet;
    BOOLEAN Privileged;
    HANDLE Token;
    PTOKEN_GROUPS Groups ;
    ULONG Size ;
    ULONG i ;
    RPC_STATUS RpcStatus ;
    BOOL Network ;

    UNICODE_STRING SubSystemName;   // LATER this should be global
    RtlInitUnicodeString(&SubSystemName, L"Win32 Registry/SystemShutdown module");

    RpcStatus = RpcImpersonateClient( NULL );

    if ( RpcStatus != 0 )
    {
        return RpcStatus ;
    }

    Status = NtOpenThreadToken( NtCurrentThread(),
                                TOKEN_QUERY,
                                TRUE,
                                &Token );

    if ( !NT_SUCCESS( Status ) )
    {
        //
        // Forget it.
        //

        RevertToSelf();

        return RtlNtStatusToDosError( Status );
    }

    //
    // Now, see if this guy has the NETWORK sid in the token:
    //


    PrivilegeRequired = RtlConvertLongToLuid(SE_SHUTDOWN_PRIVILEGE);

    if ( CheckTokenMembership( Token, NetworkSid, &Network ) )
    {
        if ( Network )
        {
            PrivilegeRequired = RtlConvertLongToLuid(SE_REMOTE_SHUTDOWN_PRIVILEGE);
        }
    }
    else
    {
        RevertToSelf();

        return GetLastError();
    }

    //
    // See if the client has the required privilege
    //


    PrivilegeSet.PrivilegeCount = 1;
    PrivilegeSet.Control = PRIVILEGE_SET_ALL_NECESSARY;
    PrivilegeSet.Privilege[0].Luid = PrivilegeRequired;
    PrivilegeSet.Privilege[0].Attributes = 0;

    Status = NtPrivilegeCheck(Token,
                              &PrivilegeSet,
                              &Privileged);

    if (NT_SUCCESS(Status) || (Status == STATUS_PRIVILEGE_NOT_HELD))
    {

        Status = NtPrivilegeObjectAuditAlarm(
                                    &SubSystemName,
                                    NULL,
                                    Token,
                                    0,
                                    &PrivilegeSet,
                                    Privileged);
    }


    NtClose( Token );

    RevertToSelf();


    //
    // Handle unexpected errors
    //

    if (!NT_SUCCESS(Status)) {
        return(RtlNtStatusToDosError(Status));
    }


    //
    // If they failed the privilege check, return an error
    //

    if (!Privileged) {
        return( ERROR_ACCESS_DENIED );
    }

    //
    // They passed muster
    //

    return(ERROR_SUCCESS);
}




DWORD
GetClientId(
    PTSTR *UserName,
    PTSTR *UserDomain
    )
/*++

Routine Description:

    Gets the name and domain of the caller, allocates and returns pointers
    to the information.

    Note we have RPC impersonate the client to discover their ID.

Arguments:

    UserName - a pointer to a NULL terminated string containing the client's
               user name is returned here.

    DomainName - a pointer to a NULL terminated string containing the client's
               domain name is returned here.

    The caller should free UserName and DomainName by calling DeleteClientId

Return Value:

    ERROR_SUCCESS - UserName and UserDomain contain valid pointers

    Other - UserName and UserDomain are invalid

--*/
{
    HANDLE  TokenHandle;
    DWORD   cbNeeded;
    PTOKEN_USER pUserToken;
    BOOL    ReturnValue=FALSE;
    DWORD   cbDomain;
    DWORD   cbName;
    SID_NAME_USE SidNameUse;
    DWORD Error;
    DWORD IgnoreError;

    //
    // Prepare for failure
    //

    *UserName = NULL;
    *UserDomain = NULL;


    Error = RpcImpersonateClient(NULL);
    if (Error != ERROR_SUCCESS) {
        return(Error);
    }

    if (OpenThreadToken(GetCurrentThread(),
                         TOKEN_QUERY,
                         FALSE,
                         &TokenHandle)) {
        //
        // Get the user Sid
        //

        if (!GetTokenInformation(TokenHandle, TokenUser,  (PVOID)NULL, 0, &cbNeeded)) {

            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

                pUserToken = (PTOKEN_USER)LocalAlloc(LPTR, cbNeeded);

                if (pUserToken != NULL) {

                    if (GetTokenInformation(TokenHandle, TokenUser,  pUserToken,
                                            cbNeeded, &cbNeeded)) {

                        //
                        // Convert User Sid to name/domain
                        //

                        cbName = 0;
                        cbDomain = 0;

                        if (!LookupAccountSid(NULL,
                                              pUserToken->User.Sid,
                                              NULL, &cbName,
                                              NULL, &cbDomain,
                                              &SidNameUse)) {

                            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

                                *UserDomain = (PTSTR)LocalAlloc(LPTR, cbDomain*sizeof(TCHAR));
                                *UserName = (PTSTR)LocalAlloc(LPTR, cbName*sizeof(TCHAR));

                                if ((*UserDomain != NULL) && (*UserName != NULL)) {

                                    ReturnValue = LookupAccountSid(
                                                      NULL,
                                                      pUserToken->User.Sid,
                                                      *UserName, &cbName,
                                                      *UserDomain, &cbDomain,
                                                      &SidNameUse);
                                }
                            }

                        }
                    }

                    LocalFree(pUserToken);
                }
            }
        }

        CloseHandle(TokenHandle);
    }


    IgnoreError = RpcRevertToSelf();
    ASSERT(IgnoreError == ERROR_SUCCESS);


    //
    // Clean up on failure
    //

    if (ReturnValue) {
        Error = ERROR_SUCCESS;
    } else {

        Error = GetLastError();

        DeleteClientId(*UserName, *UserDomain);

        *UserName = NULL;
        *UserDomain = NULL;
    }


    return(Error);
}




VOID
DeleteClientId(
    PTSTR UserName,
    PTSTR UserDomain
    )
/*++

Routine Description:

    Frees the client id returned previously by GetClientId

Arguments:

    UserName - a pointer to the username returned by GetClientId.

    DomainName - a pointer to the domain name eturned by GetClientId

Return Value:

    None

--*/
{
    if (UserName != NULL) {
        LocalFree(UserName);
    }

    if (UserDomain != NULL) {
        LocalFree(UserDomain);
    }

}




BOOL
InsertClientId(
    HWND hDlg,
    int ControlId,
    PTSTR UserName,
    PTSTR UserDomain
    )
/*++

Routine Description:

    Takes the text from the specified dialog control, treats it as a printf
    formatting string and inserts the user name and domain as the first 2
    string identifiers (%s)

Arguments:

    UserName - a pointer to the username returned by GetClientId.

    DomainName - a pointer to the domain name eturned by GetClientId

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    DWORD   StringLength;
    DWORD   StringBytes;
    PTSTR   FormatBuffer;
    PTSTR   Buffer;

    //
    // Allocate space for the formatting string out of the control
    //

    StringLength = (DWORD)SendMessage(GetDlgItem(hDlg, ControlId), WM_GETTEXTLENGTH, 0, 0);
    StringBytes = (StringLength + 1) * sizeof(TCHAR); // Allow for terminator

    FormatBuffer = (PTSTR)LocalAlloc(LPTR, StringBytes);
    if (FormatBuffer == NULL) {
        return(FALSE);
    }

    //
    // Read the format string into the buffer
    //

    GetDlgItemText(hDlg, ControlId, FormatBuffer, StringLength);

    //
    // Calculate the maximum size of the string we'll create
    // i.e. Formatting string + username + userdomain
    //

    if ( UserName == NULL )
    {
        UserName = L"" ;
    }
    if ( UserDomain == NULL )
    {
        UserDomain = L"" ;
    }

    StringLength += lstrlen(UserName);
    StringLength += lstrlen(UserDomain);

    //
    // Allocate space for formatted string
    //

    StringBytes = (StringLength + 1) * sizeof(TCHAR); // Allow for terminator

    Buffer = (PTSTR)LocalAlloc(LPTR, StringBytes);
    if (Buffer == NULL) {
        LocalFree(FormatBuffer);
        return(FALSE);
    }

    //
    // Insert the user id into the format string
    //

    wsprintf(Buffer, FormatBuffer, UserDomain, UserName);
    ASSERT((lstrlen(Buffer) * sizeof(TCHAR)) < StringBytes);

    //
    // Replace the control text with our formatted result
    //

    SetDlgItemText(hDlg, ControlId, Buffer);

    //
    // Tidy up
    //

    LocalFree(FormatBuffer);
    LocalFree(Buffer);


    return(TRUE);
}
