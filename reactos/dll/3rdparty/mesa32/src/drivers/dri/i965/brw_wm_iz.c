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
                

#include "mtypes.h"
#include "brw_wm.h"


#undef P			/* prompted depth */
#undef C			/* computed */
#undef N			/* non-promoted? */

#define P 0
#define C 1
#define N 2

const struct {
   GLuint mode:2;
   GLuint sd_present:1;
   GLuint sd_to_rt:1;
   GLuint dd_present:1;
   GLuint ds_present:1;
} wm_iz_table[IZ_BIT_MAX] =
{
 { P, 0, 0, 0, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { C, 0, 1, 0, 0 }, 
 { C, 0, 1, 0, 0 }, 
 { C, 1, 1, 0, 0 }, 
 { C, 1, 1, 0, 0 }, 
 { C, 0, 1, 0, 0 }, 
 { C, 0, 1, 0, 0 }, 
 { C, 1, 1, 1, 0 }, 
 { C, 1, 1, 1, 0 }, 
 { C, 0, 1, 1, 0 }, 
 { C, 0, 1, 1, 0 }, 
 { C, 1, 1, 1, 0 }, 
 { C, 1, 1, 1, 0 }, 
 { C, 0, 1, 1, 0 }, 
 { C, 0, 1, 1, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { C, 0, 1, 0, 0 }, 
 { C, 0, 1, 0, 0 }, 
 { C, 1, 1, 0, 0 }, 
 { C, 1, 1, 0, 0 }, 
 { C, 0, 1, 0, 0 }, 
 { C, 0, 1, 0, 0 }, 
 { C, 1, 1, 1, 0 }, 
 { C, 1, 1, 1, 0 }, 
 { C, 0, 1, 1, 0 }, 
 { C, 0, 1, 1, 0 }, 
 { C, 1, 1, 1, 0 }, 
 { C, 1, 1, 1, 0 }, 
 { C, 0, 1, 1, 0 }, 
 { C, 0, 1, 1, 0 }, 
 { C, 0, 0, 0, 1 }, 
 { C, 0, 0, 0, 1 }, 
 { C, 0, 1, 0, 1 }, 
 { C, 0, 1, 0, 1 }, 
 { C, 1, 1, 0, 1 }, 
 { C, 1, 1, 0, 1 }, 
 { C, 0, 1, 0, 1 }, 
 { C, 0, 1, 0, 1 }, 
 { C, 1, 1, 1, 1 }, 
 { C, 1, 1, 1, 1 }, 
 { C, 0, 1, 1, 1 }, 
 { C, 0, 1, 1, 1 }, 
 { C, 1, 1, 1, 1 }, 
 { C, 1, 1, 1, 1 }, 
 { C, 0, 1, 1, 1 }, 
 { C, 0, 1, 1, 1 }, 
 { C, 0, 0, 0, 1 }, 
 { C, 0, 0, 0, 1 }, 
 { C, 0, 1, 0, 1 }, 
 { C, 0, 1, 0, 1 }, 
 { C, 1, 1, 0, 1 }, 
 { C, 1, 1, 0, 1 }, 
 { C, 0, 1, 0, 1 }, 
 { C, 0, 1, 0, 1 }, 
 { C, 1, 1, 1, 1 }, 
 { C, 1, 1, 1, 1 }, 
 { C, 0, 1, 1, 1 }, 
 { C, 0, 1, 1, 1 }, 
 { C, 1, 1, 1, 1 }, 
 { C, 1, 1, 1, 1 }, 
 { C, 0, 1, 1, 1 }, 
 { C, 0, 1, 1, 1 }, 
 { P, 0, 0, 0, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { N, 1, 1, 0, 0 }, 
 { N, 0, 1, 0, 0 }, 
 { N, 0, 1, 0, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { C, 0, 1, 1, 0 }, 
 { C, 0, 1, 1, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { N, 1, 1, 0, 0 }, 
 { C, 0, 1, 1, 0 }, 
 { C, 0, 1, 1, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { N, 1, 1, 0, 0 }, 
 { N, 0, 1, 0, 0 }, 
 { N, 0, 1, 0, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { C, 0, 1, 1, 0 }, 
 { C, 0, 1, 1, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { N, 1, 1, 0, 0 }, 
 { C, 0, 1, 1, 0 }, 
 { C, 0, 1, 1, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { N, 1, 1, 0, 1 }, 
 { N, 0, 1, 0, 1 }, 
 { N, 0, 1, 0, 1 }, 
 { P, 0, 0, 0, 0 }, 
 { P, 0, 0, 0, 0 }, 
 { C, 0, 1, 1, 1 }, 
 { C, 0, 1, 1, 1 }, 
 { P, 0, 0, 0, 0 }, 
 { N, 1, 1, 0, 1 }, 
 { C, 0, 1, 1, 1 }, 
 { C, 0, 1, 1, 1 }, 
 { P, 0, 0, 0, 0 }, 
 { C, 0, 0, 0, 1 }, 
 { P, 0, 0, 0, 0 }, 
 { C, 0, 1, 0, 1 }, 
 { P, 0, 0, 0, 0 }, 
 { C, 1, 1, 0, 1 }, 
 { C, 0, 1, 0, 1 }, 
 { C, 0, 1, 0, 1 }, 
 { P, 0, 0, 0, 0 }, 
 { C, 1, 1, 1, 1 }, 
 { C, 0, 1, 1, 1 }, 
 { C, 0, 1, 1, 1 }, 
 { P, 0, 0, 0, 0 }, 
 { C, 1, 1, 1, 1 }, 
 { C, 0, 1, 1, 1 }, 
 { C, 0, 1, 1, 1 } 
};

void brw_wm_lookup_iz( GLuint line_aa,
		       GLuint lookup,
		       struct brw_wm_prog_key *key )
{
   GLuint reg = 2;

   assert (lookup < IZ_BIT_MAX);
      
   if (lookup & IZ_PS_COMPUTES_DEPTH_BIT)
      key->computes_depth = 1;

   if (wm_iz_table[lookup].sd_present) {
      key->source_depth_reg = reg;
      reg += 2;
   }

   if (wm_iz_table[lookup].sd_to_rt)
      key->source_depth_to_render_target = 1;

   if (wm_iz_table[lookup].ds_present || line_aa != AA_NEVER) {
      key->aa_dest_stencil_reg = reg;
      key->runtime_check_aads_emit = (!wm_iz_table[lookup].ds_present &&
				      line_aa == AA_SOMETIMES);
      reg++;
   }

   if (wm_iz_table[lookup].dd_present) {
      key->dest_depth_reg = reg;
      reg+=2;
   }

   key->nr_depth_regs = (reg+1)/2;
}

