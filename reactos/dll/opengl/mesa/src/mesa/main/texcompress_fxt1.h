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

#ifndef TEXCOMPRESS_FXT1_H
#define TEXCOMPRESS_FXT1_H

#include "glheader.h"
#include "mfeatures.h"
#include "texstore.h"

struct swrast_texture_image;

#if FEATURE_texture_fxt1

extern GLboolean
_mesa_texstore_rgb_fxt1(TEXSTORE_PARAMS);

extern GLboolean
_mesa_texstore_rgba_fxt1(TEXSTORE_PARAMS);

extern void
_mesa_fetch_texel_2d_f_rgba_fxt1(const struct swrast_texture_image *texImage,
                                 GLint i, GLint j, GLint k, GLfloat *texel);

extern void
_mesa_fetch_texel_2d_f_rgb_fxt1(const struct swrast_texture_image *texImage,
                                GLint i, GLint j, GLint k, GLfloat *texel);

#else /* FEATURE_texture_fxt1 */

/* these are used only in texstore_funcs[] */
#define _mesa_texstore_rgb_fxt1 NULL
#define _mesa_texstore_rgba_fxt1 NULL

/* these are used only in texfetch_funcs[] */
#define _mesa_fetch_texel_2d_f_rgba_fxt1 NULL
#define _mesa_fetch_texel_2d_f_rgb_fxt1 NULL

#endif /* FEATURE_texture_fxt1 */

#endif /* TEXCOMPRESS_FXT1_H */
