/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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

#ifndef S_ZOOM_H
#define S_ZOOM_H

#include "mtypes.h"
#include "swrast.h"

extern void
_swrast_write_zoomed_rgba_span( GLcontext *ctx, const struct sw_span *span,
                                CONST GLchan rgb[][4], GLint y0,
                                GLint skipPixels );

extern void
_swrast_write_zoomed_rgb_span( GLcontext *ctx, const struct sw_span *span,
                               CONST GLchan rgb[][3], GLint y0,
                               GLint skipPixels );

extern void
_swrast_write_zoomed_index_span( GLcontext *ctx, const struct sw_span *span,
                                 GLint y0, GLint skipPixels );

extern void
_swrast_write_zoomed_depth_span( GLcontext *ctx, const struct sw_span *span,
                                 GLint y0, GLint skipPixels );

extern void
_swrast_write_zoomed_stencil_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                                   const GLstencil stencil[], GLint y0,
                                   GLint skipPixels );

#endif
