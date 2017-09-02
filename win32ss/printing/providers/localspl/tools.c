/*
 * PROJECT:     ReactOS Local Spooler
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Various tools
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

/**
 * @name AllocAndRegQueryWSZ
 *
 * Queries a REG_SZ value in the registry, allocates memory for it and returns a buffer containing the value.
 * You have to free this buffer using DllFreeSplMem.
 *
 * @param hKey
 * HKEY variable of the key opened with RegOpenKeyExW.
 *
 * @param pwszValueName
 * Name of the REG_SZ value to query.
 *
 * @return
 * Pointer to the buffer containing the value or NULL in case of failure.
 */
PWSTR
AllocAndRegQueryWSZ(HKEY hKey, PCWSTR pwszValueName)
{
    DWORD cbNeeded;
    LONG lStatus;
    PWSTR pwszValue;

    // Determine the size of the required buffer.
    lStatus = RegQueryValueExW(hKey, pwszValueName, NULL, NULL, NULL, &cbNeeded);
    if (lStatus != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed with status %ld!\n", lStatus);
        return NULL;
    }

    // Allocate it.
    pwszValue = DllAllocSplMem(cbNeeded);
    if (!pwszValue)
    {
        ERR("DllAllocSplMem failed!\n");
        return NULL;
    }

    // Now get the actual value.
    lStatus = RegQueryValueExW(hKey, pwszValueName, NULL, NULL, (PBYTE)pwszValue, &cbNeeded);
    if (lStatus != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed with status %ld!\n", lStatus);
        DllFreeSplMem(pwszValue);
        return NULL;
    }

    return pwszValue;
}

PDEVMODEW
DuplicateDevMode(PDEVMODEW pInput)
{
    PDEVMODEW pOutput;

    // Allocate a buffer for this DevMode.
    pOutput = DllAllocSplMem(pInput->dmSize + pInput->dmDriverExtra);
    if (!pOutput)
    {
        ERR("DllAllocSplMem failed!\n");
        return NULL;
    }

    // Copy it.
    CopyMemory(pOutput, pInput, pInput->dmSize + pInput->dmDriverExtra);

    return pOutput;
}
