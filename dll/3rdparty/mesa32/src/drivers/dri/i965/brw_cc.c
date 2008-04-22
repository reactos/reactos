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


#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_util.h"
#include "macros.h"
#include "enums.h"

static void upload_cc_vp( struct brw_context *brw )
{
   struct brw_cc_viewport ccv;

   memset(&ccv, 0, sizeof(ccv));

   ccv.min_depth = 0.0;
   ccv.max_depth = 1.0;

   brw->cc.vp_gs_offset = brw_cache_data( &brw->cache[BRW_CC_VP], &ccv );
}

const struct brw_tracked_state brw_cc_vp = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .update = upload_cc_vp
};


static void upload_cc_unit( struct brw_context *brw )
{
   struct brw_cc_unit_state cc;
   
   memset(&cc, 0, sizeof(cc));

   /* _NEW_STENCIL */
   if (brw->attribs.Stencil->Enabled) {
      cc.cc0.stencil_enable = brw->attribs.Stencil->Enabled;
      cc.cc0.stencil_func = intel_translate_compare_func(brw->attribs.Stencil->Function[0]);
      cc.cc0.stencil_fail_op = intel_translate_stencil_op(brw->attribs.Stencil->FailFunc[0]);
      cc.cc0.stencil_pass_depth_fail_op = intel_translate_stencil_op(brw->attribs.Stencil->ZFailFunc[0]);
      cc.cc0.stencil_pass_depth_pass_op = intel_translate_stencil_op(brw->attribs.Stencil->ZPassFunc[0]);
      cc.cc1.stencil_ref = brw->attribs.Stencil->Ref[0];
      cc.cc1.stencil_write_mask = brw->attribs.Stencil->WriteMask[0];
      cc.cc1.stencil_test_mask = brw->attribs.Stencil->ValueMask[0];

      if (brw->attribs.Stencil->TestTwoSide) {
	 cc.cc0.bf_stencil_enable = brw->attribs.Stencil->TestTwoSide;
	 cc.cc0.bf_stencil_func = intel_translate_compare_func(brw->attribs.Stencil->Function[1]);
	 cc.cc0.bf_stencil_fail_op = intel_translate_stencil_op(brw->attribs.Stencil->FailFunc[1]);
	 cc.cc0.bf_stencil_pass_depth_fail_op = intel_translate_stencil_op(brw->attribs.Stencil->ZFailFunc[1]);
	 cc.cc0.bf_stencil_pass_depth_pass_op = intel_translate_stencil_op(brw->attribs.Stencil->ZPassFunc[1]);
	 cc.cc1.bf_stencil_ref = brw->attribs.Stencil->Ref[1];
	 cc.cc2.bf_stencil_write_mask = brw->attribs.Stencil->WriteMask[1];
	 cc.cc2.bf_stencil_test_mask = brw->attribs.Stencil->ValueMask[1];
      }

      /* Not really sure about this:
       */
      if (brw->attribs.Stencil->WriteMask[0] ||
	  (brw->attribs.Stencil->TestTwoSide && brw->attribs.Stencil->WriteMask[1]))
	 cc.cc0.stencil_write_enable = 1;
   }

   /* _NEW_COLOR */
   if (brw->attribs.Color->_LogicOpEnabled) {
      cc.cc2.logicop_enable = 1;
      cc.cc5.logicop_func = intel_translate_logic_op( brw->attribs.Color->LogicOp );
   }
   else if (brw->attribs.Color->BlendEnabled) {
      GLenum eqRGB = brw->attribs.Color->BlendEquationRGB;
      GLenum eqA = brw->attribs.Color->BlendEquationA;
      GLenum srcRGB = brw->attribs.Color->BlendSrcRGB;
      GLenum dstRGB = brw->attribs.Color->BlendDstRGB;
      GLenum srcA = brw->attribs.Color->BlendSrcA;
      GLenum dstA = brw->attribs.Color->BlendDstA;

      if (eqRGB == GL_MIN || eqRGB == GL_MAX) {
	 srcRGB = dstRGB = GL_ONE;
      }

      if (eqA == GL_MIN || eqA == GL_MAX) {
	 srcA = dstA = GL_ONE;
      }

      cc.cc6.dest_blend_factor = brw_translate_blend_factor(dstRGB); 
      cc.cc6.src_blend_factor = brw_translate_blend_factor(srcRGB); 
      cc.cc6.blend_function = brw_translate_blend_equation( eqRGB );

      cc.cc5.ia_dest_blend_factor = brw_translate_blend_factor(dstA); 
      cc.cc5.ia_src_blend_factor = brw_translate_blend_factor(srcA); 
      cc.cc5.ia_blend_function = brw_translate_blend_equation( eqA );

      cc.cc3.blend_enable = 1;
      cc.cc3.ia_blend_enable = (srcA != srcRGB || 
				dstA != dstRGB || 
				eqA != eqRGB);
   }

   if (brw->attribs.Color->AlphaEnabled) {
      cc.cc3.alpha_test = 1;
      cc.cc3.alpha_test_func = intel_translate_compare_func(brw->attribs.Color->AlphaFunc);

      UNCLAMPED_FLOAT_TO_UBYTE(cc.cc7.alpha_ref.ub[0], brw->attribs.Color->AlphaRef);

      cc.cc3.alpha_test_format = BRW_ALPHATEST_FORMAT_UNORM8;
   }

   if (brw->attribs.Color->DitherFlag) {
      cc.cc5.dither_enable = 1;
      cc.cc6.y_dither_offset = 0; 
      cc.cc6.x_dither_offset = 0;     
   }

   /* _NEW_DEPTH */
   if (brw->attribs.Depth->Test) {
      cc.cc2.depth_test = brw->attribs.Depth->Test;
      cc.cc2.depth_test_function = intel_translate_compare_func(brw->attribs.Depth->Func);
      cc.cc2.depth_write_enable = brw->attribs.Depth->Mask;
   }
 
   /* CACHE_NEW_CC_VP */
   cc.cc4.cc_viewport_state_offset =  brw->cc.vp_gs_offset >> 5;
 
   if (INTEL_DEBUG & DEBUG_STATS)
      cc.cc5.statistics_enable = 1; 

   brw->cc.state_gs_offset = brw_cache_data( &brw->cache[BRW_CC_UNIT], &cc );
}

const struct brw_tracked_state brw_cc_unit = {
   .dirty = {
      .mesa = _NEW_STENCIL | _NEW_COLOR | _NEW_DEPTH,
      .brw = 0,
      .cache = CACHE_NEW_CC_VP
   },
   .update = upload_cc_unit
};



