/*
 * groupies.c
 *
 * Copyright (C) 2003, 2004 Jonathan Campbell <jcampbell@mdjk.com>
 *
 * Decompiler and executioneer for instruction opcodes grouped by a common
 * opcode and specified by the reg portion of the mod/reg/rm byte.
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
#include "groupies.h"
#include "optable.h"
#include "binops.h"
#include "add.h"
#include "mov.h"
#include "inc.h"

sx86_uword op_ncall16(softx86_ctx* ctx,sx86_uword src)
{
	softx86_stack_pushw(ctx,ctx->state->reg_ip);
	softx86_set_near_instruction_ptr(ctx,src);
	return src;
}

sx86_udword op_ncall32(softx86_ctx* ctx,sx86_udword src)
{
	softx86_stack_pushw(ctx,(ctx->state->reg_ip>>16)&0xFFFF);
	softx86_stack_pushw(ctx,ctx->state->reg_ip&0xFFFF);
	softx86_set_near_instruction_ptr(ctx,src);
	return src;
}

sx86_uword op_popmem16(softx86_ctx* ctx)
{
	return softx86_stack_popw(ctx);
}

sx86_udword op_popmem32(softx86_ctx* ctx)
{
	sx86_udword d;

	d  = ((sx86_udword)softx86_stack_popw(ctx));
	d |= ((sx86_udword)softx86_stack_popw(ctx))<<16;
	return d;
}

sx86_uword op_pushmem16(softx86_ctx* ctx,sx86_uword src)
{
	softx86_stack_pushw(ctx,src);
	return src;
}

sx86_udword op_pushmem32(softx86_ctx* ctx,sx86_udword src)
{
	softx86_stack_pushw(ctx,(src>>16)&0xFFFF);
	softx86_stack_pushw(ctx,src&0xFFFF);
	return src;
}

sx86_uword op_njmp16(softx86_ctx* ctx,sx86_uword src)
{
	softx86_set_near_instruction_ptr(ctx,src);
	return src;
}

sx86_udword op_njmp32(softx86_ctx* ctx,sx86_udword src)
{
	softx86_set_near_instruction_ptr(ctx,src);
	return src;
}

void op_fcall16(softx86_ctx* ctx,sx86_uword seg,sx86_uword ofs)
{
	softx86_stack_pushw(ctx,ctx->state->segment_reg[SX86_SREG_CS].val);
	softx86_stack_pushw(ctx,ctx->state->reg_ip);
	softx86_set_instruction_ptr(ctx,seg,ofs);
}

void op_fcall32(softx86_ctx* ctx,sx86_udword seg,sx86_udword ofs)
{
	softx86_stack_pushw(ctx,ctx->state->segment_reg[SX86_SREG_CS].val);
	softx86_stack_pushw(ctx,ctx->state->reg_ip&0xFFFF);
	softx86_stack_pushw(ctx,(ctx->state->reg_ip>>16)&0xFFFF);
	softx86_set_instruction_ptr(ctx,seg,ofs);
}

void op_fjmp16(softx86_ctx* ctx,sx86_uword seg,sx86_uword ofs)
{
	softx86_set_instruction_ptr(ctx,seg,ofs);
}

void op_fjmp32(softx86_ctx* ctx,sx86_udword seg,sx86_udword ofs)
{
	softx86_set_instruction_ptr(ctx,seg,ofs);
}

int Sfx86OpcodeExec_group80(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if ((opcode&0xFC) == 0x80) {
		sx86_ubyte x;
		sx86_ubyte w16,sx;
		sx86_ubyte mod,reg,rm;
		sx86_ubyte (*op8)(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val);
		sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword src,sx86_uword val);
		sx86_udword (*op32)(softx86_ctx* ctx,sx86_udword src,sx86_udword val);

		w16    = opcode&1;
		sx     = (opcode>>1)&1;
		x      = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);

		if (reg == 0) {
			op8  = op_add8;
			op16 = op_add16;
			op32 = op_add32;
		}
		else if (reg == 1) {
			op8  = op_or8;
			op16 = op_or16;
			op32 = op_or32;
		}
		else if (reg == 2) {
			op8  = op_adc8;
			op16 = op_adc16;
			op32 = op_adc32;
		}
		else if (reg == 3) {
			op8  = op_sbb8;
			op16 = op_sbb16;
			op32 = op_sbb32;
		}
		else if (reg == 4) {
			op8  = op_and8;
			op16 = op_and16;
			op32 = op_and32;
		}
		else if (reg == 5) {
			op8  = op_sub8;
			op16 = op_sub16;
			op32 = op_sub32;
		}
		else if (reg == 6) {
			op8  = op_xor8;
			op16 = op_xor16;
			op32 = op_xor32;
		}
		else if (reg == 7) {
			op8  = op_sub8;		/* CMP is like SUB except that the result */
			op16 = op_sub16;	/* ends up in a temporary register, leaving */
			op32 = op_sub32;	/* the original data untouched. */
		}
		else {
			return 0;
		}

		if (reg == 7)			/* CMP doesn't modify the dest. register... */
			sx86_exec_full_modrmonly_ro_imm(ctx,w16,0,mod,rm,op8,op16,op32,sx);
		else
			sx86_exec_full_modrmonly_rw_imm(ctx,w16,0,mod,rm,op8,op16,op32,sx);

		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_group80(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if ((opcode&0xFC) == 0x80) {
		sx86_ubyte x;
		sx86_ubyte w16,sx;
		sx86_ubyte mod,reg,rm;
		sx86_uword imm16;

		w16    = opcode&1;
		sx     = (opcode>>1)&1;
		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modrmonly(ctx,w16,0,mod,rm,op1_tmp);

		imm16 = softx86_fetch_dec_byte(ctx);
		if (w16)
			if (sx)
				imm16 |= (imm16&0x80) ? 0xFF80 : 0;
			else
				imm16 |= softx86_fetch_dec_byte(ctx)<<8;

		if (reg == 0)		sprintf(buf,"ADD %s,%04Xh",op1_tmp,imm16);
		else if (reg == 1)	sprintf(buf,"OR %s,%04Xh",op1_tmp,imm16);
		else if (reg == 2)	sprintf(buf,"ADC %s,%04Xh",op1_tmp,imm16);
		else if (reg == 3)	sprintf(buf,"SBB %s,%04Xh",op1_tmp,imm16);
		else if (reg == 4)	sprintf(buf,"AND %s,%04Xh",op1_tmp,imm16);
		else if (reg == 5)	sprintf(buf,"SUB %s,%04Xh",op1_tmp,imm16);
		else if (reg == 6)	sprintf(buf,"XOR %s,%04Xh",op1_tmp,imm16);
		else if (reg == 7)	sprintf(buf,"CMP %s,%04Xh",op1_tmp,imm16);
		else			return 0;

		return 1;
	}

	return 0;
}

int Sfx86OpcodeExec_groupC0(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if ((opcode&0xFE) == 0xC0) {
		sx86_ubyte x;
		sx86_ubyte w16;
		sx86_ubyte mod,reg,rm;
		sx86_ubyte (*op8)(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val);
		sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword src,sx86_uword val);
		sx86_udword (*op32)(softx86_ctx* ctx,sx86_udword src,sx86_udword val);

		w16    = (opcode&1);
		x      = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);

		if (reg == 0) {
			op8  = op_rol8;
			op16 = op_rol16;
			op32 = op_rol32;
		}
		else if (reg == 1) {
			op8  = op_ror8;
			op16 = op_ror16;
			op32 = op_ror32;
		}
		else if (reg == 2) {
			op8  = op_rcl8;
			op16 = op_rcl16;
			op32 = op_rcl32;
		}
		else if (reg == 3) {
			op8  = op_rcr8;
			op16 = op_rcr16;
			op32 = op_rcr32;
		}
		else if (reg == 4) {
			op8  = op_shl8;
			op16 = op_shl16;
			op32 = op_shl32;
		}
		else if (reg == 5) {
			op8  = op_shr8;
			op16 = op_shr16;
			op32 = op_shr32;
		}
		else if (reg == 7) {
			op8  = op_sar8;
			op16 = op_sar16;
			op32 = op_sar32;
		}
		else {
			return 0;
		}

		sx86_exec_full_modrmonly_rw_imm8(ctx,w16,0,mod,rm,op8,op16,op32);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_groupC0(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if ((opcode&0xFE) == 0xC0) {
		sx86_ubyte x;
		sx86_ubyte w16;
		sx86_ubyte mod,reg,rm;
		sx86_uword imm16;

		w16    = (opcode&1);
		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modrmonly(ctx,w16,0,mod,rm,op1_tmp);

		imm16 = softx86_fetch_dec_byte(ctx);

		if (reg == 0)		sprintf(buf,"ROL %s,%04Xh",op1_tmp,imm16);
		else if (reg == 1)	sprintf(buf,"ROR %s,%04Xh",op1_tmp,imm16);
		else if (reg == 2)	sprintf(buf,"RCL %s,%04Xh",op1_tmp,imm16);
		else if (reg == 3)	sprintf(buf,"RCR %s,%04Xh",op1_tmp,imm16);
		else if (reg == 4)	sprintf(buf,"SHL %s,%04Xh",op1_tmp,imm16);
		else if (reg == 5)	sprintf(buf,"SHR %s,%04Xh",op1_tmp,imm16);
		else if (reg == 7)	sprintf(buf,"SAR %s,%04Xh",op1_tmp,imm16);
		else			return 0;

		return 1;
	}

	return 0;
}

int Sfx86OpcodeExec_groupD0(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if ((opcode&0xFE) == 0xD0 || (opcode&0xFE) == 0xD2) {
		sx86_ubyte x;
		sx86_ubyte w16;
		sx86_ubyte mod,reg,rm;
		sx86_ubyte (*op8)(softx86_ctx* ctx,sx86_ubyte src);
		sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword src);
		sx86_udword (*op32)(softx86_ctx* ctx,sx86_udword src);

		w16    = (opcode&1);
		x      = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);

		if (reg == 0) {
			if ((opcode&0xFE) == 0xD2) {
				op8  = op_rol_cl_8;
				op16 = op_rol_cl_16;
				op32 = op_rol_cl_32;
			}
			else {
				op8  = op_rol1_8;
				op16 = op_rol1_16;
				op32 = op_rol1_32;
			}
		}
		else if (reg == 1) {
			if ((opcode&0xFE) == 0xD2) {
				op8  = op_ror_cl_8;
				op16 = op_ror_cl_16;
				op32 = op_ror_cl_32;
			}
			else {
				op8  = op_ror1_8;
				op16 = op_ror1_16;
				op32 = op_ror1_32;
			}
		}
		else if (reg == 2) {
			if ((opcode&0xFE) == 0xD2) {
				op8  = op_rcl_cl_8;
				op16 = op_rcl_cl_16;
				op32 = op_rcl_cl_32;
			}
			else {
				op8  = op_rcl1_8;
				op16 = op_rcl1_16;
				op32 = op_rcl1_32;
			}
		}
		else if (reg == 3) {
			if ((opcode&0xFE) == 0xD2) {
				op8  = op_rcr_cl_8;
				op16 = op_rcr_cl_16;
				op32 = op_rcr_cl_32;
			}
			else {
				op8  = op_rcr1_8;
				op16 = op_rcr1_16;
				op32 = op_rcr1_32;
			}
		}
		else if (reg == 4) {
			if ((opcode&0xFE) == 0xD2) {
				op8  = op_shl_cl_8;
				op16 = op_shl_cl_16;
				op32 = op_shl_cl_32;
			}
			else {
				op8  = op_shl1_8;
				op16 = op_shl1_16;
				op32 = op_shl1_32;
			}
		}
		else if (reg == 5) {
			if ((opcode&0xFE) == 0xD2) {
				op8  = op_shr_cl_8;
				op16 = op_shr_cl_16;
				op32 = op_shr_cl_32;
			}
			else {
				op8  = op_shr1_8;
				op16 = op_shr1_16;
				op32 = op_shr1_32;
			}
		}
		else if (reg == 7) {
			if ((opcode&0xFE) == 0xD2) {
				op8  = op_sar_cl_8;
				op16 = op_sar_cl_16;
				op32 = op_sar_cl_32;
			}
			else {
				op8  = op_sar1_8;
				op16 = op_sar1_16;
				op32 = op_sar1_32;
			}
		}
		else {
			return 0;
		}

		sx86_exec_full_modrmonly_rw(ctx,w16,0,mod,rm,op8,op16,op32);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_groupD0(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if ((opcode&0xFE) == 0xD0 || (opcode&0xFE) == 0xD2) {
		sx86_ubyte x;
		sx86_ubyte w16;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modrmonly(ctx,w16,0,mod,rm,op1_tmp);

		if ((opcode&0xFE) == 0xD2) {
			if (reg == 0)		sprintf(buf,"ROL %s,CL",op1_tmp);
			else if (reg == 1)	sprintf(buf,"ROR %s,CL",op1_tmp);
			else if (reg == 2)	sprintf(buf,"RCL %s,CL",op1_tmp);
			else if (reg == 3)	sprintf(buf,"RCR %s,CL",op1_tmp);
			else if (reg == 4)	sprintf(buf,"SHL %s,CL",op1_tmp);
			else if (reg == 5)	sprintf(buf,"SHR %s,CL",op1_tmp);
			else if (reg == 7)	sprintf(buf,"SAR %s,CL",op1_tmp);
			else			return 0;
		}
		else {
			if (reg == 0)		sprintf(buf,"ROL %s,1",op1_tmp);
			else if (reg == 1)	sprintf(buf,"ROR %s,1",op1_tmp);
			else if (reg == 2)	sprintf(buf,"RCL %s,1",op1_tmp);
			else if (reg == 3)	sprintf(buf,"RCR %s,1",op1_tmp);
			else if (reg == 4)	sprintf(buf,"SHL %s,1",op1_tmp);
			else if (reg == 5)	sprintf(buf,"SHR %s,1",op1_tmp);
			else if (reg == 7)	sprintf(buf,"SAR %s,1",op1_tmp);
			else			return 0;
		}

		return 1;
	}

	return 0;
}

int Sfx86OpcodeExec_groupC6(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if ((opcode&0xFE) == 0xC6) {
		sx86_ubyte x;
		sx86_ubyte w16;
		sx86_ubyte mod,reg,rm;
		sx86_ubyte (*op8)(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val);
		sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword src,sx86_uword val);
		sx86_udword (*op32)(softx86_ctx* ctx,sx86_udword src,sx86_udword val);

		w16    = (opcode&1);
		x      = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);

		if (reg == 0) {
			op8  = op_mov8;
			op16 = op_mov16;
			op32 = op_mov32;
		}
		else {
			return 0;
		}

		sx86_exec_full_modrmonly_rw_imm(ctx,w16,0,mod,rm,op8,op16,op32,0);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_groupC6(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if ((opcode&0xFE) == 0xC6) {
		sx86_ubyte x;
		sx86_ubyte w16;
		sx86_ubyte mod,reg,rm;
		sx86_uword imm16;

		w16    = (opcode&1);
		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modrmonly(ctx,w16,0,mod,rm,op1_tmp);

		imm16 = softx86_fetch_dec_byte(ctx);
		if (w16) imm16 |= softx86_fetch_dec_byte(ctx)<<8;

		if (reg == 0)		sprintf(buf,"MOV %s,%04Xh",op1_tmp,imm16);
		else			return 0;

		return 1;
	}

	return 0;
}

int Sfx86OpcodeExec_groupFE(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if ((opcode&0xFE) == 0xFE) {
		sx86_ubyte x;
		sx86_ubyte w16;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		x      = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);

		if (reg == 0) {
			sx86_exec_full_modrmonly_rw(ctx,w16,0,mod,rm,op_inc8,op_inc16,op_inc32);
		}
		else if (reg == 1) {
			sx86_exec_full_modrmonly_rw(ctx,w16,0,mod,rm,op_dec8,op_dec16,op_dec32);
		}
		else if (reg >= 2) {
			if (!w16) return 0;

			if (reg == 3 || reg == 5) {
				void (*op16)(softx86_ctx* ctx,sx86_uword seg,sx86_uword ofs);
				void (*op32)(softx86_ctx* ctx,sx86_udword seg,sx86_udword ofs);

				/* you cannot have a far address in registers */
				if (mod == 3)
					return 0;

				if (reg == 3) {
					op16 = op_fcall16;
					op32 = op_fcall32;
				}
				else if (reg == 5) {
					op16 = op_fjmp16;
					op32 = op_fjmp32;
				}

				sx86_exec_full_modrmonly_callfar(ctx,0,mod,rm,op16,op32);
			}
			else {
				sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword src);
				sx86_udword (*op32)(softx86_ctx* ctx,sx86_udword src);

				if (reg == 2) {
					op16 = op_ncall16;
					op32 = op_ncall32;
				}
				else if (reg == 4) {
					op16 = op_njmp16;
					op32 = op_njmp32;
				}
				else if (reg == 6) {
					op16 = op_pushmem16;
					op32 = op_pushmem32;
				}
				else {
					return 0;
				}

				sx86_exec_full_modrmonly_ro(ctx,1,0,mod,rm,NULL,op16,op32);
			}
		}

		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_groupFE(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if ((opcode&0xFE) == 0xFE) {
		sx86_ubyte x;
		sx86_ubyte w16;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modrmonly(ctx,w16,0,mod,rm,op1_tmp);

		if (reg == 0)		sprintf(buf,"INC %s",op1_tmp);
		else if (reg == 1)	sprintf(buf,"DEC %s",op1_tmp);
		else {
			if (!w16)
				return 0;
			if (reg == 2)
				sprintf(buf,"CALL %s",op1_tmp);
			else if (reg == 3)
				sprintf(buf,"CALL FAR %s",op1_tmp);
			else if (reg == 4)
				sprintf(buf,"JMP %s",op1_tmp);
			else if (reg == 5)
				sprintf(buf,"JMP FAR %s",op1_tmp);
			else if (reg == 6)
				sprintf(buf,"PUSH %s",op1_tmp);
			else
				return 0;
		}

		return 1;
	}

	return 0;
}

int Sfx86OpcodeExec_group8F(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if (opcode == 0x8F) {
		sx86_ubyte x,mod,reg,rm;
		sx86_uword (*op16)(softx86_ctx* ctx);
		sx86_udword (*op32)(softx86_ctx* ctx);

		x = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);

		if (reg == 0) {
			op16 = op_popmem16;
			op32 = op_popmem32;
		}
		else {
			return 0;
		}

		sx86_exec_full_modrmonly_wo(ctx,1,0,mod,rm,NULL,op16,op32);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_group8F(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if (opcode == 0x8F) {
		sx86_ubyte x;
		sx86_ubyte mod,reg,rm;

		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modrmonly(ctx,1,0,mod,rm,op1_tmp);

		if (reg == 0)		sprintf(buf,"POP %s",op1_tmp);
		else			return 0;

		return 1;
	}

	return 0;
}

int Sfx86OpcodeExec_groupF6(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if ((opcode&0xFE) == 0xF6) {
		sx86_ubyte x;
		sx86_ubyte w16;
		sx86_ubyte mod,reg,rm;
		sx86_ubyte (*op8)(softx86_ctx* ctx,sx86_ubyte src);
		sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword src);
		sx86_udword (*op32)(softx86_ctx* ctx,sx86_udword src);

		w16    = (opcode&1);
		x      = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);

		if (reg == 0) {
			/* taken care of later */
		}
		else if (reg == 2) {
			/* taken care of later */
		}
		else if (reg == 3) {
			/* taken care of later */
		}
		else if (reg == 4) {
			op8  = op_mul8;
			op16 = op_mul16;
			op32 = op_mul32;
		}
		else if (reg == 5) {
			op8  = op_imul8;
			op16 = op_imul16;
			op32 = op_imul32;
		}
		else if (reg == 6) {
			op8  = op_div8;
			op16 = op_div16;
			op32 = op_div32;
		}
		else if (reg == 7) {
			op8  = op_idiv8;	/* CMP is like SUB except that the result */
			op16 = op_idiv16;	/* ends up in a temporary register, leaving */
			op32 = op_idiv32;	/* the original data untouched. */
		}
		else {
			return 0;
		}

/* TEST does not modify the dest. register... */
		if (reg == 0)
			sx86_exec_full_modrmonly_ro_imm(ctx,w16,0,mod,rm,op_test8,op_test16,op_test32,0);
/* NOT and NEG only have one operand */
		else if (reg == 2)
			sx86_exec_full_modrmonly_rw(ctx,w16,0,mod,rm,op_not8,op_not16,op_not32);
		else if (reg == 3)
			sx86_exec_full_modrmonly_rw(ctx,w16,0,mod,rm,op_neg8,op_neg16,op_neg32);
/* everybody else (MUL/IMUL/DIV/IDIV) has one operand but modifies (E)AX */
		else
			sx86_exec_full_modrmonly_ro(ctx,w16,0,mod,rm,op8,op16,op32);

		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_groupF6(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if ((opcode&0xFE) == 0xF6) {
		sx86_ubyte x;
		sx86_ubyte w16;
		sx86_ubyte mod,reg,rm;
		sx86_uword imm16;

		w16    = (opcode&1);
		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modrmonly(ctx,w16,0,mod,rm,op1_tmp);

		if (reg == 0) {
			imm16 = softx86_fetch_dec_byte(ctx);
			if (w16) imm16 |= softx86_fetch_dec_byte(ctx)<<8;
		}

		if (reg == 0)		sprintf(buf,"TEST %s,%04Xh",op1_tmp,imm16);
		else if (reg == 2)	sprintf(buf,"NOT %s",op1_tmp);
		else if (reg == 3)	sprintf(buf,"NEG %s",op1_tmp);
		else if (reg == 4)	sprintf(buf,"MUL %s",op1_tmp);
		else if (reg == 5)	sprintf(buf,"IMUL %s",op1_tmp);
		else if (reg == 6)	sprintf(buf,"DIV %s",op1_tmp);
		else if (reg == 7)	sprintf(buf,"IDIV %s",op1_tmp);
		else			return 0;

		return 1;
	}

	return 0;
}

