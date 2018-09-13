//*************************************************************
//
//  Events.c    -   Routines to handle the event log
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "uenv.h"
#pragma hdrstop

HANDLE  hEventLog = NULL;
TCHAR   EventSourceName[] = TEXT("Userenv");

typedef struct _ERRORSTRUCT {
    DWORD   dwTimeOut;
    LPTSTR  lpErrorText;
} ERRORSTRUCT, *LPERRORSTRUCT;

INT_PTR APIENTRY ErrorDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

//*************************************************************
//
//  InitializeEvents()
//
//  Purpose:    Opens the event log
//
//  Parameters: void
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/17/95     ericflo    Created
//
//*************************************************************

BOOL InitializeEvents (void)
{

    //
    // Open the event source
    //

    hEventLog = RegisterEventSource(NULL, EventSourceName);

    if (hEventLog) {
        return TRUE;
    }

    DebugMsg((DM_WARNING, TEXT("InitializeEvents:  Could not open event log.  Error = %d"), GetLastError()));
    return FALSE;
}

//*************************************************************
//
//  LogEvent()
//
//  Purpose:    Logs an event to the event log
//
//  Parameters: bError      -   Error or informational
//              idMsg       -   Message id
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              2/5/98      ericflo    Created
//
//*************************************************************

int LogEvent (BOOL bError, UINT idMsg, ...)
{
    TCHAR szMsg[MAX_PATH];
    LPTSTR aStrings[2];
    LPTSTR lpErrorMsg;
    PSID pSid = NULL;
    WORD wType;
    va_list marker;
    INT iChars;
    HANDLE hToken = NULL;


    //
    // Check for the event log being open.
    //

    if (!hEventLog) {
        if (!InitializeEvents()) {
            DebugMsg((DM_WARNING, TEXT("LogEvent:  Cannot log event, no handle")));
            return -1;
        }
    }


    //
    // Get the caller's token
    //

    if (!OpenThreadToken (GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_READ | TOKEN_DUPLICATE,
                          TRUE, &hToken)) {
         OpenProcessToken(GetCurrentProcess(), TOKEN_IMPERSONATE | TOKEN_READ | TOKEN_DUPLICATE,
                          &hToken);
    }


    //
    // Get the caller's sid
    //

    if (hToken) {

        pSid = GetUserSid(hToken);

        CloseHandle (hToken);

        if (!pSid) {
            DebugMsg((DM_WARNING, TEXT("LogEvent:  Failed to get the sid")));
        }
    }


    //
    // Load the message
    //

    if (idMsg != 0) {
        if (!LoadString (g_hDllInstance, idMsg, szMsg, ARRAYSIZE(szMsg))) {
            DebugMsg((DM_WARNING, TEXT("LogEvent:  LoadString failed.  Error = %d"), GetLastError()));
            return -1;
        }

    } else {
        lstrcpy (szMsg, TEXT("%s"));
    }


    //
    // Allocate space for the error message
    //

    lpErrorMsg = LocalAlloc (LPTR, (4 * MAX_PATH + 100) * sizeof(TCHAR));

    if (!lpErrorMsg) {
        DebugMsg((DM_WARNING, TEXT("LogEvent:  LocalAlloc failed.  Error = %d"), GetLastError()));
        return -1;
    }


    //
    // Plug in the arguments
    //

    va_start(marker, idMsg);
    iChars = wvsprintf(lpErrorMsg, szMsg, marker);

    DmAssert( iChars < (4 * MAX_PATH + 100));

    va_end(marker);

    //
    // Report the event to the eventlog
    //

    aStrings[0] = lpErrorMsg;

    if (bError) {
        wType = EVENTLOG_ERROR_TYPE;
    } else {
        wType = EVENTLOG_INFORMATION_TYPE;
    }

    if (!ReportEvent(hEventLog, wType, 0, EVENT_ERROR, pSid, 1, 0, aStrings, NULL)) {
        DebugMsg((DM_WARNING,  TEXT("ReportEvent failed.  Error = %d"), GetLastError()));
    }

    LocalFree (lpErrorMsg);

    if (pSid) {
        DeleteUserSid(pSid);
    }

    return 0;
}

//*************************************************************
//
//  ShutdownEvents()
//
//  Purpose:    Stops the event log
//
//  Parameters: void
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/17/95     ericflo    Created
//
//*************************************************************

BOOL ShutdownEvents (void)
{
    BOOL bRetVal = TRUE;

    if (hEventLog) {
        bRetVal = DeregisterEventSource(hEventLog);
        hEventLog = NULL;
    }

    return bRetVal;
}

//*************************************************************
//
//  ReportError()
//
//  Purpose:    Displays an error message to the user and
//              records it in the event log
//
//  Parameters: dwFlags     -   Flags
//              idMsg       -   Error message id
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/18/95     ericflo    Created
//
//*************************************************************

int ReportError (DWORD dwFlags, UINT idMsg, ...)
{
    TCHAR szMsg[MAX_PATH];
    LPTSTR lpErrorMsg;
    va_list marker;
    INT iChars;


    //
    // Load the error message
    //

    if (!LoadString (g_hDllInstance, idMsg, szMsg, ARRAYSIZE(szMsg))) {
        DebugMsg((DM_WARNING, TEXT("RecordEvent:  LoadString failed.  Error = %d"), GetLastError()));
        return -1;
    }


    //
    // Allocate space for the error message
    //

    lpErrorMsg = LocalAlloc (LPTR, (4 * MAX_PATH + 100) * sizeof(TCHAR));

    if (!lpErrorMsg) {
        DebugMsg((DM_WARNING, TEXT("RecordEvent:  LocalAlloc failed.  Error = %d"), GetLastError()));
        return -1;
    }


    //
    // Plug in the arguments
    //

    va_start(marker, idMsg);
    iChars = wvsprintf(lpErrorMsg, szMsg, marker);

    DmAssert( iChars < (4 * MAX_PATH + 100) );

    va_end(marker);

    if (!(dwFlags & PI_NOUI)) {

        ERRORSTRUCT es;
        DWORD dwDlgTimeOut = PROFILE_DLG_TIMEOUT;
        DWORD dwSize, dwType;
        LONG lResult;
        HKEY hKey;

        //
        // Find the dialog box timeout
        //

        lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                               WINLOGON_KEY,
                               0,
                               KEY_READ,
                               &hKey);

        if (lResult == ERROR_SUCCESS) {

            dwSize = sizeof(DWORD);
            RegQueryValueEx (hKey,
                             TEXT("ProfileDlgTimeOut"),
                             NULL,
                             &dwType,
                             (LPBYTE) &dwDlgTimeOut,
                             &dwSize);


            RegCloseKey (hKey);
        }


        lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                               SYSTEM_POLICIES_KEY,
                               0,
                               KEY_READ,
                               &hKey);

        if (lResult == ERROR_SUCCESS) {

            dwSize = sizeof(DWORD);
            RegQueryValueEx (hKey,
                             TEXT("ProfileDlgTimeOut"),
                             NULL,
                             &dwType,
                             (LPBYTE) &dwDlgTimeOut,
                             &dwSize);


            RegCloseKey (hKey);
        }

        //
        // Display the message
        //

        es.dwTimeOut = dwDlgTimeOut;
        es.lpErrorText = lpErrorMsg;

        DialogBoxParam (g_hDllInstance, MAKEINTRESOURCE(IDD_ERROR),
                        NULL, ErrorDlgProc, (LPARAM)&es);
    }



    //
    // Report the event to the eventlog
    //

    LogEvent (TRUE, 0, lpErrorMsg);

    LocalFree (lpErrorMsg);

    return 0;
}


//*************************************************************
//
//  ErrorDlgProc()
//
//  Purpose:    Dialog box procedure for the error dialog
//
//  Parameters: hDlg    -   handle to the dialog box
//              uMsg    -   window message
//              wParam  -   wParam
//              lParam  -   lParam
//
//  Return:     TRUE if message was processed
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              3/22/96     ericflo    Created
//
//*************************************************************

INT_PTR APIENTRY ErrorDlgProc (HWND hDlg, UINT uMsg,
                            WPARAM wParam, LPARAM lParam)
{
    TCHAR szBuffer[10];
    static DWORD dwErrorTime;

    switch (uMsg) {

        case WM_INITDIALOG:
           {
           LPERRORSTRUCT lpES = (LPERRORSTRUCT) lParam;

           CenterWindow (hDlg);
           SetDlgItemText (hDlg, IDC_ERRORTEXT, lpES->lpErrorText);

           dwErrorTime = lpES->dwTimeOut;

           if (dwErrorTime > 0) {
               wsprintf (szBuffer, TEXT("%d"), dwErrorTime);
               SetDlgItemText (hDlg, IDC_TIMEOUT, szBuffer);
               SetTimer (hDlg, 1, 1000, NULL);
           }
           return TRUE;
           }

        case WM_TIMER:

           if (dwErrorTime >= 1) {

               dwErrorTime--;
               wsprintf (szBuffer, TEXT("%d"), dwErrorTime);
               SetDlgItemText (hDlg, IDC_TIMEOUT, szBuffer);

           } else {

               //
               // Time's up.  Dismiss the dialog.
               //

               PostMessage (hDlg, WM_COMMAND, IDOK, 0);
           }
           break;

        case WM_COMMAND:

          switch (LOWORD(wParam)) {

              case IDOK:
                  if (HIWORD(wParam) == BN_KILLFOCUS) {
                      KillTimer (hDlg, 1);
                      ShowWindow(GetDlgItem(hDlg, IDC_TIMEOUT), SW_HIDE);
                      ShowWindow(GetDlgItem(hDlg, IDC_TIMETITLE), SW_HIDE);

                  } else if (HIWORD(wParam) == BN_CLICKED) {
                      KillTimer (hDlg, 1);
                      EndDialog(hDlg, TRUE);
                  }
                  break;

              case IDCANCEL:
                  KillTimer (hDlg, 1);
                  EndDialog(hDlg, FALSE);
                  break;

              default:
                  break;

          }
          break;

    }

    return FALSE;
}
