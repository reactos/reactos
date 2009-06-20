/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/RPoolMgr.h
 * PURPOSE:         A semi-generic reuseable Pool implementation
 * PROGRAMMER:      Royce Mitchell III
 * UPDATE HISTORY:
 */

#ifndef RPOOLMGR_H
#define RPOOLMGR_H

typedef unsigned long rulong;

#define R_IS_POOL_PTR(pool,ptr) (((void*)(ULONG_PTR)(ptr) >= pool->UserBase) && ((ULONG_PTR)(ptr) < ((ULONG_PTR)pool->UserBase + pool->UserSize)))
#define R_ASSERT_PTR(pool,ptr) ASSERT( R_IS_POOL_PTR(pool,ptr) )
#define R_ASSERT_SIZE(pool,sz) ASSERT( sz > (sizeof(R_USED)+2*R_RZ) && sz >= sizeof(R_FREE) && sz < pool->UserSize )

#ifndef R_ROUND_UP
#define R_ROUND_UP(x,s)    ((PVOID)(((ULONG_PTR)(x)+(s)-1) & ~((ULONG_PTR)(s)-1)))
#endif//R_ROUND_UP

#ifndef R_ROUND_DOWN
#define R_ROUND_DOWN(x,s)  ((PVOID)(((ULONG_PTR)(x)) & ~((ULONG_PTR)(s)-1)))
#endif//R_ROUND_DOWN

#ifndef R_QUEMIN
// R_QUEMIN is the minimum number of entries to keep in a que
#define R_QUEMIN 0
#endif//R_QUEMIN

#ifndef R_QUECOUNT
// 16, 32, 64, 128, 256, 512
#define R_QUECOUNT 6
#endif//R_QUECOUNT

#ifndef R_RZ
// R_RZ is the redzone size
#define R_RZ 4
#endif//R_RZ

#ifndef R_RZ_LOVALUE
#define R_RZ_LOVALUE 0x87
#endif//R_RZ_LOVALUE

#ifndef R_RZ_HIVALUE
#define R_RZ_HIVALUE 0xA5
#endif//R_RZ_HIVALUE

#ifndef R_STACK
// R_STACK is the number of stack entries to store in blocks for debug purposes
#define R_STACK 6
#else // R_STACK
#if R_STACK > 0 && R_STACK < 6
/* Increase the frame depth to get a reasonable back trace */
#undef R_STACK
#define R_STACK 6
#endif // R_STACK > 0 && R_STACK < 6
#endif//R_STACK

#ifndef R_TAG
// R_TAG do we keep track of tags on a per-memory block basis?
#define R_TAG 0
#endif//R_TAG

#ifdef R_MAGIC
#	ifndef R_FREE_MAGIC
#		define R_FREE_MAGIC (rulong)(('F'<<0) + ('r'<<8) + ('E'<<16) + ('e'<<24))
#	endif//R_FREE_MAGIC
#	ifndef R_USED_MAGIC
#		define R_USED_MAGIC (rulong)(('u'<<0) + ('S'<<8) + ('e'<<16) + ('D'<<24))
#	endif//R_USED_MAGIC
#endif//R_MAGIC

// **IMPORTANT NOTE** Magic, PrevSize, Size and Status must be at same offset
// in both the R_FREE and R_USED structures

typedef struct _R_FREE
{
#ifdef R_MAGIC
	rulong FreeMagic;
#endif//R_MAGIC
	rulong PrevSize : 30;
	rulong Status : 2;
	rulong Size;
#if R_STACK
	ULONG_PTR LastOwnerStack[R_STACK];
#endif//R_STACK
	struct _R_FREE* NextFree;
	struct _R_FREE* PrevFree;
}
R_FREE, *PR_FREE;

typedef struct _R_USED
{
#ifdef R_MAGIC
	rulong UsedMagic;
#endif//R_MAGIC
	rulong PrevSize : 30;
	rulong Status : 2;
	rulong Size;
#if R_STACK
	ULONG_PTR LastOwnerStack[R_STACK];
#endif//R_STACK
	struct _R_USED* NextUsed;
#if R_RZ
	rulong UserSize; // how many bytes the user actually asked for...
#endif//R_RZ
	rulong Tag;
}
R_USED, *PR_USED;

typedef struct _R_QUE
{
	rulong Count;
	PR_USED First, Last;
}
R_QUE, *PR_QUE;

typedef struct _R_POOL
{
	void* PoolBase;
	rulong PoolSize;
	void* UserBase;
	rulong UserSize;
	rulong Alignments[3];
	PR_FREE FirstFree, LastFree;
	R_QUE Que[R_QUECOUNT][3];
	R_MUTEX Mutex;
}
R_POOL, *PR_POOL;

#if !R_STACK
#define RiPrintLastOwner(Block)
#else
static void
RiPrintLastOwner ( PR_USED Block )
{
	int i;
	for ( i = 0; i < R_STACK; i++ )
	{
		if ( Block->LastOwnerStack[i] != 0xDEADBEEF )
		{
			R_DEBUG(" ");
			//if (!R_PRINT_ADDRESS ((PVOID)Block->LastOwnerStack[i]) )
			{
				R_DEBUG("<%X>", Block->LastOwnerStack[i] );
			}
		}
	}
}
#endif//R_STACK

static int
RQueWhich ( rulong size )
{
	rulong que, quesize;
	for ( que=0, quesize=16; que < R_QUECOUNT; que++, quesize<<=1 )
	{
		if ( quesize >= size )
		{
			return que;
		}
	}
	return -1;
}

static void
RQueInit ( PR_QUE que )
{
	que->Count = 0;
	que->First = NULL;
	que->Last = NULL;
}

static void
RQueAdd ( PR_QUE que, PR_USED Item )
{
	ASSERT(Item);
	Item->Status = 2;
	Item->NextUsed = NULL;
	++que->Count;
	if ( !que->Last )
	{
		que->First = que->Last = Item;
		return;
	}
	ASSERT(!que->Last->NextUsed);
	que->Last->NextUsed = Item;
	que->Last = Item;
}

static PR_USED
RQueRemove ( PR_QUE que )
{
	PR_USED Item;
#if R_QUEMIN
	if ( que->count < R_QUEMIN )
		return NULL;
#endif
	if ( !que->First )
		return NULL;
	Item = que->First;
	que->First = Item->NextUsed;
	if ( !--que->Count )
	{
		ASSERT ( !que->First );
		que->Last = NULL;
	}
	Item->Status = 0;
	return Item;
}

static void
RPoolAddFree ( PR_POOL pool, PR_FREE Item )
{
	ASSERT(pool);
	ASSERT(Item);
	if ( !pool->FirstFree )
	{
		pool->FirstFree = pool->LastFree = Item;
		Item->NextFree = NULL;
	}
	else
	{
		pool->FirstFree->PrevFree = Item;
		Item->NextFree = pool->FirstFree;
		pool->FirstFree = Item;
	}
	Item->PrevFree = NULL;
}

static void
RPoolRemoveFree ( PR_POOL pool, PR_FREE Item )
{
	ASSERT(pool);
	ASSERT(Item);
	if ( Item->NextFree )
		Item->NextFree->PrevFree = Item->PrevFree;
	else
	{
		ASSERT ( pool->LastFree == Item );
		pool->LastFree = Item->PrevFree;
	}
	if ( Item->PrevFree )
		Item->PrevFree->NextFree = Item->NextFree;
	else
	{
		ASSERT ( pool->FirstFree == Item );
		pool->FirstFree = Item->NextFree;
	}
#if DBG
	Item->NextFree = Item->PrevFree = (PR_FREE)(ULONG_PTR)0xDEADBEEF;
#endif//DBG
}

#if !R_STACK
#define RFreeFillStack(free)
#define RUsedFillStack(used)
#else
static void
RFreeFillStack ( PR_FREE free )
{
	int i;
	ULONG stack[R_STACK+3]; // need to skip 3 known levels of stack trace
	memset ( stack, 0xCD, sizeof(stack) );
	R_GET_STACK_FRAMES ( stack, R_STACK+3 );
	for ( i = 0; i < R_STACK; i++ )
		free->LastOwnerStack[i] = stack[i+3];
}

static void
RUsedFillStack ( PR_USED used )
{
	int i;
	ULONG stack[R_STACK+2]; // need to skip 2 known levels of stack trace
	memset ( stack, 0xCD, sizeof(stack) );
	R_GET_STACK_FRAMES ( stack, R_STACK+2 );
	for ( i = 0; i < R_STACK; i++ )
		used->LastOwnerStack[i] = stack[i+2];
}
#endif

static PR_FREE
RFreeInit ( void* memory )
{
	PR_FREE block = (PR_FREE)memory;
#if R_FREEMAGIC
	block->FreeMagic = R_FREE_MAGIC;
#endif//R_FREEMAGIC
	block->Status = 0;
	RFreeFillStack ( block );
#if DBG
	block->PrevFree = block->NextFree = (PR_FREE)(ULONG_PTR)0xDEADBEEF;
#endif//DBG
	return block;
}

PR_POOL
RPoolInit ( void* PoolBase, rulong PoolSize, int align1, int align2, int align3 )
{
	int align, que;
	PR_POOL pool = (PR_POOL)PoolBase;

	pool->PoolBase = PoolBase;
	pool->PoolSize = PoolSize;
	pool->UserBase = (char*)pool->PoolBase + sizeof(R_POOL);
	pool->UserSize = PoolSize - sizeof(R_POOL);
	pool->Alignments[0] = align1;
	pool->Alignments[1] = align2;
	pool->Alignments[2] = align3;
	pool->FirstFree = pool->LastFree = NULL;

	RPoolAddFree ( pool,
		RFreeInit ( pool->UserBase ));

	pool->FirstFree->PrevSize = 0;
	pool->FirstFree->Size = pool->UserSize;

	for ( que = 0; que < R_QUECOUNT; que++ )
	{
		for ( align = 0; align < 3; align++ )
		{
			RQueInit ( &pool->Que[que][align] );
		}
	}
	return pool;
}

#if R_RZ
static const char*
RFormatTag ( rulong Tag, char* buf )
{
	int i;
	*(rulong*)&buf[0] = Tag;
	buf[4] = 0;
	for ( i = 0; i < 4; i++ )
	{
		if ( !buf[i] )
			buf[i] = ' ';
	}
	return buf;
}
#endif

#if !R_RZ
#define RUsedRedZoneCheck(pUsed,Addr,file,line, printzone)
#else//R_RZ
static void
RiBadBlock ( PR_USED pUsed, char* Addr, const char* violation, const char* file, int line, int printzone )
{
	char tag[5];
	unsigned int i;

	R_DEBUG("%s(%i): %s detected for paged pool address 0x%x\n",
		file, line, violation, Addr );

#ifdef R_MAGIC
	R_DEBUG ( "UsedMagic 0x%x, ", pUsed->UsedMagic );
#endif//R_MAGIC
	R_DEBUG ( "Tag %s(%X), Size %i, UserSize %i",
		RFormatTag(pUsed->Tag,tag),
		pUsed->Tag,
		pUsed->Size,
		pUsed->UserSize );

	if ( printzone )
	{
		unsigned char* HiZone = (unsigned char*)Addr + pUsed->UserSize;
		unsigned char* LoZone = (unsigned char*)Addr - R_RZ; // this is to simplify indexing below...
		R_DEBUG ( ", LoZone " );
		for ( i = 0; i < R_RZ; i++ )
			R_DEBUG ( "%02x", LoZone[i] );
		R_DEBUG ( ", HiZone " );
		for ( i = 0; i < R_RZ; i++ )
			R_DEBUG ( "%02x", HiZone[i] );
	}
	R_DEBUG ( "\n" );

	R_DEBUG ( "First few Stack Frames:" );
	RiPrintLastOwner ( pUsed );
	R_DEBUG ( "\n" );

	R_DEBUG ( "Contents of Block:\n" );
	for ( i = 0; i < 8*16 && i < pUsed->UserSize; i += 16 )
	{
		int j;
		R_DEBUG ( "%04X ", i );
		for ( j = 0; j < 16; j++ )
		{
			if ( i+j < pUsed->UserSize )
			{
				R_DEBUG ( "%02X ", (unsigned)(unsigned char)Addr[i+j] );
			}
			else
			{
				R_DEBUG ( "   " );
			}
		}
		R_DEBUG(" ");
		for ( j = 0; j < 16; j++ )
		{
			if ( i+j < pUsed->UserSize )
			{
				char c = Addr[i+j];
				if ( c < 0x20 || c > 0x7E )
					c = '.';
				R_DEBUG ( "%c", c );
			}
			else
			{
				R_DEBUG ( " " );
			}
		}
		R_DEBUG("\n");
	}
	R_PANIC();
}
static void
RUsedRedZoneCheck ( PR_POOL pool, PR_USED pUsed, char* Addr, const char* file, int line )
{
	int i;
	unsigned char *LoZone, *HiZone;
	int bLow = 1;
	int bHigh = 1;

	ASSERT ( Addr >= (char*)pool->UserBase && Addr < ((char*)pool->UserBase + pool->UserSize - 16) );
#ifdef R_MAGIC
	if ( pUsed->UsedMagic == R_FREE_MAGIC )
	{
		pUsed->UserSize = 0; // just to keep from confusion, MmpBadBlock() doesn't return...
		RiBadBlock ( pUsed, Addr, "double-free", file, line, 0 );
	}
	if ( pUsed->UsedMagic != R_USED_MAGIC )
	{
		RiBadBlock ( pUsed, Addr, "bad magic", file, line, 0 );
	}
#endif//R_MAGIC
	switch ( pUsed->Status )
	{
	case 0: // freed into main pool
	case 2: // in ques
		RiBadBlock ( pUsed, Addr, "double-free", file, line, 0 );
		// no need for break here - RiBadBlock doesn't return
	case 1: // allocated - this is okay
		break;
	default:
		RiBadBlock ( pUsed, Addr, "corrupt status", file, line, 0 );
	}
	if ( pUsed->Status != 1 )
	{
		RiBadBlock ( pUsed, Addr, "double-free", file, line, 0 );
	}
	if ( pUsed->Size > pool->PoolSize || pUsed->Size == 0 )
	{
		RiBadBlock ( pUsed, Addr, "invalid size", file, line, 0 );
	}
	if ( pUsed->UserSize > pool->PoolSize || pUsed->UserSize == 0 )
	{
		RiBadBlock ( pUsed, Addr, "invalid user size", file, line, 0 );
	}
	HiZone = (unsigned char*)Addr + pUsed->UserSize;
	LoZone = (unsigned char*)Addr - R_RZ; // this is to simplify indexing below...
	for ( i = 0; i < R_RZ && bLow && bHigh; i++ )
	{
		bLow = bLow && ( LoZone[i] == R_RZ_LOVALUE );
		bHigh = bHigh && ( HiZone[i] == R_RZ_HIVALUE );
	}
	if ( !bLow || !bHigh )
	{
		const char* violation = "High and Low-side redzone overwrite";
		if ( bHigh ) // high is okay, so it was just low failed
			violation = "Low-side redzone overwrite";
		else if ( bLow ) // low side is okay, so it was just high failed
			violation = "High-side redzone overwrite";
		RiBadBlock ( pUsed, Addr, violation, file, line, 1 );
	}
}
#endif//R_RZ

PR_FREE
RPreviousBlock ( PR_FREE Block )
{
	if ( Block->PrevSize > 0 )
		return (PR_FREE)( (char*)Block - Block->PrevSize );
	return NULL;
}

PR_FREE
RNextBlock ( PR_POOL pool, PR_FREE Block )
{
	PR_FREE NextBlock = (PR_FREE)( (char*)Block + Block->Size );
	if ( (char*)NextBlock >= (char*)pool->UserBase + pool->UserSize )
		NextBlock = NULL;
	return NextBlock;
}

static __inline void*
RHdrToBody ( void* blk )
/*
 * FUNCTION: Translate a block header address to the corresponding block
 * address (internal)
 */
{
	return ( (void *) ((char*)blk + sizeof(R_USED) + R_RZ) );
}

static __inline PR_USED
RBodyToHdr ( void* addr )
{
	return (PR_USED)
	       ( ((char*)addr) - sizeof(R_USED) - R_RZ );
}

static int
RiInFreeChain ( PR_POOL pool, PR_FREE Block )
{
	PR_FREE Free;
	Free = pool->FirstFree;
	if ( Free == Block )
		return 1;
	while ( Free != Block )
	{
		Free = Free->NextFree;
		if ( !Free )
			return 0;
	}
	return 1;
}

static void
RPoolRedZoneCheck ( PR_POOL pool, const char* file, int line )
{
	{
		PR_USED Block = (PR_USED)pool->UserBase;
		PR_USED NextBlock;

		for ( ;; )
		{
			switch ( Block->Status )
			{
			case 0: // block is in chain
				ASSERT ( RiInFreeChain ( pool, (PR_FREE)Block ) );
				break;
			case 1: // block is allocated
				RUsedRedZoneCheck ( pool, Block, RHdrToBody(Block), file, line );
				break;
			case 2: // block is in que
				// nothing to verify here yet
				break;
			default:
				ASSERT ( !"invalid status in memory block found in pool!" );
			}
			NextBlock = (PR_USED)RNextBlock(pool,(PR_FREE)Block);
			if ( !NextBlock )
				break;
			ASSERT ( NextBlock->PrevSize == Block->Size );
			Block = NextBlock;
		}
	}
	{
		// now let's step through the list of free pointers and verify
		// each one can be found by size-jumping...
		PR_FREE Free = (PR_FREE)pool->FirstFree;
		while ( Free )
		{
			PR_FREE NextFree = (PR_FREE)pool->UserBase;
			if ( Free != NextFree )
			{
				while ( NextFree != Free )
				{
					NextFree = RNextBlock ( pool, NextFree );
					ASSERT(NextFree);
				}
			}
			Free = Free->NextFree;
		}
	}
}

static void
RSetSize ( PR_POOL pool, PR_FREE Block, rulong NewSize, PR_FREE NextBlock )
{
	R_ASSERT_PTR(pool,Block);
	ASSERT ( NewSize < pool->UserSize );
	ASSERT ( NewSize >= sizeof(R_FREE) );
	Block->Size = NewSize;
	if ( !NextBlock )
		NextBlock = RNextBlock ( pool, Block );
	if ( NextBlock )
		NextBlock->PrevSize = NewSize;
}

static PR_FREE
RFreeSplit ( PR_POOL pool, PR_FREE Block, rulong NewSize )
{
	PR_FREE NewBlock = (PR_FREE)((char*)Block + NewSize);
	RSetSize ( pool, NewBlock, Block->Size - NewSize, NULL );
	RSetSize ( pool, Block, NewSize, NewBlock );
	RFreeInit ( NewBlock );
	RPoolAddFree ( pool, NewBlock );
	return NewBlock;
}

static void
RFreeMerge ( PR_POOL pool, PR_FREE First, PR_FREE Second )
{
	ASSERT ( RPreviousBlock(Second) == First );
	ASSERT ( First->Size == Second->PrevSize );
	RPoolRemoveFree ( pool, Second );
	RSetSize ( pool, First, First->Size + Second->Size, NULL );
}

static void
RPoolReclaim ( PR_POOL pool, PR_FREE FreeBlock )
{
	PR_FREE NextBlock, PreviousBlock;

	RFreeInit ( FreeBlock );
	RPoolAddFree ( pool, FreeBlock );

	// TODO FIXME - don't merge and always insert freed blocks at the end for debugging purposes...

	/*
	 * If the next block is immediately adjacent to the newly freed one then
	 * merge them.
	 * PLEASE DO NOT WIPE OUT 'MAGIC' OR 'LASTOWNER' DATA FOR MERGED FREE BLOCKS
	 */
	NextBlock = RNextBlock ( pool, FreeBlock );
	if ( NextBlock != NULL && !NextBlock->Status )
	{
		RFreeMerge ( pool, FreeBlock, NextBlock );
	}

	/*
	 * If the previous block is adjacent to the newly freed one then
	 * merge them.
	 * PLEASE DO NOT WIPE OUT 'MAGIC' OR 'LASTOWNER' DATA FOR MERGED FREE BLOCKS
	 */
	PreviousBlock = RPreviousBlock ( FreeBlock );
	if ( PreviousBlock != NULL && !PreviousBlock->Status )
	{
		RFreeMerge ( pool, PreviousBlock, FreeBlock );
	}
}

static void
RiUsedInit ( PR_USED Block, rulong Tag )
{
	Block->Status = 1;
	RUsedFillStack ( Block );
#ifdef R_MAGIC
	Block->UsedMagic = R_USED_MAGIC;
#endif//R_MAGIC
	//ASSERT_SIZE ( Block->Size );

	// now add the block to the used block list
#if DBG
	Block->NextUsed = (PR_USED)(ULONG_PTR)0xDEADBEEF;
#endif//R_USED_LIST

	Block->Tag = Tag;
}

#if !R_RZ
#define RiUsedInitRedZone(Block,UserSize)
#else//R_RZ
static void
RiUsedInitRedZone ( PR_USED Block, rulong UserSize )
{
	// write out buffer-overrun detection bytes
	char* Addr = (char*)RHdrToBody(Block);
	Block->UserSize = UserSize;
	memset ( Addr - R_RZ, R_RZ_LOVALUE, R_RZ );
	memset ( Addr + Block->UserSize, R_RZ_HIVALUE, R_RZ );
#if DBG
	memset ( Addr, 0xCD, UserSize );
#endif//DBG
}
#endif//R_RZ

static void*
RPoolAlloc ( PR_POOL pool, rulong NumberOfBytes, rulong Tag, rulong align )
{
	PR_USED NewBlock;
	PR_FREE BestBlock,
		NextBlock,
		PreviousBlock,
		BestPreviousBlock,
		CurrentBlock;
	void* BestAlignedAddr;
	int que,
		queBytes = NumberOfBytes;
	rulong BlockSize,
		Alignment;
	int que_reclaimed = 0;

	ASSERT ( pool );
	ASSERT ( align < 3 );

	R_ACQUIRE_MUTEX(pool);

	if ( !NumberOfBytes )
	{
		R_DEBUG("0 bytes requested - initiating pool verification\n");
		RPoolRedZoneCheck ( pool, __FILE__, __LINE__ );
		R_RELEASE_MUTEX(pool);
		return NULL;
	}
	if ( NumberOfBytes > pool->PoolSize )
	{
		if ( R_IS_POOL_PTR(pool,NumberOfBytes) )
		{
			R_DEBUG("red zone verification requested for block 0x%X\n", NumberOfBytes );
			RUsedRedZoneCheck(pool,RBodyToHdr((void*)(ULONG_PTR)NumberOfBytes), (char*)(ULONG_PTR)NumberOfBytes, __FILE__, __LINE__ );
			R_RELEASE_MUTEX(pool);
			return NULL;
		}
		R_DEBUG("Invalid allocation request: %i bytes\n", NumberOfBytes );
		R_RELEASE_MUTEX(pool);
		return NULL;
	}

	que = RQueWhich ( NumberOfBytes );
	if ( que >= 0 )
	{
		if ( (NewBlock = RQueRemove ( &pool->Que[que][align] )) )
		{
			RiUsedInit ( NewBlock, Tag );
			RiUsedInitRedZone ( NewBlock, NumberOfBytes );
			R_RELEASE_MUTEX(pool);
			return RHdrToBody(NewBlock);
		}
		queBytes = 16 << que;
	}

	/*
	 * Calculate the total number of bytes we will need.
	 */
	BlockSize = queBytes + sizeof(R_USED) + 2*R_RZ;
	if (BlockSize < sizeof(R_FREE))
	{
		/* At least we need the size of the free block header. */
		BlockSize = sizeof(R_FREE);
	}

try_again:
	/*
	 * Find the best-fitting block.
	 */
	BestBlock = NULL;
	Alignment = pool->Alignments[align];
	PreviousBlock = NULL;
	BestPreviousBlock = NULL,
	CurrentBlock = pool->FirstFree;
	BestAlignedAddr = NULL;

	while ( CurrentBlock != NULL )
	{
		PVOID Addr = RHdrToBody(CurrentBlock);
		PVOID CurrentBlockEnd = (char*)CurrentBlock + CurrentBlock->Size;
		/* calculate last size-aligned address available within this block */
		PVOID AlignedAddr = R_ROUND_DOWN((char*)CurrentBlockEnd-queBytes-R_RZ, Alignment);
		ASSERT ( (char*)AlignedAddr+queBytes+R_RZ <= (char*)CurrentBlockEnd );

		/* special case, this address is already size-aligned, and the right size */
		if ( Addr == AlignedAddr )
		{
			BestAlignedAddr = AlignedAddr;
			BestPreviousBlock = PreviousBlock;
			BestBlock = CurrentBlock;
			break;
		}
		// if we carve out a size-aligned block... is it still past the end of this
		// block's free header?
		else if ( (char*)RBodyToHdr(AlignedAddr)
			>= (char*)CurrentBlock+sizeof(R_FREE) )
		{
			/*
			 * there's enough room to allocate our size-aligned memory out
			 * of this block, see if it's a better choice than any previous
			 * finds
			 */
			if ( BestBlock == NULL
				|| BestBlock->Size > CurrentBlock->Size )
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
	 * We didn't find anything suitable at all.
	 */
	if (BestBlock == NULL)
	{
		if ( !que_reclaimed )
		{
			// reclaim que
			int i, j;
			for ( i = 0; i < R_QUECOUNT; i++ )
			{
				for ( j = 0; j < 3; j++ )
				{
					while ( (BestBlock = (PR_FREE)RQueRemove ( &pool->Que[i][j] )) )
					{
						RPoolReclaim ( pool, BestBlock );
					}
				}
			}

			que_reclaimed = 1;
			goto try_again;
		}
		DPRINT1("Trying to allocate %lu bytes from paged pool - nothing suitable found, returning NULL\n",
			queBytes );
		R_RELEASE_MUTEX(pool);
		return NULL;
	}
	/*
	 * we found a best block. If Addr isn't already aligned, we've pre-qualified that
	 * there's room at the beginning of the block for a free block...
	 */
	{
		void* Addr = RHdrToBody(BestBlock);
		if ( BestAlignedAddr != Addr )
		{
			PR_FREE NewFreeBlock = RFreeSplit (
				pool,
				BestBlock,
				(char*)RBodyToHdr(BestAlignedAddr) - (char*)BestBlock );
			ASSERT ( BestAlignedAddr > Addr );

			//DPRINT ( "breaking off preceding bytes into their own block...\n" );
			/*DPRINT ( "NewFreeBlock 0x%x Size %lu (Old Block's new size %lu) NextFree 0x%x\n",
				NewFreeBlock, NewFreeBlock->Size, BestBlock->Size, BestBlock->NextFree );*/

			/* we want the following code to use our size-aligned block */
			BestPreviousBlock = BestBlock;
			BestBlock = NewFreeBlock;

			//VerifyPagedPool();
		}
	}
	/*
	 * Is there enough space to create a second block from the unused portion.
	 */
	if ( (BestBlock->Size - BlockSize) > sizeof(R_FREE) )
	{
		/*DPRINT("BestBlock 0x%x Size 0x%x BlockSize 0x%x NewSize 0x%x\n",
			BestBlock, BestBlock->Size, BlockSize, NewSize );*/

		/*
		 * Create the new free block.
		 */
		NextBlock = RFreeSplit ( pool, BestBlock, BlockSize );
		//ASSERT_SIZE ( NextBlock->Size );
	}
	/*
	 * Remove the selected block from the list of free blocks.
	 */
	//DPRINT ( "Removing selected block from free block list\n" );
	RPoolRemoveFree ( pool, BestBlock );
	/*
	 * Create the new used block header.
	 */
	NewBlock = (PR_USED)BestBlock;
	RiUsedInit ( NewBlock, Tag );

	/*  RtlZeroMemory(RHdrToBody(NewBlock), NumberOfBytes);*/

	RiUsedInitRedZone ( NewBlock, NumberOfBytes );
	R_RELEASE_MUTEX(pool);

	return RHdrToBody(NewBlock);
}

static void
RPoolFree ( PR_POOL pool, void* Addr )
{
	PR_USED UsedBlock;
	rulong UsedSize;
	PR_FREE FreeBlock;
	rulong UserSize;
	int que;

	ASSERT(pool);
	if ( !Addr )
	{
		R_DEBUG("Attempt to free NULL ptr, initiating Red Zone Check\n" );
		R_ACQUIRE_MUTEX(pool);
		RPoolRedZoneCheck ( pool, __FILE__, __LINE__ );
		R_RELEASE_MUTEX(pool);
		return;
	}
	R_ASSERT_PTR(pool,Addr);

	UsedBlock = RBodyToHdr(Addr);
	UsedSize = UsedBlock->Size;
	FreeBlock = (PR_FREE)UsedBlock;
#if R_RZ
	UserSize = UsedBlock->UserSize;
#else
	UserSize = UsedSize - sizeof(R_USED) - 2*R_RZ;
#endif//R_RZ

	RUsedRedZoneCheck ( pool, UsedBlock, Addr, __FILE__, __LINE__ );

#if R_RZ
	memset ( Addr, 0xCD, UsedBlock->UserSize );
#endif

	que = RQueWhich ( UserSize );
	if ( que >= 0 )
	{
		int queBytes = 16 << que;
		ASSERT( (rulong)queBytes >= UserSize );
		if ( que >= 0 )
		{
			int align = 0;
			if ( R_ROUND_UP(Addr,pool->Alignments[2]) == Addr )
				align = 2;
			else if ( R_ROUND_UP(Addr,pool->Alignments[1]) == Addr )
				align = 1;
			R_ACQUIRE_MUTEX(pool);
			RQueAdd ( &pool->Que[que][align], UsedBlock );
			R_RELEASE_MUTEX(pool);
			return;
		}
	}

	R_ACQUIRE_MUTEX(pool);
	RPoolReclaim ( pool, FreeBlock );
	R_RELEASE_MUTEX(pool);
}

#if 0
static void
RPoolDumpByTag ( PR_POOL pool, rulong Tag )
{
	PR_USED Block = (PR_USED)pool->UserBase;
	PR_USED NextBlock;
	int count = 0;
	char tag[5];

	// TODO FIXME - should we validate params or ASSERT_IRQL?
	R_DEBUG ( "PagedPool Dump by tag '%s'\n", RFormatTag(Tag,tag) );
	R_DEBUG ( "  -BLOCK-- --SIZE--\n" );

	R_ACQUIRE_MUTEX(pool);
	for ( ;; )
	{
		if ( Block->Status == 1 && Block->Tag == Tag )
		{
			R_DEBUG ( "  %08X %08X\n", Block, Block->Size );
			++count;
		}
		NextBlock = (PR_USED)RNextBlock(pool,(PR_FREE)Block);
		if ( !NextBlock )
			break;
		ASSERT ( NextBlock->PrevSize == Block->Size );
		Block = NextBlock;
	}
	R_RELEASE_MUTEX(pool);

	R_DEBUG ( "Entries found for tag '%s': %i\n", tag, count );
}
#endif

rulong
RPoolQueryTag ( void* Addr )
{
	PR_USED Block = RBodyToHdr(Addr);
	// TODO FIXME - should we validate params?
#ifdef R_MAGIC
	if ( Block->UsedMagic != R_USED_MAGIC )
		return 0xDEADBEEF;
#endif//R_MAGIC
	if ( Block->Status != 1 )
		return 0xDEADBEEF;
	return Block->Tag;
}

void
RPoolStats ( PR_POOL pool )
{
	int free=0, used=0, qued=0;
	PR_USED Block = (PR_USED)pool->UserBase;

	R_ACQUIRE_MUTEX(pool);
	while ( Block )
	{
		switch ( Block->Status )
		{
		case 0:
			++free;
			break;
		case 1:
			++used;
			break;
		case 2:
			++qued;
			break;
		default:
			ASSERT ( !"Invalid Status for Block in pool!" );
		}
		Block = (PR_USED)RNextBlock(pool,(PR_FREE)Block);
	}
	R_RELEASE_MUTEX(pool);

	R_DEBUG ( "Pool Stats: Free=%i, Used=%i, Qued=%i, Total=%i\n", free, used, qued, (free+used+qued) );
}

#ifdef R_LARGEST_ALLOC_POSSIBLE
static rulong
RPoolLargestAllocPossible ( PR_POOL pool, int align )
{
	int Alignment = pool->Alignments[align];
	rulong LargestUserSize = 0;
	PR_FREE Block = (PR_FREE)pool->UserBase;
	while ( Block )
	{
		if ( Block->Status != 1 )
		{
			void* Addr, *AlignedAddr;
			rulong BlockMaxUserSize;
			int cue, cueBytes;

			Addr = (char*)Block + sizeof(R_USED) + R_RZ;
			AlignedAddr = R_ROUND_UP(Addr,Alignment);
			if ( Addr != AlignedAddr )
				Addr = R_ROUND_UP((char*)Block + sizeof(R_FREE) + sizeof(R_USED) + R_RZ, Alignment );
			BlockMaxUserSize = (char*)Block + Block->Size - (char*)Addr - R_RZ;
			cue = RQueWhich ( BlockMaxUserSize );
			if ( cue >= 0 )
			{
				cueBytes = 16 << cue;
				if ( cueBytes > BlockMaxUserSize );
				{
					if ( !cue )
						BlockMaxUserSize = 0;
					else
						BlockMaxUserSize = 16 << (cue-1);
				}
			}
			if ( BlockMaxUserSize > LargestUserSize )
				LargestUserSize = BlockMaxUserSize;
		}
		Block = RNextBlock ( pool, Block );
	}
	return LargestUserSize;
}
#endif//R_LARGEST_ALLOC_POSSIBLE

#endif//RPOOLMGR_H
