/*
 * PROJECT:     ReactOS Local Port Monitor
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Various support functions shared by multiple files
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

/**
 * @name DoesPortExist
 *
 * Checks all Port Monitors installed on the local system to find out if a given port already exists.
 *
 * @param pwszPortName
 * The port name to check.
 *
 * @return
 * TRUE if a port with that name already exists on the local system.
 * If the return value is FALSE, either the port doesn't exist or an error occurred.
 * Use GetLastError in this case to check the error case.
 */
BOOL
DoesPortExist(PCWSTR pwszPortName)
{
    BOOL bReturnValue = FALSE;
    DWORD cbNeeded;
    DWORD dwErrorCode;
    DWORD dwReturned;
    DWORD i;
    PPORT_INFO_1W p;
    PPORT_INFO_1W pPortInfo1 = NULL;

    // Determine the required buffer size.
    EnumPortsW(NULL, 1, NULL, 0, &cbNeeded, &dwReturned);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        dwErrorCode = GetLastError();
        ERR("EnumPortsW failed with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Allocate a buffer large enough.
    pPortInfo1 = DllAllocSplMem(cbNeeded);
    if (!pPortInfo1)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("DllAllocSplMem failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    // Now get the actual port information.
    if (!EnumPortsW(NULL, 1, (PBYTE)pPortInfo1, cbNeeded, &cbNeeded, &dwReturned))
    {
        dwErrorCode = GetLastError();
        ERR("EnumPortsW failed with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // We were successful! Loop through all returned ports.
    dwErrorCode = ERROR_SUCCESS;
    p = pPortInfo1;

    for (i = 0; i < dwReturned; i++)
    {
        // Check if this existing port matches our queried one.
        if (wcsicmp(p->pName, pwszPortName) == 0)
        {
            bReturnValue = TRUE;
            goto Cleanup;
        }

        p++;
    }

Cleanup:
    if (pPortInfo1)
        DllFreeSplMem(pPortInfo1);

    SetLastError(dwErrorCode);
    return bReturnValue;
}

DWORD
GetLPTTransmissionRetryTimeout()
{
    DWORD cbBuffer;
    DWORD dwReturnValue = 90;       // Use 90 seconds as default if we fail to read from registry.
    HKEY hKey;
    LSTATUS lStatus;

    // Six digits is the most you can enter in Windows' LocalUI.dll.
    // Larger values make it crash, so introduce a limit here.
    WCHAR wszBuffer[6 + 1];

    // Open the key where our value is stored.
    lStatus = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows", 0, KEY_READ, &hKey);
    if (lStatus != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed with status %ld!\n", lStatus);
        goto Cleanup;
    }

    // Query the value.
    cbBuffer = sizeof(wszBuffer);
    lStatus = RegQueryValueExW(hKey, L"TransmissionRetryTimeout", NULL, NULL, (PBYTE)wszBuffer, &cbBuffer);
    if (lStatus != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed with status %ld!\n", lStatus);
        goto Cleanup;
    }

    // Return it converted to a DWORD.
    dwReturnValue = wcstoul(wszBuffer, NULL, 10);

Cleanup:
    if (hKey)
        RegCloseKey(hKey);

    return dwReturnValue;
}

/**
 * @name GetPortNameWithoutColon
 *
 * Most of the time, we operate on port names with a trailing colon. But some functions require the name without the trailing colon.
 * This function checks if the port has a trailing colon and if so, it returns the port name without the colon.
 *
 * @param pwszPortName
 * The port name with colon
 *
 * @param ppwszPortNameWithoutColon
 * Pointer to a PWSTR that will contain the port name without colon.
 * You have to free this buffer using DllFreeSplMem.
 *
 * @return
 * ERROR_SUCCESS if the port name without colon was successfully copied into the buffer.
 * ERROR_INVALID_PARAMETER if this port name has no trailing colon.
 * ERROR_NOT_ENOUGH_MEMORY if memory allocation failed.
 */
DWORD
GetPortNameWithoutColon(PCWSTR pwszPortName, PWSTR* ppwszPortNameWithoutColon)
{
    DWORD cchPortNameWithoutColon;

    // Compute the string length of pwszPortNameWithoutColon.
    cchPortNameWithoutColon = wcslen(pwszPortName) - 1;

    // Check if pwszPortName really has a colon as the last character.
    if (pwszPortName[cchPortNameWithoutColon] != L':')
        return ERROR_INVALID_PARAMETER;

    // Allocate the output buffer.
    *ppwszPortNameWithoutColon = DllAllocSplMem((cchPortNameWithoutColon + 1) * sizeof(WCHAR));
    if (!*ppwszPortNameWithoutColon)
    {
        ERR("DllAllocSplMem failed with error %lu!\n", GetLastError());
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Copy the port name without colon into the buffer.
    // The buffer is already zero-initialized, so no additional null-termination is necessary.
    CopyMemory(*ppwszPortNameWithoutColon, pwszPortName, cchPortNameWithoutColon * sizeof(WCHAR));

    return ERROR_SUCCESS;
}
