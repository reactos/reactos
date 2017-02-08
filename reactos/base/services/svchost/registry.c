/*
 * PROJECT:     ReactOS Service Host
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        base/services/svchost/registry.c
 * PURPOSE:     Helper functions for accessing the registry
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

#include "svchost.h"

/* FUNCTIONS *****************************************************************/

DWORD
WINAPI
RegQueryValueWithAlloc (
    _In_ HKEY hKey,
    _In_ LPCWSTR pszValueName,
    _In_ DWORD dwExpectedType,
    _Out_ PBYTE* ppbData,
    _Out_ PDWORD pdwSize
    )
{
    DWORD dwError, dwType, dwBytes;
    PBYTE pbData;
    ASSERT(hKey);
    ASSERT(pszValueName);
    ASSERT(ppbData);
    ASSERT(pdwSize);

    /* Assume failure */
    *ppbData = NULL;
    *pdwSize = 0;

    /* Query how big and what type the registry data is */
    dwBytes = 0;
    dwError = RegQueryValueExW(hKey,
                               pszValueName,
                               NULL,
                               &dwType,
                               NULL,
                               &dwBytes);
    if (dwError != ERROR_SUCCESS) return dwError;

    /* It if's not the right type, or it's sero bytes, fail*/
    if ((dwType != dwExpectedType) || (dwBytes == 0)) return ERROR_INVALID_DATA;

    /* Allocate space to hold the data */
    pbData = MemAlloc(0, dwBytes);
    if (pbData == NULL) return ERROR_OUTOFMEMORY;

    /* Now get the real registry data */
    dwError = RegQueryValueExW(hKey,
                               pszValueName,
                               NULL,
                               &dwType,
                               pbData,
                               &dwBytes);
    if (dwError != ERROR_SUCCESS)
    {
        /* We failed, free the data since it won't be needed */
        MemFree(pbData);
    }
    else
    {
        /* It worked, return the data and size back to the caller */
        *ppbData = pbData;
        *pdwSize = dwBytes;
    }

    /* All done */
    return dwError;
}

DWORD
WINAPI
RegQueryDword (
    _In_ HKEY hKey,
    _In_ LPCWSTR pszValueName,
    _Out_ PDWORD pdwValue
    )
{
    DWORD dwError, cbData, dwType;
    ASSERT(hKey);
    ASSERT(pszValueName);
    ASSERT(pdwValue);

    /* Attempt to read 4 bytes */
    cbData = sizeof(DWORD);
    dwError = RegQueryValueExW(hKey, pszValueName, 0, &dwType, 0, &cbData);

    /* If we didn't get back a DWORD... */
    if ((dwError == ERROR_SUCCESS) && (dwType != REG_DWORD))
    {
        /* Zero out the output and fail */
        *pdwValue = 0;
        dwError = ERROR_INVALID_DATATYPE;
    }

    /* All done! */
    return dwError;
}

DWORD
WINAPI
RegQueryString (
    _In_ HKEY hKey,
    _In_ LPCWSTR pszValueName,
    _In_ DWORD dwExpectedType,
    _Out_ PBYTE* ppbData
    )
{
    DWORD dwSize;
    ASSERT(hKey);
    ASSERT(pszValueName);

    /* Call the helper function */
    return RegQueryValueWithAlloc(hKey,
                                  pszValueName,
                                  dwExpectedType,
                                  ppbData,
                                  &dwSize);
}

DWORD
WINAPI
RegQueryStringA (
    _In_ HKEY hKey,
    _In_ LPCWSTR pszValueName,
    _In_ DWORD dwExpectedType,
    _Out_ LPCSTR* ppszData
    )
{
    DWORD dwError;
    LPWSTR pbLocalData;
    DWORD cchValueName, cbMultiByte;
    LPSTR pszData;
    ASSERT(hKey);
    ASSERT(pszValueName);
    ASSERT(ppszData);

    /* Assume failure */
    *ppszData = NULL;

    /* Query the string in Unicode first */
    dwError = RegQueryString(hKey,
                             pszValueName,
                             dwExpectedType,
                             (PBYTE*)&pbLocalData);
    if (dwError != ERROR_SUCCESS) return dwError;

    /* Get the length of the Unicode string */
    cchValueName = lstrlenW(pbLocalData);

    /* See how much space it would take to convert to ANSI */
    cbMultiByte = WideCharToMultiByte(CP_ACP,
                                      0,
                                      pbLocalData,
                                      cchValueName + 1,
                                      NULL,
                                      0,
                                      NULL,
                                      NULL);
    if (cbMultiByte != 0)
    {
        /* Allocate the space, assuming failure */
        dwError = ERROR_OUTOFMEMORY;
        pszData = MemAlloc(0, cbMultiByte);
        if (pszData != NULL)
        {
            /* What do you know, it worked! */
            dwError = ERROR_SUCCESS;

            /* Now do the real conversion */
            if (WideCharToMultiByte(CP_ACP,
                                    0,
                                    pbLocalData,
                                    cchValueName + 1,
                                    pszData,
                                    cbMultiByte,
                                    NULL,
                                    NULL) != 0)
            {
                /* It worked, return the data back to the caller */
                *ppszData = pszData;
            }
            else
            {
                /* It failed, free our buffer and get the error code */
                MemFree(pszData);
                dwError = GetLastError();
            }
        }
    }

    /* Free the original Unicode string and return the error */
    MemFree(pbLocalData);
    return dwError;
}

