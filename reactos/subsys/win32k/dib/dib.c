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
/* $Id: dib.c,v 1.11 2004/07/03 13:55:35 navaraf Exp $ */
#include <w32k.h>

/* Static data */

unsigned char notmask[2] = { 0x0f, 0xf0 };
unsigned char altnotmask[2] = { 0xf0, 0x0f };

DIB_FUNCTIONS DibFunctionsForBitmapFormat[] =
{
   /* 0 */
   {
      Dummy_PutPixel, Dummy_GetPixel, Dummy_HLine, Dummy_VLine,
      Dummy_BitBlt, Dummy_StretchBlt, Dummy_TransparentBlt
   },
   /* BMF_1BPP */
   {
      DIB_1BPP_PutPixel, DIB_1BPP_GetPixel, DIB_1BPP_HLine, DIB_1BPP_VLine,
      DIB_1BPP_BitBlt, DIB_1BPP_StretchBlt, DIB_1BPP_TransparentBlt
   },
   /* BMF_4BPP */
   {
      DIB_4BPP_PutPixel, DIB_4BPP_GetPixel, DIB_4BPP_HLine, DIB_4BPP_VLine,
      DIB_4BPP_BitBlt, DIB_4BPP_StretchBlt, DIB_4BPP_TransparentBlt
   },
   /* BMF_8BPP */
   {
      DIB_8BPP_PutPixel, DIB_8BPP_GetPixel, DIB_8BPP_HLine, DIB_8BPP_VLine,
      DIB_8BPP_BitBlt, DIB_8BPP_StretchBlt, DIB_8BPP_TransparentBlt
   },
   /* BMF_16BPP */
   {
      DIB_16BPP_PutPixel, DIB_16BPP_GetPixel, DIB_16BPP_HLine, DIB_16BPP_VLine,
      DIB_16BPP_BitBlt, DIB_16BPP_StretchBlt, DIB_16BPP_TransparentBlt
   },
   /* BMF_24BPP */
   {
      DIB_24BPP_PutPixel, DIB_24BPP_GetPixel, DIB_24BPP_HLine, DIB_24BPP_VLine,
      DIB_24BPP_BitBlt, DIB_24BPP_StretchBlt, DIB_24BPP_TransparentBlt
   },
   /* BMF_32BPP */
   {
      DIB_32BPP_PutPixel, DIB_32BPP_GetPixel, DIB_32BPP_HLine, DIB_32BPP_VLine,
      DIB_32BPP_BitBlt, DIB_32BPP_StretchBlt, DIB_32BPP_TransparentBlt
   },
   /* BMF_4RLE */
   {
      Dummy_PutPixel, Dummy_GetPixel, Dummy_HLine, Dummy_VLine,
      Dummy_BitBlt, Dummy_StretchBlt, Dummy_TransparentBlt
   },
   /* BMF_8RLE */
   {
      Dummy_PutPixel, Dummy_GetPixel, Dummy_HLine, Dummy_VLine,
      Dummy_BitBlt, Dummy_StretchBlt, Dummy_TransparentBlt
   },
   /* BMF_JPEG */
   {
      Dummy_PutPixel, Dummy_GetPixel, Dummy_HLine, Dummy_VLine,
      Dummy_BitBlt, Dummy_StretchBlt, Dummy_TransparentBlt
   },
   /* BMF_PNG */
   {
      Dummy_PutPixel, Dummy_GetPixel, Dummy_HLine, Dummy_VLine,
      Dummy_BitBlt, Dummy_StretchBlt, Dummy_TransparentBlt
   }
};

ULONG
DIB_GetSource(SURFOBJ* SourceSurf, ULONG sx, ULONG sy, XLATEOBJ* ColorTranslation)
{
  switch (SourceSurf->iBitmapFormat)
    {
    case BMF_1BPP:
      return(XLATEOBJ_iXlate(ColorTranslation, DIB_1BPP_GetPixel(SourceSurf, sx, sy)));
    case BMF_4BPP:
      return(XLATEOBJ_iXlate(ColorTranslation, DIB_4BPP_GetPixel(SourceSurf, sx, sy)));
    case BMF_8BPP:
      return(XLATEOBJ_iXlate(ColorTranslation, DIB_8BPP_GetPixel(SourceSurf, sx, sy)));
    case BMF_16BPP:
      return(XLATEOBJ_iXlate(ColorTranslation, DIB_16BPP_GetPixel(SourceSurf, sx, sy)));
    case BMF_24BPP:
      return(XLATEOBJ_iXlate(ColorTranslation, DIB_24BPP_GetPixel(SourceSurf, sx, sy)));
    case BMF_32BPP:
      return(XLATEOBJ_iXlate(ColorTranslation, DIB_32BPP_GetPixel(SourceSurf, sx, sy)));
    default:
      DPRINT1("DIB_GetSource: Unhandled number of bits per pixel in source (%d).\n", BitsPerFormat(SourceSurf->iBitmapFormat));
      return(0);
    }
}

ULONG
DIB_GetSourceIndex(SURFOBJ* SourceSurf, ULONG sx, ULONG sy)
{
  switch (SourceSurf->iBitmapFormat)
    {
    case BMF_1BPP:
      return DIB_1BPP_GetPixel(SourceSurf, sx, sy);
    case BMF_4BPP:
      return DIB_4BPP_GetPixel(SourceSurf, sx, sy);
    case BMF_8BPP:
      return DIB_8BPP_GetPixel(SourceSurf, sx, sy);
    case BMF_16BPP:
      return DIB_16BPP_GetPixel(SourceSurf, sx, sy);
    case BMF_24BPP:
      return DIB_24BPP_GetPixel(SourceSurf, sx, sy);
    case BMF_32BPP:
      return DIB_32BPP_GetPixel(SourceSurf, sx, sy);
    default:
      DPRINT1("DIB_GetOriginalSource: Unhandled number of bits per pixel in source (%d).\n", BitsPerFormat(SourceSurf->iBitmapFormat));
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

VOID Dummy_PutPixel(SURFOBJ* SurfObj, LONG x, LONG y, ULONG c)
{
  return;
}

ULONG Dummy_GetPixel(SURFOBJ* SurfObj, LONG x, LONG y)
{
  return 0;
}

VOID Dummy_HLine(SURFOBJ* SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  return;
}

VOID Dummy_VLine(SURFOBJ* SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  return;
}

BOOLEAN Dummy_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                     RECTL*  DestRect,  POINTL  *SourcePoint,
                     BRUSHOBJ* BrushObj, POINTL BrushOrign,
                     XLATEOBJ *ColorTranslation, ULONG Rop4)
{
  return FALSE;
}

BOOLEAN Dummy_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                         RECTL*  DestRect,  RECTL  *SourceRect,
                         POINTL* MaskOrigin, POINTL BrushOrign,
                         CLIPOBJ *ClipRegion, XLATEOBJ *ColorTranslation,
                         ULONG Mode)
{
  return FALSE;
}

BOOLEAN Dummy_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                             RECTL*  DestRect,  POINTL  *SourcePoint,
                             XLATEOBJ *ColorTranslation, ULONG iTransColor)
{
  return FALSE;
}

/* EOF */
