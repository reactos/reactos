/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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


#ifndef MIPMAP_H
#define MIPMAP_H

#include "mtypes.h"

extern void
_mesa_generate_mipmap(GLcontext *ctx, GLenum target,
                      const struct gl_texture_unit *texUnit,
                      struct gl_texture_object *texObj);


extern void
_mesa_rescale_teximage2d(GLuint bytesPerPixel,
                         GLuint srcStrideInPixels,
                         GLuint dstRowStride,
                         GLint srcWidth, GLint srcHeight,
                         GLint dstWidth, GLint dstHeight,
                         const GLvoid *srcImage, GLvoid *dstImage);

extern void
_mesa_upscale_teximage2d(GLsizei inWidth, GLsizei inHeight,
                         GLsizei outWidth, GLsizei outHeight,
                         GLint comps, const GLchan *src, GLint srcRowStride,
                         GLchan *dest);


#endif /* MIPMAP_H */
