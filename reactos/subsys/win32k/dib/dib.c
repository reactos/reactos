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
/* $Id: dib.c,v 1.9 2004/04/06 21:53:48 weiden Exp $ */

#include <windows.h>
#include <ddk/winddi.h>
#include <win32k/debug.h>
#include <debug.h>
#include "../eng/objects.h"
#include "dib.h"

/* Static data */

unsigned char notmask[2] = { 0x0f, 0xf0 };
unsigned char altnotmask[2] = { 0xf0, 0x0f };

ULONG
DIB_GetSource(SURFOBJ* SourceSurf, SURFGDI* SourceGDI, ULONG sx, ULONG sy, XLATEOBJ* ColorTranslation)
{
  switch (SourceGDI->BitsPerPixel)
    {
    case 1:
      if (DIB_1BPP_GetPixel(SourceSurf, sx, sy))
	{
	  return(XLATEOBJ_iXlate(ColorTranslation, 1));
	}
      else
	{
	  return(XLATEOBJ_iXlate(ColorTranslation, 0));
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
      return(XLATEOBJ_iXlate(ColorTranslation, DIB_32BPP_GetPixel(SourceSurf, sx, sy)));
    default:
      DPRINT1("DIB_GetSource: Unhandled number of bits per pixel in source (%d).\n", SourceGDI->BitsPerPixel);
      return(0);
    }
}

ULONG
DIB_GetSourceIndex(SURFOBJ* SourceSurf, SURFGDI* SourceGDI, ULONG sx, ULONG sy)
{
  switch (SourceGDI->BitsPerPixel)
    {
    case 1:
      return DIB_1BPP_GetPixel(SourceSurf, sx, sy);
    case 4:
      return DIB_4BPP_GetPixel(SourceSurf, sx, sy);
    case 8:
      return DIB_8BPP_GetPixel(SourceSurf, sx, sy);
    case 16:
      return DIB_16BPP_GetPixel(SourceSurf, sx, sy);
    case 24:
      return DIB_24BPP_GetPixel(SourceSurf, sx, sy);
    case 32:
      return DIB_32BPP_GetPixel(SourceSurf, sx, sy);
    default:
      DPRINT1("DIB_GetOriginalSource: Unhandled number of bits per pixel in source (%d).\n", SourceGDI->BitsPerPixel);
      return(0);
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

/* EOF */
