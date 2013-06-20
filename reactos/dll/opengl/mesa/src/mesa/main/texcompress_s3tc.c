/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 * Copyright (c) 2008 VMware, Inc.
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
 * \file texcompress_s3tc.c
 * GL_EXT_texture_compression_s3tc support.
 */

#ifndef USE_EXTERNAL_DXTN_LIB
#define USE_EXTERNAL_DXTN_LIB 1
#endif

#include "glheader.h"
#include "imports.h"
#include "colormac.h"
#include "dlopen.h"
#include "image.h"
#include "macros.h"
#include "mfeatures.h"
#include "mtypes.h"
#include "texcompress.h"
#include "texcompress_s3tc.h"
#include "texstore.h"
#include "swrast/s_context.h"


#if FEATURE_texture_s3tc


#if defined(_WIN32) || defined(WIN32)
#define DXTN_LIBNAME "dxtn.dll"
#define RTLD_LAZY 0
#define RTLD_GLOBAL 0
#elif defined(__DJGPP__)
#define DXTN_LIBNAME "dxtn.dxe"
#else
#define DXTN_LIBNAME "libtxc_dxtn.so"
#endif

#if FEATURE_EXT_texture_sRGB
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
#endif /* FEATURE_EXT_texture_sRGB */

typedef void (*dxtFetchTexelFuncExt)( GLint srcRowstride, GLubyte *pixdata, GLint col, GLint row, GLvoid *texelOut );

dxtFetchTexelFuncExt fetch_ext_rgb_dxt1 = NULL;
dxtFetchTexelFuncExt fetch_ext_rgba_dxt1 = NULL;
dxtFetchTexelFuncExt fetch_ext_rgba_dxt3 = NULL;
dxtFetchTexelFuncExt fetch_ext_rgba_dxt5 = NULL;

typedef void (*dxtCompressTexFuncExt)(GLint srccomps, GLint width,
                                      GLint height, const GLubyte *srcPixData,
                                      GLenum destformat, GLubyte *dest,
                                      GLint dstRowStride);

static dxtCompressTexFuncExt ext_tx_compress_dxtn = NULL;

static void *dxtlibhandle = NULL;


void
_mesa_init_texture_s3tc( struct gl_context *ctx )
{
   /* called during context initialization */
   ctx->Mesa_DXTn = GL_FALSE;
#if USE_EXTERNAL_DXTN_LIB
   if (!dxtlibhandle) {
      dxtlibhandle = _mesa_dlopen(DXTN_LIBNAME, 0);
      if (!dxtlibhandle) {
	 _mesa_warning(ctx, "couldn't open " DXTN_LIBNAME ", software DXTn "
	    "compression/decompression unavailable");
      }
      else {
         /* the fetch functions are not per context! Might be problematic... */
         fetch_ext_rgb_dxt1 = (dxtFetchTexelFuncExt)
            _mesa_dlsym(dxtlibhandle, "fetch_2d_texel_rgb_dxt1");
         fetch_ext_rgba_dxt1 = (dxtFetchTexelFuncExt)
            _mesa_dlsym(dxtlibhandle, "fetch_2d_texel_rgba_dxt1");
         fetch_ext_rgba_dxt3 = (dxtFetchTexelFuncExt)
            _mesa_dlsym(dxtlibhandle, "fetch_2d_texel_rgba_dxt3");
         fetch_ext_rgba_dxt5 = (dxtFetchTexelFuncExt)
            _mesa_dlsym(dxtlibhandle, "fetch_2d_texel_rgba_dxt5");
         ext_tx_compress_dxtn = (dxtCompressTexFuncExt)
            _mesa_dlsym(dxtlibhandle, "tx_compress_dxtn");

         if (!fetch_ext_rgb_dxt1 ||
             !fetch_ext_rgba_dxt1 ||
             !fetch_ext_rgba_dxt3 ||
             !fetch_ext_rgba_dxt5 ||
             !ext_tx_compress_dxtn) {
	    _mesa_warning(ctx, "couldn't reference all symbols in "
	       DXTN_LIBNAME ", software DXTn compression/decompression "
	       "unavailable");
            fetch_ext_rgb_dxt1 = NULL;
            fetch_ext_rgba_dxt1 = NULL;
            fetch_ext_rgba_dxt3 = NULL;
            fetch_ext_rgba_dxt5 = NULL;
            ext_tx_compress_dxtn = NULL;
            _mesa_dlclose(dxtlibhandle);
            dxtlibhandle = NULL;
         }
      }
   }
   if (dxtlibhandle) {
      ctx->Mesa_DXTn = GL_TRUE;
   }
#else
   (void) ctx;
#endif
}

/**
 * Store user's image in rgb_dxt1 format.
 */
GLboolean
_mesa_texstore_rgb_dxt1(TEXSTORE_PARAMS)
{
   const GLubyte *pixels;
   GLubyte *dst;
   const GLubyte *tempImage = NULL;

   ASSERT(dstFormat == MESA_FORMAT_RGB_DXT1 ||
          dstFormat == MESA_FORMAT_SRGB_DXT1);

   if (srcFormat != GL_RGB ||
       srcType != GL_UNSIGNED_BYTE ||
       ctx->_ImageTransferState ||
       srcPacking->RowLength != srcWidth ||
       srcPacking->SwapBytes) {
      /* convert image to RGB/GLubyte */
      tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
                                             baseInternalFormat,
                                             _mesa_get_format_base_format(dstFormat),
                                             srcWidth, srcHeight, srcDepth,
                                             srcFormat, srcType, srcAddr,
                                             srcPacking);
      if (!tempImage)
         return GL_FALSE; /* out of memory */
      pixels = tempImage;
      srcFormat = GL_RGB;
   }
   else {
      pixels = _mesa_image_address2d(srcPacking, srcAddr, srcWidth, srcHeight,
                                     srcFormat, srcType, 0, 0);
   }

   dst = dstSlices[0];

   if (ext_tx_compress_dxtn) {
      (*ext_tx_compress_dxtn)(3, srcWidth, srcHeight, pixels,
                              GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
                              dst, dstRowStride);
   }
   else {
      _mesa_warning(ctx, "external dxt library not available: texstore_rgb_dxt1");
   }

   if (tempImage)
      free((void *) tempImage);

   return GL_TRUE;
}


/**
 * Store user's image in rgba_dxt1 format.
 */
GLboolean
_mesa_texstore_rgba_dxt1(TEXSTORE_PARAMS)
{
   const GLubyte *pixels;
   GLubyte *dst;
   const GLubyte *tempImage = NULL;

   ASSERT(dstFormat == MESA_FORMAT_RGBA_DXT1 ||
          dstFormat == MESA_FORMAT_SRGBA_DXT1);

   if (srcFormat != GL_RGBA ||
       srcType != GL_UNSIGNED_BYTE ||
       ctx->_ImageTransferState ||
       srcPacking->RowLength != srcWidth ||
       srcPacking->SwapBytes) {
      /* convert image to RGBA/GLubyte */
      tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
                                             baseInternalFormat,
                                             _mesa_get_format_base_format(dstFormat),
                                             srcWidth, srcHeight, srcDepth,
                                             srcFormat, srcType, srcAddr,
                                             srcPacking);
      if (!tempImage)
         return GL_FALSE; /* out of memory */
      pixels = tempImage;
      srcFormat = GL_RGBA;
   }
   else {
      pixels = _mesa_image_address2d(srcPacking, srcAddr, srcWidth, srcHeight,
                                     srcFormat, srcType, 0, 0);
   }

   dst = dstSlices[0];

   if (ext_tx_compress_dxtn) {
      (*ext_tx_compress_dxtn)(4, srcWidth, srcHeight, pixels,
                              GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
                              dst, dstRowStride);
   }
   else {
      _mesa_warning(ctx, "external dxt library not available: texstore_rgba_dxt1");
   }

   if (tempImage)
      free((void*) tempImage);

   return GL_TRUE;
}


/**
 * Store user's image in rgba_dxt3 format.
 */
GLboolean
_mesa_texstore_rgba_dxt3(TEXSTORE_PARAMS)
{
   const GLubyte *pixels;
   GLubyte *dst;
   const GLubyte *tempImage = NULL;

   ASSERT(dstFormat == MESA_FORMAT_RGBA_DXT3 ||
          dstFormat == MESA_FORMAT_SRGBA_DXT3);

   if (srcFormat != GL_RGBA ||
       srcType != GL_UNSIGNED_BYTE ||
       ctx->_ImageTransferState ||
       srcPacking->RowLength != srcWidth ||
       srcPacking->SwapBytes) {
      /* convert image to RGBA/GLubyte */
      tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
                                             baseInternalFormat,
                                             _mesa_get_format_base_format(dstFormat),
                                             srcWidth, srcHeight, srcDepth,
                                             srcFormat, srcType, srcAddr,
                                             srcPacking);
      if (!tempImage)
         return GL_FALSE; /* out of memory */
      pixels = tempImage;
   }
   else {
      pixels = _mesa_image_address2d(srcPacking, srcAddr, srcWidth, srcHeight,
                                     srcFormat, srcType, 0, 0);
   }

   dst = dstSlices[0];

   if (ext_tx_compress_dxtn) {
      (*ext_tx_compress_dxtn)(4, srcWidth, srcHeight, pixels,
                              GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
                              dst, dstRowStride);
   }
   else {
      _mesa_warning(ctx, "external dxt library not available: texstore_rgba_dxt3");
   }

   if (tempImage)
      free((void *) tempImage);

   return GL_TRUE;
}


/**
 * Store user's image in rgba_dxt5 format.
 */
GLboolean
_mesa_texstore_rgba_dxt5(TEXSTORE_PARAMS)
{
   const GLubyte *pixels;
   GLubyte *dst;
   const GLubyte *tempImage = NULL;

   ASSERT(dstFormat == MESA_FORMAT_RGBA_DXT5 ||
          dstFormat == MESA_FORMAT_SRGBA_DXT5);

   if (srcFormat != GL_RGBA ||
       srcType != GL_UNSIGNED_BYTE ||
       ctx->_ImageTransferState ||
       srcPacking->RowLength != srcWidth ||
       srcPacking->SwapBytes) {
      /* convert image to RGBA/GLubyte */
      tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
                                             baseInternalFormat,
                                   	     _mesa_get_format_base_format(dstFormat),
                                             srcWidth, srcHeight, srcDepth,
                                             srcFormat, srcType, srcAddr,
                                             srcPacking);
      if (!tempImage)
         return GL_FALSE; /* out of memory */
      pixels = tempImage;
   }
   else {
      pixels = _mesa_image_address2d(srcPacking, srcAddr, srcWidth, srcHeight,
                                     srcFormat, srcType, 0, 0);
   }

   dst = dstSlices[0];

   if (ext_tx_compress_dxtn) {
      (*ext_tx_compress_dxtn)(4, srcWidth, srcHeight, pixels,
                              GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
                              dst, dstRowStride);
   }
   else {
      _mesa_warning(ctx, "external dxt library not available: texstore_rgba_dxt5");
   }

   if (tempImage)
      free((void *) tempImage);

   return GL_TRUE;
}


static void
fetch_texel_2d_rgb_dxt1( const struct swrast_texture_image *texImage,
                         GLint i, GLint j, GLint k, GLubyte *texel )
{
   (void) k;
   if (fetch_ext_rgb_dxt1) {
      fetch_ext_rgb_dxt1(texImage->RowStride,
                         texImage->Map, i, j, texel);
   }
   else
      _mesa_debug(NULL, "attempted to decode s3tc texture without library available: fetch_texel_2d_rgb_dxt1");
}


void
_mesa_fetch_texel_2d_f_rgb_dxt1(const struct swrast_texture_image *texImage,
                                GLint i, GLint j, GLint k, GLfloat *texel)
{
   /* just sample as GLubyte and convert to float here */
   GLubyte rgba[4];
   fetch_texel_2d_rgb_dxt1(texImage, i, j, k, rgba);
   texel[RCOMP] = UBYTE_TO_FLOAT(rgba[RCOMP]);
   texel[GCOMP] = UBYTE_TO_FLOAT(rgba[GCOMP]);
   texel[BCOMP] = UBYTE_TO_FLOAT(rgba[BCOMP]);
   texel[ACOMP] = UBYTE_TO_FLOAT(rgba[ACOMP]);
}


static void
fetch_texel_2d_rgba_dxt1( const struct swrast_texture_image *texImage,
                          GLint i, GLint j, GLint k, GLubyte *texel )
{
   (void) k;
   if (fetch_ext_rgba_dxt1) {
      fetch_ext_rgba_dxt1(texImage->RowStride,
                          texImage->Map, i, j, texel);
   }
   else
      _mesa_debug(NULL, "attempted to decode s3tc texture without library available: fetch_texel_2d_rgba_dxt1\n");
}


void
_mesa_fetch_texel_2d_f_rgba_dxt1(const struct swrast_texture_image *texImage,
                                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   /* just sample as GLubyte and convert to float here */
   GLubyte rgba[4];
   fetch_texel_2d_rgba_dxt1(texImage, i, j, k, rgba);
   texel[RCOMP] = UBYTE_TO_FLOAT(rgba[RCOMP]);
   texel[GCOMP] = UBYTE_TO_FLOAT(rgba[GCOMP]);
   texel[BCOMP] = UBYTE_TO_FLOAT(rgba[BCOMP]);
   texel[ACOMP] = UBYTE_TO_FLOAT(rgba[ACOMP]);
}


static void
fetch_texel_2d_rgba_dxt3( const struct swrast_texture_image *texImage,
                          GLint i, GLint j, GLint k, GLubyte *texel )
{
   (void) k;
   if (fetch_ext_rgba_dxt3) {
      fetch_ext_rgba_dxt3(texImage->RowStride,
                          texImage->Map, i, j, texel);
   }
   else
      _mesa_debug(NULL, "attempted to decode s3tc texture without library available: fetch_texel_2d_rgba_dxt3\n");
}


void
_mesa_fetch_texel_2d_f_rgba_dxt3(const struct swrast_texture_image *texImage,
                                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   /* just sample as GLubyte and convert to float here */
   GLubyte rgba[4];
   fetch_texel_2d_rgba_dxt3(texImage, i, j, k, rgba);
   texel[RCOMP] = UBYTE_TO_FLOAT(rgba[RCOMP]);
   texel[GCOMP] = UBYTE_TO_FLOAT(rgba[GCOMP]);
   texel[BCOMP] = UBYTE_TO_FLOAT(rgba[BCOMP]);
   texel[ACOMP] = UBYTE_TO_FLOAT(rgba[ACOMP]);
}


static void
fetch_texel_2d_rgba_dxt5( const struct swrast_texture_image *texImage,
                          GLint i, GLint j, GLint k, GLubyte *texel )
{
   (void) k;
   if (fetch_ext_rgba_dxt5) {
      fetch_ext_rgba_dxt5(texImage->RowStride,
                          texImage->Map, i, j, texel);
   }
   else
      _mesa_debug(NULL, "attempted to decode s3tc texture without library available: fetch_texel_2d_rgba_dxt5\n");
}


void
_mesa_fetch_texel_2d_f_rgba_dxt5(const struct swrast_texture_image *texImage,
                                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   /* just sample as GLubyte and convert to float here */
   GLubyte rgba[4];
   fetch_texel_2d_rgba_dxt5(texImage, i, j, k, rgba);
   texel[RCOMP] = UBYTE_TO_FLOAT(rgba[RCOMP]);
   texel[GCOMP] = UBYTE_TO_FLOAT(rgba[GCOMP]);
   texel[BCOMP] = UBYTE_TO_FLOAT(rgba[BCOMP]);
   texel[ACOMP] = UBYTE_TO_FLOAT(rgba[ACOMP]);
}

#if FEATURE_EXT_texture_sRGB
void
_mesa_fetch_texel_2d_f_srgb_dxt1( const struct swrast_texture_image *texImage,
                                  GLint i, GLint j, GLint k, GLfloat *texel )
{
   /* just sample as GLubyte and convert to float here */
   GLubyte rgba[4];
   fetch_texel_2d_rgb_dxt1(texImage, i, j, k, rgba);
   texel[RCOMP] = nonlinear_to_linear(rgba[RCOMP]);
   texel[GCOMP] = nonlinear_to_linear(rgba[GCOMP]);
   texel[BCOMP] = nonlinear_to_linear(rgba[BCOMP]);
   texel[ACOMP] = UBYTE_TO_FLOAT(rgba[ACOMP]);
}

void
_mesa_fetch_texel_2d_f_srgba_dxt1(const struct swrast_texture_image *texImage,
                                  GLint i, GLint j, GLint k, GLfloat *texel)
{
   /* just sample as GLubyte and convert to float here */
   GLubyte rgba[4];
   fetch_texel_2d_rgba_dxt1(texImage, i, j, k, rgba);
   texel[RCOMP] = nonlinear_to_linear(rgba[RCOMP]);
   texel[GCOMP] = nonlinear_to_linear(rgba[GCOMP]);
   texel[BCOMP] = nonlinear_to_linear(rgba[BCOMP]);
   texel[ACOMP] = UBYTE_TO_FLOAT(rgba[ACOMP]);
}

void
_mesa_fetch_texel_2d_f_srgba_dxt3(const struct swrast_texture_image *texImage,
                                  GLint i, GLint j, GLint k, GLfloat *texel)
{
   /* just sample as GLubyte and convert to float here */
   GLubyte rgba[4];
   fetch_texel_2d_rgba_dxt3(texImage, i, j, k, rgba);
   texel[RCOMP] = nonlinear_to_linear(rgba[RCOMP]);
   texel[GCOMP] = nonlinear_to_linear(rgba[GCOMP]);
   texel[BCOMP] = nonlinear_to_linear(rgba[BCOMP]);
   texel[ACOMP] = UBYTE_TO_FLOAT(rgba[ACOMP]);
}

void
_mesa_fetch_texel_2d_f_srgba_dxt5(const struct swrast_texture_image *texImage,
                                  GLint i, GLint j, GLint k, GLfloat *texel)
{
   /* just sample as GLubyte and convert to float here */
   GLubyte rgba[4];
   fetch_texel_2d_rgba_dxt5(texImage, i, j, k, rgba);
   texel[RCOMP] = nonlinear_to_linear(rgba[RCOMP]);
   texel[GCOMP] = nonlinear_to_linear(rgba[GCOMP]);
   texel[BCOMP] = nonlinear_to_linear(rgba[BCOMP]);
   texel[ACOMP] = UBYTE_TO_FLOAT(rgba[ACOMP]);
}
#endif /* FEATURE_EXT_texture_sRGB */


#endif /* FEATURE_texture_s3tc */
