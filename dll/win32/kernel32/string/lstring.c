/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/lstring.c
 * PURPOSE:         Local string functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <k32.h>

/*
 * @implemented
 */
int
STDCALL
lstrcmpA(LPCSTR lpString1, LPCSTR lpString2)
{
    int Result;

    if (lpString1 == lpString2)
        return 0;
    if (lpString1 == NULL)
        return -1;
    if (lpString2 == NULL)
        return 1;

    Result = CompareStringA(GetThreadLocale(), 0, lpString1, -1, lpString2, -1);
    if (Result) Result -= 2;

    return Result;
}


/*
 * @implemented
 */
int
STDCALL
lstrcmpiA(LPCSTR lpString1, LPCSTR lpString2)
{
    int Result;

    if (lpString1 == lpString2)
        return 0;
    if (lpString1 == NULL)
        return -1;
    if (lpString2 == NULL)
        return 1;

    Result = CompareStringA(GetThreadLocale(), NORM_IGNORECASE, lpString1, -1, lpString2, -1);
    if (Result)
        Result -= 2;

    return Result;
}

/*
 * @implemented
 */
LPSTR
STDCALL
lstrcpynA(LPSTR lpString1, LPCSTR lpString2, int iMaxLength)
{
    LPSTR d = lpString1;
    LPCSTR s = lpString2;
    UINT count = iMaxLength;
    LPSTR Ret = NULL;

    _SEH_TRY
    {
        while ((count > 1) && *s)
        {
            count--;
            *d++ = *s++;
        }

        if (count)
            *d = 0;

        Ret = lpString1;
    }
    _SEH_HANDLE
    _SEH_END;

    return Ret;
}


/*
 * @implemented
 */
LPSTR
STDCALL
lstrcpyA(LPSTR lpString1, LPCSTR lpString2)
{
    LPSTR Ret = NULL;

    _SEH_TRY
    {
        memmove(lpString1, lpString2, strlen(lpString2) + 1);
        Ret = lpString1;
     }
    _SEH_HANDLE
    _SEH_END;

    return Ret;
}


/*
 * @implemented
 */
LPSTR
STDCALL
lstrcatA(LPSTR lpString1, LPCSTR lpString2)
{
    LPSTR Ret = NULL;

    _SEH_TRY
    {
        Ret = strcat(lpString1, lpString2);
    }
    _SEH_HANDLE
    _SEH_END;

    return Ret;
}


/*
 * @implemented
 */
int
STDCALL
lstrlenA(LPCSTR lpString)
{
    INT Ret = 0;

    _SEH_TRY
    {
        Ret = strlen(lpString);
    }
    _SEH_HANDLE
    _SEH_END;

    return Ret;
}


/*
 * @implemented
 */
int
STDCALL
lstrcmpW(LPCWSTR lpString1, LPCWSTR lpString2)
{
    int Result;

    if (lpString1 == lpString2)
        return 0;
    if (lpString1 == NULL)
        return -1;
    if (lpString2 == NULL)
        return 1;

    Result = CompareStringW(GetThreadLocale(), 0, lpString1, -1, lpString2, -1);
    if (Result)
        Result -= 2;

    return Result;
}


/*
 * @implemented
 */
int
STDCALL
lstrcmpiW(LPCWSTR lpString1, LPCWSTR lpString2)
{
    int Result;

    if (lpString1 == lpString2)
        return 0;
    if (lpString1 == NULL)
        return -1;
    if (lpString2 == NULL)
        return 1;

    Result = CompareStringW(GetThreadLocale(), NORM_IGNORECASE, lpString1, -1, lpString2, -1);
    if (Result)
        Result -= 2;

    return Result;
}


/*
 * @implemented
 */
LPWSTR
STDCALL
lstrcpynW(LPWSTR lpString1, LPCWSTR lpString2, int iMaxLength)
{
    LPWSTR d = lpString1;
    LPCWSTR s = lpString2;
    UINT count = iMaxLength;
    LPWSTR Ret = NULL;

    _SEH_TRY
    {
        while ((count > 1) && *s)
        {
            count--;
            *d++ = *s++;
        }

        if (count)
            *d = 0;

        Ret = lpString1;
    }
    _SEH_HANDLE
    _SEH_END;

    return Ret;
}


/*
 * @implemented
 */
LPWSTR
STDCALL
lstrcpyW(LPWSTR lpString1, LPCWSTR lpString2)
{
    LPWSTR Ret = NULL;

    _SEH_TRY
    {
        Ret = wcscpy(lpString1, lpString2);
    }
    _SEH_HANDLE
    _SEH_END;

    return Ret;
}


/*
 * @implemented
 */
LPWSTR
STDCALL
lstrcatW(LPWSTR lpString1, LPCWSTR lpString2)
{
    LPWSTR Ret = NULL;

    _SEH_TRY
    {
        Ret = wcscat(lpString1, lpString2);
    }
    _SEH_HANDLE
    _SEH_END;

    return Ret;
}


/*
 * @implemented
 */
int
STDCALL
lstrlenW(LPCWSTR lpString)
{
    INT Ret = 0;

    _SEH_TRY
    {
        Ret = wcslen(lpString);
    }
    _SEH_HANDLE
    _SEH_END;

    return Ret;
}
