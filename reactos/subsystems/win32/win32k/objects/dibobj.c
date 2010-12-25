/*
 * ReactOS W32 Subsystem
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

static const RGBQUAD EGAColorsQuads[16] = {
/* rgbBlue, rgbGreen, rgbRed, rgbReserved */
    { 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x80, 0x00 },
    { 0x00, 0x80, 0x00, 0x00 },
    { 0x00, 0x80, 0x80, 0x00 },
    { 0x80, 0x00, 0x00, 0x00 },
    { 0x80, 0x00, 0x80, 0x00 },
    { 0x80, 0x80, 0x00, 0x00 },
    { 0x80, 0x80, 0x80, 0x00 },
    { 0xc0, 0xc0, 0xc0, 0x00 },
    { 0x00, 0x00, 0xff, 0x00 },
    { 0x00, 0xff, 0x00, 0x00 },
    { 0x00, 0xff, 0xff, 0x00 },
    { 0xff, 0x00, 0x00, 0x00 },
    { 0xff, 0x00, 0xff, 0x00 },
    { 0xff, 0xff, 0x00, 0x00 },
    { 0xff, 0xff, 0xff, 0x00 }
};

static const RGBTRIPLE EGAColorsTriples[16] = {
/* rgbBlue, rgbGreen, rgbRed */
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x80 },
    { 0x00, 0x80, 0x00 },
    { 0x00, 0x80, 0x80 },
    { 0x80, 0x00, 0x00 },
    { 0x80, 0x00, 0x80 },
    { 0x80, 0x80, 0x00 },
    { 0x80, 0x80, 0x80 },
    { 0xc0, 0xc0, 0xc0 },
    { 0x00, 0x00, 0xff },
    { 0x00, 0xff, 0x00 },
    { 0x00, 0xff, 0xff },
    { 0xff, 0x00, 0x00 },
    { 0xff, 0x00, 0xff },
    { 0xff, 0xff, 0x00 },
    { 0xff, 0xff, 0xff }
};

static const RGBQUAD DefLogPaletteQuads[20] = { /* Copy of Default Logical Palette */
/* rgbBlue, rgbGreen, rgbRed, rgbReserved */
    { 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x80, 0x00 },
    { 0x00, 0x80, 0x00, 0x00 },
    { 0x00, 0x80, 0x80, 0x00 },
    { 0x80, 0x00, 0x00, 0x00 },
    { 0x80, 0x00, 0x80, 0x00 },
    { 0x80, 0x80, 0x00, 0x00 },
    { 0xc0, 0xc0, 0xc0, 0x00 },
    { 0xc0, 0xdc, 0xc0, 0x00 },
    { 0xf0, 0xca, 0xa6, 0x00 },
    { 0xf0, 0xfb, 0xff, 0x00 },
    { 0xa4, 0xa0, 0xa0, 0x00 },
    { 0x80, 0x80, 0x80, 0x00 },
    { 0x00, 0x00, 0xf0, 0x00 },
    { 0x00, 0xff, 0x00, 0x00 },
    { 0x00, 0xff, 0xff, 0x00 },
    { 0xff, 0x00, 0x00, 0x00 },
    { 0xff, 0x00, 0xff, 0x00 },
    { 0xff, 0xff, 0x00, 0x00 },
    { 0xff, 0xff, 0xff, 0x00 }
};

static const RGBQUAD DefLogPaletteTriples[20] = { /* Copy of Default Logical Palette */
/* rgbBlue, rgbGreen, rgbRed, rgbReserved */
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x80 },
    { 0x00, 0x80, 0x00 },
    { 0x00, 0x80, 0x80 },
    { 0x80, 0x00, 0x00 },
    { 0x80, 0x00, 0x80 },
    { 0x80, 0x80, 0x00 },
    { 0xc0, 0xc0, 0xc0 },
    { 0xc0, 0xdc, 0xc0 },
    { 0xf0, 0xca, 0xa6 },
    { 0xf0, 0xfb, 0xff },
    { 0xa4, 0xa0, 0xa0 },
    { 0x80, 0x80, 0x80 },
    { 0x00, 0x00, 0xf0 },
    { 0x00, 0xff, 0x00 },
    { 0x00, 0xff, 0xff },
    { 0xff, 0x00, 0x00 },
    { 0xff, 0x00, 0xff },
    { 0xff, 0xff, 0x00 },
    { 0xff, 0xff, 0xff }
};


UINT
APIENTRY
IntSetDIBColorTable(
    HDC hDC,
    UINT StartIndex,
    UINT Entries,
    CONST RGBQUAD *Colors)
{
    PDC dc;
    PSURFACE psurf;
    PPALETTE PalGDI;
    UINT Index;
    ULONG biBitCount;

    if (!(dc = DC_LockDc(hDC))) return 0;
    if (dc->dctype == DC_TYPE_INFO)
    {
        DC_UnlockDc(dc);
        return 0;
    }

    psurf = dc->dclevel.pSurface;
    if (psurf == NULL)
    {
        DC_UnlockDc(dc);
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (psurf->hSecure == NULL)
    {
        DC_UnlockDc(dc);
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    biBitCount = BitsPerFormat(psurf->SurfObj.iBitmapFormat);
    if (biBitCount <= 8 && StartIndex < (1 << biBitCount))
    {
        if (StartIndex + Entries > (1 << biBitCount))
            Entries = (1 << biBitCount) - StartIndex;

        if (psurf->ppal == NULL)
        {
            DC_UnlockDc(dc);
            EngSetLastError(ERROR_INVALID_HANDLE);
            return 0;
        }

        PalGDI = PALETTE_LockPalette(psurf->ppal->BaseObject.hHmgr);

        for (Index = StartIndex;
             Index < StartIndex + Entries && Index < PalGDI->NumColors;
             Index++)
        {
            PalGDI->IndexedColors[Index].peRed = Colors[Index - StartIndex].rgbRed;
            PalGDI->IndexedColors[Index].peGreen = Colors[Index - StartIndex].rgbGreen;
            PalGDI->IndexedColors[Index].peBlue = Colors[Index - StartIndex].rgbBlue;
        }
        PALETTE_UnlockPalette(PalGDI);
    }
    else
        Entries = 0;

    /* Mark the brushes invalid */
    dc->pdcattr->ulDirty_ |= DIRTY_FILL|DIRTY_LINE|DIRTY_BACKGROUND|DIRTY_TEXT;

    DC_UnlockDc(dc);

    return Entries;
}

UINT
APIENTRY
IntGetDIBColorTable(
    HDC hDC,
    UINT StartIndex,
    UINT Entries,
    RGBQUAD *Colors)
{
    PDC dc;
    PSURFACE psurf;
    PPALETTE PalGDI;
    UINT Index, Count = 0;
    ULONG biBitCount;

    if (!(dc = DC_LockDc(hDC))) return 0;
    if (dc->dctype == DC_TYPE_INFO)
    {
        DC_UnlockDc(dc);
        return 0;
    }

    psurf = dc->dclevel.pSurface;
    if (psurf == NULL)
    {
        DC_UnlockDc(dc);
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (psurf->hSecure == NULL)
    {
        DC_UnlockDc(dc);
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    biBitCount = BitsPerFormat(psurf->SurfObj.iBitmapFormat);
    if (biBitCount <= 8 &&
            StartIndex < (1 << biBitCount))
    {
        if (StartIndex + Entries > (1 << biBitCount))
            Entries = (1 << biBitCount) - StartIndex;

        if (psurf->ppal == NULL)
        {
            DC_UnlockDc(dc);
            EngSetLastError(ERROR_INVALID_HANDLE);
            return 0;
        }

        PalGDI = PALETTE_LockPalette(psurf->ppal->BaseObject.hHmgr);

        for (Index = StartIndex;
             Index < StartIndex + Entries && Index < PalGDI->NumColors;
             Index++)
        {
            Colors[Index - StartIndex].rgbRed = PalGDI->IndexedColors[Index].peRed;
            Colors[Index - StartIndex].rgbGreen = PalGDI->IndexedColors[Index].peGreen;
            Colors[Index - StartIndex].rgbBlue = PalGDI->IndexedColors[Index].peBlue;
            Colors[Index - StartIndex].rgbReserved = 0;
            Count++;
        }
        PALETTE_UnlockPalette(PalGDI);
    }

    DC_UnlockDc(dc);

    return Count;
}

// Converts a DIB to a device-dependent bitmap
static INT
FASTCALL
IntSetDIBits(
    PDC   DC,
    HBITMAP  hBitmap,
    UINT  StartScan,
    UINT  ScanLines,
    CONST VOID  *Bits,
    CONST BITMAPINFO  *bmi,
    UINT  ColorUse)
{
    HBITMAP     SourceBitmap;
	PSURFACE    psurfDst, psurfSrc;
    INT         result = 0;
	RECT		rcDst;
	POINTL		ptSrc;
	PVOID		pvBits;
	EXLATEOBJ	exlo;

    SourceBitmap = DIB_CreateDIBSection(DC, bmi, ColorUse, &pvBits, NULL, 0, 0);
	if (0 == SourceBitmap)
    {
		DPRINT1("Error : Could not create a DIBSection.\n");
        EngSetLastError(ERROR_NO_SYSTEM_RESOURCES);
        return 0;
    }

	RtlCopyMemory(pvBits, Bits, DIB_GetDIBImageBytes(bmi->bmiHeader.biWidth,
													 bmi->bmiHeader.biHeight,
													 bmi->bmiHeader.biBitCount));

	psurfDst = SURFACE_LockSurface(hBitmap);
	psurfSrc = SURFACE_LockSurface(SourceBitmap);

	if(!(psurfSrc && psurfDst))
	{
		DPRINT1("Error, could not lock surfaces\n");
		goto cleanup;
	}

	rcDst.top = bmi->bmiHeader.biHeight < 0 ?
		abs(bmi->bmiHeader.biHeight) - (ScanLines + StartScan) : StartScan;
	rcDst.left = 0;
	rcDst.bottom = rcDst.top + ScanLines;
	rcDst.right = psurfDst->SurfObj.sizlBitmap.cx;

	ptSrc.x = 0;
	ptSrc.y = 0;

	EXLATEOBJ_vInitialize(&exlo, psurfSrc->ppal, psurfDst->ppal, 0, 0, 0);

	result = IntEngCopyBits(&psurfDst->SurfObj,
		                    &psurfSrc->SurfObj,
							NULL,
							&exlo.xlo,
							&rcDst,
							&ptSrc);
	if(result)
		result = ScanLines;

	EXLATEOBJ_vCleanup(&exlo);

cleanup:
	if(psurfSrc)
	{
		SURFACE_UnlockSurface(psurfSrc);
	}
	if(psurfDst)
	{
		SURFACE_UnlockSurface(psurfDst);
	}
	GreDeleteObject(SourceBitmap);

    return result;
}

// FIXME by Removing NtGdiSetDIBits!!!
// This is a victim of the Win32k Initialization BUG!!!!!
// Converts a DIB to a device-dependent bitmap
INT
APIENTRY
NtGdiSetDIBits(
    HDC  hDC,
    HBITMAP  hBitmap,
    UINT  StartScan,
    UINT  ScanLines,
    CONST VOID  *Bits,
    CONST BITMAPINFO  *bmi,
    UINT  ColorUse)
{
    PDC Dc;
    INT Ret;
    NTSTATUS Status = STATUS_SUCCESS;

    if (!Bits) return 0;

    _SEH2_TRY
    {
        ProbeForRead(&bmi->bmiHeader.biSize, sizeof(DWORD), 1);
		ProbeForRead(bmi, bmi->bmiHeader.biSize, 1);
		ProbeForRead(bmi, DIB_BitmapInfoSize(bmi, ColorUse), 1);
        ProbeForRead(Bits,
                     DIB_GetDIBImageBytes(bmi->bmiHeader.biWidth,
                                          ScanLines,
                                          bmi->bmiHeader.biBitCount),
			         1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    if (!NT_SUCCESS(Status))
    {
        return 0;
    }

    Dc = DC_LockDc(hDC);
    if (NULL == Dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }
    if (Dc->dctype == DC_TYPE_INFO)
    {
        DC_UnlockDc(Dc);
        return 0;
    }

    Ret = IntSetDIBits(Dc, hBitmap, StartScan, ScanLines, Bits, bmi, ColorUse);

    DC_UnlockDc(Dc);

    return Ret;
}

W32KAPI
INT
APIENTRY
NtGdiSetDIBitsToDeviceInternal(
    IN HDC hDC,
    IN INT XDest,
    IN INT YDest,
    IN DWORD Width,
    IN DWORD Height,
    IN INT XSrc,
    IN INT YSrc,
    IN DWORD StartScan,
    IN DWORD ScanLines,
    IN LPBYTE Bits,
    IN LPBITMAPINFO bmi,
    IN DWORD ColorUse,
    IN UINT cjMaxBits,
    IN UINT cjMaxInfo,
    IN BOOL bTransformCoordinates,
    IN OPTIONAL HANDLE hcmXform)
{
    INT ret = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    PDC pDC;
    HBITMAP hSourceBitmap = NULL;
    SURFOBJ *pDestSurf, *pSourceSurf = NULL;
    SURFACE *pSurf;
    RECTL rcDest;
    POINTL ptSource;
    INT DIBWidth;
    SIZEL SourceSize;
    EXLATEOBJ exlo;
    PPALETTE ppalDIB = NULL;
    HPALETTE hpalDIB = NULL;

    if (!Bits) return 0;

    _SEH2_TRY
    {
        ProbeForRead(bmi, cjMaxInfo, 1);
        ProbeForRead(Bits, cjMaxBits, 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    if (!NT_SUCCESS(Status))
    {
        return 0;
    }

    pDC = DC_LockDc(hDC);
    if (!pDC)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }
    if (pDC->dctype == DC_TYPE_INFO)
    {
        DC_UnlockDc(pDC);
        return 0;
    }

    pSurf = pDC->dclevel.pSurface;

    pDestSurf = pSurf ? &pSurf->SurfObj : NULL;

    ScanLines = min(ScanLines, abs(bmi->bmiHeader.biHeight) - StartScan);

    rcDest.left = XDest;
    rcDest.top = YDest;
    if (bTransformCoordinates)
    {
        CoordLPtoDP(pDC, (LPPOINT)&rcDest);
    }
    rcDest.left += pDC->ptlDCOrig.x;
    rcDest.top += pDC->ptlDCOrig.y;
    rcDest.right = rcDest.left + Width;
    rcDest.bottom = rcDest.top + Height;
    rcDest.top += StartScan;

    ptSource.x = XSrc;
    ptSource.y = YSrc;

    SourceSize.cx = bmi->bmiHeader.biWidth;
    SourceSize.cy = ScanLines;

    DIBWidth = WIDTH_BYTES_ALIGN32(SourceSize.cx, bmi->bmiHeader.biBitCount);

    hSourceBitmap = EngCreateBitmap(SourceSize,
                                    DIBWidth,
                                    BitmapFormat(bmi->bmiHeader.biBitCount,
                                                 bmi->bmiHeader.biCompression),
                                    bmi->bmiHeader.biHeight < 0 ? BMF_TOPDOWN : 0,
                                    (PVOID) Bits);
    if (!hSourceBitmap)
    {
        EngSetLastError(ERROR_NO_SYSTEM_RESOURCES);
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    pSourceSurf = EngLockSurface((HSURF)hSourceBitmap);
    if (!pSourceSurf)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    ASSERT(pSurf->ppal);

    /* Create a palette for the DIB */
    hpalDIB = BuildDIBPalette(bmi);
    if (!hpalDIB)
    {
        EngSetLastError(ERROR_NO_SYSTEM_RESOURCES);
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    /* Lock the DIB palette */
    ppalDIB = PALETTE_LockPalette(hpalDIB);
    if (!ppalDIB)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    /* Initialize EXLATEOBJ */
    EXLATEOBJ_vInitialize(&exlo, ppalDIB, pSurf->ppal, 0, 0, 0);

    /* Copy the bits */
    DPRINT("BitsToDev with dstsurf=(%d|%d) (%d|%d), src=(%d|%d) w=%d h=%d\n",
        rcDest.left, rcDest.top, rcDest.right, rcDest.bottom,
        ptSource.x, ptSource.y, SourceSize.cx, SourceSize.cy);
    Status = IntEngBitBlt(pDestSurf,
                          pSourceSurf,
                          NULL,
                          pDC->rosdc.CombinedClip,
                          &exlo.xlo,
                          &rcDest,
                          &ptSource,
                          NULL,
                          NULL,
                          NULL,
                          ROP3_TO_ROP4(SRCCOPY));

    /* Cleanup EXLATEOBJ */
    EXLATEOBJ_vCleanup(&exlo);

Exit:
    if (NT_SUCCESS(Status))
    {
        ret = ScanLines;
    }

    if (ppalDIB) PALETTE_UnlockPalette(ppalDIB);

    if (pSourceSurf) EngUnlockSurface(pSourceSurf);
    if (hSourceBitmap) EngDeleteSurface((HSURF)hSourceBitmap);
    if (hpalDIB) PALETTE_FreePaletteByHandle(hpalDIB);
    DC_UnlockDc(pDC);

    return ret;
}


/* Converts a device-dependent bitmap to a DIB */
INT
APIENTRY
NtGdiGetDIBitsInternal(
    HDC hDC,
    HBITMAP hBitmap,
    UINT StartScan,
    UINT ScanLines,
    LPBYTE Bits,
    LPBITMAPINFO Info,
    UINT Usage,
    UINT MaxBits,
    UINT MaxInfo)
{
	BITMAPCOREINFO* pbmci = NULL;
	PSURFACE psurf = NULL;
	PDC pDC;
	LONG width, height;
	WORD planes, bpp;
	DWORD compr, size ;
	int i, bitmap_type;
	RGBTRIPLE* rgbTriples;
	RGBQUAD* rgbQuads;
	VOID* colorPtr;
	NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("Entered NtGdiGetDIBitsInternal()\n");

    if ((Usage && Usage != DIB_PAL_COLORS) || !Info || !hBitmap)
        return 0;

    _SEH2_TRY
    {
		/* Probe for read and write */
        ProbeForRead(Info, MaxInfo, 1);
		ProbeForWrite(Info, MaxInfo, 1);
        if (Bits) ProbeForWrite(Bits, MaxBits, 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    if (!NT_SUCCESS(Status))
    {
        return 0;
    }

	colorPtr = (LPBYTE)Info + Info->bmiHeader.biSize;
	rgbTriples = colorPtr;
	rgbQuads = colorPtr;

	bitmap_type = DIB_GetBitmapInfo(&Info->bmiHeader,
		                            &width,
									&height,
									&planes,
									&bpp,
									&compr,
									&size);
	if(bitmap_type == -1)
	{
		DPRINT("Wrong bitmap format\n");
		EngSetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}
	else if(bitmap_type == 0)
	{
		/* We need a BITMAPINFO to create a DIB, but we have to fill
		 * the BITMAPCOREINFO we're provided */
		pbmci = (BITMAPCOREINFO*)Info;
		Info = DIB_ConvertBitmapInfo((BITMAPINFO*)pbmci, Usage);
		if(Info == NULL)
		{
			DPRINT1("Error, could not convert the BITMAPCOREINFO!\n");
			return 0;
		}
		rgbQuads = Info->bmiColors;
	}

    pDC = DC_LockDc(hDC);
    if (pDC == NULL || pDC->dctype == DC_TYPE_INFO)
    {
		ScanLines = 0;
        goto done;
    }

    /* Get a pointer to the source bitmap object */
    psurf = SURFACE_LockSurface(hBitmap);
    if (psurf == NULL)
    {
        ScanLines = 0;
        goto done;
    }

	/* Fill in the structure */
	switch(bpp)
	{
	case 0: /* Only info */
		if(pbmci)
		{
			pbmci->bmciHeader.bcWidth = psurf->SurfObj.sizlBitmap.cx;
			pbmci->bmciHeader.bcHeight = (psurf->SurfObj.fjBitmap & BMF_TOPDOWN) ?
				-psurf->SurfObj.sizlBitmap.cy :
			    psurf->SurfObj.sizlBitmap.cy;
			pbmci->bmciHeader.bcPlanes = 1;
			pbmci->bmciHeader.bcBitCount = BitsPerFormat(psurf->SurfObj.iBitmapFormat);
		}
		Info->bmiHeader.biWidth = psurf->SurfObj.sizlBitmap.cx;
		Info->bmiHeader.biHeight = (psurf->SurfObj.fjBitmap & BMF_TOPDOWN) ?
			-psurf->SurfObj.sizlBitmap.cy :
		    psurf->SurfObj.sizlBitmap.cy;;
		Info->bmiHeader.biPlanes = 1;
		Info->bmiHeader.biBitCount = BitsPerFormat(psurf->SurfObj.iBitmapFormat);
		Info->bmiHeader.biSizeImage = DIB_GetDIBImageBytes( Info->bmiHeader.biWidth,
															Info->bmiHeader.biHeight,
															Info->bmiHeader.biBitCount);
		if(psurf->hSecure)
		{
			switch(Info->bmiHeader.biBitCount)
			{
			case 16:
			case 32:
				Info->bmiHeader.biCompression = BI_BITFIELDS;
				break;
			default:
				Info->bmiHeader.biCompression = BI_RGB;
				break;
			}
		}
		else if(Info->bmiHeader.biBitCount > 8)
		{
			Info->bmiHeader.biCompression = BI_BITFIELDS;
		}
		else
		{
			Info->bmiHeader.biCompression = BI_RGB;
		}
		Info->bmiHeader.biXPelsPerMeter = 0;
        Info->bmiHeader.biYPelsPerMeter = 0;
        Info->bmiHeader.biClrUsed = 0;
        Info->bmiHeader.biClrImportant = 0;
		ScanLines = abs(Info->bmiHeader.biHeight);
		goto done;

	case 1:
	case 4:
	case 8:
		Info->bmiHeader.biClrUsed = 0;

		/* If the bitmap if a DIB section and has the same format than what
		 * we're asked, go ahead! */
		if((psurf->hSecure) &&
			(BitsPerFormat(psurf->SurfObj.iBitmapFormat) == bpp))
		{
			if(Usage == DIB_RGB_COLORS)
			{
				unsigned int colors = min(psurf->ppal->NumColors, 1 << bpp);

				if(pbmci)
				{
					for(i=0; i < colors; i++)
					{
						rgbTriples[i].rgbtRed = psurf->ppal->IndexedColors[i].peRed;
						rgbTriples[i].rgbtGreen = psurf->ppal->IndexedColors[i].peGreen;
						rgbTriples[i].rgbtBlue = psurf->ppal->IndexedColors[i].peBlue;
					}
				}
				if(colors != 1 << bpp) Info->bmiHeader.biClrUsed = colors;
				for(i=0; i < colors; i++)
				{
					rgbQuads[i].rgbRed = psurf->ppal->IndexedColors[i].peRed;
					rgbQuads[i].rgbGreen = psurf->ppal->IndexedColors[i].peGreen;
					rgbQuads[i].rgbBlue = psurf->ppal->IndexedColors[i].peBlue;
				}
			}
			else
			{
				for(i=0; i < 1 << bpp; i++)
				{
					if(pbmci) ((WORD*)rgbTriples)[i] = i;
					((WORD*)rgbQuads)[i] = i;
				}
			}
		}
		else
		{
			if(Usage == DIB_PAL_COLORS)
			{
				for(i=0; i < 1 << bpp; i++)
				{
					if(pbmci) ((WORD*)rgbTriples)[i] = i;
					((WORD*)rgbQuads)[i] = i;
				}
			}
			else if(bpp > 1 && bpp == BitsPerFormat(psurf->SurfObj.iBitmapFormat)) {
                /* For color DDBs in native depth (mono DDBs always have
                   a black/white palette):
                   Generate the color map from the selected palette */
                PPALETTE pDcPal = PALETTE_LockPalette(pDC->dclevel.hpal);
				if(!pDcPal)
				{
					ScanLines = 0 ;
					goto done ;
				}
                for (i = 0; i < pDcPal->NumColors; i++) {
                    if (pbmci)
                    {
                        rgbTriples[i].rgbtRed   = pDcPal->IndexedColors[i].peRed;
                        rgbTriples[i].rgbtGreen = pDcPal->IndexedColors[i].peGreen;
                        rgbTriples[i].rgbtBlue  = pDcPal->IndexedColors[i].peBlue;
                    }

                    rgbQuads[i].rgbRed      = pDcPal->IndexedColors[i].peRed;
                    rgbQuads[i].rgbGreen    = pDcPal->IndexedColors[i].peGreen;
                    rgbQuads[i].rgbBlue     = pDcPal->IndexedColors[i].peBlue;
                    rgbQuads[i].rgbReserved = 0;
				}
				PALETTE_UnlockPalette(pDcPal);
            } else {
                switch (bpp) {
                case 1:
                    if (pbmci)
                    {
                        rgbTriples[0].rgbtRed = rgbTriples[0].rgbtGreen =
                            rgbTriples[0].rgbtBlue = 0;
                        rgbTriples[1].rgbtRed = rgbTriples[1].rgbtGreen =
                            rgbTriples[1].rgbtBlue = 0xff;
                    }
                    rgbQuads[0].rgbRed = rgbQuads[0].rgbGreen =
                        rgbQuads[0].rgbBlue = 0;
                    rgbQuads[0].rgbReserved = 0;
                    rgbQuads[1].rgbRed = rgbQuads[1].rgbGreen =
                        rgbQuads[1].rgbBlue = 0xff;
                    rgbQuads[1].rgbReserved = 0;
                    break;

                case 4:
                    if (pbmci)
                        RtlCopyMemory(rgbTriples, EGAColorsTriples, sizeof(EGAColorsTriples));
                    RtlCopyMemory(rgbQuads, EGAColorsQuads, sizeof(EGAColorsQuads));

                    break;

                case 8:
                    {
						INT r, g, b;
                        RGBQUAD *color;
                        if (pbmci)
                        {
                            RGBTRIPLE *colorTriple;

                            RtlCopyMemory(rgbTriples, DefLogPaletteTriples,
                                       10 * sizeof(RGBTRIPLE));
                            RtlCopyMemory(rgbTriples + 246, DefLogPaletteTriples + 10,
                                       10 * sizeof(RGBTRIPLE));
                            colorTriple = rgbTriples + 10;
                            for(r = 0; r <= 5; r++) /* FIXME */
							{
                                for(g = 0; g <= 5; g++)
								{
                                    for(b = 0; b <= 5; b++)
									{
                                        colorTriple->rgbtRed =   (r * 0xff) / 5;
                                        colorTriple->rgbtGreen = (g * 0xff) / 5;
                                        colorTriple->rgbtBlue =  (b * 0xff) / 5;
                                        color++;
                                    }
								}
							}
                        }
						memcpy(rgbQuads, DefLogPaletteQuads,
                                   10 * sizeof(RGBQUAD));
                        memcpy(rgbQuads + 246, DefLogPaletteQuads + 10,
                               10 * sizeof(RGBQUAD));
                        color = rgbQuads + 10;
                        for(r = 0; r <= 5; r++) /* FIXME */
						{
                            for(g = 0; g <= 5; g++)
							{
                                for(b = 0; b <= 5; b++)
								{
                                    color->rgbRed =   (r * 0xff) / 5;
                                    color->rgbGreen = (g * 0xff) / 5;
                                    color->rgbBlue =  (b * 0xff) / 5;
                                    color->rgbReserved = 0;
                                    color++;
                                }
							}
						}
                    }
                }
            }
        }
        break;

    case 15:
        if (Info->bmiHeader.biCompression == BI_BITFIELDS)
        {
            ((PDWORD)Info->bmiColors)[0] = 0x7c00;
            ((PDWORD)Info->bmiColors)[1] = 0x03e0;
            ((PDWORD)Info->bmiColors)[2] = 0x001f;
        }
        break;

    case 16:
        if (Info->bmiHeader.biCompression == BI_BITFIELDS)
        {
            if (psurf->hSecure)
			{
				((PDWORD)Info->bmiColors)[0] = psurf->ppal->RedMask;
                ((PDWORD)Info->bmiColors)[1] = psurf->ppal->GreenMask;
                ((PDWORD)Info->bmiColors)[2] = psurf->ppal->BlueMask;
			}
            else
            {
                ((PDWORD)Info->bmiColors)[0] = 0xf800;
                ((PDWORD)Info->bmiColors)[1] = 0x07e0;
                ((PDWORD)Info->bmiColors)[2] = 0x001f;
            }
        }
        break;

    case 24:
    case 32:
        if (Info->bmiHeader.biCompression == BI_BITFIELDS)
        {
            if (psurf->hSecure)
			{
				((PDWORD)Info->bmiColors)[0] = psurf->ppal->RedMask;
                ((PDWORD)Info->bmiColors)[1] = psurf->ppal->GreenMask;
                ((PDWORD)Info->bmiColors)[2] = psurf->ppal->BlueMask;
			}
            else
            {
                ((PDWORD)Info->bmiColors)[0] = 0xff0000;
                ((PDWORD)Info->bmiColors)[1] = 0x00ff00;
                ((PDWORD)Info->bmiColors)[2] = 0x0000ff;
            }
        }
        break;
    }
	Info->bmiHeader.biSizeImage = DIB_GetDIBImageBytes(width, height, bpp);

	if(Bits && ScanLines)
	{
		/* Create a DIBSECTION, blt it, profit */
		PVOID pDIBits ;
		HBITMAP hBmpDest;
		PSURFACE psurfDest;
		EXLATEOBJ exlo;
		RECT rcDest;
		POINTL srcPoint;
		BOOL ret ;

		if (StartScan > psurf->SurfObj.sizlBitmap.cy)
        {
			ScanLines = 0;
            goto done;
        }
        else
        {
            ScanLines = min(ScanLines, psurf->SurfObj.sizlBitmap.cy - StartScan);
		}

		/* Fixup values */
		Info->bmiHeader.biWidth = psurf->SurfObj.sizlBitmap.cx;
		Info->bmiHeader.biHeight = height < 0 ?
			-ScanLines : ScanLines;
		/* Create the DIB */
		hBmpDest = DIB_CreateDIBSection(pDC, Info, Usage, &pDIBits, NULL, 0, 0);
		/* Restore them */
		Info->bmiHeader.biWidth = width;
		Info->bmiHeader.biHeight = height;

		if(!hBmpDest)
		{
			DPRINT1("Unable to create a DIB Section!\n");
			EngSetLastError(ERROR_INVALID_PARAMETER);
			ScanLines = 0;
			goto done ;
		}

		psurfDest = SURFACE_LockSurface(hBmpDest);

		rcDest.left = 0;
		rcDest.top = 0;
		rcDest.bottom = ScanLines;
		rcDest.right = psurf->SurfObj.sizlBitmap.cx;

		srcPoint.x = 0;
		srcPoint.y = height < 0 ?
			psurf->SurfObj.sizlBitmap.cy - (StartScan + ScanLines) : StartScan;

		EXLATEOBJ_vInitialize(&exlo, psurf->ppal, psurfDest->ppal, 0, 0, 0);

		ret = IntEngCopyBits(&psurfDest->SurfObj,
							 &psurf->SurfObj,
							 NULL,
							 &exlo.xlo,
							 &rcDest,
							 &srcPoint);

		if(!ret)
			ScanLines = 0;
		else
		{
			Status = STATUS_SUCCESS;
			_SEH2_TRY
			{
				RtlCopyMemory(Bits, pDIBits, DIB_GetDIBImageBytes (width, ScanLines, bpp));
			}
			_SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
			{
				Status = _SEH2_GetExceptionCode();
			}
			_SEH2_END

			if(!NT_SUCCESS(Status))
			{
				DPRINT1("Unable to copy bits to the user provided pointer\n");
				ScanLines = 0;
			}
		}

		GreDeleteObject(hBmpDest);
		EXLATEOBJ_vCleanup(&exlo);
	}
	else ScanLines = abs(height);

done:

	if(pDC) DC_UnlockDc(pDC);
	if(psurf) SURFACE_UnlockSurface(psurf);
	if(pbmci) DIB_FreeConvertedBitmapInfo(Info, (BITMAPINFO*)pbmci);

	return ScanLines;
}


INT
APIENTRY
NtGdiStretchDIBitsInternal(
    HDC  hDC,
    INT  XDest,
    INT  YDest,
    INT  DestWidth,
    INT  DestHeight,
    INT  XSrc,
    INT  YSrc,
    INT  SrcWidth,
    INT  SrcHeight,
    LPBYTE Bits,
    LPBITMAPINFO BitsInfo,
    DWORD  Usage,
    DWORD  ROP,
    UINT cjMaxInfo,
    UINT cjMaxBits,
    HANDLE hcmXform)
{
    PDC pdc;
    INT ret = 0;
    LONG height;
    LONG width;
    WORD planes, bpp;
    DWORD compr, size;
    HBITMAP hBitmap;
    BOOL fastpath = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
	PBYTE safeBits ;

    if (!Bits || !BitsInfo)
        return 0;

	safeBits = ExAllocatePoolWithTag(PagedPool, cjMaxBits, TAG_DIB);
	if(!safeBits)
	{
		EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return 0;
	}

    if (!(pdc = DC_LockDc(hDC)))
	{
		ExFreePoolWithTag(safeBits, TAG_DIB);
		EngSetLastError(ERROR_INVALID_HANDLE);
		return 0;
	}

    _SEH2_TRY
    {
        ProbeForRead(BitsInfo, cjMaxInfo, 1);
        ProbeForRead(Bits, cjMaxBits, 1);
        if (DIB_GetBitmapInfo( &BitsInfo->bmiHeader, &width, &height, &planes, &bpp, &compr, &size ) == -1)
        {
            DPRINT1("Invalid bitmap\n");
            Status = STATUS_INVALID_PARAMETER;
        }
		RtlCopyMemory(safeBits, Bits, cjMaxBits);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    if(!NT_SUCCESS(Status))
    {
        DPRINT1("Error, failed to read the DIB bits\n");
        goto cleanup;
    }

    if (width < 0)
    {
        DPRINT1("Bitmap has a negative width\n");
        return 0;
    }

    hBitmap = NtGdiGetDCObject(hDC, OBJ_BITMAP);

    if (XDest == 0 && YDest == 0 && XSrc == 0 && XSrc == 0 &&
        DestWidth == SrcWidth && DestHeight == SrcHeight &&
        compr == BI_RGB &&
        ROP == SRCCOPY)
    {
        BITMAP bmp;
        if (IntGdiGetObject(hBitmap, sizeof(bmp), &bmp) == sizeof(bmp))
        {
            if (bmp.bmBitsPixel == bpp &&
                bmp.bmWidth == SrcWidth &&
                bmp.bmHeight == SrcHeight &&
                bmp.bmPlanes == planes)
                fastpath = TRUE;
        }
    }

    if (fastpath)
    {
        /* fast path */
        DPRINT1("using fast path\n");
        ret = IntSetDIBits( pdc, hBitmap, 0, height, safeBits, BitsInfo, Usage);
    }
    else
    {
        /* slow path - need to use StretchBlt */
        HBITMAP hOldBitmap;
        HDC hdcMem;
        PVOID pvBits;

        hdcMem = NtGdiCreateCompatibleDC( hDC );
        hBitmap = DIB_CreateDIBSection(pdc, BitsInfo, Usage, &pvBits, NULL, 0, 0);
		if(!hBitmap)
		{
			DPRINT1("Error, failed to create a DIB section\n");
			NtGdiDeleteObjectApp(hdcMem);
			goto cleanup;
		}
        RtlCopyMemory(pvBits, safeBits, cjMaxBits);
        hOldBitmap = NtGdiSelectBitmap( hdcMem, hBitmap );

        /* Origin for DIBitmap may be bottom left (positive biHeight) or top
           left (negative biHeight) */
        ret = NtGdiStretchBlt( hDC, XDest, YDest, DestWidth, DestHeight,
                             hdcMem, XSrc, abs(height) - SrcHeight - YSrc,
                             SrcWidth, SrcHeight, ROP, 0 );
        
		if(ret)
            ret = SrcHeight;
        NtGdiSelectBitmap( hdcMem, hOldBitmap );
        NtGdiDeleteObjectApp( hdcMem );
        GreDeleteObject( hBitmap );
    }
cleanup:
	ExFreePoolWithTag(safeBits, TAG_DIB);
    DC_UnlockDc(pdc);
    return ret;
}


HBITMAP
FASTCALL
IntCreateDIBitmap(
    PDC Dc,
    INT width,
    INT height,
    UINT bpp,
    DWORD init,
    LPBYTE bits,
    PBITMAPINFO data,
    DWORD coloruse)
{
    HBITMAP handle;
    BOOL fColor;

    // Check if we should create a monochrome or color bitmap. We create a monochrome bitmap only if it has exactly 2
    // colors, which are black followed by white, nothing else. In all other cases, we create a color bitmap.

    if (bpp != 1) fColor = TRUE;
    else if ((coloruse != DIB_RGB_COLORS) || (init != CBM_INIT) || !data) fColor = FALSE;
    else
    {
        const RGBQUAD *rgb = (RGBQUAD*)((PBYTE)data + data->bmiHeader.biSize);
        DWORD col = RGB(rgb->rgbRed, rgb->rgbGreen, rgb->rgbBlue);

        // Check if the first color of the colormap is black
        if ((col == RGB(0, 0, 0)))
        {
            rgb++;
            col = RGB(rgb->rgbRed, rgb->rgbGreen, rgb->rgbBlue);

            // If the second color is white, create a monochrome bitmap
            fColor = (col != RGB(0xff,0xff,0xff));
        }
        else fColor = TRUE;
    }

    // Now create the bitmap
    if (fColor)
    {
        handle = IntCreateCompatibleBitmap(Dc, width, height);
    }
    else
    {
        handle = GreCreateBitmap(width,
                                 height,
                                 1,
                                 1,
                                 NULL);
    }

    if (height < 0)
        height = -height;

    if (NULL != handle && CBM_INIT == init)
    {
        IntSetDIBits(Dc, handle, 0, height, bits, data, coloruse);
    }

    return handle;
}

// The CreateDIBitmap function creates a device-dependent bitmap (DDB) from a DIB and, optionally, sets the bitmap bits
// The DDB that is created will be whatever bit depth your reference DC is
HBITMAP
APIENTRY
NtGdiCreateDIBitmapInternal(
    IN HDC hDc,
    IN INT cx,
    IN INT cy,
    IN DWORD fInit,
    IN OPTIONAL LPBYTE pjInit,
    IN OPTIONAL LPBITMAPINFO pbmi,
    IN DWORD iUsage,
    IN UINT cjMaxInitInfo,
    IN UINT cjMaxBits,
    IN FLONG fl,
    IN HANDLE hcmXform)
{
    NTSTATUS Status = STATUS_SUCCESS;
	PBYTE safeBits = NULL;
	HBITMAP hbmResult = NULL;

	if(pjInit && (fInit == CBM_INIT))
	{
		safeBits = ExAllocatePoolWithTag(PagedPool, cjMaxBits, TAG_DIB);
		if(!safeBits)
		{
			EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
			return NULL;
		}
	}

    _SEH2_TRY
    {
        if(pbmi) ProbeForRead(pbmi, cjMaxInitInfo, 1);
        if(pjInit && (fInit == CBM_INIT))
		{
			ProbeForRead(pjInit, cjMaxBits, 1);
			RtlCopyMemory(safeBits, pjInit, cjMaxBits);
		}
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    if(!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
		goto cleanup;
    }

    hbmResult =  GreCreateDIBitmapInternal(hDc,
					                       cx,
						                   cy,
									       fInit,
                                           safeBits,
                                           pbmi,
                                           iUsage,
                                           fl,
                                           hcmXform);

cleanup:
	if (safeBits) ExFreePoolWithTag(safeBits, TAG_DIB);
	return hbmResult;
}

HBITMAP
FASTCALL
GreCreateDIBitmapInternal(
    IN HDC hDc,
    IN INT cx,
    IN INT cy,
    IN DWORD fInit,
    IN OPTIONAL LPBYTE pjInit,
    IN OPTIONAL PBITMAPINFO pbmi,
    IN DWORD iUsage,
    IN FLONG fl,
    IN HANDLE hcmXform)
{
    PDC Dc;
    HBITMAP Bmp;
    WORD bpp;
    HDC hdcDest;

    if (!hDc) /* 1bpp monochrome bitmap */
    {  // Should use System Bitmap DC hSystemBM, with CreateCompatibleDC for this.
        hdcDest = NtGdiCreateCompatibleDC(0);
        if(!hdcDest)
        {
            return NULL;
        }
    }
    else
    {
        hdcDest = hDc;
    }

    Dc = DC_LockDc(hdcDest);
    if (!Dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }
    /* It's OK to set bpp=0 here, as IntCreateDIBitmap will create a compatible Bitmap
     * if bpp != 1 and ignore the real value that was passed */
    if (pbmi)
        bpp = pbmi->bmiHeader.biBitCount;
    else
        bpp = 0;
    Bmp = IntCreateDIBitmap(Dc, cx, cy, bpp, fInit, pjInit, pbmi, iUsage);
    DC_UnlockDc(Dc);

    if(!hDc)
    {
        NtGdiDeleteObjectApp(hdcDest);
    }
    return Bmp;
}


HBITMAP
APIENTRY
NtGdiCreateDIBSection(
    IN HDC hDC,
    IN OPTIONAL HANDLE hSection,
    IN DWORD dwOffset,
    IN BITMAPINFO* bmi,
    IN DWORD Usage,
    IN UINT cjHeader,
    IN FLONG fl,
    IN ULONG_PTR dwColorSpace,
    OUT PVOID *Bits)
{
    HBITMAP hbitmap = 0;
    DC *dc;
    BOOL bDesktopDC = FALSE;
	NTSTATUS Status = STATUS_SUCCESS;

    if (!bmi) return hbitmap; // Make sure.

	_SEH2_TRY
    {
        ProbeForRead(&bmi->bmiHeader.biSize, sizeof(DWORD), 1);
		ProbeForRead(bmi, bmi->bmiHeader.biSize, 1);
		ProbeForRead(bmi, DIB_BitmapInfoSize(bmi, Usage), 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    if(!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return NULL;
    }

    // If the reference hdc is null, take the desktop dc
    if (hDC == 0)
    {
        hDC = NtGdiCreateCompatibleDC(0);
        bDesktopDC = TRUE;
    }

    if ((dc = DC_LockDc(hDC)))
    {
        hbitmap = DIB_CreateDIBSection(dc,
                                       bmi,
                                       Usage,
                                       Bits,
                                       hSection,
                                       dwOffset,
                                       0);
        DC_UnlockDc(dc);
    }
    else
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
    }

    if (bDesktopDC)
        NtGdiDeleteObjectApp(hDC);

    return hbitmap;
}

HBITMAP
APIENTRY
DIB_CreateDIBSection(
    PDC dc,
    CONST BITMAPINFO *bmi,
    UINT usage,
    LPVOID *bits,
    HANDLE section,
    DWORD offset,
    DWORD ovr_pitch)
{
    HBITMAP res = 0;
    SURFACE *bmp = NULL;
    void *mapBits = NULL;
    HPALETTE hpal ;

    // Fill BITMAP32 structure with DIB data
    CONST BITMAPINFOHEADER *bi = &bmi->bmiHeader;
    INT effHeight;
    ULONG totalSize;
    BITMAP bm;
    SIZEL Size;
    HANDLE hSecure;

    DPRINT("format (%ld,%ld), planes %d, bpp %d, size %ld, colors %ld (%s)\n",
           bi->biWidth, bi->biHeight, bi->biPlanes, bi->biBitCount,
           bi->biSizeImage, bi->biClrUsed, usage == DIB_PAL_COLORS? "PAL" : "RGB");

    /* CreateDIBSection should fail for compressed formats */
    if (bi->biCompression == BI_RLE4 || bi->biCompression == BI_RLE8)
    {
        return (HBITMAP)NULL;
    }

    effHeight = bi->biHeight >= 0 ? bi->biHeight : -bi->biHeight;
    bm.bmType = 0;
    bm.bmWidth = bi->biWidth;
    bm.bmHeight = effHeight;
    bm.bmWidthBytes = ovr_pitch ? ovr_pitch : WIDTH_BYTES_ALIGN32(bm.bmWidth, bi->biBitCount);

    bm.bmPlanes = bi->biPlanes;
    bm.bmBitsPixel = bi->biBitCount;
    bm.bmBits = NULL;

    // Get storage location for DIB bits.  Only use biSizeImage if it's valid and
    // we're dealing with a compressed bitmap.  Otherwise, use width * height.
    totalSize = bi->biSizeImage && bi->biCompression != BI_RGB
                ? bi->biSizeImage : (ULONG)(bm.bmWidthBytes * effHeight);

    if (section)
    {
        SYSTEM_BASIC_INFORMATION Sbi;
        NTSTATUS Status;
        DWORD mapOffset;
        LARGE_INTEGER SectionOffset;
        SIZE_T mapSize;

        Status = ZwQuerySystemInformation(SystemBasicInformation,
                                          &Sbi,
                                          sizeof Sbi,
                                          0);
        if (!NT_SUCCESS(Status))
        {
            return NULL;
        }

        mapOffset = offset - (offset % Sbi.AllocationGranularity);
        mapSize = bi->biSizeImage + (offset - mapOffset);

        SectionOffset.LowPart  = mapOffset;
        SectionOffset.HighPart = 0;

        Status = ZwMapViewOfSection(section,
                                    NtCurrentProcess(),
                                    &mapBits,
                                    0,
                                    0,
                                    &SectionOffset,
                                    &mapSize,
                                    ViewShare,
                                    0,
                                    PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
        {
            EngSetLastError(ERROR_INVALID_PARAMETER);
            return NULL;
        }

        if (mapBits) bm.bmBits = (char *)mapBits + (offset - mapOffset);
    }
    else if (ovr_pitch && offset)
        bm.bmBits = (LPVOID) offset;
    else
    {
        offset = 0;
        bm.bmBits = EngAllocUserMem(totalSize, 0);
		if(!bm.bmBits) goto cleanup;
    }

//  hSecure = MmSecureVirtualMemory(bm.bmBits, totalSize, PAGE_READWRITE);
    hSecure = (HANDLE)0x1; // HACK OF UNIMPLEMENTED KERNEL STUFF !!!!

    if (usage == DIB_PAL_COLORS)
    {
		if(dc)
		{
			PPALETTE pdcPal ;
			pdcPal = PALETTE_LockPalette(dc->dclevel.hpal);
			hpal = DIB_MapPaletteColors(pdcPal, bmi);
			PALETTE_UnlockPalette(pdcPal);
		}
		else
		{
			/* For DIB Brushes */
			DPRINT1("FIXME : Unsupported DIB_PAL_COLORS without a DC to map colors.\n");
			/* HACK */
			hpal = (HPALETTE) 0xFFFFFFFF;
		}
    }
    else
	{
        hpal = BuildDIBPalette(bmi);
	}

	if(!hpal)
	{
		DPRINT1("Error : Could not create a palette for the DIB.\n");
		goto cleanup;
	}

    // Create Device Dependent Bitmap and add DIB pointer
    Size.cx = bm.bmWidth;
    Size.cy = abs(bm.bmHeight);
    res = GreCreateBitmapEx(bm.bmWidth,
                            abs(bm.bmHeight),
                            bm.bmWidthBytes,
                            BitmapFormat(bi->biBitCount * bi->biPlanes, bi->biCompression),
                            BMF_DONTCACHE | BMF_USERMEM | BMF_NOZEROINIT |
                              (bi->biHeight < 0 ? BMF_TOPDOWN : 0),
                            bi->biSizeImage,
                            bm.bmBits,
							0);
    if (!res)
    {
        EngSetLastError(ERROR_NO_SYSTEM_RESOURCES);
        goto cleanup;
    }
    bmp = SURFACE_LockSurface(res);
    if (NULL == bmp)
    {
		EngSetLastError(ERROR_INVALID_HANDLE);
        goto cleanup;
    }

    /* WINE NOTE: WINE makes use of a colormap, which is a color translation
                  table between the DIB and the X physical device. Obviously,
                  this is left out of the ReactOS implementation. Instead,
                  we call NtGdiSetDIBColorTable. */
    bmp->hDIBSection = section;
    bmp->hSecure = hSecure;
    bmp->dwOffset = offset;
    bmp->flags = API_BITMAP;
    bmp->biClrImportant = bi->biClrImportant;

	/* HACK */
	if(hpal != (HPALETTE)0xFFFFFFFF)
	{
		bmp->ppal = PALETTE_ShareLockPalette(hpal);
		/* Lazy delete hpal, it will be freed at surface release */
		GreDeleteObject(hpal);
	}

    // Clean up in case of errors
cleanup:
    if (!res || !bmp || !bm.bmBits)
    {
        DPRINT("got an error res=%08x, bmp=%p, bm.bmBits=%p\n", res, bmp, bm.bmBits);
        if (bm.bmBits)
        {
            // MmUnsecureVirtualMemory(hSecure); // FIXME: Implement this!
            if (section)
            {
                ZwUnmapViewOfSection(NtCurrentProcess(), mapBits);
                bm.bmBits = NULL;
            }
            else
                if (!offset)
                    EngFreeUserMem(bm.bmBits), bm.bmBits = NULL;
        }

        if (bmp)
            bmp = NULL;

        if (res)
        {
            SURFACE_FreeSurfaceByHandle(res);
            res = 0;
        }
    }

    if (bmp)
    {
        SURFACE_UnlockSurface(bmp);
    }

    // Return BITMAP handle and storage location
    if (NULL != bm.bmBits && NULL != bits)
    {
        *bits = bm.bmBits;
    }

    return res;
}

/***********************************************************************
 *           DIB_GetBitmapInfo
 *
 * Get the info from a bitmap header.
 * Return 0 for COREHEADER, 1 for INFOHEADER, -1 for error.
 */
int
FASTCALL
DIB_GetBitmapInfo( const BITMAPINFOHEADER *header, LONG *width,
                       LONG *height, WORD *planes, WORD *bpp, DWORD *compr, DWORD *size )
{
    if (header->biSize == sizeof(BITMAPCOREHEADER))
    {
        const BITMAPCOREHEADER *core = (const BITMAPCOREHEADER *)header;
        *width  = core->bcWidth;
        *height = core->bcHeight;
        *planes = core->bcPlanes;
        *bpp    = core->bcBitCount;
        *compr  = BI_RGB;
        *size   = 0;
        return 0;
    }
    if (header->biSize >= sizeof(BITMAPINFOHEADER)) /* assume BITMAPINFOHEADER */
    {
        *width  = header->biWidth;
        *height = header->biHeight;
        *planes = header->biPlanes;
        *bpp    = header->biBitCount;
        *compr  = header->biCompression;
        *size   = header->biSizeImage;
        return 1;
    }
    DPRINT1("(%d): unknown/wrong size for header\n", header->biSize );
    return -1;
}

/***********************************************************************
 *           DIB_GetDIBImageBytes
 *
 * Return the number of bytes used to hold the image in a DIB bitmap.
 * 11/16/1999 (RJJ) lifted from wine
 */

INT APIENTRY DIB_GetDIBImageBytes(INT  width, INT height, INT depth)
{
    return WIDTH_BYTES_ALIGN32(width, depth) * (height < 0 ? -height : height);
}

/***********************************************************************
 *           DIB_BitmapInfoSize
 *
 * Return the size of the bitmap info structure including color table.
 * 11/16/1999 (RJJ) lifted from wine
 */

INT FASTCALL DIB_BitmapInfoSize(const BITMAPINFO * info, WORD coloruse)
{
    unsigned int colors, size, masks = 0;

    if (info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        const BITMAPCOREHEADER *core = (const BITMAPCOREHEADER *)info;
        colors = (core->bcBitCount <= 8) ? 1 << core->bcBitCount : 0;
        return sizeof(BITMAPCOREHEADER) + colors *
             ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBTRIPLE) : sizeof(WORD));
    }
    else  /* assume BITMAPINFOHEADER */
    {
        colors = info->bmiHeader.biClrUsed;
        if (colors > 256) colors = 256;
        if (!colors && (info->bmiHeader.biBitCount <= 8))
            colors = 1 << info->bmiHeader.biBitCount;
        if (info->bmiHeader.biCompression == BI_BITFIELDS) masks = 3;
        size = max( info->bmiHeader.biSize, sizeof(BITMAPINFOHEADER) + masks * sizeof(DWORD) );
        return size + colors * ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBQUAD) : sizeof(WORD));
    }
}

HPALETTE
FASTCALL
DIB_MapPaletteColors(PPALETTE ppal, CONST BITMAPINFO* lpbmi)
{
    PALETTEENTRY* ppalEntries;
    ULONG nNumColors,i;
    USHORT *lpIndex;
	HPALETTE hpal;

    if (!(ppal->flFlags & PAL_INDEXED))
    {
        return NULL;
    }

    nNumColors = 1 << lpbmi->bmiHeader.biBitCount;
    if (lpbmi->bmiHeader.biClrUsed)
    {
        nNumColors = min(nNumColors, lpbmi->bmiHeader.biClrUsed);
    }

	/* Don't have more colors than we need */
	nNumColors = min(ppal->NumColors, nNumColors);

    ppalEntries = ExAllocatePoolWithTag(PagedPool, sizeof(PALETTEENTRY) * nNumColors, TAG_COLORMAP);
    if (ppalEntries == NULL)
    {
        DPRINT1("Could not allocate palette entries\n");
        return NULL;
    }

    lpIndex = (USHORT *)((PBYTE)lpbmi + lpbmi->bmiHeader.biSize);

    for (i = 0; i < nNumColors; i++)
    {
        if (*lpIndex < ppal->NumColors)
        {
            ppalEntries[i] = ppal->IndexedColors[*lpIndex];
        }
        else
        {
            ppalEntries[i].peRed = 0;
			ppalEntries[i].peGreen = 0;
			ppalEntries[i].peBlue = 0;
			ppalEntries[i].peFlags = 0;
        }

        lpIndex++;
    }

	hpal = PALETTE_AllocPalette(PAL_INDEXED, nNumColors, (ULONG*)ppalEntries, 0, 0, 0);

	ExFreePoolWithTag(ppalEntries, TAG_COLORMAP);

	return hpal;
}

HPALETTE
FASTCALL
BuildDIBPalette(CONST BITMAPINFO *bmi)
{
    BYTE bits;
    ULONG ColorCount;
    HPALETTE hPal;
    ULONG RedMask = 0, GreenMask = 0, BlueMask = 0;
    PDWORD pdwColors = (PDWORD)((PBYTE)bmi + bmi->bmiHeader.biSize);
	INT paletteType;

    // Determine Bits Per Pixel
    bits = bmi->bmiHeader.biBitCount;

    // Determine paletteType from Bits Per Pixel
    if (bits <= 8)
    {
        paletteType = PAL_INDEXED;
        RedMask = GreenMask = BlueMask = 0;
    }
    else if (bmi->bmiHeader.biCompression == BI_BITFIELDS)
    {
        paletteType = PAL_BITFIELDS;
        RedMask = pdwColors[0];
        GreenMask = pdwColors[1];
        BlueMask = pdwColors[2];
    }
    else
    {
        paletteType = PAL_BITFIELDS;
        switch (bits)
        {
            case 15:
                paletteType |= PAL_RGB16_555;
                RedMask = 0x7C00;
                GreenMask = 0x03E0;
                BlueMask = 0x001F;
                break;

            case 16:
                paletteType |= PAL_RGB16_565;
                RedMask = 0xF800;
                GreenMask = 0x07E0;
                BlueMask = 0x001F;
                break;

            case 24:
            case 32:
                paletteType |= PAL_BGR;
                RedMask = 0xFF0000;
                GreenMask = 0x00FF00;
                BlueMask = 0x0000FF;
                break;
        }
    }

    if (bmi->bmiHeader.biClrUsed == 0)
    {
        ColorCount = 1 << bmi->bmiHeader.biBitCount;
    }
    else
    {
        ColorCount = bmi->bmiHeader.biClrUsed;
    }

    if (PAL_INDEXED == paletteType)
    {
        hPal = PALETTE_AllocPaletteIndexedRGB(ColorCount, (RGBQUAD*)pdwColors);
    }
    else
    {
        hPal = PALETTE_AllocPalette(paletteType, 0,
                                    NULL,
                                    RedMask, GreenMask, BlueMask);
    }

    return hPal;
}

/* Converts a BITMAPCOREINFO to a BITMAPINFO structure,
 * or does nothing if it's already a BITMAPINFO (or V4 or V5) */
BITMAPINFO*
FASTCALL
DIB_ConvertBitmapInfo (CONST BITMAPINFO* pbmi, DWORD Usage)
{
	CONST BITMAPCOREINFO* pbmci = (BITMAPCOREINFO*)pbmi;
	BITMAPINFO* pNewBmi ;
	UINT numColors = 0, ColorsSize = 0;

	if(pbmi->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER)) return (BITMAPINFO*)pbmi;
	if(pbmi->bmiHeader.biSize != sizeof(BITMAPCOREHEADER)) return NULL;

	if(pbmci->bmciHeader.bcBitCount <= 8)
	{
		numColors = 1 << pbmci->bmciHeader.bcBitCount;
		if(Usage == DIB_PAL_COLORS)
		{
			ColorsSize = numColors * sizeof(WORD);
		}
		else
		{
			ColorsSize = numColors * sizeof(RGBQUAD);
		}
	}
	else if (Usage == DIB_PAL_COLORS)
	{
		/* Invalid at high Res */
		return NULL;
	}

	pNewBmi = ExAllocatePoolWithTag(PagedPool, sizeof(BITMAPINFOHEADER) + ColorsSize, TAG_DIB);
	if(!pNewBmi) return NULL;

	RtlZeroMemory(pNewBmi, sizeof(BITMAPINFOHEADER) + ColorsSize);

	pNewBmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pNewBmi->bmiHeader.biBitCount = pbmci->bmciHeader.bcBitCount;
	pNewBmi->bmiHeader.biWidth = pbmci->bmciHeader.bcWidth;
	pNewBmi->bmiHeader.biHeight = pbmci->bmciHeader.bcHeight;
	pNewBmi->bmiHeader.biPlanes = pbmci->bmciHeader.bcPlanes;
	pNewBmi->bmiHeader.biCompression = BI_RGB ;
	pNewBmi->bmiHeader.biSizeImage = DIB_GetDIBImageBytes(pNewBmi->bmiHeader.biWidth,
												pNewBmi->bmiHeader.biHeight,
												pNewBmi->bmiHeader.biBitCount);

	if(Usage == DIB_PAL_COLORS)
	{
		RtlCopyMemory(pNewBmi->bmiColors, pbmci->bmciColors, ColorsSize);
	}
	else
	{
		UINT i;
		for(i=0; i<numColors; i++)
		{
			pNewBmi->bmiColors[i].rgbRed = pbmci->bmciColors[i].rgbtRed;
			pNewBmi->bmiColors[i].rgbGreen = pbmci->bmciColors[i].rgbtGreen;
			pNewBmi->bmiColors[i].rgbBlue = pbmci->bmciColors[i].rgbtBlue;
		}
	}

	return pNewBmi ;
}

/* Frees a BITMAPINFO created with DIB_ConvertBitmapInfo */
VOID
FASTCALL
DIB_FreeConvertedBitmapInfo(BITMAPINFO* converted, BITMAPINFO* orig)
{
	if(converted != orig)
		ExFreePoolWithTag(converted, TAG_DIB);
}






/* EOF */
