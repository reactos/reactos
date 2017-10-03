/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions for writing to the Event Log
 * COPYRIGHT:   Copyright 2009 Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "precomp.h"
HANDLE hLog = NULL;

VOID
InitLogs(VOID)
{
    WCHAR szBuf[MAX_PATH] = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\RosAutotest";
    WCHAR szPath[MAX_PATH];
    DWORD dwCategoryNum = 1;
    DWORD dwDisp, dwData;
    HKEY hKey;

    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                        szBuf, 0, NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE, NULL, &hKey, &dwDisp) != ERROR_SUCCESS)
    {
        return;
    }

    if (!GetModuleFileName(NULL, szPath, sizeof(szPath) / sizeof(szPath[0])))
        return;

    if (RegSetValueExW(hKey,
                       L"EventMessageFile",
                       0,
                       REG_EXPAND_SZ,
                       (LPBYTE)szPath,
                       (DWORD)(wcslen(szPath) + 1) * sizeof(WCHAR)) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return;
    }

    dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE |
             EVENTLOG_INFORMATION_TYPE;

    if (RegSetValueExW(hKey,
                       L"TypesSupported",
                       0,
                       REG_DWORD,
                       (LPBYTE)&dwData,
                       sizeof(DWORD)) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return;
    }

    if (RegSetValueExW(hKey,
                       L"CategoryMessageFile",
                       0,
                       REG_EXPAND_SZ,
                       (LPBYTE)szPath,
                       (DWORD)(wcslen(szPath) + 1) * sizeof(WCHAR)) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return;
    }

    if (RegSetValueExW(hKey,
                       L"CategoryCount",
                       0,
                       REG_DWORD,
                       (LPBYTE)&dwCategoryNum,
                       sizeof(DWORD)) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return;
    }

    RegCloseKey(hKey);

    hLog = RegisterEventSourceW(NULL, L"RosAutotest");
}


VOID
FreeLogs(VOID)
{
    if (hLog) DeregisterEventSource(hLog);
}


BOOL
WriteLogMessage(WORD wType, DWORD dwEventID, LPWSTR lpMsg)
{
    if (!ReportEventW(hLog,
                      wType,
                      0,
                      dwEventID,
                      NULL,
                      1,
                      0,
                      (LPCWSTR*)&lpMsg,
                      NULL))
    {
        return FALSE;
    }

    return TRUE;
}
