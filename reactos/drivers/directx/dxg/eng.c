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
   PDD_SURFACE_LOCAL pSurfacelcl = NULL;

   pSurfacelcl = DdHmgLock(hDdSurface, 2, FALSE);
   if (pSurfacelcl != NULL)
   {
        pSurfacelcl = (PDD_SURFACE_LOCAL)(((PBYTE)&pSurfacelcl) + sizeof(DD_BASEOBJECT));
   }

   return pSurfacelcl;
}

BOOL
STDCALL
DxDdUnlockDirectDrawSurface(PDD_SURFACE_LOCAL pSurface)
{
    BOOL retVal = FALSE;
    PDD_BASEOBJECT pObject = NULL;

    if (pSurface)
    {
        pObject = (PDD_BASEOBJECT)( ((PBYTE)&pSurface) - sizeof(DD_BASEOBJECT));
        InterlockedDecrement(&pObject->cExclusiveLock);
        retVal = TRUE;
    }

    return retVal;
}


