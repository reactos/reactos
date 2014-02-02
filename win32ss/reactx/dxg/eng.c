/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native driver for dxg implementation
 * FILE:             drivers/directx/dxg/main.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       30/12-2007   Magnus Olsen
 */

#include <dxg_int.h>

PDD_SURFACE_LOCAL
NTAPI
DxDdLockDirectDrawSurface(HANDLE hDdSurface)
{
   PEDD_SURFACE pEDDSurface = NULL;
   PDD_SURFACE_LOCAL pSurfacelcl = NULL;

   pEDDSurface = DdHmgLock(hDdSurface, ObjType_DDSURFACE_TYPE, FALSE);
   if (pEDDSurface != NULL)
   {
        pSurfacelcl = &pEDDSurface->ddsSurfaceLocal;
   }

   return pSurfacelcl;
}

BOOL
NTAPI
DxDdUnlockDirectDrawSurface(PDD_SURFACE_LOCAL pSurface)
{
    BOOL retVal = FALSE;
    //PEDD_SURFACE pEDDSurface  = NULL;

    if (pSurface)
    {
        // pEDDSurface = (PEDD_SURFACE)( ((PBYTE)pSurface) - sizeof(DD_BASEOBJECT));
        // InterlockedDecrement(&pEDDSurface->pobj.cExclusiveLock);
        retVal = TRUE;
    }

    return retVal;
}

BOOL
NTAPI
DxDdEnableDirectDraw(PVOID arg1, BOOL arg2/*What for?*/)
{
    // taken from CORE-4490
    //PDEV_WIN32K pdev = (PDEV_WIN32K) arg1 ;
    //return pdev->DriverFunctions.EnableDirectDraw(pdev->dhpdev,
    //                                       &pdev->EDDgpl.ddCallbacks,
    //                                       &pdev->EDDgpl.ddSurfaceCallbacks,
    //                                       &pdev->EDDgpl.ddPaletteCallbacks) ;

    return TRUE;
}

