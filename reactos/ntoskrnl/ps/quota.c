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

NTSTATUS
NTAPI
PsChargeProcessPageFileQuota(IN PEPROCESS Process,
                             IN SIZE_T Amount)
{
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return STATUS_SUCCESS;
    
    /* Otherwise, not implemented */
    UNIMPLEMENTED;
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

/*
 * @implemented
 */
NTSTATUS
STDCALL
PsChargeProcessPoolQuota(IN PEPROCESS Process,
                         IN POOL_TYPE PoolType,
                         IN ULONG Amount)
{
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return STATUS_SUCCESS;

    UNIMPLEMENTED;
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
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return;

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
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return;

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
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return;

    UNIMPLEMENTED;
}

NTSTATUS
NTAPI
PsReturnProcessPageFileQuota(IN PEPROCESS Process,
                             IN SIZE_T Amount)
{
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return STATUS_SUCCESS;
    
    /* Otherwise, not implemented */
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

/* EOF */
