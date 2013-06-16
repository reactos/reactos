/**************************************************************************
 *
 * Copyright 2011 Christian König
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

#include <assert.h>

#include "pipe/p_screen.h"
#include "pipe/p_context.h"

#include "util/u_draw.h"
#include "util/u_sampler.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"

#include "tgsi/tgsi_ureg.h"

#include "vl_defines.h"
#include "vl_types.h"

#include "vl_zscan.h"
#include "vl_vertex_buffers.h"

enum VS_OUTPUT
{
   VS_O_VPOS,
   VS_O_VTEX
};

const int vl_zscan_linear[] =
{
   /* Linear scan pattern */
    0, 1, 2, 3, 4, 5, 6, 7,
    8, 9,10,11,12,13,14,15,
   16,17,18,19,20,21,22,23,
   24,25,26,27,28,29,30,31,
   32,33,34,35,36,37,38,39,
   40,41,42,43,44,45,46,47,
   48,49,50,51,52,53,54,55,
   56,57,58,59,60,61,62,63
};

const int vl_zscan_normal[] =
{
   /* Zig-Zag scan pattern */
    0, 1, 8,16, 9, 2, 3,10,
   17,24,32,25,18,11, 4, 5,
   12,19,26,33,40,48,41,34,
   27,20,13, 6, 7,14,21,28,
   35,42,49,56,57,50,43,36,
   29,22,15,23,30,37,44,51,
   58,59,52,45,38,31,39,46,
   53,60,61,54,47,55,62,63
};

const int vl_zscan_alternate[] =
{
   /* Alternate scan pattern */
    0, 8,16,24, 1, 9, 2,10,
   17,25,32,40,48,56,57,49,
   41,33,26,18, 3,11, 4,12,
   19,27,34,42,50,58,35,43,
   51,59,20,28, 5,13, 6,14,
   21,29,36,44,52,60,37,45,
   53,61,22,30, 7,15,23,31,
   38,46,54,62,39,47,55,63
};

static void *
create_vert_shader(struct vl_zscan *zscan)
{
   struct ureg_program *shader;

   struct ureg_src scale;
   struct ureg_src vrect, vpos, block_num;

   struct ureg_dst tmp;
   struct ureg_dst o_vpos;
   struct ureg_dst *o_vtex;

   signed i;

   shader = ureg_create(TGSI_PROCESSOR_VERTEX);
   if (!shader)
      return NULL;

   o_vtex = MALLOC(zscan->num_channels * sizeof(struct ureg_dst));

   scale = ureg_imm2f(shader,
      (float)BLOCK_WIDTH / zscan->buffer_width,
      (float)BLOCK_HEIGHT / zscan->buffer_height);

   vrect = ureg_DECL_vs_input(shader, VS_I_RECT);
   vpos = ureg_DECL_vs_input(shader, VS_I_VPOS);
   block_num = ureg_DECL_vs_input(shader, VS_I_BLOCK_NUM);

   tmp = ureg_DECL_temporary(shader);

   o_vpos = ureg_DECL_output(shader, TGSI_SEMANTIC_POSITION, VS_O_VPOS);

   for (i = 0; i < zscan->num_channels; ++i)
      o_vtex[i] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_VTEX + i);

   /*
    * o_vpos.xy = (vpos + vrect) * scale
    * o_vpos.zw = 1.0f
    *
    * tmp.xy = InstanceID / blocks_per_line
    * tmp.x = frac(tmp.x)
    * tmp.y = floor(tmp.y)
    *
    * o_vtex.x = vrect.x / blocks_per_line + tmp.x
    * o_vtex.y = vrect.y
    * o_vtex.z = tmp.z * blocks_per_line / blocks_total
    */
   ureg_ADD(shader, ureg_writemask(tmp, TGSI_WRITEMASK_XY), vpos, vrect);
   ureg_MUL(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_XY), ureg_src(tmp), scale);
   ureg_MOV(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_ZW), ureg_imm1f(shader, 1.0f));

   ureg_MUL(shader, ureg_writemask(tmp, TGSI_WRITEMASK_XW), ureg_scalar(block_num, TGSI_SWIZZLE_X),
            ureg_imm1f(shader, 1.0f / zscan->blocks_per_line));

   ureg_FRC(shader, ureg_writemask(tmp, TGSI_WRITEMASK_Y), ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_X));
   ureg_FLR(shader, ureg_writemask(tmp, TGSI_WRITEMASK_W), ureg_src(tmp));

   for (i = 0; i < zscan->num_channels; ++i) {
      ureg_ADD(shader, ureg_writemask(tmp, TGSI_WRITEMASK_X), ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_Y),
               ureg_imm1f(shader, 1.0f / (zscan->blocks_per_line * BLOCK_WIDTH) * (i - (signed)zscan->num_channels / 2)));

      ureg_MAD(shader, ureg_writemask(o_vtex[i], TGSI_WRITEMASK_X), vrect,
               ureg_imm1f(shader, 1.0f / zscan->blocks_per_line), ureg_src(tmp));
      ureg_MOV(shader, ureg_writemask(o_vtex[i], TGSI_WRITEMASK_Y), vrect);
      ureg_MOV(shader, ureg_writemask(o_vtex[i], TGSI_WRITEMASK_Z), vpos);
      ureg_MUL(shader, ureg_writemask(o_vtex[i], TGSI_WRITEMASK_W), ureg_src(tmp),
               ureg_imm1f(shader, (float)zscan->blocks_per_line / zscan->blocks_total));
   }

   ureg_release_temporary(shader, tmp);
   ureg_END(shader);

   FREE(o_vtex);

   return ureg_create_shader_and_destroy(shader, zscan->pipe);
}

static void *
create_frag_shader(struct vl_zscan *zscan)
{
   struct ureg_program *shader;
   struct ureg_src *vtex;

   struct ureg_src samp_src, samp_scan, samp_quant;

   struct ureg_dst *tmp;
   struct ureg_dst quant, fragment;

   unsigned i;

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return NULL;

   vtex = MALLOC(zscan->num_channels * sizeof(struct ureg_src));
   tmp = MALLOC(zscan->num_channels * sizeof(struct ureg_dst));

   for (i = 0; i < zscan->num_channels; ++i)
      vtex[i] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_VTEX + i, TGSI_INTERPOLATE_LINEAR);

   samp_src = ureg_DECL_sampler(shader, 0);
   samp_scan = ureg_DECL_sampler(shader, 1);
   samp_quant = ureg_DECL_sampler(shader, 2);

   for (i = 0; i < zscan->num_channels; ++i)
      tmp[i] = ureg_DECL_temporary(shader);
   quant = ureg_DECL_temporary(shader);

   fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);

   /*
    * tmp.x = tex(vtex, 1)
    * tmp.y = vtex.z
    * fragment = tex(tmp, 0) * quant
    */
   for (i = 0; i < zscan->num_channels; ++i)
      ureg_TEX(shader, ureg_writemask(tmp[i], TGSI_WRITEMASK_X), TGSI_TEXTURE_2D, vtex[i], samp_scan);

   for (i = 0; i < zscan->num_channels; ++i)
      ureg_MOV(shader, ureg_writemask(tmp[i], TGSI_WRITEMASK_Y), ureg_scalar(vtex[i], TGSI_SWIZZLE_W));

   for (i = 0; i < zscan->num_channels; ++i) {
      ureg_TEX(shader, ureg_writemask(tmp[0], TGSI_WRITEMASK_X << i), TGSI_TEXTURE_2D, ureg_src(tmp[i]), samp_src);
      ureg_TEX(shader, ureg_writemask(quant, TGSI_WRITEMASK_X << i), TGSI_TEXTURE_3D, vtex[i], samp_quant);
   }

   ureg_MUL(shader, quant, ureg_src(quant), ureg_imm1f(shader, 16.0f));
   ureg_MUL(shader, fragment, ureg_src(tmp[0]), ureg_src(quant));

   for (i = 0; i < zscan->num_channels; ++i)
      ureg_release_temporary(shader, tmp[i]);
   ureg_END(shader);

   FREE(vtex);
   FREE(tmp);

   return ureg_create_shader_and_destroy(shader, zscan->pipe);
}

static bool
init_shaders(struct vl_zscan *zscan)
{
   assert(zscan);

   zscan->vs = create_vert_shader(zscan);
   if (!zscan->vs)
      goto error_vs;

   zscan->fs = create_frag_shader(zscan);
   if (!zscan->fs)
      goto error_fs;

   return true;

error_fs:
   zscan->pipe->delete_vs_state(zscan->pipe, zscan->vs);

error_vs:
   return false;
}

static void
cleanup_shaders(struct vl_zscan *zscan)
{
   assert(zscan);

   zscan->pipe->delete_vs_state(zscan->pipe, zscan->vs);
   zscan->pipe->delete_fs_state(zscan->pipe, zscan->fs);
}

static bool
init_state(struct vl_zscan *zscan)
{
   struct pipe_blend_state blend;
   struct pipe_rasterizer_state rs_state;
   struct pipe_sampler_state sampler;
   unsigned i;

   assert(zscan);

   memset(&rs_state, 0, sizeof(rs_state));
   rs_state.gl_rasterization_rules = true;
   rs_state.depth_clip = 1;
   zscan->rs_state = zscan->pipe->create_rasterizer_state(zscan->pipe, &rs_state);
   if (!zscan->rs_state)
      goto error_rs_state;

   memset(&blend, 0, sizeof blend);

   blend.independent_blend_enable = 0;
   blend.rt[0].blend_enable = 0;
   blend.rt[0].rgb_func = PIPE_BLEND_ADD;
   blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].alpha_func = PIPE_BLEND_ADD;
   blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ONE;
   blend.logicop_enable = 0;
   blend.logicop_func = PIPE_LOGICOP_CLEAR;
   /* Needed to allow color writes to FB, even if blending disabled */
   blend.rt[0].colormask = PIPE_MASK_RGBA;
   blend.dither = 0;
   zscan->blend = zscan->pipe->create_blend_state(zscan->pipe, &blend);
   if (!zscan->blend)
      goto error_blend;

   for (i = 0; i < 3; ++i) {
      memset(&sampler, 0, sizeof(sampler));
      sampler.wrap_s = PIPE_TEX_WRAP_REPEAT;
      sampler.wrap_t = PIPE_TEX_WRAP_REPEAT;
      sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
      sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
      sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
      sampler.compare_mode = PIPE_TEX_COMPARE_NONE;
      sampler.compare_func = PIPE_FUNC_ALWAYS;
      sampler.normalized_coords = 1;
      zscan->samplers[i] = zscan->pipe->create_sampler_state(zscan->pipe, &sampler);
      if (!zscan->samplers[i])
         goto error_samplers;
   }

   return true;

error_samplers:
   for (i = 0; i < 2; ++i)
      if (zscan->samplers[i])
         zscan->pipe->delete_sampler_state(zscan->pipe, zscan->samplers[i]);

   zscan->pipe->delete_rasterizer_state(zscan->pipe, zscan->rs_state);

error_blend:
   zscan->pipe->delete_blend_state(zscan->pipe, zscan->blend);

error_rs_state:
   return false;
}

static void
cleanup_state(struct vl_zscan *zscan)
{
   unsigned i;

   assert(zscan);

   for (i = 0; i < 3; ++i)
      zscan->pipe->delete_sampler_state(zscan->pipe, zscan->samplers[i]);

   zscan->pipe->delete_rasterizer_state(zscan->pipe, zscan->rs_state);
   zscan->pipe->delete_blend_state(zscan->pipe, zscan->blend);
}

struct pipe_sampler_view *
vl_zscan_layout(struct pipe_context *pipe, const int layout[64], unsigned blocks_per_line)
{
   const unsigned total_size = blocks_per_line * BLOCK_WIDTH * BLOCK_HEIGHT;

   int patched_layout[64];

   struct pipe_resource res_tmpl, *res;
   struct pipe_sampler_view sv_tmpl, *sv;
   struct pipe_transfer *buf_transfer;
   unsigned x, y, i, pitch;
   float *f;

   struct pipe_box rect =
   {
      0, 0, 0,
      BLOCK_WIDTH * blocks_per_line,
      BLOCK_HEIGHT,
      1
   };

   assert(pipe && layout && blocks_per_line);

   for (i = 0; i < 64; ++i)
      patched_layout[layout[i]] = i;

   memset(&res_tmpl, 0, sizeof(res_tmpl));
   res_tmpl.target = PIPE_TEXTURE_2D;
   res_tmpl.format = PIPE_FORMAT_R32_FLOAT;
   res_tmpl.width0 = BLOCK_WIDTH * blocks_per_line;
   res_tmpl.height0 = BLOCK_HEIGHT;
   res_tmpl.depth0 = 1;
   res_tmpl.array_size = 1;
   res_tmpl.usage = PIPE_USAGE_IMMUTABLE;
   res_tmpl.bind = PIPE_BIND_SAMPLER_VIEW;

   res = pipe->screen->resource_create(pipe->screen, &res_tmpl);
   if (!res)
      goto error_resource;

   buf_transfer = pipe->get_transfer
   (
      pipe, res,
      0, PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD_RANGE,
      &rect
   );
   if (!buf_transfer)
      goto error_transfer;

   pitch = buf_transfer->stride / sizeof(float);

   f = pipe->transfer_map(pipe, buf_transfer);
   if (!f)
      goto error_map;

   for (i = 0; i < blocks_per_line; ++i)
      for (y = 0; y < BLOCK_HEIGHT; ++y)
         for (x = 0; x < BLOCK_WIDTH; ++x) {
            float addr = patched_layout[x + y * BLOCK_WIDTH] +
               i * BLOCK_WIDTH * BLOCK_HEIGHT;

            addr /= total_size;

            f[i * BLOCK_WIDTH + y * pitch + x] = addr;
         }

   pipe->transfer_unmap(pipe, buf_transfer);
   pipe->transfer_destroy(pipe, buf_transfer);

   memset(&sv_tmpl, 0, sizeof(sv_tmpl));
   u_sampler_view_default_template(&sv_tmpl, res, res->format);
   sv = pipe->create_sampler_view(pipe, res, &sv_tmpl);
   pipe_resource_reference(&res, NULL);
   if (!sv)
      goto error_map;

   return sv;

error_map:
   pipe->transfer_destroy(pipe, buf_transfer);

error_transfer:
   pipe_resource_reference(&res, NULL);

error_resource:
   return NULL;
}

bool
vl_zscan_init(struct vl_zscan *zscan, struct pipe_context *pipe,
              unsigned buffer_width, unsigned buffer_height,
              unsigned blocks_per_line, unsigned blocks_total,
              unsigned num_channels)
{
   assert(zscan && pipe);

   zscan->pipe = pipe;
   zscan->buffer_width = buffer_width;
   zscan->buffer_height = buffer_height;
   zscan->num_channels = num_channels;
   zscan->blocks_per_line = blocks_per_line;
   zscan->blocks_total = blocks_total;

   if(!init_shaders(zscan))
      return false;

   if(!init_state(zscan)) {
      cleanup_shaders(zscan);
      return false;
   }

   return true;
}

void
vl_zscan_cleanup(struct vl_zscan *zscan)
{
   assert(zscan);

   cleanup_shaders(zscan);
   cleanup_state(zscan);
}

bool
vl_zscan_init_buffer(struct vl_zscan *zscan, struct vl_zscan_buffer *buffer,
                     struct pipe_sampler_view *src, struct pipe_surface *dst)
{
   struct pipe_resource res_tmpl, *res;
   struct pipe_sampler_view sv_tmpl;

   assert(zscan && buffer);

   memset(buffer, 0, sizeof(struct vl_zscan_buffer));

   pipe_sampler_view_reference(&buffer->src, src);

   buffer->viewport.scale[0] = dst->width;
   buffer->viewport.scale[1] = dst->height;
   buffer->viewport.scale[2] = 1;
   buffer->viewport.scale[3] = 1;
   buffer->viewport.translate[0] = 0;
   buffer->viewport.translate[1] = 0;
   buffer->viewport.translate[2] = 0;
   buffer->viewport.translate[3] = 0;

   buffer->fb_state.width = dst->width;
   buffer->fb_state.height = dst->height;
   buffer->fb_state.nr_cbufs = 1;
   pipe_surface_reference(&buffer->fb_state.cbufs[0], dst);

   memset(&res_tmpl, 0, sizeof(res_tmpl));
   res_tmpl.target = PIPE_TEXTURE_3D;
   res_tmpl.format = PIPE_FORMAT_R8_UNORM;
   res_tmpl.width0 = BLOCK_WIDTH * zscan->blocks_per_line;
   res_tmpl.height0 = BLOCK_HEIGHT;
   res_tmpl.depth0 = 2;
   res_tmpl.array_size = 1;
   res_tmpl.usage = PIPE_USAGE_IMMUTABLE;
   res_tmpl.bind = PIPE_BIND_SAMPLER_VIEW;

   res = zscan->pipe->screen->resource_create(zscan->pipe->screen, &res_tmpl);
   if (!res)
      return false;

   memset(&sv_tmpl, 0, sizeof(sv_tmpl));
   u_sampler_view_default_template(&sv_tmpl, res, res->format);
   sv_tmpl.swizzle_r = sv_tmpl.swizzle_g = sv_tmpl.swizzle_b = sv_tmpl.swizzle_a = TGSI_SWIZZLE_X;
   buffer->quant = zscan->pipe->create_sampler_view(zscan->pipe, res, &sv_tmpl);
   pipe_resource_reference(&res, NULL);
   if (!buffer->quant)
      return false;

   return true;
}

void
vl_zscan_cleanup_buffer(struct vl_zscan_buffer *buffer)
{
   assert(buffer);

   pipe_sampler_view_reference(&buffer->src, NULL);
   pipe_sampler_view_reference(&buffer->layout, NULL);
   pipe_sampler_view_reference(&buffer->quant, NULL);
   pipe_surface_reference(&buffer->fb_state.cbufs[0], NULL);
}

void
vl_zscan_set_layout(struct vl_zscan_buffer *buffer, struct pipe_sampler_view *layout)
{
   assert(buffer);
   assert(layout);

   pipe_sampler_view_reference(&buffer->layout, layout);
}

void
vl_zscan_upload_quant(struct vl_zscan *zscan, struct vl_zscan_buffer *buffer,
                      const uint8_t matrix[64], bool intra)
{
   struct pipe_context *pipe;
   struct pipe_transfer *buf_transfer;
   unsigned x, y, i, pitch;
   uint8_t *data;

   struct pipe_box rect =
   {
      0, 0, intra ? 1 : 0,
      BLOCK_WIDTH,
      BLOCK_HEIGHT,
      1
   };

   assert(buffer);
   assert(matrix);

   pipe = zscan->pipe;

   rect.width *= zscan->blocks_per_line;

   buf_transfer = pipe->get_transfer
   (
      pipe, buffer->quant->texture,
      0, PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD_RANGE,
      &rect
   );
   if (!buf_transfer)
      goto error_transfer;

   pitch = buf_transfer->stride;

   data = pipe->transfer_map(pipe, buf_transfer);
   if (!data)
      goto error_map;

   for (i = 0; i < zscan->blocks_per_line; ++i)
      for (y = 0; y < BLOCK_HEIGHT; ++y)
         for (x = 0; x < BLOCK_WIDTH; ++x)
            data[i * BLOCK_WIDTH + y * pitch + x] = matrix[x + y * BLOCK_WIDTH];

   pipe->transfer_unmap(pipe, buf_transfer);

error_map:
   pipe->transfer_destroy(pipe, buf_transfer);

error_transfer:
   return;
}

void
vl_zscan_render(struct vl_zscan *zscan, struct vl_zscan_buffer *buffer, unsigned num_instances)
{
   assert(buffer);

   zscan->pipe->bind_rasterizer_state(zscan->pipe, zscan->rs_state);
   zscan->pipe->bind_blend_state(zscan->pipe, zscan->blend);
   zscan->pipe->bind_fragment_sampler_states(zscan->pipe, 3, zscan->samplers);
   zscan->pipe->set_framebuffer_state(zscan->pipe, &buffer->fb_state);
   zscan->pipe->set_viewport_state(zscan->pipe, &buffer->viewport);
   zscan->pipe->set_fragment_sampler_views(zscan->pipe, 3, &buffer->src);
   zscan->pipe->bind_vs_state(zscan->pipe, zscan->vs);
   zscan->pipe->bind_fs_state(zscan->pipe, zscan->fs);
   util_draw_arrays_instanced(zscan->pipe, PIPE_PRIM_QUADS, 0, 4, 0, num_instances);
}
