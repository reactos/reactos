/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
/* $Id: dib4bpp.c,v 1.16 2003/08/02 19:46:52 dwelch Exp $ */
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <win32k/bitmaps.h>
#include <win32k/debug.h>
#include <debug.h>
#include <ddk/winddi.h>
#include "../eng/objects.h"
#include "dib.h"

VOID
DIB_4BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c)
{
  PBYTE addr = SurfObj->pvScan0;

  addr += (x>>1) + y * SurfObj->lDelta;
  *addr = (*addr & notmask[x&1]) | (c << ((1-(x&1))<<2));
}

ULONG
DIB_4BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y)
{
  PBYTE addr = SurfObj->pvScan0;

  return (addr[(x>>1) + y * SurfObj->lDelta] >> ((1-(x&1))<<2) ) & 0x0f;
}

VOID
DIB_4BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  PBYTE  addr = SurfObj->pvScan0 + (x1>>1) + y * SurfObj->lDelta;
  LONG  cx = x1;

  while(cx < x2) {
    *addr = (*addr & notmask[x1&1]) | (c << ((1-(x1&1))<<2));
    if((++x1 & 1) == 0)
      ++addr;
    ++cx;
  }
}

VOID
DIB_4BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  PBYTE  addr = SurfObj->pvScan0;
  int  lDelta = SurfObj->lDelta;

  addr += (x>>1) + y1 * lDelta;
  while(y1++ < y2) {
    *addr = (*addr & notmask[x&1]) | (c << ((1-(x&1))<<2));
    addr += lDelta;
  }
}

ULONG
DIB_DoRop(ULONG Rop, ULONG Dest, ULONG Source, ULONG Pattern)
{
  ULONG ResultNibble;
  ULONG Result;
  ULONG i;
  static const ULONG ExpandDest[16] = 
    {
      0x55555555 /* 0000 */,
      0x555555AA /* 0001 */,
      0x5555AA55 /* 0010 */,
      0x5555AAAA /* 0011 */,
      0x55AA5555 /* 0100 */,
      0x55AA55AA /* 0101 */,
      0x55AAAA55 /* 0110 */,
      0x55AAAAAA /* 0111 */,
      0xAA555555 /* 1000 */,
      0xAA5555AA /* 1001 */,
      0xAA55AA55 /* 1010 */,
      0xAA55AAAA /* 1011 */,
      0xAAAA5555 /* 1100 */,
      0xAAAA55AA /* 1101 */,
      0xAAAAAA55 /* 1110 */,
      0xAAAAAAAA /* 1111 */,
    };
  static const ULONG ExpandSource[16] = 
    {
      0x33333333 /* 0000 */,
      0x333333CC /* 0001 */,
      0x3333CC33 /* 0010 */,
      0x3333CCCC /* 0011 */,
      0x33CC3333 /* 0100 */,
      0x33CC33CC /* 0101 */,
      0x33CCCC33 /* 0110 */,
      0x33CCCCCC /* 0111 */,
      0xCC333333 /* 1000 */,
      0xCC3333CC /* 1001 */,
      0xCC33CC33 /* 1010 */,
      0xCC33CCCC /* 1011 */,
      0xCCCC3333 /* 1100 */,
      0xCCCC33CC /* 1101 */,
      0xCCCCCC33 /* 1110 */,
      0xCCCCCCCC /* 1111 */,
    };
  static const ULONG ExpandPattern[16] = 
    {
      0x0F0F0F0F /* 0000 */,
      0x0F0F0FF0 /* 0001 */,
      0x0F0FF00F /* 0010 */,
      0x0F0FF0F0 /* 0011 */,
      0x0FF00F0F /* 0100 */,
      0x0FF00FF0 /* 0101 */,
      0x0FF0F00F /* 0110 */,
      0x0FF0F0F0 /* 0111 */,
      0xF00F0F0F /* 1000 */,
      0xF00F0FF0 /* 1001 */,
      0xF00FF00F /* 1010 */,
      0xF00FF0F0 /* 1011 */,
      0xF0F00F0F /* 1100 */,
      0xF0F00FF0 /* 1101 */,
      0xF0F0F00F /* 1110 */,
      0xF0F0F0F0 /* 1111 */,
    };

  /* Optimized code for the various named rop codes. */
  switch (Rop)
    {
    case BLACKNESS:   return(0);
    case NOTSRCERASE: return(~(Dest | Source));
    case NOTSRCCOPY:  return(~Source);
    case SRCERASE:    return((~Dest) & Source);
    case DSTINVERT:   return(~Dest);
    case PATINVERT:   return(Dest ^ Pattern);
    case SRCINVERT:   return(Dest ^ Source);
    case SRCAND:      return(Dest & Source);
    case MERGEPAINT:  return(Dest & (~Source));
    case SRCPAINT:    return(Dest | Source);
    case MERGECOPY:   return(Source & Pattern);
    case SRCCOPY:     return(Source);
    case PATCOPY:     return(Pattern);
    case PATPAINT:    return(Dest | (~Source) | Pattern);
    case WHITENESS:   return(0xFFFFFFFF);
    }
  /* Expand the ROP operation to all four bytes */
  Rop &= 0x00FF0000;
  Rop = (Rop << 8) | (Rop) | (Rop >> 8) | (Rop >> 16);
  /* Do the operation on four bits simultaneously. */
  Result = 0;
  for (i = 0; i < 8; i++)
    {
      ResultNibble = Rop & ExpandDest[Dest & 0xF] & ExpandSource[Source & 0xF] & ExpandPattern[Pattern & 0xF];
      Result |= (((ResultNibble & 0xFF000000) ? 0x8 : 0x0) | ((ResultNibble & 0x00FF0000) ? 0x4 : 0x0) | 
	((ResultNibble & 0x0000FF00) ? 0x2 : 0x0) | ((ResultNibble & 0x000000FF) ? 0x1 : 0x0)) << (i * 4);
      Dest >>= 4;
      Source >>= 4;
      Pattern >>= 4;
    }
  return(Result);
}

ULONG
DIB_4BPP_GetSource(SURFOBJ* SourceSurf, SURFGDI* SourceGDI, ULONG sx, ULONG sy, XLATEOBJ* ColorTranslation)
{
  switch (SourceGDI->BitsPerPixel)
    {
    case 1:
      if (DIB_1BPP_GetPixel(SourceSurf, sx, sy))
	{
	  return(XLATEOBJ_iXlate(ColorTranslation, 0));
	}
      else
	{
	  return(XLATEOBJ_iXlate(ColorTranslation, 1));
	}
    case 4:
      if (ColorTranslation != NULL)
	{
	  return(XLATEOBJ_iXlate(ColorTranslation, DIB_4BPP_GetPixel(SourceSurf, sx, sy)));
	}
      else
	{
	  return(DIB_4BPP_GetPixel(SourceSurf, sx, sy));
	}
    case 8:
      return(XLATEOBJ_iXlate(ColorTranslation, DIB_8BPP_GetPixel(SourceSurf, sx, sy)));
    case 16:
      return(XLATEOBJ_iXlate(ColorTranslation, DIB_16BPP_GetPixel(SourceSurf, sx, sy)));
    case 24:
      return(XLATEOBJ_iXlate(ColorTranslation, DIB_24BPP_GetPixel(SourceSurf, sx, sy)));
    case 32:
      return(XLATEOBJ_iXlate(ColorTranslation, DIB_8BPP_GetPixel(SourceSurf, sx, sy)));
    default:
      DbgPrint("DIB_4BPP_ExpandSource: Unhandled number of bits per pixel in source.\n");
      return(0);
    }
}

BOOLEAN STATIC
DIB_4BPP_BitBltSrcCopy(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
		       SURFGDI *DestGDI,  SURFGDI *SourceGDI,
		       PRECTL  DestRect,  POINTL  *SourcePoint,
		       XLATEOBJ* ColorTranslation)
{
  LONG     i, j, sx, sy, f2, xColor;
  PBYTE    SourceBits_24BPP, SourceLine_24BPP;
  PBYTE    DestBits, DestLine, SourceBits_8BPP, SourceLine_8BPP;
  PBYTE    SourceBits, SourceLine;

  DestBits = DestSurf->pvScan0 + (DestRect->left>>1) + DestRect->top * DestSurf->lDelta;

  switch(SourceGDI->BitsPerPixel)
  {
    case 1:
      sx = SourcePoint->x;
      sy = SourcePoint->y;

      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        sx = SourcePoint->x;
        for (i=DestRect->left; i<DestRect->right; i++)
        {
          if(DIB_1BPP_GetPixel(SourceSurf, sx, sy) == 0)
          {
            DIB_4BPP_PutPixel(DestSurf, i, j, XLATEOBJ_iXlate(ColorTranslation, 0));
          } else {
            DIB_4BPP_PutPixel(DestSurf, i, j, XLATEOBJ_iXlate(ColorTranslation, 1));
          }
          sx++;
        }
        sy++;
      }
      break;

    case 4:
      sy = SourcePoint->y;

      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        sx = SourcePoint->x;

        for (i=DestRect->left; i<DestRect->right; i++)
        {
	  if (NULL != ColorTranslation)
	  {
	    DIB_4BPP_PutPixel(DestSurf, i, j, XLATEOBJ_iXlate(ColorTranslation, DIB_4BPP_GetPixel(SourceSurf, sx, sy)));
	  }
	  else
	  {
	    DIB_4BPP_PutPixel(DestSurf, i, j, DIB_4BPP_GetPixel(SourceSurf, sx, sy));
	  }
          sx++;
        }
        sy++;
      }
      break;

    case 8:
      SourceBits_8BPP = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + SourcePoint->x;

      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        SourceLine_8BPP = SourceBits_8BPP;
        DestLine = DestBits;
        f2 = DestRect->left & 1;

        for (i=DestRect->left; i<DestRect->right; i++)
        {
          *DestLine = (*DestLine & notmask[i&1]) |
            ((XLATEOBJ_iXlate(ColorTranslation, *SourceLine_8BPP)) << ((4 * (1-(i & 1)))));
          if(f2 == 1) { DestLine++; f2 = 0; } else { f2 = 1; }
          SourceLine_8BPP++;
        }

        SourceBits_8BPP += SourceSurf->lDelta;
        DestBits += DestSurf->lDelta;
      }
      break;

    case 16:
      SourceLine = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + 2 * SourcePoint->x;
      DestLine = DestBits;

      for (j = DestRect->top; j < DestRect->bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;
        f2 = DestRect->left & 1;

        for (i = DestRect->left; i < DestRect->right; i++)
        {
          xColor = *((PWORD) SourceBits);
          *DestBits = (*DestBits & notmask[i&1]) |
            ((XLATEOBJ_iXlate(ColorTranslation, xColor)) << ((4 * (1-(i & 1)))));
          if(f2 == 1) { DestBits++; f2 = 0; } else { f2 = 1; }
          SourceBits += 2;
        }

        SourceLine += SourceSurf->lDelta;
        DestLine += DestSurf->lDelta;
      }
      break;

    case 24:
      SourceBits_24BPP = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + SourcePoint->x * 3;

      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        SourceLine_24BPP = SourceBits_24BPP;
        DestLine = DestBits;
        f2 = DestRect->left & 1;

        for (i=DestRect->left; i<DestRect->right; i++)
        {
          xColor = (*(SourceLine_24BPP + 2) << 0x10) +
             (*(SourceLine_24BPP + 1) << 0x08) +
             (*(SourceLine_24BPP));
          *DestLine = (*DestLine & notmask[i&1]) |
            ((XLATEOBJ_iXlate(ColorTranslation, xColor)) << ((4 * (1-(i & 1)))));
          if(f2 == 1) { DestLine++; f2 = 0; } else { f2 = 1; }
          SourceLine_24BPP+=3;
        }

        SourceBits_24BPP += SourceSurf->lDelta;
        DestBits += DestSurf->lDelta;
      }
      break;

    case 32:
      SourceLine = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + 4 * SourcePoint->x;
      DestLine = DestBits;

      for (j = DestRect->top; j < DestRect->bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;
        f2 = DestRect->left & 1;

        for (i = DestRect->left; i < DestRect->right; i++)
        {
          xColor = *((PDWORD) SourceBits);
          *DestBits = (*DestBits & notmask[i&1]) |
            ((XLATEOBJ_iXlate(ColorTranslation, xColor)) << ((4 * (1-(i & 1)))));
          if(f2 == 1) { DestBits++; f2 = 0; } else { f2 = 1; }
          SourceBits += 4;
        }

        SourceLine += SourceSurf->lDelta;
        DestLine += DestSurf->lDelta;
      }
      break;

    default:
      DbgPrint("DIB_4BPP_Bitblt: Unhandled Source BPP: %u\n", SourceGDI->BitsPerPixel);
      return FALSE;
  }
  return(TRUE);
}

BOOLEAN
DIB_4BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
		SURFGDI *DestGDI,  SURFGDI *SourceGDI,
		PRECTL  DestRect,  POINTL  *SourcePoint,
		PBRUSHOBJ Brush, PPOINTL BrushOrigin,
		XLATEOBJ *ColorTranslation, ULONG Rop4)
{
  LONG     i, j, k, sx, sy;
  ULONG    Dest, Source, Pattern;
  PULONG   DestBits;
  BOOL     UsesSource = ((Rop4 & 0xCC0000) >> 2) != (Rop4 & 0x330000);
  BOOL     UsesPattern = ((Rop4 & 0xF00000) >> 4) != (Rop4 & 0x0F0000);  
  ULONG    RoundedRight = DestRect->right - (DestRect->right & 0x7);
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

  if (Rop4 == SRCCOPY)
    {
      return(DIB_4BPP_BitBltSrcCopy(DestSurf, SourceSurf, DestGDI, SourceGDI, DestRect, SourcePoint, ColorTranslation));
    }
  else
    {
      sy = SourcePoint->y;

      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        sx = SourcePoint->x;
	DestBits = (PULONG)(DestSurf->pvScan0 + (DestRect->left>>1) + j * DestSurf->lDelta);
        for (i=DestRect->left; i<RoundedRight; i+=8, DestBits++)
	  {
	    Dest = *DestBits;
	    if (UsesSource)
	      {
		Source = 0;
		for (k = 0; k < 8; k++)
		  {
		    Source |= (DIB_4BPP_GetSource(SourceSurf, SourceGDI, sx + i + k, sy, ColorTranslation) << (k * 4));
		  }
	      }
	    if (UsesPattern)
	      {
		/* FIXME: No support for pattern brushes. */
		Pattern = ExpandSolidColor[Brush->iSolidColor & 0xF];
	      }
	    *DestBits = DIB_DoRop(Rop4, Dest, Source, Pattern);	    
	  }
	if (i < DestRect->right)
	  {
	    Dest = *DestBits;
	    for (; i < DestRect->right; i++)
	      {
		if (UsesSource)
		  {
		    Source = DIB_4BPP_GetSource(SourceSurf, SourceGDI, sx + i, sy, ColorTranslation);
		  }
		if (UsesPattern)
		  {
		    /* FIXME: No support for pattern brushes. */
		    Pattern = ExpandSolidColor[Brush->iSolidColor & 0xF];
		  }				
		DIB_4BPP_PutPixel(DestSurf, i, j, DIB_DoRop(Rop4, Dest, Source, Pattern) & 0xF);
		Dest >>= 4;
	      }	 
	  }
      }
    }
  return TRUE;
}
/* EOF */
