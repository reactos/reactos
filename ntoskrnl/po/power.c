/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/po/power.c
 * PURPOSE:         Power Manager
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                  Hervé Poussineau (hpoussin@reactos.com)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

typedef struct _REQUEST_POWER_ITEM
{
    PREQUEST_POWER_COMPLETE CompletionRoutine;
    POWER_STATE PowerState;
    PVOID Context;
    PDEVICE_OBJECT TopDeviceObject;
} REQUEST_POWER_ITEM, *PREQUEST_POWER_ITEM;

typedef struct _POWER_STATE_TRAVERSE_CONTEXT
{
    SYSTEM_POWER_STATE SystemPowerState;
    POWER_ACTION PowerAction;
    PDEVICE_OBJECT PowerDevice;
} POWER_STATE_TRAVERSE_CONTEXT, *PPOWER_STATE_TRAVERSE_CONTEXT;

PDEVICE_NODE PopSystemPowerDeviceNode = NULL;
BOOLEAN PopAcpiPresent = FALSE;
POP_POWER_ACTION PopAction;
WORK_QUEUE_ITEM PopShutdownWorkItem;

/* PRIVATE FUNCTIONS *********************************************************/

static
NTSTATUS
NTAPI
PopRequestPowerIrpCompletion(IN PDEVICE_OBJECT DeviceObject,
                             IN PIRP Irp,
                             IN PVOID Context)
{
    PIO_STACK_LOCATION Stack;
    PREQUEST_POWER_ITEM RequestPowerItem;

    Stack = IoGetNextIrpStackLocation(Irp);
    RequestPowerItem = (PREQUEST_POWER_ITEM)Context;

    RequestPowerItem->CompletionRoutine(DeviceObject,
                                        Stack->MinorFunction,
                                        RequestPowerItem->PowerState,
                                        RequestPowerItem->Context,
                                        &Irp->IoStatus);

    IoFreeIrp(Irp);

    ObDereferenceObject(RequestPowerItem->TopDeviceObject);
    ExFreePool(Context);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
NTAPI
PopCleanupPowerState(IN PPOWER_STATE PowerState)
{
    //UNIMPLEMENTED;
}

NTSTATUS
PopSendQuerySystemPowerState(PDEVICE_OBJECT DeviceObject, SYSTEM_POWER_STATE SystemState, POWER_ACTION PowerAction)
{
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION IrpSp;
    PIRP Irp;
    NTSTATUS Status;

    KeInitializeEvent(&Event,
                      NotificationEvent,
                      FALSE);

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_POWER,
                                       DeviceObject,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatusBlock);
    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    IrpSp = IoGetNextIrpStackLocation(Irp);
    IrpSp->MinorFunction = IRP_MN_QUERY_POWER;
    IrpSp->Parameters.Power.Type = SystemPowerState;
    IrpSp->Parameters.Power.State.SystemState = SystemState;
    IrpSp->Parameters.Power.ShutdownType = PowerAction;

    Status = PoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = IoStatusBlock.Status;
    }

    return Status;
}

NTSTATUS
PopSendSetSystemPowerState(PDEVICE_OBJECT DeviceObject, SYSTEM_POWER_STATE SystemState, POWER_ACTION PowerAction)
{
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION IrpSp;
    PIRP Irp;
    NTSTATUS Status;

    KeInitializeEvent(&Event,
                      NotificationEvent,
                      FALSE);

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_POWER,
                                       DeviceObject,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatusBlock);
    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    IrpSp = IoGetNextIrpStackLocation(Irp);
    IrpSp->MinorFunction = IRP_MN_SET_POWER;
    IrpSp->Parameters.Power.Type = SystemPowerState;
    IrpSp->Parameters.Power.State.SystemState = SystemState;
    IrpSp->Parameters.Power.ShutdownType = PowerAction;

    Status = PoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = IoStatusBlock.Status;
    }

    return Status;
}

NTSTATUS
PopQuerySystemPowerStateTraverse(PDEVICE_NODE DeviceNode,
                                 PVOID Context)
{
    PPOWER_STATE_TRAVERSE_CONTEXT PowerStateContext = Context;
    NTSTATUS Status;

    DPRINT("PopQuerySystemPowerStateTraverse(%p, %p)\n", DeviceNode, Context);

    if (DeviceNode == IopRootDeviceNode)
        return STATUS_SUCCESS;

    if (DeviceNode->Flags & DNF_LEGACY_DRIVER)
        return STATUS_SUCCESS;

    Status = PopSendQuerySystemPowerState(DeviceNode->PhysicalDeviceObject,
                                          PowerStateContext->SystemPowerState,
                                          PowerStateContext->PowerAction);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Device '%wZ' failed IRP_MN_QUERY_POWER\n", &DeviceNode->InstancePath);
    }

#if 0
    return Status;
#else
    return STATUS_SUCCESS;
#endif
}

NTSTATUS
PopSetSystemPowerStateTraverse(PDEVICE_NODE DeviceNode,
                               PVOID Context)
{
    PPOWER_STATE_TRAVERSE_CONTEXT PowerStateContext = Context;
    NTSTATUS Status;

    DPRINT("PopSetSystemPowerStateTraverse(%p, %p)\n", DeviceNode, Context);

    if (DeviceNode == IopRootDeviceNode)
        return STATUS_SUCCESS;

    if (DeviceNode->PhysicalDeviceObject == PowerStateContext->PowerDevice)
        return STATUS_SUCCESS;

    if (DeviceNode->Flags & DNF_LEGACY_DRIVER)
        return STATUS_SUCCESS;

    Status = PopSendSetSystemPowerState(DeviceNode->PhysicalDeviceObject,
                                        PowerStateContext->SystemPowerState,
                                        PowerStateContext->PowerAction);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Device '%wZ' failed IRP_MN_SET_POWER\n", &DeviceNode->InstancePath);
    }

#if 0
    return Status;
#else
    return STATUS_SUCCESS;
#endif
}

NTSTATUS
NTAPI
PopSetSystemPowerState(SYSTEM_POWER_STATE PowerState, POWER_ACTION PowerAction)
{
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT Fdo;
    NTSTATUS Status;
    DEVICETREE_TRAVERSE_CONTEXT Context;
    POWER_STATE_TRAVERSE_CONTEXT PowerContext;

    Status = IopGetSystemPowerDeviceObject(&DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("No system power driver available\n");
        Fdo = NULL;
    }
    else
    {
        Fdo = IoGetAttachedDeviceReference(DeviceObject);
        if (Fdo == DeviceObject)
        {
            DPRINT("An FDO was not attached\n");
            return STATUS_UNSUCCESSFUL;
        }
    }

    /* Set up context */
    PowerContext.PowerAction = PowerAction;
    PowerContext.SystemPowerState = PowerState;
    PowerContext.PowerDevice = Fdo;

    /* Query for system power change */
    IopInitDeviceTreeTraverseContext(&Context,
                                     IopRootDeviceNode,
                                     PopQuerySystemPowerStateTraverse,
                                     &PowerContext);

    Status = IopTraverseDeviceTree(&Context);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Query system power state failed; changing state anyway\n");
    }

    /* Set system power change */
    IopInitDeviceTreeTraverseContext(&Context,
                                     IopRootDeviceNode,
                                     PopSetSystemPowerStateTraverse,
                                     &PowerContext);

    IopTraverseDeviceTree(&Context);

    if (!PopAcpiPresent) return STATUS_NOT_IMPLEMENTED;

    if (Fdo != NULL)
    {
        if (PowerAction != PowerActionShutdownReset)
            PopSendSetSystemPowerState(Fdo, PowerState, PowerAction);

        ObDereferenceObject(Fdo);
    }

    return Status;
}

BOOLEAN
NTAPI
INIT_FUNCTION
PoInitSystem(IN ULONG BootPhase)
{
    PVOID NotificationEntry;
    PCHAR CommandLine;
    BOOLEAN ForceAcpiDisable = FALSE;

    /* Check if this is phase 1 init */
    if (BootPhase == 1)
    {
        /* Register power button notification */
        IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                       PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                       (PVOID)&GUID_DEVICE_SYS_BUTTON,
                                       IopRootDeviceNode->
                                       PhysicalDeviceObject->DriverObject,
                                       PopAddRemoveSysCapsCallback,
                                       NULL,
                                       &NotificationEntry);

        /* Register lid notification */
        IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                       PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                       (PVOID)&GUID_DEVICE_LID,
                                       IopRootDeviceNode->
                                       PhysicalDeviceObject->DriverObject,
                                       PopAddRemoveSysCapsCallback,
                                       NULL,
                                       &NotificationEntry);
        return TRUE;
    }

    /* Get the Command Line */
    CommandLine = KeLoaderBlock->LoadOptions;

    /* Upcase it */
    _strupr(CommandLine);

    /* Check for ACPI disable */
    if (strstr(CommandLine, "NOACPI")) ForceAcpiDisable = TRUE;

    if (ForceAcpiDisable)
    {
        /* Set the ACPI State to False if it's been forced that way */
        PopAcpiPresent = FALSE;
    }
    else
    {
        /* Otherwise check if the LoaderBlock has a ACPI Table  */
        PopAcpiPresent = KeLoaderBlock->Extension->AcpiTable != NULL ? TRUE : FALSE;
    }


    /* Initialize volume support */
    InitializeListHead(&PopVolumeDevices);
    KeInitializeGuardedMutex(&PopVolumeLock);

    /* Initialize support for dope */
    KeInitializeSpinLock(&PopDopeGlobalLock);

    /* Initialize support for shutdown waits and work-items */
    PopInitShutdownList();

    return TRUE;
}

VOID
NTAPI
PopPerfIdle(PPROCESSOR_POWER_STATE PowerState)
{
    DPRINT1("PerfIdle function: %p\n", PowerState);
}

VOID
NTAPI
PopPerfIdleDpc(IN PKDPC Dpc,
               IN PVOID DeferredContext,
               IN PVOID SystemArgument1,
               IN PVOID SystemArgument2)
{
    /* Call the Perf Idle function */
    PopPerfIdle(&((PKPRCB)DeferredContext)->PowerState);
}

VOID
FASTCALL
PopIdle0(IN PPROCESSOR_POWER_STATE PowerState)
{
    /* FIXME: Extremly naive implementation */
    HalProcessorIdle();
}

VOID
NTAPI
INIT_FUNCTION
PoInitializePrcb(IN PKPRCB Prcb)
{
    /* Initialize the Power State */
    RtlZeroMemory(&Prcb->PowerState, sizeof(Prcb->PowerState));
    Prcb->PowerState.Idle0KernelTimeLimit = 0xFFFFFFFF;
    Prcb->PowerState.CurrentThrottle = 100;
    Prcb->PowerState.CurrentThrottleIndex = 0;
    Prcb->PowerState.IdleFunction = PopIdle0;

    /* Initialize the Perf DPC and Timer */
    KeInitializeDpc(&Prcb->PowerState.PerfDpc, PopPerfIdleDpc, Prcb);
    KeSetTargetProcessorDpc(&Prcb->PowerState.PerfDpc, Prcb->Number);
    KeInitializeTimerEx(&Prcb->PowerState.PerfTimer, SynchronizationTimer);
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
PoCancelDeviceNotify(IN PVOID NotifyBlock)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
PoRegisterDeviceNotify(OUT PVOID Unknown0,
                       IN ULONG Unknown1,
                       IN ULONG Unknown2,
                       IN ULONG Unknown3,
                       IN PVOID Unknown4,
                       IN PVOID Unknown5)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
NTAPI
PoShutdownBugCheck(IN BOOLEAN LogError,
                   IN ULONG BugCheckCode,
                   IN ULONG_PTR BugCheckParameter1,
                   IN ULONG_PTR BugCheckParameter2,
                   IN ULONG_PTR BugCheckParameter3,
                   IN ULONG_PTR BugCheckParameter4)
{
    DPRINT1("PoShutdownBugCheck called\n");

    /* FIXME: Log error if requested */
    /* FIXME: Initiate a shutdown */

    /* Bugcheck the system */
    KeBugCheckEx(BugCheckCode,
                 BugCheckParameter1,
                 BugCheckParameter2,
                 BugCheckParameter3,
                 BugCheckParameter4);
}

/*
 * @unimplemented
 */
VOID
NTAPI
PoSetHiberRange(IN PVOID HiberContext,
                IN ULONG Flags,
                IN OUT PVOID StartPage,
                IN ULONG Length,
                IN ULONG PageTag)
{
    UNIMPLEMENTED;
    return;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PoCallDriver(IN PDEVICE_OBJECT DeviceObject,
             IN OUT PIRP Irp)
{
    NTSTATUS Status;

    /* Forward to Io -- FIXME! */
    Status = IoCallDriver(DeviceObject, Irp);

    /* Return status */
    return Status;
}

/*
 * @unimplemented
 */
PULONG
NTAPI
PoRegisterDeviceForIdleDetection(IN PDEVICE_OBJECT DeviceObject,
                                 IN ULONG ConservationIdleTime,
                                 IN ULONG PerformanceIdleTime,
                                 IN DEVICE_POWER_STATE State)
{
    UNIMPLEMENTED;
    return NULL;
}

/*
 * @unimplemented
 */
PVOID
NTAPI
PoRegisterSystemState(IN PVOID StateHandle,
                      IN EXECUTION_STATE Flags)
{
    UNIMPLEMENTED;
    return NULL;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PoRequestPowerIrp(IN PDEVICE_OBJECT DeviceObject,
                  IN UCHAR MinorFunction,
                  IN POWER_STATE PowerState,
                  IN PREQUEST_POWER_COMPLETE CompletionFunction,
                  IN PVOID Context,
                  OUT PIRP *pIrp OPTIONAL)
{
    PDEVICE_OBJECT TopDeviceObject;
    PIO_STACK_LOCATION Stack;
    PIRP Irp;
    PREQUEST_POWER_ITEM RequestPowerItem;

    if (MinorFunction != IRP_MN_QUERY_POWER
        && MinorFunction != IRP_MN_SET_POWER
        && MinorFunction != IRP_MN_WAIT_WAKE)
        return STATUS_INVALID_PARAMETER_2;

    RequestPowerItem = ExAllocatePool(NonPagedPool, sizeof(REQUEST_POWER_ITEM));
    if (!RequestPowerItem)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Always call the top of the device stack */
    TopDeviceObject = IoGetAttachedDeviceReference(DeviceObject);

    Irp = IoBuildAsynchronousFsdRequest(IRP_MJ_POWER,
                                        TopDeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        NULL);
    if (!Irp)
    {
        ObDereferenceObject(TopDeviceObject);
        ExFreePool(RequestPowerItem);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* POWER IRPs are always initialized with a status code of
       STATUS_NOT_IMPLEMENTED */
    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    Irp->IoStatus.Information = 0;

    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->MinorFunction = MinorFunction;
    if (MinorFunction == IRP_MN_WAIT_WAKE)
        Stack->Parameters.WaitWake.PowerState = PowerState.SystemState;
    else
    {
        Stack->Parameters.Power.Type = DevicePowerState;
        Stack->Parameters.Power.State = PowerState;
    }

    RequestPowerItem->CompletionRoutine = CompletionFunction;
    RequestPowerItem->PowerState = PowerState;
    RequestPowerItem->Context = Context;
    RequestPowerItem->TopDeviceObject = TopDeviceObject;

    if (pIrp != NULL)
        *pIrp = Irp;

    IoSetCompletionRoutine(Irp, PopRequestPowerIrpCompletion, RequestPowerItem, TRUE, TRUE, TRUE);
    PoCallDriver(TopDeviceObject, Irp);

    /* Always return STATUS_PENDING. The completion routine
     * will call CompletionFunction and complete the Irp.
     */
    return STATUS_PENDING;
}

/*
 * @unimplemented
 */
POWER_STATE
NTAPI
PoSetPowerState(IN PDEVICE_OBJECT DeviceObject,
                IN POWER_STATE_TYPE Type,
                IN POWER_STATE State)
{
    POWER_STATE ps;

    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    ps.SystemState = PowerSystemWorking;  // Fully on
    ps.DeviceState = PowerDeviceD0;       // Fully on

    return ps;
}

/*
 * @unimplemented
 */
VOID
NTAPI
PoSetSystemState(IN EXECUTION_STATE Flags)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
NTAPI
PoStartNextPowerIrp(IN PIRP Irp)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
NTAPI
PoUnregisterSystemState(IN PVOID StateHandle)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtInitiatePowerAction (IN POWER_ACTION SystemAction,
                       IN SYSTEM_POWER_STATE MinSystemState,
                       IN ULONG Flags,
                       IN BOOLEAN Asynchronous)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtPowerInformation(IN POWER_INFORMATION_LEVEL PowerInformationLevel,
                   IN PVOID InputBuffer  OPTIONAL,
                   IN ULONG InputBufferLength,
                   OUT PVOID OutputBuffer  OPTIONAL,
                   IN ULONG OutputBufferLength)
{
    NTSTATUS Status;

    PAGED_CODE();

    DPRINT("NtPowerInformation(PowerInformationLevel 0x%x, InputBuffer 0x%p, "
           "InputBufferLength 0x%x, OutputBuffer 0x%p, OutputBufferLength 0x%x)\n",
           PowerInformationLevel,
           InputBuffer, InputBufferLength,
           OutputBuffer, OutputBufferLength);

    switch (PowerInformationLevel)
    {
        case SystemBatteryState:
        {
            PSYSTEM_BATTERY_STATE BatteryState = (PSYSTEM_BATTERY_STATE)OutputBuffer;

            if (InputBuffer != NULL)
                return STATUS_INVALID_PARAMETER;
            if (OutputBufferLength < sizeof(SYSTEM_BATTERY_STATE))
                return STATUS_BUFFER_TOO_SMALL;

            /* Just zero the struct (and thus set BatteryState->BatteryPresent = FALSE) */
            RtlZeroMemory(BatteryState, sizeof(SYSTEM_BATTERY_STATE));
            BatteryState->EstimatedTime = MAXULONG;

            Status = STATUS_SUCCESS;
            break;
        }

        case SystemPowerCapabilities:
        {
            PSYSTEM_POWER_CAPABILITIES PowerCapabilities = (PSYSTEM_POWER_CAPABILITIES)OutputBuffer;

            if (InputBuffer != NULL)
                return STATUS_INVALID_PARAMETER;
            if (OutputBufferLength < sizeof(SYSTEM_POWER_CAPABILITIES))
                return STATUS_BUFFER_TOO_SMALL;

            /* Just zero the struct (and thus set BatteryState->BatteryPresent = FALSE) */
            RtlZeroMemory(PowerCapabilities, sizeof(SYSTEM_POWER_CAPABILITIES));
            //PowerCapabilities->SystemBatteriesPresent = 0;

            Status = STATUS_SUCCESS;
            break;
        }

        default:
            Status = STATUS_NOT_IMPLEMENTED;
            DPRINT1("PowerInformationLevel 0x%x is UNIMPLEMENTED! Have a nice day.\n",
                    PowerInformationLevel);
            break;
    }

    return Status;
}

NTSTATUS
NTAPI
NtGetDevicePowerState(IN HANDLE Device,
                      IN PDEVICE_POWER_STATE PowerState)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
NTAPI
NtIsSystemResumeAutomatic(VOID)
{
    UNIMPLEMENTED;
    return FALSE;
}

NTSTATUS
NTAPI
NtRequestWakeupLatency(IN LATENCY_TIME Latency)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtSetThreadExecutionState(IN EXECUTION_STATE esFlags,
                          OUT EXECUTION_STATE *PreviousFlags)
{
    PKTHREAD Thread = KeGetCurrentThread();
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    EXECUTION_STATE PreviousState;
    PAGED_CODE();

    /* Validate flags */
    if (esFlags & ~(ES_CONTINUOUS | ES_USER_PRESENT))
    {
        /* Fail the request */
        return STATUS_INVALID_PARAMETER;
    }

    /* Check for user parameters */
    if (PreviousMode != KernelMode)
    {
        /* Protect the probes */
        _SEH2_TRY
        {
            /* Check if the pointer is valid */
            ProbeForWriteUlong(PreviousFlags);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* It isn't -- fail */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Save the previous state, always masking in the continous flag */
    PreviousState = Thread->PowerState | ES_CONTINUOUS;

    /* Check if we need to update the power state */
    if (esFlags & ES_CONTINUOUS) Thread->PowerState = (UCHAR)esFlags;

    /* Protect the write back to user mode */
    _SEH2_TRY
    {
        /* Return the previous flags */
        *PreviousFlags = PreviousState;
    }
    _SEH2_EXCEPT(ExSystemExceptionFilter())
    {
        /* Something's wrong, fail */
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* All is good */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NtSetSystemPowerState(IN POWER_ACTION SystemAction,
                      IN SYSTEM_POWER_STATE MinSystemState,
                      IN ULONG Flags)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    POP_POWER_ACTION Action = {0};
    NTSTATUS Status;
    ULONG Dummy;

    /* Check for invalid parameter combinations */
    if ((MinSystemState >= PowerSystemMaximum) ||
        (MinSystemState <= PowerSystemUnspecified) ||
        (SystemAction > PowerActionWarmEject) ||
        (SystemAction < PowerActionReserved) ||
        (Flags & ~(POWER_ACTION_QUERY_ALLOWED  |
                   POWER_ACTION_UI_ALLOWED     |
                   POWER_ACTION_OVERRIDE_APPS  |
                   POWER_ACTION_LIGHTEST_FIRST |
                   POWER_ACTION_LOCK_CONSOLE   |
                   POWER_ACTION_DISABLE_WAKES  |
                   POWER_ACTION_CRITICAL)))
    {
        DPRINT1("NtSetSystemPowerState: Bad parameters!\n");
        DPRINT1("                       SystemAction: 0x%x\n", SystemAction);
        DPRINT1("                       MinSystemState: 0x%x\n", MinSystemState);
        DPRINT1("                       Flags: 0x%x\n", Flags);
        return STATUS_INVALID_PARAMETER;
    }

    /* Check for user caller */
    if (PreviousMode != KernelMode)
    {
        /* Check for shutdown permission */
        if (!SeSinglePrivilegeCheck(SeShutdownPrivilege, PreviousMode))
        {
            /* Not granted */
            DPRINT1("ERROR: Privilege not held for shutdown\n");
            return STATUS_PRIVILEGE_NOT_HELD;
        }

        /* Do it as a kernel-mode caller for consistency with system state */
        return ZwSetSystemPowerState(SystemAction, MinSystemState, Flags);
    }

    /* Read policy settings (partial shutdown vs. full shutdown) */
    if (SystemAction == PowerActionShutdown) PopReadShutdownPolicy();

    /* Disable lazy flushing of registry */
    DPRINT("Stopping lazy flush\n");
    CmSetLazyFlushState(FALSE);

    /* Setup the power action */
    Action.Action = SystemAction;
    Action.Flags = Flags;

    /* Notify callbacks */
    DPRINT("Notifying callbacks\n");
    ExNotifyCallback(PowerStateCallback, (PVOID)3, NULL);

    /* Swap in any worker thread stacks */
    DPRINT("Swapping worker threads\n");
    ExSwapinWorkerThreads(FALSE);

    /* Make our action global */
    PopAction = Action;

    /* Start power loop */
    Status = STATUS_CANCELLED;
    while (TRUE)
    {
        /* Break out if there's nothing to do */
        if (Action.Action == PowerActionNone) break;

        /* Check for first-pass or restart */
        if (Status == STATUS_CANCELLED)
        {
            /* Check for shutdown action */
            if ((PopAction.Action == PowerActionShutdown) ||
                (PopAction.Action == PowerActionShutdownReset) ||
                (PopAction.Action == PowerActionShutdownOff))
            {
                /* Set the action */
                PopAction.Shutdown = TRUE;
            }

            /* Now we are good to go */
            Status = STATUS_SUCCESS;
        }

        /* Check if we're still in an invalid status */
        if (!NT_SUCCESS(Status)) break;

#ifndef NEWCC
        /* Flush dirty cache pages */
        CcRosFlushDirtyPages(-1, &Dummy, FALSE); //HACK: We really should wait here!
#else
        Dummy = 0;
#endif

        /* Flush all volumes and the registry */
        DPRINT("Flushing volumes, cache flushed %lu pages\n", Dummy);
        PopFlushVolumes(PopAction.Shutdown);

        /* Set IRP for drivers */
        PopAction.IrpMinor = IRP_MN_SET_POWER;
        if (PopAction.Shutdown)
        {
            DPRINT("Queueing shutdown thread\n");
            /* Check if we are running in the system context */
            if (PsGetCurrentProcess() != PsInitialSystemProcess)
            {
                /* We're not, so use a worker thread for shutdown */
                ExInitializeWorkItem(&PopShutdownWorkItem,
                                     &PopGracefulShutdown,
                                     NULL);

                ExQueueWorkItem(&PopShutdownWorkItem, CriticalWorkQueue);

                /* Spend us -- when we wake up, the system is good to go down */
                KeSuspendThread(KeGetCurrentThread());
                Status = STATUS_SYSTEM_SHUTDOWN;
                goto Exit;

            }
            else
            {
                /* Do the shutdown inline */
                PopGracefulShutdown(NULL);
            }
        }

        /* You should not have made it this far */
        ASSERTMSG("System is still up and running?!", FALSE);
        break;
    }

Exit:
    /* We're done, return */
    return Status;
}
