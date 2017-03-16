/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native driver for dxg implementation
 * FILE:             win32ss/reactx/dxg/eng.c
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
