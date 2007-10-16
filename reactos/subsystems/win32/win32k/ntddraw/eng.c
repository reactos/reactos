

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
HeapVidMemAllocAligned(LPVIDMEM lpVidMem,
                       DWORD dwWidth,
                       DWORD dwHeight,
                       LPSURFACEALIGNMENT lpAlignment,
                       LPLONG lpNewPitch)
{
    pfnHeapVidMemAllocAligned = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(, pfnHeapVidMemAllocAligned);

    if (pfnHeapVidMemAllocAligned == NULL)
    {
        DPRINT1("Warring no pfnHeapVidMemAllocAligned");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnHeapVidMemAllocAligned");
    return pfnHeapVidMemAllocAligned(lpVidMem, dwWidth, dwHeight, lpAlignment, lpNewPitch);
}

VOID
STDCALL
VidMemFree(LPVMEMHEAP pvmh,
           FLATPTR ptr)
{
    pfnVidMemFree = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(, pfnVidMemFree);

    if (pfnVidMemFree == NULL)
    {
        DPRINT1("Warring no pfnVidMemFree");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnVidMemFree");
    return pfnVidMemFree(pvmh, ptr);
}

PVOID
STDCALL
EngAllocPrivateUserMem(PDD_SURFACE_LOCAL  psl,
                       SIZE_T  cj,
                       ULONG  tag)
{
    pfnEngAllocPrivateUserMem = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(, pfnEngAllocPrivateUserMem);

    if (pfnEngAllocPrivateUserMem == NULL)
    {
        DPRINT1("Warring no pfnEngAllocPrivateUserMem");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnEngAllocPrivateUserMem");
    return pfnEngAllocPrivateUserMem(psl, cj, tag);
}

VOID
STDCALL
EngFreePrivateUserMem(PDD_SURFACE_LOCAL  psl,
                      PVOID  pv)
{
    pfnEngFreePrivateUserMem = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(, pfnEngFreePrivateUserMem);

    if (pfnEngFreePrivateUserMem == NULL)
    {
        DPRINT1("Warring no pfnEngFreePrivateUserMem");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnEngFreePrivateUserMem");
    return pfnEngFreePrivateUserMem(psl, pv);
}

DWORD
STDCALL
EngDxIoctl(ULONG ulIoctl,
           PVOID pBuffer,
           ULONG ulBufferSize)
{
    pfnEngFreePrivateUserMem = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(, pfnEngFreePrivateUserMem);

    if (pfnEngFreePrivateUserMem == NULL)
    {
        DPRINT1("Warring no pfnEngFreePrivateUserMem");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnEngFreePrivateUserMem");
    return pfnEngFreePrivateUserMem(psl, pv);
}

PDD_SURFACE_LOCAL
STDCALL
EngLockDirectDrawSurface(HANDLE hSurface)
{
    pfnEngLockDirectDrawSurface = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(, pfnEngLockDirectDrawSurface);

    if (pfnEngLockDirectDrawSurface == NULL)
    {
        DPRINT1("Warring no pfnEngLockDirectDrawSurface");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnEngLockDirectDrawSurface");
    return pfnEngLockDirectDrawSurface(hSurface);
}

BOOL
STDCALL
EngUnlockDirectDrawSurface(PDD_SURFACE_LOCAL pSurface)
{
    pfnEngUnlockDirectDrawSurface = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(, pfnEngUnlockDirectDrawSurface);

    if (pfnEngUnlockDirectDrawSurface == NULL)
    {
        DPRINT1("Warring no pfnEngUnlockDirectDrawSurface");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnEngUnlockDirectDrawSurface");
    return pfnEngUnlockDirectDrawSurface(pSurface);
}

