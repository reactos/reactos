/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Process Debugging
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 *              Copyright 2005 Klemens Friedl <frik85@reactos.at>
 */

#include "precomp.h"

void ProcessPage_OnDebug(void)
{
    DWORD                dwProcessId;
    WCHAR                strErrorText[260];
    HKEY                 hKey;
    WCHAR                strDebugPath[260];
    WCHAR                strDebugger[260];
    DWORD                dwDebuggerSize;
    PROCESS_INFORMATION  pi;
    STARTUPINFOW         si;
    HANDLE               hDebugEvent;
    WCHAR                szTemp[256];
    WCHAR                szTempA[256];

    dwProcessId = GetSelectedProcessId();

    if (dwProcessId == 0)
        return;

    LoadStringW(hInst, IDS_MSG_WARNINGDEBUG, szTemp, _countof(szTemp));
    LoadStringW(hInst, IDS_MSG_TASKMGRWARNING, szTempA, _countof(szTempA));

    if (!ConfirmMessageBox(hMainWnd, szTemp, szTempA, MB_YESNO | MB_ICONWARNING))
    {
        GetLastErrorText(strErrorText, _countof(strErrorText));
        LoadStringW(hInst, IDS_MSG_UNABLEDEBUGPROCESS, szTemp, _countof(szTemp));
        MessageBoxW(hMainWnd, strErrorText, szTemp, MB_OK | MB_ICONSTOP);
        return;
    }

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug", 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
    {
        GetLastErrorText(strErrorText, _countof(strErrorText));
        LoadStringW(hInst, IDS_MSG_UNABLEDEBUGPROCESS, szTemp, _countof(szTemp));
        MessageBoxW(hMainWnd, strErrorText, szTemp, MB_OK | MB_ICONSTOP);
        return;
    }

    dwDebuggerSize = sizeof(strDebugger);
    if (RegQueryValueExW(hKey, L"Debugger", NULL, NULL, (LPBYTE)strDebugger, &dwDebuggerSize) != ERROR_SUCCESS)
    {
        GetLastErrorText(strErrorText, _countof(strErrorText));
        LoadStringW(hInst, IDS_MSG_UNABLEDEBUGPROCESS, szTemp, _countof(szTemp));
        MessageBoxW(hMainWnd, strErrorText, szTemp, MB_OK | MB_ICONSTOP);
        RegCloseKey(hKey);
        return;
    }

    RegCloseKey(hKey);

    hDebugEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!hDebugEvent)
    {
        GetLastErrorText(strErrorText, _countof(strErrorText));
        LoadStringW(hInst, IDS_MSG_UNABLEDEBUGPROCESS, szTemp, _countof(szTemp));
        MessageBoxW(hMainWnd, strErrorText, szTemp, MB_OK | MB_ICONSTOP);
        return;
    }

    wsprintfW(strDebugPath, strDebugger, dwProcessId, hDebugEvent);

    ZeroMemory(&pi, sizeof(pi));
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    if (!CreateProcessW(NULL, strDebugPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        GetLastErrorText(strErrorText, _countof(strErrorText));
        LoadStringW(hInst, IDS_MSG_UNABLEDEBUGPROCESS, szTemp, _countof(szTemp));
        MessageBoxW(hMainWnd, strErrorText, szTemp, MB_OK | MB_ICONSTOP);
    }
    else
    {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }

    CloseHandle(hDebugEvent);
}
