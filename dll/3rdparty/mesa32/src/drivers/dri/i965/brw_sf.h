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
   

#ifndef BRW_SF_H
#define BRW_SF_H


#include "brw_context.h"
#include "brw_eu.h"
#include "program.h"


#define SF_POINTS    0
#define SF_LINES     1
#define SF_TRIANGLES 2
#define SF_UNFILLED_TRIS   3

struct brw_sf_prog_key {
   GLuint primitive:2;
   GLuint do_twoside_color:1;
   GLuint do_flat_shading:1;
   GLuint attrs:16;
   GLuint frontface_ccw:1;
   GLuint pad:11;
};


struct brw_sf_compile {
   struct brw_compile func;
   struct brw_sf_prog_key key;
   struct brw_sf_prog_data prog_data;
   
   struct brw_reg pv;
   struct brw_reg det;
   struct brw_reg dx0;
   struct brw_reg dx2;
   struct brw_reg dy0;
   struct brw_reg dy2;

   /* z and 1/w passed in seperately:
    */
   struct brw_reg z[3];
   struct brw_reg inv_w[3];
   
   /* The vertices:
    */
   struct brw_reg vert[3];

    /* Temporaries, allocated after last vertex reg.
    */
   struct brw_reg inv_det;
   struct brw_reg a1_sub_a0;
   struct brw_reg a2_sub_a0;
   struct brw_reg tmp;

   struct brw_reg m1Cx;
   struct brw_reg m2Cy;
   struct brw_reg m3C0;

   GLuint nr_verts;
   GLuint nr_attrs;
   GLuint nr_attr_regs;
   GLuint nr_setup_attrs;
   GLuint nr_setup_regs;

   GLubyte attr_to_idx[VERT_RESULT_MAX];   
   GLubyte idx_to_attr[VERT_RESULT_MAX];   
};

 
void brw_emit_tri_setup( struct brw_sf_compile *c );
void brw_emit_line_setup( struct brw_sf_compile *c );
void brw_emit_point_setup( struct brw_sf_compile *c );
void brw_emit_anyprim_setup( struct brw_sf_compile *c );

#endif
