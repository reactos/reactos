/*
 * PROJECT:		 ReactOS Boot Loader
 * LICENSE:		 BSD - See COPYING.ARM in the top level directory
 * FILE:		 drivers/sac/driver/memory.c
 * PURPOSE:		 Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS:	 ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "sacdrv.h"

/* GLOBALS ********************************************************************/

LONG TotalFrees, TotalBytesFreed, TotalAllocations, TotalBytesAllocated;
KSPIN_LOCK MemoryLock;
PSAC_MEMORY_LIST GlobalMemoryList;

/* FUNCTIONS ******************************************************************/

BOOLEAN
InitializeMemoryManagement(VOID)
{
	PSAC_MEMORY_ENTRY Entry;

	SAC_DBG(SAC_DBG_ENTRY_EXIT, "Entering\n");

	GlobalMemoryList = ExAllocatePoolWithTagPriority(
		NonPagedPool,
		SAC_MEMORY_LIST_SIZE,
		INITIAL_BLOCK_TAG,
		HighPoolPriority);
	if (GlobalMemoryList)
	{
		KeInitializeSpinLock(&MemoryLock);

		GlobalMemoryList->Signature = GLOBAL_MEMORY_SIGNATURE;
		GlobalMemoryList->LocalDescriptor =
			(PSAC_MEMORY_ENTRY)(GlobalMemoryList + 1);
		GlobalMemoryList->Size = SAC_MEMORY_LIST_SIZE - sizeof(SAC_MEMORY_LIST);

		Entry = GlobalMemoryList->LocalDescriptor;
		Entry->Signature = LOCAL_MEMORY_SIGNATURE;
		Entry->Tag = FREE_POOL_TAG;
		Entry->Size = GlobalMemoryList->Size - sizeof(SAC_MEMORY_ENTRY);

		SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting with TRUE.\n");
		return TRUE;
	}

	SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting with FALSE. No pool.\n");
	return FALSE;
}

VOID
FreeMemoryManagement(
	VOID
	)
{
	PSAC_MEMORY_LIST Next;
	KIRQL OldIrql;

	SAC_DBG(SAC_DBG_ENTRY_EXIT, "Entering\n");

	KeAcquireSpinLock(&MemoryLock, &OldIrql);
	while (GlobalMemoryList)
	{
		ASSERT(GlobalMemoryList->Signature == GLOBAL_MEMORY_SIGNATURE);

		KeReleaseSpinLock(&MemoryLock, OldIrql);

		Next = GlobalMemoryList->Next;

		ExFreePoolWithTag(GlobalMemoryList, 0);

		KeAcquireSpinLock(&MemoryLock, &OldIrql);
		GlobalMemoryList = Next;
	}

	KeReleaseSpinLock(&MemoryLock, OldIrql);
	SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting\n");
}

PVOID
MyAllocatePool(
	IN SIZE_T PoolSize,
	IN ULONG Tag,
	IN PCHAR File,
	IN ULONG Line
	)
{
	KIRQL OldIrql;
	PSAC_MEMORY_LIST GlobalDescriptor, NewDescriptor;
	PSAC_MEMORY_ENTRY LocalDescriptor, NextDescriptor;
	ULONG GlobalSize, ActualSize;
	PVOID Buffer;

	ASSERT("Tag != FREE_POOL_TAG");

	SAC_DBG(SAC_DBG_MM, "Entering.\n");

	OldIrql = KfAcquireSpinLock(&MemoryLock);
	PoolSize = ALIGN_UP(PoolSize, ULONGLONG);

	GlobalDescriptor = GlobalMemoryList;
	KeAcquireSpinLock(&MemoryLock, &OldIrql);
	while (GlobalDescriptor)
	{
		ASSERT(GlobalMemoryList->Signature == GLOBAL_MEMORY_SIGNATURE);

		LocalDescriptor = GlobalDescriptor->LocalDescriptor;

		GlobalSize = GlobalDescriptor->Size;
		while (GlobalSize)
		{
			ASSERT(LocalDescriptor->Signature == LOCAL_MEMORY_SIGNATURE);

			if ((LocalDescriptor->Tag == FREE_POOL_TAG) &&
				(LocalDescriptor->Size >= PoolSize))
			{
				break;
			}

			GlobalSize -= (LocalDescriptor->Size + sizeof(SAC_MEMORY_ENTRY));

			LocalDescriptor =
				(PSAC_MEMORY_ENTRY)((ULONG_PTR)LocalDescriptor +
				LocalDescriptor->Size +
				sizeof(SAC_MEMORY_ENTRY));
		}

		GlobalDescriptor = GlobalDescriptor->Next;
	}

	if (!GlobalDescriptor)
	{
		KeReleaseSpinLock(&MemoryLock, OldIrql);

		ActualSize = min(
			PAGE_SIZE,
			PoolSize + sizeof(SAC_MEMORY_ENTRY) + sizeof(SAC_MEMORY_LIST));

		SAC_DBG(SAC_DBG_MM, "Allocating new space.\n");

		NewDescriptor = ExAllocatePoolWithTagPriority(
			0,
			ActualSize,
			ALLOC_BLOCK_TAG,
			HighPoolPriority);
		if (!NewDescriptor)
		{
			SAC_DBG(SAC_DBG_MM, "No more memory, returning NULL.\n");
			return NULL;
		}

		KeAcquireSpinLock(&MemoryLock, &OldIrql);

		NewDescriptor->Signature = GLOBAL_MEMORY_SIGNATURE;
		NewDescriptor->LocalDescriptor = (PSAC_MEMORY_ENTRY)(NewDescriptor + 1);
		NewDescriptor->Size = ActualSize - 16;
		NewDescriptor->Next = GlobalMemoryList;

		GlobalMemoryList = NewDescriptor;

		LocalDescriptor = NewDescriptor->LocalDescriptor;
		LocalDescriptor->Signature = LOCAL_MEMORY_SIGNATURE;
		LocalDescriptor->Tag = FREE_POOL_TAG;
		LocalDescriptor->Size =
			GlobalMemoryList->Size - sizeof(SAC_MEMORY_ENTRY);
	}

	SAC_DBG(SAC_DBG_MM, "Found a good sized block.\n");
	ASSERT(LocalDescriptor->Tag == FREE_POOL_TAG);
	ASSERT(LocalDescriptor->Signature == LOCAL_MEMORY_SIGNATURE);

	if (LocalDescriptor->Size > (PoolSize + sizeof(SAC_MEMORY_ENTRY)))
	{
		NextDescriptor =
			(PSAC_MEMORY_ENTRY)((ULONG_PTR)LocalDescriptor +
			PoolSize +
			sizeof(SAC_MEMORY_ENTRY));
		if (NextDescriptor->Tag == FREE_POOL_TAG)
		{
			NextDescriptor->Tag = FREE_POOL_TAG;
			NextDescriptor->Signature = LOCAL_MEMORY_SIGNATURE;
			NextDescriptor->Size =
				(LocalDescriptor->Size - PoolSize - sizeof(SAC_MEMORY_ENTRY));

			LocalDescriptor->Size = PoolSize;
		}
	}

	LocalDescriptor->Tag = Tag;
	KeReleaseSpinLock(&MemoryLock, OldIrql);

	InterlockedIncrement(&TotalAllocations);
	InterlockedExchangeAdd(&TotalBytesAllocated, LocalDescriptor->Size);
	SAC_DBG(1, "Returning block 0x%X.\n", LocalDescriptor);

	Buffer = LocalDescriptor + 1;
	RtlZeroMemory(Buffer, PoolSize);
	return Buffer;
}

VOID
MyFreePool(
	IN PVOID *Block
	)
{
	PSAC_MEMORY_ENTRY LocalDescriptor, NextDescriptor;
	PSAC_MEMORY_ENTRY ThisDescriptor, FoundDescriptor;
	ULONG GlobalSize, LocalSize;
	PSAC_MEMORY_LIST GlobalDescriptor;
	KIRQL OldIrql;

	LocalDescriptor = (PVOID)((ULONG_PTR)(*Block) - sizeof(SAC_MEMORY_ENTRY));

	SAC_DBG(SAC_DBG_MM, "Entering with block 0x%X.\n", LocalDescriptor);

	ASSERT(LocalDescriptor->Size > 0);
	ASSERT(LocalDescriptor->Signature == LOCAL_MEMORY_SIGNATURE);

	InterlockedIncrement(&TotalFrees);

	InterlockedExchangeAdd(&TotalBytesFreed, LocalDescriptor->Size);

	GlobalDescriptor = GlobalMemoryList;
	KeAcquireSpinLock(&MemoryLock, &OldIrql);
	while (GlobalDescriptor)
	{
		ASSERT(GlobalMemoryList->Signature == GLOBAL_MEMORY_SIGNATURE);

		FoundDescriptor = NULL;

		ThisDescriptor = GlobalDescriptor->LocalDescriptor;

		GlobalSize = GlobalDescriptor->Size;
		while (GlobalSize)
		{
			ASSERT(ThisDescriptor->Signature == LOCAL_MEMORY_SIGNATURE);

			if (ThisDescriptor == LocalDescriptor) break;

			GlobalSize -= (ThisDescriptor->Size + sizeof(SAC_MEMORY_ENTRY));

			ThisDescriptor =
				(PSAC_MEMORY_ENTRY)((ULONG_PTR)ThisDescriptor +
				ThisDescriptor->Size +
				sizeof(SAC_MEMORY_ENTRY));
		}

		if (ThisDescriptor == LocalDescriptor) break;

		GlobalDescriptor = GlobalDescriptor->Next;
	}

	if (!GlobalDescriptor)
	{
		KeReleaseSpinLock(&MemoryLock, OldIrql);
		SAC_DBG(SAC_DBG_MM, "Could not find block.\n");
		return;
	}

	ASSERT(ThisDescriptor->Signature == LOCAL_MEMORY_SIGNATURE);

	if (LocalDescriptor->Tag == FREE_POOL_TAG)
	{
		KeReleaseSpinLock(&MemoryLock, OldIrql);
		SAC_DBG(SAC_DBG_MM, "Attempted to free something twice.\n");
		return;
	}

	LocalSize = LocalDescriptor->Size;
	LocalDescriptor->Tag = FREE_POOL_TAG;

	if (GlobalSize > (LocalSize + sizeof(SAC_MEMORY_ENTRY)))
	{
		NextDescriptor =
			(PSAC_MEMORY_ENTRY)((ULONG_PTR)LocalDescriptor +
			LocalSize +
			sizeof(SAC_MEMORY_ENTRY));
		if (NextDescriptor->Tag == FREE_POOL_TAG)
		{
			NextDescriptor->Tag = 0;
			NextDescriptor->Signature = 0;

			LocalDescriptor->Size +=
				(NextDescriptor->Size + sizeof(SAC_MEMORY_ENTRY));
		}
	}

	if ((FoundDescriptor) && (FoundDescriptor->Tag == FREE_POOL_TAG))
	{
		LocalDescriptor->Signature = 0;
		LocalDescriptor->Tag = 0;

		FoundDescriptor->Size +=
			(LocalDescriptor->Size + sizeof(SAC_MEMORY_ENTRY));
	}

	KeReleaseSpinLock(&MemoryLock, OldIrql);
	*Block = NULL;

	SAC_DBG(SAC_DBG_MM, "exiting.\n");
	return;
}
