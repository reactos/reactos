/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2011  VMware, Inc.  All Rights Reserved.
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
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \file texstorage.c
 * GL_ARB_texture_storage functions
 */

#include <precomp.h>

/**
 * Check if the given texture target is a legal texture object target
 * for a glTexStorage() command.
 * This is a bit different than legal_teximage_target() when it comes
 * to cube maps.
 */
static GLboolean
legal_texobj_target(struct gl_context *ctx, GLuint dims, GLenum target)
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
      case GL_TEXTURE_CUBE_MAP:
      case GL_PROXY_TEXTURE_CUBE_MAP:
         return ctx->Extensions.ARB_texture_cube_map;
      default:
         return GL_FALSE;
      }
   default:
      _mesa_problem(ctx, "invalid dims=%u in legal_texobj_target()", dims);
      return GL_FALSE;
   }
}


/**
 * Compute the size of the next mipmap level.
 */
static void
next_mipmap_level_size(GLenum target,
                       GLint *width, GLint *height, GLint *depth)
{
   if (*width > 1) {
      *width /= 2;
   }

   if (*height > 1) {
      *height /= 2;
   }

   if (*depth > 1) {
      *depth /= 2;
   }
}


/**
 * Do actual memory allocation for glTexStorage1/2/3D().
 */
static void
setup_texstorage(struct gl_context *ctx,
                 struct gl_texture_object *texObj,
                 GLuint dims,
                 GLsizei levels, GLenum internalFormat,
                 GLsizei width, GLsizei height, GLsizei depth)
{
   const GLenum target = texObj->Target;
   const GLuint numFaces = (target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
   gl_format texFormat;
   GLint level, levelWidth = width, levelHeight = height, levelDepth = depth;
   GLuint face;

   assert(levels > 0);
   assert(width > 0);
   assert(height > 0);
   assert(depth > 0);

   texFormat = _mesa_choose_texture_format(ctx, texObj, target, 0,
                                           internalFormat, GL_NONE, GL_NONE);

   /* Set up all the texture object's gl_texture_images */
   for (level = 0; level < levels; level++) {
      for (face = 0; face < numFaces; face++) {
         const GLenum faceTarget =
            (target == GL_TEXTURE_CUBE_MAP)
            ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + face : target;
         struct gl_texture_image *texImage =
            _mesa_get_tex_image(ctx, texObj, faceTarget, level);

	 if (!texImage) {
	    _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage%uD", dims);
            return;
	 }

         _mesa_init_teximage_fields(ctx, texImage,
                                    levelWidth, levelHeight, levelDepth,
                                    0, internalFormat, texFormat);
      }

      next_mipmap_level_size(target, &levelWidth, &levelHeight, &levelDepth);
   }

   assert(levelWidth > 0);
   assert(levelHeight > 0);
   assert(levelDepth > 0);

   if (!_mesa_is_proxy_texture(texObj->Target)) {
      /* Do actual texture memory allocation */
      if (!ctx->Driver.AllocTextureStorage(ctx, texObj, levels,
                                           width, height, depth)) {
         /* Reset the texture images' info to zeros.
          * Strictly speaking, we probably don't have to do this since
          * generating GL_OUT_OF_MEMORY can leave things in an undefined
          * state but this puts things in a consistent state.
          */
         for (level = 0; level < levels; level++) {
            for (face = 0; face < numFaces; face++) {
               struct gl_texture_image *texImage = texObj->Image[face][level];
               if (texImage) {
                  _mesa_init_teximage_fields(ctx, texImage,
                                             0, 0, 0, 0,
                                             GL_NONE, MESA_FORMAT_NONE);
               }
            }
         }

         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexStorage%uD", dims);

         return;
      }
   }

   texObj->Immutable = GL_TRUE;
}


/**
 * Clear all fields of texture object to zeros.  Used for proxy texture tests.
 */
static void
clear_image_fields(struct gl_context *ctx,
                   GLuint dims,
                   struct gl_texture_object *texObj)
{
   const GLenum target = texObj->Target;
   const GLuint numFaces = (target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
   GLint level;
   GLuint face;

   for (level = 0; level < Elements(texObj->Image[0]); level++) {
      for (face = 0; face < numFaces; face++) {
         const GLenum faceTarget =
            (target == GL_TEXTURE_CUBE_MAP)
            ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + face : target;
         struct gl_texture_image *texImage =
            _mesa_get_tex_image(ctx, texObj, faceTarget, level);

	 if (!texImage) {
	    _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexStorage%uD", dims);
            return;
	 }

         _mesa_init_teximage_fields(ctx, texImage,
                                    0, 0, 0, 0, GL_NONE, MESA_FORMAT_NONE);
      }
   }
}


/**
 * Do error checking for calls to glTexStorage1/2/3D().
 * If an error is found, record it with _mesa_error(), unless the target
 * is a proxy texture.
 * \return GL_TRUE if any error, GL_FALSE otherwise.
 */
static GLboolean
tex_storage_error_check(struct gl_context *ctx, GLuint dims, GLenum target,
                        GLsizei levels, GLenum internalformat,
                        GLsizei width, GLsizei height, GLsizei depth)
{
   const GLboolean isProxy = _mesa_is_proxy_texture(target);
   struct gl_texture_object *texObj;
   GLuint maxDim;

   /* size check */
   if (width < 1 || height < 1 || depth < 1) {
      if (!isProxy) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glTexStorage%uD(width, height or depth < 1)", dims);
      }
      return GL_TRUE;
   }  

   /* levels check */
   if (levels < 1 || height < 1 || depth < 1) {
      if (!isProxy) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glTexStorage%uD(levels < 1)",
                     dims);
      }
      return GL_TRUE;
   }  

   /* target check */
   if (!legal_texobj_target(ctx, dims, target)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glTexStorage%uD(illegal target=%s)",
                  dims, _mesa_lookup_enum_by_nr(target));
      return GL_TRUE;
   }

   /* check levels against maximum */
   if (levels > _mesa_max_texture_levels(ctx, target)) {
      if (!isProxy) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glTexStorage%uD(levels too large)", dims);
      }
      return GL_TRUE;
   }

   /* check levels against width/height/depth */
   maxDim = MAX3(width, height, depth);
   if (levels > _mesa_logbase2(maxDim) + 1) {
      if (!isProxy) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glTexStorage%uD(too many levels for max texture dimension)",
                     dims);
      }
      return GL_TRUE;
   }

   /* non-default texture object check */
   texObj = _mesa_select_tex_object(ctx, target);
   if (!texObj || (texObj->Name == 0 && !isProxy)) {
      if (!isProxy) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glTexStorage%uD(texture object 0)", dims);
      }
      return GL_TRUE;
   }

   /* Check if texObj->Immutable is set */
   if (texObj->Immutable) {
      if (!isProxy) {
         _mesa_error(ctx, GL_INVALID_OPERATION, "glTexStorage%uD(immutable)",
                     dims);
      }
      return GL_TRUE;
   }

   return GL_FALSE;
}


/**
 * Helper used by _mesa_TexStorage1/2/3D().
 */
static void
texstorage(GLuint dims, GLenum target, GLsizei levels, GLenum internalformat,
           GLsizei width, GLsizei height, GLsizei depth)
{
   struct gl_texture_object *texObj;
   GLboolean error;

   GET_CURRENT_CONTEXT(ctx);

   texObj = _mesa_select_tex_object(ctx, target);

   error = tex_storage_error_check(ctx, dims, target, levels,
                                   internalformat, width, height, depth);
   if (!error) {
      setup_texstorage(ctx, texObj, dims, levels, internalformat,
                       width, height, depth);
   }
   else if (_mesa_is_proxy_texture(target)) {
      /* clear all image fields for [levels] */
      clear_image_fields(ctx, dims, texObj);
   }
}


void GLAPIENTRY
_mesa_TexStorage1D(GLenum target, GLsizei levels, GLenum internalformat,
                   GLsizei width)
{
   texstorage(1, target, levels, internalformat, width, 1, 1);
}


void GLAPIENTRY
_mesa_TexStorage2D(GLenum target, GLsizei levels, GLenum internalformat,
                   GLsizei width, GLsizei height)
{
   texstorage(2, target, levels, internalformat, width, height, 1);
}


void GLAPIENTRY
_mesa_TexStorage3D(GLenum target, GLsizei levels, GLenum internalformat,
                   GLsizei width, GLsizei height, GLsizei depth)
{
   texstorage(3, target, levels, internalformat, width, height, depth);
}



/*
 * Note: we don't support GL_EXT_direct_state_access and the spec says
 * we don't need the following functions.  However, glew checks for the
 * presence of all six functions and will say that GL_ARB_texture_storage
 * is not supported if these functions are missing.
 */


void GLAPIENTRY
_mesa_TextureStorage1DEXT(GLuint texture, GLenum target, GLsizei levels,
                          GLenum internalformat,
                          GLsizei width)
{
   /* no-op */
}


void GLAPIENTRY
_mesa_TextureStorage2DEXT(GLuint texture, GLenum target, GLsizei levels,
                          GLenum internalformat,
                          GLsizei width, GLsizei height)
{
   /* no-op */
}



void GLAPIENTRY
_mesa_TextureStorage3DEXT(GLuint texture, GLenum target, GLsizei levels,
                          GLenum internalformat,
                          GLsizei width, GLsizei height, GLsizei depth)
{
   /* no-op */
}
