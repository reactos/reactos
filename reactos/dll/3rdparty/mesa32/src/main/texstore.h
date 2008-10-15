/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
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


/**
 * \file texstore.h
 * Texture image storage routines.
 *
 * \author Brian Paul
 */


#ifndef TEXSTORE_H
#define TEXSTORE_H


#include "mtypes.h"


extern GLboolean _mesa_texstore_rgba(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_color_index(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_rgba8888(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_argb8888(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_rgb888(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_bgr888(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_rgb565(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_rgb565_rev(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_argb4444(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_argb4444_rev(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_argb1555(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_argb1555_rev(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_al88(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_al88_rev(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_rgb332(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_a8(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_ci8(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_ycbcr(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_z24_s8(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_s8_z24(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_z16(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_z32(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_rgba_float32(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_rgba_float16(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_rgb_fxt1(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_rgba_fxt1(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_rgb_dxt1(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_rgba_dxt1(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_rgba_dxt3(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_rgba_dxt5(TEXSTORE_PARAMS);
#if FEATURE_EXT_texture_sRGB
extern GLboolean _mesa_texstore_srgb8(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_srgba8(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_sl8(TEXSTORE_PARAMS);
extern GLboolean _mesa_texstore_sla8(TEXSTORE_PARAMS);
#endif


extern GLchan *
_mesa_make_temp_chan_image(GLcontext *ctx, GLuint dims,
                           GLenum logicalBaseFormat,
                           GLenum textureBaseFormat,
                           GLint srcWidth, GLint srcHeight, GLint srcDepth,
                           GLenum srcFormat, GLenum srcType,
                           const GLvoid *srcAddr,
                           const struct gl_pixelstore_attrib *srcPacking);


extern void
_mesa_set_fetch_functions(struct gl_texture_image *texImage, GLuint dims);


extern void
_mesa_store_teximage1d(GLcontext *ctx, GLenum target, GLint level,
                       GLint internalFormat,
                       GLint width, GLint border,
                       GLenum format, GLenum type, const GLvoid *pixels,
                       const struct gl_pixelstore_attrib *packing,
                       struct gl_texture_object *texObj,
                       struct gl_texture_image *texImage);


extern void
_mesa_store_teximage2d(GLcontext *ctx, GLenum target, GLint level,
                       GLint internalFormat,
                       GLint width, GLint height, GLint border,
                       GLenum format, GLenum type, const GLvoid *pixels,
                       const struct gl_pixelstore_attrib *packing,
                       struct gl_texture_object *texObj,
                       struct gl_texture_image *texImage);


extern void
_mesa_store_teximage3d(GLcontext *ctx, GLenum target, GLint level,
                       GLint internalFormat,
                       GLint width, GLint height, GLint depth, GLint border,
                       GLenum format, GLenum type, const GLvoid *pixels,
                       const struct gl_pixelstore_attrib *packing,
                       struct gl_texture_object *texObj,
                       struct gl_texture_image *texImage);


extern void
_mesa_store_texsubimage1d(GLcontext *ctx, GLenum target, GLint level,
                          GLint xoffset, GLint width,
                          GLenum format, GLenum type, const GLvoid *pixels,
                          const struct gl_pixelstore_attrib *packing,
                          struct gl_texture_object *texObj,
                          struct gl_texture_image *texImage);


extern void
_mesa_store_texsubimage2d(GLcontext *ctx, GLenum target, GLint level,
                          GLint xoffset, GLint yoffset,
                          GLint width, GLint height,
                          GLenum format, GLenum type, const GLvoid *pixels,
                          const struct gl_pixelstore_attrib *packing,
                          struct gl_texture_object *texObj,
                          struct gl_texture_image *texImage);


extern void
_mesa_store_texsubimage3d(GLcontext *ctx, GLenum target, GLint level,
                          GLint xoffset, GLint yoffset, GLint zoffset,
                          GLint width, GLint height, GLint depth,
                          GLenum format, GLenum type, const GLvoid *pixels,
                          const struct gl_pixelstore_attrib *packing,
                          struct gl_texture_object *texObj,
                          struct gl_texture_image *texImage);


extern void
_mesa_store_compressed_teximage1d(GLcontext *ctx, GLenum target, GLint level,
                                  GLint internalFormat,
                                  GLint width, GLint border,
                                  GLsizei imageSize, const GLvoid *data,
                                  struct gl_texture_object *texObj,
                                  struct gl_texture_image *texImage);

extern void
_mesa_store_compressed_teximage2d(GLcontext *ctx, GLenum target, GLint level,
                                  GLint internalFormat,
                                  GLint width, GLint height, GLint border,
                                  GLsizei imageSize, const GLvoid *data,
                                  struct gl_texture_object *texObj,
                                  struct gl_texture_image *texImage);

extern void
_mesa_store_compressed_teximage3d(GLcontext *ctx, GLenum target, GLint level,
                                  GLint internalFormat,
                                  GLint width, GLint height, GLint depth,
                                  GLint border,
                                  GLsizei imageSize, const GLvoid *data,
                                  struct gl_texture_object *texObj,
                                  struct gl_texture_image *texImage);


extern void
_mesa_store_compressed_texsubimage1d(GLcontext *ctx, GLenum target,
                                     GLint level,
                                     GLint xoffset, GLsizei width,
                                     GLenum format,
                                     GLsizei imageSize, const GLvoid *data,
                                     struct gl_texture_object *texObj,
                                     struct gl_texture_image *texImage);

extern void
_mesa_store_compressed_texsubimage2d(GLcontext *ctx, GLenum target,
                                     GLint level,
                                     GLint xoffset, GLint yoffset,
                                     GLsizei width, GLsizei height,
                                     GLenum format,
                                     GLsizei imageSize, const GLvoid *data,
                                     struct gl_texture_object *texObj,
                                     struct gl_texture_image *texImage);

extern void
_mesa_store_compressed_texsubimage3d(GLcontext *ctx, GLenum target,
                                GLint level,
                                GLint xoffset, GLint yoffset, GLint zoffset,
                                GLsizei width, GLsizei height, GLsizei depth,
                                GLenum format,
                                GLsizei imageSize, const GLvoid *data,
                                struct gl_texture_object *texObj,
                                struct gl_texture_image *texImage);


extern void
_mesa_get_teximage(GLcontext *ctx, GLenum target, GLint level,
                   GLenum format, GLenum type, GLvoid *pixels,
                   struct gl_texture_object *texObj,
                   struct gl_texture_image *texImage);


extern void
_mesa_get_compressed_teximage(GLcontext *ctx, GLenum target, GLint level,
                              GLvoid *img,
                              struct gl_texture_object *texObj,
                              struct gl_texture_image *texImage);

extern const GLvoid *
_mesa_validate_pbo_teximage(GLcontext *ctx, GLuint dimensions,
			    GLsizei width, GLsizei height, GLsizei depth,
			    GLenum format, GLenum type, const GLvoid *pixels,
			    const struct gl_pixelstore_attrib *unpack,
			    const char *funcName);

extern const GLvoid *
_mesa_validate_pbo_compressed_teximage(GLcontext *ctx,
                                    GLsizei imageSize, const GLvoid *pixels,
                                    const struct gl_pixelstore_attrib *packing,
                                    const char *funcName);

extern void
_mesa_unmap_teximage_pbo(GLcontext *ctx,
                         const struct gl_pixelstore_attrib *unpack);


#endif
