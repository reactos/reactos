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

.intel_syntax noprefix
#define HEX(y) 0x##y

#include <arch/pc/x86common.h>
#include <multiboot.h>

	.code16

EXTERN(_RealEntryPoint)

	cli

	/* Setup segment registers */
	xor ax, ax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	/* Setup a stack */
	mov sp, word ptr ds:stack16

	sti

	/* Init pmode */
	call	switch_to_prot

	.code32

	/* Zero BootDrive and BootPartition */
	xor eax, eax
	mov dword ptr [_FrldrBootDrive], eax
	mov dword ptr [_FrldrBootPartition], eax

	/* Store the boot drive */
	mov byte ptr [_FrldrBootDrive], dl

	/* Store the boot partition */
	mov byte ptr [_FrldrBootPartition], dh

	/* GO! */
	push eax
	call	_BootMain

	call	switch_to_real
	.code16

	int HEX(19)

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
	xor ax, ax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	/* Get the return address off the stack */
	pop word ptr ds:[code32ret]

	/* Save 16-bit stack pointer */
	mov word ptr ds:[stack16], sp

	/* Load the GDT */
	lgdt	gdtptr
	/* Load the IDT */
	lidt	i386idtptr

	/* Enable Protected Mode */
	mov eax, cr0
	or eax, CR0_PE_SET
	mov cr0, eax

	/* Clear prefetch queue & correct CS */
	//ljmp	PMODE_CS, inpmode
    jmp far ptr PMODE_CS:inpmode

.code32

inpmode:
	/* Setup segment selectors */
	mov ax, PMODE_DS
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov esp, dword ptr [stack32]

	/* Put the return address back onto the stack */
	push dword ptr [code32ret]

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
	mov ax, PMODE_DS
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	/* Get the return address off the stack */
	pop dword ptr [code16ret]

	/* Save 32-bit stack pointer */
	mov dword ptr [stack32], esp

	/* jmp to 16-bit segment to set the limit correctly */
	ljmp RMODE_CS, switch_to_real16

switch_to_real16:
.code16

	/* Restore segment registers to correct limit */
	mov ax, RMODE_DS
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	/* Disable Protected Mode */
	mov	eax, cr0
	and eax, CR0_PE_CLR
	mov	cr0, eax

	/* Clear prefetch queue & correct CS */
	//ljmp	$0, $inrmode
	jmp far ptr 0:inrmode

inrmode:
	mov ax, cs
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	/* Clear out the high 16-bits of ESP */
	/* This is needed because I have one */
	/* machine that hangs when booted to dos if */
	/* anything other than 0x0000 is in the high */
	/* 16-bits of ESP. Even though real-mode */
	/* code should only use SP and not ESP. */
	xor esp, esp

	mov sp, word ptr ds:[stack16]

	/* Put the return address back onto the stack */
	push word ptr ds:[code16ret]

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
	in al, HEX(64)
	cmp al, HEX(ff)               // legacy-free machine without keyboard
	jz empty_8042_ret           // controllers on Intel Macs read back 0xFF
	test al, 2
	jnz empty_8042
empty_8042_ret:
	ret

	/*
	 * Enable the A20 address line (to allow access to over 1mb)
	 */
EXTERN(_EnableA20)
.code32

	pusha

	call switch_to_real
.code16

	call empty_8042
	mov al, HEX(D1)                // command write
	out HEX(64), al
	call empty_8042
	mov al, HEX(DF)                // A20 on
	out	HEX(60), al
	call empty_8042
	call switch_to_prot
	.code32

	popa

	ret

	/*
	 * Disable the A20 address line
	 */
EXTERN(_DisableA20)
.code32

	pusha

	call switch_to_real
	.code16

	call	empty_8042
	mov al, HEX(D1)                // command write
	out HEX(64), al
	call empty_8042
	mov al, HEX(DD)                // A20 off
	out HEX(60), al
	call	empty_8042
	call	switch_to_prot
	.code32

	popa

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
#define FREELDR_BASE HEX(8000)
#define INITIAL_BASE HEX(200000)

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
	//ljmp	$PMODE_CS, $(mb1 + INITIAL_BASE - FREELDR_BASE)
	jmp far ptr PMODE_CS: (mb1 + INITIAL_BASE - FREELDR_BASE)
mb1:
	mov dx, PMODE_DS
	mov ds, dx
	mov es, dx

	/* Check for valid multiboot signature */
	cmp eax, MULTIBOOT_BOOTLOADER_MAGIC
	jne mbfail

	/* Store multiboot info in a safe place */
	mov esi, ebx
	mov edi, offset mb_info + INITIAL_BASE - FREELDR_BASE
	mov ecx, MB_INFO_SIZE
	rep movsb

	/* Save commandline */
	mov edx, [ebx + MB_INFO_FLAGS_OFFSET]
	test dword ptr [ebx + MB_INFO_FLAGS_OFFSET], MB_INFO_FLAG_COMMAND_LINE
	jz mb3
	mov esi, [ebx + MB_INFO_COMMAND_LINE_OFFSET]
	mov edi, offset cmdline + INITIAL_BASE - FREELDR_BASE
	mov ecx, CMDLINE_SIZE
mb2:	lodsb
	stosb
	test al, al
	jz mb3
	dec ecx
	jnz mb2
mb3:

	/* Copy to low mem */
	mov esi, INITIAL_BASE
	mov edi, FREELDR_BASE
	mov ecx, (offset __bss_end__ - FREELDR_BASE)
	add ecx, 3
	shr ecx, 2
	rep movsd

	/* Load the GDT and IDT */
	lgdt	gdtptr
	lidt	i386idtptr

	/* Clear prefetch queue & correct CS,
	 * jump to low mem */
	//ljmp	$PMODE_CS, $mb4
	jmp far ptr PMODE_CS:mb4
mb4:
	/* Reload segment selectors */
	mov dx, PMODE_DS
	mov ds, dx
	mov es, dx
	mov fs, dx
	mov gs, dx
	mov ss, dx
	mov esp, STACK32ADDR

	mov ebx, offset mb_info
	/* See if the boot device was passed in */
	mov edx, [ebx + MB_INFO_FLAGS_OFFSET]
	test edx, MB_INFO_FLAG_BOOT_DEVICE
	jz mb5
	/* Retrieve boot device info */
	mov eax, [ebx + MB_INFO_BOOT_DEVICE_OFFSET]
	shr eax, 16
	inc al
	mov byte ptr _FrldrBootPartition, al
	mov byte ptr _FrldrBootDrive, ah
	jmp mb6
mb5:	/* No boot device known, assume first partition of first harddisk */
	mov byte ptr _FrldrBootDrive, HEX(80)
	mov byte ptr _FrldrBootPartition, 1
mb6:
	/* Check for command line */
	mov eax, offset cmdline
	test dword ptr [ebx + MB_INFO_FLAGS_OFFSET], MB_INFO_FLAG_COMMAND_LINE
	jnz mb7
	xor eax, eax
mb7:

	/* GO! */
	push eax
	call	_BootMain

mbfail:
	call	switch_to_real
	.code16
	int	0x19
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


    .align 4	/* force 4-byte alignment */
gdt:
	/* NULL Descriptor */
	.word HEX(0000)
	.word HEX(0000)
	.word HEX(0000)
	.word HEX(0000)

	/* 32-bit flat CS */
	.word HEX(FFFF)
	.word HEX(0000)
	.word HEX(9A00)
	.word HEX(00CF)

	/* 32-bit flat DS */
	.word HEX(FFFF)
	.word HEX(0000)
	.word HEX(9200)
	.word HEX(00CF)

	/* 16-bit real mode CS */
	.word HEX(FFFF)
	.word HEX(0000)
	.word HEX(9E00)
	.word HEX(0000)

	/* 16-bit real mode DS */
	.word HEX(FFFF)
	.word HEX(0000)
	.word HEX(9200)
	.word HEX(0000)

/* GDT table pointer */
gdtptr:
	.word HEX(27)		/* Limit */
	.long gdt			/* Base Address */

/* Initial GDT table pointer for multiboot */
gdtptrhigh:
	.word HEX(27)		/* Limit */
	.long gdt + INITIAL_BASE - FREELDR_BASE	/* Base Address */

/* Real-mode IDT pointer */
rmode_idtptr:
	.word HEX(3ff)		/* Limit */
	.long 0			/* Base Address */

mb_info:
	.fill	MB_INFO_SIZE, 1, 0

cmdline:
	.fill	CMDLINE_SIZE, 1, 0

EXTERN(_FrldrBootDrive)
    .long 0

EXTERN(_FrldrBootPartition)
    .long 0
