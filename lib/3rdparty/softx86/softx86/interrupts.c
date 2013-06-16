/*
 * interrupts.c
 *
 * Copyright (C) 2003, 2004 Jonathan Campbell <jcampbell@mdjk.com>
 *
 * Decompiler and executioneer for the INT instruction.
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
#include "interrupts.h"

int Sfx86OpcodeExec_int(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if (opcode == 0xCC) {					// INT 3
		softx86_int_sw_signal(ctx,3);
		return 1;
	}
	else if (opcode == 0xCD) {				// INT n
		sx86_ubyte i;

		i = softx86_fetch_exec_byte(ctx);
		softx86_int_sw_signal(ctx,i);

		return 1;
	}
	else if (opcode == 0xCE) {				// INTO
		if (ctx->state->reg_flags.val & SX86_CPUFLAG_OVERFLOW) {
			softx86_int_sw_signal(ctx,4);
		}

		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_int(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if (opcode == 0xCC) {					// INT 3
		strcpy(buf,"INT 3");
		return 1;
	}
	else if (opcode == 0xCD) {				// INT n
		sx86_ubyte i;

		i = softx86_fetch_dec_byte(ctx);
		sprintf(buf,"INT %02Xh",i);
		return 1;
	}
	else if (opcode == 0xCE) {				// INTO
		strcpy(buf,"INTO");
		return 1;
	}

	return 0;
}
