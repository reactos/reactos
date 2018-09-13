/*******************************************************************************
*
*  CONNECT.C
*
*     This module contains the GUI Logon connect dialog and supporting
*       dialog routines.
*
*  Copyright Microsoft, 1997
*
*
*******************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <stdio.h>


/*=============================================================================
==   Local Defines
=============================================================================*/


/*=============================================================================
==   External Procedures Defined
=============================================================================*/

BOOL ConnectLogon( PTERMINAL, HWND );
BOOL CtxConnectSession( PVOID );

/*=============================================================================
==   Local Procedures Defined
=============================================================================*/

INT_PTR WINAPI ConnectDlgProc( HWND, UINT, WPARAM, LPARAM );
BOOL ConnectDlgInit( PTERMINAL, HWND, int );
VOID HandleFailedConnect( PTERMINAL, HWND, LPTSTR, ULONG );

/*=============================================================================
==   Procedures used
=============================================================================*/

BOOL EnumerateMatchingUsers( PTERMINAL, PULONG, PWINSTATIONINFORMATION,
                             PWINSTATIONCLIENT );


/*=============================================================================
==   Local data
=============================================================================*/

static int ListCount;

static TCHAR * wszColorDepth[] = {
    TEXT("16"),     //  0
    TEXT("256"),    //  1
    TEXT("64K"),    //  2
    TEXT("16M"),    //  3
    TEXT("-"),      //  4 Unknown Color Mode
};
#define UNDEF_COLOR 4


/******************************************************************************
 *
 *  CtxConnectSession
 *
 *   This connects a logon to an existing session.
 *
 *  ENTRY:
 *     pTerm (input)
 *        pointer to GLOBALS struct
 *     hDlg (input)
 *        handle to logon dialog
 *
 *  EXIT:
 *     TRUE  - if we successfully connected to an existing session
 *     FALSE - otherwise
 *
 *****************************************************************************/

BOOL
CtxConnectSession(
    PTERMINAL pTerm
    )
{
    HWND hDlg = NULL;

    //
    // For non-Console sessions, handle reconnect to disconnected sessions.
    //
    if ( !g_Console ) {

        //
        // Try to reconnect to an existing session.  If successful,
        // we abort this logon attempt and return DLG_USER_LOGOFF.
        //
        if ( ConnectLogon( pTerm, hDlg ) ) {
            return( TRUE );
        }
    }
    return ( FALSE );
}


/******************************************************************************
 *
 *  ConnectLogon
 *
 *   If the logged on user has disconnected session(s) already running, allow
 *   user to connect to one of those rather than continueing to start up a new
 *   one.
 *
 *  ENTRY:
 *     pTerm (input)
 *        pointer to GLOBALS struct
 *     hDlg (input)
 *        handle to logon dialog
 *
 *  EXIT:
 *     TRUE  - if we successfully connected to an existing session
 *     FALSE - otherwise
 *
 *****************************************************************************/

BOOL
ConnectLogon(
    PTERMINAL pTerm,
    HWND hDlg
    )
{
    DLG_RETURN_TYPE Result;

    /*
     * Invoke dialog to display user sessions, if any,
     * and allow user to choose one of those sessions.
     */
    WlxSetTimeout(pTerm, LOGON_TIMEOUT);

    do {

        Result = WlxDialogBoxParam(
                     pTerm,
                     NULL,
                     (LPTSTR)IDD_CONNECT,
                     NULL,
                     ConnectDlgProc,
                     (LPARAM)pTerm
                     );

    } while ( Result == IDRETRY );

    if ( Result != DLG_FAILURE ) {

        /*
         * Connect to existing session.
         */
        if ( gpfnWinStationConnect( SERVERNAME_CURRENT,
                                pTerm->MuGlobals.ConnectToSessionId,
                                g_SessionId,
                                L"",  // password
                                TRUE ) ) {
            return( TRUE );
        }

        /*
         * We failed to connect.  Display an error message to inform
         * user that a new Windows NT sesssion (this one) will be created.
         */
        HandleFailedConnect( pTerm, hDlg, pTerm->pWinStaWinlogon->UserName, pTerm->MuGlobals.ConnectToSessionId );
    }

    /*
     * We did not connect to an existing session, so return FALSE.
     */
    return( FALSE );
}


/******************************************************************************
 *
 *  ConnectDlgProc
 *
 *   Process messages for SessionId connect dialog.
 *
 *  EXIT:
 *    TRUE - if message was processed
 *
 *  DIALOG EXIT:
 *      DLG_FAILURE
 *          No sessions are available to connect to.
 *      other
 *          Another exit code will indicates that the user has chosen a session
 *          to connect to (or there was only one available) or the list box
 *          has been terminated via a timeout.  The Citrix global variable
 *          (pTerm)->MuGlobals.ConnectToSessionId will contain the SessionId
 *          to connect to.
 *
 ******************************************************************************/

INT_PTR WINAPI
ConnectDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PTERMINAL pTerm = (PTERMINAL)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (message) {

        case WM_INITDIALOG:
            pTerm = (PTERMINAL)lParam;
            SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);

            if ( !ConnectDlgInit(pTerm, hDlg, IDC_CONNECTBOX) ) {
#if DBG
DbgPrint("ConnectDlgProc: ConnectDlgInit failed\n");
#endif
                EndDialog(hDlg, DLG_FAILURE);
                return(TRUE);
            }

            /*
             * There is only one selection in the list box.  End the
             * dialog with DLG_SUCCESS to cause connect to the SessionId.
             */
            if ( SendMessage(GetDlgItem(hDlg, IDC_CONNECTBOX),
                             LB_GETCOUNT, 0, 0) == 1 ) {
#if DBG
DbgPrint("ConnectDlgProc: One selection\n");
#endif
                EndDialog(hDlg, IDOK);
                return(TRUE);
            }

            CentreWindow(hDlg);
            return(TRUE);


        case WM_COMMAND:

            /*
             * When the user double-clicks on a session or presses Enter,
             * end the dialog.
             */
            if ( (HIWORD(wParam) == LBN_DBLCLK) ||
                 (LOWORD(wParam) == IDOK) ) {

                WINSTATIONINFORMATION WSInfo;
                ULONG Length;

                /*
                 * Query the selected session to determine that it is
                 * still valid and still disconnected.
                 * It's possible the session may have gone away or has
                 * already been reconnected between the time this dialog
                 * was started and now.
                 */
                if ( !gpfnWinStationQueryInformation( SERVERNAME_CURRENT,
                                                      pTerm->MuGlobals.ConnectToSessionId,
                                                      WinStationInformation,
                                                      &WSInfo,
                                                      sizeof(WINSTATIONINFORMATION),
                                                      &Length ) ||
                     WSInfo.ConnectState != State_Disconnected ) {
                    EndDialog(hDlg, IDRETRY );
                    return(TRUE);
                }

                EndDialog(hDlg, IDOK);
                return(TRUE);
            }

            /*
             * Fetch the SessionId associated with a selected session in the
             * list box.  We do this as selections occur in case the
             * dialog times out, in which case the session to connect to
             * will always match the currently selected session in the list
             * box.
             */
            if ( HIWORD(wParam) == LBN_SELCHANGE ) {

                int LBIndex;
                HWND ListBox = GetDlgItem(hDlg, IDC_CONNECTBOX);

                LBIndex = (int)SendMessage(ListBox, LB_GETCURSEL, 0, 0);
                pTerm->MuGlobals.ConnectToSessionId =
                    (ULONG)SendMessage(ListBox,
                                       LB_GETITEMDATA, LBIndex, 0);
            }
            break;
    }

    // We didn't process this message
    return FALSE;
}


/******************************************************************************
 *
 *  ConnectDlgInit
 *
 *   Initialize the connected SessionId list box.
 *
 *  ENTRY:
 *     pTerm (input)
 *        Pointer to GLOBALS struct, where the user name to match is located
 *        in the standard UserName field.
 *      hDlg (input)
 *          This dialog's window handle.
 *      ListBoxId (input)
 *          The resource ID of the list box control to initialize.
 *
 *  EXIT:
 *      TRUE if there is at least one entry for the user to select in the
 *      connnected SessionId list box; FALSE if no matches (nothing to connect
 *      to).
 *
 ******************************************************************************/

#define LB_TAB_COUNT 4
static int LBTabs[LB_TAB_COUNT] = { 8, 38, 93, 183 };

BOOL
ConnectDlgInit( PTERMINAL pTerm,
                HWND hDlg,
                int ListBoxId )
{
    WINSTATIONINFORMATION WSInfo;
    int LBIndex;
    ULONG Index = 0;
    HWND ListBox = GetDlgItem(hDlg, ListBoxId);
    TCHAR String[1024], DisconnectTime[256], LogonTime[256];
    BOOL fAutoSelectLogon;
    WINSTATIONCLIENT ClientData;
    TCHAR Resolution[32];
    USHORT iColor;
    USHORT Mask;
    FILETIME LocalTime;
    SYSTEMTIME stime;


    /*
     *  Check to see if registry has special AutoSelectLogon flag set
     */
    fAutoSelectLogon = (BOOL)(GetProfileInt( TEXT("Winlogon"), TEXT("AutoSelectLogon"), 0) != 0);

    /*
     * Default the Citrix global ConnectToSessionId to -1 to indicate that
     * no connect logon is available (yet).
     */
    pTerm->MuGlobals.ConnectToSessionId = (ULONG)-1;

    /*
     * Initialize the connected SessionId list box.
     */
    SendMessage(ListBox, LB_RESETCONTENT, 0, 0);
    SendMessage(ListBox, LB_SETTABSTOPS, LB_TAB_COUNT, (LPARAM)LBTabs);
    while ( EnumerateMatchingUsers(pTerm, &Index, &WSInfo, &ClientData) ) {

        if ( ClientData.HRes && ClientData.VRes ) {

            /*
             *  Calculate color index
             */
            for ( iColor = 0, Mask = 1;
                  !(Mask & ClientData.ColorDepth) &&
                   (iColor <= UNDEF_COLOR);
                  Mask <<= 1, iColor++ ) ;

            wsprintf( Resolution, TEXT("%dx%d %s"),
                      ClientData.HRes,
                      ClientData.VRes,
                      wszColorDepth[iColor] );

        } else {
            iColor = UNDEF_COLOR;
            wsprintf( Resolution, TEXT("OEM Driver") );
        }


        if ( FileTimeToLocalFileTime( (FILETIME*)&(WSInfo.LogonTime), &LocalTime ) &&
             FileTimeToSystemTime( &LocalTime, &stime ) ) {

           if (!GetTimeFormatW(GetUserDefaultLCID(),
                                   LOCALE_NOUSEROVERRIDE,
                                   &stime,
                                   NULL,
                                   LogonTime,
                                   256
                                   )) {

               lstrcpy( LogonTime,
                        TEXT("   unknown    ") );
           }
        }
        if ( FileTimeToLocalFileTime( (FILETIME*)&(WSInfo.DisconnectTime), &LocalTime ) &&
             FileTimeToSystemTime( &LocalTime, &stime ) ) {

           if (!GetTimeFormatW(GetUserDefaultLCID(),
                                   LOCALE_NOUSEROVERRIDE,
                                   &stime,
                                   NULL,
                                   DisconnectTime,
                                   256
                                   )) {

               lstrcpy( DisconnectTime,
                        TEXT("   unknown    ") );
           }
        }

        wsprintf( String,
                  TEXT("\t%d\t%s\t%s\t%s"),
                  WSInfo.LogonId,
                  Resolution,
                  LogonTime,
                  (!WSInfo.DisconnectTime.LowPart &&
                   !WSInfo.DisconnectTime.HighPart) ?
                    TEXT("") :
                    DisconnectTime );

        if ( (LBIndex =
            (int)SendMessage(ListBox, LB_ADDSTRING, 0, (LPARAM)String)) < 0 )
            break;

        if ( SendMessage(ListBox, LB_SETITEMDATA,
                         LBIndex, (LPARAM)WSInfo.LogonId) < 0 ) {
            SendMessage(ListBox, LB_DELETESTRING, 0, LBIndex);
            break;
        }

        /*
         * If we haven't yet set the default connect-to SessionId, set
         * to this one.
         */
        if ( pTerm->MuGlobals.ConnectToSessionId == (ULONG)-1 )
            pTerm->MuGlobals.ConnectToSessionId = WSInfo.LogonId;

        /*
         *  If AutoSelectLogon is in effect then we are done
         */
        if ( fAutoSelectLogon )
            break;
    }

    if ( SendMessage(ListBox, LB_GETCOUNT, 0, 0) <= 0 )

        /*
         * Nothing in list box (no matches to current user); return FALSE to
         * cause dialog to exit.
         */
        return(FALSE);

    else {

        /*
         * Select the first item in the list box as default connect target.
         */
        SendMessage(ListBox, LB_SETCURSEL, 0, 0);
        return(TRUE);
    }
}


/******************************************************************************
 *
 *  HandleFailedConnect
 *
 *   Tell the user why a connection to existing SessionId failed.
 *
 *  ENTRY:
 *      hDlg (input)
 *          This dialog's window handle.
 *      pUserName (input)
 *          The user name being logged in.
 *      SessionId (input)
 *          The SessionId that couldn't be connected to.
 *
 *  EXIT:
 *
 ******************************************************************************/

VOID
HandleFailedConnect(
    PTERMINAL pTerm,
    HWND hDlg,
    LPTSTR pUserName,
    ULONG SessionId
    )
{
    DWORD Error;
    TCHAR Title[MAX_STRING_BYTES];
    TCHAR Message[MAX_STRING_BYTES*2];
    TCHAR ErrorStr[MAX_STRING_BYTES];

    Error = GetLastError();
    switch (Error) {

        default:
            LoadString( NULL, IDS_MULTIUSER_UNEXPECTED_CONNECT_FAILURE,
                        Title, MAX_STRING_BYTES );

            FormatMessage(
                   FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, Error, 0, ErrorStr, MAX_STRING_BYTES, NULL );

            _snwprintf( Message, MAX_STRING_BYTES*2, Title,
                         pUserName, SessionId, ErrorStr );

            LoadString( NULL, IDS_MULTIUSER_CONNECT_FAILED,
                        Title, MAX_STRING_BYTES );

            TimeoutMessageBoxlpstr( pTerm,
                                    NULL,
                                    Message,
                                    Title,
                                    MB_OK | MB_ICONEXCLAMATION,
                                    20 );
            break;
    }
}


/******************************************************************************
 *
 *  EnumerateMatchingUsers
 *
 *   Match the current user to all disconnected Logons with the same user.
 *
 *  ENTRY:
 *     pTerm (input)
 *        Pointer to GLOBALS struct, where the user name to match is located
 *        in the standard UserName field.
 *     pIndex (input/output)
 *        Points to index for enumeration.  The variable pointed to must be 0
 *        for the first call, and then must be passed back to
 *        EnumerateMatchingUsers unmodified (ie, as returned by this function)
 *        for subsequent calls.
 *     pWSInfo (output)
 *        Points to a WINSTATIONINFORMATION structure that will be filled
 *        with the matching SessionId session's information structure on match.
 *
 *  EXIT:
 *     TRUE if a match was found; FALSE if no match found.  When FALSE is
 *      returned, the end of enumeration is implied, as well as no match for
 *      the current call.
 *
 *****************************************************************************/

BOOL
EnumerateMatchingUsers( PTERMINAL pTerm,
                        PULONG pIndex,
                        PWINSTATIONINFORMATION pWSInfo,
                        PWINSTATIONCLIENT pClientData )
{
    LOGONID Id;
    ULONG Count, ByteCount, Length;
    WINSTATIONCLIENT CurrentClientData;
    WINSTATIONCONFIG ConfigData;
    PDCONFIG PdConfig;
    WDCONFIG WdConfig;
    PWINDOWSTATION pWS = pTerm->pWinStaWinlogon;

    /*
     * We need the current client data to get the initial program
     * and the serial number.
     */
    if ( !gpfnWinStationQueryInformation( SERVERNAME_CURRENT,
                                      LOGONID_CURRENT,
                                      WinStationClient,
                                      &CurrentClientData,
                                      sizeof(CurrentClientData),
                                      &Length ) ) {
        KdPrint(("MSGINA: EnumerateMatchingUsers could not query current WinStation\n"));
        return FALSE;
    }
#if DBG
DbgPrint("EnumerateMatchingUsers: UserName %ws, Domain %ws\n",pWS->UserName,pWS->Domain);
#endif

    /*
     * Enumerate all WinStations from specified index and check it for match
     * to this user.
     */
    Count = 1;
    ByteCount = sizeof(Id);
    while ( gpfnWinStationEnumerate_Indexed( SERVERNAME_CURRENT, &Count, &Id, &ByteCount, pIndex) ) {

        /*
         * A WinStation was returned; if it is not the current SessionId, open
         * it and check for user match.
         */
        if ( (Id.LogonId != g_SessionId) &&
             (Id.State == State_Disconnected) ) {

            if ( gpfnWinStationQueryInformation( SERVERNAME_CURRENT,
                                             Id.LogonId,
                                             WinStationInformation,
                                             pWSInfo,
                                             sizeof(WINSTATIONINFORMATION),
                                             &Length ) ) {

                /*
                 * If we have a user match and the Logon is disconnected,
                 * query the client information to set flag for Windows or
                 * Text mode and return 'true' for match.
                 */

                if ( !lstrcmpi(pTerm->pWinStaWinlogon->UserName, pWSInfo->UserName) &&
                     !lstrcmpi(pWS->Domain, pWSInfo->Domain) &&
                     (pWSInfo->ConnectState == State_Disconnected) &&
                    gpfnWinStationQueryInformation( SERVERNAME_CURRENT,
                                                Id.LogonId,
                                                WinStationClient,
                                                pClientData,
                                                sizeof(*pClientData),
                                                &Length ) &&
                    gpfnWinStationQueryInformation( SERVERNAME_CURRENT,
                                                Id.LogonId,
                                                WinStationPd,
                                                &PdConfig,
                                                sizeof(PdConfig),
                                                &Length ) &&
                    gpfnWinStationQueryInformation( SERVERNAME_CURRENT,
                                                   Id.LogonId,
                                                   WinStationWd,
                                                   &WdConfig,
                                                   sizeof(WdConfig),
                                                   &Length ) &&
                    gpfnWinStationQueryInformation( SERVERNAME_CURRENT,
                                                Id.LogonId,
                                                WinStationConfiguration,
                                                &ConfigData,
                                                sizeof(ConfigData),
                                                &Length ) ) {


                    /*
                     * Do not list sessions which we are sure to fail to
                     * reconnect to. Those include sessions from an other 
                     * protocol or resolution change to or from resolutionless than 8 bpps.
                     * ColorDepth Value 1 means 16 colors.
                     */

                    if (pClientData->ProtocolType != CurrentClientData.ProtocolType) {
                        continue;
                    }



                    /*
                     * For procols  that support the dynamic resolution change at 
		     * server side (RDP), we want to make sure that we do not take 
		     * resolutions that GDI will fail to switch to. The uggly thing with 
                     * code is its awareness of GDI rule for resolution change.
                     * so it has to keep in sync if these rules change. Currently
                     * the rule is : 'no mode change to or from resolution with
                     * color depth less than 8 bpps'.
                     */
                    if ( (WdConfig.WdFlag & WDF_DYNAMIC_RECONNECT) &&
                         pClientData->ColorDepth != CurrentClientData.ColorDepth ) {
                        if ((pClientData->ColorDepth == 1) || (CurrentClientData.ColorDepth == 1)) {
                            continue;
                        }
                    }

                    /*
                     * If the client requested a particular program,
                     * match him up to an identical disconnected initial
                     * program.
                     */
                    if ( lstrcmpi( pClientData->InitialProgram,
                                  CurrentClientData.InitialProgram ) ) {
                        KdPrint(("MSGINA: Initial program did not match\n"));
                        continue;
                    }

                    KdPrint(("MSGINA: fReconnectSame=%u\n", ConfigData.User.fReconnectSame));

                    /*
                     * If fReconnectSame flag is set then we must reconnect
                     * to same WinStation if async, otherwise we must
                     * have a serial number.  Finally, the serial number,
                     * if present, must match.
                     */

                    if ( ConfigData.User.fReconnectSame )  {
                       if ( PdConfig.Create.PdFlag & PD_SINGLE_INST ) {
                           WINSTATIONNAME WinStationName;

                           gpfnWinStationNameFromSessionId(
                               SERVERNAME_CURRENT,
                               LOGONID_CURRENT,
                               WinStationName
                               );

                           if ( lstrcmpi( WinStationName,
                                               pWSInfo->WinStationName ) ) {
                               KdPrint(("MSGINA: Reconnect to same WinStation failed\n"));
                               continue;
                           }
                       }
                       else {

						    if ( pClientData->SerialNumber != CurrentClientData.SerialNumber){
                               KdPrint(("MSGINA: Serial number mismatch, Reconnect failed\n"));
                               continue;
							}

                       }
                    }

                    return(TRUE);

                }
            }
        }
    }

    /*
     * Return 'no match'.
     */
    return(FALSE);
}


