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

PBRUSHOBJ PenToBrushObj(PDC dc, PENOBJ *pen)
{
   BRUSHOBJ *BrushObj;
   XLATEOBJ *RGBtoVGA16;

// FIXME: move color translation routines to W32kSelectObject

DbgPrint("PenToBrushObj:%08x ", pen);
   BrushObj = ExAllocatePool(NonPagedPool, sizeof(BRUSHOBJ));

   RGBtoVGA16 = EngCreateXlate(PAL_INDEXED, PAL_RGB,
                               dc->DevInfo.hpalDefault, NULL);
   BrushObj->iSolidColor = XLATEOBJ_iXlate(RGBtoVGA16, pen->logpen.lopnColor);

DbgPrint("BrushObj->iSolidColor:%u\n", BrushObj->iSolidColor);
   return BrushObj;

//   CreateGDIHandle(BrushGDI, BrushObj);
}

VOID BitmapToSurf(PSURFGDI SurfGDI, PSURFOBJ SurfObj, PBITMAPOBJ Bitmap)
{
  SurfObj->dhsurf	 = NULL;
  SurfObj->hsurf	 = NULL;
  SurfObj->dhpdev	 = NULL;
  SurfObj->hdev		 = NULL;
  SurfObj->sizlBitmap	 = Bitmap->size;
  SurfObj->cjBits	 = Bitmap->bitmap.bmHeight * Bitmap->bitmap.bmWidthBytes;
  SurfObj->pvBits	 = Bitmap->bitmap.bmBits;
  SurfObj->pvScan0	 = NULL; // start of bitmap
  SurfObj->lDelta	 = Bitmap->bitmap.bmWidthBytes;
  SurfObj->iUniq	 = 0; // not sure..
  SurfObj->iBitmapFormat = BMF_4BPP; /* FIXME */
  SurfObj->iType	 = STYPE_BITMAP;
  SurfObj->fjBitmap	 = BMF_TOPDOWN;

  SurfGDI->BytesPerPixel = bytesPerPixel(SurfObj->iType);

DbgPrint("Bitmap2Surf: cjBits: %u lDelta: %u width: %u height: %u\n",
  SurfObj->cjBits, SurfObj->lDelta, Bitmap->bitmap.bmWidth, Bitmap->bitmap.bmHeight);
}
