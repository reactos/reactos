/*
 * pushpop.c
 *
 * Copyright (C) 2003, 2004 Jonathan Campbell <jcampbell@mdjk.com>
 *
 * Decompiler and executioneer for PUSH/POP/PUSHF/POPF/SAHF/LAHF instructions.
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
#include "pushpop.h"

int Sfx86OpcodeExec_pop(sx86_ubyte opcode,softx86_ctx* ctx)
{
	sx86_uword val;

	if (opcode >= 0x58 && opcode <= 0x5F) {		// POP [reg]
		val = softx86_stack_popw(ctx);
		ctx->state->general_reg[opcode-0x58].w.lo = val;
		return 1;
	}

	if (opcode == 0x1F) {						// POP DS
		val = softx86_stack_popw(ctx);
		softx86_setsegval(ctx,SX86_SREG_DS,val);
		return 1;
	}

	if (opcode == 0x07) {						// POP ES
		val = softx86_stack_popw(ctx);
		softx86_setsegval(ctx,SX86_SREG_ES,val);
		return 1;
	}

	if (opcode == 0x17) {						// POP SS
		val = softx86_stack_popw(ctx);
		softx86_setsegval(ctx,SX86_SREG_SS,val);
		return 1;
	}

	if (opcode == 0x0F) {						// POP CS (8086/8088 only)
		if (ctx->__private->level <= SX86_CPULEVEL_80186) {
			val = softx86_stack_popw(ctx);
			softx86_setsegval(ctx,SX86_SREG_CS,val);
		}
		else {
			Sfx86OpcodeTable* sop;
			sx86_ubyte op2;

			/* leap into the second-level opcode table */
			sop = (Sfx86OpcodeTable*)ctx->__private->opcode_table_sub_0Fh;
			op2 = softx86_fetch_exec_byte(ctx);
			return sop->table[op2].exec(op2,ctx);
		}

		return 1;
	}

	if (opcode == 0x9D) {						// POPF
		ctx->state->reg_flags.w.lo = softx86_stack_popw(ctx);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_pop(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if (opcode >= 0x58 && opcode <= 0x5F) {		// POP [reg]
		sprintf(buf,"POP %s",sx86_regs16[opcode-0x58]);
		return 1;
	}

	if (opcode == 0x1F) {						// POP DS
		strcpy(buf,"POP     DS");
		return 1;
	}

	if (opcode == 0x07) {						// POP ES
		strcpy(buf,"POP     ES");
		return 1;
	}

	if (opcode == 0x17) {						// POP SS
		strcpy(buf,"POP     SS");
		return 1;
	}

	if (opcode == 0x0F) {						// POP CS (8086/8088 only)
		if (ctx->__private->level <= SX86_CPULEVEL_80186) {
			strcpy(buf,"POP     CS");
			return 1;
		}
		else {
			Sfx86OpcodeTable* sop;
			sx86_ubyte op2;

			/* leap into the second-level opcode table */
			sop = (Sfx86OpcodeTable*)ctx->__private->opcode_table_sub_0Fh;
			op2 = softx86_fetch_dec_byte(ctx);
			return sop->table[op2].dec(op2,ctx,buf);
		}
	}

	if (opcode == 0x9D) {						// POPF
		strcpy(buf,"POPF");
		return 1;
	}

	return 0;
}

int Sfx86OpcodeExec_push(sx86_ubyte opcode,softx86_ctx* ctx)
{
	sx86_uword val;

	if (opcode >= 0x50 && opcode <= 0x57) {		// PUSH [reg]
		if (opcode == 0x54 && ctx->bugs->decrement_sp_before_store) // 0x54 = (E)SP
			val = ctx->state->general_reg[opcode-0x50].w.lo - 2;
		else
			val = ctx->state->general_reg[opcode-0x50].w.lo;

		softx86_stack_pushw(ctx,val);
		return 1;
	}

	if (opcode == 0x1E) {						// PUSH DS
		softx86_stack_pushw(ctx,ctx->state->segment_reg[SX86_SREG_DS].val);
		return 1;
	}

	if (opcode == 0x06) {						// PUSH ES
		softx86_stack_pushw(ctx,ctx->state->segment_reg[SX86_SREG_ES].val);
		return 1;
	}

	if (opcode == 0x16) {						// PUSH SS
		softx86_stack_pushw(ctx,ctx->state->segment_reg[SX86_SREG_SS].val);
		return 1;
	}

	if (opcode == 0x0E) {						// PUSH CS
		softx86_stack_pushw(ctx,ctx->state->segment_reg[SX86_SREG_CS].val);
		return 1;
	}

	if (opcode == 0x9C) {						// PUSHF
		softx86_stack_pushw(ctx,ctx->state->reg_flags.w.lo);
		return 1;
	}

	if (opcode == 0x6A)							// PUSH imm8
	{
		sx86_ubyte b = softx86_fetch_exec_byte(ctx);
		softx86_stack_pushw(ctx, (sx86_uword)b);
		return 1;
	}

	if (opcode == 0x68)							// PUSH imm16
	{
		sx86_uword w = softx86_fetch_exec_byte(ctx);
		w |= softx86_fetch_exec_byte(ctx) << 8;
		softx86_stack_pushw(ctx, w);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_push(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if (opcode >= 0x50 && opcode <= 0x57) {		// PUSH [reg]
		sprintf(buf,"PUSH %s",sx86_regs16[opcode-0x50]);
		return 1;
	}

	if (opcode == 0x1E) {						// PUSH DS
		strcpy(buf,"PUSH DS");
		return 1;
	}

	if (opcode == 0x06) {						// PUSH ES
		strcpy(buf,"PUSH ES");
		return 1;
	}

	if (opcode == 0x16) {						// PUSH SS
		strcpy(buf,"PUSH SS");
		return 1;
	}

	if (opcode == 0x0E) {						// PUSH CS
		strcpy(buf,"PUSH CS");
		return 1;
	}

	if (opcode == 0x9C) {						// PUSHF
		strcpy(buf,"PUSHF");
		return 1;
	}

	if (opcode == 0x6A && ctx->__private->level >= SX86_CPULEVEL_80186)	// PUSH imm8
	{
		sx86_ubyte b = softx86_fetch_exec_byte(ctx);
		sprintf(buf, "PUSH %02Xh", b);
		return 1;
	}

	if (opcode == 0x68 && ctx->__private->level >= SX86_CPULEVEL_80186)	// PUSH imm16
	{
		sx86_uword w = softx86_fetch_exec_byte(ctx);
		w |= softx86_fetch_exec_byte(ctx) << 8;
		sprintf(buf, "PUSH %04Xh", w);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeExec_ahf(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if (opcode == 0x9E) {						// SAHF
		ctx->state->reg_flags.b.lo &= ~0xD5;
		ctx->state->reg_flags.b.lo |=  (ctx->state->general_reg[SX86_REG_AX].b.hi&0xD5);
		return 1;
	}

	if (opcode == 0x9F) {						// LAHF
		ctx->state->general_reg[SX86_REG_AX].b.hi = ctx->state->reg_flags.b.lo;
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_ahf(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if (opcode == 0x9E) {						// SAHF
		strcpy(buf,"SAHF");
		return 1;
	}

	if (opcode == 0x9F) {						// LAHF
		strcpy(buf,"LAHF");
		return 1;
	}

	return 0;
}

int Sfx86OpcodeExec_pusha(sx86_ubyte opcode,softx86_ctx* ctx)
{
/* TODO: When did PUSHA/POPA really appear in the x86 instruction set?
         Does this originate as far back as the original 8086?
	 Documentation seems to be scarce about this... */
	if (opcode == 0x60 && ctx->__private->level >= SX86_CPULEVEL_80186) { // PUSHA/PUSHAD
		sx86_uword temp_sp;

		temp_sp = ctx->state->general_reg[SX86_REG_SP].w.lo;
		softx86_stack_pushw(ctx,ctx->state->general_reg[SX86_REG_AX].w.lo);
		softx86_stack_pushw(ctx,ctx->state->general_reg[SX86_REG_CX].w.lo);
		softx86_stack_pushw(ctx,ctx->state->general_reg[SX86_REG_DX].w.lo);
		softx86_stack_pushw(ctx,ctx->state->general_reg[SX86_REG_BX].w.lo);

/* emulate bug in PUSHA that exist in CPUs prior to 286 */
		if (ctx->bugs->decrement_sp_before_store)
			softx86_stack_pushw(ctx,ctx->state->general_reg[SX86_REG_SP].w.lo);
		else
			softx86_stack_pushw(ctx,temp_sp);

		softx86_stack_pushw(ctx,ctx->state->general_reg[SX86_REG_BP].w.lo);
		softx86_stack_pushw(ctx,ctx->state->general_reg[SX86_REG_SI].w.lo);
		softx86_stack_pushw(ctx,ctx->state->general_reg[SX86_REG_DI].w.lo);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_pusha(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
/* TODO: When did PUSHA/POPA really appear in the x86 instruction set?
         Does this originate as far back as the original 8086?
	 Documentation seems to be scarce about this... */
	if (opcode == 0x60 && ctx->__private->level >= SX86_CPULEVEL_80186) { // PUSHA/PUSHAD
		strcpy(buf,"PUSHA");
		return 1;
	}

	return 0;
}

int Sfx86OpcodeExec_popa(sx86_ubyte opcode,softx86_ctx* ctx)
{
/* TODO: When did PUSHA/POPA really appear in the x86 instruction set?
         Does this originate as far back as the original 8086?
	 Documentation seems to be scarce about this... */
	if (opcode == 0x61 && ctx->__private->level >= SX86_CPULEVEL_80186) { // POPA/POPAD
		ctx->state->general_reg[SX86_REG_DI].w.lo = softx86_stack_popw(ctx);
		ctx->state->general_reg[SX86_REG_SI].w.lo = softx86_stack_popw(ctx);
		ctx->state->general_reg[SX86_REG_BP].w.lo = softx86_stack_popw(ctx);
/* TODO: all versions prior to the 286 had bugs storing SP...
         does that also mean they were stupid enough to
         restore it? anybody know? */
		softx86_stack_popw(ctx);
		ctx->state->general_reg[SX86_REG_BX].w.lo = softx86_stack_popw(ctx);
		ctx->state->general_reg[SX86_REG_DX].w.lo = softx86_stack_popw(ctx);
		ctx->state->general_reg[SX86_REG_CX].w.lo = softx86_stack_popw(ctx);
		ctx->state->general_reg[SX86_REG_AX].w.lo = softx86_stack_popw(ctx);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_popa(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
/* TODO: When did PUSHA/POPA really appear in the x86 instruction set?
         Does this originate as far back as the original 8086?
	 Documentation seems to be scarce about this... */
	if (opcode == 0x61 && ctx->__private->level >= SX86_CPULEVEL_80186) { // POPA/POPAD
		strcpy(buf,"POPA");
		return 1;
	}

	return 0;
}

