

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

/************************************************************************/
/* HeapVidMemAllocAligned                                               */
/************************************************************************/
FLATPTR
STDCALL
HeapVidMemAllocAligned(LPVIDMEM lpVidMem,
                       DWORD dwWidth,
                       DWORD dwHeight,
                       LPSURFACEALIGNMENT lpAlignment,
                       LPLONG lpNewPitch)
{
    PGD_HEAPVIDMEMALLOCALIGNED pfnHeapVidMemAllocAligned = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdHeapVidMemAllocAligned, pfnHeapVidMemAllocAligned);

    if (pfnHeapVidMemAllocAligned == NULL)
    {
        DPRINT1("Warring no pfnHeapVidMemAllocAligned");
        return 0;
    }

    DPRINT1("Calling on dxg.sys pfnHeapVidMemAllocAligned");
    return pfnHeapVidMemAllocAligned(lpVidMem, dwWidth, dwHeight, lpAlignment, lpNewPitch);
}

/************************************************************************/
/* VidMemFree                                                           */
/************************************************************************/
VOID
STDCALL
VidMemFree(LPVMEMHEAP pvmh,
           FLATPTR ptr)
{
    PGD_VIDMEMFREE pfnVidMemFree = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdHeapVidMemFree, pfnVidMemFree);

    if (pfnVidMemFree == NULL)
    {
        DPRINT1("Warring no pfnVidMemFree");
    }
    else
    {
        DPRINT1("Calling on dxg.sys pfnVidMemFree");
        pfnVidMemFree(pvmh, ptr);
    }
}

/************************************************************************/
/* EngAllocPrivateUserMem                                               */
/************************************************************************/
PVOID
STDCALL
EngAllocPrivateUserMem(PDD_SURFACE_LOCAL  psl,
                       SIZE_T  cj,
                       ULONG  tag)
{
    PGD_ENGALLOCPRIVATEUSERMEM pfnEngAllocPrivateUserMem = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdAllocPrivateUserMem, pfnEngAllocPrivateUserMem);

    if (pfnEngAllocPrivateUserMem == NULL)
    {
        DPRINT1("Warring no pfnEngAllocPrivateUserMem");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnEngAllocPrivateUserMem");
    return pfnEngAllocPrivateUserMem(psl, cj, tag);
}

/************************************************************************/
/* EngFreePrivateUserMem                                                */
/************************************************************************/
VOID
STDCALL
EngFreePrivateUserMem(PDD_SURFACE_LOCAL  psl,
                      PVOID  pv)
{
    PGD_ENGFREEPRIVATEUSERMEM pfnEngFreePrivateUserMem = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdFreePrivateUserMem, pfnEngFreePrivateUserMem);

    if (pfnEngFreePrivateUserMem == NULL)
    {
        DPRINT1("Warring no pfnEngFreePrivateUserMem");
    }
    else
    {
        DPRINT1("Calling on dxg.sys pfnEngFreePrivateUserMem");
        pfnEngFreePrivateUserMem(psl, pv);
    }
}

/************************************************************************/
/* EngDxIoctl                                                           */
/************************************************************************/
DWORD
STDCALL
EngDxIoctl(ULONG ulIoctl,
           PVOID pBuffer,
           ULONG ulBufferSize)
{
    PGD_ENGDXIOCTL pfnEngDxIoctl = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdIoctl, pfnEngDxIoctl);

    if (pfnEngDxIoctl == NULL)
    {
        DPRINT1("Warring no pfnEngDxIoctl");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnEngDxIoctl");
    return pfnEngDxIoctl(ulIoctl, pBuffer, ulBufferSize);
}

/************************************************************************/
/* EngLockDirectDrawSurface                                             */
/************************************************************************/
PDD_SURFACE_LOCAL
STDCALL
EngLockDirectDrawSurface(HANDLE hSurface)
{
    PGD_ENGLOCKDIRECTDRAWSURFACE pfnEngLockDirectDrawSurface = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdLockDirectDrawSurface, pfnEngLockDirectDrawSurface);

    if (pfnEngLockDirectDrawSurface == NULL)
    {
        DPRINT1("Warring no pfnEngLockDirectDrawSurface");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnEngLockDirectDrawSurface");
    return pfnEngLockDirectDrawSurface(hSurface);
}

/************************************************************************/
/* EngUnlockDirectDrawSurface                                           */
/************************************************************************/
BOOL
STDCALL
EngUnlockDirectDrawSurface(PDD_SURFACE_LOCAL pSurface)
{
    PGD_ENGUNLOCKDIRECTDRAWSURFACE pfnEngUnlockDirectDrawSurface = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdUnlockDirectDrawSurface, pfnEngUnlockDirectDrawSurface);

    if (pfnEngUnlockDirectDrawSurface == NULL)
    {
        DPRINT1("Warring no pfnEngUnlockDirectDrawSurface");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnEngUnlockDirectDrawSurface");
    return pfnEngUnlockDirectDrawSurface(pSurface);
}

