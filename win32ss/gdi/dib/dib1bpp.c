/*
 * PROJECT:         Win32 subsystem
 * LICENSE:         See COPYING in the top level directory
 * FILE:            win32ss/gdi/dib/dib1bpp.c
 * PURPOSE:         Device Independant Bitmap functions, 1bpp
 * PROGRAMMERS:     Jason Filby
 *                  Doug Lyons
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

#define DEC_OR_INC(var, decTrue, amount) \
    ((var) = (decTrue) ? ((var) - (amount)) : ((var) + (amount)))

VOID
DIB_1BPP_PutPixel(SURFOBJ *SurfObj, LONG x, LONG y, ULONG c)
{
  PBYTE addr = (PBYTE)SurfObj->pvScan0 + y * SurfObj->lDelta + (x >> 3);

  if (0 == (c & 0x01))
    *addr &= ~MASK1BPP(x);
  else
    *addr |= MASK1BPP(x);
}

ULONG
DIB_1BPP_GetPixel(SURFOBJ *SurfObj, LONG x, LONG y)
{
  PBYTE addr = (PBYTE)SurfObj->pvScan0 + y * SurfObj->lDelta + (x >> 3);

  return (*addr & MASK1BPP(x) ? 1 : 0);
}

VOID
DIB_1BPP_HLine(SURFOBJ *SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  while(x1 < x2)
  {
    DIB_1BPP_PutPixel(SurfObj, x1, y, c);
    x1++;
  }
}

VOID
DIB_1BPP_VLine(SURFOBJ *SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  while(y1 < y2)
  {
    DIB_1BPP_PutPixel(SurfObj, x, y1, c);
    y1++;
  }
}

static
BOOLEAN
DIB_1BPP_BitBltSrcCopy_From1BPP (
                                 SURFOBJ* DestSurf,
                                 SURFOBJ* SourceSurf,
                                 XLATEOBJ* pxlo,
                                 PRECTL DestRect,
                                 POINTL *SourcePoint,
                                 BOOLEAN bTopToBottom,
                                 BOOLEAN bLeftToRight )
{
    LONG Height = RECTL_lGetHeight(DestRect);
    LONG Width = RECTL_lGetWidth(DestRect);
    BOOLEAN XorBit = !!XLATEOBJ_iXlate(pxlo, 0);
    BOOLEAN Overlap = FALSE;
    BYTE *DstStart, *DstEnd, *SrcStart, *SrcEnd;

    /* Make sure this is as expected */
    ASSERT(DestRect->left >= 0);
    ASSERT(DestRect->top >= 0);
    ASSERT(Height > 0);
    ASSERT(Width > 0);

    /*
     * Check if we need to allocate a buffer for our operation.
     */
    DstStart = (BYTE*)DestSurf->pvScan0 + DestRect->top * DestSurf->lDelta + DestRect->left / 8;
    DstEnd = (BYTE*)DestSurf->pvScan0 + (DestRect->bottom - 1) * DestSurf->lDelta + (DestRect->right) / 8;
    SrcStart = (BYTE*)SourceSurf->pvScan0 + SourcePoint->y * SourceSurf->lDelta + SourcePoint->x / 8;
    SrcEnd = (BYTE*)SourceSurf->pvScan0 + (SourcePoint->y + Height - 1) * SourceSurf->lDelta + (SourcePoint->x + Width) / 8;

    /* Beware of bitmaps with negative pitch! */
    if (DstStart > DstEnd)
    {
        BYTE* tmp = DstStart;
        DstStart = DstEnd;
        DstEnd = tmp;
    }

    if (SrcStart > SrcEnd)
    {
        BYTE* tmp = SrcStart;
        SrcStart = SrcEnd;
        SrcEnd = tmp;
    }

    /* We allocate a new buffer when the two buffers overlap */
    Overlap = ((SrcStart >= DstStart) && (SrcStart < DstEnd)) || ((SrcEnd >= DstStart) && (SrcEnd < DstEnd));

    if (!Overlap)
    {
        LONG y;
        for (y = 0; y < Height; y++)
        {
            LONG ySrc = bTopToBottom ? SourcePoint->y + Height - y - 1 : SourcePoint->y + y;
            LONG x;

            for(x = 0; x < Width; x++)
            {
                LONG xSrc = bLeftToRight ? SourcePoint->x + Width - x - 1 : SourcePoint->x + x;
                ULONG Pixel = DIB_1BPP_GetPixel(SourceSurf, xSrc, ySrc);
                if (XorBit)
                    Pixel = !Pixel;
                DIB_1BPP_PutPixel(DestSurf, DestRect->left + x, DestRect->top + y, Pixel);
            }
        }
    }
    else
    {
        LONG y;
        BYTE* PixBuf = ExAllocatePoolZero(PagedPool, Height * (ALIGN_UP_BY(Width, 8) / 8), TAG_DIB);
        if (!PixBuf)
            return FALSE;
        for (y = 0; y < Height; y++)
        {
            BYTE* Row = PixBuf + y * ALIGN_UP_BY(Width, 8) / 8;
            LONG ySrc = bTopToBottom ? SourcePoint->y + Height - y - 1 : SourcePoint->y + y;
            LONG x;

            for (x = 0; x < Width; x++)
            {
                LONG xSrc = bLeftToRight ? SourcePoint->x + Width - x - 1 : SourcePoint->x + x;
                if ((!DIB_1BPP_GetPixel(SourceSurf, xSrc, ySrc)) == XorBit)
                    Row[x / 8] |= 1 << (x & 7);
            }
        }

        for (y = 0; y < Height; y++)
        {
            BYTE* Row = PixBuf + y * ALIGN_UP_BY(Width, 8) / 8;
            LONG x;
            for (x = 0; x < Width; x++)
                DIB_1BPP_PutPixel(DestSurf, DestRect->left + x, DestRect->top + y, !!(Row[x/8] & (1 << (x&7))));
        }

        ExFreePoolWithTag(PixBuf, TAG_DIB);
    }

    return TRUE;
}

BOOLEAN
DIB_1BPP_BitBltSrcCopy(PBLTINFO BltInfo)
{
  ULONG Color;
  LONG i, j, sx, sy;
  BOOLEAN bTopToBottom, bLeftToRight;

  // This sets sy to the top line
  sy = BltInfo->SourcePoint.y;

  DPRINT("DIB_1BPP_BitBltSrcCopy: SrcSurf cx/cy (%d/%d), DestSuft cx/cy (%d/%d) dstRect: (%d,%d)-(%d,%d)\n",
         BltInfo->SourceSurface->sizlBitmap.cx, BltInfo->SourceSurface->sizlBitmap.cy,
         BltInfo->DestSurface->sizlBitmap.cx, BltInfo->DestSurface->sizlBitmap.cy,
         BltInfo->DestRect.left, BltInfo->DestRect.top, BltInfo->DestRect.right, BltInfo->DestRect.bottom);

  /* Get back left to right flip here */
  bLeftToRight = (BltInfo->DestRect.left > BltInfo->DestRect.right);

  /* Check for top to bottom flip needed. */
  bTopToBottom = BltInfo->DestRect.top > BltInfo->DestRect.bottom;

  // Make WellOrdered with top < bottom and left < right
  RECTL_vMakeWellOrdered(&BltInfo->DestRect);

  DPRINT("BPP is '%d' & BltInfo->SourcePoint.x is '%d' & BltInfo->SourcePoint.y is '%d'.\n",
         BltInfo->SourceSurface->iBitmapFormat, BltInfo->SourcePoint.x, BltInfo->SourcePoint.y);

  switch ( BltInfo->SourceSurface->iBitmapFormat )
  {
  case BMF_1BPP:
    DPRINT("1BPP Case Selected with DestRect Width of '%d'.\n",
           BltInfo->DestRect.right - BltInfo->DestRect.left);

    return DIB_1BPP_BitBltSrcCopy_From1BPP ( BltInfo->DestSurface, BltInfo->SourceSurface,
      BltInfo->XlateSourceToDest, &BltInfo->DestRect, &BltInfo->SourcePoint,
      bTopToBottom, bLeftToRight );

  case BMF_4BPP:
    DPRINT("4BPP Case Selected with DestRect Width of '%d'.\n",
           BltInfo->DestRect.right - BltInfo->DestRect.left);

    if (bTopToBottom)
    {
      // This sets sy to the bottom line
      sy += (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) * BltInfo->SourceSurface->lDelta;
    }

    for (j=BltInfo->DestRect.top; j<BltInfo->DestRect.bottom; j++)
    {
      sx = BltInfo->SourcePoint.x;

      if (bLeftToRight)
      {
       // This sets the sx to the rightmost pixel
       sx += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
      }

      for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
      {
        Color = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, DIB_4BPP_GetPixel(BltInfo->SourceSurface, sx, sy));
        DIB_1BPP_PutPixel(BltInfo->DestSurface, i, j, Color);

        DEC_OR_INC(sx, bLeftToRight, 1);
      }
      DEC_OR_INC(sy, bTopToBottom, 1);
    }
    break;

  case BMF_8BPP:
    DPRINT("8BPP-dstRect: (%d,%d)-(%d,%d) and Width of '%d'.\n",
           BltInfo->DestRect.left, BltInfo->DestRect.top,
           BltInfo->DestRect.right, BltInfo->DestRect.bottom,
           BltInfo->DestRect.right - BltInfo->DestRect.left);

    if (bTopToBottom)
    {
      // This sets sy to the bottom line
      sy += (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) * BltInfo->SourceSurface->lDelta;
    }

    for (j=BltInfo->DestRect.top; j<BltInfo->DestRect.bottom; j++)
    {
      sx = BltInfo->SourcePoint.x;

      if (bLeftToRight)
      {
        // This sets sx to the rightmost pixel
        sx += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
      }

      for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
      {
        Color = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, DIB_8BPP_GetPixel(BltInfo->SourceSurface, sx, sy));
        DIB_1BPP_PutPixel(BltInfo->DestSurface, i, j, Color);

        DEC_OR_INC(sx, bLeftToRight, 1);
      }
      DEC_OR_INC(sy, bTopToBottom, 1);
    }
    break;

  case BMF_16BPP:
    DPRINT("16BPP-dstRect: (%d,%d)-(%d,%d) and Width of '%d'.\n",
           BltInfo->DestRect.left, BltInfo->DestRect.top,
           BltInfo->DestRect.right, BltInfo->DestRect.bottom,
           BltInfo->DestRect.right - BltInfo->DestRect.left);

    if (bTopToBottom)
    {
      // This sets sy to the bottom line
      sy += (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) * BltInfo->SourceSurface->lDelta;;
    }

    for (j=BltInfo->DestRect.top; j<BltInfo->DestRect.bottom; j++)
    {
      sx = BltInfo->SourcePoint.x;

      if (bLeftToRight)
      {
        // This sets the sx to the rightmost pixel
        sx += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
      }

      for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
      {
        Color = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, DIB_16BPP_GetPixel(BltInfo->SourceSurface, sx, sy));
        DIB_1BPP_PutPixel(BltInfo->DestSurface, i, j, Color);
        DEC_OR_INC(sx, bLeftToRight, 1);
      }
      DEC_OR_INC(sy, bTopToBottom, 1);
    }
    break;

  case BMF_24BPP:

    DPRINT("24BPP-dstRect: (%d,%d)-(%d,%d) and Width of '%d'.\n",
           BltInfo->DestRect.left, BltInfo->DestRect.top,
           BltInfo->DestRect.right, BltInfo->DestRect.bottom,
           BltInfo->DestRect.right - BltInfo->DestRect.left);

      if (bTopToBottom)
      {
        // This sets sy to the bottom line
        sy += (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) * BltInfo->SourceSurface->lDelta;
      }

    for (j=BltInfo->DestRect.top; j<BltInfo->DestRect.bottom; j++)
    {
      sx = BltInfo->SourcePoint.x;

      if (bLeftToRight)
      {
        // This sets the sx to the rightmost pixel
        sx += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
      }

      for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
      {
        Color = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, DIB_24BPP_GetPixel(BltInfo->SourceSurface, sx, sy));
        DIB_1BPP_PutPixel(BltInfo->DestSurface, i, j, Color);
        DEC_OR_INC(sx, bLeftToRight, 1);
      }
      DEC_OR_INC(sy, bTopToBottom, 1);
    }
    break;

  case BMF_32BPP:

    DPRINT("32BPP-dstRect: (%d,%d)-(%d,%d) and Width of '%d'.\n",
           BltInfo->DestRect.left, BltInfo->DestRect.top,
           BltInfo->DestRect.right, BltInfo->DestRect.bottom,
           BltInfo->DestRect.right - BltInfo->DestRect.left);

    if (bTopToBottom)
    {
      // This sets sy to the bottom line
      sy += BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1;
    }

    for (j=BltInfo->DestRect.top; j<BltInfo->DestRect.bottom; j++)
    {
      sx = BltInfo->SourcePoint.x;

      if (bLeftToRight)
      {
        // This sets the sx to the rightmost pixel
        sx += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
      }

      for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
      {
        Color = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, DIB_32BPP_GetPixel(BltInfo->SourceSurface, sx, sy));
        DIB_1BPP_PutPixel(BltInfo->DestSurface, i, j, Color);
        DEC_OR_INC(sx, bLeftToRight, 1);
      }
      DEC_OR_INC(sy, bTopToBottom, 1);
    }
    break;

  default:
    DbgPrint("DIB_1BPP_BitBlt: Unhandled Source BPP: %u\n", BitsPerFormat(BltInfo->SourceSurface->iBitmapFormat));
    return FALSE;
  }

  return TRUE;
}

#ifndef _USE_DIBLIB_
BOOLEAN
DIB_1BPP_BitBlt(PBLTINFO BltInfo)
{
  LONG DestX, DestY;
  LONG SourceX, SourceY;
  LONG PatternY = 0;
  ULONG Dest, Source = 0, Pattern = 0;
  ULONG Index;
  BOOLEAN UsesSource;
  BOOLEAN UsesPattern;
  PULONG DestBits;
  LONG RoundedRight;

  UsesSource = ROP4_USES_SOURCE(BltInfo->Rop4);
  UsesPattern = ROP4_USES_PATTERN(BltInfo->Rop4);

  RoundedRight = BltInfo->DestRect.right -
    ((BltInfo->DestRect.right - BltInfo->DestRect.left) & 31);
  SourceY = BltInfo->SourcePoint.y;

  if (UsesPattern)
  {
    if (BltInfo->PatternSurface)
    {
      PatternY = (BltInfo->DestRect.top + BltInfo->BrushOrigin.y) %
        BltInfo->PatternSurface->sizlBitmap.cy;
    }
    else
    {
      /* FIXME: Shouldn't it be expanded? */
      if (BltInfo->Brush)
        Pattern = BltInfo->Brush->iSolidColor;
    }
  }

  for (DestY = BltInfo->DestRect.top; DestY < BltInfo->DestRect.bottom; DestY++)
  {
    DestX = BltInfo->DestRect.left;
    SourceX = BltInfo->SourcePoint.x;
    DestBits = (PULONG)(
      (PBYTE)BltInfo->DestSurface->pvScan0 +
      (BltInfo->DestRect.left >> 3) +
      DestY * BltInfo->DestSurface->lDelta);

    if (DestX & 31)
    {
#if 0
      /* FIXME: This case is completely untested!!! */

      Dest = *((PBYTE)DestBits);
      NoBits = 31 - (DestX & 31);

      if (UsesSource)
      {
        Source = 0;
        /* FIXME: This is incorrect! */
        for (Index = 31 - NoBits; Index >= 0; Index++)
          Source |= (DIB_GetSource(SourceSurf, SourceX + Index, SourceY, ColorTranslation) << (31 - Index));
      }

      if (BltInfo->PatternSurface)
      {
        Pattern = 0;
        for (k = 31 - NoBits; k >= 0; k++)
          Pattern |= (DIB_GetSourceIndex(PatternObj, (X + BrushOrigin.x + k) % PatternWidth, PatternY) << (31 - k));
      }

      Dest = DIB_DoRop(BltInfo->Rop4, Dest, Source, Pattern);
      Dest &= ~((1 << (31 - NoBits)) - 1);
      Dest |= *((PBYTE)DestBits) & ((1 << (31 - NoBits)) - 1);

      *DestBits = Dest;

      DestX += NoBits;
      SourceX += NoBits;
#endif
    }

    for (; DestX < RoundedRight; DestX += 32, DestBits++, SourceX += 32)
    {
      Dest = *DestBits;

      if (UsesSource)
      {
        Source = 0;
        for (Index = 0; Index < 8; Index++)
        {
          Source |= DIB_GetSource(BltInfo->SourceSurface, SourceX + Index, SourceY, BltInfo->XlateSourceToDest) << (7 - Index);
          Source |= DIB_GetSource(BltInfo->SourceSurface, SourceX + Index + 8, SourceY, BltInfo->XlateSourceToDest) << (8 + (7 - Index));
          Source |= DIB_GetSource(BltInfo->SourceSurface, SourceX + Index + 16, SourceY, BltInfo->XlateSourceToDest) << (16 + (7 - Index));
          Source |= DIB_GetSource(BltInfo->SourceSurface, SourceX + Index + 24, SourceY, BltInfo->XlateSourceToDest) << (24 + (7 - Index));
        }
      }

      if (BltInfo->PatternSurface)
      {
        Pattern = 0;
        for (Index = 0; Index < 8; Index++)
        {
          Pattern |= DIB_GetSourceIndex(BltInfo->PatternSurface, (DestX + BltInfo->BrushOrigin.x + Index) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY) << (7 - Index);
          Pattern |= DIB_GetSourceIndex(BltInfo->PatternSurface, (DestX + BltInfo->BrushOrigin.x + Index + 8) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY) << (8 + (7 - Index));
          Pattern |= DIB_GetSourceIndex(BltInfo->PatternSurface, (DestX + BltInfo->BrushOrigin.x + Index + 16) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY) << (16 + (7 - Index));
          Pattern |= DIB_GetSourceIndex(BltInfo->PatternSurface, (DestX + BltInfo->BrushOrigin.x + Index + 24) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY) << (24 + (7 - Index));
        }
      }

      *DestBits = DIB_DoRop(BltInfo->Rop4, Dest, Source, Pattern);
    }

    if (DestX < BltInfo->DestRect.right)
    {
      for (; DestX < BltInfo->DestRect.right; DestX++, SourceX++)
      {
        Dest = DIB_1BPP_GetPixel(BltInfo->DestSurface, DestX, DestY);

        if (UsesSource)
        {
          Source = DIB_GetSource(BltInfo->SourceSurface, SourceX, SourceY, BltInfo->XlateSourceToDest);
        }

        if (BltInfo->PatternSurface)
        {
          Pattern = DIB_GetSourceIndex(BltInfo->PatternSurface, (DestX + BltInfo->BrushOrigin.x) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY);
        }

        DIB_1BPP_PutPixel(BltInfo->DestSurface, DestX, DestY, DIB_DoRop(BltInfo->Rop4, Dest, Source, Pattern) & 0xF);
      }
    }

    SourceY++;
    if (BltInfo->PatternSurface)
    {
      PatternY++;
      PatternY %= BltInfo->PatternSurface->sizlBitmap.cy;
    }
  }

  return TRUE;
}

/* BitBlt Optimize */
BOOLEAN
DIB_1BPP_ColorFill(SURFOBJ* DestSurface, RECTL* DestRect, ULONG color)
{
  LONG DestY;

  /* Make WellOrdered with top < bottom and left < right */
  RECTL_vMakeWellOrdered(DestRect);

  for (DestY = DestRect->top; DestY< DestRect->bottom; DestY++)
  {
    DIB_1BPP_HLine(DestSurface, DestRect->left, DestRect->right, DestY, color);
  }
  return TRUE;
}
#endif // !_USE_DIBLIB_

BOOLEAN
DIB_1BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                        RECTL*  DestRect,  RECTL *SourceRect,
                        XLATEOBJ *ColorTranslation, ULONG iTransColor)
{
  return FALSE;
}

/* EOF */
