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
/* $Id$ */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

#define IN_RECT(r,x,y) \
( \
 (x) >= (r).left && \
 (y) >= (r).top && \
 (x) < (r).right && \
 (y) < (r).bottom \
)

BOOL STDCALL
NtGdiBitBlt(
	HDC  hDCDest,
	INT  XDest,
	INT  YDest,
	INT  Width,
	INT  Height,
	HDC  hDCSrc,
	INT  XSrc,
	INT  YSrc,
	DWORD  ROP,
	IN DWORD crBackColor,
	IN FLONG fl)
{
	PDC DCDest = NULL;
	PDC DCSrc  = NULL;
	BITMAPOBJ *BitmapDest, *BitmapSrc;
	RECTL DestRect;
	POINTL SourcePoint, BrushOrigin;
	BOOL Status;
	XLATEOBJ *XlateObj = NULL;
	HPALETTE SourcePalette = 0, DestPalette = 0;
	PGDIBRUSHOBJ BrushObj;
	GDIBRUSHINST BrushInst;
	BOOL UsesSource = ROP3_USES_SOURCE(ROP);
	BOOL UsesPattern = ROP3_USES_PATTERN(ROP);

	DCDest = DC_LockDc(hDCDest);
	if (NULL == DCDest)
	{
		DPRINT1("Invalid destination dc handle (0x%08x) passed to NtGdiBitBlt\n", hDCDest);
		SetLastWin32Error(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	if (DCDest->IsIC)
	{
		DC_UnlockDc(DCDest);
		/* Yes, Windows really returns TRUE in this case */
		return TRUE;
	}

	if (UsesSource)
	{
		if (hDCSrc != hDCDest)
		{
			DCSrc = DC_LockDc(hDCSrc);
			if (NULL == DCSrc)
			{
				DC_UnlockDc(DCDest);
				DPRINT1("Invalid source dc handle (0x%08x) passed to NtGdiBitBlt\n", hDCSrc);
				SetLastWin32Error(ERROR_INVALID_HANDLE);
				return FALSE;
			}
			if (DCSrc->IsIC)
			{
				DC_UnlockDc(DCSrc);
				DC_UnlockDc(DCDest);
				/* Yes, Windows really returns TRUE in this case */
				return TRUE;
			}
		}
		else
		{
			DCSrc = DCDest;
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

        IntLPtoDP(DCDest, (LPPOINT)&DestRect, 2);

	SourcePoint.x = XSrc;
	SourcePoint.y = YSrc;

	BrushOrigin.x = 0;
	BrushOrigin.y = 0;

	/* Determine surfaces to be used in the bitblt */
	BitmapDest = BITMAPOBJ_LockBitmap(DCDest->w.hBitmap);
	if (UsesSource)
	{
	        if (DCSrc->w.hBitmap == DCDest->w.hBitmap)
			BitmapSrc = BitmapDest;
		else
			BitmapSrc = BITMAPOBJ_LockBitmap(DCSrc->w.hBitmap);
	}
	else
	{
		BitmapSrc = NULL;
	}

	if (UsesPattern)
	{
		BrushObj = BRUSHOBJ_LockBrush(DCDest->w.hBrush);
		if (NULL == BrushObj)
		{
			if (UsesSource && hDCSrc != hDCDest)
			{
				DC_UnlockDc(DCSrc);
			}
			if(BitmapDest != NULL)
			{
                                BITMAPOBJ_UnlockBitmap(BitmapDest);
			}
			if(BitmapSrc != NULL && BitmapSrc != BitmapDest)
			{
                                BITMAPOBJ_UnlockBitmap(BitmapSrc);
			}
			DC_UnlockDc(DCDest);
			SetLastWin32Error(ERROR_INVALID_HANDLE);
			return FALSE;
		}
		BrushOrigin = *((PPOINTL)&BrushObj->ptOrigin);
		IntGdiInitBrushInstance(&BrushInst, BrushObj, DCDest->XlateBrush);
	}
	else
	{
		BrushObj = NULL;
	}

	/* Create the XLATEOBJ. */
	if (UsesSource)
	{
		if (DCDest->w.hPalette != 0)
			DestPalette = DCDest->w.hPalette;

		if (DCSrc->w.hPalette != 0)
			SourcePalette = DCSrc->w.hPalette;

		/* KB41464 details how to convert between mono and color */
		if (DCDest->w.bitsPerPixel == 1 && DCSrc->w.bitsPerPixel == 1)
		{
			XlateObj = NULL;
		}
		else
		{
			if (DCDest->w.bitsPerPixel == 1)
			{
				XlateObj = IntEngCreateMonoXlate(0, DestPalette, SourcePalette, DCSrc->w.backgroundColor);
			}
			else if (DCSrc->w.bitsPerPixel == 1)
			{
				XlateObj = IntEngCreateSrcMonoXlate(DestPalette, DCSrc->w.backgroundColor, DCSrc->w.textColor);
			}
			else
			{
				XlateObj = IntEngCreateXlate(0, 0, DestPalette, SourcePalette);
			}
			if (NULL == XlateObj)
			{
				if (UsesSource && hDCSrc != hDCDest)
				{
					DC_UnlockDc(DCSrc);
				}
				DC_UnlockDc(DCDest);
				if(BitmapDest != NULL)
				{
                                	BITMAPOBJ_UnlockBitmap(BitmapDest);
				}
				if(BitmapSrc != NULL && BitmapSrc != BitmapDest)
				{
                                	BITMAPOBJ_UnlockBitmap(BitmapSrc);
				}
				if(BrushObj != NULL)
				{
                                        BRUSHOBJ_UnlockBrush(BrushObj);
				}
				SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
				return FALSE;
			}
		}
	}

	/* Perform the bitblt operation */
	Status = IntEngBitBlt(&BitmapDest->SurfObj, &BitmapSrc->SurfObj, NULL,
                              DCDest->CombinedClip, XlateObj, &DestRect,
                              &SourcePoint, NULL,
                              BrushObj ? &BrushInst.BrushObject : NULL,
	                      &BrushOrigin, ROP3_TO_ROP4(ROP));

	if (UsesSource && XlateObj != NULL)
		EngDeleteXlate(XlateObj);

        if(BitmapDest != NULL)
        {
                BITMAPOBJ_UnlockBitmap(BitmapDest);
        }
	if (UsesSource && BitmapSrc != BitmapDest)
	{
		BITMAPOBJ_UnlockBitmap(BitmapSrc);
	}
	if (BrushObj != NULL)
	{
		BRUSHOBJ_UnlockBrush(BrushObj);
	}
	if (UsesSource && hDCSrc != hDCDest)
	{
		DC_UnlockDc(DCSrc);
	}
	DC_UnlockDc(DCDest);

	return Status;
}

BOOL STDCALL
NtGdiTransparentBlt(
	HDC			hdcDst,
	INT			xDst,
	INT			yDst,
	INT			cxDst,
	INT			cyDst,
	HDC			hdcSrc,
	INT			xSrc,
	INT			ySrc,
	INT			cxSrc,
	INT			cySrc,
	COLORREF	TransColor)
{
  PDC DCDest, DCSrc;
  RECTL rcDest, rcSrc;
  BITMAPOBJ *BitmapDest, *BitmapSrc;
  XLATEOBJ *XlateObj;
  HPALETTE SourcePalette = 0, DestPalette = 0;
  PPALGDI PalDestGDI, PalSourceGDI;
  USHORT PalDestMode, PalSrcMode;
  ULONG TransparentColor = 0;
  BOOL Ret = FALSE;

  if(!(DCDest = DC_LockDc(hdcDst)))
  {
    DPRINT1("Invalid destination dc handle (0x%08x) passed to NtGdiTransparentBlt\n", hdcDst);
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  if (DCDest->IsIC)
  {
    DC_UnlockDc(DCDest);
    /* Yes, Windows really returns TRUE in this case */
    return TRUE;
  }

  if((hdcDst != hdcSrc) && !(DCSrc = DC_LockDc(hdcSrc)))
  {
    DC_UnlockDc(DCDest);
    DPRINT1("Invalid source dc handle (0x%08x) passed to NtGdiTransparentBlt\n", hdcSrc);
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  if(hdcDst == hdcSrc)
  {
    DCSrc = DCDest;
  }
  if (DCSrc->IsIC)
  {
    DC_UnlockDc(DCSrc);
    if(hdcDst != hdcSrc)
    {
      DC_UnlockDc(DCDest);
    }
    /* Yes, Windows really returns TRUE in this case */
    return TRUE;
  }

  /* Offset positions */
  xDst += DCDest->w.DCOrgX;
  yDst += DCDest->w.DCOrgY;
  xSrc += DCSrc->w.DCOrgX;
  ySrc += DCSrc->w.DCOrgY;

  if(DCDest->w.hPalette)
    DestPalette = DCDest->w.hPalette;

  if(DCSrc->w.hPalette)
    SourcePalette = DCSrc->w.hPalette;

  if(!(PalSourceGDI = PALETTE_LockPalette(SourcePalette)))
  {
    DC_UnlockDc(DCSrc);
    DC_UnlockDc(DCDest);
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  if((DestPalette != SourcePalette) && !(PalDestGDI = PALETTE_LockPalette(DestPalette)))
  {
    PALETTE_UnlockPalette(PalSourceGDI);
    DC_UnlockDc(DCSrc);
    DC_UnlockDc(DCDest);
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  if(DestPalette != SourcePalette)
  {
    PalDestMode = PalDestGDI->Mode;
    PalSrcMode = PalSourceGDI->Mode;
    PALETTE_UnlockPalette(PalDestGDI);
  }
  else
  {
    PalDestMode = PalSrcMode = PalSourceGDI->Mode;
  }
  PALETTE_UnlockPalette(PalSourceGDI);

  /* Translate Transparent (RGB) Color to the source palette */
  if((XlateObj = (XLATEOBJ*)IntEngCreateXlate(PalSrcMode, PAL_RGB, SourcePalette, NULL)))
  {
    TransparentColor = XLATEOBJ_iXlate(XlateObj, (ULONG)TransColor);
    EngDeleteXlate(XlateObj);
  }

  /* Create the XLATE object to convert colors between source and destination */
  XlateObj = (XLATEOBJ*)IntEngCreateXlate(PalDestMode, PalSrcMode, DestPalette, SourcePalette);

  BitmapDest = BITMAPOBJ_LockBitmap(DCDest->w.hBitmap);
  /* FIXME - BitmapDest can be NULL!!!! Don't assert here! */
  ASSERT(BitmapDest);
  BitmapSrc = BITMAPOBJ_LockBitmap(DCSrc->w.hBitmap);
  /* FIXME - BitmapSrc can be NULL!!!! Don't assert here! */
  ASSERT(BitmapSrc);

  rcDest.left = xDst;
  rcDest.top = yDst;
  rcDest.right = rcDest.left + cxDst;
  rcDest.bottom = rcDest.top + cyDst;
  rcSrc.left = xSrc;
  rcSrc.top = ySrc;
  rcSrc.right = rcSrc.left + cxSrc;
  rcSrc.bottom = rcSrc.top + cySrc;

  if((cxDst != cxSrc) || (cyDst != cySrc))
  {
    DPRINT1("TransparentBlt() does not support stretching at the moment!\n");
    goto done;
  }

  Ret = IntEngTransparentBlt(&BitmapDest->SurfObj, &BitmapSrc->SurfObj,
                             DCDest->CombinedClip, XlateObj, &rcDest, &rcSrc,
                             TransparentColor, 0);

done:
  BITMAPOBJ_UnlockBitmap(BitmapDest);
  BITMAPOBJ_UnlockBitmap(BitmapSrc);
  DC_UnlockDc(DCSrc);
  if(hdcDst != hdcSrc)
  {
    DC_UnlockDc(DCDest);
  }
  if(XlateObj)
  {
    EngDeleteXlate(XlateObj);
  }
  return Ret;
}

static HBITMAP
IntCreateBitmapIndirect(CONST BITMAP *BM)
{
   PBITMAPOBJ bmp;
   HBITMAP hBitmap;
   SIZEL Size;
   UINT BitsPixel;

   /* NOTE: Windows also doesn't store nr. of planes separately! */
   BitsPixel = BM->bmBitsPixel * BM->bmPlanes;

   /* Check parameters */
   if (0 == BM->bmHeight || 0 == BM->bmWidth)
   {
      Size.cx = Size.cy = 1;
   }
   else
   {
      Size.cx = abs(BM->bmWidth);
      Size.cy = abs(BM->bmHeight);
   }

   /* Create the bitmap object. */
   hBitmap = IntCreateBitmap(Size, BM->bmWidthBytes,
                             BitmapFormat(BitsPixel, BI_RGB),
                             (BM->bmHeight < 0 ? BMF_TOPDOWN : 0) |
                             (NULL == BM->bmBits ? 0 : BMF_NOZEROINIT), NULL);
   if (!hBitmap)
   {
      DPRINT("NtGdiCreateBitmap: IntCreateBitmap returned 0\n");
      return 0;
   }

   DPRINT("NtGdiCreateBitmap:%dx%d, %d BPP colors returning %08x\n",
          Size.cx, Size.cy, BitsPixel, hBitmap);

   bmp = BITMAPOBJ_LockBitmap( hBitmap );
   /* FIXME - bmp can be NULL!!!!!! */
   bmp->flFlags = BITMAPOBJ_IS_APIBITMAP;
   BITMAPOBJ_UnlockBitmap( bmp );

   /*
    * NOTE: It's ugly practice, but we are using the object even
    * after unlocking. Since the handle is currently known only
    * to us it should be safe.
    */

   if (NULL != BM->bmBits)
   {
      NtGdiSetBitmapBits(hBitmap, bmp->SurfObj.cjBits, BM->bmBits);
   }

   return hBitmap;
}

HBITMAP STDCALL
NtGdiCreateBitmap(
	INT  Width,
	INT  Height,
	UINT  Planes,
	UINT  BitsPixel,
	IN OPTIONAL LPBYTE Bits)
{
   BITMAP BM;

   BM.bmType = 0;
   BM.bmWidth = Width;
   BM.bmHeight = Height;
   BM.bmWidthBytes = BITMAPOBJ_GetWidthBytes(Width, Planes * BitsPixel);
   BM.bmPlanes = Planes;
   BM.bmBitsPixel = BitsPixel;
   BM.bmBits = Bits;

   return IntCreateBitmapIndirect(&BM);
}

BOOL INTERNAL_CALL
BITMAP_Cleanup(PVOID ObjectBody)
{
        PBITMAPOBJ pBmp = (PBITMAPOBJ)ObjectBody;
	if (pBmp->SurfObj.pvBits != NULL &&
	    (pBmp->flFlags & BITMAPOBJ_IS_APIBITMAP))
	{
		if (pBmp->dib == NULL)
		{
			ExFreePool(pBmp->SurfObj.pvBits);
		}
		else
		{
			EngFreeUserMem(pBmp->SurfObj.pvBits);
		}
		if (pBmp->hDIBPalette)
		{
			NtGdiDeleteObject(pBmp->hDIBPalette);
		}
	}

	if (NULL != pBmp->BitsLock)
	{
		ExFreePoolWithTag(pBmp->BitsLock, TAG_BITMAPOBJ);
		pBmp->BitsLock = NULL;
	}

	return TRUE;
}


HBITMAP FASTCALL
IntCreateCompatibleBitmap(
	PDC Dc,
	INT Width,
	INT Height)
{
	HBITMAP Bmp;

	Bmp = NULL;

	if ((Width >= 0x10000) || (Height >= 0x10000))
	{
		DPRINT1("got bad width %d or height %d, please look for reason\n", Width, Height);
		return NULL;
	}

	/* MS doc says if width or height is 0, return 1-by-1 pixel, monochrome bitmap */
	if (0 == Width || 0 == Height)
	{
		Bmp = NtGdiCreateBitmap (1, 1, 1, 1, NULL);
	}
	else
	{
		Bmp = NtGdiCreateBitmap(Width, Height, 1, Dc->w.bitsPerPixel, NULL);
	}

	return Bmp;
}

HBITMAP STDCALL
NtGdiCreateCompatibleBitmap(
	HDC hDC,
	INT Width,
	INT Height)
{
	HBITMAP Bmp;
	PDC Dc;

	Dc = DC_LockDc(hDC);

	DPRINT("NtGdiCreateCompatibleBitmap(%04x,%d,%d, bpp:%d) = \n", hDC, Width, Height, Dc->w.bitsPerPixel);

	if (NULL == Dc)
	{
		SetLastWin32Error(ERROR_INVALID_HANDLE);
		return NULL;
	}

	Bmp = IntCreateCompatibleBitmap(Dc, Width, Height);

	DPRINT ("\t\t%04x\n", Bmp);
	DC_UnlockDc(Dc);
	return Bmp;
}

HBITMAP STDCALL
NtGdiCreateBitmapIndirect(CONST BITMAP *UnsafeBM)
{
  BITMAP BM;
  NTSTATUS Status = STATUS_SUCCESS;

  _SEH_TRY
  {
    ProbeForRead(UnsafeBM, sizeof(BITMAP), 1);
    BM = *UnsafeBM;
    if (NULL != BM.bmBits)
      {
      ProbeForRead(BM.bmBits, BM.bmWidthBytes * abs(BM.bmHeight), 2);
      }
  }
  _SEH_HANDLE
  {
    Status = _SEH_GetExceptionCode();
  }
  _SEH_END;
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return NULL;
  }

  return IntCreateBitmapIndirect(&BM);
}

HBITMAP STDCALL
NtGdiCreateDiscardableBitmap(
	HDC  hDC,
	INT  Width,
	INT  Height)
{
	/* FIXME: this probably should do something else */
	return  NtGdiCreateCompatibleBitmap(hDC, Width, Height);
}

BOOL STDCALL
NtGdiExtFloodFill(
	HDC  hDC,
	INT  XStart,
	INT  YStart,
	COLORREF  Color,
	UINT  FillType)
{
   DPRINT1("FIXME: NtGdiExtFloodFill is UNIMPLEMENTED\n");

   /* lie and say we succeded */
	return TRUE;
}

BOOL STDCALL
NtGdiFloodFill(
	HDC  hDC,
	INT  XStart,
	INT  YStart,
	COLORREF  Fill)
{
	return NtGdiExtFloodFill(hDC, XStart, YStart, Fill, FLOODFILLBORDER );
}

BOOL STDCALL
NtGdiGetBitmapDimensionEx(
	HBITMAP  hBitmap,
	LPSIZE  Dimension)
{
	PBITMAPOBJ  bmp;

	bmp = BITMAPOBJ_LockBitmap(hBitmap);
	if (bmp == NULL)
	{
		return FALSE;
	}

	*Dimension = bmp->dimension;

	BITMAPOBJ_UnlockBitmap(bmp);

	return  TRUE;
}

COLORREF STDCALL
NtGdiGetPixel(HDC hDC, INT XPos, INT YPos)
{
	PDC dc = NULL;
	COLORREF Result = (COLORREF)CLR_INVALID; // default to failure
	BOOL bInRect = FALSE;
	BITMAPOBJ *BitmapObject;
	SURFOBJ *SurfaceObject;
	HPALETTE Pal = 0;
	XLATEOBJ *XlateObj;
    HBITMAP hBmpTmp;

	dc = DC_LockDc (hDC);

	if ( !dc )
	{
		SetLastWin32Error(ERROR_INVALID_HANDLE);
		return Result;
	}
	if (dc->IsIC)
	{
		DC_UnlockDc(dc);
		return Result;
	}
	XPos += dc->w.DCOrgX;
	YPos += dc->w.DCOrgY;
	if ( IN_RECT(dc->CombinedClip->rclBounds,XPos,YPos) )
	{
		bInRect = TRUE;
		BitmapObject = BITMAPOBJ_LockBitmap(dc->w.hBitmap);
		SurfaceObject = &BitmapObject->SurfObj;
		if ( BitmapObject )
		{
			if ( dc->w.hPalette != 0 )
				Pal = dc->w.hPalette;
			/* FIXME: Verify if it shouldn't be PAL_BGR! */
			XlateObj = (XLATEOBJ*)IntEngCreateXlate ( PAL_RGB, 0, NULL, Pal );
			if ( XlateObj )
			{
				// check if this DC has a DIB behind it...
				if ( SurfaceObject->pvScan0 ) // STYPE_BITMAP == SurfaceObject->iType
				{
					ASSERT ( SurfaceObject->lDelta );
					Result = XLATEOBJ_iXlate(XlateObj,
						DibFunctionsForBitmapFormat[SurfaceObject->iBitmapFormat].DIB_GetPixel ( SurfaceObject, XPos, YPos ) );
				}
				EngDeleteXlate(XlateObj);
			}
			BITMAPOBJ_UnlockBitmap(BitmapObject);
		}
	}
	DC_UnlockDc(dc);

	// if Result is still CLR_INVALID, then the "quick" method above didn't work
	if ( bInRect && Result == CLR_INVALID )
	{
		// FIXME: create a 1x1 32BPP DIB, and blit to it
		HDC hDCTmp = NtGdiCreateCompatibleDC(hDC);
		if ( hDCTmp )
		{
			static const BITMAPINFOHEADER bih = { sizeof(BITMAPINFOHEADER), 1, 1, 1, 32, BI_RGB, 0, 0, 0, 0, 0 };
			BITMAPINFO bi;
			RtlMoveMemory ( &(bi.bmiHeader), &bih, sizeof(bih) );
			hBmpTmp = NtGdiCreateDIBitmap ( hDC, &bi.bmiHeader, 0, NULL, &bi, DIB_RGB_COLORS );
			//HBITMAP hBmpTmp = NtGdiCreateBitmap ( 1, 1, 1, 32, NULL);
			if ( hBmpTmp )
			{
				HBITMAP hBmpOld = (HBITMAP)NtGdiSelectObject ( hDCTmp, hBmpTmp );
				if ( hBmpOld )
				{
					PBITMAPOBJ bmpobj;

					NtGdiBitBlt ( hDCTmp, 0, 0, 1, 1, hDC, XPos, YPos, SRCCOPY, 0, 0 );
					NtGdiSelectObject ( hDCTmp, hBmpOld );

					// our bitmap is no longer selected, so we can access it's stuff...
					bmpobj = BITMAPOBJ_LockBitmap ( hBmpTmp );
					if ( bmpobj )
					{
						Result = *(COLORREF*)bmpobj->SurfObj.pvScan0;
						BITMAPOBJ_UnlockBitmap ( bmpobj );
					}
				}
				NtGdiDeleteObject ( hBmpTmp );
			}
			NtGdiDeleteObjectApp ( hDCTmp );
		}
	}

	return Result;
}

/***********************************************************************
 * MaskBlt
 * Ported from WINE by sedwards 11-4-03
 *
 * Someone thought it would be faster to do it here and then switch back
 * to GDI32. I dunno. Write a test and let me know.
 */

static __inline BYTE
SwapROP3_SrcDst(BYTE bRop3)
{
	return (bRop3 & 0x99) | ((bRop3 & 0x22) << 1) | ((bRop3 & 0x44) >> 1);
}

#define FRGND_ROP3(ROP4)	((ROP4) & 0x00FFFFFF)
#define BKGND_ROP3(ROP4)	(ROP3Table[(SwapROP3_SrcDst((ROP4)>>24)) & 0xFF])
#define DSTCOPY 		0x00AA0029
#define DSTERASE		0x00220326 /* dest = dest & (~src) : DSna */

BOOL STDCALL
NtGdiMaskBlt (
	HDC hdcDest, INT nXDest, INT nYDest,
	INT nWidth, INT nHeight, HDC hdcSrc,
	INT nXSrc, INT nYSrc, HBITMAP hbmMask,
	INT xMask, INT yMask, DWORD dwRop,
	IN DWORD crBackColor)
{
	HBITMAP hOldMaskBitmap, hBitmap2, hOldBitmap2, hBitmap3, hOldBitmap3;
	HDC hDCMask, hDC1, hDC2;
	static const DWORD ROP3Table[256] =
	{
		0x00000042, 0x00010289,
		0x00020C89, 0x000300AA,
		0x00040C88, 0x000500A9,
		0x00060865, 0x000702C5,
		0x00080F08, 0x00090245,
		0x000A0329, 0x000B0B2A,
		0x000C0324, 0x000D0B25,
		0x000E08A5, 0x000F0001,
		0x00100C85, 0x001100A6,
		0x00120868, 0x001302C8,
		0x00140869, 0x001502C9,
		0x00165CCA, 0x00171D54,
		0x00180D59, 0x00191CC8,
		0x001A06C5, 0x001B0768,
		0x001C06CA, 0x001D0766,
		0x001E01A5, 0x001F0385,
		0x00200F09, 0x00210248,
		0x00220326, 0x00230B24,
		0x00240D55, 0x00251CC5,
		0x002606C8, 0x00271868,
		0x00280369, 0x002916CA,
		0x002A0CC9, 0x002B1D58,
		0x002C0784, 0x002D060A,
		0x002E064A, 0x002F0E2A,
		0x0030032A, 0x00310B28,
		0x00320688, 0x00330008,
		0x003406C4, 0x00351864,
		0x003601A8, 0x00370388,
		0x0038078A, 0x00390604,
		0x003A0644, 0x003B0E24,
		0x003C004A, 0x003D18A4,
		0x003E1B24, 0x003F00EA,
		0x00400F0A, 0x00410249,
		0x00420D5D, 0x00431CC4,
		0x00440328, 0x00450B29,
		0x004606C6, 0x0047076A,
		0x00480368, 0x004916C5,
		0x004A0789, 0x004B0605,
		0x004C0CC8, 0x004D1954,
		0x004E0645, 0x004F0E25,
		0x00500325, 0x00510B26,
		0x005206C9, 0x00530764,
		0x005408A9, 0x00550009,
		0x005601A9, 0x00570389,
		0x00580785, 0x00590609,
		0x005A0049, 0x005B18A9,
		0x005C0649, 0x005D0E29,
		0x005E1B29, 0x005F00E9,
		0x00600365, 0x006116C6,
		0x00620786, 0x00630608,
		0x00640788, 0x00650606,
		0x00660046, 0x006718A8,
		0x006858A6, 0x00690145,
		0x006A01E9, 0x006B178A,
		0x006C01E8, 0x006D1785,
		0x006E1E28, 0x006F0C65,
		0x00700CC5, 0x00711D5C,
		0x00720648, 0x00730E28,
		0x00740646, 0x00750E26,
		0x00761B28, 0x007700E6,
		0x007801E5, 0x00791786,
		0x007A1E29, 0x007B0C68,
		0x007C1E24, 0x007D0C69,
		0x007E0955, 0x007F03C9,
		0x008003E9, 0x00810975,
		0x00820C49, 0x00831E04,
		0x00840C48, 0x00851E05,
		0x008617A6, 0x008701C5,
		0x008800C6, 0x00891B08,
		0x008A0E06, 0x008B0666,
		0x008C0E08, 0x008D0668,
		0x008E1D7C, 0x008F0CE5,
		0x00900C45, 0x00911E08,
		0x009217A9, 0x009301C4,
		0x009417AA, 0x009501C9,
		0x00960169, 0x0097588A,
		0x00981888, 0x00990066,
		0x009A0709, 0x009B07A8,
		0x009C0704, 0x009D07A6,
		0x009E16E6, 0x009F0345,
		0x00A000C9, 0x00A11B05,
		0x00A20E09, 0x00A30669,
		0x00A41885, 0x00A50065,
		0x00A60706, 0x00A707A5,
		0x00A803A9, 0x00A90189,
		0x00AA0029, 0x00AB0889,
		0x00AC0744, 0x00AD06E9,
		0x00AE0B06, 0x00AF0229,
		0x00B00E05, 0x00B10665,
		0x00B21974, 0x00B30CE8,
		0x00B4070A, 0x00B507A9,
		0x00B616E9, 0x00B70348,
		0x00B8074A, 0x00B906E6,
		0x00BA0B09, 0x00BB0226,
		0x00BC1CE4, 0x00BD0D7D,
		0x00BE0269, 0x00BF08C9,
		0x00C000CA, 0x00C11B04,
		0x00C21884, 0x00C3006A,
		0x00C40E04, 0x00C50664,
		0x00C60708, 0x00C707AA,
		0x00C803A8, 0x00C90184,
		0x00CA0749, 0x00CB06E4,
		0x00CC0020, 0x00CD0888,
		0x00CE0B08, 0x00CF0224,
		0x00D00E0A, 0x00D1066A,
		0x00D20705, 0x00D307A4,
		0x00D41D78, 0x00D50CE9,
		0x00D616EA, 0x00D70349,
		0x00D80745, 0x00D906E8,
		0x00DA1CE9, 0x00DB0D75,
		0x00DC0B04, 0x00DD0228,
		0x00DE0268, 0x00DF08C8,
		0x00E003A5, 0x00E10185,
		0x00E20746, 0x00E306EA,
		0x00E40748, 0x00E506E5,
		0x00E61CE8, 0x00E70D79,
		0x00E81D74, 0x00E95CE6,
		0x00EA02E9, 0x00EB0849,
		0x00EC02E8, 0x00ED0848,
		0x00EE0086, 0x00EF0A08,
		0x00F00021, 0x00F10885,
		0x00F20B05, 0x00F3022A,
		0x00F40B0A, 0x00F50225,
		0x00F60265, 0x00F708C5,
		0x00F802E5, 0x00F90845,
		0x00FA0089, 0x00FB0A09,
		0x00FC008A, 0x00FD0A0A,
		0x00FE02A9, 0x00FF0062,
	};

	if (!hbmMask)
		return NtGdiBitBlt(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, FRGND_ROP3(dwRop), 0, 0);

	/* 1. make mask bitmap's dc */
	hDCMask = NtGdiCreateCompatibleDC(hdcDest);
	hOldMaskBitmap = (HBITMAP)NtGdiSelectObject(hDCMask, hbmMask);

	/* 2. make masked Background bitmap */

	/* 2.1 make bitmap */
	hDC1 = NtGdiCreateCompatibleDC(hdcDest);
	hBitmap2 = NtGdiCreateCompatibleBitmap(hdcDest, nWidth, nHeight);
	hOldBitmap2 = (HBITMAP)NtGdiSelectObject(hDC1, hBitmap2);

	/* 2.2 draw dest bitmap and mask */
	NtGdiBitBlt(hDC1, 0, 0, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, SRCCOPY, 0, 0);
	NtGdiBitBlt(hDC1, 0, 0, nWidth, nHeight, hdcDest, nXDest, nYDest, BKGND_ROP3(dwRop), 0, 0);
	NtGdiBitBlt(hDC1, 0, 0, nWidth, nHeight, hDCMask, xMask, yMask, DSTERASE, 0, 0);

	/* 3. make masked Foreground bitmap */

	/* 3.1 make bitmap */
	hDC2 = NtGdiCreateCompatibleDC(hdcDest);
	hBitmap3 = NtGdiCreateCompatibleBitmap(hdcDest, nWidth, nHeight);
	hOldBitmap3 = (HBITMAP)NtGdiSelectObject(hDC2, hBitmap3);

	/* 3.2 draw src bitmap and mask */
	NtGdiBitBlt(hDC2, 0, 0, nWidth, nHeight, hdcDest, nXDest, nYDest, SRCCOPY, 0, 0);
	NtGdiBitBlt(hDC2, 0, 0, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, FRGND_ROP3(dwRop), 0,0);
	NtGdiBitBlt(hDC2, 0, 0, nWidth, nHeight, hDCMask, xMask, yMask, SRCAND, 0, 0);

	/* 4. combine two bitmap and copy it to hdcDest */
	NtGdiBitBlt(hDC1, 0, 0, nWidth, nHeight, hDC2, 0, 0, SRCPAINT, 0, 0);
	NtGdiBitBlt(hdcDest, nXDest, nYDest, nWidth, nHeight, hDC1, 0, 0, SRCCOPY, 0, 0);

	/* 5. restore all object */
	NtGdiSelectObject(hDCMask, hOldMaskBitmap);
	NtGdiSelectObject(hDC1, hOldBitmap2);
	NtGdiSelectObject(hDC2, hOldBitmap3);

	/* 6. delete all temp object */
	NtGdiDeleteObject(hBitmap2);
	NtGdiDeleteObject(hBitmap3);

	NtGdiDeleteObjectApp(hDC1);
	NtGdiDeleteObjectApp(hDC2);
	NtGdiDeleteObjectApp(hDCMask);

	return TRUE;
}

BOOL
APIENTRY
NtGdiPlgBlt(
    IN HDC hdcTrg,
    IN LPPOINT pptlTrg,
    IN HDC hdcSrc,
    IN INT xSrc,
    IN INT ySrc,
    IN INT cxSrc,
    IN INT cySrc,
    IN HBITMAP hbmMask,
    IN INT xMask,
    IN INT yMask,
    IN DWORD crBackColor)
{
	UNIMPLEMENTED;
	return FALSE;
}

LONG STDCALL
NtGdiSetBitmapBits(
	HBITMAP  hBitmap,
	DWORD  Bytes,
	IN PBYTE Bits)
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
	height = Bytes / abs(bmp->SurfObj.lDelta);
	if (height > bmp->SurfObj.sizlBitmap.cy)
	{
		height = bmp->SurfObj.sizlBitmap.cy;
	}
	Bytes = height * abs(bmp->SurfObj.lDelta);
	DPRINT ("(%08x, bytes:%ld, bits:%p) %dx%d %d colors fetched height: %ld\n",
		hBitmap,
		Bytes,
		Bits,
		bmp->SurfObj.sizlBitmap.cx,
		bmp->SurfObj.sizlBitmap.cy,
		1 << BitsPerFormat(bmp->SurfObj.iBitmapFormat),
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
		memcpy(bmp->SurfObj.pvBits, Bits, Bytes);
		ret = Bytes;
	}

	BITMAPOBJ_UnlockBitmap(bmp);

	return ret;
}

BOOL STDCALL
NtGdiSetBitmapDimensionEx(
	HBITMAP  hBitmap,
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
		*Size = bmp->dimension;
	}
	bmp->dimension.cx = Width;
	bmp->dimension.cy = Height;

	BITMAPOBJ_UnlockBitmap (bmp);

	return TRUE;
}

COLORREF STDCALL
NtGdiSetPixel(
	HDC  hDC,
	INT  X,
	INT  Y,
	COLORREF  Color)
{
	if (NtGdiSetPixelV(hDC,X,Y,Color))
	{
		return NtGdiGetPixel(hDC,X,Y);
	}
	return ((COLORREF) -1);
}

BOOL STDCALL
NtGdiSetPixelV(
	HDC  hDC,
	INT  X,
	INT  Y,
	COLORREF  Color)
{
	HBRUSH NewBrush = NtGdiCreateSolidBrush(Color, NULL);
	HGDIOBJ OldBrush;

	if (NewBrush == NULL)
		return(FALSE);
	OldBrush = NtGdiSelectObject(hDC, NewBrush);
	if (OldBrush == NULL)
	{
		NtGdiDeleteObject(NewBrush);
		return(FALSE);
	}
	NtGdiPatBlt(hDC, X, Y, 1, 1, PATCOPY);
	NtGdiSelectObject(hDC, OldBrush);
	NtGdiDeleteObject(NewBrush);
	return TRUE;
}

BOOL STDCALL
NtGdiStretchBlt(
	HDC  hDCDest,
	INT  XOriginDest,
	INT  YOriginDest,
	INT  WidthDest,
	INT  HeightDest,
	HDC  hDCSrc,
	INT  XOriginSrc,
	INT  YOriginSrc,
	INT  WidthSrc,
	INT  HeightSrc,
	DWORD  ROP,
	IN DWORD dwBackColor)
{
	PDC DCDest = NULL;
	PDC DCSrc  = NULL;
	BITMAPOBJ *BitmapDest, *BitmapSrc;
	RECTL DestRect;
	RECTL SourceRect;
	BOOL Status;
	XLATEOBJ *XlateObj = NULL;
	HPALETTE SourcePalette = 0, DestPalette = 0;
	PGDIBRUSHOBJ BrushObj;
	BOOL UsesSource = ((ROP & 0xCC0000) >> 2) != (ROP & 0x330000);
	BOOL UsesPattern = ((ROP & 0xF00000) >> 4) != (ROP & 0x0F0000);

	if (0 == WidthDest || 0 == HeightDest || 0 == WidthSrc || 0 == HeightSrc)
	{
		SetLastWin32Error(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	DCDest = DC_LockDc(hDCDest);
	if (NULL == DCDest)
	{
		DPRINT1("Invalid destination dc handle (0x%08x) passed to NtGdiStretchBlt\n", hDCDest);
		SetLastWin32Error(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	if (DCDest->IsIC)
	{
		DC_UnlockDc(DCDest);
		/* Yes, Windows really returns TRUE in this case */
		return TRUE;
	}

	if (UsesSource)
	{
		if (hDCSrc != hDCDest)
		{
			DCSrc = DC_LockDc(hDCSrc);
			if (NULL == DCSrc)
			{
				DC_UnlockDc(DCDest);
				DPRINT1("Invalid source dc handle (0x%08x) passed to NtGdiStretchBlt\n", hDCSrc);
				SetLastWin32Error(ERROR_INVALID_HANDLE);
				return FALSE;
			}
			if (DCSrc->IsIC)
			{
				DC_UnlockDc(DCSrc);
				DC_UnlockDc(DCDest);
				/* Yes, Windows really returns TRUE in this case */
				return TRUE;
			}
		}
		else
		{
			DCSrc = DCDest;
		}
	}
	else
	{
		DCSrc = NULL;
	}

	/* Offset the destination and source by the origin of their DCs. */
	XOriginDest += DCDest->w.DCOrgX;
	YOriginDest += DCDest->w.DCOrgY;
	if (UsesSource)
	{
		XOriginSrc += DCSrc->w.DCOrgX;
		YOriginSrc += DCSrc->w.DCOrgY;
	}

	DestRect.left   = XOriginDest;
	DestRect.top    = YOriginDest;
	DestRect.right  = XOriginDest+WidthDest;
	DestRect.bottom = YOriginDest+HeightDest;

	SourceRect.left   = XOriginSrc;
	SourceRect.top    = YOriginSrc;
	SourceRect.right  = XOriginSrc+WidthSrc;
	SourceRect.bottom = YOriginSrc+HeightSrc;

	/* Determine surfaces to be used in the bitblt */
	BitmapDest = BITMAPOBJ_LockBitmap(DCDest->w.hBitmap);
	if (UsesSource)
	{
		if (DCSrc->w.hBitmap == DCDest->w.hBitmap)
			BitmapSrc = BitmapDest;
		else
			BitmapSrc = BITMAPOBJ_LockBitmap(DCSrc->w.hBitmap);
	}
	else
	{
		BitmapSrc = NULL;
	}

	if ( UsesSource )
	{
		int sw = BitmapSrc->SurfObj.sizlBitmap.cx;
		int sh = BitmapSrc->SurfObj.sizlBitmap.cy;
		if ( SourceRect.left < 0 )
		{
			DestRect.left = DestRect.right - (DestRect.right-DestRect.left) * (SourceRect.right)/abs(SourceRect.right-SourceRect.left);
			SourceRect.left = 0;
		}
		if ( SourceRect.top < 0 )
		{
			DestRect.top = DestRect.bottom - (DestRect.bottom-DestRect.top) * (SourceRect.bottom)/abs(SourceRect.bottom-SourceRect.top);
			SourceRect.top = 0;
		}
		if ( SourceRect.right < -1 )
		{
			DestRect.right = DestRect.left + (DestRect.right-DestRect.left) * (-1-SourceRect.left)/abs(SourceRect.right-SourceRect.left);
			SourceRect.right = -1;
		}
		if ( SourceRect.bottom < -1 )
		{
			DestRect.bottom = DestRect.top + (DestRect.bottom-DestRect.top) * (-1-SourceRect.top)/abs(SourceRect.bottom-SourceRect.top);
			SourceRect.bottom = -1;
		}
		if ( SourceRect.right > sw )
		{
			DestRect.right = DestRect.left + (DestRect.right-DestRect.left) * abs(sw-SourceRect.left) / abs(SourceRect.right-SourceRect.left);
			SourceRect.right = sw;
		}
		if ( SourceRect.bottom > sh )
		{
			DestRect.bottom = DestRect.top + (DestRect.bottom-DestRect.top) * abs(sh-SourceRect.top) / abs(SourceRect.bottom-SourceRect.top);
			SourceRect.bottom = sh;
		}
		sw--;
		sh--;
		if ( SourceRect.left > sw )
		{
			DestRect.left = DestRect.right - (DestRect.right-DestRect.left) * (SourceRect.right-sw) / abs(SourceRect.right-SourceRect.left);
			SourceRect.left = 0;
		}
		if ( SourceRect.top > sh )
		{
			DestRect.top = DestRect.bottom - (DestRect.bottom-DestRect.top) * (SourceRect.bottom-sh) / abs(SourceRect.bottom-SourceRect.top);
			SourceRect.top = 0;
		}
		if (0 == (DestRect.right-DestRect.left) || 0 == (DestRect.bottom-DestRect.top) || 0 == (SourceRect.right-SourceRect.left) || 0 == (SourceRect.bottom-SourceRect.top))
		{
			SetLastWin32Error(ERROR_INVALID_PARAMETER);
			Status = FALSE;
			goto failed;
		}
	}

	if (UsesPattern)
	{
		BrushObj = BRUSHOBJ_LockBrush(DCDest->w.hBrush);
		if (NULL == BrushObj)
		{
			if (UsesSource && hDCSrc != hDCDest)
			{
				DC_UnlockDc(DCSrc);
			}
			DC_UnlockDc(DCDest);
			SetLastWin32Error(ERROR_INVALID_HANDLE);
			return FALSE;
		}
	}
	else
	{
		BrushObj = NULL;
	}

	/* Create the XLATEOBJ. */
	if (UsesSource)
	{
		if (DCDest->w.hPalette != 0)
			DestPalette = DCDest->w.hPalette;

		if (DCSrc->w.hPalette != 0)
			SourcePalette = DCSrc->w.hPalette;

		/* FIXME: Use the same logic for create XLATEOBJ as in NtGdiBitBlt. */
		XlateObj = (XLATEOBJ*)IntEngCreateXlate(0, 0, DestPalette, SourcePalette);
		if (NULL == XlateObj)
		{
			if (UsesSource && hDCSrc != hDCDest)
			{
				DC_UnlockDc(DCSrc);
			}
			DC_UnlockDc(DCDest);
			SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
			return FALSE;
		}
	}

	/* Perform the bitblt operation */
	Status = IntEngStretchBlt(&BitmapDest->SurfObj, &BitmapSrc->SurfObj,
                                  NULL, DCDest->CombinedClip, XlateObj,
                                  &DestRect, &SourceRect, NULL, NULL, NULL,
                                  COLORONCOLOR);

	if (UsesSource)
		EngDeleteXlate(XlateObj);
	if (UsesPattern)
	{
		BRUSHOBJ_UnlockBrush(BrushObj);
	}
failed:
	if (UsesSource && DCSrc->w.hBitmap != DCDest->w.hBitmap)
	{
		BITMAPOBJ_UnlockBitmap(BitmapSrc);
	}
	BITMAPOBJ_UnlockBitmap(BitmapDest);
	if (UsesSource && hDCSrc != hDCDest)
	{
		DC_UnlockDc(DCSrc);
	}
	DC_UnlockDc(DCDest);

	return Status;
}

BOOL STDCALL
NtGdiAlphaBlend(
	HDC  hDCDest,
	LONG  XOriginDest,
	LONG  YOriginDest,
	LONG  WidthDest,
	LONG  HeightDest,
	HDC  hDCSrc,
	LONG  XOriginSrc,
	LONG  YOriginSrc,
	LONG  WidthSrc,
	LONG  HeightSrc,
	BLENDFUNCTION  BlendFunc,
	IN HANDLE hcmXform)
{
	PDC DCDest = NULL;
	PDC DCSrc  = NULL;
	BITMAPOBJ *BitmapDest, *BitmapSrc;
	RECTL DestRect, SourceRect;
	BOOL Status;
	XLATEOBJ *XlateObj;
	BLENDOBJ BlendObj;
	HPALETTE SourcePalette = 0, DestPalette = 0;
    BlendObj.BlendFunction = BlendFunc;

	DCDest = DC_LockDc(hDCDest);
	if (NULL == DCDest)
	{
		DPRINT1("Invalid destination dc handle (0x%08x) passed to NtGdiAlphaBlend\n", hDCDest);
		SetLastWin32Error(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	if (DCDest->IsIC)
	{
		DC_UnlockDc(DCDest);
		/* Yes, Windows really returns TRUE in this case */
		return TRUE;
	}

	if (hDCSrc != hDCDest)
	{
		DCSrc = DC_LockDc(hDCSrc);
		if (NULL == DCSrc)
		{
			DC_UnlockDc(DCDest);
			DPRINT1("Invalid source dc handle (0x%08x) passed to NtGdiAlphaBlend\n", hDCSrc);
			SetLastWin32Error(ERROR_INVALID_HANDLE);
			return FALSE;
		}
		if (DCSrc->IsIC)
		{
			DC_UnlockDc(DCSrc);
			DC_UnlockDc(DCDest);
			/* Yes, Windows really returns TRUE in this case */
			return TRUE;
		}
	}
	else
	{
		DCSrc = DCDest;
	}

	/* Offset the destination and source by the origin of their DCs. */
	XOriginDest += DCDest->w.DCOrgX;
	YOriginDest += DCDest->w.DCOrgY;
	XOriginSrc += DCSrc->w.DCOrgX;
	YOriginSrc += DCSrc->w.DCOrgY;

	DestRect.left   = XOriginDest;
	DestRect.top    = YOriginDest;
	DestRect.right  = XOriginDest + WidthDest;
	DestRect.bottom = YOriginDest + HeightDest;

	SourceRect.left   = XOriginSrc;
	SourceRect.top    = YOriginSrc;
	SourceRect.right  = XOriginSrc + WidthSrc;
	SourceRect.bottom = YOriginSrc + HeightSrc;

	/* Determine surfaces to be used in the bitblt */
	BitmapDest = BITMAPOBJ_LockBitmap(DCDest->w.hBitmap);
	if (DCSrc->w.hBitmap == DCDest->w.hBitmap)
		BitmapSrc = BitmapDest;
	else
		BitmapSrc = BITMAPOBJ_LockBitmap(DCSrc->w.hBitmap);

	/* Create the XLATEOBJ. */
	if (DCDest->w.hPalette != 0)
		DestPalette = DCDest->w.hPalette;
	if (DCSrc->w.hPalette != 0)
		SourcePalette = DCSrc->w.hPalette;

	/* KB41464 details how to convert between mono and color */
	if (DCDest->w.bitsPerPixel == 1 && DCSrc->w.bitsPerPixel == 1)
	{
		XlateObj = NULL;
	}
	else
	{
		if (DCDest->w.bitsPerPixel == 1)
		{
			XlateObj = IntEngCreateMonoXlate(0, DestPalette, SourcePalette, DCSrc->w.backgroundColor);
		}
		else if (DCSrc->w.bitsPerPixel == 1)
		{
			XlateObj = IntEngCreateSrcMonoXlate(DestPalette, DCSrc->w.backgroundColor, DCSrc->w.textColor);
		}
		else
		{
			XlateObj = IntEngCreateXlate(0, 0, DestPalette, SourcePalette);
		}
		if (NULL == XlateObj)
		{
			BITMAPOBJ_UnlockBitmap(BitmapDest);
			if (BitmapSrc != BitmapDest)
				BITMAPOBJ_UnlockBitmap(BitmapSrc);
			DC_UnlockDc(DCDest);
			if (hDCSrc != hDCDest)
				DC_UnlockDc(DCSrc);
			SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
			return FALSE;
		}
	}

	/* Perform the alpha blend operation */
	Status = IntEngAlphaBlend(&BitmapDest->SurfObj, &BitmapSrc->SurfObj,
	                          DCDest->CombinedClip, XlateObj,
	                          &DestRect, &SourceRect, &BlendObj);

	if (XlateObj != NULL)
		EngDeleteXlate(XlateObj);

	BITMAPOBJ_UnlockBitmap(BitmapDest);
	if (BitmapSrc != BitmapDest)
		BITMAPOBJ_UnlockBitmap(BitmapSrc);
	DC_UnlockDc(DCDest);
	if (hDCSrc != hDCDest)
		DC_UnlockDc(DCSrc);

	return Status;
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

	return ((bmWidth * bpp + 15) & ~15) >> 3;
}

HBITMAP FASTCALL
BITMAPOBJ_CopyBitmap(HBITMAP  hBitmap)
{
	HBITMAP  res;
	BITMAP  bm;
	BITMAPOBJ *Bitmap;

        if (hBitmap == NULL)
		return 0;

	Bitmap = GDIOBJ_LockObj(GdiHandleTable, hBitmap, GDI_OBJECT_TYPE_BITMAP);
        if (Bitmap == NULL)
		return 0;

	BITMAP_GetObject(Bitmap, sizeof(BITMAP), &bm);
	bm.bmBits = NULL;
	if (Bitmap->SurfObj.lDelta >= 0)
		bm.bmHeight = -bm.bmHeight;

	res = IntCreateBitmapIndirect(&bm);
	if(res)
	{
		PBYTE buf;

		buf = ExAllocatePoolWithTag (PagedPool, bm.bmWidthBytes * abs(bm.bmHeight), TAG_BITMAP);
		NtGdiGetBitmapBits (hBitmap, bm.bmWidthBytes * abs(bm.bmHeight), buf);
		NtGdiSetBitmapBits (res, bm.bmWidthBytes * abs(bm.bmHeight), buf);
		ExFreePool (buf);
	}

	GDIOBJ_UnlockObjByPtr(GdiHandleTable, Bitmap);

	return  res;
}

INT STDCALL
BITMAP_GetObject(BITMAPOBJ * bmp, INT count, LPVOID buffer)
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
		BITMAP Bitmap;
		if (count > (INT) sizeof(BITMAP)) count = sizeof(BITMAP);
		Bitmap.bmType = 0;
		Bitmap.bmWidth = bmp->SurfObj.sizlBitmap.cx;
		Bitmap.bmHeight = bmp->SurfObj.sizlBitmap.cy;
		Bitmap.bmWidthBytes = abs(bmp->SurfObj.lDelta);
		Bitmap.bmPlanes = 1;
		Bitmap.bmBitsPixel = BitsPerFormat(bmp->SurfObj.iBitmapFormat);
		Bitmap.bmBits = bmp->SurfObj.pvBits;
		memcpy(buffer, &Bitmap, count);
		return count;
	}
}
/* EOF */
