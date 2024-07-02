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


#ifndef FORMAT_PACK_H
#define FORMAT_PACK_H


#include "formats.h"


/** Pack a GLubyte rgba[4] color to dest address */
typedef void (*gl_pack_ubyte_rgba_func)(const GLubyte src[4], void *dst);

/** Pack a GLfloat rgba[4] color to dest address */
typedef void (*gl_pack_float_rgba_func)(const GLfloat src[4], void *dst);

/** Pack a GLfloat Z value to dest address */
typedef void (*gl_pack_float_z_func)(const GLfloat *src, void *dst);

/** Pack a GLuint Z value to dest address */
typedef void (*gl_pack_uint_z_func)(const GLuint *src, void *dst);

/** Pack a GLubyte stencil value to dest address */
typedef void (*gl_pack_ubyte_stencil_func)(const GLubyte *src, void *dst);




extern gl_pack_ubyte_rgba_func
_mesa_get_pack_ubyte_rgba_function(gl_format format);


extern gl_pack_float_rgba_func
_mesa_get_pack_float_rgba_function(gl_format format);


extern gl_pack_float_z_func
_mesa_get_pack_float_z_func(gl_format format);


extern gl_pack_uint_z_func
_mesa_get_pack_uint_z_func(gl_format format);


extern gl_pack_ubyte_stencil_func
_mesa_get_pack_ubyte_stencil_func(gl_format format);



extern void
_mesa_pack_float_rgba_row(gl_format format, GLuint n,
                          const GLfloat src[][4], void *dst);

extern void
_mesa_pack_ubyte_rgba_row(gl_format format, GLuint n,
                          const GLubyte src[][4], void *dst);



extern void
_mesa_pack_float_z_row(gl_format format, GLuint n,
                       const GLfloat *src, void *dst);

extern void
_mesa_pack_uint_z_row(gl_format format, GLuint n,
                      const GLuint *src, void *dst);

extern void
_mesa_pack_ubyte_stencil_row(gl_format format, GLuint n,
                             const GLubyte *src, void *dst);


extern void
_mesa_pack_colormask(gl_format format, const GLubyte colorMask[4], void *dst);

#endif
