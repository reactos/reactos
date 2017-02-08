/**************************************************************************
 *
 * Copyright 2010 Christian König
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
#include "pipe/p_screen.h"

#include "util/u_draw.h"
#include "util/u_sampler.h"
#include "util/u_memory.h"

#include "tgsi/tgsi_ureg.h"

#include "vl_defines.h"
#include "vl_types.h"
#include "vl_vertex_buffers.h"
#include "vl_idct.h"

enum VS_OUTPUT
{
   VS_O_VPOS,
   VS_O_L_ADDR0,
   VS_O_L_ADDR1,
   VS_O_R_ADDR0,
   VS_O_R_ADDR1
};

/**
 * The DCT matrix stored as hex representation of floats. Equal to the following equation:
 * for (i = 0; i < 8; ++i)
 *    for (j = 0; j < 8; ++j)
 *       if (i == 0) const_matrix[i][j] = 1.0f / sqrtf(8.0f);
 *       else const_matrix[i][j] = sqrtf(2.0f / 8.0f) * cosf((2 * j + 1) * i * M_PI / (2.0f * 8.0f));
 */
static const uint32_t const_matrix[8][8] = {
   { 0x3eb504f3, 0x3eb504f3, 0x3eb504f3, 0x3eb504f3, 0x3eb504f3, 0x3eb504f3, 0x3eb504f3, 0x3eb504f3 },
   { 0x3efb14be, 0x3ed4db31, 0x3e8e39da, 0x3dc7c5c4, 0xbdc7c5c2, 0xbe8e39d9, 0xbed4db32, 0xbefb14bf },
   { 0x3eec835f, 0x3e43ef15, 0xbe43ef14, 0xbeec835e, 0xbeec835f, 0xbe43ef1a, 0x3e43ef1b, 0x3eec835f },
   { 0x3ed4db31, 0xbdc7c5c2, 0xbefb14bf, 0xbe8e39dd, 0x3e8e39d7, 0x3efb14bf, 0x3dc7c5d0, 0xbed4db34 },
   { 0x3eb504f3, 0xbeb504f3, 0xbeb504f4, 0x3eb504f1, 0x3eb504f3, 0xbeb504f0, 0xbeb504ef, 0x3eb504f4 },
   { 0x3e8e39da, 0xbefb14bf, 0x3dc7c5c8, 0x3ed4db32, 0xbed4db34, 0xbdc7c5bb, 0x3efb14bf, 0xbe8e39d7 },
   { 0x3e43ef15, 0xbeec835f, 0x3eec835f, 0xbe43ef07, 0xbe43ef23, 0x3eec8361, 0xbeec835c, 0x3e43ef25 },
   { 0x3dc7c5c4, 0xbe8e39dd, 0x3ed4db32, 0xbefb14c0, 0x3efb14be, 0xbed4db31, 0x3e8e39ce, 0xbdc7c596 },
};

static void
calc_addr(struct ureg_program *shader, struct ureg_dst addr[2],
          struct ureg_src tc, struct ureg_src start, bool right_side,
          bool transposed, float size)
{
   unsigned wm_start = (right_side == transposed) ? TGSI_WRITEMASK_X : TGSI_WRITEMASK_Y;
   unsigned sw_start = right_side ? TGSI_SWIZZLE_Y : TGSI_SWIZZLE_X;

   unsigned wm_tc = (right_side == transposed) ? TGSI_WRITEMASK_Y : TGSI_WRITEMASK_X;
   unsigned sw_tc = right_side ? TGSI_SWIZZLE_X : TGSI_SWIZZLE_Y;

   /*
    * addr[0..1].(start) = right_side ? start.x : tc.x
    * addr[0..1].(tc) = right_side ? tc.y : start.y
    * addr[0..1].z = tc.z
    * addr[1].(start) += 1.0f / scale
    */
   ureg_MOV(shader, ureg_writemask(addr[0], wm_start), ureg_scalar(start, sw_start));
   ureg_MOV(shader, ureg_writemask(addr[0], wm_tc), ureg_scalar(tc, sw_tc));

   ureg_ADD(shader, ureg_writemask(addr[1], wm_start), ureg_scalar(start, sw_start), ureg_imm1f(shader, 1.0f / size));
   ureg_MOV(shader, ureg_writemask(addr[1], wm_tc), ureg_scalar(tc, sw_tc));
}

static void
increment_addr(struct ureg_program *shader, struct ureg_dst daddr[2],
               struct ureg_src saddr[2], bool right_side, bool transposed,
               int pos, float size)
{
   unsigned wm_start = (right_side == transposed) ? TGSI_WRITEMASK_X : TGSI_WRITEMASK_Y;
   unsigned wm_tc = (right_side == transposed) ? TGSI_WRITEMASK_Y : TGSI_WRITEMASK_X;

   /*
    * daddr[0..1].(start) = saddr[0..1].(start)
    * daddr[0..1].(tc) = saddr[0..1].(tc)
    */

   ureg_MOV(shader, ureg_writemask(daddr[0], wm_start), saddr[0]);
   ureg_ADD(shader, ureg_writemask(daddr[0], wm_tc), saddr[0], ureg_imm1f(shader, pos / size));
   ureg_MOV(shader, ureg_writemask(daddr[1], wm_start), saddr[1]);
   ureg_ADD(shader, ureg_writemask(daddr[1], wm_tc), saddr[1], ureg_imm1f(shader, pos / size));
}

static void
fetch_four(struct ureg_program *shader, struct ureg_dst m[2], struct ureg_src addr[2],
           struct ureg_src sampler, bool resource3d)
{
   ureg_TEX(shader, m[0], resource3d ? TGSI_TEXTURE_3D : TGSI_TEXTURE_2D, addr[0], sampler);
   ureg_TEX(shader, m[1], resource3d ? TGSI_TEXTURE_3D : TGSI_TEXTURE_2D, addr[1], sampler);
}

static void
matrix_mul(struct ureg_program *shader, struct ureg_dst dst, struct ureg_dst l[2], struct ureg_dst r[2])
{
   struct ureg_dst tmp;

   tmp = ureg_DECL_temporary(shader);

   /*
    * tmp.xy = dot4(m[0][0..1], m[1][0..1])
    * dst = tmp.x + tmp.y
    */
   ureg_DP4(shader, ureg_writemask(tmp, TGSI_WRITEMASK_X), ureg_src(l[0]), ureg_src(r[0]));
   ureg_DP4(shader, ureg_writemask(tmp, TGSI_WRITEMASK_Y), ureg_src(l[1]), ureg_src(r[1]));
   ureg_ADD(shader, dst,
      ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_X),
      ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_Y));

   ureg_release_temporary(shader, tmp);
}

static void *
create_mismatch_vert_shader(struct vl_idct *idct)
{
   struct ureg_program *shader;
   struct ureg_src vpos;
   struct ureg_src scale;
   struct ureg_dst t_tex;
   struct ureg_dst o_vpos, o_addr[2];

   shader = ureg_create(TGSI_PROCESSOR_VERTEX);
   if (!shader)
      return NULL;

   vpos = ureg_DECL_vs_input(shader, VS_I_VPOS);

   t_tex = ureg_DECL_temporary(shader);

   o_vpos = ureg_DECL_output(shader, TGSI_SEMANTIC_POSITION, VS_O_VPOS);

   o_addr[0] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_L_ADDR0);
   o_addr[1] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_L_ADDR1);

   /*
    * scale = (BLOCK_WIDTH, BLOCK_HEIGHT) / (dst.width, dst.height)
    *
    * t_vpos = vpos + 7 / BLOCK_WIDTH
    * o_vpos.xy = t_vpos * scale
    *
    * o_addr = calc_addr(...)
    *
    */

   scale = ureg_imm2f(shader,
      (float)BLOCK_WIDTH / idct->buffer_width,
      (float)BLOCK_HEIGHT / idct->buffer_height);

   ureg_MAD(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_XY), vpos, scale, scale);
   ureg_MOV(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_ZW), ureg_imm1f(shader, 1.0f));

   ureg_MUL(shader, ureg_writemask(t_tex, TGSI_WRITEMASK_XY), vpos, scale);
   calc_addr(shader, o_addr, ureg_src(t_tex), ureg_src(t_tex), false, false, idct->buffer_width / 4);

   ureg_release_temporary(shader, t_tex);

   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, idct->pipe);
}

static void *
create_mismatch_frag_shader(struct vl_idct *idct)
{
   struct ureg_program *shader;

   struct ureg_src addr[2];

   struct ureg_dst m[8][2];
   struct ureg_dst fragment;

   unsigned i;

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return NULL;

   addr[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_L_ADDR0, TGSI_INTERPOLATE_LINEAR);
   addr[1] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_L_ADDR1, TGSI_INTERPOLATE_LINEAR);

   fragment = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, 0);

   for (i = 0; i < 8; ++i) {
      m[i][0] = ureg_DECL_temporary(shader);
      m[i][1] = ureg_DECL_temporary(shader);
   }

   for (i = 0; i < 8; ++i) {
      increment_addr(shader, m[i], addr, false, false, i, idct->buffer_height);
   }

   for (i = 0; i < 8; ++i) {
      struct ureg_src s_addr[2];
      s_addr[0] = ureg_src(m[i][0]);
      s_addr[1] = ureg_src(m[i][1]);
      fetch_four(shader, m[i], s_addr, ureg_DECL_sampler(shader, 0), false);
   }

   for (i = 1; i < 8; ++i) {
      ureg_ADD(shader, m[0][0], ureg_src(m[0][0]), ureg_src(m[i][0]));
      ureg_ADD(shader, m[0][1], ureg_src(m[0][1]), ureg_src(m[i][1]));
   }

   ureg_ADD(shader, m[0][0], ureg_src(m[0][0]), ureg_src(m[0][1]));
   ureg_DP4(shader, m[0][0], ureg_abs(ureg_src(m[0][0])), ureg_imm1f(shader, 1 << 14));

   ureg_MUL(shader, ureg_writemask(m[0][0], TGSI_WRITEMASK_W), ureg_abs(ureg_src(m[7][1])), ureg_imm1f(shader, 1 << 14));
   ureg_FRC(shader, m[0][0], ureg_src(m[0][0]));
   ureg_SGT(shader, m[0][0], ureg_imm1f(shader, 0.5f), ureg_abs(ureg_src(m[0][0])));

   ureg_CMP(shader, ureg_writemask(m[0][0], TGSI_WRITEMASK_W), ureg_negate(ureg_src(m[0][0])),
            ureg_imm1f(shader, 1.0f / (1 << 15)), ureg_imm1f(shader, -1.0f / (1 << 15)));
   ureg_MUL(shader, ureg_writemask(m[0][0], TGSI_WRITEMASK_W), ureg_src(m[0][0]),
            ureg_scalar(ureg_src(m[0][0]), TGSI_SWIZZLE_X));

   ureg_MOV(shader, ureg_writemask(fragment, TGSI_WRITEMASK_XYZ), ureg_src(m[7][1]));
   ureg_ADD(shader, ureg_writemask(fragment, TGSI_WRITEMASK_W), ureg_src(m[0][0]), ureg_src(m[7][1]));

   for (i = 0; i < 8; ++i) {
      ureg_release_temporary(shader, m[i][0]);
      ureg_release_temporary(shader, m[i][1]);
   }

   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, idct->pipe);
}

static void *
create_stage1_vert_shader(struct vl_idct *idct)
{
   struct ureg_program *shader;
   struct ureg_src vrect, vpos;
   struct ureg_src scale;
   struct ureg_dst t_tex, t_start;
   struct ureg_dst o_vpos, o_l_addr[2], o_r_addr[2];

   shader = ureg_create(TGSI_PROCESSOR_VERTEX);
   if (!shader)
      return NULL;

   vrect = ureg_DECL_vs_input(shader, VS_I_RECT);
   vpos = ureg_DECL_vs_input(shader, VS_I_VPOS);

   t_tex = ureg_DECL_temporary(shader);
   t_start = ureg_DECL_temporary(shader);

   o_vpos = ureg_DECL_output(shader, TGSI_SEMANTIC_POSITION, VS_O_VPOS);

   o_l_addr[0] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_L_ADDR0);
   o_l_addr[1] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_L_ADDR1);

   o_r_addr[0] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_R_ADDR0);
   o_r_addr[1] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_R_ADDR1);

   /*
    * scale = (BLOCK_WIDTH, BLOCK_HEIGHT) / (dst.width, dst.height)
    *
    * t_vpos = vpos + vrect
    * o_vpos.xy = t_vpos * scale
    * o_vpos.zw = vpos
    *
    * o_l_addr = calc_addr(...)
    * o_r_addr = calc_addr(...)
    *
    */

   scale = ureg_imm2f(shader,
      (float)BLOCK_WIDTH / idct->buffer_width,
      (float)BLOCK_HEIGHT / idct->buffer_height);

   ureg_ADD(shader, ureg_writemask(t_tex, TGSI_WRITEMASK_XY), vpos, vrect);
   ureg_MUL(shader, ureg_writemask(t_tex, TGSI_WRITEMASK_XY), ureg_src(t_tex), scale);

   ureg_MOV(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_XY), ureg_src(t_tex));
   ureg_MOV(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_ZW), ureg_imm1f(shader, 1.0f));

   ureg_MUL(shader, ureg_writemask(t_start, TGSI_WRITEMASK_XY), vpos, scale);

   calc_addr(shader, o_l_addr, ureg_src(t_tex), ureg_src(t_start), false, false, idct->buffer_width / 4);
   calc_addr(shader, o_r_addr, vrect, ureg_imm1f(shader, 0.0f), true, true, BLOCK_WIDTH / 4);

   ureg_release_temporary(shader, t_tex);
   ureg_release_temporary(shader, t_start);

   ureg_END(shader);

   return ureg_create_shader_and_destroy(shader, idct->pipe);
}

static void *
create_stage1_frag_shader(struct vl_idct *idct)
{
   struct ureg_program *shader;

   struct ureg_src l_addr[2], r_addr[2];

   struct ureg_dst l[4][2], r[2];
   struct ureg_dst *fragment;

   int i, j;

   shader = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!shader)
      return NULL;

   fragment = MALLOC(idct->nr_of_render_targets * sizeof(struct ureg_dst));

   l_addr[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_L_ADDR0, TGSI_INTERPOLATE_LINEAR);
   l_addr[1] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_L_ADDR1, TGSI_INTERPOLATE_LINEAR);

   r_addr[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_R_ADDR0, TGSI_INTERPOLATE_LINEAR);
   r_addr[1] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_R_ADDR1, TGSI_INTERPOLATE_LINEAR);

   for (i = 0; i < idct->nr_of_render_targets; ++i)
       fragment[i] = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, i);

   for (i = 0; i < 4; ++i) {
      l[i][0] = ureg_DECL_temporary(shader);
      l[i][1] = ureg_DECL_temporary(shader);
   }

   r[0] = ureg_DECL_temporary(shader);
   r[1] = ureg_DECL_temporary(shader);

   for (i = 0; i < 4; ++i) {
      increment_addr(shader, l[i], l_addr, false, false, i - 2, idct->buffer_height);
   }

   for (i = 0; i < 4; ++i) {
      struct ureg_src s_addr[2];
      s_addr[0] = ureg_src(l[i][0]);
      s_addr[1] = ureg_src(l[i][1]);
      fetch_four(shader, l[i], s_addr, ureg_DECL_sampler(shader, 0), false);
   }

   for (i = 0; i < idct->nr_of_render_targets; ++i) {
      struct ureg_src s_addr[2];

      increment_addr(shader, r, r_addr, true, true, i - (signed)idct->nr_of_render_targets / 2, BLOCK_HEIGHT);

      s_addr[0] = ureg_src(r[0]);
      s_addr[1] = ureg_src(r[1]);
      fetch_four(shader, r, s_addr, ureg_DECL_sampler(shader, 1), false);

      for (j = 0; j < 4; ++j) {
         matrix_mul(shader, ureg_writemask(fragment[i], TGSI_WRITEMASK_X << j), l[j], r);
      }
   }

   for (i = 0; i < 4; ++i) {
      ureg_release_temporary(shader, l[i][0]);
      ureg_release_temporary(shader, l[i][1]);
   }
   ureg_release_temporary(shader, r[0]);
   ureg_release_temporary(shader, r[1]);

   ureg_END(shader);

   FREE(fragment);

   return ureg_create_shader_and_destroy(shader, idct->pipe);
}

void
vl_idct_stage2_vert_shader(struct vl_idct *idct, struct ureg_program *shader,
                           unsigned first_output, struct ureg_dst tex)
{
   struct ureg_src vrect, vpos;
   struct ureg_src scale;
   struct ureg_dst t_start;
   struct ureg_dst o_l_addr[2], o_r_addr[2];

   vrect = ureg_DECL_vs_input(shader, VS_I_RECT);
   vpos = ureg_DECL_vs_input(shader, VS_I_VPOS);

   t_start = ureg_DECL_temporary(shader);

   --first_output;

   o_l_addr[0] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, first_output + VS_O_L_ADDR0);
   o_l_addr[1] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, first_output + VS_O_L_ADDR1);

   o_r_addr[0] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, first_output + VS_O_R_ADDR0);
   o_r_addr[1] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, first_output + VS_O_R_ADDR1);

   scale = ureg_imm2f(shader,
      (float)BLOCK_WIDTH / idct->buffer_width,
      (float)BLOCK_HEIGHT / idct->buffer_height);

   ureg_MUL(shader, ureg_writemask(tex, TGSI_WRITEMASK_Z),
      ureg_scalar(vrect, TGSI_SWIZZLE_X),
      ureg_imm1f(shader, BLOCK_WIDTH / idct->nr_of_render_targets));
   ureg_MUL(shader, ureg_writemask(t_start, TGSI_WRITEMASK_XY), vpos, scale);

   calc_addr(shader, o_l_addr, vrect, ureg_imm1f(shader, 0.0f), false, false, BLOCK_WIDTH / 4);
   calc_addr(shader, o_r_addr, ureg_src(tex), ureg_src(t_start), true, false, idct->buffer_height / 4);

   ureg_MOV(shader, ureg_writemask(o_r_addr[0], TGSI_WRITEMASK_Z), ureg_src(tex));
   ureg_MOV(shader, ureg_writemask(o_r_addr[1], TGSI_WRITEMASK_Z), ureg_src(tex));
}

void
vl_idct_stage2_frag_shader(struct vl_idct *idct, struct ureg_program *shader,
                           unsigned first_input, struct ureg_dst fragment)
{
   struct ureg_src l_addr[2], r_addr[2];

   struct ureg_dst l[2], r[2];

   --first_input;

   l_addr[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, first_input + VS_O_L_ADDR0, TGSI_INTERPOLATE_LINEAR);
   l_addr[1] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, first_input + VS_O_L_ADDR1, TGSI_INTERPOLATE_LINEAR);

   r_addr[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, first_input + VS_O_R_ADDR0, TGSI_INTERPOLATE_LINEAR);
   r_addr[1] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, first_input + VS_O_R_ADDR1, TGSI_INTERPOLATE_LINEAR);

   l[0] = ureg_DECL_temporary(shader);
   l[1] = ureg_DECL_temporary(shader);
   r[0] = ureg_DECL_temporary(shader);
   r[1] = ureg_DECL_temporary(shader);

   fetch_four(shader, l, l_addr, ureg_DECL_sampler(shader, 1), false);
   fetch_four(shader, r, r_addr, ureg_DECL_sampler(shader, 0), true);

   matrix_mul(shader, fragment, l, r);

   ureg_release_temporary(shader, l[0]);
   ureg_release_temporary(shader, l[1]);
   ureg_release_temporary(shader, r[0]);
   ureg_release_temporary(shader, r[1]);
}

static bool
init_shaders(struct vl_idct *idct)
{
   idct->vs_mismatch = create_mismatch_vert_shader(idct);
   if (!idct->vs_mismatch)
      goto error_vs_mismatch;

   idct->fs_mismatch = create_mismatch_frag_shader(idct);
   if (!idct->fs_mismatch)
      goto error_fs_mismatch;

   idct->vs = create_stage1_vert_shader(idct);
   if (!idct->vs)
      goto error_vs;

   idct->fs = create_stage1_frag_shader(idct);
   if (!idct->fs)
      goto error_fs;

   return true;

error_fs:
   idct->pipe->delete_vs_state(idct->pipe, idct->vs);

error_vs:
   idct->pipe->delete_vs_state(idct->pipe, idct->vs_mismatch);

error_fs_mismatch:
   idct->pipe->delete_vs_state(idct->pipe, idct->fs);

error_vs_mismatch:
   return false;
}

static void
cleanup_shaders(struct vl_idct *idct)
{
   idct->pipe->delete_vs_state(idct->pipe, idct->vs_mismatch);
   idct->pipe->delete_fs_state(idct->pipe, idct->fs_mismatch);
   idct->pipe->delete_vs_state(idct->pipe, idct->vs);
   idct->pipe->delete_fs_state(idct->pipe, idct->fs);
}

static bool
init_state(struct vl_idct *idct)
{
   struct pipe_blend_state blend;
   struct pipe_rasterizer_state rs_state;
   struct pipe_sampler_state sampler;
   unsigned i;

   assert(idct);

   memset(&rs_state, 0, sizeof(rs_state));
   rs_state.point_size = 1;
   rs_state.gl_rasterization_rules = true;
   rs_state.depth_clip = 1;
   idct->rs_state = idct->pipe->create_rasterizer_state(idct->pipe, &rs_state);
   if (!idct->rs_state)
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
   idct->blend = idct->pipe->create_blend_state(idct->pipe, &blend);
   if (!idct->blend)
      goto error_blend;

   for (i = 0; i < 2; ++i) {
      memset(&sampler, 0, sizeof(sampler));
      sampler.wrap_s = PIPE_TEX_WRAP_REPEAT;
      sampler.wrap_t = PIPE_TEX_WRAP_REPEAT;
      sampler.wrap_r = PIPE_TEX_WRAP_REPEAT;
      sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
      sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
      sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
      sampler.compare_mode = PIPE_TEX_COMPARE_NONE;
      sampler.compare_func = PIPE_FUNC_ALWAYS;
      sampler.normalized_coords = 1;
      idct->samplers[i] = idct->pipe->create_sampler_state(idct->pipe, &sampler);
      if (!idct->samplers[i])
         goto error_samplers;
   }

   return true;

error_samplers:
   for (i = 0; i < 2; ++i)
      if (idct->samplers[i])
         idct->pipe->delete_sampler_state(idct->pipe, idct->samplers[i]);

   idct->pipe->delete_rasterizer_state(idct->pipe, idct->rs_state);

error_blend:
   idct->pipe->delete_blend_state(idct->pipe, idct->blend);

error_rs_state:
   return false;
}

static void
cleanup_state(struct vl_idct *idct)
{
   unsigned i;

   for (i = 0; i < 2; ++i)
      idct->pipe->delete_sampler_state(idct->pipe, idct->samplers[i]);

   idct->pipe->delete_rasterizer_state(idct->pipe, idct->rs_state);
   idct->pipe->delete_blend_state(idct->pipe, idct->blend);
}

static bool
init_source(struct vl_idct *idct, struct vl_idct_buffer *buffer)
{
   struct pipe_resource *tex;
   struct pipe_surface surf_templ;

   assert(idct && buffer);

   tex = buffer->sampler_views.individual.source->texture;

   buffer->fb_state_mismatch.width = tex->width0;
   buffer->fb_state_mismatch.height = tex->height0;
   buffer->fb_state_mismatch.nr_cbufs = 1;

   memset(&surf_templ, 0, sizeof(surf_templ));
   surf_templ.format = tex->format;
   surf_templ.u.tex.first_layer = 0;
   surf_templ.u.tex.last_layer = 0;
   surf_templ.usage = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
   buffer->fb_state_mismatch.cbufs[0] = idct->pipe->create_surface(idct->pipe, tex, &surf_templ);

   buffer->viewport_mismatch.scale[0] = tex->width0;
   buffer->viewport_mismatch.scale[1] = tex->height0;
   buffer->viewport_mismatch.scale[2] = 1;
   buffer->viewport_mismatch.scale[3] = 1;

   return true;
}

static void
cleanup_source(struct vl_idct_buffer *buffer)
{
   assert(buffer);

   pipe_surface_reference(&buffer->fb_state_mismatch.cbufs[0], NULL);

   pipe_sampler_view_reference(&buffer->sampler_views.individual.source, NULL);
}

static bool
init_intermediate(struct vl_idct *idct, struct vl_idct_buffer *buffer)
{
   struct pipe_resource *tex;
   struct pipe_surface surf_templ;
   unsigned i;

   assert(idct && buffer);

   tex = buffer->sampler_views.individual.intermediate->texture;

   buffer->fb_state.width = tex->width0;
   buffer->fb_state.height = tex->height0;
   buffer->fb_state.nr_cbufs = idct->nr_of_render_targets;
   for(i = 0; i < idct->nr_of_render_targets; ++i) {
      memset(&surf_templ, 0, sizeof(surf_templ));
      surf_templ.format = tex->format;
      surf_templ.u.tex.first_layer = i;
      surf_templ.u.tex.last_layer = i;
      surf_templ.usage = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
      buffer->fb_state.cbufs[i] = idct->pipe->create_surface(
         idct->pipe, tex, &surf_templ);

      if (!buffer->fb_state.cbufs[i])
         goto error_surfaces;
   }

   buffer->viewport.scale[0] = tex->width0;
   buffer->viewport.scale[1] = tex->height0;
   buffer->viewport.scale[2] = 1;
   buffer->viewport.scale[3] = 1;

   return true;

error_surfaces:
   for(i = 0; i < idct->nr_of_render_targets; ++i)
      pipe_surface_reference(&buffer->fb_state.cbufs[i], NULL);

   return false;
}

static void
cleanup_intermediate(struct vl_idct_buffer *buffer)
{
   unsigned i;

   assert(buffer);

   for(i = 0; i < PIPE_MAX_COLOR_BUFS; ++i)
      pipe_surface_reference(&buffer->fb_state.cbufs[i], NULL);

   pipe_sampler_view_reference(&buffer->sampler_views.individual.intermediate, NULL);
}

struct pipe_sampler_view *
vl_idct_upload_matrix(struct pipe_context *pipe, float scale)
{
   struct pipe_resource tex_templ, *matrix;
   struct pipe_sampler_view sv_templ, *sv;
   struct pipe_transfer *buf_transfer;
   unsigned i, j, pitch;
   float *f;

   struct pipe_box rect =
   {
      0, 0, 0,
      BLOCK_WIDTH / 4,
      BLOCK_HEIGHT,
      1
   };

   assert(pipe);

   memset(&tex_templ, 0, sizeof(tex_templ));
   tex_templ.target = PIPE_TEXTURE_2D;
   tex_templ.format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   tex_templ.last_level = 0;
   tex_templ.width0 = 2;
   tex_templ.height0 = 8;
   tex_templ.depth0 = 1;
   tex_templ.array_size = 1;
   tex_templ.usage = PIPE_USAGE_IMMUTABLE;
   tex_templ.bind = PIPE_BIND_SAMPLER_VIEW;
   tex_templ.flags = 0;

   matrix = pipe->screen->resource_create(pipe->screen, &tex_templ);
   if (!matrix)
      goto error_matrix;

   buf_transfer = pipe->get_transfer
   (
      pipe, matrix,
      0, PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD_RANGE,
      &rect
   );
   if (!buf_transfer)
      goto error_transfer;

   pitch = buf_transfer->stride / sizeof(float);

   f = pipe->transfer_map(pipe, buf_transfer);
   if (!f)
      goto error_map;

   for(i = 0; i < BLOCK_HEIGHT; ++i)
      for(j = 0; j < BLOCK_WIDTH; ++j)
         // transpose and scale
         f[i * pitch + j] = ((const float (*)[8])const_matrix)[j][i] * scale;

   pipe->transfer_unmap(pipe, buf_transfer);
   pipe->transfer_destroy(pipe, buf_transfer);

   memset(&sv_templ, 0, sizeof(sv_templ));
   u_sampler_view_default_template(&sv_templ, matrix, matrix->format);
   sv = pipe->create_sampler_view(pipe, matrix, &sv_templ);
   pipe_resource_reference(&matrix, NULL);
   if (!sv)
      goto error_map;

   return sv;

error_map:
   pipe->transfer_destroy(pipe, buf_transfer);

error_transfer:
   pipe_resource_reference(&matrix, NULL);

error_matrix:
   return NULL;
}

bool vl_idct_init(struct vl_idct *idct, struct pipe_context *pipe,
                  unsigned buffer_width, unsigned buffer_height,
                  unsigned nr_of_render_targets,
                  struct pipe_sampler_view *matrix,
                  struct pipe_sampler_view *transpose)
{
   assert(idct && pipe);
   assert(matrix && transpose);

   idct->pipe = pipe;
   idct->buffer_width = buffer_width;
   idct->buffer_height = buffer_height;
   idct->nr_of_render_targets = nr_of_render_targets;

   pipe_sampler_view_reference(&idct->matrix, matrix);
   pipe_sampler_view_reference(&idct->transpose, transpose);

   if(!init_shaders(idct))
      return false;

   if(!init_state(idct)) {
      cleanup_shaders(idct);
      return false;
   }

   return true;
}

void
vl_idct_cleanup(struct vl_idct *idct)
{
   cleanup_shaders(idct);
   cleanup_state(idct);

   pipe_sampler_view_reference(&idct->matrix, NULL);
   pipe_sampler_view_reference(&idct->transpose, NULL);
}

bool
vl_idct_init_buffer(struct vl_idct *idct, struct vl_idct_buffer *buffer,
                    struct pipe_sampler_view *source,
                    struct pipe_sampler_view *intermediate)
{
   assert(buffer && idct);
   assert(source && intermediate);

   memset(buffer, 0, sizeof(struct vl_idct_buffer));

   pipe_sampler_view_reference(&buffer->sampler_views.individual.matrix, idct->matrix);
   pipe_sampler_view_reference(&buffer->sampler_views.individual.source, source);
   pipe_sampler_view_reference(&buffer->sampler_views.individual.transpose, idct->transpose);
   pipe_sampler_view_reference(&buffer->sampler_views.individual.intermediate, intermediate);

   if (!init_source(idct, buffer))
      return false;

   if (!init_intermediate(idct, buffer))
      return false;

   return true;
}

void
vl_idct_cleanup_buffer(struct vl_idct_buffer *buffer)
{
   assert(buffer);

   cleanup_source(buffer);
   cleanup_intermediate(buffer);

   pipe_sampler_view_reference(&buffer->sampler_views.individual.matrix, NULL);
   pipe_sampler_view_reference(&buffer->sampler_views.individual.transpose, NULL);
}

void
vl_idct_flush(struct vl_idct *idct, struct vl_idct_buffer *buffer, unsigned num_instances)
{
   assert(buffer);

   idct->pipe->bind_rasterizer_state(idct->pipe, idct->rs_state);
   idct->pipe->bind_blend_state(idct->pipe, idct->blend);
   idct->pipe->bind_fragment_sampler_states(idct->pipe, 2, idct->samplers);
   idct->pipe->set_fragment_sampler_views(idct->pipe, 2, buffer->sampler_views.stage[0]);

   /* mismatch control */
   idct->pipe->set_framebuffer_state(idct->pipe, &buffer->fb_state_mismatch);
   idct->pipe->set_viewport_state(idct->pipe, &buffer->viewport_mismatch);
   idct->pipe->bind_vs_state(idct->pipe, idct->vs_mismatch);
   idct->pipe->bind_fs_state(idct->pipe, idct->fs_mismatch);
   util_draw_arrays_instanced(idct->pipe, PIPE_PRIM_POINTS, 0, 1, 0, num_instances);

   /* first stage */
   idct->pipe->set_framebuffer_state(idct->pipe, &buffer->fb_state);
   idct->pipe->set_viewport_state(idct->pipe, &buffer->viewport);
   idct->pipe->bind_vs_state(idct->pipe, idct->vs);
   idct->pipe->bind_fs_state(idct->pipe, idct->fs);
   util_draw_arrays_instanced(idct->pipe, PIPE_PRIM_QUADS, 0, 4, 0, num_instances);
}

void
vl_idct_prepare_stage2(struct vl_idct *idct, struct vl_idct_buffer *buffer)
{
   assert(buffer);

   /* second stage */
   idct->pipe->bind_rasterizer_state(idct->pipe, idct->rs_state);
   idct->pipe->bind_fragment_sampler_states(idct->pipe, 2, idct->samplers);
   idct->pipe->set_fragment_sampler_views(idct->pipe, 2, buffer->sampler_views.stage[1]);
}

