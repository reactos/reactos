/* $Id: ppool.c,v 1.25 2004/02/15 19:03:29 hbirr Exp $
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
#define ASSERT_PTR(p) assert ( ((size_t)(p)) >= ((size_t)MmPagedPoolBase) && ((size_t)(p)) < ((size_t)((size_t)MmPagedPoolBase+MmPagedPoolSize)) )

// to disable buffer over/under-run detection, set the following macro to 0
#define MM_PPOOL_REDZONE_BYTES 4
#define MM_PPOOL_REDZONE_VALUE 0xCD

typedef struct _MM_PPOOL_FREE_BLOCK_HEADER
{
  ULONG Size;
  struct _MM_PPOOL_FREE_BLOCK_HEADER* NextFree;
} MM_PPOOL_FREE_BLOCK_HEADER, *PMM_PPOOL_FREE_BLOCK_HEADER;

typedef struct _MM_PPOOL_USED_BLOCK_HEADER
{
  ULONG Size;
#if MM_PPOOL_REDZONE_BYTES
  ULONG UserSize; // how many bytes the user actually asked for...
  struct _MM_PPOOL_USED_BLOCK_HEADER* NextUsed;
#endif//MM_PPOOL_REDZONE_BYTES
} MM_PPOOL_USED_BLOCK_HEADER, *PMM_PPOOL_USED_BLOCK_HEADER;

PVOID MmPagedPoolBase;
ULONG MmPagedPoolSize;
static FAST_MUTEX MmPagedPoolLock;
static PMM_PPOOL_FREE_BLOCK_HEADER MmPagedPoolFirstFreeBlock;
#if MM_PPOOL_REDZONE_BYTES
static PMM_PPOOL_USED_BLOCK_HEADER MmPagedPoolFirstUsedBlock;
#endif//MM_PPOOL_REDZONE_BYTES

/* FUNCTIONS *****************************************************************/

inline static void* block_to_address ( PVOID blk )
/*
 * FUNCTION: Translate a block header address to the corresponding block
 * address (internal)
 */
{
  return ( (void *) ((char*)blk + sizeof(MM_PPOOL_USED_BLOCK_HEADER) + MM_PPOOL_REDZONE_BYTES) );
}

inline static PMM_PPOOL_USED_BLOCK_HEADER address_to_block(PVOID addr)
{
  return (PMM_PPOOL_USED_BLOCK_HEADER)
         ( ((char*)addr) - sizeof(MM_PPOOL_USED_BLOCK_HEADER) - MM_PPOOL_REDZONE_BYTES );
}

VOID INIT_FUNCTION
MmInitializePagedPool(VOID)
{
  MmPagedPoolFirstFreeBlock = (PMM_PPOOL_FREE_BLOCK_HEADER)MmPagedPoolBase;
  /*
   * We are still at a high IRQL level at this point so explicitly commit
   * the first page of the paged pool before writing the first block header.
   */
  MmCommitPagedPoolAddress((PVOID)MmPagedPoolFirstFreeBlock, FALSE);
  MmPagedPoolFirstFreeBlock->Size = MmPagedPoolSize;
  MmPagedPoolFirstFreeBlock->NextFree = NULL;

#if MM_PPOOL_REDZONE_BYTES
  MmPagedPoolFirstUsedBlock = NULL;
#endif//MM_PPOOL_REDZONE_BYTES

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

VOID STDCALL
MmDbgPagedPoolRedZoneCheck ( const char* file, int line )
{
#if MM_PPOOL_REDZONE_BYTES
  PMM_PPOOL_USED_BLOCK_HEADER pUsed = MmPagedPoolFirstUsedBlock;
  int i;
  BOOL bLow = TRUE;
  BOOL bHigh = TRUE;

  while ( pUsed )
  {
    PUCHAR Addr = (PUCHAR)block_to_address(pUsed);
    for ( i = 0; i < MM_PPOOL_REDZONE_BYTES; i++ )
    {
      bLow = bLow && ( *(Addr-i-1) == MM_PPOOL_REDZONE_VALUE );
      bHigh = bHigh && ( *(Addr+pUsed->UserSize+i) == MM_PPOOL_REDZONE_VALUE );
    }
    if ( !bLow || !bHigh )
    {
      const char* violation = "High and Low-side";
      if ( bHigh ) // high is okay, so it was just low failed
	violation = "Low-side";
      else if ( bLow ) // low side is okay, so it was just high failed
	violation = "High-side";
      DbgPrint("%s(%i): %s redzone violation detected for paged pool address 0x%x\n",
	file, line, violation, Addr );
      KEBUGCHECK(0);
    }
    pUsed = pUsed->NextUsed;
  }
#endif//MM_PPOOL_REDZONE_BYTES
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
  ULONG Alignment;

  ExAcquireFastMutex(&MmPagedPoolLock);

  /*
   * Don't bother allocating anything for a zero-byte block.
   */
  if (NumberOfBytes == 0)
    {
      MmDbgPagedPoolRedZoneCheck(__FILE__,__LINE__);
      ExReleaseFastMutex(&MmPagedPoolLock);
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
      Alignment = MM_POOL_ALIGNMENT;
    }

  /*
   * Calculate the total number of bytes we will need.
   */
  BlockSize = NumberOfBytes + sizeof(MM_PPOOL_USED_BLOCK_HEADER) + 2*MM_PPOOL_REDZONE_BYTES;
  if (BlockSize < sizeof(MM_PPOOL_FREE_BLOCK_HEADER))
  {
    /* At least we need the size of the free block header. */
    BlockSize = sizeof(MM_PPOOL_FREE_BLOCK_HEADER);
  }


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
	  PVOID CurrentBlockEnd = (char*)CurrentBlock + CurrentBlock->Size;
	  /* calculate last size-aligned address available within this block */
	  PVOID AlignedAddr = MM_ROUND_DOWN((char*)CurrentBlockEnd-NumberOfBytes-MM_PPOOL_REDZONE_BYTES, Alignment);
	  assert ( (char*)AlignedAddr+NumberOfBytes+MM_PPOOL_REDZONE_BYTES <= (char*)CurrentBlockEnd );

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
	      NewFreeBlock->Size = (char*)Addr + BestBlock->Size - (char*)BestAlignedAddr;
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
      DPRINT1("Trying to allocate %lu bytes from paged pool - nothing suitable found, returning NULL\n",
              NumberOfBytes );
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

#if MM_PPOOL_REDZONE_BYTES
  // now add the block to the used block list
  NewBlock->NextUsed = MmPagedPoolFirstUsedBlock;
  MmPagedPoolFirstUsedBlock = NewBlock;
#endif//MM_PPOOL_REDZONE_BYTES

  VerifyPagedPool();

  ExReleaseFastMutex(&MmPagedPoolLock);

  BlockAddress = block_to_address ( NewBlock );

  memset(BlockAddress, 0, NumberOfBytes);

#if MM_PPOOL_REDZONE_BYTES
  NewBlock->UserSize = NumberOfBytes;
  // write out buffer-overrun detection bytes
  {
    PUCHAR Addr = (PUCHAR)BlockAddress;
    //DbgPrint ( "writing buffer-overrun detection bytes" );
    memset ( Addr - MM_PPOOL_REDZONE_BYTES,
      MM_PPOOL_REDZONE_VALUE, MM_PPOOL_REDZONE_BYTES );
    memset ( Addr + NewBlock->UserSize, MM_PPOOL_REDZONE_VALUE,
      MM_PPOOL_REDZONE_BYTES );
    /*for ( i = 0; i < MM_PPOOL_REDZONE_BYTES; i++ )
    {
      //DbgPrint(".");
      *(Addr-i-1) = 0xCD;
      //DbgPrint("o");
      *(Addr+NewBlock->UserSize+i) = 0xCD;
    }*/
    //DbgPrint ( "done!\n" );
  }

#endif//MM_PPOOL_REDZONE_BYTES

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

#if MM_PPOOL_REDZONE_BYTES
  // write out buffer-overrun detection bytes
  {
    int i;
    PUCHAR Addr = (PUCHAR)Block;
    //DbgPrint ( "checking buffer-overrun detection bytes..." );
    for ( i = 0; i < MM_PPOOL_REDZONE_BYTES; i++ )
    {
      if (*(Addr-i-1) != MM_PPOOL_REDZONE_VALUE)
      {
        DPRINT1("Attempt to free memory %#08x. Redzone underrun!\n", Block);
      }
      if (*(Addr+UsedBlock->UserSize+i) != MM_PPOOL_REDZONE_VALUE)
      {
        DPRINT1("Attempt to free memory %#08x. Redzone overrun!\n", Block);
      }

      assert ( *(Addr-i-1) == MM_PPOOL_REDZONE_VALUE );
      assert ( *(Addr+UsedBlock->UserSize+i) == MM_PPOOL_REDZONE_VALUE );
    }
    //DbgPrint ( "done!\n" );
  }
#endif//MM_PPOOL_REDZONE_BYTES

  ExAcquireFastMutex(&MmPagedPoolLock);

#if MM_PPOOL_REDZONE_BYTES
  // remove from used list...
  {
    PMM_PPOOL_USED_BLOCK_HEADER pPrev = MmPagedPoolFirstUsedBlock;
    if ( pPrev == UsedBlock )
    {
      // special-case, our freeing block is first in list...
      MmPagedPoolFirstUsedBlock = pPrev->NextUsed;
    }
    else
    {
      while ( pPrev && pPrev->NextUsed != UsedBlock )
	pPrev = pPrev->NextUsed;
      // if this assert fails - memory has been corrupted
      // ( or I have a logic error...! )
      assert ( pPrev->NextUsed == UsedBlock );
      pPrev->NextUsed = UsedBlock->NextUsed;
    }
  }
#endif//MM_PPOOL_REDZONE_BYTES

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
