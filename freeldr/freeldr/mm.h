/*
 *  FreeLoader
 *  Copyright (C) 2001  Brian Palmer  <brianp@sginet.com>
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


#ifndef __MEMORY_H
#define __MEMORY_H

#include <multiboot.h>


VOID	InitMemoryManager(PVOID BaseAddress, ULONG Length);

PVOID	AllocateMemory(ULONG NumberOfBytes);
VOID	FreeMemory(PVOID MemBlock);

// These functions are implemented in mem.S
int		GetExtendedMemorySize(void);				// Returns extended memory size in KB
int		GetConventionalMemorySize(void);			// Returns conventional memory size in KB
int		GetBiosMemoryMap(memory_map_t *mem_map);	// Fills mem_map structure with BIOS memory map and returns length of memory map


#endif // defined __MEMORY_H
