/*
 * PROJECT:     ReactOS Drivers
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/sac/driver/data.c
 * PURPOSE:     Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "sacdrv.h"

/* GLOBALS ********************************************************************/

ULONG SACDebug = 0;
BOOLEAN CommandConsoleLaunchingEnabled;
BOOLEAN GlobalDataInitialized;
KMUTEX SACCMDEventInfoMutex;
BOOLEAN IoctlSubmitted;
ULONG ProcessingType;
PKEVENT SACEvent;
HANDLE SACEventHandle;

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
WorkerProcessEvents(IN PSAC_DEVICE_EXTENSION DeviceExtension)
{
    /* Call the worker function */
    ConMgrWorkerProcessEvents(DeviceExtension);
}

VOID
NTAPI
WorkerThreadStartUp(IN PVOID Context)
{
    /* Call the worker function */
    WorkerProcessEvents((PSAC_DEVICE_EXTENSION)Context);
}

NTSTATUS
NTAPI
BuildDeviceAcl(OUT PACL* Dacl)
{
    /* TODO */
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CreateDeviceSecurityDescriptor(IN PDEVICE_OBJECT *DeviceObject)
{
    NTSTATUS Status;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    BOOLEAN MemoryAllocated = FALSE;
    PACL Dacl = NULL;
    PVOID ObjectSecurityDescriptor = NULL;
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC CreateDeviceSecurityDescriptor: Entering.\n");

    /* Get the current SD of the device object */
    Status = ObGetObjectSecurity(*DeviceObject, &SecurityDescriptor, &MemoryAllocated);
    if (!NT_SUCCESS(Status))
    {
        SAC_DBG(SAC_DBG_INIT, "SAC: Unable to get security descriptor, error: %x\n", Status);
        NT_ASSERT(MemoryAllocated == FALSE);
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC CreateDeviceSecurityDescriptor: Exiting with status 0x%x\n", Status);
        return Status;
    }

    /* Build a DACL for it */
    Status = BuildDeviceAcl(&Dacl);
    if (Status >= 0)
    {
        ASSERT(FALSE);
    }
    else
    {
        SAC_DBG(SAC_DBG_INIT, "SAC CreateDeviceSecurityDescriptor : Unable to create Raw ACL, error : %x\n", Status);
        /* FIXME: Temporary hack */
        Status = STATUS_SUCCESS;
        goto CleanupPath;
    }

CleanupPath:
    /* Release the SD we queried */
    ObReleaseObjectSecurity(SecurityDescriptor, MemoryAllocated);

    /* Free anything else we may have allocated */
    if (ObjectSecurityDescriptor) ExFreePool(ObjectSecurityDescriptor);
    if (Dacl) SacFreePool(Dacl);

    /* All done */
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC CreateDeviceSecurityDescriptor: Exiting with status 0x%x\n", Status);
    return Status;
}

VOID
NTAPI
FreeGlobalData(VOID)
{
    UNICODE_STRING SymbolicLink;
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC FreeGlobalData: Entering.\n");

    /* Only free if we allocated */
    if (GlobalDataInitialized)
    {
        /* Close the SAC event if we had created one */
        if (SACEvent)
        {
            ZwClose(SACEventHandle);
            SACEvent = NULL;
        }

        /* Destroy the cached messages */
        TearDownGlobalMessageTable();

        /* Delete the Win32 symbolic link */
        RtlInitUnicodeString(&SymbolicLink, L"\\DosDevices\\SAC");
        IoDeleteSymbolicLink(&SymbolicLink);

        /* Tear down connections */
        ConMgrShutdown();

        /* Tear down channels */
        ChanMgrShutdown();

        /* Free the serial port buffer */
        if (SerialPortBuffer) SacFreePool(SerialPortBuffer);

        /* Free cached machine information */
        FreeMachineInformation();

        /* Cleanup the custom heap allocator */
        FreeMemoryManagement();

        /* We're back to a virgin state */
        GlobalDataInitialized = FALSE;
    }

    /* All done */
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC FreeGlobalData: Exiting.\n");
}

VOID
NTAPI
FreeDeviceData(IN PDEVICE_OBJECT DeviceObject)
{
    PSAC_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;
    KIRQL OldIrql;
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC FreeDeviceData: Entering.\n");

    /* Get the device extension and see how far we had gotten */
    DeviceExtension = (PSAC_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    if (!(GlobalDataInitialized) || !(DeviceExtension->Initialized))
    {
        goto Exit;
    }

    /* Attempt to rundown while holding the lock */
    KeAcquireSpinLock(&DeviceExtension->Lock, &OldIrql);
    while (DeviceExtension->RundownInProgress)
    {
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC FreeDeviceData: Waiting....\n");

        /* Initiate and wait for rundown */
        KeInitializeEvent(&DeviceExtension->RundownEvent, SynchronizationEvent, 0);
        KeReleaseSpinLock(&DeviceExtension->Lock, OldIrql);
        Status = KeWaitForSingleObject(&DeviceExtension->RundownEvent,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);
        ASSERT(Status == STATUS_SUCCESS);

        /* Re-acquire the lock and check if rundown is done */
        KeAcquireSpinLock(&DeviceExtension->Lock, &OldIrql);
    }

    /* Now set the rundown flag while we cancel the timer */
    DeviceExtension->RundownInProgress = TRUE;
    KeReleaseSpinLock(&DeviceExtension->Lock, OldIrql);

    /* Cancel it */
    KeCancelTimer(&DeviceExtension->Timer);

    /* Reacquire the lock*/
    KeAcquireSpinLock(&DeviceExtension->Lock, &OldIrql);
    DeviceExtension->RundownInProgress = FALSE;

    /* Now do the last rundown attempt, we should be the only ones here */
    KeInitializeEvent(&DeviceExtension->RundownEvent, SynchronizationEvent, 0);
    KeReleaseSpinLock(&DeviceExtension->Lock, OldIrql);
    KeSetEvent(&DeviceExtension->Event, DeviceExtension->PriorityBoost, 0);
    Status = KeWaitForSingleObject(&DeviceExtension->RundownEvent,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL);
    ASSERT(Status == STATUS_SUCCESS);

    /* We no longer care about shutdown */
    IoUnregisterShutdownNotification(DeviceObject);

    /* We are now fully uninitialized */
    KeAcquireSpinLock(&DeviceExtension->Lock, &OldIrql);
    DeviceExtension->Initialized = FALSE;
    KeReleaseSpinLock(&DeviceExtension->Lock, OldIrql);
Exit:
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC FreeDeviceData: Exiting.\n");
}

BOOLEAN
NTAPI
InitializeDeviceData(IN PDEVICE_OBJECT DeviceObject)
{
    PSAC_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    BOOLEAN EnableData;
    ULONG PriorityValue;
    NTSTATUS Status;
    LARGE_INTEGER DueTime;
    PWCHAR Message;
    PAGED_CODE();
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "Entering.\n");

    /* If we already did this, bail out */
    if (DeviceExtension->Initialized) goto SuccessExit;

    /* Setup the DO flags */
    DeviceObject->Flags |= DO_DIRECT_IO;
    DeviceObject->StackSize = 16;

    /* Setup the device extension */
    DeviceExtension->DeviceObject = DeviceObject;
    DeviceExtension->PriorityBoost = IO_SERIAL_INCREMENT;
    DeviceExtension->PriorityFail = 0;
    DeviceExtension->RundownInProgress = 0;

    /* Initialize locks, events, timers, DPCs, etc... */
    KeInitializeTimer(&DeviceExtension->Timer);
    KeInitializeDpc(&DeviceExtension->Dpc, TimerDpcRoutine, DeviceExtension);
    KeInitializeSpinLock(&DeviceExtension->Lock);
    KeInitializeEvent(&DeviceExtension->Event, SynchronizationEvent, FALSE);
    InitializeListHead(&DeviceExtension->List);

    /* Attempt to enable HDL support */
    EnableData = TRUE;
    Status = HeadlessDispatch(HeadlessCmdEnableTerminal,
                              &EnableData,
                              sizeof(EnableData),
                              NULL,
                              NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Bail out if we couldn't even get this far */
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting (1) with status FALSE\n");
        return FALSE;
    }

    /* Remember which process we started in */
    DeviceExtension->Process = IoGetCurrentProcess();

    /* Protect the device against non-admins */
    Status = CreateDeviceSecurityDescriptor(&DeviceExtension->DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        /* Write down why we failed */
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting (2) with status FALSE\n");

        /* Disable the HDL terminal on failure */
        EnableData = FALSE;
        Status = HeadlessDispatch(HeadlessCmdEnableTerminal,
                                  &EnableData,
                                  sizeof(EnableData),
                                  NULL,
                                  NULL);
        if (!NT_SUCCESS(Status)) SAC_DBG(SAC_DBG_INIT, "Failed dispatch\n");

        /* Bail out */
        return FALSE;
    }

    /* Create the worker thread */
    Status = PsCreateSystemThread(&DeviceExtension->WorkerThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  NULL,
                                  NULL,
                                  NULL,
                                  WorkerThreadStartUp,
                                  DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        /* Write down why we failed */
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting (3) with status FALSE\n");

        /* Disable the HDL terminal on failure */
        EnableData = FALSE;
        Status = HeadlessDispatch(HeadlessCmdEnableTerminal,
                                  &EnableData,
                                  sizeof(EnableData),
                                  NULL,
                                  NULL);
        if (!NT_SUCCESS(Status)) SAC_DBG(SAC_DBG_INIT, "Failed dispatch\n");

        /* Bail out */
        return FALSE;
    }

    /* Set the priority of our thread to highest */
    PriorityValue = HIGH_PRIORITY;
    Status = NtSetInformationThread(DeviceExtension->WorkerThreadHandle,
                                    ThreadPriority,
                                    &PriorityValue,
                                    sizeof(PriorityValue));
    if (!NT_SUCCESS(Status))
    {
        /* For debugging, write down why we failed */
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting (6) with status FALSE\n");
        DeviceExtension->PriorityFail = TRUE;

        /* Initialize rundown and wait for the thread to do it */
        KeInitializeEvent(&DeviceExtension->RundownEvent, SynchronizationEvent, FALSE);
        KeSetEvent(&DeviceExtension->Event, DeviceExtension->PriorityBoost, FALSE);
        Status = KeWaitForSingleObject(&DeviceExtension->RundownEvent,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);
        ASSERT(Status == STATUS_SUCCESS);

        /* Disable the HDL terminal on failure */
        EnableData = FALSE;
        Status = HeadlessDispatch(HeadlessCmdEnableTerminal,
                                  &EnableData,
                                  sizeof(EnableData),
                                  NULL,
                                  NULL);
        if (!NT_SUCCESS(Status)) SAC_DBG(SAC_DBG_INIT, "Failed dispatch\n");

        /* Bail out */
        return FALSE;
    }

    /* The first "packet" is the machine information in XML... */
    Status = TranslateMachineInformationXML(&Message, NULL);
    if (NT_SUCCESS(Status))
    {
        /* Go ahead and send it */
        UTF8EncodeAndSend(L"<?xml version=\"1.0\"?>\r\n");
        UTF8EncodeAndSend(Message);

        /* Free the temporary buffer */
        SacFreePool(Message);
    }

    /* Finally, initialize the I/O Manager */
    Status = ConMgrInitialize();
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Set the timer. Once this is done, the device is initialized */
    DueTime.QuadPart = -4000;
    KeSetTimerEx(&DeviceExtension->Timer, DueTime, 4, &DeviceExtension->Dpc);
    DeviceExtension->Initialized = TRUE;

SuccessExit:
    /* Success path -- everything worked */
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting with status TRUE\n");
    return TRUE;
}

BOOLEAN
NTAPI
InitializeGlobalData(IN PUNICODE_STRING RegistryPath,
                     IN PDRIVER_OBJECT DriverObject)
{
    NTSTATUS Status;
    UNICODE_STRING LinkName;
    UNICODE_STRING DeviceName;
    UNICODE_STRING EventName;
    PAGED_CODE();
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "Entering.\n");

    /* If we already did this, bail out */
    if (GlobalDataInitialized) goto SuccessExit;

    /* Setup the symbolic link for Win32 support */
    RtlInitUnicodeString(&LinkName, L"\\DosDevices\\SAC");
    RtlInitUnicodeString(&DeviceName, L"\\Device\\SAC");
    Status = IoCreateSymbolicLink(&LinkName, &DeviceName);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Initialize the internal heap manager */
    if (!InitializeMemoryManagement())
    {
        IoDeleteSymbolicLink(&LinkName);
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting with status FALSE\n");
        return FALSE;
    }

    /* Preload the messages in memory */
    Status = PreloadGlobalMessageTable(DriverObject->DriverStart);
    if (!NT_SUCCESS(Status))
    {
        IoDeleteSymbolicLink(&LinkName);
        SAC_DBG(SAC_DBG_INIT, "unable to pre-load message table: %X\n", Status);
        return FALSE;
    }

    /* Check if the administrator enabled this */
    Status = GetCommandConsoleLaunchingPermission(&CommandConsoleLaunchingEnabled);
    if (!NT_SUCCESS(Status))
    {
        /* Is it enabled? */
        if (CommandConsoleLaunchingEnabled)
        {
            /* Set the service start type to the correct value */
            Status = ImposeSacCmdServiceStartTypePolicy();
            if (!NT_SUCCESS(Status))
            {
                SAC_DBG(SAC_DBG_INIT, "failed ImposeSacCmdServiceStartTypePolicy: %X\n", Status);
            }
        }

        /* We're going to keep going with the default */
        SAC_DBG(SAC_DBG_INIT, "failed GetCommandConsoleLaunchingPermission: %X\n", Status);
    }

    /* Allocate the UTF-8 Conversion Buffer */
    Utf8ConversionBuffer = SacAllocatePool(Utf8ConversionBufferSize, GLOBAL_BLOCK_TAG);
    if (!Utf8ConversionBuffer)
    {
        /* Handle failure case */
        TearDownGlobalMessageTable();
        IoDeleteSymbolicLink(&LinkName);
        SAC_DBG(SAC_DBG_INIT, "unable to allocate memory for UTF8 translation\n");
        return FALSE;
    }

    /* Initialize the channel manager */
    Status = ChanMgrInitialize();
    if (!NT_SUCCESS(Status))
    {
        /* Handle failure case */
        SacFreePool(Utf8ConversionBuffer);
        TearDownGlobalMessageTable();
        IoDeleteSymbolicLink(&LinkName);
        SAC_DBG(SAC_DBG_INIT, "Failed to create SAC Channel\n");
        return FALSE;
    }

    /* Allocate the serial port buffer */
    SerialPortBuffer = SacAllocatePool(SAC_SERIAL_PORT_BUFFER_SIZE, GLOBAL_BLOCK_TAG);
    if (!SerialPortBuffer)
    {
        /* Handle failure case */
        SacFreePool(Utf8ConversionBuffer);
        TearDownGlobalMessageTable();
        IoDeleteSymbolicLink(&LinkName);
        SAC_DBG(SAC_DBG_INIT, "Failed to allocate Serial Port Buffer\n");
        return FALSE;
    }

    /* Zero it out */
    RtlZeroMemory(SerialPortBuffer, SAC_SERIAL_PORT_BUFFER_SIZE);

    /* Initialize command events. After this, driver data is good to go */
    KeInitializeMutex(&SACCMDEventInfoMutex, FALSE);
    InitializeCmdEventInfo();
    GlobalDataInitialized = TRUE;
    ProcessingType = 0;
    IoctlSubmitted = 0;

    /* Create the SAC event */
    RtlInitUnicodeString(&EventName, L"\\SACEvent");
    SACEvent = IoCreateSynchronizationEvent(&EventName, &SACEventHandle);
    if (!SACEvent)
    {
        /* Handle failure case */
        SacFreePool(Utf8ConversionBuffer);
        TearDownGlobalMessageTable();
        IoDeleteSymbolicLink(&LinkName);
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting with event NULL\n");
        return FALSE;
    }

    /* Cache machine information */
    InitializeMachineInformation();

    /* Register it */
    Status = RegisterBlueScreenMachineInformation();
    if (!NT_SUCCESS(Status))
    {
        /* Handle failure case */
        SacFreePool(Utf8ConversionBuffer);
        TearDownGlobalMessageTable();
        IoDeleteSymbolicLink(&LinkName);
        SAC_DBG(SAC_DBG_INIT, "Failed to register blue screen machine info\n");
        return FALSE;
    }

SuccessExit:
    /* Success path -- everything worked */
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting with status TRUE\n");
    return TRUE;
}
