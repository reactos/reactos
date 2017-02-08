/*
 * Mesa 3-D graphics library
 *
 * Copyright (c) 2011 VMware, Inc.
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

#ifndef FORMAT_UNPACK_H
#define FORMAT_UNPACK_H

extern void
_mesa_unpack_rgba_row(gl_format format, GLuint n,
                      const void *src, GLfloat dst[][4]);

extern void
_mesa_unpack_ubyte_rgba_row(gl_format format, GLuint n,
                            const void *src, GLubyte dst[][4]);

void
_mesa_unpack_uint_rgba_row(gl_format format, GLuint n,
                           const void *src, GLuint dst[][4]);

extern void
_mesa_unpack_rgba_block(gl_format format,
                        const void *src, GLint srcRowStride,
                        GLfloat dst[][4], GLint dstRowStride,
                        GLuint x, GLuint y, GLuint width, GLuint height);

extern void
_mesa_unpack_float_z_row(gl_format format, GLuint n,
                         const void *src, GLfloat *dst);


void
_mesa_unpack_uint_z_row(gl_format format, GLuint n,
                        const void *src, GLuint *dst);

void
_mesa_unpack_ubyte_stencil_row(gl_format format, GLuint n,
			       const void *src, GLubyte *dst);

void
_mesa_unpack_uint_24_8_depth_stencil_row(gl_format format, GLuint n,
					 const void *src, GLuint *dst);


#endif /* FORMAT_UNPACK_H */
