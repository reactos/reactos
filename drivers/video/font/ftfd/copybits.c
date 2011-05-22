/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         GDI font driver based on freetype
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include "ftfd.h"


static
VOID
FtfdCopyBits_S1D1(
    GLYPHBITS *pgb,
    FT_Bitmap *ftbitmap)
{
    ULONG ulRows, ulDstDelta, ulSrcDelta;
    PBYTE pjDstLine, pjSrcLine;

    pjDstLine = pgb->aj;
    ulDstDelta = (pgb->sizlBitmap.cx + 7) / 8;

    pjSrcLine = ftbitmap->buffer;
    ulSrcDelta = abs(ftbitmap->pitch);

    ulRows = pgb->sizlBitmap.cy;
    while (ulRows--)
    {
        /* Copy one line */
        memcpy(pjDstLine, pjSrcLine, ulDstDelta);

        /* Next ros */
        pjDstLine += ulDstDelta;
        pjSrcLine += ulSrcDelta;
    }
}

static
VOID
FtfdCopyBits_S8D1(
    GLYPHBITS *pgb,
    FT_Bitmap *ftbitmap)
{
    __debugbreak();
}


static
VOID
FtfdCopyBits_S1D4(
    GLYPHBITS *pgb,
    FT_Bitmap *ftbitmap)
{
    ULONG ulRows, ulSrcDelta;
    PBYTE pjDstLine, pjSrcLine;

//__debugbreak();

    pjDstLine = pgb->aj;

    pjSrcLine = ftbitmap->buffer;
    ulSrcDelta = abs(ftbitmap->pitch);

    ulRows = pgb->sizlBitmap.cy;
    while (ulRows--)
    {
        ULONG ulWidth = pgb->sizlBitmap.cx;
        BYTE j, *pjSrc;
        static USHORT ausExpand[] =
        {0x0000, 0x000f, 0x00f0, 0x00ff, 0x0f00, 0x0f0f, 0x0ff0, 0x0fff,
         0xf000, 0xf00f, 0xf0f0, 0xf0ff, 0xff00, 0xff0f, 0xfff0, 0xffff};

        pjSrc = pjSrcLine;

        /* Get 8 pixels */
        j = (*pjSrc++);

        while (ulWidth >= 8)
        {
            /* Set 8 pixels / 4 bytes */
            *pjDstLine++ = (BYTE)(ausExpand[j >> 4] >> 8);
            *pjDstLine++ = (BYTE)ausExpand[j >> 4];
            *pjDstLine++ = (BYTE)(ausExpand[j & 0xf] >> 8);
            *pjDstLine++ = (BYTE)ausExpand[j & 0xf];

            /* Next 8 pixels */
            j = (*pjSrc++);
            ulWidth -= 8;
        }

        /* Set remaining pixels (max 7) */
        if (ulWidth > 0) *pjDstLine++ = (BYTE)(ausExpand[j >> 4] >> 8);
        if (ulWidth > 2) *pjDstLine++ = (BYTE)ausExpand[j >> 4];
        if (ulWidth > 4) *pjDstLine++ = (BYTE)(ausExpand[j & 0xf] >> 8);
        if (ulWidth > 6) *pjDstLine++ = (BYTE)ausExpand[j & 0xf];

        /* Go to the next source line */
        pjSrcLine += ulSrcDelta;
    }
}

static
VOID
FtfdCopyBits_S8D4(
    GLYPHBITS *pgb,
    FT_Bitmap *ftbitmap)
{
    ULONG ulRows, ulDstDelta, ulSrcDelta;
    PBYTE pjDstLine, pjSrcLine;

    pjDstLine = pgb->aj;
    ulDstDelta = (pgb->sizlBitmap.cx*4 + 7) / 8;

    pjSrcLine = ftbitmap->buffer;
    ulSrcDelta = abs(ftbitmap->pitch);

    ulRows = pgb->sizlBitmap.cy;
    while (ulRows--)
    {
        ULONG ulWidth = ulDstDelta;
        BYTE j, *pjSrc;

        pjSrc = pjSrcLine;
        while (ulWidth--)
        {
            /* Get the 1st pixel */
            j = (*pjSrc++) & 0xf0;

            /* Get the 2nd pixel */
            if (ulWidth > 0 || !(pgb->sizlBitmap.cx & 1))
                j |= (*pjSrc++) >> 4;
            *pjDstLine++ = j;
        }

        /* Go to the next source line */
        pjSrcLine += ulSrcDelta;
    }
}

VOID
NTAPI
FtfdCopyBits(
    BYTE jBppDst,
    GLYPHBITS *pgb,
    FT_Bitmap *ftbitmap)
{
    /* handle empty bitmaps */
    if (ftbitmap->width == 0 || ftbitmap->rows == 0)
    {
        pgb->aj[0] = 0;
        return;
    }

    if (jBppDst == 1)
    {
        if (ftbitmap->pixel_mode == FT_PIXEL_MODE_MONO)
        {
            FtfdCopyBits_S1D1(pgb, ftbitmap);
        }
        else if (ftbitmap->pixel_mode == FT_PIXEL_MODE_GRAY &&
                 ftbitmap->num_grays == 256)
        {
            FtfdCopyBits_S8D1(pgb, ftbitmap);
        }
        else
        {
            WARN("Unsupported pixel format\n");
            __debugbreak();
        }

    }
    else if (jBppDst == 4)
    {
        if (ftbitmap->pixel_mode == FT_PIXEL_MODE_MONO)
        {
            FtfdCopyBits_S1D4(pgb, ftbitmap);
        }
        else if (ftbitmap->pixel_mode == FT_PIXEL_MODE_GRAY &&
                 ftbitmap->num_grays == 256)
        {
            FtfdCopyBits_S8D4(pgb, ftbitmap);
        }
        else
        {
            WARN("Unsupported pixel format\n");
            __debugbreak();
        }
    }
    else
    {
        WARN("Bit depth %ld not implemented\n", jBppDst);
        __debugbreak();
    }
}
