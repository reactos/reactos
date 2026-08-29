/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Power states and Idle management for System and Devices infrastructure
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

KDPC PopIdleScanDevicesDpc;
KTIMER PopIdleScanDevicesTimer;
LIST_ENTRY PopIdleDetectList;
ULONG PopIdleScanIntervalInSeconds = 1;
BOOLEAN PopResumeAutomatic = FALSE;
POWER_STATE_HANDLER PopDefaultPowerStateHandlers[PowerStateMaximum] = {0};

/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @brief
 * Restores the system from the idle state into active state by resetting
 * the system idle counter.
 */
static
VOID
PopSystemRequired(VOID)
{
    /* FIXME */
    UNIMPLEMENTED;
    return;
}

/**
 * @brief
 * Restores the display from the idle state by invoking a power callout
 * to Win32k to make the display busy. The display idle counter is reset.
 * If the display was diming, Win32k passes this request further down
 * to Videoprt to un-dim the display and whatever is needed.
 */
static
VOID
PopDisplayRequired(VOID)
{
    /* FIXME */
    UNIMPLEMENTED;
    return;
}

/**
 * @brief
 * Marks the system as having a user that interacts with the machine physically.
 * This is usually invoked when waking up from sleep or from staying idle for too long.
 * This function invokes a power callout to Win32k to indicate the presence of a user.
 */
static
VOID
PopUserPresent(VOID)
{
    /* FIXME */
    UNIMPLEMENTED;
    return;
}

/**
 * @brief
 * The main core DPC routine used to invoke the power state handler to the
 * target logical processor. CPU context is saved here as well as FPU state.
 */
_Function_class_(KDEFERRED_ROUTINE)
VOID
NTAPI
PopStateHandlerProcessorDpc(
    _In_ PKDPC Dpc,
    _In_ PVOID DeferredContext,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2)
{
    LONG CpuNumber = KeGetCurrentPrcb()->Number;
    PPOWER_STATE_HANDLER StateHandler = (PPOWER_STATE_HANDLER)DeferredContext;

    /* We do not care for these parameters */
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    /* Invoke the state handler to manage this processor */
    StateHandler->Handler(StateHandler->Context,
                          NULL, // A system handler (either hibernation or sleep handler) must be passed from the deferred context
                          NULL, // A system context must be passed from the deferred context (if hibernate or sleep is invoked)
                          KeNumberProcessors,
                          &CpuNumber);
}

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Invokes the state handler onto the boot processor and every other logical
 * processor. The state handler is responsible to power down the target processor.
 *
 * @param[in] HandlerType
 * A type to the power state handler to be used when performing a power
 * action against the target processor.
 *
 * @param[in] HiberContext
 * A pointer to a hibernation context, passed when the system is incurring
 * in a hibernation phase (and the system actually supports it).
 *
 * @return
 * Returns STATUS_SUCCESS if the state handler was invoked successfully.
 * STATUS_INVALID_PARAMETER is returned if an invalid handler type was given.
 * STATUS_DEVICE_DOES_NOT_EXIST is returned if the following handler doesn't exist.
 *
 * @remarks
 * This function is currently not fully implemented yet. See the big FIXME below.
 */
NTSTATUS
NTAPI
PopInvokeSystemStateHandler(
    _In_ POWER_STATE_HANDLER_TYPE HandlerType,
    _In_opt_ PPOP_HIBER_CONTEXT HiberContext)
{
    NTSTATUS Status;
    KIRQL OldIrql;
    KDPC StateHandlerDpc;
    ULONG ProcessorsCount, ProcessorIndex;
    PPOWER_STATE_HANDLER StateHandler;
    LONG CpuNumber = KeGetCurrentPrcb()->Number;

    /* FIXME: Hibernation support is currently not implemented yet */
    UNREFERENCED_PARAMETER(HiberContext);

    /* Nobody else must be raising the IRQL with this function called */
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    /* The caller submitted a bogus handler type, bail out */
    if (HandlerType < PowerStateSleeping1 || HandlerType > PowerStateShutdownOff)
    {
        DPRINT1("Unknown state type handler (%lu), quitting...\n", HandlerType);
        return STATUS_INVALID_PARAMETER;
    }

    /*
     * Cache the state handler and check if it has been registered
     * with the Power Manager, otherwise do not bother.
     */
    StateHandler = &PopDefaultPowerStateHandlers[HandlerType];
    if (!PopDefaultPowerStateHandlers[HandlerType].Handler)
    {
        DPRINT1("No state handler registered for this type (%lu), quitting...\n", HandlerType);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    /*
     * Query the number of active processors of which we must invoke
     * a power state handler command to each of them.
     */
    ProcessorsCount = PopQueryActiveProcessors();

    /*
     * Have the function calling thread be running on the boot processor and
     * increase the IRQL to dispatch level. We do this because the boot processor
     * is the one invoking issuing state handler DPC to every other processor of
     * the system.
     */
    KeSetSystemAffinityThread(1);
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    /*
     * If this is a MP machine we have to invoke the system state
     * handler to every processor.
     *
     * FIXME: The implementation is currently not enough. While deploying DPCs to
     * each processor and execute the state handler to them is the core purpose of
     * the design of this function, there's more to do than this. That is.......
     *
     * 1. (HiberContext != NULL) if a hibernation context has been passed, this very much
     * means that the system is entering into a deep sleep state, S4. CPU registers are flushed,
     * RAM is powered down, only the medium storage is alive. In this scenario the hibernation
     * context must be cached and this function must perform hibernation related stuff, such as
     * collecting memory pages that are marked within the hibernation bounds range and prepare
     * the hibernation image file. In this scenario the hibernation system handler alongside with
     * its argument context is further passed down to the state handler. This way the HAL understands
     * the system is undertaking hibernation and it must do very specific hibernation stuff.
     *
     * 2. Every processor must cache their CPU context locally and restore them when waking
     * from sleep state.
     *
     * 3. The FPU state must be saved with KeSaveFloatingPointState when entering into sleep
     * state and restored with KeRestoreFloatingPointState when waking up. AMD64 doesn't care for this.
     *
     * 4. Machine and kernel features must be saved and restored.
     *
     * All of the 4 points above MUST BE RELIGIOUSLY RESPECTED if the system enters into either of the
     * system states (S1, S2, S3 and S4). For AoAc capable systems, the ACPI HAL can register a special AoAc
     * state handler for PowerStateShutdownOff as such systems by definition might not support any of the
     * system states mentioned.
     */
    if (ProcessorsCount > 1)
    {
        /*
         * Setup a state handler command DPC which we will enqueue it to every
         * existing processor active in the system. This ensures that each
         * processor executes the system state handler separately and handles
         * their own processor context data by themselves.
         */
        KeInitializeDpc(&StateHandlerDpc, PopStateHandlerProcessorDpc, StateHandler);
        KeSetImportanceDpc(&StateHandlerDpc, HighImportance);

        for (ProcessorIndex = 0;
             ProcessorIndex < ProcessorsCount;
             ProcessorIndex++)
        {
            /*
             * Of course make sure that we do not insert the DPC for the boot
             * processor, thereby hurting ourselves by spinning it forever.
             */
            if (ProcessorIndex != KeGetCurrentPrcb()->Number)
            {
                /* Assign the DPC to the target processor */
                KeSetTargetProcessorDpc(&StateHandlerDpc, ProcessorIndex);

                /* Insert the DPC to the target processor in queue */
                KeInsertQueueDpc(&StateHandlerDpc, NULL, NULL);
            }
        }
    }

    /* Invoke the state handler to manage the boot processor */
    Status = StateHandler->Handler(StateHandler->Context,
                                   NULL, // If HiberContext != NULL the system handler must be passed that from hibernation context (see FIXME above)
                                   NULL, // If HiberContext != NULL the system context must be passed that from hibernation context (see FIXME above)
                                   KeNumberProcessors,
                                   &CpuNumber);

    KeLowerIrql(OldIrql);
    return Status;
}

/**
 * @brief
 * The main core DPC routine that scans for devices that are idling. Idling devices
 * that reach a certain idle trip will be put in a different power state.
 *
 * @remarks
 * This routine follows two rules on how to determine the idleness of a device, as
 * the driver who owns the said device can use several power idle APIs for this.
 * The driver owner is responsible to wake up its own device that succumbed to
 * sleep, this routine won't do it for you.
 */
_Function_class_(KDEFERRED_ROUTINE)
VOID
NTAPI
PopScanForIdleStateDevicesDpcRoutine(
    _In_ PKDPC Dpc,
    _In_ PVOID DeferredContext,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2)
{
    NTSTATUS Status;
    KIRQL OldIrql;
    PLIST_ENTRY Entry;
    POWER_STATE State;
    PDEVICE_OBJECT DeviceObject;
    POP_DEVICE_IDLE_TYPE IdleType;
    ULONG IdleTreshold, NewIdleCount;
    PDEVICE_OBJECT_POWER_EXTENSION Dope;

    /* We do not care for these parameters */
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    /* Begin iterating over the global idle detect list for any registered devices */
    PopAcquireDopeLock(&OldIrql);
    for (Entry = PopIdleDetectList.Flink;
         Entry != &PopIdleDetectList;
         Entry = Entry->Flink)
    {
        /* Capture the DOPE of this device */
        Dope = CONTAINING_RECORD(Entry, DEVICE_OBJECT_POWER_EXTENSION, IdleList);
        ASSERT(Dope != NULL);

        /* Cache the device object */
        DeviceObject = Dope->DeviceObject;

        /*
         * FIRST RULE of the thumb: look for active busy references the caller
         * is still holding onto this device. A reference above 0 means the caller
         * has an active outstanding instance call of PoStartDeviceBusy, and this
         * could go for as much as the caller wishes, until it explicitly tells the
         * power manager that the device stopped being busy. We do not touch the busy
         * count here.
         */
        if (Dope->BusyReference > 0)
        {
            /*
             * The act of disabling the idle counter as per MSDN documentation actually means
             * resetting the said counter back to 0 if the device used to be idling before.
             */
            if (Dope->IdleCount > 0)
            {
                Dope->IdleCount = 0;
            }

            DPRINT("The device object (0x%p) with DOPE (0x%p) is busy", DeviceObject, Dope);
            continue;
        }

        /*
         * SECOND RULE of the thumb: this device does not have active busy references
         * but it is being held busy for a brief period of time. The function which is
         * responsible for that is PoSetDeviceBusyEx.
         */
        if (Dope->BusyCount > 0)
        {
            /*
             * As this device was beind held for a short period of time, now it is
             * time to decrease the busy count by one. The caller is responsible to
             * keep it busy with multiple PoSetDeviceBusyEx requests.
             */
            Dope->BusyCount--;

            /*
             * If this device used to be idling before at the time of declaring itself
             * as busy, reset its idle counter.
             */
            if (Dope->IdleCount > 0)
            {
                Dope->IdleCount = 0;
            }

            DPRINT("The device object (0x%p) with DOPE (0x%p) is busy for a brief period of time", DeviceObject, Dope);
            continue;
        }

        /* This device is not busy, increment the idle counter */
        NewIdleCount = InterlockedIncrementUL(&Dope->IdleCount);

        /* Obtain the idle time treshold based on the device type */
        IdleType = Dope->IdleType;
        if (IdleType == DeviceIdleNormal)
        {
            /*
             * Grab the treshold from the registered conservation or performance
             * idle time, depending on the power policy the power manager has
             * currently enforced.
             */
            if (PopDefaultPowerPolicy == &PopDcPowerPolicy)
            {
                /* The system runs on batteries, so favor the conservation idle time */
                IdleTreshold = Dope->ConservationIdleTime;
            }
            else
            {
                /*
                 * The system runs on AC power (typically from PSU or its batteries
                 * are charging), favor the performance idle time.
                 */
                IdleTreshold = Dope->PerformanceIdleTime;
            }
        }
        else if (IdleType == DeviceIdleDisk)
        {
            /*
             * This device is a disk or mass storage device, grab the treshold from
             * the default spin-down idle time of the currently enforced power policy.
             */
            IdleTreshold = PopDefaultPowerPolicy->SpindownTimeout;
        }
        else
        {
            /* The Power Manager does not know of this device, bail out */
            DPRINT1("The device (0x%p) with DOPE (0x%p) is of an unknown type (%lu), crash is imminent\n",
                    DeviceObject, Dope, IdleType);
            KeBugCheckEx(INTERNAL_POWER_ERROR,
                         0x200,
                         POP_IDLE_DETECT_UNKNOWN_DEVICE,
                         (ULONG_PTR)DeviceObject,
                         (ULONG_PTR)Dope);
        }

        /* Send a power IRP to this device if it is idling for long enough */
        if (IdleTreshold && (NewIdleCount == IdleTreshold))
        {
            State.DeviceState = Dope->IdleState;
            Status = PopRequestPowerIrp(DeviceObject,
                                        IRP_MN_SET_POWER,
                                        State,
                                        FALSE,
                                        FALSE,
                                        NULL,
                                        NULL,
                                        NULL);
            NT_ASSERT(Status == STATUS_PENDING);
            Dope->CurrentState = Dope->IdleState;
        }
    }

    PopReleaseDopeLock(OldIrql);
}

/**
 * @brief
 * Retrieves the power flags of a device or system from the extended
 * device object extensions.
 *
 * @param[in] DevObjExts
 * A pointer to an extended device object extensions.
 *
 * @param[in] GetSystem
 * If set to TRUE, this function will retrieve the system power state
 * from the DOE. Set it to FALSE if to retrieve the device power state instead.
 *
 * @return
 * Returns the power flags from the target DOE.
 */
ULONG
NTAPI
PopGetDoePowerState(
    _In_ PEXTENDED_DEVOBJ_EXTENSION DevObjExts,
    _In_ BOOLEAN GetSystem)
{
    ULONG PowerFlags;

    if (GetSystem)
    {
        PowerFlags = (DevObjExts->PowerFlags & POP_DOE_SYSTEM_POWER_FLAG_BIT);
    }
    else
    {
        PowerFlags = ((DevObjExts->PowerFlags & POP_DOE_DEVICE_POWER_FLAG_BIT) >> 4);
    }

    return PowerFlags;
}

/**
 * @brief
 * Sets new DOE power state flags.
 *
 * @param[in] DevObjExts
 * A pointer to an extended device object extensions of which new
 * power state flags are to be set.
 *
 * @param[in] NewState
 * The new power state, provided by the caller.
 *
 * @param[in] SetSystem
 * If set to TRUE, this function will set new system state power flags.
 */
VOID
NTAPI
PopSetDoePowerState(
    _In_ PEXTENDED_DEVOBJ_EXTENSION DevObjExts,
    _In_ POWER_STATE NewState,
    _In_ BOOLEAN SetSystem)
{
    SYSTEM_POWER_STATE SystemState;
    DEVICE_POWER_STATE DeviceState;

    if (SetSystem)
    {
        SystemState = NewState.SystemState;
        DevObjExts->PowerFlags |= SystemState & POP_DOE_SYSTEM_POWER_FLAG_BIT;
    }
    else
    {
        DeviceState = NewState.DeviceState;
        DevObjExts->PowerFlags |= ((DeviceState << 4) & POP_DOE_DEVICE_POWER_FLAG_BIT);
    }
}

/**
 * @brief
 * Registers a new power state handler used by the Power Manager whenever
 * the system undergoes sleeping, hibernating or whatever power down
 * procedure it may be.
 *
 * @param[in] Type
 * The type of the power state handler to be registered.
 *
 * @param[in] RtcWake
 * If set to TRUE, this handler will allow real-time clock wakes.
 * Set it to FALSE if RTC wakes are not to be allowed with this handler.
 *
 * @param[in] Handler
 * A pointer to a power state handler used for registration.
 *
 * @param[in] Context
 * A pointer to an argument context to be passed to the state handler.
 * This parameter is optional.
 *
 * @return
 * STATUS_SUCCESS is returned if the power state handler has been fully
 * registered with the Power Manager. STATUS_INVALID_PARAMETER is returned
 * if at least one of the parameters is not valid.
 */
NTSTATUS
NTAPI
PopRegisterSystemStateHandler(
    _In_ POWER_STATE_HANDLER_TYPE Type,
    _In_ BOOLEAN RtcWake,
    _In_ PENTER_STATE_HANDLER Handler,
    _In_opt_ PVOID Context)
{
    PAGED_CODE();

    /* Caller was trying to give an invalid handler type, bail out */
    if (Type < PowerStateSleeping1 || Type >= PowerStateMaximum)
    {
        DPRINT1("Invalid power state handler type was given (Type %d)\n", Type);
        return STATUS_INVALID_PARAMETER;
    }

    /* We know the type but no state handler given? Bail out! */
    if (!Handler)
    {
        DPRINT1("No power state handler given\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Register the system state handler with the Power Manager now */
    PopDefaultPowerStateHandlers[Type].Type = Type;
    PopDefaultPowerStateHandlers[Type].RtcWake = RtcWake;
    PopDefaultPowerStateHandlers[Type].Handler = Handler;
    PopDefaultPowerStateHandlers[Type].Context = Context;

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Indicates the presence of an activity currently occurring in the system,
 * thereby making it busy. This will prevent the Power Manager from taking
 * actions such as diming the display, putting the system on sleep, reduce
 * power consumption, etc etc.
 *
 * @param[in] StateActivity
 * The type of execution state the system is currently carrying out.
 */
VOID
NTAPI
PopIndicateSystemStateActivity(
    _In_ EXECUTION_STATE StateActivity)
{
    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /*
     * We got acknowledgement from the caller that the system is currently busy,
     * handle this on an execution state basis. For system required scenario we
     * will reset the system idle counter.
     */
    if (StateActivity & ES_SYSTEM_REQUIRED)
    {
        PopSystemRequired();
    }

    /*
     * For display required we must tell GDI to not dim the display
     * by resetting the display idle counter.
     */
    if (StateActivity & ES_DISPLAY_REQUIRED)
    {
        PopDisplayRequired();
    }

    /*
     * Tell GDI the physical presence of a user, wake the system and
     * reset the system idle counter.
     */
    if (StateActivity & ES_USER_PRESENT)
    {
        PopUserPresent();
    }
}

/**
 * @brief
 * Changes the system power state capabilities upon the arrival or dissmisal of
 * a power state handler by the HAL.
 *
 * @param[in] StateHandler
 * A pointer to a state handler of the specified type of which the routine
 * will determine the system power state capability to be changed.
 *
 * @param[in] Enable
 * If set to TRUE, the following system power state capability will be enabled,
 * indicating that the system supports it. If set to FALSE, the system has
 * disabled such a power state capability.
 */
VOID
NTAPI
PopChangeSystemSystemStateCapability(
    _In_ PPOWER_STATE_HANDLER StateHandler,
    _In_ BOOLEAN Enable)
{
    POWER_STATE_HANDLER_TYPE HandlerType;

    PAGED_CODE();

    /* Cache the handler type and enable/disable the system capability */
    HandlerType = StateHandler->Type;
    switch (HandlerType)
    {
        case PowerStateSleeping1:
        {
            PopCapabilities.SystemS1 = !Enable;
            break;
        }

        case PowerStateSleeping2:
        {
            PopCapabilities.SystemS2 = !Enable;
            break;
        }

        case PowerStateSleeping3:
        {
            PopCapabilities.SystemS3 = !Enable;
            break;
        }

        case PowerStateSleeping4:
        {
            PopCapabilities.SystemS4 = !Enable;
            break;
        }

        case PowerStateShutdownOff:
        {
            PopCapabilities.SystemS5 = !Enable;
            break;
        }

        default:
            break;
    }
}

/* EOF */
