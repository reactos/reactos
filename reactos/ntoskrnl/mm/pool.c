/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/pool.c
 * PURPOSE:         Implements the kernel memory pool
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <debug.h>

/* Uncomment to enable pool overruns debugging. Don't forget to increase
   max pool sizes (MM_[NON]PAGED_POOL_SIZE) in include/internal/mm.h */
//#define DEBUG_NPOOL
//#define DEBUG_PPOOL

extern PVOID MiNonPagedPoolStart;
extern ULONG MiNonPagedPoolLength;
extern ULONG MmTotalPagedPoolQuota;
extern ULONG MmTotalNonPagedPoolQuota;

/* FUNCTIONS ***************************************************************/

ULONG NTAPI
EiGetPagedPoolTag(IN PVOID Block);

ULONG NTAPI
EiGetNonPagedPoolTag(IN PVOID Block);

PVOID
NTAPI
ExAllocateArmPoolWithTag(POOL_TYPE PoolType,
                         SIZE_T NumberOfBytes,
                         ULONG Tag);

static PVOID NTAPI
EiAllocatePool(POOL_TYPE PoolType,
               ULONG NumberOfBytes,
               ULONG Tag,
               PVOID Caller)
{
   PVOID Block;
   PCHAR TagChars = (PCHAR)&Tag;

   if (Tag == 0)
       KeBugCheckEx(BAD_POOL_CALLER, 0x9b, PoolType, NumberOfBytes, (ULONG_PTR)Caller);
   if (Tag == ' GIB')
       KeBugCheckEx(BAD_POOL_CALLER, 0x9c, PoolType, NumberOfBytes, (ULONG_PTR)Caller);

#define IS_LETTER_OR_DIGIT(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') || ((c) >= '0' && (c) <= '9'))
   if (!IS_LETTER_OR_DIGIT(TagChars[0]) &&
       !IS_LETTER_OR_DIGIT(TagChars[1]) &&
       !IS_LETTER_OR_DIGIT(TagChars[2]) &&
       !IS_LETTER_OR_DIGIT(TagChars[3]))
       KeBugCheckEx(BAD_POOL_CALLER, 0x9d, Tag, PoolType, (ULONG_PTR)Caller);

    /* FIXME: Handle SESSION_POOL_MASK, VERIFIER_POOL_MASK, QUOTA_POOL_MASK */
    if (PoolType & PAGED_POOL_MASK)
    {
        if (KeGetCurrentIrql() > APC_LEVEL)
            KeBugCheckEx(BAD_POOL_CALLER, 0x08, KeGetCurrentIrql(), PoolType, Tag);
#ifdef DEBUG_PPOOL
        if (ExpIsPoolTagDebuggable(Tag))
            Block = ExpAllocateDebugPool(PoolType, NumberOfBytes, Tag, Caller, TRUE);
        else
#endif
            Block = ExAllocatePagedPoolWithTag(PoolType, NumberOfBytes, Tag);
    }
    else
    {
        if (KeGetCurrentIrql() > DISPATCH_LEVEL)
            KeBugCheckEx(BAD_POOL_CALLER, 0x08, KeGetCurrentIrql(), PoolType, Tag);
#ifdef DEBUG_NPOOL
        if (ExpIsPoolTagDebuggable(Tag))
            Block = ExpAllocateDebugPool(PoolType, NumberOfBytes, Tag, Caller, TRUE);
        else
#endif
            Block = ExAllocateArmPoolWithTag(PoolType, NumberOfBytes, Tag);
    }

    if ((PoolType & MUST_SUCCEED_POOL_MASK) && !Block)
        KeBugCheckEx(BAD_POOL_CALLER, 0x9a, PoolType, NumberOfBytes, Tag);
    return Block;
}

/*
 * @implemented
 */
PVOID NTAPI
ExAllocatePool (POOL_TYPE PoolType, SIZE_T NumberOfBytes)
/*
 * FUNCTION: Allocates pool memory of a specified type and returns a pointer
 * to the allocated block. This routine is used for general purpose allocation
 * of memory
 * ARGUMENTS:
 *        PoolType
 *               Specifies the type of memory to allocate which can be one
 *               of the following:
 *
 *               NonPagedPool
 *               NonPagedPoolMustSucceed
 *               NonPagedPoolCacheAligned
 *               NonPagedPoolCacheAlignedMustS
 *               PagedPool
 *               PagedPoolCacheAligned
 *
 *        NumberOfBytes
 *               Specifies the number of bytes to allocate
 * RETURNS: The allocated block on success
 *          NULL on failure
 */
{
   PVOID Block;

#if defined(__GNUC__)

   Block = EiAllocatePool(PoolType,
                          NumberOfBytes,
                          TAG_NONE,
                          (PVOID)__builtin_return_address(0));
#elif defined(_MSC_VER)

   Block = EiAllocatePool(PoolType,
                          NumberOfBytes,
                          TAG_NONE,
                          &ExAllocatePool);
#else
#error Unknown compiler
#endif

   return(Block);
}


/*
 * @implemented
 */
PVOID NTAPI
ExAllocatePoolWithTag (POOL_TYPE PoolType, SIZE_T NumberOfBytes, ULONG Tag)
{
   PVOID Block;

#if defined(__GNUC__)

   Block = EiAllocatePool(PoolType,
                          NumberOfBytes,
                          Tag,
                          (PVOID)__builtin_return_address(0));
#elif defined(_MSC_VER)

   Block = EiAllocatePool(PoolType,
                          NumberOfBytes,
                          Tag,
                          &ExAllocatePoolWithTag);
#else
#error Unknown compiler
#endif

   return(Block);
}

/*
 * @implemented
 */
#undef ExFreePool
VOID NTAPI
ExFreePool(IN PVOID Block)
{
    ExFreePoolWithTag(Block, 0);
}

VOID
NTAPI
ExFreeArmPoolWithTag(PVOID P,
                     ULONG TagToFree);

/*
 * @implemented
 */
VOID
NTAPI
ExFreePoolWithTag(
    IN PVOID Block,
    IN ULONG Tag)
{
    /* Check for paged pool */
    if (Block >= MmPagedPoolBase && 
        (char*)Block < ((char*)MmPagedPoolBase + MmPagedPoolSize))
    {
        /* Validate tag */
#ifndef DEBUG_PPOOL
        if (Tag != 0 && Tag != EiGetPagedPoolTag(Block))
            KeBugCheckEx(BAD_POOL_CALLER,
                         0x0a,
                         (ULONG_PTR)Block,
                         EiGetPagedPoolTag(Block),
                         Tag);
#endif
        /* Validate IRQL */
        if (KeGetCurrentIrql() > APC_LEVEL)
            KeBugCheckEx(BAD_POOL_CALLER,
                         0x09,
                         KeGetCurrentIrql(),
                         PagedPool,
                         (ULONG_PTR)Block);

        /* Free from paged pool */
#ifdef DEBUG_PPOOL
        if (ExpIsPoolTagDebuggable(Tag))
            ExpFreeDebugPool(Block, TRUE);
        else
#endif
            ExFreePagedPool(Block);
    }
    else if (Block) ExFreeArmPoolWithTag(Block, Tag);
    else
    {
        /* Only warn and break for NULL pointers */
        if (Block == NULL)
        {
            DPRINT1("Warning: Trying to free a NULL pointer!\n");
            ASSERT(FALSE);
            return;
        }

        /* Block was not inside any pool! */
        KeBugCheckEx(BAD_POOL_CALLER, 0x42, (ULONG_PTR)Block, 0, 0);
    }
}

/* EOF */
