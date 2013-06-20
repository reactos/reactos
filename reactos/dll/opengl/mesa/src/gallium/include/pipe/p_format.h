/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright (c) 2008 VMware, Inc.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef PIPE_FORMAT_H
#define PIPE_FORMAT_H

#ifdef __cplusplus
extern "C" {
#endif


enum pipe_type {
   PIPE_TYPE_UNORM = 0,
   PIPE_TYPE_SNORM,
   PIPE_TYPE_SINT,
   PIPE_TYPE_UINT,
   PIPE_TYPE_FLOAT,
   PIPE_TYPE_COUNT
};

/**
 * Texture/surface image formats (preliminary)
 */

/* KW: Added lots of surface formats to support vertex element layout
 * definitions, and eventually render-to-vertex-buffer.
 */

enum pipe_format {
   PIPE_FORMAT_NONE                    = 0,
   PIPE_FORMAT_B8G8R8A8_UNORM          = 1,
   PIPE_FORMAT_B8G8R8X8_UNORM          = 2,
   PIPE_FORMAT_A8R8G8B8_UNORM          = 3,
   PIPE_FORMAT_X8R8G8B8_UNORM          = 4,
   PIPE_FORMAT_B5G5R5A1_UNORM          = 5,
   PIPE_FORMAT_B4G4R4A4_UNORM          = 6,
   PIPE_FORMAT_B5G6R5_UNORM            = 7,
   PIPE_FORMAT_R10G10B10A2_UNORM       = 8,
   PIPE_FORMAT_L8_UNORM                = 9,    /**< ubyte luminance */
   PIPE_FORMAT_A8_UNORM                = 10,   /**< ubyte alpha */
   PIPE_FORMAT_I8_UNORM                = 11,   /**< ubyte intensity */
   PIPE_FORMAT_L8A8_UNORM              = 12,   /**< ubyte alpha, luminance */
   PIPE_FORMAT_L16_UNORM               = 13,   /**< ushort luminance */
   PIPE_FORMAT_UYVY                    = 14,
   PIPE_FORMAT_YUYV                    = 15,
   PIPE_FORMAT_Z16_UNORM               = 16,
   PIPE_FORMAT_Z32_UNORM               = 17,
   PIPE_FORMAT_Z32_FLOAT               = 18,
   PIPE_FORMAT_Z24_UNORM_S8_UINT       = 19,
   PIPE_FORMAT_S8_UINT_Z24_UNORM       = 20,
   PIPE_FORMAT_Z24X8_UNORM             = 21,
   PIPE_FORMAT_X8Z24_UNORM             = 22,
   PIPE_FORMAT_S8_UINT                 = 23,   /**< ubyte stencil */
   PIPE_FORMAT_R64_FLOAT               = 24,
   PIPE_FORMAT_R64G64_FLOAT            = 25,
   PIPE_FORMAT_R64G64B64_FLOAT         = 26,
   PIPE_FORMAT_R64G64B64A64_FLOAT      = 27,
   PIPE_FORMAT_R32_FLOAT               = 28,
   PIPE_FORMAT_R32G32_FLOAT            = 29,
   PIPE_FORMAT_R32G32B32_FLOAT         = 30,
   PIPE_FORMAT_R32G32B32A32_FLOAT      = 31,
   PIPE_FORMAT_R32_UNORM               = 32,
   PIPE_FORMAT_R32G32_UNORM            = 33,
   PIPE_FORMAT_R32G32B32_UNORM         = 34,
   PIPE_FORMAT_R32G32B32A32_UNORM      = 35,
   PIPE_FORMAT_R32_USCALED             = 36,
   PIPE_FORMAT_R32G32_USCALED          = 37,
   PIPE_FORMAT_R32G32B32_USCALED       = 38,
   PIPE_FORMAT_R32G32B32A32_USCALED    = 39,
   PIPE_FORMAT_R32_SNORM               = 40,
   PIPE_FORMAT_R32G32_SNORM            = 41,
   PIPE_FORMAT_R32G32B32_SNORM         = 42,
   PIPE_FORMAT_R32G32B32A32_SNORM      = 43,
   PIPE_FORMAT_R32_SSCALED             = 44,
   PIPE_FORMAT_R32G32_SSCALED          = 45,
   PIPE_FORMAT_R32G32B32_SSCALED       = 46,
   PIPE_FORMAT_R32G32B32A32_SSCALED    = 47,
   PIPE_FORMAT_R16_UNORM               = 48,
   PIPE_FORMAT_R16G16_UNORM            = 49,
   PIPE_FORMAT_R16G16B16_UNORM         = 50,
   PIPE_FORMAT_R16G16B16A16_UNORM      = 51,
   PIPE_FORMAT_R16_USCALED             = 52,
   PIPE_FORMAT_R16G16_USCALED          = 53,
   PIPE_FORMAT_R16G16B16_USCALED       = 54,
   PIPE_FORMAT_R16G16B16A16_USCALED    = 55,
   PIPE_FORMAT_R16_SNORM               = 56,
   PIPE_FORMAT_R16G16_SNORM            = 57,
   PIPE_FORMAT_R16G16B16_SNORM         = 58,
   PIPE_FORMAT_R16G16B16A16_SNORM      = 59,
   PIPE_FORMAT_R16_SSCALED             = 60,
   PIPE_FORMAT_R16G16_SSCALED          = 61,
   PIPE_FORMAT_R16G16B16_SSCALED       = 62,
   PIPE_FORMAT_R16G16B16A16_SSCALED    = 63,
   PIPE_FORMAT_R8_UNORM                = 64,
   PIPE_FORMAT_R8G8_UNORM              = 65,
   PIPE_FORMAT_R8G8B8_UNORM            = 66,
   PIPE_FORMAT_R8G8B8A8_UNORM          = 67,
   PIPE_FORMAT_X8B8G8R8_UNORM          = 68,
   PIPE_FORMAT_R8_USCALED              = 69,
   PIPE_FORMAT_R8G8_USCALED            = 70,
   PIPE_FORMAT_R8G8B8_USCALED          = 71,
   PIPE_FORMAT_R8G8B8A8_USCALED        = 72,
   PIPE_FORMAT_R8_SNORM                = 74,
   PIPE_FORMAT_R8G8_SNORM              = 75,
   PIPE_FORMAT_R8G8B8_SNORM            = 76,
   PIPE_FORMAT_R8G8B8A8_SNORM          = 77,
   PIPE_FORMAT_R8_SSCALED              = 82,
   PIPE_FORMAT_R8G8_SSCALED            = 83,
   PIPE_FORMAT_R8G8B8_SSCALED          = 84,
   PIPE_FORMAT_R8G8B8A8_SSCALED        = 85,
   PIPE_FORMAT_R32_FIXED               = 87,
   PIPE_FORMAT_R32G32_FIXED            = 88,
   PIPE_FORMAT_R32G32B32_FIXED         = 89,
   PIPE_FORMAT_R32G32B32A32_FIXED      = 90,
   PIPE_FORMAT_R16_FLOAT               = 91,
   PIPE_FORMAT_R16G16_FLOAT            = 92,
   PIPE_FORMAT_R16G16B16_FLOAT         = 93,
   PIPE_FORMAT_R16G16B16A16_FLOAT      = 94,

   /* sRGB formats */
   PIPE_FORMAT_L8_SRGB                 = 95,
   PIPE_FORMAT_L8A8_SRGB               = 96,
   PIPE_FORMAT_R8G8B8_SRGB             = 97,
   PIPE_FORMAT_A8B8G8R8_SRGB           = 98,
   PIPE_FORMAT_X8B8G8R8_SRGB           = 99,
   PIPE_FORMAT_B8G8R8A8_SRGB           = 100,
   PIPE_FORMAT_B8G8R8X8_SRGB           = 101,
   PIPE_FORMAT_A8R8G8B8_SRGB           = 102,
   PIPE_FORMAT_X8R8G8B8_SRGB           = 103,
   PIPE_FORMAT_R8G8B8A8_SRGB           = 104,

   /* compressed formats */
   PIPE_FORMAT_DXT1_RGB                = 105,
   PIPE_FORMAT_DXT1_RGBA               = 106,
   PIPE_FORMAT_DXT3_RGBA               = 107,
   PIPE_FORMAT_DXT5_RGBA               = 108,

   /* sRGB, compressed */
   PIPE_FORMAT_DXT1_SRGB               = 109,
   PIPE_FORMAT_DXT1_SRGBA              = 110,
   PIPE_FORMAT_DXT3_SRGBA              = 111,
   PIPE_FORMAT_DXT5_SRGBA              = 112,

   /* rgtc compressed */
   PIPE_FORMAT_RGTC1_UNORM             = 113,
   PIPE_FORMAT_RGTC1_SNORM             = 114,
   PIPE_FORMAT_RGTC2_UNORM             = 115,
   PIPE_FORMAT_RGTC2_SNORM             = 116,

   PIPE_FORMAT_R8G8_B8G8_UNORM         = 117,
   PIPE_FORMAT_G8R8_G8B8_UNORM         = 118,

   /* mixed formats */
   PIPE_FORMAT_R8SG8SB8UX8U_NORM       = 119,
   PIPE_FORMAT_R5SG5SB6U_NORM          = 120,

   /* TODO: re-order these */
   PIPE_FORMAT_A8B8G8R8_UNORM          = 121,
   PIPE_FORMAT_B5G5R5X1_UNORM          = 122,
   PIPE_FORMAT_R10G10B10A2_USCALED     = 123,
   PIPE_FORMAT_R11G11B10_FLOAT         = 124,
   PIPE_FORMAT_R9G9B9E5_FLOAT          = 125,
   PIPE_FORMAT_Z32_FLOAT_S8X24_UINT    = 126,
   PIPE_FORMAT_R1_UNORM                = 127,
   PIPE_FORMAT_R10G10B10X2_USCALED     = 128,
   PIPE_FORMAT_R10G10B10X2_SNORM       = 129,
   PIPE_FORMAT_L4A4_UNORM              = 130,
   PIPE_FORMAT_B10G10R10A2_UNORM       = 131,
   PIPE_FORMAT_R10SG10SB10SA2U_NORM    = 132,
   PIPE_FORMAT_R8G8Bx_SNORM            = 133,
   PIPE_FORMAT_R8G8B8X8_UNORM          = 134,
   PIPE_FORMAT_B4G4R4X4_UNORM          = 135,

   /* some stencil samplers formats */
   PIPE_FORMAT_X24S8_UINT              = 136,
   PIPE_FORMAT_S8X24_UINT              = 137,
   PIPE_FORMAT_X32_S8X24_UINT          = 138,

   PIPE_FORMAT_B2G3R3_UNORM            = 139,
   PIPE_FORMAT_L16A16_UNORM            = 140,
   PIPE_FORMAT_A16_UNORM               = 141,
   PIPE_FORMAT_I16_UNORM               = 142,

   PIPE_FORMAT_LATC1_UNORM             = 143,
   PIPE_FORMAT_LATC1_SNORM             = 144,
   PIPE_FORMAT_LATC2_UNORM             = 145,
   PIPE_FORMAT_LATC2_SNORM             = 146,

   PIPE_FORMAT_A8_SNORM                = 147,
   PIPE_FORMAT_L8_SNORM                = 148,
   PIPE_FORMAT_L8A8_SNORM              = 149,
   PIPE_FORMAT_I8_SNORM                = 150,
   PIPE_FORMAT_A16_SNORM               = 151,
   PIPE_FORMAT_L16_SNORM               = 152,
   PIPE_FORMAT_L16A16_SNORM            = 153,
   PIPE_FORMAT_I16_SNORM               = 154,

   PIPE_FORMAT_A16_FLOAT               = 155,
   PIPE_FORMAT_L16_FLOAT               = 156,
   PIPE_FORMAT_L16A16_FLOAT            = 157,
   PIPE_FORMAT_I16_FLOAT               = 158,
   PIPE_FORMAT_A32_FLOAT               = 159,
   PIPE_FORMAT_L32_FLOAT               = 160,
   PIPE_FORMAT_L32A32_FLOAT            = 161,
   PIPE_FORMAT_I32_FLOAT               = 162,

   PIPE_FORMAT_YV12                    = 163,
   PIPE_FORMAT_YV16                    = 164,
   PIPE_FORMAT_IYUV                    = 165,  /**< aka I420 */
   PIPE_FORMAT_NV12                    = 166,
   PIPE_FORMAT_NV21                    = 167,
   PIPE_FORMAT_AYUV                    = PIPE_FORMAT_A8R8G8B8_UNORM,
   PIPE_FORMAT_VUYA                    = PIPE_FORMAT_B8G8R8A8_UNORM,
   PIPE_FORMAT_XYUV                    = PIPE_FORMAT_X8R8G8B8_UNORM,
   PIPE_FORMAT_VUYX                    = PIPE_FORMAT_B8G8R8X8_UNORM,

   PIPE_FORMAT_R4A4_UNORM              = 168,
   PIPE_FORMAT_A4R4_UNORM              = 169,
   PIPE_FORMAT_R8A8_UNORM              = 170,
   PIPE_FORMAT_A8R8_UNORM              = 171,

   PIPE_FORMAT_R10G10B10A2_SSCALED     = 172,
   PIPE_FORMAT_R10G10B10A2_SNORM       = 173,

   PIPE_FORMAT_B10G10R10A2_USCALED     = 174,
   PIPE_FORMAT_B10G10R10A2_SSCALED     = 175,
   PIPE_FORMAT_B10G10R10A2_SNORM       = 176,

   PIPE_FORMAT_R8_UINT                 = 177,
   PIPE_FORMAT_R8G8_UINT               = 178,
   PIPE_FORMAT_R8G8B8_UINT             = 179,
   PIPE_FORMAT_R8G8B8A8_UINT           = 180,

   PIPE_FORMAT_R8_SINT                 = 181,
   PIPE_FORMAT_R8G8_SINT               = 182,
   PIPE_FORMAT_R8G8B8_SINT             = 183,
   PIPE_FORMAT_R8G8B8A8_SINT           = 184,

   PIPE_FORMAT_R16_UINT                = 185,
   PIPE_FORMAT_R16G16_UINT             = 186,
   PIPE_FORMAT_R16G16B16_UINT          = 187,
   PIPE_FORMAT_R16G16B16A16_UINT       = 188,

   PIPE_FORMAT_R16_SINT                = 189,
   PIPE_FORMAT_R16G16_SINT             = 190,
   PIPE_FORMAT_R16G16B16_SINT          = 191,
   PIPE_FORMAT_R16G16B16A16_SINT       = 192,

   PIPE_FORMAT_R32_UINT                = 193,
   PIPE_FORMAT_R32G32_UINT             = 194,
   PIPE_FORMAT_R32G32B32_UINT          = 195,
   PIPE_FORMAT_R32G32B32A32_UINT       = 196,

   PIPE_FORMAT_R32_SINT                = 197,
   PIPE_FORMAT_R32G32_SINT             = 198,
   PIPE_FORMAT_R32G32B32_SINT          = 199,
   PIPE_FORMAT_R32G32B32A32_SINT       = 200,

   PIPE_FORMAT_A8_UINT                 = 201,
   PIPE_FORMAT_I8_UINT                 = 202,
   PIPE_FORMAT_L8_UINT                 = 203,
   PIPE_FORMAT_L8A8_UINT               = 204,

   PIPE_FORMAT_A8_SINT                 = 205,
   PIPE_FORMAT_I8_SINT                 = 206,
   PIPE_FORMAT_L8_SINT                 = 207,
   PIPE_FORMAT_L8A8_SINT               = 208,

   PIPE_FORMAT_A16_UINT                = 209,
   PIPE_FORMAT_I16_UINT                = 210,
   PIPE_FORMAT_L16_UINT                = 211,
   PIPE_FORMAT_L16A16_UINT             = 212,

   PIPE_FORMAT_A16_SINT                = 213,
   PIPE_FORMAT_I16_SINT                = 214,
   PIPE_FORMAT_L16_SINT                = 215,
   PIPE_FORMAT_L16A16_SINT             = 216,

   PIPE_FORMAT_A32_UINT                = 217,
   PIPE_FORMAT_I32_UINT                = 218,
   PIPE_FORMAT_L32_UINT                = 219,
   PIPE_FORMAT_L32A32_UINT             = 220,

   PIPE_FORMAT_A32_SINT                = 221,
   PIPE_FORMAT_I32_SINT                = 222,
   PIPE_FORMAT_L32_SINT                = 223,
   PIPE_FORMAT_L32A32_SINT             = 224,

   PIPE_FORMAT_B10G10R10A2_UINT        = 225, 

   PIPE_FORMAT_ETC1_RGB8               = 226,

   PIPE_FORMAT_COUNT
};

enum pipe_video_chroma_format
{
   PIPE_VIDEO_CHROMA_FORMAT_420,
   PIPE_VIDEO_CHROMA_FORMAT_422,
   PIPE_VIDEO_CHROMA_FORMAT_444
};

#ifdef __cplusplus
}
#endif

#endif
