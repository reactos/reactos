/*
 * PROJECT:     ReactOS Local Spooler
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Various tools
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
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

/******************************************************************
 * copy_servername_from_name  (internal)
 *
 * for an external server, the serverpart from the name is copied.
 *
 * RETURNS
 *  the length (in WCHAR) of the serverpart (0 for the local computer)
 *  (-length), when the name is too long
 *
 */
LONG copy_servername_from_name(LPCWSTR name, LPWSTR target)
{
    LPCWSTR server;
    LPWSTR  ptr;
    WCHAR   buffer[MAX_COMPUTERNAME_LENGTH +1];
    DWORD   len;
    DWORD   serverlen;

    if (target) *target = '\0';

    if (name == NULL) return 0;
    if ((name[0] != '\\') || (name[1] != '\\')) return 0;

    server = &name[2];
    /* skip over both backslash, find separator '\' */
    ptr = wcschr(server, '\\');
    serverlen = (ptr) ? ptr - server : lstrlenW(server);

    /* servername is empty */
    if (serverlen == 0) return 0;

    FIXME("found %s\n", debugstr_wn(server, serverlen));

    if (serverlen > MAX_COMPUTERNAME_LENGTH) return -serverlen;

    if (target)
    {
        memcpy(target, server, serverlen * sizeof(WCHAR));
        target[serverlen] = '\0';
    }

    len = ARRAYSIZE(buffer);
    if (GetComputerNameW(buffer, &len))
    {
        if ((serverlen == len) && (_wcsnicmp(server, buffer, len) == 0))
        {
            /* The requested Servername is our computername */
            return 0;
        }
    }
    return serverlen;
}
