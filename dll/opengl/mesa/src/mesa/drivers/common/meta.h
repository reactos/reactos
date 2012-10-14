/*
 * Mesa 3-D graphics library
 * Version:  7.6
 *
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


#ifndef META_H
#define META_H

#include "main/mtypes.h"

/**
 * \name Flags for meta operations
 * \{
 *
 * These flags are passed to _mesa_meta_begin().
 */
#define MESA_META_ALL                      ~0x0
#define MESA_META_ALPHA_TEST                0x1
#define MESA_META_BLEND                     0x2  /**< includes logicop */
#define MESA_META_COLOR_MASK                0x4
#define MESA_META_DEPTH_TEST                0x8
#define MESA_META_FOG                      0x10
#define MESA_META_PIXEL_STORE              0x20
#define MESA_META_PIXEL_TRANSFER           0x40
#define MESA_META_RASTERIZATION            0x80
#define MESA_META_SCISSOR                 0x100
#define MESA_META_SHADER                  0x200
#define MESA_META_STENCIL_TEST            0x400
#define MESA_META_TRANSFORM               0x800 /**< modelview/projection matrix state */
#define MESA_META_TEXTURE                0x1000
#define MESA_META_VERTEX                 0x2000
#define MESA_META_VIEWPORT               0x4000
#define MESA_META_CLAMP_FRAGMENT_COLOR   0x8000
#define MESA_META_CLAMP_VERTEX_COLOR    0x10000
#define MESA_META_CONDITIONAL_RENDER    0x20000
#define MESA_META_CLIP                  0x40000
#define MESA_META_SELECT_FEEDBACK       0x80000
/**\}*/

extern void
_mesa_meta_init(struct gl_context *ctx);

extern void
_mesa_meta_free(struct gl_context *ctx);

extern void
_mesa_meta_begin(struct gl_context *ctx, GLbitfield state);

extern void
_mesa_meta_end(struct gl_context *ctx);

extern GLboolean
_mesa_meta_in_progress(struct gl_context *ctx);

extern void
_mesa_meta_BlitFramebuffer(struct gl_context *ctx,
                           GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                           GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                           GLbitfield mask, GLenum filter);

extern void
_mesa_meta_Clear(struct gl_context *ctx, GLbitfield buffers);

extern void
_mesa_meta_glsl_Clear(struct gl_context *ctx, GLbitfield buffers);

extern void
_mesa_meta_CopyPixels(struct gl_context *ctx, GLint srcx, GLint srcy,
                      GLsizei width, GLsizei height,
                      GLint dstx, GLint dsty, GLenum type);

extern void
_mesa_meta_DrawPixels(struct gl_context *ctx,
                      GLint x, GLint y, GLsizei width, GLsizei height,
                      GLenum format, GLenum type,
                      const struct gl_pixelstore_attrib *unpack,
                      const GLvoid *pixels);

extern void
_mesa_meta_Bitmap(struct gl_context *ctx,
                  GLint x, GLint y, GLsizei width, GLsizei height,
                  const struct gl_pixelstore_attrib *unpack,
                  const GLubyte *bitmap);

extern GLboolean
_mesa_meta_check_generate_mipmap_fallback(struct gl_context *ctx, GLenum target,
                                          struct gl_texture_object *texObj);

extern void
_mesa_meta_GenerateMipmap(struct gl_context *ctx, GLenum target,
                          struct gl_texture_object *texObj);

extern void
_mesa_meta_CopyTexSubImage1D(struct gl_context *ctx,
                             struct gl_texture_image *texImage,
                             GLint xoffset,
                             struct gl_renderbuffer *rb,
                             GLint x, GLint y, GLsizei width);

extern void
_mesa_meta_CopyTexSubImage2D(struct gl_context *ctx,
                             struct gl_texture_image *texImage,
                             GLint xoffset, GLint yoffset,
                             struct gl_renderbuffer *rb,
                             GLint x, GLint y,
                             GLsizei width, GLsizei height);

extern void
_mesa_meta_CopyTexSubImage3D(struct gl_context *ctx,
                             struct gl_texture_image *texImage,
                             GLint xoffset, GLint yoffset, GLint zoffset,
                             struct gl_renderbuffer *rb,
                             GLint x, GLint y,
                             GLsizei width, GLsizei height);

extern void
_mesa_meta_GetTexImage(struct gl_context *ctx,
                       GLenum format, GLenum type, GLvoid *pixels,
                       struct gl_texture_image *texImage);

extern void
_mesa_meta_DrawTex(struct gl_context *ctx, GLfloat x, GLfloat y, GLfloat z,
                   GLfloat width, GLfloat height);

#endif /* META_H */
