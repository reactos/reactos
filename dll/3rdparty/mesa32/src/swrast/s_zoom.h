/*
 * Mesa 3-D graphics library
 * Version:  6.5
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

#ifndef S_ZOOM_H
#define S_ZOOM_H

#include "mtypes.h"
#include "swrast.h"


extern void
_swrast_write_zoomed_rgba_span(GLcontext *ctx, GLint imgX, GLint imgY,
                               const SWspan *span, const GLvoid *rgba);

extern void
_swrast_write_zoomed_rgb_span(GLcontext *ctx, GLint imgX, GLint imgY,
                              const SWspan *span, const GLvoid *rgb);

extern void
_swrast_write_zoomed_index_span(GLcontext *ctx, GLint imgX, GLint imgY,
                                const SWspan *span);

extern void
_swrast_write_zoomed_depth_span(GLcontext *ctx, GLint imgX, GLint imgY,
                                const SWspan *span);


extern void
_swrast_write_zoomed_stencil_span(GLcontext *ctx, GLint imgX, GLint imgY,
                                  GLint width, GLint spanX, GLint spanY,
                                  const GLstencil stencil[]);

extern void
_swrast_write_zoomed_z_span(GLcontext *ctx, GLint imgX, GLint imgY,
                            GLint width, GLint spanX, GLint spanY,
                            const GLvoid *z);


#endif
