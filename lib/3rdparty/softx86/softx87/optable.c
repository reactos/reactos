/*
 * optable.c
 *
 * Copyright (C) 2003, 2004 Jonathan Campbell <jcampbell@mdjk.com>
 *
 * Opcode jumptable.
 *
 * Allows the recognition of many opcodes without having to write approximately
 * 500+ if...then...else statements which is a) inefficient and b) apparently can
 * cause some compilers such as Microsoft C++ to crash during the compile stage with
 * an error. Since it's a table, it can be referred to via a pointer that can be easily
 * redirected to other opcode tables (i.e., one for the 8086, one for the 80286, etc.)
 * without much hassle.
 * 
 * The table contains two pointers: one for an "execute" function, and one for a
 * "decompile" function. The execute function is given the context and the initial opcode.
 * If more opcodes are needed the function calls softx86_fetch_exec_byte(). The decode
 * function is also given the opcode but also a char[] array where it is expected to
 * sprintf() or strcpy() the disassembled output. If that function needs more opcodes
 * it calls softx86_fetch_dec_byte().
 *
 ***********************************************************************************
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 ************************************************************************************/

#include <softx87.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "optab87.h"

void op_fcom(softx87_ctx* ctx,softx87_reg80 *dst,softx87_reg80 *src);
void op_fadd(softx87_ctx* ctx,softx87_reg80 *dst,softx87_reg80 *src);
void op_fsub(softx87_ctx* ctx,softx87_reg80 *dst,softx87_reg80 *src);
void op_fdiv(softx87_ctx* ctx,softx87_reg80 *dst,softx87_reg80 *src);
void op_fdivr(softx87_ctx* ctx,softx87_reg80 *dst,softx87_reg80 *src);

char				s87op1_tmp[32];
char				s87op2_tmp[32];

/* pops value from FPU stack */
void softx86_popstack(softx87_ctx* ctx87)
{
	int TOP;

	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
	SX87_FPUTAGW_TAG_SET(ctx87->state.tag_word,TOP,SX87_FPUTAGVAL_EMPTY);
	TOP = (TOP+1)&7;
	SX87_FPUSTAT_TOP_SET(ctx87->state.status_word,TOP);
}

/* given an FPU register, convert to an integer */
sx86_uldword sx87_getint(softx87_reg80 *r)
{
	sx86_uldword m;
	sx86_uword e;

// TODO: take into consideration the rounding mode
	e = 63 - (r->exponent - 16383);
	if (e > 63) {
		m = 0;
	}
	else if (e > 0) {
		// TODO: rounding mode?
		m = r->mantissa >> ((sx86_uldword)(e));
	}
	else if (e < 0) {
		sx86_uldword n;

/* first figure out if bits will be shifted out (too large).
   if that's the case compensate by setting the mantissa to
   the largest possible __uint64 value. */
		n = ((sx86_uldword)-1) << ((sx86_uldword)(63 + e));
		if (((sx86_uldword)(r->mantissa & n)) != ((sx86_uldword)0))
			m = (sx86_uldword)-1;
		else
			m = r->mantissa << ((sx86_uldword)(-e));
	}
	else {
		m = r->mantissa;
	}

	return m;
}

/* FBSTP 80 */
void op_fbstp80(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP;
	int i;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 10 || !ctx87) return;

	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);

/* TODO: check and perform cases as documented:

-infinity =	#IA exception
-infinity =     #IA exception

   TODO: what do we do really if the entry is empty?
*/
	tmp.sign_bit = ctx87->state.st[TOP].sign_bit;
	tmp.mantissa = ctx87->state.st[TOP].mantissa;
	tmp.exponent = ctx87->state.st[TOP].exponent;

/* shift data around to produce integer version. */
/* if it's too large, clip it */
	tmp.mantissa = sx87_getint(&tmp);

/* if too large for BCD coding, clip it. */
	if (tmp.mantissa > 999999999999999999)		/* 18 digits usable, so... */
		tmp.mantissa = 999999999999999999;	/* clip if >= 10^19 */

/* put it out there */
	for (i=0;i < 9;i++) {
		datz[i]  =  (char)(tmp.mantissa % ((sx86_uldword)10));
		tmp.mantissa /= (sx86_uldword)10;
		datz[i] |= ((char)(tmp.mantissa % ((sx86_uldword)10)))<<4;
		tmp.mantissa /= (sx86_uldword)10;
	}

	datz[9] = tmp.sign_bit ? ((char)0x80) : 0x00;

/* pop */
	softx86_popstack(ctx->ref_softx87_ctx);
}

/* the hard work behind FLD [mem32] is here */
void op_fld32(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 4 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_fp32(ctx87,datz,&tmp);
/* normalize */
	softx87_normalize(ctx87,&tmp);

	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
/* set C1 if stack overflow */
	if (SX87_FPUTAGW_TAG(ctx87->state.tag_word,TOP) != SX87_FPUTAGVAL_EMPTY)
		ctx87->state.status_word |=  SX87_FPUSTAT_C1;
	else
		ctx87->state.status_word &= ~SX87_FPUSTAT_C1;
/* decrement TOP, tag as valid */
	TOP = (TOP-1)&7;
	SX87_FPUSTAT_TOP_SET(ctx87->state.status_word,TOP);
	SX87_FPUTAGW_TAG_SET(ctx87->state.tag_word,TOP,SX87_FPUTAGVAL_VALID);
/* store */
	ctx87->state.st[TOP].exponent =	tmp.exponent;
	ctx87->state.st[TOP].mantissa =	tmp.mantissa;
	ctx87->state.st[TOP].sign_bit =	tmp.sign_bit;
}

/* the hard work behind FLD [mem64] is here */
void op_fld64(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 8 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_fp64(ctx87,datz,&tmp);
/* normalize */
	softx87_normalize(ctx87,&tmp);

	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
/* set C1 if stack overflow */
	if (SX87_FPUTAGW_TAG(ctx87->state.tag_word,TOP) != SX87_FPUTAGVAL_EMPTY)
		ctx87->state.status_word |=  SX87_FPUSTAT_C1;
	else
		ctx87->state.status_word &= ~SX87_FPUSTAT_C1;
/* decrement TOP, tag as valid */
	TOP = (TOP-1)&7;
	SX87_FPUSTAT_TOP_SET(ctx87->state.status_word,TOP);
	SX87_FPUTAGW_TAG_SET(ctx87->state.tag_word,TOP,SX87_FPUTAGVAL_VALID);
/* store */
	ctx87->state.st[TOP].exponent =	tmp.exponent;
	ctx87->state.st[TOP].mantissa =	tmp.mantissa;
	ctx87->state.st[TOP].sign_bit =	tmp.sign_bit;
}

/* FIADD [mem16] */
void op_fiadd16(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 2 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_int16(ctx87,datz,&tmp);
/* normalize */
	softx87_normalize(ctx87,&tmp);

/* do it */
	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
	op_fadd(ctx87,&ctx87->state.st[TOP],&tmp);
}

/* FICOM [mem16] */
void op_ficom16(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 2 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_int16(ctx87,datz,&tmp);
/* normalize */
	softx87_normalize(ctx87,&tmp);

/* do it */
	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
	op_fcom(ctx87,&ctx87->state.st[TOP],&tmp);
}

/* FICOMP [mem16] */
void op_ficomp16(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 2 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_int16(ctx87,datz,&tmp);
/* normalize */
	softx87_normalize(ctx87,&tmp);

/* do it */
	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
	op_fcom(ctx87,&ctx87->state.st[TOP],&tmp);
	softx86_popstack(ctx87);
}

/* FICOM [mem32] */
void op_ficom32(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 4 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_int32(ctx87,datz,&tmp);
/* normalize */
	softx87_normalize(ctx87,&tmp);

/* do it */
	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
	op_fcom(ctx87,&ctx87->state.st[TOP],&tmp);
}

/* FICOMP [mem32] */
void op_ficomp32(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 4 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_int32(ctx87,datz,&tmp);
/* normalize */
	softx87_normalize(ctx87,&tmp);

/* do it */
	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
	op_fcom(ctx87,&ctx87->state.st[TOP],&tmp);
	softx86_popstack(ctx87);
}

/* FISUB [mem16] */
void op_fisub16(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 2 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_int16(ctx87,datz,&tmp);
/* normalize */
	softx87_normalize(ctx87,&tmp);

/* do it */
	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
	op_fsub(ctx87,&ctx87->state.st[TOP],&tmp);
}

/* FIADD [mem32] */
void op_fiadd32(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 4 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_int32(ctx87,datz,&tmp);
/* normalize */
	softx87_normalize(ctx87,&tmp);

/* do it */
	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
	op_fadd(ctx87,&ctx87->state.st[TOP],&tmp);
}

/* FISUB [mem32] */
void op_fisub32(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 4 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_int32(ctx87,datz,&tmp);
/* normalize */
	softx87_normalize(ctx87,&tmp);

/* do it */
	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
	op_fsub(ctx87,&ctx87->state.st[TOP],&tmp);
}

/* FIDIV [mem32] */
void op_fidiv32(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 4 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_int32(ctx87,datz,&tmp);
/* normalize */
	softx87_normalize(ctx87,&tmp);

/* do it */
	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
	op_fdiv(ctx87,&ctx87->state.st[TOP],&tmp);
}

/* FIDIVR [mem32] */
void op_fidivr32(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 4 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_int32(ctx87,datz,&tmp);
/* normalize */
	softx87_normalize(ctx87,&tmp);

/* do it */
	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
	op_fdivr(ctx87,&ctx87->state.st[TOP],&tmp);
}

/* FIDIV [mem16] */
void op_fidiv16(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 2 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_int16(ctx87,datz,&tmp);
/* normalize */
	softx87_normalize(ctx87,&tmp);

/* do it */
	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
	op_fdiv(ctx87,&ctx87->state.st[TOP],&tmp);
}

/* FIDIVR [mem16] */
void op_fidivr16(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 2 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_int16(ctx87,datz,&tmp);
/* normalize */
	softx87_normalize(ctx87,&tmp);

/* do it */
	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
	op_fdivr(ctx87,&ctx87->state.st[TOP],&tmp);
}

/* FADD [mem32] */
void op_fadd32(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 4 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_fp32(ctx87,datz,&tmp);
/* normalize */
	softx87_normalize(ctx87,&tmp);

/* do it */
	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
	op_fadd(ctx87,&ctx87->state.st[TOP],&tmp);
}

/* FDIV [mem32] */
void op_fdiv32(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 4 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_fp32(ctx87,datz,&tmp);
/* normalize */
	softx87_normalize(ctx87,&tmp);

/* do it */
	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
	op_fdiv(ctx87,&ctx87->state.st[TOP],&tmp);
}

/* FDIVR [mem32] */
void op_fdivr32(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 4 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_fp32(ctx87,datz,&tmp);
/* normalize */
	softx87_normalize(ctx87,&tmp);

/* do it */
	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
	op_fdivr(ctx87,&ctx87->state.st[TOP],&tmp);
}

/* FDIVR [mem64] */
void op_fdivr64(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 8 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_fp64(ctx87,datz,&tmp);
/* normalize */
	softx87_normalize(ctx87,&tmp);

/* do it */
	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
	op_fdivr(ctx87,&ctx87->state.st[TOP],&tmp);
}

/* FDIV [mem64] */
void op_fdiv64(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 8 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_fp64(ctx87,datz,&tmp);
/* normalize */
	softx87_normalize(ctx87,&tmp);

/* do it */
	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
	op_fdiv(ctx87,&ctx87->state.st[TOP],&tmp);
}

/* FSUB [mem32] */
void op_fsub32(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 4 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_fp32(ctx87,datz,&tmp);
/* normalize */
	softx87_normalize(ctx87,&tmp);

/* do it */
	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
	op_fsub(ctx87,&ctx87->state.st[TOP],&tmp);
}

/* FADD [mem64] */
void op_fadd64(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 8 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_fp64(ctx87,datz,&tmp);
/* normalize */
	softx87_normalize(ctx87,&tmp);

/* do it */
	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
	op_fadd(ctx87,&ctx87->state.st[TOP],&tmp);
}

/* FSUB [mem64] */
void op_fsub64(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 8 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_fp64(ctx87,datz,&tmp);
/* normalize */
	softx87_normalize(ctx87,&tmp);

/* do it */
	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
	op_fsub(ctx87,&ctx87->state.st[TOP],&tmp);
}

static void op_fcom_compflags(softx87_ctx* ctx,softx87_reg80 *diff)
{
	if (diff->mantissa == 0) {	// equal (basically zero no matter what the exponent)
		ctx->state.status_word |=   SX87_FPUSTAT_C3;
		ctx->state.status_word &= ~(SX87_FPUSTAT_C2 | SX87_FPUSTAT_C0);
	}
	else if (diff->sign_bit == 1) {	// st0 < src
		ctx->state.status_word |=   SX87_FPUSTAT_C0;
		ctx->state.status_word &= ~(SX87_FPUSTAT_C3 | SX87_FPUSTAT_C2);
	}
	else if (diff->sign_bit == 0) {	// st0 > src
		ctx->state.status_word &= ~(SX87_FPUSTAT_C3 | SX87_FPUSTAT_C2 | SX87_FPUSTAT_C0);
	}
	else {
		// TODO
	}
}

/* FCOM [mem64] */
void op_fcom64(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 src;
	softx87_reg80 st0;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 8 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_fp64(ctx87,datz,&src);
/* normalize */
	softx87_normalize(ctx87,&src);

/* do it */
	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
	memcpy(&st0,&ctx87->state.st[TOP],sizeof(softx87_reg80));
	softx87_normalize(ctx87,&st0);
	op_fsub(ctx87,&st0,&src);	// st0 -= src

/* what is the result? */
	op_fcom_compflags(ctx87,&st0);
}

/* FCOMP [mem64] */
void op_fcomp64(softx86_ctx* ctx,char *datz,int sz)
{
	op_fcom64(ctx,datz,sz);
	softx86_popstack(ctx->ref_softx87_ctx);
}

/* FCOM [mem32] */
void op_fcom32(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 src;
	softx87_reg80 st0;
	softx87_ctx* ctx87;
	int TOP;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 4 || !ctx87) return;

/* convert to 80-bit floating point */
	softx87_unpack_raw_fp32(ctx87,datz,&src);
/* normalize */
	softx87_normalize(ctx87,&src);

/* do it */
	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
	memcpy(&st0,&ctx87->state.st[TOP],sizeof(softx87_reg80));
	softx87_normalize(ctx87,&st0);
	op_fsub(ctx87,&st0,&src);	// st0 -= src

/* what is the result? */
	op_fcom_compflags(ctx87,&st0);
}

/* FCOMP [mem32] */
void op_fcomp32(softx86_ctx* ctx,char *datz,int sz)
{
	op_fcom32(ctx,datz,sz);
	softx86_popstack(ctx->ref_softx87_ctx);
}

/* FBLD [mem80] */
void op_fbld80(softx86_ctx* ctx,char *datz,int sz)
{
	softx87_reg80 tmp;
	softx87_ctx* ctx87;
	int TOP,i;

/* sanity check */
	ctx87 = (softx87_ctx*)ctx->ref_softx87_ctx;
	if (sz != 10 || !ctx87) return;

/* think of it as a string where each character is stored in one nibble
   representing one digit. UNFORTUNATELY it's also stored backwards... */
	if (datz[9] & 0x80)	tmp.sign_bit = 1;
	else			tmp.sign_bit = 0;

	tmp.exponent = 16383 + 63;
	tmp.mantissa = (sx86_uldword)0;
	for (i=17;i >= 0;i--) {
		tmp.mantissa *= (sx86_uldword)10;
		tmp.mantissa += (sx86_uldword)((datz[i>>1] >> ((i&1)*4))&0xF);
	}

/* normalize */
	softx87_normalize(ctx87,&tmp);

	TOP = SX87_FPUSTAT_TOP(ctx87->state.status_word);
/* set C1 if stack overflow */
	if (SX87_FPUTAGW_TAG(ctx87->state.tag_word,TOP) != SX87_FPUTAGVAL_EMPTY)
		ctx87->state.status_word |=  SX87_FPUSTAT_C1;
	else
		ctx87->state.status_word &= ~SX87_FPUSTAT_C1;
/* decrement TOP, tag as valid */
	TOP = (TOP-1)&7;
	SX87_FPUSTAT_TOP_SET(ctx87->state.status_word,TOP);
	SX87_FPUTAGW_TAG_SET(ctx87->state.tag_word,TOP,SX87_FPUTAGVAL_VALID);
/* store */
	ctx87->state.st[TOP].exponent =	tmp.exponent;
	ctx87->state.st[TOP].mantissa =	tmp.mantissa;
	ctx87->state.st[TOP].sign_bit =	tmp.sign_bit;
}

/* general purpose FADD */
void op_fadd(softx87_ctx* ctx,softx87_reg80 *dst,softx87_reg80 *src)
{
	softx87_reg80 major;
	softx87_reg80 minor;
	sx87_uldword waste,threshhold;
	int exb;
/* TODO: check and perform cases as documented:

-infinity + (anything) =	-infinity
-infinity + +infinity  =        #IA exception
NaN       + (anything) =        NaN
*/

/* in doing this... how many extra bits will we need? */
	exb   = (int)(src->mantissa>>((sx87_uldword)63));
	exb  |= (int)(dst->mantissa>>((sx87_uldword)63));
	if (src->sign_bit != dst->sign_bit) exb++;
/* avoid addition if it wouldn't make any difference */
	if (dst->exponent >= (src->exponent+(64-exb)))
		return;
/* copy src => dst if the destination value is dwarfed by the source value */
	else if (dst->exponent <= (src->exponent-(64-exb))) {
		memcpy(dst,src,sizeof(softx87_reg80));
		return;
	}

/* scale them up so their exponents match */
	if (dst->exponent > src->exponent) {
		memcpy(&major,dst,sizeof(softx87_reg80));
		memcpy(&minor,src,sizeof(softx87_reg80));
	}
	else if (dst->exponent < src->exponent) {
		memcpy(&major,src,sizeof(softx87_reg80));
		memcpy(&minor,dst,sizeof(softx87_reg80));
	}
	else {
		memcpy(&major,dst,sizeof(softx87_reg80));
		memcpy(&minor,src,sizeof(softx87_reg80));
	}

/* "major" should have the highest exponent */
	if ((major.exponent+exb) > minor.exponent) {
		int b;

		b = (major.exponent - minor.exponent)+exb;
		threshhold   = (sx87_uldword)-1;
		threshhold >>= (sx87_uldword)(64-b);
		waste        = minor.mantissa & threshhold;
		threshhold   = (threshhold+1)>>1;

/* scale up the minor value */
		minor.exponent  += b;
		minor.mantissa >>= (sx87_uldword)b;
/* round it */
		switch (SX87_FPUCTRLW_RNDCTL(ctx->state.control_word))
		{
			case SX87_FPUCTRLW_RNDCTL_NEAREST:
				if (waste >= threshhold) minor.mantissa++;
				break;

			case SX87_FPUCTRLW_RNDCTL_DOWNINF:
				if (waste != 0L && minor.sign_bit) minor.mantissa++;
				break;

			case SX87_FPUCTRLW_RNDCTL_UPINF:
				if (waste != 0L && !minor.sign_bit) minor.mantissa++;
				break;

			case SX87_FPUCTRLW_RNDCTL_ZERO:
			/* nothing */
				break;
		}
	}

	if (exb > 0) {
/* scale up the major value */
		threshhold   = (sx87_uldword)-1;
		threshhold >>= (sx87_uldword)(64-exb);
		waste        = major.mantissa & threshhold;
		threshhold   = (threshhold+1)>>1;

		major.exponent  += exb;
		major.mantissa >>= (sx87_uldword)exb;
/* round it */
		switch (SX87_FPUCTRLW_RNDCTL(ctx->state.control_word))
		{
			case SX87_FPUCTRLW_RNDCTL_NEAREST:
				if (waste >= threshhold) major.mantissa++;
				break;

			case SX87_FPUCTRLW_RNDCTL_DOWNINF:
				if (waste != 0L && major.sign_bit) major.mantissa++;
				break;

			case SX87_FPUCTRLW_RNDCTL_UPINF:
				if (waste != 0L && !major.sign_bit) major.mantissa++;
				break;

			case SX87_FPUCTRLW_RNDCTL_ZERO:
			/* nothing */
				break;
		}
	}

	if (src->sign_bit == dst->sign_bit) {
		/* simply add */
		dst->exponent = major.exponent;
		dst->sign_bit = major.sign_bit;
		dst->mantissa = minor.mantissa + major.mantissa;
	}
	else {
		/* negate mantissa based on sign */
		if (minor.sign_bit)	minor.mantissa = -minor.mantissa;
		if (major.sign_bit)	major.mantissa = -major.mantissa;
		/* now add, treating 63rd bit as sign */
		dst->exponent = major.exponent;
		dst->mantissa = minor.mantissa + major.mantissa;
		dst->sign_bit = dst->mantissa>>((sx87_uldword)63);
		if (dst->sign_bit) dst->mantissa = -dst->mantissa;
	}

	softx87_normalize(ctx,dst);
}

/* FCOM */
void op_fcom(softx87_ctx* ctx,softx87_reg80 *dst,softx87_reg80 *src)
{
	softx87_reg80 st0;

	memcpy(&st0,dst,sizeof(softx87_reg80));
	softx87_normalize(ctx,&st0);
	op_fsub(ctx,&st0,src);	// st0 -= src

/* what is the result? */
	op_fcom_compflags(ctx,&st0);
}

/* FCOM */
void op_fcomp(softx87_ctx* ctx,softx87_reg80 *dst,softx87_reg80 *src)
{
	softx87_reg80 st0;

	memcpy(&st0,dst,sizeof(softx87_reg80));
	softx87_normalize(ctx,&st0);
	op_fsub(ctx,&st0,src);	// st0 -= src

/* what is the result? */
	op_fcom_compflags(ctx,&st0);
	softx86_popstack(ctx);
}

/* general purpose FSUB */
void op_fsub(softx87_ctx* ctx,softx87_reg80 *dst,softx87_reg80 *src)
{
	softx87_reg80 stm;

/* cheap trick for now ;) */
	memcpy(&stm,src,sizeof(softx87_reg80));
	stm.sign_bit=!stm.sign_bit;
	op_fadd(ctx,dst,&stm);
}

/* general purpose FDIV */
void op_fdiv(softx87_ctx* ctx,softx87_reg80 *dst,softx87_reg80 *src)
{
	double x,y;

// TODO: consideration for NaN, etc.
// TODO: make own dividing implementation, this is a cheap hack.
	x  = softx87_get_fpu_double(ctx,dst,NULL);
	y  = softx87_get_fpu_double(ctx,src,NULL);
	x /= y;
	softx87_set_fpu_double(ctx,dst,x);
}

/* general purpose FDIVR */
void op_fdivr(softx87_ctx* ctx,softx87_reg80 *dst,softx87_reg80 *src)
{
	double x,y;

// TODO: consideration for NaN, etc.
// TODO: make own dividing implementation, this is a cheap hack.
	x = softx87_get_fpu_double(ctx,dst,NULL);
	y = softx87_get_fpu_double(ctx,src,NULL);
	x = y / x;
	softx87_set_fpu_double(ctx,dst,x);
}

static int Sfx87OpcodeExec_group_D8(sx87_ubyte opcode,softx87_ctx* ctx)
{
/* FADD ST(0),ST(i) */
	if (opcode >= 0xC0 && opcode < 0xC8) {
		int st0,i;

		i   = (int)(opcode-0xC0);
		i   = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),i);
		st0 = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),0);
/* add */
		op_fadd(ctx,&ctx->state.st[st0],&ctx->state.st[i]);
/* done */
		return 1;
	}
/* FCOM ST(0),ST(i) */
	else if (opcode >= 0xD0 && opcode < 0xD8) {
		int st0,i;

		i   = (int)(opcode-0xD0);
		i   = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),i);
		st0 = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),0);
/* add */
		op_fcom(ctx,&ctx->state.st[st0],&ctx->state.st[i]);
/* done */
		return 1;
	}
/* FCOMP ST(0),ST(i) */
	else if (opcode >= 0xD8 && opcode < 0xE0) {
		int st0,i;

		i   = (int)(opcode-0xD8);
		i   = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),i);
		st0 = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),0);
/* add */
		op_fcomp(ctx,&ctx->state.st[st0],&ctx->state.st[i]);
/* done */
		return 1;
	}
/* FSUB ST(0),ST(i) */
	else if (opcode >= 0xE0 && opcode < 0xE8) {
		int st0,i;

		i   = (int)(opcode-0xE0);
		i   = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),i);
		st0 = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),0);
/* add */
		op_fsub(ctx,&ctx->state.st[st0],&ctx->state.st[i]);
/* done */
		return 1;
	}
/* FDIV ST(0),ST(i) */
	else if (opcode >= 0xF0 && opcode < 0xF8) {
		int st0,i;

		i   = (int)(opcode-0xF0);
		i   = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),i);
		st0 = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),0);
/* add */
		op_fdiv(ctx,&ctx->state.st[st0],&ctx->state.st[i]);
/* done */
		return 1;
	}
/* FDIVR ST(0),ST(i) */
	else if (opcode >= 0xF8) {
		int st0,i;

		i   = (int)(opcode-0xF8);
		i   = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),i);
		st0 = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),0);
/* add */
		op_fdivr(ctx,&ctx->state.st[st0],&ctx->state.st[i]);
/* done */
		return 1;
	}
/* combo */
	else if (opcode < 0xC0) {
		sx86_ubyte mod,reg,rm;

		sx86_modregrm_unpack(opcode,mod,reg,rm);

		if (reg == 0)		/* FADD mem32 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,4,op_fadd32);
		else if (reg == 2)	/* FCOM mem32 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,4,op_fcom32);
		else if (reg == 3)	/* FCOMP mem32 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,4,op_fcomp32);
		else if (reg == 4)	/* FSUB mem32 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,4,op_fsub32);
		else if (reg == 6)	/* FDIV mem32 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,4,op_fdiv32);
		else if (reg == 7)	/* FDIVR mem32 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,4,op_fdivr32);
		else
			return 0;

		return 1;
	}

	return 0;
}

static int Sfx87OpcodeDec_group_D8(sx87_ubyte opcode,softx87_ctx* ctx,char buf[128])
{
/* FADD ST(0),ST(i) */
	if (opcode >= 0xC0 && opcode < 0xC8) {
		sprintf(buf,"FADD ST(0),ST(%d)",opcode-0xC0);
		return 1;
	}
/* FCOM ST(0),ST(i) */
	else if (opcode >= 0xD0 && opcode < 0xD8) {
		sprintf(buf,"FCOM ST(0),ST(%d)",opcode-0xD0);
		return 1;
	}
/* FCOMP ST(0),ST(i) */
	else if (opcode >= 0xD8 && opcode < 0xE0) {
		sprintf(buf,"FCOMP ST(0),ST(%d)",opcode-0xD8);
		return 1;
	}
/* FSUB ST(0),ST(i) */
	else if (opcode >= 0xE0 && opcode < 0xE8) {
		sprintf(buf,"FSUB ST(0),ST(%d)",opcode-0xE0);
		return 1;
	}
/* FDIV ST(0),ST(i) */
	else if (opcode >= 0xF0 && opcode < 0xF8) {
		sprintf(buf,"FDIV ST(0),ST(%d)",opcode-0xF0);
		return 1;
	}
/* FDIVR ST(0),ST(i) */
	else if (opcode >= 0xF8) {
		sprintf(buf,"FDIVR ST(0),ST(%d)",opcode-0xF8);
		return 1;
	}
/* combo */
	else if (opcode < 0xC0) {
		sx86_ubyte mod,reg,rm;

		sx86_modregrm_unpack(opcode,mod,reg,rm);
		ctx->callbacks.on_sx86_dec_full_modrmonly(ctx->ref_softx86,1,0,mod,rm,s87op1_tmp);

		if (reg == 0)
			sprintf(buf,"FADD %s (mem32)",s87op1_tmp);
		else if (reg == 2)
			sprintf(buf,"FCOM %s (mem32)",s87op1_tmp);
		else if (reg == 3)
			sprintf(buf,"FCOMP %s (mem32)",s87op1_tmp);
		else if (reg == 4)
			sprintf(buf,"FSUB %s (mem32)",s87op1_tmp);
		else if (reg == 6)
			sprintf(buf,"FDIV %s (mem32)",s87op1_tmp);
		else if (reg == 7)
			sprintf(buf,"FDIVR %s (mem32)",s87op1_tmp);
		else
			return 0;

		return 1;
	}

	buf[0]=0;
	return 0;
}

void sx87_rnd(softx87_reg80 *val)
{
/* TODO: rounding modes */
	if (1) {
		/* round to nearest */
		if (val->exponent < 16382) {
			/* value is too small that val must be < 0.5 */
			val->mantissa = 0;
			val->exponent = 16383;
			val->sign_bit = 0;
		}
		else if (val->exponent > (16383+63)) {
			/* value is too large to round */
		}
		else {
			sx87_uldword t,s,m,i;

			s = (sx87_uldword)(val->exponent-16382);
			m = ((sx87_uldword)-1) >> s;
			t = val->mantissa & m;
			i = val->mantissa & (~m);
/* special case: when s == 0, m = 0xFFFFFFFFFFFFFFFF which can
   overflow to 0 if incremented and shifted */
			if (s == 0)	s = (sx87_uldword)0x8000000000000000L;
			else		s = (m + ((sx87_uldword)1)) >> ((sx87_uldword)1);
/* m = mask to obtain fractional part
   t = fractional part
   i = whole part
   s = fractional part / 2 */
			if (t >= s) i++;
			val->mantissa = i;
		}
	}
}

static int Sfx87OpcodeExec_group_D9(sx87_ubyte opcode,softx87_ctx* ctx)
{
/* FSCALE */
	if (opcode == 0xFD) {
		sx86_uldword e;
		int ST0,ST1;

		ST0 = SX87_FPUSTAT_TOP(ctx->state.status_word);
		ST1 = (ST0 + 1) & 7;

		// TODO: cases with infinity, NaN, etc
		e = sx87_getint(&ctx->state.st[ST1]);
		e = ctx->state.st[ST0].exponent + e;
		if (e < 0) {
			ctx->state.st[ST0].exponent = 16383;
			ctx->state.st[ST0].mantissa = 0;
		}
		else if (e > 0x7FFF) {
			ctx->state.st[ST0].exponent = 0x7FFF;
			ctx->state.st[ST0].mantissa = (sx86_uldword)(-1);
		}
		else {
			ctx->state.st[ST0].exponent = (sx86_uword)(e);
		}

		return 1;
	}
/* FRNDINT */
	else if (opcode == 0xFC) {
		int TOP;

		TOP = SX87_FPUSTAT_TOP(ctx->state.status_word);
		sx87_rnd(&ctx->state.st[TOP]);
		return 1;
	}
/* FINCSTP */
	else if (opcode == 0xF7) {
		int TOP;

		TOP = SX87_FPUSTAT_TOP(ctx->state.status_word);
		ctx->state.status_word &= ~SX87_FPUSTAT_C1;
/* decrement Top Of Stack. nothing is tagged empty. */
		TOP = (TOP+1)&7;
		SX87_FPUSTAT_TOP_SET(ctx->state.status_word,TOP);
/* done */
		return 1;
	}
/* FDECSTP */
	else if (opcode == 0xF6) {
		int TOP;

		TOP = SX87_FPUSTAT_TOP(ctx->state.status_word);
		ctx->state.status_word &= ~SX87_FPUSTAT_C1;
/* decrement Top Of Stack. nothing is tagged empty. */
		TOP = (TOP-1)&7;
		SX87_FPUSTAT_TOP_SET(ctx->state.status_word,TOP);
/* done */
		return 1;
	}
/* F2XM1 */
	else if (opcode == 0xF0) {
		int TOP;
		double v;

		TOP = SX87_FPUSTAT_TOP(ctx->state.status_word);
		v = softx87_get_fpu_register_double(ctx,TOP,NULL);
		/* NOTE: Intel says that this instruction is limited to domain -1.0 <= x <= 1.0
		         and anything outside that is undefined..... so what? */
		// TODO: Make own implementation of exponential function
		v = pow(2.0,v);
		softx87_set_fpu_register_double(ctx,TOP,v);
		return 1;
	}
/* FLD1 */
	else if (opcode == 0xE8) {
		int TOP;

		TOP = SX87_FPUSTAT_TOP(ctx->state.status_word);
/* set C1 if stack overflow */
		if (SX87_FPUTAGW_TAG(ctx->state.tag_word,TOP) != SX87_FPUTAGVAL_EMPTY)
			ctx->state.status_word |=  SX87_FPUSTAT_C1;
		else
			ctx->state.status_word &= ~SX87_FPUSTAT_C1;
/* decrement TOP, tag as valid */
		TOP = (TOP-1)&7;
		SX87_FPUSTAT_TOP_SET(ctx->state.status_word,TOP);
		SX87_FPUTAGW_TAG_SET(ctx->state.tag_word,TOP,SX87_FPUTAGVAL_VALID);
/* store */
		ctx->state.st[TOP].exponent =	16383 + 63;
		ctx->state.st[TOP].mantissa =	1;
		ctx->state.st[TOP].sign_bit =	0;
		return 1;
	}
/* FLDL2T */
	else if (opcode == 0xE9) {
		int TOP;

		TOP = SX87_FPUSTAT_TOP(ctx->state.status_word);
/* set C1 if stack overflow */
		if (SX87_FPUTAGW_TAG(ctx->state.tag_word,TOP) != SX87_FPUTAGVAL_EMPTY)
			ctx->state.status_word |=  SX87_FPUSTAT_C1;
		else
			ctx->state.status_word &= ~SX87_FPUSTAT_C1;
/* decrement TOP, tag as valid */
		TOP = (TOP-1)&7;
		SX87_FPUSTAT_TOP_SET(ctx->state.status_word,TOP);
		SX87_FPUTAGW_TAG_SET(ctx->state.tag_word,TOP,SX87_FPUTAGVAL_VALID);
/* store log2(10) = 3.3219280948873623478703194294894 */
		softx87_set_fpu_register_double(ctx,TOP,(double)(3.3219280948873623478703194294894));
		return 1;
	}
/* FLDL2E */
	else if (opcode == 0xEA) {
		int TOP;

		TOP = SX87_FPUSTAT_TOP(ctx->state.status_word);
/* set C1 if stack overflow */
		if (SX87_FPUTAGW_TAG(ctx->state.tag_word,TOP) != SX87_FPUTAGVAL_EMPTY)
			ctx->state.status_word |=  SX87_FPUSTAT_C1;
		else
			ctx->state.status_word &= ~SX87_FPUSTAT_C1;
/* decrement TOP, tag as valid */
		TOP = (TOP-1)&7;
		SX87_FPUSTAT_TOP_SET(ctx->state.status_word,TOP);
		SX87_FPUTAGW_TAG_SET(ctx->state.tag_word,TOP,SX87_FPUTAGVAL_VALID);
/* store log2(e) = 1.4426950408889634073599246810019 */
		softx87_set_fpu_register_double(ctx,TOP,(double)(1.4426950408889634073599246810019));
		return 1;
	}
/* FLDPI */
	else if (opcode == 0xEB) {
		int TOP;

		TOP = SX87_FPUSTAT_TOP(ctx->state.status_word);
/* set C1 if stack overflow */
		if (SX87_FPUTAGW_TAG(ctx->state.tag_word,TOP) != SX87_FPUTAGVAL_EMPTY)
			ctx->state.status_word |=  SX87_FPUSTAT_C1;
		else
			ctx->state.status_word &= ~SX87_FPUSTAT_C1;
/* decrement TOP, tag as valid */
		TOP = (TOP-1)&7;
		SX87_FPUSTAT_TOP_SET(ctx->state.status_word,TOP);
		SX87_FPUTAGW_TAG_SET(ctx->state.tag_word,TOP,SX87_FPUTAGVAL_VALID);
/* store PI the way Intel documents it (IA-32 Intel Architecture SW dev vol 1, ch 8.3.8) */
		ctx->state.st[TOP].exponent =	16383 + 1;
		ctx->state.st[TOP].mantissa =	(sx86_uldword)(0xC90FDAA22168C234);	// C90F DAA2 2168 C234 C
		ctx->state.st[TOP].sign_bit =	0;
		return 1;
	}
/* FLDLG2 */
	else if (opcode == 0xEC) {
		int TOP;

		TOP = SX87_FPUSTAT_TOP(ctx->state.status_word);
/* set C1 if stack overflow */
		if (SX87_FPUTAGW_TAG(ctx->state.tag_word,TOP) != SX87_FPUTAGVAL_EMPTY)
			ctx->state.status_word |=  SX87_FPUSTAT_C1;
		else
			ctx->state.status_word &= ~SX87_FPUSTAT_C1;
/* decrement TOP, tag as valid */
		TOP = (TOP-1)&7;
		SX87_FPUSTAT_TOP_SET(ctx->state.status_word,TOP);
		SX87_FPUTAGW_TAG_SET(ctx->state.tag_word,TOP,SX87_FPUTAGVAL_VALID);
/* store log10(2) = 0.30102999566398119521373889472449 */
		softx87_set_fpu_register_double(ctx,TOP,(double)(0.30102999566398119521373889472449));
		return 1;
	}
/* FLDLN2 */
	else if (opcode == 0xED) {
		int TOP;

		TOP = SX87_FPUSTAT_TOP(ctx->state.status_word);
/* set C1 if stack overflow */
		if (SX87_FPUTAGW_TAG(ctx->state.tag_word,TOP) != SX87_FPUTAGVAL_EMPTY)
			ctx->state.status_word |=  SX87_FPUSTAT_C1;
		else
			ctx->state.status_word &= ~SX87_FPUSTAT_C1;
/* decrement TOP, tag as valid */
		TOP = (TOP-1)&7;
		SX87_FPUSTAT_TOP_SET(ctx->state.status_word,TOP);
		SX87_FPUTAGW_TAG_SET(ctx->state.tag_word,TOP,SX87_FPUTAGVAL_VALID);
/* store ln(2) = 0.69314718055994530941723212145818 */
		softx87_set_fpu_register_double(ctx,TOP,(double)(0.69314718055994530941723212145818));
		return 1;
	}
/* FLDZ */
	else if (opcode == 0xEE) {
		int TOP;

		TOP = SX87_FPUSTAT_TOP(ctx->state.status_word);
/* set C1 if stack overflow */
		if (SX87_FPUTAGW_TAG(ctx->state.tag_word,TOP) != SX87_FPUTAGVAL_EMPTY)
			ctx->state.status_word |=  SX87_FPUSTAT_C1;
		else
			ctx->state.status_word &= ~SX87_FPUSTAT_C1;
/* decrement TOP, tag as valid */
		TOP = (TOP-1)&7;
		SX87_FPUSTAT_TOP_SET(ctx->state.status_word,TOP);
		SX87_FPUTAGW_TAG_SET(ctx->state.tag_word,TOP,SX87_FPUTAGVAL_VALID);
/* store */
		ctx->state.st[TOP].exponent =	16383 + 63;
		ctx->state.st[TOP].mantissa =	0;
		ctx->state.st[TOP].sign_bit =	0;
		return 1;
	}
/* FABS */
	else if (opcode == 0xE1) {
		int TOP;

		TOP = SX87_FPUSTAT_TOP(ctx->state.status_word);
		ctx->state.st[TOP].sign_bit = 0;
		return 1;
	}
/* FCHS */
	else if (opcode == 0xE0) {
		int TOP;

		TOP = SX87_FPUSTAT_TOP(ctx->state.status_word);
		ctx->state.st[TOP].sign_bit = !ctx->state.st[TOP].sign_bit;
		return 1;
	}
/* FNOP */
	else if (opcode == 0xD0) {
		/* no-op */
		return 1;
	}
/* FLD ST(i) */
	else if (opcode >= 0xC0 && opcode < 0xC8) {
		int TOP,i;

		i   = (int)(opcode-0xC0);
		i   = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),i);
		TOP = SX87_FPUSTAT_TOP(ctx->state.status_word);
/* set C1 if stack overflow */
		if (SX87_FPUTAGW_TAG(ctx->state.tag_word,TOP) != SX87_FPUTAGVAL_EMPTY)
			ctx->state.status_word |=  SX87_FPUSTAT_C1;
		else
			ctx->state.status_word &= ~SX87_FPUSTAT_C1;
/* decrement Top Of Stack, Tag as valid, and store */
		TOP = (TOP-1)&7;
		SX87_FPUSTAT_TOP_SET(ctx->state.status_word,TOP);
		SX87_FPUTAGW_TAG_SET(ctx->state.tag_word,TOP,SX87_FPUTAGVAL_VALID);
/* copy */
		ctx->state.st[TOP].exponent =	ctx->state.st[i].exponent;
		ctx->state.st[TOP].mantissa =	ctx->state.st[i].mantissa;
		ctx->state.st[TOP].sign_bit =	ctx->state.st[i].sign_bit;
/* done */
		return 1;
	}
/* combo */
	else if (opcode < 0xC0) {
		sx86_ubyte mod,reg,rm;

		sx86_modregrm_unpack(opcode,mod,reg,rm);

		if (reg == 0)	/* FLD mem32 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,4,op_fld32);
		else
			return 0;

		return 1;
	}

	return 0;
}

static int Sfx87OpcodeDec_group_D9(sx87_ubyte opcode,softx87_ctx* ctx,char buf[128])
{
/* FSCALE */
	if (opcode == 0xFD) {
		strcpy(buf,"FSCALE");
		return 1;
	}
/* FRNDINT */
	else if (opcode == 0xFC) {
		strcpy(buf,"FRNDINT");
		return 1;
	}
/* FINCSTP */
	else if (opcode == 0xF7) {
		strcpy(buf,"FINCSTP");
		return 1;
	}
/* FDECSTP */
	else if (opcode == 0xF6) {
		strcpy(buf,"FDECSTP");
		return 1;
	}
/* F2XM1 */
	else if (opcode == 0xF0) {
		strcpy(buf,"F2XM1");
		return 1;
	}
/* FLD1 */
	else if (opcode == 0xE8) {
		strcpy(buf,"FLD1");
		return 1;
	}
/* FLDL2T */
	else if (opcode == 0xE9) {
		strcpy(buf,"FLDL2T");
		return 1;
	}
/* FLDL2E */
	else if (opcode == 0xEA) {
		strcpy(buf,"FLDL2E");
		return 1;
	}
/* FLDPI */
	else if (opcode == 0xEB) {
		strcpy(buf,"FLDPI");
		return 1;
	}
/* FLDLG2 */
	else if (opcode == 0xEC) {
		strcpy(buf,"FLDLG2");
		return 1;
	}
/* FLDLN2 */
	else if (opcode == 0xED) {
		strcpy(buf,"FLDLN2");
		return 1;
	}
/* FLDZ */
	else if (opcode == 0xEE) {
		strcpy(buf,"FLDZ");
		return 1;
	}
/* FABS */
	else if (opcode == 0xE1) {
		strcpy(buf,"FABS");
		return 1;
	}
/* FCHS */
	else if (opcode == 0xE0) {
		strcpy(buf,"FABS");
		return 1;
	}
/* FNOP */
	else if (opcode == 0xD0) {
		strcpy(buf,"FNOP");
		return 1;
	}
/* FLD ST(i) */
	else if (opcode >= 0xC0 && opcode < 0xC8) {
		sprintf(buf,"FLD ST(%d)",opcode-0xC0);
		return 1;
	}
/* combo */
	else if (opcode < 0xC0) {
		sx86_ubyte mod,reg,rm;

		sx86_modregrm_unpack(opcode,mod,reg,rm);
		ctx->callbacks.on_sx86_dec_full_modrmonly(ctx->ref_softx86,1,0,mod,rm,s87op1_tmp);

		if (reg == 0)
			sprintf(buf,"FLD %s (mem32)",s87op1_tmp);
		else
			return 0;

		return 1;
	}

	buf[0]=0;
	return 0;
}

static int Sfx87OpcodeExec_group_DA(sx87_ubyte opcode,softx87_ctx* ctx)
{
/* combo */
	if (opcode < 0xC0) {
		sx86_ubyte mod,reg,rm;

		sx86_modregrm_unpack(opcode,mod,reg,rm);

		if (reg == 0)		/* FIADD mem32 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,4,op_fiadd32);
		else if (reg == 2)	/* FICOM mem32 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,4,op_ficom32);
		else if (reg == 3)	/* FICOMP mem32 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,4,op_ficomp32);
		else if (reg == 4)	/* FISUB mem32 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,4,op_fisub32);
		else if (reg == 6)	/* FIDIV mem32 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,4,op_fidiv32);
		else if (reg == 7)	/* FIDIVR mem32 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,4,op_fidivr32);
		else
			return 0;

		return 1;
	}

	return 0;
}

static int Sfx87OpcodeDec_group_DA(sx87_ubyte opcode,softx87_ctx* ctx,char buf[128])
{
/* combo */
	if (opcode < 0xC0) {
		sx86_ubyte mod,reg,rm;

		sx86_modregrm_unpack(opcode,mod,reg,rm);
		ctx->callbacks.on_sx86_dec_full_modrmonly(ctx->ref_softx86,1,0,mod,rm,s87op1_tmp);

		if (reg == 0)
			sprintf(buf,"FIADD %s (mem32)",s87op1_tmp);
		else if (reg == 2)
			sprintf(buf,"FICOM %s (mem32)",s87op1_tmp);
		else if (reg == 3)
			sprintf(buf,"FICOMP %s (mem32)",s87op1_tmp);
		else if (reg == 4)
			sprintf(buf,"FISUB %s (mem32)",s87op1_tmp);
		else if (reg == 6)
			sprintf(buf,"FIDIV %s (mem32)",s87op1_tmp);
		else if (reg == 7)
			sprintf(buf,"FIDIVR %s (mem32)",s87op1_tmp);
		else
			return 0;

		return 1;
	}

	buf[0]=0;
	return 0;
}

static int Sfx87OpcodeExec_group_DB(sx87_ubyte opcode,softx87_ctx* ctx)
{
/* FINIT a.k.a. FNINIT */
	if (opcode == 0xE3) {
		softx87_finit_setup(ctx);
		return 1;
	}
/* FCLEX a.k.a. FNCLEX */
	else if (opcode == 0xE2) {
		ctx->state.status_word = 0;
		return 1;
	}

	return 0;
}

static int Sfx87OpcodeDec_group_DB(sx87_ubyte opcode,softx87_ctx* ctx,char buf[128])
{
/* FINIT a.k.a. FNINIT */
	if (opcode == 0xE3) {
		strcpy(buf,"FINIT");
		return 1;
	}
/* FCLEX a.k.a. FNCLEX */
	else if (opcode == 0xE2) {
		strcpy(buf,"FCLEX");
		return 1;
	}

	buf[0]=0;
	return 0;
}

static int Sfx87OpcodeExec_group_DC(sx87_ubyte opcode,softx87_ctx* ctx)
{
/* FADD ST(i),ST(0) */
	if (opcode >= 0xC0 && opcode < 0xC8) {
		int st0,i;

		i   = (int)(opcode-0xC0);
		i   = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),i);
		st0 = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),0);
/* add */
		op_fadd(ctx,&ctx->state.st[i],&ctx->state.st[st0]);
/* done */
		return 1;
	}
/* FSUB ST(i),ST(0) */
	else if (opcode >= 0xE8 && opcode < 0xF0) {
		int st0,i;

		i   = (int)(opcode-0xE8);
		i   = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),i);
		st0 = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),0);
/* add */
		op_fsub(ctx,&ctx->state.st[i],&ctx->state.st[st0]);
/* done */
		return 1;
	}
/* FDIVR ST(i),ST(0) */
	else if (opcode >= 0xF0 && opcode < 0xF8) {
		int st0,i;

		i   = (int)(opcode-0xF0);
		i   = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),i);
		st0 = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),0);
/* add */
		op_fdivr(ctx,&ctx->state.st[i],&ctx->state.st[st0]);
/* done */
		return 1;
	}
/* FDIV ST(i),ST(0) */
	else if (opcode >= 0xF8) {
		int st0,i;

		i   = (int)(opcode-0xF8);
		i   = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),i);
		st0 = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),0);
/* add */
		op_fdiv(ctx,&ctx->state.st[i],&ctx->state.st[st0]);
/* done */
		return 1;
	}
/* combo */
	else if (opcode < 0xC0) {
		sx86_ubyte mod,reg,rm;

		sx86_modregrm_unpack(opcode,mod,reg,rm);

		if (reg == 0)		/* FADD mem64 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,8,op_fadd64);
		else if (reg == 2)	/* FCOM mem64 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,8,op_fcom64);
		else if (reg == 3)	/* FCOMP mem64 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,8,op_fcomp64);
		else if (reg == 4)	/* FSUB mem64 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,8,op_fsub64);
		else if (reg == 6)	/* FDIV mem64 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,8,op_fdiv64);
		else if (reg == 7)	/* FDIVR mem64 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,8,op_fdivr64);
		else
			return 0;

		return 1;
	}

	return 0;
}

static int Sfx87OpcodeDec_group_DC(sx87_ubyte opcode,softx87_ctx* ctx,char buf[128])
{
/* FADD ST(i),ST(0) */
	if (opcode >= 0xC0 && opcode < 0xC8) {
		sprintf(buf,"FADD ST(%d),ST(0)",opcode-0xC0);
		return 1;
	}
/* FSUB ST(i),ST(0) */
	else if (opcode >= 0xE8 && opcode < 0xF0) {
		sprintf(buf,"FSUB ST(%d),ST(0)",opcode-0xE8);
		return 1;
	}
/* FDIVR ST(i),ST(0) */
	else if (opcode >= 0xF0 && opcode < 0xF8) {
		sprintf(buf,"FDIVR ST(%d),ST(0)",opcode-0xF0);
		return 1;
	}
/* FDIV ST(i),ST(0) */
	else if (opcode >= 0xF8) {
		sprintf(buf,"FDIV ST(%d),ST(0)",opcode-0xF8);
		return 1;
	}
/* combo */
	else if (opcode < 0xC0) {
		sx86_ubyte mod,reg,rm;

		sx86_modregrm_unpack(opcode,mod,reg,rm);
		ctx->callbacks.on_sx86_dec_full_modrmonly(ctx->ref_softx86,1,0,mod,rm,s87op1_tmp);

		if (reg == 0)
			sprintf(buf,"FADD %s (mem64)",s87op1_tmp);
		else if (reg == 2)
			sprintf(buf,"FCOM %s (mem64)",s87op1_tmp);
		else if (reg == 3)
			sprintf(buf,"FCOMP %s (mem64)",s87op1_tmp);
		else if (reg == 4)
			sprintf(buf,"FSUB %s (mem64)",s87op1_tmp);
		else if (reg == 6)
			sprintf(buf,"FDIV %s (mem64)",s87op1_tmp);
		else if (reg == 7)
			sprintf(buf,"FDIVR %s (mem64)",s87op1_tmp);
		else
			return 0;

		return 1;
	}

	buf[0]=0;
	return 0;
}

static int Sfx87OpcodeExec_group_DD(sx87_ubyte opcode,softx87_ctx* ctx)
{
/* combo */
	if (opcode < 0xC0) {
		sx86_ubyte mod,reg,rm;

		sx86_modregrm_unpack(opcode,mod,reg,rm);

		if (reg == 0)	/* FLD mem64 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,8,op_fld64);
		else
			return 0;

		return 1;
	}
/* FFREE ST(i) */
	else if (opcode >= 0xC0 && opcode < 0xC8) {
		int i;

		i = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),(int)(opcode-0xC0));
		SX87_FPUTAGW_TAG_SET(ctx->state.tag_word,i,SX87_FPUTAGVAL_EMPTY);

		return 1;
	}

	return 0;
}

static int Sfx87OpcodeDec_group_DD(sx87_ubyte opcode,softx87_ctx* ctx,char buf[128])
{
/* combo */
	if (opcode < 0xC0) {
		sx86_ubyte mod,reg,rm;

		sx86_modregrm_unpack(opcode,mod,reg,rm);
		ctx->callbacks.on_sx86_dec_full_modrmonly(ctx->ref_softx86,1,0,mod,rm,s87op1_tmp);

		if (reg == 0)
			sprintf(buf,"FLD %s (mem64)",s87op1_tmp);
		else
			return 0;

		return 1;
	}
/* FFREE ST(i) */
	else if (opcode >= 0xC0 && opcode < 0xC8) {
		sprintf(buf,"FFREE ST(%d)",opcode-0xC0);
		return 1;
	}

	buf[0]=0;
	return 0;
}

static int Sfx87OpcodeExec_group_DE(sx87_ubyte opcode,softx87_ctx* ctx)
{
/* FADDP ST(i),ST(0) */
	if (opcode >= 0xC0 && opcode < 0xC8) {
		int st0,i;

		i   = (int)(opcode-0xC0);
		i   = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),i);
		st0 = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),0);
/* add */
		op_fadd(ctx,&ctx->state.st[i],&ctx->state.st[st0]);
/* pop */
		softx86_popstack(ctx);
/* done */
		return 1;
	}
/* FCOMPP ST(0),ST(1) */
	else if (opcode == 0xD9) {
		int st0,i;

		i   = (int)(opcode-0xD9);
		i   = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),i);
		st0 = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),0);
/* add */
		op_fcom(ctx,&ctx->state.st[i],&ctx->state.st[st0]);
/* pop */
		softx86_popstack(ctx);
		softx86_popstack(ctx);
/* done */
		return 1;
	}
/* FSUBP ST(i),ST(0) */
	else if (opcode >= 0xE8 && opcode < 0xEF) {
		int st0,i;

		i   = (int)(opcode-0xE8);
		i   = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),i);
		st0 = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),0);
/* add */
		op_fsub(ctx,&ctx->state.st[i],&ctx->state.st[st0]);
/* pop */
		softx86_popstack(ctx);
/* done */
		return 1;
	}
/* FDIVRP ST(i),ST(0) */
	else if (opcode >= 0xF0 && opcode < 0xF8) {
		int st0,i;

		i   = (int)(opcode-0xF0);
		i   = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),i);
		st0 = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),0);
/* add */
		op_fdivr(ctx,&ctx->state.st[i],&ctx->state.st[st0]);
/* pop */
		softx86_popstack(ctx);
/* done */
		return 1;
	}
/* FDIVP ST(i),ST(0) */
	else if (opcode >= 0xF8) {
		int st0,i;

		i   = (int)(opcode-0xF8);
		i   = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),i);
		st0 = SX87_FPU_ST(SX87_FPUSTAT_TOP(ctx->state.status_word),0);
/* add */
		op_fdiv(ctx,&ctx->state.st[i],&ctx->state.st[st0]);
/* pop */
		softx86_popstack(ctx);
/* done */
		return 1;
	}
/* combo */
	else if (opcode < 0xC0) {
		sx86_ubyte mod,reg,rm;

		sx86_modregrm_unpack(opcode,mod,reg,rm);

		if (reg == 0)		/* FIADD mem16 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,2,op_fiadd16);
		else if (reg == 2)	/* FICOM mem16 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,2,op_ficom16);
		else if (reg == 3)	/* FICOMP mem16 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,2,op_ficomp16);
		else if (reg == 4)	/* FISUB mem16 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,2,op_fisub16);
		else if (reg == 6)	/* FIDIV mem16 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,2,op_fidiv16);
		else if (reg == 7)	/* FIDIVR mem16 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,2,op_fidivr16);
		else
			return 0;

		return 1;
	}

	return 0;
}

static int Sfx87OpcodeDec_group_DE(sx87_ubyte opcode,softx87_ctx* ctx,char buf[128])
{
/* FADDP ST(i),ST(0) */
	if (opcode >= 0xC0 && opcode < 0xC8) {
		sprintf(buf,"FADDP ST(%d),ST(0)",opcode-0xC0);
		return 1;
	}
/* FCOMPP ST(0),ST(1) */
	else if (opcode == 0xD9) {
		strcpy(buf,"FCOMPP");
		return 1;
	}
/* FSUBP ST(i),ST(0) */
	else if (opcode >= 0xE8 && opcode < 0xEF) {
		sprintf(buf,"FSUBP ST(%d),ST(0)",opcode-0xE8);
		return 1;
	}
/* FDIVP ST(i),ST(0) */
	else if (opcode >= 0xF8) {
		sprintf(buf,"FDIVP ST(%d),ST(0)",opcode-0xF8);
		return 1;
	}
/* combo */
	else if (opcode < 0xC0) {
		sx86_ubyte mod,reg,rm;

		sx86_modregrm_unpack(opcode,mod,reg,rm);
		ctx->callbacks.on_sx86_dec_full_modrmonly(ctx->ref_softx86,1,0,mod,rm,s87op1_tmp);

		if (reg == 0)
			sprintf(buf,"FIADD %s (mem16)",s87op1_tmp);
		else if (reg == 2)
			sprintf(buf,"FICOM %s (mem16)",s87op1_tmp);
		else if (reg == 3)
			sprintf(buf,"FICOMP %s (mem16)",s87op1_tmp);
		else if (reg == 4)
			sprintf(buf,"FISUB %s (mem16)",s87op1_tmp);
		else if (reg == 6)
			sprintf(buf,"FIDIV %s (mem16)",s87op1_tmp);
		else if (reg == 7)
			sprintf(buf,"FIDIVR %s (mem16)",s87op1_tmp);
		else
			return 0;

		return 1;
	}

	buf[0]=0;
	return 0;
}

static int Sfx87OpcodeExec_group_DF(sx87_ubyte opcode,softx87_ctx* ctx)
{
/* combo */
	if (opcode < 0xC0) {
		sx86_ubyte mod,reg,rm;

		sx86_modregrm_unpack(opcode,mod,reg,rm);

		if (reg == 4)		/* FBLD mem80 */
			ctx->callbacks.on_sx86_exec_full_modrmonly_memx(ctx->ref_softx86,mod,rm,10,op_fbld80);
		else if (reg == 6)	/* FBSTP mem80 */
			ctx->callbacks.on_sx86_exec_full_modrw_memx(ctx->ref_softx86,mod,rm,10,op_fbstp80);
		else
			return 0;

		return 1;
	}

	return 0;
}

static int Sfx87OpcodeDec_group_DF(sx87_ubyte opcode,softx87_ctx* ctx,char buf[128])
{
/* combo */
	if (opcode < 0xC0) {
		sx86_ubyte mod,reg,rm;

		sx86_modregrm_unpack(opcode,mod,reg,rm);
		ctx->callbacks.on_sx86_dec_full_modrmonly(ctx->ref_softx86,1,0,mod,rm,s87op1_tmp);

		if (reg == 4)
			sprintf(buf,"FBLD %s (mem80)",s87op1_tmp);
		else if (reg == 6)
			sprintf(buf,"FBSTP %s (mem80)",s87op1_tmp);
		else
			return 0;

		return 1;
	}

	buf[0]=0;
	return 0;
}

Sfx87OpcodeTable optab8087 = {
{
	{Sfx87OpcodeExec_group_D8,	Sfx87OpcodeDec_group_D8},				/* 0xD8xx */
	{Sfx87OpcodeExec_group_D9,	Sfx87OpcodeDec_group_D9},				/* 0xD9xx */
	{Sfx87OpcodeExec_group_DA,	Sfx87OpcodeDec_group_DA},				/* 0xDAxx */
	{Sfx87OpcodeExec_group_DB,	Sfx87OpcodeDec_group_DB},				/* 0xDBxx */
	{Sfx87OpcodeExec_group_DC,	Sfx87OpcodeDec_group_DC},				/* 0xDCxx */
	{Sfx87OpcodeExec_group_DD,	Sfx87OpcodeDec_group_DD},				/* 0xDDxx */
	{Sfx87OpcodeExec_group_DE,	Sfx87OpcodeDec_group_DE},				/* 0xDExx */
	{Sfx87OpcodeExec_group_DF,	Sfx87OpcodeDec_group_DF},				/* 0xDFxx */
},
};
