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

#ifndef P_TILE_H
#define P_TILE_H

#include "pipe/p_compiler.h"
#include "pipe/p_format.h"
#include "pipe/p_state.h"

struct pipe_context;
struct pipe_transfer;

/**
 * Clip tile against transfer dims.
 *
 * XXX: this only clips width and height!
 *
 * \return TRUE if tile is totally clipped, FALSE otherwise
 */
static INLINE boolean
u_clip_tile(uint x, uint y, uint *w, uint *h, const struct pipe_box *box)
{
   if (x >= box->width)
      return TRUE;
   if (y >= box->height)
      return TRUE;
   if (x + *w > box->width)
      *w = box->width - x;
   if (y + *h > box->height)
      *h = box->height - y;
   return FALSE;
}

#ifdef __cplusplus
extern "C" {
#endif

void
pipe_get_tile_raw(struct pipe_context *pipe,
                  struct pipe_transfer *pt,
                  uint x, uint y, uint w, uint h,
                  void *p, int dst_stride);

void
pipe_put_tile_raw(struct pipe_context *pipe,
                  struct pipe_transfer *pt,
                  uint x, uint y, uint w, uint h,
                  const void *p, int src_stride);


void
pipe_get_tile_rgba(struct pipe_context *pipe,
                   struct pipe_transfer *pt,
                   uint x, uint y, uint w, uint h,
                   float *p);

void
pipe_get_tile_rgba_format(struct pipe_context *pipe,
                          struct pipe_transfer *pt,
                          uint x, uint y, uint w, uint h,
                          enum pipe_format format,
                          float *p);

void
pipe_put_tile_rgba(struct pipe_context *pipe,
                   struct pipe_transfer *pt,
                   uint x, uint y, uint w, uint h,
                   const float *p);

void
pipe_put_tile_rgba_format(struct pipe_context *pipe,
                          struct pipe_transfer *pt,
                          uint x, uint y, uint w, uint h,
                          enum pipe_format format,
                          const float *p);


void
pipe_get_tile_z(struct pipe_context *pipe,
                struct pipe_transfer *pt,
                uint x, uint y, uint w, uint h,
                uint *z);

void
pipe_put_tile_z(struct pipe_context *pipe,
                struct pipe_transfer *pt,
                uint x, uint y, uint w, uint h,
                const uint *z);

void
pipe_tile_raw_to_rgba(enum pipe_format format,
                      void *src,
                      uint w, uint h,
                      float *dst, unsigned dst_stride);

void
pipe_tile_raw_to_unsigned(enum pipe_format format,
                          void *src,
                          uint w, uint h,
                          unsigned *dst, unsigned dst_stride);

void
pipe_tile_raw_to_signed(enum pipe_format format,
                        void *src,
                        uint w, uint h,
                        int *dst, unsigned dst_stride);

void
pipe_get_tile_ui_format(struct pipe_context *pipe,
                        struct pipe_transfer *pt,
                        uint x, uint y, uint w, uint h,
                        enum pipe_format format,
                        unsigned int *p);

void
pipe_get_tile_i_format(struct pipe_context *pipe,
                       struct pipe_transfer *pt,
                       uint x, uint y, uint w, uint h,
                       enum pipe_format format,
                       int *p);

void
pipe_put_tile_ui_format(struct pipe_context *pipe,
                        struct pipe_transfer *pt,
                        uint x, uint y, uint w, uint h,
                        enum pipe_format format,
                        const unsigned *p);

void
pipe_put_tile_i_format(struct pipe_context *pipe,
                       struct pipe_transfer *pt,
                       uint x, uint y, uint w, uint h,
                       enum pipe_format format,
                       const int *p);

#ifdef __cplusplus
}
#endif

#endif
