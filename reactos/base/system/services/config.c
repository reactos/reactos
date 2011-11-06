/*
 * PROJECT:     ReactOS Service Control Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/services/config.c
 * PURPOSE:     Service configuration interface
 * COPYRIGHT:   Copyright 2005 Eric Kohl
 *
 */

/* INCLUDES *****************************************************************/

#include "services.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/


DWORD
ScmOpenServiceKey(LPWSTR lpServiceName,
                  REGSAM samDesired,
                  PHKEY phKey)
{
    HKEY hServicesKey = NULL;
    DWORD dwError;

    *phKey = NULL;

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Services",
                            0,
                            KEY_READ,
                            &hServicesKey);
    if (dwError != ERROR_SUCCESS)
        return dwError;

    dwError = RegOpenKeyExW(hServicesKey,
                            lpServiceName,
                            0,
                            samDesired,
                            phKey);

    RegCloseKey(hServicesKey);

    return dwError;
}


DWORD
ScmCreateServiceKey(LPCWSTR lpServiceName,
                    REGSAM samDesired,
                    PHKEY phKey)
{
    HKEY hServicesKey = NULL;
    DWORD dwDisposition;
    DWORD dwError;

    *phKey = NULL;

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Services",
                            0,
                            KEY_READ | KEY_CREATE_SUB_KEY,
                            &hServicesKey);
    if (dwError != ERROR_SUCCESS)
        return dwError;

    dwError = RegCreateKeyExW(hServicesKey,
                              lpServiceName,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              samDesired,
                              NULL,
                              phKey,
                              &dwDisposition);
#if 0
    if ((dwError == ERROR_SUCCESS) &&
        (dwDisposition == REG_OPENED_EXISTING_KEY))
    {
        RegCloseKey(*phKey);
        *phKey = NULL;
        dwError = ERROR_SERVICE_EXISTS;
    }
#endif

    RegCloseKey(hServicesKey);

    return dwError;
}



DWORD
ScmWriteDependencies(HKEY hServiceKey,
                     LPCWSTR lpDependencies,
                     DWORD dwDependenciesLength)
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwGroupLength = 0;
    DWORD dwServiceLength = 0;
    DWORD dwLength;
    LPWSTR lpGroupDeps;
    LPWSTR lpServiceDeps;
    LPCWSTR lpSrc;
    LPWSTR lpDst;

    if (*lpDependencies == 0)
    {
        RegDeleteValueW(hServiceKey,
                       L"DependOnService");
        RegDeleteValueW(hServiceKey,
                       L"DependOnGroup");
    }
    else
    {
        lpGroupDeps = HeapAlloc(GetProcessHeap(),
                                HEAP_ZERO_MEMORY,
                                (dwDependenciesLength + 2) * sizeof(WCHAR));
        if (lpGroupDeps == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;

        lpSrc = lpDependencies;
        lpDst = lpGroupDeps;
        while (*lpSrc != 0)
        {
            dwLength = wcslen(lpSrc) + 1;
            if (*lpSrc == SC_GROUP_IDENTIFIERW)
            {
                lpSrc++;
                dwGroupLength += dwLength;
                wcscpy(lpDst, lpSrc);
                lpDst = lpDst + dwLength;
            }

            lpSrc = lpSrc + dwLength;
        }
        *lpDst = 0;
        lpDst++;
        dwGroupLength++;

        lpSrc = lpDependencies;
        lpServiceDeps = lpDst;
        while (*lpSrc != 0)
        {
            dwLength = wcslen(lpSrc) + 1;
            if (*lpSrc != SC_GROUP_IDENTIFIERW)
            {
                dwServiceLength += dwLength;
                wcscpy(lpDst, lpSrc);
                lpDst = lpDst + dwLength;
            }

            lpSrc = lpSrc + dwLength;
        }
        *lpDst = 0;
        dwServiceLength++;

        if (dwGroupLength > 1)
        {
            dwError = RegSetValueExW(hServiceKey,
                                     L"DependOnGroup",
                                     0,
                                     REG_MULTI_SZ,
                                     (LPBYTE)lpGroupDeps,
                                     dwGroupLength * sizeof(WCHAR));
        }
        else
        {
            RegDeleteValueW(hServiceKey,
                            L"DependOnGroup");
        }

        if (dwError == ERROR_SUCCESS)
        {
            if (dwServiceLength > 1)
            {
                dwError = RegSetValueExW(hServiceKey,
                                         L"DependOnService",
                                         0,
                                         REG_MULTI_SZ,
                                         (LPBYTE)lpServiceDeps,
                                         dwServiceLength * sizeof(WCHAR));
            }
            else
            {
                RegDeleteValueW(hServiceKey,
                                L"DependOnService");
            }
        }

        HeapFree(GetProcessHeap(), 0, lpGroupDeps);
    }

    return dwError;
}


DWORD
ScmMarkServiceForDelete(PSERVICE pService)
{
    HKEY hServiceKey = NULL;
    DWORD dwValue = 1;
    DWORD dwError;

    DPRINT("ScmMarkServiceForDelete() called\n");

    dwError = ScmOpenServiceKey(pService->lpServiceName,
                                KEY_WRITE,
                                &hServiceKey);
    if (dwError != ERROR_SUCCESS)
        return dwError;

    dwError = RegSetValueExW(hServiceKey,
                             L"DeleteFlag",
                             0,
                             REG_DWORD,
                             (LPBYTE)&dwValue,
                             sizeof(DWORD));

    RegCloseKey(hServiceKey);

    return dwError;
}


BOOL
ScmIsDeleteFlagSet(HKEY hServiceKey)
{
    DWORD dwError;
    DWORD dwType;
    DWORD dwFlag;
    DWORD dwSize = sizeof(DWORD);

    dwError = RegQueryValueExW(hServiceKey,
                               L"DeleteFlag",
                               0,
                               &dwType,
                               (LPBYTE)&dwFlag,
                               &dwSize);

    return (dwError == ERROR_SUCCESS);
}


DWORD
ScmReadString(HKEY hServiceKey,
              LPWSTR lpValueName,
              LPWSTR *lpValue)
{
    DWORD dwError;
    DWORD dwSize;
    DWORD dwType;
    DWORD dwSizeNeeded;
    LPWSTR expanded = NULL;
    LPWSTR ptr = NULL;

    *lpValue = NULL;

    dwSize = 0;
    dwError = RegQueryValueExW(hServiceKey,
                               lpValueName,
                               0,
                               &dwType,
                               NULL,
                               &dwSize);
    if (dwError != ERROR_SUCCESS)
        return dwError;

    ptr = HeapAlloc(GetProcessHeap(), 0, dwSize);
    if (ptr == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    dwError = RegQueryValueExW(hServiceKey,
                               lpValueName,
                               0,
                               &dwType,
                               (LPBYTE)ptr,
                               &dwSize);
    if (dwError != ERROR_SUCCESS)
        goto done;

    if (dwType == REG_EXPAND_SZ)
    {
        /* Expand the value... */
        dwSizeNeeded = ExpandEnvironmentStringsW((LPCWSTR)ptr, NULL, 0);
        if (dwSizeNeeded == 0)
        {
            dwError = GetLastError();
            goto done;
        }
        expanded = HeapAlloc(GetProcessHeap(), 0, dwSizeNeeded * sizeof(WCHAR));
        if (dwSizeNeeded < ExpandEnvironmentStringsW((LPCWSTR)ptr, expanded, dwSizeNeeded))
        {
            dwError = GetLastError();
            goto done;
        }
        *lpValue = expanded;
        HeapFree(GetProcessHeap(), 0, ptr);
        dwError = ERROR_SUCCESS;
    }
    else
    {
        *lpValue = ptr;
    }

done:
    if (dwError != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, ptr);
        if (expanded)
            HeapFree(GetProcessHeap(), 0, expanded);
    }

    return dwError;
}


DWORD
ScmReadDependencies(HKEY hServiceKey,
                    LPWSTR *lpDependencies,
                    DWORD *lpdwDependenciesLength)
{
    LPWSTR lpGroups = NULL;
    LPWSTR lpServices = NULL;
    DWORD dwGroupsLength = 0;
    DWORD dwServicesLength = 0;
    LPWSTR lpSrc;
    LPWSTR lpDest;
    DWORD len;
    DWORD dwTotalLength;

    *lpDependencies = NULL;
    *lpdwDependenciesLength = 0;

    /* Read the dependency values */
    ScmReadString(hServiceKey,
                  L"DependOnGroup",
                  &lpGroups);

    ScmReadString(hServiceKey,
                  L"DependOnService",
                  &lpServices);

    /* Leave, if there are no dependencies */
    if (lpGroups == NULL && lpServices == NULL)
        return ERROR_SUCCESS;

    /* Determine the total buffer size for the dependencies */
    if (lpGroups)
    {
        DPRINT("Groups:\n");
        lpSrc = lpGroups;
        while (*lpSrc != 0)
        {
            DPRINT("  %S\n", lpSrc);

            len = wcslen(lpSrc) + 1;
            dwGroupsLength += len + 1;

            lpSrc = lpSrc + len;
        }
    }

    if (lpServices)
    {
        DPRINT("Services:\n");
        lpSrc = lpServices;
        while (*lpSrc != 0)
        {
            DPRINT("  %S\n", lpSrc);

            len = wcslen(lpSrc) + 1;
            dwServicesLength += len;

            lpSrc = lpSrc + len;
        }
    }

    dwTotalLength = dwGroupsLength + dwServicesLength + 1;
    DPRINT("dwTotalLength: %lu\n", dwTotalLength);

    /* Allocate the common buffer for the dependencies */
    *lpDependencies = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwTotalLength * sizeof(WCHAR));
    if (*lpDependencies == NULL)
    {
        if (lpGroups)
            HeapFree(GetProcessHeap(), 0, lpGroups);

        if (lpServices)
            HeapFree(GetProcessHeap(), 0, lpServices);

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Return the allocated buffer length in characters */
    *lpdwDependenciesLength = dwTotalLength;

    /* Copy the service dependencies into the common buffer */
    lpDest = *lpDependencies;
    if (lpServices)
    {
        memcpy(lpDest,
               lpServices,
               dwServicesLength * sizeof(WCHAR));

        lpDest = lpDest + dwServicesLength;
    }

    /* Copy the group dependencies into the common buffer */
    if (lpGroups)
    {
        lpSrc = lpGroups;
        while (*lpSrc != 0)
        {
            len = wcslen(lpSrc) + 1;

            *lpDest = SC_GROUP_IDENTIFIERW;
            lpDest++;

            wcscpy(lpDest, lpSrc);

            lpDest = lpDest + len;
            lpSrc = lpSrc + len;
        }
    }

    /* Free the temporary buffers */
    if (lpGroups)
        HeapFree(GetProcessHeap(), 0, lpGroups);

    if (lpServices)
        HeapFree(GetProcessHeap(), 0, lpServices);

    return ERROR_SUCCESS;
}

/* EOF */
