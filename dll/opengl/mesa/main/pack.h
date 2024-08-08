/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 1999-2010  VMware, Inc.  All Rights Reserved.
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


#ifndef PACK_H
#define PACK_H


#include "mtypes.h"


extern void
_mesa_unpack_polygon_stipple(const GLubyte *pattern, GLuint dest[32],
                             const struct gl_pixelstore_attrib *unpacking);


extern void
_mesa_pack_polygon_stipple(const GLuint pattern[32], GLubyte *dest,
                           const struct gl_pixelstore_attrib *packing);


extern GLvoid *
_mesa_unpack_bitmap(GLint width, GLint height, const GLubyte *pixels,
                    const struct gl_pixelstore_attrib *packing);

extern void
_mesa_pack_bitmap(GLint width, GLint height, const GLubyte *source,
                  GLubyte *dest, const struct gl_pixelstore_attrib *packing);


extern void
_mesa_pack_rgba_span_float(struct gl_context *ctx, GLuint n,
                           GLfloat rgba[][4],
                           GLenum dstFormat, GLenum dstType, GLvoid *dstAddr,
                           const struct gl_pixelstore_attrib *dstPacking,
                           GLbitfield transferOps);


extern void
_mesa_unpack_color_span_ubyte(struct gl_context *ctx,
                             GLuint n, GLenum dstFormat, GLubyte dest[],
                             GLenum srcFormat, GLenum srcType,
                             const GLvoid *source,
                             const struct gl_pixelstore_attrib *srcPacking,
                             GLbitfield transferOps);


extern void
_mesa_unpack_color_span_float(struct gl_context *ctx,
                              GLuint n, GLenum dstFormat, GLfloat dest[],
                              GLenum srcFormat, GLenum srcType,
                              const GLvoid *source,
                              const struct gl_pixelstore_attrib *srcPacking,
                              GLbitfield transferOps);

extern void
_mesa_unpack_color_span_uint(struct gl_context *ctx,
                             GLuint n, GLenum dstFormat, GLuint *dest,
                             GLenum srcFormat, GLenum srcType,
                             const GLvoid *source,
                             const struct gl_pixelstore_attrib *srcPacking);

extern void
_mesa_unpack_index_span(struct gl_context *ctx, GLuint n,
                        GLenum dstType, GLvoid *dest,
                        GLenum srcType, const GLvoid *source,
                        const struct gl_pixelstore_attrib *srcPacking,
                        GLbitfield transferOps);


extern void
_mesa_pack_index_span(struct gl_context *ctx, GLuint n,
                      GLenum dstType, GLvoid *dest, const GLuint *source,
                      const struct gl_pixelstore_attrib *dstPacking,
                      GLbitfield transferOps);


extern void
_mesa_unpack_stencil_span(struct gl_context *ctx, GLuint n,
                          GLenum dstType, GLvoid *dest,
                          GLenum srcType, const GLvoid *source,
                          const struct gl_pixelstore_attrib *srcPacking,
                          GLbitfield transferOps);

extern void
_mesa_pack_stencil_span(struct gl_context *ctx, GLuint n,
                        GLenum dstType, GLvoid *dest, const GLubyte *source,
                        const struct gl_pixelstore_attrib *dstPacking);


extern void
_mesa_unpack_depth_span(struct gl_context *ctx, GLuint n,
                        GLenum dstType, GLvoid *dest, GLuint depthMax,
                        GLenum srcType, const GLvoid *source,
                        const struct gl_pixelstore_attrib *srcPacking);

extern void
_mesa_pack_depth_span(struct gl_context *ctx, GLuint n, GLvoid *dest,
                      GLenum dstType, const GLfloat *depthSpan,
                      const struct gl_pixelstore_attrib *dstPacking);


extern void *
_mesa_unpack_image(GLuint dimensions,
                   GLsizei width, GLsizei height, GLsizei depth,
                   GLenum format, GLenum type, const GLvoid *pixels,
                   const struct gl_pixelstore_attrib *unpack);


void
_mesa_pack_rgba_span_int(struct gl_context *ctx, GLuint n, GLuint rgba[][4],
                         GLenum dstFormat, GLenum dstType,
                         GLvoid *dstAddr);


extern void
_mesa_rebase_rgba_float(GLuint n, GLfloat rgba[][4], GLenum baseFormat);

extern void
_mesa_rebase_rgba_uint(GLuint n, GLuint rgba[][4], GLenum baseFormat);

#endif
