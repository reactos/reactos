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

#include <precomp.h>

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

   /* signed, normalized */
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
