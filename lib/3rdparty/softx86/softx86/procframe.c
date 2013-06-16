/*
 * procframe.c
 *
 * Copyright (C) 2003, 2004 Jonathan Campbell <jcampbell@mdjk.com>
 *
 * Decompiler and executioneer for RET/RETF/IRET instructions.
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
#include "procframe.h"

int Sfx86OpcodeExec_returns(sx86_ubyte opcode,softx86_ctx* ctx)
{
	sx86_uword seg,ofs,flg;

	if (opcode == 0xC2) {						// RET (near) + pop n bytes
		sx86_uword popeye;

		popeye  = softx86_fetch_exec_byte(ctx);
		popeye |= softx86_fetch_exec_byte(ctx)<<8;

		ofs = softx86_stack_popw(ctx);
		softx86_set_near_instruction_ptr(ctx,ofs);
		softx86_stack_discard_n(ctx,(int)popeye);
		return 1;
	}
	if (opcode == 0xC3) {						// RET (near)
		ofs = softx86_stack_popw(ctx);
		softx86_set_near_instruction_ptr(ctx,ofs);
		return 1;
	}
	if (opcode == 0xCA) {						// RETF (far) + pop n bytes
		sx86_uword popeye;

		popeye  = softx86_fetch_exec_byte(ctx);
		popeye |= softx86_fetch_exec_byte(ctx)<<8;

		ofs = softx86_stack_popw(ctx);
		seg = softx86_stack_popw(ctx);
		softx86_set_instruction_ptr(ctx,seg,ofs);
		softx86_stack_discard_n(ctx,(int)popeye);
		return 1;
	}
	if (opcode == 0xCB) {						// RETF (far)
		ofs = softx86_stack_popw(ctx);
		seg = softx86_stack_popw(ctx);
		softx86_set_instruction_ptr(ctx,seg,ofs);
		return 1;
	}
	if (opcode == 0xCF) {						// IRET/IRETD
		ofs = softx86_stack_popw(ctx);
		seg = softx86_stack_popw(ctx);
		flg = softx86_stack_popw(ctx);
		softx86_set_instruction_ptr(ctx,seg,ofs);
		ctx->state->reg_flags.val = flg;
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_returns(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if (opcode == 0xC2) {						// RET (near) + pop n bytes
		sx86_uword popeye;
		popeye  = softx86_fetch_dec_byte(ctx);
		popeye |= softx86_fetch_dec_byte(ctx)<<8;
		sprintf(buf,"RET 0x%02X",popeye);
		return 1;
	}
	if (opcode == 0xC3) {						// RET (near)
		strcpy(buf,"RET");
		return 1;
	}
	if (opcode == 0xCA) {						// RETF (far) + pop n bytes
		sx86_uword popeye;
		popeye  = softx86_fetch_dec_byte(ctx);
		popeye |= softx86_fetch_dec_byte(ctx)<<8;
		sprintf(buf,"RETF 0x%02X",popeye);
		return 1;
	}
	if (opcode == 0xCB) {						// RETF (far)
		strcpy(buf,"RETF");
		return 1;
	}
	if (opcode == 0xCF) {						// IRET/IRETD
		strcpy(buf,"IRET");
		return 1;
	}

	return 0;
}

/* TODO: What revision of the 8086 did ENTER and LEAVE first appear? */
int Sfx86OpcodeExec_enterleave(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if (opcode == 0xC8 && ctx->__private->level >= SX86_CPULEVEL_8086) {	// ENTER
		sx86_uword Size;
		sx86_uword FrameTemp;
		sx86_ubyte NestingLevel;
		int i;

		Size         = softx86_fetch_exec_byte(ctx);
		Size        |= softx86_fetch_exec_byte(ctx)<<8;
		NestingLevel = softx86_fetch_exec_byte(ctx) & 31;

		softx86_stack_pushw(ctx,ctx->state->general_reg[SX86_REG_BP].w.lo);
		FrameTemp = ctx->state->general_reg[SX86_REG_SP].w.lo;

		if (NestingLevel > 0) {
			for (i=1;i < NestingLevel;i++) {
				ctx->state->general_reg[SX86_REG_BP].w.lo -= 2;
				softx86_stack_pushw(ctx,ctx->state->general_reg[SX86_REG_BP].w.lo);
			}

			softx86_stack_pushw(ctx,FrameTemp);
		}

		ctx->state->general_reg[SX86_REG_BP].w.lo = FrameTemp;
		softx86_set_stack_ptr(ctx,ctx->state->segment_reg[SX86_SREG_SS].val,ctx->state->general_reg[SX86_REG_BP].w.lo - Size);

		return 1;
	}
	if (opcode == 0xC9 && ctx->__private->level >= SX86_CPULEVEL_8086) { // LEAVE
		softx86_set_stack_ptr(ctx,ctx->state->segment_reg[SX86_SREG_SS].val,ctx->state->general_reg[SX86_REG_BP].w.lo);
		ctx->state->general_reg[SX86_REG_BP].w.lo = softx86_stack_popw(ctx);

		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_enterleave(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if (opcode == 0xC8 && ctx->__private->level >= SX86_CPULEVEL_8086) {
		sx86_uword popeye;
		sx86_ubyte imm;

		popeye  = softx86_fetch_dec_byte(ctx);
		popeye |= softx86_fetch_dec_byte(ctx)<<8;
		imm     = softx86_fetch_dec_byte(ctx);
		sprintf(buf,"ENTER 0x%04X,0x%02X",popeye,imm);
		return 1;
	}
	if (opcode == 0xC9 && ctx->__private->level >= SX86_CPULEVEL_8086) {
		strcpy(buf,"LEAVE");
		return 1;
	}

	return 0;
}
