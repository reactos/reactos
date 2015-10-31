/*
 * COPYRIGHT:        GNU GPL, See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Bitmap functions
 * FILE:             win32ss/gdi/ntgdi/bitmaps.c
 * PROGRAMER:        Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

BOOL
NTAPI
GreSetBitmapOwner(
    _In_ HBITMAP hbmp,
    _In_ ULONG ulOwner)
{
    /* Check if we have the correct object type */
    if (GDI_HANDLE_GET_TYPE(hbmp) != GDILoObjType_LO_BITMAP_TYPE)
    {
        DPRINT1("Incorrect type for hbmp: %p\n", hbmp);
        return FALSE;
    }

    /// FIXME: this is a hack and doesn't handle a race condition properly.
    /// It needs to be done in GDIOBJ_vSetObjectOwner atomically.

    /* Check if we set public or none */
    if ((ulOwner == GDI_OBJ_HMGR_PUBLIC) ||
        (ulOwner == GDI_OBJ_HMGR_NONE))
    {
        /* Only allow this for owned objects */
        if (GreGetObjectOwner(hbmp) != GDI_OBJ_HMGR_POWNED)
        {
            DPRINT1("Cannot change owner for non-powned hbmp\n");
            return FALSE;
        }
    }

    return GreSetObjectOwner(hbmp, ulOwner);
}

static
int
NTAPI
UnsafeSetBitmapBits(
    PSURFACE psurf,
    IN ULONG cjBits,
    IN PVOID pvBits)
{
    PUCHAR pjDst, pjSrc;
    LONG lDeltaDst, lDeltaSrc;
    ULONG nWidth, nHeight, cBitsPixel;
    NT_ASSERT(psurf->flags & API_BITMAP);
    NT_ASSERT(psurf->SurfObj.iBitmapFormat <= BMF_32BPP);

    nWidth = psurf->SurfObj.sizlBitmap.cx;
    nHeight = psurf->SurfObj.sizlBitmap.cy;
    cBitsPixel = BitsPerFormat(psurf->SurfObj.iBitmapFormat);

    /* Get pointers */
    pjDst = psurf->SurfObj.pvScan0;
    pjSrc = pvBits;
    lDeltaDst = psurf->SurfObj.lDelta;
    lDeltaSrc = WIDTH_BYTES_ALIGN16(nWidth, cBitsPixel);
    NT_ASSERT(lDeltaSrc <= abs(lDeltaDst));

    /* Make sure the buffer is large enough*/
    if (cjBits < (lDeltaSrc * nHeight))
        return 0;

    while (nHeight--)
    {
        /* Copy one line */
        memcpy(pjDst, pjSrc, lDeltaSrc);
        pjSrc += lDeltaSrc;
        pjDst += lDeltaDst;
    }

    return 1;
}

HBITMAP
NTAPI
GreCreateBitmapEx(
    _In_ ULONG nWidth,
    _In_ ULONG nHeight,
    _In_ ULONG cjWidthBytes,
    _In_ ULONG iFormat,
    _In_ USHORT fjBitmap,
    _In_ ULONG cjSizeImage,
    _In_opt_ PVOID pvBits,
    _In_ FLONG flags)
{
    PSURFACE psurf;
    HBITMAP hbmp;
    PVOID pvCompressedBits = NULL;

    /* Verify format */
    if (iFormat < BMF_1BPP || iFormat > BMF_PNG) return NULL;

    /* The infamous RLE hack */
    if ((iFormat == BMF_4RLE) || (iFormat == BMF_8RLE))
    {
        pvCompressedBits = pvBits;
        pvBits = NULL;
        iFormat = (iFormat == BMF_4RLE) ? BMF_4BPP : BMF_8BPP;
    }

    /* Allocate a surface */
    psurf = SURFACE_AllocSurface(STYPE_BITMAP,
                                 nWidth,
                                 nHeight,
                                 iFormat,
                                 fjBitmap,
                                 cjWidthBytes,
                                 pvCompressedBits ? 0 : cjSizeImage,
                                 pvBits);
    if (!psurf)
    {
        DPRINT1("SURFACE_AllocSurface failed.\n");
        return NULL;
    }

    /* The infamous RLE hack */
    if (pvCompressedBits)
    {
        SIZEL sizl;
        LONG lDelta;

        sizl.cx = nWidth;
        sizl.cy = nHeight;
        lDelta = WIDTH_BYTES_ALIGN32(nWidth, gajBitsPerFormat[iFormat]);

        pvBits = psurf->SurfObj.pvBits;
        DecompressBitmap(sizl, pvCompressedBits, pvBits, lDelta, iFormat, cjSizeImage);
    }

    /* Get the handle for the bitmap */
    hbmp = (HBITMAP)psurf->SurfObj.hsurf;

    /* Mark as API bitmap */
    psurf->flags |= (flags | API_BITMAP);

    /* Unlock the surface and return */
    SURFACE_UnlockSurface(psurf);
    return hbmp;
}

/* Creates a DDB surface,
 * as in CreateCompatibleBitmap or CreateBitmap.
 * Note that each scanline must be 32bit aligned!
 */
HBITMAP
NTAPI
GreCreateBitmap(
    _In_ ULONG nWidth,
    _In_ ULONG nHeight,
    _In_ ULONG cPlanes,
    _In_ ULONG cBitsPixel,
    _In_opt_ PVOID pvBits)
{
    /* Call the extended function */
    return GreCreateBitmapEx(nWidth,
                             nHeight,
                             0, /* Auto width */
                             BitmapFormat(cBitsPixel * cPlanes, BI_RGB),
                             0, /* No bitmap flags */
                             0, /* Auto size */
                             pvBits,
                             DDB_SURFACE /* DDB */);
}

HBITMAP
APIENTRY
NtGdiCreateBitmap(
    IN INT nWidth,
    IN INT nHeight,
    IN UINT cPlanes,
    IN UINT cBitsPixel,
    IN OPTIONAL LPBYTE pUnsafeBits)
{
    HBITMAP hbmp;
    ULONG cRealBpp, cjWidthBytes, iFormat;
    ULONGLONG cjSize;
    PSURFACE psurf;

    /* Calculate bitmap format and real bits per pixel. */
    iFormat = BitmapFormat(cBitsPixel * cPlanes, BI_RGB);
    cRealBpp = gajBitsPerFormat[iFormat];

    /* Calculate width and image size in bytes */
    cjWidthBytes = WIDTH_BYTES_ALIGN16(nWidth, cRealBpp);
    cjSize = (ULONGLONG)cjWidthBytes * nHeight;

    /* Check parameters (possible overflow of cjSize!) */
    if ((iFormat == 0) || (nWidth <= 0) || (nWidth >= 0x8000000) || (nHeight <= 0) ||
        (cBitsPixel > 32) || (cPlanes > 32) || (cjSize >= 0x100000000ULL))
    {
        DPRINT1("Invalid bitmap format! Width=%d, Height=%d, Bpp=%u, Planes=%u\n",
                nWidth, nHeight, cBitsPixel, cPlanes);
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* Allocate the surface (but don't set the bits) */
    psurf = SURFACE_AllocSurface(STYPE_BITMAP,
                                 nWidth,
                                 nHeight,
                                 iFormat,
                                 0,
                                 0,
                                 0,
                                 NULL);
    if (!psurf)
    {
        DPRINT1("SURFACE_AllocSurface failed.\n");
        return NULL;
    }

    /* Mark as API and DDB bitmap */
    psurf->flags |= (API_BITMAP | DDB_SURFACE);

    /* Check if we have bits to set */
    if (pUnsafeBits)
    {
        /* Protect with SEH and copy the bits */
        _SEH2_TRY
        {
            ProbeForRead(pUnsafeBits, (SIZE_T)cjSize, 1);
            UnsafeSetBitmapBits(psurf, cjSize, pUnsafeBits);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            GDIOBJ_vDeleteObject(&psurf->BaseObject);
            _SEH2_YIELD(return NULL;)
        }
        _SEH2_END
    }
    else
    {
        /* Zero the bits */
        RtlZeroMemory(psurf->SurfObj.pvBits, psurf->SurfObj.cjBits);
    }

    /* Get the handle for the bitmap */
    hbmp = (HBITMAP)psurf->SurfObj.hsurf;

    /* Unlock the surface */
    SURFACE_UnlockSurface(psurf);

    return hbmp;
}


HBITMAP FASTCALL
IntCreateCompatibleBitmap(
    PDC Dc,
    INT Width,
    INT Height,
    UINT Planes,
    UINT Bpp)
{
    HBITMAP Bmp = NULL;
    PPALETTE ppal;

    /* MS doc says if width or height is 0, return 1-by-1 pixel, monochrome bitmap */
    if (0 == Width || 0 == Height)
    {
        return NtGdiGetStockObject(DEFAULT_BITMAP);
    }

    if (Dc->dctype != DC_TYPE_MEMORY)
    {
        PSURFACE psurf;

        Bmp = GreCreateBitmap(abs(Width),
                              abs(Height),
                              Planes ? Planes : 1,
                              Bpp ? Bpp : Dc->ppdev->gdiinfo.cBitsPixel,
                              NULL);
        if (Bmp == NULL)
        {
            DPRINT1("Failed to allocate a bitmap!\n");
            return NULL;
        }

        psurf = SURFACE_ShareLockSurface(Bmp);
        ASSERT(psurf);

        /* Dereference old palette and set new palette */
        ppal = PALETTE_ShareLockPalette(Dc->ppdev->devinfo.hpalDefault);
        ASSERT(ppal);
        SURFACE_vSetPalette(psurf, ppal);
        PALETTE_ShareUnlockPalette(ppal);

        /* Set flags */
        psurf->flags = API_BITMAP;
        psurf->hdc = NULL; // FIXME:
        psurf->SurfObj.hdev = (HDEV)Dc->ppdev;
        SURFACE_ShareUnlockSurface(psurf);
    }
    else
    {
        DIBSECTION dibs;
        INT Count;
        PSURFACE psurf = Dc->dclevel.pSurface;
        if(!psurf) psurf = psurfDefaultBitmap;
        Count = BITMAP_GetObject(psurf, sizeof(dibs), &dibs);

        if (Count == sizeof(BITMAP))
        {
            PSURFACE psurfBmp;

            Bmp = GreCreateBitmap(abs(Width),
                          abs(Height),
                          Planes ? Planes : 1,
                          Bpp ? Bpp : dibs.dsBm.bmBitsPixel,
                          NULL);
            psurfBmp = SURFACE_ShareLockSurface(Bmp);
            ASSERT(psurfBmp);

            /* Dereference old palette and set new palette */
            SURFACE_vSetPalette(psurfBmp, psurf->ppal);

            /* Set flags */
            psurfBmp->flags = API_BITMAP;
            psurfBmp->hdc = NULL; // FIXME:
            psurf->SurfObj.hdev = (HDEV)Dc->ppdev;
            SURFACE_ShareUnlockSurface(psurfBmp);
        }
        else if (Count == sizeof(DIBSECTION))
        {
            /* A DIB section is selected in the DC */
            BYTE buf[sizeof(BITMAPINFOHEADER) + 256*sizeof(RGBQUAD)] = {0};
            PVOID Bits;
            BITMAPINFO* bi = (BITMAPINFO*)buf;

            bi->bmiHeader.biSize          = sizeof(bi->bmiHeader);
            bi->bmiHeader.biWidth         = Width;
            bi->bmiHeader.biHeight        = Height;
            bi->bmiHeader.biPlanes        = Planes ? Planes : dibs.dsBmih.biPlanes;
            bi->bmiHeader.biBitCount      = Bpp ? Bpp : dibs.dsBmih.biBitCount;
            bi->bmiHeader.biCompression   = dibs.dsBmih.biCompression;
            bi->bmiHeader.biSizeImage     = 0;
            bi->bmiHeader.biXPelsPerMeter = dibs.dsBmih.biXPelsPerMeter;
            bi->bmiHeader.biYPelsPerMeter = dibs.dsBmih.biYPelsPerMeter;
            bi->bmiHeader.biClrUsed       = dibs.dsBmih.biClrUsed;
            bi->bmiHeader.biClrImportant  = dibs.dsBmih.biClrImportant;

            if (bi->bmiHeader.biCompression == BI_BITFIELDS)
            {
                /* Copy the color masks */
                RtlCopyMemory(bi->bmiColors, dibs.dsBitfields, 3*sizeof(RGBQUAD));
            }
            else if (bi->bmiHeader.biBitCount <= 8)
            {
                /* Copy the color table */
                UINT Index;
                PPALETTE PalGDI;

                if (!psurf->ppal)
                {
                    EngSetLastError(ERROR_INVALID_HANDLE);
                    return 0;
                }

                PalGDI = psurf->ppal;

                for (Index = 0;
                        Index < 256 && Index < PalGDI->NumColors;
                        Index++)
                {
                    bi->bmiColors[Index].rgbRed   = PalGDI->IndexedColors[Index].peRed;
                    bi->bmiColors[Index].rgbGreen = PalGDI->IndexedColors[Index].peGreen;
                    bi->bmiColors[Index].rgbBlue  = PalGDI->IndexedColors[Index].peBlue;
                    bi->bmiColors[Index].rgbReserved = 0;
                }
            }

            Bmp = DIB_CreateDIBSection(Dc,
                                       bi,
                                       DIB_RGB_COLORS,
                                       &Bits,
                                       NULL,
                                       0,
                                       0);
            return Bmp;
        }
    }
    return Bmp;
}

HBITMAP APIENTRY
NtGdiCreateCompatibleBitmap(
    HDC hDC,
    INT Width,
    INT Height)
{
    HBITMAP Bmp;
    PDC Dc;

    /* Check parameters */
    if ((Width <= 0) || (Height <= 0) || ((Width * Height) > 0x3FFFFFFF))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (!hDC)
        return GreCreateBitmap(Width, Height, 1, 1, 0);

    Dc = DC_LockDc(hDC);

    DPRINT("NtGdiCreateCompatibleBitmap(%p,%d,%d, bpp:%u) = \n",
           hDC, Width, Height, Dc->ppdev->gdiinfo.cBitsPixel);

    if (NULL == Dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    Bmp = IntCreateCompatibleBitmap(Dc, Width, Height, 0, 0);

    DC_UnlockDc(Dc);
    return Bmp;
}

BOOL
APIENTRY
NtGdiGetBitmapDimension(
    HBITMAP hBitmap,
    LPSIZE psizDim)
{
    PSURFACE psurfBmp;
    BOOL bResult = TRUE;

    if (hBitmap == NULL)
        return FALSE;

    /* Lock the bitmap */
    psurfBmp = SURFACE_ShareLockSurface(hBitmap);
    if (psurfBmp == NULL)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    /* Use SEH to copy the data to the caller */
    _SEH2_TRY
    {
        ProbeForWrite(psizDim, sizeof(SIZE), 1);
        *psizDim = psurfBmp->sizlDim;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        bResult = FALSE;
    }
    _SEH2_END

    /* Unlock the bitmap */
    SURFACE_ShareUnlockSurface(psurfBmp);

    return bResult;
}


VOID
FASTCALL
UnsafeGetBitmapBits(
    PSURFACE psurf,
    DWORD Bytes,
    OUT PBYTE pvBits)
{
    PUCHAR pjDst, pjSrc;
    LONG lDeltaDst, lDeltaSrc;
    ULONG nWidth, nHeight, cBitsPixel;

    nWidth = psurf->SurfObj.sizlBitmap.cx;
    nHeight = psurf->SurfObj.sizlBitmap.cy;
    cBitsPixel = BitsPerFormat(psurf->SurfObj.iBitmapFormat);

    /* Get pointers */
    pjSrc = psurf->SurfObj.pvScan0;
    pjDst = pvBits;
    lDeltaSrc = psurf->SurfObj.lDelta;
    lDeltaDst = WIDTH_BYTES_ALIGN16(nWidth, cBitsPixel);

    while (nHeight--)
    {
        /* Copy one line */
        RtlCopyMemory(pjDst, pjSrc, lDeltaDst);
        pjSrc += lDeltaSrc;
        pjDst += lDeltaDst;
    }
}

LONG
APIENTRY
NtGdiGetBitmapBits(
    HBITMAP hBitmap,
    ULONG cjBuffer,
    OUT OPTIONAL PBYTE pUnsafeBits)
{
    PSURFACE psurf;
    ULONG cjSize;
    LONG ret;

    /* Check parameters */
    if (pUnsafeBits != NULL && cjBuffer == 0)
    {
        return 0;
    }

    /* Lock the bitmap */
    psurf = SURFACE_ShareLockSurface(hBitmap);
    if (!psurf)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    /* Calculate the size of the bitmap in bytes */
    cjSize = WIDTH_BYTES_ALIGN16(psurf->SurfObj.sizlBitmap.cx,
                BitsPerFormat(psurf->SurfObj.iBitmapFormat)) *
                abs(psurf->SurfObj.sizlBitmap.cy);

    /* If the bits vector is null, the function should return the read size */
    if (pUnsafeBits == NULL)
    {
        SURFACE_ShareUnlockSurface(psurf);
        return cjSize;
    }

    /* Don't copy more bytes than the buffer has */
    cjBuffer = min(cjBuffer, cjSize);

    // FIXME: Use MmSecureVirtualMemory
    _SEH2_TRY
    {
        ProbeForWrite(pUnsafeBits, cjBuffer, 1);
        UnsafeGetBitmapBits(psurf, cjBuffer, pUnsafeBits);
        ret = cjBuffer;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = 0;
    }
    _SEH2_END

    SURFACE_ShareUnlockSurface(psurf);

    return ret;
}


LONG APIENTRY
NtGdiSetBitmapBits(
    HBITMAP hBitmap,
    DWORD Bytes,
    IN PBYTE pUnsafeBits)
{
    LONG ret;
    PSURFACE psurf;

    if (pUnsafeBits == NULL || Bytes == 0)
    {
        return 0;
    }

    if (GDI_HANDLE_IS_STOCKOBJ(hBitmap))
    {
        return 0;
    }

    psurf = SURFACE_ShareLockSurface(hBitmap);
    if (psurf == NULL)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    if (((psurf->flags & API_BITMAP) == 0) ||
        (psurf->SurfObj.iBitmapFormat > BMF_32BPP))
    {
        DPRINT1("Invalid bitmap: iBitmapFormat = %lu, flags = 0x%lx\n",
                psurf->SurfObj.iBitmapFormat,
                psurf->flags);
        EngSetLastError(ERROR_INVALID_HANDLE);
        SURFACE_ShareUnlockSurface(psurf);
        return 0;
    }

    _SEH2_TRY
    {
        ProbeForRead(pUnsafeBits, Bytes, sizeof(WORD));
        ret = UnsafeSetBitmapBits(psurf, Bytes, pUnsafeBits);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = 0;
    }
    _SEH2_END

    SURFACE_ShareUnlockSurface(psurf);

    return ret;
}

BOOL APIENTRY
NtGdiSetBitmapDimension(
    HBITMAP hBitmap,
    INT Width,
    INT Height,
    LPSIZE Size)
{
    PSURFACE psurf;
    BOOL Ret = TRUE;

    if (hBitmap == NULL)
        return FALSE;

    psurf = SURFACE_ShareLockSurface(hBitmap);
    if (psurf == NULL)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (Size)
    {
        _SEH2_TRY
        {
            ProbeForWrite(Size, sizeof(SIZE), 1);
            *Size = psurf->sizlDim;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Ret = FALSE;
        }
        _SEH2_END
    }

    /* The dimension is changed even if writing the old value failed */
    psurf->sizlDim.cx = Width;
    psurf->sizlDim.cy = Height;

    SURFACE_ShareUnlockSurface(psurf);

    return Ret;
}

/*  Internal Functions  */

HBITMAP
FASTCALL
BITMAP_CopyBitmap(HBITMAP hBitmap)
{
    HBITMAP hbmNew;
    SURFACE *psurfSrc, *psurfNew;

    /* Fail, if no source bitmap is given */
    if (hBitmap == NULL) return 0;

    /* Lock the source bitmap */
    psurfSrc = SURFACE_ShareLockSurface(hBitmap);
    if (psurfSrc == NULL)
    {
        return 0;
    }

    /* Allocate a new bitmap with the same dimensions as the source bmp */
    hbmNew = GreCreateBitmapEx(psurfSrc->SurfObj.sizlBitmap.cx,
                               psurfSrc->SurfObj.sizlBitmap.cy,
                               abs(psurfSrc->SurfObj.lDelta),
                               psurfSrc->SurfObj.iBitmapFormat,
                               psurfSrc->SurfObj.fjBitmap & BMF_TOPDOWN,
                               psurfSrc->SurfObj.cjBits,
                               NULL,
                               psurfSrc->flags);

    if (hbmNew)
    {
        /* Lock the new bitmap */
        psurfNew = SURFACE_ShareLockSurface(hbmNew);
        if (psurfNew)
        {
            /* Copy the bitmap bits to the new bitmap buffer */
            RtlCopyMemory(psurfNew->SurfObj.pvBits,
                          psurfSrc->SurfObj.pvBits,
                          psurfNew->SurfObj.cjBits);


            /* Reference the palette of the source bitmap and use it */
            SURFACE_vSetPalette(psurfNew, psurfSrc->ppal);

            /* Unlock the new surface */
            SURFACE_ShareUnlockSurface(psurfNew);
        }
        else
        {
            /* Failed to lock the bitmap, shouldn't happen */
            GreDeleteObject(hbmNew);
            hbmNew = NULL;
        }
    }

    /* Unlock the source bitmap and return the handle of the new bitmap */
    SURFACE_ShareUnlockSurface(psurfSrc);
    return hbmNew;
}

INT APIENTRY
BITMAP_GetObject(SURFACE *psurf, INT Count, LPVOID buffer)
{
    PBITMAP pBitmap;

    if (!buffer) return sizeof(BITMAP);
    if ((UINT)Count < sizeof(BITMAP)) return 0;

    /* Always fill a basic BITMAP structure */
    pBitmap = buffer;
    pBitmap->bmType = 0;
    pBitmap->bmWidth = psurf->SurfObj.sizlBitmap.cx;
    pBitmap->bmHeight = psurf->SurfObj.sizlBitmap.cy;
    pBitmap->bmPlanes = 1;
    pBitmap->bmBitsPixel = BitsPerFormat(psurf->SurfObj.iBitmapFormat);
    pBitmap->bmWidthBytes = WIDTH_BYTES_ALIGN16(pBitmap->bmWidth, pBitmap->bmBitsPixel);

    /* Check for DIB section */
    if (psurf->hSecure)
    {
        /* Set bmBits in this case */
        pBitmap->bmBits = psurf->SurfObj.pvBits;
        /* DIBs data are 32 bits aligned */
        pBitmap->bmWidthBytes = WIDTH_BYTES_ALIGN32(pBitmap->bmWidth, pBitmap->bmBitsPixel);

        if (Count >= sizeof(DIBSECTION))
        {
            /* Fill rest of DIBSECTION */
            PDIBSECTION pds = buffer;

            pds->dsBmih.biSize = sizeof(BITMAPINFOHEADER);
            pds->dsBmih.biWidth = pds->dsBm.bmWidth;
            pds->dsBmih.biHeight = pds->dsBm.bmHeight;
            pds->dsBmih.biPlanes = pds->dsBm.bmPlanes;
            pds->dsBmih.biBitCount = pds->dsBm.bmBitsPixel;

            switch (psurf->SurfObj.iBitmapFormat)
            {
                case BMF_1BPP:
                case BMF_4BPP:
                case BMF_8BPP:
                   pds->dsBmih.biCompression = BI_RGB;
                   break;

                case BMF_16BPP:
                    if (psurf->ppal->flFlags & PAL_RGB16_555)
                        pds->dsBmih.biCompression = BI_RGB;
                    else
                        pds->dsBmih.biCompression = BI_BITFIELDS;
                    break;

                case BMF_24BPP:
                case BMF_32BPP:
                    /* 24/32bpp BI_RGB is actually BGR format */
                    if (psurf->ppal->flFlags & PAL_BGR)
                        pds->dsBmih.biCompression = BI_RGB;
                    else
                        pds->dsBmih.biCompression = BI_BITFIELDS;
                    break;

                case BMF_4RLE:
                   pds->dsBmih.biCompression = BI_RLE4;
                   break;
                case BMF_8RLE:
                   pds->dsBmih.biCompression = BI_RLE8;
                   break;
                case BMF_JPEG:
                   pds->dsBmih.biCompression = BI_JPEG;
                   break;
                case BMF_PNG:
                   pds->dsBmih.biCompression = BI_PNG;
                   break;
                default:
                    ASSERT(FALSE); /* This shouldn't happen */
            }

            pds->dsBmih.biSizeImage = psurf->SurfObj.cjBits;
            pds->dsBmih.biXPelsPerMeter = 0;
            pds->dsBmih.biYPelsPerMeter = 0;
            pds->dsBmih.biClrUsed = psurf->ppal->NumColors;
            pds->dsBmih.biClrImportant = psurf->biClrImportant;
            pds->dsBitfields[0] = psurf->ppal->RedMask;
            pds->dsBitfields[1] = psurf->ppal->GreenMask;
            pds->dsBitfields[2] = psurf->ppal->BlueMask;
            pds->dshSection = psurf->hDIBSection;
            pds->dsOffset = psurf->dwOffset;

            return sizeof(DIBSECTION);
        }
    }
    else
    {
        /* Not set according to wine test, confirmed in win2k */
        pBitmap->bmBits = NULL;
    }

    return sizeof(BITMAP);
}

/*
 * @implemented
 */
HDC
APIENTRY
NtGdiGetDCforBitmap(
    IN HBITMAP hsurf)
{
    HDC hdc = NULL;
    PSURFACE psurf = SURFACE_ShareLockSurface(hsurf);
    if (psurf)
    {
        hdc = psurf->hdc;
        SURFACE_ShareUnlockSurface(psurf);
    }
    return hdc;
}


/* EOF */
