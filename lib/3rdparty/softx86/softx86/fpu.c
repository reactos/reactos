/*
 * fpu.c
 *
 * Copyright (C) 2003, 2004 Jonathan Campbell <jcampbell@mdjk.com>
 *
 * Handles 80x87 FPU opcodes by passing them down through callbacks
 * set up in application.
 *
 * The idea behind this is to handle these opcodes the same way the
 * 8086, 80286, 80386, and some 80486s handled them by passing them
 * off to an optional external FPU (possibly provided by another
 * library similar to this).
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
#include "fpu.h"

int Sfx86OpcodeExec_fpuhandoff(sx86_ubyte opcode,softx86_ctx* ctx)
{
	return ctx->callbacks->on_fpu_opcode_exec(ctx,ctx->ref_softx87_ctx,opcode);
}

int Sfx86OpcodeDec_fpuhandoff(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	return ctx->callbacks->on_fpu_opcode_dec(ctx,ctx->ref_softx87_ctx,opcode,buf);
}
