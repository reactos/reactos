/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Inlined Functions for the Power Manager
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

//
// Ensures the passed IRP is a Power IRP (PoCallDriver & PoHandlePowerIrp)
//
#define POP_ASSERT_IRP_IS_POWER(IrpStack)                           \
    ASSERT(IrpStack->MajorFunction == IRP_MJ_POWER)

//
// Ensures the current thread actually owns the IRP lock
//
#define POP_ASSERT_IRP_LOCK_THREAD_OWNERSHIP()                      \
    ASSERT(PopIrpOwnerLockThread == KeGetCurrentThread())

//
// Ensures the IRP is a wake Power Wake IRP
//
#define POP_ASSERT_IRP_IS_WAKE(IrpStack)                             \
    ASSERT(IrpStack->MinorFunction == IRP_MN_WAIT_WAKE)

//
// Ensures the current thread owns the policy lock
//
#define POP_ASSERT_POWER_POLICY_LOCK_OWNERSHIP()                      \
    ASSERT(PopPowerPolicyOwnerLockThread == KeGetCurrentThread())

//
// Ensures the composite battery is not processing any mode requests
//
#define POP_ASSERT_NO_BATTERY_REQUEST_MODE()                           \
    ASSERT((PopBattery->Flags & POP_CB_PROCESSING_MODE_REQUEST) == 0)

//
// Ensures the current thread owns the power request lock
//
#define POP_ASSERT_POWER_REQUEST_LOCK_THREAD_OWNERSHIP()                \
    ASSERT(PopPowerRequestOwnerLockThread == KeGetCurrentThread())


//
// Ensures the current thread owns the power setting lock
//
#define POP_ASSERT_POWER_SETTING_LOCK_THREAD_OWNERSHIP()                \
    ASSERT(PopPowerSettingOwnerLockThread == KeGetCurrentThread())

//
// DOPE locking mechanism
//
FORCEINLINE
VOID
PopAcquireDopeLock(
    _Out_ PKIRQL Irql)
{
    KeAcquireSpinLock(&PopDopeGlobalLock, Irql);
}

FORCEINLINE
VOID
PopReleaseDopeLock(
    _In_ KIRQL OldIrql)
{
    KeReleaseSpinLock(&PopDopeGlobalLock, OldIrql);
}

//
// Volume locking mechanism
//
FORCEINLINE
VOID
PopAcquireVolumeLock(VOID)
{
    KeAcquireGuardedMutex(&PopVolumeLock);
}

FORCEINLINE
VOID
PopReleaseVolumeLock(VOID)
{
    KeReleaseGuardedMutex(&PopVolumeLock);
}

//
// Power IRP dispatcher inliner
//
FORCEINLINE
NTSTATUS
PopDispatchPowerIrp(
    _In_ PIRP Irp,
    _In_ PDEVICE_OBJECT Device,
    _In_ PIO_STACK_LOCATION IoStack)
{
    PDRIVER_OBJECT Driver = Device->DriverObject;

    return Driver->MajorFunction[IoStack->MajorFunction](Device,
                                                         Irp);
}

//
// IRP locking mechanism
//
FORCEINLINE
VOID
PopAcquireIrpLock(
    _Out_ PKLOCK_QUEUE_HANDLE LockHandle)
{
    KeAcquireInStackQueuedSpinLock(&PopIrpLock, LockHandle);
    PopIrpOwnerLockThread = KeGetCurrentThread();
}

FORCEINLINE
VOID
PopReleaseIrpLock(
    _In_ PKLOCK_QUEUE_HANDLE LockHandle)
{
    POP_ASSERT_IRP_LOCK_THREAD_OWNERSHIP();
    PopIrpOwnerLockThread = NULL;
    KeReleaseInStackQueuedSpinLock(LockHandle);
}

//
// Raising & Lowering IRQL during IRP dispatching
//
FORCEINLINE
VOID
PopRaiseIrqlToDpc(
    _In_ PDEVICE_OBJECT Device,
    _Out_ PKIRQL OldIrql)
{
    if (!(Device->Flags & DO_POWER_PAGABLE))
    {
        KeRaiseIrql(DISPATCH_LEVEL, OldIrql);
    }
}

FORCEINLINE
VOID
PopLowerIrqlBack(
    _In_ KIRQL Irql)
{
    if (KeGetCurrentIrql() == DISPATCH_LEVEL)
    {
        KeLowerIrql(Irql);
    }
}

//
// Mark current location of a power IRP as completed
//
FORCEINLINE
VOID
PopMarkIrpCurrentLocationComplete(
    _Inout_ PIRP Irp)
{
    Irp->CurrentLocation = (Irp->StackCount + 2);
}

//
// Selects & Releases an inrush IRP for the system to process
//
FORCEINLINE
VOID
PopSelectInrushIrp(
    _In_ PIRP Irp)
{
    POP_ASSERT_IRP_LOCK_THREAD_OWNERSHIP();
    PopInrushIrp = Irp;
}

FORCEINLINE
VOID
PopReleaseInrushIrp(
    _In_ PIRP Irp)
{
    POP_ASSERT_IRP_LOCK_THREAD_OWNERSHIP();
    PopInrushIrp = NULL;
}

//
// Applies a power action that is global for the whole system
//
FORCEINLINE
VOID
PopApplyPowerAction(
    _In_ POWER_ACTION Action)
{
    ASSERT((Action >= PowerActionNone) && (Action <= PowerActionDisplayOff));
    PopAction.Action = Action;
}

//
// Applies a thermal zone state for the whole system
//
FORCEINLINE
VOID
PopApplyThermalZoneState(
    _In_ ULONG TzState)
{
    PopCoolingSystemMode |= TzState;
}

//
// Sets the default global policy for the system
//
FORCEINLINE
VOID
PopSetDefaultPowerPolicy(
    _In_ POP_POWER_POLICY_TYPE PolicyType)
{
    switch (PolicyType)
    {
        case PolicyAc:
        {
            PopDefaultPowerPolicy = &PopAcPowerPolicy;
            break;
        }

        case PolicyDc:
        {
            PopDefaultPowerPolicy = &PopDcPowerPolicy;
            break;
        }

        default:
        {
            ASSERT(FALSE);
        }
    }
}

//
// Policy locking mechanism
//
FORCEINLINE
VOID
PopAcquirePowerPolicyLock(VOID)
{
    /* The current thread's code has to be paged */
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    /* Acquire the policy lock */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&PopPowerPolicyLock, TRUE);
    PopPowerPolicyOwnerLockThread = KeGetCurrentThread();
}

FORCEINLINE
VOID
PopReleasePowerPolicyLock(VOID)
{
    /* The current thread's code has to be paged */
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    /* Release the policy lock */
    POP_ASSERT_POWER_POLICY_LOCK_OWNERSHIP();
    PopPowerPolicyOwnerLockThread = NULL;
    ExReleaseResourceLite(&PopPowerPolicyLock);

    /* And check for pending policy workers */
    PopCheckForPendingWorkers();
    KeLeaveCriticalRegion();
}

//
// Policy worker locking mechanism
//
FORCEINLINE
VOID
PopAcquirePowerPolicyWorkerLock(
    _Out_ PKIRQL Irql)
{
    KeAcquireSpinLock(&PopPowerPolicyWorkerLock, Irql);
}

FORCEINLINE
VOID
PopReleasePowerPolicyWorkerLock(
    _In_ KIRQL OldIrql)
{
    KeReleaseSpinLock(&PopPowerPolicyWorkerLock, OldIrql);
}

//
// Shutdown locking mechanism
//
FORCEINLINE
VOID
PopAcquireShutdownLock(VOID)
{
    KeAcquireGuardedMutex(&PopShutdownListMutex);
}

FORCEINLINE
VOID
PopReleaseShutdownLock(VOID)
{
    KeReleaseGuardedMutex(&PopShutdownListMutex);
}

//
// Power request locking mechanism
//
FORCEINLINE
VOID
PopAcquirePowerRequestLock(
    _Out_ PKLOCK_QUEUE_HANDLE LockHandle)
{
    KeAcquireInStackQueuedSpinLock(&PopPowerRequestLock, LockHandle);
    PopPowerRequestOwnerLockThread = KeGetCurrentThread();
}

FORCEINLINE
VOID
PopReleasePowerRequestLock(
    _In_ PKLOCK_QUEUE_HANDLE LockHandle)
{
    POP_ASSERT_POWER_REQUEST_LOCK_THREAD_OWNERSHIP();
    PopPowerRequestOwnerLockThread = NULL;
    KeReleaseInStackQueuedSpinLock(LockHandle);
}

//
// Power setting callback locking mechanism
//
FORCEINLINE
VOID
PopAcquirePowerSettingLock(VOID)
{
    ExAcquireFastMutex(&PopPowerSettingLock);
    PopPowerSettingOwnerLockThread = KeGetCurrentThread();
}

FORCEINLINE
VOID
PopReleasePowerSettingLock(VOID)
{
    POP_ASSERT_POWER_SETTING_LOCK_THREAD_OWNERSHIP();
    PopPowerSettingOwnerLockThread = NULL;
    ExReleaseFastMutex(&PopPowerSettingLock);
}

/* EOF */
