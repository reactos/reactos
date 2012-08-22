/*
 * Mesa 3-D graphics library
 * Version:  7.7
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (c) 2008-2009 VMware, Inc.
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
 * \file texformat.c
 * Texture formats.
 *
 * \author Gareth Hughes
 * \author Brian Paul
 */


#include "context.h"
#include "enums.h"
#include "mfeatures.h"
#include "mtypes.h"
#include "texcompress.h"
#include "texformat.h"

#define RETURN_IF_SUPPORTED(f) do {		\
   if (ctx->TextureFormatSupported[f])		\
      return f;					\
} while (0)

/**
 * Choose an appropriate texture format given the format, type and
 * internalFormat parameters passed to glTexImage().
 *
 * \param ctx  the GL context.
 * \param internalFormat  user's prefered internal texture format.
 * \param format  incoming image pixel format.
 * \param type  incoming image data type.
 *
 * \return a pointer to a gl_texture_format object which describes the
 * choosen texture format, or NULL on failure.
 * 
 * This is called via dd_function_table::ChooseTextureFormat.  Hardware drivers
 * will typically override this function with a specialized version.
 */
gl_format
_mesa_choose_tex_format( struct gl_context *ctx, GLint internalFormat,
                         GLenum format, GLenum type )
{
   (void) format;
   (void) type;

   switch (internalFormat) {
      /* shallow RGBA formats */
      case 4:
      case GL_RGBA:
	 if (type == GL_UNSIGNED_SHORT_4_4_4_4_REV) {
	    RETURN_IF_SUPPORTED(MESA_FORMAT_ARGB4444);
	 } else if (type == GL_UNSIGNED_SHORT_1_5_5_5_REV) {
	    RETURN_IF_SUPPORTED(MESA_FORMAT_ARGB1555);
	 }
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA8888);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_ARGB8888);
	 break;

      case GL_RGBA8:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA8888);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_ARGB8888);
	 break;
      case GL_RGB5_A1:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_ARGB1555);
	 break;
      case GL_RGBA2:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_ARGB4444_REV); /* just to test another format*/
	 RETURN_IF_SUPPORTED(MESA_FORMAT_ARGB4444);
	 break;
      case GL_RGBA4:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_ARGB4444);
	 break;

      /* deep RGBA formats */
      case GL_RGB10_A2:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_ARGB2101010);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_ARGB8888);
	 break;
      case GL_RGBA12:
      case GL_RGBA16:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_16);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_16);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA8888);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_ARGB8888);
	 break;

      /* shallow RGB formats */
      case 3:
      case GL_RGB:
      case GL_RGB8:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGB888);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_XRGB8888);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_ARGB8888);
	 break;
      case GL_R3_G3_B2:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGB332);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGB565);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGB565_REV);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGB888);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_XRGB8888);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_ARGB8888);
	 break;
      case GL_RGB4:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGB565_REV); /* just to test another format */
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGB565);
	 break;
      case GL_RGB5:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGB565);
	 break;

      /* deep RGB formats */
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_16);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_XRGB8888);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_ARGB8888);
	 break;

      /* Alpha formats */
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_A8);
	 break;

      case GL_ALPHA12:
      case GL_ALPHA16:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_A16);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_A8);
	 break;

      /* Luminance formats */
      case 1:
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_L8);
	 break;

      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_L16);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_L8);
	 break;

      /* Luminance/Alpha formats */
      case GL_LUMINANCE4_ALPHA4:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_AL44);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_AL88);
	 break;

      case 2:
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_AL88);
	 break;

      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_AL1616);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_AL88);
	 break;

      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY8:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_I8);
	 break;

      case GL_INTENSITY12:
      case GL_INTENSITY16:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_I16);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_I8);
	 break;

      default:
         ; /* fallthrough */
   }

   if (ctx->Extensions.ARB_depth_texture) {
      switch (internalFormat) {
         case GL_DEPTH_COMPONENT:
         case GL_DEPTH_COMPONENT24:
         case GL_DEPTH_COMPONENT32:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_Z32);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_X8_Z24);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_S8_Z24);
	    break;
         case GL_DEPTH_COMPONENT16:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_Z16);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_X8_Z24);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_S8_Z24);
         default:
            ; /* fallthrough */
      }
   }

   switch (internalFormat) {
      case GL_COMPRESSED_ALPHA_ARB:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_A8);
	 break;
      case GL_COMPRESSED_LUMINANCE_ARB:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_L8);
	 break;
      case GL_COMPRESSED_LUMINANCE_ALPHA_ARB:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_AL88);
	 break;
      case GL_COMPRESSED_INTENSITY_ARB:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_I8);
	 break;
      case GL_COMPRESSED_RGB_ARB:
         if (ctx->Extensions.EXT_texture_compression_s3tc ||
             ctx->Extensions.S3_s3tc)
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGB_DXT1);
         if (ctx->Extensions.TDFX_texture_compression_FXT1)
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGB_FXT1);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGB888);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_XRGB8888);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_ARGB8888);
	 break;
      case GL_COMPRESSED_RGBA_ARB:
         if (ctx->Extensions.EXT_texture_compression_s3tc ||
             ctx->Extensions.S3_s3tc)
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_DXT3); /* Not rgba_dxt1, see spec */
         if (ctx->Extensions.TDFX_texture_compression_FXT1)
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FXT1);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA8888);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_ARGB8888);
	 break;
      default:
         ; /* fallthrough */
   }

   if (ctx->Extensions.MESA_ycbcr_texture) {
      if (internalFormat == GL_YCBCR_MESA) {
         if (type == GL_UNSIGNED_SHORT_8_8_MESA)
	    RETURN_IF_SUPPORTED(MESA_FORMAT_YCBCR);
         else
	    RETURN_IF_SUPPORTED(MESA_FORMAT_YCBCR_REV);
      }
   }

#if FEATURE_texture_fxt1
   if (ctx->Extensions.TDFX_texture_compression_FXT1) {
      switch (internalFormat) {
         case GL_COMPRESSED_RGB_FXT1_3DFX:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGB_FXT1);
	 break;
         case GL_COMPRESSED_RGBA_FXT1_3DFX:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FXT1);
	 break;
         default:
            ; /* fallthrough */
      }
   }
#endif

#if FEATURE_texture_s3tc
   if (ctx->Extensions.EXT_texture_compression_s3tc) {
      switch (internalFormat) {
         case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGB_DXT1);
	    break;
         case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_DXT1);
	    break;
         case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_DXT3);
	    break;
         case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_DXT5);
	    break;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.S3_s3tc) {
      switch (internalFormat) {
         case GL_RGB_S3TC:
         case GL_RGB4_S3TC:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGB_DXT1);
	    break;
         case GL_RGBA_S3TC:
         case GL_RGBA4_S3TC:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_DXT3);
	    break;
         default:
            ; /* fallthrough */
      }
   }
#endif

   if (ctx->Extensions.ARB_texture_float) {
      switch (internalFormat) {
         case GL_ALPHA16F_ARB:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_ALPHA_FLOAT16);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_ALPHA_FLOAT32);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT16);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT32);
	    break;
         case GL_ALPHA32F_ARB:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_ALPHA_FLOAT32);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT32);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_ALPHA_FLOAT16);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT16);
	    break;
         case GL_LUMINANCE16F_ARB:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_LUMINANCE_FLOAT16);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_LUMINANCE_FLOAT32);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT16);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT32);
	    break;
         case GL_LUMINANCE32F_ARB:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_LUMINANCE_FLOAT32);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT32);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_LUMINANCE_FLOAT16);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT16);
	    break;
         case GL_LUMINANCE_ALPHA16F_ARB:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT16);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT32);
	    break;
         case GL_LUMINANCE_ALPHA32F_ARB:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT32);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT16);
	    break;
         case GL_INTENSITY16F_ARB:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_INTENSITY_FLOAT16);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_INTENSITY_FLOAT32);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT16);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT32);
	    break;
         case GL_INTENSITY32F_ARB:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_INTENSITY_FLOAT32);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT32);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_INTENSITY_FLOAT16);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT16);
	    break;
         case GL_RGB16F_ARB:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGB_FLOAT16);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT16);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGB_FLOAT32);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT32);
	    break;
         case GL_RGB32F_ARB:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGB_FLOAT32);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT32);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGB_FLOAT16);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT16);
	    break;
         case GL_RGBA16F_ARB:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT16);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT32);
	    break;
         case GL_RGBA32F_ARB:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT32);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT16);
	    break;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.EXT_texture_shared_exponent) {
      switch (internalFormat) {
         case GL_RGB9_E5:
            ASSERT(ctx->TextureFormatSupported[MESA_FORMAT_RGB9_E5_FLOAT]);
            return MESA_FORMAT_RGB9_E5_FLOAT;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.EXT_packed_float) {
      switch (internalFormat) {
         case GL_R11F_G11F_B10F:
            ASSERT(ctx->TextureFormatSupported[MESA_FORMAT_R11_G11_B10_FLOAT]);
            return MESA_FORMAT_R11_G11_B10_FLOAT;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.EXT_packed_depth_stencil) {
      switch (internalFormat) {
         case GL_DEPTH_STENCIL_EXT:
         case GL_DEPTH24_STENCIL8_EXT:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_Z24_S8);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_S8_Z24);
	    break;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.ARB_depth_buffer_float) {
      switch (internalFormat) {
         case GL_DEPTH_COMPONENT32F:
            ASSERT(ctx->TextureFormatSupported[MESA_FORMAT_Z32_FLOAT]);
            return MESA_FORMAT_Z32_FLOAT;
         case GL_DEPTH32F_STENCIL8:
            ASSERT(ctx->TextureFormatSupported[MESA_FORMAT_Z32_FLOAT_X24S8]);
            return MESA_FORMAT_Z32_FLOAT_X24S8;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.ATI_envmap_bumpmap) {
      switch (internalFormat) {
         case GL_DUDV_ATI:
         case GL_DU8DV8_ATI:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_DUDV8);
	    break;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.EXT_texture_snorm) {
      switch (internalFormat) {
         case GL_RED_SNORM:
         case GL_R8_SNORM:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_R8);
	    break;
         case GL_RG_SNORM:
         case GL_RG8_SNORM:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RG88_REV);
	    break;
         case GL_RGB_SNORM:
         case GL_RGB8_SNORM:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBX8888);
	    /* FALLTHROUGH */
         case GL_RGBA_SNORM:
         case GL_RGBA8_SNORM:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBA8888);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBA8888_REV);
	    break;
         case GL_ALPHA_SNORM:
         case GL_ALPHA8_SNORM:
            RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_A8);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBA8888);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBA8888_REV);
            break;
         case GL_LUMINANCE_SNORM:
         case GL_LUMINANCE8_SNORM:
            RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_L8);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBX8888);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBA8888);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBA8888_REV);
            break;
         case GL_LUMINANCE_ALPHA_SNORM:
         case GL_LUMINANCE8_ALPHA8_SNORM:
            RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_AL88);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBA8888);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBA8888_REV);
            break;
         case GL_INTENSITY_SNORM:
         case GL_INTENSITY8_SNORM:
            RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_I8);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBA8888);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBA8888_REV);
            break;
         case GL_R16_SNORM:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_R16);
	    break;
         case GL_RG16_SNORM:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_GR1616);
	    break;
         case GL_RGB16_SNORM:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGB_16);
	    /* FALLTHROUGH */
         case GL_RGBA16_SNORM:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBA_16);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBA8888);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBA8888_REV);
	    break;
         case GL_ALPHA16_SNORM:
            RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_A16);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBA_16);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBA8888);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBA8888_REV);
            break;
         case GL_LUMINANCE16_SNORM:
            RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_L16);
	    /* FALLTHROUGH */
         case GL_LUMINANCE16_ALPHA16_SNORM:
            RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_AL1616);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBA_16);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBA8888);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBA8888_REV);
            break;
         case GL_INTENSITY16_SNORM:
            RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_I16);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBA_16);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBA8888);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RGBA8888_REV);
            break;
         default:
            ; /* fall-through */
      }
   }

#if FEATURE_EXT_texture_sRGB
   if (ctx->Extensions.EXT_texture_sRGB) {
      switch (internalFormat) {
         case GL_SRGB_EXT:
         case GL_SRGB8_EXT:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SRGB8);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SARGB8);
	    break;
         case GL_SRGB_ALPHA_EXT:
         case GL_SRGB8_ALPHA8_EXT:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SRGBA8);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SARGB8);
	    break;
         case GL_SLUMINANCE_EXT:
         case GL_SLUMINANCE8_EXT:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SL8);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SARGB8);
	    break;
         case GL_SLUMINANCE_ALPHA_EXT:
         case GL_SLUMINANCE8_ALPHA8_EXT:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SLA8);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SARGB8);
	    break;
         case GL_COMPRESSED_SLUMINANCE_EXT:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SL8);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SARGB8);
	    break;
         case GL_COMPRESSED_SLUMINANCE_ALPHA_EXT:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SLA8);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SARGB8);
	    break;
         case GL_COMPRESSED_SRGB_EXT:
#if FEATURE_texture_s3tc
            if (ctx->Extensions.EXT_texture_compression_s3tc)
	       RETURN_IF_SUPPORTED(MESA_FORMAT_SRGB_DXT1);
#endif
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SRGB8);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SARGB8);
	    break;
         case GL_COMPRESSED_SRGB_ALPHA_EXT:
#if FEATURE_texture_s3tc
            if (ctx->Extensions.EXT_texture_compression_s3tc)
	       RETURN_IF_SUPPORTED(MESA_FORMAT_SRGBA_DXT3); /* Not srgba_dxt1, see spec */
#endif
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SRGBA8);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SARGB8);
	    break;
#if FEATURE_texture_s3tc
         case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
            if (ctx->Extensions.EXT_texture_compression_s3tc)
	       RETURN_IF_SUPPORTED(MESA_FORMAT_SRGB_DXT1);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SARGB8);
            break;
         case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
            if (ctx->Extensions.EXT_texture_compression_s3tc)
	       RETURN_IF_SUPPORTED(MESA_FORMAT_SRGBA_DXT1);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SARGB8);
            break;
         case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
            if (ctx->Extensions.EXT_texture_compression_s3tc)
	       RETURN_IF_SUPPORTED(MESA_FORMAT_SRGBA_DXT3);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SARGB8);
            break;
         case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
            if (ctx->Extensions.EXT_texture_compression_s3tc)
	       RETURN_IF_SUPPORTED(MESA_FORMAT_SRGBA_DXT5);
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SARGB8);
            break;
#endif
         default:
            ; /* fallthrough */
      }
   }
#endif /* FEATURE_EXT_texture_sRGB */

   if (ctx->Extensions.EXT_texture_integer) {
      switch (internalFormat) {
      case GL_ALPHA8UI_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_ALPHA_UINT8);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_UINT8);
         break;
      case GL_ALPHA16UI_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_ALPHA_UINT16);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_UINT16);
         break;
      case GL_ALPHA32UI_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_ALPHA_UINT32);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_UINT32);
         break;
      case GL_ALPHA8I_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_ALPHA_INT8);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_INT8);
         break;
      case GL_ALPHA16I_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_ALPHA_INT16);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_INT16);
         break;
      case GL_ALPHA32I_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_ALPHA_INT32);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_INT32);
         break;
      case GL_LUMINANCE8UI_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_LUMINANCE_UINT8);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_UINT8);
         break;
      case GL_LUMINANCE16UI_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_LUMINANCE_UINT16);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_UINT16);
         break;
      case GL_LUMINANCE32UI_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_LUMINANCE_UINT32);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_UINT32);
         break;
      case GL_LUMINANCE8I_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_LUMINANCE_INT8);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_INT8);
         break;
      case GL_LUMINANCE16I_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_LUMINANCE_INT16);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_INT16);
         break;
      case GL_LUMINANCE32I_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_LUMINANCE_INT32);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_INT32);
         break;
      case GL_LUMINANCE_ALPHA8UI_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_LUMINANCE_ALPHA_UINT8);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_UINT8);
         break;
      case GL_LUMINANCE_ALPHA16UI_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_LUMINANCE_ALPHA_UINT16);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_UINT16);
         break;
      case GL_LUMINANCE_ALPHA32UI_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_LUMINANCE_ALPHA_UINT32);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_UINT32);
         break;
      case GL_LUMINANCE_ALPHA8I_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_LUMINANCE_ALPHA_INT8);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_INT8);
         break;
      case GL_LUMINANCE_ALPHA16I_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_LUMINANCE_ALPHA_INT16);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_INT16);
         break;
      case GL_LUMINANCE_ALPHA32I_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_LUMINANCE_ALPHA_INT32);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_INT32);
         break;
      case GL_INTENSITY8UI_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_INTENSITY_UINT8);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_UINT8);
         break;
      case GL_INTENSITY16UI_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_INTENSITY_UINT16);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_UINT16);
         break;
      case GL_INTENSITY32UI_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_INTENSITY_UINT32);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_UINT32);
         break;
      case GL_INTENSITY8I_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_INTENSITY_INT8);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_INT8);
         break;
      case GL_INTENSITY16I_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_INTENSITY_INT16);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_INT16);
         break;
      case GL_INTENSITY32I_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_INTENSITY_INT32);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_INT32);
         break;
      }
   }

   if (ctx->VersionMajor >= 3 ||
       ctx->Extensions.EXT_texture_integer) {
      switch (internalFormat) {
      case GL_RGB8UI_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGB_UINT8);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_UINT8);
         break;
      case GL_RGB16UI_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGB_UINT16);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_UINT16);
         break;
      case GL_RGB32UI_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGB_UINT32);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_UINT32);
         break;
      case GL_RGB8I_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGB_INT8);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_INT8);
         break;
      case GL_RGB16I_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGB_INT16);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_INT16);
         break;
      case GL_RGB32I_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGB_INT32);
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_INT32);
         break;
      case GL_RGBA8UI_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_UINT8);
         break;
      case GL_RGBA16UI_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_UINT16);
         break;
      case GL_RGBA32UI_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_UINT32);
         break;
      case GL_RGBA8I_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_INT8);
         break;
      case GL_RGBA16I_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_INT16);
         break;
      case GL_RGBA32I_EXT:
         RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_INT32);
         break;
      }
   }

   if (ctx->Extensions.ARB_texture_rg) {
      switch (internalFormat) {
      case GL_R8:
      case GL_RED:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_R8);
	 break;

      case GL_COMPRESSED_RED:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RED_RGTC1);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_R8);
	 break;

      case GL_R16:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_R16);
	 break;

      case GL_RG:
      case GL_RG8:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_GR88);
	 break;

      case GL_COMPRESSED_RG:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RG_RGTC2);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_GR88);
	 break;

      case GL_RG16:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RG1616);
	 break;

      default:
         ; /* fallthrough */
      }
   }

   if (ctx->Extensions.ARB_texture_rg && ctx->Extensions.ARB_texture_float) {
      switch (internalFormat) {
      case GL_R16F:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_R_FLOAT16);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RG_FLOAT16);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_R_FLOAT32);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RG_FLOAT32);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT16);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT32);
	 break;
      case GL_R32F:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_R_FLOAT32);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RG_FLOAT32);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT32);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_R_FLOAT16);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RG_FLOAT16);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT16);
	 break;
      case GL_RG16F:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RG_FLOAT16);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RG_FLOAT32);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT16);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT32);
	 break;
      case GL_RG32F:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RG_FLOAT32);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT32);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RG_FLOAT16);
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RGBA_FLOAT16);
	 break;

      default:
         ; /* fallthrough */
      }
   }

   if (ctx->VersionMajor >= 3 ||
       (ctx->Extensions.ARB_texture_rg &&
        ctx->Extensions.EXT_texture_integer)) {
      switch (internalFormat) {
      case GL_R8UI:
         RETURN_IF_SUPPORTED(MESA_FORMAT_R_UINT8);
         break;
      case GL_RG8UI:
         RETURN_IF_SUPPORTED(MESA_FORMAT_RG_UINT8);
         break;
      case GL_R16UI:
         RETURN_IF_SUPPORTED(MESA_FORMAT_R_UINT16);
	 break;
      case GL_RG16UI:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_RG_UINT16);
         break;
      case GL_R32UI:
         RETURN_IF_SUPPORTED(MESA_FORMAT_R_UINT32);
         break;
      case GL_RG32UI:
         RETURN_IF_SUPPORTED(MESA_FORMAT_RG_UINT32);
         break;
      case GL_R8I:
         RETURN_IF_SUPPORTED(MESA_FORMAT_R_INT8);
         break;
      case GL_RG8I:
         RETURN_IF_SUPPORTED(MESA_FORMAT_RG_INT8);
         break;
      case GL_R16I:
         RETURN_IF_SUPPORTED(MESA_FORMAT_R_INT16);
         break;
      case GL_RG16I:
         RETURN_IF_SUPPORTED(MESA_FORMAT_RG_INT16);
         break;
      case GL_R32I:
         RETURN_IF_SUPPORTED(MESA_FORMAT_R_INT32);
         break;
      case GL_RG32I:
         RETURN_IF_SUPPORTED(MESA_FORMAT_RG_INT32);
         break;
      default:
         break;
      }
   }

   if (ctx->Extensions.ARB_texture_rgb10_a2ui) {
      switch (internalFormat) {
      case GL_RGB10_A2UI:
         RETURN_IF_SUPPORTED(MESA_FORMAT_ARGB2101010_UINT);
         break;
      default:
         break;
      }
   }
   /* GL_BGRA can be an internal format *only* in OpenGL ES (1.x or 2.0).
    */
   if (ctx->API != API_OPENGL) {
      switch (internalFormat) {
      case GL_BGRA:
	 RETURN_IF_SUPPORTED(MESA_FORMAT_ARGB8888);
	 break;

      default:
         ; /* fallthrough */
      }
   }

   if (ctx->Extensions.ARB_texture_compression_rgtc) {
      switch (internalFormat) {
         case GL_COMPRESSED_RED_RGTC1:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RED_RGTC1);
	    break;
         case GL_COMPRESSED_SIGNED_RED_RGTC1:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RED_RGTC1);
	    break;
         case GL_COMPRESSED_RG_RGTC2:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_RG_RGTC2);
	    break;
         case GL_COMPRESSED_SIGNED_RG_RGTC2:
	    RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_RG_RGTC2);
	    break;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.EXT_texture_compression_latc) {
      switch (internalFormat) {
         case GL_COMPRESSED_LUMINANCE_LATC1_EXT:
            RETURN_IF_SUPPORTED(MESA_FORMAT_L_LATC1);
            break;
         case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
            RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_L_LATC1);
            break;
         case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
            RETURN_IF_SUPPORTED(MESA_FORMAT_LA_LATC2);
            break;
         case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
            RETURN_IF_SUPPORTED(MESA_FORMAT_SIGNED_LA_LATC2);
            break;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.ATI_texture_compression_3dc) {
      switch (internalFormat) {
         case GL_COMPRESSED_LUMINANCE_ALPHA_3DC_ATI:
            RETURN_IF_SUPPORTED(MESA_FORMAT_LA_LATC2);
            break;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.OES_compressed_ETC1_RGB8_texture) {
      switch (internalFormat) {
         case GL_ETC1_RGB8_OES:
            RETURN_IF_SUPPORTED(MESA_FORMAT_ETC1_RGB8);
            break;
         default:
            ; /* fallthrough */
      }
   }

   _mesa_problem(ctx, "unexpected format %s in _mesa_choose_tex_format()",
                 _mesa_lookup_enum_by_nr(internalFormat));
   return MESA_FORMAT_NONE;
}

