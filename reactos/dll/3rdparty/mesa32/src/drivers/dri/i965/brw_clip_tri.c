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



void brw_clip_tri_alloc_regs( struct brw_clip_compile *c, 
			      GLuint nr_verts )
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
   for (j = 0; j < nr_verts; j++) {
      c->reg.vertex[j] = brw_vec4_grf(i, 0);
      i += c->nr_regs;
   }

   if (c->nr_attrs & 1) {
      for (j = 0; j < 3; j++) {
	 GLuint delta = c->nr_attrs*16 + 32;
	 brw_MOV(&c->func, byte_offset(c->reg.vertex[j], delta), brw_imm_f(0));
      }
   }

   c->reg.t          = brw_vec1_grf(i, 0);
   c->reg.loopcount  = retype(brw_vec1_grf(i, 1), BRW_REGISTER_TYPE_UD);
   c->reg.nr_verts   = retype(brw_vec1_grf(i, 2), BRW_REGISTER_TYPE_UD);
   c->reg.planemask  = retype(brw_vec1_grf(i, 3), BRW_REGISTER_TYPE_UD);
   c->reg.plane_equation = brw_vec4_grf(i, 4);
   i++;

   c->reg.dpPrev     = brw_vec1_grf(i, 0); /* fixme - dp4 will clobber r.1,2,3 */
   c->reg.dp         = brw_vec1_grf(i, 4);
   i++;

   c->reg.inlist     = brw_uw16_reg(BRW_GENERAL_REGISTER_FILE, i, 0);
   i++;

   c->reg.outlist    = brw_uw16_reg(BRW_GENERAL_REGISTER_FILE, i, 0);
   i++;

   c->reg.freelist   = brw_uw16_reg(BRW_GENERAL_REGISTER_FILE, i, 0);
   i++;

   if (!c->key.nr_userclip) {
      c->reg.fixed_planes = brw_vec8_grf(i, 0); 
      i++;
   }

   if (c->key.do_unfilled) {
      c->reg.dir     = brw_vec4_grf(i, 0);
      c->reg.offset  = brw_vec4_grf(i, 4);
      i++;
      c->reg.tmp0    = brw_vec4_grf(i, 0);
      c->reg.tmp1    = brw_vec4_grf(i, 4);
      i++;
   }

   c->first_tmp = i;
   c->last_tmp = i;

   c->prog_data.urb_read_length = c->nr_regs; /* ? */
   c->prog_data.total_grf = i;
}



void brw_clip_tri_init_vertices( struct brw_clip_compile *c )
{
   struct brw_compile *p = &c->func;
   struct brw_reg tmp0 = c->reg.loopcount; /* handy temporary */
   struct brw_instruction *is_rev;

   /* Initial list of indices for incoming vertexes:
    */
   brw_AND(p, tmp0, get_element_ud(c->reg.R0, 2), brw_imm_ud(PRIM_MASK)); 
   brw_CMP(p, 
	   vec1(brw_null_reg()), 
	   BRW_CONDITIONAL_EQ, 
	   tmp0,
	   brw_imm_ud(_3DPRIM_TRISTRIP_REVERSE));

   /* XXX: Is there an easier way to do this?  Need to reverse every
    * second tristrip element:  Can ignore sometimes?
    */
   is_rev = brw_IF(p, BRW_EXECUTE_1);
   {   
      brw_MOV(p, get_element(c->reg.inlist, 0),  brw_address(c->reg.vertex[1]) );
      brw_MOV(p, get_element(c->reg.inlist, 1),  brw_address(c->reg.vertex[0]) );
      if (c->need_direction)
	 brw_MOV(p, c->reg.dir, brw_imm_f(-1));
   }
   is_rev = brw_ELSE(p, is_rev);
   {
      brw_MOV(p, get_element(c->reg.inlist, 0),  brw_address(c->reg.vertex[0]) );
      brw_MOV(p, get_element(c->reg.inlist, 1),  brw_address(c->reg.vertex[1]) );
      if (c->need_direction)
	 brw_MOV(p, c->reg.dir, brw_imm_f(1));
   }
   brw_ENDIF(p, is_rev);

   brw_MOV(p, get_element(c->reg.inlist, 2),  brw_address(c->reg.vertex[2]) );
   brw_MOV(p, brw_vec8_grf(c->reg.outlist.nr, 0), brw_imm_f(0));
   brw_MOV(p, c->reg.nr_verts, brw_imm_ud(3));
}



void brw_clip_tri_flat_shade( struct brw_clip_compile *c )
{
   struct brw_compile *p = &c->func;
   struct brw_instruction *is_poly;
   struct brw_reg tmp0 = c->reg.loopcount; /* handy temporary */

   brw_AND(p, tmp0, get_element_ud(c->reg.R0, 2), brw_imm_ud(PRIM_MASK)); 
   brw_CMP(p, 
	   vec1(brw_null_reg()), 
	   BRW_CONDITIONAL_EQ, 
	   tmp0,
	   brw_imm_ud(_3DPRIM_POLYGON));

   is_poly = brw_IF(p, BRW_EXECUTE_1);
   {   
      brw_clip_copy_colors(c, 1, 0);
      brw_clip_copy_colors(c, 2, 0);
   }
   is_poly = brw_ELSE(p, is_poly);
   {
      brw_clip_copy_colors(c, 0, 2);
      brw_clip_copy_colors(c, 1, 2);
   }
   brw_ENDIF(p, is_poly);
}



/* Use mesa's clipping algorithms, translated to GEN4 assembly.
 */
void brw_clip_tri( struct brw_clip_compile *c )
{
   struct brw_compile *p = &c->func;
   struct brw_indirect vtx = brw_indirect(0, 0);
   struct brw_indirect vtxPrev = brw_indirect(1, 0);
   struct brw_indirect vtxOut = brw_indirect(2, 0);
   struct brw_indirect plane_ptr = brw_indirect(3, 0);
   struct brw_indirect inlist_ptr = brw_indirect(4, 0);
   struct brw_indirect outlist_ptr = brw_indirect(5, 0);
   struct brw_indirect freelist_ptr = brw_indirect(6, 0);
   struct brw_instruction *plane_loop;
   struct brw_instruction *plane_active;
   struct brw_instruction *vertex_loop;
   struct brw_instruction *next_test;
   struct brw_instruction *prev_test;
   
   brw_MOV(p, get_addr_reg(vtxPrev),     brw_address(c->reg.vertex[2]) );
   brw_MOV(p, get_addr_reg(plane_ptr),   brw_clip_plane0_address(c));
   brw_MOV(p, get_addr_reg(inlist_ptr),  brw_address(c->reg.inlist));
   brw_MOV(p, get_addr_reg(outlist_ptr), brw_address(c->reg.outlist));

   brw_MOV(p, get_addr_reg(freelist_ptr), brw_address(c->reg.vertex[3]) );

   plane_loop = brw_DO(p, BRW_EXECUTE_1);
   {
      /* if (planemask & 1)
       */
      brw_set_conditionalmod(p, BRW_CONDITIONAL_NZ);
      brw_AND(p, vec1(brw_null_reg()), c->reg.planemask, brw_imm_ud(1));
      
      plane_active = brw_IF(p, BRW_EXECUTE_1);
      {
	 /* vtxOut = freelist_ptr++ 
	  */
	 brw_MOV(p, get_addr_reg(vtxOut),       get_addr_reg(freelist_ptr) );
	 brw_ADD(p, get_addr_reg(freelist_ptr), get_addr_reg(freelist_ptr), brw_imm_uw(c->nr_regs * REG_SIZE));

	 if (c->key.nr_userclip)
	    brw_MOV(p, c->reg.plane_equation, deref_4f(plane_ptr, 0));
	 else
	    brw_MOV(p, c->reg.plane_equation, deref_4b(plane_ptr, 0));
	    
	 brw_MOV(p, c->reg.loopcount, c->reg.nr_verts);
	 brw_MOV(p, c->reg.nr_verts, brw_imm_ud(0));

	 vertex_loop = brw_DO(p, BRW_EXECUTE_1);
	 {
	    /* vtx = *input_ptr;
	     */
	    brw_MOV(p, get_addr_reg(vtx), deref_1uw(inlist_ptr, 0));

	    /* IS_NEGATIVE(prev) */
	    brw_set_conditionalmod(p, BRW_CONDITIONAL_L);
	    brw_DP4(p, vec4(c->reg.dpPrev), deref_4f(vtxPrev, c->offset[VERT_RESULT_HPOS]), c->reg.plane_equation);
	    prev_test = brw_IF(p, BRW_EXECUTE_1);
	    {
	       /* IS_POSITIVE(next)
		*/
	       brw_set_conditionalmod(p, BRW_CONDITIONAL_GE);
	       brw_DP4(p, vec4(c->reg.dp), deref_4f(vtx, c->offset[VERT_RESULT_HPOS]), c->reg.plane_equation);
	       next_test = brw_IF(p, BRW_EXECUTE_1);
	       {

		  /* Coming back in.
		   */
		  brw_ADD(p, c->reg.t, c->reg.dpPrev, negate(c->reg.dp));
		  brw_math_invert(p, c->reg.t, c->reg.t);
		  brw_MUL(p, c->reg.t, c->reg.t, c->reg.dpPrev);

		  /* If (vtxOut == 0) vtxOut = vtxPrev
		   */
		  brw_CMP(p, vec1(brw_null_reg()), BRW_CONDITIONAL_EQ, get_addr_reg(vtxOut), brw_imm_uw(0) );
		  brw_MOV(p, get_addr_reg(vtxOut), get_addr_reg(vtxPrev) );
		  brw_set_predicate_control(p, BRW_PREDICATE_NONE);

		  brw_clip_interp_vertex(c, vtxOut, vtxPrev, vtx, c->reg.t, GL_FALSE);

		  /* *outlist_ptr++ = vtxOut;
		   * nr_verts++; 
		   * vtxOut = 0;
		   */
		  brw_MOV(p, deref_1uw(outlist_ptr, 0), get_addr_reg(vtxOut));
		  brw_ADD(p, get_addr_reg(outlist_ptr), get_addr_reg(outlist_ptr), brw_imm_uw(sizeof(short)));
		  brw_ADD(p, c->reg.nr_verts, c->reg.nr_verts, brw_imm_ud(1));
		  brw_MOV(p, get_addr_reg(vtxOut), brw_imm_uw(0) );
	       }
	       brw_ENDIF(p, next_test);
	       
	    }
	    prev_test = brw_ELSE(p, prev_test);
	    {
	       /* *outlist_ptr++ = vtxPrev;
		* nr_verts++;
		*/
	       brw_MOV(p, deref_1uw(outlist_ptr, 0), get_addr_reg(vtxPrev));
	       brw_ADD(p, get_addr_reg(outlist_ptr), get_addr_reg(outlist_ptr), brw_imm_uw(sizeof(short)));
	       brw_ADD(p, c->reg.nr_verts, c->reg.nr_verts, brw_imm_ud(1));

	       /* IS_NEGATIVE(next)
		*/
	       brw_set_conditionalmod(p, BRW_CONDITIONAL_L);
	       brw_DP4(p, vec4(c->reg.dp), deref_4f(vtx, c->offset[VERT_RESULT_HPOS]), c->reg.plane_equation);
	       next_test = brw_IF(p, BRW_EXECUTE_1);
	       {
		  /* Going out of bounds.  Avoid division by zero as we
		   * know dp != dpPrev from DIFFERENT_SIGNS, above.
		   */
		  brw_ADD(p, c->reg.t, c->reg.dp, negate(c->reg.dpPrev));
		  brw_math_invert(p, c->reg.t, c->reg.t);
		  brw_MUL(p, c->reg.t, c->reg.t, c->reg.dp);

		  /* If (vtxOut == 0) vtxOut = vtx
		   */
		  brw_CMP(p, vec1(brw_null_reg()), BRW_CONDITIONAL_EQ, get_addr_reg(vtxOut), brw_imm_uw(0) );
		  brw_MOV(p, get_addr_reg(vtxOut), get_addr_reg(vtx) );
		  brw_set_predicate_control(p, BRW_PREDICATE_NONE);

		  brw_clip_interp_vertex(c, vtxOut, vtx, vtxPrev, c->reg.t, GL_TRUE);		  

		  /* *outlist_ptr++ = vtxOut;
		   * nr_verts++; 
		   * vtxOut = 0;
		   */
		  brw_MOV(p, deref_1uw(outlist_ptr, 0), get_addr_reg(vtxOut));
		  brw_ADD(p, get_addr_reg(outlist_ptr), get_addr_reg(outlist_ptr), brw_imm_uw(sizeof(short)));
		  brw_ADD(p, c->reg.nr_verts, c->reg.nr_verts, brw_imm_ud(1));
		  brw_MOV(p, get_addr_reg(vtxOut), brw_imm_uw(0) );
	       } 	       
	       brw_ENDIF(p, next_test);
	    }
	    brw_ENDIF(p, prev_test);
	    
	    /* vtxPrev = vtx;
	     * inlist_ptr++;
	     */
	    brw_MOV(p, get_addr_reg(vtxPrev), get_addr_reg(vtx));
	    brw_ADD(p, get_addr_reg(inlist_ptr), get_addr_reg(inlist_ptr), brw_imm_uw(sizeof(short)));

	    /* while (--loopcount != 0)
	     */
	    brw_set_conditionalmod(p, BRW_CONDITIONAL_NZ);
	    brw_ADD(p, c->reg.loopcount, c->reg.loopcount, brw_imm_d(-1));
	 } 
	 brw_WHILE(p, vertex_loop);

	 /* vtxPrev = *(outlist_ptr-1)  OR: outlist[nr_verts-1]
	  * inlist = outlist
	  * inlist_ptr = &inlist[0]
	  * outlist_ptr = &outlist[0]
	  */
	 brw_ADD(p, get_addr_reg(outlist_ptr), get_addr_reg(outlist_ptr), brw_imm_w(-2));
	 brw_MOV(p, get_addr_reg(vtxPrev), deref_1uw(outlist_ptr, 0));
	 brw_MOV(p, brw_vec8_grf(c->reg.inlist.nr, 0), brw_vec8_grf(c->reg.outlist.nr, 0));
	 brw_MOV(p, get_addr_reg(inlist_ptr), brw_address(c->reg.inlist));
	 brw_MOV(p, get_addr_reg(outlist_ptr), brw_address(c->reg.outlist));
      }
      brw_ENDIF(p, plane_active);
      
      /* plane_ptr++;
       */
      brw_ADD(p, get_addr_reg(plane_ptr), get_addr_reg(plane_ptr), brw_clip_plane_stride(c));

      /* nr_verts >= 3 
       */
      brw_CMP(p,
	      vec1(brw_null_reg()),
	      BRW_CONDITIONAL_GE,
	      c->reg.nr_verts,
	      brw_imm_ud(3));
   
      /* && (planemask>>=1) != 0
       */
      brw_set_conditionalmod(p, BRW_CONDITIONAL_NZ);
      brw_SHR(p, c->reg.planemask, c->reg.planemask, brw_imm_ud(1));
   }
   brw_WHILE(p, plane_loop);
}



void brw_clip_tri_emit_polygon(struct brw_clip_compile *c)
{
   struct brw_compile *p = &c->func;
   struct brw_instruction *loop, *if_insn;

   /* for (loopcount = nr_verts-2; loopcount > 0; loopcount--)
    */
   brw_set_conditionalmod(p, BRW_CONDITIONAL_G);
   brw_ADD(p,
	   c->reg.loopcount,
	   c->reg.nr_verts,
	   brw_imm_d(-2));

   if_insn = brw_IF(p, BRW_EXECUTE_1);
   {
      struct brw_indirect v0 = brw_indirect(0, 0);
      struct brw_indirect vptr = brw_indirect(1, 0);

      brw_MOV(p, get_addr_reg(vptr), brw_address(c->reg.inlist));
      brw_MOV(p, get_addr_reg(v0), deref_1uw(vptr, 0));

      brw_clip_emit_vue(c, v0, 1, 0, ((_3DPRIM_TRIFAN << 2) | R02_PRIM_START));
      
      brw_ADD(p, get_addr_reg(vptr), get_addr_reg(vptr), brw_imm_uw(2));
      brw_MOV(p, get_addr_reg(v0), deref_1uw(vptr, 0));

      loop = brw_DO(p, BRW_EXECUTE_1);
      {
	 brw_clip_emit_vue(c, v0, 1, 0, (_3DPRIM_TRIFAN << 2));
  
	 brw_ADD(p, get_addr_reg(vptr), get_addr_reg(vptr), brw_imm_uw(2));
	 brw_MOV(p, get_addr_reg(v0), deref_1uw(vptr, 0));

	 brw_set_conditionalmod(p, BRW_CONDITIONAL_NZ);
	 brw_ADD(p, c->reg.loopcount, c->reg.loopcount, brw_imm_d(-1));
      }
      brw_WHILE(p, loop);

      brw_clip_emit_vue(c, v0, 0, 1, ((_3DPRIM_TRIFAN << 2) | R02_PRIM_END));
   }
   brw_ENDIF(p, if_insn);
}

static void do_clip_tri( struct brw_clip_compile *c )
{
   brw_clip_init_planes(c);

   brw_clip_tri(c);
}


static void maybe_do_clip_tri( struct brw_clip_compile *c )
{
   struct brw_compile *p = &c->func;
   struct brw_instruction *do_clip;

   brw_CMP(p, vec1(brw_null_reg()), BRW_CONDITIONAL_NZ, c->reg.planemask, brw_imm_ud(0));
   do_clip = brw_IF(p, BRW_EXECUTE_1);
   {
      do_clip_tri(c);
   }
   brw_ENDIF(p, do_clip);
}




void brw_emit_tri_clip( struct brw_clip_compile *c )
{
   brw_clip_tri_alloc_regs(c, 3 + c->key.nr_userclip + 6);
   brw_clip_tri_init_vertices(c);
   brw_clip_init_clipmask(c);

   /* Can't push into do_clip_tri because with polygon (or quad)
    * flatshading, need to apply the flatshade here because we don't
    * respect the PV when converting to trifan for emit:
    */
   if (c->key.do_flat_shading) 
      brw_clip_tri_flat_shade(c); 
      
   if (c->key.clip_mode == BRW_CLIPMODE_NORMAL)
      do_clip_tri(c);
   else 
      maybe_do_clip_tri(c);
      
   brw_clip_tri_emit_polygon(c);

   /* Send an empty message to kill the thread:
    */
   brw_clip_kill_thread(c);
}



