/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 * Copyright (c) 2008 VMware, Inc.
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
#include "formats.h"


/**
 * This macro defines the (many) parameters to the texstore functions.
 * \param dims  either 1 or 2 or 3
 * \param baseInternalFormat  user-specified base internal format
 * \param dstFormat  destination Mesa texture format
 * \param dstX/Y/Zoffset  destination x/y/z offset (ala TexSubImage), in texels
 * \param dstRowStride  destination image row stride, in bytes
 * \param dstSlices  array of addresses of image slices (for 3D, array texture)
 * \param srcWidth/Height/Depth  source image size, in pixels
 * \param srcFormat  incoming image format
 * \param srcType  incoming image data type
 * \param srcAddr  source image address
 * \param srcPacking  source image packing parameters
 */
#define TEXSTORE_PARAMS \
	struct gl_context *ctx, GLuint dims, \
	GLenum baseInternalFormat, \
	gl_format dstFormat, \
        GLint dstRowStride, \
        GLubyte **dstSlices, \
	GLint srcWidth, GLint srcHeight, GLint srcDepth, \
	GLenum srcFormat, GLenum srcType, \
	const GLvoid *srcAddr, \
	const struct gl_pixelstore_attrib *srcPacking


extern GLboolean
_mesa_texstore(TEXSTORE_PARAMS);


extern GLubyte *
_mesa_make_temp_ubyte_image(struct gl_context *ctx, GLuint dims,
                           GLenum logicalBaseFormat,
                           GLenum textureBaseFormat,
                           GLint srcWidth, GLint srcHeight, GLint srcDepth,
                           GLenum srcFormat, GLenum srcType,
                           const GLvoid *srcAddr,
                           const struct gl_pixelstore_attrib *srcPacking);

GLfloat *
_mesa_make_temp_float_image(struct gl_context *ctx, GLuint dims,
			    GLenum logicalBaseFormat,
			    GLenum textureBaseFormat,
			    GLint srcWidth, GLint srcHeight, GLint srcDepth,
			    GLenum srcFormat, GLenum srcType,
			    const GLvoid *srcAddr,
			    const struct gl_pixelstore_attrib *srcPacking,
			    GLbitfield transferOps);

extern void
_mesa_store_teximage1d(struct gl_context *ctx,
                       struct gl_texture_image *texImage,
                       GLint internalFormat,
                       GLint width, GLint border,
                       GLenum format, GLenum type, const GLvoid *pixels,
                       const struct gl_pixelstore_attrib *packing);


extern void
_mesa_store_teximage2d(struct gl_context *ctx,
                       struct gl_texture_image *texImage,
                       GLint internalFormat,
                       GLint width, GLint height, GLint border,
                       GLenum format, GLenum type, const GLvoid *pixels,
                       const struct gl_pixelstore_attrib *packing);


extern void
_mesa_store_texsubimage1d(struct gl_context *ctx,
                          struct gl_texture_image *texImage,
                          GLint xoffset, GLint width,
                          GLenum format, GLenum type, const GLvoid *pixels,
                          const struct gl_pixelstore_attrib *packing);


extern void
_mesa_store_texsubimage2d(struct gl_context *ctx,
                          struct gl_texture_image *texImage,
                          GLint xoffset, GLint yoffset,
                          GLint width, GLint height,
                          GLenum format, GLenum type, const GLvoid *pixels,
                          const struct gl_pixelstore_attrib *packing);


#endif
