/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power Manager public API routines
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

static
BOOLEAN
PopIsDeviceRegisteredForIdleDetection(
    _In_ PDEVICE_OBJECT_POWER_EXTENSION TargetDope,
    _Out_opt_ PLIST_ENTRY *ReturnedDopeEntry)
{
    PLIST_ENTRY Entry;
    PDEVICE_OBJECT_POWER_EXTENSION Dope;

    PAGED_CODE();

    /* Passing a NULL DOPE is illegal */
    ASSERT(TargetDope);

    /* Iterate over the list of idle detect registered devices */
    for (Entry = PopIdleDetectList.Flink;
         Entry != &PopIdleDetectList;
         Entry = Entry->Flink)
    {
        /* Is this the DOPE we are looking for? */
        Dope = CONTAINING_RECORD(Entry, DEVICE_OBJECT_POWER_EXTENSION, IdleList);
        if (Dope == TargetDope)
        {
            /* This is it, return the DOPE entry to the caller if asked */
            if (ReturnedDopeEntry != NULL)
            {
                *ReturnedDopeEntry = Entry;
            }

            return TRUE;
        }
    }

    return FALSE;
}

static
BOOLEAN
PopIsCallerRunningAtRightIrql(
    _In_ POWER_STATE_TYPE Type,
    _In_ POWER_STATE State)
{
    /*
     * First rule, we must be above the operational IRQL that PoSetPowerState
     * allows for execution. Second rule, if the device slightly powers up
     * from sleeping (it is not D0) or it powers down, the IRQL must not
     * be above APC_LEVEL.
     */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);
    if (Type == DevicePowerState &&
        State.DeviceState != PowerDeviceD0)
    {
        if (KeGetCurrentIrql() > APC_LEVEL)
        {
            return FALSE;
        }
    }

    return TRUE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

NTSTATUS
NTAPI
PoCallDriver(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ __drv_aliasesMem PIRP Irp)
{
    PIO_STACK_LOCATION IrpStack;

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* The passed IRP must be a Power IRP */
    IrpStack = IoGetNextIrpStackLocation(Irp);
    POP_ASSERT_IRP_IS_POWER(IrpStack);

    /* Forward that IRP to the device */
    return IoCallDriver(DeviceObject, Irp);
}

NTSTATUS
NTAPI
PoRequestPowerIrp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ UCHAR MinorFunction,
    _In_ POWER_STATE PowerState,
    _In_opt_ PREQUEST_POWER_COMPLETE CompletionFunction,
    _In_opt_ __drv_aliasesMem PVOID Context,
    _Outptr_opt_ PIRP *Irp)
{
    /* Invoke the private helper to do the deed */
    return PopRequestPowerIrp(DeviceObject,
                              MinorFunction,
                              PowerState,
                              FALSE,
                              FALSE,
                              CompletionFunction,
                              Context,
                              Irp);
}

VOID
NTAPI
PoStartNextPowerIrp(
    _Inout_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(Irp);
    NOTHING;
}

PVOID
NTAPI
PoRegisterSystemState(
    _Inout_opt_ PVOID StateHandle,
    _In_ EXECUTION_STATE Flags)
{
    NTSTATUS Status;
    BOOLEAN AlreadyRegistered = FALSE;
    PPOP_POWER_REQUEST StateHandleRequest;

    PAGED_CODE();

    /*
     * If this was an already registered system state handle then
     * simply instruct Power Manager to use that handle.
     */
    if (StateHandle != NULL)
    {
        StateHandleRequest = (PPOP_POWER_REQUEST)StateHandle;
        AlreadyRegistered = TRUE;
    }

    /* Invoke the power request private API to do the deed for us */
    Status = PopRegisterPowerRequest(NULL,
                                     RegisterLegacyRequest,
                                     AlreadyRegistered,
                                     Flags,
                                     NULL,
                                     &StateHandleRequest);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to register a system state with the Power Manager (Status 0x%lx)\n", Status);
        return NULL;
    }

    return (PVOID)StateHandleRequest;
}

VOID
NTAPI
PoUnregisterSystemState(
    _Inout_ PVOID StateHandle)
{
    PPOP_POWER_REQUEST StateHandleRequest;

    PAGED_CODE();

    /* Passing a NULL state handle is illegal */
    ASSERT(StateHandle != NULL);

    /*
     * Since our state handle is simply a registered object
     * invoke the Object Manager to call the delete procedure
     * of the power request handle. It only has one reference
     * count set up at the time of registering this object so
     * the delete procedure will be called immediately.
     */
    StateHandleRequest = (PPOP_POWER_REQUEST)StateHandle;
    ObDereferenceObject(StateHandleRequest);
}

VOID
NTAPI
PoSetSystemState(
    _In_ EXECUTION_STATE Flags)
{
    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* The caller did not supply any of the execution state flags, bail out */
    if (Flags & ~(ES_SYSTEM_REQUIRED  |
                  ES_DISPLAY_REQUIRED |
                  ES_USER_PRESENT))
    {
        DPRINT1("Invalid flag mask was supplied (Flag %lu)\n", Flags);
        ASSERT(FALSE);
        return;
    }

    /*
     * Also make sure the caller does not play us for absolute fools by
     * supplying ES_CONTINUOUS to a function of which the system state
     * cannot be persisted as per the documentation.
     */
    if (Flags & ES_CONTINUOUS)
    {
        DPRINT1("ES_CONTINUOUS was set when it must not be\n");
        ASSERT(FALSE);
        return;
    }

    /* Notify the Power Manager of the current busy state of the system */
    PopIndicateSystemStateActivity(Flags);
}

PULONG
NTAPI
PoRegisterDeviceForIdleDetection(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG ConservationIdleTime,
    _In_ ULONG PerformanceIdleTime,
    _In_ DEVICE_POWER_STATE State)
{
    KIRQL OldIrql;
    PLIST_ENTRY DopeEntry;
    PEXTENDED_DEVOBJ_EXTENSION DeviceExtension;
    PDEVICE_OBJECT_POWER_EXTENSION Dope;
    BOOLEAN IsDeviceDisk = FALSE;

    PAGED_CODE();

    /* The caller has to pass a DO here */
    ASSERT(DeviceObject);

    /* The caller wants to cancel the idle detection for this device */
    if (!ConservationIdleTime && !PerformanceIdleTime)
    {
        /* Grab the DOPE from the device and remove it from the idle detect list */
        PopAcquireDopeLock(&OldIrql);
        DeviceExtension = IoGetDevObjExtension(DeviceObject);
        Dope = DeviceExtension->Dope;
        if (Dope)
        {
            /* Remove the DOPE entry only if the device was registered to begin with */
            if (PopIsDeviceRegisteredForIdleDetection(Dope, &DopeEntry))
            {
                /*
                 * This device is about to disable its idle detection but make sure
                 * no active busy references are currently in force. Supposedly this
                 * is the case, the caller has forgot to end its busy periods with
                 * PoEndDeviceBusy for each PoStartDeviceBusy call it has instantiated.
                 */
                ASSERT(Dope->BusyReference == 0);

                /* Unlink it from the global idle detect list */
                RemoveEntryList(DopeEntry);

                /* Scrub the idle and busy counters */
                Dope->IdleCount = 0;
                Dope->BusyCount = 0;
                Dope->TotalBusyCount = 0;

                /* Reset the idle time values */
                Dope->ConservationIdleTime = 0;
                Dope->PerformanceIdleTime = 0;

                /* Reset the power state values of this DOPE */
                Dope->IdleState = PowerDeviceUnspecified;
                Dope->CurrentState = PowerDeviceUnspecified;

                /* And finally, reset the idle list link */
                InitializeListHead(&Dope->IdleList);
            }
        }

        PopReleaseDopeLock(OldIrql);
        return NULL;
    }

    /*
     * The naughty caller thinks that passing PowerDeviceUnspecified as power
     * state to be requested when either of the two idle timers have fired is
     * correct. It will just make things worse as the supplied power state will
     * be passed from the power manager to the device driver to set a new power
     * state, of an unspecified type. This will put the device into a limbo
     * situation as nobody will know if the device is turned ON or whatever.
     */
    if (State == PowerDeviceUnspecified)
    {
        DPRINT1("WARNING: The DO (0x%p) attempted to set state PowerDeviceUnspecified\n", DeviceObject);
        ASSERT(State != PowerDeviceUnspecified);
        return NULL;
    }

    /*
     * The caller wants to register default idle time values that the power
     * manager it provides. This is supported only for devices that is a disk
     * device or a mass storage device. Ignore the request for unsupported devices.
     */
    if (ConservationIdleTime == -1 && PerformanceIdleTime == -1)
    {
        if (!(DeviceObject->DeviceType & FILE_DEVICE_DISK) &&
            !(DeviceObject->DeviceType & FILE_DEVICE_MASS_STORAGE))
        {
            DPRINT("Default idle times requested for an unsupported device type (%lu) of DO (0x%p)\n", DeviceObject->DeviceType, DeviceObject);
            return NULL;
        }

        /* This is a disk/mass storage, we will mark it as such in the DOPE later */
        IsDeviceDisk = TRUE;
    }

    /* Get the DOPE of this device */
    Dope = PopGetDope(DeviceObject);
    if (!Dope)
    {
        DPRINT1("Failed to get DOPE or allocate one for DO (0x%p), bailing out\n", DeviceObject);
        return NULL;
    }


    /* Lock the global DOPE database as this device will get its DOPE modified */
    PopAcquireDopeLock(&OldIrql);

    /* Fill in idle detection datum */
    Dope->IdleCount = 0;
    Dope->ConservationIdleTime = ConservationIdleTime;
    Dope->PerformanceIdleTime = PerformanceIdleTime;
    Dope->IdleState = State;
    Dope->IdleType = (IsDeviceDisk == TRUE) ? DeviceIdleDisk : DeviceIdleNormal;

    /*
     * Check that if the device has already registered for idle detection
     * and it just wanted to adjust the idle time and state values. Insert it
     * to the registered devices for idle detection if this is for the first time.
     */
    if (!PopIsDeviceRegisteredForIdleDetection(Dope, NULL))
    {
        /*
         * At the time of registering for idle detection, this device was in fully
         * operational mode (this has to be in this state in order to do that).
         */
        Dope->CurrentState = PowerDeviceD0;
        InsertTailList(&PopIdleDetectList, &Dope->IdleList);
    }

    /*
     * As this device has registered with the power manager for idle time
     * detection, once the idle times have fired up, the device will incur
     * in a power state change as per the request of whom actually wanted
     * this device to be registered. Deploy any pending policy workers that
     * have been left behind.
     */
    PopReleaseDopeLock(OldIrql);
    PopCheckForPendingWorkers();

    return (PULONG)&Dope->IdleCount;
}

POWER_STATE
NTAPI
PoSetPowerState(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ POWER_STATE_TYPE Type,
    _In_ POWER_STATE State)
{
    POWER_STATE PrevState;
    KLOCK_QUEUE_HANDLE IrpLockHandle;
    PEXTENDED_DEVOBJ_EXTENSION DeviceExtension;

    /*
     * Unlike other conventional PoXxx functions, this one can allow execution
     * not above DISPATCH_LEVEL if a device decides to power down or slightly up.
     * Assert this condition.
     */
    ASSERT(PopIsCallerRunningAtRightIrql(Type, State));

    /*
     * Acquire the IRP lock here, so we avoid unexpected power transitions.
     * Especially if this device happens to power down or up out of a sudden
     * while we are here (though unlikely), so we can set a new power state safely.
     */
    PopAcquireIrpLock(&IrpLockHandle);
    DeviceExtension = IoGetDevObjExtension(DeviceObject);

    /* Apply new power state depending on type and if it is necessary */
    if (Type == SystemPowerState)
    {
        PrevState.SystemState = PopGetDoePowerState(DeviceExtension, TRUE);
        if (PrevState.SystemState != State.SystemState)
        {
            /* The system is transitioning into a new state, apply it */
#if DBG
            DPRINT1("System is transitioning to a new power state %s -> %s (DO 0x%p)\n",
                    PopTranslateSystemPowerStateToString(PrevState.SystemState), PopTranslateSystemPowerStateToString(State.SystemState), DeviceObject);
#endif
            PopSetDoePowerState(DeviceExtension, State, TRUE);
        }

    }
    else // DevicePowerState
    {
        PrevState.DeviceState = PopGetDoePowerState(DeviceExtension, FALSE);
        if (PrevState.DeviceState != State.DeviceState)
        {
            /* This device is transitioning into a new state, apply it */
#if DBG
            DPRINT1("Device is transitioning to a new power state %s -> %s (DO 0x%p)\n",
                    PopTranslateDevicePowerStateToString(PrevState.DeviceState), PopTranslateDevicePowerStateToString(State.DeviceState), DeviceObject);
#endif
            PopSetDoePowerState(DeviceExtension, State, FALSE);
        }
    }

    PopReleaseIrpLock(&IrpLockHandle);
    return PrevState;
}

NTSTATUS
NTAPI
PoRegisterPowerSettingCallback(
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_ LPCGUID SettingGuid,
    _In_ PPOWER_SETTING_CALLBACK Callback,
    _In_opt_ PVOID Context,
    _Outptr_opt_ PVOID *Handle)
{
    /* FIXME */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
PoUnregisterPowerSettingCallback(
    _Inout_ PVOID Handle)
{
    /* FIXME */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
PoSetSystemWake(
    _Inout_ PIRP Irp)
{
    PPOP_IRP_DATA IrpData;
    PIO_STACK_LOCATION IrpSp;
    KLOCK_QUEUE_HANDLE IrpLockHandle;

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* This has to be a wake power IRP, otherwise the IRP we got is bogus */
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    POP_ASSERT_IRP_IS_WAKE(IrpSp);

    /*
     * The IRP of the device that contributed to the waking of the system is
     * this one, so set it as "woke IRP".
     */
    PopAcquireIrpLock(&IrpLockHandle);
    IrpData = PopFindIrpData(Irp, NULL, SearchByIrp);
    ASSERT(IrpData != NULL);
    IrpData->Device.SystemWake = TRUE;
    PopReleaseIrpLock(&IrpLockHandle);
}

BOOLEAN
NTAPI
PoGetSystemWake(
    _In_ PIRP Irp)
{
    PPOP_IRP_DATA IrpData;
    PIO_STACK_LOCATION IrpSp;
    KLOCK_QUEUE_HANDLE IrpLockHandle;

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* This has to be a wake power IRP, otherwise the IRP we got is bogus */
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    POP_ASSERT_IRP_IS_WAKE(IrpSp);

    /* Grab the IRP power data based on the given IRP */
    PopAcquireIrpLock(&IrpLockHandle);
    IrpData = PopFindIrpData(Irp, NULL, SearchByIrp);
    PopReleaseIrpLock(&IrpLockHandle);

    /* Does this IRP contributed to the waking of the system? */
    if (IrpData->Device.SystemWake)
    {
        /* That's the guy */
        return TRUE;
    }

    return FALSE;
}

VOID
NTAPI
PoSetDeviceBusyEx(
    _Inout_ PULONG IdlePointer)
{
    /* Bail out on NULL idle pointers */
    ASSERT(IdlePointer);

    /* This device is about to get busy so increment the busy count by one */
    InterlockedIncrement((volatile LONG *)&IdlePointer + POP_BUSY_COUNT_OFFSET);
}

VOID
NTAPI
PoStartDeviceBusy(
    _Inout_ PULONG IdlePointer)
{
    /* Bail out on NULL idle pointers */
    ASSERT(IdlePointer);

    /* This device is about to get busy so keep an active reference */
    InterlockedIncrement((volatile LONG *)&IdlePointer + POP_BUSY_REFERENCE_OFFSET);
}

VOID
NTAPI
PoEndDeviceBusy(
    _Inout_ PULONG IdlePointer)
{
    /* Bail out on NULL idle pointers */
    ASSERT(IdlePointer);

    /* This device is no longer busy, take a reference away */
    InterlockedDecrement((volatile LONG *)&IdlePointer + POP_BUSY_REFERENCE_OFFSET);
}

BOOLEAN
NTAPI
PoQueryWatchdogTime(
    _In_ PDEVICE_OBJECT Pdo,
    _Out_ PULONG SecondsRemaining)
{
    PPOP_IRP_DATA IrpData;
    KLOCK_QUEUE_HANDLE IrpLockHandle;

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Assume the watchdog of this power IRP is not enabled yet */
    *SecondsRemaining = 0;

    /* Look for the appropriate IRP power data that belongs to this PDO */
    PopAcquireIrpLock(&IrpLockHandle);
    IrpData = PopFindIrpData(NULL, Pdo, SearchByDevice);
    PopReleaseIrpLock(&IrpLockHandle);

    /*
     * No IRP power data was found for this PDO, therefore no power IRPs
     * were issued from this PDO.
     */
    if (!IrpData)
    {
        return FALSE;
    }

    /* A power IRP watchdog is currently in force, return the watchdog counter */
    if (IrpData->WatchdogEnabled == TRUE)
    {
        *SecondsRemaining = IrpData->WatchdogStart;
        return TRUE;
    }

    return FALSE;
}

NTSTATUS
NTAPI
PoSetPowerRequest(
    _Inout_ PVOID PowerRequest,
    _In_ POWER_REQUEST_TYPE Type)
{
    NTSTATUS Status;
    KLOCK_QUEUE_HANDLE LockHandle;
    PPOP_POWER_REQUEST PowerRequestObject;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /*
     * This function is not supposed to be used for legacy power
     * requests but ONLY the newer ones!
     */
    PopAcquirePowerRequestLock(&LockHandle);
    PowerRequestObject = (PPOP_POWER_REQUEST)PowerRequest;
    ASSERT(PowerRequestObject->Legacy == FALSE);

    /* Invoke the private API helper to do the deed for us */
    Status = PopChangePowerRequestProperties(PowerRequestObject,
                                             Type,
                                             PreviousMode,
                                             FALSE);
    PopReleasePowerRequestLock(&LockHandle);

    return Status;
}

VOID
NTAPI
PoDeletePowerRequest(
    _Inout_ PVOID PowerRequest)
{
    PPOP_POWER_REQUEST PowerRequestObject;

    PAGED_CODE();

    /* Passing a NULL power request is illegal here */
    ASSERT(PowerRequest != NULL);

    /*
     * This function is not supposed to be used for legacy power
     * requests but ONLY the newer ones!
     */
    PowerRequestObject = (PPOP_POWER_REQUEST)PowerRequest;
    ASSERT(PowerRequestObject->Legacy == FALSE);

    /* Invoke the close procedure on this object so that it can go away */
    ObDereferenceObject(PowerRequestObject);
}

NTSTATUS
NTAPI
PoCreatePowerRequest(
    _Outptr_ PVOID *PowerRequest,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PCOUNTED_REASON_CONTEXT Context)
{
    NTSTATUS Status;
    PPOP_POWER_REQUEST PowerRequestObject;

    PAGED_CODE();

    /*
     * The naughty caller passed NULL when we should return to them
     * the created power request. Bail out.
     */
    if (PowerRequest == NULL)
    {
        DPRINT1("Failed to create a power request, the caller passed a NULL parameter\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Call the private API to do the deed for us */
    Status = PopRegisterPowerRequest(DeviceObject,
                                     RegisterALaVistaRequest,
                                     FALSE,
                                     0,
                                     Context,
                                     &PowerRequestObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to register a power request with the Power Manager (Status 0x%lx)\n", Status);
        *PowerRequest = NULL;
        return Status;
    }

    /* Give the allocated power request to the caller */
    *PowerRequest = PowerRequestObject;
    return Status;
}

NTSTATUS
NTAPI
PoClearPowerRequest(
    _Inout_ PVOID PowerRequest,
    _In_ POWER_REQUEST_TYPE Type)
{
    NTSTATUS Status;
    KLOCK_QUEUE_HANDLE LockHandle;
    PPOP_POWER_REQUEST PowerRequestObject;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();

    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /*
     * This function is not supposed to be used for legacy power
     * requests but ONLY the newer ones!
     */
    PopAcquirePowerRequestLock(&LockHandle);
    PowerRequestObject = (PPOP_POWER_REQUEST)PowerRequest;
    ASSERT(PowerRequestObject->Legacy == FALSE);

    /* Invoke the private API helper to do the deed for us */
    Status = PopChangePowerRequestProperties(PowerRequestObject,
                                             Type,
                                             PreviousMode,
                                             TRUE);
    PopReleasePowerRequestLock(&LockHandle);

    return Status;
}

NTKERNELAPI
NTSTATUS
PoCreateThermalRequest(
    _Outptr_ PVOID *ThermalRequest,
    _In_ PDEVICE_OBJECT TargetDeviceObject,
    _In_ PDEVICE_OBJECT PolicyDeviceObject,
    _In_ PCOUNTED_REASON_CONTEXT Context,
    _In_ ULONG Flags)
{
    /* FIXME */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
PoDeleteThermalRequest(
    _Inout_ PVOID ThermalRequest)
{
    /* FIXME */
    UNIMPLEMENTED;
    NOTHING;
}

/* EOF */
