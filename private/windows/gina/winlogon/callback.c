/*******************************************************************************
*
*   CALLBACK.C
*
*   This module contains the GUI Logon connect dialog and supporting
*       dialog routines.
*
*  Copyright Microsoft, 1997
*
*
*******************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*=============================================================================
==   External Procedures Defined
=============================================================================*/

DLG_RETURN_TYPE CallbackWinStation( HWND, PTERMINAL, PWINSTATIONCONFIG );


/*=============================================================================
==   Local Procedures Defined
=============================================================================*/

INT_PTR WINAPI CallbackDlgProc( HWND, UINT, WPARAM, LPARAM );
INT_PTR WINAPI CallbackWaitDlgProc( HWND, UINT, WPARAM, LPARAM );


/*=============================================================================
==   Procedures used
=============================================================================*/


/******************************************************************************
 *
 *  CallbackWinStation
 *
 *
 *  ENTRY:
 *
 *  EXIT:
 *     nothing
 *
 *****************************************************************************/

int
CallbackWinStation( HWND hDlg,
                    PTERMINAL pTerm,
                    PWINSTATIONCONFIG pConfigData )
{
    HWND hDlgWait;
    BOOL bCallback;
    int  Result;

    //
    // Is roving callback enabled for this WinStation?
    //
    if ( pConfigData->User.Callback == Callback_Roving ) {

        KdPrint(("WINLOGON: Roving callback configured: %S\n", pConfigData->User.CallbackNumber ));

        //
        // Copy callback number and type to pTerm
        //
        pTerm->MuGlobals.Callback = pConfigData->User.Callback;
        lstrcpy( pTerm->MuGlobals.CallbackNumber, pConfigData->User.CallbackNumber );

        //
        // Invoke dialog to display callback information.
        //
        WlxSetTimeout(pTerm, LOGON_TIMEOUT);
        Result = TimeoutDialogBoxParam(
                        pTerm,
                     NULL,
                     (LPTSTR)IDD_CALLBACK,
                     NULL,
                     CallbackDlgProc,
                     (LPARAM)pTerm,
                             pTerm->Gina.cTimeout | TIMEOUT_SS_NOTIFY
                     );
    }

    //
    // Is fixed callback enabled for this WinStation?
    //
    else if ( pConfigData->User.Callback == Callback_Fixed ) {

        KdPrint(("MSGINA: Fixed callback configured: %S\n", pConfigData->User.CallbackNumber ));

        //
        //  Copy callback number and type to pTerm
        //
        pTerm->MuGlobals.Callback = pConfigData->User.Callback;
        lstrcpy( pTerm->MuGlobals.CallbackNumber, pConfigData->User.CallbackNumber );

        //
        //  No number means error
        //
        if ( !lstrlen( pTerm->MuGlobals.CallbackNumber ) ) {

            TCHAR Title[MAX_STRING_BYTES];
            TCHAR Message[MAX_STRING_BYTES];

            LoadString( NULL, IDS_MULTIUSER_NO_CALLBACK_NUMBER,
                        Title, MAX_STRING_BYTES );

            LoadString( NULL, IDS_MULTIUSER_NO_CALLBACK_NUMBER_MESSAGE,
                        Message, MAX_STRING_BYTES );

            TimeoutMessageBoxlpstr( pTerm,
                                    NULL,
                                    Message,
                                    Title,
                                    MB_OK | MB_ICONEXCLAMATION,
                                    60 );

            return( WLX_SAS_ACTION_LOGOFF );
        }

        //
        //  Ok, continue with callback
        //
        Result = WLX_SAS_ACTION_LOGON;
    }

    //
    //  Callback not configured, just continue with logon
    //
    else {
        return( WLX_SAS_ACTION_LOGON );
    }

    KdPrint(("MSGINA: Callback dialog result = %u\n", Result));

    //
    //  Switch on return code
    //
    switch ( Result ) {

        //
        //  SUCCESS means perform callback function
        //
        case WLX_SAS_ACTION_LOGON :


            //
            //  Empty string means just connect, do not callback
            //
            if ( !lstrlen( pTerm->MuGlobals.CallbackNumber ) ) {
                KdPrint(("MSGINA: Skip callback\n", Result));
                break;
            }

            KdPrint(("MSGINA: Perform callback: %S\n", pTerm->MuGlobals.CallbackNumber ));

            //
            //  Create dialog to tell user to wait for callback
            //
            hDlgWait = CreateDialog( NULL,
                                     (LPTSTR)IDD_CALLBACK_WAIT,
                                     hDlg,
                                     CallbackWaitDlgProc );

            //
            //  Try callback
            //
            bCallback = gpfnWinStationCallback( SERVERNAME_CURRENT,
                                                LOGONID_CURRENT,
                                                pTerm->MuGlobals.CallbackNumber );

            //
            //  Rid ourselfs of wait dialog
            //
            if ( hDlgWait )
                DestroyWindow( hDlgWait );

            //
            //  Callback success?
            //
            if ( bCallback )
                break;

            //
            //  Call back failed, fall thru and reset winstation
            //
            KdPrint(("MSGINA: Callback failed (Result=%u)\n", Result));

        //
        //  Any thing else but success is a failure, duh!
        //
        default :

            KdPrint(("MSGINA: Callback canceled, failed or timed out (Result=%u)\n", Result));

            //
            //  Disconnect on broken connection
            //
            gpfnWinStationDisconnect( SERVERNAME_CURRENT, LOGONID_CURRENT, 0 );
            Sleep( 250 );       // BUGBUG

            //
            //  Return logoff return code
            //
            Result = WLX_SAS_ACTION_LOGOFF;
            break;

    }

    return( Result );
}


/******************************************************************************
 *
 *  CallbackDlgProc
 *
 *  Process messages for callback dialog
 *
 *  EXIT:
 *    TRUE - if message was processed
 *
 *  DIALOG EXIT:
 *      DLG_FAILURE
 *          Reset WinStation
 *      DLG_SUCCESS
 *          Connect WinStation
 *
 ******************************************************************************/

INT_PTR WINAPI
CallbackDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    TCHAR    Buffer[MAX_STRING_BYTES];
    PTERMINAL pTerm = (PTERMINAL)GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (message) {

        case WM_INITDIALOG:

            // save pTerm pointer
            SetWindowLongPtr(hDlg, GWLP_USERDATA, (LPARAM)(pTerm = (PTERMINAL)lParam));

            // load caption string and show
            LoadString(NULL, (pTerm->MuGlobals.Callback == Callback_Roving ? IDS_MULTIUSER_CALLBACK_ROVING_CAPTION : IDS_MULTIUSER_CALLBACK_FIXED_CAPTION), Buffer, MAX_STRING_BYTES);
            SetWindowText(hDlg, Buffer);

            // set phone number
            SetDlgItemText(hDlg, IDD_PHONENUMBER, pTerm->MuGlobals.CallbackNumber);

#ifdef notdef
            // do not allow phone number to change on fixed
            if ( pTerm->MuGlobals.Callback == Callback_Fixed ) {
                EnableWindow(GetDlgItem(hDlg, IDD_PHONENUMBER), FALSE);
            }
#endif

            CentreWindow(hDlg);
            return(TRUE);

        case WM_COMMAND:

            if ( (LOWORD(wParam) == IDOK) ) {

                // even if somehow the user manages to change a fixed, do not read it
                if ( pTerm->MuGlobals.Callback == Callback_Roving ) {
                    GetDlgItemText(hDlg, IDD_PHONENUMBER, pTerm->MuGlobals.CallbackNumber,
                                                          sizeof(pTerm->MuGlobals.CallbackNumber) - 1);
                }
                EndDialog(hDlg, DLG_SUCCESS);
                return(TRUE);
            }
            else if ( (LOWORD(wParam) == IDCANCEL) ) {
                EndDialog(hDlg, DLG_FAILURE);
                return(TRUE);
            }
            break;
    }

    // We didn't process this message
    return FALSE;
}


/******************************************************************************
 *
 *  CallbackWaitDlgProc
 *
 *  Process messages for callback wait dialog
 *
 *  EXIT:
 *    TRUE - if message was processed
 *
 *  DIALOG EXIT:
 *
 ******************************************************************************/

INT_PTR WINAPI
CallbackWaitDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    switch (message) {

        case WM_INITDIALOG:

            CentreWindow(hDlg);

            return(TRUE);

        case WM_KILLFOCUS:
        case WM_SETFOCUS:

            return(TRUE);
    }

    // We didn't process this message
    return FALSE;
}
