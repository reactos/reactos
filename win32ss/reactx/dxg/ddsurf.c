/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native driver for dxg implementation
 * FILE:             win32ss/reactx/dxg/ddsurf.c
 * PROGRAMER:        Sebastian Gasiorek (sebastian.gasiorek@reactos.org)
 */

#include <dxg_int.h>

/*++
* @name DxDdLock
* @implemented
*
* The function DxDdLock locks the surface and calls
* MapMemory driver function to assign surface memory.
* Surface memory is returned in mapMemoryData.fpProcess variable
*
* @param HANDLE hSurface
* Handle to DirectDraw surface
*
* @param PDD_LOCKDATA puLockData
* Structure with lock details
*
* @param HDC hdcClip
* Reserved
*
* @return
* Returns DDHAL_DRIVER_HANDLED or DDHAL_DRIVER_NOTHANDLED. 
*
* @remarks.
* Missing lock data and error handling.
*--*/
DWORD
NTAPI
DxDdLock(HANDLE hSurface,
         PDD_LOCKDATA puLockData,
         HDC hdcClip)
{
    PEDD_SURFACE pSurface;
    PEDD_DIRECTDRAW_LOCAL peDdL;
    PEDD_DIRECTDRAW_GLOBAL peDdGl;
    DD_MAPMEMORYDATA mapMemoryData;

    pSurface = (PEDD_SURFACE)DdHmgLock(hSurface, ObjType_DDSURFACE_TYPE, TRUE);
    peDdL = pSurface->peDirectDrawLocal;
    peDdGl = peDdL->peDirectDrawGlobal2;

    // Map memory if it's not already mapped and driver function is provided
    if (!peDdL->isMemoryMapped && (peDdGl->ddCallbacks.dwFlags & DDHAL_CB32_MAPMEMORY))
    {
        mapMemoryData.bMap = 1;
        mapMemoryData.hProcess = (HANDLE)-1;
        mapMemoryData.fpProcess = 0;
        mapMemoryData.lpDD = (PDD_DIRECTDRAW_GLOBAL)peDdGl;

        peDdGl->ddCallbacks.MapMemory(&mapMemoryData);

        if (!mapMemoryData.ddRVal)
        {
            peDdL->isMemoryMapped = 1;
            peDdL->fpProcess2 = mapMemoryData.fpProcess;
        }
    }

    if (pSurface)
    {
        InterlockedExchangeAdd((LONG*)&pSurface->pobj.cExclusiveLock, 0xFFFFFFFF);
    }

    puLockData->ddRVal = DD_OK;

    return DDHAL_DRIVER_HANDLED;
}

/*++
* @name DxDdUnlock
* @unimplemented
*
* The function DxDdUnlock releases the lock from specified surface
*
* @param HANDLE hSurface
* Handle to DirectDraw surface
*
* @param PDD_UNLOCKDATA puUnlockData
* Structure with lock details
*
* @return
* Returns DDHAL_DRIVER_HANDLED or DDHAL_DRIVER_NOTHANDLED. 
*
* @remarks.
* Stub
*--*/
DWORD
NTAPI
DxDdUnlock(HANDLE hSurface,
           PDD_UNLOCKDATA puUnlockData)
{
    puUnlockData->ddRVal = DD_OK;

    return DDHAL_DRIVER_HANDLED;
}
