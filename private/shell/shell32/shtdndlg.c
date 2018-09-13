/*************************************************************************
 Private shell shutdown dialog implementation

  The shell needs a shutdown dialog implementation independent of MSGINA's
  implementation to ensure it will function on NT4 and Win9x machines.

  This version of shutdown should only be called if the LoadLib and
  GetProcAddress for the MSGINA version fail.

  This code should be kept in some type of sync with the code in 
  win/gina/msgina/shtdndlg.c

  -dsheldon 10/27/98
*************************************************************************/

#include "shellprv.h"

#include <windowsx.h>
#include <help.h>

#define MAX_SHTDN_OPTIONS               7

#define MAX_CCH_NAME 64
#define MAX_CCH_DESC 256

typedef struct _SHUTDOWNOPTION
{
    DWORD dwOption;
    TCHAR szName[MAX_CCH_NAME + 1];
    TCHAR szDesc[MAX_CCH_DESC + 1];
} SHUTDOWNOPTION, *PSHUTDOWNOPTION;

typedef struct _SHUTDOWNDLGDATA
{
    SHUTDOWNOPTION rgShutdownOptions[MAX_SHTDN_OPTIONS];
    int cShutdownOptions;
    DWORD dwFlags;
    DWORD dwItemSelect;
    BOOL fEndDialogOnActivate;
} SHUTDOWNDLGDATA, *PSHUTDOWNDLGDATA;

// Internal function prototypes
void SetShutdownOptionDescription(HWND hwndCombo, HWND hwndStatic);

BOOL LoadShutdownOptionStrings(int idStringName, int idStringDesc, 
                               PSHUTDOWNOPTION pOption);

BOOL BuildShutdownOptionArray(DWORD dwItems, LPCTSTR szUsername,
                              PSHUTDOWNDLGDATA pdata);

BOOL Shutdown_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);

DWORD GetOptionSelection(HWND hwndCombo);

void SetShutdownOptionDescription(HWND hwndCombo, HWND hwndStatic);

BOOL Shutdown_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

BOOL_PTR CALLBACK Shutdown_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam);

// Copied function implementations and constants
// A function copied from gina that shutdown dialog needs.
/*-----------------------------------------------------------------------------
/ MoveControls
/ ------------
/   Load the image and add the control to the dialog.
/
/ In:
/   hWnd = window to move controls in
/   aID, cID = array of control ids to be moved
/   dx, dy = deltas to apply to controls
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
VOID MoveControls(HWND hWnd, UINT* aID, INT cID, INT dx, INT dy, BOOL fSizeWnd)
{
    RECT rc;

    while ( --cID >= 0 )
    {
        HWND hWndCtrl = GetDlgItem(hWnd, aID[cID]);

        if ( hWndCtrl )
        {
            GetWindowRect(hWndCtrl, &rc);
            MapWindowPoints(NULL, hWnd, (LPPOINT)&rc, 2);
            OffsetRect(&rc, dx, dy);
            SetWindowPos(hWndCtrl, NULL, rc.left, rc.top, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
        }
    }

    if ( fSizeWnd )
    {
        GetWindowRect(hWnd, &rc);
        MapWindowPoints(NULL, GetParent(hWnd), (LPPOINT)&rc, 2);
        SetWindowPos(hWnd, NULL,
                     0, 0, (rc.right-rc.left)+dx, (rc.bottom-rc.top)+dy,
                     SWP_NOZORDER|SWP_NOMOVE);
    }
}

/****************************************************
 Option flags (dwFlags)
 ----------------------
****************************************************/
#define SHTDN_NOHELP                    0x000000001
#define SHTDN_NOPALETTECHANGE           0x000000002

// Shutdown reg value name
#define SHUTDOWN_SETTING TEXT("Shutdown Setting")

// Da code
// -------

BOOL LoadShutdownOptionStrings(int idStringName, int idStringDesc, 
                               PSHUTDOWNOPTION pOption)
{
    BOOL fSuccess = (LoadString(HINST_THISDLL, idStringName, pOption->szName,
        ARRAYSIZE(pOption->szName)) != 0);

    fSuccess &= (LoadString(HINST_THISDLL, idStringDesc, pOption->szDesc,
        ARRAYSIZE(pOption->szDesc)) != 0);

    return fSuccess;
}

BOOL BuildShutdownOptionArray(DWORD dwItems, LPCTSTR szUsername,
                              PSHUTDOWNDLGDATA pdata)
{
    BOOL fSuccess = TRUE;
    pdata->cShutdownOptions = 0;

    if (dwItems & SHTDN_LOGOFF)
    {
        pdata->rgShutdownOptions[pdata->cShutdownOptions].dwOption = SHTDN_LOGOFF;

        // Note that logoff is a special case: format using a user name ala
        // "log off <username>".
        fSuccess &= LoadShutdownOptionStrings(IDS_LOGOFF_NAME, 
            IDS_LOGOFF_DESC, 
            &(pdata->rgShutdownOptions[pdata->cShutdownOptions]));

        if (fSuccess)
        {
            TCHAR szTemp[ARRAYSIZE(pdata->rgShutdownOptions[pdata->cShutdownOptions].szName)];

            if (szUsername != NULL)
            {
                wnsprintf(szTemp, ARRAYSIZE(szTemp),
                    pdata->rgShutdownOptions[pdata->cShutdownOptions].szName,
                    szUsername);
            }
            else
            {
                wnsprintf(szTemp, ARRAYSIZE(szTemp),
                    pdata->rgShutdownOptions[pdata->cShutdownOptions].szName,
                    TEXT(""));
            }

            // Now we have the real logoff title in szTemp; copy is back
            lstrcpy(pdata->rgShutdownOptions[pdata->cShutdownOptions].szName,
                szTemp); 

            // Success!
            pdata->cShutdownOptions ++;
        }

    }

    if (dwItems & SHTDN_SHUTDOWN)
    {
        pdata->rgShutdownOptions[pdata->cShutdownOptions].dwOption = SHTDN_SHUTDOWN;
        fSuccess &= LoadShutdownOptionStrings(IDS_SHUTDOWN_NAME, 
            IDS_SHUTDOWN_DESC, 
            &(pdata->rgShutdownOptions[pdata->cShutdownOptions ++]));
    }

    if (dwItems & SHTDN_RESTART)
    {
        pdata->rgShutdownOptions[pdata->cShutdownOptions].dwOption = SHTDN_RESTART;
        fSuccess &= LoadShutdownOptionStrings(IDS_RESTART_NAME, 
            IDS_RESTART_DESC, 
            &(pdata->rgShutdownOptions[pdata->cShutdownOptions ++]));
    }

    if (dwItems & SHTDN_RESTART_DOS)
    {
        pdata->rgShutdownOptions[pdata->cShutdownOptions].dwOption = SHTDN_RESTART_DOS;
        fSuccess &= LoadShutdownOptionStrings(IDS_RESTARTDOS_NAME, 
            IDS_RESTARTDOS_DESC, 
            &(pdata->rgShutdownOptions[pdata->cShutdownOptions ++]));
    }

    if (dwItems & SHTDN_SLEEP)
    {
        pdata->rgShutdownOptions[pdata->cShutdownOptions].dwOption = SHTDN_SLEEP;
        fSuccess &= LoadShutdownOptionStrings(IDS_SLEEP_NAME, 
            IDS_SLEEP_DESC, 
            &(pdata->rgShutdownOptions[pdata->cShutdownOptions ++]));
    }

    if (dwItems & SHTDN_SLEEP2)
    {
        pdata->rgShutdownOptions[pdata->cShutdownOptions].dwOption = SHTDN_SLEEP2;
        fSuccess &= LoadShutdownOptionStrings(IDS_SLEEP2_NAME, 
            IDS_SLEEP2_DESC, 
            &(pdata->rgShutdownOptions[pdata->cShutdownOptions ++]));
    }

    if (dwItems & SHTDN_HIBERNATE)
    {
        pdata->rgShutdownOptions[pdata->cShutdownOptions].dwOption = SHTDN_HIBERNATE;
        fSuccess &= LoadShutdownOptionStrings(IDS_HIBERNATE_NAME, 
            IDS_HIBERNATE_DESC, 
            &(pdata->rgShutdownOptions[pdata->cShutdownOptions ++]));
    }

    return fSuccess;
}

BOOL Shutdown_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    PSHUTDOWNDLGDATA pdata = (PSHUTDOWNDLGDATA) lParam;
    HWND hwndCombo;
    int iOption;
    int iComboItem;
    
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) lParam);

    // Hide the help button and move over OK and Cancel if applicable
    if (pdata->dwFlags & SHTDN_NOHELP)
    {
        static UINT rgidNoHelp[] = {IDOK, IDCANCEL};
        RECT rc1, rc2;
        int dx;
        HWND hwndHelp = GetDlgItem(hwnd, IDHELP);

        EnableWindow(hwndHelp, FALSE);
        ShowWindow(hwndHelp, SW_HIDE);

        GetWindowRect(hwndHelp, &rc1);
        GetWindowRect(GetDlgItem(hwnd, IDCANCEL), &rc2);

        dx = rc1.left - rc2.left;

        MoveControls(hwnd, rgidNoHelp, ARRAYSIZE(rgidNoHelp), dx, 0, FALSE);
    }

    // Add the items specified to the combo box
    hwndCombo = GetDlgItem(hwnd, IDC_EXITOPTIONS_COMBO);

    for (iOption = 0; iOption < pdata->cShutdownOptions; iOption ++)
    {
        // Add the option
        iComboItem = ComboBox_AddString(hwndCombo, 
            pdata->rgShutdownOptions[iOption].szName);

        if (iComboItem != (int) CB_ERR)
        {
            // Store a pointer to the option
            ComboBox_SetItemData(hwndCombo, iComboItem, 
                &(pdata->rgShutdownOptions[iOption]));

            // See if we should select this option
            if (pdata->rgShutdownOptions[iOption].dwOption == pdata->dwItemSelect)
            {
                ComboBox_SetCurSel(hwndCombo, iComboItem);
            }
        }
    }

    // If we don't have a selection in the combo, do a default selection
    if (ComboBox_GetCurSel(hwndCombo) == CB_ERR)
    {
        ComboBox_SetCurSel(hwndCombo, 0);
    }

    SetShutdownOptionDescription(hwndCombo, 
        GetDlgItem(hwnd, IDC_EXITOPTIONS_DESCRIPTION));

    // If we get an activate message, dismiss the dialog, since we just lost
    // focus
    pdata->fEndDialogOnActivate = TRUE;

    return TRUE;
}

DWORD GetOptionSelection(HWND hwndCombo)
{
    DWORD dwResult;
    PSHUTDOWNOPTION pOption;
    int iItem = ComboBox_GetCurSel(hwndCombo);

    if (iItem != (int) CB_ERR)
    {
        pOption = (PSHUTDOWNOPTION) ComboBox_GetItemData(hwndCombo, iItem);
        dwResult = pOption->dwOption;
    }
    else
    {
        dwResult = SHTDN_NONE;
    }

    return dwResult;
}

void SetShutdownOptionDescription(HWND hwndCombo, HWND hwndStatic)
{
    int iItem;
    PSHUTDOWNOPTION pOption;

    iItem = ComboBox_GetCurSel(hwndCombo);

    if (iItem != CB_ERR)
    {
        pOption = (PSHUTDOWNOPTION) ComboBox_GetItemData(hwndCombo, iItem);

        SetWindowText(hwndStatic, pOption->szDesc);
    }
}

BOOL Shutdown_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    BOOL fHandled = FALSE;
    DWORD dwDlgResult;
    PSHUTDOWNDLGDATA pdata = (PSHUTDOWNDLGDATA) 
        GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (id)
    {
    case IDOK:
        dwDlgResult = GetOptionSelection(GetDlgItem(hwnd, IDC_EXITOPTIONS_COMBO));
        if (dwDlgResult != SHTDN_NONE)
        {
            pdata->fEndDialogOnActivate = FALSE;
            fHandled = TRUE;
            EndDialog(hwnd, (int) dwDlgResult);
        }
        break;
    case IDCANCEL:
        pdata->fEndDialogOnActivate = FALSE;
        EndDialog(hwnd, (int) SHTDN_NONE);
        fHandled = TRUE;
        break;
    case IDC_EXITOPTIONS_COMBO:
        if (codeNotify == CBN_SELCHANGE)
        {
            SetShutdownOptionDescription(hwndCtl, 
                GetDlgItem(hwnd, IDC_EXITOPTIONS_DESCRIPTION));
            fHandled = TRUE;
        }
        break;
    case IDHELP:
        WinHelp(hwnd, TEXT("windows.hlp>proc4"), HELP_CONTEXT, (DWORD) IDH_TRAY_SHUTDOWN_HELP);
        break;
    }
    return fHandled;
}

BOOL_PTR CALLBACK Shutdown_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, Shutdown_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, Shutdown_OnCommand);
        case WM_INITMENUPOPUP:
        {
            EnableMenuItem((HMENU)wParam, SC_MOVE, MF_BYCOMMAND|MF_GRAYED);
        }
        break;
        case WM_SYSCOMMAND:
            // Blow off moves (only really needed for 32bit land).
            if ((wParam & ~0x0F) == SC_MOVE)
                return TRUE;
            break;
        case WM_ACTIVATE:
            // If we're loosing the activation for some other reason than
            // the user click OK/CANCEL then bail.
            if (LOWORD(wParam) == WA_INACTIVE)
            {
                PSHUTDOWNDLGDATA pdata = (PSHUTDOWNDLGDATA) GetWindowLongPtr(hwnd, GWLP_USERDATA);

                if (pdata->fEndDialogOnActivate)
                {
                    pdata->fEndDialogOnActivate = FALSE;
                    EndDialog(hwnd, SHTDN_NONE);
                }
            }
            break;
    }

    return FALSE;
}

/****************************************************************************
 ShutdownDialog
 --------------

  Launches the shutdown dialog. 
  
  hWlx and pfnWlxDialogBoxParam MUST be null for this shell-only version

  Other flags are listed in shtdnp.h.
****************************************************************************/
DWORD ShutdownDialog(HWND hwndParent, DWORD dwItems, DWORD dwItemSelect,
                     LPCTSTR szUsername, DWORD dwFlags, void* hWlx, 
                     void* pfnWlxDialogBoxParam)
{
    // Array of shutdown options - the dialog data
    SHUTDOWNDLGDATA data;
    DWORD dwResult;

    // Set the flags
    data.dwFlags = dwFlags;

    // Set the initially selected item
    data.dwItemSelect = dwItemSelect;

    // Read in the strings for the shutdown option names and descriptions
    if (BuildShutdownOptionArray(dwItems, szUsername, &data))
    {
        // Display the dialog and return the user's selection

        // ..if the caller wants, use a Wlx dialog box function
        if ((hWlx != NULL) || (pfnWlxDialogBoxParam != NULL))
        {
            // Error; winlogon should never call this
            // cheesy shell version of shutdown
            dwResult = SHTDN_NONE;
        }
        else
        {
            // Use standard dialog box
            dwResult = (DWORD) DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(IDD_EXITWINDOWS_DIALOG), hwndParent,
                Shutdown_DialogProc, (LPARAM) &data);
        }
    }
    else
    {
        dwResult = SHTDN_NONE;
    }

    return dwResult;
}

DWORD DownlevelShellShutdownDialog(HWND hwndParent, DWORD dwItems, LPCTSTR szUsername)
{
    DWORD dwSelect;
    DWORD dwDialogResult;
    
    HKEY hkeyShutdown;
    DWORD dwType;
    DWORD dwDisposition;
    LONG lResult;

    // get the User's last selection.
    lResult = RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER,
                0, 0, 0, KEY_READ, NULL, &hkeyShutdown, &dwDisposition);

    if (lResult == ERROR_SUCCESS) 
    {
        DWORD cbData = sizeof(dwSelect);
        lResult = SHQueryValueEx(hkeyShutdown, SHUTDOWN_SETTING,
            0, &dwType, (LPBYTE)&dwSelect, &cbData);
        RegCloseKey(hkeyShutdown);
    }

    if (dwSelect == SHTDN_NONE)
    {
        dwSelect = SHTDN_SHUTDOWN;
    }

    dwDialogResult = ShutdownDialog(hwndParent, dwItems, dwSelect,
        szUsername, SHTDN_NOPALETTECHANGE, NULL, NULL);

    if (dwDialogResult != SHTDN_NONE)
    {
        // Save back the user's choice to the registry
        if (RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER,
            0, 0, 0, KEY_WRITE, NULL, &hkeyShutdown, &dwDisposition) == ERROR_SUCCESS) 
        {
            RegSetValueEx(hkeyShutdown, SHUTDOWN_SETTING,
                0, REG_DWORD, (LPBYTE)&dwDialogResult, sizeof(dwDialogResult));
            RegCloseKey(hkeyShutdown);
        }
    }

    return dwDialogResult;
}
