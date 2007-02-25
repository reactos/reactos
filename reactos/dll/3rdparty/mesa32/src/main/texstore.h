/*
 * Mesa 3-D graphics library
 * Version:  6.3
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


/**
 * \file texstore.h
 * Texture image storage routines.
 *
 * \author Brian Paul
 */


#ifndef TEXSTORE_H
#define TEXSTORE_H


#include "mtypes.h"

/* Macro just to save some typing */
#define STORE_PARAMS \
	GLcontext *ctx, GLuint dims, \
	GLenum baseInternalFormat, \
	const struct gl_texture_format *dstFormat, \
	GLvoid *dstAddr, \
	GLint dstXoffset, GLint dstYoffset, GLint dstZoffset, \
	GLint dstRowStride, GLint dstImageStride, \
	GLint srcWidth, GLint srcHeight, GLint srcDepth, \
	GLenum srcFormat, GLenum srcType, \
	const GLvoid *srcAddr, \
	const struct gl_pixelstore_attrib *srcPacking


extern GLboolean _mesa_texstore_rgba(STORE_PARAMS);
extern GLboolean _mesa_texstore_color_index(STORE_PARAMS);
extern GLboolean _mesa_texstore_depth_component16(STORE_PARAMS);
extern GLboolean _mesa_texstore_depth_component_float32(STORE_PARAMS);
extern GLboolean _mesa_texstore_rgba8888(STORE_PARAMS);
extern GLboolean _mesa_texstore_argb8888(STORE_PARAMS);
extern GLboolean _mesa_texstore_rgb888(STORE_PARAMS);
extern GLboolean _mesa_texstore_bgr888(STORE_PARAMS);
extern GLboolean _mesa_texstore_rgb565(STORE_PARAMS);
extern GLboolean _mesa_texstore_rgb565_rev(STORE_PARAMS);
extern GLboolean _mesa_texstore_argb4444(STORE_PARAMS);
extern GLboolean _mesa_texstore_argb4444_rev(STORE_PARAMS);
extern GLboolean _mesa_texstore_argb1555(STORE_PARAMS);
extern GLboolean _mesa_texstore_argb1555_rev(STORE_PARAMS);
extern GLboolean _mesa_texstore_al88(STORE_PARAMS);
extern GLboolean _mesa_texstore_al88_rev(STORE_PARAMS);
extern GLboolean _mesa_texstore_rgb332(STORE_PARAMS);
extern GLboolean _mesa_texstore_a8(STORE_PARAMS);
extern GLboolean _mesa_texstore_ci8(STORE_PARAMS);
extern GLboolean _mesa_texstore_ycbcr(STORE_PARAMS);
extern GLboolean _mesa_texstore_rgba_float32(STORE_PARAMS);
extern GLboolean _mesa_texstore_rgba_float16(STORE_PARAMS);
extern GLboolean _mesa_texstore_rgb_fxt1(STORE_PARAMS);
extern GLboolean _mesa_texstore_rgba_fxt1(STORE_PARAMS);
extern GLboolean _mesa_texstore_rgb_dxt1(STORE_PARAMS);
extern GLboolean _mesa_texstore_rgba_dxt1(STORE_PARAMS);
extern GLboolean _mesa_texstore_rgba_dxt3(STORE_PARAMS);
extern GLboolean _mesa_texstore_rgba_dxt5(STORE_PARAMS);


extern GLchan *
_mesa_make_temp_chan_image(GLcontext *ctx, GLuint dims,
                           GLenum logicalBaseFormat,
                           GLenum textureBaseFormat,
                           GLint srcWidth, GLint srcHeight, GLint srcDepth,
                           GLenum srcFormat, GLenum srcType,
                           const GLvoid *srcAddr,
                           const struct gl_pixelstore_attrib *srcPacking);


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


extern void
_mesa_get_teximage(GLcontext *ctx, GLenum target, GLint level,
                   GLenum format, GLenum type, GLvoid *pixels,
                   const struct gl_texture_object *texObj,
                   const struct gl_texture_image *texImage);


extern void
_mesa_get_compressed_teximage(GLcontext *ctx, GLenum target, GLint level,
                              GLvoid *img,
                              const struct gl_texture_object *texObj,
                              const struct gl_texture_image *texImage);

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
