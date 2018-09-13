/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    hardware.c

Abstract:

    Implements the Hardware tab of the System Control Panel Applet

Author:

    William Hsieh (williamh) 03-Jul-1997

Revision History:

    17-Oct-1997 scotthal
        Complete overhaul

--*/
#include "sysdm.h"

//
// Help IDs
//
DWORD aHardwareHelpIds[] = {
    IDC_WIZARD_ICON,           (IDH_HARDWARE + 0),
    IDC_WIZARD_TEXT,           (IDH_HARDWARE + 0),
    IDC_WIZARD_START,          (IDH_HARDWARE + 1),
    IDC_DEVMGR_ICON,           (IDH_HARDWARE + 2),
    IDC_DEVMGR_TEXT,           (IDH_HARDWARE + 2),
    IDC_DEVMGR_START,          (IDH_HARDWARE + 3),
    IDC_HWPROFILES_ICON,       (IDH_HARDWARE + 4),
    IDC_HWPROFILES_START_TEXT, (IDH_HARDWARE + 4),
    IDC_HWPROFILES_START,      (IDH_HARDWARE + 5),
    0, 0
};

//
// Function prototypes
//
void 
InitHardwareDlg(
    IN HWND hDlg
);

BOOL
HardwareHandleCommand(
    IN HWND hDlg,
    IN WPARAM wParam,
    IN LPARAM lParam
);

void 
StartHardwareWizard(
    IN HWND hDlg
);

void 
StartDeviceManager(
    IN HWND hDlg
);


HPROPSHEETPAGE 
CreateHardwarePage(
    IN HINSTANCE hInst
)
/*++

Routine Description:

    Creates the hardware page

Arguments:

    hInst -
        Supplies instance handle of applet

Return Value:

    Valid HPROPSHEETPAGE if successful
    NULL if unsuccessful

--*/
{
    PROPSHEETPAGE psp;

    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = 0;
    psp.hInstance = hInst;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_HARDWARE);
    psp.pfnDlgProc = HardwareDlgProc;
    psp.pszTitle = NULL;
    psp.lParam = 0;

    return CreatePropertySheetPage(&psp);
}

VOID 
InitHardwareDlg(
    IN HWND hDlg
)
/*++

Routine Description:

    Initialize the hardware page

Arguments:

    hDlg -
        Supplies the window handle

Return Value:

    None

--*/
{


    HICON hIconNew;
    HICON hIconOld;

    hIconNew = ExtractIcon(hInstance, WIZARD_FILENAME, 0);

    if (hIconNew && 1 != (int)hIconNew) {
        hIconOld = (HICON)SendDlgItemMessage(hDlg, IDC_WIZARD_ICON, STM_SETICON, (WPARAM)hIconNew, 0);
        if(hIconOld) {
            DestroyIcon(hIconOld);
        }
    }

    hIconNew = ExtractIcon(hInstance, DEVMGR_FILENAME, 0);
    if (hIconNew && 1 != (int)hIconNew) {
        hIconOld = (HICON)SendDlgItemMessage(hDlg, IDC_DEVMGR_ICON, STM_SETICON, (WPARAM)hIconNew, 0);
        if(hIconOld) {
            DestroyIcon(hIconOld);
        }
    }

}


BOOL 
APIENTRY 
HardwareDlgProc(
    IN HWND hDlg, 
    IN UINT uMsg, 
    IN WPARAM wParam, 
    IN LPARAM lParam
)
/*++

Routine Description:

    Handles messages sent to the hardware tab

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

    switch (uMsg) {
        case WM_INITDIALOG:
            InitHardwareDlg(hDlg);
            break;
    
        case WM_COMMAND:
            return(HardwareHandleCommand(hDlg, wParam, lParam));
            break;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP, (DWORD) (LPSTR) aHardwareHelpIds);
            break;
    
        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU, (DWORD) (LPSTR) aHardwareHelpIds);
            break;

        default:
            return(FALSE);
    } // switch

    return(TRUE);
}


BOOL
HardwareHandleCommand(
    IN HWND hDlg,
    IN WPARAM wParam,
    IN LPARAM lParam
)
/*++

Routine Description:

    Handles WM_COMMAND messages sent to the Hardware tab

Arguments:

    hDlg -
        Supplies the window handle

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
        case IDC_WIZARD_START:
            StartHardwareWizard(hDlg);
            break;

        case IDC_DEVMGR_START:
            StartDeviceManager(hDlg);
            break;

        case IDC_HWPROFILES_START:
            dwResult = DialogBox(
                hInstance,
                (LPTSTR) MAKEINTRESOURCE(DLG_HWPROFILES),
                hDlg,
                HardwareProfilesDlg
            );
            break;
        
        default:
            return(FALSE);
    } // switch

    return(TRUE);
}


void
StartHardwareWizard(
    IN HWND hDlg
)
/*++

Routine Description:

    Start the Hardware wizard

Arguments:

    hDlg -
        Supplies window handle

Return Value:

    None
--*/
{
    SHELLEXECUTEINFO sei;

    memset(&sei, 0, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.hwnd = hDlg;
    sei.lpFile = WIZARD_FILENAME;
    sei.lpParameters = WIZARD_PARAMETERS;
    sei.lpVerb = WIZARD_VERB;
    sei.nShow = SW_NORMAL;
    sei.hInstApp = hInstance;
    if (!ShellExecuteEx(&sei))
    {
	if (ERROR_FILE_NOT_FOUND == GetLastError() ||
	    ERROR_PATH_NOT_FOUND == GetLastError())
	{
	    // reinitialize the contents of the dialog in case
	    // user has fixed the problem
	    InitHardwareDlg(hDlg);
	}
    }
}


void
StartDeviceManager(
    IN HWND hDlg
)
/*++

Routine Description:

    Start Device Manager

Arguments:

    hDlg -
        Supplies window handle

Return Value:

    None

--*/
{
    HINSTANCE hDevMgr;

    PDEVMGR_EXECUTE_PROC    DevMgrProc;
    hDevMgr = LoadLibrary(DEVMGR_FILENAME);
    if (hDevMgr)
    {
	DevMgrProc = (PDEVMGR_EXECUTE_PROC) GetProcAddress(hDevMgr, DEVMGR_EXECUTE_PROC_NAME);
	if (DevMgrProc)
	    (*DevMgrProc)(hDlg, hInstance, NULL, SW_NORMAL);
	else
	    MsgBoxParam(hDlg, SYSTEM+53, INITS+1, MB_OK | MB_ICONEXCLAMATION, DEVMGR_FILENAME);

	FreeLibrary(hDevMgr);
    }
    else
    {
	MsgBoxParam(hDlg, SYSTEM+52, INITS+1, MB_OK | MB_ICONEXCLAMATION, DEVMGR_FILENAME);
    }
}
