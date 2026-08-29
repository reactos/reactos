/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Power Manager public API routines
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @brief
 * Checks if a device is registered for idle detection and
 * returns the Device Object Power Extension (DOPE) entry
 * if needed.
 *
 * @param[in] TargetDope
 * A a pointer to a DOPE of a device which is checked if the
 * said device is registered for device idle detection.
 *
 * @param[out] ReturnedDopeEntry
 * A pointer to the entry of the DOPE from the idle detection
 * list, returned to the caller. This parameter is optional
 * if the caller doesn't want it.
 *
 * @return
 * Returns TRUE if the said device is registered for idle
 * detection, FALSE otherwise.
 */
static
BOOLEAN
PopIsDeviceRegisteredForIdleDetection(
    _In_ PDEVICE_OBJECT_POWER_EXTENSION TargetDope,
    _Out_opt_ PLIST_ENTRY *ReturnedDopeEntry)
{
    PLIST_ENTRY Entry;
    PDEVICE_OBJECT_POWER_EXTENSION Dope;

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

/**
 * @brief
 * Checks if the current calling thread is running at the
 * correct IRQL. This function is exclusively used by
 * PoSetPowerState.
 *
 * @param[in] Type
 * The type of power state of which the caller is setting
 * for.
 *
 * @param[in] State
 * The power state to be set.
 *
 * @return
 * Returns TRUE if the caller is running at the right IRQL
 * when setting a certain power state.
 */
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

/**
 * @brief
 * Forwards a power I/O request packet down to the target lower
 * device driver within the device stack.
 *
 * @param[in] DeviceObject
 * A pointer to a device object created by the driver. The Power Manager
 * will forward the power IRP down to the device stack by starting at
 * the pointed device by this parameter.
 *
 * @param[in,out] Irp
 * A pointer to a power IRP of which power I/O request has to be passed
 * down to the device.
 *
 * @return
 * STATUS_SUCCESS is returned if the operation has completed successfully
 * and was synchronous. STATUS_PENDING is returned if the IRP was queued
 * and it has to be forwarded asynchronously. A failure NTSTATUS code
 * is returned otherwise.
 *
 * @remarks
 * The following function is obsolete and only reserved for compatibility
 * purposes. Prior Windows NT 5.x the driver was responsible of the IRP
 * forwarding flow with PoCallDriver and PoStartNextPowerIrp at completion.
 * Starting with Windows Vista, drivers are recommended to use IoCallDriver
 * instead.
 */
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

/**
 * @brief
 * Creates a power IRP on behalf of the caller (such as a driver
 * attempting to power up or down its device) and forwards it
 * at the top of the device within the device stack chain.
 *
 * @param[in] DeviceObject
 * A pointer to a device object created by the driver. This can be a
 * a physical device object (PDO) or a functional device object (FDO).
 * The Power Manager, once it allocates the power IRP, it'll immediately
 * call the target device object pointed by this parameter.
 *
 * @param[in] MinorFunction
 * A minor function power code, specified by the caller. This code represents
 * the power request that the caller wants to take. The following codes are:
 *
 * IRP_MN_QUERY_POWER -- Queries the power state of a device
 *
 * IRP_MN_SET_POWER -- Sets a new power state, PowerState must point to the
 *                     new power state that it wants to take.
 *
 * IRP_MN_WAIT_WAKE -- Awakes a device based upon the target power state,
 *                     pointed by PowerState.
 *
 * @param[in] PowerState
 * A power state, given by the caller. This can either be a power state for the
 * system, or for a driver. Ordinary callers are discouraged from setting
 * a new system power state, that is reserved by the system itself!
 *
 * @param[in] CompletionFunction
 * A pointer to a function of which it gets called by the Power Manager when
 * the power IRP has completed its journey of walking down the device stack
 * chain. This parameter is optional.
 *
 * @param[in] Context
 * A pointer to a caller's supplied context parameter that is passed to the
 * completion function. This parameter is optional.
 *
 * @param[in,out] Irp
 * A pointer to the allocated power IRP, returned to the caller. This parameter
 * must be NULL if the function code is IRP_MN_WAIT_WAKE because the IRP
 * might be completed before this function returns and it'll be discarded
 * from memory.
 *
 * @return
 * Returns STATUS_PENDING to indicate the power IRP has been sent to the
 * target device. STATUS_INSUFFICIENT_RESOURCES is returned if the IRP couldn't
 * be allocated. STATUS_INVALID_PARAMETER_2 is returned if the function code
 * pointed by MinorFunction is not valid. A failure NTSTATUS code is returned
 * otherwise.
 *
 * @remarks
 * This function forwards the power IRP asynchronously. That is, the IRP might
 * be enqueued if the said device driver has requested more than one power IRP.
 * With that said, a device caller can request many power IRPs at its will as
 * it wants. Thus, the caller must NOT ASSUME the power IRP will be always
 * handled synchronously and it MUST NOT write code based on this expectation!!!
 */
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

/**
 * @brief
 * Informs the Power Manager the device driver has finished completing
 * its power IRP and is ready to handle another one.
 *
 * @param[in] Irp
 * A pointer to a power IRP that's been completed. On function exit,
 * this parameter will point to a new power IRP that has to be handled
 * by the driver.
 *
 * @remarks
 * This function is obsolete and does nothing on ReactOS. Starting with
 * Windows Vista, the Power Manager is fully responsible to enqueue and
 * dequeue power IRPs at completion process, the device driver doesn't
 * need to do anything.
 */
VOID
NTAPI
PoStartNextPowerIrp(
    _Inout_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(Irp);
    NOTHING;
}

/**
 * @brief
 * Registers a system handle that makes the system busy. The Power Manager
 * will alert the system that an activity is in progress and it will
 * inhibit certain system power operations, like going to sleep.
 *
 * @param[in,out] StateHandle
 * A pointer to a system state handle created by the Power Manager.
 * The Power Manager keeps track of registered handles to determine
 * that system activity is occuring and busy due to external factors
 * imposed by device drivers that call this function. This parameter
 * can point to an already existing state handle if the caller wants
 * to make the system busy for other reasons. Otherwise if this is
 * a first registration, this parameter must be NULL!
 *
 * @param[in] Flags
 * A flag bit that indicates the type of activity the system must incur
 * because of the caller. The following flags are:
 *
 * ES_SYSTEM_REQUIRED -- The system is busy, this prevents the system from
 *                       going to sleep, low power mode or stay idle.
 *
 * ES_DISPLAY_REQUIRED -- The display is always used, this prevents the display
 *                        from going dim or turning off.
 *
 * ES_USER_PRESENT -- Indicates that a user is present physically and is using
 *                    the machine. This is tipically used in conjuction with a
 *                    user waking up the system like using a keyboard which
 *                    generated a wake-up signal input event. This flag is useful
 *                    to differentiate wake-up events or activity occurences due
 *                    to human interference and computer-driven interference.
 *
 * ES_CONTINUOUS -- The following ES_XXX settings must remain in effect until
 *                  explicit further change.
 *
 * @return
 * Returns a registered system state handle to the caller. If such a handle was
 * already created before, it returns the previous created handle. NULL is returned
 * if the handle couldn't be created.
 *
 * @remarks
 * This is a legacy API function! For the newer version of this API call, use
 * PoCreatePowerRequest and its variants.
 */
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

/**
 * @brief
 * Unregisters a system handle, previously created by PoRegisterSystemState.
 *
 * @param[in,out] StateHandle
 * A pointer to a system state handle, given by the caller.
 * This parameter cannot be NULL!
 */
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

/**
 * @brief
 * Makes the system busy due to certain activity that is
 * currently occuring.
 *
 * @param[in] Flags
 * A flag bit that indicates the type of activity the system must incur
 * because of the caller. Refer to PoRegisterSystemState for more
 * information about these flags.
 *
 * @remarks
 * This function works in the same fashion as PoRegisterSystemState, that
 * is, it makes the system busy due to activity. The only difference is that
 * while registering a system state handle makes the busy state persistent,
 * this function does not. With that said, this function is useful to indicate
 * the system state activity for a shorter period of time.
 */
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

/**
 * @brief
 * Registers a device for idle detection. The Power Manager has a dedicated
 * idle detection mechanism of which, if a deviceh has been idling for too
 * long based on the pointed parameters, the Power Manager will change
 * the power state of the said device.
 *
 * @param[in] DeviceObject
 * A pointer to a device object created by the driver. This can be a
 * a physical device object (PDO) or a functional device object (FDO).
 * The Power Manager will need this to register the device for idle detection.
 *
 * @param[in] ConservationIdleTime
 * A caller supplied conservation idle time, in seconds. This time is used
 * in the context of conservation energy policy. The Power Manager uses this
 * idle time if the current active power policy of the system is DC (aka
 * the system is powered by batteries).
 *
 * @param[in] PerformanceIdleTime
 * A caller supplied performance idle time, in seconds. This time is used
 * in the context of performance energy policy. The Power Manager uses this
 * idle time if the current active power policy of the system is AC (aka
 * the system is powered by AC source like AC power cord).
 *
 * @param[in] State
 * A device power state, given by the caller. The Power Manager uses this
 * value to set a new power state to the device once the device has been
 * idling for too long.
 *
 * @return
 * Returns a pointer to the address of the idle time counter of the device
 * that has been registered. The caller can use such idle counter to reset
 * the idleness of its device by using the appropriate functions.
 * NULL is returned if the idle detection has been cancelled on behalf of
 * the caller or registering for idle detection has failed.
 *
 * @remarks
 * Drivers can set both ConservationIdleTime and PerformanceIdleTime to 0
 * if one wants to cancel idle detection for the target device. If the
 * target device is a disk device or mass storage, the caller can set both
 * these parameters to -1 so that the Power Manager will assign default
 * idle time values from the currently enacted power policy.
 *
 * This function inquires the Power Policy Manager to check for policy workers
 * to be deplyed. This is because there might be a pending system idle worker
 * request that wasn't deployed yet, of which it'll notify the rest of the
 * system (and the drivers too) that the system is indeed idling.
 */
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

/**
 * @brief
 * Assigns a new power state to a device. This function is tipically
 * used when a driver's code switches the power state of a device
 * and then it informs the Power Manager of this fact with this function.
 * Power state for the system is assigned in reservation to the system
 * core itself.
 *
 * @param[in] DeviceObject
 * A pointer to the target device object of which a new power state
 * is to be assigned.
 *
 * @param[in] Type
 * The type of power state, given by the caller. Drivers must set this
 * to DevicePowerState. System power state is reserved only to critical
 * core compoments of the OS.
 *
 * @param[in] State
 * The new power state to be assigned, given by the caller.
 *
 * @return
 * Returns the previous power state.
 *
 * @remarks
 * This function acquires the power IRP lock. This is because we must avoid
 * unexpecting power transitions of a device at the time the caller is assigning
 * a new power state to this device, so that power states of the said device
 * are properly synchronized.
 */
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

/**
 * @brief
 * Registers a power setting callback of which a device driver gets
 * notified of changes in a power setting.
 *
 * @param[in] DeviceObject
 * A pointer to a device object of whom it registers a power setting.
 * This parameter is optional as it's used for debugging purposes.
 *
 * @param[in] SettingGuid
 * A pointer to a power setting GUID that identifies the power setting.
 * Refer to Wdm.h for more information about these GUIDs.
 *
 * @param[in] Callback
 * A pointer to a power setting callback, supplied by the caller. The
 * Power Manager calls this callback when a change in state of a power
 * setting for this callback has occurred.
 *
 * @param[in] Context
 * A pointer to a context, supplied by the caller. This serves as an
 * additional argument to be passed to the callback if need be. This
 * parameter is optional.
 *
 * @param[out] Handle
 * A registered handle to a power setting callback, returned to the
 * caller.
 *
 * @return
 * Returns STATUS_SUCCESS if the handle registration has completed
 * successfully, or if the callback was already registered.
 * STATUS_ INSUFFICIENT_RESOURCES is returned if memory couldn't be
 * allocated for the callback.
 *
 * @remarks
 * Power settings are usually initialized when a caller registers
 * notifications for the respective power setting. That is, the Power
 * Manager will initialize the value (depending on the nature of the
 * power setting itself) and immediately invoke the driver's supplied
 * callback. With that said, the callback might be called before this
 * function returns. The caller is reponsible to unregister its handle
 * with PoUnregisterPowerSettingCallback.
 */
NTSTATUS
NTAPI
PoRegisterPowerSettingCallback(
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_ LPCGUID SettingGuid,
    _In_ PPOWER_SETTING_CALLBACK Callback,
    _In_opt_ PVOID Context,
    _Outptr_opt_ PVOID *Handle)
{
    NTSTATUS Status;
    PKSTART_ROUTINE Routine;
    PPOP_POWER_SETTING_CALLBACK SettingCallback;

    /* This function only permits IRQL at the lowest level */
    POP_ASSERT_IRQL_PASSIVE();

    /* Lock the entire callbacks list as we're about to register a new callback */
    PopAcquirePowerSettingLock();

    /*
     * Check that if the caller has already registered a power setting
     * callback before or not. We don't want to bloat the list with
     * duplicated callbacks, simply return the already registered
     * callback to the caller.
     */
    SettingCallback = PopFindPowerSettingCallbackByCallback(Callback);
    if (SettingCallback)
    {
        DPRINT("The caller has already registered a power setting callback before, just return that (Callback 0x%p)\n", SettingCallback);
        *Handle = SettingCallback;
        PopReleasePowerSettingLock();
        return STATUS_SUCCESS;
    }

    /*
     * The callback was never registered, allocate some memory space
     * for this callback and register it as well as notify the power
     * setting worker immediately to set the value to the driver's
     * callback for the first time.
     */
    Status = PopAllocatePowerSettingCallback(DeviceObject,
                                             SettingGuid,
                                             Callback,
                                             Context,
                                             &SettingCallback);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to allocate memory for the power setting callback (Status 0x%lx)\n", Status);
        PopReleasePowerSettingLock();
        return Status;
    }

    /* It's been registered, count it as one of the registered callbacks */
    InterlockedIncrementUL(&PopPowerSettingCallbacksCount);

    /* Get the power setting worker */
    Routine = PopGetPowerSettingHelper(SettingGuid);
    if (Routine == NULL)
    {
        /*
         * FIXME: If a specific power setting doesn't come with a dedicated
         * setting worker then use the default worker which takes care of
         * unimplemented power settings. This is a placebo solution and
         * it must be treated as such. As every power setting is implemented
         * this workaround must be removed!
         */
        Routine = PopUnimplementedPowerSettingWorker;
    }

    /* Service this callback to a worker thread */
    Status = PopCreateWorkerThread(Routine,
                                   SettingCallback,
                                   POP_POWER_SETTING_WORKER_THREAD_PRIORITY);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create a power manager worker thread for the power setting (Status 0x%lx)\n", Status);
        PopReleasePowerSettingCallback(SettingCallback);
        PopReleasePowerSettingLock();
        return Status;
    }

    /* The power setting callback is all set up, we no longer need the lock */
    PopReleasePowerSettingLock();

    /*
     * Now wait for the power setting worker to enter into the caller's
     * supplied callback routine to set an initial value. Note that as
     * per the MSDN documentation, the callback routine might (and by
     * absolute chance) be called right before this function returns.
     */
    KeWaitForSingleObject(&SettingCallback->CallbackReturned,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    /* Give the setting callback handle to the caller */
    *Handle = SettingCallback;
    return Status;
}

/**
 * @brief
 * Unregisters a power setting callback.
 *
 * @param[in,out] Handle
 * A pointer to a handle of the power setting callback that's
 * been registered.
 *
 * @return
 * Returns STATUS_SUCCESS if the handle was unregistered successfully.
 * STATUS_INVALID_PARAMETER is returned if the pointed handle is not
 * a valid one.
 */
NTSTATUS
NTAPI
PoUnregisterPowerSettingCallback(
    _Inout_ PVOID Handle)
{
    PLIST_ENTRY ListHead, NextEntry;
    PPOP_POWER_SETTING_CALLBACK SettingCallback;
    BOOLEAN CallbackFound = FALSE;

    /* This function only permits IRQL at the lowest level */
    POP_ASSERT_IRQL_PASSIVE();

    /* Lock the entire callbacks list, this callback is about to be unregistered */
    PopAcquirePowerSettingLock();

    /* Iterate over the registered power setting callbacks and look for the targer callback */
    ListHead = &PopPowerSettingCallbacksList;
    for (NextEntry = ListHead->Flink;
         NextEntry != ListHead;
         NextEntry = NextEntry->Flink)
    {
        /* If this is the target callback based on the given handle, stop looking further */
        SettingCallback = CONTAINING_RECORD(NextEntry, POP_POWER_SETTING_CALLBACK, Link);
        if (SettingCallback == Handle)
        {
            CallbackFound = TRUE;
            break;
        }
    }

    /*
     * The caller is fooling us by giving a handle of whatever thing
     * that we don't know what it is, as the handle doesn't correspond to
     * any of the registered callbacks. Punt the caller!
     */
    if (!CallbackFound)
    {
        DPRINT1("No registered power setting callback was found with this handle (Handle: 0x%p)\n", Handle);
        PopReleasePowerSettingLock();
        return STATUS_INVALID_PARAMETER;
    }

    /*
     * At the time of locking down the callbacks list, there may be a power
     * worker already executing into driver's supplied callback routine or
     * it's about to be notified soon. We don't know how long it'll take for
     * the callback routine to finish so we must wait for the thread worker
     * to finish before we unregister the callback.
     */
    if (PopPowerSettingReadAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK) ||
        PopPowerSettingReadAttribute(SettingCallback, POP_PSC_GETTING_NOTIFIED))
    {
        KeWaitForSingleObject(&SettingCallback->CallbackReturned,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    /* Mark the callback for un-registration and free up the callback handle */
    PopPowerSettingApplyAttribute(SettingCallback, POP_PSC_UNREGISTERED);
    PopReleasePowerSettingCallback(SettingCallback);

    /* It's been unregistered, count the number of registered callbacks one less */
    InterlockedDecrementUL(&PopPowerSettingCallbacksCount);
    PopReleasePowerSettingLock();
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Marks the power IRP as a "woke" IRP that was responsible
 * for waking up the system.
 *
 * @param[in,out] Irp
 * A pointer to a power IRP of which it has be marked responsible
 * for waking up the system.
 *
 * @remarks
 * The power IRP lock is held here, as we don't want for this IRP
 * to suddenly fade away in our lap or leading towards unexpecting
 * power transitions.
 */
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

/**
 * @brief
 * Determines if the power IRP is the one that was
 * responsible for waking up the system.
 *
 * @param[in] Irp
 * A pointer to a power IRP of which the Power Manager checks
 * if it was responsible for waking up the system.
 *
 * @return
 * Returns TRUE if the pointed IRP woke up the system, FALSE
 * otherwise.
 */
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

/**
 * @brief
 * Marks the device whom it's been registered for idle detection
 * as busy.
 *
 * @param[in,out] IdlePointer
 * A pointer to the address of the idle counter of the device
 * that's registered for idle detection.
 *
 * @remarks
 * This function increments the busy count by offsetting right through
 * the member field. The function assumes the integrity of the pointer
 * address therefore it's the caller's responsibility to not trash it.
 */
VOID
NTAPI
PoSetDeviceBusyEx(
    _Inout_ PULONG IdlePointer)
{
    PDEVICE_OBJECT_POWER_EXTENSION Dope;

    /* Bail out on NULL idle pointers */
    ASSERT(IdlePointer);

    /* This device is about to get busy so increment the busy count by one */
    Dope = CONTAINING_RECORD(IdlePointer, DEVICE_OBJECT_POWER_EXTENSION, IdleCount);
    InterlockedIncrement((LONG volatile *)&Dope->BusyCount);
}

/**
 * @brief
 * Marks the device whom it's been registered for idle detection
 * as busy. It adds one reference to the busy counter.
 *
 * @param[in,out] IdlePointer
 * A pointer to the address of the idle counter of the device
 * that's registered for idle detection.
 *
 * @remarks
 * This function increments the busy count by offsetting right through
 * the member field. The function assumes the integrity of the pointer
 * address therefore it's the caller's responsibility to not trash it.
 *
 * Unlike PoSetDeviceBusy and its Ex variant, this function adds a
 * reference by one, which is permanent, unless explicitly dereferenced
 * with a call to PoEndDeviceBusy. Every single reference incremented
 * must be decremented with each subsequent call to PoEndDeviceBusy.
 */
VOID
NTAPI
PoStartDeviceBusy(
    _Inout_ PULONG IdlePointer)
{
    PDEVICE_OBJECT_POWER_EXTENSION Dope;

    /* Bail out on NULL idle pointers */
    ASSERT(IdlePointer);

    /* This device is about to get busy so keep an active reference */
    Dope = CONTAINING_RECORD(IdlePointer, DEVICE_OBJECT_POWER_EXTENSION, IdleCount);
    InterlockedIncrement((LONG volatile *)&Dope->BusyReference);
}

/**
 * @brief
 * Decrements the busy counter by one done with a call to
 * PoStartDeviceBusy.
 *
 * @param[in,out] IdlePointer
 * A pointer to the address of the idle counter of the device
 * that's registered for idle detection.
 */
VOID
NTAPI
PoEndDeviceBusy(
    _Inout_ PULONG IdlePointer)
{
    PDEVICE_OBJECT_POWER_EXTENSION Dope;

    /* Bail out on NULL idle pointers */
    ASSERT(IdlePointer);

    /* This device is no longer busy, take a reference away */
    Dope = CONTAINING_RECORD(IdlePointer, DEVICE_OBJECT_POWER_EXTENSION, IdleCount);
    InterlockedDecrement((LONG volatile *)&Dope->BusyReference);
}

/**
 * @brief
 * Queries the watchdog time of a power IRP that is currently
 * walking down the device stack.
 *
 * @param[in] Pdo
 * A pointer to a physical device object of which it has issued
 * power IRPs.
 *
 * @param[out] SecondsRemaining
 * A pointer the watchdog time left, in seconds, returned to the
 * caller. The function returns 0 if this device had never issued
 * a power IRP or the IRP watchdog is currently not enabled yet.
 *
 * @return
 * Returns TRUE if this device object has an outstanding power IRP
 * and its watchdog is currently active. FALSE otherwise.
 */
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

/**
 * @brief
 * Sets a power request of the specified type. This makes the
 * Power Manager override its power policy settings.
 *
 * @param[in] PowerRequest
 * A pointer to a power request object that's been created by
 * a call to PoCreatePowerRequest.
 *
 * @param[in] Type
 * The type of power request to be set.
 *
 * @return
 * Returns STATUS_SUCCESS if the power request of the specified
 * type has been set. A failure NTSTATUS code is returned otherwise.
 */
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

/**
 * @brief
 * Deletes a power request object that's been previously
 * created by PoCreatePowerRequest.
 *
 * @param[in,out] PowerRequest
 * A pointer to a power request object that's been created by
 * a call to PoCreatePowerRequest.
 */
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

/**
 * @brief
 * Creates a power request object. A power request is an inquiry
 * by the caller of which it tells the Power Mananger that certain
 * activity is occurring at this moment which prevents the system
 * from undergoing power state transitions, such as, going to sleep.
 *
 * @param[out] PowerRequest
 * A pointer to a power request object that's allocated for
 * the caller.
 *
 * @param[in] DeviceObject
 * A pointer to a device object of which it requests such
 * an object.
 *
 * @param[in] Context
 * A pointer to a reason context which describes why creating this
 * power request is needed. This parameter is optional.
 *
 * @return
 * Returns STATUS_SUCCESS if the power request object was created
 * successfully. STATUS_INSUFFICIENT_RESOURCES is returned if
 * memory allocation for such an object has failed.
 */
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

/**
 * @brief
 * Clears a power request of the specified type. This makes
 * the Power Manager return to the previous power policy
 * settings.
 *
 * @param[in,out] PowerRequest
 * A pointer to a power request object that's been created by
 * a call to PoCreatePowerRequest.
 *
 * @param[in] Type
 * The type of power request to be cleared.
 *
 * @return
 * Returns STATUS_SUCCESS if the power request of the specified
 * type has been cleared. Otherwise a failure NTSTATUS code
 * is returned.
 */
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

/**
 * @brief
 * Creates a thermal request object. The Power Manager will inquire
 * the ACPI driver component to cool down the target device object
 * when needed.
 *
 * @param[out] ThermalRequest
 * A pointer to a thermal request object, returned to the caller.
 * The caller is responsible to delete the said object once it's
 * no longer needed.
 *
 * @param[in] TargetDeviceObject
 * A pointer to the target device object of which it has to
 * be cooled, actively or passively.
 *
 * @param[in] PolicyDeviceObject
 * A pointer to the power policy device object (PPO) that is
 * requesting thermal operation against the target device.
 *
 * @param[in] Context
 * A pointer to a reason context which describes why the thermal
 * request has to be created.
 *
 * @param[in] Flags
 * A flag bit, provided by the caller. The flags for this parameter
 * are currently to be investigated.
 *
 * @return
 * Returns STATUS_SUCCESS if the thermal request object has been
 * created successfully, otherwise a failure NSTATUS code is
 * returned.
 */
NTSTATUS
NTAPI
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

/**
 * @brief
 * Deletes a thermal request object, that was previously created
 * by a call to PoCreateThermalRequest.
 *
 * @param[in,out] ThermalRequest
 * A pointer to a thermal request object to be deleted.
 */
VOID
NTAPI
PoDeleteThermalRequest(
    _Inout_ PVOID ThermalRequest)
{
    /* FIXME */
    UNIMPLEMENTED;
    NOTHING;
}

/**
 * @brief
 * Updates the throttle limit level of the passive cooling of a
 * thermal request object.
 *
 * @param[in,out] ThermalRequest
 * A pointer to a thermal request object of which passive cooling
 * constaint is to be updated.
 *
 * @param[in] Throttle
 * The new passive cooling throttle limit level to be set for this
 * thermal request.
 */
NTSTATUS
NTAPI
PoSetThermalPassiveCooling(
  _Inout_ PVOID ThermalRequest,
  _In_ UCHAR Throttle)
{
    /* FIXME */
    UNIMPLEMENTED;
    UNREFERENCED_PARAMETER(ThermalRequest);
    return STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief
 * Sets the active cooling engagement or disables it on behalf
 * of the caller that owns the thermal request.
 *
 * @param[in,out] ThermalRequest
 * A pointer to a thermal request object of which active cooling
 * is to be enforced or disabled.
 *
 * @param[in] Engaged
 * If set to TRUE, the active cooling will be engaged. Set it to
 * FALSE otherwise.
 */
NTSTATUS
NTAPI
PoSetThermalActiveCooling(
  _Inout_ PVOID ThermalRequest,
  _In_ BOOLEAN Engaged)
{
    /* FIXME */
    UNIMPLEMENTED;
    UNREFERENCED_PARAMETER(ThermalRequest);
    return STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief
 * Determines whether the thermal request supports the specified
 * thermal cooling interface.
 *
 * @param[in] ThermalRequest
 * A pointer to a thermal request object that is to be determined
 * its cooling capability.
 *
 * @param[in] Type
 * The type of the cooling mechanism interface to check against.
 * The following types are:
 *
 * PoThermalRequestPassive -- The following thermal request supports
 *                            the passive cooling interface.
 *
 * PoThermalRequestActive -- The following thermal request supports
 *                           the active cooling interface.
 */
BOOLEAN
NTAPI
PoGetThermalRequestSupport(
  _In_ PVOID ThermalRequest,
  _In_ PO_THERMAL_REQUEST_TYPE Type)
{
    /* FIXME */
    UNIMPLEMENTED;
    UNREFERENCED_PARAMETER(ThermalRequest);
    return FALSE;
}

/* EOF */
