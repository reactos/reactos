/* $Id$
 *
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
#include <internal/debug.h>

extern ULONG MiNonPagedPoolLength;
extern ULONG MmTotalPagedPoolQuota;
extern ULONG MmTotalNonPagedPoolQuota;
extern MM_STATS MmStats;

/* FUNCTIONS ***************************************************************/

static PVOID STDCALL
EiAllocatePool(POOL_TYPE PoolType,
               ULONG NumberOfBytes,
               ULONG Tag,
               PVOID Caller)
{
   PVOID Block;

   /* FIXME: Handle SESSION_POOL_MASK, VERIFIER_POOL_MASK, QUOTA_POOL_MASK */
   if (PoolType & PAGED_POOL_MASK)
   {
      Block = ExAllocatePagedPoolWithTag(PoolType,NumberOfBytes,Tag);
   }
   else
   {
      Block = ExAllocateNonPagedPoolWithTag(PoolType,NumberOfBytes,Tag,Caller);
   }

   if ((PoolType & MUST_SUCCEED_POOL_MASK) && Block==NULL)
   {
      KEBUGCHECK(MUST_SUCCEED_POOL_EMPTY);
   }
   return(Block);
}

/*
 * @implemented
 */
PVOID STDCALL
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
PVOID STDCALL
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
PVOID STDCALL
ExAllocatePoolWithQuota (POOL_TYPE PoolType, ULONG NumberOfBytes)
{
   return(ExAllocatePoolWithQuotaTag(PoolType, NumberOfBytes, TAG_NONE));
}

/*
 * @implemented
 */
PVOID
STDCALL
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

_SEH_DEFINE_LOCALS(ExQuotaPoolVars)
{
    PVOID Block;
};

_SEH_FILTER(FreeAndGoOn)
{
    _SEH_ACCESS_LOCALS(ExQuotaPoolVars);

    /* Couldn't charge, so free the pool and let the caller SEH manage */
    ExFreePool(_SEH_VAR(Block));
    return EXCEPTION_CONTINUE_SEARCH;
}

/*
 * @implemented
 */
PVOID
NTAPI
ExAllocatePoolWithQuotaTag (IN POOL_TYPE PoolType,
                            IN ULONG NumberOfBytes,
                            IN ULONG Tag)
{
    PEPROCESS Process;
    _SEH_DECLARE_LOCALS(ExQuotaPoolVars);

    /* Allocate the Pool First */
    _SEH_VAR(Block) = EiAllocatePool(PoolType,
                                     NumberOfBytes,
                                     Tag,
                                     &ExAllocatePoolWithQuotaTag);

    /* "Quota is not charged to the thread for allocations >= PAGE_SIZE" - OSR Docs */
    if (!(NumberOfBytes >= PAGE_SIZE))
    {
        /* Get the Current Process */
        Process = PsGetCurrentProcess();

        /* PsChargePoolQuota returns an exception, so this needs SEH */
        _SEH_TRY
        {
            /* FIXME: Is there a way to get the actual Pool size allocated from the pool header? */
            PsChargePoolQuota(Process,
                              PoolType & PAGED_POOL_MASK,
                              NumberOfBytes);
        }
        _SEH_EXCEPT(FreeAndGoOn)
        {
            /* Quota Exceeded and the caller had no SEH! */
            KeBugCheck(STATUS_QUOTA_EXCEEDED);
        }
        _SEH_END;
    }

    /* Return the allocated block */
    return _SEH_VAR(Block);
}

/*
 * @implemented
 */
#undef ExFreePool
VOID STDCALL
ExFreePool(IN PVOID Block)
{
   ASSERT_IRQL(DISPATCH_LEVEL);

   if (Block >= MmPagedPoolBase && (char*)Block < ((char*)MmPagedPoolBase + MmPagedPoolSize))
   {
      ExFreePagedPool(Block);
   }
   else
   {
      ExFreeNonPagedPool(Block);
   }
}

/*
 * @implemented
 */
VOID STDCALL
ExFreePoolWithTag(IN PVOID Block, IN ULONG Tag)
{
   /* FIXME: Validate the tag */
   ExFreePool(Block);
}

/*
 * @unimplemented
 */
SIZE_T
STDCALL
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
STDCALL
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
STDCALL
MmFreeMappingAddress (
     IN PVOID BaseAddress,
     IN ULONG PoolTag
     )
{
	UNIMPLEMENTED;
}

BOOLEAN
STDCALL
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
