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
STDCALL
DxDdLockDirectDrawSurface(HANDLE hDdSurface)
{
   PEDD_SURFACE_LOCAL pEDDSurfacelcl = NULL;
   PDD_SURFACE_LOCAL pSurfacelcl = NULL;

   pSurfacelcl = DdHmgLock(hDdSurface, 2, FALSE);
   if (pSurfacelcl != NULL)
   {
        pSurfacelcl = &pEDDSurfacelcl->Surfacelcl;
   }

   return pSurfacelcl;
}

BOOL
STDCALL
DxDdUnlockDirectDrawSurface(PDD_SURFACE_LOCAL pSurface)
{
    BOOL retVal = FALSE;
    PEDD_SURFACE_LOCAL pEDDSurfacelcl  = NULL;

    if (pSurface)
    {
        pEDDSurfacelcl = (PEDD_SURFACE_LOCAL)( ((PBYTE)pSurface) - sizeof(DD_BASEOBJECT));
        InterlockedDecrement(&pEDDSurfacelcl->Object.cExclusiveLock);
        retVal = TRUE;
    }

    return retVal;
}




