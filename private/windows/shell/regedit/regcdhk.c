/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGCDHK.C
*
*  VERSION:     4.0
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        21 Nov 1993
*
*  Common dialog box hook functions for the Registry Editor.
*
*******************************************************************************/

#include "pch.h"
#include "regedit.h"
#include "regkey.h"
#include "regcdhk.h"
#include "regresid.h"
#include "reghelp.h"

//  Buffer to store the starting path for a registry export or print operation.
TCHAR g_SelectedPath[SIZE_SELECTED_PATH];

//  TRUE if registry operation should be applied to the entire registry or to
//  only start at g_SelectedPath.
BOOL g_fRangeAll;

//  Contains the resource identifier for the dialog that is currently being
//  used.  Assumes that there is only one instance of a hook dialog at a time.
UINT g_RegCommDlgDialogTemplate;

const DWORD s_RegCommDlgExportHelpIDs[] = {
    stc32,                 NO_HELP,
    IDC_EXPORTRANGE,       IDH_REGEDIT_EXPORT,
    IDC_RANGEALL,          IDH_REGEDIT_EXPORT,
    IDC_RANGESELECTEDPATH, IDH_REGEDIT_EXPORT,
    IDC_SELECTEDPATH,      IDH_REGEDIT_EXPORT,

    0, 0
};

const DWORD s_RegCommDlgPrintHelpIDs[] = {
    IDC_EXPORTRANGE,       IDH_REGEDIT_PRINTRANGE,
    IDC_RANGEALL,          IDH_REGEDIT_PRINTRANGE,
    IDC_RANGESELECTEDPATH, IDH_REGEDIT_PRINTRANGE,
    IDC_SELECTEDPATH,      IDH_REGEDIT_PRINTRANGE,

    0, 0
};

BOOL
PASCAL
RegCommDlg_OnInitDialog(
    HWND hWnd,
    HWND hFocusWnd,
    LPARAM lParam
    );

LRESULT
PASCAL
RegCommDlg_OnNotify(
    HWND hWnd,
    int DlgItem,
    LPNMHDR lpNMHdr
    );

UINT_PTR
PASCAL
RegCommDlg_OnCommand(
    HWND hWnd,
    int DlgItem,
    UINT NotificationCode
    );

BOOL
PASCAL
RegCommDlg_ValidateSelectedPath(
    HWND hWnd,
    BOOL fIsFileDialog
    );

/*******************************************************************************
*
*  RegCommDlgHookProc
*
*  DESCRIPTION:
*     Callback procedure for the RegCommDlg common dialog box.
*
*  PARAMETERS:
*     hWnd, handle of RegCommDlg window.
*     Message,
*     wParam,
*     lParam,
*     (returns),
*
*******************************************************************************/

UINT_PTR
CALLBACK
RegCommDlgHookProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{

    int DlgItem;
    const DWORD FAR* lpHelpIDs;

    switch (Message) {

        HANDLE_MSG(hWnd, WM_INITDIALOG, RegCommDlg_OnInitDialog);

        case WM_NOTIFY:
            SetDlgMsgResult(hWnd, WM_NOTIFY, HANDLE_WM_NOTIFY(hWnd, wParam,
                lParam, RegCommDlg_OnNotify));
            return TRUE;

        case WM_COMMAND:
            return RegCommDlg_OnCommand(hWnd, GET_WM_COMMAND_ID(wParam, lParam),
                GET_WM_COMMAND_CMD(wParam, lParam));

        case WM_HELP:
            //
            //  We only want to intercept help messages for controls that we are
            //  responsible for.
            //

            DlgItem = GetDlgCtrlID(((LPHELPINFO) lParam)-> hItemHandle);

            if (DlgItem < IDC_FIRSTREGCOMMDLGID || DlgItem >
                IDC_LASTREGCOMMDLGID)
                break;

            lpHelpIDs = (g_RegCommDlgDialogTemplate == IDD_REGEXPORT) ?
                s_RegCommDlgExportHelpIDs : s_RegCommDlgPrintHelpIDs;

            WinHelp(((LPHELPINFO) lParam)-> hItemHandle, g_pHelpFileName,
                HELP_WM_HELP, (ULONG_PTR) lpHelpIDs);
            return TRUE;

        case WM_CONTEXTMENU:
            //
            //  We only want to intercept help messages for controls that we are
            //  responsible for.
            //

            DlgItem = GetDlgCtrlID((HWND) wParam);

            if (g_RegCommDlgDialogTemplate == IDD_REGEXPORT)
                lpHelpIDs = s_RegCommDlgExportHelpIDs;

            else {

                if (DlgItem < IDC_FIRSTREGCOMMDLGID || DlgItem >
                    IDC_LASTREGCOMMDLGID)
                    break;

                lpHelpIDs = s_RegCommDlgPrintHelpIDs;

            }

            WinHelp((HWND) wParam, g_pHelpFileName, HELP_CONTEXTMENU,
                (ULONG_PTR) lpHelpIDs);
            return TRUE;

    }

    return FALSE;

}

/*******************************************************************************
*
*  RegCommDlg_OnInitDialog
*
*  DESCRIPTION:
*     Initializes the RegCommDlg dialog box.
*
*  PARAMETERS:
*     hWnd, handle of RegCommDlg window.
*     hFocusWnd, handle of control to receive the default keyboard focus.
*     lParam, additional initialization data passed by dialog creation function.
*     (returns), TRUE to set focus to hFocusWnd, else FALSE to prevent a
*        keyboard focus from being set.
*
*******************************************************************************/

BOOL
PASCAL
RegCommDlg_OnInitDialog(
    HWND hWnd,
    HWND hFocusWnd,
    LPARAM lParam
    )
{

    HWND hKeyTreeWnd;
    HTREEITEM hSelectedTreeItem;
    int DlgItem;

    g_RegEditData.fSaveInDownlevelFormat = FALSE;

    hKeyTreeWnd = g_RegEditData.hKeyTreeWnd;
    hSelectedTreeItem = TreeView_GetSelection(hKeyTreeWnd);

    KeyTree_BuildKeyPath(hKeyTreeWnd, hSelectedTreeItem, g_SelectedPath,
        BKP_TOSYMBOLICROOT);
    SetDlgItemText(hWnd, IDC_SELECTEDPATH, g_SelectedPath);

    DlgItem = (TreeView_GetParent(hKeyTreeWnd, hSelectedTreeItem) == NULL) ?
        IDC_RANGEALL : IDC_RANGESELECTEDPATH;
    CheckRadioButton(hWnd, IDC_RANGEALL, IDC_RANGESELECTEDPATH, DlgItem);

    return TRUE;

    UNREFERENCED_PARAMETER(hFocusWnd);
    UNREFERENCED_PARAMETER(lParam);

}

/*******************************************************************************
*
*  RegCommDlg_OnNotify
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of RegCommDlg window.
*     DlgItem, identifier of control.
*     lpNMHdr, control notification data.
*
*******************************************************************************/

LRESULT
PASCAL
RegCommDlg_OnNotify(
    HWND hWnd,
    int DlgItem,
    LPNMHDR lpNMHdr
    )
{

    HWND hControlWnd;
    RECT DialogRect;
    RECT ControlRect;
    int dxChange;
    LPOFNOTIFY lpon;

    switch (lpNMHdr-> code) {

        case CDN_INITDONE:
            GetWindowRect(hWnd, &DialogRect);
            // Use window coordinates because it works for mirrored 
            // and non mirrored windows.
            MapWindowPoints(NULL, hWnd, (LPPOINT)&DialogRect, 2);

            hControlWnd = GetDlgItem(hWnd, IDC_EXPORTRANGE);
            GetWindowRect(hControlWnd, &ControlRect);
            MapWindowPoints(NULL, hWnd, (LPPOINT)&ControlRect, 2);

            dxChange = DialogRect.right - ControlRect.right -
                (ControlRect.left - DialogRect.left);

            SetWindowPos(hControlWnd, NULL, 0, 0, ControlRect.right -
                ControlRect.left + dxChange, ControlRect.bottom -
                ControlRect.top, SWP_NOMOVE | SWP_NOZORDER);

            hControlWnd = GetDlgItem(hWnd, IDC_SELECTEDPATH);
            GetWindowRect(hControlWnd, &ControlRect);
            MapWindowPoints(NULL, hWnd, (LPPOINT)&ControlRect, 2);

            SetWindowPos(hControlWnd, NULL, 0, 0, ControlRect.right -
                ControlRect.left + dxChange, ControlRect.bottom -
                ControlRect.top, SWP_NOMOVE | SWP_NOZORDER);

            break;

        case CDN_TYPECHANGE:
            // If the new Save Type is "Win9x/NT4 Reg File (REGEDIT4)"
            // (the second on the filter list) set the flag to tell us 
            // to save in downlevel format.
            lpon = (LPOFNOTIFY) lpNMHdr;
            if (lpon->lpOFN->nFilterIndex == 2) {
                g_RegEditData.fSaveInDownlevelFormat = TRUE;
            }
            else {
                g_RegEditData.fSaveInDownlevelFormat = FALSE;
            }

            break;

        case CDN_FILEOK:
            return ( RegCommDlg_ValidateSelectedPath(hWnd, TRUE) != FALSE );

    }

    return FALSE;

}

/*******************************************************************************
*
*  RegCommDlg_OnCommand
*
*  DESCRIPTION:
*     Handles the selection of a menu item by the user, notification messages
*     from a child control, or translated accelerated keystrokes for the
*     RegPrint dialog box.
*
*  PARAMETERS:
*     hWnd, handle of RegCommDlg window.
*     DlgItem, identifier of control.
*     NotificationCode, notification code from control.
*
*******************************************************************************/

UINT_PTR
PASCAL
RegCommDlg_OnCommand(
    HWND hWnd,
    int DlgItem,
    UINT NotificationCode
    )
{

    switch (DlgItem) {

        case IDC_RANGESELECTEDPATH:
            SetFocus(GetDlgItem(hWnd, IDC_SELECTEDPATH));
            break;

        case IDC_SELECTEDPATH:
            switch (NotificationCode) {

                case EN_SETFOCUS:
                    SendDlgItemMessage(hWnd, IDC_SELECTEDPATH, EM_SETSEL,
                        0, -1);
                    break;

                case EN_CHANGE:
                    CheckRadioButton(hWnd, IDC_RANGEALL, IDC_RANGESELECTEDPATH,
                        IDC_RANGESELECTEDPATH);
                    break;

            }
            break;

        case IDOK:
            return ( RegCommDlg_ValidateSelectedPath(hWnd, FALSE) != FALSE );

    }

    return FALSE;

}

/*******************************************************************************
*
*  RegCommDlg_ValidateSelectedPath
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of RegCommDlg window.
*     (returns), TRUE if the registry selected path is invalid, else FALSE.
*
*******************************************************************************/

BOOL
PASCAL
RegCommDlg_ValidateSelectedPath(
    HWND hWnd,
    BOOL fIsFileDialog
    )
{

    HKEY hKey;
    HWND hTitleWnd;
    TCHAR Title[256];

    if (!(g_fRangeAll = IsDlgButtonChecked(hWnd, IDC_RANGEALL))) {

        GetDlgItemText(hWnd, IDC_SELECTEDPATH, g_SelectedPath,
            sizeof(g_SelectedPath)/sizeof(TCHAR));

        if (g_SelectedPath[0] == '\0')
            g_fRangeAll = TRUE;

        else {

            if (EditRegistryKey(&hKey, g_SelectedPath, ERK_OPEN) !=
                ERROR_SUCCESS) {

                //
                //  Determine the "real" parent of this dialog and get the
                //  message box title from that window.  Our HWND may really
                //  be a subdialog if we're a file dialog.
                //

                hTitleWnd = fIsFileDialog ? GetParent(hWnd) : hWnd;
                GetWindowText(hTitleWnd, Title, sizeof(Title)/sizeof(TCHAR));
                InternalMessageBox(g_hInstance, hTitleWnd,
                    MAKEINTRESOURCE(IDS_ERRINVALIDREGPATH), Title,
                    MB_ICONERROR | MB_OK);

                return TRUE;

            }

            RegCloseKey(hKey);

        }

    }

    return FALSE;

}
