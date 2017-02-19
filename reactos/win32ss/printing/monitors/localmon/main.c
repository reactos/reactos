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


/**
 * @name _IsNEPort
 *
 * Checks if the given port name is a virtual Ne port.
 * A virtual Ne port may appear in HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Ports and can have the formats
 * Ne00:, Ne01:, Ne-02:, Ne456:
 * This check is extra picky to not cause false positives (like file name ports starting with "Ne").
 *
 * @param pwszPortName
 * The port name to check.
 *
 * @return
 * TRUE if this is definitely a virtual Ne port, FALSE if not.
 */
static __inline BOOL
_IsNEPort(PCWSTR pwszPortName)
{
    PCWSTR p = pwszPortName;

    // First character needs to be 'N' (uppercase or lowercase)
    if (*p != L'N' && *p != L'n')
        return FALSE;

    // Next character needs to be 'E' (uppercase or lowercase)
    p++;
    if (*p != L'E' && *p != L'e')
        return FALSE;

    // An optional hyphen may follow now.
    p++;
    if (*p == L'-')
        p++;

    // Now an arbitrary number of digits may follow.
    while (*p >= L'0' && *p <= L'9')
        p++;

    // Finally, the virtual Ne port must be terminated by a colon.
    if (*p != ':')
        return FALSE;

    // If this is the end of the string, we have a virtual Ne port.
    p++;
    return (*p == L'\0');
}

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
    PLOCALMON_XCV pXcv;

    TRACE("LocalmonShutdown(%p)\n", hMonitor);

    pLocalmon = (PLOCALMON_HANDLE)hMonitor;

    // Close all virtual file ports.
    while (!IsListEmpty(&pLocalmon->FilePorts))
    {
        pPort = CONTAINING_RECORD(&pLocalmon->FilePorts.Flink, LOCALMON_PORT, Entry);
        LocalmonClosePort((HANDLE)pPort);
    }

    // Do the same for the open Xcv ports.
    while (!IsListEmpty(&pLocalmon->XcvHandles))
    {
        pXcv = CONTAINING_RECORD(&pLocalmon->XcvHandles.Flink, LOCALMON_XCV, Entry);
        LocalmonXcvClosePort((HANDLE)pXcv);
    }

    // Now close all registry ports, remove them from the list and free their memory.
    while (!IsListEmpty(&pLocalmon->RegistryPorts))
    {
        pPort = CONTAINING_RECORD(&pLocalmon->RegistryPorts.Flink, LOCALMON_PORT, Entry);
        LocalmonClosePort((HANDLE)pPort);
        RemoveEntryList(&pPort->Entry);
        DllFreeSplMem(pPort);
    }

    // Finally clean the LOCALMON_HANDLE structure itself.
    DeleteCriticalSection(&pLocalmon->Section);
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

    TRACE("InitializePrintMonitor2(%p, %p)\n", pMonitorInit, phMonitor);

    // Create a new LOCALMON_HANDLE structure.
    pLocalmon = DllAllocSplMem(sizeof(LOCALMON_HANDLE));
    InitializeCriticalSection(&pLocalmon->Section);
    InitializeListHead(&pLocalmon->FilePorts);
    InitializeListHead(&pLocalmon->RegistryPorts);
    InitializeListHead(&pLocalmon->XcvHandles);

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

        pPort->pLocalmon = pLocalmon;
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

        // pwszPortName can be one of the following to be valid for this Port Monitor:
        //    COMx:                        - Physical COM port
        //    LPTx:                        - Physical LPT port (or redirected one using "net use LPT1 ...")
        //    FILE:                        - Opens a prompt that asks for an output filename
        //    C:\bla.txt                   - Redirection into the file "C:\bla.txt"
        //    \\COMPUTERNAME\PrinterName   - Redirection to a shared network printer installed as a local port
        //
        // We can't detect valid and invalid ones by the name, so we can only exclude empty ports and the virtual "Ne00:", "Ne01:", ... ports.
        // Skip the invalid ones here.
        if (!cchPortName || _IsNEPort(pPort->pwszPortName))
        {
            DllFreeSplMem(pPort);
            pPort = NULL;
            continue;
        }

        // Add it to the list.
        InsertTailList(&pLocalmon->RegistryPorts, &pPort->Entry);

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
