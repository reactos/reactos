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
    SURFACE_Unlock(pSurface);

    /* Return handle to it */
    return hSurface;
}

VOID FASTCALL
GreDeleteBitmap(HGDIOBJ hBitmap)
{
    /* Get ownership */
    GDIOBJ_SetOwnership(hBitmap, PsGetCurrentProcess());

    /* Free it */
    GDIOBJ_FreeObjByHandle(hBitmap, GDI_OBJECT_TYPE_BITMAP);
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
    x += pDC->rcVport.left + pDC->rcDcRect.left;
    y += pDC->rcVport.top + pDC->rcDcRect.top;

    /* If point is outside the combined clipping region - return error */
    if (!RECTL_bPointInRect(&pDC->CombinedClip->rclBounds, x, y))
        return CLR_INVALID;

    /* Get DC's surface */
    psurf = pDC->pBitmap;

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

VOID
NTAPI
GreSetPixel(
    PDC pDC,
    UINT x,
    UINT y,
    COLORREF crColor)
{
    PBRUSHGDI pOldBrush = pDC->pFillBrush;

    /* Create a solid brush with this color */
    pDC->pFillBrush = GreCreateSolidBrush(pDC->pBitmap->hDIBPalette, crColor);

    /* Put pixel */
    GrePatBlt(pDC, x, y, 1, 1, PATCOPY, pDC->pFillBrush);

    /* Free the created brush */
    GreFreeBrush(pDC->pFillBrush);

    /* Restore the old brush */
    pDC->pFillBrush = pOldBrush;
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
