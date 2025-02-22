/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Xcv* functions
 * COPYRIGHT:   Copyright 2020 ReactOS
 */

#include "precomp.h"

static DWORD
_HandleAddPort(HANDLE hXcv, PBYTE pInputData, PDWORD pcbOutputNeeded, DWORD* pdwStatus)
{
    DWORD res;
    PLOCAL_PRINT_MONITOR pPrintMonitor;
    PLOCAL_XCV_HANDLE pXcv;
    PLOCAL_HANDLE pHandle = (PLOCAL_HANDLE)hXcv;
    PWSTR pPortName = (PWSTR)pInputData;

    FIXME("LocalXcvAddPort : %s\n", debugstr_w( pPortName ) );

    // Check if this is a printer handle.
    if (pHandle->HandleType != HandleType_Xcv)
    {
        ERR("LocalXcvAddPort : Invalid XCV Handle\n");
        res = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    pXcv = (PLOCAL_XCV_HANDLE)pHandle->pSpecificHandle;

    pPrintMonitor = pXcv->pPrintMonitor;
    if (!pPrintMonitor )
    {
        res = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Sanity checks
    if (!pInputData || !pcbOutputNeeded)
    {
        res = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    pPrintMonitor->refcount++;
    if ( pPrintMonitor->bIsLevel2 && ((PMONITOR2)pPrintMonitor->pMonitor)->pfnXcvDataPort )
    {
        res = ((PMONITOR2)pPrintMonitor->pMonitor)->pfnXcvDataPort(pXcv->hXcv, L"AddPort", pInputData, 0, NULL, 0, pcbOutputNeeded);
    }
    else if ( !pPrintMonitor->bIsLevel2 && ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnXcvDataPort )
    {
        res = ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnXcvDataPort(pXcv->hXcv, L"AddPort", pInputData, 0, NULL, 0, pcbOutputNeeded);
    }
    pPrintMonitor->refcount--;

    if ( res == ERROR_SUCCESS )
    {
        CreatePortEntry( pPortName, pPrintMonitor );
    }

    FIXME("=> %u\n", res);

Cleanup:
    if (pdwStatus) *pdwStatus = res;
    return res;
}

static DWORD
_HandleDeletePort(HANDLE hXcv, PBYTE pInputData, PDWORD pcbOutputNeeded, DWORD* pdwStatus)
{
    DWORD res;
    PLOCAL_PRINT_MONITOR pPrintMonitor;
    PLOCAL_XCV_HANDLE pXcv;
    PLOCAL_PORT pPort;
    PLOCAL_HANDLE pHandle = (PLOCAL_HANDLE)hXcv;
    PWSTR pPortName = (PWSTR)pInputData;

    FIXME("LocalXcvDeletePort : %s\n", debugstr_w( pPortName ) );

    // Check if this is a printer handle.
    if (pHandle->HandleType != HandleType_Xcv)
    {
        ERR("LocalXcvDeletePort : Invalid XCV Handle\n");
        res = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    pXcv = (PLOCAL_XCV_HANDLE)pHandle->pSpecificHandle;

    pPrintMonitor = pXcv->pPrintMonitor;
    if (!pPrintMonitor )
    {
        res = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Sanity checks
    if (!pInputData || !pcbOutputNeeded)
    {
        res = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }


    pPrintMonitor->refcount++;
    //
    //  Call back to monitor, update the Registry and Registry List.
    //
    if ( pPrintMonitor->bIsLevel2 && ((PMONITOR2)pPrintMonitor->pMonitor)->pfnXcvDataPort )
    {
        res = ((PMONITOR2)pPrintMonitor->pMonitor)->pfnXcvDataPort(pXcv->hXcv, L"DeletePort", pInputData, 0, NULL, 0, pcbOutputNeeded);
    }
    else if ( !pPrintMonitor->bIsLevel2 && ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnXcvDataPort )
    {
        res = ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnXcvDataPort(pXcv->hXcv, L"DeletePort", pInputData, 0, NULL, 0, pcbOutputNeeded);
    }
    pPrintMonitor->refcount--;
    //
    //  Now find and remove Local Port data.
    //
    if ( res == ERROR_SUCCESS )
    {
        pPort = FindPort( pPortName );
        if (pPort )
        {
            FIXME("LocalXcvDeletePort removed Port Entry\n");
            RemoveEntryList(&pPort->Entry);

            DllFreeSplMem(pPort);
        }
        FIXME("=> %u with %u\n", res, GetLastError() );
    }

Cleanup:
    if (pdwStatus) *pdwStatus = res;
    return res;
}

BOOL WINAPI
LocalXcvData(HANDLE hXcv, const WCHAR* pszDataName, BYTE* pInputData, DWORD cbInputData, BYTE* pOutputData, DWORD cbOutputData, DWORD* pcbOutputNeeded, DWORD* pdwStatus)
{
    DWORD res;
    PLOCAL_PRINT_MONITOR pPrintMonitor;
    PLOCAL_XCV_HANDLE pXcv;
    PLOCAL_HANDLE pHandle = (PLOCAL_HANDLE)hXcv;

    FIXME("LocalXcvData(%p, %S, %p, %lu, %p, %lu, %p)\n", hXcv, pszDataName, pInputData, cbInputData, pOutputData, cbOutputData, pcbOutputNeeded);

    // Sanity checks
    if (!pszDataName)
    {
        res = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Call the appropriate handler for the requested data name.
    if (wcscmp(pszDataName, L"AddPort") == 0)
        return _HandleAddPort(hXcv, pInputData, pcbOutputNeeded, pdwStatus);

    if (wcscmp(pszDataName, L"DeletePort") == 0)
        return _HandleDeletePort(hXcv, pInputData, pcbOutputNeeded, pdwStatus);

    //
    // After the two Intercept Handlers, defer call back to Monitor.
    //

    // Check if this is a printer handle.
    if (pHandle->HandleType != HandleType_Xcv)
    {
        ERR("LocalXcvData : Invalid XCV Handle\n");
        res = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    pXcv = (PLOCAL_XCV_HANDLE)pHandle->pSpecificHandle;

    pPrintMonitor = pXcv->pPrintMonitor;
    if (!pPrintMonitor )
    {
        res = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    pPrintMonitor->refcount++;
    if ( pPrintMonitor->bIsLevel2 && ((PMONITOR2)pPrintMonitor->pMonitor)->pfnXcvDataPort )
    {
        res = ((PMONITOR2)pPrintMonitor->pMonitor)->pfnXcvDataPort(pXcv->hXcv, pszDataName, pInputData, cbInputData, pOutputData, cbOutputData, pcbOutputNeeded);
    }
    else if ( !pPrintMonitor->bIsLevel2 && ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnXcvDataPort )
    {
        res = ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnXcvDataPort(pXcv->hXcv, pszDataName, pInputData, cbInputData, pOutputData, cbOutputData, pcbOutputNeeded);
    }
    pPrintMonitor->refcount--;

Cleanup:
    SetLastError(res);
    if (pdwStatus) *pdwStatus = res;
    return res;
}
