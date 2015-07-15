/*
 * PROJECT:     ReactOS Local Port Monitor
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Implementation of Xcv* and support functions
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

static DWORD
_HandleAddPort(PLOCALMON_XCV pXcv, PBYTE pInputData, PDWORD pcbOutputNeeded)
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/**
 * @name _HandleConfigureLPTPortCommandOK
 *
 * Writes the value for "TransmissionRetryTimeout" to the registry. Checks for granted SERVER_ACCESS_ADMINISTER access.
 * Actually the opposite of _HandleGetTransmissionRetryTimeout, but name kept for compatibility.
 *
 * @param pXcv
 * Pointer to the LOCALMON_XCV structure of the currently opened Xcv port.
 *
 * @param pInputData
 * Pointer to a Unicode string containing the value to be written to the registry.
 *
 * @param pcbOutputNeeded
 * Pointer to a DWORD that will be zeroed on return.
 *
 * @return
 * An error code indicating success or failure.
 */
static DWORD
_HandleConfigureLPTPortCommandOK(PLOCALMON_XCV pXcv, PBYTE pInputData, PDWORD pcbOutputNeeded)
{
    DWORD cbBuffer;
    DWORD dwErrorCode;
    HKEY hKey = NULL;
    HKEY hToken = NULL;

    // Sanity checks
    if (!pXcv || !pInputData || !pcbOutputNeeded)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    *pcbOutputNeeded = 0;

    // This action can only happen at SERVER_ACCESS_ADMINISTER access level.
    if (!(pXcv->GrantedAccess & SERVER_ACCESS_ADMINISTER))
    {
        dwErrorCode = ERROR_ACCESS_DENIED;
        goto Cleanup;
    }

    // Switch to the SYSTEM context for modifying the registry.
    hToken = RevertToPrinterSelf();
    if (!hToken)
    {
        dwErrorCode = GetLastError();
        ERR("RevertToPrinterSelf failed with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Open the key where our value is stored.
    dwErrorCode = (DWORD)RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows", 0, KEY_SET_VALUE, &hKey);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // We don't use cbInputData here, because the buffer pInputData could be bigger than the data it contains.
    cbBuffer = (wcslen((PWSTR)pInputData) + 1) * sizeof(WCHAR);

    // Write the value to the registry.
    dwErrorCode = (DWORD)RegSetValueExW(hKey, L"TransmissionRetryTimeout", 0, REG_SZ, pInputData, cbBuffer);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RegSetValueExW failed with status %lu!\n", dwErrorCode);
        goto Cleanup;
    }

Cleanup:
    if (hKey)
        RegCloseKey(hKey);

    if (hToken)
        ImpersonatePrinterClient(hToken);

    return dwErrorCode;
}

static DWORD
_HandleDeletePort(PLOCALMON_XCV pXcv, PBYTE pInputData, PDWORD pcbOutputNeeded)
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/**
 * @name _HandleGetDefaultCommConfig
 *
 * Gets the default configuration of a legacy port.
 * The opposite function is _HandleSetDefaultCommConfig.
 *
 * @param pInputData
 * The port name (without colon!) whose default configuration you want to get.
 *
 * @param pOutputData
 * Pointer to a COMMCONFIG structure that will receive the configuration information.
 *
 * @param cbOutputData
 * Size of the variable pointed to by pOutputData.
 *
 * @param pcbOutputNeeded
 * Pointer to a DWORD that contains the required size for pOutputData on return.
 *
 * @return
 * An error code indicating success or failure.
 */
static DWORD
_HandleGetDefaultCommConfig(PBYTE pInputData, PBYTE pOutputData, DWORD cbOutputData, PDWORD pcbOutputNeeded)
{
    // Sanity checks
    if (!pInputData || !pcbOutputNeeded)
        return ERROR_INVALID_PARAMETER;

    *pcbOutputNeeded = sizeof(COMMCONFIG);

    // Check if the supplied buffer is large enough.
    if (cbOutputData < *pcbOutputNeeded)
        return ERROR_INSUFFICIENT_BUFFER;

    // Finally get the port configuration.
    if (!GetDefaultCommConfigW((PCWSTR)pInputData, (LPCOMMCONFIG)pOutputData, pcbOutputNeeded))
        return GetLastError();

    return ERROR_SUCCESS;
}

/**
 * @name _HandleGetTransmissionRetryTimeout
 *
 * Reads the value for "TransmissionRetryTimeout" from the registry and converts it to a DWORD.
 * The opposite function is _HandleConfigureLPTPortCommandOK.
 *
 * @param pOutputData
 * Pointer to a DWORD that will receive the timeout value.
 *
 * @param cbOutputData
 * Size of the variable pointed to by pOutputData.
 *
 * @param pcbOutputNeeded
 * Pointer to a DWORD that contains the required size for pOutputData on return.
 *
 * @return
 * An error code indicating success or failure.
 */
static DWORD
_HandleGetTransmissionRetryTimeout(PBYTE pOutputData, DWORD cbOutputData, PDWORD pcbOutputNeeded)
{
    DWORD dwTimeout;

    // Sanity checks
    if (!pOutputData || !pcbOutputNeeded)
        return ERROR_INVALID_PARAMETER;

    *pcbOutputNeeded = sizeof(DWORD);

    // Check if the supplied buffer is large enough.
    if (cbOutputData < *pcbOutputNeeded)
        return ERROR_INSUFFICIENT_BUFFER;

    // Retrieve and copy the number.
    dwTimeout = GetLPTTransmissionRetryTimeout();
    CopyMemory(pOutputData, &dwTimeout, sizeof(DWORD));
    return ERROR_SUCCESS;
}

/**
 * @name _HandleMonitorUI
 *
 * Returns the filename of the associated UI DLL for this Port Monitor.
 *
 * @param pOutputData
 * Pointer to a Unicode string that will receive the DLL filename.
 *
 * @param cbOutputData
 * Size of the variable pointed to by pOutputData.
 *
 * @param pcbOutputNeeded
 * Pointer to a DWORD that contains the required size for pOutputData on return.
 *
 * @return
 * An error code indicating success or failure.
 */
static DWORD
_HandleMonitorUI(PBYTE pOutputData, DWORD cbOutputData, PDWORD pcbOutputNeeded)
{
    const WCHAR wszMonitorUI[] = L"LocalUI.dll";

    // Sanity checks
    if (!pOutputData || !pcbOutputNeeded)
        return ERROR_INVALID_PARAMETER;

    *pcbOutputNeeded = sizeof(wszMonitorUI);

    // Check if the supplied buffer is large enough.
    if (cbOutputData < *pcbOutputNeeded)
        return ERROR_INSUFFICIENT_BUFFER;

    // Copy the string.
    CopyMemory(pOutputData, wszMonitorUI, sizeof(wszMonitorUI));
    return ERROR_SUCCESS;
}

/**
 * @name _HandlePortExists
 *
 * Checks all Port Monitors installed on the local system to find out if a given port already exists.
 *
 * @param pInputData
 * Pointer to a Unicode string specifying the port name to check.
 *
 * @param pOutputData
 * Pointer to a BOOL that receives the result of the check.
 *
 * @param cbOutputData
 * Size of the variable pointed to by pOutputData.
 *
 * @param pcbOutputNeeded
 * Pointer to a DWORD that contains the required size for pOutputData on return.
 *
 * @return
 * An error code indicating success or failure.
 */
static DWORD
_HandlePortExists(PBYTE pInputData, PBYTE pOutputData, DWORD cbOutputData, PDWORD pcbOutputNeeded)
{
    // Sanity checks
    if (!pInputData || !pOutputData || !pcbOutputNeeded)
        return ERROR_INVALID_PARAMETER;

    *pcbOutputNeeded = sizeof(BOOL);

    // Check if the supplied buffer is large enough.
    if (cbOutputData < *pcbOutputNeeded)
        return ERROR_INSUFFICIENT_BUFFER;

    // Return the check result and error code.
    *(PBOOL)pOutputData = DoesPortExist((PCWSTR)pInputData);
    return GetLastError();
}

static DWORD
_HandlePortIsValid(PBYTE pInputData, PBYTE pOutputData, DWORD cbOutputData, PDWORD pcbOutputNeeded)
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/**
 * @name _HandleSetDefaultCommConfig
 *
 * Sets the default configuration of a legacy port. Checks for granted SERVER_ACCESS_ADMINISTER access.
 * You have to supply the port name (with colon!) in XcvOpenPort.
 * The opposite function is _HandleGetDefaultCommConfig.
 *
 * @param pXcv
 * Pointer to the LOCALMON_XCV structure of the currently opened Xcv port.
 *
 * @param pInputData
 * Pointer to the COMMCONFIG structure that shall be passed to SetDefaultCommConfigW.
 *
 * @param pcbOutputNeeded
 * Pointer to a DWORD that will be zeroed on return.
 *
 * @return
 * An error code indicating success or failure.
 */
static DWORD
_HandleSetDefaultCommConfig(PLOCALMON_XCV pXcv, PBYTE pInputData, PDWORD pcbOutputNeeded)
{
    DWORD dwErrorCode;
    HANDLE hToken = NULL;
    LPCOMMCONFIG pCommConfig;
    PWSTR pwszPortNameWithoutColon = NULL;

    // Sanity checks
    // pwszObject needs to be at least 2 characters long to be a port name with a trailing colon.
    if (!pXcv || !pXcv->pwszObject || !pXcv->pwszObject[0] || !pXcv->pwszObject[1] || !pInputData || !pcbOutputNeeded)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    *pcbOutputNeeded = 0;

    // This action can only happen at SERVER_ACCESS_ADMINISTER access level.
    if (!(pXcv->GrantedAccess & SERVER_ACCESS_ADMINISTER))
    {
        dwErrorCode = ERROR_ACCESS_DENIED;
        goto Cleanup;
    }

    // SetDefaultCommConfigW needs the port name without colon.
    dwErrorCode = GetPortNameWithoutColon(pXcv->pwszObject, &pwszPortNameWithoutColon);
    if (dwErrorCode != ERROR_SUCCESS)
        goto Cleanup;

    // Switch to the SYSTEM context for setting the port configuration.
    hToken = RevertToPrinterSelf();
    if (!hToken)
    {
        dwErrorCode = GetLastError();
        ERR("RevertToPrinterSelf failed with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Finally pass the parameters to SetDefaultCommConfigW.
    pCommConfig = (LPCOMMCONFIG)pInputData;
    if (!SetDefaultCommConfigW(pwszPortNameWithoutColon, pCommConfig, pCommConfig->dwSize))
    {
        dwErrorCode = GetLastError();
        ERR("SetDefaultCommConfigW failed with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    if (hToken)
        ImpersonatePrinterClient(hToken);

    if (pwszPortNameWithoutColon)
        DllFreeSplMem(pwszPortNameWithoutColon);

    return dwErrorCode;
}

BOOL WINAPI
LocalmonXcvClosePort(HANDLE hXcv)
{
    PLOCALMON_XCV pXcv = (PLOCALMON_XCV)hXcv;

    TRACE("LocalmonXcvClosePort(%p)\n", hXcv);

    // Sanity checks
    if (!pXcv)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Remove it from the list and free the memory.
    RemoveEntryList(&pXcv->Entry);
    DllFreeSplMem(pXcv);

    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

DWORD WINAPI
LocalmonXcvDataPort(HANDLE hXcv, PCWSTR pszDataName, PBYTE pInputData, DWORD cbInputData, PBYTE pOutputData, DWORD cbOutputData, PDWORD pcbOutputNeeded)
{
    TRACE("LocalmonXcvDataPort(%p, %S, %p, %lu, %p, %lu, %p)\n", hXcv, pszDataName, pInputData, cbInputData, pOutputData, cbOutputData, pcbOutputNeeded);

    // Sanity checks
    if (!pszDataName)
        return ERROR_INVALID_PARAMETER;

    // Call the appropriate handler for the requested data name.
    if (wcscmp(pszDataName, L"AddPort") == 0)
        return _HandleAddPort((PLOCALMON_XCV)hXcv, pInputData, pcbOutputNeeded);

    if (wcscmp(pszDataName, L"ConfigureLPTPortCommandOK") == 0)
        return _HandleConfigureLPTPortCommandOK((PLOCALMON_XCV)hXcv, pInputData, pcbOutputNeeded);

    if (wcscmp(pszDataName, L"DeletePort") == 0)
        return _HandleDeletePort((PLOCALMON_XCV)hXcv, pInputData, pcbOutputNeeded);

    if (wcscmp(pszDataName, L"GetDefaultCommConfig") == 0)
        return _HandleGetDefaultCommConfig(pInputData, pOutputData, cbOutputData, pcbOutputNeeded);

    if (wcscmp(pszDataName, L"GetTransmissionRetryTimeout") == 0)
        return _HandleGetTransmissionRetryTimeout(pOutputData, cbOutputData, pcbOutputNeeded);

    if (wcscmp(pszDataName, L"MonitorUI") == 0)
        return _HandleMonitorUI(pOutputData, cbOutputData, pcbOutputNeeded);

    if (wcscmp(pszDataName, L"PortExists") == 0)
        return _HandlePortExists(pInputData, pOutputData, cbOutputData, pcbOutputNeeded);

    if (wcscmp(pszDataName, L"PortIsValid") == 0)
        return _HandlePortIsValid(pInputData, pOutputData, cbOutputData, pcbOutputNeeded);

    if (wcscmp(pszDataName, L"SetDefaultCommConfig") == 0)
        return _HandleSetDefaultCommConfig((PLOCALMON_XCV)hXcv, pInputData, pcbOutputNeeded);

    return ERROR_INVALID_PARAMETER;
}

BOOL WINAPI
LocalmonXcvOpenPort(HANDLE hMonitor, PCWSTR pwszObject, ACCESS_MASK GrantedAccess, PHANDLE phXcv)
{
    DWORD cbObject = 0;
    DWORD dwErrorCode;
    PLOCALMON_HANDLE pLocalmon = (PLOCALMON_HANDLE)hMonitor;
    PLOCALMON_XCV pXcv;

    TRACE("LocalmonXcvOpenPort(%p, %S, %lu, %p)\n", hMonitor, pwszObject, GrantedAccess, phXcv);

    // Sanity checks
    if (!pLocalmon || !phXcv)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (pwszObject)
        cbObject = (wcslen(pwszObject) + 1) * sizeof(WCHAR);

    // Create a new LOCALMON_XCV structure and fill the relevant fields.
    pXcv = DllAllocSplMem(sizeof(LOCALMON_XCV) + cbObject);
    if (!pXcv)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("DllAllocSplMem failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    pXcv->pLocalmon = pLocalmon;
    pXcv->GrantedAccess = GrantedAccess;

    if (cbObject)
    {
        pXcv->pwszObject = (PWSTR)((PBYTE)pXcv + sizeof(LOCALMON_XCV));
        CopyMemory(pXcv->pwszObject, pwszObject, cbObject);
    }

    InsertTailList(&pLocalmon->XcvHandles, &pXcv->Entry);

    // Return it as the Xcv handle.
    *phXcv = (HANDLE)pXcv;
    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}
