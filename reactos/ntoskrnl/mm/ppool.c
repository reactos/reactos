/* $Id: ppool.c,v 1.34 2004/12/11 00:13:37 royce Exp $
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

/* Define to enable strict checking of the paged pool on every allocation */
/* #define ENABLE_VALIDATE_POOL */

#undef assert
#define assert(x) if (!(x)) {DbgPrint("Assertion "#x" failed at %s:%d\n", __FILE__,__LINE__); KeBugCheck(0); }
#define ASSERT_SIZE(n) assert ( (n) <= MmPagedPoolSize && (n) > 0 )
#define IS_PPOOL_PTR(p) ((size_t)(p)) >= ((size_t)MmPagedPoolBase) && ((size_t)(p)) < ((size_t)((size_t)MmPagedPoolBase+MmPagedPoolSize))
#define ASSERT_PTR(p) assert ( IS_PPOOL_PTR(p) )

// to disable buffer over/under-run detection, set the following macro to 0
#if !defined(DBG) && !defined(KDBG)
#define MM_PPOOL_REDZONE_BYTES 0
#else
#define MM_PPOOL_REDZONE_BYTES 4
#define MM_PPOOL_REDZONE_LOVALUE 0x87
#define MM_PPOOL_REDZONE_HIVALUE 0xA5
#define MM_PPOOL_FREEMAGIC (ULONG)(('F'<<0) + ('r'<<8) + ('E'<<16) + ('e'<<24))
#define MM_PPOOL_USEDMAGIC (ULONG)(('u'<<0) + ('S'<<8) + ('e'<<16) + ('D'<<24))
#define MM_PPOOL_LASTOWNER_ENTRIES 3
#endif

typedef struct _MM_PPOOL_FREE_BLOCK_HEADER
{
#if MM_PPOOL_REDZONE_BYTES
   ULONG FreeMagic;
#endif//MM_PPOOL_REDZONE_BYTES
   ULONG Size;
   struct _MM_PPOOL_FREE_BLOCK_HEADER* NextFree;
#if MM_PPOOL_REDZONE_BYTES
   ULONG LastOwnerStack[MM_PPOOL_LASTOWNER_ENTRIES];
#endif//MM_PPOOL_REDZONE_BYTES
}
MM_PPOOL_FREE_BLOCK_HEADER, *PMM_PPOOL_FREE_BLOCK_HEADER;

typedef struct _MM_PPOOL_USED_BLOCK_HEADER
{
#if MM_PPOOL_REDZONE_BYTES
   ULONG UsedMagic;
#endif//MM_PPOOL_REDZONE_BYTES
   ULONG Size;
#if MM_PPOOL_REDZONE_BYTES
   ULONG UserSize; // how many bytes the user actually asked for...
#endif//MM_PPOOL_REDZONE_BYTES
   struct _MM_PPOOL_USED_BLOCK_HEADER* NextUsed;
   ULONG Tag;
}
MM_PPOOL_USED_BLOCK_HEADER, *PMM_PPOOL_USED_BLOCK_HEADER;

PVOID MmPagedPoolBase;
ULONG MmPagedPoolSize;
ULONG MmTotalPagedPoolQuota = 0;
static FAST_MUTEX MmPagedPoolLock;
static PMM_PPOOL_FREE_BLOCK_HEADER MmPagedPoolFirstFreeBlock;
static PMM_PPOOL_USED_BLOCK_HEADER MmPagedPoolFirstUsedBlock;

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
   MmPagedPoolFirstFreeBlock->FreeMagic = MM_PPOOL_FREEMAGIC;
   {
      int i;
      for ( i = 0; i < MM_PPOOL_LASTOWNER_ENTRIES; i++ )
         MmPagedPoolFirstFreeBlock->LastOwnerStack[i] = 0;
   }

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
#if MM_PPOOL_REDZONE_BYTES
      ASSERT ( p->FreeMagic == MM_PPOOL_FREEMAGIC );
#endif//MM_PPOOL_REDZONE_BYTES
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

BOOLEAN STDCALL
KeRosPrintAddress(PVOID address);

#if !MM_PPOOL_REDZONE_BYTES
#define MmpRedZoneCheck(pUsed,Addr,file,line)
#else//MM_PPOOL_REDZONE_BYTES
static VOID FASTCALL
MmpRedZoneCheck ( PMM_PPOOL_USED_BLOCK_HEADER pUsed, PUCHAR Addr, const char* file, int line )
{
   int i;
   PUCHAR AddrEnd = Addr + pUsed->UserSize;
   BOOL bLow = TRUE;
   BOOL bHigh = TRUE;

   ASSERT_PTR(Addr);
   if ( pUsed->UsedMagic == MM_PPOOL_FREEMAGIC )
   {
      PMM_PPOOL_FREE_BLOCK_HEADER pFree = (PMM_PPOOL_FREE_BLOCK_HEADER)pUsed;
      DPRINT1 ( "Double-free detected for Block 0x%x (kthread=0x%x)!\n", Addr, KeGetCurrentThread() );
      DbgPrint ( "First Free Stack Frames:" );
      for ( i = 0; i < MM_PPOOL_LASTOWNER_ENTRIES; i++ )
	  {
		  if ( pFree->LastOwnerStack[i] != 0xDEADBEEF )
		  {
			DbgPrint(" ");
			if (!KeRosPrintAddress ((PVOID)pFree->LastOwnerStack[i]) )
			{
				DbgPrint("<%X>", pFree->LastOwnerStack[i] );
			}
		  }
	  }
      DbgPrint ( "\n" );
      KEBUGCHECK(BAD_POOL_HEADER);
   }
   if ( pUsed->UsedMagic != MM_PPOOL_USEDMAGIC )
   {
      DPRINT1 ( "Bad magic in Block 0x%x!\n", Addr );
      KEBUGCHECK(BAD_POOL_HEADER);
   }
   ASSERT_SIZE(pUsed->Size);
   ASSERT_SIZE(pUsed->UserSize);
   ASSERT_PTR(AddrEnd);
   Addr -= MM_PPOOL_REDZONE_BYTES; // this is to simplify indexing below...
   for ( i = 0; i < MM_PPOOL_REDZONE_BYTES && bLow && bHigh; i++ )
   {
      bLow = bLow && ( Addr[i] == MM_PPOOL_REDZONE_LOVALUE );
      bHigh = bHigh && ( AddrEnd[i] == MM_PPOOL_REDZONE_HIVALUE );
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
      DbgPrint ( "UsedMagic 0x%x, LoZone ", pUsed->UsedMagic );
      for ( i = 0; i < MM_PPOOL_REDZONE_BYTES; i++ )
         DbgPrint ( "%02x", Addr[i] );
      DbgPrint ( ", HiZone " );
      for ( i = 0; i < MM_PPOOL_REDZONE_BYTES; i++ )
         DbgPrint ( "%02x", AddrEnd[i] );
      DbgPrint ( "\n" );
      KEBUGCHECK(BAD_POOL_HEADER);
   }
}
#endif//MM_PPOOL_REDZONE_BYTES

VOID STDCALL
MmDbgPagedPoolRedZoneCheck ( const char* file, int line )
{
#if MM_PPOOL_REDZONE_BYTES
   PMM_PPOOL_USED_BLOCK_HEADER pUsed = MmPagedPoolFirstUsedBlock;

   while ( pUsed )
   {
      MmpRedZoneCheck ( pUsed, block_to_address(pUsed), __FILE__, __LINE__ );
      pUsed = pUsed->NextUsed;
   }
#endif//MM_PPOOL_REDZONE_BYTES
}

/**********************************************************************
 * NAME       INTERNAL
 * ExAllocatePagedPoolWithTag@12
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 */
PVOID STDCALL
ExAllocatePagedPoolWithTag (IN POOL_TYPE PoolType,
                            IN ULONG  NumberOfBytes,
                            IN ULONG  Tag)
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

   ASSERT_IRQL(APC_LEVEL);

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
#if MM_PPOOL_REDZONE_BYTES
            NewFreeBlock->FreeMagic = MM_PPOOL_FREEMAGIC;
#endif//MM_PPOOL_REDZONE_BYTES
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
   else
      while ( CurrentBlock != NULL )
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
#if MM_PPOOL_REDZONE_BYTES
      NextBlock->FreeMagic = MM_PPOOL_FREEMAGIC;
#endif//MM_PPOOL_REDZONE_BYTES
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
#if MM_PPOOL_REDZONE_BYTES
      NewBlock->UsedMagic = MM_PPOOL_USEDMAGIC;
#endif//MM_PPOOL_REDZONE_BYTES
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
#if MM_PPOOL_REDZONE_BYTES
      NewBlock->UsedMagic = MM_PPOOL_USEDMAGIC;
#endif//MM_PPOOL_REDZONE_BYTES
      ASSERT_SIZE ( NewBlock->Size );
   }

   // now add the block to the used block list
   NewBlock->NextUsed = MmPagedPoolFirstUsedBlock;
   MmPagedPoolFirstUsedBlock = NewBlock;

   NewBlock->Tag = Tag;

   VerifyPagedPool();

   ExReleaseFastMutex(&MmPagedPoolLock);

   BlockAddress = block_to_address ( NewBlock );
   /*  RtlZeroMemory(BlockAddress, NumberOfBytes);*/

#if MM_PPOOL_REDZONE_BYTES

   NewBlock->UserSize = NumberOfBytes;
   // write out buffer-overrun detection bytes
   {
      PUCHAR Addr = (PUCHAR)BlockAddress;
      //DbgPrint ( "writing buffer-overrun detection bytes" );
      memset ( Addr - MM_PPOOL_REDZONE_BYTES,
               MM_PPOOL_REDZONE_LOVALUE, MM_PPOOL_REDZONE_BYTES );
      memset ( Addr + NewBlock->UserSize, MM_PPOOL_REDZONE_HIVALUE,
               MM_PPOOL_REDZONE_BYTES );
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

   ASSERT_IRQL(APC_LEVEL);

   MmpRedZoneCheck ( UsedBlock, Block, __FILE__, __LINE__ );

#if MM_PPOOL_REDZONE_BYTES
   memset ( Block, 0xCD, UsedBlock->UserSize );
#endif

   ExAcquireFastMutex(&MmPagedPoolLock);

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

   /*
    * Begin setting up the newly freed block's header.
    */
   FreeBlock->Size = UsedSize;
#if MM_PPOOL_REDZONE_BYTES
   FreeBlock->FreeMagic = MM_PPOOL_FREEMAGIC;
   {
      PULONG Frame;
      int i;
#if defined __GNUC__
      __asm__("mov %%ebp, %%ebx" : "=b" (Frame) : );
#elif defined(_MSC_VER)
      __asm mov [Frame], ebp
#endif
      //DbgPrint ( "Stack Frames for Free Block 0x%x:", Block );
      Frame = (PULONG)Frame[0]; // step out of ExFreePagedPool
      for ( i = 0; i < MM_PPOOL_LASTOWNER_ENTRIES; i++ )
      {
         if ( Frame == 0 || (ULONG)Frame == 0xDEADBEEF )
            FreeBlock->LastOwnerStack[i] = 0xDEADBEEF;
         else
         {
            //DbgPrint ( " 0x%x", Frame[1] );
            FreeBlock->LastOwnerStack[i] = Frame[1];
            Frame = (PULONG)Frame[0];
         }
      }
      //DbgPrint ( "\n" );
      //KeRosDumpStackFrames ( NULL, 4 );
   }
#endif//MM_PPOOL_REDZONE_BYTES
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
    * PLEASE DO NOT WIPE OUT 'MAGIC' OR 'LASTOWNER' DATA FOR MERGED FREE BLOCKS
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
    * PLEASE DO NOT WIPE OUT 'MAGIC' OR 'LASTOWNER' DATA FOR MERGED FREE BLOCKS
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

VOID STDCALL
ExRosDumpPagedPoolByTag ( ULONG Tag )
{
	PMM_PPOOL_USED_BLOCK_HEADER UsedBlock = MmPagedPoolFirstUsedBlock;
	int count = 0;
	char tag[5];

	// TODO FIXME - should we validate params or ASSERT_IRQL?
	*(ULONG*)&tag[0] = Tag;
	tag[4] = 0;
	DbgPrint ( "PagedPool Dump by tag '%s'\n", tag );
	DbgPrint ( "  -BLOCK-- --SIZE--\n" );
	while ( IS_PPOOL_PTR(UsedBlock) )
	{
		if ( UsedBlock->Tag == Tag )
		{
			DbgPrint ( "  %08X %08X\n", UsedBlock, UsedBlock->Size );
			++count;
		}
		UsedBlock = UsedBlock->NextUsed;
	}
	if ( UsedBlock && !IS_PPOOL_PTR(UsedBlock) )
	{
		DPRINT1 ( "!!NextUsed took me to lala land: 0x%08X\n", UsedBlock );
	}
	DbgPrint ( "Entries found for tag '%s': %i\n", tag, count );
}

ULONG STDCALL
ExRosQueryPagedPoolTag ( PVOID Block )
{
	PMM_PPOOL_USED_BLOCK_HEADER UsedBlock = address_to_block(Block);
	// TODO FIXME - should we validate params or ASSERT_IRQL?
	return UsedBlock->Tag;
}

/* EOF */
