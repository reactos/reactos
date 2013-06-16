/*
 * drooling_duh.c
 *
 * Copyright (C) 2003, 2004 Jonathan Campbell <jcampbell@mdjk.com>
 *
 * Decompiler and executioneer for HLT/WAIT instructions.
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
#include "drooling_duh.h"

int Sfx86OpcodeExec_duhhh(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if (opcode == 0x9B) {						// WAIT
		return 1;
	}
	if (opcode == 0xF4) {						// HLT
/* if an external interrupt pending, proceed */
		if (ctx->state->int_hw_flag || ctx->state->int_nmi)
			return 1;
/* else, do not proceed */
		else
			return 3;
	}

	return 0;
}

int Sfx86OpcodeDec_duhhh(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if (opcode == 0x9B) {						// WAIT
		strcpy(buf,"WAIT");
		return 1;
	}
	if (opcode == 0xF4) {						// HLT
		strcpy(buf,"HLT");
		return 1;
	}

	return 0;
}
