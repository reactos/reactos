/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntddraw/ddeng.c
 * PURPOSE:         Support Routines for DirectDraw
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

FLATPTR
APIENTRY
HeapVidMemAllocAligned(LPVIDMEM lpVidMem,
                       DWORD dwWidth,
                       DWORD dwHeight,
                       LPSURFACEALIGNMENT lpAlignment,
                       LPLONG lpNewPitch)
{
    UNIMPLEMENTED;
	return 0;
}

VOID
APIENTRY
VidMemFree(LPVMEMHEAP pvmh,
           FLATPTR ptr)
{
    UNIMPLEMENTED;
}

PVOID
APIENTRY
EngAllocPrivateUserMem(PDD_SURFACE_LOCAL  psl,
                       SIZE_T  cj,
                       ULONG  tag)
{
    UNIMPLEMENTED;
	return NULL;
}

VOID
APIENTRY
EngFreePrivateUserMem(PDD_SURFACE_LOCAL  psl,
                      PVOID  pv)
{
    UNIMPLEMENTED;
}

DWORD
APIENTRY
EngDxIoctl(ULONG ulIoctl,
           PVOID pBuffer,
           ULONG ulBufferSize)
{
    UNIMPLEMENTED;
	return 0;
}

PDD_SURFACE_LOCAL
APIENTRY
EngLockDirectDrawSurface(HANDLE hSurface)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL
APIENTRY
EngUnlockDirectDrawSurface(PDD_SURFACE_LOCAL pSurface)
{
    UNIMPLEMENTED;
	return FALSE;
}
