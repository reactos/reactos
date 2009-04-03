/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 2009 ReactOS Team
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

#include <w32k.h>

#define NDEBUG
#include <debug.h>

BOOLEAN DIB_XXBPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf, SURFOBJ *MaskSurf,
                            SURFOBJ *PatternSurface,
                            RECTL *DestRect, RECTL *SourceRect,
                            POINTL *MaskOrigin, BRUSHOBJ *Brush,
                            POINTL *BrushOrigin, XLATEOBJ *ColorTranslation,
                            XLATEOBJ *XlatePatternToDest, ROP4 ROP)
{
    LONG sx = 0;
    LONG sy = 0;
    LONG DesX;
    LONG DesY;

    LONG DstHeight;
    LONG DstWidth;
    LONG SrcHeight;
    LONG SrcWidth;

    ULONG Color;
    ULONG Dest, Source = 0, Pattern = 0;
    ULONG xxBPPMask;
    BOOLEAN CanDraw;

    PFN_DIB_GetPixel fnSource_GetPixel = NULL;
    PFN_DIB_GetPixel fnDest_GetPixel = NULL;
    PFN_DIB_PutPixel fnDest_PutPixel = NULL;
    PFN_DIB_GetPixel fnPattern_GetPixel = NULL;
    PFN_DIB_GetPixel fnMask_GetPixel = NULL;

    ULONG PatternX = 0, PatternY = 0;

    BOOL UsesSource = ROP4_USES_SOURCE(ROP);
    BOOL UsesPattern = ROP4_USES_PATTERN(ROP);

    fnDest_GetPixel = DibFunctionsForBitmapFormat[DestSurf->iBitmapFormat].DIB_GetPixel;
    fnDest_PutPixel = DibFunctionsForBitmapFormat[DestSurf->iBitmapFormat].DIB_PutPixel;

    DPRINT("Dest BPP: %u, dstRect: (%d,%d)-(%d,%d)\n",
        BitsPerFormat(DestSurf->iBitmapFormat), DestRect->left, DestRect->top, DestRect->right, DestRect->bottom);

    if (UsesSource)
    {
        fnSource_GetPixel = DibFunctionsForBitmapFormat[SourceSurf->iBitmapFormat].DIB_GetPixel;
        DPRINT("Source BPP: %u, srcRect: (%d,%d)-(%d,%d)\n",
            BitsPerFormat(SourceSurf->iBitmapFormat), SourceRect->left, SourceRect->top, SourceRect->right, SourceRect->bottom);
    }

    if (MaskSurf)
    {
        fnMask_GetPixel = DibFunctionsForBitmapFormat[MaskSurf->iBitmapFormat].DIB_GetPixel;
    }

    DstHeight = DestRect->bottom - DestRect->top;
    DstWidth = DestRect->right - DestRect->left;
    SrcHeight = SourceRect->bottom - SourceRect->top;
    SrcWidth = SourceRect->right - SourceRect->left;

    /* FIXME :  MaskOrigin? */

    switch(DestSurf->iBitmapFormat)
    {
        case BMF_1BPP: xxBPPMask = 0x1; break;
        case BMF_4BPP: xxBPPMask = 0xF; break;
        case BMF_8BPP: xxBPPMask = 0xFF; break;
        case BMF_16BPP: xxBPPMask = 0xFFFF; break;
        case BMF_24BPP: xxBPPMask = 0xFFFFFF; break;
        default:
            xxBPPMask = 0xFFFFFFFF;
    }

    if (UsesPattern)
    {
        if (PatternSurface)
        {
            PatternY = (DestRect->top - BrushOrigin->y) % PatternSurface->sizlBitmap.cy;
            if (PatternY < 0)
            {
                PatternY += PatternSurface->sizlBitmap.cy;
            }
            fnPattern_GetPixel = DibFunctionsForBitmapFormat[PatternSurface->iBitmapFormat].DIB_GetPixel;
        }
        else
        {
            if (Brush)
                Pattern = Brush->iSolidColor;
        }
    }


    for (DesY = DestRect->top; DesY < DestRect->bottom; DesY++)
    {
        if (PatternSurface)
        {
            PatternX = (DestRect->left - BrushOrigin->x) % PatternSurface->sizlBitmap.cx;
            if (PatternX < 0)
            {
                PatternX += PatternSurface->sizlBitmap.cx;
            }
        }
        if (UsesSource)
          sy = SourceRect->top+(DesY - DestRect->top) * SrcHeight / DstHeight;

        for (DesX = DestRect->left; DesX < DestRect->right; DesX++)
        {
            CanDraw = TRUE;

            if (fnMask_GetPixel)
            {
                sx = SourceRect->left+(DesX - DestRect->left) * SrcWidth / DstWidth;
                if (sx < 0 || sy < 0 ||
                    MaskSurf->sizlBitmap.cx < sx || MaskSurf->sizlBitmap.cy < sy || 
                    fnMask_GetPixel(MaskSurf, sx, sy) != 0)
                {
                    CanDraw = FALSE;
                }
            }

            if (UsesSource && CanDraw)
            {
                sx = SourceRect->left+(DesX - DestRect->left) * SrcWidth / DstWidth;
                if (sx >= 0 && sy >= 0 &&
                    SourceSurf->sizlBitmap.cx > sx && SourceSurf->sizlBitmap.cy > sy)
                {
                     Source = XLATEOBJ_iXlate(ColorTranslation, fnSource_GetPixel(SourceSurf, sx, sy));
                }
                else 
                {
                     Source = 0;
                     CanDraw = (ROP3_TO_ROP4(SRCCOPY) != ROP);
                }
            }

            if (CanDraw)
            {
                if (PatternSurface)
                {
                    Pattern = XLATEOBJ_iXlate(XlatePatternToDest, fnPattern_GetPixel(PatternSurface, PatternX, PatternY));
                    PatternX++;
                    PatternX %= PatternSurface->sizlBitmap.cx;
                }

                Dest = fnDest_GetPixel(DestSurf, DesX, DesY);
                Color = DIB_DoRop(ROP, Dest, Source, Pattern) & xxBPPMask;

                fnDest_PutPixel(DestSurf, DesX, DesY, Color);
            }
        }

        if (PatternSurface)
        {
            PatternY++;
            PatternY %= PatternSurface->sizlBitmap.cy;
        }
    }

    return TRUE;
}

/* EOF */
