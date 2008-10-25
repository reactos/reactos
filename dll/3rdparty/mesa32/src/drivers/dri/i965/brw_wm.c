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
#include "brw_util.h"
#include "brw_wm.h"
#include "brw_state.h"
#include "brw_hal.h"


GLuint brw_wm_nr_args( GLuint opcode )
{
   switch (opcode) {

   case WM_PIXELXY:
   case OPCODE_ABS:
   case OPCODE_FLR:
   case OPCODE_FRC:
   case OPCODE_SWZ:
   case OPCODE_MOV:
   case OPCODE_COS:
   case OPCODE_EX2:
   case OPCODE_LG2:
   case OPCODE_RCP:
   case OPCODE_RSQ:
   case OPCODE_SIN:
   case OPCODE_SCS:
   case OPCODE_TEX:
   case OPCODE_TXB:
   case OPCODE_TXP:	
   case OPCODE_KIL:
   case OPCODE_LIT: 
   case WM_CINTERP: 
   case WM_WPOSXY: 
      return 1;

   case OPCODE_POW:
   case OPCODE_SUB:
   case OPCODE_SGE:
   case OPCODE_SLT:
   case OPCODE_ADD:
   case OPCODE_MAX:
   case OPCODE_MIN:
   case OPCODE_MUL:
   case OPCODE_XPD:
   case OPCODE_DP3:	
   case OPCODE_DP4:
   case OPCODE_DPH:
   case OPCODE_DST:
   case WM_LINTERP: 
   case WM_DELTAXY:
   case WM_PIXELW:
      return 2;

   case WM_FB_WRITE:
   case WM_PINTERP: 
   case OPCODE_MAD:	
   case OPCODE_CMP:
   case OPCODE_LRP:
      return 3;
      
   default:
      return 0;
   }
}


GLuint brw_wm_is_scalar_result( GLuint opcode )
{
   switch (opcode) {
   case OPCODE_COS:
   case OPCODE_EX2:
   case OPCODE_LG2:
   case OPCODE_POW:
   case OPCODE_RCP:
   case OPCODE_RSQ:
   case OPCODE_SIN:
   case OPCODE_DP3:
   case OPCODE_DP4:
   case OPCODE_DPH:
   case OPCODE_DST:
      return 1;
      
   default:
      return 0;
   }
}


static void brw_wm_pass_hal (struct brw_wm_compile *c)
{
   static void (*hal_wm_pass) (struct brw_wm_compile *c);
   static GLboolean hal_tried;
   
   if (!hal_tried)
   {
      hal_wm_pass = brw_hal_find_symbol ("intel_hal_wm_pass");
      hal_tried = 1;
   }
   if (hal_wm_pass)
      (*hal_wm_pass) (c);
}

static void do_wm_prog( struct brw_context *brw,
			struct brw_fragment_program *fp, 
			struct brw_wm_prog_key *key)
{
   struct brw_wm_compile *c;
   const GLuint *program;
   GLuint program_size;

   c = brw->wm.compile_data;
   if (c == NULL) {
     brw->wm.compile_data = calloc(1, sizeof(*brw->wm.compile_data));
     c = brw->wm.compile_data;
   } else {
     memset(c, 0, sizeof(*brw->wm.compile_data));
   }
   memcpy(&c->key, key, sizeof(*key));

   c->fp = fp;
   c->env_param = brw->intel.ctx.FragmentProgram.Parameters;


   /* Augment fragment program.  Add instructions for pre- and
    * post-fragment-program tasks such as interpolation and fogging.
    */
   brw_wm_pass_fp(c);
   
   /* Translate to intermediate representation.  Build register usage
    * chains.
    */
   brw_wm_pass0(c);

   /* Dead code removal.
    */
   brw_wm_pass1(c);

   /* Hal optimization
    */
   brw_wm_pass_hal (c);
   
   /* Register allocation.
    */
   c->grf_limit = BRW_WM_MAX_GRF/2;

   /* This is where we start emitting gen4 code:
    */
   brw_init_compile(&c->func);    

   brw_wm_pass2(c);

   c->prog_data.total_grf = c->max_wm_grf;
   if (c->last_scratch) {
      c->prog_data.total_scratch =
	 c->last_scratch + 0x40;
   } else {
      c->prog_data.total_scratch = 0;
   }

   /* Emit GEN4 code.
    */
   brw_wm_emit(c);

   /* get the program
    */
   program = brw_get_program(&c->func, &program_size);

   /*
    */
   brw->wm.prog_gs_offset = brw_upload_cache( &brw->cache[BRW_WM_PROG],
					      &c->key,
					      sizeof(c->key),
					      program,
					      program_size,
					      &c->prog_data,
					      &brw->wm.prog_data );
}



static void brw_wm_populate_key( struct brw_context *brw,
				 struct brw_wm_prog_key *key )
{
   /* BRW_NEW_FRAGMENT_PROGRAM */
   struct brw_fragment_program *fp = 
      (struct brw_fragment_program *)brw->fragment_program;
   GLuint lookup = 0;
   GLuint line_aa;
   GLuint i;

   memset(key, 0, sizeof(*key));

   /* Build the index for table lookup
    */
   /* _NEW_COLOR */
   if (fp->program.UsesKill ||
       brw->attribs.Color->AlphaEnabled)
      lookup |= IZ_PS_KILL_ALPHATEST_BIT;

   if (fp->program.Base.OutputsWritten & (1<<FRAG_RESULT_DEPR))
      lookup |= IZ_PS_COMPUTES_DEPTH_BIT;

   /* _NEW_DEPTH */
   if (brw->attribs.Depth->Test)
      lookup |= IZ_DEPTH_TEST_ENABLE_BIT;

   if (brw->attribs.Depth->Test &&  
       brw->attribs.Depth->Mask) /* ?? */
      lookup |= IZ_DEPTH_WRITE_ENABLE_BIT;

   /* _NEW_STENCIL */
   if (brw->attribs.Stencil->Enabled) {
      lookup |= IZ_STENCIL_TEST_ENABLE_BIT;

      if (brw->attribs.Stencil->WriteMask[0] ||
	  (brw->attribs.Stencil->TestTwoSide && brw->attribs.Stencil->WriteMask[1]))
	 lookup |= IZ_STENCIL_WRITE_ENABLE_BIT;
   }

   /* XXX: when should this be disabled?
    */
   if (1)
      lookup |= IZ_EARLY_DEPTH_TEST_BIT;

   
   line_aa = AA_NEVER;

   /* _NEW_LINE, _NEW_POLYGON, BRW_NEW_REDUCED_PRIMITIVE */
   if (brw->attribs.Line->SmoothFlag) {
      if (brw->intel.reduced_primitive == GL_LINES) {
	 line_aa = AA_ALWAYS;
      }
      else if (brw->intel.reduced_primitive == GL_TRIANGLES) {
	 if (brw->attribs.Polygon->FrontMode == GL_LINE) {
	    line_aa = AA_SOMETIMES;

	    if (brw->attribs.Polygon->BackMode == GL_LINE ||
		(brw->attribs.Polygon->CullFlag &&
		 brw->attribs.Polygon->CullFaceMode == GL_BACK))
	       line_aa = AA_ALWAYS;
	 }
	 else if (brw->attribs.Polygon->BackMode == GL_LINE) {
	    line_aa = AA_SOMETIMES;

	    if ((brw->attribs.Polygon->CullFlag &&
		 brw->attribs.Polygon->CullFaceMode == GL_FRONT))
	       line_aa = AA_ALWAYS;
	 }
      }
   }
	 
   brw_wm_lookup_iz(line_aa,
		    lookup,
		    key);


   /* BRW_NEW_WM_INPUT_DIMENSIONS */
   key->projtex_mask = brw->wm.input_size_masks[4-1]; 

   /* _NEW_LIGHT */
   key->flat_shade = (brw->attribs.Light->ShadeModel == GL_FLAT);

   /* _NEW_TEXTURE */
   for (i = 0; i < BRW_MAX_TEX_UNIT; i++) {
      const struct gl_texture_unit *unit = &brw->attribs.Texture->Unit[i];
      const struct gl_texture_object *t = unit->_Current;

      if (unit->_ReallyEnabled) {

	 if (t->CompareMode == GL_COMPARE_R_TO_TEXTURE_ARB &&
	     t->Image[0][t->BaseLevel]->_BaseFormat == GL_DEPTH_COMPONENT) {
	    key->shadowtex_mask |= 1<<i;
	 }

	 if (t->Image[0][t->BaseLevel]->InternalFormat == GL_YCBCR_MESA)
	    key->yuvtex_mask |= 1<<i;
      }
   }
	  

   /* Extra info:
    */
   key->program_string_id = fp->id;

}


static void brw_upload_wm_prog( struct brw_context *brw )
{
   struct brw_wm_prog_key key;
   struct brw_fragment_program *fp = (struct brw_fragment_program *)
      brw->fragment_program;
     
   brw_wm_populate_key(brw, &key);

   /* Make an early check for the key.
    */
   if (brw_search_cache(&brw->cache[BRW_WM_PROG], 
			&key, sizeof(key),
			&brw->wm.prog_data,
			&brw->wm.prog_gs_offset))
      return;

   do_wm_prog(brw, fp, &key);
}


/* See brw_wm.c:
 */
const struct brw_tracked_state brw_wm_prog = {
   .dirty = {
      .mesa  = (_NEW_COLOR |
		_NEW_DEPTH |
		_NEW_STENCIL |
		_NEW_POLYGON |
		_NEW_LINE |
		_NEW_LIGHT |
		_NEW_TEXTURE),
      .brw   = (BRW_NEW_FRAGMENT_PROGRAM |
		BRW_NEW_WM_INPUT_DIMENSIONS |
		BRW_NEW_REDUCED_PRIMITIVE),
      .cache = 0
   },
   .update = brw_upload_wm_prog
};

