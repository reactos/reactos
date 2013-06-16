/*
 * shovel.c
 *
 * Copyright (C) 2003, 2004 Jonathan Campbell <jcampbell@mdjk.com>
 *
 * Decompiler and executioneer for MOVS, LODS, CMPS, SCAS, and STOS.
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
#include "shovel.h"

/* references to add.c */
sx86_ubyte op_sub8(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val);
sx86_uword op_sub16(softx86_ctx* ctx,sx86_uword src,sx86_uword val);

int Sfx86OpcodeExec_shovel(sx86_ubyte opcode,softx86_ctx* ctx)
{
	int sz,ecx_terminal,zflag_terminal;
	int inc_esi,inc_edi,sto_edi,get_edi,df;
	sx86_udword si,di,addr;
	sx86_ubyte tmp8;
	sx86_uword tmp16;
	sx86_ubyte tmp8dst;
	sx86_uword tmp16dst;
	sx86_uword seg;

	if ((opcode&0xFE) == 0xA4) {		// MOVS
		if (opcode&1)		sz = 2;
		else			sz = 1;
		inc_esi = 1;
		inc_edi = 1;
		sto_edi = 1;
		get_edi = 0;
	}
	else if ((opcode&0xFE) == 0xA6) {	// CMPS
		if (opcode&1)		sz = 2;
		else			sz = 1;
		inc_esi = 1;
		inc_edi = 1;
		sto_edi = 0;
		get_edi = 1;			/* CMPS compares ds:si and es:di */
	}
	else if ((opcode&0xFE) == 0xAA) {	// STOS
		if (opcode&1)		sz = 2;
		else			sz = 1;
		inc_esi = 0;
		inc_edi = 1;
		sto_edi = 1;
		get_edi = 0;
	}
	else if ((opcode&0xFE) == 0xAC) {	// LODS
		if (opcode&1)		sz = 2;
		else			sz = 1;
		inc_esi = 1;
		inc_edi = 0;
		sto_edi = 0;
		get_edi = 0;
	}
	else if ((opcode&0xFE) == 0xAE) {	// SCAS
		if (opcode&1)		sz = 2;
		else			sz = 1;
		inc_esi = 0;
		inc_edi = 1;
		sto_edi = 0;
		get_edi = 1;
	}
	else {
		return 0;
	}

/* TODO: If 32-bit address override get full 32 bits */
	si = (sx86_udword)ctx->state->general_reg[SX86_REG_SI].w.lo;
	di = (sx86_udword)ctx->state->general_reg[SX86_REG_DI].w.lo;

/* [E]CX == 0 is a terminal condition if REP was encountered */
/* the ZF flag can also be a terminal condition, depending on the opcode */
	if (ctx->state->rep_flag > 0) {
		/* TODO: If 32-bit override present, use ECX not CX */
		ecx_terminal = (ctx->state->general_reg[SX86_REG_CX].w.lo == 0);

/* some instructions don't take the ZF flag into account */
		if ((opcode&0xFE) == 0xA4)	/* MOVS */
			zflag_terminal = 0;
		else if ((opcode&0xFE) == 0xAA)	/* STOS */
			zflag_terminal = 0;
		else if ((opcode&0xFE) == 0xAC)	/* LODS */
			zflag_terminal = 0;
		else
			zflag_terminal = 0;
	}
	else {
		ecx_terminal   = 0;
		zflag_terminal = 0;
	}

/* are we in a REP loop terminal condition? */
	if (ecx_terminal)	return 1;
	if (zflag_terminal)	return 1;

/* get direction flag */
	df = (ctx->state->reg_flags.val & SX86_CPUFLAG_DIRECTIONREV);

/* fetch from src */
	if (inc_esi) {
		if (ctx->state->is_segment_override)
			seg = ctx->state->segment_override;
		else
			seg = ctx->state->segment_reg[SX86_SREG_DS].val;

		addr = sx86_far_to_linear(ctx,seg,si);
		if (sz == 2) {
			tmp16=0;
			softx86_fetch(ctx,NULL,addr,&tmp16,2);
			SWAP_WORD_FROM_LE(tmp16);
			if (df)		si -= 2;
			else		si += 2;
		}
		else if (sz == 1) {
			tmp8=0;
			softx86_fetch(ctx,NULL,addr,&tmp8,1);
			if (df)		si--;
			else		si++;
		}
	}

/* MOVS? */
	if ((opcode&0xFE) == 0xA4) {
		tmp16dst = tmp16;
		tmp8dst  = tmp8;
	}
/* LODSB? */
	else if (opcode == 0xAC) {
		ctx->state->general_reg[SX86_REG_AX].b.lo = tmp8;
	}
/* LODSW? */
	else if (opcode == 0xAD) {
/* TODO: 32-bit data override means LODSD not LODSW */
		ctx->state->general_reg[SX86_REG_AX].w.lo = tmp16;
	}
/* STOSB? */
	else if (opcode == 0xAA) {
		tmp8dst = ctx->state->general_reg[SX86_REG_AX].b.lo;
	}
/* STOSW? */
	else if (opcode == 0xAB) {
/* TODO: 32-bit data override means LODSD not LODSW */
		tmp16dst = ctx->state->general_reg[SX86_REG_AX].w.lo;
	}

/* place in dst */
	if (inc_edi) {
		if (ctx->state->is_segment_override)
			seg = ctx->state->segment_override;
		else
			seg = ctx->state->segment_reg[SX86_SREG_ES].val;

		addr = sx86_far_to_linear(ctx,seg,di);
		if (get_edi) {
			if (sz == 2) {
				tmp16dst=0;
				softx86_fetch(ctx,NULL,addr,&tmp16dst,2);
				SWAP_WORD_FROM_LE(tmp16dst);
			}
			else if (sz == 1) {
				tmp8dst=0;
				softx86_fetch(ctx,NULL,addr,&tmp8dst,1);
			}
		}
		else if (sto_edi) {
			if (sz == 2) {
				SWAP_WORD_TO_LE(tmp16dst);
				softx86_write(ctx,NULL,addr,&tmp16dst,2);
			}
			else if (sz == 1) {
				softx86_write(ctx,NULL,addr,&tmp8dst,1);
			}
		}

		if (df) {
			if (sz == 2)
				di -= 2;
			else if (sz == 1)
				di--;
		}
		else {
			if (sz == 2)
				di += 2;
			else if (sz == 1)
				di++;
		}
	}

/* CMPSB? */
	if (opcode == 0xA6) {
		/* functions like this: */
		/* tmp8    = ds:si      */
		/* tmp8dst = es:di      */
		/* cmp tmp8,tmp8dst     */
		op_sub8(ctx,tmp8,tmp8dst);

/* update terminal condition now that ZF changed */
		if (ctx->state->rep_flag == 1)
			zflag_terminal =  (ctx->state->reg_flags.val & SX86_CPUFLAG_ZERO);
		else if (ctx->state->rep_flag == 2)
			zflag_terminal = !(ctx->state->reg_flags.val & SX86_CPUFLAG_ZERO);
	}
/* CMPSW? */
	else if (opcode == 0xA7) {
		/* functions like this: */
		/* tmp16    = ds:si     */
		/* tmp16dst = es:di     */
		/* cmp tmp16,tmp16dst   */
		op_sub16(ctx,tmp16,tmp16dst);

/* update terminal condition now that ZF changed */
		if (ctx->state->rep_flag == 1)
			zflag_terminal =  (ctx->state->reg_flags.val & SX86_CPUFLAG_ZERO);
		else if (ctx->state->rep_flag == 2)
			zflag_terminal = !(ctx->state->reg_flags.val & SX86_CPUFLAG_ZERO);
	}
/* SCASB? */
	else if (opcode == 0xAE) {
		/* functions like this: */
		/* tmp8    = AL         */
		/* tmp8dst = es:di      */
		/* cmp tmp8,tmp8dst     */
		op_sub8(ctx,ctx->state->general_reg[SX86_REG_AX].b.lo,tmp8dst);

/* update terminal condition now that ZF changed */
		if (ctx->state->rep_flag == 1)
			zflag_terminal =  (ctx->state->reg_flags.val & SX86_CPUFLAG_ZERO);
		else if (ctx->state->rep_flag == 2)
			zflag_terminal = !(ctx->state->reg_flags.val & SX86_CPUFLAG_ZERO);
	}
/* SCASW? */
	else if (opcode == 0xAF) {
		/* functions like this: */
		/* tmp16    = AL        */
		/* tmp16dst = es:di     */
		/* cmp tmp16,tmp16dst   */
		op_sub16(ctx,ctx->state->general_reg[SX86_REG_AX].w.lo,tmp16dst);

/* update terminal condition now that ZF changed */
		if (ctx->state->rep_flag == 1)
			zflag_terminal =  (ctx->state->reg_flags.val & SX86_CPUFLAG_ZERO);
		else if (ctx->state->rep_flag == 2)
			zflag_terminal = !(ctx->state->reg_flags.val & SX86_CPUFLAG_ZERO);
	}

/* TODO: If 32-bit address override store full 32 bits */
	ctx->state->general_reg[SX86_REG_SI].w.lo = (sx86_uword)si;
	ctx->state->general_reg[SX86_REG_DI].w.lo = (sx86_uword)di;

/* TODO: If 32-bit data override decrement full 32 bits */
	if (ctx->state->rep_flag > 0)
		ctx->state->general_reg[SX86_REG_CX].w.lo--;	/* decrement [E]CX */

/* keep the REP loop going */
	if (ctx->state->rep_flag > 0) {
/* why execute this instruction again when we KNOW that [E]CX == 0? */
		/* TODO: If 32-bit override present, use ECX not CX */
		ecx_terminal = (ctx->state->general_reg[SX86_REG_CX].w.lo == 0);

		if (ecx_terminal)		return 1;
		else if (zflag_terminal)	return 1;
		else				return 3;
	}

	return 1;
}

int Sfx86OpcodeDec_shovel(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if (opcode == 0xA4) {		// MOVSB
		strcpy(buf,"MOVSB ");
		return 1;
	}
	if (opcode == 0xA5) {		// MOVSW
		strcpy(buf,"MOVSW ");
		return 1;
	}
	if (opcode == 0xA6) {		// CMPSB
		strcpy(buf,"CMPSB ");
		return 1;
	}
	if (opcode == 0xA7) {		// CMPSW
		strcpy(buf,"CMPSW ");
		return 1;
	}
	if (opcode == 0xAA) {		// STOSB
		strcpy(buf,"STOSB ");
		return 1;
	}
	if (opcode == 0xAB) {		// STOSW
		strcpy(buf,"STOSW ");
		return 1;
	}
	if (opcode == 0xAC) {		// LODSB
		strcpy(buf,"LODSB ");
		return 1;
	}
	if (opcode == 0xAD) {		// LODSW
		strcpy(buf,"LODSW ");
		return 1;
	}
	if (opcode == 0xAE) {		// SCASB
		strcpy(buf,"SCASB ");
		return 1;
	}
	if (opcode == 0xAF) {		// SCASW
		strcpy(buf,"SCASW ");
		return 1;
	}

	return 0;
}
