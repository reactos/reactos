/**************************************************************************
 *
 * Copyright 2011 Lauri Kasanen
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
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "pipe/p_compiler.h"

#include "postprocess/filters.h"

#include "pipe/p_screen.h"
#include "util/u_inlines.h"
#include "util/u_blit.h"
#include "util/u_math.h"
#include "util/u_debug.h"
#include "util/u_memory.h"
#include "cso_cache/cso_context.h"

/** Initialize the post-processing queue. */
struct pp_queue_t *
pp_init(struct pipe_screen *pscreen, const unsigned int *enabled)
{

   unsigned int curpos = 0, i, tmp_req = 0;
   struct pp_queue_t *ppq;
   pp_func *tmp_q;

   pp_debug("Initializing the post-processing queue.\n");

   /* How many filters were requested? */
   for (i = 0; i < PP_FILTERS; i++) {
      if (enabled[i])
         curpos++;
   }
   if (!curpos)
      return NULL;

   ppq = CALLOC(1, sizeof(struct pp_queue_t));
   tmp_q = CALLOC(curpos, sizeof(pp_func));
   ppq->shaders = CALLOC(curpos, sizeof(void *));
   ppq->verts = CALLOC(curpos, sizeof(unsigned int));

   if (!tmp_q || !ppq || !ppq->shaders || !ppq->verts)
      goto error;

   ppq->p = pp_init_prog(ppq, pscreen);
   if (!ppq->p)
      goto error;

   /* Add the enabled filters to the queue, in order */
   curpos = 0;
   ppq->pp_queue = tmp_q;
   for (i = 0; i < PP_FILTERS; i++) {
      if (enabled[i]) {
         ppq->pp_queue[curpos] = pp_filters[i].main;
         tmp_req = MAX2(tmp_req, pp_filters[i].inner_tmps);

         if (pp_filters[i].shaders) {
            ppq->shaders[curpos] =
               CALLOC(pp_filters[i].shaders + 1, sizeof(void *));
            ppq->verts[curpos] = pp_filters[i].verts;
            if (!ppq->shaders[curpos])
               goto error;
         }
         pp_filters[i].init(ppq, curpos, enabled[i]);

         curpos++;
      }
   }

   ppq->p->blitctx = util_create_blit(ppq->p->pipe, ppq->p->cso);
   if (!ppq->p->blitctx)
      goto error;

   ppq->n_filters = curpos;
   ppq->n_tmp = (curpos > 2 ? 2 : 1);
   ppq->n_inner_tmp = tmp_req;

   ppq->fbos_init = false;

   for (i = 0; i < curpos; i++)
      ppq->shaders[i][0] = ppq->p->passvs;

   pp_debug("Queue successfully allocated. %u filter(s).\n", curpos);

   return ppq;

 error:
   pp_debug("Error setting up pp\n");

   if (ppq)
      FREE(ppq->p);
   FREE(ppq);
   FREE(tmp_q);

   return NULL;
}

/** Free any allocated FBOs (temp buffers). Called after resizing for example. */
void
pp_free_fbos(struct pp_queue_t *ppq)
{

   unsigned int i;

   if (!ppq->fbos_init)
      return;

   for (i = 0; i < ppq->n_tmp; i++) {
      pipe_surface_reference(&ppq->tmps[i], NULL);
      pipe_resource_reference(&ppq->tmp[i], NULL);
   }
   for (i = 0; i < ppq->n_inner_tmp; i++) {
      pipe_surface_reference(&ppq->inner_tmps[i], NULL);
      pipe_resource_reference(&ppq->inner_tmp[i], NULL);
   }
   pipe_surface_reference(&ppq->stencils, NULL);
   pipe_resource_reference(&ppq->stencil, NULL);

   ppq->fbos_init = false;
}

/** Free the pp queue. Called on context termination. */
void
pp_free(struct pp_queue_t *ppq)
{

   unsigned int i, j;

   pp_free_fbos(ppq);

   util_destroy_blit(ppq->p->blitctx);

   cso_set_fragment_sampler_views(ppq->p->cso, 0, NULL);
   cso_release_all(ppq->p->cso);

   for (i = 0; i < ppq->n_filters; i++) {
      for (j = 0; j < PP_MAX_PASSES && ppq->shaders[i][j]; j++) {
         if (j >= ppq->verts[i]) {
            ppq->p->pipe->delete_fs_state(ppq->p->pipe, ppq->shaders[i][j]);
            ppq->shaders[i][j] = NULL;
         }
         else if (ppq->shaders[i][j] != ppq->p->passvs) {
            ppq->p->pipe->delete_vs_state(ppq->p->pipe, ppq->shaders[i][j]);
            ppq->shaders[i][j] = NULL;
         }
      }
   }

   cso_destroy_context(ppq->p->cso);
   ppq->p->pipe->destroy(ppq->p->pipe);

   FREE(ppq->p);
   FREE(ppq->pp_queue);
   FREE(ppq);

   pp_debug("Queue taken down.\n");
}

/** Internal debug function. Should be available to final users. */
void
pp_debug(const char *fmt, ...)
{
   va_list ap;

   if (!debug_get_bool_option("PP_DEBUG", FALSE))
      return;

   va_start(ap, fmt);
   _debug_vprintf(fmt, ap);
   va_end(ap);
}

/** Allocate the temp FBOs. Called on makecurrent and resize. */
void
pp_init_fbos(struct pp_queue_t *ppq, unsigned int w,
             unsigned int h)
{

   struct program *p = ppq->p;  /* The lazy will inherit the earth */

   unsigned int i;
   struct pipe_resource tmp_res;

   if (ppq->fbos_init)
      return;

   pp_debug("Initializing FBOs, size %ux%u\n", w, h);
   pp_debug("Requesting %u temps and %u inner temps\n", ppq->n_tmp,
            ppq->n_inner_tmp);

   memset(&tmp_res, 0, sizeof(tmp_res));
   tmp_res.target = PIPE_TEXTURE_2D;
   tmp_res.format = p->surf.format = PIPE_FORMAT_B8G8R8A8_UNORM;
   tmp_res.width0 = w;
   tmp_res.height0 = h;
   tmp_res.depth0 = 1;
   tmp_res.array_size = 1;
   tmp_res.last_level = 0;
   tmp_res.bind = p->surf.usage = PIPE_BIND_RENDER_TARGET;

   if (!p->screen->is_format_supported(p->screen, tmp_res.format,
                                       tmp_res.target, 1, tmp_res.bind))
      pp_debug("Temp buffers' format fail\n");

   for (i = 0; i < ppq->n_tmp; i++) {
      ppq->tmp[i] = p->screen->resource_create(p->screen, &tmp_res);
      ppq->tmps[i] = p->pipe->create_surface(p->pipe, ppq->tmp[i], &p->surf);

      if (!ppq->tmp[i] || !ppq->tmps[i])
         goto error;
   }

   for (i = 0; i < ppq->n_inner_tmp; i++) {
      ppq->inner_tmp[i] = p->screen->resource_create(p->screen, &tmp_res);
      ppq->inner_tmps[i] = p->pipe->create_surface(p->pipe,
                                                   ppq->inner_tmp[i],
                                                   &p->surf);

      if (!ppq->inner_tmp[i] || !ppq->inner_tmps[i])
         goto error;
   }

   tmp_res.bind = p->surf.usage = PIPE_BIND_DEPTH_STENCIL;

   tmp_res.format = p->surf.format = PIPE_FORMAT_S8_UINT_Z24_UNORM;

   if (!p->screen->is_format_supported(p->screen, tmp_res.format,
                                       tmp_res.target, 1, tmp_res.bind)) {

      tmp_res.format = p->surf.format = PIPE_FORMAT_Z24_UNORM_S8_UINT;

      if (!p->screen->is_format_supported(p->screen, tmp_res.format,
                                          tmp_res.target, 1, tmp_res.bind))
         pp_debug("Temp Sbuffer format fail\n");
   }

   ppq->stencil = p->screen->resource_create(p->screen, &tmp_res);
   ppq->stencils = p->pipe->create_surface(p->pipe, ppq->stencil, &p->surf);
   if (!ppq->stencil || !ppq->stencils)
      goto error;


   p->framebuffer.width = w;
   p->framebuffer.height = h;

   p->viewport.scale[0] = p->viewport.translate[0] = (float) w / 2.0;
   p->viewport.scale[1] = p->viewport.translate[1] = (float) h / 2.0;
   p->viewport.scale[3] = 1.0f;
   p->viewport.translate[3] = 0.0f;

   ppq->fbos_init = true;

   return;

 error:
   pp_debug("Failed to allocate temp buffers!\n");
}
