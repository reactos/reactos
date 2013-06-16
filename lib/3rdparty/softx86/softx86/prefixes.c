/*
 * prefixes.c
 *
 * Copyright (C) 2003, 2004 Jonathan Campbell <jcampbell@mdjk.com>
 *
 * Decompiler and executioneer for segment override prefixes.
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
#include "prefixes.h"

int Sfx86OpcodeExec_segover(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if (opcode == 0x26) {		// ES:
		ctx->state->is_segment_override = 1;
		ctx->state->segment_override = ctx->state->segment_reg[SX86_SREG_ES].val;
		return 2;
	}
	if (opcode == 0x2E) {		// CS:
		ctx->state->is_segment_override = 1;
		ctx->state->segment_override = ctx->state->segment_reg[SX86_SREG_CS].val;
		return 2;
	}
	if (opcode == 0x36) {		// SS:
		ctx->state->is_segment_override = 1;
		ctx->state->segment_override = ctx->state->segment_reg[SX86_SREG_SS].val;
		return 2;
	}
	if (opcode == 0x3E) {		// DS:
		ctx->state->is_segment_override = 1;
		ctx->state->segment_override = ctx->state->segment_reg[SX86_SREG_DS].val;
		return 2;
	}

	return 0;
}

int Sfx86OpcodeDec_segover(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if (opcode == 0x26) {		// ES:
		strcpy(buf,"ES: ");
		return 2;
	}
	if (opcode == 0x2E) {		// CS:
		strcpy(buf,"CS: ");
		return 2;
	}
	if (opcode == 0x36) {		// SS:
		strcpy(buf,"SS: ");
		return 2;
	}
	if (opcode == 0x3E) {		// DS:
		strcpy(buf,"DS: ");
		return 2;
	}

	return 0;
}

int Sfx86OpcodeExec_repetition(sx86_ubyte opcode,softx86_ctx* ctx)
{
// you can't have REP REP MOSVB...
// it's not like "ICE ICE BABY"
	if (ctx->state->rep_flag > 0)
		return 0;

	if (opcode == 0xF3) {		// REP/REPZ
		ctx->state->rep_flag=2;
		return 2;
	}
	if (opcode == 0xF2) {		// REPNZ
		ctx->state->rep_flag=1;
		return 2;
	}

	return 0;
}

int Sfx86OpcodeDec_repetition(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if (opcode == 0xF3) {		// REP/REPZ
		strcpy(buf,"REP ");
		return 2;
	}
	if (opcode == 0xF2) {		// REPNZ
		strcpy(buf,"REPNZ ");
		return 2;
	}

	return 0;
}

int Sfx86OpcodeExec_lock(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if (opcode == 0xF0) {		// LOCK
		/* TODO: Do something with this... */
		return 2;
	}

	return 0;
}

int Sfx86OpcodeDec_lock(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if (opcode == 0xF0) {		// LOCK
		strcpy(buf,"LOCK ");
		return 2;
	}

	return 0;
}
