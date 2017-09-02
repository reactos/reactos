/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native driver for dxg implementation
 * FILE:             win32ss/reactx/dxg/d3d.c
 * PROGRAMER:        Sebastian Gasiorek (sebastian.gasiorek@reactos.org)
 */

#include <string.h>
#include <dxg_int.h>

DWORD
NTAPI
DxDdCanCreateD3DBuffer(
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

    if (peDdGl->d3dBufCallbacks.CanCreateD3DBuffer)
        RetVal = peDdGl->d3dBufCallbacks.CanCreateD3DBuffer(SurfaceData);

    gpEngFuncs.DxEngUnlockHdev(peDdGl->hDev);
    InterlockedDecrement((VOID*)&peDdL->pobj.cExclusiveLock);

    return RetVal;
}

DWORD
FASTCALL
intDdCreateSurfaceOrBuffer(HANDLE hDirectDrawLocal, 
                           PEDD_SURFACE pDdSurfList, DDSURFACEDESC2 *a3, 
                           DD_SURFACE_GLOBAL *pDdSurfGlob, 
                           DD_SURFACE_LOCAL *pDdSurfLoc, 
                           DD_SURFACE_MORE *pDdSurfMore, 
                           DD_CREATESURFACEDATA *pDdCreateSurfaceData, 
                           PVOID Address)
{
  PEDD_DIRECTDRAW_LOCAL peDdL = NULL;
  PEDD_DIRECTDRAW_GLOBAL peDdGl = NULL;
  DD_SURFACE_LOCAL *pCurSurfLocal;
  DD_SURFACE_GLOBAL *pCurSurfGlobal;
  DD_SURFACE_MORE *pCurSurfMore;
  PEDD_SURFACE pCurSurf;

  ULONG CurSurf;

  if (!pDdCreateSurfaceData)
      return FALSE;

  if (!pDdCreateSurfaceData->dwSCnt)
  {
      pDdCreateSurfaceData->ddRVal = E_FAIL;
      return FALSE;
  }

  peDdL = (PEDD_DIRECTDRAW_LOCAL)DdHmgLock(hDirectDrawLocal, ObjType_DDLOCAL_TYPE, FALSE);
  if (!peDdL)
      return FALSE;

  peDdGl = peDdL->peDirectDrawGlobal2;

  if (!(pDdSurfLoc->ddsCaps.dwCaps & DDSCAPS_VISIBLE) && !(peDdGl->ddCallbacks.dwFlags & DDHAL_CB32_CREATESURFACE))
  {
      pDdCreateSurfaceData->ddRVal = E_FAIL;
      return FALSE;
  }

  pDdSurfList = (PEDD_SURFACE)EngAllocMem(FL_ZERO_MEMORY, pDdCreateSurfaceData->dwSCnt * sizeof(EDD_SURFACE), TAG_GDDP);
  pDdSurfGlob = (DD_SURFACE_GLOBAL *)EngAllocMem(FL_ZERO_MEMORY, pDdCreateSurfaceData->dwSCnt * sizeof(DD_SURFACE_GLOBAL), TAG_GDDP);
  pDdSurfLoc = (DD_SURFACE_LOCAL *)EngAllocMem(FL_ZERO_MEMORY, pDdCreateSurfaceData->dwSCnt * sizeof(DD_SURFACE_LOCAL), TAG_GDDP);
  pDdSurfMore = (DD_SURFACE_MORE *)EngAllocMem(FL_ZERO_MEMORY, pDdCreateSurfaceData->dwSCnt * sizeof(DD_SURFACE_MORE), TAG_GDDP);

  gpEngFuncs.DxEngLockShareSem();
  gpEngFuncs.DxEngLockHdev(peDdGl->hDev);

  // create all surface objects
  for (CurSurf = 0; CurSurf < pDdCreateSurfaceData->dwSCnt; CurSurf++)
  {
      pCurSurf       = &pDdSurfList[CurSurf];
      pCurSurfLocal  = &pDdSurfLoc[CurSurf];
      pCurSurfGlobal = &pDdSurfGlob[CurSurf];
      pCurSurfMore   = &pDdSurfMore[CurSurf];

      pCurSurf = intDdCreateNewSurfaceObject(
                          peDdL,
                          pCurSurf,
                          pCurSurfGlobal,
                          pCurSurfLocal,
                          pCurSurfMore);
      Address = pCurSurf;
  }

  gpEngFuncs.DxEngUnlockHdev(peDdGl->hDev);
  gpEngFuncs.DxEngUnlockShareSem();

  return DDHAL_DRIVER_HANDLED;
}

DWORD
NTAPI
DxDdCreateD3DBuffer(
    HANDLE hDirectDrawLocal,
    PEDD_SURFACE pDdSurfList,
    DDSURFACEDESC2 *a3,
    DD_SURFACE_GLOBAL *pDdSurfGlob,
    DD_SURFACE_LOCAL *pDdSurfLoc,
    DD_SURFACE_MORE *pDdSurfMore,
    DD_CREATESURFACEDATA *pDdCreateSurfaceData,
    PVOID Address)
{
    return intDdCreateSurfaceOrBuffer(hDirectDrawLocal, pDdSurfList, a3, pDdSurfGlob, pDdSurfLoc, pDdSurfMore, pDdCreateSurfaceData, Address);
}
