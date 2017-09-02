/*
* PROJECT:         input.dll
* FILE:            dll/cpl/input/key_settings_dialog.c
* PURPOSE:         input.dll
* PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
*/

#include "input.h"

static KEY_SETTINGS _KeySettings = { 0 };


DWORD
ReadAttributes(VOID)
{
    DWORD dwAttributes = 0;
    HKEY hKey;

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Keyboard Layout",
                      0,
                      KEY_QUERY_VALUE,
                      &hKey) == ERROR_SUCCESS)
    {
        DWORD dwSize = sizeof(dwAttributes);

        RegQueryValueExW(hKey,
                         L"Attributes",
                         NULL, NULL,
                         (LPBYTE)&dwAttributes,
                         &dwSize);

        RegCloseKey(hKey);
    }

    return dwAttributes;
}

static VOID
ReadKeysSettings(VOID)
{
    HKEY hKey;

    _KeySettings.dwAttributes = ReadAttributes();

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Keyboard Layout\\Toggle",
                      0,
                      KEY_QUERY_VALUE,
                      &hKey) == ERROR_SUCCESS)
    {
        WCHAR szBuffer[MAX_PATH];
        DWORD dwSize;

        dwSize = sizeof(szBuffer);

        if (RegQueryValueExW(hKey,
                             L"Language Hotkey",
                             NULL, NULL,
                             (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS)
        {
            _KeySettings.dwLanguage = _wtoi(szBuffer);
        }

        dwSize = sizeof(szBuffer);

        if (RegQueryValueExW(hKey,
                             L"Layout Hotkey",
                             NULL, NULL,
                             (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS)
        {
            _KeySettings.dwLayout = _wtoi(szBuffer);
        }

        RegCloseKey(hKey);
    }
}


static VOID
WriteKeysSettings(VOID)
{
    HKEY hKey;

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Keyboard Layout",
                      0,
                      KEY_SET_VALUE,
                      &hKey) == ERROR_SUCCESS)
    {
        RegSetValueExW(hKey,
                       L"Attributes",
                       0,
                       REG_DWORD,
                       (LPBYTE)&_KeySettings.dwAttributes,
                       sizeof(DWORD));

        RegCloseKey(hKey);
    }

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Keyboard Layout\\Toggle",
                      0,
                      KEY_SET_VALUE,
                      &hKey) == ERROR_SUCCESS)
    {
        WCHAR szBuffer[MAX_PATH];

        StringCchPrintfW(szBuffer, ARRAYSIZE(szBuffer), L"%lu", _KeySettings.dwLanguage);

        RegSetValueExW(hKey,
                       L"Hotkey",
                       0,
                       REG_SZ,
                       (LPBYTE)szBuffer,
                       (wcslen(szBuffer) + 1) * sizeof(WCHAR));

        RegSetValueExW(hKey,
                       L"Language Hotkey",
                       0,
                       REG_SZ,
                       (LPBYTE)szBuffer,
                       (wcslen(szBuffer) + 1) * sizeof(WCHAR));

        StringCchPrintfW(szBuffer, ARRAYSIZE(szBuffer), L"%lu", _KeySettings.dwLayout);

        RegSetValueExW(hKey,
                       L"Layout Hotkey",
                       0,
                       REG_SZ,
                       (LPBYTE)szBuffer,
                       (wcslen(szBuffer) + 1) * sizeof(WCHAR));

        RegCloseKey(hKey);
    }

    /* Notice system of change hotkeys parameters */
    SystemParametersInfoW(SPI_SETLANGTOGGLE, 0, NULL, 0);

    /* Notice system of change CapsLock mode parameters */
    ActivateKeyboardLayout(GetKeyboardLayout(0), KLF_RESET | _KeySettings.dwAttributes);
}


static VOID
UpdateKeySettingsListView(HWND hwndList)
{
    WCHAR szBuffer[MAX_STR_LEN];
    LV_ITEM item;
    INT iItemIndex;

    ListView_DeleteAllItems(hwndList);

    ZeroMemory(&item, sizeof(item));

    LoadStringW(hApplet, IDS_SWITCH_BET_INLANG, szBuffer, ARRAYSIZE(szBuffer));
    item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    item.pszText = szBuffer;
    item.iItem = 0;

    iItemIndex = ListView_InsertItem(hwndList, &item);

    if (_KeySettings.dwLanguage == 1)
    {
        LoadStringW(hApplet, IDS_LEFT_ALT_SHIFT, szBuffer, ARRAYSIZE(szBuffer));
    }
    else if (_KeySettings.dwLanguage == 2)
    {
        LoadStringW(hApplet, IDS_CTRL_SHIFT, szBuffer, ARRAYSIZE(szBuffer));
    }
    else
    {
        LoadStringW(hApplet, IDS_NONE, szBuffer, ARRAYSIZE(szBuffer));
    }

    ListView_SetItemText(hwndList, iItemIndex, 1, szBuffer);
}


static VOID
OnInitKeySettingsDialog(HWND hwndDlg)
{
    LV_COLUMN column;
    HWND hwndList;

    ReadKeysSettings();

    if (_KeySettings.dwAttributes & KLF_SHIFTLOCK)
    {
        CheckDlgButton(hwndDlg, IDC_PRESS_SHIFT_KEY_RB, BST_CHECKED);
        CheckDlgButton(hwndDlg, IDC_PRESS_CL_KEY_RB, BST_UNCHECKED);
    }
    else
    {
        CheckDlgButton(hwndDlg, IDC_PRESS_SHIFT_KEY_RB, BST_UNCHECKED);
        CheckDlgButton(hwndDlg, IDC_PRESS_CL_KEY_RB, BST_CHECKED);
    }

    hwndList = GetDlgItem(hwndDlg, IDC_KEY_LISTVIEW);

    ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT);

    ZeroMemory(&column, sizeof(column));

    column.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    column.fmt      = LVCFMT_LEFT;
    column.iSubItem = 0;
    column.pszText  = L"";
    column.cx       = 210;
    ListView_InsertColumn(hwndList, 0, &column);

    column.fmt      = LVCFMT_RIGHT;
    column.cx       = 145;
    column.iSubItem = 1;
    column.pszText  = L"";
    ListView_InsertColumn(hwndList, 1, &column);

    UpdateKeySettingsListView(hwndList);
}


INT_PTR CALLBACK
KeySettingsDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            OnInitKeySettingsDialog(hwndDlg);
        }
        break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_CHANGE_KEY_SEQ_BTN:
                {
                    if (DialogBoxParamW(hApplet,
                                        MAKEINTRESOURCEW(IDD_CHANGE_KEY_SEQ),
                                        hwndDlg,
                                        ChangeKeySeqDialogProc,
                                        (LPARAM)&_KeySettings) == IDOK)
                    {
                        UpdateKeySettingsListView(GetDlgItem(hwndDlg, IDC_KEY_LISTVIEW));
                    }
                }
                break;

                case IDOK:
                {
                    if (IsDlgButtonChecked(hwndDlg, IDC_PRESS_CL_KEY_RB) == BST_CHECKED)
                    {
                        _KeySettings.dwAttributes &= ~KLF_SHIFTLOCK;
                    }
                    else
                    {
                        _KeySettings.dwAttributes |= KLF_SHIFTLOCK;
                    }

                    WriteKeysSettings();
                    EndDialog(hwndDlg, LOWORD(wParam));
                }
                break;

                case IDCANCEL:
                {
                    EndDialog(hwndDlg, LOWORD(wParam));
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
