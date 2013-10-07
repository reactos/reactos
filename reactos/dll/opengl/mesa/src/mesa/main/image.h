/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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


#ifndef IMAGE_H
#define IMAGE_H


#include "glheader.h"

struct gl_context;
struct gl_pixelstore_attrib;

extern void
_mesa_swap2( GLushort *p, GLuint n );

extern void
_mesa_swap4( GLuint *p, GLuint n );

extern GLboolean
_mesa_type_is_packed(GLenum type);

extern GLint
_mesa_sizeof_type( GLenum type );

extern GLint
_mesa_sizeof_packed_type( GLenum type );

extern GLint
_mesa_components_in_format( GLenum format );

extern GLint
_mesa_bytes_per_pixel( GLenum format, GLenum type );

extern GLenum
_mesa_error_check_format_and_type(const struct gl_context *ctx,
                                  GLenum format, GLenum type);

extern GLboolean
_mesa_is_color_format(GLenum format);

extern GLboolean
_mesa_is_depth_format(GLenum format);

extern GLboolean
_mesa_is_stencil_format(GLenum format);

extern GLboolean
_mesa_is_ycbcr_format(GLenum format);

extern GLboolean
_mesa_is_depth_or_stencil_format(GLenum format);

extern GLboolean
_mesa_is_integer_format(GLenum format);

extern GLboolean
_mesa_base_format_has_channel(GLenum base_format, GLenum pname);

extern GLintptr
_mesa_image_offset( GLuint dimensions,
                    const struct gl_pixelstore_attrib *packing,
                    GLsizei width, GLsizei height,
                    GLenum format, GLenum type,
                    GLint img, GLint row, GLint column );

extern GLvoid *
_mesa_image_address( GLuint dimensions,
                     const struct gl_pixelstore_attrib *packing,
                     const GLvoid *image,
                     GLsizei width, GLsizei height,
                     GLenum format, GLenum type,
                     GLint img, GLint row, GLint column );

extern GLvoid *
_mesa_image_address1d( const struct gl_pixelstore_attrib *packing,
                       const GLvoid *image,
                       GLsizei width,
                       GLenum format, GLenum type,
                       GLint column );

extern GLvoid *
_mesa_image_address2d( const struct gl_pixelstore_attrib *packing,
                       const GLvoid *image,
                       GLsizei width, GLsizei height,
                       GLenum format, GLenum type,
                       GLint row, GLint column );

extern GLvoid *
_mesa_image_address3d( const struct gl_pixelstore_attrib *packing,
                       const GLvoid *image,
                       GLsizei width, GLsizei height,
                       GLenum format, GLenum type,
                       GLint img, GLint row, GLint column );


extern GLint
_mesa_image_row_stride( const struct gl_pixelstore_attrib *packing,
                        GLint width, GLenum format, GLenum type );


extern GLint
_mesa_image_image_stride( const struct gl_pixelstore_attrib *packing,
                          GLint width, GLint height,
                          GLenum format, GLenum type );


extern void
_mesa_expand_bitmap(GLsizei width, GLsizei height,
                    const struct gl_pixelstore_attrib *unpack,
                    const GLubyte *bitmap,
                    GLubyte *destBuffer, GLint destStride,
                    GLubyte onValue);


extern void
_mesa_convert_colors(GLenum srcType, const GLvoid *src,
                     GLenum dstType, GLvoid *dst,
                     GLuint count, const GLubyte mask[]);


extern GLboolean
_mesa_clip_drawpixels(const struct gl_context *ctx,
                      GLint *destX, GLint *destY,
                      GLsizei *width, GLsizei *height,
                      struct gl_pixelstore_attrib *unpack);


extern GLboolean
_mesa_clip_readpixels(const struct gl_context *ctx,
                      GLint *srcX, GLint *srcY,
                      GLsizei *width, GLsizei *height,
                      struct gl_pixelstore_attrib *pack);

extern GLboolean
_mesa_clip_copytexsubimage(const struct gl_context *ctx,
                           GLint *destX, GLint *destY,
                           GLint *srcX, GLint *srcY,
                           GLsizei *width, GLsizei *height);
                           
extern GLboolean
_mesa_clip_to_region(GLint xmin, GLint ymin,
                     GLint xmax, GLint ymax,
                     GLint *x, GLint *y,
                     GLsizei *width, GLsizei *height );

extern GLboolean
_mesa_clip_blit(struct gl_context *ctx,
                GLint *srcX0, GLint *srcY0, GLint *srcX1, GLint *srcY1,
                GLint *dstX0, GLint *dstY0, GLint *dstX1, GLint *dstY1);


#endif
