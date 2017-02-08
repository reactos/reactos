/**************************************************************************
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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
 *
 **************************************************************************/

#ifndef _RTASM_X86SSE_H_
#define _RTASM_X86SSE_H_

#include "pipe/p_compiler.h"
#include "pipe/p_config.h"

#if defined(PIPE_ARCH_X86) || defined(PIPE_ARCH_X86_64)

/* It is up to the caller to ensure that instructions issued are
 * suitable for the host cpu.  There are no checks made in this module
 * for mmx/sse/sse2 support on the cpu.
 */
struct x86_reg {
   unsigned file:2;
   unsigned idx:4;
   unsigned mod:2;		/* mod_REG if this is just a register */
   int      disp:24;		/* only +/- 23bits of offset - should be enough... */
};

#define X86_MMX 1
#define X86_MMX2 2
#define X86_SSE 4
#define X86_SSE2 8
#define X86_SSE3 0x10
#define X86_SSE4_1 0x20

struct x86_function {
   unsigned caps;
   unsigned size;
   unsigned char *store;
   unsigned char *csr;

   unsigned stack_offset:16;
   unsigned need_emms:8;
   int x87_stack:8;

   unsigned char error_overflow[4];
};

enum x86_reg_file {
   file_REG32,
   file_MMX,
   file_XMM,
   file_x87
};

/* Values for mod field of modr/m byte
 */
enum x86_reg_mod {
   mod_INDIRECT,
   mod_DISP8,
   mod_DISP32,
   mod_REG
};

enum x86_reg_name {
   reg_AX,
   reg_CX,
   reg_DX,
   reg_BX,
   reg_SP,
   reg_BP,
   reg_SI,
   reg_DI,
   reg_R8,
   reg_R9,
   reg_R10,
   reg_R11,
   reg_R12,
   reg_R13,
   reg_R14,
   reg_R15
};


enum x86_cc {
   cc_O,			/* overflow */
   cc_NO,			/* not overflow */
   cc_NAE,			/* not above or equal / carry */
   cc_AE,			/* above or equal / not carry */
   cc_E,			/* equal / zero */
   cc_NE			/* not equal / not zero */
};

enum sse_cc {
   cc_Equal,
   cc_LessThan,
   cc_LessThanEqual,
   cc_Unordered,
   cc_NotEqual,
   cc_NotLessThan,
   cc_NotLessThanEqual,
   cc_Ordered
};

#define cc_Z  cc_E
#define cc_NZ cc_NE


/** generic pointer to function */
typedef void (*x86_func)(void);


/* Begin/end/retrieve function creation:
 */

enum x86_target
{
   X86_32,
   X86_64_STD_ABI,
   X86_64_WIN64_ABI
};

/* make this read a member of x86_function if target != host is desired */
static INLINE enum x86_target x86_target( struct x86_function* p )
{
#ifdef PIPE_ARCH_X86
   return X86_32;
#elif defined(_WIN64)
   return X86_64_WIN64_ABI;
#elif defined(PIPE_ARCH_X86_64)
   return X86_64_STD_ABI;
#endif
}

static INLINE unsigned x86_target_caps( struct x86_function* p )
{
   return p->caps;
}

void x86_init_func( struct x86_function *p );
void x86_init_func_size( struct x86_function *p, unsigned code_size );
void x86_release_func( struct x86_function *p );
x86_func x86_get_func( struct x86_function *p );

/* Debugging:
 */
void x86_print_reg( struct x86_reg reg );


/* Create and manipulate registers and regmem values:
 */
struct x86_reg x86_make_reg( enum x86_reg_file file,
			     enum x86_reg_name idx );

struct x86_reg x86_make_disp( struct x86_reg reg,
			      int disp );

struct x86_reg x86_deref( struct x86_reg reg );

struct x86_reg x86_get_base_reg( struct x86_reg reg );


/* Labels, jumps and fixup:
 */
int x86_get_label( struct x86_function *p );

void x64_rexw(struct x86_function *p);

void x86_jcc( struct x86_function *p,
	      enum x86_cc cc,
	      int label );

int x86_jcc_forward( struct x86_function *p,
			  enum x86_cc cc );

int x86_jmp_forward( struct x86_function *p);

int x86_call_forward( struct x86_function *p);

void x86_fixup_fwd_jump( struct x86_function *p,
			 int fixup );

void x86_jmp( struct x86_function *p, int label );

/* void x86_call( struct x86_function *p, void (*label)() ); */
void x86_call( struct x86_function *p, struct x86_reg reg);

void x86_mov_reg_imm( struct x86_function *p, struct x86_reg dst, int imm );
void x86_add_imm( struct x86_function *p, struct x86_reg dst, int imm );
void x86_or_imm( struct x86_function *p, struct x86_reg dst, int imm );
void x86_and_imm( struct x86_function *p, struct x86_reg dst, int imm );
void x86_sub_imm( struct x86_function *p, struct x86_reg dst, int imm );
void x86_xor_imm( struct x86_function *p, struct x86_reg dst, int imm );
void x86_cmp_imm( struct x86_function *p, struct x86_reg dst, int imm );


/* Macro for sse_shufps() and sse2_pshufd():
 */
#define SHUF(_x,_y,_z,_w)       (((_x)<<0) | ((_y)<<2) | ((_z)<<4) | ((_w)<<6))
#define SHUF_NOOP               RSW(0,1,2,3)
#define GET_SHUF(swz, idx)      (((swz) >> ((idx)*2)) & 0x3)

void mmx_emms( struct x86_function *p );
void mmx_movd( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void mmx_movq( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void mmx_packssdw( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void mmx_packuswb( struct x86_function *p, struct x86_reg dst, struct x86_reg src );

void sse2_movd( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse2_movq( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse2_movdqu( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse2_movdqa( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse2_movsd( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse2_movupd( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse2_movapd( struct x86_function *p, struct x86_reg dst, struct x86_reg src );

void sse2_cvtps2dq( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse2_cvttps2dq( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse2_cvtdq2ps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse2_cvtsd2ss( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse2_cvtpd2ps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );

void sse2_movd( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse2_packssdw( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse2_packsswb( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse2_packuswb( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse2_pshufd( struct x86_function *p, struct x86_reg dest, struct x86_reg arg0,
                  unsigned char shuf );
void sse2_pshuflw( struct x86_function *p, struct x86_reg dest, struct x86_reg arg0,
                  unsigned char shuf );
void sse2_pshufhw( struct x86_function *p, struct x86_reg dest, struct x86_reg arg0,
                  unsigned char shuf );
void sse2_rcpps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse2_rcpss( struct x86_function *p, struct x86_reg dst, struct x86_reg src );

void sse2_punpcklbw( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse2_punpcklwd( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse2_punpckldq( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse2_punpcklqdq( struct x86_function *p, struct x86_reg dst, struct x86_reg src );

void sse2_psllw_imm( struct x86_function *p, struct x86_reg dst, unsigned imm );
void sse2_pslld_imm( struct x86_function *p, struct x86_reg dst, unsigned imm );
void sse2_psllq_imm( struct x86_function *p, struct x86_reg dst, unsigned imm );

void sse2_psrlw_imm( struct x86_function *p, struct x86_reg dst, unsigned imm );
void sse2_psrld_imm( struct x86_function *p, struct x86_reg dst, unsigned imm );
void sse2_psrlq_imm( struct x86_function *p, struct x86_reg dst, unsigned imm );

void sse2_psraw_imm( struct x86_function *p, struct x86_reg dst, unsigned imm );
void sse2_psrad_imm( struct x86_function *p, struct x86_reg dst, unsigned imm );

void sse2_por( struct x86_function *p, struct x86_reg dst, struct x86_reg src );

void sse2_pshuflw( struct x86_function *p, struct x86_reg dst, struct x86_reg src, uint8_t imm );
void sse2_pshufhw( struct x86_function *p, struct x86_reg dst, struct x86_reg src, uint8_t imm );
void sse2_pshufd( struct x86_function *p, struct x86_reg dst, struct x86_reg src, uint8_t imm );

void sse_prefetchnta( struct x86_function *p, struct x86_reg ptr);
void sse_prefetch0( struct x86_function *p, struct x86_reg ptr);
void sse_prefetch1( struct x86_function *p, struct x86_reg ptr);

void sse_movntps( struct x86_function *p, struct x86_reg dst, struct x86_reg src);

void sse_addps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_addss( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_cvtps2pi( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_divss( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_andnps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_andps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_cmpps( struct x86_function *p, struct x86_reg dst, struct x86_reg src,
                enum sse_cc cc );
void sse_maxps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_maxss( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_minps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_movaps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_movhlps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_movhps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_movlhps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_movlps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_movss( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_movups( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_mulps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_mulss( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_orps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_xorps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_subps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_rsqrtps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_rsqrtss( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_shufps( struct x86_function *p, struct x86_reg dest, struct x86_reg arg0,
                 unsigned char shuf );
void sse_unpckhps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_unpcklps( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void sse_pmovmskb( struct x86_function *p, struct x86_reg dest, struct x86_reg src );
void sse_movmskps( struct x86_function *p, struct x86_reg dst, struct x86_reg src);

void x86_add( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void x86_and( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void x86_cmovcc( struct x86_function *p, struct x86_reg dst, struct x86_reg src, enum x86_cc cc );
void x86_cmp( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void x86_dec( struct x86_function *p, struct x86_reg reg );
void x86_inc( struct x86_function *p, struct x86_reg reg );
void x86_lea( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void x86_mov( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void x64_mov64( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void x86_mov8( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void x86_mov16( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void x86_movzx8(struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void x86_movzx16(struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void x86_mov_imm(struct x86_function *p, struct x86_reg dst, int imm );
void x86_mov8_imm(struct x86_function *p, struct x86_reg dst, uint8_t imm );
void x86_mov16_imm(struct x86_function *p, struct x86_reg dst, uint16_t imm );
void x86_mul( struct x86_function *p, struct x86_reg src );
void x86_imul( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void x86_or( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void x86_pop( struct x86_function *p, struct x86_reg reg );
void x86_push( struct x86_function *p, struct x86_reg reg );
void x86_push_imm32( struct x86_function *p, int imm );
void x86_ret( struct x86_function *p );
void x86_retw( struct x86_function *p, unsigned short imm );
void x86_sub( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void x86_test( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void x86_xor( struct x86_function *p, struct x86_reg dst, struct x86_reg src );
void x86_sahf( struct x86_function *p );
void x86_div( struct x86_function *p, struct x86_reg src );
void x86_bswap( struct x86_function *p, struct x86_reg src );
void x86_shr_imm( struct x86_function *p, struct x86_reg reg, unsigned imm );
void x86_sar_imm( struct x86_function *p, struct x86_reg reg, unsigned imm );
void x86_shl_imm( struct x86_function *p, struct x86_reg reg, unsigned imm  );

void x86_cdecl_caller_push_regs( struct x86_function *p );
void x86_cdecl_caller_pop_regs( struct x86_function *p );

void x87_assert_stack_empty( struct x86_function *p );

void x87_f2xm1( struct x86_function *p );
void x87_fabs( struct x86_function *p );
void x87_fadd( struct x86_function *p, struct x86_reg dst, struct x86_reg arg );
void x87_faddp( struct x86_function *p, struct x86_reg dst );
void x87_fchs( struct x86_function *p );
void x87_fclex( struct x86_function *p );
void x87_fcmovb( struct x86_function *p, struct x86_reg src );
void x87_fcmovbe( struct x86_function *p, struct x86_reg src );
void x87_fcmove( struct x86_function *p, struct x86_reg src );
void x87_fcmovnb( struct x86_function *p, struct x86_reg src );
void x87_fcmovnbe( struct x86_function *p, struct x86_reg src );
void x87_fcmovne( struct x86_function *p, struct x86_reg src );
void x87_fcom( struct x86_function *p, struct x86_reg dst );
void x87_fcomi( struct x86_function *p, struct x86_reg dst );
void x87_fcomip( struct x86_function *p, struct x86_reg dst );
void x87_fcomp( struct x86_function *p, struct x86_reg dst );
void x87_fcos( struct x86_function *p );
void x87_fdiv( struct x86_function *p, struct x86_reg dst, struct x86_reg arg );
void x87_fdivp( struct x86_function *p, struct x86_reg dst );
void x87_fdivr( struct x86_function *p, struct x86_reg dst, struct x86_reg arg );
void x87_fdivrp( struct x86_function *p, struct x86_reg dst );
void x87_fild( struct x86_function *p, struct x86_reg arg );
void x87_fist( struct x86_function *p, struct x86_reg dst );
void x87_fistp( struct x86_function *p, struct x86_reg dst );
void x87_fld( struct x86_function *p, struct x86_reg arg );
void x87_fld1( struct x86_function *p );
void x87_fldcw( struct x86_function *p, struct x86_reg arg );
void x87_fldl2e( struct x86_function *p );
void x87_fldln2( struct x86_function *p );
void x87_fldz( struct x86_function *p );
void x87_fmul( struct x86_function *p, struct x86_reg dst, struct x86_reg arg );
void x87_fmulp( struct x86_function *p, struct x86_reg dst );
void x87_fnclex( struct x86_function *p );
void x87_fprndint( struct x86_function *p );
void x87_fpop( struct x86_function *p );
void x87_fscale( struct x86_function *p );
void x87_fsin( struct x86_function *p );
void x87_fsincos( struct x86_function *p );
void x87_fsqrt( struct x86_function *p );
void x87_fst( struct x86_function *p, struct x86_reg dst );
void x87_fstp( struct x86_function *p, struct x86_reg dst );
void x87_fsub( struct x86_function *p, struct x86_reg dst, struct x86_reg arg );
void x87_fsubp( struct x86_function *p, struct x86_reg dst );
void x87_fsubr( struct x86_function *p, struct x86_reg dst, struct x86_reg arg );
void x87_fsubrp( struct x86_function *p, struct x86_reg dst );
void x87_ftst( struct x86_function *p );
void x87_fxch( struct x86_function *p, struct x86_reg dst );
void x87_fxtract( struct x86_function *p );
void x87_fyl2x( struct x86_function *p );
void x87_fyl2xp1( struct x86_function *p );
void x87_fwait( struct x86_function *p );
void x87_fnstcw( struct x86_function *p, struct x86_reg dst );
void x87_fnstsw( struct x86_function *p, struct x86_reg dst );
void x87_fucompp( struct x86_function *p );
void x87_fucomp( struct x86_function *p, struct x86_reg arg );
void x87_fucom( struct x86_function *p, struct x86_reg arg );



/* Retrieve a reference to one of the function arguments, taking into
 * account any push/pop activity.  Note - doesn't track explicit
 * manipulation of ESP by other instructions.
 */
struct x86_reg x86_fn_arg( struct x86_function *p, unsigned arg );

#endif
#endif
