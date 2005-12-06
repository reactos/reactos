/*
 * Mesa 3-D graphics library
 * Version:  6.3
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

/**
 * \file t_vb_arb_program_sse.c
 *
 * Translate simplified vertex_program representation to
 * x86/x87/SSE/SSE2 machine code using mesa's rtasm runtime assembler.
 *
 * This is very much a first attempt - build something that works.
 * There are probably better approaches for applying SSE to vertex
 * programs, and the whole thing is crying out for static analysis of
 * the programs to avoid redundant operations.
 *
 * \author Keith Whitwell
 */

#include "glheader.h"
#include "context.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"
#include "arbprogparse.h"
#include "program.h"
#include "math/m_matrix.h"
#include "math/m_translate.h"
#include "t_context.h"
#include "t_vb_arbprogram.h"

#if defined(USE_SSE_ASM)

#include "x86/rtasm/x86sse.h"
#include "x86/common_x86_asm.h"

#define X    0
#define Y    1
#define Z    2
#define W    3

/* Reg usage:
 *
 * EAX - temp
 * EBX - point to 'm->File[0]'
 * ECX - point to 'm->File[3]'
 * EDX - holds 'm'
 * EBP,
 * ESI,
 * EDI
 */

#define DISASSEM 0

#define FAIL								\
do {									\
   _mesa_printf("x86 translation failed in %s\n", __FUNCTION__);	\
   return GL_FALSE;							\
} while (0)

struct compilation {
   struct x86_function func;
   struct tnl_compiled_program *p;   
   GLuint insn_counter;

   struct {
      GLuint file:2;
      GLuint idx:7;
      GLuint dirty:1;
      GLuint last_used:10;
   } xmm[8];

   struct {
      struct x86_reg base;
   } file[4];

   GLboolean have_sse2;
   GLshort fpucntl;
};

static INLINE GLboolean eq( struct x86_reg a,
			    struct x86_reg b )
{
   return (a.file == b.file &&
	   a.idx == b.idx &&
	   a.mod == b.mod &&
	   a.disp == b.disp);
}
      
static GLint get_offset( const void *a, const void *b )
{
   return (const char *)b - (const char *)a;
}


static struct x86_reg get_reg_ptr(GLuint file,
				  GLuint idx )
{
   struct x86_reg reg;

   switch (file) {
   case FILE_REG:
      reg = x86_make_reg(file_REG32, reg_BX);
      assert(idx != REG_UNDEF);
      break;
   case FILE_STATE_PARAM:
      reg = x86_make_reg(file_REG32, reg_CX);
      break;
   default:
      assert(0);
   }

   return x86_make_disp(reg, 16 * idx);
}
			  

static void spill( struct compilation *cp, GLuint idx )
{
   struct x86_reg oldval = get_reg_ptr(cp->xmm[idx].file,
				       cp->xmm[idx].idx);

   assert(cp->xmm[idx].dirty);
   sse_movups(&cp->func, oldval, x86_make_reg(file_XMM, idx));
   cp->xmm[idx].dirty = 0;
}

static struct x86_reg get_xmm_reg( struct compilation *cp )
{
   GLuint i;
   GLuint oldest = 0;

   for (i = 0; i < 8; i++) 
      if (cp->xmm[i].last_used < cp->xmm[oldest].last_used)
	 oldest = i;

   /* Need to write out the old value?
    */
   if (cp->xmm[oldest].dirty) 
      spill(cp, oldest);

   assert(cp->xmm[oldest].last_used != cp->insn_counter);

   cp->xmm[oldest].file = FILE_REG;
   cp->xmm[oldest].idx = REG_UNDEF;
   cp->xmm[oldest].last_used = cp->insn_counter;
   return x86_make_reg(file_XMM, oldest);
}

static void invalidate_xmm( struct compilation *cp, 
			    GLuint file, GLuint idx )
{
   GLuint i;

   /* Invalidate any old copy of this register in XMM0-7.  
    */
   for (i = 0; i < 8; i++) {
      if (cp->xmm[i].file == file && cp->xmm[i].idx == idx) {
	 cp->xmm[i].file = FILE_REG;
	 cp->xmm[i].idx = REG_UNDEF;
	 cp->xmm[i].dirty = 0;
	 break;
      }
   }
}
      

/* Return an XMM reg to receive the results of an operation.
 */
static struct x86_reg get_dst_xmm_reg( struct compilation *cp, 
				       GLuint file, GLuint idx )
{
   struct x86_reg reg;

   /* Invalidate any old copy of this register in XMM0-7.  Don't reuse
    * as this may be one of the arguments.
    */
   invalidate_xmm( cp, file, idx );

   reg = get_xmm_reg( cp );
   cp->xmm[reg.idx].file = file;
   cp->xmm[reg.idx].idx = idx;
   cp->xmm[reg.idx].dirty = 1;
   return reg;   
}

/* As above, but return a pointer.  Note - this pointer may alias
 * those returned by get_arg_ptr().
 */
static struct x86_reg get_dst_ptr( struct compilation *cp, 
				   GLuint file, GLuint idx )
{
   /* Invalidate any old copy of this register in XMM0-7.  Don't reuse
    * as this may be one of the arguments.
    */
   invalidate_xmm( cp, file, idx );

   return get_reg_ptr(file, idx);
}



/* Return an XMM reg if the argument is resident, otherwise return a
 * base+offset pointer to the saved value.
 */
static struct x86_reg get_arg( struct compilation *cp, GLuint file, GLuint idx )
{
   GLuint i;

   for (i = 0; i < 8; i++) {
      if (cp->xmm[i].file == file &&
	  cp->xmm[i].idx == idx) {
	 cp->xmm[i].last_used = cp->insn_counter;
	 return x86_make_reg(file_XMM, i);
      }
   }

   return get_reg_ptr(file, idx);
}

/* As above, but always return a pointer:
 */
static struct x86_reg get_arg_ptr( struct compilation *cp, GLuint file, GLuint idx )
{
   GLuint i;

   /* If there is a modified version of this register in one of the
    * XMM regs, write it out to memory.
    */
   for (i = 0; i < 8; i++) {
      if (cp->xmm[i].file == file && 
	  cp->xmm[i].idx == idx &&
	  cp->xmm[i].dirty) 
	 spill(cp, i);
   }

   return get_reg_ptr(file, idx);
}

/* Emulate pshufd insn in regular SSE, if necessary:
 */
static void emit_pshufd( struct compilation *cp,
			 struct x86_reg dst,
			 struct x86_reg arg0,
			 GLubyte shuf )
{
   if (cp->have_sse2) {
      sse2_pshufd(&cp->func, dst, arg0, shuf);
      cp->func.fn = 0;
   }
   else {
      if (!eq(dst, arg0)) 
	 sse_movups(&cp->func, dst, arg0);

      sse_shufps(&cp->func, dst, dst, shuf);
   }
}

static void set_fpu_round_neg_inf( struct compilation *cp )
{
   if (cp->fpucntl != RND_NEG_FPU) {
      struct x86_reg regEDX = x86_make_reg(file_REG32, reg_DX);
      struct arb_vp_machine *m = NULL;

      cp->fpucntl = RND_NEG_FPU;
      x87_fnclex(&cp->func);
      x87_fldcw(&cp->func, x86_make_disp(regEDX, get_offset(m, &m->fpucntl_rnd_neg)));
   }
}


/* Perform a reduced swizzle.  
 */
static GLboolean emit_RSW( struct compilation *cp, union instruction op ) 
{
   struct x86_reg arg0 = get_arg(cp, op.rsw.file0, op.rsw.idx0);
   struct x86_reg dst = get_dst_xmm_reg(cp, FILE_REG, op.rsw.dst);
   GLuint swz = op.rsw.swz;
   GLuint neg = op.rsw.neg;

   emit_pshufd(cp, dst, arg0, swz);
   
   if (neg) {
      struct x86_reg negs = get_arg(cp, FILE_REG, REG_SWZ);
      struct x86_reg tmp = get_xmm_reg(cp);
      /* Load 1,-1,0,0
       * Use neg as arg to pshufd
       * Multiply
       */
      emit_pshufd(cp, tmp, negs, 
		  SHUF((neg & 1) ? 1 : 0,
		       (neg & 2) ? 1 : 0,
		       (neg & 4) ? 1 : 0,
		       (neg & 8) ? 1 : 0));
      sse_mulps(&cp->func, dst, tmp);
   }

   return GL_TRUE;
}

/* Helper for writemask:
 */
static GLboolean emit_shuf_copy1( struct compilation *cp,
				  struct x86_reg dst,
				  struct x86_reg arg0,
				  struct x86_reg arg1,
				  GLubyte shuf )
{
   struct x86_reg tmp = get_xmm_reg(cp);
   sse_movups(&cp->func, dst, arg1);
   emit_pshufd(cp, dst, dst, shuf);
   emit_pshufd(cp, tmp, arg0, shuf);

   sse_movss(&cp->func, dst, tmp);

   emit_pshufd(cp, dst, dst, shuf);
   return GL_TRUE;
}


/* Helper for writemask:
 */
static GLboolean emit_shuf_copy2( struct compilation *cp,
				  struct x86_reg dst,
				  struct x86_reg arg0,
				  struct x86_reg arg1,
				  GLubyte shuf )
{
   struct x86_reg tmp = get_xmm_reg(cp);
   emit_pshufd(cp, dst, arg1, shuf);
   emit_pshufd(cp, tmp, arg0, shuf);

   sse_shufps(&cp->func, dst, tmp, SHUF(X, Y, Z, W));

   emit_pshufd(cp, dst, dst, shuf);
   return GL_TRUE;
}


static void emit_x87_ex2( struct compilation *cp )
{
   struct x86_reg st0 = x86_make_reg(file_x87, 0);
   struct x86_reg st1 = x86_make_reg(file_x87, 1);
   struct x86_reg st3 = x86_make_reg(file_x87, 3);

   set_fpu_round_neg_inf( cp );

   x87_fld(&cp->func, st0); /* a a */
   x87_fprndint( &cp->func );	/* int(a) a */
   x87_fld(&cp->func, st0); /* int(a) int(a) a */
   x87_fstp(&cp->func, st3); /* int(a) a int(a)*/
   x87_fsubp(&cp->func, st1); /* frac(a) int(a) */
   x87_f2xm1(&cp->func);    /* (2^frac(a))-1 int(a)*/
   x87_fld1(&cp->func);    /* 1 (2^frac(a))-1 int(a)*/
   x87_faddp(&cp->func, st1);	/* 2^frac(a) int(a) */
   x87_fscale(&cp->func);	/* 2^a */
}

#if 0
static GLboolean emit_MSK2( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.msk.file, op.msk.arg);
   struct x86_reg arg1 = get_arg(cp, FILE_REG, op.msk.dst); /* NOTE! */
   struct x86_reg dst = get_dst_xmm_reg(cp, FILE_REG, op.msk.dst);
   
   /* make full width bitmask in tmp 
    * dst = ~tmp
    * tmp &= arg0
    * dst &= arg1
    * dst |= tmp
    */
   emit_pshufd(cp, tmp, get_arg(cp, FILE_REG, REG_NEGS), 
	       SHUF((op.msk.mask & 1) ? 2 : 0,
		    (op.msk.mask & 2) ? 2 : 0,
		    (op.msk.mask & 4) ? 2 : 0,
		    (op.msk.mask & 8) ? 2 : 0));
   sse2_pnot(&cp->func, dst, tmp);
   sse2_pand(&cp->func, arg0, tmp);
   sse2_pand(&cp->func, arg1, dst);
   sse2_por(&cp->func, tmp, dst);
   return GL_TRUE;
}
#endif


/* Used to implement write masking.  This and most of the other instructions
 * here would be easier to implement if there had been a translation
 * to a 2 argument format (dst/arg0, arg1) at the shader level before
 * attempting to translate to x86/sse code.
 */
static GLboolean emit_MSK( struct compilation *cp, union instruction op )
{
   struct x86_reg arg = get_arg(cp, op.msk.file, op.msk.idx);
   struct x86_reg dst0 = get_arg(cp, FILE_REG, op.msk.dst); /* NOTE! */
   struct x86_reg dst = get_dst_xmm_reg(cp, FILE_REG, op.msk.dst);
   
   /* Note that dst and dst0 refer to the same program variable, but
    * will definitely be different XMM registers.  We're effectively
    * treating this as a 2 argument SEL now, just one of which happens
    * always to be the same register as the destination.
    */

   switch (op.msk.mask) {
   case 0:
      sse_movups(&cp->func, dst, dst0);
      return GL_TRUE;

   case WRITEMASK_X:
      if (arg.file == file_XMM) {
	 sse_movups(&cp->func, dst, dst0);
	 sse_movss(&cp->func, dst, arg);
      }
      else {
	 struct x86_reg tmp = get_xmm_reg(cp);
	 sse_movups(&cp->func, dst, dst0);
	 sse_movss(&cp->func, tmp, arg);
	 sse_movss(&cp->func, dst, tmp);
      }
      return GL_TRUE;

   case WRITEMASK_XY:
      sse_movups(&cp->func, dst, dst0);
      sse_shufps(&cp->func, dst, arg, SHUF(X, Y, Z, W));
      return GL_TRUE;

   case WRITEMASK_ZW: 
      sse_movups(&cp->func, dst, arg);
      sse_shufps(&cp->func, dst, dst0, SHUF(X, Y, Z, W));
      return GL_TRUE;

   case WRITEMASK_YZW: 
      if (dst0.file == file_XMM) {
	 sse_movups(&cp->func, dst, arg);
	 sse_movss(&cp->func, dst, dst0);
      }
      else {
	 struct x86_reg tmp = get_xmm_reg(cp);      
	 sse_movups(&cp->func, dst, arg);
	 sse_movss(&cp->func, tmp, dst0);
	 sse_movss(&cp->func, dst, tmp);
      }
      return GL_TRUE;

   case WRITEMASK_Y:
      emit_shuf_copy1(cp, dst, arg, dst0, SHUF(Y,X,Z,W));
      return GL_TRUE;

   case WRITEMASK_Z: 
      emit_shuf_copy1(cp, dst, arg, dst0, SHUF(Z,Y,X,W));
      return GL_TRUE;

   case WRITEMASK_W: 
      emit_shuf_copy1(cp, dst, arg, dst0, SHUF(W,Y,Z,X));
      return GL_TRUE;

   case WRITEMASK_XZ:
      emit_shuf_copy2(cp, dst, arg, dst0, SHUF(X,Z,Y,W));
      return GL_TRUE;

   case WRITEMASK_XW: 
      emit_shuf_copy2(cp, dst, arg, dst0, SHUF(X,W,Z,Y));

   case WRITEMASK_YZ:      
      emit_shuf_copy2(cp, dst, arg, dst0, SHUF(Z,Y,X,W));
      return GL_TRUE;

   case WRITEMASK_YW:
      emit_shuf_copy2(cp, dst, arg, dst0, SHUF(W,Y,Z,X));
      return GL_TRUE;

   case WRITEMASK_XZW:
      emit_shuf_copy1(cp, dst, dst0, arg, SHUF(Y,X,Z,W));
      return GL_TRUE;

   case WRITEMASK_XYW: 
      emit_shuf_copy1(cp, dst, dst0, arg, SHUF(Z,Y,X,W));
      return GL_TRUE;

   case WRITEMASK_XYZ: 
      emit_shuf_copy1(cp, dst, dst0, arg, SHUF(W,Y,Z,X));
      return GL_TRUE;

   case WRITEMASK_XYZW:
      sse_movups(&cp->func, dst, arg);
      return GL_TRUE;      

   default:
      assert(0);
      break;
   }
}



static GLboolean emit_PRT( struct compilation *cp, union instruction op )
{
   FAIL;
}


/**
 * The traditional instructions.  All operate on internal registers
 * and ignore write masks and swizzling issues.
 */

static GLboolean emit_ABS( struct compilation *cp, union instruction op ) 
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg dst = get_dst_xmm_reg(cp, FILE_REG, op.alu.dst);
   struct x86_reg neg = get_reg_ptr(FILE_REG, REG_NEG);

   sse_movups(&cp->func, dst, arg0);
   sse_mulps(&cp->func, dst, neg);
   sse_maxps(&cp->func, dst, arg0);
   return GL_TRUE;
}

static GLboolean emit_ADD( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1);
   struct x86_reg dst = get_dst_xmm_reg(cp, FILE_REG, op.alu.dst);

   sse_movups(&cp->func, dst, arg0);
   sse_addps(&cp->func, dst, arg1);
   return GL_TRUE;
}


/* The dotproduct instructions don't really do that well in sse:
 */
static GLboolean emit_DP3( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1);
   struct x86_reg dst = get_dst_xmm_reg(cp, FILE_REG, op.alu.dst);
   struct x86_reg tmp = get_xmm_reg(cp); 

   sse_movups(&cp->func, dst, arg0);
   sse_mulps(&cp->func, dst, arg1);
   
   /* Now the hard bit: sum the first 3 values:
    */ 
   sse_movhlps(&cp->func, tmp, dst);
   sse_addss(&cp->func, dst, tmp); /* a*x+c*z, b*y, ?, ? */
   emit_pshufd(cp, tmp, dst, SHUF(Y,X,W,Z));
   sse_addss(&cp->func, dst, tmp);
   sse_shufps(&cp->func, dst, dst, SHUF(X, X, X, X));
   return GL_TRUE;
}



static GLboolean emit_DP4( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1);
   struct x86_reg dst = get_dst_xmm_reg(cp, FILE_REG, op.alu.dst);
   struct x86_reg tmp = get_xmm_reg(cp);      

   sse_movups(&cp->func, dst, arg0);
   sse_mulps(&cp->func, dst, arg1);
   
   /* Now the hard bit: sum the values:
    */ 
   sse_movhlps(&cp->func, tmp, dst);
   sse_addps(&cp->func, dst, tmp); /* a*x+c*z, b*y+d*w, a*x+c*z, b*y+d*w */
   emit_pshufd(cp, tmp, dst, SHUF(Y,X,W,Z));
   sse_addss(&cp->func, dst, tmp);
   sse_shufps(&cp->func, dst, dst, SHUF(X, X, X, X));
   return GL_TRUE;
}

static GLboolean emit_DPH( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0); 
   struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1); 
   struct x86_reg dst = get_dst_xmm_reg(cp, FILE_REG, op.alu.dst);
   struct x86_reg ones = get_reg_ptr(FILE_REG, REG_ONES);
   struct x86_reg tmp = get_xmm_reg(cp);      

   emit_pshufd(cp, dst, arg0, SHUF(W,X,Y,Z));
   sse_movss(&cp->func, dst, ones);
   emit_pshufd(cp, dst, dst, SHUF(W,X,Y,Z));
   sse_mulps(&cp->func, dst, arg1);
   
   /* Now the hard bit: sum the values (from DP4):
    */ 
   sse_movhlps(&cp->func, tmp, dst);
   sse_addps(&cp->func, dst, tmp); /* a*x+c*z, b*y+d*w, a*x+c*z, b*y+d*w */
   emit_pshufd(cp, tmp, dst, SHUF(Y,X,W,Z));
   sse_addss(&cp->func, dst, tmp);
   sse_shufps(&cp->func, dst, dst, SHUF(X, X, X, X));
   return GL_TRUE;
}

#if 0
static GLboolean emit_DST( struct compilation *cp, union instruction op )
{
    struct x86_reg arg0 = get_arg_ptr(cp, op.alu.file0, op.alu.idx0); 
    struct x86_reg arg1 = get_arg_ptr(cp, op.alu.file1, op.alu.idx1); 
    struct x86_reg dst = get_dst_ptr(cp, FILE_REG, op.alu.dst); 

/*    dst[0] = 1.0     * 1.0F; */
/*    dst[1] = arg0[1] * arg1[1]; */
/*    dst[2] = arg0[2] * 1.0; */
/*    dst[3] = 1.0     * arg1[3]; */

    /* Would rather do some of this with integer regs, but:
     *  1) No proper support for immediate values yet
     *  2) I'd need to push/pop somewhere to get a free reg.
     */ 
    x87_fld1(&cp->func);
    x87_fstp(&cp->func, dst); /* would rather do an immediate store... */
    x87_fld(&cp->func, x86_make_disp(arg0, 4));
    x87_fmul(&cp->func, x86_make_disp(arg1, 4));
    x87_fstp(&cp->func, x86_make_disp(dst, 4));
    
    if (!eq(arg0, dst)) {
       x86_fld(&cp->func, x86_make_disp(arg0, 8));
       x86_stp(&cp->func, x86_make_disp(dst, 8));
    }

    if (!eq(arg1, dst)) {
       x86_fld(&cp->func, x86_make_disp(arg0, 12));
       x86_stp(&cp->func, x86_make_disp(dst, 12));
    } 

    return GL_TRUE;
}
#else
static GLboolean emit_DST( struct compilation *cp, union instruction op )
{
    struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0); 
    struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1); 
    struct x86_reg dst = get_dst_xmm_reg(cp, FILE_REG, op.alu.dst); 
    struct x86_reg tmp = get_xmm_reg(cp);
    struct x86_reg ones = get_reg_ptr(FILE_REG, REG_ONES);

    emit_shuf_copy2(cp, dst, arg0, ones, SHUF(X,W,Z,Y));
    emit_shuf_copy2(cp, tmp, arg1, ones, SHUF(X,Z,Y,W));
    sse_mulps(&cp->func, dst, tmp);

/*    dst[0] = 1.0     * 1.0F; */
/*    dst[1] = arg0[1] * arg1[1]; */
/*    dst[2] = arg0[2] * 1.0; */
/*    dst[3] = 1.0     * arg1[3]; */

    return GL_TRUE;
}
#endif

static GLboolean emit_LG2( struct compilation *cp, union instruction op ) 
{
   struct x86_reg arg0 = get_arg_ptr(cp, op.alu.file0, op.alu.idx0); 
   struct x86_reg dst = get_dst_ptr(cp, FILE_REG, op.alu.dst); 

   x87_fld1(&cp->func);		/* 1 */
   x87_fld(&cp->func, arg0);	/* a0 1 */
   x87_fyl2x(&cp->func);	/* log2(a0) */
   x87_fst(&cp->func, x86_make_disp(dst, 0));
   x87_fst(&cp->func, x86_make_disp(dst, 4));
   x87_fst(&cp->func, x86_make_disp(dst, 8));
   x87_fstp(&cp->func, x86_make_disp(dst, 12));
   
   return GL_TRUE;
}


static GLboolean emit_EX2( struct compilation *cp, union instruction op ) 
{
   struct x86_reg arg0 = get_arg_ptr(cp, op.alu.file0, op.alu.idx0); 
   struct x86_reg dst = get_dst_ptr(cp, FILE_REG, op.alu.dst);

   /* CAUTION: dst may alias arg0!
    */
   x87_fld(&cp->func, arg0);	

   emit_x87_ex2(cp);

   x87_fst(&cp->func, x86_make_disp(dst, 0));    
   x87_fst(&cp->func, x86_make_disp(dst, 4));    
   x87_fst(&cp->func, x86_make_disp(dst, 8));    
   x87_fst(&cp->func, x86_make_disp(dst, 12));    
   return GL_TRUE;
}

static GLboolean emit_EXP( struct compilation *cp, union instruction op )
{
    struct x86_reg arg0 = get_arg_ptr(cp, op.alu.file0, op.alu.idx0); 
    struct x86_reg dst = get_dst_ptr(cp, FILE_REG, op.alu.dst); 
    struct x86_reg st0 = x86_make_reg(file_x87, 0);
    struct x86_reg st1 = x86_make_reg(file_x87, 1);
    struct x86_reg st3 = x86_make_reg(file_x87, 3);

    /* CAUTION: dst may alias arg0!
     */
    x87_fld(&cp->func, arg0);	/* arg0.x */
    x87_fld(&cp->func, st0); /* arg arg */

    /* by default, fpu is setup to round-to-nearest.  We want to
     * change this now, and track the state through to the end of the
     * generated function so that it isn't repeated unnecessarily.
     * Alternately, could subtract .5 to get round to -inf behaviour.
     */
    set_fpu_round_neg_inf( cp );
    x87_fprndint( &cp->func );	/* flr(a) a */
    x87_fld(&cp->func, st0); /* flr(a) flr(a) a */
    x87_fld1(&cp->func);    /* 1 floor(a) floor(a) a */
    x87_fst(&cp->func, x86_make_disp(dst, 12));  /* stack unchanged */
    x87_fscale(&cp->func);  /* 2^floor(a) floor(a) a */
    x87_fst(&cp->func, st3); /* 2^floor(a) floor(a) a 2^floor(a)*/
    x87_fstp(&cp->func, x86_make_disp(dst, 0)); /* flr(a) a 2^flr(a) */
    x87_fsubrp(&cp->func, st1); /* frac(a) 2^flr(a) */
    x87_fst(&cp->func, x86_make_disp(dst, 4));    /* frac(a) 2^flr(a) */
    x87_f2xm1(&cp->func);    /* (2^frac(a))-1 2^flr(a)*/
    x87_fld1(&cp->func);    /* 1 (2^frac(a))-1 2^flr(a)*/
    x87_faddp(&cp->func, st1);	/* 2^frac(a) 2^flr(a) */
    x87_fmulp(&cp->func, st1);	/* 2^a */
    x87_fst(&cp->func, x86_make_disp(dst, 8));    
    


/*    dst[0] = 2^floor(tmp); */
/*    dst[1] = frac(tmp); */
/*    dst[2] = 2^floor(tmp) * 2^frac(tmp); */
/*    dst[3] = 1.0F; */
    return GL_TRUE;
}

static GLboolean emit_LOG( struct compilation *cp, union instruction op )
{
    struct x86_reg arg0 = get_arg_ptr(cp, op.alu.file0, op.alu.idx0); 
    struct x86_reg dst = get_dst_ptr(cp, FILE_REG, op.alu.dst); 
    struct x86_reg st0 = x86_make_reg(file_x87, 0);
    struct x86_reg st1 = x86_make_reg(file_x87, 1);
    struct x86_reg st2 = x86_make_reg(file_x87, 2);
 
    /* CAUTION: dst may alias arg0!
     */
    x87_fld(&cp->func, arg0);	/* arg0.x */
    x87_fabs(&cp->func);	/* |arg0.x| */
    x87_fxtract(&cp->func);	/* mantissa(arg0.x), exponent(arg0.x) */
    x87_fst(&cp->func, st2);	/* mantissa, exponent, mantissa */
    x87_fld1(&cp->func);	/* 1, mantissa, exponent, mantissa */
    x87_fyl2x(&cp->func); 	/* log2(mantissa), exponent, mantissa */
    x87_fadd(&cp->func, st0, st1);	/* e+l2(m), e, m  */
    x87_fstp(&cp->func, x86_make_disp(dst, 8)); /* e, m */

    x87_fld1(&cp->func);	/* 1, e, m */
    x87_fsub(&cp->func, st1, st0);	/* 1, e-1, m */
    x87_fstp(&cp->func, x86_make_disp(dst, 12)); /* e-1,m */
    x87_fstp(&cp->func, dst);	/* m */

    x87_fadd(&cp->func, st0, st0);	/* 2m */
    x87_fstp(&cp->func, x86_make_disp(dst, 4));	

    return GL_TRUE;
}

static GLboolean emit_FLR( struct compilation *cp, union instruction op ) 
{
   struct x86_reg arg0 = get_arg_ptr(cp, op.alu.file0, op.alu.idx0); 
   struct x86_reg dst = get_dst_ptr(cp, FILE_REG, op.alu.dst); 
   int i;

   set_fpu_round_neg_inf( cp );

   for (i = 0; i < 4; i++) {
      x87_fld(&cp->func, x86_make_disp(arg0, i*4));   
      x87_fprndint( &cp->func );   
      x87_fstp(&cp->func, x86_make_disp(dst, i*4));
   }


   return GL_TRUE;
}

static GLboolean emit_FRC( struct compilation *cp, union instruction op ) 
{
   struct x86_reg arg0 = get_arg_ptr(cp, op.alu.file0, op.alu.idx0); 
   struct x86_reg dst = get_dst_ptr(cp, FILE_REG, op.alu.dst); 
   struct x86_reg st0 = x86_make_reg(file_x87, 0);
   struct x86_reg st1 = x86_make_reg(file_x87, 1);
   int i;

   set_fpu_round_neg_inf( cp );

   /* Knowing liveness info or even just writemask would be useful
    * here:
    */
   for (i = 0; i < 4; i++) {
      x87_fld(&cp->func, x86_make_disp(arg0, i*4));   
      x87_fld(&cp->func, st0);	/* a a */
      x87_fprndint( &cp->func );   /* flr(a) a */
      x87_fsubrp(&cp->func, st1); /* frc(a) */
      x87_fstp(&cp->func, x86_make_disp(dst, i*4));
   }

   return GL_TRUE;
}



static GLboolean emit_LIT( struct compilation *cp, union instruction op )
{
#if 1
   struct x86_reg arg0 = get_arg_ptr(cp, op.alu.file0, op.alu.idx0); 
   struct x86_reg dst = get_dst_ptr(cp, FILE_REG, op.alu.dst); 
   struct x86_reg lit = get_arg(cp, FILE_REG, REG_LIT);
   struct x86_reg tmp = get_xmm_reg(cp);
   struct x86_reg st1 = x86_make_reg(file_x87, 1);
   struct x86_reg regEAX = x86_make_reg(file_REG32, reg_AX);
   GLubyte *fixup1, *fixup2;


   /* Load the interesting parts of arg0:
    */
   x87_fld(&cp->func, x86_make_disp(arg0, 12));	/* a3 */
   x87_fld(&cp->func, x86_make_disp(arg0, 4)); /* a1 a3 */
   x87_fld(&cp->func, x86_make_disp(arg0, 0)); /* a0 a1 a3 */
   
   /* Intialize dst:
    */
   sse_movaps(&cp->func, tmp, lit);
   sse_movaps(&cp->func, dst, tmp);
   
   /* Check arg0[0]:
    */
   x87_fldz(&cp->func);		/* 0 a0 a1 a3 */
   x87_fucomp(&cp->func, st1);	/* a0 a1 a3 */
   x87_fnstsw(&cp->func, regEAX);
   x86_sahf(&cp->func);
   fixup1 = x86_jcc_forward(&cp->func, cc_AE); 
   
   x87_fstp(&cp->func, x86_make_disp(dst, 4));	/* a1 a3 */

   /* Check arg0[1]:
    */ 
   x87_fldz(&cp->func);		/* 0 a1 a3 */
   x87_fucomp(&cp->func, st1);	/* a1 a3 */
   x87_fnstsw(&cp->func, regEAX);
   x86_sahf(&cp->func);
   fixup2 = x86_jcc_forward(&cp->func, cc_AE); 

   /* Compute pow(a1, a3)
    */
   x87_fyl2x(&cp->func);	/* a3*log2(a1) */

   emit_x87_ex2( cp );		/* 2^(a3*log2(a1)) */

   x87_fstp(&cp->func, x86_make_disp(dst, 8));
   
   /* Land jumps:
    */
   x86_fixup_fwd_jump(&cp->func, fixup1);
   x86_fixup_fwd_jump(&cp->func, fixup2);
#else
   struct x86_reg dst = get_dst_xmm_reg(cp, FILE_REG, op.alu.dst); 
   struct x86_reg ones = get_reg_ptr(FILE_REG, REG_LIT);
   sse_movups(&cp->func, dst, ones);
#endif   
   return GL_TRUE;
}



static GLboolean emit_MAX( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1);
   struct x86_reg dst = get_dst_xmm_reg(cp, FILE_REG, op.alu.dst);

   sse_movups(&cp->func, dst, arg0);
   sse_maxps(&cp->func, dst, arg1);
   return GL_TRUE;
}


static GLboolean emit_MIN( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1);
   struct x86_reg dst = get_dst_xmm_reg(cp, FILE_REG, op.alu.dst);

   sse_movups(&cp->func, dst, arg0);
   sse_minps(&cp->func, dst, arg1);
   return GL_TRUE;
}

static GLboolean emit_MOV( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg dst = get_dst_xmm_reg(cp, FILE_REG, op.alu.dst);

   sse_movups(&cp->func, dst, arg0);
   return GL_TRUE;
}

static GLboolean emit_MUL( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1);
   struct x86_reg dst = get_dst_xmm_reg(cp, FILE_REG, op.alu.dst);

   sse_movups(&cp->func, dst, arg0);
   sse_mulps(&cp->func, dst, arg1);
   return GL_TRUE;
}


static GLboolean emit_POW( struct compilation *cp, union instruction op ) 
{
   struct x86_reg arg0 = get_arg_ptr(cp, op.alu.file0, op.alu.idx0); 
   struct x86_reg arg1 = get_arg_ptr(cp, op.alu.file1, op.alu.idx1); 
   struct x86_reg dst = get_dst_ptr(cp, FILE_REG, op.alu.dst);

   x87_fld(&cp->func, arg1);   	/* a1 */
   x87_fld(&cp->func, arg0);	/* a0 a1 */
   x87_fyl2x(&cp->func);	/* a1*log2(a0) */

   emit_x87_ex2( cp );		/* 2^(a1*log2(a0)) */

   x87_fst(&cp->func, x86_make_disp(dst, 0));    
   x87_fst(&cp->func, x86_make_disp(dst, 4));    
   x87_fst(&cp->func, x86_make_disp(dst, 8));    
   x87_fstp(&cp->func, x86_make_disp(dst, 12));    
    
   return GL_TRUE;
}

static GLboolean emit_REL( struct compilation *cp, union instruction op )
{
/*    GLuint idx = (op.alu.idx0 + (GLint)cp->File[0][REG_ADDR][0]) & (MAX_NV_VERTEX_PROGRAM_PARAMS-1); */
/*    GLuint idx = 0; */
/*    struct x86_reg arg0 = get_arg(cp, op.alu.file0, idx); */
/*    struct x86_reg dst = get_dst_xmm_reg(cp, FILE_REG, op.alu.dst); */

/*    dst[0] = arg0[0]; */
/*    dst[1] = arg0[1]; */
/*    dst[2] = arg0[2]; */
/*    dst[3] = arg0[3]; */

   FAIL;
}

static GLboolean emit_RCP( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg dst = get_dst_xmm_reg(cp, FILE_REG, op.alu.dst);

   if (cp->have_sse2) {
      sse2_rcpss(&cp->func, dst, arg0);
   }
   else {
      struct x86_reg ones = get_reg_ptr(FILE_REG, REG_ONES);
      sse_movss(&cp->func, dst, ones);
      sse_divss(&cp->func, dst, arg0);
   }

   sse_shufps(&cp->func, dst, dst, SHUF(X, X, X, X));
   return GL_TRUE;
}

static GLboolean emit_RSQ( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg dst = get_dst_xmm_reg(cp, FILE_REG, op.alu.dst);

   /* TODO: Calculate absolute value
    */
#if 0
   sse_movss(&cp->func, dst, arg0);
   sse_mulss(&cp->func, dst, neg);
   sse_maxss(&cp->func, dst, arg0);
#endif

   sse_rsqrtss(&cp->func, dst, arg0);
   sse_shufps(&cp->func, dst, dst, SHUF(X, X, X, X));
   return GL_TRUE;
}


static GLboolean emit_SGE( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1);
   struct x86_reg dst = get_dst_xmm_reg(cp, FILE_REG, op.alu.dst);
   struct x86_reg ones = get_reg_ptr(FILE_REG, REG_ONES);

   sse_movups(&cp->func, dst, arg0);
   sse_cmpps(&cp->func, dst, arg1, cc_NotLessThan);
   sse_andps(&cp->func, dst, ones);
   return GL_TRUE;
}


static GLboolean emit_SLT( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1);
   struct x86_reg dst = get_dst_xmm_reg(cp, FILE_REG, op.alu.dst);
   struct x86_reg ones = get_reg_ptr(FILE_REG, REG_ONES);
   
   sse_movups(&cp->func, dst, arg0);
   sse_cmpps(&cp->func, dst, arg1, cc_LessThan);
   sse_andps(&cp->func, dst, ones);
   return GL_TRUE;
}

static GLboolean emit_SUB( struct compilation *cp, union instruction op ) 
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1);
   struct x86_reg dst = get_dst_xmm_reg(cp, FILE_REG, op.alu.dst);

   sse_movups(&cp->func, dst, arg0);
   sse_subps(&cp->func, dst, arg1);
   return GL_TRUE;
}


static GLboolean emit_XPD( struct compilation *cp, union instruction op ) 
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1);
   struct x86_reg dst = get_dst_xmm_reg(cp, FILE_REG, op.alu.dst);
   struct x86_reg tmp0 = get_xmm_reg(cp);
   struct x86_reg tmp1 = get_xmm_reg(cp);

   /* Could avoid tmp0, tmp1 if we overwrote arg0, arg1.  Need a way
    * to invalidate registers.  This will come with better analysis
    * (liveness analysis) of the incoming program.
    */
   emit_pshufd(cp, dst, arg0, SHUF(Y, Z, X, W));
   emit_pshufd(cp, tmp1, arg1, SHUF(Z, X, Y, W));
   sse_mulps(&cp->func, dst, tmp1);
   emit_pshufd(cp, tmp0, arg0, SHUF(Z, X, Y, W));
   emit_pshufd(cp, tmp1, arg1, SHUF(Y, Z, X, W));
   sse_mulps(&cp->func, tmp0, tmp1);
   sse_subps(&cp->func, dst, tmp0);

/*    dst[0] = arg0[1] * arg1[2] - arg0[2] * arg1[1]; */
/*    dst[1] = arg0[2] * arg1[0] - arg0[0] * arg1[2]; */
/*    dst[2] = arg0[0] * arg1[1] - arg0[1] * arg1[0]; */
/*    dst[3] is undef */

   return GL_TRUE;
}

static GLboolean emit_NOP( struct compilation *cp, union instruction op ) 
{
   return GL_TRUE;
}


static GLboolean (* const emit_func[])(struct compilation *, union instruction) = 
{
   emit_ABS,
   emit_ADD,
   emit_NOP,
   emit_DP3,
   emit_DP4,
   emit_DPH,
   emit_DST,
   emit_NOP,
   emit_EX2,
   emit_EXP,
   emit_FLR,
   emit_FRC,
   emit_LG2,
   emit_LIT,
   emit_LOG,
   emit_NOP,
   emit_MAX,
   emit_MIN,
   emit_MOV,
   emit_MUL,
   emit_POW,
   emit_PRT,
   emit_NOP,
   emit_RCP,
   emit_RSQ,
   emit_SGE,
   emit_SLT,
   emit_SUB,
   emit_RSW,
   emit_XPD,
   emit_RSW,
   emit_MSK,
   emit_REL,
};



static GLboolean build_vertex_program( struct compilation *cp )
{
   struct arb_vp_machine *m = NULL;
   GLuint j;

   struct x86_reg regEBX = x86_make_reg(file_REG32, reg_BX);
   struct x86_reg regECX = x86_make_reg(file_REG32, reg_CX);
   struct x86_reg regEDX = x86_make_reg(file_REG32, reg_DX);

   x86_push(&cp->func, regEBX);

   x86_mov(&cp->func, regEDX, x86_fn_arg(&cp->func, 1));   
   x86_mov(&cp->func, regEBX, x86_make_disp(regEDX, get_offset(m, m->File + FILE_REG)));
   x86_mov(&cp->func, regECX, x86_make_disp(regEDX, get_offset(m, m->File + FILE_STATE_PARAM)));

   for (j = 0; j < cp->p->nr_instructions; j++) {
      union instruction inst = cp->p->instructions[j];	 
      cp->insn_counter = j+1;	/* avoid zero */
      
      if (DISASSEM) {
	 _mesa_printf("%p: ", cp->func.csr); 
	 _tnl_disassem_vba_insn( inst );
      }
      cp->func.fn = NULL;

      if (!emit_func[inst.alu.opcode]( cp, inst )) {
	 return GL_FALSE;
      }
   }

   /* TODO: only for outputs:
    */
   for (j = 0; j < 8; j++) {
      if (cp->xmm[j].dirty) 
	 spill(cp, j);
   }
      

   /* Exit mmx state?
    */
   if (cp->func.need_emms)
      mmx_emms(&cp->func);

   /* Restore FPU control word?
    */
   if (cp->fpucntl != RESTORE_FPU) {
      x87_fnclex(&cp->func);
      x87_fldcw(&cp->func, x86_make_disp(regEDX, get_offset(m, &m->fpucntl_restore)));
   }

   x86_pop(&cp->func, regEBX);
   x86_ret(&cp->func);

   return GL_TRUE;
}

/**
 * Execute the given vertex program.  
 * 
 * TODO: Integrate the t_vertex.c code here, to build machine vertices
 * directly at this point.
 *
 * TODO: Eliminate the VB struct entirely and just use
 * struct arb_vertex_machine.
 */
GLboolean
_tnl_sse_codegen_vertex_program(struct tnl_compiled_program *p)
{
   struct compilation cp;
   
   memset(&cp, 0, sizeof(cp));
   cp.p = p;
   cp.have_sse2 = 1;

   if (p->compiled_func) {
      _mesa_free((void *)p->compiled_func);
      p->compiled_func = NULL;
   }

   x86_init_func(&cp.func);

   cp.fpucntl = RESTORE_FPU;


   /* Note ctx state is not referenced in building the function, so it
    * depends only on the list of instructions:
    */
   if (!build_vertex_program(&cp)) {
      x86_release_func( &cp.func );
      return GL_FALSE;
   }


   p->compiled_func = (void (*)(struct arb_vp_machine *))x86_get_func( &cp.func );
   return GL_TRUE;
}



#else

GLboolean
_tnl_sse_codegen_vertex_program(struct tnl_compiled_program *p)
{
   /* Dummy version for when USE_SSE_ASM not defined */
   return GL_FALSE;
}

#endif
