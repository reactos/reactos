

/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             subsys/win32k/ntddraw/dvd.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       19/1-2006   Magnus Olsen
 */


#include <w32k.h>
#include <debug.h>

FLATPTR
STDCALL
HeapVidMemAllocAligned(
   IN LPVIDMEM lpVidMem,
   IN DWORD dwWidth,
   IN DWORD dwHeight,
   IN LPSURFACEALIGNMENT lpAlignment,
   OUT LPLONG lpNewPitch)
{

}

VOID
STDCALL
VidMemFree(LPVMEMHEAP pvmh,
           FLATPTR ptr)
{

}

PVOID
STDCALL
EngAllocPrivateUserMem(PDD_SURFACE_LOCAL  psl,
                       SIZE_T  cj,
                       ULONG  tag)
{

}

VOID STDCALL
EngFreePrivateUserMem(PDD_SURFACE_LOCAL  psl,
                      PVOID  pv)
{

}

HRESULT
STDCALL
EngDxIoctl(ULONG ulIoctl,
           PVOID pBuffer,
           ULONG ulBufferSize)
{

}

PDD_SURFACE_LOCAL
STDCALL
EngLockDirectDrawSurface(HANDLE hSurface)
{

}

BOOL
STDCALL
EngUnlockDirectDrawSurface(PDD_SURFACE_LOCAL pSurface)
{

}

