/*
 * cbw.c
 *
 * Copyright (C) 2003, 2004 Jonathan Campbell <jcampbell@mdjk.com>
 *
 * Decompiler and executioneer for CBW/CWDE/CWD/CDQ instructions.
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
#include "cbw.h"

int Sfx86OpcodeExec_cxex(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if (opcode == 0x98) {						// CBW/CWDE
		if (ctx->state->general_reg[SX86_REG_AX].b.lo & 0x80)	ctx->state->general_reg[SX86_REG_AX].w.lo |= 0xFF00;
		else							ctx->state->general_reg[SX86_REG_AX].b.hi  = 0x00;

		return 1;
	}

	if (opcode == 0x99) {						// CWD/CDQ
		if (ctx->state->general_reg[SX86_REG_AX].w.lo & 0x8000)	ctx->state->general_reg[SX86_REG_DX].w.lo = 0xFFFF;
		else							ctx->state->general_reg[SX86_REG_DX].w.lo = 0x0000;

		return 1;
	}


	return 0;
}

int Sfx86OpcodeDec_cxex(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if (opcode == 0x98) {						// CBW/CWDE
		strcpy(buf,"CBW");
		return 1;
	}

	if (opcode == 0x99) {						// CWD/CDQ
		strcpy(buf,"CWD");
		return 1;
	}

	return 0;
}
