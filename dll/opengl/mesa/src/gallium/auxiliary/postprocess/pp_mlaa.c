/**
 * Copyright (C) 2010 Jorge Jimenez (jorge@iryoku.com)
 * Copyright (C) 2010 Belen Masia (bmasia@unizar.es)
 * Copyright (C) 2010 Jose I. Echevarria (joseignacioechevarria@gmail.com)
 * Copyright (C) 2010 Fernando Navarro (fernandn@microsoft.com)
 * Copyright (C) 2010 Diego Gutierrez (diegog@unizar.es)
 * Copyright (C) 2011 Lauri Kasanen (cand@gmx.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the following statement:
 *
 *       "Uses Jimenez's MLAA. Copyright (C) 2010 by Jorge Jimenez, Belen Masia,
 *        Jose I. Echevarria, Fernando Navarro and Diego Gutierrez."
 *
 *       Only for use in the Mesa project, this point 2 is filled by naming the
 *       technique Jimenez's MLAA in the Mesa config options.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of the copyright holders.
 */

#include "pipe/p_compiler.h"

#include "postprocess/postprocess.h"
#include "postprocess/pp_mlaa.h"
#include "postprocess/pp_filters.h"
#include "util/u_blit.h"
#include "util/u_box.h"
#include "util/u_sampler.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_string.h"
#include "pipe/p_screen.h"

#define IMM_SPACE 80

static float constants[] = { 1, 1, 0, 0 };
static unsigned int dimensions[2] = { 0, 0 };

static struct pipe_resource *constbuf, *areamaptex;

/** Upload the constants. */
static void
up_consts(struct pipe_context *pipe)
{
   struct pipe_box box;

   u_box_2d(0, 0, sizeof(constants), 1, &box);
   pipe->transfer_inline_write(pipe, constbuf, 0, PIPE_TRANSFER_WRITE,
                               &box, constants, sizeof(constants),
                               sizeof(constants));
}

/** Run function of the MLAA filter. */
static void
pp_jimenezmlaa_run(struct pp_queue_t *ppq, struct pipe_resource *in,
                   struct pipe_resource *out, unsigned int n, bool iscolor)
{

   struct program *p = ppq->p;

   struct pipe_depth_stencil_alpha_state mstencil;
   struct pipe_sampler_view v_tmp, *arr[3];

   unsigned int w = p->framebuffer.width;
   unsigned int h = p->framebuffer.height;

   const struct pipe_stencil_ref ref = { {1} };
   memset(&mstencil, 0, sizeof(mstencil));
   cso_set_stencil_ref(p->cso, &ref);

   /* Init the pixel size constant */
   if (dimensions[0] != p->framebuffer.width ||
       dimensions[1] != p->framebuffer.height) {
      constants[0] = 1.0 / p->framebuffer.width;
      constants[1] = 1.0 / p->framebuffer.height;

      up_consts(p->pipe);
      dimensions[0] = p->framebuffer.width;
      dimensions[1] = p->framebuffer.height;
   }

   p->pipe->set_constant_buffer(p->pipe, PIPE_SHADER_VERTEX, 0, constbuf);
   p->pipe->set_constant_buffer(p->pipe, PIPE_SHADER_FRAGMENT, 0, constbuf);

   mstencil.stencil[0].enabled = 1;
   mstencil.stencil[0].valuemask = mstencil.stencil[0].writemask = ~0;
   mstencil.stencil[0].func = PIPE_FUNC_ALWAYS;
   mstencil.stencil[0].fail_op = PIPE_STENCIL_OP_KEEP;
   mstencil.stencil[0].zfail_op = PIPE_STENCIL_OP_KEEP;
   mstencil.stencil[0].zpass_op = PIPE_STENCIL_OP_REPLACE;

   p->framebuffer.zsbuf = ppq->stencils;

   /* First pass: depth edge detection */
   if (iscolor)
      pp_filter_setup_in(p, in);
   else
      pp_filter_setup_in(p, ppq->depth);

   pp_filter_setup_out(p, ppq->inner_tmp[0]);

   pp_filter_set_fb(p);
   pp_filter_misc_state(p);
   cso_set_depth_stencil_alpha(p->cso, &mstencil);
   p->pipe->clear(p->pipe, PIPE_CLEAR_STENCIL | PIPE_CLEAR_COLOR,
                  &p->clear_color, 0, 0);

   cso_single_sampler(p->cso, 0, &p->sampler_point);
   cso_single_sampler_done(p->cso);
   cso_set_fragment_sampler_views(p->cso, 1, &p->view);

   cso_set_vertex_shader_handle(p->cso, ppq->shaders[n][1]);    /* offsetvs */
   cso_set_fragment_shader_handle(p->cso, ppq->shaders[n][2]);

   pp_filter_draw(p);
   pp_filter_end_pass(p);


   /* Second pass: blend weights */
   /* Sampler order: areamap, edgesmap, edgesmapL (reversed, thx compiler) */
   mstencil.stencil[0].func = PIPE_FUNC_EQUAL;
   mstencil.stencil[0].zpass_op = PIPE_STENCIL_OP_KEEP;
   cso_set_depth_stencil_alpha(p->cso, &mstencil);

   pp_filter_setup_in(p, areamaptex);
   pp_filter_setup_out(p, ppq->inner_tmp[1]);

   u_sampler_view_default_template(&v_tmp, ppq->inner_tmp[0],
                                   ppq->inner_tmp[0]->format);
   arr[1] = arr[2] = p->pipe->create_sampler_view(p->pipe,
                                                  ppq->inner_tmp[0], &v_tmp);

   pp_filter_set_clear_fb(p);

   cso_single_sampler(p->cso, 0, &p->sampler_point);
   cso_single_sampler(p->cso, 1, &p->sampler_point);
   cso_single_sampler(p->cso, 2, &p->sampler);
   cso_single_sampler_done(p->cso);

   arr[0] = p->view;
   cso_set_fragment_sampler_views(p->cso, 3, arr);

   cso_set_vertex_shader_handle(p->cso, ppq->shaders[n][0]);    /* passvs */
   cso_set_fragment_shader_handle(p->cso, ppq->shaders[n][3]);

   pp_filter_draw(p);
   pp_filter_end_pass(p);
   pipe_sampler_view_reference(&arr[1], NULL);


   /* Third pass: smoothed edges */
   /* Sampler order: colormap, blendmap (wtf compiler) */
   pp_filter_setup_in(p, ppq->inner_tmp[1]);
   pp_filter_setup_out(p, out);

   pp_filter_set_fb(p);

   /* Blit the input to the output */
   util_blit_pixels(p->blitctx, in, 0, 0, 0,
                    w, h, 0, p->framebuffer.cbufs[0],
                    0, 0, w, h, 0, PIPE_TEX_MIPFILTER_NEAREST);

   u_sampler_view_default_template(&v_tmp, in, in->format);
   arr[0] = p->pipe->create_sampler_view(p->pipe, in, &v_tmp);

   cso_single_sampler(p->cso, 0, &p->sampler_point);
   cso_single_sampler(p->cso, 1, &p->sampler_point);
   cso_single_sampler_done(p->cso);

   arr[1] = p->view;
   cso_set_fragment_sampler_views(p->cso, 2, arr);

   cso_set_vertex_shader_handle(p->cso, ppq->shaders[n][1]);    /* offsetvs */
   cso_set_fragment_shader_handle(p->cso, ppq->shaders[n][4]);

   p->blend.rt[0].blend_enable = 1;
   cso_set_blend(p->cso, &p->blend);

   pp_filter_draw(p);
   pp_filter_end_pass(p);
   pipe_sampler_view_reference(&arr[0], NULL);

   p->blend.rt[0].blend_enable = 0;
   p->framebuffer.zsbuf = NULL;
}

/** The init function of the MLAA filter. */
static void
pp_jimenezmlaa_init_run(struct pp_queue_t *ppq, unsigned int n,
                        unsigned int val, bool iscolor)
{

   struct pipe_box box;
   struct pipe_resource res;
   char *tmp_text;

   constbuf = pipe_buffer_create(ppq->p->screen, PIPE_BIND_CONSTANT_BUFFER,
                                 PIPE_USAGE_STATIC, sizeof(constants));
   if (!constbuf) {
      pp_debug("Failed to allocate constant buffer\n");
      return;
   }


   pp_debug("mlaa: using %u max search steps\n", val);

   tmp_text = CALLOC(sizeof(blend2fs_1) + sizeof(blend2fs_2) +
                     IMM_SPACE, sizeof(char));

   if (!tmp_text) {
      pp_debug("Failed to allocate shader space\n");
      return;
   }
   util_sprintf(tmp_text, "%s"
                "IMM FLT32 {    %.8f,     0.0000,     0.0000,     0.0000}\n"
                "%s\n", blend2fs_1, (float) val, blend2fs_2);

   memset(&res, 0, sizeof(res));

   res.target = PIPE_TEXTURE_2D;
   res.format = PIPE_FORMAT_R8G8_UNORM;
   res.width0 = res.height0 = 165;
   res.bind = PIPE_BIND_SAMPLER_VIEW;
   res.usage = PIPE_USAGE_STATIC;
   res.depth0 = res.array_size = res.nr_samples = 1;

   if (!ppq->p->screen->is_format_supported(ppq->p->screen, res.format,
                                            res.target, 1, res.bind))
      pp_debug("Areamap format not supported\n");

   areamaptex = ppq->p->screen->resource_create(ppq->p->screen, &res);
   u_box_2d(0, 0, 165, 165, &box);

   ppq->p->pipe->transfer_inline_write(ppq->p->pipe, areamaptex, 0,
                                       PIPE_TRANSFER_WRITE, &box,
                                       areamap, 165 * 2, sizeof(areamap));



   ppq->shaders[n][1] = pp_tgsi_to_state(ppq->p->pipe, offsetvs, true,
                                         "offsetvs");
   if (iscolor)
      ppq->shaders[n][2] = pp_tgsi_to_state(ppq->p->pipe, color1fs,
                                            false, "color1fs");
   else
      ppq->shaders[n][2] = pp_tgsi_to_state(ppq->p->pipe, depth1fs,
                                            false, "depth1fs");
   ppq->shaders[n][3] = pp_tgsi_to_state(ppq->p->pipe, tmp_text, false,
                                         "blend2fs");
   ppq->shaders[n][4] = pp_tgsi_to_state(ppq->p->pipe, neigh3fs, false,
                                         "neigh3fs");

   FREE(tmp_text);
}

/** Short wrapper to init the depth version. */
void
pp_jimenezmlaa_init(struct pp_queue_t *ppq, unsigned int n, unsigned int val)
{

   pp_jimenezmlaa_init_run(ppq, n, val, false);
}

/** Short wrapper to init the color version. */
void
pp_jimenezmlaa_init_color(struct pp_queue_t *ppq, unsigned int n,
                          unsigned int val)
{

   pp_jimenezmlaa_init_run(ppq, n, val, true);
}

/** Short wrapper to run the depth version. */
void
pp_jimenezmlaa(struct pp_queue_t *ppq, struct pipe_resource *in,
               struct pipe_resource *out, unsigned int n)
{
   pp_jimenezmlaa_run(ppq, in, out, n, false);
}

/** Short wrapper to run the color version. */
void
pp_jimenezmlaa_color(struct pp_queue_t *ppq, struct pipe_resource *in,
                     struct pipe_resource *out, unsigned int n)
{
   pp_jimenezmlaa_run(ppq, in, out, n, true);
}
