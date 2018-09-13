/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    envvar.c

Abstract:

    Implements the Environment Variables dialog of the System 
    Control Panel Applet

Author:

    Eric Flo (ericflo) 19-Jun-1995

Revision History:

    15-Oct-1997 scotthal
        Complete overhaul

--*/
#include "sysdm.h"

#include <help.h>
// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>


//==========================================================================
//                             Local Definitions
//==========================================================================
#define LB_SYSVAR   1
#define LB_USERVAR  2

#define SYSTEMROOT TEXT("SystemRoot")
#define SYSTEMDRIVE TEXT("SystemDrive")

//==========================================================================
//                            Typedefs and Structs
//==========================================================================

//  Registry valuename linked-list structure
typedef struct _regval
{
    struct _regval *prvNext;
    LPTSTR szValueName;
} REGVAL;


//==========================================================================
//                             Local Functions
//==========================================================================
void EVDoCommand(HWND hDlg, HWND hwndCtl, int idCtl, int iNotify );
void EVSave(HWND hDlg);
void EVCleanUp (HWND hDlg);
PENVAR GetVar(HWND hDlg, UINT VarType, int iSelection);
int  FindVar (HWND hwndLB, LPTSTR szVar);

void
SetVar(
    IN HWND hDlg,
    IN UINT VarType,
    IN LPCTSTR szVarName,
    IN LPCTSTR szVarValue
);

void
DeleteVar(
    IN HWND hDlg,
    IN UINT VarType,
    IN LPCTSTR szVarName
);

//
// New.../Edit... subdialog functions
//
BOOL 
APIENTRY
EnvVarsEditDlg(
    IN HWND hDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
);

void
EVDoEdit(
    IN HWND hWnd,
    IN UINT VarType,
    IN UINT EditType,
    IN int  iSelection
);
 
BOOL 
ExpandSystemVar( 
    IN LPCTSTR pszSrc, 
    OUT LPTSTR pszDst, 
    IN DWORD cchDst 
);

//==========================================================================
//                      "Global" Variables for this page
//==========================================================================
BOOL bEditSystemVars = FALSE;
DWORD cxLBSysVars = 0;
BOOL bUserVars = TRUE;

//
// Help ID's
//

DWORD aEnvVarsHelpIds[] = {
    IDC_STATIC,                   NO_HELP,
    IDC_ENVVAR_SYS_USERGROUP,     NO_HELP,
    IDC_ENVVAR_SYS_LB_SYSVARS,    (IDH_ENV + 0),
    IDC_ENVVAR_SYS_SYSVARS,       (IDH_ENV + 0),
    IDC_ENVVAR_SYS_USERENV,       (IDH_ENV + 2),
    IDC_ENVVAR_SYS_LB_USERVARS,   (IDH_ENV + 2),
    IDC_ENVVAR_SYS_NEWUV,         (IDH_ENV + 7),
    IDC_ENVVAR_SYS_EDITUV,        (IDH_ENV + 8),
    IDC_ENVVAR_SYS_NDELUV,        (IDH_ENV + 9),
    IDC_ENVVAR_SYS_NEWSV,         (IDH_ENV + 10),
    IDC_ENVVAR_SYS_EDITSV,        (IDH_ENV + 11),
    IDC_ENVVAR_SYS_DELSV,         (IDH_ENV + 12),
    0, 0
};

TCHAR szUserEnv[] = TEXT( "Environment" );
TCHAR szSysEnv[]  = TEXT( "System\\CurrentControlSet\\Control\\Session Manager\\Environment" );


BOOL 
InitEnvVarsDlg(
    IN HWND hDlg
)
/*++

Routine Description:

    Initializes the environment variables page

Arguments:

    hDlg -
        Supplies window handle

Return Value:

    TRUE if successful
    FALSE if an error occurs

--*/
{
    TCHAR szBuffer1[200];
    TCHAR szBuffer2[300];
    TCHAR szUserName[MAX_USER_NAME];
    DWORD dwSize = MAX_USER_NAME;
    HWND hwndTemp;
    HKEY hkeyEnv;
    TCHAR  *pszValue;
    HANDLE hKey;
    DWORD dwBufz, dwValz, dwIndex, dwType;
    LONG Error;
    TCHAR   szTemp[MAX_PATH];
    LPTSTR  pszString;
    ENVARS *penvar;
    int     n;
    LV_COLUMN col;
    LV_ITEM item;
    RECT rect;
    int cxFirstCol;


    HourGlass (TRUE);


    //
    // Create the first column
    //

    LoadString (hInstance, SYSTEM + 50, szBuffer1, 200);

    if (!GetClientRect (GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_SYSVARS), &rect)) {
        rect.right = 300;
    }

    cxFirstCol = (int)(rect.right * .3);

    col.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    col.fmt = LVCFMT_LEFT;
    col.cx = cxFirstCol;
    col.pszText = szBuffer1;
    col.iSubItem = 0;

    SendDlgItemMessage (hDlg, IDC_ENVVAR_SYS_LB_SYSVARS, LVM_INSERTCOLUMN,
                        0, (LPARAM) &col);

    SendDlgItemMessage (hDlg, IDC_ENVVAR_SYS_LB_USERVARS, LVM_INSERTCOLUMN,
                        0, (LPARAM) &col);


    //
    // Create the second column
    //

    LoadString (hInstance, SYSTEM + 51, szBuffer1, 200);

    col.cx = rect.right - cxFirstCol - GetSystemMetrics(SM_CYHSCROLL);
    col.iSubItem = 1;

    SendDlgItemMessage (hDlg, IDC_ENVVAR_SYS_LB_SYSVARS, LVM_INSERTCOLUMN,
                        1, (LPARAM) &col);

    SendDlgItemMessage (hDlg, IDC_ENVVAR_SYS_LB_USERVARS, LVM_INSERTCOLUMN,
                        1, (LPARAM) &col);


    ////////////////////////////////////////////////////////////////////
    // Display System Variables from registry in listbox
    ////////////////////////////////////////////////////////////////////

    hwndTemp = GetDlgItem (hDlg, IDC_ENVVAR_SYS_LB_SYSVARS);

    hKey = MemAlloc (LPTR, BUFZ*SIZEOF(TCHAR));
    pszString = (LPTSTR) MemAlloc (LPTR, BUFZ*sizeof(TCHAR));

    bEditSystemVars = FALSE;
    cxLBSysVars = 0;
    hkeyEnv = NULL;

    //  Try to open the System Environment variables area with
    //  Read AND Write permission.  If successful, then we allow
    //  the User to edit them the same as their own variables

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, szSysEnv, 0, KEY_READ | KEY_WRITE, &hkeyEnv) != ERROR_SUCCESS) {

        //  On failure, just try to open it for reading
        if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, szSysEnv, 0, KEY_READ, &hkeyEnv) != ERROR_SUCCESS) {
            hkeyEnv = NULL;
        }

    } else {

        bEditSystemVars = TRUE;
    }

    if (hkeyEnv)
    {
        pszValue = (TCHAR *) hKey;
        dwBufz = ARRAYSIZE(szTemp);
        dwValz = BUFZ * SIZEOF(TCHAR);
        dwIndex = 0;

        //  Read all values until an error is encountered

        while (!RegEnumValue(hkeyEnv,
                             dwIndex++, // Index'th value name/data
                             szTemp,    // Ptr to ValueName buffer
                             &dwBufz,   // Size of ValueName buffer
                             NULL,      // Title index return
                             &dwType,   // Type code of entry
                    (LPBYTE) pszValue,  // Ptr to ValueData buffer
                             &dwValz))  // Size of ValueData buffer
        {
            if ((dwType != REG_SZ) && (dwType != REG_EXPAND_SZ))
                goto SysLoop;

            //
            //  Clip length of returned Environment variable string
            //  to MAX_VALUE_LEN-1, as necessary.
            //

            pszValue[MAX_VALUE_LEN-1] = TEXT('\0');

            ExpandSystemVar (pszValue, pszString, BUFZ);

            penvar = (ENVARS *) MemAlloc (LPTR, SIZEOF(ENVARS));

            penvar->dwType      = dwType;
            penvar->szValueName = CloneString( szTemp );
            penvar->szValue     = CloneString( pszValue );
            penvar->szExpValue  = CloneString( pszString );


            item.mask = LVIF_TEXT | LVIF_PARAM;
            item.iItem = (dwIndex - 1);
            item.iSubItem = 0;
            item.pszText = penvar->szValueName;
            item.lParam = (LPARAM) penvar;

            n = SendMessage (hwndTemp, LVM_INSERTITEM, 0, (LPARAM) &item);

            if (n != -1) {
                item.mask = LVIF_TEXT;
                item.iItem = n;
                item.iSubItem = 1;
                item.pszText = penvar->szExpValue;

                SendMessage (hwndTemp, LVM_SETITEMTEXT, n, (LPARAM) &item);
            }

SysLoop:
            //  Reset vars for next iteration

            dwBufz = ARRAYSIZE(szTemp);
            dwValz = BUFZ * SIZEOF(TCHAR);
        }
        RegCloseKey (hkeyEnv);

    }


    ////////////////////////////////////////////////////////////////////
    //  Display USER variables from registry in listbox
    ////////////////////////////////////////////////////////////////////


    //
    // Set the "User Environments for <username>" string
    //

    if (GetUserName(szUserName, &dwSize) &&
        LoadString (hInstance, IDS_USERENVVARS, szBuffer1, 200)) {

        wsprintf (szBuffer2, szBuffer1, szUserName);
        SetDlgItemText (hDlg, IDC_ENVVAR_SYS_USERGROUP, szBuffer2);
    }


    Error = RegCreateKey (HKEY_CURRENT_USER, szUserEnv, &hkeyEnv);

    if (Error == ERROR_SUCCESS)
    {
        hwndTemp = GetDlgItem (hDlg, IDC_ENVVAR_SYS_LB_USERVARS);

        pszValue = (TCHAR *) hKey;
        dwBufz = ARRAYSIZE(szTemp);
        dwValz = BUFZ * SIZEOF(TCHAR);
        dwIndex = 0;


        //  Read all values until an error is encountered

        while (!RegEnumValue(hkeyEnv,
                             dwIndex++, // Index'th value name/data
                             szTemp,    // Ptr to ValueName buffer
                             &dwBufz,   // Size of ValueName buffer
                             NULL,      // Title index return
                             &dwType,   // Type code of entry
                    (LPBYTE) pszValue,  // Ptr to ValueData buffer
                             &dwValz))  // Size of ValueData buffer
        {
            if ((dwType != REG_SZ) && (dwType != REG_EXPAND_SZ))
                goto UserLoop;

            //
            //  Clip length of returned Environment variable string
            //  to MAX_VALUE_LEN-1, as necessary.
            //

            pszValue[MAX_VALUE_LEN-1] = TEXT('\0');

            ExpandEnvironmentStrings (pszValue, pszString, BUFZ);

            penvar = (ENVARS *) MemAlloc (LPTR, sizeof(ENVARS));

            penvar->dwType      = dwType;
            penvar->szValueName = CloneString (szTemp);
            penvar->szValue     = CloneString (pszValue);
            penvar->szExpValue  = CloneString (pszString);

            item.mask = LVIF_TEXT | LVIF_PARAM;
            item.iItem = (dwIndex - 1);
            item.iSubItem = 0;
            item.pszText = penvar->szValueName;
            item.lParam = (LPARAM) penvar;

            n = SendMessage (hwndTemp, LVM_INSERTITEM, 0, (LPARAM) &item);

            if (n != -1) {
                item.mask = LVIF_TEXT;
                item.iItem = n;
                item.iSubItem = 1;
                item.pszText = penvar->szExpValue;

                SendMessage (hwndTemp, LVM_SETITEMTEXT, n, (LPARAM) &item);
            }

UserLoop:
            //  Reset vars for next iteration

            dwBufz = ARRAYSIZE(szTemp);
            dwValz = BUFZ * SIZEOF(TCHAR);

        }
        RegCloseKey (hkeyEnv);
    }
    else
    {
        //  Report opening USER Environment key
        if (MsgBoxParam (hDlg, SYSTEM+8, INITS+1,
                          MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL)
        {
            //  Free allocated memory since we are returning from here
            MemFree ((LPVOID)hKey);
            MemFree (pszString);

            HourGlass (FALSE);
            return FALSE;
        }
    }

    //
    // Select the first items in the listviews
    // It is important to set the User listview first, and
    // then the system.  When the system listview is set,
    // we will receive a LVN_ITEMCHANGED notification and
    // clear the focus in the User listview.  But when someone
    // tabs to the control the arrow keys will work correctly.
    //

    item.mask = LVIF_STATE;
    item.iItem = 0;
    item.iSubItem = 0;
    item.state = LVIS_SELECTED | LVIS_FOCUSED;
    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

    SendDlgItemMessage (hDlg, IDC_ENVVAR_SYS_LB_USERVARS,
                        LVM_SETITEMSTATE, 0, (LPARAM) &item);

    SendDlgItemMessage (hDlg, IDC_ENVVAR_SYS_LB_SYSVARS,
                        LVM_SETITEMSTATE, 0, (LPARAM) &item);


    // EM_LIMITTEXT of VARIABLE and VALUE editbox

    SendDlgItemMessage (hDlg, IDC_ENVVAR_SYS_VAR, EM_LIMITTEXT, MAX_PATH-1, 0L);
    SendDlgItemMessage (hDlg, IDC_ENVVAR_SYS_VALUE, EM_LIMITTEXT, MAX_VALUE_LEN-1, 0L);

    MemFree ((LPVOID)hKey);
    MemFree (pszString);

    // Set extended LV style for whole line selection
    SendDlgItemMessage(hDlg, IDC_ENVVAR_SYS_LB_SYSVARS, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
    SendDlgItemMessage(hDlg, IDC_ENVVAR_SYS_LB_USERVARS, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

    HourGlass (FALSE);

    //
    // Disable System Var Editing buttons if
    // user is not an administrator
    //
    EnableWindow(
        GetDlgItem(hDlg, IDC_ENVVAR_SYS_NEWSV),
        bEditSystemVars
    );
    EnableWindow(
        GetDlgItem(hDlg, IDC_ENVVAR_SYS_EDITSV),
        bEditSystemVars
    );
    EnableWindow(
        GetDlgItem(hDlg, IDC_ENVVAR_SYS_DELSV),
        bEditSystemVars
    );

    ///////////////////
    // Return succes //
    ///////////////////
    return TRUE;
}


BOOL 
APIENTRY 
EnvVarsDlgProc(
    IN HWND hDlg, 
    IN UINT uMsg, 
    IN WPARAM wParam, 
    IN LPARAM lParam
)
/*++

Routine Description:

    Handles messages sent to the Environment Variables dialog box

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
    INT i = 0;
    HWND hWndTemp = NULL;

    switch (uMsg)
    {
    case WM_INITDIALOG:

        if (!InitEnvVarsDlg(hDlg)) {
            EndDialog (hDlg, 0);
        }
        break;


    case WM_NOTIFY:

        switch (((NMHDR FAR*)lParam)->code)
        {
        case LVN_KEYDOWN:
            switch (((NMHDR FAR*)lParam)->idFrom) {
                case IDC_ENVVAR_SYS_LB_USERVARS:
                    i = IDC_ENVVAR_SYS_NDELUV;
                    break;
                case IDC_ENVVAR_SYS_LB_SYSVARS:
                    i = IDC_ENVVAR_SYS_DELSV;
                    break;
                default:
                    return(FALSE);
                    break;
            } // switch

            hWndTemp = GetDlgItem(hDlg, i);

            if ((VK_DELETE == ((LV_KEYDOWN FAR *) lParam)->wVKey)) {
                if (IsWindowEnabled(hWndTemp)) {
                    SendMessage(
                        hDlg,
                        WM_COMMAND,
                        MAKEWPARAM(i, BN_CLICKED),
                        (LPARAM) hWndTemp
                    );
                } // if (IsWindowEnabled...
                else {
                    MessageBeep(MB_ICONASTERISK);
                } // else
            } // if (VK_DELETE...
            break;

            
        case NM_DBLCLK:
            switch (((NMHDR FAR*)lParam)->idFrom) {
                case IDC_ENVVAR_SYS_LB_USERVARS:
                    i = IDC_ENVVAR_SYS_EDITUV;
                    break;
                case IDC_ENVVAR_SYS_LB_SYSVARS:
                    i = IDC_ENVVAR_SYS_EDITSV;
                    break;
                default:
                    return(FALSE);
                    break;
            } // switch

            hWndTemp = GetDlgItem(hDlg, i);

            if (IsWindowEnabled(hWndTemp)) {
                SendMessage(
                    hDlg,
                    WM_COMMAND,
                    MAKEWPARAM(i, BN_CLICKED),
                    (LPARAM) hWndTemp
                );
            } // if (IsWindowEnabled...
            else {
                MessageBeep(MB_ICONASTERISK);
            } // else
            break;

        default:
            return FALSE;
        }
        break;


    case WM_COMMAND:
        EVDoCommand(hDlg, (HWND)lParam, LOWORD(wParam), HIWORD(wParam));
        break;

    case WM_DESTROY:
        EVCleanUp (hDlg);
        break;

    case WM_HELP:      // F1
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP, (DWORD) (LPSTR) aEnvVarsHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU, (DWORD) (LPSTR) aEnvVarsHelpIds);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

void 
EVDoCommand(
    IN HWND hDlg, 
    IN HWND hwndCtl, 
    IN int idCtl, 
    IN int iNotify 
)
/*++

Routine Description:

    Handles WM_COMMAND messages sent to Environment Variables dialog box

Arguments:

    hDlg -
        Supplies window handle

    hwndCtl -
        Supplies window handle of control which sent the WM_COMMAND

    idCtl -
        Supplies ID of control which sent the WM_COMMAND

    iNotify -
        Supplies notification code

Return Value:

    None

--*/
{
    TCHAR   szTemp[MAX_PATH];
    int     i;
    HWND    hwndTemp;
    PENVAR  penvar;

    switch (idCtl) {
        case IDOK:
            EVSave(hDlg);
            EndDialog(hDlg, 0);
            break;

        case IDCANCEL:
            EndDialog(hDlg, 0);
            break;

        case IDC_ENVVAR_SYS_EDITSV:
            EVDoEdit(
                hDlg, 
                SYSTEM_VAR,
                EDIT_VAR,
                GetSelectedItem(GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_SYSVARS))
            );
            SetFocus(GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_SYSVARS));
            break;

        case IDC_ENVVAR_SYS_EDITUV:
            EVDoEdit(
                hDlg, 
                USER_VAR,
                EDIT_VAR,
                GetSelectedItem(GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_USERVARS))
            );
            SetFocus(GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_USERVARS));
            break;

        case IDC_ENVVAR_SYS_NEWSV:
            EVDoEdit(hDlg, SYSTEM_VAR, NEW_VAR, -1);
            SetFocus(GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_SYSVARS));
            break;

        case IDC_ENVVAR_SYS_NEWUV:
            EVDoEdit(hDlg, USER_VAR, NEW_VAR, -1); 
            SetFocus(GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_USERVARS));
            break;

        case IDC_ENVVAR_SYS_DELSV:
            i = GetSelectedItem(GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_SYSVARS));
            if (-1 != i) {
                penvar = GetVar(hDlg, SYSTEM_VAR, i);
                ASSERT(penvar);
                DeleteVar(hDlg, SYSTEM_VAR, penvar->szValueName);
            } // if
            SetFocus(GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_SYSVARS));
            break;

        case IDC_ENVVAR_SYS_NDELUV:
            i = GetSelectedItem(GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_USERVARS));
            if (-1 != i) {
                penvar = GetVar(hDlg, USER_VAR, i);
                ASSERT(penvar);
                DeleteVar(hDlg, USER_VAR, penvar->szValueName);
            } // if
            SetFocus(GetDlgItem(hDlg, IDC_ENVVAR_SYS_LB_USERVARS));
            break;

        default:
            break;
    } // switch

    return;

}

void
DeleteVar(
    IN HWND hDlg,
    IN UINT VarType,
    IN LPCTSTR szVarName
)
/*++

Routine Description:

    Deletes an environment variable of a given name and type

Arguments:

    hDlg -
        Supplies window handle

    VarType -
        Supplies variable type (user or system)

    szVarName -
        Supplies variable name

Return Value:

    None, although it really should have one someday.

--*/
{
    TCHAR   szTemp2[MAX_PATH];
    int     i, n;
    TCHAR  *bBuffer;
    TCHAR  *pszTemp;
    LPTSTR  pszString;
    HWND    hwndTemp;
    ENVARS *penvar;
    LV_ITEM item;

    // Delete listbox entry that matches value in szVarName
    //  If found, delete entry else ignore
    wsprintf(
        szTemp2,
        TEXT("%s"),
        szVarName
    );

    if (szTemp2[0] == TEXT('\0'))
        return;

    //  Determine which Listbox to use (SYSTEM or USER vars)
    switch (VarType) {
        case SYSTEM_VAR:
            i = IDC_ENVVAR_SYS_LB_SYSVARS;
            break;

        case USER_VAR:
        default:
            i = IDC_ENVVAR_SYS_LB_USERVARS;
            break;

    } // switch (VarType)

    hwndTemp = GetDlgItem (hDlg, i);

    n = FindVar (hwndTemp, szTemp2);

    if (n != -1)
    {
        // Free existing strings (listbox and ours)

        item.mask = LVIF_PARAM;
        item.iItem = n;
        item.iSubItem = 0;


        if (SendMessage (hwndTemp, LVM_GETITEM, 0, (LPARAM) &item)) {
            penvar = (ENVARS *) item.lParam;

        } else {
            penvar = NULL;
        }


        if (penvar) {
            MemFree (penvar->szValueName);
            MemFree (penvar->szValue);
            MemFree (penvar->szExpValue);
            MemFree ((LPVOID) penvar);
        }

        SendMessage (hwndTemp, LVM_DELETEITEM, n, 0L);
        PropSheet_Changed(GetParent(hDlg), hDlg);

        //  Fix selection state in listview
        if (n > 0) {
            n--;
        }

        item.mask = LVIF_STATE;
        item.iItem = n;
        item.iSubItem = 0;
        item.state = LVIS_SELECTED | LVIS_FOCUSED;
        item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

        SendDlgItemMessage (hDlg, i,
                            LVM_SETITEMSTATE, n, (LPARAM) &item);

    }

    return;
}

void
SetVar(
    IN HWND hDlg,
    IN UINT VarType,
    IN LPCTSTR szVarName,
    IN LPCTSTR szVarValue
)
/*++

Routine Description:

    Given an environment variable's type (system or user), name, and value,
    creates a ENVVARS structure for that environment variable and inserts
    it into the proper list view control, deleteing any existing variable
    of the same name.

Arguments:

    hDlg -
        Supplies window handle

    VarType -
        Supplies the type of the environment variable (system or user)

    szVarName -
        Supplies the name of the environment variable

    szVarValue -
        Supplies the value of the environment variable

Return Value:

    None, although it really should have one someday.

--*/
{
    TCHAR   szTemp2[MAX_PATH];
    int     i, n;
    TCHAR  *bBuffer;
    TCHAR  *pszTemp;
    LPTSTR  pszString;
    HWND    hwndTemp;
    int     idTemp;
    ENVARS *penvar;
    LV_ITEM item;

    wsprintf(
        szTemp2,
        TEXT("%s"),
        szVarName
    );

    //  Strip trailing whitespace from end of Env Variable

    i = lstrlen(szTemp2) - 1;

    while (i >= 0)
    {
        if (_istspace(szTemp2[i]))
            szTemp2[i--] = TEXT('\0');
        else
            break;
    }

    //
    // Make sure variable name does not contain the "=" sign.
    //

    pszTemp = _tcspbrk (szTemp2, TEXT("="));

    if (pszTemp)
        *pszTemp = TEXT('\0');


    if (szTemp2[0] == TEXT('\0'))
        return;


    bBuffer = (TCHAR *) MemAlloc (LPTR, BUFZ * sizeof(TCHAR));
    pszString = (LPTSTR) MemAlloc (LPTR, BUFZ * sizeof(TCHAR));

    wsprintf(
        bBuffer,
        TEXT("%s"),
        szVarValue
    );

    //  Determine which Listbox to use (SYSTEM or USER vars)
    switch (VarType) {
        case SYSTEM_VAR:
            idTemp = IDC_ENVVAR_SYS_LB_SYSVARS;
            break;

        case USER_VAR:
        default:
            idTemp = IDC_ENVVAR_SYS_LB_USERVARS;
            break;

    } // switch (VarType)
    hwndTemp = GetDlgItem(hDlg, idTemp);

    n = FindVar (hwndTemp, szTemp2);

    if (n != -1)
    {
        // Free existing strings (listview and ours)

        item.mask = LVIF_PARAM;
        item.iItem = n;
        item.iSubItem = 0;

        if (SendMessage (hwndTemp, LVM_GETITEM, 0, (LPARAM) &item)) {
            penvar = (ENVARS *) item.lParam;

        } else {
            penvar = NULL;
        }


        if (penvar) {
            MemFree (penvar->szValueName);
            MemFree (penvar->szValue);
            MemFree (penvar->szExpValue);
        }

        SendMessage (hwndTemp, LVM_DELETEITEM, n, 0L);
    }
    else
    {
        //  Get some storage for new Env Var
        penvar = (ENVARS *) MemAlloc (LPTR, sizeof(ENVARS));
    }

    //  If there are two '%' chars in string, then this is a
    //  REG_EXPAND_SZ style environment string

    pszTemp = _tcspbrk (bBuffer, TEXT("%"));

    if (pszTemp && _tcspbrk (pszTemp, TEXT("%")))
        penvar->dwType = REG_EXPAND_SZ;
    else
        penvar->dwType = REG_SZ;

    switch (VarType) {

    case SYSTEM_VAR:
        ExpandSystemVar(bBuffer, pszString, BUFZ);
        break;

    case USER_VAR:
        ExpandEnvironmentStrings (bBuffer, pszString, BUFZ);
        break;

    default:
        break;

    } /* switch */

    penvar->szValueName = CloneString (szTemp2);
    penvar->szValue     = CloneString (bBuffer);
    penvar->szExpValue  = CloneString (pszString);


    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.iItem = ListView_GetItemCount(hwndTemp);
    item.iSubItem = 0;
    item.pszText = penvar->szValueName;
    item.lParam = (LPARAM) penvar;

    n = SendMessage (hwndTemp, LVM_INSERTITEM, 0, (LPARAM) &item);

    if (n != -1) {
        item.mask = LVIF_TEXT;
        item.iItem = n;
        item.iSubItem = 1;
        item.pszText = penvar->szExpValue;

        SendMessage (hwndTemp, LVM_SETITEMTEXT, n, (LPARAM) &item);

        item.mask = LVIF_STATE;
        item.iItem = n;
        item.iSubItem = 0;
        item.state = LVIS_SELECTED | LVIS_FOCUSED;
        item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

        SendDlgItemMessage (hDlg, idTemp,
                            LVM_SETITEMSTATE, n, (LPARAM) &item);
    }


    MemFree (bBuffer);
    MemFree (pszString);

    return;

}

void
EVDoEdit(
    IN HWND hWnd,
    IN UINT VarType,
    IN UINT EditType,
    IN int iSelection
)
/*++

Routine Description:

    Sets up for, executes, and cleans up after an Environment Variable
    New... or Edit... dialog.  Called when user presses a New... or Edit...
    button.

Arguments:

    hWnd -
        Supplies window handle

    VarType -
        Supplies the type of the variable:  User (USER_VAR) or 
        System (SYSTEM_VAR)

    EditType -
        Supplies the type of the edit:  create New (NEW_VAR) or 
        Edit existing (EDIT_VAR)

    iSelection -
        Supplies the currently selected variable of type VarType.  This
        value is ignored if EditType is NEW_VAR.

Return Value:

    None.  May alter the contents of a list view control as a side effect.

--*/
{
    int Result = 0;
    BOOL fVarChanged = FALSE;
    HWND hWndLB = NULL;
    ENVARS *penvar = NULL;
    LV_ITEM item;

    ASSERT((-1 != iSelection) || (NEW_VAR == EditType));

    g_VarType = VarType;
    g_EditType = EditType;

    penvar = GetVar(hWnd, VarType, iSelection);

    switch (EditType) {
        case NEW_VAR:
            ZeroMemory((LPVOID) g_szVarName, (DWORD) BUFZ * sizeof(TCHAR));
            ZeroMemory((LPVOID) g_szVarValue, (DWORD) BUFZ * sizeof(TCHAR));
            break;

        case EDIT_VAR:
            if (penvar) {
                wsprintf(
                    g_szVarName,
                    TEXT("%s"),
                    penvar->szValueName
                );
                wsprintf(
                    g_szVarValue,
                    TEXT("%s"),
                    penvar->szValue
                );

            } // if
            else {
                MessageBeep(MB_ICONASTERISK);
                return;
            } // else
            break;

        case INVALID_EDIT_TYPE:
        default:
            return;
    } // switch
    
    Result = DialogBox(
        hInstance,
        (LPTSTR) MAKEINTRESOURCE(IDD_ENVVAREDIT),
        hWnd,
        EnvVarsEditDlg
    );

    //
    // Only update the list view control if the user
    // actually changed or created a variable
    //
    switch (Result) {
        case EDIT_CHANGE:
            if (EDIT_VAR == EditType) {
                fVarChanged = 
                    lstrcmp(penvar->szValueName, g_szVarName) ||
                    lstrcmp(penvar->szValue, g_szVarValue);
            } // if (EDIT_VAR...
            else if (NEW_VAR == EditType) {
                fVarChanged =
                    lstrlen(g_szVarName) && lstrlen(g_szVarValue);
            } // else if (NEW_VAR...
            else {
                fVarChanged = FALSE;
            } // else

            if (fVarChanged) {
                if (EDIT_VAR == EditType) {
                    DeleteVar(hWnd, VarType, penvar->szValueName);
                } // if (EDIT_VAR...
                SetVar(hWnd, VarType, g_szVarName, g_szVarValue);
            } // if (fVarChanged)
            break;

        default:
        break;
    } // switch (Result)

    g_VarType = INVALID_VAR_TYPE;
    g_EditType = INVALID_EDIT_TYPE;
    return; 
}

PENVAR
GetVar(
    IN HWND hDlg, 
    IN UINT VarType, 
    IN int iSelection
)
/*++

Routine Description:

    Returns a given System or User environment variable, as stored
    in the System or User environment variable listview control.

    Changing the structure returned by this routine is not
    recommended, because it will alter the values actually stored
    in the listview control.

Arguments:

    hDlg -
        Supplies window handle

    VarType -
        Supplies variable type--System or User

    iSelection -
        Supplies the selection index into the listview control of
        the desired environment variable

Return Value:

    Pointer to a valid ENVARS structure if successful.

    NULL if unsuccessful.

--*/
{
    HWND hWndLB = NULL;
    PENVAR penvar = NULL;
    LV_ITEM item;

    switch (VarType) {
        case SYSTEM_VAR:
            hWndLB = GetDlgItem(
                hDlg,
                IDC_ENVVAR_SYS_LB_SYSVARS
            );
            break;

        case USER_VAR:
            hWndLB = GetDlgItem(
                hDlg,
                IDC_ENVVAR_SYS_LB_USERVARS
            );
            break;

        case INVALID_VAR_TYPE:
        default:
            return NULL;
    } // switch (VarType)

    item.mask = LVIF_PARAM;
    item.iItem = iSelection;
    item.iSubItem = 0;
    if (SendMessage (hWndLB, LVM_GETITEM, 0, (LPARAM) &item)) {
        penvar = (ENVARS *) item.lParam;

    } else {
        penvar = NULL;
    }
    
    return(penvar);
}

int 
FindVar(
    IN HWND hwndLV, 
    IN LPTSTR szVar
)
/*++

Routine Description:

    Find the USER Environment variable that matches passed string
    and return its listview index or -1

Arguments:

    hwndLV -
        Supplies window handle to the list view control containing the
        environment variables

    szVar -
        Supplies the variable name in string form

Return Value:

    List view item index which matches the passed in string if the string
    is the name of an environment variable

    -1 if the passed in string is not the name of an environment variable

--*/
{
    LV_FINDINFO FindInfo;


    FindInfo.flags = LVFI_STRING;
    FindInfo.psz = szVar;

    return (SendMessage (hwndLV, LVM_FINDITEM, (WPARAM) -1, (LPARAM) &FindInfo));
}

void 
EVSave(
    IN HWND hDlg
)
/*++

Routine Description:

    Saves the environment variables in the registry

Arguments:

    hDlg -
        Supplies window handle

Return Value:

    None

--*/
{
    TCHAR   szTemp[MAX_PATH];
    int     selection;
    int     i, n;
    TCHAR  *bBuffer;
    TCHAR  *pszTemp;
    LPTSTR  pszString;
    HWND    hwndTemp;
    ENVARS *penvar;
    REGVAL *prvFirst;
    REGVAL *prvRegVal;
    HKEY    hkeyEnv;
    DWORD   dwBufz, dwIndex, dwType;
    LV_ITEM item;

    HourGlass (TRUE);

    /////////////////////////////////////////////////////////////////
    //  Set all new USER environment variables to current values
    //  but delete all old environment variables first
    /////////////////////////////////////////////////////////////////

    if (RegOpenKeyEx (HKEY_CURRENT_USER, szUserEnv, 0,
                     KEY_READ | KEY_WRITE, &hkeyEnv)
            == ERROR_SUCCESS)
    {
        dwBufz = ARRAYSIZE(szTemp) * sizeof(TCHAR);
        dwIndex = 0;

        //  Delete all values of type REG_SZ & REG_EXPAND_SZ under key

        //  First: Make a linked list of all USER Env string vars

        prvFirst = (REGVAL *) NULL;

        while (!RegEnumValue(hkeyEnv,
                             dwIndex++, // Index'th value name/data
                             szTemp,    // Ptr to ValueName buffer
                             &dwBufz,   // Size of ValueName buffer
                             NULL,      // Title index return
                             &dwType,   // Type code of entry
                             NULL,      // Ptr to ValueData buffer
                             NULL))     // Size of ValueData buffer
        {
            if ((dwType != REG_SZ) && (dwType != REG_EXPAND_SZ))
                goto EVSGetNextUserVar;

            if (prvFirst)
            {
                prvRegVal->prvNext = (REGVAL *) MemAlloc (LPTR, sizeof(REGVAL));
                prvRegVal = prvRegVal->prvNext;
            }
            else        // First time thru
            {
                prvFirst = prvRegVal = (REGVAL *) MemAlloc (LPTR, sizeof(REGVAL));
            }

            prvRegVal->prvNext = NULL;
            prvRegVal->szValueName = CloneString (szTemp);

            // Reset vars for next call
EVSGetNextUserVar:
            dwBufz = ARRAYSIZE(szTemp) * sizeof(TCHAR);
        }

        //  Now traverse the list, deleting them all

        prvRegVal = prvFirst;

        while (prvRegVal)
        {
            RegDeleteValue (hkeyEnv, prvRegVal->szValueName);

            MemFree (prvRegVal->szValueName);

            prvFirst  = prvRegVal;
            prvRegVal = prvRegVal->prvNext;

            MemFree ((LPVOID) prvFirst);
        }

        ///////////////////////////////////////////////////////////////
        //  Set all new USER environment variables to current values
        ///////////////////////////////////////////////////////////////

        hwndTemp = GetDlgItem (hDlg, IDC_ENVVAR_SYS_LB_USERVARS);

        if ((n = SendMessage (hwndTemp, LVM_GETITEMCOUNT, 0, 0L)) != LB_ERR)
        {

            item.mask = LVIF_PARAM;
            item.iSubItem = 0;

            for (i = 0; i < n; i++)
            {

                item.iItem = i;

                if (SendMessage (hwndTemp, LVM_GETITEM, 0, (LPARAM) &item)) {
                    penvar = (ENVARS *) item.lParam;

                } else {
                    penvar = NULL;
                }

                if (penvar) {
                    if (RegSetValueEx (hkeyEnv,
                                       penvar->szValueName,
                                       0L,
                                       penvar->dwType,
                              (LPBYTE) penvar->szValue,
                                       (lstrlen (penvar->szValue)+1) * sizeof(TCHAR)))
                    {
                        //  Report error trying to set registry values

                        if (MsgBoxParam (hDlg, SYSTEM+9, INITS+1,
                            MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL)
                            break;
                    }
                }
            }
        }

        RegFlushKey (hkeyEnv);
        RegCloseKey (hkeyEnv);
    }
    else
    {
        //  Report opening USER Environment key
        if (MsgBoxParam (hDlg, SYSTEM+8, INITS+1,
                       MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL)
            goto Exit;
    }

    /////////////////////////////////////////////////////////////////
    //  Set all new SYSTEM environment variables to current values
    //  but delete all old environment variables first
    /////////////////////////////////////////////////////////////////

    if (!bEditSystemVars)
        goto SkipSystemVars;

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                       szSysEnv,
                       0,
                       KEY_READ | KEY_WRITE,
                       &hkeyEnv)
            == ERROR_SUCCESS)
    {
        dwBufz = ARRAYSIZE(szTemp) * sizeof(TCHAR);
        dwIndex = 0;

        //  Delete all values of type REG_SZ & REG_EXPAND_SZ under key

        //  First: Make a linked list of all Env string vars

        prvFirst = (REGVAL *) NULL;

        while (!RegEnumValue(hkeyEnv,
                             dwIndex++, // Index'th value name/data
                             szTemp,    // Ptr to ValueName buffer
                             &dwBufz,   // Size of ValueName buffer
                             NULL,      // Title index return
                             &dwType,   // Type code of entry
                             NULL,      // Ptr to ValueData buffer
                             NULL))     // Size of ValueData buffer
        {
            if ((dwType != REG_SZ) && (dwType != REG_EXPAND_SZ))
                goto EVSGetNextSysVar;

            if (prvFirst)
            {
                prvRegVal->prvNext = (REGVAL *) MemAlloc (LPTR, sizeof(REGVAL));
                prvRegVal = prvRegVal->prvNext;
            }
            else        // First time thru
            {
                prvFirst = prvRegVal = (REGVAL *) MemAlloc (LPTR, sizeof(REGVAL));
            }

            prvRegVal->prvNext = NULL;
            prvRegVal->szValueName = CloneString (szTemp);

            // Reset vars for next call
EVSGetNextSysVar:
            dwBufz = ARRAYSIZE(szTemp) * sizeof(TCHAR);
        }

        //  Now traverse the list, deleting them all

        prvRegVal = prvFirst;

        while (prvRegVal)
        {
            RegDeleteValue (hkeyEnv, prvRegVal->szValueName);

            MemFree (prvRegVal->szValueName);

            prvFirst  = prvRegVal;
            prvRegVal = prvRegVal->prvNext;

            MemFree ((LPVOID) prvFirst);
        }

        ///////////////////////////////////////////////////////////////
        //  Set all new SYSTEM environment variables to current values
        ///////////////////////////////////////////////////////////////

        hwndTemp = GetDlgItem (hDlg, IDC_ENVVAR_SYS_LB_SYSVARS);

        if ((n = SendMessage (hwndTemp, LVM_GETITEMCOUNT, 0, 0L)) != LB_ERR)
        {
            item.mask = LVIF_PARAM;
            item.iSubItem = 0;

            for (i = 0; i < n; i++)
            {
                item.iItem = i;

                if (SendMessage (hwndTemp, LVM_GETITEM, 0, (LPARAM) &item)) {
                    penvar = (ENVARS *) item.lParam;

                } else {
                    penvar = NULL;
                }

                if (penvar) {
                    if (RegSetValueEx (hkeyEnv,
                                       penvar->szValueName,
                                       0L,
                                       penvar->dwType,
                              (LPBYTE) penvar->szValue,
                                       (lstrlen (penvar->szValue)+1) * sizeof(TCHAR)))
                    {
                        //  Report error trying to set registry values

                        if (MsgBoxParam (hDlg, SYSTEM+9, INITS+1,
                            MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL)
                            break;
                    }
                }
            }
        }

        RegFlushKey (hkeyEnv);
        RegCloseKey (hkeyEnv);
    }
    else
    {
        //  Report opening SYSTEM Environment key
        if (MsgBoxParam (hDlg, SYSTEM+21, INITS+1,
                       MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL)
            goto Exit;
    }

SkipSystemVars:

    // Send public message announcing change to Environment
    SendMessageTimeout( (HWND)-1, WM_WININICHANGE, 0L, (LONG)szUserEnv,
                                            SMTO_ABORTIFHUNG, 1000, NULL );


Exit:

    HourGlass (FALSE);
}


void 
EVCleanUp(
    IN HWND hDlg
)
/*++

Routine Description:

    Frees memory allocated for environment variables

Arguments:

    hDlg -
        Supplies window handle

Return Value:

    None.

--*/
{
    int     i, n;
    HWND    hwndTemp;
    ENVARS *penvar;
    LV_ITEM item;


    //
    //  Free alloc'd strings and memory for UserEnvVars list box items
    //

    hwndTemp = GetDlgItem (hDlg, IDC_ENVVAR_SYS_LB_USERVARS);
    n = SendMessage (hwndTemp, LVM_GETITEMCOUNT, 0, 0L);

    item.mask = LVIF_PARAM;
    item.iSubItem = 0;

    for (i = 0; i < n; i++) {

        item.iItem = i;

        if (SendMessage (hwndTemp, LVM_GETITEM, 0, (LPARAM) &item)) {
            penvar = (ENVARS *) item.lParam;

        } else {
            penvar = NULL;
        }

        if (penvar) {
            MemFree (penvar->szValueName);
            MemFree (penvar->szValue);
            MemFree (penvar->szExpValue);
            MemFree ((LPVOID) penvar);
        }
    }


    //
    //  Free alloc'd strings and memory for SysEnvVars list box items
    //

    hwndTemp = GetDlgItem (hDlg, IDC_ENVVAR_SYS_LB_SYSVARS);
    n = SendMessage (hwndTemp, LVM_GETITEMCOUNT, 0, 0L);

    for (i = 0; i < n; i++) {

        item.iItem = i;

        if (SendMessage (hwndTemp, LVM_GETITEM, 0, (LPARAM) &item)) {
            penvar = (ENVARS *) item.lParam;

        } else {
            penvar = NULL;
        }

        if (penvar) {
            MemFree (penvar->szValueName);
            MemFree (penvar->szValue);
            MemFree (penvar->szExpValue);
            MemFree ((LPVOID) penvar);
        }
    }
}


BOOL 
ExpandSystemVar( 
    IN LPCTSTR pszSrc, 
    OUT LPTSTR pszDst, 
    IN DWORD cchDst 
) 
/*++

Routine Description:

    Private version of ExpandEnvironmentStrings() which only expands
    references to the variables "SystemRoot" and "SystemDrive".

    This behavior is intended to match the way SMSS expands system
    environment variables.

Arguments:

    pszSrc -
        Supplies the system variable value to be expanded.

    pszDst -
        Returns the expanded system variable value.

    cchDst -
        Supplies the size, in characters, of the buffer pointed to
        by pszDst

Return Value:

    TRUE if there was room in the supplied buffer for the entire
    expanded string.

    FALSE if there was insufficient space in the supplied buffer
    for the entire expanded string.

--*/
{
    TCHAR ch;
    LPTSTR p;
    TCHAR szVar[MAX_PATH];
    DWORD cch;

    do {

        ch = *pszSrc++;

        if (ch != TEXT('%') ) {

            // no space left, truncate string and return false
            if (--cchDst == 0) {
                *pszDst = TEXT('\0');
                return FALSE;
            }

            *pszDst++ = ch;

        } else {
            /*
             * Expand variable
             */
            // look for the next '%'
            p = szVar;
            while( *pszSrc != TEXT('\0') && *pszSrc != TEXT('%') )
                    *p++ = *pszSrc++;

            *p = TEXT('\0');

            if (*pszSrc == TEXT('\0')) {
                // end of string, first '%' must be literal
                cch = lstrlen(szVar) + 1;

                // no more space, return false
                if (cch + 1 > cchDst) {
                    *pszDst++ = TEXT('\0');
                    return FALSE;
                }

                *pszDst++ = TEXT('%');
                CopyMemory( pszDst, szVar, cch * sizeof(TCHAR));
                return TRUE;

            } else {
                // we found the ending '%' sign, expand that string

                //
                // We're expanding a SYSTEM variable, so only expand
                // references to SystemRoot and SystemDrive.
                //
                if ((!lstrcmpi(szVar, SYSTEMROOT)) || (!lstrcmpi(szVar, SYSTEMDRIVE))) {
                    cch = GetEnvironmentVariable(szVar, pszDst, cchDst);
                } /* if */
                else {
                    cch = 0;
                } /* else */

                if (cch == 0 || cch >= cchDst) {
                    //String didn't expand, copy it as a literal
                    cch = lstrlen(szVar);

                    // no space left, trunc string and return FALSE
                    if (cch + 2 + 1 > cchDst ) {
                        *pszDst = TEXT('\0');
                        return FALSE;
                    }

                    *pszDst++ = TEXT('%');

                    CopyMemory(pszDst, szVar, cch * sizeof(TCHAR));
                    pszDst += cch;

                    *pszDst++ = TEXT('%');

                    // cchDst -= two %'s and the string
                    cchDst -= (2 + cch);

                } else {
                    // string was expanded in place, bump pointer past its end
                    pszDst += cch;
                    cchDst -= cch;
                }

                // continue with next char after ending '%'
                pszSrc++;
            }
        }

    } while( ch != TEXT('\0') );

    return TRUE;
}