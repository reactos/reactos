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

//#define NDEBUG
#include <win32k/debug1.h>


PBRUSHOBJ PenToBrushObj(PDC dc, PENOBJ *pen)
{
  BRUSHOBJ *BrushObj;
  XLATEOBJ *RGBtoVGA16;

  BrushObj = ExAllocatePool(NonPagedPool, sizeof(BRUSHOBJ));
  BrushObj->iSolidColor = pen->logpen.lopnColor;

  return BrushObj;
}

VOID BitmapToSurf(HDC hdc, PSURFGDI SurfGDI, PSURFOBJ SurfObj, PBITMAPOBJ Bitmap)
{
  ASSERT( SurfGDI );
  if( Bitmap ){
  	if(Bitmap->dib)
  	{
  	  SurfGDI->BitsPerPixel = Bitmap->dib->dsBm.bmBitsPixel;
  	  SurfObj->lDelta       = Bitmap->dib->dsBm.bmWidthBytes;
  	  SurfObj->pvBits       = Bitmap->dib->dsBm.bmBits;
  	  SurfObj->cjBits       = Bitmap->dib->dsBm.bmHeight * Bitmap->dib->dsBm.bmWidthBytes;
  	} else {
  	  SurfGDI->BitsPerPixel = Bitmap->bitmap.bmBitsPixel;
  	  SurfObj->lDelta	  = Bitmap->bitmap.bmWidthBytes;
  	  SurfObj->pvBits	  = Bitmap->bitmap.bmBits;
  	  SurfObj->cjBits       = Bitmap->bitmap.bmHeight * Bitmap->bitmap.bmWidthBytes;
  	}
	SurfObj->sizlBitmap	= Bitmap->size; // FIXME: alloc memory for our own struct?
  }

  SurfObj->dhsurf	= NULL;
  SurfObj->hsurf	= NULL;
  SurfObj->dhpdev	= NULL;
  SurfObj->hdev		= NULL;
  SurfObj->pvScan0	= SurfObj->pvBits; // start of bitmap
  SurfObj->iUniq	 = 0; // not sure..
  SurfObj->iBitmapFormat = BitmapFormat(SurfGDI->BitsPerPixel, BI_RGB);
  SurfObj->iType	 = STYPE_BITMAP;
  SurfObj->fjBitmap	 = BMF_TOPDOWN;
}
