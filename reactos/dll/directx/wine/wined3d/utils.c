/*
 * Utility functions for the WineD3D Library
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006-2008 Henri Verbeet
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
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

#include "config.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

struct StaticPixelFormatDesc
{
    WINED3DFORMAT format;
    DWORD alphaMask, redMask, greenMask, blueMask;
    UINT bpp;
    short depthSize, stencilSize;
    BOOL isFourcc;
};

/*****************************************************************************
 * Pixel format array
 *
 * For the formats WINED3DFMT_A32B32G32R32F, WINED3DFMT_A16B16G16R16F,
 * and WINED3DFMT_A16B16G16R16 do not have correct alpha masks, because the
 * high masks do not fit into the 32 bit values needed for ddraw. It is only
 * used for ddraw mostly, and to figure out if the format has alpha at all, so
 * setting a mask like 0x1 for those surfaces is correct. The 64 and 128 bit
 * formats are not usable in 2D rendering because ddraw doesn't support them.
 */
static const struct StaticPixelFormatDesc formats[] =
{
  /* WINED3DFORMAT               alphamask    redmask    greenmask    bluemask     bpp    depth  stencil   isFourcc */
    {WINED3DFMT_UNKNOWN,            0x0,        0x0,        0x0,        0x0,        0,      0,      0,      FALSE},
    /* FourCC formats, kept here to have WINED3DFMT_R8G8B8(=20) at position 20 */
    {WINED3DFMT_UYVY,               0x0,        0x0,        0x0,        0x0,        2,      0,      0,      TRUE },
    {WINED3DFMT_YUY2,               0x0,        0x0,        0x0,        0x0,        2,      0,      0,      TRUE },
    {WINED3DFMT_YV12,               0x0,        0x0,        0x0,        0x0,        1,      0,      0,      TRUE },
    {WINED3DFMT_DXT1,               0x0,        0x0,        0x0,        0x0,        1,      0,      0,      TRUE },
    {WINED3DFMT_DXT2,               0x0,        0x0,        0x0,        0x0,        1,      0,      0,      TRUE },
    {WINED3DFMT_DXT3,               0x0,        0x0,        0x0,        0x0,        1,      0,      0,      TRUE },
    {WINED3DFMT_DXT4,               0x0,        0x0,        0x0,        0x0,        1,      0,      0,      TRUE },
    {WINED3DFMT_DXT5,               0x0,        0x0,        0x0,        0x0,        1,      0,      0,      TRUE },
    {WINED3DFMT_MULTI2_ARGB8,       0x0,        0x0,        0x0,        0x0,        1/*?*/, 0,      0,      TRUE },
    {WINED3DFMT_G8R8_G8B8,          0x0,        0x0,        0x0,        0x0,        1/*?*/, 0,      0,      TRUE },
    {WINED3DFMT_R8G8_B8G8,          0x0,        0x0,        0x0,        0x0,        1/*?*/, 0,      0,      TRUE },
    /* IEEE formats */
    {WINED3DFMT_R32_FLOAT,          0x0,        0x0,        0x0,        0x0,        4,      0,      0,      FALSE},
    {WINED3DFMT_R32G32_FLOAT,       0x0,        0x0,        0x0,        0x0,        8,      0,      0,      FALSE},
    {WINED3DFMT_R32G32B32_FLOAT,    0x0,        0x0,        0x0,        0x0,        12,     0,      0,      FALSE},
    {WINED3DFMT_R32G32B32A32_FLOAT, 0x1,        0x0,        0x0,        0x0,        16,     0,      0,      FALSE},
    /* Hmm? */
    {WINED3DFMT_CxV8U8,             0x0,        0x0,        0x0,        0x0,        2,      0,      0,      FALSE},
    /* Float */
    {WINED3DFMT_R16_FLOAT,          0x0,        0x0,        0x0,        0x0,        2,      0,      0,      FALSE},
    {WINED3DFMT_R16G16_FLOAT,       0x0,        0x0,        0x0,        0x0,        4,      0,      0,      FALSE},
    {WINED3DFMT_R16G16_SINT,        0x0,        0x0,        0x0,        0x0,        4,      0,      0,      FALSE},
    {WINED3DFMT_R16G16B16A16_FLOAT, 0x1,        0x0,        0x0,        0x0,        8,      0,      0,      FALSE},
    {WINED3DFMT_R16G16B16A16_SINT,  0x1,        0x0,        0x0,        0x0,        8,      0,      0,      FALSE},
    /* Palettized formats */
    {WINED3DFMT_A8P8,               0x0000ff00, 0x0,        0x0,        0x0,        2,      0,      0,      FALSE},
    {WINED3DFMT_P8,                 0x0,        0x0,        0x0,        0x0,        1,      0,      0,      FALSE},
    /* Standard ARGB formats. */
    {WINED3DFMT_R8G8B8,             0x0,        0x00ff0000, 0x0000ff00, 0x000000ff, 3,      0,      0,      FALSE},
    {WINED3DFMT_A8R8G8B8,           0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff, 4,      0,      0,      FALSE},
    {WINED3DFMT_X8R8G8B8,           0x0,        0x00ff0000, 0x0000ff00, 0x000000ff, 4,      0,      0,      FALSE},
    {WINED3DFMT_R5G6B5,             0x0,        0x0000f800, 0x000007e0, 0x0000001f, 2,      0,      0,      FALSE},
    {WINED3DFMT_X1R5G5B5,           0x0,        0x00007c00, 0x000003e0, 0x0000001f, 2,      0,      0,      FALSE},
    {WINED3DFMT_A1R5G5B5,           0x00008000, 0x00007c00, 0x000003e0, 0x0000001f, 2,      0,      0,      FALSE},
    {WINED3DFMT_A4R4G4B4,           0x0000f000, 0x00000f00, 0x000000f0, 0x0000000f, 2,      0,      0,      FALSE},
    {WINED3DFMT_R3G3B2,             0x0,        0x000000e0, 0x0000001c, 0x00000003, 1,      0,      0,      FALSE},
    {WINED3DFMT_A8_UNORM,           0x000000ff, 0x0,        0x0,        0x0,        1,      0,      0,      FALSE},
    {WINED3DFMT_A8R3G3B2,           0x0000ff00, 0x000000e0, 0x0000001c, 0x00000003, 2,      0,      0,      FALSE},
    {WINED3DFMT_X4R4G4B4,           0x0,        0x00000f00, 0x000000f0, 0x0000000f, 2,      0,      0,      FALSE},
    {WINED3DFMT_R10G10B10A2_UNORM,  0xb0000000, 0x000003ff, 0x000ffc00, 0x3ff00000, 4,      0,      0,      FALSE},
    {WINED3DFMT_R10G10B10A2_UINT,   0xb0000000, 0x000003ff, 0x000ffc00, 0x3ff00000, 4,      0,      0,      FALSE},
    {WINED3DFMT_R10G10B10A2_SNORM,  0xb0000000, 0x000003ff, 0x000ffc00, 0x3ff00000, 4,      0,      0,      FALSE},
    {WINED3DFMT_R8G8B8A8_UNORM,     0xff000000, 0x000000ff, 0x0000ff00, 0x00ff0000, 4,      0,      0,      FALSE},
    {WINED3DFMT_R8G8B8A8_UINT,      0xff000000, 0x000000ff, 0x0000ff00, 0x00ff0000, 4,      0,      0,      FALSE},
    {WINED3DFMT_X8B8G8R8,           0x0,        0x000000ff, 0x0000ff00, 0x00ff0000, 4,      0,      0,      FALSE},
    {WINED3DFMT_R16G16_UNORM,       0x0,        0x0000ffff, 0xffff0000, 0x0,        4,      0,      0,      FALSE},
    {WINED3DFMT_A2R10G10B10,        0xb0000000, 0x3ff00000, 0x000ffc00, 0x000003ff, 4,      0,      0,      FALSE},
    {WINED3DFMT_R16G16B16A16_UNORM, 0x1,        0x0000ffff, 0xffff0000, 0x0,        8,      0,      0,      FALSE},
    /* Luminance */
    {WINED3DFMT_L8,                 0x0,        0x0,        0x0,        0x0,        1,      0,      0,      FALSE},
    {WINED3DFMT_A8L8,               0x0000ff00, 0x0,        0x0,        0x0,        2,      0,      0,      FALSE},
    {WINED3DFMT_A4L4,               0x000000f0, 0x0,        0x0,        0x0,        1,      0,      0,      FALSE},
    /* Bump mapping stuff */
    {WINED3DFMT_R8G8_SNORM,         0x0,        0x0,        0x0,        0x0,        2,      0,      0,      FALSE},
    {WINED3DFMT_L6V5U5,             0x0,        0x0,        0x0,        0x0,        2,      0,      0,      FALSE},
    {WINED3DFMT_X8L8V8U8,           0x0,        0x0,        0x0,        0x0,        4,      0,      0,      FALSE},
    {WINED3DFMT_R8G8B8A8_SNORM,     0x0,        0x0,        0x0,        0x0,        4,      0,      0,      FALSE},
    {WINED3DFMT_R16G16_SNORM,       0x0,        0x0,        0x0,        0x0,        4,      0,      0,      FALSE},
    {WINED3DFMT_W11V11U10,          0x0,        0x0,        0x0,        0x0,        4,      0,      0,      FALSE},
    {WINED3DFMT_A2W10V10U10,        0xb0000000, 0x0,        0x0,        0x0,        4,      0,      0,      FALSE},
    /* Depth stencil formats */
    {WINED3DFMT_D16_LOCKABLE,       0x0,        0x0,        0x0,        0x0,        2,      16,     0,      FALSE},
    {WINED3DFMT_D32,                0x0,        0x0,        0x0,        0x0,        4,      32,     0,      FALSE},
    {WINED3DFMT_D15S1,              0x0,        0x0,        0x0,        0x0,        2,      15,     1,      FALSE},
    {WINED3DFMT_D24S8,              0x0,        0x0,        0x0,        0x0,        4,      24,     8,      FALSE},
    {WINED3DFMT_D24X8,              0x0,        0x0,        0x0,        0x0,        4,      24,     0,      FALSE},
    {WINED3DFMT_D24X4S4,            0x0,        0x0,        0x0,        0x0,        4,      24,     4,      FALSE},
    {WINED3DFMT_D16_UNORM,          0x0,        0x0,        0x0,        0x0,        2,      16,     0,      FALSE},
    {WINED3DFMT_L16,                0x0,        0x0,        0x0,        0x0,        2,      16,     0,      FALSE},
    {WINED3DFMT_D32F_LOCKABLE,      0x0,        0x0,        0x0,        0x0,        4,      32,     0,      FALSE},
    {WINED3DFMT_D24FS8,             0x0,        0x0,        0x0,        0x0,        4,      24,     8,      FALSE},
    /* Is this a vertex buffer? */
    {WINED3DFMT_VERTEXDATA,         0x0,        0x0,        0x0,        0x0,        0,      0,      0,      FALSE},
    {WINED3DFMT_R16_UINT,           0x0,        0x0,        0x0,        0x0,        2,      0,      0,      FALSE},
    {WINED3DFMT_R32_UINT,           0x0,        0x0,        0x0,        0x0,        4,      0,      0,      FALSE},
    {WINED3DFMT_R16G16B16A16_SNORM, 0x0,        0x0,        0x0,        0x0,        8,      0,      0,      FALSE},
    /* Vendor-specific formats */
    {WINED3DFMT_ATI2N,              0x0,        0x0,        0x0,        0x0,        1,      0,      0,      TRUE },
    {WINED3DFMT_NVHU,               0x0,        0x0,        0x0,        0x0,        2,      0,      0,      TRUE },
    {WINED3DFMT_NVHS,               0x0,        0x0,        0x0,        0x0,        2,      0,      0,      TRUE },
};

struct wined3d_format_vertex_info
{
    WINED3DFORMAT format;
    enum wined3d_ffp_emit_idx emit_idx;
    GLint component_count;
    GLenum gl_vtx_type;
    GLint gl_vtx_format;
    GLboolean gl_normalized;
    unsigned int component_size;
};

static const struct wined3d_format_vertex_info format_vertex_info[] =
{
    {WINED3DFMT_R32_FLOAT,          WINED3D_FFP_EMIT_FLOAT1,    1, GL_FLOAT,          1, GL_FALSE, sizeof(float)},
    {WINED3DFMT_R32G32_FLOAT,       WINED3D_FFP_EMIT_FLOAT2,    2, GL_FLOAT,          2, GL_FALSE, sizeof(float)},
    {WINED3DFMT_R32G32B32_FLOAT,    WINED3D_FFP_EMIT_FLOAT3,    3, GL_FLOAT,          3, GL_FALSE, sizeof(float)},
    {WINED3DFMT_R32G32B32A32_FLOAT, WINED3D_FFP_EMIT_FLOAT4,    4, GL_FLOAT,          4, GL_FALSE, sizeof(float)},
    {WINED3DFMT_A8R8G8B8,           WINED3D_FFP_EMIT_D3DCOLOR,  4, GL_UNSIGNED_BYTE,  4, GL_TRUE,  sizeof(BYTE)},
    {WINED3DFMT_R8G8B8A8_UINT,      WINED3D_FFP_EMIT_UBYTE4,    4, GL_UNSIGNED_BYTE,  4, GL_FALSE, sizeof(BYTE)},
    {WINED3DFMT_R16G16_SINT,        WINED3D_FFP_EMIT_SHORT2,    2, GL_SHORT,          2, GL_FALSE, sizeof(short int)},
    {WINED3DFMT_R16G16B16A16_SINT,  WINED3D_FFP_EMIT_SHORT4,    4, GL_SHORT,          4, GL_FALSE, sizeof(short int)},
    {WINED3DFMT_R8G8B8A8_UNORM,     WINED3D_FFP_EMIT_UBYTE4N,   4, GL_UNSIGNED_BYTE,  4, GL_TRUE,  sizeof(BYTE)},
    {WINED3DFMT_R16G16_SNORM,       WINED3D_FFP_EMIT_SHORT2N,   2, GL_SHORT,          2, GL_TRUE,  sizeof(short int)},
    {WINED3DFMT_R16G16B16A16_SNORM, WINED3D_FFP_EMIT_SHORT4N,   4, GL_SHORT,          4, GL_TRUE,  sizeof(short int)},
    {WINED3DFMT_R16G16_UNORM,       WINED3D_FFP_EMIT_USHORT2N,  2, GL_UNSIGNED_SHORT, 2, GL_TRUE,  sizeof(short int)},
    {WINED3DFMT_R16G16B16A16_UNORM, WINED3D_FFP_EMIT_USHORT4N,  4, GL_UNSIGNED_SHORT, 4, GL_TRUE,  sizeof(short int)},
    {WINED3DFMT_R10G10B10A2_UINT,   WINED3D_FFP_EMIT_UDEC3,     3, GL_UNSIGNED_SHORT, 3, GL_FALSE, sizeof(short int)},
    {WINED3DFMT_R10G10B10A2_SNORM,  WINED3D_FFP_EMIT_DEC3N,     3, GL_SHORT,          3, GL_TRUE,  sizeof(short int)},
    {WINED3DFMT_R16G16_FLOAT,       WINED3D_FFP_EMIT_FLOAT16_2, 2, GL_FLOAT,          2, GL_FALSE, sizeof(GLhalfNV)},
    {WINED3DFMT_R16G16B16A16_FLOAT, WINED3D_FFP_EMIT_FLOAT16_4, 4, GL_FLOAT,          4, GL_FALSE, sizeof(GLhalfNV)}
};

typedef struct {
    WINED3DFORMAT           fmt;
    GLint                   glInternal, glGammaInternal, rtInternal, glFormat, glType;
    unsigned int            Flags;
} GlPixelFormatDescTemplate;

/*****************************************************************************
 * OpenGL format template. Contains unexciting formats which do not need
 * extension checks. The order in this table is independent of the order in
 * the table StaticPixelFormatDesc above. Not all formats have to be in this
 * table.
 */
static const GlPixelFormatDescTemplate gl_formats_template[] = {
    /* WINED3DFORMAT                internal                          srgbInternal                            rtInternal
            format                  type
            flags */
    {WINED3DFMT_UNKNOWN,            0,                                0,                                      0,
            0,                      0,
            0},
    /* FourCC formats */
    /* GL_APPLE_ycbcr_422 claims that its '2YUV' format, which is supported via the UNSIGNED_SHORT_8_8_REV_APPLE type
     * is equivalent to 'UYVY' format on Windows, and the 'YUVS' via UNSIGNED_SHORT_8_8_APPLE equates to 'YUY2'. The
     * d3d9 test however shows that the opposite is true. Since the extension is from 2002, it predates the x86 based
     * Macs, so probably the endianess differs. This could be tested as soon as we have a Windows and MacOS on a big
     * endian machine
     */
    {WINED3DFMT_UYVY,               GL_RGB,                           GL_RGB,                                 0,
            GL_YCBCR_422_APPLE,     UNSIGNED_SHORT_8_8_APPLE,
            WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_YUY2,               GL_RGB,                           GL_RGB,                                 0,
            GL_YCBCR_422_APPLE, UNSIGNED_SHORT_8_8_REV_APPLE,
            WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_YV12,               GL_ALPHA,                         GL_ALPHA,                               0,
            GL_ALPHA,               GL_UNSIGNED_BYTE,
            WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_DXT1,               GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, 0,
            GL_RGBA,                GL_UNSIGNED_BYTE,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_DXT2,               GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, 0,
            GL_RGBA,                GL_UNSIGNED_BYTE,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_DXT3,               GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, 0,
            GL_RGBA,                GL_UNSIGNED_BYTE,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_DXT4,               GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, 0,
            GL_RGBA,                GL_UNSIGNED_BYTE,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_DXT5,               GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, 0,
            GL_RGBA,                GL_UNSIGNED_BYTE,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_MULTI2_ARGB8,       0,                                0,                                      0,
            0,                      0,
            0},
    {WINED3DFMT_G8R8_G8B8,          0,                                0,                                      0,
            0,                      0,
            0},
    {WINED3DFMT_R8G8_B8G8,          0,                                0,                                      0,
            0,                      0,
            0},
    /* IEEE formats */
    {WINED3DFMT_R32_FLOAT,          GL_RGB32F_ARB,                    GL_RGB32F_ARB,                          0,
            GL_RED,                 GL_FLOAT,
            WINED3DFMT_FLAG_RENDERTARGET},
    {WINED3DFMT_R32G32_FLOAT,       GL_RG32F,                         GL_RG32F,                               0,
            GL_RG,                  GL_FLOAT,
            WINED3DFMT_FLAG_RENDERTARGET},
    {WINED3DFMT_R32G32B32A32_FLOAT, GL_RGBA32F_ARB,                   GL_RGBA32F_ARB,                         0,
            GL_RGBA,                GL_FLOAT,
            WINED3DFMT_FLAG_RENDERTARGET},
    /* Hmm? */
    {WINED3DFMT_CxV8U8,             0,                                0,                                      0,
            0,                      0,
            0},
    /* Float */
    {WINED3DFMT_R16_FLOAT,          GL_RGB16F_ARB,                    GL_RGB16F_ARB,                          0,
            GL_RED,             GL_HALF_FLOAT_ARB,
            WINED3DFMT_FLAG_FILTERING | WINED3DFMT_FLAG_RENDERTARGET},
    {WINED3DFMT_R16G16_FLOAT,       GL_RG16F,                         GL_RG16F,                               0,
            GL_RG,              GL_HALF_FLOAT_ARB,
            WINED3DFMT_FLAG_FILTERING | WINED3DFMT_FLAG_RENDERTARGET},
    {WINED3DFMT_R16G16B16A16_FLOAT, GL_RGBA16F_ARB,                   GL_RGBA16F_ARB,                         0,
            GL_RGBA,            GL_HALF_FLOAT_ARB,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING | WINED3DFMT_FLAG_RENDERTARGET},
    /* Palettized formats */
    {WINED3DFMT_A8P8,               0,                                0,                                      0,
            0,                      0,
            0},
    {WINED3DFMT_P8,                 GL_COLOR_INDEX8_EXT,              GL_COLOR_INDEX8_EXT,                    0,
            GL_COLOR_INDEX,         GL_UNSIGNED_BYTE,
            0},
    /* Standard ARGB formats */
    {WINED3DFMT_R8G8B8,             GL_RGB8,                          GL_RGB8,                                0,
            GL_BGR,                 GL_UNSIGNED_BYTE,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING | WINED3DFMT_FLAG_RENDERTARGET},
    {WINED3DFMT_A8R8G8B8,           GL_RGBA8,                         GL_SRGB8_ALPHA8_EXT,                    0,
            GL_BGRA,                GL_UNSIGNED_INT_8_8_8_8_REV,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING | WINED3DFMT_FLAG_RENDERTARGET},
    {WINED3DFMT_X8R8G8B8,           GL_RGB8,                          GL_SRGB8_EXT,                           0,
            GL_BGRA,                GL_UNSIGNED_INT_8_8_8_8_REV,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING | WINED3DFMT_FLAG_RENDERTARGET},
    {WINED3DFMT_R5G6B5,             GL_RGB5,                          GL_RGB5,                                GL_RGB8,
            GL_RGB,                 GL_UNSIGNED_SHORT_5_6_5,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING | WINED3DFMT_FLAG_RENDERTARGET},
    {WINED3DFMT_X1R5G5B5,           GL_RGB5,                          GL_RGB5_A1,                             0,
            GL_BGRA,                GL_UNSIGNED_SHORT_1_5_5_5_REV,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_A1R5G5B5,           GL_RGB5_A1,                       GL_RGB5_A1,                             0,
            GL_BGRA,                GL_UNSIGNED_SHORT_1_5_5_5_REV,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_A4R4G4B4,           GL_RGBA4,                         GL_SRGB8_ALPHA8_EXT,                    0,
            GL_BGRA,                GL_UNSIGNED_SHORT_4_4_4_4_REV,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_R3G3B2,             GL_R3_G3_B2,                      GL_R3_G3_B2,                            0,
            GL_RGB,                 GL_UNSIGNED_BYTE_3_3_2,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING},
    {WINED3DFMT_A8_UNORM,           GL_ALPHA8,                        GL_ALPHA8,                              0,
            GL_ALPHA,               GL_UNSIGNED_BYTE,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING},
    {WINED3DFMT_A8R3G3B2,           0,                                0,                                      0,
            0,                      0,
            0},
    {WINED3DFMT_X4R4G4B4,           GL_RGB4,                          GL_RGB4,                                0,
            GL_BGRA,                GL_UNSIGNED_SHORT_4_4_4_4_REV,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_R10G10B10A2_UNORM,  GL_RGB10_A2,                      GL_RGB10_A2,                            0,
            GL_RGBA,                GL_UNSIGNED_INT_2_10_10_10_REV,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_R8G8B8A8_UNORM,     GL_RGBA8,                         GL_RGBA8,                               0,
            GL_RGBA,                GL_UNSIGNED_INT_8_8_8_8_REV,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_X8B8G8R8,           GL_RGB8,                          GL_RGB8,                                0,
            GL_RGBA,                GL_UNSIGNED_INT_8_8_8_8_REV,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_R16G16_UNORM,       GL_RGB16_EXT,                     GL_RGB16_EXT,                           0,
            GL_RGB,                 GL_UNSIGNED_SHORT,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_A2R10G10B10,        GL_RGB10_A2,                      GL_RGB10_A2,                            0,
            GL_BGRA,                GL_UNSIGNED_INT_2_10_10_10_REV,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_R16G16B16A16_UNORM, GL_RGBA16_EXT,                    GL_RGBA16_EXT,                          0,
            GL_RGBA,                GL_UNSIGNED_SHORT,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING | WINED3DFMT_FLAG_RENDERTARGET},
    /* Luminance */
    {WINED3DFMT_L8,                 GL_LUMINANCE8,                    GL_SLUMINANCE8_EXT,                     0,
            GL_LUMINANCE,           GL_UNSIGNED_BYTE,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_A8L8,               GL_LUMINANCE8_ALPHA8,             GL_SLUMINANCE8_ALPHA8_EXT,              0,
            GL_LUMINANCE_ALPHA,     GL_UNSIGNED_BYTE,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_A4L4,               GL_LUMINANCE4_ALPHA4,             GL_LUMINANCE4_ALPHA4,                   0,
            GL_LUMINANCE_ALPHA,     GL_UNSIGNED_BYTE,
            0},
    /* Bump mapping stuff */
    {WINED3DFMT_R8G8_SNORM,         GL_DSDT8_NV,                      GL_DSDT8_NV,                            0,
            GL_DSDT_NV,             GL_BYTE,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_L6V5U5,             GL_DSDT8_MAG8_NV,                 GL_DSDT8_MAG8_NV,                       0,
            GL_DSDT_MAG_NV,         GL_BYTE,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_X8L8V8U8,           GL_DSDT8_MAG8_INTENSITY8_NV,      GL_DSDT8_MAG8_INTENSITY8_NV,            0,
            GL_DSDT_MAG_VIB_NV,     GL_UNSIGNED_INT_8_8_S8_S8_REV_NV,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_R8G8B8A8_SNORM,     GL_SIGNED_RGBA8_NV,               GL_SIGNED_RGBA8_NV,                     0,
            GL_RGBA,                GL_BYTE,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_R16G16_SNORM,       GL_SIGNED_HILO16_NV,              GL_SIGNED_HILO16_NV,                    0,
            GL_HILO_NV,             GL_SHORT,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_W11V11U10,          0,                                0,                                      0,
            0,                      0,
            0},
    {WINED3DFMT_A2W10V10U10,        0,                                0,                                      0,
            0,                      0,
            0},
    /* Depth stencil formats */
    {WINED3DFMT_D16_LOCKABLE,       GL_DEPTH_COMPONENT24_ARB,         GL_DEPTH_COMPONENT24_ARB,               0,
            GL_DEPTH_COMPONENT,     GL_UNSIGNED_SHORT,
            WINED3DFMT_FLAG_DEPTH},
    {WINED3DFMT_D32,                GL_DEPTH_COMPONENT32_ARB,         GL_DEPTH_COMPONENT32_ARB,               0,
            GL_DEPTH_COMPONENT,     GL_UNSIGNED_INT,
            WINED3DFMT_FLAG_DEPTH},
    {WINED3DFMT_D15S1,              GL_DEPTH_COMPONENT24_ARB,         GL_DEPTH_COMPONENT24_ARB,               0,
            GL_DEPTH_COMPONENT,     GL_UNSIGNED_SHORT,
            WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL},
    {WINED3DFMT_D24S8,              GL_DEPTH_COMPONENT24_ARB,         GL_DEPTH_COMPONENT24_ARB,               0,
            GL_DEPTH_COMPONENT,     GL_UNSIGNED_INT,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING | WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL},
    {WINED3DFMT_D24X8,              GL_DEPTH_COMPONENT24_ARB,         GL_DEPTH_COMPONENT24_ARB,               0,
            GL_DEPTH_COMPONENT,     GL_UNSIGNED_INT,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING | WINED3DFMT_FLAG_DEPTH},
    {WINED3DFMT_D24X4S4,            GL_DEPTH_COMPONENT24_ARB,         GL_DEPTH_COMPONENT24_ARB,               0,
            GL_DEPTH_COMPONENT,     GL_UNSIGNED_INT,
            WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL},
    {WINED3DFMT_D16_UNORM,          GL_DEPTH_COMPONENT24_ARB,         GL_DEPTH_COMPONENT24_ARB,               0,
            GL_DEPTH_COMPONENT,     GL_UNSIGNED_SHORT,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING | WINED3DFMT_FLAG_DEPTH},
    {WINED3DFMT_L16,                GL_LUMINANCE16_EXT,               GL_LUMINANCE16_EXT,                     0,
            GL_LUMINANCE,           GL_UNSIGNED_SHORT,
            WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING | WINED3DFMT_FLAG_FILTERING},
    {WINED3DFMT_D32F_LOCKABLE,      GL_DEPTH_COMPONENT32_ARB,         GL_DEPTH_COMPONENT32_ARB,               0,
            GL_DEPTH_COMPONENT,     GL_FLOAT,
            WINED3DFMT_FLAG_DEPTH},
    {WINED3DFMT_D24FS8,             GL_DEPTH_COMPONENT24_ARB,         GL_DEPTH_COMPONENT24_ARB,               0,
            GL_DEPTH_COMPONENT,     GL_FLOAT,
            WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL},
    /* Is this a vertex buffer? */
    {WINED3DFMT_VERTEXDATA,         0,                                0,                                      0,
            0,                      0,
            0},
    {WINED3DFMT_R16_UINT,           0,                                0,                                      0,
            0,                      0,
            0},
    {WINED3DFMT_R32_UINT,           0,                                0,                                      0,
            0,                      0,
            0},
    {WINED3DFMT_R16G16B16A16_SNORM, GL_COLOR_INDEX,                   GL_COLOR_INDEX,                         0,
            GL_COLOR_INDEX,         GL_UNSIGNED_SHORT,
            0},
    /* Vendor-specific formats */
    {WINED3DFMT_ATI2N,              0,                                0,                                      0,
            GL_LUMINANCE_ALPHA,     GL_UNSIGNED_BYTE,
            0},
    {WINED3DFMT_NVHU,               0,                                0,                                      0,
            GL_LUMINANCE_ALPHA,     GL_UNSIGNED_BYTE,
            0},
    {WINED3DFMT_NVHS,               0,                                0,                                      0,
            GL_LUMINANCE_ALPHA,     GL_UNSIGNED_BYTE,
            0}
};

static inline int getFmtIdx(WINED3DFORMAT fmt) {
    /* First check if the format is at the position of its value.
     * This will catch the argb formats before the loop is entered
     */
    if(fmt < (sizeof(formats) / sizeof(formats[0])) && formats[fmt].format == fmt) {
        return fmt;
    } else {
        unsigned int i;
        for(i = 0; i < (sizeof(formats) / sizeof(formats[0])); i++) {
            if(formats[i].format == fmt) {
                return i;
            }
        }
    }
    return -1;
}

static BOOL init_format_base_info(WineD3D_GL_Info *gl_info)
{
    UINT format_count = sizeof(formats) / sizeof(*formats);
    UINT i;

    gl_info->gl_formats = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, format_count * sizeof(*gl_info->gl_formats));
    if (!gl_info->gl_formats)
    {
        ERR("Failed to allocate memory.\n");
        return FALSE;
    }

    for (i = 0; i < format_count; ++i)
    {
        struct GlPixelFormatDesc *desc = &gl_info->gl_formats[i];
        desc->format = formats[i].format;
        desc->red_mask = formats[i].redMask;
        desc->green_mask = formats[i].greenMask;
        desc->blue_mask = formats[i].blueMask;
        desc->alpha_mask = formats[i].alphaMask;
        desc->byte_count = formats[i].bpp;
        desc->depth_size = formats[i].depthSize;
        desc->stencil_size = formats[i].stencilSize;
        if (formats[i].isFourcc) desc->Flags |= WINED3DFMT_FLAG_FOURCC;
    }

    return TRUE;
}

#define GLINFO_LOCATION (*gl_info)

static BOOL check_fbo_compat(const WineD3D_GL_Info *gl_info, GLint internal_format)
{
    GLuint tex, fb;
    GLenum status;

    while(glGetError());
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    GL_EXTCALL(glGenFramebuffersEXT(1, &fb));
    GL_EXTCALL(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb));
    GL_EXTCALL(glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, tex, 0));

    status = GL_EXTCALL(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT));
    GL_EXTCALL(glDeleteFramebuffersEXT(1, &fb));
    glDeleteTextures(1, &tex);

    checkGLcall("Framebuffer format check");

    return status == GL_FRAMEBUFFER_COMPLETE_EXT;
}

static BOOL init_format_texture_info(WineD3D_GL_Info *gl_info)
{
    unsigned int i;

    for (i = 0; i < sizeof(gl_formats_template) / sizeof(gl_formats_template[0]); ++i)
    {
        int fmt_idx = getFmtIdx(gl_formats_template[i].fmt);
        struct GlPixelFormatDesc *desc;

        if (fmt_idx == -1)
        {
            ERR("Format %s (%#x) not found.\n",
                    debug_d3dformat(gl_formats_template[i].fmt), gl_formats_template[i].fmt);
            return FALSE;
        }

        desc = &gl_info->gl_formats[fmt_idx];
        desc->glInternal = gl_formats_template[i].glInternal;
        desc->glGammaInternal = gl_formats_template[i].glGammaInternal;
        desc->glFormat = gl_formats_template[i].glFormat;
        desc->glType = gl_formats_template[i].glType;
        desc->color_fixup = COLOR_FIXUP_IDENTITY;
        desc->Flags |= gl_formats_template[i].Flags;
        desc->heightscale = 1.0;

        if (wined3d_settings.offscreen_rendering_mode == ORM_FBO && gl_formats_template[i].rtInternal)
        {
            /* Check if the default internal format is supported as a frame buffer target, otherwise
             * fall back to the render target internal.
             *
             * Try to stick to the standard format if possible, this limits precision differences */
            if (!check_fbo_compat(gl_info, gl_formats_template[i].glInternal))
            {
                TRACE("Internal format of %s not supported as FBO target, using render target internal instead\n",
                        debug_d3dformat(gl_formats_template[i].fmt));
                desc->rtInternal = gl_formats_template[i].rtInternal;
            }
            else
            {
                TRACE("Format %s is supported as fbo target\n", debug_d3dformat(gl_formats_template[i].fmt));
                desc->rtInternal = gl_formats_template[i].glInternal;
            }
        }
        else
        {
            desc->rtInternal = gl_formats_template[i].glInternal;
        }
    }

    return TRUE;
}

static void apply_format_fixups(WineD3D_GL_Info *gl_info)
{
    int idx;

    idx = getFmtIdx(WINED3DFMT_R16_FLOAT);
    gl_info->gl_formats[idx].color_fixup = create_color_fixup_desc(
            0, CHANNEL_SOURCE_X, 0, CHANNEL_SOURCE_ONE, 0, CHANNEL_SOURCE_ONE, 0, CHANNEL_SOURCE_W);
    /* When ARB_texture_rg is supported we only require 16-bit for R16F instead of 64-bit RGBA16F */
    if (GL_SUPPORT(ARB_TEXTURE_RG))
    {
        gl_info->gl_formats[idx].glInternal = GL_R16F;
        gl_info->gl_formats[idx].glGammaInternal = GL_R16F;
    }

    idx = getFmtIdx(WINED3DFMT_R32_FLOAT);
    gl_info->gl_formats[idx].color_fixup = create_color_fixup_desc(
            0, CHANNEL_SOURCE_X, 0, CHANNEL_SOURCE_ONE, 0, CHANNEL_SOURCE_ONE, 0, CHANNEL_SOURCE_W);
    /* When ARB_texture_rg is supported we only require 32-bit for R32F instead of 128-bit RGBA32F */
    if (GL_SUPPORT(ARB_TEXTURE_RG))
    {
        gl_info->gl_formats[idx].glInternal = GL_R32F;
        gl_info->gl_formats[idx].glGammaInternal = GL_R32F;
    }

    idx = getFmtIdx(WINED3DFMT_R16G16_UNORM);
    gl_info->gl_formats[idx].color_fixup = create_color_fixup_desc(
            0, CHANNEL_SOURCE_X, 0, CHANNEL_SOURCE_Y, 0, CHANNEL_SOURCE_ONE, 0, CHANNEL_SOURCE_W);

    idx = getFmtIdx(WINED3DFMT_R16G16_FLOAT);
    gl_info->gl_formats[idx].color_fixup = create_color_fixup_desc(
            0, CHANNEL_SOURCE_X, 0, CHANNEL_SOURCE_Y, 0, CHANNEL_SOURCE_ONE, 0, CHANNEL_SOURCE_W);

    idx = getFmtIdx(WINED3DFMT_R32G32_FLOAT);
    gl_info->gl_formats[idx].color_fixup = create_color_fixup_desc(
            0, CHANNEL_SOURCE_X, 0, CHANNEL_SOURCE_Y, 0, CHANNEL_SOURCE_ONE, 0, CHANNEL_SOURCE_W);

    /* V8U8 is supported natively by GL_ATI_envmap_bumpmap and GL_NV_texture_shader.
     * V16U16 is only supported by GL_NV_texture_shader. The formats need fixup if
     * their extensions are not available. GL_ATI_envmap_bumpmap is not used because
     * the only driver that implements it(fglrx) has a buggy implementation.
     *
     * V8U8 and V16U16 need a fixup of the undefined blue channel. OpenGL
     * returns 0.0 when sampling from it, DirectX 1.0. So we always have in-shader
     * conversion for this format.
     */
    if (!GL_SUPPORT(NV_TEXTURE_SHADER))
    {
        idx = getFmtIdx(WINED3DFMT_R8G8_SNORM);
        gl_info->gl_formats[idx].color_fixup = create_color_fixup_desc(
                1, CHANNEL_SOURCE_X, 1, CHANNEL_SOURCE_Y, 0, CHANNEL_SOURCE_ONE, 0, CHANNEL_SOURCE_ONE);
        idx = getFmtIdx(WINED3DFMT_R16G16_SNORM);
        gl_info->gl_formats[idx].color_fixup = create_color_fixup_desc(
                1, CHANNEL_SOURCE_X, 1, CHANNEL_SOURCE_Y, 0, CHANNEL_SOURCE_ONE, 0, CHANNEL_SOURCE_ONE);
    }
    else
    {
        idx = getFmtIdx(WINED3DFMT_R8G8_SNORM);
        gl_info->gl_formats[idx].color_fixup = create_color_fixup_desc(
                0, CHANNEL_SOURCE_X, 0, CHANNEL_SOURCE_Y, 0, CHANNEL_SOURCE_ONE, 0, CHANNEL_SOURCE_ONE);
        idx = getFmtIdx(WINED3DFMT_R16G16_SNORM);
        gl_info->gl_formats[idx].color_fixup = create_color_fixup_desc(
                0, CHANNEL_SOURCE_X, 0, CHANNEL_SOURCE_Y, 0, CHANNEL_SOURCE_ONE, 0, CHANNEL_SOURCE_ONE);
    }

    if (!GL_SUPPORT(NV_TEXTURE_SHADER))
    {
        /* If GL_NV_texture_shader is not supported, those formats are converted, incompatibly
         * with each other
         */
        idx = getFmtIdx(WINED3DFMT_L6V5U5);
        gl_info->gl_formats[idx].color_fixup = create_color_fixup_desc(
                1, CHANNEL_SOURCE_X, 1, CHANNEL_SOURCE_Z, 0, CHANNEL_SOURCE_Y, 0, CHANNEL_SOURCE_ONE);
        idx = getFmtIdx(WINED3DFMT_X8L8V8U8);
        gl_info->gl_formats[idx].color_fixup = create_color_fixup_desc(
                1, CHANNEL_SOURCE_X, 1, CHANNEL_SOURCE_Y, 0, CHANNEL_SOURCE_Z, 0, CHANNEL_SOURCE_W);
        idx = getFmtIdx(WINED3DFMT_R8G8B8A8_SNORM);
        gl_info->gl_formats[idx].color_fixup = create_color_fixup_desc(
                1, CHANNEL_SOURCE_X, 1, CHANNEL_SOURCE_Y, 1, CHANNEL_SOURCE_Z, 1, CHANNEL_SOURCE_W);
    }
    else
    {
        /* If GL_NV_texture_shader is supported, WINED3DFMT_L6V5U5 and WINED3DFMT_X8L8V8U8
         * are converted at surface loading time, but they do not need any modification in
         * the shader, thus they are compatible with all WINED3DFMT_UNKNOWN group formats.
         * WINED3DFMT_Q8W8V8U8 doesn't even need load-time conversion
         */
    }

    if (GL_SUPPORT(EXT_TEXTURE_COMPRESSION_RGTC))
    {
        idx = getFmtIdx(WINED3DFMT_ATI2N);
        gl_info->gl_formats[idx].glInternal = GL_COMPRESSED_RED_GREEN_RGTC2_EXT;
        gl_info->gl_formats[idx].glGammaInternal = GL_COMPRESSED_RED_GREEN_RGTC2_EXT;
        gl_info->gl_formats[idx].color_fixup = create_color_fixup_desc(
                0, CHANNEL_SOURCE_Y, 0, CHANNEL_SOURCE_X, 0, CHANNEL_SOURCE_ONE, 0, CHANNEL_SOURCE_ONE);
    }
    else if (GL_SUPPORT(ATI_TEXTURE_COMPRESSION_3DC))
    {
        idx = getFmtIdx(WINED3DFMT_ATI2N);
        gl_info->gl_formats[idx].glInternal = GL_COMPRESSED_LUMINANCE_ALPHA_3DC_ATI;
        gl_info->gl_formats[idx].glGammaInternal = GL_COMPRESSED_LUMINANCE_ALPHA_3DC_ATI;
        gl_info->gl_formats[idx].color_fixup= create_color_fixup_desc(
                0, CHANNEL_SOURCE_X, 0, CHANNEL_SOURCE_W, 0, CHANNEL_SOURCE_ONE, 0, CHANNEL_SOURCE_ONE);
    }

    if (!GL_SUPPORT(APPLE_YCBCR_422))
    {
        idx = getFmtIdx(WINED3DFMT_YUY2);
        gl_info->gl_formats[idx].glInternal = GL_LUMINANCE_ALPHA;
        gl_info->gl_formats[idx].glGammaInternal = GL_LUMINANCE_ALPHA; /* not srgb */
        gl_info->gl_formats[idx].glFormat = GL_LUMINANCE_ALPHA;
        gl_info->gl_formats[idx].glType = GL_UNSIGNED_BYTE;
        gl_info->gl_formats[idx].color_fixup = create_yuv_fixup_desc(YUV_FIXUP_YUY2);

        idx = getFmtIdx(WINED3DFMT_UYVY);
        gl_info->gl_formats[idx].glInternal = GL_LUMINANCE_ALPHA;
        gl_info->gl_formats[idx].glGammaInternal = GL_LUMINANCE_ALPHA; /* not srgb */
        gl_info->gl_formats[idx].glFormat = GL_LUMINANCE_ALPHA;
        gl_info->gl_formats[idx].glType = GL_UNSIGNED_BYTE;
        gl_info->gl_formats[idx].color_fixup = create_yuv_fixup_desc(YUV_FIXUP_UYVY);
    }

    idx = getFmtIdx(WINED3DFMT_YV12);
    gl_info->gl_formats[idx].heightscale = 1.5;
    gl_info->gl_formats[idx].color_fixup = create_yuv_fixup_desc(YUV_FIXUP_YV12);

    if (GL_SUPPORT(EXT_VERTEX_ARRAY_BGRA))
    {
        idx = getFmtIdx(WINED3DFMT_A8R8G8B8);
        gl_info->gl_formats[idx].gl_vtx_format = GL_BGRA;
    }

    if (GL_SUPPORT(NV_HALF_FLOAT))
    {
        /* Do not change the size of the type, it is CPU side. We have to change the GPU-side information though.
         * It is the job of the vertex buffer code to make sure that the vbos have the right format */
        idx = getFmtIdx(WINED3DFMT_R16G16_FLOAT);
        gl_info->gl_formats[idx].gl_vtx_type = GL_HALF_FLOAT_NV;

        idx = getFmtIdx(WINED3DFMT_R16G16B16A16_FLOAT);
        gl_info->gl_formats[idx].gl_vtx_type = GL_HALF_FLOAT_NV;
    }
}

static BOOL init_format_vertex_info(WineD3D_GL_Info *gl_info)
{
    unsigned int i;

    for (i = 0; i < (sizeof(format_vertex_info) / sizeof(*format_vertex_info)); ++i)
    {
        struct GlPixelFormatDesc *format_desc;
        int fmt_idx = getFmtIdx(format_vertex_info[i].format);

        if (fmt_idx == -1)
        {
            ERR("Format %s (%#x) not found.\n",
                    debug_d3dformat(format_vertex_info[i].format), format_vertex_info[i].format);
            return FALSE;
        }

        format_desc = &gl_info->gl_formats[fmt_idx];
        format_desc->emit_idx = format_vertex_info[i].emit_idx;
        format_desc->component_count = format_vertex_info[i].component_count;
        format_desc->gl_vtx_type = format_vertex_info[i].gl_vtx_type;
        format_desc->gl_vtx_format = format_vertex_info[i].gl_vtx_format;
        format_desc->gl_normalized = format_vertex_info[i].gl_normalized;
        format_desc->component_size = format_vertex_info[i].component_size;
    }

    return TRUE;
}

BOOL initPixelFormatsNoGL(WineD3D_GL_Info *gl_info)
{
    return init_format_base_info(gl_info);
}

BOOL initPixelFormats(WineD3D_GL_Info *gl_info)
{
    if (!init_format_base_info(gl_info)) return FALSE;

    if (!init_format_texture_info(gl_info))
    {
        HeapFree(GetProcessHeap(), 0, gl_info->gl_formats);
        return FALSE;
    }

    if (!init_format_vertex_info(gl_info))
    {
        HeapFree(GetProcessHeap(), 0, gl_info->gl_formats);
        return FALSE;
    }

    apply_format_fixups(gl_info);

    return TRUE;
}

#undef GLINFO_LOCATION

#define GLINFO_LOCATION This->adapter->gl_info

const struct GlPixelFormatDesc *getFormatDescEntry(WINED3DFORMAT fmt, const WineD3D_GL_Info *gl_info)
{
    int idx = getFmtIdx(fmt);

    if(idx == -1) {
        FIXME("Can't find format %s(%d) in the format lookup table\n", debug_d3dformat(fmt), fmt);
        /* Get the caller a valid pointer */
        idx = getFmtIdx(WINED3DFMT_UNKNOWN);
    }

    return &gl_info->gl_formats[idx];
}

/*****************************************************************************
 * Trace formatting of useful values
 */
const char* debug_d3dformat(WINED3DFORMAT fmt) {
  switch (fmt) {
#define FMT_TO_STR(fmt) case fmt: return #fmt
    FMT_TO_STR(WINED3DFMT_UNKNOWN);
    FMT_TO_STR(WINED3DFMT_R8G8B8);
    FMT_TO_STR(WINED3DFMT_A8R8G8B8);
    FMT_TO_STR(WINED3DFMT_X8R8G8B8);
    FMT_TO_STR(WINED3DFMT_R5G6B5);
    FMT_TO_STR(WINED3DFMT_X1R5G5B5);
    FMT_TO_STR(WINED3DFMT_A1R5G5B5);
    FMT_TO_STR(WINED3DFMT_A4R4G4B4);
    FMT_TO_STR(WINED3DFMT_R3G3B2);
    FMT_TO_STR(WINED3DFMT_A8R3G3B2);
    FMT_TO_STR(WINED3DFMT_X4R4G4B4);
    FMT_TO_STR(WINED3DFMT_X8B8G8R8);
    FMT_TO_STR(WINED3DFMT_A2R10G10B10);
    FMT_TO_STR(WINED3DFMT_A8P8);
    FMT_TO_STR(WINED3DFMT_P8);
    FMT_TO_STR(WINED3DFMT_L8);
    FMT_TO_STR(WINED3DFMT_A8L8);
    FMT_TO_STR(WINED3DFMT_A4L4);
    FMT_TO_STR(WINED3DFMT_L6V5U5);
    FMT_TO_STR(WINED3DFMT_X8L8V8U8);
    FMT_TO_STR(WINED3DFMT_W11V11U10);
    FMT_TO_STR(WINED3DFMT_A2W10V10U10);
    FMT_TO_STR(WINED3DFMT_UYVY);
    FMT_TO_STR(WINED3DFMT_YUY2);
    FMT_TO_STR(WINED3DFMT_YV12);
    FMT_TO_STR(WINED3DFMT_DXT1);
    FMT_TO_STR(WINED3DFMT_DXT2);
    FMT_TO_STR(WINED3DFMT_DXT3);
    FMT_TO_STR(WINED3DFMT_DXT4);
    FMT_TO_STR(WINED3DFMT_DXT5);
    FMT_TO_STR(WINED3DFMT_MULTI2_ARGB8);
    FMT_TO_STR(WINED3DFMT_G8R8_G8B8);
    FMT_TO_STR(WINED3DFMT_R8G8_B8G8);
    FMT_TO_STR(WINED3DFMT_D16_LOCKABLE);
    FMT_TO_STR(WINED3DFMT_D32);
    FMT_TO_STR(WINED3DFMT_D15S1);
    FMT_TO_STR(WINED3DFMT_D24S8);
    FMT_TO_STR(WINED3DFMT_D24X8);
    FMT_TO_STR(WINED3DFMT_D24X4S4);
    FMT_TO_STR(WINED3DFMT_L16);
    FMT_TO_STR(WINED3DFMT_D32F_LOCKABLE);
    FMT_TO_STR(WINED3DFMT_D24FS8);
    FMT_TO_STR(WINED3DFMT_VERTEXDATA);
    FMT_TO_STR(WINED3DFMT_CxV8U8);
    FMT_TO_STR(WINED3DFMT_ATI2N);
    FMT_TO_STR(WINED3DFMT_NVHU);
    FMT_TO_STR(WINED3DFMT_NVHS);
    FMT_TO_STR(WINED3DFMT_R32G32B32A32_TYPELESS);
    FMT_TO_STR(WINED3DFMT_R32G32B32A32_FLOAT);
    FMT_TO_STR(WINED3DFMT_R32G32B32A32_UINT);
    FMT_TO_STR(WINED3DFMT_R32G32B32A32_SINT);
    FMT_TO_STR(WINED3DFMT_R32G32B32_TYPELESS);
    FMT_TO_STR(WINED3DFMT_R32G32B32_FLOAT);
    FMT_TO_STR(WINED3DFMT_R32G32B32_UINT);
    FMT_TO_STR(WINED3DFMT_R32G32B32_SINT);
    FMT_TO_STR(WINED3DFMT_R16G16B16A16_TYPELESS);
    FMT_TO_STR(WINED3DFMT_R16G16B16A16_FLOAT);
    FMT_TO_STR(WINED3DFMT_R16G16B16A16_UNORM);
    FMT_TO_STR(WINED3DFMT_R16G16B16A16_UINT);
    FMT_TO_STR(WINED3DFMT_R16G16B16A16_SNORM);
    FMT_TO_STR(WINED3DFMT_R16G16B16A16_SINT);
    FMT_TO_STR(WINED3DFMT_R32G32_TYPELESS);
    FMT_TO_STR(WINED3DFMT_R32G32_FLOAT);
    FMT_TO_STR(WINED3DFMT_R32G32_UINT);
    FMT_TO_STR(WINED3DFMT_R32G32_SINT);
    FMT_TO_STR(WINED3DFMT_R32G8X24_TYPELESS);
    FMT_TO_STR(WINED3DFMT_D32_FLOAT_S8X24_UINT);
    FMT_TO_STR(WINED3DFMT_R32_FLOAT_X8X24_TYPELESS);
    FMT_TO_STR(WINED3DFMT_X32_TYPELESS_G8X24_UINT);
    FMT_TO_STR(WINED3DFMT_R10G10B10A2_TYPELESS);
    FMT_TO_STR(WINED3DFMT_R10G10B10A2_UNORM);
    FMT_TO_STR(WINED3DFMT_R10G10B10A2_UINT);
    FMT_TO_STR(WINED3DFMT_R10G10B10A2_SNORM);
    FMT_TO_STR(WINED3DFMT_R11G11B10_FLOAT);
    FMT_TO_STR(WINED3DFMT_R8G8B8A8_TYPELESS);
    FMT_TO_STR(WINED3DFMT_R8G8B8A8_UNORM);
    FMT_TO_STR(WINED3DFMT_R8G8B8A8_UNORM_SRGB);
    FMT_TO_STR(WINED3DFMT_R8G8B8A8_UINT);
    FMT_TO_STR(WINED3DFMT_R8G8B8A8_SNORM);
    FMT_TO_STR(WINED3DFMT_R8G8B8A8_SINT);
    FMT_TO_STR(WINED3DFMT_R16G16_TYPELESS);
    FMT_TO_STR(WINED3DFMT_R16G16_FLOAT);
    FMT_TO_STR(WINED3DFMT_R16G16_UNORM);
    FMT_TO_STR(WINED3DFMT_R16G16_UINT);
    FMT_TO_STR(WINED3DFMT_R16G16_SNORM);
    FMT_TO_STR(WINED3DFMT_R16G16_SINT);
    FMT_TO_STR(WINED3DFMT_R32_TYPELESS);
    FMT_TO_STR(WINED3DFMT_D32_FLOAT);
    FMT_TO_STR(WINED3DFMT_R32_FLOAT);
    FMT_TO_STR(WINED3DFMT_R32_UINT);
    FMT_TO_STR(WINED3DFMT_R32_SINT);
    FMT_TO_STR(WINED3DFMT_R24G8_TYPELESS);
    FMT_TO_STR(WINED3DFMT_D24_UNORM_S8_UINT);
    FMT_TO_STR(WINED3DFMT_R24_UNORM_X8_TYPELESS);
    FMT_TO_STR(WINED3DFMT_X24_TYPELESS_G8_UINT);
    FMT_TO_STR(WINED3DFMT_R8G8_TYPELESS);
    FMT_TO_STR(WINED3DFMT_R8G8_UNORM);
    FMT_TO_STR(WINED3DFMT_R8G8_UINT);
    FMT_TO_STR(WINED3DFMT_R8G8_SNORM);
    FMT_TO_STR(WINED3DFMT_R8G8_SINT);
    FMT_TO_STR(WINED3DFMT_R16_TYPELESS);
    FMT_TO_STR(WINED3DFMT_R16_FLOAT);
    FMT_TO_STR(WINED3DFMT_D16_UNORM);
    FMT_TO_STR(WINED3DFMT_R16_UNORM);
    FMT_TO_STR(WINED3DFMT_R16_UINT);
    FMT_TO_STR(WINED3DFMT_R16_SNORM);
    FMT_TO_STR(WINED3DFMT_R16_SINT);
    FMT_TO_STR(WINED3DFMT_R8_TYPELESS);
    FMT_TO_STR(WINED3DFMT_R8_UNORM);
    FMT_TO_STR(WINED3DFMT_R8_UINT);
    FMT_TO_STR(WINED3DFMT_R8_SNORM);
    FMT_TO_STR(WINED3DFMT_R8_SINT);
    FMT_TO_STR(WINED3DFMT_A8_UNORM);
    FMT_TO_STR(WINED3DFMT_R1_UNORM);
    FMT_TO_STR(WINED3DFMT_R9G9B9E5_SHAREDEXP);
    FMT_TO_STR(WINED3DFMT_R8G8_B8G8_UNORM);
    FMT_TO_STR(WINED3DFMT_G8R8_G8B8_UNORM);
    FMT_TO_STR(WINED3DFMT_BC1_TYPELESS);
    FMT_TO_STR(WINED3DFMT_BC1_UNORM);
    FMT_TO_STR(WINED3DFMT_BC1_UNORM_SRGB);
    FMT_TO_STR(WINED3DFMT_BC2_TYPELESS);
    FMT_TO_STR(WINED3DFMT_BC2_UNORM);
    FMT_TO_STR(WINED3DFMT_BC2_UNORM_SRGB);
    FMT_TO_STR(WINED3DFMT_BC3_TYPELESS);
    FMT_TO_STR(WINED3DFMT_BC3_UNORM);
    FMT_TO_STR(WINED3DFMT_BC3_UNORM_SRGB);
    FMT_TO_STR(WINED3DFMT_BC4_TYPELESS);
    FMT_TO_STR(WINED3DFMT_BC4_UNORM);
    FMT_TO_STR(WINED3DFMT_BC4_SNORM);
    FMT_TO_STR(WINED3DFMT_BC5_TYPELESS);
    FMT_TO_STR(WINED3DFMT_BC5_UNORM);
    FMT_TO_STR(WINED3DFMT_BC5_SNORM);
    FMT_TO_STR(WINED3DFMT_B5G6R5_UNORM);
    FMT_TO_STR(WINED3DFMT_B5G5R5A1_UNORM);
    FMT_TO_STR(WINED3DFMT_B8G8R8A8_UNORM);
    FMT_TO_STR(WINED3DFMT_B8G8R8X8_UNORM);
#undef FMT_TO_STR
  default:
    {
      char fourcc[5];
      fourcc[0] = (char)(fmt);
      fourcc[1] = (char)(fmt >> 8);
      fourcc[2] = (char)(fmt >> 16);
      fourcc[3] = (char)(fmt >> 24);
      fourcc[4] = 0;
      if( isprint(fourcc[0]) && isprint(fourcc[1]) && isprint(fourcc[2]) && isprint(fourcc[3]) )
        FIXME("Unrecognized %u (as fourcc: %s) WINED3DFORMAT!\n", fmt, fourcc);
      else
        FIXME("Unrecognized %u WINED3DFORMAT!\n", fmt);
    }
    return "unrecognized";
  }
}

const char* debug_d3ddevicetype(WINED3DDEVTYPE devtype) {
  switch (devtype) {
#define DEVTYPE_TO_STR(dev) case dev: return #dev
    DEVTYPE_TO_STR(WINED3DDEVTYPE_HAL);
    DEVTYPE_TO_STR(WINED3DDEVTYPE_REF);
    DEVTYPE_TO_STR(WINED3DDEVTYPE_SW);
#undef DEVTYPE_TO_STR
  default:
    FIXME("Unrecognized %u WINED3DDEVTYPE!\n", devtype);
    return "unrecognized";
  }
}

const char* debug_d3dusage(DWORD usage) {
  switch (usage & WINED3DUSAGE_MASK) {
#define WINED3DUSAGE_TO_STR(u) case u: return #u
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_RENDERTARGET);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_DEPTHSTENCIL);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_WRITEONLY);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_SOFTWAREPROCESSING);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_DONOTCLIP);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_POINTS);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_RTPATCHES);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_NPATCHES);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_DYNAMIC);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_AUTOGENMIPMAP);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_DMAP);
#undef WINED3DUSAGE_TO_STR
  case 0: return "none";
  default:
    FIXME("Unrecognized %u Usage!\n", usage);
    return "unrecognized";
  }
}

const char* debug_d3dusagequery(DWORD usagequery) {
  switch (usagequery & WINED3DUSAGE_QUERY_MASK) {
#define WINED3DUSAGEQUERY_TO_STR(u) case u: return #u
    WINED3DUSAGEQUERY_TO_STR(WINED3DUSAGE_QUERY_FILTER);
    WINED3DUSAGEQUERY_TO_STR(WINED3DUSAGE_QUERY_LEGACYBUMPMAP);
    WINED3DUSAGEQUERY_TO_STR(WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING);
    WINED3DUSAGEQUERY_TO_STR(WINED3DUSAGE_QUERY_SRGBREAD);
    WINED3DUSAGEQUERY_TO_STR(WINED3DUSAGE_QUERY_SRGBWRITE);
    WINED3DUSAGEQUERY_TO_STR(WINED3DUSAGE_QUERY_VERTEXTEXTURE);
    WINED3DUSAGEQUERY_TO_STR(WINED3DUSAGE_QUERY_WRAPANDMIP);
#undef WINED3DUSAGEQUERY_TO_STR
  case 0: return "none";
  default:
    FIXME("Unrecognized %u Usage Query!\n", usagequery);
    return "unrecognized";
  }
}

const char* debug_d3ddeclmethod(WINED3DDECLMETHOD method) {
    switch (method) {
#define WINED3DDECLMETHOD_TO_STR(u) case u: return #u
        WINED3DDECLMETHOD_TO_STR(WINED3DDECLMETHOD_DEFAULT);
        WINED3DDECLMETHOD_TO_STR(WINED3DDECLMETHOD_PARTIALU);
        WINED3DDECLMETHOD_TO_STR(WINED3DDECLMETHOD_PARTIALV);
        WINED3DDECLMETHOD_TO_STR(WINED3DDECLMETHOD_CROSSUV);
        WINED3DDECLMETHOD_TO_STR(WINED3DDECLMETHOD_UV);
        WINED3DDECLMETHOD_TO_STR(WINED3DDECLMETHOD_LOOKUP);
        WINED3DDECLMETHOD_TO_STR(WINED3DDECLMETHOD_LOOKUPPRESAMPLED);
#undef WINED3DDECLMETHOD_TO_STR
        default:
            FIXME("Unrecognized %u declaration method!\n", method);
            return "unrecognized";
    }
}

const char* debug_d3ddeclusage(BYTE usage) {
    switch (usage) {
#define WINED3DDECLUSAGE_TO_STR(u) case u: return #u
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_POSITION);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_BLENDWEIGHT);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_BLENDINDICES);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_NORMAL);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_PSIZE);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_TEXCOORD);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_TANGENT);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_BINORMAL);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_TESSFACTOR);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_POSITIONT);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_COLOR);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_FOG);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_DEPTH);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_SAMPLE);
#undef WINED3DDECLUSAGE_TO_STR
        default:
            FIXME("Unrecognized %u declaration usage!\n", usage);
            return "unrecognized";
    }
}

const char* debug_d3dresourcetype(WINED3DRESOURCETYPE res) {
  switch (res) {
#define RES_TO_STR(res) case res: return #res
    RES_TO_STR(WINED3DRTYPE_SURFACE);
    RES_TO_STR(WINED3DRTYPE_VOLUME);
    RES_TO_STR(WINED3DRTYPE_TEXTURE);
    RES_TO_STR(WINED3DRTYPE_VOLUMETEXTURE);
    RES_TO_STR(WINED3DRTYPE_CUBETEXTURE);
    RES_TO_STR(WINED3DRTYPE_VERTEXBUFFER);
    RES_TO_STR(WINED3DRTYPE_INDEXBUFFER);
    RES_TO_STR(WINED3DRTYPE_BUFFER);
#undef  RES_TO_STR
  default:
    FIXME("Unrecognized %u WINED3DRESOURCETYPE!\n", res);
    return "unrecognized";
  }
}

const char* debug_d3dprimitivetype(WINED3DPRIMITIVETYPE PrimitiveType) {
  switch (PrimitiveType) {
#define PRIM_TO_STR(prim) case prim: return #prim
    PRIM_TO_STR(WINED3DPT_UNDEFINED);
    PRIM_TO_STR(WINED3DPT_POINTLIST);
    PRIM_TO_STR(WINED3DPT_LINELIST);
    PRIM_TO_STR(WINED3DPT_LINESTRIP);
    PRIM_TO_STR(WINED3DPT_TRIANGLELIST);
    PRIM_TO_STR(WINED3DPT_TRIANGLESTRIP);
    PRIM_TO_STR(WINED3DPT_TRIANGLEFAN);
    PRIM_TO_STR(WINED3DPT_LINELIST_ADJ);
    PRIM_TO_STR(WINED3DPT_LINESTRIP_ADJ);
    PRIM_TO_STR(WINED3DPT_TRIANGLELIST_ADJ);
    PRIM_TO_STR(WINED3DPT_TRIANGLESTRIP_ADJ);
#undef  PRIM_TO_STR
  default:
    FIXME("Unrecognized %u WINED3DPRIMITIVETYPE!\n", PrimitiveType);
    return "unrecognized";
  }
}

const char* debug_d3drenderstate(DWORD state) {
  switch (state) {
#define D3DSTATE_TO_STR(u) case u: return #u
    D3DSTATE_TO_STR(WINED3DRS_TEXTUREHANDLE             );
    D3DSTATE_TO_STR(WINED3DRS_ANTIALIAS                 );
    D3DSTATE_TO_STR(WINED3DRS_TEXTUREADDRESS            );
    D3DSTATE_TO_STR(WINED3DRS_TEXTUREPERSPECTIVE        );
    D3DSTATE_TO_STR(WINED3DRS_WRAPU                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAPV                     );
    D3DSTATE_TO_STR(WINED3DRS_ZENABLE                   );
    D3DSTATE_TO_STR(WINED3DRS_FILLMODE                  );
    D3DSTATE_TO_STR(WINED3DRS_SHADEMODE                 );
    D3DSTATE_TO_STR(WINED3DRS_LINEPATTERN               );
    D3DSTATE_TO_STR(WINED3DRS_MONOENABLE                );
    D3DSTATE_TO_STR(WINED3DRS_ROP2                      );
    D3DSTATE_TO_STR(WINED3DRS_PLANEMASK                 );
    D3DSTATE_TO_STR(WINED3DRS_ZWRITEENABLE              );
    D3DSTATE_TO_STR(WINED3DRS_ALPHATESTENABLE           );
    D3DSTATE_TO_STR(WINED3DRS_LASTPIXEL                 );
    D3DSTATE_TO_STR(WINED3DRS_TEXTUREMAG                );
    D3DSTATE_TO_STR(WINED3DRS_TEXTUREMIN                );
    D3DSTATE_TO_STR(WINED3DRS_SRCBLEND                  );
    D3DSTATE_TO_STR(WINED3DRS_DESTBLEND                 );
    D3DSTATE_TO_STR(WINED3DRS_TEXTUREMAPBLEND           );
    D3DSTATE_TO_STR(WINED3DRS_CULLMODE                  );
    D3DSTATE_TO_STR(WINED3DRS_ZFUNC                     );
    D3DSTATE_TO_STR(WINED3DRS_ALPHAREF                  );
    D3DSTATE_TO_STR(WINED3DRS_ALPHAFUNC                 );
    D3DSTATE_TO_STR(WINED3DRS_DITHERENABLE              );
    D3DSTATE_TO_STR(WINED3DRS_ALPHABLENDENABLE          );
    D3DSTATE_TO_STR(WINED3DRS_FOGENABLE                 );
    D3DSTATE_TO_STR(WINED3DRS_SPECULARENABLE            );
    D3DSTATE_TO_STR(WINED3DRS_ZVISIBLE                  );
    D3DSTATE_TO_STR(WINED3DRS_SUBPIXEL                  );
    D3DSTATE_TO_STR(WINED3DRS_SUBPIXELX                 );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEDALPHA             );
    D3DSTATE_TO_STR(WINED3DRS_FOGCOLOR                  );
    D3DSTATE_TO_STR(WINED3DRS_FOGTABLEMODE              );
    D3DSTATE_TO_STR(WINED3DRS_FOGSTART                  );
    D3DSTATE_TO_STR(WINED3DRS_FOGEND                    );
    D3DSTATE_TO_STR(WINED3DRS_FOGDENSITY                );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEENABLE             );
    D3DSTATE_TO_STR(WINED3DRS_EDGEANTIALIAS             );
    D3DSTATE_TO_STR(WINED3DRS_COLORKEYENABLE            );
    D3DSTATE_TO_STR(WINED3DRS_BORDERCOLOR               );
    D3DSTATE_TO_STR(WINED3DRS_TEXTUREADDRESSU           );
    D3DSTATE_TO_STR(WINED3DRS_TEXTUREADDRESSV           );
    D3DSTATE_TO_STR(WINED3DRS_MIPMAPLODBIAS             );
    D3DSTATE_TO_STR(WINED3DRS_ZBIAS                     );
    D3DSTATE_TO_STR(WINED3DRS_RANGEFOGENABLE            );
    D3DSTATE_TO_STR(WINED3DRS_ANISOTROPY                );
    D3DSTATE_TO_STR(WINED3DRS_FLUSHBATCH                );
    D3DSTATE_TO_STR(WINED3DRS_TRANSLUCENTSORTINDEPENDENT);
    D3DSTATE_TO_STR(WINED3DRS_STENCILENABLE             );
    D3DSTATE_TO_STR(WINED3DRS_STENCILFAIL               );
    D3DSTATE_TO_STR(WINED3DRS_STENCILZFAIL              );
    D3DSTATE_TO_STR(WINED3DRS_STENCILPASS               );
    D3DSTATE_TO_STR(WINED3DRS_STENCILFUNC               );
    D3DSTATE_TO_STR(WINED3DRS_STENCILREF                );
    D3DSTATE_TO_STR(WINED3DRS_STENCILMASK               );
    D3DSTATE_TO_STR(WINED3DRS_STENCILWRITEMASK          );
    D3DSTATE_TO_STR(WINED3DRS_TEXTUREFACTOR             );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN00          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN01          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN02          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN03          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN04          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN05          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN06          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN07          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN08          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN09          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN10          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN11          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN12          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN13          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN14          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN15          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN16          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN17          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN18          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN19          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN20          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN21          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN22          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN23          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN24          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN25          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN26          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN27          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN28          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN29          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN30          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN31          );
    D3DSTATE_TO_STR(WINED3DRS_WRAP0                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP1                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP2                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP3                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP4                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP5                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP6                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP7                     );
    D3DSTATE_TO_STR(WINED3DRS_CLIPPING                  );
    D3DSTATE_TO_STR(WINED3DRS_LIGHTING                  );
    D3DSTATE_TO_STR(WINED3DRS_EXTENTS                   );
    D3DSTATE_TO_STR(WINED3DRS_AMBIENT                   );
    D3DSTATE_TO_STR(WINED3DRS_FOGVERTEXMODE             );
    D3DSTATE_TO_STR(WINED3DRS_COLORVERTEX               );
    D3DSTATE_TO_STR(WINED3DRS_LOCALVIEWER               );
    D3DSTATE_TO_STR(WINED3DRS_NORMALIZENORMALS          );
    D3DSTATE_TO_STR(WINED3DRS_COLORKEYBLENDENABLE       );
    D3DSTATE_TO_STR(WINED3DRS_DIFFUSEMATERIALSOURCE     );
    D3DSTATE_TO_STR(WINED3DRS_SPECULARMATERIALSOURCE    );
    D3DSTATE_TO_STR(WINED3DRS_AMBIENTMATERIALSOURCE     );
    D3DSTATE_TO_STR(WINED3DRS_EMISSIVEMATERIALSOURCE    );
    D3DSTATE_TO_STR(WINED3DRS_VERTEXBLEND               );
    D3DSTATE_TO_STR(WINED3DRS_CLIPPLANEENABLE           );
    D3DSTATE_TO_STR(WINED3DRS_SOFTWAREVERTEXPROCESSING  );
    D3DSTATE_TO_STR(WINED3DRS_POINTSIZE                 );
    D3DSTATE_TO_STR(WINED3DRS_POINTSIZE_MIN             );
    D3DSTATE_TO_STR(WINED3DRS_POINTSPRITEENABLE         );
    D3DSTATE_TO_STR(WINED3DRS_POINTSCALEENABLE          );
    D3DSTATE_TO_STR(WINED3DRS_POINTSCALE_A              );
    D3DSTATE_TO_STR(WINED3DRS_POINTSCALE_B              );
    D3DSTATE_TO_STR(WINED3DRS_POINTSCALE_C              );
    D3DSTATE_TO_STR(WINED3DRS_MULTISAMPLEANTIALIAS      );
    D3DSTATE_TO_STR(WINED3DRS_MULTISAMPLEMASK           );
    D3DSTATE_TO_STR(WINED3DRS_PATCHEDGESTYLE            );
    D3DSTATE_TO_STR(WINED3DRS_PATCHSEGMENTS             );
    D3DSTATE_TO_STR(WINED3DRS_DEBUGMONITORTOKEN         );
    D3DSTATE_TO_STR(WINED3DRS_POINTSIZE_MAX             );
    D3DSTATE_TO_STR(WINED3DRS_INDEXEDVERTEXBLENDENABLE  );
    D3DSTATE_TO_STR(WINED3DRS_COLORWRITEENABLE          );
    D3DSTATE_TO_STR(WINED3DRS_TWEENFACTOR               );
    D3DSTATE_TO_STR(WINED3DRS_BLENDOP                   );
    D3DSTATE_TO_STR(WINED3DRS_POSITIONDEGREE            );
    D3DSTATE_TO_STR(WINED3DRS_NORMALDEGREE              );
    D3DSTATE_TO_STR(WINED3DRS_SCISSORTESTENABLE         );
    D3DSTATE_TO_STR(WINED3DRS_SLOPESCALEDEPTHBIAS       );
    D3DSTATE_TO_STR(WINED3DRS_ANTIALIASEDLINEENABLE     );
    D3DSTATE_TO_STR(WINED3DRS_MINTESSELLATIONLEVEL      );
    D3DSTATE_TO_STR(WINED3DRS_MAXTESSELLATIONLEVEL      );
    D3DSTATE_TO_STR(WINED3DRS_ADAPTIVETESS_X            );
    D3DSTATE_TO_STR(WINED3DRS_ADAPTIVETESS_Y            );
    D3DSTATE_TO_STR(WINED3DRS_ADAPTIVETESS_Z            );
    D3DSTATE_TO_STR(WINED3DRS_ADAPTIVETESS_W            );
    D3DSTATE_TO_STR(WINED3DRS_ENABLEADAPTIVETESSELLATION);
    D3DSTATE_TO_STR(WINED3DRS_TWOSIDEDSTENCILMODE       );
    D3DSTATE_TO_STR(WINED3DRS_CCW_STENCILFAIL           );
    D3DSTATE_TO_STR(WINED3DRS_CCW_STENCILZFAIL          );
    D3DSTATE_TO_STR(WINED3DRS_CCW_STENCILPASS           );
    D3DSTATE_TO_STR(WINED3DRS_CCW_STENCILFUNC           );
    D3DSTATE_TO_STR(WINED3DRS_COLORWRITEENABLE1         );
    D3DSTATE_TO_STR(WINED3DRS_COLORWRITEENABLE2         );
    D3DSTATE_TO_STR(WINED3DRS_COLORWRITEENABLE3         );
    D3DSTATE_TO_STR(WINED3DRS_BLENDFACTOR               );
    D3DSTATE_TO_STR(WINED3DRS_SRGBWRITEENABLE           );
    D3DSTATE_TO_STR(WINED3DRS_DEPTHBIAS                 );
    D3DSTATE_TO_STR(WINED3DRS_WRAP8                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP9                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP10                    );
    D3DSTATE_TO_STR(WINED3DRS_WRAP11                    );
    D3DSTATE_TO_STR(WINED3DRS_WRAP12                    );
    D3DSTATE_TO_STR(WINED3DRS_WRAP13                    );
    D3DSTATE_TO_STR(WINED3DRS_WRAP14                    );
    D3DSTATE_TO_STR(WINED3DRS_WRAP15                    );
    D3DSTATE_TO_STR(WINED3DRS_SEPARATEALPHABLENDENABLE  );
    D3DSTATE_TO_STR(WINED3DRS_SRCBLENDALPHA             );
    D3DSTATE_TO_STR(WINED3DRS_DESTBLENDALPHA            );
    D3DSTATE_TO_STR(WINED3DRS_BLENDOPALPHA              );
#undef D3DSTATE_TO_STR
  default:
    FIXME("Unrecognized %u render state!\n", state);
    return "unrecognized";
  }
}

const char* debug_d3dsamplerstate(DWORD state) {
  switch (state) {
#define D3DSTATE_TO_STR(u) case u: return #u
    D3DSTATE_TO_STR(WINED3DSAMP_BORDERCOLOR  );
    D3DSTATE_TO_STR(WINED3DSAMP_ADDRESSU     );
    D3DSTATE_TO_STR(WINED3DSAMP_ADDRESSV     );
    D3DSTATE_TO_STR(WINED3DSAMP_ADDRESSW     );
    D3DSTATE_TO_STR(WINED3DSAMP_MAGFILTER    );
    D3DSTATE_TO_STR(WINED3DSAMP_MINFILTER    );
    D3DSTATE_TO_STR(WINED3DSAMP_MIPFILTER    );
    D3DSTATE_TO_STR(WINED3DSAMP_MIPMAPLODBIAS);
    D3DSTATE_TO_STR(WINED3DSAMP_MAXMIPLEVEL  );
    D3DSTATE_TO_STR(WINED3DSAMP_MAXANISOTROPY);
    D3DSTATE_TO_STR(WINED3DSAMP_SRGBTEXTURE  );
    D3DSTATE_TO_STR(WINED3DSAMP_ELEMENTINDEX );
    D3DSTATE_TO_STR(WINED3DSAMP_DMAPOFFSET   );
#undef D3DSTATE_TO_STR
  default:
    FIXME("Unrecognized %u sampler state!\n", state);
    return "unrecognized";
  }
}

const char *debug_d3dtexturefiltertype(WINED3DTEXTUREFILTERTYPE filter_type) {
    switch (filter_type) {
#define D3DTEXTUREFILTERTYPE_TO_STR(u) case u: return #u
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3DTEXF_NONE);
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3DTEXF_POINT);
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3DTEXF_LINEAR);
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3DTEXF_ANISOTROPIC);
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3DTEXF_FLATCUBIC);
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3DTEXF_GAUSSIANCUBIC);
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3DTEXF_PYRAMIDALQUAD);
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3DTEXF_GAUSSIANQUAD);
#undef D3DTEXTUREFILTERTYPE_TO_STR
        default:
            FIXME("Unrecognied texture filter type 0x%08x\n", filter_type);
            return "unrecognized";
    }
}

const char* debug_d3dtexturestate(DWORD state) {
  switch (state) {
#define D3DSTATE_TO_STR(u) case u: return #u
    D3DSTATE_TO_STR(WINED3DTSS_COLOROP               );
    D3DSTATE_TO_STR(WINED3DTSS_COLORARG1             );
    D3DSTATE_TO_STR(WINED3DTSS_COLORARG2             );
    D3DSTATE_TO_STR(WINED3DTSS_ALPHAOP               );
    D3DSTATE_TO_STR(WINED3DTSS_ALPHAARG1             );
    D3DSTATE_TO_STR(WINED3DTSS_ALPHAARG2             );
    D3DSTATE_TO_STR(WINED3DTSS_BUMPENVMAT00          );
    D3DSTATE_TO_STR(WINED3DTSS_BUMPENVMAT01          );
    D3DSTATE_TO_STR(WINED3DTSS_BUMPENVMAT10          );
    D3DSTATE_TO_STR(WINED3DTSS_BUMPENVMAT11          );
    D3DSTATE_TO_STR(WINED3DTSS_TEXCOORDINDEX         );
    D3DSTATE_TO_STR(WINED3DTSS_BUMPENVLSCALE         );
    D3DSTATE_TO_STR(WINED3DTSS_BUMPENVLOFFSET        );
    D3DSTATE_TO_STR(WINED3DTSS_TEXTURETRANSFORMFLAGS );
    D3DSTATE_TO_STR(WINED3DTSS_COLORARG0             );
    D3DSTATE_TO_STR(WINED3DTSS_ALPHAARG0             );
    D3DSTATE_TO_STR(WINED3DTSS_RESULTARG             );
    D3DSTATE_TO_STR(WINED3DTSS_CONSTANT              );
#undef D3DSTATE_TO_STR
  default:
    FIXME("Unrecognized %u texture state!\n", state);
    return "unrecognized";
  }
}

const char* debug_d3dtop(WINED3DTEXTUREOP d3dtop) {
    switch (d3dtop) {
#define D3DTOP_TO_STR(u) case u: return #u
        D3DTOP_TO_STR(WINED3DTOP_DISABLE);
        D3DTOP_TO_STR(WINED3DTOP_SELECTARG1);
        D3DTOP_TO_STR(WINED3DTOP_SELECTARG2);
        D3DTOP_TO_STR(WINED3DTOP_MODULATE);
        D3DTOP_TO_STR(WINED3DTOP_MODULATE2X);
        D3DTOP_TO_STR(WINED3DTOP_MODULATE4X);
        D3DTOP_TO_STR(WINED3DTOP_ADD);
        D3DTOP_TO_STR(WINED3DTOP_ADDSIGNED);
        D3DTOP_TO_STR(WINED3DTOP_ADDSIGNED2X);
        D3DTOP_TO_STR(WINED3DTOP_SUBTRACT);
        D3DTOP_TO_STR(WINED3DTOP_ADDSMOOTH);
        D3DTOP_TO_STR(WINED3DTOP_BLENDDIFFUSEALPHA);
        D3DTOP_TO_STR(WINED3DTOP_BLENDTEXTUREALPHA);
        D3DTOP_TO_STR(WINED3DTOP_BLENDFACTORALPHA);
        D3DTOP_TO_STR(WINED3DTOP_BLENDTEXTUREALPHAPM);
        D3DTOP_TO_STR(WINED3DTOP_BLENDCURRENTALPHA);
        D3DTOP_TO_STR(WINED3DTOP_PREMODULATE);
        D3DTOP_TO_STR(WINED3DTOP_MODULATEALPHA_ADDCOLOR);
        D3DTOP_TO_STR(WINED3DTOP_MODULATECOLOR_ADDALPHA);
        D3DTOP_TO_STR(WINED3DTOP_MODULATEINVALPHA_ADDCOLOR);
        D3DTOP_TO_STR(WINED3DTOP_MODULATEINVCOLOR_ADDALPHA);
        D3DTOP_TO_STR(WINED3DTOP_BUMPENVMAP);
        D3DTOP_TO_STR(WINED3DTOP_BUMPENVMAPLUMINANCE);
        D3DTOP_TO_STR(WINED3DTOP_DOTPRODUCT3);
        D3DTOP_TO_STR(WINED3DTOP_MULTIPLYADD);
        D3DTOP_TO_STR(WINED3DTOP_LERP);
#undef D3DTOP_TO_STR
        default:
            FIXME("Unrecognized %u WINED3DTOP\n", d3dtop);
            return "unrecognized";
    }
}

const char* debug_d3dtstype(WINED3DTRANSFORMSTATETYPE tstype) {
    switch (tstype) {
#define TSTYPE_TO_STR(tstype) case tstype: return #tstype
    TSTYPE_TO_STR(WINED3DTS_VIEW);
    TSTYPE_TO_STR(WINED3DTS_PROJECTION);
    TSTYPE_TO_STR(WINED3DTS_TEXTURE0);
    TSTYPE_TO_STR(WINED3DTS_TEXTURE1);
    TSTYPE_TO_STR(WINED3DTS_TEXTURE2);
    TSTYPE_TO_STR(WINED3DTS_TEXTURE3);
    TSTYPE_TO_STR(WINED3DTS_TEXTURE4);
    TSTYPE_TO_STR(WINED3DTS_TEXTURE5);
    TSTYPE_TO_STR(WINED3DTS_TEXTURE6);
    TSTYPE_TO_STR(WINED3DTS_TEXTURE7);
    TSTYPE_TO_STR(WINED3DTS_WORLDMATRIX(0));
#undef TSTYPE_TO_STR
    default:
        if (tstype > 256 && tstype < 512) {
            FIXME("WINED3DTS_WORLDMATRIX(%u). 1..255 not currently supported\n", tstype);
            return ("WINED3DTS_WORLDMATRIX > 0");
        }
        FIXME("Unrecognized %u WINED3DTS\n", tstype);
        return "unrecognized";
    }
}

const char* debug_d3dpool(WINED3DPOOL Pool) {
  switch (Pool) {
#define POOL_TO_STR(p) case p: return #p
    POOL_TO_STR(WINED3DPOOL_DEFAULT);
    POOL_TO_STR(WINED3DPOOL_MANAGED);
    POOL_TO_STR(WINED3DPOOL_SYSTEMMEM);
    POOL_TO_STR(WINED3DPOOL_SCRATCH);
#undef  POOL_TO_STR
  default:
    FIXME("Unrecognized %u WINED3DPOOL!\n", Pool);
    return "unrecognized";
  }
}

const char *debug_fbostatus(GLenum status) {
    switch(status) {
#define FBOSTATUS_TO_STR(u) case u: return #u
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_COMPLETE_EXT);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_UNSUPPORTED_EXT);
#undef FBOSTATUS_TO_STR
        default:
            FIXME("Unrecognied FBO status 0x%08x\n", status);
            return "unrecognized";
    }
}

const char *debug_glerror(GLenum error) {
    switch(error) {
#define GLERROR_TO_STR(u) case u: return #u
        GLERROR_TO_STR(GL_NO_ERROR);
        GLERROR_TO_STR(GL_INVALID_ENUM);
        GLERROR_TO_STR(GL_INVALID_VALUE);
        GLERROR_TO_STR(GL_INVALID_OPERATION);
        GLERROR_TO_STR(GL_STACK_OVERFLOW);
        GLERROR_TO_STR(GL_STACK_UNDERFLOW);
        GLERROR_TO_STR(GL_OUT_OF_MEMORY);
        GLERROR_TO_STR(GL_INVALID_FRAMEBUFFER_OPERATION_EXT);
#undef GLERROR_TO_STR
        default:
            FIXME("Unrecognied GL error 0x%08x\n", error);
            return "unrecognized";
    }
}

const char *debug_d3dbasis(WINED3DBASISTYPE basis) {
    switch(basis) {
        case WINED3DBASIS_BEZIER:       return "WINED3DBASIS_BEZIER";
        case WINED3DBASIS_BSPLINE:      return "WINED3DBASIS_BSPLINE";
        case WINED3DBASIS_INTERPOLATE:  return "WINED3DBASIS_INTERPOLATE";
        default:                        return "unrecognized";
    }
}

const char *debug_d3ddegree(WINED3DDEGREETYPE degree) {
    switch(degree) {
        case WINED3DDEGREE_LINEAR:      return "WINED3DDEGREE_LINEAR";
        case WINED3DDEGREE_QUADRATIC:   return "WINED3DDEGREE_QUADRATIC";
        case WINED3DDEGREE_CUBIC:       return "WINED3DDEGREE_CUBIC";
        case WINED3DDEGREE_QUINTIC:     return "WINED3DDEGREE_QUINTIC";
        default:                        return "unrecognized";
    }
}

static const char *debug_fixup_channel_source(enum fixup_channel_source source)
{
    switch(source)
    {
#define WINED3D_TO_STR(x) case x: return #x
        WINED3D_TO_STR(CHANNEL_SOURCE_ZERO);
        WINED3D_TO_STR(CHANNEL_SOURCE_ONE);
        WINED3D_TO_STR(CHANNEL_SOURCE_X);
        WINED3D_TO_STR(CHANNEL_SOURCE_Y);
        WINED3D_TO_STR(CHANNEL_SOURCE_Z);
        WINED3D_TO_STR(CHANNEL_SOURCE_W);
        WINED3D_TO_STR(CHANNEL_SOURCE_YUV0);
        WINED3D_TO_STR(CHANNEL_SOURCE_YUV1);
#undef WINED3D_TO_STR
        default:
            FIXME("Unrecognized fixup_channel_source %#x\n", source);
            return "unrecognized";
    }
}

static const char *debug_yuv_fixup(enum yuv_fixup yuv_fixup)
{
    switch(yuv_fixup)
    {
#define WINED3D_TO_STR(x) case x: return #x
        WINED3D_TO_STR(YUV_FIXUP_YUY2);
        WINED3D_TO_STR(YUV_FIXUP_UYVY);
        WINED3D_TO_STR(YUV_FIXUP_YV12);
#undef WINED3D_TO_STR
        default:
            FIXME("Unrecognized YUV fixup %#x\n", yuv_fixup);
            return "unrecognized";
    }
}

void dump_color_fixup_desc(struct color_fixup_desc fixup)
{
    if (is_yuv_fixup(fixup))
    {
        TRACE("\tYUV: %s\n", debug_yuv_fixup(get_yuv_fixup(fixup)));
        return;
    }

    TRACE("\tX: %s%s\n", debug_fixup_channel_source(fixup.x_source), fixup.x_sign_fixup ? ", SIGN_FIXUP" : "");
    TRACE("\tY: %s%s\n", debug_fixup_channel_source(fixup.y_source), fixup.y_sign_fixup ? ", SIGN_FIXUP" : "");
    TRACE("\tZ: %s%s\n", debug_fixup_channel_source(fixup.z_source), fixup.z_sign_fixup ? ", SIGN_FIXUP" : "");
    TRACE("\tW: %s%s\n", debug_fixup_channel_source(fixup.w_source), fixup.w_sign_fixup ? ", SIGN_FIXUP" : "");
}

const char *debug_surflocation(DWORD flag) {
    char buf[128];

    buf[0] = 0;
    if(flag & SFLAG_INSYSMEM) strcat(buf, " | SFLAG_INSYSMEM");
    if(flag & SFLAG_INDRAWABLE) strcat(buf, " | SFLAG_INDRAWABLE");
    if(flag & SFLAG_INTEXTURE) strcat(buf, " | SFLAG_INTEXTURE");
    if(flag & SFLAG_INSRGBTEX) strcat(buf, " | SFLAG_INSRGBTEX");
    return wine_dbg_sprintf("%s", buf[0] ? buf + 3 : "0");
}

/*****************************************************************************
 * Useful functions mapping GL <-> D3D values
 */
GLenum StencilOp(DWORD op) {
    switch(op) {
    case WINED3DSTENCILOP_KEEP    : return GL_KEEP;
    case WINED3DSTENCILOP_ZERO    : return GL_ZERO;
    case WINED3DSTENCILOP_REPLACE : return GL_REPLACE;
    case WINED3DSTENCILOP_INCRSAT : return GL_INCR;
    case WINED3DSTENCILOP_DECRSAT : return GL_DECR;
    case WINED3DSTENCILOP_INVERT  : return GL_INVERT;
    case WINED3DSTENCILOP_INCR    : return GL_INCR_WRAP_EXT;
    case WINED3DSTENCILOP_DECR    : return GL_DECR_WRAP_EXT;
    default:
        FIXME("Unrecognized stencil op %d\n", op);
        return GL_KEEP;
    }
}

GLenum CompareFunc(DWORD func) {
    switch ((WINED3DCMPFUNC)func) {
    case WINED3DCMP_NEVER        : return GL_NEVER;
    case WINED3DCMP_LESS         : return GL_LESS;
    case WINED3DCMP_EQUAL        : return GL_EQUAL;
    case WINED3DCMP_LESSEQUAL    : return GL_LEQUAL;
    case WINED3DCMP_GREATER      : return GL_GREATER;
    case WINED3DCMP_NOTEQUAL     : return GL_NOTEQUAL;
    case WINED3DCMP_GREATEREQUAL : return GL_GEQUAL;
    case WINED3DCMP_ALWAYS       : return GL_ALWAYS;
    default:
        FIXME("Unrecognized WINED3DCMPFUNC value %d\n", func);
        return 0;
    }
}

BOOL is_invalid_op(IWineD3DDeviceImpl *This, int stage, WINED3DTEXTUREOP op, DWORD arg1, DWORD arg2, DWORD arg3) {
    if (op == WINED3DTOP_DISABLE) return FALSE;
    if (This->stateBlock->textures[stage]) return FALSE;

    if ((arg1 & WINED3DTA_SELECTMASK) == WINED3DTA_TEXTURE
            && op != WINED3DTOP_SELECTARG2) return TRUE;
    if ((arg2 & WINED3DTA_SELECTMASK) == WINED3DTA_TEXTURE
            && op != WINED3DTOP_SELECTARG1) return TRUE;
    if ((arg3 & WINED3DTA_SELECTMASK) == WINED3DTA_TEXTURE
            && (op == WINED3DTOP_MULTIPLYADD || op == WINED3DTOP_LERP)) return TRUE;

    return FALSE;
}

/* Setup this textures matrix according to the texture flags*/
void set_texture_matrix(const float *smat, DWORD flags, BOOL calculatedCoords, BOOL transformed,
        WINED3DFORMAT vtx_fmt, BOOL ffp_proj_control)
{
    float mat[16];

    glMatrixMode(GL_TEXTURE);
    checkGLcall("glMatrixMode(GL_TEXTURE)");

    if (flags == WINED3DTTFF_DISABLE || flags == WINED3DTTFF_COUNT1 || transformed) {
        glLoadIdentity();
        checkGLcall("glLoadIdentity()");
        return;
    }

    if (flags == (WINED3DTTFF_COUNT1|WINED3DTTFF_PROJECTED)) {
        ERR("Invalid texture transform flags: WINED3DTTFF_COUNT1|WINED3DTTFF_PROJECTED\n");
        return;
    }

    memcpy(mat, smat, 16 * sizeof(float));

    if (flags & WINED3DTTFF_PROJECTED) {
        if(!ffp_proj_control) {
            switch (flags & ~WINED3DTTFF_PROJECTED) {
            case WINED3DTTFF_COUNT2:
                mat[3] = mat[1], mat[7] = mat[5], mat[11] = mat[9], mat[15] = mat[13];
                mat[1] = mat[5] = mat[9] = mat[13] = 0;
                break;
            case WINED3DTTFF_COUNT3:
                mat[3] = mat[2], mat[7] = mat[6], mat[11] = mat[10], mat[15] = mat[14];
                mat[2] = mat[6] = mat[10] = mat[14] = 0;
                break;
            }
        }
    } else { /* under directx the R/Z coord can be used for translation, under opengl we use the Q coord instead */
        if(!calculatedCoords) {
            switch(vtx_fmt)
            {
                case WINED3DFMT_R32_FLOAT:
                    /* Direct3D passes the default 1.0 in the 2nd coord, while gl passes it in the 4th.
                     * swap 2nd and 4th coord. No need to store the value of mat[12] in mat[4] because
                     * the input value to the transformation will be 0, so the matrix value is irrelevant
                     */
                    mat[12] = mat[4];
                    mat[13] = mat[5];
                    mat[14] = mat[6];
                    mat[15] = mat[7];
                    break;
                case WINED3DFMT_R32G32_FLOAT:
                    /* See above, just 3rd and 4th coord
                    */
                    mat[12] = mat[8];
                    mat[13] = mat[9];
                    mat[14] = mat[10];
                    mat[15] = mat[11];
                    break;
                case WINED3DFMT_R32G32B32_FLOAT: /* Opengl defaults match dx defaults */
                case WINED3DFMT_R32G32B32A32_FLOAT: /* No defaults apply, all app defined */

                /* This is to prevent swapping the matrix lines and put the default 4th coord = 1.0
                 * into a bad place. The division elimination below will apply to make sure the
                 * 1.0 doesn't do anything bad. The caller will set this value if the stride is 0
                 */
                case WINED3DFMT_UNKNOWN: /* No texture coords, 0/0/0/1 defaults are passed */
                    break;
                default:
                    FIXME("Unexpected fixed function texture coord input\n");
            }
        }
        if(!ffp_proj_control) {
            switch (flags & ~WINED3DTTFF_PROJECTED) {
                /* case WINED3DTTFF_COUNT1: Won't ever get here */
                case WINED3DTTFF_COUNT2: mat[2] = mat[6] = mat[10] = mat[14] = 0;
                /* OpenGL divides the first 3 vertex coord by the 4th by default,
                * which is essentially the same as D3DTTFF_PROJECTED. Make sure that
                * the 4th coord evaluates to 1.0 to eliminate that.
                *
                * If the fixed function pipeline is used, the 4th value remains unused,
                * so there is no danger in doing this. With vertex shaders we have a
                * problem. Should an app hit that problem, the code here would have to
                * check for pixel shaders, and the shader has to undo the default gl divide.
                *
                * A more serious problem occurs if the app passes 4 coordinates in, and the
                * 4th is != 1.0(opengl default). This would have to be fixed in drawStridedSlow
                * or a replacement shader
                */
                default: mat[3] = mat[7] = mat[11] = 0; mat[15] = 1;
            }
        }
    }

    glLoadMatrixf(mat);
    checkGLcall("glLoadMatrixf(mat)");
}
#undef GLINFO_LOCATION

/* This small helper function is used to convert a bitmask into the number of masked bits */
unsigned int count_bits(unsigned int mask)
{
    unsigned int count;
    for (count = 0; mask; ++count)
    {
        mask &= mask - 1;
    }
    return count;
}

/* Helper function for retrieving color info for ChoosePixelFormat and wglChoosePixelFormatARB.
 * The later function requires individual color components. */
BOOL getColorBits(const struct GlPixelFormatDesc *format_desc,
        short *redSize, short *greenSize, short *blueSize, short *alphaSize, short *totalSize)
{
    TRACE("fmt: %s\n", debug_d3dformat(format_desc->format));
    switch(format_desc->format)
    {
        case WINED3DFMT_X8R8G8B8:
        case WINED3DFMT_R8G8B8:
        case WINED3DFMT_A8R8G8B8:
        case WINED3DFMT_R8G8B8A8_UNORM:
        case WINED3DFMT_A2R10G10B10:
        case WINED3DFMT_X1R5G5B5:
        case WINED3DFMT_A1R5G5B5:
        case WINED3DFMT_R5G6B5:
        case WINED3DFMT_X4R4G4B4:
        case WINED3DFMT_A4R4G4B4:
        case WINED3DFMT_R3G3B2:
        case WINED3DFMT_A8P8:
        case WINED3DFMT_P8:
            break;
        default:
            ERR("Unsupported format: %s\n", debug_d3dformat(format_desc->format));
            return FALSE;
    }

    *redSize = count_bits(format_desc->red_mask);
    *greenSize = count_bits(format_desc->green_mask);
    *blueSize = count_bits(format_desc->blue_mask);
    *alphaSize = count_bits(format_desc->alpha_mask);
    *totalSize = *redSize + *greenSize + *blueSize + *alphaSize;

    TRACE("Returning red:  %d, green: %d, blue: %d, alpha: %d, total: %d for fmt=%s\n",
            *redSize, *greenSize, *blueSize, *alphaSize, *totalSize, debug_d3dformat(format_desc->format));
    return TRUE;
}

/* Helper function for retrieving depth/stencil info for ChoosePixelFormat and wglChoosePixelFormatARB */
BOOL getDepthStencilBits(const struct GlPixelFormatDesc *format_desc, short *depthSize, short *stencilSize)
{
    TRACE("fmt: %s\n", debug_d3dformat(format_desc->format));
    switch(format_desc->format)
    {
        case WINED3DFMT_D16_LOCKABLE:
        case WINED3DFMT_D16_UNORM:
        case WINED3DFMT_D15S1:
        case WINED3DFMT_D24X8:
        case WINED3DFMT_D24X4S4:
        case WINED3DFMT_D24S8:
        case WINED3DFMT_D24FS8:
        case WINED3DFMT_D32:
        case WINED3DFMT_D32F_LOCKABLE:
            break;
        default:
            FIXME("Unsupported stencil format: %s\n", debug_d3dformat(format_desc->format));
            return FALSE;
    }

    *depthSize = format_desc->depth_size;
    *stencilSize = format_desc->stencil_size;

    TRACE("Returning depthSize: %d and stencilSize: %d for fmt=%s\n",
            *depthSize, *stencilSize, debug_d3dformat(format_desc->format));
    return TRUE;
}

/* DirectDraw stuff */
WINED3DFORMAT pixelformat_for_depth(DWORD depth) {
    switch(depth) {
        case 8:  return WINED3DFMT_P8;
        case 15: return WINED3DFMT_X1R5G5B5;
        case 16: return WINED3DFMT_R5G6B5;
        case 24: return WINED3DFMT_X8R8G8B8; /* Robots needs 24bit to be X8R8G8B8 */
        case 32: return WINED3DFMT_X8R8G8B8; /* EVE online and the Fur demo need 32bit AdapterDisplayMode to return X8R8G8B8 */
        default: return WINED3DFMT_UNKNOWN;
    }
}

void multiply_matrix(WINED3DMATRIX *dest, const WINED3DMATRIX *src1, const WINED3DMATRIX *src2) {
    WINED3DMATRIX temp;

    /* Now do the multiplication 'by hand'.
       I know that all this could be optimised, but this will be done later :-) */
    temp.u.s._11 = (src1->u.s._11 * src2->u.s._11) + (src1->u.s._21 * src2->u.s._12) + (src1->u.s._31 * src2->u.s._13) + (src1->u.s._41 * src2->u.s._14);
    temp.u.s._21 = (src1->u.s._11 * src2->u.s._21) + (src1->u.s._21 * src2->u.s._22) + (src1->u.s._31 * src2->u.s._23) + (src1->u.s._41 * src2->u.s._24);
    temp.u.s._31 = (src1->u.s._11 * src2->u.s._31) + (src1->u.s._21 * src2->u.s._32) + (src1->u.s._31 * src2->u.s._33) + (src1->u.s._41 * src2->u.s._34);
    temp.u.s._41 = (src1->u.s._11 * src2->u.s._41) + (src1->u.s._21 * src2->u.s._42) + (src1->u.s._31 * src2->u.s._43) + (src1->u.s._41 * src2->u.s._44);

    temp.u.s._12 = (src1->u.s._12 * src2->u.s._11) + (src1->u.s._22 * src2->u.s._12) + (src1->u.s._32 * src2->u.s._13) + (src1->u.s._42 * src2->u.s._14);
    temp.u.s._22 = (src1->u.s._12 * src2->u.s._21) + (src1->u.s._22 * src2->u.s._22) + (src1->u.s._32 * src2->u.s._23) + (src1->u.s._42 * src2->u.s._24);
    temp.u.s._32 = (src1->u.s._12 * src2->u.s._31) + (src1->u.s._22 * src2->u.s._32) + (src1->u.s._32 * src2->u.s._33) + (src1->u.s._42 * src2->u.s._34);
    temp.u.s._42 = (src1->u.s._12 * src2->u.s._41) + (src1->u.s._22 * src2->u.s._42) + (src1->u.s._32 * src2->u.s._43) + (src1->u.s._42 * src2->u.s._44);

    temp.u.s._13 = (src1->u.s._13 * src2->u.s._11) + (src1->u.s._23 * src2->u.s._12) + (src1->u.s._33 * src2->u.s._13) + (src1->u.s._43 * src2->u.s._14);
    temp.u.s._23 = (src1->u.s._13 * src2->u.s._21) + (src1->u.s._23 * src2->u.s._22) + (src1->u.s._33 * src2->u.s._23) + (src1->u.s._43 * src2->u.s._24);
    temp.u.s._33 = (src1->u.s._13 * src2->u.s._31) + (src1->u.s._23 * src2->u.s._32) + (src1->u.s._33 * src2->u.s._33) + (src1->u.s._43 * src2->u.s._34);
    temp.u.s._43 = (src1->u.s._13 * src2->u.s._41) + (src1->u.s._23 * src2->u.s._42) + (src1->u.s._33 * src2->u.s._43) + (src1->u.s._43 * src2->u.s._44);

    temp.u.s._14 = (src1->u.s._14 * src2->u.s._11) + (src1->u.s._24 * src2->u.s._12) + (src1->u.s._34 * src2->u.s._13) + (src1->u.s._44 * src2->u.s._14);
    temp.u.s._24 = (src1->u.s._14 * src2->u.s._21) + (src1->u.s._24 * src2->u.s._22) + (src1->u.s._34 * src2->u.s._23) + (src1->u.s._44 * src2->u.s._24);
    temp.u.s._34 = (src1->u.s._14 * src2->u.s._31) + (src1->u.s._24 * src2->u.s._32) + (src1->u.s._34 * src2->u.s._33) + (src1->u.s._44 * src2->u.s._34);
    temp.u.s._44 = (src1->u.s._14 * src2->u.s._41) + (src1->u.s._24 * src2->u.s._42) + (src1->u.s._34 * src2->u.s._43) + (src1->u.s._44 * src2->u.s._44);

    /* And copy the new matrix in the good storage.. */
    memcpy(dest, &temp, 16 * sizeof(float));
}

DWORD get_flexible_vertex_size(DWORD d3dvtVertexType) {
    DWORD size = 0;
    int i;
    int numTextures = (d3dvtVertexType & WINED3DFVF_TEXCOUNT_MASK) >> WINED3DFVF_TEXCOUNT_SHIFT;

    if (d3dvtVertexType & WINED3DFVF_NORMAL) size += 3 * sizeof(float);
    if (d3dvtVertexType & WINED3DFVF_DIFFUSE) size += sizeof(DWORD);
    if (d3dvtVertexType & WINED3DFVF_SPECULAR) size += sizeof(DWORD);
    if (d3dvtVertexType & WINED3DFVF_PSIZE) size += sizeof(DWORD);
    switch (d3dvtVertexType & WINED3DFVF_POSITION_MASK) {
        case WINED3DFVF_XYZ:    size += 3 * sizeof(float); break;
        case WINED3DFVF_XYZRHW: size += 4 * sizeof(float); break;
        case WINED3DFVF_XYZB1:  size += 4 * sizeof(float); break;
        case WINED3DFVF_XYZB2:  size += 5 * sizeof(float); break;
        case WINED3DFVF_XYZB3:  size += 6 * sizeof(float); break;
        case WINED3DFVF_XYZB4:  size += 7 * sizeof(float); break;
        case WINED3DFVF_XYZB5:  size += 8 * sizeof(float); break;
        case WINED3DFVF_XYZW:   size += 4 * sizeof(float); break;
        default: ERR("Unexpected position mask\n");
    }
    for (i = 0; i < numTextures; i++) {
        size += GET_TEXCOORD_SIZE_FROM_FVF(d3dvtVertexType, i) * sizeof(float);
    }

    return size;
}

/***********************************************************************
 * CalculateTexRect
 *
 * Calculates the dimensions of the opengl texture used for blits.
 * Handled oversized opengl textures and updates the source rectangle
 * accordingly
 *
 * Params:
 *  This: Surface to operate on
 *  Rect: Requested rectangle
 *
 * Returns:
 *  TRUE if the texture part can be loaded,
 *  FALSE otherwise
 *
 *********************************************************************/
#define GLINFO_LOCATION This->resource.wineD3DDevice->adapter->gl_info

BOOL CalculateTexRect(IWineD3DSurfaceImpl *This, RECT *Rect, float glTexCoord[4]) {
    int x1 = Rect->left, x2 = Rect->right;
    int y1 = Rect->top, y2 = Rect->bottom;
    GLint maxSize = GL_LIMITS(texture_size);

    TRACE("(%p)->(%d,%d)-(%d,%d)\n", This,
          Rect->left, Rect->top, Rect->right, Rect->bottom);

    /* The sizes might be reversed */
    if(Rect->left > Rect->right) {
        x1 = Rect->right;
        x2 = Rect->left;
    }
    if(Rect->top > Rect->bottom) {
        y1 = Rect->bottom;
        y2 = Rect->top;
    }

    /* No oversized texture? This is easy */
    if(!(This->Flags & SFLAG_OVERSIZE)) {
        /* Which rect from the texture do I need? */
        if(This->glDescription.target == GL_TEXTURE_RECTANGLE_ARB) {
            glTexCoord[0] = (float) Rect->left;
            glTexCoord[2] = (float) Rect->top;
            glTexCoord[1] = (float) Rect->right;
            glTexCoord[3] = (float) Rect->bottom;
        } else {
            glTexCoord[0] = (float) Rect->left / (float) This->pow2Width;
            glTexCoord[2] = (float) Rect->top / (float) This->pow2Height;
            glTexCoord[1] = (float) Rect->right / (float) This->pow2Width;
            glTexCoord[3] = (float) Rect->bottom / (float) This->pow2Height;
        }

        return TRUE;
    } else {
        /* Check if we can succeed at all */
        if( (x2 - x1) > maxSize ||
            (y2 - y1) > maxSize ) {
            TRACE("Requested rectangle is too large for gl\n");
            return FALSE;
        }

        /* A part of the texture has to be picked. First, check if
         * some texture part is loaded already, if yes try to re-use it.
         * If the texture is dirty, or the part can't be used,
         * re-position the part to load
         */
        if(This->Flags & SFLAG_INTEXTURE) {
            if(This->glRect.left <= x1 && This->glRect.right >= x2 &&
               This->glRect.top <= y1 && This->glRect.bottom >= x2 ) {
                /* Ok, the rectangle is ok, re-use it */
                TRACE("Using existing gl Texture\n");
            } else {
                /* Rectangle is not ok, dirtify the texture to reload it */
                TRACE("Dirtifying texture to force reload\n");
                This->Flags &= ~SFLAG_INTEXTURE;
            }
        }

        /* Now if we are dirty(no else if!) */
        if(!(This->Flags & SFLAG_INTEXTURE)) {
            /* Set the new rectangle. Use the following strategy:
             * 1) Use as big textures as possible.
             * 2) Place the texture part in the way that the requested
             *    part is in the middle of the texture(well, almost)
             * 3) If the texture is moved over the edges of the
             *    surface, replace it nicely
             * 4) If the coord is not limiting the texture size,
             *    use the whole size
             */
            if((This->pow2Width) > maxSize) {
                This->glRect.left = x1 - maxSize / 2;
                if(This->glRect.left < 0) {
                    This->glRect.left = 0;
                }
                This->glRect.right = This->glRect.left + maxSize;
                if(This->glRect.right > This->currentDesc.Width) {
                    This->glRect.right = This->currentDesc.Width;
                    This->glRect.left = This->glRect.right - maxSize;
                }
            } else {
                This->glRect.left = 0;
                This->glRect.right = This->pow2Width;
            }

            if(This->pow2Height > maxSize) {
                This->glRect.top = x1 - GL_LIMITS(texture_size) / 2;
                if(This->glRect.top < 0) This->glRect.top = 0;
                This->glRect.bottom = This->glRect.left + maxSize;
                if(This->glRect.bottom > This->currentDesc.Height) {
                    This->glRect.bottom = This->currentDesc.Height;
                    This->glRect.top = This->glRect.bottom - maxSize;
                }
            } else {
                This->glRect.top = 0;
                This->glRect.bottom = This->pow2Height;
            }
            TRACE("(%p): Using rect (%d,%d)-(%d,%d)\n", This,
                   This->glRect.left, This->glRect.top, This->glRect.right, This->glRect.bottom);
        }

        /* Re-calculate the rect to draw */
        Rect->left -= This->glRect.left;
        Rect->right -= This->glRect.left;
        Rect->top -= This->glRect.top;
        Rect->bottom -= This->glRect.top;

        /* Get the gl coordinates. The gl rectangle is a power of 2, eigher the max size,
         * or the pow2Width / pow2Height of the surface.
         *
         * Can never be GL_TEXTURE_RECTANGLE_ARB because oversized surfaces are always set up
         * as regular GL_TEXTURE_2D.
         */
        glTexCoord[0] = (float) Rect->left / (float) (This->glRect.right - This->glRect.left);
        glTexCoord[2] = (float) Rect->top / (float) (This->glRect.bottom - This->glRect.top);
        glTexCoord[1] = (float) Rect->right / (float) (This->glRect.right - This->glRect.left);
        glTexCoord[3] = (float) Rect->bottom / (float) (This->glRect.bottom - This->glRect.top);
    }
    return TRUE;
}
#undef GLINFO_LOCATION

/* Hash table functions */

struct hash_table_t *hash_table_create(hash_function_t *hash_function, compare_function_t *compare_function)
{
    struct hash_table_t *table;
    unsigned int initial_size = 8;

    table = HeapAlloc(GetProcessHeap(), 0, sizeof(struct hash_table_t) + (initial_size * sizeof(struct list)));
    if (!table)
    {
        ERR("Failed to allocate table, returning NULL.\n");
        return NULL;
    }

    table->hash_function = hash_function;
    table->compare_function = compare_function;

    table->grow_size = initial_size - (initial_size >> 2);
    table->shrink_size = 0;

    table->buckets = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, initial_size * sizeof(struct list));
    if (!table->buckets)
    {
        ERR("Failed to allocate table buckets, returning NULL.\n");
        HeapFree(GetProcessHeap(), 0, table);
        return NULL;
    }
    table->bucket_count = initial_size;

    table->entries = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, table->grow_size * sizeof(struct hash_table_entry_t));
    if (!table->entries)
    {
        ERR("Failed to allocate table entries, returning NULL.\n");
        HeapFree(GetProcessHeap(), 0, table->buckets);
        HeapFree(GetProcessHeap(), 0, table);
        return NULL;
    }
    table->entry_count = 0;

    list_init(&table->free_entries);
    table->count = 0;

    return table;
}

void hash_table_destroy(struct hash_table_t *table, void (*free_value)(void *value, void *cb), void *cb)
{
    unsigned int i = 0;

    for (i = 0; i < table->entry_count; ++i)
    {
        if(free_value) {
            free_value(table->entries[i].value, cb);
        }
        HeapFree(GetProcessHeap(), 0, table->entries[i].key);
    }

    HeapFree(GetProcessHeap(), 0, table->entries);
    HeapFree(GetProcessHeap(), 0, table->buckets);
    HeapFree(GetProcessHeap(), 0, table);
}

void hash_table_for_each_entry(struct hash_table_t *table, void (*callback)(void *value, void *context), void *context)
{
    unsigned int i = 0;

    for (i = 0; i < table->entry_count; ++i)
    {
        callback(table->entries[i].value, context);
    }
}

static inline struct hash_table_entry_t *hash_table_get_by_idx(const struct hash_table_t *table, const void *key,
        unsigned int idx)
{
    struct hash_table_entry_t *entry;

    if (table->buckets[idx].next)
        LIST_FOR_EACH_ENTRY(entry, &(table->buckets[idx]), struct hash_table_entry_t, entry)
            if (table->compare_function(entry->key, key)) return entry;

    return NULL;
}

static BOOL hash_table_resize(struct hash_table_t *table, unsigned int new_bucket_count)
{
    unsigned int new_entry_count = 0;
    struct hash_table_entry_t *new_entries;
    struct list *new_buckets;
    unsigned int grow_size = new_bucket_count - (new_bucket_count >> 2);
    unsigned int i;

    new_buckets = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, new_bucket_count * sizeof(struct list));
    if (!new_buckets)
    {
        ERR("Failed to allocate new buckets, returning FALSE.\n");
        return FALSE;
    }

    new_entries = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, grow_size * sizeof(struct hash_table_entry_t));
    if (!new_entries)
    {
        ERR("Failed to allocate new entries, returning FALSE.\n");
        HeapFree(GetProcessHeap(), 0, new_buckets);
        return FALSE;
    }

    for (i = 0; i < table->bucket_count; ++i)
    {
        if (table->buckets[i].next)
        {
            struct hash_table_entry_t *entry, *entry2;

            LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &table->buckets[i], struct hash_table_entry_t, entry)
            {
                int j;
                struct hash_table_entry_t *new_entry = new_entries + (new_entry_count++);
                *new_entry = *entry;

                j = new_entry->hash & (new_bucket_count - 1);

                if (!new_buckets[j].next) list_init(&new_buckets[j]);
                list_add_head(&new_buckets[j], &new_entry->entry);
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, table->buckets);
    table->buckets = new_buckets;

    HeapFree(GetProcessHeap(), 0, table->entries);
    table->entries = new_entries;

    table->entry_count = new_entry_count;
    list_init(&table->free_entries);

    table->bucket_count = new_bucket_count;
    table->grow_size = grow_size;
    table->shrink_size = new_bucket_count > 8 ? new_bucket_count >> 2 : 0;

    return TRUE;
}

void hash_table_put(struct hash_table_t *table, void *key, void *value)
{
    unsigned int idx;
    unsigned int hash;
    struct hash_table_entry_t *entry;

    hash = table->hash_function(key);
    idx = hash & (table->bucket_count - 1);
    entry = hash_table_get_by_idx(table, key, idx);

    if (entry)
    {
        HeapFree(GetProcessHeap(), 0, key);
        entry->value = value;

        if (!value)
        {
            HeapFree(GetProcessHeap(), 0, entry->key);
            entry->key = NULL;

            /* Remove the entry */
            list_remove(&entry->entry);
            list_add_head(&table->free_entries, &entry->entry);

            --table->count;

            /* Shrink if necessary */
            if (table->count < table->shrink_size) {
                if (!hash_table_resize(table, table->bucket_count >> 1))
                {
                    ERR("Failed to shrink the table...\n");
                }
            }
        }

        return;
    }

    if (!value) return;

    /* Grow if necessary */
    if (table->count >= table->grow_size)
    {
        if (!hash_table_resize(table, table->bucket_count << 1))
        {
            ERR("Failed to grow the table, returning.\n");
            return;
        }

        idx = hash & (table->bucket_count - 1);
    }

    /* Find an entry to insert */
    if (!list_empty(&table->free_entries))
    {
        struct list *elem = list_head(&table->free_entries);

        list_remove(elem);
        entry = LIST_ENTRY(elem, struct hash_table_entry_t, entry);
    } else {
        entry = table->entries + (table->entry_count++);
    }

    /* Insert the entry */
    entry->key = key;
    entry->value = value;
    entry->hash = hash;
    if (!table->buckets[idx].next) list_init(&table->buckets[idx]);
    list_add_head(&table->buckets[idx], &entry->entry);

    ++table->count;
}

void hash_table_remove(struct hash_table_t *table, void *key)
{
    hash_table_put(table, key, NULL);
}

void *hash_table_get(const struct hash_table_t *table, const void *key)
{
    unsigned int idx;
    struct hash_table_entry_t *entry;

    idx = table->hash_function(key) & (table->bucket_count - 1);
    entry = hash_table_get_by_idx(table, key, idx);

    return entry ? entry->value : NULL;
}

#define GLINFO_LOCATION stateblock->wineD3DDevice->adapter->gl_info
void gen_ffp_frag_op(IWineD3DStateBlockImpl *stateblock, struct ffp_frag_settings *settings, BOOL ignore_textype) {
#define ARG1 0x01
#define ARG2 0x02
#define ARG0 0x04
    static const unsigned char args[WINED3DTOP_LERP + 1] = {
        /* undefined                        */  0,
        /* D3DTOP_DISABLE                   */  0,
        /* D3DTOP_SELECTARG1                */  ARG1,
        /* D3DTOP_SELECTARG2                */  ARG2,
        /* D3DTOP_MODULATE                  */  ARG1 | ARG2,
        /* D3DTOP_MODULATE2X                */  ARG1 | ARG2,
        /* D3DTOP_MODULATE4X                */  ARG1 | ARG2,
        /* D3DTOP_ADD                       */  ARG1 | ARG2,
        /* D3DTOP_ADDSIGNED                 */  ARG1 | ARG2,
        /* D3DTOP_ADDSIGNED2X               */  ARG1 | ARG2,
        /* D3DTOP_SUBTRACT                  */  ARG1 | ARG2,
        /* D3DTOP_ADDSMOOTH                 */  ARG1 | ARG2,
        /* D3DTOP_BLENDDIFFUSEALPHA         */  ARG1 | ARG2,
        /* D3DTOP_BLENDTEXTUREALPHA         */  ARG1 | ARG2,
        /* D3DTOP_BLENDFACTORALPHA          */  ARG1 | ARG2,
        /* D3DTOP_BLENDTEXTUREALPHAPM       */  ARG1 | ARG2,
        /* D3DTOP_BLENDCURRENTALPHA         */  ARG1 | ARG2,
        /* D3DTOP_PREMODULATE               */  ARG1 | ARG2,
        /* D3DTOP_MODULATEALPHA_ADDCOLOR    */  ARG1 | ARG2,
        /* D3DTOP_MODULATECOLOR_ADDALPHA    */  ARG1 | ARG2,
        /* D3DTOP_MODULATEINVALPHA_ADDCOLOR */  ARG1 | ARG2,
        /* D3DTOP_MODULATEINVCOLOR_ADDALPHA */  ARG1 | ARG2,
        /* D3DTOP_BUMPENVMAP                */  ARG1 | ARG2,
        /* D3DTOP_BUMPENVMAPLUMINANCE       */  ARG1 | ARG2,
        /* D3DTOP_DOTPRODUCT3               */  ARG1 | ARG2,
        /* D3DTOP_MULTIPLYADD               */  ARG1 | ARG2 | ARG0,
        /* D3DTOP_LERP                      */  ARG1 | ARG2 | ARG0
    };
    unsigned int i;
    DWORD ttff;
    DWORD cop, aop, carg0, carg1, carg2, aarg0, aarg1, aarg2;

    for(i = 0; i < GL_LIMITS(texture_stages); i++) {
        IWineD3DBaseTextureImpl *texture;
        settings->op[i].padding = 0;
        if(stateblock->textureState[i][WINED3DTSS_COLOROP] == WINED3DTOP_DISABLE) {
            settings->op[i].cop = WINED3DTOP_DISABLE;
            settings->op[i].aop = WINED3DTOP_DISABLE;
            settings->op[i].carg0 = settings->op[i].carg1 = settings->op[i].carg2 = ARG_UNUSED;
            settings->op[i].aarg0 = settings->op[i].aarg1 = settings->op[i].aarg2 = ARG_UNUSED;
            settings->op[i].color_fixup = COLOR_FIXUP_IDENTITY;
            settings->op[i].dst = resultreg;
            settings->op[i].tex_type = tex_1d;
            settings->op[i].projected = proj_none;
            i++;
            break;
        }

        texture = (IWineD3DBaseTextureImpl *) stateblock->textures[i];
        if(texture) {
            settings->op[i].color_fixup = texture->resource.format_desc->color_fixup;
            if(ignore_textype) {
                settings->op[i].tex_type = tex_1d;
            } else {
                switch (IWineD3DBaseTexture_GetTextureDimensions((IWineD3DBaseTexture *)texture)) {
                    case GL_TEXTURE_1D:
                        settings->op[i].tex_type = tex_1d;
                        break;
                    case GL_TEXTURE_2D:
                        settings->op[i].tex_type = tex_2d;
                        break;
                    case GL_TEXTURE_3D:
                        settings->op[i].tex_type = tex_3d;
                        break;
                    case GL_TEXTURE_CUBE_MAP_ARB:
                        settings->op[i].tex_type = tex_cube;
                        break;
                    case GL_TEXTURE_RECTANGLE_ARB:
                        settings->op[i].tex_type = tex_rect;
                        break;
                }
            }
        } else {
            settings->op[i].color_fixup = COLOR_FIXUP_IDENTITY;
            settings->op[i].tex_type = tex_1d;
        }

        cop = stateblock->textureState[i][WINED3DTSS_COLOROP];
        aop = stateblock->textureState[i][WINED3DTSS_ALPHAOP];

        carg1 = (args[cop] & ARG1) ? stateblock->textureState[i][WINED3DTSS_COLORARG1] : ARG_UNUSED;
        carg2 = (args[cop] & ARG2) ? stateblock->textureState[i][WINED3DTSS_COLORARG2] : ARG_UNUSED;
        carg0 = (args[cop] & ARG0) ? stateblock->textureState[i][WINED3DTSS_COLORARG0] : ARG_UNUSED;

        if(is_invalid_op(stateblock->wineD3DDevice, i, cop,
                         carg1, carg2, carg0)) {
            carg0 = ARG_UNUSED;
            carg2 = ARG_UNUSED;
            carg1 = WINED3DTA_CURRENT;
            cop = WINED3DTOP_SELECTARG1;
        }

        if(cop == WINED3DTOP_DOTPRODUCT3) {
            /* A dotproduct3 on the colorop overwrites the alphaop operation and replicates
             * the color result to the alpha component of the destination
             */
            aop = cop;
            aarg1 = carg1;
            aarg2 = carg2;
            aarg0 = carg0;
        } else {
            aarg1 = (args[aop] & ARG1) ? stateblock->textureState[i][WINED3DTSS_ALPHAARG1] : ARG_UNUSED;
            aarg2 = (args[aop] & ARG2) ? stateblock->textureState[i][WINED3DTSS_ALPHAARG2] : ARG_UNUSED;
            aarg0 = (args[aop] & ARG0) ? stateblock->textureState[i][WINED3DTSS_ALPHAARG0] : ARG_UNUSED;
        }

        if (i == 0 && stateblock->textures[0] && stateblock->renderState[WINED3DRS_COLORKEYENABLE])
        {
            UINT texture_dimensions = IWineD3DBaseTexture_GetTextureDimensions(stateblock->textures[0]);

            if (texture_dimensions == GL_TEXTURE_2D || texture_dimensions == GL_TEXTURE_RECTANGLE_ARB)
            {
                IWineD3DSurfaceImpl *surf;
                surf = (IWineD3DSurfaceImpl *) ((IWineD3DTextureImpl *) stateblock->textures[0])->surfaces[0];

                if (surf->CKeyFlags & WINEDDSD_CKSRCBLT && !surf->resource.format_desc->alpha_mask)
                {
                    if (aop == WINED3DTOP_DISABLE)
                    {
                       aarg1 = WINED3DTA_TEXTURE;
                       aop = WINED3DTOP_SELECTARG1;
                    }
                    else if (aop == WINED3DTOP_SELECTARG1 && aarg1 != WINED3DTA_TEXTURE)
                    {
                        if (stateblock->renderState[WINED3DRS_ALPHABLENDENABLE])
                        {
                            aarg2 = WINED3DTA_TEXTURE;
                            aop = WINED3DTOP_MODULATE;
                        }
                        else aarg1 = WINED3DTA_TEXTURE;
                    }
                    else if (aop == WINED3DTOP_SELECTARG2 && aarg2 != WINED3DTA_TEXTURE)
                    {
                        if (stateblock->renderState[WINED3DRS_ALPHABLENDENABLE])
                        {
                            aarg1 = WINED3DTA_TEXTURE;
                            aop = WINED3DTOP_MODULATE;
                        }
                        else aarg2 = WINED3DTA_TEXTURE;
                    }
                }
            }
        }

        if(is_invalid_op(stateblock->wineD3DDevice, i, aop,
           aarg1, aarg2, aarg0)) {
               aarg0 = ARG_UNUSED;
               aarg2 = ARG_UNUSED;
               aarg1 = WINED3DTA_CURRENT;
               aop = WINED3DTOP_SELECTARG1;
        }

        if(carg1 == WINED3DTA_TEXTURE || carg2 == WINED3DTA_TEXTURE || carg0 == WINED3DTA_TEXTURE ||
           aarg1 == WINED3DTA_TEXTURE || aarg2 == WINED3DTA_TEXTURE || aarg0 == WINED3DTA_TEXTURE) {
            ttff = stateblock->textureState[i][WINED3DTSS_TEXTURETRANSFORMFLAGS];
            if(ttff == (WINED3DTTFF_PROJECTED | WINED3DTTFF_COUNT3)) {
                settings->op[i].projected = proj_count3;
            } else if(ttff == (WINED3DTTFF_PROJECTED | WINED3DTTFF_COUNT4)) {
                settings->op[i].projected = proj_count4;
            } else {
                settings->op[i].projected = proj_none;
            }
        } else {
            settings->op[i].projected = proj_none;
        }

        settings->op[i].cop = cop;
        settings->op[i].aop = aop;
        settings->op[i].carg0 = carg0;
        settings->op[i].carg1 = carg1;
        settings->op[i].carg2 = carg2;
        settings->op[i].aarg0 = aarg0;
        settings->op[i].aarg1 = aarg1;
        settings->op[i].aarg2 = aarg2;

        if(stateblock->textureState[i][WINED3DTSS_RESULTARG] == WINED3DTA_TEMP) {
            settings->op[i].dst = tempreg;
        } else {
            settings->op[i].dst = resultreg;
        }
    }

    /* Clear unsupported stages */
    for(; i < MAX_TEXTURES; i++) {
        memset(&settings->op[i], 0xff, sizeof(settings->op[i]));
    }

    if(stateblock->renderState[WINED3DRS_FOGENABLE] == FALSE) {
        settings->fog = FOG_OFF;
    } else if(stateblock->renderState[WINED3DRS_FOGTABLEMODE] == WINED3DFOG_NONE) {
        if(use_vs(stateblock) || ((IWineD3DVertexDeclarationImpl *) stateblock->vertexDecl)->position_transformed) {
            settings->fog = FOG_LINEAR;
        } else {
            switch(stateblock->renderState[WINED3DRS_FOGVERTEXMODE]) {
                case WINED3DFOG_NONE:
                case WINED3DFOG_LINEAR:
                    settings->fog = FOG_LINEAR;
                    break;
                case WINED3DFOG_EXP:
                    settings->fog = FOG_EXP;
                    break;
                case WINED3DFOG_EXP2:
                    settings->fog = FOG_EXP2;
                    break;
            }
        }
    } else {
        switch(stateblock->renderState[WINED3DRS_FOGTABLEMODE]) {
            case WINED3DFOG_LINEAR:
                settings->fog = FOG_LINEAR;
                break;
            case WINED3DFOG_EXP:
                settings->fog = FOG_EXP;
                break;
            case WINED3DFOG_EXP2:
                settings->fog = FOG_EXP2;
                break;
        }
    }
    if(stateblock->renderState[WINED3DRS_SRGBWRITEENABLE]) {
        settings->sRGB_write = 1;
    } else {
        settings->sRGB_write = 0;
    }
}
#undef GLINFO_LOCATION

const struct ffp_frag_desc *find_ffp_frag_shader(const struct hash_table_t *fragment_shaders,
        const struct ffp_frag_settings *settings)
{
    return hash_table_get(fragment_shaders, settings);
}

void add_ffp_frag_shader(struct hash_table_t *shaders, struct ffp_frag_desc *desc) {
    struct ffp_frag_settings *key = HeapAlloc(GetProcessHeap(), 0, sizeof(*key));
    /* Note that the key is the implementation independent part of the ffp_frag_desc structure,
     * whereas desc points to an extended structure with implementation specific parts.
     * Make a copy of the key because hash_table_put takes ownership of it
     */
    *key = desc->settings;
    hash_table_put(shaders, key, desc);
}

/* Activates the texture dimension according to the bound D3D texture.
 * Does not care for the colorop or correct gl texture unit(when using nvrc)
 * Requires the caller to activate the correct unit before
 */
#define GLINFO_LOCATION stateblock->wineD3DDevice->adapter->gl_info
void texture_activate_dimensions(DWORD stage, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(stateblock->textures[stage]) {
        switch (IWineD3DBaseTexture_GetTextureDimensions(stateblock->textures[stage])) {
            case GL_TEXTURE_2D:
                glDisable(GL_TEXTURE_3D);
                checkGLcall("glDisable(GL_TEXTURE_3D)");
                if(GL_SUPPORT(ARB_TEXTURE_CUBE_MAP)) {
                    glDisable(GL_TEXTURE_CUBE_MAP_ARB);
                    checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
                }
                if(GL_SUPPORT(ARB_TEXTURE_RECTANGLE)) {
                    glDisable(GL_TEXTURE_RECTANGLE_ARB);
                    checkGLcall("glDisable(GL_TEXTURE_RECTANGLE_ARB)");
                }
                glEnable(GL_TEXTURE_2D);
                checkGLcall("glEnable(GL_TEXTURE_2D)");
                break;
            case GL_TEXTURE_RECTANGLE_ARB:
                glDisable(GL_TEXTURE_2D);
                checkGLcall("glDisable(GL_TEXTURE_2D)");
                glDisable(GL_TEXTURE_3D);
                checkGLcall("glDisable(GL_TEXTURE_3D)");
                if(GL_SUPPORT(ARB_TEXTURE_CUBE_MAP)) {
                    glDisable(GL_TEXTURE_CUBE_MAP_ARB);
                    checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
                }
                glEnable(GL_TEXTURE_RECTANGLE_ARB);
                checkGLcall("glEnable(GL_TEXTURE_RECTANGLE_ARB)");
                break;
            case GL_TEXTURE_3D:
                if(GL_SUPPORT(ARB_TEXTURE_CUBE_MAP)) {
                    glDisable(GL_TEXTURE_CUBE_MAP_ARB);
                    checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
                }
                if(GL_SUPPORT(ARB_TEXTURE_RECTANGLE)) {
                    glDisable(GL_TEXTURE_RECTANGLE_ARB);
                    checkGLcall("glDisable(GL_TEXTURE_RECTANGLE_ARB)");
                }
                glDisable(GL_TEXTURE_2D);
                checkGLcall("glDisable(GL_TEXTURE_2D)");
                glEnable(GL_TEXTURE_3D);
                checkGLcall("glEnable(GL_TEXTURE_3D)");
                break;
            case GL_TEXTURE_CUBE_MAP_ARB:
                glDisable(GL_TEXTURE_2D);
                checkGLcall("glDisable(GL_TEXTURE_2D)");
                glDisable(GL_TEXTURE_3D);
                checkGLcall("glDisable(GL_TEXTURE_3D)");
                if(GL_SUPPORT(ARB_TEXTURE_RECTANGLE)) {
                    glDisable(GL_TEXTURE_RECTANGLE_ARB);
                    checkGLcall("glDisable(GL_TEXTURE_RECTANGLE_ARB)");
                }
                glEnable(GL_TEXTURE_CUBE_MAP_ARB);
                checkGLcall("glEnable(GL_TEXTURE_CUBE_MAP_ARB)");
              break;
        }
    } else {
        glEnable(GL_TEXTURE_2D);
        checkGLcall("glEnable(GL_TEXTURE_2D)");
        glDisable(GL_TEXTURE_3D);
        checkGLcall("glDisable(GL_TEXTURE_3D)");
        if(GL_SUPPORT(ARB_TEXTURE_CUBE_MAP)) {
            glDisable(GL_TEXTURE_CUBE_MAP_ARB);
            checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
        }
        if(GL_SUPPORT(ARB_TEXTURE_RECTANGLE)) {
            glDisable(GL_TEXTURE_RECTANGLE_ARB);
            checkGLcall("glDisable(GL_TEXTURE_RECTANGLE_ARB)");
        }
        /* Binding textures is done by samplers. A dummy texture will be bound */
    }
}

void sampler_texdim(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    DWORD sampler = state - STATE_SAMPLER(0);
    DWORD mapped_stage = stateblock->wineD3DDevice->texUnitMap[sampler];

    /* No need to enable / disable anything here for unused samplers. The tex_colorop
    * handler takes care. Also no action is needed with pixel shaders, or if tex_colorop
    * will take care of this business
    */
    if(mapped_stage == WINED3D_UNMAPPED_STAGE || mapped_stage >= GL_LIMITS(textures)) return;
    if(sampler >= stateblock->lowest_disabled_stage) return;
    if(isStateDirty(context, STATE_TEXTURESTAGE(sampler, WINED3DTSS_COLOROP))) return;

    texture_activate_dimensions(sampler, stateblock, context);
}
#undef GLINFO_LOCATION

unsigned int ffp_frag_program_key_hash(const void *key)
{
    const struct ffp_frag_settings *k = key;
    unsigned int hash = 0, i;
    const DWORD *blob;

    /* This takes the texture op settings of stage 0 and 1 into account.
     * how exactly depends on the memory laybout of the compiler, but it
     * should not matter too much. Stages > 1 are used rarely, so there's
     * no need to process them. Even if they're used it is likely that
     * the ffp setup has distinct stage 0 and 1 settings.
     */
    for(i = 0; i < 2; i++) {
        blob = (const DWORD *)&k->op[i];
        hash ^= blob[0] ^ blob[1];
    }

    hash += ~(hash << 15);
    hash ^=  (hash >> 10);
    hash +=  (hash << 3);
    hash ^=  (hash >> 6);
    hash += ~(hash << 11);
    hash ^=  (hash >> 16);

    return hash;
}

BOOL ffp_frag_program_key_compare(const void *keya, const void *keyb)
{
    const struct ffp_frag_settings *ka = keya;
    const struct ffp_frag_settings *kb = keyb;

    return memcmp(ka, kb, sizeof(*ka)) == 0;
}

UINT wined3d_log2i(UINT32 x)
{
    static const BYTE l[] =
    {
        0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    };
    UINT32 i;

    return (i = x >> 16) ? (x = i >> 8) ? l[x] + 24 : l[i] + 16 : (i = x >> 8) ? l[i] + 8 : l[x];
}
