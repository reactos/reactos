/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native driver for dxg implementation
 * FILE:             win32ss/reactx/dxg/dd.c
 * PROGRAMER:        Sebastian Gasiorek (sebastian.gasiorek@reactos.org)
 */

#include <dxg_int.h>

DWORD
NTAPI
DxDdCanCreateSurface(
    HANDLE DdHandle,
    PDD_CANCREATESURFACEDATA SurfaceData)
{
    PEDD_DIRECTDRAW_LOCAL peDdL;
    PEDD_DIRECTDRAW_GLOBAL peDdGl;
    DWORD RetVal = DDHAL_DRIVER_NOTHANDLED;

    peDdL = (PEDD_DIRECTDRAW_LOCAL)DdHmgLock(DdHandle, ObjType_DDLOCAL_TYPE, FALSE);
    if (!peDdL)
        return RetVal;

    peDdGl = peDdL->peDirectDrawGlobal2;
    gpEngFuncs.DxEngLockHdev(peDdGl->hDev);

    // assign out DirectDrawGlobal to SurfaceData
    SurfaceData->lpDD = (PDD_DIRECTDRAW_GLOBAL)peDdGl;

    if (peDdGl->ddCallbacks.dwFlags && DDHAL_CB32_CANCREATESURFACE)
    {
        RetVal = peDdGl->ddCallbacks.CanCreateSurface(SurfaceData);
    }

    gpEngFuncs.DxEngUnlockHdev(peDdGl->hDev);
    InterlockedDecrement((VOID*)&peDdL->pobj.cExclusiveLock);

    return RetVal;
}
