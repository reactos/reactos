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
	call	_MachVideoPrepareForReactOS

	call	_multi_boot

	// Should never get here
	cli
bootloop:
	hlt
	jmp	bootloop


	/*
	 * After you have setup the _mb_header and _mb_info structures
	 * then call this routine to transfer control to the kernel.
	 */
EXTERN(_multi_boot)

	cli

	/*
	 * Load the absolute address of the multiboot information structure
	 */
	movl	$_mb_info,%ebx

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
	.rept	(32 * /*sizeof(memory_map_t)*/24)
	.byte	0
	.endr

EXTERN(_multiboot_kernel_cmdline)
	.rept	255
	.byte	0
	.endr
