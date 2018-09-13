/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGDWDED.C
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        24 Sep 1994
*
*  Dword edit dialog for use by the Registry Editor.
*
*******************************************************************************/

#include "pch.h"
#include "regresid.h"
#include "reghelp.h"

const DWORD s_EditDwordValueHelpIDs[] = {
    IDC_VALUEDATA,      IDH_REGEDIT_VALUEDATA,
    IDC_VALUENAME,      IDH_REGEDIT_VALUENAME,
    IDC_HEXADECIMAL,    IDH_REGEDIT_DWORDBASE,
    IDC_DECIMAL,        IDH_REGEDIT_DWORDBASE,
    0, 0
};

const TCHAR s_DecimalFormatSpec[] = TEXT("%u");
const TCHAR s_HexadecimalFormatSpec[] = TEXT("%x");

//  Subclassed IDC_VALUEDATA's previous window procedure.  Only one instance of
//  this dialog is assumed to exist.
WNDPROC s_PrevValueDataWndProc;

//  The radio button that is currently selected: IDC_HEXADECIMAL or IDC_DECIMAL.
UINT s_SelectedBase;

UINT
PASCAL
GetDlgItemHex(
    HWND hWnd,
    int nIDDlgItem,
    BOOL *lpTranslated
    );

BOOL
PASCAL
EditDwordValue_OnInitDialog(
    HWND hWnd,
    HWND hFocusWnd,
    LPARAM lParam
    );

VOID
PASCAL
EditDwordValue_SetValueDataText(
    HWND hWnd,
    LPEDITVALUEPARAM lpEditValueParam,
    UINT DlgItem
    );

LRESULT
CALLBACK
EditDwordValue_ValueDataWndProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

/*******************************************************************************
*
*  GetDlgItemHex
*
*  DESCRIPTION:
*     Like GetDlgItemInt, only for hexadecimal numbers.
*
*  PARAMETERS:
*     See GetDlgItemInt.
*
*******************************************************************************/

UINT
PASCAL
GetDlgItemHex(
    HWND hWnd,
    int nIDDlgItem,
    BOOL *lpTranslated
    )
{

    TCHAR Buffer[10];                   //  Enough to hold 8 digits, null, extra
    UINT Length;
    DWORD Dword;
    UINT Index;
    DWORD Nibble;

    Dword = 0;

    //
    //  We'll assume that the edit control contains only valid characters and
    //  doesn't begin with any spaces (for Regedit this will be true).  So, we
    //  only need to validate that the length of the string isn't too long.
    //

    Length = GetDlgItemText(hWnd, nIDDlgItem, Buffer, sizeof(Buffer)/sizeof(TCHAR));

    if (Length > 0 && Length <= 8) {

        for (Index = 0; Index < Length; Index++) {

            if (Buffer[Index] >= TEXT('0') && Buffer[Index] <= TEXT('9'))
                Nibble = Buffer[Index] - TEXT('0');
            else if (Buffer[Index] >= TEXT('a') && Buffer[Index] <= TEXT('f'))
                Nibble = Buffer[Index] - TEXT('a') + 10;
            else
                Nibble = Buffer[Index] - TEXT('A') + 10;

            Dword = (Dword << 4) + Nibble;

        }

        *lpTranslated = TRUE;

    }

    else
        *lpTranslated = FALSE;

    return Dword;

}

/*******************************************************************************
*
*  EditDwordValueDlgProc
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

INT_PTR
CALLBACK
EditDwordValueDlgProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{

    LPEDITVALUEPARAM lpEditValueParam;
    BOOL Translated;
    DWORD Dword;

    lpEditValueParam = (LPEDITVALUEPARAM) GetWindowLongPtr(hWnd, DWLP_USER);

    switch (Message) {

        HANDLE_MSG(hWnd, WM_INITDIALOG, EditDwordValue_OnInitDialog);

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {

                case IDC_VALUEDATA:
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_KILLFOCUS) {

                        Dword = (s_SelectedBase == IDC_HEXADECIMAL) ?
                            GetDlgItemHex(hWnd, IDC_VALUEDATA, &Translated) :
                            GetDlgItemInt(hWnd, IDC_VALUEDATA, &Translated,
                            FALSE);

                        if (Translated)
                            ((LPDWORD) lpEditValueParam-> pValueData)[0] =
                            Dword;

                        else {

                            MessageBeep(0);
                            EditDwordValue_SetValueDataText(hWnd,
                                lpEditValueParam, s_SelectedBase);

                        }

                    }
                    break;

                case IDC_DECIMAL:
                case IDC_HEXADECIMAL:
                    EditDwordValue_SetValueDataText(hWnd, lpEditValueParam,
                        GET_WM_COMMAND_ID(wParam, lParam));
                    break;

                case IDOK:
                case IDCANCEL:
                    EndDialog(hWnd, GET_WM_COMMAND_ID(wParam, lParam));
                    break;

            }
            break;

        case WM_HELP:
            WinHelp(((LPHELPINFO) lParam)-> hItemHandle, g_pHelpFileName,
                HELP_WM_HELP, (ULONG_PTR) s_EditDwordValueHelpIDs);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND) wParam, g_pHelpFileName, HELP_CONTEXTMENU,
                (ULONG_PTR) s_EditDwordValueHelpIDs);
            break;

        default:
            return FALSE;

    }

    return TRUE;

}

/*******************************************************************************
*
*  EditDwordValue_OnInitDialog
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of EditDwordValue window.
*     hFocusWnd,
*     lParam,
*
*******************************************************************************/

BOOL
PASCAL
EditDwordValue_OnInitDialog(
    HWND hWnd,
    HWND hFocusWnd,
    LPARAM lParam
    )
{

    LPEDITVALUEPARAM lpEditValueParam;

    SetWindowLongPtr(hWnd, DWLP_USER, lParam);
    lpEditValueParam = (LPEDITVALUEPARAM) lParam;

    s_PrevValueDataWndProc = SubclassWindow(GetDlgItem(hWnd, IDC_VALUEDATA),
        EditDwordValue_ValueDataWndProc);

    SetDlgItemText(hWnd, IDC_VALUENAME, lpEditValueParam-> pValueName);

    CheckRadioButton(hWnd, IDC_HEXADECIMAL, IDC_DECIMAL, IDC_HEXADECIMAL);
    EditDwordValue_SetValueDataText(hWnd, lpEditValueParam, IDC_HEXADECIMAL);

    return TRUE;

    UNREFERENCED_PARAMETER(hFocusWnd);

}

/*******************************************************************************
*
*  EditDwordValue_SetValueDataText
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of EditDwordValue window.
*
*******************************************************************************/

VOID
PASCAL
EditDwordValue_SetValueDataText(
    HWND hWnd,
    LPEDITVALUEPARAM lpEditValueParam,
    UINT DlgItem
    )
{

    TCHAR Buffer[12];                    //  Enough to hold 2^32 in decimal
    LPCTSTR lpFormatSpec;

    s_SelectedBase = DlgItem;

    lpFormatSpec = (DlgItem == IDC_HEXADECIMAL) ? s_HexadecimalFormatSpec :
        s_DecimalFormatSpec;
    wsprintf(Buffer, lpFormatSpec, ((LPDWORD) lpEditValueParam->
        pValueData)[0]);
    SetDlgItemText(hWnd, IDC_VALUEDATA, Buffer);

}

/*******************************************************************************
*
*  EditDwordValue_ValueDataEditProc
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of EditDwordValue window.
*
*******************************************************************************/

LRESULT
CALLBACK
EditDwordValue_ValueDataWndProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{

    TCHAR Char;

    switch (Message) {

        case WM_CHAR:
            Char = (TCHAR) wParam;

            if (Char >= TEXT(' ')) {

                if ((Char >= TEXT('0') && Char <= TEXT('9')))
                    break;

                if (s_SelectedBase == IDC_HEXADECIMAL &&
                    ((Char >= TEXT('A') && Char <= TEXT('F')) || ((Char >= TEXT('a')) &&
                    (Char <= TEXT('f')))))
                    break;

                MessageBeep(0);
                return 0;

            }
            break;

    }

    return CallWindowProc(s_PrevValueDataWndProc, hWnd, Message, wParam,
        lParam);

}
