/* $Id: stubs.c 28533 2007-08-24 22:44:36Z greatlrd $
 *
 * reactos/lib/gdi32/misc/eng.c
 *
 * GDI32.DLL eng part
 *
 *
 */

#include "precomp.h"

/*
 * @implemented
 */
VOID
STDCALL
EngAcquireSemaphore ( IN HSEMAPHORE hsem )
{
    RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)hsem);
}

/*
 * @implemented
 */
HSEMAPHORE
STDCALL
EngCreateSemaphore ( VOID )
{
    PRTL_CRITICAL_SECTION CritSect = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(RTL_CRITICAL_SECTION));
    if (!CritSect)
    {
        return NULL;
    }

    RtlInitializeCriticalSection( CritSect );
    return (HSEMAPHORE)CritSect;
}

/*
 * @implemented
 */
VOID
STDCALL
EngDeleteSemaphore ( IN HSEMAPHORE hsem )
{
 if (!hsem) return;

 RtlDeleteCriticalSection( (PRTL_CRITICAL_SECTION) hsem );
 RtlFreeHeap( GetProcessHeap(), 0, hsem );
}

/*
 * @implemented
 */
PVOID STDCALL
EngFindResource(HANDLE h,
                int iName,
                int iType,
                PULONG pulSize)
{
    HRSRC HRSrc;
    DWORD Size = 0;
    HGLOBAL Hg;
    LPVOID Lock = NULL;

    if ((HRSrc = FindResourceW( (HMODULE) h, MAKEINTRESOURCEW(iName), MAKEINTRESOURCEW(iType))))
    {
        if ((Size = SizeofResource( (HMODULE) h, HRSrc )))
        {
            if ((Hg = LoadResource( (HMODULE) h, HRSrc )))
            {
                Lock = LockResource( Hg );
            }
        }
    }

    *pulSize = Size;
    return (PVOID) Lock;
}

