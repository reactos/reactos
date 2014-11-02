/*
 * Copyright (C) 2002 Andreas Mohr
 * Copyright (C) 2002 Shachar Shemesh
 * Copyright (C) 2013 Edijs Kolesnikovics
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

/* Based on the Wine "bootup" handler application
 *
 * This app handles the various "hooks" windows allows for applications to perform
 * as part of the bootstrap process. Theses are roughly devided into three types.
 * Knowledge base articles that explain this are 137367, 179365, 232487 and 232509.
 * Also, 119941 has some info on grpconv.exe
 * The operations performed are (by order of execution):
 *
 * After log in
 * - HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\RunOnceEx (synch, no imp)
 * - HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\RunOnce (synch)
 * - HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Run (asynch)
 * - HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run (asynch)
 * - All users Startup folder "%ALLUSERSPROFILE%\Start Menu\Programs\Startup" (asynch, no imp)
 * - Current user Startup folder "%USERPROFILE%\Start Menu\Programs\Startup" (asynch, no imp)
 * - HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\RunOnce (asynch)
 *
 * None is processed in Safe Mode // FIXME: Check RunOnceEx in Safe Mode
 */

#include "precomp.h"

EXTERN_C HRESULT WINAPI SHCreateSessionKey(REGSAM samDesired, PHKEY phKey);

#define INVALID_RUNCMD_RETURN -1
/**
 * This function runs the specified command in the specified dir.
 * [in,out] cmdline - the command line to run. The function may change the passed buffer.
 * [in] dir - the dir to run the command in. If it is NULL, then the current dir is used.
 * [in] wait - whether to wait for the run program to finish before returning.
 * [in] minimized - Whether to ask the program to run minimized.
 *
 * Returns:
 * If running the process failed, returns INVALID_RUNCMD_RETURN. Use GetLastError to get the error code.
 * If wait is FALSE - returns 0 if successful.
 * If wait is TRUE - returns the program's return value.
 */
static int runCmd(LPWSTR cmdline, LPCWSTR dir, BOOL wait, BOOL minimized)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    DWORD exit_code = 0;
    WCHAR szCmdLineExp[MAX_PATH+1] = L"\0";

    ExpandEnvironmentStringsW(cmdline, szCmdLineExp, sizeof(szCmdLineExp) / sizeof(WCHAR));

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    if (minimized)
    {
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_MINIMIZE;
    }
    memset(&info, 0, sizeof(info));

    if (!CreateProcessW(NULL, szCmdLineExp, NULL, NULL, FALSE, 0, NULL, dir, &si, &info))
    {
        TRACE("Failed to run command (%lu)\n", GetLastError());

        return INVALID_RUNCMD_RETURN;
    }

    TRACE("Successfully ran command\n");

    if (wait)
    {   /* wait for the process to exit */
        WaitForSingleObject(info.hProcess, INFINITE);
        GetExitCodeProcess(info.hProcess, &exit_code);
    }

    CloseHandle(info.hThread);
    CloseHandle(info.hProcess);

    return exit_code;
}


/**
 * Process a "Run" type registry key.
 * hkRoot is the HKEY from which "Software\Microsoft\Windows\CurrentVersion" is
 *      opened.
 * szKeyName is the key holding the actual entries.
 * bDelete tells whether we should delete each value right before executing it.
 * bSynchronous tells whether we should wait for the prog to complete before
 *      going on to the next prog.
 */
static BOOL ProcessRunKeys(HKEY hkRoot, LPCWSTR szKeyName, BOOL bDelete,
        BOOL bSynchronous)
{
    HKEY hkWin = NULL, hkRun = NULL;
    LONG res = ERROR_SUCCESS;
    DWORD i, cbMaxCmdLine = 0, cchMaxValue = 0;
    WCHAR *szCmdLine = NULL;
    WCHAR *szValue = NULL;

    if (hkRoot == HKEY_LOCAL_MACHINE)
        TRACE("processing %ls entries under HKLM\n", szKeyName);
    else
        TRACE("processing %ls entries under HKCU\n", szKeyName);

    res = RegOpenKeyExW(hkRoot,
                        L"Software\\Microsoft\\Windows\\CurrentVersion",
                        0,
                        KEY_READ,
                        &hkWin);
    if (res != ERROR_SUCCESS)
    {
        TRACE("RegOpenKey failed on Software\\Microsoft\\Windows\\CurrentVersion (%ld)\n", res);

        goto end;
    }

    res = RegOpenKeyExW(hkWin,
                        szKeyName,
                        0,
                        bDelete ? KEY_ALL_ACCESS : KEY_READ,
                        &hkRun);
    if (res != ERROR_SUCCESS)
    {
        if (res == ERROR_FILE_NOT_FOUND)
        {
            TRACE("Key doesn't exist - nothing to be done\n");

            res = ERROR_SUCCESS;
        }
        else
            TRACE("RegOpenKeyEx failed on run key (%ld)\n", res);

        goto end;
    }

    res = RegQueryInfoKeyW(hkRun,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           &i,
                           &cchMaxValue,
                           &cbMaxCmdLine,
                           NULL,
                           NULL);
    if (res != ERROR_SUCCESS)
    {
        TRACE("Couldn't query key info (%ld)\n", res);

        goto end;
    }

    if (i == 0)
    {
        TRACE("No commands to execute.\n");

        res = ERROR_SUCCESS;
        goto end;
    }

    szCmdLine = (WCHAR*)HeapAlloc(hProcessHeap,
                          0,
                          cbMaxCmdLine);
    if (szCmdLine == NULL)
    {
        TRACE("Couldn't allocate memory for the commands to be executed\n");

        res = ERROR_NOT_ENOUGH_MEMORY;
        goto end;
    }

    ++cchMaxValue;
    szValue = (WCHAR*)HeapAlloc(hProcessHeap,
                        0,
                        cchMaxValue * sizeof(*szValue));
    if (szValue == NULL)
    {
        TRACE("Couldn't allocate memory for the value names\n");

        res = ERROR_NOT_ENOUGH_MEMORY;
        goto end;
    }

    while (i > 0)
    {
        DWORD cchValLength = cchMaxValue, cbDataLength = cbMaxCmdLine;
        DWORD type;

        --i;

        res = RegEnumValueW(hkRun,
                            i,
                            szValue,
                            &cchValLength,
                            0,
                            &type,
                            (PBYTE)szCmdLine,
                            &cbDataLength);
        if (res != ERROR_SUCCESS)
        {
            TRACE("Couldn't read in value %lu - %ld\n", i, res);

            continue;
        }

        /* safe mode - force to run if prefixed with asterisk */
        if (GetSystemMetrics(SM_CLEANBOOT) && (szValue[0] != L'*')) continue;

        if (bDelete && (res = RegDeleteValueW(hkRun, szValue)) != ERROR_SUCCESS)
        {
            TRACE("Couldn't delete value - %lu, %ld. Running command anyways.\n", i, res);
        }

        if (type != REG_SZ)
        {
            TRACE("Incorrect type of value #%lu (%lu)\n", i, type);

            continue;
        }

        res = runCmd(szCmdLine, NULL, bSynchronous, FALSE);
        if (res == INVALID_RUNCMD_RETURN)
        {
            TRACE("Error running cmd #%lu (%lu)\n", i, GetLastError());
        }

        TRACE("Done processing cmd #%lu\n", i);
    }

    res = ERROR_SUCCESS;
end:
    if (szValue != NULL)
        HeapFree(hProcessHeap, 0, szValue);
    if (szCmdLine != NULL)
        HeapFree(hProcessHeap, 0, szCmdLine);
    if (hkRun != NULL)
        RegCloseKey(hkRun);
    if (hkWin != NULL)
        RegCloseKey(hkWin);

    TRACE("done\n");

    return res == ERROR_SUCCESS ? TRUE : FALSE;
}


int
ProcessStartupItems(VOID)
{
    /* TODO: ProcessRunKeys already checks SM_CLEANBOOT -- items prefixed with * should probably run even in safe mode */
    BOOL bNormalBoot = GetSystemMetrics(SM_CLEANBOOT) == 0; /* Perform the operations that are performed every boot */
    /* First, set the current directory to SystemRoot */
    WCHAR gen_path[MAX_PATH];
    DWORD res;
    HKEY hSessionKey, hKey;
    HRESULT hr;

    res = GetWindowsDirectoryW(gen_path, sizeof(gen_path) / sizeof(gen_path[0]));
    if (res == 0)
    {
        TRACE("Couldn't get the windows directory - error %lu\n", GetLastError());

        return 100;
    }

    if (!SetCurrentDirectoryW(gen_path))
    {
        TRACE("Cannot set the dir to %ls (%lu)\n", gen_path, GetLastError());

        return 100;
    }

    hr = SHCreateSessionKey(KEY_WRITE, &hSessionKey);
    if (SUCCEEDED(hr))
    {
        LONG Error;
        DWORD dwDisp;

        Error = RegCreateKeyExW(hSessionKey, L"StartupHasBeenRun", 0, NULL, REG_OPTION_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisp);
        RegCloseKey(hSessionKey);
        if (Error == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            if (dwDisp == REG_OPENED_EXISTING_KEY)
            {
                /* Startup programs has already been run */
                return 0;
            }
        }
    }

    /* Perform the operations by order checking if policy allows it, checking if this is not Safe Mode,
     * stopping if one fails, skipping if necessary.
    */
    res = TRUE;
    /* TODO: RunOnceEx */

    if (res && (SHRestricted(REST_NOLOCALMACHINERUNONCE) == 0))
        res = ProcessRunKeys(HKEY_LOCAL_MACHINE, L"RunOnce", TRUE, TRUE);

    if (res && bNormalBoot && (SHRestricted(REST_NOLOCALMACHINERUN) == 0))
        res = ProcessRunKeys(HKEY_LOCAL_MACHINE, L"Run", FALSE, FALSE);

    if (res && bNormalBoot && (SHRestricted(REST_NOCURRENTUSERRUNONCE) == 0))
        res = ProcessRunKeys(HKEY_CURRENT_USER, L"Run", FALSE, FALSE);

    /* TODO: All users Startup folder */

    /* TODO: Current user Startup folder */

    /* TODO: HKCU\RunOnce runs even if StartupHasBeenRun exists */
    if (res && bNormalBoot && (SHRestricted(REST_NOCURRENTUSERRUNONCE) == 0))
        res = ProcessRunKeys(HKEY_CURRENT_USER, L"RunOnce", TRUE, FALSE);

    TRACE("Operation done\n");

    return res ? 0 : 101;
}
