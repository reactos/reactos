/*
 * GdiPlusPixelFormats.h
 *
 * Windows GDI+
 *
 * This file is part of the w32api package.
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef _GDIPLUSPIXELFORMATS_H
#define _GDIPLUSPIXELFORMATS_H

#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#define PixelFormatIndexed 0x00010000
#define PixelFormatGDI 0x00020000
#define PixelFormatAlpha 0x00040000
#define PixelFormatPAlpha 0x00080000
#define PixelFormatExtended 0x00100000
#define PixelFormatCanonical 0x00200000

#define PixelFormatUndefined 0
#define PixelFormatDontCare 0

#define PixelFormat1bppIndexed (1 | ( 1 << 8) | PixelFormatIndexed | PixelFormatGDI)
#define PixelFormat4bppIndexed (2 | ( 4 << 8) | PixelFormatIndexed | PixelFormatGDI)
#define PixelFormat8bppIndexed (3 | ( 8 << 8) | PixelFormatIndexed | PixelFormatGDI)
#define PixelFormat16bppARGB1555 (7 | (16 << 8) | PixelFormatAlpha | PixelFormatGDI)
#define PixelFormat16bppGrayScale (4 | (16 << 8) | PixelFormatExtended)
#define PixelFormat16bppRGB555 (5 | (16 << 8) | PixelFormatGDI)
#define PixelFormat16bppRGB565 (6 | (16 << 8) | PixelFormatGDI)
#define PixelFormat24bppRGB (8 | (24 << 8) | PixelFormatGDI)
#define PixelFormat32bppARGB (10 | (32 << 8) | PixelFormatAlpha | PixelFormatGDI | PixelFormatCanonical)
#define PixelFormat32bppPARGB (11 | (32 << 8) | PixelFormatAlpha | PixelFormatPAlpha | PixelFormatGDI)
#define PixelFormat32bppRGB (9 | (32 << 8) | PixelFormatGDI)
#define PixelFormat48bppRGB (12 | (48 << 8) | PixelFormatExtended)
#define PixelFormat64bppARGB (13 | (64 << 8) | PixelFormatAlpha  | PixelFormatCanonical | PixelFormatExtended)
#define PixelFormat64bppPARGB (14 | (64 << 8) | PixelFormatAlpha  | PixelFormatPAlpha | PixelFormatExtended)


typedef INT PixelFormat;

typedef DWORD ARGB;

typedef struct {
  UINT Flags;
  UINT Count;
  ARGB Entries[1];
} ColorPalette;

#endif /* _GDIPLUSPIXELFORMATS_H */
