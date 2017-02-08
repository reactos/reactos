/*
 * Copyright (C) 2011 Red Hat Inc.
 * 
 * block compression parts are:
 * Copyright (C) 2004  Roland Scheidegger   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author:
 *    Dave Airlie
 */

/**
 * \file texcompress_rgtc.c
 * GL_EXT_texture_compression_rgtc support.
 */


#include "glheader.h"
#include "imports.h"
#include "colormac.h"
#include "image.h"
#include "macros.h"
#include "mfeatures.h"
#include "mipmap.h"
#include "texcompress.h"
#include "texcompress_rgtc.h"
#include "texstore.h"
#include "swrast/s_context.h"


#define RGTC_DEBUG 0

static void unsigned_encode_rgtc_ubyte(GLubyte *blkaddr, GLubyte srccolors[4][4],
					GLint numxpixels, GLint numypixels);
static void signed_encode_rgtc_ubyte(GLbyte *blkaddr, GLbyte srccolors[4][4],
			     GLint numxpixels, GLint numypixels);

static void unsigned_fetch_texel_rgtc(unsigned srcRowStride, const GLubyte *pixdata,
				      unsigned i, unsigned j, GLubyte *value, unsigned comps);

static void signed_fetch_texel_rgtc(unsigned srcRowStride, const GLbyte *pixdata,
				      unsigned i, unsigned j, GLbyte *value, unsigned comps);

static void extractsrc_u( GLubyte srcpixels[4][4], const GLubyte *srcaddr,
			  GLint srcRowStride, GLint numxpixels, GLint numypixels, GLint comps)
{
   GLubyte i, j;
   const GLubyte *curaddr;
   for (j = 0; j < numypixels; j++) {
      curaddr = srcaddr + j * srcRowStride * comps;
      for (i = 0; i < numxpixels; i++) {
	 srcpixels[j][i] = *curaddr;
	 curaddr += comps;
      }
   }
}

static void extractsrc_s( GLbyte srcpixels[4][4], const GLfloat *srcaddr,
			  GLint srcRowStride, GLint numxpixels, GLint numypixels, GLint comps)
{
   GLubyte i, j;
   const GLfloat *curaddr;
   for (j = 0; j < numypixels; j++) {
      curaddr = srcaddr + j * srcRowStride * comps;
      for (i = 0; i < numxpixels; i++) {
	 srcpixels[j][i] = FLOAT_TO_BYTE_TEX(*curaddr);
	 curaddr += comps;
      }
   }
}


GLboolean
_mesa_texstore_red_rgtc1(TEXSTORE_PARAMS)
{
   GLubyte *dst;
   const GLubyte *tempImage = NULL;
   int i, j;
   int numxpixels, numypixels;
   const GLubyte *srcaddr;
   GLubyte srcpixels[4][4];
   GLubyte *blkaddr;
   GLint dstRowDiff;
   ASSERT(dstFormat == MESA_FORMAT_RED_RGTC1 ||
          dstFormat == MESA_FORMAT_L_LATC1);

   tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
					  baseInternalFormat,
					  _mesa_get_format_base_format(dstFormat),
					  srcWidth, srcHeight, srcDepth,
					  srcFormat, srcType, srcAddr,
					  srcPacking);
   if (!tempImage)
      return GL_FALSE; /* out of memory */

   dst = dstSlices[0];

   blkaddr = dst;
   dstRowDiff = dstRowStride >= (srcWidth * 2) ? dstRowStride - (((srcWidth + 3) & ~3) * 2) : 0;
   for (j = 0; j < srcHeight; j+=4) {
      if (srcHeight > j + 3) numypixels = 4;
      else numypixels = srcHeight - j;
      srcaddr = tempImage + j * srcWidth;
      for (i = 0; i < srcWidth; i += 4) {
	 if (srcWidth > i + 3) numxpixels = 4;
	 else numxpixels = srcWidth - i;
	 extractsrc_u(srcpixels, srcaddr, srcWidth, numxpixels, numypixels, 1);
	 unsigned_encode_rgtc_ubyte(blkaddr, srcpixels, numxpixels, numypixels);
	 srcaddr += numxpixels;
	 blkaddr += 8;
      }
      blkaddr += dstRowDiff;
   }
   if (tempImage)
      free((void *) tempImage);

   return GL_TRUE;
}

GLboolean
_mesa_texstore_signed_red_rgtc1(TEXSTORE_PARAMS)
{
   GLbyte *dst;
   const GLfloat *tempImage = NULL;
   int i, j;
   int numxpixels, numypixels;
   const GLfloat *srcaddr;
   GLbyte srcpixels[4][4];
   GLbyte *blkaddr;
   GLint dstRowDiff;
   ASSERT(dstFormat == MESA_FORMAT_SIGNED_RED_RGTC1 ||
          dstFormat == MESA_FORMAT_SIGNED_L_LATC1);

   tempImage = _mesa_make_temp_float_image(ctx, dims,
					   baseInternalFormat,
					   _mesa_get_format_base_format(dstFormat),
					   srcWidth, srcHeight, srcDepth,
					   srcFormat, srcType, srcAddr,
					   srcPacking, 0x0);
   if (!tempImage)
      return GL_FALSE; /* out of memory */

   dst = (GLbyte *) dstSlices[0];

   blkaddr = dst;
   dstRowDiff = dstRowStride >= (srcWidth * 2) ? dstRowStride - (((srcWidth + 3) & ~3) * 2) : 0;
   for (j = 0; j < srcHeight; j+=4) {
      if (srcHeight > j + 3) numypixels = 4;
      else numypixels = srcHeight - j;
      srcaddr = tempImage + j * srcWidth;
      for (i = 0; i < srcWidth; i += 4) {
	 if (srcWidth > i + 3) numxpixels = 4;
	 else numxpixels = srcWidth - i;
	 extractsrc_s(srcpixels, srcaddr, srcWidth, numxpixels, numypixels, 1);
	 signed_encode_rgtc_ubyte(blkaddr, srcpixels, numxpixels, numypixels);
	 srcaddr += numxpixels;
	 blkaddr += 8;
      }
      blkaddr += dstRowDiff;
   }
   if (tempImage)
      free((void *) tempImage);

   return GL_TRUE;
}

GLboolean
_mesa_texstore_rg_rgtc2(TEXSTORE_PARAMS)
{
   GLubyte *dst;
   const GLubyte *tempImage = NULL;
   int i, j;
   int numxpixels, numypixels;
   const GLubyte *srcaddr;
   GLubyte srcpixels[4][4];
   GLubyte *blkaddr;
   GLint dstRowDiff;

   ASSERT(dstFormat == MESA_FORMAT_RG_RGTC2 ||
          dstFormat == MESA_FORMAT_LA_LATC2);

   tempImage = _mesa_make_temp_ubyte_image(ctx, dims,
					  baseInternalFormat,
					  _mesa_get_format_base_format(dstFormat),
					  srcWidth, srcHeight, srcDepth,
					  srcFormat, srcType, srcAddr,
					  srcPacking);
   if (!tempImage)
      return GL_FALSE; /* out of memory */

   dst = dstSlices[0];

   blkaddr = dst;
   dstRowDiff = dstRowStride >= (srcWidth * 4) ? dstRowStride - (((srcWidth + 3) & ~3) * 4) : 0;
   for (j = 0; j < srcHeight; j+=4) {
      if (srcHeight > j + 3) numypixels = 4;
      else numypixels = srcHeight - j;
      srcaddr = tempImage + j * srcWidth * 2;
      for (i = 0; i < srcWidth; i += 4) {
	 if (srcWidth > i + 3) numxpixels = 4;
	 else numxpixels = srcWidth - i;
	 extractsrc_u(srcpixels, srcaddr, srcWidth, numxpixels, numypixels, 2);
	 unsigned_encode_rgtc_ubyte(blkaddr, srcpixels, numxpixels, numypixels);

	 blkaddr += 8;
	 extractsrc_u(srcpixels, (GLubyte *)srcaddr + 1, srcWidth, numxpixels, numypixels, 2);
	 unsigned_encode_rgtc_ubyte(blkaddr, srcpixels, numxpixels, numypixels);

	 blkaddr += 8;

	 srcaddr += numxpixels * 2;
      }
      blkaddr += dstRowDiff;
   }
   if (tempImage)
      free((void *) tempImage);

   return GL_TRUE;
}

GLboolean
_mesa_texstore_signed_rg_rgtc2(TEXSTORE_PARAMS)
{
   GLbyte *dst;
   const GLfloat *tempImage = NULL;
   int i, j;
   int numxpixels, numypixels;
   const GLfloat *srcaddr;
   GLbyte srcpixels[4][4];
   GLbyte *blkaddr;
   GLint dstRowDiff;

   ASSERT(dstFormat == MESA_FORMAT_SIGNED_RG_RGTC2 ||
          dstFormat == MESA_FORMAT_SIGNED_LA_LATC2);

   tempImage = _mesa_make_temp_float_image(ctx, dims,
					   baseInternalFormat,
					   _mesa_get_format_base_format(dstFormat),
					   srcWidth, srcHeight, srcDepth,
					   srcFormat, srcType, srcAddr,
					   srcPacking, 0x0);
   if (!tempImage)
      return GL_FALSE; /* out of memory */

   dst = (GLbyte *) dstSlices[0];

   blkaddr = dst;
   dstRowDiff = dstRowStride >= (srcWidth * 4) ? dstRowStride - (((srcWidth + 3) & ~3) * 4) : 0;
   for (j = 0; j < srcHeight; j += 4) {
      if (srcHeight > j + 3) numypixels = 4;
      else numypixels = srcHeight - j;
      srcaddr = tempImage + j * srcWidth * 2;
      for (i = 0; i < srcWidth; i += 4) {
	 if (srcWidth > i + 3) numxpixels = 4;
	 else numxpixels = srcWidth - i;

	 extractsrc_s(srcpixels, srcaddr, srcWidth, numxpixels, numypixels, 2);
	 signed_encode_rgtc_ubyte(blkaddr, srcpixels, numxpixels, numypixels);
	 blkaddr += 8;

	 extractsrc_s(srcpixels, srcaddr + 1, srcWidth, numxpixels, numypixels, 2);
	 signed_encode_rgtc_ubyte(blkaddr, srcpixels, numxpixels, numypixels);
	 blkaddr += 8;

	 srcaddr += numxpixels * 2;

      }
      blkaddr += dstRowDiff;
   }
   if (tempImage)
      free((void *) tempImage);

   return GL_TRUE;
}

void
_mesa_fetch_texel_2d_f_red_rgtc1(const struct swrast_texture_image *texImage,
				 GLint i, GLint j, GLint k, GLfloat *texel)
{
   GLubyte red;
   unsigned_fetch_texel_rgtc(texImage->RowStride, texImage->Map,
		       i, j, &red, 1);
   texel[RCOMP] = UBYTE_TO_FLOAT(red);
   texel[GCOMP] = 0.0;
   texel[BCOMP] = 0.0;
   texel[ACOMP] = 1.0;
}

void
_mesa_fetch_texel_2d_f_signed_red_rgtc1(const struct swrast_texture_image *texImage,
					GLint i, GLint j, GLint k, GLfloat *texel)
{
   GLbyte red;
   signed_fetch_texel_rgtc(texImage->RowStride, (GLbyte *)(texImage->Map),
		       i, j, &red, 1);
   texel[RCOMP] = BYTE_TO_FLOAT_TEX(red);
   texel[GCOMP] = 0.0;
   texel[BCOMP] = 0.0;
   texel[ACOMP] = 1.0;
}

void
_mesa_fetch_texel_2d_f_rg_rgtc2(const struct swrast_texture_image *texImage,
				 GLint i, GLint j, GLint k, GLfloat *texel)
{
   GLubyte red, green;
   unsigned_fetch_texel_rgtc(texImage->RowStride, texImage->Map,
		     i, j, &red, 2);
   unsigned_fetch_texel_rgtc(texImage->RowStride, texImage->Map + 8,
		     i, j, &green, 2);
   texel[RCOMP] = UBYTE_TO_FLOAT(red);
   texel[GCOMP] = UBYTE_TO_FLOAT(green);
   texel[BCOMP] = 0.0;
   texel[ACOMP] = 1.0;
}

void
_mesa_fetch_texel_2d_f_signed_rg_rgtc2(const struct swrast_texture_image *texImage,
				       GLint i, GLint j, GLint k, GLfloat *texel)
{
   GLbyte red, green;
   signed_fetch_texel_rgtc(texImage->RowStride, (GLbyte *)(texImage->Map),
		     i, j, &red, 2);
   signed_fetch_texel_rgtc(texImage->RowStride, (GLbyte *)(texImage->Map) + 8,
		     i, j, &green, 2);
   texel[RCOMP] = BYTE_TO_FLOAT_TEX(red);
   texel[GCOMP] = BYTE_TO_FLOAT_TEX(green);
   texel[BCOMP] = 0.0;
   texel[ACOMP] = 1.0;
}

void
_mesa_fetch_texel_2d_f_l_latc1(const struct swrast_texture_image *texImage,
                                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   GLubyte red;
   unsigned_fetch_texel_rgtc(texImage->RowStride, texImage->Map,
                       i, j, &red, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = UBYTE_TO_FLOAT(red);
   texel[ACOMP] = 1.0;
}

void
_mesa_fetch_texel_2d_f_signed_l_latc1(const struct swrast_texture_image *texImage,
                                        GLint i, GLint j, GLint k, GLfloat *texel)
{
   GLbyte red;
   signed_fetch_texel_rgtc(texImage->RowStride, (GLbyte *)(texImage->Map),
                       i, j, &red, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = BYTE_TO_FLOAT_TEX(red);
   texel[ACOMP] = 1.0;
}

void
_mesa_fetch_texel_2d_f_la_latc2(const struct swrast_texture_image *texImage,
                                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   GLubyte red, green;
   unsigned_fetch_texel_rgtc(texImage->RowStride, texImage->Map,
                     i, j, &red, 2);
   unsigned_fetch_texel_rgtc(texImage->RowStride, texImage->Map + 8,
                     i, j, &green, 2);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = UBYTE_TO_FLOAT(red);
   texel[ACOMP] = UBYTE_TO_FLOAT(green);
}

void
_mesa_fetch_texel_2d_f_signed_la_latc2(const struct swrast_texture_image *texImage,
                                       GLint i, GLint j, GLint k, GLfloat *texel)
{
   GLbyte red, green;
   signed_fetch_texel_rgtc(texImage->RowStride, (GLbyte *)(texImage->Map),
                     i, j, &red, 2);
   signed_fetch_texel_rgtc(texImage->RowStride, (GLbyte *)(texImage->Map) + 8,
                     i, j, &green, 2);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = BYTE_TO_FLOAT_TEX(red);
   texel[ACOMP] = BYTE_TO_FLOAT_TEX(green);
}

#define TAG(x) unsigned_##x

#define TYPE GLubyte
#define T_MIN 0
#define T_MAX 0xff

#include "texcompress_rgtc_tmp.h"

#undef TAG
#undef TYPE
#undef T_MIN
#undef T_MAX

#define TAG(x) signed_##x
#define TYPE GLbyte
#define T_MIN (GLbyte)-128
#define T_MAX (GLbyte)127

#include "texcompress_rgtc_tmp.h"

#undef TAG
#undef TYPE
#undef T_MIN
#undef T_MAX
