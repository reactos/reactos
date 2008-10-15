/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "glheader.h"
#include "macros.h"
#include "enums.h"
#include "shader/program.h"
#include "shader/prog_parameter.h"
#include "shader/prog_instruction.h"
#include "shader/prog_print.h"
#include "shader/prog_statevars.h"
#include "texenvprogram.h"


struct texenvprog_cache_item
{
   GLuint hash;
   void *key;
   struct gl_fragment_program *data;
   struct texenvprog_cache_item *next;
};


/**
 * This MAX is probably a bit generous, but that's OK.  There can be
 * up to four instructions per texture unit (TEX + 3 for combine),
 * then there's fog and specular add.
 */
#define MAX_INSTRUCTIONS ((MAX_TEXTURE_UNITS * 4) + 12)

#define DISASSEM (MESA_VERBOSE & VERBOSE_DISASSEM)

struct mode_opt {
   GLuint Source:4;
   GLuint Operand:3;
};

struct state_key {
   GLbitfield enabled_units;
   GLuint separate_specular:1;
   GLuint fog_enabled:1;
   GLuint fog_mode:2;

   struct {
      GLuint enabled:1;
      GLuint source_index:3;   /* one of TEXTURE_1D/2D/3D/CUBE/RECT_INDEX */
      GLuint shadow:1;
      GLuint ScaleShiftRGB:2;
      GLuint ScaleShiftA:2;

      GLuint NumArgsRGB:2;
      GLuint ModeRGB:4;
      struct mode_opt OptRGB[3];

      GLuint NumArgsA:2;
      GLuint ModeA:4;
      struct mode_opt OptA[3];
   } unit[8];
};

#define FOG_LINEAR  0
#define FOG_EXP     1
#define FOG_EXP2    2
#define FOG_UNKNOWN 3

static GLuint translate_fog_mode( GLenum mode )
{
   switch (mode) {
   case GL_LINEAR: return FOG_LINEAR;
   case GL_EXP: return FOG_EXP;
   case GL_EXP2: return FOG_EXP2;
   default: return FOG_UNKNOWN;
   }
}

#define OPR_SRC_COLOR           0
#define OPR_ONE_MINUS_SRC_COLOR 1
#define OPR_SRC_ALPHA           2
#define OPR_ONE_MINUS_SRC_ALPHA	3
#define OPR_ZERO                4
#define OPR_ONE                 5
#define OPR_UNKNOWN             7

static GLuint translate_operand( GLenum operand )
{
   switch (operand) {
   case GL_SRC_COLOR: return OPR_SRC_COLOR;
   case GL_ONE_MINUS_SRC_COLOR: return OPR_ONE_MINUS_SRC_COLOR;
   case GL_SRC_ALPHA: return OPR_SRC_ALPHA;
   case GL_ONE_MINUS_SRC_ALPHA: return OPR_ONE_MINUS_SRC_ALPHA;
   case GL_ZERO: return OPR_ZERO;
   case GL_ONE: return OPR_ONE;
   default:	return OPR_UNKNOWN;
   }
}

#define SRC_TEXTURE  0
#define SRC_TEXTURE0 1
#define SRC_TEXTURE1 2
#define SRC_TEXTURE2 3
#define SRC_TEXTURE3 4
#define SRC_TEXTURE4 5
#define SRC_TEXTURE5 6
#define SRC_TEXTURE6 7
#define SRC_TEXTURE7 8
#define SRC_CONSTANT 9
#define SRC_PRIMARY_COLOR 10
#define SRC_PREVIOUS 11
#define SRC_UNKNOWN  15

static GLuint translate_source( GLenum src )
{
   switch (src) {
   case GL_TEXTURE: return SRC_TEXTURE;
   case GL_TEXTURE0:
   case GL_TEXTURE1:
   case GL_TEXTURE2:
   case GL_TEXTURE3:
   case GL_TEXTURE4:
   case GL_TEXTURE5:
   case GL_TEXTURE6:
   case GL_TEXTURE7: return SRC_TEXTURE0 + (src - GL_TEXTURE0);
   case GL_CONSTANT: return SRC_CONSTANT;
   case GL_PRIMARY_COLOR: return SRC_PRIMARY_COLOR;
   case GL_PREVIOUS: return SRC_PREVIOUS;
   default: return SRC_UNKNOWN;
   }
}

#define MODE_REPLACE       0
#define MODE_MODULATE      1
#define MODE_ADD           2
#define MODE_ADD_SIGNED    3
#define MODE_INTERPOLATE   4
#define MODE_SUBTRACT      5
#define MODE_DOT3_RGB      6
#define MODE_DOT3_RGB_EXT  7
#define MODE_DOT3_RGBA     8
#define MODE_DOT3_RGBA_EXT 9
#define MODE_MODULATE_ADD_ATI           10
#define MODE_MODULATE_SIGNED_ADD_ATI    11
#define MODE_MODULATE_SUBTRACT_ATI      12
#define MODE_UNKNOWN       15

static GLuint translate_mode( GLenum mode )
{
   switch (mode) {
   case GL_REPLACE: return MODE_REPLACE;
   case GL_MODULATE: return MODE_MODULATE;
   case GL_ADD: return MODE_ADD;
   case GL_ADD_SIGNED: return MODE_ADD_SIGNED;
   case GL_INTERPOLATE: return MODE_INTERPOLATE;
   case GL_SUBTRACT: return MODE_SUBTRACT;
   case GL_DOT3_RGB: return MODE_DOT3_RGB;
   case GL_DOT3_RGB_EXT: return MODE_DOT3_RGB_EXT;
   case GL_DOT3_RGBA: return MODE_DOT3_RGBA;
   case GL_DOT3_RGBA_EXT: return MODE_DOT3_RGBA_EXT;
   case GL_MODULATE_ADD_ATI: return MODE_MODULATE_ADD_ATI;
   case GL_MODULATE_SIGNED_ADD_ATI: return MODE_MODULATE_SIGNED_ADD_ATI;
   case GL_MODULATE_SUBTRACT_ATI: return MODE_MODULATE_SUBTRACT_ATI;
   default: return MODE_UNKNOWN;
   }
}

#define TEXTURE_UNKNOWN_INDEX 7
static GLuint translate_tex_src_bit( GLbitfield bit )
{
   switch (bit) {
   case TEXTURE_1D_BIT:   return TEXTURE_1D_INDEX;
   case TEXTURE_2D_BIT:   return TEXTURE_2D_INDEX;
   case TEXTURE_RECT_BIT: return TEXTURE_RECT_INDEX;
   case TEXTURE_3D_BIT:   return TEXTURE_3D_INDEX;
   case TEXTURE_CUBE_BIT: return TEXTURE_CUBE_INDEX;
   default: return TEXTURE_UNKNOWN_INDEX;
   }
}

/**
 * Examine current texture environment state and generate a unique
 * key to identify it.
 */
static void make_state_key( GLcontext *ctx,  struct state_key *key )
{
   GLuint i, j;
	
   memset(key, 0, sizeof(*key));

   for (i=0;i<MAX_TEXTURE_UNITS;i++) {
      const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[i];

      if (!texUnit->_ReallyEnabled || !texUnit->Enabled)
         continue;

      key->unit[i].enabled = 1;
      key->enabled_units |= (1<<i);

      key->unit[i].source_index = 
	 translate_tex_src_bit(texUnit->_ReallyEnabled);		
      key->unit[i].shadow = texUnit->_Current->CompareMode == GL_COMPARE_R_TO_TEXTURE;

      key->unit[i].NumArgsRGB = texUnit->_CurrentCombine->_NumArgsRGB;
      key->unit[i].NumArgsA = texUnit->_CurrentCombine->_NumArgsA;

      key->unit[i].ModeRGB =
	 translate_mode(texUnit->_CurrentCombine->ModeRGB);
      key->unit[i].ModeA =
	 translate_mode(texUnit->_CurrentCombine->ModeA);
		
      key->unit[i].ScaleShiftRGB = texUnit->_CurrentCombine->ScaleShiftRGB;
      key->unit[i].ScaleShiftA = texUnit->_CurrentCombine->ScaleShiftA;

      for (j=0;j<3;j++) {
         key->unit[i].OptRGB[j].Operand =
	    translate_operand(texUnit->_CurrentCombine->OperandRGB[j]);
         key->unit[i].OptA[j].Operand =
	    translate_operand(texUnit->_CurrentCombine->OperandA[j]);
         key->unit[i].OptRGB[j].Source =
	    translate_source(texUnit->_CurrentCombine->SourceRGB[j]);
         key->unit[i].OptA[j].Source =
	    translate_source(texUnit->_CurrentCombine->SourceA[j]);
      }
   }
	
   if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR)
      key->separate_specular = 1;

   if (ctx->Fog.Enabled) {
      key->fog_enabled = 1;
      key->fog_mode = translate_fog_mode(ctx->Fog.Mode);
   }
}

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
   GLuint idx:8;
   GLuint negatebase:1;
   GLuint abs:1;
   GLuint negateabs:1;
   GLuint swz:12;
   GLuint pad:5;
};

static const struct ureg undef = { 
   PROGRAM_UNDEFINED,
   ~0,
   0,
   0,
   0,
   0,
   0
};


/* State used to build the fragment program:
 */
struct texenv_fragment_program {
   struct gl_fragment_program *program;
   GLcontext *ctx;
   struct state_key *state;

   GLbitfield alu_temps;	/* Track texture indirections, see spec. */
   GLbitfield temps_output;	/* Track texture indirections, see spec. */
   GLbitfield temp_in_use;	/* Tracks temporary regs which are in use. */
   GLboolean error;

   struct ureg src_texture[MAX_TEXTURE_UNITS];   
   /* Reg containing each texture unit's sampled texture color,
    * else undef.
    */

   struct ureg src_previous;	/* Reg containing color from previous 
				 * stage.  May need to be decl'd.
				 */

   GLuint last_tex_stage;	/* Number of last enabled texture unit */

   struct ureg half;
   struct ureg one;
   struct ureg zero;
};



static struct ureg make_ureg(GLuint file, GLuint idx)
{
   struct ureg reg;
   reg.file = file;
   reg.idx = idx;
   reg.negatebase = 0;
   reg.abs = 0;
   reg.negateabs = 0;
   reg.swz = SWIZZLE_NOOP;
   reg.pad = 0;
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

static struct ureg negate( struct ureg reg )
{
   reg.negatebase ^= 1;
   return reg;
}

static GLboolean is_undef( struct ureg reg )
{
   return reg.file == PROGRAM_UNDEFINED;
}


static struct ureg get_temp( struct texenv_fragment_program *p )
{
   GLint bit;
   
   /* First try and reuse temps which have been used already:
    */
   bit = _mesa_ffs( ~p->temp_in_use & p->alu_temps );

   /* Then any unused temporary:
    */
   if (!bit)
      bit = _mesa_ffs( ~p->temp_in_use );

   if (!bit) {
      _mesa_problem(NULL, "%s: out of temporaries\n", __FILE__);
      _mesa_exit(1);
   }

   if ((GLuint) bit > p->program->Base.NumTemporaries)
      p->program->Base.NumTemporaries = bit;

   p->temp_in_use |= 1<<(bit-1);
   return make_ureg(PROGRAM_TEMPORARY, (bit-1));
}

static struct ureg get_tex_temp( struct texenv_fragment_program *p )
{
   int bit;
   
   /* First try to find availble temp not previously used (to avoid
    * starting a new texture indirection).  According to the spec, the
    * ~p->temps_output isn't necessary, but will keep it there for
    * now:
    */
   bit = _mesa_ffs( ~p->temp_in_use & ~p->alu_temps & ~p->temps_output );

   /* Then any unused temporary:
    */
   if (!bit) 
      bit = _mesa_ffs( ~p->temp_in_use );

   if (!bit) {
      _mesa_problem(NULL, "%s: out of temporaries\n", __FILE__);
      _mesa_exit(1);
   }

   if ((GLuint) bit > p->program->Base.NumTemporaries)
      p->program->Base.NumTemporaries = bit;

   p->temp_in_use |= 1<<(bit-1);
   return make_ureg(PROGRAM_TEMPORARY, (bit-1));
}


static void release_temps( struct texenv_fragment_program *p )
{
   GLuint max_temp = p->ctx->Const.FragmentProgram.MaxTemps;

   /* KW: To support tex_env_crossbar, don't release the registers in
    * temps_output.
    */
   if (max_temp >= sizeof(int) * 8)
      p->temp_in_use = p->temps_output;
   else
      p->temp_in_use = ~((1<<max_temp)-1) | p->temps_output;
}


static struct ureg register_param5( struct texenv_fragment_program *p, 
				    GLint s0,
				    GLint s1,
				    GLint s2,
				    GLint s3,
				    GLint s4)
{
   gl_state_index tokens[STATE_LENGTH];
   GLuint idx;
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


static struct ureg register_input( struct texenv_fragment_program *p, GLuint input )
{
   p->program->Base.InputsRead |= (1 << input);
   return make_ureg(PROGRAM_INPUT, input);
}


static void emit_arg( struct prog_src_register *reg,
		      struct ureg ureg )
{
   reg->File = ureg.file;
   reg->Index = ureg.idx;
   reg->Swizzle = ureg.swz;
   reg->NegateBase = ureg.negatebase ? 0xf : 0x0;
   reg->Abs = ureg.abs;
   reg->NegateAbs = ureg.negateabs;
}

static void emit_dst( struct prog_dst_register *dst,
		      struct ureg ureg, GLuint mask )
{
   dst->File = ureg.file;
   dst->Index = ureg.idx;
   dst->WriteMask = mask;
   dst->CondMask = COND_TR;  /* always pass cond test */
   dst->CondSwizzle = SWIZZLE_NOOP;
}

static struct prog_instruction *
emit_op(struct texenv_fragment_program *p,
	enum prog_opcode op,
	struct ureg dest,
	GLuint mask,
	GLboolean saturate,
	struct ureg src0,
	struct ureg src1,
	struct ureg src2 )
{
   GLuint nr = p->program->Base.NumInstructions++;
   struct prog_instruction *inst = &p->program->Base.Instructions[nr];

   assert(nr < MAX_INSTRUCTIONS);

   _mesa_init_instructions(inst, 1);
   inst->Opcode = op;
   
   emit_arg( &inst->SrcReg[0], src0 );
   emit_arg( &inst->SrcReg[1], src1 );
   emit_arg( &inst->SrcReg[2], src2 );
   
   inst->SaturateMode = saturate ? SATURATE_ZERO_ONE : SATURATE_OFF;

   emit_dst( &inst->DstReg, dest, mask );

   /* Accounting for indirection tracking:
    */
   if (dest.file == PROGRAM_TEMPORARY)
      p->temps_output |= 1 << dest.idx;

   return inst;
}
   

static struct ureg emit_arith( struct texenv_fragment_program *p,
			       enum prog_opcode op,
			       struct ureg dest,
			       GLuint mask,
			       GLboolean saturate,
			       struct ureg src0,
			       struct ureg src1,
			       struct ureg src2 )
{
   emit_op(p, op, dest, mask, saturate, src0, src1, src2);
   
   /* Accounting for indirection tracking:
    */
   if (src0.file == PROGRAM_TEMPORARY)
      p->alu_temps |= 1 << src0.idx;

   if (!is_undef(src1) && src1.file == PROGRAM_TEMPORARY)
      p->alu_temps |= 1 << src1.idx;

   if (!is_undef(src2) && src2.file == PROGRAM_TEMPORARY)
      p->alu_temps |= 1 << src2.idx;

   if (dest.file == PROGRAM_TEMPORARY)
      p->alu_temps |= 1 << dest.idx;
       
   p->program->Base.NumAluInstructions++;
   return dest;
}

static struct ureg emit_texld( struct texenv_fragment_program *p,
			       enum prog_opcode op,
			       struct ureg dest,
			       GLuint destmask,
			       GLuint tex_unit,
			       GLuint tex_idx,
			       struct ureg coord )
{
   struct prog_instruction *inst = emit_op( p, op, 
					  dest, destmask, 
					  GL_FALSE,	/* don't saturate? */
					  coord, 	/* arg 0? */
					  undef,
					  undef);
   
   inst->TexSrcTarget = tex_idx;
   inst->TexSrcUnit = tex_unit;

   p->program->Base.NumTexInstructions++;

   /* Is this a texture indirection?
    */
   if ((coord.file == PROGRAM_TEMPORARY &&
	(p->temps_output & (1<<coord.idx))) ||
       (dest.file == PROGRAM_TEMPORARY &&
	(p->alu_temps & (1<<dest.idx)))) {
      p->program->Base.NumTexIndirections++;
      p->temps_output = 1<<coord.idx;
      p->alu_temps = 0;
      assert(0);		/* KW: texture env crossbar */
   }

   return dest;
}


static struct ureg register_const4f( struct texenv_fragment_program *p, 
				     GLfloat s0,
				     GLfloat s1,
				     GLfloat s2,
				     GLfloat s3)
{
   GLfloat values[4];
   GLuint idx, swizzle;
   values[0] = s0;
   values[1] = s1;
   values[2] = s2;
   values[3] = s3;
   idx = _mesa_add_unnamed_constant( p->program->Base.Parameters, values, 4,
                                     &swizzle );
   ASSERT(swizzle == SWIZZLE_NOOP);
   return make_ureg(PROGRAM_STATE_VAR, idx);
}

#define register_scalar_const(p, s0)    register_const4f(p, s0, s0, s0, s0)
#define register_const1f(p, s0)         register_const4f(p, s0, 0, 0, 1)
#define register_const2f(p, s0, s1)     register_const4f(p, s0, s1, 0, 1)
#define register_const3f(p, s0, s1, s2) register_const4f(p, s0, s1, s2, 1)


static struct ureg get_one( struct texenv_fragment_program *p )
{
   if (is_undef(p->one)) 
      p->one = register_scalar_const(p, 1.0);
   return p->one;
}

static struct ureg get_half( struct texenv_fragment_program *p )
{
   if (is_undef(p->half)) 
      p->half = register_scalar_const(p, 0.5);
   return p->half;
}

static struct ureg get_zero( struct texenv_fragment_program *p )
{
   if (is_undef(p->zero)) 
      p->zero = register_scalar_const(p, 0.0);
   return p->zero;
}


static void program_error( struct texenv_fragment_program *p, const char *msg )
{
   _mesa_problem(NULL, msg);
   p->error = 1;
}

static struct ureg get_source( struct texenv_fragment_program *p, 
			       GLuint src, GLuint unit )
{
   switch (src) {
   case SRC_TEXTURE: 
      assert(!is_undef(p->src_texture[unit]));
      return p->src_texture[unit];

   case SRC_TEXTURE0:
   case SRC_TEXTURE1:
   case SRC_TEXTURE2:
   case SRC_TEXTURE3:
   case SRC_TEXTURE4:
   case SRC_TEXTURE5:
   case SRC_TEXTURE6:
   case SRC_TEXTURE7: 
      assert(!is_undef(p->src_texture[src - SRC_TEXTURE0]));
      return p->src_texture[src - SRC_TEXTURE0];

   case SRC_CONSTANT:
      return register_param2(p, STATE_TEXENV_COLOR, unit);

   case SRC_PRIMARY_COLOR:
      return register_input(p, FRAG_ATTRIB_COL0);

   case SRC_PREVIOUS:
   default:
      if (is_undef(p->src_previous))
	 return register_input(p, FRAG_ATTRIB_COL0);
      else
	 return p->src_previous;
   }
}

static struct ureg emit_combine_source( struct texenv_fragment_program *p, 
					GLuint mask,
					GLuint unit,
					GLuint source, 
					GLuint operand )
{
   struct ureg arg, src, one;

   src = get_source(p, source, unit);

   switch (operand) {
   case OPR_ONE_MINUS_SRC_COLOR: 
      /* Get unused tmp,
       * Emit tmp = 1.0 - arg.xyzw
       */
      arg = get_temp( p );
      one = get_one( p );
      return emit_arith( p, OPCODE_SUB, arg, mask, 0, one, src, undef);

   case OPR_SRC_ALPHA: 
      if (mask == WRITEMASK_W)
	 return src;
      else
	 return swizzle1( src, SWIZZLE_W );
   case OPR_ONE_MINUS_SRC_ALPHA: 
      /* Get unused tmp,
       * Emit tmp = 1.0 - arg.wwww
       */
      arg = get_temp(p);
      one = get_one(p);
      return emit_arith(p, OPCODE_SUB, arg, mask, 0,
			one, swizzle1(src, SWIZZLE_W), undef);
   case OPR_ZERO:
      return get_zero(p);
   case OPR_ONE:
      return get_one(p);
   case OPR_SRC_COLOR: 
   default:
      return src;
   }
}

static GLboolean args_match( struct state_key *key, GLuint unit )
{
   GLuint i, nr = key->unit[unit].NumArgsRGB;

   for (i = 0 ; i < nr ; i++) {
      if (key->unit[unit].OptA[i].Source != key->unit[unit].OptRGB[i].Source) 
	 return GL_FALSE;

      switch(key->unit[unit].OptA[i].Operand) {
      case OPR_SRC_ALPHA: 
	 switch(key->unit[unit].OptRGB[i].Operand) {
	 case OPR_SRC_COLOR: 
	 case OPR_SRC_ALPHA: 
	    break;
	 default:
	    return GL_FALSE;
	 }
	 break;
      case OPR_ONE_MINUS_SRC_ALPHA: 
	 switch(key->unit[unit].OptRGB[i].Operand) {
	 case OPR_ONE_MINUS_SRC_COLOR: 
	 case OPR_ONE_MINUS_SRC_ALPHA: 
	    break;
	 default:
	    return GL_FALSE;
	 }
	 break;
      default: 
	 return GL_FALSE;	/* impossible */
      }
   }

   return GL_TRUE;
}

static struct ureg emit_combine( struct texenv_fragment_program *p,
				 struct ureg dest,
				 GLuint mask,
				 GLboolean saturate,
				 GLuint unit,
				 GLuint nr,
				 GLuint mode,
				 const struct mode_opt *opt)
{
   struct ureg src[3];
   struct ureg tmp, half;
   GLuint i;

   tmp = undef; /* silence warning (bug 5318) */

   for (i = 0; i < nr; i++)
      src[i] = emit_combine_source( p, mask, unit, opt[i].Source, opt[i].Operand );

   switch (mode) {
   case MODE_REPLACE: 
      if (mask == WRITEMASK_XYZW && !saturate)
	 return src[0];
      else
	 return emit_arith( p, OPCODE_MOV, dest, mask, saturate, src[0], undef, undef );
   case MODE_MODULATE: 
      return emit_arith( p, OPCODE_MUL, dest, mask, saturate,
			 src[0], src[1], undef );
   case MODE_ADD: 
      return emit_arith( p, OPCODE_ADD, dest, mask, saturate, 
			 src[0], src[1], undef );
   case MODE_ADD_SIGNED:
      /* tmp = arg0 + arg1
       * result = tmp - .5
       */
      half = get_half(p);
      tmp = get_temp( p );
      emit_arith( p, OPCODE_ADD, tmp, mask, 0, src[0], src[1], undef );
      emit_arith( p, OPCODE_SUB, dest, mask, saturate, tmp, half, undef );
      return dest;
   case MODE_INTERPOLATE: 
      /* Arg0 * (Arg2) + Arg1 * (1-Arg2) -- note arguments are reordered:
       */
      return emit_arith( p, OPCODE_LRP, dest, mask, saturate, src[2], src[0], src[1] );

   case MODE_SUBTRACT: 
      return emit_arith( p, OPCODE_SUB, dest, mask, saturate, src[0], src[1], undef );

   case MODE_DOT3_RGBA:
   case MODE_DOT3_RGBA_EXT: 
   case MODE_DOT3_RGB_EXT:
   case MODE_DOT3_RGB: {
      struct ureg tmp0 = get_temp( p );
      struct ureg tmp1 = get_temp( p );
      struct ureg neg1 = register_scalar_const(p, -1);
      struct ureg two  = register_scalar_const(p, 2);

      /* tmp0 = 2*src0 - 1
       * tmp1 = 2*src1 - 1
       *
       * dst = tmp0 dot3 tmp1 
       */
      emit_arith( p, OPCODE_MAD, tmp0, WRITEMASK_XYZW, 0, 
		  two, src[0], neg1);

      if (_mesa_memcmp(&src[0], &src[1], sizeof(struct ureg)) == 0)
	 tmp1 = tmp0;
      else
	 emit_arith( p, OPCODE_MAD, tmp1, WRITEMASK_XYZW, 0, 
		     two, src[1], neg1);
      emit_arith( p, OPCODE_DP3, dest, mask, saturate, tmp0, tmp1, undef);
      return dest;
   }
   case MODE_MODULATE_ADD_ATI:
      /* Arg0 * Arg2 + Arg1 */
      return emit_arith( p, OPCODE_MAD, dest, mask, saturate,
			 src[0], src[2], src[1] );
   case MODE_MODULATE_SIGNED_ADD_ATI: {
      /* Arg0 * Arg2 + Arg1 - 0.5 */
      struct ureg tmp0 = get_temp(p);
      half = get_half(p);
      emit_arith( p, OPCODE_MAD, tmp0, mask, 0, src[0], src[2], src[1] );
      emit_arith( p, OPCODE_SUB, dest, mask, saturate, tmp0, half, undef );
      return dest;
   }
   case MODE_MODULATE_SUBTRACT_ATI:
      /* Arg0 * Arg2 - Arg1 */
      emit_arith( p, OPCODE_MAD, dest, mask, 0, src[0], src[2], negate(src[1]) );
      return dest;
   default: 
      return src[0];
   }
}


/**
 * Generate instructions for one texture unit's env/combiner mode.
 */
static struct ureg
emit_texenv(struct texenv_fragment_program *p, GLuint unit)
{
   struct state_key *key = p->state;
   GLboolean saturate = (unit < p->last_tex_stage);
   GLuint rgb_shift, alpha_shift;
   struct ureg out, shift;
   struct ureg dest;

   if (!key->unit[unit].enabled) {
      return get_source(p, SRC_PREVIOUS, 0);
   }
   
   switch (key->unit[unit].ModeRGB) {
   case MODE_DOT3_RGB_EXT:
      alpha_shift = key->unit[unit].ScaleShiftA;
      rgb_shift = 0;
      break;
   case MODE_DOT3_RGBA_EXT:
      alpha_shift = 0;
      rgb_shift = 0;
      break;
   default:
      rgb_shift = key->unit[unit].ScaleShiftRGB;
      alpha_shift = key->unit[unit].ScaleShiftA;
      break;
   }
   
   /* If this is the very last calculation, emit direct to output reg:
    */
   if (key->separate_specular ||
       unit != p->last_tex_stage ||
       alpha_shift ||
       rgb_shift)
      dest = get_temp( p );
   else
      dest = make_ureg(PROGRAM_OUTPUT, FRAG_RESULT_COLR);

   /* Emit the RGB and A combine ops
    */
   if (key->unit[unit].ModeRGB == key->unit[unit].ModeA &&
       args_match(key, unit)) {
      out = emit_combine( p, dest, WRITEMASK_XYZW, saturate,
			  unit,
			  key->unit[unit].NumArgsRGB,
			  key->unit[unit].ModeRGB,
			  key->unit[unit].OptRGB);
   }
   else if (key->unit[unit].ModeRGB == MODE_DOT3_RGBA_EXT ||
	    key->unit[unit].ModeRGB == MODE_DOT3_RGBA) {

      out = emit_combine( p, dest, WRITEMASK_XYZW, saturate,
			  unit,
			  key->unit[unit].NumArgsRGB,
			  key->unit[unit].ModeRGB,
			  key->unit[unit].OptRGB);
   }
   else {
      /* Need to do something to stop from re-emitting identical
       * argument calculations here:
       */
      out = emit_combine( p, dest, WRITEMASK_XYZ, saturate,
			  unit,
			  key->unit[unit].NumArgsRGB,
			  key->unit[unit].ModeRGB,
			  key->unit[unit].OptRGB);
      out = emit_combine( p, dest, WRITEMASK_W, saturate,
			  unit,
			  key->unit[unit].NumArgsA,
			  key->unit[unit].ModeA,
			  key->unit[unit].OptA);
   }

   /* Deal with the final shift:
    */
   if (alpha_shift || rgb_shift) {
      if (rgb_shift == alpha_shift) {
	 shift = register_scalar_const(p, 1<<rgb_shift);
      }
      else {
	 shift = register_const4f(p, 
				  1<<rgb_shift,
				  1<<rgb_shift,
				  1<<rgb_shift,
				  1<<alpha_shift);
      }
      return emit_arith( p, OPCODE_MUL, dest, WRITEMASK_XYZW, 
			 saturate, out, shift, undef );
   }
   else
      return out;
}


/**
 * Generate instruction for getting a texture source term.
 */
static void load_texture( struct texenv_fragment_program *p, GLuint unit )
{
   if (is_undef(p->src_texture[unit])) {
      GLuint dim = p->state->unit[unit].source_index;
      struct ureg texcoord = register_input(p, FRAG_ATTRIB_TEX0+unit);
      struct ureg tmp = get_tex_temp( p );

      if (dim == TEXTURE_UNKNOWN_INDEX)
         program_error(p, "TexSrcBit");
			  
      /* TODO: Use D0_MASK_XY where possible.
       */
      if (p->state->unit[unit].enabled) {
	 p->src_texture[unit] = emit_texld( p, OPCODE_TXP,
					    tmp, WRITEMASK_XYZW, 
					    unit, dim, texcoord );
	 if (p->state->unit[unit].shadow)
	    p->program->Base.ShadowSamplers |= 1 << unit;
      } else
	 p->src_texture[unit] = get_zero(p);
   }
}

static GLboolean load_texenv_source( struct texenv_fragment_program *p, 
				     GLuint src, GLuint unit )
{
   switch (src) {
   case SRC_TEXTURE:
      load_texture(p, unit);
      break;

   case SRC_TEXTURE0:
   case SRC_TEXTURE1:
   case SRC_TEXTURE2:
   case SRC_TEXTURE3:
   case SRC_TEXTURE4:
   case SRC_TEXTURE5:
   case SRC_TEXTURE6:
   case SRC_TEXTURE7:       
      load_texture(p, src - SRC_TEXTURE0);
      break;
      
   default:
      break;
   }
 
   return GL_TRUE;
}


/**
 * Generate instructions for loading all texture source terms.
 */
static GLboolean
load_texunit_sources( struct texenv_fragment_program *p, int unit )
{
   struct state_key *key = p->state;
   GLuint i;

   for (i = 0; i < key->unit[unit].NumArgsRGB; i++) {
      load_texenv_source( p, key->unit[unit].OptRGB[i].Source, unit);
   }

   for (i = 0; i < key->unit[unit].NumArgsA; i++) {
      load_texenv_source( p, key->unit[unit].OptA[i].Source, unit );
   }

   return GL_TRUE;
}


/**
 * Generate a new fragment program which implements the context's
 * current texture env/combine mode.
 */
static void
create_new_program(GLcontext *ctx, struct state_key *key,
                   struct gl_fragment_program *program)
{
   struct prog_instruction instBuffer[MAX_INSTRUCTIONS];
   struct texenv_fragment_program p;
   GLuint unit;
   struct ureg cf, out;

   _mesa_memset(&p, 0, sizeof(p));
   p.ctx = ctx;
   p.state = key;
   p.program = program;

   /* During code generation, use locally-allocated instruction buffer,
    * then alloc dynamic storage below.
    */
   p.program->Base.Instructions = instBuffer;
   p.program->Base.Target = GL_FRAGMENT_PROGRAM_ARB;
   p.program->Base.NumTexIndirections = 1;	/* correct? */
   p.program->Base.NumTexInstructions = 0;
   p.program->Base.NumAluInstructions = 0;
   p.program->Base.String = NULL;
   p.program->Base.NumInstructions =
   p.program->Base.NumTemporaries =
   p.program->Base.NumParameters =
   p.program->Base.NumAttributes = p.program->Base.NumAddressRegs = 0;
   p.program->Base.Parameters = _mesa_new_parameter_list();

   p.program->Base.InputsRead = 0;
   p.program->Base.OutputsWritten = 1 << FRAG_RESULT_COLR;

   for (unit = 0; unit < MAX_TEXTURE_UNITS; unit++)
      p.src_texture[unit] = undef;

   p.src_previous = undef;
   p.half = undef;
   p.zero = undef;
   p.one = undef;

   p.last_tex_stage = 0;
   release_temps(&p);

   if (key->enabled_units) {
      /* First pass - to support texture_env_crossbar, first identify
       * all referenced texture sources and emit texld instructions
       * for each:
       */
      for (unit = 0 ; unit < ctx->Const.MaxTextureUnits ; unit++)
	 if (key->unit[unit].enabled) {
	    load_texunit_sources( &p, unit );
	    p.last_tex_stage = unit;
	 }

      /* Second pass - emit combine instructions to build final color:
       */
      for (unit = 0 ; unit < ctx->Const.MaxTextureUnits; unit++)
	 if (key->enabled_units & (1<<unit)) {
	    p.src_previous = emit_texenv( &p, unit );
	    release_temps(&p);	/* release all temps */
	 }
   }

   cf = get_source( &p, SRC_PREVIOUS, 0 );
   out = make_ureg( PROGRAM_OUTPUT, FRAG_RESULT_COLR );

   if (key->separate_specular) {
      /* Emit specular add.
       */
      struct ureg s = register_input(&p, FRAG_ATTRIB_COL1);
      emit_arith( &p, OPCODE_ADD, out, WRITEMASK_XYZ, 0, cf, s, undef );
      emit_arith( &p, OPCODE_MOV, out, WRITEMASK_W, 0, cf, undef, undef );
   }
   else if (_mesa_memcmp(&cf, &out, sizeof(cf)) != 0) {
      /* Will wind up in here if no texture enabled or a couple of
       * other scenarios (GL_REPLACE for instance).
       */
      emit_arith( &p, OPCODE_MOV, out, WRITEMASK_XYZW, 0, cf, undef, undef );
   }

   /* Finish up:
    */
   emit_arith( &p, OPCODE_END, undef, WRITEMASK_XYZW, 0, undef, undef, undef);

   if (key->fog_enabled) {
      /* Pull fog mode from GLcontext, the value in the state key is
       * a reduced value and not what is expected in FogOption
       */
      p.program->FogOption = ctx->Fog.Mode;
   } else
      p.program->FogOption = GL_NONE;

   if (p.program->Base.NumTexIndirections > ctx->Const.FragmentProgram.MaxTexIndirections) 
      program_error(&p, "Exceeded max nr indirect texture lookups");

   if (p.program->Base.NumTexInstructions > ctx->Const.FragmentProgram.MaxTexInstructions)
      program_error(&p, "Exceeded max TEX instructions");

   if (p.program->Base.NumAluInstructions > ctx->Const.FragmentProgram.MaxAluInstructions)
      program_error(&p, "Exceeded max ALU instructions");

   ASSERT(p.program->Base.NumInstructions <= MAX_INSTRUCTIONS);

   /* Allocate final instruction array */
   program->Base.Instructions
      = _mesa_alloc_instructions(program->Base.NumInstructions);
   if (!program->Base.Instructions) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY,
                  "generating tex env program");
      return;
   }
   _mesa_copy_instructions(program->Base.Instructions, instBuffer,
                           program->Base.NumInstructions);

   /* Notify driver the fragment program has (actually) changed.
    */
   if (ctx->Driver.ProgramStringNotify) {
      ctx->Driver.ProgramStringNotify( ctx, GL_FRAGMENT_PROGRAM_ARB, 
                                       &p.program->Base );
   }

   if (DISASSEM) {
      _mesa_print_program(&p.program->Base);
      _mesa_printf("\n");
   }
}


static struct gl_fragment_program *
search_cache(const struct texenvprog_cache *cache,
             GLuint hash,
             const void *key,
             GLuint keysize)
{
   struct texenvprog_cache_item *c;

   for (c = cache->items[hash % cache->size]; c; c = c->next) {
      if (c->hash == hash && memcmp(c->key, key, keysize) == 0)
	 return c->data;
   }

   return NULL;
}

static void rehash( struct texenvprog_cache *cache )
{
   struct texenvprog_cache_item **items;
   struct texenvprog_cache_item *c, *next;
   GLuint size, i;

   size = cache->size * 3;
   items = (struct texenvprog_cache_item**) _mesa_malloc(size * sizeof(*items));
   _mesa_memset(items, 0, size * sizeof(*items));

   for (i = 0; i < cache->size; i++)
      for (c = cache->items[i]; c; c = next) {
	 next = c->next;
	 c->next = items[c->hash % size];
	 items[c->hash % size] = c;
      }

   _mesa_free(cache->items);
   cache->items = items;
   cache->size = size;
}

static void clear_cache(GLcontext *ctx, struct texenvprog_cache *cache)
{
   struct texenvprog_cache_item *c, *next;
   GLuint i;

   for (i = 0; i < cache->size; i++) {
      for (c = cache->items[i]; c; c = next) {
	 next = c->next;
	 _mesa_free(c->key);
         _mesa_reference_fragprog(ctx, &c->data, NULL);
	 _mesa_free(c);
      }
      cache->items[i] = NULL;
   }


   cache->n_items = 0;
}


static void cache_item( GLcontext *ctx,
                        struct texenvprog_cache *cache,
			GLuint hash,
			const struct state_key *key,
                        struct gl_fragment_program *prog)
{
   struct texenvprog_cache_item *c = CALLOC_STRUCT(texenvprog_cache_item);
   c->hash = hash;

   c->key = _mesa_malloc(sizeof(*key));
   memcpy(c->key, key, sizeof(*key));

   c->data = prog;

   if (cache->n_items > cache->size * 1.5) {
      if (cache->size < 1000)
	 rehash(cache);
      else 
	 clear_cache(ctx, cache);
   }

   cache->n_items++;
   c->next = cache->items[hash % cache->size];
   cache->items[hash % cache->size] = c;
}

static GLuint hash_key( const struct state_key *key )
{
   GLuint *ikey = (GLuint *)key;
   GLuint hash = 0, i;

   /* Make a slightly better attempt at a hash function:
    */
   for (i = 0; i < sizeof(*key)/sizeof(*ikey); i++)
   {
      hash += ikey[i];
      hash += (hash << 10);
      hash ^= (hash >> 6);
   }

   return hash;
}


/**
 * If _MaintainTexEnvProgram is set we'll generate a fragment program that
 * implements the current texture env/combine mode.
 * This function generates that program and puts it into effect.
 */
void
_mesa_UpdateTexEnvProgram( GLcontext *ctx )
{
   struct state_key key;
   GLuint hash;
   const struct gl_fragment_program *prev = ctx->FragmentProgram._Current;
	
   ASSERT(ctx->FragmentProgram._MaintainTexEnvProgram);

   /* If a conventional fragment program/shader isn't in effect... */
   if (!ctx->FragmentProgram._Enabled &&
       (!ctx->Shader.CurrentProgram || !ctx->Shader.CurrentProgram->FragmentProgram)) {
      struct gl_fragment_program *newProg;

      make_state_key(ctx, &key);
      hash = hash_key(&key);
      
      newProg = search_cache(&ctx->Texture.env_fp_cache, hash, &key, sizeof(key));

      if (!newProg) {
         /* create new tex env program */

         if (0)
            _mesa_printf("Building new texenv proggy for key %x\n", hash);

         newProg = (struct gl_fragment_program *) 
            ctx->Driver.NewProgram(ctx, GL_FRAGMENT_PROGRAM_ARB, 0);

         create_new_program(ctx, &key, newProg);

         /* Our ownership of newProg is transferred to the cache */
         cache_item(ctx, &ctx->Texture.env_fp_cache, hash, &key, newProg);
      }

      _mesa_reference_fragprog(ctx, &ctx->FragmentProgram._Current, newProg);
      _mesa_reference_fragprog(ctx, &ctx->FragmentProgram._TexEnvProgram, newProg);
   } 
   else {
      /* _Current pointer has been updated in update_program */
      /* ctx->FragmentProgram._Current = ctx->FragmentProgram.Current; */
   }

   /* Tell the driver about the change.  Could define a new target for
    * this?
    */
   if (ctx->FragmentProgram._Current != prev && ctx->Driver.BindProgram) {
      ctx->Driver.BindProgram(ctx, GL_FRAGMENT_PROGRAM_ARB,
                         (struct gl_program *) ctx->FragmentProgram._Current);
   }
}


void _mesa_TexEnvProgramCacheInit( GLcontext *ctx )
{
   ctx->Texture.env_fp_cache.ctx = ctx;
   ctx->Texture.env_fp_cache.size = 17;
   ctx->Texture.env_fp_cache.n_items = 0;
   ctx->Texture.env_fp_cache.items = (struct texenvprog_cache_item **)
      _mesa_calloc(ctx->Texture.env_fp_cache.size * 
		   sizeof(struct texenvprog_cache_item));
}


void _mesa_TexEnvProgramCacheDestroy( GLcontext *ctx )
{
   clear_cache(ctx, &ctx->Texture.env_fp_cache);
   _mesa_free(ctx->Texture.env_fp_cache.items);
}
