/*
 * PROJECT:         Win32 subsystem
 * LICENSE:         See COPYING in the top level directory
 * FILE:            win32ss/gdi/dib/dib4bpp.c
 * PURPOSE:         Device Independant Bitmap functions, 4bpp
 * PROGRAMMERS:     Jason Filby
 *                  Doug Lyons
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

#define DEC_OR_INC(var, decTrue, amount) \
    ((var) = (decTrue) ? ((var) - (amount)) : ((var) + (amount)))

VOID
DIB_4BPP_PutPixel(SURFOBJ *SurfObj, LONG x, LONG y, ULONG c)
{
   PBYTE addr = (PBYTE)SurfObj->pvScan0 + (x>>1) + y * SurfObj->lDelta;
   *addr = (*addr & notmask[x&1]) | (BYTE)(c << ((1-(x&1))<<2));
}

ULONG
DIB_4BPP_GetPixel(SURFOBJ *SurfObj, LONG x, LONG y)
{
   PBYTE addr = (PBYTE)SurfObj->pvScan0 + (x>>1) + y * SurfObj->lDelta;
   return (*addr >> ((1-(x&1))<<2)) & 0x0f;
}

VOID
DIB_4BPP_HLine(SURFOBJ *SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  PBYTE  addr = (PBYTE)SurfObj->pvScan0 + (x1>>1) + y * SurfObj->lDelta;
  LONG  cx = x1;

  while(cx < x2)
  {
    *addr = (*addr & notmask[x1&1]) | (BYTE)(c << ((1-(x1&1))<<2));
    if((++x1 & 1) == 0)
      ++addr;
    ++cx;
  }
}

VOID
DIB_4BPP_VLine(SURFOBJ *SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  PBYTE  addr = SurfObj->pvScan0;
  int  lDelta = SurfObj->lDelta;

  addr += (x>>1) + y1 * lDelta;
  while(y1++ < y2)
  {
    *addr = (*addr & notmask[x&1]) | (BYTE)(c << ((1-(x&1))<<2));
    addr += lDelta;
  }
}

#ifndef _USE_DIBLIB_
BOOLEAN
DIB_4BPP_BitBltSrcCopy(PBLTINFO BltInfo)
{
  LONG     i, j, sx, sy, f2, xColor;
  PBYTE    SourceBits_24BPP, SourceLine_24BPP;
  PBYTE    DestBits, DestLine, SourceBits_8BPP, SourceLine_8BPP;
  PBYTE    SourceBits, SourceLine;
  BOOLEAN  bTopToBottom, bLeftToRight;

  DPRINT("DIB_4BPP_BitBltSrcCopy: SrcSurf cx/cy (%d/%d), DestSuft cx/cy (%d/%d) dstRect: (%d,%d)-(%d,%d)\n",
         BltInfo->SourceSurface->sizlBitmap.cx, BltInfo->SourceSurface->sizlBitmap.cy,
         BltInfo->DestSurface->sizlBitmap.cx, BltInfo->DestSurface->sizlBitmap.cy,
         BltInfo->DestRect.left, BltInfo->DestRect.top, BltInfo->DestRect.right, BltInfo->DestRect.bottom);

  /* Get back left to right flip here */
  bLeftToRight = (BltInfo->DestRect.left > BltInfo->DestRect.right);

  /* Check for top to bottom flip needed. */
  bTopToBottom = BltInfo->DestRect.top > BltInfo->DestRect.bottom;

  /* Make WellOrdered with top < bottom and left < right */
  RECTL_vMakeWellOrdered(&BltInfo->DestRect);

  DPRINT("BPP is '%d/%d' & BltInfo->SourcePoint.x is '%d' & BltInfo->SourcePoint.y is '%d'.\n",
         BltInfo->SourceSurface->iBitmapFormat, BltInfo->SourcePoint.x, BltInfo->SourcePoint.y);

  DestBits = (PBYTE)BltInfo->DestSurface->pvScan0 +
             (BltInfo->DestRect.left >> 1) +
             BltInfo->DestRect.top * BltInfo->DestSurface->lDelta;

  switch (BltInfo->SourceSurface->iBitmapFormat)
  {
    case BMF_1BPP:
      DPRINT("1BPP Case Selected with DestRect Width of '%d'.\n",
             BltInfo->DestRect.right - BltInfo->DestRect.left);

      sx = BltInfo->SourcePoint.x;

      /* This sets sy to the top line */
      sy = BltInfo->SourcePoint.y;

      if (bTopToBottom)
      {
        /* This sets sy to the bottom line */
        sy += (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) * BltInfo->SourceSurface->lDelta;
      }

      for (j=BltInfo->DestRect.top; j<BltInfo->DestRect.bottom; j++)
      {
        sx = BltInfo->SourcePoint.x;

        if (bLeftToRight)
        {
          /* This sets the sx to the rightmost pixel */
          sx += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
        }

        for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
        {
          if(DIB_1BPP_GetPixel(BltInfo->SourceSurface, sx, sy) == 0)
          {
            DIB_4BPP_PutPixel(BltInfo->DestSurface, i, j, XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, 0));
          }
          else
          {
            DIB_4BPP_PutPixel(BltInfo->DestSurface, i, j, XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, 1));
          }
          DEC_OR_INC(sx, bLeftToRight, 1);
        }
        DEC_OR_INC(sy, bTopToBottom, 1);
      }
      break;

    case BMF_4BPP:
      DPRINT("4BPP Case Selected with DestRect Width of '%d'.\n",
             BltInfo->DestRect.right - BltInfo->DestRect.left);

      /* This sets sy to the top line */
      sy = BltInfo->SourcePoint.y;

      if (bTopToBottom)
      {
        /* This sets sy to the bottom line */
        sy += (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1)
              * BltInfo->SourceSurface->lDelta;
      }

      for (j=BltInfo->DestRect.top; j<BltInfo->DestRect.bottom; j++)
      {
        sx = BltInfo->SourcePoint.x;

        if (bLeftToRight)
        {
          /* This sets the sx to the rightmost pixel */
          sx += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
        }

        for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
        {
          if (NULL != BltInfo->XlateSourceToDest)
          {
            DIB_4BPP_PutPixel(BltInfo->DestSurface, i, j,
              XLATEOBJ_iXlate(BltInfo->XlateSourceToDest,
              DIB_4BPP_GetPixel(BltInfo->SourceSurface, sx, sy)));
          }
          else
          {
            DIB_4BPP_PutPixel(BltInfo->DestSurface, i, j,
              DIB_4BPP_GetPixel(BltInfo->SourceSurface, sx, sy));
          }
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

      SourceBits_8BPP = (PBYTE)BltInfo->SourceSurface->pvScan0 +
        (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + BltInfo->SourcePoint.x;

      if (bTopToBottom)
      {
        /* This sets SourceBits to the bottom line */
        SourceBits_8BPP = (PBYTE)((LONG_PTR)SourceBits_8BPP +
          ((BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) *
          BltInfo->SourceSurface->lDelta));
      }

      for (j=BltInfo->DestRect.top; j<BltInfo->DestRect.bottom; j++)
      {
        SourceLine_8BPP = SourceBits_8BPP;
        DestLine = DestBits;

        if (bLeftToRight)
        {
          /* This sets SourceBits_8BPP to the rightmost pixel */
          SourceBits_8BPP += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
        }

        f2 = BltInfo->DestRect.left & 1;

        for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
        {
          *DestLine = (*DestLine & notmask[f2]) |
            (BYTE)((XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *SourceLine_8BPP)) << ((4 * (1 - f2))));
          if (f2 == 1) { DestLine++; f2 = 0; } else { f2 = 1; }
          DEC_OR_INC(SourceLine_8BPP, bLeftToRight, 1);
        }
        DEC_OR_INC(SourceBits_8BPP, bTopToBottom, BltInfo->SourceSurface->lDelta);
        DestBits += BltInfo->DestSurface->lDelta;
      }
      break;

    case BMF_16BPP:
      DPRINT("16BPP Case Selected with DestRect Width of '%d'.\n",
             BltInfo->DestRect.right - BltInfo->DestRect.left);

      DPRINT("BMF_16BPP-dstRect: (%d,%d)-(%d,%d) and Width of '%d'.\n",
             BltInfo->DestRect.left, BltInfo->DestRect.top,
             BltInfo->DestRect.right, BltInfo->DestRect.bottom,
             BltInfo->DestRect.right - BltInfo->DestRect.left);

      SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 +
        (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) +
        2 * BltInfo->SourcePoint.x;

      if (bTopToBottom)
      {
        /* This sets SourceLine to the bottom line */
        SourceLine += (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) *
          BltInfo->SourceSurface->lDelta;;
      }

      DestLine = DestBits;

      for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
      {
        SourceBits = SourceLine;

        if (bLeftToRight)
        {
          /* This sets SourceBits to the rightmost pixel */
          SourceBits += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1) * 2;
        }

        DestBits = DestLine;
        f2 = BltInfo->DestRect.left & 1;

        for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
        {
          xColor = *((PWORD) SourceBits);
          *DestBits = (*DestBits & notmask[f2]) |
            (BYTE)((XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, xColor)) << ((4 * (1 - f2))));
          if(f2 == 1) { DestBits++; f2 = 0; } else { f2 = 1; }

          DEC_OR_INC(SourceBits, bLeftToRight, 2);
        }

        DEC_OR_INC(SourceLine, bTopToBottom, BltInfo->SourceSurface->lDelta);
        DestLine += BltInfo->DestSurface->lDelta;
      }
      break;

    case BMF_24BPP:

      DPRINT("24BPP-dstRect: (%d,%d)-(%d,%d) and Width of '%d'.\n",
             BltInfo->DestRect.left, BltInfo->DestRect.top,
             BltInfo->DestRect.right, BltInfo->DestRect.bottom,
             BltInfo->DestRect.right - BltInfo->DestRect.left);

      SourceBits_24BPP = (PBYTE)BltInfo->SourceSurface->pvScan0 +
        (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) +
        BltInfo->SourcePoint.x * 3;

      if (bTopToBottom)
      {
        /* This sets SourceLine to the bottom line */
        SourceBits_24BPP += BltInfo->SourceSurface->lDelta *
          (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1);
      }

      for (j=BltInfo->DestRect.top; j<BltInfo->DestRect.bottom; j++)
      {
        SourceLine_24BPP = SourceBits_24BPP;
        DestLine = DestBits;
        f2 = BltInfo->DestRect.left & 1;

        if (bLeftToRight)
        {
          /* This sets the SourceBits_24BPP to the rightmost pixel */
          SourceLine_24BPP += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1) * 3;
        }

        for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
        {
          xColor = (*(SourceLine_24BPP + 2) << 0x10) +
             (*(SourceLine_24BPP + 1) << 0x08) +
             (*(SourceLine_24BPP));
          *DestLine = (*DestLine & notmask[f2]) |
            (BYTE)((XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, xColor)) << ((4 * (1 - f2))));
          if(f2 == 1) { DestLine++; f2 = 0; } else { f2 = 1; }
          DEC_OR_INC(SourceLine_24BPP, bLeftToRight, 3);
        }
        DEC_OR_INC(SourceBits_24BPP, bTopToBottom, BltInfo->SourceSurface->lDelta);
        DestBits += BltInfo->DestSurface->lDelta;
      }
      break;

    case BMF_32BPP:
      DPRINT("32BPP Case Selected with DestRect Width of '%d'.\n",
             BltInfo->DestRect.right - BltInfo->DestRect.left);

      SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 +
        (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) +
        4 * BltInfo->SourcePoint.x;

      if (bTopToBottom)
      {
        /* This sets SourceLine to the bottom line */
        SourceLine += BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1;
      }

      DestLine = DestBits;

      for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;

        if (bLeftToRight)
        {
          /* This sets SourceBits to the rightmost pixel */
          SourceBits += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1) * 4;
        }

        f2 = BltInfo->DestRect.left & 1;

        for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
        {
          xColor = *((PDWORD) SourceBits);
          *DestBits = (*DestBits & notmask[f2]) |
            (BYTE)((XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, xColor)) << ((4 * (1 - f2))));
          if(f2 == 1) { DestBits++; f2 = 0; } else { f2 = 1; }

          DEC_OR_INC(SourceBits, bLeftToRight, 4);
        }

        DEC_OR_INC(SourceLine, bTopToBottom, BltInfo->SourceSurface->lDelta);
        DestLine += BltInfo->DestSurface->lDelta;
      }
      break;

    default:
      DbgPrint("DIB_4BPP_BitBltSrcCopy: Unhandled Source BPP: %u\n",
        BitsPerFormat(BltInfo->SourceSurface->iBitmapFormat));
      return FALSE;
  }
  return(TRUE);
}

BOOLEAN
DIB_4BPP_BitBlt(PBLTINFO BltInfo)
{
  LONG DestX, DestY;
  LONG SourceX, SourceY;
  LONG PatternY = 0;
  ULONG Dest, Source = 0, Pattern = 0;
  BOOLEAN UsesSource;
  BOOLEAN UsesPattern;
  PULONG DestBits;
  LONG RoundedRight;
  static const ULONG ExpandSolidColor[16] =
  {
    0x00000000 /* 0 */,
    0x11111111 /* 1 */,
    0x22222222 /* 2 */,
    0x33333333 /* 3 */,
    0x44444444 /* 4 */,
    0x55555555 /* 5 */,
    0x66666666 /* 6 */,
    0x77777777 /* 7 */,
    0x88888888 /* 8 */,
    0x99999999 /* 9 */,
    0xAAAAAAAA /* 10 */,
    0xBBBBBBBB /* 11 */,
    0xCCCCCCCC /* 12 */,
    0xDDDDDDDD /* 13 */,
    0xEEEEEEEE /* 14 */,
    0xFFFFFFFF /* 15 */,
  };

  UsesSource = ROP4_USES_SOURCE(BltInfo->Rop4);
  UsesPattern = ROP4_USES_PATTERN(BltInfo->Rop4);

  SourceY = BltInfo->SourcePoint.y;
  RoundedRight = BltInfo->DestRect.right -
    ((BltInfo->DestRect.right - BltInfo->DestRect.left) & 0x7);

  if (UsesPattern)
  {
    if (BltInfo->PatternSurface)
    {
      PatternY = (BltInfo->DestRect.top + BltInfo->BrushOrigin.y) %
        BltInfo->PatternSurface->sizlBitmap.cy;
    }
    else
    {
      if (BltInfo->Brush)
        Pattern = ExpandSolidColor[BltInfo->Brush->iSolidColor];
    }
  }

  for (DestY = BltInfo->DestRect.top; DestY < BltInfo->DestRect.bottom; DestY++)
  {
    DestBits = (PULONG)(
      (PBYTE)BltInfo->DestSurface->pvScan0 +
      (BltInfo->DestRect.left >> 1) +
      DestY * BltInfo->DestSurface->lDelta);
    SourceX = BltInfo->SourcePoint.x;
    DestX = BltInfo->DestRect.left;

    if (DestX & 0x1)
    {
      Dest = DIB_4BPP_GetPixel(BltInfo->DestSurface, DestX, DestY);

      if (UsesSource)
      {
        Source = DIB_GetSource(BltInfo->SourceSurface, SourceX, SourceY, BltInfo->XlateSourceToDest);
      }

      if (BltInfo->PatternSurface)
      {
        Pattern = DIB_GetSourceIndex(BltInfo->PatternSurface,
         (DestX + BltInfo->BrushOrigin.x) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY);
      }

      DIB_4BPP_PutPixel(BltInfo->DestSurface, DestX, DestY, DIB_DoRop(BltInfo->Rop4, Dest, Source, Pattern) & 0xF);

      DestX++;
      SourceX++;
      DestBits = (PULONG)((ULONG_PTR)DestBits + 1);
    }

    for (; DestX < RoundedRight; DestX += 8, SourceX += 8, DestBits++)
    {
      Dest = *DestBits;
      if (UsesSource)
      {
        Source =
          (DIB_GetSource(BltInfo->SourceSurface, SourceX + 1, SourceY, BltInfo->XlateSourceToDest)) |
          (DIB_GetSource(BltInfo->SourceSurface, SourceX + 0, SourceY, BltInfo->XlateSourceToDest) << 4) |
          (DIB_GetSource(BltInfo->SourceSurface, SourceX + 3, SourceY, BltInfo->XlateSourceToDest) << 8) |
          (DIB_GetSource(BltInfo->SourceSurface, SourceX + 2, SourceY, BltInfo->XlateSourceToDest) << 12) |
          (DIB_GetSource(BltInfo->SourceSurface, SourceX + 5, SourceY, BltInfo->XlateSourceToDest) << 16) |
          (DIB_GetSource(BltInfo->SourceSurface, SourceX + 4, SourceY, BltInfo->XlateSourceToDest) << 20) |
          (DIB_GetSource(BltInfo->SourceSurface, SourceX + 7, SourceY, BltInfo->XlateSourceToDest) << 24) |
          (DIB_GetSource(BltInfo->SourceSurface, SourceX + 6, SourceY, BltInfo->XlateSourceToDest) << 28);
      }
      if (BltInfo->PatternSurface)
      {
        Pattern = DIB_GetSourceIndex(BltInfo->PatternSurface, (DestX + BltInfo->BrushOrigin.x + 1) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY);
        Pattern |= DIB_GetSourceIndex(BltInfo->PatternSurface, (DestX + BltInfo->BrushOrigin.x + 0) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY) << 4;
        Pattern |= DIB_GetSourceIndex(BltInfo->PatternSurface, (DestX + BltInfo->BrushOrigin.x + 3) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY) << 8;
        Pattern |= DIB_GetSourceIndex(BltInfo->PatternSurface, (DestX + BltInfo->BrushOrigin.x + 2) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY) << 12;
        Pattern |= DIB_GetSourceIndex(BltInfo->PatternSurface, (DestX + BltInfo->BrushOrigin.x + 5) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY) << 16;
        Pattern |= DIB_GetSourceIndex(BltInfo->PatternSurface, (DestX + BltInfo->BrushOrigin.x + 4) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY) << 20;
        Pattern |= DIB_GetSourceIndex(BltInfo->PatternSurface, (DestX + BltInfo->BrushOrigin.x + 7) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY) << 24;
        Pattern |= DIB_GetSourceIndex(BltInfo->PatternSurface, (DestX + BltInfo->BrushOrigin.x + 6) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY) << 28;
      }
      *DestBits = DIB_DoRop(BltInfo->Rop4, Dest, Source, Pattern);
    }

    /* Process the rest of pixel on the line */
    for (; DestX < BltInfo->DestRect.right; DestX++, SourceX++)
    {
      Dest = DIB_4BPP_GetPixel(BltInfo->DestSurface, DestX, DestY);
      if (UsesSource)
      {
        Source = DIB_GetSource(BltInfo->SourceSurface, SourceX, SourceY, BltInfo->XlateSourceToDest);
      }
      if (BltInfo->PatternSurface)
      {
        Pattern = DIB_GetSourceIndex(BltInfo->PatternSurface,
          (DestX + BltInfo->BrushOrigin.x) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY);
      }
      DIB_4BPP_PutPixel(BltInfo->DestSurface, DestX, DestY, DIB_DoRop(BltInfo->Rop4, Dest, Source, Pattern) & 0xF);
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
DIB_4BPP_ColorFill(SURFOBJ* DestSurface, RECTL* DestRect, ULONG color)
{
  LONG DestY;

  /* Make WellOrdered by making top < bottom and left < right */
  RECTL_vMakeWellOrdered(DestRect);

  for (DestY = DestRect->top; DestY < DestRect->bottom; DestY++)
  {
    DIB_4BPP_HLine(DestSurface, DestRect->left, DestRect->right, DestY, color);
  }
  return TRUE;
}
#endif // !_USE_DIBLIB_

BOOLEAN
DIB_4BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                        RECTL*  DestRect,  RECTL *SourceRect,
                        XLATEOBJ *ColorTranslation, ULONG iTransColor)
{
  return FALSE;
}

/* EOF */
