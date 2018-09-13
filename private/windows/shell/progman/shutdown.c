/****************************** Module Header ******************************\
* Module Name: shutdown.c
*
* Copyright (c) 1992, Microsoft Corporation
*
* Handles shutdown dialog.
*
* History:
* 2-25-92 JohanneC       Created - extracted from old pmdlgs.c with security
*                                  added.
\***************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "progman.h"

#define LOGOFF_SETTING L"Logoff Setting"

INT_PTR WINAPI
ShutdownDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );

VOID
CentreWindow(
    HWND    hwnd
    );


/***************************************************************************\
* ShutdownDialog
*
* Creates the shutdown dialog
*
*
* History:
* 2-28-92  Johannec     Created
*
\***************************************************************************/
BOOL
ShutdownDialog(
    HANDLE hInst,
    HWND hwnd
    )
{
    INT_PTR nResult;
    BOOLEAN WasEnabled;
    NTSTATUS Status;
    TCHAR szMessage[MAXMESSAGELEN];
    TCHAR szTitle[MAXMESSAGELEN];

    Status = RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE,
                                (BOOLEAN)TRUE,
                                (BOOLEAN)FALSE,
                                &WasEnabled);

    if (!NT_SUCCESS(Status)) {

        //
        // We don't have permission to shutdown
        //

        LoadString(hInst, IDS_NO_PERMISSION_SHUTDOWN, szMessage, CharSizeOf(szMessage));
        LoadString(hInst, IDS_SHUTDOWN_MESSAGE, szTitle, CharSizeOf(szTitle));

        nResult = MessageBox(hwnd, szMessage, szTitle, MB_OK | MB_ICONSTOP);

        return(FALSE);
    }

    //
    // Put up the shutdown dialog
    //

    nResult = DialogBox(hInst, (LPTSTR) MAKEINTRESOURCE(IDD_SHUTDOWN_QUERY), hwnd, ShutdownDlgProc);

    //
    // Restore the shutdown privilege state
    //

    if (!WasEnabled) {

        Status = RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE,
                                    (BOOLEAN)WasEnabled,
                                    (BOOLEAN)FALSE,
                                    &WasEnabled);
    }

    return(FALSE);
}


/***************************************************************************\
* FUNCTION: ShutdownDlgProc
*
* PURPOSE:  Processes messages for shutdown confirmation dialog
*
* RETURNS:  if user decided not to shutdown or reboot.
*
* HISTORY:
*
*   05-17-92 Davidc       Created.
*
\***************************************************************************/

INT_PTR WINAPI
ShutdownDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    UINT uiOptions;
static DWORD dwShutdown = 1;
static HKEY hkeyShutdown = NULL;

    switch (message) {

    case WM_INITDIALOG:
        {
        DWORD dwType;
        DWORD cbData;
        DWORD dwDisposition;
        BOOL bPowerdown;

        bPowerdown = GetProfileInt(TEXT("Winlogon"), TEXT("PowerdownAfterShutdown"), 0);
        if (!bPowerdown) {
            ShowWindow(GetDlgItem(hDlg, IDD_POWEROFF), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDOK2), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDCANCEL2), SW_HIDE);
            SendMessage(hDlg, DM_SETDEFID, IDOK, 0);
        }
        else {
            ShowWindow(GetDlgItem(hDlg, IDOK), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDCANCEL), SW_HIDE);
        }

        //
        // Check the button that was the user's last shutdown selection.
        //
        if (RegCreateKeyEx(HKEY_CURRENT_USER, SHUTDOWN_SETTING_KEY, 0, 0, 0,
                     KEY_READ | KEY_WRITE | DELETE,
                     NULL, &hkeyShutdown, &dwDisposition) == ERROR_SUCCESS) {
           cbData = sizeof(dwShutdown);
           RegQueryValueEx(hkeyShutdown, SHUTDOWN_SETTING, 0, &dwType, (LPBYTE)&dwShutdown, &cbData);
        }
        switch(dwShutdown) {
        case DLGSEL_SHUTDOWN_AND_RESTART:
            CheckDlgButton(hDlg, IDD_RESTART, 1);
            break;
        case DLGSEL_SHUTDOWN_AND_POWEROFF:
            if (bPowerdown) {
                CheckDlgButton(hDlg, IDD_POWEROFF, 1);
                break;
            }
            //
            // Fall thru.
            // Default to shutdown.
            //
        default:
            CheckDlgButton(hDlg, IDD_SHUTDOWN, 1);
            break;
        }

        //
        // Position ourselves
        //
        CentreWindow(hDlg);

        return(TRUE);
        }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case IDOK:
        case IDOK2:
            if (IsDlgButtonChecked(hDlg, IDD_SHUTDOWN)) {
                uiOptions = EWX_SHUTDOWN;
                dwShutdown = DLGSEL_SHUTDOWN;
            }
            else if (IsDlgButtonChecked(hDlg, IDD_RESTART)) {
                uiOptions = EWX_SHUTDOWN | EWX_REBOOT;
                dwShutdown = DLGSEL_SHUTDOWN_AND_RESTART;
            }
            else {
                uiOptions = EWX_SHUTDOWN | EWX_POWEROFF;
                dwShutdown = DLGSEL_SHUTDOWN_AND_POWEROFF;
            }

            if (hkeyShutdown) {
                RegSetValueEx(hkeyShutdown, SHUTDOWN_SETTING, 0, REG_DWORD, (LPBYTE)&dwShutdown, sizeof(dwShutdown));
                RegCloseKey(hkeyShutdown);
                hkeyShutdown = NULL;
            }

            ExitWindowsEx(uiOptions, (DWORD)-1);

        case IDCANCEL2:
        case IDCANCEL:
            if (hkeyShutdown) {
                RegCloseKey(hkeyShutdown);
                hkeyShutdown = NULL;
            }
            EndDialog(hDlg, 0);
            return(TRUE);
        }
        break;
    }

    // We didn't process the message
    return(FALSE);
}

/***************************************************************************\
* FUNCTION: NewLogoffDlgProc
*
* PURPOSE:  Processes messages for logoff/shutdown confirmation dialog
*
* RETURNS:  if user decided not to logoff, shutdown or reboot.
*
* HISTORY:
*
*   10-05-93 Johannec       Created.
*
\***************************************************************************/

INT_PTR APIENTRY
NewLogoffDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    UINT uiOptions;
    BOOLEAN WasEnabled = FALSE;
    NTSTATUS Status;
static DWORD dwShutdown = 0;
static HKEY hkeyShutdown = NULL;

    switch (message) {

    case WM_INITDIALOG:
        {
        DWORD dwType;
        BOOL bPowerdown;
        DWORD cbData;
        DWORD dwDisposition;

        bPowerdown = GetProfileInt(TEXT("Winlogon"), TEXT("PowerdownAfterShutdown"), 0);
        if (!bPowerdown) {
            ShowWindow(GetDlgItem(hDlg, IDD_POWEROFF), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDOK2), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDCANCEL2), SW_HIDE);
            SendMessage(hDlg, DM_SETDEFID, IDOK, 0);
        }
        else {
            ShowWindow(GetDlgItem(hDlg, IDOK), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDCANCEL), SW_HIDE);
        }

        //
        // Initial setting is User's last selection
        //
        if (RegCreateKeyEx(HKEY_CURRENT_USER, SHUTDOWN_SETTING_KEY, 0, 0, 0,
                     KEY_READ | KEY_WRITE | DELETE,
                     NULL, &hkeyShutdown, &dwDisposition) == ERROR_SUCCESS) {
           cbData = sizeof(dwShutdown);
           RegQueryValueEx(hkeyShutdown, LOGOFF_SETTING, 0, &dwType, (LPBYTE)&dwShutdown, &cbData);
        }
        switch(dwShutdown) {
        case DLGSEL_SHUTDOWN:
            CheckDlgButton(hDlg, IDD_SHUTDOWN, 1);
            break;
        case DLGSEL_SHUTDOWN_AND_RESTART:
            CheckDlgButton(hDlg, IDD_RESTART, 1);
            break;
        case DLGSEL_SHUTDOWN_AND_POWEROFF:
            if (bPowerdown) {
                CheckDlgButton(hDlg, IDD_POWEROFF, 1);
            }
            else {
                CheckDlgButton(hDlg, IDD_SHUTDOWN, 1);
            }
            break;
        default:
            CheckDlgButton(hDlg, IDD_LOGOFF, 1);
            break;
        }

        //
        // Make sure the user has the privileges to shutdown the computer.
        //

        Status = RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE,
                                    (BOOLEAN)TRUE,
                                    (BOOLEAN)FALSE,
                                    &WasEnabled);

        if (!NT_SUCCESS(Status)) {

            //
            // We don't have permission to shutdown
            //
            EnableWindow(GetDlgItem(hDlg, IDD_SHUTDOWN), 0);
            EnableWindow(GetDlgItem(hDlg, IDD_RESTART), 0);
            EnableWindow(GetDlgItem(hDlg, IDD_POWEROFF), 0);
        }

        //
        // Position ourselves
        //
        CentreWindow(hDlg);

        return(TRUE);
        }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDOK2:
            if (IsDlgButtonChecked(hDlg, IDD_LOGOFF)) {
                uiOptions = EWX_LOGOFF;
                dwShutdown = DLGSEL_LOGOFF;
            }
            else if (IsDlgButtonChecked(hDlg, IDD_SHUTDOWN)) {
                uiOptions = EWX_SHUTDOWN;
                dwShutdown = DLGSEL_SHUTDOWN;
            }
            else if (IsDlgButtonChecked(hDlg, IDD_RESTART)) {
                uiOptions = EWX_SHUTDOWN | EWX_REBOOT;
                dwShutdown = DLGSEL_SHUTDOWN_AND_RESTART;
            }
            else {
                uiOptions = EWX_SHUTDOWN | EWX_POWEROFF;
                dwShutdown = DLGSEL_SHUTDOWN_AND_POWEROFF;
            }

            //
            // Save user's shutdown selection.
            //
            if (hkeyShutdown) {
                RegSetValueEx(hkeyShutdown, LOGOFF_SETTING, 0, REG_DWORD, (LPBYTE)&dwShutdown, sizeof(dwShutdown));
                RegCloseKey(hkeyShutdown);
                hkeyShutdown = NULL;
            }

            //
            // Make sure the user has the privileges to shutdown the computer.
            //

            ExitWindowsEx(uiOptions, (DWORD)-1);
            //
            // Restore the shutdown privilege state
            //

            if (!WasEnabled) {

                Status = RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE,
                                    (BOOLEAN)WasEnabled,
                                    (BOOLEAN)FALSE,
                                    &WasEnabled);
            }


        case IDCANCEL:
        case IDCANCEL2:
            if (hkeyShutdown) {
                RegCloseKey(hkeyShutdown);
                hkeyShutdown = NULL;
            }
            EndDialog(hDlg, 0);
            return(TRUE);
        }
        break;
    }

    // We didn't process the message
    return(FALSE);
}

/***************************************************************************\
* CentreWindow
*
* Purpose : Positions a window so that it is centred in its parent
*
* !!! WARNING this code is duplicated in winlogon\winutil.c
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/
VOID
CentreWindow(
    HWND    hwnd
    )
{
    RECT    rect;
    RECT    rectParent;
    HWND    hwndParent;
    LONG    dx, dy;
    LONG    dxParent, dyParent;
    LONG    Style;

    // Get window rect
    GetWindowRect(hwnd, &rect);

    dx = rect.right - rect.left;
    dy = rect.bottom - rect.top;

    // Get parent rect
    Style = GetWindowLong(hwnd, GWL_STYLE);
    if ((Style & WS_CHILD) == 0) {

        // Return the desktop windows size (size of main screen)
        dxParent = GetSystemMetrics(SM_CXSCREEN);
        dyParent = GetSystemMetrics(SM_CYSCREEN);
    } else {
        hwndParent = GetParent(hwnd);
        if (hwndParent == NULL) {
            hwndParent = GetDesktopWindow();
        }
        GetWindowRect(hwndParent, &rectParent);

        dxParent = rectParent.right - rectParent.left;
        dyParent = rectParent.bottom - rectParent.top;
    }

    // Centre the child in the parent
    rect.left = (dxParent - dx) / 2;
    rect.top  = (dyParent - dy) / 3;

    // Move the child into position
    SetWindowPos(hwnd, NULL, rect.left, rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    SetForegroundWindow(hwnd);
}

INT_PTR APIENTRY ExitDlgProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
 {
     switch (uiMsg) {
     case WM_INITDIALOG:
         CentreWindow(hwnd);
         break;

     case WM_COMMAND:
         switch (LOWORD(wParam)) {

         case IDOK:

             // fall through...

         case IDCANCEL:
             EndDialog(hwnd, LOWORD(wParam) == IDOK);
             break;

         default:
             return(FALSE);
         }
         break;

     default:
         return(FALSE);
     }
     UNREFERENCED_PARAMETER(lParam);
     return(TRUE);
}
