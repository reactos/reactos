/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Process Pool Quotas Support
 * COPYRIGHT:       Copyright 2005 Alex Ionescu <alex@relsoft.net>
 *                  Copyright 2007 Mike Nordell
 *                  Copyright 2021 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES **************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

EPROCESS_QUOTA_BLOCK PspDefaultQuotaBlock;
static LIST_ENTRY PspQuotaBlockList = {&PspQuotaBlockList, &PspQuotaBlockList};
static KSPIN_LOCK PspQuotaLock;

#define VALID_QUOTA_FLAGS (QUOTA_LIMITS_HARDWS_MIN_ENABLE | \
                           QUOTA_LIMITS_HARDWS_MIN_DISABLE | \
                           QUOTA_LIMITS_HARDWS_MAX_ENABLE | \
                           QUOTA_LIMITS_HARDWS_MAX_DISABLE)

/* PRIVATE FUNCTIONS *******************************************************/

/**
 * @brief
 * Returns pool quotas back to the Memory Manager
 * when the pool quota block is no longer being
 * used by anybody.
 *
 * @param[in] QuotaBlock
 * The pool quota block of which quota resources
 * are to be sent back.
 *
 * @return
 * Nothing.
 *
 * @remarks
 * The function only returns quotas back to Memory
 * Manager that is paged or non paged. It does not
 * return page file quotas as page file quota
 * management is done in a different way. Furthermore,
 * quota spin lock has to be held when returning quotas.
 */
_Requires_lock_held_(PspQuotaLock)
VOID
NTAPI
PspReturnQuotasOnDestroy(
    _In_ PEPROCESS_QUOTA_BLOCK QuotaBlock)
{
    ULONG PsQuotaTypeIndex;
    SIZE_T QuotaToReturn;

    /*
     * We must be in a dispatch level interrupt here
     * as we should be under a spin lock.
     */
    ASSERT_IRQL_EQUAL(DISPATCH_LEVEL);

    /* Make sure that the quota block is not plain garbage */
    ASSERT(QuotaBlock);

    /* Loop over the Process quota types */
    for (PsQuotaTypeIndex = PsNonPagedPool; PsQuotaTypeIndex < PsPageFile; PsQuotaTypeIndex++)
    {
        /* The amount needed to return to Mm is the limit and return fields */
        QuotaToReturn = QuotaBlock->QuotaEntry[PsQuotaTypeIndex].Limit + QuotaBlock->QuotaEntry[PsQuotaTypeIndex].Return;
        MmReturnPoolQuota(PsQuotaTypeIndex, QuotaToReturn);
    }
}

/**
 * @brief
 * Releases some of excess quotas in order to attempt
 * free up some resources. This is done primarily in
 * in case the Memory Manager fails to raise the quota
 * limit.
 *
 * @param[in] QuotaType
 * Process pool quota type.
 *
 * @param[out] ReturnedQuotas
 * A pointer to the returned amount of quotas
 * back to Memory Manager.
 *
 * @return
 * Nothing.
 *
 * @remarks
 * The function releases excess paged or non
 * paged pool quotas. Page file quota type is
 * not permitted. Furthermore, quota spin lock
 * has to be held when returning quotas.
 */
_Requires_lock_held_(PspQuotaLock)
VOID
NTAPI
PspReturnExcessQuotas(
    _In_ PS_QUOTA_TYPE QuotaType,
    _Outptr_ PSIZE_T ReturnedQuotas)
{
    PLIST_ENTRY PspQuotaList;
    PEPROCESS_QUOTA_BLOCK QuotaBlockFromList;
    SIZE_T AmountToReturn = 0;

    /*
     * We must be in a dispatch level interrupt here
     * as we should be under a spin lock.
     */
    ASSERT_IRQL_EQUAL(DISPATCH_LEVEL);

    /*
     * Loop over the quota block lists and reap
     * whatever quotas we haven't returned which
     * is needed to free up resources.
     */
    for (PspQuotaList = PspQuotaBlockList.Flink;
         PspQuotaList != &PspQuotaBlockList;
         PspQuotaList = PspQuotaList->Flink)
    {
        /* Gather the quota block from the list */
        QuotaBlockFromList = CONTAINING_RECORD(PspQuotaList, EPROCESS_QUOTA_BLOCK, QuotaList);

        /*
         * Gather any unreturned quotas and cache
         * them to a variable.
         */
        AmountToReturn += InterlockedExchangeSizeT(&QuotaBlockFromList->QuotaEntry[QuotaType].Return, 0);

        /*
         * If no other process is taking use of this
         * block, then it means that this block has
         * only shared pool quota and the last process
         * no longer uses this block. If the limit is
         * grater than the usage then trim the limit
         * and use that as additional amount of quota
         * to return.
         */
        if (QuotaBlockFromList->ProcessCount == 0)
        {
            if (QuotaBlockFromList->QuotaEntry[QuotaType].Usage <
                QuotaBlockFromList->QuotaEntry[QuotaType].Limit)
            {
                InterlockedExchangeSizeT(&QuotaBlockFromList->QuotaEntry[QuotaType].Limit,
                                         QuotaBlockFromList->QuotaEntry[QuotaType].Usage);
                AmountToReturn += QuotaBlockFromList->QuotaEntry[QuotaType].Limit;
            }
        }
    }

    /* Invoke Mm to return quotas */
    DPRINT("PspReturnExcessQuotas(): Amount of quota released -- %lu\n", AmountToReturn);
    MmReturnPoolQuota(QuotaType, AmountToReturn);
    *ReturnedQuotas = AmountToReturn;
}

/**
 * @brief
 * Internal kernel function that provides the
 * bulk logic of process quota charging,
 * necessary for exported kernel routines
 * needed for quota management.
 *
 * @param[in] Process
 * A process, represented as a EPROCESS object.
 * This parameter is used to charge the own
 * process' quota usage.
 *
 * @param[in] QuotaBlock
 * The quota block which quotas are to be charged.
 * This block can either come from the process itself
 * or from an object with specified quota charges.
 *
 * @param[in] QuotaType
 * The quota type which quota in question is to
 * be charged. The permitted types are PsPagedPool,
 * PsNonPagedPool and PsPageFile.
 *
 * @param[in] Amount
 * The amount of quota to be charged.
 *
 * @return
 * Returns STATUS_SUCCESS if quota charging has
 * been done successfully without problemns.
 * STATUS_QUOTA_EXCEEDED is returned if the caller
 * wants to charge quotas with amount way over
 * the limits. STATUS_PAGEFILE_QUOTA_EXCEEDED
 * is returned for the same situation but
 * specific to page files instead.
 */
NTSTATUS
NTAPI
PspChargeProcessQuotaSpecifiedPool(
    _In_opt_ PEPROCESS Process,
    _In_ PEPROCESS_QUOTA_BLOCK QuotaBlock,
    _In_ PS_QUOTA_TYPE QuotaType,
    _In_ SIZE_T Amount)
{
    KIRQL OldIrql;
    SIZE_T ReturnedQuotas;
    SIZE_T UpdatedLimit;

    /* Sanity checks */
    ASSERT(QuotaType < PsQuotaTypes);
    ASSERT((SSIZE_T)Amount >= 0);

    /* Guard ourselves in a spin lock */
    KeAcquireSpinLock(&PspQuotaLock, &OldIrql);

    /* Are we within the bounds of quota limit? */
    if (QuotaBlock->QuotaEntry[QuotaType].Usage + Amount >
        QuotaBlock->QuotaEntry[QuotaType].Limit &&
        QuotaBlock != &PspDefaultQuotaBlock)
    {
        /* We aren't... Is this a page file quota charging? */
        if (QuotaType == PsPageFile)
        {
            /* It is, return the appropriate status code */
            DPRINT1("PspChargeProcessQuotaSpecifiedPool(): Quota amount exceeds the limit on page file quota (limit -- %lu || amount -- %lu)\n",
                    QuotaBlock->QuotaEntry[QuotaType].Limit, Amount);
            return STATUS_PAGEFILE_QUOTA_EXCEEDED;
        }

        /*
         * This is not a page file charge. What we can do at best
         * in this scenario is to attempt to raise (expand) the
         * quota limit charges of the block.
         */
        if (!MmRaisePoolQuota(QuotaType,
                              QuotaBlock->QuotaEntry[QuotaType].Limit,
                              &UpdatedLimit))
        {
            /*
             * We can't? It could be that we must free
             * up some resources in order to raise the
             * limit, which in that case we must return
             * the excess of quota that hasn't been
             * returned. If we haven't returned anything
             * then what we're doing here is futile.
             * Bail out...
             */
            PspReturnExcessQuotas(QuotaType, &ReturnedQuotas);
            if (ReturnedQuotas == 0)
            {
                DPRINT1("PspChargeProcessQuotaSpecifiedPool(): Failed to free some resources in order to raise quota limits...\n");
                KeReleaseSpinLock(&PspQuotaLock, OldIrql);
                return STATUS_QUOTA_EXCEEDED;
            }

            /* Try to raise the quota limits again */
            MmRaisePoolQuota(QuotaType,
                             QuotaBlock->QuotaEntry[QuotaType].Limit,
                             &UpdatedLimit);
        }

        /* Enforce a new raised limit */
        InterlockedExchangeSizeT(&QuotaBlock->QuotaEntry[QuotaType].Limit, UpdatedLimit);

        /*
         * Now determine if the current usage and the
         * amounting by the caller still exceeds the
         * quota limit of the process. If it's still
         * over the limit then there's nothing we can
         * do, so fail.
         */
        if (QuotaBlock->QuotaEntry[QuotaType].Usage + Amount >
            QuotaBlock->QuotaEntry[QuotaType].Limit)
        {
            DPRINT1("PspChargeProcessQuotaSpecifiedPool(): Quota amount exceeds the limit (limit -- %lu || amount -- %lu)\n",
                    QuotaBlock->QuotaEntry[QuotaType].Limit, Amount);
            return STATUS_QUOTA_EXCEEDED;
        }
    }

    /* Update the quota usage */
    InterlockedExchangeAddSizeT(&QuotaBlock->QuotaEntry[QuotaType].Usage, Amount);

    /* Update the entry peak if it's less than the usage */
    if (QuotaBlock->QuotaEntry[QuotaType].Peak <
        QuotaBlock->QuotaEntry[QuotaType].Usage)
    {
        InterlockedExchangeSizeT(&QuotaBlock->QuotaEntry[QuotaType].Peak,
                                 QuotaBlock->QuotaEntry[QuotaType].Usage);
    }

    /* Are we being given a process as well? */
    if (Process)
    {
        /* We're being given, check that's not a system one */
        ASSERT(Process != PsInitialSystemProcess);

        InterlockedExchangeAddSizeT(&Process->QuotaUsage[QuotaType], Amount);

        /*
         * OK, we've now updated the quota usage of the process
         * based upon the amount that the caller wanted to charge.
         * Although the peak of process quota can be less than it was
         * before so update the peaks as well accordingly.
         */
        if (Process->QuotaPeak[QuotaType] < Process->QuotaUsage[QuotaType])
        {
            InterlockedExchangeSizeT(&Process->QuotaPeak[QuotaType],
                                     Process->QuotaUsage[QuotaType]);
        }
    }

    /* Release the lock */
    KeReleaseSpinLock(&PspQuotaLock, OldIrql);
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Internal kernel function that provides the
 * bulk logic of process quota returning. It
 * returns (takes away) quotas back from a
 * process and/or quota block, which is
 * the opposite of charging quotas.
 *
 * @param[in] Process
 * A process, represented as a EPROCESS object.
 * This parameter is used to return the own
 * process' quota usage.
 *
 * @param[in] QuotaBlock
 * The quota block which quotas are to be returned.
 * This block can either come from the process itself
 * or from an object with specified quota charges.
 *
 * @param[in] QuotaType
 * The quota type which quota in question is to
 * be returned. The permitted types are PsPagedPool,
 * PsNonPagedPool and PsPageFile.
 *
 * @param[in] Amount
 * The amount of quota to be returned.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
PspReturnProcessQuotaSpecifiedPool(
    _In_opt_ PEPROCESS Process,
    _In_ PEPROCESS_QUOTA_BLOCK QuotaBlock,
    _In_ PS_QUOTA_TYPE QuotaType,
    _In_ SIZE_T Amount)
{
    KIRQL OldIrql;
    SIZE_T ReturnThreshold;
    SIZE_T AmountToReturn = 0;

    /* Sanity checks */
    ASSERT(QuotaType < PsQuotaTypes);
    ASSERT((SSIZE_T)Amount >= 0);

    /* Guard ourselves in a spin lock */
    KeAcquireSpinLock(&PspQuotaLock, &OldIrql);

    /* Does the caller return more quota than it was previously charged? */
    if ((Process && Process->QuotaUsage[QuotaType] < Amount) ||
        QuotaBlock->QuotaEntry[QuotaType].Usage < Amount)
    {
        /* It does, crash the system! */
        KeBugCheckEx(QUOTA_UNDERFLOW,
                     (ULONG_PTR)Process,
                     (ULONG_PTR)QuotaType,
                     Process ? (ULONG_PTR)Process->QuotaUsage[QuotaType] :
                               QuotaBlock->QuotaEntry[QuotaType].Usage,
                     (ULONG_PTR)Amount);
    }

    /* The return threshold can be non paged or paged */
    ReturnThreshold = QuotaType ? PSP_NON_PAGED_POOL_QUOTA_THRESHOLD : PSP_PAGED_POOL_QUOTA_THRESHOLD;

    /*
     * We need to trim the quota limits based on the
     * amount we're going to return quotas back.
     */
    if ((QuotaType != PsPageFile && QuotaBlock != &PspDefaultQuotaBlock) &&
        (QuotaBlock->QuotaEntry[QuotaType].Limit > QuotaBlock->QuotaEntry[QuotaType].Usage + ReturnThreshold))
    {
        /*
         * If the amount to return exceeds the threshold,
         * the new amount becomes the default, otherwise
         * the amount is just the one given by the caller.
         */
        AmountToReturn = min(Amount, ReturnThreshold);

        /* Add up the lots to the Return field */
        InterlockedExchangeAddSizeT(&QuotaBlock->QuotaEntry[QuotaType].Return, AmountToReturn);

        /*
         * If the amount to return exceeds the threshold then
         * we have lots of quota to return to Mm. So do it so
         * and zerou out the Return field.
         */
        if (QuotaBlock->QuotaEntry[QuotaType].Return > ReturnThreshold)
        {
            MmReturnPoolQuota(QuotaType, QuotaBlock->QuotaEntry[QuotaType].Return);
            InterlockedExchangeSizeT(QuotaBlock->QuotaEntry[QuotaType].Return, 0);
        }

        /* And try to trim the limit */
        InterlockedExchangeSizeT(&QuotaBlock->QuotaEntry[QuotaType].Limit,
                                 QuotaBlock->QuotaEntry[QuotaType].Limit - AmountToReturn);
    }

    /* Update the usage member of the block */
    InterlockedExchangeAddSizeT(&QuotaBlock->QuotaEntry[QuotaType].Usage, -(LONG_PTR)Amount);

    /* Are we being given a process? */
    if (Process)
    {
        /* We're being given, check that's not a system one  */
        ASSERT(Process != PsInitialSystemProcess);

        /* Decrease the process' quota usage */
        InterlockedExchangeAddSizeT(&Process->QuotaUsage[QuotaType], -(LONG_PTR)Amount);
    }

    /* We're done, release the lock */
    KeReleaseSpinLock(&PspQuotaLock, OldIrql);
}

/* FUNCTIONS ***************************************************************/

/**
 * @brief
 * Initializes the quota system during boot
 * phase of the system, which sets up the
 * default quota block that is used across
 * several processes.
 *
 * @return
 * Nothing.
 */
CODE_SEG("INIT")
VOID
NTAPI
PsInitializeQuotaSystem(VOID)
{
    /* Initialize the default block */
    RtlZeroMemory(&PspDefaultQuotaBlock, sizeof(PspDefaultQuotaBlock));

    /* Assign the default quota limits */
    PspDefaultQuotaBlock.QuotaEntry[PsNonPagedPool].Limit = (SIZE_T)-1;
    PspDefaultQuotaBlock.QuotaEntry[PsPagedPool].Limit = (SIZE_T)-1;
    PspDefaultQuotaBlock.QuotaEntry[PsPageFile].Limit = (SIZE_T)-1;

    /*
     * Set up the count references as the
     * default block will going to be used.
     */
    PspDefaultQuotaBlock.ReferenceCount = 1;
    PspDefaultQuotaBlock.ProcessCount = 1;

    /* Assign that block to initial process */
    PsGetCurrentProcess()->QuotaBlock = &PspDefaultQuotaBlock;
}

/**
 * @brief
 * Inherits the quota block to another newborn
 * (child) process. If there's no parent
 * process, the default quota block is
 * assigned.
 *
 * @param[in] Process
 * The child process which quota block
 * is to be given.
 *
 * @param[in] ParentProcess
 * The parent process.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
PspInheritQuota(
    _In_ PEPROCESS Process,
    _In_opt_ PEPROCESS ParentProcess)
{
    PEPROCESS_QUOTA_BLOCK QuotaBlock;

    if (ParentProcess != NULL)
    {
        ASSERT(ParentProcess->QuotaBlock != NULL);
        QuotaBlock = ParentProcess->QuotaBlock;
    }
    else
    {
        QuotaBlock = &PspDefaultQuotaBlock;
    }

    InterlockedIncrementSizeT(&QuotaBlock->ProcessCount);
    InterlockedIncrementSizeT(&QuotaBlock->ReferenceCount);

    Process->QuotaBlock = QuotaBlock;
}

/**
 * @brief
 * Inserts the new quota block into
 * the quota list.
 *
 * @param[in] QuotaBlock
 * The new quota block.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
PspInsertQuotaBlock(
    _In_ PEPROCESS_QUOTA_BLOCK QuotaBlock)
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&PspQuotaLock, &OldIrql);
    InsertTailList(&PspQuotaBlockList, &QuotaBlock->QuotaList);
    KeReleaseSpinLock(&PspQuotaLock, OldIrql);
}

/**
 * @brief
 * De-references a quota block when quotas
 * have been returned back because of an
 * object de-allocation or when a process
 * gets destroyed. If the last instance
 * that held up the block gets de-referenced
 * the function will perform a cleanup against
 * that block and it'll free the quota block
 * from memory.
 *
 * @param[in] Process
 * A pointer to a process that de-references the
 * quota block.
 *
 * @param[in] QuotaBlock
 * A pointer to a quota block that is to be
 * de-referenced. This block can come from a
 * process that references it or an object.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
PspDereferenceQuotaBlock(
    _In_opt_ PEPROCESS Process,
    _In_ PEPROCESS_QUOTA_BLOCK QuotaBlock)
{
    ULONG PsQuotaTypeIndex;
    KIRQL OldIrql;

    /* Make sure the quota block is not trash */
    ASSERT(QuotaBlock);

    /* Iterate over the process quota types if we have a process */
    if (Process)
    {
        for (PsQuotaTypeIndex = PsNonPagedPool; PsQuotaTypeIndex < PsQuotaTypes; PsQuotaTypeIndex++)
        {
            /*
             * We need to make sure that the quota usage
             * uniquely associated with the process is 0
             * on that moment the process gets destroyed.
             */
            ASSERT(Process->QuotaUsage[PsQuotaTypeIndex] == 0);
        }

        /* As the process is now gone, decrement the process count */
        InterlockedDecrementUL(&QuotaBlock->ProcessCount);
    }

    /* If no one is using this block, begin to destroy it */
    if (QuotaBlock != &PspDefaultQuotaBlock &&
        InterlockedDecrementUL(&QuotaBlock->ReferenceCount) == 0)
    {
        /* Acquire the quota lock */
        KeAcquireSpinLock(&PspQuotaLock, &OldIrql);

        /* Return all the quotas back to Mm and remove the quota from list */
        PspReturnQuotasOnDestroy(QuotaBlock);
        RemoveEntryList(&QuotaBlock->QuotaList);

        /* Release the lock and free the block */
        KeReleaseSpinLock(&PspQuotaLock, OldIrql);
        ExFreePoolWithTag(QuotaBlock, TAG_QUOTA_BLOCK);
    }
}

/**
 * @brief
 * Returns the shared (paged and non paged)
 * pool quotas. The function is used exclusively
 * by the Object Manager to manage quota returns
 * handling of objects.
 *
 * @param[in] QuotaBlock
 * The quota block which quotas are to
 * be returned.
 *
 * @param[in] AmountToReturnPaged
 * The amount of paged quotas quotas to
 * be returned.
 *
 * @param[in] AmountToReturnNonPaged
 * The amount of non paged quotas to
 * be returned.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
PsReturnSharedPoolQuota(
    _In_ PEPROCESS_QUOTA_BLOCK QuotaBlock,
    _In_ SIZE_T AmountToReturnPaged,
    _In_ SIZE_T AmountToReturnNonPaged)
{
    /* Sanity check */
    ASSERT(QuotaBlock);

    /* Return the pool quotas if there're any */
    if (AmountToReturnPaged != 0)
    {
        PspReturnProcessQuotaSpecifiedPool(NULL, QuotaBlock, PsPagedPool, AmountToReturnPaged);
    }

    if (AmountToReturnNonPaged != 0)
    {
        PspReturnProcessQuotaSpecifiedPool(NULL, QuotaBlock, PsNonPagedPool, AmountToReturnNonPaged);
    }

    DPRINT("PsReturnSharedPoolQuota(): Amount returned back (paged %lu -- non paged %lu)\n", AmountToReturnPaged, AmountToReturnNonPaged);

    /* Dereference the quota block */
    PspDereferenceQuotaBlock(NULL, QuotaBlock);
}

/**
 * @brief
 * Charges the shared (paged and non paged)
 * pool quotas. The function is used exclusively
 * by the Object Manager to manage quota charges
 * handling of objects.
 *
 * @param[in] Process
 * The process which quotas are to
 * be charged within its quota block.
 *
 * @param[in] AmountToChargePaged
 * The amount of paged quotas quotas to
 * be charged.
 *
 * @param[in] AmountToChargeNonPaged
 * The amount of non paged quotas to
 * be charged.
 *
 * @return
 * Returns the charged quota block, which it'll
 * be used by the Object Manager to attach
 * the charged quotas information to the object.
 * If the function fails to charge quotas, NULL
 * is returned to the caller.
 */
PEPROCESS_QUOTA_BLOCK
NTAPI
PsChargeSharedPoolQuota(
    _In_ PEPROCESS Process,
    _In_ SIZE_T AmountToChargePaged,
    _In_ SIZE_T AmountToChargeNonPaged)
{
    NTSTATUS Status;

    /* Sanity checks */
    ASSERT(Process);
    ASSERT(Process->QuotaBlock);

    /* Do we have some paged pool quota to charge? */
    if (AmountToChargePaged != 0)
    {
        /* We do, charge! */
        Status = PspChargeProcessQuotaSpecifiedPool(NULL, Process->QuotaBlock, PsPagedPool, AmountToChargePaged);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("PsChargeSharedPoolQuota(): Failed to charge the shared pool quota (Status 0x%lx)\n", Status);
            return NULL;
        }
    }

    /* Do we have some non paged pool quota to charge? */
    if (AmountToChargeNonPaged != 0)
    {
        /* We do, charge! */
        Status = PspChargeProcessQuotaSpecifiedPool(NULL, Process->QuotaBlock, PsNonPagedPool, AmountToChargeNonPaged);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("PsChargeSharedPoolQuota(): Failed to charge the shared pool quota (Status 0x%lx). Attempting to return some paged pool back...\n", Status);
            PspReturnProcessQuotaSpecifiedPool(NULL, Process->QuotaBlock, PsPagedPool, AmountToChargePaged);
            return NULL;
        }
    }

    /* We have charged the quotas of an object, increment the reference */
    InterlockedIncrementSizeT(&Process->QuotaBlock->ReferenceCount);

    DPRINT("PsChargeSharedPoolQuota(): Amount charged (paged %lu --  non paged %lu)\n", AmountToChargePaged, AmountToChargeNonPaged);
    return Process->QuotaBlock;
}

/**
 * @brief
 * Charges the process page file quota.
 * The function is used internally by
 * the kernel.
 *
 * @param[in] Process
 * The process which page file quota is
 * to be charged.
 *
 * @param[in] Amount
 * The amount of page file quota to charge.
 *
 * @return
 * Returns STATUS_SUCCESS if quota charging has
 * been done with success, otherwise a NTSTATUS
 * code of STATUS_PAGEFILE_QUOTA_EXCEEDED is
 * returned.
 */
NTSTATUS
NTAPI
PsChargeProcessPageFileQuota(
    _In_ PEPROCESS Process,
    _In_ SIZE_T Amount)
{
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return STATUS_SUCCESS;

    return PspChargeProcessQuotaSpecifiedPool(Process, Process->QuotaBlock, PsPageFile, Amount);
}

/**
 * @brief
 * Charges the pool quota of a given process.
 * The kind of pool quota to charge is determined
 * by the PoolType parameter.
 *
 * @param[in] Process
 * The process which quota is to be
 * charged.
 *
 * @param[in] PoolType
 * The pool type to choose to charge quotas
 * (e.g. PagedPool or NonPagedPool).
 *
 * @param[in] Amount
 * The amount of quotas to charge into a process.
 *
 * @return
 * Nothing.
 *
 * @remarks
 * The function raises an exception if STATUS_QUOTA_EXCEEDED
 * status code is returned. Callers are responsible on their
 * own to handle the raised exception.
 */
VOID
NTAPI
PsChargePoolQuota(
    _In_ PEPROCESS Process,
    _In_ POOL_TYPE PoolType,
    _In_ SIZE_T Amount)
{
    NTSTATUS Status;
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return;

    /* Charge the usage */
    Status = PsChargeProcessPoolQuota(Process, PoolType, Amount);
    if (!NT_SUCCESS(Status)) ExRaiseStatus(Status);
}

/**
 * @brief
 * Charges the non paged pool quota
 * of a given process.
 *
 * @param[in] Process
 * The process which non paged quota
 * is to be charged.
 *
 * @param[in] Amount
 * The amount of quotas to charge into a process.
 *
 * @return
 * Returns STATUS_SUCCESS if quota charing has
 * suceeded, STATUS_QUOTA_EXCEEDED is returned
 * otherwise to indicate the caller attempted
 * to charge quotas over the limits.
 */
NTSTATUS
NTAPI
PsChargeProcessNonPagedPoolQuota(
    _In_ PEPROCESS Process,
    _In_ SIZE_T Amount)
{
    /* Call the general function */
    return PsChargeProcessPoolQuota(Process, NonPagedPool, Amount);
}

/**
 * @brief
 * Charges the paged pool quota of a
 * given process.
 *
 * @param[in] Process
 * The process which paged quota
 * is to be charged.
 *
 * @param[in] Amount
 * The amount of quotas to charge into a process.
 *
 * @return
 * Returns STATUS_SUCCESS if quota charing has
 * suceeded, STATUS_QUOTA_EXCEEDED is returned
 * otherwise to indicate the caller attempted
 * to charge quotas over the limits.
 */
NTSTATUS
NTAPI
PsChargeProcessPagedPoolQuota(
    _In_ PEPROCESS Process,
    _In_ SIZE_T Amount)
{
    /* Call the general function */
    return PsChargeProcessPoolQuota(Process, PagedPool, Amount);
}

/**
 * @brief
 * Charges the process' quota pool.
 * The type of quota to be charged
 * depends upon the PoolType parameter.
 *
 * @param[in] Process
 * The process which quota is to
 * be charged.
 *
 * @param[in] PoolType
 * The type of quota pool to charge (e.g.
 * PagedPool or NonPagedPool).
 *
 * @param[in] Amount
 * The amount of quotas to charge into a process.
 *
 * @return
 * Returns STATUS_SUCCESS if quota charing has
 * suceeded, STATUS_QUOTA_EXCEEDED is returned
 * otherwise to indicate the caller attempted
 * to charge quotas over the limits.
 */
NTSTATUS
NTAPI
PsChargeProcessPoolQuota(
    _In_ PEPROCESS Process,
    _In_ POOL_TYPE PoolType,
    _In_ SIZE_T Amount)
{
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return STATUS_SUCCESS;

    return PspChargeProcessQuotaSpecifiedPool(Process,
                                              Process->QuotaBlock,
                                              (PoolType & PAGED_POOL_MASK),
                                              Amount);
}

/**
 * @brief
 * Returns the pool quota that the
 * process was taking up.
 *
 * @param[in] Process
 * The process which quota is to
 * be returned.
 *
 * @param[in] PoolType
 * The type of quota pool to return (e.g.
 * PagedPool or NonPagedPool).
 *
 * @param[in] Amount
 * The amount of quotas to return from a process.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
PsReturnPoolQuota(
    _In_ PEPROCESS Process,
    _In_ POOL_TYPE PoolType,
    _In_ SIZE_T Amount)
{
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return;

    PspReturnProcessQuotaSpecifiedPool(Process,
                                       Process->QuotaBlock,
                                       (PoolType & PAGED_POOL_MASK),
                                       Amount);
}

/**
 * @brief
 * Returns the non paged quota pool
 * that the process was taking up.
 *
 * @param[in] Process
 * The process which non paged quota
 * is to be returned.
 *
 * @param[in] Amount
 * The amount of quotas to return from a process.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
PsReturnProcessNonPagedPoolQuota(
    _In_ PEPROCESS Process,
    _In_ SIZE_T Amount)
{
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return;

    PsReturnPoolQuota(Process, NonPagedPool, Amount);
}

/**
 * @brief
 * Returns the paged pool quota that
 * the process was taking up.
 *
 * @param[in] Process
 * The process which paged pool
 * quota is to be returned.
 *
 * @param[in] Amount
 * The amount of quotas to return from a process.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
PsReturnProcessPagedPoolQuota(
    _In_ PEPROCESS Process,
    _In_ SIZE_T Amount)
{
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return;

    PsReturnPoolQuota(Process, PagedPool, Amount);
}

/**
 * @brief
 * Returns the page file quota that the
 * process was taking up. The function
 * is used exclusively by the kernel.
 *
 * @param[in] Process
 * The process which pagefile quota is
 * to be returned.
 *
 * @param[in] Amount
 * The amount of quotas to return from a process.
 *
 * @return
 * Returns STATUS_SUCCESS.
 */
NTSTATUS
NTAPI
PsReturnProcessPageFileQuota(
    _In_ PEPROCESS Process,
    _In_ SIZE_T Amount)
{
    /* Don't do anything for the system process */
    if (Process == PsInitialSystemProcess) return STATUS_SUCCESS;

    PspReturnProcessQuotaSpecifiedPool(Process, Process->QuotaBlock, PsPageFile, Amount);
    return STATUS_SUCCESS;
}

/**
 * @brief
 * This function adjusts the working set limits
 * of a process and sets up new quota limits
 * when necessary. The function is used
 * when the caller requests to set up
 * new working set sizes.
 *
 * @param[in] Process
 * The process which quota limits or working
 * set sizes are to be changed.
 *
 * @param[in] Unused
 * This parameter is unused.
 *
 * @param[in] QuotaLimits
 * An arbitrary pointer that points to a quota
 * limits structure, needed to determine on
 * setting up new working set sizes.
 *
 * @param[in] QuotaLimitsLength
 * The length of QuotaLimits buffer, which size
 * is expressed in bytes.
 *
 * @param[in] PreviousMode
 * The processor level access mode.
 *
 * @return
 * Returns STATUS_SUCCESS if the function has completed
 * successfully. STATUS_INVALID_PARAMETER is returned if
 * the caller has given a quota limits structure with
 * invalid data. STATUS_INFO_LENGTH_MISMATCH is returned
 * if the length of QuotaLimits pointed by QuotaLimitsLength
 * is not right. STATUS_PRIVILEGE_NOT_HELD is returned if
 * the calling thread of the process doesn't hold the necessary
 * right privilege to increase quotas. STATUS_NO_MEMORY is
 * returned if a memory pool allocation has failed. A failure
 * NTSTATUS code is returned otherwise.
 */
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
