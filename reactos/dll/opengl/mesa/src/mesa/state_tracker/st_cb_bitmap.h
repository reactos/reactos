/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#ifndef ST_CB_BITMAP_H
#define ST_CB_BITMAP_H


#include "main/compiler.h"
#include "main/mfeatures.h"

struct dd_function_table;
struct st_context;
struct gl_fragment_program;
struct st_fragment_program;

#if FEATURE_drawpix

extern void
st_init_bitmap_functions(struct dd_function_table *functions);

extern void
st_init_bitmap(struct st_context *st);

extern void
st_destroy_bitmap(struct st_context *st);

extern void
st_make_bitmap_fragment_program(struct st_context *st,
                                struct gl_fragment_program *fpIn,
                                struct gl_fragment_program **fpOut,
                                GLuint *bitmap_sampler);

extern void
st_flush_bitmap_cache(struct st_context *st);

/* Flush bitmap cache and release vertex buffer.  Needed at end of
 * frame to avoid synchronous rendering.
 */
extern void
st_flush_bitmap(struct st_context *st);

#else

static INLINE void
st_init_bitmap_functions(struct dd_function_table *functions)
{
}

static INLINE void
st_init_bitmap(struct st_context *st)
{
}

static INLINE void
st_destroy_bitmap(struct st_context *st)
{
}

static INLINE void
st_flush_bitmap_cache(struct st_context *st)
{
}

static INLINE void
st_flush_bitmap(struct st_context *st)
{
}

#endif /* FEATURE_drawpix */

#endif /* ST_CB_BITMAP_H */
