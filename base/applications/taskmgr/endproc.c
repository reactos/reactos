/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Process Termination.
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 *              Copyright 2005 Klemens Friedl <frik85@reactos.at>
 *              Copyright 2014 Ismael Ferreras Morezuelas <swyterzone+ros@gmail.com>
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

BOOL ShutdownProcessTreeHelper(HANDLE hSnapshot, HANDLE hParentProcess, DWORD dwParentPID)
{
    HANDLE hChildHandle;
    PROCESSENTRY32W ProcessEntry = {0};
    ProcessEntry.dwSize = sizeof(ProcessEntry);

    if (Process32FirstW(hSnapshot, &ProcessEntry))
    {
        do
        {
            if (ProcessEntry.th32ParentProcessID == dwParentPID)
            {
                hChildHandle = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION,
                                           FALSE,
                                           ProcessEntry.th32ProcessID);
                if (!hChildHandle || IsCriticalProcess(hChildHandle))
                {
                    if (hChildHandle)
                    {
                        CloseHandle(hChildHandle);
                    }
                    continue;
                }
                if (!ShutdownProcessTreeHelper(hSnapshot, hChildHandle, ProcessEntry.th32ProcessID))
                {
                    CloseHandle(hChildHandle);
                    return FALSE;
                }
                CloseHandle(hChildHandle);
            }
        } while (Process32NextW(hSnapshot, &ProcessEntry));
    }

    return TerminateProcess(hParentProcess, 0);
}

BOOL ShutdownProcessTree(HANDLE hParentProcess, DWORD dwParentPID)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    BOOL bResult;

    if (!hSnapshot)
    {
        return FALSE;
    }

    bResult = ShutdownProcessTreeHelper(hSnapshot, hParentProcess, dwParentPID);
    CloseHandle(hSnapshot);
    return bResult;
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

    if (!ShutdownProcessTree(hProcess, dwProcessId))
    {
        GetLastErrorText(strErrorText, 260);
        LoadStringW(hInst, IDS_MSG_UNABLETERMINATEPRO, szTitle, 256);
        MessageBoxW(hMainWnd, strErrorText, szTitle, MB_OK|MB_ICONSTOP);
    }

    CloseHandle(hProcess);
}
