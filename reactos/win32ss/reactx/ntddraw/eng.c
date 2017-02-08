/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             win32ss/reactx/ntddraw/eng.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       19/1-2006   Magnus Olsen
 */


#include <win32k.h>
#include <debug.h>

/************************************************************************/
/* HeapVidMemAllocAligned                                               */
/************************************************************************/
FLATPTR
APIENTRY
HeapVidMemAllocAligned(LPVIDMEM lpVidMem,
                       DWORD dwWidth,
                       DWORD dwHeight,
                       LPSURFACEALIGNMENT lpAlignment,
                       LPLONG lpNewPitch)
{
    PGD_HEAPVIDMEMALLOCALIGNED pfnHeapVidMemAllocAligned = (PGD_HEAPVIDMEMALLOCALIGNED)gpDxFuncs[DXG_INDEX_DxDdHeapVidMemAllocAligned].pfn;

    if (pfnHeapVidMemAllocAligned == NULL)
    {
        DPRINT1("Warning: no pfnHeapVidMemAllocAligned\n");
        return 0;
    }

    DPRINT1("Calling dxg.sys pfnHeapVidMemAllocAligned\n");
    return pfnHeapVidMemAllocAligned(lpVidMem, dwWidth, dwHeight, lpAlignment, lpNewPitch);
}

/************************************************************************/
/* VidMemFree                                                           */
/************************************************************************/
VOID
APIENTRY
VidMemFree(LPVMEMHEAP pvmh,
           FLATPTR ptr)
{
    PGD_VIDMEMFREE pfnVidMemFree = (PGD_VIDMEMFREE)gpDxFuncs[DXG_INDEX_DxDdHeapVidMemFree].pfn;

    if (pfnVidMemFree == NULL)
    {
        DPRINT1("Warning: no pfnVidMemFree\n");
    }
    else
    {
        DPRINT1("Calling dxg.sys pfnVidMemFree\n");
        pfnVidMemFree(pvmh, ptr);
    }
}

/************************************************************************/
/* EngAllocPrivateUserMem                                               */
/************************************************************************/
_Must_inspect_result_
_Ret_opt_bytecount_(cjMemSize)
__drv_allocatesMem(PrivateUserMem)
ENGAPI
PVOID
APIENTRY
EngAllocPrivateUserMem(
    _In_ PDD_SURFACE_LOCAL psl,
    _In_ SIZE_T cjMemSize,
    _In_ ULONG ulTag)
{
    PGD_ENGALLOCPRIVATEUSERMEM pfnEngAllocPrivateUserMem = (PGD_ENGALLOCPRIVATEUSERMEM)gpDxFuncs[DXG_INDEX_DxDdAllocPrivateUserMem].pfn;

    if (pfnEngAllocPrivateUserMem == NULL)
    {
        DPRINT1("Warning: no pfnEngAllocPrivateUserMem\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnEngAllocPrivateUserMem\n");
    return pfnEngAllocPrivateUserMem(psl, cjMemSize, ulTag);
}

/************************************************************************/
/* EngFreePrivateUserMem                                                */
/************************************************************************/
VOID
APIENTRY
EngFreePrivateUserMem(PDD_SURFACE_LOCAL  psl,
                      PVOID  pv)
{
    PGD_ENGFREEPRIVATEUSERMEM pfnEngFreePrivateUserMem = (PGD_ENGFREEPRIVATEUSERMEM)gpDxFuncs[DXG_INDEX_DxDdFreePrivateUserMem].pfn;

    if (pfnEngFreePrivateUserMem == NULL)
    {
        DPRINT1("Warning: no pfnEngFreePrivateUserMem\n");
    }
    else
    {
        DPRINT1("Calling dxg.sys pfnEngFreePrivateUserMem\n");
        pfnEngFreePrivateUserMem(psl, pv);
    }
}

/*++
* @name EngDxIoctl
* @implemented
*
* The function EngDxIoctl is the ioctl call to different DirectX functions
* in the driver dxg.sys
*
* @param ULONG ulIoctl
* The ioctl code that we want call to
*
* @param PVOID pBuffer
* Our in or out buffer with data to the ioctl code we are using
*
* @param ULONG ulBufferSize
* The buffer size in bytes
*
* @return
* Always returns DDERR_UNSUPPORTED
*
* @remarks.
* dxg.sys EngDxIoctl call is redirected to dxg.sys
* This function is no longer used in Windows NT 2000/XP/2003
*
*--*/
HRESULT
APIENTRY
EngDxIoctl(ULONG ulIoctl,
           PVOID pBuffer,
           ULONG ulBufferSize)
{
    PGD_ENGDXIOCTL pfnEngDxIoctl = (PGD_ENGDXIOCTL)gpDxFuncs[DXG_INDEX_DxDdIoctl].pfn;
    DWORD retVal = DDERR_UNSUPPORTED;

    DPRINT1("Calling dxg.sys pfnEngDxIoctl\n");

    if (pfnEngDxIoctl != NULL)
    {
        retVal = pfnEngDxIoctl(ulIoctl, pBuffer, ulBufferSize);
    }

    return retVal;
}

/*++
* @name EngLockDirectDrawSurface
* @implemented
*
* The function EngUnlockDirectDrawSurface locks the DirectX surface.

* @param HANDLE hSurface
* The handle of a surface
*
* @return
* This return a vaild or NULL pointer to a PDD_SURFACE_LOCAL object
*
* @remarks.
* None
*
*--*/
PDD_SURFACE_LOCAL
APIENTRY
EngLockDirectDrawSurface(HANDLE hSurface)
{
    PGD_ENGLOCKDIRECTDRAWSURFACE pfnEngLockDirectDrawSurface = (PGD_ENGLOCKDIRECTDRAWSURFACE)gpDxFuncs[DXG_INDEX_DxDdLockDirectDrawSurface].pfn;
    PDD_SURFACE_LOCAL retVal = NULL;

    DPRINT1("Calling dxg.sys pfnEngLockDirectDrawSurface\n");

    if (pfnEngLockDirectDrawSurface != NULL)
    {
       retVal = pfnEngLockDirectDrawSurface(hSurface);
    }

    return retVal;
}


/*++
* @name EngUnlockDirectDrawSurface
* @implemented
*
* The function EngUnlockDirectDrawSurface unlocks the DirectX surface

* @param PDD_SURFACE_LOCAL pSurface
* The Surface we want to unlock
*
* @return
* This return TRUE for success, FALSE for failure
*
* @remarks.
* None
*
*--*/
BOOL
APIENTRY
EngUnlockDirectDrawSurface(PDD_SURFACE_LOCAL pSurface)
{
    PGD_ENGUNLOCKDIRECTDRAWSURFACE pfnEngUnlockDirectDrawSurface = (PGD_ENGUNLOCKDIRECTDRAWSURFACE)gpDxFuncs[DXG_INDEX_DxDdUnlockDirectDrawSurface].pfn;
    BOOL retVal = FALSE;

    DPRINT1("Calling dxg.sys pfnEngUnlockDirectDrawSurface\n");

    if (pfnEngUnlockDirectDrawSurface != NULL)
    {
        retVal = pfnEngUnlockDirectDrawSurface(pSurface);
    }

    return retVal;
}

