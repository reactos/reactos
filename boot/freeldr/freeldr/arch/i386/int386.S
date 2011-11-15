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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

	.text
	.code16

#include <arch/pc/x86common.h>

#define I386FLAG_CF		HEX(0001)  // Carry Flag
#define I386FLAG_ZF		HEX(0040)  // Zero Flag
#define I386FLAG_SF		HEX(0080)  // Sign Flag
/* Only these flags are propagated into Int386() */
#define FLAGS_PROP	(I386FLAG_CF | \
                     I386FLAG_ZF | \
                     I386FLAG_SF)

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
Int386_ebp:
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
	.long 0
Int386_regsin:
	.long 0
Int386_regsout:
	.long 0

/*
 * int Int386(int ivec, REGS* in, REGS* out);
 */
EXTERN(_Int386)
	.code32

	/* Get the function parameters */
	movl	0x04(%esp),%eax
	movl	%eax,Int386_vector
	movb	%al,Int386_vector_opcode
	movl	0x08(%esp),%eax
	movl	%eax,Int386_regsin
	movl	0x0c(%esp),%eax
	movl	%eax,Int386_regsout

	/* Save all registers + segment registers */
	pushw	%ds
	pushw	%es
	pushw	%fs
	pushw	%gs
	pushal

	/* Copy the input regs to our variables */
	movl	$Int386_REGS,%edi
	movl	Int386_regsin,%esi
	movl	$0x28,%ecx
	cld
	rep
	movsb

	call	switch_to_real
	.code16

	/* Setup the registers */
	movw	%cs:Int386_ds,%ax
	movw	%ax,%ds					/* DS register */
	movw	%cs:Int386_es,%ax
	movw	%ax,%es					/* ES register */
	movw	%cs:Int386_fs,%ax
	movw	%ax,%fs					/* FS register */
	movw	%cs:Int386_gs,%ax
	movw	%ax,%gs					/* GS register */

	/* Prepare EFLAGS for recover */
	pushf
	movw	%cs:Int386_eflags, %ax
	popw	%cx
	andw	$FLAGS_PROP, %ax
	andw	$~FLAGS_PROP, %cx
	orw		%cx, %ax
	pushw	%ax

	/* Recover general purpose registers */
	movl	%cs:Int386_eax,%eax		/* EAX register */
	movl	%cs:Int386_ebx,%ebx		/* EBX register */
	movl	%cs:Int386_ecx,%ecx		/* ECX register */
	movl	%cs:Int386_edx,%edx		/* EDX register */

	movl	%cs:Int386_esi,%esi		/* ESI register */
	movl	%cs:Int386_edi,%edi		/* EDI register */

	/* Recover previously prepared flags */
	popf

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
	movl	%eax,%cs:Int386_eax		/* EAX register */
	movl	%ebx,%cs:Int386_ebx		/* EBX register */
	movl	%ecx,%cs:Int386_ecx		/* ECX register */
	movl	%edx,%cs:Int386_edx		/* EDX register */

	movl	%esi,%cs:Int386_esi		/* ESI register */
	movl	%edi,%cs:Int386_edi		/* EDI register */

	movw	%ds,%ax					/* DS register */
	movw	%ax,%cs:Int386_ds
	movw	%es,%ax					/* ES register */
	movw	%ax,%cs:Int386_es
	movw	%fs,%ax					/* FS register */
	movw	%ax,%cs:Int386_fs
	movw	%gs,%ax					/* GS register */
	movw	%ax,%cs:Int386_gs

	pushf
	popw	%cs:Int386_eflags		/* EFLAGS register */

	call	switch_to_prot
	.code32

	/* Copy the variables to the output regs */
	movl	$Int386_REGS,%esi
	movl	Int386_regsout,%edi
	movl	$0x28,%ecx
	cld
	rep
	movsb

	/* Restore segment and all other registers */


	popal
	popw	%gs
	popw	%fs
	popw	%es
	popw	%ds

	/* Get return value */
	movl	Int386_eax,%eax

	ret
