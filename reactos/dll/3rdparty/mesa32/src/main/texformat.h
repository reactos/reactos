/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
 * \file texformat.h
 * Texture formats definitions.
 *
 * \author Gareth Hughes
 */


#ifndef TEXFORMAT_H
#define TEXFORMAT_H


#include "mtypes.h"


/**
 * Mesa internal texture image formats.
 * All texture images are stored in one of these formats.
 *
 * NOTE: when you add a new format, be sure to update the do_row()
 * function in texstore.c used for auto mipmap generation.
 */
enum _format {
   /** 
    * \name Hardware-friendly formats.  
    *
    * Drivers can override the default formats and convert texture images to
    * one of these as required.  The driver's
    * dd_function_table::ChooseTextureFormat function will choose one of these
    * formats.
    *
    * \note In the default case, some of these formats will be duplicates of
    * the generic formats listed below.  However, these formats guarantee their
    * internal component sizes, while GLchan may vary between GLubyte, GLushort
    * and GLfloat.
    */
   /*@{*/
				/* msb <------ TEXEL BITS -----------> lsb */
				/* ---- ---- ---- ---- ---- ---- ---- ---- */
   MESA_FORMAT_RGBA8888,	/* RRRR RRRR GGGG GGGG BBBB BBBB AAAA AAAA */
   MESA_FORMAT_RGBA8888_REV,	/* AAAA AAAA BBBB BBBB GGGG GGGG RRRR RRRR */
   MESA_FORMAT_ARGB8888,	/* AAAA AAAA RRRR RRRR GGGG GGGG BBBB BBBB */
   MESA_FORMAT_ARGB8888_REV,	/* BBBB BBBB GGGG GGGG RRRR RRRR AAAA AAAA */
   MESA_FORMAT_RGB888,		/*           RRRR RRRR GGGG GGGG BBBB BBBB */
   MESA_FORMAT_BGR888,		/*           BBBB BBBB GGGG GGGG RRRR RRRR */
   MESA_FORMAT_RGB565,		/*                     RRRR RGGG GGGB BBBB */
   MESA_FORMAT_RGB565_REV,	/*                     GGGB BBBB RRRR RGGG */
   MESA_FORMAT_ARGB4444,	/*                     AAAA RRRR GGGG BBBB */
   MESA_FORMAT_ARGB4444_REV,	/*                     GGGG BBBB AAAA RRRR */
   MESA_FORMAT_ARGB1555,	/*                     ARRR RRGG GGGB BBBB */
   MESA_FORMAT_ARGB1555_REV,	/*                     GGGB BBBB ARRR RRGG */
   MESA_FORMAT_AL88,		/*                     AAAA AAAA LLLL LLLL */
   MESA_FORMAT_AL88_REV,	/*                     LLLL LLLL AAAA AAAA */
   MESA_FORMAT_RGB332,		/*                               RRRG GGBB */
   MESA_FORMAT_A8,		/*                               AAAA AAAA */
   MESA_FORMAT_L8,		/*                               LLLL LLLL */
   MESA_FORMAT_I8,		/*                               IIII IIII */
   MESA_FORMAT_CI8,		/*                               CCCC CCCC */
   MESA_FORMAT_YCBCR,		/*                     YYYY YYYY UorV UorV */
   MESA_FORMAT_YCBCR_REV,	/*                     UorV UorV YYYY YYYY */
   MESA_FORMAT_Z24_S8,          /* ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ SSSS SSSS */
   MESA_FORMAT_Z16,             /*                     ZZZZ ZZZZ ZZZZ ZZZZ */
   MESA_FORMAT_Z32,             /* ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ */
   /*@}*/

#if FEATURE_EXT_texture_sRGB
   /**
    * \name 8-bit/channel sRGB formats
    */
   /*@{*/
   MESA_FORMAT_SRGB8,
   MESA_FORMAT_SRGBA8,
   MESA_FORMAT_SL8,
   MESA_FORMAT_SLA8,
   /*@}*/
#endif

   /**
    * \name Compressed texture formats.
    */
   /*@{*/
   MESA_FORMAT_RGB_FXT1,
   MESA_FORMAT_RGBA_FXT1,
   MESA_FORMAT_RGB_DXT1,
   MESA_FORMAT_RGBA_DXT1,
   MESA_FORMAT_RGBA_DXT3,
   MESA_FORMAT_RGBA_DXT5,
   /*@}*/

   /**
    * \name Generic GLchan-based formats.
    *
    * Software-oriented texture formats.  Texels are arrays of GLchan
    * values so there are no byte order issues.
    *
    * \note Because these are based on the GLchan data type, one cannot assume
    * 8 bits per channel with these formats.  If you require GLubyte channels,
    * use one of the hardware formats above.
    */
   /*@{*/
   MESA_FORMAT_RGBA,
   MESA_FORMAT_RGB,
   MESA_FORMAT_ALPHA,
   MESA_FORMAT_LUMINANCE,
   MESA_FORMAT_LUMINANCE_ALPHA,
   MESA_FORMAT_INTENSITY,
   /*@}*/

   /**
    * \name Floating point texture formats.
    */
   /*@{*/
   MESA_FORMAT_RGBA_FLOAT32,
   MESA_FORMAT_RGBA_FLOAT16,
   MESA_FORMAT_RGB_FLOAT32,
   MESA_FORMAT_RGB_FLOAT16,
   MESA_FORMAT_ALPHA_FLOAT32,
   MESA_FORMAT_ALPHA_FLOAT16,
   MESA_FORMAT_LUMINANCE_FLOAT32,
   MESA_FORMAT_LUMINANCE_FLOAT16,
   MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32,
   MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16,
   MESA_FORMAT_INTENSITY_FLOAT32,
   MESA_FORMAT_INTENSITY_FLOAT16
   /*@}*/
};


/** GLchan-valued formats */
/*@{*/
extern const struct gl_texture_format _mesa_texformat_rgba;
extern const struct gl_texture_format _mesa_texformat_rgb;
extern const struct gl_texture_format _mesa_texformat_alpha;
extern const struct gl_texture_format _mesa_texformat_luminance;
extern const struct gl_texture_format _mesa_texformat_luminance_alpha;
extern const struct gl_texture_format _mesa_texformat_intensity;
/*@}*/

#if FEATURE_EXT_texture_sRGB
/** sRGB (nonlinear) formats */
/*@{*/
extern const struct gl_texture_format _mesa_texformat_srgb8;
extern const struct gl_texture_format _mesa_texformat_srgba8;
extern const struct gl_texture_format _mesa_texformat_sl8;
extern const struct gl_texture_format _mesa_texformat_sla8;
/*@}*/
#endif

/** Floating point texture formats */
/*@{*/
extern const struct gl_texture_format _mesa_texformat_rgba_float32;
extern const struct gl_texture_format _mesa_texformat_rgba_float16;
extern const struct gl_texture_format _mesa_texformat_rgb_float32;
extern const struct gl_texture_format _mesa_texformat_rgb_float16;
extern const struct gl_texture_format _mesa_texformat_alpha_float32;
extern const struct gl_texture_format _mesa_texformat_alpha_float16;
extern const struct gl_texture_format _mesa_texformat_luminance_float32;
extern const struct gl_texture_format _mesa_texformat_luminance_float16;
extern const struct gl_texture_format _mesa_texformat_luminance_alpha_float32;
extern const struct gl_texture_format _mesa_texformat_luminance_alpha_float16;
extern const struct gl_texture_format _mesa_texformat_intensity_float32;
extern const struct gl_texture_format _mesa_texformat_intensity_float16;
/*@}*/

/** \name Assorted hardware-friendly formats */
/*@{*/
extern const struct gl_texture_format _mesa_texformat_rgba8888;
extern const struct gl_texture_format _mesa_texformat_rgba8888_rev;
extern const struct gl_texture_format _mesa_texformat_argb8888;
extern const struct gl_texture_format _mesa_texformat_argb8888_rev;
extern const struct gl_texture_format _mesa_texformat_rgb888;
extern const struct gl_texture_format _mesa_texformat_bgr888;
extern const struct gl_texture_format _mesa_texformat_rgb565;
extern const struct gl_texture_format _mesa_texformat_rgb565_rev;
extern const struct gl_texture_format _mesa_texformat_argb4444;
extern const struct gl_texture_format _mesa_texformat_argb4444_rev;
extern const struct gl_texture_format _mesa_texformat_argb1555;
extern const struct gl_texture_format _mesa_texformat_argb1555_rev;
extern const struct gl_texture_format _mesa_texformat_al88;
extern const struct gl_texture_format _mesa_texformat_al88_rev;
extern const struct gl_texture_format _mesa_texformat_rgb332;
extern const struct gl_texture_format _mesa_texformat_a8;
extern const struct gl_texture_format _mesa_texformat_l8;
extern const struct gl_texture_format _mesa_texformat_i8;
extern const struct gl_texture_format _mesa_texformat_ci8;
extern const struct gl_texture_format _mesa_texformat_z24_s8;
extern const struct gl_texture_format _mesa_texformat_z16;
extern const struct gl_texture_format _mesa_texformat_z32;
/*@}*/

/** \name YCbCr formats */
/*@{*/
extern const struct gl_texture_format _mesa_texformat_ycbcr;
extern const struct gl_texture_format _mesa_texformat_ycbcr_rev;
/*@}*/

/** \name Compressed formats */
/*@{*/
extern const struct gl_texture_format _mesa_texformat_rgb_fxt1;
extern const struct gl_texture_format _mesa_texformat_rgba_fxt1;
extern const struct gl_texture_format _mesa_texformat_rgb_dxt1;
extern const struct gl_texture_format _mesa_texformat_rgba_dxt1;
extern const struct gl_texture_format _mesa_texformat_rgba_dxt3;
extern const struct gl_texture_format _mesa_texformat_rgba_dxt5;
/*@}*/

/** \name The null format */
/*@{*/
extern const struct gl_texture_format _mesa_null_texformat;
/*@}*/


extern const struct gl_texture_format *
_mesa_choose_tex_format( GLcontext *ctx, GLint internalFormat,
                         GLenum format, GLenum type );

#endif
