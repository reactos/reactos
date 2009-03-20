/* 
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Brush Functions
 * FILE:              subsystem/win32/win32k/eng/engbrush.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 3/7/1999: Created
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/** Internal functions ********************************************************/

VOID FASTCALL
EBRUSHOBJ_vInit(EBRUSHOBJ *pebo, PBRUSH pbrush, XLATEOBJ *pxlo)
{
    ASSERT(pebo);
    ASSERT(pbrush);

    if (pbrush->flAttrs & GDIBRUSH_IS_NULL)
    {
        pebo->BrushObject.iSolidColor = 0;
    }
    else if (pbrush->flAttrs & GDIBRUSH_IS_SOLID)
    {
        pebo->BrushObject.iSolidColor = XLATEOBJ_iXlate(pxlo, pbrush->BrushAttr.lbColor);
    }
    else
    {
        pebo->BrushObject.iSolidColor = 0xFFFFFFFF;
        // FIXME: What about calling DrvRealizeBrush?
    }

    pebo->BrushObject.pvRbrush = pbrush->ulRealization;
    pebo->BrushObject.flColorType = 0;
    pebo->GdiBrushObject = pbrush;
    pebo->XlateObject = pxlo;
}


/** Exported DDI functions ****************************************************/

/*
 * @implemented
 */
PVOID APIENTRY
BRUSHOBJ_pvAllocRbrush(IN BRUSHOBJ  *BrushObj,
		       IN ULONG  ObjSize)
{
  BrushObj->pvRbrush=EngAllocMem(0, ObjSize, 0);
  return(BrushObj->pvRbrush);
}

/*
 * @implemented
 */
PVOID APIENTRY
BRUSHOBJ_pvGetRbrush(IN BRUSHOBJ  *BrushObj)
{
    // FIXME: this is wrong! Read msdn.
  return(BrushObj->pvRbrush);
}

/*
 * @implemented
 */
ULONG APIENTRY
BRUSHOBJ_ulGetBrushColor(IN BRUSHOBJ *BrushObj)
{
   return BrushObj->iSolidColor;
}

/* EOF */
