
/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
 *
 * Authors:
 *    Gareth Hughes
 */


#ifndef TEXUTIL_H
#define TEXUTIL_H

#include "mtypes.h"
#include "texformat.h"

extern GLboolean
_mesa_convert_texsubimage1d( GLint mesaFormat,
			     GLint xoffset,
			     GLint width,
			     GLenum format, GLenum type,
			     const struct gl_pixelstore_attrib *packing,
			     const GLvoid *srcImage, GLvoid *dstImage );

extern GLboolean
_mesa_convert_texsubimage2d( GLint mesaFormat,
			     GLint xoffset, GLint yoffset,
			     GLint width, GLint height,
			     GLint imageWidth,
			     GLenum format, GLenum type,
			     const struct gl_pixelstore_attrib *packing,
			     const GLvoid *srcImage, GLvoid *dstImage );

extern GLboolean
_mesa_convert_texsubimage3d( GLint mesaFormat,
			     GLint xoffset, GLint yoffset, GLint zoffset,
			     GLint width, GLint height, GLint depth,
			     GLint imageWidth, GLint imageHeight,
			     GLenum format, GLenum type,
			     const struct gl_pixelstore_attrib *packing,
			     const GLvoid *srcImage, GLvoid *dstImage );

/* Nearest filtering only (for broken hardware that can't support
 * all aspect ratios).  FIXME: Make this a subimage update as well...
 */
extern void
_mesa_rescale_teximage2d( GLuint bytesPerPixel, GLuint dstRowStride,
			  GLint srcWidth, GLint srcHeight,
			  GLint dstWidth, GLint dstHeight,
			  const GLvoid *srcImage, GLvoid *dstImage );


#endif
