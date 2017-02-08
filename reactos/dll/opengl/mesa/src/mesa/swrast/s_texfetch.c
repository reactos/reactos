/*
 * Mesa 3-D graphics library
 * Version:  7.7
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (c) 2009  VMware, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \file s_texfetch.c
 *
 * Texel fetch/store functions
 *
 * \author Gareth Hughes
 */


#include "main/colormac.h"
#include "main/macros.h"
#include "main/texcompress.h"
#include "main/texcompress_fxt1.h"
#include "main/texcompress_s3tc.h"
#include "main/texcompress_rgtc.h"
#include "main/texcompress_etc.h"
#include "main/teximage.h"
#include "s_context.h"
#include "s_texfetch.h"
#include "../../gallium/auxiliary/util/u_format_rgb9e5.h"
#include "../../gallium/auxiliary/util/u_format_r11g11b10f.h"


/**
 * Convert an 8-bit sRGB value from non-linear space to a
 * linear RGB value in [0, 1].
 * Implemented with a 256-entry lookup table.
 */
static inline GLfloat
nonlinear_to_linear(GLubyte cs8)
{
   static GLfloat table[256];
   static GLboolean tableReady = GL_FALSE;
   if (!tableReady) {
      /* compute lookup table now */
      GLuint i;
      for (i = 0; i < 256; i++) {
         const GLfloat cs = UBYTE_TO_FLOAT(i);
         if (cs <= 0.04045) {
            table[i] = cs / 12.92f;
         }
         else {
            table[i] = (GLfloat) pow((cs + 0.055) / 1.055, 2.4);
         }
      }
      tableReady = GL_TRUE;
   }
   return table[cs8];
}



/* Texel fetch routines for all supported formats
 */
#define DIM 1
#include "s_texfetch_tmp.h"

#define DIM 2
#include "s_texfetch_tmp.h"

#define DIM 3
#include "s_texfetch_tmp.h"

/**
 * Null texel fetch function.
 *
 * Have to have this so the FetchTexel function pointer is never NULL.
 */
static void fetch_null_texelf( const struct swrast_texture_image *texImage,
                               GLint i, GLint j, GLint k, GLfloat *texel )
{
   (void) texImage; (void) i; (void) j; (void) k;
   texel[RCOMP] = 0.0;
   texel[GCOMP] = 0.0;
   texel[BCOMP] = 0.0;
   texel[ACOMP] = 0.0;
   _mesa_warning(NULL, "fetch_null_texelf() called!");
}


/**
 * Table to map MESA_FORMAT_ to texel fetch/store funcs.
 * XXX this is somewhat temporary.
 */
static struct {
   gl_format Name;
   FetchTexelFunc Fetch1D;
   FetchTexelFunc Fetch2D;
   FetchTexelFunc Fetch3D;
}
texfetch_funcs[MESA_FORMAT_COUNT] =
{
   {
      MESA_FORMAT_NONE,
      fetch_null_texelf,
      fetch_null_texelf,
      fetch_null_texelf
   },

   {
      MESA_FORMAT_RGBA8888,
      fetch_texel_1d_f_rgba8888,
      fetch_texel_2d_f_rgba8888,
      fetch_texel_3d_f_rgba8888
   },
   {
      MESA_FORMAT_RGBA8888_REV,
      fetch_texel_1d_f_rgba8888_rev,
      fetch_texel_2d_f_rgba8888_rev,
      fetch_texel_3d_f_rgba8888_rev
   },
   {
      MESA_FORMAT_ARGB8888,
      fetch_texel_1d_f_argb8888,
      fetch_texel_2d_f_argb8888,
      fetch_texel_3d_f_argb8888
   },
   {
      MESA_FORMAT_ARGB8888_REV,
      fetch_texel_1d_f_argb8888_rev,
      fetch_texel_2d_f_argb8888_rev,
      fetch_texel_3d_f_argb8888_rev
   },
   {
      MESA_FORMAT_RGBX8888,
      fetch_texel_1d_f_rgbx8888,
      fetch_texel_2d_f_rgbx8888,
      fetch_texel_3d_f_rgbx8888
   },
   {
      MESA_FORMAT_RGBX8888_REV,
      fetch_texel_1d_f_rgbx8888_rev,
      fetch_texel_2d_f_rgbx8888_rev,
      fetch_texel_3d_f_rgbx8888_rev
   },
   {
      MESA_FORMAT_XRGB8888,
      fetch_texel_1d_f_xrgb8888,
      fetch_texel_2d_f_xrgb8888,
      fetch_texel_3d_f_xrgb8888
   },
   {
      MESA_FORMAT_XRGB8888_REV,
      fetch_texel_1d_f_xrgb8888_rev,
      fetch_texel_2d_f_xrgb8888_rev,
      fetch_texel_3d_f_xrgb8888_rev
   },
   {
      MESA_FORMAT_RGB888,
      fetch_texel_1d_f_rgb888,
      fetch_texel_2d_f_rgb888,
      fetch_texel_3d_f_rgb888
   },
   {
      MESA_FORMAT_BGR888,
      fetch_texel_1d_f_bgr888,
      fetch_texel_2d_f_bgr888,
      fetch_texel_3d_f_bgr888
   },
   {
      MESA_FORMAT_RGB565,
      fetch_texel_1d_f_rgb565,
      fetch_texel_2d_f_rgb565,
      fetch_texel_3d_f_rgb565
   },
   {
      MESA_FORMAT_RGB565_REV,
      fetch_texel_1d_f_rgb565_rev,
      fetch_texel_2d_f_rgb565_rev,
      fetch_texel_3d_f_rgb565_rev
   },
   {
      MESA_FORMAT_ARGB4444,
      fetch_texel_1d_f_argb4444,
      fetch_texel_2d_f_argb4444,
      fetch_texel_3d_f_argb4444
   },
   {
      MESA_FORMAT_ARGB4444_REV,
      fetch_texel_1d_f_argb4444_rev,
      fetch_texel_2d_f_argb4444_rev,
      fetch_texel_3d_f_argb4444_rev
   },
   {
      MESA_FORMAT_RGBA5551,
      fetch_texel_1d_f_rgba5551,
      fetch_texel_2d_f_rgba5551,
      fetch_texel_3d_f_rgba5551
   },
   {
      MESA_FORMAT_ARGB1555,
      fetch_texel_1d_f_argb1555,
      fetch_texel_2d_f_argb1555,
      fetch_texel_3d_f_argb1555
   },
   {
      MESA_FORMAT_ARGB1555_REV,
      fetch_texel_1d_f_argb1555_rev,
      fetch_texel_2d_f_argb1555_rev,
      fetch_texel_3d_f_argb1555_rev
   },
   {
      MESA_FORMAT_AL44,
      fetch_texel_1d_f_al44,
      fetch_texel_2d_f_al44,
      fetch_texel_3d_f_al44
   },
   {
      MESA_FORMAT_AL88,
      fetch_texel_1d_f_al88,
      fetch_texel_2d_f_al88,
      fetch_texel_3d_f_al88
   },
   {
      MESA_FORMAT_AL88_REV,
      fetch_texel_1d_f_al88_rev,
      fetch_texel_2d_f_al88_rev,
      fetch_texel_3d_f_al88_rev
   },
   {
      MESA_FORMAT_AL1616,
      fetch_texel_1d_f_al1616,
      fetch_texel_2d_f_al1616,
      fetch_texel_3d_f_al1616
   },
   {
      MESA_FORMAT_AL1616_REV,
      fetch_texel_1d_f_al1616_rev,
      fetch_texel_2d_f_al1616_rev,
      fetch_texel_3d_f_al1616_rev
   },
   {
      MESA_FORMAT_RGB332,
      fetch_texel_1d_f_rgb332,
      fetch_texel_2d_f_rgb332,
      fetch_texel_3d_f_rgb332
   },
   {
      MESA_FORMAT_A8,
      fetch_texel_1d_f_a8,
      fetch_texel_2d_f_a8,
      fetch_texel_3d_f_a8
   },
   {
      MESA_FORMAT_A16,
      fetch_texel_1d_f_a16,
      fetch_texel_2d_f_a16,
      fetch_texel_3d_f_a16
   },
   {
      MESA_FORMAT_L8,
      fetch_texel_1d_f_l8,
      fetch_texel_2d_f_l8,
      fetch_texel_3d_f_l8
   },
   {
      MESA_FORMAT_L16,
      fetch_texel_1d_f_l16,
      fetch_texel_2d_f_l16,
      fetch_texel_3d_f_l16
   },
   {
      MESA_FORMAT_I8,
      fetch_texel_1d_f_i8,
      fetch_texel_2d_f_i8,
      fetch_texel_3d_f_i8
   },
   {
      MESA_FORMAT_I16,
      fetch_texel_1d_f_i16,
      fetch_texel_2d_f_i16,
      fetch_texel_3d_f_i16
   },
   {
      MESA_FORMAT_YCBCR,
      fetch_texel_1d_f_ycbcr,
      fetch_texel_2d_f_ycbcr,
      fetch_texel_3d_f_ycbcr
   },
   {
      MESA_FORMAT_YCBCR_REV,
      fetch_texel_1d_f_ycbcr_rev,
      fetch_texel_2d_f_ycbcr_rev,
      fetch_texel_3d_f_ycbcr_rev
   },
   {
      MESA_FORMAT_R8,
      fetch_texel_1d_f_r8,
      fetch_texel_2d_f_r8,
      fetch_texel_3d_f_r8
   },
   {
      MESA_FORMAT_GR88,
      fetch_texel_1d_f_gr88,
      fetch_texel_2d_f_gr88,
      fetch_texel_3d_f_gr88
   },
   {
      MESA_FORMAT_RG88,
      fetch_texel_1d_f_rg88,
      fetch_texel_2d_f_rg88,
      fetch_texel_3d_f_rg88
   },
   {
      MESA_FORMAT_R16,
      fetch_texel_1d_f_r16,
      fetch_texel_2d_f_r16,
      fetch_texel_3d_f_r16
   },
   {
      MESA_FORMAT_RG1616,
      fetch_texel_1d_f_rg1616,
      fetch_texel_2d_f_rg1616,
      fetch_texel_3d_f_rg1616
   },
   {
      MESA_FORMAT_RG1616_REV,
      fetch_texel_1d_f_rg1616_rev,
      fetch_texel_2d_f_rg1616_rev,
      fetch_texel_3d_f_rg1616_rev
   },
   {
      MESA_FORMAT_ARGB2101010,
      fetch_texel_1d_f_argb2101010,
      fetch_texel_2d_f_argb2101010,
      fetch_texel_3d_f_argb2101010
   },
   {
      MESA_FORMAT_Z24_S8,
      fetch_texel_1d_f_z24_s8,
      fetch_texel_2d_f_z24_s8,
      fetch_texel_3d_f_z24_s8
   },
   {
      MESA_FORMAT_S8_Z24,
      fetch_texel_1d_f_s8_z24,
      fetch_texel_2d_f_s8_z24,
      fetch_texel_3d_f_s8_z24
   },
   {
      MESA_FORMAT_Z16,
      fetch_texel_1d_f_z16,
      fetch_texel_2d_f_z16,
      fetch_texel_3d_f_z16
   },
   {
      MESA_FORMAT_X8_Z24,
      fetch_texel_1d_f_s8_z24,
      fetch_texel_2d_f_s8_z24,
      fetch_texel_3d_f_s8_z24
   },
   {
      MESA_FORMAT_Z24_X8,
      fetch_texel_1d_f_z24_s8,
      fetch_texel_2d_f_z24_s8,
      fetch_texel_3d_f_z24_s8
   },
   {
      MESA_FORMAT_Z32,
      fetch_texel_1d_f_z32,
      fetch_texel_2d_f_z32,
      fetch_texel_3d_f_z32
   },
   {
      MESA_FORMAT_S8,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_SRGB8,
      fetch_texel_1d_srgb8,
      fetch_texel_2d_srgb8,
      fetch_texel_3d_srgb8
   },
   {
      MESA_FORMAT_SRGBA8,
      fetch_texel_1d_srgba8,
      fetch_texel_2d_srgba8,
      fetch_texel_3d_srgba8
   },
   {
      MESA_FORMAT_SARGB8,
      fetch_texel_1d_sargb8,
      fetch_texel_2d_sargb8,
      fetch_texel_3d_sargb8
   },
   {
      MESA_FORMAT_SL8,
      fetch_texel_1d_sl8,
      fetch_texel_2d_sl8,
      fetch_texel_3d_sl8
   },
   {
      MESA_FORMAT_SLA8,
      fetch_texel_1d_sla8,
      fetch_texel_2d_sla8,
      fetch_texel_3d_sla8
   },
   {
      MESA_FORMAT_SRGB_DXT1,
      NULL,
      _mesa_fetch_texel_2d_f_srgb_dxt1,
      NULL
   },
   {
      MESA_FORMAT_SRGBA_DXT1,
      NULL,
      _mesa_fetch_texel_2d_f_srgba_dxt1,
      NULL
   },
   {
      MESA_FORMAT_SRGBA_DXT3,
      NULL,
      _mesa_fetch_texel_2d_f_srgba_dxt3,
      NULL
   },
   {
      MESA_FORMAT_SRGBA_DXT5,
      NULL,
      _mesa_fetch_texel_2d_f_srgba_dxt5,
      NULL
   },

   {
      MESA_FORMAT_RGB_FXT1,
      NULL,
      _mesa_fetch_texel_2d_f_rgb_fxt1,
      NULL
   },
   {
      MESA_FORMAT_RGBA_FXT1,
      NULL,
      _mesa_fetch_texel_2d_f_rgba_fxt1,
      NULL
   },
   {
      MESA_FORMAT_RGB_DXT1,
      NULL,
      _mesa_fetch_texel_2d_f_rgb_dxt1,
      NULL
   },
   {
      MESA_FORMAT_RGBA_DXT1,
      NULL,
      _mesa_fetch_texel_2d_f_rgba_dxt1,
      NULL
   },
   {
      MESA_FORMAT_RGBA_DXT3,
      NULL,
      _mesa_fetch_texel_2d_f_rgba_dxt3,
      NULL
   },
   {
      MESA_FORMAT_RGBA_DXT5,
      NULL,
      _mesa_fetch_texel_2d_f_rgba_dxt5,
      NULL
   },
   {
      MESA_FORMAT_RGBA_FLOAT32,
      fetch_texel_1d_f_rgba_f32,
      fetch_texel_2d_f_rgba_f32,
      fetch_texel_3d_f_rgba_f32
   },
   {
      MESA_FORMAT_RGBA_FLOAT16,
      fetch_texel_1d_f_rgba_f16,
      fetch_texel_2d_f_rgba_f16,
      fetch_texel_3d_f_rgba_f16
   },
   {
      MESA_FORMAT_RGB_FLOAT32,
      fetch_texel_1d_f_rgb_f32,
      fetch_texel_2d_f_rgb_f32,
      fetch_texel_3d_f_rgb_f32
   },
   {
      MESA_FORMAT_RGB_FLOAT16,
      fetch_texel_1d_f_rgb_f16,
      fetch_texel_2d_f_rgb_f16,
      fetch_texel_3d_f_rgb_f16
   },
   {
      MESA_FORMAT_ALPHA_FLOAT32,
      fetch_texel_1d_f_alpha_f32,
      fetch_texel_2d_f_alpha_f32,
      fetch_texel_3d_f_alpha_f32
   },
   {
      MESA_FORMAT_ALPHA_FLOAT16,
      fetch_texel_1d_f_alpha_f16,
      fetch_texel_2d_f_alpha_f16,
      fetch_texel_3d_f_alpha_f16
   },
   {
      MESA_FORMAT_LUMINANCE_FLOAT32,
      fetch_texel_1d_f_luminance_f32,
      fetch_texel_2d_f_luminance_f32,
      fetch_texel_3d_f_luminance_f32
   },
   {
      MESA_FORMAT_LUMINANCE_FLOAT16,
      fetch_texel_1d_f_luminance_f16,
      fetch_texel_2d_f_luminance_f16,
      fetch_texel_3d_f_luminance_f16
   },
   {
      MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32,
      fetch_texel_1d_f_luminance_alpha_f32,
      fetch_texel_2d_f_luminance_alpha_f32,
      fetch_texel_3d_f_luminance_alpha_f32
   },
   {
      MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16,
      fetch_texel_1d_f_luminance_alpha_f16,
      fetch_texel_2d_f_luminance_alpha_f16,
      fetch_texel_3d_f_luminance_alpha_f16
   },
   {
      MESA_FORMAT_INTENSITY_FLOAT32,
      fetch_texel_1d_f_intensity_f32,
      fetch_texel_2d_f_intensity_f32,
      fetch_texel_3d_f_intensity_f32
   },
   {
      MESA_FORMAT_INTENSITY_FLOAT16,
      fetch_texel_1d_f_intensity_f16,
      fetch_texel_2d_f_intensity_f16,
      fetch_texel_3d_f_intensity_f16
   },
   {
      MESA_FORMAT_R_FLOAT32,
      fetch_texel_1d_f_r_f32,
      fetch_texel_2d_f_r_f32,
      fetch_texel_3d_f_r_f32
   },
   {
      MESA_FORMAT_R_FLOAT16,
      fetch_texel_1d_f_r_f16,
      fetch_texel_2d_f_r_f16,
      fetch_texel_3d_f_r_f16
   },
   {
      MESA_FORMAT_RG_FLOAT32,
      fetch_texel_1d_f_rg_f32,
      fetch_texel_2d_f_rg_f32,
      fetch_texel_3d_f_rg_f32
   },
   {
      MESA_FORMAT_RG_FLOAT16,
      fetch_texel_1d_f_rg_f16,
      fetch_texel_2d_f_rg_f16,
      fetch_texel_3d_f_rg_f16
   },

   {
      MESA_FORMAT_ALPHA_UINT8,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_ALPHA_UINT16,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_ALPHA_UINT32,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_ALPHA_INT8,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_ALPHA_INT16,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_ALPHA_INT32,
      NULL,
      NULL,
      NULL
   },


   {
      MESA_FORMAT_INTENSITY_UINT8,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_INTENSITY_UINT16,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_INTENSITY_UINT32,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_INTENSITY_INT8,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_INTENSITY_INT16,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_INTENSITY_INT32,
      NULL,
      NULL,
      NULL
   },


   {
      MESA_FORMAT_LUMINANCE_UINT8,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_LUMINANCE_UINT16,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_LUMINANCE_UINT32,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_LUMINANCE_INT8,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_LUMINANCE_INT16,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_LUMINANCE_INT32,
      NULL,
      NULL,
      NULL
   },


   {
      MESA_FORMAT_LUMINANCE_ALPHA_UINT8,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_LUMINANCE_ALPHA_UINT16,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_LUMINANCE_ALPHA_UINT32,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_LUMINANCE_ALPHA_INT8,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_LUMINANCE_ALPHA_INT16,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_LUMINANCE_ALPHA_INT32,
      NULL,
      NULL,
      NULL
   },


   {
      MESA_FORMAT_R_INT8,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_RG_INT8,
      NULL,
      NULL,
      NULL
   },

   {
      MESA_FORMAT_RGB_INT8,
      NULL,
      NULL,
      NULL
   },

   /* non-normalized, signed int */
   {
      MESA_FORMAT_RGBA_INT8,
      fetch_texel_1d_rgba_int8,
      fetch_texel_2d_rgba_int8,
      fetch_texel_3d_rgba_int8
   },
   {
      MESA_FORMAT_R_INT16,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RG_INT16,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGB_INT16,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGBA_INT16,
      fetch_texel_1d_rgba_int16,
      fetch_texel_2d_rgba_int16,
      fetch_texel_3d_rgba_int16
   },
   {
      MESA_FORMAT_R_INT32,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RG_INT32,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGB_INT32,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGBA_INT32,
      fetch_texel_1d_rgba_int32,
      fetch_texel_2d_rgba_int32,
      fetch_texel_3d_rgba_int32
   },

   /* non-normalized, unsigned int */
   {
      MESA_FORMAT_R_UINT8,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RG_UINT8,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGB_UINT8,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGBA_UINT8,
      fetch_texel_1d_rgba_uint8,
      fetch_texel_2d_rgba_uint8,
      fetch_texel_3d_rgba_uint8
   },
   {
      MESA_FORMAT_R_UINT16,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RG_UINT16,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGB_UINT16,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGBA_UINT16,
      fetch_texel_1d_rgba_uint16,
      fetch_texel_2d_rgba_uint16,
      fetch_texel_3d_rgba_uint16
   },
   {
      MESA_FORMAT_R_UINT32,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RG_UINT32,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGB_UINT32,
      NULL,
      NULL,
      NULL
   },
   {
      MESA_FORMAT_RGBA_UINT32,
      fetch_texel_1d_rgba_uint32,
      fetch_texel_2d_rgba_uint32,
      fetch_texel_3d_rgba_uint32
   },

   /* dudv */
   {
      MESA_FORMAT_DUDV8,
      fetch_texel_1d_dudv8,
      fetch_texel_2d_dudv8,
      fetch_texel_3d_dudv8
   },

   /* signed, normalized */
   {
      MESA_FORMAT_SIGNED_R8,
      fetch_texel_1d_signed_r8,
      fetch_texel_2d_signed_r8,
      fetch_texel_3d_signed_r8
   },
   {
      MESA_FORMAT_SIGNED_RG88_REV,
      fetch_texel_1d_signed_rg88_rev,
      fetch_texel_2d_signed_rg88_rev,
      fetch_texel_3d_signed_rg88_rev
   },
   {
      MESA_FORMAT_SIGNED_RGBX8888,
      fetch_texel_1d_signed_rgbx8888,
      fetch_texel_2d_signed_rgbx8888,
      fetch_texel_3d_signed_rgbx8888
   },
   {
      MESA_FORMAT_SIGNED_RGBA8888,
      fetch_texel_1d_signed_rgba8888,
      fetch_texel_2d_signed_rgba8888,
      fetch_texel_3d_signed_rgba8888
   },
   {
      MESA_FORMAT_SIGNED_RGBA8888_REV,
      fetch_texel_1d_signed_rgba8888_rev,
      fetch_texel_2d_signed_rgba8888_rev,
      fetch_texel_3d_signed_rgba8888_rev
   },
   {
      MESA_FORMAT_SIGNED_R16,
      fetch_texel_1d_signed_r16,
      fetch_texel_2d_signed_r16,
      fetch_texel_3d_signed_r16
   },
   {
      MESA_FORMAT_SIGNED_GR1616,
      fetch_texel_1d_signed_rg1616,
      fetch_texel_2d_signed_rg1616,
      fetch_texel_3d_signed_rg1616
   },
   {
      MESA_FORMAT_SIGNED_RGB_16,
      fetch_texel_1d_signed_rgb_16,
      fetch_texel_2d_signed_rgb_16,
      fetch_texel_3d_signed_rgb_16
   },
   {
      MESA_FORMAT_SIGNED_RGBA_16,
      fetch_texel_1d_signed_rgba_16,
      fetch_texel_2d_signed_rgba_16,
      fetch_texel_3d_signed_rgba_16
   },
   {
      MESA_FORMAT_RGBA_16,
      fetch_texel_1d_rgba_16,
      fetch_texel_2d_rgba_16,
      fetch_texel_3d_rgba_16
   },
   {
      MESA_FORMAT_RED_RGTC1,
      NULL,
      _mesa_fetch_texel_2d_f_red_rgtc1,
      NULL
   },
   {
      MESA_FORMAT_SIGNED_RED_RGTC1,
      NULL,
      _mesa_fetch_texel_2d_f_signed_red_rgtc1,
      NULL
   },
   {
      MESA_FORMAT_RG_RGTC2,
      NULL,
      _mesa_fetch_texel_2d_f_rg_rgtc2,
      NULL
   },
   {
      MESA_FORMAT_SIGNED_RG_RGTC2,
      NULL,
      _mesa_fetch_texel_2d_f_signed_rg_rgtc2,
      NULL
   },
   {
      MESA_FORMAT_L_LATC1,
      NULL,
      _mesa_fetch_texel_2d_f_l_latc1,
      NULL
   },
   {
      MESA_FORMAT_SIGNED_L_LATC1,
      NULL,
      _mesa_fetch_texel_2d_f_signed_l_latc1,
      NULL
   },
   {
      MESA_FORMAT_LA_LATC2,
      NULL,
      _mesa_fetch_texel_2d_f_la_latc2,
      NULL
   },
   {
      MESA_FORMAT_SIGNED_LA_LATC2,
      NULL,
      _mesa_fetch_texel_2d_f_signed_la_latc2,
      NULL
   },
   {
      MESA_FORMAT_ETC1_RGB8,
      NULL,
      _mesa_fetch_texel_2d_f_etc1_rgb8,
      NULL
   },
   {
      MESA_FORMAT_SIGNED_A8,
      fetch_texel_1d_signed_a8,
      fetch_texel_2d_signed_a8,
      fetch_texel_3d_signed_a8
   },
   {
      MESA_FORMAT_SIGNED_L8,
      fetch_texel_1d_signed_l8,
      fetch_texel_2d_signed_l8,
      fetch_texel_3d_signed_l8
   },
   {
      MESA_FORMAT_SIGNED_AL88,
      fetch_texel_1d_signed_al88,
      fetch_texel_2d_signed_al88,
      fetch_texel_3d_signed_al88
   },
   {
      MESA_FORMAT_SIGNED_I8,
      fetch_texel_1d_signed_i8,
      fetch_texel_2d_signed_i8,
      fetch_texel_3d_signed_i8
   },
   {
      MESA_FORMAT_SIGNED_A16,
      fetch_texel_1d_signed_a16,
      fetch_texel_2d_signed_a16,
      fetch_texel_3d_signed_a16
   },
   {
      MESA_FORMAT_SIGNED_L16,
      fetch_texel_1d_signed_l16,
      fetch_texel_2d_signed_l16,
      fetch_texel_3d_signed_l16
   },
   {
      MESA_FORMAT_SIGNED_AL1616,
      fetch_texel_1d_signed_al1616,
      fetch_texel_2d_signed_al1616,
      fetch_texel_3d_signed_al1616
   },
   {
      MESA_FORMAT_SIGNED_I16,
      fetch_texel_1d_signed_i16,
      fetch_texel_2d_signed_i16,
      fetch_texel_3d_signed_i16
   },
   {
      MESA_FORMAT_RGB9_E5_FLOAT,
      fetch_texel_1d_rgb9_e5,
      fetch_texel_2d_rgb9_e5,
      fetch_texel_3d_rgb9_e5
   },
   {
      MESA_FORMAT_R11_G11_B10_FLOAT,
      fetch_texel_1d_r11_g11_b10f,
      fetch_texel_2d_r11_g11_b10f,
      fetch_texel_3d_r11_g11_b10f
   },
   {
      MESA_FORMAT_Z32_FLOAT,
      fetch_texel_1d_f_r_f32, /* Reuse the R32F functions. */
      fetch_texel_2d_f_r_f32,
      fetch_texel_3d_f_r_f32
   },
   {
      MESA_FORMAT_Z32_FLOAT_X24S8,
      fetch_texel_1d_z32f_x24s8,
      fetch_texel_2d_z32f_x24s8,
      fetch_texel_3d_z32f_x24s8
   },
   {
      MESA_FORMAT_ARGB2101010_UINT,
      NULL,
      NULL,
      NULL
   }
};


FetchTexelFunc
_mesa_get_texel_fetch_func(gl_format format, GLuint dims)
{
#ifdef DEBUG
   /* check that the table entries are sorted by format name */
   gl_format fmt;
   for (fmt = 0; fmt < MESA_FORMAT_COUNT; fmt++) {
      assert(texfetch_funcs[fmt].Name == fmt);
   }
#endif

   STATIC_ASSERT(Elements(texfetch_funcs) == MESA_FORMAT_COUNT);

   assert(format < MESA_FORMAT_COUNT);

   switch (dims) {
   case 1:
      return texfetch_funcs[format].Fetch1D;
   case 2:
      return texfetch_funcs[format].Fetch2D;
   case 3:
      return texfetch_funcs[format].Fetch3D;
   default:
      assert(0 && "bad dims in _mesa_get_texel_fetch_func");
      return NULL;
   }
}


/**
 * Initialize the texture image's FetchTexel methods.
 */
static void
set_fetch_functions(struct swrast_texture_image *texImage, GLuint dims)
{
   gl_format format = texImage->Base.TexFormat;

   ASSERT(dims == 1 || dims == 2 || dims == 3);

   if (texImage->Base.TexObject->Sampler.sRGBDecode == GL_SKIP_DECODE_EXT &&
       _mesa_get_format_color_encoding(format) == GL_SRGB) {
      format = _mesa_get_srgb_format_linear(format);
   }

   texImage->FetchTexel = _mesa_get_texel_fetch_func(format, dims);
   ASSERT(texImage->FetchTexel);
}

void
_mesa_update_fetch_functions(struct gl_texture_object *texObj)
{
   GLuint face, i;
   GLuint dims;

   dims = _mesa_get_texture_dimensions(texObj->Target);

   for (face = 0; face < 6; face++) {
      for (i = 0; i < MAX_TEXTURE_LEVELS; i++) {
         if (texObj->Image[face][i]) {
	    set_fetch_functions(swrast_texture_image(texObj->Image[face][i]),
                                dims);
         }
      }
   }
}
