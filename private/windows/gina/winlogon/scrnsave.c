/****************************** Module Header ******************************\
* Module Name: scrnsave.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Support routines to implement screen-saver-invokation
*
* History:
* 01-23-91 Davidc       Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define LPTSTR  LPWSTR

extern DWORD g_dwScreenSaverDSSize;
extern BOOL ReturnFromPowerState ;
//
// Define the structure used to pass data into the screen-saver control dialog
//
typedef struct {
    PTERMINAL    pTerm;
    BOOL        fSecure;
    BOOL        fEnabled;
    LPTSTR      ScreenSaverName;
    DWORD       SasInterrupt;       // Sas interrupt, if any
    BOOL        WeKilledIt;
    HWND        hDlg;
    PWINLOGON_JOB Job ;
    int         ReturnValue;
    LPTSTR      DesktopPath;        // Name of desktop on which screen saver is running
} SCREEN_SAVER_DATA;
typedef SCREEN_SAVER_DATA *PSCREEN_SAVER_DATA;

#define WLX_WM_SCREENSAVER_DIED (WM_USER + 800)

// Parameters added to screen saver command line
TCHAR Parameters[] = TEXT(" /s");

//
// Private prototypes
//

INT_PTR WINAPI
ScreenSaverDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );

BOOL
ScreenSaverDlgInit(
    HWND    hDlg
    );

BOOL
StartScreenSaver(
    PSCREEN_SAVER_DATA ScreenSaverData
    );

BOOL
KillScreenSaver(
    PSCREEN_SAVER_DATA ScreenSaverData,
    int ReturnValue
    );

BOOL
StartScreenSaverMonitor(
    HWND hDlg
    );

VOID
DeleteScreenSaverMonitor(
    HWND hDlg
    );

DWORD ScreenSaverMonitorThread(
    LPVOID lpThreadParameter
    );

BOOL
GetScreenSaverInfo(
    PSCREEN_SAVER_DATA ScreenSaverData
    );

VOID
DeleteScreenSaverInfo(
    PSCREEN_SAVER_DATA ScreenSaverData
    );

// Message sent by the monitor thread to main thread window
#define WM_SCREEN_SAVER_ENDED (WM_USER + 10)


/***************************************************************************\
* ScreenSaverEnabled
*
* Checks that a screen-saver is enabled for the current logged-on user.
*
* Returns : TRUE if the current user has an enabled screen-saver, otherwise FALSE
*
* 10-15-92 Davidc       Created.
\***************************************************************************/

BOOL
ScreenSaverEnabled(
    PTERMINAL pTerm)
{
    SCREEN_SAVER_DATA ScreenSaverData;
    BOOL Enabled;
    BOOL GetSuccess ;

    ZeroMemory( &ScreenSaverData, sizeof( ScreenSaverData ) );

    ScreenSaverData.pTerm = pTerm;

    GetSuccess = GetScreenSaverInfo( &ScreenSaverData );

    Enabled = ScreenSaverData.fEnabled ;

    if ( GetSuccess ) {

        DeleteScreenSaverInfo(&ScreenSaverData);

    }

    return(Enabled);
}


/***************************************************************************\
* ValidateScreenSaver
*
* Confirm that the screen saver executable exists and it enabled.
*
* Returns :
*       TRUE - the screen-saver is ready.
*       FALSE - the screen-saver does not exist or is not enabled.
*
* 01-23-91 ericflo       Created.
\***************************************************************************/
BOOL
ValidateScreenSaver(
    PSCREEN_SAVER_DATA ssd)
{
    WIN32_FIND_DATA fd;
    HANDLE hFile;
    BOOL Enabled;
    LPTSTR  lpTempSS, lpEnd;
    WCHAR   szFake[2];
    LPTSTR  lpFake;
    HANDLE Imp ;


    //
    // Check if the screen saver enabled
    //

    Enabled = ssd->fEnabled;

    //
    // If the screen saver is enabled, confirm that the executable exists.
    //

    if (Enabled) {

        //
        // The screen save executable name contains some parameters after
        // it.  We need to allocate a temporary buffer, remove the arguments
        // and test if the executable exists.
        //

        lpTempSS = (LPTSTR) GlobalAlloc (GPTR,
                   sizeof (TCHAR) * (lstrlen (ssd->ScreenSaverName) + 1));

        if (!lpTempSS) {
            return FALSE;
        }

        //
        // Copy the filename to the temp buffer.
        //

        lstrcpy (lpTempSS, ssd->ScreenSaverName);


        //
        // Since we know how many arguments were added to the executable,
        // we can get the string length, move that many characters in from
        // the right and insert a NULL.
        //

        lpEnd = lpTempSS + lstrlen (lpTempSS);
        *(lpEnd - lstrlen (Parameters)) = TEXT('\0');

        //
        // Test to see if the executable exists.
        //

        if ( ssd->pTerm->UserLoggedOn )
        {
            Imp = ImpersonateUser( &ssd->pTerm->pWinStaWinlogon->UserProcessData,
                                   NULL );
        }
        else 
        {
            Imp = NULL ;
        }

        if (SearchPath(NULL, lpTempSS, NULL, 2, szFake, &lpFake) == 0)
        {
            Enabled = FALSE;

            DebugLog((DEB_TRACE, "Screen Saver <%S> does not exist.  Error is %d",
                      lpTempSS, GetLastError()));

        }

        if ( Imp )
        {
            StopImpersonating( Imp );
        }

        //
        // Clean up.
        //

        GlobalFree (lpTempSS);
    }

    return (Enabled);
}


/***************************************************************************\
* RunScreenSaver
*
* Starts the appropriate screen-saver for the current user in the correct
* context and waits for it to complete.
* If the user presses the SAS, we kill the screen-saver and return.
* If a logoff notification comes in, we kill the screen-saver and return.
*
* Returns :
*       DLG_SUCCESS - the screen-saver completed successfully.
*       DLG_FAILURE - unable to run the screen-saver
*       DLG_LOCK_WORKSTATION - the screen-saver completed successfully and
*                              was designated secure.
*       DLG_LOGOFF - the screen-saver was interrupted by a user logoff notification
*
* Normally the original desktop is switched back to and the desktop lock
* returned to its original state on exit.
* If the return value is DLG_LOCK_WORKSTATION or DLG_LOGOFF - the winlogon
* desktop has been switched to and the desktop lock retained.
*
* 01-23-91 Davidc       Created.
\***************************************************************************/

int
RunScreenSaver(
    PTERMINAL pTerm,
    BOOL WindowStationLocked,
    BOOL AllowFastUnlock)
{
    HDESK hdeskPrevious;
    int Result = DLG_FAILURE;
    SCREEN_SAVER_DATA ScreenSaverData;
    BOOL Success;
    WinstaState PriorState;
    DWORD BeginTime;
    DWORD EndTime;

    if ( pTerm->ScreenSaverActive )
    {
        DebugLog(( DEB_WARN, "Screensaver already active, skipping notification\n" ));
        return -1 ; 
    }
    //
    // If no one is logged on, make SYSTEM the user.
    //
    if (!pTerm->UserLoggedOn)
    {
        //
        // Toggle the locks, in case the OPENLOCK is still set from
        // a prior logoff.
        //

        UnlockWindowStation(pTerm->pWinStaWinlogon->hwinsta);
        LockWindowStation(pTerm->pWinStaWinlogon->hwinsta);
    }

    //
    // Fill in screen-saver data structure
    //

    ZeroMemory( &ScreenSaverData, sizeof( ScreenSaverData ) );
    ScreenSaverData.pTerm = pTerm;

    //
    // If we failed, but it is enabled and secure, return locked
    //

    if ( !GetScreenSaverInfo( &ScreenSaverData ) )
    {
        if ( ScreenSaverData.fEnabled && ScreenSaverData.fSecure )
        {
            return WLX_SAS_ACTION_LOCK_WKSTA ;
        }
        else
        {
            return DLG_FAILURE ;
        }

    }

    if (!ValidateScreenSaver(&ScreenSaverData)) {

        //
        // Not a valid screen saver, so bail.
        //

        DeleteScreenSaverInfo(&ScreenSaverData);

        if ( ScreenSaverData.fEnabled && ScreenSaverData.fSecure )
        {
            SetActiveDesktop( pTerm, Desktop_Winlogon );
            return WLX_SAS_ACTION_LOCK_WKSTA ;
        }
        else
        {
            return DLG_FAILURE ;
        }
    }

    //
    // If we have a secured screen saver, Create and open the app desktop.
    // Do not do that if the windowstation is locked !
    //

    if (ScreenSaverData.fSecure || pTerm->WinlogonState == Winsta_Locked_Display) {

        pTerm->pWinStaWinlogon->hdeskScreenSaver = CreateDesktopW (SCREENSAVER_DESKTOP_NAME,
                                             NULL, NULL, 0, MAXIMUM_ALLOWED, NULL);
    } else {
        pTerm->pWinStaWinlogon->hdeskScreenSaver = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
    }

    if (pTerm->pWinStaWinlogon->hdeskScreenSaver == NULL) {
        DebugLog((DEB_TRACE, "Failed to create/open screen saver desktop\n"));
        goto ExitRunScreenSaver;
    }


    //
    // Get the name of the desktop, so that we know where to launch the screen saver.
    //

    {
        DWORD   LengthNeeded;
        DWORD   CurrentLength;
        int     iWinStaLen;
        LPTSTR  Buffer;

        LengthNeeded = 0;
        CurrentLength = (lstrlen(pTerm->pWinStaWinlogon->lpWinstaName) + 2 + TYPICAL_STRING_LENGTH) * sizeof(TCHAR);
        ScreenSaverData.DesktopPath = Alloc( CurrentLength );
        if ( !ScreenSaverData.DesktopPath )
        {
            DebugLog((DEB_TRACE, "Failed to alloc %d bytes of memory\n", CurrentLength));
            goto ExitRunScreenSaver;
        }

        lstrcpy(ScreenSaverData.DesktopPath, pTerm->pWinStaWinlogon->lpWinstaName);
        lstrcat(ScreenSaverData.DesktopPath, TEXT("\\"));
        iWinStaLen = lstrlen(ScreenSaverData.DesktopPath);
        Buffer = ScreenSaverData.DesktopPath + iWinStaLen;

        if (!GetUserObjectInformation( pTerm->pWinStaWinlogon->hdeskScreenSaver,
                                       UOI_NAME,
                                       Buffer,
                                       CurrentLength - iWinStaLen,
                                       &LengthNeeded ) )
        {
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER )
            {
                DebugLog((DEB_TRACE, "GetUserObjectInformation failed GetLastError()=%d\n", GetLastError()));
                goto ExitRunScreenSaver;
            }

            Buffer = ReAlloc( ScreenSaverData.DesktopPath,
                             (LengthNeeded + (lstrlen(pTerm->pWinStaWinlogon->lpWinstaName) + 2) * sizeof(TCHAR)));
            if ( Buffer == NULL )
            {
                //
                // Reallocation failed.  Free the old buffer via DeleteScreenSaverInfo().
                //
                DebugLog((DEB_TRACE, "Failed to realloc %d bytes of memory\n",
                         (LengthNeeded + (lstrlen(pTerm->pWinStaWinlogon->lpWinstaName) + 2) * sizeof(TCHAR))));
                goto ExitRunScreenSaver;
            }

            ScreenSaverData.DesktopPath = Buffer;
            Buffer += iWinStaLen;

            if (!GetUserObjectInformation( pTerm->pWinStaWinlogon->hdeskScreenSaver,
                                           UOI_NAME,
                                           Buffer,
                                           LengthNeeded,
                                           &LengthNeeded ))
            {
                DebugLog((DEB_TRACE, "GetUserObjectInformation failed GetLastError()=%d\n", GetLastError()));
                goto ExitRunScreenSaver;
            }
        }
    }


    //
    // Update windowstation lock so screen saver can start
    //

    if (!pTerm->Gina.pWlxScreenSaverNotify(
                            pTerm->Gina.pGinaContext,
                            &ScreenSaverData.fSecure ))
    {
        DebugLog((DEB_TRACE, "GINA DLL rejected screen saver\n"));
        goto ExitRunScreenSaver;
    }


    pTerm->ScreenSaverActive = TRUE;

    if ( ScreenSaverData.fSecure )
    {

        PriorState = pTerm->WinlogonState;
        pTerm->WinlogonState = Winsta_Locked;

        DebugLog((DEB_TRACE_STATE, "RunScreenSaver:  Setting state to %s\n",
                GetState(Winsta_Locked)));
    }


    if (ScreenSaverData.fSecure || pTerm->WinlogonState == Winsta_Locked_Display) {

        //
        // Switch to screen-saver desktop
        //
        if (!SetActiveDesktop(pTerm, Desktop_ScreenSaver)) {

            DebugLog((DEB_TRACE, "Failed to switch to screen saver desktop\n"));

            pTerm->ScreenSaverActive = FALSE;
            goto ExitRunScreenSaver;
        }
    }

    //
    // Start the screen-saver
    //
    if (!StartScreenSaver(&ScreenSaverData)) {

        DebugLog((DEB_TRACE, "Failed to start screen-saver\n"));

        if (ScreenSaverData.fSecure && pTerm->UserLoggedOn)
        {
            Result = WLX_SAS_ACTION_LOCK_WKSTA;
        }
        else
        {
            Result = DLG_FAILURE;
        }
        if ( ( ScreenSaverData.fSecure ) || 
             ( pTerm->WinlogonState == Winsta_Locked_Display ) ) {

            SetActiveDesktop(pTerm, 
                             Desktop_Winlogon );
        }
        pTerm->ScreenSaverActive = FALSE;
        goto ExitRunScreenSaver;
    }


    //
    // Notify the clients
    //

    WlWalkNotifyList( pTerm, WL_NOTIFY_STARTSCREENSAVER );

    //
    // Stash begin time
    //

    BeginTime = GetTickCount();


    //
    // Initialize the sas type to report
    //

    ScreenSaverData.SasInterrupt = WLX_SAS_TYPE_SCRNSVR_ACTIVITY;

    //
    // Summon the dialog that monitors the screen-saver
    //
    Result = WlxSetTimeout( pTerm, TIMEOUT_NONE);

    Result = WlxDialogBoxParam(     pTerm,
                                    g_hInstance,
                                    (LPTSTR)IDD_CONTROL,
                                    NULL, ScreenSaverDlgProc,
                                    (LPARAM)&ScreenSaverData);

    EndTime = GetTickCount();


    //
    // Notify the clients
    //

    WlWalkNotifyList( pTerm, WL_NOTIFY_STOPSCREENSAVER );


    if (EndTime <= BeginTime)
    {
        //
        // TickCount must have wrapped around:
        //

        EndTime += (0xFFFFFFFF - BeginTime);
    }
    else
    {
        EndTime -= BeginTime;
    }

    //
    // If the screen saver ran for less than the default period, don't enforce
    // the lock.
    //
    if (ScreenSaverData.fSecure && pTerm->UserLoggedOn)
    {

        if (AllowFastUnlock &&
            (EndTime < (GetProfileInt(  APPLICATION_NAME,
                                        LOCK_GRACE_PERIOD_KEY,
                                        LOCK_DEFAULT_VALUE) * 1000)))
        {
            Result = WLX_SAS_ACTION_NONE;
            pTerm->WinlogonState = PriorState;
        }
        else
        {
            Result = WLX_SAS_ACTION_LOCK_WKSTA;
        }

    }

    DebugLog((DEB_TRACE, "Screensaver completed, SasInterrupt == %d\n",
                ScreenSaverData.SasInterrupt));



    //
    // Set up desktop and windowstation lock appropriately.  If we got a logoff,
    // or we're supposed to lock the workstation, switch back to the winlogon
    // desktop.  Otherwise, go back to the users current desktop.
    //

    if ((ScreenSaverData.SasInterrupt == WLX_SAS_TYPE_USER_LOGOFF) ||
        (Result == WLX_SAS_ACTION_LOCK_WKSTA) ) {

        //
        // Switch to the winlogon desktop and retain windowstation lock
        //
        Success = SetActiveDesktop(pTerm, Desktop_Winlogon);

    } else {

        if (ScreenSaverData.fSecure || pTerm->WinlogonState == Winsta_Locked_Display) {
            //
            // Switch to previous desktop and retore lock to previous state
            //
            SetActiveDesktop(pTerm, pTerm->pWinStaWinlogon->PreviousDesktop);
        }


        //
        // Tickle the messenger so it will display any queue'd messages.
        // (This call is a kind of NoOp).
        //
        TickleMessenger();
    }

    //
    // We are no longer active (our knowledge of the desktop state is correct),
    // let SASRouter know, so any future SAS's will be sent correctly.
    //

    pTerm->ScreenSaverActive = FALSE;
    //
    // If we killed it, it means we got a SAS and killed the screen saver.
    // we need to repost the SAS so that whomever invoked the screen saver
    // can catch it and pass it off to the gina.  Note, we special case
    // WLX_SAS_TYPE_USER_LOGOFF below, because WlxSasNotify filters it out,
    // and we really need to return as the result code...
    //
    if ( ScreenSaverData.WeKilledIt &&
         (ScreenSaverData.SasInterrupt != WLX_SAS_TYPE_SCRNSVR_ACTIVITY) )
    {
        if ( ScreenSaverData.SasInterrupt == WLX_SAS_TYPE_USER_LOGOFF )
        {
            //
            // So HandleLoggedOn() will know what to do:
            //

            pTerm->SasType = WLX_SAS_TYPE_USER_LOGOFF ;
            Result = WLX_SAS_ACTION_LOGOFF ;
        }
        else
        {
            WlxSasNotify( pTerm, ScreenSaverData.SasInterrupt );
        }
    }

ExitRunScreenSaver:

    pTerm->bIgnoreScreenSaverRequest = FALSE;

    DeleteScreenSaverInfo(&ScreenSaverData);

    if (!CloseDesktop(pTerm->pWinStaWinlogon->hdeskScreenSaver)) {
        DebugLog((DEB_TRACE, "Failed to close screen saver desktop!\n\n"));
    }
    pTerm->pWinStaWinlogon->hdeskScreenSaver = NULL;

    return(Result);
}


/***************************************************************************\
* FUNCTION: ScreenSaverDlgProc
*
* PURPOSE:  Processes messages for the screen-saver control dialog
*
* DIALOG RETURNS : DLG_FAILURE if dialog could not be created
*                  DLG_SUCCESS if the screen-saver ran correctly and
*                              has now completed.
*                  DLG_LOGOFF() if the screen-saver was interrupted by
*                              a logoff notification.
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

INT_PTR WINAPI
ScreenSaverDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PSCREEN_SAVER_DATA pScreenSaverData = (PSCREEN_SAVER_DATA)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (message) {

        case WM_INITDIALOG:
            SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);

            if (!ScreenSaverDlgInit(hDlg)) {
                EndDialog(hDlg, DLG_FAILURE);

            }

            pScreenSaverData = (PSCREEN_SAVER_DATA)lParam;

            //
            // Tell the mapping layer that we're a winlogon window, so we
            // can handle our own timeouts.
            //

            SetMapperFlag(hDlg, MAPPERFLAG_WINLOGON, pScreenSaverData->pTerm);

            EnableSasMessages( hDlg, pScreenSaverData->pTerm );

            //
            // The screen saver might have completed before we got the
            // dialog up.  In that case, the callback will not have 
            // sent a message, since there was no window.  Post it
            // back so we catch it
            //

            if ( !IsJobActive( pScreenSaverData->Job ) )
            {
                DebugLog(( DEB_TRACE, "Screen saver died during init, cancelling\n" ));

                PostMessage( hDlg, WLX_WM_SCREENSAVER_DIED, 0, 0 );
            }

            return(TRUE);

        case WLX_WM_SAS:

            if ( ( wParam != WLX_SAS_TYPE_SCRNSVR_TIMEOUT ) &&
                 ( wParam != WLX_SAS_TYPE_SC_REMOVE ) )
            {
                //
                // Just kill the screen-saver, the monitor thread will notice that
                // the process has ended and send us a message
                //

                if ( pScreenSaverData->WeKilledIt )
                {
                    //
                    // We already tried to kill it, and it's still
                    // around apparently.  Shut down this dialog and
                    // go back to normal
                    //

                    DebugLog(( DEB_TRACE, "Screen saver resisting, killing dialog\n" ));

                    PostMessage( hDlg, WLX_WM_SCREENSAVER_DIED, 0, 0 );
                }
                else 
                {
                    //
                    // Actually, stash the Sas code away, so that who ever invoked us
                    // can use it.
                    //

                    pScreenSaverData->SasInterrupt = (DWORD)wParam;

                    if (KillScreenSaver(pScreenSaverData, DLG_SUCCESS)) {

                        //
                        // Nothing.  Let this cycle around
                        //

                        DebugLog(( DEB_TRACE, "Killing screen saver due to received SAS %x\n", wParam ));

                    }
                    else 
                    {
                        //
                        // If KillScreenSaver failed, just bail out now:
                        //

                        PostMessage( hDlg, WLX_WM_SCREENSAVER_DIED, 0, 0 );
                    }

                }
            }
            else 
            {
                EnableSasMessages( hDlg, pScreenSaverData->pTerm );
            }

            return(TRUE);

        case WLX_WM_SCREENSAVER_DIED:

            EndDialog( hDlg, pScreenSaverData->ReturnValue );

            if ( !ReturnFromPowerState )
            {
                DisableSasMessages();
            }

            return TRUE ;

    }

    // We didn't process this message
    return FALSE;
}


/***************************************************************************\
* FUNCTION: ScreenSaverDlgInit
*
* PURPOSE:  Handles initialization of screen-saver control dialog
*           Actually starts the screen-saver and puts the id of the
*           screen-saver process in the screen-saver data structure.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL
ScreenSaverDlgInit(
    HWND    hDlg
    )
{
    PSCREEN_SAVER_DATA ScreenSaverData = (PSCREEN_SAVER_DATA)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    // Set our size to zero so we we don't appear
    SetWindowPos(hDlg, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE |
                                         SWP_NOREDRAW | SWP_NOZORDER);

    //
    // Initialize our return value
    //
    ScreenSaverData->ReturnValue = DLG_SUCCESS;
    ScreenSaverData->hDlg = hDlg ;


    return(TRUE);
}

DWORD
ScreenSaverCallback(
    PVOID Parameter
    )
{
    PSCREEN_SAVER_DATA pScreenSaverData = (PSCREEN_SAVER_DATA) Parameter ;

    PostMessage( pScreenSaverData->hDlg, WLX_WM_SCREENSAVER_DIED, 0, 0 );

    return 0 ;
}

/***************************************************************************\
* FUNCTION: StartScreenSaver
*
* PURPOSE:  Creates the screen-saver process
*
* RETURNS:  TRUE on success, FALSE on failure
*
* On successful return, the ProcessId field in our global data structure
* is set to the screen-saver process id
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL
StartScreenSaver(
    PSCREEN_SAVER_DATA ScreenSaverData
    )
{
    PWINLOGON_JOB Job ;


    Job = CreateWinlogonJob();

    if ( Job == NULL )
    {
        return FALSE ;
    }

    ScreenSaverData->WeKilledIt = FALSE ;


    SetWinlogonJobTimeout( Job, INFINITE );

    SetJobCallback( Job, ScreenSaverCallback, ScreenSaverData );

    SetWinlogonJobOption( Job, WINLOGON_JOB_MONITOR_ROOT_PROCESS );

    if (! StartProcessInJob(
                    ScreenSaverData->pTerm,
                    (ScreenSaverData->pTerm->UserLoggedOn ? ProcessAsUser : ProcessAsSystemRestricted),
                    ScreenSaverData->DesktopPath,
                    ScreenSaverData->pTerm->pWinStaWinlogon->UserProcessData.pEnvironment,
                    ScreenSaverData->ScreenSaverName,
                    CREATE_SEPARATE_WOW_VDM | IDLE_PRIORITY_CLASS,
                    STARTF_SCREENSAVER,
                    Job ) )
    {
        DebugLog((DEB_ERROR, "Failed to start screen saver %ws, %d\n",
                ScreenSaverData->ScreenSaverName, GetLastError()));

        DeleteJob( Job );
        return(FALSE);
    }

    ScreenSaverData->Job = Job ;

    return TRUE;
}


/***************************************************************************\
* FUNCTION: KillScreenSaver
*
* PURPOSE:  Terminates the screen-saver process
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL
KillScreenSaver(
    PSCREEN_SAVER_DATA ScreenSaverData,
    int ReturnValue
    )
{
    BOOL Result ;

    ScreenSaverData->WeKilledIt = TRUE ;

    Result = TerminateJob( ScreenSaverData->Job, 0 );

    ScreenSaverData->ReturnValue = ReturnValue;

    return ( Result );
}




/***************************************************************************\
* FUNCTION: GetScreenSaverInfo
*
* PURPOSE:  Gets the name of the screen-saver that should be run. Also whether
*           the user wanted the screen-saver to be secure. These values are
*           filled in in the ScreenSaver data structure on return.
*
*           If there is no current user logged on or if we fail to get the
*           user's preferred screen-saver info, we default to the system
*           secure screen-saver.
*
*           NOTE:  On return, fSecure and fEnabled are *always valid*, even if
*           the rest of the screensaver info could not be loaded.
*
* RETURNS:  TRUE on success
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL
GetScreenSaverInfo(
    PSCREEN_SAVER_DATA ScreenSaverData
    )
{
    BOOL Success = FALSE;
    TCHAR SystemScreenSaverName[MAX_STRING_BYTES];
    WCHAR ExpandedName[MAX_PATH];
    DWORD ScreenSaverLength;
    DWORD ExpandedSize;
    DWORD dwType ;
    DWORD dwSize ;
    CHAR  Buffer[ 10 ];
    int err ;
    HKEY UserControl ;
    HKEY hKeyPolicy;
    PWINDOWSTATION pWS ;

    //
    // Find out if the screen-saver should be secure
    //

    pWS = ScreenSaverData->pTerm->pWinStaWinlogon ;

    //
    // Fail secure if we can't get to config data:
    //

    ScreenSaverData->fSecure = TRUE ;
    ScreenSaverData->fEnabled = TRUE ;

    if ( OpenHKeyCurrentUser( pWS ))
    {
        ScreenSaverData->fSecure = FALSE ;
        ScreenSaverData->fEnabled = FALSE ;
        ScreenSaverData->ScreenSaverName = NULL ;

        strcpy(Buffer, "0" );

        err = RegOpenKeyEx(
                    pWS->UserProcessData.hCurrentUser,
                    SCREENSAVER_KEY,
                    0,
                    KEY_READ,
                    &UserControl );

        if ( err == 0 )
        {
            dwSize = sizeof( Buffer );

            err = RegQueryValueExA(
                    UserControl,
                    SCREEN_SAVER_SECURE_KEY,
                    NULL,
                    &dwType,
                    Buffer,
                    &dwSize );

            if ( err == 0 )
            {
                if ( dwType == REG_SZ )
                {
                    ScreenSaverData->fSecure = atoi( Buffer );
                }
            }

            dwSize = sizeof( Buffer );

            err = RegQueryValueExA(
                    UserControl,
                    SCREEN_SAVER_ENABLED_KEY,
                    NULL,
                    &dwType,
                    Buffer,
                    &dwSize );
                    
            if ( err == 0 )
            {
                if ( dwType == REG_SZ )
                {
                    ScreenSaverData->fEnabled = atoi( Buffer );
                }
            }

            dwSize = 0 ;

            err = RegQueryValueExW(
                    UserControl,
                    SCREEN_SAVER_FILENAME_KEY,
                    NULL,
                    &dwType,
                    NULL,
                    &dwSize );

            if ( err != ERROR_FILE_NOT_FOUND )
            {
                ScreenSaverData->ScreenSaverName = LocalAlloc( LMEM_FIXED, dwSize );

                if ( ScreenSaverData->ScreenSaverName )
                {
                    err = RegQueryValueExW(
                            UserControl,
                            SCREEN_SAVER_FILENAME_KEY,
                            NULL,
                            &dwType,
                            (PUCHAR) ScreenSaverData->ScreenSaverName,
                            &dwSize );

                    if ( err )
                    {
                        LocalFree( ScreenSaverData->ScreenSaverName );

                        ScreenSaverData->ScreenSaverName = NULL ;

                    }

                }

            }
            else 
            {
                //
                // SCRNSAVE.EXE value not present.  Override the fEnabled flag
                //

                ScreenSaverData->fEnabled = FALSE ;
            }

            RegCloseKey( UserControl );

        }


        if (RegOpenKeyEx (
              pWS->UserProcessData.hCurrentUser,
              SCREENSAVER_POLICY_KEY,
              0,
              KEY_READ,
              &hKeyPolicy) == ERROR_SUCCESS)
        {
            DWORD   dwBuffer = 0;

			/*
			 * handle policy values
			 * lous NTBUG:421424
			 * remove query for REG_DWORD type

            
            dwSize = sizeof( dwBuffer );

            err = RegQueryValueExA(
                    hKeyPolicy,
                    SCREEN_SAVER_SECURE_KEY,
                    NULL,
                    &dwType,
                    (LPBYTE)&dwBuffer,
                    &dwSize );
			if ( err == 0 && dwType == REG_DWORD )
            {
                ScreenSaverData->fSecure = dwBuffer;
            }

            dwSize = sizeof( dwBuffer );

			err = RegQueryValueExA(
                    hKeyPolicy,
                    SCREEN_SAVER_ENABLED_KEY,
                    NULL,
                    &dwType,
                    (LPBYTE)&dwBuffer,
                    &dwSize );

            if ( err == 0 && dwType == REG_DWORD )
            {
               ScreenSaverData->fEnabled = dwBuffer;
            }
			*/

			/*
			 * handle policy values
			 * lous NTBUG:421424
			 * add query for REG_SZ type
			 */

			dwSize = sizeof( Buffer );

            err = RegQueryValueExA(
                    hKeyPolicy,
                    SCREEN_SAVER_SECURE_KEY,
                    NULL,
                    &dwType,
                    Buffer,
                    &dwSize );

            if ( err == 0 )
            {
                if ( dwType == REG_SZ )
                {
                    ScreenSaverData->fSecure = atoi( Buffer );
                }
            }

			dwSize = sizeof( Buffer );

            err = RegQueryValueExA(
                    hKeyPolicy,
                    SCREEN_SAVER_ENABLED_KEY,
                    NULL,
                    &dwType,
                    Buffer,
                    &dwSize );
                    
            if ( err == 0 )
            {
                if ( dwType == REG_SZ )
                {
                    ScreenSaverData->fEnabled = atoi( Buffer );
                }
            }

            dwSize = 0 ;

            err = RegQueryValueExW(
                    hKeyPolicy,
                    SCREEN_SAVER_FILENAME_KEY,
                    NULL,
                    &dwType,
                    NULL,
                    &dwSize );

            if ( err != ERROR_FILE_NOT_FOUND )
            {
                    if ( ScreenSaverData->ScreenSaverName )
                    {
                            LocalFree( ScreenSaverData->ScreenSaverName );
                            ScreenSaverData->ScreenSaverName = 0;
                    }

                ScreenSaverData->ScreenSaverName = LocalAlloc( LMEM_FIXED, dwSize );

                if ( ScreenSaverData->ScreenSaverName )
                {
                    err = RegQueryValueExW(
                            hKeyPolicy,
                            SCREEN_SAVER_FILENAME_KEY,
                            NULL,
                            &dwType,
                            (PUCHAR) ScreenSaverData->ScreenSaverName,
                            &dwSize );

                    if ( err )
                    {
                        LocalFree( ScreenSaverData->ScreenSaverName );

                        ScreenSaverData->ScreenSaverName = NULL ;
                    }
                }
            }

            RegCloseKey (hKeyPolicy);
        }

        CloseHKeyCurrentUser( pWS );
    }


    if (ScreenSaverData->ScreenSaverName == NULL) {
        DebugLog((DEB_TRACE, "Failed to get screen-saver name\n"));
        goto Exit;
    }

    ScreenSaverLength = lstrlen(ScreenSaverData->ScreenSaverName);

    //
    // Figure out how big based on the expanded environment variables, if any.
    // subtract one, since Expand returns the # characters, plus the \0.
    //
    ExpandedSize = ExpandEnvironmentStrings(ScreenSaverData->ScreenSaverName,
                                            ExpandedName,
                                            MAX_PATH) - 1;

    if (ExpandedSize != ScreenSaverLength )
    {
        //
        // Well, we expanded to something other than what we had originally.
        // So, alloc anew and do the parameter thing right now.
        //

        Free(ScreenSaverData->ScreenSaverName);
        ScreenSaverData->ScreenSaverName = Alloc( (ExpandedSize +
                                                    lstrlen(Parameters) + 1) *
                                                    sizeof(WCHAR) );

        if (!ScreenSaverData->ScreenSaverName)
        {
            DebugLog((DEB_WARN, "No memory for screensaver\n"));
            goto Exit;
        }

        wcscpy( ScreenSaverData->ScreenSaverName,
                ExpandedName);
        wcscpy( &ScreenSaverData->ScreenSaverName[ExpandedSize],
                Parameters);

    }
    else
    {
        LPTSTR  Temp;

        //
        // Always add some fixed screen-saver parameters
        //

        Temp = ReAlloc( ScreenSaverData->ScreenSaverName,
                        (lstrlen(ScreenSaverData->ScreenSaverName) +
                         lstrlen(Parameters) + 1)
                        * sizeof(TCHAR));
        if (Temp == NULL) {
            DebugLog((DEB_TRACE, "Realloc of screen-saver name failed\n"));
            goto Exit;
        }

        ScreenSaverData->ScreenSaverName = Temp;
        lstrcat(ScreenSaverData->ScreenSaverName, Parameters);

    }

    if ( _wcsicmp( ScreenSaverData->ScreenSaverName, TEXT("(NONE)") ) == 0 )
    {
        ScreenSaverData->fEnabled = FALSE ;
    }


    Success = TRUE;

Exit:

    //
    // Close the ini file mapping - this closes the user registry key
    //


    return(Success);
}


/***************************************************************************\
* FUNCTION: DeleteScreenSaverInfo
*
* PURPOSE:  Frees up any space allocate by screen-saver data structure
*
* RETURNS:  Nothing
*
* HISTORY:
*
*   11-17-92 Davidc       Created.
*
\***************************************************************************/

VOID
DeleteScreenSaverInfo(
    PSCREEN_SAVER_DATA ScreenSaverData
    )
{
    if (ScreenSaverData->ScreenSaverName != NULL) {
        Free(ScreenSaverData->ScreenSaverName);
    }

    if (ScreenSaverData->DesktopPath != NULL) {
        Free(ScreenSaverData->DesktopPath);
    }

    if ( ScreenSaverData->Job )
    {
        DeleteJob( ScreenSaverData->Job );
    }

}
