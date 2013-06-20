/**************************************************************************
 *
 * Copyright 2010 Luca Barbieri
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "util/u_staging.h"
#include "pipe/p_context.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"

static void
util_staging_resource_template(struct pipe_resource *pt, unsigned width, unsigned height, unsigned depth, struct pipe_resource *template)
{
   memset(template, 0, sizeof(struct pipe_resource));
   if(pt->target != PIPE_BUFFER && depth <= 1)
      template->target = PIPE_TEXTURE_RECT;
   else
      template->target = pt->target;
   template->format = pt->format;
   template->width0 = width;
   template->height0 = height;
   template->depth0 = depth;
   template->array_size = 1;
   template->last_level = 0;
   template->nr_samples = pt->nr_samples;
   template->bind = 0;
   template->usage = PIPE_USAGE_STAGING;
   template->flags = 0;
}

struct util_staging_transfer *
util_staging_transfer_init(struct pipe_context *pipe,
           struct pipe_resource *pt,
           unsigned level,
           unsigned usage,
           const struct pipe_box *box,
           boolean direct, struct util_staging_transfer *tx)
{
   struct pipe_screen *pscreen = pipe->screen;

   struct pipe_resource staging_resource_template;

   pipe_resource_reference(&tx->base.resource, pt);
   tx->base.level = level;
   tx->base.usage = usage;
   tx->base.box = *box;

   if (direct)
   {
      tx->staging_resource = pt;
      return tx;
   }

   util_staging_resource_template(pt, box->width, box->height, box->depth, &staging_resource_template);
   tx->staging_resource = pscreen->resource_create(pscreen, &staging_resource_template);
   if (!tx->staging_resource)
   {
      pipe_resource_reference(&tx->base.resource, NULL);
      FREE(tx);
      return NULL;
   }

   if (usage & PIPE_TRANSFER_READ)
   {
      /* XXX this looks wrong dst is always the same but looping over src z? */
      unsigned zi;
      struct pipe_box sbox;
      sbox.x = box->x;
      sbox.y = box->y;
      sbox.z = box->z;
      sbox.width = box->width;
      sbox.height = box->height;
      sbox.depth = 1;
      for(zi = 0; zi < box->depth; ++zi) {
         sbox.z = sbox.z + zi;
         pipe->resource_copy_region(pipe, tx->staging_resource, 0, 0, 0, 0,
                                    tx->base.resource, level, &sbox);
      }
   }

   return tx;
}

void
util_staging_transfer_destroy(struct pipe_context *pipe, struct pipe_transfer *ptx)
{
   struct util_staging_transfer *tx = (struct util_staging_transfer *)ptx;

   if (tx->staging_resource != tx->base.resource)
   {
      if(tx->base.usage & PIPE_TRANSFER_WRITE) {
         /* XXX this looks wrong src is always the same but looping over dst z? */
         unsigned zi;
         struct pipe_box sbox;
         sbox.x = 0;
         sbox.y = 0;
         sbox.z = 0;
         sbox.width = tx->base.box.width;
         sbox.height = tx->base.box.height;
         sbox.depth = 1;
         for(zi = 0; zi < tx->base.box.depth; ++zi)
            pipe->resource_copy_region(pipe, tx->base.resource, tx->base.level, tx->base.box.x, tx->base.box.y, tx->base.box.z + zi,
                                       tx->staging_resource, 0, &sbox);
      }

      pipe_resource_reference(&tx->staging_resource, NULL);
   }

   pipe_resource_reference(&ptx->resource, NULL);
   FREE(ptx);
}
