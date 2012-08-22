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


/**
 * \file texfetch_tmp.h
 * Texel fetch functions template.
 * 
 * This template file is used by texfetch.c to generate texel fetch functions
 * for 1-D, 2-D and 3-D texture images. 
 *
 * It should be expanded by defining \p DIM as the number texture dimensions
 * (1, 2 or 3).  According to the value of \p DIM a series of macros is defined
 * for the texel lookup in the gl_texture_image::Data.
 * 
 * \author Gareth Hughes
 * \author Brian Paul
 */


#if DIM == 1

#define TEXEL_ADDR( type, image, i, j, k, size ) \
	((void) (j), (void) (k), ((type *)(image)->Map + (i) * (size)))

#define FETCH(x) fetch_texel_1d_##x

#elif DIM == 2

#define TEXEL_ADDR( type, image, i, j, k, size )			\
	((void) (k),							\
	 ((type *)(image)->Map + ((image)->RowStride * (j) + (i)) * (size)))

#define FETCH(x) fetch_texel_2d_##x

#elif DIM == 3

#define TEXEL_ADDR( type, image, i, j, k, size )			\
	((type *)(image)->Map + ((image)->ImageOffsets[k]		\
             + (image)->RowStride * (j) + (i)) * (size))

#define FETCH(x) fetch_texel_3d_##x

#else
#error	illegal number of texture dimensions
#endif


/* MESA_FORMAT_Z32 ***********************************************************/

/* Fetch depth texel from 1D, 2D or 3D 32-bit depth texture,
 * returning 1 GLfloat.
 * Note: no GLchan version of this function.
 */
static void FETCH(f_z32)( const struct swrast_texture_image *texImage,
                          GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLuint *src = TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[0] = src[0] * (1.0F / 0xffffffff);
}


/* MESA_FORMAT_Z16 ***********************************************************/

/* Fetch depth texel from 1D, 2D or 3D 16-bit depth texture,
 * returning 1 GLfloat.
 * Note: no GLchan version of this function.
 */
static void FETCH(f_z16)(const struct swrast_texture_image *texImage,
                         GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[0] = src[0] * (1.0F / 65535.0F);
}



/* MESA_FORMAT_RGBA_F32 ******************************************************/

/* Fetch texel from 1D, 2D or 3D RGBA_FLOAT32 texture, returning 4 GLfloats.
 */
static void FETCH(f_rgba_f32)( const struct swrast_texture_image *texImage,
                               GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 4);
   texel[RCOMP] = src[0];
   texel[GCOMP] = src[1];
   texel[BCOMP] = src[2];
   texel[ACOMP] = src[3];
}




/* MESA_FORMAT_RGBA_F16 ******************************************************/

/* Fetch texel from 1D, 2D or 3D RGBA_FLOAT16 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_rgba_f16)( const struct swrast_texture_image *texImage,
                               GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLhalfARB *src = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 4);
   texel[RCOMP] = _mesa_half_to_float(src[0]);
   texel[GCOMP] = _mesa_half_to_float(src[1]);
   texel[BCOMP] = _mesa_half_to_float(src[2]);
   texel[ACOMP] = _mesa_half_to_float(src[3]);
}



/* MESA_FORMAT_RGB_F32 *******************************************************/

/* Fetch texel from 1D, 2D or 3D RGB_FLOAT32 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_rgb_f32)( const struct swrast_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 3);
   texel[RCOMP] = src[0];
   texel[GCOMP] = src[1];
   texel[BCOMP] = src[2];
   texel[ACOMP] = 1.0F;
}




/* MESA_FORMAT_RGB_F16 *******************************************************/

/* Fetch texel from 1D, 2D or 3D RGB_FLOAT16 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_rgb_f16)( const struct swrast_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLhalfARB *src = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 3);
   texel[RCOMP] = _mesa_half_to_float(src[0]);
   texel[GCOMP] = _mesa_half_to_float(src[1]);
   texel[BCOMP] = _mesa_half_to_float(src[2]);
   texel[ACOMP] = 1.0F;
}




/* MESA_FORMAT_ALPHA_F32 *****************************************************/

/* Fetch texel from 1D, 2D or 3D ALPHA_FLOAT32 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_alpha_f32)( const struct swrast_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = src[0];
}




/* MESA_FORMAT_ALPHA_F32 *****************************************************/

/* Fetch texel from 1D, 2D or 3D ALPHA_FLOAT16 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_alpha_f16)( const struct swrast_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLhalfARB *src = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = _mesa_half_to_float(src[0]);
}




/* MESA_FORMAT_LUMINANCE_F32 *************************************************/

/* Fetch texel from 1D, 2D or 3D LUMINANCE_FLOAT32 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_luminance_f32)( const struct swrast_texture_image *texImage,
                                    GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = src[0];
   texel[ACOMP] = 1.0F;
}




/* MESA_FORMAT_LUMINANCE_F16 *************************************************/

/* Fetch texel from 1D, 2D or 3D LUMINANCE_FLOAT16 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_luminance_f16)( const struct swrast_texture_image *texImage,
                                    GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLhalfARB *src = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = _mesa_half_to_float(src[0]);
   texel[ACOMP] = 1.0F;
}




/* MESA_FORMAT_LUMINANCE_ALPHA_F32 *******************************************/

/* Fetch texel from 1D, 2D or 3D LUMINANCE_ALPHA_FLOAT32 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_luminance_alpha_f32)( const struct swrast_texture_image *texImage,
                                    GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 2);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = src[0];
   texel[ACOMP] = src[1];
}




/* MESA_FORMAT_LUMINANCE_ALPHA_F16 *******************************************/

/* Fetch texel from 1D, 2D or 3D LUMINANCE_ALPHA_FLOAT16 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_luminance_alpha_f16)( const struct swrast_texture_image *texImage,
                                    GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLhalfARB *src = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 2);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = _mesa_half_to_float(src[0]);
   texel[ACOMP] = _mesa_half_to_float(src[1]);
}




/* MESA_FORMAT_INTENSITY_F32 *************************************************/

/* Fetch texel from 1D, 2D or 3D INTENSITY_FLOAT32 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_intensity_f32)( const struct swrast_texture_image *texImage,
                                    GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] =
   texel[ACOMP] = src[0];
}




/* MESA_FORMAT_INTENSITY_F16 *************************************************/

/* Fetch texel from 1D, 2D or 3D INTENSITY_FLOAT16 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_intensity_f16)( const struct swrast_texture_image *texImage,
                                    GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLhalfARB *src = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] =
   texel[ACOMP] = _mesa_half_to_float(src[0]);
}




/* MESA_FORMAT_R_FLOAT32 *****************************************************/

/* Fetch texel from 1D, 2D or 3D R_FLOAT32 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_r_f32)( const struct swrast_texture_image *texImage,
                            GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 1);
   texel[RCOMP] = src[0];
   texel[GCOMP] = 0.0F;
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = 1.0F;
}




/* MESA_FORMAT_R_FLOAT16 *****************************************************/

/* Fetch texel from 1D, 2D or 3D R_FLOAT16 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_r_f16)( const struct swrast_texture_image *texImage,
                            GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLhalfARB *src = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 1);
   texel[RCOMP] = _mesa_half_to_float(src[0]);
   texel[GCOMP] = 0.0F;
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = 1.0F;
}




/* MESA_FORMAT_RG_FLOAT32 ****************************************************/

/* Fetch texel from 1D, 2D or 3D RG_FLOAT32 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_rg_f32)( const struct swrast_texture_image *texImage,
                             GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 2);
   texel[RCOMP] = src[0];
   texel[GCOMP] = src[1];
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = 1.0F;
}




/* MESA_FORMAT_RG_FLOAT16 ****************************************************/

/* Fetch texel from 1D, 2D or 3D RG_FLOAT16 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_rg_f16)( const struct swrast_texture_image *texImage,
                             GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLhalfARB *src = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 2);
   texel[RCOMP] = _mesa_half_to_float(src[0]);
   texel[GCOMP] = _mesa_half_to_float(src[1]);
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = 1.0F;
}




/*
 * Begin Hardware formats
 */

/* MESA_FORMAT_RGBA8888 ******************************************************/

/* Fetch texel from 1D, 2D or 3D rgba8888 texture, return 4 GLfloats */
static void FETCH(f_rgba8888)( const struct swrast_texture_image *texImage,
                               GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_FLOAT( (s >> 24)        );
   texel[GCOMP] = UBYTE_TO_FLOAT( (s >> 16) & 0xff );
   texel[BCOMP] = UBYTE_TO_FLOAT( (s >>  8) & 0xff );
   texel[ACOMP] = UBYTE_TO_FLOAT( (s      ) & 0xff );
}






/* MESA_FORMAT_RGBA888_REV ***************************************************/

/* Fetch texel from 1D, 2D or 3D abgr8888 texture, return 4 GLchans */
static void FETCH(f_rgba8888_rev)( const struct swrast_texture_image *texImage,
                                   GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_FLOAT( (s      ) & 0xff );
   texel[GCOMP] = UBYTE_TO_FLOAT( (s >>  8) & 0xff );
   texel[BCOMP] = UBYTE_TO_FLOAT( (s >> 16) & 0xff );
   texel[ACOMP] = UBYTE_TO_FLOAT( (s >> 24)        );
}




/* MESA_FORMAT_ARGB8888 ******************************************************/

/* Fetch texel from 1D, 2D or 3D argb8888 texture, return 4 GLchans */
static void FETCH(f_argb8888)( const struct swrast_texture_image *texImage,
                               GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_FLOAT( (s >> 16) & 0xff );
   texel[GCOMP] = UBYTE_TO_FLOAT( (s >>  8) & 0xff );
   texel[BCOMP] = UBYTE_TO_FLOAT( (s      ) & 0xff );
   texel[ACOMP] = UBYTE_TO_FLOAT( (s >> 24)        );
}




/* MESA_FORMAT_ARGB8888_REV **************************************************/

/* Fetch texel from 1D, 2D or 3D argb8888_rev texture, return 4 GLfloats */
static void FETCH(f_argb8888_rev)( const struct swrast_texture_image *texImage,
                                   GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_FLOAT( (s >>  8) & 0xff );
   texel[GCOMP] = UBYTE_TO_FLOAT( (s >> 16) & 0xff );
   texel[BCOMP] = UBYTE_TO_FLOAT( (s >> 24)        );
   texel[ACOMP] = UBYTE_TO_FLOAT( (s      ) & 0xff );
}




/* MESA_FORMAT_RGBX8888 ******************************************************/

/* Fetch texel from 1D, 2D or 3D rgbx8888 texture, return 4 GLfloats */
static void FETCH(f_rgbx8888)( const struct swrast_texture_image *texImage,
                               GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_FLOAT( (s >> 24)        );
   texel[GCOMP] = UBYTE_TO_FLOAT( (s >> 16) & 0xff );
   texel[BCOMP] = UBYTE_TO_FLOAT( (s >>  8) & 0xff );
   texel[ACOMP] = 1.0f;
}




/* MESA_FORMAT_RGBX888_REV ***************************************************/

/* Fetch texel from 1D, 2D or 3D rgbx8888_rev texture, return 4 GLchans */
static void FETCH(f_rgbx8888_rev)( const struct swrast_texture_image *texImage,
                                   GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_FLOAT( (s      ) & 0xff );
   texel[GCOMP] = UBYTE_TO_FLOAT( (s >>  8) & 0xff );
   texel[BCOMP] = UBYTE_TO_FLOAT( (s >> 16) & 0xff );
   texel[ACOMP] = 1.0f;
}




/* MESA_FORMAT_XRGB8888 ******************************************************/

/* Fetch texel from 1D, 2D or 3D xrgb8888 texture, return 4 GLchans */
static void FETCH(f_xrgb8888)( const struct swrast_texture_image *texImage,
                               GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_FLOAT( (s >> 16) & 0xff );
   texel[GCOMP] = UBYTE_TO_FLOAT( (s >>  8) & 0xff );
   texel[BCOMP] = UBYTE_TO_FLOAT( (s      ) & 0xff );
   texel[ACOMP] = 1.0f;
}




/* MESA_FORMAT_XRGB8888_REV **************************************************/

/* Fetch texel from 1D, 2D or 3D xrgb8888_rev texture, return 4 GLfloats */
static void FETCH(f_xrgb8888_rev)( const struct swrast_texture_image *texImage,
                                   GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_FLOAT( (s >>  8) & 0xff );
   texel[GCOMP] = UBYTE_TO_FLOAT( (s >> 16) & 0xff );
   texel[BCOMP] = UBYTE_TO_FLOAT( (s >> 24)        );
   texel[ACOMP] = 1.0f;
}




/* MESA_FORMAT_RGB888 ********************************************************/

/* Fetch texel from 1D, 2D or 3D rgb888 texture, return 4 GLchans */
static void FETCH(f_rgb888)( const struct swrast_texture_image *texImage,
                             GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 3);
   texel[RCOMP] = UBYTE_TO_FLOAT( src[2] );
   texel[GCOMP] = UBYTE_TO_FLOAT( src[1] );
   texel[BCOMP] = UBYTE_TO_FLOAT( src[0] );
   texel[ACOMP] = 1.0F;
}




/* MESA_FORMAT_BGR888 ********************************************************/

/* Fetch texel from 1D, 2D or 3D bgr888 texture, return 4 GLchans */
static void FETCH(f_bgr888)( const struct swrast_texture_image *texImage,
                             GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 3);
   texel[RCOMP] = UBYTE_TO_FLOAT( src[0] );
   texel[GCOMP] = UBYTE_TO_FLOAT( src[1] );
   texel[BCOMP] = UBYTE_TO_FLOAT( src[2] );
   texel[ACOMP] = 1.0F;
}




/* use color expansion like (g << 2) | (g >> 4) (does somewhat random rounding)
   instead of slow (g << 2) * 255 / 252 (always rounds down) */

/* MESA_FORMAT_RGB565 ********************************************************/

/* Fetch texel from 1D, 2D or 3D rgb565 texture, return 4 GLchans */
static void FETCH(f_rgb565)( const struct swrast_texture_image *texImage,
                             GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   const GLushort s = *src;
   texel[RCOMP] = ((s >> 11) & 0x1f) * (1.0F / 31.0F);
   texel[GCOMP] = ((s >> 5 ) & 0x3f) * (1.0F / 63.0F);
   texel[BCOMP] = ((s      ) & 0x1f) * (1.0F / 31.0F);
   texel[ACOMP] = 1.0F;
}




/* MESA_FORMAT_RGB565_REV ****************************************************/

/* Fetch texel from 1D, 2D or 3D rgb565_rev texture, return 4 GLchans */
static void FETCH(f_rgb565_rev)( const struct swrast_texture_image *texImage,
                                 GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   const GLushort s = (*src >> 8) | (*src << 8); /* byte swap */
   texel[RCOMP] = UBYTE_TO_FLOAT( ((s >> 8) & 0xf8) | ((s >> 13) & 0x7) );
   texel[GCOMP] = UBYTE_TO_FLOAT( ((s >> 3) & 0xfc) | ((s >>  9) & 0x3) );
   texel[BCOMP] = UBYTE_TO_FLOAT( ((s << 3) & 0xf8) | ((s >>  2) & 0x7) );
   texel[ACOMP] = 1.0F;
}




/* MESA_FORMAT_ARGB4444 ******************************************************/

/* Fetch texel from 1D, 2D or 3D argb444 texture, return 4 GLchans */
static void FETCH(f_argb4444)( const struct swrast_texture_image *texImage,
                               GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   const GLushort s = *src;
   texel[RCOMP] = ((s >>  8) & 0xf) * (1.0F / 15.0F);
   texel[GCOMP] = ((s >>  4) & 0xf) * (1.0F / 15.0F);
   texel[BCOMP] = ((s      ) & 0xf) * (1.0F / 15.0F);
   texel[ACOMP] = ((s >> 12) & 0xf) * (1.0F / 15.0F);
}




/* MESA_FORMAT_ARGB4444_REV **************************************************/

/* Fetch texel from 1D, 2D or 3D argb4444_rev texture, return 4 GLchans */
static void FETCH(f_argb4444_rev)( const struct swrast_texture_image *texImage,
                                   GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort s = *TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[RCOMP] = ((s      ) & 0xf) * (1.0F / 15.0F);
   texel[GCOMP] = ((s >> 12) & 0xf) * (1.0F / 15.0F);
   texel[BCOMP] = ((s >>  8) & 0xf) * (1.0F / 15.0F);
   texel[ACOMP] = ((s >>  4) & 0xf) * (1.0F / 15.0F);
}



/* MESA_FORMAT_RGBA5551 ******************************************************/

/* Fetch texel from 1D, 2D or 3D argb1555 texture, return 4 GLchans */
static void FETCH(f_rgba5551)( const struct swrast_texture_image *texImage,
                               GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   const GLushort s = *src;
   texel[RCOMP] = ((s >> 11) & 0x1f) * (1.0F / 31.0F);
   texel[GCOMP] = ((s >>  6) & 0x1f) * (1.0F / 31.0F);
   texel[BCOMP] = ((s >>  1) & 0x1f) * (1.0F / 31.0F);
   texel[ACOMP] = ((s      ) & 0x01) * 1.0F;
}



/* MESA_FORMAT_ARGB1555 ******************************************************/

/* Fetch texel from 1D, 2D or 3D argb1555 texture, return 4 GLchans */
static void FETCH(f_argb1555)( const struct swrast_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   const GLushort s = *src;
   texel[RCOMP] = ((s >> 10) & 0x1f) * (1.0F / 31.0F);
   texel[GCOMP] = ((s >>  5) & 0x1f) * (1.0F / 31.0F);
   texel[BCOMP] = ((s >>  0) & 0x1f) * (1.0F / 31.0F);
   texel[ACOMP] = ((s >> 15) & 0x01) * 1.0F;
}




/* MESA_FORMAT_ARGB1555_REV **************************************************/

/* Fetch texel from 1D, 2D or 3D argb1555_rev texture, return 4 GLchans */
static void FETCH(f_argb1555_rev)( const struct swrast_texture_image *texImage,
                                   GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   const GLushort s = (*src << 8) | (*src >> 8); /* byteswap */
   texel[RCOMP] = UBYTE_TO_FLOAT( ((s >>  7) & 0xf8) | ((s >> 12) & 0x7) );
   texel[GCOMP] = UBYTE_TO_FLOAT( ((s >>  2) & 0xf8) | ((s >>  7) & 0x7) );
   texel[BCOMP] = UBYTE_TO_FLOAT( ((s <<  3) & 0xf8) | ((s >>  2) & 0x7) );
   texel[ACOMP] = UBYTE_TO_FLOAT( ((s >> 15) & 0x01) * 255 );
}




/* MESA_FORMAT_ARGB2101010 ***************************************************/

/* Fetch texel from 1D, 2D or 3D argb2101010 texture, return 4 GLchans */
static void FETCH(f_argb2101010)( const struct swrast_texture_image *texImage,
                                  GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLuint *src = TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   const GLuint s = *src;
   texel[RCOMP] = ((s >> 20) & 0x3ff) * (1.0F / 1023.0F);
   texel[GCOMP] = ((s >> 10) & 0x3ff) * (1.0F / 1023.0F);
   texel[BCOMP] = ((s >>  0) & 0x3ff) * (1.0F / 1023.0F);
   texel[ACOMP] = ((s >> 30) & 0x03) * (1.0F / 3.0F);
}




/* MESA_FORMAT_GR88 **********************************************************/

/* Fetch texel from 1D, 2D or 3D rg88 texture, return 4 GLchans */
static void FETCH(f_gr88)( const struct swrast_texture_image *texImage,
                           GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort s = *TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_FLOAT( s & 0xff );
   texel[GCOMP] = UBYTE_TO_FLOAT( s >> 8 );
   texel[BCOMP] = 0.0;
   texel[ACOMP] = 1.0;
}




/* MESA_FORMAT_RG88 ******************************************************/

/* Fetch texel from 1D, 2D or 3D rg88_rev texture, return 4 GLchans */
static void FETCH(f_rg88)( const struct swrast_texture_image *texImage,
                           GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort s = *TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_FLOAT( s >> 8 );
   texel[GCOMP] = UBYTE_TO_FLOAT( s & 0xff );
   texel[BCOMP] = 0.0;
   texel[ACOMP] = 1.0;
}




/* MESA_FORMAT_AL44 **********************************************************/

/* Fetch texel from 1D, 2D or 3D al44 texture, return 4 GLchans */
static void FETCH(f_al44)( const struct swrast_texture_image *texImage,
                           GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte s = *TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = (s & 0xf) * (1.0F / 15.0F);
   texel[ACOMP] = ((s >> 4) & 0xf) * (1.0F / 15.0F);
}




/* MESA_FORMAT_AL88 **********************************************************/

/* Fetch texel from 1D, 2D or 3D al88 texture, return 4 GLchans */
static void FETCH(f_al88)( const struct swrast_texture_image *texImage,
                           GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort s = *TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[RCOMP] = 
   texel[GCOMP] = 
   texel[BCOMP] = UBYTE_TO_FLOAT( s & 0xff );
   texel[ACOMP] = UBYTE_TO_FLOAT( s >> 8 );
}




/* MESA_FORMAT_R8 ************************************************************/

/* Fetch texel from 1D, 2D or 3D rg88 texture, return 4 GLchans */
static void FETCH(f_r8)(const struct swrast_texture_image *texImage,
			GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLubyte s = *TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_FLOAT(s);
   texel[GCOMP] = 0.0;
   texel[BCOMP] = 0.0;
   texel[ACOMP] = 1.0;
}




/* MESA_FORMAT_R16 ***********************************************************/

/* Fetch texel from 1D, 2D or 3D r16 texture, return 4 GLchans */
static void FETCH(f_r16)(const struct swrast_texture_image *texImage,
			GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort s = *TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[RCOMP] = USHORT_TO_FLOAT(s);
   texel[GCOMP] = 0.0;
   texel[BCOMP] = 0.0;
   texel[ACOMP] = 1.0;
}




/* MESA_FORMAT_AL88_REV ******************************************************/

/* Fetch texel from 1D, 2D or 3D al88_rev texture, return 4 GLchans */
static void FETCH(f_al88_rev)( const struct swrast_texture_image *texImage,
                               GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort s = *TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[RCOMP] = 
   texel[GCOMP] = 
   texel[BCOMP] = UBYTE_TO_FLOAT( s >> 8 );
   texel[ACOMP] = UBYTE_TO_FLOAT( s & 0xff );
}




/* MESA_FORMAT_RG1616 ********************************************************/

/* Fetch texel from 1D, 2D or 3D rg1616 texture, return 4 GLchans */
static void FETCH(f_rg1616)( const struct swrast_texture_image *texImage,
                           GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = USHORT_TO_FLOAT( s & 0xffff );
   texel[GCOMP] = USHORT_TO_FLOAT( s >> 16 );
   texel[BCOMP] = 0.0;
   texel[ACOMP] = 1.0;
}




/* MESA_FORMAT_RG1616_REV ****************************************************/

/* Fetch texel from 1D, 2D or 3D rg1616_rev texture, return 4 GLchans */
static void FETCH(f_rg1616_rev)( const struct swrast_texture_image *texImage,
                           GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = USHORT_TO_FLOAT( s >> 16 );
   texel[GCOMP] = USHORT_TO_FLOAT( s & 0xffff );
   texel[BCOMP] = 0.0;
   texel[ACOMP] = 1.0;
}




/* MESA_FORMAT_AL1616 ********************************************************/

/* Fetch texel from 1D, 2D or 3D al1616 texture, return 4 GLchans */
static void FETCH(f_al1616)( const struct swrast_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = USHORT_TO_FLOAT( s & 0xffff );
   texel[ACOMP] = USHORT_TO_FLOAT( s >> 16 );
}




/* MESA_FORMAT_AL1616_REV ****************************************************/

/* Fetch texel from 1D, 2D or 3D al1616_rev texture, return 4 GLchans */
static void FETCH(f_al1616_rev)( const struct swrast_texture_image *texImage,
				 GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = USHORT_TO_FLOAT( s >> 16 );
   texel[ACOMP] = USHORT_TO_FLOAT( s & 0xffff );
}




/* MESA_FORMAT_RGB332 ********************************************************/

/* Fetch texel from 1D, 2D or 3D rgb332 texture, return 4 GLchans */
static void FETCH(f_rgb332)( const struct swrast_texture_image *texImage,
                             GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   const GLubyte s = *src;
   texel[RCOMP] = ((s >> 5) & 0x7) * (1.0F / 7.0F);
   texel[GCOMP] = ((s >> 2) & 0x7) * (1.0F / 7.0F);
   texel[BCOMP] = ((s     ) & 0x3) * (1.0F / 3.0F);
   texel[ACOMP] = 1.0F;
}




/* MESA_FORMAT_A8 ************************************************************/

/* Fetch texel from 1D, 2D or 3D a8 texture, return 4 GLchans */
static void FETCH(f_a8)( const struct swrast_texture_image *texImage,
                         GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = UBYTE_TO_FLOAT( src[0] );
}




/* MESA_FORMAT_A16 ************************************************************/

/* Fetch texel from 1D, 2D or 3D a8 texture, return 4 GLchans */
static void FETCH(f_a16)( const struct swrast_texture_image *texImage,
                          GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = USHORT_TO_FLOAT( src[0] );
}




/* MESA_FORMAT_L8 ************************************************************/

/* Fetch texel from 1D, 2D or 3D l8 texture, return 4 GLchans */
static void FETCH(f_l8)( const struct swrast_texture_image *texImage,
                         GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = UBYTE_TO_FLOAT( src[0] );
   texel[ACOMP] = 1.0F;
}




/* MESA_FORMAT_L16 ***********************************************************/

/* Fetch texel from 1D, 2D or 3D l16 texture, return 4 GLchans */
static void FETCH(f_l16)( const struct swrast_texture_image *texImage,
                          GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = USHORT_TO_FLOAT( src[0] );
   texel[ACOMP] = 1.0F;
}




/* MESA_FORMAT_I8 ************************************************************/

/* Fetch texel from 1D, 2D or 3D i8 texture, return 4 GLchans */
static void FETCH(f_i8)( const struct swrast_texture_image *texImage,
                         GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] =
   texel[ACOMP] = UBYTE_TO_FLOAT( src[0] );
}




/* MESA_FORMAT_I16 ***********************************************************/

/* Fetch texel from 1D, 2D or 3D i16 texture, return 4 GLchans */
static void FETCH(f_i16)( const struct swrast_texture_image *texImage,
                          GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] =
   texel[ACOMP] = USHORT_TO_FLOAT( src[0] );
}




/* Fetch texel from 1D, 2D or 3D srgb8 texture, return 4 GLfloats */
/* Note: component order is same as for MESA_FORMAT_RGB888 */
static void FETCH(srgb8)(const struct swrast_texture_image *texImage,
                         GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 3);
   texel[RCOMP] = nonlinear_to_linear(src[2]);
   texel[GCOMP] = nonlinear_to_linear(src[1]);
   texel[BCOMP] = nonlinear_to_linear(src[0]);
   texel[ACOMP] = 1.0F;
}



/* Fetch texel from 1D, 2D or 3D srgba8 texture, return 4 GLfloats */
static void FETCH(srgba8)(const struct swrast_texture_image *texImage,
                          GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = nonlinear_to_linear( (s >> 24) );
   texel[GCOMP] = nonlinear_to_linear( (s >> 16) & 0xff );
   texel[BCOMP] = nonlinear_to_linear( (s >>  8) & 0xff );
   texel[ACOMP] = UBYTE_TO_FLOAT( (s      ) & 0xff ); /* linear! */
}



/* Fetch texel from 1D, 2D or 3D sargb8 texture, return 4 GLfloats */
static void FETCH(sargb8)(const struct swrast_texture_image *texImage,
                          GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = nonlinear_to_linear( (s >> 16) & 0xff );
   texel[GCOMP] = nonlinear_to_linear( (s >>  8) & 0xff );
   texel[BCOMP] = nonlinear_to_linear( (s      ) & 0xff );
   texel[ACOMP] = UBYTE_TO_FLOAT( (s >> 24) ); /* linear! */
}



/* Fetch texel from 1D, 2D or 3D sl8 texture, return 4 GLfloats */
static void FETCH(sl8)(const struct swrast_texture_image *texImage,
                       GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   texel[RCOMP] = 
   texel[GCOMP] = 
   texel[BCOMP] = nonlinear_to_linear(src[0]);
   texel[ACOMP] = 1.0F;
}



/* Fetch texel from 1D, 2D or 3D sla8 texture, return 4 GLfloats */
static void FETCH(sla8)(const struct swrast_texture_image *texImage,
                       GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 2);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = nonlinear_to_linear(src[0]);
   texel[ACOMP] = UBYTE_TO_FLOAT(src[1]); /* linear */
}




/* MESA_FORMAT_RGBA_INT8 **************************************************/

static void
FETCH(rgba_int8)(const struct swrast_texture_image *texImage,
                 GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLbyte *src = TEXEL_ADDR(GLbyte, texImage, i, j, k, 4);
   texel[RCOMP] = (GLfloat) src[0];
   texel[GCOMP] = (GLfloat) src[1];
   texel[BCOMP] = (GLfloat) src[2];
   texel[ACOMP] = (GLfloat) src[3];
}




/* MESA_FORMAT_RGBA_INT16 **************************************************/

static void
FETCH(rgba_int16)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLshort *src = TEXEL_ADDR(GLshort, texImage, i, j, k, 4);
   texel[RCOMP] = (GLfloat) src[0];
   texel[GCOMP] = (GLfloat) src[1];
   texel[BCOMP] = (GLfloat) src[2];
   texel[ACOMP] = (GLfloat) src[3];
}




/* MESA_FORMAT_RGBA_INT32 **************************************************/

static void
FETCH(rgba_int32)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLint *src = TEXEL_ADDR(GLint, texImage, i, j, k, 4);
   texel[RCOMP] = (GLfloat) src[0];
   texel[GCOMP] = (GLfloat) src[1];
   texel[BCOMP] = (GLfloat) src[2];
   texel[ACOMP] = (GLfloat) src[3];
}




/* MESA_FORMAT_RGBA_UINT8 **************************************************/

static void
FETCH(rgba_uint8)(const struct swrast_texture_image *texImage,
                 GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 4);
   texel[RCOMP] = (GLfloat) src[0];
   texel[GCOMP] = (GLfloat) src[1];
   texel[BCOMP] = (GLfloat) src[2];
   texel[ACOMP] = (GLfloat) src[3];
}




/* MESA_FORMAT_RGBA_UINT16 **************************************************/

static void
FETCH(rgba_uint16)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 4);
   texel[RCOMP] = (GLfloat) src[0];
   texel[GCOMP] = (GLfloat) src[1];
   texel[BCOMP] = (GLfloat) src[2];
   texel[ACOMP] = (GLfloat) src[3];
}




/* MESA_FORMAT_RGBA_UINT32 **************************************************/

static void
FETCH(rgba_uint32)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLuint *src = TEXEL_ADDR(GLuint, texImage, i, j, k, 4);
   texel[RCOMP] = (GLfloat) src[0];
   texel[GCOMP] = (GLfloat) src[1];
   texel[BCOMP] = (GLfloat) src[2];
   texel[ACOMP] = (GLfloat) src[3];
}




/* MESA_FORMAT_DUDV8 ********************************************************/

/* this format by definition produces 0,0,0,1 as rgba values,
   however we'll return the dudv values as rg and fix up elsewhere */
static void FETCH(dudv8)(const struct swrast_texture_image *texImage,
                         GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLbyte *src = TEXEL_ADDR(GLbyte, texImage, i, j, k, 2);
   texel[RCOMP] = BYTE_TO_FLOAT(src[0]);
   texel[GCOMP] = BYTE_TO_FLOAT(src[1]);
   texel[BCOMP] = 0;
   texel[ACOMP] = 0;
}


/* MESA_FORMAT_SIGNED_R8 ***********************************************/

static void FETCH(signed_r8)( const struct swrast_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLbyte s = *TEXEL_ADDR(GLbyte, texImage, i, j, k, 1);
   texel[RCOMP] = BYTE_TO_FLOAT_TEX( s );
   texel[GCOMP] = 0.0F;
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = 1.0F;
}




/* MESA_FORMAT_SIGNED_A8 ***********************************************/

static void FETCH(signed_a8)( const struct swrast_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLbyte s = *TEXEL_ADDR(GLbyte, texImage, i, j, k, 1);
   texel[RCOMP] = 0.0F;
   texel[GCOMP] = 0.0F;
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = BYTE_TO_FLOAT_TEX( s );
}




/* MESA_FORMAT_SIGNED_L8 ***********************************************/

static void FETCH(signed_l8)( const struct swrast_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLbyte s = *TEXEL_ADDR(GLbyte, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = BYTE_TO_FLOAT_TEX( s );
   texel[ACOMP] = 1.0F;
}




/* MESA_FORMAT_SIGNED_I8 ***********************************************/

static void FETCH(signed_i8)( const struct swrast_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLbyte s = *TEXEL_ADDR(GLbyte, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] =
   texel[ACOMP] = BYTE_TO_FLOAT_TEX( s );
}




/* MESA_FORMAT_SIGNED_RG88_REV ***********************************************/

static void FETCH(signed_rg88_rev)( const struct swrast_texture_image *texImage,
                                    GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort s = *TEXEL_ADDR(GLshort, texImage, i, j, k, 1);
   texel[RCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s & 0xff) );
   texel[GCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s >> 8) );
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = 1.0F;
}




/* MESA_FORMAT_SIGNED_AL88 ***********************************************/

static void FETCH(signed_al88)( const struct swrast_texture_image *texImage,
                                GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort s = *TEXEL_ADDR(GLshort, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s & 0xff) );
   texel[ACOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s >> 8) );
}




/* MESA_FORMAT_SIGNED_RGBX8888 ***********************************************/

static void FETCH(signed_rgbx8888)( const struct swrast_texture_image *texImage,
			            GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s >> 24) );
   texel[GCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s >> 16) );
   texel[BCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s >>  8) );
   texel[ACOMP] = 1.0f;
}




/* MESA_FORMAT_SIGNED_RGBA8888 ***********************************************/

static void FETCH(signed_rgba8888)( const struct swrast_texture_image *texImage,
			            GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s >> 24) );
   texel[GCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s >> 16) );
   texel[BCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s >>  8) );
   texel[ACOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s      ) );
}



static void FETCH(signed_rgba8888_rev)( const struct swrast_texture_image *texImage,
                                        GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s      ) );
   texel[GCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s >>  8) );
   texel[BCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s >> 16) );
   texel[ACOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s >> 24) );
}





/* MESA_FORMAT_SIGNED_R16 ***********************************************/

static void
FETCH(signed_r16)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLshort s = *TEXEL_ADDR(GLshort, texImage, i, j, k, 1);
   texel[RCOMP] = SHORT_TO_FLOAT_TEX( s );
   texel[GCOMP] = 0.0F;
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = 1.0F;
}




/* MESA_FORMAT_SIGNED_A16 ***********************************************/

static void
FETCH(signed_a16)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLshort s = *TEXEL_ADDR(GLshort, texImage, i, j, k, 1);
   texel[RCOMP] = 0.0F;
   texel[GCOMP] = 0.0F;
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = SHORT_TO_FLOAT_TEX( s );
}




/* MESA_FORMAT_SIGNED_L16 ***********************************************/

static void
FETCH(signed_l16)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLshort s = *TEXEL_ADDR(GLshort, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = SHORT_TO_FLOAT_TEX( s );
   texel[ACOMP] = 1.0F;
}




/* MESA_FORMAT_SIGNED_I16 ***********************************************/

static void
FETCH(signed_i16)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLshort s = *TEXEL_ADDR(GLshort, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] =
   texel[ACOMP] = SHORT_TO_FLOAT_TEX( s );
}




/* MESA_FORMAT_SIGNED_RG1616 ***********************************************/

static void
FETCH(signed_rg1616)(const struct swrast_texture_image *texImage,
                    GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLshort *s = TEXEL_ADDR(GLshort, texImage, i, j, k, 2);
   texel[RCOMP] = SHORT_TO_FLOAT_TEX( s[0] );
   texel[GCOMP] = SHORT_TO_FLOAT_TEX( s[1] );
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = 1.0F;
}




/* MESA_FORMAT_SIGNED_AL1616 ***********************************************/

static void
FETCH(signed_al1616)(const struct swrast_texture_image *texImage,
                    GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLshort *s = TEXEL_ADDR(GLshort, texImage, i, j, k, 2);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = SHORT_TO_FLOAT_TEX( s[0] );
   texel[ACOMP] = SHORT_TO_FLOAT_TEX( s[1] );
}




/* MESA_FORMAT_SIGNED_RGB_16 ***********************************************/

static void 
FETCH(signed_rgb_16)(const struct swrast_texture_image *texImage,
                     GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLshort *s = TEXEL_ADDR(GLshort, texImage, i, j, k, 3);
   texel[RCOMP] = SHORT_TO_FLOAT_TEX( s[0] );
   texel[GCOMP] = SHORT_TO_FLOAT_TEX( s[1] );
   texel[BCOMP] = SHORT_TO_FLOAT_TEX( s[2] );
   texel[ACOMP] = 1.0F;
}




/* MESA_FORMAT_SIGNED_RGBA_16 ***********************************************/

static void
FETCH(signed_rgba_16)(const struct swrast_texture_image *texImage,
                      GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLshort *s = TEXEL_ADDR(GLshort, texImage, i, j, k, 4);
   texel[RCOMP] = SHORT_TO_FLOAT_TEX( s[0] );
   texel[GCOMP] = SHORT_TO_FLOAT_TEX( s[1] );
   texel[BCOMP] = SHORT_TO_FLOAT_TEX( s[2] );
   texel[ACOMP] = SHORT_TO_FLOAT_TEX( s[3] );
}





/* MESA_FORMAT_RGBA_16 ***********************************************/

static void
FETCH(rgba_16)(const struct swrast_texture_image *texImage,
               GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort *s = TEXEL_ADDR(GLushort, texImage, i, j, k, 4);
   texel[RCOMP] = USHORT_TO_FLOAT( s[0] );
   texel[GCOMP] = USHORT_TO_FLOAT( s[1] );
   texel[BCOMP] = USHORT_TO_FLOAT( s[2] );
   texel[ACOMP] = USHORT_TO_FLOAT( s[3] );
}





/* MESA_FORMAT_YCBCR *********************************************************/

/* Fetch texel from 1D, 2D or 3D ycbcr texture, return 4 GLfloats.
 * We convert YCbCr to RGB here.
 */
static void FETCH(f_ycbcr)( const struct swrast_texture_image *texImage,
                            GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort *src0 = TEXEL_ADDR(GLushort, texImage, (i & ~1), j, k, 1); /* even */
   const GLushort *src1 = src0 + 1;                               /* odd */
   const GLubyte y0 = (*src0 >> 8) & 0xff;  /* luminance */
   const GLubyte cb = *src0 & 0xff;         /* chroma U */
   const GLubyte y1 = (*src1 >> 8) & 0xff;  /* luminance */
   const GLubyte cr = *src1 & 0xff;         /* chroma V */
   const GLubyte y = (i & 1) ? y1 : y0;     /* choose even/odd luminance */
   GLfloat r = 1.164F * (y - 16) + 1.596F * (cr - 128);
   GLfloat g = 1.164F * (y - 16) - 0.813F * (cr - 128) - 0.391F * (cb - 128);
   GLfloat b = 1.164F * (y - 16) + 2.018F * (cb - 128);
   r *= (1.0F / 255.0F);
   g *= (1.0F / 255.0F);
   b *= (1.0F / 255.0F);
   texel[RCOMP] = CLAMP(r, 0.0F, 1.0F);
   texel[GCOMP] = CLAMP(g, 0.0F, 1.0F);
   texel[BCOMP] = CLAMP(b, 0.0F, 1.0F);
   texel[ACOMP] = 1.0F;
}




/* MESA_FORMAT_YCBCR_REV *****************************************************/

/* Fetch texel from 1D, 2D or 3D ycbcr_rev texture, return 4 GLfloats.
 * We convert YCbCr to RGB here.
 */
static void FETCH(f_ycbcr_rev)( const struct swrast_texture_image *texImage,
                                GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort *src0 = TEXEL_ADDR(GLushort, texImage, (i & ~1), j, k, 1); /* even */
   const GLushort *src1 = src0 + 1;                               /* odd */
   const GLubyte y0 = *src0 & 0xff;         /* luminance */
   const GLubyte cr = (*src0 >> 8) & 0xff;  /* chroma V */
   const GLubyte y1 = *src1 & 0xff;         /* luminance */
   const GLubyte cb = (*src1 >> 8) & 0xff;  /* chroma U */
   const GLubyte y = (i & 1) ? y1 : y0;     /* choose even/odd luminance */
   GLfloat r = 1.164F * (y - 16) + 1.596F * (cr - 128);
   GLfloat g = 1.164F * (y - 16) - 0.813F * (cr - 128) - 0.391F * (cb - 128);
   GLfloat b = 1.164F * (y - 16) + 2.018F * (cb - 128);
   r *= (1.0F / 255.0F);
   g *= (1.0F / 255.0F);
   b *= (1.0F / 255.0F);
   texel[RCOMP] = CLAMP(r, 0.0F, 1.0F);
   texel[GCOMP] = CLAMP(g, 0.0F, 1.0F);
   texel[BCOMP] = CLAMP(b, 0.0F, 1.0F);
   texel[ACOMP] = 1.0F;
}




/* MESA_TEXFORMAT_Z24_S8 ***************************************************/

static void FETCH(f_z24_s8)( const struct swrast_texture_image *texImage,
                             GLint i, GLint j, GLint k, GLfloat *texel )
{
   /* only return Z, not stencil data */
   const GLuint *src = TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   const GLdouble scale = 1.0 / (GLdouble) 0xffffff;
   texel[0] = ((*src) >> 8) * scale;
   ASSERT(texImage->Base.TexFormat == MESA_FORMAT_Z24_S8 ||
	  texImage->Base.TexFormat == MESA_FORMAT_Z24_X8);
   ASSERT(texel[0] >= 0.0F);
   ASSERT(texel[0] <= 1.0F);
}




/* MESA_TEXFORMAT_S8_Z24 ***************************************************/

static void FETCH(f_s8_z24)( const struct swrast_texture_image *texImage,
                             GLint i, GLint j, GLint k, GLfloat *texel )
{
   /* only return Z, not stencil data */
   const GLuint *src = TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   const GLdouble scale = 1.0 / (GLdouble) 0xffffff;
   texel[0] = ((*src) & 0x00ffffff) * scale;
   ASSERT(texImage->Base.TexFormat == MESA_FORMAT_S8_Z24 ||
	  texImage->Base.TexFormat == MESA_FORMAT_X8_Z24);
   ASSERT(texel[0] >= 0.0F);
   ASSERT(texel[0] <= 1.0F);
}




/* MESA_FORMAT_RGB9_E5 ******************************************************/

static void FETCH(rgb9_e5)( const struct swrast_texture_image *texImage,
                            GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLuint *src = TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   rgb9e5_to_float3(*src, texel);
   texel[ACOMP] = 1.0F;
}




/* MESA_FORMAT_R11_G11_B10_FLOAT *********************************************/

static void FETCH(r11_g11_b10f)( const struct swrast_texture_image *texImage,
                                 GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLuint *src = TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   r11g11b10f_to_float3(*src, texel);
   texel[ACOMP] = 1.0F;
}




/* MESA_FORMAT_Z32_FLOAT_X24S8 ***********************************************/

static void FETCH(z32f_x24s8)(const struct swrast_texture_image *texImage,
			      GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 2);
   texel[RCOMP] = src[0];
   texel[GCOMP] = 0.0F;
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = 1.0F;
}



#undef TEXEL_ADDR
#undef DIM
#undef FETCH
