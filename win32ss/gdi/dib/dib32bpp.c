/*
 * PROJECT:         Win32 subsystem
 * LICENSE:         See COPYING in the top level directory
 * FILE:            win32ss/gdi/dib/dib32bpp.c
 * PURPOSE:         Device Independant Bitmap functions, 32bpp
 * PROGRAMMERS:     Jason Filby
 *                  Thomas Bluemel
 *                  Gregor Anich
 *                  Doug Lyons
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

VOID
DIB_32BPP_PutPixel(SURFOBJ *SurfObj, LONG x, LONG y, ULONG c)
{
  PBYTE byteaddr = (PBYTE)SurfObj->pvScan0 + y * SurfObj->lDelta;
  PDWORD addr = (PDWORD)byteaddr + x;

  *addr = c;
}

ULONG
DIB_32BPP_GetPixel(SURFOBJ *SurfObj, LONG x, LONG y)
{
  PBYTE byteaddr = (PBYTE)SurfObj->pvScan0 + y * SurfObj->lDelta;
  PDWORD addr = (PDWORD)byteaddr + x;

  return (ULONG)(*addr);
}

VOID
DIB_32BPP_VLine(SURFOBJ *SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  PBYTE byteaddr = (PBYTE)SurfObj->pvScan0 + y1 * SurfObj->lDelta;
  PDWORD addr = (PDWORD)byteaddr + x;
  LONG lDelta = SurfObj->lDelta >> 2; // >> 2 == / sizeof(DWORD)

  byteaddr = (PBYTE)addr;
  while (y1++ < y2)
  {
    *addr = (DWORD)c;
    addr += lDelta;
  }
}

BOOLEAN
DIB_32BPP_BitBltSrcCopy(PBLTINFO BltInfo)
{
  LONG     i, j, sx, sy, xColor, f1, flip, lTmp;
  PBYTE    SourceBits, DestBits, SourceLine, DestLine;
  PBYTE    SourceBitsT, SourceBitsB, DestBitsT, DestBitsB;
  PBYTE    SourceBits_4BPP, SourceLine_4BPP;
  PDWORD   Source32, Dest32;
  DWORD    Index;

  DPRINT("DIB_32BPP_BitBltSrcCopy: SrcPt (%d, %d), SrcSurf cx/cy (%d/%d), DestSuft cx/cy (%d/%d) dstRect: (%d,%d)-(%d,%d)\n",
    BltInfo->SourcePoint.x, BltInfo->SourcePoint.y,
    BltInfo->SourceSurface->sizlBitmap.cx, BltInfo->SourceSurface->sizlBitmap.cy,
    BltInfo->DestSurface->sizlBitmap.cx, BltInfo->DestSurface->sizlBitmap.cy,
    BltInfo->DestRect.left, BltInfo->DestRect.top, BltInfo->DestRect.right, BltInfo->DestRect.bottom);

  /* Get back flip here */
  if ((BltInfo->DestRect.left > BltInfo->DestRect.right) && (BltInfo->DestRect.top > BltInfo->DestRect.bottom))
  {
    flip = 3;
  }
  else if (BltInfo->DestRect.top > BltInfo->DestRect.bottom)
  {
    flip = 2;
  }
  else if (BltInfo->DestRect.left > BltInfo->DestRect.right)
  {
    flip = 1;
  }
  else
  {
    flip = 0;
  }

  DPRINT("Flip is '%d'.\n", flip);

  /* If we came from copybits.c with a Top-Down SourceSurface bit set, */
  /* then we need a flip of 2. This mostly fixes Lazarus and PeaZip.   */
  if ((BltInfo->SourceSurface->fjBitmap & BMF_UMPDMEM) && (flip == 0))
  {
    flip = 2;
  }

  DPRINT("flip is '%d' & BltInfo->SourcePoint.x is '%d' & BltInfo->SourcePoint.y is '%d'.\n",
    flip, BltInfo->SourcePoint.x, BltInfo->SourcePoint.y);

  /* Make WellOrdered with top < bottom and left < right */
  if (BltInfo->DestRect.left > BltInfo->DestRect.right)
  {
    lTmp = BltInfo->DestRect.left;
    BltInfo->DestRect.left = BltInfo->DestRect.right;
    BltInfo->DestRect.right = lTmp;
  }
  if (BltInfo->DestRect.top > BltInfo->DestRect.bottom)
  {
    lTmp = BltInfo->DestRect.top;
    BltInfo->DestRect.top = BltInfo->DestRect.bottom;
    BltInfo->DestRect.bottom = lTmp;
  }

  DestBits = (PBYTE)BltInfo->DestSurface->pvScan0
    + (BltInfo->DestRect.top * BltInfo->DestSurface->lDelta)
    + 4 * BltInfo->DestRect.left;

  DPRINT("iBitmapFormat is %d and width,height is (%d,%d).\n", BltInfo->SourceSurface->iBitmapFormat,
    BltInfo->DestRect.right - BltInfo->DestRect.left, BltInfo->DestRect.bottom - BltInfo->DestRect.top);

  DPRINT("Being Drawn at point '(%d,%d)-(%d,%d)'.\n",
    BltInfo->DestRect.left, BltInfo->DestRect.top, BltInfo->DestRect.right, BltInfo->DestRect.bottom);

  switch (BltInfo->SourceSurface->iBitmapFormat)
  {
  case BMF_1BPP:
    DPRINT("1BPP Case Selected with DestRect Width of '%d' and flip is '%d'.\n",
           BltInfo->DestRect.right - BltInfo->DestRect.left, flip);

    sx = BltInfo->SourcePoint.x;

    /* This sets sy to the top line */
    sy = BltInfo->SourcePoint.y;

    if ((flip == 2) || (flip ==3))
    {
      /* This sets sy to the bottom line */
      sy += BltInfo->SourceSurface->lDelta * (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1);
    }

    for (j=BltInfo->DestRect.top; j<BltInfo->DestRect.bottom; j++)
    {
      sx = BltInfo->SourcePoint.x;

      if ((flip == 1) || (flip == 3))
      {
        /* This sets the sx to the rightmost pixel */
        sx += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
      }

      for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
      {
        if (DIB_1BPP_GetPixel(BltInfo->SourceSurface, sx, sy) == 0)
        {
          DIB_32BPP_PutPixel(BltInfo->DestSurface, i, j, XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, 0));
        } else {
          DIB_32BPP_PutPixel(BltInfo->DestSurface, i, j, XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, 1));
        }

        if ((flip == 1) || (flip == 3))
        {
          sx--;
        }
        else
        {
          sx++;
        }
      }
      if ((flip == 2) || (flip == 3))
      {
        sy--;
      }
      else
      {
        sy++;
      }
    }
    break;

  case BMF_4BPP:
    DPRINT("4BPP Case Selected with DestRect Width of '%d' and flip is '%d'.\n",
           BltInfo->DestRect.right - BltInfo->DestRect.left, flip);

    /* This sets SourceBits_4BPP to the top line */
    SourceBits_4BPP = (PBYTE)BltInfo->SourceSurface->pvScan0
      + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta)
      + (BltInfo->SourcePoint.x >> 1);

    if ((flip == 2) || (flip ==3))
    {
      /* This sets SourceBits_4BPP to the bottom line */
      SourceBits_4BPP += BltInfo->SourceSurface->lDelta * (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1);
    }

    for (j=BltInfo->DestRect.top; j<BltInfo->DestRect.bottom; j++)
    {
      SourceLine_4BPP = SourceBits_4BPP;
      sx = BltInfo->SourcePoint.x;

      if ((flip == 1) || (flip == 3))
      {
        /* This sets sx to the rightmost pixel */
        sx += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
      }

      f1 = sx & 1;

      for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
      {
        xColor = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest,
          (*SourceLine_4BPP & altnotmask[f1]) >> (4 * (1 - f1)));
        DIB_32BPP_PutPixel(BltInfo->DestSurface, i, j, xColor);
        if (f1 == 1) {
          if ((flip == 1) || (flip == 3))
          {
            SourceLine_4BPP--;
          }
          else
          {
            SourceLine_4BPP++;
          }
          f1 = 0;
        } else {
          f1 = 1;
        }
        if ((flip == 1) || (flip == 3))
        {
          sx--;
        }
        else
        {
          sx++;
        }
      }
      if ((flip == 2) || (flip == 3))
      {
        SourceBits_4BPP -= BltInfo->SourceSurface->lDelta;
      }
      else
      {
        SourceBits_4BPP += BltInfo->SourceSurface->lDelta;
      }
    }
    break;

  case BMF_8BPP:
    DPRINT("8BPP Case Selected with DestRect Width of '%d' and flip is '%d'.\n",
           BltInfo->DestRect.right - BltInfo->DestRect.left, flip);

    /* This sets SourceLine to the top line */
    SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + BltInfo->SourcePoint.x;
    DestLine = DestBits;

    if ((flip == 2) || (flip ==3))
    {
      /* This sets SourceLine to the bottom line */
      SourceLine += BltInfo->SourceSurface->lDelta * (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1);
    }

    for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
    {
      SourceBits = SourceLine;
      DestBits = DestLine;

      if ((flip == 1) || (flip == 3))
      {
        /* This sets the SourceBits to the rightmost pixel */
        SourceBits += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
      }

      for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
      {
        xColor = *SourceBits;
        *((PDWORD) DestBits) = (DWORD)XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, xColor);
        if ((flip == 1) || (flip == 3))
        {
          SourceBits--;
        }
        else
        {
          SourceBits++;
        }
        DestBits += 4;
      }
      if ((flip == 2) || (flip == 3))
      {
        SourceLine -= BltInfo->SourceSurface->lDelta;
      }
      else
      {
        SourceLine += BltInfo->SourceSurface->lDelta;
      }
      DestLine += BltInfo->DestSurface->lDelta;
    }
    break;

  case BMF_16BPP:
    DPRINT("16BPP Case Selected with DestRect Width of '%d' and flip is '%d'.\n",
            BltInfo->DestRect.right - BltInfo->DestRect.left, flip);

    /* This sets SourceLine to the top line */
    SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + 2 * BltInfo->SourcePoint.x;
    DestLine = DestBits;

    if ((flip == 2) || (flip ==3))
    {
      /* This sets SourceLine to the bottom line */
      SourceLine += BltInfo->SourceSurface->lDelta * (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1);
    }

    for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
    {
      SourceBits = SourceLine;
      DestBits = DestLine;

      if ((flip == 1) || (flip == 3))
      {
        /* This sets the SourceBits to the rightmost pixel */
        SourceBits += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1) * 2;
      }

      for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
      {
        xColor = *((PWORD) SourceBits);
        *((PDWORD) DestBits) = (DWORD)XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, xColor);
        if ((flip == 1) || (flip == 3))
        {
          SourceBits -= 2;
        }
        else
        {
          SourceBits += 2;
        }
        DestBits += 4;
      }

      if ((flip == 2) || (flip == 3))
      {
        SourceLine -= BltInfo->SourceSurface->lDelta;
      }
      else
      {
        SourceLine += BltInfo->SourceSurface->lDelta;
      }
      DestLine += BltInfo->DestSurface->lDelta;
    }
    break;

  case BMF_24BPP:
    DPRINT("24BPP Case Selected with DestRect Width of '%d' and flip is '%d'.\n",
      BltInfo->DestRect.right - BltInfo->DestRect.left, flip);

    /* This sets SourceLine to the top line */
    SourceLine = (PBYTE)BltInfo->SourceSurface->pvScan0
      + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta)
      + 3 * BltInfo->SourcePoint.x;

    if ((flip == 2) || (flip ==3))
    {
      /* This sets SourceLine to the bottom line */
      SourceLine += BltInfo->SourceSurface->lDelta * (BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1);
    }

    DestLine = DestBits;

    for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
    {
      SourceBits = SourceLine;
      DestBits = DestLine;

      if ((flip == 1) || (flip == 3))
      {
        /* This sets the SourceBits to the rightmost pixel */
        SourceBits += (BltInfo->DestRect.right - BltInfo->DestRect.left - 1) * 3;
      }

      for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
      {
        xColor = (*(SourceBits + 2) << 0x10) +
          (*(SourceBits + 1) << 0x08) +
          (*(SourceBits));
        *((PDWORD)DestBits) = (DWORD)XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, xColor);
        if ((flip == 1) || (flip == 3))
        {
          SourceBits -= 3;
        }
        else
        {
          SourceBits += 3;
        }
        DestBits += 4;
      }

      if ((flip == 2) || (flip ==3))
      {
        SourceLine -= BltInfo->SourceSurface->lDelta;
      }
      else
      {
        SourceLine += BltInfo->SourceSurface->lDelta;
      }
      DestLine += BltInfo->DestSurface->lDelta;
    }
    break;

  case BMF_32BPP:
    DPRINT("32BPP Case Selected with SrcPt (%d,%d) and DestRect Width/height of '%d/%d' and flip of '%d'.\n",
      BltInfo->SourcePoint.x, BltInfo->SourcePoint.y,
      BltInfo->DestRect.right - BltInfo->DestRect.left,
      BltInfo->DestRect.bottom - BltInfo->DestRect.top, flip);

    /* This tests for whether we can use simplified/quicker code below which uses RtlMoveMemory.
     * It works for increasing source and destination areas only where there is no full overlap and no flip.
     */
    if ((NULL == BltInfo->XlateSourceToDest || 0 != (BltInfo->XlateSourceToDest->flXlate & XO_TRIVIAL))
      && (flip == 0))
    {
      DPRINT("XO_TRIVIAL is TRUE.\n");
      if (BltInfo->DestRect.top < BltInfo->SourcePoint.y)
      {
        SourceBits = (PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + 4 * BltInfo->SourcePoint.x;
        for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
        {
          RtlMoveMemory(DestBits, SourceBits, 4 * (BltInfo->DestRect.right - BltInfo->DestRect.left));
          SourceBits += BltInfo->SourceSurface->lDelta;
          DestBits += BltInfo->DestSurface->lDelta;
        }
      }
      else
      {
        SourceBits = (PBYTE)BltInfo->SourceSurface->pvScan0
          + ((BltInfo->SourcePoint.y
          + BltInfo->DestRect.bottom
          - BltInfo->DestRect.top - 1) * BltInfo->SourceSurface->lDelta)
          + 4 * BltInfo->SourcePoint.x;
        DestBits = (PBYTE)BltInfo->DestSurface->pvScan0 + ((BltInfo->DestRect.bottom - 1) * BltInfo->DestSurface->lDelta) + 4 * BltInfo->DestRect.left;
        for (j = BltInfo->DestRect.bottom - 1; BltInfo->DestRect.top <= j; j--)
        {
          RtlMoveMemory(DestBits, SourceBits, 4 * (BltInfo->DestRect.right - BltInfo->DestRect.left));
          SourceBits -= BltInfo->SourceSurface->lDelta;
          DestBits -= BltInfo->DestSurface->lDelta;
        }
      }
    }
    else
    {
      DPRINT("XO_TRIVIAL is NOT TRUE.\n");
      if (flip == 0)
      /* **Note: Indent is purposefully less than desired to keep reviewable differences to a minimum for PR** */
      {
      if (BltInfo->DestRect.top < BltInfo->SourcePoint.y)
      {
        SourceBits = ((PBYTE)BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + 4 * BltInfo->SourcePoint.x);
        for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
        {
          if (BltInfo->DestRect.left < BltInfo->SourcePoint.x)
          {
            Dest32 = (DWORD *) DestBits;
            Source32 = (DWORD *) SourceBits;
            for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
            {
              *Dest32++ = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *Source32++);
            }
          }
          else
          {
            Dest32 = (DWORD *) DestBits + (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
            Source32 = (DWORD *) SourceBits + (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
            for (i = BltInfo->DestRect.right - 1; BltInfo->DestRect.left <= i; i--)
            {
              *Dest32-- = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *Source32--);
            }
          }
          SourceBits += BltInfo->SourceSurface->lDelta;
          DestBits += BltInfo->DestSurface->lDelta;
        }
      }
      else
      {
        SourceBits = (PBYTE)BltInfo->SourceSurface->pvScan0 + ((BltInfo->SourcePoint.y + BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) * BltInfo->SourceSurface->lDelta) + 4 * BltInfo->SourcePoint.x;
        DestBits = (PBYTE)BltInfo->DestSurface->pvScan0 + ((BltInfo->DestRect.bottom - 1) * BltInfo->DestSurface->lDelta) + 4 * BltInfo->DestRect.left;
        for (j = BltInfo->DestRect.bottom - 1; BltInfo->DestRect.top <= j; j--)
        {
          if (BltInfo->DestRect.left < BltInfo->SourcePoint.x)
          {
            Dest32 = (DWORD *) DestBits;
            Source32 = (DWORD *) SourceBits;
            for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
            {
              *Dest32++ = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *Source32++);
            }
          }
          else
          {
            Dest32 = (DWORD *) DestBits + (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
            Source32 = (DWORD *) SourceBits + (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
            for (i = BltInfo->DestRect.right - 1; BltInfo->DestRect.left <= i; i--)
            {
              *Dest32-- = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *Source32--);
            }
          }
          SourceBits -= BltInfo->SourceSurface->lDelta;
          DestBits -= BltInfo->DestSurface->lDelta;
        }
      }
    }
    else
      {
        /* Buffering for source and destination flip overlaps. Fixes KHMZ MirrorTest CORE-16642 */
        BOOL OneDone = FALSE;

        if ((flip == 1) || (flip == 3))
        {
          DPRINT("Flip == 1 or 3.\n");

          /* Allocate enough pixels for a row in DWORD's */
          DWORD *store = ExAllocatePoolWithTag(NonPagedPool,
            (BltInfo->DestRect.right - BltInfo->DestRect.left + 1) * 4, TAG_DIB);
          if (store == NULL)
          {
            return FALSE;
          }

          /* This sets SourceBits to the bottom line */
          SourceBits = (PBYTE)BltInfo->SourceSurface->pvScan0
            + ((BltInfo->SourcePoint.y + BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1)
            * BltInfo->SourceSurface->lDelta) + 4 * BltInfo->SourcePoint.x;

          /* Set DestBits to bottom line */
          DestBits = (PBYTE)BltInfo->DestSurface->pvScan0
           + (BltInfo->DestRect.bottom - 1) * BltInfo->DestSurface->lDelta
           + 4 * BltInfo->DestRect.left;

          for (j = BltInfo->DestRect.bottom - 1; BltInfo->DestRect.top <= j; j--)
          {

              /* Set Dest32 to right pixel */
              Dest32 = (DWORD *) DestBits + (BltInfo->DestRect.right - BltInfo->DestRect.left - 1);
              Source32 = (DWORD *) SourceBits;

              Index = 0;

              /* Store pixels from left to right */
              for (i = BltInfo->DestRect.right - 1; BltInfo->DestRect.left <= i; i--)
              {
                store[Index] = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *Source32++);
                Index++;
              }

              Index = 0;

              /* Copy stored dat to pixels from right to left */
              for (i = BltInfo->DestRect.right - 1; BltInfo->DestRect.left <= i; i--)
              {
                *Dest32-- = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, store[Index]);
                Index++;
              }
            SourceBits -= BltInfo->SourceSurface->lDelta;
            DestBits -= BltInfo->DestSurface->lDelta;
          }
          ExFreePoolWithTag(store, TAG_DIB);
          OneDone = TRUE;
        }

        if ((flip == 2) || (flip == 3))
        {

          /* Note: It is very important that this code remain optimized for time used. */
          /*   Otherwise you will have random crashes in ReactOS that are undesirable. */
          /*   For an example of this just try executing the code here two times.      */

          DPRINT("Flip == 2 or 3.\n");

          /* Allocate enough pixels for a row in DWORD's */
          DWORD *store = ExAllocatePoolWithTag(NonPagedPool,
            (BltInfo->DestRect.right - BltInfo->DestRect.left + 1) * 4, TAG_DIB);
          if (store == NULL)
          {
            return FALSE;
          }

          /* This set DestBitsT to the top line */
          DestBitsT = (PBYTE)BltInfo->DestSurface->pvScan0
            + (BltInfo->DestRect.top * BltInfo->DestSurface->lDelta)
            + 4 * BltInfo->DestRect.left;

          /* This sets DestBitsB to the bottom line */
          DestBitsB = (PBYTE)BltInfo->DestSurface->pvScan0
           + (BltInfo->DestRect.bottom - 1) * BltInfo->DestSurface->lDelta
           + 4 * BltInfo->DestRect.left;

          /* The OneDone flag indicates that we are doing a flip == 3 and have already */
          /* completed the flip == 1. So we will lose our first flip output unless     */
          /* we work with its output which is at the destination site. So in this case */
          /* our new Source becomes the previous outputs Destination. */

          if (OneDone)
          {
            /* This sets SourceBitsB to the bottom line */
            SourceBitsB = DestBitsB;

            /* This sets SourceBitsT to the top line */
            SourceBitsT = DestBitsT;
          }
          else
          {
            /* This sets SourceBitsB to the bottom line */
            SourceBitsB = (PBYTE)BltInfo->SourceSurface->pvScan0
              + ((BltInfo->SourcePoint.y + BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1)
              * BltInfo->SourceSurface->lDelta) + 4 * BltInfo->SourcePoint.x;

            /* This sets SourceBitsT to the top line */
            SourceBitsT = (PBYTE)BltInfo->SourceSurface->pvScan0
              + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + 4 * BltInfo->SourcePoint.x;
          }

          for (j = 0; j < (BltInfo->DestRect.bottom - BltInfo->DestRect.top) / 2 ; j++)
          {
            /* Store bottom row */
            RtlMoveMemory(&store[0], SourceBitsB, 4 * (BltInfo->DestRect.right - BltInfo->DestRect.left));

            /* Copy top row to bottom row overwriting it */
            RtlMoveMemory(DestBitsB, SourceBitsT, 4 * (BltInfo->DestRect.right - BltInfo->DestRect.left));

            /* Copy stored bottom row to top row */
            RtlMoveMemory(DestBitsT, &store[0], 4 * (BltInfo->DestRect.right - BltInfo->DestRect.left));

            /* Index top rows down and bottom rows up */
            SourceBitsT += BltInfo->SourceSurface->lDelta;
            SourceBitsB -= BltInfo->SourceSurface->lDelta;

            DestBitsT += BltInfo->DestSurface->lDelta;
            DestBitsB -= BltInfo->DestSurface->lDelta;
          }
          if ((BltInfo->DestRect.bottom - BltInfo->DestRect.top) % 2)
          {
            /* If we had an odd number of lines we handle the center one here */
            RtlMoveMemory(DestBitsB, SourceBitsT, 4 * (BltInfo->DestRect.right - BltInfo->DestRect.left));
          }
          ExFreePoolWithTag(store, TAG_DIB);
        }

      }
    }
    break;

  default:
    DPRINT1("DIB_32BPP_BitBltSrcCopy: Unhandled Source BPP: %u\n", BitsPerFormat(BltInfo->SourceSurface->iBitmapFormat));
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
DIB_32BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                         RECTL*  DestRect,  RECTL *SourceRect,
                         XLATEOBJ *ColorTranslation, ULONG iTransColor)
{
  LONG X, Y, SourceX, SourceY = 0, wd;
  ULONG *DestBits, Source = 0;

  LONG DstHeight;
  LONG DstWidth;
  LONG SrcHeight;
  LONG SrcWidth;

  DstHeight = DestRect->bottom - DestRect->top;
  DstWidth = DestRect->right - DestRect->left;
  SrcHeight = SourceRect->bottom - SourceRect->top;
  SrcWidth = SourceRect->right - SourceRect->left;

  DestBits = (ULONG*)((PBYTE)DestSurf->pvScan0 +
    (DestRect->left << 2) +
    DestRect->top * DestSurf->lDelta);
  wd = DestSurf->lDelta - ((DestRect->right - DestRect->left) << 2);

  for (Y = DestRect->top; Y < DestRect->bottom; Y++)
  {
    SourceY = SourceRect->top+(Y - DestRect->top) * SrcHeight / DstHeight;
    for (X = DestRect->left; X < DestRect->right; X++, DestBits++)
    {
      SourceX = SourceRect->left+(X - DestRect->left) * SrcWidth / DstWidth;
      if (SourceX >= 0 && SourceY >= 0 &&
        SourceSurf->sizlBitmap.cx > SourceX && SourceSurf->sizlBitmap.cy > SourceY)
      {
        Source = DIB_GetSourceIndex(SourceSurf, SourceX, SourceY);
        if ((0x00FFFFFF & Source) != (0x00FFFFFF & iTransColor))
        {
          *DestBits = XLATEOBJ_iXlate(ColorTranslation, Source);
        }
      }
    }

    DestBits = (ULONG*)((ULONG_PTR)DestBits + wd);
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
DIB_32BPP_AlphaBlend(SURFOBJ* Dest, SURFOBJ* Source, RECTL* DestRect,
                     RECTL* SourceRect, CLIPOBJ* ClipRegion,
                     XLATEOBJ* ColorTranslation, BLENDOBJ* BlendObj)
{
  INT Rows, Cols, SrcX, SrcY;
  register PULONG Dst;
  BLENDFUNCTION BlendFunc;
  register NICEPIXEL32 DstPixel, SrcPixel;
  UCHAR Alpha, SrcBpp;

  DPRINT("DIB_32BPP_AlphaBlend: srcRect: (%d,%d)-(%d,%d), dstRect: (%d,%d)-(%d,%d)\n",
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

  Dst = (PULONG)((ULONG_PTR)Dest->pvScan0 + (DestRect->top * Dest->lDelta) +
    (DestRect->left << 2));
  SrcBpp = BitsPerFormat(Source->iBitmapFormat);

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
      SrcPixel.col.green = (SrcPixel.col.green * BlendFunc.SourceConstantAlpha)  / 255;
      SrcPixel.col.blue = (SrcPixel.col.blue * BlendFunc.SourceConstantAlpha) / 255;
      SrcPixel.col.alpha = (32 == SrcBpp) ?
                        (SrcPixel.col.alpha * BlendFunc.SourceConstantAlpha) / 255 :
                        BlendFunc.SourceConstantAlpha ;

      Alpha = ((BlendFunc.AlphaFormat & AC_SRC_ALPHA) != 0) ?
           SrcPixel.col.alpha : BlendFunc.SourceConstantAlpha ;

      DstPixel.ul = *Dst;
      DstPixel.col.red = Clamp8((DstPixel.col.red * (255 - Alpha)) / 255 + SrcPixel.col.red) ;
      DstPixel.col.green = Clamp8((DstPixel.col.green * (255 - Alpha)) / 255 + SrcPixel.col.green) ;
      DstPixel.col.blue = Clamp8((DstPixel.col.blue * (255 - Alpha)) / 255 + SrcPixel.col.blue) ;
      DstPixel.col.alpha = Clamp8((DstPixel.col.alpha * (255 - Alpha)) / 255 + SrcPixel.col.alpha) ;
      *Dst++ = DstPixel.ul;
      SrcX = SourceRect->left + (Cols*(SourceRect->right - SourceRect->left))/(DestRect->right - DestRect->left);
    }
    Dst = (PULONG)((ULONG_PTR)Dest->pvScan0 + ((DestRect->top + Rows) * Dest->lDelta) +
                (DestRect->left << 2));
    SrcY = SourceRect->top + (Rows*(SourceRect->bottom - SourceRect->top))/(DestRect->bottom - DestRect->top);
  }

  return TRUE;
}

/* EOF */
