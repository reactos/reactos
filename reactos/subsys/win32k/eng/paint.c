/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Paint Functions
 * FILE:              subsys/win32k/eng/paint.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 3/7/1999: Created
 */

#include <ddk/winddi.h>
#include "objects.h"
#include "brush.h"
#include "enum.h"

BOOL FillSolid(SURFOBJ *Surface, PRECTL Dimensions, ULONG iColor)
{
  LONG y;
  ULONG x, LineWidth, leftOfBitmap;
  SURFGDI *SurfaceGDI;

  SurfaceGDI = (SURFGDI*)AccessInternalObjectFromUserObject(Surface);
  LineWidth  = Dimensions->right - Dimensions->left;

  for (y = Dimensions->top; y < Dimensions->bottom; y++)
  {
//      EngHLine(Surface, SurfaceGDI, Dimensions->left, y, LineWidth, iColor);
  }

  return TRUE;
}

BOOL EngPaintRgn(SURFOBJ *Surface, CLIPOBJ *ClipRegion, ULONG iColor, MIX Mix,
               BRUSHINST *BrushInst, POINTL *BrushPoint)
{
  RECT_ENUM RectEnum;
  BOOL EnumMore;

  switch(ClipRegion->iMode) {

    case TC_RECTANGLES:

    /* Rectangular clipping can be handled without enumeration.
       Note that trivial clipping is not possible, since the clipping
       region defines the area to fill */

    if (ClipRegion->iDComplexity == DC_RECT)
    {
      FillSolid(Surface, &ClipRegion->rclBounds, iColor);
    } else {

      /* Enumerate all the rectangles and draw them */
      CLIPOBJ_cEnumStart(ClipRegion, FALSE, CT_RECTANGLES, CD_ANY, ENUM_RECT_LIMIT);

      do {
        EnumMore = CLIPOBJ_bEnum(ClipRegion, sizeof(RectEnum), (PVOID) &RectEnum);
        FillSolid(Surface, &RectEnum.arcl[0], iColor);
      } while (EnumMore);
    }

    return(TRUE);

    default:
       return(FALSE);
  }
}

BOOL EngPaint(IN SURFOBJ *Surface, IN CLIPOBJ *ClipRegion,
              IN BRUSHOBJ *Brush,  IN POINTL *BrushOrigin,
              IN MIX  Mix)
{
  BOOLEAN ret;
  SURFGDI *SurfGDI;

  // Is the surface's Paint function hooked?
  SurfGDI = (SURFGDI*)AccessInternalObjectFromUserObject(Surface);

  // FIXME: Perform Mouse Safety on the given ClipRegion
  // MouseSafetyOnDrawStart(Surface, SurfGDI, x1, y1, x2, y2);

  if((Surface->iType!=STYPE_BITMAP) && (SurfGDI->Paint!=NULL))
  {
    // Call the driver's DrvPaint
    ret = SurfGDI->Paint(Surface, ClipRegion, Brush, BrushOrigin, Mix);
    // MouseSafetyOnDrawEnd(Surface, SurfGDI);
    return ret;
  }

  // FIXME: We only support a brush's solid color attribute
  ret = EngPaintRgn(Surface, ClipRegion, Brush->iSolidColor, Mix, NULL, BrushOrigin);

  // MouseSafetyOnDrawEnd(Surface, SurfGDI);

  return ret;
}

BOOL EngEraseSurface(SURFOBJ *Surface, RECTL *Rect, ULONG iColor)
{
  return FillSolid(Surface, Rect, iColor);
}
