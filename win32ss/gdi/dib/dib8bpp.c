/*
 * PROJECT:         Win32 subsystem
 * LICENSE:         See COPYING in the top level directory
 * FILE:            win32ss/gdi/dib/dib8bpp.c
 * PURPOSE:         Device Independant Bitmap functions, 8bpp
 * PROGRAMMERS:     Jason Filby
 *                  Thomas Bluemel
 *                  Gregor Anich
 *                  Doug Lyons
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

#define DEC_OR_INC(var, decTrue, amount) \
    ((var) = (decTrue) ? ((var) - (amount)) : ((var) + (amount)))

VOID
DIB_8BPP_PutPixel(SURFOBJ *SurfObj, LONG x, LONG y, ULONG c)
{
  PBYTE byteaddr = (PBYTE)SurfObj->pvScan0 + y * SurfObj->lDelta + x;

  *byteaddr = (BYTE)c;
}

ULONG
DIB_8BPP_GetPixel(SURFOBJ *SurfObj, LONG x, LONG y)
{
  PBYTE byteaddr = (PBYTE)SurfObj->pvScan0 + y * SurfObj->lDelta + x;

  return (ULONG)(*byteaddr);
}

VOID
DIB_8BPP_HLine(SURFOBJ *SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  memset((PBYTE)SurfObj->pvScan0 + y * SurfObj->lDelta + x1, (BYTE) c, x2 - x1);
}

VOID
DIB_8BPP_VLine(SURFOBJ *SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  PBYTE byteaddr = (PBYTE)SurfObj->pvScan0 + y1 * SurfObj->lDelta;
  PBYTE addr = byteaddr + x;
  LONG lDelta = SurfObj->lDelta;

  byteaddr = addr;
  while(y1++ < y2)
  {
    *addr = (BYTE)c;

    addr += lDelta;
  }
}

BOOLEAN
DIB_8BPP_BitBltSrcCopy(PBLTINFO BltInfo)
{
  LONG     i, j, sx, sy, xColor, f1;
  PBYTE    SourceBits, DestBits, SourceLine, DestLine;
  PBYTE    SourceBits_4BPP, SourceLine_4BPP;
  BOOLEAN  bTopToBottom, bLeftToRight;

  DPRINT("DIB_8BPP_BitBltSrcCopy: SrcSurf cx/cy (%d/%d), DestSuft cx/cy (%d/%d) dstRect: (%d,%d)-(%d,%d)\n",
         BltInfo->SourceSurface->sizlBitmap.cx, BltInfo->SourceSurface->sizlBitmap.cy,
         BltInfo->DestSurface->sizlBitmap.cx, BltInfo->DestSurface->sizlBitmap.cy,
         BltInfo->DestRect.left, BltInfo->DestRect.top, BltInfo->DestRect.right, BltInfo->DestRect.bottom);

  /* Get back left to right flip here */
  bLeftToRight = (BltInfo->DestRect.left > BltInfo->DestRect.right);

  /* Check for top to bottom flip needed. */
  bTopToBottom = BltInfo->DestRect.top > BltInfo->DestRect.bottom;

  DPRINT("bTopToBottom is '%d' and bLeftToRight is '%d'.\n", bTopToBottom, bLeftToRight);

  /* Make WellOrdered by making top < bottom and left < right */
  RECTL_vMakeWellOrdered(&BltInfo->DestRect);

  DPRINT("BPP is '%d' & BltInfo->SourcePoint.x is '%d' & BltInfo->SourcePoint.y is '%d'.\n",
         BltInfo->SourceSurface->iBitmapFormat, BltInfo->SourcePoint.x, BltInfo->SourcePoint.y);

  DestBits = (PBYTE)BltInfo->DestSurface->pvScan0 + (BltInfo->DestRect.top * BltInfo->DestSurface->lDelta) + BltInfo->DestRect.left;

  switch(BltInfo->SourceSurface->iBitmapFormat)
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
        sy += BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1;
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
            DIB_8BPP_PutPixel(BltInfo->DestSurface, i, j, XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, 0));
          }
          else
          {
            DIB_8BPP_PutPixel(BltInfo->DestSurface, i, j, XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, 1));
          }
          DEC_OR_INC(sx, bLeftToRight, 1);
        }
        DEC_OR_INC(sy, bTopToBottom, 1);
      }
      break;

    case BMF_4BPP:
      DPRINT("4BPP Case Selected with DestRect Width of '%d'.\n",
             BltInfo->DestRect.right - BltInfo->DestRect.left);

      /* This sets SourceBits_4BPP to the top line */
      SourceBits_4BPP = (PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + (BltInfo->SourcePoint.x >> 1);

      if (bTopToBottom)
      {
        /* This sets SourceBits_4BPP to the bottom line */
        SourceBits_4BPP += (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) * BltInfo->SourceSurface->lDelta;
      }

      for (j=BltInfo->DestRect.top; j<BltInfo->DestRect.bottom; j++)
      {
        SourceLine_4BPP = SourceBits_4BPP;
        sx = BltInfo->SourcePoint.x;

        if (bLeftToRight)
        {
          /* This sets sx to the rightmost pixel */
          sx += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
        }

        f1 = sx & 1;

        for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
        {
          xColor = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest,
              (*SourceLine_4BPP & altnotmask[f1]) >> (4 * (1 - f1)));
          DIB_8BPP_PutPixel(BltInfo->DestSurface, i, j, xColor);
          if(f1 == 1)
          {
            DEC_OR_INC(SourceLine_4BPP, bLeftToRight, 1);
            f1 = 0;
          }
          else
          {
            f1 = 1;
          }

          DEC_OR_INC(sx, bLeftToRight, 1);
        }
        DEC_OR_INC(SourceBits_4BPP, bTopToBottom, BltInfo->SourceSurface->lDelta);
      }
      break;

    case BMF_8BPP:
      DPRINT("8BPP-dstRect: (%d,%d)-(%d,%d) and Width of '%d'.\n",
              BltInfo->DestRect.left, BltInfo->DestRect.top,
              BltInfo->DestRect.right, BltInfo->DestRect.bottom,
              BltInfo->DestRect.right - BltInfo->DestRect.left);

      if ((BltInfo->XlateSourceToDest == NULL ||
        (BltInfo->XlateSourceToDest->flXlate & XO_TRIVIAL) != 0) &&
        (!bTopToBottom && !bLeftToRight))
      {
        if (BltInfo->DestRect.top < BltInfo->SourcePoint.y)
        {
          DPRINT("BltInfo->DestRect.top < BltInfo->SourcePoint.y.\n");
          SourceBits = (PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + BltInfo->SourcePoint.x;
          for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
          {
            RtlMoveMemory(DestBits, SourceBits, BltInfo->DestRect.right - BltInfo->DestRect.left);
            SourceBits += BltInfo->SourceSurface->lDelta;
            DestBits += BltInfo->DestSurface->lDelta;
          }
        }
        else
        {
          DPRINT("BltInfo->DestRect.top >= BltInfo->SourcePoint.y.\n");
          SourceBits = (PBYTE)BltInfo->SourceSurface->pvScan0 + ((BltInfo->SourcePoint.y + BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) * BltInfo->SourceSurface->lDelta) + BltInfo->SourcePoint.x;
          DestBits = (PBYTE)BltInfo->DestSurface->pvScan0 + ((BltInfo->DestRect.bottom - 1) * BltInfo->DestSurface->lDelta) + BltInfo->DestRect.left;
          for (j = BltInfo->DestRect.bottom - 1; BltInfo->DestRect.top <= j; j--)
          {
            RtlMoveMemory(DestBits, SourceBits, BltInfo->DestRect.right - BltInfo->DestRect.left);
            SourceBits -= BltInfo->SourceSurface->lDelta;
            DestBits -= BltInfo->DestSurface->lDelta;
          }
        }
      }
      else
      {
        DPRINT("XO_TRIVIAL is NOT TRUE or we have flips.\n");

        if (!bTopToBottom && !bLeftToRight)
      /* **Note: Indent is purposefully less than desired to keep reviewable differences to a minimum for PR** */
        {
        if (BltInfo->DestRect.top < BltInfo->SourcePoint.y)
        {
          DPRINT("Dest.top < SourcePoint.y.\n");
          SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + BltInfo->SourcePoint.x;
          DestLine = DestBits;
          for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
          {
            SourceBits = SourceLine;
            DestBits = DestLine;
            for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
            {
              *DestBits++ = (BYTE)XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *SourceBits++);
            }
            SourceLine += BltInfo->SourceSurface->lDelta;
            DestLine += BltInfo->DestSurface->lDelta;
          }
        }
        else
        {
          DPRINT("Dest.top >= SourcePoint.y.\n");
          SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 + ((BltInfo->SourcePoint.y + BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) * BltInfo->SourceSurface->lDelta) + BltInfo->SourcePoint.x;
          DestLine = (PBYTE)BltInfo->DestSurface->pvScan0 + ((BltInfo->DestRect.bottom - 1) * BltInfo->DestSurface->lDelta) + BltInfo->DestRect.left;
          for (j = BltInfo->DestRect.bottom - 1; BltInfo->DestRect.top <= j; j--)
          {
            SourceBits = SourceLine;
            DestBits = DestLine;
            for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
            {
              *DestBits++ = (BYTE)XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *SourceBits++);
            }
            SourceLine -= BltInfo->SourceSurface->lDelta;
            DestLine -= BltInfo->DestSurface->lDelta;
          }
        }
      }
      else
      {
          /* Buffering for source and destination flip overlaps. Fixes KHMZ MirrorTest CORE-16642 */
          BOOL OneDone = FALSE;

          if (bLeftToRight)
          {
            DPRINT("Flip is bLeftToRight.\n");

            /* Allocate enough pixels for a row in BYTE's */
            BYTE *store = ExAllocatePoolWithTag(NonPagedPool,
              BltInfo->DestRect.right - BltInfo->DestRect.left + 1, TAG_DIB);
            if (store == NULL)
            {
              DPRINT1("Storage Allocation Failed.\n");
              return FALSE;
            }
            WORD  Index;

            /* This sets SourceLine to the top line */
            SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 +
              (BltInfo->SourcePoint.y *
              BltInfo->SourceSurface->lDelta) +
              BltInfo->SourcePoint.x;

            /* This set the DestLine to the top line */
            DestLine = (PBYTE)BltInfo->DestSurface->pvScan0 +
              (BltInfo->DestRect.top * BltInfo->DestSurface->lDelta) +
              BltInfo->DestRect.left;

            for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
            {
              SourceBits = SourceLine;
              DestBits = DestLine;

              /* This sets SourceBits to the rightmost pixel */
              SourceBits += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);

              Index = 0;

              for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
              {
                store[Index] = (BYTE)XLATEOBJ_iXlate(
                  BltInfo->XlateSourceToDest,
                  *SourceBits);
                SourceBits--;
                Index++;
              }

              Index = 0;

              for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
              {
                *DestBits = store[Index];
                DestBits++;
                Index++;
              }

              SourceLine += BltInfo->SourceSurface->lDelta;
              DestLine += BltInfo->DestSurface->lDelta;
            }
            ExFreePoolWithTag(store, TAG_DIB);
            OneDone = TRUE;
          }

          if (bTopToBottom)
          {
            DPRINT("Flip is bTopToBottom.\n");

            DWORD  Index;

            /* Allocate enough pixels for a column in BYTE's */
            BYTE *store = ExAllocatePoolWithTag(NonPagedPool,
              BltInfo->DestRect.bottom - BltInfo->DestRect.top + 1, TAG_DIB);
            if (store == NULL)
            {
              DPRINT1("Storage Allocation Failed.\n");
              return FALSE;
            }

            /* The OneDone flag indicates that we are flipping for bTopToBottom and bLeftToRight
             * and have already completed the bLeftToRight. So we will lose our first flip output
             * unless we work with its output which is at the destination site. So in this case
             * our new Source becomes the previous outputs Destination. */

            if (OneDone)
            {
              /* This sets SourceLine to the bottom line of our previous destination */
              SourceLine = (PBYTE)BltInfo->DestSurface->pvScan0 +
                (BltInfo->DestRect.top * BltInfo->DestSurface->lDelta) + BltInfo->DestRect.left  +
                (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) * BltInfo->DestSurface->lDelta;
            }
            else
            {
              /* This sets SourceLine to the bottom line */
              SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 +
                ((BltInfo->SourcePoint.y + BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1)
                * BltInfo->SourceSurface->lDelta) +
                BltInfo->SourcePoint.x;
            }

            /* This set the DestLine to the top line */
            DestLine = (PBYTE)BltInfo->DestSurface->pvScan0 +
              (BltInfo->DestRect.top * BltInfo->DestSurface->lDelta) +
              BltInfo->DestRect.left;

            /* Read columns */
            for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
            {

              DestBits = DestLine;
              SourceBits = SourceLine;

              Index = 0;

              /* Read up the column and store the pixels */
              for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
              {
                store[Index] = (BYTE)XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *SourceBits);
                /* Go up a line */
                SourceBits -= BltInfo->SourceSurface->lDelta;
                Index++;
              }

              Index = 0;

              /* Get the stored pixel and copy then down the column */
              for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
              {
                *DestBits = store[Index];
                /* Go down a line */
                DestBits += BltInfo->SourceSurface->lDelta;
                Index++;
              }
              /* Index to next column */
              SourceLine += 1;
              DestLine += 1;
            }
            ExFreePoolWithTag(store, TAG_DIB);
          }

        }
      }
      break;

    case BMF_16BPP:
      DPRINT("16BPP Case Selected with DestRect Width of '%d'.\n",
             BltInfo->DestRect.right - BltInfo->DestRect.left);

      DPRINT("BMF_16BPP-dstRect: (%d,%d)-(%d,%d) and Width of '%d'.\n",
             BltInfo->DestRect.left, BltInfo->DestRect.top,
             BltInfo->DestRect.right, BltInfo->DestRect.bottom,
             BltInfo->DestRect.right - BltInfo->DestRect.left);

      SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + 2 * BltInfo->SourcePoint.x;

      if (bTopToBottom)
      {
        /* This sets SourceLine to the bottom line */
        SourceLine += (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) * BltInfo->SourceSurface->lDelta;;
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

        for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
        {
          xColor = *((PWORD) SourceBits);
          *DestBits = (BYTE)XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, xColor);

          DEC_OR_INC(SourceBits, bLeftToRight, 2);
          DestBits += 1;
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

      SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + 3 * BltInfo->SourcePoint.x;
      DestLine = DestBits;

      if (bTopToBottom)
      {
        /* This sets SourceLine to the bottom line */
        SourceLine += BltInfo->SourceSurface->lDelta * (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1);
      }

      for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;

        if (bLeftToRight)
        {
          /* This sets the SourceBits to the rightmost pixel */
          SourceBits += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1) * 3;
        }

        for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
        {
          xColor = (*(SourceBits + 2) << 0x10) +
             (*(SourceBits + 1) << 0x08) +
             (*(SourceBits));
          *DestBits = (BYTE)XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, xColor);
          DEC_OR_INC(SourceBits, bLeftToRight, 3);
          DestBits += 1;
        }
        DEC_OR_INC(SourceLine, bTopToBottom, BltInfo->SourceSurface->lDelta);
        DestLine += BltInfo->DestSurface->lDelta;
      }
      break;

    case BMF_32BPP:
      DPRINT("32BPP Case Selected with DestRect Width of '%d'.\n",
             BltInfo->DestRect.right - BltInfo->DestRect.left);

      SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + 4 * BltInfo->SourcePoint.x;

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

        for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
        {
          xColor = *((PDWORD) SourceBits);
          *DestBits = (BYTE)XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, xColor);

          DEC_OR_INC(SourceBits, bLeftToRight, 4);
          DestBits += 1;
        }
        DEC_OR_INC(SourceLine, bTopToBottom, BltInfo->SourceSurface->lDelta);
        DestLine += BltInfo->DestSurface->lDelta;
      }
      break;

    default:
      DPRINT1("DIB_8BPP_Bitblt: Unhandled Source BPP: %u\n", BitsPerFormat(BltInfo->SourceSurface->iBitmapFormat));
      return FALSE;
  }

  return TRUE;
}

/* BitBlt Optimize */
BOOLEAN
DIB_8BPP_ColorFill(SURFOBJ* DestSurface, RECTL* DestRect, ULONG color)
{
  LONG DestY;

  /* Make WellOrdered by making top < bottom and left < right */
  RECTL_vMakeWellOrdered(DestRect);

  for (DestY = DestRect->top; DestY< DestRect->bottom; DestY++)
  {
    DIB_8BPP_HLine(DestSurface, DestRect->left, DestRect->right, DestY, color);
  }
  return TRUE;
}


BOOLEAN
DIB_8BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                        RECTL*  DestRect,  RECTL *SourceRect,
                        XLATEOBJ *ColorTranslation, ULONG iTransColor)
{
  LONG RoundedRight, X, Y, SourceX = 0, SourceY = 0;
  ULONG *DestBits, Source, Dest;

  LONG DstHeight;
  LONG DstWidth;
  LONG SrcHeight;
  LONG SrcWidth;

  DstHeight = DestRect->bottom - DestRect->top;
  DstWidth = DestRect->right - DestRect->left;
  SrcHeight = SourceRect->bottom - SourceRect->top;
  SrcWidth = SourceRect->right - SourceRect->left;

  RoundedRight = DestRect->right - ((DestRect->right - DestRect->left) & 0x3);
  DestBits = (ULONG*)((PBYTE)DestSurf->pvScan0 + DestRect->left +
                      (DestRect->top * DestSurf->lDelta));

  for(Y = DestRect->top; Y < DestRect->bottom; Y++)
  {
    DestBits = (ULONG*)((PBYTE)DestSurf->pvScan0 + DestRect->left +
                        (Y * DestSurf->lDelta));
    SourceY = SourceRect->top+(Y - DestRect->top) * SrcHeight / DstHeight;
    for (X = DestRect->left; X < RoundedRight; X += 4, DestBits++)
    {
      Dest = *DestBits;

      SourceX = SourceRect->left+(X - DestRect->left) * SrcWidth / DstWidth;
      if (SourceX >= 0 && SourceY >= 0 &&
          SourceSurf->sizlBitmap.cx > SourceX && SourceSurf->sizlBitmap.cy > SourceY)
      {
        Source = DIB_GetSourceIndex(SourceSurf, SourceX, SourceY);
        if(Source != iTransColor)
        {
          Dest &= 0xFFFFFF00;
          Dest |= (XLATEOBJ_iXlate(ColorTranslation, Source) & 0xFF);
        }
      }

      SourceX = SourceRect->left+(X+1 - DestRect->left) * SrcWidth / DstWidth;
      if (SourceX >= 0 && SourceY >= 0 &&
          SourceSurf->sizlBitmap.cx > SourceX && SourceSurf->sizlBitmap.cy > SourceY)
      {
        Source = DIB_GetSourceIndex(SourceSurf, SourceX, SourceY);
        if(Source != iTransColor)
        {
          Dest &= 0xFFFF00FF;
          Dest |= ((XLATEOBJ_iXlate(ColorTranslation, Source) << 8) & 0xFF00);
        }
      }

      SourceX = SourceRect->left+(X+2 - DestRect->left) * SrcWidth / DstWidth;
      if (SourceX >= 0 && SourceY >= 0 &&
          SourceSurf->sizlBitmap.cx > SourceX && SourceSurf->sizlBitmap.cy > SourceY)
      {
        Source = DIB_GetSourceIndex(SourceSurf, SourceX, SourceY);
        if(Source != iTransColor)
        {
          Dest &= 0xFF00FFFF;
          Dest |= ((XLATEOBJ_iXlate(ColorTranslation, Source) << 16) & 0xFF0000);
        }
      }

      SourceX = SourceRect->left+(X+3 - DestRect->left) * SrcWidth / DstWidth;
      if (SourceX >= 0 && SourceY >= 0 &&
          SourceSurf->sizlBitmap.cx > SourceX && SourceSurf->sizlBitmap.cy > SourceY)
      {
        Source = DIB_GetSourceIndex(SourceSurf, SourceX, SourceY);
        if(Source != iTransColor)
        {
          Dest &= 0x00FFFFFF;
          Dest |= ((XLATEOBJ_iXlate(ColorTranslation, Source) << 24) & 0xFF000000);
        }
      }

      *DestBits = Dest;
    }

    if(X < DestRect->right)
    {
      for (; X < DestRect->right; X++)
      {
        SourceX = SourceRect->left+(X - DestRect->left) * SrcWidth / DstWidth;
        if (SourceX >= 0 && SourceY >= 0 &&
            SourceSurf->sizlBitmap.cx > SourceX && SourceSurf->sizlBitmap.cy > SourceY)
        {
          Source = DIB_GetSourceIndex(SourceSurf, SourceX, SourceY);
          if(Source != iTransColor)
          {
            *((BYTE*)DestBits) = (BYTE)(XLATEOBJ_iXlate(ColorTranslation, Source) & 0xFF);
          }
        }
        DestBits = (PULONG)((ULONG_PTR)DestBits + 1);
      }
    }
  }

  return TRUE;
}

/* EOF */
