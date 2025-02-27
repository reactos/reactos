/*
 * Copyright 2016 Nikolay Sivov for CodeWeavers
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
#include "initguid.h"
#include "d3d10_1.h"
#include "d3dx10.h"
#include "wine/test.h"

#define D3DERR_INVALIDCALL 0x8876086c

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)  \
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |  \
    ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif

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

/* 1x1 1bpp bmp image */
static const BYTE test_bmp_1bpp[] =
{
    0x42, 0x4d, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xf1, 0xf2, 0xf3, 0x80, 0xf4, 0xf5, 0xf6, 0x81, 0x00, 0x00,
    0x00, 0x00
};
static const BYTE test_bmp_1bpp_data[] =
{
    0xf3, 0xf2, 0xf1, 0xff
};

/* 1x1 2bpp bmp image */
static const BYTE test_bmp_2bpp[] =
{
    0x42, 0x4d, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xf1, 0xf2, 0xf3, 0x80, 0xf4, 0xf5, 0xf6, 0x81, 0x00, 0x00,
    0x00, 0x00
};

/* 1x1 4bpp bmp image */
static const BYTE test_bmp_4bpp[] =
{
    0x42, 0x4d, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xf1, 0xf2, 0xf3, 0x80, 0xf4, 0xf5, 0xf6, 0x81, 0x00, 0x00,
    0x00, 0x00
};
static const BYTE test_bmp_4bpp_data[] =
{
    0xf3, 0xf2, 0xf1, 0xff
};

/* 1x1 8bpp bmp image */
static const BYTE test_bmp_8bpp[] =
{
    0x42, 0x4d, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xf1, 0xf2, 0xf3, 0x80, 0xf4, 0xf5, 0xf6, 0x81, 0x00, 0x00,
    0x00, 0x00
};
static const BYTE test_bmp_8bpp_data[] =
{
    0xf3, 0xf2, 0xf1, 0xff
};

/* 1x1 16bpp bmp image */
static const BYTE test_bmp_16bpp[] =
{
    0x42, 0x4d, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x42, 0x00, 0x00, 0x00, 0x00
};
static const BYTE test_bmp_16bpp_data[] =
{
    0x84, 0x84, 0x73, 0xff
};

/* 1x1 24bpp bmp image */
static const BYTE test_bmp_24bpp[] =
{
    0x42, 0x4d, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x73, 0x84, 0x84, 0x00, 0x00, 0x00
};
static const BYTE test_bmp_24bpp_data[] =
{
    0x84, 0x84, 0x73, 0xff
};

/* 2x2 32bpp XRGB bmp image */
static const BYTE test_bmp_32bpp_xrgb[] =
{
    0x42, 0x4d, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0xb0, 0xc0, 0x00, 0xa1, 0xb1, 0xc1, 0x00, 0xa2, 0xb2,
    0xc2, 0x00, 0xa3, 0xb3, 0xc3, 0x00
};
static const BYTE test_bmp_32bpp_xrgb_data[] =
{
    0xc2, 0xb2, 0xa2, 0xff, 0xc3, 0xb3, 0xa3, 0xff, 0xc0, 0xb0, 0xa0, 0xff, 0xc1, 0xb1, 0xa1, 0xff

};

/* 2x2 32bpp ARGB bmp image */
static const BYTE test_bmp_32bpp_argb[] =
{
    0x42, 0x4d, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0xb0, 0xc0, 0x00, 0xa1, 0xb1, 0xc1, 0x00, 0xa2, 0xb2,
    0xc2, 0x00, 0xa3, 0xb3, 0xc3, 0x01
};
static const BYTE test_bmp_32bpp_argb_data[] =
{
    0xc2, 0xb2, 0xa2, 0xff, 0xc3, 0xb3, 0xa3, 0xff, 0xc0, 0xb0, 0xa0, 0xff, 0xc1, 0xb1, 0xa1, 0xff

};

/* 1x1 8bpp gray png image */
static const BYTE test_png_8bpp_gray[] =
{
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x3a, 0x7e, 0x9b,
    0x55, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44, 0x41, 0x54, 0x08, 0xd7, 0x63, 0xf8, 0x0f, 0x00, 0x01,
    0x01, 0x01, 0x00, 0x1b, 0xb6, 0xee, 0x56, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae,
    0x42, 0x60, 0x82
};
static const BYTE test_png_8bpp_gray_data[] =
{
    0xff, 0xff, 0xff, 0xff
};

/* 1x1 jpg image */
static const BYTE test_jpg[] =
{
    0xff, 0xd8, 0xff, 0xe0, 0x00, 0x10, 0x4a, 0x46, 0x49, 0x46, 0x00, 0x01, 0x01, 0x01, 0x01, 0x2c,
    0x01, 0x2c, 0x00, 0x00, 0xff, 0xdb, 0x00, 0x43, 0x00, 0x05, 0x03, 0x04, 0x04, 0x04, 0x03, 0x05,
    0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x06, 0x07, 0x0c, 0x08, 0x07, 0x07, 0x07, 0x07, 0x0f, 0x0b,
    0x0b, 0x09, 0x0c, 0x11, 0x0f, 0x12, 0x12, 0x11, 0x0f, 0x11, 0x11, 0x13, 0x16, 0x1c, 0x17, 0x13,
    0x14, 0x1a, 0x15, 0x11, 0x11, 0x18, 0x21, 0x18, 0x1a, 0x1d, 0x1d, 0x1f, 0x1f, 0x1f, 0x13, 0x17,
    0x22, 0x24, 0x22, 0x1e, 0x24, 0x1c, 0x1e, 0x1f, 0x1e, 0xff, 0xdb, 0x00, 0x43, 0x01, 0x05, 0x05,
    0x05, 0x07, 0x06, 0x07, 0x0e, 0x08, 0x08, 0x0e, 0x1e, 0x14, 0x11, 0x14, 0x1e, 0x1e, 0x1e, 0x1e,
    0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
    0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
    0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0xff, 0xc0,
    0x00, 0x11, 0x08, 0x00, 0x01, 0x00, 0x01, 0x03, 0x01, 0x22, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11,
    0x01, 0xff, 0xc4, 0x00, 0x15, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0xff, 0xc4, 0x00, 0x14, 0x10, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xc4,
    0x00, 0x14, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xc4, 0x00, 0x14, 0x11, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xda, 0x00, 0x0c, 0x03, 0x01,
    0x00, 0x02, 0x11, 0x03, 0x11, 0x00, 0x3f, 0x00, 0xb2, 0xc0, 0x07, 0xff, 0xd9
};
static const BYTE test_jpg_data[] =
{
    0xff, 0xff, 0xff, 0xff
};

/* 1x1 gif image */
static const BYTE test_gif[] =
{
    0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x01, 0x00, 0x01, 0x00, 0x80, 0x00, 0x00, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x02, 0x02, 0x44,
    0x01, 0x00, 0x3b
};
static const BYTE test_gif_data[] =
{
    0xff, 0xff, 0xff, 0xff
};

/* 1x1 tiff image */
static const BYTE test_tiff[] =
{
    0x49, 0x49, 0x2a, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0xfe, 0x00,
    0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x02, 0x01, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0xd2, 0x00, 0x00, 0x00, 0x03, 0x01,
    0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0d, 0x01, 0x02, 0x00, 0x1b, 0x00, 0x00, 0x00, 0xd8, 0x00,
    0x00, 0x00, 0x11, 0x01, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x12, 0x01,
    0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x15, 0x01, 0x03, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x16, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x40, 0x00,
    0x00, 0x00, 0x17, 0x01, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x1a, 0x01,
    0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0xf4, 0x00, 0x00, 0x00, 0x1b, 0x01, 0x05, 0x00, 0x01, 0x00,
    0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 0x1c, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x28, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x08, 0x00, 0x08, 0x00, 0x08, 0x00, 0x2f, 0x68, 0x6f, 0x6d, 0x65, 0x2f, 0x6d, 0x65,
    0x68, 0x2f, 0x44, 0x65, 0x73, 0x6b, 0x74, 0x6f, 0x70, 0x2f, 0x74, 0x65, 0x73, 0x74, 0x2e, 0x74,
    0x69, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x48,
    0x00, 0x00, 0x00, 0x01
};
static const BYTE test_tiff_data[] =
{
    0x00, 0x00, 0x00, 0xff
};

/* 1x1 alpha dds image */
static const BYTE test_dds_alpha[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff
};
static const BYTE test_dds_alpha_data[] =
{
    0xff
};

/* 1x1 luminance dds image */
static const BYTE test_dds_luminance[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x82
};
static const BYTE test_dds_luminance_data[] =
{
    0x82, 0x82, 0x82, 0xff
};

/* 1x1 16bpp dds image */
static const BYTE test_dds_16bpp[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00,
    0xe0, 0x03, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0e, 0x42
};
static const BYTE test_dds_16bpp_data[] =
{
    0x84, 0x84, 0x73, 0xff
};

/* 1x1 24bpp dds image */
static const BYTE test_dds_24bpp[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00,
    0x00, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x70, 0x81, 0x83
};
static const BYTE test_dds_24bpp_data[] =
{
    0x83, 0x81, 0x70, 0xff
};

/* 1x1 32bpp dds image */
static const BYTE test_dds_32bpp[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00,
    0x00, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x70, 0x81, 0x83, 0xff
};
static const BYTE test_dds_32bpp_data[] =
{
    0x83, 0x81, 0x70, 0xff
};

/* 1x1 64bpp dds image */
static const BYTE test_dds_64bpp[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x0f, 0x10, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x83, 0x83, 0x81, 0x81, 0x70, 0x70, 0xff, 0xff
};
static const BYTE test_dds_64bpp_data[] =
{
    0x83, 0x83, 0x81, 0x81, 0x70, 0x70, 0xff, 0xff
};

/* 1x1 96bpp dds image */
static const BYTE test_dds_96bpp[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x0f, 0x10, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x44, 0x58, 0x31, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x84, 0x83, 0x03, 0x3f, 0x82, 0x81, 0x01, 0x3f, 0xe2, 0xe0, 0xe0, 0x3e
};
static const BYTE test_dds_96bpp_data[] =
{
    0x84, 0x83, 0x03, 0x3f, 0x82, 0x81, 0x01, 0x3f, 0xe2, 0xe0, 0xe0, 0x3e
};

/* 1x1 128bpp dds image */
static const BYTE test_dds_128bpp[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x0f, 0x10, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x84, 0x83, 0x03, 0x3f, 0x82, 0x81, 0x01, 0x3f, 0xe2, 0xe0, 0xe0, 0x3e, 0x00, 0x00, 0x80, 0x3f
};
static const BYTE test_dds_128bpp_data[] =
{
    0x84, 0x83, 0x03, 0x3f, 0x82, 0x81, 0x01, 0x3f, 0xe2, 0xe0, 0xe0, 0x3e, 0x00, 0x00, 0x80, 0x3f

};

/* 4x4 DXT1 dds image */
static const BYTE test_dds_dxt1[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x44, 0x58, 0x54, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2a, 0x31, 0xf5, 0xbc, 0xe3, 0x6e, 0x2a, 0x3a
};
static const BYTE test_dds_dxt1_data[] =
{
    0x2a, 0x31, 0xf5, 0xbc, 0xe3, 0x6e, 0x2a, 0x3a
};

/* 4x8 DXT1 dds image */
static const BYTE test_dds_dxt1_4x8[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x0a, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x44, 0x58, 0x54, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x92, 0xce, 0x09, 0x7a, 0x5d, 0xdd, 0xa7, 0x26, 0x55, 0xde, 0xaf, 0x52, 0xbc, 0xf8, 0x6c, 0x44,
    0x53, 0xbd, 0x8b, 0x72, 0x55, 0x33, 0x88, 0xaa, 0xb2, 0x9c, 0x6c, 0x93, 0x55, 0x00, 0x55, 0x00,
    0x0f, 0x9c, 0x0f, 0x9c, 0x00, 0x00, 0x00, 0x00,
};
static const BYTE test_dds_dxt1_4x8_data[] =
{
    0x92, 0xce, 0x09, 0x7a, 0x5d, 0xdd, 0xa7, 0x26, 0x55, 0xde, 0xaf, 0x52, 0xbc, 0xf8, 0x6c, 0x44,
};

/* 4x4 DXT2 dds image */
static const BYTE test_dds_dxt2[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x44, 0x58, 0x54, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfd, 0xde, 0xc4, 0x10, 0x2f, 0xbf, 0xff, 0x7b,
    0xff, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x57, 0x53, 0x00, 0x00, 0x52, 0x52, 0x55, 0x55,
    0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xce, 0x59, 0x00, 0x00, 0x54, 0x55, 0x55, 0x55
};
static const BYTE test_dds_dxt2_data[] =
{
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfd, 0xde, 0xc4, 0x10, 0x2f, 0xbf, 0xff, 0x7b

};

/* 1x3 DXT3 dds image */
static const BYTE test_dds_dxt3[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x0a, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x44, 0x58, 0x54, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0c, 0x92, 0x38, 0x84, 0x00, 0xff, 0x55, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x53, 0x8b, 0x53, 0x8b, 0x00, 0x00, 0x00, 0x00
};
static const BYTE test_dds_dxt3_data[] =
{
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x4e, 0x92, 0xd6, 0x83, 0x00, 0xaa, 0x55, 0x55

};

/* 4x4 DXT4 dds image */
static const BYTE test_dds_dxt4[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x44, 0x58, 0x54, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfd, 0xde, 0xc4, 0x10, 0x2f, 0xbf, 0xff, 0x7b,
    0xff, 0x00, 0x40, 0x02, 0x24, 0x49, 0x92, 0x24, 0x57, 0x53, 0x00, 0x00, 0x52, 0x52, 0x55, 0x55,
    0xff, 0x00, 0x48, 0x92, 0x24, 0x49, 0x92, 0x24, 0xce, 0x59, 0x00, 0x00, 0x54, 0x55, 0x55, 0x55
};
static const BYTE test_dds_dxt4_data[] =
{
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfd, 0xde, 0xc4, 0x10, 0x2f, 0xbf, 0xff, 0x7b

};

/* 4x2 DXT5 dds image */
static const BYTE test_dds_dxt5[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x44, 0x58, 0x54, 0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xef, 0x87, 0x0f, 0x78, 0x05, 0x05, 0x50, 0x50
};
static const BYTE test_dds_dxt5_data[] =
{
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xef, 0x87, 0x0f, 0x78, 0x05, 0x05, 0x05, 0x05

};

/* 8x8 DXT5 dds image */
static const BYTE test_dds_dxt5_8x8[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x0a, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x44, 0x58, 0x54, 0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4b, 0x8a, 0x72, 0x39, 0x5e, 0x5e, 0xfa, 0xa8,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0xd7, 0xd5, 0x4a, 0x2d, 0x2d, 0xad, 0xfd,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x9a, 0x73, 0x83, 0xa0, 0xf0, 0x78, 0x78,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x5b, 0x06, 0x19, 0x00, 0xe8, 0x78, 0x58,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39, 0xbe, 0x8c, 0x49, 0x35, 0xb5, 0xff, 0x7f,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x96, 0x84, 0xab, 0x59, 0x11, 0xff, 0x11, 0xff,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x6a, 0xf0, 0x6a, 0x00, 0x00, 0x00, 0x00,
};
static const BYTE test_dds_dxt5_8x8_data[] =
{
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4b, 0x8a, 0x72, 0x39, 0x5e, 0x5e, 0xfa, 0xa8,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0xd7, 0xd5, 0x4a, 0x2d, 0x2d, 0xad, 0xfd,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x9a, 0x73, 0x83, 0xa0, 0xf0, 0x78, 0x78,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x5b, 0x06, 0x19, 0x00, 0xe8, 0x78, 0x58,
};

/* 4x4 BC4 dds image */
static const BYTE test_dds_bc4[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x0a, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x42, 0x43, 0x34, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xd9, 0x15, 0xbc, 0x41, 0x5b, 0xa3, 0x3d, 0x3a, 0x8f, 0x3d, 0x45, 0x81, 0x20, 0x45, 0x81, 0x20,
    0x6f, 0x6f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const BYTE test_dds_bc4_data[] =
{
    0xd9, 0x15, 0xbc, 0x41, 0x5b, 0xa3, 0x3d, 0x3a
};

/* 6x3 BC5 dds image */
static const BYTE test_dds_bc5[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x0a, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x42, 0x43, 0x35, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x9f, 0x28, 0x73, 0xac, 0xd5, 0x80, 0xaa, 0xd5, 0x70, 0x2c, 0x4e, 0xd6, 0x76, 0x1d, 0xd6, 0x76,
    0xd5, 0x0f, 0xc3, 0x50, 0x96, 0xcf, 0x53, 0x96, 0xdf, 0x16, 0xc3, 0x50, 0x96, 0xcf, 0x53, 0x96,
    0x83, 0x55, 0x08, 0x83, 0x30, 0x08, 0x83, 0x30, 0x79, 0x46, 0x31, 0x1c, 0xc3, 0x31, 0x1c, 0xc3,
    0x6d, 0x6d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c, 0x5c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const BYTE test_dds_bc5_data[] =
{
    0x95, 0x35, 0xe2, 0xa3, 0xf5, 0xd2, 0x28, 0x68, 0x65, 0x32, 0x7c, 0x4e, 0xdb, 0xe4, 0x56, 0x0a,
    0xb9, 0x33, 0xaf, 0xf0, 0x52, 0xbe, 0xed, 0x27, 0xb4, 0x2e, 0xa6, 0x60, 0x4e, 0xb6, 0x5d, 0x3f

};

/* 4x4 DXT1 cube map */
static const BYTE test_dds_cube[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x0a, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x44, 0x58, 0x54, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7, 0x32, 0x96, 0x0b, 0x7b, 0xcc, 0x55, 0xcc, 0x55,
    0x0e, 0x84, 0x0e, 0x84, 0x00, 0x00, 0x00, 0x00, 0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7,
    0x32, 0x96, 0x0b, 0x7b, 0xcc, 0x55, 0xcc, 0x55, 0x0e, 0x84, 0x0e, 0x84, 0x00, 0x00, 0x00, 0x00,
    0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7, 0x32, 0x96, 0x0b, 0x7b, 0xcc, 0x55, 0xcc, 0x55,
    0x0e, 0x84, 0x0e, 0x84, 0x00, 0x00, 0x00, 0x00, 0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7,
    0x32, 0x96, 0x0b, 0x7b, 0xcc, 0x55, 0xcc, 0x55, 0x0e, 0x84, 0x0e, 0x84, 0x00, 0x00, 0x00, 0x00,
    0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7, 0x32, 0x96, 0x0b, 0x7b, 0xcc, 0x55, 0xcc, 0x55,
    0x0e, 0x84, 0x0e, 0x84, 0x00, 0x00, 0x00, 0x00, 0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7,
    0x32, 0x96, 0x0b, 0x7b, 0xcc, 0x55, 0xcc, 0x55, 0x0e, 0x84, 0x0e, 0x84, 0x00, 0x00, 0x00, 0x00
};
static const BYTE test_dds_cube_data[] =
{
    0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7,
    0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7,
    0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7,
    0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7,
    0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7,
    0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7
};

/* 4x4x2 DXT3 volume dds, 2 mipmaps */
static const BYTE test_dds_volume[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x8a, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x44, 0x58, 0x54, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0x87, 0x0f, 0x78, 0x05, 0x05, 0x50, 0x50,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0x87, 0x0f, 0x78, 0x05, 0x05, 0x50, 0x50,
    0xff, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2f, 0x7e, 0xcf, 0x79, 0x01, 0x54, 0x5c, 0x5c,
    0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x84, 0xef, 0x7b, 0xaa, 0xab, 0xab, 0xab
};
static const BYTE test_dds_volume_data[] =
{
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0x87, 0x0f, 0x78, 0x05, 0x05, 0x50, 0x50,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0x87, 0x0f, 0x78, 0x05, 0x05, 0x50, 0x50,
};

/* 1x1 wmp image */
static const BYTE test_wmp[] =
{
    0x49, 0x49, 0xbc, 0x01, 0x20, 0x00, 0x00, 0x00, 0x24, 0xc3, 0xdd, 0x6f, 0x03, 0x4e, 0xfe, 0x4b,
    0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x01, 0xbc, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x02, 0xbc,
    0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xbc, 0x04, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x81, 0xbc, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x82, 0xbc, 0x0b, 0x00, 0x01, 0x00, 0x00, 0x00, 0x25, 0x06, 0xc0, 0x42, 0x83, 0xbc,
    0x0b, 0x00, 0x01, 0x00, 0x00, 0x00, 0x25, 0x06, 0xc0, 0x42, 0xc0, 0xbc, 0x04, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x86, 0x00, 0x00, 0x00, 0xc1, 0xbc, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x92, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x57, 0x4d, 0x50, 0x48, 0x4f, 0x54, 0x4f, 0x00, 0x11, 0x45,
    0xc0, 0x71, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0xc0, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0xc0,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x25, 0xff, 0xff, 0x00, 0x00, 0x01,
    0x01, 0xc8, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x04, 0x10, 0x10, 0xa6, 0x18, 0x8c, 0x21,
    0x00, 0xc4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x4e, 0x0f, 0x3a, 0x4c, 0x94, 0x9d, 0xba, 0x79, 0xe7, 0x38,
    0x4c, 0xcf, 0x14, 0xc3, 0x43, 0x91, 0x88, 0xfb, 0xdc, 0xe0, 0x7c, 0x34, 0x70, 0x9b, 0x28, 0xa9,
    0x18, 0x74, 0x62, 0x87, 0x8e, 0xe4, 0x68, 0x5f, 0xb9, 0xcc, 0x0e, 0xe1, 0x8c, 0x76, 0x3a, 0x9b,
    0x82, 0x76, 0x71, 0x13, 0xde, 0x50, 0xd4, 0x2d, 0xc2, 0xda, 0x1e, 0x3b, 0xa6, 0xa1, 0x62, 0x7b,
    0xca, 0x1a, 0x85, 0x4b, 0x6e, 0x74, 0xec, 0x60
};
static const BYTE test_wmp_data[] =
{
    0xff, 0xff, 0xff, 0xff
};

static const char *test_fx_source =
"cbuffer cb : register(b1)\n"
"{\n"
"    float f1 : SV_POSITION;\n"
"    float f2 : COLOR0;\n"
"}\n";

static const BYTE test_fx[] =
{
    0x44, 0x58, 0x42, 0x43, 0x95, 0x89, 0xe1, 0xa2, 0xcc, 0x97, 0x05, 0x54, 0x73, 0x9d, 0x0b, 0x67,
    0x90, 0xe1, 0x7f, 0x77, 0x01, 0x00, 0x00, 0x00, 0x0a, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x24, 0x00, 0x00, 0x00, 0x46, 0x58, 0x31, 0x30, 0xde, 0x00, 0x00, 0x00, 0x01, 0x10, 0xff, 0xfe,
    0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x62, 0x00, 0x66,
    0x6c, 0x6f, 0x61, 0x74, 0x00, 0x07, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x04, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x09, 0x09, 0x00,
    0x00, 0x66, 0x31, 0x00, 0x53, 0x56, 0x5f, 0x50, 0x4f, 0x53, 0x49, 0x54, 0x49, 0x4f, 0x4e, 0x00,
    0x66, 0x32, 0x00, 0x43, 0x4f, 0x4c, 0x4f, 0x52, 0x30, 0x00, 0x04, 0x00, 0x00, 0x00, 0x10, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00,
    0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const struct test_image
{
    const BYTE *data;
    unsigned int size;
    const BYTE *expected_data;
    D3DX10_IMAGE_INFO expected_info;
}
test_image[] =
{
    {
        test_bmp_1bpp,       sizeof(test_bmp_1bpp),          test_bmp_1bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_BMP}
    },
    {
        test_bmp_4bpp,       sizeof(test_bmp_4bpp),          test_bmp_4bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_BMP}
    },
    {
        test_bmp_8bpp,       sizeof(test_bmp_8bpp),          test_bmp_8bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_BMP}
    },
    {
        test_bmp_16bpp,      sizeof(test_bmp_16bpp),         test_bmp_16bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_BMP}
    },
    {
        test_bmp_24bpp,      sizeof(test_bmp_24bpp),         test_bmp_24bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_BMP}
    },
    {
        test_bmp_32bpp_xrgb, sizeof(test_bmp_32bpp_xrgb),    test_bmp_32bpp_xrgb_data,
        {2, 2, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_BMP}
    },
    {
        test_bmp_32bpp_argb, sizeof(test_bmp_32bpp_argb),    test_bmp_32bpp_argb_data,
        {2, 2, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_BMP}
    },
    {
        test_png_8bpp_gray,  sizeof(test_png_8bpp_gray),     test_png_8bpp_gray_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_PNG}
    },
    {
        test_jpg,            sizeof(test_jpg),               test_jpg_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_JPG}
    },
    {
        test_gif,            sizeof(test_gif),               test_gif_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_GIF}
    },
    {
        test_tiff,           sizeof(test_tiff),              test_tiff_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_TIFF}
    },
    {
        test_dds_alpha,      sizeof(test_dds_alpha),         test_dds_alpha_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_A8_UNORM,           D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_DDS}
    },
    {
        test_dds_luminance,  sizeof(test_dds_luminance),     test_dds_luminance_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_DDS}
    },
    {
        test_dds_16bpp,      sizeof(test_dds_16bpp),         test_dds_16bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_DDS}
    },
    {
        test_dds_24bpp,      sizeof(test_dds_24bpp),         test_dds_24bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_DDS}
    },
    {
        test_dds_32bpp,      sizeof(test_dds_32bpp),         test_dds_32bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_DDS}
    },
    {
        test_dds_64bpp,      sizeof(test_dds_64bpp),         test_dds_64bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R16G16B16A16_UNORM, D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_DDS}
    },
    {
        test_dds_96bpp,      sizeof(test_dds_96bpp),         test_dds_96bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R32G32B32_FLOAT,    D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_DDS}
    },
    {
        test_dds_128bpp,     sizeof(test_dds_128bpp),        test_dds_128bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R32G32B32A32_FLOAT, D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_DDS}
    },
    {
        test_dds_dxt1,       sizeof(test_dds_dxt1),          test_dds_dxt1_data,
        {4, 4, 1, 1, 1, 0,   DXGI_FORMAT_BC1_UNORM,          D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_DDS}
    },
    {
        test_dds_dxt1_4x8,   sizeof(test_dds_dxt1_4x8),      test_dds_dxt1_4x8_data,
        {4, 8, 1, 1, 4, 0,   DXGI_FORMAT_BC1_UNORM,          D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_DDS}
    },
    {
        test_dds_dxt2,       sizeof(test_dds_dxt2),          test_dds_dxt2_data,
        {4, 4, 1, 1, 3, 0,   DXGI_FORMAT_BC2_UNORM,          D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_DDS}
    },
    {
        test_dds_dxt3,       sizeof(test_dds_dxt3),          test_dds_dxt3_data,
        {1, 3, 1, 1, 2, 0,   DXGI_FORMAT_BC2_UNORM,          D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_DDS}
    },
    {
        test_dds_dxt4,       sizeof(test_dds_dxt4),          test_dds_dxt4_data,
        {4, 4, 1, 1, 3, 0,   DXGI_FORMAT_BC3_UNORM,          D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_DDS}
    },
    {
        test_dds_dxt5,       sizeof(test_dds_dxt5),          test_dds_dxt5_data,
        {4, 2, 1, 1, 1, 0,   DXGI_FORMAT_BC3_UNORM,          D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_DDS}
    },
    {
        test_dds_dxt5_8x8,   sizeof(test_dds_dxt5_8x8),      test_dds_dxt5_8x8_data,
        {8, 8, 1, 1, 4, 0,   DXGI_FORMAT_BC3_UNORM,          D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_DDS}
    },
    {
        test_dds_bc4,        sizeof(test_dds_bc4),           test_dds_bc4_data,
        {4, 4, 1, 1, 3, 0,   DXGI_FORMAT_BC4_UNORM,          D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_DDS}
    },
    {
        test_dds_bc5,        sizeof(test_dds_bc5),           test_dds_bc5_data,
        {6, 3, 1, 1, 3, 0,   DXGI_FORMAT_BC5_UNORM,          D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_DDS}
    },
    {
        test_dds_cube,       sizeof(test_dds_cube),          test_dds_cube_data,
        {4, 4, 1, 6, 3, 0x4, DXGI_FORMAT_BC1_UNORM,          D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_DDS}
    },
    {
        test_dds_volume,     sizeof(test_dds_volume),        test_dds_volume_data,
        {4, 4, 2, 1, 3, 0,   DXGI_FORMAT_BC2_UNORM,          D3D10_RESOURCE_DIMENSION_TEXTURE3D, D3DX10_IFF_DDS}
    },
    {
        test_wmp,            sizeof(test_wmp),               test_wmp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D10_RESOURCE_DIMENSION_TEXTURE2D, D3DX10_IFF_WMP}
    },
};

static WCHAR temp_dir[MAX_PATH];

static DXGI_FORMAT block_compressed_formats[] =
{
    DXGI_FORMAT_BC1_TYPELESS,  DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_BC1_UNORM_SRGB,
    DXGI_FORMAT_BC2_TYPELESS,  DXGI_FORMAT_BC2_UNORM, DXGI_FORMAT_BC2_UNORM_SRGB,
    DXGI_FORMAT_BC3_TYPELESS,  DXGI_FORMAT_BC3_UNORM, DXGI_FORMAT_BC3_UNORM_SRGB,
    DXGI_FORMAT_BC4_TYPELESS,  DXGI_FORMAT_BC4_UNORM, DXGI_FORMAT_BC4_SNORM,
    DXGI_FORMAT_BC5_TYPELESS,  DXGI_FORMAT_BC5_UNORM, DXGI_FORMAT_BC5_SNORM,
    DXGI_FORMAT_BC6H_TYPELESS, DXGI_FORMAT_BC6H_UF16, DXGI_FORMAT_BC6H_SF16,
    DXGI_FORMAT_BC7_TYPELESS,  DXGI_FORMAT_BC7_UNORM, DXGI_FORMAT_BC7_UNORM_SRGB
};

static BOOL is_block_compressed(DXGI_FORMAT format)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(block_compressed_formats); ++i)
        if (format == block_compressed_formats[i])
            return TRUE;

    return FALSE;
}

static unsigned int get_bpp_from_format(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return 128;
        case DXGI_FORMAT_R32G32B32_TYPELESS:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
            return 96;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R32G32_TYPELESS:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        case DXGI_FORMAT_Y416:
        case DXGI_FORMAT_Y210:
        case DXGI_FORMAT_Y216:
            return 64;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_R16G16_TYPELESS:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        case DXGI_FORMAT_R8G8_B8G8_UNORM:
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DXGI_FORMAT_AYUV:
        case DXGI_FORMAT_Y410:
        case DXGI_FORMAT_YUY2:
            return 32;
        case DXGI_FORMAT_P010:
        case DXGI_FORMAT_P016:
            return 24;
        case DXGI_FORMAT_R8G8_TYPELESS:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_B5G6R5_UNORM:
        case DXGI_FORMAT_B5G5R5A1_UNORM:
        case DXGI_FORMAT_A8P8:
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            return 16;
        case DXGI_FORMAT_NV12:
        case DXGI_FORMAT_420_OPAQUE:
        case DXGI_FORMAT_NV11:
            return 12;
        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_A8_UNORM:
        case DXGI_FORMAT_AI44:
        case DXGI_FORMAT_IA44:
        case DXGI_FORMAT_P8:
        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return 8;
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
            return 4;
        case DXGI_FORMAT_R1_UNORM:
            return 1;
        default:
            return 0;
    }
}

static HRESULT WINAPI D3DX10ThreadPump_QueryInterface(ID3DX10ThreadPump *iface, REFIID riid, void **out)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI D3DX10ThreadPump_AddRef(ID3DX10ThreadPump *iface)
{
    return 2;
}

static ULONG WINAPI D3DX10ThreadPump_Release(ID3DX10ThreadPump *iface)
{
    return 1;
}

static int add_work_item_count = 1;

static HRESULT WINAPI D3DX10ThreadPump_AddWorkItem(ID3DX10ThreadPump *iface, ID3DX10DataLoader *loader,
        ID3DX10DataProcessor *processor, HRESULT *result, void **object)
{
    SIZE_T size;
    void *data;
    HRESULT hr;

    ok(!add_work_item_count++, "unexpected call\n");

    hr = ID3DX10DataLoader_Load(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX10DataLoader_Decompress(loader, &data, &size);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX10DataProcessor_Process(processor, data, size);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX10DataProcessor_CreateDeviceObject(processor, object);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX10DataProcessor_Destroy(processor);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX10DataLoader_Destroy(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    if (result) *result = S_OK;
    return S_OK;
}

static UINT WINAPI D3DX10ThreadPump_GetWorkItemCount(ID3DX10ThreadPump *iface)
{
    ok(0, "unexpected call\n");
    return 0;
}

static HRESULT WINAPI D3DX10ThreadPump_WaitForAllItems(ID3DX10ThreadPump *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI D3DX10ThreadPump_ProcessDeviceWorkItems(ID3DX10ThreadPump *iface, UINT count)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI D3DX10ThreadPump_PurgeAllItems(ID3DX10ThreadPump *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI D3DX10ThreadPump_GetQueueStatus(ID3DX10ThreadPump *iface, UINT *queue,
        UINT *processqueue, UINT *devicequeue)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ID3DX10ThreadPumpVtbl D3DX10ThreadPumpVtbl =
{
    D3DX10ThreadPump_QueryInterface,
    D3DX10ThreadPump_AddRef,
    D3DX10ThreadPump_Release,
    D3DX10ThreadPump_AddWorkItem,
    D3DX10ThreadPump_GetWorkItemCount,
    D3DX10ThreadPump_WaitForAllItems,
    D3DX10ThreadPump_ProcessDeviceWorkItems,
    D3DX10ThreadPump_PurgeAllItems,
    D3DX10ThreadPump_GetQueueStatus
};
static ID3DX10ThreadPump thread_pump = { &D3DX10ThreadPumpVtbl };

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

static BOOL compare_uint(unsigned int x, unsigned int y, unsigned int max_diff)
{
    unsigned int diff = x > y ? x - y : y - x;

    return diff <= max_diff;
}

static BOOL compare_float(float f, float g, unsigned int ulps)
{
    int x = *(int *)&f;
    int y = *(int *)&g;

    if (x < 0)
        x = INT_MIN - x;
    if (y < 0)
        y = INT_MIN - y;

    return compare_uint(x, y, ulps);
}

static char *get_str_a(const WCHAR *wstr)
{
    static char buffer[MAX_PATH];

    WideCharToMultiByte(CP_ACP, 0, wstr, -1, buffer, sizeof(buffer), NULL, NULL);
    return buffer;
}

static BOOL create_file(const WCHAR *filename, const void *data, unsigned int size, WCHAR *out_path)
{
    WCHAR path[MAX_PATH];
    DWORD written;
    HANDLE file;

    if (!temp_dir[0])
        GetTempPathW(ARRAY_SIZE(temp_dir), temp_dir);
    lstrcpyW(path, temp_dir);
    lstrcatW(path, filename);

    file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE)
        return FALSE;

    if (WriteFile(file, data, size, &written, NULL))
    {
        CloseHandle(file);

        if (out_path)
            lstrcpyW(out_path, path);
        return TRUE;
    }

    CloseHandle(file);
    return FALSE;
}

static BOOL delete_file(const WCHAR *filename)
{
    WCHAR path[MAX_PATH];

    lstrcpyW(path, temp_dir);
    lstrcatW(path, filename);
    return DeleteFileW(path);
}

static ID3D10Device *create_device(void)
{
    ID3D10Device *device;
    HMODULE d3d10_mod = LoadLibraryA("d3d10.dll");
    HRESULT (WINAPI *pD3D10CreateDevice)(IDXGIAdapter *, D3D10_DRIVER_TYPE, HMODULE, UINT, UINT, ID3D10Device **);

    if (!d3d10_mod)
    {
        win_skip("d3d10.dll not present\n");
        return NULL;
    }

    pD3D10CreateDevice = (void *)GetProcAddress(d3d10_mod, "D3D10CreateDevice");
    if (SUCCEEDED(pD3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &device)))
        return device;
    if (SUCCEEDED(pD3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_WARP, NULL, 0, D3D10_SDK_VERSION, &device)))
        return device;
    if (SUCCEEDED(pD3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_REFERENCE, NULL, 0, D3D10_SDK_VERSION, &device)))
        return device;

    return NULL;
}

static HMODULE create_resource_module(const WCHAR *filename, const void *data, unsigned int size)
{
    WCHAR resource_module_path[MAX_PATH], current_module_path[MAX_PATH];
    HANDLE resource;
    HMODULE module;
    BOOL ret;

    if (!temp_dir[0])
        GetTempPathW(ARRAY_SIZE(temp_dir), temp_dir);
    lstrcpyW(resource_module_path, temp_dir);
    lstrcatW(resource_module_path, filename);

    GetModuleFileNameW(NULL, current_module_path, ARRAY_SIZE(current_module_path));
    ret = CopyFileW(current_module_path, resource_module_path, FALSE);
    ok(ret, "CopyFileW failed, error %lu.\n", GetLastError());
    SetFileAttributesW(resource_module_path, FILE_ATTRIBUTE_NORMAL);

    resource = BeginUpdateResourceW(resource_module_path, TRUE);
    UpdateResourceW(resource, (LPCWSTR)RT_RCDATA, filename, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (void *)data, size);
    EndUpdateResourceW(resource, FALSE);

    module = LoadLibraryExW(resource_module_path, NULL, LOAD_LIBRARY_AS_DATAFILE);

    return module;
}

static void delete_resource_module(const WCHAR *filename, HMODULE module)
{
    WCHAR path[MAX_PATH];

    FreeLibrary(module);

    lstrcpyW(path, temp_dir);
    lstrcatW(path, filename);
    DeleteFileW(path);
}

static void check_image_info(D3DX10_IMAGE_INFO *image_info, const struct test_image *image, unsigned int line)
{
    ok_(__FILE__, line)(image_info->Width == image->expected_info.Width,
            "Got unexpected Width %u, expected %u.\n",
            image_info->Width, image->expected_info.Width);
    ok_(__FILE__, line)(image_info->Height == image->expected_info.Height,
            "Got unexpected Height %u, expected %u.\n",
            image_info->Height, image->expected_info.Height);
    ok_(__FILE__, line)(image_info->Depth == image->expected_info.Depth,
            "Got unexpected Depth %u, expected %u.\n",
            image_info->Depth, image->expected_info.Depth);
    ok_(__FILE__, line)(image_info->ArraySize == image->expected_info.ArraySize,
            "Got unexpected ArraySize %u, expected %u.\n",
            image_info->ArraySize, image->expected_info.ArraySize);
    ok_(__FILE__, line)(image_info->MipLevels == image->expected_info.MipLevels,
            "Got unexpected MipLevels %u, expected %u.\n",
            image_info->MipLevels, image->expected_info.MipLevels);
    ok_(__FILE__, line)(image_info->MiscFlags == image->expected_info.MiscFlags,
            "Got unexpected MiscFlags %#x, expected %#x.\n",
            image_info->MiscFlags, image->expected_info.MiscFlags);
    ok_(__FILE__, line)(image_info->Format == image->expected_info.Format,
            "Got unexpected Format %#x, expected %#x.\n",
            image_info->Format, image->expected_info.Format);
    ok_(__FILE__, line)(image_info->ResourceDimension == image->expected_info.ResourceDimension,
            "Got unexpected ResourceDimension %u, expected %u.\n",
            image_info->ResourceDimension, image->expected_info.ResourceDimension);
    ok_(__FILE__, line)(image_info->ImageFileFormat == image->expected_info.ImageFileFormat,
            "Got unexpected ImageFileFormat %u, expected %u.\n",
            image_info->ImageFileFormat, image->expected_info.ImageFileFormat);
}

static ID3D10Texture2D *get_texture2d_readback(ID3D10Texture2D *texture)
{
    D3D10_TEXTURE2D_DESC desc;
    ID3D10Texture2D *readback;
    ID3D10Device *device;
    HRESULT hr;

    ID3D10Texture2D_GetDevice(texture, &device);

    ID3D10Texture2D_GetDesc(texture, &desc);
    desc.Usage = D3D10_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;

    hr = ID3D10Device_CreateTexture2D(device, &desc, NULL, &readback);
    if (hr != S_OK)
    {
        ID3D10Device_Release(device);
        return NULL;
    }
    ID3D10Device_CopyResource(device, (ID3D10Resource *)readback, (ID3D10Resource *)texture);

    ID3D10Device_Release(device);
    return readback;
}

static ID3D10Texture3D *get_texture3d_readback(ID3D10Texture3D *texture)
{
    D3D10_TEXTURE3D_DESC desc;
    ID3D10Texture3D *readback;
    ID3D10Device *device;
    HRESULT hr;

    ID3D10Texture3D_GetDevice(texture, &device);

    ID3D10Texture3D_GetDesc(texture, &desc);
    desc.Usage = D3D10_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;

    hr = ID3D10Device_CreateTexture3D(device, &desc, NULL, &readback);
    if (hr != S_OK)
    {
        ID3D10Device_Release(device);
        return NULL;
    }
    ID3D10Device_CopyResource(device, (ID3D10Resource *)readback, (ID3D10Resource *)texture);

    ID3D10Device_Release(device);
    return readback;
}

static void check_resource_info(ID3D10Resource *resource, const struct test_image *image, unsigned int line)
{
    unsigned int expected_mip_levels, expected_width, expected_height, max_dimension;
    D3D10_RESOURCE_DIMENSION resource_dimension;
    D3D10_TEXTURE2D_DESC desc_2d;
    D3D10_TEXTURE3D_DESC desc_3d;
    ID3D10Texture2D *texture_2d;
    ID3D10Texture3D *texture_3d;
    HRESULT hr;

    expected_width = image->expected_info.Width;
    expected_height = image->expected_info.Height;
    if (is_block_compressed(image->expected_info.Format))
    {
        expected_width = (expected_width + 3) & ~3;
        expected_height = (expected_height + 3) & ~3;
    }
    expected_mip_levels = 0;
    max_dimension = max(expected_width, expected_height);
    while (max_dimension)
    {
        ++expected_mip_levels;
        max_dimension >>= 1;
    }

    ID3D10Resource_GetType(resource, &resource_dimension);
    todo_wine_if (image->expected_info.ResourceDimension == D3D10_RESOURCE_DIMENSION_TEXTURE3D)
        ok(resource_dimension == image->expected_info.ResourceDimension,
                "Got unexpected ResourceDimension %u, expected %u.\n",
                 resource_dimension, image->expected_info.ResourceDimension);

    switch (resource_dimension)
    {
        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
            hr = ID3D10Resource_QueryInterface(resource, &IID_ID3D10Texture2D, (void **)&texture_2d);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n",  hr);
            ID3D10Texture2D_GetDesc(texture_2d, &desc_2d);
            ok_(__FILE__, line)(desc_2d.Width == expected_width,
                    "Got unexpected Width %u, expected %u.\n",
                     desc_2d.Width, expected_width);
            ok_(__FILE__, line)(desc_2d.Height == expected_height,
                    "Got unexpected Height %u, expected %u.\n",
                     desc_2d.Height, expected_height);
            todo_wine_if(expected_mip_levels != image->expected_info.MipLevels)
            ok_(__FILE__, line)(desc_2d.MipLevels == expected_mip_levels,
                    "Got unexpected MipLevels %u, expected %u.\n",
                     desc_2d.MipLevels, expected_mip_levels);
            ok_(__FILE__, line)(desc_2d.ArraySize == image->expected_info.ArraySize,
                    "Got unexpected ArraySize %u, expected %u.\n",
                     desc_2d.ArraySize, image->expected_info.ArraySize);
            ok_(__FILE__, line)(desc_2d.Format == image->expected_info.Format,
                    "Got unexpected Format %u, expected %u.\n",
                     desc_2d.Format, image->expected_info.Format);
            ok_(__FILE__, line)(desc_2d.SampleDesc.Count == 1,
                    "Got unexpected SampleDesc.Count %u, expected %u\n",
                     desc_2d.SampleDesc.Count, 1);
            ok_(__FILE__, line)(desc_2d.SampleDesc.Quality == 0,
                    "Got unexpected SampleDesc.Quality %u, expected %u\n",
                     desc_2d.SampleDesc.Quality, 0);
            ok_(__FILE__, line)(desc_2d.Usage == D3D10_USAGE_DEFAULT,
                    "Got unexpected Usage %u, expected %u\n",
                     desc_2d.Usage, D3D10_USAGE_DEFAULT);
            ok_(__FILE__, line)(desc_2d.BindFlags == D3D10_BIND_SHADER_RESOURCE,
                    "Got unexpected BindFlags %#x, expected %#x\n",
                     desc_2d.BindFlags, D3D10_BIND_SHADER_RESOURCE);
            ok_(__FILE__, line)(desc_2d.CPUAccessFlags == 0,
                    "Got unexpected CPUAccessFlags %#x, expected %#x\n",
                     desc_2d.CPUAccessFlags, 0);
            ok_(__FILE__, line)(desc_2d.MiscFlags == image->expected_info.MiscFlags,
                    "Got unexpected MiscFlags %#x, expected %#x.\n",
                     desc_2d.MiscFlags, image->expected_info.MiscFlags);

            ID3D10Texture2D_Release(texture_2d);
            break;

        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
            hr = ID3D10Resource_QueryInterface(resource, &IID_ID3D10Texture3D, (void **)&texture_3d);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n",  hr);
            ID3D10Texture3D_GetDesc(texture_3d, &desc_3d);
            ok_(__FILE__, line)(desc_3d.Width == expected_width,
                    "Got unexpected Width %u, expected %u.\n",
                     desc_3d.Width, expected_width);
            ok_(__FILE__, line)(desc_3d.Height == expected_height,
                    "Got unexpected Height %u, expected %u.\n",
                     desc_3d.Height, expected_height);
            ok_(__FILE__, line)(desc_3d.Depth == image->expected_info.Depth,
                    "Got unexpected Depth %u, expected %u.\n",
                     desc_3d.Depth, image->expected_info.Depth);
            ok_(__FILE__, line)(desc_3d.MipLevels == expected_mip_levels,
                    "Got unexpected MipLevels %u, expected %u.\n",
                     desc_3d.MipLevels, expected_mip_levels);
            ok_(__FILE__, line)(desc_3d.Format == image->expected_info.Format,
                    "Got unexpected Format %u, expected %u.\n",
                     desc_3d.Format, image->expected_info.Format);
            ok_(__FILE__, line)(desc_3d.Usage == D3D10_USAGE_DEFAULT,
                    "Got unexpected Usage %u, expected %u\n",
                     desc_3d.Usage, D3D10_USAGE_DEFAULT);
            ok_(__FILE__, line)(desc_3d.BindFlags == D3D10_BIND_SHADER_RESOURCE,
                    "Got unexpected BindFlags %#x, expected %#x\n",
                     desc_3d.BindFlags, D3D10_BIND_SHADER_RESOURCE);
            ok_(__FILE__, line)(desc_3d.CPUAccessFlags == 0,
                    "Got unexpected CPUAccessFlags %#x, expected %#x\n",
                     desc_3d.CPUAccessFlags, 0);
            ok_(__FILE__, line)(desc_3d.MiscFlags == image->expected_info.MiscFlags,
                    "Got unexpected MiscFlags %#x, expected %#x.\n",
                     desc_3d.MiscFlags, image->expected_info.MiscFlags);
            ID3D10Texture3D_Release(texture_3d);
            break;

        default:
            break;
    }
}

static void check_texture2d_data(ID3D10Texture2D *texture, const struct test_image *image, unsigned int line)
{
    unsigned int width, height, stride, i, array_slice;
    D3D10_MAPPED_TEXTURE2D map;
    D3D10_TEXTURE2D_DESC desc;
    ID3D10Texture2D *readback;
    const BYTE *expected_data;
    BOOL line_match;
    HRESULT hr;

    readback = get_texture2d_readback(texture);
    ok_(__FILE__, line)(readback != NULL, "Failed to get texture readback.\n");
    if (!readback)
        return;

    ID3D10Texture2D_GetDesc(readback, &desc);
    width = desc.Width;
    height = desc.Height;
    stride = (width * get_bpp_from_format(desc.Format) + 7) / 8;
    if (is_block_compressed(desc.Format))
    {
        stride *= 4;
        height /= 4;
    }

    expected_data = image->expected_data;
    for (array_slice = 0; array_slice < desc.ArraySize; ++array_slice)
    {
        hr = ID3D10Texture2D_Map(readback, array_slice * desc.MipLevels, D3D10_MAP_READ, 0, &map);
        ok_(__FILE__, line)(hr == S_OK, "Map failed, hr %#lx.\n", hr);
        if (hr != S_OK)
        {
            ID3D10Texture2D_Release(readback);
            return;
        }

        for (i = 0; i < height; ++i)
        {
            line_match = !memcmp(expected_data + stride * i,
                    (BYTE *)map.pData + map.RowPitch * i, stride);
            todo_wine_if(is_block_compressed(image->expected_info.Format)
                    && (image->expected_info.Width % 4 != 0 || image->expected_info.Height % 4 != 0))
                ok_(__FILE__, line)(line_match, "Data mismatch for line %u, array slice %u.\n", i, array_slice);
            if (!line_match)
                break;
        }
        expected_data += stride * height;

        ID3D10Texture2D_Unmap(readback, 0);
    }

    ID3D10Texture2D_Release(readback);
}

static void check_texture3d_data(ID3D10Texture3D *texture, const struct test_image *image, unsigned int line)
{
    unsigned int width, height, depth, stride, i;
    D3D10_MAPPED_TEXTURE3D map;
    D3D10_TEXTURE3D_DESC desc;
    ID3D10Texture3D *readback;
    const BYTE *expected_data;
    BOOL line_match;
    HRESULT hr;

    readback = get_texture3d_readback(texture);
    ok_(__FILE__, line)(readback != NULL, "Failed to get texture readback.\n");
    if (!readback)
        return;

    ID3D10Texture3D_GetDesc(readback, &desc);
    width = desc.Width;
    height = desc.Height;
    depth = desc.Depth;
    stride = (width * get_bpp_from_format(desc.Format) + 7) / 8;
    if (is_block_compressed(desc.Format))
    {
        stride *= 4;
        height /= 4;
    }

    expected_data = image->expected_data;
    hr = ID3D10Texture3D_Map(readback, 0, D3D10_MAP_READ, 0, &map);
    ok_(__FILE__, line)(hr == S_OK, "Map failed, hr %#lx.\n", hr);

    for (i = 0; i < height * depth; ++i)
    {
        line_match = !memcmp(expected_data + stride * i,
                (BYTE *)map.pData + map.RowPitch * i, stride);
        ok_(__FILE__, line)(line_match, "Data mismatch for line %u.\n", i);
        if (!line_match)
        {
            for (unsigned int j = 0; j < stride; ++j)
                trace("%02x\n", *((BYTE *)map.pData + map.RowPitch * i + j));
            break;
        }
    }

    ID3D10Texture3D_Unmap(readback, 0);
    ID3D10Texture3D_Release(readback);
}

static void check_resource_data(ID3D10Resource *resource, const struct test_image *image, unsigned int line)
{
    ID3D10Texture3D *texture3d;
    ID3D10Texture2D *texture2d;

    if (SUCCEEDED(ID3D10Resource_QueryInterface(resource, &IID_ID3D10Texture3D, (void **)&texture3d)))
    {
        check_texture3d_data(texture3d, image, line);
        ID3D10Texture3D_Release(texture3d);
    }
    else if (SUCCEEDED(ID3D10Resource_QueryInterface(resource, &IID_ID3D10Texture2D, (void **)&texture2d)))
    {
        check_texture2d_data(texture2d, image, line);
        ID3D10Texture2D_Release(texture2d);
    }
    else
    {
        ok(0, "Failed to get 2D or 3D texture interface.\n");
    }
}

static void test_D3DX10UnsetAllDeviceObjects(void)
{
    static const D3D10_INPUT_ELEMENT_DESC layout_desc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0},
    };
#if 0
float4 main(float4 pos : POSITION) : POSITION
{
    return pos;
}
#endif
    static const DWORD simple_vs[] =
    {
        0x43425844, 0x66689e7c, 0x643f0971, 0xb7f67ff4, 0xabc48688, 0x00000001, 0x000000d4, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0xababab00,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x49534f50, 0x4e4f4954, 0xababab00, 0x52444853, 0x00000038, 0x00010040,
        0x0000000e, 0x0300005f, 0x001010f2, 0x00000000, 0x03000065, 0x001020f2, 0x00000000, 0x05000036,
        0x001020f2, 0x00000000, 0x00101e46, 0x00000000, 0x0100003e,
    };

#if 0
struct gs_out
{
    float4 pos : SV_POSITION;
};

[maxvertexcount(4)]
void main(point float4 vin[1] : POSITION, inout TriangleStream<gs_out> vout)
{
    float offset = 0.1 * vin[0].w;
    gs_out v;

    v.pos = float4(vin[0].x - offset, vin[0].y - offset, vin[0].z, vin[0].w);
    vout.Append(v);
    v.pos = float4(vin[0].x - offset, vin[0].y + offset, vin[0].z, vin[0].w);
    vout.Append(v);
    v.pos = float4(vin[0].x + offset, vin[0].y - offset, vin[0].z, vin[0].w);
    vout.Append(v);
    v.pos = float4(vin[0].x + offset, vin[0].y + offset, vin[0].z, vin[0].w);
    vout.Append(v);
}
#endif
    static const DWORD simple_gs[] =
    {
        0x43425844, 0x000ee786, 0xc624c269, 0x885a5cbe, 0x444b3b1f, 0x00000001, 0x0000023c, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0xababab00,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x52444853, 0x000001a0, 0x00020040,
        0x00000068, 0x0400005f, 0x002010f2, 0x00000001, 0x00000000, 0x02000068, 0x00000001, 0x0100085d,
        0x0100285c, 0x04000067, 0x001020f2, 0x00000000, 0x00000001, 0x0200005e, 0x00000004, 0x0f000032,
        0x00100032, 0x00000000, 0x80201ff6, 0x00000041, 0x00000000, 0x00000000, 0x00004002, 0x3dcccccd,
        0x3dcccccd, 0x00000000, 0x00000000, 0x00201046, 0x00000000, 0x00000000, 0x05000036, 0x00102032,
        0x00000000, 0x00100046, 0x00000000, 0x06000036, 0x001020c2, 0x00000000, 0x00201ea6, 0x00000000,
        0x00000000, 0x01000013, 0x05000036, 0x00102012, 0x00000000, 0x0010000a, 0x00000000, 0x0e000032,
        0x00100052, 0x00000000, 0x00201ff6, 0x00000000, 0x00000000, 0x00004002, 0x3dcccccd, 0x00000000,
        0x3dcccccd, 0x00000000, 0x00201106, 0x00000000, 0x00000000, 0x05000036, 0x00102022, 0x00000000,
        0x0010002a, 0x00000000, 0x06000036, 0x001020c2, 0x00000000, 0x00201ea6, 0x00000000, 0x00000000,
        0x01000013, 0x05000036, 0x00102012, 0x00000000, 0x0010000a, 0x00000000, 0x05000036, 0x00102022,
        0x00000000, 0x0010001a, 0x00000000, 0x06000036, 0x001020c2, 0x00000000, 0x00201ea6, 0x00000000,
        0x00000000, 0x01000013, 0x05000036, 0x00102032, 0x00000000, 0x00100086, 0x00000000, 0x06000036,
        0x001020c2, 0x00000000, 0x00201ea6, 0x00000000, 0x00000000, 0x01000013, 0x0100003e,
    };

#if 0
float4 main(float4 color : COLOR) : SV_TARGET
{
    return color;
}
#endif
    static const DWORD simple_ps[] =
    {
        0x43425844, 0x08c2b568, 0x17d33120, 0xb7d82948, 0x13a570fb, 0x00000001, 0x000000d0, 0x00000003,
        0x0000002c, 0x0000005c, 0x00000090, 0x4e475349, 0x00000028, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x4f4c4f43, 0xabab0052, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000038, 0x00000040, 0x0000000e,
        0x03001062, 0x001010f2, 0x00000000, 0x03000065, 0x001020f2, 0x00000000, 0x05000036, 0x001020f2,
        0x00000000, 0x00101e46, 0x00000000, 0x0100003e,
    };

    D3D10_VIEWPORT tmp_viewport[D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    ID3D10ShaderResourceView *tmp_srv[D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    ID3D10ShaderResourceView *srv[D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    ID3D10RenderTargetView *tmp_rtv[D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT];
    RECT tmp_rect[D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    ID3D10SamplerState *tmp_sampler[D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT];
    ID3D10RenderTargetView *rtv[D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT];
    ID3D10Texture2D *rt_texture[D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT];
    ID3D10Buffer *cb[D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    ID3D10Buffer *tmp_buffer[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    ID3D10SamplerState *sampler[D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT];
    ID3D10Buffer *buffer[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    unsigned int offset[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    unsigned int stride[D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    ID3D10Buffer *so_buffer[D3D10_SO_BUFFER_SLOT_COUNT];
    ID3D10InputLayout *tmp_input_layout, *input_layout;
    ID3D10DepthStencilState *tmp_ds_state, *ds_state;
    ID3D10BlendState *tmp_blend_state, *blend_state;
    ID3D10RasterizerState *tmp_rs_state, *rs_state;
    ID3D10Predicate *tmp_predicate, *predicate;
    D3D10_SHADER_RESOURCE_VIEW_DESC srv_desc;
    ID3D10DepthStencilView *tmp_dsv, *dsv;
    D3D10_PRIMITIVE_TOPOLOGY topology;
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10GeometryShader *tmp_gs, *gs;
    D3D10_DEPTH_STENCIL_DESC ds_desc;
    ID3D10VertexShader *tmp_vs, *vs;
    D3D10_SAMPLER_DESC sampler_desc;
    D3D10_QUERY_DESC predicate_desc;
    ID3D10PixelShader *tmp_ps, *ps;
    D3D10_RASTERIZER_DESC rs_desc;
    D3D10_BUFFER_DESC buffer_desc;
    D3D10_BLEND_DESC blend_desc;
    ID3D10Texture2D *ds_texture;
    unsigned int sample_mask;
    unsigned int stencil_ref;
    unsigned int count, i;
    float blend_factor[4];
    ID3D10Device *device;
    BOOL predicate_value;
    DXGI_FORMAT format;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    buffer_desc.ByteWidth = 1024;
    buffer_desc.Usage = D3D10_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;

    for (i = 0; i < D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i)
    {
        hr = ID3D10Device_CreateBuffer(device, &buffer_desc, NULL, &cb[i]);
        ok(SUCCEEDED(hr), "Failed to create buffer, hr %#lx.\n", hr);
    }

    buffer_desc.BindFlags = D3D10_BIND_VERTEX_BUFFER | D3D10_BIND_INDEX_BUFFER | D3D10_BIND_SHADER_RESOURCE;

    for (i = 0; i < D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        hr = ID3D10Device_CreateBuffer(device, &buffer_desc, NULL, &buffer[i]);
        ok(SUCCEEDED(hr), "Failed to create buffer, hr %#lx.\n", hr);

        stride[i] = (i + 1) * 4;
        offset[i] = (i + 1) * 16;
    }

    buffer_desc.BindFlags = D3D10_BIND_STREAM_OUTPUT;

    for (i = 0; i < D3D10_SO_BUFFER_SLOT_COUNT; ++i)
    {
        hr = ID3D10Device_CreateBuffer(device, &buffer_desc, NULL, &so_buffer[i]);
        ok(SUCCEEDED(hr), "Failed to create buffer, hr %#lx.\n", hr);
    }

    srv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srv_desc.ViewDimension = D3D10_SRV_DIMENSION_BUFFER;
    srv_desc.Buffer.ElementOffset = 0;
    srv_desc.Buffer.ElementWidth = 64;

    for (i = 0; i < D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        hr = ID3D10Device_CreateShaderResourceView(device,
                (ID3D10Resource *)buffer[i % D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT], &srv_desc, &srv[i]);
        ok(SUCCEEDED(hr), "Failed to create shader resource view, hr %#lx.\n", hr);
    }

    sampler_desc.Filter = D3D10_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MipLODBias = 0.0f;
    sampler_desc.MaxAnisotropy = 16;
    sampler_desc.ComparisonFunc = D3D10_COMPARISON_NEVER;
    sampler_desc.BorderColor[0] = 0.0f;
    sampler_desc.BorderColor[1] = 0.0f;
    sampler_desc.BorderColor[2] = 0.0f;
    sampler_desc.BorderColor[3] = 0.0f;
    sampler_desc.MinLOD = 0.0f;
    sampler_desc.MaxLOD = 16.0f;

    for (i = 0; i < D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        sampler_desc.MinLOD = (float)i;

        hr = ID3D10Device_CreateSamplerState(device, &sampler_desc, &sampler[i]);
        ok(SUCCEEDED(hr), "Failed to create sampler state, hr %#lx.\n", hr);
    }

    hr = ID3D10Device_CreateVertexShader(device, simple_vs, sizeof(simple_vs), &vs);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#lx.\n", hr);

    hr = ID3D10Device_CreateGeometryShader(device, simple_gs, sizeof(simple_gs), &gs);
    ok(SUCCEEDED(hr), "Failed to create geometry shader, hr %#lx.\n", hr);

    hr = ID3D10Device_CreatePixelShader(device, simple_ps, sizeof(simple_ps), &ps);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#lx.\n", hr);

    hr = ID3D10Device_CreateInputLayout(device, layout_desc, ARRAY_SIZE(layout_desc), simple_vs,
            sizeof(simple_vs), &input_layout);
    ok(SUCCEEDED(hr), "Failed to create input layout, hr %#lx.\n", hr);

    blend_desc.AlphaToCoverageEnable = FALSE;
    blend_desc.BlendEnable[0] = FALSE;
    blend_desc.BlendEnable[1] = FALSE;
    blend_desc.BlendEnable[2] = FALSE;
    blend_desc.BlendEnable[3] = FALSE;
    blend_desc.BlendEnable[4] = FALSE;
    blend_desc.BlendEnable[5] = FALSE;
    blend_desc.BlendEnable[6] = FALSE;
    blend_desc.BlendEnable[7] = FALSE;
    blend_desc.SrcBlend = D3D10_BLEND_ONE;
    blend_desc.DestBlend = D3D10_BLEND_ZERO;
    blend_desc.BlendOp = D3D10_BLEND_OP_ADD;
    blend_desc.SrcBlendAlpha = D3D10_BLEND_ONE;
    blend_desc.DestBlendAlpha = D3D10_BLEND_ZERO;
    blend_desc.BlendOpAlpha = D3D10_BLEND_OP_ADD;
    blend_desc.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTargetWriteMask[1] = D3D10_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTargetWriteMask[2] = D3D10_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTargetWriteMask[3] = D3D10_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTargetWriteMask[4] = D3D10_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTargetWriteMask[5] = D3D10_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTargetWriteMask[6] = D3D10_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTargetWriteMask[7] = D3D10_COLOR_WRITE_ENABLE_ALL;

    hr = ID3D10Device_CreateBlendState(device, &blend_desc, &blend_state);
    ok(SUCCEEDED(hr), "Failed to create blend state, hr %#lx.\n", hr);

    ds_desc.DepthEnable = TRUE;
    ds_desc.DepthWriteMask = D3D10_DEPTH_WRITE_MASK_ALL;
    ds_desc.DepthFunc = D3D10_COMPARISON_LESS;
    ds_desc.StencilEnable = FALSE;
    ds_desc.StencilReadMask = D3D10_DEFAULT_STENCIL_READ_MASK;
    ds_desc.StencilWriteMask = D3D10_DEFAULT_STENCIL_WRITE_MASK;
    ds_desc.FrontFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.FrontFace.StencilDepthFailOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.FrontFace.StencilPassOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.FrontFace.StencilFunc = D3D10_COMPARISON_ALWAYS;
    ds_desc.BackFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.BackFace.StencilDepthFailOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.BackFace.StencilPassOp = D3D10_STENCIL_OP_KEEP;
    ds_desc.BackFace.StencilFunc = D3D10_COMPARISON_ALWAYS;

    hr = ID3D10Device_CreateDepthStencilState(device, &ds_desc, &ds_state);
    ok(SUCCEEDED(hr), "Failed to create depthstencil state, hr %#lx.\n", hr);

    texture_desc.Width = 512;
    texture_desc.Height = 512;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_RENDER_TARGET;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    for (i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &rt_texture[i]);
        ok(SUCCEEDED(hr), "Failed to create texture, hr %#lx.\n", hr);
    }

    texture_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    texture_desc.BindFlags = D3D10_BIND_DEPTH_STENCIL;

    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &ds_texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#lx.\n", hr);

    for (i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        hr = ID3D10Device_CreateRenderTargetView(device, (ID3D10Resource *)rt_texture[i], NULL, &rtv[i]);
        ok(SUCCEEDED(hr), "Failed to create rendertarget view, hr %#lx.\n", hr);
    }

    hr = ID3D10Device_CreateDepthStencilView(device, (ID3D10Resource *)ds_texture, NULL, &dsv);
    ok(SUCCEEDED(hr), "Failed to create depthstencil view, hr %#lx.\n", hr);

    for (i = 0; i < D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE; ++i)
    {
        SetRect(&tmp_rect[i], i, i * 2, i + 1, (i + 1) * 2);

        tmp_viewport[i].TopLeftX = i * 3;
        tmp_viewport[i].TopLeftY = i * 4;
        tmp_viewport[i].Width = 3;
        tmp_viewport[i].Height = 4;
        tmp_viewport[i].MinDepth = i * 0.01f;
        tmp_viewport[i].MaxDepth = (i + 1) * 0.01f;
    }

    rs_desc.FillMode = D3D10_FILL_SOLID;
    rs_desc.CullMode = D3D10_CULL_BACK;
    rs_desc.FrontCounterClockwise = FALSE;
    rs_desc.DepthBias = 0;
    rs_desc.DepthBiasClamp = 0.0f;
    rs_desc.SlopeScaledDepthBias = 0.0f;
    rs_desc.DepthClipEnable = TRUE;
    rs_desc.ScissorEnable = FALSE;
    rs_desc.MultisampleEnable = FALSE;
    rs_desc.AntialiasedLineEnable = FALSE;

    hr = ID3D10Device_CreateRasterizerState(device, &rs_desc, &rs_state);
    ok(SUCCEEDED(hr), "Failed to create rasterizer state, hr %#lx.\n", hr);

    predicate_desc.Query = D3D10_QUERY_OCCLUSION_PREDICATE;
    predicate_desc.MiscFlags = 0;

    hr = ID3D10Device_CreatePredicate(device, &predicate_desc, &predicate);
    ok(SUCCEEDED(hr), "Failed to create predicate, hr %#lx.\n", hr);

    ID3D10Device_VSSetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, cb);
    ID3D10Device_VSSetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srv);
    ID3D10Device_VSSetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, sampler);
    ID3D10Device_VSSetShader(device, vs);

    ID3D10Device_GSSetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, cb);
    ID3D10Device_GSSetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srv);
    ID3D10Device_GSSetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, sampler);
    ID3D10Device_GSSetShader(device, gs);

    ID3D10Device_PSSetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, cb);
    ID3D10Device_PSSetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, srv);
    ID3D10Device_PSSetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, sampler);
    ID3D10Device_PSSetShader(device, ps);

    ID3D10Device_IASetVertexBuffers(device, 0, D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, buffer, stride, offset);
    ID3D10Device_IASetIndexBuffer(device, buffer[0], DXGI_FORMAT_R32_UINT, offset[0]);
    ID3D10Device_IASetInputLayout(device, input_layout);
    ID3D10Device_IASetPrimitiveTopology(device, D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    blend_factor[0] = 0.1f;
    blend_factor[1] = 0.2f;
    blend_factor[2] = 0.3f;
    blend_factor[3] = 0.4f;
    ID3D10Device_OMSetBlendState(device, blend_state, blend_factor, 0xff00ff00);
    ID3D10Device_OMSetDepthStencilState(device, ds_state, 3);
    ID3D10Device_OMSetRenderTargets(device, D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT, rtv, dsv);

    ID3D10Device_RSSetScissorRects(device, D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, tmp_rect);
    ID3D10Device_RSSetViewports(device, D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, tmp_viewport);
    ID3D10Device_RSSetState(device, rs_state);

    ID3D10Device_SOSetTargets(device, D3D10_SO_BUFFER_SLOT_COUNT, so_buffer, offset);

    ID3D10Device_SetPredication(device, predicate, TRUE);

    hr = D3DX10UnsetAllDeviceObjects(device);
    ok(SUCCEEDED(hr), "D3DX10UnsetAllDeviceObjects() failed, %#lx.\n", hr);

    ID3D10Device_VSGetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, tmp_buffer);
    for (i = 0; i < D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i)
    {
        ok(!tmp_buffer[i], "Got unexpected constant buffer %p in slot %u.\n", tmp_buffer[i], i);
    }
    ID3D10Device_VSGetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, tmp_srv);
    for (i = 0; i < D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ok(!tmp_srv[i], "Got unexpected shader resource view %p in slot %u.\n", tmp_srv[i], i);
    }
    ID3D10Device_VSGetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, tmp_sampler);
    for (i = 0; i < D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        ok(!tmp_sampler[i], "Got unexpected sampler %p in slot %u.\n", tmp_sampler[i], i);
    }
    ID3D10Device_VSGetShader(device, &tmp_vs);
    ok(!tmp_vs, "Got unexpected vertex shader %p.\n", tmp_vs);

    ID3D10Device_GSGetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, tmp_buffer);
    for (i = 0; i < D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i)
    {
        ok(!tmp_buffer[i], "Got unexpected constant buffer %p in slot %u.\n", tmp_buffer[i], i);
    }
    ID3D10Device_GSGetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, tmp_srv);
    for (i = 0; i < D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ok(!tmp_srv[i], "Got unexpected shader resource view %p in slot %u.\n", tmp_srv[i], i);
    }
    ID3D10Device_GSGetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, tmp_sampler);
    for (i = 0; i < D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        ok(!tmp_sampler[i], "Got unexpected sampler %p in slot %u.\n", tmp_sampler[i], i);
    }
    ID3D10Device_GSGetShader(device, &tmp_gs);
    ok(!tmp_gs, "Got unexpected geometry shader %p.\n", tmp_gs);

    ID3D10Device_PSGetConstantBuffers(device, 0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, tmp_buffer);
    for (i = 0; i < D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i)
    {
        ok(!tmp_buffer[i], "Got unexpected constant buffer %p in slot %u.\n", tmp_buffer[i], i);
    }
    ID3D10Device_PSGetShaderResources(device, 0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, tmp_srv);
    for (i = 0; i < D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ok(!tmp_srv[i], "Got unexpected shader resource view %p in slot %u.\n", tmp_srv[i], i);
    }
    ID3D10Device_PSGetSamplers(device, 0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT, tmp_sampler);
    for (i = 0; i < D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        ok(!tmp_sampler[i], "Got unexpected sampler %p in slot %u.\n", tmp_sampler[i], i);
    }
    ID3D10Device_PSGetShader(device, &tmp_ps);
    ok(!tmp_ps, "Got unexpected pixel shader %p.\n", tmp_ps);

    ID3D10Device_IAGetVertexBuffers(device, 0, D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, tmp_buffer, stride, offset);
    for (i = 0; i < D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ok(!tmp_buffer[i], "Got unexpected vertex buffer %p in slot %u.\n", tmp_buffer[i], i);
        ok(!stride[i], "Got unexpected stride %u in slot %u.\n", stride[i], i);
        ok(!offset[i], "Got unexpected offset %u in slot %u.\n", offset[i], i);
    }
    ID3D10Device_IAGetIndexBuffer(device, tmp_buffer, &format, offset);
    ok(!tmp_buffer[0], "Got unexpected index buffer %p.\n", tmp_buffer[0]);
    ok(format == DXGI_FORMAT_R32_UINT, "Got unexpected index buffer format %#x.\n", format);
    ok(!offset[0], "Got unexpected index buffer offset %u.\n", offset[0]);
    ID3D10Device_IAGetInputLayout(device, &tmp_input_layout);
    ok(!tmp_input_layout, "Got unexpected input layout %p.\n", tmp_input_layout);
    ID3D10Device_IAGetPrimitiveTopology(device, &topology);
    ok(topology == D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, "Got unexpected primitive topology %#x.\n", topology);

    ID3D10Device_OMGetBlendState(device, &tmp_blend_state, blend_factor, &sample_mask);
    ok(!tmp_blend_state, "Got unexpected blend state %p.\n", tmp_blend_state);
    ok(blend_factor[0] == 0.0f && blend_factor[1] == 0.0f
            && blend_factor[2] == 0.0f && blend_factor[3] == 0.0f,
            "Got unexpected blend factor {%.8e, %.8e, %.8e, %.8e}.\n",
            blend_factor[0], blend_factor[1], blend_factor[2], blend_factor[3]);
    ok(sample_mask == 0, "Got unexpected sample mask %#x.\n", sample_mask);
    ID3D10Device_OMGetDepthStencilState(device, &tmp_ds_state, &stencil_ref);
    ok(!tmp_ds_state, "Got unexpected depth stencil state %p.\n", tmp_ds_state);
    ok(!stencil_ref, "Got unexpected stencil ref %u.\n", stencil_ref);
    ID3D10Device_OMGetRenderTargets(device, D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT, tmp_rtv, &tmp_dsv);
    for (i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        ok(!tmp_rtv[i], "Got unexpected render target view %p in slot %u.\n", tmp_rtv[i], i);
    }
    ok(!tmp_dsv, "Got unexpected depth stencil view %p.\n", tmp_dsv);

    ID3D10Device_RSGetScissorRects(device, &count, NULL);
    ok(count == D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE,
            "Got unexpected scissor rect count %u.\n", count);
    memset(tmp_rect, 0x55, sizeof(tmp_rect));
    count = D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    ID3D10Device_RSGetScissorRects(device, &count, tmp_rect);
    for (i = 0; i < D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE; ++i)
    {
        ok(tmp_rect[i].left == i
                && tmp_rect[i].top == i * 2
                && tmp_rect[i].right == i + 1
                && tmp_rect[i].bottom == (i + 1) * 2,
                "Got unexpected scissor rect %s in slot %u.\n",
                wine_dbgstr_rect(&tmp_rect[i]), i);
    }
    ID3D10Device_RSGetViewports(device, &count, NULL);
    ok(count == D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE,
            "Got unexpected viewport count %u.\n", count);
    memset(tmp_viewport, 0x55, sizeof(tmp_viewport));
    count = D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    ID3D10Device_RSGetViewports(device, &count, tmp_viewport);
    for (i = 0; i < D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE; ++i)
    {
        ok(tmp_viewport[i].TopLeftX == i * 3
                    && tmp_viewport[i].TopLeftY == i * 4
                    && tmp_viewport[i].Width == 3
                    && tmp_viewport[i].Height == 4
                    && compare_float(tmp_viewport[i].MinDepth, i * 0.01f, 16)
                    && compare_float(tmp_viewport[i].MaxDepth, (i + 1) * 0.01f, 16),
                    "Got unexpected viewport {%d, %d, %u, %u, %.8e, %.8e} in slot %u.\n",
                    tmp_viewport[i].TopLeftX, tmp_viewport[i].TopLeftY, tmp_viewport[i].Width,
                    tmp_viewport[i].Height, tmp_viewport[i].MinDepth, tmp_viewport[i].MaxDepth, i);
    }
    ID3D10Device_RSGetState(device, &tmp_rs_state);
    ok(!tmp_rs_state, "Got unexpected rasterizer state %p.\n", tmp_rs_state);

    ID3D10Device_SOGetTargets(device, D3D10_SO_BUFFER_SLOT_COUNT, tmp_buffer, offset);
    for (i = 0; i < D3D10_SO_BUFFER_SLOT_COUNT; ++i)
    {
        ok(!tmp_buffer[i], "Got unexpected stream output %p in slot %u.\n", tmp_buffer[i], i);
        ok(offset[i] == ~0u, "Got unexpected stream output offset %u in slot %u.\n", offset[i], i);
    }

    ID3D10Device_GetPredication(device, &tmp_predicate, &predicate_value);
    ok(!tmp_predicate, "Got unexpected predicate %p.\n", tmp_predicate);
    ok(!predicate_value, "Got unexpected predicate value %#x.\n", predicate_value);

    ID3D10Predicate_Release(predicate);
    ID3D10RasterizerState_Release(rs_state);
    ID3D10DepthStencilView_Release(dsv);
    ID3D10Texture2D_Release(ds_texture);

    for (i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        ID3D10RenderTargetView_Release(rtv[i]);
        ID3D10Texture2D_Release(rt_texture[i]);
    }

    ID3D10DepthStencilState_Release(ds_state);
    ID3D10BlendState_Release(blend_state);
    ID3D10InputLayout_Release(input_layout);
    ID3D10VertexShader_Release(vs);
    ID3D10GeometryShader_Release(gs);
    ID3D10PixelShader_Release(ps);

    for (i = 0; i < D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        ID3D10SamplerState_Release(sampler[i]);
    }

    for (i = 0; i < D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ID3D10ShaderResourceView_Release(srv[i]);
    }

    for (i = 0; i < D3D10_SO_BUFFER_SLOT_COUNT; ++i)
    {
        ID3D10Buffer_Release(so_buffer[i]);
    }

    for (i = 0; i < D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        ID3D10Buffer_Release(buffer[i]);
    }

    for (i = 0; i < D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i)
    {
        ID3D10Buffer_Release(cb[i]);
    }

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

static void test_D3DX10CreateAsyncMemoryLoader(void)
{
    ID3DX10DataLoader *loader;
    SIZE_T size;
    DWORD data;
    HRESULT hr;
    void *ptr;

    hr = D3DX10CreateAsyncMemoryLoader(NULL, 0, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX10CreateAsyncMemoryLoader(NULL, 0, &loader);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX10CreateAsyncMemoryLoader(&data, 0, &loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    size = 100;
    hr = ID3DX10DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(ptr == &data, "Got data pointer %p, original %p.\n", ptr, &data);
    ok(!size, "Got unexpected data size.\n");

    /* Load() is no-op. */
    hr = ID3DX10DataLoader_Load(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DX10DataLoader_Destroy(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    data = 0;
    hr = D3DX10CreateAsyncMemoryLoader(&data, sizeof(data), &loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Load() is no-op. */
    hr = ID3DX10DataLoader_Load(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DX10DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(ptr == &data, "Got data pointer %p, original %p.\n", ptr, &data);
    ok(size == sizeof(data), "Got unexpected data size.\n");

    hr = ID3DX10DataLoader_Destroy(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
}

static void test_D3DX10CreateAsyncFileLoader(void)
{
    static const WCHAR test_filename[] = L"asyncloader.data";
    static const char test_data1[] = "test data";
    static const char test_data2[] = "more test data";
    ID3DX10DataLoader *loader;
    WCHAR path[MAX_PATH];
    SIZE_T size;
    HRESULT hr;
    void *ptr;
    BOOL ret;

    hr = D3DX10CreateAsyncFileLoaderA(NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX10CreateAsyncFileLoaderA(NULL, &loader);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX10CreateAsyncFileLoaderA("nonexistentfilename", &loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DX10DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DX10DataLoader_Load(loader);
    ok(hr == D3D10_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DX10DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DX10DataLoader_Destroy(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Test file sharing using dummy empty file. */
    create_file(test_filename, test_data1, sizeof(test_data1), path);

    hr = D3DX10CreateAsyncFileLoaderW(path, &loader);
    ok(SUCCEEDED(hr), "Failed to create file loader, hr %#lx.\n", hr);

    ret = delete_file(test_filename);
    ok(ret, "DeleteFile() failed, ret %d, error %ld.\n", ret, GetLastError());

    /* File was removed before Load(). */
    hr = ID3DX10DataLoader_Load(loader);
    ok(hr == D3D10_ERROR_FILE_NOT_FOUND, "Load() returned unexpected result, hr %#lx.\n", hr);

    /* Create it again. */
    create_file(test_filename, test_data1, sizeof(test_data1), NULL);
    hr = ID3DX10DataLoader_Load(loader);
    ok(SUCCEEDED(hr), "Load() failed, hr %#lx.\n", hr);

    /* Already loaded. */
    hr = ID3DX10DataLoader_Load(loader);
    ok(SUCCEEDED(hr), "Load() failed, hr %#lx.\n", hr);

    ret = delete_file(test_filename);
    ok(ret, "DeleteFile() failed, ret %d, error %ld.\n", ret, GetLastError());

    /* Already loaded, file removed. */
    hr = ID3DX10DataLoader_Load(loader);
    ok(hr == D3D10_ERROR_FILE_NOT_FOUND, "Load() returned unexpected result, hr %#lx.\n", hr);

    /* Decompress still works. */
    ptr = NULL;
    hr = ID3DX10DataLoader_Decompress(loader, &ptr, &size);
    ok(SUCCEEDED(hr), "Decompress() failed, hr %#lx.\n", hr);
    ok(ptr != NULL, "Got unexpected ptr %p.\n", ptr);
    ok(size == sizeof(test_data1), "Got unexpected decompressed size.\n");
    if (size == sizeof(test_data1))
        ok(!memcmp(ptr, test_data1, size), "Got unexpected file data.\n");

    /* Create it again, with different data. */
    create_file(test_filename, test_data2, sizeof(test_data2), NULL);

    hr = ID3DX10DataLoader_Load(loader);
    ok(SUCCEEDED(hr), "Load() failed, hr %#lx.\n", hr);

    ptr = NULL;
    hr = ID3DX10DataLoader_Decompress(loader, &ptr, &size);
    ok(SUCCEEDED(hr), "Decompress() failed, hr %#lx.\n", hr);
    ok(ptr != NULL, "Got unexpected ptr %p.\n", ptr);
    ok(size == sizeof(test_data2), "Got unexpected decompressed size.\n");
    if (size == sizeof(test_data2))
        ok(!memcmp(ptr, test_data2, size), "Got unexpected file data.\n");

    hr = ID3DX10DataLoader_Destroy(loader);
    ok(SUCCEEDED(hr), "Destroy() failed, hr %#lx.\n", hr);

    ret = delete_file(test_filename);
    ok(ret, "DeleteFile() failed, ret %d, error %ld.\n", ret, GetLastError());
}

static void test_D3DX10CreateAsyncResourceLoader(void)
{
    ID3DX10DataLoader *loader;
    HRESULT hr;

    hr = D3DX10CreateAsyncResourceLoaderA(NULL, NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX10CreateAsyncResourceLoaderA(NULL, NULL, &loader);
    ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX10CreateAsyncResourceLoaderA(NULL, "noname", &loader);
    ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX10CreateAsyncResourceLoaderW(NULL, NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX10CreateAsyncResourceLoaderW(NULL, NULL, &loader);
    ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX10CreateAsyncResourceLoaderW(NULL, L"noname", &loader);
    ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
}

static void test_D3DX10CreateAsyncTextureInfoProcessor(void)
{
    ID3DX10DataProcessor *dp;
    D3DX10_IMAGE_INFO info;
    HRESULT hr;
    int i;

    CoInitialize(NULL);

    hr = D3DX10CreateAsyncTextureInfoProcessor(NULL, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX10CreateAsyncTextureInfoProcessor(&info, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX10CreateAsyncTextureInfoProcessor(NULL, &dp);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    if (0)
    {
        /* Crashes on native. */
        hr = ID3DX10DataProcessor_Process(dp, (void *)test_image[0].data, test_image[0].size);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    }

    hr = ID3DX10DataProcessor_Destroy(dp);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX10CreateAsyncTextureInfoProcessor(&info, &dp);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DX10DataProcessor_Process(dp, (void *)test_image[0].data, 0);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX10DataProcessor_Process(dp, NULL, test_image[0].size);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);

        hr = ID3DX10DataProcessor_Process(dp, (void *)test_image[i].data, test_image[i].size);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX10_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
            check_image_info(&info, test_image + i, __LINE__);

        winetest_pop_context();
    }

    hr = ID3DX10DataProcessor_CreateDeviceObject(dp, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DX10DataProcessor_Destroy(dp);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    CoUninitialize();
}

static void test_D3DX10CreateAsyncTextureProcessor(void)
{
    ID3DX10DataProcessor *dp;
    ID3D10Resource *resource;
    ID3D10Device *device;
    HRESULT hr;
    int i;

    device = create_device();
    if (!device)
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    CoInitialize(NULL);

    hr = D3DX10CreateAsyncTextureProcessor(device, NULL, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX10CreateAsyncTextureProcessor(NULL, NULL, &dp);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX10CreateAsyncTextureProcessor(device, NULL, &dp);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX10DataProcessor_Process(dp, (void *)test_image[0].data, 0);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX10DataProcessor_Process(dp, NULL, test_image[0].size);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX10DataProcessor_Destroy(dp);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);

        hr = D3DX10CreateAsyncTextureProcessor(device, NULL, &dp);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = ID3DX10DataProcessor_Process(dp, (void *)test_image[i].data, test_image[i].size);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX10_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
        {
            hr = ID3DX10DataProcessor_CreateDeviceObject(dp, (void **)&resource);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            check_resource_info(resource, test_image + i, __LINE__);
            check_resource_data(resource, test_image + i, __LINE__);
            ID3D10Resource_Release(resource);
        }

        hr = ID3DX10DataProcessor_Destroy(dp);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        winetest_pop_context();
    }

    CoUninitialize();

    ok(!ID3D10Device_Release(device), "Unexpected refcount.\n");
}

static DWORD main_tid;
static DWORD io_tid;

struct data_object
{
    ID3DX10DataLoader ID3DX10DataLoader_iface;
    ID3DX10DataProcessor ID3DX10DataProcessor_iface;

    HANDLE load_started;
    HANDLE load_done;
    HANDLE decompress_done;
    HRESULT load_ret;

    DWORD process_tid;
};

static struct data_object *data_object_from_ID3DX10DataLoader(ID3DX10DataLoader *iface)
{
    return CONTAINING_RECORD(iface, struct data_object, ID3DX10DataLoader_iface);
}

static LONG data_loader_load_count;
static WINAPI HRESULT data_loader_Load(ID3DX10DataLoader *iface)
{
    struct data_object *data_object = data_object_from_ID3DX10DataLoader(iface);
    DWORD ret;

    ok(InterlockedDecrement(&data_loader_load_count) >= 0, "Got unexpected call.\n");

    if (!io_tid)
        io_tid = GetCurrentThreadId();
    ok(io_tid != main_tid, "Load called in main thread.\n");
    ok(io_tid == GetCurrentThreadId(), "Load called in wrong thread.\n");

    SetEvent(data_object->load_started);
    ret = WaitForSingleObject(data_object->load_done, INFINITE);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx.\n", ret);
    return data_object->load_ret;
}

static LONG data_loader_decompress_count;
static WINAPI HRESULT data_loader_Decompress(ID3DX10DataLoader *iface, void **data, SIZE_T *bytes)
{
    struct data_object *data_object = data_object_from_ID3DX10DataLoader(iface);
    DWORD ret;

    ok(InterlockedDecrement(&data_loader_decompress_count) >= 0, "Got unexpected call.\n");
    ok(!!data, "Got unexpected data %p.\n", data);
    ok(!!bytes, "Got unexpected bytes %p.\n", bytes);

    data_object->process_tid = GetCurrentThreadId();
    ok(data_object->process_tid != main_tid, "Decompress called in main thread.\n");
    ok(data_object->process_tid != io_tid, "Decompress called in IO thread.\n");

    *data = (void *)0xdeadbeef;
    *bytes = 0xdead;
    ret = WaitForSingleObject(data_object->decompress_done, INFINITE);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx.\n", ret);
    return S_OK;
}

static LONG data_loader_destroy_count;
static WINAPI HRESULT data_loader_Destroy(ID3DX10DataLoader *iface)
{
    ok(InterlockedDecrement(&data_loader_destroy_count) >= 0, "Got unexpected call.\n");
    return S_OK;
}

static ID3DX10DataLoaderVtbl D3DX10DataLoaderVtbl =
{
    data_loader_Load,
    data_loader_Decompress,
    data_loader_Destroy
};

static struct data_object* data_object_from_ID3DX10DataProcessor(ID3DX10DataProcessor *iface)
{
    return CONTAINING_RECORD(iface, struct data_object, ID3DX10DataProcessor_iface);
}

static LONG data_processor_process_count;
static HRESULT WINAPI data_processor_Process(ID3DX10DataProcessor *iface, void *data, SIZE_T bytes)
{
    struct data_object *data_object = data_object_from_ID3DX10DataProcessor(iface);

    ok(InterlockedDecrement(&data_processor_process_count) >= 0, "Got unexpected call.\n");
    ok(data_object->process_tid == GetCurrentThreadId(), "Process called in unexpected thread.\n");

    ok(data == (void *)0xdeadbeef, "Got unexpected data %p.\n", data);
    ok(bytes == 0xdead, "Got unexpected bytes %Iu.\n", bytes);
    return S_OK;
}

static LONG data_processor_create_count;
static HRESULT WINAPI data_processor_CreateDeviceObject(ID3DX10DataProcessor *iface, void **object)
{
    ok(InterlockedDecrement(&data_processor_create_count) >= 0, "Got unexpected call.\n");
    ok(main_tid == GetCurrentThreadId(), "CreateDeviceObject not called in main thread.\n");

    *object = (void *)0xdeadf00d;
    return S_OK;
}

static LONG data_processor_destroy_count;
static HRESULT WINAPI data_processor_Destroy(ID3DX10DataProcessor *iface)
{
    struct data_object *data_object = data_object_from_ID3DX10DataProcessor(iface);

    ok(InterlockedDecrement(&data_processor_destroy_count) >= 0, "Got unexpected call.\n");

    CloseHandle(data_object->load_started);
    CloseHandle(data_object->load_done);
    CloseHandle(data_object->decompress_done);
    free(data_object);
    return S_OK;
}

static ID3DX10DataProcessorVtbl D3DX10DataProcessorVtbl =
{
    data_processor_Process,
    data_processor_CreateDeviceObject,
    data_processor_Destroy
};

static struct data_object *create_data_object(HRESULT load_ret)
{
    struct data_object *data_object = malloc(sizeof(*data_object));

    data_object->ID3DX10DataLoader_iface.lpVtbl = &D3DX10DataLoaderVtbl;
    data_object->ID3DX10DataProcessor_iface.lpVtbl = &D3DX10DataProcessorVtbl;

    data_object->load_started = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!data_object->load_started, "CreateEvent failed, error %lu.\n", GetLastError());
    data_object->load_done = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!data_object->load_done, "CreateEvent failed, error %lu.\n", GetLastError());
    data_object->decompress_done = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!data_object->decompress_done, "CreateEvent failed, error %lu.\n", GetLastError());
    data_object->load_ret = load_ret;

    return data_object;
}

static void test_D3DX10CreateThreadPump(void)
{
    UINT io_count, process_count, device_count, count;
    struct data_object *data_object[2];
    ID3DX10DataProcessor *processor;
    D3DX10_IMAGE_INFO image_info;
    ID3DX10DataLoader *loader;
    HRESULT hr, work_item_hr;
    ID3D10Resource *resource;
    ID3DX10ThreadPump *pump;
    ID3D10Device *device;
    SYSTEM_INFO info;
    void *object;
    DWORD ret;
    int i;

    main_tid = GetCurrentThreadId();

    hr = D3DX10CreateThreadPump(1024, 0, &pump);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    hr = D3DX10CreateThreadPump(0, 1024, &pump);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    GetSystemInfo(&info);
    if (info.dwNumberOfProcessors > 1)
        hr = D3DX10CreateThreadPump(0, 0, &pump);
    else
        hr = D3DX10CreateThreadPump(0, 2, &pump);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    count = ID3DX10ThreadPump_GetWorkItemCount(pump);
    ok(!count, "GetWorkItemCount returned %u.\n", count);
    hr = ID3DX10ThreadPump_GetQueueStatus(pump, &io_count, &process_count, &device_count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!io_count, "Got unexpected io_count %u.\n", io_count);
    ok(!process_count, "Got unexpected process_count %u.\n", process_count);
    ok(!device_count, "Got unexpected device_count %u.\n", device_count);

    data_object[0] = create_data_object(E_NOTIMPL);
    data_object[1] = create_data_object(S_OK);

    data_loader_load_count = 1;
    hr = ID3DX10ThreadPump_AddWorkItem(pump, &data_object[0]->ID3DX10DataLoader_iface,
            &data_object[0]->ID3DX10DataProcessor_iface, &work_item_hr, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ret = WaitForSingleObject(data_object[0]->load_started, INFINITE);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx.\n", ret);
    ok(!data_loader_load_count, "Got unexpected data_loader_load_count %ld.\n",
            data_loader_load_count);
    count = ID3DX10ThreadPump_GetWorkItemCount(pump);
    ok(count == 1, "GetWorkItemCount returned %u.\n", count);
    hr = ID3DX10ThreadPump_GetQueueStatus(pump, &io_count, &process_count, &device_count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!io_count, "Got unexpected io_count %u.\n", io_count);
    ok(!process_count, "Got unexpected process_count %u.\n", process_count);
    ok(!device_count, "Got unexpected device_count %u.\n", device_count);

    hr = ID3DX10ThreadPump_AddWorkItem(pump, &data_object[1]->ID3DX10DataLoader_iface,
            &data_object[1]->ID3DX10DataProcessor_iface, NULL, &object);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ret = WaitForSingleObject(data_object[0]->load_started, 50);
    ok(ret == WAIT_TIMEOUT, "WaitForSingleObject returned %#lx.\n", ret);
    count = ID3DX10ThreadPump_GetWorkItemCount(pump);
    ok(count == 2, "GetWorkItemCount returned %u.\n", count);
    hr = ID3DX10ThreadPump_GetQueueStatus(pump, &io_count, &process_count, &device_count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(io_count == 1, "Got unexpected io_count %u.\n", io_count);
    ok(!process_count, "Got unexpected process_count %u.\n", process_count);
    ok(!device_count, "Got unexpected device_count %u.\n", device_count);

    data_loader_load_count = 1;
    data_loader_destroy_count = 1;
    data_processor_destroy_count = 1;
    SetEvent(data_object[0]->load_done);
    ret = WaitForSingleObject(data_object[1]->load_started, INFINITE);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx.\n", ret);
    ok(work_item_hr == E_NOTIMPL, "Got unexpected work_item_hr %#lx.\n", work_item_hr);
    ok(!data_loader_destroy_count, "Got unexpected data_loader_destroy_count %ld.\n",
            data_loader_destroy_count);
    ok(!data_processor_destroy_count, "Got unexpected data_processor_destroy_count %ld.\n",
            data_processor_destroy_count);
    ok(!data_loader_load_count, "Got unexpected data_loader_load_count %ld.\n",
            data_loader_load_count);

    data_loader_decompress_count = 1;
    data_processor_process_count = 1;
    SetEvent(data_object[1]->load_done);
    SetEvent(data_object[1]->decompress_done);

    data_processor_create_count = 1;
    data_loader_destroy_count = 1;
    data_processor_destroy_count = 1;
    hr = ID3DX10ThreadPump_WaitForAllItems(pump);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(object == (void *)0xdeadf00d, "Got unexpected object %p.\n", object);
    ok(!data_loader_decompress_count, "Got unexpected data_loader_decompress_count %ld.\n",
            data_loader_decompress_count);
    ok(!data_processor_process_count, "Got unexpected data_processor_process_count %ld.\n",
            data_processor_process_count);
    ok(!data_processor_create_count, "Got unexpected data_processor_create_count %ld.\n",
            data_processor_create_count);
    ok(!data_loader_destroy_count, "Got unexpected data_loader_destroy_count %ld.\n",
            data_loader_destroy_count);
    ok(!data_processor_destroy_count, "Got unexpected data_processor_destroy_count %ld.\n",
            data_processor_destroy_count);

    data_object[0] = create_data_object(S_OK);
    data_object[1] = create_data_object(S_OK);
    SetEvent(data_object[0]->load_done);
    SetEvent(data_object[1]->load_done);
    SetEvent(data_object[1]->decompress_done);

    data_loader_load_count = 2;
    data_loader_decompress_count = 2;
    data_processor_process_count = 1;
    hr = ID3DX10ThreadPump_AddWorkItem(pump, &data_object[0]->ID3DX10DataLoader_iface,
            &data_object[0]->ID3DX10DataProcessor_iface, NULL, &object);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX10ThreadPump_AddWorkItem(pump, &data_object[1]->ID3DX10DataLoader_iface,
            &data_object[1]->ID3DX10DataProcessor_iface, NULL, &object);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    for (;;)
    {
        hr = ID3DX10ThreadPump_GetQueueStatus(pump, &io_count, &process_count, &device_count);
        if (hr != S_OK || device_count)
            break;
        Sleep(1);
    }
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!io_count, "Got unexpected io_count %u.\n", io_count);
    ok(!process_count, "Got unexpected process_count %u.\n", process_count);
    ok(device_count == 1, "Got unexpected device_count %u.\n", device_count);
    ok(!data_loader_load_count, "Got unexpected data_loader_load_count %ld.\n",
            data_loader_load_count);
    ok(!data_loader_decompress_count, "Got unexpected data_loader_decompress_count %ld.\n",
            data_loader_decompress_count);
    ok(!data_processor_process_count, "Got unexpected data_processor_process_count %ld.\n",
            data_processor_process_count);

    hr = ID3DX10ThreadPump_ProcessDeviceWorkItems(pump, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX10ThreadPump_GetQueueStatus(pump, &io_count, &process_count, &device_count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!io_count, "Got unexpected io_count %u.\n", io_count);
    ok(!process_count, "Got unexpected process_count %u.\n", process_count);
    ok(device_count == 1, "Got unexpected device_count %u.\n", device_count);

    data_processor_create_count = 1;
    data_loader_destroy_count = 1;
    data_processor_destroy_count = 1;
    hr = ID3DX10ThreadPump_ProcessDeviceWorkItems(pump, 1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX10ThreadPump_GetQueueStatus(pump, &io_count, &process_count, &device_count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!io_count, "Got unexpected io_count %u.\n", io_count);
    ok(!process_count, "Got unexpected process_count %u.\n", process_count);
    ok(!device_count, "Got unexpected device_count %u.\n", device_count);
    ok(!data_processor_create_count, "Got unexpected data_processor_create_count %ld.\n",
            data_processor_create_count);
    ok(!data_loader_destroy_count, "Got unexpected data_loader_destroy_count %ld.\n",
            data_loader_destroy_count);
    ok(!data_processor_destroy_count, "Got unexpected data_processor_destroy_count %ld.\n",
            data_processor_destroy_count);

    data_processor_process_count = 1;
    data_loader_destroy_count = 1;
    data_processor_destroy_count = 1;
    SetEvent(data_object[0]->decompress_done);
    hr = ID3DX10ThreadPump_PurgeAllItems(pump);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    data_processor_process_count = 0;
    ok(!data_loader_destroy_count, "Got unexpected data_loader_destroy_count %ld.\n",
            data_loader_destroy_count);
    ok(!data_processor_destroy_count, "Got unexpected data_processor_destroy_count %ld.\n",
            data_processor_destroy_count);

    device = create_device();
    if (!device)
    {
        skip("Failed to create device, skipping tests.\n");
        ID3DX10ThreadPump_Release(pump);
        return;
    }

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);

        hr = D3DX10CreateAsyncMemoryLoader(test_image[i].data, test_image[i].size, &loader);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = D3DX10CreateAsyncTextureInfoProcessor(&image_info, &processor);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = ID3DX10ThreadPump_AddWorkItem(pump, loader, processor, &work_item_hr, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = ID3DX10ThreadPump_WaitForAllItems(pump);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(work_item_hr == S_OK || (work_item_hr == E_FAIL
                    && test_image[i].expected_info.ImageFileFormat == D3DX10_IFF_WMP),
                "Got unexpected hr %#lx.\n", work_item_hr);
        if (work_item_hr == S_OK)
            check_image_info(&image_info, test_image + i, __LINE__);

        hr = D3DX10CreateAsyncMemoryLoader(test_image[i].data, test_image[i].size, &loader);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = D3DX10CreateAsyncTextureProcessor(device, NULL, &processor);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = ID3DX10ThreadPump_AddWorkItem(pump, loader, processor, &work_item_hr, (void **)&resource);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = ID3DX10ThreadPump_WaitForAllItems(pump);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(work_item_hr == S_OK || (work_item_hr == E_FAIL
                    && test_image[i].expected_info.ImageFileFormat == D3DX10_IFF_WMP),
                "Got unexpected hr %#lx.\n", work_item_hr);
        if (work_item_hr == S_OK)
        {
            check_resource_info(resource, test_image + i, __LINE__);
            check_resource_data(resource, test_image + i, __LINE__);
            ID3D10Resource_Release(resource);
        }

        winetest_pop_context();
    }

    ok(!ID3D10Device_Release(device), "Got unexpected refcount.\n");

    ret = ID3DX10ThreadPump_Release(pump);
    ok(!ret, "Got unexpected refcount %lu.\n", ret);
}

#define check_dds_pixel_format(flags, fourcc, bpp, rmask, gmask, bmask, amask, format) \
        check_dds_pixel_format_(__LINE__, flags, fourcc, bpp, rmask, gmask, bmask, amask, format)
static void check_dds_pixel_format_(unsigned int line, DWORD flags, DWORD fourcc, DWORD bpp,
        DWORD rmask, DWORD gmask, DWORD bmask, DWORD amask, DXGI_FORMAT expected_format)
{
    D3DX10_IMAGE_INFO info;
    HRESULT hr;
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

    hr = D3DX10GetImageInfoFromMemory(&dds, sizeof(dds), NULL, &info, NULL);
    ok_(__FILE__, line)(hr == S_OK, "Got unexpected hr %#lx for pixel format %#x.\n", hr, expected_format);
    if (SUCCEEDED(hr))
    {
        ok_(__FILE__, line)(info.Format == expected_format, "Unexpected format %#x, expected %#x\n",
                info.Format, expected_format);
    }
}

static void test_get_image_info(void)
{
    static const WCHAR test_resource_name[] = L"resource.data";
    static const WCHAR test_filename[] = L"image.data";
    char buffer[sizeof(test_bmp_1bpp) * 2];
    D3DX10_IMAGE_INFO image_info;
    HMODULE resource_module;
    WCHAR path[MAX_PATH];
    unsigned int i;
    DWORD dword;
    HRESULT hr, hr2;

    CoInitialize(NULL);

    hr2 = 0xdeadbeef;
    hr = D3DX10GetImageInfoFromMemory(test_image[0].data, 0, NULL, &image_info, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX10GetImageInfoFromMemory(NULL, test_image[0].size, NULL, &image_info, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    dword = 0xdeadbeef;
    hr = D3DX10GetImageInfoFromMemory(&dword, sizeof(dword), NULL, &image_info, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);

    /* NULL hr2 is valid. */
    hr = D3DX10GetImageInfoFromMemory(test_image[0].data, test_image[0].size, NULL, &image_info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Test a too-large size. */
    memset(buffer, 0xcc, sizeof(buffer));
    memcpy(buffer, test_bmp_1bpp, sizeof(test_bmp_1bpp));
    hr = D3DX10GetImageInfoFromMemory(buffer, sizeof(buffer), NULL, &image_info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info(&image_info, &test_image[0], __LINE__);

    /* Test a too-small size. */
    hr2 = 0xdeadbeef;
    hr = D3DX10GetImageInfoFromMemory(test_bmp_1bpp, sizeof(test_bmp_1bpp) - 1, NULL, &image_info, &hr2);
    todo_wine ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);

    /* 2 bpp is not a valid bit count. */
    hr2 = 0xdeadbeef;
    hr = D3DX10GetImageInfoFromMemory(test_bmp_2bpp, sizeof(test_bmp_2bpp), NULL, &image_info, &hr2);
    todo_wine ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);

        hr2 = 0xdeadbeef;
        hr = D3DX10GetImageInfoFromMemory(test_image[i].data, test_image[i].size, NULL, &image_info, &hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX10_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        if (hr == S_OK)
            check_image_info(&image_info, test_image + i, __LINE__);

        winetest_pop_context();
    }

    hr2 = 0xdeadbeef;
    add_work_item_count = 0;
    hr = D3DX10GetImageInfoFromMemory(test_image[0].data, test_image[0].size, &thread_pump, &image_info, &hr2);
    ok(add_work_item_count == 1, "Got unexpected add_work_item_count %u.\n", add_work_item_count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    check_image_info(&image_info, test_image, __LINE__);

    hr2 = 0xdeadbeef;
    hr = D3DX10GetImageInfoFromFileW(NULL, NULL, &image_info, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX10GetImageInfoFromFileW(L"deadbeaf", NULL, &image_info, &hr2);
    ok(hr == D3D10_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX10GetImageInfoFromFileA(NULL, NULL, &image_info, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX10GetImageInfoFromFileA("deadbeaf", NULL, &image_info, &hr2);
    ok(hr == D3D10_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);
        create_file(test_filename, test_image[i].data, test_image[i].size, path);

        hr2 = 0xdeadbeef;
        hr = D3DX10GetImageInfoFromFileW(path, NULL, &image_info, &hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX10_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        if (hr == S_OK)
            check_image_info(&image_info, test_image + i, __LINE__);

        hr2 = 0xdeadbeef;
        hr = D3DX10GetImageInfoFromFileA(get_str_a(path), NULL, &image_info, &hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX10_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        if (hr == S_OK)
            check_image_info(&image_info, test_image + i, __LINE__);

        delete_file(test_filename);
        winetest_pop_context();
    }

    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('D','X','T','1'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC1_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('D','X','T','2'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC2_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('D','X','T','3'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC2_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('D','X','T','4'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC3_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('D','X','T','5'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC3_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('R','G','B','G'), 0, 0, 0, 0, 0, DXGI_FORMAT_R8G8_B8G8_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('G','R','G','B'), 0, 0, 0, 0, 0, DXGI_FORMAT_G8R8_G8B8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB, 0, 16, 0xf800, 0x07e0, 0x001f, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 16, 0x7c00, 0x03e0, 0x001f, 0x8000, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 16, 0x0f00, 0x00f0, 0x000f, 0xf000, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB, 0, 8, 0xe0, 0x1c, 0x03, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_ALPHA_ONLY, 0, 8, 0, 0, 0, 0xff, DXGI_FORMAT_A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 16, 0x00e0, 0x001c, 0x0003, 0xff00, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB, 0, 16, 0xf00, 0x0f0, 0x00f, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000, DXGI_FORMAT_R10G10B10A2_UNORM);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000, DXGI_FORMAT_R10G10B10A2_UNORM);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB, 0, 32, 0xff0000, 0x00ff00, 0x0000ff, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB, 0, 32, 0x0000ff, 0x00ff00, 0xff0000, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB, 0, 32, 0x0000ffff, 0xffff0000, 0, 0, DXGI_FORMAT_R16G16_UNORM);
    check_dds_pixel_format(DDS_PF_LUMINANCE, 0, 8, 0xff, 0, 0, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_LUMINANCE, 0, 16, 0xffff, 0, 0, 0, DXGI_FORMAT_R16G16B16A16_UNORM);
    check_dds_pixel_format(DDS_PF_LUMINANCE | DDS_PF_ALPHA, 0, 16, 0x00ff, 0, 0, 0xff00, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_LUMINANCE | DDS_PF_ALPHA, 0, 8, 0x0f, 0, 0, 0xf0, DXGI_FORMAT_R8G8B8A8_UNORM);

    /* D3DX10GetImageInfoFromResource tests */

    hr2 = 0xdeadbeef;
    hr = D3DX10GetImageInfoFromResourceW(NULL, NULL, NULL, &image_info, &hr2);
    ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX10GetImageInfoFromResourceW(NULL, L"deadbeaf", NULL, &image_info, &hr2);
    ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX10GetImageInfoFromResourceA(NULL, NULL, NULL, &image_info, &hr2);
    ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX10GetImageInfoFromResourceA(NULL, "deadbeaf", NULL, &image_info, &hr2);
    ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);
        resource_module = create_resource_module(test_resource_name, test_image[i].data, test_image[i].size);

        hr2 = 0xdeadbeef;
        hr = D3DX10GetImageInfoFromResourceW(resource_module, L"deadbeef", NULL, &image_info, &hr2);
        ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
        ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);

        hr2 = 0xdeadbeef;
        hr = D3DX10GetImageInfoFromResourceW(resource_module, test_resource_name, NULL, &image_info, &hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX10_IFF_WMP)
                || broken(hr == D3DX10_ERR_INVALID_DATA) /* Vista */,
                "Got unexpected hr %#lx.\n", hr);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        if (hr == S_OK)
            check_image_info(&image_info, test_image + i, __LINE__);

        hr2 = 0xdeadbeef;
        hr = D3DX10GetImageInfoFromResourceA(resource_module, get_str_a(test_resource_name), NULL, &image_info, &hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX10_IFF_WMP)
                || broken(hr == D3DX10_ERR_INVALID_DATA) /* Vista */,
                "Got unexpected hr %#lx.\n", hr);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        if (hr == S_OK)
            check_image_info(&image_info, test_image + i, __LINE__);

        delete_resource_module(test_resource_name, resource_module);
        winetest_pop_context();
    }

    CoUninitialize();
}

static void test_create_texture(void)
{
    static const WCHAR test_resource_name[] = L"resource.data";
    static const WCHAR test_filename[] = L"image.data";
    ID3D10Resource *resource;
    HMODULE resource_module;
    ID3D10Device *device;
    WCHAR path[MAX_PATH];
    HRESULT hr, hr2;
    unsigned int i;

    device = create_device();
    if (!device)
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    CoInitialize(NULL);

    /* D3DX10CreateTextureFromMemory tests */

    resource = (ID3D10Resource *)0xdeadbeef;
    hr2 = 0xdeadbeef;
    hr = D3DX10CreateTextureFromMemory(NULL, test_bmp_1bpp, sizeof(test_bmp_1bpp), NULL, NULL, &resource, &hr2);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    ok(resource == (ID3D10Resource *)0xdeadbeef, "Got unexpected resource %p.\n", resource);

    resource = (ID3D10Resource *)0xdeadbeef;
    hr2 = 0xdeadbeef;
    hr = D3DX10CreateTextureFromMemory(device, NULL, 0, NULL, NULL, &resource, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    ok(resource == (ID3D10Resource *)0xdeadbeef, "Got unexpected resource %p.\n", resource);

    resource = (ID3D10Resource *)0xdeadbeef;
    hr2 = 0xdeadbeef;
    hr = D3DX10CreateTextureFromMemory(device, NULL, sizeof(test_bmp_1bpp), NULL, NULL, &resource, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    ok(resource == (ID3D10Resource *)0xdeadbeef, "Got unexpected resource %p.\n", resource);

    resource = (ID3D10Resource *)0xdeadbeef;
    hr2 = 0xdeadbeef;
    hr = D3DX10CreateTextureFromMemory(device, test_bmp_1bpp, 0, NULL, NULL, &resource, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    ok(resource == (ID3D10Resource *)0xdeadbeef, "Got unexpected resource %p.\n", resource);

    resource = (ID3D10Resource *)0xdeadbeef;
    hr2 = 0xdeadbeef;
    hr = D3DX10CreateTextureFromMemory(device, test_bmp_1bpp, sizeof(test_bmp_1bpp) - 1, NULL, NULL, &resource, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    ok(resource == (ID3D10Resource *)0xdeadbeef, "Got unexpected resource %p.\n", resource);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);

        hr2 = 0xdeadbeef;
        hr = D3DX10CreateTextureFromMemory(device, test_image[i].data, test_image[i].size, NULL, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX10_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
        {
            check_resource_info(resource, test_image + i, __LINE__);
            check_resource_data(resource, test_image + i, __LINE__);
            ID3D10Resource_Release(resource);
        }

        winetest_pop_context();
    }

    hr2 = 0xdeadbeef;
    add_work_item_count = 0;
    hr = D3DX10CreateTextureFromMemory(device, test_image[0].data, test_image[0].size,
            NULL, &thread_pump, &resource, &hr2);
    ok(add_work_item_count == 1, "Got unexpected add_work_item_count %u.\n", add_work_item_count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    check_resource_info(resource, test_image, __LINE__);
    check_resource_data(resource, test_image, __LINE__);
    ID3D10Resource_Release(resource);

    /* D3DX10CreateTextureFromFile tests */

    hr2 = 0xdeadbeef;
    hr = D3DX10CreateTextureFromFileW(device, NULL, NULL, NULL, &resource, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX10CreateTextureFromFileW(device, L"deadbeef", NULL, NULL, &resource, &hr2);
    ok(hr == D3D10_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX10CreateTextureFromFileA(device, NULL, NULL, NULL, &resource, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX10CreateTextureFromFileA(device, "deadbeef", NULL, NULL, &resource, &hr2);
    ok(hr == D3D10_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);
        create_file(test_filename, test_image[i].data, test_image[i].size, path);

        hr2 = 0xdeadbeef;
        hr = D3DX10CreateTextureFromFileW(device, path, NULL, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX10_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
        {
            check_resource_info(resource, test_image + i, __LINE__);
            check_resource_data(resource, test_image + i, __LINE__);
            ID3D10Resource_Release(resource);
        }

        hr2 = 0xdeadbeef;
        hr = D3DX10CreateTextureFromFileA(device, get_str_a(path), NULL, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX10_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
        {
            check_resource_info(resource, test_image + i, __LINE__);
            check_resource_data(resource, test_image + i, __LINE__);
            ID3D10Resource_Release(resource);
        }

        delete_file(test_filename);
        winetest_pop_context();
    }

    /* D3DX10CreateTextureFromResource tests */

    hr2 = 0xdeadbeef;
    hr = D3DX10CreateTextureFromResourceW(device, NULL, NULL, NULL, NULL, &resource, &hr2);
    ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX10CreateTextureFromResourceW(device, NULL, L"deadbeef", NULL, NULL, &resource, &hr2);
    ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX10CreateTextureFromResourceA(device, NULL, NULL, NULL, NULL, &resource, &hr2);
    ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX10CreateTextureFromResourceA(device, NULL, "deadbeef", NULL, NULL, &resource, &hr2);
    ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);
        resource_module = create_resource_module(test_resource_name, test_image[i].data, test_image[i].size);

        hr2 = 0xdeadbeef;
        hr = D3DX10CreateTextureFromResourceW(device, resource_module, L"deadbeef", NULL, NULL, &resource, &hr2);
        ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
        ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);

        hr2 = 0xdeadbeef;
        hr = D3DX10CreateTextureFromResourceW(device, resource_module,
                test_resource_name, NULL, NULL, &resource, &hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX10_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        if (hr == S_OK)
        {
            check_resource_info(resource, test_image + i, __LINE__);
            check_resource_data(resource, test_image + i, __LINE__);
            ID3D10Resource_Release(resource);
        }

        hr2 = 0xdeadbeef;
        hr = D3DX10CreateTextureFromResourceA(device, resource_module,
                get_str_a(test_resource_name), NULL, NULL, &resource, &hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX10_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        if (hr == S_OK)
        {
            check_resource_info(resource, test_image + i, __LINE__);
            check_resource_data(resource, test_image + i, __LINE__);
            ID3D10Resource_Release(resource);
        }

        delete_resource_module(test_resource_name, resource_module);
        winetest_pop_context();
    }

    CoUninitialize();

    ok(!ID3D10Device_Release(device), "Unexpected refcount.\n");
}

#define check_rect(rect, left, top, right, bottom) _check_rect(__LINE__, rect, left, top, right, bottom)
static inline void _check_rect(unsigned int line, const RECT *rect, int left, int top, int right, int bottom)
{
    ok_(__FILE__, line)(rect->left == left, "Unexpected rect.left %ld\n", rect->left);
    ok_(__FILE__, line)(rect->top == top, "Unexpected rect.top %ld\n", rect->top);
    ok_(__FILE__, line)(rect->right == right, "Unexpected rect.right %ld\n", rect->right);
    ok_(__FILE__, line)(rect->bottom == bottom, "Unexpected rect.bottom %ld\n", rect->bottom);
}

static void test_font(void)
{
    static const WCHAR testW[] = L"test";
    static const char long_text[] = "Example text to test clipping and other related things";
    static const WCHAR long_textW[] = L"Example text to test clipping and other related things";
    static const MAT2 mat = { {0,1}, {0,0}, {0,0}, {0,1} };
    static const D3DXCOLOR color = { 1.0f, 0.0f, 1.0f, 0.0f };
    static const D3DXCOLOR white = { 1.0f, 1.0f, 1.0f, 0.0f };
    static const struct
    {
        int font_height;
        unsigned int expected_size;
        unsigned int expected_levels;
    }
    tests[] =
    {
        {   2,  32,  2 },
        {   6, 128,  4 },
        {  10, 256,  5 },
        {  12, 256,  5 },
        {  72, 256,  8 },
        { 250, 256,  9 },
        { 258, 512, 10 },
        { 512, 512, 10 },
    };
    const unsigned int size = ARRAY_SIZE(testW);
    TEXTMETRICA metrics, expmetrics;
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10Device *device, *device2;
    ID3D10ShaderResourceView *srv;
    GLYPHMETRICS glyph_metrics;
    int ref, i, height, count;
    ID3D10Texture2D *texture;
    ID3D10Resource *resource;
    D3DX10_FONT_DESCA desc;
    ID3DX10Sprite *sprite;
    RECT rect, blackbox;
    ID3DX10Font *font;
    TEXTMETRICW tm;
    POINT cellinc;
    HRESULT hr;
    WORD glyph;
    BOOL ret;
    HDC hdc;
    char c;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    ref = get_refcount(device);
    hr = D3DX10CreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma", &font);
    ok(hr == S_OK, "Failed to create a font, hr %#lx.\n", hr);
    ok(ref < get_refcount(device), "Unexpected device refcount.\n");
    ID3DX10Font_Release(font);
    ok(ref == get_refcount(device), "Unexpected device refcount.\n");

    /* Zero size */
    hr = D3DX10CreateFontA(device, 0, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma", &font);
    ok(hr == S_OK, "Failed to create a font, hr %#lx.\n", hr);
    ID3DX10Font_Release(font);

    /* Unspecified font name */
    hr = D3DX10CreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, NULL, &font);
    ok(hr == S_OK, "Failed to create a font, hr %#lx.\n", hr);
    ID3DX10Font_Release(font);

    /* Empty font name */
    hr = D3DX10CreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "", &font);
    ok(hr == S_OK, "Failed to create a font, hr %#lx.\n", hr);
    ID3DX10Font_Release(font);

    hr = D3DX10CreateFontA(NULL, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma", &font);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    hr = D3DX10CreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma", NULL);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    hr = D3DX10CreateFontA(NULL, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma", NULL);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    /* D3DX10CreateFontIndirect */
    desc.Height = 12;
    desc.Width = 0;
    desc.Weight = FW_DONTCARE;
    desc.MipLevels = 0;
    desc.Italic = FALSE;
    desc.CharSet = DEFAULT_CHARSET;
    desc.OutputPrecision = OUT_DEFAULT_PRECIS;
    desc.Quality = DEFAULT_QUALITY;
    desc.PitchAndFamily = DEFAULT_PITCH;
    strcpy(desc.FaceName, "Tahoma");
    hr = D3DX10CreateFontIndirectA(device, &desc, &font);
    ok(hr == S_OK, "Failed to create a font, hr %#lx.\n", hr);
    ID3DX10Font_Release(font);

    hr = D3DX10CreateFontIndirectA(NULL, &desc, &font);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    hr = D3DX10CreateFontIndirectA(device, NULL, &font);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    hr = D3DX10CreateFontIndirectA(device, &desc, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    /* GetDevice */
    hr = D3DX10CreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma", &font);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ID3DX10Font_GetDevice(font, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    hr = ID3DX10Font_GetDevice(font, &device2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ID3D10Device_Release(device2);

    ID3DX10Font_Release(font);

    /* GetDesc */
    hr = D3DX10CreateFontA(device, 12, 8, FW_BOLD, 2, TRUE, ANSI_CHARSET, OUT_RASTER_PRECIS,
            ANTIALIASED_QUALITY, VARIABLE_PITCH, "Tahoma", &font);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ID3DX10Font_GetDescA(font, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    hr = ID3DX10Font_GetDescA(font, &desc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(desc.Height == 12, "Unexpected height %d.\n", desc.Height);
    ok(desc.Width == 8, "Unexpected width %u.\n", desc.Width);
    ok(desc.Weight == FW_BOLD, "Unexpected weight %u.\n", desc.Weight);
    ok(desc.MipLevels == 2, "Unexpected miplevels %u.\n", desc.MipLevels);
    ok(desc.Italic == TRUE, "Unexpected italic %#x.\n", desc.Italic);
    ok(desc.CharSet == ANSI_CHARSET, "Unexpected charset %u.\n", desc.CharSet);
    ok(desc.OutputPrecision == OUT_RASTER_PRECIS, "Unexpected output precision %u.\n", desc.OutputPrecision);
    ok(desc.Quality == ANTIALIASED_QUALITY, "Unexpected quality %u.\n", desc.Quality);
    ok(desc.PitchAndFamily == VARIABLE_PITCH, "Unexpected pitch and family %#x.\n", desc.PitchAndFamily);
    ok(!strcmp(desc.FaceName, "Tahoma"), "Unexpected facename %s.\n", debugstr_a(desc.FaceName));

    ID3DX10Font_Release(font);

    /* GetDC + GetTextMetrics */
    hr = D3DX10CreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma", &font);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hdc = ID3DX10Font_GetDC(font);
    ok(!!hdc, "Unexpected hdc %p.\n", hdc);

    ret = GetTextMetricsA(hdc, &expmetrics);
    ok(ret, "Unexpected ret %#x.\n", ret);

    ret = ID3DX10Font_GetTextMetricsA(font, &metrics);
    ok(ret, "Unexpected ret %#x.\n", ret);

    ok(metrics.tmHeight == expmetrics.tmHeight, "Unexpected height %ld, expected %ld.\n",
            metrics.tmHeight, expmetrics.tmHeight);
    ok(metrics.tmAscent == expmetrics.tmAscent, "Unexpected ascent %ld, expected %ld.\n",
            metrics.tmAscent, expmetrics.tmAscent);
    ok(metrics.tmDescent == expmetrics.tmDescent, "Unexpected descent %ld, expected %ld.\n",
            metrics.tmDescent, expmetrics.tmDescent);
    ok(metrics.tmInternalLeading == expmetrics.tmInternalLeading, "Unexpected internal leading %ld, expected %ld.\n",
            metrics.tmInternalLeading, expmetrics.tmInternalLeading);
    ok(metrics.tmExternalLeading == expmetrics.tmExternalLeading, "Unexpected external leading %ld, expected %ld.\n",
            metrics.tmExternalLeading, expmetrics.tmExternalLeading);
    ok(metrics.tmAveCharWidth == expmetrics.tmAveCharWidth, "Unexpected average char width %ld, expected %ld.\n",
            metrics.tmAveCharWidth, expmetrics.tmAveCharWidth);
    ok(metrics.tmMaxCharWidth == expmetrics.tmMaxCharWidth, "Unexpected maximum char width %ld, expected %ld.\n",
            metrics.tmMaxCharWidth, expmetrics.tmMaxCharWidth);
    ok(metrics.tmWeight == expmetrics.tmWeight, "Unexpected weight %ld, expected %ld.\n",
            metrics.tmWeight, expmetrics.tmWeight);
    ok(metrics.tmOverhang == expmetrics.tmOverhang, "Unexpected overhang %ld, expected %ld.\n",
            metrics.tmOverhang, expmetrics.tmOverhang);
    ok(metrics.tmDigitizedAspectX == expmetrics.tmDigitizedAspectX, "Unexpected digitized x aspect %ld, expected %ld.\n",
            metrics.tmDigitizedAspectX, expmetrics.tmDigitizedAspectX);
    ok(metrics.tmDigitizedAspectY == expmetrics.tmDigitizedAspectY, "Unexpected digitized y aspect %ld, expected %ld.\n",
            metrics.tmDigitizedAspectY, expmetrics.tmDigitizedAspectY);
    ok(metrics.tmFirstChar == expmetrics.tmFirstChar, "Unexpected first char %u, expected %u.\n",
            metrics.tmFirstChar, expmetrics.tmFirstChar);
    ok(metrics.tmLastChar == expmetrics.tmLastChar, "Unexpected last char %u, expected %u.\n",
            metrics.tmLastChar, expmetrics.tmLastChar);
    ok(metrics.tmDefaultChar == expmetrics.tmDefaultChar, "Unexpected default char %u, expected %u.\n",
            metrics.tmDefaultChar, expmetrics.tmDefaultChar);
    ok(metrics.tmBreakChar == expmetrics.tmBreakChar, "Unexpected break char %u, expected %u.\n",
            metrics.tmBreakChar, expmetrics.tmBreakChar);
    ok(metrics.tmItalic == expmetrics.tmItalic, "Unexpected italic %u, expected %u.\n",
            metrics.tmItalic, expmetrics.tmItalic);
    ok(metrics.tmUnderlined == expmetrics.tmUnderlined, "Unexpected underlined %u, expected %u.\n",
            metrics.tmUnderlined, expmetrics.tmUnderlined);
    ok(metrics.tmStruckOut == expmetrics.tmStruckOut, "Unexpected struck out %u, expected %u.\n",
            metrics.tmStruckOut, expmetrics.tmStruckOut);
    ok(metrics.tmPitchAndFamily == expmetrics.tmPitchAndFamily, "Unexpected pitch and family %u, expected %u.\n",
            metrics.tmPitchAndFamily, expmetrics.tmPitchAndFamily);
    ok(metrics.tmCharSet == expmetrics.tmCharSet, "Unexpected charset %u, expected %u.\n",
            metrics.tmCharSet, expmetrics.tmCharSet);

    ID3DX10Font_Release(font);

    /* PreloadText */
    hr = D3DX10CreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma", &font);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ID3DX10Font_PreloadTextA(font, NULL, -1);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);
    hr = ID3DX10Font_PreloadTextA(font, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = ID3DX10Font_PreloadTextA(font, NULL, 1);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);
    hr = ID3DX10Font_PreloadTextA(font, "test", -1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = ID3DX10Font_PreloadTextA(font, "", 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = ID3DX10Font_PreloadTextA(font, "", -1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ID3DX10Font_PreloadTextW(font, NULL, -1);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);
    hr = ID3DX10Font_PreloadTextW(font, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = ID3DX10Font_PreloadTextW(font, NULL, 1);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);
    hr = ID3DX10Font_PreloadTextW(font, testW, -1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = ID3DX10Font_PreloadTextW(font, L"", 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = ID3DX10Font_PreloadTextW(font, L"", -1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ID3DX10Font_Release(font);

    /* GetGlyphData, PreloadGlyphs, PreloadCharacters */
    hr = D3DX10CreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma", &font);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hdc = ID3DX10Font_GetDC(font);
    ok(!!hdc, "Unexpected hdc %p.\n", hdc);

    hr = ID3DX10Font_GetGlyphData(font, 0, NULL, &blackbox, &cellinc);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = ID3DX10Font_GetGlyphData(font, 0, &srv, NULL, &cellinc);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        ID3D10ShaderResourceView_Release(srv);
    hr = ID3DX10Font_GetGlyphData(font, 0, &srv, &blackbox, NULL);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        ID3D10ShaderResourceView_Release(srv);

    hr = ID3DX10Font_PreloadCharacters(font, 'b', 'a');
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = ID3DX10Font_PreloadGlyphs(font, 1, 0);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ID3DX10Font_PreloadCharacters(font, 'a', 'a');
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    for (c = 'b'; c <= 'z'; ++c)
    {
        winetest_push_context("Character %c", c);
        count = GetGlyphIndicesA(hdc, &c, 1, &glyph, 0);
        ok(count != GDI_ERROR, "Unexpected count %u.\n", count);

        hr = ID3DX10Font_GetGlyphData(font, glyph, &srv, &blackbox, &cellinc);
        todo_wine
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        if (FAILED(hr))
        {
            winetest_pop_context();
            break;
        }

        ID3D10ShaderResourceView_GetResource(srv, &resource);
        hr = ID3D10Resource_QueryInterface(resource, &IID_ID3D10Texture2D, (void **)&texture);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ID3D10Resource_Release(resource);

        ID3D10Texture2D_GetDesc(texture, &texture_desc);
        ok(texture_desc.Width == 256, "Unexpected width %u.\n", texture_desc.Width);
        ok(texture_desc.Height == 256, "Unexpected height %u.\n", texture_desc.Height);
        ok(texture_desc.MipLevels == 5, "Unexpected miplevels %u.\n", texture_desc.MipLevels);
        ok(texture_desc.ArraySize == 1, "Unexpected array size %u.\n", texture_desc.ArraySize);
        ok(texture_desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM, "Unexpected format %#x.\n",
                texture_desc.Format);
        ok(texture_desc.SampleDesc.Count == 1, "Unexpected samples count %u.\n",
                texture_desc.SampleDesc.Count);
        ok(texture_desc.SampleDesc.Quality == 0, "Unexpected quality level %u.\n",
                texture_desc.SampleDesc.Quality);
        ok(texture_desc.Usage == 0, "Unexpected usage %#x.\n", texture_desc.Usage);
        ok(texture_desc.BindFlags == D3D10_BIND_SHADER_RESOURCE, "Unexpected bind flags %#x.\n",
                texture_desc.BindFlags);
        ok(texture_desc.CPUAccessFlags == 0, "Unexpected access flags %#x.\n",
                texture_desc.CPUAccessFlags);
        ok(texture_desc.MiscFlags == 0, "Unexpected misc flags %#x.\n", texture_desc.MiscFlags);

        count = GetGlyphOutlineW(hdc, glyph, GGO_GLYPH_INDEX | GGO_METRICS, &glyph_metrics, 0, NULL, &mat);
        ok(count != GDI_ERROR, "Unexpected count %#x.\n", count);

        ret = ID3DX10Font_GetTextMetricsW(font, &tm);
        ok(ret, "Unexpected ret %#x.\n", ret);

        todo_wine ok(blackbox.right - blackbox.left == glyph_metrics.gmBlackBoxX + 2, "Got %ld, expected %d.\n",
                blackbox.right - blackbox.left, glyph_metrics.gmBlackBoxX + 2);
        todo_wine ok(blackbox.bottom - blackbox.top == glyph_metrics.gmBlackBoxY + 2, "Got %ld, expected %d.\n",
                blackbox.bottom - blackbox.top, glyph_metrics.gmBlackBoxY + 2);
        ok(cellinc.x == glyph_metrics.gmptGlyphOrigin.x - 1, "Got %ld, expected %ld.\n",
                cellinc.x, glyph_metrics.gmptGlyphOrigin.x - 1);
        ok(cellinc.y == tm.tmAscent - glyph_metrics.gmptGlyphOrigin.y - 1, "Got %ld, expected %ld.\n",
                cellinc.y, tm.tmAscent - glyph_metrics.gmptGlyphOrigin.y - 1);

        ID3D10Texture2D_Release(texture);
        winetest_pop_context();
    }

    hr = ID3DX10Font_PreloadCharacters(font, 'a', 'z');
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

#if D3DX10_SDK_VERSION > 34
    /* Test multiple textures.
     * Native d3dx10_34.dll shows signs of memory corruption in this call. */
    hr = ID3DX10Font_PreloadGlyphs(font, 0, 1000);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
#endif

    /* Test glyphs that are not rendered */
    for (glyph = 1; glyph < 4; ++glyph)
    {
        srv = (void *)0xdeadbeef;
        hr = ID3DX10Font_GetGlyphData(font, glyph, &srv, &blackbox, &cellinc);
    todo_wine {
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!srv, "Unexpected resource view %p.\n", srv);
    }
    }

    ID3DX10Font_Release(font);

    c = 'a';
    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        winetest_push_context("Test %u", i);
        hr = D3DX10CreateFontA(device, tests[i].font_height, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma", &font);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hdc = ID3DX10Font_GetDC(font);
        ok(!!hdc, "Unexpected hdc %p.\n", hdc);

        count = GetGlyphIndicesA(hdc, &c, 1, &glyph, 0);
        ok(count != GDI_ERROR, "Unexpected count %u.\n", count);

        hr = ID3DX10Font_GetGlyphData(font, glyph, &srv, NULL, NULL);
        todo_wine
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        if (FAILED(hr))
        {
            ID3DX10Font_Release(font);
            winetest_pop_context();
            break;
        }

        ID3D10ShaderResourceView_GetResource(srv, &resource);
        hr = ID3D10Resource_QueryInterface(resource, &IID_ID3D10Texture2D, (void **)&texture);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ID3D10Resource_Release(resource);

        ID3D10Texture2D_GetDesc(texture, &texture_desc);

        ok(texture_desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM, "Unexpected format %#x.\n",
                texture_desc.Format);
        ok(texture_desc.Usage == 0, "Unexpected usage %#x.\n", texture_desc.Usage);
        ok(texture_desc.Width == tests[i].expected_size, "Unexpected width %u.\n", texture_desc.Width);
        ok(texture_desc.Height == tests[i].expected_size, "Unexpected height %u.\n", texture_desc.Height);
        ok(texture_desc.CPUAccessFlags == 0, "Unexpected access flags %#x.\n",
                texture_desc.CPUAccessFlags);
        ok(texture_desc.BindFlags == D3D10_BIND_SHADER_RESOURCE, "Unexpected bind flags %#x.\n",
                texture_desc.BindFlags);

        ID3D10Texture2D_Release(texture);

        /* DrawText */
        hr = D3DX10CreateSprite(device, 0, &sprite);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        SetRect(&rect, 0, 0, 640, 480);

        hr = ID3DX10Sprite_Begin(sprite, 0);
        ok (hr == S_OK, "Unexpected hr %#lx.\n", hr);

        height = ID3DX10Font_DrawTextW(font, sprite, testW, -1, &rect, DT_TOP, white);
        ok(height == tests[i].font_height, "Unexpected height %u.\n", height);
        height = ID3DX10Font_DrawTextW(font, sprite, testW, size, &rect, DT_TOP, white);
        ok(height == tests[i].font_height, "Unexpected height %u.\n", height);
        height = ID3DX10Font_DrawTextW(font, sprite, testW, size, &rect, DT_RIGHT, white);
        ok(height == tests[i].font_height, "Unexpected height %u.\n", height);
        height = ID3DX10Font_DrawTextW(font, sprite, testW, size, &rect, DT_LEFT | DT_NOCLIP, white);
        ok(height == tests[i].font_height, "Unexpected height %u.\n", height);

        SetRectEmpty(&rect);
        height = ID3DX10Font_DrawTextW(font, sprite, testW, size, &rect,
                DT_LEFT | DT_CALCRECT, white);
        ok(height == tests[i].font_height, "Unexpected height %u.\n", height);
        ok(!rect.left, "Unexpected rect left %ld.\n", rect.left);
        ok(!rect.top, "Unexpected rect top %ld.\n", rect.top);
        ok(rect.right, "Unexpected rect right %ld.\n", rect.right);
        ok(rect.bottom == tests[i].font_height, "Unexpected rect bottom %ld.\n", rect.bottom);

        hr = ID3DX10Sprite_End(sprite);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ID3DX10Sprite_Release(sprite);

        ID3DX10Font_Release(font);
        winetest_pop_context();
    }

    if (!strcmp(winetest_platform, "wine"))
        return;

    /* DrawText */
    hr = D3DX10CreateFontA(device, 12, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma", &font);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    SetRect(&rect, 10, 10, 200, 200);

    height = ID3DX10Font_DrawTextA(font, NULL, "test", -2, &rect, 0, color);
    ok(height == 12, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextA(font, NULL, "test", -1, &rect, 0, color);
    ok(height == 12, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextA(font, NULL, "test", 0, &rect, 0, color);
    ok(height == 0, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextA(font, NULL, "test", 1, &rect, 0, color);
    ok(height == 12, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextA(font, NULL, "test", 2, &rect, 0, color);
    ok(height == 12, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextA(font, NULL, "", 0, &rect, 0, color);
    ok(height == 0, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextA(font, NULL, "", -1, &rect, 0, color);
    ok(height == 0, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextA(font, NULL, "test", -1, NULL, 0, color);
    ok(height == 12, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextA(font, NULL, "test", -1, NULL, DT_CALCRECT, color);
    ok(height == 12, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextA(font, NULL, NULL, -1, NULL, 0, color);
    ok(height == 0, "Unexpected height %d.\n", height);

    SetRect(&rect, 10, 10, 50, 50);

    height = ID3DX10Font_DrawTextA(font, NULL, long_text, -1, &rect, DT_WORDBREAK, color);
    ok(height == 60, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextA(font, NULL, long_text, -1, &rect, DT_WORDBREAK | DT_NOCLIP, color);
    ok(height == 96, "Unexpected height %d.\n", height);

    SetRect(&rect, 10, 10, 200, 200);

    height = ID3DX10Font_DrawTextW(font, NULL, testW, -1, &rect, 0, color);
    ok(height == 12, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, testW, 0, &rect, 0, color);
    ok(height == 0, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, testW, 1, &rect, 0, color);
    ok(height == 12, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, testW, 2, &rect, 0, color);
    ok(height == 12, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"", 0, &rect, 0, color);
    ok(height == 0, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"", -1, &rect, 0, color);
    ok(height == 0, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, testW, -1, NULL, 0, color);
    ok(height == 12, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, testW, -1, NULL, DT_CALCRECT, color);
    ok(height == 12, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, NULL, -1, NULL, 0, color);
    ok(height == 0, "Unexpected height %d.\n", height);

    SetRect(&rect, 10, 10, 50, 50);

    height = ID3DX10Font_DrawTextW(font, NULL, long_textW, -1, &rect, DT_WORDBREAK, color);
    ok(height == 60, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, long_textW, -1, &rect, DT_WORDBREAK | DT_NOCLIP, color);
    ok(height == 96, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"a\na", -1, NULL, 0, color);
    ok(height == 24, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"a\na", -1, &rect, 0, color);
    ok(height == 24, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"a\r\na", -1, &rect, 0, color);
    ok(height == 24, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"a\ra", -1, &rect, 0, color);
    ok(height == 12, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"a\na", -1, &rect, DT_SINGLELINE, color);
    ok(height == 12, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"a\naaaaa aaaa", -1, &rect, DT_SINGLELINE, color);
    ok(height == 12, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"a\naaaaa aaaa", -1, &rect, 0, color);
    ok(height == 24, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"a\naaaaa aaaa", -1, &rect, DT_WORDBREAK, color);
    ok(height == 36, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"a\naaaaa aaaa", -1, &rect, DT_WORDBREAK | DT_SINGLELINE, color);
    ok(height == 12, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"1\n2\n3\n4\n5\n6", -1, &rect, 0, color);
    ok(height == 48, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"1\n2\n3\n4\n5\n6", -1, &rect, DT_NOCLIP, color);
    ok(height == 72, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"\t\t\t\t\t\t\t\t\t\t", -1, &rect, DT_WORDBREAK, color);
    ok(height == 0, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"\t\t\t\t\t\t\t\t\t\ta", -1, &rect, DT_WORDBREAK, color);
    ok(height == 12, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"\taaaaaaaaaa", -1, &rect, DT_WORDBREAK, color);
    ok(height == 24, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"\taaaaaaaaaa", -1, &rect, DT_EXPANDTABS | DT_WORDBREAK, color);
    ok(height == 36, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"\taaa\taaa\taaa", -1, &rect, DT_WORDBREAK, color);
    ok(height == 24, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"\taaa\taaa\taaa", -1, &rect, DT_EXPANDTABS | DT_WORDBREAK, color);
    ok(height == 48, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"\t\t\t\t\t\t\t\t\t\t", -1, &rect, DT_EXPANDTABS | DT_WORDBREAK, color);
    ok(height == 60, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"a\ta", -1, &rect, DT_EXPANDTABS | DT_WORDBREAK, color);
    ok(height == 12, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"a\ta\ta", -1, &rect, DT_EXPANDTABS | DT_WORDBREAK, color);
    ok(height == 24, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaaaaaaaaaaaaaaaaaa", -1, &rect, DT_WORDBREAK, color);
    ok(height == 36, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"a                        a", -1, &rect, DT_WORDBREAK, color);
    ok(height == 36, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa              aaaa", -1, &rect, DT_WORDBREAK, color);
    ok(height == 36, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa              aaaa", -1, &rect, DT_WORDBREAK | DT_RIGHT, color);
    ok(height == 36, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa              aaaa", -1, &rect, DT_WORDBREAK | DT_CENTER, color);
    ok(height == 36, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_BOTTOM, color);
    ok(height == 40, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_VCENTER, color);
    ok(height == 32, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_RIGHT, color);
    ok(height == 24, "Unexpected height %d.\n", height);

    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER, color);
    ok(height == 24, "Unexpected height %d.\n", height);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, 10, 30, 34);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, -10, 10, 10, 34);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, -10, 30, 14);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, 10, 30, 34);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, 10, 30, 34);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, 10, 30, 34);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, -10, 10, 10, 34);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, -10, 30, 14);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 12, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, 10, 53, 22);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, 10, 30, 34);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_BOTTOM | DT_CALCRECT, color);
    ok(height == 40, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, 26, 30, 50);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_BOTTOM | DT_CALCRECT, color);
    ok(height == 40, "Unexpected height %d.\n", height);
    check_rect(&rect, -10, 26, 10, 50);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_BOTTOM | DT_CALCRECT, color);
    ok(height == 40, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, 6, 30, 30);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_BOTTOM | DT_CALCRECT, color);
    ok(height == 40, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, 26, 30, 50);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_BOTTOM | DT_CALCRECT, color);
    ok(height == -40, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, -54, 30, -30);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_BOTTOM | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 40, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, 26, 30, 50);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_BOTTOM | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 40, "Unexpected height %d.\n", height);
    check_rect(&rect, -10, 26, 10, 50);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_BOTTOM | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 40, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, 6, 30, 30);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_BOTTOM | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 40, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, 38, 53, 50);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_BOTTOM | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == -40, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, -54, 30, -30);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_VCENTER | DT_CALCRECT, color);
    ok(height == 32, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, 18, 30, 42);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_VCENTER | DT_CALCRECT, color);
    ok(height == 32, "Unexpected height %d.\n", height);
    check_rect(&rect, -10, 18, 10, 42);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_VCENTER | DT_CALCRECT, color);
    ok(height == 32, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, -2, 30, 22);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_VCENTER | DT_CALCRECT, color);
    ok(height == 32, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, 18, 30, 42);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_VCENTER | DT_CALCRECT, color);
    ok(height == -8, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, -22, 30, 2);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_VCENTER | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 32, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, 18, 30, 42);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_VCENTER | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 32, "Unexpected height %d.\n", height);
    check_rect(&rect, -10, 18, 10, 42);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_VCENTER | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 32, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, -2, 30, 22);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_VCENTER | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 26, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, 24, 53, 36);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_VCENTER | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == -8, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, -22, 30, 2);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_RIGHT | DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, 30, 10, 50, 34);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_RIGHT | DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, 10, 30, 34);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_RIGHT | DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, 30, -10, 50, 14);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_RIGHT | DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, -50, 10, -30, 34);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_RIGHT | DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, 30, 10, 50, 34);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_RIGHT | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, 30, 10, 50, 34);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_RIGHT | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, 10, 10, 30, 34);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_RIGHT | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, 30, -10, 50, 14);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_RIGHT | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 12, "Unexpected height %d.\n", height);
    check_rect(&rect, -73, 10, -30, 22);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_RIGHT | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, 30, 10, 50, 34);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER | DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, 20, 10, 40, 34);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER | DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, 0, 10, 20, 34);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER | DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, 20, -10, 40, 14);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER | DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, -20, 10, 0, 34);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER | DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, 20, 10, 40, 34);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_CENTER | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, 20, 10, 40, 34);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_CENTER | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, 0, 10, 20, 34);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_CENTER | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, 20, -10, 40, 14);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_CENTER | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 12, "Unexpected height %d.\n", height);
    check_rect(&rect, -31, 10, 12, 22);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_CENTER | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 24, "Unexpected height %d.\n", height);
    check_rect(&rect, 20, 10, 40, 34);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_CALCRECT, color);
    ok(height == 32, "Unexpected height %d.\n", height);
    check_rect(&rect, 20, 18, 40, 42);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_CALCRECT, color);
    ok(height == 32, "Unexpected height %d.\n", height);
    check_rect(&rect, 20, 18, 40, 42);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_CALCRECT, color);
    ok(height == 32, "Unexpected height %d.\n", height);
    check_rect(&rect, 0, 18, 20, 42);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_CALCRECT, color);
    ok(height == 32, "Unexpected height %d.\n", height);
    check_rect(&rect, 20, -2, 40, 22);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_CALCRECT, color);
    ok(height == 32, "Unexpected height %d.\n", height);
    check_rect(&rect, -20, 18, 0, 42);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa\naaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_CALCRECT, color);
    ok(height == -8, "Unexpected height %d.\n", height);
    check_rect(&rect, 20, -22, 40, 2);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 32, "Unexpected height %d.\n", height);
    check_rect(&rect, 20, 18, 40, 42);

    SetRect(&rect, 10, 10, 50, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 32, "Unexpected height %d.\n", height);
    check_rect(&rect, 20, 18, 40, 42);

    SetRect(&rect, -10, 10, 30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 32, "Unexpected height %d.\n", height);
    check_rect(&rect, 0, 18, 20, 42);

    SetRect(&rect, 10, -10, 50, 30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 32, "Unexpected height %d.\n", height);
    check_rect(&rect, 20, -2, 40, 22);

    SetRect(&rect, 10, 10, -30, 50);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == 26, "Unexpected height %d.\n", height);
    check_rect(&rect, -31, 24, 12, 36);

    SetRect(&rect, 10, 10, 50, -30);
    height = ID3DX10Font_DrawTextW(font, NULL, L"aaaa aaaa", -1, &rect, DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_CALCRECT, color);
    ok(height == -8, "Unexpected height %d.\n", height);
    check_rect(&rect, 20, -22, 40, 2);

    ID3DX10Font_Release(font);
}

static void test_sprite(void)
{
    ID3D10ShaderResourceView *srv1, *srv2;
    ID3D10Texture2D *texture1, *texture2;
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10Device *device, *device2;
    D3DX10_SPRITE sprite_desc;
    ID3DX10Sprite *sprite;
    D3DXMATRIX mat, mat2;
    ULONG refcount;
    HRESULT hr;
    static const D3DXMATRIX identity =
    {
        ._11 = 1.0f,
        ._22 = 1.0f,
        ._33 = 1.0f,
        ._44 = 1.0f,
    };

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    texture_desc.Width = 64;
    texture_desc.Height = 64;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &texture1);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#lx.\n", hr);

    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &texture2);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#lx.\n", hr);

    hr = ID3D10Device_CreateShaderResourceView(device, (ID3D10Resource *)texture1, NULL, &srv1);
    ok(SUCCEEDED(hr), "Failed to create srv, hr %#lx.\n", hr);

    hr = ID3D10Device_CreateShaderResourceView(device, (ID3D10Resource *)texture1, NULL, &srv2);
    ok(SUCCEEDED(hr), "Failed to create srv, hr %#lx.\n", hr);

    hr = D3DX10CreateSprite(device, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    hr = D3DX10CreateSprite(NULL, 0, &sprite);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    hr = D3DX10CreateSprite(device, 0, &sprite);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* GetDevice */
    hr = ID3DX10Sprite_GetDevice(sprite, NULL);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = ID3DX10Sprite_GetDevice(sprite, &device2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(device == device2, "Unexpected device.\n");

    ID3D10Device_Release(device2);

    /* Projection transform */
    hr = ID3DX10Sprite_GetProjectionTransform(sprite, NULL);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    hr = ID3DX10Sprite_GetProjectionTransform(sprite, &mat);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!memcmp(&mat, &identity, sizeof(mat)), "Unexpected projection transform.\n");

    /* Set a transform and test if it gets returned correctly */
    mat.m[0][0] = 2.1f; mat.m[0][1] = 6.5f; mat.m[0][2] =-9.6f; mat.m[0][3] = 1.7f;
    mat.m[1][0] = 4.2f; mat.m[1][1] =-2.5f; mat.m[1][2] = 2.1f; mat.m[1][3] = 5.5f;
    mat.m[2][0] =-2.6f; mat.m[2][1] = 0.3f; mat.m[2][2] = 8.6f; mat.m[2][3] = 8.4f;
    mat.m[3][0] = 6.7f; mat.m[3][1] =-5.1f; mat.m[3][2] = 6.1f; mat.m[3][3] = 2.2f;

    hr = ID3DX10Sprite_SetProjectionTransform(sprite, NULL);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = ID3DX10Sprite_SetProjectionTransform(sprite, &mat);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ID3DX10Sprite_GetProjectionTransform(sprite, &mat2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!memcmp(&mat, &mat2, sizeof(mat)), "Unexpected matrix.\n");

    /* View transform */
    hr = ID3DX10Sprite_SetViewTransform(sprite, NULL);
    todo_wine
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = ID3DX10Sprite_SetViewTransform(sprite, &mat);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Begin */
    hr = ID3DX10Sprite_Begin(sprite, 0);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Flush/End */
    hr = ID3DX10Sprite_Flush(sprite);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ID3DX10Sprite_End(sprite);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* May not be called before next Begin */
    hr = ID3DX10Sprite_Flush(sprite);
    todo_wine
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    hr = ID3DX10Sprite_End(sprite);
    todo_wine
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    /* Draw */
    hr = ID3DX10Sprite_DrawSpritesBuffered(sprite, NULL, 0);
    todo_wine
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    memset(&sprite_desc, 0, sizeof(sprite_desc));
    hr = ID3DX10Sprite_DrawSpritesBuffered(sprite, &sprite_desc, 0);
    todo_wine
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = ID3DX10Sprite_DrawSpritesBuffered(sprite, &sprite_desc, 1);
    todo_wine
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = ID3DX10Sprite_Begin(sprite, 0);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    memset(&sprite_desc, 0, sizeof(sprite_desc));
    hr = ID3DX10Sprite_DrawSpritesBuffered(sprite, &sprite_desc, 1);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    sprite_desc.pTexture = srv1;
    hr = ID3DX10Sprite_DrawSpritesBuffered(sprite, &sprite_desc, 1);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ID3DX10Sprite_Flush(sprite);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ID3DX10Sprite_Flush(sprite);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ID3DX10Sprite_End(sprite);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* D3DX10_SPRITE_ADDREF_TEXTURES */
    hr = ID3DX10Sprite_Begin(sprite, D3DX10_SPRITE_ADDREF_TEXTURES);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    memset(&sprite_desc, 0, sizeof(sprite_desc));
    sprite_desc.pTexture = srv1;

    refcount = get_refcount(srv1);
    hr = ID3DX10Sprite_DrawSpritesBuffered(sprite, &sprite_desc, 1);
todo_wine {
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(get_refcount(srv1) > refcount, "Unexpected refcount.\n");
}

    hr = ID3DX10Sprite_Flush(sprite);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(get_refcount(srv1) == refcount, "Unexpected refcount.\n");

    hr = ID3DX10Sprite_End(sprite);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ID3DX10Sprite_Release(sprite);
    ID3D10Texture2D_Release(texture1);
    ID3D10Texture2D_Release(texture2);
    ID3D10ShaderResourceView_Release(srv1);
    ID3D10ShaderResourceView_Release(srv2);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Unexpected refcount.\n");
}

static void test_create_effect_from_memory(void)
{
    ID3D10Device *device;
    ID3D10Effect *effect;
    ID3D10Blob *errors;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    /* Test NULL data. */
    errors = (ID3D10Blob *)0xdeadbeef;
    effect = (ID3D10Effect *)0xdeadbeef;
    hr = D3DX10CreateEffectFromMemory(NULL, 0, NULL, NULL, NULL, NULL,
            0x0, 0x0, device, NULL, NULL, &effect, &errors, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(errors == (ID3D10Blob *)0xdeadbeef, "Got unexpected errors %p.\n", errors);
    ok(effect == (ID3D10Effect *)0xdeadbeef, "Got unexpected effect %p.\n", effect);

    /* Test NULL device. */
    errors = (ID3D10Blob *)0xdeadbeef;
    effect = (ID3D10Effect *)0xdeadbeef;
    hr = D3DX10CreateEffectFromMemory(test_fx, sizeof(test_fx), NULL, NULL, NULL, NULL,
            0x0, 0x0, NULL, NULL, NULL, &effect, &errors, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(errors == (ID3D10Blob *)0xdeadbeef, "Got unexpected errors %p.\n", errors);
    ok(effect == (ID3D10Effect *)0xdeadbeef, "Got unexpected effect %p.\n", effect);

    /* Test creating effect from compiled shader. */
    errors = (ID3D10Blob *)0xdeadbeef;
    effect = (ID3D10Effect *)0xdeadbeef;
    hr = D3DX10CreateEffectFromMemory(test_fx, sizeof(test_fx), NULL, NULL, NULL, NULL,
            0x0, 0x0, device, NULL, NULL, &effect, &errors, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!errors, "Got unexpected errors %p.\n", errors);
    ok(!!effect && effect != (ID3D10Effect *)0xdeadbeef, "Got unexpected effect %p.\n", effect);
    effect->lpVtbl->Release(effect);

    /* Test creating effect from source without setting profile. */
    errors = (ID3D10Blob *)0xdeadbeef;
    effect = (ID3D10Effect *)0xdeadbeef;
    hr = D3DX10CreateEffectFromMemory(test_fx_source, strlen(test_fx_source) + 1, NULL, NULL, NULL, NULL,
            0x0, 0x0, device, NULL, NULL, &effect, &errors, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    todo_wine ok(!!errors && errors != (ID3D10Blob *)0xdeadbeef, "Got unexpected errors %p.\n", errors);
    ok(effect == (ID3D10Effect *)0xdeadbeef, "Got unexpected effect %p.\n", effect);
    if (errors && errors != (ID3D10Blob *)0xdeadbeef)
        ID3D10Blob_Release(errors);

    /* Test creating effect from source. */
    errors = (ID3D10Blob *)0xdeadbeef;
    effect = (ID3D10Effect *)0xdeadbeef;
    hr = D3DX10CreateEffectFromMemory(test_fx_source, strlen(test_fx_source) + 1, NULL, NULL, NULL, "fx_4_0",
            0x0, 0x0, device, NULL, NULL, &effect, &errors, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!errors, "Got unexpected errors %p.\n", errors);
    ok(!!effect && effect != (ID3D10Effect *)0xdeadbeef, "Got unexpected effect %p.\n", effect);
    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Got unexpected refcount %lu.\n", refcount);
}

static void test_create_effect_from_file(void)
{
    static const WCHAR *test_file_name = L"test.fx";
    WCHAR path[MAX_PATH];
    ID3D10Device *device;
    ID3D10Effect *effect;
    ID3D10Blob *errors;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    /* Test NULL file name. */
    errors = (ID3D10Blob *)0xdeadbeef;
    effect = (ID3D10Effect *)0xdeadbeef;
    hr = D3DX10CreateEffectFromFileW(NULL, NULL, NULL, NULL, 0x0, 0x0,
            device, NULL, NULL, &effect, &errors, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(errors == (ID3D10Blob *)0xdeadbeef, "Got unexpected errors %p.\n", errors);
    ok(effect == (ID3D10Effect *)0xdeadbeef, "Got unexpected effect %p.\n", effect);

    errors = (ID3D10Blob *)0xdeadbeef;
    effect = (ID3D10Effect *)0xdeadbeef;
    hr = D3DX10CreateEffectFromFileA(NULL, NULL, NULL, NULL, 0x0, 0x0,
            device, NULL, NULL, &effect, &errors, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(errors == (ID3D10Blob *)0xdeadbeef, "Got unexpected errors %p.\n", errors);
    ok(effect == (ID3D10Effect *)0xdeadbeef, "Got unexpected effect %p.\n", effect);

    /* Test non-existent file. */
    errors = (ID3D10Blob *)0xdeadbeef;
    effect = (ID3D10Effect *)0xdeadbeef;
    hr = D3DX10CreateEffectFromFileW(L"deadbeef", NULL, NULL, NULL, 0x0, 0x0,
            device, NULL, NULL, &effect, &errors, NULL);
    ok(hr == D3D10_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);
    todo_wine ok(!errors, "Got unexpected errors %p.\n", errors);
    ok(effect == (ID3D10Effect *)0xdeadbeef, "Got unexpected effect %p.\n", effect);

    errors = (ID3D10Blob *)0xdeadbeef;
    effect = (ID3D10Effect *)0xdeadbeef;
    hr = D3DX10CreateEffectFromFileA("deadbeef", NULL, NULL, NULL, 0x0, 0x0,
            device, NULL, NULL, &effect, &errors, NULL);
    ok(hr == D3D10_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);
    todo_wine ok(!errors, "Got unexpected errors %p.\n", errors);
    ok(effect == (ID3D10Effect *)0xdeadbeef, "Got unexpected effect %p.\n", effect);

    /* Test creating effect from compiled shader file. */
    create_file(test_file_name, test_fx, sizeof(test_fx), path);

    errors = (ID3D10Blob *)0xdeadbeef;
    effect = (ID3D10Effect *)0xdeadbeef;
    hr = D3DX10CreateEffectFromFileW(path, NULL, NULL, NULL, 0x0, 0x0,
            device, NULL, NULL, &effect, &errors, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!errors, "Got unexpected errors %p.\n", errors);
    ok(!!effect && effect != (ID3D10Effect *)0xdeadbeef, "Got unexpected effect %p.\n", effect);
    effect->lpVtbl->Release(effect);

    errors = (ID3D10Blob *)0xdeadbeef;
    effect = (ID3D10Effect *)0xdeadbeef;
    hr = D3DX10CreateEffectFromFileA(get_str_a(path), NULL, NULL, NULL, 0x0, 0x0,
            device, NULL, NULL, &effect, &errors, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!errors, "Got unexpected errors %p.\n", errors);
    ok(!!effect && effect != (ID3D10Effect *)0xdeadbeef, "Got unexpected effect %p.\n", effect);
    effect->lpVtbl->Release(effect);

    delete_file(test_file_name);

    /* Test creating effect from source file. */
    create_file(test_file_name, test_fx_source, strlen(test_fx_source) + 1, path);

    errors = (ID3D10Blob *)0xdeadbeef;
    effect = (ID3D10Effect *)0xdeadbeef;
    hr = D3DX10CreateEffectFromFileW(path, NULL, NULL, "fx_4_0", 0x0, 0x0,
            device, NULL, NULL, &effect, &errors, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!errors, "Got unexpected errors %p.\n", errors);
    ok(effect && effect != (ID3D10Effect *)0xdeadbeef, "Got unexpected effect %p.\n", effect);
    effect->lpVtbl->Release(effect);

    errors = (ID3D10Blob *)0xdeadbeef;
    effect = (ID3D10Effect *)0xdeadbeef;
    hr = D3DX10CreateEffectFromFileA(get_str_a(path), NULL, NULL, "fx_4_0", 0x0, 0x0,
            device, NULL, NULL, &effect, &errors, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!errors, "Got unexpected errors %p.\n", errors);
    ok(effect && effect != (ID3D10Effect *)0xdeadbeef, "Got unexpected effect %p.\n", effect);
    effect->lpVtbl->Release(effect);

    delete_file(test_file_name);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Got unexpected refcount %lu.\n", refcount);
}

static void test_create_effect_from_resource(void)
{
    static const WCHAR *test_resource_name = L"test.fx";
    HMODULE resource_module;
    ID3D10Device *device;
    ID3D10Effect *effect;
    ID3D10Blob *errors;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    /* Test NULL module. */
    errors = (ID3D10Blob *)0xdeadbeef;
    effect = (ID3D10Effect *)0xdeadbeef;
    hr = D3DX10CreateEffectFromResourceW(NULL, NULL, NULL, NULL, NULL, NULL,
            0, 0, device, NULL, NULL, &effect, &errors, NULL);
    ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(errors == (ID3D10Blob *)0xdeadbeef, "Got unexpected errors %p.\n", errors);
    ok(effect == (ID3D10Effect *)0xdeadbeef, "Got unexpected effect %p.\n", effect);

    errors = (ID3D10Blob *)0xdeadbeef;
    effect = (ID3D10Effect *)0xdeadbeef;
    hr = D3DX10CreateEffectFromResourceA(NULL, NULL, NULL, NULL, NULL, NULL,
            0, 0, device, NULL, NULL, &effect, &errors, NULL);
    ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(errors == (ID3D10Blob *)0xdeadbeef, "Got unexpected errors %p.\n", errors);
    ok(effect == (ID3D10Effect *)0xdeadbeef, "Got unexpected effect %p.\n", effect);

    /* Test NULL resource name. */
    errors = (ID3D10Blob *)0xdeadbeef;
    effect = (ID3D10Effect *)0xdeadbeef;
    hr = D3DX10CreateEffectFromResourceW(GetModuleHandleW(NULL), NULL, NULL, NULL, NULL, NULL,
            0, 0, device, NULL, NULL, &effect, &errors, NULL);
    ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(errors == (ID3D10Blob *)0xdeadbeef, "Got unexpected errors %p.\n", errors);
    ok(effect == (ID3D10Effect *)0xdeadbeef, "Got unexpected effect %p.\n", effect);

    errors = (ID3D10Blob *)0xdeadbeef;
    effect = (ID3D10Effect *)0xdeadbeef;
    hr = D3DX10CreateEffectFromResourceA(GetModuleHandleA(NULL), NULL, NULL, NULL, NULL, NULL,
            0, 0, device, NULL, NULL, &effect, &errors, NULL);
    ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(errors == (ID3D10Blob *)0xdeadbeef, "Got unexpected errors %p.\n", errors);
    ok(effect == (ID3D10Effect *)0xdeadbeef, "Got unexpected effect %p.\n", effect);

    /* Test non-existent resource name. */
    errors = (ID3D10Blob *)0xdeadbeef;
    effect = (ID3D10Effect *)0xdeadbeef;
    hr = D3DX10CreateEffectFromResourceW(GetModuleHandleW(NULL), L"deadbeef", NULL, NULL, NULL, NULL,
            0, 0, device, NULL, NULL, &effect, &errors, NULL);
    ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(errors == (ID3D10Blob *)0xdeadbeef, "Got unexpected errors %p.\n", errors);
    ok(effect == (ID3D10Effect *)0xdeadbeef, "Got unexpected effect %p.\n", effect);

    errors = (ID3D10Blob *)0xdeadbeef;
    effect = (ID3D10Effect *)0xdeadbeef;
    hr = D3DX10CreateEffectFromResourceA(GetModuleHandleA(NULL), "deadbeef", NULL, NULL, NULL, NULL,
            0, 0, device, NULL, NULL, &effect, &errors, NULL);
    ok(hr == D3DX10_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(errors == (ID3D10Blob *)0xdeadbeef, "Got unexpected errors %p.\n", errors);
    ok(effect == (ID3D10Effect *)0xdeadbeef, "Got unexpected effect %p.\n", effect);

    /* Test creating effect from compiled shader resource. */
    resource_module = create_resource_module(test_resource_name, test_fx, sizeof(test_fx));

    errors = (ID3D10Blob *)0xdeadbeef;
    effect = (ID3D10Effect *)0xdeadbeef;
    hr = D3DX10CreateEffectFromResourceW(resource_module, test_resource_name, NULL, NULL, NULL, NULL,
            0, 0, device, NULL, NULL, &effect, &errors, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!errors, "Got unexpected errors %p.\n", errors);
    ok(!!effect && effect != (ID3D10Effect *)0xdeadbeef, "Got unexpected effect %p.\n", effect);
    effect->lpVtbl->Release(effect);

    errors = (ID3D10Blob *)0xdeadbeef;
    effect = (ID3D10Effect *)0xdeadbeef;
    hr = D3DX10CreateEffectFromResourceA(resource_module, get_str_a(test_resource_name), NULL, NULL, NULL, NULL,
            0, 0, device, NULL, NULL, &effect, &errors, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!errors, "Got unexpected errors %p.\n", errors);
    ok(!!effect && effect != (ID3D10Effect *)0xdeadbeef, "Got unexpected effect %p.\n", effect);
    effect->lpVtbl->Release(effect);

    delete_resource_module(test_resource_name, resource_module);

    /* Test creating effect from source resource. */
    resource_module = create_resource_module(test_resource_name, test_fx_source, strlen(test_fx_source) + 1);

    errors = (ID3D10Blob *)0xdeadbeef;
    effect = (ID3D10Effect *)0xdeadbeef;
    hr = D3DX10CreateEffectFromResourceW(resource_module, test_resource_name, NULL, NULL, NULL, "fx_4_0",
            0, 0, device, NULL, NULL, &effect, &errors, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!errors, "Got unexpected errors %p.\n", errors);
    ok(effect && effect != (ID3D10Effect *)0xdeadbeef, "Got unexpected effect %p.\n", effect);
    effect->lpVtbl->Release(effect);

    errors = (ID3D10Blob *)0xdeadbeef;
    effect = (ID3D10Effect *)0xdeadbeef;
    hr = D3DX10CreateEffectFromResourceA(resource_module, get_str_a(test_resource_name), NULL, NULL, NULL, "fx_4_0",
            0, 0, device, NULL, NULL, &effect, &errors, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!errors, "Got unexpected errors %p.\n", errors);
    ok(effect && effect != (ID3D10Effect *)0xdeadbeef, "Got unexpected effect %p.\n", effect);
    effect->lpVtbl->Release(effect);

    delete_resource_module(test_resource_name, resource_module);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Got unexpected refcount %lu.\n", refcount);
}

static void test_preprocess_shader(void)
{
    static const char shader_source[] =
        "float4 main()\n"
        "{\n"
        "    return float4(1.0);\n"
        "}\n";
    ID3D10Blob *preprocessed, *errors;
    HRESULT hr, hr2;

    hr2 = 0xdeadbeef;
    hr = D3DX10PreprocessShaderFromMemory(NULL, 0, NULL, NULL, NULL,
            NULL, &preprocessed, &errors, &hr2);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Unexpected hr2 %#lx.\n", hr2);

    hr2 = 0xdeadbeef;
    hr = D3DX10PreprocessShaderFromMemory(shader_source, strlen(shader_source), NULL, NULL, NULL,
            NULL, &preprocessed, &errors, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!preprocessed, "Unexpected preprocessed %p.\n", preprocessed);
    ok(!errors, "Unexpected errors %p.\n", errors);
    ID3D10Blob_Release(preprocessed);

    hr2 = 0xdeadbeef;
    hr = D3DX10PreprocessShaderFromMemory(shader_source, strlen(shader_source), NULL, NULL, NULL,
            NULL, &preprocessed, &errors, &hr2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Unexpected hr2 %#lx.\n", hr2);
    ok(!!preprocessed, "Unexpected preprocessed %p.\n", preprocessed);
    ok(!errors, "Unexpected errors %p.\n", errors);
    ID3D10Blob_Release(preprocessed);
}

START_TEST(d3dx10)
{
    test_D3DX10UnsetAllDeviceObjects();
    test_D3DX10CreateAsyncMemoryLoader();
    test_D3DX10CreateAsyncFileLoader();
    test_D3DX10CreateAsyncResourceLoader();
    test_D3DX10CreateAsyncTextureInfoProcessor();
    test_D3DX10CreateAsyncTextureProcessor();
    test_D3DX10CreateThreadPump();
    test_get_image_info();
    test_create_texture();
    test_font();
    test_sprite();
    test_create_effect_from_memory();
    test_create_effect_from_file();
    test_create_effect_from_resource();
    test_preprocess_shader();
}
