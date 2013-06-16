/*
 * inc.c
 *
 * Copyright (C) 2003, 2004 Jonathan Campbell <jcampbell@mdjk.com>
 *
 * Decompiler and executioneer for the INC/DEC instructions.
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
#include "inc.h"

sx86_ubyte op_dec8(softx86_ctx* ctx,sx86_ubyte src)
{
	sx86_ubyte ret;

/* peform the subtraction */
	ret = src-1;

/* apparently CF is not touched by this instruction */

/* if result treated as signed value is negative */
	if (ret & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* auxiliary flag emulation (not defined by Intel's documentation, but observed
   from many trials in DEBUG on an old Pentium Pro 166MHz I have kickin' around
   as a web server :)
   
   apparently aux is set if:
   given operation a - b = c, ((a AND 0Fh) - (b AND 0Fh) < 0).
   in other words, if the lowest nibbles subtracted cause carry. */
	if ((src&0xF) < 1)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_AUX;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity8(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_uword op_dec16(softx86_ctx* ctx,sx86_uword src)
{
	sx86_uword ret;

/* peform the subtraction */
	ret = src-1;

/* apparently CF is not touched by this instruction */

/* if result treated as signed value is negative */
	if (ret & 0x8000)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* auxiliary flag emulation (not defined by Intel's documentation, but observed
   from many trials in DEBUG on an old Pentium Pro 166MHz I have kickin' around
   as a web server :)
   
   apparently aux is set if:
   given operation a - b = c, ((a AND 0Fh) - (b AND 0Fh) < 0).
   in other words, if the lowest nibbles subtracted cause carry. */
	if ((src&0xF) < 1)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_AUX;
	else			
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity16(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_udword op_dec32(softx86_ctx* ctx,sx86_udword src)
{
	sx86_udword ret;

/* peform the subtraction */
	ret = src-1;

/* apparently CF is not touched by this instruction */

/* if result treated as signed value is negative */
	if (ret & 0x80000000)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* auxiliary flag emulation (not defined by Intel's documentation, but observed
   from many trials in DEBUG on an old Pentium Pro 166MHz I have kickin' around
   as a web server :)
   
   apparently aux is set if:
   given operation a - b = c, ((a AND 0Fh) - (b AND 0Fh) < 0).
   in other words, if the lowest nibbles subtracted cause carry. */
	if ((src&0xF) < 1)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_AUX;
	else			
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity16(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_ubyte op_inc8(softx86_ctx* ctx,sx86_ubyte src)
{
	sx86_ubyte ret;

/* peform the addition */
	ret = src+1;

/* apparently CF is not touched by this instruction */

/* if result treated as signed value is negative */
	if (ret & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* auxiliary flag emulation (not defined by Intel's documentation, but observed
   from many trials in DEBUG on an old Pentium Pro 166MHz I have kickin' around
   as a web server :)
   
   apparently aux is set if:
   given operation a + b = c, ((b AND 0Fh) + (a AND 0Fh) >= 0x10).
   in other words, if the lowest nibbles added together overflow. */
	if ((1+(src&0xF)) >= 0x10)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_AUX;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity8(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_uword op_inc16(softx86_ctx* ctx,sx86_uword src)
{
	sx86_uword ret;

/* peform the addition */
	ret = src+1;

/* apparently CF is not touched by this instruction */

/* if result treated as signed value is negative */
	if (ret & 0x8000)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* auxiliary flag emulation (not defined by Intel's documentation, but observed
   from many trials in DEBUG on an old Pentium Pro 166MHz I have kickin' around
   as a web server :)
   
   apparently aux is set if:
   given operation a + b = c, ((b AND 0Fh) + (a AND 0Fh) >= 0x10).
   in other words, if the lowest nibbles added together overflow. */
	if ((1+(src&0xF)) >= 0x10)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_AUX;
	else			
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity16(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_udword op_inc32(softx86_ctx* ctx,sx86_udword src)
{
	sx86_udword ret;

/* peform the addition */
	ret = src+1;

/* apparently CF is not touched by this instruction */

/* if result treated as signed value is negative */
	if (ret & 0x80000000)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* auxiliary flag emulation (not defined by Intel's documentation, but observed
   from many trials in DEBUG on an old Pentium Pro 166MHz I have kickin' around
   as a web server :)
   
   apparently aux is set if:
   given operation a + b = c, ((b AND 0Fh) + (a AND 0Fh) >= 0x10).
   in other words, if the lowest nibbles added together overflow. */
	if ((1+(src&0xF)) >= 0x10)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_AUX;
	else			
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity16(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

int Sfx86OpcodeExec_inc(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if ((opcode&0xF8) == 0x40) {					// INC [reg]
		sx86_uword w,i;

		i = opcode-0x40;
		w = ctx->state->general_reg[i].w.lo;
		ctx->state->general_reg[i].w.lo = op_inc16(ctx,w);
		return 1;
	}
	else if ((opcode&0xF8) == 0x48) {				// DEC [reg]
		sx86_uword w,i;

		i = opcode-0x48;
		w = ctx->state->general_reg[i].w.lo;
		ctx->state->general_reg[i].w.lo = op_dec16(ctx,w);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_inc(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if ((opcode&0xF8) == 0x40) {					// INC [reg]
		sprintf(buf,"INC %s",sx86_regs16[opcode-0x40]);
		return 1;
	}
	else if ((opcode&0xF8) == 0x48) {				// DEC [reg]
		sprintf(buf,"DEC %s",sx86_regs16[opcode-0x48]);
		return 1;
	}

	return 0;
}
