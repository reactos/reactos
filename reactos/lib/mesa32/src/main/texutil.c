
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
 *
 * Authors:
 *    Gareth Hughes
 */

/*
 * Description:
 * Functions for texture image conversion.  This takes care of converting
 * typical GL_RGBA/GLubyte textures into hardware-specific formats.
 * We can handle non-standard row strides and pixel unpacking parameters.
 */


#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "enums.h"
#include "image.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"
#include "texformat.h"
#include "texutil.h"


#define DEBUG_TEXUTIL 0


#ifdef MESA_BIG_ENDIAN
#define APPEND16( a, b )	( (a) << 16 | (b) )
#else
#define APPEND16( a, b )	( (a) | (b) << 16 )
#endif


struct convert_info {
   GLint xoffset, yoffset, zoffset;	/* Subimage offset */
   GLint width, height, depth;		/* Subimage region */

   GLint dstImageWidth, dstImageHeight;	/* Dest image size */
                                        /* Needed for subimage replacement */
   GLenum format, type;                 /* Source (user) format and type */

   const struct gl_pixelstore_attrib *unpacking;

   const GLvoid *srcImage;
   GLvoid *dstImage;

   GLint index;
};

typedef GLboolean (*convert_func)( const struct convert_info *convert );

/* bitvalues for convert->index */
#define CONVERT_STRIDE_BIT	0x1
#define CONVERT_UNPACKING_BIT	0x2



/* =============================================================
 * Convert to RGBA8888 textures:
 */

#define DST_TYPE		GLuint
#define DST_TEXELS_PER_DWORD	1

#define CONVERT_TEXEL( dst, src )					\
	dst = PACK_COLOR_8888_LE( src[3], src[2], src[1], src[0] )

#define CONVERT_DIRECT

#define SRC_TEXEL_BYTES		4

#define TAG(x) x##_rgba8888_direct
#define PRESERVE_DST_TYPE
#include "texutil_tmp.h"


#define CONVERT_TEXEL( dst, src )					\
	dst = PACK_COLOR_8888_LE( src[0], src[1], src[2], src[3] )

#define CONVERT_TEXEL_DWORD( dst, src )		CONVERT_TEXEL( dst, src )

#define SRC_TEXEL_BYTES		4

#define TAG(x) x##_abgr8888_to_rgba8888
#define PRESERVE_DST_TYPE
#include "texutil_tmp.h"


#define CONVERT_TEXEL( dst, src )					\
	dst = PACK_COLOR_8888_LE( src[0], src[1], src[2], 0xff )

#define CONVERT_TEXEL_DWORD( dst, src )		CONVERT_TEXEL( dst, src )

#define SRC_TEXEL_BYTES		3

#define TAG(x) x##_bgr888_to_rgba8888
#include "texutil_tmp.h"


#define CONVERT_RGBA8888( name )					\
static GLboolean							\
convert_##name##_rgba8888( const struct convert_info *convert )		\
{									\
   convert_func *tab;							\
   GLint index = convert->index;					\
									\
   if ( convert->format == GL_ABGR_EXT &&				\
	convert->type == GL_UNSIGNED_INT_8_8_8_8_REV )			\
   {									\
      tab = name##_tab_rgba8888_direct;					\
   }									\
   else if ( convert->format == GL_RGBA &&				\
	     ( convert->type == GL_UNSIGNED_BYTE ||			\
	       convert->type == GL_UNSIGNED_INT_8_8_8_8 ) )		\
   {									\
      tab = name##_tab_abgr8888_to_rgba8888;				\
   }									\
   else if ( convert->format == GL_RGB &&				\
	     convert->type == GL_UNSIGNED_BYTE )			\
   {									\
      tab = name##_tab_bgr888_to_rgba8888;				\
   }									\
   else									\
   {									\
      /* Can't handle this source format/type combination */		\
      return GL_FALSE;							\
   }									\
									\
   return tab[index]( convert );					\
}

CONVERT_RGBA8888( texsubimage2d )
CONVERT_RGBA8888( texsubimage3d )



/* =============================================================
 * Convert to ARGB8888 textures:
 */

#define DST_TYPE		GLuint
#define DST_TEXELS_PER_DWORD	1

#define CONVERT_TEXEL( dst, src )					\
	dst = PACK_COLOR_8888_LE( src[3], src[2], src[1], src[0] )

#define CONVERT_DIRECT

#define SRC_TEXEL_BYTES		4

#define TAG(x) x##_argb8888_direct
#define PRESERVE_DST_TYPE
#include "texutil_tmp.h"


#define CONVERT_TEXEL( dst, src )					\
	dst = PACK_COLOR_8888_LE( src[3], src[0], src[1], src[2] )

#define CONVERT_TEXEL_DWORD( dst, src )		CONVERT_TEXEL( dst, src )

#define SRC_TEXEL_BYTES		4

#define TAG(x) x##_abgr8888_to_argb8888
#define PRESERVE_DST_TYPE
#include "texutil_tmp.h"


#define CONVERT_TEXEL( dst, src )					\
	dst = PACK_COLOR_8888_LE( 0xff, src[0], src[1], src[2] )

#define CONVERT_TEXEL_DWORD( dst, src )		CONVERT_TEXEL( dst, src )

#define SRC_TEXEL_BYTES		3

#define TAG(x) x##_bgr888_to_argb8888
#include "texutil_tmp.h"


#define CONVERT_ARGB8888( name )					\
static GLboolean							\
convert_##name##_argb8888( const struct convert_info *convert )		\
{									\
   convert_func *tab;							\
   GLint index = convert->index;					\
									\
   if ( convert->format == GL_BGRA &&					\
	convert->type == GL_UNSIGNED_INT_8_8_8_8_REV )			\
   {									\
      tab = name##_tab_argb8888_direct;					\
   }									\
   else if ( convert->format == GL_RGBA &&				\
	     convert->type == GL_UNSIGNED_BYTE )			\
   {									\
      tab = name##_tab_abgr8888_to_argb8888;				\
   }									\
   else if ( convert->format == GL_RGB &&				\
	     convert->type == GL_UNSIGNED_BYTE )			\
   {									\
      tab = name##_tab_bgr888_to_argb8888;				\
   }									\
   else									\
   {									\
      /* Can't handle this source format/type combination */		\
      return GL_FALSE;							\
   }									\
									\
   return tab[index]( convert );					\
}

CONVERT_ARGB8888( texsubimage2d )
CONVERT_ARGB8888( texsubimage3d )



/* =============================================================
 * Convert to RGB888 textures:
 */

static GLboolean
convert_texsubimage2d_rgb888( const struct convert_info *convert )
{
   /* This is a placeholder for now...
    */
   return GL_FALSE;
}

static GLboolean
convert_texsubimage3d_rgb888( const struct convert_info *convert )
{
   /* This is a placeholder for now...
    */
   return GL_FALSE;
}



/* =============================================================
 * Convert to RGB565 textures:
 */

#define DST_TYPE		GLushort
#define DST_TEXELS_PER_DWORD	2

#define CONVERT_TEXEL( dst, src )					\
	dst = PACK_COLOR_565_LE( src[0], src[1], src[2] )

#define CONVERT_DIRECT

#define SRC_TEXEL_BYTES		2

#define TAG(x) x##_rgb565_direct
#define PRESERVE_DST_TYPE
#include "texutil_tmp.h"


#define CONVERT_TEXEL( dst, src )					\
	dst = PACK_COLOR_565_LE( src[0], src[1], src[2] )

#define CONVERT_TEXEL_DWORD( dst, src )					\
	dst = APPEND16( PACK_COLOR_565_LE( src[0], src[1], src[2] ),	\
			PACK_COLOR_565_LE( src[3], src[4], src[5] ) )

#define SRC_TEXEL_BYTES		3

#define TAG(x) x##_bgr888_to_rgb565
#define PRESERVE_DST_TYPE
#include "texutil_tmp.h"


#define CONVERT_TEXEL( dst, src )					\
	dst = PACK_COLOR_565_LE( src[0], src[1], src[2] )

#define CONVERT_TEXEL_DWORD( dst, src )					\
	dst = APPEND16( PACK_COLOR_565_LE( src[0], src[1], src[2] ),	\
			PACK_COLOR_565_LE( src[4], src[5], src[6] ) )

#define SRC_TEXEL_BYTES		4

#define TAG(x) x##_abgr8888_to_rgb565
#include "texutil_tmp.h"


#define CONVERT_RGB565( name )						\
static GLboolean							\
convert_##name##_rgb565( const struct convert_info *convert )		\
{									\
   convert_func *tab;							\
   GLint index = convert->index;					\
									\
   if ( convert->format == GL_RGB &&					\
	convert->type == GL_UNSIGNED_SHORT_5_6_5 )			\
   {									\
      tab = name##_tab_rgb565_direct;					\
   }									\
   else if ( convert->format == GL_RGB &&				\
	     convert->type == GL_UNSIGNED_BYTE )			\
   {									\
      tab = name##_tab_bgr888_to_rgb565;				\
   }									\
   else if ( convert->format == GL_RGBA &&				\
	     convert->type == GL_UNSIGNED_BYTE )			\
   {									\
      tab = name##_tab_abgr8888_to_rgb565;				\
   }									\
   else									\
   {									\
      /* Can't handle this source format/type combination */		\
      return GL_FALSE;							\
   }									\
									\
   return tab[index]( convert );					\
}

CONVERT_RGB565( texsubimage2d )
CONVERT_RGB565( texsubimage3d )



/* =============================================================
 * Convert to ARGB4444 textures:
 */

#define DST_TYPE		GLushort
#define DST_TEXELS_PER_DWORD	2

#define CONVERT_TEXEL( dst, src )					\
	dst = PACK_COLOR_4444_LE( src[3], src[0], src[1], src[2] )

#define CONVERT_DIRECT

#define SRC_TEXEL_BYTES		2

#define TAG(x) x##_argb4444_direct
#define PRESERVE_DST_TYPE
#include "texutil_tmp.h"


#define CONVERT_TEXEL( dst, src )					\
	dst = PACK_COLOR_4444_LE( src[3], src[0], src[1], src[2] )

#define CONVERT_TEXEL_DWORD( dst, src )					\
	dst = APPEND16( PACK_COLOR_4444_LE( src[3], src[0], src[1], src[2] ),	\
			PACK_COLOR_4444_LE( src[7], src[4], src[5], src[6] ) )

#define SRC_TEXEL_BYTES		4

#define TAG(x) x##_abgr8888_to_argb4444
#include "texutil_tmp.h"


#define CONVERT_ARGB4444( name )					\
static GLboolean							\
convert_##name##_argb4444( const struct convert_info *convert )		\
{									\
   convert_func *tab;							\
   GLint index = convert->index;					\
									\
   if ( convert->format == GL_BGRA &&					\
	convert->type == GL_UNSIGNED_SHORT_4_4_4_4_REV )		\
   {									\
      tab = name##_tab_argb4444_direct;					\
   }									\
   else if ( convert->format == GL_RGBA &&				\
	     convert->type == GL_UNSIGNED_BYTE )			\
   {									\
      tab = name##_tab_abgr8888_to_argb4444;				\
   }									\
   else									\
   {									\
      /* Can't handle this source format/type combination */		\
      return GL_FALSE;							\
   }									\
									\
   return tab[index]( convert );					\
}

CONVERT_ARGB4444( texsubimage2d )
CONVERT_ARGB4444( texsubimage3d )



/* =============================================================
 * Convert to ARGB1555 textures:
 */

#define DST_TYPE		GLushort
#define DST_TEXELS_PER_DWORD	2

#define CONVERT_TEXEL( dst, src )					\
	dst = PACK_COLOR_1555_LE( src[3], src[0], src[1], src[2] )

#define CONVERT_DIRECT

#define SRC_TEXEL_BYTES		2

#define TAG(x) x##_argb1555_direct
#define PRESERVE_DST_TYPE
#include "texutil_tmp.h"


#ifdef MESA_BIG_ENDIAN

#define CONVERT_TEXEL( dst, src )					\
	{ const GLushort s = *(GLushort *)src;				\
	  dst = (s >> 9) | ((s & 0x1ff) << 7); }

#define CONVERT_TEXEL_DWORD( dst, src )					\
	{ const GLuint s = ((fi_type *)src)->i;				\
	  dst = (((s & 0xfe00fe00) >> 9) |				\
		 ((s & 0x01ff01ff) << 7)); }

#else

#define CONVERT_TEXEL( dst, src )					\
	{ const GLushort s = *(GLushort *)src;				\
	  dst = (s >> 1) | ((s & 1) << 15); }

#define CONVERT_TEXEL_DWORD( dst, src )					\
	{ const GLuint s = ((fi_type *)src)->i;				\
	  dst = (((s & 0xfffefffe) >> 1) |				\
		 ((s & 0x00010001) << 15)); }

#endif

#define SRC_TEXEL_BYTES		2

#define TAG(x) x##_rgba5551_to_argb1555
#define PRESERVE_DST_TYPE
#include "texutil_tmp.h"


#define CONVERT_TEXEL( dst, src )					\
	dst = PACK_COLOR_1555_LE( src[3], src[0], src[1], src[2] )

#define CONVERT_TEXEL_DWORD( dst, src )					\
	dst = APPEND16( PACK_COLOR_1555_LE( src[3], src[0], src[1], src[2] ),	\
			PACK_COLOR_1555_LE( src[7], src[4], src[5], src[6] ) )

#define SRC_TEXEL_BYTES		4

#define TAG(x) x##_abgr8888_to_argb1555
#include "texutil_tmp.h"


#define CONVERT_ARGB1555( name )					\
static GLboolean							\
convert_##name##_argb1555( const struct convert_info *convert )		\
{									\
   convert_func *tab;							\
   GLint index = convert->index;					\
									\
   if ( convert->format == GL_BGRA &&					\
	convert->type == GL_UNSIGNED_SHORT_1_5_5_5_REV )		\
   {									\
      tab = name##_tab_argb1555_direct;					\
   }									\
   else if ( convert->format == GL_RGBA &&				\
	     convert->type == GL_UNSIGNED_SHORT_5_5_5_1 )		\
   {									\
      tab = name##_tab_rgba5551_to_argb1555;				\
   }									\
   else if ( convert->format == GL_RGBA &&				\
	     convert->type == GL_UNSIGNED_BYTE )			\
   {									\
      tab = name##_tab_abgr8888_to_argb1555;				\
   }									\
   else									\
   {									\
      /* Can't handle this source format/type combination */		\
      return GL_FALSE;							\
   }									\
									\
   return tab[index]( convert );					\
}

CONVERT_ARGB1555( texsubimage2d )
CONVERT_ARGB1555( texsubimage3d )



/* =============================================================
 * AL88 textures:
 */

#define DST_TYPE		GLushort
#define DST_TEXELS_PER_DWORD	2

#define CONVERT_TEXEL( dst, src )					\
	dst = PACK_COLOR_88_LE( src[0], src[1] )

#define CONVERT_DIRECT

#define SRC_TEXEL_BYTES		2

#define TAG(x) x##_al88_direct
#define PRESERVE_DST_TYPE
#include "texutil_tmp.h"


#define CONVERT_TEXEL( dst, src )					\
	dst = PACK_COLOR_88_LE( src[0], 0x00 )

#define CONVERT_TEXEL_DWORD( dst, src )					\
	dst = APPEND16( PACK_COLOR_88_LE( src[0], 0x00 ),			\
			PACK_COLOR_88_LE( src[1], 0x00 ) )

#define SRC_TEXEL_BYTES		1

#define TAG(x) x##_a8_to_al88
#define PRESERVE_DST_TYPE
#include "texutil_tmp.h"


#define CONVERT_TEXEL( dst, src )					\
	dst = PACK_COLOR_88_LE( 0xff, src[0] )

#define CONVERT_TEXEL_DWORD( dst, src )					\
	dst = APPEND16( PACK_COLOR_88_LE( 0xff, src[0] ),			\
			PACK_COLOR_88_LE( 0xff, src[1] ) )

#define SRC_TEXEL_BYTES		1

#define TAG(x) x##_l8_to_al88
#define PRESERVE_DST_TYPE
#include "texutil_tmp.h"


#define CONVERT_TEXEL( dst, src )					\
	dst = PACK_COLOR_88_LE( src[3], src[0] )

#define CONVERT_TEXEL_DWORD( dst, src )					\
	dst = APPEND16( PACK_COLOR_88_LE( src[3], src[0] ),		\
			PACK_COLOR_88_LE( src[7], src[4] ) )

#define SRC_TEXEL_BYTES		4

#define TAG(x) x##_abgr8888_to_al88
#include "texutil_tmp.h"


#define CONVERT_AL88( name )						\
static GLboolean							\
convert_##name##_al88( const struct convert_info *convert )		\
{									\
   convert_func *tab;							\
   GLint index = convert->index;					\
									\
   if ( convert->format == GL_LUMINANCE_ALPHA &&			\
	convert->type == GL_UNSIGNED_BYTE )				\
   {									\
      tab = name##_tab_al88_direct;					\
   }									\
   else if ( convert->format == GL_ALPHA &&				\
	     convert->type == GL_UNSIGNED_BYTE )			\
   {									\
      tab = name##_tab_a8_to_al88;					\
   }									\
   else if ( convert->format == GL_LUMINANCE &&				\
	     convert->type == GL_UNSIGNED_BYTE )			\
   {									\
      tab = name##_tab_l8_to_al88;					\
   }									\
   else if ( convert->format == GL_RGBA &&				\
	     convert->type == GL_UNSIGNED_BYTE )			\
   {									\
      tab = name##_tab_abgr8888_to_al88;				\
   }									\
   else									\
   {									\
      /* Can't handle this source format/type combination */		\
      return GL_FALSE;							\
   }									\
									\
   return tab[index]( convert );					\
}

CONVERT_AL88( texsubimage2d )
CONVERT_AL88( texsubimage3d )



/* =============================================================
 * Convert to RGB332 textures:
 */

static GLboolean
convert_texsubimage2d_rgb332( const struct convert_info *convert )
{
   /* This is a placeholder for now...
    */
   return GL_FALSE;
}

static GLboolean
convert_texsubimage3d_rgb332( const struct convert_info *convert )
{
   /* This is a placeholder for now...
    */
   return GL_FALSE;
}



/* =============================================================
 * CI8 (and all other single-byte texel) textures:
 */

#define DST_TYPE		GLubyte
#define DST_TEXELS_PER_DWORD	4

#define CONVERT_TEXEL( dst, src )	dst = src[0]

#define CONVERT_DIRECT

#define SRC_TEXEL_BYTES		1

#define TAG(x) x##_ci8_direct
#include "texutil_tmp.h"


#define CONVERT_CI8( name )						\
static GLboolean							\
convert_##name##_ci8( const struct convert_info *convert )		\
{									\
   convert_func *tab;							\
   GLint index = convert->index;					\
									\
   if ( ( convert->format == GL_ALPHA ||				\
	  convert->format == GL_LUMINANCE ||				\
	  convert->format == GL_INTENSITY ||				\
	  convert->format == GL_COLOR_INDEX ) &&			\
	convert->type == GL_UNSIGNED_BYTE )				\
   {									\
      tab = name##_tab_ci8_direct;					\
   }									\
   else									\
   {									\
      /* Can't handle this source format/type combination */		\
      return GL_FALSE;							\
   }									\
									\
   return tab[index]( convert );					\
}

CONVERT_CI8( texsubimage2d )
CONVERT_CI8( texsubimage3d )


/* =============================================================
 * convert to YCBCR textures:
 */

#define DST_TYPE		GLushort
#define DST_TEXELS_PER_DWORD	2

#define CONVERT_TEXEL( dst, src ) \
   dst = (src[0] << 8) | src[1];

#define CONVERT_DIRECT

#define SRC_TEXEL_BYTES		2

#define TAG(x) x##_ycbcr_direct
#include "texutil_tmp.h"


#define CONVERT_YCBCR( name )						\
static GLboolean							\
convert_##name##_ycbcr( const struct convert_info *convert )		\
{									\
   convert_func *tab;							\
   GLint index = convert->index;					\
									\
   if (convert->format != GL_YCBCR_MESA) {				\
      /* Can't handle this source format/type combination */		\
      return GL_FALSE;							\
   }      								\
   tab = name##_tab_ycbcr_direct;					\
									\
   return tab[index]( convert );					\
}

CONVERT_YCBCR( texsubimage2d )
CONVERT_YCBCR( texsubimage3d )


/* =============================================================
 * convert to YCBCR_REV textures:
 */

#define DST_TYPE		GLushort
#define DST_TEXELS_PER_DWORD	2

#define CONVERT_TEXEL( dst, src ) \
   dst = (src[1] << 8) | src[0];

#define CONVERT_DIRECT

#define SRC_TEXEL_BYTES		2

#define TAG(x) x##_ycbcr_rev_direct
#include "texutil_tmp.h"


#define CONVERT_YCBCR_REV( name )					\
static GLboolean							\
convert_##name##_ycbcr_rev( const struct convert_info *convert )	\
{									\
   convert_func *tab;							\
   GLint index = convert->index;					\
									\
   if (convert->format != GL_YCBCR_MESA) {				\
      /* Can't handle this source format/type combination */		\
      return GL_FALSE;							\
   }      								\
   tab = name##_tab_ycbcr_rev_direct;					\
									\
   return tab[index]( convert );					\
}

CONVERT_YCBCR_REV( texsubimage2d )
CONVERT_YCBCR_REV( texsubimage3d )



/* =============================================================
 * Global entry points
 */

static convert_func convert_texsubimage2d_tab[] = {
   convert_texsubimage2d_rgba8888,
   convert_texsubimage2d_argb8888,
   convert_texsubimage2d_rgb888,
   convert_texsubimage2d_rgb565,
   convert_texsubimage2d_argb4444,
   convert_texsubimage2d_argb1555,
   convert_texsubimage2d_al88,
   convert_texsubimage2d_rgb332,
   convert_texsubimage2d_ci8,		/* These are all the same... */
   convert_texsubimage2d_ci8,
   convert_texsubimage2d_ci8,
   convert_texsubimage2d_ci8,
   convert_texsubimage2d_ycbcr,
   convert_texsubimage2d_ycbcr_rev,
};

static convert_func convert_texsubimage3d_tab[] = {
   convert_texsubimage3d_rgba8888,
   convert_texsubimage3d_argb8888,
   convert_texsubimage3d_rgb888,
   convert_texsubimage3d_rgb565,
   convert_texsubimage3d_argb4444,
   convert_texsubimage3d_argb1555,
   convert_texsubimage3d_al88,
   convert_texsubimage3d_rgb332,
   convert_texsubimage3d_ci8,		/* These are all the same... */
   convert_texsubimage3d_ci8,
   convert_texsubimage3d_ci8,
   convert_texsubimage3d_ci8,
   convert_texsubimage3d_ycbcr,
   convert_texsubimage3d_ycbcr_rev,
};


/* See if we need to care about the pixel store attributes when we're
 * converting the texture image.  This should be stored as
 * unpacking->_SomeBoolean and updated when the values change, to avoid
 * testing every time...
 */
static INLINE GLboolean
convert_needs_unpacking( const struct gl_pixelstore_attrib *unpacking,
		       GLenum format, GLenum type )
{
   if ( ( unpacking->Alignment == 1 ||
	  ( unpacking->Alignment == 4 &&   /* Pick up the common Q3A case... */
	    format == GL_RGBA && type == GL_UNSIGNED_BYTE ) ) &&
	unpacking->RowLength == 0 &&
	unpacking->SkipPixels == 0 &&
	unpacking->SkipRows == 0 &&
	unpacking->ImageHeight == 0 &&
	unpacking->SkipImages == 0 &&
	unpacking->SwapBytes == GL_FALSE &&
	unpacking->LsbFirst == GL_FALSE ) {
      return GL_FALSE;
   } else {
      return GL_TRUE;
   }
}


GLboolean
_mesa_convert_texsubimage1d( GLint mesaFormat,
			     GLint xoffset,
			     GLint width,
			     GLenum format, GLenum type,
			     const struct gl_pixelstore_attrib *unpacking,
			     const GLvoid *srcImage, GLvoid *dstImage )
{
   struct convert_info convert;

   ASSERT( unpacking );
   ASSERT( srcImage );
   ASSERT( dstImage );

   ASSERT( mesaFormat >= MESA_FORMAT_RGBA8888 );
   ASSERT( mesaFormat <= MESA_FORMAT_YCBCR_REV );

   /* Make it easier to pass all the parameters around.
    */
   convert.xoffset = xoffset;
   convert.yoffset = 0;
   convert.width = width;
   convert.height = 1;
   convert.format = format;
   convert.type = type;
   convert.unpacking = unpacking;
   convert.srcImage = srcImage;
   convert.dstImage = dstImage;

   convert.index = 0;

   if ( convert_needs_unpacking( unpacking, format, type ) )
      convert.index |= CONVERT_UNPACKING_BIT;

   ASSERT(convert.index < 4);

   return convert_texsubimage2d_tab[mesaFormat]( &convert );
}


/* Convert a user's 2D image into a texture image.  This basically
 * repacks pixel data into the special texture formats used by core Mesa
 * and the DRI drivers.  This function can do full images or subimages.
 *
 * We return a boolean because this function may not accept some kinds
 * of source image formats and/or types.  For example, if the incoming
 * format/type = GL_BGR, GL_UNSIGNED_INT this function probably won't
 * be able to do the conversion.
 *
 * In that case, the incoming image should first be simplified to one of
 * the "canonical" formats (GL_ALPHA, GL_LUMINANCE, GL_LUMINANCE_ALPHA,
 * GL_INTENSITY, GL_RGB, GL_RGBA) and types (GL_CHAN).  We can do that
 * with the _mesa_transfer_teximage() function.  That function will also
 * do image transfer operations such as scale/bias and convolution.
 *
 * \param
 *   mesaFormat - one of the MESA_FORMAT_* values from texformat.h
 *   xoffset, yoffset - position in dest image to put data
 *   width, height - incoming image size, also size of dest region.
 *   dstImageWidth - width (row stride) of dest image in pixels
 *   format, type - incoming image format and type
 *   unpacking - describes incoming image unpacking
 *   srcImage - pointer to source image
 *   destImage - pointer to dest image
 */
GLboolean
_mesa_convert_texsubimage2d( GLint mesaFormat,  /* dest */
			     GLint xoffset, GLint yoffset,
			     GLint width, GLint height,
			     GLint destImageWidth,
			     GLenum format, GLenum type,  /* source */
			     const struct gl_pixelstore_attrib *unpacking,
			     const GLvoid *srcImage, GLvoid *dstImage )
{
   struct convert_info convert;

   ASSERT( unpacking );
   ASSERT( srcImage );
   ASSERT( dstImage );

   ASSERT( mesaFormat >= MESA_FORMAT_RGBA8888 );
   ASSERT( mesaFormat <= MESA_FORMAT_YCBCR_REV );

   /* Make it easier to pass all the parameters around.
    */
   convert.xoffset = xoffset;
   convert.yoffset = yoffset;
   convert.width = width;
   convert.height = height;
   convert.dstImageWidth = destImageWidth;
   convert.format = format;
   convert.type = type;
   convert.unpacking = unpacking;
   convert.srcImage = srcImage;
   convert.dstImage = dstImage;

   convert.index = 0;

   if ( convert_needs_unpacking( unpacking, format, type ) )
      convert.index |= CONVERT_UNPACKING_BIT;

   if ( width != destImageWidth )
      convert.index |= CONVERT_STRIDE_BIT;

   return convert_texsubimage2d_tab[mesaFormat]( &convert );
}

GLboolean
_mesa_convert_texsubimage3d( GLint mesaFormat,  /* dest */
			     GLint xoffset, GLint yoffset, GLint zoffset,
			     GLint width, GLint height, GLint depth,
			     GLint dstImageWidth, GLint dstImageHeight,
			     GLenum format, GLenum type,  /* source */
			     const struct gl_pixelstore_attrib *unpacking,
			     const GLvoid *srcImage, GLvoid *dstImage )
{
   struct convert_info convert;

   ASSERT( unpacking );
   ASSERT( srcImage );
   ASSERT( dstImage );

   ASSERT( mesaFormat >= MESA_FORMAT_RGBA8888 );
   ASSERT( mesaFormat <= MESA_FORMAT_YCBCR_REV );

   /* Make it easier to pass all the parameters around.
    */
   convert.xoffset = xoffset;
   convert.yoffset = yoffset;
   convert.zoffset = zoffset;
   convert.width = width;
   convert.height = height;
   convert.depth = depth;
   convert.dstImageWidth = dstImageWidth;
   convert.dstImageHeight = dstImageHeight;
   convert.format = format;
   convert.type = type;
   convert.unpacking = unpacking;
   convert.srcImage = srcImage;
   convert.dstImage = dstImage;

   convert.index = 0;

   if ( convert_needs_unpacking( unpacking, format, type ) )
      convert.index |= CONVERT_UNPACKING_BIT;

   if ( width != dstImageWidth || height != dstImageHeight )
      convert.index |= CONVERT_STRIDE_BIT;

   return convert_texsubimage3d_tab[mesaFormat]( &convert );
}



/* Nearest filtering only (for broken hardware that can't support
 * all aspect ratios).  This can be made a lot faster, but I don't
 * really care enough...
 */
void _mesa_rescale_teximage2d( GLuint bytesPerPixel, GLuint dstRowStride,
			       GLint srcWidth, GLint srcHeight,
			       GLint dstWidth, GLint dstHeight,
			       const GLvoid *srcImage, GLvoid *dstImage )
{
   GLint row, col;

#define INNER_LOOP( TYPE, HOP, WOP )					\
   for ( row = 0 ; row < dstHeight ; row++ ) {				\
      GLint srcRow = row HOP hScale;					\
      for ( col = 0 ; col < dstWidth ; col++ ) {			\
	 GLint srcCol = col WOP wScale;					\
	 dst[col] = src[srcRow * srcWidth + srcCol];			\
      }									\
      dst = (TYPE *) ((GLubyte *) dst + dstRowStride);			\
   }									\

#define RESCALE_IMAGE( TYPE )						\
do {									\
   const TYPE *src = (const TYPE *)srcImage;				\
   TYPE *dst = (TYPE *)dstImage;					\
									\
   if ( srcHeight <= dstHeight ) {					\
      const GLint hScale = dstHeight / srcHeight;			\
      if ( srcWidth <= dstWidth ) {					\
	 const GLint wScale = dstWidth / srcWidth;			\
	 INNER_LOOP( TYPE, /, / );					\
      }									\
      else {								\
	 const GLint wScale = srcWidth / dstWidth;			\
	 INNER_LOOP( TYPE, /, * );					\
      }									\
   }									\
   else {								\
      const GLint hScale = srcHeight / dstHeight;			\
      if ( srcWidth <= dstWidth ) {					\
	 const GLint wScale = dstWidth / srcWidth;			\
	 INNER_LOOP( TYPE, *, / );					\
      }									\
      else {								\
	 const GLint wScale = srcWidth / dstWidth;			\
	 INNER_LOOP( TYPE, *, * );					\
      }									\
   }									\
} while (0)

   switch ( bytesPerPixel ) {
   case 4:
      RESCALE_IMAGE( GLuint );
      break;

   case 2:
      RESCALE_IMAGE( GLushort );
      break;

   case 1:
      RESCALE_IMAGE( GLubyte );
      break;
   default:
      _mesa_problem(NULL,"unexpected bytes/pixel in _mesa_rescale_teximage2d");
   }
}
