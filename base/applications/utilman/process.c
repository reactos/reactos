/*
 * PROJECT:         ReactOS Utility Manager (Accessibility)
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Process handling functions
 * COPYRIGHT:       Copyright 2019 Bișoc George (fraizeraust99 at gmail dot com)
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

/* FUNCTIONS ******************************************************************/

/**
 * @IsProcessRunning
 *
 * Checks if a process is running.
 *
 * @param[in]   ProcName
 *     The name of the executable process.
 *
 * @return
 *     Returns TRUE if the given process' name is running,
 *     FALSE otherwise.
 *
 */
BOOL IsProcessRunning(IN LPCWSTR lpProcessName)
{
    BOOL bIsRunning = FALSE;
    PROCESSENTRY32W Process = {0};

    /* Create a snapshot and check whether the given process' executable name is running */
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapshot == INVALID_HANDLE_VALUE)
        return FALSE;

    Process.dwSize = sizeof(Process);

    /* Enumerate the processes */
    if (Process32FirstW(hSnapshot, &Process))
    {
        do
        {
            if (_wcsicmp(Process.szExeFile, lpProcessName) == 0)
            {
                /* The process we are searching for is running */
                bIsRunning = TRUE;
                break;
            }
        }
        while (Process32NextW(hSnapshot, &Process));
    }

    /* Free the handle and return */
    CloseHandle(hSnapshot);
    return bIsRunning;
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
BOOL LaunchProcess(LPCWSTR lpProcessName)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    HANDLE hUserToken, hProcessToken;
    BOOL bSuccess;
    WCHAR ExpandedCmdLine[MAX_PATH];

    /* Expand the process path string */
    ExpandEnvironmentStringsW(lpProcessName, ExpandedCmdLine, ARRAYSIZE(ExpandedCmdLine));

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
 * @param[in]   lpProcessName
 *     The name of the executable process.
 *
 * @return
 *     Returns TRUE if the process has been terminated successfully,
 *     FALSE otherwise.
 *
 */
BOOL CloseProcess(IN LPCWSTR lpProcessName)
{
    BOOL bSuccess = FALSE;
    PROCESSENTRY32W Process = {0};

    /* Create a snapshot and check if the given process' executable name is running */
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapshot == INVALID_HANDLE_VALUE)
        return FALSE;

    Process.dwSize = sizeof(Process);

    /* Enumerate the processes */
    if (Process32FirstW(hSnapshot, &Process))
    {
        do
        {
            if (_wcsicmp(Process.szExeFile, lpProcessName) == 0)
            {
                /*
                 * We have found the process. However we must make
                 * sure that we DO NOT kill ourselves (the process ID
                 * matching with the current parent process ID).
                 */
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, Process.th32ProcessID);
                if ((hProcess != NULL) && (Process.th32ProcessID != GetCurrentProcessId()))
                {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }
                bSuccess = TRUE;
                break;
            }
        }
        while (Process32NextW(hSnapshot, &Process));
    }

    /* Free the handle and return */
    CloseHandle(hSnapshot);
    return bSuccess;
}
