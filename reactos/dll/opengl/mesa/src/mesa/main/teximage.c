/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
 * \file teximage.c
 * Texture image-related functions.
 */


#include "glheader.h"
#include "bufferobj.h"
#include "context.h"
#include "enums.h"
#include "fbobject.h"
#include "framebuffer.h"
#include "hash.h"
#include "image.h"
#include "imports.h"
#include "macros.h"
#include "mfeatures.h"
#include "state.h"
#include "texcompress.h"
#include "teximage.h"
#include "texstate.h"
#include "texpal.h"
#include "mtypes.h"


/**
 * State changes which we care about for glCopyTex[Sub]Image() calls.
 * In particular, we care about pixel transfer state and buffer state
 * (such as glReadBuffer to make sure we read from the right renderbuffer).
 */
#define NEW_COPY_TEX_STATE (_NEW_BUFFERS | _NEW_PIXEL)



/**
 * Return the simple base format for a given internal texture format.
 * For example, given GL_LUMINANCE12_ALPHA4, return GL_LUMINANCE_ALPHA.
 *
 * \param ctx GL context.
 * \param internalFormat the internal texture format token or 1, 2, 3, or 4.
 *
 * \return the corresponding \u base internal format (GL_ALPHA, GL_LUMINANCE,
 * GL_LUMANCE_ALPHA, GL_INTENSITY, GL_RGB, or GL_RGBA), or -1 if invalid enum.
 *
 * This is the format which is used during texture application (i.e. the
 * texture format and env mode determine the arithmetic used.
 *
 * XXX this could be static
 */
GLint
_mesa_base_tex_format( struct gl_context *ctx, GLint internalFormat )
{
   switch (internalFormat) {
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
         return GL_ALPHA;
      case 1:
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
         return GL_LUMINANCE;
      case 2:
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
         return GL_LUMINANCE_ALPHA;
      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY8:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
         return GL_INTENSITY;
      case 3:
      case GL_RGB:
      case GL_R3_G3_B2:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
         return GL_RGB;
      case 4:
      case GL_RGBA:
      case GL_RGBA2:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
         return GL_RGBA;
      default:
         ; /* fallthrough */
   }

   /* GL_BGRA can be an internal format *only* in OpenGL ES (1.x or 2.0).
    */
   if (ctx->API != API_OPENGL) {
      switch (internalFormat) {
         case GL_BGRA:
            return GL_RGBA;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.ARB_depth_texture) {
      switch (internalFormat) {
         case GL_DEPTH_COMPONENT:
         case GL_DEPTH_COMPONENT16:
         case GL_DEPTH_COMPONENT24:
         case GL_DEPTH_COMPONENT32:
            return GL_DEPTH_COMPONENT;
         default:
            ; /* fallthrough */
      }
   }

   switch (internalFormat) {
   case GL_COMPRESSED_ALPHA:
      return GL_ALPHA;
   case GL_COMPRESSED_LUMINANCE:
      return GL_LUMINANCE;
   case GL_COMPRESSED_LUMINANCE_ALPHA:
      return GL_LUMINANCE_ALPHA;
   case GL_COMPRESSED_INTENSITY:
      return GL_INTENSITY;
   case GL_COMPRESSED_RGB:
      return GL_RGB;
   case GL_COMPRESSED_RGBA:
      return GL_RGBA;
   default:
      ; /* fallthrough */
   }
         
   if (ctx->Extensions.TDFX_texture_compression_FXT1) {
      switch (internalFormat) {
         case GL_COMPRESSED_RGB_FXT1_3DFX:
            return GL_RGB;
         case GL_COMPRESSED_RGBA_FXT1_3DFX:
            return GL_RGBA;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.EXT_texture_compression_s3tc) {
      switch (internalFormat) {
         case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            return GL_RGB;
         case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
         case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
         case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            return GL_RGBA;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.S3_s3tc) {
      switch (internalFormat) {
         case GL_RGB_S3TC:
         case GL_RGB4_S3TC:
            return GL_RGB;
         case GL_RGBA_S3TC:
         case GL_RGBA4_S3TC:
            return GL_RGBA;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.MESA_ycbcr_texture) {
      if (internalFormat == GL_YCBCR_MESA)
         return GL_YCBCR_MESA;
   }

   if (ctx->Extensions.ARB_texture_float) {
      switch (internalFormat) {
         case GL_ALPHA16F_ARB:
         case GL_ALPHA32F_ARB:
            return GL_ALPHA;
         case GL_RGBA16F_ARB:
         case GL_RGBA32F_ARB:
            return GL_RGBA;
         case GL_RGB16F_ARB:
         case GL_RGB32F_ARB:
            return GL_RGB;
         case GL_INTENSITY16F_ARB:
         case GL_INTENSITY32F_ARB:
            return GL_INTENSITY;
         case GL_LUMINANCE16F_ARB:
         case GL_LUMINANCE32F_ARB:
            return GL_LUMINANCE;
         case GL_LUMINANCE_ALPHA16F_ARB:
         case GL_LUMINANCE_ALPHA32F_ARB:
            return GL_LUMINANCE_ALPHA;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.ATI_envmap_bumpmap) {
      switch (internalFormat) {
         case GL_DUDV_ATI:
         case GL_DU8DV8_ATI:
            return GL_DUDV_ATI;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.EXT_texture_snorm) {
      switch (internalFormat) {
         case GL_RED_SNORM:
         case GL_R8_SNORM:
         case GL_R16_SNORM:
            return GL_RED;
         case GL_RG_SNORM:
         case GL_RG8_SNORM:
         case GL_RG16_SNORM:
            return GL_RG;
         case GL_RGB_SNORM:
         case GL_RGB8_SNORM:
         case GL_RGB16_SNORM:
            return GL_RGB;
         case GL_RGBA_SNORM:
         case GL_RGBA8_SNORM:
         case GL_RGBA16_SNORM:
            return GL_RGBA;
         case GL_ALPHA_SNORM:
         case GL_ALPHA8_SNORM:
         case GL_ALPHA16_SNORM:
            return GL_ALPHA;
         case GL_LUMINANCE_SNORM:
         case GL_LUMINANCE8_SNORM:
         case GL_LUMINANCE16_SNORM:
            return GL_LUMINANCE;
         case GL_LUMINANCE_ALPHA_SNORM:
         case GL_LUMINANCE8_ALPHA8_SNORM:
         case GL_LUMINANCE16_ALPHA16_SNORM:
            return GL_LUMINANCE_ALPHA;
         case GL_INTENSITY_SNORM:
         case GL_INTENSITY8_SNORM:
         case GL_INTENSITY16_SNORM:
            return GL_INTENSITY;
         default:
            ; /* fallthrough */
      }
   }

   if (ctx->Extensions.EXT_packed_depth_stencil) {
      switch (internalFormat) {
         case GL_DEPTH_STENCIL_EXT:
         case GL_DEPTH24_STENCIL8_EXT:
            return GL_DEPTH_STENCIL_EXT;
         default:
            ; /* fallthrough */
      }
   }

#if FEATURE_EXT_texture_sRGB
   if (ctx->Extensions.EXT_texture_sRGB) {
      switch (internalFormat) {
      case GL_SRGB_EXT:
      case GL_SRGB8_EXT:
      case GL_COMPRESSED_SRGB_EXT:
      case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
         return GL_RGB;
      case GL_SRGB_ALPHA_EXT:
      case GL_SRGB8_ALPHA8_EXT:
      case GL_COMPRESSED_SRGB_ALPHA_EXT:
      case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
      case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
      case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
         return GL_RGBA;
      case GL_SLUMINANCE_ALPHA_EXT:
      case GL_SLUMINANCE8_ALPHA8_EXT:
      case GL_COMPRESSED_SLUMINANCE_ALPHA_EXT:
         return GL_LUMINANCE_ALPHA;
      case GL_SLUMINANCE_EXT:
      case GL_SLUMINANCE8_EXT:
      case GL_COMPRESSED_SLUMINANCE_EXT:
         return GL_LUMINANCE;
      default:
         ; /* fallthrough */
      }
   }
#endif /* FEATURE_EXT_texture_sRGB */

   if (ctx->VersionMajor >= 3 ||
       ctx->Extensions.EXT_texture_integer) {
      switch (internalFormat) {
      case GL_RGBA8UI_EXT:
      case GL_RGBA16UI_EXT:
      case GL_RGBA32UI_EXT:
      case GL_RGBA8I_EXT:
      case GL_RGBA16I_EXT:
      case GL_RGBA32I_EXT:
      case GL_RGB10_A2UI:
         return GL_RGBA;
      case GL_RGB8UI_EXT:
      case GL_RGB16UI_EXT:
      case GL_RGB32UI_EXT:
      case GL_RGB8I_EXT:
      case GL_RGB16I_EXT:
      case GL_RGB32I_EXT:
         return GL_RGB;
      }
   }

   if (ctx->Extensions.EXT_texture_integer) {
      switch (internalFormat) {
      case GL_ALPHA8UI_EXT:
      case GL_ALPHA16UI_EXT:
      case GL_ALPHA32UI_EXT:
      case GL_ALPHA8I_EXT:
      case GL_ALPHA16I_EXT:
      case GL_ALPHA32I_EXT:
         return GL_ALPHA;
      case GL_INTENSITY8UI_EXT:
      case GL_INTENSITY16UI_EXT:
      case GL_INTENSITY32UI_EXT:
      case GL_INTENSITY8I_EXT:
      case GL_INTENSITY16I_EXT:
      case GL_INTENSITY32I_EXT:
         return GL_INTENSITY;
      case GL_LUMINANCE8UI_EXT:
      case GL_LUMINANCE16UI_EXT:
      case GL_LUMINANCE32UI_EXT:
      case GL_LUMINANCE8I_EXT:
      case GL_LUMINANCE16I_EXT:
      case GL_LUMINANCE32I_EXT:
         return GL_LUMINANCE;
      case GL_LUMINANCE_ALPHA8UI_EXT:
      case GL_LUMINANCE_ALPHA16UI_EXT:
      case GL_LUMINANCE_ALPHA32UI_EXT:
      case GL_LUMINANCE_ALPHA8I_EXT:
      case GL_LUMINANCE_ALPHA16I_EXT:
      case GL_LUMINANCE_ALPHA32I_EXT:
         return GL_LUMINANCE_ALPHA;
      default:
         ; /* fallthrough */
      }
   }

   if (ctx->Extensions.ARB_texture_rg) {
      switch (internalFormat) {
      case GL_R16F:
	 /* R16F depends on both ARB_half_float_pixel and ARB_texture_float.
	  */
	 if (!ctx->Extensions.ARB_half_float_pixel)
	    break;
	 /* FALLTHROUGH */
      case GL_R32F:
	 if (!ctx->Extensions.ARB_texture_float)
	    break;
         return GL_RED;
      case GL_R8I:
      case GL_R8UI:
      case GL_R16I:
      case GL_R16UI:
      case GL_R32I:
      case GL_R32UI:
	 if (ctx->VersionMajor < 3 && !ctx->Extensions.EXT_texture_integer)
	    break;
	 /* FALLTHROUGH */
      case GL_R8:
      case GL_R16:
      case GL_RED:
      case GL_COMPRESSED_RED:
         return GL_RED;

      case GL_RG16F:
	 /* RG16F depends on both ARB_half_float_pixel and ARB_texture_float.
	  */
	 if (!ctx->Extensions.ARB_half_float_pixel)
	    break;
	 /* FALLTHROUGH */
      case GL_RG32F:
	 if (!ctx->Extensions.ARB_texture_float)
	    break;
         return GL_RG;
      case GL_RG8I:
      case GL_RG8UI:
      case GL_RG16I:
      case GL_RG16UI:
      case GL_RG32I:
      case GL_RG32UI:
	 if (ctx->VersionMajor < 3 && !ctx->Extensions.EXT_texture_integer)
	    break;
	 /* FALLTHROUGH */
      case GL_RG:
      case GL_RG8:
      case GL_RG16:
      case GL_COMPRESSED_RG:
         return GL_RG;
      default:
         ; /* fallthrough */
      }
   }

   if (ctx->Extensions.EXT_texture_shared_exponent) {
      switch (internalFormat) {
      case GL_RGB9_E5_EXT:
         return GL_RGB;
      default:
         ; /* fallthrough */
      }
   }

   if (ctx->Extensions.EXT_packed_float) {
      switch (internalFormat) {
      case GL_R11F_G11F_B10F_EXT:
         return GL_RGB;
      default:
         ; /* fallthrough */
      }
   }

   if (ctx->Extensions.ARB_depth_buffer_float) {
      switch (internalFormat) {
      case GL_DEPTH_COMPONENT32F:
         return GL_DEPTH_COMPONENT;
      case GL_DEPTH32F_STENCIL8:
         return GL_DEPTH_STENCIL;
      default:
         ; /* fallthrough */
      }
   }

   if (ctx->Extensions.ARB_texture_compression_rgtc) {
      switch (internalFormat) {
      case GL_COMPRESSED_RED_RGTC1:
      case GL_COMPRESSED_SIGNED_RED_RGTC1:
         return GL_RED;
      case GL_COMPRESSED_RG_RGTC2:
      case GL_COMPRESSED_SIGNED_RG_RGTC2:
         return GL_RG;
      default:
         ; /* fallthrough */
      }
   }

   if (ctx->Extensions.EXT_texture_compression_latc) {
      switch (internalFormat) {
      case GL_COMPRESSED_LUMINANCE_LATC1_EXT:
      case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
         return GL_LUMINANCE;
      case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
      case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
         return GL_LUMINANCE_ALPHA;
      default:
         ; /* fallthrough */
      }
   }

   if (ctx->Extensions.ATI_texture_compression_3dc) {
      switch (internalFormat) {
      case GL_COMPRESSED_LUMINANCE_ALPHA_3DC_ATI:
         return GL_LUMINANCE_ALPHA;
      default:
         ; /* fallthrough */
      }
   }

   if (ctx->Extensions.OES_compressed_ETC1_RGB8_texture) {
      switch (internalFormat) {
      case GL_ETC1_RGB8_OES:
         return GL_RGB;
      default:
         ; /* fallthrough */
      }
   }

   if (ctx->API == API_OPENGLES) {
      switch (internalFormat) {
      case GL_PALETTE4_RGB8_OES:
      case GL_PALETTE4_R5_G6_B5_OES:
      case GL_PALETTE8_RGB8_OES:
      case GL_PALETTE8_R5_G6_B5_OES:
	 return GL_RGB;
      case GL_PALETTE4_RGBA8_OES:
      case GL_PALETTE8_RGB5_A1_OES:
      case GL_PALETTE4_RGBA4_OES:
      case GL_PALETTE4_RGB5_A1_OES:
      case GL_PALETTE8_RGBA8_OES:
      case GL_PALETTE8_RGBA4_OES:
	 return GL_RGBA;
      default:
         ; /* fallthrough */
      }
   }

   return -1; /* error */
}


/**
 * Is the given texture format a generic compressed format?
 */
static GLboolean
is_generic_compressed_format(GLenum format)
{
   switch (format) {
   case GL_COMPRESSED_RED:
   case GL_COMPRESSED_RG:
   case GL_COMPRESSED_RGB:
   case GL_COMPRESSED_RGBA:
   case GL_COMPRESSED_ALPHA:
   case GL_COMPRESSED_LUMINANCE:
   case GL_COMPRESSED_LUMINANCE_ALPHA:
   case GL_COMPRESSED_INTENSITY:
   case GL_COMPRESSED_SRGB:
   case GL_COMPRESSED_SRGB_ALPHA:
   case GL_COMPRESSED_SLUMINANCE:
   case GL_COMPRESSED_SLUMINANCE_ALPHA:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


/**
 * For cube map faces, return a face index in [0,5].
 * For other targets return 0;
 */
GLuint
_mesa_tex_target_to_face(GLenum target)
{
   if (_mesa_is_cube_face(target))
      return (GLuint) target - (GLuint) GL_TEXTURE_CUBE_MAP_POSITIVE_X;
   else
      return 0;
}



/**
 * Install gl_texture_image in a gl_texture_object according to the target
 * and level parameters.
 * 
 * \param tObj texture object.
 * \param target texture target.
 * \param level image level.
 * \param texImage texture image.
 */
static void
set_tex_image(struct gl_texture_object *tObj,
              GLenum target, GLint level,
              struct gl_texture_image *texImage)
{
   const GLuint face = _mesa_tex_target_to_face(target);

   ASSERT(tObj);
   ASSERT(texImage);
   if (target == GL_TEXTURE_RECTANGLE_NV || target == GL_TEXTURE_EXTERNAL_OES)
      assert(level == 0);

   tObj->Image[face][level] = texImage;

   /* Set the 'back' pointer */
   texImage->TexObject = tObj;
   texImage->Level = level;
   texImage->Face = face;
}


/**
 * Allocate a texture image structure.
 * 
 * Called via ctx->Driver.NewTextureImage() unless overriden by a device
 * driver.
 *
 * \return a pointer to gl_texture_image struct with all fields initialized to
 * zero.
 */
struct gl_texture_image *
_mesa_new_texture_image( struct gl_context *ctx )
{
   (void) ctx;
   return CALLOC_STRUCT(gl_texture_image);
}


/**
 * Free a gl_texture_image and associated data.
 * This function is a fallback called via ctx->Driver.DeleteTextureImage().
 *
 * \param texImage texture image.
 *
 * Free the texture image structure and the associated image data.
 */
void
_mesa_delete_texture_image(struct gl_context *ctx,
                           struct gl_texture_image *texImage)
{
   /* Free texImage->Data and/or any other driver-specific texture
    * image storage.
    */
   ASSERT(ctx->Driver.FreeTextureImageBuffer);
   ctx->Driver.FreeTextureImageBuffer( ctx, texImage );
   free(texImage);
}


/**
 * Test if a target is a proxy target.
 *
 * \param target texture target.
 *
 * \return GL_TRUE if the target is a proxy target, GL_FALSE otherwise.
 */
GLboolean
_mesa_is_proxy_texture(GLenum target)
{
   /*
    * NUM_TEXTURE_TARGETS should match number of terms below, except there's no
    * proxy for GL_TEXTURE_BUFFER and GL_TEXTURE_EXTERNAL_OES.
    */
   assert(NUM_TEXTURE_TARGETS == 7 + 2);

   return (target == GL_PROXY_TEXTURE_1D ||
           target == GL_PROXY_TEXTURE_2D ||
           target == GL_PROXY_TEXTURE_3D ||
           target == GL_PROXY_TEXTURE_CUBE_MAP_ARB ||
           target == GL_PROXY_TEXTURE_RECTANGLE_NV ||
           target == GL_PROXY_TEXTURE_1D_ARRAY_EXT ||
           target == GL_PROXY_TEXTURE_2D_ARRAY_EXT);
}


/**
 * Return the proxy target which corresponds to the given texture target
 */
static GLenum
get_proxy_target(GLenum target)
{
   switch (target) {
   case GL_TEXTURE_1D:
   case GL_PROXY_TEXTURE_1D:
      return GL_PROXY_TEXTURE_1D;
   case GL_TEXTURE_2D:
   case GL_PROXY_TEXTURE_2D:
      return GL_PROXY_TEXTURE_2D;
   case GL_TEXTURE_3D:
   case GL_PROXY_TEXTURE_3D:
      return GL_PROXY_TEXTURE_3D;
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
   case GL_TEXTURE_CUBE_MAP_ARB:
   case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
      return GL_PROXY_TEXTURE_CUBE_MAP_ARB;
   case GL_TEXTURE_RECTANGLE_NV:
   case GL_PROXY_TEXTURE_RECTANGLE_NV:
      return GL_PROXY_TEXTURE_RECTANGLE_NV;
   case GL_TEXTURE_1D_ARRAY_EXT:
   case GL_PROXY_TEXTURE_1D_ARRAY_EXT:
      return GL_PROXY_TEXTURE_1D_ARRAY_EXT;
   case GL_TEXTURE_2D_ARRAY_EXT:
   case GL_PROXY_TEXTURE_2D_ARRAY_EXT:
      return GL_PROXY_TEXTURE_2D_ARRAY_EXT;
   default:
      _mesa_problem(NULL, "unexpected target in get_proxy_target()");
      return 0;
   }
}


/**
 * Get the texture object that corresponds to the target of the given
 * texture unit.  The target should have already been checked for validity.
 *
 * \param ctx GL context.
 * \param texUnit texture unit.
 * \param target texture target.
 *
 * \return pointer to the texture object on success, or NULL on failure.
 */
struct gl_texture_object *
_mesa_select_tex_object(struct gl_context *ctx,
                        const struct gl_texture_unit *texUnit,
                        GLenum target)
{
   const GLboolean arrayTex = (ctx->Extensions.MESA_texture_array ||
                               ctx->Extensions.EXT_texture_array);

   switch (target) {
      case GL_TEXTURE_1D:
         return texUnit->CurrentTex[TEXTURE_1D_INDEX];
      case GL_PROXY_TEXTURE_1D:
         return ctx->Texture.ProxyTex[TEXTURE_1D_INDEX];
      case GL_TEXTURE_2D:
         return texUnit->CurrentTex[TEXTURE_2D_INDEX];
      case GL_PROXY_TEXTURE_2D:
         return ctx->Texture.ProxyTex[TEXTURE_2D_INDEX];
      case GL_TEXTURE_3D:
         return texUnit->CurrentTex[TEXTURE_3D_INDEX];
      case GL_PROXY_TEXTURE_3D:
         return ctx->Texture.ProxyTex[TEXTURE_3D_INDEX];
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
      case GL_TEXTURE_CUBE_MAP_ARB:
         return ctx->Extensions.ARB_texture_cube_map
                ? texUnit->CurrentTex[TEXTURE_CUBE_INDEX] : NULL;
      case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
         return ctx->Extensions.ARB_texture_cube_map
                ? ctx->Texture.ProxyTex[TEXTURE_CUBE_INDEX] : NULL;
      case GL_TEXTURE_RECTANGLE_NV:
         return ctx->Extensions.NV_texture_rectangle
                ? texUnit->CurrentTex[TEXTURE_RECT_INDEX] : NULL;
      case GL_PROXY_TEXTURE_RECTANGLE_NV:
         return ctx->Extensions.NV_texture_rectangle
                ? ctx->Texture.ProxyTex[TEXTURE_RECT_INDEX] : NULL;
      case GL_TEXTURE_1D_ARRAY_EXT:
         return arrayTex ? texUnit->CurrentTex[TEXTURE_1D_ARRAY_INDEX] : NULL;
      case GL_PROXY_TEXTURE_1D_ARRAY_EXT:
         return arrayTex ? ctx->Texture.ProxyTex[TEXTURE_1D_ARRAY_INDEX] : NULL;
      case GL_TEXTURE_2D_ARRAY_EXT:
         return arrayTex ? texUnit->CurrentTex[TEXTURE_2D_ARRAY_INDEX] : NULL;
      case GL_PROXY_TEXTURE_2D_ARRAY_EXT:
         return arrayTex ? ctx->Texture.ProxyTex[TEXTURE_2D_ARRAY_INDEX] : NULL;
      case GL_TEXTURE_BUFFER:
         return ctx->Extensions.ARB_texture_buffer_object
            ? texUnit->CurrentTex[TEXTURE_BUFFER_INDEX] : NULL;
      case GL_TEXTURE_EXTERNAL_OES:
         return ctx->Extensions.OES_EGL_image_external
            ? texUnit->CurrentTex[TEXTURE_EXTERNAL_INDEX] : NULL;
      default:
         _mesa_problem(NULL, "bad target in _mesa_select_tex_object()");
         return NULL;
   }
}


/**
 * Return pointer to texture object for given target on current texture unit.
 */
struct gl_texture_object *
_mesa_get_current_tex_object(struct gl_context *ctx, GLenum target)
{
   struct gl_texture_unit *texUnit = _mesa_get_current_tex_unit(ctx);
   return _mesa_select_tex_object(ctx, texUnit, target);
}


/**
 * Get a texture image pointer from a texture object, given a texture
 * target and mipmap level.  The target and level parameters should
 * have already been error-checked.
 *
 * \param ctx GL context.
 * \param texObj texture unit.
 * \param target texture target.
 * \param level image level.
 *
 * \return pointer to the texture image structure, or NULL on failure.
 */
struct gl_texture_image *
_mesa_select_tex_image(struct gl_context *ctx,
                       const struct gl_texture_object *texObj,
		       GLenum target, GLint level)
{
   const GLuint face = _mesa_tex_target_to_face(target);

   ASSERT(texObj);
   ASSERT(level >= 0);
   ASSERT(level < MAX_TEXTURE_LEVELS);

   return texObj->Image[face][level];
}


/**
 * Like _mesa_select_tex_image() but if the image doesn't exist, allocate
 * it and install it.  Only return NULL if passed a bad parameter or run
 * out of memory.
 */
struct gl_texture_image *
_mesa_get_tex_image(struct gl_context *ctx, struct gl_texture_object *texObj,
                    GLenum target, GLint level)
{
   struct gl_texture_image *texImage;

   if (!texObj)
      return NULL;
   
   texImage = _mesa_select_tex_image(ctx, texObj, target, level);
   if (!texImage) {
      texImage = ctx->Driver.NewTextureImage(ctx);
      if (!texImage) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "texture image allocation");
         return NULL;
      }

      set_tex_image(texObj, target, level, texImage);
   }

   return texImage;
}


/**
 * Return pointer to the specified proxy texture image.
 * Note that proxy textures are per-context, not per-texture unit.
 * \return pointer to texture image or NULL if invalid target, invalid
 *         level, or out of memory.
 */
struct gl_texture_image *
_mesa_get_proxy_tex_image(struct gl_context *ctx, GLenum target, GLint level)
{
   struct gl_texture_image *texImage;
   GLuint texIndex;

   if (level < 0 )
      return NULL;

   switch (target) {
   case GL_PROXY_TEXTURE_1D:
      if (level >= ctx->Const.MaxTextureLevels)
         return NULL;
      texIndex = TEXTURE_1D_INDEX;
      break;
   case GL_PROXY_TEXTURE_2D:
      if (level >= ctx->Const.MaxTextureLevels)
         return NULL;
      texIndex = TEXTURE_2D_INDEX;
      break;
   case GL_PROXY_TEXTURE_3D:
      if (level >= ctx->Const.Max3DTextureLevels)
         return NULL;
      texIndex = TEXTURE_3D_INDEX;
      break;
   case GL_PROXY_TEXTURE_CUBE_MAP:
      if (level >= ctx->Const.MaxCubeTextureLevels)
         return NULL;
      texIndex = TEXTURE_CUBE_INDEX;
      break;
   case GL_PROXY_TEXTURE_RECTANGLE_NV:
      if (level > 0)
         return NULL;
      texIndex = TEXTURE_RECT_INDEX;
      break;
   case GL_PROXY_TEXTURE_1D_ARRAY_EXT:
      if (level >= ctx->Const.MaxTextureLevels)
         return NULL;
      texIndex = TEXTURE_1D_ARRAY_INDEX;
      break;
   case GL_PROXY_TEXTURE_2D_ARRAY_EXT:
      if (level >= ctx->Const.MaxTextureLevels)
         return NULL;
      texIndex = TEXTURE_2D_ARRAY_INDEX;
      break;
   default:
      return NULL;
   }

   texImage = ctx->Texture.ProxyTex[texIndex]->Image[0][level];
   if (!texImage) {
      texImage = ctx->Driver.NewTextureImage(ctx);
      if (!texImage) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "proxy texture allocation");
         return NULL;
      }
      ctx->Texture.ProxyTex[texIndex]->Image[0][level] = texImage;
      /* Set the 'back' pointer */
      texImage->TexObject = ctx->Texture.ProxyTex[texIndex];
   }
   return texImage;
}


/**
 * Get the maximum number of allowed mipmap levels.
 *
 * \param ctx GL context.
 * \param target texture target.
 * 
 * \return the maximum number of allowed mipmap levels for the given
 * texture target, or zero if passed a bad target.
 *
 * \sa gl_constants.
 */
GLint
_mesa_max_texture_levels(struct gl_context *ctx, GLenum target)
{
   switch (target) {
   case GL_TEXTURE_1D:
   case GL_PROXY_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_PROXY_TEXTURE_2D:
      return ctx->Const.MaxTextureLevels;
   case GL_TEXTURE_3D:
   case GL_PROXY_TEXTURE_3D:
      return ctx->Const.Max3DTextureLevels;
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
   case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
      return ctx->Extensions.ARB_texture_cube_map
         ? ctx->Const.MaxCubeTextureLevels : 0;
   case GL_TEXTURE_RECTANGLE_NV:
   case GL_PROXY_TEXTURE_RECTANGLE_NV:
      return ctx->Extensions.NV_texture_rectangle ? 1 : 0;
   case GL_TEXTURE_1D_ARRAY_EXT:
   case GL_PROXY_TEXTURE_1D_ARRAY_EXT:
   case GL_TEXTURE_2D_ARRAY_EXT:
   case GL_PROXY_TEXTURE_2D_ARRAY_EXT:
      return (ctx->Extensions.MESA_texture_array ||
              ctx->Extensions.EXT_texture_array)
         ? ctx->Const.MaxTextureLevels : 0;
   case GL_TEXTURE_BUFFER:
   case GL_TEXTURE_EXTERNAL_OES:
      /* fall-through */
   default:
      return 0; /* bad target */
   }
}


/**
 * Return number of dimensions per mipmap level for the given texture target.
 */
GLint
_mesa_get_texture_dimensions(GLenum target)
{
   switch (target) {
   case GL_TEXTURE_1D:
   case GL_PROXY_TEXTURE_1D:
      return 1;
   case GL_TEXTURE_2D:
   case GL_TEXTURE_RECTANGLE:
   case GL_TEXTURE_CUBE_MAP:
   case GL_PROXY_TEXTURE_2D:
   case GL_PROXY_TEXTURE_RECTANGLE:
   case GL_PROXY_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
   case GL_TEXTURE_1D_ARRAY:
   case GL_PROXY_TEXTURE_1D_ARRAY:
   case GL_TEXTURE_EXTERNAL_OES:
      return 2;
   case GL_TEXTURE_3D:
   case GL_PROXY_TEXTURE_3D:
   case GL_TEXTURE_2D_ARRAY:
   case GL_PROXY_TEXTURE_2D_ARRAY:
      return 3;
   case GL_TEXTURE_BUFFER:
      /* fall-through */
   default:
      _mesa_problem(NULL, "invalid target 0x%x in get_texture_dimensions()",
                    target);
      return 2;
   }
}




#if 000 /* not used anymore */
/*
 * glTexImage[123]D can accept a NULL image pointer.  In this case we
 * create a texture image with unspecified image contents per the OpenGL
 * spec.
 */
static GLubyte *
make_null_texture(GLint width, GLint height, GLint depth, GLenum format)
{
   const GLint components = _mesa_components_in_format(format);
   const GLint numPixels = width * height * depth;
   GLubyte *data = (GLubyte *) MALLOC(numPixels * components * sizeof(GLubyte));

#ifdef DEBUG
   /*
    * Let's see if anyone finds this.  If glTexImage2D() is called with
    * a NULL image pointer then load the texture image with something
    * interesting instead of leaving it indeterminate.
    */
   if (data) {
      static const char message[8][32] = {
         "   X   X  XXXXX   XXX     X    ",
         "   XX XX  X      X   X   X X   ",
         "   X X X  X      X      X   X  ",
         "   X   X  XXXX    XXX   XXXXX  ",
         "   X   X  X          X  X   X  ",
         "   X   X  X      X   X  X   X  ",
         "   X   X  XXXXX   XXX   X   X  ",
         "                               "
      };

      GLubyte *imgPtr = data;
      GLint h, i, j, k;
      for (h = 0; h < depth; h++) {
         for (i = 0; i < height; i++) {
            GLint srcRow = 7 - (i % 8);
            for (j = 0; j < width; j++) {
               GLint srcCol = j % 32;
               GLubyte texel = (message[srcRow][srcCol]=='X') ? 255 : 70;
               for (k = 0; k < components; k++) {
                  *imgPtr++ = texel;
               }
            }
         }
      }
   }
#endif

   return data;
}
#endif



/**
 * Set the size and format-related fields of a gl_texture_image struct
 * to zero.  This is used when a proxy texture test fails.
 */
static void
clear_teximage_fields(struct gl_texture_image *img)
{
   ASSERT(img);
   img->_BaseFormat = 0;
   img->InternalFormat = 0;
   img->Border = 0;
   img->Width = 0;
   img->Height = 0;
   img->Depth = 0;
   img->Width2 = 0;
   img->Height2 = 0;
   img->Depth2 = 0;
   img->WidthLog2 = 0;
   img->HeightLog2 = 0;
   img->DepthLog2 = 0;
   img->TexFormat = MESA_FORMAT_NONE;
}


/**
 * Initialize basic fields of the gl_texture_image struct.
 *
 * \param ctx GL context.
 * \param img texture image structure to be initialized.
 * \param width image width.
 * \param height image height.
 * \param depth image depth.
 * \param border image border.
 * \param internalFormat internal format.
 * \param format  the actual hardware format (one of MESA_FORMAT_*)
 *
 * Fills in the fields of \p img with the given information.
 * Note: width, height and depth include the border.
 */
void
_mesa_init_teximage_fields(struct gl_context *ctx,
                           struct gl_texture_image *img,
                           GLsizei width, GLsizei height, GLsizei depth,
                           GLint border, GLenum internalFormat,
                           gl_format format)
{
   GLenum target;
   ASSERT(img);
   ASSERT(width >= 0);
   ASSERT(height >= 0);
   ASSERT(depth >= 0);

   target = img->TexObject->Target;
   img->_BaseFormat = _mesa_base_tex_format( ctx, internalFormat );
   ASSERT(img->_BaseFormat > 0);
   img->InternalFormat = internalFormat;
   img->Border = border;
   img->Width = width;
   img->Height = height;
   img->Depth = depth;

   img->Width2 = width - 2 * border;   /* == 1 << img->WidthLog2; */
   img->WidthLog2 = _mesa_logbase2(img->Width2);

   switch(target) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_BUFFER:
   case GL_PROXY_TEXTURE_1D:
      if (height == 0)
         img->Height2 = 0;
      else
         img->Height2 = 1;
      img->HeightLog2 = 0;
      if (depth == 0)
         img->Depth2 = 0;
      else
         img->Depth2 = 1;
      img->DepthLog2 = 0;
      break;
   case GL_TEXTURE_1D_ARRAY:
   case GL_PROXY_TEXTURE_1D_ARRAY:
      img->Height2 = height; /* no border */
      img->HeightLog2 = 0; /* not used */
      if (depth == 0)
         img->Depth2 = 0;
      else
         img->Depth2 = 1;
      img->DepthLog2 = 0;
      break;
   case GL_TEXTURE_2D:
   case GL_TEXTURE_RECTANGLE:
   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
   case GL_TEXTURE_EXTERNAL_OES:
   case GL_PROXY_TEXTURE_2D:
   case GL_PROXY_TEXTURE_RECTANGLE:
   case GL_PROXY_TEXTURE_CUBE_MAP:
      img->Height2 = height - 2 * border; /* == 1 << img->HeightLog2; */
      img->HeightLog2 = _mesa_logbase2(img->Height2);
      if (depth == 0)
         img->Depth2 = 0;
      else
         img->Depth2 = 1;
      img->DepthLog2 = 0;
      break;
   case GL_TEXTURE_2D_ARRAY:
   case GL_PROXY_TEXTURE_2D_ARRAY:
      img->Height2 = height - 2 * border; /* == 1 << img->HeightLog2; */
      img->HeightLog2 = _mesa_logbase2(img->Height2);
      img->Depth2 = depth; /* no border */
      img->DepthLog2 = 0; /* not used */
      break;
   case GL_TEXTURE_3D:
   case GL_PROXY_TEXTURE_3D:
      img->Height2 = height - 2 * border; /* == 1 << img->HeightLog2; */
      img->HeightLog2 = _mesa_logbase2(img->Height2);
      img->Depth2 = depth - 2 * border;   /* == 1 << img->DepthLog2; */
      img->DepthLog2 = _mesa_logbase2(img->Depth2);
      break;
   default:
      _mesa_problem(NULL, "invalid target 0x%x in _mesa_init_teximage_fields()",
                    target);
   }

   img->MaxLog2 = MAX2(img->WidthLog2, img->HeightLog2);
   img->TexFormat = format;
}


/**
 * Free and clear fields of the gl_texture_image struct.
 *
 * \param ctx GL context.
 * \param texImage texture image structure to be cleared.
 *
 * After the call, \p texImage will have no data associated with it.  Its
 * fields are cleared so that its parent object will test incomplete.
 */
void
_mesa_clear_texture_image(struct gl_context *ctx,
                          struct gl_texture_image *texImage)
{
   ctx->Driver.FreeTextureImageBuffer(ctx, texImage);
   clear_teximage_fields(texImage);
}


/**
 * This is the fallback for Driver.TestProxyTexImage().  Test the texture
 * level, width, height and depth against the ctx->Const limits for textures.
 *
 * A hardware driver might override this function if, for example, the
 * max 3D texture size is 512x512x64 (i.e. not a cube).
 *
 * Note that width, height, depth == 0 is not an error.  However, a
 * texture with zero width/height/depth will be considered "incomplete"
 * and texturing will effectively be disabled.
 *
 * \param target  one of GL_PROXY_TEXTURE_1D, GL_PROXY_TEXTURE_2D,
 *                GL_PROXY_TEXTURE_3D, GL_PROXY_TEXTURE_RECTANGLE_NV,
 *                GL_PROXY_TEXTURE_CUBE_MAP_ARB.
 * \param level  as passed to glTexImage
 * \param internalFormat  as passed to glTexImage
 * \param format  as passed to glTexImage
 * \param type  as passed to glTexImage
 * \param width  as passed to glTexImage
 * \param height  as passed to glTexImage
 * \param depth  as passed to glTexImage
 * \param border  as passed to glTexImage
 * \return GL_TRUE if the image is acceptable, GL_FALSE if not acceptable.
 */
GLboolean
_mesa_test_proxy_teximage(struct gl_context *ctx, GLenum target, GLint level,
                          GLint internalFormat, GLenum format, GLenum type,
                          GLint width, GLint height, GLint depth, GLint border)
{
   GLint maxSize;

   (void) internalFormat;
   (void) format;
   (void) type;

   switch (target) {
   case GL_PROXY_TEXTURE_1D:
      maxSize = 1 << (ctx->Const.MaxTextureLevels - 1);
      if (width < 2 * border || width > 2 * border + maxSize)
         return GL_FALSE;
      if (level >= ctx->Const.MaxTextureLevels)
         return GL_FALSE;
      if (!ctx->Extensions.ARB_texture_non_power_of_two) {
         if (width > 0 && !_mesa_is_pow_two(width - 2 * border))
            return GL_FALSE;
      }
      return GL_TRUE;

   case GL_PROXY_TEXTURE_2D:
      maxSize = 1 << (ctx->Const.MaxTextureLevels - 1);
      if (width < 2 * border || width > 2 * border + maxSize)
         return GL_FALSE;
      if (height < 2 * border || height > 2 * border + maxSize)
         return GL_FALSE;
      if (level >= ctx->Const.MaxTextureLevels)
         return GL_FALSE;
      if (!ctx->Extensions.ARB_texture_non_power_of_two) {
         if (width > 0 && !_mesa_is_pow_two(width - 2 * border))
            return GL_FALSE;
         if (height > 0 && !_mesa_is_pow_two(height - 2 * border))
            return GL_FALSE;
      }
      return GL_TRUE;

   case GL_PROXY_TEXTURE_3D:
      maxSize = 1 << (ctx->Const.Max3DTextureLevels - 1);
      if (width < 2 * border || width > 2 * border + maxSize)
         return GL_FALSE;
      if (height < 2 * border || height > 2 * border + maxSize)
         return GL_FALSE;
      if (depth < 2 * border || depth > 2 * border + maxSize)
         return GL_FALSE;
      if (level >= ctx->Const.Max3DTextureLevels)
         return GL_FALSE;
      if (!ctx->Extensions.ARB_texture_non_power_of_two) {
         if (width > 0 && !_mesa_is_pow_two(width - 2 * border))
            return GL_FALSE;
         if (height > 0 && !_mesa_is_pow_two(height - 2 * border))
            return GL_FALSE;
         if (depth > 0 && !_mesa_is_pow_two(depth - 2 * border))
            return GL_FALSE;
      }
      return GL_TRUE;

   case GL_PROXY_TEXTURE_RECTANGLE_NV:
      maxSize = ctx->Const.MaxTextureRectSize;
      if (width < 0 || width > maxSize)
         return GL_FALSE;
      if (height < 0 || height > maxSize)
         return GL_FALSE;
      if (level != 0)
         return GL_FALSE;
      return GL_TRUE;

   case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
      maxSize = 1 << (ctx->Const.MaxCubeTextureLevels - 1);
      if (width < 2 * border || width > 2 * border + maxSize)
         return GL_FALSE;
      if (height < 2 * border || height > 2 * border + maxSize)
         return GL_FALSE;
      if (level >= ctx->Const.MaxCubeTextureLevels)
         return GL_FALSE;
      if (!ctx->Extensions.ARB_texture_non_power_of_two) {
         if (width > 0 && !_mesa_is_pow_two(width - 2 * border))
            return GL_FALSE;
         if (height > 0 && !_mesa_is_pow_two(height - 2 * border))
            return GL_FALSE;
      }
      return GL_TRUE;

   case GL_PROXY_TEXTURE_1D_ARRAY_EXT:
      maxSize = 1 << (ctx->Const.MaxTextureLevels - 1);
      if (width < 2 * border || width > 2 * border + maxSize)
         return GL_FALSE;
      if (height < 1 || height > ctx->Const.MaxArrayTextureLayers)
         return GL_FALSE;
      if (level >= ctx->Const.MaxTextureLevels)
         return GL_FALSE;
      if (!ctx->Extensions.ARB_texture_non_power_of_two) {
         if (width > 0 && !_mesa_is_pow_two(width - 2 * border))
            return GL_FALSE;
      }
      return GL_TRUE;

   case GL_PROXY_TEXTURE_2D_ARRAY_EXT:
      maxSize = 1 << (ctx->Const.MaxTextureLevels - 1);
      if (width < 2 * border || width > 2 * border + maxSize)
         return GL_FALSE;
      if (height < 2 * border || height > 2 * border + maxSize)
         return GL_FALSE;
      if (depth < 1 || depth > ctx->Const.MaxArrayTextureLayers)
         return GL_FALSE;
      if (level >= ctx->Const.MaxTextureLevels)
         return GL_FALSE;
      if (!ctx->Extensions.ARB_texture_non_power_of_two) {
         if (width > 0 && !_mesa_is_pow_two(width - 2 * border))
            return GL_FALSE;
         if (height > 0 && !_mesa_is_pow_two(height - 2 * border))
            return GL_FALSE;
      }
      return GL_TRUE;

   default:
      _mesa_problem(ctx, "Invalid target in _mesa_test_proxy_teximage");
      return GL_FALSE;
   }
}


/**
 * Check if the memory used by the texture would exceed the driver's limit.
 * This lets us support a max 3D texture size of 8K (for example) but
 * prevents allocating a full 8K x 8K x 8K texture.
 * XXX this could be rolled into the proxy texture size test (above) but
 * we don't have the actual texture internal format at that point.
 */
static GLboolean
legal_texture_size(struct gl_context *ctx, gl_format format,
                   GLint width, GLint height, GLint depth)
{
   uint64_t bytes = _mesa_format_image_size64(format, width, height, depth);
   uint64_t mbytes = bytes / (1024 * 1024); /* convert to MB */
   return mbytes <= (uint64_t) ctx->Const.MaxTextureMbytes;
}


/**
 * Return true if the format is only valid for glCompressedTexImage.
 */
static GLboolean
compressedteximage_only_format(const struct gl_context *ctx, GLenum format)
{
   switch (format) {
   case GL_ETC1_RGB8_OES:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


/**
 * Helper function to determine whether a target and specific compression
 * format are supported.
 */
static GLboolean
target_can_be_compressed(const struct gl_context *ctx, GLenum target,
                         GLenum intFormat)
{
   (void) intFormat;  /* not used yet */

   switch (target) {
   case GL_TEXTURE_2D:
   case GL_PROXY_TEXTURE_2D:
      return GL_TRUE; /* true for any compressed format so far */
   case GL_PROXY_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
      return ctx->Extensions.ARB_texture_cube_map;
   case GL_PROXY_TEXTURE_2D_ARRAY_EXT:
   case GL_TEXTURE_2D_ARRAY_EXT:
      return (ctx->Extensions.MESA_texture_array ||
              ctx->Extensions.EXT_texture_array);
   default:
      return GL_FALSE;
   }      
}


/**
 * Check if the given texture target value is legal for a
 * glTexImage1/2/3D call.
 */
static GLboolean
legal_teximage_target(struct gl_context *ctx, GLuint dims, GLenum target)
{
   switch (dims) {
   case 1:
      switch (target) {
      case GL_TEXTURE_1D:
      case GL_PROXY_TEXTURE_1D:
         return GL_TRUE;
      default:
         return GL_FALSE;
      }
   case 2:
      switch (target) {
      case GL_TEXTURE_2D:
      case GL_PROXY_TEXTURE_2D:
         return GL_TRUE;
      case GL_PROXY_TEXTURE_CUBE_MAP:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
         return ctx->Extensions.ARB_texture_cube_map;
      case GL_TEXTURE_RECTANGLE_NV:
      case GL_PROXY_TEXTURE_RECTANGLE_NV:
         return ctx->Extensions.NV_texture_rectangle;
      case GL_TEXTURE_1D_ARRAY_EXT:
      case GL_PROXY_TEXTURE_1D_ARRAY_EXT:
         return (ctx->Extensions.MESA_texture_array ||
                 ctx->Extensions.EXT_texture_array);
      default:
         return GL_FALSE;
      }
   case 3:
      switch (target) {
      case GL_TEXTURE_3D:
      case GL_PROXY_TEXTURE_3D:
         return GL_TRUE;
      case GL_TEXTURE_2D_ARRAY_EXT:
      case GL_PROXY_TEXTURE_2D_ARRAY_EXT:
         return (ctx->Extensions.MESA_texture_array ||
                 ctx->Extensions.EXT_texture_array);
      default:
         return GL_FALSE;
      }
   default:
      _mesa_problem(ctx, "invalid dims=%u in legal_teximage_target()", dims);
      return GL_FALSE;
   }
}


/**
 * Check if the given texture target value is legal for a
 * glTexSubImage, glCopyTexSubImage or glCopyTexImage call.
 * The difference compared to legal_teximage_target() above is that
 * proxy targets are not supported.
 */
static GLboolean
legal_texsubimage_target(struct gl_context *ctx, GLuint dims, GLenum target)
{
   switch (dims) {
   case 1:
      return target == GL_TEXTURE_1D;
   case 2:
      switch (target) {
      case GL_TEXTURE_2D:
         return GL_TRUE;
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
         return ctx->Extensions.ARB_texture_cube_map;
      case GL_TEXTURE_RECTANGLE_NV:
         return ctx->Extensions.NV_texture_rectangle;
      case GL_TEXTURE_1D_ARRAY_EXT:
         return (ctx->Extensions.MESA_texture_array ||
                 ctx->Extensions.EXT_texture_array);
      default:
         return GL_FALSE;
      }
   case 3:
      switch (target) {
      case GL_TEXTURE_3D:
         return GL_TRUE;
      case GL_TEXTURE_2D_ARRAY_EXT:
         return (ctx->Extensions.MESA_texture_array ||
                 ctx->Extensions.EXT_texture_array);
      default:
         return GL_FALSE;
      }
   default:
      _mesa_problem(ctx, "invalid dims=%u in legal_texsubimage_target()",
                    dims);
      return GL_FALSE;
   }
}


/**
 * Helper function to determine if a texture object is mutable (in terms
 * of GL_ARB_texture_storage).
 */
static GLboolean
mutable_tex_object(struct gl_context *ctx, GLenum target)
{
   if (ctx->Extensions.ARB_texture_storage) {
      struct gl_texture_object *texObj =
         _mesa_get_current_tex_object(ctx, target);
      return !texObj->Immutable;
   }
   return GL_TRUE;
}



/**
 * Test the glTexImage[123]D() parameters for errors.
 * 
 * \param ctx GL context.
 * \param dimensions texture image dimensions (must be 1, 2 or 3).
 * \param target texture target given by the user.
 * \param level image level given by the user.
 * \param internalFormat internal format given by the user.
 * \param format pixel data format given by the user.
 * \param type pixel data type given by the user.
 * \param width image width given by the user.
 * \param height image height given by the user.
 * \param depth image depth given by the user.
 * \param border image border given by the user.
 * 
 * \return GL_TRUE if an error was detected, or GL_FALSE if no errors.
 *
 * Verifies each of the parameters against the constants specified in
 * __struct gl_contextRec::Const and the supported extensions, and according
 * to the OpenGL specification.
 */
static GLboolean
texture_error_check( struct gl_context *ctx,
                     GLuint dimensions, GLenum target,
                     GLint level, GLint internalFormat,
                     GLenum format, GLenum type,
                     GLint width, GLint height,
                     GLint depth, GLint border )
{
   const GLenum proxyTarget = get_proxy_target(target);
   const GLboolean isProxy = target == proxyTarget;
   GLboolean sizeOK = GL_TRUE;
   GLboolean colorFormat;
   GLenum err;

   /* Even though there are no color-index textures, we still have to support
    * uploading color-index data and remapping it to RGB via the
    * GL_PIXEL_MAP_I_TO_[RGBA] tables.
    */
   const GLboolean indexFormat = (format == GL_COLOR_INDEX);

   /* Basic level check (more checking in ctx->Driver.TestProxyTexImage) */
   if (level < 0 || level >= MAX_TEXTURE_LEVELS) {
      if (!isProxy) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glTexImage%dD(level=%d)", dimensions, level);
      }
      return GL_TRUE;
   }

   /* Check border */
   if (border < 0 || border > 1 ||
       ((target == GL_TEXTURE_RECTANGLE_NV ||
         target == GL_PROXY_TEXTURE_RECTANGLE_NV) && border != 0)) {
      if (!isProxy) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glTexImage%dD(border=%d)", dimensions, border);
      }
      return GL_TRUE;
   }

   if (width < 0 || height < 0 || depth < 0) {
      if (!isProxy) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glTexImage%dD(width, height or depth < 0)", dimensions);
      }
      return GL_TRUE;
   }

   /* Do this simple check before calling the TestProxyTexImage() function */
   if (proxyTarget == GL_PROXY_TEXTURE_CUBE_MAP_ARB) {
      sizeOK = (width == height);
   }

   /*
    * Use the proxy texture driver hook to see if the size/level/etc are
    * legal.
    */
   sizeOK = sizeOK && ctx->Driver.TestProxyTexImage(ctx, proxyTarget, level,
                                                    internalFormat, format,
                                                    type, width, height,
                                                    depth, border);
   if (!sizeOK) {
      if (!isProxy) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glTexImage%dD(level=%d, width=%d, height=%d, depth=%d)",
                     dimensions, level, width, height, depth);
      }
      return GL_TRUE;
   }

   /* Check internalFormat */
   if (_mesa_base_tex_format(ctx, internalFormat) < 0) {
      if (!isProxy) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glTexImage%dD(internalFormat=%s)",
                     dimensions, _mesa_lookup_enum_by_nr(internalFormat));
      }
      return GL_TRUE;
   }

   /* Check incoming image format and type */
   err = _mesa_error_check_format_and_type(ctx, format, type);
   if (err != GL_NO_ERROR) {
      if (!isProxy) {
         _mesa_error(ctx, err,
                     "glTexImage%dD(incompatible format 0x%x, type 0x%x)",
                     dimensions, format, type);
      }
      return GL_TRUE;
   }

   /* make sure internal format and format basically agree */
   colorFormat = _mesa_is_color_format(format);
   if ((_mesa_is_color_format(internalFormat) && !colorFormat && !indexFormat) ||
       (_mesa_is_depth_format(internalFormat) != _mesa_is_depth_format(format)) ||
       (_mesa_is_ycbcr_format(internalFormat) != _mesa_is_ycbcr_format(format)) ||
       (_mesa_is_depthstencil_format(internalFormat) != _mesa_is_depthstencil_format(format)) ||
       (_mesa_is_dudv_format(internalFormat) != _mesa_is_dudv_format(format))) {
      if (!isProxy)
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glTexImage%dD(incompatible internalFormat 0x%x, format 0x%x)",
                     dimensions, internalFormat, format);
      return GL_TRUE;
   }

   /* additional checks for ycbcr textures */
   if (internalFormat == GL_YCBCR_MESA) {
      ASSERT(ctx->Extensions.MESA_ycbcr_texture);
      if (type != GL_UNSIGNED_SHORT_8_8_MESA &&
          type != GL_UNSIGNED_SHORT_8_8_REV_MESA) {
         char message[100];
         _mesa_snprintf(message, sizeof(message),
                        "glTexImage%dD(format/type YCBCR mismatch", dimensions);
         _mesa_error(ctx, GL_INVALID_ENUM, "%s", message);
         return GL_TRUE; /* error */
      }
      if (target != GL_TEXTURE_2D &&
          target != GL_PROXY_TEXTURE_2D &&
          target != GL_TEXTURE_RECTANGLE_NV &&
          target != GL_PROXY_TEXTURE_RECTANGLE_NV) {
         if (!isProxy)
            _mesa_error(ctx, GL_INVALID_ENUM, "glTexImage(target)");
         return GL_TRUE;
      }
      if (border != 0) {
         if (!isProxy) {
            char message[100];
            _mesa_snprintf(message, sizeof(message),
                           "glTexImage%dD(format=GL_YCBCR_MESA and border=%d)",
                           dimensions, border);
            _mesa_error(ctx, GL_INVALID_VALUE, "%s", message);
         }
         return GL_TRUE;
      }
   }

   /* additional checks for depth textures */
   if (_mesa_base_tex_format(ctx, internalFormat) == GL_DEPTH_COMPONENT) {
      /* Only 1D, 2D, rect, array and cube textures supported, not 3D
       * Cubemaps are only supported for GL version > 3.0 or with EXT_gpu_shader4 */
      if (target != GL_TEXTURE_1D &&
          target != GL_PROXY_TEXTURE_1D &&
          target != GL_TEXTURE_2D &&
          target != GL_PROXY_TEXTURE_2D &&
          target != GL_TEXTURE_1D_ARRAY &&
          target != GL_PROXY_TEXTURE_1D_ARRAY &&
          target != GL_TEXTURE_2D_ARRAY &&
          target != GL_PROXY_TEXTURE_2D_ARRAY &&
          target != GL_TEXTURE_RECTANGLE_ARB &&
          target != GL_PROXY_TEXTURE_RECTANGLE_ARB &&
         !((_mesa_is_cube_face(target) || target == GL_PROXY_TEXTURE_CUBE_MAP) &&
           (ctx->VersionMajor >= 3 || ctx->Extensions.EXT_gpu_shader4))) {
         if (!isProxy)
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexImage(target/internalFormat)");
         return GL_TRUE;
      }
   }

   /* additional checks for compressed textures */
   if (_mesa_is_compressed_format(ctx, internalFormat) ||
       is_generic_compressed_format(internalFormat)) {
      if (!target_can_be_compressed(ctx, target, internalFormat)) {
         if (!isProxy)
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexImage%dD(target)", dimensions);
         return GL_TRUE;
      }
      if (compressedteximage_only_format(ctx, internalFormat)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
               "glTexImage%dD(no compression for format)", dimensions);
         return GL_TRUE;
      }
      if (border != 0) {
         if (!isProxy) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glTexImage%dD(border!=0)", dimensions);
         }
         return GL_TRUE;
      }
   }

   /* additional checks for integer textures */
   if ((ctx->VersionMajor >= 3 || ctx->Extensions.EXT_texture_integer) &&
       (_mesa_is_integer_format(format) !=
        _mesa_is_integer_format(internalFormat))) {
      if (!isProxy) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glTexImage%dD(integer/non-integer format mismatch)",
                     dimensions);
      }
      return GL_TRUE;
   }

   if (!mutable_tex_object(ctx, target)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glTexImage%dD(immutable texture)", dimensions);
      return GL_TRUE;
   }

   /* if we get here, the parameters are OK */
   return GL_FALSE;
}


/**
 * Test glTexSubImage[123]D() parameters for errors.
 * 
 * \param ctx GL context.
 * \param dimensions texture image dimensions (must be 1, 2 or 3).
 * \param target texture target given by the user.
 * \param level image level given by the user.
 * \param xoffset sub-image x offset given by the user.
 * \param yoffset sub-image y offset given by the user.
 * \param zoffset sub-image z offset given by the user.
 * \param format pixel data format given by the user.
 * \param type pixel data type given by the user.
 * \param width image width given by the user.
 * \param height image height given by the user.
 * \param depth image depth given by the user.
 * 
 * \return GL_TRUE if an error was detected, or GL_FALSE if no errors.
 *
 * Verifies each of the parameters against the constants specified in
 * __struct gl_contextRec::Const and the supported extensions, and according
 * to the OpenGL specification.
 */
static GLboolean
subtexture_error_check( struct gl_context *ctx, GLuint dimensions,
                        GLenum target, GLint level,
                        GLint xoffset, GLint yoffset, GLint zoffset,
                        GLint width, GLint height, GLint depth,
                        GLenum format, GLenum type )
{
   GLenum err;

   /* Basic level check */
   if (level < 0 || level >= MAX_TEXTURE_LEVELS) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glTexSubImage2D(level=%d)", level);
      return GL_TRUE;
   }

   /* Check for negative sizes */
   if (width < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glTexSubImage%dD(width=%d)", dimensions, width);
      return GL_TRUE;
   }
   if (height < 0 && dimensions > 1) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glTexSubImage%dD(height=%d)", dimensions, height);
      return GL_TRUE;
   }
   if (depth < 0 && dimensions > 2) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glTexSubImage%dD(depth=%d)", dimensions, depth);
      return GL_TRUE;
   }

   err = _mesa_error_check_format_and_type(ctx, format, type);
   if (err != GL_NO_ERROR) {
      _mesa_error(ctx, err,
                  "glTexSubImage%dD(incompatible format 0x%x, type 0x%x)",
                  dimensions, format, type);
      return GL_TRUE;
   }

   return GL_FALSE;
}


/**
 * Do second part of glTexSubImage which depends on the destination texture.
 * \return GL_TRUE if error recorded, GL_FALSE otherwise
 */
static GLboolean
subtexture_error_check2( struct gl_context *ctx, GLuint dimensions,
			 GLenum target, GLint level,
			 GLint xoffset, GLint yoffset, GLint zoffset,
			 GLint width, GLint height, GLint depth,
			 GLenum format, GLenum type,
			 const struct gl_texture_image *destTex )
{
   if (!destTex) {
      /* undefined image level */
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTexSubImage%dD", dimensions);
      return GL_TRUE;
   }

   if (xoffset < -((GLint)destTex->Border)) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glTexSubImage%dD(xoffset)",
                  dimensions);
      return GL_TRUE;
   }
   if (xoffset + width > (GLint) (destTex->Width + destTex->Border)) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glTexSubImage%dD(xoffset+width)",
                  dimensions);
      return GL_TRUE;
   }
   if (dimensions > 1) {
      if (yoffset < -((GLint)destTex->Border)) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glTexSubImage%dD(yoffset)",
                     dimensions);
         return GL_TRUE;
      }
      if (yoffset + height > (GLint) (destTex->Height + destTex->Border)) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glTexSubImage%dD(yoffset+height)",
                     dimensions);
         return GL_TRUE;
      }
   }
   if (dimensions > 2) {
      if (zoffset < -((GLint)destTex->Border)) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glTexSubImage3D(zoffset)");
         return GL_TRUE;
      }
      if (zoffset + depth  > (GLint) (destTex->Depth + destTex->Border)) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glTexSubImage3D(zoffset+depth)");
         return GL_TRUE;
      }
   }

   if (_mesa_is_format_compressed(destTex->TexFormat)) {
      GLuint bw, bh;

      if (compressedteximage_only_format(ctx, destTex->InternalFormat)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
               "glTexSubImage%dD(no compression for format)", dimensions);
         return GL_TRUE;
      }

      /* do tests which depend on compression block size */
      _mesa_get_format_block_size(destTex->TexFormat, &bw, &bh);

      /* offset must be multiple of block size */
      if ((xoffset % bw != 0) || (yoffset % bh != 0)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glTexSubImage%dD(xoffset = %d, yoffset = %d)",
                     dimensions, xoffset, yoffset);
         return GL_TRUE;
      }
      /* size must be multiple of bw by bh or equal to whole texture size */
      if ((width % bw != 0) && (GLuint) width != destTex->Width) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glTexSubImage%dD(width = %d)", dimensions, width);
         return GL_TRUE;
      }         
      if ((height % bh != 0) && (GLuint) height != destTex->Height) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glTexSubImage%dD(height = %d)", dimensions, height);
         return GL_TRUE;
      }         
   }

   if (ctx->VersionMajor >= 3 || ctx->Extensions.EXT_texture_integer) {
      /* both source and dest must be integer-valued, or neither */
      if (_mesa_is_format_integer_color(destTex->TexFormat) !=
          _mesa_is_integer_format(format)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glTexSubImage%dD(integer/non-integer format mismatch)",
                     dimensions);
         return GL_TRUE;
      }
   }

   return GL_FALSE;
}


/**
 * Test glCopyTexImage[12]D() parameters for errors.
 * 
 * \param ctx GL context.
 * \param dimensions texture image dimensions (must be 1, 2 or 3).
 * \param target texture target given by the user.
 * \param level image level given by the user.
 * \param internalFormat internal format given by the user.
 * \param width image width given by the user.
 * \param height image height given by the user.
 * \param border texture border.
 * 
 * \return GL_TRUE if an error was detected, or GL_FALSE if no errors.
 * 
 * Verifies each of the parameters against the constants specified in
 * __struct gl_contextRec::Const and the supported extensions, and according
 * to the OpenGL specification.
 */
static GLboolean
copytexture_error_check( struct gl_context *ctx, GLuint dimensions,
                         GLenum target, GLint level, GLint internalFormat,
                         GLint width, GLint height, GLint border )
{
   const GLenum proxyTarget = get_proxy_target(target);
   const GLenum type = GL_FLOAT;
   GLboolean sizeOK;
   GLint baseFormat;

   /* check target */
   if (!legal_texsubimage_target(ctx, dimensions, target)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glCopyTexImage%uD(target=%s)",
                  dimensions, _mesa_lookup_enum_by_nr(target));
      return GL_TRUE;
   }       

   /* Basic level check (more checking in ctx->Driver.TestProxyTexImage) */
   if (level < 0 || level >= MAX_TEXTURE_LEVELS) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyTexImage%dD(level=%d)", dimensions, level);
      return GL_TRUE;
   }

   /* Check that the source buffer is complete */
   if (ctx->ReadBuffer->Name) {
      if (ctx->ReadBuffer->_Status == 0) {
         _mesa_test_framebuffer_completeness(ctx, ctx->ReadBuffer);
      }
      if (ctx->ReadBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
         _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                     "glCopyTexImage%dD(invalid readbuffer)", dimensions);
         return GL_TRUE;
      }

      if (ctx->ReadBuffer->Visual.samples > 0) {
	 _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION,
		     "glCopyTexImage%dD(multisample FBO)",
		     dimensions);
	 return GL_TRUE;
      }
   }

   /* Check border */
   if (border < 0 || border > 1 ||
       ((target == GL_TEXTURE_RECTANGLE_NV ||
         target == GL_PROXY_TEXTURE_RECTANGLE_NV) && border != 0)) {
      return GL_TRUE;
   }

   baseFormat = _mesa_base_tex_format(ctx, internalFormat);
   if (baseFormat < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyTexImage%dD(internalFormat)", dimensions);
      return GL_TRUE;
   }

   if (!_mesa_source_buffer_exists(ctx, baseFormat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCopyTexImage%dD(missing readbuffer)", dimensions);
      return GL_TRUE;
   }

   /* From the EXT_texture_integer spec:
    *
    *     "INVALID_OPERATION is generated by CopyTexImage* and CopyTexSubImage*
    *      if the texture internalformat is an integer format and the read color
    *      buffer is not an integer format, or if the internalformat is not an
    *      integer format and the read color buffer is an integer format."
    */
   if (_mesa_is_color_format(internalFormat)) {
      struct gl_renderbuffer *rb = ctx->ReadBuffer->_ColorReadBuffer;

      if (_mesa_is_integer_format(rb->InternalFormat) !=
	  _mesa_is_integer_format(internalFormat)) {
	 _mesa_error(ctx, GL_INVALID_OPERATION,
		     "glCopyTexImage%dD(integer vs non-integer)", dimensions);
	 return GL_TRUE;
      }
   }

   /* Do size, level checking */
   sizeOK = (proxyTarget == GL_PROXY_TEXTURE_CUBE_MAP_ARB)
      ? (width == height) : 1;

   sizeOK = sizeOK && ctx->Driver.TestProxyTexImage(ctx, proxyTarget, level,
                                                    internalFormat, baseFormat,
                                                    type, width, height,
                                                    1, border);

   if (!sizeOK) {
      if (dimensions == 1) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCopyTexImage1D(width=%d)", width);
      }
      else {
         ASSERT(dimensions == 2);
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCopyTexImage2D(width=%d, height=%d)", width, height);
      }
      return GL_TRUE;
   }

   if (_mesa_is_compressed_format(ctx, internalFormat) ||
       is_generic_compressed_format(internalFormat)) {
      if (!target_can_be_compressed(ctx, target, internalFormat)) {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glCopyTexImage%dD(target)", dimensions);
         return GL_TRUE;
      }
      if (compressedteximage_only_format(ctx, internalFormat)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
               "glCopyTexImage%dD(no compression for format)", dimensions);
         return GL_TRUE;
      }
      if (border != 0) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glCopyTexImage%dD(border!=0)", dimensions);
         return GL_TRUE;
      }
   }

   if (!mutable_tex_object(ctx, target)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCopyTexImage%dD(immutable texture)", dimensions);
      return GL_TRUE;
   }

   /* if we get here, the parameters are OK */
   return GL_FALSE;
}


/**
 * Test glCopyTexSubImage[12]D() parameters for errors.
 * Note that this is the first part of error checking.
 * See also copytexsubimage_error_check2() below for the second part.
 * 
 * \param ctx GL context.
 * \param dimensions texture image dimensions (must be 1, 2 or 3).
 * \param target texture target given by the user.
 * \param level image level given by the user.
 * 
 * \return GL_TRUE if an error was detected, or GL_FALSE if no errors.
 */
static GLboolean
copytexsubimage_error_check1( struct gl_context *ctx, GLuint dimensions,
                              GLenum target, GLint level)
{
   /* Check that the source buffer is complete */
   if (ctx->ReadBuffer->Name) {
      if (ctx->ReadBuffer->_Status == 0) {
         _mesa_test_framebuffer_completeness(ctx, ctx->ReadBuffer);
      }
      if (ctx->ReadBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
         _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                     "glCopyTexImage%dD(invalid readbuffer)", dimensions);
         return GL_TRUE;
      }

      if (ctx->ReadBuffer->Visual.samples > 0) {
	 _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION,
		     "glCopyTexSubImage%dD(multisample FBO)",
		     dimensions);
	 return GL_TRUE;
      }
   }

   /* check target (proxies not allowed) */
   if (!legal_texsubimage_target(ctx, dimensions, target)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glCopyTexSubImage%uD(target=%s)",
                  dimensions, _mesa_lookup_enum_by_nr(target));
      return GL_TRUE;
   }

   /* Check level */
   if (level < 0 || level >= MAX_TEXTURE_LEVELS) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyTexSubImage%dD(level=%d)", dimensions, level);
      return GL_TRUE;
   }

   return GL_FALSE;
}


/**
 * Second part of error checking for glCopyTexSubImage[12]D().
 * \param xoffset sub-image x offset given by the user.
 * \param yoffset sub-image y offset given by the user.
 * \param zoffset sub-image z offset given by the user.
 * \param width image width given by the user.
 * \param height image height given by the user.
 */
static GLboolean
copytexsubimage_error_check2( struct gl_context *ctx, GLuint dimensions,
			      GLenum target, GLint level,
			      GLint xoffset, GLint yoffset, GLint zoffset,
			      GLsizei width, GLsizei height,
			      const struct gl_texture_image *teximage )
{
   /* check that dest tex image exists */
   if (!teximage) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCopyTexSubImage%dD(undefined texture level: %d)",
                  dimensions, level);
      return GL_TRUE;
   }

   /* Check size */
   if (width < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyTexSubImage%dD(width=%d)", dimensions, width);
      return GL_TRUE;
   }
   if (dimensions > 1 && height < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyTexSubImage%dD(height=%d)", dimensions, height);
      return GL_TRUE;
   }

   /* check x/y offsets */
   if (xoffset < -((GLint)teximage->Border)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyTexSubImage%dD(xoffset=%d)", dimensions, xoffset);
      return GL_TRUE;
   }
   if (xoffset + width > (GLint) (teximage->Width + teximage->Border)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCopyTexSubImage%dD(xoffset+width)", dimensions);
      return GL_TRUE;
   }
   if (dimensions > 1) {
      if (yoffset < -((GLint)teximage->Border)) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCopyTexSubImage%dD(yoffset=%d)", dimensions, yoffset);
         return GL_TRUE;
      }
      /* NOTE: we're adding the border here, not subtracting! */
      if (yoffset + height > (GLint) (teximage->Height + teximage->Border)) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCopyTexSubImage%dD(yoffset+height)", dimensions);
         return GL_TRUE;
      }
   }

   /* check z offset */
   if (dimensions > 2) {
      if (zoffset < -((GLint)teximage->Border)) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCopyTexSubImage%dD(zoffset)", dimensions);
         return GL_TRUE;
      }
      if (zoffset > (GLint) (teximage->Depth + teximage->Border)) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCopyTexSubImage%dD(zoffset+depth)", dimensions);
         return GL_TRUE;
      }
   }

   if (_mesa_is_format_compressed(teximage->TexFormat)) {
      if (compressedteximage_only_format(ctx, teximage->InternalFormat)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
               "glCopyTexSubImage%dD(no compression for format)", dimensions);
         return GL_TRUE;
      }
      /* offset must be multiple of 4 */
      if ((xoffset & 3) || (yoffset & 3)) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCopyTexSubImage%dD(xoffset or yoffset)", dimensions);
         return GL_TRUE;
      }
      /* size must be multiple of 4 */
      if ((width & 3) != 0 && (GLuint) width != teximage->Width) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCopyTexSubImage%dD(width)", dimensions);
         return GL_TRUE;
      }         
      if ((height & 3) != 0 && (GLuint) height != teximage->Height) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCopyTexSubImage%dD(height)", dimensions);
         return GL_TRUE;
      }         
   }

   if (teximage->InternalFormat == GL_YCBCR_MESA) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glCopyTexSubImage2D");
      return GL_TRUE;
   }

   if (!_mesa_source_buffer_exists(ctx, teximage->_BaseFormat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCopyTexSubImage%dD(missing readbuffer, format=0x%x)",
                  dimensions, teximage->_BaseFormat);
      return GL_TRUE;
   }

   /* From the EXT_texture_integer spec:
    *
    *     "INVALID_OPERATION is generated by CopyTexImage* and CopyTexSubImage*
    *      if the texture internalformat is an integer format and the read color
    *      buffer is not an integer format, or if the internalformat is not an
    *      integer format and the read color buffer is an integer format."
    */
   if (_mesa_is_color_format(teximage->InternalFormat)) {
      struct gl_renderbuffer *rb = ctx->ReadBuffer->_ColorReadBuffer;

      if (_mesa_is_format_integer_color(rb->Format) !=
	  _mesa_is_format_integer_color(teximage->TexFormat)) {
	 _mesa_error(ctx, GL_INVALID_OPERATION,
		     "glCopyTexImage%dD(integer vs non-integer)", dimensions);
	 return GL_TRUE;
      }
   }

   /* if we get here, the parameters are OK */
   return GL_FALSE;
}


/** Callback info for walking over FBO hash table */
struct cb_info
{
   struct gl_context *ctx;
   struct gl_texture_object *texObj;
   GLuint level, face;
};


/**
 * Check render to texture callback.  Called from _mesa_HashWalk().
 */
static void
check_rtt_cb(GLuint key, void *data, void *userData)
{
   struct gl_framebuffer *fb = (struct gl_framebuffer *) data;
   const struct cb_info *info = (struct cb_info *) userData;
   struct gl_context *ctx = info->ctx;
   const struct gl_texture_object *texObj = info->texObj;
   const GLuint level = info->level, face = info->face;

   /* If this is a user-created FBO */
   if (fb->Name) {
      GLuint i;
      /* check if any of the FBO's attachments point to 'texObj' */
      for (i = 0; i < BUFFER_COUNT; i++) {
         struct gl_renderbuffer_attachment *att = fb->Attachment + i;
         if (att->Type == GL_TEXTURE &&
             att->Texture == texObj &&
             att->TextureLevel == level &&
             att->CubeMapFace == face) {
            ASSERT(_mesa_get_attachment_teximage(att));
            /* Tell driver about the new renderbuffer texture */
            ctx->Driver.RenderTexture(ctx, ctx->DrawBuffer, att);
            /* Mark fb status as indeterminate to force re-validation */
            fb->_Status = 0;
         }
      }
   }
}


/**
 * When a texture image is specified we have to check if it's bound to
 * any framebuffer objects (render to texture) in order to detect changes
 * in size or format since that effects FBO completeness.
 * Any FBOs rendering into the texture must be re-validated.
 */
void
_mesa_update_fbo_texture(struct gl_context *ctx,
                         struct gl_texture_object *texObj,
                         GLuint face, GLuint level)
{
   /* Only check this texture if it's been marked as RenderToTexture */
   if (texObj->_RenderToTexture) {
      struct cb_info info;
      info.ctx = ctx;
      info.texObj = texObj;
      info.level = level;
      info.face = face;
      _mesa_HashWalk(ctx->Shared->FrameBuffers, check_rtt_cb, &info);
   }
}


/**
 * If the texture object's GenerateMipmap flag is set and we've
 * changed the texture base level image, regenerate the rest of the
 * mipmap levels now.
 */
static inline void
check_gen_mipmap(struct gl_context *ctx, GLenum target,
                 struct gl_texture_object *texObj, GLint level)
{
   ASSERT(target != GL_TEXTURE_CUBE_MAP);
   if (texObj->GenerateMipmap &&
       level == texObj->BaseLevel &&
       level < texObj->MaxLevel) {
      ASSERT(ctx->Driver.GenerateMipmap);
      ctx->Driver.GenerateMipmap(ctx, target, texObj);
   }
}


/** Debug helper: override the user-requested internal format */
static GLenum
override_internal_format(GLenum internalFormat, GLint width, GLint height)
{
#if 0
   if (internalFormat == GL_RGBA16F_ARB ||
       internalFormat == GL_RGBA32F_ARB) {
      printf("Convert rgba float tex to int %d x %d\n", width, height);
      return GL_RGBA;
   }
   else if (internalFormat == GL_RGB16F_ARB ||
            internalFormat == GL_RGB32F_ARB) {
      printf("Convert rgb float tex to int %d x %d\n", width, height);
      return GL_RGB;
   }
   else if (internalFormat == GL_LUMINANCE_ALPHA16F_ARB ||
            internalFormat == GL_LUMINANCE_ALPHA32F_ARB) {
      printf("Convert luminance float tex to int %d x %d\n", width, height);
      return GL_LUMINANCE_ALPHA;
   }
   else if (internalFormat == GL_LUMINANCE16F_ARB ||
            internalFormat == GL_LUMINANCE32F_ARB) {
      printf("Convert luminance float tex to int %d x %d\n", width, height);
      return GL_LUMINANCE;
   }
   else if (internalFormat == GL_ALPHA16F_ARB ||
            internalFormat == GL_ALPHA32F_ARB) {
      printf("Convert luminance float tex to int %d x %d\n", width, height);
      return GL_ALPHA;
   }
   /*
   else if (internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) {
      internalFormat = GL_RGBA;
   }
   */
   else {
      return internalFormat;
   }
#else
   return internalFormat;
#endif
}


/**
 * Choose the actual hardware format for a texture image.
 * Try to use the same format as the previous image level when possible.
 * Otherwise, ask the driver for the best format.
 * It's important to try to choose a consistant format for all levels
 * for efficient texture memory layout/allocation.  In particular, this
 * comes up during automatic mipmap generation.
 */
gl_format
_mesa_choose_texture_format(struct gl_context *ctx,
                            struct gl_texture_object *texObj,
                            GLenum target, GLint level,
                            GLenum internalFormat, GLenum format, GLenum type)
{
   gl_format f;

   /* see if we've already chosen a format for the previous level */
   if (level > 0) {
      struct gl_texture_image *prevImage =
	 _mesa_select_tex_image(ctx, texObj, target, level - 1);
      /* See if the prev level is defined and has an internal format which
       * matches the new internal format.
       */
      if (prevImage &&
          prevImage->Width > 0 &&
          prevImage->InternalFormat == internalFormat) {
         /* use the same format */
         ASSERT(prevImage->TexFormat != MESA_FORMAT_NONE);
         return prevImage->TexFormat;
      }
   }

   /* choose format from scratch */
   f = ctx->Driver.ChooseTextureFormat(ctx, internalFormat, format, type);
   ASSERT(f != MESA_FORMAT_NONE);
   return f;
}

/**
 * Adjust pixel unpack params and image dimensions to strip off the
 * texture border.
 *
 * Gallium and intel don't support texture borders.  They've seldem been used
 * and seldom been implemented correctly anyway.
 *
 * \param unpackNew returns the new pixel unpack parameters
 */
static void
strip_texture_border(GLint *border,
                     GLint *width, GLint *height, GLint *depth,
                     const struct gl_pixelstore_attrib *unpack,
                     struct gl_pixelstore_attrib *unpackNew)
{
   assert(*border > 0);  /* sanity check */

   *unpackNew = *unpack;

   if (unpackNew->RowLength == 0)
      unpackNew->RowLength = *width;

   if (depth && unpackNew->ImageHeight == 0)
      unpackNew->ImageHeight = *height;

   unpackNew->SkipPixels += *border;
   if (height)
      unpackNew->SkipRows += *border;
   if (depth)
      unpackNew->SkipImages += *border;

   assert(*width >= 3);
   *width = *width - 2 * *border;
   if (height && *height >= 3)
      *height = *height - 2 * *border;
   if (depth && *depth >= 3)
      *depth = *depth - 2 * *border;
   *border = 0;
}

/**
 * Common code to implement all the glTexImage1D/2D/3D functions.
 */
static void
teximage(struct gl_context *ctx, GLuint dims,
         GLenum target, GLint level, GLint internalFormat,
         GLsizei width, GLsizei height, GLsizei depth,
         GLint border, GLenum format, GLenum type,
         const GLvoid *pixels)
{
   GLboolean error;
   struct gl_pixelstore_attrib unpack_no_border;
   const struct gl_pixelstore_attrib *unpack = &ctx->Unpack;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glTexImage%uD %s %d %s %d %d %d %d %s %s %p\n",
                  dims,
                  _mesa_lookup_enum_by_nr(target), level,
                  _mesa_lookup_enum_by_nr(internalFormat),
                  width, height, depth, border,
                  _mesa_lookup_enum_by_nr(format),
                  _mesa_lookup_enum_by_nr(type), pixels);

   internalFormat = override_internal_format(internalFormat, width, height);

   /* target error checking */
   if (!legal_teximage_target(ctx, dims, target)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glTexImage%uD(target=%s)",
                  dims, _mesa_lookup_enum_by_nr(target));
      return;
   }

   /* general error checking */
   error = texture_error_check(ctx, dims, target, level, internalFormat,
                               format, type, width, height, depth, border);

   if (_mesa_is_proxy_texture(target)) {
      /* Proxy texture: just clear or set state depending on error checking */
      struct gl_texture_image *texImage =
         _mesa_get_proxy_tex_image(ctx, target, level);

      if (error) {
         /* when error, clear all proxy texture image parameters */
         if (texImage)
            clear_teximage_fields(texImage);
      }
      else {
         /* no error, set the tex image parameters */
         struct gl_texture_object *texObj =
            _mesa_get_current_tex_object(ctx, target);
         gl_format texFormat = _mesa_choose_texture_format(ctx, texObj,
                                                           target, level,
                                                           internalFormat,
                                                           format, type);

         if (legal_texture_size(ctx, texFormat, width, height, depth)) {
            _mesa_init_teximage_fields(ctx, texImage, width, height,
                                       depth, border, internalFormat,
                                       texFormat);
         }
         else if (texImage) {
            clear_teximage_fields(texImage);
         }
      }
   }
   else {
      /* non-proxy target */
      const GLuint face = _mesa_tex_target_to_face(target);
      struct gl_texture_object *texObj;
      struct gl_texture_image *texImage;

      if (error) {
         return;   /* error was recorded */
      }

      /* Allow a hardware driver to just strip out the border, to provide
       * reliable but slightly incorrect hardware rendering instead of
       * rarely-tested software fallback rendering.
       */
      if (border && ctx->Const.StripTextureBorder) {
	 strip_texture_border(&border, &width, &height, &depth, unpack,
			      &unpack_no_border);
	 unpack = &unpack_no_border;
      }

      if (ctx->NewState & _NEW_PIXEL)
	 _mesa_update_state(ctx);

      texObj = _mesa_get_current_tex_object(ctx, target);

      _mesa_lock_texture(ctx, texObj);
      {
	 texImage = _mesa_get_tex_image(ctx, texObj, target, level);

	 if (!texImage) {
	    _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage%uD", dims);
	 }
         else {
            gl_format texFormat;

            ctx->Driver.FreeTextureImageBuffer(ctx, texImage);

            texFormat = _mesa_choose_texture_format(ctx, texObj, target, level,
                                                    internalFormat, format,
                                                    type);

            if (legal_texture_size(ctx, texFormat, width, height, depth)) {
               _mesa_init_teximage_fields(ctx, texImage,
                                          width, height, depth,
                                          border, internalFormat, texFormat);

               /* Give the texture to the driver.  <pixels> may be null. */
               ASSERT(ctx->Driver.TexImage3D);
               switch (dims) {
               case 1:
                  ctx->Driver.TexImage1D(ctx, texImage, internalFormat,
                                         width, border, format,
                                         type, pixels, unpack);
                  break;
               case 2:
                  ctx->Driver.TexImage2D(ctx, texImage, internalFormat,
                                         width, height, border, format,
                                         type, pixels, unpack);
                  break;
               case 3:
                  ctx->Driver.TexImage3D(ctx, texImage, internalFormat,
                                         width, height, depth, border, format,
                                         type, pixels, unpack);
                  break;
               default:
                  _mesa_problem(ctx, "invalid dims=%u in teximage()", dims);
               }

               check_gen_mipmap(ctx, target, texObj, level);

               _mesa_update_fbo_texture(ctx, texObj, face, level);

               /* state update */
               texObj->_Complete = GL_FALSE;
               ctx->NewState |= _NEW_TEXTURE;
            }
            else {
               _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage%uD", dims);
            }
         }
      }
      _mesa_unlock_texture(ctx, texObj);
   }
}


/*
 * Called from the API.  Note that width includes the border.
 */
void GLAPIENTRY
_mesa_TexImage1D( GLenum target, GLint level, GLint internalFormat,
                  GLsizei width, GLint border, GLenum format,
                  GLenum type, const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   teximage(ctx, 1, target, level, internalFormat, width, 1, 1,
            border, format, type, pixels);
}


void GLAPIENTRY
_mesa_TexImage2D( GLenum target, GLint level, GLint internalFormat,
                  GLsizei width, GLsizei height, GLint border,
                  GLenum format, GLenum type,
                  const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   teximage(ctx, 2, target, level, internalFormat, width, height, 1,
            border, format, type, pixels);
}


/*
 * Called by the API or display list executor.
 * Note that width and height include the border.
 */
void GLAPIENTRY
_mesa_TexImage3D( GLenum target, GLint level, GLint internalFormat,
                  GLsizei width, GLsizei height, GLsizei depth,
                  GLint border, GLenum format, GLenum type,
                  const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   teximage(ctx, 3, target, level, internalFormat, width, height, depth,
            border, format, type, pixels);
}


void GLAPIENTRY
_mesa_TexImage3DEXT( GLenum target, GLint level, GLenum internalFormat,
                     GLsizei width, GLsizei height, GLsizei depth,
                     GLint border, GLenum format, GLenum type,
                     const GLvoid *pixels )
{
   _mesa_TexImage3D(target, level, (GLint) internalFormat, width, height,
                    depth, border, format, type, pixels);
}


#if FEATURE_OES_EGL_image
void GLAPIENTRY
_mesa_EGLImageTargetTexture2DOES (GLenum target, GLeglImageOES image)
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if ((target == GL_TEXTURE_2D &&
        !ctx->Extensions.OES_EGL_image) ||
       (target == GL_TEXTURE_EXTERNAL_OES &&
        !ctx->Extensions.OES_EGL_image_external)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
		  "glEGLImageTargetTexture2D(target=%d)", target);
      return;
   }

   if (ctx->NewState & _NEW_PIXEL)
      _mesa_update_state(ctx);

   texObj = _mesa_get_current_tex_object(ctx, target);
   _mesa_lock_texture(ctx, texObj);

   texImage = _mesa_get_tex_image(ctx, texObj, target, 0);
   if (!texImage) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glEGLImageTargetTexture2D");
   } else {
      ctx->Driver.FreeTextureImageBuffer(ctx, texImage);

      ctx->Driver.EGLImageTargetTexture2D(ctx, target,
					  texObj, texImage, image);

      /* state update */
      texObj->_Complete = GL_FALSE;
      ctx->NewState |= _NEW_TEXTURE;
   }
   _mesa_unlock_texture(ctx, texObj);

}
#endif



/**
 * Implement all the glTexSubImage1/2/3D() functions.
 */
static void
texsubimage(struct gl_context *ctx, GLuint dims, GLenum target, GLint level,
            GLint xoffset, GLint yoffset, GLint zoffset,
            GLsizei width, GLsizei height, GLsizei depth,
            GLenum format, GLenum type, const GLvoid *pixels )
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glTexSubImage%uD %s %d %d %d %d %d %d %d %s %s %p\n",
                  dims,
                  _mesa_lookup_enum_by_nr(target), level,
                  xoffset, yoffset, zoffset, width, height, depth,
                  _mesa_lookup_enum_by_nr(format),
                  _mesa_lookup_enum_by_nr(type), pixels);

   /* check target (proxies not allowed) */
   if (!legal_texsubimage_target(ctx, dims, target)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glTexSubImage%uD(target=%s)",
                  dims, _mesa_lookup_enum_by_nr(target));
      return;
   }       

   if (ctx->NewState & _NEW_PIXEL)
      _mesa_update_state(ctx);

   if (subtexture_error_check(ctx, dims, target, level, xoffset, yoffset, zoffset,
                              width, height, depth, format, type)) {
      return;   /* error was detected */
   }

   texObj = _mesa_get_current_tex_object(ctx, target);

   _mesa_lock_texture(ctx, texObj);
   {
      texImage = _mesa_select_tex_image(ctx, texObj, target, level);

      if (subtexture_error_check2(ctx, dims, target, level,
                                  xoffset, yoffset, zoffset,
				  width, height, depth,
                                  format, type, texImage)) {
         /* error was recorded */
      }
      else if (width > 0 && height > 0 && depth > 0) {
         /* If we have a border, offset=-1 is legal.  Bias by border width. */
         switch (dims) {
         case 3:
            zoffset += texImage->Border;
            /* fall-through */
         case 2:
            yoffset += texImage->Border;
            /* fall-through */
         case 1:
            xoffset += texImage->Border;
         }

         switch (dims) {
         case 1:
            ctx->Driver.TexSubImage1D(ctx, texImage,
                                      xoffset, width,
                                      format, type, pixels, &ctx->Unpack);
            break;
         case 2:
            ctx->Driver.TexSubImage2D(ctx, texImage,
                                      xoffset, yoffset, width, height,
                                      format, type, pixels, &ctx->Unpack);
            break;
         case 3:
            ctx->Driver.TexSubImage3D(ctx, texImage,
                                      xoffset, yoffset, zoffset,
                                      width, height, depth,
                                      format, type, pixels, &ctx->Unpack);
            break;
         default:
            _mesa_problem(ctx, "unexpected dims in subteximage()");
         }

         check_gen_mipmap(ctx, target, texObj, level);

         ctx->NewState |= _NEW_TEXTURE;
      }
   }
   _mesa_unlock_texture(ctx, texObj);
}


void GLAPIENTRY
_mesa_TexSubImage1D( GLenum target, GLint level,
                     GLint xoffset, GLsizei width,
                     GLenum format, GLenum type,
                     const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   texsubimage(ctx, 1, target, level,
               xoffset, 0, 0,
               width, 1, 1,
               format, type, pixels);
}


void GLAPIENTRY
_mesa_TexSubImage2D( GLenum target, GLint level,
                     GLint xoffset, GLint yoffset,
                     GLsizei width, GLsizei height,
                     GLenum format, GLenum type,
                     const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   texsubimage(ctx, 2, target, level,
               xoffset, yoffset, 0,
               width, height, 1,
               format, type, pixels);
}



void GLAPIENTRY
_mesa_TexSubImage3D( GLenum target, GLint level,
                     GLint xoffset, GLint yoffset, GLint zoffset,
                     GLsizei width, GLsizei height, GLsizei depth,
                     GLenum format, GLenum type,
                     const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   texsubimage(ctx, 3, target, level,
               xoffset, yoffset, zoffset,
               width, height, depth,
               format, type, pixels);
}



/**
 * For glCopyTexSubImage, return the source renderbuffer to copy texel data
 * from.  This depends on whether the texture contains color or depth values.
 */
static struct gl_renderbuffer *
get_copy_tex_image_source(struct gl_context *ctx, gl_format texFormat)
{
   if (_mesa_get_format_bits(texFormat, GL_DEPTH_BITS) > 0) {
      /* reading from depth/stencil buffer */
      return ctx->ReadBuffer->Attachment[BUFFER_DEPTH].Renderbuffer;
   }
   else {
      /* copying from color buffer */
      return ctx->ReadBuffer->_ColorReadBuffer;
   }
}



/**
 * Implement the glCopyTexImage1/2D() functions.
 */
static void
copyteximage(struct gl_context *ctx, GLuint dims,
             GLenum target, GLint level, GLenum internalFormat,
             GLint x, GLint y, GLsizei width, GLsizei height, GLint border )
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   const GLuint face = _mesa_tex_target_to_face(target);

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glCopyTexImage%uD %s %d %s %d %d %d %d %d\n",
                  dims,
                  _mesa_lookup_enum_by_nr(target), level,
                  _mesa_lookup_enum_by_nr(internalFormat),
                  x, y, width, height, border);

   if (ctx->NewState & NEW_COPY_TEX_STATE)
      _mesa_update_state(ctx);

   if (copytexture_error_check(ctx, dims, target, level, internalFormat,
                               width, height, border))
      return;

   texObj = _mesa_get_current_tex_object(ctx, target);

   if (border && ctx->Const.StripTextureBorder) {
      x += border;
      width -= border * 2;
      if (dims == 2) {
	 y += border;
	 height -= border * 2;
      }
      border = 0;
   }

   _mesa_lock_texture(ctx, texObj);
   {
      texImage = _mesa_get_tex_image(ctx, texObj, target, level);

      if (!texImage) {
	 _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexImage%uD", dims);
      }
      else {
         /* choose actual hw format */
         gl_format texFormat = _mesa_choose_texture_format(ctx, texObj,
                                                           target, level,
                                                           internalFormat,
                                                           GL_NONE, GL_NONE);

         if (legal_texture_size(ctx, texFormat, width, height, 1)) {
            GLint srcX = x, srcY = y, dstX = 0, dstY = 0;

            /* Free old texture image */
            ctx->Driver.FreeTextureImageBuffer(ctx, texImage);

            _mesa_init_teximage_fields(ctx, texImage, width, height, 1,
                                       border, internalFormat, texFormat);

            /* Allocate texture memory (no pixel data yet) */
            if (dims == 1) {
               ctx->Driver.TexImage1D(ctx, texImage, internalFormat,
                                      width, border, GL_NONE, GL_NONE, NULL,
                                      &ctx->Unpack);
            }
            else {
               ctx->Driver.TexImage2D(ctx, texImage, internalFormat,
                                      width, height, border, GL_NONE, GL_NONE,
                                      NULL, &ctx->Unpack);
            }

            if (_mesa_clip_copytexsubimage(ctx, &dstX, &dstY, &srcX, &srcY,
                                           &width, &height)) {
               struct gl_renderbuffer *srcRb =
                  get_copy_tex_image_source(ctx, texImage->TexFormat);

               if (dims == 1)
                  ctx->Driver.CopyTexSubImage1D(ctx, texImage, dstX,
                                                srcRb, srcX, srcY, width);
                                                
               else
                  ctx->Driver.CopyTexSubImage2D(ctx, texImage, dstX, dstY,
                                                srcRb, srcX, srcY, width, height);
            }

            check_gen_mipmap(ctx, target, texObj, level);

            _mesa_update_fbo_texture(ctx, texObj, face, level);

            /* state update */
            texObj->_Complete = GL_FALSE;
            ctx->NewState |= _NEW_TEXTURE;
         }
         else {
            /* probably too large of image */
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexImage%uD", dims);
         }
      }
   }
   _mesa_unlock_texture(ctx, texObj);
}



void GLAPIENTRY
_mesa_CopyTexImage1D( GLenum target, GLint level,
                      GLenum internalFormat,
                      GLint x, GLint y,
                      GLsizei width, GLint border )
{
   GET_CURRENT_CONTEXT(ctx);
   copyteximage(ctx, 1, target, level, internalFormat, x, y, width, 1, border);
}



void GLAPIENTRY
_mesa_CopyTexImage2D( GLenum target, GLint level, GLenum internalFormat,
                      GLint x, GLint y, GLsizei width, GLsizei height,
                      GLint border )
{
   GET_CURRENT_CONTEXT(ctx);
   copyteximage(ctx, 2, target, level, internalFormat,
                x, y, width, height, border);
}



/**
 * Implementation for glCopyTexSubImage1/2/3D() functions.
 */
static void
copytexsubimage(struct gl_context *ctx, GLuint dims, GLenum target, GLint level,
                GLint xoffset, GLint yoffset, GLint zoffset,
                GLint x, GLint y, GLsizei width, GLsizei height)
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx, "glCopyTexSubImage%uD %s %d %d %d %d %d %d %d %d\n",
                  dims,
                  _mesa_lookup_enum_by_nr(target),
                  level, xoffset, yoffset, zoffset, x, y, width, height);

   if (ctx->NewState & NEW_COPY_TEX_STATE)
      _mesa_update_state(ctx);

   if (copytexsubimage_error_check1(ctx, dims, target, level))
      return;

   texObj = _mesa_get_current_tex_object(ctx, target);

   _mesa_lock_texture(ctx, texObj);
   {
      texImage = _mesa_select_tex_image(ctx, texObj, target, level);

      if (copytexsubimage_error_check2(ctx, dims, target, level, xoffset, yoffset,
				       zoffset, width, height, texImage)) {
         /* error was recored */
      }
      else {
         /* If we have a border, offset=-1 is legal.  Bias by border width. */
         switch (dims) {
         case 3:
            zoffset += texImage->Border;
            /* fall-through */
         case 2:
            yoffset += texImage->Border;
            /* fall-through */
         case 1:
            xoffset += texImage->Border;
         }

         if (_mesa_clip_copytexsubimage(ctx, &xoffset, &yoffset, &x, &y,
                                        &width, &height)) {
            struct gl_renderbuffer *srcRb =
               get_copy_tex_image_source(ctx, texImage->TexFormat);

            switch (dims) {
            case 1:
               ctx->Driver.CopyTexSubImage1D(ctx, texImage, xoffset,
                                             srcRb, x, y, width);
               break;
            case 2:
               ctx->Driver.CopyTexSubImage2D(ctx, texImage, xoffset, yoffset,
                                             srcRb, x, y, width, height);
               break;
            case 3:
               ctx->Driver.CopyTexSubImage3D(ctx, texImage,
                                             xoffset, yoffset, zoffset,
                                             srcRb, x, y, width, height);
               break;
            default:
               _mesa_problem(ctx, "bad dims in copytexsubimage()");
            }

            check_gen_mipmap(ctx, target, texObj, level);

            ctx->NewState |= _NEW_TEXTURE;
         }
      }
   }
   _mesa_unlock_texture(ctx, texObj);
}


void GLAPIENTRY
_mesa_CopyTexSubImage1D( GLenum target, GLint level,
                         GLint xoffset, GLint x, GLint y, GLsizei width )
{
   GET_CURRENT_CONTEXT(ctx);
   copytexsubimage(ctx, 1, target, level, xoffset, 0, 0, x, y, width, 1);
}



void GLAPIENTRY
_mesa_CopyTexSubImage2D( GLenum target, GLint level,
                         GLint xoffset, GLint yoffset,
                         GLint x, GLint y, GLsizei width, GLsizei height )
{
   GET_CURRENT_CONTEXT(ctx);
   copytexsubimage(ctx, 2, target, level, xoffset, yoffset, 0, x, y,
                   width, height);
}



void GLAPIENTRY
_mesa_CopyTexSubImage3D( GLenum target, GLint level,
                         GLint xoffset, GLint yoffset, GLint zoffset,
                         GLint x, GLint y, GLsizei width, GLsizei height )
{
   GET_CURRENT_CONTEXT(ctx);
   copytexsubimage(ctx, 3, target, level, xoffset, yoffset, zoffset,
                   x, y, width, height);
}




/**********************************************************************/
/******                   Compressed Textures                    ******/
/**********************************************************************/


/**
 * Return expected size of a compressed texture.
 */
static GLuint
compressed_tex_size(GLsizei width, GLsizei height, GLsizei depth,
                    GLenum glformat)
{
   gl_format mesaFormat = _mesa_glenum_to_compressed_format(glformat);
   return _mesa_format_image_size(mesaFormat, width, height, depth);
}


/*
 * Return compressed texture block size, in pixels.
 */
static void
get_compressed_block_size(GLenum glformat, GLuint *bw, GLuint *bh)
{
   gl_format mesaFormat = _mesa_glenum_to_compressed_format(glformat);
   _mesa_get_format_block_size(mesaFormat, bw, bh);
}


/**
 * Error checking for glCompressedTexImage[123]D().
 * \param reason  returns reason for error, if any
 * \return error code or GL_NO_ERROR.
 */
static GLenum
compressed_texture_error_check(struct gl_context *ctx, GLint dimensions,
                               GLenum target, GLint level,
                               GLenum internalFormat, GLsizei width,
                               GLsizei height, GLsizei depth, GLint border,
                               GLsizei imageSize, char **reason)
{
   const GLenum proxyTarget = get_proxy_target(target);
   const GLint maxLevels = _mesa_max_texture_levels(ctx, target);
   GLint expectedSize;
   GLenum choose_format;
   GLenum choose_type;
   GLenum proxy_format;

   *reason = ""; /* no error */

   if (!target_can_be_compressed(ctx, target, internalFormat)) {
      *reason = "target";
      return GL_INVALID_ENUM;
   }

   /* This will detect any invalid internalFormat value */
   if (!_mesa_is_compressed_format(ctx, internalFormat)) {
      *reason = "internalFormat";
      return GL_INVALID_ENUM;
   }

   switch (internalFormat) {
#if FEATURE_ES
   case GL_PALETTE4_RGB8_OES:
   case GL_PALETTE4_RGBA8_OES:
   case GL_PALETTE4_R5_G6_B5_OES:
   case GL_PALETTE4_RGBA4_OES:
   case GL_PALETTE4_RGB5_A1_OES:
   case GL_PALETTE8_RGB8_OES:
   case GL_PALETTE8_RGBA8_OES:
   case GL_PALETTE8_R5_G6_B5_OES:
   case GL_PALETTE8_RGBA4_OES:
   case GL_PALETTE8_RGB5_A1_OES:
      _mesa_cpal_compressed_format_type(internalFormat, &choose_format,
					&choose_type);
      proxy_format = choose_format;

      /* check level */
      if (level > 0 || level < -maxLevels) {
	 *reason = "level";
	 return GL_INVALID_VALUE;
      }

      if (dimensions != 2) {
	 *reason = "compressed paletted textures must be 2D";
	 return GL_INVALID_OPERATION;
      }

      /* Figure out the expected texture size (in bytes).  This will be
       * checked against the actual size later.
       */
      expectedSize = _mesa_cpal_compressed_size(level, internalFormat,
						width, height);

      /* This is for the benefit of the TestProxyTexImage below.  It expects
       * level to be non-negative.  OES_compressed_paletted_texture uses a
       * weird mechanism where the level specified to glCompressedTexImage2D
       * is -(n-1) number of levels in the texture, and the data specifies the
       * complete mipmap stack.  This is done to ensure the palette is the
       * same for all levels.
       */
      level = -level;
      break;
#endif

   default:
      choose_format = GL_NONE;
      choose_type = GL_NONE;
      proxy_format = internalFormat;

      /* check level */
      if (level < 0 || level >= maxLevels) {
	 *reason = "level";
	 return GL_INVALID_VALUE;
      }

      /* Figure out the expected texture size (in bytes).  This will be
       * checked against the actual size later.
       */
      expectedSize = compressed_tex_size(width, height, depth, internalFormat);
      break;
   }

   /* This should really never fail */
   if (_mesa_base_tex_format(ctx, internalFormat) < 0) {
      *reason = "internalFormat";
      return GL_INVALID_ENUM;
   }

   /* No compressed formats support borders at this time */
   if (border != 0) {
      *reason = "border != 0";
      return GL_INVALID_VALUE;
   }

   /* For cube map, width must equal height */
   if (_mesa_is_cube_face(target) && width != height) {
      *reason = "width != height";
      return GL_INVALID_VALUE;
   }

   /* check image size against compression block size */
   {
      gl_format texFormat =
         ctx->Driver.ChooseTextureFormat(ctx, proxy_format,
					 choose_format, choose_type);
      GLuint bw, bh;

      _mesa_get_format_block_size(texFormat, &bw, &bh);
      if ((width > bw && width % bw > 0) ||
          (height > bh && height % bh > 0)) {
         /*
          * Per GL_ARB_texture_compression:  GL_INVALID_OPERATION is
          * generated [...] if any parameter combinations are not
          * supported by the specific compressed internal format. 
          */
         *reason = "invalid width or height for compression format";
         return GL_INVALID_OPERATION;
      }
   }

   /* check image sizes */
   if (!ctx->Driver.TestProxyTexImage(ctx, proxyTarget, level,
				      proxy_format, choose_format,
				      choose_type,
				      width, height, depth, border)) {
      /* See error comment above */
      *reason = "invalid width, height or format";
      return GL_INVALID_OPERATION;
   }

   /* check image size in bytes */
   if (expectedSize != imageSize) {
      /* Per GL_ARB_texture_compression:  GL_INVALID_VALUE is generated [...]
       * if <imageSize> is not consistent with the format, dimensions, and
       * contents of the specified image.
       */
      *reason = "imageSize inconsistant with width/height/format";
      return GL_INVALID_VALUE;
   }

   if (!mutable_tex_object(ctx, target)) {
      *reason = "immutable texture";
      return GL_INVALID_OPERATION;
   }

   return GL_NO_ERROR;
}


/**
 * Error checking for glCompressedTexSubImage[123]D().
 * \warning  There are some bad assumptions here about the size of compressed
 *           texture tiles (multiple of 4) used to test the validity of the
 *           offset and size parameters.
 * \return error code or GL_NO_ERROR.
 */
static GLenum
compressed_subtexture_error_check(struct gl_context *ctx, GLint dimensions,
                                  GLenum target, GLint level,
                                  GLint xoffset, GLint yoffset, GLint zoffset,
                                  GLsizei width, GLsizei height, GLsizei depth,
                                  GLenum format, GLsizei imageSize)
{
   GLint expectedSize, maxLevels = 0, maxTextureSize;
   GLuint bw, bh;
   (void) zoffset;

   if (dimensions == 1) {
      /* 1D compressed textures not allowed */
      return GL_INVALID_ENUM;
   }
   else if (dimensions == 2) {
      if (target == GL_PROXY_TEXTURE_2D) {
         maxLevels = ctx->Const.MaxTextureLevels;
      }
      else if (target == GL_TEXTURE_2D) {
         maxLevels = ctx->Const.MaxTextureLevels;
      }
      else if (target == GL_PROXY_TEXTURE_CUBE_MAP_ARB) {
         if (!ctx->Extensions.ARB_texture_cube_map)
            return GL_INVALID_ENUM; /*target*/
         maxLevels = ctx->Const.MaxCubeTextureLevels;
      }
      else if (_mesa_is_cube_face(target)) {
         if (!ctx->Extensions.ARB_texture_cube_map)
            return GL_INVALID_ENUM; /*target*/
         maxLevels = ctx->Const.MaxCubeTextureLevels;
      }
      else {
         return GL_INVALID_ENUM; /*target*/
      }
   }
   else if (dimensions == 3) {
      /* 3D compressed textures not allowed */
      return GL_INVALID_ENUM;
   }

   maxTextureSize = 1 << (maxLevels - 1);

   /* this will catch any invalid compressed format token */
   if (!_mesa_is_compressed_format(ctx, format))
      return GL_INVALID_ENUM;

   if (width < 1 || width > maxTextureSize)
      return GL_INVALID_VALUE;

   if ((height < 1 || height > maxTextureSize)
       && dimensions > 1)
      return GL_INVALID_VALUE;

   if (level < 0 || level >= maxLevels)
      return GL_INVALID_VALUE;

   /*
    * do checks which depend on compression block size
    */
   get_compressed_block_size(format, &bw, &bh);

   if ((xoffset % bw != 0) || (yoffset % bh != 0))
      return GL_INVALID_VALUE;

   if ((width % bw != 0) && width != 2 && width != 1)
      return GL_INVALID_VALUE;

   if ((height % bh != 0) && height != 2 && height != 1)
      return GL_INVALID_VALUE;

   expectedSize = compressed_tex_size(width, height, depth, format);
   if (expectedSize != imageSize)
      return GL_INVALID_VALUE;

   return GL_NO_ERROR;
}


/**
 * Do second part of glCompressedTexSubImage error checking.
 * \return GL_TRUE if error found, GL_FALSE otherwise.
 */
static GLboolean
compressed_subtexture_error_check2(struct gl_context *ctx, GLuint dims,
                                   GLsizei width, GLsizei height,
                                   GLsizei depth, GLenum format,
                                   struct gl_texture_image *texImage)
{

   if ((GLint) format != texImage->InternalFormat) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCompressedTexSubImage%uD(format=0x%x)", dims, format);
      return GL_TRUE;
   }

   if (compressedteximage_only_format(ctx, format)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCompressedTexSubImage%uD(format=0x%x cannot be updated)"
                  , dims, format);
      return GL_TRUE;
   }

   if (((width == 1 || width == 2) &&
        width != (GLsizei) texImage->Width) ||
       (width > (GLsizei) texImage->Width)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glCompressedTexSubImage%uD(width=%d)", dims, width);
      return GL_TRUE;
   }

   if (dims >= 2) {
      if (((height == 1 || height == 2) &&
           height != (GLsizei) texImage->Height) ||
          (height > (GLsizei) texImage->Height)) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCompressedTexSubImage%uD(height=%d)", dims, height);
         return GL_TRUE;
      }
   }

   if (dims >= 3) {
      if (((depth == 1 || depth == 2) &&
           depth != (GLsizei) texImage->Depth) ||
          (depth > (GLsizei) texImage->Depth)) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glCompressedTexSubImage%uD(depth=%d)", dims, depth);
         return GL_TRUE;
      }
   }

   return GL_FALSE;
}


/**
 * Implementation of the glCompressedTexImage1/2/3D() functions.
 */
static void
compressedteximage(struct gl_context *ctx, GLuint dims,
                   GLenum target, GLint level,
                   GLenum internalFormat, GLsizei width,
                   GLsizei height, GLsizei depth, GLint border,
                   GLsizei imageSize, const GLvoid *data)
{
   GLenum error;
   char *reason = "";

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      _mesa_debug(ctx,
                  "glCompressedTexImage%uDARB %s %d %s %d %d %d %d %d %p\n",
                  dims,
                  _mesa_lookup_enum_by_nr(target), level,
                  _mesa_lookup_enum_by_nr(internalFormat),
                  width, height, depth, border, imageSize, data);

   /* check target */
   if (!legal_teximage_target(ctx, dims, target)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glCompressedTexImage%uD(target=%s)",
                  dims, _mesa_lookup_enum_by_nr(target));
      return;
   }

   error = compressed_texture_error_check(ctx, dims, target, level,
                                          internalFormat, width, height, depth,
                                          border, imageSize, &reason);

#if FEATURE_ES
   /* XXX this is kind of a hack */
   if (!error && dims == 2) {
      switch (internalFormat) {
      case GL_PALETTE4_RGB8_OES:
      case GL_PALETTE4_RGBA8_OES:
      case GL_PALETTE4_R5_G6_B5_OES:
      case GL_PALETTE4_RGBA4_OES:
      case GL_PALETTE4_RGB5_A1_OES:
      case GL_PALETTE8_RGB8_OES:
      case GL_PALETTE8_RGBA8_OES:
      case GL_PALETTE8_R5_G6_B5_OES:
      case GL_PALETTE8_RGBA4_OES:
      case GL_PALETTE8_RGB5_A1_OES:
         _mesa_cpal_compressed_teximage2d(target, level, internalFormat,
                                          width, height, imageSize, data);
         return;
      }
   }
#endif

   if (_mesa_is_proxy_texture(target)) {
      /* Proxy texture: just check for errors and update proxy state */
      struct gl_texture_image *texImage;

      if (!error) {
         struct gl_texture_object *texObj =
            _mesa_get_current_tex_object(ctx, target);
         gl_format texFormat =
            _mesa_choose_texture_format(ctx, texObj, target, level,
                                        internalFormat, GL_NONE, GL_NONE);
         if (!legal_texture_size(ctx, texFormat, width, height, depth)) {
            error = GL_OUT_OF_MEMORY;
         }
      }

      texImage = _mesa_get_proxy_tex_image(ctx, target, level);
      if (texImage) {
         if (error) {
            /* if error, clear all proxy texture image parameters */
            clear_teximage_fields(texImage);
         }
         else {
            /* no error: store the teximage parameters */
            _mesa_init_teximage_fields(ctx, texImage, width, height,
                                       depth, border, internalFormat,
                                       MESA_FORMAT_NONE);
         }
      }
   }
   else {
      /* non-proxy target */
      struct gl_texture_object *texObj;
      struct gl_texture_image *texImage;

      if (error) {
         _mesa_error(ctx, error, "glCompressedTexImage%uD(%s)", dims, reason);
         return;
      }

      texObj = _mesa_get_current_tex_object(ctx, target);

      _mesa_lock_texture(ctx, texObj);
      {
	 texImage = _mesa_get_tex_image(ctx, texObj, target, level);
	 if (!texImage) {
	    _mesa_error(ctx, GL_OUT_OF_MEMORY,
                        "glCompressedTexImage%uD", dims);
	 }
         else {
            gl_format texFormat;

            ctx->Driver.FreeTextureImageBuffer(ctx, texImage);

            texFormat = _mesa_choose_texture_format(ctx, texObj, target, level,
                                                    internalFormat, GL_NONE,
                                                    GL_NONE);

            if (legal_texture_size(ctx, texFormat, width, height, depth)) {
               _mesa_init_teximage_fields(ctx, texImage,
                                          width, height, depth,
                                          border, internalFormat, texFormat);

               switch (dims) {
               case 1:
                  ASSERT(ctx->Driver.CompressedTexImage1D);
                  ctx->Driver.CompressedTexImage1D(ctx, texImage,
                                                   internalFormat,
                                                   width,
                                                   border, imageSize, data);
                  break;
               case 2:
                  ASSERT(ctx->Driver.CompressedTexImage2D);
                  ctx->Driver.CompressedTexImage2D(ctx, texImage,
                                                   internalFormat,
                                                   width, height,
                                                   border, imageSize, data);
                  break;
               case 3:
                  ASSERT(ctx->Driver.CompressedTexImage3D);
                  ctx->Driver.CompressedTexImage3D(ctx, texImage,
                                                   internalFormat,
                                                   width, height, depth,
                                                   border, imageSize, data);
                  break;
               default:
                  _mesa_problem(ctx, "bad dims in compressedteximage");
               }

               check_gen_mipmap(ctx, target, texObj, level);

               /* state update */
               texObj->_Complete = GL_FALSE;
               ctx->NewState |= _NEW_TEXTURE;
            }
            else {
               _mesa_error(ctx, GL_OUT_OF_MEMORY,
                           "glCompressedTexImage%uD", dims);
            }
         }
      }
      _mesa_unlock_texture(ctx, texObj);
   }
}


void GLAPIENTRY
_mesa_CompressedTexImage1DARB(GLenum target, GLint level,
                              GLenum internalFormat, GLsizei width,
                              GLint border, GLsizei imageSize,
                              const GLvoid *data)
{
   GET_CURRENT_CONTEXT(ctx);
   compressedteximage(ctx, 1, target, level, internalFormat,
                      width, 1, 1, border, imageSize, data);
}


void GLAPIENTRY
_mesa_CompressedTexImage2DARB(GLenum target, GLint level,
                              GLenum internalFormat, GLsizei width,
                              GLsizei height, GLint border, GLsizei imageSize,
                              const GLvoid *data)
{
   GET_CURRENT_CONTEXT(ctx);
   compressedteximage(ctx, 2, target, level, internalFormat,
                      width, height, 1, border, imageSize, data);
}


void GLAPIENTRY
_mesa_CompressedTexImage3DARB(GLenum target, GLint level,
                              GLenum internalFormat, GLsizei width,
                              GLsizei height, GLsizei depth, GLint border,
                              GLsizei imageSize, const GLvoid *data)
{
   GET_CURRENT_CONTEXT(ctx);
   compressedteximage(ctx, 3, target, level, internalFormat,
                      width, height, depth, border, imageSize, data);
}


/**
 * Common helper for glCompressedTexSubImage1/2/3D().
 */
static void
compressed_tex_sub_image(GLuint dims, GLenum target, GLint level,
                         GLint xoffset, GLint yoffset, GLint zoffset,
                         GLsizei width, GLsizei height, GLsizei depth,
                         GLenum format, GLsizei imageSize, const GLvoid *data)
{
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GLenum error;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   error = compressed_subtexture_error_check(ctx, dims, target, level,
                                             xoffset, 0, 0, /* pos */
                                             width, height, depth,   /* size */
                                             format, imageSize);
   if (error) {
      _mesa_error(ctx, error, "glCompressedTexSubImage%uD", dims);
      return;
   }

   texObj = _mesa_get_current_tex_object(ctx, target);

   _mesa_lock_texture(ctx, texObj);
   {
      texImage = _mesa_select_tex_image(ctx, texObj, target, level);
      assert(texImage);

      if (compressed_subtexture_error_check2(ctx, dims, width, height, depth,
                                             format, texImage)) {
         /* error was recorded */
      }
      else if (width > 0 && height > 0 && depth > 0) {
         switch (dims) {
         case 1:
            if (ctx->Driver.CompressedTexSubImage1D) {
               ctx->Driver.CompressedTexSubImage1D(ctx, texImage,
                                                   xoffset, width,
                                                   format, imageSize, data);
            }
            break;
         case 2:
            if (ctx->Driver.CompressedTexSubImage2D) {
               ctx->Driver.CompressedTexSubImage2D(ctx, texImage,
                                                   xoffset, yoffset,
                                                   width, height,
                                                   format, imageSize, data);
            }
            break;
         case 3:
            if (ctx->Driver.CompressedTexSubImage3D) {
               ctx->Driver.CompressedTexSubImage3D(ctx, texImage,
                                                   xoffset, yoffset, zoffset,
                                                   width, height, depth,
                                                   format, imageSize, data);
            }
            break;
         default:
            ;
         }

         check_gen_mipmap(ctx, target, texObj, level);

         ctx->NewState |= _NEW_TEXTURE;
      }
   }
   _mesa_unlock_texture(ctx, texObj);
}


void GLAPIENTRY
_mesa_CompressedTexSubImage1DARB(GLenum target, GLint level, GLint xoffset,
                                 GLsizei width, GLenum format,
                                 GLsizei imageSize, const GLvoid *data)
{
   compressed_tex_sub_image(1, target, level, xoffset, 0, 0, width, 1, 1,
                            format, imageSize, data);
}


void GLAPIENTRY
_mesa_CompressedTexSubImage2DARB(GLenum target, GLint level, GLint xoffset,
                                 GLint yoffset, GLsizei width, GLsizei height,
                                 GLenum format, GLsizei imageSize,
                                 const GLvoid *data)
{
   compressed_tex_sub_image(2, target, level, xoffset, yoffset, 0,
                            width, height, 1, format, imageSize, data);
}


void GLAPIENTRY
_mesa_CompressedTexSubImage3DARB(GLenum target, GLint level, GLint xoffset,
                                 GLint yoffset, GLint zoffset, GLsizei width,
                                 GLsizei height, GLsizei depth, GLenum format,
                                 GLsizei imageSize, const GLvoid *data)
{
   compressed_tex_sub_image(3, target, level, xoffset, yoffset, zoffset,
                            width, height, depth, format, imageSize, data);
}


/**
 * Helper for glTexBuffer().  Check if internalFormat is legal.  If so,
 * return the basic data type and number of components for the format.
 * \param return  GL_TRUE if internalFormat is legal, GL_FALSE otherwise
 */
static GLboolean
get_sized_format_info(const struct gl_context *ctx, GLenum internalFormat,
                      GLenum *datatype, GLuint *components)
{
   switch (internalFormat) {
   case GL_ALPHA8:
      *datatype = GL_UNSIGNED_BYTE;
      *components = 1;
      break;
   case GL_ALPHA16:
      *datatype = GL_UNSIGNED_SHORT;
      *components = 1;
      break;
   case GL_ALPHA16F_ARB:
      *datatype = GL_HALF_FLOAT;
      *components = 1;
      break;
   case GL_ALPHA32F_ARB:
      *datatype = GL_FLOAT;
      *components = 1;
      break;
   case GL_ALPHA8I_EXT:
      *datatype = GL_BYTE;
      *components = 1;
      break;
   case GL_ALPHA16I_EXT:
      *datatype = GL_SHORT;
      *components = 1;
      break;
   case GL_ALPHA32I_EXT:
      *datatype = GL_INT;
      *components = 1;
      break;
   case GL_ALPHA8UI_EXT:
      *datatype = GL_UNSIGNED_BYTE;
      *components = 1;
      break;
   case GL_ALPHA16UI_EXT:
      *datatype = GL_UNSIGNED_SHORT;
      *components = 1;
      break;
   case GL_ALPHA32UI_EXT:
      *datatype = GL_UNSIGNED_INT;
      *components = 1;
      break;
   case GL_LUMINANCE8:
      *datatype = GL_UNSIGNED_BYTE;
      *components = 1;
      break;
   case GL_LUMINANCE16:
      *datatype = GL_UNSIGNED_SHORT;
      *components = 1;
      break;
   case GL_LUMINANCE16F_ARB:
      *datatype = GL_HALF_FLOAT;
      *components = 1;
      break;
   case GL_LUMINANCE32F_ARB:
      *datatype = GL_FLOAT;
      *components = 1;
      break;
   case GL_LUMINANCE8I_EXT:
      *datatype = GL_BYTE;
      *components = 1;
      break;
   case GL_LUMINANCE16I_EXT:
      *datatype = GL_SHORT;
      *components = 1;
      break;
   case GL_LUMINANCE32I_EXT:
      *datatype = GL_INT;
      *components = 1;
      break;
   case GL_LUMINANCE8UI_EXT:
      *datatype = GL_UNSIGNED_BYTE;
      *components = 1;
      break;
   case GL_LUMINANCE16UI_EXT:
      *datatype = GL_UNSIGNED_SHORT;
      *components = 1;
      break;
   case GL_LUMINANCE32UI_EXT:
      *datatype = GL_UNSIGNED_INT;
      *components = 1;
      break;
   case GL_LUMINANCE8_ALPHA8:
      *datatype = GL_UNSIGNED_BYTE;
      *components = 2;
      break;
   case GL_LUMINANCE16_ALPHA16:
      *datatype = GL_UNSIGNED_SHORT;
      *components = 2;
      break;
   case GL_LUMINANCE_ALPHA16F_ARB:
      *datatype = GL_HALF_FLOAT;
      *components = 2;
      break;
   case GL_LUMINANCE_ALPHA32F_ARB:
      *datatype = GL_FLOAT;
      *components = 2;
      break;
   case GL_LUMINANCE_ALPHA8I_EXT:
      *datatype = GL_BYTE;
      *components = 2;
      break;
   case GL_LUMINANCE_ALPHA16I_EXT:
      *datatype = GL_SHORT;
      *components = 2;
      break;
   case GL_LUMINANCE_ALPHA32I_EXT:
      *datatype = GL_INT;
      *components = 2;
      break;
   case GL_LUMINANCE_ALPHA8UI_EXT:
      *datatype = GL_UNSIGNED_BYTE;
      *components = 2;
      break;
   case GL_LUMINANCE_ALPHA16UI_EXT:
      *datatype = GL_UNSIGNED_SHORT;
      *components = 2;
      break;
   case GL_LUMINANCE_ALPHA32UI_EXT:
      *datatype = GL_UNSIGNED_INT;
      *components = 2;
      break;
   case GL_INTENSITY8:
      *datatype = GL_UNSIGNED_BYTE;
      *components = 1;
      break;
   case GL_INTENSITY16:
      *datatype = GL_UNSIGNED_SHORT;
      *components = 1;
      break;
   case GL_INTENSITY16F_ARB:
      *datatype = GL_HALF_FLOAT;
      *components = 1;
      break;
   case GL_INTENSITY32F_ARB:
      *datatype = GL_FLOAT;
      *components = 1;
      break;
   case GL_INTENSITY8I_EXT:
      *datatype = GL_BYTE;
      *components = 1;
      break;
   case GL_INTENSITY16I_EXT:
      *datatype = GL_SHORT;
      *components = 1;
      break;
   case GL_INTENSITY32I_EXT:
      *datatype = GL_INT;
      *components = 1;
      break;
   case GL_INTENSITY8UI_EXT:
      *datatype = GL_UNSIGNED_BYTE;
      *components = 1;
      break;
   case GL_INTENSITY16UI_EXT:
      *datatype = GL_UNSIGNED_SHORT;
      *components = 1;
      break;
   case GL_INTENSITY32UI_EXT:
      *datatype = GL_UNSIGNED_INT;
      *components = 1;
      break;
   case GL_RGBA8:
      *datatype = GL_UNSIGNED_BYTE;
      *components = 4;
      break;
   case GL_RGBA16:
      *datatype = GL_UNSIGNED_SHORT;
      *components = 4;
      break;
   case GL_RGBA16F_ARB:
      *datatype = GL_HALF_FLOAT;
      *components = 4;
      break;
   case GL_RGBA32F_ARB:
      *datatype = GL_FLOAT;
      *components = 4;
      break;
   case GL_RGBA8I_EXT:
      *datatype = GL_BYTE;
      *components = 4;
      break;
   case GL_RGBA16I_EXT:
      *datatype = GL_SHORT;
      *components = 4;
      break;
   case GL_RGBA32I_EXT:
      *datatype = GL_INT;
      *components = 4;
      break;
   case GL_RGBA8UI_EXT:
      *datatype = GL_UNSIGNED_BYTE;
      *components = 4;
      break;
   case GL_RGBA16UI_EXT:
      *datatype = GL_UNSIGNED_SHORT;
      *components = 4;
      break;
   case GL_RGBA32UI_EXT:
      *datatype = GL_UNSIGNED_INT;
      *components = 4;
      break;
   default:
      return GL_FALSE;
   }

   if (*datatype == GL_FLOAT && !ctx->Extensions.ARB_texture_float)
      return GL_FALSE;

   if (*datatype == GL_HALF_FLOAT && !ctx->Extensions.ARB_half_float_pixel)
      return GL_FALSE;

   return GL_TRUE;
}


/** GL_ARB_texture_buffer_object */
void GLAPIENTRY
_mesa_TexBuffer(GLenum target, GLenum internalFormat, GLuint buffer)
{
   struct gl_texture_object *texObj;
   struct gl_buffer_object *bufObj;
   GLenum dataType;
   GLuint comps;

   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (!ctx->Extensions.ARB_texture_buffer_object) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTexBuffer");
      return;
   }

   if (target != GL_TEXTURE_BUFFER_ARB) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glTexBuffer(target)");
      return;
   }

   if (!get_sized_format_info(ctx, internalFormat, &dataType, &comps)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glTexBuffer(internalFormat 0x%x)",
                  internalFormat);
      return;
   }

   bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (buffer && !bufObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTexBuffer(buffer %u)", buffer);
      return;
   }

   texObj = _mesa_get_current_tex_object(ctx, target);

   _mesa_lock_texture(ctx, texObj);
   {
      _mesa_reference_buffer_object(ctx, &texObj->BufferObject, bufObj);
      texObj->BufferObjectFormat = internalFormat;
   }
   _mesa_unlock_texture(ctx, texObj);
}
