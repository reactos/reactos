/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
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

static
VOID
PopSystemRequired(VOID)
{
    /* FIXME */
    UNIMPLEMENTED;
    return;
}

static
VOID
PopDisplayRequired(VOID)
{
    /* FIXME */
    UNIMPLEMENTED;
    return;
}

static
VOID
PopUserPresent(VOID)
{
    /* FIXME */
    UNIMPLEMENTED;
    return;
}

static
VOID
PopWaitForAllSystemStateDpcs(
    _In_ PPOP_POWER_STATE_HANDLER_COMMAND_CONTEXT StateCommandContext,
    _In_ ULONG ProcessorsCount)
{
    /* Keep waiting until every processor has their DPC ready */
    for (;;)
    {
        if (StateCommandContext->DpcReadyForProcess == ProcessorsCount)
        {
            break;
        }
    }
}

static
VOID
PopHandlePshc(
    _In_ PPOP_POWER_STATE_HANDLER_COMMAND_CONTEXT StateCommandContext,
    _Inout_ PPOP_POWER_STATE_HANDLER_PROCESSOR_CONTEXT ProcessorContext)
{
    NTSTATUS Status, FpStatus;
    KFLOATING_SAVE FloatingSave;
    POP_POWER_HANDLER_COMMAND Command;

    /* Wait until the boot processor instructs a new command */
    for (;;)
    {
        if (StateCommandContext->ExecutingCommand != ProcessorContext->CurrentCommand)
        {
            break;
        }

        YieldProcessor();
    }

    /* Process the following command */
    Command = StateCommandContext->ExecutingCommand;
    switch (Command)
    {
        case SaveFloatingPointContext:
        {
            /* Save the floating point context of this processor */
            FpStatus = KeSaveFloatingPointState(&FloatingSave);
            if (NT_SUCCESS(FpStatus))
            {
                ProcessorContext->FpContext = FloatingSave;
                ProcessorContext->FpStatus = FpStatus;
            }

            break;
        }

        case InvokePowerStateHandler:
        {
            /* Invoke the state handler on this processor */
            Status = StateCommandContext->StateHandler->Handler(StateCommandContext->ContextData.StateHandlerData.Context,
                                                                StateCommandContext->ContextData.StateHandlerData.SystemHandler,
                                                                StateCommandContext->ContextData.StateHandlerData.SystemContext,
                                                                StateCommandContext->ContextData.StateHandlerData.NumberProcessors,
                                                                &StateCommandContext->ContextData.StateHandlerData.Number);

            /* Cache the state handler status to the processor context */
            ProcessorContext->Status = Status;
            break;
        }

        case RestoreFloatingPointContext:
        {
            /* Restore the floating point context if we saved it */
            if (NT_SUCCESS(ProcessorContext->FpStatus))
            {
                KeRestoreFloatingPointState(&ProcessorContext->FpContext);
            }

            break;
        }

        case QuitDpc:
        {
            /* There is nothing to do here */
            break;
        }

        default:
        {
            /* I do not know any other PSHC commands */
            ASSERT(FALSE);
            break;
        }
    }

    /*
     * This processor has done processing the command, cache it as the last
     * command and acknowledge the boot processor that we have handled it.
     */
    ProcessorContext->CurrentCommand = Command;
    InterlockedIncrementUL(&StateCommandContext->ProcessorHandledCommand);
}

static
VOID
PopSendPshc(
    _In_ POP_POWER_HANDLER_COMMAND Command,
    _In_ ULONG ProcessorsCount,
    _In_ PPOP_POWER_STATE_HANDLER_COMMAND_CONTEXT StateCommandContext,
    _Inout_ PPOP_POWER_STATE_HANDLER_PROCESSOR_CONTEXT ProcessorContext)
{
    /* Setup a new command to the global state command context */
    InterlockedExchangeUL(&StateCommandContext->ExecutingCommand, Command);

    /* Initialize the handled command counter */
    InterlockedExchangeUL(&StateCommandContext->ProcessorHandledCommand, 0);

    /* Handle the power state handler command */
    PopHandlePshc(StateCommandContext, ProcessorContext);

    /* Now wait for every processor to handle this command */
    for (;;)
    {
        if (StateCommandContext->ProcessorHandledCommand == ProcessorsCount)
        {
            break;
        }

        YieldProcessor();
    }
}

_Function_class_(KDEFERRED_ROUTINE)
VOID
NTAPI
PopProcessSystemStateCommand(
    _In_ PKDPC Dpc,
    _In_ PVOID DeferredContext,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2)
{
    POP_POWER_STATE_HANDLER_PROCESSOR_CONTEXT ProcessorContext;
    PPOP_POWER_STATE_HANDLER_COMMAND_CONTEXT StateCommandContext = (PPOP_POWER_STATE_HANDLER_COMMAND_CONTEXT)DeferredContext;

    /* We do not care for these parameters */
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    /* Setup the processor command context that is bound to this processor */
    RtlZeroMemory(&ProcessorContext, sizeof(ProcessorContext));

    /* Is the boot processor initializing DPCs for other processors? */
    if (StateCommandContext->InitializingDpcs)
    {
        /*
         * Tell the boot processor that we are ready on our end by incrementing the
         * count of DPCs that are ready for processing power state handler commands.
         */
        InterlockedIncrementUL(&StateCommandContext->DpcReadyForProcess);
    }

    /*
     * Handle any upcoming power state handler commnad (PSHC) on this processor
     * from the invoking boot processor. Cease the execution when the boot processor
     * told us the system handler invocation is done and no longer progressing.
     */
    for (;;)
    {
        if (ProcessorContext.CurrentCommand == QuitDpc)
        {
            break;
        }

        PopHandlePshc(StateCommandContext, &ProcessorContext);
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

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
    POP_POWER_STATE_HANDLER_PROCESSOR_CONTEXT ProcessorContext;
    POP_POWER_STATE_HANDLER_COMMAND_CONTEXT StateCommandContext;

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
    RtlZeroMemory(&StateCommandContext, sizeof(StateCommandContext));
    StateCommandContext.StateHandler = &PopDefaultPowerStateHandlers[HandlerType];
    if (!StateCommandContext.StateHandler->Handler)
    {
        DPRINT1("No state handler registered for this type (%lu), quitting...\n", HandlerType);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    /*
     * Query the number of active processors of which we must invoke
     * a power state handler command to each of them.
     */
    ProcessorsCount = PopQueryActiveProcessors();

    /* TODO: Grab the hibernation context and do something with it (once I write support for it) */
    StateCommandContext.HiberContext = HiberContext;

    /*
     * Have the function calling thread be running on the boot processor and
     * increase the IRQL to dispatch level. We do this because the boot processor
     * is the one invoking the power state handler command to every other active
     * processor of the system.
     */
    KeSetSystemAffinityThread(1);
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    /*
     * If this is a MP machine we have to invoke the system state
     * handler to every processor.
     */
    if (ProcessorsCount > 1)
    {
        /*
         * Setup a state handler command DPC which we will enqueue it to every
         * existing processor active in the system. This ensures that each
         * processor executes the system state handler separately and handles
         * their own processor context data by themselves.
         */
        StateCommandContext.InitializingDpcs = TRUE;
        KeInitializeDpc(&StateHandlerDpc, PopProcessSystemStateCommand, &StateCommandContext);
        KeSetImportanceDpc(&StateHandlerDpc, HighImportance);

        /*
         * Loop every CPU and enqueue a DPC to each of them. This will make each
         * processor spinning until the boot processor (that is us) invokes a
         * power state handler command to every processor.
         */
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

        /* Wait for all the processor DPCs to be ready */
        PopWaitForAllSystemStateDpcs(&StateCommandContext, ProcessorsCount);

        /* All DPCs ready, we are no longer initializing */
        StateCommandContext.InitializingDpcs = FALSE;
    }

    /* Setup a state handler processor context for the boot processor */
    RtlZeroMemory(&ProcessorContext, sizeof(ProcessorContext));

    /*
     * Now that processors are ready to receive PSHC commands, send a "save floating
     * point state" command to every other processor, including the boot one.
     */
    PopSendPshc(SaveFloatingPointContext,
                ProcessorsCount,
                &StateCommandContext,
                &ProcessorContext);

    /* Check if we have a hibernation context or is the system sleeping/shutting down */
    if (HiberContext)
    {
        DPRINT1("Not implemented yet my fam buddy\n");
        ASSERT(FALSE);
    }
    else
    {
        /* Setup system state arguments (FIXME: this is not enough though...) */
        StateCommandContext.ContextData.StateHandlerData.NumberProcessors = KeNumberProcessors;
        StateCommandContext.ContextData.StateHandlerData.Number = KeNumberProcessors;

        /*
         * The system is either sleeping or shutting down, invoke the system
         * state handler to every existing active processor.
         */
        PopSendPshc(InvokePowerStateHandler,
                    ProcessorsCount,
                    &StateCommandContext,
                    &ProcessorContext);

        /* Cache the returned status from the system state handler */
        Status = ProcessorContext.Status;
    }

    /* Restore the saved floating point state context */
    PopSendPshc(RestoreFloatingPointContext,
                ProcessorsCount,
                &StateCommandContext,
                &ProcessorContext);

    /* Invoke every processor to quit their DPCs as the operation is done */
    PopSendPshc(QuitDpc,
                ProcessorsCount,
                &StateCommandContext,
                &ProcessorContext);

    KeLowerIrql(OldIrql);
    return Status;
}

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
        return STATUS_INVALID_PARAMETER_1;
    }

    /* We know the type but no state handler given? Bail out! */
    if (!Handler)
    {
        DPRINT1("No power state handler given\n");
        return STATUS_INVALID_PARAMETER_3;
    }

    /* Register the system state handler with the Power Manager now */
    PopDefaultPowerStateHandlers[Type].Type = Type;
    PopDefaultPowerStateHandlers[Type].RtcWake = RtcWake;
    PopDefaultPowerStateHandlers[Type].Handler = Handler;
    PopDefaultPowerStateHandlers[Type].Context = Context;

    return STATUS_SUCCESS;
}

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

/* EOF */
