/*
 * PROJECT:     ReactOS Local Spooler
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Print Monitors
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck <colin@reactos.org>
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

        // Try to find a Level 2 initialization routine first.
        pfnInitializePrintMonitor2 = (PInitializePrintMonitor2)GetProcAddress(hinstPrintMonitor, "InitializePrintMonitor2");
        if (pfnInitializePrintMonitor2)
        {
            // Prepare a MONITORINIT structure.
            MonitorInit.cbSize = sizeof(MONITORINIT);
            MonitorInit.bLocal = TRUE;

            // TODO: Fill the other fields.

            // Call the Level 2 initialization routine.
            pPrintMonitor->pMonitor = (PMONITOR2)pfnInitializePrintMonitor2(&MonitorInit, &pPrintMonitor->hMonitor);
            if (!pPrintMonitor->pMonitor)
            {
                ERR("InitializePrintMonitor2 failed for \"%S\" with error %lu!\n", pPrintMonitor->pwszFileName, GetLastError());
                continue;
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
    PWSTR pwszStrings[1];

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
    PWSTR pwszStrings[3];

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
