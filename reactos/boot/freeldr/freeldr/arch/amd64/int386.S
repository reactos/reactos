/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

	.intel_syntax noprefix
	.text
	.code16

#define ASM
#include <arch.h>


Int386_REGS:

Int386_eax:
	.long 0
Int386_ebx:
	.long 0
Int386_ecx:
	.long 0
Int386_edx:
	.long 0

Int386_esi:
	.long 0
Int386_edi:
	.long 0

Int386_ds:
	.word 0
Int386_es:
	.word 0
Int386_fs:
	.word 0
Int386_gs:
	.word 0

Int386_eflags:
	.long 0

Int386_vector:
	.quad 0
Int386_regsin:
	.quad 0
Int386_regsout:
	.quad 0

/*
 * int Int386(int ivec, REGS* in, REGS* out);
 */
EXTERN(_Int386)
	.code64

	/* Get the function parameters */
	mov Int386_vector, rcx
	mov Int386_vector_opcode, cl
	mov Int386_regsin, rdx
	mov Int386_regsout, r8

	/* Save all registers + segment registers */
//	push ds
//	push es
	push fs
	push gs
	push rbx
	push rcx
	push rdx
	push rsi
	push rdi

	/* Copy the input regs to our variables */
	lea rdi, Int386_REGS
	mov esi, Int386_regsin
	mov ecx, 0x24
	rep movsb

	call x86_64_SwitchToReal
	.code16

	/* Setup the registers */
	mov ax, cs:Int386_ds
	mov ds, ax					/* DS register */
	mov ax, cs:Int386_es
	mov es, ax					/* ES register */
	mov ax, cs:Int386_fs
	mov fs, ax					/* FS register */
	mov ax, cs:Int386_gs
	mov gs, ax					/* GS register */

	mov eax, cs:Int386_eax		/* EAX register */
	mov ebx, cs:Int386_ebx		/* EBX register */
	mov ecx, cs:Int386_ecx		/* ECX register */
	mov edx, cs:Int386_edx		/* EDX register */

	mov esi, cs:Int386_esi		/* ESI register */
	mov edi, cs:Int386_edi		/* EDI register */

	/* Do not set the flags register */
	/* only return its value in regsout */
	//pushl	Int386_eflags
	//popfl							/* EFLAGS register */

	/* Call the interrupt vector */
	/*int		Int386_vector*/
Int386_int_opcode:
	.byte	0xcd
Int386_vector_opcode:
	.byte	0x00

	/* Save the registers */
	mov cs:Int386_eax, eax		/* EAX register */
	mov cs:Int386_ebx, ebx		/* EBX register */
	mov cs:Int386_ecx, ecx		/* ECX register */
	mov cs:Int386_edx, edx		/* EDX register */

	mov cs:Int386_esi, esi		/* ESI register */
	mov cs:Int386_edi, edi		/* EDI register */

	mov ax, ds					/* DS register */
	mov cs:Int386_ds, ax
	mov ax, es					/* ES register */
	mov cs:Int386_es, ax
	mov ax, fs					/* FS register */
	mov cs:Int386_fs, ax
	mov ax, gs					/* GS register */
	mov cs:Int386_gs, ax

	pushf
	pop cs:Int386_eflags		/* EFLAGS register */

	call	x86_16_SwitchToLong
	.code64

	/* Copy the variables to the output regs */
	lea rsi, Int386_REGS
	mov rdi, Int386_regsout
	mov rcx, 0x24
	rep movsb

	/* Restore segment and all other registers */
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop gs
	pop fs
//	pop es
//	pop ds

	/* Get return value */
	xor rax, rax
	mov eax, Int386_eax

	ret
