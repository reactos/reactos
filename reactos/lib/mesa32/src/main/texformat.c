/**
 * \file texformat.c
 * Texture formats.
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


#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "image.h"
#include "imports.h"
#include "mtypes.h"
#include "texformat.h"
#include "teximage.h"
#include "texstate.h"


/* Texel fetch routines for all supported formats
 */
#define DIM 1
#include "texformat_tmp.h"

#define DIM 2
#include "texformat_tmp.h"

#define DIM 3
#include "texformat_tmp.h"

/**
 * Null texel fetch function.
 *
 * Have to have this so the FetchTexel function pointer is never NULL.
 */
static void fetch_null_texel( const struct gl_texture_image *texImage,
			      GLint i, GLint j, GLint k, GLvoid *texel )
{
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = 0;
   rgba[GCOMP] = 0;
   rgba[BCOMP] = 0;
   rgba[ACOMP] = 0;
}


/***************************************************************/
/** \name Default GLchan-based formats */
/*@{*/

const struct gl_texture_format _mesa_texformat_rgba = {
   MESA_FORMAT_RGBA,			/* MesaFormat */
   GL_RGBA,				/* BaseFormat */
   CHAN_BITS,				/* RedBits */
   CHAN_BITS,				/* GreenBits */
   CHAN_BITS,				/* BlueBits */
   CHAN_BITS,				/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   4 * CHAN_BITS / 8,			/* TexelBytes */
   fetch_1d_texel_rgba,			/* FetchTexel1D */
   fetch_2d_texel_rgba,			/* FetchTexel2D */
   fetch_3d_texel_rgba,			/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_rgb = {
   MESA_FORMAT_RGB,			/* MesaFormat */
   GL_RGB,				/* BaseFormat */
   CHAN_BITS,				/* RedBits */
   CHAN_BITS,				/* GreenBits */
   CHAN_BITS,				/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   3 * CHAN_BITS / 8,			/* TexelBytes */
   fetch_1d_texel_rgb,			/* FetchTexel1D */
   fetch_2d_texel_rgb,			/* FetchTexel2D */
   fetch_3d_texel_rgb,			/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_alpha = {
   MESA_FORMAT_ALPHA,			/* MesaFormat */
   GL_ALPHA,				/* BaseFormat */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   CHAN_BITS,				/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   CHAN_BITS / 8,			/* TexelBytes */
   fetch_1d_texel_alpha,		/* FetchTexel1D */
   fetch_2d_texel_alpha,		/* FetchTexel2D */
   fetch_3d_texel_alpha,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_luminance = {
   MESA_FORMAT_LUMINANCE,		/* MesaFormat */
   GL_LUMINANCE,			/* BaseFormat */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   0,					/* AlphaBits */
   CHAN_BITS,				/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   CHAN_BITS / 8,			/* TexelBytes */
   fetch_1d_texel_luminance,		/* FetchTexel1D */
   fetch_2d_texel_luminance,		/* FetchTexel2D */
   fetch_3d_texel_luminance,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_luminance_alpha = {
   MESA_FORMAT_LUMINANCE_ALPHA,		/* MesaFormat */
   GL_LUMINANCE_ALPHA,			/* BaseFormat */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   CHAN_BITS,				/* AlphaBits */
   CHAN_BITS,				/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   2 * CHAN_BITS / 8,			/* TexelBytes */
   fetch_1d_texel_luminance_alpha,	/* FetchTexel1D */
   fetch_2d_texel_luminance_alpha,	/* FetchTexel2D */
   fetch_3d_texel_luminance_alpha,	/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_intensity = {
   MESA_FORMAT_INTENSITY,		/* MesaFormat */
   GL_INTENSITY,			/* BaseFormat */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   CHAN_BITS,				/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   CHAN_BITS / 8,			/* TexelBytes */
   fetch_1d_texel_intensity,		/* FetchTexel1D */
   fetch_2d_texel_intensity,		/* FetchTexel2D */
   fetch_3d_texel_intensity,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_color_index = {
   MESA_FORMAT_COLOR_INDEX,		/* MesaFormat */
   GL_COLOR_INDEX,			/* BaseFormat */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   CHAN_BITS,				/* IndexBits */
   0,					/* DepthBits */
   CHAN_BITS / 8,			/* TexelBytes */
   fetch_1d_texel_color_index,		/* FetchTexel1D */
   fetch_2d_texel_color_index,		/* FetchTexel2D */
   fetch_3d_texel_color_index,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_depth_component = {
   MESA_FORMAT_DEPTH_COMPONENT,		/* MesaFormat */
   GL_DEPTH_COMPONENT,			/* BaseFormat */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   sizeof(GLfloat) * 8,			/* DepthBits */
   sizeof(GLfloat),			/* TexelBytes */
   fetch_1d_texel_depth_component,	/* FetchTexel1D */
   fetch_2d_texel_depth_component,	/* FetchTexel2D */
   fetch_3d_texel_depth_component,	/* FetchTexel3D */
};

/*@}*/


/***************************************************************/
/** \name Hardware formats */
/*@{*/

const struct gl_texture_format _mesa_texformat_rgba8888 = {
   MESA_FORMAT_RGBA8888,		/* MesaFormat */
   GL_RGBA,				/* BaseFormat */
   8,					/* RedBits */
   8,					/* GreenBits */
   8,					/* BlueBits */
   8,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   4,					/* TexelBytes */
   fetch_1d_texel_rgba8888,		/* FetchTexel1D */
   fetch_2d_texel_rgba8888,		/* FetchTexel2D */
   fetch_3d_texel_rgba8888,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_argb8888 = {
   MESA_FORMAT_ARGB8888,		/* MesaFormat */
   GL_RGBA,				/* BaseFormat */
   8,					/* RedBits */
   8,					/* GreenBits */
   8,					/* BlueBits */
   8,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   4,					/* TexelBytes */
   fetch_1d_texel_argb8888,		/* FetchTexel1D */
   fetch_2d_texel_argb8888,		/* FetchTexel2D */
   fetch_3d_texel_argb8888,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_rgb888 = {
   MESA_FORMAT_RGB888,			/* MesaFormat */
   GL_RGB,				/* BaseFormat */
   8,					/* RedBits */
   8,					/* GreenBits */
   8,					/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   3,					/* TexelBytes */
   fetch_1d_texel_rgb888,		/* FetchTexel1D */
   fetch_2d_texel_rgb888,		/* FetchTexel2D */
   fetch_3d_texel_rgb888,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_rgb565 = {
   MESA_FORMAT_RGB565,			/* MesaFormat */
   GL_RGB,				/* BaseFormat */
   5,					/* RedBits */
   6,					/* GreenBits */
   5,					/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   2,					/* TexelBytes */
   fetch_1d_texel_rgb565,		/* FetchTexel1D */
   fetch_2d_texel_rgb565,		/* FetchTexel2D */
   fetch_3d_texel_rgb565,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_argb4444 = {
   MESA_FORMAT_ARGB4444,		/* MesaFormat */
   GL_RGBA,				/* BaseFormat */
   4,					/* RedBits */
   4,					/* GreenBits */
   4,					/* BlueBits */
   4,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   2,					/* TexelBytes */
   fetch_1d_texel_argb4444,		/* FetchTexel1D */
   fetch_2d_texel_argb4444,		/* FetchTexel2D */
   fetch_3d_texel_argb4444,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_argb1555 = {
   MESA_FORMAT_ARGB1555,		/* MesaFormat */
   GL_RGBA,				/* BaseFormat */
   5,					/* RedBits */
   5,					/* GreenBits */
   5,					/* BlueBits */
   1,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   2,					/* TexelBytes */
   fetch_1d_texel_argb1555,		/* FetchTexel1D */
   fetch_2d_texel_argb1555,		/* FetchTexel2D */
   fetch_3d_texel_argb1555,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_al88 = {
   MESA_FORMAT_AL88,			/* MesaFormat */
   GL_LUMINANCE_ALPHA,			/* BaseFormat */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   8,					/* AlphaBits */
   8,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   2,					/* TexelBytes */
   fetch_1d_texel_al88,			/* FetchTexel1D */
   fetch_2d_texel_al88,			/* FetchTexel2D */
   fetch_3d_texel_al88,			/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_rgb332 = {
   MESA_FORMAT_RGB332,			/* MesaFormat */
   GL_RGB,				/* BaseFormat */
   3,					/* RedBits */
   3,					/* GreenBits */
   2,					/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   1,					/* TexelBytes */
   fetch_1d_texel_rgb332,		/* FetchTexel1D */
   fetch_2d_texel_rgb332,		/* FetchTexel2D */
   fetch_3d_texel_rgb332,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_a8 = {
   MESA_FORMAT_A8,			/* MesaFormat */
   GL_ALPHA,				/* BaseFormat */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   8,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   1,					/* TexelBytes */
   fetch_1d_texel_a8,			/* FetchTexel1D */
   fetch_2d_texel_a8,			/* FetchTexel2D */
   fetch_3d_texel_a8,			/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_l8 = {
   MESA_FORMAT_L8,			/* MesaFormat */
   GL_LUMINANCE,			/* BaseFormat */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   0,					/* AlphaBits */
   8,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   1,					/* TexelBytes */
   fetch_1d_texel_l8,			/* FetchTexel1D */
   fetch_2d_texel_l8,			/* FetchTexel2D */
   fetch_3d_texel_l8,			/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_i8 = {
   MESA_FORMAT_I8,			/* MesaFormat */
   GL_INTENSITY,			/* BaseFormat */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   8,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   1,					/* TexelBytes */
   fetch_1d_texel_i8,			/* FetchTexel1D */
   fetch_2d_texel_i8,			/* FetchTexel2D */
   fetch_3d_texel_i8,			/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_ci8 = {
   MESA_FORMAT_CI8,			/* MesaFormat */
   GL_COLOR_INDEX,			/* BaseFormat */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   8,					/* IndexBits */
   0,					/* DepthBits */
   1,					/* TexelBytes */
   fetch_1d_texel_ci8,			/* FetchTexel1D */
   fetch_2d_texel_ci8,			/* FetchTexel2D */
   fetch_3d_texel_ci8,			/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_ycbcr = {
   MESA_FORMAT_YCBCR,			/* MesaFormat */
   GL_YCBCR_MESA,			/* BaseFormat */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   2,					/* TexelBytes */
   fetch_1d_texel_ycbcr,		/* FetchTexel1D */
   fetch_2d_texel_ycbcr,		/* FetchTexel2D */
   fetch_3d_texel_ycbcr,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_ycbcr_rev = {
   MESA_FORMAT_YCBCR_REV,		/* MesaFormat */
   GL_YCBCR_MESA,			/* BaseFormat */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   2,					/* TexelBytes */
   fetch_1d_texel_ycbcr_rev,		/* FetchTexel1D */
   fetch_2d_texel_ycbcr_rev,		/* FetchTexel2D */
   fetch_3d_texel_ycbcr_rev,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_rgb_fxt1 = {
   MESA_FORMAT_RGB_FXT1,		/* MesaFormat */
   GL_RGB,				/* BaseFormat */
   4, /*approx*/			/* RedBits */
   4, /*approx*/			/* GreenBits */
   4, /*approx*/			/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   0,					/* TexelBytes */
   NULL, /*impossible*/ 		/* FetchTexel1D */
   fetch_2d_texel_rgb_fxt1, 		/* FetchTexel2D */
   NULL, /*impossible*/ 		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_rgba_fxt1 = {
   MESA_FORMAT_RGBA_FXT1,		/* MesaFormat */
   GL_RGBA,				/* BaseFormat */
   4, /*approx*/			/* RedBits */
   4, /*approx*/			/* GreenBits */
   4, /*approx*/			/* BlueBits */
   1, /*approx*/			/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   0,					/* TexelBytes */
   NULL, /*impossible*/ 		/* FetchTexel1D */
   fetch_2d_texel_rgba_fxt1, 		/* FetchTexel2D */
   NULL, /*impossible*/ 		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_rgb_dxt1 = {
   MESA_FORMAT_RGB_DXT1,		/* MesaFormat */
   GL_RGB,				/* BaseFormat */
   4, /*approx*/			/* RedBits */
   4, /*approx*/			/* GreenBits */
   4, /*approx*/			/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   0,					/* TexelBytes */
   NULL, /*impossible*/ 		/* FetchTexel1D */
   fetch_2d_texel_rgb_dxt1, 		/* FetchTexel2D */
   NULL, /*impossible*/ 		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_rgba_dxt1 = {
   MESA_FORMAT_RGBA_DXT1,		/* MesaFormat */
   GL_RGBA,				/* BaseFormat */
   4, /*approx*/			/* RedBits */
   4, /*approx*/			/* GreenBits */
   4, /*approx*/			/* BlueBits */
   1, /*approx*/			/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   0,					/* TexelBytes */
   NULL, /*impossible*/ 		/* FetchTexel1D */
   fetch_2d_texel_rgba_dxt1, 		/* FetchTexel2D */
   NULL, /*impossible*/ 		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_rgba_dxt3 = {
   MESA_FORMAT_RGBA_DXT3,		/* MesaFormat */
   GL_RGBA,				/* BaseFormat */
   4, /*approx*/			/* RedBits */
   4, /*approx*/			/* GreenBits */
   4, /*approx*/			/* BlueBits */
   4, /*approx*/			/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   0,					/* TexelBytes */
   NULL, /*impossible*/ 		/* FetchTexel1D */
   fetch_2d_texel_rgba_dxt3, 		/* FetchTexel2D */
   NULL, /*impossible*/ 		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_rgba_dxt5 = {
   MESA_FORMAT_RGBA_DXT5,		/* MesaFormat */
   GL_RGBA,				/* BaseFormat */
   4,/*approx*/				/* RedBits */
   4,/*approx*/				/* GreenBits */
   4,/*approx*/				/* BlueBits */
   4,/*approx*/				/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   0,					/* TexelBytes */
   NULL, /*impossible*/ 		/* FetchTexel1D */
   fetch_2d_texel_rgba_dxt5, 		/* FetchTexel2D */
   NULL, /*impossible*/ 		/* FetchTexel3D */
};


/* Big-endian */
#if 0
const struct gl_texture_format _mesa_texformat_abgr8888 = {
   MESA_FORMAT_ABGR8888,		/* MesaFormat */
   GL_RGBA,				/* BaseFormat */
   GL_UNSIGNED_INT_8_8_8_8,		/* Type */
   8,					/* RedBits */
   8,					/* GreenBits */
   8,					/* BlueBits */
   8,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   4,					/* TexelBytes */
   fetch_1d_texel_abgr8888,		/* FetchTexel1D */
   fetch_2d_texel_abgr8888,		/* FetchTexel2D */
   fetch_3d_texel_abgr8888,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_bgra8888 = {
   MESA_FORMAT_BGRA8888,		/* MesaFormat */
   GL_RGBA,				/* BaseFormat */
   GL_UNSIGNED_INT_8_8_8_8,		/* Type */
   8,					/* RedBits */
   8,					/* GreenBits */
   8,					/* BlueBits */
   8,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   4,					/* TexelBytes */
   fetch_1d_texel_bgra8888,		/* FetchTexel1D */
   fetch_2d_texel_bgra8888,		/* FetchTexel2D */
   fetch_3d_texel_bgra8888,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_bgr888 = {
   MESA_FORMAT_BGR888,			/* MesaFormat */
   GL_RGB,				/* BaseFormat */
   GL_UNSIGNED_BYTE,			/* Type */
   8,					/* RedBits */
   8,					/* GreenBits */
   8,					/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   3,					/* TexelBytes */
   fetch_1d_texel_bgr888,		/* FetchTexel1D */
   fetch_2d_texel_bgr888,		/* FetchTexel2D */
   fetch_3d_texel_bgr888,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_bgr565 = {
   MESA_FORMAT_BGR565,			/* MesaFormat */
   GL_RGB,				/* BaseFormat */
   GL_UNSIGNED_SHORT_5_6_5,		/* Type */
   5,					/* RedBits */
   6,					/* GreenBits */
   5,					/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   2,					/* TexelBytes */
   fetch_1d_texel_bgr565,		/* FetchTexel1D */
   fetch_2d_texel_bgr565,		/* FetchTexel2D */
   fetch_3d_texel_bgr565,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_bgra4444 = {
   MESA_FORMAT_BGRA4444,		/* MesaFormat */
   GL_RGBA,				/* BaseFormat */
   GL_UNSIGNED_SHORT_4_4_4_4_REV,	/* Type */
   4,					/* RedBits */
   4,					/* GreenBits */
   4,					/* BlueBits */
   4,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   2,					/* TexelBytes */
   fetch_1d_texel_bgra4444,		/* FetchTexel1D */
   fetch_2d_texel_bgra4444,		/* FetchTexel2D */
   fetch_3d_texel_bgra4444,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_bgra5551 = {
   MESA_FORMAT_BGRA5551,		/* MesaFormat */
   GL_RGBA,				/* BaseFormat */
   GL_UNSIGNED_SHORT_1_5_5_5_REV,	/* Type */
   5,					/* RedBits */
   5,					/* GreenBits */
   5,					/* BlueBits */
   1,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   2,					/* TexelBytes */
   fetch_1d_texel_bgra1555,		/* FetchTexel1D */
   fetch_2d_texel_bgra1555,		/* FetchTexel2D */
   fetch_3d_texel_bgra1555,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_la88 = {
   MESA_FORMAT_LA88,			/* MesaFormat */
   GL_LUMINANCE_ALPHA,			/* BaseFormat */
   GL_UNSIGNED_BYTE,			/* Type */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   8,					/* AlphaBits */
   8,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   2,					/* TexelBytes */
   fetch_1d_texel_la88,			/* FetchTexel1D */
   fetch_2d_texel_la88,			/* FetchTexel2D */
   fetch_3d_texel_la88,			/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_bgr233 = {
   MESA_FORMAT_BGR233,			/* MesaFormat */
   GL_RGB,				/* BaseFormat */
   GL_UNSIGNED_BYTE_3_3_2,		/* Type */
   3,					/* RedBits */
   3,					/* GreenBits */
   2,					/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   1,					/* TexelBytes */
   fetch_1d_texel_bgr233,		/* FetchTexel1D */
   fetch_2d_texel_bgr233,		/* FetchTexel2D */
   fetch_3d_texel_bgr233,		/* FetchTexel3D */
};
#endif

/*@}*/


/***************************************************************/
/** \name Null format (useful for proxy textures) */
/*@{*/

const struct gl_texture_format _mesa_null_texformat = {
   -1,					/* MesaFormat */
   0,					/* BaseFormat */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   0,					/* TexelBytes */
   fetch_null_texel,			/* FetchTexel1D */
   fetch_null_texel,			/* FetchTexel2D */
   fetch_null_texel,			/* FetchTexel3D */
};

/*@}*/


/**
 * Determine whether a given texture format is a hardware texture
 * format.
 *
 * \param format texture format.
 * 
 * \return GL_TRUE if \p format is a hardware texture format, or GL_FALSE
 * otherwise.
 *
 * \p format is a hardware texture format if gl_texture_format::MesaFormat is
 * lower than _format::MESA_FORMAT_RGBA.
 */
GLboolean
_mesa_is_hardware_tex_format( const struct gl_texture_format *format )
{
   return (format->MesaFormat < MESA_FORMAT_RGBA);
}


/**
 * Choose an appropriate texture format.
 *
 * \param ctx GL context.
 * \param internalFormat internal texture format.
 * \param format pixel format.
 * \param type data type.
 *
 * \return a pointer to a gl_texture_format in which to store the texture on
 * success, or NULL on failure.
 * 
 * This is called via dd_function_table::ChooseTextureFormat.  Hardware drivers
 * typically override this function with a specialized version.
 */
const struct gl_texture_format *
_mesa_choose_tex_format( GLcontext *ctx, GLint internalFormat,
                         GLenum format, GLenum type )
{
   (void) format;
   (void) type;

   switch ( internalFormat ) {
      /* GH: Bias towards GL_RGB, GL_RGBA texture formats.  This has
       * got to be better than sticking them way down the end of this
       * huge list.
       */
   case 4:				/* Quake3 uses this... */
   case GL_RGBA:
      return &_mesa_texformat_rgba;

   case 3:				/* ... and this. */
   case GL_RGB:
      return &_mesa_texformat_rgb;

      /* GH: Okay, keep checking as normal.  Still test for GL_RGB,
       * GL_RGBA formats first.
       */
   case GL_RGBA2:
   case GL_RGBA4:
   case GL_RGB5_A1:
   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
      return &_mesa_texformat_rgba;

   case GL_R3_G3_B2:
   case GL_RGB4:
   case GL_RGB5:
   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      return &_mesa_texformat_rgb;

   case GL_ALPHA:
   case GL_ALPHA4:
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
      return &_mesa_texformat_alpha;

   case 1:
   case GL_LUMINANCE:
   case GL_LUMINANCE4:
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
      return &_mesa_texformat_luminance;

   case 2:
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE4_ALPHA4:
   case GL_LUMINANCE6_ALPHA2:
   case GL_LUMINANCE8_ALPHA8:
   case GL_LUMINANCE12_ALPHA4:
   case GL_LUMINANCE12_ALPHA12:
   case GL_LUMINANCE16_ALPHA16:
      return &_mesa_texformat_luminance_alpha;

   case GL_INTENSITY:
   case GL_INTENSITY4:
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
      return &_mesa_texformat_intensity;

   case GL_COLOR_INDEX:
   case GL_COLOR_INDEX1_EXT:
   case GL_COLOR_INDEX2_EXT:
   case GL_COLOR_INDEX4_EXT:
   case GL_COLOR_INDEX8_EXT:
   case GL_COLOR_INDEX12_EXT:
   case GL_COLOR_INDEX16_EXT:
      return &_mesa_texformat_color_index;

   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_COMPONENT16_SGIX:
   case GL_DEPTH_COMPONENT24_SGIX:
   case GL_DEPTH_COMPONENT32_SGIX:
      if (!ctx->Extensions.SGIX_depth_texture)
	 _mesa_problem(ctx, "depth format without GL_SGIX_depth_texture");
      return &_mesa_texformat_depth_component;

   case GL_COMPRESSED_ALPHA_ARB:
      if (!ctx->Extensions.ARB_texture_compression)
	 _mesa_problem(ctx, "texture compression extension not enabled");
      return &_mesa_texformat_alpha;
   case GL_COMPRESSED_LUMINANCE_ARB:
      if (!ctx->Extensions.ARB_texture_compression)
	 _mesa_problem(ctx, "texture compression extension not enabled");
      return &_mesa_texformat_luminance;
   case GL_COMPRESSED_LUMINANCE_ALPHA_ARB:
      if (!ctx->Extensions.ARB_texture_compression)
	 _mesa_problem(ctx, "texture compression extension not enabled");
      return &_mesa_texformat_luminance_alpha;
   case GL_COMPRESSED_INTENSITY_ARB:
      if (!ctx->Extensions.ARB_texture_compression)
	 _mesa_problem(ctx, "texture compression extension not enabled");
      return &_mesa_texformat_intensity;
   case GL_COMPRESSED_RGB_ARB:
      if (!ctx->Extensions.ARB_texture_compression)
	 _mesa_problem(ctx, "texture compression extension not enabled");
      if (ctx->Extensions.TDFX_texture_compression_FXT1)
         return &_mesa_texformat_rgb_fxt1;
      else if (ctx->Extensions.EXT_texture_compression_s3tc || ctx->Extensions.S3_s3tc)
         return &_mesa_texformat_rgb_dxt1;
      return &_mesa_texformat_rgb;
   case GL_COMPRESSED_RGBA_ARB:
      if (!ctx->Extensions.ARB_texture_compression)
	 _mesa_problem(ctx, "texture compression extension not enabled");
      if (ctx->Extensions.TDFX_texture_compression_FXT1)
         return &_mesa_texformat_rgba_fxt1;
      else if (ctx->Extensions.EXT_texture_compression_s3tc || ctx->Extensions.S3_s3tc)
         return &_mesa_texformat_rgba_dxt3;  /* Not rgba_dxt1!  See the spec */
      return &_mesa_texformat_rgba;

   /* GL_MESA_ycrcr_texture */
   case GL_YCBCR_MESA:
      if (type == GL_UNSIGNED_SHORT_8_8_MESA)
         return &_mesa_texformat_ycbcr;
      else
         return &_mesa_texformat_ycbcr_rev;

   /* GL_3DFX_texture_compression_FXT1 */
   case GL_COMPRESSED_RGB_FXT1_3DFX:
      if (ctx->Extensions.TDFX_texture_compression_FXT1)
         return &_mesa_texformat_rgb_fxt1;
      else
         return NULL;
   case GL_COMPRESSED_RGBA_FXT1_3DFX:
      if (ctx->Extensions.TDFX_texture_compression_FXT1)
         return &_mesa_texformat_rgba_fxt1;
      else
         return NULL;

   /* GL_EXT_texture_compression_s3tc */
   case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      if (ctx->Extensions.EXT_texture_compression_s3tc)
         return &_mesa_texformat_rgb_dxt1;
      else
         return NULL;
   case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
      if (ctx->Extensions.EXT_texture_compression_s3tc)
         return &_mesa_texformat_rgba_dxt1;
      else
         return NULL;
   case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
      if (ctx->Extensions.EXT_texture_compression_s3tc)
         return &_mesa_texformat_rgba_dxt3;
      else
         return NULL;
   case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
      if (ctx->Extensions.EXT_texture_compression_s3tc)
         return &_mesa_texformat_rgba_dxt5;
      else
         return NULL;

   /* GL_S3_s3tc */
   case GL_RGB_S3TC:
   case GL_RGB4_S3TC:
      if (ctx->Extensions.S3_s3tc)
         return &_mesa_texformat_rgb_dxt1;
      else
         return NULL;
   case GL_RGBA_S3TC:
   case GL_RGBA4_S3TC:
      if (ctx->Extensions.S3_s3tc)
         return &_mesa_texformat_rgba_dxt3;
      else
         return NULL;

   default:
      _mesa_problem(ctx, "unexpected format in _mesa_choose_tex_format()");
      return NULL;
   }
}


/**
 * Return the base texture format for the given compressed format
 * 
 * Called via dd_function_table::Driver.BaseCompressedTexFormat.
 * This function is used by software rasterizers.  Hardware drivers
 * which support texture compression should not use this function but
 * a specialized function instead.
 */
GLint
_mesa_base_compressed_texformat(GLcontext *ctx, GLint intFormat)
{
   switch (intFormat) {
   case GL_COMPRESSED_ALPHA_ARB:
      return GL_ALPHA;
   case GL_COMPRESSED_LUMINANCE_ARB:
      return GL_LUMINANCE;
   case GL_COMPRESSED_LUMINANCE_ALPHA_ARB:
      return GL_LUMINANCE_ALPHA;
   case GL_COMPRESSED_INTENSITY_ARB:
      return GL_INTENSITY;
   case GL_COMPRESSED_RGB_ARB:
      return GL_RGB;
   case GL_COMPRESSED_RGBA_ARB:
      return GL_RGBA;
   default:
      return -1;  /* not a recognized compressed format */
   }
}
