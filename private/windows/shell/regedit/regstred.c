/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGSTRED.C
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        05 Mar 1994
*
*  String edit dialog for use by the Registry Editor.
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  05 Mar 1994 TCS Original implementation.
*
*******************************************************************************/

#include "pch.h"
#include "regresid.h"
#include "reghelp.h"

const DWORD s_EditStringValueHelpIDs[] = {
    IDC_VALUEDATA, IDH_REGEDIT_VALUEDATA,
    IDC_VALUENAME, IDH_REGEDIT_VALUENAME,
    0, 0
};

BOOL
PASCAL
EditStringValue_OnInitDialog(
    HWND hWnd,
    HWND hFocusWnd,
    LPARAM lParam
    );

/*******************************************************************************
*
*  EditStringValueDlgProc
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

INT_PTR
CALLBACK
EditStringValueDlgProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{

    LPEDITVALUEPARAM lpEditValueParam;

    switch (Message) {

        HANDLE_MSG(hWnd, WM_INITDIALOG, EditStringValue_OnInitDialog);

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {

                case IDOK:
                    lpEditValueParam = (LPEDITVALUEPARAM) GetWindowLongPtr(hWnd,
                        DWLP_USER);

                    lpEditValueParam-> cbValueData = sizeof(TCHAR) * ((UINT)
                        SendDlgItemMessage(hWnd, IDC_VALUEDATA,
                        WM_GETTEXTLENGTH, 0, 0) + 1);

                    GetDlgItemText(hWnd, IDC_VALUEDATA, (PTSTR)lpEditValueParam->
                        pValueData, MAXDATA_LENGTH - 1);

                    //  FALL THROUGH

                case IDCANCEL:
                    EndDialog(hWnd, GET_WM_COMMAND_ID(wParam, lParam));
                    break;

            }
            break;

        case WM_HELP:
            WinHelp(((LPHELPINFO) lParam)-> hItemHandle, g_pHelpFileName,
                HELP_WM_HELP, (ULONG_PTR) s_EditStringValueHelpIDs);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND) wParam, g_pHelpFileName, HELP_CONTEXTMENU,
                (ULONG_PTR) s_EditStringValueHelpIDs);
            break;

        default:
            return FALSE;

    }

    return TRUE;

}

/*******************************************************************************
*
*  EditStringValue_OnInitDialog
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of EditStringValue window.
*     hFocusWnd,
*     lParam,
*
*******************************************************************************/

BOOL
PASCAL
EditStringValue_OnInitDialog(
    HWND hWnd,
    HWND hFocusWnd,
    LPARAM lParam
    )
{

    LPEDITVALUEPARAM lpEditValueParam;

    SetWindowLongPtr(hWnd, DWLP_USER, lParam);
    lpEditValueParam = (LPEDITVALUEPARAM) lParam;

    SetDlgItemText(hWnd, IDC_VALUENAME, lpEditValueParam-> pValueName);
    SetDlgItemText(hWnd, IDC_VALUEDATA, (PTSTR)lpEditValueParam-> pValueData);

    return TRUE;

    UNREFERENCED_PARAMETER(hFocusWnd);

}
