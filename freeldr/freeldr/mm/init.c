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

#include <freeldr.h>
#include <arch.h>
#include <mm.h>
#include "mem.h"
#include <rtl.h>
#include <debug.h>
#include <ui.h>


ULONG		RealFreeLoaderModuleEnd = 0;

PVOID		HeapBaseAddress = NULL;
ULONG		HeapLengthInBytes = 0;
ULONG		HeapMemBlockCount = 0;
PMEMBLOCK	HeapMemBlockArray = NULL;

VOID InitMemoryManager(VOID)
{
	ULONG	MemBlocks;
	ULONG	BaseAddress;
	ULONG	Length;

	// Round up to the next page of memory
	RealFreeLoaderModuleEnd = ((FreeLoaderModuleEnd + 4095) / 4096) * 4096;
	BaseAddress = RealFreeLoaderModuleEnd;
	Length = (MAXLOWMEMADDR - RealFreeLoaderModuleEnd);

	DbgPrint((DPRINT_MEMORY, "Initializing Memory Manager.\n"));
	DbgPrint((DPRINT_MEMORY, "Conventional memory size: %d KB\n", GetConventionalMemorySize()));
	DbgPrint((DPRINT_MEMORY, "Low memory map:\n"));
	DbgPrint((DPRINT_MEMORY, "00000\t1000\tReserved\n"));
	DbgPrint((DPRINT_MEMORY, "01000\t6000\t16-bit stack\n"));
	DbgPrint((DPRINT_MEMORY, "07000\t1000\tUnused\n"));
	DbgPrint((DPRINT_MEMORY, "08000\t%x\tFreeLoader program code\n", (RealFreeLoaderModuleEnd - 0x8000)));
	DbgPrint((DPRINT_MEMORY, "%x\t%x\tLow memory heap\n", BaseAddress, Length));
	DbgPrint((DPRINT_MEMORY, "78000\t8000\t32-bit stack\n"));
	DbgPrint((DPRINT_MEMORY, "80000\t10000\tFile system read buffer\n"));
	DbgPrint((DPRINT_MEMORY, "90000\t10000\tDisk read buffer\n"));
	DbgPrint((DPRINT_MEMORY, "A0000\t60000\tReserved\n"));

	// Calculate how many memory blocks we have
	MemBlocks = (Length / MEM_BLOCK_SIZE);

	// Adjust the heap length so we can reserve
	// enough storage space for the MEMBLOCK array
	Length -= (MemBlocks * sizeof(MEMBLOCK));

	// Initialize our tracking variables
	HeapBaseAddress = BaseAddress;
	HeapLengthInBytes = Length;
	HeapMemBlockCount = (HeapLengthInBytes / MEM_BLOCK_SIZE);
	HeapMemBlockArray = (PMEMBLOCK)(HeapBaseAddress + HeapLengthInBytes);

	// Clear the memory
	RtlZeroMemory(HeapBaseAddress, HeapLengthInBytes);
	RtlZeroMemory(HeapMemBlockArray, (HeapMemBlockCount * sizeof(MEMBLOCK)));

#ifdef DEBUG
	DbgPrint((DPRINT_MEMORY, "Memory Manager initialized. BaseAddress = 0x%x Length = 0x%x. %d blocks in heap.\n", BaseAddress, Length, HeapMemBlockCount));
	//MemAllocTest();
#endif
}
