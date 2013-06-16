/*
 * add.c
 *
 * Copyright (C) 2003, 2004 Jonathan Campbell <jcampbell@mdjk.com>
 *
 * Decompiler and executioneer for MOV and XLAT instructions.
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
#include "mov.h"

sx86_ubyte op_mov8(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val)
{
	return val;
}

sx86_uword op_mov16(softx86_ctx* ctx,sx86_uword src,sx86_uword val)
{
	return val;
}

sx86_udword op_mov32(softx86_ctx* ctx,sx86_udword src,sx86_udword val)
{
	return val;
}

int Sfx86OpcodeExec_mov(sx86_ubyte opcode,softx86_ctx* ctx)
{
	sx86_udword lo;
	sx86_uword seg;

	if (!ctx->state->is_segment_override)
		seg = ctx->state->segment_reg[SX86_SREG_DS].val;
	else
		seg = ctx->state->segment_override;

	if ((opcode&0xFC) == 0x88) {					// MOV reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte w16,opswap;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		opswap = (opcode&2)>>1;					// mov from reg to r/m UNLESS this bit it set (then it's the other way around)
		x      = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_exec_full_modregrm_rw(ctx,w16,0,mod,reg,
			rm,opswap,op_mov8,op_mov16,op_mov32);

		return 1;
	}
	else if (opcode == 0x8C || opcode == 0x8E) {			// MOV reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte opswap;
		sx86_ubyte mod,reg,rm;

		opswap = (opcode&2)>>1;					// mov from reg to r/m UNLESS this bit it set (then it's the other way around)
		x      = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_exec_full_modsregrm_rw(ctx,mod,reg,rm,opswap,op_mov16);

		return 1;
	}
	else if ((opcode&0xFE) == 0xA0) {				// MOV AL/AX,[mem]
		sx86_uword o;

		o  = softx86_fetch_exec_byte(ctx);
		o |= softx86_fetch_exec_byte(ctx)<<8;
		lo = sx86_far_to_linear(ctx,seg,o);

		if (opcode&1) {
			sx86_uword mem;

			softx86_fetch(ctx,NULL,lo,&mem,2);
			SWAP_WORD_FROM_LE(mem);
			ctx->state->general_reg[SX86_REG_AX].w.lo = mem;
		}
		else {
			sx86_ubyte mem;

			softx86_fetch(ctx,NULL,lo,&mem,1);
			ctx->state->general_reg[SX86_REG_AX].b.lo = mem;
		}

		return 1;
	}
	else if ((opcode&0xFE) == 0xA2) {				// MOV [mem],AL/AX
		sx86_uword o;

		o  = softx86_fetch_exec_byte(ctx);
		o |= softx86_fetch_exec_byte(ctx)<<8;
		lo = sx86_far_to_linear(ctx,seg,o);

		if (opcode&1) {
			sx86_uword mem;

			mem = ctx->state->general_reg[SX86_REG_AX].w.lo;
			SWAP_WORD_TO_LE(mem);
			softx86_write(ctx,NULL,lo,&mem,2);
		}
		else {
			sx86_ubyte mem;

			mem = ctx->state->general_reg[SX86_REG_AX].b.lo;
			softx86_write(ctx,NULL,lo,&mem,1);
		}

		return 1;
	}
	else if ((opcode&0xF8) == 0xB0) {				// MOV [reg = (opcode-0xB0)],imm8
		sx86_ubyte b;

		b = softx86_fetch_exec_byte(ctx);
		*(ctx->__private->ptr_regs_8reg[opcode-0xB0]) = b;
		return 1;
	}
	else if ((opcode&0xF8) == 0xB8) {				// MOV [reg = (opcode-0xB8)],imm16
		sx86_uword w;

		w  = softx86_fetch_exec_byte(ctx);
		w |= softx86_fetch_exec_byte(ctx)<<8;
		ctx->state->general_reg[opcode-0xB8].w.lo = w;
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_mov(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if ((opcode&0xFC) == 0x88) {					// MOV reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte w16,opswap;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		opswap = (opcode&2)>>1;					// mov from reg to r/m UNLESS this bit it set (then it's the other way around)
		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modregrm(ctx,w16,0,mod,reg,rm,op1_tmp,op2_tmp);

		if (opswap)	sprintf(buf,"MOV %s,%s",op2_tmp,op1_tmp);
		else		sprintf(buf,"MOV %s,%s",op1_tmp,op2_tmp);
		return 1;
	}
	else if (opcode == 0x8C || opcode == 0x8E) {			// MOV reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte opswap;
		sx86_ubyte mod,reg,rm;

		opswap = (opcode&2)>>1;					// mov from reg to r/m UNLESS this bit it set (then it's the other way around)
		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modsregrm(ctx,mod,reg,rm,op1_tmp,op2_tmp);

		if (opswap)	sprintf(buf,"MOV %s,%s",op2_tmp,op1_tmp);
		else		sprintf(buf,"MOV %s,%s",op1_tmp,op2_tmp);
		return 1;
	}
	else if ((opcode&0xFE) == 0xA0) {				// MOV AL/AX,[mem]
		sx86_uword o;

		o  = softx86_fetch_dec_byte(ctx);
		o |= softx86_fetch_dec_byte(ctx)<<8;
		sprintf(buf,"MOV %s,[%04Xh]",opcode == 0xA1 ? "AX" : "AL",o);
		return 1;
	}
	else if ((opcode&0xFE) == 0xA2) {				// MOV [mem],AL/AX
		sx86_uword o;

		o  = softx86_fetch_dec_byte(ctx);
		o |= softx86_fetch_dec_byte(ctx)<<8;
		sprintf(buf,"MOV [%04Xh],%s",o,opcode == 0xA3 ? "AX" : "AL");
		return 1;
	}
	else if ((opcode&0xF8) == 0xB0) {				// MOV [reg = (opcode-0xB0)],imm8
		sx86_ubyte b;

		b = softx86_fetch_dec_byte(ctx);
		sprintf(buf,"MOV %s,%02Xh",sx86_regs8[opcode-0xB0],b);
		return 1;
	}
	else if ((opcode&0xF8) == 0xB8) {				// MOV [reg = (opcode-0xB8)],imm16
		sx86_uword w;

		w  = softx86_fetch_dec_byte(ctx);
		w |= softx86_fetch_dec_byte(ctx)<<8;
		sprintf(buf,"MOV %s,%04Xh",sx86_regs16[opcode-0xB8],w);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeExec_xlat(sx86_ubyte opcode,softx86_ctx* ctx)
{
	sx86_uword seg;

	if (!ctx->state->is_segment_override)
		seg = ctx->state->segment_reg[SX86_SREG_DS].val;
	else
		seg = ctx->state->segment_override;

	if ((opcode&0xFC) == 0xD7) {					// MOV reg,reg/mem or reg/mem,reg
		sx86_ubyte d;
		sx86_udword ofs;

		ofs  = (sx86_udword)(ctx->state->general_reg[SX86_REG_BX].w.lo);
		ofs += (sx86_udword)(ctx->state->general_reg[SX86_REG_AX].b.lo);
		ofs &= 0xFFFF;
		ofs  = sx86_far_to_linear(ctx,seg,ofs);

		softx86_fetch(ctx,NULL,ofs,&d,1);
		ctx->state->general_reg[SX86_REG_AX].b.lo = d;

		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_xlat(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if ((opcode&0xFC) == 0xD7) {					// XLAT
		strcpy(buf,"XLAT");
		return 1;
	}

	return 0;
}

sx86_uword op_les16(softx86_ctx* ctx,sx86_uword seg,sx86_uword ofs)
{
	softx86_setsegval(ctx,SX86_SREG_ES,seg);
	return ofs;
}

sx86_udword op_les32(softx86_ctx* ctx,sx86_udword seg,sx86_udword ofs)
{
	softx86_setsegval(ctx,SX86_SREG_ES,seg);
	return ofs;
}

sx86_uword op_lds16(softx86_ctx* ctx,sx86_uword seg,sx86_uword ofs)
{
	softx86_setsegval(ctx,SX86_SREG_DS,seg);
	return ofs;
}

sx86_udword op_lds32(softx86_ctx* ctx,sx86_udword seg,sx86_udword ofs)
{
	softx86_setsegval(ctx,SX86_SREG_DS,seg);
	return ofs;
}

int Sfx86OpcodeExec_les(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if (opcode == 0xC4) {			// LES
		sx86_ubyte x;
		sx86_ubyte mod,reg,rm;

		x  = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_exec_full_modregrm_far(ctx,0,mod,reg,rm,op_les16,op_les32);
		return 1;
	}
	if (opcode == 0xC5) {			// LDS
		sx86_ubyte x;
		sx86_ubyte mod,reg,rm;

		x  = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_exec_full_modregrm_far(ctx,0,mod,reg,rm,op_lds16,op_lds32);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_les(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if (opcode == 0xC4) {			// LES
		sx86_ubyte x;
		sx86_ubyte mod,reg,rm;

		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modregrm(ctx,1,0,mod,reg,rm,op1_tmp,op2_tmp);
		sprintf(buf,"LES %s,%s",op2_tmp,op1_tmp);
		return 1;
	}
	if (opcode == 0xC5) {			// LDS
		sx86_ubyte x;
		sx86_ubyte mod,reg,rm;

		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modregrm(ctx,1,0,mod,reg,rm,op1_tmp,op2_tmp);
		sprintf(buf,"LDS %s,%s",op2_tmp,op1_tmp);
		return 1;
	}

	return 0;
}

sx86_uword op_lmsw16(softx86_ctx* ctx,sx86_uword src)
{
/* bit 0 can only be set, not reset (80286 method of PM) */
	ctx->state->reg_cr0 |=	(src & SX86_CR0FLAG_PE);
/* bits 1-3 are only used here */
	ctx->state->reg_cr0 &=	~SX86_CR0_LMSW_MASK;
	ctx->state->reg_cr0 |=	(src & SX86_CR0_LMSW_MASK);
/* the return value doesn't matter */
	return 0;
}

sx86_uword op_smsw16(softx86_ctx* ctx,sx86_uword src)
{
	return ((sx86_uword)ctx->state->reg_cr0);
}

int Sfx86OpcodeExec_lmsw286(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if (opcode == 0x01) {						// LMSW [reg]
		sx86_ubyte x;
		sx86_ubyte mod,reg,rm;

		x      = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);

		if (reg == 4)
			sx86_exec_full_modrmonly_rw(ctx,1,0,mod,rm,NULL,op_smsw16,NULL);
		else if (reg == 6)
			sx86_exec_full_modrmonly_ro(ctx,1,0,mod,rm,NULL,op_lmsw16,NULL);
		else
			return 0;

		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_lmsw286(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if (opcode == 0x01) {						// LMSW [reg]
		sx86_ubyte x;
		sx86_ubyte w16;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modrmonly(ctx,w16,0,mod,rm,op1_tmp);

		if (reg == 4)
			sprintf(buf,"SMSW %s",op1_tmp);
		else if (reg == 6)
			sprintf(buf,"LMSW %s",op1_tmp);
		else
			return 0;

		return 1;
	}

	return 0;
}
