/****************************** Module Header ******************************\
* Module Name: sas.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Support routines to implement processing of the secure attention sequence
*
* Users must always press the SAS key sequence before entering a password.
* This module catches the key press and forwards a SAS message to the
* correct winlogon window.
*
* History:
* 12-05-91 Davidc       Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "regstr.h"

#include <initguid.h>
#include <mstask.h>

// Internal Prototypes

typedef LRESULT
(WINAPI LOGON_NOTIFY_FN)(
    IN PTERMINAL pTerm,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

typedef LOGON_NOTIFY_FN * PLOGON_NOTIFY_FN ;

LRESULT
SasPlaySound(
    PTERMINAL pTerm,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT SASWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);

BOOL SASCreate(
    HWND hwnd);

NTSTATUS
SleepSystem(
    IN PTERMINAL pTerm,
    IN HWND hWnd,
    IN PPOWERSTATEPARAMS pPSP
    );

INT_PTR WINAPI
SleepDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );

// Global for SAS window class name
static PWCHAR  szSASClass = TEXT("SAS window class");

#define SHELL_RESTART_TIMER_ID  100

/*
 * The PlayEventThread thread will play the specified sound.  This allows
 * win32k to play event sounds without having to go through CSRSS.
 */

/*
 * This list must be kept in sync with the USER_SOUND_ equates in winuserp.h
 */

static CONST LPCWSTR lpszUserSounds[USER_SOUND_MAX] = {
    L".Default",
    L"SystemHand",
    L"SystemQuestion",
    L"SystemExclamation",
    L"SystemAsterisk",
    L"MenuPopup",
    L"MenuCommand",
    L"Open",
    L"Close",
    L"RestoreUp",
    L"RestoreDown",
    L"Minimize",
    L"Maximize",
    L"SnapShot"};


/*
 * This array of registry pointers is tied in with the discharge policies in
 * ntpoapi.h.  The array should never be greater than NUM_DISCHARGE_POLICIES.
 */

CONST WCHAR wcCrit[] = REGSTR_PATH_APPS L"\\PowerCfg\\CriticalBatteryAlarm\\.Current";
CONST WCHAR wcLow[] = REGSTR_PATH_APPS L"\\PowerCfg\\LowBatteryAlarm\\.Current";
CONST LPCWSTR gaPowerEventSounds[] = {wcCrit, wcLow};

CONST LPCWSTR gaPowerEventWorkItems[] = {
    TEXT("Critical Battery Alarm Program"),
    TEXT("Low Battery Alarm Program")
};

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define STR_POWEREVENT_START    IDS_CRITBAT_TITLE
#define STR_POWEREVENT_END      IDS_LOWBAT_TITLE

BOOL ReturnFromPowerState ;

/***************************************************************************\
* SASInit
*
* Initialises this module.
*
* Creates a window to receive the SAS and registers the
* key sequence as a hot key.
*
* Returns TRUE on success, FALSE on failure.
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

BOOL SASInit(
    PTERMINAL pTerm)
{
    WNDCLASS wc;

    //
    // Register the notification window class
    //

    wc.style            = CS_SAVEBITS;
    wc.lpfnWndProc      = SASWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = g_hInstance;
    wc.hIcon            = NULL;
    wc.hCursor          = NULL;
    wc.hbrBackground    = NULL;
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = szSASClass;

    //
    // Don't check the return value because for multimonitors
    // we already register the SAS class.
    //

    RegisterClass(&wc);

    pTerm->hwndSAS = CreateWindowEx(0L, szSASClass, TEXT("SAS window"),
            WS_OVERLAPPEDWINDOW,
            0, 0, 0, 0,
            NULL, NULL, g_hInstance, NULL);

    if (pTerm->hwndSAS == NULL)
        return FALSE;


    //
    // Store our terminal pointer in the window user data
    //

    SetWindowLongPtr(pTerm->hwndSAS, GWLP_USERDATA, (LONG_PTR)pTerm);


    //
    // Register this window with windows so we get notified for
    // screen-saver startup and user log-off
    //

    if (!SetLogonNotifyWindow(pTerm->hwndSAS)) {
        DebugLog((DEB_ERROR, "Failed to set logon notify window"));
        return FALSE;
    }

    return TRUE;
}

/***************************************************************************\
* SASTerminate
*
* Terminates this module.
*
* Unregisters the SAS and destroys the SAS windows
*
* 12-05-91 Davidc       Created.
\***************************************************************************/

VOID SASTerminate(
    PTERMINAL pTerm)
{
    DestroyWindow(pTerm->hwndSAS);

    pTerm->hwndSAS = NULL;
}


#if DBG
void QuickReboot(void)
{
    EnablePrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE);
    NtShutdownSystem(TRUE);
}
#endif

/***************************************************************************\
* SASWndProc
*
* Window procedure for the SAS window.
*
* This window registers the SAS hotkey sequence, and forwards any hotkey
* messages to the current winlogon window. It does this using a
* timeout module function. i.e. every window should register a timeout
* even if it's 0 if they want to get SAS messages.
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

LPTHREAD_START_ROUTINE aRtn[] = {
    NULL,
    StickyKeysNotificationThread,   //ACCESS_STICKYKEYS
    FilterKeysNotificationThread,   //ACCESS_FILTERKEYS
    MouseKeysNotificationThread,    //ACCESS_MOUSEKEYS
    ToggleKeysNotificationThread,   //ACCESS_TOGGLEKEYS
    HighContNotificationThread, //ACCESS_HIGHCONTRAST
    UtilManStartThread          //ACCESS_UTILITYMANGER
};


LRESULT
SasPlaySound(
    PTERMINAL pTerm,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HANDLE           hImp;
    BOOL b;

    if (lParam >= USER_SOUND_MAX)
    {
        return 0 ;
    }

    OpenIniFileUserMapping( pTerm );

    hImp = ImpersonateUser(&(pTerm->pWinStaWinlogon->UserProcessData), NULL);

    if (hImp == NULL) {
        CloseIniFileUserMapping( pTerm );
        return 0 ;
    }

    __try
    {
        b = PlaySound(
                lpszUserSounds[lParam],
                NULL,
                SND_ALIAS | SND_NODEFAULT | SND_ASYNC );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        b = FALSE;
    }

    if (b == FALSE) {
        switch(lParam) {
        case USER_SOUND_SYSTEMHAND:
        case USER_SOUND_SYSTEMQUESTION:
        case USER_SOUND_SYSTEMEXCLAMATION:
        case USER_SOUND_SYSTEMASTERISK:
        case USER_SOUND_DEFAULT:
            Beep(440, 125);
        }
    }

    StopImpersonating(hImp);

    CloseIniFileUserMapping( pTerm );

    return 0 ;
}

LRESULT
SasPlayPowerSound(
    IN PTERMINAL pTerm,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    HANDLE           hImp;
    UINT wEventType = LOWORD(lParam);
    UINT wEventLevel = HIWORD(lParam);
    BOOL b ;

    OpenIniFileUserMapping( pTerm );

    hImp = ImpersonateUser(&(pTerm->pWinStaWinlogon->UserProcessData), NULL);

    if (hImp == NULL) {
        CloseIniFileUserMapping( pTerm );
        return 0 ;
    }

    /*
     * Alert the user that the battery is low by playing a sound.
     */
    if (wEventType & POWER_LEVEL_USER_NOTIFY_SOUND) {
        HKEY hkeyMM;
        HKEY hCurrent ;
        DWORD Status;
        DWORD dwType;
        WCHAR wcSound[MAXIMUM_FILENAME_LENGTH];
        DWORD cbData = MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR);

        if (wEventLevel >= ARRAY_SIZE(gaPowerEventSounds)) {
            goto CleanUp;
        }

        b = FALSE;

        if ( !NT_SUCCESS( RtlOpenCurrentUser( KEY_READ, &hCurrent ) ) )
        {
            goto CleanUp ;
        }

        Status = RegOpenKeyEx(
                    hCurrent,
                    gaPowerEventSounds[wEventLevel],
                    0,
                    KEY_READ,
                    &hkeyMM);

        RegCloseKey( hCurrent );

        if (Status == ERROR_SUCCESS) {

            Status = RegQueryValueEx(hkeyMM, NULL, NULL, &dwType,
                (LPBYTE)wcSound, &cbData);
            if ((Status == ERROR_SUCCESS) &&
                (dwType == REG_SZ) &&
                (wcSound[(cbData/sizeof(WCHAR))-1] == '\0')) {
                __try {
                    b = PlaySound(wcSound,
                                NULL,
                                SND_FILENAME | SND_NODEFAULT | SND_ASYNC);
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                }
            }

            RegCloseKey(hkeyMM);
        }
        if (b == FALSE) {
          /*
           * No sound card, so just do the default beep.
           */
            Beep(440, 125);
        }
    }

    /*
     * Alert the user that the battery is low by putting up a message box.
     */
    if (wEventType & POWER_LEVEL_USER_NOTIFY_TEXT) {
        PWSTR pszMsg;
        WCHAR TitleBuffer[MAX_STRING_BYTES];
        WCHAR MsgBuffer[MAX_STRING_BYTES];
        UNICODE_STRING Title, Msg;
        ULONG_PTR Parameters[ 3 ];
        ULONG Response;

        if (wEventLevel + STR_POWEREVENT_START > STR_POWEREVENT_END) {
            goto CleanUp;
        }

        LoadString(NULL, wEventLevel + STR_POWEREVENT_START,
                       TitleBuffer, ARRAYSIZE(TitleBuffer));
        LoadString(NULL, IDS_BATTERY_MSG,
                       MsgBuffer, ARRAYSIZE(MsgBuffer));

        RtlInitUnicodeString(&Msg, MsgBuffer);
        RtlInitUnicodeString(&Title, TitleBuffer);

        Parameters[0] = (ULONG_PTR)&Msg;
        Parameters[1] = (ULONG_PTR)&Title;
        Parameters[2] = MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND;
        NtRaiseHardError(STATUS_SERVICE_NOTIFICATION | HARDERROR_OVERRIDE_ERRORMODE,
            3,  // Number of parameters
            3,  // Parameter mask -- first two are pointers
            Parameters,
            OptionOkNoWait,
            &Response
        );
    }

    /*
     * Execute the action specified by the user when the battery is low.
     */
    if (wEventType & POWER_LEVEL_USER_NOTIFY_EXEC) {
        ITaskScheduler   *pISchedAgent = NULL;
        ITask            *pITask;

        HRESULT     hr;

        if (wEventLevel >= ARRAY_SIZE(gaPowerEventWorkItems)) {
            DebugLog((DEB_ERROR, "SASWndProc: bad wEventLevel\n"));
            goto CleanUp;
        }

        //
        // OLE is delay loaded, so this try/except is to catch
        // any delay load failures and drop the call
        //

        __try {

            hr = CoInitialize(NULL);

            if (SUCCEEDED(hr)) {
                hr = CoCreateInstance( &CLSID_CSchedulingAgent,
                                       NULL,
                                       CLSCTX_INPROC_SERVER,
                                       &IID_ISchedulingAgent,
                                       (LPVOID*)&pISchedAgent);

                if (SUCCEEDED(hr)) {
                    hr = pISchedAgent->lpVtbl->Activate(
                                pISchedAgent,
                                gaPowerEventWorkItems[wEventLevel],
                                &IID_ITask,
                                &(IUnknown *)pITask);

                    if (SUCCEEDED(hr)) {
                        pITask->lpVtbl->Run(pITask);
                        pITask->lpVtbl->Release(pITask);
                    }
                    else {
                        DebugLog((DEB_ERROR, "SASWndProc: ISchedAgent::Activate failed.\n"));
                    }

                    pISchedAgent->lpVtbl->Release(pISchedAgent);
                }
                else {
                    DebugLog((DEB_ERROR, "SASWndProc: CoCreateInstance failed.\n"));
                }

                CoUninitialize();
            }
            else {
                DebugLog((DEB_ERROR, "SASWndProc: CoInitialize failed.\n"));
            }

        } __except( EXCEPTION_EXECUTE_HANDLER )
        {
            NOTHING ;
        }


    }


CleanUp:
    StopImpersonating(hImp);
    CloseIniFileUserMapping( pTerm );

    return 0;

}

SasAccessNotify(
    IN PTERMINAL pTerm,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    WCHAR szDesktop[ MAX_PATH ];
    HANDLE hThread;
    HDESK hDesk ;
    WCHAR buf[80], bb[4];
    int Len1, Len2;
    BOOL b ;
    PWINLOGON_JOB Job ;

    switch(LOWORD(lParam)) {
        case ACCESS_STICKYKEYS:
        case ACCESS_FILTERKEYS:
        case ACCESS_TOGGLEKEYS:
        case ACCESS_MOUSEKEYS:
        case ACCESS_HIGHCONTRAST:
        case ACCESS_UTILITYMANAGER:
            hThread = CreateThread(NULL, 0, aRtn[LOWORD(lParam)], (LPVOID)(pTerm), 0, NULL);
            CloseHandle(hThread);
            break;
        case ACCESS_HIGHCONTRASTON:
        case ACCESS_HIGHCONTRASTONNOREG:
            bb[0] = TEXT('1');
            bb[1] = TEXT('0');
            goto SpawnProcess;
        case ACCESS_HIGHCONTRASTOFF:
        case ACCESS_HIGHCONTRASTOFFNOREG:
            bb[0] = TEXT('0');
            bb[1] = TEXT('1');
            goto SpawnProcess;
        case ACCESS_HIGHCONTRASTCHANGE:
        case ACCESS_HIGHCONTRASTCHANGENOREG:
            bb[0] = TEXT('1');
            bb[1] = TEXT('1');
    SpawnProcess:
        if (LOWORD(lParam) & ACCESS_HIGHCONTRASTNOREG) {
            bb[2] = TEXT('1');
        } else {
            bb[2] = TEXT('0');
        }
        bb[3] = 0;
        wsprintfW(buf, L"sethc %ws", bb);

        hDesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);

        if (!hDesk) break;

        wsprintfW (szDesktop, L"%s\\", pTerm->pWinStaWinlogon->lpWinstaName);

        Len1 = wcslen(szDesktop);

        b = GetUserObjectInformation(hDesk, UOI_NAME, &szDesktop[Len1], MAX_PATH - Len1, &Len2);

        Job = CreateWinlogonJob();

        if ( Job )
        {
            SetWinlogonJobTimeout( Job, 5 * 60 * 1000 );

            SetWinlogonJobOption( Job, WINLOGON_JOB_AUTONOMOUS );

            StartProcessInJob(
                pTerm,
                (pTerm->UserLoggedOn ? ProcessAsUser : ProcessAsSystem),
                szDesktop,
                NULL,
                buf,
                0,
                0,
                Job );

            //
            // Delete the job.  The job will self-destruct if it
            // hasn't completed after 5 minutes.  We don't care
            // at this point.
            //

            DeleteJob( Job );

        }

        CloseDesktop(hDesk);

    }
    return 0 ;

}


LRESULT SASWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    WCHAR     szDesktop[MAX_PATH];
    PTERMINAL pTerm = (PTERMINAL)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    HANDLE hThread;
    DWORD dwCode;
    WINSTATIONINFORMATION InfoData;
    ULONG Length;


    switch (message)
    {

        case WM_CREATE:
            if (!SASCreate(hwnd))
            {
                return(TRUE);   // Fail creation
            }
            return(FALSE); // Continue creating window

        case WM_DESTROY:
            DebugLog(( DEB_TRACE, "SAS Window Shutting down?\n"));
            SASDestroy(hwnd);
            return(0);

        case WM_HOTKEY:
#if DBG
            if (wParam == 1)
            {
                QuickReboot();
                return(0);
            }


            if (wParam == 2)
            {
                switch (pTerm->pWinStaWinlogon->ActiveDesktop)
                {
                    case Desktop_Winlogon:
                        SetActiveDesktop(pTerm, Desktop_Application);
                        break;
                    case Desktop_Application:
                        SetActiveDesktop(pTerm, Desktop_Winlogon);
                        break;
                }
                return(0);
            }
            if (wParam == 3)
            {
                DebugBreak();
                return(0);
            }
#endif
            if (wParam == 4)
            {
                WCHAR szTaskMgr[] = L"taskmgr.exe";

                wsprintfW (szDesktop, L"%s\\%s", pTerm->pWinStaWinlogon->lpWinstaName,
                           APPLICATION_DESKTOP_NAME);

                DebugLog((DEB_TRACE, "Starting taskmgr.exe.\n"));

                if ( pTerm->UserLoggedOn &&
                     !IsLocked( pTerm->WinlogonState ) ) {
                    StartApplication(pTerm,
                                     szDesktop,
                                     pTerm->pWinStaWinlogon->UserProcessData.pEnvironment,
                                     szTaskMgr);
                }
                return(0);
            }

            CADNotify(pTerm, WLX_SAS_TYPE_CTRL_ALT_DEL);
            return(0);

        case WM_LOGONNOTIFY: // A private notification from Windows

            DebugLog((DEB_TRACE_SAS, "LOGONNOTIFY message %d\n", wParam ));

            switch (wParam)
            {
                /*
                 * LOGON_PLAYEVENTSOUND and LOGON_PLAYPOWERSOUND
                 * are posted from the kernel so
                 * that sounds can be played without an intricate
                 * connection to CSRSS.  This allows the multimedia
                 * code to not be loaded into CSRSS which makes
                 * system booting much more predictable.
                 */
                case LOGON_PLAYEVENTSOUND:
                    return SasPlaySound( pTerm,
                                         wParam,
                                         lParam );



                case LOGON_PLAYPOWERSOUND:
                    return SasPlayPowerSound( pTerm,
                                              wParam,
                                              lParam );


                /*
                 * LOGON_ACCESSNOTIFY feature added 2/97 to facilitate
                 * notification dialogs for accessibility features and
                 * to facilitate the changing of display schemes to
                 * support the High Contrast accessibility feature.
                 *      Fritz Sands.
                 */

                case LOGON_ACCESSNOTIFY:
                    return SasAccessNotify( pTerm,
                                            wParam,
                                            lParam );
                    break;

                case SESSION_LOGOFF:
                    //
                    // Logoff the user
                    //
                    if ( !ExitWindowsInProgress ) {
                         if (!pTerm->UserLoggedOn && ( !g_Console ) ) {
                             //
                             // If we are not logged on, only the following works
                             //
                            ExitProcess( 0 );
                         }
                    }
                    //
                    // If a user is logged on fall thru
                    //
                case LOGON_LOGOFF:

#if DBG
                    DebugLog((DEB_TRACE_SAS, "\tWINLOGON     : %s\n", (lParam & EWX_WINLOGON_CALLER) ? "True" : "False"));
                    DebugLog((DEB_TRACE_SAS, "\tSYSTEM       : %s\n", (lParam & EWX_SYSTEM_CALLER) ? "True" : "False"));
                    DebugLog((DEB_TRACE_SAS, "\tSHUTDOWN     : %s\n", (lParam & EWX_SHUTDOWN) ? "True" : "False"));
                    DebugLog((DEB_TRACE_SAS, "\tREBOOT       : %s\n", (lParam & EWX_REBOOT) ? "True" : "False"));
                    DebugLog((DEB_TRACE_SAS, "\tPOWEROFF     : %s\n", (lParam & EWX_POWEROFF) ? "True" : "False"));
                    DebugLog((DEB_TRACE_SAS, "\tFORCE        : %s\n", (lParam & EWX_FORCE) ? "True" : "False"));
                    DebugLog((DEB_TRACE_SAS, "\tOLD_SYSTEM   : %s\n", (lParam & EWX_WINLOGON_OLD_SYSTEM) ? "True" : "False"));
                    DebugLog((DEB_TRACE_SAS, "\tOLD_SHUTDOWN : %s\n", (lParam & EWX_WINLOGON_OLD_SHUTDOWN) ? "True" : "False"));
                    DebugLog((DEB_TRACE_SAS, "\tOLD_REBOOT   : %s\n", (lParam & EWX_WINLOGON_OLD_REBOOT) ? "True" : "False"));
                    DebugLog((DEB_TRACE_SAS, "\tOLD_POWEROFF : %s\n", (lParam & EWX_WINLOGON_OLD_POWEROFF) ? "True" : "False"));
#endif

                    //
                    // If there is an exit windows in progress, reject this
                    // message if it is not our own call coming back.  This
                    // prevents people from calling ExitWindowsEx repeatedly
                    //

                    if ( ExitWindowsInProgress &&
                         ( !( lParam & EWX_WINLOGON_CALLER ) ) )
                    {
                        break;

                    }
                    pTerm->LogoffFlags = (DWORD)lParam;
                    CADNotify(pTerm, WLX_SAS_TYPE_USER_LOGOFF);
                    break;

                case LOGON_LOGOFFCANCELED:
                    //
                    // User has cancelled a logoff.
                    //

                    if ( !ExitWindowsInProgress)
                    {
                        DebugLog(( DEB_WARN, "Logoff Cancelled notice with no logoff pending?\n"));
                    }
                    ExitWindowsInProgress = FALSE ;
                    break;

                case LOGON_INPUT_TIMEOUT:
                {
                    BOOL bSecure = TRUE ;
                    //
                    // Notify the current window
                    //

                    //
                    // Only run the screen saver if we are NOT disconnected
                    //
                    if ( !g_Console && gpfnWinStationQueryInformation( SERVERNAME_CURRENT,
                                                     LOGONID_CURRENT,
                                                     WinStationInformation,
                                                     &InfoData,
                                                     sizeof(InfoData),
                                                     &Length )) {

                        if (InfoData.ConnectState == State_Disconnected) {

                            pTerm->bIgnoreScreenSaverRequest = TRUE;
                        }


                    }

                    if ( OpenHKeyCurrentUser( pTerm->pWinStaWinlogon ) )
                    {
                        int err ;
                        HKEY Desktop ;
                        DWORD dwSize ;
                        DWORD dwType ;
                        CHAR Value[ 4 ];

                        err = RegOpenKeyEx( pTerm->pWinStaWinlogon->UserProcessData.hCurrentUser,
                                            SCREENSAVER_KEY,
                                            0,
                                            KEY_READ,
                                            &Desktop );

                        if ( err == 0 )
                        {
                            dwSize = sizeof( Value );
                            err = RegQueryValueExA(
                                        Desktop,
                                        SCREEN_SAVER_SECURE_KEY,
                                        0,
                                        &dwType,
                                        Value,
                                        &dwSize );

                            if ( err == 0 )
                            {
                                bSecure = atoi( Value );
                            }

                            RegCloseKey( Desktop );
                        }

                        CloseHKeyCurrentUser( pTerm->pWinStaWinlogon );

                    }


                    if ((!bSecure && lParam != 0) || pTerm->bIgnoreScreenSaverRequest) {
                        break;
                    }

                    // pTerm->bIgnoreScreenSaverRequest = TRUE;

                    CADNotify(pTerm, WLX_SAS_TYPE_SCRNSVR_TIMEOUT);
                    break;
                }
                case LOGON_RESTARTSHELL:
                    //
                    // Restart the shell after X seconds
                    //
                    // We don't restart the shell for the following conditions:
                    //
                    // 1) No one is logged on
                    // 2) We are in the process of logging off
                    //    (logoffflags will be non-zero)
                    // 3) The shell exiting gracefully
                    //    (Exit status is in lParam.  1 = graceful)
                    // 4) A new user has logged on after the request
                    //    to restart the shell.
                    //    (in the case of autoadminlogon, the new
                    //     user could be logged on before the restart
                    //     request comes through).
                    //

                    if (!pTerm->UserLoggedOn  ||
                        pTerm->LogoffFlags    ||
                        (lParam == 1)            ||
                        (pTerm->TickCount > (DWORD)GetMessageTime())) {

                        break;
                    }

                    SetTimer (hwnd, SHELL_RESTART_TIMER_ID, 2000, NULL);
                    break;

                case LOGON_POWERSTATE:
                    return SleepSystem(pTerm,
                                       hwnd,
                                       (PPOWERSTATEPARAMS)lParam);

                case SESSION_RECONNECTED:
                    if ( !g_Console && gpfnWinStationQueryInformation( SERVERNAME_CURRENT,
                                                     LOGONID_CURRENT,
                                                     WinStationInformation,
                                                     &InfoData,
                                                     sizeof(InfoData),
                                                     &Length ) ) {
                        SetUserEnvironmentVariable(
                            &pTerm->pWinStaWinlogon->UserProcessData.pEnvironment,
                            WINSTATIONNAME_VARIABLE,
                            InfoData.WinStationName, TRUE );
                    }
                    break;


                case LOGON_LOCKWORKSTATION:
                    if (pTerm->UserLoggedOn &&
                        pTerm->Gina.pWlxIsLockOk(pTerm->Gina.pGinaContext) &&
                        (!IsLocked(pTerm->WinlogonState))) {

                        SetActiveDesktop(pTerm, Desktop_Winlogon);
                        DoLockWksta (pTerm, FALSE);
                    }
                    break;
            }

            return(0);


        case WLX_WM_SAS:
            {
                SC_EVENT_TYPE ScEvent ;
                PSC_DATA ScData ;
                //
                // If we got a message like this posted here,
                // it is most likely our own internal events,
                // but make sure:
                //

                switch ( wParam )
                {
                    case WLX_SAS_INTERNAL_SC_EVENT:
                        break;
                    default:
                        return 0 ;
                }


                if ( ScRemoveEvent( &ScEvent, &ScData ) )
                {
                    if ( pTerm->CurrentScEvent )
                    {
                        ScFreeEventData( (PSC_DATA) pTerm->CurrentScEvent );

                        pTerm->CurrentScEvent = NULL ;
                    }

                    pTerm->CurrentScEvent = ScData ;

                    if ( pTerm->EnableSC )
                    {
                        if ( ScEvent == ScInsert )
                        {
                            SASRouter( pTerm, WLX_SAS_TYPE_SC_INSERT );
                        }
                        else
                        {
                            SASRouter( pTerm, WLX_SAS_TYPE_SC_REMOVE );
                        }
                    }

                }

            }

        case WM_TIMER:
            {
            LONG lResult;
            HKEY hKey;
            BOOL bRestart = TRUE;
            DWORD dwType, dwSize;


            //
            //  Restart the shell
            //

            if (wParam != SHELL_RESTART_TIMER_ID) {
                break;
            }

            KillTimer (hwnd, SHELL_RESTART_TIMER_ID);


            //
            // Check if we should restart the shell
            //

            lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                                    WINLOGON_KEY,
                                    0,
                                    KEY_READ,
                                    &hKey);

            if (lResult == ERROR_SUCCESS) {

                dwSize = sizeof(bRestart);
                RegQueryValueEx (hKey,
                                 TEXT("AutoRestartShell"),
                                 NULL,
                                 &dwType,
                                 (LPBYTE) &bRestart,
                                 &dwSize);

                RegCloseKey (hKey);
            }

            if (!pTerm->UserLoggedOn) {
                bRestart = FALSE;
            }

            if (bRestart) {
                PWCH  pchData;
                PWSTR pszTok;

                DebugLog((DEB_TRACE, "Restarting user's shell.\n"));


                pchData = AllocAndGetPrivateProfileString(APPLICATION_NAME,
                                                          SHELL_KEY,
                                                          TEXT("explorer.exe"),
                                                          NULL);

                if (!pchData) {
                    break;
                }

                wsprintfW (szDesktop, L"%s\\%s", pTerm->pWinStaWinlogon->lpWinstaName,
                           APPLICATION_DESKTOP_NAME);


                pszTok = wcstok(pchData, TEXT(","));
                while (pszTok)
                {
                    if (*pszTok == TEXT(' '))
                    {
                        while (*pszTok++ == TEXT(' '))
                            ;
                    }


                    if (StartApplication(pTerm,
                                    szDesktop,
                                    pTerm->pWinStaWinlogon->UserProcessData.pEnvironment,
                                    pszTok)) {

                        ReportWinlogonEvent(pTerm,
                                EVENTLOG_INFORMATION_TYPE,
                                EVENT_SHELL_RESTARTED,
                                0,
                                NULL,
                                1,
                                pszTok);
                    }

                    pszTok = wcstok(NULL, TEXT(","));
                }

                Free(pchData);
            }

            }
            break;

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);

    }

    return 0L;
}

BOOL bRegisteredDesktopSwitching;
BOOL bRegisteredWinlogonBreakpoint;
BOOL bRegisteredTaskmgr;

/***************************************************************************\
* SASCreate
*
* Does any processing required for WM_CREATE message.
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

BOOL SASCreate(
    HWND hwnd)
{
    // Register the SAS unless we are told not to.


    if (GetProfileInt( APPNAME_WINLOGON, VARNAME_AUTOLOGON, 0 ) != 2) {
        if (!RegisterHotKey(hwnd, 0, MOD_SAS | MOD_CONTROL | MOD_ALT, VK_DELETE)) {
            DebugLog((DEB_ERROR, "failed to register SAS"));
            return(FALSE);   // Fail creation
        }
    }


#if DBG

    //
    // C+A+D + Shift causes a quick reboot
    //

    RegisterHotKey(hwnd, 1, MOD_CONTROL | MOD_ALT | MOD_SHIFT, VK_DELETE);


    //
    // (Ctrl+Alt+Tab) will switch between desktops
    //
    if (GetProfileInt( APPNAME_WINLOGON, VARNAME_ENABLEDESKTOPSWITCHING, 0 ) != 0) {
        if (!RegisterHotKey(hwnd, 2, MOD_CONTROL | MOD_ALT, VK_TAB)) {
            DebugLog((DEB_ERROR, "failed to register desktop switch SAS"));
            bRegisteredDesktopSwitching = FALSE;
        } else {
            bRegisteredDesktopSwitching = TRUE;
        }
    }


    if (WinlogonInfoLevel & DEB_COOL_SWITCH) {
        if (!RegisterHotKey(hwnd, 3, MOD_CONTROL | MOD_ALT | MOD_SHIFT, VK_TAB)) {
            DebugLog((DEB_ERROR, "failed to register breakpoint SAS"));
            bRegisteredWinlogonBreakpoint = FALSE;
        } else {
            bRegisteredWinlogonBreakpoint = TRUE;
        }
    }
#endif

    //
    // (Ctrl+Shift+Esc) will start taskmgr
    //

    if (!RegisterHotKey(hwnd, 4, MOD_CONTROL | MOD_SHIFT, VK_ESCAPE)) {
        DebugLog((DEB_ERROR, "failed to register taskmgr hotkey"));
        bRegisteredTaskmgr = FALSE;
    } else {
        bRegisteredTaskmgr = TRUE;
    }

    return(TRUE);
}

/***************************************************************************\
* SASDestroy
*
* Does any processing required for WM_DESTROY message.
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

BOOL SASDestroy(HWND hwnd)
{
    // Unregister the SAS
    UnregisterHotKey(hwnd, 0);

    if (bRegisteredDesktopSwitching) {
        UnregisterHotKey(hwnd, 2);
    }

#if DBG
    UnregisterHotKey(hwnd, 1);

    if (bRegisteredWinlogonBreakpoint) {
        UnregisterHotKey(hwnd, 3);
    }
#endif

    if (bRegisteredTaskmgr) {
        UnregisterHotKey(hwnd, 4);
    }


    return(TRUE);
}


NTSTATUS
SleepSystem(
    IN PTERMINAL pTerm,
    IN HWND hWnd,
    IN PPOWERSTATEPARAMS pPSP
    )
/*++

Routine Description:

    This routine actually calls the kernel to invoke the sleeping
    state. This is also responsible for displaying the relevant
    sleep progress dialog and locking the desktop on return.

Arguments:

    pTerm - Supplies the current terminal

    hWnd - Supplies the parent HWND

    pPSP - Supplies the power state parameters

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS NtStatus;
    BOOL SwitchDesktop;

    if (pTerm->UserLoggedOn &&
        pTerm->Gina.pWlxIsLockOk(pTerm->Gina.pGinaContext) &&
        (!IsLocked(pTerm->WinlogonState))) {

        SwitchDesktop = TRUE;
        SetActiveDesktop(pTerm, Desktop_Winlogon);
    } else {
        SwitchDesktop = FALSE;
    }

    g_fAllowStatusUI = TRUE;

    //
    // Display the appropriate status message
    //
    if ((pPSP->SystemAction == PowerActionShutdown) ||
        (pPSP->SystemAction == PowerActionShutdownReset) ||
        (pPSP->SystemAction == PowerActionShutdownOff)) {
        StatusMessage (FALSE, 0, IDS_STATUS_SAVING_DATA);
    } else if (pPSP->SystemAction == PowerActionWarmEject) {

        StatusMessage (FALSE, STATUSMSG_OPTION_NOANIMATION, IDS_STATUS_EJECTING);

    } else {
        if (pPSP->MinSystemState == PowerSystemHibernate) {
            StatusMessage (FALSE, STATUSMSG_OPTION_NOANIMATION, IDS_STATUS_HIBERNATE);
        } else {
            StatusMessage (FALSE, STATUSMSG_OPTION_NOANIMATION, IDS_STATUS_STANDBY);
        }
    }


    EnablePrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE);

    NtStatus = NtSetSystemPowerState (pPSP->SystemAction,
                                      pPSP->MinSystemState,
                                      pPSP->Flags);

    EnablePrivilege(SE_SHUTDOWN_PRIVILEGE, FALSE);

    RemoveStatusMessage(TRUE);

    g_fAllowStatusUI = FALSE;


    //
    // See if we should lock the desktop
    //
    if ((NT_SUCCESS(NtStatus)) &&
        (pPSP->Flags & POWER_ACTION_LOCK_CONSOLE) &&
        (pTerm->UserLoggedOn) &&
        (pTerm->Gina.pWlxIsLockOk(pTerm->Gina.pGinaContext)) &&
        (!IsLocked(pTerm->WinlogonState))) {

        //
        // This will ensure the display will be reenabled
        // before the dialog is put up.
        //
        ReplyMessage(NtStatus);

        EnableSasMessages( hWnd, pTerm );

        ReturnFromPowerState = TRUE ;

        DoLockWksta(pTerm, FALSE);

        ReturnFromPowerState = FALSE ;

    } else {
        //
        // If we're going back to full screen mode, give the system a chance
        // to stabilize first.
        //
        if (pPSP->FullScreenMode) {
            ReplyMessage(NtStatus);
            Sleep(3000);
        }

        if (SwitchDesktop) {
            SetActiveDesktop(pTerm, Desktop_Application);
        }
    }

    return (NtStatus);
}
