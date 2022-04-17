/*
 * PROJECT:         Win32 subsystem
 * LICENSE:         See COPYING in the top level directory
 * FILE:            win32ss/gdi/dib/dib24bpp.c
 * PURPOSE:         Device Independant Bitmap functions, 24bpp
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
DIB_24BPP_PutPixel(SURFOBJ *SurfObj, LONG x, LONG y, ULONG c)
{
  PBYTE addr = (PBYTE)SurfObj->pvScan0 + (y * SurfObj->lDelta) + (x << 1) + x;
  *(PUSHORT)(addr) = c & 0xFFFF;
  *(addr + 2) = (c >> 16) & 0xFF;
}

ULONG
DIB_24BPP_GetPixel(SURFOBJ *SurfObj, LONG x, LONG y)
{
  PBYTE addr = (PBYTE)SurfObj->pvScan0 + y * SurfObj->lDelta + (x << 1) + x;
  return *(PUSHORT)(addr) + (*(addr + 2) << 16);
}



VOID
DIB_24BPP_VLine(SURFOBJ *SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  PBYTE addr = (PBYTE)SurfObj->pvScan0 + y1 * SurfObj->lDelta + (x << 1) + x;
  LONG lDelta = SurfObj->lDelta;

  c &= 0xFFFFFF;
  while(y1++ < y2)
  {
    *(PUSHORT)(addr) = c & 0xFFFF;
    *(addr + 2) = (BYTE)(c >> 16);

    addr += lDelta;
  }
}

BOOLEAN
DIB_24BPP_BitBltSrcCopy(PBLTINFO BltInfo)
{
  LONG     i, j, sx, sy, xColor, f1;
  PBYTE    SourceBits, DestBits, SourceLine, DestLine;
  PBYTE    SourceBits_4BPP, SourceLine_4BPP;
  PWORD    SourceBits_16BPP, SourceLine_16BPP;
  BOOLEAN  bTopToBottom, bLeftToRight;

  DPRINT("DIB_24BPP_BitBltSrcCopy: SrcSurf cx/cy (%d/%d), DestSuft cx/cy (%d/%d) dstRect: (%d,%d)-(%d,%d)\n",
         BltInfo->SourceSurface->sizlBitmap.cx, BltInfo->SourceSurface->sizlBitmap.cy,
         BltInfo->DestSurface->sizlBitmap.cx, BltInfo->DestSurface->sizlBitmap.cy,
         BltInfo->DestRect.left, BltInfo->DestRect.top, BltInfo->DestRect.right, BltInfo->DestRect.bottom);

  /* Get back left to right flip here */
  bLeftToRight = (BltInfo->DestRect.left > BltInfo->DestRect.right);

  /* Check for top to bottom flip needed. */
  bTopToBottom = BltInfo->DestRect.top > BltInfo->DestRect.bottom;

  DPRINT("BltInfo->SourcePoint.x is '%d' and BltInfo->SourcePoint.y is '%d'.\n",
         BltInfo->SourcePoint.x, BltInfo->SourcePoint.y);

  /* Make WellOrdered by making top < bottom and left < right */
  RECTL_vMakeWellOrdered(&BltInfo->DestRect);

  DestBits = (PBYTE)BltInfo->DestSurface->pvScan0 + (BltInfo->DestRect.top * BltInfo->DestSurface->lDelta) + BltInfo->DestRect.left * 3;

  switch(BltInfo->SourceSurface->iBitmapFormat)
  {
    case BMF_1BPP:
      sx = BltInfo->SourcePoint.x;

      /* This sets sy to the top line */
      sy = BltInfo->SourcePoint.y;

      if (bTopToBottom)
      {
        /* This sets sy to the bottom line */
        sy += BltInfo->SourceSurface->lDelta * (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1);
      }

      for (j=BltInfo->DestRect.top; j<BltInfo->DestRect.bottom; j++)
      {
        sx = BltInfo->SourcePoint.x;

        if (bLeftToRight)
        {
          /* This sets sx to the rightmost pixel */
          sx += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
        }

        for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
        {
          if(DIB_1BPP_GetPixel(BltInfo->SourceSurface, sx, sy) == 0)
          {
            DIB_24BPP_PutPixel(BltInfo->DestSurface, i, j, XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, 0));
          } else {
            DIB_24BPP_PutPixel(BltInfo->DestSurface, i, j, XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, 1));
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
        SourceBits_4BPP += BltInfo->SourceSurface->lDelta * (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1);
      }

      for (j=BltInfo->DestRect.top; j<BltInfo->DestRect.bottom; j++)
      {
        SourceLine_4BPP = SourceBits_4BPP;
        DestLine = DestBits;
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
          *DestLine++ = xColor & 0xff;
          *(PWORD)DestLine = (WORD)(xColor >> 8);
          DestLine += 2;
          if(f1 == 1) {
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
        DestBits += BltInfo->DestSurface->lDelta;
      }
      break;

    case BMF_8BPP:
      DPRINT("8BPP Case Selected with DestRect Width of '%d'.\n",
             BltInfo->DestRect.right - BltInfo->DestRect.left);

      /* This sets SourceLine to the top line */
      SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + BltInfo->SourcePoint.x;

      if (bTopToBottom)
      {
        /* This sets SourceLine to the bottom line */
        SourceLine += BltInfo->SourceSurface->lDelta * (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1);
      }
      DestLine = DestBits;

      for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;

        if (bLeftToRight)
        {
          /* This sets the SourceBits to the rightmost pixel */
          SourceBits += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
        }

        for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
        {
          xColor = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *SourceBits);
          *DestBits = xColor & 0xff;
          *(PWORD)(DestBits + 1) = (WORD)(xColor >> 8);
          DEC_OR_INC(SourceBits, bLeftToRight, 1);
          DestBits += 3;
        }

        DEC_OR_INC(SourceLine, bTopToBottom, BltInfo->SourceSurface->lDelta);
        DestLine += BltInfo->DestSurface->lDelta;
      }
      break;

    case BMF_16BPP:
      DPRINT("16BPP Case Selected with DestRect Width of '%d'.\n",
             BltInfo->DestRect.right - BltInfo->DestRect.left);

      /* This sets SourceBits_16BPP to the top line */
      SourceBits_16BPP = (PWORD)((PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + 2 * BltInfo->SourcePoint.x);

      if (bTopToBottom)
      {
        /* This sets SourceBits_16BPP to the bottom line */
        SourceBits_16BPP += (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) * BltInfo->SourceSurface->lDelta;
      }

      for (j=BltInfo->DestRect.top; j<BltInfo->DestRect.bottom; j++)
      {
        SourceLine_16BPP = SourceBits_16BPP;
        DestLine = DestBits;

        if (bLeftToRight)
        {
          /* This sets the SourceLine_16BPP to the rightmost pixel */
          SourceLine_16BPP += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
        }

        for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
        {
          xColor = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *SourceLine_16BPP);
          *DestLine++ = xColor & 0xff;
          *(PWORD)DestLine = (WORD)(xColor >> 8);
          DestLine += 2;
          DEC_OR_INC(SourceLine_16BPP, bLeftToRight, 1);
        }
        if (bTopToBottom)
        {
          SourceBits_16BPP = (PWORD)((PBYTE)SourceBits_16BPP - BltInfo->SourceSurface->lDelta);
        }
        else
        {
          SourceBits_16BPP = (PWORD)((PBYTE)SourceBits_16BPP + BltInfo->SourceSurface->lDelta);
        }
        DestBits += BltInfo->DestSurface->lDelta;
      }
      break;

    case BMF_24BPP:
      DPRINT("24BPP Case Selected with DestRect Width of '%d'.\n",
             BltInfo->DestRect.right - BltInfo->DestRect.left);

      /* Check for no flips here because we are about to use RtlMoveMemory and it can only do increasing src & dst */
      if ((BltInfo->XlateSourceToDest == NULL ||
        (BltInfo->XlateSourceToDest->flXlate & XO_TRIVIAL) != 0) &&
        (!bTopToBottom && !bLeftToRight))
      {
        DPRINT("XO_TRIVIAL is TRUE.\n");
        if (BltInfo->DestRect.top < BltInfo->SourcePoint.y)
        {
          /* This sets SourceBits to the top line */
          SourceBits = (PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + 3 * BltInfo->SourcePoint.x;
          for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
          {
            RtlMoveMemory(DestBits, SourceBits, 3 * (BltInfo->DestRect.right - BltInfo->DestRect.left));
            SourceBits += BltInfo->SourceSurface->lDelta;
            DestBits += BltInfo->DestSurface->lDelta;
          }
        }
        else
        {
          SourceBits = (PBYTE)BltInfo->SourceSurface->pvScan0 + ((BltInfo->SourcePoint.y + BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) * BltInfo->SourceSurface->lDelta) + 3 * BltInfo->SourcePoint.x;
          DestBits = (PBYTE)BltInfo->DestSurface->pvScan0 + ((BltInfo->DestRect.bottom - 1) * BltInfo->DestSurface->lDelta) + 3 * BltInfo->DestRect.left;
          for (j = BltInfo->DestRect.bottom - 1; BltInfo->DestRect.top <= j; j--)
          {
            RtlMoveMemory(DestBits, SourceBits, 3 * (BltInfo->DestRect.right - BltInfo->DestRect.left));
            SourceBits -= BltInfo->SourceSurface->lDelta;
            DestBits -= BltInfo->DestSurface->lDelta;
          }
        }
      }
      else
      {
        DPRINT("XO_TRIVIAL is NOT TRUE.\n");

        if (!bTopToBottom && !bLeftToRight)
      /* **Note: Indent is purposefully less than desired to keep reviewable differences to a minimum for PR** */
      {
        sx = BltInfo->SourcePoint.x;
        sy = BltInfo->SourcePoint.y;

        for (j=BltInfo->DestRect.top; j<BltInfo->DestRect.bottom; j++)
        {
          sx = BltInfo->SourcePoint.x;
          for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
          {
            DWORD pixel = DIB_24BPP_GetPixel(BltInfo->SourceSurface, sx, sy);
            DIB_24BPP_PutPixel(BltInfo->DestSurface, i, j, XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, pixel));
            sx++;
          }
          sy++;
        }
      }
        else
        {
          /* Buffering for source and destination flip overlaps. Fixes KHMZ MirrorTest CORE-16642 */
          BOOL TopToBottomDone = FALSE;

          if (bLeftToRight)
          {
            DPRINT("Flip is bLeftToRight.\n");
            DWORD  Index;

            /* Allocate enough pixels for a row in DWORD's */
            DWORD *store = ExAllocatePoolWithTag(NonPagedPool,
              (BltInfo->DestRect.right - BltInfo->DestRect.left + 1) * 4, TAG_DIB);
            if (store == NULL)
            {
              DPRINT1("Storage Allocation Failed.\n");
              return FALSE;
            }

            sx = BltInfo->SourcePoint.x;
            /* This sets sy to the top line */
            sy = BltInfo->SourcePoint.y;

            /* This sets sx to the rightmost pixel */
            sx = BltInfo->SourcePoint.x + (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);

            for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
            {

              /* This sets sx to the rightmost pixel */
              sx = BltInfo->SourcePoint.x + (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);

              Index = 0;

              // Read right to left and store
              for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
              {
                store[Index] = DIB_24BPP_GetPixel(BltInfo->SourceSurface, sx, sy);
                Index++;
                sx--;
              }

              Index = 0;

              // Write left to right to pixel
              for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
              {
                DIB_24BPP_PutPixel(BltInfo->DestSurface, i, j, XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, store[Index]));
                Index++;
              }
              sy++;
            }
            ExFreePoolWithTag(store, TAG_DIB);
            TopToBottomDone = TRUE;
          }

          if (bTopToBottom)
          {
            DPRINT("Flip is bTopToBottom.\n");
            DWORD  Index;

            /* Allocate enough pixels for a column in DWORD's */
            DWORD *store = ExAllocatePoolWithTag(NonPagedPool,
              (BltInfo->DestRect.bottom - BltInfo->DestRect.top + 1) * 4, TAG_DIB);
            if (store == NULL)
            {
              DPRINT1("Storage Allocation Failed.\n");
              return FALSE;
            }

            /* The TopToBottomDone flag indicates that we are flipping for bTopToBottom and bLeftToRight
             * and have already completed the bLeftToRight. So we will lose our first flip output
             * unless we work with its output which is at the destination site. So in this case
             * our new Source becomes the previous outputs Destination.
             */

            if (TopToBottomDone)
            {
             sx = BltInfo->DestRect.left;
              sy = BltInfo->DestRect.top;

              /* This sets sy to the bottom line */
              sy += (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1);
            }
            else
            {
              sx = BltInfo->SourcePoint.x;

              /* This sets sy to the top line */
              sy = BltInfo->SourcePoint.y;

              /* This sets sy to the bottom line */
              sy = BltInfo->SourcePoint.y + (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1);
            }

            for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
            {

              /* This sets sy to the bottom line */
              sy = BltInfo->SourcePoint.y + (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1);
              Index = 0;

              /* Read bottom to top and store */
              for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
              {
                if (TopToBottomDone)
                {
                  store[Index] = DIB_24BPP_GetPixel(BltInfo->DestSurface, sx, sy);
                }
                else
                {
                  store[Index] = DIB_24BPP_GetPixel(BltInfo->SourceSurface, sx, sy);
                }
                Index++;
                sy--;
              }

              Index = 0;

              for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
              {
                DIB_24BPP_PutPixel(BltInfo->DestSurface, i, j, XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, store[Index]));
                Index++;
              }
              sx++;
            }
            ExFreePoolWithTag(store, TAG_DIB);
          }

        }
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
          xColor = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *((PDWORD) SourceBits));
          *DestBits = xColor & 0xff;
          *(PWORD)(DestBits + 1) = (WORD)(xColor >> 8);
          DEC_OR_INC(SourceBits, bLeftToRight, 4);
          DestBits += 3;
        }

        DEC_OR_INC(SourceLine, bTopToBottom, BltInfo->SourceSurface->lDelta);
        DestLine += BltInfo->DestSurface->lDelta;
      }
      break;

    default:
      DbgPrint("DIB_24BPP_Bitblt: Unhandled Source BPP: %u\n", BitsPerFormat(BltInfo->SourceSurface->iBitmapFormat));
      return FALSE;
  }

  return TRUE;
}

BOOLEAN
DIB_24BPP_BitBlt(PBLTINFO BltInfo)
{
   LONG DestX, DestY;
   LONG SourceX, SourceY;
   LONG PatternY = 0;
   ULONG Dest, Source = 0, Pattern = 0;
   BOOL UsesSource;
   BOOL UsesPattern;
   PBYTE DestBits;

   UsesSource = ROP4_USES_SOURCE(BltInfo->Rop4);
   UsesPattern = ROP4_USES_PATTERN(BltInfo->Rop4);

   SourceY = BltInfo->SourcePoint.y;
   DestBits = (PBYTE)(
      (PBYTE)BltInfo->DestSurface->pvScan0 +
      (BltInfo->DestRect.left << 1) + BltInfo->DestRect.left +
      BltInfo->DestRect.top * BltInfo->DestSurface->lDelta);

   if (UsesPattern)
   {
      if (BltInfo->PatternSurface)
      {
         PatternY = (BltInfo->DestRect.top - BltInfo->BrushOrigin.y) %
                    BltInfo->PatternSurface->sizlBitmap.cy;
      }
      else
      {
         if (BltInfo->Brush)
         {
            Pattern = BltInfo->Brush->iSolidColor;
         }
      }
   }

   for (DestY = BltInfo->DestRect.top; DestY < BltInfo->DestRect.bottom; DestY++)
   {
      SourceX = BltInfo->SourcePoint.x;

      for (DestX = BltInfo->DestRect.left; DestX < BltInfo->DestRect.right; DestX++, DestBits += 3, SourceX++)
      {
         Dest = *((PUSHORT)DestBits) + (*(DestBits + 2) << 16);

         if (UsesSource)
         {
            Source = DIB_GetSource(BltInfo->SourceSurface, SourceX, SourceY, BltInfo->XlateSourceToDest);
         }

         if (BltInfo->PatternSurface)
         {
            Pattern = DIB_GetSourceIndex(BltInfo->PatternSurface, (DestX - BltInfo->BrushOrigin.x) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY);
         }

         Dest = DIB_DoRop(BltInfo->Rop4, Dest, Source, Pattern) & 0xFFFFFF;
         *(PUSHORT)(DestBits) = Dest & 0xFFFF;
         *(DestBits + 2) = (BYTE)(Dest >> 16);
      }

      SourceY++;
      if (BltInfo->PatternSurface)
      {
         PatternY++;
         PatternY %= BltInfo->PatternSurface->sizlBitmap.cy;
      }
      DestBits -= (BltInfo->DestRect.right - BltInfo->DestRect.left) * 3;
      DestBits += BltInfo->DestSurface->lDelta;
   }

   return TRUE;
}

/* BitBlt Optimize */
BOOLEAN
DIB_24BPP_ColorFill(SURFOBJ* DestSurface, RECTL* DestRect, ULONG color)
{
  LONG DestY;

  /* Make WellOrdered by making top < bottom and left < right */
  RECTL_vMakeWellOrdered(DestRect);

#if defined(_M_IX86) && !defined(_MSC_VER)
  PBYTE xaddr = (PBYTE)DestSurface->pvScan0 + DestRect->top * DestSurface->lDelta + (DestRect->left << 1) + DestRect->left;
  PBYTE addr;
  ULONG Count;
  ULONG xCount=DestRect->right - DestRect->left;

  for (DestY = DestRect->top; DestY< DestRect->bottom; DestY++)
  {
    Count = xCount;
    addr = xaddr;
    xaddr = (PBYTE)((ULONG_PTR)addr + DestSurface->lDelta);

    if (Count < 8)
    {
      /* For small fills, don't bother doing anything fancy */
      while (Count--)
        {
          *(PUSHORT)(addr) = color;
          addr += 2;
          *(addr) = color >> 16;
          addr += 1;
        }
    }
    else
    {
      /* Align to 4-byte address */
      while (0 != ((ULONG_PTR) addr & 0x3))
      {
        *(PUSHORT)(addr) = color;
        addr += 2;
        *(addr) = color >> 16;
        addr += 1;
        Count--;
      }
      /* If the color we need to fill with is 0ABC, then the final mem pattern
       * (note little-endianness) would be:
       *
       * |C.B.A|C.B.A|C.B.A|C.B.A|   <- pixel borders
       * |C.B.A.C|B.A.C.B|A.C.B.A|   <- ULONG borders
       *
       * So, taking endianness into account again, we need to fill with these
       * ULONGs: CABC BCAB ABCA */

      /* This is about 30% faster than the generic C code below */
      __asm__ __volatile__ (
        "movl %1, %%ecx\n\t"
        "andl $0xffffff, %%ecx\n\t"     /* 0ABC */
        "movl %%ecx, %%ebx\n\t"         /* Construct BCAB in ebx */
        "shrl $8, %%ebx\n\t"
        "movl %%ecx, %%eax\n\t"
        "shll $16, %%eax\n\t"
        "orl  %%eax, %%ebx\n\t"
        "movl %%ecx, %%edx\n\t"         /* Construct ABCA in edx */
        "shll $8, %%edx\n\t"
        "movl %%ecx, %%eax\n\t"
        "shrl $16, %%eax\n\t"
        "orl  %%eax, %%edx\n\t"
        "movl %%ecx, %%eax\n\t"         /* Construct CABC in eax */
        "shll $24, %%eax\n\t"
        "orl  %%ecx, %%eax\n\t"
        "movl %2, %%ecx\n\t"            /* Load count */
        "shr  $2, %%ecx\n\t"
        "movl %3, %%edi\n"              /* Load dest */
        "1:\n\t"
        "movl %%eax, (%%edi)\n\t"       /* Store 4 pixels, 12 bytes */
        "movl %%ebx, 4(%%edi)\n\t"
        "movl %%edx, 8(%%edi)\n\t"
        "addl $12, %%edi\n\t"
        "dec  %%ecx\n\t"
        "jnz  1b\n\t"
        "movl %%edi, %0"
        : "=m"(addr)
        : "m"(color), "m"(Count), "m"(addr)
        : "%eax", "%ebx", "%ecx", "%edx", "%edi");
      Count = Count & 0x03;
      while (0 != Count--)
      {
        *(PUSHORT)(addr) = color;
        addr += 2;
        *(addr) = color >> 16;
        addr += 1;
      }
    }
  }
#else

  for (DestY = DestRect->top; DestY< DestRect->bottom; DestY++)
  {
    DIB_24BPP_HLine(DestSurface, DestRect->left, DestRect->right, DestY, color);
  }
#endif
  return TRUE;
}

BOOLEAN
DIB_24BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                         RECTL*  DestRect,  RECTL *SourceRect,
                         XLATEOBJ *ColorTranslation, ULONG iTransColor)
{
  LONG X, Y, SourceX, SourceY = 0, wd;
  ULONG Source = 0, Dest;
  BYTE *DestBits;

  LONG DstHeight;
  LONG DstWidth;
  LONG SrcHeight;
  LONG SrcWidth;

  DstHeight = DestRect->bottom - DestRect->top;
  DstWidth = DestRect->right - DestRect->left;
  SrcHeight = SourceRect->bottom - SourceRect->top;
  SrcWidth = SourceRect->right - SourceRect->left;

  DestBits = (BYTE*)((PBYTE)DestSurf->pvScan0 +
                      (DestRect->left * 3) +
                      DestRect->top * DestSurf->lDelta);
  wd = DestSurf->lDelta - ((DestRect->right - DestRect->left) * 3);

  for(Y = DestRect->top; Y < DestRect->bottom; Y++)
  {
    SourceY = SourceRect->top+(Y - DestRect->top) * SrcHeight / DstHeight;
    for(X = DestRect->left; X < DestRect->right; X++, DestBits += 3)
    {
      SourceX = SourceRect->left+(X - DestRect->left) * SrcWidth / DstWidth;
      if (SourceX >= 0 && SourceY >= 0 &&
          SourceSurf->sizlBitmap.cx > SourceX && SourceSurf->sizlBitmap.cy > SourceY)
      {
        Source = DIB_GetSourceIndex(SourceSurf, SourceX, SourceY);
        if(Source != iTransColor)
        {
          Dest = XLATEOBJ_iXlate(ColorTranslation, Source) & 0xFFFFFF;
           *(PUSHORT)(DestBits) = Dest & 0xFFFF;
           *(DestBits + 2) = (BYTE)(Dest >> 16);
        }
      }
    }

    DestBits = (BYTE*)((ULONG_PTR)DestBits + wd);
  }

  return TRUE;
}

typedef union {
   ULONG ul;
   struct {
      UCHAR red;
      UCHAR green;
      UCHAR blue;
      UCHAR alpha;
   } col;
} NICEPIXEL32;

static __inline UCHAR
Clamp8(ULONG val)
{
   return (val > 255) ? 255 : (UCHAR)val;
}

BOOLEAN
DIB_24BPP_AlphaBlend(SURFOBJ* Dest, SURFOBJ* Source, RECTL* DestRect,
                     RECTL* SourceRect, CLIPOBJ* ClipRegion,
                     XLATEOBJ* ColorTranslation, BLENDOBJ* BlendObj)
{
   INT Rows, Cols, SrcX, SrcY;
   register PUCHAR Dst;
   BLENDFUNCTION BlendFunc;
   register NICEPIXEL32 DstPixel, SrcPixel;
   UCHAR Alpha;
   //UCHAR SrcBpp;

   DPRINT("DIB_24BPP_AlphaBlend: srcRect: (%d,%d)-(%d,%d), dstRect: (%d,%d)-(%d,%d)\n",
          SourceRect->left, SourceRect->top, SourceRect->right, SourceRect->bottom,
          DestRect->left, DestRect->top, DestRect->right, DestRect->bottom);

   BlendFunc = BlendObj->BlendFunction;
   if (BlendFunc.BlendOp != AC_SRC_OVER)
   {
      DPRINT1("BlendOp != AC_SRC_OVER\n");
      return FALSE;
   }
   if (BlendFunc.BlendFlags != 0)
   {
      DPRINT1("BlendFlags != 0\n");
      return FALSE;
   }
   if ((BlendFunc.AlphaFormat & ~AC_SRC_ALPHA) != 0)
   {
      DPRINT1("Unsupported AlphaFormat (0x%x)\n", BlendFunc.AlphaFormat);
      return FALSE;
   }
   if ((BlendFunc.AlphaFormat & AC_SRC_ALPHA) != 0 &&
       BitsPerFormat(Source->iBitmapFormat) != 32)
   {
      DPRINT1("Source bitmap must be 32bpp when AC_SRC_ALPHA is set\n");
      return FALSE;
   }

   Dst = (PUCHAR)((ULONG_PTR)Dest->pvScan0 + (DestRect->top * Dest->lDelta) +
                             (DestRect->left * 3));
   //SrcBpp = BitsPerFormat(Source->iBitmapFormat);

   Rows = 0;
   SrcY = SourceRect->top;
   while (++Rows <= DestRect->bottom - DestRect->top)
  {
    Cols = 0;
    SrcX = SourceRect->left;
    while (++Cols <= DestRect->right - DestRect->left)
    {
      SrcPixel.ul = DIB_GetSource(Source, SrcX, SrcY, ColorTranslation);
      SrcPixel.col.red = (SrcPixel.col.red * BlendFunc.SourceConstantAlpha) / 255;
      SrcPixel.col.green = (SrcPixel.col.green * BlendFunc.SourceConstantAlpha) / 255;
      SrcPixel.col.blue = (SrcPixel.col.blue * BlendFunc.SourceConstantAlpha) / 255;
      if (!(BlendFunc.AlphaFormat & AC_SRC_ALPHA))
      {
          Alpha = BlendFunc.SourceConstantAlpha ;
      }
      else
      {
        Alpha = (SrcPixel.col.alpha * BlendFunc.SourceConstantAlpha) / 255;
      }

      DstPixel.col.red = Clamp8((*Dst * (255 - Alpha)) / 255 + SrcPixel.col.red) ;
      DstPixel.col.green = Clamp8((*(Dst+1) * (255 - Alpha) / 255 + SrcPixel.col.green)) ;
      DstPixel.col.blue = Clamp8((*(Dst+2) * (255 - Alpha)) / 255 + SrcPixel.col.blue) ;
      *Dst++ = DstPixel.col.red;
      *Dst++ = DstPixel.col.green;
      *Dst++ = DstPixel.col.blue;
      SrcX = SourceRect->left + (Cols*(SourceRect->right - SourceRect->left))/(DestRect->right - DestRect->left);
    }
    Dst = (PUCHAR)((ULONG_PTR)Dest->pvScan0 + ((DestRect->top + Rows) * Dest->lDelta) +
                (DestRect->left*3));
    SrcY = SourceRect->top + (Rows*(SourceRect->bottom - SourceRect->top))/(DestRect->bottom - DestRect->top);
  }

   return TRUE;
}

/* EOF */
