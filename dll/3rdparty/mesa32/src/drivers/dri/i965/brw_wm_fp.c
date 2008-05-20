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
#include "brw_context.h"
#include "brw_wm.h"
#include "brw_util.h"

#include "shader/prog_parameter.h"
#include "shader/prog_print.h"
#include "shader/prog_statevars.h"


#define FIRST_INTERNAL_TEMP MAX_NV_FRAGMENT_PROGRAM_TEMPS

#define X    0
#define Y    1
#define Z    2
#define W    3


static const char *wm_opcode_strings[] = {   
   "PIXELXY",
   "DELTAXY",
   "PIXELW",
   "LINTERP",
   "PINTERP",
   "CINTERP",
   "WPOSXY",
   "FB_WRITE"
};

#if 0
static const char *wm_file_strings[] = {   
   "PAYLOAD"
};
#endif


/***********************************************************************
 * Source regs
 */

static struct prog_src_register src_reg(GLuint file, GLuint idx)
{
   struct prog_src_register reg;
   reg.File = file;
   reg.Index = idx;
   reg.Swizzle = SWIZZLE_NOOP;
   reg.RelAddr = 0;
   reg.NegateBase = 0;
   reg.Abs = 0;
   reg.NegateAbs = 0;
   return reg;
}

static struct prog_src_register src_reg_from_dst(struct prog_dst_register dst)
{
   return src_reg(dst.File, dst.Index);
}

static struct prog_src_register src_undef( void )
{
   return src_reg(PROGRAM_UNDEFINED, 0);
}

static GLboolean src_is_undef(struct prog_src_register src)
{
   return src.File == PROGRAM_UNDEFINED;
}

static struct prog_src_register src_swizzle( struct prog_src_register reg, int x, int y, int z, int w )
{
   reg.Swizzle = MAKE_SWIZZLE4(x,y,z,w);
   return reg;
}

static struct prog_src_register src_swizzle1( struct prog_src_register reg, int x )
{
   return src_swizzle(reg, x, x, x, x);
}


/***********************************************************************
 * Dest regs
 */

static struct prog_dst_register dst_reg(GLuint file, GLuint idx)
{
   struct prog_dst_register reg;
   reg.File = file;
   reg.Index = idx;
   reg.WriteMask = WRITEMASK_XYZW;
   reg.CondMask = 0;
   reg.CondSwizzle = 0;
   reg.pad = 0;
   reg.CondSrc = 0;
   return reg;
}

static struct prog_dst_register dst_mask( struct prog_dst_register reg, int mask )
{
   reg.WriteMask &= mask;
   return reg;
}

static struct prog_dst_register dst_undef( void )
{
   return dst_reg(PROGRAM_UNDEFINED, 0);
}



static struct prog_dst_register get_temp( struct brw_wm_compile *c )
{
   int bit = ffs( ~c->fp_temp );

   if (!bit) {
      _mesa_printf("%s: out of temporaries\n", __FILE__);
      exit(1);
   }

   c->fp_temp |= 1<<(bit-1);
   return dst_reg(PROGRAM_TEMPORARY, FIRST_INTERNAL_TEMP+(bit-1));
}


static void release_temp( struct brw_wm_compile *c, struct prog_dst_register temp )
{
   c->fp_temp &= ~1<<(temp.Index + 1 - FIRST_INTERNAL_TEMP);
}


/***********************************************************************
 * Instructions 
 */

static struct prog_instruction *get_fp_inst(struct brw_wm_compile *c)
{
   return &c->prog_instructions[c->nr_fp_insns++];
}

static struct prog_instruction *emit_insn(struct brw_wm_compile *c,
					const struct prog_instruction *inst0)
{
   struct prog_instruction *inst = get_fp_inst(c);
   *inst = *inst0;
   return inst;
}

static struct prog_instruction * emit_op(struct brw_wm_compile *c,
				       GLuint op,
				       struct prog_dst_register dest,
				       GLuint saturate,
				       GLuint tex_src_unit,
				       GLuint tex_src_target,
				       struct prog_src_register src0,
				       struct prog_src_register src1,
				       struct prog_src_register src2 )
{
   struct prog_instruction *inst = get_fp_inst(c);
      
   memset(inst, 0, sizeof(*inst));

   inst->Opcode = op;
   inst->DstReg = dest;
   inst->SaturateMode = saturate;   
   inst->TexSrcUnit = tex_src_unit;
   inst->TexSrcTarget = tex_src_target;
   inst->SrcReg[0] = src0;
   inst->SrcReg[1] = src1;
   inst->SrcReg[2] = src2;
   
   return inst;
}
   



/***********************************************************************
 * Special instructions for interpolation and other tasks
 */

static struct prog_src_register get_pixel_xy( struct brw_wm_compile *c )
{
   if (src_is_undef(c->pixel_xy)) {
      struct prog_dst_register pixel_xy = get_temp(c);
      struct prog_src_register payload_r0_depth = src_reg(PROGRAM_PAYLOAD, PAYLOAD_DEPTH);
      
      
      /* Emit the out calculations, and hold onto the results.  Use
       * two instructions as a temporary is required.
       */   
      /* pixel_xy.xy = PIXELXY payload[0];
       */
      emit_op(c,
	      WM_PIXELXY,
	      dst_mask(pixel_xy, WRITEMASK_XY),
	      0, 0, 0,
	      payload_r0_depth,
	      src_undef(),
	      src_undef());

      c->pixel_xy = src_reg_from_dst(pixel_xy);
   }

   return c->pixel_xy;
}

static struct prog_src_register get_delta_xy( struct brw_wm_compile *c )
{
   if (src_is_undef(c->delta_xy)) {
      struct prog_dst_register delta_xy = get_temp(c);
      struct prog_src_register pixel_xy = get_pixel_xy(c);
      struct prog_src_register payload_r0_depth = src_reg(PROGRAM_PAYLOAD, PAYLOAD_DEPTH);
      
      /* deltas.xy = DELTAXY pixel_xy, payload[0]
       */
      emit_op(c,
	      WM_DELTAXY,
	      dst_mask(delta_xy, WRITEMASK_XY),
	      0, 0, 0,
	      pixel_xy, 
	      payload_r0_depth,
	      src_undef());
      
      c->delta_xy = src_reg_from_dst(delta_xy);
   }

   return c->delta_xy;
}

static struct prog_src_register get_pixel_w( struct brw_wm_compile *c )
{
   if (src_is_undef(c->pixel_w)) {
      struct prog_dst_register pixel_w = get_temp(c);
      struct prog_src_register deltas = get_delta_xy(c);
      struct prog_src_register interp_wpos = src_reg(PROGRAM_PAYLOAD, FRAG_ATTRIB_WPOS);
      
      
      /* deltas.xyw = DELTAS2 deltas.xy, payload.interp_wpos.x
       */
      emit_op(c,
	      WM_PIXELW,
	      dst_mask(pixel_w, WRITEMASK_W),
	      0, 0, 0,
	      interp_wpos,
	      deltas, 
	      src_undef());
      

      c->pixel_w = src_reg_from_dst(pixel_w);
   }

   return c->pixel_w;
}

static void emit_interp( struct brw_wm_compile *c,
			 GLuint idx )
{
   struct prog_dst_register dst = dst_reg(PROGRAM_INPUT, idx);
   struct prog_src_register interp = src_reg(PROGRAM_PAYLOAD, idx);
   struct prog_src_register deltas = get_delta_xy(c);
   struct prog_src_register arg2;
   GLuint opcode;
   
   /* Need to use PINTERP on attributes which have been
    * multiplied by 1/W in the SF program, and LINTERP on those
    * which have not:
    */
   switch (idx) {
   case FRAG_ATTRIB_WPOS:
      opcode = WM_LINTERP;
      arg2 = src_undef();

      /* Have to treat wpos.xy specially:
       */
      emit_op(c,
	      WM_WPOSXY,
	      dst_mask(dst, WRITEMASK_XY),
	      0, 0, 0,
	      get_pixel_xy(c),
	      src_undef(),
	      src_undef());
      
      dst = dst_mask(dst, WRITEMASK_ZW);

      /* PROGRAM_INPUT.attr.xyzw = INTERP payload.interp[attr].x, deltas.xyw
       */
      emit_op(c,
	      WM_LINTERP,
	      dst,
	      0, 0, 0,
	      interp,
	      deltas,
	      arg2);
      break;
   case FRAG_ATTRIB_COL0:
   case FRAG_ATTRIB_COL1:
      if (c->key.flat_shade) {
	 emit_op(c,
		 WM_CINTERP,
		 dst,
		 0, 0, 0,
		 interp,
		 src_undef(),
		 src_undef());
      }
      else {
	 emit_op(c,
		 WM_LINTERP,
		 dst,
		 0, 0, 0,
		 interp,
		 deltas,
		 src_undef());
      }
      break;
   default:
      emit_op(c,
	      WM_PINTERP,
	      dst,
	      0, 0, 0,
	      interp,
	      deltas,
	      get_pixel_w(c));
      break;
   }

   c->fp_interp_emitted |= 1<<idx;
}


/***********************************************************************
 * Hacks to extend the program parameter and constant lists.
 */

/* Add the fog parameters to the parameter list of the original
 * program, rather than creating a new list.  Doesn't really do any
 * harm and it's not as if the parameter handling isn't a big hack
 * anyway.
 */
static struct prog_src_register search_or_add_param5(struct brw_wm_compile *c, 
                                                     GLint s0,
                                                     GLint s1,
                                                     GLint s2,
                                                     GLint s3,
                                                     GLint s4)
{
   struct gl_program_parameter_list *paramList = c->fp->program.Base.Parameters;
   gl_state_index tokens[STATE_LENGTH];
   GLuint idx;
   tokens[0] = s0;
   tokens[1] = s1;
   tokens[2] = s2;
   tokens[3] = s3;
   tokens[4] = s4;
   
   for (idx = 0; idx < paramList->NumParameters; idx++) {
      if (paramList->Parameters[idx].Type == PROGRAM_STATE_VAR &&
	  memcmp(paramList->Parameters[idx].StateIndexes, tokens, sizeof(tokens)) == 0)
	 return src_reg(PROGRAM_STATE_VAR, idx);
   }

   idx = _mesa_add_state_reference( paramList, tokens );

   /* Recalculate state dependency: 
    */
   c->fp->param_state = paramList->StateFlags;

   return src_reg(PROGRAM_STATE_VAR, idx);
}


static struct prog_src_register search_or_add_const4f( struct brw_wm_compile *c, 
						     GLfloat s0,
						     GLfloat s1,
						     GLfloat s2,
						     GLfloat s3)
{
   struct gl_program_parameter_list *paramList = c->fp->program.Base.Parameters;
   GLfloat values[4];
   GLuint idx;
   GLuint swizzle;

   values[0] = s0;
   values[1] = s1;
   values[2] = s2;
   values[3] = s3;

   /* Have to search, otherwise multiple compilations will each grow
    * the parameter list.
    */
   for (idx = 0; idx < paramList->NumParameters; idx++) {
      if (paramList->Parameters[idx].Type == PROGRAM_CONSTANT &&
	  memcmp(paramList->ParameterValues[idx], values, sizeof(values)) == 0)

	 /* XXX: this mimics the mesa bug which puts all constants and
	  * parameters into the "PROGRAM_STATE_VAR" category:
	  */
	 return src_reg(PROGRAM_STATE_VAR, idx);
   }
   
   idx = _mesa_add_unnamed_constant( paramList, values, 4, &swizzle );
   /* XXX what about swizzle? */
   return src_reg(PROGRAM_STATE_VAR, idx);
}



/***********************************************************************
 * Expand various instructions here to simpler forms.  
 */
static void precalc_dst( struct brw_wm_compile *c,
			       const struct prog_instruction *inst )
{
   struct prog_src_register src0 = inst->SrcReg[0];
   struct prog_src_register src1 = inst->SrcReg[1];
   struct prog_dst_register dst = inst->DstReg;
   
   if (dst.WriteMask & WRITEMASK_Y) {      
      /* dst.y = mul src0.y, src1.y
       */
      emit_op(c,
	      OPCODE_MUL,
	      dst_mask(dst, WRITEMASK_Y),
	      inst->SaturateMode, 0, 0,
	      src0,
	      src1,
	      src_undef());
   }


   if (dst.WriteMask & WRITEMASK_XZ) {
      GLuint z = GET_SWZ(src0.Swizzle, Z);

      /* dst.xz = swz src0.1zzz
       */
      emit_op(c,
	      OPCODE_SWZ,
	      dst_mask(dst, WRITEMASK_XZ),
	      inst->SaturateMode, 0, 0,
	      src_swizzle(src0, SWIZZLE_ONE, z, z, z),
	      src_undef(),
	      src_undef());
   }
   if (dst.WriteMask & WRITEMASK_W) {
      /* dst.w = mov src1.w
       */
      emit_op(c,
	      OPCODE_MOV,
	      dst_mask(dst, WRITEMASK_W),
	      inst->SaturateMode, 0, 0,
	      src1,
	      src_undef(),
	      src_undef());
   }
}


static void precalc_lit( struct brw_wm_compile *c,
			 const struct prog_instruction *inst )
{
   struct prog_src_register src0 = inst->SrcReg[0];
   struct prog_dst_register dst = inst->DstReg;
   
   if (dst.WriteMask & WRITEMASK_XW) {
      /* dst.xw = swz src0.1111
       */
      emit_op(c,
	      OPCODE_SWZ,
	      dst_mask(dst, WRITEMASK_XW),
	      0, 0, 0,
	      src_swizzle1(src0, SWIZZLE_ONE),
	      src_undef(),
	      src_undef());
   }


   if (dst.WriteMask & WRITEMASK_YZ) {
      emit_op(c,
	      OPCODE_LIT,
	      dst_mask(dst, WRITEMASK_YZ),
	      inst->SaturateMode, 0, 0,
	      src0,
	      src_undef(),
	      src_undef());
   }
}

static void precalc_tex( struct brw_wm_compile *c,
			 const struct prog_instruction *inst )
{
   struct prog_src_register coord;
   struct prog_dst_register tmpcoord;

   if (inst->TexSrcTarget == TEXTURE_CUBE_INDEX) {
       struct prog_instruction *out;
       struct prog_dst_register tmp0 = get_temp(c);
       struct prog_src_register tmp0src = src_reg_from_dst(tmp0);
       struct prog_dst_register tmp1 = get_temp(c);
       struct prog_src_register tmp1src = src_reg_from_dst(tmp1);
       struct prog_src_register src0 = inst->SrcReg[0];

       tmpcoord = get_temp(c);
       coord = src_reg_from_dst(tmpcoord);

       out = emit_op(c, OPCODE_MOV,
                     tmpcoord,
                     0, 0, 0,
                     src0,
                     src_undef(),
                     src_undef());
       out->SrcReg[0].NegateBase = 0;
       out->SrcReg[0].Abs = 1;

       emit_op(c, OPCODE_MAX,
               tmp0,
               0, 0, 0,
               src_swizzle1(coord, X),
               src_swizzle1(coord, Y),
               src_undef());

       emit_op(c, OPCODE_MAX,
               tmp1,
               0, 0, 0,
               tmp0src,
               src_swizzle1(coord, Z),
               src_undef());

       emit_op(c, OPCODE_RCP,
               tmp0,
               0, 0, 0,
               tmp1src,
               src_undef(),
               src_undef());

       emit_op(c, OPCODE_MUL,
               tmpcoord,
               0, 0, 0,
               src0,
               tmp0src,
               src_undef());

       release_temp(c, tmp0);
       release_temp(c, tmp1);
   } else if (inst->TexSrcTarget == TEXTURE_RECT_INDEX) {
      struct prog_src_register scale = 
	 search_or_add_param5( c, 
			       STATE_INTERNAL, 
			       STATE_TEXRECT_SCALE,
			       inst->TexSrcUnit,
			       0,0 );

      tmpcoord = get_temp(c);

      /* coord.xy   = MUL inst->SrcReg[0], { 1/width, 1/height }
       */
      emit_op(c,
	      OPCODE_MUL,
	      tmpcoord,
	      0, 0, 0,
	      inst->SrcReg[0],
	      scale,
	      src_undef());

      coord = src_reg_from_dst(tmpcoord);
   }
   else {
      coord = inst->SrcReg[0];
   }

   /* Need to emit YUV texture conversions by hand.  Probably need to
    * do this here - the alternative is in brw_wm_emit.c, but the
    * conversion requires allocating a temporary variable which we
    * don't have the facility to do that late in the compilation.
    */
   if (!(c->key.yuvtex_mask & (1<<inst->TexSrcUnit))) {
      emit_op(c, 
	      OPCODE_TEX,
	      inst->DstReg,
	      inst->SaturateMode,
	      inst->TexSrcUnit,
	      inst->TexSrcTarget,
	      coord,
	      src_undef(),
	      src_undef());
   }
   else {
      /* 
	 CONST C0 = { -.5, -.0625,  -.5, 1.164 }
	 CONST C1 = { 1.596, -0.813, 2.018, -.391 }
	 UYV     = TEX ...
	 UYV.xyz = ADD UYV,     C0
	 UYV.y   = MUL UYV.y,   C0.w
	 RGB.xyz = MAD UYV.xxz, C1,   UYV.y
	 RGB.y   = MAD UYV.z,   C1.w, RGB.y
      */
      struct prog_dst_register dst = inst->DstReg;
      struct prog_src_register src0 = inst->SrcReg[0];
      struct prog_dst_register tmp = get_temp(c);
      struct prog_src_register tmpsrc = src_reg_from_dst(tmp);
      struct prog_src_register C0 = search_or_add_const4f( c,  -.5, -.0625, -.5, 1.164 );
      struct prog_src_register C1 = search_or_add_const4f( c, 1.596, -0.813, 2.018, -.391 );
     
      /* tmp     = TEX ...
       */
      emit_op(c, 
	      OPCODE_TEX,
	      tmp,
	      inst->SaturateMode,
	      inst->TexSrcUnit,
	      inst->TexSrcTarget,
	      src0,
	      src_undef(),
	      src_undef());

      /* tmp.xyz =  ADD TMP, C0
       */
      emit_op(c,
	      OPCODE_ADD,
	      dst_mask(tmp, WRITEMASK_XYZ),
	      0, 0, 0,
	      tmpsrc,
	      C0,
	      src_undef());

      /* YUV.y   = MUL YUV.y, C0.w
       */
      emit_op(c,
	      OPCODE_MUL,
	      dst_mask(tmp, WRITEMASK_Y),
	      0, 0, 0,
	      tmpsrc,
	      src_swizzle1(C0, W),
	      src_undef());

      /* RGB.xyz = MAD YUV.xxz, C1, YUV.y
       */
      emit_op(c,
	      OPCODE_MAD,
	      dst_mask(dst, WRITEMASK_XYZ),
	      0, 0, 0,
	      src_swizzle(tmpsrc, X,X,Z,Z),
	      C1,
	      src_swizzle1(tmpsrc, Y));

      /*  RGB.y   = MAD YUV.z, C1.w, RGB.y
       */
      emit_op(c,
	      OPCODE_MAD,
	      dst_mask(dst, WRITEMASK_Y),
	      0, 0, 0,
	      src_swizzle1(tmpsrc, Z),
	      src_swizzle1(C1, W),
	      src_swizzle1(src_reg_from_dst(dst), Y));

      release_temp(c, tmp);
   }

   if (inst->TexSrcTarget == GL_TEXTURE_RECTANGLE_NV) 
      release_temp(c, tmpcoord);
}


static GLboolean projtex( struct brw_wm_compile *c,
			  const struct prog_instruction *inst )
{
   struct prog_src_register src = inst->SrcReg[0];

   /* Only try to detect the simplest cases.  Could detect (later)
    * cases where we are trying to emit code like RCP {1.0}, MUL x,
    * {1.0}, and so on.
    *
    * More complex cases than this typically only arise from
    * user-provided fragment programs anyway:
    */
   if (inst->TexSrcTarget == TEXTURE_CUBE_INDEX)
      return 0;  /* ut2004 gun rendering !?! */
   else if (src.File == PROGRAM_INPUT && 
	    GET_SWZ(src.Swizzle, W) == W &&
	    (c->key.projtex_mask & (1<<src.Index)) == 0)
      return 0;
   else
      return 1;
}


static void precalc_txp( struct brw_wm_compile *c,
			       const struct prog_instruction *inst )
{
   struct prog_src_register src0 = inst->SrcReg[0];

   if (projtex(c, inst)) {
      struct prog_dst_register tmp = get_temp(c);
      struct prog_instruction tmp_inst;

      /* tmp0.w = RCP inst.arg[0][3]
       */
      emit_op(c,
	      OPCODE_RCP,
	      dst_mask(tmp, WRITEMASK_W),
	      0, 0, 0,
	      src_swizzle1(src0, GET_SWZ(src0.Swizzle, W)),
	      src_undef(),
	      src_undef());

      /* tmp0.xyz =  MUL inst.arg[0], tmp0.wwww
       */
      emit_op(c,
	      OPCODE_MUL,
	      dst_mask(tmp, WRITEMASK_XYZ),
	      0, 0, 0,
	      src0,
	      src_swizzle1(src_reg_from_dst(tmp), W),
	      src_undef());

      /* dst = precalc(TEX tmp0)
       */
      tmp_inst = *inst;
      tmp_inst.SrcReg[0] = src_reg_from_dst(tmp);
      precalc_tex(c, &tmp_inst);

      release_temp(c, tmp);
   }
   else
   {
      /* dst = precalc(TEX src0)
       */
      precalc_tex(c, inst);
   }
}





/***********************************************************************
 * Add instructions to perform fog blending
 */

static void fog_blend( struct brw_wm_compile *c,
			     struct prog_src_register fog_factor )
{
   struct prog_dst_register outcolor = dst_reg(PROGRAM_OUTPUT, FRAG_RESULT_COLR);
   struct prog_src_register fogcolor = search_or_add_param5( c, STATE_FOG_COLOR, 0,0,0,0 );

   /* color.xyz = LRP fog_factor.xxxx, output_color, fog_color */
   
   emit_op(c, 
	   OPCODE_LRP,
	   dst_mask(outcolor, WRITEMASK_XYZ),
	   0, 0, 0,
	   fog_factor,
	   src_reg_from_dst(outcolor),
	   fogcolor);
}



/* This one is simple - just take the interpolated fog coordinate and
 * use it as the fog blend factor.
 */
static void fog_interpolated( struct brw_wm_compile *c )
{
   struct prog_src_register fogc = src_reg(PROGRAM_INPUT, FRAG_ATTRIB_FOGC);
   
   if (!(c->fp_interp_emitted & (1<<FRAG_ATTRIB_FOGC))) 
      emit_interp(c, FRAG_ATTRIB_FOGC);

   fog_blend( c, src_swizzle1(fogc, GET_SWZ(fogc.Swizzle,X)));
}

static void emit_fog( struct brw_wm_compile *c ) 
{
   if (!c->fp->program.FogOption)
      return;

   if (1) 
      fog_interpolated( c );
   else {
      /* TODO: per-pixel fog */
      assert(0);
   }
}

static void emit_fb_write( struct brw_wm_compile *c )
{
   struct prog_src_register outcolor = src_reg(PROGRAM_OUTPUT, FRAG_RESULT_COLR);
   struct prog_src_register payload_r0_depth = src_reg(PROGRAM_PAYLOAD, PAYLOAD_DEPTH);
   struct prog_src_register outdepth = src_reg(PROGRAM_OUTPUT, FRAG_RESULT_DEPR);

   emit_op(c,
	   WM_FB_WRITE,
	   dst_mask(dst_undef(),0),
	   0, 0, 0,
	   outcolor,
	   payload_r0_depth,
	   outdepth);
}




/***********************************************************************
 * Emit INTERP instructions ahead of first use of each attrib.
 */

static void validate_src_regs( struct brw_wm_compile *c,
			       const struct prog_instruction *inst )
{
   GLuint nr_args = brw_wm_nr_args( inst->Opcode );
   GLuint i;

   for (i = 0; i < nr_args; i++) {
      if (inst->SrcReg[i].File == PROGRAM_INPUT) {
	 GLuint idx = inst->SrcReg[i].Index;
	 if (!(c->fp_interp_emitted & (1<<idx))) {
	    emit_interp(c, idx);
	 }
      }
   }
}
	 


static void print_insns( const struct prog_instruction *insn,
			 GLuint nr )
{
   GLuint i;
   for (i = 0; i < nr; i++, insn++) {
      _mesa_printf("%3d: ", i);
      if (insn->Opcode < MAX_OPCODE)
	 _mesa_print_instruction(insn);
      else if (insn->Opcode < MAX_WM_OPCODE) {
	 GLuint idx = insn->Opcode - MAX_OPCODE;

	 _mesa_print_alu_instruction(insn,
				     wm_opcode_strings[idx],
				     3);
      }
      else 
	 _mesa_printf("UNKNOWN\n");
	   
   }
}

void brw_wm_pass_fp( struct brw_wm_compile *c )
{
   struct brw_fragment_program *fp = c->fp;
   GLuint insn;

   if (INTEL_DEBUG & DEBUG_WM) {
      _mesa_printf("\n\n\npre-fp:\n");
      _mesa_print_program(&fp->program.Base); 
      _mesa_printf("\n");
   }

   c->pixel_xy = src_undef();
   c->delta_xy = src_undef();
   c->pixel_w = src_undef();
   c->nr_fp_insns = 0;

   /* Emit preamble instructions:
    */


   for (insn = 0; insn < fp->program.Base.NumInstructions; insn++) {
      const struct prog_instruction *inst = &fp->program.Base.Instructions[insn];
      struct prog_instruction *out;

      /* Check for INPUT values, emit INTERP instructions where
       * necessary:
       */
      validate_src_regs(c, inst);


      switch (inst->Opcode) {
      case OPCODE_SWZ: 
	 out = emit_insn(c, inst);
	 out->Opcode = OPCODE_MOV;
	 break;
	 
      case OPCODE_ABS:
	 out = emit_insn(c, inst);
	 out->Opcode = OPCODE_MOV;
	 out->SrcReg[0].NegateBase = 0;
	 out->SrcReg[0].Abs = 1;
	 break;

      case OPCODE_SUB: 
	 out = emit_insn(c, inst);
	 out->Opcode = OPCODE_ADD;
	 out->SrcReg[1].NegateBase ^= 0xf;
	 break;

      case OPCODE_SCS: 
	 out = emit_insn(c, inst);
	 /* This should probably be done in the parser. 
	  */
	 out->DstReg.WriteMask &= WRITEMASK_XY;
	 break;
	 
      case OPCODE_DST:
	 precalc_dst(c, inst);
	 break;

      case OPCODE_LIT:
	 precalc_lit(c, inst);
	 break;
     
      case OPCODE_TXP:
	 precalc_txp(c, inst);
	 break;

      case OPCODE_XPD: 
	 out = emit_insn(c, inst);
	 /* This should probably be done in the parser. 
	  */
	 out->DstReg.WriteMask &= WRITEMASK_XYZ;
	 break;

      case OPCODE_KIL: 
	 out = emit_insn(c, inst);
	 /* This should probably be done in the parser. 
	  */
	 out->DstReg.WriteMask = 0;
	 break;

      case OPCODE_END:
      case OPCODE_PRINT:
	 break;
	 
      default:
	 emit_insn(c, inst);
	 break;
      }
   }
   
   emit_fog(c);
   emit_fb_write(c);


   if (INTEL_DEBUG & DEBUG_WM) {
      _mesa_printf("\n\n\npass_fp:\n");
      print_insns( c->prog_instructions, c->nr_fp_insns );
      _mesa_printf("\n");
   }
}

