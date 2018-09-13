/****************************** Module Header ******************************\
* Module Name: provider.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Implements functions that support multiple network providers.
* Currently this involves notifying credential managers of logon and
* password change operations.
*
* History:
* 01-10-93 Davidc       Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

//
// Define this to enable verbose output for this module
//

// #define DEBUG_PROVIDER

#ifdef DEBUG_PROVIDER
#define VerbosePrint(s) WLPrint(s)
#else
#define VerbosePrint(s)
#endif


//
// Define the key in the winlogon section of win.ini that
// defines the the multiple provider notify app name.
//

#define NOTIFY_KEY_NAME             TEXT("mpnotify")

//
// Define the default multiple provider notify app name.
//

#define DEFAULT_NOTIFY_APP_NAME     TEXT("mpnotify.exe")


//
// Define environment variables used to pass information to multiple
// provider notify process
//

#define MPR_STATION_NAME_VARIABLE       TEXT("WlMprNotifyStationName")
#define MPR_STATION_HANDLE_VARIABLE     TEXT("WlMprNotifyStationHandle")
#define MPR_WINLOGON_WINDOW_VARIABLE    TEXT("WlMprNotifyWinlogonWindow")

#define MPR_LOGON_FLAG_VARIABLE         TEXT("WlMprNotifyLogonFlag")
#define MPR_USERNAME_VARIABLE           TEXT("WlMprNotifyUserName")
#define MPR_DOMAIN_VARIABLE             TEXT("WlMprNotifyDomain")
#define MPR_PASSWORD_VARIABLE           TEXT("WlMprNotifyPassword")
#define MPR_OLD_PASSWORD_VARIABLE       TEXT("WlMprNotifyOldPassword")
#define MPR_OLD_PASSWORD_VALID_VARIABLE TEXT("WlMprNotifyOldPasswordValid")
#define MPR_LOGONID_VARIABLE            TEXT("WlMprNotifyLogonId")
#define MPR_CHANGE_INFO_VARIABLE        TEXT("WlMprNotifyChangeInfo")
#define MPR_PASSTHROUGH_VARIABLE        TEXT("WlMprNotifyPassThrough")
#define MPR_PROVIDER_VARIABLE           TEXT("WlMprNotifyProvider")
#define MPR_DESKTOP_VARIABLE            TEXT("WlMprNotifyDesktop")


#define WM_MONITOR_DIED     (WM_USER + 1)
#define DLG_SAS_RAISED      (DLG_FAILURE + 1)


//
// Define the structure used to pass data into the notify control dialog
//

typedef struct {
    PTERMINAL   pTerm;
    LPWSTR      ReturnBuffer; // Returned from dialog
    BOOL        ProcessRunning;
    PWINLOGON_JOB Job ;
    HWND        hDlg ;
    PVOID       Env ;
    WPARAM      SAS ;
} NOTIFY_DATA;

typedef NOTIFY_DATA *PNOTIFY_DATA;




//
// Private prototypes
//

BOOL
MprNotifyDlgInit(
    HWND      hDlg,
    PTERMINAL pTerm
    );

BOOL
StartNotifyProcessMonitor(
    HWND hDlg
    );

VOID
DeleteNotifyProcessMonitor(
    HWND hDlg
    );

BOOL
KillNotifyProcess(
    PNOTIFY_DATA pNotifyData
    );


/***************************************************************************\
* FUNCTION: DeleteNotifyVariables
*
* PURPOSE:  Deletes all the notify data environment variables from the
*           current process's environment.
*
* RETURNS:  Nothing
*
* HISTORY:
*
*   01-12-93 Davidc       Created.
*
\***************************************************************************/

VOID
DeleteNotifyVariables(
    VOID 
    )
{
    SetEnvironmentVariable(MPR_STATION_NAME_VARIABLE, NULL);
    SetEnvironmentVariable(MPR_STATION_HANDLE_VARIABLE, NULL);
    SetEnvironmentVariable(MPR_WINLOGON_WINDOW_VARIABLE, NULL);

    SetEnvironmentVariable(MPR_LOGON_FLAG_VARIABLE, NULL);
    SetEnvironmentVariable(MPR_USERNAME_VARIABLE, NULL);
    SetEnvironmentVariable(MPR_DOMAIN_VARIABLE, NULL);
    SetEnvironmentVariable(MPR_PASSWORD_VARIABLE, NULL);
    SetEnvironmentVariable(MPR_OLD_PASSWORD_VALID_VARIABLE, NULL);
    SetEnvironmentVariable(MPR_OLD_PASSWORD_VARIABLE, NULL);
    SetEnvironmentVariable(MPR_LOGONID_VARIABLE, NULL);
    SetEnvironmentVariable(MPR_CHANGE_INFO_VARIABLE, NULL);
    SetEnvironmentVariable(MPR_PASSTHROUGH_VARIABLE, NULL);
    SetEnvironmentVariable(MPR_PROVIDER_VARIABLE, NULL);
    SetEnvironmentVariable(MPR_DESKTOP_VARIABLE, NULL );
}


/***************************************************************************\
* FUNCTION: SetWinlogonWindowVariable
*
* PURPOSE:  Sets winlogon window environment variable in current process's
*           environment - this is inherited by notify process.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   01-12-93 Davidc       Created.
*
\***************************************************************************/

BOOL
SetWinlogonWindowVariable(
    PVOID * Env,
    HWND hwnd
    )
{
    BOOL Result;

    Result = SetEnvironmentULong(
                Env, 
                MPR_WINLOGON_WINDOW_VARIABLE, 
                (ULONG_PTR)hwnd);

    if (!Result) {
        DebugLog((DEB_ERROR, "SetWinlogonWindowVariable: Failed to set variable, error = %d\n", GetLastError()));
    }

    return(Result);
}

BOOL
SetNotifyVariable(
    PVOID * Env,
    LPWSTR  Variable,
    LPWSTR  Value
    )
{
    NTSTATUS Status ;
    UNICODE_STRING Var ;
    UNICODE_STRING Val ;

    if ( !Variable )
    {
        return FALSE ;
    }

    if ( !Value )
    {
        return TRUE ;
    }

    RtlInitUnicodeString( &Var, Variable );
    RtlInitUnicodeString( &Val, Value );
    Status = RtlSetEnvironmentVariable(
                Env,
                &Var,
                &Val);

    return NT_SUCCESS( Status );
}


/***************************************************************************\
* FUNCTION: SetCommonNotifyVariables
*
* PURPOSE:  Sets environment variables to pass information to notify process
*           for data that is common to all notifications.
*           The variables are set in winlogon's environment - this is
*           inherited by the notify process.
*
* RETURNS:  TRUE on success, FALSE on failure
*
*           On failure return, all notify variables have been deleted
*
* HISTORY:
*
*   01-12-93 Davidc       Created.
*
\***************************************************************************/

BOOL
SetCommonNotifyVariables(
    PVOID * Env,
    HWND   hwndOwner,
    LPTSTR Name        OPTIONAL,
    LPTSTR Domain      OPTIONAL,
    LPTSTR Password    OPTIONAL,
    LPTSTR OldPassword OPTIONAL
    )
{
    BOOL Result = TRUE;

    if (Result) {
        Result = SetNotifyVariable(
                    Env,
                    MPR_STATION_NAME_VARIABLE, 
                    WINDOW_STATION_NAME);
    }
    if (Result) {
        Result = SetEnvironmentULong(
                    Env,
                    MPR_STATION_HANDLE_VARIABLE, 
                    (ULONG_PTR)hwndOwner);
    }

    if (Result && ARGUMENT_PRESENT( Name )) {
        Result = SetNotifyVariable(
                    Env,
                    MPR_USERNAME_VARIABLE, 
                    Name);
    }
    if (Result && ARGUMENT_PRESENT( Domain )) {
        Result = SetNotifyVariable(
                    Env,
                    MPR_DOMAIN_VARIABLE, 
                    Domain);
    }
    if (Result && ARGUMENT_PRESENT( Password )) {
        Result = SetNotifyVariable(
                    Env,
                    MPR_PASSWORD_VARIABLE, 
                    Password);
    }
    if (Result) {
        Result = SetEnvironmentULong(
                    Env,
                    MPR_OLD_PASSWORD_VALID_VARIABLE,
                    (OldPassword != NULL) ? 1 : 0);
    }
    if (Result) {
        Result = SetNotifyVariable(
                    Env,
                    MPR_OLD_PASSWORD_VARIABLE, 
                    OldPassword);

        if (OldPassword == NULL) {
            Result = TRUE; // Ignore failure since deleting a variable that
                           // doesn't exist returns failure.
        }
    }

    if (!Result) {
        DebugLog((DEB_ERROR, "SetCommonNotifyVariables: Failed to set a variable, error = %d\n", GetLastError()));
    }

    return(Result);
}


/***************************************************************************\
* FUNCTION: SetLogonNotifyVariables
*
* PURPOSE:  Sets environment variables to pass information to notify process
*           for data that is specific to logon notifications.
*           The variables are set in winlogon's environment - this is
*           inherited by the notify process.
*
* RETURNS:  TRUE on success, FALSE on failure
*
*           On failure return, all notify variables have been deleted
*
* HISTORY:
*
*   01-12-93 Davidc       Created.
*
\***************************************************************************/

BOOL
SetLogonNotifyVariables(
    PVOID * Env,
    PLUID   LogonId
    )
{
    BOOL Result;
    LARGE_INTEGER LargeInt;

    LargeInt.LowPart = LogonId->LowPart;
    LargeInt.HighPart = LogonId->HighPart;
    Result = SetEnvironmentLargeInt(
                Env,
                MPR_LOGONID_VARIABLE, 
                LargeInt);

    if (Result) {
        Result = SetEnvironmentULong(
                    Env,
                    MPR_LOGON_FLAG_VARIABLE, 
                    1);
    }

    if (!Result) {
        DebugLog((DEB_ERROR, "SetLogonNotifyVariables: Failed to set variable, error = %d\n", GetLastError()));
    }

    return(Result);
}


/***************************************************************************\
* FUNCTION: SetChangePasswordNotifyVariables
*
* PURPOSE:  Sets environment variables to pass information to notify process
*           for data that is specific to change password notifications.
*           The variables are set in winlogon's environment - this is
*           inherited by the notify process.
*
* RETURNS:  TRUE on success, FALSE on failure
*
*           On failure return, all notify variables have been deleted
*
* HISTORY:
*
*   01-12-93 Davidc       Created.
*
\***************************************************************************/

BOOL
SetChangePasswordNotifyVariables(
    PVOID Env,
    DWORD ChangeInfo,
    BOOL PassThrough,
    PWSTR Provider OPTIONAL
    )
{
    BOOL Result;

    Result = SetEnvironmentULong(
                Env,
                MPR_CHANGE_INFO_VARIABLE, 
                ChangeInfo);

    if (Result) {
        Result = SetEnvironmentULong(
                    Env,
                    MPR_LOGON_FLAG_VARIABLE, 
                    0);
    }

    if (Result) {
        Result = SetEnvironmentULong(
                    Env,
                    MPR_PASSTHROUGH_VARIABLE, 
                    (PassThrough ? 1 : 0));
    }

    if (Result && ARGUMENT_PRESENT( Provider ) )
    {
        Result = SetNotifyVariable( 
                    Env,
                    MPR_PROVIDER_VARIABLE, 
                    Provider );
    }

    if (!Result) {
        DebugLog((DEB_ERROR, "SetChangePasswordNotifyVariables: Failed to set variable, error = %d\n", GetLastError()));
    }

    return(Result);
}


/***************************************************************************\
* FUNCTION: MprNotifyDlgProc
*
* PURPOSE:  Processes messages for the Mpr Notify dialog
*
* RETURNS:  DLG_SUCCESS     - the notification went without a hitch
*                           - NotifyData->ReturnBuffer is valid.
*           DLG_FAILURE     - something failed or there is no buffer to return.
*                           - NotifyData->ReturnBuffer is invalid.
*
*           DLG_INTERRUPTED() - a set defined in winlogon.h
*
* HISTORY:
*
*   01-11-93 Davidc       Created.
*
\***************************************************************************/

INT_PTR WINAPI
MprNotifyDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PNOTIFY_DATA pNotifyData = (PNOTIFY_DATA)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    PCOPYDATASTRUCT CopyData;

    switch (message) {

    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);

        pNotifyData = (PNOTIFY_DATA)lParam;

        pNotifyData->hDlg = hDlg ;

        if (!MprNotifyDlgInit(hDlg, pNotifyData->pTerm)) {
            EndDialog(hDlg, DLG_FAILURE);
            return(TRUE);
        }

        //
        // Send ourselves a message so we can hide without the
        // dialog code trying to force us to be visible
        //

        PostMessage(hDlg, WM_HIDEOURSELVES, 0, 0);
        return(TRUE);


    case WM_HIDEOURSELVES:
        ShowWindow(hDlg, SW_HIDE);
        return(TRUE);


    case WLX_WM_SAS:

        if (wParam == WLX_SAS_TYPE_USER_LOGOFF)
        {
            DebugLog((DEB_TRACE_MPR, "Got a logoff notification\n"));
        }
        //
        // Interrupt the notify process
        // This gives us a way to terminate the notify process if it hangs up.
        //

        DebugLog((DEB_TRACE_MPR, "Got SAS message - interrupting notify process\n"));
        pNotifyData->SAS = wParam ;
        EndDialog(hDlg, DLG_SAS_RAISED);
        return(TRUE);


    case WM_COPYDATA:

        //
        // The notify process completed and is passing us the result
        //

        CopyData = (PCOPYDATASTRUCT)lParam;

        DebugLog((DEB_TRACE_MPR, "Got WM_COPYDATA message from notify process\n"));
        DebugLog((DEB_TRACE_MPR, "/tdwData = %d", CopyData->dwData));
        DebugLog((DEB_TRACE_MPR, "/tcbData = %d", CopyData->cbData));

        //
        // Copy the passed data and quit this dialog
        //

        if (CopyData->dwData == 0) {
            if (CopyData->cbData != 0) {
                pNotifyData->ReturnBuffer = Alloc(CopyData->cbData);
                if (pNotifyData->ReturnBuffer != NULL) {
                    CopyMemory(pNotifyData->ReturnBuffer, CopyData->lpData, CopyData->cbData);
                } else {
                    DebugLog((DEB_ERROR, ("Failed to allocate memory for returned logon scripts")));
                }
            } else {
                pNotifyData->ReturnBuffer = NULL;
            }

        } else {
            DebugLog((DEB_TRACE_MPR, "Notify completed with an error: %d", CopyData->dwData));
        }

        EndDialog(hDlg, pNotifyData->ReturnBuffer ? DLG_SUCCESS : DLG_FAILURE);

        return(TRUE);   // We processed this message



    case WM_MONITOR_DIED:

        //
        // The notify process terminated for some reason
        //

        DebugLog((DEB_TRACE_MPR, "Notify process terminated - got monitor notification\n"));
        EndDialog(hDlg, DLG_FAILURE);
        return(TRUE);



    case WM_DESTROY:

        //
        // Terminate the notify process and delete the monitor object.
        //

        if (pNotifyData->ProcessRunning) {

            DebugLog((DEB_TRACE_MPR, "NotifyDlgProc: Deleting notify process and monitor\n"));

            DeleteJob( pNotifyData->Job );

            pNotifyData->Job = NULL ;

        }

        return(0);
    }


    // We didn't process the message
    return(FALSE);
}

DWORD
ProviderCallback(
    PVOID Parameter
    )
{
    PNOTIFY_DATA pNotifyData = (PNOTIFY_DATA) Parameter ;

    PostMessage( pNotifyData->hDlg, WM_MONITOR_DIED, 0, 0 );

    return 0 ;
}


/***************************************************************************\
* FUNCTION: MprNotifyDlgInit
*
* PURPOSE:  Handles initialization of Mpr notify dialog
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   01-11-93 Davidc       Created.
*
\***************************************************************************/

#if DEVL
BOOL bDebugMpNotify = FALSE;
#endif

BOOL
MprNotifyDlgInit(
    HWND      hDlg,
    PTERMINAL pTerm
    )
{
    PNOTIFY_DATA pNotifyData = (PNOTIFY_DATA)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    USER_PROCESS_DATA SystemProcessData;
    BOOL Success;
    LPTSTR NotifyApp;
    PROCESS_INFORMATION ProcessInformation;
    PWSTR pchCmdLine;
    WCHAR szDesktop[ 32 ];
    PWINLOGON_JOB Job ;
    MEMORY_BASIC_INFORMATION MemInfo ;

#if DEVL
    WCHAR chDebugCmdLine[ MAX_PATH ];
#endif

    //
    // Initialize flag to show we haven't created the notify process yet
    //

    pNotifyData->ProcessRunning = FALSE;

    //
    // Set our size to zero so we we don't appear
    //

    SetWindowPos(hDlg, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE |
                                         SWP_NOREDRAW | SWP_NOZORDER);



    //
    // Set the winlogon window variable so the process knows who we are
    //

    SetWinlogonWindowVariable(&pNotifyData->Env, hDlg);

    //
    // Start the notify process in system context
    //

    SystemProcessData.UserToken = NULL;
    SystemProcessData.UserSid = g_WinlogonSid;
    SystemProcessData.NewProcessSD = NULL;
    SystemProcessData.NewProcessTokenSD = NULL;
    SystemProcessData.NewThreadSD = NULL;
    SystemProcessData.NewThreadTokenSD = NULL;
    SystemProcessData.Quotas.PagedPoolLimit = 0;
    SystemProcessData.CurrentDirectory = NULL;



    //
    // Get the name of the notify app
    //

    NotifyApp = AllocAndGetProfileString(WINLOGON, NOTIFY_KEY_NAME, DEFAULT_NOTIFY_APP_NAME);
    if (NotifyApp == NULL) {
        DebugLog((DEB_ERROR, "Failed to get name of provider notify app from registry\n"));
        return(FALSE);
    }

    pchCmdLine = NotifyApp;

    //
    // Try and execute it
    //
#if DEVL
        if (bDebugMpNotify) {
            wsprintf( chDebugCmdLine, TEXT("ntsd -d %s%s"),
                     bDebugMpNotify == 2 ? TEXT("-g -G ") : TEXT(""),
                     pchCmdLine
                   );
            pchCmdLine = chDebugCmdLine;
        }
#endif

    wsprintf(szDesktop, WINLOGON_DESKTOP_PATH, pTerm->pWinStaWinlogon->lpWinstaName);
    if ( pTerm->pWinStaWinlogon->ActiveDesktop == Desktop_Winlogon )
    {
        SetNotifyVariable( &pNotifyData->Env, MPR_DESKTOP_VARIABLE, WINLOGON_DESKTOP_NAME );
    }
    else
    {
        SetNotifyVariable( &pNotifyData->Env, MPR_DESKTOP_VARIABLE, APPLICATION_DESKTOP_NAME );

    }

    //
    // Protect the environment:
    //

    VirtualQuery( pNotifyData->Env, &MemInfo, sizeof( MemInfo ) );
    VirtualLock( pNotifyData->Env, MemInfo.RegionSize );

    SystemProcessData.pEnvironment = pNotifyData->Env ;

    //
    // Last moment:
    //

    if ( ExitWindowsInProgress )
    {
        return FALSE ;
    }

    Job = CreateWinlogonJob();

    if ( Job )
    {
        SetWinlogonJobTimeout( Job, INFINITE );

        SetJobCallback( Job, ProviderCallback, pNotifyData );


        Success = StartProcessInJob(
                                pTerm,
                                ProcessAsSystem,
                                szDesktop,
                                pNotifyData->Env,
                                pchCmdLine,
                                0,
                                0,
                                Job );
    }
    else 
    {
        Success = FALSE ;
    }



    Free(NotifyApp);

    if (!Success) {

        DebugLog((DEB_ERROR, "Failed to start multiple provider notifier\n"));

        if ( Job )
        {
            DeleteJob( Job );
        }

        return(FALSE);
    }

    pNotifyData->Job = Job ;

    //
    // Record the fact we started the notify process so we know
    // to cleanup during WM_DESTROY
    //

    pNotifyData->ProcessRunning = TRUE;

    // Success
    return (TRUE);
}




/***************************************************************************\
* FUNCTION: NoNeedToNotify
*
* PURPOSE:  Determines if it is necessary to call the notify apis.
*           It is not necessary if there is only one provider installed.
*
*           We use this to save time in the common case where there is
*           only one provider. We can avoid the overhead of creating
*           the notify process in this case.
*
* RETURNS:  TRUE if there is only one provider, otherwise FALSE
*
* HISTORY:
*
*   01-11-93 Davidc       Created.
*
\***************************************************************************/

#define NET_PROVIDER_ORDER_KEY TEXT("system\\CurrentControlSet\\Control\\NetworkProvider\\Order")
#define NET_PROVIDER_ORDER_VALUE  TEXT("ProviderOrder")
#define NET_ORDER_SEPARATOR  TEXT(',')

BOOL
NoNeedToNotify(
    VOID
    )
{
    HKEY ProviderKey;
    DWORD Error;
    DWORD ValueType;
    LPTSTR Value;
    BOOL NeedToNotify = TRUE;

    Error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,     // hKey
                NET_PROVIDER_ORDER_KEY, // lpSubKey
                0,                      // Must be 0
                KEY_QUERY_VALUE,        // Desired access
                &ProviderKey            // Newly Opened Key Handle
                );

    if (Error != ERROR_SUCCESS) {
        DebugLog((DEB_ERROR, "NoNeedToNotify - failed to open provider key, assuming notification is necessary\n"));
        return(!NeedToNotify);
    }

    Value = AllocAndRegQueryValueEx(
                ProviderKey,            // Key
                NET_PROVIDER_ORDER_VALUE,// Value name
                NULL,                   // Must be NULL
                &ValueType              // Type returned here
                );

    if (Value != NULL) {
        if (ValueType == REG_SZ) {

            LPTSTR p = Value;
            while (*p) {
                if (*p == NET_ORDER_SEPARATOR) {
                    break;
                }
                p = CharNext(p);
            }

            if (*p == 0) {

                //
                // We got to the end without finding a separator
                // Only one provider is installed.
                //

                if (lstrcmpi(Value, SERVICE_WORKSTATION) == 0) {

                    //
                    // it's Lanman, don't notify
                    //

                    NeedToNotify = FALSE;


                } else {

                    //
                    //  it isn't Lanman, notify
                    //

                    NeedToNotify = TRUE;
                }
            }

        } else {
            DebugLog((DEB_ERROR, "NoNeedToNotify - provider order key unexpected type: %d, assuming notification is necessary", ValueType));
        }

        Free(Value);

    } else {
        DebugLog((DEB_ERROR, "NoNeedToNotify - failed to query provider order value, assuming notification is necessary\n"));
    }

    Error = RegCloseKey(ProviderKey);
    ASSERT(Error == ERROR_SUCCESS);

    return(!NeedToNotify);
}



/***************************************************************************\
* MprLogonNotify
*
* Purpose : Notifies credential managers of a logon.
*
* RETURNS:  DLG_SUCCESS     - the notification went without a hitch
*           DLG_FAILURE     - something failed.
*           DLG_INTERRUPTED() - a set of interruptions defined in winlogon.h
*
* On DLG_SUCCESS return MprLogonScripts contains a pointer to a
* Multi-sz string or NULL if there is no data. i.e. multiple concatenated
* zero terminated strings with a final terminator.
* The memory should be freed by the caller (if pointer non-NULL) using Free().
*
* History:
* 11-12-92 Davidc       Created.
\***************************************************************************/

int
MprLogonNotify(
    PTERMINAL pTerm,
    HWND hwndOwner,
    LPTSTR Name,
    LPTSTR Domain,
    LPTSTR Password,
    LPTSTR OldPassword OPTIONAL,
    PLUID LogonId,
    LPWSTR *MprLogonScripts
    )
{
    int Result;
    NOTIFY_DATA NotifyData;
    NTSTATUS Status ;

    //
    // Check if we really need to bother with this
    //

    if (NoNeedToNotify()) {
        DebugLog((DEB_TRACE_MPR, "MprLogonNotify - skipping notification - only one provider\n"));
        *MprLogonScripts = NULL;
        return(DLG_SUCCESS);
    }

    RtlZeroMemory( &NotifyData, sizeof( NotifyData ) );
    //
    // Set up the environment variables that we will use to pass
    // information to notify process
    //

    Status = RtlCreateEnvironment(
                TRUE,
                &NotifyData.Env );

    if ( !NT_SUCCESS( Status ) )
    {
        return DLG_FAILURE ;
    }


    if (!SetCommonNotifyVariables(
                             &NotifyData.Env,
                             hwndOwner,
                             Name,
                             Domain,
                             Password,
                             OldPassword
                             )) {
        return(DLG_FAILURE);
    }

    if (!SetLogonNotifyVariables(&NotifyData.Env, LogonId)) {
        return(DLG_FAILURE);
    }


    //
    // Initialize our notify data structure
    //


    NotifyData.pTerm = pTerm;
    NotifyData.ReturnBuffer = NULL;

    //
    // Update windowstation lock so mpnotify can start.
    //

    if ( pTerm->pWinStaWinlogon->ActiveDesktop == Desktop_Winlogon )
    {
        UnlockWindowStation(pTerm->pWinStaWinlogon->hwinsta);
        FastSetWinstaSecurity(pTerm->pWinStaWinlogon, FALSE);
    }



    //
    // Create the dialog that will initiate the notify and wait
    // for it to complete
    //

    Result = WlxDialogBoxParam( pTerm,
                                g_hInstance,
                                (LPTSTR)IDD_CONTROL,
                                hwndOwner,
                                MprNotifyDlgProc,
                                (LPARAM)&NotifyData);

    if (Result == DLG_SUCCESS) 
    {
        DebugLog((DEB_TRACE_MPR, "Logon notification return buffer (first string only) = <%ws>\n", NotifyData.ReturnBuffer));

        *MprLogonScripts = NotifyData.ReturnBuffer;

    } 
    else 
    {
        DebugLog((DEB_TRACE_MPR, "Logon notification failed\n"));
    }



    //
    // Re-lock the windowstation.
    //

    if ( pTerm->pWinStaWinlogon->ActiveDesktop == Desktop_Winlogon )
    {
        LockWindowStation(pTerm->pWinStaWinlogon->hwinsta);
    }

    RtlDestroyEnvironment( NotifyData.Env );

    if ( NotifyData.SAS != 0 )
    {
        DebugLog(( DEB_TRACE_MPR, "SAS interruption - reposting SAS\n" ));
        EnableSasMessages( NULL, pTerm );
        SASRouter( pTerm, (DWORD) NotifyData.SAS );
    }

    return(Result);
}



/***************************************************************************\
* MprChangePasswordNotify
*
* Purpose : Notifies credential managers of a password change
*
* RETURNS:  DLG_SUCCESS     - the notification went without a hitch
*           DLG_FAILURE     - something failed.
*           DLG_INTERRUPTED() - a set of interruptions defined in winlogon.h
*
* History:
* 01-12-93 Davidc       Created.
\***************************************************************************/

int
MprChangePasswordNotify(
    PTERMINAL pTerm,
    HWND hwndOwner,
    PWSTR Provider,
    LPTSTR Name,
    LPTSTR Domain,
    LPTSTR Password,
    LPTSTR OldPassword,
    DWORD ChangeInfo,
    BOOL PassThrough
    )
{
    int Result;
    NOTIFY_DATA NotifyData;
    NTSTATUS Status ;

    //
    // Check if we really need to bother with this
    //

    if (NoNeedToNotify()) {
        DebugLog((DEB_TRACE_MPR, "MprChangePasswordNotify - skipping notification - only one provider\n"));
        return(DLG_SUCCESS);
    }

    //
    // Set up the environment variables that we will use to pass
    // information to notify process
    //

    Status = RtlCreateEnvironment(
                TRUE,
                &NotifyData.Env );

    if (!SetCommonNotifyVariables(
                             &NotifyData.Env,
                             hwndOwner,
                             Name,
                             Domain,
                             Password,
                             OldPassword
                             )) {
        return(DLG_FAILURE);
    }

    if (!SetChangePasswordNotifyVariables(&NotifyData.Env,
                                          ChangeInfo,
                                          PassThrough,
                                          Provider ) )
    {
        return(DLG_FAILURE);
    }


    //
    // Initialize our notify data structure
    //

    NotifyData.pTerm = pTerm;
    NotifyData.ReturnBuffer = NULL;


    //
    // Update windowstation security so mpnotify can start.
    //

    FastSetWinstaSecurity( pTerm->pWinStaWinlogon,
                           FALSE );

    //
    // Create the dialog that will initiate the notify and wait
    // for it to complete
    //

    //
    // Set timeout to 5 minutes, so the nwcs provider has time to run.
    //

    WlxSetTimeout( pTerm, 5 * 60 );

    Result = WlxDialogBoxParam( pTerm,
                                g_hInstance,
                                (LPTSTR)IDD_CONTROL,
                                hwndOwner,
                                MprNotifyDlgProc,
                                (LPARAM)&NotifyData);
    //
    // Reset the windowstation security.
    //

    FastSetWinstaSecurity( pTerm->pWinStaWinlogon,
                           TRUE );

    if (Result == DLG_SUCCESS) {
        Free(NotifyData.ReturnBuffer);
    } else {
        DebugLog((DEB_TRACE_MPR, "Change password notification failed\n"));
    }

    RtlDestroyEnvironment( NotifyData.Env );

    return(Result);
}
