/*
 * PROJECT:     ReactOS Local Port Monitor
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Main functions
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

// Global Variables
DWORD cbLocalMonitor;
DWORD cbLocalPort;
PCWSTR pwszLocalMonitor;
PCWSTR pwszLocalPort;

// Local Constants
static MONITOR2 _MonitorFunctions = {
    sizeof(MONITOR2),               // cbSize
    LocalmonEnumPorts,              // pfnEnumPorts
    LocalmonOpenPort,               // pfnOpenPort
    NULL,                           // pfnOpenPortEx
    LocalmonStartDocPort,           // pfnStartDocPort
    LocalmonWritePort,              // pfnWritePort
    LocalmonReadPort,               // pfnReadPort
    LocalmonEndDocPort,             // pfnEndDocPort
    LocalmonClosePort,              // pfnClosePort
    NULL,                           // pfnAddPort
    NULL,                           // pfnAddPortEx
    NULL,                           // pfnConfigurePort
    NULL,                           // pfnDeletePort
    LocalmonGetPrinterDataFromPort, // pfnGetPrinterDataFromPort
    LocalmonSetPortTimeOuts,        // pfnSetPortTimeOuts
    LocalmonXcvOpenPort,            // pfnXcvOpenPort
    LocalmonXcvDataPort,            // pfnXcvDataPort
    LocalmonXcvClosePort,           // pfnXcvClosePort
    LocalmonShutdown,               // pfnShutdown
    NULL,                           // pfnSendRecvBidiDataFromPort
};


static void
_LoadResources(HINSTANCE hinstDLL)
{
    LoadStringW(hinstDLL, IDS_LOCAL_MONITOR, (PWSTR)&pwszLocalMonitor, 0);
    cbLocalMonitor = (wcslen(pwszLocalMonitor) + 1) * sizeof(WCHAR);

    LoadStringW(hinstDLL, IDS_LOCAL_PORT, (PWSTR)&pwszLocalPort, 0);
    cbLocalPort = (wcslen(pwszLocalPort) + 1) * sizeof(WCHAR);
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            _LoadResources(hinstDLL);
            break;
    }

    return TRUE;
}

void WINAPI
LocalmonShutdown(HANDLE hMonitor)
{
    PLOCALMON_HANDLE pLocalmon;
    PLOCALMON_PORT pPort;

    pLocalmon = (PLOCALMON_HANDLE)hMonitor;

    // Close all virtual file ports.
    while (!IsListEmpty(&pLocalmon->FilePorts))
    {
        pPort = CONTAINING_RECORD(&pLocalmon->FilePorts.Flink, LOCALMON_PORT, Entry);
        LocalmonClosePort((HANDLE)pPort);
    }

    // Now close all regular ports and remove them from the list.
    while (!IsListEmpty(&pLocalmon->Ports))
    {
        pPort = CONTAINING_RECORD(&pLocalmon->Ports.Flink, LOCALMON_PORT, Entry);
        RemoveEntryList(&pPort->Entry);
        LocalmonClosePort((HANDLE)pPort);
    }

    // Finally free the memory for the LOCALMON_HANDLE structure itself.
    DllFreeSplMem(pLocalmon);
}

PMONITOR2 WINAPI
InitializePrintMonitor2(PMONITORINIT pMonitorInit, PHANDLE phMonitor)
{
    DWORD cchMaxPortName;
    DWORD cchPortName;
    DWORD dwErrorCode;
    DWORD dwPortCount;
    DWORD i;
    HKEY hKey;
    PMONITOR2 pReturnValue = NULL;
    PLOCALMON_HANDLE pLocalmon;
    PLOCALMON_PORT pPort = NULL;

    // Create a new LOCALMON_HANDLE structure.
    pLocalmon = DllAllocSplMem(sizeof(LOCALMON_HANDLE));
    InitializeListHead(&pLocalmon->FilePorts);
    InitializeListHead(&pLocalmon->Ports);

    // The Local Spooler Port Monitor doesn't need to care about the given registry key and functions.
    // Instead it uses a well-known registry key for getting its information about local ports. Open this one.
    dwErrorCode = (DWORD)RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Ports", 0, KEY_READ, &hKey);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Get the number of ports and the length of the largest port name.
    dwErrorCode = (DWORD)RegQueryInfoKeyW(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &dwPortCount, &cchMaxPortName, NULL, NULL, NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegQueryInfoKeyW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Loop through all ports.
    for (i = 0; i < dwPortCount; i++)
    {
        // Allocate memory for a new LOCALMON_PORT structure and its name.
        pPort = DllAllocSplMem(sizeof(LOCALMON_PORT) + (cchMaxPortName + 1) * sizeof(WCHAR));
        if (!pPort)
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("DllAllocSplMem failed with error %lu!\n", GetLastError());
            goto Cleanup;
        }

        pPort->hFile = INVALID_HANDLE_VALUE;
        pPort->pwszPortName = (PWSTR)((PBYTE)pPort + sizeof(LOCALMON_PORT));

        // Get the port name.
        cchPortName = cchMaxPortName + 1;
        dwErrorCode = (DWORD)RegEnumValueW(hKey, i, pPort->pwszPortName, &cchPortName, NULL, NULL, NULL, NULL);
        if (dwErrorCode != ERROR_SUCCESS)
        {
            ERR("RegEnumValueW failed with status %lu!\n", dwErrorCode);
            goto Cleanup;
        }

        // This Port Monitor supports COM, FILE and LPT ports. Skip all others.
        if (_wcsnicmp(pPort->pwszPortName, L"COM", 3) != 0 && _wcsicmp(pPort->pwszPortName, L"FILE:") != 0 && _wcsnicmp(pPort->pwszPortName, L"LPT", 3) != 0)
        {
            DllFreeSplMem(pPort);
            continue;
        }

        // Add it to the list.
        InsertTailList(&pLocalmon->Ports, &pPort->Entry);

        // Don't let the cleanup routine free this.
        pPort = NULL;
    }

    // Return our handle and the Print Monitor functions.
    *phMonitor = (HANDLE)pLocalmon;
    pReturnValue = &_MonitorFunctions;
    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    if (pPort)
        DllFreeSplMem(pPort);

    SetLastError(dwErrorCode);
    return pReturnValue;
}
