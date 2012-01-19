/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/**
 * \file ffvertex_prog.c
 *
 * Create a vertex program to execute the current fixed function T&L pipeline.
 * \author Keith Whitwell
 */


#include "main/glheader.h"
#include "main/mtypes.h"
#include "main/macros.h"
#include "main/enums.h"
#include "main/ffvertex_prog.h"
#include "shader/program.h"
#include "shader/prog_cache.h"
#include "shader/prog_instruction.h"
#include "shader/prog_parameter.h"
#include "shader/prog_print.h"
#include "shader/prog_statevars.h"


struct state_key {
   unsigned light_global_enabled:1;
   unsigned light_local_viewer:1;
   unsigned light_twoside:1;
   unsigned light_color_material:1;
   unsigned light_color_material_mask:12;
   unsigned light_material_mask:12;
   unsigned material_shininess_is_zero:1;

   unsigned need_eye_coords:1;
   unsigned normalize:1;
   unsigned rescale_normals:1;
   unsigned fog_source_is_depth:1;
   unsigned tnl_do_vertex_fog:1;
   unsigned separate_specular:1;
   unsigned fog_mode:2;
   unsigned point_attenuated:1;
   unsigned point_array:1;
   unsigned texture_enabled_global:1;
   unsigned fragprog_inputs_read:12;

   struct {
      unsigned light_enabled:1;
      unsigned light_eyepos3_is_zero:1;
      unsigned light_spotcutoff_is_180:1;
      unsigned light_attenuated:1;      
      unsigned texunit_really_enabled:1;
      unsigned texmat_enabled:1;
      unsigned texgen_enabled:4;
      unsigned texgen_mode0:4;
      unsigned texgen_mode1:4;
      unsigned texgen_mode2:4;
      unsigned texgen_mode3:4;
   } unit[8];
};



#define FOG_NONE   0
#define FOG_LINEAR 1
#define FOG_EXP    2
#define FOG_EXP2   3

static GLuint translate_fog_mode( GLenum mode )
{
   switch (mode) {
   case GL_LINEAR: return FOG_LINEAR;
   case GL_EXP: return FOG_EXP;
   case GL_EXP2: return FOG_EXP2;
   default: return FOG_NONE;
   }
}


#define TXG_NONE           0
#define TXG_OBJ_LINEAR     1
#define TXG_EYE_LINEAR     2
#define TXG_SPHERE_MAP     3
#define TXG_REFLECTION_MAP 4
#define TXG_NORMAL_MAP     5

static GLuint translate_texgen( GLboolean enabled, GLenum mode )
{
   if (!enabled)
      return TXG_NONE;

   switch (mode) {
   case GL_OBJECT_LINEAR: return TXG_OBJ_LINEAR;
   case GL_EYE_LINEAR: return TXG_EYE_LINEAR;
   case GL_SPHERE_MAP: return TXG_SPHERE_MAP;
   case GL_REFLECTION_MAP_NV: return TXG_REFLECTION_MAP;
   case GL_NORMAL_MAP_NV: return TXG_NORMAL_MAP;
   default: return TXG_NONE;
   }
}


/**
 * Returns bitmask of flags indicating which materials are set per-vertex
 * in the current VB.
 * XXX get these from the VBO...
 */
static GLbitfield
tnl_get_per_vertex_materials(GLcontext *ctx)
{
   GLbitfield mask = 0x0;
#if 0
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;

   for (i = _TNL_FIRST_MAT; i <= _TNL_LAST_MAT; i++) 
      if (VB->AttribPtr[i] && VB->AttribPtr[i]->stride) 
         mask |= 1 << (i - _TNL_FIRST_MAT);
#endif
   return mask;
}


/**
 * Should fog be computed per-vertex?
 */
static GLboolean
tnl_get_per_vertex_fog(GLcontext *ctx)
{
#if 0
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   return tnl->_DoVertexFog;
#else
   return GL_FALSE;
#endif
}


static GLboolean check_active_shininess( GLcontext *ctx,
                                         const struct state_key *key,
                                         GLuint side )
{
   GLuint bit = 1 << (MAT_ATTRIB_FRONT_SHININESS + side);

   if (key->light_color_material_mask & bit)
      return GL_TRUE;

   if (key->light_material_mask & bit)
      return GL_TRUE;

   if (ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_SHININESS + side][0] != 0.0F)
      return GL_TRUE;

   return GL_FALSE;
}


static void make_state_key( GLcontext *ctx, struct state_key *key )
{
   const struct gl_fragment_program *fp;
   GLuint i;

   memset(key, 0, sizeof(struct state_key));
   fp = ctx->FragmentProgram._Current;

   /* This now relies on texenvprogram.c being active:
    */
   assert(fp);

   key->need_eye_coords = ctx->_NeedEyeCoords;

   key->fragprog_inputs_read = fp->Base.InputsRead;

   if (ctx->RenderMode == GL_FEEDBACK) {
      /* make sure the vertprog emits color and tex0 */
      key->fragprog_inputs_read |= (FRAG_BIT_COL0 | FRAG_BIT_TEX0);
   }

   key->separate_specular = (ctx->Light.Model.ColorControl ==
			     GL_SEPARATE_SPECULAR_COLOR);

   if (ctx->Light.Enabled) {
      key->light_global_enabled = 1;

      if (ctx->Light.Model.LocalViewer)
	 key->light_local_viewer = 1;

      if (ctx->Light.Model.TwoSide)
	 key->light_twoside = 1;

      if (ctx->Light.ColorMaterialEnabled) {
	 key->light_color_material = 1;
	 key->light_color_material_mask = ctx->Light.ColorMaterialBitmask;
      }

      key->light_material_mask = tnl_get_per_vertex_materials(ctx);

      for (i = 0; i < MAX_LIGHTS; i++) {
	 struct gl_light *light = &ctx->Light.Light[i];

	 if (light->Enabled) {
	    key->unit[i].light_enabled = 1;

	    if (light->EyePosition[3] == 0.0)
	       key->unit[i].light_eyepos3_is_zero = 1;
	    
	    if (light->SpotCutoff == 180.0)
	       key->unit[i].light_spotcutoff_is_180 = 1;

	    if (light->ConstantAttenuation != 1.0 ||
		light->LinearAttenuation != 0.0 ||
		light->QuadraticAttenuation != 0.0)
	       key->unit[i].light_attenuated = 1;
	 }
      }

      if (check_active_shininess(ctx, key, 0)) {
         key->material_shininess_is_zero = 0;
      }
      else if (key->light_twoside &&
               check_active_shininess(ctx, key, 1)) {
         key->material_shininess_is_zero = 0;
      }
      else {
         key->material_shininess_is_zero = 1;
      }
   }

   if (ctx->Transform.Normalize)
      key->normalize = 1;

   if (ctx->Transform.RescaleNormals)
      key->rescale_normals = 1;

   key->fog_mode = translate_fog_mode(fp->FogOption);
   
   if (ctx->Fog.FogCoordinateSource == GL_FRAGMENT_DEPTH_EXT)
      key->fog_source_is_depth = 1;
   
   key->tnl_do_vertex_fog = tnl_get_per_vertex_fog(ctx);

   if (ctx->Point._Attenuated)
      key->point_attenuated = 1;

#if FEATURE_point_size_array
   if (ctx->Array.ArrayObj->PointSize.Enabled)
      key->point_array = 1;
#endif

   if (ctx->Texture._TexGenEnabled ||
       ctx->Texture._TexMatEnabled ||
       ctx->Texture._EnabledUnits)
      key->texture_enabled_global = 1;
      
   for (i = 0; i < MAX_TEXTURE_COORD_UNITS; i++) {
      struct gl_texture_unit *texUnit = &ctx->Texture.Unit[i];

      if (texUnit->_ReallyEnabled)
	 key->unit[i].texunit_really_enabled = 1;

      if (ctx->Texture._TexMatEnabled & ENABLE_TEXMAT(i))      
	 key->unit[i].texmat_enabled = 1;
      
      if (texUnit->TexGenEnabled) {
	 key->unit[i].texgen_enabled = 1;
      
	 key->unit[i].texgen_mode0 = 
	    translate_texgen( texUnit->TexGenEnabled & (1<<0),
			      texUnit->GenModeS );
	 key->unit[i].texgen_mode1 = 
	    translate_texgen( texUnit->TexGenEnabled & (1<<1),
			      texUnit->GenModeT );
	 key->unit[i].texgen_mode2 = 
	    translate_texgen( texUnit->TexGenEnabled & (1<<2),
			      texUnit->GenModeR );
	 key->unit[i].texgen_mode3 = 
	    translate_texgen( texUnit->TexGenEnabled & (1<<3),
			      texUnit->GenModeQ );
      }
   }
}


   
/* Very useful debugging tool - produces annotated listing of
 * generated program with line/function references for each
 * instruction back into this file:
 */
#define DISASSEM 0

/* Should be tunable by the driver - do we want to do matrix
 * multiplications with DP4's or with MUL/MAD's?  SSE works better
 * with the latter, drivers may differ.
 */
#define PREFER_DP4 0


/* Use uregs to represent registers internally, translate to Mesa's
 * expected formats on emit.  
 *
 * NOTE: These are passed by value extensively in this file rather
 * than as usual by pointer reference.  If this disturbs you, try
 * remembering they are just 32bits in size.
 *
 * GCC is smart enough to deal with these dword-sized structures in
 * much the same way as if I had defined them as dwords and was using
 * macros to access and set the fields.  This is much nicer and easier
 * to evolve.
 */
struct ureg {
   GLuint file:4;
   GLint idx:9;      /* relative addressing may be negative */
                     /* sizeof(idx) should == sizeof(prog_src_reg::Index) */
   GLuint negate:1;
   GLuint swz:12;
   GLuint pad:6;
};


struct tnl_program {
   const struct state_key *state;
   struct gl_vertex_program *program;
   GLint max_inst;  /** number of instructions allocated for program */
   
   GLuint temp_in_use;
   GLuint temp_reserved;
   
   struct ureg eye_position;
   struct ureg eye_position_z;
   struct ureg eye_position_normalized;
   struct ureg transformed_normal;
   struct ureg identity;

   GLuint materials;
   GLuint color_materials;
};


static const struct ureg undef = { 
   PROGRAM_UNDEFINED,
   0,
   0,
   0,
   0
};

/* Local shorthand:
 */
#define X    SWIZZLE_X
#define Y    SWIZZLE_Y
#define Z    SWIZZLE_Z
#define W    SWIZZLE_W


/* Construct a ureg:
 */
static struct ureg make_ureg(GLuint file, GLint idx)
{
   struct ureg reg;
   reg.file = file;
   reg.idx = idx;
   reg.negate = 0;
   reg.swz = SWIZZLE_NOOP;
   reg.pad = 0;
   return reg;
}



static struct ureg negate( struct ureg reg )
{
   reg.negate ^= 1;
   return reg;
} 


static struct ureg swizzle( struct ureg reg, int x, int y, int z, int w )
{
   reg.swz = MAKE_SWIZZLE4(GET_SWZ(reg.swz, x),
			   GET_SWZ(reg.swz, y),
			   GET_SWZ(reg.swz, z),
			   GET_SWZ(reg.swz, w));

   return reg;
}


static struct ureg swizzle1( struct ureg reg, int x )
{
   return swizzle(reg, x, x, x, x);
}


static struct ureg get_temp( struct tnl_program *p )
{
   int bit = _mesa_ffs( ~p->temp_in_use );
   if (!bit) {
      _mesa_problem(NULL, "%s: out of temporaries\n", __FILE__);
      _mesa_exit(1);
   }

   if ((GLuint) bit > p->program->Base.NumTemporaries)
      p->program->Base.NumTemporaries = bit;

   p->temp_in_use |= 1<<(bit-1);
   return make_ureg(PROGRAM_TEMPORARY, bit-1);
}


static struct ureg reserve_temp( struct tnl_program *p )
{
   struct ureg temp = get_temp( p );
   p->temp_reserved |= 1<<temp.idx;
   return temp;
}


static void release_temp( struct tnl_program *p, struct ureg reg )
{
   if (reg.file == PROGRAM_TEMPORARY) {
      p->temp_in_use &= ~(1<<reg.idx);
      p->temp_in_use |= p->temp_reserved; /* can't release reserved temps */
   }
}


static void release_temps( struct tnl_program *p )
{
   p->temp_in_use = p->temp_reserved;
}


/**
 * \param input  one of VERT_ATTRIB_x tokens.
 */
static struct ureg register_input( struct tnl_program *p, GLuint input )
{
   p->program->Base.InputsRead |= (1<<input);
   return make_ureg(PROGRAM_INPUT, input);
}


/**
 * \param input  one of VERT_RESULT_x tokens.
 */
static struct ureg register_output( struct tnl_program *p, GLuint output )
{
   p->program->Base.OutputsWritten |= (1<<output);
   return make_ureg(PROGRAM_OUTPUT, output);
}


static struct ureg register_const4f( struct tnl_program *p, 
			      GLfloat s0,
			      GLfloat s1,
			      GLfloat s2,
			      GLfloat s3)
{
   GLfloat values[4];
   GLint idx;
   GLuint swizzle;
   values[0] = s0;
   values[1] = s1;
   values[2] = s2;
   values[3] = s3;
   idx = _mesa_add_unnamed_constant( p->program->Base.Parameters, values, 4,
                                     &swizzle );
   ASSERT(swizzle == SWIZZLE_NOOP);
   return make_ureg(PROGRAM_CONSTANT, idx);
}


#define register_const1f(p, s0)         register_const4f(p, s0, 0, 0, 1)
#define register_scalar_const(p, s0)    register_const4f(p, s0, s0, s0, s0)
#define register_const2f(p, s0, s1)     register_const4f(p, s0, s1, 0, 1)
#define register_const3f(p, s0, s1, s2) register_const4f(p, s0, s1, s2, 1)

static GLboolean is_undef( struct ureg reg )
{
   return reg.file == PROGRAM_UNDEFINED;
}


static struct ureg get_identity_param( struct tnl_program *p )
{
   if (is_undef(p->identity)) 
      p->identity = register_const4f(p, 0,0,0,1);

   return p->identity;
}


static struct ureg register_param5(struct tnl_program *p, 
				   GLint s0,
				   GLint s1,
				   GLint s2,
				   GLint s3,
                                   GLint s4)
{
   gl_state_index tokens[STATE_LENGTH];
   GLint idx;
   tokens[0] = s0;
   tokens[1] = s1;
   tokens[2] = s2;
   tokens[3] = s3;
   tokens[4] = s4;
   idx = _mesa_add_state_reference( p->program->Base.Parameters, tokens );
   return make_ureg(PROGRAM_STATE_VAR, idx);
}


#define register_param1(p,s0)          register_param5(p,s0,0,0,0,0)
#define register_param2(p,s0,s1)       register_param5(p,s0,s1,0,0,0)
#define register_param3(p,s0,s1,s2)    register_param5(p,s0,s1,s2,0,0)
#define register_param4(p,s0,s1,s2,s3) register_param5(p,s0,s1,s2,s3,0)


static void register_matrix_param5( struct tnl_program *p,
				    GLint s0, /* modelview, projection, etc */
				    GLint s1, /* texture matrix number */
				    GLint s2, /* first row */
				    GLint s3, /* last row */
				    GLint s4, /* inverse, transpose, etc */
				    struct ureg *matrix )
{
   GLint i;

   /* This is a bit sad as the support is there to pull the whole
    * matrix out in one go:
    */
   for (i = 0; i <= s3 - s2; i++) 
      matrix[i] = register_param5( p, s0, s1, i, i, s4 );
}


static void emit_arg( struct prog_src_register *src,
		      struct ureg reg )
{
   src->File = reg.file;
   src->Index = reg.idx;
   src->Swizzle = reg.swz;
   src->NegateBase = reg.negate ? NEGATE_XYZW : 0;
   src->Abs = 0;
   src->NegateAbs = 0;
   src->RelAddr = 0;
   /* Check that bitfield sizes aren't exceeded */
   ASSERT(src->Index == reg.idx);
}


static void emit_dst( struct prog_dst_register *dst,
		      struct ureg reg, GLuint mask )
{
   dst->File = reg.file;
   dst->Index = reg.idx;
   /* allow zero as a shorthand for xyzw */
   dst->WriteMask = mask ? mask : WRITEMASK_XYZW; 
   dst->CondMask = COND_TR;  /* always pass cond test */
   dst->CondSwizzle = SWIZZLE_NOOP;
   dst->CondSrc = 0;
   dst->pad = 0;
   /* Check that bitfield sizes aren't exceeded */
   ASSERT(dst->Index == reg.idx);
}


static void debug_insn( struct prog_instruction *inst, const char *fn,
			GLuint line )
{
   if (DISASSEM) {
      static const char *last_fn;
   
      if (fn != last_fn) {
	 last_fn = fn;
	 _mesa_printf("%s:\n", fn);
      }
	 
      _mesa_printf("%d:\t", line);
      _mesa_print_instruction(inst);
   }
}


static void emit_op3fn(struct tnl_program *p,
                       enum prog_opcode op,
		       struct ureg dest,
		       GLuint mask,
		       struct ureg src0,
		       struct ureg src1,
		       struct ureg src2,
		       const char *fn,
		       GLuint line)
{
   GLuint nr;
   struct prog_instruction *inst;
      
   assert((GLint) p->program->Base.NumInstructions <= p->max_inst);

   if (p->program->Base.NumInstructions == p->max_inst) {
      /* need to extend the program's instruction array */
      struct prog_instruction *newInst;

      /* double the size */
      p->max_inst *= 2;

      newInst = _mesa_alloc_instructions(p->max_inst);
      if (!newInst) {
         _mesa_error(NULL, GL_OUT_OF_MEMORY, "vertex program build");
         return;
      }

      _mesa_copy_instructions(newInst,
                              p->program->Base.Instructions,
                              p->program->Base.NumInstructions);

      _mesa_free_instructions(p->program->Base.Instructions,
                              p->program->Base.NumInstructions);

      p->program->Base.Instructions = newInst;
   }
      
   nr = p->program->Base.NumInstructions++;

   inst = &p->program->Base.Instructions[nr];
   inst->Opcode = (enum prog_opcode) op; 
   inst->StringPos = 0;
   inst->Data = 0;
   
   emit_arg( &inst->SrcReg[0], src0 );
   emit_arg( &inst->SrcReg[1], src1 );
   emit_arg( &inst->SrcReg[2], src2 );   

   emit_dst( &inst->DstReg, dest, mask );

   debug_insn(inst, fn, line);
}


#define emit_op3(p, op, dst, mask, src0, src1, src2) \
   emit_op3fn(p, op, dst, mask, src0, src1, src2, __FUNCTION__, __LINE__)

#define emit_op2(p, op, dst, mask, src0, src1) \
    emit_op3fn(p, op, dst, mask, src0, src1, undef, __FUNCTION__, __LINE__)

#define emit_op1(p, op, dst, mask, src0) \
    emit_op3fn(p, op, dst, mask, src0, undef, undef, __FUNCTION__, __LINE__)


static struct ureg make_temp( struct tnl_program *p, struct ureg reg )
{
   if (reg.file == PROGRAM_TEMPORARY && 
       !(p->temp_reserved & (1<<reg.idx)))
      return reg;
   else {
      struct ureg temp = get_temp(p);
      emit_op1(p, OPCODE_MOV, temp, 0, reg);
      return temp;
   }
}


/* Currently no tracking performed of input/output/register size or
 * active elements.  Could be used to reduce these operations, as
 * could the matrix type.
 */
static void emit_matrix_transform_vec4( struct tnl_program *p,
					struct ureg dest,
					const struct ureg *mat,
					struct ureg src)
{
   emit_op2(p, OPCODE_DP4, dest, WRITEMASK_X, src, mat[0]);
   emit_op2(p, OPCODE_DP4, dest, WRITEMASK_Y, src, mat[1]);
   emit_op2(p, OPCODE_DP4, dest, WRITEMASK_Z, src, mat[2]);
   emit_op2(p, OPCODE_DP4, dest, WRITEMASK_W, src, mat[3]);
}


/* This version is much easier to implement if writemasks are not
 * supported natively on the target or (like SSE), the target doesn't
 * have a clean/obvious dotproduct implementation.
 */
static void emit_transpose_matrix_transform_vec4( struct tnl_program *p,
						  struct ureg dest,
						  const struct ureg *mat,
						  struct ureg src)
{
   struct ureg tmp;

   if (dest.file != PROGRAM_TEMPORARY)
      tmp = get_temp(p);
   else
      tmp = dest;

   emit_op2(p, OPCODE_MUL, tmp, 0, swizzle1(src,X), mat[0]);
   emit_op3(p, OPCODE_MAD, tmp, 0, swizzle1(src,Y), mat[1], tmp);
   emit_op3(p, OPCODE_MAD, tmp, 0, swizzle1(src,Z), mat[2], tmp);
   emit_op3(p, OPCODE_MAD, dest, 0, swizzle1(src,W), mat[3], tmp);

   if (dest.file != PROGRAM_TEMPORARY)
      release_temp(p, tmp);
}


static void emit_matrix_transform_vec3( struct tnl_program *p,
					struct ureg dest,
					const struct ureg *mat,
					struct ureg src)
{
   emit_op2(p, OPCODE_DP3, dest, WRITEMASK_X, src, mat[0]);
   emit_op2(p, OPCODE_DP3, dest, WRITEMASK_Y, src, mat[1]);
   emit_op2(p, OPCODE_DP3, dest, WRITEMASK_Z, src, mat[2]);
}


static void emit_normalize_vec3( struct tnl_program *p,
				 struct ureg dest,
				 struct ureg src )
{
#if 0
   /* XXX use this when drivers are ready for NRM3 */
   emit_op1(p, OPCODE_NRM3, dest, WRITEMASK_XYZ, src);
#else
   struct ureg tmp = get_temp(p);
   emit_op2(p, OPCODE_DP3, tmp, WRITEMASK_X, src, src);
   emit_op1(p, OPCODE_RSQ, tmp, WRITEMASK_X, tmp);
   emit_op2(p, OPCODE_MUL, dest, 0, src, swizzle1(tmp, X));
   release_temp(p, tmp);
#endif
}


static void emit_passthrough( struct tnl_program *p, 
			      GLuint input,
			      GLuint output )
{
   struct ureg out = register_output(p, output);
   emit_op1(p, OPCODE_MOV, out, 0, register_input(p, input)); 
}


static struct ureg get_eye_position( struct tnl_program *p )
{
   if (is_undef(p->eye_position)) {
      struct ureg pos = register_input( p, VERT_ATTRIB_POS ); 
      struct ureg modelview[4];

      p->eye_position = reserve_temp(p);

      if (PREFER_DP4) {
	 register_matrix_param5( p, STATE_MODELVIEW_MATRIX, 0, 0, 3,
                                 0, modelview );

	 emit_matrix_transform_vec4(p, p->eye_position, modelview, pos);
      }
      else {
	 register_matrix_param5( p, STATE_MODELVIEW_MATRIX, 0, 0, 3,
				 STATE_MATRIX_TRANSPOSE, modelview );

	 emit_transpose_matrix_transform_vec4(p, p->eye_position, modelview, pos);
      }
   }
   
   return p->eye_position;
}


static struct ureg get_eye_position_z( struct tnl_program *p )
{
   if (!is_undef(p->eye_position)) 
      return swizzle1(p->eye_position, Z);

   if (is_undef(p->eye_position_z)) {
      struct ureg pos = register_input( p, VERT_ATTRIB_POS ); 
      struct ureg modelview[4];

      p->eye_position_z = reserve_temp(p);

      register_matrix_param5( p, STATE_MODELVIEW_MATRIX, 0, 0, 3,
                              0, modelview );

      emit_op2(p, OPCODE_DP4, p->eye_position_z, 0, pos, modelview[2]);
   }
   
   return p->eye_position_z;
}
   

static struct ureg get_eye_position_normalized( struct tnl_program *p )
{
   if (is_undef(p->eye_position_normalized)) {
      struct ureg eye = get_eye_position(p);
      p->eye_position_normalized = reserve_temp(p);
      emit_normalize_vec3(p, p->eye_position_normalized, eye);
   }
   
   return p->eye_position_normalized;
}


static struct ureg get_transformed_normal( struct tnl_program *p )
{
   if (is_undef(p->transformed_normal) &&
       !p->state->need_eye_coords &&
       !p->state->normalize &&
       !(p->state->need_eye_coords == p->state->rescale_normals))
   {
      p->transformed_normal = register_input(p, VERT_ATTRIB_NORMAL );
   }
   else if (is_undef(p->transformed_normal)) 
   {
      struct ureg normal = register_input(p, VERT_ATTRIB_NORMAL );
      struct ureg mvinv[3];
      struct ureg transformed_normal = reserve_temp(p);

      if (p->state->need_eye_coords) {
         register_matrix_param5( p, STATE_MODELVIEW_MATRIX, 0, 0, 2,
                                 STATE_MATRIX_INVTRANS, mvinv );

         /* Transform to eye space:
          */
         emit_matrix_transform_vec3( p, transformed_normal, mvinv, normal );
         normal = transformed_normal;
      }

      /* Normalize/Rescale:
       */
      if (p->state->normalize) {
	 emit_normalize_vec3( p, transformed_normal, normal );
         normal = transformed_normal;
      }
      else if (p->state->need_eye_coords == p->state->rescale_normals) {
         /* This is already adjusted for eye/non-eye rendering:
          */
	 struct ureg rescale = register_param2(p, STATE_INTERNAL,
                                               STATE_NORMAL_SCALE);

	 emit_op2( p, OPCODE_MUL, transformed_normal, 0, normal, rescale );
         normal = transformed_normal;
      }
      
      assert(normal.file == PROGRAM_TEMPORARY);
      p->transformed_normal = normal;
   }

   return p->transformed_normal;
}


static void build_hpos( struct tnl_program *p )
{
   struct ureg pos = register_input( p, VERT_ATTRIB_POS ); 
   struct ureg hpos = register_output( p, VERT_RESULT_HPOS );
   struct ureg mvp[4];

   if (PREFER_DP4) {
      register_matrix_param5( p, STATE_MVP_MATRIX, 0, 0, 3, 
			      0, mvp );
      emit_matrix_transform_vec4( p, hpos, mvp, pos );
   }
   else {
      register_matrix_param5( p, STATE_MVP_MATRIX, 0, 0, 3, 
			      STATE_MATRIX_TRANSPOSE, mvp );
      emit_transpose_matrix_transform_vec4( p, hpos, mvp, pos );
   }
}


static GLuint material_attrib( GLuint side, GLuint property )
{
   return ((property - STATE_AMBIENT) * 2 + 
	   side);
}


/**
 * Get a bitmask of which material values vary on a per-vertex basis.
 */
static void set_material_flags( struct tnl_program *p )
{
   p->color_materials = 0;
   p->materials = 0;

   if (p->state->light_color_material) {
      p->materials = 
	 p->color_materials = p->state->light_color_material_mask;
   }

   p->materials |= p->state->light_material_mask;
}


/* XXX temporary!!! */
#define _TNL_ATTRIB_MAT_FRONT_AMBIENT 32

static struct ureg get_material( struct tnl_program *p, GLuint side, 
				 GLuint property )
{
   GLuint attrib = material_attrib(side, property);

   if (p->color_materials & (1<<attrib))
      return register_input(p, VERT_ATTRIB_COLOR0);
   else if (p->materials & (1<<attrib)) 
      return register_input( p, attrib + _TNL_ATTRIB_MAT_FRONT_AMBIENT );
   else
      return register_param3( p, STATE_MATERIAL, side, property );
}

#define SCENE_COLOR_BITS(side) (( MAT_BIT_FRONT_EMISSION | \
				   MAT_BIT_FRONT_AMBIENT | \
				   MAT_BIT_FRONT_DIFFUSE) << (side))


/**
 * Either return a precalculated constant value or emit code to
 * calculate these values dynamically in the case where material calls
 * are present between begin/end pairs.
 *
 * Probably want to shift this to the program compilation phase - if
 * we always emitted the calculation here, a smart compiler could
 * detect that it was constant (given a certain set of inputs), and
 * lift it out of the main loop.  That way the programs created here
 * would be independent of the vertex_buffer details.
 */
static struct ureg get_scenecolor( struct tnl_program *p, GLuint side )
{
   if (p->materials & SCENE_COLOR_BITS(side)) {
      struct ureg lm_ambient = register_param1(p, STATE_LIGHTMODEL_AMBIENT);
      struct ureg material_emission = get_material(p, side, STATE_EMISSION);
      struct ureg material_ambient = get_material(p, side, STATE_AMBIENT);
      struct ureg material_diffuse = get_material(p, side, STATE_DIFFUSE);
      struct ureg tmp = make_temp(p, material_diffuse);
      emit_op3(p, OPCODE_MAD, tmp,  WRITEMASK_XYZ, lm_ambient, 
	       material_ambient, material_emission);
      return tmp;
   }
   else
      return register_param2( p, STATE_LIGHTMODEL_SCENECOLOR, side );
}


static struct ureg get_lightprod( struct tnl_program *p, GLuint light, 
				  GLuint side, GLuint property )
{
   GLuint attrib = material_attrib(side, property);
   if (p->materials & (1<<attrib)) {
      struct ureg light_value = 
	 register_param3(p, STATE_LIGHT, light, property);
      struct ureg material_value = get_material(p, side, property);
      struct ureg tmp = get_temp(p);
      emit_op2(p, OPCODE_MUL, tmp,  0, light_value, material_value);
      return tmp;
   }
   else
      return register_param4(p, STATE_LIGHTPROD, light, side, property);
}


static struct ureg calculate_light_attenuation( struct tnl_program *p,
						GLuint i, 
						struct ureg VPpli,
						struct ureg dist )
{
   struct ureg attenuation = register_param3(p, STATE_LIGHT, i,
					     STATE_ATTENUATION);
   struct ureg att = get_temp(p);

   /* Calculate spot attenuation:
    */
   if (!p->state->unit[i].light_spotcutoff_is_180) {
      struct ureg spot_dir_norm = register_param3(p, STATE_INTERNAL,
						  STATE_LIGHT_SPOT_DIR_NORMALIZED, i);
      struct ureg spot = get_temp(p);
      struct ureg slt = get_temp(p);

      emit_op2(p, OPCODE_DP3, spot, 0, negate(VPpli), spot_dir_norm);
      emit_op2(p, OPCODE_SLT, slt, 0, swizzle1(spot_dir_norm,W), spot);
      emit_op2(p, OPCODE_POW, spot, 0, spot, swizzle1(attenuation, W));
      emit_op2(p, OPCODE_MUL, att, 0, slt, spot);

      release_temp(p, spot);
      release_temp(p, slt);
   }

   /* Calculate distance attenuation:
    */
   if (p->state->unit[i].light_attenuated) {

      /* 1/d,d,d,1/d */
      emit_op1(p, OPCODE_RCP, dist, WRITEMASK_YZ, dist); 
      /* 1,d,d*d,1/d */
      emit_op2(p, OPCODE_MUL, dist, WRITEMASK_XZ, dist, swizzle1(dist,Y)); 
      /* 1/dist-atten */
      emit_op2(p, OPCODE_DP3, dist, 0, attenuation, dist); 

      if (!p->state->unit[i].light_spotcutoff_is_180) {
	 /* dist-atten */
	 emit_op1(p, OPCODE_RCP, dist, 0, dist); 
	 /* spot-atten * dist-atten */
	 emit_op2(p, OPCODE_MUL, att, 0, dist, att);	
      } else {
	 /* dist-atten */
	 emit_op1(p, OPCODE_RCP, att, 0, dist); 
      }
   }

   return att;
}
						

/**
 * Compute:
 *   lit.y = MAX(0, dots.x)
 *   lit.z = SLT(0, dots.x)
 */
static void emit_degenerate_lit( struct tnl_program *p,
                                 struct ureg lit,
                                 struct ureg dots )
{
   struct ureg id = get_identity_param(p);  /* id = {0,0,0,1} */

   /* Note that lit.x & lit.w will not be examined.  Note also that
    * dots.xyzw == dots.xxxx.
    */

   /* MAX lit, id, dots;
    */
   emit_op2(p, OPCODE_MAX, lit, WRITEMASK_XYZW, id, dots); 

   /* result[2] = (in > 0 ? 1 : 0)
    * SLT lit.z, id.z, dots;   # lit.z = (0 < dots.z) ? 1 : 0
    */
   emit_op2(p, OPCODE_SLT, lit, WRITEMASK_Z, swizzle1(id,Z), dots);
}


/* Need to add some addtional parameters to allow lighting in object
 * space - STATE_SPOT_DIRECTION and STATE_HALF_VECTOR implicitly assume eye
 * space lighting.
 */
static void build_lighting( struct tnl_program *p )
{
   const GLboolean twoside = p->state->light_twoside;
   const GLboolean separate = p->state->separate_specular;
   GLuint nr_lights = 0, count = 0;
   struct ureg normal = get_transformed_normal(p);
   struct ureg lit = get_temp(p);
   struct ureg dots = get_temp(p);
   struct ureg _col0 = undef, _col1 = undef;
   struct ureg _bfc0 = undef, _bfc1 = undef;
   GLuint i;

   /*
    * NOTE:
    * dot.x = dot(normal, VPpli)
    * dot.y = dot(normal, halfAngle)
    * dot.z = back.shininess
    * dot.w = front.shininess
    */

   for (i = 0; i < MAX_LIGHTS; i++) 
      if (p->state->unit[i].light_enabled)
	 nr_lights++;
   
   set_material_flags(p);

   {
      if (!p->state->material_shininess_is_zero) {
         struct ureg shininess = get_material(p, 0, STATE_SHININESS);
         emit_op1(p, OPCODE_MOV, dots,  WRITEMASK_W, swizzle1(shininess,X));
         release_temp(p, shininess);
      }

      _col0 = make_temp(p, get_scenecolor(p, 0));
      if (separate)
	 _col1 = make_temp(p, get_identity_param(p));
      else
	 _col1 = _col0;

   }

   if (twoside) {
      if (!p->state->material_shininess_is_zero) {
         struct ureg shininess = get_material(p, 1, STATE_SHININESS);
         emit_op1(p, OPCODE_MOV, dots, WRITEMASK_Z, 
                  negate(swizzle1(shininess,X)));
         release_temp(p, shininess);
      }

      _bfc0 = make_temp(p, get_scenecolor(p, 1));
      if (separate)
	 _bfc1 = make_temp(p, get_identity_param(p));
      else
	 _bfc1 = _bfc0;
   }

   /* If no lights, still need to emit the scenecolor.
    */
   {
      struct ureg res0 = register_output( p, VERT_RESULT_COL0 );
      emit_op1(p, OPCODE_MOV, res0, 0, _col0);
   }

   if (separate) {
      struct ureg res1 = register_output( p, VERT_RESULT_COL1 );
      emit_op1(p, OPCODE_MOV, res1, 0, _col1);
   }

   if (twoside) {
      struct ureg res0 = register_output( p, VERT_RESULT_BFC0 );
      emit_op1(p, OPCODE_MOV, res0, 0, _bfc0);
   }
      
   if (twoside && separate) {
      struct ureg res1 = register_output( p, VERT_RESULT_BFC1 );
      emit_op1(p, OPCODE_MOV, res1, 0, _bfc1);
   }
      
   if (nr_lights == 0) {
      release_temps(p);
      return;
   }

   for (i = 0; i < MAX_LIGHTS; i++) {
      if (p->state->unit[i].light_enabled) {
	 struct ureg half = undef;
	 struct ureg att = undef, VPpli = undef;
	  
	 count++;

	 if (p->state->unit[i].light_eyepos3_is_zero) {
	    /* Can used precomputed constants in this case.
	     * Attenuation never applies to infinite lights.
	     */
	    VPpli = register_param3(p, STATE_INTERNAL, 
				    STATE_LIGHT_POSITION_NORMALIZED, i); 
            
            if (!p->state->material_shininess_is_zero) {
               if (p->state->light_local_viewer) {
                  struct ureg eye_hat = get_eye_position_normalized(p);
                  half = get_temp(p);
                  emit_op2(p, OPCODE_SUB, half, 0, VPpli, eye_hat);
                  emit_normalize_vec3(p, half, half);
               } else {
                  half = register_param3(p, STATE_INTERNAL, 
                                         STATE_LIGHT_HALF_VECTOR, i);
               }
            }
	 } 
	 else {
	    struct ureg Ppli = register_param3(p, STATE_INTERNAL, 
					       STATE_LIGHT_POSITION, i); 
	    struct ureg V = get_eye_position(p);
	    struct ureg dist = get_temp(p);

	    VPpli = get_temp(p); 
 
	    /* Calculate VPpli vector
	     */
	    emit_op2(p, OPCODE_SUB, VPpli, 0, Ppli, V); 

	    /* Normalize VPpli.  The dist value also used in
	     * attenuation below.
	     */
	    emit_op2(p, OPCODE_DP3, dist, 0, VPpli, VPpli);
	    emit_op1(p, OPCODE_RSQ, dist, 0, dist);
	    emit_op2(p, OPCODE_MUL, VPpli, 0, VPpli, dist);

	    /* Calculate attenuation:
	     */ 
	    if (!p->state->unit[i].light_spotcutoff_is_180 ||
		p->state->unit[i].light_attenuated) {
	       att = calculate_light_attenuation(p, i, VPpli, dist);
	    }

	    /* Calculate viewer direction, or use infinite viewer:
	     */
            if (!p->state->material_shininess_is_zero) {
               half = get_temp(p);

               if (p->state->light_local_viewer) {
                  struct ureg eye_hat = get_eye_position_normalized(p);
                  emit_op2(p, OPCODE_SUB, half, 0, VPpli, eye_hat);
               }
               else {
                  struct ureg z_dir = swizzle(get_identity_param(p),X,Y,W,Z); 
                  emit_op2(p, OPCODE_ADD, half, 0, VPpli, z_dir);
               }

               emit_normalize_vec3(p, half, half);
            }

	    release_temp(p, dist);
	 }

	 /* Calculate dot products:
	  */
         if (p->state->material_shininess_is_zero) {
            emit_op2(p, OPCODE_DP3, dots, 0, normal, VPpli);
         }
         else {
            emit_op2(p, OPCODE_DP3, dots, WRITEMASK_X, normal, VPpli);
            emit_op2(p, OPCODE_DP3, dots, WRITEMASK_Y, normal, half);
         }

	 /* Front face lighting:
	  */
	 {
	    struct ureg ambient = get_lightprod(p, i, 0, STATE_AMBIENT);
	    struct ureg diffuse = get_lightprod(p, i, 0, STATE_DIFFUSE);
	    struct ureg specular = get_lightprod(p, i, 0, STATE_SPECULAR);
	    struct ureg res0, res1;
	    GLuint mask0, mask1;

	    if (count == nr_lights) {
	       if (separate) {
		  mask0 = WRITEMASK_XYZ;
		  mask1 = WRITEMASK_XYZ;
		  res0 = register_output( p, VERT_RESULT_COL0 );
		  res1 = register_output( p, VERT_RESULT_COL1 );
	       }
	       else {
		  mask0 = 0;
		  mask1 = WRITEMASK_XYZ;
		  res0 = _col0;
		  res1 = register_output( p, VERT_RESULT_COL0 );
	       }
	    } else {
	       mask0 = 0;
	       mask1 = 0;
	       res0 = _col0;
	       res1 = _col1;
	    }

	    if (!is_undef(att)) {
               /* light is attenuated by distance */
               emit_op1(p, OPCODE_LIT, lit, 0, dots);
               emit_op2(p, OPCODE_MUL, lit, 0, lit, att);
               emit_op3(p, OPCODE_MAD, _col0, 0, swizzle1(lit,X), ambient, _col0);
            } 
            else if (!p->state->material_shininess_is_zero) {
               /* there's a non-zero specular term */
               emit_op1(p, OPCODE_LIT, lit, 0, dots);
               emit_op2(p, OPCODE_ADD, _col0, 0, ambient, _col0);
            } 
            else {
               /* no attenutation, no specular */
               emit_degenerate_lit(p, lit, dots);
               emit_op2(p, OPCODE_ADD, _col0, 0, ambient, _col0);
            }

	    emit_op3(p, OPCODE_MAD, res0, mask0, swizzle1(lit,Y), diffuse, _col0);
	    emit_op3(p, OPCODE_MAD, res1, mask1, swizzle1(lit,Z), specular, _col1);
      
	    release_temp(p, ambient);
	    release_temp(p, diffuse);
	    release_temp(p, specular);
	 }

	 /* Back face lighting:
	  */
	 if (twoside) {
	    struct ureg ambient = get_lightprod(p, i, 1, STATE_AMBIENT);
	    struct ureg diffuse = get_lightprod(p, i, 1, STATE_DIFFUSE);
	    struct ureg specular = get_lightprod(p, i, 1, STATE_SPECULAR);
	    struct ureg res0, res1;
	    GLuint mask0, mask1;
	       
	    if (count == nr_lights) {
	       if (separate) {
		  mask0 = WRITEMASK_XYZ;
		  mask1 = WRITEMASK_XYZ;
		  res0 = register_output( p, VERT_RESULT_BFC0 );
		  res1 = register_output( p, VERT_RESULT_BFC1 );
	       }
	       else {
		  mask0 = 0;
		  mask1 = WRITEMASK_XYZ;
		  res0 = _bfc0;
		  res1 = register_output( p, VERT_RESULT_BFC0 );
	       }
	    } else {
	       res0 = _bfc0;
	       res1 = _bfc1;
	       mask0 = 0;
	       mask1 = 0;
	    }

            dots = negate(swizzle(dots,X,Y,W,Z));

	    if (!is_undef(att)) {
               emit_op1(p, OPCODE_LIT, lit, 0, dots);
	       emit_op2(p, OPCODE_MUL, lit, 0, lit, att);
               emit_op3(p, OPCODE_MAD, _bfc0, 0, swizzle1(lit,X), ambient, _bfc0);
            }
            else if (!p->state->material_shininess_is_zero) {
               emit_op1(p, OPCODE_LIT, lit, 0, dots);
               emit_op2(p, OPCODE_ADD, _bfc0, 0, ambient, _bfc0);
            } 
            else {
               emit_degenerate_lit(p, lit, dots);
               emit_op2(p, OPCODE_ADD, _bfc0, 0, ambient, _bfc0);
            }

	    emit_op3(p, OPCODE_MAD, res0, mask0, swizzle1(lit,Y), diffuse, _bfc0);
	    emit_op3(p, OPCODE_MAD, res1, mask1, swizzle1(lit,Z), specular, _bfc1);
            /* restore negate flag for next lighting */
            dots = negate(dots);

	    release_temp(p, ambient);
	    release_temp(p, diffuse);
	    release_temp(p, specular);
	 }

	 release_temp(p, half);
	 release_temp(p, VPpli);
	 release_temp(p, att);
      }
   }

   release_temps( p );
}


static void build_fog( struct tnl_program *p )
{
   struct ureg fog = register_output(p, VERT_RESULT_FOGC);
   struct ureg input;

   if (p->state->fog_source_is_depth) {
      input = get_eye_position_z(p);
   }
   else {
      input = swizzle1(register_input(p, VERT_ATTRIB_FOG), X);
   }

   if (p->state->fog_mode && p->state->tnl_do_vertex_fog) {
      struct ureg params = register_param2(p, STATE_INTERNAL,
					   STATE_FOG_PARAMS_OPTIMIZED);
      struct ureg tmp = get_temp(p);
      GLboolean useabs = (p->state->fog_mode != FOG_EXP2);

      if (useabs) {
	 emit_op1(p, OPCODE_ABS, tmp, 0, input);
      }

      switch (p->state->fog_mode) {
      case FOG_LINEAR: {
	 struct ureg id = get_identity_param(p);
	 emit_op3(p, OPCODE_MAD, tmp, 0, useabs ? tmp : input,
			swizzle1(params,X), swizzle1(params,Y));
	 emit_op2(p, OPCODE_MAX, tmp, 0, tmp, swizzle1(id,X)); /* saturate */
	 emit_op2(p, OPCODE_MIN, fog, WRITEMASK_X, tmp, swizzle1(id,W));
	 break;
      }
      case FOG_EXP:
	 emit_op2(p, OPCODE_MUL, tmp, 0, useabs ? tmp : input,
			swizzle1(params,Z));
	 emit_op1(p, OPCODE_EX2, fog, WRITEMASK_X, negate(tmp));
	 break;
      case FOG_EXP2:
	 emit_op2(p, OPCODE_MUL, tmp, 0, input, swizzle1(params,W));
	 emit_op2(p, OPCODE_MUL, tmp, 0, tmp, tmp);
	 emit_op1(p, OPCODE_EX2, fog, WRITEMASK_X, negate(tmp));
	 break;
      }

      release_temp(p, tmp);
   }
   else {
      /* results = incoming fog coords (compute fog per-fragment later) 
       *
       * KW:  Is it really necessary to do anything in this case?
       * BP: Yes, we always need to compute the absolute value, unless
       * we want to push that down into the fragment program...
       */
      GLboolean useabs = GL_TRUE;
      emit_op1(p, useabs ? OPCODE_ABS : OPCODE_MOV, fog, WRITEMASK_X, input);
   }
}

 
static void build_reflect_texgen( struct tnl_program *p,
				  struct ureg dest,
				  GLuint writemask )
{
   struct ureg normal = get_transformed_normal(p);
   struct ureg eye_hat = get_eye_position_normalized(p);
   struct ureg tmp = get_temp(p);

   /* n.u */
   emit_op2(p, OPCODE_DP3, tmp, 0, normal, eye_hat); 
   /* 2n.u */
   emit_op2(p, OPCODE_ADD, tmp, 0, tmp, tmp); 
   /* (-2n.u)n + u */
   emit_op3(p, OPCODE_MAD, dest, writemask, negate(tmp), normal, eye_hat);

   release_temp(p, tmp);
}


static void build_sphere_texgen( struct tnl_program *p,
				 struct ureg dest,
				 GLuint writemask )
{
   struct ureg normal = get_transformed_normal(p);
   struct ureg eye_hat = get_eye_position_normalized(p);
   struct ureg tmp = get_temp(p);
   struct ureg half = register_scalar_const(p, .5);
   struct ureg r = get_temp(p);
   struct ureg inv_m = get_temp(p);
   struct ureg id = get_identity_param(p);

   /* Could share the above calculations, but it would be
    * a fairly odd state for someone to set (both sphere and
    * reflection active for different texture coordinate
    * components.  Of course - if two texture units enable
    * reflect and/or sphere, things start to tilt in favour
    * of seperating this out:
    */

   /* n.u */
   emit_op2(p, OPCODE_DP3, tmp, 0, normal, eye_hat); 
   /* 2n.u */
   emit_op2(p, OPCODE_ADD, tmp, 0, tmp, tmp); 
   /* (-2n.u)n + u */
   emit_op3(p, OPCODE_MAD, r, 0, negate(tmp), normal, eye_hat); 
   /* r + 0,0,1 */
   emit_op2(p, OPCODE_ADD, tmp, 0, r, swizzle(id,X,Y,W,Z)); 
   /* rx^2 + ry^2 + (rz+1)^2 */
   emit_op2(p, OPCODE_DP3, tmp, 0, tmp, tmp); 
   /* 2/m */
   emit_op1(p, OPCODE_RSQ, tmp, 0, tmp); 
   /* 1/m */
   emit_op2(p, OPCODE_MUL, inv_m, 0, tmp, half); 
   /* r/m + 1/2 */
   emit_op3(p, OPCODE_MAD, dest, writemask, r, inv_m, half); 
	       
   release_temp(p, tmp);
   release_temp(p, r);
   release_temp(p, inv_m);
}


static void build_texture_transform( struct tnl_program *p )
{
   GLuint i, j;

   for (i = 0; i < MAX_TEXTURE_COORD_UNITS; i++) {

      if (!(p->state->fragprog_inputs_read & FRAG_BIT_TEX(i)))
	 continue;
							     
      if (p->state->unit[i].texgen_enabled || 
	  p->state->unit[i].texmat_enabled) {
	 
	 GLuint texmat_enabled = p->state->unit[i].texmat_enabled;
	 struct ureg out = register_output(p, VERT_RESULT_TEX0 + i);
	 struct ureg out_texgen = undef;

	 if (p->state->unit[i].texgen_enabled) {
	    GLuint copy_mask = 0;
	    GLuint sphere_mask = 0;
	    GLuint reflect_mask = 0;
	    GLuint normal_mask = 0;
	    GLuint modes[4];
	 
	    if (texmat_enabled) 
	       out_texgen = get_temp(p);
	    else
	       out_texgen = out;

	    modes[0] = p->state->unit[i].texgen_mode0;
	    modes[1] = p->state->unit[i].texgen_mode1;
	    modes[2] = p->state->unit[i].texgen_mode2;
	    modes[3] = p->state->unit[i].texgen_mode3;

	    for (j = 0; j < 4; j++) {
	       switch (modes[j]) {
	       case TXG_OBJ_LINEAR: {
		  struct ureg obj = register_input(p, VERT_ATTRIB_POS);
		  struct ureg plane = 
		     register_param3(p, STATE_TEXGEN, i,
				     STATE_TEXGEN_OBJECT_S + j);

		  emit_op2(p, OPCODE_DP4, out_texgen, WRITEMASK_X << j, 
			   obj, plane );
		  break;
	       }
	       case TXG_EYE_LINEAR: {
		  struct ureg eye = get_eye_position(p);
		  struct ureg plane = 
		     register_param3(p, STATE_TEXGEN, i, 
				     STATE_TEXGEN_EYE_S + j);

		  emit_op2(p, OPCODE_DP4, out_texgen, WRITEMASK_X << j, 
			   eye, plane );
		  break;
	       }
	       case TXG_SPHERE_MAP: 
		  sphere_mask |= WRITEMASK_X << j;
		  break;
	       case TXG_REFLECTION_MAP:
		  reflect_mask |= WRITEMASK_X << j;
		  break;
	       case TXG_NORMAL_MAP: 
		  normal_mask |= WRITEMASK_X << j;
		  break;
	       case TXG_NONE:
		  copy_mask |= WRITEMASK_X << j;
	       }
	    }

	    if (sphere_mask) {
	       build_sphere_texgen(p, out_texgen, sphere_mask);
	    }

	    if (reflect_mask) {
	       build_reflect_texgen(p, out_texgen, reflect_mask);
	    }

	    if (normal_mask) {
	       struct ureg normal = get_transformed_normal(p);
	       emit_op1(p, OPCODE_MOV, out_texgen, normal_mask, normal );
	    }

	    if (copy_mask) {
	       struct ureg in = register_input(p, VERT_ATTRIB_TEX0+i);
	       emit_op1(p, OPCODE_MOV, out_texgen, copy_mask, in );
	    }
	 }

	 if (texmat_enabled) {
	    struct ureg texmat[4];
	    struct ureg in = (!is_undef(out_texgen) ? 
			      out_texgen : 
			      register_input(p, VERT_ATTRIB_TEX0+i));
	    if (PREFER_DP4) {
	       register_matrix_param5( p, STATE_TEXTURE_MATRIX, i, 0, 3,
				       0, texmat );
	       emit_matrix_transform_vec4( p, out, texmat, in );
	    }
	    else {
	       register_matrix_param5( p, STATE_TEXTURE_MATRIX, i, 0, 3,
				       STATE_MATRIX_TRANSPOSE, texmat );
	       emit_transpose_matrix_transform_vec4( p, out, texmat, in );
	    }
	 }

	 release_temps(p);
      } 
      else {
	 emit_passthrough(p, VERT_ATTRIB_TEX0+i, VERT_RESULT_TEX0+i);
      }
   }
}


/**
 * Point size attenuation computation.
 */
static void build_atten_pointsize( struct tnl_program *p )
{
   struct ureg eye = get_eye_position_z(p);
   struct ureg state_size = register_param1(p, STATE_POINT_SIZE);
   struct ureg state_attenuation = register_param1(p, STATE_POINT_ATTENUATION);
   struct ureg out = register_output(p, VERT_RESULT_PSIZ);
   struct ureg ut = get_temp(p);

   /* dist = |eyez| */
   emit_op1(p, OPCODE_ABS, ut, WRITEMASK_Y, swizzle1(eye, Z));
   /* p1 + dist * (p2 + dist * p3); */
   emit_op3(p, OPCODE_MAD, ut, WRITEMASK_X, swizzle1(ut, Y),
		swizzle1(state_attenuation, Z), swizzle1(state_attenuation, Y));
   emit_op3(p, OPCODE_MAD, ut, WRITEMASK_X, swizzle1(ut, Y),
		ut, swizzle1(state_attenuation, X));

   /* 1 / sqrt(factor) */
   emit_op1(p, OPCODE_RSQ, ut, WRITEMASK_X, ut );

#if 0
   /* out = pointSize / sqrt(factor) */
   emit_op2(p, OPCODE_MUL, out, WRITEMASK_X, ut, state_size);
#else
   /* this is a good place to clamp the point size since there's likely
    * no hardware registers to clamp point size at rasterization time.
    */
   emit_op2(p, OPCODE_MUL, ut, WRITEMASK_X, ut, state_size);
   emit_op2(p, OPCODE_MAX, ut, WRITEMASK_X, ut, swizzle1(state_size, Y));
   emit_op2(p, OPCODE_MIN, out, WRITEMASK_X, ut, swizzle1(state_size, Z));
#endif

   release_temp(p, ut);
}


/**
 * Emit constant point size.
 */
static void build_constant_pointsize( struct tnl_program *p )
{
   struct ureg state_size = register_param1(p, STATE_POINT_SIZE);
   struct ureg out = register_output(p, VERT_RESULT_PSIZ);
   emit_op1(p, OPCODE_MOV, out, WRITEMASK_X, state_size);
}


/**
 * Pass-though per-vertex point size, from user's point size array.
 */
static void build_array_pointsize( struct tnl_program *p )
{
   struct ureg in = register_input(p, VERT_ATTRIB_POINT_SIZE);
   struct ureg out = register_output(p, VERT_RESULT_PSIZ);
   emit_op1(p, OPCODE_MOV, out, WRITEMASK_X, in);
}


static void build_tnl_program( struct tnl_program *p )
{   /* Emit the program, starting with modelviewproject:
    */
   build_hpos(p);

   /* Lighting calculations:
    */
   if (p->state->fragprog_inputs_read & (FRAG_BIT_COL0|FRAG_BIT_COL1)) {
      if (p->state->light_global_enabled)
	 build_lighting(p);
      else {
	 if (p->state->fragprog_inputs_read & FRAG_BIT_COL0)
	    emit_passthrough(p, VERT_ATTRIB_COLOR0, VERT_RESULT_COL0);

	 if (p->state->fragprog_inputs_read & FRAG_BIT_COL1)
	    emit_passthrough(p, VERT_ATTRIB_COLOR1, VERT_RESULT_COL1);
      }
   }

   if ((p->state->fragprog_inputs_read & FRAG_BIT_FOGC) ||
       p->state->fog_mode != FOG_NONE)
      build_fog(p);

   if (p->state->fragprog_inputs_read & FRAG_BITS_TEX_ANY)
      build_texture_transform(p);

   if (p->state->point_attenuated)
      build_atten_pointsize(p);
   else if (p->state->point_array)
      build_array_pointsize(p);
#if 0
   else
      build_constant_pointsize(p);
#else
   (void) build_constant_pointsize;
#endif

   /* Finish up:
    */
   emit_op1(p, OPCODE_END, undef, 0, undef);

   /* Disassemble:
    */
   if (DISASSEM) {
      _mesa_printf ("\n");
   }
}


static void
create_new_program( const struct state_key *key,
                    struct gl_vertex_program *program,
                    GLuint max_temps)
{
   struct tnl_program p;

   _mesa_memset(&p, 0, sizeof(p));
   p.state = key;
   p.program = program;
   p.eye_position = undef;
   p.eye_position_z = undef;
   p.eye_position_normalized = undef;
   p.transformed_normal = undef;
   p.identity = undef;
   p.temp_in_use = 0;
   
   if (max_temps >= sizeof(int) * 8)
      p.temp_reserved = 0;
   else
      p.temp_reserved = ~((1<<max_temps)-1);

   /* Start by allocating 32 instructions.
    * If we need more, we'll grow the instruction array as needed.
    */
   p.max_inst = 32;
   p.program->Base.Instructions = _mesa_alloc_instructions(p.max_inst);
   p.program->Base.String = NULL;
   p.program->Base.NumInstructions =
   p.program->Base.NumTemporaries =
   p.program->Base.NumParameters =
   p.program->Base.NumAttributes = p.program->Base.NumAddressRegs = 0;
   p.program->Base.Parameters = _mesa_new_parameter_list();
   p.program->Base.InputsRead = 0;
   p.program->Base.OutputsWritten = 0;

   build_tnl_program( &p );
}


/**
 * Return a vertex program which implements the current fixed-function
 * transform/lighting/texgen operations.
 * XXX move this into core mesa (main/)
 */
struct gl_vertex_program *
_mesa_get_fixed_func_vertex_program(GLcontext *ctx)
{
   struct gl_vertex_program *prog;
   struct state_key key;

   /* Grab all the relevent state and put it in a single structure:
    */
   make_state_key(ctx, &key);

   /* Look for an already-prepared program for this state:
    */
   prog = (struct gl_vertex_program *)
      _mesa_search_program_cache(ctx->VertexProgram.Cache, &key, sizeof(key));
   
   if (!prog) {
      /* OK, we'll have to build a new one */
      if (0)
         _mesa_printf("Build new TNL program\n");
	 
      prog = (struct gl_vertex_program *)
         ctx->Driver.NewProgram(ctx, GL_VERTEX_PROGRAM_ARB, 0); 
      if (!prog)
         return NULL;

      create_new_program( &key, prog,
                          ctx->Const.VertexProgram.MaxTemps );

#if 0
      if (ctx->Driver.ProgramStringNotify)
         ctx->Driver.ProgramStringNotify( ctx, GL_VERTEX_PROGRAM_ARB, 
                                          &prog->Base );
#endif
      _mesa_program_cache_insert(ctx, ctx->VertexProgram.Cache,
                                 &key, sizeof(key), &prog->Base);
   }

   return prog;
}
