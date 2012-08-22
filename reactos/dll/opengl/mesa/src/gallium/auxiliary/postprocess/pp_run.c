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

#include "postprocess.h"

#include "postprocess/pp_filters.h"
#include "util/u_blit.h"
#include "util/u_inlines.h"
#include "util/u_sampler.h"

/**
*	Main run function of the PP queue. Called on swapbuffers/flush.
*
*	Runs all requested filters in order and handles shuffling the temp
*	buffers in between.
*/
void
pp_run(struct pp_queue_t *ppq, struct pipe_resource *in,
       struct pipe_resource *out, struct pipe_resource *indepth)
{
   struct pipe_resource *refin = NULL, *refout = NULL;
   unsigned int i;

   if (in->width0 != ppq->p->framebuffer.width ||
       in->height0 != ppq->p->framebuffer.height) {
      pp_debug("Resizing the temp pp buffers\n");
      pp_free_fbos(ppq);
      pp_init_fbos(ppq, in->width0, in->height0);
   }

   if (in == out && ppq->n_filters == 1) {
      /* Make a copy of in to tmp[0] in this case. */
      unsigned int w = ppq->p->framebuffer.width;
      unsigned int h = ppq->p->framebuffer.height;

      util_blit_pixels(ppq->p->blitctx, in, 0, 0, 0,
                       w, h, 0, ppq->tmps[0],
                       0, 0, w, h, 0, PIPE_TEX_MIPFILTER_NEAREST);

      in = ppq->tmp[0];
   }

   // Kept only for this frame.
   pipe_resource_reference(&ppq->depth, indepth);
   pipe_resource_reference(&refin, in);
   pipe_resource_reference(&refout, out);

   switch (ppq->n_filters) {
   case 1:                     /* No temp buf */
      ppq->pp_queue[0] (ppq, in, out, 0);
      break;
   case 2:                     /* One temp buf */

      ppq->pp_queue[0] (ppq, in, ppq->tmp[0], 0);
      ppq->pp_queue[1] (ppq, ppq->tmp[0], out, 1);

      break;
   default:                    /* Two temp bufs */
      ppq->pp_queue[0] (ppq, in, ppq->tmp[0], 0);

      for (i = 1; i < (ppq->n_filters - 1); i++) {
         if (i % 2 == 0)
            ppq->pp_queue[i] (ppq, ppq->tmp[1], ppq->tmp[0], i);

         else
            ppq->pp_queue[i] (ppq, ppq->tmp[0], ppq->tmp[1], i);
      }

      if (i % 2 == 0)
         ppq->pp_queue[i] (ppq, ppq->tmp[1], out, i);

      else
         ppq->pp_queue[i] (ppq, ppq->tmp[0], out, i);

      break;
   }

   pipe_resource_reference(&ppq->depth, NULL);
   pipe_resource_reference(&refin, NULL);
   pipe_resource_reference(&refout, NULL);
}


/* Utility functions for the filters. You're not forced to use these if */
/* your filter is more complicated. */

/** Setup this resource as the filter input. */
void
pp_filter_setup_in(struct program *p, struct pipe_resource *in)
{
   struct pipe_sampler_view v_tmp;
   u_sampler_view_default_template(&v_tmp, in, in->format);
   p->view = p->pipe->create_sampler_view(p->pipe, in, &v_tmp);
}

/** Setup this resource as the filter output. */
void
pp_filter_setup_out(struct program *p, struct pipe_resource *out)
{
   p->surf.format = out->format;
   p->surf.usage = PIPE_BIND_RENDER_TARGET;

   p->framebuffer.cbufs[0] = p->pipe->create_surface(p->pipe, out, &p->surf);
}

/** Clean up the input and output set with the above. */
void
pp_filter_end_pass(struct program *p)
{
   pipe_surface_reference(&p->framebuffer.cbufs[0], NULL);
   pipe_sampler_view_reference(&p->view, NULL);
}

/**
*	Convert the TGSI assembly to a runnable shader.
*
* We need not care about geometry shaders. All we have is screen quads.
*/
void *
pp_tgsi_to_state(struct pipe_context *pipe, const char *text, bool isvs,
                 const char *name)
{
   struct pipe_shader_state state;
   struct tgsi_token tokens[PP_MAX_TOKENS];

   if (tgsi_text_translate(text, tokens, Elements(tokens)) == FALSE) {
      pp_debug("Failed to translate %s\n", name);
      return NULL;
   }

   state.tokens = tokens;
   memset(&state.stream_output, 0, sizeof(state.stream_output));

   if (isvs)
      return pipe->create_vs_state(pipe, &state);
   else
      return pipe->create_fs_state(pipe, &state);
}

/** Setup misc state for the filter. */
void
pp_filter_misc_state(struct program *p)
{
   cso_set_blend(p->cso, &p->blend);
   cso_set_depth_stencil_alpha(p->cso, &p->depthstencil);
   cso_set_rasterizer(p->cso, &p->rasterizer);
   cso_set_viewport(p->cso, &p->viewport);

   cso_set_vertex_elements(p->cso, 2, p->velem);
}

/** Draw with the filter to the set output. */
void
pp_filter_draw(struct program *p)
{
   util_draw_vertex_buffer(p->pipe, p->cso, p->vbuf, 0,
                           PIPE_PRIM_QUADS, 4, 2);
   p->pipe->flush(p->pipe, NULL);
}

/** Set the framebuffer as active. */
void
pp_filter_set_fb(struct program *p)
{
   cso_set_framebuffer(p->cso, &p->framebuffer);
}

/** Set the framebuffer as active and clear it. */
void
pp_filter_set_clear_fb(struct program *p)
{
   cso_set_framebuffer(p->cso, &p->framebuffer);
   p->pipe->clear(p->pipe, PIPE_CLEAR_COLOR, &p->clear_color, 0, 0);
}
