/*
 * PROJECT:         Win32 subsystem
 * LICENSE:         See COPYING in the top level directory
 * FILE:            win32ss/gdi/dib/dib16bpp.c
 * PURPOSE:         Device Independant Bitmap functions, 16bpp
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
DIB_16BPP_PutPixel(SURFOBJ *SurfObj, LONG x, LONG y, ULONG c)
{
  PBYTE byteaddr = (PBYTE)SurfObj->pvScan0 + y * SurfObj->lDelta;
  PWORD addr = (PWORD)byteaddr + x;

  *addr = (WORD)c;
}

ULONG
DIB_16BPP_GetPixel(SURFOBJ *SurfObj, LONG x, LONG y)
{
  PBYTE byteaddr = (PBYTE)SurfObj->pvScan0 + y * SurfObj->lDelta;
  PWORD addr = (PWORD)byteaddr + x;
  return (ULONG)(*addr);
}

VOID
DIB_16BPP_HLine(SURFOBJ *SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  PDWORD addr = (PDWORD)((PWORD)((PBYTE)SurfObj->pvScan0 + y * SurfObj->lDelta) + x1);

#if defined(_M_IX86) && !defined(_MSC_VER)
  /* This is about 10% faster than the generic C code below */
  LONG Count = x2 - x1;

  __asm__ __volatile__ (
    "  cld\n"
    "  mov  %0, %%eax\n"
    "  shl  $16, %%eax\n"
    "  andl $0xffff, %0\n"  /* If the pixel value is "abcd", put "abcdabcd" in %eax */
    "  or   %0, %%eax\n"
    "  mov  %2, %%edi\n"
    "  test $0x03, %%edi\n" /* Align to fullword boundary */
    "  jz   0f\n"
    "  stosw\n"
    "  dec  %1\n"
    "  jz   1f\n"
    "0:\n"
    "  mov  %1,%%ecx\n"     /* Setup count of fullwords to fill */
    "  shr  $1,%%ecx\n"
    "  rep stosl\n"         /* The actual fill */
    "  test $0x01, %1\n"    /* One left to do at the right side? */
    "  jz   1f\n"
    "  stosw\n"
    "1:\n"
    : /* no output */
  : "r"(c), "r"(Count), "m"(addr)
    : "%eax", "%ecx", "%edi");
#else /* _M_IX86 */
  LONG cx = x1;
  DWORD cc;

  if (0 != (cx & 0x01))
  {
    *((PWORD) addr) = (WORD)c;
    cx++;
    addr = (PDWORD)((PWORD)(addr) + 1);
  }
  cc = ((c & 0xffff) << 16) | (c & 0xffff);
  while(cx + 1 < x2)
  {
    *addr++ = cc;
    cx += 2;
  }
  if (cx < x2)
  {
    *((PWORD) addr) = (WORD)c;
  }
#endif /* _M_IX86 */
}


VOID
DIB_16BPP_VLine(SURFOBJ *SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
#if defined(_M_IX86) && !defined(_MSC_VER)
  asm volatile(
    "   testl %2, %2"       "\n\t"
    "   jle   2f"           "\n\t"
    "   movl  %2, %%ecx"    "\n\t"
    "   shrl  $2, %2"       "\n\t"
    "   andl  $3, %%ecx"    "\n\t"
    "   jz    1f"           "\n\t"
    "0:"                    "\n\t"
    "   movw  %%ax, (%0)"   "\n\t"
    "   addl  %1, %0"       "\n\t"
    "   decl  %%ecx"        "\n\t"
    "   jnz   0b"           "\n\t"
    "   testl %2, %2"       "\n\t"
    "   jz    2f"           "\n\t"
    "1:"                    "\n\t"
    "   movw  %%ax, (%0)"   "\n\t"
    "   addl  %1, %0"       "\n\t"
    "   movw  %%ax, (%0)"   "\n\t"
    "   addl  %1, %0"       "\n\t"
    "   movw  %%ax, (%0)"   "\n\t"
    "   addl  %1, %0"       "\n\t"
    "   movw  %%ax, (%0)"   "\n\t"
    "   addl  %1, %0"       "\n\t"
    "   decl  %2"           "\n\t"
    "   jnz   1b"           "\n\t"
    "2:"                    "\n\t"
    : /* no output */
  : "r"((PBYTE)SurfObj->pvScan0 + (y1 * SurfObj->lDelta) + (x * sizeof (WORD))),
    "r"(SurfObj->lDelta), "r"(y2 - y1), "a"(c)
    : "cc", "memory", "%ecx");
#else
  PBYTE byteaddr = (PBYTE)(ULONG_PTR)SurfObj->pvScan0 + y1 * SurfObj->lDelta;
  PWORD addr = (PWORD)byteaddr + x;
  LONG lDelta = SurfObj->lDelta;

  byteaddr = (PBYTE)addr;
  while(y1++ < y2)
  {
    *addr = (WORD)c;

    byteaddr += lDelta;
    addr = (PWORD)byteaddr;
  }
#endif
}

BOOLEAN
DIB_16BPP_BitBltSrcCopy(PBLTINFO BltInfo)
{
  LONG     i, j, sx, sy, xColor, f1;
  PBYTE    SourceBits, DestBits, SourceLine, DestLine;
  PBYTE    SourceBits_4BPP, SourceLine_4BPP;
  PWORD    Source32, Dest32;
  DWORD    Index, StartLeft, EndRight;
  BOOLEAN  bTopToBottom, bLeftToRight;

  DPRINT("DIB_16BPP_BitBltSrcCopy: SrcSurf cx/cy (%d/%d), DestSuft cx/cy (%d/%d) dstRect: (%d,%d)-(%d,%d)\n",
         BltInfo->SourceSurface->sizlBitmap.cx, BltInfo->SourceSurface->sizlBitmap.cy,
         BltInfo->DestSurface->sizlBitmap.cx, BltInfo->DestSurface->sizlBitmap.cy,
         BltInfo->DestRect.left, BltInfo->DestRect.top, BltInfo->DestRect.right, BltInfo->DestRect.bottom);

  /* Get back left to right flip here */
  bLeftToRight = BltInfo->DestRect.left > BltInfo->DestRect.right;

  /* Check for top to bottom flip needed. */
  bTopToBottom = BltInfo->DestRect.top > BltInfo->DestRect.bottom;

  DPRINT("BltInfo->SourcePoint.x is '%d' & BltInfo->SourcePoint.y is '%d'.\n",
         BltInfo->SourcePoint.x, BltInfo->SourcePoint.y);

  /* Make WellOrdered with top < bottom and left < right */
  RECTL_vMakeWellOrdered(&BltInfo->DestRect);

  DPRINT("BPP is '%d/%d' & BltInfo->SourcePoint.x is '%d' & BltInfo->SourcePoint.y is '%d'.\n",
         BltInfo->SourceSurface->iBitmapFormat, BltInfo->SourcePoint.x, BltInfo->SourcePoint.y);

  DestBits = (PBYTE)BltInfo->DestSurface->pvScan0 + (BltInfo->DestRect.top * BltInfo->DestSurface->lDelta) + 2 * BltInfo->DestRect.left;

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
      sy += BltInfo->SourceSurface->lDelta * (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1);
    }

    for (j=BltInfo->DestRect.top; j<BltInfo->DestRect.bottom; j++)
    {
      sx = BltInfo->SourcePoint.x;

      if (bLeftToRight)
      {
        /* This sets the sx to the rightmost pixel */
        sx += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
        StartLeft = BltInfo->DestRect.left + 1;
        EndRight = BltInfo->DestRect.right + 1;
      }
      else
      {
        StartLeft = BltInfo->DestRect.left;
        EndRight = BltInfo->DestRect.right;
      }

      for (i = StartLeft; i < EndRight; i++)
      {
        if(DIB_1BPP_GetPixel(BltInfo->SourceSurface, sx, sy) == 0)
        {
          DIB_16BPP_PutPixel(BltInfo->DestSurface, i, j,
            XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, 0));
        }
        else
        {
          DIB_16BPP_PutPixel(BltInfo->DestSurface, i, j,
            XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, 1));
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
    SourceBits_4BPP = (PBYTE)BltInfo->SourceSurface->pvScan0 +
      (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) +
      (BltInfo->SourcePoint.x >> 1);

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
        DIB_16BPP_PutPixel(BltInfo->DestSurface, i, j, xColor);
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
    DPRINT("8BPP Case Selected with DestRect Width of '%d'.\n",
           BltInfo->DestRect.right - BltInfo->DestRect.left);

    /* This sets SourceLine to the top line */
    SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 +
      (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) +
      BltInfo->SourcePoint.x;
    DestLine = DestBits;

    if (bTopToBottom)
    {
      /* This sets SourceLine to the bottom line */
      SourceLine += BltInfo->SourceSurface->lDelta
        * (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1);
    }

    for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
    {
      SourceBits = SourceLine;
      DestBits = DestLine;

      if (bLeftToRight)
      {
        /* This sets SourceBits to the rightmost pixel */
        SourceBits += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
      }

      for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
      {
        *((WORD *)DestBits) = (WORD)XLATEOBJ_iXlate(
          BltInfo->XlateSourceToDest, *SourceBits);
        DEC_OR_INC(SourceBits, bLeftToRight, 1);
        DestBits += 2;
      }
      DEC_OR_INC(SourceLine, bTopToBottom, BltInfo->SourceSurface->lDelta);
      DestLine += BltInfo->DestSurface->lDelta;
    }
    break;

  case BMF_16BPP:
    DPRINT("16BPP Case Selected with DestRect Width of '%d'.\n",
           BltInfo->DestRect.right - BltInfo->DestRect.left);

    DPRINT("BMF_16BPP-dstRect: (%d,%d)-(%d,%d) and Width of '%d'.\n",
           BltInfo->DestRect.left, BltInfo->DestRect.top,
           BltInfo->DestRect.right, BltInfo->DestRect.bottom,
           BltInfo->DestRect.right - BltInfo->DestRect.left);

    if ((BltInfo->XlateSourceToDest == NULL ||
      (BltInfo->XlateSourceToDest->flXlate & XO_TRIVIAL) != 0) &&
      (!bTopToBottom && !bLeftToRight))
    {
      DPRINT("XO_TRIVIAL is TRUE.\n");
      if (BltInfo->DestRect.top < BltInfo->SourcePoint.y)
      {
        /* This sets SourceBits to the top line */
        SourceBits = (PBYTE)BltInfo->SourceSurface->pvScan0 +
          (BltInfo->SourcePoint.y *
          BltInfo->SourceSurface->lDelta) + 2 *
          BltInfo->SourcePoint.x;

        for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
        {
          RtlMoveMemory(DestBits, SourceBits,
            2 * (BltInfo->DestRect.right -
            BltInfo->DestRect.left));

          SourceBits += BltInfo->SourceSurface->lDelta;
          DestBits += BltInfo->DestSurface->lDelta;
        }
      }
      else
      {
        /* This sets SourceBits to the bottom line */
        SourceBits = (PBYTE)BltInfo->SourceSurface->pvScan0 +
          ((BltInfo->SourcePoint.y + BltInfo->DestRect.bottom -
          BltInfo->DestRect.top - 1) *
          BltInfo->SourceSurface->lDelta) + 2 *
          BltInfo->SourcePoint.x;

        DestBits = (PBYTE)BltInfo->DestSurface->pvScan0 +
          ((BltInfo->DestRect.bottom - 1) *
          BltInfo->DestSurface->lDelta) + 2 *
          BltInfo->DestRect.left;

        for (j = BltInfo->DestRect.bottom - 1;
          BltInfo->DestRect.top <= j; j--)
        {
          RtlMoveMemory(DestBits, SourceBits, 2 *
            (BltInfo->DestRect.right -
            BltInfo->DestRect.left));

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
      DPRINT("Flip is None.\n");
      if (BltInfo->DestRect.top < BltInfo->SourcePoint.y)
      {
        SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 +
          (BltInfo->SourcePoint.y *
          BltInfo->SourceSurface->lDelta) + 2 *
          BltInfo->SourcePoint.x;

        DestLine = DestBits;
        for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
        {
          SourceBits = SourceLine;
          DestBits = DestLine;
          for (i = BltInfo->DestRect.left; i <
            BltInfo->DestRect.right; i++)
          {
            *((WORD *)DestBits) = (WORD)XLATEOBJ_iXlate(
              BltInfo->XlateSourceToDest,
              *((WORD *)SourceBits));
            SourceBits += 2;
            DestBits += 2;
          }
          SourceLine += BltInfo->SourceSurface->lDelta;
          DestLine += BltInfo->DestSurface->lDelta;
        }
      }
      else
      {
        SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 +
          ((BltInfo->SourcePoint.y +
          BltInfo->DestRect.bottom -
          BltInfo->DestRect.top - 1) *
          BltInfo->SourceSurface->lDelta) + 2 *
          BltInfo->SourcePoint.x;

        DestLine = (PBYTE)BltInfo->DestSurface->pvScan0 +
          ((BltInfo->DestRect.bottom - 1) *
          BltInfo->DestSurface->lDelta) + 2 *
          BltInfo->DestRect.left;

        for (j = BltInfo->DestRect.bottom - 1;
          BltInfo->DestRect.top <= j; j--)
        {
          SourceBits = SourceLine;
          DestBits = DestLine;
          for (i = BltInfo->DestRect.left; i <
            BltInfo->DestRect.right; i++)
          {
            *((WORD *)DestBits) = (WORD)XLATEOBJ_iXlate(
              BltInfo->XlateSourceToDest,
              *((WORD *)SourceBits));
            SourceBits += 2;
            DestBits += 2;
          }
          SourceLine -= BltInfo->SourceSurface->lDelta;
          DestLine -= BltInfo->DestSurface->lDelta;
        }
      }
    }
      else
      {
        /* Buffering for source and destination flip overlaps. Fixes KHMZ MirrorTest CORE-16642 */
        BOOL TopToBottomDone = FALSE;

        if (bLeftToRight)
        {
          DPRINT("Flip is bLeftToRight.\n");

          /* Allocate enough pixels for a row in WORD's */
          WORD *store = ExAllocatePoolWithTag(NonPagedPool,
            (BltInfo->DestRect.right - BltInfo->DestRect.left + 1) * 2, TAG_DIB);
          if (store == NULL)
          {
            DPRINT1("Storage Allocation Failed.\n");
            return FALSE;
          }
          WORD  Index;

          /* This sets SourceBits to the bottom line */
          SourceBits = (PBYTE)BltInfo->SourceSurface->pvScan0
            + ((BltInfo->SourcePoint.y + BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1)
            * BltInfo->SourceSurface->lDelta) + 2 * BltInfo->SourcePoint.x;

          /* Sets DestBits to the bottom line */
          DestBits = (PBYTE)BltInfo->DestSurface->pvScan0
           + (BltInfo->DestRect.bottom - 1) * BltInfo->DestSurface->lDelta
           + 2 * BltInfo->DestRect.left + 2;

          for (j = BltInfo->DestRect.bottom - 1; BltInfo->DestRect.top <= j; j--)
          {
            /* Set Dest32 to right pixel */
            Dest32 = (WORD *) DestBits + (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
            Source32 = (WORD *) SourceBits;

            Index = 0;

            /* Store pixels from left to right */
            for (i = BltInfo->DestRect.right - 1; BltInfo->DestRect.left <= i; i--)
            {
              store[Index] = (WORD)XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *(WORD *)Source32++);
              Index++;
            }

            Index = 0;

            /* Copy stored data to pixels from right to left */
            for (i = BltInfo->DestRect.right - 1; BltInfo->DestRect.left <= i; i--)
            {
              *(WORD *)Dest32-- = store[Index];
              Index++;
            }

            SourceBits -= BltInfo->SourceSurface->lDelta;
            DestBits -= BltInfo->DestSurface->lDelta;
          }
          ExFreePoolWithTag(store, TAG_DIB);
          TopToBottomDone = TRUE;
        }

        if (bTopToBottom)
        {
          DPRINT("Flip is bTopToBottom.\n");

          /* Allocate enough pixels for a column in WORD's */
          WORD *store = ExAllocatePoolWithTag(NonPagedPool,
            (BltInfo->DestRect.bottom - BltInfo->DestRect.top + 1) * 2, TAG_DIB);
          if (store == NULL)
          {
            DPRINT1("Storage Allocation Failed.\n");
            return FALSE;
          }

          /* This set SourceBits to the top line */
          SourceBits = (PBYTE)BltInfo->SourceSurface->pvScan0
            + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta)
            + 2 * BltInfo->SourcePoint.x;

          /* This sets DestBits to the top line */
          DestBits = (PBYTE)BltInfo->DestSurface->pvScan0
           + ((BltInfo->DestRect.top) * BltInfo->DestSurface->lDelta)
           + 2 * BltInfo->DestRect.left;

          if ((BltInfo->SourceSurface->fjBitmap & BMF_TOPDOWN) == 0)
          {
            DestBits += BltInfo->DestSurface->lDelta;
          }

          if (bLeftToRight)
          {
            DPRINT("Adjusting DestBits for bLeftToRight.\n");
            DestBits += 2;
          }

          /* The TopToBottomDone flag indicates that we are flipping for bTopToBottom and bLeftToRight
           * and have already completed the bLeftToRight. So we will lose our first flip output
           * unless we work with its output which is at the destination site. So in this case
           * our new Source becomes the previous outputs Destination. */

          if (TopToBottomDone)
          {
            /* This sets SourceBits to the top line */
            SourceBits = DestBits;
          }

          for (j = BltInfo->DestRect.right - 1; BltInfo->DestRect.left <= j ; j--)
          {
            /* Set Dest32 to bottom pixel */
            Dest32 = (WORD *) DestBits  + (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1)
              * BltInfo->DestSurface->lDelta / 2;
            Source32 = (WORD *) SourceBits;

            Index = 0;

            /* Store pixels from top to bottom */
            for (i = BltInfo->DestRect.top; i <= BltInfo->DestRect.bottom - 1; i++)
            {
              store[Index] = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *Source32);
              Source32 += BltInfo->SourceSurface->lDelta / 2;
              Index++;
            }

            Index = 0;

            /* Copy stored data to pixels from bottom to top */
            for (i = BltInfo->DestRect.bottom - 1; BltInfo->DestRect.top <= i; i--)
            {
              *Dest32 = store[Index];
              Dest32 -= BltInfo->DestSurface->lDelta / 2;
              Index++;
            }
            SourceBits += 2;
            DestBits += 2;
          }
          ExFreePoolWithTag(store, TAG_DIB);
        }

      }
    }
    break;

  case BMF_24BPP:

    DPRINT("BMF_24BPP-dstRect: (%d,%d)-(%d,%d) and Width of '%d'.\n",
           BltInfo->DestRect.left, BltInfo->DestRect.top,
           BltInfo->DestRect.right, BltInfo->DestRect.bottom,
           BltInfo->DestRect.right - BltInfo->DestRect.left);

    /* This sets SourceLine to the top line */
    SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 +
      (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) +
      3 * BltInfo->SourcePoint.x;

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
        SourceBits += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1) * 3;
      }
      for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
      {
        xColor = (*(SourceBits + 2) << 0x10) +
          (*(SourceBits + 1) << 0x08) + (*(SourceBits));

        *((WORD *)DestBits) = (WORD)XLATEOBJ_iXlate(
          BltInfo->XlateSourceToDest, xColor);

        DEC_OR_INC(SourceBits, bLeftToRight, 3);
        DestBits += 2;
      }
      DEC_OR_INC(SourceLine, bTopToBottom, BltInfo->SourceSurface->lDelta);
      DestLine += BltInfo->DestSurface->lDelta;
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

      for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
      {
        *((WORD *)DestBits) = (WORD)XLATEOBJ_iXlate(
          BltInfo->XlateSourceToDest,
          *((PDWORD) SourceBits));
        DEC_OR_INC(SourceBits, bLeftToRight, 4);
        DestBits += 2;
      }

      DEC_OR_INC(SourceLine, bTopToBottom, BltInfo->SourceSurface->lDelta);
      DestLine += BltInfo->DestSurface->lDelta;
    }
    break;

  default:
    DPRINT1("DIB_16BPP_BitBltSrcCopy: Unhandled Source BPP: %u\n",
            BitsPerFormat(BltInfo->SourceSurface->iBitmapFormat));
    return FALSE;
  }

  return TRUE;
}

/* Optimize for bitBlt */
BOOLEAN
DIB_16BPP_ColorFill(SURFOBJ* DestSurface, RECTL* DestRect, ULONG color)
{
  LONG DestY;

  /* Make WellOrdered with top < bottom and left < right */
  RECTL_vMakeWellOrdered(DestRect);

#if defined(_M_IX86) && !defined(_MSC_VER)
  /* This is about 10% faster than the generic C code below */
  ULONG delta = DestSurface->lDelta;
  ULONG width = (DestRect->right - DestRect->left) ;
  PULONG pos =  (PULONG) ((PBYTE)DestSurface->pvScan0 + DestRect->top * delta + (DestRect->left<<1));
  color = (color&0xffff);  /* If the color value is "abcd", put "abcdabcd" into color */
  color += (color<<16);

  for (DestY = DestRect->top; DestY< DestRect->bottom; DestY++)
  {
    __asm__ __volatile__ (
      "cld\n\t"
      "mov  %1,%%ebx\n\t"
      "mov  %2,%%edi\n\t"
      "test $0x03, %%edi\n\t"   /* Align to fullword boundary */
      "jz   1f\n\t"
      "stosw\n\t"
      "dec  %%ebx\n\t"
      "jz   2f\n"
      "1:\n\t"
      "mov  %%ebx,%%ecx\n\t"    /* Setup count of fullwords to fill */
      "shr  $1,%%ecx\n\t"
      "rep stosl\n\t"           /* The actual fill */
      "test $0x01, %%ebx\n\t"   /* One left to do at the right side? */
      "jz   2f\n\t"
      "stosw\n"
      "2:"
      :
      : "a" (color), "r" (width), "m" (pos)
      : "%ecx", "%ebx", "%edi");
    pos =(PULONG)((ULONG_PTR)pos + delta);
  }
#else /* _M_IX86 */

  for (DestY = DestRect->top; DestY< DestRect->bottom; DestY++)
  {
    DIB_16BPP_HLine (DestSurface, DestRect->left, DestRect->right, DestY, color);
  }
#endif
  return TRUE;
}

BOOLEAN
DIB_16BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                         RECTL*  DestRect,  RECTL *SourceRect,
                         XLATEOBJ *ColorTranslation, ULONG iTransColor)
{
  LONG RoundedRight, X, Y, SourceX = 0, SourceY = 0, wd;
  ULONG *DestBits, Source, Dest;

  LONG DstHeight;
  LONG DstWidth;
  LONG SrcHeight;
  LONG SrcWidth;

  DstHeight = DestRect->bottom - DestRect->top;
  DstWidth = DestRect->right - DestRect->left;
  SrcHeight = SourceRect->bottom - SourceRect->top;
  SrcWidth = SourceRect->right - SourceRect->left;

  RoundedRight = DestRect->right - ((DestRect->right - DestRect->left) & 0x1);
  DestBits = (ULONG*)((PBYTE)DestSurf->pvScan0 +
    (DestRect->left << 1) +
    DestRect->top * DestSurf->lDelta);
  wd = DestSurf->lDelta - ((DestRect->right - DestRect->left) << 1);

  for(Y = DestRect->top; Y < DestRect->bottom; Y++)
  {
    SourceY = SourceRect->top+(Y - DestRect->top) * SrcHeight / DstHeight;
    for(X = DestRect->left; X < RoundedRight; X += 2, DestBits++, SourceX += 2)
    {
      Dest = *DestBits;

      SourceX = SourceRect->left+(X - DestRect->left) * SrcWidth / DstWidth;
      if (SourceX >= 0 && SourceY >= 0 &&
        SourceSurf->sizlBitmap.cx > SourceX && SourceSurf->sizlBitmap.cy > SourceY)
      {
        Source = DIB_GetSourceIndex(SourceSurf, SourceX, SourceY);
        if(Source != iTransColor)
        {
          Dest &= 0xFFFF0000;
          Dest |= (XLATEOBJ_iXlate(ColorTranslation, Source) & 0xFFFF);
        }
      }

      SourceX = SourceRect->left+(X+1 - DestRect->left) * SrcWidth / DstWidth;
      if (SourceX >= 0 && SourceY >= 0 &&
        SourceSurf->sizlBitmap.cx > SourceX && SourceSurf->sizlBitmap.cy > SourceY)
      {
        Source = DIB_GetSourceIndex(SourceSurf, SourceX, SourceY);
        if(Source != iTransColor)
        {
          Dest &= 0xFFFF;
          Dest |= (XLATEOBJ_iXlate(ColorTranslation, Source) << 16);
        }
      }

      *DestBits = Dest;
    }

    if(X < DestRect->right)
    {
      SourceX = SourceRect->left+(X - DestRect->left) * SrcWidth / DstWidth;
      if (SourceX >= 0 && SourceY >= 0 &&
        SourceSurf->sizlBitmap.cx > SourceX && SourceSurf->sizlBitmap.cy > SourceY)
      {
        Source = DIB_GetSourceIndex(SourceSurf, SourceX, SourceY);
        if(Source != iTransColor)
        {
          *((USHORT*)DestBits) = (USHORT)XLATEOBJ_iXlate(ColorTranslation,
            Source);
        }
      }

      DestBits = (PULONG)((ULONG_PTR)DestBits + 2);
    }

    DestBits = (ULONG*)((ULONG_PTR)DestBits + wd);
  }

  return TRUE;
}

typedef union
{
  ULONG ul;
  struct
  {
    UCHAR red;
    UCHAR green;
    UCHAR blue;
    UCHAR alpha;
  } col;
} NICEPIXEL32;

typedef union
{
  USHORT us;
  struct
  {
    USHORT blue  :5;
    USHORT green :6;
    USHORT red   :5;
  } col;
} NICEPIXEL16_565;

typedef union
{
  USHORT us;
  struct
  {
    USHORT blue  :5;
    USHORT green :5;
    USHORT red   :5;
    USHORT xxxx  :1;
  } col;
} NICEPIXEL16_555;

static __inline UCHAR
Clamp6(ULONG val)
{
   return (val > 63) ? 63 : (UCHAR)val;
}

static __inline UCHAR
Clamp5(ULONG val)
{
   return (val > 31) ? 31 : (UCHAR)val;
}

BOOLEAN
DIB_16BPP_AlphaBlend(SURFOBJ* Dest, SURFOBJ* Source, RECTL* DestRect,
                     RECTL* SourceRect, CLIPOBJ* ClipRegion,
                     XLATEOBJ* ColorTranslation, BLENDOBJ* BlendObj)
{
  INT DstX, DstY, SrcX, SrcY;
  BLENDFUNCTION BlendFunc;
  NICEPIXEL32 SrcPixel32;
  UCHAR Alpha;
  EXLATEOBJ* pexlo;
  EXLATEOBJ exloSrcRGB;

  DPRINT("DIB_16BPP_AlphaBlend: srcRect: (%d,%d)-(%d,%d), dstRect: (%d,%d)-(%d,%d)\n",
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
          (BitsPerFormat(Source->iBitmapFormat) != 32))
  {
    DPRINT1("Source bitmap must be 32bpp when AC_SRC_ALPHA is set\n");
    return FALSE;
  }

  if (!ColorTranslation)
  {
    DPRINT1("ColorTranslation must not be NULL!\n");
    return FALSE;
  }

  pexlo = CONTAINING_RECORD(ColorTranslation, EXLATEOBJ, xlo);
  EXLATEOBJ_vInitialize(&exloSrcRGB, pexlo->ppalSrc, &gpalRGB, 0, 0, 0);

  if (pexlo->ppalDst->flFlags & PAL_RGB16_555)
  {
      NICEPIXEL16_555 DstPixel16;

      SrcY = SourceRect->top;
      DstY = DestRect->top;
      while ( DstY < DestRect->bottom )
      {
        SrcX = SourceRect->left;
        DstX = DestRect->left;
        while(DstX < DestRect->right)
        {
          SrcPixel32.ul = DIB_GetSource(Source, SrcX, SrcY, &exloSrcRGB.xlo);
          SrcPixel32.col.red = (SrcPixel32.col.red * BlendFunc.SourceConstantAlpha) / 255;
          SrcPixel32.col.green = (SrcPixel32.col.green * BlendFunc.SourceConstantAlpha) / 255;
          SrcPixel32.col.blue = (SrcPixel32.col.blue * BlendFunc.SourceConstantAlpha) / 255;

          Alpha = ((BlendFunc.AlphaFormat & AC_SRC_ALPHA) != 0) ?
               (SrcPixel32.col.alpha * BlendFunc.SourceConstantAlpha) / 255 :
               BlendFunc.SourceConstantAlpha;

          Alpha >>= 3;

          DstPixel16.us = DIB_16BPP_GetPixel(Dest, DstX, DstY) & 0xFFFF;
          /* Perform bit loss */
          SrcPixel32.col.red >>= 3;
          SrcPixel32.col.green >>= 3;
          SrcPixel32.col.blue >>= 3;

          /* Do the blend in the right bit depth */
          DstPixel16.col.red = Clamp5((DstPixel16.col.red * (31 - Alpha)) / 31 + SrcPixel32.col.red);
          DstPixel16.col.green = Clamp5((DstPixel16.col.green * (31 - Alpha)) / 31 + SrcPixel32.col.green);
          DstPixel16.col.blue = Clamp5((DstPixel16.col.blue * (31 - Alpha)) / 31 + SrcPixel32.col.blue);

          DIB_16BPP_PutPixel(Dest, DstX, DstY, DstPixel16.us);

          DstX++;
          SrcX = SourceRect->left + ((DstX-DestRect->left)*(SourceRect->right - SourceRect->left))
                                                /(DestRect->right-DestRect->left);
        }
        DstY++;
        SrcY = SourceRect->top + ((DstY-DestRect->top)*(SourceRect->bottom - SourceRect->top))
                                                /(DestRect->bottom-DestRect->top);
      }
  }
  else
  {
      NICEPIXEL16_565 DstPixel16;
      UCHAR Alpha6, Alpha5;

      SrcY = SourceRect->top;
      DstY = DestRect->top;
      while ( DstY < DestRect->bottom )
      {
        SrcX = SourceRect->left;
        DstX = DestRect->left;
        while(DstX < DestRect->right)
        {
          SrcPixel32.ul = DIB_GetSource(Source, SrcX, SrcY, &exloSrcRGB.xlo);
          SrcPixel32.col.red = (SrcPixel32.col.red * BlendFunc.SourceConstantAlpha) / 255;
          SrcPixel32.col.green = (SrcPixel32.col.green * BlendFunc.SourceConstantAlpha) / 255;
          SrcPixel32.col.blue = (SrcPixel32.col.blue * BlendFunc.SourceConstantAlpha) / 255;

          Alpha = ((BlendFunc.AlphaFormat & AC_SRC_ALPHA) != 0) ?
               (SrcPixel32.col.alpha * BlendFunc.SourceConstantAlpha) / 255 :
               BlendFunc.SourceConstantAlpha;

          Alpha6 = Alpha >> 2;
          Alpha5 = Alpha >> 3;

          DstPixel16.us = DIB_16BPP_GetPixel(Dest, DstX, DstY) & 0xFFFF;
          /* Perform bit loss */
          SrcPixel32.col.red >>= 3;
          SrcPixel32.col.green >>= 2;
          SrcPixel32.col.blue >>= 3;

          /* Do the blend in the right bit depth */
          DstPixel16.col.red = Clamp5((DstPixel16.col.red * (31 - Alpha5)) / 31 + SrcPixel32.col.red);
          DstPixel16.col.green = Clamp6((DstPixel16.col.green * (63 - Alpha6)) / 63 + SrcPixel32.col.green);
          DstPixel16.col.blue = Clamp5((DstPixel16.col.blue * (31 - Alpha5)) / 31 + SrcPixel32.col.blue);

          DIB_16BPP_PutPixel(Dest, DstX, DstY, DstPixel16.us);

          DstX++;
          SrcX = SourceRect->left + ((DstX-DestRect->left)*(SourceRect->right - SourceRect->left))
                                                /(DestRect->right-DestRect->left);
        }
        DstY++;
        SrcY = SourceRect->top + ((DstY-DestRect->top)*(SourceRect->bottom - SourceRect->top))
                                                /(DestRect->bottom-DestRect->top);
      }
  }

  EXLATEOBJ_vCleanup(&exloSrcRGB);

  return TRUE;
}

/* EOF */
