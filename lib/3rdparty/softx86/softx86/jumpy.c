/*
 * jumpy.c
 *
 * Copyright (C) 2003, 2004 Jonathan Campbell <jcampbell@mdjk.com>
 *
 * Decompiler and executioneer for CALL/J<condition> instructions.
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
#include "jumpy.h"

int Sfx86OpcodeExec_jc(sx86_ubyte opcode,softx86_ctx* ctx)
{
	int tf;
	sx86_udword rel;

/* Intel's tendancy to sort the instructions in their documentation alphabetically
   by name instead of by opcode tends to distort the fact that all of these
   opcodes (except for JCXZ) lie between 0x70 and 0x7F. */
	if (!(opcode == 0xE3 || ((opcode&0xF0) == 0x70))) return 0;

	rel = (sx86_udword)softx86_fetch_exec_byte(ctx);
	if (rel & 0x80) rel |= 0xFFFFFF00;
	rel += ctx->state->reg_ip;
	rel &= 0xFFFF;

/* JCXZ */
	if (opcode == 0xE3)
		tf = (ctx->state->general_reg[SX86_REG_CX].w.lo == 0);
/* JO */
	else if (opcode == 0x70)
		tf = (ctx->state->reg_flags.val & SX86_CPUFLAG_OVERFLOW);
/* JNO */
	else if (opcode == 0x71)
		tf = !(ctx->state->reg_flags.val & SX86_CPUFLAG_OVERFLOW);
/* JC */
	else if (opcode == 0x72)
		tf = (ctx->state->reg_flags.val & SX86_CPUFLAG_CARRY);
/* JNC */
	else if (opcode == 0x73)
		tf = !(ctx->state->reg_flags.val & SX86_CPUFLAG_CARRY);
/* JZ */
	else if (opcode == 0x74)
		tf = (ctx->state->reg_flags.val & SX86_CPUFLAG_ZERO);
/* JNZ */
	else if (opcode == 0x75)
		tf = !(ctx->state->reg_flags.val & SX86_CPUFLAG_ZERO);
/* JBE */
	else if (opcode == 0x76)
		tf =	(ctx->state->reg_flags.val & SX86_CPUFLAG_ZERO) ||
			(ctx->state->reg_flags.val & SX86_CPUFLAG_CARRY);
/* JA */
	else if (opcode == 0x77)
		tf =	(!(ctx->state->reg_flags.val & SX86_CPUFLAG_ZERO)) &&
			(!(ctx->state->reg_flags.val & SX86_CPUFLAG_CARRY));
/* JS */
	else if (opcode == 0x78)
		tf = (ctx->state->reg_flags.val & SX86_CPUFLAG_SIGN);
/* JNS */
	else if (opcode == 0x79)
		tf = !(ctx->state->reg_flags.val & SX86_CPUFLAG_SIGN);
/* JP */
	else if (opcode == 0x7A)
		tf = (ctx->state->reg_flags.val & SX86_CPUFLAG_PARITY);
/* JNP */
	else if (opcode == 0x7B)
		tf = !(ctx->state->reg_flags.val & SX86_CPUFLAG_PARITY);
/* JL */
	else if (opcode == 0x7C)
		tf =	 (((ctx->state->reg_flags.val & SX86_CPUFLAG_SIGN) ? 1 : 0) !=
			  ((ctx->state->reg_flags.val & SX86_CPUFLAG_OVERFLOW) ? 1 : 0));
/* JGE */
	else if (opcode == 0x7D)
		tf =	((ctx->state->reg_flags.val & SX86_CPUFLAG_SIGN) ? 1 : 0) ==
			((ctx->state->reg_flags.val & SX86_CPUFLAG_OVERFLOW) ? 1 : 0);
/* JLE */
	else if (opcode == 0x7E)
		tf =	(((ctx->state->reg_flags.val & SX86_CPUFLAG_SIGN) ? 1 : 0) !=
			 ((ctx->state->reg_flags.val & SX86_CPUFLAG_OVERFLOW) ? 1 : 0) ||
			 (ctx->state->reg_flags.val & SX86_CPUFLAG_ZERO));
/* JG */
	else if (opcode == 0x7F)
		tf =	 (((ctx->state->reg_flags.val & SX86_CPUFLAG_SIGN) ? 1 : 0) ==
			  ((ctx->state->reg_flags.val & SX86_CPUFLAG_OVERFLOW) ? 1 : 0) &&
			(!(ctx->state->reg_flags.val & SX86_CPUFLAG_ZERO)));
/* should NOT be here !*/
	else
		return 0;

/* if condition met, go! */
	if (tf)
		softx86_set_instruction_ptr(ctx,ctx->state->segment_reg[SX86_SREG_CS].val,rel);

	return 1;
}

int Sfx86OpcodeDec_jc(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	sx86_udword rel;

/* Intel's tendancy to sort the instructions in their documentation alphabetically
   by name instead of by opcode tends to distort the fact that all of these
   opcodes (except for JCXZ) lie between 0x70 and 0x7F. */
	if (!(opcode == 0xE3 || ((opcode&0xF0) == 0x70))) return 0;

	rel = (sx86_udword)softx86_fetch_dec_byte(ctx);
	if (rel & 0x80) rel |= 0xFFFFFF00;
	rel += ctx->state->reg_ip_decompiler;
	rel &= 0xFFFF;

	if (opcode == 0xE3)
		sprintf(buf,"JCXZ %04Xh",rel);
	else if (opcode == 0x70)
		sprintf(buf,"JO %04Xh",rel);
	else if (opcode == 0x71)
		sprintf(buf,"JNO %04Xh",rel);
	else if (opcode == 0x72)
		sprintf(buf,"JC %04Xh",rel);
	else if (opcode == 0x73)
		sprintf(buf,"JNC %04Xh",rel);
	else if (opcode == 0x74)
		sprintf(buf,"JZ %04Xh",rel);
	else if (opcode == 0x75)
		sprintf(buf,"JNZ %04Xh",rel);
	else if (opcode == 0x76)
		sprintf(buf,"JBE %04Xh",rel);
	else if (opcode == 0x77)
		sprintf(buf,"JA %04Xh",rel);
	else if (opcode == 0x78)
		sprintf(buf,"JS %04Xh",rel);
	else if (opcode == 0x79)
		sprintf(buf,"JNS %04Xh",rel);
	else if (opcode == 0x7A)
		sprintf(buf,"JP %04Xh",rel);
	else if (opcode == 0x7B)
		sprintf(buf,"JNP %04Xh",rel);
	else if (opcode == 0x7C)
		sprintf(buf,"JL %04Xh",rel);
	else if (opcode == 0x7D)
		sprintf(buf,"JGE %04Xh",rel);
	else if (opcode == 0x7E)
		sprintf(buf,"JLE %04Xh",rel);
	else if (opcode == 0x7F)
		sprintf(buf,"JG %04Xh",rel);
/* should NOT be here !*/
	else
		return 0;

	return 1;
}

int Sfx86OpcodeExec_call(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if (opcode == 0x9A) {		// CALL FAR seg:offs
		sx86_uword seg,ofs;

		ofs  =  (sx86_uword)softx86_fetch_exec_byte(ctx);
		ofs |= ((sx86_uword)softx86_fetch_exec_byte(ctx))<<8;
		seg  =  (sx86_uword)softx86_fetch_exec_byte(ctx);
		seg |= ((sx86_uword)softx86_fetch_exec_byte(ctx))<<8;
		softx86_stack_pushw(ctx,ctx->state->segment_reg[SX86_SREG_CS].val);
		softx86_stack_pushw(ctx,ctx->state->reg_ip);
		softx86_set_instruction_ptr(ctx,seg,ofs);
		return 1;
	}
	else if (opcode == 0xE8) {	// CALL rel16
		int ofs;

		ofs  =  (int)softx86_fetch_exec_byte(ctx);
		ofs |= ((int)softx86_fetch_exec_byte(ctx))<<8;
		if (ofs & 0x8000) ofs -= 0x10000;
		ofs +=  ctx->state->reg_ip;
		ofs &=  0xFFFF;
		softx86_stack_pushw(ctx,ctx->state->reg_ip);
		softx86_set_near_instruction_ptr(ctx,(sx86_udword)ofs);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_call(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if (opcode == 0x9A) {		// CALL FAR seg:offs
		sx86_uword seg,ofs;

		ofs  =  (sx86_uword)softx86_fetch_dec_byte(ctx);
		ofs |= ((sx86_uword)softx86_fetch_dec_byte(ctx))<<8;
		seg  =  (sx86_uword)softx86_fetch_dec_byte(ctx);
		seg |= ((sx86_uword)softx86_fetch_dec_byte(ctx))<<8;
		sprintf(buf,"CALL FAR %04X:%04X",seg,ofs);
		return 1;
	}
	else if (opcode == 0xE8) {	// CALL rel16
		int ofs;

		ofs  =  (int)softx86_fetch_dec_byte(ctx);
		ofs |= ((int)softx86_fetch_dec_byte(ctx))<<8;
		if (ofs & 0x8000) ofs -= 0x10000;
		ofs +=  ctx->state->reg_ip_decompiler;
		ofs &=  0xFFFF;
		sprintf(buf,"CALL %04X",ofs);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeExec_jmp(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if (opcode == 0xE9) {		// JMP rel16
		int ofs;

		ofs  =  (int)softx86_fetch_exec_byte(ctx);
		ofs |= ((int)softx86_fetch_exec_byte(ctx))<<8;
		if (ofs & 0x8000) ofs -= 0x10000;
		ofs +=  ctx->state->reg_ip;
		ofs &=  0xFFFF;
		softx86_set_near_instruction_ptr(ctx,(sx86_udword)ofs);
		return 1;
	}
	else if (opcode == 0xEA) {	// JMP FAR seg:offs
		sx86_uword seg,ofs;

		ofs  =  (sx86_uword)softx86_fetch_exec_byte(ctx);
		ofs |= ((sx86_uword)softx86_fetch_exec_byte(ctx))<<8;
		seg  =  (sx86_uword)softx86_fetch_exec_byte(ctx);
		seg |= ((sx86_uword)softx86_fetch_exec_byte(ctx))<<8;
		softx86_set_instruction_ptr(ctx,seg,ofs);
		return 1;
	}
	else if (opcode == 0xEB) {	// JMP rel8
		int ofs;

		ofs  =  (int)softx86_fetch_exec_byte(ctx);
		if (ofs & 0x80) ofs -= 0x100;
		ofs +=  ctx->state->reg_ip;
		ofs &=  0xFFFF;
		softx86_set_near_instruction_ptr(ctx,(sx86_udword)ofs);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_jmp(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if (opcode == 0xE9) {		// JMP rel16
		int ofs;

		ofs  =  (int)softx86_fetch_dec_byte(ctx);
		ofs |= ((int)softx86_fetch_dec_byte(ctx))<<8;
		if (ofs & 0x8000) ofs -= 0x10000;
		ofs +=  ctx->state->reg_ip_decompiler;
		ofs &=  0xFFFF;
		sprintf(buf,"JMP %04X",ofs);
		return 1;
	}
	else if (opcode == 0xEA) {	// JMP FAR seg:offs
		sx86_uword seg,ofs;

		ofs  =  (sx86_uword)softx86_fetch_dec_byte(ctx);
		ofs |= ((sx86_uword)softx86_fetch_dec_byte(ctx))<<8;
		seg  =  (sx86_uword)softx86_fetch_dec_byte(ctx);
		seg |= ((sx86_uword)softx86_fetch_dec_byte(ctx))<<8;
		sprintf(buf,"JMP FAR %04X:%04X",seg,ofs);
		return 1;
	}
	else if (opcode == 0xEB) {	// JMP rel8
		int ofs;

		ofs  =  (int)softx86_fetch_dec_byte(ctx);
		if (ofs & 0x80) ofs -= 0x100;
		ofs +=  ctx->state->reg_ip_decompiler;
		ofs &=  0xFFFF;
		sprintf(buf,"JMP %04X",ofs);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeExec_loop(sx86_ubyte opcode,softx86_ctx* ctx)
{
	int tf;
	sx86_udword rel;

	if (opcode < 0xE0 || opcode > 0xE2) return 0;
	rel = (sx86_udword)softx86_fetch_exec_byte(ctx);
	if (rel & 0x80) rel |= 0xFFFFFF00;
	rel += ctx->state->reg_ip;
	rel &= 0xFFFF;

	ctx->state->general_reg[SX86_REG_CX].w.lo--;

/* LOOP */
	if (opcode == 0xE2)
		tf =	(ctx->state->general_reg[SX86_REG_CX].w.lo != 0);
/* LOOPZ */
	else if (opcode == 0xE1)
		tf =	(ctx->state->reg_flags.val & SX86_CPUFLAG_ZERO) &&
			(ctx->state->general_reg[SX86_REG_CX].w.lo != 0);
/* LOOPNZ */
	else if (opcode == 0xE0)
		tf =	((ctx->state->reg_flags.val & SX86_CPUFLAG_ZERO) == 0) &&
			(ctx->state->general_reg[SX86_REG_CX].w.lo != 0);
/* should NOT be here !*/
	else
		return 0;

/* if condition met, go! */
	if (tf)
		softx86_set_instruction_ptr(ctx,ctx->state->segment_reg[SX86_SREG_CS].val,rel);

	return 1;
}

int Sfx86OpcodeDec_loop(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	sx86_udword rel;

	rel = (sx86_udword)softx86_fetch_dec_byte(ctx);
	if (rel & 0x80) rel |= 0xFFFFFF00;
	rel += ctx->state->reg_ip_decompiler;
	rel &= 0xFFFF;

	if (opcode == 0xE2)
		sprintf(buf,"LOOP %04Xh",rel);
	else if (opcode == 0xE1)
		sprintf(buf,"LOOPZ %04Xh",rel);
	else if (opcode == 0xE0)
		sprintf(buf,"LOOPNZ %04Xh",rel);
/* should NOT be here !*/
	else
		return 0;

	return 1;
}

