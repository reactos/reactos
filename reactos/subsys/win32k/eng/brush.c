/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Brush Functions
 * FILE:              subsys/win32k/eng/brush.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 3/7/1999: Created
 */

#define NTOS_KERNEL_MODE
#include <ntos.h>
#include <ddk/winddi.h>

PVOID STDCALL
BRUSHOBJ_pvAllocRbrush(IN BRUSHOBJ  *BrushObj,
		       IN ULONG  ObjSize)
{
  ROS_BRUSHOBJ *bo = (ROS_BRUSHOBJ *)BrushObj;
  bo->pvRbrush=EngAllocMem(0, ObjSize, 0);
  return(bo->pvRbrush);
}

PVOID STDCALL BRUSHOBJ_pvGetRbrush(IN BRUSHOBJ  *BrushObj)
{
  ROS_BRUSHOBJ *bo = (ROS_BRUSHOBJ *)BrushObj;
  return(bo->pvRbrush);
}
