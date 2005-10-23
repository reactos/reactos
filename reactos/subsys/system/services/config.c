/*
 * config.c
 */

/* INCLUDES *****************************************************************/

#include "services.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

DWORD
ScmWriteDependencies(HKEY hServiceKey,
                     LPWSTR lpDependencies,
                     DWORD dwDependenciesLength)
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwGroupLength = 0;
    DWORD dwServiceLength = 0;
    DWORD dwLength;
    LPWSTR lpGroupDeps;
    LPWSTR lpServiceDeps;
    LPWSTR lpSrc;
    LPWSTR lpDst;

    if (*lpDependencies == 0)
    {
        RegDeleteValue(hServiceKey,
                       L"DependOnService");
        RegDeleteValue(hServiceKey,
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
            dwLength = wcslen(lpSrc);
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

        dwError = RegSetValueExW(hServiceKey,
                                 L"DependOnGroup",
                                 0,
                                 REG_MULTI_SZ,
                                 (LPBYTE)lpGroupDeps,
                                 dwGroupLength * sizeof(WCHAR));

        if (dwError == ERROR_SUCCESS)
        {
            dwError = RegSetValueExW(hServiceKey,
                                     L"DependOnService",
                                     0,
                                     REG_MULTI_SZ,
                                     (LPBYTE)lpServiceDeps,
                                     dwServiceLength * sizeof(WCHAR));
        }

        HeapFree(GetProcessHeap(), 0, lpGroupDeps);
    }

    return dwError;
}

/* EOF */

