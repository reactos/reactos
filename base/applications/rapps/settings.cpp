/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Functions to load / save settings from reg.
 * COPYRIGHT:   Copyright 2020 He Yang           (1160386205@qq.com)
 */

#include "rapps.h"
#include "settings.h"

#define OFFSET_OF(TYPE,MEMBER) ((size_t)&((TYPE *)0)->MEMBER)

SETTINGS_REG_ENTRY SettingsList[] = {
    {OFFSET_OF(SETTINGS_INFO, bSaveWndPos), SettingsFieldBool, 0, L"bSaveWndPos"},
    {OFFSET_OF(SETTINGS_INFO, bUpdateAtStart), SettingsFieldBool, 0, L"bUpdateAtStart"},
    {OFFSET_OF(SETTINGS_INFO, bLogEnabled), SettingsFieldBool, 0, L"bLogEnabled"},
    {OFFSET_OF(SETTINGS_INFO, szDownloadDir), SettingsFieldString, MAX_PATH, L"szDownloadDir"},
    {OFFSET_OF(SETTINGS_INFO, bDelInstaller), SettingsFieldBool, 0, L"bDelInstaller"},
    {OFFSET_OF(SETTINGS_INFO, Maximized), SettingsFieldBool, 0, L"WindowPosMaximized"},
    {OFFSET_OF(SETTINGS_INFO, Left), SettingsFieldInt, 0, L"WindowPosLeft"},
    {OFFSET_OF(SETTINGS_INFO, Top), SettingsFieldInt, 0, L"WindowPosTop"},
    {OFFSET_OF(SETTINGS_INFO, Width), SettingsFieldInt, 0, L"WindowPosWidth"},
    {OFFSET_OF(SETTINGS_INFO, Height), SettingsFieldInt, 0, L"WindowPosHeight"},
    {OFFSET_OF(SETTINGS_INFO, Proxy), SettingsFieldInt, 0, L"ProxyMode"},
    {OFFSET_OF(SETTINGS_INFO, szProxyServer), SettingsFieldString, MAX_PATH, L"ProxyServer"},
    {OFFSET_OF(SETTINGS_INFO, szNoProxyFor), SettingsFieldString, MAX_PATH, L"NoProxyFor"},
    {OFFSET_OF(SETTINGS_INFO, bUseSource), SettingsFieldBool, 0, L"bUseSource"},
    {OFFSET_OF(SETTINGS_INFO, szSourceURL), SettingsFieldString, INTERNET_MAX_URL_LENGTH, L"SourceURL"},
};

VOID FillDefaultSettings(PSETTINGS_INFO pSettingsInfo)
{
    ATL::CStringW szDownloadDir;
    ZeroMemory(pSettingsInfo, sizeof(SETTINGS_INFO));

    pSettingsInfo->bSaveWndPos = TRUE;
    pSettingsInfo->bUpdateAtStart = FALSE;
    pSettingsInfo->bLogEnabled = TRUE;
    pSettingsInfo->bUseSource = FALSE;

    if (FAILED(SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, szDownloadDir.GetBuffer(MAX_PATH))))
    {
        szDownloadDir.ReleaseBuffer();
        if (!szDownloadDir.GetEnvironmentVariableW(L"SystemDrive"))
        {
            szDownloadDir = L"C:";
        }
    }
    else
    {
        szDownloadDir.ReleaseBuffer();
    }

    PathAppendW(szDownloadDir.GetBuffer(MAX_PATH), L"\\RAPPS Downloads");
    szDownloadDir.ReleaseBuffer();

    ATL::CStringW::CopyChars(pSettingsInfo->szDownloadDir,
        _countof(pSettingsInfo->szDownloadDir),
        szDownloadDir.GetString(),
        szDownloadDir.GetLength() + 1);

    pSettingsInfo->bDelInstaller = FALSE;
    pSettingsInfo->Maximized = FALSE;
    pSettingsInfo->Left = CW_USEDEFAULT;
    pSettingsInfo->Top = CW_USEDEFAULT;
    pSettingsInfo->Width = 680;
    pSettingsInfo->Height = 450;
}

BOOL LoadSettings()
{
    ATL::CRegKey RegKey;
    if (RegKey.Open(HKEY_CURRENT_USER, L"Software\\ReactOS\\rapps", KEY_READ) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    for (UINT i = 0; i < _countof(SettingsList); i++)
    {
        void *pField = (((BYTE *)(&SettingsInfo)) + SettingsList[i].Offset);
        switch (SettingsList[i].FieldType)
        {
        case SettingsFieldBool:
        {
            DWORD dwField;
            LONG lResult = RegKey.QueryDWORDValue(SettingsList[i].RegKeyName, dwField);
            if (lResult != ERROR_SUCCESS)
            {
                RegKey.Close();
                return FALSE;
            }
            *((BOOL *)pField) = (BOOL)dwField;
            break;
        }
        case SettingsFieldInt:
        {
            DWORD dwField;
            LONG lResult = RegKey.QueryDWORDValue(SettingsList[i].RegKeyName, dwField);
            if (lResult != ERROR_SUCCESS)
            {
                RegKey.Close();
                return FALSE;
            }
            *((INT *)pField) = (INT)dwField;
            break;
        }
        case SettingsFieldString:
        {
            ULONG nChar = SettingsList[i].cchStrlen - 1; // make sure the terminating L'\0'
            LONG lResult = RegKey.QueryStringValue(SettingsList[i].RegKeyName, ((WCHAR *)pField), &nChar);
            if (lResult != ERROR_SUCCESS)
            {
                RegKey.Close();
                return FALSE;
            }
        }
        }
    }
    RegKey.Close();

    return TRUE;
}

VOID SaveSettings(HWND hwnd)
{
    WINDOWPLACEMENT wp;
    ATL::CRegKey RegKey;

    if (SettingsInfo.bSaveWndPos)
    {
        wp.length = sizeof(wp);
        GetWindowPlacement(hwnd, &wp);

        SettingsInfo.Left = wp.rcNormalPosition.left;
        SettingsInfo.Top = wp.rcNormalPosition.top;
        SettingsInfo.Width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
        SettingsInfo.Height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
        SettingsInfo.Maximized = (wp.showCmd == SW_MAXIMIZE
            || (wp.showCmd == SW_SHOWMINIMIZED
                && (wp.flags & WPF_RESTORETOMAXIMIZED)));
    }

    if (RegKey.Create(HKEY_CURRENT_USER, L"Software\\ReactOS\\rapps", NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, NULL) == ERROR_SUCCESS)
    {
        for (UINT i = 0; i < _countof(SettingsList); i++)
        {
            void *pField = (((BYTE *)(&SettingsInfo)) + SettingsList[i].Offset);
            switch (SettingsList[i].FieldType)
            {
            case SettingsFieldBool:
            {
                DWORD dwField = *((BOOL *)pField);
                RegKey.SetDWORDValue(SettingsList[i].RegKeyName, dwField);
                break;
            }
            case SettingsFieldInt:
            {
                DWORD dwField = *((INT *)pField);
                RegKey.SetDWORDValue(SettingsList[i].RegKeyName, dwField);
                break;
            }
            case SettingsFieldString:
            {
                WCHAR * szField = ((WCHAR *)pField);
                RegKey.SetStringValue(SettingsList[i].RegKeyName, szField);
                break;
            }
            }
        }
        
        RegKey.Close();
    }
}
