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

#ifndef TEXCOMPRESS_S3TC_H
#define TEXCOMPRESS_S3TC_H

#include "compiler.h"
#include "glheader.h"
#include "mfeatures.h"
#include "texstore.h"

struct gl_context;
struct swrast_texture_image;

#if FEATURE_texture_s3tc

extern GLboolean
_mesa_texstore_rgb_dxt1(TEXSTORE_PARAMS);

extern GLboolean
_mesa_texstore_rgba_dxt1(TEXSTORE_PARAMS);

extern GLboolean
_mesa_texstore_rgba_dxt3(TEXSTORE_PARAMS);

extern GLboolean
_mesa_texstore_rgba_dxt5(TEXSTORE_PARAMS);

extern void
_mesa_fetch_texel_2d_f_rgb_dxt1(const struct swrast_texture_image *texImage,
                                GLint i, GLint j, GLint k, GLfloat *texel);

extern void
_mesa_fetch_texel_2d_f_rgba_dxt1(const struct swrast_texture_image *texImage,
                                 GLint i, GLint j, GLint k, GLfloat *texel);

extern void
_mesa_fetch_texel_2d_f_rgba_dxt3(const struct swrast_texture_image *texImage,
                                 GLint i, GLint j, GLint k, GLfloat *texel);

extern void
_mesa_fetch_texel_2d_f_rgba_dxt5(const struct swrast_texture_image *texImage,
                                 GLint i, GLint j, GLint k, GLfloat *texel);

extern void
_mesa_fetch_texel_2d_f_srgb_dxt1(const struct swrast_texture_image *texImage,
                                 GLint i, GLint j, GLint k, GLfloat *texel);

extern void
_mesa_fetch_texel_2d_f_srgba_dxt1(const struct swrast_texture_image *texImage,
                                  GLint i, GLint j, GLint k, GLfloat *texel);

extern void
_mesa_fetch_texel_2d_f_srgba_dxt3(const struct swrast_texture_image *texImage,
                                  GLint i, GLint j, GLint k, GLfloat *texel);

extern void
_mesa_fetch_texel_2d_f_srgba_dxt5(const struct swrast_texture_image *texImage,
                                  GLint i, GLint j, GLint k, GLfloat *texel);

extern void
_mesa_init_texture_s3tc(struct gl_context *ctx);

#else /* FEATURE_texture_s3tc */

/* these are used only in texstore_funcs[] */
#define _mesa_texstore_rgb_dxt1 NULL
#define _mesa_texstore_rgba_dxt1 NULL
#define _mesa_texstore_rgba_dxt3 NULL
#define _mesa_texstore_rgba_dxt5 NULL

/* these are used only in texfetch_funcs[] */
#define _mesa_fetch_texel_2d_f_rgb_dxt1 NULL
#define _mesa_fetch_texel_2d_f_rgba_dxt1 NULL
#define _mesa_fetch_texel_2d_f_rgba_dxt3 NULL
#define _mesa_fetch_texel_2d_f_rgba_dxt5 NULL
#define _mesa_fetch_texel_2d_f_srgb_dxt1 NULL
#define _mesa_fetch_texel_2d_f_srgba_dxt1 NULL
#define _mesa_fetch_texel_2d_f_srgba_dxt3 NULL
#define _mesa_fetch_texel_2d_f_srgba_dxt5 NULL

static inline void
_mesa_init_texture_s3tc(struct gl_context *ctx)
{
}

#endif /* FEATURE_texture_s3tc */

#endif /* TEXCOMPRESS_S3TC_H */
