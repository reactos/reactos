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
/* $Id: bitmaps.c,v 1.41 2003/10/22 17:44:01 gvg Exp $ */
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <win32k/gdiobj.h>
#include <win32k/bitmaps.h>
#include <win32k/brush.h>
//#include <win32k/debug.h>
#include "../eng/handle.h"
#include <include/inteng.h>
#include <include/eng.h>
#include <include/error.h>
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
  PDC DCDest = NULL;
  PDC DCSrc  = NULL;
  PSURFOBJ SurfDest, SurfSrc;
  PSURFGDI SurfGDIDest, SurfGDISrc;
  RECTL DestRect;
  POINTL SourcePoint;
  BOOL Status;
  PPALGDI PalDestGDI, PalSourceGDI;
  PXLATEOBJ XlateObj = NULL;
  HPALETTE SourcePalette, DestPalette;
  ULONG SourceMode, DestMode;
  PBRUSHOBJ BrushObj;
  BOOL UsesSource = ((ROP & 0xCC0000) >> 2) != (ROP & 0x330000);
  BOOL UsesPattern = ((ROP & 0xF00000) >> 4) != (ROP & 0x0F0000);  

  DCDest = DC_LockDc(hDCDest);
  if (NULL == DCDest)
    {
      DPRINT1("Invalid destination dc handle (0x%08x) passed to NtGdiBitBlt\n", hDCDest);
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }

  if (UsesSource)
    {
      DCSrc = DC_LockDc(hDCSrc);
      if (NULL == DCSrc)
        {
          DC_UnlockDc(hDCDest);
          DPRINT1("Invalid source dc handle (0x%08x) passed to NtGdiBitBlt\n", hDCSrc);
          SetLastWin32Error(ERROR_INVALID_HANDLE);
          return FALSE;
        }
    }
  else
    {
      DCSrc = NULL;
    }

  /* Offset the destination and source by the origin of their DCs. */
  XDest += DCDest->w.DCOrgX;
  YDest += DCDest->w.DCOrgY;
  if (UsesSource)
    {
      XSrc += DCSrc->w.DCOrgX;
      YSrc += DCSrc->w.DCOrgY;
    }

  DestRect.left   = XDest;
  DestRect.top    = YDest;
  DestRect.right  = XDest+Width;
  DestRect.bottom = YDest+Height;

  SourcePoint.x = XSrc;
  SourcePoint.y = YSrc;

  /* Determine surfaces to be used in the bitblt */
  SurfDest = (PSURFOBJ)AccessUserObject((ULONG)DCDest->Surface);
  SurfGDIDest = (PSURFGDI)AccessInternalObjectFromUserObject(SurfDest);
  if (UsesSource)
    {
      SurfSrc  = (PSURFOBJ)AccessUserObject((ULONG)DCSrc->Surface);
      SurfGDISrc  = (PSURFGDI)AccessInternalObjectFromUserObject(SurfSrc);
    }
  else
    {
      SurfSrc  = NULL;
      SurfGDISrc  = NULL;
    }

  if (UsesPattern)
    {
      BrushObj = BRUSHOBJ_LockBrush(DCDest->w.hBrush);
      if (NULL == BrushObj)
        {
          SetLastWin32Error(ERROR_INVALID_HANDLE);
          return FALSE;
        }
    }
  else
    {
      BrushObj = NULL;
    }

  if (DCDest->w.hPalette != 0)
    {
      DestPalette = DCDest->w.hPalette;
    }
  else
    {
      DestPalette = NtGdiGetStockObject(DEFAULT_PALETTE);
    }

  if (UsesSource && DCSrc->w.hPalette != 0)
    {
      SourcePalette = DCSrc->w.hPalette;
    }
  else
    {
      SourcePalette = NtGdiGetStockObject(DEFAULT_PALETTE);
    }

  PalSourceGDI = PALETTE_LockPalette(SourcePalette);
  if (NULL == PalSourceGDI)
    {
      if (UsesSource)
        {
          DC_UnlockDc(hDCSrc);
        }
      DC_UnlockDc(hDCDest);
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }
  SourceMode = PalSourceGDI->Mode;
  PALETTE_UnlockPalette(SourcePalette);

  if (DestPalette == SourcePalette)
    {
      DestMode = SourceMode;
    }
  else
    {
      PalDestGDI = PALETTE_LockPalette(DestPalette);
      if (NULL == PalDestGDI)
        {
          if (UsesSource)
            {
              DC_UnlockDc(hDCSrc);
            }
          DC_UnlockDc(hDCDest);
          SetLastWin32Error(ERROR_INVALID_HANDLE);
          return FALSE;
        }
      DestMode = PalDestGDI->Mode;
      PALETTE_UnlockPalette(DestPalette);
    }

  XlateObj = (PXLATEOBJ)IntEngCreateXlate(DestMode, SourceMode, DestPalette, SourcePalette);
  if (NULL == XlateObj)
    {
      if (UsesSource)
        {
          DC_UnlockDc(hDCSrc);
        }
      DC_UnlockDc(hDCDest);
      SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
      return FALSE;
    }

  /* Perform the bitblt operation */
  Status = IntEngBitBlt(SurfDest, SurfSrc, NULL, DCDest->CombinedClip, XlateObj,
                        &DestRect, &SourcePoint, NULL, BrushObj, NULL, ROP);

  EngDeleteXlate(XlateObj);
  if (UsesPattern)
    {
      BRUSHOBJ_UnlockBrush(DCDest->w.hBrush);
    }
  if (UsesSource)
    {
      DC_UnlockDc(hDCSrc);
    }
  DC_UnlockDc(hDCDest);

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
   COLORREF cr = NtGdiGetPixel(hDC,X,Y);
   if(cr != CLR_INVALID && NtGdiSetPixelV(hDC,X,Y,Color))
   {
      return(cr);
   }
   return ((COLORREF) -1);
}

BOOL STDCALL NtGdiSetPixelV(HDC  hDC,
                    INT  X,
                    INT  Y,
                    COLORREF  Color)
{
  HBRUSH NewBrush = NtGdiCreateSolidBrush(Color);
  HGDIOBJ OldBrush;
  
  if (NewBrush == NULL)
    return(FALSE);
  OldBrush = NtGdiSelectObject(hDC, NewBrush);
  if (OldBrush == NULL)
    return(FALSE);
  NtGdiPatBlt(hDC, X, Y, 1, 1, PATCOPY);
  NtGdiSelectObject(hDC, OldBrush);
  NtGdiDeleteObject(NewBrush);
  return TRUE;
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
