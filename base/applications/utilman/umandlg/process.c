/*
 * PROJECT:         ReactOS Utility Manager Resources DLL (UManDlg.dll)
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Process handling functions
 * COPYRIGHT:       Copyright 2019-2020 George Bișoc (george.bisoc@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "umandlg.h"

/* FUNCTIONS ******************************************************************/

/**
 * @GetProcessID
 *
 * Returns the process executable ID based on the given executable name.
 *
 * @param[in]   lpszProcessName
 *     The name of the executable process.
 *
 * @return
 *      Returns the ID number of the process, otherwise 0.
 *
 */
DWORD GetProcessID(IN LPCWSTR lpszProcessName)
{
    PROCESSENTRY32W Process;

    /* Create a snapshot and check if the given process name matches with the one from the process entry structure */
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapshot == INVALID_HANDLE_VALUE)
        return 0;

    /* Get the whole size of the structure */
    Process.dwSize = sizeof(Process);

    /* Enumerate the processes */
    if (Process32FirstW(hSnapshot, &Process))
    {
        do
        {
            if (_wcsicmp(Process.szExeFile, lpszProcessName) == 0)
            {
                /* The names match, return the process ID we're interested */
                CloseHandle(hSnapshot);
                return Process.th32ProcessID;
            }
        }
        while (Process32NextW(hSnapshot, &Process));
    }

    CloseHandle(hSnapshot);
    return 0;
}

/**
 * @IsProcessRunning
 *
 * Checks if a process is running.
 *
 * @param[in]   lpszProcessName
 *     The name of the executable process.
 *
 * @return
 *     Returns TRUE if the given process' name is running,
 *     FALSE otherwise.
 *
 */
BOOL IsProcessRunning(IN LPCWSTR lpszProcessName)
{
    DWORD dwReturn, dwProcessID;
    HANDLE hProcess;

    /* Get the process ID */
    dwProcessID = GetProcessID(lpszProcessName);
    if (dwProcessID == 0)
    {
        return FALSE;
    }

    /* Synchronize the process to get its signaling state */
    hProcess = OpenProcess(SYNCHRONIZE, FALSE, dwProcessID);
    if (!hProcess)
    {
        DPRINT("IsProcessRunning(): Failed to open the process! (Error: %lu)", GetLastError());
        return FALSE;
    }

    /* Wait for the process */
    dwReturn = WaitForSingleObject(hProcess, 0);
    if (dwReturn == WAIT_TIMEOUT)
    {
        /* The process is still running */
        CloseHandle(hProcess);
        return TRUE;
    }

    CloseHandle(hProcess);
    return FALSE;
}

/**
 * @LaunchProcess
 *
 * Executes a process.
 *
 * @param[in]   lpProcessName
 *     The name of the executable process.
 *
 * @return
 *     Returns TRUE if the process has been launched successfully,
 *     FALSE otherwise.
 *
 */
BOOL LaunchProcess(IN LPCWSTR lpszProcessName)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    HANDLE hUserToken, hProcessToken;
    BOOL bSuccess;
    WCHAR ExpandedCmdLine[MAX_PATH];

    /* Expand the process path string */
    ExpandEnvironmentStringsW(lpszProcessName, ExpandedCmdLine, ARRAYSIZE(ExpandedCmdLine));

    ZeroMemory(&pi, sizeof(pi));
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWNORMAL;

    /* Get the token of the parent (current) process of the application */
    bSuccess = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE, &hUserToken);
    if (!bSuccess)
    {
        DPRINT("OpenProcessToken() failed with error -> %lu\n", GetLastError());
        return FALSE;
    }

    /* Duplicate a new token so that we can use it to create our process */
    bSuccess = DuplicateTokenEx(hUserToken, TOKEN_ALL_ACCESS, NULL, SecurityIdentification, TokenPrimary, &hProcessToken);
    if (!bSuccess)
    {
        DPRINT("DuplicateTokenEx() failed with error -> %lu\n", GetLastError());
        CloseHandle(hUserToken);
        return FALSE;
    }

    /* Finally create the process */
    bSuccess = CreateProcessAsUserW(hProcessToken,
                                    NULL,
                                    ExpandedCmdLine,
                                    NULL,
                                    NULL,
                                    FALSE,
                                    0, // DETACHED_PROCESS, NORMAL_PRIORITY_CLASS
                                    NULL,
                                    NULL,
                                    &si,
                                    &pi);

    if (!bSuccess)
    {
        DPRINT("CreateProcessAsUserW() failed with error -> %lu\n", GetLastError());
        CloseHandle(hUserToken);
        CloseHandle(hProcessToken);
        return FALSE;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hUserToken);
    CloseHandle(hProcessToken);
    return TRUE;
}

/**
 * @CloseProcess
 *
 * Closes a process.
 *
 * @param[in]   lpszProcessName
 *     The name of the executable process.
 *
 * @return
 *     Returns TRUE if the process has been terminated successfully,
 *     FALSE otherwise.
 *
 */
BOOL CloseProcess(IN LPCWSTR lpszProcessName)
{
    HANDLE hProcess;
    DWORD dwProcessID;

    /* Get the process ID */
    dwProcessID = GetProcessID(lpszProcessName);
    if (dwProcessID == 0)
    {
        return FALSE;
    }

    /* Make sure that the given process ID is not ours, the parent process, so that we do not kill ourselves */
    if (dwProcessID == GetCurrentProcessId())
    {
        return FALSE;
    }

    /* Open the process so that we can terminate it */
    hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwProcessID);
    if (!hProcess)
    {
        DPRINT("CloseProcess(): Failed to open the process for termination! (Error: %lu)", GetLastError());
        return FALSE;
    }

    /* Terminate it */
    TerminateProcess(hProcess, 0);
    CloseHandle(hProcess);
    return TRUE;
}
