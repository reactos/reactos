/*
 * ReactOS Boot video driver
 *
 * Copyright (C) 2003 Casper S. Hornstroup
 * Copyright (C) 2004 Filip Navara
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id: bootvid.h,v 1.1 2004/03/04 18:55:09 navaraf Exp $
 */

#ifndef _BOOTVID_H
#define _BOOTVID_H

#define PALETTE_FADE_STEPS  20
#define PALETTE_FADE_TIME   20 * 10000 /* 20ms */

/*
 * Windows Bitmap structures
 */

#define RT_BITMAP   2

typedef struct tagRGBQUAD
{
   unsigned char rgbBlue;
   unsigned char rgbGreen;
   unsigned char rgbRed;
   unsigned char rgbReserved;
} RGBQUAD, *PRGBQUAD;

typedef long FXPT2DOT30;

typedef struct tagCIEXYZ
{
   FXPT2DOT30 ciexyzX; 
   FXPT2DOT30 ciexyzY; 
   FXPT2DOT30 ciexyzZ; 
} CIEXYZ, *LPCIEXYZ;

typedef struct tagCIEXYZTRIPLE
{
   CIEXYZ ciexyzRed; 
   CIEXYZ ciexyzGreen; 
   CIEXYZ ciexyzBlue; 
} CIEXYZTRIPLE, *LPCIEXYZTRIPLE;

typedef struct { 
   DWORD bV5Size; 
   LONG bV5Width; 
   LONG bV5Height; 
   WORD bV5Planes; 
   WORD bV5BitCount; 
   DWORD bV5Compression; 
   DWORD bV5SizeImage; 
   LONG bV5XPelsPerMeter; 
   LONG bV5YPelsPerMeter; 
   DWORD bV5ClrUsed; 
   DWORD bV5ClrImportant; 
   DWORD bV5RedMask; 
   DWORD bV5GreenMask; 
   DWORD bV5BlueMask; 
   DWORD bV5AlphaMask; 
   DWORD bV5CSType; 
   CIEXYZTRIPLE bV5Endpoints; 
   DWORD bV5GammaRed; 
   DWORD bV5GammaGreen; 
   DWORD bV5GammaBlue; 
   DWORD bV5Intent; 
   DWORD bV5ProfileData; 
   DWORD bV5ProfileSize; 
   DWORD bV5Reserved; 
} BITMAPV5HEADER, *PBITMAPV5HEADER; 

/*
 * Private driver structures
 */

typedef struct {
  ULONG r;
  ULONG g;
  ULONG b;
} FADER_PALETTE_ENTRY;

typedef struct _VGA_REGISTERS
{
   UCHAR CRT[24];
   UCHAR Attribute[21];
   UCHAR Graphics[9];
   UCHAR Sequencer[5];
   UCHAR Misc;
} VGA_REGISTERS, *PVGA_REGISTERS;

/* VGA registers */
#define MISC         (PUCHAR)0x3c2
#define SEQ          (PUCHAR)0x3c4
#define SEQDATA      (PUCHAR)0x3c5
#define CRTC         (PUCHAR)0x3d4
#define CRTCDATA     (PUCHAR)0x3d5
#define GRAPHICS     (PUCHAR)0x3ce
#define GRAPHICSDATA (PUCHAR)0x3cf
#define ATTRIB       (PUCHAR)0x3c0
#define STATUS       (PUCHAR)0x3da
#define PELMASK      (PUCHAR)0x3c6
#define PELINDEX     (PUCHAR)0x3c8
#define PELDATA      (PUCHAR)0x3c9

/* In pixelsups.S */
extern VOID
InbvPutPixels(int x, int y, unsigned long c);

#endif /* _BOOTVID_H */
