/*
 * Mesa 3-D graphics library
 * Version:  7.7
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (c) 2008-2009  VMware, Inc.
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

/*
 * Authors:
 *   Brian Paul
 */


#ifndef FORMATS_H
#define FORMATS_H


#include <GL/gl.h>


#ifdef __cplusplus
extern "C" {
#endif


/* OpenGL doesn't have GL_UNSIGNED_BYTE_4_4, so we must define our own type
 * for GL_LUMINANCE4_ALPHA4. */
#define MESA_UNSIGNED_BYTE_4_4 (GL_UNSIGNED_BYTE<<1)


/**
 * Max number of bytes for any non-compressed pixel format below, or for
 * intermediate pixel storage in Mesa.  This should never be less than
 * 16.  Maybe 32 someday?
 */
#define MAX_PIXEL_BYTES 16


/**
 * Mesa texture/renderbuffer image formats.
 */
typedef enum
{
   MESA_FORMAT_NONE = 0,

   /** 
    * \name Basic hardware formats
    */
   /*@{*/
				/* msb <------ TEXEL BITS -----------> lsb */
				/* ---- ---- ---- ---- ---- ---- ---- ---- */
   MESA_FORMAT_RGBA8888,	/* RRRR RRRR GGGG GGGG BBBB BBBB AAAA AAAA */
   MESA_FORMAT_RGBA8888_REV,	/* AAAA AAAA BBBB BBBB GGGG GGGG RRRR RRRR */
   MESA_FORMAT_ARGB8888,	/* AAAA AAAA RRRR RRRR GGGG GGGG BBBB BBBB */
   MESA_FORMAT_ARGB8888_REV,	/* BBBB BBBB GGGG GGGG RRRR RRRR AAAA AAAA */
   MESA_FORMAT_RGBX8888,	/* RRRR RRRR GGGG GGGG BBBB BBBB XXXX XXXX */
   MESA_FORMAT_RGBX8888_REV,	/* xxxx xxxx BBBB BBBB GGGG GGGG RRRR RRRR */
   MESA_FORMAT_XRGB8888,	/* xxxx xxxx RRRR RRRR GGGG GGGG BBBB BBBB */
   MESA_FORMAT_XRGB8888_REV,	/* BBBB BBBB GGGG GGGG RRRR RRRR xxxx xxxx */
   MESA_FORMAT_RGB888,		/*           RRRR RRRR GGGG GGGG BBBB BBBB */
   MESA_FORMAT_BGR888,		/*           BBBB BBBB GGGG GGGG RRRR RRRR */
   MESA_FORMAT_RGB565,		/*                     RRRR RGGG GGGB BBBB */
   MESA_FORMAT_RGB565_REV,	/*                     GGGB BBBB RRRR RGGG */
   MESA_FORMAT_ARGB4444,	/*                     AAAA RRRR GGGG BBBB */
   MESA_FORMAT_ARGB4444_REV,	/*                     GGGG BBBB AAAA RRRR */
   MESA_FORMAT_RGBA5551,        /*                     RRRR RGGG GGBB BBBA */
   MESA_FORMAT_ARGB1555,	/*                     ARRR RRGG GGGB BBBB */
   MESA_FORMAT_ARGB1555_REV,	/*                     GGGB BBBB ARRR RRGG */
   MESA_FORMAT_AL44,		/*                               AAAA LLLL */
   MESA_FORMAT_AL88,		/*                     AAAA AAAA LLLL LLLL */
   MESA_FORMAT_AL88_REV,	/*                     LLLL LLLL AAAA AAAA */
   MESA_FORMAT_AL1616,          /* AAAA AAAA AAAA AAAA LLLL LLLL LLLL LLLL */
   MESA_FORMAT_AL1616_REV,      /* LLLL LLLL LLLL LLLL AAAA AAAA AAAA AAAA */
   MESA_FORMAT_RGB332,		/*                               RRRG GGBB */
   MESA_FORMAT_A8,		/*                               AAAA AAAA */
   MESA_FORMAT_A16,             /*                     AAAA AAAA AAAA AAAA */
   MESA_FORMAT_L8,		/*                               LLLL LLLL */
   MESA_FORMAT_L16,             /*                     LLLL LLLL LLLL LLLL */
   MESA_FORMAT_I8,		/*                               IIII IIII */
   MESA_FORMAT_I16,             /*                     IIII IIII IIII IIII */
   MESA_FORMAT_YCBCR,		/*                     YYYY YYYY UorV UorV */
   MESA_FORMAT_YCBCR_REV,	/*                     UorV UorV YYYY YYYY */
   MESA_FORMAT_R8,		/*                               RRRR RRRR */
   MESA_FORMAT_GR88,		/*                     GGGG GGGG RRRR RRRR */
   MESA_FORMAT_RG88,    	/*                     RRRR RRRR GGGG GGGG */
   MESA_FORMAT_R16,		/*                     RRRR RRRR RRRR RRRR */
   MESA_FORMAT_RG1616,		/* GGGG GGGG GGGG GGGG RRRR RRRR RRRR RRRR */
   MESA_FORMAT_RG1616_REV,	/* RRRR RRRR RRRR RRRR GGGG GGGG GGGG GGGG */
   MESA_FORMAT_ARGB2101010,     /* AARR RRRR RRRR GGGG GGGG GGBB BBBB BBBB */
   MESA_FORMAT_Z24_S8,          /* ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ SSSS SSSS */
   MESA_FORMAT_S8_Z24,          /* SSSS SSSS ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ */
   MESA_FORMAT_Z16,             /*                     ZZZZ ZZZZ ZZZZ ZZZZ */
   MESA_FORMAT_X8_Z24,          /* xxxx xxxx ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ */
   MESA_FORMAT_Z24_X8,          /* ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ xxxx xxxx */
   MESA_FORMAT_Z32,             /* ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ */
   MESA_FORMAT_S8,              /*                               SSSS SSSS */
   /*@}*/

   /**
    * \name 8-bit/channel sRGB formats
    */
   /*@{*/
   MESA_FORMAT_SRGB8,           /*           RRRR RRRR GGGG GGGG BBBB BBBB */
   MESA_FORMAT_SRGBA8,          /* RRRR RRRR GGGG GGGG BBBB BBBB AAAA AAAA */
   MESA_FORMAT_SARGB8,          /* AAAA AAAA RRRR RRRR GGGG GGGG BBBB BBBB */
   MESA_FORMAT_SL8,             /*                               LLLL LLLL */
   MESA_FORMAT_SLA8,            /*                     AAAA AAAA LLLL LLLL */
   MESA_FORMAT_SRGB_DXT1,
   MESA_FORMAT_SRGBA_DXT1,
   MESA_FORMAT_SRGBA_DXT3,
   MESA_FORMAT_SRGBA_DXT5,
   /*@}*/

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
   MESA_FORMAT_INTENSITY_FLOAT16,
   MESA_FORMAT_R_FLOAT32,
   MESA_FORMAT_R_FLOAT16,
   MESA_FORMAT_RG_FLOAT32,
   MESA_FORMAT_RG_FLOAT16,
   /*@}*/

   /**
    * \name Non-normalized signed integer formats.
    * XXX Note: these are just stand-ins for some better hardware
    * formats TBD such as BGRA or ARGB.
    */
   MESA_FORMAT_ALPHA_UINT8,
   MESA_FORMAT_ALPHA_UINT16,
   MESA_FORMAT_ALPHA_UINT32,
   MESA_FORMAT_ALPHA_INT8,
   MESA_FORMAT_ALPHA_INT16,
   MESA_FORMAT_ALPHA_INT32,

   MESA_FORMAT_INTENSITY_UINT8,
   MESA_FORMAT_INTENSITY_UINT16,
   MESA_FORMAT_INTENSITY_UINT32,
   MESA_FORMAT_INTENSITY_INT8,
   MESA_FORMAT_INTENSITY_INT16,
   MESA_FORMAT_INTENSITY_INT32,

   MESA_FORMAT_LUMINANCE_UINT8,
   MESA_FORMAT_LUMINANCE_UINT16,
   MESA_FORMAT_LUMINANCE_UINT32,
   MESA_FORMAT_LUMINANCE_INT8,
   MESA_FORMAT_LUMINANCE_INT16,
   MESA_FORMAT_LUMINANCE_INT32,

   MESA_FORMAT_LUMINANCE_ALPHA_UINT8,
   MESA_FORMAT_LUMINANCE_ALPHA_UINT16,
   MESA_FORMAT_LUMINANCE_ALPHA_UINT32,
   MESA_FORMAT_LUMINANCE_ALPHA_INT8,
   MESA_FORMAT_LUMINANCE_ALPHA_INT16,
   MESA_FORMAT_LUMINANCE_ALPHA_INT32,

   MESA_FORMAT_R_INT8,
   MESA_FORMAT_RG_INT8,
   MESA_FORMAT_RGB_INT8,
   MESA_FORMAT_RGBA_INT8,
   MESA_FORMAT_R_INT16,
   MESA_FORMAT_RG_INT16,
   MESA_FORMAT_RGB_INT16,
   MESA_FORMAT_RGBA_INT16,
   MESA_FORMAT_R_INT32,
   MESA_FORMAT_RG_INT32,
   MESA_FORMAT_RGB_INT32,
   MESA_FORMAT_RGBA_INT32,

   /**
    * \name Non-normalized unsigned integer formats.
    */
   MESA_FORMAT_R_UINT8,
   MESA_FORMAT_RG_UINT8,
   MESA_FORMAT_RGB_UINT8,
   MESA_FORMAT_RGBA_UINT8,
   MESA_FORMAT_R_UINT16,
   MESA_FORMAT_RG_UINT16,
   MESA_FORMAT_RGB_UINT16,
   MESA_FORMAT_RGBA_UINT16,
   MESA_FORMAT_R_UINT32,
   MESA_FORMAT_RG_UINT32,
   MESA_FORMAT_RGB_UINT32,
   MESA_FORMAT_RGBA_UINT32,

                                  /* msb <------ TEXEL BITS -----------> lsb */
                                  /* ---- ---- ---- ---- ---- ---- ---- ---- */
   /**
    * \name Signed fixed point texture formats.
    */
   /*@{*/
   MESA_FORMAT_DUDV8,             /*                     DUDU DUDU DVDV DVDV */
   MESA_FORMAT_SIGNED_R8,         /*                               RRRR RRRR */
   MESA_FORMAT_SIGNED_RG88_REV,   /*                     GGGG GGGG RRRR RRRR */
   MESA_FORMAT_SIGNED_RGBX8888,   /* RRRR RRRR GGGG GGGG BBBB BBBB xxxx xxxx */
   MESA_FORMAT_SIGNED_RGBA8888,   /* RRRR RRRR GGGG GGGG BBBB BBBB AAAA AAAA */
   MESA_FORMAT_SIGNED_RGBA8888_REV,/*AAAA AAAA BBBB BBBB GGGG GGGG RRRR RRRR */
   MESA_FORMAT_SIGNED_R16,        /*                     RRRR RRRR RRRR RRRR */
   MESA_FORMAT_SIGNED_GR1616,     /* GGGG GGGG GGGG GGGG RRRR RRRR RRRR RRRR */
   MESA_FORMAT_SIGNED_RGB_16,     /* ushort[0]=R, ushort[1]=G, ushort[2]=B */
   MESA_FORMAT_SIGNED_RGBA_16,    /* ... */
   MESA_FORMAT_RGBA_16,           /* ... */
   /*@}*/

   /*@{*/
   MESA_FORMAT_RED_RGTC1,
   MESA_FORMAT_SIGNED_RED_RGTC1,
   MESA_FORMAT_RG_RGTC2,
   MESA_FORMAT_SIGNED_RG_RGTC2,
   /*@}*/

   /*@{*/
   MESA_FORMAT_L_LATC1,
   MESA_FORMAT_SIGNED_L_LATC1,
   MESA_FORMAT_LA_LATC2,
   MESA_FORMAT_SIGNED_LA_LATC2,
   /*@}*/

   MESA_FORMAT_ETC1_RGB8,

   MESA_FORMAT_SIGNED_A8,         /*                               AAAA AAAA */
   MESA_FORMAT_SIGNED_L8,         /*                               LLLL LLLL */
   MESA_FORMAT_SIGNED_AL88,       /*                     AAAA AAAA LLLL LLLL */
   MESA_FORMAT_SIGNED_I8,         /*                               IIII IIII */
   MESA_FORMAT_SIGNED_A16,        /*                     AAAA AAAA AAAA AAAA */
   MESA_FORMAT_SIGNED_L16,        /*                     LLLL LLLL LLLL LLLL */
   MESA_FORMAT_SIGNED_AL1616,     /* AAAA AAAA AAAA AAAA LLLL LLLL LLLL LLLL */
   MESA_FORMAT_SIGNED_I16,        /*                     IIII IIII IIII IIII */

   MESA_FORMAT_RGB9_E5_FLOAT,
   MESA_FORMAT_R11_G11_B10_FLOAT,

   MESA_FORMAT_Z32_FLOAT,
   MESA_FORMAT_Z32_FLOAT_X24S8,

   MESA_FORMAT_ARGB2101010_UINT,

   MESA_FORMAT_COUNT
} gl_format;


extern const char *
_mesa_get_format_name(gl_format format);

extern GLint
_mesa_get_format_bytes(gl_format format);

extern GLint
_mesa_get_format_bits(gl_format format, GLenum pname);

extern GLuint
_mesa_get_format_max_bits(gl_format format);

extern GLenum
_mesa_get_format_datatype(gl_format format);

extern GLenum
_mesa_get_format_base_format(gl_format format);

extern void
_mesa_get_format_block_size(gl_format format, GLuint *bw, GLuint *bh);

extern GLboolean
_mesa_is_format_compressed(gl_format format);

extern GLboolean
_mesa_is_format_packed_depth_stencil(gl_format format);

extern GLboolean
_mesa_is_format_integer_color(gl_format format);

extern GLenum
_mesa_get_format_color_encoding(gl_format format);

extern GLuint
_mesa_format_image_size(gl_format format, GLsizei width,
                        GLsizei height, GLsizei depth);

extern uint64_t
_mesa_format_image_size64(gl_format format, GLsizei width,
                          GLsizei height, GLsizei depth);

extern GLint
_mesa_format_row_stride(gl_format format, GLsizei width);

extern void
_mesa_format_to_type_and_comps(gl_format format,
                               GLenum *datatype, GLuint *comps);

extern void
_mesa_test_formats(void);

extern gl_format
_mesa_get_srgb_format_linear(gl_format format);

extern gl_format
_mesa_get_uncompressed_format(gl_format format);

extern GLuint
_mesa_format_num_components(gl_format format);

GLboolean
_mesa_format_matches_format_and_type(gl_format gl_format,
				     GLenum format, GLenum type);


#ifdef __cplusplus
}
#endif

#endif /* FORMATS_H */
