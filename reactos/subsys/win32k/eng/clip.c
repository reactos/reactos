/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Clipping Functions
 * FILE:              subsys/win32k/eng/clip.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 21/8/1999: Created
 */

#include <ddk/winddi.h>
#include <ddk/ntddk.h>
#include "objects.h"
#include "clip.h"
#include <include/object.h>

VOID EngDeleteClipRegion(CLIPOBJ *ClipObj)
{
  HCLIP HClip      = AccessHandleFromUserObject(ClipObj);
  CLIPGDI *ClipGDI = (CLIPGDI*)AccessInternalObject(HClip);

  EngFreeMem(ClipGDI);
  EngFreeMem(ClipObj);
  FreeGDIHandle(HClip);
}


CLIPOBJ * STDCALL
EngCreateClip(VOID)
{
  return EngAllocMem(FL_ZERO_MEMORY, sizeof(CLIPOBJ), 0);
}

VOID STDCALL
EngDeleteClip(CLIPOBJ *ClipRegion)
{
  EngFreeMem(ClipRegion);
}

ULONG STDCALL
CLIPOBJ_cEnumStart(IN PCLIPOBJ ClipObj,
		   IN BOOL ShouldDoAll,
		   IN ULONG ClipType,
		   IN ULONG BuildOrder,
		   IN ULONG MaxRects)
{
  CLIPGDI *ClipGDI = (CLIPGDI*)AccessInternalObjectFromUserObject(ClipObj);

  ClipGDI->EnumPos     = 0;
  ClipGDI->EnumRects.c = MaxRects;

  // Return the number of rectangles enumerated
  if(ClipGDI->EnumRects.c>MaxRects)
  {
    ClipGDI->EnumRects.c = 0xFFFFFFFF;
  }

  return ClipGDI->EnumRects.c;
}

BOOL STDCALL
CLIPOBJ_bEnum(IN PCLIPOBJ ClipObj,
	      IN ULONG ObjSize,
	      OUT ULONG *EnumRects)
{
  CLIPGDI *ClipGDI = (CLIPGDI*)AccessInternalObjectFromUserObject(ClipObj);

  ClipGDI->EnumPos++;

  if(ClipGDI->EnumPos > ClipGDI->EnumRects.c)
  {
    return FALSE;
  } else
    return TRUE;
}
