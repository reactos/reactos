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
#include <multiboot.h>


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

	/* Zero BootDrive and BootPartition */
	xorl	%eax,%eax
	movl	%eax,(_BootDrive)
	movl	%eax,(_BootPartition)

	/* Store the boot drive */
	movb	%dl,(_BootDrive)

	/* Store the boot partition */
	movb	%dh,(_BootPartition)

	/* GO! */
	pushl	%eax
	call	_BootMain

	call	switch_to_real
	.code16

	int	$0x19

	/* We should never get here */
stop:
	jmp	stop
	nop
	nop

/*
 * Switches the processor to protected mode
 * it destroys eax
 */
EXTERN(switch_to_prot)

	.code16

	cli		/* None of these */

	/* We don't know what values are currently */
	/* in the segment registers. So we are */
	/* going to reload them with sane values. */
	/* Of course CS has to already be valid. */
	/* We are currently in real-mode so we */
	/* need real-mode segment values. */
	xorw	%ax,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	movw	%ax,%ss

	/* Get the return address off the stack */
	popw	(code32ret)

	/* Save 16-bit stack pointer */
	movw	%sp,stack16

	/* Load the GDT */
	lgdt	gdtptr
	/* Load the IDT */
	lidt	i386idtptr

	/* Enable Protected Mode */
	mov	%cr0,%eax
	orl	$CR0_PE_SET,%eax
	mov	%eax,%cr0

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

	/* We don't know what values are currently */
	/* in the segment registers. So we are */
	/* going to reload them with sane values. */
	/* Of course CS has to already be valid. */
	/* We are currently in protected-mode so we */
	/* need protected-mode segment values. */
	movw	$PMODE_DS,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	movw	%ax,%ss

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
	mov	%cr0,%eax
	andl	$CR0_PE_CLR,%eax
	mov	%eax,%cr0

	/* Clear prefetch queue & correct CS */
	ljmp	$0, $inrmode

inrmode:
	movw	%cs,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	movw	%ax,%ss

	/* Clear out the high 16-bits of ESP */
	/* This is needed because I have one */
	/* machine that hangs when booted to dos if */
	/* anything other than 0x0000 is in the high */
	/* 16-bits of ESP. Even though real-mode */
	/* code should only use SP and not ESP. */
	xorl	%esp,%esp

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
	inb	$0x64,%al
	cmp	$0xff, %al               // legacy-free machine without keyboard
	jz	empty_8042_ret           // controllers on Intel Macs read back 0xFF
	testb	$0x02,%al
	jnz	empty_8042
empty_8042_ret:
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
	mov	$0xDF,%al                // A20 on
	out	%al,$0x60
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
	mov	$0xDD,%al                // A20 off
	out	%al,$0x60
	call	empty_8042
	call	switch_to_prot
	.code32

	popal

	ret

/* Multiboot support
 *
 * Allows freeldr to be loaded as a "multiboot kernel" by
 * other boot loaders like Grub
 */

#define MB_INFO_SIZE                90
#define MB_INFO_FLAGS_OFFSET        0
#define MB_INFO_BOOT_DEVICE_OFFSET  12
#define MB_INFO_COMMAND_LINE_OFFSET 16
#define CMDLINE_SIZE                256

/*
 * We want to execute at 0x8000 (to be compatible with bootsector
 * loading), but Grub only allows loading of multiboot kernels
 * above 1MB. So we let Grub load us there and then relocate
 * ourself to 0x8000
 */
#define FREELDR_BASE 0x8000
#define INITIAL_BASE 0x200000

	/* Align 32 bits boundary */
	.align 4
	
	/* Multiboot header */
MultibootHeader:
	/* magic */
	.long MULTIBOOT_HEADER_MAGIC
	/* flags */
	.long MULTIBOOT_HEADER_FLAGS
	/* checksum */
	.long -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
	/* header_addr */
	.long INITIAL_BASE + MultibootHeader - FREELDR_BASE
	/* load_addr */
	.long INITIAL_BASE
	/* load_end_addr */
	.long INITIAL_BASE + __bss_start__ - FREELDR_BASE
	/* bss_end_addr */
	.long INITIAL_BASE + __bss_end__ - FREELDR_BASE
	/* entry_addr */
	.long INITIAL_BASE + MultibootEntry - FREELDR_BASE

MultibootEntry:
	cli		/* Even after setting up the our IDT below we are
			 * not ready to handle hardware interrupts (no entries
			 * in IDT), so there's no sti here. Interrupts will be
			 * enabled in due time */

	/* Although the multiboot spec says we should be called with the
	 * segment registers set to 4GB flat mode, let's be sure and set up
	 * our own */
	lgdt	gdtptrhigh + INITIAL_BASE - FREELDR_BASE
	/* Reload segment selectors */
	ljmp	$PMODE_CS, $(mb1 + INITIAL_BASE - FREELDR_BASE)
mb1:
	movw	$PMODE_DS,%dx
	movw	%dx,%ds
	movw	%dx,%es

	/* Check for valid multiboot signature */
	cmpl	$MULTIBOOT_BOOTLOADER_MAGIC,%eax
	jne	mbfail

	/* Store multiboot info in a safe place */
	movl	%ebx,%esi
	movl	$(mb_info + INITIAL_BASE - FREELDR_BASE),%edi
	movl	$MB_INFO_SIZE,%ecx
	rep movsb

	/* Save commandline */
	movl	MB_INFO_FLAGS_OFFSET(%ebx),%edx
	testl	$MB_INFO_FLAG_COMMAND_LINE,MB_INFO_FLAGS_OFFSET(%ebx)
	jz	mb3
	movl	MB_INFO_COMMAND_LINE_OFFSET(%ebx),%esi
	movl	$(cmdline + INITIAL_BASE - FREELDR_BASE),%edi
	movl	$CMDLINE_SIZE,%ecx
mb2:	lodsb
	stosb
	testb	%al,%al
	jz	mb3
	dec	%ecx
	jnz	mb2
mb3:

	/* Copy to low mem */
	movl	$INITIAL_BASE,%esi
	movl	$FREELDR_BASE,%edi
	movl	$(__bss_end__ - FREELDR_BASE),%ecx
	addl	$3,%ecx
	shrl	$2,%ecx
	rep movsl

	/* Load the GDT and IDT */
	lgdt	gdtptr
	lidt	i386idtptr

	/* Clear prefetch queue & correct CS,
	 * jump to low mem */
	ljmp	$PMODE_CS, $mb4
mb4:
	/* Reload segment selectors */
	movw	$PMODE_DS,%dx
	movw	%dx,%ds
	movw	%dx,%es
	movw	%dx,%fs
	movw	%dx,%gs
	movw	%dx,%ss
	movl	$STACK32ADDR,%esp

	movl	$mb_info,%ebx
	/* See if the boot device was passed in */
	movl	MB_INFO_FLAGS_OFFSET(%ebx),%edx
	testl	$MB_INFO_FLAG_BOOT_DEVICE,%edx
	jz	mb5
	/* Retrieve boot device info */
	movl	MB_INFO_BOOT_DEVICE_OFFSET(%ebx),%eax
	shrl	$16,%eax
	incb	%al
	movb	%al,_BootPartition
	movb	%ah,_BootDrive
	jmp	mb6
mb5:	/* No boot device known, assume first partition of first harddisk */
	movb	$0x80,_BootDrive
	movb	$1,_BootPartition
mb6:
	/* Check for command line */
	mov	$cmdline,%eax
	testl	$MB_INFO_FLAG_COMMAND_LINE,MB_INFO_FLAGS_OFFSET(%ebx)
	jnz	mb7
	xorl	%eax,%eax
mb7:

	/* GO! */
	pushl	%eax	
	call	_BootMain

mbfail:	call	switch_to_real
	.code16
	int	$0x19
mbstop:	jmp	mbstop	/* We should never get here */

	.code32

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

/* Initial GDT table pointer for multiboot */
gdtptrhigh:
	.word	0x27		/* Limit */
	.long	gdt + INITIAL_BASE - FREELDR_BASE	/* Base Address */

/* Real-mode IDT pointer */
rmode_idtptr:
	.word	0x3ff		/* Limit */
	.long	0			/* Base Address */

mb_info:
	.fill	MB_INFO_SIZE, 1, 0

cmdline:
	.fill	CMDLINE_SIZE, 1, 0

EXTERN(_BootDrive)
    .long 0
    
EXTERN(_BootPartition)
    .long 0
