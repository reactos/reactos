/*
 * ioport.c
 *
 * Copyright (C) 2003, 2004 Jonathan Campbell <jcampbell@mdjk.com>
 *
 * Decompiler and executioneer for IN/OUT instructions.
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
#include "ioport.h"

int Sfx86OpcodeExec_io(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if ((opcode&0xFE) == 0xE4) {					// IN AL/AX,imm8
		sx86_ubyte ionum;
		sx86_ubyte w16;

/* TODO: If emulating 386+ and protected/v86 mode check I/O permission bitmap */

		w16    = (opcode&1);
		ionum  = softx86_fetch_exec_byte(ctx);			// get imm8

		if (w16) {	/* NOTE: It is always assumed that it is in Little Endian */
			sx86_uword rw;

			ctx->callbacks->on_read_io(ctx,ionum,(sx86_ubyte*)(&rw),2);
			SWAP_WORD_FROM_LE(rw);
			ctx->state->general_reg[SX86_REG_AX].w.lo = rw;
		}
		else {
			sx86_ubyte rb;

			ctx->callbacks->on_read_io(ctx,ionum,(sx86_ubyte*)(&rb),1);
			ctx->state->general_reg[SX86_REG_AX].b.lo = rb;
		}

		return 1;
	}
	else if ((opcode&0xFE) == 0xEC) {				// IN AL/AX,DX
		sx86_ubyte w16;

/* TODO: If emulating 386+ and protected/v86 mode check I/O permission bitmap */

		w16 = (opcode&1);
		if (w16) {	/* NOTE: It is always assumed that it is in Little Endian */
			sx86_uword rw;

			ctx->callbacks->on_read_io(ctx,ctx->state->general_reg[SX86_REG_DX].w.lo,(sx86_ubyte*)(&rw),2);
			SWAP_WORD_FROM_LE(rw);
			ctx->state->general_reg[SX86_REG_AX].w.lo = rw;
		}
		else {
			sx86_ubyte rb;

			ctx->callbacks->on_read_io(ctx,ctx->state->general_reg[SX86_REG_DX].w.lo,(sx86_ubyte*)(&rb),1);
			ctx->state->general_reg[SX86_REG_AX].b.lo = rb;
		}

		return 1;
	}
	else if ((opcode&0xFE) == 0xE6) {				// OUT AL/AX,imm8
		sx86_ubyte ionum;
		sx86_ubyte w16;

/* TODO: If emulating 386+ and protected/v86 mode check I/O permission bitmap */

		w16    = (opcode&1);
		ionum  = softx86_fetch_exec_byte(ctx);			// get imm8

		if (w16) {	/* NOTE: It is always assumed that it is in Little Endian */
			sx86_uword rw;

			rw = ctx->state->general_reg[SX86_REG_AX].w.lo;
			SWAP_WORD_TO_LE(rw);
			ctx->callbacks->on_write_io(ctx,ionum,(sx86_ubyte*)(&rw),2);
		}
		else {
			sx86_ubyte rb;

			rb = ctx->state->general_reg[SX86_REG_AX].b.lo;
			ctx->callbacks->on_write_io(ctx,ionum,(sx86_ubyte*)(&rb),1);
		}

		return 1;
	}
	else if ((opcode&0xFE) == 0xEE) {				// OUT AL/AX,DX
		sx86_ubyte w16;

/* TODO: If emulating 386+ and protected/v86 mode check I/O permission bitmap */

		w16 = (opcode&1);
		if (w16) {	/* NOTE: It is always assumed that it is in Little Endian */
			sx86_uword rw;

			rw = ctx->state->general_reg[SX86_REG_AX].w.lo;
			SWAP_WORD_TO_LE(rw);
			ctx->callbacks->on_write_io(ctx,ctx->state->general_reg[SX86_REG_DX].w.lo,(sx86_ubyte*)(&rw),2);
		}
		else {
			sx86_ubyte rb;

			rb = ctx->state->general_reg[SX86_REG_AX].b.lo;
			ctx->callbacks->on_write_io(ctx,ctx->state->general_reg[SX86_REG_DX].w.lo,(sx86_ubyte*)(&rb),1);
		}

		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_io(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if ((opcode&0xFE) == 0xE4) {					// IN AL/AX,imm8
		sx86_ubyte ionum;
		sx86_ubyte w16;

		w16    = (opcode&1);
		ionum  = softx86_fetch_dec_byte(ctx);			// get imm8

		sprintf(buf,"IN %s,%02Xh",w16 ? "AX" : "AL",ionum);
		return 1;
	}
	else if ((opcode&0xFE) == 0xEC) {				// IN AL/AX,DX
		sx86_ubyte w16;

		w16 = (opcode&1);
		sprintf(buf,"IN %s,DX",w16 ? "AX" : "AL");
		return 1;
	}
	else if ((opcode&0xFE) == 0xE6) {				// OUT AL/AX,imm8
		sx86_ubyte ionum;
		sx86_ubyte w16;

		w16    = (opcode&1);
		ionum  = softx86_fetch_dec_byte(ctx);			// get imm8

		sprintf(buf,"OUT %02Xh,%s",ionum,w16 ? "AX" : "AL");
		return 1;
	}
	else if ((opcode&0xFE) == 0xEE) {				// OUT AL/AX,DX
		sx86_ubyte w16;

		w16 = (opcode&1);
		sprintf(buf,"OUT DX,%s",w16 ? "AX" : "AL");
		return 1;
	}

	return 0;
}
