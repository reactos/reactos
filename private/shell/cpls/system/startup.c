/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    startup.c

Abstract:

    Implements the Startup/Recovery dialog of the System
    Control Panel Applet

Author:

    Eric Flo (ericflo) 19-Jun-1995

Revision History:

    15-Oct-1997 scotthal
        Complete overhaul

--*/
#include <sysdm.h>

//
// Globals for this dialog
//

#define CSEC_START_MAX    9999        // Maximum number of seconds allowed

BOOL g_fStartupInitializing;
BOOL g_fRebootRequired = FALSE;

//
// Help ID's
//

DWORD aStartupHelpIds[] = {
    IDC_STARTUP_SYS_OS,                    (IDH_STARTUP + 0),
    IDC_STARTUP_SYS_ENABLECOUNTDOWN,       (IDH_STARTUP + 1),
    IDC_STARTUP_SYS_SECONDS,               (IDH_STARTUP + 2),
    IDC_STARTUP_SYS_SECONDS_LABEL,         (IDH_STARTUP + 2),
    IDC_STARTUP_CDMP_TXT1,                 (IDH_STARTUP + 3),
    IDC_STARTUP_CDMP_LOG,                  (IDH_STARTUP + 4),
    IDC_STARTUP_CDMP_SEND,                 (IDH_STARTUP + 5),
    IDC_STARTUP_CDMP_FILENAME,             (IDH_STARTUP + 7),
    IDC_STARTUP_CDMP_OVERWRITE,            (IDH_STARTUP + 13),
    IDC_STARTUP_CDMP_AUTOREBOOT,           (IDH_STARTUP + 9),
    IDC_STARTUP_SYSTEM_GRP,                (IDH_STARTUP + 10),
    IDC_STARTUP_SYS_SECSCROLL,             (IDH_STARTUP + 11),
    IDC_STARTUP_CDMP_GRP,                  (IDH_STARTUP + 12),
    IDC_STARTUP_CDMP_DEBUGINFO_GROUP,      (IDH_STARTUP + 14),
    IDC_STARTUP_CDMP_TYPE,                 (IDH_STARTUP + 8),
    IDC_STARTUP_CDMP_FILE_LABEL,           (IDH_STARTUP + 7),
    0, 0
};


INT_PTR
APIENTRY 
StartupDlgProc(
    IN HWND hDlg, 
    IN UINT uMsg, 
    IN WPARAM wParam, 
    IN LPARAM lParam
)
/*++

Routine Description:

    Handles messages sent to the Startup/Recovery dialog box

Arguments:

    hDlg -
        Supplies window handle

    uMsg -
        Supplies message being sent

    wParam -
        Supplies message parameter

    lParam -
        Supplies message parameter

Return Value:

    TRUE if message was handled
    FALSE if message was unhandled

--*/
{
    int iRet;

    // If any controls on this page are touched by the user,
    // enable the "Apply" button.
    if ((WM_COMMAND == uMsg) && (!g_fStartupInitializing)) {
        switch (HIWORD(wParam)) {
            case EN_CHANGE:
            case BN_CLICKED:
            case CBN_SELCHANGE:
                PropSheet_Changed(GetParent(hDlg), hDlg);
                break;

            default:
                break;
        } // switch (HIWORD(wParam))
    } // if ((WM_COMMAND...

    // Call the Core Dump stuff first.  Only handle
    // messages not handled by the Core Dump stuff.
    iRet = CoreDumpDlgProc(hDlg, uMsg, wParam, lParam);

    switch( iRet ) {
        case RET_CONTINUE: {
            break;
        }

        case RET_BREAK: {
            return TRUE;
        }

        case RET_ERROR: {
            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
            return TRUE;
        }

        case RET_CHANGE_NO_REBOOT:
        case RET_NO_CHANGE: {
            break;
        }

        case RET_VIRTUAL_CHANGE:
        case RET_RECOVER_CHANGE:
        case RET_VIRT_AND_RECOVER: {
            g_fRebootRequired = TRUE;

            MsgBoxParam(
                hDlg, 
                SYSTEM + 42, 
                INITS + 1,
                MB_OK | MB_ICONINFORMATION
            );

            break;
        }
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        g_fStartupInitializing = TRUE;
        StartListInit(hDlg, wParam, lParam);
        g_fStartupInitializing = FALSE;
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case IDC_STARTUP_SYS_ENABLECOUNTDOWN:
            if (HIWORD(wParam) == BN_CLICKED) {
                int timeout;

                timeout = IsDlgButtonChecked(hDlg, IDC_STARTUP_SYS_ENABLECOUNTDOWN) ? 0 : 30;
                CheckDlgButton(hDlg, IDC_STARTUP_SYS_ENABLECOUNTDOWN,
                                 (BOOL) timeout);
                SetDlgItemInt(hDlg, IDC_STARTUP_SYS_SECONDS, timeout, FALSE);
                EnableWindow(GetDlgItem (hDlg, IDC_STARTUP_SYS_SECONDS), (BOOL) timeout);
                EnableWindow(GetDlgItem (hDlg, IDC_STARTUP_SYS_SECSCROLL), (BOOL) timeout);

            }
            break;

        case IDOK:
            if (StartListExit(hDlg, wParam, lParam) != RET_BREAK) {
                EndDialog(hDlg, 0);
            } // if

            break;

        case IDCANCEL:

            EndDialog(hDlg, 0);
            break;

        case IDC_STARTUP_SYS_SECONDS:
            if (HIWORD(wParam) == EN_UPDATE) {
                if (!CheckVal(hDlg, IDC_STARTUP_SYS_SECONDS, FORMIN, FORMAX, SYSTEM+4)) {
                    SetDlgItemInt(hDlg, IDC_STARTUP_SYS_SECONDS, FORDEF, FALSE); 
                    SendMessage((HWND) lParam, EM_SETSEL, 0, -1);
                    
                } // if (!CheckVal(...

            } // if (HIWORD(wParam == EN_UPDATE)

            break;

        } // switch (LOWORD(wParam))
        break;

    case WM_DESTROY:
        StartListDestroy(hDlg, wParam, lParam);
        break;

    case WM_HELP:      // F1
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
        (DWORD_PTR) (LPSTR) aStartupHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
        (DWORD_PTR) (LPSTR) aStartupHelpIds);
        break;

    default:
        return FALSE;
    }
        
    return TRUE;
}
