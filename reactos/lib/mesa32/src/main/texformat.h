/**
 * \file texformat.h
 * Texture formats definitions.
 *
 * \author Gareth Hughes
 */

/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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


#ifndef TEXFORMAT_H
#define TEXFORMAT_H

#include "mtypes.h"


/**
 * Mesa internal texture image types.
 * 
 * All texture images must be stored in one of these formats.
 */
enum _format {
   /** 
    * \name Hardware-friendly formats.  
    *
    * Drivers can override the default formats and convert texture images to
    * one of these as required.  The driver's
    * dd_function_table::ChooseTextureFormat function will choose one of these
    * formats.  These formats are all little endian, as shown below.  They will
    * be most useful for x86-based PC graphics card drivers.
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
   MESA_FORMAT_ARGB8888,	/* AAAA AAAA RRRR RRRR GGGG GGGG BBBB BBBB */
   MESA_FORMAT_RGB888,		/*           RRRR RRRR GGGG GGGG BBBB BBBB */
   MESA_FORMAT_RGB565,		/*                     RRRR RGGG GGGB BBBB */
   MESA_FORMAT_ARGB4444,	/*                     AAAA RRRR GGGG BBBB */
   MESA_FORMAT_ARGB1555,	/*                     ARRR RRGG GGGB BBBB */
   MESA_FORMAT_AL88,		/*                     AAAA AAAA LLLL LLLL */
   MESA_FORMAT_RGB332,		/*                               RRRG GGBB */
   MESA_FORMAT_A8,		/*                               AAAA AAAA */
   MESA_FORMAT_L8,		/*                               LLLL LLLL */
   MESA_FORMAT_I8,		/*                               IIII IIII */
   MESA_FORMAT_CI8,		/*                               CCCC CCCC */
   MESA_FORMAT_YCBCR,		/*                     YYYY YYYY UorV UorV */
   MESA_FORMAT_YCBCR_REV,	/*                     UorV UorV YYYY YYYY */
   /*@}*/

   MESA_FORMAT_RGB_FXT1,
   MESA_FORMAT_RGBA_FXT1,
   MESA_FORMAT_RGB_DXT1,
   MESA_FORMAT_RGBA_DXT1,
   MESA_FORMAT_RGBA_DXT3,
   MESA_FORMAT_RGBA_DXT5,

#if 0
   /** 
    * \name Upcoming little-endian formats 
    */
   /*@{*/
				/* msb <------ TEXEL BITS -----------> lsb */
				/* ---- ---- ---- ---- ---- ---- ---- ---- */
   MESA_FORMAT_ABGR8888,	/* AAAA AAAA BBBB BBBB GGGG GGGG RRRR RRRR */
   MESA_FORMAT_BGRA8888,	/* BBBB BBBB GGGG GGGG RRRR RRRR AAAA AAAA */
   MESA_FORMAT_BGR888,		/*           BBBB BBBB GGGG GGGG RRRR RRRR */
   MESA_FORMAT_BGR565,		/*                     BBBB BGGG GGGR RRRR */
   MESA_FORMAT_BGRA4444,	/*                     BBBB GGGG RRRR AAAA */
   MESA_FORMAT_BGRA5551,	/*                     BBBB BGGG GGRR RRRA */
   MESA_FORMAT_LA88,		/*                     LLLL LLLL AAAA AAAA */
   MESA_FORMAT_BGR233,		/*                               BBGG GRRR */
   /*@}*/
#endif

   /**
    * \name Generic GLchan-based formats.
    *
    * These are the default formats used by the software rasterizer and, unless
    * the driver overrides the texture image functions, incoming images will be
    * converted to one of these formats.  Components are arrays of GLchan
    * values, so there will be no big/little endian issues.
    *
    * \note Because these are based on the GLchan data type, one cannot assume 8
    * bits per channel with these formats.  If you require GLubyte channels,
    * use one of the hardware formats above.
    */
   /*@{*/
   MESA_FORMAT_RGBA,
   MESA_FORMAT_RGB,
   MESA_FORMAT_ALPHA,
   MESA_FORMAT_LUMINANCE,
   MESA_FORMAT_LUMINANCE_ALPHA,
   MESA_FORMAT_INTENSITY,
   MESA_FORMAT_COLOR_INDEX,
   MESA_FORMAT_DEPTH_COMPONENT
   /*@}*/
};


extern GLboolean
_mesa_is_hardware_tex_format( const struct gl_texture_format *format );

extern const struct gl_texture_format *
_mesa_choose_tex_format( GLcontext *ctx, GLint internalFormat,
                         GLenum format, GLenum type );

extern GLint
_mesa_base_compressed_texformat(GLcontext *ctx, GLint intFormat);


/** The default formats, GLchan per component */
/*@{*/
extern const struct gl_texture_format _mesa_texformat_rgba;
extern const struct gl_texture_format _mesa_texformat_rgb;
extern const struct gl_texture_format _mesa_texformat_alpha;
extern const struct gl_texture_format _mesa_texformat_luminance;
extern const struct gl_texture_format _mesa_texformat_luminance_alpha;
extern const struct gl_texture_format _mesa_texformat_intensity;
extern const struct gl_texture_format _mesa_texformat_color_index;
extern const struct gl_texture_format _mesa_texformat_depth_component;
/*@}*/

/** \name The hardware-friendly formats */
/*@{*/
extern const struct gl_texture_format _mesa_texformat_rgba8888;
extern const struct gl_texture_format _mesa_texformat_argb8888;
extern const struct gl_texture_format _mesa_texformat_rgb888;
extern const struct gl_texture_format _mesa_texformat_rgb565;
extern const struct gl_texture_format _mesa_texformat_argb4444;
extern const struct gl_texture_format _mesa_texformat_argb1555;
extern const struct gl_texture_format _mesa_texformat_al88;
extern const struct gl_texture_format _mesa_texformat_rgb332;
extern const struct gl_texture_format _mesa_texformat_a8;
extern const struct gl_texture_format _mesa_texformat_l8;
extern const struct gl_texture_format _mesa_texformat_i8;
extern const struct gl_texture_format _mesa_texformat_ci8;
extern const struct gl_texture_format _mesa_texformat_ycbcr;
extern const struct gl_texture_format _mesa_texformat_ycbcr_rev;
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

#endif
