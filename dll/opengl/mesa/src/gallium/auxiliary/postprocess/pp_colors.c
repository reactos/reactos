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

#include "postprocess/postprocess.h"
#include "postprocess/pp_colors.h"
#include "postprocess/pp_filters.h"

/** The run function of the color filters */
void
pp_nocolor(struct pp_queue_t *ppq, struct pipe_resource *in,
           struct pipe_resource *out, unsigned int n)
{

   struct program *p = ppq->p;

   pp_filter_setup_in(p, in);
   pp_filter_setup_out(p, out);

   pp_filter_set_fb(p);
   pp_filter_misc_state(p);

   cso_single_sampler(p->cso, 0, &p->sampler_point);
   cso_single_sampler_done(p->cso);
   cso_set_fragment_sampler_views(p->cso, 1, &p->view);

   cso_set_vertex_shader_handle(p->cso, ppq->shaders[n][0]);
   cso_set_fragment_shader_handle(p->cso, ppq->shaders[n][1]);

   pp_filter_draw(p);
   pp_filter_end_pass(p);
}


/* Init functions */

void
pp_nored_init(struct pp_queue_t *ppq, unsigned int n, unsigned int val)
{
   ppq->shaders[n][1] = pp_tgsi_to_state(ppq->p->pipe, nored, false, "nored");
}


void
pp_nogreen_init(struct pp_queue_t *ppq, unsigned int n, unsigned int val)
{
   ppq->shaders[n][1] =
      pp_tgsi_to_state(ppq->p->pipe, nogreen, false, "nogreen");
}


void
pp_noblue_init(struct pp_queue_t *ppq, unsigned int n, unsigned int val)
{
   ppq->shaders[n][1] =
      pp_tgsi_to_state(ppq->p->pipe, noblue, false, "noblue");
}
