/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Brush Functions
 * FILE:              subsys/win32k/eng/brush.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 3/7/1999: Created
 */

#include <ddk/winddi.h>

PVOID BRUSHOBJ_pvAllocRbrush(IN PBRUSHOBJ  BrushObj,
                             IN ULONG  ObjSize)
{
   BrushObj->pvRbrush=EngAllocMem(0, ObjSize, 0);
   return BrushObj->pvRbrush;
}

PVOID BRUSHOBJ_pvGetRbrush(IN PBRUSHOBJ  BrushObj)
{
   return BrushObj->pvRbrush;
}
