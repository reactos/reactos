/*
 * softx87.c
 *
 * Copyright (C) 2003, 2004 Jonathan Campbell <jcampbell@mdjk.com>
 * 
 * Softx87 library API.
 *
 * Initialization, de-initialization, register modification, and default
 * routines for callbacks (in case they weren't set by the host app).
 *
 * Internal to this library, there are also functions to take care of
 * fetching the current opcode (as referred to by CS:IP), fetching
 * the current opcode for the decompiler (as referred to by a separate
 * pair CS:IP), and pushing/popping data to/from the stack (SS:SP).
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
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "optab87.h"

/* utility function to normalize a floating point value */
void softx87_normalize(softx87_ctx* ctx,softx87_reg80 *val)
{
	int lead0;
	sx87_uldword sh;

	if (!val->mantissa)
		return;

/* how many leading 0 bits in the mantissa */
	lead0=0;
	sh=0;
	while (!((val->mantissa>>(63-sh))&1)) {
		lead0++;
		sh++;
	}

/* if none, don't do anything */
	if (!sh)
		return;

/* normalize */
	val->exponent  -= sh;
	val->mantissa <<= sh;
}


/* API for converting an FPU register to a type "double" value.
   Yes, it is an approximation. Don't take it as the exact value. */
double softx87_get_fpu_double(softx87_ctx* ctx,softx87_reg80 *reg,int *numtype)
{
	double val;
	int exp;

/* all special cases seem to revolve around exp == 0x7FFF */
	if (reg->exponent == 0x7FFF) {
		if (reg->mantissa == 0) {
			if (reg->sign_bit) {
				if (numtype)
					*numtype = SX87_FPU_NUMTYPE_NEGINF;

				return 0.00;
			}
			else {
				if (numtype)
					*numtype = SX87_FPU_NUMTYPE_POSINF;

				return 0.00;
			}
		}
		else {
			if (numtype)
				*numtype = SX87_FPU_NUMTYPE_NAN;

			return 0.00;
		}
	}

/* 16383 is the IEEE-754 bias for double extended precision numbers */
	exp  = reg->exponent - (16383+63);
#ifdef MSVC_CANT_CONVERT_UI64_TO_DOUBLE
	if ((reg->mantissa >> ((sx87_uldword)63))&1)
		/* +2^64 to encourage positive value */
		val = ((double)((sx87_sldword)reg->mantissa)) + 18446744073709551616.00;
	else
		val =  (double)((sx87_sldword)reg->mantissa);
#else
	val  = (double)(reg->mantissa);
#endif
	val *= pow(2,(double)exp);

	if (numtype)
		*numtype = SX87_FPU_NUMTYPE_NUMBER;

	if (reg->sign_bit)
		val = -val;

	return val;
}

double softx87_get_fpu_register_double(softx87_ctx* ctx,int i,int *numtype)
{
	if (i < 0 || i > 7) {
		if (numtype)
			*numtype = SX87_FPU_NUMTYPE_NAN;

		return 0.00;
	}

	return softx87_get_fpu_double(ctx,&ctx->state.st[i],numtype);
}

/* API for converting a type "double" value to an FPU register.
   Yes, it is an approximation. Don't take it as the exact value. */
void softx87_set_fpu_double(softx87_ctx* ctx,softx87_reg80 *reg,double val)
{
/* log2 x = ln x / ln 2 */
	double sal;
	int l2b;
	int sgn;

	if (val < 0) {
		sgn = 1;
		val = -val;
	}
	else {
		sgn = 0;
	}

	if (val == 0) {
		reg->sign_bit =	sgn;
		reg->exponent =	0;
		reg->mantissa =	0;
		return;
	}

	l2b  = (int)ceil(log(val)/log(2.0));
/* calculate just so to produce a normalized value */
	sal  = val * pow(2,(double)(63-l2b));
	l2b  = 16383 + l2b;
/* store */
	reg->sign_bit =		sgn;
	reg->exponent =		l2b;
	reg->mantissa =		(sx87_uldword)sal;
}

void softx87_set_fpu_register_double(softx87_ctx* ctx,int i,double val)
{
	if (i < 0 || i > 7)
		return;

	softx87_set_fpu_double(ctx,&ctx->state.st[i],val);
}

/* callbacks intended to be called by softx86 */
int softx87_on_fpu_opcode_exec(/* softx86_ctx */ void* _ctx86,/* softx87_ctx */ void* _ctx87,sx86_ubyte opcode)
{
	Sfx87OpcodeTable *sop;
	softx86_ctx *ctx86;
	softx87_ctx *ctx87;
	sx87_ubyte op2;

/* sanity check */
	ctx86 = (softx86_ctx*)_ctx86;
	ctx87 = (softx87_ctx*)_ctx87;
	if (!_ctx86 || !_ctx87) return 0;
	if (opcode < 0xD8 || opcode > 0xDF) return 0;
	sop = (Sfx87OpcodeTable*)ctx87->opcode_table;

	op2 = ctx87->callbacks.on_softx86_fetch_exec_byte(ctx86);
	return sop->table[opcode-0xD8].exec(op2,ctx87);
}

int softx87_on_fpu_opcode_dec(/* softx86_ctx */ void* _ctx86,/* softx87_ctx */ void* _ctx87,sx86_ubyte opcode,char buf[128])
{
	Sfx87OpcodeTable *sop;
	softx86_ctx *ctx86;
	softx87_ctx *ctx87;
	sx87_ubyte op2;

/* sanity check */
	ctx86 = (softx86_ctx*)_ctx86;
	ctx87 = (softx87_ctx*)_ctx87;
	if (!_ctx86 || !_ctx87) return 0;
	if (opcode < 0xD8 || opcode > 0xDF) return 0;
	sop = (Sfx87OpcodeTable*)ctx87->opcode_table;

	op2 = ctx87->callbacks.on_softx86_fetch_dec_byte(ctx86);
	return sop->table[opcode-0xD8].dec(op2,ctx87,buf);
}

void softx87_finit_setup(softx87_ctx* ctx)
{
	softx87_set_fpu_register_double(ctx,0,0.00);
	softx87_set_fpu_register_double(ctx,1,0.00);
	softx87_set_fpu_register_double(ctx,2,0.00);
	softx87_set_fpu_register_double(ctx,3,0.00);
	softx87_set_fpu_register_double(ctx,4,0.00);
	softx87_set_fpu_register_double(ctx,5,0.00);
	softx87_set_fpu_register_double(ctx,6,0.00);
	softx87_set_fpu_register_double(ctx,7,0.00);
	ctx->state.control_word =			0x037F;
	ctx->state.status_word =			0x0000;
	ctx->state.tag_word =				0xFFFF;	// all empty
	ctx->state.last_instruction_memptr.offset =	0;
	ctx->state.last_instruction_memptr.segment =	0;
	ctx->state.data_pointer.offset =		0;
	ctx->state.data_pointer.segment	=		0;
	ctx->state.last_opcode =			0;
}

/* softx87_reset(context)
   Resets a FPU

   return value:
   0 = failed
   1 = success */
int softx87_reset(softx87_ctx* ctx)
{
	if (!ctx) return 0;

/* switch on/off bugs */
	ctx->bugs.ip_ignores_prefix =
		(ctx->level <= SX87_FPULEVEL_8087)?1:0;

/* opcode table */
	ctx->opcode_table = (void*)(&optab8087);

/* set up as if FINIT */
	softx87_finit_setup(ctx);

	return 1; /* success */
}

int softx87_getversion(int *major,int *minor,int *subminor)
{
	if (!minor || !major || !subminor) return 0;

	*major =	SOFTX87_VERSION_HI;
	*minor =	SOFTX87_VERSION_LO;
	*subminor =	SOFTX87_VERSION_SUBLO;
	return 1;
}

/* softx87_init(context)
   Initialize a FPU context structure.

   return value:
   0 = failed
   1 = success
   2 = beta development advisory (FPU level emulation not quite stable) */
int softx87_init(softx87_ctx* ctx,int level)
{
	int ret;

	ret=1;
	if (!ctx) return 0;
	if (level > SX87_FPULEVEL_8087) return 0; /* we currently support up to the 8087 */
	if (level < 0) return 0; /* apparently the host wants an 80(-1)87? :) */
//	if (level > SX87_FPULEVEL_8087) ret=2;	/* 80287 or higher emulation is not stable yet */
	ctx->level = level;

	if (!softx87_reset(ctx)) return 0;

/* store version in the structure */
	ctx->version_hi =		SOFTX87_VERSION_HI;
	ctx->version_lo =		SOFTX87_VERSION_LO;
	ctx->version_sublo =		SOFTX87_VERSION_SUBLO;

/* default callbacks */
	ctx->callbacks.on_read_memory			= softx87_step_def_on_read_memory;
	ctx->callbacks.on_write_memory			= softx87_step_def_on_write_memory;
	ctx->callbacks.on_softx86_fetch_exec_byte	= softx87_def_on_softx86_fetch_exec_byte;
	ctx->callbacks.on_softx86_fetch_dec_byte	= softx87_def_on_softx86_fetch_dec_byte;
	ctx->callbacks.on_sx86_exec_full_modrmonly_memx	= softx87_on_sx86_exec_full_modrmonly_memx;
	ctx->callbacks.on_sx86_dec_full_modrmonly	= softx87_on_sx86_dec_full_modrmonly;

	return ret; /* success */
}

/* softx86_free(context)
   Free a FPU context structure */
int softx87_free(softx87_ctx* ctx)
{
	if (!ctx) return 0;

	return 1; /* success */
}

/* enable/disable emulation of specific bugs. */
int softx87_setbug(softx87_ctx* ctx,sx86_udword bug_id,sx86_ubyte on_off)
{
	if (bug_id == SX87_BUG_IP_IGNORES_PREFIX) {
		ctx->bugs.ip_ignores_prefix = on_off;
		return 1;
	}

	return 0;
}

/*--------------------------------------------------------------------------------
  default callbacks
  --------------------------------------------------------------------------------*/

void softx87_on_sx86_exec_full_modrmonly_memx(softx86_ctx *ctx,sx86_ubyte mod,sx86_ubyte rm,int sz,void (*op64)(softx86_ctx* ctx,char *datz,int sz))
{
}

void softx87_on_sx86_dec_full_modrmonly(softx86_ctx* ctx,sx86_ubyte is_word,sx86_ubyte dat32,sx86_ubyte mod,sx86_ubyte rm,char* op1)
{
}

sx86_ubyte softx87_def_on_softx86_fetch_exec_byte(softx86_ctx* ctx)
{
	return 0xFF;
}

sx86_ubyte softx87_def_on_softx86_fetch_dec_byte(softx86_ctx* ctx)
{
	return 0xFF;
}

void softx87_step_def_on_read_memory(void* _ctx,sx86_udword address,sx86_ubyte *buf,int size)
{
	softx87_ctx *ctx = ((softx87_ctx*)_ctx);
	if (!ctx || !buf || size < 1) return;
	memset(buf,0xFF,size);
}

void softx87_step_def_on_write_memory(void* _ctx,sx86_udword address,sx86_ubyte *buf,int size)
{
	softx87_ctx *ctx = ((softx87_ctx*)_ctx);
	if (!ctx || !buf || size < 1) return;
}

/* loading/storing and conversion code */

/* loads 16-bit integer from data[] */
void softx87_unpack_raw_int16(softx87_ctx* ctx,sx87_ubyte *data,softx87_reg80 *v)
{
#if SX86_BYTE_ORDER == LE
	v->mantissa =	(sx87_uldword)(*((sx87_uword*)data));
#else
	v->mantissa  =	 (sx87_uldword)data[0];
	v->mantissa |=	((sx87_uldword)data[1])<<((sx87_uldword)8);
#endif

/* there. we have all 16 bits. spread them around. */
	v->sign_bit   =	(v->mantissa >> ((sx87_uldword)15));
	v->exponent   =	14;
	if (v->sign_bit) v->mantissa = ((v->mantissa ^ 0xFFFF)+1);
	v->mantissa <<= (sx87_uldword)49;

/* convert the exponent */
	v->exponent  =  v->exponent + 16383;
}

/* loads 32-bit integer from data[] */
void softx87_unpack_raw_int32(softx87_ctx* ctx,sx87_ubyte *data,softx87_reg80 *v)
{
#if SX86_BYTE_ORDER == LE
	v->mantissa =	(sx87_uldword)(*((sx87_udword*)data));
#else
	v->mantissa  =	 (sx87_uldword)data[3];
	v->mantissa |=	((sx87_uldword)data[2])<<((sx87_uldword)8);
	v->mantissa |=	((sx87_uldword)data[1])<<((sx87_uldword)16);
	v->mantissa |=	((sx87_uldword)data[0])<<((sx87_uldword)24);
#endif

/* there. we have all 32 bits. spread them around. */
	v->sign_bit   =	(v->mantissa >> ((sx87_uldword)31));
	v->exponent   =	30;
	if (v->sign_bit) v->mantissa = ((v->mantissa ^ 0xFFFFFFFF)+1);
	v->mantissa <<= (sx87_uldword)33;

/* convert the exponent */
	v->exponent  =  v->exponent + 16383;
}

/* loads 32-bit double precision floating point from data[] */
void softx87_unpack_raw_fp32(softx87_ctx* ctx,sx87_ubyte *data,softx87_reg80 *v)
{
#if SX86_BYTE_ORDER == LE
	v->mantissa =	(sx87_uldword)(*((sx87_udword*)data));
#else
	v->mantissa  =	 (sx87_uldword)data[3];
	v->mantissa |=	((sx87_uldword)data[2])<<((sx87_uldword)8);
	v->mantissa |=	((sx87_uldword)data[1])<<((sx87_uldword)16);
	v->mantissa |=	((sx87_uldword)data[0])<<((sx87_uldword)24);
#endif

/* there. we have all 32 bits. spread them around. */
	v->sign_bit   =	(v->mantissa >> ((sx87_uldword)31));
	v->exponent   =	(v->mantissa >> ((sx87_uldword)23))&0xFF;
	v->mantissa  &=	(sx87_uldword)(0x7FFFFF);
	v->mantissa  |=	(sx87_uldword)(0x800000);	/* implied "1"? */
	v->mantissa <<= (sx87_uldword)40;

/* convert the exponent */
	v->exponent  =  (v->exponent - 127) + 16383;
}

/* loads 64-bit double precision floating point from data[] */
void softx87_unpack_raw_fp64(softx87_ctx* ctx,sx87_ubyte *data,softx87_reg80 *v)
{
#if SX86_BYTE_ORDER == LE
	v->mantissa =	*((sx87_uldword*)data);
#else
	v->mantissa  =	 (sx87_uldword)data[7];
	v->mantissa |=	((sx87_uldword)data[6])<<((sx87_uldword)8);
	v->mantissa |=	((sx87_uldword)data[5])<<((sx87_uldword)16);
	v->mantissa |=	((sx87_uldword)data[4])<<((sx87_uldword)24);
	v->mantissa |=	((sx87_uldword)data[3])<<((sx87_uldword)32);
	v->mantissa |=	((sx87_uldword)data[2])<<((sx87_uldword)40);
	v->mantissa |=	((sx87_uldword)data[1])<<((sx87_uldword)48);
	v->mantissa |=	((sx87_uldword)data[0])<<((sx87_uldword)56);
#endif

/* there. we have all 64 bits. spread them around. */
	v->sign_bit   =	(v->mantissa >> ((sx87_uldword)63));
	v->exponent   =	(v->mantissa >> ((sx87_uldword)52))&0x7FF;
	v->mantissa  &=	(sx87_uldword)(0x0FFFFFFFFFFFFF);
	v->mantissa  |=	(sx87_uldword)(0x10000000000000);	/* implied "1"? */
	v->mantissa <<= (sx87_uldword)11;

/* convert the exponent */
	v->exponent  =  (v->exponent - 1023) + 16383;
}
