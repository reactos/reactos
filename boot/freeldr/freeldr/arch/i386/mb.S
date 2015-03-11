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

#include <asm.inc>
#include <arch/pc/x86common.h>
#include <multiboot.h>

.code32
PUBLIC _PageDirectoryStart

PUBLIC _startup_pagedirectory
PUBLIC _lowmem_pagetable
PUBLIC _kernel_pagetable
PUBLIC _apic_pagetable
PUBLIC _PageDirectoryEnd

#ifndef _USE_ML
.bss
#endif
_PageDirectoryStart:
_startup_pagedirectory:
    .fill 4096, 1, 0

_lowmem_pagetable:
    .fill 4096, 1, 0

_kernel_pagetable:
    .fill 2*4096, 1, 0

_apic_pagetable:
    .fill 4096, 1, 0

_PageDirectoryEnd:

END
