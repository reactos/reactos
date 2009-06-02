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
extern MM_STATS MmStats;

/* FUNCTIONS ***************************************************************/

ULONG NTAPI
EiGetPagedPoolTag(IN PVOID Block);

ULONG NTAPI
EiGetNonPagedPoolTag(IN PVOID Block);

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
   if (Tag == TAG('B','I','G',0))
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
            Block = ExAllocateNonPagedPoolWithTag(PoolType, NumberOfBytes, Tag, Caller);
    }

    if ((PoolType & MUST_SUCCEED_POOL_MASK) && !Block)
        KeBugCheckEx(BAD_POOL_CALLER, 0x9a, PoolType, NumberOfBytes, Tag);
    return Block;
}

/*
 * @implemented
 */
PVOID NTAPI
ExAllocatePool (POOL_TYPE PoolType, ULONG NumberOfBytes)
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
ExAllocatePoolWithTag (POOL_TYPE PoolType, ULONG NumberOfBytes, ULONG Tag)
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
#undef ExAllocatePoolWithQuota
PVOID NTAPI
ExAllocatePoolWithQuota (POOL_TYPE PoolType, ULONG NumberOfBytes)
{
   return(ExAllocatePoolWithQuotaTag(PoolType, NumberOfBytes, TAG_NONE));
}

/*
 * @implemented
 */
PVOID
NTAPI
ExAllocatePoolWithTagPriority(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN EX_POOL_PRIORITY Priority
    )
{
    /* Check if this is one of the "Special" Flags, used by the Verifier */
    if (Priority & 8) {
        /* Check if this is a xxSpecialUnderrun */
        if (Priority & 1) {
            return MiAllocateSpecialPool(PoolType, NumberOfBytes, Tag, 1);
        } else { /* xxSpecialOverrun */
            return MiAllocateSpecialPool(PoolType, NumberOfBytes, Tag, 0);
        }
    }

    /* FIXME: Do Ressource Checking Based on Priority and fail if resources too low*/

    /* Do the allocation */
    return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
}

/*
 * @implemented
 */
#undef ExAllocatePoolWithQuotaTag
PVOID
NTAPI
ExAllocatePoolWithQuotaTag (IN POOL_TYPE PoolType,
                            IN ULONG NumberOfBytes,
                            IN ULONG Tag)
{
    PEPROCESS Process;
    PVOID Block;

    /* Allocate the Pool First */
    Block = EiAllocatePool(PoolType,
                           NumberOfBytes,
                           Tag,
                           &ExAllocatePoolWithQuotaTag);

    /* "Quota is not charged to the thread for allocations >= PAGE_SIZE" - OSR Docs */
    if (!(NumberOfBytes >= PAGE_SIZE))
    {
        /* Get the Current Process */
        Process = PsGetCurrentProcess();

        /* PsChargePoolQuota returns an exception, so this needs SEH */
        _SEH2_TRY
        {
            /* FIXME: Is there a way to get the actual Pool size allocated from the pool header? */
            PsChargePoolQuota(Process,
                              PoolType & PAGED_POOL_MASK,
                              NumberOfBytes);
        }
        _SEH2_EXCEPT((ExFreePool(Block), EXCEPTION_CONTINUE_SEARCH))
        {
            /* Quota Exceeded and the caller had no SEH! */
            KeBugCheck(STATUS_QUOTA_EXCEEDED);
        }
        _SEH2_END;
    }

    /* Return the allocated block */
    return Block;
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
#if 0
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

    /* Check for non-paged pool */
    else if (Block >= MiNonPagedPoolStart &&
             (char*)Block < ((char*)MiNonPagedPoolStart + MiNonPagedPoolLength))
    {
        /* Validate tag */
        /*if (Tag != 0 && Tag != EiGetNonPagedPoolTag(Block))
            KeBugCheckEx(BAD_POOL_CALLER,
                         0x0a,
                         (ULONG_PTR)Block,
                         EiGetNonPagedPoolTag(Block),
                         Tag);*/

        /* Validate IRQL */
        if (KeGetCurrentIrql() > DISPATCH_LEVEL)
            KeBugCheckEx(BAD_POOL_CALLER,
                         0x09,
                         KeGetCurrentIrql(),
                         NonPagedPool,
                         (ULONG_PTR)Block);

        /* Free from non-paged pool */
#ifdef DEBUG_NPOOL
        if (ExpIsPoolTagDebuggable(Tag))
            ExpFreeDebugPool(Block, FALSE);
        else
#endif
            ExFreeNonPagedPool(Block);
    }
    else
    {
        /* Warn only for NULL pointers */
        if (Block == NULL)
        {
            DPRINT1("Warning: Trying to free a NULL pointer!\n");
            return;
        }

        /* Block was not inside any pool! */
        KeBugCheckEx(BAD_POOL_CALLER, 0x42, (ULONG_PTR)Block, 0, 0);
    }
}

/*
 * @unimplemented
 */
SIZE_T
NTAPI
ExQueryPoolBlockSize (
    IN PVOID PoolBlock,
    OUT PBOOLEAN QuotaCharged
    )
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
PVOID
NTAPI
MmAllocateMappingAddress (
     IN SIZE_T NumberOfBytes,
     IN ULONG PoolTag
     )
{
	UNIMPLEMENTED;
	return 0;
}


/*
 * @unimplemented
 */
VOID
NTAPI
MmFreeMappingAddress (
     IN PVOID BaseAddress,
     IN ULONG PoolTag
     )
{
	UNIMPLEMENTED;
}

BOOLEAN
NTAPI
MiRaisePoolQuota(
    IN POOL_TYPE PoolType,
    IN ULONG CurrentMaxQuota,
    OUT PULONG NewMaxQuota
    )
{
    /* Different quota raises depending on the type (64K vs 512K) */
    if (PoolType == PagedPool) {

        /* Make sure that 4MB is still left */
        if ((MM_PAGED_POOL_SIZE >> 12) < ((MmPagedPoolSize + 4194304) >> 12)) {
            return FALSE;
        }

        /* Increase Paged Pool Quota by 512K */
        MmTotalPagedPoolQuota += 524288;
        *NewMaxQuota = CurrentMaxQuota + 524288;
        return TRUE;

    } else { /* Nonpaged Pool */

        /* Check if we still have 200 pages free*/
        if (MmStats.NrFreePages < 200) return FALSE;

        /* Check that 4MB is still left */
        if ((MM_NONPAGED_POOL_SIZE >> 12) < ((MiNonPagedPoolLength + 4194304) >> 12)) {
            return FALSE;
        }

        /* Increase Non Paged Pool Quota by 64K */
        MmTotalNonPagedPoolQuota += 65536;
        *NewMaxQuota = CurrentMaxQuota + 65536;
        return TRUE;
    }
}

/* EOF */
