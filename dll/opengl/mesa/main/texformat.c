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

#include <precomp.h>

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
	 RETURN_IF_SUPPORTED(MESA_FORMAT_ARGB8888);
	 break;
      case GL_RGBA12:
      case GL_RGBA16:
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

   if (ctx->Extensions.MESA_ycbcr_texture) {
      if (internalFormat == GL_YCBCR_MESA) {
         if (type == GL_UNSIGNED_SHORT_8_8_MESA)
	    RETURN_IF_SUPPORTED(MESA_FORMAT_YCBCR);
         else
	    RETURN_IF_SUPPORTED(MESA_FORMAT_YCBCR_REV);
      }
   }

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

   _mesa_problem(ctx, "unexpected format %s in _mesa_choose_tex_format()",
                 _mesa_lookup_enum_by_nr(internalFormat));
   return MESA_FORMAT_NONE;
}

