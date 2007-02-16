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

    /* Charge the usage */
    Status = PsChargeProcessPoolQuota(Process, PoolType, Amount);

    /* Raise Exception */
    if (!NT_SUCCESS(Status))
    {
        ExRaiseStatus(Status);
    }
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

/*
 * @implemented
 */
NTSTATUS
STDCALL
PsChargeProcessPoolQuota(IN PEPROCESS Process,
                         IN POOL_TYPE PoolType,
                         IN ULONG Amount)
{
    PEPROCESS_QUOTA_BLOCK QuotaBlock;
    ULONG NewUsageSize;
    ULONG NewMaxQuota;

    /* Get current Quota Block */
    QuotaBlock = Process->QuotaBlock;

    /* Quota Operations are not to be done on the SYSTEM Process */
    if (Process == PsInitialSystemProcess) return STATUS_SUCCESS;

    /* New Size in use */
    NewUsageSize = QuotaBlock->QuotaEntry[PoolType].Usage + Amount;

    /* Does this size respect the quota? */
    if (NewUsageSize > QuotaBlock->QuotaEntry[PoolType].Limit)
    {
        /* It doesn't, so keep raising the Quota */
        while (MiRaisePoolQuota(PoolType,
               QuotaBlock->QuotaEntry[PoolType].Limit,
               &NewMaxQuota))
        {
            /* Save new Maximum Quota */
            QuotaBlock->QuotaEntry[PoolType].Limit = NewMaxQuota;

            /* See if the new Maximum Quota fulfills our need */
            if (NewUsageSize <= NewMaxQuota) goto QuotaChanged;
        }

        return STATUS_QUOTA_EXCEEDED;
    }

QuotaChanged:
    /* Save new Usage */
    QuotaBlock->QuotaEntry[PoolType].Usage = NewUsageSize;

    /* Is this a new peak? */
    if (NewUsageSize > QuotaBlock->QuotaEntry[PoolType].Peak)
    {
        QuotaBlock->QuotaEntry[PoolType].Peak = NewUsageSize;
    }

    /* All went well */
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
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
PsReturnProcessNonPagedPoolQuota(IN PEPROCESS Process,
                                 IN ULONG_PTR Amount)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
PsReturnProcessPagedPoolQuota(IN PEPROCESS Process,
                              IN ULONG_PTR Amount)
{
    UNIMPLEMENTED;
}

/* EOF */
