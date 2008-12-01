/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/mem/isbad.c
 * PURPOSE:         Handles probing of memory addresses
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Thomas Weidenmueller (w3seek@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

extern SYSTEM_BASIC_INFORMATION BaseCachedSysInfo;

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
IsBadReadPtr(IN LPCVOID lp,
             IN UINT_PTR ucb)
{
    ULONG PageSize;
    BOOLEAN Result = FALSE;
    volatile CHAR *Current;
    PCHAR Last;

    /* Quick cases */
    if (!ucb) return FALSE;
    if (!lp) return TRUE;

    /* Get the page size */
    PageSize = BaseCachedSysInfo.PageSize;

    /* Calculate the last page */
    Last = (PCHAR)((ULONG_PTR)lp + ucb - 1);

    /* Another quick failure case */
    if ((ULONG_PTR)Last < (ULONG_PTR)lp) return TRUE;

    /* Enter SEH */
    _SEH2_TRY
    {
        /* Probe the entire range */
        Current = (volatile CHAR*)lp;
        Last = (PCHAR)(PAGE_ROUND_DOWN(Last));
        do
        {
            *Current;
            Current = (volatile CHAR*)(PAGE_ROUND_DOWN(Current) + PAGE_SIZE);
        } while (Current <= Last);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* We hit an exception, so return true */
        Result = TRUE;
    }
    _SEH2_END

    /* Return exception status */
    return Result;
}

/*
 * @implemented
 */
BOOL
NTAPI
IsBadHugeReadPtr(LPCVOID lp,
                 UINT_PTR ucb)
{
    /* Implementation is the same on 32-bit */
    return IsBadReadPtr(lp, ucb);
}

/*
 * @implemented
 */
BOOL
NTAPI
IsBadCodePtr(FARPROC lpfn)
{
    /* Executing has the same privileges as reading */
    return IsBadReadPtr((LPVOID)lpfn, 1);
}

/*
 * @implemented
 */
BOOL
NTAPI
IsBadWritePtr(LPVOID lp,
              UINT_PTR ucb)
{
    ULONG PageSize;
    BOOLEAN Result = FALSE;
    volatile CHAR *Current;
    PCHAR Last;

    /* Quick cases */
    if (!ucb) return FALSE;
    if (!lp) return TRUE;

    /* Get the page size */
    PageSize = BaseCachedSysInfo.PageSize;

    /* Calculate the last page */
    Last = (PCHAR)((ULONG_PTR)lp + ucb - 1);

    /* Another quick failure case */
    if ((ULONG_PTR)Last < (ULONG_PTR)lp) return TRUE;

    /* Enter SEH */
    _SEH2_TRY
    {
        /* Probe the entire range */
        Current = (volatile CHAR*)lp;
        Last = (PCHAR)(PAGE_ROUND_DOWN(Last));
        do
        {
            *Current = *Current;
            Current = (volatile CHAR*)(PAGE_ROUND_DOWN(Current) + PAGE_SIZE);
        } while (Current <= Last);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* We hit an exception, so return true */
        Result = TRUE;
    }
    _SEH2_END

    /* Return exception status */
    return Result;
}

/*
 * @implemented
 */
BOOL
NTAPI
IsBadHugeWritePtr(LPVOID lp,
                  UINT_PTR ucb)
{
    /* Implementation is the same on 32-bit */
    return IsBadWritePtr(lp, ucb);
}

/*
 * @implemented
 */
BOOL
NTAPI
IsBadStringPtrW(IN LPCWSTR lpsz,
                UINT_PTR ucchMax)
{
    BOOLEAN Result = FALSE;
    volatile WCHAR *Current;
    PWCHAR Last;
    WCHAR Char;

    /* Quick cases */
    if (!ucchMax) return FALSE;
    if (!lpsz) return TRUE;

    /* Calculate the last page */
    Last = (PWCHAR)((ULONG_PTR)lpsz + (ucchMax * 2) - 2);

    /* Another quick failure case */
    if ((ULONG_PTR)Last < (ULONG_PTR)lpsz) return TRUE;

    /* Enter SEH */
    _SEH2_TRY
    {
        /* Probe the entire range */
        Current = (volatile WCHAR*)lpsz;
        Last = (PWCHAR)(PAGE_ROUND_DOWN(Last));
        do
        {
            Char = *Current;
            Current++;
        } while (Char && (Current <= Last));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* We hit an exception, so return true */
        Result = TRUE;
    }
    _SEH2_END

    /* Return exception status */
    return Result;
}

/*
 * @implemented
 */
BOOL
NTAPI
IsBadStringPtrA(IN LPCSTR lpsz,
                UINT_PTR ucchMax)
{
    BOOLEAN Result = FALSE;
    volatile CHAR *Current;
    PCHAR Last;
    CHAR Char;

    /* Quick cases */
    if (!ucchMax) return FALSE;
    if (!lpsz) return TRUE;

    /* Calculate the last page */
    Last = (PCHAR)((ULONG_PTR)lpsz + ucchMax - 1);

    /* Another quick failure case */
    if ((ULONG_PTR)Last < (ULONG_PTR)lpsz) return TRUE;

    /* Enter SEH */
    _SEH2_TRY
    {
        /* Probe the entire range */
        Current = (volatile CHAR*)lpsz;
        Last = (PCHAR)(PAGE_ROUND_DOWN(Last));
        do
        {
            Char = *Current;
            Current++;
        } while (Char && (Current <= Last));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* We hit an exception, so return true */
        Result = TRUE;
    }
    _SEH2_END

    /* Return exception status */
    return Result;
}

/* EOF */
