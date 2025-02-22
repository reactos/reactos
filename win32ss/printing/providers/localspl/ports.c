/*
 * PROJECT:     ReactOS Local Spooler
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Ports of the Print Monitors
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

// Local Variables
static LIST_ENTRY _PortList;


PLOCAL_PORT
FindPort(PCWSTR pwszName)
{
    PLIST_ENTRY pEntry;
    PLOCAL_PORT pPort;

    TRACE("FindPort(%S)\n", pwszName);

    if (!pwszName)
        return NULL;

    for (pEntry = _PortList.Flink; pEntry != &_PortList; pEntry = pEntry->Flink)
    {
        pPort = CONTAINING_RECORD(pEntry, LOCAL_PORT, Entry);

        if (_wcsicmp(pPort->pwszName, pwszName) == 0)
            return pPort;
    }

    return NULL;
}

BOOL
CreatePortEntry( PCWSTR pwszName, PLOCAL_PRINT_MONITOR pPrintMonitor )
{
    PLOCAL_PORT pPort;
    DWORD cbPortName = (wcslen( pwszName ) + 1) * sizeof(WCHAR);

    // Create a new LOCAL_PORT structure for it.
    pPort = DllAllocSplMem(sizeof(LOCAL_PORT) + cbPortName);
    if (!pPort)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    pPort->pPrintMonitor = pPrintMonitor;
    pPort->pwszName = wcscpy( (PWSTR)(pPort+1), pwszName );

    // Insert it into the list and advance to the next port.
    InsertTailList(&_PortList, &pPort->Entry);

    return TRUE;
}

BOOL
InitializePortList(void)
{
    BOOL bReturnValue;
    DWORD cbNeeded;
    DWORD cbPortName;
    DWORD dwErrorCode;
    DWORD dwReturned;
    DWORD i;
    PLOCAL_PORT pPort;
    PLOCAL_PRINT_MONITOR pPrintMonitor;
    PLIST_ENTRY pEntry;
    PPORT_INFO_1W p;
    PPORT_INFO_1W pPortInfo1 = NULL;

    TRACE("InitializePortList()\n");

    // Initialize an empty list for our Ports.
    InitializeListHead(&_PortList);

    // Loop through all Print Monitors.
    for (pEntry = PrintMonitorList.Flink; pEntry != &PrintMonitorList; pEntry = pEntry->Flink)
    {
        // Cleanup from the previous run.
        if (pPortInfo1)
        {
            DllFreeSplMem(pPortInfo1);
            pPortInfo1 = NULL;
        }

        pPrintMonitor = CONTAINING_RECORD(pEntry, LOCAL_PRINT_MONITOR, Entry);

        // Determine the required buffer size for EnumPorts.
        if (pPrintMonitor->bIsLevel2)
            bReturnValue = ((PMONITOR2)pPrintMonitor->pMonitor)->pfnEnumPorts(pPrintMonitor->hMonitor, NULL, 1, NULL, 0, &cbNeeded, &dwReturned);
        else
            bReturnValue = ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnEnumPorts(NULL, 1, NULL, 0, &cbNeeded, &dwReturned);

        // Check the returned error code.
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            ERR("Print Monitor \"%S\" failed with error %lu on EnumPorts!\n", pPrintMonitor->pwszName, GetLastError());
            continue;
        }

        // Allocate a buffer large enough.
        pPortInfo1 = DllAllocSplMem(cbNeeded);
        if (!pPortInfo1)
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("DllAllocSplMem failed!\n");
            goto Cleanup;
        }

        // Get the ports handled by this monitor.
        if (pPrintMonitor->bIsLevel2)
            bReturnValue = ((PMONITOR2)pPrintMonitor->pMonitor)->pfnEnumPorts(pPrintMonitor->hMonitor, NULL, 1, (PBYTE)pPortInfo1, cbNeeded, &cbNeeded, &dwReturned);
        else
            bReturnValue = ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnEnumPorts(NULL, 1, (PBYTE)pPortInfo1, cbNeeded, &cbNeeded, &dwReturned);

        // Check the return value.
        if (!bReturnValue)
        {
            ERR("Print Monitor \"%S\" failed with error %lu on EnumPorts!\n", pPrintMonitor->pwszName, GetLastError());
            continue;
        }

        // Loop through all returned ports.
        p = pPortInfo1;

        for (i = 0; i < dwReturned; i++)
        {
            cbPortName = (wcslen(p->pName) + 1) * sizeof(WCHAR);

            // Create a new LOCAL_PORT structure for it.
            pPort = DllAllocSplMem(sizeof(LOCAL_PORT) + cbPortName);
            if (!pPort)
            {
                dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                ERR("DllAllocSplMem failed!\n");
                goto Cleanup;
            }

            pPort->pPrintMonitor = pPrintMonitor;
            pPort->pwszName = (PWSTR)((PBYTE)pPort + sizeof(LOCAL_PORT));
            CopyMemory(pPort->pwszName, p->pName, cbPortName);

            // Insert it into the list and advance to the next port.
            InsertTailList(&_PortList, &pPort->Entry);
            p++;
        }
    }

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    // Inside the loop
    if (pPortInfo1)
        DllFreeSplMem(pPortInfo1);

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
LocalEnumPorts(PWSTR pName, DWORD Level, PBYTE pPorts, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    BOOL bReturnValue = TRUE;
    DWORD cbCallBuffer;
    DWORD cbNeeded;
    DWORD dwReturned;
    PBYTE pCallBuffer;
    PLOCAL_PRINT_MONITOR pPrintMonitor;
    PLIST_ENTRY pEntry;

    TRACE("LocalEnumPorts(%S, %lu, %p, %lu, %p, %p)\n", pName, Level, pPorts, cbBuf, pcbNeeded, pcReturned);

    // Begin counting.
    *pcbNeeded = 0;
    *pcReturned = 0;

    // At the beginning, we have the full buffer available.
    cbCallBuffer = cbBuf;
    pCallBuffer = pPorts;

    // Loop through all Print Monitors.
    for (pEntry = PrintMonitorList.Flink; pEntry != &PrintMonitorList; pEntry = pEntry->Flink)
    {
        pPrintMonitor = CONTAINING_RECORD(pEntry, LOCAL_PRINT_MONITOR, Entry);

        // Call the EnumPorts function of this Print Monitor.
        cbNeeded = 0;
        dwReturned = 0;

        if (pPrintMonitor->bIsLevel2)
            bReturnValue = ((PMONITOR2)pPrintMonitor->pMonitor)->pfnEnumPorts(pPrintMonitor->hMonitor, pName, Level, pCallBuffer, cbCallBuffer, &cbNeeded, &dwReturned);
        else
            bReturnValue = ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnEnumPorts(pName, Level, pCallBuffer, cbCallBuffer, &cbNeeded, &dwReturned);

        // Add the returned counts to the total values.
        *pcbNeeded += cbNeeded;
        *pcReturned += dwReturned;

        // Reduce the available buffer size for the next call without risking an underflow.
        if (cbNeeded < cbCallBuffer)
            cbCallBuffer -= cbNeeded;
        else
            cbCallBuffer = 0;

        // Advance the buffer if the caller provided it.
        if (pCallBuffer)
            pCallBuffer += cbNeeded;
    }

    return bReturnValue;
}

BOOL WINAPI
LocalAddPortEx(PWSTR pName, DWORD Level, PBYTE lpBuffer, PWSTR lpMonitorName)
{
    DWORD lres;
    BOOL res = FALSE;
    PLOCAL_PORT pPort;
    PLOCAL_PRINT_MONITOR pPrintMonitor;
    PORT_INFO_1W * pi = (PORT_INFO_1W *) lpBuffer;

    FIXME("LocalAddPortEx(%S, %lu, %p, %S)\n", pName, Level, lpBuffer, lpMonitorName);

    lres = copy_servername_from_name(pName, NULL);
    if ( lres )
    {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ( Level != 1 )
    {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if ((!pi) || (!lpMonitorName) || (!lpMonitorName[0]))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pPrintMonitor = FindPrintMonitor( lpMonitorName );
    if (!pPrintMonitor )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pPort = FindPort( pi->pName );
    if ( pPort )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ( pPrintMonitor->bIsLevel2 && ((PMONITOR2)pPrintMonitor->pMonitor)->pfnAddPortEx )
    {
        res = ((PMONITOR2)pPrintMonitor->pMonitor)->pfnAddPortEx(pPrintMonitor->hMonitor, pName, Level, lpBuffer, lpMonitorName);
    }
    else if ( !pPrintMonitor->bIsLevel2 && ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnAddPortEx )
    {
        res = ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnAddPortEx(pName, Level, lpBuffer, lpMonitorName);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    if ( res )
    {
        res = CreatePortEntry( pi->pName, pPrintMonitor );
    }

    return res;
}

//
// Local (AP, CP & DP) is still around, seems to be a backup if a failure was encountered.. New way, WinSpool->LocalUI->XcvDataW.
//
BOOL WINAPI
LocalAddPort(LPWSTR pName, HWND hWnd, LPWSTR pMonitorName)
{
    DWORD lres;
    BOOL res = FALSE;
    PLOCAL_PRINT_MONITOR pPrintMonitor;

    FIXME("LocalAddPort(%S, %p, %s)\n", pName, hWnd, debugstr_w(pMonitorName));

    lres = copy_servername_from_name(pName, NULL);
    if (lres)
    {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* an empty Monitorname is Invalid */
    if (!pMonitorName[0])
    {
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    pPrintMonitor = FindPrintMonitor( pMonitorName );
    if (!pPrintMonitor )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ( pPrintMonitor->bIsLevel2 && ((PMONITOR2)pPrintMonitor->pMonitor)->pfnAddPort )
    {
        res = ((PMONITOR2)pPrintMonitor->pMonitor)->pfnAddPort(pPrintMonitor->hMonitor, pName, hWnd, pMonitorName);
    }
    else if ( !pPrintMonitor->bIsLevel2 && ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnAddPort )
    {
        res = ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnAddPort(pName, hWnd, pMonitorName);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    if ( res )
    {
        DWORD cbNeeded, cReturned, i;
        PPORT_INFO_1 pPorts;

        //
        // Play it safe,,, we know its Monitor2.... This is ReactOS.
        //
        if ( LocalEnumPorts( pName, 1, NULL, 0, &cbNeeded, &cReturned ) )
        {
            pPorts = DllAllocSplMem( cbNeeded );
            if (pPorts)
            {
                if ( LocalEnumPorts( pName, 1, (PBYTE)pPorts, cbNeeded, &cbNeeded, &cReturned ) )
                {
                    for ( i = 0; i < cReturned; i++ )
                    {
                        if ( !FindPort( pPorts[i].pName ) )
                        {
                            CreatePortEntry( pPorts[i].pName, pPrintMonitor );
                        }
                    }
                }
                DllFreeSplMem( pPorts );
            }
        }
    }

    return res;
}

BOOL WINAPI
LocalConfigurePort(PWSTR pName, HWND hWnd, PWSTR pPortName)
{
    LONG lres;
    DWORD res;
    PLOCAL_PORT pPrintPort;
    PLOCAL_PRINT_MONITOR pPrintMonitor;

    FIXME("LocalConfigurePort(%S, %p, %S)\n", pName, hWnd, pPortName);

    lres = copy_servername_from_name(pName, NULL);
    if (lres)
    {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    /* an empty Portname is Invalid, but can popup a Dialog */
    if (!pPortName[0])
    {
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    pPrintPort = FindPort(pPortName);
    if (!pPrintPort )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pPrintMonitor = pPrintPort->pPrintMonitor;

    if ( pPrintMonitor->bIsLevel2 && ((PMONITOR2)pPrintMonitor->pMonitor)->pfnConfigurePort )
    {
        res = ((PMONITOR2)pPrintMonitor->pMonitor)->pfnConfigurePort(pPrintMonitor->hMonitor, pName, hWnd, pPortName);
    }
    else if ( !pPrintMonitor->bIsLevel2 && ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnConfigurePort )
    {
        res = ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnConfigurePort(pName, hWnd, pPortName);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return res;
}

BOOL WINAPI
LocalDeletePort(PWSTR pName, HWND hWnd, PWSTR pPortName)
{
    LONG lres;
    DWORD res = FALSE;
    PLOCAL_PORT pPrintPort;
    PLOCAL_PRINT_MONITOR pPrintMonitor;

    FIXME("LocalDeletePort(%S, %p, %S)\n", pName, hWnd, pPortName);

    lres = copy_servername_from_name(pName, NULL);
    if (lres)
    {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    if (!pPortName[0])
    {
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    pPrintPort = FindPort(pPortName);
    if (!pPrintPort )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pPrintMonitor = pPrintPort->pPrintMonitor;

    if ( pPrintMonitor->bIsLevel2 && ((PMONITOR2)pPrintMonitor->pMonitor)->pfnDeletePort )
    {
        res = ((PMONITOR2)pPrintMonitor->pMonitor)->pfnDeletePort(pPrintMonitor->hMonitor, pName, hWnd, pPortName);
    }
    else if ( !pPrintMonitor->bIsLevel2 && ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnDeletePort )
    {
        res = ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnDeletePort(pName, hWnd, pPortName);
    }

    RemoveEntryList(&pPrintPort->Entry);

    DllFreeSplMem(pPrintPort);

    return res;
}

BOOL WINAPI
LocalSetPort(PWSTR pName, PWSTR pPortName, DWORD dwLevel, PBYTE pPortInfo)
{
    LONG lres;
    DWORD res = 0;
    PPORT_INFO_3W ppi3w = (PPORT_INFO_3W)pPortInfo;
    PLOCAL_PORT pPrintPort;

    TRACE("LocalSetPort(%S, %S, %lu, %p)\n", pName, pPortName, dwLevel, pPortInfo);

    lres = copy_servername_from_name(pName, NULL);
    if (lres)
    {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    if ((dwLevel < 1) || (dwLevel > 2))
    {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if ( !ppi3w )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pPrintPort = FindPort(pPortName);
    if ( !pPrintPort )
    {
        SetLastError(ERROR_UNKNOWN_PORT);
        return FALSE;
    }

    FIXME("Add Status Support to Local Ports!\n");

    return res;
}
