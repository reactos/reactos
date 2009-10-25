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
	.code64
    .globl _PageDirectoryStart
    
    .globl _pml4_startup
    .globl _pdp_startup
    .globl _pd_startup

    .globl _PageDirectoryEnd

	//
	// Boot information structure
	//

EXTERN(_reactos_memory_map_descriptor_size)
	.long	0

EXTERN(_reactos_memory_map)
	.rept	(32 * /*sizeof(memory_map_t)*/24)
	.byte	0
	.endr

.bss
_PageDirectoryStart:
_pml4_startup:
	.fill 4096, 1, 0

_pdp_startup:
	.fill 4096, 1, 0

_pd_startup:
	.fill 4096, 1, 0

_PageDirectoryEnd:
