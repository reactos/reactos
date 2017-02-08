/**************************************************************************
 * 
 * Copyright 2010 VMware, Inc.
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
 * IN NO EVENT SHALL THE AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#include "st_context.h"
#include "pipe/p_context.h"
#include "st_atom.h"

#include "cso_cache/cso_context.h"


/* Second state atom for user clip planes:
 */
static void update_sample_mask( struct st_context *st )
{
   unsigned sample_mask = 0xffffffff;
   unsigned sample_count = 1;
   struct pipe_framebuffer_state *framebuffer = &st->state.framebuffer;

   /* dependency here on bound surface (or rather, sample count) is worrying */
   if (framebuffer->zsbuf)
      sample_count = framebuffer->zsbuf->texture->nr_samples;
   else if (framebuffer->cbufs[0])
      sample_count = framebuffer->cbufs[0]->texture->nr_samples;

   if (st->ctx->Multisample.Enabled && sample_count > 1) {
   /* unlike in gallium/d3d10 the mask is only active if msaa is enabled */
      if (st->ctx->Multisample.SampleCoverage) {
         unsigned nr_bits;
         nr_bits = st->ctx->Multisample.SampleCoverageValue * (float)sample_count;
         /* there's lot of ways how to do this. We just use first few bits,
            since we have no knowledge of sample positions here. When
            app-supplied mask though is used too might need to be smarter.
            Also, there's a interface restriction here in theory it is
            encouraged this mask not be the same at each pixel. */
         sample_mask = (1 << nr_bits) - 1;
         if (st->ctx->Multisample.SampleCoverageInvert)
            sample_mask = ~sample_mask;
      }
      /* TODO merge with app-supplied sample mask */
   }

   /* mask off unused bits or don't care? */

   if (sample_mask != st->state.sample_mask) {
      st->state.sample_mask = sample_mask;
      cso_set_sample_mask(st->cso_context, sample_mask);
   }
}


const struct st_tracked_state st_update_msaa = {
   "st_update_msaa",					/* name */
   {							/* dirty */
      (_NEW_MULTISAMPLE | _NEW_BUFFERS),		/* mesa */
      ST_NEW_FRAMEBUFFER,				/* st */
   },
   update_sample_mask					/* update */
};
