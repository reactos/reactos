/*
 * Copyright (C) 2011 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file texcompress_etc.c
 * GL_OES_compressed_ETC1_RGB8_texture support.
 */


#include "mfeatures.h"
#include "texcompress.h"
#include "texcompress_etc.h"
#include "texstore.h"
#include "macros.h"
#include "swrast/s_context.h"

GLboolean
_mesa_texstore_etc1_rgb8(TEXSTORE_PARAMS)
{
   /* GL_ETC1_RGB8_OES is only valid in glCompressedTexImage2D */
   ASSERT(0);

   return GL_FALSE;
}

/* define etc1_parse_block and etc. */
#define UINT8_TYPE GLubyte
#define TAG(x) x
#include "texcompress_etc_tmp.h"
#undef TAG
#undef UINT8_TYPE

void
_mesa_fetch_texel_2d_f_etc1_rgb8(const struct swrast_texture_image *texImage,
                                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   struct etc1_block block;
   GLubyte dst[3];
   const GLubyte *src;

   src = (const GLubyte *) texImage->Map +
      (((texImage->RowStride + 3) / 4) * (j / 4) + (i / 4)) * 8;

   etc1_parse_block(&block, src);
   etc1_fetch_texel(&block, i % 4, j % 4, dst);

   texel[RCOMP] = UBYTE_TO_FLOAT(dst[0]);
   texel[GCOMP] = UBYTE_TO_FLOAT(dst[1]);
   texel[BCOMP] = UBYTE_TO_FLOAT(dst[2]);
   texel[ACOMP] = 1.0f;
}
