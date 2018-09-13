/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    advanced.c

Abstract:

    Implements the Advanced tab of the System Control Panel Applet.

Author:

    Scott Hallock (scotthal) 15-Oct-1997

--*/
#include "sysdm.h"

//
// Help IDs
//
DWORD aAdvancedHelpIds[] = {
    IDC_ADV_PERF_ICON,             (IDH_ADVANCED + 0),
    IDC_ADV_PERF_TEXT,             (IDH_ADVANCED + 0),
    IDC_ADV_PERF_BTN,              (IDH_ADVANCED + 1),
    IDC_ADV_ENV_ICON,              (IDH_ADVANCED + 2),
    IDC_ADV_ENV_TEXT,              (IDH_ADVANCED + 2),
    IDC_ADV_ENV_BTN,               (IDH_ADVANCED + 3),
    IDC_ADV_RECOVERY_ICON,         (IDH_ADVANCED + 4),
    IDC_ADV_RECOVERY_TEXT,         (IDH_ADVANCED + 4),
    IDC_ADV_RECOVERY_BTN,          (IDH_ADVANCED + 5),
    0, 0
};
//
// Private function prototypes
//
BOOL
AdvancedHandleCommand(
    IN HWND hDlg,
    IN WPARAM wParam,
    IN LPARAM lParam
);

BOOL
AdvancedHandleNotify(
    IN HWND hDlg,
    IN WPARAM wParam,
    IN LPARAM lParam
);


HPROPSHEETPAGE
CreateAdvancedPage(
    IN HINSTANCE hInst
)
/*++

Routine Description:

    Creates and initializes the Advanced page

Arguments:

    hInst -
        Supplies the instance handle of the applet

Return Value:

    Valid HPROPSHEETPAGE if successful
    NULL if an error occurs

--*/
{
    PROPSHEETPAGE psp;

    ZeroMemory(
        (LPVOID) &psp,
        (DWORD) sizeof(PROPSHEETPAGE)
    );

    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = hInst;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_ADVANCED);
    psp.pszTitle = NULL;
    psp.pfnDlgProc = AdvancedDlgProc;
    psp.lParam = 0;

    return(CreatePropertySheetPage(&psp));

}


BOOL
APIENTRY
AdvancedDlgProc(
    IN HWND hDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
)
/*++

Routine Description:

    Handles messages sent to the Advanced page

Arguments:

    hDlg -
        Window handle

    uMsg -
        Message being sent

    wParam -
        Message parameter

    lParam -
        Message parameter

Return Value:

    TRUE if message was handled
    FALSE if message was unhandled

--*/
{

    switch (uMsg) {
        case WM_COMMAND:
            return(AdvancedHandleCommand(hDlg, wParam, lParam));
            break;

        case WM_NOTIFY:
            return(AdvancedHandleNotify(hDlg, wParam, lParam));
            break;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP, (DWORD) (LPSTR) aAdvancedHelpIds);
            break;
    
        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU, (DWORD) (LPSTR) aAdvancedHelpIds);
            break;

        default:
            return(FALSE);
    } // switch

    return(TRUE);

}


BOOL
AdvancedHandleCommand(
    IN HWND hDlg,
    IN WPARAM wParam,
    IN LPARAM lParam
)
/*++

Routine Description:

    Handles WM_COMMAND messages sent to Advanced tab

Arguments:

    hDlg -
        Supplies window handle

    wParam -
        Supplies message parameter

    lParam -
        Supplies message parameter

Return Value:

    TRUE if message was handled
    FALSE if message was unhandled

--*/
{
    DWORD dwResult = 0;

    switch (LOWORD(wParam)) {
        case IDC_ADV_PERF_BTN:
            dwResult = DialogBox(
                hInstance,
                (LPTSTR) MAKEINTRESOURCE(IDD_PERFORMANCE),
                hDlg,
                PerformanceDlgProc
            );

            break;

        case IDC_ADV_ENV_BTN:
            dwResult = DialogBox(
                hInstance,
                (LPTSTR) MAKEINTRESOURCE(IDD_ENVVARS),
                hDlg,
                EnvVarsDlgProc
            );

            break;

        case IDC_ADV_RECOVERY_BTN:
            dwResult = DialogBox(
                hInstance,
                (LPTSTR) MAKEINTRESOURCE(IDD_STARTUP),
                hDlg,
                StartupDlgProc
            );

            break;

        default:
            return(FALSE);
    } // switch

    return(TRUE);

}


BOOL
AdvancedHandleNotify(
    IN HWND hDlg,
    IN WPARAM wParam,
    IN LPARAM lParam
)
/*++

Routine Description:

    Handles WM_NOTIFY messages sent to Advanced tab

Arguments:

    hDlg -
        Supplies window handle

    wParam -
        Supplies message parameter

    lParam -
        Supplies message parameter

Return Value:

    TRUE if message was handled
    FALSE if message was unhandled

--*/
{
    LPNMHDR pnmh = (LPNMHDR) lParam;
    LPPSHNOTIFY psh = (LPPSHNOTIFY) lParam;

    switch (pnmh->code) {
        case PSN_APPLY:
            //
            // If the user is pressing "OK" and a reboot is required,
            // send the PSM_REBOOTSYSTEM message.
            //
            if ((psh->lParam) && g_fRebootRequired) {
                PropSheet_RebootSystem(GetParent(hDlg));
            } // if

            break;

        default:
            return(FALSE);

    } // switch

    return(TRUE);
}
