/**************************************************************************
 *
 * Copyright 2009 Younes Manton.
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

#include "pipe/p_context.h"

#include "util/u_sampler.h"
#include "util/u_draw.h"

#include "tgsi/tgsi_ureg.h"

#include "vl_defines.h"
#include "vl_vertex_buffers.h"
#include "vl_mc.h"
#include "vl_idct.h"

enum VS_OUTPUT
{
   VS_O_VPOS,
   VS_O_VTOP,
   VS_O_VBOTTOM,

   VS_O_FLAGS = VS_O_VTOP,
   VS_O_VTEX = VS_O_VBOTTOM
};

static struct ureg_dst
calc_position(struct vl_mc *r, struct ureg_program *shader, struct ureg_src block_scale)
{
   struct ureg_src vrect, vpos;
   struct ureg_dst t_vpos;
   struct ureg_dst o_vpos;

   vrect = ureg_DECL_vs_input(shader, VS_I_RECT);
   vpos = ureg_DECL_vs_input(shader, VS_I_VPOS);

   t_vpos = ureg_DECL_temporary(shader);

   o_vpos = ureg_DECL_output(shader, TGSI_SEMANTIC_POSITION, VS_O_VPOS);

   /*
    * block_scale = (MACROBLOCK_WIDTH, MACROBLOCK_HEIGHT) / (dst.width, dst.height)
    *
    * t_vpos = (vpos + vrect) * block_scale
    * o_vpos.xy = t_vpos
    * o_vpos.zw = vpos
    */
   ureg_ADD(shader, ureg_writemask(t_vpos, TGSI_WRITEMASK_XY), vpos, vrect);
   ureg_MUL(shader, ureg_writemask(t_vpos, TGSI_WRITEMASK_XY), ureg_src(t_vpos), block_scale);
   ureg_MOV(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_XY), ureg_src(t_vpos));
   ureg_MOV(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_ZW), ureg_imm1f(shader, 1.0f));

   return t_vpos;
}

static struct ureg_dst
calc_line(struct ureg_program *shader)
{
   struct ureg_dst tmp;
   struct ureg_src pos;

   tmp = ureg_DECL_temporary(shader);

   pos = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_POSITION, VS_O_VPOS, TGSI_INTERPOLATE_LINEAR);

   /*
    * tmp.y = fraction(pos.y / 2) >= 0.5 ? 1 : 0
    */
   ureg_MUL(shader, ureg_writemask(tmp, TGSI_WRITEMASK_Y), pos, ureg_imm1f(shader, 0.5f));
   ureg_FRC(shader, ureg_writemask(tmp, TGSI_WRITEMASK_Y), ureg_src(tmp));
   ureg_SGE(shader, ureg_writemask(tmp, TGSI_WRITEMASK_Y), ureg_src(tmp), ureg_imm1f(shader, 0.5f));

   return tmp;
}

static void *
create_ref_vert_shader(struct vl_mc *r)
{
   struct ureg_program *shader;
   struct ureg_src mv_scale;
   struct ureg_src vmv[2];
   struct ureg_dst t_vpos;
   struct ureg_dst o_vmv[2];
   unsigned i;

   shader = ureg_create(TGSI_PROCESSOR_VERTEX);
   if (!shader)
      return NULL;

   vmv[0] = ureg_DECL_vs_input(shader, VS_I_MV_TOP);
   vmv[1] = ureg_DECL_vs_input(shader, VS_I_MV_BOTTOM);

   t_vpos = calc_position(r, shader, ureg_imm2f(shader,
      (float)MACROBLOCK_WIDTH / r->buffer_width,
      (float)MACROBLOCK_HEIGHT / r->buffer_height)
   );

   o_vmv[0] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_VTOP);
   o_vmv[1] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_VBOTTOM);

   /*
    * mv_scale.xy = 0.5 / (dst.width, dst.height);
    * mv_scale.z = 1.0f / 4.0f
    * mv_scale.w = 1.0f / 255.0f
    *
    * // Apply motion vectors
    * o_vmv[0..1].xy = vmv[0..1] * mv_scale + t_vpos
    * o_vmv[0..1].zw = vmv[0..1] * mv_scale
    *
    */

   mv_scale = ureg_imm4f(shader,
      0.5f / r->buffer_width,
      0.5f / r->buffer_height,
      1.0f / 4.0f,
      1.0f / PIPE_VIDEO_MV_WEIGHT_MAX);

   for (i = 0; i < 2; ++i) {
      ureg_MAD(shader, ureg_writemask(o_vmv[i], TGSI_WRITEMASK_XY), mv_scale, vmv[i], ureg_src(t_vpos));
      ureg_MUL(shader, ureg_writemask(o_vmv[i], TGSI_WRITEMASK_ZW), mv_scale, vmv[i]);
   }

   ureg_release_temporary(shader, t_vpos);

   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, r->pipe);
}

static void *
create_ref_frag_shader(struct vl_mc *r)
{
   const float y_scale =
      r->buffer_height / 2 *
      r->macroblock_size / MACROBLOCK_HEIGHT;

   struct ureg_program *shader;
   struct ureg_src tc[2], sampler;
   struct ureg_dst ref, field;
   struct ureg_dst fragment;
   unsigned label;

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return NULL;

   tc[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_VTOP, TGSI_INTERPOLATE_LINEAR);
   tc[1] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_VBOTTOM, TGSI_INTERPOLATE_LINEAR);

   sampler = ureg_DECL_sampler(shader, 0);
   ref = ureg_DECL_temporary(shader);

   fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);

   field = calc_line(shader);

   /*
    * ref = field.z ? tc[1] : tc[0]
    *
    * // Adjust tc acording to top/bottom field selection
    * if (|ref.z|) {
    *    ref.y *= y_scale
    *    ref.y = floor(ref.y)
    *    ref.y += ref.z
    *    ref.y /= y_scale
    * }
    * fragment.xyz = tex(ref, sampler[0])
    */
   ureg_CMP(shader, ureg_writemask(ref, TGSI_WRITEMASK_XYZ),
            ureg_negate(ureg_scalar(ureg_src(field), TGSI_SWIZZLE_Y)),
            tc[1], tc[0]);
   ureg_CMP(shader, ureg_writemask(fragment, TGSI_WRITEMASK_W),
            ureg_negate(ureg_scalar(ureg_src(field), TGSI_SWIZZLE_Y)),
            tc[1], tc[0]);

   ureg_IF(shader, ureg_scalar(ureg_src(ref), TGSI_SWIZZLE_Z), &label);

      ureg_MUL(shader, ureg_writemask(ref, TGSI_WRITEMASK_Y),
               ureg_src(ref), ureg_imm1f(shader, y_scale));
      ureg_FLR(shader, ureg_writemask(ref, TGSI_WRITEMASK_Y), ureg_src(ref));
      ureg_ADD(shader, ureg_writemask(ref, TGSI_WRITEMASK_Y),
               ureg_src(ref), ureg_scalar(ureg_src(ref), TGSI_SWIZZLE_Z));
      ureg_MUL(shader, ureg_writemask(ref, TGSI_WRITEMASK_Y),
               ureg_src(ref), ureg_imm1f(shader, 1.0f / y_scale));

   ureg_fixup_label(shader, label, ureg_get_instruction_number(shader));
   ureg_ENDIF(shader);

   ureg_TEX(shader, ureg_writemask(fragment, TGSI_WRITEMASK_XYZ), TGSI_TEXTURE_2D, ureg_src(ref), sampler);

   ureg_release_temporary(shader, ref);

   ureg_release_temporary(shader, field);
   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, r->pipe);
}

static void *
create_ycbcr_vert_shader(struct vl_mc *r, vl_mc_ycbcr_vert_shader vs_callback, void *callback_priv)
{
   struct ureg_program *shader;

   struct ureg_src vrect, vpos;
   struct ureg_dst t_vpos, t_vtex;
   struct ureg_dst o_vpos, o_flags;

   struct vertex2f scale = {
      (float)BLOCK_WIDTH / r->buffer_width * MACROBLOCK_WIDTH / r->macroblock_size,
      (float)BLOCK_HEIGHT / r->buffer_height * MACROBLOCK_HEIGHT / r->macroblock_size
   };

   unsigned label;

   shader = ureg_create(TGSI_PROCESSOR_VERTEX);
   if (!shader)
      return NULL;

   vrect = ureg_DECL_vs_input(shader, VS_I_RECT);
   vpos = ureg_DECL_vs_input(shader, VS_I_VPOS);

   t_vpos = calc_position(r, shader, ureg_imm2f(shader, scale.x, scale.y));
   t_vtex = ureg_DECL_temporary(shader);

   o_vpos = ureg_DECL_output(shader, TGSI_SEMANTIC_POSITION, VS_O_VPOS);
   o_flags = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_FLAGS);

   /*
    * o_vtex.xy = t_vpos
    * o_flags.z = intra * 0.5
    *
    * if(interlaced) {
    *    t_vtex.xy = vrect.y ? { 0, scale.y } : { -scale.y : 0 }
    *    t_vtex.z = vpos.y % 2
    *    t_vtex.y = t_vtex.z ? t_vtex.x : t_vtex.y
    *    o_vpos.y = t_vtex.y + t_vpos.y
    *
    *    o_flags.w = t_vtex.z ? 0 : 1
    * }
    *
    */

   vs_callback(callback_priv, r, shader, VS_O_VTEX, t_vpos);

   ureg_MUL(shader, ureg_writemask(o_flags, TGSI_WRITEMASK_Z),
            ureg_scalar(vpos, TGSI_SWIZZLE_Z), ureg_imm1f(shader, 0.5f));
   ureg_MOV(shader, ureg_writemask(o_flags, TGSI_WRITEMASK_W), ureg_imm1f(shader, -1.0f));

   if (r->macroblock_size == MACROBLOCK_HEIGHT) { //TODO
      ureg_IF(shader, ureg_scalar(vpos, TGSI_SWIZZLE_W), &label);

         ureg_CMP(shader, ureg_writemask(t_vtex, TGSI_WRITEMASK_XY),
                  ureg_negate(ureg_scalar(vrect, TGSI_SWIZZLE_Y)),
                  ureg_imm2f(shader, 0.0f, scale.y),
                  ureg_imm2f(shader, -scale.y, 0.0f));
         ureg_MUL(shader, ureg_writemask(t_vtex, TGSI_WRITEMASK_Z),
                  ureg_scalar(vpos, TGSI_SWIZZLE_Y), ureg_imm1f(shader, 0.5f));

         ureg_FRC(shader, ureg_writemask(t_vtex, TGSI_WRITEMASK_Z), ureg_src(t_vtex));

         ureg_CMP(shader, ureg_writemask(t_vtex, TGSI_WRITEMASK_Y),
                  ureg_negate(ureg_scalar(ureg_src(t_vtex), TGSI_SWIZZLE_Z)),
                  ureg_scalar(ureg_src(t_vtex), TGSI_SWIZZLE_X),
                  ureg_scalar(ureg_src(t_vtex), TGSI_SWIZZLE_Y));
         ureg_ADD(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_Y),
                  ureg_src(t_vpos), ureg_src(t_vtex));

         ureg_CMP(shader, ureg_writemask(o_flags, TGSI_WRITEMASK_W),
                  ureg_negate(ureg_scalar(ureg_src(t_vtex), TGSI_SWIZZLE_Z)),
                  ureg_imm1f(shader, 0.0f), ureg_imm1f(shader, 1.0f));

      ureg_fixup_label(shader, label, ureg_get_instruction_number(shader));
      ureg_ENDIF(shader);
   }

   ureg_release_temporary(shader, t_vtex);
   ureg_release_temporary(shader, t_vpos);

   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, r->pipe);
}

static void *
create_ycbcr_frag_shader(struct vl_mc *r, float scale, bool invert,
                         vl_mc_ycbcr_frag_shader fs_callback, void *callback_priv)
{
   struct ureg_program *shader;
   struct ureg_src flags;
   struct ureg_dst tmp;
   struct ureg_dst fragment;
   unsigned label;

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return NULL;

   flags = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_FLAGS, TGSI_INTERPOLATE_LINEAR);

   fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);

   tmp = calc_line(shader);

   /*
    * if (field == tc.w)
    *    kill();
    * else {
    *    fragment.xyz  = tex(tc, sampler) * scale + tc.z
    *    fragment.w = 1.0f
    * }
    */

   ureg_SEQ(shader, ureg_writemask(tmp, TGSI_WRITEMASK_Y),
            ureg_scalar(flags, TGSI_SWIZZLE_W), ureg_src(tmp));

   ureg_IF(shader, ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_Y), &label);

      ureg_KILP(shader);

   ureg_fixup_label(shader, label, ureg_get_instruction_number(shader));
   ureg_ELSE(shader, &label);

      fs_callback(callback_priv, r, shader, VS_O_VTEX, tmp);

      if (scale != 1.0f)
         ureg_MAD(shader, ureg_writemask(tmp, TGSI_WRITEMASK_XYZ),
                  ureg_src(tmp), ureg_imm1f(shader, scale),
                  ureg_scalar(flags, TGSI_SWIZZLE_Z));
      else
         ureg_ADD(shader, ureg_writemask(tmp, TGSI_WRITEMASK_XYZ),
                  ureg_src(tmp), ureg_scalar(flags, TGSI_SWIZZLE_Z));
                  
      ureg_MUL(shader, ureg_writemask(fragment, TGSI_WRITEMASK_XYZ), ureg_src(tmp), ureg_imm1f(shader, invert ? -1.0f : 1.0f));
      ureg_MOV(shader, ureg_writemask(fragment, TGSI_WRITEMASK_W), ureg_imm1f(shader, 1.0f));

   ureg_fixup_label(shader, label, ureg_get_instruction_number(shader));
   ureg_ENDIF(shader);

   ureg_release_temporary(shader, tmp);

   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, r->pipe);
}

static bool
init_pipe_state(struct vl_mc *r)
{
   struct pipe_sampler_state sampler;
   struct pipe_blend_state blend;
   struct pipe_rasterizer_state rs_state;
   unsigned i;

   assert(r);

   memset(&sampler, 0, sizeof(sampler));
   sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_BORDER;
   sampler.min_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   sampler.mag_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.compare_mode = PIPE_TEX_COMPARE_NONE;
   sampler.compare_func = PIPE_FUNC_ALWAYS;
   sampler.normalized_coords = 1;
   r->sampler_ref = r->pipe->create_sampler_state(r->pipe, &sampler);
   if (!r->sampler_ref)
      goto error_sampler_ref;

   for (i = 0; i < VL_MC_NUM_BLENDERS; ++i) {
      memset(&blend, 0, sizeof blend);
      blend.independent_blend_enable = 0;
      blend.rt[0].blend_enable = 1;
      blend.rt[0].rgb_func = PIPE_BLEND_ADD;
      blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_SRC_ALPHA;
      blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
      blend.rt[0].alpha_func = PIPE_BLEND_ADD;
      blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_SRC_ALPHA;
      blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
      blend.logicop_enable = 0;
      blend.logicop_func = PIPE_LOGICOP_CLEAR;
      blend.rt[0].colormask = i;
      blend.dither = 0;
      r->blend_clear[i] = r->pipe->create_blend_state(r->pipe, &blend);
      if (!r->blend_clear[i])
         goto error_blend;

      blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ONE;
      blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ONE;
      r->blend_add[i] = r->pipe->create_blend_state(r->pipe, &blend);
      if (!r->blend_add[i])
         goto error_blend;

      blend.rt[0].rgb_func = PIPE_BLEND_REVERSE_SUBTRACT;
      blend.rt[0].alpha_dst_factor = PIPE_BLEND_REVERSE_SUBTRACT;
      r->blend_sub[i] = r->pipe->create_blend_state(r->pipe, &blend);
      if (!r->blend_sub[i])
         goto error_blend;
   }

   memset(&rs_state, 0, sizeof(rs_state));
   /*rs_state.sprite_coord_enable */
   rs_state.sprite_coord_mode = PIPE_SPRITE_COORD_UPPER_LEFT;
   rs_state.point_quad_rasterization = true;
   rs_state.point_size = BLOCK_WIDTH;
   rs_state.gl_rasterization_rules = true;
   rs_state.depth_clip = 1;
   r->rs_state = r->pipe->create_rasterizer_state(r->pipe, &rs_state);
   if (!r->rs_state)
      goto error_rs_state;

   return true;

error_rs_state:
error_blend:
   for (i = 0; i < VL_MC_NUM_BLENDERS; ++i) {
      if (r->blend_sub[i])
         r->pipe->delete_blend_state(r->pipe, r->blend_sub[i]);

      if (r->blend_add[i])
         r->pipe->delete_blend_state(r->pipe, r->blend_add[i]);

      if (r->blend_clear[i])
         r->pipe->delete_blend_state(r->pipe, r->blend_clear[i]);
   }

   r->pipe->delete_sampler_state(r->pipe, r->sampler_ref);

error_sampler_ref:
   return false;
}

static void
cleanup_pipe_state(struct vl_mc *r)
{
   unsigned i;

   assert(r);

   r->pipe->delete_sampler_state(r->pipe, r->sampler_ref);
   for (i = 0; i < VL_MC_NUM_BLENDERS; ++i) {
      r->pipe->delete_blend_state(r->pipe, r->blend_clear[i]);
      r->pipe->delete_blend_state(r->pipe, r->blend_add[i]);
      r->pipe->delete_blend_state(r->pipe, r->blend_sub[i]);
   }
   r->pipe->delete_rasterizer_state(r->pipe, r->rs_state);
}

bool
vl_mc_init(struct vl_mc *renderer, struct pipe_context *pipe,
           unsigned buffer_width, unsigned buffer_height,
           unsigned macroblock_size, float scale,
           vl_mc_ycbcr_vert_shader vs_callback,
           vl_mc_ycbcr_frag_shader fs_callback,
           void *callback_priv)
{
   assert(renderer);
   assert(pipe);

   memset(renderer, 0, sizeof(struct vl_mc));

   renderer->pipe = pipe;
   renderer->buffer_width = buffer_width;
   renderer->buffer_height = buffer_height;
   renderer->macroblock_size = macroblock_size;

   if (!init_pipe_state(renderer))
      goto error_pipe_state;

   renderer->vs_ref = create_ref_vert_shader(renderer);
   if (!renderer->vs_ref)
      goto error_vs_ref;

   renderer->vs_ycbcr = create_ycbcr_vert_shader(renderer, vs_callback, callback_priv);
   if (!renderer->vs_ycbcr)
      goto error_vs_ycbcr;

   renderer->fs_ref = create_ref_frag_shader(renderer);
   if (!renderer->fs_ref)
      goto error_fs_ref;

   renderer->fs_ycbcr = create_ycbcr_frag_shader(renderer, scale, false, fs_callback, callback_priv);
   if (!renderer->fs_ycbcr)
      goto error_fs_ycbcr;

   renderer->fs_ycbcr_sub = create_ycbcr_frag_shader(renderer, scale, true, fs_callback, callback_priv);
   if (!renderer->fs_ycbcr_sub)
      goto error_fs_ycbcr_sub;

   return true;
   
error_fs_ycbcr_sub:
   renderer->pipe->delete_fs_state(renderer->pipe, renderer->fs_ycbcr);

error_fs_ycbcr:
   renderer->pipe->delete_fs_state(renderer->pipe, renderer->fs_ref);

error_fs_ref:
   renderer->pipe->delete_vs_state(renderer->pipe, renderer->vs_ycbcr);

error_vs_ycbcr:
   renderer->pipe->delete_vs_state(renderer->pipe, renderer->vs_ref);

error_vs_ref:
   cleanup_pipe_state(renderer);

error_pipe_state:
   return false;
}

void
vl_mc_cleanup(struct vl_mc *renderer)
{
   assert(renderer);

   cleanup_pipe_state(renderer);

   renderer->pipe->delete_vs_state(renderer->pipe, renderer->vs_ref);
   renderer->pipe->delete_vs_state(renderer->pipe, renderer->vs_ycbcr);
   renderer->pipe->delete_fs_state(renderer->pipe, renderer->fs_ref);
   renderer->pipe->delete_fs_state(renderer->pipe, renderer->fs_ycbcr);
   renderer->pipe->delete_fs_state(renderer->pipe, renderer->fs_ycbcr_sub);
}

bool
vl_mc_init_buffer(struct vl_mc *renderer, struct vl_mc_buffer *buffer)
{
   assert(renderer && buffer);

   buffer->viewport.scale[2] = 1;
   buffer->viewport.scale[3] = 1;
   buffer->viewport.translate[0] = 0;
   buffer->viewport.translate[1] = 0;
   buffer->viewport.translate[2] = 0;
   buffer->viewport.translate[3] = 0;

   buffer->fb_state.nr_cbufs = 1;
   buffer->fb_state.zsbuf = NULL;

   return true;
}

void
vl_mc_cleanup_buffer(struct vl_mc_buffer *buffer)
{
   assert(buffer);
}

void
vl_mc_set_surface(struct vl_mc_buffer *buffer, struct pipe_surface *surface)
{
   assert(buffer && surface);

   buffer->surface_cleared = false;

   buffer->viewport.scale[0] = surface->width;
   buffer->viewport.scale[1] = surface->height;

   buffer->fb_state.width = surface->width;
   buffer->fb_state.height = surface->height;
   buffer->fb_state.cbufs[0] = surface;
}

static void
prepare_pipe_4_rendering(struct vl_mc *renderer, struct vl_mc_buffer *buffer, unsigned mask)
{
   assert(buffer);

   renderer->pipe->bind_rasterizer_state(renderer->pipe, renderer->rs_state);

   if (buffer->surface_cleared)
      renderer->pipe->bind_blend_state(renderer->pipe, renderer->blend_add[mask]);
   else
      renderer->pipe->bind_blend_state(renderer->pipe, renderer->blend_clear[mask]);

   renderer->pipe->set_framebuffer_state(renderer->pipe, &buffer->fb_state);
   renderer->pipe->set_viewport_state(renderer->pipe, &buffer->viewport);
}

void
vl_mc_render_ref(struct vl_mc *renderer, struct vl_mc_buffer *buffer, struct pipe_sampler_view *ref)
{
   assert(buffer && ref);

   prepare_pipe_4_rendering(renderer, buffer, PIPE_MASK_R | PIPE_MASK_G | PIPE_MASK_B);

   renderer->pipe->bind_vs_state(renderer->pipe, renderer->vs_ref);
   renderer->pipe->bind_fs_state(renderer->pipe, renderer->fs_ref);

   renderer->pipe->set_fragment_sampler_views(renderer->pipe, 1, &ref);
   renderer->pipe->bind_fragment_sampler_states(renderer->pipe, 1, &renderer->sampler_ref);

   util_draw_arrays_instanced(renderer->pipe, PIPE_PRIM_QUADS, 0, 4, 0,
                              renderer->buffer_width / MACROBLOCK_WIDTH *
                              renderer->buffer_height / MACROBLOCK_HEIGHT);

   buffer->surface_cleared = true;
}

void
vl_mc_render_ycbcr(struct vl_mc *renderer, struct vl_mc_buffer *buffer, unsigned component, unsigned num_instances)
{
   unsigned mask = 1 << component;

   assert(buffer);

   if (num_instances == 0)
      return;

   prepare_pipe_4_rendering(renderer, buffer, mask);

   renderer->pipe->bind_vs_state(renderer->pipe, renderer->vs_ycbcr);
   renderer->pipe->bind_fs_state(renderer->pipe, renderer->fs_ycbcr);

   util_draw_arrays_instanced(renderer->pipe, PIPE_PRIM_QUADS, 0, 4, 0, num_instances);
   
   if (buffer->surface_cleared) {
      renderer->pipe->bind_blend_state(renderer->pipe, renderer->blend_sub[mask]);
      renderer->pipe->bind_fs_state(renderer->pipe, renderer->fs_ycbcr_sub);
      util_draw_arrays_instanced(renderer->pipe, PIPE_PRIM_QUADS, 0, 4, 0, num_instances);
   }
}
