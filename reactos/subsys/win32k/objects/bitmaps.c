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
/* $Id: bitmaps.c,v 1.38 2003/08/31 07:56:24 gvg Exp $ */
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <win32k/gdiobj.h>
#include <win32k/bitmaps.h>
//#include <win32k/debug.h>
#include "../eng/handle.h"
#include <include/inteng.h>
#include <include/eng.h>
#include <include/surface.h>
#include <include/palette.h>

#define NDEBUG
#include <win32k/debug1.h>

//FIXME: where should CLR_INVALID be defined?
#define CLR_INVALID 0xffffffff

BOOL STDCALL NtGdiBitBlt(HDC  hDCDest,
                 INT  XDest,
                 INT  YDest,
                 INT  Width,
                 INT  Height,
                 HDC  hDCSrc,
                 INT  XSrc,
                 INT  YSrc,
                 DWORD  ROP)
{
  GDIMULTILOCK Lock[2] = {{hDCDest, 0, GDI_OBJECT_TYPE_DC}, {hDCSrc, 0, GDI_OBJECT_TYPE_DC}};
  PDC DCDest = NULL;
  PDC DCSrc  = NULL;
  PSURFOBJ SurfDest, SurfSrc;
  PSURFGDI SurfGDIDest, SurfGDISrc;
  RECTL DestRect;
  POINTL SourcePoint;
  //PBITMAPOBJ DestBitmapObj;
  //PBITMAPOBJ SrcBitmapObj;
  BOOL Status, SurfDestAlloc, SurfSrcAlloc;
  PPALGDI PalDestGDI, PalSourceGDI;
  PXLATEOBJ XlateObj = NULL;
  HPALETTE SourcePalette, DestPalette;
  ULONG SourceMode, DestMode;

  if ( !GDIOBJ_LockMultipleObj(Lock, sizeof(Lock)/sizeof(Lock[0])) )
    {
      DPRINT1("GDIOBJ_LockMultipleObj() failed\n" );
      return STATUS_INVALID_PARAMETER;
    }

  DCDest = Lock[0].pObj;
  DCSrc = Lock[1].pObj;

  if ( !DCDest || !DCSrc )
    return STATUS_INVALID_PARAMETER;

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
  SurfDest = (PSURFOBJ)AccessUserObject((ULONG)DCDest->Surface);
  SurfSrc  = (PSURFOBJ)AccessUserObject((ULONG)DCSrc->Surface);

  SurfGDIDest = (PSURFGDI)AccessInternalObjectFromUserObject(SurfDest);
  SurfGDISrc  = (PSURFGDI)AccessInternalObjectFromUserObject(SurfSrc);

  if (DCDest->w.hPalette != 0)
    {
      DestPalette = DCDest->w.hPalette;
    }
  else
    {
      DestPalette = NtGdiGetStockObject(DEFAULT_PALETTE);
    }

  if(DCSrc->w.hPalette != 0)
    {
      SourcePalette = DCSrc->w.hPalette;
    }
  else
    {
      SourcePalette = NtGdiGetStockObject(DEFAULT_PALETTE);
    }

  PalSourceGDI = PALETTE_LockPalette(SourcePalette);
  SourceMode = PalSourceGDI->Mode;
  PALETTE_UnlockPalette(SourcePalette);
  if (DestPalette == SourcePalette)
    {
      DestMode = SourceMode;
    }
  else
    {
      PalDestGDI = PALETTE_LockPalette(DestPalette);
      DestMode = PalDestGDI->Mode;
      PALETTE_UnlockPalette(DestPalette);
    }

  XlateObj = (PXLATEOBJ)IntEngCreateXlate(DestMode, SourceMode, DestPalette, SourcePalette);

  // Perform the bitblt operation

  Status = IntEngBitBlt(SurfDest, SurfSrc, NULL, DCDest->CombinedClip, XlateObj, &DestRect, &SourcePoint, NULL, NULL, NULL, ROP);

  EngDeleteXlate(XlateObj);
  if (SurfDestAlloc) ExFreePool(SurfDest);
  if (SurfSrcAlloc) ExFreePool(SurfSrc);

  GDIOBJ_UnlockMultipleObj(Lock, sizeof(Lock) / sizeof(Lock[0]));

  return Status;
}

HBITMAP STDCALL NtGdiCreateBitmap(INT  Width,
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
	DPRINT("NtGdiCreateBitmap: BITMAPOBJ_AllocBitmap returned 0\n");
    return 0;
  }

  bmp = BITMAPOBJ_LockBitmap( hBitmap );

  DPRINT("NtGdiCreateBitmap:%dx%d, %d (%d BPP) colors returning %08x\n", Width, Height,
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

  BITMAPOBJ_UnlockBitmap( hBitmap );

  if (Bits) /* Set bitmap bits */
  {
    NtGdiSetBitmapBits(hBitmap, Height * bmp->bitmap.bmWidthBytes, Bits);
  }

  return  hBitmap;
}

BOOL FASTCALL Bitmap_InternalDelete( PBITMAPOBJ pBmp )
{
  ASSERT( pBmp );

  if (NULL != pBmp->bitmap.bmBits)
    {
      if (NULL != pBmp->dib)
	{
	  if (NULL == pBmp->dib->dshSection)
	    {
	      EngFreeUserMem(pBmp->bitmap.bmBits);
	    }
	  else
	    {
	      /* This is a file-mapped section */
	      UNIMPLEMENTED;
	    }
	}
      else
	{
	  ExFreePool(pBmp->bitmap.bmBits);
	}
    }

  return TRUE;
}


HBITMAP STDCALL NtGdiCreateCompatibleBitmap(HDC hDC,
                                    INT  Width,
                                    INT  Height)
{
  HBITMAP  hbmpRet;
  PDC  dc;

  hbmpRet = 0;
  dc = DC_LockDc(hDC);

  DPRINT("NtGdiCreateCompatibleBitmap(%04x,%d,%d, bpp:%d) = \n", hDC, Width, Height, dc->w.bitsPerPixel);

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
      hbmpRet = NtGdiCreateBitmap (1, 1, 1, 1, NULL);
    }
    else
    {
      hbmpRet = NtGdiCreateBitmap(Width, Height, 1, dc->w.bitsPerPixel, NULL);
    }
  }
  DPRINT ("\t\t%04x\n", hbmpRet);
  DC_UnlockDc( hDC );
  return hbmpRet;
}

HBITMAP STDCALL NtGdiCreateBitmapIndirect(CONST BITMAP  *BM)
{
  return NtGdiCreateBitmap (BM->bmWidth,
                           BM->bmHeight,
                           BM->bmPlanes,
                           BM->bmBitsPixel,
                           BM->bmBits);
}

HBITMAP STDCALL NtGdiCreateDiscardableBitmap(HDC  hDC,
                                     INT  Width,
                                     INT  Height)
{
  /* FIXME: this probably should do something else */
  return  NtGdiCreateCompatibleBitmap(hDC, Width, Height);
}

BOOL STDCALL NtGdiExtFloodFill(HDC  hDC,
                      INT  XStart,
                      INT  YStart,
                      COLORREF  Color,
                      UINT  FillType)
{
  UNIMPLEMENTED;
}

BOOL STDCALL NtGdiFloodFill(HDC  hDC,
                    INT  XStart,
                    INT  YStart,
                    COLORREF  Fill)
{
  UNIMPLEMENTED;
}

BOOL STDCALL NtGdiGetBitmapDimensionEx(HBITMAP  hBitmap,
                               LPSIZE  Dimension)
{
  PBITMAPOBJ  bmp;

  bmp = BITMAPOBJ_LockBitmap(hBitmap);
  if (bmp == NULL)
  {
    return FALSE;
  }

  *Dimension = bmp->size;

  BITMAPOBJ_UnlockBitmap(hBitmap);

  return  TRUE;
}

COLORREF STDCALL NtGdiGetPixel(HDC  hDC,
                       INT  XPos,
                       INT  YPos)
{
  PDC      dc = NULL;
  COLORREF cr = (COLORREF) 0;

  dc = DC_LockDc (hDC);
  if (NULL == dc)
  {
    return (COLORREF) CLR_INVALID;
  }
  //FIXME: get actual pixel RGB value
  DC_UnlockDc (hDC);
  return cr;
}

BOOL STDCALL NtGdiMaskBlt(HDC  hDCDest,
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

BOOL STDCALL NtGdiPlgBlt(HDC  hDCDest,
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

LONG STDCALL NtGdiSetBitmapBits(HBITMAP  hBitmap,
                        DWORD  Bytes,
                        CONST VOID *Bits)
{
  LONG height, ret;
  PBITMAPOBJ bmp;

  bmp = BITMAPOBJ_LockBitmap(hBitmap);
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

  BITMAPOBJ_UnlockBitmap(hBitmap);

  return ret;
}

BOOL STDCALL NtGdiSetBitmapDimensionEx(HBITMAP  hBitmap,
                               INT  Width,
                               INT  Height,
                               LPSIZE  Size)
{
  PBITMAPOBJ  bmp;

  bmp = BITMAPOBJ_LockBitmap(hBitmap);
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

  BITMAPOBJ_UnlockBitmap (hBitmap);

  return TRUE;
}

COLORREF STDCALL NtGdiSetPixel(HDC  hDC,
                       INT  X,
                       INT  Y,
                       COLORREF  Color)
{
   if(NtGdiSetPixelV(hDC,X,Y,Color))
   {
      COLORREF cr = NtGdiGetPixel(hDC,X,Y);
      if(CLR_INVALID != cr) return(cr);
   }
   return ((COLORREF) -1);
}

BOOL STDCALL NtGdiSetPixelV(HDC  hDC,
                    INT  X,
                    INT  Y,
                    COLORREF  Color)
{
  PDC        dc = NULL;
  PBITMAPOBJ bmp = NULL;
  BITMAP     bm;

  dc = DC_LockDc (hDC);
  if(NULL == dc)
  {
    return(FALSE);
  }
  bmp = BITMAPOBJ_LockBitmap (dc->w.hBitmap);
  if(NULL == bmp)
  {
    DC_UnlockDc (hDC);
    return(FALSE);
  }
  bm = bmp->bitmap;
  //FIXME: set the actual pixel value
  BITMAPOBJ_UnlockBitmap (dc->w.hBitmap);
  DC_UnlockDc (hDC);
  return(TRUE);
}

BOOL STDCALL NtGdiStretchBlt(HDC  hDCDest,
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

INT FASTCALL
BITMAPOBJ_GetWidthBytes (INT bmWidth, INT bpp)
{
#if 0
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
#endif

  return ((bmWidth * bpp + 31) & ~31) >> 3;
}

HBITMAP FASTCALL BITMAPOBJ_CopyBitmap(HBITMAP  hBitmap)
{
  PBITMAPOBJ  bmp;
  HBITMAP  res;
  BITMAP  bm;

  bmp = BITMAPOBJ_LockBitmap(hBitmap);
  if (bmp == NULL)
  {
    return 0;
  }
  res = 0;

  bm = bmp->bitmap;
  bm.bmBits = NULL;
  BITMAPOBJ_UnlockBitmap(hBitmap);
  res = NtGdiCreateBitmapIndirect(&bm);
  if(res)
  {
    char *buf;

    buf = ExAllocatePool (NonPagedPool, bm.bmWidthBytes * bm.bmHeight);
    NtGdiGetBitmapBits (hBitmap, bm.bmWidthBytes * bm.bmHeight, buf);
    NtGdiSetBitmapBits (res, bm.bmWidthBytes * bm.bmHeight, buf);
    ExFreePool (buf);
  }

  return  res;
}

INT STDCALL BITMAP_GetObject(BITMAPOBJ * bmp, INT count, LPVOID buffer)
{
  if(bmp->dib)
  {
    if(count < (INT) sizeof(DIBSECTION))
    {
      if (count > (INT) sizeof(BITMAP)) count = sizeof(BITMAP);
    }
    else
    {
      if (count > (INT) sizeof(DIBSECTION)) count = sizeof(DIBSECTION);
    }
    memcpy(buffer, bmp->dib, count);
    return count;
  }
  else
  {
    if (count > (INT) sizeof(BITMAP)) count = sizeof(BITMAP);
    memcpy(buffer, &bmp->bitmap, count);
    return count;
  }
}
/* EOF */
