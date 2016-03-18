/**
 * \file teximage.h
 * Texture images manipulation functions.
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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


#ifndef TEXIMAGE_H
#define TEXIMAGE_H


#include "mtypes.h"
#include "formats.h"


/** Is the given value one of the 6 cube faces? */
static inline GLboolean
_mesa_is_cube_face(GLenum target)
{
   return (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB &&
           target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB);
}

/** Is any of the dimensions of given texture equal to zero? */
static inline GLboolean
_mesa_is_zero_size_texture(const struct gl_texture_image *texImage)
{
   return (texImage->Width == 0 ||
           texImage->Height == 0 ||
           texImage->Depth == 0);
}

/** \name Internal functions */
/*@{*/

extern GLint
_mesa_base_tex_format( struct gl_context *ctx, GLint internalFormat );


extern GLboolean
_mesa_is_proxy_texture(GLenum target);


extern struct gl_texture_image *
_mesa_new_texture_image( struct gl_context *ctx );


extern void
_mesa_delete_texture_image( struct gl_context *ctx,
                            struct gl_texture_image *teximage );


extern void
_mesa_init_teximage_fields(struct gl_context *ctx,
                           struct gl_texture_image *img,
                           GLsizei width, GLsizei height, GLsizei depth,
                           GLint border, GLenum internalFormat,
                           gl_format format);


extern gl_format
_mesa_choose_texture_format(struct gl_context *ctx,
                            struct gl_texture_object *texObj,
                            GLenum target, GLint level,
                            GLenum internalFormat, GLenum format, GLenum type);

extern void
_mesa_clear_texture_image(struct gl_context *ctx,
                          struct gl_texture_image *texImage);


extern struct gl_texture_object *
_mesa_select_tex_object(struct gl_context *ctx,
                        GLenum target);


extern struct gl_texture_image *
_mesa_select_tex_image(struct gl_context *ctx,
                       const struct gl_texture_object *texObj,
                       GLenum target, GLint level);


extern struct gl_texture_image *
_mesa_get_tex_image(struct gl_context *ctx, struct gl_texture_object *texObj,
                    GLenum target, GLint level);


extern struct gl_texture_image *
_mesa_get_proxy_tex_image(struct gl_context *ctx, GLenum target, GLint level);


extern GLint
_mesa_max_texture_levels(struct gl_context *ctx, GLenum target);


extern GLboolean
_mesa_test_proxy_teximage(struct gl_context *ctx, GLenum target, GLint level,
                         GLint internalFormat, GLenum format, GLenum type,
                         GLint width, GLint height, GLint depth, GLint border);


extern GLuint
_mesa_tex_target_to_face(GLenum target);

extern GLint
_mesa_get_texture_dimensions(GLenum target);

/**
 * Lock a texture for updating.  See also _mesa_lock_context_textures().
 */
static inline void
_mesa_lock_texture(struct gl_context *ctx, struct gl_texture_object *texObj)
{
   _glthread_LOCK_MUTEX(ctx->Shared->TexMutex);
   ctx->Shared->TextureStateStamp++;
   (void) texObj;
}

static inline void
_mesa_unlock_texture(struct gl_context *ctx, struct gl_texture_object *texObj)
{
   (void) texObj;
   _glthread_UNLOCK_MUTEX(ctx->Shared->TexMutex);
}

/*@}*/


/** \name API entry point functions */
/*@{*/

extern void GLAPIENTRY
_mesa_TexImage1D( GLenum target, GLint level, GLint internalformat,
                  GLsizei width, GLint border,
                  GLenum format, GLenum type, const GLvoid *pixels );


extern void GLAPIENTRY
_mesa_TexImage2D( GLenum target, GLint level, GLint internalformat,
                  GLsizei width, GLsizei height, GLint border,
                  GLenum format, GLenum type, const GLvoid *pixels );


extern void GLAPIENTRY
_mesa_TexSubImage1D( GLenum target, GLint level, GLint xoffset,
                     GLsizei width,
                     GLenum format, GLenum type,
                     const GLvoid *pixels );


extern void GLAPIENTRY
_mesa_TexSubImage2D( GLenum target, GLint level,
                     GLint xoffset, GLint yoffset,
                     GLsizei width, GLsizei height,
                     GLenum format, GLenum type,
                     const GLvoid *pixels );


extern void GLAPIENTRY
_mesa_CopyTexImage1D( GLenum target, GLint level, GLenum internalformat,
                      GLint x, GLint y, GLsizei width, GLint border );


extern void GLAPIENTRY
_mesa_CopyTexImage2D( GLenum target, GLint level,
                      GLenum internalformat, GLint x, GLint y,
                      GLsizei width, GLsizei height, GLint border );


extern void GLAPIENTRY
_mesa_CopyTexSubImage1D( GLenum target, GLint level, GLint xoffset,
                         GLint x, GLint y, GLsizei width );


extern void GLAPIENTRY
_mesa_CopyTexSubImage2D( GLenum target, GLint level,
                         GLint xoffset, GLint yoffset,
                         GLint x, GLint y, GLsizei width, GLsizei height );

/*@}*/

#endif
