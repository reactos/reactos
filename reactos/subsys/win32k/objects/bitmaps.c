#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <win32k/bitmaps.h>
//#include <win32k/debug.h>
#include "../eng/handle.h"
#include <include/inteng.h>

#define NDEBUG
#include <win32k/debug1.h>

BOOL STDCALL W32kBitBlt(HDC  hDCDest,
                 INT  XDest,
                 INT  YDest,
                 INT  Width,
                 INT  Height,
                 HDC  hDCSrc,
                 INT  XSrc,
                 INT  YSrc,
                 DWORD  ROP)
{
  PDC DCDest = DC_HandleToPtr(hDCDest);
  PDC DCSrc  = DC_HandleToPtr(hDCSrc);
  PSURFOBJ SurfDest, SurfSrc;
  PSURFGDI SurfGDIDest, SurfGDISrc;
  RECTL DestRect;
  POINTL SourcePoint;
  PBITMAPOBJ DestBitmapObj;
  PBITMAPOBJ SrcBitmapObj;
  BOOL Status, SurfDestAlloc, SurfSrcAlloc;
  PPALOBJ DCLogPal;
  PPALGDI PalDestGDI, PalSourceGDI;
  PXLATEOBJ XlateObj = NULL;
  HPALETTE SourcePalette, DestPalette;

  /* Offset the destination and source by the origin of their DCs. */
  XDest += DCDest->w.DCOrgX;
  YDest += DCDest->w.DCOrgY;
  XSrc += DCSrc->w.DCOrgX;
  YSrc += DCSrc->w.DCOrgY;

  DestRect.left   = XDest;
  DestRect.top    = YDest;
  DestRect.right  = XDest+Width;
  DestRect.bottom = YDest+Height;

  SourcePoint.x = XSrc;
  SourcePoint.y = YSrc;

  SurfDestAlloc = FALSE;
  SurfSrcAlloc  = FALSE;

  // Determine surfaces to be used in the bitblt
  SurfDest = (PSURFOBJ)AccessUserObject(DCDest->Surface);
  SurfSrc  = (PSURFOBJ)AccessUserObject(DCSrc->Surface);

  SurfGDIDest = (PSURFGDI)AccessInternalObjectFromUserObject(SurfDest);
  SurfGDISrc  = (PSURFGDI)AccessInternalObjectFromUserObject(SurfSrc);

  // Retrieve the logical palette of the destination DC
  DCLogPal = (PPALOBJ)AccessUserObject(DCDest->w.hPalette);

  if(DCLogPal)
    if(DCLogPal->logicalToSystem)
      XlateObj = DCLogPal->logicalToSystem;

  // If the source and destination formats differ, create an XlateObj [what if we already have one??]
  if((BitsPerFormat(SurfDest->iBitmapFormat) != BitsPerFormat(SurfSrc->iBitmapFormat)) && (XlateObj == NULL))
  {
    if(DCDest->w.hPalette != 0)
    {
      DestPalette = DCDest->w.hPalette;
    } else
      DestPalette = W32kGetStockObject(DEFAULT_PALETTE);

    if(DCSrc->w.hPalette != 0)
    {
      SourcePalette = DCSrc->w.hPalette;
    } else
      SourcePalette = W32kGetStockObject(DEFAULT_PALETTE);

    PalDestGDI   = (PPALGDI)AccessInternalObject(DestPalette);
    PalSourceGDI = (PPALGDI)AccessInternalObject(SourcePalette);

    XlateObj = (PXLATEOBJ)IntEngCreateXlate(PalDestGDI->Mode, PalSourceGDI->Mode, DestPalette, SourcePalette);
  }

  // Perform the bitblt operation

  Status = IntEngBitBlt(SurfDest, SurfSrc, NULL, NULL, XlateObj, &DestRect, &SourcePoint, NULL, NULL, NULL, ROP);

  if(SurfDestAlloc == TRUE) ExFreePool(SurfDest);
  if(SurfSrcAlloc  == TRUE) ExFreePool(SurfSrc);

  DC_ReleasePtr(hDCDest);
  DC_ReleasePtr(hDCSrc);

  return Status;
}

HBITMAP STDCALL W32kCreateBitmap(INT  Width,
                          INT  Height,
                          UINT  Planes,
                          UINT  BitsPerPel,
                          CONST VOID *Bits)
{
  PBITMAPOBJ  bmp;
  HBITMAP  hBitmap;

  Planes = (BYTE) Planes;
  BitsPerPel = (BYTE) BitsPerPel;

  /* Check parameters */
  if (!Height || !Width)
  {
    return 0;
  }
  if (Planes != 1)
  {
    UNIMPLEMENTED;
    return  0;
  }
  if (Height < 0)
  {
    Height = -Height;
  }
  if (Width < 0)
  {
    Width = -Width;
  }

  /* Create the BITMAPOBJ */
  hBitmap = BITMAPOBJ_AllocBitmap ();
  if (!hBitmap)
  {
	DPRINT("W32kCreateBitmap: BITMAPOBJ_AllocBitmap returned 0\n");
    return 0;
  }

  bmp = BITMAPOBJ_HandleToPtr( hBitmap );

  DPRINT("W32kCreateBitmap:%dx%d, %d (%d BPP) colors returning %08x\n", Width, Height,
         1 << (Planes * BitsPerPel), BitsPerPel, bmp);

  bmp->size.cx = Width;
  bmp->size.cy = Height;
  bmp->bitmap.bmType = 0;
  bmp->bitmap.bmWidth = Width;
  bmp->bitmap.bmHeight = Height;
  bmp->bitmap.bmPlanes = Planes;
  bmp->bitmap.bmBitsPixel = BitsPerPel;
  bmp->bitmap.bmWidthBytes = BITMAPOBJ_GetWidthBytes (Width, BitsPerPel);
  bmp->bitmap.bmBits = NULL;
  bmp->DDBitmap = NULL;
  bmp->dib = NULL;

  // Allocate memory for bitmap bits
  bmp->bitmap.bmBits = ExAllocatePool(PagedPool, bmp->bitmap.bmWidthBytes * bmp->bitmap.bmHeight);

  if (Bits) /* Set bitmap bits */
  {
    W32kSetBitmapBits(hBitmap, Height * bmp->bitmap.bmWidthBytes, Bits);
  }

  BITMAPOBJ_ReleasePtr( hBitmap );

  return  hBitmap;
}

BOOL Bitmap_InternalDelete( PBITMAPOBJ pBmp )
{
	ASSERT( pBmp );
	if( pBmp->bitmap.bmBits )
		ExFreePool(pBmp->bitmap.bmBits);
	return TRUE;
}


HBITMAP STDCALL W32kCreateCompatibleBitmap(HDC hDC,
                                    INT  Width,
                                    INT  Height)
{
  HBITMAP  hbmpRet;
  PDC  dc;

  hbmpRet = 0;
  dc = DC_HandleToPtr (hDC);

  DPRINT("W32kCreateCompatibleBitmap(%04x,%d,%d, bpp:%d) = \n", hDC, Width, Height, dc->w.bitsPerPixel);

  if (!dc)
  {
    return 0;
  }
  if ((Width >= 0x10000) || (Height >= 0x10000))
  {
    DPRINT("got bad width %d or height %d, please look for reason\n", Width, Height);
  }
  else
  {
    /* MS doc says if width or height is 0, return 1-by-1 pixel, monochrome bitmap */
    if (!Width || !Height)
    {
      hbmpRet = W32kCreateBitmap (1, 1, 1, 1, NULL);
    }
    else
    {
      hbmpRet = W32kCreateBitmap(Width, Height, 1, dc->w.bitsPerPixel, NULL);
    }
  }
  DPRINT ("\t\t%04x\n", hbmpRet);
  DC_ReleasePtr( hDC );
  return hbmpRet;
}

HBITMAP STDCALL W32kCreateBitmapIndirect(CONST BITMAP  *BM)
{
  return W32kCreateBitmap (BM->bmWidth,
                           BM->bmHeight,
                           BM->bmPlanes,
                           BM->bmBitsPixel,
                           BM->bmBits);
}

HBITMAP STDCALL W32kCreateDiscardableBitmap(HDC  hDC,
                                     INT  Width,
                                     INT  Height)
{
  /* FIXME: this probably should do something else */
  return  W32kCreateCompatibleBitmap(hDC, Width, Height);
}

BOOL STDCALL W32kExtFloodFill(HDC  hDC,
                      INT  XStart,
                      INT  YStart,
                      COLORREF  Color,
                      UINT  FillType)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kFloodFill(HDC  hDC,
                    INT  XStart,
                    INT  YStart,
                    COLORREF  Fill)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kGetBitmapDimensionEx(HBITMAP  hBitmap,
                               LPSIZE  Dimension)
{
  PBITMAPOBJ  bmp;

  bmp = BITMAPOBJ_HandleToPtr (hBitmap);
  if (bmp == NULL)
  {
    return FALSE;
  }

  *Dimension = bmp->size;

  return  TRUE;
}

COLORREF STDCALL W32kGetPixel(HDC  hDC,
                       INT  XPos,
                       INT  YPos)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kMaskBlt(HDC  hDCDest,
                  INT  XDest,
                  INT  YDest,
                  INT  Width,
                  INT  Height,
                  HDC  hDCSrc,
                  INT  XSrc,
                  INT  YSrc,
                  HBITMAP  hMaskBitmap,
                  INT  xMask,
                  INT  yMask,
                  DWORD  ROP)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kPlgBlt(HDC  hDCDest,
                CONST POINT  *Point,
                HDC  hDCSrc,
                INT  XSrc,
                INT  YSrc,
                INT  Width,
                INT  Height,
                HBITMAP  hMaskBitmap,
                INT  xMask,
                INT  yMask)
{
  UNIMPLEMENTED;
}

LONG STDCALL W32kSetBitmapBits(HBITMAP  hBitmap,
                        DWORD  Bytes,
                        CONST VOID *Bits)
{
  DWORD  height, ret;
  PBITMAPOBJ  bmp;

  bmp = BITMAPOBJ_HandleToPtr (hBitmap);
  if (bmp == NULL || Bits == NULL)
  {
    return 0;
  }

  if (Bytes < 0)
  {
    DPRINT ("(%ld): Negative number of bytes passed???\n", Bytes );
    Bytes = -Bytes;
  }

  /* Only get entire lines */
  height = Bytes / bmp->bitmap.bmWidthBytes;
  if (height > bmp->bitmap.bmHeight)
  {
    height = bmp->bitmap.bmHeight;
  }
  Bytes = height * bmp->bitmap.bmWidthBytes;
  DPRINT ("(%08x, bytes:%ld, bits:%p) %dx%d %d colors fetched height: %ld\n",
          hBitmap,
          Bytes,
          Bits,
          bmp->bitmap.bmWidth,
          bmp->bitmap.bmHeight,
          1 << bmp->bitmap.bmBitsPixel,
          height);

#if 0
  /* FIXME: call DDI specific function here if available  */
  if(bmp->DDBitmap)
  {
    DPRINT ("Calling device specific BitmapBits\n");
    if (bmp->DDBitmap->funcs->pBitmapBits)
    {
      ret = bmp->DDBitmap->funcs->pBitmapBits(hBitmap, (void *) Bits, Bytes, DDB_SET);
    }
    else
    {
      DPRINT ("BitmapBits == NULL??\n");
      ret = 0;
    }
  }
  else
#endif
    {
      /* FIXME: Alloc enough for entire bitmap */
      if (bmp->bitmap.bmBits == NULL)
      {
        bmp->bitmap.bmBits = ExAllocatePool (PagedPool, Bytes);
      }
      if(!bmp->bitmap.bmBits)
      {
        DPRINT ("Unable to allocate bit buffer\n");
        ret = 0;
      }
      else
      {
        memcpy(bmp->bitmap.bmBits, Bits, Bytes);
        ret = Bytes;
      }
    }

  return ret;
}

BOOL STDCALL W32kSetBitmapDimensionEx(HBITMAP  hBitmap,
                               INT  Width,
                               INT  Height,
                               LPSIZE  Size)
{
  PBITMAPOBJ  bmp;

  bmp = BITMAPOBJ_HandleToPtr (hBitmap);
  if (bmp == NULL)
  {
    return FALSE;
  }

  if (Size)
  {
    *Size = bmp->size;
  }
  bmp->size.cx = Width;
  bmp->size.cy = Height;

  return TRUE;
}

COLORREF STDCALL W32kSetPixel(HDC  hDC,
                       INT  X,
                       INT  Y,
                       COLORREF  Color)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kSetPixelV(HDC  hDC,
                    INT  X,
                    INT  Y,
                    COLORREF  Color)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kStretchBlt(HDC  hDCDest,
                     INT  XOriginDest,
                     INT  YOriginDest,
                     INT  WidthDest,
                     INT  HeightDest,
                     HDC  hDCSrc,
                     INT  XOriginSrc,
                     INT  YOriginSrc,
                     INT  WidthSrc,
                     INT  HeightSrc,
                     DWORD  ROP)
{
  UNIMPLEMENTED;
}

/*  Internal Functions  */

INT
BITMAPOBJ_GetWidthBytes (INT bmWidth, INT bpp)
{
  switch(bpp)
  {
    case 1:
      return 2 * ((bmWidth+15) >> 4);

    case 24:
      bmWidth *= 3; /* fall through */
    case 8:
      return bmWidth + (bmWidth & 1);

    case 32:
      return bmWidth * 4;

    case 16:
    case 15:
      return bmWidth * 2;

    case 4:
      return 2 * ((bmWidth+3) >> 2);

    default:
      DPRINT ("stub");
  }

  return -1;
}

HBITMAP  BITMAPOBJ_CopyBitmap(HBITMAP  hBitmap)
{
  PBITMAPOBJ  bmp;
  HBITMAP  res;
  BITMAP  bm;

  bmp = BITMAPOBJ_HandleToPtr (hBitmap);
  if (bmp == NULL)
  {
    return 0;
  }
  res = 0;

  bm = bmp->bitmap;
  bm.bmBits = NULL;
  res = W32kCreateBitmapIndirect(&bm);
  if(res)
  {
    char *buf;

    buf = ExAllocatePool (NonPagedPool, bm.bmWidthBytes * bm.bmHeight);
    W32kGetBitmapBits (hBitmap, bm.bmWidthBytes * bm.bmHeight, buf);
    W32kSetBitmapBits (res, bm.bmWidthBytes * bm.bmHeight, buf);
    ExFreePool (buf);
  }

  return  res;
}

INT BITMAP_GetObject(BITMAPOBJ * bmp, INT count, LPVOID buffer)
{
  if(bmp->dib)
  {
    if(count < sizeof(DIBSECTION))
    {
      if (count > sizeof(BITMAP)) count = sizeof(BITMAP);
    }
    else
    {
      if (count > sizeof(DIBSECTION)) count = sizeof(DIBSECTION);
    }
    memcpy(buffer, bmp->dib, count);
    return count;
  }
  else
  {
    if (count > sizeof(BITMAP)) count = sizeof(BITMAP);
    memcpy(buffer, &bmp->bitmap, count);
    return count;
  }
}
