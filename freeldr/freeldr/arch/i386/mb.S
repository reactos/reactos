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

	/*
     * Here we assume the kernel is loaded at 1mb
     * This boots the kernel
	 */
	.code32
EXTERN(_boot_reactos)
	call	switch_to_real
	.code16

    /* Save cursor position */
	movw    $3,%ax          //! Reset video mode
	int     $0x10


	movb    $10,%bl
	movb    $12,%ah
	int     $0x10

	movw    $0x1112,%ax		// Use 8x8 font
	xorb	%bl,%bl
	int     $0x10
	movw    $0x1200,%ax		// Use alternate print screen
	movb    $0x20,%bl
	int     $0x10
	movb    $1,%ah			// Define cursor (scan lines 6 to 7)
	movw    $0x0607,%cx
	int     $0x10

	movb    $1,%ah
	movw    $0x600,%cx
	int     $0x10

	movb	$6,%ah			// Scroll active page up
	movb	$0x32,%al		// Clear 25 lines
	movw	$0,%cx			// Upper left of scroll
	movw	$0x314F,%dx		// Lower right of scroll
	movb	$(1*0x10+1),%bh	// Use normal attribute on blanked line
	int		$0x10			// Video-IO


	movw	$0,%dx
	movb	$0,%dh

	movb	$2,%ah
	movb	$0,%bh
	int		$0x10

	movw	$0,%dx
	movb	$0,%dh

	movb	$2,%ah
	movb	$0,%bh
	int		$0x10

	call	_multi_boot

	// Should never get here
	cli
	hlt


	/*
	 * After you have setup the _mb_header and _mb_info structures
	 * then call this routine to transfer control to the kernel.
	 * This routine must be entered in 16-bit mode.
	 */
	.code16
EXTERN(_multi_boot)

	cli

	/*
	 * Setup various variables
	 */
	movw	%ds,%bx
	movzwl	%bx,%eax
	shll	$4,%eax
	addl	%eax,kernel_gdtbase

	/*
	 * Load the absolute address of the multiboot information structure
	 */
	movl	$_mb_info,%ebx

	/*
	 * load gdt
	 */
	lgdt	kernel_gdtptr

	/*       
	 * Enter pmode and clear prefetch queue
	 */
	movl	%cr0,%eax
	orl		$0x10001,%eax
	movl	%eax,%cr0
	jmp		next
next:
	/*
	 * NOTE: This must be position independant (no references to
	 * non absolute variables)
	 */

	/*
	 * Initalize segment registers
	 */
	movw	$KERNEL_DS,%ax
	movw	%ax,%ds
	movw	%ax,%ss
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs

	/*
	 * Initalize eflags
	 */
	pushl	$0
	popfl	

	/*
	 * Load the multiboot magic value into eax
	 */	
	movl	$0x2badb002,%eax

	/*
	 * Jump to start of 32 bit code at 0xc0000000 + 0x1000
	 */

	pushl	$KERNEL_CS
	pushl	_mb_entry_addr
	lretl

	//ljmpl	$KERNEL_CS,$(0x0200000+0x1000)
	//ljmpl	$KERNEL_CS,(_mb_entry_addr)
	//ljmpl	$KERNEL_CS,$(KERNEL_BASE+0x1000)


	.p2align	2	/* force 4-byte alignment */
kernel_gdt:
	.word 0               // Zero descriptor
	.word 0
	.word 0
	.word 0

	.word 0xffff          // Kernel code descriptor 
	.word 0x0000          // 
	.word 0x9a00          // base 0h limit 4gb
	.word 0x00cf
                       
	.word 0xffff          // Kernel data descriptor
	.word 0x0000          // 
	.word 0x9200          // base 0h limit 4gb
	.word 0x00cf

kernel_gdtptr:
	.word	(3*8)-1			/* Limit */
kernel_gdtbase:
	.long	kernel_gdt				/* Base Address */


EXTERN(_mb_header)
_mb_magic:
	.long	0		// unsigned long magic;
_mb_flags:
  	.long	0		// unsigned long flags;
_mb_checksum:
  	.long	0		// unsigned long checksum;
_mb_header_addr:
  	.long	0		// unsigned long header_addr;
_mb_load_addr:
  	.long	0		// unsigned long load_addr;
_mb_load_end_addr:
  	.long	0		// unsigned long load_end_addr;
_mb_bss_end_addr:
  	.long	0		// unsigned long bss_end_addr;
_mb_entry_addr:
  	.long	0		// unsigned long entry_addr;

	//
	// Boot information structure
	//

EXTERN(_mb_info)
_multiboot_flags:
	.long	0
_multiboot_mem_lower:
	.long	0
_multiboot_mem_upper:
	.long	0
_multiboot_boot_device:
	.long	0
_multiboot_cmdline:	
	.long	0
_multiboot_mods_count:
	.long	0
_multiboot_mods_addr:
	.long	0
_multiboot_syms:
	.rept	12
	.byte	0
	.endr
_multiboot_mmap_length:
	.long	0
_multiboot_mmap_addr:
	.long	0
_multiboot_drives_count:
	.long	0
_multiboot_drives_addr:
	.long	0
_multiboot_config_table:
	.long	0
_multiboot_boot_loader_name:
	.long	0
_multiboot_apm_table:
	.long	0

EXTERN(_multiboot_modules)
	.rept	(64 * /*multiboot_module_size*/ 16)
	.byte	0
	.endr
EXTERN(_multiboot_module_strings)
	.rept	(64*256)
	.byte	0
	.endr

EXTERN(_multiboot_memory_map_descriptor_size)
	.long	0

EXTERN(_multiboot_memory_map)
	.rept	(64 * /*sizeof(memory_map_t)*/24)
	.byte	0
	.endr

EXTERN(_multiboot_kernel_cmdline)
	.rept	255
	.byte	0
	.endr
