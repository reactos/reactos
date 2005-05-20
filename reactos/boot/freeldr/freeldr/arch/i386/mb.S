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
    .globl PageDirectoryStart
    
    .globl startup_pagedirectory
    .globl lowmem_pagetable
    .globl kernel_pagetable
    .globl hyperspace_pagetable
    .globl apic_pagetable
    .globl kpcr_pagetable

    .globl startup_pagedirectorytable_pae
    .globl startup_pagedirectory_pae
    .globl lowmem_pagetable_pae
    .globl kernel_pagetable_pae
    .globl hyperspace_pagetable_pae
    .globl apic_pagetable_pae
    .globl kpcr_pagetable_pae
    
    .globl PageDirectoryEnd

	//
	// Boot information structure
	//

EXTERN(multiboot_memory_map_descriptor_size)
	.long	0

EXTERN(multiboot_memory_map)
	.rept	(32 * /*sizeof(memory_map_t)*/24)
	.byte	0
	.endr

.bss
	.align 4096
PageDirectoryStart:
startup_pagedirectory:
	.fill 4096, 1, 0

lowmem_pagetable:
	.fill 4096, 1, 0

kernel_pagetable:
	.fill 2*4096, 1, 0

hyperspace_pagetable:
	.fill 4096, 1, 0
	
apic_pagetable:
	.fill 4096, 1, 0

kpcr_pagetable:
	.fill 4096, 1, 0   

startup_pagedirectory_pae:
	.fill 4 * 4096, 1, 0

lowmem_pagetable_pae:
	.fill 2 * 4096, 1, 0

kernel_pagetable_pae:
	.fill 3*4096, 1, 0

hyperspace_pagetable_pae:
	.fill 2*4096, 1, 0
	
apic_pagetable_pae:
	.fill 2*4096, 1, 0

kpcr_pagetable_pae:
	.fill 4*4096, 1, 0   
	
startup_pagedirectorytable_pae:
	.fill 4096, 1, 0	
PageDirectoryEnd:
