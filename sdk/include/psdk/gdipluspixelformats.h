/*
 * Copyright (C) 2007 Google (Evan Stade)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _GDIPLUSPIXELFORMATS_H
#define _GDIPLUSPIXELFORMATS_H

typedef DWORD ARGB;
typedef INT PixelFormat;

#define ALPHA_SHIFT 24
#define RED_SHIFT   16
#define GREEN_SHIFT 8
#define BLUE_SHIFT  0
#define ALPHA_MASK  ((ARGB) 0xff << ALPHA_SHIFT)

#define    PixelFormatIndexed   0x00010000
#define    PixelFormatGDI       0x00020000
#define    PixelFormatAlpha     0x00040000
#define    PixelFormatPAlpha    0x00080000
#define    PixelFormatExtended  0x00100000
#define    PixelFormatCanonical 0x00200000

#define    PixelFormatUndefined 0
#define    PixelFormatDontCare  0

#define    PixelFormat1bppIndexed       (1 | ( 1 << 8) | PixelFormatIndexed | PixelFormatGDI)
#define    PixelFormat4bppIndexed       (2 | ( 4 << 8) | PixelFormatIndexed | PixelFormatGDI)
#define    PixelFormat8bppIndexed       (3 | ( 8 << 8) | PixelFormatIndexed | PixelFormatGDI)
#define    PixelFormat16bppGrayScale    (4 | (16 << 8) | PixelFormatExtended)
#define    PixelFormat16bppRGB555       (5 | (16 << 8) | PixelFormatGDI)
#define    PixelFormat16bppRGB565       (6 | (16 << 8) | PixelFormatGDI)
#define    PixelFormat16bppARGB1555     (7 | (16 << 8) | PixelFormatAlpha | PixelFormatGDI)
#define    PixelFormat24bppRGB          (8 | (24 << 8) | PixelFormatGDI)
#define    PixelFormat32bppRGB          (9 | (32 << 8) | PixelFormatGDI)
#define    PixelFormat32bppARGB         (10 | (32 << 8) | PixelFormatAlpha | PixelFormatGDI | PixelFormatCanonical)
#define    PixelFormat32bppPARGB        (11 | (32 << 8) | PixelFormatAlpha | PixelFormatPAlpha | PixelFormatGDI)
#define    PixelFormat48bppRGB          (12 | (48 << 8) | PixelFormatExtended)
#define    PixelFormat64bppARGB         (13 | (64 << 8) | PixelFormatAlpha  | PixelFormatCanonical | PixelFormatExtended)
#define    PixelFormat64bppPARGB        (14 | (64 << 8) | PixelFormatAlpha  | PixelFormatPAlpha | PixelFormatExtended)
#define    PixelFormat32bppCMYK         (15 | (32 << 8))
#define    PixelFormatMax               16

static inline BOOL IsIndexedPixelFormat(PixelFormat format)
{
    return (format & PixelFormatIndexed) != 0;
}

static inline BOOL IsAlphaPixelFormat(PixelFormat format)
{
    return (format & PixelFormatAlpha) != 0;
}

static inline BOOL IsCanonicalPixelFormat(PixelFormat format)
{
    return (format & PixelFormatCanonical) != 0;
}

static inline BOOL IsExtendedPixelFormat(PixelFormat format)
{
    return (format & PixelFormatExtended) != 0;
}

static inline UINT GetPixelFormatSize(PixelFormat format)
{
    return (format >> 8) & 0xff;
}

enum PaletteFlags
{
    PaletteFlagsHasAlpha        = 1,
    PaletteFlagsGrayScale       = 2,
    PaletteFlagsHalftone        = 4
};

#ifdef __cplusplus

struct ColorPalette
{
public:
    UINT Flags;
    UINT Count;
    ARGB Entries[1];
};

#else /* end of c++ typedefs */

typedef struct ColorPalette
{
    UINT Flags;
    UINT Count;
    ARGB Entries[1];
} ColorPalette;

#endif  /* end of c typedefs */

typedef enum DitherType
{
    DitherTypeNone,
    DitherTypeSolid,
    DitherTypeOrdered4x4,
    DitherTypeOrdered8x8,
    DitherTypeOrdered16x16,
    DitherTypeSpiral4x4,
    DitherTypeSpiral8x8,
    DitherTypeDualSpiral4x4,
    DitherTypeDualSpiral8x8,
    DitherTypeErrorDiffusion,
    DitherTypeMax
} DitherType;

typedef enum PaletteType
{
    PaletteTypeCustom,
    PaletteTypeOptimal,
    PaletteTypeFixedBW,
    PaletteTypeFixedHalftone8,
    PaletteTypeFixedHalftone27,
    PaletteTypeFixedHalftone64,
    PaletteTypeFixedHalftone125,
    PaletteTypeFixedHalftone216,
    PaletteTypeFixedHalftone252,
    PaletteTypeFixedHalftone256
} PaletteType;

#endif
