/*
 * aaa.c
 *
 * Copyright (C) 2003, 2004 Jonathan Campbell <jcampbell@mdjk.com>
 *
 * Decompiler and executioneer for AAA/AAD/AAM/AAS/DAA instructions.
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
#include "aaa.h"

int Sfx86OpcodeExec_aaaseries(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if (opcode == 0x27) {						// DAA
		sx86_ubyte al,nal;

		al = ctx->state->general_reg[SX86_REG_AX].b.lo;
		if (((al&0x0F) > 9) || (ctx->state->reg_flags.val & SX86_CPUFLAG_AUX)) {
			nal = (al&0xF)+6;
			al += 6;
/* if addition to lower nibble overflowed... */
			if (nal >= 0x10) ctx->state->reg_flags.val |= SX86_CPUFLAG_CARRY;
			ctx->state->reg_flags.val |= SX86_CPUFLAG_AUX;
		}
		else {
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;
		}

		if (((al & 0xF0) > 0x90) || (ctx->state->reg_flags.val & SX86_CPUFLAG_CARRY)) {
			al += 0x60;
			ctx->state->reg_flags.val |= SX86_CPUFLAG_CARRY;
		}
		else {
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;
		}

/* "The OF flag is undefined"... */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;

/* SF emulation */
		if (al & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
		else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* ZF emulation */
		if (!al)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
		else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* PF emulation */
		if (softx86_parity8(al))	ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
		else				ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

		ctx->state->general_reg[SX86_REG_AX].b.lo = al;
		return 1;
	}

	if (opcode == 0x2F) {						// DAS
		sx86_ubyte al;
		int nal;

		al = ctx->state->general_reg[SX86_REG_AX].b.lo;
		if (((al&0x0F) > 9) || (ctx->state->reg_flags.val & SX86_CPUFLAG_AUX)) {
			nal = (al&0xF)-6;
			al -= 6;
/* if subtraction from lower nibble overflowed... */
			if (nal < 0) ctx->state->reg_flags.val |= SX86_CPUFLAG_CARRY;
			ctx->state->reg_flags.val |= SX86_CPUFLAG_AUX;
		}
		else {
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;
		}

		if ((al > 0x9F) || (ctx->state->reg_flags.val & SX86_CPUFLAG_CARRY)) {
			al -= 0x60;
			ctx->state->reg_flags.val |= SX86_CPUFLAG_CARRY;
		}
		else {
			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;
		}

/* "The OF flag is undefined"... */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;

/* SF emulation */
		if (al & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
		else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* ZF emulation */
		if (!al)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
		else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* PF emulation */
		if (softx86_parity8(al))	ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
		else				ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

		ctx->state->general_reg[SX86_REG_AX].b.lo = al;
		return 1;
	}

	if (opcode == 0x37) {						// AAA
		sx86_ubyte ah,al;

		al = ctx->state->general_reg[SX86_REG_AX].b.lo;
		ah = ctx->state->general_reg[SX86_REG_AX].b.hi;

		if (((al&0xF) > 9) || (ctx->state->reg_flags.val & SX86_CPUFLAG_AUX)) {
			al += 6; ah++;
			ctx->state->reg_flags.val |=  (SX86_CPUFLAG_AUX | SX86_CPUFLAG_CARRY);
		}
		else {
			ctx->state->reg_flags.val &= ~(SX86_CPUFLAG_AUX | SX86_CPUFLAG_CARRY);
		}

		ctx->state->general_reg[SX86_REG_AX].b.hi = ah;
		ctx->state->general_reg[SX86_REG_AX].b.lo = al & 0xF;

/* "OF, SF, ZF, and PF flags are undefined" */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;

		return 1;
	}

	if (opcode == 0x3F) {						// AAS
		sx86_ubyte ah,al;

		al = ctx->state->general_reg[SX86_REG_AX].b.lo;
		ah = ctx->state->general_reg[SX86_REG_AX].b.hi;

		if (((al&0xF) > 9) || (ctx->state->reg_flags.val & SX86_CPUFLAG_AUX)) {
			al -= 6; ah--;
			ctx->state->reg_flags.val |=  (SX86_CPUFLAG_AUX | SX86_CPUFLAG_CARRY);
		}
		else {
			ctx->state->reg_flags.val &= ~(SX86_CPUFLAG_AUX | SX86_CPUFLAG_CARRY);
		}

		ctx->state->general_reg[SX86_REG_AX].b.hi = ah;
		ctx->state->general_reg[SX86_REG_AX].b.lo = al & 0xF;

/* "OF, SF, ZF, and PF flags are undefined" */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;

		return 1;
	}

	if (opcode == 0xD4) {						// AAM
		sx86_ubyte ah,al,x;

		x  = softx86_fetch_exec_byte(ctx);
		al = ctx->state->general_reg[SX86_REG_AX].b.lo;

		if (x == 0) {
			/* TODO: signal division by zero exception */
		}
		else {
			al = al % x;
			ah = al / x;
		}

		ctx->state->general_reg[SX86_REG_AX].b.hi = ah;
		ctx->state->general_reg[SX86_REG_AX].b.lo = al;

/* SF emulation */
		if (al & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
		else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* ZF emulation */
		if (!al)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
		else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* PF emulation */
		if (softx86_parity8(al))	ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
		else				ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

/* "The OF, AF and CF flags are undefined"... */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;

		return 1;
	}

	if (opcode == 0xD5) {						// AAD
		sx86_ubyte ah,al,x;

		x  = softx86_fetch_exec_byte(ctx);
		al = ctx->state->general_reg[SX86_REG_AX].b.lo;
		ah = ctx->state->general_reg[SX86_REG_AX].b.hi;

		al = (al + (ah * x)) & 0xFF;
		ah = 0;

		ctx->state->general_reg[SX86_REG_AX].b.hi = ah;
		ctx->state->general_reg[SX86_REG_AX].b.lo = al;

/* SF emulation */
		if (al & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
		else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* ZF emulation */
		if (!al)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
		else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* PF emulation */
		if (softx86_parity8(al))	ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
		else				ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

/* "The OF, AF and CF flags are undefined"... */
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;

		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_aaaseries(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	sx86_ubyte x;

	if (opcode == 0x27) {						// DAA
		strcpy(buf,"DAA");
		return 1;
	}

	if (opcode == 0x2F) {						// DAS
		strcpy(buf,"DAS");
		return 1;
	}

	if (opcode == 0x37) {						// AAA
		strcpy(buf,"AAA");
		return 1;
	}

	if (opcode == 0x3F) {						// AAS
		strcpy(buf,"AAS");
		return 1;
	}

	if (opcode == 0xD4) {						// AAM
		x = softx86_fetch_dec_byte(ctx);
		if (x == 10)	strcpy(buf,"AAM");
		else		sprintf(buf,"AAM %d",x);

		return 1;
	}

	if (opcode == 0xD5) {						// AAD
		x = softx86_fetch_dec_byte(ctx);
		if (x == 10)	strcpy(buf,"AAD");
		else		sprintf(buf,"AAD %d",x);

		return 1;
	}

	return 0;
}
