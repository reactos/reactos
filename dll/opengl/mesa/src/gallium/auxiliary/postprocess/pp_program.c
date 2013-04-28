/**************************************************************************
 *
 * Copyright 2010 Jakob Bornecrantz
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
#include "cso_cache/cso_context.h"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_inlines.h"
#include "util/u_simple_shaders.h"
#include "util/u_memory.h"

/** Initialize the internal details */
struct program *
pp_init_prog(struct pp_queue_t *ppq, struct pipe_screen *pscreen)
{

   struct program *p;

   pp_debug("Initializing program\n");
   if (!pscreen)
      return NULL;

   p = CALLOC(1, sizeof(struct program));
   if (!p)
      return NULL;

   p->screen = pscreen;
   p->pipe = pscreen->context_create(pscreen, NULL);
   p->cso = cso_create_context(p->pipe);

   {
      static const float verts[4][2][4] = {
         {
          {1.0f, 1.0f, 0.0f, 1.0f},
          {1.0f, 1.0f, 0.0f, 1.0f}
          },
         {
          {-1.0f, 1.0f, 0.0f, 1.0f},
          {0.0f, 1.0f, 0.0f, 1.0f}
          },
         {
          {-1.0f, -1.0f, 0.0f, 1.0f},
          {0.0f, 0.0f, 0.0f, 1.0f}
          },
         {
          {1.0f, -1.0f, 0.0f, 1.0f},
          {1.0f, 0.0f, 0.0f, 1.0f}
          }
      };

      p->vbuf = pipe_buffer_create(pscreen, PIPE_BIND_VERTEX_BUFFER,
                                   PIPE_USAGE_STATIC, sizeof(verts));
      pipe_buffer_write(p->pipe, p->vbuf, 0, sizeof(verts), verts);
   }

   p->blend.rt[0].colormask = PIPE_MASK_RGBA;
   p->blend.rt[0].rgb_src_factor = p->blend.rt[0].alpha_src_factor =
      PIPE_BLENDFACTOR_SRC_ALPHA;
   p->blend.rt[0].rgb_dst_factor = p->blend.rt[0].alpha_dst_factor =
      PIPE_BLENDFACTOR_INV_SRC_ALPHA;

   p->rasterizer.cull_face = PIPE_FACE_NONE;
   p->rasterizer.gl_rasterization_rules = 1;
   p->rasterizer.depth_clip = 1;

   p->sampler.wrap_s = p->sampler.wrap_t = p->sampler.wrap_r =
      PIPE_TEX_WRAP_CLAMP_TO_EDGE;

   p->sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   p->sampler.min_img_filter = p->sampler.mag_img_filter =
      PIPE_TEX_FILTER_LINEAR;
   p->sampler.normalized_coords = 1;

   p->sampler_point.wrap_s = p->sampler_point.wrap_t =
      p->sampler_point.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   p->sampler_point.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   p->sampler_point.min_img_filter = p->sampler_point.mag_img_filter =
      PIPE_TEX_FILTER_NEAREST;
   p->sampler_point.normalized_coords = 1;

   p->velem[0].src_offset = 0;
   p->velem[0].instance_divisor = 0;
   p->velem[0].vertex_buffer_index = 0;
   p->velem[0].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   p->velem[1].src_offset = 1 * 4 * sizeof(float);
   p->velem[1].instance_divisor = 0;
   p->velem[1].vertex_buffer_index = 0;
   p->velem[1].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;

   if (!p->screen->is_format_supported(p->screen,
                                       PIPE_FORMAT_R32G32B32A32_FLOAT,
                                       PIPE_BUFFER, 1,
                                       PIPE_BIND_VERTEX_BUFFER))
      pp_debug("Vertex buf format fail\n");


   {
      const uint semantic_names[] = { TGSI_SEMANTIC_POSITION,
         TGSI_SEMANTIC_GENERIC
      };
      const uint semantic_indexes[] = { 0, 0 };
      p->passvs = util_make_vertex_passthrough_shader(p->pipe, 2,
                                                      semantic_names,
                                                      semantic_indexes);
   }

   p->framebuffer.nr_cbufs = 1;

   p->surf.usage = PIPE_BIND_RENDER_TARGET;
   p->surf.format = PIPE_FORMAT_B8G8R8A8_UNORM;

   p->pipe->set_sample_mask(p->pipe, ~0);

   return p;
}
