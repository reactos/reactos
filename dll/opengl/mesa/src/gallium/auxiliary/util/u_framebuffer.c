/**************************************************************************
 *
 * Copyright 2009-2010 VMware, Inc.  All Rights Reserved.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * @file
 * Framebuffer utility functions.
 *  
 * @author Brian Paul
 */


#include "pipe/p_screen.h"
#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"

#include "util/u_memory.h"
#include "util/u_framebuffer.h"


/**
 * Compare pipe_framebuffer_state objects.
 * \return TRUE if same, FALSE if different
 */
boolean
util_framebuffer_state_equal(const struct pipe_framebuffer_state *dst,
                             const struct pipe_framebuffer_state *src)
{
   unsigned i;

   if (dst->width != src->width ||
       dst->height != src->height)
      return FALSE;

   for (i = 0; i < Elements(src->cbufs); i++) {
      if (dst->cbufs[i] != src->cbufs[i]) {
         return FALSE;
      }
   }

   if (dst->nr_cbufs != src->nr_cbufs) {
      return FALSE;
   }

   if (dst->zsbuf != src->zsbuf) {
      return FALSE;
   }

   return TRUE;
}


/**
 * Copy framebuffer state from src to dst, updating refcounts.
 */
void
util_copy_framebuffer_state(struct pipe_framebuffer_state *dst,
                            const struct pipe_framebuffer_state *src)
{
   unsigned i;

   dst->width = src->width;
   dst->height = src->height;

   for (i = 0; i < src->nr_cbufs; i++)
      pipe_surface_reference(&dst->cbufs[i], src->cbufs[i]);

   for (i = src->nr_cbufs; i < dst->nr_cbufs; i++)
      pipe_surface_reference(&dst->cbufs[i], NULL);

   dst->nr_cbufs = src->nr_cbufs;

   pipe_surface_reference(&dst->zsbuf, src->zsbuf);
}


void
util_unreference_framebuffer_state(struct pipe_framebuffer_state *fb)
{
   unsigned i;

   for (i = 0; i < fb->nr_cbufs; i++) {
      pipe_surface_reference(&fb->cbufs[i], NULL);
   }

   pipe_surface_reference(&fb->zsbuf, NULL);

   fb->width = fb->height = 0;
   fb->nr_cbufs = 0;
}


/* Where multiple sizes are allowed for framebuffer surfaces, find the
 * minimum width and height of all bound surfaces.
 */
boolean
util_framebuffer_min_size(const struct pipe_framebuffer_state *fb,
                          unsigned *width,
                          unsigned *height)
{
   unsigned w = ~0;
   unsigned h = ~0;
   unsigned i;

   for (i = 0; i < fb->nr_cbufs; i++) {
      w = MIN2(w, fb->cbufs[i]->width);
      h = MIN2(h, fb->cbufs[i]->height);
   }

   if (fb->zsbuf) {
      w = MIN2(w, fb->zsbuf->width);
      h = MIN2(h, fb->zsbuf->height);
   }

   if (w == ~0) {
      *width = 0;
      *height = 0;
      return FALSE;
   }
   else {
      *width = w;
      *height = h;
      return TRUE;
   }
}
