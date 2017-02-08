/*
 * PROJECT:         input.dll
 * FILE:            dll/cpl/input/keysettings.c
 * PURPOSE:         input.dll
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 *                  Colin Finck
 * UPDATE HISTORY:
 *      06-09-2007  Created
 */

#include "input.h"

static HWND hKeySettingsWnd;

static VOID
AddListColumn(HWND hDlg)
{
    LV_COLUMN column;
    HWND hList = GetDlgItem(hDlg, IDC_KEY_LISTVIEW);

    ZeroMemory(&column, sizeof(LV_COLUMN));
    column.mask         = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    column.fmt          = LVCFMT_LEFT;
    column.iSubItem     = 0;
    column.pszText      = _T("");
    column.cx           = 210;
    (VOID) ListView_InsertColumn(hList, 0, &column);

    column.fmt          = LVCFMT_RIGHT;
    column.cx           = 145;
    column.iSubItem     = 1;
    column.pszText      = _T("");
    (VOID) ListView_InsertColumn(hList, 1, &column);
}

static DWORD
GetAttributes()
{
    DWORD dwValue, dwType, dwSize;
    HKEY hKey;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout"), 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
        return 0x0;

    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);

    if (RegQueryValueEx(hKey, _T("Attributes"), NULL, &dwType, (LPBYTE)&dwValue, &dwSize) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return 0x0;
    }

    RegCloseKey(hKey);
    return dwValue;
}

static VOID
SaveKeySettings(HWND hDlg)
{
    HKEY hKey;
    DWORD dwValue;

    if (SendDlgItemMessage(hDlg, IDC_PRESS_CL_KEY_RB, BM_GETCHECK, 0, 0) == BST_CHECKED)
        dwValue = 0x0;
    else
        dwValue = 0x10000;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout"), 0, NULL,
                       REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                       NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueEx(hKey, _T("Attributes"), 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));

        RegCloseKey(hKey);
    }
}

static VOID
InitKeySettingsDlg(HWND hDlg)
{
    TCHAR szHotkey[1 + 1], szLangHotkey[1 + 1], szLayoutHotkey[1 + 1],
          szTitle[MAX_PATH], szText[MAX_PATH];
    LV_ITEM item = {0};
    HWND hHotkeyList = GetDlgItem(hDlg, IDC_KEY_LISTVIEW);
    INT i;

    if (GetAttributes() != 0x0)
        SendDlgItemMessage(hDlg, IDC_PRESS_SHIFT_KEY_RB, BM_SETCHECK, 1, 1);
    else
        SendDlgItemMessage(hDlg, IDC_PRESS_CL_KEY_RB, BM_SETCHECK, 1, 1);

    if (!GetHotkeys(szHotkey, szLangHotkey, szLayoutHotkey))
        return;

    if (!LoadString(hApplet, IDS_SWITCH_BET_INLANG, szTitle, sizeof(szTitle) / sizeof(TCHAR)))
        return;

    if (_tcscmp(szLangHotkey, _T("2")) == 0)
        LoadString(hApplet, IDS_CTRL_SHIFT, szText, sizeof(szText) / sizeof(TCHAR));
    else if (_tcscmp(szLangHotkey, _T("1")) == 0)
        LoadString(hApplet, IDS_LEFT_ALT_SHIFT, szText, sizeof(szText) / sizeof(TCHAR));
    else
        LoadString(hApplet, IDS_NONE, szText, sizeof(szText) / sizeof(TCHAR));

    item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    item.pszText = szTitle;
    item.iItem   = 0;
    i = ListView_InsertItem(hHotkeyList, &item);

    ListView_SetItemText(hHotkeyList, i, 1, szText);

    (VOID) ListView_SetHotItem(hHotkeyList, i);
    ListView_SetItemState(hHotkeyList, i, LVIS_SELECTED, LVIS_OVERLAYMASK);
}

VOID
UpdateKeySettingsList()
{
    (VOID) ListView_DeleteAllItems(GetDlgItem(hKeySettingsWnd, IDC_KEY_LISTVIEW));
    InitKeySettingsDlg(hKeySettingsWnd);
}

INT_PTR CALLBACK
KeySettingsDlgProc(HWND hDlg,
                   UINT message,
                   WPARAM wParam,
                   LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
            hKeySettingsWnd = hDlg;
            AddListColumn(hDlg);
            (VOID) ListView_SetExtendedListViewStyle(GetDlgItem(hDlg, IDC_KEY_LISTVIEW),
                                                     LVS_EX_FULLROWSELECT);
            InitKeySettingsDlg(hDlg);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_CHANGE_KEY_SEQ_BTN:
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_CHANGE_KEY_SEQ),
                              hDlg,
                              ChangeKeySeqDlgProc);
                    break;

                case IDOK:
                    SaveKeySettings(hDlg);
                    EndDialog(hDlg, LOWORD(wParam));
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
                    break;
            }
            break;
    }

    return FALSE;
}

/* EOF */
