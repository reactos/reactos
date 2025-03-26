/*
 * Copyright 2020 Ziqing Hui
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "wincodec.h"
#include "wine/test.h"

#ifdef __REACTOS__
#define debugstr_guid wine_dbgstr_guid
#endif

#define GET_RGB565_R(color)   ((BYTE)(((color) >> 11) & 0x1F))
#define GET_RGB565_G(color)   ((BYTE)(((color) >> 5)  & 0x3F))
#define GET_RGB565_B(color)   ((BYTE)(((color) >> 0)  & 0x1F))
#define MAKE_RGB565(r, g, b)  ((WORD)(((BYTE)(r) << 11) | ((BYTE)(g) << 5) | (BYTE)(b)))
#define MAKE_ARGB(a, r, g, b) (((DWORD)(a) << 24) | ((DWORD)(r) << 16) | ((DWORD)(g) << 8) | (DWORD)(b))

#define BLOCK_WIDTH  4
#define BLOCK_HEIGHT 4

/* 1x1 uncompressed(Alpha) DDS image */
static BYTE test_dds_alpha[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF
};

/* 1x1 uncompressed(Luminance) DDS image */
static BYTE test_dds_luminance[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x82
};

/* 4x4 uncompressed(16bpp RGB565) DDS image */
static BYTE test_dds_rgb565[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00,
    0xE0, 0x07, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xF5, 0xA7, 0x08, 0x69, 0x4C, 0x7B, 0x08, 0x69, 0xF5, 0xA7, 0xF5, 0xA7, 0xF5, 0xA7, 0x4C, 0x7B,
    0x4C, 0x7B, 0x4C, 0x7B, 0x4C, 0x7B, 0xB1, 0x95, 0x4C, 0x7B, 0x08, 0x69, 0x08, 0x69, 0x4C, 0x7B
};

/* 1x1 uncompressed(24bpp RGB) DDS image */
static BYTE test_dds_24bpp[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00,
    0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x70, 0x81, 0x83
};

/* 1x1 uncompressed(32bpp XRGB) DDS image */
static BYTE test_dds_32bpp_xrgb[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00,
    0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x70, 0x81, 0x83, 0x00
};

/* 1x1 uncompressed(32bpp ARGB) DDS image */
static BYTE test_dds_32bpp_argb[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00,
    0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x70, 0x81, 0x83, 0xFF
};

/* 1x1 uncompressed(64bpp ABGR) DDS image */
static BYTE test_dds_64bpp[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x0F, 0x10, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x83, 0x83, 0x81, 0x81, 0x70, 0x70, 0xFF, 0xFF
};

/* 1x1 uncompressed(96bpp ABGR float) DDS image */
static BYTE test_dds_96bpp[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x0F, 0x10, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 'D',  'X',  '1',  '0',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x84, 0x83, 0x03, 0x3F, 0x82, 0x81, 0x01, 0x3F, 0xE2, 0xE0, 0xE0, 0x3E
};

/* 1x1 uncompressed(128bpp ABGR float) DDS image */
static BYTE test_dds_128bpp[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x0F, 0x10, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x84, 0x83, 0x03, 0x3F, 0x82, 0x81, 0x01, 0x3F, 0xE2, 0xE0, 0xE0, 0x3E, 0x00, 0x00, 0x80, 0x3F
};

/* 4x4 compressed(DXT1) cube map, mipMapCount = 3 */
static BYTE test_dds_cube[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x0A, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 'D',  'X',  'T',  '1',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xF5, 0xA7, 0x08, 0x69, 0x74, 0xC0, 0xBF, 0xD7, 0x32, 0x96, 0x0B, 0x7B, 0xCC, 0x55, 0xCC, 0x55,
    0x0E, 0x84, 0x0E, 0x84, 0x00, 0x00, 0x00, 0x00, 0xF5, 0xA7, 0x08, 0x69, 0x74, 0xC0, 0xBF, 0xD7,
    0x32, 0x96, 0x0B, 0x7B, 0xCC, 0x55, 0xCC, 0x55, 0x0E, 0x84, 0x0E, 0x84, 0x00, 0x00, 0x00, 0x00,
    0xF5, 0xA7, 0x08, 0x69, 0x74, 0xC0, 0xBF, 0xD7, 0x32, 0x96, 0x0B, 0x7B, 0xCC, 0x55, 0xCC, 0x55,
    0x0E, 0x84, 0x0E, 0x84, 0x00, 0x00, 0x00, 0x00, 0xF5, 0xA7, 0x08, 0x69, 0x74, 0xC0, 0xBF, 0xD7,
    0x32, 0x96, 0x0B, 0x7B, 0xCC, 0x55, 0xCC, 0x55, 0x0E, 0x84, 0x0E, 0x84, 0x00, 0x00, 0x00, 0x00,
    0xF5, 0xA7, 0x08, 0x69, 0x74, 0xC0, 0xBF, 0xD7, 0x32, 0x96, 0x0B, 0x7B, 0xCC, 0x55, 0xCC, 0x55,
    0x0E, 0x84, 0x0E, 0x84, 0x00, 0x00, 0x00, 0x00, 0xF5, 0xA7, 0x08, 0x69, 0x74, 0xC0, 0xBF, 0xD7,
    0x32, 0x96, 0x0B, 0x7B, 0xCC, 0x55, 0xCC, 0x55, 0x0E, 0x84, 0x0E, 0x84, 0x00, 0x00, 0x00, 0x00
};

/* 4x4 compressed(DXT1) cube map with extended header, mipMapCount=3 */
static BYTE test_dds_cube_dx10[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x0A, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 'D',  'X',  '1',  '0',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x47, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x03, 0x00, 0x00, 0x00, 0xF5, 0xA7, 0x08, 0x69, 0x74, 0xC0, 0xBF, 0xD7, 0x32, 0x96, 0x0B, 0x7B,
    0xCC, 0x55, 0xCC, 0x55, 0x0E, 0x84, 0x0E, 0x84, 0x00, 0x00, 0x00, 0x00, 0xF5, 0xA7, 0x08, 0x69,
    0x74, 0xC0, 0xBF, 0xD7, 0x32, 0x96, 0x0B, 0x7B, 0xCC, 0x55, 0xCC, 0x55, 0x0E, 0x84, 0x0E, 0x84,
    0x00, 0x00, 0x00, 0x00, 0xF5, 0xA7, 0x08, 0x69, 0x74, 0xC0, 0xBF, 0xD7, 0x32, 0x96, 0x0B, 0x7B,
    0xCC, 0x55, 0xCC, 0x55, 0x0E, 0x84, 0x0E, 0x84, 0x00, 0x00, 0x00, 0x00, 0xF5, 0xA7, 0x08, 0x69,
    0x74, 0xC0, 0xBF, 0xD7, 0x32, 0x96, 0x0B, 0x7B, 0xCC, 0x55, 0xCC, 0x55, 0x0E, 0x84, 0x0E, 0x84,
    0x00, 0x00, 0x00, 0x00, 0xF5, 0xA7, 0x08, 0x69, 0x74, 0xC0, 0xBF, 0xD7, 0x32, 0x96, 0x0B, 0x7B,
    0xCC, 0x55, 0xCC, 0x55, 0x0E, 0x84, 0x0E, 0x84, 0x00, 0x00, 0x00, 0x00, 0xF5, 0xA7, 0x08, 0x69,
    0x74, 0xC0, 0xBF, 0xD7, 0x32, 0x96, 0x0B, 0x7B, 0xCC, 0x55, 0xCC, 0x55, 0x0E, 0x84, 0x0E, 0x84,
    0x00, 0x00, 0x00, 0x00
};

/* 4x4 compressed(DXT1) DDS image with mip maps, mipMapCount=3 */
static BYTE test_dds_mipmaps[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x0A, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 'D',  'X',  'T',  '1',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xF5, 0xA7, 0x08, 0x69, 0x74, 0xC0, 0xBF, 0xD7, 0xB1, 0x95, 0x6D, 0x7B, 0xFC, 0x55, 0x5D, 0x5D,
    0x2E, 0x8C, 0x4E, 0x7C, 0xAA, 0xAB, 0xAB, 0xAB
};

/* 4x4 compressed(DXT1) volume texture, depth=4, mipMapCount=3 */
static BYTE test_dds_volume[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x8A, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 'D',  'X',  'T',  '1',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xD5, 0xA7, 0x2C, 0x7B, 0xE0, 0x00, 0x55, 0x55, 0xD5, 0xA7, 0x49, 0x69, 0x57, 0x00, 0xFF, 0x55,
    0xD5, 0xA7, 0x48, 0x69, 0xFD, 0x80, 0xFF, 0x55, 0x30, 0x8D, 0x89, 0x71, 0x55, 0xA8, 0x00, 0xFF,
    0x32, 0x96, 0x6D, 0x83, 0xA8, 0x55, 0x5D, 0x5D, 0x0E, 0x84, 0x6D, 0x7B, 0xA8, 0xA9, 0xAD, 0xAD,
    0x2E, 0x8C, 0x2E, 0x7C, 0xAA, 0xAB, 0xAB, 0xAB
};

/* 4x4 compressed(DXT1) texture array, arraySize=3, mipMapCount=3 */
static BYTE test_dds_array[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x0A, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 'D',  'X',  '1',  '0',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x47, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xF5, 0xA7, 0x08, 0x69, 0x74, 0xC0, 0xBF, 0xD7, 0x32, 0x96, 0x0B, 0x7B,
    0xCC, 0x55, 0xCC, 0x55, 0x0E, 0x84, 0x0E, 0x84, 0x00, 0x00, 0x00, 0x00, 0xF5, 0xA7, 0x08, 0x69,
    0x74, 0xC0, 0xBF, 0xD7, 0x32, 0x96, 0x0B, 0x7B, 0xCC, 0x55, 0xCC, 0x55, 0x0E, 0x84, 0x0E, 0x84,
    0x00, 0x00, 0x00, 0x00, 0xF5, 0xA7, 0x08, 0x69, 0x74, 0xC0, 0xBF, 0xD7, 0x32, 0x96, 0x0B, 0x7B,
    0xCC, 0x55, 0xCC, 0x55, 0x0E, 0x84, 0x0E, 0x84, 0x00, 0x00, 0x00, 0x00
};

/* 4x4 compressed(DXT1c) DDS image */
static BYTE test_dds_dxt1c[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 'D',  'X',  'T',  '1',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x9A, 0xE6, 0x2B, 0x39, 0x37, 0xB7, 0x7F, 0x7F
};

/* 4x4 compressed(DXT1a) DDS image */
static BYTE test_dds_dxt1a[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 'D',  'X',  'T',  '1',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2A, 0x31, 0xF5, 0xBC, 0xE3, 0x6E, 0x2A, 0x3A
};

/* 4x4 compressed(DXT2) DDS image, mipMapCount=3 */
static BYTE test_dds_dxt2[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 'D',  'X',  'T',  '2',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD, 0xDE, 0xC4, 0x10, 0x2F, 0xBF, 0xFF, 0x7B,
    0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x57, 0x53, 0x00, 0x00, 0x52, 0x52, 0x55, 0x55,
    0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCE, 0x59, 0x00, 0x00, 0x54, 0x55, 0x55, 0x55
};

/* 1x3 compressed(DXT3) DDS image, mipMapCount=2 */
static BYTE test_dds_dxt3[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x0A, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 'D',  'X',  'T',  '3',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0C, 0x92, 0x38, 0x84, 0x00, 0xFF, 0x55, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x53, 0x8B, 0x53, 0x8B, 0x00, 0x00, 0x00, 0x00
};

/* 4x4 compressed(DXT4) DDS image, mipMapCount=3 */
static BYTE test_dds_dxt4[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 'D',  'X',  'T',  '4',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFD, 0xDE, 0xC4, 0x10, 0x2F, 0xBF, 0xFF, 0x7B,
    0xFF, 0x00, 0x40, 0x02, 0x24, 0x49, 0x92, 0x24, 0x57, 0x53, 0x00, 0x00, 0x52, 0x52, 0x55, 0x55,
    0xFF, 0x00, 0x48, 0x92, 0x24, 0x49, 0x92, 0x24, 0xCE, 0x59, 0x00, 0x00, 0x54, 0x55, 0x55, 0x55
};

/* 6x6 compressed(DXT5) image, mipMapCount=3 */
static BYTE test_dds_dxt5[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x0A, 0x00, 0x06, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x44, 0x58, 0x54, 0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x73, 0x73, 0x8E, 0x51, 0x97, 0x97, 0xBF, 0xAF,
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9D, 0xC6, 0xCF, 0x52, 0x22, 0x22, 0xBB, 0x55,
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0xA2, 0xB8, 0x5B, 0xF8, 0xF8, 0xF8, 0xF8,
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x3A, 0x05, 0x19, 0xCC, 0x66, 0xCC, 0x66,
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x9D, 0x0A, 0x39, 0xCF, 0xEF, 0x9B, 0xEF,
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x6A, 0xF0, 0x6A, 0x00, 0x00, 0x00, 0x00
};

/* 12x12 compressed(DXT3) texture array, arraySize=2, mipMapCount=4 */
static BYTE test_dds_12x12[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x0A, 0x00, 0x0C, 0x00, 0x00, 0x00,
    0x0C, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 'D',  'X',  '1',  '0',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x4A, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xB5, 0xA7, 0xAD, 0x83,
    0x60, 0x60, 0xE0, 0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x12, 0x96, 0x6B, 0x72,
    0xD5, 0xD5, 0xAF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x30, 0x8D, 0x89, 0x69,
    0x57, 0x5F, 0x5E, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xB4, 0xA7, 0xAD, 0x83,
    0x00, 0xAA, 0xFF, 0x55, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF3, 0x9E, 0x6D, 0x83,
    0x00, 0x00, 0xAA, 0x55, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x12, 0x96, 0xCD, 0x83,
    0x5C, 0xF8, 0xAA, 0xAF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEC, 0x7A, 0xC9, 0x71,
    0x80, 0x60, 0x60, 0x60, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x4A, 0x72, 0xA8, 0x68,
    0x28, 0xBE, 0xD7, 0xD7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xAF, 0x8C, 0xEA, 0x71,
    0x0B, 0xAB, 0xAD, 0xBD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x54, 0x9F, 0xCC, 0x7A,
    0x5C, 0xA8, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x8D, 0x49, 0x69,
    0x77, 0xEE, 0x88, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0C, 0x7B, 0x08, 0x69,
    0xF8, 0x58, 0xF8, 0x58, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x4E, 0x84, 0x6B, 0x72,
    0x33, 0x99, 0x33, 0x99, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x14, 0x9F, 0x0A, 0x72,
    0xDC, 0xAA, 0x75, 0xAA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0E, 0x84, 0x0E, 0x84,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xB5, 0xA7, 0xAD, 0x83,
    0x60, 0x60, 0xE0, 0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x12, 0x96, 0x6B, 0x72,
    0xD5, 0xD5, 0xAF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x30, 0x8D, 0x89, 0x69,
    0x57, 0x5F, 0x5E, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xB4, 0xA7, 0xAD, 0x83,
    0x00, 0xAA, 0xFF, 0x55, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF3, 0x9E, 0x6D, 0x83,
    0x00, 0x00, 0xAA, 0x55, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x12, 0x96, 0xCD, 0x83,
    0x5C, 0xF8, 0xAA, 0xAF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEC, 0x7A, 0xC9, 0x71,
    0x80, 0x60, 0x60, 0x60, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x4A, 0x72, 0xA8, 0x68,
    0x28, 0xBE, 0xD7, 0xD7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xAF, 0x8C, 0xEA, 0x71,
    0x0B, 0xAB, 0xAD, 0xBD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x54, 0x9F, 0xCC, 0x7A,
    0x5C, 0xA8, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x8D, 0x49, 0x69,
    0x77, 0xEE, 0x88, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0C, 0x7B, 0x08, 0x69,
    0xF8, 0x58, 0xF8, 0x58, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x4E, 0x84, 0x6B, 0x72,
    0x33, 0x99, 0x33, 0x99, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x14, 0x9F, 0x0A, 0x72,
    0xDC, 0xAA, 0x75, 0xAA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0E, 0x84, 0x0E, 0x84,
    0x00, 0x00, 0x00, 0x00
};

static BYTE  test_dds_bad_magic[sizeof(test_dds_dxt1c)];
static BYTE  test_dds_bad_header[sizeof(test_dds_dxt1c)];
static BYTE  test_byte[1]    = { 0 };
static BYTE  test_word[2]    = { 0 };
static BYTE  test_dword[4]   = { 0 };
static BYTE  test_qword_a[8] = { 0 };
static BYTE  test_qword_b[8] = "DDS ";

static struct test_data {
    BYTE *data;
    UINT size;
    HRESULT init_hr;
    UINT expected_frame_count;
    UINT expected_bytes_per_block;
    UINT pixel_format_bpp;
    const GUID *expected_pixel_format;
    WICDdsParameters expected_parameters;
    BOOL wine_init;
} test_data[] = {
    { test_dds_alpha,      sizeof(test_dds_alpha),         WINCODEC_ERR_BADHEADER, 1,  1,  8,  &GUID_WICPixelFormat8bppAlpha,
      { 1,  1,  1, 1, 1,   DXGI_FORMAT_A8_UNORM,           WICDdsTexture2D,   WICDdsAlphaModeUnknown },       TRUE },
    { test_dds_luminance,  sizeof(test_dds_luminance),     WINCODEC_ERR_BADHEADER, 1,  1,  8,  &GUID_WICPixelFormat8bppGray,
      { 1,  1,  1, 1, 1,   DXGI_FORMAT_R8_UNORM,           WICDdsTexture2D,   WICDdsAlphaModeUnknown },       TRUE },
    { test_dds_rgb565,     sizeof(test_dds_rgb565),        WINCODEC_ERR_BADHEADER, 1,  2,  16,  &GUID_WICPixelFormat16bppBGR565,
      { 4,  4,  1, 1, 1,   DXGI_FORMAT_B5G6R5_UNORM,       WICDdsTexture2D,   WICDdsAlphaModeUnknown },       TRUE },
    { test_dds_24bpp,      sizeof(test_dds_24bpp),         WINCODEC_ERR_BADHEADER, 1,  3,  24,  &GUID_WICPixelFormat24bppBGR,
      { 1,  1,  1, 1, 1,   DXGI_FORMAT_UNKNOWN,            WICDdsTexture2D,   WICDdsAlphaModeUnknown },       TRUE },
    { test_dds_32bpp_xrgb, sizeof(test_dds_32bpp_xrgb),    WINCODEC_ERR_BADHEADER, 1,  4,  32,  &GUID_WICPixelFormat32bppBGR,
      { 1,  1,  1, 1, 1,   DXGI_FORMAT_B8G8R8X8_UNORM,     WICDdsTexture2D,   WICDdsAlphaModeUnknown },       TRUE },
    { test_dds_32bpp_argb, sizeof(test_dds_32bpp_argb),    WINCODEC_ERR_BADHEADER, 1,  4,  32,  &GUID_WICPixelFormat32bppBGRA,
      { 1,  1,  1, 1, 1,   DXGI_FORMAT_B8G8R8A8_UNORM,     WICDdsTexture2D,   WICDdsAlphaModeUnknown },       TRUE },
    { test_dds_64bpp,      sizeof(test_dds_64bpp),         WINCODEC_ERR_BADHEADER, 1,  8,  64,  &GUID_WICPixelFormat64bppRGBA,
      { 1,  1,  1, 1, 1,   DXGI_FORMAT_R16G16B16A16_UNORM, WICDdsTexture2D,   WICDdsAlphaModeUnknown },       TRUE },
    { test_dds_96bpp,      sizeof(test_dds_96bpp),         WINCODEC_ERR_BADHEADER, 1,  12, 96,  &GUID_WICPixelFormat96bppRGBFloat,
      { 1,  1,  1, 1, 1,   DXGI_FORMAT_R32G32B32_FLOAT,    WICDdsTexture2D,   WICDdsAlphaModeUnknown },       TRUE },
    { test_dds_128bpp,     sizeof(test_dds_128bpp),        WINCODEC_ERR_BADHEADER, 1,  16, 128, &GUID_WICPixelFormat128bppRGBAFloat,
      { 1,  1,  1, 1, 1,   DXGI_FORMAT_R32G32B32A32_FLOAT, WICDdsTexture2D,   WICDdsAlphaModeUnknown },       TRUE },
    { test_dds_cube,       sizeof(test_dds_cube),          WINCODEC_ERR_BADHEADER, 18, 8,  32,  &GUID_WICPixelFormat32bppPBGRA,
      { 4,  4,  1, 3, 1,   DXGI_FORMAT_BC1_UNORM,          WICDdsTextureCube, WICDdsAlphaModePremultiplied }, TRUE },
    { test_dds_cube_dx10,  sizeof(test_dds_cube_dx10),     WINCODEC_ERR_BADHEADER, 18, 8,  32,  &GUID_WICPixelFormat32bppBGRA,
      { 4,  4,  1, 3, 1,   DXGI_FORMAT_BC1_UNORM,          WICDdsTextureCube, WICDdsAlphaModeUnknown },       TRUE },
    { test_dds_mipmaps,    sizeof(test_dds_mipmaps),       S_OK,                   3,  8,  32,  &GUID_WICPixelFormat32bppPBGRA,
      { 4,  4,  1, 3, 1,   DXGI_FORMAT_BC1_UNORM,          WICDdsTexture2D,   WICDdsAlphaModePremultiplied } },
    { test_dds_volume,     sizeof(test_dds_volume),        S_OK,                   7,  8,  32,  &GUID_WICPixelFormat32bppPBGRA,
      { 4,  4,  4, 3, 1,   DXGI_FORMAT_BC1_UNORM,          WICDdsTexture3D,   WICDdsAlphaModePremultiplied } },
    { test_dds_array,      sizeof(test_dds_array),         S_OK,                   9,  8,  32,  &GUID_WICPixelFormat32bppBGRA,
      { 4,  4,  1, 3, 3,   DXGI_FORMAT_BC1_UNORM,          WICDdsTexture2D,   WICDdsAlphaModeUnknown } },
    { test_dds_dxt1c,      sizeof(test_dds_dxt1c),         S_OK,                   1,  8,  32,  &GUID_WICPixelFormat32bppPBGRA,
      { 4,  4,  1, 1, 1,   DXGI_FORMAT_BC1_UNORM,          WICDdsTexture2D,   WICDdsAlphaModePremultiplied } },
    { test_dds_dxt1a,      sizeof(test_dds_dxt1a),         S_OK,                   1,  8,  32,  &GUID_WICPixelFormat32bppPBGRA,
      { 4,  4,  1, 1, 1,   DXGI_FORMAT_BC1_UNORM,          WICDdsTexture2D,   WICDdsAlphaModePremultiplied } },
    { test_dds_dxt2,       sizeof(test_dds_dxt2),          S_OK,                   3,  16, 32,  &GUID_WICPixelFormat32bppPBGRA,
      { 4,  4,  1, 3, 1,   DXGI_FORMAT_BC2_UNORM,          WICDdsTexture2D,   WICDdsAlphaModePremultiplied } },
    { test_dds_dxt3,       sizeof(test_dds_dxt3),          S_OK,                   2,  16, 32,  &GUID_WICPixelFormat32bppBGRA,
      { 1,  3,  1, 2, 1,   DXGI_FORMAT_BC2_UNORM,          WICDdsTexture2D,   WICDdsAlphaModeUnknown } },
    { test_dds_dxt4,       sizeof(test_dds_dxt4),          S_OK,                   3,  16, 32,  &GUID_WICPixelFormat32bppPBGRA,
      { 4,  4,  1, 3, 1,   DXGI_FORMAT_BC3_UNORM,          WICDdsTexture2D,   WICDdsAlphaModePremultiplied } },
    { test_dds_dxt5,       sizeof(test_dds_dxt5),          S_OK,                   3,  16, 32,  &GUID_WICPixelFormat32bppBGRA,
      { 6,  6,  1, 3, 1,   DXGI_FORMAT_BC3_UNORM,          WICDdsTexture2D,   WICDdsAlphaModeUnknown } },
    { test_dds_12x12,      sizeof(test_dds_12x12),         S_OK,                   8,  16, 32,  &GUID_WICPixelFormat32bppBGRA,
      { 12, 12, 1, 4, 2,   DXGI_FORMAT_BC2_UNORM,          WICDdsTexture2D,   WICDdsAlphaModeUnknown } },
    { test_dds_bad_magic,  sizeof(test_dds_bad_magic),     WINCODEC_ERR_UNKNOWNIMAGEFORMAT },
    { test_dds_bad_header, sizeof(test_dds_bad_header),    WINCODEC_ERR_BADHEADER },
    { test_byte,           sizeof(test_byte),              WINCODEC_ERR_STREAMREAD },
    { test_word,           sizeof(test_word),              WINCODEC_ERR_STREAMREAD },
    { test_dword,          sizeof(test_dword),             WINCODEC_ERR_UNKNOWNIMAGEFORMAT },
    { test_qword_a,        sizeof(test_qword_a),           WINCODEC_ERR_UNKNOWNIMAGEFORMAT },
    { test_qword_b,        sizeof(test_qword_b),           WINCODEC_ERR_STREAMREAD },
};

static DXGI_FORMAT compressed_formats[] = {
    DXGI_FORMAT_BC1_TYPELESS,  DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_BC1_UNORM_SRGB,
    DXGI_FORMAT_BC2_TYPELESS,  DXGI_FORMAT_BC2_UNORM, DXGI_FORMAT_BC2_UNORM_SRGB,
    DXGI_FORMAT_BC3_TYPELESS,  DXGI_FORMAT_BC3_UNORM, DXGI_FORMAT_BC3_UNORM_SRGB,
    DXGI_FORMAT_BC4_TYPELESS,  DXGI_FORMAT_BC4_UNORM, DXGI_FORMAT_BC4_SNORM,
    DXGI_FORMAT_BC5_TYPELESS,  DXGI_FORMAT_BC5_UNORM, DXGI_FORMAT_BC5_SNORM,
    DXGI_FORMAT_BC6H_TYPELESS, DXGI_FORMAT_BC6H_UF16, DXGI_FORMAT_BC6H_SF16,
    DXGI_FORMAT_BC7_TYPELESS,  DXGI_FORMAT_BC7_UNORM, DXGI_FORMAT_BC7_UNORM_SRGB
};

static IWICImagingFactory *factory = NULL;

static IWICStream *create_stream(const void *image_data, UINT image_size)
{
    HRESULT hr;
    IWICStream *stream = NULL;

    hr = IWICImagingFactory_CreateStream(factory, &stream);
    ok(hr == S_OK, "CreateStream failed, hr %#lx\n", hr);
    if (hr != S_OK) goto fail;

    hr = IWICStream_InitializeFromMemory(stream, (BYTE *)image_data, image_size);
    ok(hr == S_OK, "InitializeFromMemory failed, hr %#lx\n", hr);
    if (hr != S_OK) goto fail;

    return stream;

fail:
    if (stream) IWICStream_Release(stream);
    return NULL;
}

static IWICBitmapDecoder *create_decoder(void)
{
    HRESULT hr;
    IWICBitmapDecoder *decoder = NULL;
    GUID guidresult;

    hr = CoCreateInstance(&CLSID_WICDdsDecoder, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICBitmapDecoder, (void **)&decoder);
    if (hr != S_OK) {
        win_skip("Dds decoder is not supported\n");
        return NULL;
    }

    memset(&guidresult, 0, sizeof(guidresult));
    hr = IWICBitmapDecoder_GetContainerFormat(decoder, &guidresult);
    ok(hr == S_OK, "GetContainerFormat failed, hr %#lx\n", hr);
    ok(IsEqualGUID(&guidresult, &GUID_ContainerFormatDds),
       "Got unexpected container format %s\n", debugstr_guid(&guidresult));

    return decoder;
}

static IWICBitmapEncoder *create_encoder(void)
{
    IWICBitmapEncoder *encoder = NULL;
    GUID guidresult;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_WICDdsEncoder, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICBitmapEncoder, (void **)&encoder);
    if (hr != S_OK)
    {
        win_skip("DDS encoder is not supported\n");
        return NULL;
    }

    memset(&guidresult, 0, sizeof(guidresult));

    hr = IWICBitmapEncoder_GetContainerFormat(encoder, &guidresult);
    ok(hr == S_OK, "GetContainerFormat failed, hr %#lx\n", hr);

    ok(IsEqualGUID(&guidresult, &GUID_ContainerFormatDds),
       "Got unexpected container format %s\n", debugstr_guid(&guidresult));

    return encoder;
}

static HRESULT init_decoder(IWICBitmapDecoder *decoder, IWICStream *stream, HRESULT expected, BOOL wine_init)
{
    HRESULT hr;
    IWICWineDecoder *wine_decoder;

    hr = IWICBitmapDecoder_Initialize(decoder, (IStream*)stream, WICDecodeMetadataCacheOnDemand);
    ok(hr == expected, "Expected hr %#lx, got %#lx\n", expected, hr);

    if (hr != S_OK && wine_init) {
        hr = IWICBitmapDecoder_QueryInterface(decoder, &IID_IWICWineDecoder, (void **)&wine_decoder);
        ok(hr == S_OK || broken(hr != S_OK), "QueryInterface failed, hr %#lx\n", hr);

        if (hr == S_OK) {
            hr = IWICWineDecoder_Initialize(wine_decoder, (IStream*)stream, WICDecodeMetadataCacheOnDemand);
            ok(hr == S_OK, "Initialize failed, hr %#lx\n", hr);
        }
    }

    return hr;
}

static void release_encoder(IWICBitmapEncoder *encoder, IWICDdsEncoder *dds_encoder, IWICStream *stream)
{
    if (dds_encoder) IWICDdsEncoder_Release(dds_encoder);
    if (stream) IWICStream_Release(stream);
    if (encoder) IWICBitmapEncoder_Release(encoder);
}

static HRESULT create_and_init_encoder(BYTE *image_buffer, UINT buffer_size, WICDdsParameters *params,
                                       IWICBitmapEncoder **encoder, IWICDdsEncoder **dds_encoder, IWICStream **stream)
{
    IWICDdsEncoder *dds = NULL;
    HRESULT hr;

    *encoder = create_encoder();
    if (!*encoder) goto fail;

    *stream = create_stream(image_buffer, buffer_size);
    if (!*stream) goto fail;

    hr = IWICBitmapEncoder_Initialize(*encoder, (IStream *)*stream, WICBitmapEncoderNoCache);
    ok(hr == S_OK, "Initialize failed, hr %#lx\n", hr);
    if (hr != S_OK) goto fail;

    hr = IWICBitmapEncoder_QueryInterface(*encoder, &IID_IWICDdsEncoder, (void **)&dds);
    ok(hr == S_OK, "QueryInterface failed, hr %#lx\n", hr);
    if (hr != S_OK) goto fail;

    if (params)
    {
        hr = IWICDdsEncoder_SetParameters(dds, params);
        ok(hr == S_OK, "SetParameters failed, hr %#lx\n", hr);
        if (hr != S_OK) goto fail;
    }

    if (dds_encoder)
    {
        *dds_encoder = dds;
    }
    else
    {
        IWICDdsEncoder_Release(dds);
        dds = NULL;
    }

    return S_OK;

fail:
    release_encoder(*encoder, dds, *stream);
    return E_FAIL;
}

static BOOL is_compressed(DXGI_FORMAT format)
{
    UINT i;
    for (i = 0; i < ARRAY_SIZE(compressed_formats); i++)
    {
        if (format == compressed_formats[i]) return TRUE;
    }
    return FALSE;
}

static BOOL has_extended_header(const BYTE *data)
{
    return data[84] == 'D' && data[85] == 'X' && data[86] == '1' && data[87] == '0';
}

static DWORD rgb565_to_argb(WORD color, BYTE alpha)
{
    return MAKE_ARGB(alpha, (GET_RGB565_R(color) * 0xFF + 0x0F) / 0x1F,
                            (GET_RGB565_G(color) * 0xFF + 0x1F) / 0x3F,
                            (GET_RGB565_B(color) * 0xFF + 0x0F) / 0x1F);
}

static void decode_block(const BYTE *block_data, UINT block_count, DXGI_FORMAT format,
                         UINT width, UINT height, DWORD *buffer)
{
    const BYTE *block, *color_indices, *alpha_indices, *alpha_table;
    int i, j, x, y, block_x, block_y, color_index, alpha_index;
    int block_size, color_offset, color_indices_offset;
    WORD color[4], color_value = 0;
    BYTE alpha[8], alpha_value = 0;

    if (format == DXGI_FORMAT_BC1_UNORM) {
        block_size = 8;
        color_offset = 0;
        color_indices_offset = 4;
    } else {
        block_size = 16;
        color_offset = 8;
        color_indices_offset = 12;
    }
    block_x = 0;
    block_y = 0;

    for (i = 0; i < block_count; i++)
    {
        block = block_data + i * block_size;

        color[0] = *((WORD *)(block + color_offset));
        color[1] = *((WORD *)(block + color_offset + 2));
        color[2] = MAKE_RGB565(((GET_RGB565_R(color[0]) * 2 + GET_RGB565_R(color[1]) + 1) / 3),
                               ((GET_RGB565_G(color[0]) * 2 + GET_RGB565_G(color[1]) + 1) / 3),
                               ((GET_RGB565_B(color[0]) * 2 + GET_RGB565_B(color[1]) + 1) / 3));
        color[3] = MAKE_RGB565(((GET_RGB565_R(color[0]) + GET_RGB565_R(color[1]) * 2 + 1) / 3),
                               ((GET_RGB565_G(color[0]) + GET_RGB565_G(color[1]) * 2 + 1) / 3),
                               ((GET_RGB565_B(color[0]) + GET_RGB565_B(color[1]) * 2 + 1) / 3));

        switch (format)
        {
            case DXGI_FORMAT_BC1_UNORM:
                if (color[0] <= color[1]) {
                    color[2] = MAKE_RGB565(((GET_RGB565_R(color[0]) + GET_RGB565_R(color[1]) + 1) / 2),
                                           ((GET_RGB565_G(color[0]) + GET_RGB565_G(color[1]) + 1) / 2),
                                           ((GET_RGB565_B(color[0]) + GET_RGB565_B(color[1]) + 1) / 2));
                    color[3] = 0;
                }
                break;
            case DXGI_FORMAT_BC2_UNORM:
                alpha_table = block;
                break;
            case DXGI_FORMAT_BC3_UNORM:
                alpha[0] = *block;
                alpha[1] = *(block + 1);
                if (alpha[0] > alpha[1]) {
                    for (j = 2; j < 8; j++)
                    {
                        alpha[j] = (BYTE)((alpha[0] * (8 - j) + alpha[1] * (j - 1) + 3) / 7);
                    }
                } else {
                    for (j = 2; j < 6; j++)
                    {
                        alpha[j] = (BYTE)((alpha[0] * (6 - j) + alpha[1] * (j - 1) + 2) / 5);
                    }
                    alpha[6] = 0;
                    alpha[7] = 0xFF;
                }
                alpha_indices = block + 2;
                break;
            default:
                break;
        }

        color_indices = block + color_indices_offset;
        for (j = 0; j < 16; j++)
        {
            x = block_x + j % 4;
            y = block_y + j / 4;
            if (x >= width || y >= height) continue;

            color_index = (color_indices[j / 4] >> ((j % 4) * 2)) & 0x3;
            color_value = color[color_index];

            switch (format)
            {
                case DXGI_FORMAT_BC1_UNORM:
                    if ((color[0] <= color[1]) && !color_value) {
                        color_value = 0;
                        alpha_value = 0;
                    } else {
                        alpha_value = 0xFF;
                    }
                    break;
                case DXGI_FORMAT_BC2_UNORM:
                    alpha_value = (alpha_table[j / 2] >> (j % 2) * 4) & 0xF;
                    alpha_value = (BYTE)((alpha_value * 0xFF + 0x7)/ 0xF);
                    break;
                case DXGI_FORMAT_BC3_UNORM:
                    alpha_index = (*((DWORD *)(alpha_indices + (j / 8) * 3)) >> ((j % 8) * 3)) & 0x7;
                    alpha_value = alpha[alpha_index];
                    break;
                default:
                    break;
            }
            buffer[x + y * width] = rgb565_to_argb(color_value, alpha_value);
        }

        block_x += BLOCK_WIDTH;
        if (block_x >= width) {
            block_x = 0;
            block_y += BLOCK_HEIGHT;
        }
    }
}

static BOOL color_match(DWORD color_a, DWORD color_b)
{
    static const int tolerance = 8;

    const int da = abs((int)((color_a & 0xFF000000) >> 24) - (int)((color_b & 0xFF000000) >> 24));
    const int dr = abs((int)((color_a & 0x00FF0000) >> 16) - (int)((color_b & 0x00FF0000) >> 16));
    const int dg = abs((int)((color_a & 0x0000FF00) >> 8)  - (int)((color_b & 0x0000FF00) >> 8));
    const int db = abs((int)((color_a & 0x000000FF) >> 0)  - (int)((color_b & 0x000000FF) >> 0));

    return (da <= tolerance && dr <= tolerance && dg <= tolerance && db <= tolerance);
}

static BOOL color_buffer_match(DWORD *color_buffer_a, DWORD *color_buffer_b, UINT color_count)
{
    UINT i;

    for (i = 0; i < color_count; i++)
    {
        if (!color_match(color_buffer_a[i], color_buffer_b[i])) return FALSE;
    }

    return TRUE;
}

static void copy_pixels(void *src_buffer, UINT src_stride, void *dst_buffer, UINT dst_stride, UINT size)
{
    char *src = src_buffer, *dst = dst_buffer;
    UINT i;

    for (i = 0; i < size; i++)
    {
        *dst = src[i];
        if (i % src_stride == src_stride - 1) dst += dst_stride - src_stride;
        dst ++;
    }
}

static void test_dds_decoder_initialize(void)
{
    int i;

    memcpy(test_dds_bad_magic, test_dds_dxt1c, sizeof(test_dds_dxt1c));
    memcpy(test_dds_bad_header, test_dds_dxt1c, sizeof(test_dds_dxt1c));
    test_dds_bad_magic[0] = 0;
    test_dds_bad_header[4] = 0;

    for (i = 0; i < ARRAY_SIZE(test_data); i++)
    {
        IWICStream *stream = NULL;
        IWICBitmapDecoder *decoder = NULL;

        winetest_push_context("Test %u", i);

        stream = create_stream(test_data[i].data, test_data[i].size);
        if (!stream) goto next;

        decoder = create_decoder();
        if (!decoder) goto next;

        init_decoder(decoder, stream, test_data[i].init_hr, test_data[i].wine_init);

    next:
        if (decoder) IWICBitmapDecoder_Release(decoder);
        if (stream) IWICStream_Release(stream);
        winetest_pop_context();
    }
}

static void test_dds_decoder_global_properties(IWICBitmapDecoder *decoder)
{
    HRESULT hr;
    IWICPalette *palette = NULL;
    IWICMetadataQueryReader *metadata_reader = NULL;
    IWICBitmapSource *preview = NULL, *thumnail = NULL;
    IWICColorContext *color_context = NULL;
    UINT count;

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette failed, hr %#lx\n", hr);
    if (hr == S_OK) {
        hr = IWICBitmapDecoder_CopyPalette(decoder, palette);
        ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "CopyPalette got unexpected hr %#lx\n", hr);
        hr = IWICBitmapDecoder_CopyPalette(decoder, NULL);
        ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "CopyPalette got unexpected hr %#lx\n", hr);
    }

    hr = IWICBitmapDecoder_GetMetadataQueryReader(decoder, &metadata_reader);
    todo_wine ok (hr == S_OK, "GetMetadataQueryReader got unexpected hr %#lx\n", hr);
    hr = IWICBitmapDecoder_GetMetadataQueryReader(decoder, NULL);
    ok(hr == E_INVALIDARG, "GetMetadataQueryReader got unexpected hr %#lx\n", hr);

    hr = IWICBitmapDecoder_GetPreview(decoder, &preview);
    ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "GetPreview got unexpected hr %#lx\n", hr);
    hr = IWICBitmapDecoder_GetPreview(decoder, NULL);
    ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "GetPreview got unexpected hr %#lx\n", hr);

    hr = IWICBitmapDecoder_GetColorContexts(decoder, 1, &color_context, &count);
    ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "GetColorContexts got unexpected hr %#lx\n", hr);
    hr = IWICBitmapDecoder_GetColorContexts(decoder, 1, NULL, NULL);
    ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "GetColorContexts got unexpected hr %#lx\n", hr);

    hr = IWICBitmapDecoder_GetThumbnail(decoder, &thumnail);
    ok(hr == WINCODEC_ERR_CODECNOTHUMBNAIL, "GetThumbnail got unexpected hr %#lx\n", hr);
    hr = IWICBitmapDecoder_GetThumbnail(decoder, NULL);
    ok(hr == WINCODEC_ERR_CODECNOTHUMBNAIL, "GetThumbnail got unexpected hr %#lx\n", hr);

    if (palette) IWICPalette_Release(palette);
    if (metadata_reader) IWICMetadataQueryReader_Release(metadata_reader);
    if (preview) IWICBitmapSource_Release(preview);
    if (color_context) IWICColorContext_Release(color_context);
    if (thumnail) IWICBitmapSource_Release(thumnail);
}

static void test_dds_decoder_image_parameters(void)
{
    int i;
    HRESULT hr;
    WICDdsParameters parameters;

    for (i = 0; i < ARRAY_SIZE(test_data); i++)
    {
        UINT frame_count;
        IWICStream *stream = NULL;
        IWICBitmapDecoder *decoder = NULL;
        IWICDdsDecoder *dds_decoder = NULL;

        winetest_push_context("Test %u", i);

        stream = create_stream(test_data[i].data, test_data[i].size);
        if (!stream) goto next;

        decoder = create_decoder();
        if (!decoder) goto next;

        hr = IWICBitmapDecoder_QueryInterface(decoder, &IID_IWICDdsDecoder, (void **)&dds_decoder);
        ok(hr == S_OK, "QueryInterface failed, hr %#lx\n", hr);
        if (hr != S_OK) goto next;

        hr = IWICBitmapDecoder_GetFrameCount(decoder, &frame_count);
        ok(hr == WINCODEC_ERR_WRONGSTATE, "GetFrameCount got unexpected hr %#lx\n", hr);
        hr = IWICBitmapDecoder_GetFrameCount(decoder, NULL);
        ok(hr == E_INVALIDARG, "GetFrameCount got unexpected hr %#lx\n", hr);

        hr = IWICDdsDecoder_GetParameters(dds_decoder, &parameters);
        ok(hr == WINCODEC_ERR_WRONGSTATE, "GetParameters got unexpected hr %#lx\n", hr);
        hr = IWICDdsDecoder_GetParameters(dds_decoder, NULL);
        ok(hr == E_INVALIDARG, "GetParameters got unexpected hr %#lx\n", hr);

        if (test_data[i].init_hr != S_OK && !test_data[i].wine_init) goto next;

        hr = init_decoder(decoder, stream, test_data[i].init_hr, test_data[i].wine_init);
        if (hr != S_OK) {
            if (test_data[i].expected_parameters.Dimension == WICDdsTextureCube) {
                win_skip("Cube map is not supported\n");
            } else {
                win_skip("Uncompressed DDS image is not supported\n");
            }
            goto next;
        }

        hr = IWICBitmapDecoder_GetFrameCount(decoder, &frame_count);
        ok(hr == S_OK, "GetFrameCount failed, hr %#lx\n", hr);
        if (hr == S_OK) {
            ok(frame_count == test_data[i].expected_frame_count, "Expected frame count %u, got %u\n",
               test_data[i].expected_frame_count, frame_count);
        }
        hr = IWICBitmapDecoder_GetFrameCount(decoder, NULL);
        ok(hr == E_INVALIDARG, "GetParameters got unexpected hr %#lx\n", hr);

        hr = IWICDdsDecoder_GetParameters(dds_decoder, &parameters);
        ok(hr == S_OK, "GetParameters failed, hr %#lx\n", hr);
        if (hr == S_OK) {
            ok(parameters.Width == test_data[i].expected_parameters.Width,
               "Expected Width %u, got %u\n", test_data[i].expected_parameters.Width, parameters.Width);
            ok(parameters.Height == test_data[i].expected_parameters.Height,
               "Expected Height %u, got %u\n", test_data[i].expected_parameters.Height, parameters.Height);
            ok(parameters.Depth == test_data[i].expected_parameters.Depth,
               "Expected Depth %u, got %u\n", test_data[i].expected_parameters.Depth, parameters.Depth);
            ok(parameters.MipLevels == test_data[i].expected_parameters.MipLevels,
               "Expected MipLevels %u, got %u\n", test_data[i].expected_parameters.MipLevels, parameters.MipLevels);
            ok(parameters.ArraySize == test_data[i].expected_parameters.ArraySize,
               "Expected ArraySize %u, got %u\n", test_data[i].expected_parameters.ArraySize, parameters.ArraySize);
            ok(parameters.DxgiFormat == test_data[i].expected_parameters.DxgiFormat,
               "Expected DxgiFormat %#x, got %#x\n", test_data[i].expected_parameters.DxgiFormat, parameters.DxgiFormat);
            ok(parameters.Dimension == test_data[i].expected_parameters.Dimension,
               "Expected Dimension %#x, got %#x\n", test_data[i].expected_parameters.Dimension, parameters.Dimension);
            ok(parameters.AlphaMode == test_data[i].expected_parameters.AlphaMode,
               "Expected AlphaMode %#x, got %#x\n", test_data[i].expected_parameters.AlphaMode, parameters.AlphaMode);
        }
        hr = IWICDdsDecoder_GetParameters(dds_decoder, NULL);
        ok(hr == E_INVALIDARG, "GetParameters got unexpected hr %#lx\n", hr);

    next:
        if (decoder) IWICBitmapDecoder_Release(decoder);
        if (stream) IWICStream_Release(stream);
        if (dds_decoder) IWICDdsDecoder_Release(dds_decoder);
        winetest_pop_context();
    }
}

static void test_dds_decoder_frame_properties(IWICBitmapFrameDecode *frame_decode, IWICDdsFrameDecode *dds_frame,
                                              UINT frame_count, WICDdsParameters *params, struct test_data *test, UINT frame_index)
{
    HRESULT hr;
    UINT width, height ,expected_width, expected_height, slice_index, depth;
    UINT width_in_blocks, height_in_blocks, expected_width_in_blocks, expected_height_in_blocks;
    UINT expected_block_width, expected_block_height;
    WICDdsFormatInfo format_info;
    GUID pixel_format;

    /* frame size tests */

    hr = IWICBitmapFrameDecode_GetSize(frame_decode, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetSize got unexpected hr %#lx\n", hr);
    hr = IWICBitmapFrameDecode_GetSize(frame_decode, NULL, &height);
    ok(hr == E_INVALIDARG, "GetSize got unexpected hr %#lx\n", hr);
    hr = IWICBitmapFrameDecode_GetSize(frame_decode, &width, NULL);
    ok(hr == E_INVALIDARG, "GetSize got unexpected hr %#lx\n", hr);
    hr = IWICBitmapFrameDecode_GetSize(frame_decode, &width, &height);
    ok(hr == S_OK, "GetSize failed, hr %#lx\n", hr);
    if (hr != S_OK) return;

    depth = params->Depth;
    expected_width = params->Width;
    expected_height = params->Height;
    slice_index = frame_index % (frame_count / params->ArraySize);
    while (slice_index >= depth)
    {
        if (expected_width > 1) expected_width /= 2;
        if (expected_height > 1) expected_height /= 2;
        slice_index -= depth;
        if (depth > 1) depth /= 2;
    }
    ok(width == expected_width, "Expected width %u, got %u\n", expected_width, width);
    ok(height == expected_height, "Expected height %u, got %u\n", expected_height, height);

    /* frame format information tests */

    if (is_compressed(test->expected_parameters.DxgiFormat)) {
        expected_block_width = BLOCK_WIDTH;
        expected_block_height = BLOCK_HEIGHT;
    } else {
        expected_block_width = 1;
        expected_block_height = 1;
    }

    hr = IWICDdsFrameDecode_GetFormatInfo(dds_frame, NULL);
    ok(hr == E_INVALIDARG, "GetFormatInfo got unexpected hr %#lx\n", hr);
    hr = IWICDdsFrameDecode_GetFormatInfo(dds_frame, &format_info);
    ok(hr == S_OK, "GetFormatInfo failed, hr %#lx\n", hr);
    if (hr != S_OK) return;

    ok(format_info.DxgiFormat == test->expected_parameters.DxgiFormat,
       "Expected DXGI format %#x, got %#x\n",
       test->expected_parameters.DxgiFormat, format_info.DxgiFormat);
    ok(format_info.BytesPerBlock == test->expected_bytes_per_block,
       "Expected bytes per block %u, got %u\n",
       test->expected_bytes_per_block, format_info.BytesPerBlock);
    ok(format_info.BlockWidth == expected_block_width,
       "Expected block width %u, got %u\n",
       expected_block_width, format_info.BlockWidth);
    ok(format_info.BlockHeight == expected_block_height,
       "Expected block height %u, got %u\n",
       expected_block_height, format_info.BlockHeight);


    /* size in blocks tests */

    hr = IWICDdsFrameDecode_GetSizeInBlocks(dds_frame, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetSizeInBlocks got unexpected hr %#lx\n", hr);
    hr = IWICDdsFrameDecode_GetSizeInBlocks(dds_frame, NULL, &height_in_blocks);
    ok(hr == E_INVALIDARG, "GetSizeInBlocks got unexpected hr %#lx\n", hr);
    hr = IWICDdsFrameDecode_GetSizeInBlocks(dds_frame, &width_in_blocks, NULL);
    ok(hr == E_INVALIDARG, "GetSizeInBlocks got unexpected hr %#lx\n", hr);
    hr = IWICDdsFrameDecode_GetSizeInBlocks(dds_frame, &width_in_blocks, &height_in_blocks);
    ok(hr == S_OK, "GetSizeInBlocks failed, hr %#lx\n", hr);
    if (hr != S_OK) return;

    expected_width_in_blocks = (expected_width + expected_block_width - 1) / expected_block_width;
    expected_height_in_blocks = (expected_height + expected_block_height - 1) / expected_block_height;
    ok(width_in_blocks == expected_width_in_blocks,
       "Expected width in blocks %u, got %u\n", expected_width_in_blocks, width_in_blocks);
    ok(height_in_blocks == expected_height_in_blocks,
       "Expected height in blocks %u, got %u\n", expected_height_in_blocks, height_in_blocks);

    /* pixel format tests */

    hr = IWICBitmapFrameDecode_GetPixelFormat(frame_decode, NULL);
    ok(hr == E_INVALIDARG, "GetPixelFormat got unexpected hr %#lx\n", hr);
    hr = IWICBitmapFrameDecode_GetPixelFormat(frame_decode, &pixel_format);
    ok(hr == S_OK, "GetPixelFormat failed, hr %#lx\n", hr);
    if (hr != S_OK) return;
    ok(IsEqualGUID(&pixel_format, test->expected_pixel_format),
       "Expected pixel format %s, got %s\n",
       debugstr_guid(test->expected_pixel_format), debugstr_guid(&pixel_format));
}

static void test_dds_decoder_frame_data(IWICBitmapFrameDecode* frame, IWICDdsFrameDecode *dds_frame, UINT frame_count,
                                        WICDdsParameters *params, struct test_data *test, UINT frame_index)
{
    HRESULT hr;
    GUID pixel_format;
    WICDdsFormatInfo format_info;
    WICRect rect = { 0, 0, 1, 1 }, rect_test_a = { 0, 0, 0, 0 }, rect_test_b = { 0, 0, 0xdeadbeaf, 0xdeadbeaf };
    WICRect rect_test_c = { -0xdeadbeaf, -0xdeadbeaf, 1, 1 }, rect_test_d = { 0xdeadbeaf, 0xdeadbeaf, 1, 1 };
    BYTE buffer[2048], pixels[2048];
    UINT stride, frame_stride, frame_size, frame_width, frame_height, width_in_blocks, height_in_blocks, bpp;
    UINT width, height, depth, array_index;
    UINT block_offset;
    int slice_index;

    hr = IWICBitmapFrameDecode_GetPixelFormat(frame, &pixel_format);
    ok(hr == S_OK, "GetPixelFormat failed, hr %#lx\n", hr);
    if (hr != S_OK) return;
    hr = IWICBitmapFrameDecode_GetSize(frame, &frame_width, &frame_height);
    ok(hr == S_OK, "GetSize failed, hr %#lx\n", hr);
    if (hr != S_OK) return;
    hr = IWICDdsFrameDecode_GetFormatInfo(dds_frame, &format_info);
    ok(hr == S_OK, "GetFormatInfo failed, hr %#lx\n", hr);
    if (hr != S_OK) return;
    hr = IWICDdsFrameDecode_GetSizeInBlocks(dds_frame, &width_in_blocks, &height_in_blocks);
    ok(hr == S_OK, "GetSizeInBlocks failed, hr %#lx\n", hr);
    if (hr != S_OK) return;
    stride = rect.Width * format_info.BytesPerBlock;
    frame_stride = width_in_blocks * format_info.BytesPerBlock;
    frame_size = frame_stride * height_in_blocks;

    /* CopyBlocks tests */

    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, NULL, 0, 0, NULL);
    ok(hr == E_INVALIDARG, "CopyBlocks got unexpected hr %#lx\n", hr);

    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect_test_a, stride, sizeof(buffer), buffer);
    ok(hr == E_INVALIDARG, "CopyBlocks got unexpected hr %#lx\n", hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect_test_b, stride, sizeof(buffer), buffer);
    ok(hr == E_INVALIDARG, "CopyBlocks got unexpected hr %#lx\n", hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect_test_c, stride, sizeof(buffer), buffer);
    ok(hr == E_INVALIDARG, "CopyBlocks got unexpected hr %#lx\n", hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect_test_d, stride, sizeof(buffer), buffer);
    ok(hr == E_INVALIDARG, "CopyBlocks got unexpected hr %#lx\n", hr);

    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, NULL, frame_stride - 1, sizeof(buffer), buffer);
    ok(hr == E_INVALIDARG, "CopyBlocks got unexpected hr %#lx\n", hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, NULL, frame_stride * 2, sizeof(buffer), buffer);
    ok(hr == S_OK, "CopyBlocks got unexpected hr %#lx\n", hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, NULL, frame_stride, sizeof(buffer), buffer);
    ok(hr == S_OK, "CopyBlocks got unexpected hr %#lx\n", hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, NULL, frame_stride, frame_stride * height_in_blocks - 1, buffer);
    ok(hr == E_INVALIDARG, "CopyBlocks got unexpected hr %#lx\n", hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, NULL, frame_stride, frame_stride * height_in_blocks, buffer);
    ok(hr == S_OK, "CopyBlocks got unexpected hr %#lx\n", hr);

    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect, 0, sizeof(buffer), buffer);
    ok(hr == E_INVALIDARG, "CopyBlocks got unexpected hr %#lx\n", hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect, stride - 1, sizeof(buffer), buffer);
    ok(hr == E_INVALIDARG, "CopyBlocks got unexpected hr %#lx\n", hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect, stride * 2, sizeof(buffer), buffer);
    ok(hr == S_OK, "CopyBlocks got unexpected hr %#lx\n", hr);

    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect, stride, 0, buffer);
    ok(hr == E_INVALIDARG, "CopyBlocks got unexpected hr %#lx\n", hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect, stride, 1, buffer);
    ok(hr == E_INVALIDARG || (hr == S_OK && test->expected_bytes_per_block == 1),
       "CopyBlocks got unexpected hr %#lx\n", hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect, stride, stride * rect.Height - 1, buffer);
    ok(hr == E_INVALIDARG, "CopyBlocks got unexpected hr %#lx\n", hr);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect, stride, stride * rect.Height, buffer);
    ok(hr == S_OK, "CopyBlocks got unexpected hr %#lx\n", hr);

    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect, stride, sizeof(buffer), NULL);
    ok(hr == E_INVALIDARG, "CopyBlocks got unexpected hr %#lx\n", hr);

    block_offset = 128; /* DDS magic and header */
    if (has_extended_header(test->data)) block_offset += 20; /* DDS extended header */
    width = params->Width;
    height = params->Height;
    depth = params->Depth;
    slice_index = frame_index % (frame_count / params->ArraySize);
    array_index = frame_index / (frame_count / params->ArraySize);
    block_offset += (test->size - block_offset) / params->ArraySize * array_index;
    while (slice_index >= 0)
    {
        width_in_blocks = (width + format_info.BlockWidth - 1) / format_info.BlockWidth;
        height_in_blocks = (width + format_info.BlockWidth - 1) / format_info.BlockWidth;
        block_offset += (slice_index >= depth) ?
                        (width_in_blocks * height_in_blocks * format_info.BytesPerBlock * depth) :
                        (width_in_blocks * height_in_blocks * format_info.BytesPerBlock * slice_index);
        if (width > 1) width /= 2;
        if (height > 1) height /= 2;
        slice_index -= depth;
        if (depth > 1) depth /= 2;
    }

    memset(buffer, 0, sizeof(buffer));
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, &rect, stride, sizeof(buffer), buffer);
    ok(hr == S_OK, "CopyBlocks failed, hr %#lx\n", hr);
    if (hr != S_OK) return;
    ok(!memcmp(test->data + block_offset, buffer, format_info.BytesPerBlock),
       "Block data mismatch\n");

    memset(buffer, 0, sizeof(buffer));
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, NULL, frame_stride, sizeof(buffer), buffer);
    ok(hr == S_OK, "CopyBlocks failed, hr %#lx\n", hr);
    if (hr != S_OK) return;
    ok(!memcmp(test->data + block_offset, buffer, frame_size),
       "Block data mismatch\n");

    memset(buffer, 0, sizeof(buffer));
    memset(pixels, 0, sizeof(pixels));
    copy_pixels(test->data + block_offset, frame_stride, pixels, frame_stride * 2, frame_size);
    hr = IWICDdsFrameDecode_CopyBlocks(dds_frame, NULL, frame_stride * 2, sizeof(buffer), buffer);
    ok(hr == S_OK, "CopyBlocks failed, hr %#lx\n", hr);
    if (hr != S_OK) return;
    ok(!memcmp(pixels, buffer, frame_size),
       "Block data mismatch\n");

    /* CopyPixels tests */

    bpp = test->pixel_format_bpp;
    stride = rect.Width * bpp / 8;
    frame_stride = frame_width * bpp / 8;
    frame_size = frame_stride * frame_height;

    hr = IWICBitmapFrameDecode_CopyPixels(frame, NULL, 0, 0, NULL);
    ok(hr == E_INVALIDARG, "CopyPixels got unexpected hr %#lx\n", hr);

    hr = IWICBitmapFrameDecode_CopyPixels(frame, &rect_test_a, stride, sizeof(buffer), buffer);
    ok(hr == E_INVALIDARG, "CopyPixels got unexpected hr %#lx\n", hr);
    hr = IWICBitmapFrameDecode_CopyPixels(frame, &rect_test_b, stride, sizeof(buffer), buffer);
    ok(hr == E_INVALIDARG, "CopyPixels got unexpected hr %#lx\n", hr);
    hr = IWICBitmapFrameDecode_CopyPixels(frame, &rect_test_c, stride, sizeof(buffer), buffer);
    ok(hr == E_INVALIDARG, "CopyPixels got unexpected hr %#lx\n", hr);
    hr = IWICBitmapFrameDecode_CopyPixels(frame, &rect_test_d, stride, sizeof(buffer), buffer);
    ok(hr == E_INVALIDARG, "CopyPixels got unexpected hr %#lx\n", hr);

    hr = IWICBitmapFrameDecode_CopyPixels(frame, NULL, frame_stride - 1, sizeof(buffer), buffer);
    ok(hr == E_INVALIDARG, "CopyPixels got unexpected hr %#lx\n", hr);
    hr = IWICBitmapFrameDecode_CopyPixels(frame, NULL, frame_stride * 2, sizeof(buffer), buffer);
    ok(hr == S_OK, "CopyPixels got unexpected hr %#lx\n", hr);
    hr = IWICBitmapFrameDecode_CopyPixels(frame, NULL, frame_stride, sizeof(buffer), buffer);
    ok(hr == S_OK, "CopyPixels got unexpected hr %#lx\n", hr);
    hr = IWICBitmapFrameDecode_CopyPixels(frame, NULL, frame_stride, frame_stride * frame_height - 1, buffer);
    ok(hr == WINCODEC_ERR_INSUFFICIENTBUFFER, "CopyPixels got unexpected hr %#lx\n", hr);
    hr = IWICBitmapFrameDecode_CopyPixels(frame, NULL, frame_stride, frame_stride * frame_height, buffer);
    ok(hr == S_OK, "CopyPixels got unexpected hr %#lx\n", hr);

    hr = IWICBitmapFrameDecode_CopyPixels(frame, &rect, 0, sizeof(buffer), buffer);
    ok(hr == E_INVALIDARG, "CopyPixels got unexpected hr %#lx\n", hr);
    hr = IWICBitmapFrameDecode_CopyPixels(frame, &rect, stride - 1, sizeof(buffer), buffer);
    ok(hr == E_INVALIDARG, "CopyPixels got unexpected hr %#lx\n", hr);
    hr = IWICBitmapFrameDecode_CopyPixels(frame, &rect, stride * 2, sizeof(buffer), buffer);
    ok(hr == S_OK, "CopyPixels got unexpected hr %#lx\n", hr);

    hr = IWICBitmapFrameDecode_CopyPixels(frame, &rect, stride, 0, buffer);
    ok(hr == WINCODEC_ERR_INSUFFICIENTBUFFER, "CopyPixels got unexpected hr %#lx\n", hr);
    hr = IWICBitmapFrameDecode_CopyPixels(frame, &rect, stride, 1, buffer);
    ok(hr == WINCODEC_ERR_INSUFFICIENTBUFFER || (hr == S_OK && test->expected_bytes_per_block == 1),
       "CopyPixels got unexpected hr %#lx\n", hr);
    hr = IWICBitmapFrameDecode_CopyPixels(frame, &rect, stride, stride * rect.Height - 1, buffer);
    ok(hr == WINCODEC_ERR_INSUFFICIENTBUFFER, "CopyPixels got unexpected hr %#lx\n", hr);
    hr = IWICBitmapFrameDecode_CopyPixels(frame, &rect, stride, stride * rect.Height, buffer);
    ok(hr == S_OK, "CopyPixels got unexpected hr %#lx\n", hr);

    hr = IWICBitmapFrameDecode_CopyPixels(frame, &rect, stride, sizeof(buffer), NULL);
    ok(hr == E_INVALIDARG, "CopyBlocks got unexpected hr %#lx\n", hr);

    memset(buffer, 0, sizeof(pixels));
    if (is_compressed(format_info.DxgiFormat)) {
        decode_block(test->data + block_offset, width_in_blocks * height_in_blocks,
                     format_info.DxgiFormat, frame_width, frame_height, (DWORD *)pixels);
    } else {
        memcpy(pixels, test->data + block_offset, frame_size);
    }

    memset(buffer, 0, sizeof(buffer));
    hr = IWICBitmapFrameDecode_CopyPixels(frame, &rect, stride, sizeof(buffer), buffer);
    ok(hr == S_OK, "CopyPixels failed, hr %#lx\n", hr);
    if (hr == S_OK) {
        if (is_compressed(format_info.DxgiFormat)) {
            ok(color_buffer_match((DWORD *)pixels, (DWORD *)buffer, 1), "Pixels mismatch\n");
        } else {
            ok(!memcmp(pixels, buffer, bpp / 8), "Pixels mismatch\n");
        }
    }

    memset(buffer, 0, sizeof(buffer));
    hr = IWICBitmapFrameDecode_CopyPixels(frame, NULL, frame_stride, sizeof(buffer), buffer);
    ok(hr == S_OK, "CopyPixels failed, hr %#lx\n", hr);
    if (hr == S_OK) {
        if (is_compressed(format_info.DxgiFormat)) {
            ok(color_buffer_match((DWORD *)pixels, (DWORD *)buffer, frame_size / (bpp / 8)), "Pixels mismatch\n");
        } else {
            ok(!memcmp(pixels, buffer, frame_size), "Pixels mismatch\n");
        };
    }
}

static void test_dds_decoder_frame(IWICBitmapDecoder *decoder, struct test_data *test)
{
    HRESULT hr;
    IWICDdsDecoder *dds_decoder = NULL;
    UINT frame_count, j;
    WICDdsParameters params;

    hr = IWICBitmapDecoder_GetFrameCount(decoder, &frame_count);
    ok(hr == S_OK, "GetFrameCount failed, hr %#lx\n", hr);
    if (hr != S_OK) return;
    hr = IWICBitmapDecoder_QueryInterface(decoder, &IID_IWICDdsDecoder, (void **)&dds_decoder);
    ok(hr == S_OK, "QueryInterface failed, hr %#lx\n", hr);
    if (hr != S_OK) goto end;
    hr = IWICDdsDecoder_GetParameters(dds_decoder, &params);
    ok(hr == S_OK, "GetParameters failed, hr %#lx\n", hr);
    if (hr != S_OK) goto end;

    if (test->expected_parameters.Dimension == WICDdsTextureCube) params.ArraySize *= 6;

    for (j = 0; j < frame_count; j++)
    {
        IWICBitmapFrameDecode *frame_decode = NULL;
        IWICDdsFrameDecode *dds_frame = NULL;

        winetest_push_context("Frame %u", j);

        hr = IWICBitmapDecoder_GetFrame(decoder, j, &frame_decode);
        ok(hr == S_OK, "GetFrame failed, hr %#lx\n", hr);
        if (hr != S_OK) goto next;
        hr = IWICBitmapFrameDecode_QueryInterface(frame_decode, &IID_IWICDdsFrameDecode, (void **)&dds_frame);
        ok(hr == S_OK, "QueryInterface failed, hr %#lx\n", hr);
        if (hr != S_OK) goto next;

        test_dds_decoder_frame_properties(frame_decode, dds_frame, frame_count, &params, test, j);
        test_dds_decoder_frame_data(frame_decode, dds_frame, frame_count, &params, test, j);

    next:
        if (frame_decode) IWICBitmapFrameDecode_Release(frame_decode);
        if (dds_frame) IWICDdsFrameDecode_Release(dds_frame);
        winetest_pop_context();
    }

end:
    if (dds_decoder) IWICDdsDecoder_Release(dds_decoder);
}

static void test_dds_decoder(void)
{
    int i;
    HRESULT hr;

    test_dds_decoder_initialize();
    test_dds_decoder_image_parameters();

    for (i = 0; i < ARRAY_SIZE(test_data); i++)
    {
        IWICStream *stream = NULL;
        IWICBitmapDecoder *decoder = NULL;

        if (test_data[i].init_hr != S_OK && !test_data[i].wine_init) continue;

        winetest_push_context("Test %u", i);

        stream = create_stream(test_data[i].data, test_data[i].size);
        if (!stream) goto next;
        decoder = create_decoder();
        if (!decoder) goto next;
        hr = init_decoder(decoder, stream, test_data[i].init_hr, test_data[i].wine_init);
        if (hr != S_OK) {
            if (test_data[i].expected_parameters.Dimension == WICDdsTextureCube) {
                win_skip("Cube map is not supported\n");
            } else {
                win_skip("Uncompressed DDS image is not supported\n");
            }
            goto next;
        }

        test_dds_decoder_global_properties(decoder);
        test_dds_decoder_frame(decoder, test_data + i);

    next:
        if (decoder) IWICBitmapDecoder_Release(decoder);
        if (stream) IWICStream_Release(stream);
        winetest_pop_context();
    }
}

static void test_dds_encoder_initialize(void)
{
    IWICBitmapEncoder *encoder = NULL;
    IWICStream *stream = NULL;
    BYTE buffer[1];
    HRESULT hr;

    encoder = create_encoder();
    if (!encoder) goto end;

    stream = create_stream(buffer, sizeof(buffer));
    if (!stream) goto end;

    /* initialize with invalid cache option */

    hr = IWICBitmapEncoder_Initialize(encoder, (IStream *)stream, 0xdeadbeef);
    todo_wine
    ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "Initialize got unexpected hr %#lx\n", hr);

    hr = IWICBitmapEncoder_Initialize(encoder, (IStream *)stream, WICBitmapEncoderNoCache);
    todo_wine
    ok(hr == E_INVALIDARG, "Initialize got unexpected hr %#lx\n", hr);

    IWICBitmapEncoder_Release(encoder);

    /* initialize with null stream */

    encoder = create_encoder();
    if (!encoder) goto end;

    hr = IWICBitmapEncoder_Initialize(encoder, NULL, WICBitmapEncoderNoCache);
    ok(hr == E_INVALIDARG, "Initialize got unexpected hr %#lx\n", hr);

    hr = IWICBitmapEncoder_Initialize(encoder, (IStream *)stream, WICBitmapEncoderNoCache);
    ok(hr == S_OK, "Initialize failed, hr %#lx\n", hr);

    IWICBitmapEncoder_Release(encoder);

    /* regularly initialize */

    encoder = create_encoder();
    if (!encoder) goto end;

    hr = IWICBitmapEncoder_Initialize(encoder, (IStream *)stream, WICBitmapEncoderNoCache);
    ok(hr == S_OK, "Initialize failed, hr %#lx\n", hr);

    hr = IWICBitmapEncoder_Initialize(encoder, (IStream *)stream, WICBitmapEncoderNoCache);
    ok(hr == WINCODEC_ERR_WRONGSTATE, "Initialize got unexpected hr %#lx\n", hr);

end:
    if (stream) IWICStream_Release(stream);
    if (encoder) IWICBitmapEncoder_Release(encoder);
}

static void test_dds_encoder_params(void)
{
    WICDdsParameters params, params_set = { 4, 4, 4, 3, 1,   DXGI_FORMAT_BC1_UNORM,
                                            WICDdsTexture3D, WICDdsAlphaModePremultiplied };
    IWICDdsEncoder *dds_encoder = NULL;
    IWICBitmapEncoder *encoder = NULL;
    IWICStream *stream = NULL;
    BYTE buffer[1024];
    HRESULT hr;
    UINT i;

    hr = create_and_init_encoder(buffer, sizeof(buffer), NULL, &encoder, &dds_encoder, &stream);
    if (hr != S_OK) goto end;

    hr = IWICDdsEncoder_GetParameters(dds_encoder, NULL);
    ok(hr == E_INVALIDARG, "GetParameters got unexpected hr %#lx\n", hr);

    hr = IWICDdsEncoder_GetParameters(dds_encoder, &params);
    ok(hr == S_OK, "GetParameters failed, hr %#lx\n", hr);
    if (hr != S_OK) goto end;

    /* default DDS parameters for encoder */
    ok(params.Width      == 1, "Got unexpected Width %u\n",     params.Width);
    ok(params.Height     == 1, "Got unexpected Height %u\n",    params.Height);
    ok(params.Depth      == 1, "Got unexpected Depth %u\n",     params.Depth);
    ok(params.MipLevels  == 1, "Got unexpected MipLevels %u\n", params.MipLevels);
    ok(params.ArraySize  == 1, "Got unexpected ArraySize %u\n", params.ArraySize);
    ok(params.DxgiFormat == DXGI_FORMAT_BC3_UNORM,  "Got unexpected DxgiFormat %#x\n", params.DxgiFormat);
    ok(params.Dimension  == WICDdsTexture2D,        "Got unexpected Dimension %#x\n",  params.Dimension);
    ok(params.AlphaMode  == WICDdsAlphaModeUnknown, "Got unexpected AlphaMode %#x\n",  params.AlphaMode);

    hr = IWICDdsEncoder_SetParameters(dds_encoder, NULL);
    ok(hr == E_INVALIDARG, "SetParameters got unexpected hr %#lx\n", hr);

    hr = IWICDdsEncoder_SetParameters(dds_encoder, &params_set);
    ok(hr == S_OK, "SetParameters failed, hr %#lx\n", hr);
    if (hr != S_OK) goto end;

    IWICDdsEncoder_GetParameters(dds_encoder, &params);

    ok(params.Width == params_set.Width,
       "Expected Width %u, got %u\n",       params_set.Width,      params.Width);
    ok(params.Height == params_set.Height,
       "Expected Height %u, got %u\n",      params_set.Height,     params.Height);
    ok(params.Depth == params_set.Depth,
       "Expected Depth %u, got %u\n",       params_set.Depth,      params.Depth);
    ok(params.MipLevels == params_set.MipLevels,
       "Expected MipLevels %u, got %u\n",   params_set.MipLevels,  params.MipLevels);
    ok(params.ArraySize == params_set.ArraySize,
       "Expected ArraySize %u, got %u\n",   params_set.ArraySize,  params.ArraySize);
    ok(params.DxgiFormat == params_set.DxgiFormat,
       "Expected DxgiFormat %u, got %#x\n", params_set.DxgiFormat, params.DxgiFormat);
    ok(params.Dimension == params_set.Dimension,
       "Expected Dimension %u, got %#x\n",  params_set.Dimension,  params.Dimension);
    ok(params.AlphaMode == params_set.AlphaMode,
       "Expected AlphaMode %u, got %#x\n",  params_set.AlphaMode,  params.AlphaMode);

    for (i = 0; i < ARRAY_SIZE(test_data); ++i)
    {
        hr = IWICDdsEncoder_SetParameters(dds_encoder, &test_data[i].expected_parameters);
        todo_wine_if(test_data[i].init_hr != S_OK)
        ok((hr == S_OK && test_data[i].init_hr == S_OK) || hr == WINCODEC_ERR_BADHEADER,
           "Test %u: SetParameters got unexpected hr %#lx\n", i, hr);
    }

end:
    release_encoder(encoder, dds_encoder, stream);
}

static void test_dds_encoder_create_frame(void)
{
    WICDdsParameters params = { 4, 4, 1, 3, 1,   DXGI_FORMAT_BC1_UNORM,
                                WICDdsTexture2D, WICDdsAlphaModePremultiplied };
    IWICBitmapFrameEncode *frame0 = NULL, *frame1 = NULL;
    UINT array_index, mip_level, slice_index;
    IWICDdsEncoder *dds_encoder = NULL;
    IWICBitmapEncoder *encoder = NULL;
    IWICStream *stream = NULL;
    BYTE buffer[1024];
    HRESULT hr;

    hr = create_and_init_encoder(buffer, sizeof(buffer), &params, &encoder, &dds_encoder, &stream);
    if (hr != S_OK) goto end;

    hr = IWICBitmapEncoder_CreateNewFrame(encoder, &frame0, NULL);
    ok(hr == S_OK, "CreateNewFrame failed, hr %#lx\n", hr);
    hr = IWICBitmapEncoder_CreateNewFrame(encoder, &frame1, NULL);
    ok(hr == WINCODEC_ERR_WRONGSTATE, "CreateNewFrame got unexpected hr %#lx\n", hr);

    IWICBitmapFrameEncode_Release(frame0);
    hr = IWICBitmapEncoder_CreateNewFrame(encoder, &frame1, NULL);
    ok(hr == WINCODEC_ERR_WRONGSTATE, "CreateNewFrame got unexpected hr %#lx\n", hr);

    release_encoder(encoder, dds_encoder, stream);

    create_and_init_encoder(buffer, sizeof(buffer), &params, &encoder, &dds_encoder, &stream);
    hr = IWICDdsEncoder_CreateNewFrame(dds_encoder, &frame0, &array_index, &mip_level, &slice_index);
    ok(hr == S_OK, "CreateNewFrame failed, hr %#lx\n", hr);
    IWICBitmapFrameEncode_Release(frame0);
    release_encoder(encoder, dds_encoder, stream);

    create_and_init_encoder(buffer, sizeof(buffer), &params, &encoder, &dds_encoder, &stream);
    hr = IWICDdsEncoder_CreateNewFrame(dds_encoder, &frame0, NULL, NULL, NULL);
    ok(hr == S_OK, "CreateNewFrame failed, hr %#lx\n", hr);
    IWICBitmapFrameEncode_Release(frame0);

end:
    release_encoder(encoder, dds_encoder, stream);
}

static void test_dds_encoder_pixel_format(void)
{
    DXGI_FORMAT image_formats[] = { DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_BC2_UNORM, DXGI_FORMAT_BC3_UNORM };
    const WICPixelFormatGUID *test_formats[] =
    {
        &GUID_WICPixelFormat8bppIndexed,
        &GUID_WICPixelFormatBlackWhite,
        &GUID_WICPixelFormat16bppGray,
        &GUID_WICPixelFormat8bppAlpha,
        &GUID_WICPixelFormat16bppBGR555,
        &GUID_WICPixelFormat16bppBGR565,
        &GUID_WICPixelFormat24bppBGR,
        &GUID_WICPixelFormat32bppBGR,
        &GUID_WICPixelFormat32bppBGRA,
        &GUID_WICPixelFormat32bppPBGRA,
        &GUID_WICPixelFormat32bppRGB,
        &GUID_WICPixelFormat32bppRGBA,
        &GUID_WICPixelFormat32bppPRGBA,
        &GUID_WICPixelFormat48bppRGB,
        &GUID_WICPixelFormat64bppRGB,
        &GUID_WICPixelFormat64bppRGBA
    };
    IWICBitmapFrameEncode *frame = NULL;
    IWICDdsEncoder *dds_encoder = NULL;
    IWICBitmapEncoder *encoder = NULL;
    IWICStream *stream = NULL;
    WICPixelFormatGUID format;
    WICDdsParameters params;
    BYTE buffer[1];
    HRESULT hr;
    UINT i, j;

    for (i = 0; i < ARRAY_SIZE(image_formats); ++i)
    {
        hr = create_and_init_encoder(buffer, sizeof(buffer), NULL, &encoder, &dds_encoder, &stream);
        if (hr != S_OK)
        {
            release_encoder(encoder, dds_encoder, stream);
            return;
        }

        IWICDdsEncoder_GetParameters(dds_encoder, &params);
        params.DxgiFormat = image_formats[i];
        IWICDdsEncoder_SetParameters(dds_encoder, &params);

        IWICBitmapEncoder_CreateNewFrame(encoder, &frame, NULL);

        hr = IWICBitmapFrameEncode_SetPixelFormat(frame, &format);
        ok(hr == WINCODEC_ERR_NOTINITIALIZED, "SetPixelFormat got unexpected hr %#lx\n", hr);

        IWICBitmapFrameEncode_Initialize(frame, NULL);

        for (j = 0; j < ARRAY_SIZE(test_formats); ++j)
        {
            winetest_push_context("Test %u", j);

            format = *(test_formats[j]);
            hr = IWICBitmapFrameEncode_SetPixelFormat(frame, &format);
            ok(hr == S_OK, "SetPixelFormat failed, hr %#lx\n", hr);
            ok(IsEqualGUID(&format, &GUID_WICPixelFormat32bppBGRA),
               "Got unexpected GUID %s\n", debugstr_guid(&format));

            winetest_pop_context();
        }

        IWICBitmapFrameEncode_Release(frame);
        release_encoder(encoder, dds_encoder, stream);
    }
}

static void test_dds_encoder(void)
{
    test_dds_encoder_initialize();
    test_dds_encoder_params();
    test_dds_encoder_create_frame();
    test_dds_encoder_pixel_format();
}

START_TEST(ddsformat)
{
    HRESULT hr;
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICImagingFactory, (void **)&factory);
    ok(hr == S_OK, "CoCreateInstance failed, hr %#lx\n", hr);
    if (hr != S_OK) goto end;

    test_dds_decoder();
    test_dds_encoder();

end:
    if (factory) IWICImagingFactory_Release(factory);
    CoUninitialize();
}
