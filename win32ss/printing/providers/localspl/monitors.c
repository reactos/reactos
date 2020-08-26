/*
 * PROJECT:     ReactOS Local Spooler
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Print Monitors
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

// Global Variables
LIST_ENTRY PrintMonitorList;

// Local Constants
static DWORD dwMonitorInfo1Offsets[] = {
    FIELD_OFFSET(MONITOR_INFO_1W, pName),
    MAXDWORD
};

static DWORD dwMonitorInfo2Offsets[] = {
    FIELD_OFFSET(MONITOR_INFO_2W, pName),
    FIELD_OFFSET(MONITOR_INFO_2W, pEnvironment),
    FIELD_OFFSET(MONITOR_INFO_2W, pDLLName),
    MAXDWORD
};


PLOCAL_PRINT_MONITOR
FindPrintMonitor(PCWSTR pwszName)
{
    PLIST_ENTRY pEntry;
    PLOCAL_PRINT_MONITOR pPrintMonitor;

    TRACE("FindPrintMonitor(%S)\n", pwszName);

    if (!pwszName)
        return NULL;

    for (pEntry = PrintMonitorList.Flink; pEntry != &PrintMonitorList; pEntry = pEntry->Flink)
    {
        pPrintMonitor = CONTAINING_RECORD(pEntry, LOCAL_PRINT_MONITOR, Entry);

        if (_wcsicmp(pPrintMonitor->pwszName, pwszName) == 0)
            return pPrintMonitor;
    }

    return NULL;
}

static LONG WINAPI CreateKey(HANDLE hcKey, LPCWSTR pszSubKey, DWORD dwOptions, REGSAM samDesired, PSECURITY_ATTRIBUTES pSecurityAttributes, PHANDLE phckResult, PDWORD pdwDisposition, HANDLE hSpooler)
{
    FIXME("stub\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI OpenKey(HANDLE hcKey, LPCWSTR pszSubKey, REGSAM samDesired, PHANDLE phkResult, HANDLE hSpooler)
{
    FIXME("stub\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI CloseKey(HANDLE hcKey, HANDLE hSpooler)
{
    FIXME("stub\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI DeleteKey(HANDLE hcKey, LPCWSTR pszSubKey, HANDLE hSpooler)
{
    FIXME("stub\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI EnumKey(HANDLE hcKey, DWORD dwIndex, LPWSTR pszName, PDWORD pcchName, PFILETIME pftLastWriteTime, HANDLE hSpooler)
{
    FIXME("stub\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI QueryInfoKey(HANDLE hcKey, PDWORD pcSubKeys, PDWORD pcbKey, PDWORD pcValues, PDWORD pcbValue, PDWORD pcbData, PDWORD pcbSecurityDescriptor, PFILETIME pftLastWriteTime,
                HANDLE hSpooler)
{
    FIXME("stub\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI SetValue(HANDLE hcKey, LPCWSTR pszValue, DWORD dwType, const BYTE* pData, DWORD cbData, HANDLE hSpooler)
{
    FIXME("stub\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI DeleteValue(HANDLE hcKey, LPCWSTR pszValue, HANDLE hSpooler)
{
    FIXME("stub\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI EnumValue(HANDLE hcKey, DWORD dwIndex, LPWSTR pszValue, PDWORD pcbValue, PDWORD pType, PBYTE pData, PDWORD pcbData, HANDLE hSpooler)
{
    FIXME("stub\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI QueryValue(HANDLE hcKey, LPCWSTR pszValue, PDWORD pType, PBYTE pData, PDWORD pcbData, HANDLE hSpooler)
{
    FIXME("stub\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static MONITORREG MonReg =
{
    sizeof(MONITORREG),
    CreateKey,
    OpenKey,
    CloseKey,
    DeleteKey,
    EnumKey,
    QueryInfoKey,
    SetValue,
    DeleteValue,
    EnumValue,
    QueryValue
};

BOOL
InitializePrintMonitorList(void)
{
    const WCHAR wszMonitorsPath[] = L"SYSTEM\\CurrentControlSet\\Control\\Print\\Monitors";
    const DWORD cchMonitorsPath = _countof(wszMonitorsPath) - 1;

    DWORD cchMaxSubKey;
    DWORD cchPrintMonitorName;
    DWORD dwErrorCode;
    DWORD dwSubKeys;
    DWORD i;
    HINSTANCE hinstPrintMonitor = NULL;
    HKEY hKey = NULL;
    HKEY hSubKey = NULL;
    MONITORINIT MonitorInit;
    PInitializePrintMonitor pfnInitializePrintMonitor;
    PInitializePrintMonitor2 pfnInitializePrintMonitor2;
    PLOCAL_PRINT_MONITOR pPrintMonitor = NULL;
    PWSTR pwszRegistryPath = NULL;

    TRACE("InitializePrintMonitorList()\n");

    // Initialize an empty list for our Print Monitors.
    InitializeListHead(&PrintMonitorList);

    // Open the key containing Print Monitors.
    dwErrorCode = (DWORD)RegOpenKeyExW(HKEY_LOCAL_MACHINE, wszMonitorsPath, 0, KEY_READ, &hKey);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Get the number of Print Providers and maximum sub key length.
    dwErrorCode = (DWORD)RegQueryInfoKeyW(hKey, NULL, NULL, NULL, &dwSubKeys, &cchMaxSubKey, NULL, NULL, NULL, NULL, NULL, NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegQueryInfoKeyW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Loop through all available Print Providers.
    for (i = 0; i < dwSubKeys; i++)
    {
        // Cleanup tasks from the previous run
        if (hSubKey)
        {
            RegCloseKey(hSubKey);
            hSubKey = NULL;
        }

        if (pwszRegistryPath)
        {
            DllFreeSplMem(pwszRegistryPath);
            pwszRegistryPath = NULL;
        }

        if (pPrintMonitor)
        {
            if (pPrintMonitor->pwszFileName)
                DllFreeSplMem(pPrintMonitor->pwszFileName);

            if (pPrintMonitor->pwszName)
                DllFreeSplMem(pPrintMonitor->pwszName);

            DllFreeSplMem(pPrintMonitor);
            pPrintMonitor = NULL;
        }

        // Create a new LOCAL_PRINT_MONITOR structure for it.
        pPrintMonitor = DllAllocSplMem(sizeof(LOCAL_PRINT_MONITOR));
        if (!pPrintMonitor)
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("DllAllocSplMem failed!\n");
            goto Cleanup;
        }

        memset( pPrintMonitor, 0, sizeof(LOCAL_PRINT_MONITOR));

        // Allocate memory for the Print Monitor Name.
        pPrintMonitor->pwszName = DllAllocSplMem((cchMaxSubKey + 1) * sizeof(WCHAR));
        if (!pPrintMonitor->pwszName)
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("DllAllocSplMem failed!\n");
            goto Cleanup;
        }

        // Get the name of this Print Monitor.
        cchPrintMonitorName = cchMaxSubKey + 1;
        dwErrorCode = (DWORD)RegEnumKeyExW(hKey, i, pPrintMonitor->pwszName, &cchPrintMonitorName, NULL, NULL, NULL, NULL);
        if (dwErrorCode != ERROR_SUCCESS)
        {
            ERR("RegEnumKeyExW failed for iteration %lu with status %lu!\n", i, dwErrorCode);
            continue;
        }

        // Open this Print Monitor's registry key.
        dwErrorCode = (DWORD)RegOpenKeyExW(hKey, pPrintMonitor->pwszName, 0, KEY_READ, &hSubKey);
        if (dwErrorCode != ERROR_SUCCESS)
        {
            ERR("RegOpenKeyExW failed for Print Provider \"%S\" with status %lu!\n", pPrintMonitor->pwszName, dwErrorCode);
            continue;
        }

        // Get the file name of the Print Monitor.
        pPrintMonitor->pwszFileName = AllocAndRegQueryWSZ(hSubKey, L"Driver");
        if (!pPrintMonitor->pwszFileName)
            continue;

        // Try to load it.
        hinstPrintMonitor = LoadLibraryW(pPrintMonitor->pwszFileName);
        if (!hinstPrintMonitor)
        {
            ERR("LoadLibraryW failed for \"%S\" with error %lu!\n", pPrintMonitor->pwszFileName, GetLastError());
            continue;
        }

        pPrintMonitor->hModule = hinstPrintMonitor;

        // Try to find a Level 2 initialization routine first.
        pfnInitializePrintMonitor2 = (PInitializePrintMonitor2)GetProcAddress(hinstPrintMonitor, "InitializePrintMonitor2");
        if (pfnInitializePrintMonitor2)
        {
            // Prepare a MONITORINIT structure.
            MonitorInit.cbSize = sizeof(MONITORINIT);
            MonitorInit.bLocal = TRUE;

            // TODO: Fill the other fields.
            MonitorInit.hckRegistryRoot = hKey;
            MonitorInit.pMonitorReg = &MonReg;

            // Call the Level 2 initialization routine.
            pPrintMonitor->pMonitor = (PMONITOR2)pfnInitializePrintMonitor2(&MonitorInit, &pPrintMonitor->hMonitor);
            if (!pPrintMonitor->pMonitor)
            {
                ERR("InitializePrintMonitor2 failed for \"%S\" with error %lu!\n", pPrintMonitor->pwszFileName, GetLastError());
                continue;
            }
            FIXME("InitializePrintMonitor2 loaded.\n");
            pPrintMonitor->bIsLevel2 = TRUE;
        }
        else
        {
            // Try to find a Level 1 initialization routine then.
            pfnInitializePrintMonitor = (PInitializePrintMonitor)GetProcAddress(hinstPrintMonitor, "InitializePrintMonitor");
            if (pfnInitializePrintMonitor)
            {
                // Construct the registry path.
                pwszRegistryPath = DllAllocSplMem((cchMonitorsPath + 1 + cchPrintMonitorName + 1) * sizeof(WCHAR));
                if (!pwszRegistryPath)
                {
                    dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                    ERR("DllAllocSplMem failed!\n");
                    goto Cleanup;
                }

                CopyMemory(pwszRegistryPath, wszMonitorsPath, cchMonitorsPath * sizeof(WCHAR));
                pwszRegistryPath[cchMonitorsPath] = L'\\';
                CopyMemory(&pwszRegistryPath[cchMonitorsPath + 1], pPrintMonitor->pwszName, (cchPrintMonitorName + 1) * sizeof(WCHAR));

                // Call the Level 1 initialization routine.
                pPrintMonitor->pMonitor = (LPMONITOREX)pfnInitializePrintMonitor(pwszRegistryPath);
                if (!pPrintMonitor->pMonitor)
                {
                    ERR("InitializePrintMonitor failed for \"%S\" with error %lu!\n", pPrintMonitor->pwszFileName, GetLastError());
                    continue;
                }
            }
            else
            {
                ERR("No initialization routine found for \"%S\"!\n", pPrintMonitor->pwszFileName);
                continue;
            }
        }

        // Add this Print Monitor to the list.
        InsertTailList(&PrintMonitorList, &pPrintMonitor->Entry);
        FIXME("InitializePrintMonitorList Handle %p\n",pPrintMonitor->hMonitor);
        pPrintMonitor->refcount++;

        // Don't let the cleanup routine free this.
        pPrintMonitor = NULL;
    }

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    // Inside the loop
    if (hSubKey)
        RegCloseKey(hSubKey);

    if (pwszRegistryPath)
        DllFreeSplMem(pwszRegistryPath);

    if (pPrintMonitor)
    {
        if (pPrintMonitor->pwszFileName)
            DllFreeSplMem(pPrintMonitor->pwszFileName);

        if (pPrintMonitor->pwszName)
            DllFreeSplMem(pPrintMonitor->pwszName);

        DllFreeSplMem(pPrintMonitor);
    }

    // Outside the loop
    if (hKey)
        RegCloseKey(hKey);

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}


static void
_LocalGetMonitorLevel1(PLOCAL_PRINT_MONITOR pPrintMonitor, PMONITOR_INFO_1W* ppMonitorInfo, PBYTE* ppMonitorInfoEnd, PDWORD pcbNeeded)
{
    DWORD cbMonitorName;
    PCWSTR pwszStrings[1];

    // Calculate the string lengths.
    if (!ppMonitorInfo)
    {
        cbMonitorName = (wcslen(pPrintMonitor->pwszName) + 1) * sizeof(WCHAR);

        *pcbNeeded += sizeof(MONITOR_INFO_1W) + cbMonitorName;
        return;
    }

    // Set the pName field.
    pwszStrings[0] = pPrintMonitor->pwszName;

    // Copy the structure and advance to the next one in the output buffer.
    *ppMonitorInfoEnd = PackStrings(pwszStrings, (PBYTE)(*ppMonitorInfo), dwMonitorInfo1Offsets, *ppMonitorInfoEnd);
    (*ppMonitorInfo)++;
}

static void
_LocalGetMonitorLevel2(PLOCAL_PRINT_MONITOR pPrintMonitor, PMONITOR_INFO_2W* ppMonitorInfo, PBYTE* ppMonitorInfoEnd, PDWORD pcbNeeded)
{
    DWORD cbFileName;
    DWORD cbMonitorName;
    PCWSTR pwszStrings[3];

    // Calculate the string lengths.
    if (!ppMonitorInfo)
    {
        cbMonitorName = (wcslen(pPrintMonitor->pwszName) + 1) * sizeof(WCHAR);
        cbFileName = (wcslen(pPrintMonitor->pwszFileName) + 1) * sizeof(WCHAR);

        *pcbNeeded += sizeof(MONITOR_INFO_2W) + cbMonitorName + cbCurrentEnvironment + cbFileName;
        return;
    }

    // Set the pName field.
    pwszStrings[0] = pPrintMonitor->pwszName;

    // Set the pEnvironment field.
    pwszStrings[1] = (PWSTR)wszCurrentEnvironment;

    // Set the pDLLName field.
    pwszStrings[2] = pPrintMonitor->pwszFileName;

    // Copy the structure and advance to the next one in the output buffer.
    *ppMonitorInfoEnd = PackStrings(pwszStrings, (PBYTE)(*ppMonitorInfo), dwMonitorInfo2Offsets, *ppMonitorInfoEnd);
    (*ppMonitorInfo)++;
}

BOOL WINAPI
LocalEnumMonitors(PWSTR pName, DWORD Level, PBYTE pMonitors, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD dwErrorCode;
    PBYTE pMonitorInfoEnd;
    PLIST_ENTRY pEntry;
    PLOCAL_PRINT_MONITOR pPrintMonitor;

    TRACE("LocalEnumMonitors(%S, %lu, %p, %lu, %p, %p)\n", pName, Level, pMonitors, cbBuf, pcbNeeded, pcReturned);

    // Sanity checks.
    if (Level > 2)
    {
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    // Begin counting.
    *pcbNeeded = 0;
    *pcReturned = 0;

    // Count the required buffer size and the number of monitors.
    for (pEntry = PrintMonitorList.Flink; pEntry != &PrintMonitorList; pEntry = pEntry->Flink)
    {
        pPrintMonitor = CONTAINING_RECORD(pEntry, LOCAL_PRINT_MONITOR, Entry);

        if (Level == 1)
            _LocalGetMonitorLevel1(pPrintMonitor, NULL, NULL, pcbNeeded);
        else if (Level == 2)
            _LocalGetMonitorLevel2(pPrintMonitor, NULL, NULL, pcbNeeded);
    }

    // Check if the supplied buffer is large enough.
    if (cbBuf < *pcbNeeded)
    {
        dwErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    // Copy over the Monitor information.
    pMonitorInfoEnd = &pMonitors[*pcbNeeded];

    for (pEntry = PrintMonitorList.Flink; pEntry != &PrintMonitorList; pEntry = pEntry->Flink)
    {
        pPrintMonitor = CONTAINING_RECORD(pEntry, LOCAL_PRINT_MONITOR, Entry);

        if (Level == 1)
            _LocalGetMonitorLevel1(pPrintMonitor, (PMONITOR_INFO_1W*)&pMonitors, &pMonitorInfoEnd, NULL);
        else if (Level == 2)
            _LocalGetMonitorLevel2(pPrintMonitor, (PMONITOR_INFO_2W*)&pMonitors, &pMonitorInfoEnd, NULL);

        (*pcReturned)++;
    }

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL
AddPrintMonitorList( LPCWSTR pName, LPWSTR DllName )
{
    const WCHAR wszMonitorsPath[] = L"SYSTEM\\CurrentControlSet\\Control\\Print\\Monitors\\";
    const DWORD cchMonitorsPath = _countof(wszMonitorsPath) - 1;

    WCHAR wszRegRoot[MAX_PATH] = {0};

    DWORD cchPrintMonitorName;
    DWORD dwErrorCode;
    HINSTANCE hinstPrintMonitor = NULL;
    HKEY hKey = NULL;
    MONITORINIT MonitorInit;
    PInitializePrintMonitor pfnInitializePrintMonitor;
    PInitializePrintMonitor2 pfnInitializePrintMonitor2;
    PLOCAL_PRINT_MONITOR pPrintMonitor = NULL;
    PWSTR pwszRegistryPath = NULL;

    FIXME("AddPrintMonitorList( %S, %S)\n",pName, DllName);

    StringCbCopyW(wszRegRoot, sizeof(wszRegRoot), wszMonitorsPath);
    StringCbCatW(wszRegRoot, sizeof(wszRegRoot), pName);

    // Open the key containing Print Monitors.
    dwErrorCode = (DWORD)RegOpenKeyW( HKEY_LOCAL_MACHINE, wszRegRoot, &hKey );
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW %S failed with status %lu!\n", wszRegRoot, dwErrorCode);
        goto Cleanup;
    }

    // Create a new LOCAL_PRINT_MONITOR structure for it.
    pPrintMonitor = DllAllocSplMem(sizeof(LOCAL_PRINT_MONITOR));
    if (!pPrintMonitor)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("DllAllocSplMem failed!\n");
        goto Cleanup;
    }

    memset( pPrintMonitor, 0, sizeof(LOCAL_PRINT_MONITOR));

    // Allocate memory for the Print Monitor Name.
    pPrintMonitor->pwszName = AllocSplStr( pName );
    if (!pPrintMonitor->pwszName)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("DllAllocSplMem failed!\n");
        goto Cleanup;
    }

    cchPrintMonitorName = wcslen(pPrintMonitor->pwszName);

    if ( DllName == NULL )
    {
        DWORD namesize;

        dwErrorCode = RegQueryValueExW( hKey, L"Driver", NULL, NULL, NULL, &namesize );

        if ( dwErrorCode == ERROR_SUCCESS )
        {
            DllName = DllAllocSplMem(namesize);

            RegQueryValueExW( hKey, L"Driver", NULL, NULL, (LPBYTE)DllName, &namesize );

            pPrintMonitor->pwszFileName = DllName;
        }
        else
        {
            ERR("DllName not found\n");
            goto Cleanup;
        }
    }
    else
    {
        pPrintMonitor->pwszFileName = AllocSplStr( DllName );
    }

    // Try to load it.
    hinstPrintMonitor = LoadLibraryW(pPrintMonitor->pwszFileName);
    if (!hinstPrintMonitor)
    {
        ERR("LoadLibraryW failed for \"%S\" with error %lu!\n", pPrintMonitor->pwszFileName, GetLastError());
        dwErrorCode = GetLastError();
        goto Cleanup;
    }

    pPrintMonitor->hModule = hinstPrintMonitor;

    // Try to find a Level 2 initialization routine first.
    pfnInitializePrintMonitor2 = (PInitializePrintMonitor2)GetProcAddress(hinstPrintMonitor, "InitializePrintMonitor2");
    if (pfnInitializePrintMonitor2)
    {
        // Prepare a MONITORINIT structure.
        MonitorInit.cbSize = sizeof(MONITORINIT);
        MonitorInit.bLocal = TRUE;

        // TODO: Fill the other fields.
        MonitorInit.hckRegistryRoot = hKey;
        MonitorInit.pMonitorReg = &MonReg;

        // Call the Level 2 initialization routine.
        pPrintMonitor->pMonitor = (PMONITOR2)pfnInitializePrintMonitor2(&MonitorInit, &pPrintMonitor->hMonitor);
        if (!pPrintMonitor->pMonitor)
        {
            ERR("InitializePrintMonitor2 failed for \"%S\" with error %lu!\n", pPrintMonitor->pwszFileName, GetLastError());
            goto Cleanup;
        }

        pPrintMonitor->bIsLevel2 = TRUE;
    }
    else
    {
        // Try to find a Level 1 initialization routine then.
        pfnInitializePrintMonitor = (PInitializePrintMonitor)GetProcAddress(hinstPrintMonitor, "InitializePrintMonitor");
        if (pfnInitializePrintMonitor)
        {
            // Construct the registry path.
            pwszRegistryPath = DllAllocSplMem((cchMonitorsPath + 1 + cchPrintMonitorName + 1) * sizeof(WCHAR));
            if (!pwszRegistryPath)
            {
                dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                ERR("DllAllocSplMem failed!\n");
                goto Cleanup;
            }

            CopyMemory(pwszRegistryPath, wszMonitorsPath, cchMonitorsPath * sizeof(WCHAR));
            pwszRegistryPath[cchMonitorsPath] = L'\\';
            CopyMemory(&pwszRegistryPath[cchMonitorsPath + 1], pPrintMonitor->pwszName, (cchPrintMonitorName + 1) * sizeof(WCHAR));

            // Call the Level 1 initialization routine.
            pPrintMonitor->pMonitor = (LPMONITOREX)pfnInitializePrintMonitor(pwszRegistryPath);
            if (!pPrintMonitor->pMonitor)
            {
                ERR("InitializePrintMonitor failed for \"%S\" with error %lu!\n", pPrintMonitor->pwszFileName, GetLastError());
                goto Cleanup;
            }
        }
        else
        {
            ERR("No initialization routine found for \"%S\"!\n", pPrintMonitor->pwszFileName);
            dwErrorCode = ERROR_PROC_NOT_FOUND;
            goto Cleanup;
        }
    }
    // Add this Print Monitor to the list.
    InsertTailList(&PrintMonitorList, &pPrintMonitor->Entry);
    FIXME("AddPrintMonitorList Handle %p\n",pPrintMonitor->hMonitor);

    pPrintMonitor->refcount++;

    // Don't let the cleanup routine free this.
    pPrintMonitor = NULL;

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    if (pwszRegistryPath)
        DllFreeSplMem(pwszRegistryPath);

    if (pPrintMonitor)
    {
        if (pPrintMonitor->pwszFileName)
            DllFreeSplMem(pPrintMonitor->pwszFileName);

        if (pPrintMonitor->pwszName)
            DllFreeSplMem(pPrintMonitor->pwszName);

        DllFreeSplMem(pPrintMonitor);
    }

    // Outside the loop
    if (hKey)
        RegCloseKey(hKey);

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
LocalAddMonitor(PWSTR pName, DWORD Level, PBYTE pMonitors)
{
    PPRINTENV_T env;
    LPMONITOR_INFO_2W mi2w;
    HKEY hroot = NULL;
    HKEY hentry = NULL;
    DWORD disposition;
    BOOL res = FALSE;
    const WCHAR wszMonitorsPath[] = L"SYSTEM\\CurrentControlSet\\Control\\Print\\Monitors\\";

    mi2w = (LPMONITOR_INFO_2W) pMonitors;

    FIXME("LocalAddMonitor(%s, %d, %p): %s %s %s\n", debugstr_w(pName), Level, pMonitors,
        debugstr_w(mi2w->pName), debugstr_w(mi2w->pEnvironment), debugstr_w(mi2w->pDLLName));

    if (copy_servername_from_name(pName, NULL))
    {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    if (!mi2w->pName || (!mi2w->pName[0]) )
    {
        FIXME("pName not valid : %s\n", debugstr_w(mi2w->pName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    env = validate_envW(mi2w->pEnvironment);
    if (!env)
        return FALSE;   /* ERROR_INVALID_ENVIRONMENT */

    if (!mi2w->pDLLName || (!mi2w->pDLLName[0]) )
    {
        FIXME("pDLLName not valid : %s\n", debugstr_w(mi2w->pDLLName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (RegCreateKeyW(HKEY_LOCAL_MACHINE, wszMonitorsPath, &hroot) != ERROR_SUCCESS) {
        ERR("unable to create key %s\n", debugstr_w(wszMonitorsPath));
        return FALSE;
    }

    if (RegCreateKeyExW(hroot, mi2w->pName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_QUERY_VALUE, NULL, &hentry, &disposition) == ERROR_SUCCESS)
    {
        /* Some installers set options for the port before calling AddMonitor.
           We query the "Driver" entry to verify that the monitor is installed,
           before we return an error.
           When a user installs two print monitors at the same time with the
           same name, a race condition is possible but silently ignored. */

        DWORD   namesize = 0;

        if ((disposition == REG_OPENED_EXISTING_KEY) &&
            (RegQueryValueExW(hentry, L"Driver", NULL, NULL, NULL, &namesize) == ERROR_SUCCESS))
        {
            FIXME("monitor %s already exists\n", debugstr_w(mi2w->pName));
            /* 9x use ERROR_ALREADY_EXISTS */
            SetLastError(ERROR_PRINT_MONITOR_ALREADY_INSTALLED);
        }
        else
        {
            INT len = (lstrlenW(mi2w->pDLLName) +1) * sizeof(WCHAR);

            res = (RegSetValueExW(hentry, L"Driver", 0, REG_SZ, (LPBYTE) mi2w->pDLLName, len) == ERROR_SUCCESS);

            /* Load and initialize the monitor. SetLastError() is called on failure */

            res = AddPrintMonitorList( mi2w->pName, mi2w->pDLLName );

            if ( !res )
            {
                RegDeleteKeyW(hroot, mi2w->pName);
            }
            else
                SetLastError(ERROR_SUCCESS); /* Monitor installer depends on this */
        }

        RegCloseKey(hentry);
    }

    RegCloseKey(hroot);
    return res;
}

BOOL WINAPI
LocalDeleteMonitor(PWSTR pName, PWSTR pEnvironment, PWSTR pMonitorName)
{
    HKEY hroot = NULL;
    LONG lres;
    PLOCAL_PRINT_MONITOR pPrintMonitor;
    const WCHAR wszMonitorsPath[] = L"SYSTEM\\CurrentControlSet\\Control\\Print\\Monitors\\";

    FIXME("LocalDeleteMonitor(%s, %s, %s)\n",debugstr_w(pName),debugstr_w(pEnvironment),
           debugstr_w(pMonitorName));

    lres = copy_servername_from_name(pName, NULL);
    if (lres)
    {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    /*  pEnvironment is ignored in Windows for the local Computer */
    if (!pMonitorName || !pMonitorName[0])
    {
        ERR("pMonitorName %s is invalid\n", debugstr_w(pMonitorName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pPrintMonitor = FindPrintMonitor( pMonitorName );
    if ( pPrintMonitor )
    {
       if ( pPrintMonitor->refcount ) pPrintMonitor->refcount--;

       if ( pPrintMonitor->refcount == 0 )
       {       /* Unload the monitor if it's loaded */
       RemoveEntryList(&pPrintMonitor->Entry);

       if ( pPrintMonitor->bIsLevel2 )
       {
           PMONITOR2 pm2 = pPrintMonitor->pMonitor;
           if ( pm2 && pm2->pfnShutdown )
           {
               pm2->pfnShutdown(pPrintMonitor->hMonitor);
           }
       }

       if ( pPrintMonitor->hModule )
           FreeLibrary(pPrintMonitor->hModule);

       if (pPrintMonitor->pwszFileName)
           DllFreeSplStr(pPrintMonitor->pwszFileName);

       if (pPrintMonitor->pwszName)
           DllFreeSplStr(pPrintMonitor->pwszName);

       DllFreeSplMem(pPrintMonitor);
       pPrintMonitor = NULL;
       }
    }
    else
    {
       FIXME("Could not find %s\n", debugstr_w(pMonitorName));
    }

    if (RegCreateKeyW(HKEY_LOCAL_MACHINE, wszMonitorsPath, &hroot) != ERROR_SUCCESS)
    {
        ERR("unable to create key %s\n", debugstr_w(wszMonitorsPath));
        return FALSE;
    }

    if (RegDeleteTreeW(hroot, pMonitorName) == ERROR_SUCCESS)
    {
        FIXME("%s deleted\n", debugstr_w(pMonitorName));
        RegCloseKey(hroot);
        return TRUE;
    }

    FIXME("%s does not exist\n", debugstr_w(pMonitorName));
    RegCloseKey(hroot);

    /* NT: ERROR_UNKNOWN_PRINT_MONITOR (3000), 9x: ERROR_INVALID_PARAMETER (87) */
    SetLastError(ERROR_UNKNOWN_PRINT_MONITOR);
    return FALSE;
}
