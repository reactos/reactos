/*
 * Tests for the D3DX9 surface functions
 *
 * Copyright 2009 Tony Wasserka
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

#define COBJMACROS
#include <assert.h>
#include "wine/test.h"
#include "d3dx9tex.h"
#include "resources.h"

#define check_release(obj, exp) _check_release(__LINE__, obj, exp)
static inline void _check_release(unsigned int line, IUnknown *obj, int exp)
{
    int ref = IUnknown_Release(obj);
    ok_(__FILE__, line)(ref == exp, "Invalid refcount. Expected %d, got %d\n", exp, ref);
}

/* 1x1 bmp (1 bpp) */
static const unsigned char bmp_1bpp[] = {
0x42,0x4d,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,
0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,
0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
0x00,0x00,0x02,0x00,0x00,0x00,0xf1,0xf2,0xf3,0x80,0xf4,0xf5,0xf6,0x81,0x00,0x00,
0x00,0x00
};

/* 1x1 bmp (2 bpp) */
static const unsigned char bmp_2bpp[] = {
0x42,0x4d,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,
0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x02,0x00,0x00,0x00,
0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
0x00,0x00,0x02,0x00,0x00,0x00,0xf1,0xf2,0xf3,0x80,0xf4,0xf5,0xf6,0x81,0x00,0x00,
0x00,0x00
};

/* 1x1 bmp (4 bpp) */
static const unsigned char bmp_4bpp[] = {
0x42,0x4d,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,
0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x04,0x00,0x00,0x00,
0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
0x00,0x00,0x02,0x00,0x00,0x00,0xf1,0xf2,0xf3,0x80,0xf4,0xf5,0xf6,0x81,0x00,0x00,
0x00,0x00
};

/* 1x1 bmp (8 bpp) */
static const unsigned char bmp_8bpp[] = {
0x42,0x4d,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,
0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x08,0x00,0x00,0x00,
0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
0x00,0x00,0x02,0x00,0x00,0x00,0xf1,0xf2,0xf3,0x80,0xf4,0xf5,0xf6,0x81,0x00,0x00,
0x00,0x00
};

/* 2x2 bmp (32 bpp XRGB) */
static const unsigned char bmp_32bpp_xrgb[] =
{
    0x42,0x4d,0x46,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x02,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x01,0x00,0x20,0x00,0x00,0x00,
    0x00,0x00,0x10,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0xa0,0xb0,0xc0,0x00,0xa1,0xb1,0xc1,0x00,0xa2,0xb2,
    0xc2,0x00,0xa3,0xb3,0xc3,0x00
};

/* 2x2 bmp (32 bpp ARGB) */
static const unsigned char bmp_32bpp_argb[] =
{
    0x42,0x4d,0x46,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x02,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x01,0x00,0x20,0x00,0x00,0x00,
    0x00,0x00,0x10,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0xa0,0xb0,0xc0,0x00,0xa1,0xb1,0xc1,0x00,0xa2,0xb2,
    0xc2,0x00,0xa3,0xb3,0xc3,0x01
};

static const unsigned char png_grayscale[] =
{
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49,
    0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x00,
    0x00, 0x00, 0x00, 0x3a, 0x7e, 0x9b, 0x55, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44,
    0x41, 0x54, 0x08, 0xd7, 0x63, 0xf8, 0x0f, 0x00, 0x01, 0x01, 0x01, 0x00, 0x1b,
    0xb6, 0xee, 0x56, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42,
    0x60, 0x82
};

/* 2x2 A8R8G8B8 pixel data */
static const unsigned char pixdata[] = {
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};

/* invalid image file */
static const unsigned char noimage[4] = {
0x11,0x22,0x33,0x44
};

/* 16x4 8-bit dds  */
static const unsigned char dds_8bit[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x0f,0x10,0x00,0x00,0x04,0x00,0x00,0x00,
    0x10,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x47,0x49,0x4d,0x50,0x2d,0x44,0x44,0x53,0x5a,0x09,0x03,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0x8c,0xcd,0x12,0xff,
    0x78,0x01,0x14,0xff,0x50,0xcd,0x12,0xff,0x00,0x3d,0x8c,0xff,0x02,0x00,0x00,0xff,
    0x47,0x00,0x00,0xff,0xda,0x07,0x02,0xff,0x50,0xce,0x12,0xff,0xea,0x11,0x01,0xff,
    0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x08,0x3d,0x8c,0xff,0x08,0x01,0x00,0xff,
    0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x60,0xcc,0x12,0xff,
    0xa1,0xb2,0xd4,0xff,0xda,0x07,0x02,0xff,0x47,0x00,0x00,0xff,0x00,0x00,0x00,0xff,
    0x50,0xce,0x12,0xff,0x00,0x00,0x14,0xff,0xa8,0xcc,0x12,0xff,0x3c,0xb2,0xd4,0xff,
    0xda,0x07,0x02,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x01,0xff,
    0x21,0x00,0x00,0xff,0xd8,0xcb,0x12,0xff,0x54,0xcd,0x12,0xff,0x8b,0x4f,0xd5,0xff,
    0x00,0x04,0xda,0xff,0x00,0x00,0x00,0xff,0x3d,0x04,0x91,0xff,0x70,0xce,0x18,0xff,
    0xb4,0xcc,0x12,0xff,0x6b,0x4e,0xd5,0xff,0xb0,0xcc,0x12,0xff,0x00,0x00,0x00,0xff,
    0xc8,0x05,0x91,0xff,0x98,0xc7,0xcc,0xff,0x7c,0xcd,0x12,0xff,0x51,0x05,0x91,0xff,
    0x48,0x07,0x14,0xff,0x6d,0x05,0x91,0xff,0x00,0x07,0xda,0xff,0xa0,0xc7,0xcc,0xff,
    0x00,0x07,0xda,0xff,0x3a,0x77,0xd5,0xff,0xda,0x07,0x02,0xff,0x7c,0x94,0xd4,0xff,
    0xe0,0xce,0xd6,0xff,0x0a,0x80,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,
    0x78,0x9a,0xab,0xff,0xde,0x08,0x18,0xff,0xda,0x07,0x02,0xff,0x30,0x00,0x00,0xff,
    0x00,0x00,0x00,0xff,0x50,0xce,0x12,0xff,0x8c,0xcd,0x12,0xff,0xd0,0xb7,0xd8,0xff,
    0x00,0x00,0x00,0xff,0x60,0x32,0xd9,0xff,0x30,0xc1,0x1a,0xff,0xa8,0xcd,0x12,0xff,
    0xa4,0xcd,0x12,0xff,0xc0,0x1d,0x4b,0xff,0x46,0x71,0x0e,0xff,0xc0,0x1d,0x4b,0xff,
    0x09,0x87,0xd4,0xff,0x00,0x00,0x00,0xff,0xf6,0x22,0x00,0xff,0x64,0xcd,0x12,0xff,
    0x00,0x00,0x00,0xff,0xca,0x1d,0x4b,0xff,0x09,0x87,0xd4,0xff,0xaa,0x02,0x05,0xff,
    0x82,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0xc0,0x1d,0x4b,0xff,
    0xcd,0xab,0xba,0xff,0x00,0x00,0x00,0xff,0xa4,0xcd,0x12,0xff,0xc0,0x1d,0x4b,0xff,
    0xd4,0xcd,0x12,0xff,0xa6,0x4c,0xd5,0xff,0x00,0xf0,0xfd,0xff,0xd4,0xcd,0x12,0xff,
    0xf4,0x4c,0xd5,0xff,0x90,0xcd,0x12,0xff,0xc2,0x4c,0xd5,0xff,0x82,0x00,0x00,0xff,
    0xaa,0x02,0x05,0xff,0x88,0xd4,0xba,0xff,0x14,0x00,0x00,0xff,0x01,0x00,0x00,0xff,
    0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x10,0x00,0x00,0xff,0x00,0x00,0x00,0xff,
    0x0c,0x08,0x13,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,
    0xd0,0xcd,0x12,0xff,0xc6,0x84,0xf1,0xff,0x7c,0x84,0xf1,0xff,0x20,0x20,0xf5,0xff,
    0x00,0x00,0x0a,0xff,0xf0,0xb0,0x94,0xff,0x64,0x6c,0xf1,0xff,0x85,0x6c,0xf1,0xff,
    0x8b,0x4f,0xd5,0xff,0x00,0x04,0xda,0xff,0x88,0xd4,0xba,0xff,0x82,0x00,0x00,0xff,
    0x39,0xde,0xd4,0xff,0x10,0x50,0xd5,0xff,0xaa,0x02,0x05,0xff,0x00,0x00,0x00,0xff,
    0x4f,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x5c,0xce,0x12,0xff,0x00,0x00,0x00,0xff,
    0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x5c,0xce,0x12,0xff,
    0xaa,0x02,0x05,0xff,0x4c,0xce,0x12,0xff,0x39,0xe6,0xd4,0xff,0x00,0x00,0x00,0xff,
    0x82,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x5b,0xe6,0xd4,0xff,0x00,0x00,0x00,0xff,
    0x00,0x00,0x00,0xff,0x68,0x50,0xcd,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,
    0x00,0x00,0x00,0xff,0x10,0x00,0x00,0xff,0xe3,0xea,0x90,0xff,0x5c,0xce,0x12,0xff,
    0x18,0x00,0x00,0xff,0x88,0xd4,0xba,0xff,0x82,0x00,0x00,0xff,0x00,0x00,0x00,0xff,
    0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
    0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
    0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
    0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01
};

/* 2x2 24-bit dds, 2 mipmaps */
static const unsigned char dds_24bit[] = {
0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x0a,0x00,0x02,0x00,0x00,0x00,
0x02,0x00,0x00,0x00,0x0c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0xff,0x00,
0x00,0xff,0x00,0x00,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x40,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};

/* 2x2 16-bit dds, no mipmaps */
static const unsigned char dds_16bit[] = {
0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x08,0x00,0x02,0x00,0x00,0x00,
0x02,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x7c,0x00,0x00,
0xe0,0x03,0x00,0x00,0x1f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0xff,0x7f,0xff,0x7f,0xff,0x7f,0xff,0x7f
};

/* 4x4 cube map dds */
static const unsigned char dds_cube_map[] = {
0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x08,0x00,0x04,0x00,0x00,0x00,
0x04,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
0x04,0x00,0x00,0x00,0x44,0x58,0x54,0x35,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x00,0x00,
0x00,0xfe,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50
};

/* 4x4x2 volume map dds, 2 mipmaps */
static const unsigned char dds_volume_map[] = {
0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x8a,0x00,0x04,0x00,0x00,0x00,
0x04,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
0x04,0x00,0x00,0x00,0x44,0x58,0x54,0x33,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x40,0x00,
0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
0xff,0x00,0xff,0x00,0x00,0x00,0x00,0x00,0x2f,0x7e,0xcf,0x79,0x01,0x54,0x5c,0x5c,
0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x84,0xef,0x7b,0xaa,0xab,0xab,0xab
};

/* 4x2 dxt5 */
static const BYTE dds_dxt5[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x08,0x00,0x02,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x44,0x58,0x54,0x35,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
};

/* 8x8 dxt5 */
static const BYTE dds_dxt5_8_8[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x08,0x00,0x08,0x00,0x00,0x00,
    0x08,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x44,0x58,0x54,0x35,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x1f,0x00,0xe0,0x07,0x05,0x05,0x50,0x50,
    0x3f,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf8,0xff,0x07,0x05,0x05,0x50,0x50,
    0x7f,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x1f,0xf8,0xe0,0xff,0x05,0x05,0x50,0x50,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x00,0x00,0x05,0x05,0x50,0x50,
};

static HRESULT create_file(const char *filename, const unsigned char *data, const unsigned int size)
{
    DWORD received;
    HANDLE hfile;

    hfile = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if(hfile == INVALID_HANDLE_VALUE) return HRESULT_FROM_WIN32(GetLastError());

    if(WriteFile(hfile, data, size, &received, NULL))
    {
        CloseHandle(hfile);
        return D3D_OK;
    }

    CloseHandle(hfile);
    return D3DERR_INVALIDCALL;
}

/* dds_header.flags */
#define DDS_CAPS 0x00000001
#define DDS_HEIGHT 0x00000002
#define DDS_WIDTH 0x00000004
#define DDS_PITCH 0x00000008
#define DDS_PIXELFORMAT 0x00001000
#define DDS_MIPMAPCOUNT 0x00020000
#define DDS_LINEARSIZE 0x00080000

/* dds_header.caps */
#define DDSCAPS_ALPHA    0x00000002
#define DDS_CAPS_TEXTURE 0x00001000

/* dds_pixel_format.flags */
#define DDS_PF_ALPHA 0x00000001
#define DDS_PF_ALPHA_ONLY 0x00000002
#define DDS_PF_FOURCC 0x00000004
#define DDS_PF_RGB 0x00000040
#define DDS_PF_LUMINANCE 0x00020000
#define DDS_PF_BUMPLUMINANCE 0x00040000
#define DDS_PF_BUMPDUDV 0x00080000

struct dds_pixel_format
{
    DWORD size;
    DWORD flags;
    DWORD fourcc;
    DWORD bpp;
    DWORD rmask;
    DWORD gmask;
    DWORD bmask;
    DWORD amask;
};

struct dds_header
{
    DWORD size;
    DWORD flags;
    DWORD height;
    DWORD width;
    DWORD pitch_or_linear_size;
    DWORD depth;
    DWORD miplevels;
    DWORD reserved[11];
    struct dds_pixel_format pixel_format;
    DWORD caps;
    DWORD caps2;
    DWORD caps3;
    DWORD caps4;
    DWORD reserved2;
};

/* fills dds_header with reasonable default values */
static void fill_dds_header(struct dds_header *header)
{
    memset(header, 0, sizeof(*header));

    header->size = sizeof(*header);
    header->flags = DDS_CAPS | DDS_WIDTH | DDS_HEIGHT | DDS_PIXELFORMAT;
    header->height = 4;
    header->width = 4;
    header->pixel_format.size = sizeof(header->pixel_format);
    /* X8R8G8B8 */
    header->pixel_format.flags = DDS_PF_RGB;
    header->pixel_format.fourcc = 0;
    header->pixel_format.bpp = 32;
    header->pixel_format.rmask = 0xff0000;
    header->pixel_format.gmask = 0x00ff00;
    header->pixel_format.bmask = 0x0000ff;
    header->pixel_format.amask = 0;
    header->caps = DDS_CAPS_TEXTURE;
}

#define check_dds_pixel_format(flags, fourcc, bpp, rmask, gmask, bmask, amask, format) \
        check_dds_pixel_format_(__LINE__, flags, fourcc, bpp, rmask, gmask, bmask, amask, format)
static void check_dds_pixel_format_(unsigned int line,
                                    DWORD flags, DWORD fourcc, DWORD bpp,
                                    DWORD rmask, DWORD gmask, DWORD bmask, DWORD amask,
                                    D3DFORMAT expected_format)
{
    HRESULT hr;
    D3DXIMAGE_INFO info;
    struct
    {
        DWORD magic;
        struct dds_header header;
        BYTE data[256];
    } dds;

    dds.magic = MAKEFOURCC('D','D','S',' ');
    fill_dds_header(&dds.header);
    dds.header.pixel_format.flags = flags;
    dds.header.pixel_format.fourcc = fourcc;
    dds.header.pixel_format.bpp = bpp;
    dds.header.pixel_format.rmask = rmask;
    dds.header.pixel_format.gmask = gmask;
    dds.header.pixel_format.bmask = bmask;
    dds.header.pixel_format.amask = amask;
    memset(dds.data, 0, sizeof(dds.data));

    hr = D3DXGetImageInfoFromFileInMemory(&dds, sizeof(dds), &info);
    ok_(__FILE__, line)(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#x for pixel format %#x, expected %#x\n",
            hr, expected_format, D3D_OK);
    if (SUCCEEDED(hr))
    {
        ok_(__FILE__, line)(info.Format == expected_format, "D3DXGetImageInfoFromFileInMemory returned format %#x, expected %#x\n",
                info.Format, expected_format);
    }
}

static void test_dds_header_handling(void)
{
    int i;
    HRESULT hr;
    D3DXIMAGE_INFO info;
    struct
    {
        DWORD magic;
        struct dds_header header;
        BYTE data[4096 * 1024];
    } *dds;

    struct
    {
        struct dds_pixel_format pixel_format;
        DWORD flags;
        DWORD width;
        DWORD height;
        DWORD pitch;
        DWORD miplevels;
        DWORD pixel_data_size;
        struct
        {
            HRESULT hr;
            UINT miplevels;
        }
        expected;
    } tests[] = {
        /* pitch is ignored */
        { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, 0, 4, 4, 0, 0,
          63 /* pixel data size */, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, DDS_PITCH, 4, 4, 0 /* pitch */, 0,
          64, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, DDS_PITCH, 4, 4, 1 /* pitch */, 0,
          64, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, DDS_PITCH, 4, 4, 2 /* pitch */, 0,
          64, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, DDS_PITCH, 4, 4, 3 /* pitch */, 0,
          64, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, DDS_PITCH, 4, 4, 4 /* pitch */, 0,
          64, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, DDS_PITCH, 4, 4, 16 /* pitch */, 0,
          64, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, DDS_PITCH, 4, 4, 1024 /* pitch */, 0,
          64, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, DDS_PITCH, 4, 4, -1 /* pitch */, 0,
          64, { D3D_OK, 1 } },
        /* linear size is ignored */
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, 0, 4, 4, 0, 0,
          7 /* pixel data size */, { D3DXERR_INVALIDDATA, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDS_LINEARSIZE, 4, 4, 0 /* linear size */, 0,
          8, { D3D_OK, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDS_LINEARSIZE, 4, 4, 1 /* linear size */, 0,
          8, { D3D_OK, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDS_LINEARSIZE, 4, 4, 2 /* linear size */, 0,
          8, { D3D_OK, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDS_LINEARSIZE, 4, 4, 9 /* linear size */, 0,
          8, { D3D_OK, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDS_LINEARSIZE, 4, 4, 16 /* linear size */, 0,
          8, { D3D_OK, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDS_LINEARSIZE, 4, 4, -1 /* linear size */, 0,
          8, { D3D_OK, 1 } },
        /* integer overflows */
        { { 32, DDS_PF_RGB, 0, 32, 0xff0000, 0x00ff00, 0x0000ff, 0 }, 0, 0x80000000, 0x80000000 /* 0x80000000 * 0x80000000 * 4 = 0 */, 0, 0,
          64, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB, 0, 32, 0xff0000, 0x00ff00, 0x0000ff, 0 }, 0, 0x8000100, 0x800100 /* 0x8000100 * 0x800100 * 4 = 262144 */, 0, 0,
          64, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_RGB, 0, 32, 0xff0000, 0x00ff00, 0x0000ff, 0 }, 0, 0x80000001, 0x80000001 /* 0x80000001 * 0x80000001 * 4 = 4 */, 0, 0,
          4, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB, 0, 32, 0xff0000, 0x00ff00, 0x0000ff, 0 }, 0, 0x80000001, 0x80000001 /* 0x80000001 * 0x80000001 * 4 = 4 */, 0, 0,
          3 /* pixel data size */, { D3DXERR_INVALIDDATA, 0 } },
        /* file size is validated */
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, 0, 256, 64, 0, 0, 49151, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, 0, 256, 64, 0, 0, 49152, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDS_MIPMAPCOUNT, 256, 64, 0, 4, 65279, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDS_MIPMAPCOUNT, 256, 64, 0, 4, 65280, { D3D_OK, 4 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDS_MIPMAPCOUNT, 256, 64, 0, 9, 65540, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDS_MIPMAPCOUNT, 256, 64, 0, 9, 65541, { D3D_OK, 9 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, 0, 256, 256, 0, 0, 196607, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, 0, 256, 256, 0, 0, 196608, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, 0, 256, 256, 0, 0, 196609, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDS_MIPMAPCOUNT, 256, 256, 0, 1, 196607, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDS_MIPMAPCOUNT, 256, 256, 0, 1, 196608, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDS_MIPMAPCOUNT, 256, 256, 0, 0, 196607, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDS_MIPMAPCOUNT, 256, 256, 0, 0, 196608, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDS_MIPMAPCOUNT, 256, 256, 0, 0, 400000, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDS_MIPMAPCOUNT, 256, 256, 0, 9, 262142, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDS_MIPMAPCOUNT, 256, 256, 0, 9, 262143, { D3D_OK, 9 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDS_MIPMAPCOUNT, 256, 256, 0, 10, 262145, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDS_MIPMAPCOUNT, 256, 256, 0, 10, 262146, { D3D_OK, 10 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDS_MIPMAPCOUNT, 256, 256, 0, 20, 262175, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDS_MIPMAPCOUNT, 256, 256, 0, 20, 262176, { D3D_OK, 20 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, 0, 256, 256, 0, 0, 32767, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, 0, 256, 256, 0, 0, 32768, { D3D_OK, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDS_MIPMAPCOUNT, 256, 256, 0, 0, 32767, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDS_MIPMAPCOUNT, 256, 256, 0, 0, 32768, { D3D_OK, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDS_MIPMAPCOUNT, 256, 256, 0, 9, 43703, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDS_MIPMAPCOUNT, 256, 256, 0, 9, 43704, { D3D_OK, 9 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDS_MIPMAPCOUNT, 256, 256, 0, 20, 43791, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDS_MIPMAPCOUNT, 256, 256, 0, 20, 43792, { D3D_OK, 20 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT4, 0, 0, 0, 0, 0 }, 0, 256, 256, 0, 0, 65535, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT4, 0, 0, 0, 0, 0 }, 0, 256, 256, 0, 0, 65536, { D3D_OK, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT4, 0, 0, 0, 0, 0 }, DDS_MIPMAPCOUNT, 256, 256, 0, 0, 65535, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT4, 0, 0, 0, 0, 0 }, DDS_MIPMAPCOUNT, 256, 256, 0, 0, 65536, { D3D_OK, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT4, 0, 0, 0, 0, 0 }, DDS_MIPMAPCOUNT, 256, 256, 0, 9, 87407, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT4, 0, 0, 0, 0, 0 }, DDS_MIPMAPCOUNT, 256, 256, 0, 9, 87408, { D3D_OK, 9 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT4, 0, 0, 0, 0, 0 }, DDS_MIPMAPCOUNT, 256, 256, 0, 20, 87583, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT4, 0, 0, 0, 0, 0 }, DDS_MIPMAPCOUNT, 256, 256, 0, 20, 87584, { D3D_OK, 20 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT4, 0, 0, 0, 0, 0 }, DDS_MIPMAPCOUNT, 256, 64, 0, 4, 21759, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT4, 0, 0, 0, 0, 0 }, DDS_MIPMAPCOUNT, 256, 64, 0, 4, 21760, { D3D_OK, 4 } },
        /* DDS_MIPMAPCOUNT is ignored */
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, 0, 256, 256, 0, 0, 262146, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, 0, 256, 256, 0, 2, 262146, { D3D_OK, 2 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, 0, 256, 256, 0, 9, 262146, { D3D_OK, 9 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, 0, 256, 256, 0, 10, 262146, { D3D_OK, 10 } },
    };

    dds = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*dds));
    if (!dds)
    {
        skip("Failed to allocate memory.\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        DWORD file_size = sizeof(dds->magic) + sizeof(dds->header) + tests[i].pixel_data_size;
        assert(file_size <= sizeof(*dds));

        dds->magic = MAKEFOURCC('D','D','S',' ');
        fill_dds_header(&dds->header);
        dds->header.flags |= tests[i].flags;
        dds->header.width = tests[i].width;
        dds->header.height = tests[i].height;
        dds->header.pitch_or_linear_size = tests[i].pitch;
        dds->header.miplevels = tests[i].miplevels;
        dds->header.pixel_format = tests[i].pixel_format;

        hr = D3DXGetImageInfoFromFileInMemory(dds, file_size, &info);
        ok(hr == tests[i].expected.hr, "%d: D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n",
                i, hr, tests[i].expected.hr);
        if (SUCCEEDED(hr))
        {
            ok(info.MipLevels == tests[i].expected.miplevels, "%d: Got MipLevels %u, expected %u\n",
                    i, info.MipLevels, tests[i].expected.miplevels);
        }
    }

    HeapFree(GetProcessHeap(), 0, dds);
}

static void test_D3DXGetImageInfo(void)
{
    HRESULT hr;
    D3DXIMAGE_INFO info;
    BOOL testdummy_ok, testbitmap_ok;

    hr = create_file("testdummy.bmp", noimage, sizeof(noimage));  /* invalid image */
    testdummy_ok = SUCCEEDED(hr);

    hr = create_file("testbitmap.bmp", bmp_1bpp, sizeof(bmp_1bpp));  /* valid image */
    testbitmap_ok = SUCCEEDED(hr);

    /* D3DXGetImageInfoFromFile */
    if(testbitmap_ok) {
        hr = D3DXGetImageInfoFromFileA("testbitmap.bmp", &info);
        ok(hr == D3D_OK, "D3DXGetImageInfoFromFile returned %#x, expected %#x\n", hr, D3D_OK);

        hr = D3DXGetImageInfoFromFileA("testbitmap.bmp", NULL); /* valid image, second parameter is NULL */
        ok(hr == D3D_OK, "D3DXGetImageInfoFromFile returned %#x, expected %#x\n", hr, D3D_OK);
    } else skip("Couldn't create \"testbitmap.bmp\"\n");

    if(testdummy_ok) {
        hr = D3DXGetImageInfoFromFileA("testdummy.bmp", NULL); /* invalid image, second parameter is NULL */
        ok(hr == D3D_OK, "D3DXGetImageInfoFromFile returned %#x, expected %#x\n", hr, D3D_OK);

        hr = D3DXGetImageInfoFromFileA("testdummy.bmp", &info);
        ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFile returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);
    } else skip("Couldn't create \"testdummy.bmp\"\n");

    hr = D3DXGetImageInfoFromFileA("filedoesnotexist.bmp", &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFile returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromFileA("filedoesnotexist.bmp", NULL);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFile returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromFileA("", &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFile returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromFileA(NULL, &info);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFile returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileA(NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFile returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);


    /* D3DXGetImageInfoFromResource */
    hr = D3DXGetImageInfoFromResourceA(NULL, MAKEINTRESOURCEA(IDB_BITMAP_1x1), &info); /* RT_BITMAP */
    ok(hr == D3D_OK, "D3DXGetImageInfoFromResource returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXGetImageInfoFromResourceA(NULL, MAKEINTRESOURCEA(IDB_BITMAP_1x1), NULL);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromResource returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXGetImageInfoFromResourceA(NULL, MAKEINTRESOURCEA(IDD_BITMAPDATA_1x1), &info); /* RT_RCDATA */
    ok(hr == D3D_OK, "D3DXGetImageInfoFromResource returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXGetImageInfoFromResourceA(NULL, MAKEINTRESOURCEA(IDS_STRING), &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromResource returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromResourceA(NULL, MAKEINTRESOURCEA(IDS_STRING), NULL);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromResource returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromResourceA(NULL, "resourcedoesnotexist", &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromResource returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromResourceA(NULL, "resourcedoesnotexist", NULL);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromResource returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromResourceA(NULL, NULL, NULL);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromResource returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);


    /* D3DXGetImageInfoFromFileInMemory */
    hr = D3DXGetImageInfoFromFileInMemory(bmp_1bpp, sizeof(bmp_1bpp), &info);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXGetImageInfoFromFileInMemory(bmp_1bpp, sizeof(bmp_1bpp)+5, &info); /* too large size */
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXGetImageInfoFromFileInMemory(bmp_1bpp, sizeof(bmp_1bpp), NULL);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXGetImageInfoFromFileInMemory(noimage, sizeof(noimage), NULL);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromResource returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXGetImageInfoFromFileInMemory(noimage, sizeof(noimage), &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    todo_wine {
        hr = D3DXGetImageInfoFromFileInMemory(bmp_1bpp, sizeof(bmp_1bpp)-1, &info);
        ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);
    }

    hr = D3DXGetImageInfoFromFileInMemory(bmp_1bpp+1, sizeof(bmp_1bpp)-1, &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromFileInMemory(bmp_1bpp, 0, &info);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileInMemory(bmp_1bpp, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileInMemory(noimage, 0, &info);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileInMemory(noimage, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileInMemory(NULL, 0, &info);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileInMemory(NULL, 4, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileInMemory(NULL, 4, &info);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileInMemory(NULL, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    /* test BMP support */
    hr = D3DXGetImageInfoFromFileInMemory(bmp_1bpp, sizeof(bmp_1bpp), &info);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);
    ok(info.Depth == 1, "Got depth %u, expected 1\n", info.Depth);
    ok(info.Format == D3DFMT_P8, "Got format %u, expected %u\n", info.Format, D3DFMT_P8);
    hr = D3DXGetImageInfoFromFileInMemory(bmp_2bpp, sizeof(bmp_2bpp), &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);
    hr = D3DXGetImageInfoFromFileInMemory(bmp_4bpp, sizeof(bmp_4bpp), &info);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);
    ok(info.Depth == 1, "Got depth %u, expected 1\n", info.Depth);
    ok(info.Format == D3DFMT_P8, "Got format %u, expected %u\n", info.Format, D3DFMT_P8);
    hr = D3DXGetImageInfoFromFileInMemory(bmp_8bpp, sizeof(bmp_8bpp), &info);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);
    ok(info.Depth == 1, "Got depth %u, expected 1\n", info.Depth);
    ok(info.Format == D3DFMT_P8, "Got format %u, expected %u\n", info.Format, D3DFMT_P8);

    hr = D3DXGetImageInfoFromFileInMemory(bmp_32bpp_xrgb, sizeof(bmp_32bpp_xrgb), &info);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(info.Format == D3DFMT_X8R8G8B8, "Got unexpected format %u.\n", info.Format);
    hr = D3DXGetImageInfoFromFileInMemory(bmp_32bpp_argb, sizeof(bmp_32bpp_argb), &info);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(info.Format == D3DFMT_A8R8G8B8, "Got unexpected format %u.\n", info.Format);

    /* Grayscale PNG */
    hr = D3DXGetImageInfoFromFileInMemory(png_grayscale, sizeof(png_grayscale), &info);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);
    ok(info.Depth == 1, "Got depth %u, expected 1\n", info.Depth);
    ok(info.Format == D3DFMT_L8, "Got format %u, expected %u\n", info.Format, D3DFMT_L8);

    /* test DDS support */
    hr = D3DXGetImageInfoFromFileInMemory(dds_24bit, sizeof(dds_24bit), &info);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);
    if (hr == D3D_OK) {
        ok(info.Width == 2, "Got width %u, expected 2\n", info.Width);
        ok(info.Height == 2, "Got height %u, expected 2\n", info.Height);
        ok(info.Depth == 1, "Got depth %u, expected 1\n", info.Depth);
        ok(info.MipLevels == 2, "Got miplevels %u, expected 2\n", info.MipLevels);
        ok(info.Format == D3DFMT_R8G8B8, "Got format %#x, expected %#x\n", info.Format, D3DFMT_R8G8B8);
        ok(info.ResourceType == D3DRTYPE_TEXTURE, "Got resource type %#x, expected %#x\n", info.ResourceType, D3DRTYPE_TEXTURE);
        ok(info.ImageFileFormat == D3DXIFF_DDS, "Got image file format %#x, expected %#x\n", info.ImageFileFormat, D3DXIFF_DDS);
    } else skip("Couldn't get image info from 24-bit DDS file in memory\n");

    hr = D3DXGetImageInfoFromFileInMemory(dds_16bit, sizeof(dds_16bit), &info);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);
    if (hr == D3D_OK) {
        ok(info.Width == 2, "Got width %u, expected 2\n", info.Width);
        ok(info.Height == 2, "Got height %u, expected 2\n", info.Height);
        ok(info.Depth == 1, "Got depth %u, expected 1\n", info.Depth);
        ok(info.MipLevels == 1, "Got miplevels %u, expected 1\n", info.MipLevels);
        ok(info.Format == D3DFMT_X1R5G5B5, "Got format %#x, expected %#x\n", info.Format, D3DFMT_X1R5G5B5);
        ok(info.ResourceType == D3DRTYPE_TEXTURE, "Got resource type %#x, expected %#x\n", info.ResourceType, D3DRTYPE_TEXTURE);
        ok(info.ImageFileFormat == D3DXIFF_DDS, "Got image file format %#x, expected %#x\n", info.ImageFileFormat, D3DXIFF_DDS);
    } else skip("Couldn't get image info from 16-bit DDS file in memory\n");

    memset(&info, 0, sizeof(info));
    hr = D3DXGetImageInfoFromFileInMemory(dds_8bit, sizeof(dds_8bit), &info);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#x\n", hr);
    ok(info.Width == 16, "Got width %u.\n", info.Width);
    ok(info.Height == 4, "Got height %u.\n", info.Height);
    ok(info.Depth == 1, "Got depth %u.\n", info.Depth);
    ok(info.MipLevels == 1, "Got miplevels %u.\n", info.MipLevels);
    ok(info.Format == D3DFMT_P8, "Got format %#x.\n", info.Format);
    ok(info.ResourceType == D3DRTYPE_TEXTURE, "Got resource type %#x.\n", info.ResourceType);
    ok(info.ImageFileFormat == D3DXIFF_DDS, "Got image file format %#x.\n", info.ImageFileFormat);

    hr = D3DXGetImageInfoFromFileInMemory(dds_cube_map, sizeof(dds_cube_map), &info);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);
    if (hr == D3D_OK) {
        ok(info.Width == 4, "Got width %u, expected 4\n", info.Width);
        ok(info.Height == 4, "Got height %u, expected 4\n", info.Height);
        ok(info.Depth == 1, "Got depth %u, expected 1\n", info.Depth);
        ok(info.MipLevels == 1, "Got miplevels %u, expected 1\n", info.MipLevels);
        ok(info.Format == D3DFMT_DXT5, "Got format %#x, expected %#x\n", info.Format, D3DFMT_DXT5);
        ok(info.ResourceType == D3DRTYPE_CUBETEXTURE, "Got resource type %#x, expected %#x\n", info.ResourceType, D3DRTYPE_CUBETEXTURE);
        ok(info.ImageFileFormat == D3DXIFF_DDS, "Got image file format %#x, expected %#x\n", info.ImageFileFormat, D3DXIFF_DDS);
    } else skip("Couldn't get image info from cube map in memory\n");

    hr = D3DXGetImageInfoFromFileInMemory(dds_volume_map, sizeof(dds_volume_map), &info);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);
    if (hr == D3D_OK) {
        ok(info.Width == 4, "Got width %u, expected 4\n", info.Width);
        ok(info.Height == 4, "Got height %u, expected 4\n", info.Height);
        ok(info.Depth == 2, "Got depth %u, expected 2\n", info.Depth);
        ok(info.MipLevels == 3, "Got miplevels %u, expected 3\n", info.MipLevels);
        ok(info.Format == D3DFMT_DXT3, "Got format %#x, expected %#x\n", info.Format, D3DFMT_DXT3);
        ok(info.ResourceType == D3DRTYPE_VOLUMETEXTURE, "Got resource type %#x, expected %#x\n", info.ResourceType, D3DRTYPE_VOLUMETEXTURE);
        ok(info.ImageFileFormat == D3DXIFF_DDS, "Got image file format %#x, expected %#x\n", info.ImageFileFormat, D3DXIFF_DDS);
    } else skip("Couldn't get image info from volume map in memory\n");

    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0, D3DFMT_DXT1);
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_DXT2, 0, 0, 0, 0, 0, D3DFMT_DXT2);
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_DXT3, 0, 0, 0, 0, 0, D3DFMT_DXT3);
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_DXT4, 0, 0, 0, 0, 0, D3DFMT_DXT4);
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_DXT5, 0, 0, 0, 0, 0, D3DFMT_DXT5);
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_R8G8_B8G8, 0, 0, 0, 0, 0, D3DFMT_R8G8_B8G8);
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_G8R8_G8B8, 0, 0, 0, 0, 0, D3DFMT_G8R8_G8B8);
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_UYVY, 0, 0, 0, 0, 0, D3DFMT_UYVY);
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_YUY2, 0, 0, 0, 0, 0, D3DFMT_YUY2);
    check_dds_pixel_format(DDS_PF_RGB, 0, 16, 0xf800, 0x07e0, 0x001f, 0, D3DFMT_R5G6B5);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 16, 0x7c00, 0x03e0, 0x001f, 0x8000, D3DFMT_A1R5G5B5);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 16, 0x0f00, 0x00f0, 0x000f, 0xf000, D3DFMT_A4R4G4B4);
    check_dds_pixel_format(DDS_PF_RGB, 0, 8, 0xe0, 0x1c, 0x03, 0, D3DFMT_R3G3B2);
    check_dds_pixel_format(DDS_PF_ALPHA_ONLY, 0, 8, 0, 0, 0, 0xff, D3DFMT_A8);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 16, 0x00e0, 0x001c, 0x0003, 0xff00, D3DFMT_A8R3G3B2);
    check_dds_pixel_format(DDS_PF_RGB, 0, 16, 0xf00, 0x0f0, 0x00f, 0, D3DFMT_X4R4G4B4);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000, D3DFMT_A2B10G10R10);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000, D3DFMT_A2R10G10B10);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000, D3DFMT_A8R8G8B8);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000, D3DFMT_A8B8G8R8);
    check_dds_pixel_format(DDS_PF_RGB, 0, 32, 0xff0000, 0x00ff00, 0x0000ff, 0, D3DFMT_X8R8G8B8);
    check_dds_pixel_format(DDS_PF_RGB, 0, 32, 0x0000ff, 0x00ff00, 0xff0000, 0, D3DFMT_X8B8G8R8);
    check_dds_pixel_format(DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0, D3DFMT_R8G8B8);
    check_dds_pixel_format(DDS_PF_RGB, 0, 32, 0x0000ffff, 0xffff0000, 0, 0, D3DFMT_G16R16);
    check_dds_pixel_format(DDS_PF_LUMINANCE, 0, 8, 0xff, 0, 0, 0, D3DFMT_L8);
    check_dds_pixel_format(DDS_PF_LUMINANCE, 0, 16, 0xffff, 0, 0, 0, D3DFMT_L16);
    check_dds_pixel_format(DDS_PF_LUMINANCE | DDS_PF_ALPHA, 0, 16, 0x00ff, 0, 0, 0xff00, D3DFMT_A8L8);
    check_dds_pixel_format(DDS_PF_LUMINANCE | DDS_PF_ALPHA, 0, 8, 0x0f, 0, 0, 0xf0, D3DFMT_A4L4);
    check_dds_pixel_format(DDS_PF_BUMPDUDV, 0, 16, 0x00ff, 0xff00, 0, 0, D3DFMT_V8U8);
    check_dds_pixel_format(DDS_PF_BUMPDUDV, 0, 32, 0x0000ffff, 0xffff0000, 0, 0, D3DFMT_V16U16);
    check_dds_pixel_format(DDS_PF_BUMPLUMINANCE, 0, 32, 0x0000ff, 0x00ff00, 0xff0000, 0, D3DFMT_X8L8V8U8);

    test_dds_header_handling();

    hr = D3DXGetImageInfoFromFileInMemory(dds_16bit, sizeof(dds_16bit) - 1, &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromFileInMemory(dds_24bit, sizeof(dds_24bit) - 1, &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromFileInMemory(dds_cube_map, sizeof(dds_cube_map) - 1, &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromFileInMemory(dds_volume_map, sizeof(dds_volume_map) - 1, &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);


    /* cleanup */
    if(testdummy_ok) DeleteFileA("testdummy.bmp");
    if(testbitmap_ok) DeleteFileA("testbitmap.bmp");
}

#define check_pixel_2bpp(lockrect, x, y, color) _check_pixel_2bpp(__LINE__, lockrect, x, y, color)
static inline void _check_pixel_2bpp(unsigned int line, const D3DLOCKED_RECT *lockrect, int x, int y, WORD expected_color)
{
    WORD color = ((WORD*)lockrect->pBits)[x + y * lockrect->Pitch / 2];
    ok_(__FILE__, line)(color == expected_color, "Got color 0x%04x, expected 0x%04x\n", color, expected_color);
}

#define check_pixel_4bpp(lockrect, x, y, color) _check_pixel_4bpp(__LINE__, lockrect, x, y, color)
static inline void _check_pixel_4bpp(unsigned int line, const D3DLOCKED_RECT *lockrect, int x, int y, DWORD expected_color)
{
   DWORD color = ((DWORD*)lockrect->pBits)[x + y * lockrect->Pitch / 4];
   ok_(__FILE__, line)(color == expected_color, "Got color 0x%08x, expected 0x%08x\n", color, expected_color);
}

static void test_D3DXLoadSurface(IDirect3DDevice9 *device)
{
    HRESULT hr;
    BOOL testdummy_ok, testbitmap_ok;
    IDirect3DTexture9 *tex;
    IDirect3DSurface9 *surf, *newsurf;
    RECT rect, destrect;
    D3DLOCKED_RECT lockrect;
    static const WORD pixdata_a8r3g3b2[] = { 0x57df, 0x98fc, 0xacdd, 0xc891 };
    static const WORD pixdata_a1r5g5b5[] = { 0x46b5, 0x99c8, 0x06a2, 0x9431 };
    static const WORD pixdata_r5g6b5[] = { 0x9ef6, 0x658d, 0x0aee, 0x42ee };
    static const WORD pixdata_a8l8[] = { 0xff00, 0x00ff, 0xff30, 0x7f7f };
    static const DWORD pixdata_g16r16[] = { 0x07d23fbe, 0xdc7f44a4, 0xe4d8976b, 0x9a84fe89 };
    static const DWORD pixdata_a8b8g8r8[] = { 0xc3394cf0, 0x235ae892, 0x09b197fd, 0x8dc32bf6 };
    static const DWORD pixdata_a2r10g10b10[] = { 0x57395aff, 0x5b7668fd, 0xb0d856b5, 0xff2c61d6 };

    hr = create_file("testdummy.bmp", noimage, sizeof(noimage));  /* invalid image */
    testdummy_ok = SUCCEEDED(hr);

    hr = create_file("testbitmap.bmp", bmp_1bpp, sizeof(bmp_1bpp));  /* valid image */
    testbitmap_ok = SUCCEEDED(hr);

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 256, 256, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &surf, NULL);
    if(FAILED(hr)) {
        skip("Failed to create a surface (%#x)\n", hr);
        if(testdummy_ok) DeleteFileA("testdummy.bmp");
        if(testbitmap_ok) DeleteFileA("testbitmap.bmp");
        return;
    }

    /* D3DXLoadSurfaceFromFile */
    if(testbitmap_ok) {
        hr = D3DXLoadSurfaceFromFileA(surf, NULL, NULL, "testbitmap.bmp", NULL, D3DX_DEFAULT, 0, NULL);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromFile returned %#x, expected %#x\n", hr, D3D_OK);

        hr = D3DXLoadSurfaceFromFileA(NULL, NULL, NULL, "testbitmap.bmp", NULL, D3DX_DEFAULT, 0, NULL);
        ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromFile returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);
    } else skip("Couldn't create \"testbitmap.bmp\"\n");

    if(testdummy_ok) {
        hr = D3DXLoadSurfaceFromFileA(surf, NULL, NULL, "testdummy.bmp", NULL, D3DX_DEFAULT, 0, NULL);
        ok(hr == D3DXERR_INVALIDDATA, "D3DXLoadSurfaceFromFile returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);
    } else skip("Couldn't create \"testdummy.bmp\"\n");

    hr = D3DXLoadSurfaceFromFileA(surf, NULL, NULL, NULL, NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromFile returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadSurfaceFromFileA(surf, NULL, NULL, "", NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXLoadSurfaceFromFile returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);


    /* D3DXLoadSurfaceFromResource */
    hr = D3DXLoadSurfaceFromResourceA(surf, NULL, NULL, NULL,
            MAKEINTRESOURCEA(IDB_BITMAP_1x1), NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromResource returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXLoadSurfaceFromResourceA(surf, NULL, NULL, NULL,
            MAKEINTRESOURCEA(IDD_BITMAPDATA_1x1), NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromResource returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXLoadSurfaceFromResourceA(surf, NULL, NULL, NULL, NULL, NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXLoadSurfaceFromResource returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXLoadSurfaceFromResourceA(NULL, NULL, NULL, NULL,
            MAKEINTRESOURCEA(IDB_BITMAP_1x1), NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromResource returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadSurfaceFromResourceA(surf, NULL, NULL, NULL,
            MAKEINTRESOURCEA(IDS_STRING), NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXLoadSurfaceFromResource returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);


    /* D3DXLoadSurfaceFromFileInMemory */
    hr = D3DXLoadSurfaceFromFileInMemory(surf, NULL, NULL, bmp_1bpp, sizeof(bmp_1bpp), NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXLoadSurfaceFromFileInMemory(surf, NULL, NULL, noimage, sizeof(noimage), NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXLoadSurfaceFromFileInMemory returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXLoadSurfaceFromFileInMemory(surf, NULL, NULL, bmp_1bpp, 0, NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadSurfaceFromFileInMemory(NULL, NULL, NULL, bmp_1bpp, sizeof(bmp_1bpp), NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadSurfaceFromFileInMemory(surf, NULL, NULL, NULL, 8, NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadSurfaceFromFileInMemory(surf, NULL, NULL, NULL, 0, NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadSurfaceFromFileInMemory(NULL, NULL, NULL, NULL, 0, NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);


    /* D3DXLoadSurfaceFromMemory */
    SetRect(&rect, 0, 0, 2, 2);

    hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata, D3DFMT_A8R8G8B8, sizeof(pixdata), NULL, &rect, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata, D3DFMT_A8R8G8B8, 0, NULL, &rect, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, NULL, D3DFMT_A8R8G8B8, sizeof(pixdata), NULL, &rect, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadSurfaceFromMemory(NULL, NULL, NULL, pixdata, D3DFMT_A8R8G8B8, sizeof(pixdata), NULL, &rect, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata, D3DFMT_A8R8G8B8, sizeof(pixdata), NULL, NULL, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata, D3DFMT_UNKNOWN, sizeof(pixdata), NULL, &rect, D3DX_DEFAULT, 0);
    ok(hr == E_FAIL, "D3DXLoadSurfaceFromMemory returned %#x, expected %#x\n", hr, E_FAIL);

    SetRect(&destrect, -1, -1, 1, 1); /* destination rect is partially outside texture boundaries */
    hr = D3DXLoadSurfaceFromMemory(surf, NULL, &destrect, pixdata, D3DFMT_A8R8G8B8, sizeof(pixdata), NULL, &rect, D3DX_FILTER_NONE, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    SetRect(&destrect, 255, 255, 257, 257); /* destination rect is partially outside texture boundaries */
    hr = D3DXLoadSurfaceFromMemory(surf, NULL, &destrect, pixdata, D3DFMT_A8R8G8B8, sizeof(pixdata), NULL, &rect, D3DX_FILTER_NONE, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    SetRect(&destrect, 1, 1, 0, 0); /* left > right, top > bottom */
    hr = D3DXLoadSurfaceFromMemory(surf, NULL, &destrect, pixdata, D3DFMT_A8R8G8B8, sizeof(pixdata), NULL, &rect, D3DX_FILTER_NONE, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    SetRect(&destrect, 1, 2, 1, 2); /* left = right, top = bottom */
    hr = D3DXLoadSurfaceFromMemory(surf, NULL, &destrect, pixdata, D3DFMT_A8R8G8B8, sizeof(pixdata), NULL, &rect, D3DX_FILTER_NONE, 0);
    /* fails when debug version of d3d9 is used */
    ok(hr == D3D_OK || broken(hr == D3DERR_INVALIDCALL), "D3DXLoadSurfaceFromMemory returned %#x, expected %#x\n", hr, D3D_OK);

    SetRect(&destrect, 257, 257, 257, 257); /* left = right, top = bottom, but invalid values */
    hr = D3DXLoadSurfaceFromMemory(surf, NULL, &destrect, pixdata, D3DFMT_A8R8G8B8, sizeof(pixdata), NULL, &rect, D3DX_FILTER_NONE, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);


    /* D3DXLoadSurfaceFromSurface */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 256, 256, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &newsurf, NULL);
    if(SUCCEEDED(hr)) {
        hr = D3DXLoadSurfaceFromSurface(newsurf, NULL, NULL, surf, NULL, NULL, D3DX_DEFAULT, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#x, expected %#x\n", hr, D3D_OK);

        hr = D3DXLoadSurfaceFromSurface(NULL, NULL, NULL, surf, NULL, NULL, D3DX_DEFAULT, 0);
        ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromSurface returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        hr = D3DXLoadSurfaceFromSurface(newsurf, NULL, NULL, NULL, NULL, NULL, D3DX_DEFAULT, 0);
        ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromSurface returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        check_release((IUnknown*)newsurf, 0);
    } else skip("Failed to create a second surface\n");

    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex, NULL);
    if (SUCCEEDED(hr))
    {
        IDirect3DTexture9_GetSurfaceLevel(tex, 0, &newsurf);

        hr = D3DXLoadSurfaceFromSurface(newsurf, NULL, NULL, surf, NULL, NULL, D3DX_DEFAULT, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#x, expected %#x\n", hr, D3D_OK);

        IDirect3DSurface9_Release(newsurf);
        IDirect3DTexture9_Release(tex);
    } else skip("Failed to create texture\n");

    /* non-lockable render target */
    hr = IDirect3DDevice9_CreateRenderTarget(device, 256, 256, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &newsurf, NULL);
    ok(hr == D3D_OK, "Unexpected hr %#x.\n", hr);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, NULL, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "Unexpected hr %#x.\n", hr);
    IDirect3DSurface9_Release(newsurf);

    /* non-lockable multisampled render target */
    hr = IDirect3DDevice9_CreateRenderTarget(device, 256, 256, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_2_SAMPLES, 0, FALSE, &newsurf, NULL);
    if (SUCCEEDED(hr))
    {
       hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, NULL, D3DX_FILTER_NONE, 0);
       ok(hr == D3D_OK, "Unexpected hr %#x.\n", hr);

       IDirect3DSurface9_Release(newsurf);
    }
    else
    {
        skip("Failed to create multisampled render target.\n");
    }

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &newsurf);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderTarget returned %#x, expected %#x.\n", hr, D3D_OK);

    hr = D3DXLoadSurfaceFromSurface(newsurf, NULL, NULL, surf, NULL, NULL, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#x, expected %#x.\n", hr, D3D_OK);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, NULL, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#x, expected %#x.\n", hr, D3D_OK);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, NULL, D3DX_FILTER_NONE, 0xff000000);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#x, expected %#x.\n", hr, D3D_OK);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, NULL, D3DX_FILTER_TRIANGLE | D3DX_FILTER_MIRROR, 0);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#x, expected %#x.\n", hr, D3D_OK);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, NULL, D3DX_FILTER_LINEAR, 0);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#x, expected %#x.\n", hr, D3D_OK);

    /* rects */
    SetRect(&rect, 2, 2, 1, 1);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, &rect, D3DX_FILTER_NONE, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromSurface returned %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, &rect, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromSurface returned %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, &rect, newsurf, NULL, NULL, D3DX_FILTER_NONE, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromSurface returned %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, &rect, newsurf, NULL, NULL, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromSurface returned %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, &rect, newsurf, NULL, NULL, D3DX_FILTER_POINT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromSurface returned %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);
    SetRect(&rect, 1, 1, 1, 1);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, &rect, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#x, expected %#x.\n", hr, D3D_OK);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, &rect, newsurf, NULL, NULL, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#x, expected %#x.\n", hr, D3D_OK);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, &rect, D3DX_DEFAULT, 0);
    ok(hr == E_FAIL, "D3DXLoadSurfaceFromSurface returned %#x, expected %#x.\n", hr, E_FAIL);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, &rect, newsurf, NULL, NULL, D3DX_DEFAULT, 0);
    ok(hr == E_FAIL, "D3DXLoadSurfaceFromSurface returned %#x, expected %#x.\n", hr, E_FAIL);
    if (0)
    {
        /* Somehow it crashes with a STATUS_INTEGER_DIVIDE_BY_ZERO exception
         * on Windows. */
        hr = D3DXLoadSurfaceFromSurface(surf, NULL, &rect, newsurf, NULL, NULL, D3DX_FILTER_POINT, 0);
        ok(hr == E_FAIL, "D3DXLoadSurfaceFromSurface returned %#x, expected %#x.\n", hr, E_FAIL);
    }
    SetRect(&rect, 1, 1, 2, 2);
    SetRect(&destrect, 1, 1, 2, 2);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, &rect, newsurf, NULL, &destrect, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#x, expected %#x.\n", hr, D3D_OK);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, &rect, newsurf, NULL, NULL, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#x, expected %#x.\n", hr, D3D_OK);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, &destrect, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#x, expected %#x.\n", hr, D3D_OK);

    IDirect3DSurface9_Release(newsurf);

    check_release((IUnknown*)surf, 0);

    SetRect(&rect, 1, 1, 2, 2);
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 1, 1, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &surf, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8b8g8r8,
            D3DFMT_A8R8G8B8, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
    check_pixel_4bpp(&lockrect, 0, 0, 0x8dc32bf6);
    IDirect3DSurface9_UnlockRect(surf);
    check_release((IUnknown *)surf, 0);

    /* test color conversion */
    SetRect(&rect, 0, 0, 2, 2);
    /* A8R8G8B8 */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 2, 2, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &surf, NULL);
    if(FAILED(hr)) skip("Failed to create a surface (%#x)\n", hr);
    else {
        PALETTEENTRY palette;

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8r3g3b2,
                D3DFMT_A8R3G3B2, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#x, expected %#x\n", hr, D3D_OK);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        check_pixel_4bpp(&lockrect, 0, 0, 0x57dbffff);
        check_pixel_4bpp(&lockrect, 1, 0, 0x98ffff00);
        check_pixel_4bpp(&lockrect, 0, 1, 0xacdbff55);
        check_pixel_4bpp(&lockrect, 1, 1, 0xc8929255);
        IDirect3DSurface9_UnlockRect(surf);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a1r5g5b5,
                D3DFMT_A1R5G5B5, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#x, expected %#x\n", hr, D3D_OK);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        check_pixel_4bpp(&lockrect, 0, 0, 0x008cadad);
        check_pixel_4bpp(&lockrect, 1, 0, 0xff317342);
        check_pixel_4bpp(&lockrect, 0, 1, 0x0008ad10);
        check_pixel_4bpp(&lockrect, 1, 1, 0xff29088c);
        IDirect3DSurface9_UnlockRect(surf);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_r5g6b5,
                D3DFMT_R5G6B5, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#x, expected %#x\n", hr, D3D_OK);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        check_pixel_4bpp(&lockrect, 0, 0, 0xff9cdfb5);
        check_pixel_4bpp(&lockrect, 1, 0, 0xff63b26b);
        check_pixel_4bpp(&lockrect, 0, 1, 0xff085d73);
        check_pixel_4bpp(&lockrect, 1, 1, 0xff425d73);
        IDirect3DSurface9_UnlockRect(surf);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_g16r16,
                D3DFMT_G16R16, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#x, expected %#x\n", hr, D3D_OK);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        todo_wine {
            check_pixel_4bpp(&lockrect, 0, 0, 0xff3f08ff);
        }
        check_pixel_4bpp(&lockrect, 1, 0, 0xff44dcff);
        check_pixel_4bpp(&lockrect, 0, 1, 0xff97e4ff);
        check_pixel_4bpp(&lockrect, 1, 1, 0xfffe9aff);
        IDirect3DSurface9_UnlockRect(surf);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8b8g8r8,
                D3DFMT_A8B8G8R8, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
        check_pixel_4bpp(&lockrect, 0, 0, 0xc3f04c39);
        check_pixel_4bpp(&lockrect, 1, 0, 0x2392e85a);
        check_pixel_4bpp(&lockrect, 0, 1, 0x09fd97b1);
        check_pixel_4bpp(&lockrect, 1, 1, 0x8df62bc3);
        IDirect3DSurface9_UnlockRect(surf);

        SetRect(&rect, 0, 0, 1, 1);
        SetRect(&destrect, 1, 1, 2, 2);
        hr = D3DXLoadSurfaceFromMemory(surf, NULL, &destrect, pixdata_a8b8g8r8,
                D3DFMT_A8B8G8R8, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        check_pixel_4bpp(&lockrect, 0, 0, 0xc3f04c39);
        check_pixel_4bpp(&lockrect, 1, 0, 0x2392e85a);
        check_pixel_4bpp(&lockrect, 0, 1, 0x09fd97b1);
        check_pixel_4bpp(&lockrect, 1, 1, 0xc3f04c39);
        IDirect3DSurface9_UnlockRect(surf);

        SetRect(&rect, 0, 0, 2, 2);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a2r10g10b10,
                D3DFMT_A2R10G10B10, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        check_pixel_4bpp(&lockrect, 0, 0, 0x555c95bf);
        check_pixel_4bpp(&lockrect, 1, 0, 0x556d663f);
        check_pixel_4bpp(&lockrect, 0, 1, 0xaac385ad);
        todo_wine {
            check_pixel_4bpp(&lockrect, 1, 1, 0xfffcc575);
        }
        IDirect3DSurface9_UnlockRect(surf);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8l8,
                D3DFMT_A8L8, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(SUCCEEDED(hr), "Failed to load surface, hr %#x.\n", hr);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        check_pixel_4bpp(&lockrect, 0, 0, 0xff000000);
        check_pixel_4bpp(&lockrect, 1, 0, 0x00ffffff);
        check_pixel_4bpp(&lockrect, 0, 1, 0xff303030);
        check_pixel_4bpp(&lockrect, 1, 1, 0x7f7f7f7f);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        /* Test D3DXLoadSurfaceFromMemory with indexed color image */
        if (0)
        {
        /* Crashes on Nvidia Win10. */
        palette.peRed   = bmp_1bpp[56];
        palette.peGreen = bmp_1bpp[55];
        palette.peBlue  = bmp_1bpp[54];
        palette.peFlags = bmp_1bpp[57]; /* peFlags is the alpha component in DX8 and higher */
        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, &bmp_1bpp[62],
                D3DFMT_P8, 1, (const PALETTEENTRY *)&palette, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x\n", hr);
        ok(*(DWORD*)lockrect.pBits == 0x80f3f2f1,
                "Pixel color mismatch: got %#x, expected 0x80f3f2f1\n", *(DWORD*)lockrect.pBits);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x\n", hr);
        }

        /* Test D3DXLoadSurfaceFromFileInMemory with indexed color image (alpha is not taken into account for bmp file) */
        hr = D3DXLoadSurfaceFromFileInMemory(surf, NULL, NULL, bmp_1bpp, sizeof(bmp_1bpp), NULL, D3DX_FILTER_NONE, 0, NULL);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x\n", hr);
        ok(*(DWORD*)lockrect.pBits == 0xfff3f2f1, "Pixel color mismatch: got %#x, expected 0xfff3f2f1\n", *(DWORD*)lockrect.pBits);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x\n", hr);

        check_release((IUnknown*)surf, 0);
    }

    /* A1R5G5B5 */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 2, 2, D3DFMT_A1R5G5B5, D3DPOOL_DEFAULT, &surf, NULL);
    if(FAILED(hr)) skip("Failed to create a surface (%#x)\n", hr);
    else {
        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8r3g3b2,
                D3DFMT_A8R3G3B2, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#x, expected %#x\n", hr, D3D_OK);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        check_pixel_2bpp(&lockrect, 0, 0, 0x6fff);
        check_pixel_2bpp(&lockrect, 1, 0, 0xffe0);
        check_pixel_2bpp(&lockrect, 0, 1, 0xefea);
        check_pixel_2bpp(&lockrect, 1, 1, 0xca4a);
        IDirect3DSurface9_UnlockRect(surf);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a1r5g5b5,
                D3DFMT_A1R5G5B5, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#x, expected %#x\n", hr, D3D_OK);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        check_pixel_2bpp(&lockrect, 0, 0, 0x46b5);
        check_pixel_2bpp(&lockrect, 1, 0, 0x99c8);
        check_pixel_2bpp(&lockrect, 0, 1, 0x06a2);
        check_pixel_2bpp(&lockrect, 1, 1, 0x9431);
        IDirect3DSurface9_UnlockRect(surf);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_r5g6b5,
                D3DFMT_R5G6B5, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#x, expected %#x\n", hr, D3D_OK);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        check_pixel_2bpp(&lockrect, 0, 0, 0xcf76);
        check_pixel_2bpp(&lockrect, 1, 0, 0xb2cd);
        check_pixel_2bpp(&lockrect, 0, 1, 0x856e);
        check_pixel_2bpp(&lockrect, 1, 1, 0xa16e);
        IDirect3DSurface9_UnlockRect(surf);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_g16r16,
                D3DFMT_G16R16, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#x, expected %#x\n", hr, D3D_OK);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        todo_wine {
            check_pixel_2bpp(&lockrect, 0, 0, 0xa03f);
        }
        check_pixel_2bpp(&lockrect, 1, 0, 0xa37f);
        check_pixel_2bpp(&lockrect, 0, 1, 0xcb9f);
        check_pixel_2bpp(&lockrect, 1, 1, 0xfe7f);
        IDirect3DSurface9_UnlockRect(surf);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8b8g8r8,
                D3DFMT_A8B8G8R8, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#x, expected %#x\n", hr, D3D_OK);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        todo_wine {
            check_pixel_2bpp(&lockrect, 0, 0, 0xf527);
            check_pixel_2bpp(&lockrect, 1, 0, 0x4b8b);
        }
        check_pixel_2bpp(&lockrect, 0, 1, 0x7e56);
        check_pixel_2bpp(&lockrect, 1, 1, 0xf8b8);
        IDirect3DSurface9_UnlockRect(surf);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a2r10g10b10,
                D3DFMT_A2R10G10B10, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#x, expected %#x\n", hr, D3D_OK);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        check_pixel_2bpp(&lockrect, 0, 0, 0x2e57);
        todo_wine {
            check_pixel_2bpp(&lockrect, 1, 0, 0x3588);
        }
        check_pixel_2bpp(&lockrect, 0, 1, 0xe215);
        check_pixel_2bpp(&lockrect, 1, 1, 0xff0e);
        IDirect3DSurface9_UnlockRect(surf);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8l8,
                D3DFMT_A8L8, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(SUCCEEDED(hr), "Failed to load surface, hr %#x.\n", hr);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        check_pixel_2bpp(&lockrect, 0, 0, 0x8000);
        check_pixel_2bpp(&lockrect, 1, 0, 0x7fff);
        check_pixel_2bpp(&lockrect, 0, 1, 0x98c6);
        check_pixel_2bpp(&lockrect, 1, 1, 0x3def);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        check_release((IUnknown*)surf, 0);
    }

    /* A8L8 */
    hr = IDirect3DDevice9_CreateTexture(device, 2, 2, 1, 0, D3DFMT_A8L8, D3DPOOL_MANAGED, &tex, NULL);
    if (FAILED(hr))
        skip("Failed to create A8L8 texture, hr %#x.\n", hr);
    else
    {
        hr = IDirect3DTexture9_GetSurfaceLevel(tex, 0, &surf);
        ok(SUCCEEDED(hr), "Failed to get the surface, hr %#x.\n", hr);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8r3g3b2,
                D3DFMT_A8R3G3B2, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(SUCCEEDED(hr), "Failed to load surface, hr %#x.\n", hr);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        check_pixel_2bpp(&lockrect, 0, 0, 0x57f7);
        check_pixel_2bpp(&lockrect, 1, 0, 0x98ed);
        check_pixel_2bpp(&lockrect, 0, 1, 0xaceb);
        check_pixel_2bpp(&lockrect, 1, 1, 0xc88d);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a1r5g5b5,
                D3DFMT_A1R5G5B5, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#x, expected %#x\n", hr, D3D_OK);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        check_pixel_2bpp(&lockrect, 0, 0, 0x00a6);
        check_pixel_2bpp(&lockrect, 1, 0, 0xff62);
        check_pixel_2bpp(&lockrect, 0, 1, 0x007f);
        check_pixel_2bpp(&lockrect, 1, 1, 0xff19);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_r5g6b5,
                D3DFMT_R5G6B5, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(SUCCEEDED(hr), "Failed to load surface, hr %#x.\n", hr);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        check_pixel_2bpp(&lockrect, 0, 0, 0xffce);
        check_pixel_2bpp(&lockrect, 1, 0, 0xff9c);
        check_pixel_2bpp(&lockrect, 0, 1, 0xff4d);
        check_pixel_2bpp(&lockrect, 1, 1, 0xff59);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_g16r16,
                D3DFMT_G16R16, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(SUCCEEDED(hr), "Failed to load surface, hr %#x.\n", hr);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        check_pixel_2bpp(&lockrect, 0, 0, 0xff25);
        check_pixel_2bpp(&lockrect, 1, 0, 0xffbe);
        check_pixel_2bpp(&lockrect, 0, 1, 0xffd6);
        check_pixel_2bpp(&lockrect, 1, 1, 0xffb6);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8b8g8r8,
                D3DFMT_A8B8G8R8, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(SUCCEEDED(hr), "Failed to load surface, hr %#x.\n", hr);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        check_pixel_2bpp(&lockrect, 0, 0, 0xc36d);
        check_pixel_2bpp(&lockrect, 1, 0, 0x23cb);
        check_pixel_2bpp(&lockrect, 0, 1, 0x09af);
        check_pixel_2bpp(&lockrect, 1, 1, 0x8d61);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a2r10g10b10,
                D3DFMT_A2R10G10B10, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(SUCCEEDED(hr), "Failed to load surface, hr %#x.\n", hr);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        check_pixel_2bpp(&lockrect, 0, 0, 0x558c);
        check_pixel_2bpp(&lockrect, 1, 0, 0x5565);
        check_pixel_2bpp(&lockrect, 0, 1, 0xaa95);
        check_pixel_2bpp(&lockrect, 1, 1, 0xffcb);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8l8,
                D3DFMT_A8L8, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(SUCCEEDED(hr), "Failed to load surface, hr %#x.\n", hr);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        check_pixel_2bpp(&lockrect, 0, 0, 0xff00);
        check_pixel_2bpp(&lockrect, 1, 0, 0x00ff);
        check_pixel_2bpp(&lockrect, 0, 1, 0xff30);
        check_pixel_2bpp(&lockrect, 1, 1, 0x7f7f);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        check_release((IUnknown*)surf, 1);
        check_release((IUnknown*)tex, 0);
    }

    /* DXT1, DXT2, DXT3, DXT4, DXT5 */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 4, 4, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surf, NULL);
    if (FAILED(hr))
        skip("Failed to create A8R8G8B8 surface, hr %#x.\n", hr);
    else
    {
        hr = D3DXLoadSurfaceFromFileInMemory(surf, NULL, NULL, dds_24bit, sizeof(dds_24bit), NULL, D3DX_FILTER_NONE, 0, NULL);
        ok(SUCCEEDED(hr), "Failed to load surface, hr %#x.\n", hr);

        hr = IDirect3DDevice9_CreateTexture(device, 4, 4, 1, 0, D3DFMT_DXT2, D3DPOOL_SYSTEMMEM, &tex, NULL);
        if (FAILED(hr))
            skip("Failed to create DXT2 texture, hr %#x.\n", hr);
        else
        {
            hr = IDirect3DTexture9_GetSurfaceLevel(tex, 0, &newsurf);
            ok(SUCCEEDED(hr), "Failed to get the surface, hr %#x.\n", hr);
            hr = D3DXLoadSurfaceFromSurface(newsurf, NULL, NULL, surf, NULL, NULL, D3DX_FILTER_NONE, 0);
            ok(SUCCEEDED(hr), "Failed to convert pixels to DXT2 format.\n");
            check_release((IUnknown*)newsurf, 1);
            check_release((IUnknown*)tex, 0);
        }

        hr = IDirect3DDevice9_CreateTexture(device, 4, 4, 1, 0, D3DFMT_DXT3, D3DPOOL_SYSTEMMEM, &tex, NULL);
        if (FAILED(hr))
            skip("Failed to create DXT3 texture, hr %#x.\n", hr);
        else
        {
            hr = IDirect3DTexture9_GetSurfaceLevel(tex, 0, &newsurf);
            ok(SUCCEEDED(hr), "Failed to get the surface, hr %#x.\n", hr);
            hr = D3DXLoadSurfaceFromSurface(newsurf, NULL, NULL, surf, NULL, NULL, D3DX_FILTER_NONE, 0);
            ok(SUCCEEDED(hr), "Failed to convert pixels to DXT3 format.\n");
            check_release((IUnknown*)newsurf, 1);
            check_release((IUnknown*)tex, 0);
        }

        hr = IDirect3DDevice9_CreateTexture(device, 4, 4, 1, 0, D3DFMT_DXT4, D3DPOOL_SYSTEMMEM, &tex, NULL);
        if (FAILED(hr))
            skip("Failed to create DXT4 texture, hr %#x.\n", hr);
        else
        {
            hr = IDirect3DTexture9_GetSurfaceLevel(tex, 0, &newsurf);
            ok(SUCCEEDED(hr), "Failed to get the surface, hr %#x.\n", hr);
            hr = D3DXLoadSurfaceFromSurface(newsurf, NULL, NULL, surf, NULL, NULL, D3DX_FILTER_NONE, 0);
            ok(SUCCEEDED(hr), "Failed to convert pixels to DXT4 format.\n");
            check_release((IUnknown*)newsurf, 1);
            check_release((IUnknown*)tex, 0);
        }

        hr = IDirect3DDevice9_CreateTexture(device, 4, 4, 1, 0, D3DFMT_DXT5, D3DPOOL_SYSTEMMEM, &tex, NULL);
        if (FAILED(hr))
            skip("Failed to create DXT5 texture, hr %#x.\n", hr);
        else
        {
            hr = IDirect3DTexture9_GetSurfaceLevel(tex, 0, &newsurf);
            ok(SUCCEEDED(hr), "Failed to get the surface, hr %#x.\n", hr);
            hr = D3DXLoadSurfaceFromSurface(newsurf, NULL, NULL, surf, NULL, NULL, D3DX_FILTER_NONE, 0);
            ok(SUCCEEDED(hr), "Failed to convert pixels to DXT5 format.\n");

            SetRect(&rect, 0, 0, 4, 2);
            hr = D3DXLoadSurfaceFromSurface(newsurf, NULL, &rect, surf, NULL, &rect, D3DX_FILTER_NONE, 0);
            ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
            hr = D3DXLoadSurfaceFromMemory(newsurf, NULL, &rect, &dds_dxt5[128],
                    D3DFMT_DXT5, 16, NULL, &rect, D3DX_FILTER_NONE, 0);
            ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
            check_release((IUnknown *)newsurf, 1);
            check_release((IUnknown *)tex, 0);

            /* Test a rect larger than but not an integer multiple of the block size. */
            hr = IDirect3DDevice9_CreateTexture(device, 4, 8, 1, 0, D3DFMT_DXT5, D3DPOOL_SYSTEMMEM, &tex, NULL);
            ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
            hr = IDirect3DTexture9_GetSurfaceLevel(tex, 0, &newsurf);
            ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
            SetRect(&rect, 0, 0, 4, 6);
            hr = D3DXLoadSurfaceFromMemory(newsurf, NULL, &rect, &dds_dxt5[112],
                    D3DFMT_DXT5, 16, NULL, &rect, D3DX_FILTER_POINT, 0);
            ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

            check_release((IUnknown *)newsurf, 1);
            check_release((IUnknown *)tex, 0);

            /* More misalignment tests. */
            hr = IDirect3DDevice9_CreateTexture(device, 8, 8, 1, 0, D3DFMT_DXT5, D3DPOOL_SYSTEMMEM, &tex, NULL);
            ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
            hr = IDirect3DTexture9_GetSurfaceLevel(tex, 0, &newsurf);
            ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

            SetRect(&rect, 2, 2, 6, 6);
            hr = D3DXLoadSurfaceFromMemory(newsurf, NULL, NULL, &dds_dxt5_8_8[128],
                    D3DFMT_DXT5, 16 * 2, NULL, &rect, D3DX_FILTER_POINT, 0);
            ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

            hr = D3DXLoadSurfaceFromMemory(newsurf, NULL, &rect, &dds_dxt5_8_8[128],
                    D3DFMT_DXT5, 16 * 2, NULL, NULL, D3DX_FILTER_POINT, 0);
            ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

            hr = D3DXLoadSurfaceFromMemory(newsurf, NULL, &rect, &dds_dxt5_8_8[128],
                    D3DFMT_DXT5, 16 * 2, NULL, &rect, D3DX_FILTER_POINT, 0);
            ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

            check_release((IUnknown *)newsurf, 1);
            check_release((IUnknown *)tex, 0);
        }

        hr = IDirect3DDevice9_CreateTexture(device, 4, 4, 1, 0, D3DFMT_DXT1, D3DPOOL_SYSTEMMEM, &tex, NULL);
        if (FAILED(hr))
            skip("Failed to create DXT1 texture, hr %#x.\n", hr);
        else
        {
            hr = IDirect3DTexture9_GetSurfaceLevel(tex, 0, &newsurf);
            ok(SUCCEEDED(hr), "Failed to get the surface, hr %#x.\n", hr);
            hr = D3DXLoadSurfaceFromSurface(newsurf, NULL, NULL, surf, NULL, NULL, D3DX_FILTER_NONE, 0);
            ok(SUCCEEDED(hr), "Failed to convert pixels to DXT1 format.\n");

            hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, NULL, D3DX_FILTER_NONE, 0);
            ok(SUCCEEDED(hr), "Failed to convert pixels from DXT1 format.\n");

            check_release((IUnknown*)newsurf, 1);
            check_release((IUnknown*)tex, 0);
        }

        check_release((IUnknown*)surf, 0);
    }

    /* cleanup */
    if(testdummy_ok) DeleteFileA("testdummy.bmp");
    if(testbitmap_ok) DeleteFileA("testbitmap.bmp");
}

static void test_D3DXSaveSurfaceToFileInMemory(IDirect3DDevice9 *device)
{
    static const struct
    {
        DWORD usage;
        D3DPOOL pool;
    }
    test_access_types[] =
    {
        {0,  D3DPOOL_MANAGED},
        {0,  D3DPOOL_DEFAULT},
        {D3DUSAGE_RENDERTARGET, D3DPOOL_DEFAULT},
    };

    struct
    {
         DWORD magic;
         struct dds_header header;
         BYTE *data;
    } *dds;
    IDirect3DSurface9 *surface;
    IDirect3DTexture9 *texture;
    ID3DXBuffer *buffer;
    unsigned int i;
    HRESULT hr;
    RECT rect;

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 4, 4, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &surface, NULL);
    if (FAILED(hr)) {
       skip("Couldn't create surface\n");
       return;
    }

    SetRectEmpty(&rect);
    hr = D3DXSaveSurfaceToFileInMemory(&buffer, D3DXIFF_BMP, surface, NULL, &rect);
    /* fails with the debug version of d3d9 */
    ok(hr == D3D_OK || broken(hr == D3DERR_INVALIDCALL), "D3DXSaveSurfaceToFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);
    if (SUCCEEDED(hr)) {
        DWORD size = ID3DXBuffer_GetBufferSize(buffer);
        ok(size > 0, "ID3DXBuffer_GetBufferSize returned %u, expected > 0\n", size);
        ID3DXBuffer_Release(buffer);
    }

    SetRectEmpty(&rect);
    hr = D3DXSaveSurfaceToFileInMemory(&buffer, D3DXIFF_DDS, surface, NULL, &rect);
    todo_wine ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    if (SUCCEEDED(hr))
    {
        dds = ID3DXBuffer_GetBufferPointer(buffer);

        ok(dds->magic == MAKEFOURCC('D','D','S',' '), "Got unexpected DDS signature %#x.\n", dds->magic);
        ok(dds->header.size == sizeof(dds->header), "Got unexpected DDS size %u.\n", dds->header.size);
        ok(!dds->header.height, "Got unexpected height %u.\n", dds->header.height);
        ok(!dds->header.width, "Got unexpected width %u.\n", dds->header.width);
        ok(!dds->header.depth, "Got unexpected depth %u.\n", dds->header.depth);
        ok(!dds->header.miplevels, "Got unexpected miplevels %u.\n", dds->header.miplevels);
        ok(!dds->header.pitch_or_linear_size, "Got unexpected pitch_or_linear_size %u.\n", dds->header.pitch_or_linear_size);
        ok(dds->header.caps == (DDS_CAPS_TEXTURE | DDSCAPS_ALPHA), "Got unexpected caps %#x.\n", dds->header.caps);
        ok(dds->header.flags == (DDS_CAPS | DDS_HEIGHT | DDS_WIDTH | DDS_PIXELFORMAT),
                "Got unexpected flags %#x.\n", dds->header.flags);
        ID3DXBuffer_Release(buffer);
    }

    hr = D3DXSaveSurfaceToFileInMemory(&buffer, D3DXIFF_DDS, surface, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    dds = ID3DXBuffer_GetBufferPointer(buffer);
    ok(dds->magic == MAKEFOURCC('D','D','S',' '), "Got unexpected DDS signature %#x.\n", dds->magic);
    ok(dds->header.size == sizeof(dds->header), "Got unexpected DDS size %u.\n", dds->header.size);
    ok(dds->header.height == 4, "Got unexpected height %u.\n", dds->header.height);
    ok(dds->header.width == 4, "Got unexpected width %u.\n", dds->header.width);
    ok(!dds->header.depth, "Got unexpected depth %u.\n", dds->header.depth);
    ok(!dds->header.miplevels, "Got unexpected miplevels %u.\n", dds->header.miplevels);
    ok(!dds->header.pitch_or_linear_size, "Got unexpected pitch_or_linear_size %u.\n", dds->header.pitch_or_linear_size);
    todo_wine ok(dds->header.caps == (DDS_CAPS_TEXTURE | DDSCAPS_ALPHA), "Got unexpected caps %#x.\n", dds->header.caps);
    ok(dds->header.flags == (DDS_CAPS | DDS_HEIGHT | DDS_WIDTH | DDS_PIXELFORMAT),
            "Got unexpected flags %#x.\n", dds->header.flags);
    ID3DXBuffer_Release(buffer);

    IDirect3DSurface9_Release(surface);

    for (i = 0; i < ARRAY_SIZE(test_access_types); ++i)
    {
        hr = IDirect3DDevice9_CreateTexture(device, 4, 4, 0, test_access_types[i].usage,
                D3DFMT_A8R8G8B8, test_access_types[i].pool, &texture, NULL);
        ok(hr == D3D_OK, "Unexpected hr %#x, i %u.\n", hr, i);

        hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
        ok(hr == D3D_OK, "Unexpected hr %#x, i %u.\n", hr, i);

        hr = D3DXSaveSurfaceToFileInMemory(&buffer, D3DXIFF_DDS, surface, NULL, NULL);
        ok(hr == D3D_OK, "Unexpected hr %#x, i %u.\n", hr, i);
        ID3DXBuffer_Release(buffer);

        hr = D3DXSaveSurfaceToFileInMemory(&buffer, D3DXIFF_BMP, surface, NULL, NULL);
        ok(hr == D3D_OK, "Unexpected hr %#x, i %u.\n", hr, i);
        ID3DXBuffer_Release(buffer);

        IDirect3DSurface9_Release(surface);
        IDirect3DTexture9_Release(texture);
    }
}

static void test_D3DXSaveSurfaceToFile(IDirect3DDevice9 *device)
{
    static const BYTE pixels[] =
            {0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
             0x00, 0x00, 0xff, 0x00, 0x00, 0xff,};
    DWORD pitch = sizeof(pixels) / 2;
    IDirect3DSurface9 *surface;
    D3DXIMAGE_INFO image_info;
    D3DLOCKED_RECT lock_rect;
    HRESULT hr;
    RECT rect;

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 2, 2, D3DFMT_R8G8B8, D3DPOOL_SCRATCH, &surface, NULL);
    if (FAILED(hr))
    {
       skip("Couldn't create surface.\n");
       return;
    }

    SetRect(&rect, 0, 0, 2, 2);
    hr = D3DXLoadSurfaceFromMemory(surface, NULL, NULL, pixels, D3DFMT_R8G8B8,
            pitch, NULL, &rect, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = D3DXSaveSurfaceToFileA("saved_surface.bmp", D3DXIFF_BMP, surface, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = D3DXLoadSurfaceFromFileA(surface, NULL, NULL, "saved_surface.bmp",
            NULL, D3DX_FILTER_NONE, 0, &image_info);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    ok(image_info.Width == 2, "Wrong width %u.\n", image_info.Width);
    ok(image_info.Height == 2, "Wrong height %u.\n", image_info.Height);
    ok(image_info.Format == D3DFMT_R8G8B8, "Wrong format %#x.\n", image_info.Format);
    ok(image_info.ImageFileFormat == D3DXIFF_BMP, "Wrong file format %u.\n", image_info.ImageFileFormat);

    hr = IDirect3DSurface9_LockRect(surface, &lock_rect, NULL, D3DLOCK_READONLY);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    ok(!memcmp(lock_rect.pBits, pixels, pitch),
            "Pixel data mismatch in the first row.\n");
    ok(!memcmp((BYTE *)lock_rect.pBits + lock_rect.Pitch, pixels + pitch, pitch),
            "Pixel data mismatch in the second row.\n");

    IDirect3DSurface9_UnlockRect(surface);

    SetRect(&rect, 0, 1, 2, 2);
    hr = D3DXSaveSurfaceToFileA("saved_surface.bmp", D3DXIFF_BMP, surface, NULL, &rect);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    SetRect(&rect, 0, 0, 2, 1);
    hr = D3DXLoadSurfaceFromFileA(surface, NULL, &rect, "saved_surface.bmp", NULL,
            D3DX_FILTER_NONE, 0, &image_info);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DSurface9_LockRect(surface, &lock_rect, NULL, D3DLOCK_READONLY);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(!memcmp(lock_rect.pBits, pixels + pitch, pitch),
            "Pixel data mismatch in the first row.\n");
    ok(!memcmp((BYTE *)lock_rect.pBits + lock_rect.Pitch, pixels + pitch, pitch),
            "Pixel data mismatch in the second row.\n");
    IDirect3DSurface9_UnlockRect(surface);

    SetRect(&rect, 0, 0, 2, 2);
    hr = D3DXLoadSurfaceFromMemory(surface, NULL, NULL, pixels, D3DFMT_R8G8B8,
            pitch, NULL, &rect, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = D3DXSaveSurfaceToFileA(NULL, D3DXIFF_BMP, surface, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    /* PPM and TGA are supported, even though MSDN claims they aren't */
    todo_wine
    {
        hr = D3DXSaveSurfaceToFileA("saved_surface.ppm", D3DXIFF_PPM, surface, NULL, NULL);
        ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
        hr = D3DXSaveSurfaceToFileA("saved_surface.tga", D3DXIFF_TGA, surface, NULL, NULL);
        ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    }

    hr = D3DXSaveSurfaceToFileA("saved_surface.dds", D3DXIFF_DDS, surface, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = D3DXLoadSurfaceFromFileA(surface, NULL, NULL, "saved_surface.dds",
            NULL, D3DX_FILTER_NONE, 0, &image_info);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    ok(image_info.Width == 2, "Wrong width %u.\n", image_info.Width);
    ok(image_info.Format == D3DFMT_R8G8B8, "Wrong format %#x.\n", image_info.Format);
    ok(image_info.ImageFileFormat == D3DXIFF_DDS, "Wrong file format %u.\n", image_info.ImageFileFormat);

    hr = IDirect3DSurface9_LockRect(surface, &lock_rect, NULL, D3DLOCK_READONLY);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(!memcmp(lock_rect.pBits, pixels, pitch),
            "Pixel data mismatch in the first row.\n");
    ok(!memcmp((BYTE *)lock_rect.pBits + lock_rect.Pitch, pixels + pitch, pitch),
            "Pixel data mismatch in the second row.\n");
    IDirect3DSurface9_UnlockRect(surface);

    hr = D3DXSaveSurfaceToFileA("saved_surface", D3DXIFF_PFM + 1, surface, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    SetRect(&rect, 0, 0, 4, 4);
    hr = D3DXSaveSurfaceToFileA("saved_surface.bmp", D3DXIFF_BMP, surface, NULL, &rect);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    SetRect(&rect, 2, 0, 1, 4);
    hr = D3DXSaveSurfaceToFileA("saved_surface.bmp", D3DXIFF_BMP, surface, NULL, &rect);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    SetRect(&rect, 0, 2, 4, 1);
    hr = D3DXSaveSurfaceToFileA("saved_surface.bmp", D3DXIFF_BMP, surface, NULL, &rect);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    SetRect(&rect, -1, -1, 2, 2);
    hr = D3DXSaveSurfaceToFileA("saved_surface.bmp", D3DXIFF_BMP, surface, NULL, &rect);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    SetRectEmpty(&rect);
    hr = D3DXSaveSurfaceToFileA("saved_surface.bmp", D3DXIFF_BMP, surface, NULL, &rect);
    /* fails when debug version of d3d9 is used */
    ok(hr == D3D_OK || broken(hr == D3DERR_INVALIDCALL), "Got unexpected hr %#x.\n", hr);

    DeleteFileA("saved_surface.bmp");
    DeleteFileA("saved_surface.ppm");
    DeleteFileA("saved_surface.tga");
    DeleteFileA("saved_surface.dds");

    IDirect3DSurface9_Release(surface);
}

START_TEST(surface)
{
    HWND wnd;
    IDirect3D9 *d3d;
    IDirect3DDevice9 *device;
    D3DPRESENT_PARAMETERS d3dpp;
    HRESULT hr;

    if (!(wnd = CreateWindowA("static", "d3dx9_test", WS_OVERLAPPEDWINDOW, 0, 0,
            640, 480, NULL, NULL, NULL, NULL)))
    {
        skip("Couldn't create application window\n");
        return;
    }
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d) {
        skip("Couldn't create IDirect3D9 object\n");
        DestroyWindow(wnd);
        return;
    }

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed   = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device);
    if(FAILED(hr)) {
        skip("Failed to create IDirect3DDevice9 object %#x\n", hr);
        IDirect3D9_Release(d3d);
        DestroyWindow(wnd);
        return;
    }

    test_D3DXGetImageInfo();
    test_D3DXLoadSurface(device);
    test_D3DXSaveSurfaceToFileInMemory(device);
    test_D3DXSaveSurfaceToFile(device);

    check_release((IUnknown*)device, 0);
    check_release((IUnknown*)d3d, 0);
    DestroyWindow(wnd);
}
