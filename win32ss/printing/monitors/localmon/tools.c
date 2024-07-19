/*
 * PROJECT:     ReactOS Local Port Monitor
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Various support functions shared by multiple files
 * COPYRIGHT:   Copyright 2015 Colin Finck (colin@reactos.org)
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
        if (_wcsicmp(p->pName, pwszPortName) == 0)
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
GetLPTTransmissionRetryTimeout(VOID)
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

DWORD
GetTypeFromName(LPCWSTR name)
{
    HANDLE  hfile;

    if (!wcsncmp(name, L"LPT", ARRAYSIZE(L"LPT") - 1) )
        return PORT_IS_LPT;

    if (!wcsncmp(name, L"COM", ARRAYSIZE(L"COM") - 1) )
        return PORT_IS_COM;

    if (!lstrcmpW(name, L"FILE:") )
        return PORT_IS_FILE;

//    if (name[0] == '/')
//        return PORT_IS_UNIXNAME;

//    if (name[0] == '|')
//        return PORT_IS_PIPE;

    if ( _IsNEPort( name ) )
        return PORT_IS_VNET;

    if (!wcsncmp(name, L"XPS", ARRAYSIZE(L"XPS") - 1))
        return PORT_IS_XPS;

    /* Must be a file or a directory. Does the file exist ? */
    hfile = CreateFileW(name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    FIXME("%p for OPEN_EXISTING on %s\n", hfile, debugstr_w(name));

    if (hfile == INVALID_HANDLE_VALUE)
    {
        /* Can we create the file? */
        hfile = CreateFileW(name, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, NULL);
        FIXME("%p for OPEN_ALWAYS\n", hfile);
    }

    if (hfile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hfile); FIXME("PORT_IS_FILENAME %d\n",PORT_IS_FILENAME);
        return PORT_IS_FILENAME;
    }
    FIXME("PORT_IS_UNKNOWN %d\n",PORT_IS_UNKNOWN);
    /* We can't use the name. use GetLastError() for the reason */
    return PORT_IS_UNKNOWN;
}
