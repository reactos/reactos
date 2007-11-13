/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */

#include "glheader.h"
#include "macros.h"
#include "enums.h"

#include "shader/program.h"
#include "intel_batchbuffer.h"

#include "brw_defines.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_util.h"
#include "brw_clip.h"



static void brw_clip_line_alloc_regs( struct brw_clip_compile *c )
{
   GLuint i = 0,j;

   /* Register usage is static, precompute here:
    */
   c->reg.R0 = retype(brw_vec8_grf(i, 0), BRW_REGISTER_TYPE_UD); i++;

   if (c->key.nr_userclip) {
      c->reg.fixed_planes = brw_vec4_grf(i, 0);
      i += (6 + c->key.nr_userclip + 1) / 2;

      c->prog_data.curb_read_length = (6 + c->key.nr_userclip + 1) / 2;
   }
   else
      c->prog_data.curb_read_length = 0;


   /* Payload vertices plus space for more generated vertices:
    */
   for (j = 0; j < 4; j++) {
      c->reg.vertex[j] = brw_vec4_grf(i, 0);
      i += c->nr_regs;
   }

   c->reg.t           = brw_vec1_grf(i, 0);
   c->reg.t0          = brw_vec1_grf(i, 1);
   c->reg.t1          = brw_vec1_grf(i, 2);
   c->reg.planemask   = retype(brw_vec1_grf(i, 3), BRW_REGISTER_TYPE_UD);
   c->reg.plane_equation = brw_vec4_grf(i, 4);
   i++;

   c->reg.dp0         = brw_vec1_grf(i, 0); /* fixme - dp4 will clobber r.1,2,3 */
   c->reg.dp1         = brw_vec1_grf(i, 4);
   i++;

   if (!c->key.nr_userclip) {
      c->reg.fixed_planes = brw_vec8_grf(i, 0); 
      i++;
   }


   c->first_tmp = i;
   c->last_tmp = i;

   c->prog_data.urb_read_length = c->nr_regs; /* ? */
   c->prog_data.total_grf = i;
}



/* Line clipping, more or less following the following algorithm:
 *
 *  for (p=0;p<MAX_PLANES;p++) {
 *     if (clipmask & (1 << p)) {
 *        GLfloat dp0 = DOTPROD( vtx0, plane[p] );
 *        GLfloat dp1 = DOTPROD( vtx1, plane[p] );
 *
 *        if (IS_NEGATIVE(dp1)) {
 *           GLfloat t = dp1 / (dp1 - dp0);
 *           if (t > t1) t1 = t;
 *        } else {
 *           GLfloat t = dp0 / (dp0 - dp1);
 *           if (t > t0) t0 = t;
 *        }
 *  
 *        if (t0 + t1 >= 1.0)
 *           return;
 *     }
 *  }
 *
 *  interp( ctx, newvtx0, vtx0, vtx1, t0 );
 *  interp( ctx, newvtx1, vtx1, vtx0, t1 );
 *
 */
static void clip_and_emit_line( struct brw_clip_compile *c )
{
   struct brw_compile *p = &c->func;
   struct brw_indirect vtx0     = brw_indirect(0, 0);
   struct brw_indirect vtx1      = brw_indirect(1, 0);
   struct brw_indirect newvtx0   = brw_indirect(2, 0);
   struct brw_indirect newvtx1   = brw_indirect(3, 0);
   struct brw_indirect plane_ptr = brw_indirect(4, 0);
   struct brw_instruction *plane_loop;
   struct brw_instruction *plane_active;
   struct brw_instruction *is_negative;
   struct brw_instruction *not_culled;
   struct brw_reg v1_null_ud = retype(vec1(brw_null_reg()), BRW_REGISTER_TYPE_UD);

   brw_MOV(p, get_addr_reg(vtx0),      brw_address(c->reg.vertex[0]));
   brw_MOV(p, get_addr_reg(vtx1),      brw_address(c->reg.vertex[1]));
   brw_MOV(p, get_addr_reg(newvtx0),   brw_address(c->reg.vertex[2]));
   brw_MOV(p, get_addr_reg(newvtx1),   brw_address(c->reg.vertex[3]));
   brw_MOV(p, get_addr_reg(plane_ptr), brw_clip_plane0_address(c));

   /* Note: init t0, t1 together: 
    */
   brw_MOV(p, vec2(c->reg.t0), brw_imm_f(0));

   brw_clip_init_planes(c);
   brw_clip_init_clipmask(c);

   plane_loop = brw_DO(p, BRW_EXECUTE_1);
   {
      /* if (planemask & 1)
       */
      brw_set_conditionalmod(p, BRW_CONDITIONAL_NZ);
      brw_AND(p, v1_null_ud, c->reg.planemask, brw_imm_ud(1));
      
      plane_active = brw_IF(p, BRW_EXECUTE_1);
      {
	 if (c->key.nr_userclip)
	    brw_MOV(p, c->reg.plane_equation, deref_4f(plane_ptr, 0));
	 else
	    brw_MOV(p, c->reg.plane_equation, deref_4b(plane_ptr, 0));

	 /* dp = DP4(vtx->position, plane) 
	  */
	 brw_DP4(p, vec4(c->reg.dp0), deref_4f(vtx0, c->offset[VERT_RESULT_HPOS]), c->reg.plane_equation);

	 /* if (IS_NEGATIVE(dp1)) 
	  */
	 brw_set_conditionalmod(p, BRW_CONDITIONAL_L);
	 brw_DP4(p, vec4(c->reg.dp1), deref_4f(vtx1, c->offset[VERT_RESULT_HPOS]), c->reg.plane_equation);
	 is_negative = brw_IF(p, BRW_EXECUTE_1);
	 {
	    brw_ADD(p, c->reg.t, c->reg.dp1, negate(c->reg.dp0));
	    brw_math_invert(p, c->reg.t, c->reg.t);
	    brw_MUL(p, c->reg.t, c->reg.t, c->reg.dp1);

	    brw_CMP(p, vec1(brw_null_reg()), BRW_CONDITIONAL_G, c->reg.t, c->reg.t1 );
	    brw_MOV(p, c->reg.t1, c->reg.t);
	    brw_set_predicate_control(p, BRW_PREDICATE_NONE);
	 } 
	 is_negative = brw_ELSE(p, is_negative);
	 {
	    /* Coming back in.  We know that both cannot be negative
	     * because the line would have been culled in that case.
	     */
	    brw_ADD(p, c->reg.t, c->reg.dp0, negate(c->reg.dp1));
	    brw_math_invert(p, c->reg.t, c->reg.t);
	    brw_MUL(p, c->reg.t, c->reg.t, c->reg.dp0);

	    brw_CMP(p, vec1(brw_null_reg()), BRW_CONDITIONAL_G, c->reg.t, c->reg.t0 );
	    brw_MOV(p, c->reg.t0, c->reg.t);
	    brw_set_predicate_control(p, BRW_PREDICATE_NONE);
	 }
	 brw_ENDIF(p, is_negative);	 
      }
      brw_ENDIF(p, plane_active);
      
      /* plane_ptr++;
       */
      brw_ADD(p, get_addr_reg(plane_ptr), get_addr_reg(plane_ptr), brw_clip_plane_stride(c));

      /* while (planemask>>=1) != 0
       */
      brw_set_conditionalmod(p, BRW_CONDITIONAL_NZ);
      brw_SHR(p, c->reg.planemask, c->reg.planemask, brw_imm_ud(1));
   }
   brw_WHILE(p, plane_loop);

   brw_ADD(p, c->reg.t, c->reg.t0, c->reg.t1);
   brw_CMP(p, vec1(brw_null_reg()), BRW_CONDITIONAL_L, c->reg.t, brw_imm_f(1.0));
   not_culled = brw_IF(p, BRW_EXECUTE_1);
   {
      brw_clip_interp_vertex(c, newvtx0, vtx0, vtx1, c->reg.t0, GL_FALSE);
      brw_clip_interp_vertex(c, newvtx1, vtx1, vtx0, c->reg.t1, GL_FALSE);

      brw_clip_emit_vue(c, newvtx0, 1, 0, (_3DPRIM_LINESTRIP << 2) | R02_PRIM_START);
      brw_clip_emit_vue(c, newvtx1, 0, 1, (_3DPRIM_LINESTRIP << 2) | R02_PRIM_END); 
   }
   brw_ENDIF(p, not_culled);
   brw_clip_kill_thread(c);
}



void brw_emit_line_clip( struct brw_clip_compile *c )
{
   brw_clip_line_alloc_regs(c);

   if (c->key.do_flat_shading)
      brw_clip_copy_colors(c, 0, 1);
                
   clip_and_emit_line(c);
}
