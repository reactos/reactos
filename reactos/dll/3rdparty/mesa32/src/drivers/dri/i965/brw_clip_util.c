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





static struct brw_reg get_tmp( struct brw_clip_compile *c )
{
   struct brw_reg tmp = brw_vec4_grf(c->last_tmp, 0);

   if (++c->last_tmp > c->prog_data.total_grf)
      c->prog_data.total_grf = c->last_tmp;

   return tmp;
}

static void release_tmp( struct brw_clip_compile *c, struct brw_reg tmp )
{
   if (tmp.nr == c->last_tmp-1)
      c->last_tmp--;
}


static struct brw_reg make_plane_ud(GLuint x, GLuint y, GLuint z, GLuint w)
{
   return brw_imm_ud((w<<24) | (z<<16) | (y<<8) | x);
}


void brw_clip_init_planes( struct brw_clip_compile *c )
{
   struct brw_compile *p = &c->func;

   if (!c->key.nr_userclip) {
      brw_MOV(p, get_element_ud(c->reg.fixed_planes, 0), make_plane_ud( 0,    0, 0xff, 1));
      brw_MOV(p, get_element_ud(c->reg.fixed_planes, 1), make_plane_ud( 0,    0,    1, 1));
      brw_MOV(p, get_element_ud(c->reg.fixed_planes, 2), make_plane_ud( 0, 0xff,    0, 1));
      brw_MOV(p, get_element_ud(c->reg.fixed_planes, 3), make_plane_ud( 0,    1,    0, 1));
      brw_MOV(p, get_element_ud(c->reg.fixed_planes, 4), make_plane_ud(0xff,  0,    0, 1));
      brw_MOV(p, get_element_ud(c->reg.fixed_planes, 5), make_plane_ud( 1,    0,    0, 1));
   }
}



#define W 3

/* Project 'pos' to screen space (or back again), overwrite with results:
 */
static void brw_clip_project_position(struct brw_clip_compile *c, struct brw_reg pos )
{
   struct brw_compile *p = &c->func;

   /* calc rhw 
    */
   brw_math_invert(p, get_element(pos, W), get_element(pos, W));

   /* value.xyz *= value.rhw
    */
   brw_set_access_mode(p, BRW_ALIGN_16);
   brw_MUL(p, brw_writemask(pos, WRITEMASK_XYZ), pos, brw_swizzle1(pos, W));
   brw_set_access_mode(p, BRW_ALIGN_1);
}


static void brw_clip_project_vertex( struct brw_clip_compile *c, 
				     struct brw_indirect vert_addr )
{
   struct brw_compile *p = &c->func;
   struct brw_reg tmp = get_tmp(c);

   /* Fixup position.  Extract from the original vertex and re-project
    * to screen space:
    */
   brw_MOV(p, tmp, deref_4f(vert_addr, c->offset[VERT_RESULT_HPOS]));
   brw_clip_project_position(c, tmp);
   brw_MOV(p, deref_4f(vert_addr, c->header_position_offset), tmp);
	 
   release_tmp(c, tmp);
}




/* Interpolate between two vertices and put the result into a0.0.  
 * Increment a0.0 accordingly.
 */
void brw_clip_interp_vertex( struct brw_clip_compile *c,
			     struct brw_indirect dest_ptr,
			     struct brw_indirect v0_ptr, /* from */
			     struct brw_indirect v1_ptr, /* to */
			     struct brw_reg t0,
			     GLboolean force_edgeflag)
{
   struct brw_compile *p = &c->func;
   struct brw_reg tmp = get_tmp(c);
   GLuint i;

   /* Just copy the vertex header:
    */
   brw_copy_indirect_to_indirect(p, dest_ptr, v0_ptr, 1);
      
   /* Iterate over each attribute (could be done in pairs?)
    */
   for (i = 0; i < c->nr_attrs; i++) {
      GLuint delta = i*16 + 32;

      if (delta == c->offset[VERT_RESULT_EDGE]) {
	 if (force_edgeflag) 
	    brw_MOV(p, deref_4f(dest_ptr, delta), brw_imm_f(1));
	 else
	    brw_MOV(p, deref_4f(dest_ptr, delta), deref_4f(v0_ptr, delta));
      }
      else {
	 /* Interpolate: 
	  *
	  *        New = attr0 + t*attr1 - t*attr0
	  */
	 brw_MUL(p, 
		 vec4(brw_null_reg()),
		 deref_4f(v1_ptr, delta),
		 t0);

	 brw_MAC(p, 
		 tmp,	      
		 negate(deref_4f(v0_ptr, delta)),
		 t0); 
	      
	 brw_ADD(p,
		 deref_4f(dest_ptr, delta), 
		 deref_4f(v0_ptr, delta),
		 tmp);
      }
   }

   if (i & 1) {
      GLuint delta = i*16 + 32;
      brw_MOV(p, deref_4f(dest_ptr, delta), brw_imm_f(0));
   }

   release_tmp(c, tmp);

   /* Recreate the projected (NDC) coordinate in the new vertex
    * header:
    */
   brw_clip_project_vertex(c, dest_ptr );
}




#define MAX_MRF 16

void brw_clip_emit_vue(struct brw_clip_compile *c, 
		       struct brw_indirect vert,
		       GLboolean allocate,
		       GLboolean eot,
		       GLuint header)
{
   struct brw_compile *p = &c->func;
   GLuint start = c->last_mrf;

   assert(!(allocate && eot));
   
   /* Cycle through mrf regs - probably futile as we have to wait for
    * the allocation response anyway.  Also, the order this function
    * is invoked doesn't correspond to the order the instructions will
    * be executed, so it won't have any effect in many cases.
    */
#if 0
   if (start + c->nr_regs + 1 >= MAX_MRF)
      start = 0;

   c->last_mrf = start + c->nr_regs + 1;
#endif
	
   /* Copy the vertex from vertn into m1..mN+1:
    */
   brw_copy_from_indirect(p, brw_message_reg(start+1), vert, c->nr_regs);

   /* Overwrite PrimType and PrimStart in the message header, for
    * each vertex in turn:
    */
   brw_MOV(p, get_element_ud(c->reg.R0, 2), brw_imm_ud(header));


   /* Send each vertex as a seperate write to the urb.  This
    * is different to the concept in brw_sf_emit.c, where
    * subsequent writes are used to build up a single urb
    * entry.  Each of these writes instantiates a seperate
    * urb entry - (I think... what about 'allocate'?)
    */
   brw_urb_WRITE(p, 
		 allocate ? c->reg.R0 : retype(brw_null_reg(), BRW_REGISTER_TYPE_UD),
		 start,
		 c->reg.R0,
		 allocate,
		 1,		/* used */
		 c->nr_regs + 1, /* msg length */
		 allocate ? 1 : 0, /* response_length */ 
		 eot,		/* eot */
		 1,		/* writes_complete */
		 0,		/* urb offset */
		 BRW_URB_SWIZZLE_NONE);
}



void brw_clip_kill_thread(struct brw_clip_compile *c)
{
   struct brw_compile *p = &c->func;

   /* Send an empty message to kill the thread and release any
    * allocated urb entry:
    */
   brw_urb_WRITE(p, 
		 retype(brw_null_reg(), BRW_REGISTER_TYPE_UD),
		 0,
		 c->reg.R0,
		 0,		/* allocate */
		 0,		/* used */
		 0, 		/* msg len */
		 0, 		/* response len */
		 1, 		/* eot */
		 1,		/* writes complete */
		 0,
		 BRW_URB_SWIZZLE_NONE);
}



struct brw_reg brw_clip_plane0_address( struct brw_clip_compile *c )
{
   return brw_address(c->reg.fixed_planes);
}


struct brw_reg brw_clip_plane_stride( struct brw_clip_compile *c )
{
   if (c->key.nr_userclip) {
      return brw_imm_uw(16);
   }
   else {
      return brw_imm_uw(4);
   }
}


/* If flatshading, distribute color from provoking vertex prior to
 * clipping.
 */
void brw_clip_copy_colors( struct brw_clip_compile *c,
			   GLuint to, GLuint from )
{
   struct brw_compile *p = &c->func;

   if (c->offset[VERT_RESULT_COL0])
      brw_MOV(p, 
	      byte_offset(c->reg.vertex[to], c->offset[VERT_RESULT_COL0]),
	      byte_offset(c->reg.vertex[from], c->offset[VERT_RESULT_COL0]));

   if (c->offset[VERT_RESULT_COL1])
      brw_MOV(p, 
	      byte_offset(c->reg.vertex[to], c->offset[VERT_RESULT_COL1]),
	      byte_offset(c->reg.vertex[from], c->offset[VERT_RESULT_COL1]));

   if (c->offset[VERT_RESULT_BFC0])
      brw_MOV(p, 
	      byte_offset(c->reg.vertex[to], c->offset[VERT_RESULT_BFC0]),
	      byte_offset(c->reg.vertex[from], c->offset[VERT_RESULT_BFC0]));

   if (c->offset[VERT_RESULT_BFC1])
      brw_MOV(p, 
	      byte_offset(c->reg.vertex[to], c->offset[VERT_RESULT_BFC1]),
	      byte_offset(c->reg.vertex[from], c->offset[VERT_RESULT_BFC1]));
}



void brw_clip_init_clipmask( struct brw_clip_compile *c )
{
   struct brw_compile *p = &c->func;
   struct brw_reg incoming = get_element_ud(c->reg.R0, 2);
   
   /* Shift so that lowest outcode bit is rightmost: 
    */
   brw_MOV(p, c->reg.planemask, incoming);
   brw_SHR(p, c->reg.planemask, c->reg.planemask, brw_imm_ud(26));

   if (c->key.nr_userclip) {
      struct brw_reg tmp = retype(vec1(get_tmp(c)), BRW_REGISTER_TYPE_UD);

      /* Rearrange userclip outcodes so that they come directly after
       * the fixed plane bits.
       */
      brw_AND(p, tmp, incoming, brw_imm_ud(0x3f<<14));
      brw_SHR(p, tmp, tmp, brw_imm_ud(8));
      brw_OR(p, c->reg.planemask, c->reg.planemask, tmp);
      
      release_tmp(c, tmp);
   }

   /* Test for -ve rhw workaround 
    */
   brw_set_conditionalmod(p, BRW_CONDITIONAL_NZ);
   brw_AND(p, vec1(brw_null_reg()), incoming, brw_imm_ud(1<<20));
   brw_OR(p, c->reg.planemask, c->reg.planemask, brw_imm_ud(0x3f));
   brw_set_predicate_control(p, BRW_PREDICATE_NONE);

}

