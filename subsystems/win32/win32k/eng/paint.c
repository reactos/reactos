/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Paint Functions
 * FILE:              subsys/win32k/eng/paint.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 3/7/1999: Created
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

static BOOL APIENTRY FillSolidUnlocked(SURFOBJ *pso, PRECTL pRect, ULONG iColor)
{
  LONG y;
  ULONG LineWidth;
  SURFACE *psurf;

  ASSERT(pso);
  ASSERT(pRect);
  psurf = CONTAINING_RECORD(pso, SURFACE, SurfObj);
  MouseSafetyOnDrawStart(pso, pRect->left, pRect->top, pRect->right, pRect->bottom);
  LineWidth  = pRect->right - pRect->left;
  DPRINT(" LineWidth: %d, top: %d, bottom: %d\n", LineWidth, pRect->top, pRect->bottom);
  for (y = pRect->top; y < pRect->bottom; y++)
  {
    DibFunctionsForBitmapFormat[pso->iBitmapFormat].DIB_HLine(
      pso, pRect->left, pRect->right, y, iColor);
  }
  MouseSafetyOnDrawEnd(pso);

  return TRUE;
}

BOOL APIENTRY FillSolid(SURFOBJ *pso, PRECTL pRect, ULONG iColor)
{
  SURFACE *psurf;
  BOOL Result;
  psurf = CONTAINING_RECORD(pso, SURFACE, SurfObj);
  SURFACE_LockBitmapBits(psurf);
  Result = FillSolidUnlocked(pso, pRect, iColor);
  SURFACE_UnlockBitmapBits(psurf);
  return Result;
}

BOOL APIENTRY
EngPaintRgn(SURFOBJ *pso, CLIPOBJ *ClipRegion, ULONG iColor, MIX Mix,
            BRUSHOBJ *BrushObj, POINTL *BrushPoint)
{
  RECT_ENUM RectEnum;
  BOOL EnumMore;
  ULONG i;

  ASSERT(pso);
  ASSERT(ClipRegion);

  DPRINT("ClipRegion->iMode:%d, ClipRegion->iDComplexity: %d\n Color: %d", ClipRegion->iMode, ClipRegion->iDComplexity, iColor);
  switch(ClipRegion->iMode) {

    case TC_RECTANGLES:

    /* Rectangular clipping can be handled without enumeration.
       Note that trivial clipping is not possible, since the clipping
       region defines the area to fill */

    if (ClipRegion->iDComplexity == DC_RECT)
    {
      FillSolidUnlocked(pso, &(ClipRegion->rclBounds), iColor);
    } else {

      /* Enumerate all the rectangles and draw them */
      CLIPOBJ_cEnumStart(ClipRegion, FALSE, CT_RECTANGLES, CD_ANY, 0);

      do {
        EnumMore = CLIPOBJ_bEnum(ClipRegion, sizeof(RectEnum), (PVOID) &RectEnum);
        for (i = 0; i < RectEnum.c; i++) {
          FillSolidUnlocked(pso, RectEnum.arcl + i, iColor);
        }
      } while (EnumMore);
    }

    return(TRUE);

    default:
       return(FALSE);
  }
}

/*
 * @unimplemented
 */
BOOL APIENTRY
EngPaint(IN SURFOBJ *pso,
	 IN CLIPOBJ *ClipRegion,
	 IN BRUSHOBJ *Brush,
	 IN POINTL *BrushOrigin,
	 IN MIX  Mix)
{
  BOOLEAN ret;

  // FIXME: We only support a brush's solid color attribute
  ret = EngPaintRgn(pso, ClipRegion, Brush->iSolidColor, Mix, Brush, BrushOrigin);

  return ret;
}

BOOL APIENTRY
IntEngPaint(IN SURFOBJ *pso,
            IN CLIPOBJ *ClipRegion,
            IN BRUSHOBJ *Brush,
            IN POINTL *BrushOrigin,
            IN MIX  Mix)
{
  SURFACE *psurf = CONTAINING_RECORD(pso, SURFACE, SurfObj);
  BOOL ret;

  DPRINT("pso->iType == %d\n", pso->iType);
  /* Is the surface's Paint function hooked? */
  if((pso->iType!=STYPE_BITMAP) && (psurf->flHooks & HOOK_PAINT))
  {
    // Call the driver's DrvPaint
    SURFACE_LockBitmapBits(psurf);
    MouseSafetyOnDrawStart(pso, ClipRegion->rclBounds.left,
	                         ClipRegion->rclBounds.top, ClipRegion->rclBounds.right,
							 ClipRegion->rclBounds.bottom);

    ret = GDIDEVFUNCS(pso).Paint(
      pso, ClipRegion, Brush, BrushOrigin, Mix);
    MouseSafetyOnDrawEnd(pso);
    SURFACE_UnlockBitmapBits(psurf);
    return ret;
  }
  return EngPaint(pso, ClipRegion, Brush, BrushOrigin, Mix );

}
/* EOF */
