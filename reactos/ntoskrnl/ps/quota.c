/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/quota.c
 * PURPOSE:         Process Pool Quotas
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES **************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

EPROCESS_QUOTA_BLOCK PspDefaultQuotaBlock;


/* Define this macro to enable quota code testing. Once quota code is */
/* stable and verified, remove this macro and checks for it. */
/*#define PS_QUOTA_ENABLE_QUOTA_CODE*/


/* FUNCTIONS ***************************************************************/

VOID
NTAPI
PsInitializeQuotaSystem(VOID)
{
    RtlZeroMemory(&PspDefaultQuotaBlock, sizeof(PspDefaultQuotaBlock));
    PspDefaultQuotaBlock.QuotaEntry[PagedPool].Limit = (SIZE_T)-1;
    PspDefaultQuotaBlock.QuotaEntry[NonPagedPool].Limit = (SIZE_T)-1;
    PspDefaultQuotaBlock.QuotaEntry[2].Limit = (SIZE_T)-1; /* Page file */
    PsGetCurrentProcess()->QuotaBlock = &PspDefaultQuotaBlock;
}

VOID
STDCALL
PspInheritQuota(PEPROCESS Process, PEPROCESS ParentProcess)
{
    if (ParentProcess != NULL)
    {
        PEPROCESS_QUOTA_BLOCK QuotaBlock = ParentProcess->QuotaBlock;
        
        ASSERT(QuotaBlock != NULL);

        (void)InterlockedIncrementUL(&QuotaBlock->ReferenceCount);
        
        Process->QuotaBlock = QuotaBlock;
    }
    else
        Process->QuotaBlock = &PspDefaultQuotaBlock;
}

VOID
STDCALL
PspDestroyQuotaBlock(PEPROCESS Process)
{
    PEPROCESS_QUOTA_BLOCK QuotaBlock = Process->QuotaBlock;

    if (QuotaBlock != &PspDefaultQuotaBlock &&
        InterlockedDecrementUL(&QuotaBlock->ReferenceCount) == 0)
    {
        ExFreePool(QuotaBlock);
    }
}

NTSTATUS
NTAPI
PsChargeProcessPageFileQuota(IN PEPROCESS Process,
                             IN SIZE_T Amount)
{
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return STATUS_SUCCESS;

#ifdef PS_QUOTA_ENABLE_QUOTA_CODE
    if (Process)
    {
        /* TODO: Check with Process->QuotaBlock if this can be satisfied, */
        /* assuming this indeed is the place to check it. */
        /* Probably something like:
        if (Process->QuotaUsage[2] + Amount >
            Process->QuotaBlock->QuotaEntry[2].Limit)
        {
            refuse
        }
        */
        Process->QuotaUsage[2] += Amount;
        if (Process->QuotaPeak[2] < Process->QuotaUsage[2])
        {
            Process->QuotaPeak[2] = Process->QuotaUsage[2];
        }
    }
#else
    /* Otherwise, not implemented */
    UNIMPLEMENTED;
#endif
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
STDCALL
PsChargePoolQuota(IN PEPROCESS Process,
                  IN POOL_TYPE PoolType,
                  IN ULONG Amount)
{
    NTSTATUS Status;

    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return;
    
    /* Charge the usage */
    Status = PsChargeProcessPoolQuota(Process, PoolType, Amount);
    if (!NT_SUCCESS(Status)) ExRaiseStatus(Status);
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
PsChargeProcessNonPagedPoolQuota(IN PEPROCESS Process,
                                 IN ULONG_PTR Amount)
{
    /* Call the general function */
    return PsChargeProcessPoolQuota(Process, NonPagedPool, Amount);
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
PsChargeProcessPagedPoolQuota(IN PEPROCESS Process,
                              IN ULONG_PTR Amount)
{
    /* Call the general function */
    return PsChargeProcessPoolQuota(Process, PagedPool, Amount);
}

#ifdef PS_QUOTA_ENABLE_QUOTA_CODE
/*
 * Internal helper function.
 * Returns the index of the Quota* member in EPROCESS for
 * a specified pool type, or -1 on failure.
 */
static
INT
PspPoolQuotaIndexFromPoolType(POOL_TYPE PoolType)
{
    switch (PoolType)
    {
        case NonPagedPool:
        case NonPagedPoolMustSucceed:
        case NonPagedPoolCacheAligned:
        case NonPagedPoolCacheAlignedMustS:
        case NonPagedPoolSession:
        case NonPagedPoolMustSucceedSession:
        case NonPagedPoolCacheAlignedSession:
        case NonPagedPoolCacheAlignedMustSSession:
            return 1;
        case PagedPool:
        case PagedPoolCacheAligned:
        case PagedPoolSession:
        case PagedPoolCacheAlignedSession:
            return 0;
        default:
            return -1;
    }
}
#endif

/*
 * @implemented
 */
NTSTATUS
STDCALL
PsChargeProcessPoolQuota(IN PEPROCESS Process,
                         IN POOL_TYPE PoolType,
                         IN ULONG Amount)
{
    INT PoolIndex;
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return STATUS_SUCCESS;

#ifdef PS_QUOTA_ENABLE_QUOTA_CODE
    PoolIndex = PspPoolQuotaIndexFromPoolType(PoolType);
    if (Process && PoolIndex != -1)
    {
        /* TODO: Check with Process->QuotaBlock if this can be satisfied, */
        /* assuming this indeed is the place to check it. */
        /* Probably something like:
        if (Process->QuotaUsage[PoolIndex] + Amount >
            Process->QuotaBlock->QuotaEntry[PoolIndex].Limit)
        {
            refuse
        }
        */
        Process->QuotaUsage[PoolIndex] += Amount;
        if (Process->QuotaPeak[PoolIndex] < Process->QuotaUsage[PoolIndex])
        {
            Process->QuotaPeak[PoolIndex] = Process->QuotaUsage[PoolIndex];
        }
    }
#else
    UNIMPLEMENTED;
#endif
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
VOID
STDCALL
PsReturnPoolQuota(IN PEPROCESS Process,
                  IN POOL_TYPE PoolType,
                  IN ULONG_PTR Amount)
{
    INT PoolIndex;
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return;

#ifdef PS_QUOTA_ENABLE_QUOTA_CODE
    PoolIndex = PspPoolQuotaIndexFromPoolType(PoolType);
    if (Process && PoolIndex != -1)
    {
        if (Process->QuotaUsage[PoolIndex] < Amount)
        {
            DPRINT1("WARNING: Process->QuotaUsage sanity check failed.\n");
        }
        else
        {
            Process->QuotaUsage[PoolIndex] -= Amount;
        }
    }
#else
    UNIMPLEMENTED;
#endif
}

/*
 * @unimplemented
 */
VOID
STDCALL
PsReturnProcessNonPagedPoolQuota(IN PEPROCESS Process,
                                 IN ULONG_PTR Amount)
{
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return;

#ifdef PS_QUOTA_ENABLE_QUOTA_CODE
    PsReturnPoolQuota(Process, NonPagedPool, Amount);
#else
    UNIMPLEMENTED;
#endif
}

/*
 * @unimplemented
 */
VOID
STDCALL
PsReturnProcessPagedPoolQuota(IN PEPROCESS Process,
                              IN ULONG_PTR Amount)
{
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return;

#ifdef PS_QUOTA_ENABLE_QUOTA_CODE
    PsReturnPoolQuota(Process, PagedPool, Amount);
#else
    UNIMPLEMENTED;
#endif
}

NTSTATUS
NTAPI
PsReturnProcessPageFileQuota(IN PEPROCESS Process,
                             IN SIZE_T Amount)
{
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return STATUS_SUCCESS;
    
#ifdef PS_QUOTA_ENABLE_QUOTA_CODE
    if (Process)
    {
        if (Process->QuotaUsage[2] < Amount)
        {
            DPRINT1("WARNING: Process PageFileQuotaUsage sanity check failed.\n");
        }
        else
        {
            Process->QuotaUsage[2] -= Amount;
        }
    }
#else
    /* Otherwise, not implemented */
    UNIMPLEMENTED;
#endif
    return STATUS_SUCCESS;
}

/* EOF */
