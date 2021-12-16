/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Process Priority.
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 *              Copyright 2005 Klemens Friedl <frik85@reactos.at>
 */

#include "precomp.h"

void DoSetPriority(DWORD priority)
{
    DWORD   dwProcessId;
    HANDLE  hProcess;
    WCHAR   szText[260];
    WCHAR   szTitle[256];

    dwProcessId = GetSelectedProcessId();

    if (dwProcessId == 0)
        return;

    LoadStringW(hInst, IDS_MSG_TASKMGRWARNING, szTitle, 256);
    LoadStringW(hInst, IDS_MSG_WARNINGCHANGEPRIORITY, szText, 260);
    if (MessageBoxW(hMainWnd, szText, szTitle, MB_YESNO|MB_ICONWARNING) != IDYES)
        return;

    hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, dwProcessId);

    if (!hProcess)
    {
        GetLastErrorText(szText, 260);
        LoadStringW(hInst, IDS_MSG_UNABLECHANGEPRIORITY, szTitle, 256);
        MessageBoxW(hMainWnd, szText, szTitle, MB_OK|MB_ICONSTOP);
        return;
    }

    if (!SetPriorityClass(hProcess, priority))
    {
        GetLastErrorText(szText, 260);
        LoadStringW(hInst, IDS_MSG_UNABLECHANGEPRIORITY, szTitle, 256);
        MessageBoxW(hMainWnd, szText, szTitle, MB_OK|MB_ICONSTOP);
    }

    CloseHandle(hProcess);
}
