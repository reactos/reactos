/* $Id: ppool.c,v 1.10 2002/09/07 15:13:00 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/ppool.c
 * PURPOSE:         Implements the paged pool
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* GLOBALS *******************************************************************/

typedef struct _MM_PPOOL_FREE_BLOCK_HEADER
{
  ULONG Size;
  struct _MM_PPOOL_FREE_BLOCK_HEADER* NextFree;
} MM_PPOOL_FREE_BLOCK_HEADER, *PMM_PPOOL_FREE_BLOCK_HEADER;

typedef struct _MM_PPOOL_USED_BLOCK_HEADER
{
  ULONG Size;
} MM_PPOOL_USED_BLOCK_HEADER, *PMM_PPOOL_USED_BLOCK_HEADER;

PVOID MmPagedPoolBase;
ULONG MmPagedPoolSize;
static FAST_MUTEX MmPagedPoolLock;
static PMM_PPOOL_FREE_BLOCK_HEADER MmPagedPoolFirstFreeBlock;

/* FUNCTIONS *****************************************************************/

VOID MmInitializePagedPool(VOID)
{
  MmPagedPoolFirstFreeBlock = (PMM_PPOOL_FREE_BLOCK_HEADER)MmPagedPoolBase;
  /*
   * We are still at a high IRQL level at this point so explicitly commit
   * the first page of the paged pool before writing the first block header.
   */
  MmCommitPagedPoolAddress((PVOID)MmPagedPoolFirstFreeBlock);
  MmPagedPoolFirstFreeBlock->Size = MmPagedPoolSize;
  MmPagedPoolFirstFreeBlock->NextFree = NULL;

  ExInitializeFastMutex(&MmPagedPoolLock);
}

/**********************************************************************
 * NAME							INTERNAL
 *	ExAllocatePagedPoolWithTag@12
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 */
PVOID STDCALL
ExAllocatePagedPoolWithTag (IN	POOL_TYPE	PoolType,
			    IN	ULONG		NumberOfBytes,
			    IN	ULONG		Tag)
{
  PMM_PPOOL_FREE_BLOCK_HEADER BestBlock;
  PMM_PPOOL_FREE_BLOCK_HEADER CurrentBlock;
  ULONG BlockSize;
  PMM_PPOOL_USED_BLOCK_HEADER NewBlock;
  PMM_PPOOL_FREE_BLOCK_HEADER NextBlock;
  PMM_PPOOL_FREE_BLOCK_HEADER PreviousBlock;
  PMM_PPOOL_FREE_BLOCK_HEADER BestPreviousBlock;
  PVOID BlockAddress;

  /*
   * Don't bother allocating anything for a zero-byte block.
   */
  if (NumberOfBytes == 0)
    {
      return(NULL);
    }

  /*
   * Calculate the total number of bytes we will need.
   */
  BlockSize = NumberOfBytes + sizeof(MM_PPOOL_USED_BLOCK_HEADER);
  if (BlockSize < sizeof(MM_PPOOL_FREE_BLOCK_HEADER))
  {
    /* At least we need the size of the free block header. */
    BlockSize = sizeof(MM_PPOOL_FREE_BLOCK_HEADER);
  }

  ExAcquireFastMutex(&MmPagedPoolLock);

  /*
   * Find the best fitting block.
   */
  PreviousBlock = NULL;
  BestPreviousBlock = BestBlock = NULL;
  CurrentBlock = MmPagedPoolFirstFreeBlock;
  while (CurrentBlock != NULL)
    {
      if (CurrentBlock->Size >= BlockSize &&
	  (BestBlock == NULL || 
	   (BestBlock->Size - BlockSize) > (CurrentBlock->Size - BlockSize)))
	{
	  BestPreviousBlock = PreviousBlock;
	  BestBlock = CurrentBlock;
	}

      PreviousBlock = CurrentBlock;
      CurrentBlock = CurrentBlock->NextFree;
    }

  /*
   * We didn't find anything suitable at all.
   */
  if (BestBlock == NULL)
    {
      ExReleaseFastMutex(&MmPagedPoolLock);
      return(NULL);
    }

  /*
   * Is there enough space to create a second block from the unused portion.
   */
  if ((BestBlock->Size - BlockSize) > sizeof(MM_PPOOL_FREE_BLOCK_HEADER))
    {
      ULONG NewSize = BestBlock->Size - BlockSize;

      /*
       * Create the new free block.
       */
      NextBlock = (PMM_PPOOL_FREE_BLOCK_HEADER)((PVOID)BestBlock + BlockSize);
      NextBlock->Size = NewSize;
      NextBlock->NextFree = BestBlock->NextFree;

      /*
       * Replace the old free block with it.
       */
      if (BestPreviousBlock == NULL)
	{
	  MmPagedPoolFirstFreeBlock = NextBlock;
	}
      else
	{
	  BestPreviousBlock->NextFree = NextBlock;
	}

      /*
       * Create the new used block header.
       */
      NewBlock = (PMM_PPOOL_USED_BLOCK_HEADER)BestBlock;
      NewBlock->Size = BlockSize;
    }
  else
    {
      ULONG NewSize = BestBlock->Size;

      /*
       * Remove the selected block from the list of free blocks.
       */
      if (BestPreviousBlock == NULL)
	{
	  MmPagedPoolFirstFreeBlock = BestBlock->NextFree;
	}
      else
	{
	  BestPreviousBlock->NextFree = BestBlock->NextFree;
	}

      /*
       * Set up the header of the new block
       */
      NewBlock = (PMM_PPOOL_USED_BLOCK_HEADER)BestBlock;
      NewBlock->Size = NewSize;
    }

  ExReleaseFastMutex(&MmPagedPoolLock);

  BlockAddress = (PVOID)NewBlock + sizeof(MM_PPOOL_USED_BLOCK_HEADER);

  memset(BlockAddress, 0, NumberOfBytes);

  return(BlockAddress);
}

VOID STDCALL
ExFreePagedPool(IN PVOID Block)
{
  PMM_PPOOL_FREE_BLOCK_HEADER PreviousBlock;
  PMM_PPOOL_USED_BLOCK_HEADER UsedBlock = 
    (PMM_PPOOL_USED_BLOCK_HEADER)(Block - sizeof(MM_PPOOL_USED_BLOCK_HEADER));
  ULONG UsedSize = UsedBlock->Size;
  PMM_PPOOL_FREE_BLOCK_HEADER FreeBlock = 
    (PMM_PPOOL_FREE_BLOCK_HEADER)UsedBlock;
  PMM_PPOOL_FREE_BLOCK_HEADER NextBlock;
  PMM_PPOOL_FREE_BLOCK_HEADER NextNextBlock;

  ExAcquireFastMutex(&MmPagedPoolLock);

  /*
   * Begin setting up the newly freed block's header.
   */
  FreeBlock->Size = UsedSize;

  /*
   * Find the blocks immediately before and after the newly freed block on the free list.
   */
  PreviousBlock = NULL;
  NextBlock = MmPagedPoolFirstFreeBlock;
  while (NextBlock != NULL && NextBlock < FreeBlock)
    {
      PreviousBlock = NextBlock;
      NextBlock = NextBlock->NextFree;
    }

  /*
   * Insert the freed block on the free list.
   */
  if (PreviousBlock == NULL)
    {
      FreeBlock->NextFree = MmPagedPoolFirstFreeBlock;
      MmPagedPoolFirstFreeBlock = FreeBlock;
    }
  else
    {
      PreviousBlock->NextFree = FreeBlock;
      FreeBlock->NextFree = NextBlock;
    }

  /*
   * If the next block is immediately adjacent to the newly freed one then
   * merge them.
   */
  if (NextBlock != NULL && 
      ((PVOID)FreeBlock + FreeBlock->Size) == (PVOID)NextBlock)
    {
      FreeBlock->Size = FreeBlock->Size + NextBlock->Size;
      FreeBlock->NextFree = NextBlock->NextFree;
      NextNextBlock = NextBlock->NextFree;
    }
  else
    {
      NextNextBlock = NextBlock;
    }

  /*
   * If the previous block is adjacent to the newly freed one then
   * merge them.
   */
  if (PreviousBlock != NULL && 
      ((PVOID)PreviousBlock + PreviousBlock->Size) == (PVOID)FreeBlock)
    {
      PreviousBlock->Size = PreviousBlock->Size + FreeBlock->Size;
      PreviousBlock->NextFree = NextNextBlock;
    }

  ExReleaseFastMutex(&MmPagedPoolLock);
}

/* EOF */
