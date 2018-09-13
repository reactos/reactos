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
    IDC_DRIVER_SIGNING,          (IDH_HARDWARE + 6),
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

typedef HRESULT (WINAPI *PFNDRIVERSIGNING)(HWND hwnd, DWORD dwFlags);

BOOL ShowDriverSigning(HWND hDlg)
{
    BOOL bFound = FALSE;
    HMODULE hmod = LoadLibrary(TEXT("sigtab.dll"));
    if (hmod)
    {
        PFNDRIVERSIGNING pfn = (PFNDRIVERSIGNING)GetProcAddress(hmod, "DriverSigningDialog");
        if (pfn)
        {
            bFound = TRUE;
            if (hDlg)
                pfn(hDlg, 0);
        }
        FreeLibrary(hmod);
    }
    return bFound;
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

    if (hIconNew && (HICON)1 != hIconNew) {
        hIconOld = (HICON)SendDlgItemMessage(hDlg, IDC_WIZARD_ICON, STM_SETICON, (WPARAM)hIconNew, 0);
        if(hIconOld) {
            DestroyIcon(hIconOld);
        }
    }

    hIconNew = ExtractIcon(hInstance, DEVMGR_FILENAME, 0);
    if (hIconNew && (HICON)1 != hIconNew) {
        hIconOld = (HICON)SendDlgItemMessage(hDlg, IDC_DEVMGR_ICON, STM_SETICON, (WPARAM)hIconNew, 0);
        if(hIconOld) {
            DestroyIcon(hIconOld);
        }
    }

    if (!ShowDriverSigning(NULL))
        ShowWindow(GetDlgItem(hDlg, IDC_DRIVER_SIGNING), SW_HIDE);
}


INT_PTR
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
            return HardwareHandleCommand(hDlg, wParam, lParam);
            break;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP, (DWORD_PTR) (LPSTR) aHardwareHelpIds);
            break;
    
        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) aHardwareHelpIds);
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
    DWORD_PTR dwResult = 0;

    switch (LOWORD(wParam)) {
        case IDC_WIZARD_START:
            StartHardwareWizard(hDlg);
            break;

        case IDC_DEVMGR_START:
            StartDeviceManager(hDlg);
            break;

        case IDC_DRIVER_SIGNING:
            ShowDriverSigning(hDlg);
            break;

        case IDC_HWPROFILES_START:
            dwResult = DialogBox(
                hInstance,
                MAKEINTRESOURCE(DLG_HWPROFILES),
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

    HourGlass(TRUE);

	DevMgrProc = (PDEVMGR_EXECUTE_PROC) GetProcAddress(hDevMgr, DEVMGR_EXECUTE_PROC_NAME);
	if (DevMgrProc)
	    (*DevMgrProc)(hDlg, hInstance, NULL, SW_NORMAL);
	else
	    MsgBoxParam(hDlg, SYSTEM+53, INITS+1, MB_OK | MB_ICONEXCLAMATION, DEVMGR_FILENAME);

	FreeLibrary(hDevMgr);

    HourGlass(FALSE);

    }
    else
    {
	MsgBoxParam(hDlg, SYSTEM+52, INITS+1, MB_OK | MB_ICONEXCLAMATION, DEVMGR_FILENAME);
    }

}
