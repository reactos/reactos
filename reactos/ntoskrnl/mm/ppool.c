/* $Id: ppool.c,v 1.16 2003/08/04 08:26:19 hbirr Exp $
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

#include <ddk/ntddk.h>
#include <internal/pool.h>
#include <internal/mm.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

/* Enable strict checking of the paged pool on every allocation */
#define ENABLE_VALIDATE_POOL

#undef assert
#define assert(x) if (!(x)) {DbgPrint("Assertion "#x" failed at %s:%d\n", __FILE__,__LINE__); KeBugCheck(0); }
#define ASSERT_SIZE(n) assert ( (n) <= MmPagedPoolSize && (n) >= 0 )
#define ASSERT_PTR(p) assert ( ((size_t)(p)) >= ((size_t)MmPagedPoolBase) && ((size_t)(p)) < ((size_t)(MmPagedPoolBase+MmPagedPoolSize)) )

// to disable buffer over/under-run detection, set the following macro to 0
#define MM_PPOOL_BOUNDARY_BYTES 4

typedef struct _MM_PPOOL_FREE_BLOCK_HEADER
{
  LONG Size;
  struct _MM_PPOOL_FREE_BLOCK_HEADER* NextFree;
} MM_PPOOL_FREE_BLOCK_HEADER, *PMM_PPOOL_FREE_BLOCK_HEADER;

typedef struct _MM_PPOOL_USED_BLOCK_HEADER
{
  LONG Size;
#if MM_PPOOL_BOUNDARY_BYTES
  LONG UserSize; // how many bytes the user actually asked for...
#endif//MM_PPOOL_BOUNDARY_BYTES
} MM_PPOOL_USED_BLOCK_HEADER, *PMM_PPOOL_USED_BLOCK_HEADER;

PVOID MmPagedPoolBase;
ULONG MmPagedPoolSize;
static FAST_MUTEX MmPagedPoolLock;
static PMM_PPOOL_FREE_BLOCK_HEADER MmPagedPoolFirstFreeBlock;

/* FUNCTIONS *****************************************************************/

inline static void* block_to_address ( PVOID blk )
/*
 * FUNCTION: Translate a block header address to the corresponding block
 * address (internal)
 */
{
  return ( (void *) ((char*)blk + sizeof(MM_PPOOL_USED_BLOCK_HEADER) + MM_PPOOL_BOUNDARY_BYTES) );
}

inline static PMM_PPOOL_USED_BLOCK_HEADER address_to_block(PVOID addr)
{
  return (PMM_PPOOL_USED_BLOCK_HEADER)
         ( ((char*)addr) - sizeof(MM_PPOOL_USED_BLOCK_HEADER) - MM_PPOOL_BOUNDARY_BYTES );
}

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

#ifdef ENABLE_VALIDATE_POOL
static void VerifyPagedPool ( int line )
{
  PMM_PPOOL_FREE_BLOCK_HEADER p = MmPagedPoolFirstFreeBlock;
  int count = 0;
  DPRINT ( "VerifyPagedPool(%i):\n", line );
  while ( p )
  {
    DPRINT ( "  0x%x: %lu bytes (next 0x%x)\n", p, p->Size, p->NextFree );
    ASSERT_PTR(p);
    ASSERT_SIZE(p->Size);
    count++;
    p = p->NextFree;
  }
  DPRINT ( "VerifyPagedPool(%i): (%lu blocks)\n", line, count );
}
#define VerifyPagedPool() VerifyPagedPool(__LINE__)
#else
#define VerifyPagedPool()
#endif

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
  ULONG Alignment;

  /*
   * Don't bother allocating anything for a zero-byte block.
   */
  if (NumberOfBytes == 0)
    {
      return(NULL);
    }

  DPRINT ( "ExAllocatePagedPoolWithTag(%i,%lu,%lu)\n", PoolType, NumberOfBytes, Tag );
  VerifyPagedPool();

  if (NumberOfBytes >= PAGE_SIZE)
    {
      Alignment = PAGE_SIZE;
    }
  else if (PoolType == PagedPoolCacheAligned)
    {
      Alignment = MM_CACHE_LINE_SIZE;
    }
  else
    {
      Alignment = 0;
    }

  /*
   * Calculate the total number of bytes we will need.
   */
  BlockSize = NumberOfBytes + sizeof(MM_PPOOL_USED_BLOCK_HEADER) + 2*MM_PPOOL_BOUNDARY_BYTES;
  if (BlockSize < sizeof(MM_PPOOL_FREE_BLOCK_HEADER))
  {
    /* At least we need the size of the free block header. */
    BlockSize = sizeof(MM_PPOOL_FREE_BLOCK_HEADER);
  }

  ExAcquireFastMutex(&MmPagedPoolLock);

  /*
   * Find the best-fitting block.
   */
  PreviousBlock = NULL;
  BestPreviousBlock = BestBlock = NULL;
  CurrentBlock = MmPagedPoolFirstFreeBlock;
  if ( Alignment > 0 )
    {
      PVOID BestAlignedAddr = NULL;
      while ( CurrentBlock != NULL )
	{
	  PVOID Addr = block_to_address(CurrentBlock);
	  PVOID CurrentBlockEnd = (PVOID)CurrentBlock + CurrentBlock->Size;
	  /* calculate last size-aligned address available within this block */
	  PVOID AlignedAddr = MM_ROUND_DOWN(CurrentBlockEnd-NumberOfBytes-MM_PPOOL_BOUNDARY_BYTES, Alignment);
	  assert ( AlignedAddr+NumberOfBytes+MM_PPOOL_BOUNDARY_BYTES <= CurrentBlockEnd );

	  /* special case, this address is already size-aligned, and the right size */
	  if ( Addr == AlignedAddr )
	    {
	      BestAlignedAddr = AlignedAddr;
	      BestPreviousBlock = PreviousBlock;
	      BestBlock = CurrentBlock;
	      break;
	    }
	  else if ( Addr < (PVOID)address_to_block(AlignedAddr) )
	    {
	      /*
	       * there's enough room to allocate our size-aligned memory out
	       * of this block, see if it's a better choice than any previous
	       * finds
	       */
	      if ( BestBlock == NULL || BestBlock->Size > CurrentBlock->Size )
		{
		  BestAlignedAddr = AlignedAddr;
		  BestPreviousBlock = PreviousBlock;
		  BestBlock = CurrentBlock;
		}
	    }

	  PreviousBlock = CurrentBlock;
	  CurrentBlock = CurrentBlock->NextFree;
	}

      /*
       * we found a best block can/should we chop a few bytes off the beginning
       * into a separate memory block?
       */
      if ( BestBlock != NULL )
	{
	  PVOID Addr = block_to_address(BestBlock);
	  if ( BestAlignedAddr != Addr )
	    {
	      PMM_PPOOL_FREE_BLOCK_HEADER NewFreeBlock =
		(PMM_PPOOL_FREE_BLOCK_HEADER)address_to_block(BestAlignedAddr);
	      assert ( BestAlignedAddr > Addr );
	      NewFreeBlock->Size = Addr + BestBlock->Size - BestAlignedAddr;
	      ASSERT_SIZE(NewFreeBlock->Size);
	      BestBlock->Size = (size_t)NewFreeBlock - (size_t)Addr;
	      ASSERT_SIZE(BestBlock->Size);

	      DPRINT ( "breaking off preceding bytes into their own block...\n" );
	      DPRINT ( "NewFreeBlock 0x%x Size %lu (Old Block's new size %lu) NextFree 0x%x\n",
		NewFreeBlock, NewFreeBlock->Size, BestBlock->Size, BestBlock->NextFree );

	      /* insert the new block into the chain */
	      NewFreeBlock->NextFree = BestBlock->NextFree;
	      BestBlock->NextFree = NewFreeBlock;

	      /* we want the following code to use our size-aligned block */
	      BestPreviousBlock = BestBlock;
	      BestBlock = NewFreeBlock;

	      //VerifyPagedPool();
	    }
	}
    }
  /*
   * non-size-aligned block search
   */
  else while ( CurrentBlock != NULL )
    {
      if (    CurrentBlock->Size >= BlockSize
	   && ( BestBlock == NULL || BestBlock->Size > CurrentBlock->Size )
	 )
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
      DPRINT("ExAllocatePagedPoolWithTag() - nothing suitable found, returning NULL\n" );
      ExReleaseFastMutex(&MmPagedPoolLock);
      return(NULL);
    }

  DPRINT("BestBlock 0x%x NextFree 0x%x\n", BestBlock, BestBlock->NextFree );

  //VerifyPagedPool();

  /*
   * Is there enough space to create a second block from the unused portion.
   */
  if ( BestBlock->Size > BlockSize
    && (BestBlock->Size - BlockSize) > sizeof(MM_PPOOL_FREE_BLOCK_HEADER)
    )
    {
      ULONG NewSize = BestBlock->Size - BlockSize;
      ASSERT_SIZE ( NewSize );

      //DPRINT("creating 2nd block from unused portion\n");
      DPRINT("BestBlock 0x%x Size 0x%x BlockSize 0x%x NewSize 0x%x\n",
	BestBlock, BestBlock->Size, BlockSize, NewSize );

      /*
       * Create the new free block.
       */
      //DPRINT("creating the new free block");
      NextBlock = (PMM_PPOOL_FREE_BLOCK_HEADER)((char*)BestBlock + BlockSize);
      //DPRINT(".");
      NextBlock->Size = NewSize;
      ASSERT_SIZE ( NextBlock->Size );
      //DPRINT(".");
      NextBlock->NextFree = BestBlock->NextFree;
      //DPRINT(".\n");

      /*
       * Replace the old free block with it.
       */
      //DPRINT("replacing old free block with it");
      if (BestPreviousBlock == NULL)
	{
	  //DPRINT("(from beginning)");
	  MmPagedPoolFirstFreeBlock = NextBlock;
	}
      else
	{
	  //DPRINT("(from previous)");
	  BestPreviousBlock->NextFree = NextBlock;
	}
      //DPRINT(".\n");

      /*
       * Create the new used block header.
       */
      //DPRINT("create new used block header");
      NewBlock = (PMM_PPOOL_USED_BLOCK_HEADER)BestBlock;
      //DPRINT(".");
      NewBlock->Size = BlockSize;
      ASSERT_SIZE ( NewBlock->Size );
      //DPRINT(".\n");
    }
  else
    {
      ULONG NewSize = BestBlock->Size;

      /*
       * Remove the selected block from the list of free blocks.
       */
      //DPRINT ( "Removing selected block from free block list\n" );
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
      ASSERT_SIZE ( NewBlock->Size );
    }

  VerifyPagedPool();

  ExReleaseFastMutex(&MmPagedPoolLock);

  BlockAddress = block_to_address ( NewBlock );

  memset(BlockAddress, 0, NumberOfBytes);

#if MM_PPOOL_BOUNDARY_BYTES
  NewBlock->UserSize = NumberOfBytes;
  // write out buffer-overrun detection bytes
  {
    int i;
    PUCHAR Addr = (PUCHAR)BlockAddress;
    //DbgPrint ( "writing buffer-overrun detection bytes" );
    for ( i = 0; i < MM_PPOOL_BOUNDARY_BYTES; i++ )
    {
      //DbgPrint(".");
      *(Addr-i-1) = 0xCD;
      //DbgPrint("o");
      *(Addr+NewBlock->UserSize+i) = 0xCD;
    }
    //DbgPrint ( "done!\n" );
  }
#endif//MM_PPOOL_BOUNDARY_BYTES

  return(BlockAddress);
}

VOID STDCALL
ExFreePagedPool(IN PVOID Block)
{
  PMM_PPOOL_FREE_BLOCK_HEADER PreviousBlock;
  PMM_PPOOL_USED_BLOCK_HEADER UsedBlock = address_to_block(Block);
  ULONG UsedSize = UsedBlock->Size;
  PMM_PPOOL_FREE_BLOCK_HEADER FreeBlock = 
    (PMM_PPOOL_FREE_BLOCK_HEADER)UsedBlock;
  PMM_PPOOL_FREE_BLOCK_HEADER NextBlock;
  PMM_PPOOL_FREE_BLOCK_HEADER NextNextBlock;

#if MM_PPOOL_BOUNDARY_BYTES
  // write out buffer-overrun detection bytes
  {
    int i;
    PUCHAR Addr = (PUCHAR)Block;
    //DbgPrint ( "checking buffer-overrun detection bytes..." );
    for ( i = 0; i < MM_PPOOL_BOUNDARY_BYTES; i++ )
    {
      assert ( *(Addr-i-1) == 0xCD );
      assert ( *(Addr+UsedBlock->UserSize+i) == 0xCD );
    }
    //DbgPrint ( "done!\n" );
  }
#endif//MM_PPOOL_BOUNDARY_BYTES

  ExAcquireFastMutex(&MmPagedPoolLock);

  /*
   * Begin setting up the newly freed block's header.
   */
  FreeBlock->Size = UsedSize;
  ASSERT_SIZE ( FreeBlock->Size );

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
      ((char*)FreeBlock + FreeBlock->Size) == (char*)NextBlock)
    {
      FreeBlock->Size = FreeBlock->Size + NextBlock->Size;
      ASSERT_SIZE ( FreeBlock->Size );
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
      ((char*)PreviousBlock + PreviousBlock->Size) == (char*)FreeBlock)
    {
      PreviousBlock->Size = PreviousBlock->Size + FreeBlock->Size;
      ASSERT_SIZE ( PreviousBlock->Size );
      PreviousBlock->NextFree = NextNextBlock;
    }

  VerifyPagedPool();

  ExReleaseFastMutex(&MmPagedPoolLock);
}

/* EOF */
