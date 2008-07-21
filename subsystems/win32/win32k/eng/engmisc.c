/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engmisc.c
 * PURPOSE:         Miscellaneous Support Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

VOID
APIENTRY
EngSort(IN OUT PBYTE Buffer,
        IN ULONG ElemSize,
        IN ULONG ElemCount,
        IN SORTCOMP CompFunc)
{
    /* Forward to the CRT */
    qsort(Buffer, ElemCount, ElemSize, CompFunc);
}

PVOID
APIENTRY
EngFindImageProcAddress(IN HANDLE hModule,
                        IN PSTR ProcName)
{
    UNIMPLEMENTED;
	return NULL;
}

PVOID
APIENTRY
EngFindResource(IN HANDLE hModule,
                IN INT iName,
                IN INT iType,
                OUT PULONG pulSize)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL
APIENTRY
EngLpkInstalled(VOID)
{
    UNIMPLEMENTED;
	return FALSE;
}

INT
APIENTRY
EngMulDiv(IN INT a,
          IN INT  b,
          IN INT  c)
{
    UNIMPLEMENTED;
	return 0;
}

HANDLE
APIENTRY
EngGetCurrentProcessId(VOID)
{
    UNIMPLEMENTED;
	return NULL;
}

HANDLE
APIENTRY
EngGetCurrentThreadId(VOID)
{
    UNIMPLEMENTED;
	return NULL;
}

HANDLE
APIENTRY
EngGetProcessHandle(VOID)
{
    /* Deprecated */
	return NULL;
}

ULONGLONG
APIENTRY
EngGetTickCount(VOID)
{
    UNIMPLEMENTED;
    return 0;
}
