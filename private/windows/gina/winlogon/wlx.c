/****************************** Module Header ******************************\
* Module Name: wlx.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Winlogon main module
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include <commctrl.h>
#define NO_COMMCTRL_ALLOCFCNS
#include <comctrlp.h>

#if DBG
char * SASTypes[] = { "Timeout", "Ctrl-Alt-Del", "ScreenSaver Timeout",
                      "ScreenSaver Activity", "User Logoff", "S/C Insert",
                      "S/C Remove" };
#define SASName(x)  (x < (sizeof(SASTypes) / sizeof(char *)) ? SASTypes[x] : "User Defined")

char * WlxRets[] = { "invalid", "Logon", "None", "LockWksta", "Logoff", "Shutdown",
                        "Pwd Changed", "TaskList", "UnlockWksta", "ForceLogoff",
                        "Shutdown-PowerOff", "Shutdown-Reboot", "Sleep", "Sleep2", "Hibernate" };
#define WlxName(x)  (x < (sizeof(WlxRets) / sizeof(char *)) ? WlxRets[x] : "Invalid")
#endif

#define RealFlagsFromStoredFlags(Flags) \
                EWX_LOGOFF | \
                ((Flags & EWX_WINLOGON_OLD_SYSTEM) ? EWX_SYSTEM_CALLER : 0) | \
                ((Flags & EWX_WINLOGON_OLD_SHUTDOWN) ? EWX_SHUTDOWN : 0) | \
                ((Flags & EWX_WINLOGON_OLD_REBOOT) ? EWX_REBOOT : 0) | \
                ((Flags & EWX_WINLOGON_OLD_POWEROFF) ? EWX_POWEROFF : 0)

#define StoredFlagsFromRealFlags(Flags) \
                EWX_LOGOFF | \
                ((Flags & EWX_SYSTEM_CALLER) ? EWX_WINLOGON_OLD_SYSTEM : 0) | \
                ((Flags & EWX_SHUTDOWN) ? EWX_WINLOGON_OLD_SHUTDOWN : 0) | \
                ((Flags & EWX_REBOOT) ? EWX_WINLOGON_OLD_REBOOT : 0) | \
                ((Flags & EWX_POWEROFF) ? EWX_WINLOGON_OLD_POWEROFF : 0)


BOOLEAN SasMessages = TRUE;
extern BOOL ReturnFromPowerState ;

#ifndef _WIN64

extern HWND g_hwndAppDesktopThread;

#endif // _WIN64

typedef BOOL (*PFNRESTORENETCON)(
     HWND   hwndParent,
     LPTSTR lpDevice
     );


//
// For checking page file
//

extern TCHAR szMemMan[];

extern TCHAR szNoPageFile[];

//
// For migration
//

TCHAR szAdminName[ MAX_STRING_BYTES ];

//
// application desktop thread declaration
//

VOID StartAppDesktopThread(PTERMINAL pTerm);


//
// for closing Ras Connections
//

BOOL
DeleteRasConnections(
    PTERMINAL pTerm
    );


typedef 
(WINAPI *PFNINITCOMMONCONTROLSEX)(LPINITCOMMONCONTROLSEX);

//
// Local Prototypes
//

int
DoScreenSaver(
    PTERMINAL   pTerm,
    BOOL        WkstaLocked);

void
WinsrvNotify(
    PTERMINAL   pTerm,
    DWORD       SasType);

BOOL
LoggedonDlgInit(
    HWND    hDlg
    );

int MultiUserLogonAttempt(
        PTERMINAL pTerm,
        PWLX_MPR_NOTIFY_INFO pMprInfo,
        HANDLE hToken);

VOID
NetworkProviderTask(
    PVOID pv,
    BOOLEAN timeout
    )
{
    PTERMINAL Terminal ;
    PWINDOWSTATION WS ;
    WLX_MPR_NOTIFY_INFO MprInfo ;
    PWSTR LogonScripts = NULL ;

    if ( timeout )
    {
        return;
    }

    Terminal = (PTERMINAL) pv ;

    WS = Terminal->pWinStaWinlogon ;

    if ( (!IsLocked( Terminal->WinlogonState)) &&
         (!Terminal->ScreenSaverActive) &&
         (!Terminal->ShutdownStarted) )
    {
        DebugLog(( DEB_TRACE, "NetworkProviderTask:  Event set, calling gina\n" ));

        if ( Terminal->Gina.pWlxNetworkProviderLoad( Terminal->Gina.pGinaContext,
                                                      & MprInfo ) )
        {
            DebugLog(( DEB_TRACE, "NPTask:  Gina returned success, notifying providers\n" ));

            MprLogonNotify(
                        Terminal,
                        NULL,
                        MprInfo.pszUserName,
                        MprInfo.pszDomain,
                        MprInfo.pszPassword,
                        MprInfo.pszOldPassword,
                        &WS->LogonId,
                        &LogonScripts );

            DestroyMprInfo(&MprInfo);

            if ( LogonScripts )
            {
                LocalFree( LogonScripts );
            }

        }
    }
    else
    {
        DebugLog(( DEB_TRACE, "NetworkProviderTask: Dropping suspect np load event\n" ));
    }

    return ;
}


//+---------------------------------------------------------------------------
//
//  Function:   DropWorkingSet
//
//  Synopsis:   Reduce working set when we're
//
//  Arguments:  (none)
//
//  History:    11-04-94   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void
DropWorkingSet(void)
{
    NTSTATUS    Status;
    QUOTA_LIMITS    Quota;

    Status = NtQueryInformationProcess( NtCurrentProcess(),
                                        ProcessQuotaLimits,
                                        &Quota,
                                        sizeof(QUOTA_LIMITS),
                                        NULL );

    if (NT_SUCCESS(Status))
    {
        Quota.MinimumWorkingSetSize = (SIZE_T)-1;
        Quota.MaximumWorkingSetSize = (SIZE_T)-1;

        NtSetInformationProcess(NtCurrentProcess(),
                                ProcessQuotaLimits,
                                &Quota,
                                sizeof(QUOTA_LIMITS) );
    }
}

//
// Code from the shell guys to flush comctl32's notion of the atom table.
// 
DWORD
PokeComCtl32(
    PVOID ignored
    )
{
    HINSTANCE ComCtl;
    PFNINITCOMMONCONTROLSEX ICCEx;

    ComCtl = GetModuleHandle( L"COMCTL32" );

    if ( ComCtl )
    {
        ICCEx = (PFNINITCOMMONCONTROLSEX) GetProcAddress( ComCtl, "InitCommonControlsEx" );

        if ( ICCEx )
        {
            INITCOMMONCONTROLSEX iccex = { sizeof(INITCOMMONCONTROLSEX), ICC_WINLOGON_REINIT };
            ICCEx( &iccex );
        }
    }

    return 0 ;
}


//+---------------------------------------------------------------------------
//
//  Function:   InitializeGinaDll
//
//  Synopsis:   Initializes the gina
//
//  Arguments:  [pTerm] --
//              [pWS]   --
//              [ppGinaContext] -- Receives the gina context
//
//  History:    10-17-94   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
InitializeGinaDll(PTERMINAL  pTerm)
{

#if DBG
    if (TEST_FLAG(GinaBreakFlags, BREAK_INITIALIZE))
    {
        DebugLog((DEB_TRACE, "About to call WlxInitialize\n"));


        DebugBreak();
    }
#endif


    //
    // Perversely, this may not return.  The GINA may in fact call SASNotify
    // immediately, so update the state before we go in:
    //

    pTerm->WinlogonState = Winsta_NoOne;
    DebugLog((DEB_TRACE_STATE, "InitGina:  State is %d %s\n", Winsta_NoOne, GetState(Winsta_NoOne)));

    if (!pTerm->Gina.pWlxInitialize(pTerm->pWinStaWinlogon->lpWinstaName,
                                pTerm,
                                NULL,
                                (PVOID) &WlxDispatchTable,
                                &pTerm->Gina.pGinaContext))
    {
        //
        // If the GINA failed to init, we're dead.  bugcheck time:
        //

        ExitProcess( EXIT_GINA_INIT_ERROR );
    }
    return(TRUE);
}



//+---------------------------------------------------------------------------
//
//  Function:   SASRouter
//
//  Synopsis:   Routes an SAS event to the appropriate recipient
//
//  Arguments:  [pTerm] --
//              [SasType]  --
//
//  History:    8-24-94   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void
SASRouter(  PTERMINAL   pTerm,
            DWORD       SasType )
{


    if (!TestSasMessages())
    {
        QueueSasEvent(SasType, pTerm);
        return;
    }

    pTerm->SasType = SasType;

    if (!IsSASState(pTerm->WinlogonState))
    {
        if (IsDisplayState(pTerm->WinlogonState) ||
            (pTerm->WinlogonState == Winsta_WaitForShutdown) ||
            (pTerm->WinlogonState == Winsta_Locked))
        {
            DebugLog((DEB_TRACE, "In state %s, sending kill message to window\n",
                        GetState(pTerm->WinlogonState)));

            if (!SendSasToTopWindow(pTerm, SasType))
                DebugLog((DEB_WARN, "No window to send SAS notice to?\n"));
        }

        //
        // If this was a timeout message,
        if ((SasType == WLX_SAS_TYPE_SCRNSVR_TIMEOUT) ||
            (SasType == WLX_SAS_TYPE_TIMEOUT) )
        {
            //
            // We do *not* change state on a timeout!
            //
            return;
        }

        ChangeStateForSAS(pTerm);


        if (!pTerm->ScreenSaverActive)
        {
            SetActiveDesktop(pTerm, Desktop_Winlogon);
        }

        //
        // We should be in one of the three base states now:
        //

        DebugLog((DEB_TRACE_STATE, "SASRouter:  In state %s\n", GetState(pTerm->WinlogonState)));
        switch (pTerm->WinlogonState)
        {
            case Winsta_NoOne_SAS:
            case Winsta_LoggedOn_SAS:
            case Winsta_Locked_SAS:
            case Winsta_WaitForLogoff:
            case Winsta_WaitForShutdown:
                if ( !ReturnFromPowerState )
                {                    
                    DisableSasMessages();
                }
                break;


            default:
                DebugLog((DEB_ERROR, "SASRouter: Incorrect state %d, %s.\n",
                            pTerm->WinlogonState, GetState(pTerm->WinlogonState)));
                break;

        }
    }
    else
    {
        //
        // We are already handling an SAS attempt.
        //
        // Note:  This may fail.  There may not be a window currently to
        // receive the message.  Life is tough that way.  The SAS will be
        // *dropped*.
        //

        DebugLog((DEB_TRACE, "Sending SAS %s to top window\n", SASName(SasType)));

        SendSasToTopWindow(pTerm, SasType);
    }


}


//+---------------------------------------------------------------------------
//
//  Function:   CADNotify
//
//  Synopsis:   Called by sas.c, this is the entrypoint for a Ctrl-Alt-Del
//              call.  Expanded to handle all notification from winsrv.
//
//  Arguments:  [pTerm] --
//              [SasType]  --
//
//  Algorithm:
//
//  History:    10-17-94   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void
CADNotify(
    PTERMINAL   pTerm,
    DWORD       SasType)
{
    DebugLog((DEB_TRACE, "Received SAS from winsrv, code %d (%s)\n", SasType, SASName(SasType)));
    if (SasType == WLX_SAS_TYPE_USER_LOGOFF)
    {
        WinsrvNotify(pTerm, SasType);
    }
    else if (pTerm->ForwardCAD)
    {
        SASRouter(pTerm, SasType);
    }
}

PWSTR
AllocAndDuplicateString(
    PWSTR   pszString)
{
    int     len;
    PWSTR   pszNewString;

    if (!pszString)
    {
        return(NULL);
    }

    len = (wcslen(pszString) + 1) * sizeof(WCHAR);

    pszNewString = LocalAlloc(LMEM_FIXED, len);
    if (pszNewString)
    {
        CopyMemory(pszNewString, pszString, len);
    }

    return(pszNewString);

}

PWSTR
AllocAndDuplicateStrings(
    PWSTR   pszStrings)
{
    DWORD   len;
    PWSTR   pszNewStrings;

    if (!pszStrings)
    {
        return(NULL);
    }


    len = (DWORD)LocalSize (pszStrings);

    pszNewStrings = LocalAlloc(LPTR, len);
    if (pszNewStrings)
    {
        CopyMemory(pszNewStrings, pszStrings, len);
    }

    return(pszNewStrings);

}


PVOID
CopyEnvironment(
    PVOID   pEnv)
{
    MEMORY_BASIC_INFORMATION    mbi;
    PVOID                       pNew;

    if ( !pEnv )
    {
        return NULL ;
    }

    if (VirtualQueryEx(
                GetCurrentProcess(),
                pEnv,
                &mbi,
                sizeof(mbi) ) )
    {
        pNew = VirtualAlloc(NULL,
                            mbi.RegionSize,
                            MEM_COMMIT,
                            PAGE_READWRITE);
        if (pNew)
        {
            CopyMemory(pNew, pEnv, mbi.RegionSize);
            return(pNew);
        }
    }

    return(NULL);
}



//+---------------------------------------------------------------------------
//
//  Function:   LogonAttempt
//
//  Synopsis:   Handles a logon attempt.
//
//  Arguments:  [pTerm] --
//
//  History:    10-17-94   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
int
LogonAttempt(
    PTERMINAL    pTerm)
{
    DWORD               WlxResult;
    WLX_MPR_NOTIFY_INFO MprInfo;
    PWLX_PROFILE_V2_0   pProfileInfo;
    PSID                pLogonSid;
    DWORD               Options = 0;
    HANDLE              hToken;
    HANDLE              uh;
    PWINDOWSTATION      pWS = pTerm->pWinStaWinlogon;
    PVOID               pGinaContext = pTerm->Gina.pGinaContext;
    HKEY                hKey;
    DWORD               dwType, dwSize;
    DWORD               dwVal;
    LONG                lResult;
    INT                 i;

    pLogonSid = CreateLogonSid(NULL);

    if ( pLogonSid == NULL )
    {
        return WLX_SAS_ACTION_NONE ;
    }

#if DBG
    if (TEST_FLAG(GinaBreakFlags, BREAK_LOGGEDOUT))
    {
        DebugLog((DEB_TRACE, "About to call WlxLoggedOutSAS(%d, @%x,\n",
                                    pTerm->SasType, pLogonSid));
        DebugLog((DEB_TRACE, "   @%#x, @%#x, @%#x, @%#x)\n", &Options, &hToken,
                                    &MprInfo, &pProfileInfo));

        DebugBreak();
    }
#endif

    //
    // See if multi-user Window Stations are disabled
    //
    if (!g_Console) {
        //
        //  BUGBUG Disabling winstations should be in terminal server and not in
        //  the registry. WinStationNotifyLogon() can return an error status noting this, and
        //  winlogon can call the GINA to put up the message.
        //
        if (GetProfileInt(APPLICATION_NAME, WINSTATIONS_DISABLED, 0) == 1)  {
            TimeoutDialogBoxParam(
                    pTerm,
                    NULL,
                    (LPTSTR)IDD_DISABLED,
                    NULL,
                    LogonDisabledDlgProc,
                    0,
                    TIMEOUT_NONE
                    );

            WlxResult = WLX_SAS_ACTION_LOGOFF;
            pTerm->WinlogonState = Winsta_NoOne;
            DeleteLogonSid(pLogonSid);
            return(WlxResult);
        }

        //
        // Send a signal to the communications channel that
        // we are going into Secure-Attention-Sequence mode.
        //
        gpfnWinStationSetInformation(
            SERVERNAME_CURRENT,
            LOGONID_CURRENT,
            WinStationSecureDesktopEnter,
            NULL,
            0);
    }

    //
    // if this terminal has had a log out, then tell the gina to ignore auto logon
    //

    if ( pTerm->IgnoreAutoLogon )
        Options |= WLX_OPTION_IGNORE_AUTO_LOGON;

    g_fAllowStatusUI = FALSE;
    RemoveStatusMessage(TRUE);

    WlxSetTimeout(pTerm, 120);

    WlxResult = pTerm->Gina.pWlxLoggedOutSAS(pGinaContext,
            pTerm->SasType,
            &pWS->LogonId,
            pLogonSid,
            &Options,
            &hToken,
            &MprInfo,
            &pProfileInfo );
    DebugLog((DEB_TRACE, "WlxLoggedOutSAS returned %d, %s\n", WlxResult, WlxName(WlxResult)));

    g_fAllowStatusUI = TRUE;

    //
    // Signal the communications channel that we are no longer
    // in SAS mode.
    //
    if( !g_Console ) {
        gpfnWinStationSetInformation(
            SERVERNAME_CURRENT,
            LOGONID_CURRENT,
            WinStationSecureDesktopExit,
            NULL,
            0);
    }

    if (WlxResult == WLX_SAS_ACTION_LOGON)
    {
        if (MprInfo.pszUserName)
        {
            pWS->UserName = AllocAndDuplicateString(MprInfo.pszUserName);
        }

        if ( MprInfo.pszDomain )
        {
            pWS->Domain = AllocAndDuplicateString( MprInfo.pszDomain );
        }

        if (g_IsTerminalServer)
        {
            WlxResult = MultiUserLogonAttempt(pTerm, &MprInfo, hToken);
        }
    }

    if (WlxResult != WLX_SAS_ACTION_LOGON)
    {
        DebugLog((DEB_TRACE_STATE, "LogonAttempt:  Resetting state to %s\n", GetState(Winsta_NoOne)));

        pTerm->WinlogonState = Winsta_NoOne;

        DeleteLogonSid(pLogonSid);

        if (pWS->UserName)
        {
            FreeAndNull(pWS->UserName);
        }

        if (pWS->Domain)
        {
            FreeAndNull(pWS->Domain);
        }

        return(WlxResult);
    }


    //
    // Check if the profile is set for merge, from a Win9x upgrade.
    //

    if (pWS->Domain) {

        i = WLX_SAS_ACTION_LOGON;

        lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                                LOCAL_USERS_KEY,
                                0,
                                KEY_READ|KEY_WRITE,
                                &hKey);

        if (lResult == ERROR_SUCCESS) {
            //
            // Check the user who is logging on
            //

            dwSize = sizeof (dwVal);

            lResult = RegQueryValueEx (hKey,
                                       pWS->UserName,
                                       NULL,
                                       &dwType,
                                       (PBYTE) &dwVal,
                                       &dwSize
                                       );

            if (lResult == ERROR_SUCCESS) {
                //
                // Ask the user if they want to merge their profiles
                //

                ImpersonateLoggedOnUser (hToken);

                lResult = MergeProfiles (pTerm);

                RevertToSelf();

                if (lResult == ERROR_SUCCESS || lResult == ERROR_CONTINUE) {
                    //
                    // The profile was remapped or split into separate profiles
                    //

                    RegDeleteValue (hKey, pWS->UserName);

                    if (lResult != ERROR_CONTINUE) {
                        i = WLX_SAS_ACTION_NONE;
                    }

                } else if (lResult == ERROR_REQUEST_ABORTED) {
                    //
                    // User cancelled the UI
                    //

                    // nothing needed

                } else {
                    //
                    // An error occurred (code in lResult)
                    //

                    i = WLX_SAS_ACTION_LOGOFF;
                }
            }

            RegCloseKey (hKey);

            if (i != WLX_SAS_ACTION_LOGON) {
                //
                // Re-logon is required either because the profile
                // was remapped, or an error occurred.
                //

                return i;
            }
        }
    }

    //
    // Okay, someone logged on.  This, this is interesting.
    //

    MprLogonNotify(
            pTerm,
            NULL,
            MprInfo.pszUserName,
            MprInfo.pszDomain,
            MprInfo.pszPassword,
            MprInfo.pszOldPassword,
            &pWS->LogonId,
            &pWS->LogonScripts);

    DestroyMprInfo(&MprInfo);

    //
    // Wait on the font loading thread here, so we don't inadvertantly re-enter
    // the stuff in user that gets confused.
    //

    if ( hFontThread )
    {
        WaitForSingleObject( hFontThread, INFINITE );

        CloseHandle( hFontThread );

        hFontThread = NULL;

    }

    SecurityChangeUser(pTerm, hToken, NULL, pLogonSid, TRUE);

    if (!TEST_FLAG(Options, WLX_LOGON_OPT_NO_PROFILE))
    {
        if (pProfileInfo) {
            if (pProfileInfo->pszProfile)
            {
                pWS->UserProfile.ProfilePath =
                    AllocAndExpandEnvironmentStrings(pProfileInfo->pszProfile);
                LocalFree(pProfileInfo->pszProfile);
            }
            else
            {
                pWS->UserProfile.ProfilePath = NULL;
            }

            if (pProfileInfo->dwType >= WLX_PROFILE_TYPE_V2_0) {
                if (pProfileInfo->pszPolicy)
                {
                    pWS->UserProfile.PolicyPath =
                        AllocAndDuplicateString(pProfileInfo->pszPolicy);

                    LocalFree(pProfileInfo->pszPolicy);
                }
                else
                {
                    pWS->UserProfile.PolicyPath = NULL;
                }

                if (pProfileInfo->pszNetworkDefaultUserProfile)
                {
                    pWS->UserProfile.NetworkDefaultUserProfile =
                        AllocAndDuplicateString(pProfileInfo->pszNetworkDefaultUserProfile);

                    LocalFree(pProfileInfo->pszNetworkDefaultUserProfile);
                }
                else
                {
                    pWS->UserProfile.NetworkDefaultUserProfile = NULL;
                }

                if (pProfileInfo->pszServerName)
                {
                    pWS->UserProfile.ServerName =
                        AllocAndDuplicateString(pProfileInfo->pszServerName);

                    LocalFree(pProfileInfo->pszServerName);
                }
                else
                {
                    pWS->UserProfile.ServerName = NULL;
                }

                if (pProfileInfo->pszEnvironment)
                {
                    pWS->UserProfile.Environment =
                        AllocAndDuplicateStrings(pProfileInfo->pszEnvironment);

                    LocalFree(pProfileInfo->pszEnvironment);
                }
                else
                {
                    pWS->UserProfile.Environment = NULL;
                }
            } else {
                pWS->UserProfile.PolicyPath = NULL;
                pWS->UserProfile.NetworkDefaultUserProfile = NULL;
                pWS->UserProfile.ServerName = NULL;
                pWS->UserProfile.Environment = NULL;
            }
        }

        DebugLog((DEB_TRACE_PROFILE, "Using initial profile path of %ws\n", pWS->UserProfile.ProfilePath));

        LocalFree(pProfileInfo);


        //
        // Load profile, set environment variables, etc.
        //

        if (SetupUserEnvironment(pTerm))
        {
            OpenIniFileUserMapping(pTerm);

            uh = ImpersonateUser(&pWS->UserProcessData, NULL);

            SetWindowStationUser(pWS->hwinsta, &pWS->LogonId,
                pWS->UserProcessData.UserSid,
                RtlLengthSid(pWS->UserProcessData.UserSid));

            StopImpersonating(uh);

            //
            // Update the window station lock so that apps can start.
            //

            UnlockWindowStation(pWS->hwinsta);
            LockWindowStation(pWS->hwinsta);

            //
            // allocate floppies and CDRoms (if so configured)
            //

            RmvAllocateRemovableMedia( pLogonSid );

            //
            // Start a task for the network provider notify
            //
            if ( NetworkProviderEvent )
            {
                RegisterWaitForSingleObject(
                    &pWS->UserProfile.NetworkProviderTask,
                    NetworkProviderEvent,
                    NetworkProviderTask,
                    pTerm,
                    INFINITE,
                    0);

            }
        }
        else
        {
            //
            // Whoops, something went wrong.  we *must* log the user
            // out.  We do this by passing LOGOFF back to mainloop.
            //

            WlxResult = WLX_SAS_ACTION_LOGOFF;
        }
    }

    return(WlxResult);

}

/****************************************************************************\
*
* FUNCTION: DisplayPreShellLogonMessages
*
* PURPOSE:  Displays any security warnings to the user after a successful logon
*           The messages are displayed before the shell starts
*
* RETURNS:  DLG_SUCCESS - the dialogs were displayed successfully.
*           DLG_INTERRUPTED() - a set defined in winlogon.h
*
* NOTE:     Screen-saver timeouts are handled by our parent dialog so this
*           routine should never return DLG_SCREEN_SAVER_TIMEOUT
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\****************************************************************************/

int
DisplayPreShellLogonMessages(
    PTERMINAL    pTerm
    )
{
    int Result;

    if (PageFilePopup) {
        HKEY hkeyMM;
        DWORD dwTempFile, cbTempFile, dwType;

        //
        // WinLogon created a temp page file.  If a previous user has not
        // created a real one already, then inform this user to do so.
        //

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szMemMan, 0, KEY_READ,
                &hkeyMM) == ERROR_SUCCESS) {

            cbTempFile = sizeof(dwTempFile);
            if (RegQueryValueEx (hkeyMM, szNoPageFile, NULL, &dwType,
                    (LPBYTE) &dwTempFile, &cbTempFile) != ERROR_SUCCESS ||
                    dwType != REG_DWORD || cbTempFile != sizeof(dwTempFile)) {
                dwTempFile = 0;
            }

            RegCloseKey(hkeyMM);
        } else
            dwTempFile = 0;

        if (dwTempFile == 1) {

            WlxSetTimeout(pTerm, TIMEOUT_NONE);

            Result = TimeoutMessageBox(
                             pTerm,
                             NULL,
                             IDS_NO_PAGING_FILE,
                             IDS_LIMITED_RESOURCES,
                             MB_OK | MB_ICONSTOP
                             );

            if (Result == WLX_DLG_INPUT_TIMEOUT) {
                return(Result);
            }
        }
    }

    return(DLG_SUCCESS);
}


//
// Function proto-types for powrprof.dll apis
//

typedef BOOLEAN (*PFNGETACTIVEPWRSCHEME)(PUINT);
typedef BOOLEAN (*PFNSETACTIVEPWRSCHEME)(UINT, LPVOID, LPVOID);

BOOL SetPowerProfile(PTERMINAL pTerm)
{
    HANDLE hImp;
    UINT uScheme;
    HINSTANCE hInstDLL;
    PFNGETACTIVEPWRSCHEME pfnGetActivePwrScheme;
    PFNSETACTIVEPWRSCHEME pfnSetActivePwrScheme;


    if (OpenHKeyCurrentUser(pTerm->pWinStaWinlogon))
    {
        if (OpenIniFileUserMapping(pTerm))
        {
            if (hImp = ImpersonateUser(&pTerm->pWinStaWinlogon->UserProcessData, NULL))
            {
                hInstDLL = LoadLibrary (TEXT("powrprof.dll"));

                if (hInstDLL)
                {
                    pfnGetActivePwrScheme = (PFNGETACTIVEPWRSCHEME)GetProcAddress (hInstDLL,
                                                   "GetActivePwrScheme");

                    pfnSetActivePwrScheme = (PFNSETACTIVEPWRSCHEME)GetProcAddress (hInstDLL,
                                                   "SetActivePwrScheme");

                    if (pfnGetActivePwrScheme && pfnSetActivePwrScheme)
                    {
                        if (pfnGetActivePwrScheme(&uScheme))
                        {
                            pfnSetActivePwrScheme(uScheme, NULL, NULL);
                        }
                    }

                    FreeLibrary (hInstDLL);
                }

                StopImpersonating(hImp);
            }

            CloseIniFileUserMapping(pTerm);
        }

        CloseHKeyCurrentUser(pTerm->pWinStaWinlogon);
    }

    return TRUE;
}

typedef struct _SOUND_THREAD_PARMS {
    PTERMINAL Terminal ;
    HANDLE Event ;
    LPCTSTR Sound ;
} SOUND_THREAD_PARMS, * PSOUND_THREAD_PARMS ;

DWORD
PlaySoundThread(
    PVOID Parameter
    )
{
    PSOUND_THREAD_PARMS pParms ;
    SOUND_THREAD_PARMS Parms ;

    pParms = (PSOUND_THREAD_PARMS) Parameter ;

    Parms = *pParms ;


    SetEvent( Parms.Event );

    __try
    {
        PlaySound( Parms.Sound,
                   NULL,
                   SND_ALIAS_ID | SND_NODEFAULT );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        NOTHING ;
    }


    return 0 ;
}

PlaySoundAsync(
    PTERMINAL pTerminal,
    LPCTSTR Sound
    )
{
    HANDLE hThread ;
    SOUND_THREAD_PARMS Parms ;
    DWORD tid ;

    Parms.Terminal = pTerminal ;
    Parms.Event = CreateEvent( NULL, FALSE, FALSE, NULL );
    Parms.Sound = Sound ;

    hThread = ExecUserThread( pTerminal,
                              PlaySoundThread,
                              &Parms,
                              0,
                              &tid );

    if ( hThread )
    {
        WaitForSingleObject( Parms.Event, INFINITE );

        CloseHandle( hThread );
    }

    //
    // After the event is set, the thread has made a copy of the parameter
    // block, so we can get out of here.
    //

    CloseHandle( Parms.Event );

    return 0 ;
}


//+---------------------------------------------------------------------------
//
//  Function:   DoStartShell
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pTerm] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    9-30-94   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
DoStartShell(
    PTERMINAL pTerm
    )
{
    HANDLE           hImp;
    BOOL             StartStatus ;
    HINSTANCE        hInstMPR;
    PFNRESTORENETCON pfnRestoreNetCon;
    PVOID            pNewEnvironment;
    WCHAR            szDesktop[MAX_PATH];
    PWINDOWSTATION   pWS = pTerm->pWinStaWinlogon;
    UINT             ErrorMode ;


    RemoveStatusMessage(FALSE);
    (void) DisplayPreShellLogonMessages(pTerm);

    //
    // If not logging in as Guest, System or Administrator then check for
    // migration of Windows 3.1 configuration inforation.
    //

    if (szAdminName[ 0 ] == TEXT('\0'))
    {
        LoadString(NULL, IDS_ADMIN_ACCOUNT_NAME, szAdminName, ARRAYSIZE(szAdminName));
    }


    if (g_Console) {

        if (!IsUserAGuest(pWS) &&
            _wcsicmp(pWS->UserName, szAdminName)
           )
        {
            Windows31Migration(pTerm);
        }

        //
        // Setup the user's power profile. For Hydra, default power settings for
        // physical console's user take effect.
        //

        StatusMessage(TRUE, 0, IDS_STATUS_POWER_PROFILE);
        SetPowerProfile(pTerm);

    }




    //
    // Further initialize multimedia and play the user's logon sound
    //

    StatusMessage(TRUE, 0, IDS_STATUS_PLAY_LOGON_SOUND);

    {
        BOOL   fBeep;

            hImp = ImpersonateUser(&pWS->UserProcessData, NULL);

            __try { WinmmLogon(g_Console); }
            __except (EXCEPTION_EXECUTE_HANDLER) { NOTHING; }

        if (OpenIniFileUserMapping(pTerm))
        {
            __try
            {
                //
                // Migrate driver settings
                //
                MigrateAllDrivers();

                        MigrateSoundEvents();
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                NOTHING ;
            }

            //
            // Whenever a user logs in, have WINMM.DLL check if there
            // are any sound events within the [SOUNDS] section of
            // CONTROL.INI that haven't been ported into HKCU/AppEvents.
            // If there are, migrate those schemes to their new home.
            // This must be done before the upcoming PlaySound() call,
            // as PlaySound() uses the HKCU/AppEvents schemes-listing
            // to resolve an SND_ALIAS_ID request.
            //

            if (!SystemParametersInfo(SPI_GETBEEP, 0, &fBeep, FALSE)) {
                // Failed to get hold of beep setting.  Should we be
                // noisy or quiet?  We have to choose one value...
                fBeep = TRUE;
            }

            if (fBeep) {
                PlaySoundAsync( pTerm, (LPCTSTR) SND_ALIAS_SYSTEMSTART );

                //(*(g_PlaySound))((LPCSTR)SND_ALIAS_SYSTEMSTART,
                //                NULL,
                //                SND_ALIAS_ID | SND_ASYNC | SND_NODEFAULT);
            }


            CloseIniFileUserMapping(pTerm);
        }

        StopImpersonating(hImp);

    }


    StatusMessage(TRUE, 0, IDS_STATUS_RESTORE_NET);


    //
    // Restore the user's network connections
    //
    if ( pTerm->SafeMode == FALSE )
    {
        if (OpenHKeyCurrentUser(pTerm->pWinStaWinlogon))
        {
            if (OpenIniFileUserMapping(pTerm))
            {
                if (hImp = ImpersonateUser(&pWS->UserProcessData, NULL))
                {
                    if (hInstMPR = LoadLibrary(TEXT("mpr.dll")))
                    {
                        pfnRestoreNetCon = (PFNRESTORENETCON)GetProcAddress (hInstMPR,
                                                       "WNetRestoreConnectionW");

                        if (pfnRestoreNetCon)
                        {
                            WCHAR szUserName[100];

                            if (pWS->UserName) {
                                szUserName[0] = TEXT('\0');
                                GetEnvironmentVariableW (USERNAME_VARIABLE, szUserName, 100);
                                SetEnvironmentVariableW (USERNAME_VARIABLE, pWS->UserName);
                            }

                            pfnRestoreNetCon(NULL, NULL);

                            if (pWS->UserName) {
                                if (szUserName[0] != TEXT('\0'))
                                    SetEnvironmentVariableW (USERNAME_VARIABLE, szUserName);
                                else
                                    SetEnvironmentVariableW (USERNAME_VARIABLE, NULL);
                            }
                        }

                        FreeLibrary (hInstMPR);
                    }

                    StopImpersonating(hImp);
                }

                CloseIniFileUserMapping(pTerm);
            }

            CloseHKeyCurrentUser(pTerm->pWinStaWinlogon);
        }
    }



    //
    // Notify the clients
    //

    WlWalkNotifyList( pTerm, WL_NOTIFY_STARTSHELL );

    RemoveStatusMessage(TRUE);

    WlxSetTimeout(pTerm, 120);

    pNewEnvironment = CopyEnvironment(pWS->UserProcessData.pEnvironment);

    wsprintfW (szDesktop, L"%s\\%s", pWS->lpWinstaName,
               APPLICATION_DESKTOP_NAME);

#if DBG
    if (TEST_FLAG(GinaBreakFlags, BREAK_ACTIVATE))
    {
        DebugLog((DEB_TRACE, "About to call WlxActivateUserShell(%#x, %ws, %ws, %#x)\n",
                        pTerm->Gina.pGinaContext, szDesktop,
                        pWS->LogonScripts, NULL));
        DebugBreak();
    }
#endif

    ErrorMode = SetErrorMode( pTerm->ErrorMode );

    StartStatus = pTerm->Gina.pWlxActivateUserShell(pTerm->Gina.pGinaContext,
                                            szDesktop,
                                            pWS->LogonScripts,
                                            pNewEnvironment) ;

    SetErrorMode( ErrorMode );

    return StartStatus ;
}

//+---------------------------------------------------------------------------
//
//  Function:   SuspendComputer
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [bHibernate] --
//              [bDisableWakeupEvents]  --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    6-19-97   Eric Flo   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

typedef struct _WLPOWERSTATEINFO
{
    POWER_ACTION  pa;
    ULONG         Flags;
} WLPOWERSTATEINFO, *PWLPOWERSTATEINFO;

VOID SleepThread (PWLPOWERSTATEINFO pStateInfo)
{
    NTSTATUS ntsRetVal;

    EnablePrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE);

    ntsRetVal = NtInitiatePowerAction(pStateInfo->pa, PowerSystemSleeping1, pStateInfo->Flags, FALSE);


    EnablePrivilege(SE_SHUTDOWN_PRIVILEGE, FALSE);

    if (ntsRetVal != STATUS_SUCCESS) {
        DebugLog((DEB_ERROR, "NtInitiatePowerAction, failed: 0x%08X\n", ntsRetVal));
    }

    LocalFree (pStateInfo);
}


VOID SuspendComputer (BOOL bHibernate, BOOL bDisableWakeupEvents)
{
    PWLPOWERSTATEINFO pStateInfo;
    HANDLE hThread;
    DWORD dwID;


    pStateInfo = LocalAlloc (LPTR, sizeof(WLPOWERSTATEINFO));

    if (!pStateInfo)
        return;

    if (bHibernate) {
        pStateInfo->pa = PowerActionHibernate;
    } else {
        pStateInfo->pa = PowerActionSleep;
    }

    pStateInfo->Flags = POWER_ACTION_QUERY_ALLOWED | POWER_ACTION_UI_ALLOWED;

    if (GetKeyState(VK_CONTROL) < 0) {
        pStateInfo->Flags |= POWER_ACTION_CRITICAL;
    }

    if (bDisableWakeupEvents) {
        pStateInfo->Flags |= POWER_ACTION_DISABLE_WAKES;
    }

    hThread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) SleepThread,
                            (LPVOID)pStateInfo, 0, &dwID);

    if (hThread)
        CloseHandle (hThread);
    else
        LocalFree (pStateInfo);
}

//+---------------------------------------------------------------------------
//
//  Function:   HandleLoggedOn
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pTerm] --
//              [SasType]  --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    9-30-94   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HandleLoggedOn(
    PTERMINAL pTerm,
    DWORD     SasType)
{
    DWORD                   Result;
    int                     Flags;
    int                     MprRet;
    int                     LogoffResult;
    PWINDOWSTATION          pWS = pTerm->pWinStaWinlogon;

    SetActiveDesktop(pTerm, Desktop_Winlogon);

    WlxSetTimeout(pTerm, 120);

#if DBG
    if (TEST_FLAG(GinaBreakFlags, BREAK_LOGGEDON))
    {
        DebugLog((DEB_TRACE, "About to call WlxLoggedOnSAS( %#x, %d, NULL)\n",
                        pTerm->Gina.pGinaContext, pTerm->SasType));
        DebugBreak();
    }
#endif

    //
    // Send a signal to the communications channel that
    // we are going into Secure-Attention-Sequence mode.
    //
    if( !g_Console ) {
        gpfnWinStationSetInformation(
            SERVERNAME_CURRENT,
            LOGONID_CURRENT,
            WinStationSecureDesktopEnter,
            NULL,
            0);
    }

    Result = pTerm->Gina.pWlxLoggedOnSAS(pTerm->Gina.pGinaContext,
                                        SasType,
                                        NULL );

    WlxSetTimeout(pTerm, TIMEOUT_NONE);

    //
    // Signal the communications channel that we are no longer
    // in SAS mode.
    //
    if( !g_Console ) {
        gpfnWinStationSetInformation(
            SERVERNAME_CURRENT,
            LOGONID_CURRENT,
            WinStationSecureDesktopExit,
            NULL,
            0);
    }

    DebugLog((DEB_TRACE, "WlxLoggedOnSAS returned %d, %s\n", Result, WlxName(Result)));
    pTerm->LastGinaRet = Result;

    //
    // if a new SAS has come in while we were processing that one, repost it and
    // pick it up in LoggedOnDlgProc.
    //

    if ( pTerm->SasType != SasType )
    {
        DebugLog((DEB_TRACE, "New SAS (%d: %s) came in while handling (%d: %s).  Routing it now\n",
            pTerm->SasType, SASName(pTerm->SasType),
            SasType, SASName(SasType) ));

        SASRouter( pTerm, pTerm->SasType );

    }

    if (Result == WLX_SAS_ACTION_LOCK_WKSTA)
    {
        return (DoLockWksta(pTerm, FALSE));
    }

    if ((Result == WLX_SAS_ACTION_TASKLIST)        ||
        (Result == WLX_SAS_ACTION_NONE)            ||
        (Result == WLX_SAS_ACTION_SHUTDOWN_SLEEP)  ||
        (Result == WLX_SAS_ACTION_SHUTDOWN_SLEEP2) ||
        (Result == WLX_SAS_ACTION_SHUTDOWN_HIBERNATE))
    {
        if ((pTerm->WinlogonState != Winsta_WaitForLogoff) &&
            (pTerm->WinlogonState != Winsta_WaitForShutdown) )
        {
            SetActiveDesktop(pTerm, Desktop_Application);


        }

        if (Result == WLX_SAS_ACTION_TASKLIST)
        {
            WCHAR szTaskMgr[] = L"taskmgr.exe";
            WCHAR szDesktop[MAX_PATH];

            wsprintfW (szDesktop, L"%s\\%s", pWS->lpWinstaName,
                       APPLICATION_DESKTOP_NAME);

            DebugLog((DEB_TRACE, "Starting taskmgr.exe.\n"));

            if (pTerm->UserLoggedOn ) {
                StartApplication(pTerm,
                                 szDesktop,
                                 pWS->UserProcessData.pEnvironment,
                                 szTaskMgr);
            }
        }
        else if (Result == WLX_SAS_ACTION_SHUTDOWN_SLEEP)
        {
            SuspendComputer (FALSE, FALSE);
        }
        else if (Result == WLX_SAS_ACTION_SHUTDOWN_SLEEP2)
        {
            SuspendComputer (FALSE, TRUE);
        }
        else if (Result == WLX_SAS_ACTION_SHUTDOWN_HIBERNATE)
        {
            SuspendComputer (TRUE, FALSE);
        }


        TickleMessenger();

        return(Result);

    }


    switch (Result)
    {
        case WLX_SAS_ACTION_LOGOFF:
            Flags = EWX_LOGOFF;
            break;

        case WLX_SAS_ACTION_FORCE_LOGOFF:
            Flags = EWX_LOGOFF | EWX_FORCE;
            break;

        case WLX_SAS_ACTION_SHUTDOWN:
            Flags = EWX_LOGOFF | EWX_WINLOGON_OLD_SHUTDOWN;
            break;

        case WLX_SAS_ACTION_SHUTDOWN_REBOOT:
            Flags = EWX_LOGOFF | EWX_WINLOGON_OLD_SHUTDOWN | EWX_WINLOGON_OLD_REBOOT;
            break;

        case WLX_SAS_ACTION_SHUTDOWN_POWER_OFF:
            Flags = EWX_LOGOFF | EWX_WINLOGON_OLD_SHUTDOWN | EWX_WINLOGON_OLD_POWEROFF;
            break;

        default:
            DebugLog((DEB_ERROR, "Incorrect result (%d) from WlxLoggedOnSAS\n", Result));
            return(0);
    }


    LogoffResult = InitiateLogoff(pTerm, Flags);
    if (LogoffResult == DLG_FAILURE)
    {
        return(WLX_SAS_ACTION_NONE);
    }

    return(Result);

}



//+---------------------------------------------------------------------------
//
//  Function:   DoLockWksta
//
//  Synopsis:
//
//  Arguments:  [pTerm] --
//
//  History:    9-16-94   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
int
DoLockWksta(
    PTERMINAL   pTerm,
    BOOL        ScreenSaverInvoked)
{
    int             Result;
    WinstaState     PriorState ;
    PWINDOWSTATION  pWS = pTerm->pWinStaWinlogon;

    PriorState = pTerm->WinlogonState ;

    pTerm->WinlogonState = Winsta_Locked;
    DebugLog((DEB_TRACE_STATE, "DoLockWksta: Setting state to %s\n", GetState(Winsta_Locked)));

    LockWindowStation(pWS->hwinsta);

    do
    {

        pTerm->WinlogonState = Winsta_Locked_Display;

        WlWalkNotifyList( pTerm, WL_NOTIFY_LOCK );

        DebugLog((DEB_TRACE_STATE, "DoLockWksta: Setting state to %s\n",
                GetState(Winsta_Locked_Display)));
#if DBG
        if (TEST_FLAG(GinaBreakFlags, BREAK_DISPLAYLOCKED))
        {
            DebugLog((DEB_TRACE, "About to call WlxDisplayLockedNotice( %#x )\n",
                        pTerm->Gina.pGinaContext ));
            DebugBreak();
        }
#endif
        //
        // No input timeout
        //

        WlxSetTimeout(pTerm, TIMEOUT_NONE);

        pTerm->Gina.pWlxDisplayLockedNotice( pTerm->Gina.pGinaContext );

        DebugLog((DEB_TRACE, "Out of DisplayLockedNotice, SAS = %s\n",
                        SASName(pTerm->SasType)));

        if (pTerm->SasType == WLX_SAS_TYPE_SCRNSVR_TIMEOUT)
        {
            //
            // If we were invoked as part of a secure screen saver,
            // then this timeout means that we should return to it
            // and let it cycle.
            //
            if (ScreenSaverInvoked)
            {
                return(WLX_SAS_ACTION_NONE);
            }

            //
            // Invoke the screen saver:
            //

            if (DoScreenSaver(pTerm, TRUE) >=  0)
            {

                //
                // Jump right back to the top.
                //

                Result = WLX_SAS_ACTION_NONE;

                continue;
            }

            //
            // A return of -1 indicates that some other SAS occurred,
            // e.g. a Logoff, or a GINA specific SAS.  Fall through to
            // the default handling.
            //

        }

        //
        // An unfortunate label, but things get awfully convoluted switching
        // between the screen saver and the locked state.  The screen saver
        // has stopped due to a SAS, and here is where we figure out what to
        // do.  This is jumped to from below, if the WkstaLocked dialog
        // ended with a screen saver timeout.
        //


ResetLockCall:

        if (pTerm->SasType == WLX_SAS_TYPE_USER_LOGOFF)
        {
            if (pTerm->LogoffFlags & EWX_WINLOGON_API_SHUTDOWN)
            {
                pTerm->WinlogonState = Winsta_Shutdown ;
            }
            else 
            {
                pTerm->WinlogonState = Winsta_WaitForLogoff ;
            }
            DebugLog(( DEB_TRACE_STATE, "ResetLockCall:  Change state to %s\n",
                       GetState( pTerm->WinlogonState ) ));

            return(WLX_SAS_ACTION_LOGOFF);
        }


#if DBG
        if (TEST_FLAG(GinaBreakFlags, BREAK_WKSTALOCKED))
        {
            DebugLog((DEB_TRACE, "About to call WlxWkstaLockedSAS( %#x, %d )\n",
                        pTerm->Gina.pGinaContext, pTerm->SasType ));
            DebugBreak();
        }
#endif
        WlxSetTimeout(pTerm, 120);


        //
        // Send a signal to the communications channel that
        // we are going into Secure-Attention-Sequence mode.
        //
        if( !g_Console ) {
            gpfnWinStationSetInformation(
                SERVERNAME_CURRENT,
                LOGONID_CURRENT,
                WinStationSecureDesktopEnter,
                NULL,
                0);
        }


        Result = pTerm->Gina.pWlxWkstaLockedSAS( pTerm->Gina.pGinaContext, pTerm->SasType );

        WlxSetTimeout(pTerm, TIMEOUT_NONE);

        //
        // Signal the communications channel that we are no longer
        // in SAS mode.
        //
        if( !g_Console ) {
            gpfnWinStationSetInformation(
                SERVERNAME_CURRENT,
                LOGONID_CURRENT,
                WinStationSecureDesktopExit,
                NULL,
                0);
        }

        DebugLog((DEB_TRACE, "WlxWkstaLockedSAS returned %d, %s\n", Result, WlxName(Result)));

        pTerm->LastGinaRet = Result;

        if ( (Result == WLX_SAS_ACTION_NONE) &&
             (pTerm->SasType == WLX_SAS_TYPE_SCRNSVR_TIMEOUT ) )
        {
            //
            // The GINA was interrupted by a screen saver timeout.  If
            // we were invoked by a screen saver, we should return immediately,
            // otherwise, run the screen saver.  Same as before when the display
            // call timed out.
            //

            if (ScreenSaverInvoked)
            {
                return(WLX_SAS_ACTION_NONE);
            }

            //
            // A return of -1 indicates that the screen saver terminated
            // due to some other SAS.  Jump back up to the point where we
            // handle that, so that we have one point where we do things
            // correctly.
            //

            if ( DoScreenSaver( pTerm, TRUE ) < 0 )
            {
                goto ResetLockCall;

            }

            //
            // Otherwise, fall through, and loop again.
            //


        }

    } while (Result == WLX_SAS_ACTION_NONE);


    if (Result == WLX_SAS_ACTION_FORCE_LOGOFF)
    {
        InitiateLogoff(pTerm, EWX_LOGOFF | EWX_FORCE);
    }
    else
    {
        //
        // Back to the application:
        //

        WlWalkNotifyList( pTerm, WL_NOTIFY_UNLOCK );

        DebugLog(( DEB_TRACE_STATE, "Lock: Resetting to prior state %d %s\n",
                    PriorState, GetState( PriorState ) ));

        pTerm->WinlogonState = PriorState ;

        SetActiveDesktop( pTerm, Desktop_Application );

        TickleMessenger();
    }

    return(Result);
}


//+---------------------------------------------------------------------------
//
//  Function:   DoScreenSaver
//
//  Synopsis:   Starts up the screen saver
//
//  Effects:
//
//  Arguments:  [pTerm] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    10-13-94   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
int
DoScreenSaver(
    PTERMINAL   pTerm,
    BOOL        WkstaLocked)
{
    int     Result;
    BOOL    FastUnlock;
    WinstaState PriorState ;

    PriorState = pTerm->WinlogonState ;

    //
    // WkstaLocked indicates that we were called by the DoLockWksta
    // path.  This means that we should not recursively call them, since
    // there is no stopping case and we could chew up a lot of stack.
    // So, we loop here, but we can break out if RunScreenSaver doesn't
    // return Wksta locked.
    //

    //
    // FastUnlock determines if we allow a grace period or not.  If the
    // wksta is locked coming in, then it is not allowed at all.  If it is
    // not locked on entry, then we allow it once.
    //

    FastUnlock = !WkstaLocked;

    do
    {

        Result = RunScreenSaver(pTerm, FALSE, FastUnlock);

        FastUnlock = FALSE;

        if (Result == WLX_SAS_ACTION_LOCK_WKSTA)
        {
            //
            // Ok, it's a secure screen saver.  If we are already locked,
            // break and return
            //
            if (WkstaLocked)
            {
                break;
            }

            //
            // Ok, it's not.  Invoke the lock code ourselves, but tell it
            // that it's being called from the screen saver path.  The desktop
            // switch is a no-op if we have already switched.
            //

            SetActiveDesktop( pTerm, Desktop_Winlogon );

            Result = DoLockWksta(pTerm, TRUE);

            if ( Result == WLX_SAS_ACTION_UNLOCK_WKSTA )
            {
                //
                // Reset the state here.  RunScreenSaver has set the state
                // to locked, but the DoLockWksta won't be able to reset it,
                // since on its entry, it is already locked.
                //

                DebugLog(( DEB_TRACE_STATE, "DoScreenSaver: Resetting state to %d %s\n",
                            PriorState, GetState( PriorState ) ));

                pTerm->WinlogonState = PriorState ;
            }

        }
        else
        {
            break;
        }

        //
        // Loop clause:  we only get here if we have invoked DoLockWksta.
        // Loop only if it return WLX_SAS_ACTION_NONE, not unlock or force
        // logoff.
        //

    } while (Result == WLX_SAS_ACTION_NONE);

    if ( (Result == -1) || (Result == WLX_SAS_ACTION_LOGOFF) )
    {
        return( Result );
    }
    else if (Result == WLX_SAS_ACTION_FORCE_LOGOFF)
    {
        return(Result);
    }
    return(0);
}



/***************************************************************************\
* FUNCTION: LogoffWaitDlgProc
*
* PURPOSE:  Processes messages for the forced logoff wait dialog
*
* RETURNS:
*   DLG_FAILURE     - the dialog could not be displayed
*   DLG_INTERRUPTED() - this is a set of possible interruptions (see winlogon.h)
*
* HISTORY:
*
*   05-09-92 Davidc       Created.
*
\***************************************************************************/

INT_PTR
CALLBACK
LogoffWaitDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    switch (message) {

        case WM_INITDIALOG:

            EnableSasMessages(hDlg, (PTERMINAL)lParam);

            SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);

            CentreWindow(hDlg);

            return(TRUE);

    }

    // We didn't process this message
    return FALSE;
}


BOOL
WaitForForceLogoff(
    HWND            hWnd,
    PTERMINAL       pTerm)
{
    int Result;

    do
    {
        SetActiveDesktop( pTerm, Desktop_Winlogon );

        WlxSetTimeout(pTerm, TIMEOUT_NONE);

        Result = WlxDialogBoxParam( pTerm,
                                    g_hInstance,
                                    MAKEINTRESOURCE(IDD_FORCED_LOGOFF_WAIT),
                                    hWnd,
                                    LogoffWaitDlgProc,
                                    (LPARAM) pTerm);

    } while ( (Result == WLX_DLG_INPUT_TIMEOUT) ||
              (Result == WLX_DLG_SCREEN_SAVER_TIMEOUT) );


    return(TRUE);

}

/***************************************************************************\
* FUNCTION: LoggedOnDlgProc
*
* PURPOSE:  Processes messages for the logged-on control dialog
*
* DIALOG RETURNS:
*
*   DLG_FAILURE -       Couldn't bring up the dialog
*   DLG_LOGOFF() -      The user logged off
*
* NOTES:
*
* On entry, it assumed that the winlogon desktop is switched to and the
* desktop lock is held. This same state exists on exit.
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

INT_PTR WINAPI
LoggedonDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PTERMINAL pTerm = (PTERMINAL)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    int Result;
    DWORD GinaState ;


    switch (message)
    {

        case WM_INITDIALOG:
            SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
            pTerm = (PTERMINAL)lParam;

            if (!LoggedonDlgInit(hDlg)) {
                EndDialog(hDlg, DLG_FAILURE);
                return(TRUE);
            }



            // Send ourselves a message so we can hide ourselves without the
            // dialog code trying to force us to be visible
            PostMessage(hDlg, WM_HIDEOURSELVES, 0, 0);

            //
            // Switch to app desktop and release lock
            //

            SetActiveDesktop(pTerm, Desktop_Application);

            //
            // Tickle the messenger so it will display any queue'd messages.
            // (This call is a kind of NoOp).
            //
            TickleMessenger();



            return(TRUE);

        case WM_HIDEOURSELVES:
            ShowWindow(hDlg, SW_HIDE);

            DropWorkingSet();

            return(TRUE);


        case WLX_WM_SAS:

            //
            // Disable further SAS events until we decide what to do.  If
            // we start another window, they will automagically be forwarded
            // to it.  This lets us call right into the individual cases.
            //

            DisableSasMessages();

            if (wParam == WLX_SAS_TYPE_SCRNSVR_TIMEOUT)
            {
                Result = DoScreenSaver(pTerm, FALSE);

                if ( (Result < 0) || (Result == WLX_SAS_ACTION_LOGOFF) )
                {
                    //
                    // Ugly case:  the screen saver received a SAS event
                    // which has interrupted it.  So, that SAS is now stored
                    // in the globals.  We snag it, stuff it in wParam, and
                    // FALL THROUGH to the rest of this code.
                    //

                    wParam = pTerm->SasType;
                }
                else if (Result == WLX_SAS_ACTION_FORCE_LOGOFF)
                {
                    EndDialog(hDlg, WLX_SAS_ACTION_FORCE_LOGOFF);
                    return(TRUE);

                }
                else
                {
                    EnableSasMessages(hDlg, pTerm);
                    return(TRUE);
                }
            }

            //
            // Ok, more ugly cases.  The user could asynchronously log off
            // while we're in HandleLoggedOn(), in which case we would get
            // the logoff notify in some other dialog, and returned to us
            // here.  But, we also have some logoff cases coming through
            // here if we are waiting for the logoff, so if this is not
            // winsrv telling us it's logged the guy off, ask the gina
            // what to do.
            //

            if ((wParam != WLX_SAS_TYPE_USER_LOGOFF) &&
                (wParam != WLX_SAS_TYPE_TIMEOUT) )
            {
                Result = HandleLoggedOn(pTerm, (DWORD) wParam);
            }
            else
            {
                Result = -1;
            }


            if ((wParam == WLX_SAS_TYPE_USER_LOGOFF ) ||
                (Result == WLX_SAS_ACTION_LOGOFF ) ||
                (Result == WLX_SAS_ACTION_FORCE_LOGOFF ) )
            {
                //
                // If we were shut down by the remote guy, handle that
                //
                if (pTerm->WinlogonState == Winsta_Shutdown)
                {
                    EndDialog(hDlg, pTerm->LastGinaRet);
                    return(TRUE);
                }
                if ((pTerm->WinlogonState == Winsta_WaitForLogoff) ||
                    (wParam == WLX_SAS_TYPE_USER_LOGOFF) )
                {
                    GinaState = LogoffFlagsToWlxCode( RealFlagsFromStoredFlags( pTerm->LogoffFlags ) );

                    if ( ( IsShutdown(pTerm->LastGinaRet)) ||
                         ( IsShutdown( GinaState ) ) )
                    {
                        pTerm->WinlogonState = Winsta_WaitForShutdown;
                        if ( !IsShutdown( pTerm->LastGinaRet ) )
                        {
                            pTerm->LastGinaRet = GinaState ;
                        }
                    }
                    else
                        pTerm->WinlogonState = Winsta_NoOne;

                    DebugLog((DEB_TRACE_STATE, "LoggedOnDlg: setting state to %s\n",
                                GetState(pTerm->WinlogonState)));

                    EndDialog(hDlg, pTerm->LastGinaRet);
                    return(TRUE);

                }
                else
                {
                    DebugLog((DEB_TRACE_STATE, "LoggedOnDlg:  setting state to WaitForLogoff\n"));
                    pTerm->WinlogonState = Winsta_WaitForLogoff;

                    //
                    // If this is a force-logoff, end now, so that we fall
                    // through to the special dialog (WaitForForceLogoff())
                    //

                    if (Result == WLX_SAS_ACTION_FORCE_LOGOFF)
                    {
                        EndDialog(hDlg, Result);
                        return(TRUE);
                    }
                }
            }
            else
            {
                //
                // Now it is perverse.  If the user logged off *while the
                // options dialog was up*, they will return NONE, expecting
                // us to deal with it correctly.
                //

                if (pTerm->WinlogonState == Winsta_WaitForLogoff)
                {
                    GinaState = LogoffFlagsToWlxCode( RealFlagsFromStoredFlags( pTerm->LogoffFlags ) );

                    if ( ( IsShutdown(pTerm->LastGinaRet)) ||
                         ( IsShutdown( GinaState ) ) )
                    {
                        pTerm->WinlogonState = Winsta_WaitForShutdown;
                        if ( !IsShutdown( pTerm->LastGinaRet )  )
                        {
                            pTerm->LastGinaRet = GinaState ;
                        }
                    }
                    else
                        pTerm->WinlogonState = Winsta_NoOne;

                    DebugLog((DEB_TRACE_STATE, "LoggedOnDlg: setting state to %s\n",
                                GetState(pTerm->WinlogonState)));

                    EndDialog(hDlg, pTerm->LastGinaRet);
                    return(TRUE);

                }
            }

            EnableSasMessages(hDlg, pTerm);

            DropWorkingSet();

            return(TRUE);
    }

    // We didn't process this message
    return(FALSE);
}


/***************************************************************************\
* FUNCTION: LoggedonDlgInit
*
* PURPOSE:  Handles initialization of logged-on dialog
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL
LoggedonDlgInit(
    HWND    hDlg
    )
{
    PTERMINAL pTerm = (PTERMINAL)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    // Set our size to zero so we we don't appear
    SetWindowPos(hDlg, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE |
                                         SWP_NOREDRAW | SWP_NOZORDER);

    SetMapperFlag(hDlg, MAPPERFLAG_WINLOGON, pTerm);

    return(TRUE);
}



//+---------------------------------------------------------------------------
//
//  Function:   BlockWaitForUserAction
//
//  Synopsis:   Blocks, waiting for the interactive user to do something, or
//              a SAS to come in from the gina.
//
//  Effects:
//
//  Arguments:  [pTerm] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    10-17-94   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


int
BlockWaitForUserAction(PTERMINAL pTerm)
{
    int res;

    g_fAllowStatusUI = FALSE;
    RemoveStatusMessage (TRUE);
    SetActiveDesktop(pTerm, Desktop_Application);
    WlxSetTimeout(pTerm, TIMEOUT_NONE);
    res =   WlxDialogBoxParam(  pTerm, g_hInstance,
                                MAKEINTRESOURCE(IDD_CONTROL),
                                NULL,
                                LoggedonDlgProc,
                                (LPARAM) pTerm) ;
#if DBG
    if (res == -1)
    {
        DebugLog((DEB_ERROR, "Failed to start LoggedOnDlgProc, %d\n", GetLastError()));
    }
#endif
    g_fAllowStatusUI = TRUE;

    return(res);

}

//+---------------------------------------------------------------------------
//
//  Function:   MainLoop
//
//  Synopsis:   Main winlogon loop.
//
//  Arguments:  [pTerm] --
//
//  History:    10-17-94   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void
MainLoop(PTERMINAL   pTerm)
{

#if defined(_WIN64_LOGON)

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    BOOL Result;
    WCHAR InitialCommand[MAX_PATH];
    LUID luidNone = {0, 0};
    NTSTATUS Status;
    HANDLE Token;
    MSG msg;

    //
    // Whack system as current user:
    //

    SetWindowStationUser(pTerm->pWinStaWinlogon->hwinsta, &luidNone, NULL, 0);
    Status = NtOpenProcessToken(NtCurrentProcess(),
                                MAXIMUM_ALLOWED,
                                &Token);

    if (NT_SUCCESS(Status)) {
        UpdatePerUserSystemParameters(Token, TRUE);
    }

    //
    // At this stage, we're mostly set.
    //

    wcscpy(InitialCommand, TEXT("cmd.exe"));
    do {
        ZeroMemory(&si, sizeof(STARTUPINFO));
        si.cb = sizeof(STARTUPINFO);
        si.lpTitle = InitialCommand ;
        si.dwFlags = 0 ;
        si.wShowWindow = SW_SHOW;
        si.lpDesktop = TEXT("Winsta0\\Winlogon");
        Result = CreateProcessW(NULL,
                                InitialCommand,
                                NULL,
                                NULL,
                                FALSE,
                                0,
                                NULL,
                                NULL,
                                &si,
                                &pi);

        if (!Result) {
            KdPrint((" Failed to start initial command\n" ));
            continue;
        }

        CloseHandle(pi.hThread);
        while (MsgWaitForMultipleObjects(1, &pi.hProcess, FALSE, INFINITE, QS_ALLINPUT) == 1) {
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        CloseHandle(pi.hProcess);
    } while (TRUE);

    return;

#else

    DWORD               WlxResult;
    int                 ScreenSaverResult;

    WlxResult = WLX_SAS_ACTION_NONE;

    //
    // Initialize the gina dll
    //

    if (!InitializeGinaDll(pTerm))
    {
        DebugLog((DEB_ERROR, "InitializeGinaDll failed\n"));
        return;
    }


    //
    // Wait for various services to start
    //

    if (g_Console) {
        WaitForServices(pTerm);
    }

    //
    // Start profile mapping APIs
    //

    if (g_Console) {
        InitializeProfileMappingApi();
    }

    //
    // Do Net Access Wizard stuff
    //

    if ( CheckForNetAccessWizard(pTerm) )
    {
        // Reboot is required
        pTerm->LastGinaRet = (DWORD) WLX_SAS_ACTION_SHUTDOWN_REBOOT;
        return;
    }


    //
    // Send the Startup notification
    //

    WlWalkNotifyList( pTerm, WL_NOTIFY_STARTUP );


    //
    // Initialize the sas type to something useful:
    //

    pTerm->SasType = WLX_SAS_TYPE_CTRL_ALT_DEL;

    //
    // So long as action is none, loop here:
    //

    while (WlxResult == WLX_SAS_ACTION_NONE)
    {

        DealWithAutochkLogs();
        QueryVerboseStatus();
        RemoveStatusMessage(TRUE);

        //
        // If no one is logged on, switch to the display state, and call
        // the gina to display a message.  This is structured this way so
        // that a gina can call us immediately during Initialize, and we
        // can fall into the loop in the correct state.
        //
        if (pTerm->WinlogonState == Winsta_NoOne)
        {
            pTerm->WinlogonState = Winsta_NoOne_Display;
            DebugLog((DEB_TRACE_STATE, "Setting state to %s\n",
                    GetState(Winsta_NoOne_Display)));
#if DBG
            if (TEST_FLAG(GinaBreakFlags, BREAK_DISPLAY))
            {
                DebugLog((DEB_TRACE, "About to call WlxDisplaySASNotice(%x)\n",
                            pTerm->Gina.pGinaContext));
                DebugBreak();
            }
#endif
            g_fAllowStatusUI = FALSE;

            WlxSetTimeout(pTerm, TIMEOUT_NONE);

            pTerm->Gina.pWlxDisplaySASNotice(pTerm->Gina.pGinaContext);

            g_fAllowStatusUI = TRUE;

            //
            // If we got a user logoff notify, that means that WE HAVE ALREADY
            // SHUT DOWN.  A remote shutdown has taken place, and it has been
            // started by sysshut.c.
            //
            if (pTerm->SasType == WLX_SAS_TYPE_USER_LOGOFF)
            {
                //
                // We are *done*
                //

                DebugLog(( DEB_TRACE_STATE, "Received Logoff, setting state to %s\n",
                                    GetState(Winsta_Shutdown) ));

                break;
            }

            //
            // If we got a time out,
            //
            if (pTerm->SasType == WLX_SAS_TYPE_SCRNSVR_TIMEOUT)
            {
                //
                // run the screen saver
                //

                ScreenSaverResult = DoScreenSaver( pTerm, FALSE );

                if (ScreenSaverResult < 0)
                {
                    //
                    // This means that a SAS other than activity cancelled
                    // the screen saver, such as a GINA specific one.
                    // In this case, drop on through to the logonattempt,
                    // since the current sas is in pTerm.
                    //

                    NOTHING ;

                }
                else
                {
                    if ( (ScreenSaverResult == 0) &&
                         (pTerm->SasType == WLX_SAS_TYPE_USER_LOGOFF)  )
                    {
                        //
                        // Shutdown during the screen saver.
                        //

                        DebugLog(( DEB_TRACE_STATE, "Received Logoff during screensaver, setting state to %s\n",
                                    GetState(Winsta_Shutdown) ));

                        pTerm->WinlogonState = Winsta_Shutdown;

                        break;

                    }
                    else 
                    {
                        if ( ScreenSaverResult == WLX_SAS_ACTION_LOGOFF )
                        {
                            //
                            // Remote shutdown has started.
                            //

                            break;
                        }
                    }

                    //
                    // And start the loop over again.
                    //
                    WlxResult = WLX_SAS_ACTION_NONE;

                    //
                    // Remember, we're in Winsta_NoOne_Display right now, so we
                    // reset this so that we'll drop back into this at the top of
                    // the loop.
                    //

                    DebugLog((DEB_TRACE_STATE, "Resetting to %s\n",
                                GetState(Winsta_NoOne) ));

                    pTerm->WinlogonState = Winsta_NoOne;

                    continue;
                }

                //
                // If we got a user logoff notify, that means that WE HAVE ALREADY
                // SHUT DOWN.  A remote shutdown has taken place, and it has been
                // started by sysshut.c.
                //
                if (pTerm->SasType == WLX_SAS_TYPE_USER_LOGOFF)
                {
                    //
                    // We are *done*
                    //
                    DebugLog(( DEB_TRACE_STATE, "Received Logoff during no-one screensaver, setting state to %s\n",
                                    GetState(Winsta_Shutdown) ));

                    pTerm->WinlogonState = Winsta_Shutdown;
                    break;
                }
            }

        }

        WlxResult = LogonAttempt(pTerm);


        if (!g_Console) {
           if ( ((WlxResult == WLX_SAS_ACTION_RECONNECTED) || (WlxResult == WLX_SAS_ACTION_LOGOFF)) ) {
              //
              // LogonAttempt returns WLX_SAS_ACTION_RECONNECTED in the case that we
              // reconnected to an existing session.  In this case, we simply want
              // to exit the while loop and then exit WinLogon.
              // We also want to exit winlogon when a user hits Cancel on the Logon dialog.
              // In this case LogonAttempt will return WLX_SAS_ACTION_LOGOFF
              //

               break;

           } else if ((WlxResult == WLX_SAS_ACTION_NONE) && (pTerm->SasType == WLX_SAS_TYPE_CTRL_ALT_DEL) ) {

               //
               // If the Logon dialog is idle for 120 seconds (LOGON_TIMEOUT)
               // exit the session.
               //
               DbgPrint("Timed out!!. Session %d will be terminated\n",NtCurrentPeb()->SessionId);
               break;

           }
        }


        //
        // Another case in which we get wlx_logoff from logonattempt is when profiles failed
        // to load. In this case we need to close any of the RAS connections, if applicable.
        // Since we are setting the state to to none below, it is not going to go through to the
        // logoff(). Calling DeleteRasConnection below.
        
        if ( WlxResult == WLX_SAS_ACTION_LOGOFF )
        {
            DeleteRasConnections(pTerm);        
            WlxResult = WLX_SAS_ACTION_NONE ;
        }


        if (WlxResult == WLX_SAS_ACTION_NONE)
        {
            //
            // If we got a user logoff notify, that means that WE HAVE ALREADY
            // SHUT DOWN.  A remote shutdown has taken place, and it has been
            // started by sysshut.c.
            //
            if (pTerm->SasType == WLX_SAS_TYPE_USER_LOGOFF)
            {
                //
                // We are *done*
                //

                DebugLog(( DEB_TRACE_STATE, "Got logoff during logon, setting to %s\n",
                            GetState(Winsta_Shutdown) ));

                pTerm->WinlogonState = Winsta_Shutdown;

                break;
            }
            //
            // If we got a time out (meaning a screensaver timeout
            // occurred during the logon prompt, then the prompt should be dead,
            // but we'll hit here
            //
            if (pTerm->SasType == WLX_SAS_TYPE_SCRNSVR_TIMEOUT)
            {
                //
                // run the screen saver
                //
                ScreenSaverResult = DoScreenSaver(pTerm, FALSE);

                if (ScreenSaverResult < 0)
                {
                    //
                    // This means that a SAS other than activity cancelled
                    // the screen saver, such as a GINA specific one.
                    // In this case, drop on through to the logonattempt,
                    // since the current sas is in pTerm.
                    //

                    NOTHING ;

                }
                else
                {
                    if ( (ScreenSaverResult == 0) &&
                         (pTerm->SasType == WLX_SAS_TYPE_USER_LOGOFF)  )
                    {
                        //
                        // Shutdown during the screen saver.
                        //

                        DebugLog(( DEB_TRACE_STATE, "Received Logoff during screensaver, setting state to %s\n",
                                    GetState(Winsta_Shutdown) ));

                        pTerm->WinlogonState = Winsta_Shutdown;

                        break;

                    }

                    if ( ScreenSaverResult == WLX_SAS_ACTION_LOGOFF )
                    {
                        break;
                    }

                }

                //
                // We're already at WlxResult == NONE, and State == NoOne,
                // so we can just continue
                //

            }

            //
            // Make sure that we're back to NoOne:
            //

            pTerm->WinlogonState = Winsta_NoOne;

            continue;
        }

        if (IsSuspend(WlxResult))
        {
            SuspendComputer((WlxResult == WLX_SAS_ACTION_SHUTDOWN_HIBERNATE) ? TRUE : FALSE,
                            (WlxResult == WLX_SAS_ACTION_SHUTDOWN_SLEEP2) ? TRUE : FALSE);

            pTerm->WinlogonState = Winsta_NoOne;
            WlxResult = WLX_SAS_ACTION_NONE;
            continue;
        }

        if (IsShutdown(WlxResult))
        {
            pTerm->LastGinaRet = WlxResult;
            DebugLog((DEB_TRACE_STATE, "Setting state to %d (%s)\n",
                    Winsta_WaitForShutdown, GetState(Winsta_WaitForShutdown)));
            pTerm->WinlogonState = Winsta_WaitForShutdown;
            break;
        }

        //
        // Because profile or something else could have gone wrong, the gina
        // could have returned LOGON, but it was changed to LOGOFF in
        // LogonAttempt().  In that case, we don't try and start the shell,
        // we just go straight to logoff processing.
        //

        if (WlxResult == WLX_SAS_ACTION_LOGON)
        {
            
#ifndef _WIN64
            
            //
            // Since someone logged on notify the nddeagnt thread that
            // it can impersonate the logged on user
            //

            if (g_hwndAppDesktopThread != NULL) {
                PostMessage(g_hwndAppDesktopThread,
                            WM_USERCHANGED,
                            (WPARAM)g_hwndAppDesktopThread,
                            (LPARAM)pTerm->pWinStaWinlogon->hToken);
            }
#endif // _WIN64

            WlWalkNotifyList( pTerm, WL_NOTIFY_LOGON );

            if (DoStartShell(pTerm))
            {
                DebugLog(( DEB_TRACE_STATE, "Setting state to LoggedOnUser\n" ));

                pTerm->WinlogonState = Winsta_LoggedOnUser ;

                WlWalkNotifyList( pTerm, WL_NOTIFY_POSTSHELL );

                //
                // now wait for the user to take an action (like generating the SAS)
                //

                WlxResult = BlockWaitForUserAction(pTerm);

                if (WlxResult == WLX_SAS_ACTION_FORCE_LOGOFF)
                {
                    WaitForForceLogoff(NULL, pTerm);

                    WlxResult = WLX_SAS_ACTION_LOGOFF;

                }
            }
            else
            {
                WlxResult = WLX_SAS_ACTION_LOGOFF;
            }
        }

        EnableSasMessages(NULL, pTerm);

        SetActiveDesktop(pTerm, Desktop_Winlogon);

        if ( ( pTerm->WinlogonState == Winsta_Shutdown ) &&
             ( !g_Console ) )
        {
            DebugLog(( DEB_ERROR, "Winstation already shutdown?\n" ));
            DebugBreak();
            break;
        }


        Logoff(pTerm, WlxResult);

        if (WlxResult == WLX_SAS_ACTION_LOGOFF)
        {
            pTerm->WinlogonState = Winsta_NoOne;
            DebugLog((DEB_TRACE, "WlxResult was logoff, so beginning loop again\n"));
            DebugLog((DEB_TRACE_STATE, "State set to %s\n", GetState(Winsta_NoOne)));
            WlxResult = WLX_SAS_ACTION_NONE;
        }

        //
        // Notify the GINA that the user is logged off
        //

#if DBG
        if (TEST_FLAG(GinaBreakFlags, BREAK_LOGOFF))
        {
            DebugLog((DEB_TRACE, "About to call WlxLogoff(%x)\n",
                     pTerm->Gina.pGinaContext));
            DebugBreak();
        }
#endif
        pTerm->Gina.pWlxLogoff(pTerm->Gina.pGinaContext);

        //
        // For non-Console WinStations we exit WinLogon after
        // one logon/logoff iteration.
        //
        if ( !g_Console )  {

            break;

        }

#ifndef _WIN64

        //
        // Don't start the NetDDE thread on sessions
        //
        if (g_Console) {
            //
            // start the application thread.  Note: no win64 support
            // for NetDDE (hooray!)
            //

            StartAppDesktopThread(pTerm);
        }
#endif

        //
        // Toggle the winsta lock on and off.  This clears the openlock, and
        // sets the switchlock, allowing services to start, but no one can
        // switch the active desktop.
        //

        UnlockWindowStation(pTerm->pWinStaWinlogon->hwinsta);
        LockWindowStation(pTerm->pWinStaWinlogon->hwinsta);

#if DBG
        if ((WlxResult == WLX_SAS_ACTION_NONE) ||
            (WlxResult == WLX_SAS_ACTION_LOGON) ||
            (WlxResult == WLX_SAS_ACTION_SHUTDOWN) ||
            (WlxResult == WLX_SAS_ACTION_SHUTDOWN_REBOOT) ||
            (WlxResult == WLX_SAS_ACTION_SHUTDOWN_POWER_OFF) ||
            (WlxResult == WLX_SAS_ACTION_SHUTDOWN_SLEEP) ||
            (WlxResult == WLX_SAS_ACTION_SHUTDOWN_SLEEP2) ||
            (WlxResult == WLX_SAS_ACTION_SHUTDOWN_HIBERNATE) )
        {
            continue;
        }

        DebugLog((DEB_TRACE, "WlxResult not acceptible value: %d\n", WlxResult));
        DebugLog((DEB_TRACE, "Resetting to WLX_SAS_ACTION_NONE\n"));
        WlxResult = WLX_SAS_ACTION_NONE;
#endif

    }

#endif

}


//+---------------------------------------------------------------------------
//
//  Function:   LogoffFlagsToWlxCode
//
//  Synopsis:   Translates a winsrv USER_LOGOFF message flags to a
//              wlx return code.
//
//  Arguments:  [Flags] --
//
//  History:    10-17-94   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
LogoffFlagsToWlxCode(DWORD Flags)
{
    if (Flags & EWX_POWEROFF)
    {
        return(WLX_SAS_ACTION_SHUTDOWN_POWER_OFF);
    }
    if (Flags & EWX_REBOOT)
    {
        return(WLX_SAS_ACTION_SHUTDOWN_REBOOT);
    }
    if (Flags & EWX_SHUTDOWN)
    {
        return(WLX_SAS_ACTION_SHUTDOWN);
    }

    return(WLX_SAS_ACTION_LOGOFF);

}


//+---------------------------------------------------------------------------
//
//  Function:   WinsrvNotify
//
//  Synopsis:   Handles when winsrv talks to us
//
//  Arguments:  [pTerm] --
//              [SasType]  --
//
//  Algorithm:
//
//  History:    10-17-94   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void
WinsrvNotify(
    PTERMINAL   pTerm,
    DWORD       SasType)
{
    DWORD       RealFlags;
    DWORD       LogoffResult;
    WinstaState PriorState;
    //
    // If the caller isn't system, and no-one is logged on, discard this message.
    //

    if ( ( (pTerm->LogoffFlags & (EWX_SYSTEM_CALLER)) == 0 ) && 
         ( pTerm->UserLoggedOn == FALSE ) )
    {
        DebugLog((DEB_TRACE, "Discarding notice from winsrv!\n"));
        return;
    }

    //
    // If the caller isn't system, and the workstation is locked, and force
    // isn't set, reject it:
    //

    if ( !(pTerm->LogoffFlags & EWX_WINLOGON_CALLER) &&
         ( ( pTerm->WinlogonState == Winsta_Locked_Display ) ||
           ( pTerm->WinlogonState == Winsta_Locked ) ||
           ( pTerm->WinlogonState == Winsta_Locked_SAS ) ) &&
         !(pTerm->LogoffFlags & EWX_FORCE ) )
    {
        DebugLog(( DEB_TRACE, "Discarding exit request 'cause we're locked\n"));
        return;
    }

    //
    // If this indicates that winlogon initiated this message (by calling
    // InitiateLogoff() somewhere else, or we're in a wait state, then pass
    // the message along.  This is what will kill our LoggedOnDlg.
    //

    if ((pTerm->LogoffFlags & EWX_WINLOGON_CALLER) ||
        (pTerm->WinlogonState == Winsta_WaitForLogoff) ||
        (pTerm->WinlogonState == Winsta_WaitForShutdown) )
    {
        SASRouter(pTerm, SasType);
        RealFlags = RealFlagsFromStoredFlags(pTerm->LogoffFlags);
        pTerm->LastGinaRet = LogoffFlagsToWlxCode(RealFlags);

        ExitWindowsInProgress = FALSE ;

        return;
    }

    //
    // Well, this means that the user has called ExitWindowsEx(), and winsrv
    // has passed the ball to us.  We have to turn around and ask the gina if
    // logoff is ok.  This is convenient for some security architectures.  I
    // guess.
    //

#if DBG
    if (TEST_FLAG(GinaBreakFlags, BREAK_ISLOGOFFOK))
    {
        DebugLog((DEB_TRACE, "About to call WlxIsLogoffOk(%#x)\n",
                 pTerm->Gina.pGinaContext));
        DebugBreak();
    }
#endif
    if (!pTerm->Gina.pWlxIsLogoffOk(pTerm->Gina.pGinaContext))
    {
        DebugLog((DEB_TRACE, "Gina said no logoff...\n"));
        return;
    }


    //
    // Well, if we're not in a wait state, then initiate logoff and possibly
    // shutdown.  Convoluted, right?
    //

    if ((pTerm->WinlogonState != Winsta_WaitForLogoff) &&
        (pTerm->WinlogonState != Winsta_WaitForShutdown) )
    {
        PriorState = pTerm->WinlogonState;
        pTerm->WinlogonState = Winsta_WaitForLogoff;
        DebugLog((DEB_TRACE_STATE, "WinsrvNotify:  Setting state to %s\n",
            GetState(Winsta_WaitForLogoff)));

        pTerm->LastGinaRet = LogoffFlagsToWlxCode(pTerm->LogoffFlags);
        DebugLog((DEB_TRACE, "Setting lastginaret to %s\n",
            WlxName(pTerm->LastGinaRet)));

        LogoffResult = InitiateLogoff(  pTerm,
                                (pTerm->LogoffFlags & EWX_FORCE) |
                                StoredFlagsFromRealFlags(pTerm->LogoffFlags)
                                 );

        if (LogoffResult == DLG_FAILURE)
        {
            DebugLog((DEB_TRACE, "Logoff refused, resetting\n"));
            pTerm->WinlogonState = PriorState;
            DebugLog((DEB_TRACE_STATE, "WinsrvNotify:  resetting state back to %s\n",
                        GetState(PriorState)));
        }
    }

}


int MultiUserLogonAttempt(
        PTERMINAL pTerm,
        PWLX_MPR_NOTIFY_INFO pMprInfo,
        HANDLE hToken)
{
    int WlxResult;
    WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    ULONG Length;
    PWINSTATIONCLIENTDATA pClientData;
    PBYTE pInfo, pInfo1;
    ULONG SizeWsCD, Size1, Size2, Size3, TotalSize;
    PDOMAIN_CONTROLLER_INFO DcInfo = NULL ;
    DWORD Error;

    Length = MAX_COMPUTERNAME_LENGTH + 1;
    if (!GetComputerNameW(ComputerName, &Length)) {
        ComputerName[0] = L'\0';
    }

    //
    // For console Invalidate the userconfig data for every logon attempt
    //
    if (g_Console) {
       pTerm->MuGlobals.ConfigQueryResult = ERROR_INVALID_DATA;
    }
    // If the GINA did not query the USERCONFIG data, we need to.
    if (pTerm->MuGlobals.ConfigQueryResult != ERROR_INVALID_DATA)
        Error = pTerm->MuGlobals.ConfigQueryResult;
    else
        Error = QueryTerminalServicesDataWorker(pTerm, pMprInfo->pszUserName,
                pMprInfo->pszDomain);

    // Check for an error during the USERCONFIG retrieval, or if the user is
    // disallowed from logging on to remote WinStations.
    if (Error != ERROR_SUCCESS || pTerm->MuGlobals.UserConfig.fLogonDisabled) {
        if (Error != ERROR_SUCCESS) {
            DebugLog((DEB_ERROR,"MultiUserLogonAttempt: Error from user "
                    "config query %u\n", Error));
        }
        if (pTerm->MuGlobals.UserConfig.fLogonDisabled) {
            DebugLog((DEB_TRACE,"MultiUserLogonAttempt: fLogonDisabled\n"));
        }

        HandleFailedLogon(pTerm, NULL, (NTSTATUS)ERROR_CTX_LOGON_DISABLED, 0,
                pMprInfo->pszUserName, pMprInfo->pszDomain);
        return WLX_SAS_ACTION_LOGOFF;
    }

    //
    // Send the domain, username and token TerminalServer.
    // This will also verify that the license limit has not been reached
    //
    // NOTE: The password argument of this routine is never sent or used,
    //       and could be removed. It is legacy and obsoleted.
    // Fix Bug 404814. "TestTokenForAdmin()" is called so that fIsUserAdmin parameter is set 
    // correctly in WinStationNotifyLogonWorker()
    if (!gpfnWinStationNotifyLogon((BOOLEAN)TestTokenForAdmin(hToken), hToken, pMprInfo->pszDomain,
            pMprInfo->pszUserName, L"", 0, &pTerm->MuGlobals.UserConfig)) {
        LONG error;

        if (!g_Console) {
            error = GetLastError();
            KdPrint(( "gpfnWinStationNotifyLogon, Error=%lx\n", error ));

            //
            // Don't allow logon; clean up and display appropriate
            // license dialog box
            //
            HandleFailedLogon(pTerm, NULL, (NTSTATUS)error, 0,
                    pMprInfo->pszUserName, pMprInfo->pszDomain);
            return WLX_SAS_ACTION_LOGOFF;
        }
    }

    //BUGBUG This should be done from the terminal server
    //
    //  Send server name, user name, and user domain to the client.
    //
    SizeWsCD = sizeof( WINSTATIONCLIENTDATA );

    Size1 = ((MAX_COMPUTERNAME_LENGTH + 1) * sizeof(WCHAR));
    TotalSize = sizeof( ULONG ) + SizeWsCD + Size1;

    if (pMprInfo->pszUserName != NULL) {
        Size2 = wcslen( pMprInfo->pszUserName ) * sizeof(pMprInfo->pszUserName[0]);
        TotalSize += sizeof( ULONG ) + SizeWsCD + Size2;
    }

    if (pMprInfo->pszDomain != NULL) {
        Size3 = wcslen( pMprInfo->pszDomain ) * sizeof(pMprInfo->pszDomain[0]);
        TotalSize += sizeof( ULONG ) + SizeWsCD + Size3;
    }


    pInfo = (PBYTE) Alloc( TotalSize );
    if ( pInfo != NULL) {
        pInfo1 = pInfo;

        // Size of Server Name structure
        *(ULONG UNALIGNED *)pInfo1 = SizeWsCD + Size1;
        pInfo1 += sizeof(ULONG);

        // Tell the client about the server name.
        pClientData = (PWINSTATIONCLIENTDATA) pInfo1;
        pClientData->fUnicodeData = TRUE;
        memcpy( pClientData->DataName, CLIENTDATA_SERVER,
               sizeof( CLIENTDATA_SERVER ) );
        memcpy( pInfo1 + SizeWsCD, ComputerName, Size1 );
        pInfo1 += SizeWsCD + Size1;

        if (pMprInfo->pszUserName != NULL) {
            // Size of User Name structure
            *(ULONG UNALIGNED *)pInfo1 = SizeWsCD + Size2;
            pInfo1 += sizeof(ULONG);

            // Tell the client about the user name.
            pClientData = (PWINSTATIONCLIENTDATA) pInfo1;
            pClientData->fUnicodeData = TRUE;
            memcpy( pClientData->DataName, CLIENTDATA_USERNAME,
                    sizeof( CLIENTDATA_USERNAME ) );
            memcpy( pInfo1 + SizeWsCD, pMprInfo->pszUserName, Size2 );
            pInfo1 += SizeWsCD + Size2;
        }

        if (pMprInfo->pszDomain != NULL) {
            // Size of User Domain Name structure
            *(ULONG UNALIGNED *)pInfo1 = SizeWsCD + Size3;
            pInfo1 += sizeof(ULONG);

            // Tell the client about the user's domain name.
            pClientData = (PWINSTATIONCLIENTDATA) pInfo1;
            pClientData->fUnicodeData = TRUE;
            memcpy( pClientData->DataName, CLIENTDATA_DOMAIN,
                    sizeof( CLIENTDATA_DOMAIN ) );
            memcpy( pInfo1 + SizeWsCD, pMprInfo->pszDomain, Size3 );
        }

        gpfnWinStationSetInformation( SERVERNAME_CURRENT,
                                      LOGONID_CURRENT,
                                      WinStationClientData,
                                      pInfo,
                                      TotalSize );
        Free( pInfo );
    }

    //
    // For non-Console sessions, handle WinStation callback
    //
    if ( !g_Console ) {
        WINSTATIONCONFIG ConfigData;
        WINSTATIONCLIENT ClientData;
        ULONG Length;
        BOOL  Result;
        DWORD Error;

        //
        // See if callback info is defined for the WinStation.
        // See if encryption is defined for the WinStation
        //
        if ( gpfnWinStationQueryInformation( SERVERNAME_CURRENT,
                                         LOGONID_CURRENT,
                                         WinStationConfiguration,
                                         &ConfigData,
                                         sizeof(ConfigData),
                                         &Length ) ) {
            //
            // Enforce encryption if desired
            //
            if ( ConfigData.User.MinEncryptionLevel ) {
                //
                // Get the encryption level from the client
                //
                Result = gpfnWinStationQueryInformation(
                             SERVERNAME_CURRENT,
                             LOGONID_CURRENT,
                             WinStationClient,
                             &ClientData,
                             sizeof(ClientData),
                             &Length
                             );
                if ( !Result ||
                    ((ClientData.ProtocolType != PROTOCOL_TSHARE) &&
                        (ConfigData.User.MinEncryptionLevel >
                                 ClientData.EncryptionLevel)) ) {
                    Result = TimeoutMessageBox(pTerm, NULL,
                                             IDS_MULTIUSER_ENCRYPTION_LEVEL_REQUIRED,
                                             IDS_LOGON_MESSAGE,
                                             MB_OK | MB_ICONEXCLAMATION);
                    return (WLX_SAS_ACTION_LOGOFF);
                }
            }

            //
            // Do callback, if not enabled function will return
            //
            Error = CallbackWinStation( NULL, pTerm, &ConfigData );
            if ( Error != WLX_SAS_ACTION_LOGON ) {
                return (WLX_SAS_ACTION_LOGOFF);
            }

            //
            // If we don't disabled encryption after logon, set
            // it "permanant".  The encryption logic can use this
            // to be more efficient (no extra byte for control).
            //
            if ( !ConfigData.User.fDisableEncryption ) {
                gpfnWinStationSetInformation( SERVERNAME_CURRENT,
                                          LOGONID_CURRENT,
                                          WinStationEncryptionPerm,
                                          NULL,
                                          0 );
            }
            else {
                //
                // Turn off encryption, we don't want the overhead
                //
                gpfnWinStationSetInformation( SERVERNAME_CURRENT,
                                          LOGONID_CURRENT,
                                          WinStationEncryptionOff,
                                          NULL,
                                          0 );
            }
        }
    }

    //
    // Time to reconnect sessions
    //
    WlxResult = WLX_SAS_ACTION_LOGON;
    if (CtxConnectSession(pTerm)) {
        WlxResult = WLX_SAS_ACTION_RECONNECTED;
    }
    else {
        // Tell terminal service that a new terminal user session has been
        // created.
        if ((!g_Console) && (NULL != gpfnWinStationNotifyNewSession)) {
            if (!gpfnWinStationNotifyNewSession(
                    SERVERNAME_CURRENT,
                    LOGONID_CURRENT))
            {
                HandleFailedLogon(pTerm, NULL, (NTSTATUS)GetLastError(), 0,
                        pMprInfo->pszUserName, pMprInfo->pszDomain);
                WlxResult = WLX_SAS_ACTION_LOGOFF;
            }
        }
    }

    return WlxResult;
}
