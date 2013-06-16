/*
 * optable.c
 *
 * Copyright (C) 2003, 2004 Jonathan Campbell <jcampbell@mdjk.com>
 *
 * Opcode jumptable.
 *
 * Allows the recognition of many opcodes without having to write approximately
 * 500+ if...then...else statements which is a) inefficient and b) apparently can
 * cause some compilers such as Microsoft C++ to crash during the compile stage with
 * an error. Since it's a table, it can be referred to via a pointer that can be easily
 * redirected to other opcode tables (i.e., one for the 8086, one for the 80286, etc.)
 * without much hassle.
 * 
 * The table contains two pointers: one for an "execute" function, and one for a
 * "decompile" function. The execute function is given the context and the initial opcode.
 * If more opcodes are needed the function calls softx86_fetch_exec_byte(). The decode
 * function is also given the opcode but also a char[] array where it is expected to
 * sprintf() or strcpy() the disassembled output. If that function needs more opcodes
 * it calls softx86_fetch_dec_byte().
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
#include "pushpop.h"
#include "procframe.h"
#include "aaa.h"
#include "add.h"
#include "cbw.h"
#include "clc.h"
#include "drooling_duh.h"
#include "groupies.h"
#include "interrupts.h"
#include "ioport.h"
#include "inc.h"
#include "fpu.h"
#include "mov.h"
#include "prefixes.h"
#include "binops.h"
#include "jumpy.h"
#include "shovel.h"

/* temporary buffers for decoding instruction parameters */
char			op1_tmp[32];
char			op2_tmp[32];

char *sx86_regs8[8] = {
	"AL","CL","DL","BL","AH","CH","DH","BH"};
char *sx86_regs16[8] = {
	"AX","CX","DX","BX","SP","BP","SI","DI"};
char *sx86_regs32[8] = {
	"EAX","ECX","EDX","EBX","ESP","EBP","ESI","EDI"};
char *sx86_regsaddr16_16[8] = {
	"BX+SI","BX+DI","BP+SI","BP+DI","SI","DI","BP","BX"};
char *sx86_segregs[8] = {
	"ES","CS","SS","DS","?4","?5","?6","?7"};

static int Sfx86OpcodeExec_default(sx86_ubyte opcode,softx86_ctx* ctx)
{
	return 0;
}

static int Sfx86OpcodeDec_default(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	buf[0]=0;

	return 0;
}

int Sfx86OpcodeExec_nop(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if (opcode != 0x90) return 0;
	return 1;
}

int Sfx86OpcodeDec_nop(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if (opcode != 0x90) return 0;
	strcpy(buf,"NOP");
	return 1;
}

Sfx86OpcodeTable optab8086 = {
{
	{Sfx86OpcodeExec_add,		Sfx86OpcodeDec_add},					/* 0x00 */
	{Sfx86OpcodeExec_add,		Sfx86OpcodeDec_add},					/* 0x01 */
	{Sfx86OpcodeExec_add,		Sfx86OpcodeDec_add},					/* 0x02 */
	{Sfx86OpcodeExec_add,		Sfx86OpcodeDec_add},					/* 0x03 */
	{Sfx86OpcodeExec_add,		Sfx86OpcodeDec_add},					/* 0x04 */
	{Sfx86OpcodeExec_add,		Sfx86OpcodeDec_add},					/* 0x05 */
	{Sfx86OpcodeExec_push,		Sfx86OpcodeDec_push},					/* 0x06 */
	{Sfx86OpcodeExec_pop,		Sfx86OpcodeDec_pop},					/* 0x07 */

	{Sfx86OpcodeExec_or,		Sfx86OpcodeDec_or},					/* 0x08 */
	{Sfx86OpcodeExec_or,		Sfx86OpcodeDec_or},					/* 0x09 */
	{Sfx86OpcodeExec_or,		Sfx86OpcodeDec_or},					/* 0x0A */
	{Sfx86OpcodeExec_or,		Sfx86OpcodeDec_or},					/* 0x0B */
	{Sfx86OpcodeExec_or,		Sfx86OpcodeDec_or},					/* 0x0C */
	{Sfx86OpcodeExec_or,		Sfx86OpcodeDec_or},					/* 0x0D */
	{Sfx86OpcodeExec_push,		Sfx86OpcodeDec_push},					/* 0x0E */
	{Sfx86OpcodeExec_pop,		Sfx86OpcodeDec_pop},					/* 0x0F */

	{Sfx86OpcodeExec_adc,		Sfx86OpcodeDec_adc},					/* 0x10 */
	{Sfx86OpcodeExec_adc,		Sfx86OpcodeDec_adc},					/* 0x11 */
	{Sfx86OpcodeExec_adc,		Sfx86OpcodeDec_adc},					/* 0x12 */
	{Sfx86OpcodeExec_adc,		Sfx86OpcodeDec_adc},					/* 0x13 */
	{Sfx86OpcodeExec_adc,		Sfx86OpcodeDec_adc},					/* 0x14 */
	{Sfx86OpcodeExec_adc,		Sfx86OpcodeDec_adc},					/* 0x15 */
	{Sfx86OpcodeExec_push,		Sfx86OpcodeDec_push},					/* 0x16 */
	{Sfx86OpcodeExec_pop,		Sfx86OpcodeDec_pop},					/* 0x17 */

	{Sfx86OpcodeExec_sbb,		Sfx86OpcodeDec_sbb},					/* 0x18 */
	{Sfx86OpcodeExec_sbb,		Sfx86OpcodeDec_sbb},					/* 0x19 */
	{Sfx86OpcodeExec_sbb,		Sfx86OpcodeDec_sbb},					/* 0x1A */
	{Sfx86OpcodeExec_sbb,		Sfx86OpcodeDec_sbb},					/* 0x1B */
	{Sfx86OpcodeExec_sbb,		Sfx86OpcodeDec_sbb},					/* 0x1C */
	{Sfx86OpcodeExec_sbb,		Sfx86OpcodeDec_sbb},					/* 0x1D */
	{Sfx86OpcodeExec_push,		Sfx86OpcodeDec_push},					/* 0x1E */
	{Sfx86OpcodeExec_pop,		Sfx86OpcodeDec_pop},					/* 0x1F */

	{Sfx86OpcodeExec_and,		Sfx86OpcodeDec_and},					/* 0x20 */
	{Sfx86OpcodeExec_and,		Sfx86OpcodeDec_and},					/* 0x21 */
	{Sfx86OpcodeExec_and,		Sfx86OpcodeDec_and},					/* 0x22 */
	{Sfx86OpcodeExec_and,		Sfx86OpcodeDec_and},					/* 0x23 */
	{Sfx86OpcodeExec_and,		Sfx86OpcodeDec_and},					/* 0x24 */
	{Sfx86OpcodeExec_and,		Sfx86OpcodeDec_and},					/* 0x25 */
	{Sfx86OpcodeExec_segover,	Sfx86OpcodeDec_segover},				/* 0x26 */
	{Sfx86OpcodeExec_aaaseries,	Sfx86OpcodeDec_aaaseries},				/* 0x27 */

	{Sfx86OpcodeExec_sub,		Sfx86OpcodeDec_sub},					/* 0x28 */
	{Sfx86OpcodeExec_sub,		Sfx86OpcodeDec_sub},					/* 0x29 */
	{Sfx86OpcodeExec_sub,		Sfx86OpcodeDec_sub},					/* 0x2A */
	{Sfx86OpcodeExec_sub,		Sfx86OpcodeDec_sub},					/* 0x2B */
	{Sfx86OpcodeExec_sub,		Sfx86OpcodeDec_sub},					/* 0x2C */
	{Sfx86OpcodeExec_sub,		Sfx86OpcodeDec_sub},					/* 0x2D */
	{Sfx86OpcodeExec_segover,	Sfx86OpcodeDec_segover},				/* 0x2E */
	{Sfx86OpcodeExec_aaaseries,	Sfx86OpcodeDec_aaaseries},				/* 0x2F */

	{Sfx86OpcodeExec_xor,		Sfx86OpcodeDec_xor},					/* 0x30 */
	{Sfx86OpcodeExec_xor,		Sfx86OpcodeDec_xor},					/* 0x31 */
	{Sfx86OpcodeExec_xor,		Sfx86OpcodeDec_xor},					/* 0x32 */
	{Sfx86OpcodeExec_xor,		Sfx86OpcodeDec_xor},					/* 0x33 */
	{Sfx86OpcodeExec_xor,		Sfx86OpcodeDec_xor},					/* 0x34 */
	{Sfx86OpcodeExec_xor,		Sfx86OpcodeDec_xor},					/* 0x35 */
	{Sfx86OpcodeExec_segover,	Sfx86OpcodeDec_segover},				/* 0x36 */
	{Sfx86OpcodeExec_aaaseries,	Sfx86OpcodeDec_aaaseries},				/* 0x37 */

	{Sfx86OpcodeExec_cmp,		Sfx86OpcodeDec_cmp},					/* 0x38 */
	{Sfx86OpcodeExec_cmp,		Sfx86OpcodeDec_cmp},					/* 0x39 */
	{Sfx86OpcodeExec_cmp,		Sfx86OpcodeDec_cmp},					/* 0x3A */
	{Sfx86OpcodeExec_cmp,		Sfx86OpcodeDec_cmp},					/* 0x3B */
	{Sfx86OpcodeExec_cmp,		Sfx86OpcodeDec_cmp},					/* 0x3C */
	{Sfx86OpcodeExec_cmp,		Sfx86OpcodeDec_cmp},					/* 0x3D */
	{Sfx86OpcodeExec_segover,	Sfx86OpcodeDec_segover},				/* 0x3E */
	{Sfx86OpcodeExec_aaaseries,	Sfx86OpcodeDec_aaaseries},				/* 0x3F */

	{Sfx86OpcodeExec_inc,		Sfx86OpcodeDec_inc},					/* 0x40 */
	{Sfx86OpcodeExec_inc,		Sfx86OpcodeDec_inc},					/* 0x41 */
	{Sfx86OpcodeExec_inc,		Sfx86OpcodeDec_inc},					/* 0x42 */
	{Sfx86OpcodeExec_inc,		Sfx86OpcodeDec_inc},					/* 0x43 */
	{Sfx86OpcodeExec_inc,		Sfx86OpcodeDec_inc},					/* 0x44 */
	{Sfx86OpcodeExec_inc,		Sfx86OpcodeDec_inc},					/* 0x45 */
	{Sfx86OpcodeExec_inc,		Sfx86OpcodeDec_inc},					/* 0x46 */
	{Sfx86OpcodeExec_inc,		Sfx86OpcodeDec_inc},					/* 0x47 */

	{Sfx86OpcodeExec_inc,		Sfx86OpcodeDec_inc},					/* 0x48 */
	{Sfx86OpcodeExec_inc,		Sfx86OpcodeDec_inc},					/* 0x49 */
	{Sfx86OpcodeExec_inc,		Sfx86OpcodeDec_inc},					/* 0x4A */
	{Sfx86OpcodeExec_inc,		Sfx86OpcodeDec_inc},					/* 0x4B */
	{Sfx86OpcodeExec_inc,		Sfx86OpcodeDec_inc},					/* 0x4C */
	{Sfx86OpcodeExec_inc,		Sfx86OpcodeDec_inc},					/* 0x4D */
	{Sfx86OpcodeExec_inc,		Sfx86OpcodeDec_inc},					/* 0x4E */
	{Sfx86OpcodeExec_inc,		Sfx86OpcodeDec_inc},					/* 0x4F */

	{Sfx86OpcodeExec_push,		Sfx86OpcodeDec_push},					/* 0x50 */
	{Sfx86OpcodeExec_push,		Sfx86OpcodeDec_push},					/* 0x51 */
	{Sfx86OpcodeExec_push,		Sfx86OpcodeDec_push},					/* 0x52 */
	{Sfx86OpcodeExec_push,		Sfx86OpcodeDec_push},					/* 0x53 */
	{Sfx86OpcodeExec_push,		Sfx86OpcodeDec_push},					/* 0x54 */
	{Sfx86OpcodeExec_push,		Sfx86OpcodeDec_push},					/* 0x55 */
	{Sfx86OpcodeExec_push,		Sfx86OpcodeDec_push},					/* 0x56 */
	{Sfx86OpcodeExec_push,		Sfx86OpcodeDec_push},					/* 0x57 */

	{Sfx86OpcodeExec_pop,		Sfx86OpcodeDec_pop},					/* 0x58 */
	{Sfx86OpcodeExec_pop,		Sfx86OpcodeDec_pop},					/* 0x59 */
	{Sfx86OpcodeExec_pop,		Sfx86OpcodeDec_pop},					/* 0x5A */
	{Sfx86OpcodeExec_pop,		Sfx86OpcodeDec_pop},					/* 0x5B */
	{Sfx86OpcodeExec_pop,		Sfx86OpcodeDec_pop},					/* 0x5C */
	{Sfx86OpcodeExec_pop,		Sfx86OpcodeDec_pop},					/* 0x5D */
	{Sfx86OpcodeExec_pop,		Sfx86OpcodeDec_pop},					/* 0x5E */
	{Sfx86OpcodeExec_pop,		Sfx86OpcodeDec_pop},					/* 0x5F */

	{Sfx86OpcodeExec_pusha,		Sfx86OpcodeDec_pusha},					/* 0x60 */
	{Sfx86OpcodeExec_popa,		Sfx86OpcodeDec_popa},					/* 0x61 */
	{Sfx86OpcodeExec_bound,		Sfx86OpcodeDec_bound},					/* 0x62 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x63 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x64 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x65 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x66 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x67 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x68 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x69 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x6A */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x6B */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x6C */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x6D */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x6E */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x6F */

	{Sfx86OpcodeExec_jc,		Sfx86OpcodeDec_jc},					/* 0x70 */
	{Sfx86OpcodeExec_jc,		Sfx86OpcodeDec_jc},					/* 0x71 */
	{Sfx86OpcodeExec_jc,		Sfx86OpcodeDec_jc},					/* 0x72 */
	{Sfx86OpcodeExec_jc,		Sfx86OpcodeDec_jc},					/* 0x73 */
	{Sfx86OpcodeExec_jc,		Sfx86OpcodeDec_jc},					/* 0x74 */
	{Sfx86OpcodeExec_jc,		Sfx86OpcodeDec_jc},					/* 0x75 */
	{Sfx86OpcodeExec_jc,		Sfx86OpcodeDec_jc},					/* 0x76 */
	{Sfx86OpcodeExec_jc,		Sfx86OpcodeDec_jc},					/* 0x77 */

	{Sfx86OpcodeExec_jc,		Sfx86OpcodeDec_jc},					/* 0x78 */
	{Sfx86OpcodeExec_jc,		Sfx86OpcodeDec_jc},					/* 0x79 */
	{Sfx86OpcodeExec_jc,		Sfx86OpcodeDec_jc},					/* 0x7A */
	{Sfx86OpcodeExec_jc,		Sfx86OpcodeDec_jc},					/* 0x7B */
	{Sfx86OpcodeExec_jc,		Sfx86OpcodeDec_jc},					/* 0x7C */
	{Sfx86OpcodeExec_jc,		Sfx86OpcodeDec_jc},					/* 0x7D */
	{Sfx86OpcodeExec_jc,		Sfx86OpcodeDec_jc},					/* 0x7E */
	{Sfx86OpcodeExec_jc,		Sfx86OpcodeDec_jc},					/* 0x7F */

	{Sfx86OpcodeExec_group80,	Sfx86OpcodeDec_group80},				/* 0x80 */
	{Sfx86OpcodeExec_group80,	Sfx86OpcodeDec_group80},				/* 0x81 */
	{Sfx86OpcodeExec_group80,	Sfx86OpcodeDec_group80},				/* 0x82 */
	{Sfx86OpcodeExec_group80,	Sfx86OpcodeDec_group80},				/* 0x83 */
	{Sfx86OpcodeExec_test,		Sfx86OpcodeDec_test},					/* 0x84 */
	{Sfx86OpcodeExec_test,		Sfx86OpcodeDec_test},					/* 0x85 */
	{Sfx86OpcodeExec_xchg,		Sfx86OpcodeDec_xchg},					/* 0x86 */
	{Sfx86OpcodeExec_xchg,		Sfx86OpcodeDec_xchg},					/* 0x87 */

	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0x88 */
	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0x89 */
	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0x8A */
	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0x8B */
	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0x8C */
	{Sfx86OpcodeExec_lea,		Sfx86OpcodeDec_lea},					/* 0x8D */
	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0x8E */
	{Sfx86OpcodeExec_group8F,	Sfx86OpcodeDec_group8F},				/* 0x8F */

	{Sfx86OpcodeExec_nop,		Sfx86OpcodeDec_nop},					/* 0x90 */
	{Sfx86OpcodeExec_xchg,		Sfx86OpcodeDec_xchg},					/* 0x91 */
	{Sfx86OpcodeExec_xchg,		Sfx86OpcodeDec_xchg},					/* 0x92 */
	{Sfx86OpcodeExec_xchg,		Sfx86OpcodeDec_xchg},					/* 0x93 */
	{Sfx86OpcodeExec_xchg,		Sfx86OpcodeDec_xchg},					/* 0x94 */
	{Sfx86OpcodeExec_xchg,		Sfx86OpcodeDec_xchg},					/* 0x95 */
	{Sfx86OpcodeExec_xchg,		Sfx86OpcodeDec_xchg},					/* 0x96 */
	{Sfx86OpcodeExec_xchg,		Sfx86OpcodeDec_xchg},					/* 0x97 */

	{Sfx86OpcodeExec_cxex,		Sfx86OpcodeDec_cxex},					/* 0x98 */
	{Sfx86OpcodeExec_cxex,		Sfx86OpcodeDec_cxex},					/* 0x99 */
	{Sfx86OpcodeExec_call,		Sfx86OpcodeDec_call},					/* 0x9A */
	{Sfx86OpcodeExec_duhhh,		Sfx86OpcodeDec_duhhh},					/* 0x9B */
	{Sfx86OpcodeExec_push,		Sfx86OpcodeDec_push},					/* 0x9C */
	{Sfx86OpcodeExec_pop,		Sfx86OpcodeDec_pop},					/* 0x9D */
	{Sfx86OpcodeExec_ahf,		Sfx86OpcodeDec_ahf},					/* 0x9E */
	{Sfx86OpcodeExec_ahf,		Sfx86OpcodeDec_ahf},					/* 0x9F */

	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0xA0 */
	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0xA1 */
	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0xA2 */
	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0xA3 */
	{Sfx86OpcodeExec_shovel,	Sfx86OpcodeDec_shovel},					/* 0xA4 */
	{Sfx86OpcodeExec_shovel,	Sfx86OpcodeDec_shovel},					/* 0xA5 */
	{Sfx86OpcodeExec_shovel,	Sfx86OpcodeDec_shovel},					/* 0xA6 */
	{Sfx86OpcodeExec_shovel,	Sfx86OpcodeDec_shovel},					/* 0xA7 */

	{Sfx86OpcodeExec_test,		Sfx86OpcodeDec_test},					/* 0xA8 */
	{Sfx86OpcodeExec_test,		Sfx86OpcodeDec_test},					/* 0xA9 */
	{Sfx86OpcodeExec_shovel,	Sfx86OpcodeDec_shovel},					/* 0xAA */
	{Sfx86OpcodeExec_shovel,	Sfx86OpcodeDec_shovel},					/* 0xAB */
	{Sfx86OpcodeExec_shovel,	Sfx86OpcodeDec_shovel},					/* 0xAC */
	{Sfx86OpcodeExec_shovel,	Sfx86OpcodeDec_shovel},					/* 0xAD */
	{Sfx86OpcodeExec_shovel,	Sfx86OpcodeDec_shovel},					/* 0xAE */
	{Sfx86OpcodeExec_shovel,	Sfx86OpcodeDec_shovel},					/* 0xAF */

	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0xB0 */
	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0xB1 */
	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0xB2 */
	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0xB3 */
	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0xB4 */
	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0xB5 */
	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0xB6 */
	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0xB7 */

	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0xB8 */
	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0xB9 */
	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0xBA */
	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0xBB */
	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0xBC */
	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0xBD */
	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0xBE */
	{Sfx86OpcodeExec_mov,		Sfx86OpcodeDec_mov},					/* 0xBF */

	{Sfx86OpcodeExec_groupC0,	Sfx86OpcodeDec_groupC0},				/* 0xC0 */
	{Sfx86OpcodeExec_groupC0,	Sfx86OpcodeDec_groupC0},				/* 0xC1 */
	{Sfx86OpcodeExec_returns,	Sfx86OpcodeDec_returns},				/* 0xC2 */
	{Sfx86OpcodeExec_returns,	Sfx86OpcodeDec_returns},				/* 0xC3 */
	{Sfx86OpcodeExec_les,		Sfx86OpcodeDec_les},					/* 0xC4 */
	{Sfx86OpcodeExec_les,		Sfx86OpcodeDec_les},					/* 0xC5 */
	{Sfx86OpcodeExec_groupC6,	Sfx86OpcodeDec_groupC6},				/* 0xC6 */
	{Sfx86OpcodeExec_groupC6,	Sfx86OpcodeDec_groupC6},				/* 0xC7 */

	{Sfx86OpcodeExec_enterleave,	Sfx86OpcodeDec_enterleave},				/* 0xC8 */
	{Sfx86OpcodeExec_enterleave,	Sfx86OpcodeDec_enterleave},				/* 0xC9 */
	{Sfx86OpcodeExec_returns,	Sfx86OpcodeDec_returns},				/* 0xCA */
	{Sfx86OpcodeExec_returns,	Sfx86OpcodeDec_returns},				/* 0xCB */
	{Sfx86OpcodeExec_int,		Sfx86OpcodeDec_int},					/* 0xCC */
	{Sfx86OpcodeExec_int,		Sfx86OpcodeDec_int},					/* 0xCD */
	{Sfx86OpcodeExec_int,		Sfx86OpcodeDec_int},					/* 0xCE */
	{Sfx86OpcodeExec_returns,	Sfx86OpcodeDec_returns},				/* 0xCF */

	{Sfx86OpcodeExec_groupD0,	Sfx86OpcodeDec_groupD0},				/* 0xD0 */
	{Sfx86OpcodeExec_groupD0,	Sfx86OpcodeDec_groupD0},				/* 0xD1 */
	{Sfx86OpcodeExec_groupD0,	Sfx86OpcodeDec_groupD0},				/* 0xD2 */
	{Sfx86OpcodeExec_groupD0,	Sfx86OpcodeDec_groupD0},				/* 0xD3 */
	{Sfx86OpcodeExec_aaaseries,	Sfx86OpcodeDec_aaaseries},				/* 0xD4 */
	{Sfx86OpcodeExec_aaaseries,	Sfx86OpcodeDec_aaaseries},				/* 0xD5 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0xD6 */
	{Sfx86OpcodeExec_xlat,		Sfx86OpcodeDec_xlat},					/* 0xD7 */

	{Sfx86OpcodeExec_fpuhandoff,	Sfx86OpcodeDec_fpuhandoff},				/* 0xD8 */
	{Sfx86OpcodeExec_fpuhandoff,	Sfx86OpcodeDec_fpuhandoff},				/* 0xD9 */
	{Sfx86OpcodeExec_fpuhandoff,	Sfx86OpcodeDec_fpuhandoff},				/* 0xDA */
	{Sfx86OpcodeExec_fpuhandoff,	Sfx86OpcodeDec_fpuhandoff},				/* 0xDB */
	{Sfx86OpcodeExec_fpuhandoff,	Sfx86OpcodeDec_fpuhandoff},				/* 0xDC */
	{Sfx86OpcodeExec_fpuhandoff,	Sfx86OpcodeDec_fpuhandoff},				/* 0xDD */
	{Sfx86OpcodeExec_fpuhandoff,	Sfx86OpcodeDec_fpuhandoff},				/* 0xDE */
	{Sfx86OpcodeExec_fpuhandoff,	Sfx86OpcodeDec_fpuhandoff},				/* 0xDF */

	{Sfx86OpcodeExec_loop,		Sfx86OpcodeDec_loop},					/* 0xE0 */
	{Sfx86OpcodeExec_loop,		Sfx86OpcodeDec_loop},					/* 0xE1 */
	{Sfx86OpcodeExec_loop,		Sfx86OpcodeDec_loop},					/* 0xE2 */
	{Sfx86OpcodeExec_jc,		Sfx86OpcodeDec_jc},					/* 0xE3 */
	{Sfx86OpcodeExec_io,		Sfx86OpcodeDec_io},					/* 0xE4 */
	{Sfx86OpcodeExec_io,		Sfx86OpcodeDec_io},					/* 0xE5 */
	{Sfx86OpcodeExec_io,		Sfx86OpcodeDec_io},					/* 0xE6 */
	{Sfx86OpcodeExec_io,		Sfx86OpcodeDec_io},					/* 0xE7 */

	{Sfx86OpcodeExec_call,		Sfx86OpcodeDec_call},					/* 0xE8 */
	{Sfx86OpcodeExec_jmp,		Sfx86OpcodeDec_jmp},					/* 0xE9 */
	{Sfx86OpcodeExec_jmp,		Sfx86OpcodeDec_jmp},					/* 0xEA */
	{Sfx86OpcodeExec_jmp,		Sfx86OpcodeDec_jmp},					/* 0xEB */
	{Sfx86OpcodeExec_io,		Sfx86OpcodeDec_io},					/* 0xEC */
	{Sfx86OpcodeExec_io,		Sfx86OpcodeDec_io},					/* 0xED */
	{Sfx86OpcodeExec_io,		Sfx86OpcodeDec_io},					/* 0xEE */
	{Sfx86OpcodeExec_io,		Sfx86OpcodeDec_io},					/* 0xEF */

	{Sfx86OpcodeExec_lock,		Sfx86OpcodeDec_lock},					/* 0xF0 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0xF1 */
	{Sfx86OpcodeExec_repetition,	Sfx86OpcodeDec_repetition},				/* 0xF2 */
	{Sfx86OpcodeExec_repetition,	Sfx86OpcodeDec_repetition},				/* 0xF3 */
	{Sfx86OpcodeExec_duhhh,		Sfx86OpcodeDec_duhhh},					/* 0xF4 */
	{Sfx86OpcodeExec_clx,		Sfx86OpcodeDec_clx},					/* 0xF5 */
	{Sfx86OpcodeExec_groupF6,	Sfx86OpcodeDec_groupF6},				/* 0xF6 */
	{Sfx86OpcodeExec_groupF6,	Sfx86OpcodeDec_groupF6},				/* 0xF7 */

	{Sfx86OpcodeExec_clx,		Sfx86OpcodeDec_clx},					/* 0xF8 */
	{Sfx86OpcodeExec_clx,		Sfx86OpcodeDec_clx},					/* 0xF9 */
	{Sfx86OpcodeExec_clx,		Sfx86OpcodeDec_clx},					/* 0xFA */
	{Sfx86OpcodeExec_clx,		Sfx86OpcodeDec_clx},					/* 0xFB */
	{Sfx86OpcodeExec_clx,		Sfx86OpcodeDec_clx},					/* 0xFC */
	{Sfx86OpcodeExec_clx,		Sfx86OpcodeDec_clx},					/* 0xFD */
	{Sfx86OpcodeExec_groupFE,	Sfx86OpcodeDec_groupFE},				/* 0xFE */
	{Sfx86OpcodeExec_groupFE,	Sfx86OpcodeDec_groupFE},				/* 0xFF */
},
};

Sfx86OpcodeTable optab8086_0Fh = {
{
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_lmsw286,	Sfx86OpcodeDec_lmsw286},				/* 0x01 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */

	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
	{Sfx86OpcodeExec_default,	Sfx86OpcodeDec_default},				/* 0x00 */
},
};
