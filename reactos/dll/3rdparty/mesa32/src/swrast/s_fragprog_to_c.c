/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* An amusing little utility to print ARB fragment programs out as a C
 * function.   Resulting code not tested except visually.
 */


#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "nvfragprog.h"
#include "macros.h"
#include "program.h"

#include "s_nvfragprog.h"
#include "s_span.h"
#include "s_texture.h"


#ifdef USE_TCC

/* UREG - a way of representing an FP source register including
 * swizzling and negation in a single GLuint.  Major flaw is the
 * limitiation to source->Index < 32.  Secondary flaw is the fact that
 * it's overkill & we could probably just pass around the original
 * datatypes instead.
 */

#define UREG_TYPE_TEMP              0
#define UREG_TYPE_INTERP            1
#define UREG_TYPE_LOCAL_CONST       2
#define UREG_TYPE_ENV_CONST         3
#define UREG_TYPE_STATE_CONST       4
#define UREG_TYPE_PARAM             5
#define UREG_TYPE_OUTPUT            6
#define UREG_TYPE_MASK              0x7

#define UREG_TYPE_SHIFT               29
#define UREG_NR_SHIFT                 24
#define UREG_NR_MASK                  0x1f /* 31 */
#define UREG_CHANNEL_X_NEGATE_SHIFT   23
#define UREG_CHANNEL_X_SHIFT          20
#define UREG_CHANNEL_Y_NEGATE_SHIFT   19
#define UREG_CHANNEL_Y_SHIFT          16
#define UREG_CHANNEL_Z_NEGATE_SHIFT   15
#define UREG_CHANNEL_Z_SHIFT          12
#define UREG_CHANNEL_W_NEGATE_SHIFT   11
#define UREG_CHANNEL_W_SHIFT          8
#define UREG_CHANNEL_ZERO_NEGATE_MBZ  5
#define UREG_CHANNEL_ZERO_SHIFT       4
#define UREG_CHANNEL_ONE_NEGATE_MBZ   1
#define UREG_CHANNEL_ONE_SHIFT        0

#define UREG_BAD          0xffffffff /* not a valid ureg */

#define _X    0
#define _Y    1
#define _Z    2
#define _W    3
#define _ZERO 4			/* NOTE! */
#define _ONE  5			/* NOTE! */


/* Construct a ureg:
 */
#define UREG( type, nr ) (((type)<< UREG_TYPE_SHIFT) |		\
			  ((nr)  << UREG_NR_SHIFT) |		\
			  (_X     << UREG_CHANNEL_X_SHIFT) |	\
			  (_Y     << UREG_CHANNEL_Y_SHIFT) |	\
			  (_Z     << UREG_CHANNEL_Z_SHIFT) |	\
			  (_W     << UREG_CHANNEL_W_SHIFT) |	\
			  (_ZERO  << UREG_CHANNEL_ZERO_SHIFT) |	\
			  (_ONE   << UREG_CHANNEL_ONE_SHIFT))

#define GET_CHANNEL_SRC( reg, channel ) ((reg<<(channel*4)) & \
                                         (0xf<<UREG_CHANNEL_X_SHIFT))
#define CHANNEL_SRC( src, channel ) (src>>(channel*4))

#define GET_UREG_TYPE(reg) (((reg)>>UREG_TYPE_SHIFT)&UREG_TYPE_MASK)
#define GET_UREG_NR(reg)   (((reg)>>UREG_NR_SHIFT)&UREG_NR_MASK)



#define UREG_XYZW_CHANNEL_MASK 0x00ffff00

#define deref(reg,pos) swizzle(reg, pos, pos, pos, pos)


static INLINE int is_swizzled( int reg )
{
   return ((reg & UREG_XYZW_CHANNEL_MASK) !=
	   (UREG(0,0) & UREG_XYZW_CHANNEL_MASK));
}


/* One neat thing about the UREG representation:
 */
static INLINE int swizzle( int reg, int x, int y, int z, int w )
{
   return ((reg & ~UREG_XYZW_CHANNEL_MASK) |
	   CHANNEL_SRC( GET_CHANNEL_SRC( reg, x ), 0 ) |
	   CHANNEL_SRC( GET_CHANNEL_SRC( reg, y ), 1 ) |
	   CHANNEL_SRC( GET_CHANNEL_SRC( reg, z ), 2 ) |
	   CHANNEL_SRC( GET_CHANNEL_SRC( reg, w ), 3 ));
}

/* Another neat thing about the UREG representation:
 */
static INLINE int negate( int reg, int x, int y, int z, int w )
{
   return reg ^ (((x&1)<<UREG_CHANNEL_X_NEGATE_SHIFT)|
		 ((y&1)<<UREG_CHANNEL_Y_NEGATE_SHIFT)|
		 ((z&1)<<UREG_CHANNEL_Z_NEGATE_SHIFT)|
		 ((w&1)<<UREG_CHANNEL_W_NEGATE_SHIFT));
}



static GLuint src_reg_file( GLuint file )
{
   switch (file) {
   case PROGRAM_TEMPORARY: return UREG_TYPE_TEMP;
   case PROGRAM_INPUT: return UREG_TYPE_INTERP;
   case PROGRAM_LOCAL_PARAM: return UREG_TYPE_LOCAL_CONST;
   case PROGRAM_ENV_PARAM: return UREG_TYPE_ENV_CONST;

   case PROGRAM_STATE_VAR: return UREG_TYPE_STATE_CONST;
   case PROGRAM_NAMED_PARAM: return UREG_TYPE_PARAM;
   default: return UREG_BAD;
   }
}

static void emit( struct fragment_program *p,
		  const char *fmt,
		  ... )
{
   va_list ap;
   va_start( ap, fmt );

   if (p->c_strlen < sizeof(p->c_str))
      p->c_strlen += vsnprintf( p->c_str + p->c_strlen,
				sizeof(p->c_str) - p->c_strlen,
				fmt, ap );

   va_end( ap );
}

static INLINE void emit_char( struct fragment_program *p, char c )
{
   if (p->c_strlen < sizeof(p->c_str)) {
       p->c_str[p->c_strlen] = c;
       p->c_strlen++;
   }
}


/**
 * Retrieve a ureg for the given source register.  Will emit
 * constants, apply swizzling and negation as needed.
 */
static GLuint src_vector( const struct fp_src_register *source )
{
   GLuint src;

   assert(source->Index < 32);	/* limitiation of UREG representation */

   src = UREG( src_reg_file( source->File ), source->Index );

   src = swizzle(src,
		 _X + source->Swizzle[0],
		 _X + source->Swizzle[1],
		 _X + source->Swizzle[2],
		 _X + source->Swizzle[3]);

   if (source->NegateBase)
      src = negate( src, 1,1,1,1 );

   return src;
}


static void print_header( struct fragment_program *p )
{
   emit(p, "\n\n\n");

   /* Mesa's program_parameter struct:
    */
   emit(p,
	"struct program_parameter\n"
	"{\n"
	"   const char *Name;\n"
	"   int Type;\n"
	"   int StateIndexes[6];\n"
	"   float Values[4];\n"
	"};\n");


   /* Texture samplers, not written yet:
    */
   emit(p, "extern void TEX( void *ctx, const float *txc, int unit, float *rslt );\n"
	  "extern void TXB( void *ctx, const float *txc, int unit, float *rslt );\n"
	  "extern void TXP( void *ctx, const float *txc, int unit, float *rslt );\n");

   /* Resort to the standard math library (float versions):
    */
   emit(p, "extern float fabsf( float );\n"
	  "extern float cosf( float );\n"
	  "extern float sinf( float );\n"
	  "extern float expf( float );\n"
	  "extern float powf( float, float );\n"
	  "extern float floorf( float );\n");

   /* These ones we have fast code in Mesa for:
    */
   emit(p, "extern float LOG2( float );\n"
	  "extern float _mesa_inv_sqrtf( float );\n");

   /* The usual macros, not really needed, but handy:
    */
   emit(p, "#define MIN2(x,y) ((x)<(y)?(x):(y))\n"
	  "#define MAX2(x,y) ((x)<(y)?(x):(y))\n"
	  "#define SATURATE(x) ((x)>1.0?1.0:((x)<0.0?0.0:(x)))\n");

   /* Our function!
    */
   emit(p, "int run_program( void *ctx, \n"
	  "                  const float (*local_param)[4], \n"
	  "                  const float (*env_param)[4], \n"
	  "                  const struct program_parameter *state_param, \n"
	  "                  const float (*interp)[4], \n"
	  "                  float (*outputs)[4])\n"
	  "{\n"
	  "   float temp[32][4];\n"
      );
}

static void print_footer( struct fragment_program *p )
{
   emit(p, "   return 1;");
   emit(p, "}\n");
}

static void print_dest_reg( struct fragment_program *p,
			    const struct fp_instruction *inst )
{
   switch (inst->DstReg.File) {
   case PROGRAM_OUTPUT:
      emit(p, "outputs[%d]", inst->DstReg.Index);
      break;
   case PROGRAM_TEMPORARY:
      emit(p, "temp[%d]", inst->DstReg.Index);
      break;
   default:
      break;
   }
}

static void print_dest( struct fragment_program *p,
			const struct fp_instruction *inst,
			GLuint idx )
{
   print_dest_reg(p, inst);
   emit(p, "[%d]", idx);
}


#define UREG_SRC0(reg) (((reg)>>UREG_CHANNEL_X_SHIFT) & 0x7)

static void print_reg( struct fragment_program *p,
		       GLuint arg )
{
   switch (GET_UREG_TYPE(arg)) {
   case UREG_TYPE_TEMP: emit(p, "temp"); break;
   case UREG_TYPE_INTERP: emit(p, "interp"); break;
   case UREG_TYPE_LOCAL_CONST: emit(p, "local_const"); break;
   case UREG_TYPE_ENV_CONST: emit(p, "env_const"); break;
   case UREG_TYPE_STATE_CONST: emit(p, "state_param"); break;
   case UREG_TYPE_PARAM: emit(p, "local_param"); break;
   };

   emit(p, "[%d]", GET_UREG_NR(arg));

   if (GET_UREG_TYPE(arg) == UREG_TYPE_STATE_CONST) {
      emit(p, ".Values");
   }
}


static void print_arg( struct fragment_program *p,
		       GLuint arg )
{
   GLuint src = UREG_SRC0(arg);

   if (src == _ZERO) {
      emit(p, "0");
      return;
   }

   if (arg & (1<<UREG_CHANNEL_X_NEGATE_SHIFT))
      emit(p, "-");

   if (src == _ONE) {
      emit(p, "1");
      return;
   }

   if (GET_UREG_TYPE(arg) == UREG_TYPE_STATE_CONST &&
       p->Parameters->Parameters[GET_UREG_NR(arg)].Type == CONSTANT) {
      emit(p, "%g", p->Parameters->Parameters[GET_UREG_NR(arg)].Values[src]);
      return;
   }

   print_reg( p, arg );

   switch (src){
   case _X: emit(p, "[0]"); break;
   case _Y: emit(p, "[1]"); break;
   case _Z: emit(p, "[2]"); break;
   case _W: emit(p, "[3]"); break;
   }
}


/* This is where the handling of expressions breaks down into string
 * processing:
 */
static void print_expression( struct fragment_program *p,
			      GLuint i,
			      const char *fmt,
			      va_list ap )
{
   while (*fmt) {
      if (*fmt == '%' && *(fmt+1) == 's') {
	 int reg = va_arg(ap, int);

	 /* Use of deref() is a bit of a hack:
	  */
	 print_arg( p, deref(reg, i) );
	 fmt += 2;
      }
      else {
	 emit_char(p, *fmt);
	 fmt++;
      }
   }

   emit(p, ";\n");
}

static void do_tex_kill( struct fragment_program *p,
			 const struct fp_instruction *inst,
			 GLuint arg )
{
   GLuint i;

   emit(p, "if (");

   for (i = 0; i < 4; i++) {
      print_arg( p, deref(arg, i) );
      emit(p, " < 0 ");
      if (i + 1 < 4)
	 emit(p, "|| ");
   }

   emit(p, ")\n");
   emit(p, "           return 0;\n");

}

static void do_tex_simple( struct fragment_program *p,
			   const struct fp_instruction *inst,
			   const char *fn, GLuint texunit, GLuint arg )
{
   emit(p, "   %s( ctx, ", fn);
   print_reg( p, arg );
   emit(p, ", %d, ", texunit );
   print_dest_reg(p, inst);
   emit(p, ");\n");
}


static void do_tex( struct fragment_program *p,
		    const struct fp_instruction *inst,
		    const char *fn, GLuint texunit, GLuint arg )
{
   GLuint i;
   GLboolean need_tex = GL_FALSE, need_result = GL_FALSE;

   for (i = 0; i < 4; i++)
      if (!inst->DstReg.WriteMask[i])
	 need_result = GL_TRUE;

   if (is_swizzled(arg))
      need_tex = GL_TRUE;

   if (!need_tex && !need_result) {
      do_tex_simple( p, inst, fn, texunit, arg );
      return;
   }

   emit(p, "   {\n");
   emit(p, "       float texcoord[4];\n");
   emit(p, "       float result[4];\n");

   for (i = 0; i < 4; i++) {
      emit(p, "      texcoord[%d] = ", i);
      print_arg( p, deref(arg, i) );
      emit(p, ";\n");
   }

   emit(p, "       %s( ctx, texcoord, %d, result);\n", fn, texunit );

   for (i = 0; i < 4; i++) {
      if (inst->DstReg.WriteMask[i]) {
	 emit(p, "      ");
	 print_dest(p, inst, i);
	 emit(p, " = result[%d];\n", i);
      }
   }

   emit(p, "   }\n");
}


static void saturate( struct fragment_program *p,
		      const struct fp_instruction *inst,
		      GLuint i )
{
   emit(p, "   ");
   print_dest(p, inst, i);
   emit(p, " = SATURATE( ");
   print_dest(p, inst, i);
   emit(p, ");\n");
}

static void assign_single( GLuint i,
			   struct fragment_program *p,
			   const struct fp_instruction *inst,
			   const char *fmt,
			   ... )
{
   va_list ap;
   va_start( ap, fmt );

   if (inst->DstReg.WriteMask[i]) {
      emit(p, "   ");
      print_dest(p, inst, i);
      emit(p, " = ");
      print_expression( p, i, fmt, ap);
      if (inst->Saturate)
	 saturate(p, inst, i);
   }

   va_end( ap );
}

static void assign4( struct fragment_program *p,
		     const struct fp_instruction *inst,
		     const char *fmt,
		     ... )
{
   GLuint i;
   va_list ap;
   va_start( ap, fmt );

   for (i = 0; i < 4; i++)
      if (inst->DstReg.WriteMask[i]) {
	 emit(p, "   ");
	 print_dest(p, inst, i);
	 emit(p, " = ");
	 print_expression( p, i, fmt, ap);
	 if (inst->Saturate)
	    saturate(p, inst, i);
      }

   va_end( ap );
}

static void assign4_replicate( struct fragment_program *p,
			       const struct fp_instruction *inst,
			       const char *fmt,
			       ... )
{
   GLuint i, first = 0;
   GLboolean ok = 0;
   va_list ap;

   for (i = 0; i < 4; i++)
      if (inst->DstReg.WriteMask[i]) {
	 ok = 1;
	 first = i;
	 break;
      }

   if (!ok) return;

   va_start( ap, fmt );

   emit(p, "   ");

   print_dest(p, inst, first);
   emit(p, " = ");
   print_expression( p, 0, fmt, ap);
   if (inst->Saturate)
      saturate(p, inst, first);
   va_end( ap );

   for (i = first+1; i < 4; i++)
      if (inst->DstReg.WriteMask[i]) {
	 emit(p, "   ");
	 print_dest(p, inst, i);
	 emit(p, " = ");
	 print_dest(p, inst, first);
	 emit(p, ";\n");
      }
}




static GLuint nr_args( GLuint opcode )
{
   switch (opcode) {
   case FP_OPCODE_ABS: return 1;
   case FP_OPCODE_ADD: return 2;
   case FP_OPCODE_CMP: return 3;
   case FP_OPCODE_COS: return 1;
   case FP_OPCODE_DP3: return 2;
   case FP_OPCODE_DP4: return 2;
   case FP_OPCODE_DPH: return 2;
   case FP_OPCODE_DST: return 2;
   case FP_OPCODE_EX2: return 1;
   case FP_OPCODE_FLR: return 1;
   case FP_OPCODE_FRC: return 1;
   case FP_OPCODE_KIL: return 1;
   case FP_OPCODE_LG2: return 1;
   case FP_OPCODE_LIT: return 1;
   case FP_OPCODE_LRP: return 3;
   case FP_OPCODE_MAD: return 3;
   case FP_OPCODE_MAX: return 2;
   case FP_OPCODE_MIN: return 2;
   case FP_OPCODE_MOV: return 1;
   case FP_OPCODE_MUL: return 2;
   case FP_OPCODE_POW: return 2;
   case FP_OPCODE_RCP: return 1;
   case FP_OPCODE_RSQ: return 1;
   case FP_OPCODE_SCS: return 1;
   case FP_OPCODE_SGE: return 2;
   case FP_OPCODE_SIN: return 1;
   case FP_OPCODE_SLT: return 2;
   case FP_OPCODE_SUB: return 2;
   case FP_OPCODE_SWZ: return 1;
   case FP_OPCODE_TEX: return 1;
   case FP_OPCODE_TXB: return 1;
   case FP_OPCODE_TXP: return 1;
   case FP_OPCODE_XPD: return 2;
   default: return 0;
   }
}



static void translate_program( struct fragment_program *p )
{
   const struct fp_instruction *inst = p->Instructions;

   for (; inst->Opcode != FP_OPCODE_END; inst++) {

      GLuint src[3], i;
      GLuint nr = nr_args( inst->Opcode );

      for (i = 0; i < nr; i++)
	 src[i] = src_vector( &inst->SrcReg[i] );

      /* Print the original program instruction string */
      if (p->Base.String)
      {
         const char *s = (const char *) p->Base.String + inst->StringPos;
         emit(p, "   /* ");
         while (*s != ';') {
            emit_char(p, *s);
            s++;
         }
         emit(p, "; */\n");
      }

      switch (inst->Opcode) {
      case FP_OPCODE_ABS:
	 assign4(p, inst, "fabsf(%s)", src[0]);
	 break;

      case FP_OPCODE_ADD:
	 assign4(p, inst, "%s + %s", src[0], src[1]);
	 break;

      case FP_OPCODE_CMP:
	 assign4(p, inst, "%s < 0.0F ? %s : %s", src[0], src[1], src[2]);
	 break;

      case FP_OPCODE_COS:
	 assign4_replicate(p, inst, "COS(%s)", src[0]);
	 break;

      case FP_OPCODE_DP3:
	 assign4_replicate(p, inst,
			   "%s*%s + %s*%s + %s*%s",
			   deref(src[0],_X),
			   deref(src[1],_X),
			   deref(src[0],_Y),
			   deref(src[1],_Y),
			   deref(src[0],_Z),
			   deref(src[1],_Z));
	 break;

      case FP_OPCODE_DP4:
	 assign4_replicate(p, inst,
			   "%s*%s + %s*%s + %s*%s + %s*%s",
			   deref(src[0],_X),
			   deref(src[1],_X),
			   deref(src[0],_Y),
			   deref(src[1],_Y),
			   deref(src[0],_Z),
			   deref(src[1],_Z));
	 break;

      case FP_OPCODE_DPH:
	 assign4_replicate(p, inst,
			   "%s*%s + %s*%s + %s*%s + %s",
			   deref(src[0],_X),
			   deref(src[1],_X),
			   deref(src[0],_Y),
			   deref(src[1],_Y),
			   deref(src[1],_Z));
	 break;

      case FP_OPCODE_DST:
	 /* result[0] = 1    * 1;
	  * result[1] = a[1] * b[1];
	  * result[2] = a[2] * 1;
	  * result[3] = 1    * b[3];
	  */
	 assign_single(0, p, inst, "1.0");

	 assign_single(1, p, inst, "%s * %s",
		       deref(src[0], _Y), deref(src[1], _Y));

	 assign_single(2, p, inst, "%s", deref(src[0], _Z));
	 assign_single(3, p, inst, "%s", deref(src[1], _W));
	 break;

      case FP_OPCODE_EX2:
	 assign4_replicate(p, inst, "powf(2.0, %s)", src[0]);
	 break;

      case FP_OPCODE_FLR:
	 assign4_replicate(p, inst, "floorf(%s)", src[0]);
	 break;

      case FP_OPCODE_FRC:
	 assign4_replicate(p, inst, "%s - floorf(%s)", src[0], src[0]);
	 break;

      case FP_OPCODE_KIL:
	 do_tex_kill(p, inst, src[0]);
	 break;

      case FP_OPCODE_LG2:
	 assign4_replicate(p, inst, "LOG2(%s)", src[0]);
	 break;

      case FP_OPCODE_LIT:
	 assign_single(0, p, inst, "1.0");
	 assign_single(1, p, inst, "MIN2(%s, 0)", deref(src[0], _X));
	 assign_single(2, p, inst, "(%s > 0.0) ? expf(%s * MIN2(%s, 0)) : 0.0",
		       deref(src[0], _X),
		       deref(src[0], _Z),
		       deref(src[0], _Y));
	 assign_single(3, p, inst, "1.0");
	 break;

      case FP_OPCODE_LRP:
	 assign4(p, inst,
		 "%s * %s + (1.0 - %s) * %s",
		 src[0], src[1], src[0], src[2]);
	 break;

      case FP_OPCODE_MAD:
	 assign4(p, inst, "%s * %s + %s", src[0], src[1], src[2]);
	 break;

      case FP_OPCODE_MAX:
	 assign4(p, inst, "MAX2(%s, %s)", src[0], src[1]);
	 break;

      case FP_OPCODE_MIN:
	 assign4(p, inst, "MIN2(%s, %s)", src[0], src[1]);
	 break;

      case FP_OPCODE_MOV:
	 assign4(p, inst, "%s", src[0]);
	 break;

      case FP_OPCODE_MUL:
	 assign4(p, inst, "%s * %s", src[0], src[1]);
	 break;

      case FP_OPCODE_POW:
	 assign4_replicate(p, inst, "powf(%s, %s)", src[0], src[1]);
	 break;

      case FP_OPCODE_RCP:
	 assign4_replicate(p, inst, "1.0/%s", src[0]);
	 break;

      case FP_OPCODE_RSQ:
	 assign4_replicate(p, inst, "_mesa_inv_sqrtf(%s)", src[0]);
	 break;

      case FP_OPCODE_SCS:
	 if (inst->DstReg.WriteMask[0]) {
	    assign_single(0, p, inst, "cosf(%s)", deref(src[0], _X));
	 }

	 if (inst->DstReg.WriteMask[1]) {
	    assign_single(1, p, inst, "sinf(%s)", deref(src[0], _X));
	 }
	 break;

      case FP_OPCODE_SGE:
	 assign4(p, inst, "%s >= %s ? 1.0 : 0.0", src[0], src[1]);
	 break;

      case FP_OPCODE_SIN:
	 assign4_replicate(p, inst, "sinf(%s)", src[0]);
	 break;

      case FP_OPCODE_SLT:
	 assign4(p, inst, "%s < %s ? 1.0 : 0.0", src[0], src[1]);
	 break;

      case FP_OPCODE_SUB:
	 assign4(p, inst, "%s - %s", src[0], src[1]);
	 break;

      case FP_OPCODE_SWZ: 	/* same implementation as MOV: */
	 assign4(p, inst, "%s", src[0]);
	 break;

      case FP_OPCODE_TEX:
	 do_tex(p, inst, "TEX", inst->TexSrcUnit, src[0]);
	 break;

      case FP_OPCODE_TXB:
	 do_tex(p, inst, "TXB", inst->TexSrcUnit, src[0]);
	 break;

      case FP_OPCODE_TXP:
	 do_tex(p, inst, "TXP", inst->TexSrcUnit, src[0]);
	 break;

      case FP_OPCODE_XPD:
	 /* Cross product:
	  *      result.x = src[0].y * src[1].z - src[0].z * src[1].y;
	  *      result.y = src[0].z * src[1].x - src[0].x * src[1].z;
	  *      result.z = src[0].x * src[1].y - src[0].y * src[1].x;
	  *      result.w = undef;
	  */
	 assign4(p, inst,
		 "%s * %s - %s * %s",
		 swizzle(src[0], _Y, _Z, _X, _ONE),
		 swizzle(src[1], _Z, _X, _Y, _ONE),
		 swizzle(src[0], _Z, _X, _Y, _ONE),
		 swizzle(src[1], _Y, _Z, _X, _ONE));
	 break;

      default:
	 emit(p, "BOGUS OPCODE\n");
	 return;
      }
   }
}





void _swrast_translate_program( GLcontext *ctx )
{
   struct fragment_program *p = ctx->FragmentProgram._Current;

   if (p) {
      p->c_strlen = 0;

      print_header( p );
      translate_program( p );
      print_footer( p );
   }
}

#endif /*USE_TCC*/
