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


#ifndef __MEMORY_H
#define __MEMORY_H


#define	MEMTYPE_USABLE			0x01
#define	MEMTYPE_RESERVED		0x02
#define MEMTYPE_ACPI_RECLAIM	0x03
#define MEMTYPE_ACPI_NVS		0x04

typedef struct
{
	ULONG	BaseAddressLow;
	ULONG	BaseAddressHigh;
	ULONG	LengthLow;
	ULONG	LengthHigh;
	ULONG	Type;
	ULONG	Reserved;
} PACKED BIOS_MEMORY_MAP, *PBIOS_MEMORY_MAP;


ULONG	GetSystemMemorySize(VOID);								// Returns the amount of total memory in the system

// These functions are implemented in mem.S
ULONG	GetExtendedMemorySize(VOID);							// Returns extended memory size in KB
ULONG	GetConventionalMemorySize(VOID);						// Returns conventional memory size in KB
ULONG	GetBiosMemoryMap(BIOS_MEMORY_MAP BiosMemoryMap[32]);	// Fills mem_map structure with BIOS memory map and returns length of memory map











//BOOL	MmInitializeMemoryManager(ULONG LowMemoryStart, ULONG LowMemoryLength);
BOOL	MmInitializeMemoryManager(VOID);
PVOID	MmAllocateMemory(ULONG MemorySize);
VOID	MmFreeMemory(PVOID MemoryPointer);
//PVOID	MmAllocateLowMemory(ULONG MemorySize);
//VOID	MmFreeLowMemory(PVOID MemoryPointer);
//PVOID	MmAllocateMemoryFrom1Mb(ULONG MemorySize);

#endif // defined __MEMORY_H
