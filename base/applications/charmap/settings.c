/*
 * PROJECT:     ReactOS Character Map
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/charmap/settings.c
 * PURPOSE:     save/load settings
 * COPYRIGHT:   Copyright 2012 Edijs Kolesnikovics <terminedijs@yahoo.com>
 *
 */

#include "precomp.h"

#include <winreg.h>
#include <windowsx.h>
#include <tchar.h>

const TCHAR g_szGeneralRegKey[] = _T("Software\\Microsoft\\CharMap");
HWND hWnd;

LONG QueryStringValue(HKEY hKey, LPCTSTR lpSubKey, LPCTSTR lpValueName, LPTSTR pszBuffer, DWORD dwBufferLen)
{
    LONG lResult;
    HKEY hSubKey = NULL;
    DWORD cbData, dwType;

    if (lpSubKey)
    {
        lResult = RegOpenKey(hKey, lpSubKey, &hSubKey);
        if (lResult != ERROR_SUCCESS)
            goto done;
        hKey = hSubKey;
    }

    cbData = (dwBufferLen - 1) * sizeof(*pszBuffer);
    lResult = RegQueryValueEx(hKey, lpValueName, NULL, &dwType, (LPBYTE) pszBuffer, &cbData);
    if (lResult != ERROR_SUCCESS)
        goto done;
    if (dwType != REG_SZ)
    {
        lResult = -1;
        goto done;
    }

    pszBuffer[cbData / sizeof(*pszBuffer)] = _T('\0');

done:
    if (lResult != ERROR_SUCCESS)
        pszBuffer[0] = _T('\0');
    if (hSubKey)
        RegCloseKey(hSubKey);
    return lResult;
}

extern void LoadSettings(void)
{
    HKEY hKey = NULL;
    int iItemIndex = -1;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, g_szGeneralRegKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        TCHAR szBuffer[MAX_PATH];
        DWORD dwAdvancedChecked;
        DWORD type, size = sizeof(dwAdvancedChecked);
        LSTATUS lstatus;

        /* Restore last selected font */
        if (QueryStringValue(HKEY_CURRENT_USER, g_szGeneralRegKey, _T("Font"), szBuffer, (sizeof(szBuffer)/sizeof(szBuffer[0]))) == ERROR_SUCCESS)
        {
            //Get combobox handle
            hWnd = GetDlgItem(hCharmapDlg, IDC_FONTCOMBO);

            //Search for match and return index if match found
            iItemIndex = ComboBox_FindStringExact(hWnd, -1, szBuffer);
            if(iItemIndex != CB_ERR)
            {
                ComboBox_SetCurSel(hWnd, iItemIndex);
                ChangeMapFont(hCharmapDlg);
            }
        }

        /* Restore last selected character set */
        if (QueryStringValue(HKEY_CURRENT_USER, g_szGeneralRegKey, _T("CodePage"), szBuffer, (sizeof(szBuffer)/sizeof(szBuffer[0]))) == ERROR_SUCCESS)
        {
            //Get combobox handle
            hWnd = GetDlgItem(hCharmapDlg, IDC_COMBO_CHARSET);

            iItemIndex = ComboBox_FindStringExact(hWnd, -1, szBuffer);
            if(iItemIndex != CB_ERR)
            {
                ComboBox_SetCurSel(hWnd, iItemIndex);
            }
        }

        lstatus = RegQueryValueEx(hKey, _T("Advanced"), NULL, &type, (LPBYTE)&dwAdvancedChecked, &size);
        if (lstatus == ERROR_SUCCESS && type == REG_DWORD && dwAdvancedChecked != FALSE)
        {
            SendDlgItemMessage(hCharmapDlg, IDC_CHECK_ADVANCED, BM_CLICK, 0, 0);
        }

    RegCloseKey(hKey);
    }
    else
    {
        /* Default font seems to be Arial */
        hWnd = GetDlgItem(hCharmapDlg, IDC_FONTCOMBO);

        iItemIndex = ComboBox_FindStringExact(hWnd, -1, _T("Arial"));
        if(iItemIndex != CB_ERR)
        {
            ComboBox_SetCurSel(hWnd, iItemIndex);
            ChangeMapFont(hCharmapDlg);
        }
    }
}

extern void SaveSettings(void)
{
    HKEY hKey = NULL;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, g_szGeneralRegKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        TCHAR szBuffer[MAX_PATH];

        hWnd = GetDlgItem(hCharmapDlg, IDC_FONTCOMBO);
        ComboBox_GetText(hWnd, szBuffer, MAX_PATH);

        if(*szBuffer != '\0')
            RegSetValueEx(hKey, _T("Font"), 0, REG_SZ, (LPBYTE) szBuffer, (DWORD) MAX_PATH);

        hWnd = GetDlgItem(hCharmapDlg, IDC_COMBO_CHARSET);
        ComboBox_GetText(hWnd, szBuffer, MAX_PATH);

        if(*szBuffer != '\0')
            RegSetValueEx(hKey, _T("CodePage"), 0, REG_SZ, (LPBYTE) szBuffer, (DWORD) MAX_PATH);

        RegSetValueEx(hKey, _T("Advanced"), 0, REG_DWORD, (LPBYTE)&Settings.IsAdvancedView, (DWORD) sizeof(DWORD));

        RegCloseKey(hKey);
    }
}
