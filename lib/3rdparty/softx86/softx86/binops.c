/*
 * binops.c
 *
 * Copyright (C) 2003, 2004 Jonathan Campbell <jcampbell@mdjk.com>
 *
 * Decompiler and executioneer for XOR/OR/TEST instruction.
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

#include <softx86.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include "optable.h"
#include "binops.h"

/**************************************************
 ************** SAR 8/16/32 emulation *************
 **************************************************/
sx86_ubyte op_sar8(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val)
{
	sx86_ubyte ret,osrc,sa;
	int shic,oshic;

	if (ctx->bugs->mask_5bit_shiftcount)
		oshic = val&0x1F;
	else
		oshic = val;

/* Intel explicity documents that CF carries the bit that was
   shifted out, and that all flags are untouched if CF=0. */
	if (!oshic)	/* works for me... */
		return src;

/* peform the operation. */
	shic = oshic;
	osrc = src;
	ret  = src;
	sa   = (src&0x80)?(0xFF<<(8-shic)):0;

/* WARNING: This code assumes shic != 0 */
	if (shic > 1)
		ret >>= (shic-1);

/* "The CF flag contains the value of the last bit shifted out..." */
	if (ret & 1)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;
	ret >>= 1;
	ret  |= sa;

/* the OF flag follows some sort of bizarre logic */
	if (oshic == 1) {
/* "OF <- MSB(DEST)" */
		if (osrc>>7)
			ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
		else
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}
	else {
/* apparently undefined */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}

/* if result treated as signed value is negative */
	if (ret & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity8(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_ubyte op_sar1_8(softx86_ctx* ctx,sx86_ubyte src)
{
	return op_sar8(ctx,src,1);
}

sx86_ubyte op_sar_cl_8(softx86_ctx* ctx,sx86_ubyte src)
{
	return op_sar8(ctx,src,ctx->state->general_reg[SX86_REG_CX].b.lo);
}

sx86_uword op_sar16(softx86_ctx* ctx,sx86_uword src,sx86_uword val)
{
	sx86_uword ret,osrc;
	int shic,oshic;

	if (ctx->bugs->mask_5bit_shiftcount)
		oshic = val&0x1F;
	else
		oshic = val;

/* Intel explicity documents that CF carries the bit that was
   shifted out, and that all flags are untouched if CF=0. */
	if (!oshic)	/* works for me... */
		return src;

/* peform the operation. */
	shic = oshic;
	osrc = src;
	ret  = src;

/* WARNING: This code assumes shic != 0 */
	if (shic > 1)
		ret >>= (shic-1);

/* "The CF flag contains the value of the last bit shifted out..." */
	if (ret & 1)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;
	ret >>= 1;

/* the OF flag follows some sort of bizarre logic */
	if (oshic == 1) {
/* "OF <- MSB(DEST)" */
		if (osrc>>15)
			ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
		else
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}
	else {
/* apparently undefined */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}

/* if result treated as signed value is negative */
	if (ret & 0x8000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity16(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_uword op_sar1_16(softx86_ctx* ctx,sx86_uword src)
{
	return op_sar16(ctx,src,1);
}

sx86_uword op_sar_cl_16(softx86_ctx* ctx,sx86_uword src)
{
	return op_sar16(ctx,src,ctx->state->general_reg[SX86_REG_CX].b.lo);
}

sx86_udword op_sar32(softx86_ctx* ctx,sx86_udword src,sx86_udword val)
{
	sx86_udword ret,osrc;
	int shic,oshic;

	if (ctx->bugs->mask_5bit_shiftcount)
		oshic = val&0x1F;
	else
		oshic = val;

/* Intel explicity documents that CF carries the bit that was
   shifted out, and that all flags are untouched if CF=0. */
	if (!oshic)	/* works for me... */
		return src;

/* peform the operation. */
	shic = oshic;
	osrc = src;
	ret  = src;

/* WARNING: This code assumes shic != 0 */
	if (shic > 1)
		ret >>= (shic-1);

/* "The CF flag contains the value of the last bit shifted out..." */
	if (ret & 1)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;
	ret >>= 1;

/* the OF flag follows some sort of bizarre logic */
	if (oshic == 1) {
/* "OF <- MSB(DEST)" */
		if (osrc>>31)
			ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
		else
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}
	else {
/* apparently undefined */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}

/* if result treated as signed value is negative */
	if (ret & 0x80000000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity32(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_udword op_sar1_32(softx86_ctx* ctx,sx86_udword src)
{
	return op_sar32(ctx,src,1);
}

sx86_udword op_sar_cl_32(softx86_ctx* ctx,sx86_udword src)
{
	return op_sar32(ctx,src,ctx->state->general_reg[SX86_REG_CX].b.lo);
}

/**************************************************
 ************** SHR 8/16/32 emulation *************
 **************************************************/
sx86_ubyte op_shr8(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val)
{
	sx86_ubyte ret,osrc;
	int shic,oshic;

	if (ctx->bugs->mask_5bit_shiftcount)
		oshic = val&0x1F;
	else
		oshic = val;

/* Intel explicity documents that CF carries the bit that was
   shifted out, and that all flags are untouched if CF=0. */
	if (!oshic)	/* works for me... */
		return src;

/* peform the operation. */
	shic = oshic;
	osrc = src;
	ret  = src;

/* WARNING: This code assumes shic != 0 */
	if (shic > 1)
		ret >>= (shic-1);

/* "The CF flag contains the value of the last bit shifted out..." */
	if (ret & 1)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;
	ret >>= 1;

/* the OF flag follows some sort of bizarre logic */
	if (oshic == 1) {
/* "OF <- MSB(DEST)" */
		if (osrc>>7)
			ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
		else
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}
	else {
/* apparently undefined */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}

/* if result treated as signed value is negative */
	if (ret & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity8(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_ubyte op_shr1_8(softx86_ctx* ctx,sx86_ubyte src)
{
	return op_shr8(ctx,src,1);
}

sx86_ubyte op_shr_cl_8(softx86_ctx* ctx,sx86_ubyte src)
{
	return op_shr8(ctx,src,ctx->state->general_reg[SX86_REG_CX].b.lo);
}

sx86_uword op_shr16(softx86_ctx* ctx,sx86_uword src,sx86_uword val)
{
	sx86_uword ret,osrc;
	int shic,oshic;

	if (ctx->bugs->mask_5bit_shiftcount)
		oshic = val&0x1F;
	else
		oshic = val;

/* Intel explicity documents that CF carries the bit that was
   shifted out, and that all flags are untouched if CF=0. */
	if (!oshic)	/* works for me... */
		return src;

/* peform the operation. */
	shic = oshic;
	osrc = src;
	ret  = src;

/* WARNING: This code assumes shic != 0 */
	if (shic > 1)
		ret >>= (shic-1);

/* "The CF flag contains the value of the last bit shifted out..." */
	if (ret & 1)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;
	ret >>= 1;

/* the OF flag follows some sort of bizarre logic */
	if (oshic == 1) {
/* "OF <- MSB(DEST)" */
		if (osrc>>15)
			ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
		else
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}
	else {
/* apparently undefined */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}

/* if result treated as signed value is negative */
	if (ret & 0x8000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity16(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_uword op_shr1_16(softx86_ctx* ctx,sx86_uword src)
{
	return op_shr16(ctx,src,1);
}

sx86_uword op_shr_cl_16(softx86_ctx* ctx,sx86_uword src)
{
	return op_shr16(ctx,src,ctx->state->general_reg[SX86_REG_CX].b.lo);
}

sx86_udword op_shr32(softx86_ctx* ctx,sx86_udword src,sx86_udword val)
{
	sx86_udword ret,osrc;
	int shic,oshic;

	if (ctx->bugs->mask_5bit_shiftcount)
		oshic = val&0x1F;
	else
		oshic = val;

/* Intel explicity documents that CF carries the bit that was
   shifted out, and that all flags are untouched if CF=0. */
	if (!oshic)	/* works for me... */
		return src;

/* peform the operation. */
	shic = oshic;
	osrc = src;
	ret  = src;

/* WARNING: This code assumes shic != 0 */
	if (shic > 1)
		ret >>= (shic-1);

/* "The CF flag contains the value of the last bit shifted out..." */
	if (ret & 1)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;
	ret >>= 1;

/* the OF flag follows some sort of bizarre logic */
	if (oshic == 1) {
/* "OF <- MSB(DEST)" */
		if (osrc>>31)
			ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
		else
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}
	else {
/* apparently undefined */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}

/* if result treated as signed value is negative */
	if (ret & 0x80000000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity32(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_udword op_shr1_32(softx86_ctx* ctx,sx86_udword src)
{
	return op_shr32(ctx,src,1);
}

sx86_udword op_shr_cl_32(softx86_ctx* ctx,sx86_udword src)
{
	return op_shr32(ctx,src,ctx->state->general_reg[SX86_REG_CX].b.lo);
}

/**************************************************
 ************** SHL 8/16/32 emulation *************
 **************************************************/
sx86_ubyte op_shl8(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val)
{
	sx86_ubyte ret;
	int shic,oshic;

	if (ctx->bugs->mask_5bit_shiftcount)
		oshic = val&0x1F;
	else
		oshic = val;

/* Intel explicity documents that CF carries the bit that was
   shifted out, and that all flags are untouched if CF=0. */
	if (!oshic)	/* works for me... */
		return src;

/* peform the operation. */
	shic = oshic;
	ret = src;

/* WARNING: This code assumes shic != 0 */
	if (shic > 1)
		ret <<= (shic-1);

/* "The CF flag contains the value of the last bit shifted out..." */
	if (ret & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;
	ret <<= 1;

/* the OF flag follows some sort of bizarre logic */
	if (oshic == 1) {
/* "OF <- MSB(DEST) XOR CF" */
		if ((ret>>7)^((ctx->state->reg_flags.val>>SX86_CPUFLAGBO_CARRY)&1))
			ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
		else
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}
	else {
/* apparently undefined */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}

/* if result treated as signed value is negative */
	if (ret & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity8(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_ubyte op_shl1_8(softx86_ctx* ctx,sx86_ubyte src)
{
	return op_shl8(ctx,src,1);
}

sx86_ubyte op_shl_cl_8(softx86_ctx* ctx,sx86_ubyte src)
{
	return op_shl8(ctx,src,ctx->state->general_reg[SX86_REG_CX].b.lo);
}

sx86_uword op_shl16(softx86_ctx* ctx,sx86_uword src,sx86_uword val)
{
	sx86_uword ret;
	int shic,oshic;

	if (ctx->bugs->mask_5bit_shiftcount)
		oshic = val&0x1F;
	else
		oshic = val;

/* Intel explicity documents that CF carries the bit that was
   shifted out, and that all flags are untouched if CF=0. */
	if (!oshic)	/* works for me... */
		return src;

/* peform the operation. */
	shic = oshic;
	ret = src;

/* WARNING: This code assumes shic != 0 */
	if (shic > 1)
		ret <<= (shic-1);

/* "The CF flag contains the value of the last bit shifted out..." */
	if (ret & 0x8000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;
	ret <<= 1;

/* the OF flag follows some sort of bizarre logic */
	if (oshic == 1) {
/* "OF <- MSB(DEST) XOR CF" */
		if ((ret>>15)^((ctx->state->reg_flags.val>>SX86_CPUFLAGBO_CARRY)&1))
			ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
		else
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}
	else {
/* apparently undefined */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}

/* if result treated as signed value is negative */
	if (ret & 0x8000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity16(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_uword op_shl1_16(softx86_ctx* ctx,sx86_uword src)
{
	return op_shl16(ctx,src,1);
}

sx86_uword op_shl_cl_16(softx86_ctx* ctx,sx86_uword src)
{
	return op_shl16(ctx,src,ctx->state->general_reg[SX86_REG_CX].b.lo);
}

sx86_udword op_shl32(softx86_ctx* ctx,sx86_udword src,sx86_udword val)
{
	sx86_udword ret;
	int shic,oshic;

	if (ctx->bugs->mask_5bit_shiftcount)
		oshic = val&0x1F;
	else
		oshic = val;

/* Intel explicity documents that CF carries the bit that was
   shifted out, and that all flags are untouched if CF=0. */
	if (!oshic)	/* works for me... */
		return src;

/* peform the operation. */
	shic = oshic;
	ret = src;

/* WARNING: This code assumes shic != 0 */
	if (shic > 1)
		ret <<= (shic-1);

/* "The CF flag contains the value of the last bit shifted out..." */
	if (ret & 0x80000000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;
	ret <<= 1;

/* the OF flag follows some sort of bizarre logic */
	if (oshic == 1) {
/* "OF <- MSB(DEST) XOR CF" */
		if ((ret>>31)^((ctx->state->reg_flags.val>>SX86_CPUFLAGBO_CARRY)&1))
			ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
		else
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}
	else {
/* apparently undefined */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}

/* if result treated as signed value is negative */
	if (ret & 0x80000000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity32(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_udword op_shl1_32(softx86_ctx* ctx,sx86_udword src)
{
	return op_shl32(ctx,src,1);
}

sx86_udword op_shl_cl_32(softx86_ctx* ctx,sx86_udword src)
{
	return op_shl32(ctx,src,ctx->state->general_reg[SX86_REG_CX].b.lo);
}

/**************************************************
 ************** ROR 8/16/32 emulation *************
 **************************************************/
sx86_ubyte op_ror8(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val)
{
	sx86_ubyte ret;
	int shic,oshic;

	oshic = val&7;
	if (!oshic)	/* don't bother */
		return src;

/* peform the operation. */
	shic = oshic;
	ret  = (src>>shic)|(src<<(8-shic));

/* "CF <- MSB(DEST)" */
	if (ret & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;

/* the OF flag follows some sort of bizarre logic */
	if (oshic == 1) {
/* "OF <- MSB(DEST) XOR MSB - 1(DEST)" */
		if ((ret>>7)^((ret>>6)&1))
			ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
		else
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}
	else {
/* apparently undefined */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}

/* if result treated as signed value is negative */
	if (ret & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity8(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_ubyte op_ror1_8(softx86_ctx* ctx,sx86_ubyte src)
{
	return op_ror8(ctx,src,1);
}

sx86_ubyte op_ror_cl_8(softx86_ctx* ctx,sx86_ubyte src)
{
	return op_ror8(ctx,src,ctx->state->general_reg[SX86_REG_CX].b.lo);
}

sx86_uword op_ror16(softx86_ctx* ctx,sx86_uword src,sx86_uword val)
{
	sx86_uword ret;
	int shic,oshic;

	oshic = val&15;
	if (!oshic)	/* don't bother */
		return src;

/* peform the operation. */
	shic = oshic;
	ret  = (src>>shic)|(src<<(16-shic));

/* "CF <- MSB(DEST)" */
	if (ret & 0x8000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;

/* the OF flag follows some sort of bizarre logic */
	if (oshic == 1) {
/* "OF <- MSB(DEST) XOR MSB - 1(DEST)" */
		if ((ret>>15)^((ret>>14)&1))
			ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
		else
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}
	else {
/* apparently undefined */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}

/* if result treated as signed value is negative */
	if (ret & 0x8000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity16(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_uword op_ror1_16(softx86_ctx* ctx,sx86_uword src)
{
	return op_ror16(ctx,src,1);
}

sx86_uword op_ror_cl_16(softx86_ctx* ctx,sx86_uword src)
{
	return op_ror16(ctx,src,ctx->state->general_reg[SX86_REG_CX].b.lo);
}

sx86_udword op_ror32(softx86_ctx* ctx,sx86_udword src,sx86_udword val)
{
	sx86_udword ret;
	int shic,oshic;

	oshic = val&31;
	if (!oshic)	/* don't bother */
		return src;

/* peform the operation. */
	shic = oshic;
	ret  = (src>>shic)|(src<<(32-shic));

/* "CF <- MSB(DEST)" */
	if (ret & 0x80000000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;

/* the OF flag follows some sort of bizarre logic */
	if (oshic == 1) {
/* "OF <- MSB(DEST) XOR MSB - 1(DEST)" */
		if ((ret>>31)^((ret>>30)&1))
			ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
		else
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}
	else {
/* apparently undefined */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}

/* if result treated as signed value is negative */
	if (ret & 0x80000000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity32(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_udword op_ror1_32(softx86_ctx* ctx,sx86_udword src)
{
	return op_ror32(ctx,src,1);
}

sx86_udword op_ror_cl_32(softx86_ctx* ctx,sx86_udword src)
{
	return op_ror32(ctx,src,ctx->state->general_reg[SX86_REG_CX].b.lo);
}

/**************************************************
 ************** ROL 8/16/32 emulation *************
 **************************************************/
sx86_ubyte op_rol8(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val)
{
	sx86_ubyte ret;
	int shic,oshic;

	oshic = val&7;
	if (!oshic)	/* don't bother */
		return src;

/* peform the operation. */
	shic = oshic;
	ret  = (src<<shic)|(src>>(8-shic));

/* "CF <- LSB(DEST)" */
	if (ret & 1)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;

/* the OF flag follows some sort of bizarre logic */
	if (oshic == 1) {
/* "OF <- MSB(DEST) XOR CF" */
		if ((ret>>7)^((ctx->state->reg_flags.val>>SX86_CPUFLAGBO_CARRY)&1))
			ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
		else
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}
	else {
/* apparently undefined */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}

/* if result treated as signed value is negative */
	if (ret & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity8(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_ubyte op_rol1_8(softx86_ctx* ctx,sx86_ubyte src)
{
	return op_rol8(ctx,src,1);
}

sx86_ubyte op_rol_cl_8(softx86_ctx* ctx,sx86_ubyte src)
{
	return op_rol8(ctx,src,ctx->state->general_reg[SX86_REG_CX].b.lo);
}

sx86_uword op_rol16(softx86_ctx* ctx,sx86_uword src,sx86_uword val)
{
	sx86_uword ret;
	int shic,oshic;

	oshic = val&15;
	if (!oshic)	/* don't bother */
		return src;

/* peform the operation. */
	shic = oshic;
	ret  = (src<<shic)|(src>>(16-shic));

/* "CF <- LSB(DEST)" */
	if (ret & 1)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;

/* the OF flag follows some sort of bizarre logic */
	if (oshic == 1) {
/* "OF <- MSB(DEST) XOR CF" */
		if ((ret>>15)^((ctx->state->reg_flags.val>>SX86_CPUFLAGBO_CARRY)&1))
			ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
		else
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}
	else {
/* apparently undefined */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}

/* if result treated as signed value is negative */
	if (ret & 0x8000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity16(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_uword op_rol1_16(softx86_ctx* ctx,sx86_uword src)
{
	return op_rol16(ctx,src,1);
}

sx86_uword op_rol_cl_16(softx86_ctx* ctx,sx86_uword src)
{
	return op_rol16(ctx,src,ctx->state->general_reg[SX86_REG_CX].b.lo);
}

sx86_udword op_rol32(softx86_ctx* ctx,sx86_udword src,sx86_udword val)
{
	sx86_udword ret;
	int shic,oshic;

	oshic = val&31;
	if (!oshic)	/* don't bother */
		return src;

/* peform the operation. */
	shic = oshic;
	ret  = (src<<shic)|(src>>(32-shic));

/* "CF <- LSB(DEST)" */
	if (ret & 1)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;

/* the OF flag follows some sort of bizarre logic */
	if (oshic == 1) {
/* "OF <- MSB(DEST) XOR CF" */
		if ((ret>>31)^((ctx->state->reg_flags.val>>SX86_CPUFLAGBO_CARRY)&1))
			ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
		else
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}
	else {
/* apparently undefined */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}

/* if result treated as signed value is negative */
	if (ret & 0x80000000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity16(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_udword op_rol1_32(softx86_ctx* ctx,sx86_udword src)
{
	return op_rol32(ctx,src,1);
}

sx86_udword op_rol_cl_32(softx86_ctx* ctx,sx86_udword src)
{
	return op_rol32(ctx,src,ctx->state->general_reg[SX86_REG_CX].b.lo);
}

/**************************************************
 ************** RCL 8/16/32 emulation *************
 **************************************************/
sx86_ubyte op_rcl8(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val)
{
	sx86_ubyte ret,cf;
	int shic,oshic;

	oshic = val%9;	/* 8 bits + CF */
	if (!oshic)	/* don't bother */
		return src;

/* peform the operation. */
	ret  = src;
	shic = oshic;
	while (shic-- > 0) {
		cf  = ret>>7;
		ret = (ret<<1)|((ctx->state->reg_flags.val>>SX86_CPUFLAGBO_CARRY)&1);
		if (cf)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
		else	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;
	}

/* the OF flag follows some sort of bizarre logic */
	if (oshic == 1) {
/* "OF <- MSB(DEST) XOR CF" */
		if ((ret>>7)^((ctx->state->reg_flags.val>>SX86_CPUFLAGBO_CARRY)&1))
			ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
		else
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}
	else {
/* apparently undefined */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}

/* if result treated as signed value is negative */
	if (ret & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity8(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_ubyte op_rcl1_8(softx86_ctx* ctx,sx86_ubyte src)
{
	return op_rcl8(ctx,src,1);
}

sx86_ubyte op_rcl_cl_8(softx86_ctx* ctx,sx86_ubyte src)
{
	return op_rcl8(ctx,src,ctx->state->general_reg[SX86_REG_CX].b.lo);
}

sx86_uword op_rcl16(softx86_ctx* ctx,sx86_uword src,sx86_uword val)
{
	sx86_uword ret,cf;
	int shic,oshic;

	oshic = val%17;	/* 16 bits + CF */
	if (!oshic)	/* don't bother */
		return src;

/* peform the operation. */
	ret  = src;
	shic = oshic;
	while (shic-- > 0) {
		cf  = ret>>15;
		ret = (ret<<1)|((ctx->state->reg_flags.val>>SX86_CPUFLAGBO_CARRY)&1);
		if (cf)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
		else	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;
	}

/* the OF flag follows some sort of bizarre logic */
	if (oshic == 1) {
/* "OF <- MSB(DEST) XOR CF" */
		if ((ret>>15)^((ctx->state->reg_flags.val>>SX86_CPUFLAGBO_CARRY)&1))
			ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
		else
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}
	else {
/* apparently undefined */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}

/* if result treated as signed value is negative */
	if (ret & 0x8000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity16(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_uword op_rcl1_16(softx86_ctx* ctx,sx86_uword src)
{
	return op_rcl16(ctx,src,1);
}

sx86_uword op_rcl_cl_16(softx86_ctx* ctx,sx86_uword src)
{
	return op_rcl16(ctx,src,ctx->state->general_reg[SX86_REG_CX].b.lo);
}

sx86_udword op_rcl32(softx86_ctx* ctx,sx86_udword src,sx86_udword val)
{
	sx86_udword ret,cf;
	int shic,oshic;

	oshic = val%33;	/* 32 bits + CF */
	if (!oshic)	/* don't bother */
		return src;

/* peform the operation. */
	ret  = src;
	shic = oshic;
	while (shic-- > 0) {
		cf  = ret>>31;
		ret = (ret<<1)|((ctx->state->reg_flags.val>>SX86_CPUFLAGBO_CARRY)&1);
		if (cf)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
		else	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;
	}

/* the OF flag follows some sort of bizarre logic */
	if (oshic == 1) {
/* "OF <- MSB(DEST) XOR CF" */
		if ((ret>>31)^((ctx->state->reg_flags.val>>SX86_CPUFLAGBO_CARRY)&1))
			ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
		else
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}
	else {
/* apparently undefined */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}

/* if result treated as signed value is negative */
	if (ret & 0x80000000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity32(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_udword op_rcl1_32(softx86_ctx* ctx,sx86_udword src)
{
	return op_rcl32(ctx,src,1);
}

sx86_udword op_rcl_cl_32(softx86_ctx* ctx,sx86_udword src)
{
	return op_rcl32(ctx,src,ctx->state->general_reg[SX86_REG_CX].b.lo);
}

/**************************************************
 ************** RCR 8/16/32 emulation *************
 **************************************************/
sx86_ubyte op_rcr8(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val)
{
	sx86_ubyte ret,cf;
	int shic,oshic;

	oshic = val%9;	/* 8 bits + CF */
	if (!oshic)	/* don't bother */
		return src;

/* the OF flag follows some sort of bizarre logic... and...
   why is this done BEFORE the operation (according to
   the pseudocode)? */
	ret = src;
	if (oshic == 1) {
/* "OF <- MSB(DEST) XOR CF" */
		if ((ret>>7)^((ctx->state->reg_flags.val>>SX86_CPUFLAGBO_CARRY)&1))
			ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
		else
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}
	else {
/* apparently undefined */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}

/* peform the operation. */
	shic = oshic;
	while (shic-- > 0) {
		cf  = ret&1;
		ret = (ret>>1)|(((ctx->state->reg_flags.val>>SX86_CPUFLAGBO_CARRY)&1)<<7);
		if (cf)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
		else	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;
	}

/* if result treated as signed value is negative */
	if (ret & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity8(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_ubyte op_rcr1_8(softx86_ctx* ctx,sx86_ubyte src)
{
	return op_rcr8(ctx,src,1);
}

sx86_ubyte op_rcr_cl_8(softx86_ctx* ctx,sx86_ubyte src)
{
	return op_rcr8(ctx,src,ctx->state->general_reg[SX86_REG_CX].b.lo);
}

sx86_uword op_rcr16(softx86_ctx* ctx,sx86_uword src,sx86_uword val)
{
	sx86_uword ret,cf;
	int shic,oshic;

	oshic = val%17;	/* 16 bits + CF */
	if (!oshic)	/* don't bother */
		return src;

/* the OF flag follows some sort of bizarre logic... and...
   why is this done BEFORE the operation (according to
   the pseudocode)? */
	ret = src;
	if (oshic == 1) {
/* "OF <- MSB(DEST) XOR CF" */
		if ((ret>>15)^((ctx->state->reg_flags.val>>SX86_CPUFLAGBO_CARRY)&1))
			ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
		else
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}
	else {
/* apparently undefined */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}

/* peform the operation. */
	shic = oshic;
	while (shic-- > 0) {
		cf  = ret&1;
		ret = (ret>>1)|(((ctx->state->reg_flags.val>>SX86_CPUFLAGBO_CARRY)&1)<<15);
		if (cf)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
		else	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;
	}

/* if result treated as signed value is negative */
	if (ret & 0x8000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity16(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_uword op_rcr1_16(softx86_ctx* ctx,sx86_uword src)
{
	return op_rcr16(ctx,src,1);
}

sx86_uword op_rcr_cl_16(softx86_ctx* ctx,sx86_uword src)
{
	return op_rcr16(ctx,src,ctx->state->general_reg[SX86_REG_CX].b.lo);
}

sx86_udword op_rcr32(softx86_ctx* ctx,sx86_udword src,sx86_udword val)
{
	sx86_udword ret,cf;
	int shic,oshic;

	oshic = val%33;	/* 32 bits + CF */
	if (!oshic)	/* don't bother */
		return src;

/* the OF flag follows some sort of bizarre logic... and...
   why is this done BEFORE the operation (according to
   the pseudocode)? */
	ret = src;
	if (oshic == 1) {
/* "OF <- MSB(DEST) XOR CF" */
		if ((ret>>31)^((ctx->state->reg_flags.val>>SX86_CPUFLAGBO_CARRY)&1))
			ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
		else
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}
	else {
/* apparently undefined */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
	}

/* peform the operation. */
	shic = oshic;
	while (shic-- > 0) {
		cf  = ret&1;
		ret = (ret>>1)|(((ctx->state->reg_flags.val>>SX86_CPUFLAGBO_CARRY)&1)<<31);
		if (cf)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
		else	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;
	}

/* if result treated as signed value is negative */
	if (ret & 0x80000000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity32(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_udword op_rcr1_32(softx86_ctx* ctx,sx86_udword src)
{
	return op_rcr32(ctx,src,1);
}

sx86_udword op_rcr_cl_32(softx86_ctx* ctx,sx86_udword src)
{
	return op_rcr32(ctx,src,ctx->state->general_reg[SX86_REG_CX].b.lo);
}

/**************************************************
 ************** XOR 8/16/32 emulation *************
 **************************************************/
sx86_ubyte op_xor8(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val)
{
	sx86_ubyte ret;

/* peform the operation */
	ret = src ^ val;

/* carry/overflow always cleared */
	ctx->state->reg_flags.val &=
		~(SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);

/* if result treated as signed value is negative */
	if (ret & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity8(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_uword op_xor16(softx86_ctx* ctx,sx86_uword src,sx86_uword val)
{
	sx86_uword ret;

/* peform the operation */
	ret = src ^ val;

/* carry/overflow always cleared */
	ctx->state->reg_flags.val &=
		~(SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);

/* if result treated as signed value is negative */
	if (ret & 0x8000)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity16(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_udword op_xor32(softx86_ctx* ctx,sx86_udword src,sx86_udword val)
{
	sx86_udword ret;

/* peform the operation */
	ret = src ^ val;

/* carry/overflow always cleared */
	ctx->state->reg_flags.val &=
		~(SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);

/* if result treated as signed value is negative */
	if (ret & 0x80000000)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity32(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

/**************************************************
 ************** OR 8/16/32 emulation **************
 **************************************************/
sx86_ubyte op_or8(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val)
{
	sx86_ubyte ret;

/* peform the operation */
	ret = src | val;

/* carry/overflow always cleared */
	ctx->state->reg_flags.val &=
		~(SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);

/* if result treated as signed value is negative */
	if (ret & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity8(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_uword op_or16(softx86_ctx* ctx,sx86_uword src,sx86_uword val)
{
	sx86_uword ret;

/* peform the operation */
	ret = src | val;

/* carry/overflow always cleared */
	ctx->state->reg_flags.val &=
		~(SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);

/* if result treated as signed value is negative */
	if (ret & 0x8000)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity16(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_udword op_or32(softx86_ctx* ctx,sx86_udword src,sx86_udword val)
{
	sx86_udword ret;

/* peform the operation */
	ret = src | val;

/* carry/overflow always cleared */
	ctx->state->reg_flags.val &=
		~(SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);

/* if result treated as signed value is negative */
	if (ret & 0x80000000)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity32(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

/**************************************************
 ************** AND 8/16/32 emulation *************
 **************************************************/
sx86_ubyte op_and8(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val)
{
	sx86_ubyte ret;

/* peform the operation */
	ret = src & val;

/* carry/overflow always cleared */
	ctx->state->reg_flags.val &=
		~(SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);

/* if result treated as signed value is negative */
	if (ret & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity8(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_uword op_and16(softx86_ctx* ctx,sx86_uword src,sx86_uword val)
{
	sx86_uword ret;

/* peform the operation */
	ret = src & val;

/* carry/overflow always cleared */
	ctx->state->reg_flags.val &=
		~(SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);

/* if result treated as signed value is negative */
	if (ret & 0x8000)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity16(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_udword op_and32(softx86_ctx* ctx,sx86_udword src,sx86_udword val)
{
	sx86_udword ret;

/* peform the operation */
	ret = src & val;

/* carry/overflow always cleared */
	ctx->state->reg_flags.val &=
		~(SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);

/* if result treated as signed value is negative */
	if (ret & 0x80000000)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity32(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

/**************************************************
 ************* TEST 8/16/32 emulation *************
 **************************************************/
/* apparently TEST is a lot like AND except for subtle differences... */
sx86_ubyte op_test8(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val)
{
	sx86_ubyte ret;

/* peform the operation */
	ret = src & val;

/* carry/overflow always cleared */
	ctx->state->reg_flags.val &=
		~(SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);

/* if result treated as signed value is negative */
	if (ret & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit.
   NOTE: Intel's documentation states that only the lower 8 bits
         are used in setting PF. Odd... */
	if (softx86_parity8(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_uword op_test16(softx86_ctx* ctx,sx86_uword src,sx86_uword val)
{
	sx86_uword ret;

/* peform the operation */
	ret = src & val;

/* carry/overflow always cleared */
	ctx->state->reg_flags.val &=
		~(SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);

/* if result treated as signed value is negative */
	if (ret & 0x8000)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit.
   NOTE: Intel's documentation states that only the lower 8 bits
         are used in setting PF. Odd... */
	if (softx86_parity8((sx86_ubyte)ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_udword op_test32(softx86_ctx* ctx,sx86_udword src,sx86_udword val)
{
	sx86_udword ret;

/* peform the operation */
	ret = src & val;

/* carry/overflow always cleared */
	ctx->state->reg_flags.val &=
		~(SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);

/* if result treated as signed value is negative */
	if (ret & 0x80000000)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* quoting Intel: "The state of the AF flag is undefined"... */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit.
   NOTE: Intel's documentation states that only the lower 8 bits
         are used in setting PF. Odd... */
	if (softx86_parity8(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

/**************************************************
 ************* NOT 8/16/32 emulation *************
 **************************************************/
sx86_ubyte op_not8(softx86_ctx* ctx,sx86_ubyte src)
{
/* peform the operation, no flags are affected */
	return (~src);
}

sx86_uword op_not16(softx86_ctx* ctx,sx86_uword src)
{
/* peform the operation, no flags are affected */
	return (~src);
}

sx86_udword op_not32(softx86_ctx* ctx,sx86_udword src)
{
/* peform the operation, no flags are affected */
	return (~src);
}

/*************************************************
 ************* NEG 8/16/32 emulation *************
 *************************************************/
sx86_ubyte op_neg8(softx86_ctx* ctx,sx86_ubyte src)
{
	sx86_ubyte ret;

/* peform the operation */
	ret = (~src)+1;	/* binary equivalent of x = 0 - x */

/* carry set if operand was and still is 0 (since NEG 0 = 0) */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;

/* Intel isn't clear how the OF flag is set "according to the result" */
/* perhaps this is a minor detail that most programmers and clone
   chip makers overlook anyway... but here is my interpretation of it.
   based on the assumption that the chip just inverts the bits then adds
   +1. ((NOT 0) + 1) = 0x100, and 0x100 squeezed into 8 bits is 0x00
   with one bit that "overflowed". */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;

/* if result treated as signed value is negative */
	if (ret & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* Intel isn't clear on how this flag is set either. */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit. */
	if (softx86_parity8(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_uword op_neg16(softx86_ctx* ctx,sx86_uword src)
{
	sx86_uword ret;

/* peform the operation */
	ret = (~src)+1;	/* binary equivalent of x = 0 - x */

/* carry set if operand was and still is 0 (since NEG 0 = 0) */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;

/* Intel isn't clear how the OF flag is set "according to the result" */
/* perhaps this is a minor detail that most programmers and clone
   chip makers overlook anyway... but here is my interpretation of it.
   based on the assumption that the chip just inverts the bits then adds
   +1. ((NOT 0) + 1) = 0x10000, and 0x10000 squeezed into 16 bits is 0x00
   with one bit that "overflowed". */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;

/* if result treated as signed value is negative */
	if (ret & 0x8000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* Intel isn't clear on how this flag is set either. */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit. */
	if (softx86_parity16(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_udword op_neg32(softx86_ctx* ctx,sx86_udword src)
{
	sx86_udword ret;

/* peform the operation */
	ret = (~src)+1;	/* binary equivalent of x = 0 - x */

/* carry set if operand was and still is 0 (since NEG 0 = 0) */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;

/* Intel isn't clear how the OF flag is set "according to the result" */
/* perhaps this is a minor detail that most programmers and clone
   chip makers overlook anyway... but here is my interpretation of it.
   based on the assumption that the chip just inverts the bits then adds
   +1. ((NOT 0) + 1) = 0x100000000, and 0x100000000 squeezed into 32 bits
   is 0x00 with one bit that "overflowed". */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_OVERFLOW;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;

/* if result treated as signed value is negative */
	if (ret & 0x80000000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* Intel isn't clear on how this flag is set either. */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit. */
	if (softx86_parity32(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

/*************************************************
 ************* MUL 8/16/32 emulation *************
 *************************************************/
sx86_ubyte op_mul8(softx86_ctx* ctx,sx86_ubyte src)
{
	sx86_uword result;

/* peform the operation AX = AL * r/m8 */
	result  = ((sx86_uword)ctx->state->general_reg[SX86_REG_AX].b.lo);
	result *= ((sx86_uword)src);

/* the result is stored into AX */
	ctx->state->general_reg[SX86_REG_AX].w.lo = result;

/* OF and CF are set if the upper half is nonzero */
	if (result>>8)	ctx->state->reg_flags.val |=   SX86_CPUFLAG_OVERFLOW | SX86_CPUFLAG_CARRY;
	else		ctx->state->reg_flags.val &= ~(SX86_CPUFLAG_OVERFLOW | SX86_CPUFLAG_CARRY);

/* Intel documents SF ZF AF and PF as "undefined" after this instruction. */
/* We'll just set them anyway by the normal rules... */
/* if result treated as signed value is negative */
	if (result & 0x8000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!result)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* Whatever */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit. */
	if (softx86_parity16(result))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return src;	/* this is called by sx86_exec_full_modrmonly_ro() so it doesn't matter */
}

sx86_uword op_mul16(softx86_ctx* ctx,sx86_uword src)
{
	sx86_udword result;

/* peform the operation DX:AX = AX * r/m16 */
	result  = ((sx86_udword)ctx->state->general_reg[SX86_REG_AX].w.lo);
	result *= ((sx86_udword)src);

/* the result is stored into DX:AX */
	ctx->state->general_reg[SX86_REG_AX].w.lo = (sx86_uword)(result&0xFFFF);
	ctx->state->general_reg[SX86_REG_DX].w.lo = (sx86_uword)(result>>16);

/* OF and CF are set if the upper half is nonzero */
	if (result>>16)	ctx->state->reg_flags.val |=   SX86_CPUFLAG_OVERFLOW | SX86_CPUFLAG_CARRY;
	else		ctx->state->reg_flags.val &= ~(SX86_CPUFLAG_OVERFLOW | SX86_CPUFLAG_CARRY);

/* Intel documents SF ZF AF and PF as "undefined" after this instruction. */
/* We'll just set them anyway by the normal rules... */
/* if result treated as signed value is negative */
	if (result & 0x80000000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else				ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!result)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* Whatever */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit. */
	if (softx86_parity32(result))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return src;	/* this is called by sx86_exec_full_modrmonly_ro() so it doesn't matter */
}

sx86_udword op_mul32(softx86_ctx* ctx,sx86_udword src)
{
	sx86_uldword result;

/* peform the operation EDX:EAX = EAX * r/m32 */
	result  = ((sx86_uldword)ctx->state->general_reg[SX86_REG_AX].val);
	result *= ((sx86_uldword)src);

/* the result is stored into EDX:EAX */
	ctx->state->general_reg[SX86_REG_AX].val = (sx86_udword)(result&0xFFFFFFFF);
	ctx->state->general_reg[SX86_REG_DX].val = (sx86_udword)(result>>32);

/* OF and CF are set if the upper half is nonzero */
	if (result>>32)	ctx->state->reg_flags.val |=   SX86_CPUFLAG_OVERFLOW | SX86_CPUFLAG_CARRY;
	else		ctx->state->reg_flags.val &= ~(SX86_CPUFLAG_OVERFLOW | SX86_CPUFLAG_CARRY);

/* Intel documents SF ZF AF and PF as "undefined" after this instruction. */
/* We'll just set them anyway by the normal rules... */
/* if result treated as signed value is negative */
	if (result & 0x8000000000000000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else					ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!result)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* Whatever */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit. */
	if (softx86_parity64(result))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return src;	/* this is called by sx86_exec_full_modrmonly_ro() so it doesn't matter */
}

/**************************************************
 ************* IMUL 8/16/32 emulation *************
 **************************************************/
sx86_ubyte op_imul8(softx86_ctx* ctx,sx86_ubyte src)
{
	sx86_sword result;

/* peform the operation AX = AL * r/m8 */
	result  = (sx86_sword)((sx86_sbyte)ctx->state->general_reg[SX86_REG_AX].b.lo);
	result *= (sx86_sword)((sx86_sbyte)src);

/* the result is stored into AX */
	ctx->state->general_reg[SX86_REG_AX].w.lo = result;

/* OF and CF are set if the upper half is nonzero */
	if ((result>>8) == 0 || ((result>>8)&0xFF) == 0xFF)
		ctx->state->reg_flags.val &= ~(SX86_CPUFLAG_OVERFLOW | SX86_CPUFLAG_CARRY);
	else
		ctx->state->reg_flags.val |=   SX86_CPUFLAG_OVERFLOW | SX86_CPUFLAG_CARRY;

/* Intel documents SF ZF AF and PF as "undefined" after this instruction. */
/* We'll just set them anyway by the normal rules... */
/* if result treated as signed value is negative */
	if (result & 0x8000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!result)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* Whatever */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit. */
	if (softx86_parity16(result))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return src;	/* this is called by sx86_exec_full_modrmonly_ro() so it doesn't matter */
}

sx86_uword op_imul16(softx86_ctx* ctx,sx86_uword src)
{
	sx86_sdword result;

/* peform the operation AX = AL * r/m8 */
	result  = (sx86_sdword)((sx86_sword)ctx->state->general_reg[SX86_REG_AX].w.lo);
	result *= (sx86_sdword)((sx86_sword)src);

/* the result is stored into AX */
	ctx->state->general_reg[SX86_REG_AX].w.lo = (sx86_uword)(result&0xFFFF);
	ctx->state->general_reg[SX86_REG_DX].w.lo = (sx86_uword)(result>>16);

/* OF and CF are set if the upper half is nonzero */
	if ((result>>16) == 0 || ((result>>16)&0xFFFF) == 0xFFFF)
		ctx->state->reg_flags.val &= ~(SX86_CPUFLAG_OVERFLOW | SX86_CPUFLAG_CARRY);
	else
		ctx->state->reg_flags.val |=   SX86_CPUFLAG_OVERFLOW | SX86_CPUFLAG_CARRY;

/* Intel documents SF ZF AF and PF as "undefined" after this instruction. */
/* We'll just set them anyway by the normal rules... */
/* if result treated as signed value is negative */
	if (result & 0x80000000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else				ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!result)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* Whatever */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit. */
	if (softx86_parity32(result))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return src;	/* this is called by sx86_exec_full_modrmonly_ro() so it doesn't matter */
}

sx86_udword op_imul32(softx86_ctx* ctx,sx86_udword src)
{
	sx86_sldword result;

/* peform the operation AX = AL * r/m8 */
	result  = (sx86_sldword)((sx86_sdword)ctx->state->general_reg[SX86_REG_AX].val);
	result *= (sx86_sldword)((sx86_sdword)src);

/* the result is stored into AX */
	ctx->state->general_reg[SX86_REG_AX].val = (sx86_uword)(result&0xFFFFFFFF);
	ctx->state->general_reg[SX86_REG_DX].val = (sx86_uword)(result>>32);

/* OF and CF are set if the upper half is nonzero */
	if ((result>>32) == 0 || ((result>>32)&0xFFFFFFFF) == 0xFFFFFFFF)
		ctx->state->reg_flags.val &= ~(SX86_CPUFLAG_OVERFLOW | SX86_CPUFLAG_CARRY);
	else
		ctx->state->reg_flags.val |=   SX86_CPUFLAG_OVERFLOW | SX86_CPUFLAG_CARRY;

/* Intel documents SF ZF AF and PF as "undefined" after this instruction. */
/* We'll just set them anyway by the normal rules... */
/* if result treated as signed value is negative */
	if (result & 0x8000000000000000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else					ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!result)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* Whatever */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit. */
	if (softx86_parity64(result))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return src;	/* this is called by sx86_exec_full_modrmonly_ro() so it doesn't matter */
}

/*************************************************
 ************* DIV 8/16/32 emulation *************
 *************************************************/
sx86_ubyte op_div8(softx86_ctx* ctx,sx86_ubyte src)
{
	sx86_uword result;

/* signal division overflow */
	if (src == 0) {
		softx86_int_sw_signal(ctx,0);
		return src;
	}

/* peform the operation AL = AX / r/m8 */
	result = ((sx86_uword)ctx->state->general_reg[SX86_REG_AX].w.lo) / ((sx86_uword)src);

/* signal division overflow */
	if (result > 0xFF) {
		softx86_int_sw_signal(ctx,0);
		return src;
	}

/* the result is stored into AL, remainder in AH */
	ctx->state->general_reg[SX86_REG_AX].b.hi =
		((sx86_uword)ctx->state->general_reg[SX86_REG_AX].w.lo) % ((sx86_uword)src);
	ctx->state->general_reg[SX86_REG_AX].b.lo = (sx86_ubyte)result;

/* Apparently all the flags are undefined after this instruction. */
	ctx->state->reg_flags.val &=
		~(SX86_CPUFLAG_SIGN | SX86_CPUFLAG_ZERO | SX86_CPUFLAG_AUX | SX86_CPUFLAG_PARITY);

	return src;	/* this is called by sx86_exec_full_modrmonly_ro() so it doesn't matter */
}

sx86_uword op_div16(softx86_ctx* ctx,sx86_uword src)
{
	sx86_udword result;

/* signal division overflow */
	if (src == 0) {
		softx86_int_sw_signal(ctx,0);
		return src;
	}

/* peform the operation AX = DX:AX / r/m16 */
	result  = ((sx86_udword)ctx->state->general_reg[SX86_REG_AX].w.lo);
	result |= ((sx86_udword)ctx->state->general_reg[SX86_REG_DX].w.lo)<<16;
	result /= ((sx86_udword)src);

/* signal division overflow */
	if (result > 0xFFFF) {
		softx86_int_sw_signal(ctx,0);
		return src;
	}

/* the result is stored into AX, remainder in DX */
	ctx->state->general_reg[SX86_REG_DX].w.lo =
		((sx86_uword)ctx->state->general_reg[SX86_REG_AX].w.lo) % ((sx86_uword)src);
	ctx->state->general_reg[SX86_REG_AX].w.lo = (sx86_uword)result;

/* Apparently all the flags are undefined after this instruction. */
	ctx->state->reg_flags.val &=
		~(SX86_CPUFLAG_SIGN | SX86_CPUFLAG_ZERO | SX86_CPUFLAG_AUX | SX86_CPUFLAG_PARITY);

	return src;	/* this is called by sx86_exec_full_modrmonly_ro() so it doesn't matter */
}

sx86_udword op_div32(softx86_ctx* ctx,sx86_udword src)
{
	sx86_uldword result;

/* signal division overflow */
	if (src == 0) {
		softx86_int_sw_signal(ctx,0);
		return src;
	}

/* peform the operation AX = EDX:EAX / r/m32 */
	result  = ((sx86_uldword)ctx->state->general_reg[SX86_REG_AX].val);
	result |= ((sx86_uldword)ctx->state->general_reg[SX86_REG_DX].val)<<32;
	result /= ((sx86_uldword)src);

/* signal division overflow */
	if (result > 0xFFFFFFFFL) {
		softx86_int_sw_signal(ctx,0);
		return src;
	}

/* the result is stored into EAX, remainder in EDX */
	ctx->state->general_reg[SX86_REG_DX].val =
		((sx86_udword)ctx->state->general_reg[SX86_REG_AX].val) % ((sx86_udword)src);
	ctx->state->general_reg[SX86_REG_AX].val = (sx86_udword)result;

/* Apparently all the flags are undefined after this instruction. */
	ctx->state->reg_flags.val &=
		~(SX86_CPUFLAG_SIGN | SX86_CPUFLAG_ZERO | SX86_CPUFLAG_AUX | SX86_CPUFLAG_PARITY);

	return src;	/* this is called by sx86_exec_full_modrmonly_ro() so it doesn't matter */
}

/**************************************************
 ************* IDIV 8/16/32 emulation *************
 **************************************************/
sx86_ubyte op_idiv8(softx86_ctx* ctx,sx86_ubyte src)
{
	sx86_sword result;

/* signal division overflow */
	if (src == 0) {
		softx86_int_sw_signal(ctx,0);
		return src;
	}

/* peform the operation AL = AX / r/m8 */
	result = ((sx86_sword)ctx->state->general_reg[SX86_REG_AX].w.lo) / ((sx86_sbyte)src);

/* signal division overflow */
	if (result < -0x80 || result >= 0x80) {
		softx86_int_sw_signal(ctx,0);
		return src;
	}

/* the result is stored into AL, remainder in AH */
	ctx->state->general_reg[SX86_REG_AX].b.hi =
		((sx86_sword)ctx->state->general_reg[SX86_REG_AX].w.lo) % ((sx86_sbyte)src);
	ctx->state->general_reg[SX86_REG_AX].b.lo = (sx86_ubyte)result;

/* Apparently all the flags are undefined after this instruction. */
	ctx->state->reg_flags.val &=
		~(SX86_CPUFLAG_SIGN | SX86_CPUFLAG_ZERO | SX86_CPUFLAG_AUX | SX86_CPUFLAG_PARITY);

	return src;	/* this is called by sx86_exec_full_modrmonly_ro() so it doesn't matter */
}

sx86_uword op_idiv16(softx86_ctx* ctx,sx86_uword src)
{
	sx86_sdword result;

/* signal division overflow */
	if (src == 0) {
		softx86_int_sw_signal(ctx,0);
		return src;
	}

/* peform the operation AX = DX:AX / r/m16 */
	result  = ((sx86_sdword)ctx->state->general_reg[SX86_REG_AX].w.lo);
	result |= ((sx86_sdword)ctx->state->general_reg[SX86_REG_DX].w.lo)<<16;
	result /= ((sx86_sword)src);

/* signal division overflow */
	if (result < -0x8000 || result >= 0x8000) {
		softx86_int_sw_signal(ctx,0);
		return src;
	}

/* the result is stored into AX, remainder in DX */
	ctx->state->general_reg[SX86_REG_DX].w.lo =
		((sx86_sword)ctx->state->general_reg[SX86_REG_AX].w.lo) % ((sx86_sword)src);
	ctx->state->general_reg[SX86_REG_AX].w.lo = (sx86_uword)result;

/* Apparently all the flags are undefined after this instruction. */
	ctx->state->reg_flags.val &=
		~(SX86_CPUFLAG_SIGN | SX86_CPUFLAG_ZERO | SX86_CPUFLAG_AUX | SX86_CPUFLAG_PARITY);

	return src;	/* this is called by sx86_exec_full_modrmonly_ro() so it doesn't matter */
}

sx86_udword op_idiv32(softx86_ctx* ctx,sx86_udword src)
{
	sx86_sldword result;

/* signal division overflow */
	if (src == 0) {
		softx86_int_sw_signal(ctx,0);
		return src;
	}

/* peform the operation AX = EDX:EAX / r/m32 */
	result  = ((sx86_sldword)ctx->state->general_reg[SX86_REG_AX].val);
	result |= ((sx86_sldword)ctx->state->general_reg[SX86_REG_DX].val)<<32;
	result /= ((sx86_sldword)src);

/* signal division overflow */
	if (result < -0x80000000 || result >= 0x80000000) {
		softx86_int_sw_signal(ctx,0);
		return src;
	}

/* the result is stored into EAX, remainder in EDX */
	ctx->state->general_reg[SX86_REG_DX].val =
		((sx86_sdword)ctx->state->general_reg[SX86_REG_AX].val) % ((sx86_sdword)src);
	ctx->state->general_reg[SX86_REG_AX].val = (sx86_sdword)result;

/* Apparently all the flags are undefined after this instruction. */
	ctx->state->reg_flags.val &=
		~(SX86_CPUFLAG_SIGN | SX86_CPUFLAG_ZERO | SX86_CPUFLAG_AUX | SX86_CPUFLAG_PARITY);

	return src;	/* this is called by sx86_exec_full_modrmonly_ro() so it doesn't matter */
}

int Sfx86OpcodeExec_xor(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if ((opcode&0xFC) == 0x30) {					// XOR reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte w16,opswap;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		opswap = (opcode&2)>>1;					// XOR from reg to r/m UNLESS this bit it set (then it's the other way around)
		x      = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_exec_full_modregrm_rw(ctx,w16,0,mod,reg,
			rm,opswap,op_xor8,op_xor16,op_xor32);

		return 1;
	}
	if (opcode == 0x34) {						// XOR AL,imm8
		sx86_ubyte x;

		x = softx86_fetch_exec_byte(ctx);
		x = op_xor8(ctx,ctx->state->general_reg[SX86_REG_AX].b.lo,x);
		ctx->state->general_reg[SX86_REG_AX].b.lo = x;
		return 1;
	}
	if (opcode == 0x35) {						// XOR AX,imm16
		sx86_uword x;

		x  = softx86_fetch_exec_byte(ctx);
		x |= softx86_fetch_exec_byte(ctx) << 8;
		x  = op_xor16(ctx,ctx->state->general_reg[SX86_REG_AX].w.lo,x);
		ctx->state->general_reg[SX86_REG_AX].w.lo = x;
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_xor(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if ((opcode&0xFC) == 0x30) {					// XOR reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte w16,opswap;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		opswap = (opcode&2)>>1;					// XOR from reg to r/m UNLESS this bit it set (then it's the other way around)
		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modregrm(ctx,w16,0,mod,reg,rm,op1_tmp,op2_tmp);

		if (opswap)	sprintf(buf,"XOR %s,%s",op2_tmp,op1_tmp);
		else		sprintf(buf,"XOR %s,%s",op1_tmp,op2_tmp);
		return 1;
	}
	if (opcode == 0x34) {						// XOR AL,imm8
		sx86_ubyte x;

		x = softx86_fetch_dec_byte(ctx);
		sprintf(buf,"XOR AL,%02Xh",x);
		return 1;
	}
	if (opcode == 0x35) {						// XOR AX,imm16
		sx86_uword x;

		x  = softx86_fetch_dec_byte(ctx);
		x |= softx86_fetch_dec_byte(ctx) << 8;
		sprintf(buf,"XOR AX,%04Xh",x);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeExec_or(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if ((opcode&0xFC) == 0x08) {					// OR reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte w16,opswap;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		opswap = (opcode&2)>>1;					// OR from reg to r/m UNLESS this bit it set (then it's the other way around)
		x      = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_exec_full_modregrm_rw(ctx,w16,0,mod,reg,
			rm,opswap,op_or8,op_or16,op_or32);

		return 1;
	}
	if (opcode == 0x0C) {						// OR AL,imm8
		sx86_ubyte x;

		x = softx86_fetch_exec_byte(ctx);
		x = op_or8(ctx,ctx->state->general_reg[SX86_REG_AX].b.lo,x);
		ctx->state->general_reg[SX86_REG_AX].b.lo = x;
		return 1;
	}
	if (opcode == 0x0D) {						// OR AX,imm16
		sx86_uword x;

		x  = softx86_fetch_exec_byte(ctx);
		x |= softx86_fetch_exec_byte(ctx) << 8;
		x  = op_or16(ctx,ctx->state->general_reg[SX86_REG_AX].w.lo,x);
		ctx->state->general_reg[SX86_REG_AX].w.lo = x;
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_or(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if ((opcode&0xFC) == 0x08) {					// OR reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte w16,opswap;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		opswap = (opcode&2)>>1;					// OR from reg to r/m UNLESS this bit it set (then it's the other way around)
		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modregrm(ctx,w16,0,mod,reg,rm,op1_tmp,op2_tmp);

		if (opswap)	sprintf(buf,"OR %s,%s",op2_tmp,op1_tmp);
		else		sprintf(buf,"OR %s,%s",op1_tmp,op2_tmp);
		return 1;
	}
	if (opcode == 0x0C) {						// OR AL,imm8
		sx86_ubyte x;

		x = softx86_fetch_dec_byte(ctx);
		sprintf(buf,"OR AL,%02Xh",x);
		return 1;
	}
	if (opcode == 0x0D) {						// OR AX,imm16
		sx86_uword x;

		x  = softx86_fetch_dec_byte(ctx);
		x |= softx86_fetch_dec_byte(ctx) << 8;
		sprintf(buf,"OR AX,%04Xh",x);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeExec_and(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if ((opcode&0xFC) == 0x20) {					// AND reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte w16,opswap;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		opswap = (opcode&2)>>1;					// AND from reg to r/m UNLESS this bit it set (then it's the other way around)
		x      = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_exec_full_modregrm_rw(ctx,w16,0,mod,reg,
			rm,opswap,op_and8,op_and16,op_and32);

		return 1;
	}
	if (opcode == 0x24) {						// AND AL,imm8
		sx86_ubyte x;

		x = softx86_fetch_exec_byte(ctx);
		x = op_and8(ctx,ctx->state->general_reg[SX86_REG_AX].b.lo,x);
		ctx->state->general_reg[SX86_REG_AX].b.lo = x;
		return 1;
	}
	if (opcode == 0x25) {						// AND AX,imm16
		sx86_uword x;

		x  = softx86_fetch_exec_byte(ctx);
		x |= softx86_fetch_exec_byte(ctx) << 8;
		x  = op_and16(ctx,ctx->state->general_reg[SX86_REG_AX].w.lo,x);
		ctx->state->general_reg[SX86_REG_AX].w.lo = x;
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_and(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if ((opcode&0xFC) == 0x20) {					// AND reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte w16,opswap;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		opswap = (opcode&2)>>1;					// AND from reg to r/m UNLESS this bit it set (then it's the other way around)
		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modregrm(ctx,w16,0,mod,reg,rm,op1_tmp,op2_tmp);

		if (opswap)	sprintf(buf,"AND %s,%s",op2_tmp,op1_tmp);
		else		sprintf(buf,"AND %s,%s",op1_tmp,op2_tmp);
		return 1;
	}
	if (opcode == 0x24) {						// AND AL,imm8
		sx86_ubyte x;

		x = softx86_fetch_dec_byte(ctx);
		sprintf(buf,"AND AL,%02Xh",x);
		return 1;
	}
	if (opcode == 0x25) {						// AND AX,imm16
		sx86_uword x;

		x  = softx86_fetch_dec_byte(ctx);
		x |= softx86_fetch_dec_byte(ctx) << 8;
		sprintf(buf,"AND AX,%04Xh",x);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeExec_test(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if ((opcode&0xFE) == 0x84) {					// TEST reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte w16,opswap;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		opswap = (opcode&2)>>1;					// TEST from reg to r/m UNLESS this bit it set (then it's the other way around)
		x      = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_exec_full_modregrm_ro(ctx,w16,0,mod,reg,		// unlike AND, TEST does not
			rm,opswap,op_test8,op_test16,op_test32);	// modify the dest. operand

		return 1;
	}
	if (opcode == 0xA8) {						// TEST AL,imm8
		sx86_ubyte x;

		x = softx86_fetch_exec_byte(ctx);
		op_test8(ctx,ctx->state->general_reg[SX86_REG_AX].b.lo,x);
		return 1;
	}
	if (opcode == 0xA9) {						// TEST AX,imm16
		sx86_uword x;

		x  = softx86_fetch_exec_byte(ctx);
		x |= softx86_fetch_exec_byte(ctx) << 8;
		op_test16(ctx,ctx->state->general_reg[SX86_REG_AX].w.lo,x);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_test(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if ((opcode&0xFE) == 0x84) {					// TEST reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte w16,opswap;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		opswap = (opcode&2)>>1;					// TEST from reg to r/m UNLESS this bit it set (then it's the other way around)
		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modregrm(ctx,w16,0,mod,reg,rm,op1_tmp,op2_tmp);

		if (opswap)	sprintf(buf,"TEST %s,%s",op2_tmp,op1_tmp);
		else		sprintf(buf,"TEST %s,%s",op1_tmp,op2_tmp);
		return 1;
	}
	if (opcode == 0xA8) {						// TEST AL,imm8
		sx86_ubyte x;

		x = softx86_fetch_dec_byte(ctx);
		sprintf(buf,"TEST AL,%02Xh",x);
		return 1;
	}
	if (opcode == 0xA9) {						// TEST AX,imm16
		sx86_uword x;

		x  = softx86_fetch_dec_byte(ctx);
		x |= softx86_fetch_dec_byte(ctx) << 8;
		sprintf(buf,"TEST AX,%04Xh",x);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeExec_xchg(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if ((opcode&0xFE) == 0x86) {					// XCHG reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte w16;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		x      = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_exec_full_modregrm_xchg(ctx,w16,0,mod,reg,rm);

		return 1;
	}
	if ((opcode&0xF8) == 0x90) {					// XCHG reg16,AX
		sx86_uword tmp;
		int ro;

/* interesting fact: Apparently the opcode 0x90 that Intel documents as NOP (no-op) also
                     decodes to XCHG AX,AX (which does nothing anyway) */
		ro = opcode - 0x90;
		tmp = ctx->state->general_reg[ro].w.lo;
		ctx->state->general_reg[ro].w.lo = ctx->state->general_reg[SX86_REG_AX].w.lo;
		ctx->state->general_reg[SX86_REG_AX].w.lo = tmp;

		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_xchg(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if ((opcode&0xFE) == 0x86) {					// XCHG reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte w16;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modregrm(ctx,w16,0,mod,reg,rm,op1_tmp,op2_tmp);

		sprintf(buf,"XCHG %s,%s",op1_tmp,op2_tmp);		// for this instruction either way works
		return 1;
	}
	if ((opcode&0xF8) == 0x90) {					// XCHG reg16,AX
		sprintf(buf,"XCHG %s,AX",sx86_regs16[opcode-0x90]);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeExec_lea(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if (opcode == 0x8D) {						// LEA reg,reg/mem
		sx86_ubyte x;
		sx86_ubyte mod,reg,rm;

		x = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_exec_full_modregrm_lea(ctx,0,mod,reg,rm);

		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_lea(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if (opcode == 0x8D) {						// LEA reg,reg/mem
		sx86_ubyte x;
		sx86_ubyte mod,reg,rm;

		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modregrm(ctx,1,0,mod,reg,rm,op1_tmp,op2_tmp);

		sprintf(buf,"LEA %s,%s",op2_tmp,op1_tmp);		// for this instruction either way works
		return 1;
	}

	return 0;
}

sx86_sword op_bound16(softx86_ctx* ctx,sx86_sword idx,sx86_sword upper,sx86_sword lower)
{
	if (idx >= lower && idx <= (upper+2)) {
		/* everything is fine */
		return lower;
	}
	else {
		/* reset instruction pointer */
		softx86_set_near_instruction_ptr(ctx,ctx->state->reg_ip_exec_initial);
		/* signal BOUNDs exception */
		softx86_int_sw_signal(ctx,5);
	}

	return lower;
}

sx86_sdword op_bound32(softx86_ctx* ctx,sx86_sdword idx,sx86_sdword upper,sx86_sdword lower)
{
	if (idx >= lower && idx <= (upper+4)) {
		/* everything is fine */
		return lower;
	}
	else {
		/* reset instruction pointer */
		softx86_set_near_instruction_ptr(ctx,ctx->state->reg_ip_exec_initial);
		/* signal BOUNDs exception */
		softx86_int_sw_signal(ctx,5);
	}

	return lower;
}

/* TODO: I get the impression that the BOUNDs instruction was not
         part of the original 8086 instruction set. When was it added? */
int Sfx86OpcodeExec_bound(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if (opcode == 0x62 && ctx->__private->level >= SX86_CPULEVEL_8086) {	// BOUND reg,mem
		sx86_ubyte x;
		sx86_ubyte mod,reg,rm;

		x = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_exec_full_modregrm_far_ro3(ctx,0,mod,reg,rm,op_bound16,op_bound32);

		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_bound(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if (opcode == 0x62 && ctx->__private->level >= SX86_CPULEVEL_8086) {	// BOUND reg,mem
		sx86_ubyte x;
		sx86_ubyte mod,reg,rm;

		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modregrm(ctx,1,0,mod,reg,rm,op1_tmp,op2_tmp);

		sprintf(buf,"BOUND %s,%s",op2_tmp,op1_tmp);		// for this instruction either way works
		return 1;
	}

	return 0;
}
