/**
 * \file texformat_tmp.h
 * Texel fetch functions template.
 * 
 * This template file is used by texformat.c to generate texel fetch functions
 * for 1-D, 2-D and 3-D texture images. 
 *
 * It should be expanded by definining \p DIM as the number texture dimensions
 * (1, 2 or 3).  According to the value of \p DIM a serie of macros is defined
 * for the texel lookup in the gl_texture_image::Data.
 * 
 * \sa texformat.c and FetchTexel.
 * 
 * \author Gareth Hughes
 * \author Brian Paul
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.0.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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


#if DIM == 1

#define CHAN_SRC( t, i, j, k, sz )					\
	((GLchan *)(t)->Data + (i) * (sz))
#define UBYTE_SRC( t, i, j, k, sz )					\
	((GLubyte *)(t)->Data + (i) * (sz))
#define USHORT_SRC( t, i, j, k )					\
	((GLushort *)(t)->Data + (i))
#define FLOAT_SRC( t, i, j, k )						\
	((GLfloat *)(t)->Data + (i))

#define FETCH(x) fetch_1d_texel_##x

#elif DIM == 2

#define CHAN_SRC( t, i, j, k, sz )					\
	((GLchan *)(t)->Data + ((t)->RowStride * (j) + (i)) * (sz))
#define UBYTE_SRC( t, i, j, k, sz )					\
	((GLubyte *)(t)->Data + ((t)->RowStride * (j) + (i)) * (sz))
#define USHORT_SRC( t, i, j, k )					\
	((GLushort *)(t)->Data + ((t)->RowStride * (j) + (i)))
#define FLOAT_SRC( t, i, j, k )						\
	((GLfloat *)(t)->Data + ((t)->RowStride * (j) + (i)))

#define FETCH(x) fetch_2d_texel_##x

#elif DIM == 3

#define CHAN_SRC( t, i, j, k, sz )					\
	(GLchan *)(t)->Data + (((t)->Height * (k) + (j)) *		\
				(t)->RowStride + (i)) * (sz)
#define UBYTE_SRC( t, i, j, k, sz )					\
	((GLubyte *)(t)->Data + (((t)->Height * (k) + (j)) *		\
				 (t)->RowStride + (i)) * (sz))
#define USHORT_SRC( t, i, j, k )					\
	((GLushort *)(t)->Data + (((t)->Height * (k) + (j)) *		\
				  (t)->RowStride + (i)))
#define FLOAT_SRC( t, i, j, k )						\
	((GLfloat *)(t)->Data + (((t)->Height * (k) + (j)) *		\
				  (t)->RowStride + (i)))

#define FETCH(x) fetch_3d_texel_##x

#else
#error	illegal number of texture dimensions
#endif


static void FETCH(rgba)( const struct gl_texture_image *texImage,
			 GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 4 );
   GLchan *rgba = (GLchan *) texel;
   COPY_CHAN4( rgba, src );
}

static void FETCH(rgb)( const struct gl_texture_image *texImage,
			GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 3 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = src[0];
   rgba[GCOMP] = src[1];
   rgba[BCOMP] = src[2];
   rgba[ACOMP] = CHAN_MAX;
}

static void FETCH(alpha)( const struct gl_texture_image *texImage,
			  GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 1 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = 0;
   rgba[GCOMP] = 0;
   rgba[BCOMP] = 0;
   rgba[ACOMP] = src[0];
}

static void FETCH(luminance)( const struct gl_texture_image *texImage,
			      GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 1 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = src[0];
   rgba[GCOMP] = src[0];
   rgba[BCOMP] = src[0];
   rgba[ACOMP] = CHAN_MAX;
}

static void FETCH(luminance_alpha)( const struct gl_texture_image *texImage,
				    GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 2 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = src[0];
   rgba[GCOMP] = src[0];
   rgba[BCOMP] = src[0];
   rgba[ACOMP] = src[1];
}

static void FETCH(intensity)( const struct gl_texture_image *texImage,
			      GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 1 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = src[0];
   rgba[GCOMP] = src[0];
   rgba[BCOMP] = src[0];
   rgba[ACOMP] = src[0];
}

static void FETCH(color_index)( const struct gl_texture_image *texImage,
				GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 1 );
   GLchan *index = (GLchan *) texel;
   *index = *src;
}

static void FETCH(depth_component)( const struct gl_texture_image *texImage,
				    GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLfloat *src = FLOAT_SRC( texImage, i, j, k );
   GLfloat *depth = (GLfloat *) texel;
   *depth = *src;
}

static void FETCH(rgba8888)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 4 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( src[3] );
   rgba[GCOMP] = UBYTE_TO_CHAN( src[2] );
   rgba[BCOMP] = UBYTE_TO_CHAN( src[1] );
   rgba[ACOMP] = UBYTE_TO_CHAN( src[0] );
}

static void FETCH(argb8888)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 4 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( src[2] );
   rgba[GCOMP] = UBYTE_TO_CHAN( src[1] );
   rgba[BCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[ACOMP] = UBYTE_TO_CHAN( src[3] );
}

static void FETCH(rgb888)( const struct gl_texture_image *texImage,
			   GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 3 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( src[2] );
   rgba[GCOMP] = UBYTE_TO_CHAN( src[1] );
   rgba[BCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[ACOMP] = CHAN_MAX;
}

static void FETCH(rgb565)( const struct gl_texture_image *texImage,
			   GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLushort *src = USHORT_SRC( texImage, i, j, k );
   const GLushort s = *src;
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( ((s >> 8) & 0xf8) * 255 / 0xf8 );
   rgba[GCOMP] = UBYTE_TO_CHAN( ((s >> 3) & 0xfc) * 255 / 0xfc );
   rgba[BCOMP] = UBYTE_TO_CHAN( ((s << 3) & 0xf8) * 255 / 0xf8 );
   rgba[ACOMP] = CHAN_MAX;
}

static void FETCH(argb4444)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLushort *src = USHORT_SRC( texImage, i, j, k );
   const GLushort s = *src;
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( ((s >>  8) & 0xf) * 255 / 0xf );
   rgba[GCOMP] = UBYTE_TO_CHAN( ((s >>  4) & 0xf) * 255 / 0xf );
   rgba[BCOMP] = UBYTE_TO_CHAN( ((s      ) & 0xf) * 255 / 0xf );
   rgba[ACOMP] = UBYTE_TO_CHAN( ((s >> 12) & 0xf) * 255 / 0xf );
}

static void FETCH(argb1555)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLushort *src = USHORT_SRC( texImage, i, j, k );
   const GLushort s = *src;
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( ((s >> 10) & 0x1f) * 255 / 0x1f );
   rgba[GCOMP] = UBYTE_TO_CHAN( ((s >>  5) & 0x1f) * 255 / 0x1f );
   rgba[BCOMP] = UBYTE_TO_CHAN( ((s      ) & 0x1f) * 255 / 0x1f );
   rgba[ACOMP] = UBYTE_TO_CHAN( ((s >> 15) & 0x01) * 255 );
}

static void FETCH(al88)( const struct gl_texture_image *texImage,
			 GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 2 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[GCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[BCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[ACOMP] = UBYTE_TO_CHAN( src[1] );
}

static void FETCH(rgb332)( const struct gl_texture_image *texImage,
			   GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 1 );
   const GLubyte s = *src;
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( ((s     ) & 0xe0) * 255 / 0xe0 );
   rgba[GCOMP] = UBYTE_TO_CHAN( ((s << 3) & 0xe0) * 255 / 0xe0 );
   rgba[BCOMP] = UBYTE_TO_CHAN( ((s << 6) & 0xc0) * 255 / 0xc0 );
   rgba[ACOMP] = CHAN_MAX;
}

static void FETCH(a8)( const struct gl_texture_image *texImage,
		       GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 1 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = 0;
   rgba[GCOMP] = 0;
   rgba[BCOMP] = 0;
   rgba[ACOMP] = UBYTE_TO_CHAN( src[0] );
}

static void FETCH(l8)( const struct gl_texture_image *texImage,
		       GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 1 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[GCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[BCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[ACOMP] = CHAN_MAX;
}

static void FETCH(i8)( const struct gl_texture_image *texImage,
		       GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 1 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[GCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[BCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[ACOMP] = UBYTE_TO_CHAN( src[0] );
}

static void FETCH(ci8)( const struct gl_texture_image *texImage,
			GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 1 );
   GLchan *index = (GLchan *) texel;
   *index = UBYTE_TO_CHAN( *src );
}

/* XXX this may break if GLchan != GLubyte */
static void FETCH(ycbcr)( const struct gl_texture_image *texImage,
                          GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLushort *src0 = USHORT_SRC( texImage, (i & ~1), j, k ); /* even */
   const GLushort *src1 = src0 + 1;                               /* odd */
   const GLubyte y0 = (*src0 >> 8) & 0xff;  /* luminance */
   const GLubyte cb = *src0 & 0xff;         /* chroma U */
   const GLubyte y1 = (*src1 >> 8) & 0xff;  /* luminance */
   const GLubyte cr = *src1 & 0xff;         /* chroma V */
   GLchan *rgba = (GLchan *) texel;
   GLint r, g, b;
   if (i & 1) {
      /* odd pixel: use y1,cr,cb */
      r = (GLint) (1.164 * (y1-16) + 1.596 * (cr-128));
      g = (GLint) (1.164 * (y1-16) - 0.813 * (cr-128) - 0.391 * (cb-128));
      b = (GLint) (1.164 * (y1-16) + 2.018 * (cb-128));
   }
   else {
      /* even pixel: use y0,cr,cb */
      r = (GLint) (1.164 * (y0-16) + 1.596 * (cr-128));
      g = (GLint) (1.164 * (y0-16) - 0.813 * (cr-128) - 0.391 * (cb-128));
      b = (GLint) (1.164 * (y0-16) + 2.018 * (cb-128));
   }
   rgba[RCOMP] = CLAMP(r, 0, CHAN_MAX);
   rgba[GCOMP] = CLAMP(g, 0, CHAN_MAX);
   rgba[BCOMP] = CLAMP(b, 0, CHAN_MAX);
   rgba[ACOMP] = CHAN_MAX;
}

/* XXX this may break if GLchan != GLubyte */
static void FETCH(ycbcr_rev)( const struct gl_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLushort *src0 = USHORT_SRC( texImage, (i & ~1), j, k ); /* even */
   const GLushort *src1 = src0 + 1;                               /* odd */
   const GLubyte y0 = *src0 & 0xff;         /* luminance */
   const GLubyte cr = (*src0 >> 8) & 0xff;  /* chroma V */
   const GLubyte y1 = *src1 & 0xff;         /* luminance */
   const GLubyte cb = (*src1 >> 8) & 0xff;  /* chroma U */
   GLchan *rgba = (GLchan *) texel;
   GLint r, g, b;
   if (i & 1) {
      /* odd pixel: use y1,cr,cb */
      r = (GLint) (1.164 * (y1-16) + 1.596 * (cr-128));
      g = (GLint) (1.164 * (y1-16) - 0.813 * (cr-128) - 0.391 * (cb-128));
      b = (GLint) (1.164 * (y1-16) + 2.018 * (cb-128));
   }
   else {
      /* even pixel: use y0,cr,cb */
      r = (GLint) (1.164 * (y0-16) + 1.596 * (cr-128));
      g = (GLint) (1.164 * (y0-16) - 0.813 * (cr-128) - 0.391 * (cb-128));
      b = (GLint) (1.164 * (y0-16) + 2.018 * (cb-128));
   }
   rgba[RCOMP] = CLAMP(r, 0, CHAN_MAX);
   rgba[GCOMP] = CLAMP(g, 0, CHAN_MAX);
   rgba[BCOMP] = CLAMP(b, 0, CHAN_MAX);
   rgba[ACOMP] = CHAN_MAX;
}


#if DIM == 2
static void FETCH(rgb_fxt1)( const struct gl_texture_image *texImage,
                             GLint i, GLint j, GLint k, GLvoid *texel )
{
   /* Extract the (i,j) pixel from texImage->Data and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */
}
#endif

#if DIM == 2
static void FETCH(rgba_fxt1)( const struct gl_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLvoid *texel )
{
   /* Extract the (i,j) pixel from texImage->Data and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */
}
#endif


#if DIM == 2
static void FETCH(rgb_dxt1)( const struct gl_texture_image *texImage,
                             GLint i, GLint j, GLint k, GLvoid *texel )
{
   /* Extract the (i,j) pixel from texImage->Data and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */
}
#endif

#if DIM == 2
static void FETCH(rgba_dxt1)( const struct gl_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLvoid *texel )
{
   /* Extract the (i,j) pixel from texImage->Data and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */
}
#endif

#if DIM == 2
static void FETCH(rgba_dxt3)( const struct gl_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLvoid *texel )
{
   /* Extract the (i,j) pixel from texImage->Data and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */
}
#endif

#if DIM == 2
static void FETCH(rgba_dxt5)( const struct gl_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLvoid *texel )
{
   /* Extract the (i,j) pixel from texImage->Data and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */
}
#endif



/* big-endian */

#if 0
static void FETCH(abgr8888)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 4 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( src[3] );
   rgba[GCOMP] = UBYTE_TO_CHAN( src[2] );
   rgba[BCOMP] = UBYTE_TO_CHAN( src[1] );
   rgba[ACOMP] = UBYTE_TO_CHAN( src[0] );
}

static void FETCH(bgra8888)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 4 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( src[2] );
   rgba[GCOMP] = UBYTE_TO_CHAN( src[1] );
   rgba[BCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[ACOMP] = UBYTE_TO_CHAN( src[3] );
}

static void FETCH(bgr888)( const struct gl_texture_image *texImage,
			   GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 3 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( src[2] );
   rgba[GCOMP] = UBYTE_TO_CHAN( src[1] );
   rgba[BCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[ACOMP] = CHAN_MAX;
}

static void FETCH(bgr565)( const struct gl_texture_image *texImage,
			   GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLushort *src = USHORT_SRC( texImage, i, j, k );
   const GLushort s = *src;
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( ((s >> 8) & 0xf8) * 255 / 0xf8 );
   rgba[GCOMP] = UBYTE_TO_CHAN( ((s >> 3) & 0xfc) * 255 / 0xfc );
   rgba[BCOMP] = UBYTE_TO_CHAN( ((s << 3) & 0xf8) * 255 / 0xf8 );
   rgba[ACOMP] = CHAN_MAX;
}

static void FETCH(bgra4444)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLushort *src = USHORT_SRC( texImage, i, j, k );
   const GLushort s = *src;
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( ((s >>  8) & 0xf) * 255 / 0xf );
   rgba[GCOMP] = UBYTE_TO_CHAN( ((s >>  4) & 0xf) * 255 / 0xf );
   rgba[BCOMP] = UBYTE_TO_CHAN( ((s      ) & 0xf) * 255 / 0xf );
   rgba[ACOMP] = UBYTE_TO_CHAN( ((s >> 12) & 0xf) * 255 / 0xf );
}

static void FETCH(bgra5551)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLushort *src = USHORT_SRC( texImage, i, j, k );
   const GLushort s = *src;
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( ((s >> 10) & 0x1f) * 255 / 0x1f );
   rgba[GCOMP] = UBYTE_TO_CHAN( ((s >>  5) & 0x1f) * 255 / 0x1f );
   rgba[BCOMP] = UBYTE_TO_CHAN( ((s      ) & 0x1f) * 255 / 0x1f );
   rgba[ACOMP] = UBYTE_TO_CHAN( ((s >> 15) & 0x01) * 255 );
}

static void FETCH(la88)( const struct gl_texture_image *texImage,
			 GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 2 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[GCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[BCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[ACOMP] = UBYTE_TO_CHAN( src[1] );
}

static void FETCH(bgr233)( const struct gl_texture_image *texImage,
			   GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 1 );
   const GLubyte s = *src;
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( ((s     ) & 0xe0) * 255 / 0xe0 );
   rgba[GCOMP] = UBYTE_TO_CHAN( ((s << 3) & 0xe0) * 255 / 0xe0 );
   rgba[BCOMP] = UBYTE_TO_CHAN( ((s << 5) & 0xc0) * 255 / 0xc0 );
   rgba[ACOMP] = CHAN_MAX;
}
#endif


#undef CHAN_SRC
#undef UBYTE_SRC
#undef USHORT_SRC
#undef FLOAT_SRC
#undef FETCH
#undef DIM
