#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>

/* FIXME: Surely we should just have one include file that includes all of these.. */
#include <win32k/bitmaps.h>
#include <win32k/coord.h>
#include <win32k/driver.h>
#include <win32k/dc.h>
#include <win32k/print.h>
#include <win32k/region.h>
#include <win32k/gdiobj.h>
#include <win32k/pen.h>
#include "../eng/objects.h"

#define NDEBUG
#include <win32k/debug1.h>


PBRUSHOBJ PenToBrushObj(PDC dc, PENOBJ *pen)
{
  BRUSHOBJ *BrushObj;
  XLATEOBJ *RGBtoVGA16;

  ASSERT( pen );

  BrushObj = ExAllocatePool(NonPagedPool, sizeof(BRUSHOBJ));
  BrushObj->iSolidColor = pen->logpen.lopnColor;

  return BrushObj;
}

HBITMAP BitmapToSurf(PBITMAPOBJ BitmapObj)
{
  HBITMAP BitmapHandle;

  if (NULL != BitmapObj->dib)
    {
    BitmapHandle = EngCreateBitmap(BitmapObj->size, BitmapObj->dib->dsBm.bmWidthBytes,
                                   BitmapFormat(BitmapObj->dib->dsBm.bmBitsPixel, BI_RGB),
                                   0, BitmapObj->dib->dsBm.bmBits);
    }
  else
    {
    BitmapHandle = EngCreateBitmap(BitmapObj->size, BitmapObj->bitmap.bmWidthBytes,
                                   BitmapFormat(BitmapObj->bitmap.bmBitsPixel, BI_RGB),
                                   0, BitmapObj->bitmap.bmBits);
    }

  return BitmapHandle;
}
