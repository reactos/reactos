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

BOOLEAN DIB_XXBPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                            RECTL *DestRect, RECTL *SourceRect,
                            POINTL *MaskOrigin, BRUSHOBJ *Brush,
                            POINTL *BrushOrigin, CLIPOBJ *ClipRegion, 
                            XLATEOBJ *ColorTranslation, ROP4 ROP)
{
    LONG SrcSizeY;
    LONG SrcSizeX;
    LONG DesSizeY;
    LONG DesSizeX;
    LONG sx = 0;
    LONG sy = 0;
    LONG DesX;
    LONG DesY;

    LONG SrcZoomXHight;
    LONG SrcZoomXLow;
    LONG SrcZoomYHight;
    LONG SrcZoomYLow;

    LONG sy_dec = 0;
    LONG sy_max;

    LONG sx_dec = 0;
    LONG sx_max;
    ULONG color;
    ULONG Dest, Source = 0, Pattern = 0;
    ULONG xxBPPMask;

    PFN_DIB_GetPixel fnSource_GetPixel = NULL;
    PFN_DIB_GetPixel fnDest_GetPixel = NULL;
    PFN_DIB_PutPixel fnDest_PutPixel = NULL;

    RECT_ENUM RectEnum;
    BOOL EnumMore;
    unsigned i;
    RECTL OutputRect;

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

    /* Calc the Zoom height of Source */
    SrcSizeY = SourceRect->bottom - SourceRect->top;

    /* Calc the Zoom Width of Source */
    SrcSizeX = SourceRect->right - SourceRect->left;

    /* Calc the Zoom height of Destinations */
    DesSizeY = DestRect->bottom - DestRect->top;

    /* Calc the Zoom width of Destinations */
    DesSizeX = DestRect->right - DestRect->left;

    /* Calc the zoom factor of source height */
    SrcZoomYHight = SrcSizeY / DesSizeY;
    SrcZoomYLow = SrcSizeY - (SrcZoomYHight * DesSizeY);

    /* Calc the zoom factor of source width */
    SrcZoomXHight = SrcSizeX / DesSizeX;
    SrcZoomXLow = SrcSizeX - (SrcZoomXHight * DesSizeX);

    sx_max = DesSizeX;
    sy_max = DesSizeY;

    /* FIXME :  MaskOrigin, BrushOrigin? */

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
        DPRINT1("StretchBlt does not support pattern ROPs yet\n");
        return TRUE;
    }
    
    CLIPOBJ_cEnumStart(ClipRegion, FALSE, CT_RECTANGLES, CD_ANY, 0);
    do
    {
        EnumMore = CLIPOBJ_bEnum(ClipRegion,(ULONG) sizeof(RectEnum), (PVOID) &RectEnum);

        for (i = 0; i < RectEnum.c; i++)
        {
            OutputRect.left = RectEnum.arcl[i].left;
            OutputRect.right = RectEnum.arcl[i].right;
            OutputRect.top = RectEnum.arcl[i].top;
            OutputRect.bottom = RectEnum.arcl[i].bottom;
                
            sy = SourceRect->top;
            sy_dec = 0;
            for (DesY=DestRect->top; DesY<DestRect->bottom; DesY++)
            {
                sx = SourceRect->left;
                sx_dec = 0;

                for (DesX=DestRect->left; DesX<DestRect->right; DesX++)
                {
                    /* Check if inside clip region */
                    if (DesX >= OutputRect.left &&
                        DesX < OutputRect.right &&
                        DesY >= OutputRect.top &&
                        DesY < OutputRect.bottom)
                    {
                        if (UsesSource) 
                        {
                            Source = XLATEOBJ_iXlate(ColorTranslation, fnSource_GetPixel(SourceSurf, sx, sy));
                        }

                        if (UsesPattern)
                        {
                            /* TBD as soon as BRUSHOBJ is available */
                        }

                        Dest = fnDest_GetPixel(DestSurf, DesX, DesY);
                        color = DIB_DoRop(ROP, Dest, Source, Pattern) & xxBPPMask;

                        fnDest_PutPixel(DestSurf, DesX, DesY, color);

                    }
                    sx += SrcZoomXHight;
                    sx_dec += SrcZoomXLow;
                    if (sx_dec >= sx_max)
                    {
                        sx++;
                        sx_dec -= sx_max;
                    }
                }

                sy += SrcZoomYHight;
                sy_dec += SrcZoomYLow;
                if (sy_dec >= sy_max)
                {
                    sy++;
                    sy_dec -= sy_max;
                }
            }
        }
    }
    while (EnumMore);

    return TRUE;
}

/* EOF */
