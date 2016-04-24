/*
 *  ReactOS Task Manager
 *
 *  endproc.c
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer               <brianp@reactos.org>
 *                2005         Klemens Friedl             <frik85@reactos.at>
 *                2014         Ismael Ferreras Morezuelas <swyterzone+ros@gmail.com>
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

#define NTOS_MODE_USER
#include <ndk/psfuncs.h>

void ProcessPage_OnEndProcess(void)
{
    DWORD   dwProcessId;
    HANDLE  hProcess;
    WCHAR   szTitle[256];
    WCHAR   strErrorText[260];

    dwProcessId = GetSelectedProcessId();

    if (dwProcessId == 0)
        return;

    hProcess = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION, FALSE, dwProcessId);

    /* forbid killing system processes even if we have privileges -- sigh, windows kludge! */
    if (hProcess && IsCriticalProcess(hProcess))
    {
        LoadStringW(hInst, IDS_MSG_UNABLETERMINATEPRO, szTitle, 256);
        LoadStringW(hInst, IDS_MSG_CLOSESYSTEMPROCESS, strErrorText, 256);
        MessageBoxW(hMainWnd, strErrorText, szTitle, MB_OK|MB_ICONWARNING|MB_TOPMOST);
        CloseHandle(hProcess);
        return;
    }

    /* if this is a standard process just ask for confirmation before doing it */
    LoadStringW(hInst, IDS_MSG_WARNINGTERMINATING, strErrorText, 256);
    LoadStringW(hInst, IDS_MSG_TASKMGRWARNING, szTitle, 256);
    if (MessageBoxW(hMainWnd, strErrorText, szTitle, MB_YESNO|MB_ICONWARNING|MB_TOPMOST) != IDYES)
    {
        if (hProcess) CloseHandle(hProcess);
        return;
    }

    /* no such process or not enough privileges to open its token */
    if (!hProcess)
    {
        GetLastErrorText(strErrorText, 260);
        LoadStringW(hInst, IDS_MSG_UNABLETERMINATEPRO, szTitle, 256);
        MessageBoxW(hMainWnd, strErrorText, szTitle, MB_OK|MB_ICONSTOP|MB_TOPMOST);
        return;
    }

    /* try to kill it, and notify the user if didn't work */
    if (!TerminateProcess(hProcess, 1))
    {
        GetLastErrorText(strErrorText, 260);
        LoadStringW(hInst, IDS_MSG_UNABLETERMINATEPRO, szTitle, 256);
        MessageBoxW(hMainWnd, strErrorText, szTitle, MB_OK|MB_ICONSTOP|MB_TOPMOST);
    }

    CloseHandle(hProcess);
}

BOOL IsCriticalProcess(HANDLE hProcess)
{
    NTSTATUS status;
    ULONG BreakOnTermination;

    /* return early if the process handle does not exist */
    if (!hProcess)
        return FALSE;

    /* the important system processes that we don't want to let the user
       kill come marked as critical, this simplifies the check greatly.

       a critical process brings the system down when is terminated:
       <http://www.geoffchappell.com/studies/windows/win32/ntdll/api/rtl/peb/setprocessiscritical.htm> */

    status = NtQueryInformationProcess(hProcess,
                                       ProcessBreakOnTermination,
                                       &BreakOnTermination,
                                       sizeof(ULONG),
                                       NULL);

    if (NT_SUCCESS(status) && BreakOnTermination)
        return TRUE;

    return FALSE;
}

void ProcessPage_OnEndProcessTree(void)
{
    DWORD   dwProcessId;
    HANDLE  hProcess;
    WCHAR   szTitle[256];
    WCHAR   strErrorText[260];

    dwProcessId = GetSelectedProcessId();

    if (dwProcessId == 0)
        return;

    hProcess = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION, FALSE, dwProcessId);

    /* forbid killing system processes even if we have privileges -- sigh, windows kludge! */
    if (hProcess && IsCriticalProcess(hProcess))
    {
        LoadStringW(hInst, IDS_MSG_UNABLETERMINATEPRO, szTitle, 256);
        LoadStringW(hInst, IDS_MSG_CLOSESYSTEMPROCESS, strErrorText, 256);
        MessageBoxW(hMainWnd, strErrorText, szTitle, MB_OK|MB_ICONWARNING|MB_TOPMOST);
        CloseHandle(hProcess);
        return;
    }

    LoadStringW(hInst, IDS_MSG_WARNINGTERMINATING, strErrorText, 256);
    LoadStringW(hInst, IDS_MSG_TASKMGRWARNING, szTitle, 256);
    if (MessageBoxW(hMainWnd, strErrorText, szTitle, MB_YESNO|MB_ICONWARNING) != IDYES)
    {
        if (hProcess) CloseHandle(hProcess);
        return;
    }

    if (!hProcess)
    {
        GetLastErrorText(strErrorText, 260);
        LoadStringW(hInst, IDS_MSG_UNABLETERMINATEPRO, szTitle, 256);
        MessageBoxW(hMainWnd, strErrorText, szTitle, MB_OK|MB_ICONSTOP);
        return;
    }

    if (!TerminateProcess(hProcess, 0))
    {
        GetLastErrorText(strErrorText, 260);
        LoadStringW(hInst, IDS_MSG_UNABLETERMINATEPRO, szTitle, 256);
        MessageBoxW(hMainWnd, strErrorText, szTitle, MB_OK|MB_ICONSTOP);
    }

    CloseHandle(hProcess);
}
