/*
 *  ReactOS Task Manager
 *
 *  debug.c
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *                2005         Klemens Friedl <frik85@reactos.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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

    LoadStringW(hInst, IDS_MSG_WARNINGDEBUG, szTemp, ARRAYSIZE(szTemp));
    LoadStringW(hInst, IDS_MSG_TASKMGRWARNING, szTempA, ARRAYSIZE(szTempA));

    if (MessageBoxW(hMainWnd, szTemp, szTempA, MB_YESNO | MB_ICONWARNING) != IDYES)
    {
        GetLastErrorText(strErrorText, ARRAYSIZE(strErrorText));
        LoadStringW(hInst, IDS_MSG_UNABLEDEBUGPROCESS, szTemp, ARRAYSIZE(szTemp));
        MessageBoxW(hMainWnd, strErrorText, szTemp, MB_OK | MB_ICONSTOP);
        return;
    }

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug", 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
    {
        GetLastErrorText(strErrorText, ARRAYSIZE(strErrorText));
        LoadStringW(hInst, IDS_MSG_UNABLEDEBUGPROCESS, szTemp, ARRAYSIZE(szTemp));
        MessageBoxW(hMainWnd, strErrorText, szTemp, MB_OK | MB_ICONSTOP);
        return;
    }

    dwDebuggerSize = sizeof(strDebugger);
    if (RegQueryValueExW(hKey, L"Debugger", NULL, NULL, (LPBYTE)strDebugger, &dwDebuggerSize) != ERROR_SUCCESS)
    {
        GetLastErrorText(strErrorText, ARRAYSIZE(strErrorText));
        LoadStringW(hInst, IDS_MSG_UNABLEDEBUGPROCESS, szTemp, ARRAYSIZE(szTemp));
        MessageBoxW(hMainWnd, strErrorText, szTemp, MB_OK | MB_ICONSTOP);
        RegCloseKey(hKey);
        return;
    }

    RegCloseKey(hKey);

    hDebugEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!hDebugEvent)
    {
        GetLastErrorText(strErrorText, ARRAYSIZE(strErrorText));
        LoadStringW(hInst, IDS_MSG_UNABLEDEBUGPROCESS, szTemp, ARRAYSIZE(szTemp));
        MessageBoxW(hMainWnd, strErrorText, szTemp, MB_OK | MB_ICONSTOP);
        return;
    }

    wsprintfW(strDebugPath, strDebugger, dwProcessId, hDebugEvent);

    ZeroMemory(&pi, sizeof(pi));
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    if (!CreateProcessW(NULL, strDebugPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        GetLastErrorText(strErrorText, ARRAYSIZE(strErrorText));
        LoadStringW(hInst, IDS_MSG_UNABLEDEBUGPROCESS, szTemp, ARRAYSIZE(szTemp));
        MessageBoxW(hMainWnd, strErrorText, szTemp, MB_OK | MB_ICONSTOP);
    }
    else
    {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }

    CloseHandle(hDebugEvent);
}
