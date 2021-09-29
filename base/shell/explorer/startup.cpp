/*
 * Copyright (C) 2002 Andreas Mohr
 * Copyright (C) 2002 Shachar Shemesh
 * Copyright (C) 2013 Edijs Kolesnikovics
 * Copyright (C) 2018 Katayama Hirofumi MZ
 * Copyright (C) 2021 He Yang
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
 * - HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\RunOnceEx (synch)
 * - HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\RunOnce (synch)
 * - HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Run (asynch)
 * - HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run (asynch)
 * - All users Startup folder "%ALLUSERSPROFILE%\Start Menu\Programs\Startup" (asynch, no imp)
 * - Current user Startup folder "%USERPROFILE%\Start Menu\Programs\Startup" (asynch, no imp)
 * - HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\RunOnce (asynch)
 *
 * None is processed in Safe Mode
 */

#include "precomp.h"

// For the auto startup process
static HANDLE s_hStartupMutex = NULL;

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

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    if (minimized)
    {
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_MINIMIZE;
    }
    memset(&info, 0, sizeof(info));

    if (!CreateProcessW(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, dir, &si, &info))
    {
        TRACE("Failed to run command (%lu)\n", GetLastError());

        return INVALID_RUNCMD_RETURN;
    }

    TRACE("Successfully ran command\n");

    if (wait)
    {
        HANDLE Handles[] = { info.hProcess };
        DWORD nCount = _countof(Handles);
        DWORD dwWait;
        MSG msg;

        /* wait for the process to exit */
        for (;;)
        {
            /* We need to keep processing messages,
               otherwise we will hang anything that is trying to send a message to us */
            dwWait = MsgWaitForMultipleObjects(nCount, Handles, FALSE, INFINITE, QS_ALLINPUT);

            /* WAIT_OBJECT_0 + nCount signals an event in the message queue,
               so anything other than that means we are done. */
            if (dwWait != WAIT_OBJECT_0 + nCount)
            {
                if (dwWait >= WAIT_OBJECT_0 && dwWait < WAIT_OBJECT_0 + nCount)
                    TRACE("Event %u signaled\n", dwWait - WAIT_OBJECT_0);
                else
                    WARN("Return code: %u\n", dwWait);
                break;
            }

            while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }

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
        TRACE("RegOpenKeyW failed on Software\\Microsoft\\Windows\\CurrentVersion (%ld)\n", res);

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
            TRACE("RegOpenKeyExW failed on run key (%ld)\n", res);

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
        WCHAR *szCmdLineExp = NULL;
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

        if (type != REG_SZ && type != REG_EXPAND_SZ)
        {
            TRACE("Incorrect type of value #%lu (%lu)\n", i, type);

            continue;
        }

        if (type == REG_EXPAND_SZ)
        {
            DWORD dwNumOfChars;

            dwNumOfChars = ExpandEnvironmentStringsW(szCmdLine, NULL, 0);
            if (dwNumOfChars)
            {
                szCmdLineExp = (WCHAR *)HeapAlloc(hProcessHeap, 0, dwNumOfChars * sizeof(*szCmdLineExp));

                if (szCmdLineExp == NULL)
                {
                    TRACE("Couldn't allocate memory for the commands to be executed\n");

                    res = ERROR_NOT_ENOUGH_MEMORY;
                    goto end;
                }

                ExpandEnvironmentStringsW(szCmdLine, szCmdLineExp, dwNumOfChars);
            }
        }

        res = runCmd(szCmdLineExp ? szCmdLineExp : szCmdLine, NULL, bSynchronous, FALSE);
        if (res == INVALID_RUNCMD_RETURN)
        {
            TRACE("Error running cmd #%lu (%lu)\n", i, GetLastError());
        }

        if (szCmdLineExp != NULL)
        {
            HeapFree(hProcessHeap, 0, szCmdLineExp);
            szCmdLineExp = NULL;
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

/**
 * Process "RunOnceEx" type registry key.
 * rundll32.exe will be invoked if the corresponding key has items inside, and wait for it.
 * hkRoot is the HKEY from which
 *      "Software\Microsoft\Windows\CurrentVersion\RunOnceEx"
 *      is opened.
 */
static BOOL ProcessRunOnceEx(HKEY hkRoot)
{
    HKEY hkRunOnceEx = NULL;
    LONG res = ERROR_SUCCESS;
    WCHAR cmdLine[] = L"rundll32 iernonce.dll RunOnceExProcess";
    DWORD dwSubKeyCnt;

    res = RegOpenKeyExW(hkRoot,
                        L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx",
                        0,
                        KEY_READ,
                        &hkRunOnceEx);
    if (res != ERROR_SUCCESS)
    {
        TRACE("RegOpenKeyW failed on Software\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx (%ld)\n", res);
        goto end;
    }

    res = RegQueryInfoKeyW(hkRunOnceEx,
                           NULL,
                           NULL,
                           NULL,
                           &dwSubKeyCnt,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL);

    if (res != ERROR_SUCCESS)
    {
        TRACE("RegQueryInfoKeyW failed on Software\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx (%ld)\n", res);
        goto end;
    }

    if (dwSubKeyCnt != 0)
    {
        if (runCmd(cmdLine, NULL, TRUE, TRUE) == INVALID_RUNCMD_RETURN)
        {
            TRACE("runCmd failed (%ld)\n", res = GetLastError());
            goto end;
        }
    }

end:
    if (hkRunOnceEx != NULL)
        RegCloseKey(hkRunOnceEx);

    TRACE("done\n");

    return res == ERROR_SUCCESS ? TRUE : FALSE;
}

static BOOL
AutoStartupApplications(INT nCSIDL_Folder)
{
    WCHAR szPath[MAX_PATH] = { 0 };
    HRESULT hResult;
    HANDLE hFind;
    WIN32_FIND_DATAW FoundData;
    size_t cchPathLen;

    TRACE("(%d)\n", nCSIDL_Folder);

    // Get the special folder path
    hResult = SHGetFolderPathW(NULL, nCSIDL_Folder, NULL, SHGFP_TYPE_CURRENT, szPath);
    cchPathLen = wcslen(szPath);
    if (!SUCCEEDED(hResult) || cchPathLen == 0)
    {
        WARN("SHGetFolderPath() failed with error %lu\n", GetLastError());
        return FALSE;
    }

    // Build a path with wildcard
    StringCbCatW(szPath, sizeof(szPath), L"\\*");

    // Start enumeration of files
    hFind = FindFirstFileW(szPath, &FoundData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        WARN("FindFirstFile(%s) failed with error %lu\n", debugstr_w(szPath), GetLastError());
        return FALSE;
    }

    // Enumerate the files
    do
    {
        // Ignore "." and ".."
        if (wcscmp(FoundData.cFileName, L".") == 0 ||
            wcscmp(FoundData.cFileName, L"..") == 0)
        {
            continue;
        }

        // Don't run hidden files
        if (FoundData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
            continue;

        // Build the path
        szPath[cchPathLen + 1] = UNICODE_NULL;
        StringCbCatW(szPath, sizeof(szPath), FoundData.cFileName);

        TRACE("Executing %s in directory %s\n", debugstr_w(FoundData.cFileName), debugstr_w(szPath));

        DWORD dwType;
        if (GetBinaryTypeW(szPath, &dwType))
        {
            runCmd(szPath, NULL, TRUE, FALSE);
        }
        else
        {
            SHELLEXECUTEINFOW ExecInfo;
            ZeroMemory(&ExecInfo, sizeof(ExecInfo));
            ExecInfo.cbSize = sizeof(ExecInfo);
            ExecInfo.lpFile = szPath;
            ShellExecuteExW(&ExecInfo);
        }
    } while (FindNextFileW(hFind, &FoundData));

    FindClose(hFind);
    return TRUE;
}

INT ProcessStartupItems(VOID)
{
    /* TODO: ProcessRunKeys already checks SM_CLEANBOOT -- items prefixed with * should probably run even in safe mode */
    BOOL bNormalBoot = GetSystemMetrics(SM_CLEANBOOT) == 0; /* Perform the operations that are performed every boot */
    /* First, set the current directory to SystemRoot */
    WCHAR gen_path[MAX_PATH];
    DWORD res;

    res = GetWindowsDirectoryW(gen_path, _countof(gen_path));
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

    /* Perform the operations by order checking if policy allows it, checking if this is not Safe Mode,
     * stopping if one fails, skipping if necessary.
     */
    res = TRUE;

    if (res && bNormalBoot)
        ProcessRunOnceEx(HKEY_LOCAL_MACHINE);

    if (res && (SHRestricted(REST_NOLOCALMACHINERUNONCE) == 0))
        res = ProcessRunKeys(HKEY_LOCAL_MACHINE, L"RunOnce", TRUE, TRUE);

    if (res && bNormalBoot && (SHRestricted(REST_NOLOCALMACHINERUN) == 0))
        res = ProcessRunKeys(HKEY_LOCAL_MACHINE, L"Run", FALSE, FALSE);

    if (res && bNormalBoot && (SHRestricted(REST_NOCURRENTUSERRUNONCE) == 0))
        res = ProcessRunKeys(HKEY_CURRENT_USER, L"Run", FALSE, FALSE);

    /* All users Startup folder */
    AutoStartupApplications(CSIDL_COMMON_STARTUP);

    /* Current user Startup folder */
    AutoStartupApplications(CSIDL_STARTUP);

    /* TODO: HKCU\RunOnce runs even if StartupHasBeenRun exists */
    if (res && bNormalBoot && (SHRestricted(REST_NOCURRENTUSERRUNONCE) == 0))
        res = ProcessRunKeys(HKEY_CURRENT_USER, L"RunOnce", TRUE, FALSE);

    TRACE("Operation done\n");

    return res ? 0 : 101;
}

BOOL DoFinishStartupItems(VOID)
{
    if (s_hStartupMutex)
    {
        ReleaseMutex(s_hStartupMutex);
        CloseHandle(s_hStartupMutex);
        s_hStartupMutex = NULL;
    }
    return TRUE;
}

BOOL DoStartStartupItems(ITrayWindow *Tray)
{
    DWORD dwWait;

    if (!bExplorerIsShell)
        return FALSE;

    if (!s_hStartupMutex)
    {
        // Accidentally, there is possibility that the system starts multiple Explorers
        // before startup of shell. We use a mutex to match the timing of shell initialization.
        s_hStartupMutex = CreateMutexW(NULL, FALSE, L"ExplorerIsShellMutex");
        if (s_hStartupMutex == NULL)
            return FALSE;
    }

    dwWait = WaitForSingleObject(s_hStartupMutex, INFINITE);
    TRACE("dwWait: 0x%08lX\n", dwWait);
    if (dwWait != WAIT_OBJECT_0)
    {
        TRACE("LastError: %ld\n", GetLastError());

        DoFinishStartupItems();
        return FALSE;
    }

    const DWORD dwWaitTotal = 3000;     // in milliseconds
    DWORD dwTick = GetTickCount();
    while (GetShellWindow() == NULL && GetTickCount() - dwTick < dwWaitTotal)
    {
        TrayProcessMessages(Tray);
    }

    if (GetShellWindow() == NULL)
    {
        DoFinishStartupItems();
        return FALSE;
    }

    // Check the volatile "StartupHasBeenRun" key
    HKEY hSessionKey, hKey;
    HRESULT hr = SHCreateSessionKey(KEY_WRITE, &hSessionKey);
    if (SUCCEEDED(hr))
    {
        ASSERT(hSessionKey);

        DWORD dwDisp;
        LONG Error = RegCreateKeyExW(hSessionKey, L"StartupHasBeenRun", 0, NULL,
                                     REG_OPTION_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisp);
        RegCloseKey(hSessionKey);
        RegCloseKey(hKey);
        if (Error == ERROR_SUCCESS && dwDisp == REG_OPENED_EXISTING_KEY)
        {
            return FALSE;   // Startup programs has already been run
        }
    }

    return TRUE;
}
