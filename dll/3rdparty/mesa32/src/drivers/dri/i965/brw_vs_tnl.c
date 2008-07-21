/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 2005  Tungsten Graphics   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * TUNGSTEN GRAPHICS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file t_vp_build.c
 * Create a vertex program to execute the current fixed function T&L pipeline.
 * \author Keith Whitwell
 */


#include "glheader.h"
#include "macros.h"
#include "enums.h"
#include "shader/prog_parameter.h"
#include "shader/prog_print.h"
#include "brw_vs.h"
#include "brw_state.h"


struct state_key {
   unsigned light_global_enabled:1;
   unsigned light_local_viewer:1;
   unsigned light_twoside:1;
   unsigned light_color_material:1;
   unsigned light_color_material_mask:12;
   unsigned light_material_mask:12;
   unsigned normalize:1;
   unsigned rescale_normals:1;
   unsigned fog_source_is_depth:1;
   unsigned tnl_do_vertex_fog:1;
   unsigned separate_specular:1;
   unsigned fog_option:2;
   unsigned point_attenuated:1;
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

static void make_state_key( GLcontext *ctx, struct state_key *key )
{
   struct brw_context *brw = brw_context(ctx);
   const struct gl_fragment_program *fp = brw->fragment_program;
   GLuint i;

   /* This now relies on texenvprogram.c being active:
    */
   assert(fp);

   memset(key, 0, sizeof(*key));

   /* BRW_NEW_FRAGMENT_PROGRAM */
   key->fragprog_inputs_read = fp->Base.InputsRead;

   /* _NEW_LIGHT */
   key->separate_specular = (brw->attribs.Light->Model.ColorControl ==
			     GL_SEPARATE_SPECULAR_COLOR);

   /* _NEW_LIGHT */
   if (brw->attribs.Light->Enabled) {
      key->light_global_enabled = 1;

      if (brw->attribs.Light->Model.LocalViewer)
	 key->light_local_viewer = 1;

      if (brw->attribs.Light->Model.TwoSide)
	 key->light_twoside = 1;

      if (brw->attribs.Light->ColorMaterialEnabled) {
	 key->light_color_material = 1;
	 key->light_color_material_mask = brw->attribs.Light->ColorMaterialBitmask;
      }

      /* BRW_NEW_INPUT_VARYING */

      /* For these programs, material values are stuffed into the
       * generic slots:
       */
      for (i = 0 ; i < MAT_ATTRIB_MAX ; i++) 
	 if (brw->vb.info.varying & (1<<(VERT_ATTRIB_GENERIC0 + i))) 
	    key->light_material_mask |= 1<<i;

      for (i = 0; i < MAX_LIGHTS; i++) {
	 struct gl_light *light = &brw->attribs.Light->Light[i];

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
   }

   /* _NEW_TRANSFORM */
   if (brw->attribs.Transform->Normalize)
      key->normalize = 1;

   if (brw->attribs.Transform->RescaleNormals)
      key->rescale_normals = 1;

   /* BRW_NEW_FRAGMENT_PROGRAM */
   key->fog_option = translate_fog_mode(fp->FogOption);
   if (key->fog_option)
      key->fragprog_inputs_read |= FRAG_BIT_FOGC;
   
   /* _NEW_FOG */
   if (brw->attribs.Fog->FogCoordinateSource == GL_FRAGMENT_DEPTH_EXT)
      key->fog_source_is_depth = 1;
   
   /* _NEW_HINT, ??? */
   if (1)
      key->tnl_do_vertex_fog = 1;

   /* _NEW_POINT */
   if (brw->attribs.Point->_Attenuated)
      key->point_attenuated = 1;

   /* _NEW_TEXTURE */
   if (brw->attribs.Texture->_TexGenEnabled ||
       brw->attribs.Texture->_TexMatEnabled ||
       brw->attribs.Texture->_EnabledUnits)
      key->texture_enabled_global = 1;
      
   for (i = 0; i < MAX_TEXTURE_UNITS; i++) {
      struct gl_texture_unit *texUnit = &brw->attribs.Texture->Unit[i];

      if (texUnit->_ReallyEnabled)
 	 key->unit[i].texunit_really_enabled = 1;

      if (brw->attribs.Texture->_TexMatEnabled & ENABLE_TEXMAT(i))      
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
#define PREFER_DP4 1


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
   GLint idx:8;      /* relative addressing may be negative */
   GLuint negate:1;
   GLuint swz:12;
   GLuint pad:7;
};


struct tnl_program {
   const struct state_key *state;
   struct gl_vertex_program *program;
   
   GLuint nr_instructions;
   GLuint temp_in_use;
   GLuint temp_reserved;
   
   struct ureg eye_position;
   struct ureg eye_position_normalized;
   struct ureg eye_normal;
   struct ureg identity;

   GLuint materials;
   GLuint color_materials;
};


const static struct ureg undef = { 
   PROGRAM_UNDEFINED,
   ~0,
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



static struct ureg ureg_negate( struct ureg reg )
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
   int bit = ffs( ~p->temp_in_use );
   if (!bit) {
      fprintf(stderr, "%s: out of temporaries\n", __FILE__);
      assert(0);
   }

   if (bit > p->program->Base.NumTemporaries)
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



static struct ureg register_input( struct tnl_program *p, GLuint input )
{
   assert(input < 32);

   p->program->Base.InputsRead |= (1<<input);
   return make_ureg(PROGRAM_INPUT, input);
}

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
                                     &swizzle);
   /* XXX what about swizzle? */
   return make_ureg(PROGRAM_STATE_VAR, idx);
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

static struct ureg register_param5( struct tnl_program *p, 
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
				    GLint s0, /* matrix name */
				    GLint s1, /* texture matrix number */
				    GLint s2, /* first row */
				    GLint s3, /* last row */
				    GLint s4, /* modifier */
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
   src->RelAddr = 0;
   src->NegateBase = reg.negate;
   src->Abs = 0;
   src->NegateAbs = 0;
}

static void emit_dst( struct prog_dst_register *dst,
		      struct ureg reg, GLuint mask )
{
   dst->File = reg.file;
   dst->Index = reg.idx;
   /* allow zero as a shorthand for xyzw */
   dst->WriteMask = mask ? mask : WRITEMASK_XYZW; 
   dst->CondMask = 0;
   dst->CondSwizzle = 0;
   dst->CondSrc = 0;
   dst->pad = 0;
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
		       GLuint op,
		       struct ureg dest,
		       GLuint mask,
		       struct ureg src0,
		       struct ureg src1,
		       struct ureg src2,
		       const char *fn,
		       GLuint line)
{
   GLuint nr = p->program->Base.NumInstructions++;
      
   if (nr >= p->nr_instructions) {
      p->program->Base.Instructions = 
	 _mesa_realloc(p->program->Base.Instructions,
		       sizeof(struct prog_instruction) * p->nr_instructions,
		       sizeof(struct prog_instruction) * (p->nr_instructions *= 2));
   }

   {      
      struct prog_instruction *inst = &p->program->Base.Instructions[nr];
      memset(inst, 0, sizeof(*inst));
      inst->Opcode = op; 
      inst->StringPos = 0;
      inst->Data = 0;
   
      emit_arg( &inst->SrcReg[0], src0 );
      emit_arg( &inst->SrcReg[1], src1 );
      emit_arg( &inst->SrcReg[2], src2 );   

      emit_dst( &inst->DstReg, dest, mask );

      debug_insn(inst, fn, line);
   }
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
   emit_op2(p, OPCODE_DP3, dest, WRITEMASK_W, src, src);
   emit_op1(p, OPCODE_RSQ, dest, WRITEMASK_W, swizzle1(dest,W));
   emit_op2(p, OPCODE_MUL, dest, WRITEMASK_XYZ, src, swizzle1(dest,W));
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


#if 0
static struct ureg get_eye_z( struct tnl_program *p )
{
   if (!is_undef(p->eye_position)) {
      return swizzle1(p->eye_position, Z);
   }
   else if (!is_undef(p->eye_z)) {
      struct ureg pos = register_input( p, BRW_ATTRIB_POS ); 
      struct ureg modelview2;

      p->eye_z = reserve_temp(p);

      register_matrix_param6( p, STATE_MATRIX, STATE_MODELVIEW, 0, 2, 1, 
			      STATE_MATRIX, &modelview2 );

      emit_matrix_transform_vec4(p, p->eye_position, modelview, pos);
      emit_op2(p, OPCODE_DP4, p->eye_z, WRITEMASK_Z, pos, modelview2);
   }
   
   return swizzle1(p->eye_z, Z)
}
#endif



static struct ureg get_eye_position_normalized( struct tnl_program *p )
{
   if (is_undef(p->eye_position_normalized)) {
      struct ureg eye = get_eye_position(p);
      p->eye_position_normalized = reserve_temp(p);
      emit_normalize_vec3(p, p->eye_position_normalized, eye);
   }
   
   return p->eye_position_normalized;
}


static struct ureg get_eye_normal( struct tnl_program *p )
{
   if (is_undef(p->eye_normal)) {
      struct ureg normal = register_input(p, VERT_ATTRIB_NORMAL );
      struct ureg mvinv[3];

      register_matrix_param5( p, STATE_MODELVIEW_MATRIX, 0, 0, 2,
			      STATE_MATRIX_INVTRANS, mvinv );

      p->eye_normal = reserve_temp(p);

      /* Transform to eye space:
       */
      emit_matrix_transform_vec3( p, p->eye_normal, mvinv, normal );

      /* Normalize/Rescale:
       */
      if (p->state->normalize) {
	 emit_normalize_vec3( p, p->eye_normal, p->eye_normal );
      }
      else if (p->state->rescale_normals) {
	 struct ureg rescale = register_param2(p, STATE_INTERNAL,
					       STATE_NORMAL_SCALE);

	 emit_op2( p, OPCODE_MUL, p->eye_normal, 0, p->eye_normal, 
		   swizzle1(rescale, X));
      }
   }

   return p->eye_normal;
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
   return (property - STATE_AMBIENT) * 2 + side;
}

/* Get a bitmask of which material values vary on a per-vertex basis.
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

#define SCENE_COLOR_BITS(side) ((MAT_BIT_FRONT_EMISSION | \
				 MAT_BIT_FRONT_AMBIENT | \
				 MAT_BIT_FRONT_DIFFUSE) << (side))

/* Either return a precalculated constant value or emit code to
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
						  STATE_SPOT_DIR_NORMALIZED, i);
      struct ureg spot = get_temp(p);
      struct ureg slt = get_temp(p);

      emit_op2(p, OPCODE_DP3, spot, 0, ureg_negate(VPpli), spot_dir_norm);
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
						




/* Need to add some addtional parameters to allow lighting in object
 * space - STATE_SPOT_DIRECTION and STATE_HALF_VECTOR implicitly assume eye
 * space lighting.
 */
static void build_lighting( struct tnl_program *p )
{
   const GLboolean twoside = p->state->light_twoside;
   const GLboolean separate = p->state->separate_specular;
   GLuint nr_lights = 0, count = 0;
   struct ureg normal = get_eye_normal(p);
   struct ureg lit = get_temp(p);
   struct ureg dots = get_temp(p);
   struct ureg _col0 = undef, _col1 = undef;
   struct ureg _bfc0 = undef, _bfc1 = undef;
   GLuint i;

   for (i = 0; i < MAX_LIGHTS; i++) 
      if (p->state->unit[i].light_enabled)
	 nr_lights++;
   
   set_material_flags(p);

   {
      struct ureg shininess = get_material(p, 0, STATE_SHININESS);
      emit_op1(p, OPCODE_MOV, dots,  WRITEMASK_W, swizzle1(shininess,X));
      release_temp(p, shininess);

      _col0 = make_temp(p, get_scenecolor(p, 0));
      if (separate)
	 _col1 = make_temp(p, get_identity_param(p));
      else
	 _col1 = _col0;

   }

   if (twoside) {
      struct ureg shininess = get_material(p, 1, STATE_SHININESS);
      emit_op1(p, OPCODE_MOV, dots, WRITEMASK_Z, 
	       ureg_negate(swizzle1(shininess,X)));
      release_temp(p, shininess);

      _bfc0 = make_temp(p, get_scenecolor(p, 1));
      if (separate)
	 _bfc1 = make_temp(p, get_identity_param(p));
      else
	 _bfc1 = _bfc0;
   }


   /* If no lights, still need to emit the scenecolor.
    */
   /* KW: changed to do this always - v1.17 "Fix lighting alpha result"? 
    */
   if (p->state->fragprog_inputs_read & FRAG_BIT_COL0)
   {
      struct ureg res0 = register_output( p, VERT_RESULT_COL0 );
      emit_op1(p, OPCODE_MOV, res0, 0, _col0);

      if (twoside) {
	 struct ureg res0 = register_output( p, VERT_RESULT_BFC0 );
	 emit_op1(p, OPCODE_MOV, res0, 0, _bfc0);
      }
   }

   if (separate && (p->state->fragprog_inputs_read & FRAG_BIT_COL1)) {

      struct ureg res1 = register_output( p, VERT_RESULT_COL1 );
      emit_op1(p, OPCODE_MOV, res1, 0, _col1);
      
      if (twoside) {
	 struct ureg res1 = register_output( p, VERT_RESULT_BFC1 );
	 emit_op1(p, OPCODE_MOV, res1, 0, _bfc1);
      }
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
	    VPpli = register_param3(p, STATE_LIGHT, i, 
				    STATE_POSITION_NORMALIZED); 
            if (p->state->light_local_viewer) {
                struct ureg eye_hat = get_eye_position_normalized(p);
                half = get_temp(p);
                emit_op2(p, OPCODE_SUB, half, 0, VPpli, eye_hat);
                emit_normalize_vec3(p, half, half);
            } else {
                half = register_param3(p, STATE_LIGHT, i, STATE_HALF_VECTOR);
            }
	 } 
	 else {
	    struct ureg Ppli = register_param3(p, STATE_LIGHT, i, 
					       STATE_POSITION); 
	    struct ureg V = get_eye_position(p);
	    struct ureg dist = get_temp(p);
       struct ureg tmpPpli = get_temp(p);

	    VPpli = get_temp(p); 
	    half = get_temp(p);

       /* In homogeneous object coordinates
        */
       emit_op1(p, OPCODE_RCP, dist, 0, swizzle1(Ppli, W));
       emit_op2(p, OPCODE_MUL, tmpPpli, 0, Ppli, dist);

	    /* Calulate VPpli vector
	     */
	    emit_op2(p, OPCODE_SUB, VPpli, 0, tmpPpli, V); 

	    /* Normalize VPpli.  The dist value also used in
	     * attenuation below.
	     */
	    emit_op2(p, OPCODE_DP3, dist, 0, VPpli, VPpli);
	    emit_op1(p, OPCODE_RSQ, dist, 0, dist);
	    emit_op2(p, OPCODE_MUL, VPpli, 0, VPpli, dist);


	    /* Calculate  attenuation:
	     */ 
	    if (!p->state->unit[i].light_spotcutoff_is_180 ||
		p->state->unit[i].light_attenuated) {
	       att = calculate_light_attenuation(p, i, VPpli, dist);
	    }
	 
      
	    /* Calculate viewer direction, or use infinite viewer:
	     */
	    if (p->state->light_local_viewer) {
	       struct ureg eye_hat = get_eye_position_normalized(p);
	       emit_op2(p, OPCODE_SUB, half, 0, VPpli, eye_hat);
	    }
	    else {
	       struct ureg z_dir = swizzle(get_identity_param(p),X,Y,W,Z); 
	       emit_op2(p, OPCODE_ADD, half, 0, VPpli, z_dir);
	    }

	    emit_normalize_vec3(p, half, half);

	    release_temp(p, dist);
       release_temp(p, tmpPpli);
	 }

	 /* Calculate dot products:
	  */
	 emit_op2(p, OPCODE_DP3, dots, WRITEMASK_X, normal, VPpli);
	 emit_op2(p, OPCODE_DP3, dots, WRITEMASK_Y, normal, half);

	
	 /* Front face lighting:
	  */
	 {
	    struct ureg ambient = get_lightprod(p, i, 0, STATE_AMBIENT);
	    struct ureg diffuse = get_lightprod(p, i, 0, STATE_DIFFUSE);
	    struct ureg specular = get_lightprod(p, i, 0, STATE_SPECULAR);
	    struct ureg res0, res1;
	    GLuint mask0, mask1;

	    emit_op1(p, OPCODE_LIT, lit, 0, dots);
   
	    if (!is_undef(att)) 
	       emit_op2(p, OPCODE_MUL, lit, 0, lit, att);


	    mask0 = 0;
	    mask1 = 0;
	    res0 = _col0;
	    res1 = _col1;
	    
	    if (count == nr_lights) {
	       if (separate) {
		  mask0 = WRITEMASK_XYZ;
		  mask1 = WRITEMASK_XYZ;

		  if (p->state->fragprog_inputs_read & FRAG_BIT_COL0)
		     res0 = register_output( p, VERT_RESULT_COL0 );

		  if (p->state->fragprog_inputs_read & FRAG_BIT_COL1)
		     res1 = register_output( p, VERT_RESULT_COL1 );
	       }
	       else {
		  mask1 = WRITEMASK_XYZ;

		  if (p->state->fragprog_inputs_read & FRAG_BIT_COL0)
		     res1 = register_output( p, VERT_RESULT_COL0 );
	       }
	    } 

	    emit_op3(p, OPCODE_MAD, _col0, 0, swizzle1(lit,X), ambient, _col0);
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
	       
	    emit_op1(p, OPCODE_LIT, lit, 0, ureg_negate(swizzle(dots,X,Y,W,Z)));

	    if (!is_undef(att)) 
	       emit_op2(p, OPCODE_MUL, lit, 0, lit, att);

	    mask0 = 0;
	    mask1 = 0;
	    res0 = _bfc0;
	    res1 = _bfc1;

	    if (count == nr_lights) {
	       if (separate) {
		  mask0 = WRITEMASK_XYZ;
		  mask1 = WRITEMASK_XYZ;
		  if (p->state->fragprog_inputs_read & FRAG_BIT_COL0)
		     res0 = register_output( p, VERT_RESULT_BFC0 );

		  if (p->state->fragprog_inputs_read & FRAG_BIT_COL1)
		     res1 = register_output( p, VERT_RESULT_BFC1 );
	       }
	       else {
		  mask1 = WRITEMASK_XYZ;

		  if (p->state->fragprog_inputs_read & FRAG_BIT_COL0)
		     res1 = register_output( p, VERT_RESULT_BFC0 );
	       }
	    }

	    emit_op3(p, OPCODE_MAD, _bfc0, 0, swizzle1(lit,X), ambient, _bfc0);
	    emit_op3(p, OPCODE_MAD, res0, mask0, swizzle1(lit,Y), diffuse, _bfc0);
	    emit_op3(p, OPCODE_MAD, res1, mask1, swizzle1(lit,Z), specular, _bfc1);

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
      input = swizzle1(get_eye_position(p), Z);
   }
   else {
      input = swizzle1(register_input(p, VERT_ATTRIB_FOG), X);
   }

   if (p->state->fog_option &&
       p->state->tnl_do_vertex_fog) {
      struct ureg params = register_param2(p, STATE_INTERNAL,
					   STATE_FOG_PARAMS_OPTIMIZED);
      struct ureg tmp = get_temp(p);
      struct ureg id = get_identity_param(p);

      emit_op1(p, OPCODE_MOV, fog, 0, id);

      switch (p->state->fog_option) {
      case FOG_LINEAR: {
	 emit_op1(p, OPCODE_ABS, tmp, 0, input);
	 emit_op3(p, OPCODE_MAD, tmp, 0, tmp, swizzle1(params,X), swizzle1(params,Y));
	 emit_op2(p, OPCODE_MAX, tmp, 0, tmp, swizzle1(id,X)); /* saturate */
	 emit_op2(p, OPCODE_MIN, fog, WRITEMASK_X, tmp, swizzle1(id,W));
	 break;
      }
      case FOG_EXP:
	 emit_op1(p, OPCODE_ABS, tmp, 0, input); 
	 emit_op2(p, OPCODE_MUL, tmp, 0, tmp, swizzle1(params,Z));
	 emit_op1(p, OPCODE_EX2, fog, WRITEMASK_X, ureg_negate(tmp));
	 break;
      case FOG_EXP2:
	 emit_op2(p, OPCODE_MUL, tmp, 0, input, swizzle1(params,W));
	 emit_op2(p, OPCODE_MUL, tmp, 0, tmp, tmp); 
	 emit_op1(p, OPCODE_EX2, fog, WRITEMASK_X, ureg_negate(tmp));
	 break;
      }
      
      release_temp(p, tmp);
   }
   else {
      /* results = incoming fog coords (compute fog per-fragment later) 
       *
       * KW:  Is it really necessary to do anything in this case?
       */
      emit_op1(p, OPCODE_MOV, fog, 0, input);
   }
}
 
static void build_reflect_texgen( struct tnl_program *p,
				  struct ureg dest,
				  GLuint writemask )
{
   struct ureg normal = get_eye_normal(p);
   struct ureg eye_hat = get_eye_position_normalized(p);
   struct ureg tmp = get_temp(p);

   /* n.u */
   emit_op2(p, OPCODE_DP3, tmp, 0, normal, eye_hat); 
   /* 2n.u */
   emit_op2(p, OPCODE_ADD, tmp, 0, tmp, tmp); 
   /* (-2n.u)n + u */
   emit_op3(p, OPCODE_MAD, dest, writemask, ureg_negate(tmp), normal, eye_hat);

   release_temp(p, tmp);
}

static void build_sphere_texgen( struct tnl_program *p,
				 struct ureg dest,
				 GLuint writemask )
{
   struct ureg normal = get_eye_normal(p);
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
   emit_op3(p, OPCODE_MAD, r, 0, ureg_negate(tmp), normal, eye_hat); 
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

   for (i = 0; i < MAX_TEXTURE_UNITS; i++) {

      if (!(p->state->fragprog_inputs_read & (FRAG_BIT_TEX0<<i)))
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
	       struct ureg normal = get_eye_normal(p);
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


/* Seems like it could be tighter:
 */
static void build_pointsize( struct tnl_program *p )
{
   struct ureg eye = get_eye_position(p);
   struct ureg state_size = register_param1(p, STATE_POINT_SIZE);
   struct ureg state_attenuation = register_param1(p, STATE_POINT_ATTENUATION);
   struct ureg out = register_output(p, VERT_RESULT_PSIZ);
   struct ureg ut = get_temp(p);

   /* 1, Z, Z * Z, 1 */      
   emit_op1(p, OPCODE_MOV, ut, WRITEMASK_XW, swizzle1(get_identity_param(p), W));
   emit_op1(p, OPCODE_ABS, ut, WRITEMASK_YZ, swizzle1(eye, Z));
   emit_op2(p, OPCODE_MUL, ut, WRITEMASK_Z, ut, ut);


   /* p1 +  p2 * dist + p3 * dist * dist, 0 */
   emit_op2(p, OPCODE_DP3, ut, WRITEMASK_X, ut, state_attenuation);

   /* 1 / sqrt(factor) */
   emit_op1(p, OPCODE_RSQ, ut, WRITEMASK_X, ut ); 

   /* ut = pointSize / factor */
   emit_op2(p, OPCODE_MUL, ut, WRITEMASK_X, ut, state_size); 

   /* Clamp to min/max - state_size.[yz]
    */
   emit_op2(p, OPCODE_MAX, ut, WRITEMASK_X, ut, swizzle1(state_size, Y)); 
   emit_op2(p, OPCODE_MIN, out, 0, swizzle1(ut, X), swizzle1(state_size, Z)); 
   
   release_temp(p, ut);
}

static void build_tnl_program( struct tnl_program *p )
{  
   /* Emit the program, starting with modelviewproject:
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
       p->state->fog_option != FOG_NONE)
      build_fog(p);

   if (p->state->fragprog_inputs_read & FRAG_BITS_TEX_ANY)
      build_texture_transform(p);

   if (p->state->point_attenuated)
      build_pointsize(p);

   /* Finish up:
    */
   emit_op1(p, OPCODE_END, undef, 0, undef);

   /* Disassemble:
    */
   if (DISASSEM) {
      _mesa_printf ("\n");
   }
}


static void build_new_tnl_program( const struct state_key *key,
				   struct gl_vertex_program *program,
				   GLuint max_temps)
{
   struct tnl_program p;

   _mesa_memset(&p, 0, sizeof(p));
   p.state = key;
   p.program = program;
   p.eye_position = undef;
   p.eye_position_normalized = undef;
   p.eye_normal = undef;
   p.identity = undef;
   p.temp_in_use = 0;
   p.nr_instructions = 16;
   
   if (max_temps >= sizeof(int) * 8)
      p.temp_reserved = 0;
   else
      p.temp_reserved = ~((1<<max_temps)-1);

   p.program->Base.Instructions = 
      _mesa_malloc(sizeof(struct prog_instruction) * p.nr_instructions);
   p.program->Base.String = 0;
   p.program->Base.NumInstructions =
   p.program->Base.NumTemporaries =
   p.program->Base.NumParameters =
   p.program->Base.NumAttributes = p.program->Base.NumAddressRegs = 0;
   p.program->Base.Parameters = _mesa_new_parameter_list();
   p.program->Base.InputsRead = 0;
   p.program->Base.OutputsWritten = 0;

   build_tnl_program( &p );
}

static void *search_cache( struct brw_tnl_cache *cache,
			   GLuint hash,
			   const void *key,
			   GLuint keysize)
{
   struct brw_tnl_cache_item *c;

   for (c = cache->items[hash % cache->size]; c; c = c->next) {
      if (c->hash == hash && memcmp(c->key, key, keysize) == 0)
	 return c->data;
   }

   return NULL;
}

static void rehash( struct brw_tnl_cache *cache )
{
   struct brw_tnl_cache_item **items;
   struct brw_tnl_cache_item *c, *next;
   GLuint size, i;

   size = cache->size * 3;
   items = (struct brw_tnl_cache_item**) _mesa_malloc(size * sizeof(*items));
   _mesa_memset(items, 0, size * sizeof(*items));

   for (i = 0; i < cache->size; i++)
      for (c = cache->items[i]; c; c = next) {
	 next = c->next;
	 c->next = items[c->hash % size];
	 items[c->hash % size] = c;
      }

   FREE(cache->items);
   cache->items = items;
   cache->size = size;
}

static void cache_item( struct brw_tnl_cache *cache,
			GLuint hash,
			const struct state_key *key,
			void *data )
{
   struct brw_tnl_cache_item *c = MALLOC(sizeof(*c));
   c->hash = hash;

   c->key = malloc(sizeof(*key));
   memcpy(c->key, key, sizeof(*key));

   c->data = data;

   if (++cache->n_items > cache->size * 1.5)
      rehash(cache);

   c->next = cache->items[hash % cache->size];
   cache->items[hash % cache->size] = c;
}


static GLuint hash_key( struct state_key *key )
{
   GLuint *ikey = (GLuint *)key;
   GLuint hash = 0, i;

   /* I'm sure this can be improved on, but speed is important:
    */
   for (i = 0; i < sizeof(*key)/sizeof(GLuint); i++)
      hash += ikey[i];

   return hash;
}

static void update_tnl_program( struct brw_context *brw )
{
   GLcontext *ctx = &brw->intel.ctx;
   struct state_key key;
   GLuint hash;
   struct gl_vertex_program *old = brw->tnl_program;

   /* _NEW_PROGRAM */
   if (brw->attribs.VertexProgram->_Enabled) 
      return;
      
   /* Grab all the relevent state and put it in a single structure:
    */
   make_state_key(ctx, &key);
   hash = hash_key(&key);

   /* Look for an already-prepared program for this state:
    */
   brw->tnl_program = (struct gl_vertex_program *)
      search_cache( &brw->tnl_program_cache, hash, &key, sizeof(key) );
   
   /* OK, we'll have to build a new one:
    */
   if (!brw->tnl_program) {
      brw->tnl_program = (struct gl_vertex_program *)
	 ctx->Driver.NewProgram(ctx, GL_VERTEX_PROGRAM_ARB, 0); 

      build_new_tnl_program( &key, brw->tnl_program, 
/* 			     ctx->Const.MaxVertexProgramTemps  */
			     32
	 );

      if (ctx->Driver.ProgramStringNotify)
	 ctx->Driver.ProgramStringNotify( ctx, GL_VERTEX_PROGRAM_ARB, 
					  &brw->tnl_program->Base );

      cache_item( &brw->tnl_program_cache, 
		  hash, &key, brw->tnl_program );
   }

   if (old != brw->tnl_program)
      brw->state.dirty.brw |= BRW_NEW_TNL_PROGRAM;
}

/* Note: See brw_draw.c - the vertex program must not rely on
 * brw->primitive or brw->reduced_prim.
 */
const struct brw_tracked_state brw_tnl_vertprog = {
   .dirty = {
      .mesa = (_NEW_PROGRAM | 
	       _NEW_LIGHT | 
	       _NEW_TRANSFORM | 
	       _NEW_FOG | 
	       _NEW_HINT | 
	       _NEW_POINT | 
	       _NEW_TEXTURE),
      .brw = (BRW_NEW_FRAGMENT_PROGRAM | 
	      BRW_NEW_INPUT_VARYING),
      .cache = 0
   },
   .update = update_tnl_program
};




static void update_active_vertprog( struct brw_context *brw )
{
   const struct gl_vertex_program *prev = brw->vertex_program;

   /* NEW_PROGRAM */
   if (brw->attribs.VertexProgram->_Enabled) {
      brw->vertex_program = brw->attribs.VertexProgram->Current;
   }
   else {
      /* BRW_NEW_TNL_PROGRAM */
      brw->vertex_program = brw->tnl_program;
   }

   if (brw->vertex_program != prev) 
      brw->state.dirty.brw |= BRW_NEW_VERTEX_PROGRAM;
}



const struct brw_tracked_state brw_active_vertprog = {
   .dirty = {
      .mesa = _NEW_PROGRAM,
      .brw = BRW_NEW_TNL_PROGRAM,
      .cache = 0
   },
   .update = update_active_vertprog
};


void brw_ProgramCacheInit( GLcontext *ctx )
{
   struct brw_context *brw = brw_context(ctx);

   brw->tnl_program_cache.size = 17;
   brw->tnl_program_cache.n_items = 0;
   brw->tnl_program_cache.items = (struct brw_tnl_cache_item **)
      _mesa_calloc(brw->tnl_program_cache.size * 
		   sizeof(struct brw_tnl_cache_item));
}

void brw_ProgramCacheDestroy( GLcontext *ctx )
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_tnl_cache_item *c, *next;
   GLuint i;

   for (i = 0; i < brw->tnl_program_cache.size; i++)
      for (c = brw->tnl_program_cache.items[i]; c; c = next) {
	 next = c->next;
	 FREE(c->key);
	 FREE(c->data);
	 FREE(c);
      }

   FREE(brw->tnl_program_cache.items);
}
