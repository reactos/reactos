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
   PDD_ENTRY pObject;
   PDD_SURFACE_LOCAL pSurfacelcl = NULL;

   pObject = DdHmgLock(hDdSurface, 2, 0);
   if (pObject != NULL)
   {
        pSurfacelcl = (PDD_SURFACE_LOCAL)((PBYTE)pObject + sizeof(PDD_ENTRY));
   }

   return pSurfacelcl;
}


