/*
 * PROJECT:         ReactOS Win32k subsystem
 * LICENSE:         See COPYING in the top level directory
 * FILE:            win32ss/gdi/dib/stretchblt.c
 * PURPOSE:         StretchBlt implementation suitable for all bit depths
 * PROGRAMMERS:     Magnus Olsen
 *                  Evgeniy Boltik
 *                  Gregor Schneider
 *                  Doug Lyons
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

BOOLEAN DIB_XXBPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf, SURFOBJ *MaskSurf,
                            SURFOBJ *PatternSurface,
                            RECTL *DestRect, RECTL *SourceRect,
                            POINTL *MaskOrigin, BRUSHOBJ *Brush,
                            POINTL *BrushOrigin, XLATEOBJ *ColorTranslation,
                            ROP4 ROP)
{
  LONG sx = 0;
  LONG sy = 0;
  LONG DesX;
  LONG DesY;

  LONG DstHeight;
  LONG DstWidth;
  LONG SrcHeight;
  LONG SrcWidth;
  LONG MaskCy;
  LONG SourceCy;

  ULONG Color;
  ULONG Dest, Source = 0, Pattern = 0;
  ULONG xxBPPMask;
  BOOLEAN CanDraw;

  PFN_DIB_GetPixel fnSource_GetPixel = NULL;
  PFN_DIB_GetPixel fnDest_GetPixel = NULL;
  PFN_DIB_PutPixel fnDest_PutPixel = NULL;
  PFN_DIB_GetPixel fnPattern_GetPixel = NULL;
  PFN_DIB_GetPixel fnMask_GetPixel = NULL;

  LONG PatternX = 0, PatternY = 0;

  BOOL UsesSource = ROP4_USES_SOURCE(ROP);
  BOOL UsesPattern = ROP4_USES_PATTERN(ROP);
  LONG flip;
  BOOL flipx, flipy;
  RECTL OutputRect;

  ASSERT(IS_VALID_ROP4(ROP));

  fnDest_GetPixel = DibFunctionsForBitmapFormat[DestSurf->iBitmapFormat].DIB_GetPixel;
  fnDest_PutPixel = DibFunctionsForBitmapFormat[DestSurf->iBitmapFormat].DIB_PutPixel;

  DPRINT("Dest BPP: %u, dstRect: (%d,%d)-(%d,%d)\n",
    BitsPerFormat(DestSurf->iBitmapFormat), DestRect->left, DestRect->top, DestRect->right, DestRect->bottom);

  DstHeight = DestRect->bottom - DestRect->top;
  DstWidth = DestRect->right - DestRect->left;
  SrcHeight = SourceRect->bottom - SourceRect->top;
  SrcWidth = SourceRect->right - SourceRect->left;

  /* Here we do the flip tests and set our conditions */
  if (((SrcWidth < 0) && (DstWidth < 0)) || ((SrcWidth >= 0) && (DstWidth >= 0)))
  {
    flipy = FALSE;
  }
  else
  {
    flipy = TRUE;
  }

  if (((SrcHeight < 0) && (DstHeight < 0)) || ((SrcHeight >= 0) && (DstHeight >= 0)))
  {
    flipx = FALSE;
  }
  else
  {
    flipx = TRUE;
  }

  DPRINT("flip about x-axis is '%d' and flip and y-axis is '%d'.\n", flipx, flipy);

  if (!flipx && !flipy)
  {
    flip = 0;
  }
  else if (!flipx && flipy)
  {
    flip = 1;
  }
  else if (flipx && !flipy)
  {
    flip = 2;
  }
  else
  {
    flip = 3;
  }

  DPRINT("flip is '%d'.\n", flip);

  /* Make Well Ordered to start */
  OutputRect = *DestRect;
  RECTL_vMakeWellOrdered(&OutputRect);
  *DestRect = OutputRect;

  if (UsesSource)
  {
    SourceCy = SourceSurf->sizlBitmap.cy;
    fnSource_GetPixel = DibFunctionsForBitmapFormat[SourceSurf->iBitmapFormat].DIB_GetPixel;
    DPRINT("Source BPP: %u, srcRect: (%d,%d)-(%d,%d)\n",
      BitsPerFormat(SourceSurf->iBitmapFormat), SourceRect->left, SourceRect->top, SourceRect->right, SourceRect->bottom);
  }

  if (MaskSurf)
  {
    DPRINT("MaskSurf is not NULL.\n");
    fnMask_GetPixel = DibFunctionsForBitmapFormat[MaskSurf->iBitmapFormat].DIB_GetPixel;
    MaskCy = MaskSurf->sizlBitmap.cy;
  }

  DstHeight = DestRect->bottom - DestRect->top;
  DstWidth = DestRect->right - DestRect->left;
  SrcHeight = SourceRect->bottom - SourceRect->top;
  SrcWidth = SourceRect->right - SourceRect->left;

  /* FIXME: MaskOrigin? */

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
  DPRINT("xxBPPMask is 0x%x.\n", xxBPPMask);

  if (UsesPattern)
  {
    DPRINT("UsesPattern is not NULL.\n");
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

  if (PatternSurface)
  {
    DPRINT("PatternSurface is not NULL.\n");
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
    {
      if ((flip == 2) || (flip == 3))
      {
        sy = SourceRect->bottom-(DesY - DestRect->top) * SrcHeight / DstHeight;  // flips about the x-axis
      }
      else
      {
        sy = SourceRect->top+(DesY - DestRect->top) * SrcHeight / DstHeight;
      }
    }

    for (DesX = DestRect->left; DesX < DestRect->right; DesX++)
    {
      CanDraw = TRUE;

      if (fnMask_GetPixel)
      {
        if ((flip == 1) || (flip == 3))
        {
          sx = SourceRect->right-(DesX - DestRect->left) * SrcWidth / DstWidth;  // flips about the y-axis
        }
        else
        {
          sx = SourceRect->left+(DesX - DestRect->left) * SrcWidth / DstWidth;
        }
        if (sx < 0 || sy < 0 ||
          MaskSurf->sizlBitmap.cx < sx || MaskCy < sy ||
          fnMask_GetPixel(MaskSurf, sx, sy) != 0)
        {
          CanDraw = FALSE;
        }
      }

      if (UsesSource && CanDraw)
      {
        if ((flip == 1) || (flip == 3))
        {
          sx = SourceRect->right-(DesX - DestRect->left) * SrcWidth / DstWidth;  // flips about the y-axis
        }
        else
        {
          sx = SourceRect->left+(DesX - DestRect->left) * SrcWidth / DstWidth;
        }
        if (sx >= 0 && sy >= 0 &&
          SourceSurf->sizlBitmap.cx > sx && SourceCy > sy)
        {
          Source = XLATEOBJ_iXlate(ColorTranslation, fnSource_GetPixel(SourceSurf, sx, sy));
        }
        else
        {
          Source = 0;
          CanDraw = ((ROP & 0xFF) != R3_OPINDEX_SRCCOPY);
        }
      }

      if (CanDraw)
      {
        if (UsesPattern && PatternSurface)
        {
          Pattern = fnPattern_GetPixel(PatternSurface, PatternX, PatternY);
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
