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
 * \file texformat_tmp.h
 * Texel fetch functions template.
 * 
 * This template file is used by texformat.c to generate texel fetch functions
 * for 1-D, 2-D and 3-D texture images. 
 *
 * It should be expanded by defining \p DIM as the number texture dimensions
 * (1, 2 or 3).  According to the value of \p DIM a series of macros is defined
 * for the texel lookup in the gl_texture_image::Data.
 * 
 * \sa texformat.c and FetchTexel.
 * 
 * \author Gareth Hughes
 * \author Brian Paul
 */


#if DIM == 1

#define TEXEL_ADDR( type, image, i, j, k, size ) \
	((void) (j), (void) (k), ((type *)(image)->Data + (i) * (size)))

#define FETCH(x) fetch_texel_1d_##x

#elif DIM == 2

#define TEXEL_ADDR( type, image, i, j, k, size )			\
	((void) (k),							\
	 ((type *)(image)->Data + ((image)->RowStride * (j) + (i)) * (size)))

#define FETCH(x) fetch_texel_2d_##x

#elif DIM == 3

#define TEXEL_ADDR( type, image, i, j, k, size )			\
	((type *)(image)->Data + ((image)->ImageOffsets[k]		\
             + (image)->RowStride * (j) + (i)) * (size))

#define FETCH(x) fetch_texel_3d_##x

#else
#error	illegal number of texture dimensions
#endif


/* MESA_FORMAT_RGBA **********************************************************/

/* Fetch texel from 1D, 2D or 3D RGBA texture, returning 4 GLchans */
static void FETCH(rgba)( const struct gl_texture_image *texImage,
			 GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLchan *src = TEXEL_ADDR(GLchan, texImage, i, j, k, 4);
   COPY_CHAN4( texel, src );
}

/* Fetch texel from 1D, 2D or 3D RGBA texture, returning 4 GLfloats */
static void FETCH(f_rgba)( const struct gl_texture_image *texImage,
                           GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLchan *src = TEXEL_ADDR(GLchan, texImage, i, j, k, 4);
   texel[RCOMP] = CHAN_TO_FLOAT(src[0]);
   texel[GCOMP] = CHAN_TO_FLOAT(src[1]);
   texel[BCOMP] = CHAN_TO_FLOAT(src[2]);
   texel[ACOMP] = CHAN_TO_FLOAT(src[3]);
}

#if DIM == 3
/* Store a GLchan RGBA texel */
static void store_texel_rgba(struct gl_texture_image *texImage,
                             GLint i, GLint j, GLint k, const void *texel)
{
   const GLchan *rgba = (const GLchan *) texel;
   GLchan *dst = TEXEL_ADDR(GLchan, texImage, i, j, k, 4);
   dst[0] = rgba[RCOMP];
   dst[1] = rgba[GCOMP];
   dst[2] = rgba[BCOMP];
   dst[3] = rgba[ACOMP];
}
#endif

/* MESA_FORMAT_RGB ***********************************************************/

/* Fetch texel from 1D, 2D or 3D RGB texture, returning 4 GLchans */
static void FETCH(rgb)( const struct gl_texture_image *texImage,
			GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLchan *src = TEXEL_ADDR(GLchan, texImage, i, j, k, 3);
   texel[RCOMP] = src[0];
   texel[GCOMP] = src[1];
   texel[BCOMP] = src[2];
   texel[ACOMP] = CHAN_MAX;
}

/* Fetch texel from 1D, 2D or 3D RGB texture, returning 4 GLfloats */
static void FETCH(f_rgb)( const struct gl_texture_image *texImage,
                          GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLchan *src = TEXEL_ADDR(GLchan, texImage, i, j, k, 3);
   texel[RCOMP] = CHAN_TO_FLOAT(src[0]);
   texel[GCOMP] = CHAN_TO_FLOAT(src[1]);
   texel[BCOMP] = CHAN_TO_FLOAT(src[2]);
   texel[ACOMP] = 1.0F;
}

#if DIM == 3
static void store_texel_rgb(struct gl_texture_image *texImage,
                            GLint i, GLint j, GLint k, const void *texel)
{
   const GLchan *rgba = (const GLchan *) texel;
   GLchan *dst = TEXEL_ADDR(GLchan, texImage, i, j, k, 3);
   dst[0] = rgba[RCOMP];
   dst[1] = rgba[GCOMP];
   dst[2] = rgba[BCOMP];
}
#endif

/* MESA_FORMAT_ALPHA *********************************************************/

/* Fetch texel from 1D, 2D or 3D ALPHA texture, returning 4 GLchans */
static void FETCH(alpha)( const struct gl_texture_image *texImage,
			  GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLchan *src = TEXEL_ADDR(GLchan, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = 0;
   texel[ACOMP] = src[0];
}

#if DIM == 3
static void store_texel_alpha(struct gl_texture_image *texImage,
                              GLint i, GLint j, GLint k, const void *texel)
{
   const GLchan *rgba = (const GLchan *) texel;
   GLchan *dst = TEXEL_ADDR(GLchan, texImage, i, j, k, 1);
   dst[0] = rgba[ACOMP];
}
#endif

/* MESA_FORMAT_LUMINANCE *****************************************************/

/* Fetch texel from 1D, 2D or 3D LUMIN texture, returning 4 GLchans */
static void FETCH(luminance)( const struct gl_texture_image *texImage,
			      GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLchan *src = TEXEL_ADDR(GLchan, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = src[0];
   texel[ACOMP] = CHAN_MAX;
}

#if DIM == 3
static void store_texel_luminance(struct gl_texture_image *texImage,
                                  GLint i, GLint j, GLint k, const void *texel)
{
   const GLchan *rgba = (const GLchan *) texel;
   GLchan *dst = TEXEL_ADDR(GLchan, texImage, i, j, k, 1);
   dst[0] = rgba[RCOMP];
}
#endif

/* MESA_FORMAT_LUMINANCE_ALPHA ***********************************************/

/* Fetch texel from 1D, 2D or 3D L_A texture, returning 4 GLchans */
static void FETCH(luminance_alpha)( const struct gl_texture_image *texImage,
				    GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLchan *src = TEXEL_ADDR(GLchan, texImage, i, j, k, 2);
   texel[RCOMP] = src[0];
   texel[GCOMP] = src[0];
   texel[BCOMP] = src[0];
   texel[ACOMP] = src[1];
}

#if DIM == 3
static void store_texel_luminance_alpha(struct gl_texture_image *texImage,
                                  GLint i, GLint j, GLint k, const void *texel)
{
   const GLchan *rgba = (const GLchan *) texel;
   GLchan *dst = TEXEL_ADDR(GLchan, texImage, i, j, k, 2);
   dst[0] = rgba[RCOMP];
   dst[1] = rgba[ACOMP];
}
#endif

/* MESA_FORMAT_INTENSITY *****************************************************/

/* Fetch texel from 1D, 2D or 3D INT. texture, returning 4 GLchans */
static void FETCH(intensity)( const struct gl_texture_image *texImage,
			      GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLchan *src = TEXEL_ADDR(GLchan, texImage, i, j, k, 1);
   texel[RCOMP] = src[0];
   texel[GCOMP] = src[0];
   texel[BCOMP] = src[0];
   texel[ACOMP] = src[0];
}

#if DIM == 3
static void store_texel_intensity(struct gl_texture_image *texImage,
                                  GLint i, GLint j, GLint k, const void *texel)
{
   const GLchan *rgba = (const GLchan *) texel;
   GLchan *dst = TEXEL_ADDR(GLchan, texImage, i, j, k, 1);
   dst[0] = rgba[RCOMP];
}
#endif


/* MESA_FORMAT_Z32 ***********************************************************/

/* Fetch depth texel from 1D, 2D or 3D 32-bit depth texture,
 * returning 1 GLfloat.
 * Note: no GLchan version of this function.
 */
static void FETCH(f_z32)( const struct gl_texture_image *texImage,
                          GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLuint *src = TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[0] = src[0] * (1.0F / 0xffffffff);
}

#if DIM == 3
static void store_texel_z32(struct gl_texture_image *texImage,
                            GLint i, GLint j, GLint k, const void *texel)
{
   const GLuint *depth = (const GLuint *) texel;
   GLuint *dst = TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   dst[0] = *depth;
}
#endif


/* MESA_FORMAT_Z16 ***********************************************************/

/* Fetch depth texel from 1D, 2D or 3D 16-bit depth texture,
 * returning 1 GLfloat.
 * Note: no GLchan version of this function.
 */
static void FETCH(f_z16)(const struct gl_texture_image *texImage,
                         GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[0] = src[0] * (1.0F / 65535.0F);
}

#if DIM == 3
static void store_texel_z16(struct gl_texture_image *texImage,
                            GLint i, GLint j, GLint k, const void *texel)
{
   const GLushort *depth = (const GLushort *) texel;
   GLushort *dst = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   dst[0] = *depth;
}
#endif


/* MESA_FORMAT_RGBA_F32 ******************************************************/

/* Fetch texel from 1D, 2D or 3D RGBA_FLOAT32 texture, returning 4 GLfloats.
 */
static void FETCH(f_rgba_f32)( const struct gl_texture_image *texImage,
                               GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 4);
   texel[RCOMP] = src[0];
   texel[GCOMP] = src[1];
   texel[BCOMP] = src[2];
   texel[ACOMP] = src[3];
}

#if DIM == 3
static void store_texel_rgba_f32(struct gl_texture_image *texImage,
                                 GLint i, GLint j, GLint k, const void *texel)
{
   const GLfloat *depth = (const GLfloat *) texel;
   GLfloat *dst = TEXEL_ADDR(GLfloat, texImage, i, j, k, 1);
   dst[0] = depth[RCOMP];
   dst[1] = depth[GCOMP];
   dst[2] = depth[BCOMP];
   dst[3] = depth[ACOMP];
}
#endif


/* MESA_FORMAT_RGBA_F16 ******************************************************/

/* Fetch texel from 1D, 2D or 3D RGBA_FLOAT16 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_rgba_f16)( const struct gl_texture_image *texImage,
                               GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLhalfARB *src = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 4);
   texel[RCOMP] = _mesa_half_to_float(src[0]);
   texel[GCOMP] = _mesa_half_to_float(src[1]);
   texel[BCOMP] = _mesa_half_to_float(src[2]);
   texel[ACOMP] = _mesa_half_to_float(src[3]);
}

#if DIM == 3
static void store_texel_rgba_f16(struct gl_texture_image *texImage,
                                 GLint i, GLint j, GLint k, const void *texel)
{
   const GLfloat *depth = (const GLfloat *) texel;
   GLhalfARB *dst = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 1);
   dst[0] = _mesa_float_to_half(*depth);
}
#endif

/* MESA_FORMAT_RGB_F32 *******************************************************/

/* Fetch texel from 1D, 2D or 3D RGB_FLOAT32 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_rgb_f32)( const struct gl_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 3);
   texel[RCOMP] = src[0];
   texel[GCOMP] = src[1];
   texel[BCOMP] = src[2];
   texel[ACOMP] = 1.0F;
}

#if DIM == 3
static void store_texel_rgb_f32(struct gl_texture_image *texImage,
                                 GLint i, GLint j, GLint k, const void *texel)
{
   const GLfloat *depth = (const GLfloat *) texel;
   GLfloat *dst = TEXEL_ADDR(GLfloat, texImage, i, j, k, 1);
   dst[0] = *depth;
}
#endif


/* MESA_FORMAT_RGB_F16 *******************************************************/

/* Fetch texel from 1D, 2D or 3D RGB_FLOAT16 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_rgb_f16)( const struct gl_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLhalfARB *src = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 3);
   texel[RCOMP] = _mesa_half_to_float(src[0]);
   texel[GCOMP] = _mesa_half_to_float(src[1]);
   texel[BCOMP] = _mesa_half_to_float(src[2]);
   texel[ACOMP] = 1.0F;
}

#if DIM == 3
static void store_texel_rgb_f16(struct gl_texture_image *texImage,
                                GLint i, GLint j, GLint k, const void *texel)
{
   const GLfloat *depth = (const GLfloat *) texel;
   GLhalfARB *dst = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 1);
   dst[0] = _mesa_float_to_half(*depth);
}
#endif


/* MESA_FORMAT_ALPHA_F32 *****************************************************/

/* Fetch texel from 1D, 2D or 3D ALPHA_FLOAT32 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_alpha_f32)( const struct gl_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = src[0];
}

#if DIM == 3
static void store_texel_alpha_f32(struct gl_texture_image *texImage,
                                  GLint i, GLint j, GLint k, const void *texel)
{
   const GLfloat *rgba = (const GLfloat *) texel;
   GLfloat *dst = TEXEL_ADDR(GLfloat, texImage, i, j, k, 1);
   dst[0] = rgba[ACOMP];
}
#endif


/* MESA_FORMAT_ALPHA_F32 *****************************************************/

/* Fetch texel from 1D, 2D or 3D ALPHA_FLOAT16 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_alpha_f16)( const struct gl_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLhalfARB *src = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = _mesa_half_to_float(src[0]);
}

#if DIM == 3
static void store_texel_alpha_f16(struct gl_texture_image *texImage,
                                  GLint i, GLint j, GLint k, const void *texel)
{
   const GLfloat *rgba = (const GLfloat *) texel;
   GLhalfARB *dst = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 1);
   dst[0] = _mesa_float_to_half(rgba[ACOMP]);
}
#endif


/* MESA_FORMAT_LUMINANCE_F32 *************************************************/

/* Fetch texel from 1D, 2D or 3D LUMINANCE_FLOAT32 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_luminance_f32)( const struct gl_texture_image *texImage,
                                    GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = src[0];
   texel[ACOMP] = 1.0F;
}

#if DIM == 3
static void store_texel_luminance_f32(struct gl_texture_image *texImage,
                                  GLint i, GLint j, GLint k, const void *texel)
{
   const GLfloat *rgba = (const GLfloat *) texel;
   GLfloat *dst = TEXEL_ADDR(GLfloat, texImage, i, j, k, 1);
   dst[0] = rgba[RCOMP];
}
#endif


/* MESA_FORMAT_LUMINANCE_F16 *************************************************/

/* Fetch texel from 1D, 2D or 3D LUMINANCE_FLOAT16 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_luminance_f16)( const struct gl_texture_image *texImage,
                                    GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLhalfARB *src = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = _mesa_half_to_float(src[0]);
   texel[ACOMP] = 1.0F;
}

#if DIM == 3
static void store_texel_luminance_f16(struct gl_texture_image *texImage,
                                  GLint i, GLint j, GLint k, const void *texel)
{
   const GLfloat *rgba = (const GLfloat *) texel;
   GLhalfARB *dst = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 1);
   dst[0] = _mesa_float_to_half(rgba[RCOMP]);
}
#endif


/* MESA_FORMAT_LUMINANCE_ALPHA_F32 *******************************************/

/* Fetch texel from 1D, 2D or 3D LUMINANCE_ALPHA_FLOAT32 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_luminance_alpha_f32)( const struct gl_texture_image *texImage,
                                    GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 2);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = src[0];
   texel[ACOMP] = src[1];
}

#if DIM == 3
static void store_texel_luminance_alpha_f32(struct gl_texture_image *texImage,
                                  GLint i, GLint j, GLint k, const void *texel)
{
   const GLfloat *rgba = (const GLfloat *) texel;
   GLfloat *dst = TEXEL_ADDR(GLfloat, texImage, i, j, k, 2);
   dst[0] = rgba[RCOMP];
   dst[1] = rgba[ACOMP];
}
#endif


/* MESA_FORMAT_LUMINANCE_ALPHA_F16 *******************************************/

/* Fetch texel from 1D, 2D or 3D LUMINANCE_ALPHA_FLOAT16 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_luminance_alpha_f16)( const struct gl_texture_image *texImage,
                                    GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLhalfARB *src = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 2);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = _mesa_half_to_float(src[0]);
   texel[ACOMP] = _mesa_half_to_float(src[1]);
}

#if DIM == 3
static void store_texel_luminance_alpha_f16(struct gl_texture_image *texImage,
                                  GLint i, GLint j, GLint k, const void *texel)
{
   const GLfloat *rgba = (const GLfloat *) texel;
   GLhalfARB *dst = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 2);
   dst[0] = _mesa_float_to_half(rgba[RCOMP]);
   dst[1] = _mesa_float_to_half(rgba[ACOMP]);
}
#endif


/* MESA_FORMAT_INTENSITY_F32 *************************************************/

/* Fetch texel from 1D, 2D or 3D INTENSITY_FLOAT32 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_intensity_f32)( const struct gl_texture_image *texImage,
                                    GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] =
   texel[ACOMP] = src[0];
}

#if DIM == 3
static void store_texel_intensity_f32(struct gl_texture_image *texImage,
                                  GLint i, GLint j, GLint k, const void *texel)
{
   const GLfloat *rgba = (const GLfloat *) texel;
   GLfloat *dst = TEXEL_ADDR(GLfloat, texImage, i, j, k, 1);
   dst[0] = rgba[RCOMP];
}
#endif


/* MESA_FORMAT_INTENSITY_F16 *************************************************/

/* Fetch texel from 1D, 2D or 3D INTENSITY_FLOAT16 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_intensity_f16)( const struct gl_texture_image *texImage,
                                    GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLhalfARB *src = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] =
   texel[ACOMP] = _mesa_half_to_float(src[0]);
}

#if DIM == 3
static void store_texel_intensity_f16(struct gl_texture_image *texImage,
                                  GLint i, GLint j, GLint k, const void *texel)
{
   const GLfloat *rgba = (const GLfloat *) texel;
   GLhalfARB *dst = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 1);
   dst[0] = _mesa_float_to_half(rgba[RCOMP]);
}
#endif




/*
 * Begin Hardware formats
 */

/* MESA_FORMAT_RGBA8888 ******************************************************/

/* Fetch texel from 1D, 2D or 3D rgba8888 texture, return 4 GLchans */
static void FETCH(rgba8888)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_CHAN( (s >> 24)        );
   texel[GCOMP] = UBYTE_TO_CHAN( (s >> 16) & 0xff );
   texel[BCOMP] = UBYTE_TO_CHAN( (s >>  8) & 0xff );
   texel[ACOMP] = UBYTE_TO_CHAN( (s      ) & 0xff );
}

#if DIM == 3
static void store_texel_rgba8888(struct gl_texture_image *texImage,
                                 GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLuint *dst = TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   *dst = PACK_COLOR_8888(rgba[RCOMP], rgba[GCOMP], rgba[BCOMP], rgba[ACOMP]);
}
#endif


/* MESA_FORMAT_RGBA888_REV ***************************************************/

/* Fetch texel from 1D, 2D or 3D abgr8888 texture, return 4 GLchans */
static void FETCH(rgba8888_rev)( const struct gl_texture_image *texImage,
                                 GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_CHAN( (s      ) & 0xff );
   texel[GCOMP] = UBYTE_TO_CHAN( (s >>  8) & 0xff );
   texel[BCOMP] = UBYTE_TO_CHAN( (s >> 16) & 0xff );
   texel[ACOMP] = UBYTE_TO_CHAN( (s >> 24)        );
}

#if DIM == 3
static void store_texel_rgba8888_rev(struct gl_texture_image *texImage,
                                  GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLuint *dst = TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   *dst = PACK_COLOR_8888_REV(rgba[RCOMP], rgba[GCOMP], rgba[BCOMP], rgba[ACOMP]);
}
#endif


/* MESA_FORMAT_ARGB8888 ******************************************************/

/* Fetch texel from 1D, 2D or 3D argb8888 texture, return 4 GLchans */
static void FETCH(argb8888)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_CHAN( (s >> 16) & 0xff );
   texel[GCOMP] = UBYTE_TO_CHAN( (s >>  8) & 0xff );
   texel[BCOMP] = UBYTE_TO_CHAN( (s      ) & 0xff );
   texel[ACOMP] = UBYTE_TO_CHAN( (s >> 24)        );
}

#if DIM == 3
static void store_texel_argb8888(struct gl_texture_image *texImage,
                                 GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLuint *dst = TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   *dst = PACK_COLOR_8888(rgba[ACOMP], rgba[RCOMP], rgba[GCOMP], rgba[BCOMP]);
}
#endif


/* MESA_FORMAT_ARGB8888_REV **************************************************/

/* Fetch texel from 1D, 2D or 3D argb8888_rev texture, return 4 GLchans */
static void FETCH(argb8888_rev)( const struct gl_texture_image *texImage,
                                 GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_CHAN( (s >>  8) & 0xff );
   texel[GCOMP] = UBYTE_TO_CHAN( (s >> 16) & 0xff );
   texel[BCOMP] = UBYTE_TO_CHAN( (s >> 24)        );
   texel[ACOMP] = UBYTE_TO_CHAN( (s      ) & 0xff );
}

#if DIM == 3
static void store_texel_argb8888_rev(struct gl_texture_image *texImage,
                                  GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLuint *dst = TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   *dst = PACK_COLOR_8888(rgba[BCOMP], rgba[GCOMP], rgba[RCOMP], rgba[ACOMP]);
}
#endif


/* MESA_FORMAT_RGB888 ********************************************************/

/* Fetch texel from 1D, 2D or 3D rgb888 texture, return 4 GLchans */
static void FETCH(rgb888)( const struct gl_texture_image *texImage,
			   GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 3);
   texel[RCOMP] = UBYTE_TO_CHAN( src[2] );
   texel[GCOMP] = UBYTE_TO_CHAN( src[1] );
   texel[BCOMP] = UBYTE_TO_CHAN( src[0] );
   texel[ACOMP] = CHAN_MAX;
}

#if DIM == 3
static void store_texel_rgb888(struct gl_texture_image *texImage,
                               GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLubyte *dst = TEXEL_ADDR(GLubyte, texImage, i, j, k, 3);
   dst[0] = rgba[BCOMP];
   dst[1] = rgba[GCOMP];
   dst[2] = rgba[RCOMP];
}
#endif


/* MESA_FORMAT_BGR888 ********************************************************/

/* Fetch texel from 1D, 2D or 3D bgr888 texture, return 4 GLchans */
static void FETCH(bgr888)( const struct gl_texture_image *texImage,
			   GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 3);
   texel[RCOMP] = UBYTE_TO_CHAN( src[0] );
   texel[GCOMP] = UBYTE_TO_CHAN( src[1] );
   texel[BCOMP] = UBYTE_TO_CHAN( src[2] );
   texel[ACOMP] = CHAN_MAX;
}

#if DIM == 3
static void store_texel_bgr888(struct gl_texture_image *texImage,
                               GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLubyte *dst = TEXEL_ADDR(GLubyte, texImage, i, j, k, 3);
   dst[0] = rgba[RCOMP];
   dst[1] = rgba[GCOMP];
   dst[2] = rgba[BCOMP];
}
#endif


/* use color expansion like (g << 2) | (g >> 4) (does somewhat random rounding)
   instead of slow (g << 2) * 255 / 252 (always rounds down) */

/* MESA_FORMAT_RGB565 ********************************************************/

/* Fetch texel from 1D, 2D or 3D rgb565 texture, return 4 GLchans */
static void FETCH(rgb565)( const struct gl_texture_image *texImage,
			   GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   const GLushort s = *src;
   texel[RCOMP] = UBYTE_TO_CHAN( ((s >> 8) & 0xf8) | ((s >> 13) & 0x7) );
   texel[GCOMP] = UBYTE_TO_CHAN( ((s >> 3) & 0xfc) | ((s >>  9) & 0x3) );
   texel[BCOMP] = UBYTE_TO_CHAN( ((s << 3) & 0xf8) | ((s >>  2) & 0x7) );
   texel[ACOMP] = CHAN_MAX;
}

#if DIM == 3
static void store_texel_rgb565(struct gl_texture_image *texImage,
                               GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLushort *dst = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   *dst = PACK_COLOR_565(rgba[RCOMP], rgba[GCOMP], rgba[BCOMP]);
}
#endif


/* MESA_FORMAT_RGB565_REV ****************************************************/

/* Fetch texel from 1D, 2D or 3D rgb565_rev texture, return 4 GLchans */
static void FETCH(rgb565_rev)( const struct gl_texture_image *texImage,
                               GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   const GLushort s = (*src >> 8) | (*src << 8); /* byte swap */
   texel[RCOMP] = UBYTE_TO_CHAN( ((s >> 8) & 0xf8) | ((s >> 13) & 0x7) );
   texel[GCOMP] = UBYTE_TO_CHAN( ((s >> 3) & 0xfc) | ((s >>  9) & 0x3) );
   texel[BCOMP] = UBYTE_TO_CHAN( ((s << 3) & 0xf8) | ((s >>  2) & 0x7) );
   texel[ACOMP] = CHAN_MAX;
}

#if DIM == 3
static void store_texel_rgb565_rev(struct gl_texture_image *texImage,
                                  GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLushort *dst = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   *dst = PACK_COLOR_565(rgba[BCOMP], rgba[GCOMP], rgba[RCOMP]);
}
#endif

/* MESA_FORMAT_RGBA4444 ******************************************************/

/* Fetch texel from 1D, 2D or 3D argb444 texture, return 4 GLchans */
static void FETCH(rgba4444)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   const GLushort s = *src;
   texel[RCOMP] = UBYTE_TO_CHAN( ((s >> 12) & 0xf) | ((s >> 8) & 0xf0) );
   texel[GCOMP] = UBYTE_TO_CHAN( ((s >>  8) & 0xf) | ((s >> 4) & 0xf0) );
   texel[BCOMP] = UBYTE_TO_CHAN( ((s >>  4) & 0xf) | ((s     ) & 0xf0) );
   texel[ACOMP] = UBYTE_TO_CHAN( ((s      ) & 0xf) | ((s << 4) & 0xf0) );
}

#if DIM == 3
static void store_texel_rgba4444(struct gl_texture_image *texImage,
                                 GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLushort *dst = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   *dst = PACK_COLOR_4444(rgba[RCOMP], rgba[GCOMP], rgba[BCOMP], rgba[ACOMP]);
}
#endif


/* MESA_FORMAT_ARGB4444 ******************************************************/

/* Fetch texel from 1D, 2D or 3D argb444 texture, return 4 GLchans */
static void FETCH(argb4444)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   const GLushort s = *src;
   texel[RCOMP] = UBYTE_TO_CHAN( ((s >>  8) & 0xf) | ((s >> 4) & 0xf0) );
   texel[GCOMP] = UBYTE_TO_CHAN( ((s >>  4) & 0xf) | ((s     ) & 0xf0) );
   texel[BCOMP] = UBYTE_TO_CHAN( ((s      ) & 0xf) | ((s << 4) & 0xf0) );
   texel[ACOMP] = UBYTE_TO_CHAN( ((s >> 12) & 0xf) | ((s >> 8) & 0xf0) );
}

#if DIM == 3
static void store_texel_argb4444(struct gl_texture_image *texImage,
                                 GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLushort *dst = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   *dst = PACK_COLOR_4444(rgba[ACOMP], rgba[RCOMP], rgba[GCOMP], rgba[BCOMP]);
}
#endif


/* MESA_FORMAT_ARGB4444_REV **************************************************/

/* Fetch texel from 1D, 2D or 3D argb4444_rev texture, return 4 GLchans */
static void FETCH(argb4444_rev)( const struct gl_texture_image *texImage,
                                 GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLushort s = *TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_CHAN( ((s      ) & 0xf) | ((s << 4) & 0xf0) );
   texel[GCOMP] = UBYTE_TO_CHAN( ((s >> 12) & 0xf) | ((s >> 8) & 0xf0) );
   texel[BCOMP] = UBYTE_TO_CHAN( ((s >>  8) & 0xf) | ((s >> 4) & 0xf0) );
   texel[ACOMP] = UBYTE_TO_CHAN( ((s >>  4) & 0xf) | ((s     ) & 0xf0) );
}

#if DIM == 3
static void store_texel_argb4444_rev(struct gl_texture_image *texImage,
                                 GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLushort *dst = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   *dst = PACK_COLOR_4444(rgba[ACOMP], rgba[BCOMP], rgba[GCOMP], rgba[RCOMP]);
}
#endif

/* MESA_FORMAT_RGBA5551 ******************************************************/

/* Fetch texel from 1D, 2D or 3D argb1555 texture, return 4 GLchans */
static void FETCH(rgba5551)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   const GLushort s = *src;
   texel[RCOMP] = UBYTE_TO_CHAN( ((s >>  8) & 0xf8) | ((s >> 13) & 0x7) );
   texel[GCOMP] = UBYTE_TO_CHAN( ((s >>  3) & 0xf8) | ((s >>  8) & 0x7) );
   texel[BCOMP] = UBYTE_TO_CHAN( ((s <<  2) & 0xf8) | ((s >>  3) & 0x7) );
   texel[ACOMP] = UBYTE_TO_CHAN( ((s) & 0x01) ? 255 : 0);
}

#if DIM == 3
static void store_texel_rgba5551(struct gl_texture_image *texImage,
                                 GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLushort *dst = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   *dst = PACK_COLOR_5551(rgba[RCOMP], rgba[GCOMP], rgba[BCOMP], rgba[ACOMP]);
}
#endif

/* MESA_FORMAT_ARGB1555 ******************************************************/

/* Fetch texel from 1D, 2D or 3D argb1555 texture, return 4 GLchans */
static void FETCH(argb1555)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   const GLushort s = *src;
   texel[RCOMP] = UBYTE_TO_CHAN( ((s >>  7) & 0xf8) | ((s >> 12) & 0x7) );
   texel[GCOMP] = UBYTE_TO_CHAN( ((s >>  2) & 0xf8) | ((s >>  7) & 0x7) );
   texel[BCOMP] = UBYTE_TO_CHAN( ((s <<  3) & 0xf8) | ((s >>  2) & 0x7) );
   texel[ACOMP] = UBYTE_TO_CHAN( ((s >> 15) & 0x01) * 255 );
}

#if DIM == 3
static void store_texel_argb1555(struct gl_texture_image *texImage,
                                 GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLushort *dst = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   *dst = PACK_COLOR_1555(rgba[ACOMP], rgba[RCOMP], rgba[GCOMP], rgba[BCOMP]);
}
#endif


/* MESA_FORMAT_ARGB1555_REV **************************************************/

/* Fetch texel from 1D, 2D or 3D argb1555_rev texture, return 4 GLchans */
static void FETCH(argb1555_rev)( const struct gl_texture_image *texImage,
                                 GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   const GLushort s = (*src << 8) | (*src >> 8); /* byteswap */
   texel[RCOMP] = UBYTE_TO_CHAN( ((s >>  7) & 0xf8) | ((s >> 12) & 0x7) );
   texel[GCOMP] = UBYTE_TO_CHAN( ((s >>  2) & 0xf8) | ((s >>  7) & 0x7) );
   texel[BCOMP] = UBYTE_TO_CHAN( ((s <<  3) & 0xf8) | ((s >>  2) & 0x7) );
   texel[ACOMP] = UBYTE_TO_CHAN( ((s >> 15) & 0x01) * 255 );
}

#if DIM == 3
static void store_texel_argb1555_rev(struct gl_texture_image *texImage,
                                 GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLushort *dst = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   *dst = PACK_COLOR_1555_REV(rgba[ACOMP], rgba[RCOMP], rgba[GCOMP], rgba[BCOMP]);
}
#endif


/* MESA_FORMAT_AL88 **********************************************************/

/* Fetch texel from 1D, 2D or 3D al88 texture, return 4 GLchans */
static void FETCH(al88)( const struct gl_texture_image *texImage,
			 GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLushort s = *TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[RCOMP] = 
   texel[GCOMP] = 
   texel[BCOMP] = UBYTE_TO_CHAN( s & 0xff );
   texel[ACOMP] = UBYTE_TO_CHAN( s >> 8 );
}

#if DIM == 3
static void store_texel_al88(struct gl_texture_image *texImage,
                             GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLushort *dst = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   *dst = PACK_COLOR_88(rgba[ACOMP], rgba[RCOMP]);
}
#endif


/* MESA_FORMAT_AL88_REV ******************************************************/

/* Fetch texel from 1D, 2D or 3D al88_rev texture, return 4 GLchans */
static void FETCH(al88_rev)( const struct gl_texture_image *texImage,
                             GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLushort s = *TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[RCOMP] = 
   texel[GCOMP] = 
   texel[BCOMP] = UBYTE_TO_CHAN( s >> 8 );
   texel[ACOMP] = UBYTE_TO_CHAN( s & 0xff );
}

#if DIM == 3
static void store_texel_al88_rev(struct gl_texture_image *texImage,
                                 GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLushort *dst = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   *dst = PACK_COLOR_88(rgba[RCOMP], rgba[ACOMP]);
}
#endif


/* MESA_FORMAT_RGB332 ********************************************************/

/* Fetch texel from 1D, 2D or 3D rgb332 texture, return 4 GLchans */
static void FETCH(rgb332)( const struct gl_texture_image *texImage,
			   GLint i, GLint j, GLint k, GLchan *texel )
{
   static const GLubyte lut2to8[4] = {0, 85, 170, 255};
   static const GLubyte lut3to8[8] = {0, 36, 73, 109, 146, 182, 219, 255};
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   const GLubyte s = *src;
   texel[RCOMP] = UBYTE_TO_CHAN( lut3to8[(s >> 5) & 0x7] );
   texel[GCOMP] = UBYTE_TO_CHAN( lut3to8[(s >> 2) & 0x7] );
   texel[BCOMP] = UBYTE_TO_CHAN( lut2to8[(s     ) & 0x3] );
   texel[ACOMP] = CHAN_MAX;
}

#if DIM == 3
static void store_texel_rgb332(struct gl_texture_image *texImage,
                               GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLubyte *dst = TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   *dst = PACK_COLOR_332(rgba[RCOMP], rgba[GCOMP], rgba[BCOMP]);
}
#endif


/* MESA_FORMAT_A8 ************************************************************/

/* Fetch texel from 1D, 2D or 3D a8 texture, return 4 GLchans */
static void FETCH(a8)( const struct gl_texture_image *texImage,
		       GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = 0;
   texel[ACOMP] = UBYTE_TO_CHAN( src[0] );
}

#if DIM == 3
static void store_texel_a8(struct gl_texture_image *texImage,
                           GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLubyte *dst = TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   *dst = rgba[ACOMP];
}
#endif


/* MESA_FORMAT_L8 ************************************************************/

/* Fetch texel from 1D, 2D or 3D l8 texture, return 4 GLchans */
static void FETCH(l8)( const struct gl_texture_image *texImage,
		       GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = UBYTE_TO_CHAN( src[0] );
   texel[ACOMP] = CHAN_MAX;
}

#if DIM == 3
static void store_texel_l8(struct gl_texture_image *texImage,
                           GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLubyte *dst = TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   *dst = rgba[RCOMP];
}
#endif


/* MESA_FORMAT_I8 ************************************************************/

/* Fetch texel from 1D, 2D or 3D i8 texture, return 4 GLchans */
static void FETCH(i8)( const struct gl_texture_image *texImage,
		       GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] =
   texel[ACOMP] = UBYTE_TO_CHAN( src[0] );
}

#if DIM == 3
static void store_texel_i8(struct gl_texture_image *texImage,
                           GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLubyte *dst = TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   *dst = rgba[RCOMP];
}
#endif


/* MESA_FORMAT_CI8 ***********************************************************/

/* Fetch CI texel from 1D, 2D or 3D ci8 texture, lookup the index in a
 * color table, and return 4 GLchans.
 */
static void FETCH(ci8)( const struct gl_texture_image *texImage,
			GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   const struct gl_color_table *palette;
   GLubyte texelUB[4];
   GLuint index;
   GET_CURRENT_CONTEXT(ctx);

   if (ctx->Texture.SharedPalette) {
      palette = &ctx->Texture.Palette;
   }
   else {
      palette = &texImage->TexObject->Palette;
   }
   if (palette->Size == 0)
      return; /* undefined results */

   /* Mask the index against size of palette to avoid going out of bounds */
   index = (*src) & (palette->Size - 1);

   {
      const GLubyte *table = palette->TableUB;
      switch (palette->_BaseFormat) {
      case GL_ALPHA:
         texelUB[RCOMP] =
         texelUB[GCOMP] =
         texelUB[BCOMP] = 0;
         texelUB[ACOMP] = table[index];
         break;;
      case GL_LUMINANCE:
         texelUB[RCOMP] =
         texelUB[GCOMP] =
         texelUB[BCOMP] = table[index];
         texelUB[ACOMP] = 255;
         break;
      case GL_INTENSITY:
         texelUB[RCOMP] =
         texelUB[GCOMP] =
         texelUB[BCOMP] =
         texelUB[ACOMP] = table[index];
         break;;
      case GL_LUMINANCE_ALPHA:
         texelUB[RCOMP] =
         texelUB[GCOMP] =
         texelUB[BCOMP] = table[index * 2 + 0];
         texelUB[ACOMP] = table[index * 2 + 1];
         break;;
      case GL_RGB:
         texelUB[RCOMP] = table[index * 3 + 0];
         texelUB[GCOMP] = table[index * 3 + 1];
         texelUB[BCOMP] = table[index * 3 + 2];
         texelUB[ACOMP] = 255;
         break;;
      case GL_RGBA:
         texelUB[RCOMP] = table[index * 4 + 0];
         texelUB[GCOMP] = table[index * 4 + 1];
         texelUB[BCOMP] = table[index * 4 + 2];
         texelUB[ACOMP] = table[index * 4 + 3];
         break;;
      default:
         _mesa_problem(ctx, "Bad palette format in fetch_texel_ci8");
         return;
      }
#if CHAN_TYPE == GL_UNSIGNED_BYTE
      COPY_4UBV(texel, texelUB);
#elif CHAN_TYPE == GL_UNSIGNED_SHORT
      texel[0] = UBYTE_TO_USHORT(texelUB[0]);
      texel[1] = UBYTE_TO_USHORT(texelUB[1]);
      texel[2] = UBYTE_TO_USHORT(texelUB[2]);
      texel[3] = UBYTE_TO_USHORT(texelUB[3]);
#else
      texel[0] = UBYTE_TO_FLOAT(texelUB[0]);
      texel[1] = UBYTE_TO_FLOAT(texelUB[1]);
      texel[2] = UBYTE_TO_FLOAT(texelUB[2]);
      texel[3] = UBYTE_TO_FLOAT(texelUB[3]);
#endif
   }
}

#if DIM == 3
static void store_texel_ci8(struct gl_texture_image *texImage,
                            GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *index = (const GLubyte *) texel;
   GLubyte *dst = TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   *dst = *index;
}
#endif


#if FEATURE_EXT_texture_sRGB

/* Fetch texel from 1D, 2D or 3D srgb8 texture, return 4 GLfloats */
static void FETCH(srgb8)(const struct gl_texture_image *texImage,
                         GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 3);
   texel[RCOMP] = nonlinear_to_linear(src[0]);
   texel[GCOMP] = nonlinear_to_linear(src[1]);
   texel[BCOMP] = nonlinear_to_linear(src[2]);
   texel[ACOMP] = CHAN_MAX;
}

#if DIM == 3
static void store_texel_srgb8(struct gl_texture_image *texImage,
                              GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLubyte *dst = TEXEL_ADDR(GLubyte, texImage, i, j, k, 3);
   dst[0] = rgba[RCOMP]; /* no conversion */
   dst[1] = rgba[GCOMP];
   dst[2] = rgba[BCOMP];
}
#endif

/* Fetch texel from 1D, 2D or 3D srgba8 texture, return 4 GLfloats */
static void FETCH(srgba8)(const struct gl_texture_image *texImage,
                          GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 4);
   texel[RCOMP] = nonlinear_to_linear(src[0]);
   texel[GCOMP] = nonlinear_to_linear(src[1]);
   texel[BCOMP] = nonlinear_to_linear(src[2]);
   texel[ACOMP] = UBYTE_TO_FLOAT(src[3]); /* linear! */
}

#if DIM == 3
static void store_texel_srgba8(struct gl_texture_image *texImage,
                               GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLubyte *dst = TEXEL_ADDR(GLubyte, texImage, i, j, k, 4);
   dst[0] = rgba[RCOMP];
   dst[1] = rgba[GCOMP];
   dst[2] = rgba[BCOMP];
}
#endif

/* Fetch texel from 1D, 2D or 3D sl8 texture, return 4 GLfloats */
static void FETCH(sl8)(const struct gl_texture_image *texImage,
                       GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   texel[RCOMP] = 
   texel[GCOMP] = 
   texel[BCOMP] = nonlinear_to_linear(src[0]);
   texel[ACOMP] = CHAN_MAX;
}

#if DIM == 3
static void store_texel_sl8(struct gl_texture_image *texImage,
                            GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLubyte *dst = TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   dst[0] = rgba[RCOMP];
}
#endif

/* Fetch texel from 1D, 2D or 3D sla8 texture, return 4 GLfloats */
static void FETCH(sla8)(const struct gl_texture_image *texImage,
                       GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 2);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = nonlinear_to_linear(src[0]);
   texel[ACOMP] = UBYTE_TO_FLOAT(src[1]); /* linear */
}

#if DIM == 3
static void store_texel_sla8(struct gl_texture_image *texImage,
                            GLint i, GLint j, GLint k, const void *texel)
{
   const GLubyte *rgba = (const GLubyte *) texel;
   GLubyte *dst = TEXEL_ADDR(GLubyte, texImage, i, j, k, 2);
   dst[0] = rgba[RCOMP];
   dst[1] = rgba[ACOMP];
}
#endif



#endif /* FEATURE_EXT_texture_sRGB */



/* MESA_FORMAT_YCBCR *********************************************************/

/* Fetch texel from 1D, 2D or 3D ycbcr texture, return 4 GLchans */
/* We convert YCbCr to RGB here */
/* XXX this may break if GLchan != GLubyte */
static void FETCH(ycbcr)( const struct gl_texture_image *texImage,
                          GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLushort *src0 = TEXEL_ADDR(GLushort, texImage, (i & ~1), j, k, 1); /* even */
   const GLushort *src1 = src0 + 1;                               /* odd */
   const GLubyte y0 = (*src0 >> 8) & 0xff;  /* luminance */
   const GLubyte cb = *src0 & 0xff;         /* chroma U */
   const GLubyte y1 = (*src1 >> 8) & 0xff;  /* luminance */
   const GLubyte cr = *src1 & 0xff;         /* chroma V */
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
   texel[RCOMP] = CLAMP(r, 0, CHAN_MAX);
   texel[GCOMP] = CLAMP(g, 0, CHAN_MAX);
   texel[BCOMP] = CLAMP(b, 0, CHAN_MAX);
   texel[ACOMP] = CHAN_MAX;
}

#if DIM == 3
static void store_texel_ycbcr(struct gl_texture_image *texImage,
                              GLint i, GLint j, GLint k, const void *texel)
{
   (void) texImage;
   (void) i;
   (void) j;
   (void) k;
   (void) texel;
   /* XXX to do */
}
#endif


/* MESA_FORMAT_YCBCR_REV *****************************************************/

/* Fetch texel from 1D, 2D or 3D ycbcr_rev texture, return 4 GLchans */
/* We convert YCbCr to RGB here */
/* XXX this may break if GLchan != GLubyte */
static void FETCH(ycbcr_rev)( const struct gl_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLushort *src0 = TEXEL_ADDR(GLushort, texImage, (i & ~1), j, k, 1); /* even */
   const GLushort *src1 = src0 + 1;                               /* odd */
   const GLubyte y0 = *src0 & 0xff;         /* luminance */
   const GLubyte cr = (*src0 >> 8) & 0xff;  /* chroma V */
   const GLubyte y1 = *src1 & 0xff;         /* luminance */
   const GLubyte cb = (*src1 >> 8) & 0xff;  /* chroma U */
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
   texel[RCOMP] = CLAMP(r, 0, CHAN_MAX);
   texel[GCOMP] = CLAMP(g, 0, CHAN_MAX);
   texel[BCOMP] = CLAMP(b, 0, CHAN_MAX);
   texel[ACOMP] = CHAN_MAX;
}

#if DIM == 3
static void store_texel_ycbcr_rev(struct gl_texture_image *texImage,
                                  GLint i, GLint j, GLint k, const void *texel)
{
   (void) texImage;
   (void) i;
   (void) j;
   (void) k;
   (void) texel;
   /* XXX to do */
}
#endif


/* MESA_TEXFORMAT_Z24_S8 ***************************************************/

static void FETCH(f_z24_s8)( const struct gl_texture_image *texImage,
                             GLint i, GLint j, GLint k, GLfloat *texel )
{
   /* only return Z, not stencil data */
   const GLuint *src = TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   const GLfloat scale = 1.0F / (GLfloat) 0xffffff;
   texel[0] = ((*src) >> 8) * scale;
   ASSERT(texImage->TexFormat->MesaFormat == MESA_FORMAT_Z24_S8);
   ASSERT(texel[0] >= 0.0F);
   ASSERT(texel[0] <= 1.0F);
}

#if DIM == 3
static void store_texel_z24_s8(struct gl_texture_image *texImage,
                               GLint i, GLint j, GLint k, const void *texel)
{
   /* only store Z, not stencil */
   GLuint *dst = TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   GLfloat depth = *((GLfloat *) texel);
   GLuint zi = ((GLuint) (depth * 0xffffff)) << 8;
   *dst = zi | (*dst & 0xff);
}
#endif


/* MESA_TEXFORMAT_S8_Z24 ***************************************************/

static void FETCH(f_s8_z24)( const struct gl_texture_image *texImage,
                             GLint i, GLint j, GLint k, GLfloat *texel )
{
   /* only return Z, not stencil data */
   const GLuint *src = TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   const GLfloat scale = 1.0F / (GLfloat) 0xffffff;
   texel[0] = ((*src) & 0x00ffffff) * scale;
   ASSERT(texImage->TexFormat->MesaFormat == MESA_FORMAT_S8_Z24);
   ASSERT(texel[0] >= 0.0F);
   ASSERT(texel[0] <= 1.0F);
}

#if DIM == 3
static void store_texel_s8_z24(struct gl_texture_image *texImage,
                               GLint i, GLint j, GLint k, const void *texel)
{
   /* only store Z, not stencil */
   GLuint *dst = TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   GLfloat depth = *((GLfloat *) texel);
   GLuint zi = (GLuint) (depth * 0xffffff);
   *dst = zi | (*dst & 0xff000000);
}
#endif


#undef TEXEL_ADDR
#undef DIM
#undef FETCH
