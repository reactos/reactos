/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Surace Functions
 * FILE:              subsys/win32k/eng/surface.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 3/7/1999: Created
 */

#include <ddk/winddi.h>

BOOL EngAssociateSurface(IN HSURF  Surface,
                         IN HDEV  Dev,
                         IN ULONG  Hooks)
{
   SURFOBJ *Surfobj;

   // Point our new Surfobj to hsurf (assumes hsurf is value returned by
   // ExAllocatePool)
   Surfobj = Surface;

   // Associate the hdev
   Surfobj->hdev = Dev;

   // FIXME: Hook up the specified functions
}

BOOL APIENTRY EngDeleteSurface(HSURF hsurf)
{
   // Assume the hsurf was the value returned by ExAllocatePool
   ExFreePool(hsurf);
}

SURFOBJ *EngLockSurface(IN HSURF hsurf)
{
  /* We assume that hsurf is the value returned from ExAllocatePool */
  return EngAllocUserMem(NULL, sizeof(SURFOBJ), "");
}

VOID EngUnlockSurface(IN SURFOBJ *pso)
{
  EngFreeUserMem(sizeof(pso));
}
