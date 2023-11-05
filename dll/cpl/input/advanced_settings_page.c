/*
* PROJECT:         input.dll
* FILE:            dll/cpl/input/advanced_settings_page.c
* PURPOSE:         input.dll
* PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
*                  Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
*/

#include "input.h"

BOOL g_bTextServiceIsOff = FALSE;

BOOL LoadAdvancedSettings(HWND hwndDlg)
{
    HKEY hKey;
    LONG error;
    DWORD dwValue = FALSE;
    DWORD cbValue = sizeof(dwValue);

    error = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\CTF", 0, KEY_READ, &hKey);
    if (error != ERROR_SUCCESS)
        return FALSE;

    RegQueryValueExW(hKey, L"Disable Thread Input Manager", NULL, NULL,
                     (LPBYTE)&dwValue, &cbValue);
    RegCloseKey(hKey);

    CheckDlgButton(hwndDlg, IDC_TURNOFFTEXTSVCS_CB, (dwValue ? BST_CHECKED : BST_UNCHECKED));
    g_bTextServiceIsOff = !!dwValue;
    return TRUE;
}

BOOL SaveAdvancedSettings(HWND hwndDlg)
{
    HKEY hKey;
    LONG error;
    const DWORD dwValue = g_bTextServiceIsOff, cbValue = sizeof(dwValue);

    error = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\CTF", 0, KEY_WRITE, &hKey);
    if (error != ERROR_SUCCESS)
        return FALSE;

    error = RegSetValueExW(hKey, L"Disable Thread Input Manager", 0, REG_DWORD,
                           (const BYTE *)&dwValue, cbValue);

    RegCloseKey(hKey);
    return error == ERROR_SUCCESS;
}

static INT_PTR OnNotifyAdvancedSettingsPage(HWND hwndDlg, LPARAM lParam)
{
    LPNMHDR header = (LPNMHDR)lParam;

    switch (header->code)
    {
        case PSN_APPLY:
        {
            BOOL bOff = (IsDlgButtonChecked(hwndDlg, IDC_TURNOFFTEXTSVCS_CB) == BST_CHECKED);
            g_bRebootNeeded |= (g_bTextServiceIsOff && !bOff);
            g_bTextServiceIsOff = bOff;
            break;
        }
    }

    return 0;
}

VOID OnCommandAdvancedSettingsPage(HWND hwndDlg, WPARAM wParam)
{
    switch (LOWORD(wParam))
    {
        case IDC_TURNOFFTEXTSVCS_CB:
            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            break;
    }
}

INT_PTR CALLBACK
AdvancedSettingsPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            LoadAdvancedSettings(hwndDlg);
            return TRUE;

        case WM_NOTIFY:
            return OnNotifyAdvancedSettingsPage(hwndDlg, lParam);

        case WM_COMMAND:
            OnCommandAdvancedSettingsPage(hwndDlg, wParam);
            break;
    }

    return 0;
}
