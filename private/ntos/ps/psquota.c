/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    psquota.c

Abstract:

    This module implements the quota mechanism for NT

Author:

    Mark Lucovsky (markl) 18-Sep-1989

Revision History:

--*/

#include "psp.h"

PEPROCESS_QUOTA_BLOCK
PsChargeSharedPoolQuota(
    IN PEPROCESS Process,
    IN SIZE_T PagedAmount,
    IN SIZE_T NonPagedAmount
    )

/*++

Routine Description:

    This function charges shared pool quota of the specified pool type
    to the specified process's pooled quota block.  If the quota charge
    would exceed the limits allowed to the process, then an exception is
    raised and quota is not charged.

Arguments:

    Process - Supplies the process to charge quota to.

    PagedAmount - Supplies the amount of paged pool quota to charge.

    PagedAmount - Supplies the amount of non paged pool quota to charge.

Return Value:

    NULL - Quota was exceeded

    NON-NULL - A referenced pointer to the quota block that was charged

--*/

{

    KIRQL OldIrql;
    SIZE_T NewPoolUsage;
    PEPROCESS_QUOTA_BLOCK QuotaBlock;
    SIZE_T NewLimit;

    ASSERT((Process->Pcb.Header.Type == ProcessObject) || (Process->Pcb.Header.Type == 0));

    QuotaBlock = Process->QuotaBlock;

retry_charge:
    if ( QuotaBlock != &PspDefaultQuotaBlock ) {
        ExAcquireSpinLock(&QuotaBlock->QuotaLock,&OldIrql);
do_charge:

        if ( PagedAmount ) {

            NewPoolUsage = QuotaBlock->QuotaPoolUsage[PagedPool] + PagedAmount;

            //
            // See if enough quota exists in the block to satisfy the
            // request
            //

            if ( NewPoolUsage > QuotaBlock->QuotaPoolLimit[PagedPool] ) {
                while ( (PspDefaultPagedLimit == 0) && MmRaisePoolQuota(PagedPool,QuotaBlock->QuotaPoolLimit[PagedPool],&NewLimit) ) {
                    QuotaBlock->QuotaPoolLimit[PagedPool] = NewLimit;
                    if ( NewPoolUsage <= NewLimit ) {
                        goto LimitRaised0;
                        }
                    }
//DbgPrint("PS: ChargeShared(0) Failed P %8x QB %8x PA %8x NPA %8x\n",Process,QuotaBlock,PagedAmount,NonPagedAmount);DbgBreakPoint();
                ExReleaseSpinLock(&QuotaBlock->QuotaLock,OldIrql);
                return NULL;
                }
LimitRaised0:
            if ( NewPoolUsage < QuotaBlock->QuotaPoolUsage[PagedPool] ||
                 NewPoolUsage < PagedAmount ) {

                ExReleaseSpinLock(&QuotaBlock->QuotaLock,OldIrql);
//DbgPrint("PS: ChargeShared(1) Failed P %8x QB %8x PA %8x NPA %8x\n",Process,QuotaBlock,PagedAmount,NonPagedAmount);DbgBreakPoint();
                return NULL;
                }

            QuotaBlock->QuotaPoolUsage[PagedPool] = NewPoolUsage;
            if ( NewPoolUsage > QuotaBlock->QuotaPeakPoolUsage[PagedPool] ) {
                QuotaBlock->QuotaPeakPoolUsage[PagedPool] = NewPoolUsage;
                }
            }

        if ( NonPagedAmount ) {

            NewPoolUsage = QuotaBlock->QuotaPoolUsage[NonPagedPool] + NonPagedAmount;

            //
            // See if enough quota exists in the block to satisfy the
            // request
            //

            if ( NewPoolUsage > QuotaBlock->QuotaPoolLimit[NonPagedPool] ) {
                while ( (PspDefaultNonPagedLimit == 0) && MmRaisePoolQuota(NonPagedPool,QuotaBlock->QuotaPoolLimit[NonPagedPool],&NewLimit) ) {
                    QuotaBlock->QuotaPoolLimit[NonPagedPool] = NewLimit;
                    if ( NewPoolUsage <= NewLimit ) {
                        goto LimitRaised1;
                        }
                    }
                if ( PagedAmount ) {
                    QuotaBlock->QuotaPoolUsage[PagedPool] -= PagedAmount;
                    }
                ExReleaseSpinLock(&QuotaBlock->QuotaLock,OldIrql);
//DbgPrint("PS: ChargeShared(2) Failed P %8x QB %8x PA %8x NPA %8x\n",Process,QuotaBlock,PagedAmount,NonPagedAmount);DbgBreakPoint();
                return NULL;
                }

LimitRaised1:
            if ( NewPoolUsage < QuotaBlock->QuotaPoolUsage[NonPagedPool] ||
                 NewPoolUsage < NonPagedAmount ) {
                if ( PagedAmount ) {
                    QuotaBlock->QuotaPoolUsage[PagedPool] -= PagedAmount;
                    }

                ExReleaseSpinLock(&QuotaBlock->QuotaLock,OldIrql);
//DbgPrint("PS: ChargeShared(3) Failed P %8x QB %8x PA %8x NPA %8x\n",Process,QuotaBlock,PagedAmount,NonPagedAmount);DbgBreakPoint();
                return NULL;
                }

            QuotaBlock->QuotaPoolUsage[NonPagedPool] = NewPoolUsage;
            if ( NewPoolUsage > QuotaBlock->QuotaPeakPoolUsage[NonPagedPool] ) {
                QuotaBlock->QuotaPeakPoolUsage[NonPagedPool] = NewPoolUsage;
                }
            }

        QuotaBlock->ReferenceCount++;
        ExReleaseSpinLock(&QuotaBlock->QuotaLock,OldIrql);
        }
    else {

        //
        // Don't do ANY quota operations on the initial system process
        //

        if ( Process == PsInitialSystemProcess ) {
            return (PEPROCESS_QUOTA_BLOCK)1;
            }

        ExAcquireSpinLock(&PspDefaultQuotaBlock.QuotaLock,&OldIrql);
        if ( (QuotaBlock = Process->QuotaBlock) != &PspDefaultQuotaBlock ) {
            ExReleaseSpinLock(&PspDefaultQuotaBlock.QuotaLock,OldIrql);
            goto retry_charge;
            }
        goto do_charge;

        }
    return QuotaBlock;

}

VOID
PsReturnSharedPoolQuota(
    IN PEPROCESS_QUOTA_BLOCK QuotaBlock,
    IN SIZE_T PagedAmount,
    IN SIZE_T NonPagedAmount
    )

/*++

Routine Description:

    This function returns pool quota of the specified pool type to the
    specified process.

Arguments:

    QuotaBlock - Supplies the quota block to return quota to.

    PagedAmount - Supplies the amount of paged pool quota to return.

    PagedAmount - Supplies the amount of non paged pool quota to return.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;

    //
    // if we bypassed the quota charge, don't do anything here either
    //

    if ( QuotaBlock == (PEPROCESS_QUOTA_BLOCK)1 ) {
        return;
        }

    ExAcquireSpinLock(&QuotaBlock->QuotaLock,&OldIrql);

    if ( PagedAmount ) {

        if ( PagedAmount <= QuotaBlock->QuotaPoolUsage[PagedPool] ) {
            QuotaBlock->QuotaPoolUsage[PagedPool] -= PagedAmount;
            }
        else {
            ASSERT(FALSE);
            QuotaBlock->QuotaPoolUsage[PagedPool] = 0;
            }
        }
    if ( NonPagedAmount ) {

        if ( NonPagedAmount <= QuotaBlock->QuotaPoolUsage[NonPagedPool] ) {
            QuotaBlock->QuotaPoolUsage[NonPagedPool] -= NonPagedAmount;
            }
        else {
            ASSERT(FALSE);
            QuotaBlock->QuotaPoolUsage[NonPagedPool] = 0;
            }
        }
    QuotaBlock->ReferenceCount--;
    if ( QuotaBlock->ReferenceCount == 0 ) {
        ExReleaseSpinLock(&QuotaBlock->QuotaLock,OldIrql);
        ExFreePool(QuotaBlock);
        }
    else {
        ExReleaseSpinLock(&QuotaBlock->QuotaLock,OldIrql);
        }
}

VOID
PsChargePoolQuota(
    IN PEPROCESS Process,
    IN POOL_TYPE PoolType,
    IN SIZE_T Amount
    )

/*++

Routine Description:

    This function charges pool quota of the specified pool type to
    the specified process. If the quota charge would exceed the limits
    allowed to the process, then an exception is raised and quota is
    not charged.

Arguments:

    Process - Supplies the process to charge quota to.

    PoolType - Supplies the type of pool quota to charge.

    Amount - Supplies the amount of pool quota to charge.

Return Value:

    Raises STATUS_QUOTA_EXCEEDED if the quota charge would exceed the
        limits allowed to the process.

--*/

{

    KIRQL OldIrql;
    SIZE_T NewPoolUsage;
    PEPROCESS_QUOTA_BLOCK QuotaBlock;
    SIZE_T NewLimit;
    SIZE_T HardLimit;

    ASSERT((Process->Pcb.Header.Type == ProcessObject) || (Process->Pcb.Header.Type == 0));

    QuotaBlock = Process->QuotaBlock;

retry_charge:
    if ( QuotaBlock != &PspDefaultQuotaBlock ) {
        ExAcquireSpinLock(&QuotaBlock->QuotaLock,&OldIrql);
do_charge:
        NewPoolUsage = QuotaBlock->QuotaPoolUsage[PoolType] + Amount;

        //
        // See if enough quota exists in the block to satisfy the
        // request
        //

        if ( NewPoolUsage > QuotaBlock->QuotaPoolLimit[PoolType] ) {
            if ( PoolType == PagedPool ) {
                HardLimit = PspDefaultPagedLimit;
                }
            else {
                HardLimit = PspDefaultNonPagedLimit;
                }

            while ( (HardLimit == 0) && MmRaisePoolQuota(PoolType,QuotaBlock->QuotaPoolLimit[PoolType],&NewLimit) ) {
                QuotaBlock->QuotaPoolLimit[PoolType] = NewLimit;
                if ( NewPoolUsage <= NewLimit ) {
                    goto LimitRaised2;
                    }
                }

//DbgPrint("PS: ChargePool Failed P %8x QB %8x PT %8x Amount %8x\n",Process,QuotaBlock,PoolType,Amount);DbgBreakPoint();
            ExReleaseSpinLock(&QuotaBlock->QuotaLock,OldIrql);
            ExRaiseStatus(STATUS_QUOTA_EXCEEDED);
            }
LimitRaised2:
        if ( NewPoolUsage < QuotaBlock->QuotaPoolUsage[PoolType] ||
             NewPoolUsage < Amount ) {

            ExReleaseSpinLock(&QuotaBlock->QuotaLock,OldIrql);
            ExRaiseStatus(STATUS_QUOTA_EXCEEDED);
            }

        QuotaBlock->QuotaPoolUsage[PoolType] = NewPoolUsage;
        if ( NewPoolUsage > QuotaBlock->QuotaPeakPoolUsage[PoolType] ) {
            QuotaBlock->QuotaPeakPoolUsage[PoolType] = NewPoolUsage;
            }

        NewPoolUsage = Process->QuotaPoolUsage[PoolType] + Amount;
        Process->QuotaPoolUsage[PoolType] = NewPoolUsage;
        if ( NewPoolUsage > Process->QuotaPeakPoolUsage[PoolType] ) {
            Process->QuotaPeakPoolUsage[PoolType] = NewPoolUsage;
            }

        ExReleaseSpinLock(&QuotaBlock->QuotaLock,OldIrql);
        }
    else {
        if ( Process == PsInitialSystemProcess ) {
            return;
            }

        ExAcquireSpinLock(&PspDefaultQuotaBlock.QuotaLock,&OldIrql);
        if ( (QuotaBlock = Process->QuotaBlock) != &PspDefaultQuotaBlock ) {
            ExReleaseSpinLock(&PspDefaultQuotaBlock.QuotaLock,OldIrql);
            goto retry_charge;
            }
        goto do_charge;

        }

}

VOID
PsReturnPoolQuota(
    IN PEPROCESS Process,
    IN POOL_TYPE PoolType,
    IN SIZE_T Amount
    )

/*++

Routine Description:

    This function returns pool quota of the specified pool type to the
    specified process.

Arguments:

    Process - Supplies the process to return quota to.

    PoolType - Supplies the type of pool quota to return.

    Amount - Supplies the amount of pool quota to return

Return Value:

    Raises STATUS_QUOTA_EXCEEDED if the quota charge would exceed the
        limits allowed to the process.

--*/

{

    KIRQL OldIrql;
    PEPROCESS_QUOTA_BLOCK QuotaBlock;
    SIZE_T GiveBackLimit;
    SIZE_T RoundedUsage;

    ASSERT((Process->Pcb.Header.Type == ProcessObject) || (Process->Pcb.Header.Type == 0));

    QuotaBlock = Process->QuotaBlock;
retry_return:
    if ( QuotaBlock != &PspDefaultQuotaBlock) {
        ExAcquireSpinLock(&QuotaBlock->QuotaLock,&OldIrql);

        if ( PspDoingGiveBacks ) {
            if ( PoolType == PagedPool ) {
                GiveBackLimit = MMPAGED_QUOTA_INCREASE;
                }
            else {
                GiveBackLimit = MMNONPAGED_QUOTA_INCREASE;
                }
            }
        else {
            GiveBackLimit = 0;
            }
do_return:
        if ( Amount <= Process->QuotaPoolUsage[PoolType] ) {
            Process->QuotaPoolUsage[PoolType] -= Amount;
            }
        else {
            ASSERT(FALSE);
            GiveBackLimit = 0;
            Process->QuotaPoolUsage[PoolType] = 0;
            }

        if ( Amount <= QuotaBlock->QuotaPoolUsage[PoolType] ) {
            QuotaBlock->QuotaPoolUsage[PoolType] -= Amount;
            }
        else {
            ASSERT(FALSE);
            GiveBackLimit = 0;
            QuotaBlock->QuotaPoolUsage[PoolType] = 0;
            }

        if ( GiveBackLimit ) {
            if (QuotaBlock->QuotaPoolLimit[PoolType] - QuotaBlock->QuotaPoolUsage[PoolType] > GiveBackLimit ) {

                //
                // round up the current usage to a page multiple
                //

                RoundedUsage = ROUND_TO_PAGES(QuotaBlock->QuotaPoolUsage[PoolType]);

                //
                // Give back the limit minus the rounded usage
                //

                GiveBackLimit = QuotaBlock->QuotaPoolLimit[PoolType] - RoundedUsage;
                QuotaBlock->QuotaPoolLimit[PoolType] -= GiveBackLimit;
                MmReturnPoolQuota(PoolType,GiveBackLimit);

                }
            }
        ExReleaseSpinLock(&QuotaBlock->QuotaLock,OldIrql);
        }
    else {

        if ( Process == PsInitialSystemProcess ) {
            return;
            }

        ExAcquireSpinLock(&PspDefaultQuotaBlock.QuotaLock,&OldIrql);
        if ( (QuotaBlock = Process->QuotaBlock) != &PspDefaultQuotaBlock) {
            ExReleaseSpinLock(&PspDefaultQuotaBlock.QuotaLock,OldIrql);
            goto retry_return;
            }
        GiveBackLimit = 0;
        goto do_return;
        }
}

VOID
PspInheritQuota(
    IN PEPROCESS NewProcess,
    IN PEPROCESS ParentProcess
    )
{
    PEPROCESS_QUOTA_BLOCK QuotaBlock;
    KIRQL OldIrql;

    if ( ParentProcess ) {
        QuotaBlock = ParentProcess->QuotaBlock;
        }
    else {
        QuotaBlock = &PspDefaultQuotaBlock;
        }

    ExAcquireSpinLock(&QuotaBlock->QuotaLock,&OldIrql);
    QuotaBlock->ReferenceCount++;
    NewProcess->QuotaBlock = QuotaBlock;
    ExReleaseSpinLock(&QuotaBlock->QuotaLock,OldIrql);
}

VOID
MiReturnPageFileQuota (
    IN SIZE_T QuotaCharge,
    IN PEPROCESS CurrentProcess
    );

VOID
PspDereferenceQuota(
    IN PEPROCESS Process
    )
{
    PEPROCESS_QUOTA_BLOCK QuotaBlock;
    KIRQL OldIrql;

    QuotaBlock = Process->QuotaBlock;

    PsReturnPoolQuota(Process,NonPagedPool,Process->QuotaPoolUsage[NonPagedPool]);
    PsReturnPoolQuota(Process,PagedPool,Process->QuotaPoolUsage[PagedPool]);
    MiReturnPageFileQuota(Process->PagefileUsage,Process);

    ExAcquireSpinLock(&QuotaBlock->QuotaLock,&OldIrql);

    QuotaBlock->ReferenceCount--;
    if ( QuotaBlock->ReferenceCount == 0 ) {
        ExReleaseSpinLock(&QuotaBlock->QuotaLock,OldIrql);
        ExFreePool(QuotaBlock);
        }
    else {
        ExReleaseSpinLock(&QuotaBlock->QuotaLock,OldIrql);
        }
}
