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

#include <freeldr.h>
#include <mm.h>
#include <rtl.h>
#include <debug.h>
#include <ui.h>


//
// Define this to 1 if you want the entire contents
// of the memory allocation bitmap displayed
// when a chunk is allocated or freed
//
#define DUMP_MEM_MAP_ON_VERIFY	0

#define MEM_BLOCK_SIZE	256

typedef struct
{
	BOOL	MemBlockAllocated;		// Is this block allocated or free
	ULONG	BlocksAllocated;		// Block length, in multiples of 256 bytes
} MEMBLOCK, *PMEMBLOCK;

PVOID		HeapBaseAddress = NULL;
ULONG		HeapLengthInBytes = 0;
ULONG		HeapMemBlockCount = 0;
PMEMBLOCK	HeapMemBlockArray = NULL;

#ifdef DEBUG
ULONG		AllocationCount = 0;

VOID		VerifyHeap(VOID);
VOID		DumpMemoryAllocMap(VOID);
VOID		IncrementAllocationCount(VOID);
VOID		DecrementAllocationCount(VOID);
VOID		MemAllocTest(VOID);
#endif DEBUG

VOID InitMemoryManager(PVOID BaseAddress, ULONG Length)
{
	ULONG	MemBlocks;

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

PVOID AllocateMemory(ULONG NumberOfBytes)
{
	ULONG	BlocksNeeded;
	ULONG	Idx;
	ULONG	NumFree;
	PVOID	MemPointer;

	if (NumberOfBytes == 0)
	{
		DbgPrint((DPRINT_MEMORY, "AllocateMemory() called for 0 bytes. Returning NULL.\n"));
		return NULL;
	}

	// Find out how many blocks it will take to
	// satisfy this allocation
	BlocksNeeded = ROUND_UP(NumberOfBytes, MEM_BLOCK_SIZE) / MEM_BLOCK_SIZE;

	// Now loop through our array of blocks and
	// see if we have enough space
	for (Idx=0,NumFree=0; Idx<HeapMemBlockCount; Idx++)
	{
		// Check this block and see if it is already allocated
		// If so reset our counter and continue the loop
		if (HeapMemBlockArray[Idx].MemBlockAllocated)
		{
			NumFree = 0;
			continue;
		}
		else
		{
			// It is free memory so lets increment our count
			NumFree++;
		}

		// If we have found enough blocks to satisfy the request
		// then we're done searching
		if (NumFree >= BlocksNeeded)
		{
			break;
		}
	}
	Idx++;

	// If we don't have enough available mem
	// then return NULL
	if (NumFree < BlocksNeeded)
	{
		DbgPrint((DPRINT_MEMORY, "Memory allocation failed. Not enough free memory to allocate %d bytes. AllocationCount: %d\n", NumberOfBytes, AllocationCount));
		MessageBox("Memory allocation failed: out of memory.");
		return NULL;
	}

	// Subtract the block count from Idx and we have
	// the start block of the memory
	Idx -= NumFree;

	// Now we know which block to give them
	MemPointer = HeapBaseAddress + (Idx * MEM_BLOCK_SIZE);

	// Now loop through and mark all the blocks as allocated
	for (NumFree=0; NumFree<BlocksNeeded; NumFree++)
	{
		HeapMemBlockArray[Idx + NumFree].MemBlockAllocated = TRUE;
		HeapMemBlockArray[Idx + NumFree].BlocksAllocated = NumFree ? 0 : BlocksNeeded; // Mark only the first block with the count
	}

#ifdef DEBUG
	IncrementAllocationCount();
	DbgPrint((DPRINT_MEMORY, "Allocated %d bytes (%d blocks) of memory starting at block %d. AllocCount: %d\n", NumberOfBytes, BlocksNeeded, Idx, AllocationCount));
	VerifyHeap();
#endif DEBUG

	// Now return the pointer
	return MemPointer;
}

VOID FreeMemory(PVOID MemBlock)
{
	ULONG	BlockNumber;
	ULONG	BlockCount;
	ULONG	Idx;

#ifdef DEBUG

	// Make sure we didn't get a bogus pointer
	if ((MemBlock < HeapBaseAddress) || (MemBlock > (HeapBaseAddress + HeapLengthInBytes)))
	{
		BugCheck((DPRINT_MEMORY, "Bogus memory pointer (0x%x) passed to FreeMemory()\n", MemBlock));
	}
#endif DEBUG

	// Find out the block number if the first
	// block of memory they allocated
	BlockNumber = (MemBlock - HeapBaseAddress) / MEM_BLOCK_SIZE;
	BlockCount = HeapMemBlockArray[BlockNumber].BlocksAllocated;

#ifdef DEBUG
	// Make sure we didn't get a bogus pointer
	if ((BlockCount < 1) || (BlockCount > HeapMemBlockCount))
	{
		BugCheck((DPRINT_MEMORY, "Invalid block count in heap page header. HeapMemBlockArray[BlockNumber].BlocksAllocated = %d\n", HeapMemBlockArray[BlockNumber].BlocksAllocated));
	}
#endif

	// Loop through our array and mark all the
	// blocks as free
	for (Idx=BlockNumber; Idx<(BlockNumber + BlockCount); Idx++)
	{
		HeapMemBlockArray[Idx].MemBlockAllocated = FALSE;
		HeapMemBlockArray[Idx].BlocksAllocated = 0;
	}

#ifdef DEBUG
	DecrementAllocationCount();
	DbgPrint((DPRINT_MEMORY, "Freed %d blocks of memory starting at block %d. AllocationCount: %d\n", BlockCount, BlockNumber, AllocationCount));
	VerifyHeap();
#endif DEBUG
}

#ifdef DEBUG
VOID VerifyHeap(VOID)
{
	ULONG	Idx;
	ULONG	Idx2;
	ULONG	Count;

	if (DUMP_MEM_MAP_ON_VERIFY)
	{
		DumpMemoryAllocMap();
	}

	// Loop through the array and verify that
	// everything is kosher
	for (Idx=0; Idx<HeapMemBlockCount; Idx++)
	{
		// Check if this block is allocation
		if (HeapMemBlockArray[Idx].MemBlockAllocated)
		{
			// This is the first block in the run so it
			// had better have a length that is within range
			if ((HeapMemBlockArray[Idx].BlocksAllocated < 1) || (HeapMemBlockArray[Idx].BlocksAllocated > (HeapMemBlockCount - Idx)))
			{
				BugCheck((DPRINT_MEMORY, "Allocation length out of range in heap table. HeapMemBlockArray[Idx].BlocksAllocated = %d\n", HeapMemBlockArray[Idx].BlocksAllocated));
			}

			// Now go through and verify that the rest of
			// this run has the blocks marked allocated
			// with a length of zero but don't check the
			// first one because we already did
			Count = HeapMemBlockArray[Idx].BlocksAllocated;
			for (Idx2=1; Idx2<Count; Idx2++)
			{
				// Make sure it's allocated
				if (HeapMemBlockArray[Idx + Idx2].MemBlockAllocated != TRUE)
				{
					BugCheck((DPRINT_MEMORY, "Heap table indicates hole in memory allocation. HeapMemBlockArray[Idx + Idx2].MemBlockAllocated != TRUE\n"));
				}

				// Make sure the length is zero
				if (HeapMemBlockArray[Idx + Idx2].BlocksAllocated != 0)
				{
					BugCheck((DPRINT_MEMORY, "Allocation chain has non-zero value in non-first block in heap table. HeapMemBlockArray[Idx + Idx2].BlocksAllocated != 0\n"));
				}
			}

			// Move on to the next run
			Idx += (Count - 1);
		}
		else
		{
			// Nope, not allocated so make sure the length is zero
			if (HeapMemBlockArray[Idx].BlocksAllocated != 0)
			{
				BugCheck((DPRINT_MEMORY, "Free block is start of memory allocation. HeapMemBlockArray[Idx].BlocksAllocated != 0\n"));
			}
		}
	}
}

VOID DumpMemoryAllocMap(VOID)
{
	ULONG	Idx;

	DbgPrint((DPRINT_MEMORY, "----------- Memory Allocation Bitmap -----------\n"));

	for (Idx=0; Idx<HeapMemBlockCount; Idx++)
	{
		if ((Idx % 32) == 0)
		{
			DbgPrint((DPRINT_MEMORY, "\n%x:\t", (Idx * 256)));
		}
		else if ((Idx % 4) == 0)
		{
			DbgPrint((DPRINT_MEMORY, " "));
		}

		if (HeapMemBlockArray[Idx].MemBlockAllocated)
		{
			DbgPrint((DPRINT_MEMORY, "X"));
		}
		else
		{
			DbgPrint((DPRINT_MEMORY, "*"));
		}
	}

	DbgPrint((DPRINT_MEMORY, "\n"));
}

VOID IncrementAllocationCount(VOID)
{
	AllocationCount++;
}

VOID DecrementAllocationCount(VOID)
{
	AllocationCount--;
}

VOID MemAllocTest(VOID)
{
	PVOID	MemPtr1;
	PVOID	MemPtr2;
	PVOID	MemPtr3;
	PVOID	MemPtr4;
	PVOID	MemPtr5;

	MemPtr1 = AllocateMemory(4096);
	printf("MemPtr1: 0x%x\n", (int)MemPtr1);
	getch();
	MemPtr2 = AllocateMemory(4096);
	printf("MemPtr2: 0x%x\n", (int)MemPtr2);
	getch();
	MemPtr3 = AllocateMemory(4096);
	printf("MemPtr3: 0x%x\n", (int)MemPtr3);
	DumpMemoryAllocMap();
	VerifyHeap();
	getch();

	FreeMemory(MemPtr2);
	getch();

	MemPtr4 = AllocateMemory(2048);
	printf("MemPtr4: 0x%x\n", (int)MemPtr4);
	getch();
	MemPtr5 = AllocateMemory(4096);
	printf("MemPtr5: 0x%x\n", (int)MemPtr5);
	getch();
}
#endif DEBUG
