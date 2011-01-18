/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/gre/bitmap.c
 * PURPOSE:         Surface and Bitmap Support Routines
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>, taken from ReactOS eng/surface.c
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

enum Rle_EscapeCodes
{
    RLE_EOL   = 0, /* End of line */
    RLE_END   = 1, /* End of bitmap */
    RLE_DELTA = 2  /* Delta */
};

ULONG
NTAPI
GrepBitmapFormat(WORD Bits, DWORD Compression)
{
    switch (Compression)
    {
        case BI_RGB:
            /* Fall through */
        case BI_BITFIELDS:
            switch (Bits)
            {
                case 1:
                    return BMF_1BPP;
                case 4:
                    return BMF_4BPP;
                case 8:
                    return BMF_8BPP;
                case 16:
                    return BMF_16BPP;
                case 24:
                    return BMF_24BPP;
                case 32:
                    return BMF_32BPP;
            }
            return 0;

        case BI_RLE4:
            return BMF_4RLE;

        case BI_RLE8:
            return BMF_8RLE;

        default:
            return 0;
    }
}

INT
FASTCALL
BitsPerFormat(ULONG Format)
{
    switch (Format)
    {
        case BMF_1BPP:
            return 1;

        case BMF_4BPP:
            /* Fall through */
        case BMF_4RLE:
            return 4;

        case BMF_8BPP:
            /* Fall through */
        case BMF_8RLE:
            return 8;

        case BMF_16BPP:
            return 16;

        case BMF_24BPP:
            return 24;

        case BMF_32BPP:
            return 32;

        default:
            return 0;
    }
}

INT FASTCALL
BITMAP_GetWidthBytes(INT bmWidth, INT bpp)
{
#if 0
    switch (bpp)
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

UINT FASTCALL
BITMAP_GetRealBitsPixel(UINT nBitsPixel)
{
    if (nBitsPixel <= 1)
        return 1;
    if (nBitsPixel <= 4)
        return 4;
    if (nBitsPixel <= 8)
        return 8;
    if (nBitsPixel <= 16)
        return 16;
    if (nBitsPixel <= 24)
        return 24;
    if (nBitsPixel <= 32)
        return 32;

    return 0;
}


INT APIENTRY
BITMAP_GetObject(SURFACE *psurf, INT Count, LPVOID buffer)
{
    PBITMAP pBitmap;

    if (!buffer) return sizeof(BITMAP);
    if ((UINT)Count < sizeof(BITMAP)) return 0;

    /* always fill a basic BITMAP structure */
    pBitmap = buffer;
    pBitmap->bmType = 0;
    pBitmap->bmWidth = psurf->SurfObj.sizlBitmap.cx;
    pBitmap->bmHeight = psurf->SurfObj.sizlBitmap.cy;
    pBitmap->bmWidthBytes = abs(psurf->SurfObj.lDelta);
    pBitmap->bmPlanes = 1;
    pBitmap->bmBitsPixel = BitsPerFormat(psurf->SurfObj.iBitmapFormat);
#if 0
    /* Check for DIB section */
    if (psurf->hSecure)
    {
        /* Set bmBits in this case */
        pBitmap->bmBits = psurf->SurfObj.pvBits;

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
                /* FIXME: What about BI_BITFIELDS? */
                case BMF_1BPP:
                case BMF_4BPP:
                case BMF_8BPP:
                case BMF_16BPP:
                case BMF_24BPP:
                case BMF_32BPP:
                   pds->dsBmih.biCompression = BI_RGB;
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
            }
            pds->dsBmih.biSizeImage = psurf->SurfObj.cjBits;
            pds->dsBmih.biXPelsPerMeter = 0;
            pds->dsBmih.biYPelsPerMeter = 0;
            //pds->dsBmih.biClrUsed = psurf->biClrUsed;
            //pds->dsBmih.biClrImportant = psurf->biClrImportant;
            //pds->dsBitfields[0] = psurf->dsBitfields[0];
            //pds->dsBitfields[1] = psurf->dsBitfields[1];
            //pds->dsBitfields[2] = psurf->dsBitfields[2];
            //pds->dshSection = psurf->hDIBSection;
            //pds->dsOffset = psurf->dwOffset;

            return sizeof(DIBSECTION);
        }
    }
    else
    {
        /* not set according to wine test, confirmed in win2k */
        pBitmap->bmBits = NULL;
    }
#else
    pBitmap->bmBits = NULL;
#endif

    return sizeof(BITMAP);
}


VOID DecompressBitmap(SIZEL Size, BYTE *CompressedBits, BYTE *UncompressedBits, LONG Delta, ULONG Format)
{
    INT x = 0;
    INT y = Size.cy - 1;
    INT c;
    INT length;
    INT width;
    INT height = Size.cy - 1;
    BYTE *begin = CompressedBits;
    BYTE *bits = CompressedBits;
    BYTE *temp;
    INT shift;

    switch (Format)
    {
    case BMF_4RLE:
        shift = 1;
        break;
    case BMF_8RLE:
        shift = 0;
        break;
    default:
        DPRINT1("Unsupported RLE format 0x%x\n", Format);
        return;
    }

    width = ((Size.cx + shift) >> shift);

    while (y >= 0)
    {
        length = (*bits++) >> shift;
        if (length)
        {
            c = *bits++;
            while (length--)
            {
                if (x >= width) break;
                temp = UncompressedBits + (((height - y) * Delta) + x);
                x++;
                *temp = c;
            }
        }
        else
        {
            length = *bits++;
            switch (length)
            {
                case RLE_EOL:
                    x = 0;
                    y--;
                    break;
                case RLE_END:
                    return;
                case RLE_DELTA:
                    x += (*bits++) >> shift;
                    y -= (*bits++) >> shift;
                    break;
                default:
                    length = length >> shift;
                    while (length--)
                    {
                        c = *bits++;
                        if (x < width)
                        {
                            temp = UncompressedBits + (((height - y) * Delta) + x);
                            x++;
                            *temp = c;
                        }
                    }
                    if ((bits - begin) & 1)
                    {
                        bits++;
                    }
            }
        }
    }
}

/* PUBLIC FUNCTIONS **********************************************************/

HBITMAP FASTCALL
BITMAP_CopyBitmap(HBITMAP hBitmap)
{
    HBITMAP res;
    BITMAP bm;
    SURFACE *Bitmap, *resBitmap;
    SIZEL Size;

    if (hBitmap == NULL)
    {
        return 0;
    }

    Bitmap = GDIOBJ_LockObj(hBitmap, GDI_OBJECT_TYPE_BITMAP);
    if (Bitmap == NULL)
    {
        return 0;
    }

    BITMAP_GetObject(Bitmap, sizeof(BITMAP), (PVOID)&bm);
    bm.bmBits = NULL;
    if (Bitmap->SurfObj.lDelta >= 0)
        bm.bmHeight = -bm.bmHeight;

    Size.cx = abs(bm.bmWidth);
    Size.cy = abs(bm.bmHeight);
    res = GreCreateBitmap(Size,
                          bm.bmWidthBytes,
                          GrepBitmapFormat(bm.bmBitsPixel * bm.bmPlanes, BI_RGB),
                          (bm.bmHeight < 0 ? BMF_TOPDOWN : 0) | BMF_NOZEROINIT,
                          NULL);

    if (res)
    {
        PBYTE buf;

        resBitmap = GDIOBJ_LockObj(res, GDI_OBJECT_TYPE_BITMAP);
        if (resBitmap)
        {
            buf = ExAllocatePoolWithTag(PagedPool,
                                        bm.bmWidthBytes * abs(bm.bmHeight),
                                        TAG_BITMAP);
            if (buf == NULL)
            {
                GDIOBJ_UnlockObjByPtr((POBJ)resBitmap);
                GDIOBJ_UnlockObjByPtr((POBJ)Bitmap);
                GreDeleteObject(res);
                return 0;
            }
            GreGetBitmapBits(Bitmap, bm.bmWidthBytes * abs(bm.bmHeight), buf);
            GreSetBitmapBits(resBitmap, bm.bmWidthBytes * abs(bm.bmHeight), buf);
            ExFreePoolWithTag(buf,TAG_BITMAP);
            resBitmap->ulFlags = Bitmap->ulFlags;
            GDIOBJ_UnlockObjByPtr((POBJ)resBitmap);
        }
        else
        {
            GreDeleteObject(res);
            res = NULL;
        }
    }

    GDIOBJ_UnlockObjByPtr((POBJ)Bitmap);

    return  res;
}

HBITMAP
GreCreateBitmap(IN SIZEL Size,
                IN LONG Width,
                IN ULONG Format,
                IN ULONG Flags,
                IN PVOID Bits)
{
    PSURFACE pSurface;
    SURFOBJ *pSurfObj;
    HBITMAP hSurface;

    if (Format == 0)
        return 0;

    /* Allocate storage for surface object */
    pSurface = (PSURFACE)GDIOBJ_AllocObjWithHandle(GDI_OBJECT_TYPE_BITMAP);
    if (!pSurface) return 0;

    /* Save a handle to it */
    hSurface = pSurface->BaseObject.hHmgr;

    /* Initialize SURFOBJ */
    pSurfObj = &pSurface->SurfObj;

    pSurfObj->hsurf = (HSURF)hSurface;
    pSurfObj->sizlBitmap = Size;
    pSurfObj->iType = STYPE_BITMAP;
    pSurfObj->fjBitmap = Flags & (BMF_TOPDOWN | BMF_NOZEROINIT);

    /* Check the format */
    if (Format == BMF_4RLE || Format == BMF_8RLE)
    {
        PVOID UncompressedBits;
        pSurfObj->lDelta = DIB_GetDIBWidthBytes(Size.cx, BitsPerFormat(Format));
        pSurfObj->cjBits = pSurfObj->lDelta * Size.cy;

        UncompressedBits = EngAllocMem(FL_ZERO_MEMORY, pSurfObj->cjBits, TAG_DIB);
        if(!UncompressedBits)
        {
            /* Cleanup and exit */
            GDIOBJ_FreeObjByHandle(hSurface, GDI_OBJECT_TYPE_BITMAP);
            return 0;
        }
        if(Bits)
        {
            DecompressBitmap(Size, (BYTE *)Bits, (BYTE *)UncompressedBits, pSurfObj->lDelta, Format);
        }
        pSurfObj->pvBits = UncompressedBits;
        Format = (Format == BMF_4RLE)? BMF_4BPP : BMF_8BPP;
        /* Indicate we allocated memory ourselves */
        pSurface->ulFlags |= SRF_BITSALLOCD;
    }
    else
    {
        /* Calculate byte width automatically if it was not provided */
        if (Width == 0)
            Width = BITMAP_GetWidthBytes(Size.cx, BitsPerFormat(Format));
        pSurfObj->lDelta = abs(Width);
        pSurfObj->cjBits = pSurfObj->lDelta * Size.cy;

        if (!Bits)
        {
            /* Allocate memory for bitmap bits */
            pSurfObj->pvBits = EngAllocMem(0 != (Flags & BMF_NOZEROINIT) ? 0 : FL_ZERO_MEMORY,
                pSurfObj->cjBits,
                TAG_DIB);
            if (!pSurfObj->pvBits)
            {
                /* Cleanup and exit */
                GDIOBJ_FreeObjByHandle(hSurface, GDI_OBJECT_TYPE_BITMAP);
                return 0;
            }

            /* Indicate we allocated memory ourselves */
            pSurface->ulFlags |= SRF_BITSALLOCD;
        }
        else
        {
            pSurfObj->pvBits = Bits;
        }
    }

    pSurfObj->pvScan0 = pSurfObj->pvBits;

    /* Override the 0th scanline if it's not topdown */
    if (!(Flags & BMF_TOPDOWN))
    {
        pSurfObj->pvScan0 = (PVOID)((ULONG_PTR)pSurfObj->pvBits + pSurfObj->cjBits - pSurfObj->lDelta);
        pSurfObj->lDelta = -pSurfObj->lDelta;
    }

    /* Set the format */
    pSurfObj->iBitmapFormat = Format;

    /* Initialize bits lock */
    pSurface->pBitsLock = ExAllocatePoolWithTag(NonPagedPool,
                                                sizeof(FAST_MUTEX),
                                                TAG_SURFOBJ);

    if (!pSurface->pBitsLock)
    {
        /* Cleanup and return */
        if (!Bits && (pSurface->ulFlags & SRF_BITSALLOCD))
            EngFreeMem(pSurfObj->pvBits);
        GDIOBJ_FreeObjByHandle(hSurface, GDI_OBJECT_TYPE_BITMAP);
        return 0;
    }

    ExInitializeFastMutex(pSurface->pBitsLock);

    /* Unlock the surface */
    SURFACE_UnlockSurface(pSurface);

    /* Return handle to it */
    return hSurface;
}

                
HBITMAP APIENTRY
IntGdiCreateBitmap(
    INT Width,
    INT Height,
    UINT Planes,
    UINT BitsPixel,
    IN OPTIONAL LPBYTE pBits)
{
    HBITMAP hBitmap;
    SIZEL Size;
    LONG WidthBytes;
    PSURFACE psurfBmp;

    /* NOTE: Windows also doesn't store nr. of planes separately! */
    BitsPixel = BITMAP_GetRealBitsPixel(BitsPixel * Planes);

    /* Check parameters */
    if (BitsPixel == 0 || Width <= 0 || Width >= 0x8000000 || Height == 0)
    {
        DPRINT1("Width = %d, Height = %d BitsPixel = %d\n",
                Width, Height, BitsPixel);
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return 0;
    }

    WidthBytes = BITMAP_GetWidthBytes(Width, BitsPixel);

    Size.cx = Width;
    Size.cy = abs(Height);

    /* Make sure that cjBits will not overflow */
    if ((ULONGLONG)WidthBytes * Size.cy >= 0x100000000ULL)
    {
        DPRINT1("Width = %d, Height = %d BitsPixel = %d\n",
                Width, Height, BitsPixel);
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* Create the bitmap object. */
    hBitmap = GreCreateBitmap(Size, WidthBytes,
                              GrepBitmapFormat(BitsPixel, BI_RGB),
                              (Height < 0 ? BMF_TOPDOWN : 0) |
                              (NULL == pBits ? 0 : BMF_NOZEROINIT), NULL);
    if (!hBitmap)
    {
        DPRINT("IntGdiCreateBitmap: returned 0\n");
        return 0;
    }

    psurfBmp = SURFACE_LockSurface(hBitmap);
    if (psurfBmp == NULL)
    {
        GreDeleteObject(hBitmap);
        return NULL;
    }

//    psurfBmp->flFlags = BITMAPOBJ_IS_APIBITMAP;

    if (NULL != pBits)
    {
        GreSetBitmapBits(psurfBmp, psurfBmp->SurfObj.cjBits, pBits);
    }

    SURFACE_UnlockSurface(psurfBmp);

    DPRINT("IntGdiCreateBitmap : %dx%d, %d BPP colors, topdown %d, returning %08x\n",
           Size.cx, Size.cy, BitsPixel, (Height < 0 ? 1 : 0), hBitmap);

    return hBitmap;
}

BOOL APIENTRY
SURFACE_Cleanup(PVOID ObjectBody)
{
    PSURFACE pSurf = (PSURFACE)ObjectBody;

    /* Free bits memory */
    if (pSurf->SurfObj.pvBits && (pSurf->ulFlags & SRF_BITSALLOCD))
        EngFreeMem(pSurf->SurfObj.pvBits);

    /* Delete DIB palette if it exists */
    if (pSurf->hDIBPalette)
        GDIOBJ_FreeObjByHandle(pSurf->hDIBPalette, GDI_OBJECT_TYPE_PALETTE);

    /* Free bitslock storage */
    if (pSurf->pBitsLock)
        ExFreePoolWithTag(pSurf->pBitsLock, TAG_SURFOBJ);

    return TRUE;
}


LONG FASTCALL
GreGetBitmapBits(PSURFACE pSurf, ULONG ulBytes, PVOID pBits)
{
    LONG height, width, bytewidth;
    LPBYTE tbuf, startline;
    int h, w;
    COLORREF cr;
    PFN_DIB_GetPixel DibGetPixel;

    DPRINT("(bmp=%p, buffer=%p, count=0x%x)\n", pSurf, pBits, ulBytes);

    /* Check ulBytes */
    if (!ulBytes) return 0;

    bytewidth = pSurf->SurfObj.lDelta;
    if (bytewidth < 0) bytewidth = -bytewidth;

    height = ulBytes / bytewidth;
    width = pSurf->SurfObj.sizlBitmap.cx;

    /* Get getpixel routine address */
    DibGetPixel = DibFunctionsForBitmapFormat[pSurf->SurfObj.iBitmapFormat].DIB_GetPixel;

    /* copy bitmap to 16 bit padded image buffer with real bitsperpixel */
    startline = pBits;
    switch (BitsPerFormat(pSurf->SurfObj.iBitmapFormat))
    {
    case 1:
        for (h=0;h<height;h++)
        {
            tbuf = startline;
            *tbuf = 0;
            for (w=0;w<width;w++)
            {
                if ((w%8) == 0)
                    *tbuf = 0;
                cr = DibGetPixel(&pSurf->SurfObj, w, h);
                *tbuf |= cr<<(7-(w&7));
                if ((w&7) == 7) ++tbuf;
            }
            startline += bytewidth;
        }
        break;
    case 4:
        for (h=0;h<height;h++)
        {
            tbuf = startline;
            for (w=0;w<width;w++)
            {
                if (!(w & 1))
                    *tbuf = DibGetPixel(&pSurf->SurfObj, w, h) << 4;
                else
                    *tbuf++ |= DibGetPixel(&pSurf->SurfObj, w, h) & 0x0f;
            }
            startline += bytewidth;
        }
        break;
    case 8:
        for (h=0;h<height;h++)
        {
            tbuf = startline;
            for (w=0;w<width;w++)
                *tbuf++ = DibGetPixel(&pSurf->SurfObj, w, h);
            startline += bytewidth;
        }
        break;
    case 15:
    case 16:
        for (h=0;h<height;h++)
        {
            tbuf = startline;
            for (w=0;w<width;w++)
            {
            long pixel = DibGetPixel(&pSurf->SurfObj, w, h);

            *tbuf++ = pixel & 0xff;
            *tbuf++ = (pixel>>8) & 0xff;
            }
            startline += bytewidth;
        }
        break;
    case 24:
        for (h=0;h<height;h++)
        {
            tbuf = startline;
            for (w=0;w<width;w++)
            {
                long pixel = DibGetPixel(&pSurf->SurfObj, w, h);

                *tbuf++ = pixel & 0xff;
                *tbuf++ = (pixel>> 8) & 0xff;
                *tbuf++ = (pixel>>16) & 0xff;
            }
            startline += bytewidth;
        }
        break;

    case 32:
        for (h=0;h<height;h++)
        {
            tbuf = startline;
            for (w=0;w<width;w++)
            {
                long pixel = DibGetPixel(&pSurf->SurfObj, w, h);

                *tbuf++ = pixel & 0xff;
                *tbuf++ = (pixel>> 8) & 0xff;
                *tbuf++ = (pixel>>16) & 0xff;
                *tbuf++ = (pixel>>24) & 0xff;
            }
            startline += bytewidth;
        }
        break;
    default:
        DPRINT1("Unhandled bits:%d\n", BitsPerFormat(pSurf->SurfObj.iBitmapFormat));
    }
    return ulBytes;
}

LONG FASTCALL
GreSetBitmapBits(PSURFACE pSurf, ULONG ulBytes, PVOID pBits)
{
    LONG height, width, bytewidth;
    const BYTE *sbuf, *startline;
    int h, w;
    PFN_DIB_PutPixel DibPutPixel;

    /* Check ulBytes */
    if (!ulBytes) return 0;

    DPRINT("(bmp=%p, buffer=%p, count=0x%x)\n", pSurf, pBits, ulBytes);

    bytewidth = pSurf->SurfObj.lDelta;
    if (bytewidth < 0) bytewidth = -bytewidth;

    height = ulBytes / bytewidth;
    width = pSurf->SurfObj.sizlBitmap.cx;

    /* Get getpixel routine address */
    DibPutPixel = DibFunctionsForBitmapFormat[pSurf->SurfObj.iBitmapFormat].DIB_PutPixel;

    /* copy bitmap to 16 bit padded image buffer with real bitsperpixel */
    startline = pBits;
    switch (BitsPerFormat(pSurf->SurfObj.iBitmapFormat))
    {
    case 1:
        for (h=0;h<height;h++)
        {
            sbuf = startline;
            for (w=0;w<width;w++)
            {
                DibPutPixel(&pSurf->SurfObj, w, h, (sbuf[0]>>(7-(w&7))) & 1);
                if ((w&7) == 7)
                    sbuf++;
            }
            startline += bytewidth;
        }
        break;
    case 4:
        for (h=0;h<height;h++)
        {
            sbuf = startline;
            for (w=0;w<width;w++)
            {
                if (!(w & 1))
                    DibPutPixel(&pSurf->SurfObj, w, h, *sbuf >> 4);
                else
                    DibPutPixel(&pSurf->SurfObj, w, h, *sbuf++ & 0xf);
            }
            startline += bytewidth;
        }
        break;
    case 8:
        for (h=0;h<height;h++)
        {
            sbuf = startline;
            for (w=0;w<width;w++)
                DibPutPixel(&pSurf->SurfObj, w, h, *sbuf++);
            startline += bytewidth;
        }
        break;
    case 15:
    case 16:
        for (h=0;h<height;h++)
        {
            sbuf = startline;
            for (w=0;w<width;w++)
            {
                DibPutPixel(&pSurf->SurfObj, w, h, sbuf[1]*256+sbuf[0]);
                sbuf+=2;
            }
            startline += bytewidth;
        }
        break;
    case 24:
        for (h=0;h<height;h++)
        {
            sbuf = startline;
            for (w=0;w<width;w++)
            {
                DibPutPixel(&pSurf->SurfObj, w, h, (sbuf[2]<<16)+(sbuf[1]<<8)+sbuf[0]);
                sbuf += 3;
            }
            startline += bytewidth;
        }
        break;
    case 32:
        for (h=0;h<height;h++)
        {
            sbuf = startline;
            for (w=0;w<width;w++)
            {
                DibPutPixel(&pSurf->SurfObj, w, h, (sbuf[3]<<24)+(sbuf[2]<<16)+(sbuf[1]<<8)+sbuf[0]);
                sbuf += 4;
            }
            startline += bytewidth;
        }
        break;
    default:
      DPRINT1("Unhandled bits:%d\n", BitsPerFormat(pSurf->SurfObj.iBitmapFormat));
    }

    /* Return amount copied */
    return ulBytes;
}

COLORREF
NTAPI
GreGetPixel(
    PDC pDC,
    UINT x,
    UINT y)
{
    COLORREF crPixel = (COLORREF)CLR_INVALID;
    PSURFACE psurf;
    SURFOBJ *pso;

    /* Offset coordinate by DC origin */
    x += pDC->ptlDCOrig.x;
    y += pDC->ptlDCOrig.y;

    /* If point is outside the combined clipping region - return error */
    if (!RECTL_bPointInRect(&pDC->CombinedClip->rclBounds, x, y))
        return CLR_INVALID;

    /* Get DC's surface */
    psurf = pDC->dclevel.pSurface;

    if (!psurf) return CLR_INVALID;

    pso = &psurf->SurfObj;

    /* Actually get the pixel */
    if (pso->pvScan0)
    {
        ASSERT(pso->lDelta);

        // FIXME: Translate the color?
        crPixel = DibFunctionsForBitmapFormat[pso->iBitmapFormat].DIB_GetPixel(pso, x, y);
    }

    /* Return found pixel color */
    return crPixel;
}

BOOL
APIENTRY
GreCopyBits(SURFOBJ *psoDest,
            SURFOBJ *psoSource,
            CLIPOBJ *pco,
            XLATEOBJ *pxlo,
            RECTL *prclDest,
            POINTL *ptlSource)
{
    BOOL bResult;

    /* Start mouse safety */
    MouseSafetyOnDrawStart(psoSource, ptlSource->x, ptlSource->y,
                         (ptlSource->x + abs(prclDest->right - prclDest->left)),
                         (ptlSource->y + abs(prclDest->bottom - prclDest->top)));

    MouseSafetyOnDrawStart(psoDest, prclDest->left, prclDest->top, prclDest->right, prclDest->bottom);

    /* Copy bits using Eng function */
    bResult = EngCopyBits(psoDest, psoSource, pco, pxlo, prclDest, ptlSource);

    /* Finish mouse safety */
    MouseSafetyOnDrawEnd(psoDest);
    MouseSafetyOnDrawEnd(psoSource);

    return bResult;
}
