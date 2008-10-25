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

#ifndef BRW_CLIP_H
#define BRW_CLIP_H


#include "brw_context.h"
#include "brw_eu.h"

#define MAX_VERTS (3+6+6)	

/* Note that if unfilled primitives are being emitted, we have to fix
 * up polygon offset and flatshading at this point:
 */
struct brw_clip_prog_key {
   GLuint attrs:16;		
   GLuint primitive:4;
   GLuint nr_userclip:3;
   GLuint do_flat_shading:1;
   GLuint do_unfilled:1;
   GLuint fill_cw:2;		/* includes cull information */
   GLuint fill_ccw:2;		/* includes cull information */
   GLuint offset_cw:1;
   GLuint offset_ccw:1;
   GLuint pad0:1;

   GLuint copy_bfc_cw:1;
   GLuint copy_bfc_ccw:1;
   GLuint clip_mode:3;
   GLuint pad1:27;
   
   GLfloat offset_factor;
   GLfloat offset_units;
};


#define CLIP_LINE   0
#define CLIP_POINT  1
#define CLIP_FILL   2
#define CLIP_CULL   3


#define PRIM_MASK  (0x1f)

struct brw_clip_compile {
   struct brw_compile func;
   struct brw_clip_prog_key key;
   struct brw_clip_prog_data prog_data;
   
   struct {
      struct brw_reg R0;
      struct brw_reg vertex[MAX_VERTS];

      struct brw_reg t;
      struct brw_reg t0, t1;
      struct brw_reg dp0, dp1;

      struct brw_reg dpPrev;
      struct brw_reg dp;
      struct brw_reg loopcount;
      struct brw_reg nr_verts;
      struct brw_reg planemask;

      struct brw_reg inlist;
      struct brw_reg outlist;
      struct brw_reg freelist;

      struct brw_reg dir;
      struct brw_reg tmp0, tmp1;
      struct brw_reg offset;
      
      struct brw_reg fixed_planes;
      struct brw_reg plane_equation;
   } reg;

   /* 3 different ways of expressing vertex size:
    */
   GLuint nr_attrs;
   GLuint nr_regs;
   GLuint nr_bytes;

   GLuint first_tmp;
   GLuint last_tmp;

   GLboolean need_direction;

   GLuint last_mrf;

   GLuint header_position_offset;
   GLuint offset[VERT_ATTRIB_MAX];
};

#define ATTR_SIZE  (4*4)

/* Points are only culled, so no need for a clip routine, however it
 * works out easier to have a dummy one.
 */
void brw_emit_unfilled_clip( struct brw_clip_compile *c );
void brw_emit_tri_clip( struct brw_clip_compile *c );
void brw_emit_line_clip( struct brw_clip_compile *c );
void brw_emit_point_clip( struct brw_clip_compile *c );

/* brw_clip_tri.c, for use by the unfilled clip routine:
 */
void brw_clip_tri_init_vertices( struct brw_clip_compile *c );
void brw_clip_tri_flat_shade( struct brw_clip_compile *c );
void brw_clip_tri( struct brw_clip_compile *c );
void brw_clip_tri_emit_polygon( struct brw_clip_compile *c );
void brw_clip_tri_alloc_regs( struct brw_clip_compile *c, 
			      GLuint nr_verts );


/* Utils:
 */

void brw_clip_interp_vertex( struct brw_clip_compile *c,
			     struct brw_indirect dest_ptr,
			     struct brw_indirect v0_ptr, /* from */
			     struct brw_indirect v1_ptr, /* to */
			     struct brw_reg t0,
			     GLboolean force_edgeflag );

void brw_clip_init_planes( struct brw_clip_compile *c );

void brw_clip_emit_vue(struct brw_clip_compile *c, 
		       struct brw_indirect vert,
		       GLboolean allocate,
		       GLboolean eot,
		       GLuint header);

void brw_clip_kill_thread(struct brw_clip_compile *c);

struct brw_reg brw_clip_plane_stride( struct brw_clip_compile *c );
struct brw_reg brw_clip_plane0_address( struct brw_clip_compile *c );

void brw_clip_copy_colors( struct brw_clip_compile *c,
			   GLuint to, GLuint from );

void brw_clip_init_clipmask( struct brw_clip_compile *c );

#endif
