/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/quota.c
 * PURPOSE:         Process Pool Quotas
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Mike Nordell
 */

/* INCLUDES **************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

EPROCESS_QUOTA_BLOCK PspDefaultQuotaBlock;
static LIST_ENTRY PspQuotaBlockList = {&PspQuotaBlockList, &PspQuotaBlockList};
static KSPIN_LOCK PspQuotaLock;

#define TAG_QUOTA_BLOCK 'bQsP'
#define VALID_QUOTA_FLAGS (QUOTA_LIMITS_HARDWS_MIN_ENABLE | \
                           QUOTA_LIMITS_HARDWS_MIN_DISABLE | \
                           QUOTA_LIMITS_HARDWS_MAX_ENABLE | \
                           QUOTA_LIMITS_HARDWS_MAX_DISABLE)

/* PRIVATE FUNCTIONS *******************************************************/

/*
 * Private helper to charge the specified process quota.
 * Returns STATUS_QUOTA_EXCEEDED on quota limit check failure.
 * Updates QuotaPeak as needed for specified quota type in PS_QUOTA_TYPE enum.
 * Notes: Conceptually translation unit local/private.
 */
NTSTATUS
NTAPI
PspChargeProcessQuotaSpecifiedPool(IN PEPROCESS Process,
                                   IN PS_QUOTA_TYPE QuotaType,
                                   IN SIZE_T    Amount)
{
    KIRQL OldIrql;
    ASSERT(Process);
    ASSERT(Process != PsInitialSystemProcess);
    ASSERT(QuotaType < PsQuotaTypes);
    ASSERT(Process->QuotaBlock);

    /* Guard our quota in a spin lock */
    KeAcquireSpinLock(&PspQuotaLock, &OldIrql);

    if (Process->QuotaUsage[QuotaType] + Amount >
        Process->QuotaBlock->QuotaEntry[QuotaType].Limit)
    {
        DPRINT1("Quota exceeded, but ROS will let it slide...\n");
        KeReleaseSpinLock(&PspQuotaLock, OldIrql);
        return STATUS_SUCCESS;
        //return STATUS_QUOTA_EXCEEDED; /* caller raises the exception */
    }

    InterlockedExchangeAdd((LONG*)&Process->QuotaUsage[QuotaType], Amount);

    if (Process->QuotaPeak[QuotaType] < Process->QuotaUsage[QuotaType])
    {
        Process->QuotaPeak[QuotaType] = Process->QuotaUsage[QuotaType];
    }

    KeReleaseSpinLock(&PspQuotaLock, OldIrql);
    return STATUS_SUCCESS;
}

/*
 * Private helper to remove quota charge from the specified process quota.
 * Notes: Conceptually translation unit local/private.
 */
VOID
NTAPI
PspReturnProcessQuotaSpecifiedPool(IN PEPROCESS Process,
                                   IN PS_QUOTA_TYPE QuotaType,
                                   IN SIZE_T    Amount)
{
    KIRQL OldIrql;
    ASSERT(Process);
    ASSERT(Process != PsInitialSystemProcess);
    ASSERT(QuotaType < PsQuotaTypes);
    ASSERT(!(Amount & 0x80000000)); /* we need to be able to negate it */

    /* Guard our quota in a spin lock */
    KeAcquireSpinLock(&PspQuotaLock, &OldIrql);

    if (Process->QuotaUsage[QuotaType] < Amount)
    {
        DPRINT1("WARNING: Process->QuotaUsage sanity check failed.\n");
    }
    else
    {
        InterlockedExchangeAdd((LONG*)&Process->QuotaUsage[QuotaType],
                               -(LONG)Amount);
    }

    KeReleaseSpinLock(&PspQuotaLock, OldIrql);
}

/* FUNCTIONS ***************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
PsInitializeQuotaSystem(VOID)
{
    RtlZeroMemory(&PspDefaultQuotaBlock, sizeof(PspDefaultQuotaBlock));
    PspDefaultQuotaBlock.QuotaEntry[PsNonPagedPool].Limit = (SIZE_T)-1;
    PspDefaultQuotaBlock.QuotaEntry[PsPagedPool].Limit = (SIZE_T)-1;
    PspDefaultQuotaBlock.QuotaEntry[PsPageFile].Limit = (SIZE_T)-1;
    PsGetCurrentProcess()->QuotaBlock = &PspDefaultQuotaBlock;
}

VOID
NTAPI
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
NTAPI
PspInsertQuotaBlock(
    PEPROCESS_QUOTA_BLOCK QuotaBlock)
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&PspQuotaLock, &OldIrql);
    InsertTailList(&PspQuotaBlockList, &QuotaBlock->QuotaList);
    KeReleaseSpinLock(&PspQuotaLock, OldIrql);
}

VOID
NTAPI
PspDestroyQuotaBlock(PEPROCESS Process)
{
    PEPROCESS_QUOTA_BLOCK QuotaBlock = Process->QuotaBlock;
    KIRQL OldIrql;

    if (QuotaBlock != &PspDefaultQuotaBlock &&
        InterlockedDecrementUL(&QuotaBlock->ReferenceCount) == 0)
    {
        KeAcquireSpinLock(&PspQuotaLock, &OldIrql);
        RemoveEntryList(&QuotaBlock->QuotaList);
        KeReleaseSpinLock(&PspQuotaLock, OldIrql);
        ExFreePool(QuotaBlock);
    }
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PsChargeProcessPageFileQuota(IN PEPROCESS Process,
                             IN SIZE_T Amount)
{
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return STATUS_SUCCESS;

    return PspChargeProcessQuotaSpecifiedPool(Process, PsPageFile, Amount);
}

/*
 * @implemented
 */
VOID
NTAPI
PsChargePoolQuota(IN PEPROCESS Process,
                  IN POOL_TYPE PoolType,
                  IN SIZE_T Amount)
{
    NTSTATUS Status;
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

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
NTAPI
PsChargeProcessNonPagedPoolQuota(IN PEPROCESS Process,
                                 IN SIZE_T Amount)
{
    /* Call the general function */
    return PsChargeProcessPoolQuota(Process, NonPagedPool, Amount);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PsChargeProcessPagedPoolQuota(IN PEPROCESS Process,
                              IN SIZE_T Amount)
{
    /* Call the general function */
    return PsChargeProcessPoolQuota(Process, PagedPool, Amount);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PsChargeProcessPoolQuota(IN PEPROCESS Process,
                         IN POOL_TYPE PoolType,
                         IN SIZE_T Amount)
{
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return STATUS_SUCCESS;

    return PspChargeProcessQuotaSpecifiedPool(Process,
                                              (PoolType & PAGED_POOL_MASK),
                                              Amount);
}

/*
 * @implemented
 */
VOID
NTAPI
PsReturnPoolQuota(IN PEPROCESS Process,
                  IN POOL_TYPE PoolType,
                  IN SIZE_T Amount)
{
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return;

    PspReturnProcessQuotaSpecifiedPool(Process,
                                       (PoolType & PAGED_POOL_MASK),
                                       Amount);
}

/*
 * @implemented
 */
VOID
NTAPI
PsReturnProcessNonPagedPoolQuota(IN PEPROCESS Process,
                                 IN SIZE_T Amount)
{
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return;

    PsReturnPoolQuota(Process, NonPagedPool, Amount);
}

/*
 * @implemented
 */
VOID
NTAPI
PsReturnProcessPagedPoolQuota(IN PEPROCESS Process,
                              IN SIZE_T Amount)
{
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return;

    PsReturnPoolQuota(Process, PagedPool, Amount);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PsReturnProcessPageFileQuota(IN PEPROCESS Process,
                             IN SIZE_T Amount)
{
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return STATUS_SUCCESS;

    PspReturnProcessQuotaSpecifiedPool(Process, PsPageFile, Amount);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PspSetQuotaLimits(
    _In_ PEPROCESS Process,
    _In_ ULONG Unused,
    _In_ PVOID QuotaLimits,
    _In_ ULONG QuotaLimitsLength,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    QUOTA_LIMITS_EX CapturedQuotaLimits;
    PEPROCESS_QUOTA_BLOCK QuotaBlock, OldQuotaBlock;
    BOOLEAN IncreaseOkay;
    KAPC_STATE SavedApcState;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Unused);

    _SEH2_TRY
    {
        ProbeForRead(QuotaLimits, QuotaLimitsLength, sizeof(ULONG));

        /* Check if we have the basic or extended structure */
        if (QuotaLimitsLength == sizeof(QUOTA_LIMITS))
        {
            /* Copy the basic structure, zero init the remaining fields */
            RtlCopyMemory(&CapturedQuotaLimits, QuotaLimits, sizeof(QUOTA_LIMITS));
            CapturedQuotaLimits.WorkingSetLimit = 0;
            CapturedQuotaLimits.Reserved2 = 0;
            CapturedQuotaLimits.Reserved3 = 0;
            CapturedQuotaLimits.Reserved4 = 0;
            CapturedQuotaLimits.CpuRateLimit.RateData = 0;
            CapturedQuotaLimits.Flags = 0;
        }
        else if (QuotaLimitsLength == sizeof(QUOTA_LIMITS_EX))
        {
            /* Copy the full structure */
            RtlCopyMemory(&CapturedQuotaLimits, QuotaLimits, sizeof(QUOTA_LIMITS_EX));

            /* Verify that the caller passed valid flags */
            if ((CapturedQuotaLimits.Flags & ~VALID_QUOTA_FLAGS) ||
                ((CapturedQuotaLimits.Flags & QUOTA_LIMITS_HARDWS_MIN_ENABLE) &&
                 (CapturedQuotaLimits.Flags & QUOTA_LIMITS_HARDWS_MIN_DISABLE)) ||
                ((CapturedQuotaLimits.Flags & QUOTA_LIMITS_HARDWS_MAX_ENABLE) &&
                 (CapturedQuotaLimits.Flags & QUOTA_LIMITS_HARDWS_MAX_DISABLE)))
            {
                DPRINT1("Invalid quota flags: 0x%lx\n", CapturedQuotaLimits.Flags);
                _SEH2_YIELD(return STATUS_INVALID_PARAMETER);
            }

            /* Verify that the caller didn't pass reserved values */
            if ((CapturedQuotaLimits.WorkingSetLimit != 0) ||
                (CapturedQuotaLimits.Reserved2 != 0) ||
                (CapturedQuotaLimits.Reserved3 != 0) ||
                (CapturedQuotaLimits.Reserved4 != 0) ||
                (CapturedQuotaLimits.CpuRateLimit.RateData != 0))
            {
                DPRINT1("Invalid value: (%lx,%lx,%lx,%lx,%lx)\n",
                        CapturedQuotaLimits.WorkingSetLimit,
                        CapturedQuotaLimits.Reserved2,
                        CapturedQuotaLimits.Reserved3,
                        CapturedQuotaLimits.Reserved4,
                        CapturedQuotaLimits.CpuRateLimit.RateData);
                _SEH2_YIELD(return STATUS_INVALID_PARAMETER);
            }
        }
        else
        {
            DPRINT1("Invalid quota size: 0x%lx\n", QuotaLimitsLength);
            _SEH2_YIELD(return STATUS_INFO_LENGTH_MISMATCH);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("Exception while copying data\n");
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Check the caller changes the working set size limits */
    if ((CapturedQuotaLimits.MinimumWorkingSetSize != 0) &&
        (CapturedQuotaLimits.MaximumWorkingSetSize != 0))
    {
        /* Check for special case: trimming the WS */
        if ((CapturedQuotaLimits.MinimumWorkingSetSize == SIZE_T_MAX) &&
            (CapturedQuotaLimits.MaximumWorkingSetSize == SIZE_T_MAX))
        {
            /* No increase allowed */
            IncreaseOkay = FALSE;
        }
        else
        {
            /* Check if the caller has the required privilege */
            IncreaseOkay = SeSinglePrivilegeCheck(SeIncreaseQuotaPrivilege,
                                                  PreviousMode);
        }

        /* Attach to the target process and disable APCs */
        KeStackAttachProcess(&Process->Pcb, &SavedApcState);
        KeEnterGuardedRegion();

        /* Call Mm to adjust the process' working set size */
        Status = MmAdjustWorkingSetSize(CapturedQuotaLimits.MinimumWorkingSetSize,
                                        CapturedQuotaLimits.MaximumWorkingSetSize,
                                        0,
                                        IncreaseOkay);

        /* Bring back APCs and detach from the process */
        KeLeaveGuardedRegion();
        KeUnstackDetachProcess(&SavedApcState);
    }
    else if (Process->QuotaBlock == &PspDefaultQuotaBlock)
    {
        /* Check if the caller has the required privilege */
        if (!SeSinglePrivilegeCheck(SeIncreaseQuotaPrivilege, PreviousMode))
        {
            return STATUS_PRIVILEGE_NOT_HELD;
        }

        /* Allocate a new quota block */
        QuotaBlock = ExAllocatePoolWithTag(NonPagedPool,
                                           sizeof(EPROCESS_QUOTA_BLOCK),
                                           TAG_QUOTA_BLOCK);
        if (QuotaBlock == NULL)
        {
            ObDereferenceObject(Process);
            return STATUS_NO_MEMORY;
        }

        /* Initialize the quota block */
        QuotaBlock->ReferenceCount = 1;
        QuotaBlock->ProcessCount = 1;
        QuotaBlock->QuotaEntry[PsNonPagedPool].Peak = Process->QuotaPeak[PsNonPagedPool];
        QuotaBlock->QuotaEntry[PsPagedPool].Peak = Process->QuotaPeak[PsPagedPool];
        QuotaBlock->QuotaEntry[PsPageFile].Peak = Process->QuotaPeak[PsPageFile];
        QuotaBlock->QuotaEntry[PsNonPagedPool].Limit = PspDefaultQuotaBlock.QuotaEntry[PsNonPagedPool].Limit;
        QuotaBlock->QuotaEntry[PsPagedPool].Limit = PspDefaultQuotaBlock.QuotaEntry[PsPagedPool].Limit;
        QuotaBlock->QuotaEntry[PsPageFile].Limit = PspDefaultQuotaBlock.QuotaEntry[PsPageFile].Limit;

        /* Try to exchange the quota block, if that failed, just drop it */
        OldQuotaBlock = InterlockedCompareExchangePointer((PVOID*)&Process->QuotaBlock,
                                                          QuotaBlock,
                                                          &PspDefaultQuotaBlock);
        if (OldQuotaBlock == &PspDefaultQuotaBlock)
        {
            /* Success, insert the new quota block */
            PspInsertQuotaBlock(QuotaBlock);
        }
        else
        {
            /* Failed, free the quota block and ignore it */
            ExFreePoolWithTag(QuotaBlock, TAG_QUOTA_BLOCK);
        }

        Status = STATUS_SUCCESS;
    }
    else
    {
        Status = STATUS_SUCCESS;
    }

    return Status;
}


/* EOF */
