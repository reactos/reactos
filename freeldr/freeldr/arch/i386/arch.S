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

	.text
	.code16

#define ASM
#include <arch.h>


EXTERN(RealEntryPoint)

	cli

	/* Setup segment registers */
	xorw	%ax,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	movw	%ax,%ss
	/* Setup a stack */
	movw	stack16,%sp

	sti

	/* Init pmode */
	call	switch_to_prot

	.code32

	/* Store the boot drive */
	movb	%dl,(_BootDrive)

	/* Store the boot partition */
	movb	%dh,(_BootPartition)

	/* GO! */
	call	_BootMain

	call	switch_to_real
	.code16

	int		$0x19

	/* We should never get here */
stop:
	jmp		stop


/*
 * Switches the processor to protected mode
 * it destroys eax
 */
EXTERN(switch_to_prot)

	.code16

	cli		/* None of these */

	/* Get the return address off the stack */
	popw	(code32ret)

	/* Save 16-bit stack pointer */
	movw	%sp,stack16

	/* Load the GDT */
	lgdt	gdtptr
	/* Load the IDT */
	lidt	i386idtptr

	/* Enable Protected Mode */
	mov		%cr0,%eax
	orl		$CR0_PE_SET,%eax
	mov		%eax,%cr0

	/* Clear prefetch queue & correct CS */
	ljmp	$PMODE_CS, $inpmode


	.code32

inpmode:
	/* Setup segment selectors */
	movw	$PMODE_DS,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	movw	%ax,%ss
	movl	stack32,%esp

	/* Put the return address back onto the stack */
	pushl	(code32ret)

	/* Now return in p-mode! */
	ret

/*
 * Switches the processor back to real mode
 * it destroys eax
 */
EXTERN(switch_to_real)

	.code32

	/* Get the return address off the stack */
	popl	(code16ret)

	/* Save 32-bit stack pointer */
	movl	%esp,stack32

	/* jmp to 16-bit segment to set the limit correctly */
	ljmp	$RMODE_CS, $switch_to_real16

switch_to_real16:
	.code16

	/* Restore segment registers to correct limit */
	movw	$RMODE_DS,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	movw	%ax,%ss

	/* Disable Protected Mode */
	mov		%cr0,%eax
	andl	$CR0_PE_CLR,%eax
	mov		%eax,%cr0

	/* Clear prefetch queue & correct CS */
	ljmp	$0, $inrmode

inrmode:
	movw	%cs,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	movw	%ax,%ss
	movw	stack16,%sp

	/* Put the return address back onto the stack */
	pushw	(code16ret)

	/* Load IDTR with real mode value */
	lidt	rmode_idtptr

	sti		/* These are ok now */

	/* Now return in r-mode! */
	ret


	/*
	 * Needed for enabling the a20 address line
	 */
	.code16
empty_8042:
	.word	0x00eb,0x00eb            // jmp $+2, jmp $+2
	inb     $0x64,%al
    testb   $0x02,%al
    jnz     empty_8042
	ret

	/*
	 * Enable the A20 address line (to allow access to over 1mb)
	 */
EXTERN(_EnableA20)
	.code32

	pushal

	call	switch_to_real
	.code16

	call	empty_8042
    movb	$0xD1,%al                // command write
    outb	%al,$0x64
	call	empty_8042
    mov		$0xDF,%al                // A20 on
    out		%al,$0x60
	call	empty_8042
	call	switch_to_prot
	.code32

	popal

	ret

	/*
	 * Disable the A20 address line
	 */
EXTERN(_DisableA20)
	.code32

	pushal

	call	switch_to_real
	.code16

	call	empty_8042
    movb	$0xD1,%al                // command write
    outb	%al,$0x64
	call	empty_8042
    mov		$0xDD,%al                // A20 off
    out		%al,$0x60
	call	empty_8042
	call	switch_to_prot
	.code32

	popal

	ret




	/* 16-bit stack pointer */
stack16:
	.word	STACK16ADDR

	/* 32-bit stack pointer */
stack32:
	.long	STACK32ADDR

	/* 16-bit return address */
code16ret:
	.long	0

	/* 32-bit return address */
code32ret:
	.long	0


	.p2align	2	/* force 4-byte alignment */
gdt:
	/* NULL Descriptor */
	.word	0x0000
	.word	0x0000
	.word	0x0000
	.word	0x0000

	/* 32-bit flat CS */
	.word	0xFFFF
	.word	0x0000
	.word	0x9A00
	.word	0x00CF

	/* 32-bit flat DS */
	.word	0xFFFF
	.word	0x0000
	.word	0x9200
	.word	0x00CF

	/* 16-bit real mode CS */
	.word	0xFFFF
	.word	0x0000
	.word	0x9E00
	.word	0x0000

	/* 16-bit real mode DS */
	.word	0xFFFF
	.word	0x0000
	.word	0x9200
	.word	0x0000

/* GDT table pointer */
gdtptr:
	.word	0x27		/* Limit */
	.long	gdt			/* Base Address */

/* Real-mode IDT pointer */
rmode_idtptr:
	.word	0x3ff		/* Limit */
	.long	0			/* Base Address */

EXTERN(_BootDrive)
	.long	0

EXTERN(_BootPartition)
	.long	0
