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

#include <precomp.h>

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

   if (ctx->Extensions.MESA_ycbcr_texture) {
      if (internalFormat == GL_YCBCR_MESA)
         return GL_YCBCR_MESA;
   }

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

   return -1; /* error */
}


/**
 * Is the given texture format a generic compressed format?
 */


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
           target == GL_PROXY_TEXTURE_CUBE_MAP_ARB);
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
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
   case GL_TEXTURE_CUBE_MAP_ARB:
   case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
      return GL_PROXY_TEXTURE_CUBE_MAP_ARB;
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
                        GLenum target)
{
   switch (target) {
      case GL_TEXTURE_1D:
         return ctx->Texture.Unit.CurrentTex[TEXTURE_1D_INDEX];
      case GL_PROXY_TEXTURE_1D:
         return ctx->Texture.ProxyTex[TEXTURE_1D_INDEX];
      case GL_TEXTURE_2D:
         return ctx->Texture.Unit.CurrentTex[TEXTURE_2D_INDEX];
      case GL_PROXY_TEXTURE_2D:
         return ctx->Texture.ProxyTex[TEXTURE_2D_INDEX];
      default:
         _mesa_problem(NULL, "bad target in _mesa_select_tex_object()");
         return NULL;
   }
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
   case GL_PROXY_TEXTURE_CUBE_MAP:
      if (level >= ctx->Const.MaxCubeTextureLevels)
         return NULL;
      texIndex = TEXTURE_CUBE_INDEX;
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
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
   case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
      return ctx->Extensions.ARB_texture_cube_map
         ? ctx->Const.MaxCubeTextureLevels : 0;
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
   case GL_TEXTURE_CUBE_MAP:
   case GL_PROXY_TEXTURE_2D:
   case GL_PROXY_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
      return 2;
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
   case GL_TEXTURE_2D:
   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
   case GL_PROXY_TEXTURE_2D:
   case GL_PROXY_TEXTURE_CUBE_MAP:
      img->Height2 = height - 2 * border; /* == 1 << img->HeightLog2; */
      img->HeightLog2 = _mesa_logbase2(img->Height2);
      if (depth == 0)
         img->Depth2 = 0;
      else
         img->Depth2 = 1;
      img->DepthLog2 = 0;
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
      if (width > 0 && !_mesa_is_pow_two(width - 2 * border))
         return GL_FALSE;
      return GL_TRUE;

   case GL_PROXY_TEXTURE_2D:
      maxSize = 1 << (ctx->Const.MaxTextureLevels - 1);
      if (width < 2 * border || width > 2 * border + maxSize)
         return GL_FALSE;
      if (height < 2 * border || height > 2 * border + maxSize)
         return GL_FALSE;
      if (level >= ctx->Const.MaxTextureLevels)
         return GL_FALSE;
      if (width > 0 && !_mesa_is_pow_two(width - 2 * border))
         return GL_FALSE;
      if (height > 0 && !_mesa_is_pow_two(height - 2 * border))
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
      if (width > 0 && !_mesa_is_pow_two(width - 2 * border))
         return GL_FALSE;
      if (height > 0 && !_mesa_is_pow_two(height - 2 * border))
         return GL_FALSE;
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
         _mesa_select_tex_object(ctx, target);
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
   if (border < 0 || border > 1) {
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
       (_mesa_is_ycbcr_format(internalFormat) != _mesa_is_ycbcr_format(format))) {
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
          target != GL_PROXY_TEXTURE_2D) {
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
         !((_mesa_is_cube_face(target) || target == GL_PROXY_TEXTURE_CUBE_MAP) &&
           (ctx->VersionMajor >= 3))) {
         if (!isProxy)
            _mesa_error(ctx, GL_INVALID_ENUM,
                        "glTexImage(target/internalFormat)");
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

   /* Check border */
   if (border < 0 || border > 1) {
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
            _mesa_select_tex_object(ctx, target);
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

      texObj = _mesa_select_tex_object(ctx, target);

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
               default:
                  _mesa_problem(ctx, "invalid dims=%u in teximage()", dims);
               }

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

   texObj = _mesa_select_tex_object(ctx, target);

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
         default:
            _mesa_problem(ctx, "unexpected dims in subteximage()");
         }

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

   texObj = _mesa_select_tex_object(ctx, target);

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

   texObj = _mesa_select_tex_object(ctx, target);

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
            default:
               _mesa_problem(ctx, "bad dims in copytexsubimage()");
            }

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
