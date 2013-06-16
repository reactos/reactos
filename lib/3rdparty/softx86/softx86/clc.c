/*
 * clc.c
 *
 * Copyright (C) 2003, 2004 Jonathan Campbell <jcampbell@mdjk.com>
 *
 * Decompiler and executioneer for CLC/CLD/CLI/CMC/STC/STD/STI instructions.
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
#include "clc.h"
#include "drooling_duh.h"

int Sfx86OpcodeExec_clx(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if (opcode == 0xF5) {						// CMC
		ctx->state->reg_flags.val ^=  SX86_CPUFLAG_CARRY;
		return 1;
	}

	if (opcode == 0xF8) {						// CLC
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;
		return 1;
	}

	if (opcode == 0xF9) {						// STC
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_CARRY;
		return 1;
	}

	if (opcode == 0xFA) {						// CLI
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_INTENABLE;
		return 1;
	}

	if (opcode == 0xFB) {						// STI
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_INTENABLE;
		return 1;
	}

	if (opcode == 0xFC) {						// CLD
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_DIRECTIONREV;
		return 1;
	}

	if (opcode == 0xFD) {						// STD
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_DIRECTIONREV;
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_clx(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if (opcode == 0xF5) {						// CMC
		strcpy(buf,"CMC");
		return 1;
	}

	if (opcode == 0xF8) {						// CLC
		strcpy(buf,"CLC");
		return 1;
	}

	if (opcode == 0xF9) {						// STC
		strcpy(buf,"STC");
		return 1;
	}

	if (opcode == 0xFA) {						// CLI
		strcpy(buf,"CLI");
		return 1;
	}

	if (opcode == 0xFB) {						// STI
		strcpy(buf,"STI");
		return 1;
	}

	if (opcode == 0xFC) {						// CLD
		strcpy(buf,"CLD");
		return 1;
	}

	if (opcode == 0xFD) {						// STD
		strcpy(buf,"STD");
		return 1;
	}

	return 0;
}
