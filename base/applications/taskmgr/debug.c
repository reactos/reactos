/*
 *  ReactOS Task Manager
 *
 *  debug.cpp
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <precomp.h>

void ProcessPage_OnDebug(void)
{
    LVITEM               lvitem;
    ULONG                Index;
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


    for (Index=0; Index<(ULONG)ListView_GetItemCount(hProcessPageListCtrl); Index++)
    {
        memset(&lvitem, 0, sizeof(LVITEM));

        lvitem.mask = LVIF_STATE;
        lvitem.stateMask = LVIS_SELECTED;
        lvitem.iItem = Index;

        (void)ListView_GetItem(hProcessPageListCtrl, &lvitem);

        if (lvitem.state & LVIS_SELECTED)
            break;
    }

    dwProcessId = PerfDataGetProcessId(Index);

    if ((ListView_GetSelectedCount(hProcessPageListCtrl) != 1) || (dwProcessId == 0))
        return;

    LoadStringW(hInst, IDS_MSG_WARNINGDEBUG, szTemp, 256);
    LoadStringW(hInst, IDS_MSG_TASKMGRWARNING, szTempA, 256);

    if (MessageBoxW(hMainWnd, szTemp, szTempA, MB_YESNO|MB_ICONWARNING) != IDYES)
    {
        GetLastErrorText(strErrorText, 260);
        LoadStringW(hInst, IDS_MSG_UNABLEDEBUGPROCESS, szTemp, 256);
        MessageBoxW(hMainWnd, strErrorText, szTemp, MB_OK|MB_ICONSTOP);
        return;
    }

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        GetLastErrorText(strErrorText, 260);
        LoadStringW(hInst, IDS_MSG_UNABLEDEBUGPROCESS, szTemp, 256);
        MessageBoxW(hMainWnd, strErrorText, szTemp, MB_OK|MB_ICONSTOP);
        return;
    }

    dwDebuggerSize = 260;
    if (RegQueryValueExW(hKey, L"Debugger", NULL, NULL, (LPBYTE)strDebugger, &dwDebuggerSize) != ERROR_SUCCESS)
    {
        GetLastErrorText(strErrorText, 260);
        LoadStringW(hInst, IDS_MSG_UNABLEDEBUGPROCESS, szTemp, 256);
        MessageBoxW(hMainWnd, strErrorText, szTemp, MB_OK|MB_ICONSTOP);
        RegCloseKey(hKey);
        return;
    }

    RegCloseKey(hKey);

    hDebugEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!hDebugEvent)
    {
        GetLastErrorText(strErrorText, 260);
        LoadStringW(hInst, IDS_MSG_UNABLEDEBUGPROCESS, szTemp, 256);
        MessageBoxW(hMainWnd, strErrorText, szTemp, MB_OK|MB_ICONSTOP);
        return;
    }

    wsprintfW(strDebugPath, strDebugger, dwProcessId, hDebugEvent);

    memset(&pi, 0, sizeof(PROCESS_INFORMATION));
    memset(&si, 0, sizeof(STARTUPINFOW));
    si.cb = sizeof(STARTUPINFOW);
    if (!CreateProcessW(NULL, strDebugPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        GetLastErrorText(strErrorText, 260);
        LoadStringW(hInst, IDS_MSG_UNABLEDEBUGPROCESS, szTemp, 256);
        MessageBoxW(hMainWnd, strErrorText, szTemp, MB_OK|MB_ICONSTOP);
    }

    CloseHandle(hDebugEvent);
}
