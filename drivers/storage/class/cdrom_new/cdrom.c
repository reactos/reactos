/*--

Copyright (C) Microsoft Corporation. All rights reserved.

Module Name:

    cdrom.c

Abstract:

    The CDROM class driver tranlates IRPs to SRBs with embedded CDBs
    and sends them to its devices through the port driver.

Environment:

    kernel mode only

Notes:


Revision History:

--*/

// this definition is used to link _StorDebugPrint() function.
#define DEBUG_MAIN_SOURCE   1


#include "ntddk.h"
#include "ntstrsafe.h"

#include "ntddstor.h"
#include "ntddtape.h"
#include "wdfcore.h"
#include "devpkey.h"

#include "cdrom.h"
#include "ioctl.h"
#include "mmc.h"
#include "scratch.h"


#ifdef DEBUG_USE_WPP
#include "cdrom.tmh"
#endif

BOOLEAN
BootEnvironmentIsWinPE(
    VOID
    );

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, BootEnvironmentIsWinPE)

#pragma alloc_text(PAGE, DriverEvtCleanup)
#pragma alloc_text(PAGE, DriverEvtDeviceAdd)
#pragma alloc_text(PAGE, DeviceEvtCleanup)
#pragma alloc_text(PAGE, DeviceEvtSelfManagedIoCleanup)
#pragma alloc_text(PAGE, DeviceEvtD0Exit)
#pragma alloc_text(PAGE, CreateQueueEvtIoDefault)
#pragma alloc_text(PAGE, DeviceEvtFileClose)
#pragma alloc_text(PAGE, DeviceCleanupProtectedLocks)
#pragma alloc_text(PAGE, DeviceCleanupDisableMcn)
#pragma alloc_text(PAGE, RequestProcessSerializedIoctl)
#pragma alloc_text(PAGE, ReadWriteWorkItemRoutine)
#pragma alloc_text(PAGE, IoctlWorkItemRoutine)
#pragma alloc_text(PAGE, DeviceEvtSurpriseRemoval)

#endif

NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
    NTSTATUS                status;
    WDF_DRIVER_CONFIG       config;
    WDF_OBJECT_ATTRIBUTES   attributes;
    WDFDRIVER               driverObject = NULL;

    PAGED_CODE();

    // Initialize WPP Tracing
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                "CDROM.SYS DriverObject %p loading\n",
                DriverObject));

    // Register DeviceAdd and DriverEvtCleanup callback.
    // WPP_CLEANUP will be called in DriverEvtCleanup
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CDROM_DRIVER_EXTENSION);
    attributes.EvtCleanupCallback = DriverEvtCleanup;

    WDF_DRIVER_CONFIG_INIT(&config, DriverEvtDeviceAdd);

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             &attributes,
                             &config,
                             &driverObject);

    if (!NT_SUCCESS(status))
    {
        TracePrint((TRACE_LEVEL_FATAL, TRACE_FLAG_INIT,
                    "WdfDriverCreate failed. %x\n",
                    status));

        // Cleanup tracing here because DriverUnload will not be called
        // as we have failed to create WDFDRIVER object itself.
        WPP_CLEANUP(DriverObject);

    }
    else
    {
        PCDROM_DRIVER_EXTENSION driverExtension = DriverGetExtension(driverObject);

        // Copy the registry path into the driver extension so we can use it later
        driverExtension->Version = 0x01;
        driverExtension->DriverObject = DriverObject;

        if (BootEnvironmentIsWinPE()) {

            SET_FLAG(driverExtension->Flags, CDROM_FLAG_WINPE_MODE);
        }

    }

    return status;
}


BOOLEAN
BootEnvironmentIsWinPE(
    VOID
    )
/*++

Routine Description:

    This routine determines if the boot enviroment is WinPE

Arguments:

    None

Return Value:

    BOOLEAN - TRUE if the environment is WinPE; FALSE otherwise

--*/
{
    NTSTATUS    status;
    WDFKEY      registryKey = NULL;

    DECLARE_CONST_UNICODE_STRING(registryKeyName, WINPE_REG_KEY_NAME);

    PAGED_CODE();

    status = WdfRegistryOpenKey(NULL,
                                &registryKeyName,
                                KEY_READ,
                                WDF_NO_OBJECT_ATTRIBUTES,
                                &registryKey);

    if (!NT_SUCCESS(status))
    {
        return FALSE;
    }

    WdfRegistryClose(registryKey);
    return TRUE;
} // end BootEnvironmentIsWinPE()


VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DriverEvtCleanup(
    _In_ WDFOBJECT Driver
    )
/*++
Routine Description:

    Free all the resources allocated in DriverEntry.

Arguments:

    Driver - handle to a WDF Driver object.

Return Value:

    VOID.

--*/
{
    WDFDRIVER driver = (WDFDRIVER)Driver;

    PAGED_CODE ();

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                "CDROM.SYS DriverObject %p cleanup. Will be unloaded soon\n",
                driver));

    // Stop WPP Tracing
    WPP_CLEANUP( WdfDriverWdmGetDriverObject(driver) );


    return;
}


NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DriverEvtDeviceAdd(
    _In_    WDFDRIVER        Driver,
    _Inout_ PWDFDEVICE_INIT  DeviceInit
    )
/*++

Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager.


Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                        status = STATUS_SUCCESS;
    PCDROM_DRIVER_EXTENSION         driverExtension = NULL;
    WDFDEVICE                       device = NULL;
    PCDROM_DEVICE_EXTENSION         deviceExtension = NULL;
    BOOLEAN                         deviceClaimed = FALSE;

    WDF_OBJECT_ATTRIBUTES           attributes;
    WDF_FILEOBJECT_CONFIG           fileObjectConfig;
    WDF_IO_TARGET_OPEN_PARAMS       ioTargetOpenParams;
    WDF_IO_QUEUE_CONFIG             queueConfig;
    WDF_PNPPOWER_EVENT_CALLBACKS    pnpPowerCallbacks;
    WDF_REMOVE_LOCK_OPTIONS         removeLockOptions;
    PWCHAR                          wideDeviceName = NULL;
    UNICODE_STRING                  unicodeDeviceName;
    PDEVICE_OBJECT                  lowerPdo = NULL;
    ULONG                           deviceNumber = 0;
    ULONG                           devicePropertySessionId = INVALID_SESSION;
    ULONG                           devicePropertySize = 0;
    DEVPROPTYPE                     devicePropertyType = DEVPROP_TYPE_EMPTY;

    PAGED_CODE();

    driverExtension = DriverGetExtension(Driver);

    // 0. Initialize the objects that we're going to use
    RtlInitUnicodeString(&unicodeDeviceName, NULL);

    // 1. Register PnP&Power callbacks for any we are interested in.
    // If a callback isn't set, Framework will take the default action by itself.
    {
        // Zero out the PnpPowerCallbacks structure.
        WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

        // Use this callback to init resources that are used by the device and only needs to be called once.
        pnpPowerCallbacks.EvtDeviceSelfManagedIoInit = DeviceEvtSelfManagedIoInit;

        // Use this callback to prepare device for coming back from a lower power mode to D0.
        pnpPowerCallbacks.EvtDeviceD0Entry = DeviceEvtD0Entry;

        // Use this callback to prepare device for entering into a lower power mode.
        pnpPowerCallbacks.EvtDeviceD0Exit = DeviceEvtD0Exit;

        // Use this callback to free any resources used by device and will be called when the device is
        // powered down.
        pnpPowerCallbacks.EvtDeviceSelfManagedIoCleanup = DeviceEvtSelfManagedIoCleanup;

        pnpPowerCallbacks.EvtDeviceSurpriseRemoval = DeviceEvtSurpriseRemoval;

        // Register the PnP and power callbacks.
        WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);
    }

    // 2. Register the EvtIoInCallerContext to deal with IOCTLs that need to stay in original context.
    WdfDeviceInitSetIoInCallerContextCallback(DeviceInit,
                                              DeviceEvtIoInCallerContext);

    // 3. Register PreprocessCallback for IRP_MJ_POWER, IRP_MJ_FLUSH_BUFFERS and IRP_MJ_SHUTDOWN
    {
        UCHAR minorFunctions[1];

        minorFunctions[0] = IRP_MN_SET_POWER;

        status = WdfDeviceInitAssignWdmIrpPreprocessCallback(DeviceInit,
                                                             RequestProcessSetPower,
                                                             IRP_MJ_POWER,
                                                             minorFunctions,
                                                             RTL_NUMBER_OF(minorFunctions));
        if (!NT_SUCCESS(status))
        {
            TracePrint((TRACE_LEVEL_FATAL, TRACE_FLAG_PNP,
                        "DriverEvtDeviceAdd: Assign IrpPreprocessCallback for IRP_MJ_POWER failed, "
                        "status: 0x%X\n", status));

            goto Exit;
        }

        status = WdfDeviceInitAssignWdmIrpPreprocessCallback(DeviceInit,
                                                             RequestProcessShutdownFlush,
                                                             IRP_MJ_FLUSH_BUFFERS,
                                                             NULL,
                                                             0);
        if (!NT_SUCCESS(status))
        {
            TracePrint((TRACE_LEVEL_FATAL, TRACE_FLAG_PNP,
                        "DriverEvtDeviceAdd: Assign IrpPreprocessCallback for IRP_MJ_FLUSH_BUFFERS failed, "
                        "status: 0x%X\n", status));

            goto Exit;
        }

        status = WdfDeviceInitAssignWdmIrpPreprocessCallback(DeviceInit,
                                                             RequestProcessShutdownFlush,
                                                             IRP_MJ_SHUTDOWN,
                                                             NULL,
                                                             0);
        if (!NT_SUCCESS(status))
        {
            TracePrint((TRACE_LEVEL_FATAL, TRACE_FLAG_PNP,
                        "DriverEvtDeviceAdd: Assign IrpPreprocessCallback for IRP_MJ_SHUTDOWN failed, "
                        "status: 0x%X\n", status));

            goto Exit;
        }
    }

    // 4. Set attributes to create Request Context area.
    {
        //Reuse this structure.
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes,
                                                CDROM_REQUEST_CONTEXT);
        attributes.EvtCleanupCallback = RequestEvtCleanup;

        WdfDeviceInitSetRequestAttributes(DeviceInit,
                                          &attributes);
    }

    // 5. Register FileObject related callbacks
    {
        // Add FILE_OBJECT_EXTENSION as the context to the file object.
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, FILE_OBJECT_CONTEXT);

        // Set Entry points for Create and Close..

        // The framework doesn't sync the file create requests with pnp/power
        // state. Re-direct all the file create requests to a dedicated
        // queue, which will be purged manually during removal.
        WDF_FILEOBJECT_CONFIG_INIT(&fileObjectConfig,
                                   NULL, //CreateQueueEvtIoDefault,
                                   DeviceEvtFileClose,
                                   WDF_NO_EVENT_CALLBACK); // No callback for Cleanup

        fileObjectConfig.FileObjectClass = WdfFileObjectWdfCannotUseFsContexts;

        // Since we are registering file events and fowarding create request
        // ourself, we must also set AutoForwardCleanupClose so that cleanup
        // and close can also get forwarded.
        fileObjectConfig.AutoForwardCleanupClose = WdfTrue;
        attributes.SynchronizationScope = WdfSynchronizationScopeNone;
        attributes.ExecutionLevel = WdfExecutionLevelPassive;

        // Indicate that file object is optional.
        fileObjectConfig.FileObjectClass |= WdfFileObjectCanBeOptional;

        WdfDeviceInitSetFileObjectConfig(DeviceInit,
                                         &fileObjectConfig,
                                         &attributes);
    }

    // 6. Initialize device-wide attributes
    {
        // Declare ourselves as NOT owning power policy.
        // The power policy owner in storage stack is port driver.
        WdfDeviceInitSetPowerPolicyOwnership(DeviceInit, FALSE);

        // Set other DeviceInit attributes.
        WdfDeviceInitSetExclusive(DeviceInit, FALSE);
        WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_CD_ROM);
        WdfDeviceInitSetCharacteristics(DeviceInit, FILE_REMOVABLE_MEDIA, FALSE);
        WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoDirect);
        WdfDeviceInitSetPowerPageable(DeviceInit);

        // We require the framework to acquire a remove lock before delivering all IRP types
        WDF_REMOVE_LOCK_OPTIONS_INIT(&removeLockOptions,
                                     WDF_REMOVE_LOCK_OPTION_ACQUIRE_FOR_IO);

        WdfDeviceInitSetRemoveLockOptions(DeviceInit, &removeLockOptions);

        // save the PDO for later reference
        lowerPdo = WdfFdoInitWdmGetPhysicalDevice(DeviceInit);

        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes,
                                                CDROM_DEVICE_EXTENSION);

        // We have a parallel queue, so WdfSynchronizationScopeNone is our only choice.
        attributes.SynchronizationScope = WdfSynchronizationScopeNone;
        attributes.ExecutionLevel = WdfExecutionLevelDispatch;

        // Provide a cleanup callback which will release memory allocated with ExAllocatePool*
        attributes.EvtCleanupCallback = DeviceEvtCleanup;
    }

    // 7. Now, the device can be created.
    {
        wideDeviceName = ExAllocatePoolWithTag(NonPagedPoolNx,
                                               64 * sizeof(WCHAR),
                                               CDROM_TAG_STRINGS);

        if (wideDeviceName == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        // Find the lowest device number currently available.
        do {
            status = RtlStringCchPrintfW((NTSTRSAFE_PWSTR)wideDeviceName,
                                         64,
                                         L"\\Device\\CdRom%d",
                                         deviceNumber);

            if (!NT_SUCCESS(status)) {
                TracePrint((TRACE_LEVEL_FATAL, TRACE_FLAG_PNP,
                            "DriverEvtDeviceAdd: Format device name failed with error: 0x%X\n", status));

                goto Exit;
            }

            RtlInitUnicodeString(&unicodeDeviceName, wideDeviceName);

            status = WdfDeviceInitAssignName(DeviceInit, &unicodeDeviceName);
            if (!NT_SUCCESS(status))
            {
                TracePrint((TRACE_LEVEL_FATAL, TRACE_FLAG_PNP,
                            "DriverEvtDeviceAdd: WdfDeviceInitAssignName() failed with error: 0x%X\n", status));

                goto Exit;
            }

            status = WdfDeviceCreate(&DeviceInit, &attributes, &device);

            deviceNumber++;

        } while (status == STATUS_OBJECT_NAME_COLLISION);

        // When this loop exits the count is inflated by one - fix that.
        deviceNumber--;

        if (!NT_SUCCESS(status))
        {
            TracePrint((TRACE_LEVEL_FATAL, TRACE_FLAG_PNP,
                        "DriverEvtDeviceAdd: Can not create a new device, status: 0x%X\n",
                        status));

            goto Exit;
        }
    }

    // 8. Fill up basic Device Extension settings and create a remote I/O target for the next-lower driver.
    //    The reason why we do not use the local I/O target is because we want to be able to close the
    //    I/O target on surprise removal.
    {
        deviceExtension = DeviceGetExtension(device);

        deviceExtension->Version = 0x01;
        deviceExtension->Size = sizeof(CDROM_DEVICE_EXTENSION);

        deviceExtension->DeviceObject = WdfDeviceWdmGetDeviceObject(device);
        deviceExtension->Device = device;
        deviceExtension->DriverExtension = driverExtension;

        deviceExtension->LowerPdo = lowerPdo;

        deviceExtension->DeviceType = FILE_DEVICE_CD_ROM;   //Always a FILE_DEVICE_CD_ROM for all device it manages.
        deviceExtension->DeviceName = unicodeDeviceName;

        deviceExtension->DeviceNumber = deviceNumber;
    }
    {
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = deviceExtension->Device;

        status = WdfIoTargetCreate(deviceExtension->Device,
                                   &attributes,
                                   &deviceExtension->IoTarget);

        if (!NT_SUCCESS(status))
        {
            TracePrint((TRACE_LEVEL_FATAL, TRACE_FLAG_PNP,
                        "DriverEvtDeviceAdd: Can not create a remote I/O target object, status: 0x%X\n",
                        status));
            goto Exit;
        }

        WDF_IO_TARGET_OPEN_PARAMS_INIT_EXISTING_DEVICE(&ioTargetOpenParams,
                                                       WdfDeviceWdmGetAttachedDevice(deviceExtension->Device));

        status = WdfIoTargetOpen(deviceExtension->IoTarget,
                                 &ioTargetOpenParams);
        if (!NT_SUCCESS(status))
        {
            TracePrint((TRACE_LEVEL_FATAL, TRACE_FLAG_PNP,
                        "DriverEvtDeviceAdd: Can not open a remote I/O target for the next-lower device, status: 0x%X\n",
                        status));

            WdfObjectDelete(deviceExtension->IoTarget);
            deviceExtension->IoTarget = NULL;

            goto Exit;
        }
    }

    // 9. Claim the device, so that port driver will only accept the commands from CDROM.SYS for this device.
    // NOTE: The I/O should be issued after the device is started. But we would like to claim
    // the device as soon as possible, so this legacy behavior is kept.
    status = DeviceClaimRelease(deviceExtension, FALSE);

    if (!NT_SUCCESS(status))
    {
        // Someone already had this device - we're in trouble
        goto Exit;
    }
    else
    {
        deviceClaimed = TRUE;
    }

    //
    // CDROM Queueing Structure
    //
    // a. EvtIoInCallerContext (prior to queueing):
    //      This event will be used ONLY to forward down IOCTLs that come in at PASSIVE LEVEL
    //      and need to be forwarded down the stack in the original context.
    //
    // b. Main input queue: serial queue for main dispatching
    //      This queue will be used to do all dispatching of requests to serialize
    //      access to the device.  Any request that was previously completed in
    //      the Dispatch routines will be completed here.  Anything requiring device
    //      I/O will be sent through the serial I/O queue.
    //
    // 10. Set up IO queues after device being created.
    //
    {
        WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig,
                                               WdfIoQueueDispatchSequential);

        queueConfig.PowerManaged                = WdfFalse;

#pragma prefast(push)
#pragma prefast(disable: 28155, "a joint handler for read/write cannot be EVT_WDF_IO_QUEUE_IO_READ and EVT_WDF_IO_QUEUE_IO_WRITE simultaneously")
#pragma prefast(disable: 28023, "a joint handler for read/write cannot be EVT_WDF_IO_QUEUE_IO_READ and EVT_WDF_IO_QUEUE_IO_WRITE simultaneously")
        queueConfig.EvtIoRead                   = SequentialQueueEvtIoReadWrite;
        queueConfig.EvtIoWrite                  = SequentialQueueEvtIoReadWrite;
#pragma prefast(pop)

        queueConfig.EvtIoDeviceControl          = SequentialQueueEvtIoDeviceControl;
        queueConfig.EvtIoCanceledOnQueue        = SequentialQueueEvtCanceledOnQueue;

        status = WdfIoQueueCreate(device,
                                  &queueConfig,
                                  WDF_NO_OBJECT_ATTRIBUTES,
                                  &(deviceExtension->SerialIOQueue));
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }

        // this queue is dedicated for file create requests.
        WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchParallel);

        queueConfig.PowerManaged = WdfFalse;
        queueConfig.EvtIoDefault = CreateQueueEvtIoDefault;

        //Reuse this structure.
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes,
                                                CDROM_REQUEST_CONTEXT);

        attributes.SynchronizationScope = WdfSynchronizationScopeNone;
        attributes.ExecutionLevel = WdfExecutionLevelPassive;

        status = WdfIoQueueCreate(device,
                                  &queueConfig,
                                  &attributes,
                                  &(deviceExtension->CreateQueue));

        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }

        // Configure the device to use driver created queue for dispatching create.
        status = WdfDeviceConfigureRequestDispatching(device,
                                                      deviceExtension->CreateQueue,
                                                      WdfRequestTypeCreate);

        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    // 11. Set the alignment requirements for the device based on the host adapter requirements.
    //
    // NOTE: this should have been set when device is attached on device stack,
    // by keeping this legacy code, we could avoid issue that this value was not correctly set at that time.
    if (deviceExtension->LowerPdo->AlignmentRequirement > deviceExtension->DeviceObject->AlignmentRequirement)
    {
        WdfDeviceSetAlignmentRequirement(deviceExtension->Device,
                                         deviceExtension->LowerPdo->AlignmentRequirement);
    }

    // 12. Initialization of miscellaneous internal properties

    // CDROMs are not partitionable so starting offset is 0.
    deviceExtension->StartingOffset.LowPart = 0;
    deviceExtension->StartingOffset.HighPart = 0;

    // Set the default geometry for the cdrom to match what NT 4 used.
    // these values will be used to compute the cylinder count rather
    // than using it's NT 5.0 defaults.
    deviceExtension->DiskGeometry.MediaType = RemovableMedia;
    deviceExtension->DiskGeometry.TracksPerCylinder = 0x40;
    deviceExtension->DiskGeometry.SectorsPerTrack = 0x20;

    deviceExtension->DeviceAdditionalData.ReadWriteRetryDelay100nsUnits = WRITE_RETRY_DELAY_DVD_1x;

    // Clear the SrbFlags and disable synchronous transfers
    deviceExtension->SrbFlags = SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

    // Set timeout value in seconds.
    deviceExtension->TimeOutValue = DeviceGetTimeOutValueFromRegistry();
    if ((deviceExtension->TimeOutValue > 30 * 60) || // longer than 30 minutes
        (deviceExtension->TimeOutValue == 0))
    {
        deviceExtension->TimeOutValue = SCSI_CDROM_TIMEOUT;
    }

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
               "DriverEvtDeviceAdd: device timeout is set to %x seconds",
               deviceExtension->TimeOutValue
               ));

#if (NTDDI_VERSION >= NTDDI_WIN8)
    deviceExtension->IsVolumeOnlinePending = TRUE;

    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);

    queueConfig.PowerManaged = WdfFalse;

    status = WdfIoQueueCreate(device,
                              &queueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &deviceExtension->ManualVolumeReadyQueue);

    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }
#endif

    // 13. Initialize the stuff related to media locking
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = deviceExtension->Device;
    status = WdfWaitLockCreate(&attributes,
                               &deviceExtension->EjectSynchronizationLock);

    deviceExtension->LockCount = 0;

    // 14. Initialize context structures needed for asynchronous release queue and power requests

    if (NT_SUCCESS(status))
    {
        status = DeviceInitReleaseQueueContext(deviceExtension);
    }

    if (NT_SUCCESS(status))
    {
        status = DeviceInitPowerContext(deviceExtension);
    }

    // 15. Create external access points other than device name.
    if (NT_SUCCESS(status))
    {
        status = DeviceCreateWellKnownName(deviceExtension);
    }

    // 16. Query session id from the PDO.
    if (NT_SUCCESS(status))
    {
        status = IoGetDevicePropertyData(deviceExtension->LowerPdo,
                                         &DEVPKEY_Device_SessionId,
                                         0,
                                         0,
                                         sizeof(devicePropertySessionId),
                                         &devicePropertySessionId,
                                         &devicePropertySize,
                                         &devicePropertyType);

        if (!NT_SUCCESS(status))
        {
            // The device is global.
            devicePropertySessionId = INVALID_SESSION;
            status = STATUS_SUCCESS;
        }
    }

    // 17. Register interfaces for this device.
    if (NT_SUCCESS(status))
    {
        status = DeviceRegisterInterface(deviceExtension, CdRomDeviceInterface);
    }

    if (NT_SUCCESS(status))
    {
        // If this is a per-session DO, don't register for mount interface so that
        // mountmgr will not automatically assign a drive letter.
        if (devicePropertySessionId == INVALID_SESSION)
        {
            status = DeviceRegisterInterface(deviceExtension, MountedDeviceInterface);
        }
    }

    // 18. Initialize the shutdown/flush lock
    if (NT_SUCCESS(status))
    {
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = deviceExtension->Device;

        status = WdfWaitLockCreate(&attributes, &deviceExtension->ShutdownFlushWaitLock);
        if (!NT_SUCCESS(status))
        {
            TracePrint((TRACE_LEVEL_FATAL, TRACE_FLAG_PNP,
                        "DriverEvtDeviceAdd: Cannot create shutdown/flush waitlock, status: 0x%X\n",
                        status));
        }
    }

    // 19. Initialize the work item that is used to initiate asynchronous reads/writes
    if (NT_SUCCESS(status))
    {
        WDF_WORKITEM_CONFIG workItemConfig;
        WDF_OBJECT_ATTRIBUTES workItemAttributes;

        WDF_WORKITEM_CONFIG_INIT(&workItemConfig,
                                 ReadWriteWorkItemRoutine
                                 );

        WDF_OBJECT_ATTRIBUTES_INIT(&workItemAttributes);
        workItemAttributes.ParentObject = deviceExtension->Device;

        status = WdfWorkItemCreate(&workItemConfig,
                                   &workItemAttributes,
                                   &deviceExtension->ReadWriteWorkItem
                                   );

        if (!NT_SUCCESS(status))
        {
            TracePrint((TRACE_LEVEL_FATAL, TRACE_FLAG_INIT,
                        "DriverEvtDeviceAdd: Cannot create read/write work item, status: 0x%X\n",
                        status));
        }
    }

    // 20. Initialize the work item that is used to process most IOCTLs at PASSIVE_LEVEL.
    if (NT_SUCCESS(status))
    {
        WDF_WORKITEM_CONFIG workItemConfig;
        WDF_OBJECT_ATTRIBUTES workItemAttributes;

        WDF_WORKITEM_CONFIG_INIT(&workItemConfig,
                                 IoctlWorkItemRoutine
                                 );

        WDF_OBJECT_ATTRIBUTES_INIT(&workItemAttributes);
        workItemAttributes.ParentObject = deviceExtension->Device;

        status = WdfWorkItemCreate(&workItemConfig,
                                   &workItemAttributes,
                                   &deviceExtension->IoctlWorkItem
                                   );

        if (!NT_SUCCESS(status))
        {
            TracePrint((TRACE_LEVEL_FATAL, TRACE_FLAG_INIT,
                        "DriverEvtDeviceAdd: Cannot create ioctl work item, status: 0x%X\n",
                        status));
        }
    }


Exit:

    if (!NT_SUCCESS(status))
    {
        FREE_POOL(wideDeviceName);

        if (deviceExtension != NULL)
        {
            RtlInitUnicodeString(&deviceExtension->DeviceName, NULL);
        }

        // Release the device with the port driver, if it was claimed
        if ((deviceExtension != NULL) && deviceClaimed)
        {
            DeviceClaimRelease(deviceExtension, TRUE);
        }
        deviceClaimed = FALSE;
    }

    return status;
}


VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DeviceEvtCleanup(
    _In_ WDFOBJECT Device
    )
/*++
Routine Description:

    Free all the resources allocated in DriverEvtDeviceAdd.

Arguments:

    Device - handle to a WDF Device object.

Return Value:

    VOID.

--*/
{
    WDFDEVICE                device = (WDFDEVICE)Device;
    PCDROM_DEVICE_EXTENSION  deviceExtension = NULL;

    PAGED_CODE ();

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                "WDFDEVICE %p cleanup: The device is about to be destroyed.\n",
                device));

    deviceExtension = DeviceGetExtension(device);

    FREE_POOL(deviceExtension->DeviceName.Buffer);
    RtlInitUnicodeString(&deviceExtension->DeviceName, NULL);

    if (deviceExtension->DeviceAdditionalData.WellKnownName.Buffer != NULL)
    {
        IoDeleteSymbolicLink(&deviceExtension->DeviceAdditionalData.WellKnownName);
    }

    FREE_POOL(deviceExtension->DeviceAdditionalData.WellKnownName.Buffer);
    RtlInitUnicodeString(&deviceExtension->DeviceAdditionalData.WellKnownName, NULL);

    return;
}


VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DeviceEvtSelfManagedIoCleanup(
    _In_ WDFDEVICE    Device
    )
/*++

Routine Description:

    this function is called when the device is removed.
    release the ownership of the device, release allocated resources.

Arguments:

    Device - Handle to device object

Return Value:

    None.

--*/
{
    NTSTATUS                status;
    PCDROM_DEVICE_EXTENSION deviceExtension = NULL;

    PAGED_CODE ();

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP,
                "DeviceEvtSelfManagedIoCleanup: WDFDEVICE %p is being stopped.\n",
                Device));

    // extract the device and driver extensions
    deviceExtension = DeviceGetExtension(Device);

    // Purge unprocessed requests, stop the IO queues.
    // Incoming request will be completed with STATUS_INVALID_DEVICE_STATE status.
    WdfIoQueuePurge(deviceExtension->SerialIOQueue, WDF_NO_EVENT_CALLBACK, WDF_NO_CONTEXT);
    WdfIoQueuePurge(deviceExtension->CreateQueue, WDF_NO_EVENT_CALLBACK, WDF_NO_CONTEXT);

    // Close the IoTarget so that we are sure there are no outstanding I/Os in the stack.
    if (deviceExtension->IoTarget)
    {
        WdfIoTargetClose(deviceExtension->IoTarget);
    }

    // Release the device
    if (!deviceExtension->SurpriseRemoved)
    {
        status = DeviceClaimRelease(deviceExtension, TRUE);  //status is mainly for debugging. we don't really care.
        UNREFERENCED_PARAMETER(status);             //defensive coding, avoid PREFAST warning.
    }

    // Be sure to flush the DPCs as the READ/WRITE timer routine may still be running
    // during device removal.  This call may take a while to complete.
    KeFlushQueuedDpcs();

    // Release all the memory that we have allocated.

    DeviceDeallocateMmcResources(Device);
    ScratchBuffer_Deallocate(deviceExtension);
    RtlZeroMemory(&(deviceExtension->DeviceAdditionalData.Mmc), sizeof(CDROM_MMC_EXTENSION));

    FREE_POOL(deviceExtension->DeviceDescriptor);
    FREE_POOL(deviceExtension->AdapterDescriptor);
    FREE_POOL(deviceExtension->PowerDescriptor);
    FREE_POOL(deviceExtension->SenseData);

    if (deviceExtension->DeviceAdditionalData.CachedInquiryData != NULL)
    {
        FREE_POOL(deviceExtension->DeviceAdditionalData.CachedInquiryData);
        deviceExtension->DeviceAdditionalData.CachedInquiryDataByteCount = 0;
    }

    FREE_POOL(deviceExtension->PrivateFdoData);

    DeviceReleaseMcnResources(deviceExtension);

    DeviceReleaseZPODDResources(deviceExtension);

    // Keep the system-wide CDROM count accurate, as programs use this info to
    // know when they have found all the cdroms in a system.
    IoGetConfigurationInformation()->CdRomCount--;

    deviceExtension->PartitionLength.QuadPart = 0;

    // All WDF objects related to Device will be automatically released
    // when the root object is deleted. No need to release them manually.

    return;
}

NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DeviceEvtD0Entry(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
    )
/*++

Routine Description:

    This function is called when the device is coming back from a lower power state to D0.
    This function cannot be placed in a pageable section.

Arguments:

    Device - Handle to device object
    PreviousState - Power state the device was in.

Return Value:

    NTSTATUS: alway STATUS_SUCCESS

--*/
{
    PCDROM_DEVICE_EXTENSION     deviceExtension;
    NTSTATUS                    status = STATUS_SUCCESS;
    PZERO_POWER_ODD_INFO        zpoddInfo = NULL;
    STORAGE_IDLE_POWERUP_REASON powerupReason = {0};

    UNREFERENCED_PARAMETER(PreviousState);
    deviceExtension = DeviceGetExtension(Device);

    // Make certain not to do anything before properly initialized
    if (deviceExtension->IsInitialized)
    {
        zpoddInfo = deviceExtension->ZeroPowerODDInfo;

        if (zpoddInfo != NULL)
        {
            if (zpoddInfo->InZeroPowerState != FALSE)
            {
                // We just woke up from Zero Power state
                zpoddInfo->InZeroPowerState = FALSE;
                zpoddInfo->RetryFirstCommand = TRUE;
                zpoddInfo->BecomingReadyRetryCount = BECOMING_READY_RETRY_COUNT;

                status = DeviceZPODDGetPowerupReason(deviceExtension, &powerupReason);

                if (NT_SUCCESS(status) &&
                    (powerupReason.PowerupReason == StoragePowerupDeviceAttention))
                {
                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER,
                                "DeviceEvtD0Entry: Device has left zero power state due to eject button pressed\n"
                                ));

                    // This wake-up is caused by user pressing the eject button.
                    // In case of drawer type, we need to soft eject the tray to emulate the effect.
                    // Note that the first command to the device after power resumed will
                    // be terminated with CHECK CONDITION status with sense code 6/29/00,
                    // but we already have a retry logic to handle this.
                    if ((zpoddInfo->LoadingMechanism == LOADING_MECHANISM_TRAY) && (zpoddInfo->Load == 0))    // Drawer
                    {
                        DeviceSendIoctlAsynchronously(deviceExtension, IOCTL_STORAGE_EJECT_MEDIA, deviceExtension->DeviceObject);
                    }
                }
                else
                {
                    // This wake-up is caused by non-cached CDB received or a 3rd-party driver
                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER,
                                "DeviceEvtD0Entry: Device has left zero power state due to IO received\n"
                                ));

                }
            }
        }

        DeviceEnableMainTimer(deviceExtension);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DeviceEvtD0Exit(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState
    )
/*++

Routine Description:

    This function is called when the device is entering lower powe state from D0 or it's removed.
    We only care about the case of device entering D3.
    The purpose of this function is to send SYNC CACHE command and STOP UNIT command if it's necessary.

Arguments:

    Device - Handle to device object
    TargetState - Power state the device is entering.

Return Value:

    NTSTATUS: alway STATUS_SUCCESS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PCDROM_DEVICE_EXTENSION deviceExtension = NULL;
    PZERO_POWER_ODD_INFO    zpoddInfo = NULL;

    PAGED_CODE ();

    deviceExtension = DeviceGetExtension(Device);
    zpoddInfo = deviceExtension->ZeroPowerODDInfo;

    // we only process the situation that the device is going into D3.
    if ((TargetState != WdfPowerDeviceD3) &&
        (TargetState != WdfPowerDeviceD3Final))
    {
        return STATUS_SUCCESS;
    }

    // Stop the main timer
    DeviceDisableMainTimer(deviceExtension);

    // note: do not stop CreateQueue as the create request can be handled by port driver even the device is in D3 status.

    // If initialization was not finished or the device was removed, we cannot interact
    // with it device, so we have to exit
    if ((!deviceExtension->IsInitialized) || deviceExtension->SurpriseRemoved)
    {
        return STATUS_SUCCESS;
    }


#ifdef DBG
    #if (WINVER >= 0x0601)
    // this API is introduced in Windows7
    {
        ULONG   secondsRemaining = 0;
        BOOLEAN watchdogTimeSupported = FALSE;

        watchdogTimeSupported = PoQueryWatchdogTime(deviceExtension->LowerPdo, &secondsRemaining);
        UNREFERENCED_PARAMETER(watchdogTimeSupported);
    }
    #endif
#endif

    deviceExtension->PowerDownInProgress = TRUE;

    status = PowerContextBeginUse(deviceExtension);

    deviceExtension->PowerContext.Options.PowerDown = TRUE;

    // Step 1. LOCK QUEUE
    if (NT_SUCCESS(status) &&
        (TargetState != WdfPowerDeviceD3Final))
    {
        status = DeviceSendPowerDownProcessRequest(deviceExtension, NULL, NULL);

        if (NT_SUCCESS(status))
        {
            deviceExtension->PowerContext.Options.LockQueue = TRUE;
        }

        // Ignore failure.
        status = STATUS_SUCCESS;
    }

    deviceExtension->PowerContext.PowerChangeState.PowerDown++;

    // Step 2. QUIESCE QUEUE
    if (NT_SUCCESS(status) &&
        (TargetState != WdfPowerDeviceD3Final))
    {
        status = DeviceSendPowerDownProcessRequest(deviceExtension, NULL, NULL);
        UNREFERENCED_PARAMETER(status);
        // We don't care about the status.
        status = STATUS_SUCCESS;
    }

    deviceExtension->PowerContext.PowerChangeState.PowerDown++;

    // Step 3. SYNC CACHE command should be sent to drive if the media is currently writable.
    if (NT_SUCCESS(status) &&
        deviceExtension->DeviceAdditionalData.Mmc.WriteAllowed)
    {
        status = DeviceSendPowerDownProcessRequest(deviceExtension, NULL, NULL);
        UNREFERENCED_PARAMETER(status);
        status = STATUS_SUCCESS;
    }

    deviceExtension->PowerContext.PowerChangeState.PowerDown++;

    // Step 4. STOP UNIT
    if (NT_SUCCESS(status) &&
        !TEST_FLAG(deviceExtension->ScanForSpecialFlags, CDROM_SPECIAL_DISABLE_SPIN_DOWN))
    {
        status = DeviceSendPowerDownProcessRequest(deviceExtension, NULL, NULL);
        UNREFERENCED_PARAMETER(status);
        status = STATUS_SUCCESS;
    }

    if (TargetState == WdfPowerDeviceD3Final)
    {
        // We're done with the power context.
        PowerContextEndUse(deviceExtension);
    }

    // Bumping the media  change count  will force the file system to verify the volume when we resume
    SET_FLAG(deviceExtension->DeviceObject->Flags, DO_VERIFY_VOLUME);
    InterlockedIncrement((PLONG)&deviceExtension->MediaChangeCount);

    // If this power down is caused by Zero Power ODD, we should mark the device as in ZPODD mode.
    if (zpoddInfo != NULL)
    {
        zpoddInfo->InZeroPowerState = TRUE;

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER,
                    "Device has entered zero power state\n"
                    ));
    }

    deviceExtension->PowerDownInProgress = FALSE;

    return STATUS_SUCCESS;
}


VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DeviceEvtSurpriseRemoval(
    _In_ WDFDEVICE    Device
    )
/*++

Routine Description:

    this function is called when the device is surprisely removed.
    Stop all IO queues so that there will be no more request being sent down.

Arguments:

    Device - Handle to device object

Return Value:

    None.

--*/
{
    PCDROM_DEVICE_EXTENSION deviceExtension = NULL;

    PAGED_CODE();

    deviceExtension = DeviceGetExtension(Device);

    deviceExtension->SurpriseRemoved = TRUE;

    // Stop the main timer
    DeviceDisableMainTimer(deviceExtension);

    // legacy behavior to set partition length to be 0.
    deviceExtension->PartitionLength.QuadPart = 0;

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                "Surprisely remove a WDFDEVICE %p\n", Device));

    return;
}


VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
CreateQueueEvtIoDefault(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request
    )
/*++

Routine Description:

    this function is called when CREATE irp comes.
    setup FileObject context fields, so it can be used to track MCN or exclusive lock/unlock.

Arguments:

    Queue - Handle to device queue

    Request - the creation request

Return Value:

    None

--*/
{
    WDFFILEOBJECT           fileObject = WdfRequestGetFileObject(Request);
    WDFDEVICE               device = WdfIoQueueGetDevice(Queue);
    NTSTATUS                status = STATUS_SUCCESS;
    PCDROM_DEVICE_EXTENSION deviceExtension = DeviceGetExtension(device);
    PFILE_OBJECT_CONTEXT    fileObjectContext = NULL;

    PAGED_CODE();

    if (fileObject == NULL) {

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_QUEUE,
                    "Error: received a file create request with file object set to NULL\n"));

        RequestCompletion(deviceExtension, Request, STATUS_INTERNAL_ERROR, 0);
        return;
    }

    fileObjectContext = FileObjectGetContext(fileObject);

    // Initialize this WDFFILEOBJECT's context
    fileObjectContext->DeviceObject = device;
    fileObjectContext->FileObject = fileObject;
    fileObjectContext->LockCount = 0;
    fileObjectContext->McnDisableCount = 0;
    fileObjectContext->EnforceStreamingRead = FALSE;
    fileObjectContext->EnforceStreamingWrite = FALSE;

    // send down the create synchronously
    status = DeviceSendRequestSynchronously(device, Request, FALSE);

    // Need to complete the request in this routine.
    RequestCompletion(deviceExtension, Request, status, WdfRequestGetInformation(Request));

    return;
}

VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DeviceEvtFileClose(
    _In_ WDFFILEOBJECT FileObject
    )
/*++

Routine Description:

    this function is called when CLOSE irp comes.
    clean up MCN / Lock if necessary

Arguments:

    FileObject - WDF file object created for the irp.

Return Value:

    None

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;

    PAGED_CODE();

    if (FileObject != NULL)
    {
        WDFDEVICE               device = WdfFileObjectGetDevice(FileObject);
        PCDROM_DEVICE_EXTENSION deviceExtension = DeviceGetExtension(device);
        PCDROM_DATA             cdData = &(deviceExtension->DeviceAdditionalData);
        PFILE_OBJECT_CONTEXT    fileObjectContext = FileObjectGetContext(FileObject);

        // cleanup locked media tray
        status = DeviceCleanupProtectedLocks(deviceExtension, fileObjectContext);
        if (!NT_SUCCESS(status))
        {
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                        "Failed to cleanup protected locks for WDFDEVICE %p, %!STATUS!\n", device, status));
        }

        // cleanup disabled MCN
        status = DeviceCleanupDisableMcn(deviceExtension, fileObjectContext);
        if (!NT_SUCCESS(status))
        {
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                        "Failed to disable MCN for WDFDEVICE %p, %!STATUS!\n", device, status));
        }

        // cleanup exclusive access
        if (EXCLUSIVE_MODE(cdData) && EXCLUSIVE_OWNER(cdData, FileObject))
        {
            status = DeviceUnlockExclusive(deviceExtension, FileObject, FALSE);
            if (!NT_SUCCESS(status))
            {
                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                            "Failed to release exclusive lock for WDFDEVICE %p, %!STATUS!\n", device, status));
            }
        }
    }

    return;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceCleanupProtectedLocks(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ PFILE_OBJECT_CONTEXT     FileObjectContext
    )
/*++

Routine Description:

    this function removes protected locks for the handle

Arguments:

    DeviceExtension - device context

    FileObject - WDF file object created for the irp.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;

    PAGED_CODE();

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                "CleanupProtectedLocks called for WDFDEVICE %p, WDFFILEOBJECT %p, locked %d times.\n",
                DeviceExtension->Device, FileObjectContext->FileObject, FileObjectContext->LockCount));

    // Synchronize with ejection and ejection control requests.
    WdfWaitLockAcquire(DeviceExtension->EjectSynchronizationLock, NULL);

    // For each secure lock on this handle decrement the secured lock count
    // for the FDO.  Keep track of the new value.
    if (FileObjectContext->LockCount != 0)
    {
        DeviceExtension->ProtectedLockCount -= FileObjectContext->LockCount;
        FileObjectContext->LockCount = 0;

        // If the new lock count has been dropped to zero then issue a lock
        // command to the device.
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                    "FDO secured lock count = %d "
                    "lock count = %d\n",
                    DeviceExtension->ProtectedLockCount,
                    DeviceExtension->LockCount));

        if ((DeviceExtension->ProtectedLockCount == 0) && (DeviceExtension->LockCount == 0))
        {
            SCSI_REQUEST_BLOCK  srb = {0};
            PCDB                cdb = (PCDB) &(srb.Cdb);

            srb.CdbLength = 6;

            cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;

            // TRUE - prevent media removal.
            // FALSE - allow media removal.
            cdb->MEDIA_REMOVAL.Prevent = FALSE;

            // Set timeout value.
            srb.TimeOutValue = DeviceExtension->TimeOutValue;
            status = DeviceSendSrbSynchronously(DeviceExtension->Device,
                                                &srb,
                                                NULL,
                                                0,
                                                FALSE,
                                                NULL);

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                        "Allow media removal (unlock) request to drive returned %!STATUS!\n",
                        status));
        }
    }

    WdfWaitLockRelease(DeviceExtension->EjectSynchronizationLock);

    return status;
}


_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceCleanupDisableMcn(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ PFILE_OBJECT_CONTEXT     FileObjectContext
    )
/*++

Routine Description:

    cleanup the MCN disable count for the handle

Arguments:

    DeviceExtension - device context

    FileObject - WDF file object created for the irp.

Return Value:

    NTSTATUS

--*/
{
    PAGED_CODE();

    TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_INIT,
                "CleanupDisableMcn called for WDFDEVICE %p, WDFFILEOBJECT %p, locked %d times.\n",
                DeviceExtension->Device, FileObjectContext->FileObject, FileObjectContext->McnDisableCount));

    // For each secure lock on this handle decrement the secured lock count
    // for the FDO.  Keep track of the new value.
    while (FileObjectContext->McnDisableCount != 0)
    {
        DeviceEnableMediaChangeDetection(DeviceExtension, FileObjectContext, TRUE);
    }

    return STATUS_SUCCESS;
}

VOID
NormalizeIoctl(
    _Inout_ PWDF_REQUEST_PARAMETERS  requestParameters
    )
{
    ULONG ioctlCode;
    ULONG baseCode;
    ULONG functionCode;

    // if this is a class driver ioctl then we need to change the base code
    // to IOCTL_STORAGE_BASE so that the switch statement can handle it.
    //
    // WARNING - currently the scsi class ioctl function codes are between
    // 0x200 & 0x300.  this routine depends on that fact
    ioctlCode = requestParameters->Parameters.DeviceIoControl.IoControlCode;
    baseCode = DEVICE_TYPE_FROM_CTL_CODE(ioctlCode);
    functionCode = (ioctlCode & (~0xffffc003)) >> 2;

    if ((baseCode == IOCTL_SCSI_BASE) ||
        (baseCode == IOCTL_DISK_BASE) ||
        (baseCode == IOCTL_TAPE_BASE) ||
        (baseCode == IOCTL_DVD_BASE) ||
        (baseCode == IOCTL_CDROM_BASE))
        //IOCTL_STORAGE_BASE does not need to be converted.
    {
        if((functionCode >= 0x200) && (functionCode <= 0x300))
        {
            ioctlCode = (ioctlCode & 0x0000ffff) | CTL_CODE(IOCTL_STORAGE_BASE, 0, 0, 0);

            TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL,
                        "IOCTL code recalibrate, New ioctl code is %lx\n",
                        ioctlCode));

            // Set the code into request parameters, then "requestParameters" needs to be used for dispatch functions.
            requestParameters->Parameters.DeviceIoControl.IoControlCode = ioctlCode;
        }
    }
}

VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DeviceEvtIoInCallerContext(
    _In_ WDFDEVICE    Device,
    _In_ WDFREQUEST   Request
    )
/*++
Routine Description:

    Responds to EvtIoInCallerContext events from KMDF
    It calls different functions to process different type of IOCTLs.

Arguments:

    Device - handle to a WDF Device object

    Request - handle to the incoming WDF Request object

Return Value:

    VOID.

--*/
{
    NTSTATUS                status  = STATUS_SUCCESS;
    PCDROM_DEVICE_EXTENSION deviceExtension = DeviceGetExtension(Device);
    PCDROM_REQUEST_CONTEXT  requestContext = RequestGetContext(Request);
    WDF_REQUEST_PARAMETERS  requestParameters;

    requestContext->DeviceExtension = deviceExtension;

    // set the received time
    RequestSetReceivedTime(Request);

    // get the request parameters
    WDF_REQUEST_PARAMETERS_INIT(&requestParameters);
    WdfRequestGetParameters(Request, &requestParameters);

    if (requestParameters.Type == WdfRequestTypeDeviceControl)
    {
        BOOLEAN                         processed = FALSE;
        PCDROM_DATA                     cdData = &(deviceExtension->DeviceAdditionalData);
        PMEDIA_CHANGE_DETECTION_INFO    info = deviceExtension->MediaChangeDetectionInfo;

        if (requestParameters.Parameters.DeviceIoControl.IoControlCode != IOCTL_MCN_SYNC_FAKE_IOCTL)
        {
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                        "Receiving IOCTL: %lx\n",
                        requestParameters.Parameters.DeviceIoControl.IoControlCode));
        }
        else
        {
            TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_IOCTL,
                        "Receiving IOCTL: %lx\n",
                        requestParameters.Parameters.DeviceIoControl.IoControlCode));
        }

        // If the device is in exclusive mode, check whether the request is from
        // the handle that locked the device.
        if (EXCLUSIVE_MODE(cdData) && !EXCLUSIVE_OWNER(cdData, WdfRequestGetFileObject(Request)))
        {
            BOOLEAN isBlocked = FALSE;

            status = RequestIsIoctlBlockedByExclusiveAccess(Request, &isBlocked);
            UNREFERENCED_PARAMETER(status); //defensive coding, avoid PREFAST warning.

            if (isBlocked)
            {
                if ((requestParameters.Parameters.DeviceIoControl.IoControlCode == IOCTL_STORAGE_EVENT_NOTIFICATION) &&
                    (info != NULL) && (info->AsynchronousNotificationSupported != FALSE))
                {
                    // If AN is supported and we receive a signal but we can't send down GESN
                    // due to exclusive lock, we should save it and fire a GESN when it's unlocked.
                    // We just need true/false here and don't need count because we will keep sending
                    // GESN until we deplete all events.
                    info->ANSignalPendingDueToExclusiveLock = TRUE;
                }

                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                            "Access Denied! Device in exclusive mode.Failing Ioctl %lx\n",
                            requestParameters.Parameters.DeviceIoControl.IoControlCode));
                RequestCompletion(deviceExtension, Request, STATUS_ACCESS_DENIED, 0);

                return;
            }
        }

        NormalizeIoctl(&requestParameters);

        // 1. All requests that don't need to access device can be processed immediately
        if (!processed)
        {
            processed = RequestDispatchProcessDirectly(Device, Request, requestParameters);
        }

        // 2. Requests that should be put in sequential queue.
        if (!processed)
        {
            processed = RequestDispatchToSequentialQueue(Device, Request, requestParameters);
        }

        // 3. Requests that need to be processed sequentially and in caller's context.
        if (!processed)
        {
            processed = RequestDispatchSyncWithSequentialQueue(Device, Request, requestParameters);
        }

        // 4. Special requests that needs different process in different cases.
        if (!processed)
        {
            processed = RequestDispatchSpecialIoctls(Device, Request, requestParameters);
        }

        // 5. This is default behavior for unknown IOCTLs. To pass it to lower level.
        if (!processed)
        {
            processed = RequestDispatchUnknownRequests(Device, Request, requestParameters);
        }

        // All requests should be processed already.
        NT_ASSERT(processed);
        UNREFERENCED_PARAMETER(processed); //defensive coding, avoid PREFAST warning.
    }
    else if (requestParameters.Type == WdfRequestTypeDeviceControlInternal)
    {
        RequestProcessInternalDeviceControl(Request, deviceExtension);
    }
    else
    {
        // Requests other than IOCTLs will be forwarded to default queue.
        status = WdfDeviceEnqueueRequest(Device, Request);
        if (!NT_SUCCESS(status))
        {
            RequestCompletion(deviceExtension, Request, status, WdfRequestGetInformation(Request));
        }
    }

    return;
}


BOOLEAN
RequestDispatchProcessDirectly(
    _In_ WDFDEVICE              Device,
    _In_ WDFREQUEST             Request,
    _In_ WDF_REQUEST_PARAMETERS RequestParameters
    )
/*++
Routine Description:

    These requests can be processed in a non-serialized manner, most of them don't need to access device.

Arguments:

    Device - handle to a WDF Device object

    Request - handle to the incoming WDF Request object

    RequestParameters - request parameters

Return Value:

    BOOLEAN - TRUE (request processed); FALSE (request is not processed in this function).

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    BOOLEAN                 processed = FALSE;
    size_t                  dataLength = 0;

    PCDROM_DEVICE_EXTENSION deviceExtension = DeviceGetExtension(Device);
    ULONG                   ioctlCode = RequestParameters.Parameters.DeviceIoControl.IoControlCode;

    switch (ioctlCode)
    {

    case IOCTL_CDROM_GET_INQUIRY_DATA:
    {
        status = RequestHandleGetInquiryData(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break; // complete the irp
    }

    case IOCTL_STORAGE_GET_MEDIA_TYPES_EX:
    {
        status = RequestHandleGetMediaTypeEx(deviceExtension, Request, &dataLength);

        processed = TRUE;
        break; // complete the irp
    }

    case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:
    {
        status = RequestHandleMountQueryUniqueId(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break; // complete the irp
    }

    case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
    {
        status = RequestHandleMountQueryDeviceName(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break; // complete the irp
    }

    case IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME:
    {
        status = RequestHandleMountQuerySuggestedLinkName(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break; // complete the irp
    }

    case IOCTL_STORAGE_GET_DEVICE_NUMBER:
    {
        status = RequestHandleGetDeviceNumber(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break; // complete the irp
    }

    case IOCTL_STORAGE_GET_HOTPLUG_INFO:
    {
        status = RequestHandleGetHotPlugInfo(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break; // complete the irp
    }

    case IOCTL_STORAGE_SET_HOTPLUG_INFO:
    {
        status = RequestHandleSetHotPlugInfo(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break; // complete the irp
    }

    case IOCTL_STORAGE_EVENT_NOTIFICATION:
    {
        status = RequestHandleEventNotification(deviceExtension, Request, &RequestParameters, &dataLength);

        processed = TRUE;
        break; // complete the irp
    }

#if (NTDDI_VERSION >= NTDDI_WIN8)
    case IOCTL_VOLUME_ONLINE:
    {
        //
        // Mount manager and volume manager will
        // follow this online with a post online
        // but other callers may not. In those
        // cases, we process this request right
        // away. We approximate that these other
        // callers are from user mode
        //

        if (WdfRequestGetRequestorMode(Request) == KernelMode)
        {
            processed = TRUE;
        }
        break;
    }
#endif

    default:
    {
        processed = FALSE;
        break;
    }

    } //end of switch (ioctlCode)

    if (processed)
    {
        UCHAR currentStackLocationFlags = 0;
        currentStackLocationFlags = RequestGetCurrentStackLocationFlags(Request);

        if ((status == STATUS_VERIFY_REQUIRED) &&
            (currentStackLocationFlags & SL_OVERRIDE_VERIFY_VOLUME))
        {
            // If the status is verified required and this request
            // should bypass verify required then retry the request.
            status = STATUS_IO_DEVICE_ERROR;
            UNREFERENCED_PARAMETER(status); // disables prefast warning; defensive coding...

            processed = RequestDispatchProcessDirectly(Device, Request, RequestParameters);
        }
        else
        {
            // Complete the request after processing it.
            RequestCompletion(deviceExtension, Request, status, dataLength);
        }
    }

    return processed;
}


BOOLEAN
RequestDispatchToSequentialQueue(
    _In_ WDFDEVICE              Device,
    _In_ WDFREQUEST             Request,
    _In_ WDF_REQUEST_PARAMETERS RequestParameters
    )
/*++
Routine Description:

    These requests can be processed in a non-serialized manner, most of them don't need to access device.

Arguments:

    Device - handle to a WDF Device object

    Request - handle to the incoming WDF Request object

    RequestParameters - request parameters

Return Value:

    BOOLEAN - TRUE (request processed); FALSE (request is not processed in this function).

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    BOOLEAN                 processed = FALSE;
    size_t                  dataLength = 0;
    BOOLEAN                 inZeroPowerState = FALSE;

    PCDROM_DEVICE_EXTENSION deviceExtension = DeviceGetExtension(Device);
    ULONG                   ioctlCode = RequestParameters.Parameters.DeviceIoControl.IoControlCode;
    PZERO_POWER_ODD_INFO    zpoddInfo = deviceExtension->ZeroPowerODDInfo;

    if ((zpoddInfo != NULL) &&
        (zpoddInfo->InZeroPowerState != FALSE))
    {
        inZeroPowerState = TRUE;
    }

    switch (ioctlCode)
    {

    case IOCTL_CDROM_RAW_READ:
    {
        status = RequestValidateRawRead(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_DISK_GET_DRIVE_GEOMETRY_EX:
    case IOCTL_CDROM_GET_DRIVE_GEOMETRY_EX:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: Get drive geometryEx\n"));
        if ( RequestParameters.Parameters.DeviceIoControl.OutputBufferLength <
             (ULONG)FIELD_OFFSET(DISK_GEOMETRY_EX, Data))
        {
            status = STATUS_BUFFER_TOO_SMALL;
            dataLength = FIELD_OFFSET(DISK_GEOMETRY_EX, Data);
        }
        else if (inZeroPowerState != FALSE)
        {
            status = STATUS_NO_MEDIA_IN_DEVICE;
        }
        else
        {
            status = STATUS_SUCCESS;
        }

        processed = TRUE;
        break;
    }

    case IOCTL_DISK_GET_DRIVE_GEOMETRY:
    case IOCTL_CDROM_GET_DRIVE_GEOMETRY:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: Get drive geometry\n"));
        if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(DISK_GEOMETRY))
        {
            status = STATUS_BUFFER_TOO_SMALL;
            dataLength = sizeof(DISK_GEOMETRY);
        }
        else if (inZeroPowerState != FALSE)
        {
            status = STATUS_NO_MEDIA_IN_DEVICE;
        }
        else
        {
            status = STATUS_SUCCESS;
        }

        processed = TRUE;
        break;
    }

    case IOCTL_CDROM_READ_TOC_EX:
    {
        status = RequestValidateReadTocEx(deviceExtension, Request, RequestParameters, &dataLength);

        if (inZeroPowerState != FALSE)
        {
            status = STATUS_NO_MEDIA_IN_DEVICE;
        }

        processed = TRUE;
        break;
    }

    case IOCTL_CDROM_READ_TOC:
    {
        status = RequestValidateReadToc(deviceExtension, RequestParameters, &dataLength);

        if (inZeroPowerState != FALSE)
        {
            status = STATUS_NO_MEDIA_IN_DEVICE;
        }

        processed = TRUE;
        break;
    }

    case IOCTL_CDROM_GET_LAST_SESSION:
    {
        status = RequestValidateGetLastSession(deviceExtension, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_CDROM_PLAY_AUDIO_MSF:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: Play audio MSF\n"));

        if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength <
            sizeof(CDROM_PLAY_AUDIO_MSF))
        {
            status = STATUS_INFO_LENGTH_MISMATCH;
        }
        else
        {
            status = STATUS_SUCCESS;
        }

        processed = TRUE;
        break;
    }

    case IOCTL_CDROM_SEEK_AUDIO_MSF:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: Seek audio MSF\n"));

        if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength <
            sizeof(CDROM_SEEK_AUDIO_MSF))
        {
            status = STATUS_INFO_LENGTH_MISMATCH;
        }
        else
        {
            status = STATUS_SUCCESS;
        }

        processed = TRUE;
        break;
    }

    case IOCTL_CDROM_PAUSE_AUDIO:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: Pause audio\n"));

        status = STATUS_SUCCESS;
        processed = TRUE;
        break;
    }

    case IOCTL_CDROM_RESUME_AUDIO:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: Resume audio\n"));

        status = STATUS_SUCCESS;
        processed = TRUE;
        break;
    }

    case IOCTL_CDROM_READ_Q_CHANNEL:
    {
        status = RequestValidateReadQChannel(Request, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_CDROM_GET_VOLUME:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: Get volume control\n"));

        if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(VOLUME_CONTROL))
        {
            status = STATUS_BUFFER_TOO_SMALL;
            dataLength = sizeof(VOLUME_CONTROL);
        }
        else
        {
            status = STATUS_SUCCESS;
        }

        processed = TRUE;
        break;
    }

    case IOCTL_CDROM_SET_VOLUME:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: Set volume control\n"));

        if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength <
            sizeof(VOLUME_CONTROL))
        {
            status = STATUS_INFO_LENGTH_MISMATCH;
        }
        else
        {
            status = STATUS_SUCCESS;
        }

        processed = TRUE;
        break;
    }

    case IOCTL_CDROM_STOP_AUDIO:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: Stop audio\n"));

        status = STATUS_SUCCESS;
        processed = TRUE;
        break;
    }

    case IOCTL_STORAGE_CHECK_VERIFY:
    case IOCTL_STORAGE_CHECK_VERIFY2:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: [%p] Check Verify\n", Request));

        // Following check will let the condition "OutputBufferLength == 0" pass.
        // Since it's legacy behavior in classpnp, we need to keep it.
        if ((RequestParameters.Parameters.DeviceIoControl.OutputBufferLength > 0) &&
            (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)))
        {
            status = STATUS_BUFFER_TOO_SMALL;
            dataLength = sizeof(ULONG);
        }
        else if (inZeroPowerState != FALSE)
        {
            status = STATUS_NO_MEDIA_IN_DEVICE;
        }
        else
        {
            status = STATUS_SUCCESS;
        }

        processed = TRUE;
        break;
    }

    case IOCTL_DVD_GET_REGION:
    {
        // validation will be done when process it.
        status = STATUS_SUCCESS;
        processed = TRUE;
        break;
    }

    case IOCTL_DVD_READ_STRUCTURE:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: [%p] IOCTL_DVD_READ_STRUCTURE\n", Request));

        status = RequestValidateDvdReadStructure(deviceExtension, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_DVD_READ_KEY:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: [%p] IOCTL_DVD_READ_KEY\n", Request));

        status = RequestValidateDvdReadKey(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_DVD_START_SESSION:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: [%p] IOCTL_DVD_START_SESSION\n", Request));

        status = RequestValidateDvdStartSession(deviceExtension, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_DVD_SEND_KEY:
    case IOCTL_DVD_SEND_KEY2:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: [%p] IOCTL_DVD_SEND_KEY\n", Request));

        status = RequestValidateDvdSendKey(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_STORAGE_SET_READ_AHEAD:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: [%p] SetReadAhead\n", Request));

        if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength <
            sizeof(STORAGE_SET_READ_AHEAD))
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            status = STATUS_SUCCESS;
        }

        processed = TRUE;
        break;
    }

    case IOCTL_DISK_IS_WRITABLE:
    {
        status = STATUS_SUCCESS;

        processed = TRUE;
        break;
    }

    case IOCTL_DISK_GET_DRIVE_LAYOUT:
    {
        ULONG requiredSize = 0;

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: Get drive layout\n"));

        requiredSize = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION, PartitionEntry[1]);

        if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength <
            requiredSize)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            dataLength = requiredSize;
        }
        else
        {
            status = STATUS_SUCCESS;
        }

        processed = TRUE;
        break;
    }

    case IOCTL_DISK_GET_DRIVE_LAYOUT_EX:
    {
        ULONG requiredSize = 0;

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: Get drive layoutEx\n"));

        requiredSize = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry[1]);

        if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength <
            requiredSize)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            dataLength = requiredSize;
        }
        else
        {
            status = STATUS_SUCCESS;
        }

        processed = TRUE;
        break;
    }

    case IOCTL_DISK_GET_PARTITION_INFO:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: Get Partition Info\n"));

        if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(PARTITION_INFORMATION))
        {
            status = STATUS_BUFFER_TOO_SMALL;
            dataLength = sizeof(PARTITION_INFORMATION);
        }
        else
        {
            status = STATUS_SUCCESS;
        }

        processed = TRUE;
        break;
    }

    case IOCTL_DISK_GET_PARTITION_INFO_EX:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: Get Partition InfoEx\n"));

        if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(PARTITION_INFORMATION_EX))
        {
            status = STATUS_BUFFER_TOO_SMALL;
            dataLength = sizeof(PARTITION_INFORMATION_EX);
        }
        else
        {
            status = STATUS_SUCCESS;
        }

        processed = TRUE;
        break;
    }

    case IOCTL_DISK_VERIFY:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: IOCTL_DISK_VERIFY to device %p through request %p\n",
                    Device,
                    Request));

        if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength <
            sizeof(VERIFY_INFORMATION))
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            status = STATUS_SUCCESS;
        }

        processed = TRUE;
        break;
    }

    case IOCTL_DISK_GET_LENGTH_INFO:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: Disk Get Length InfoEx\n"));

        if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(GET_LENGTH_INFORMATION))
        {
            status = STATUS_BUFFER_TOO_SMALL;
            dataLength = sizeof(GET_LENGTH_INFORMATION);
        }
        else if (inZeroPowerState != FALSE)
        {
            status = STATUS_NO_MEDIA_IN_DEVICE;
        }
        else
        {
            status = STATUS_SUCCESS;
        }

        processed = TRUE;
        break;
    }

    case IOCTL_CDROM_GET_CONFIGURATION:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: [%p] IOCTL_CDROM_GET_CONFIGURATION\n", Request));

        status = RequestValidateGetConfiguration(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_CDROM_SET_SPEED:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: [%p] IOCTL_CDROM_SET_SPEED\n", Request));

        status = RequestValidateSetSpeed(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_DVD_END_SESSION:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: [%p] IOCTL_DVD_END_SESSION\n", Request));

        status = RequestValidateDvdEndSession(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_AACS_END_SESSION:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: [%p] IOCTL_AACS_END_SESSION\n", Request));

        status = RequestValidateAacsEndSession(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_AACS_READ_MEDIA_KEY_BLOCK:
    {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                    "AACS: Querying full MKB with bufferSize of %x bytes\n",
                    (int)RequestParameters.Parameters.DeviceIoControl.OutputBufferLength
                    ));

        status = RequestValidateAacsReadMediaKeyBlock(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_AACS_START_SESSION:
    {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                    "AACS: Requesting AGID\n"
                    ));

        status = RequestValidateAacsStartSession(deviceExtension, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_AACS_SEND_CERTIFICATE:
    {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                    "AACS: Sending host certificate to drive\n"
                    ));

        status = RequestValidateAacsSendCertificate(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_AACS_GET_CERTIFICATE:
    {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                    "AACS: Querying drive certificate\n"
                    ));

        status = RequestValidateAacsGetCertificate(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_AACS_GET_CHALLENGE_KEY:
    {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                    "AACS: Querying drive challenge key\n"
                    ));

        status = RequestValidateAacsGetChallengeKey(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_AACS_SEND_CHALLENGE_KEY:
    {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                    "AACS: Sending drive challenge key\n"
                    ));

        status = RequestValidateAacsSendChallengeKey(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_AACS_READ_VOLUME_ID:
    {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                    "AACS: Reading volume ID\n"
                    ));

        status = RequestValidateAacsReadVolumeId(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_AACS_READ_SERIAL_NUMBER:
    {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                    "AACS: Reading Serial Number\n"
                    ));

        status = RequestValidateAacsReadSerialNumber(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_AACS_READ_MEDIA_ID:
    {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                    "AACS: Reading media ID\n"
                    ));

        status = RequestValidateAacsReadMediaId(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_AACS_READ_BINDING_NONCE:
    case IOCTL_AACS_GENERATE_BINDING_NONCE:
    {
        if (ioctlCode == IOCTL_AACS_GENERATE_BINDING_NONCE)
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                        "AACS: Generating new binding nonce\n"
                        ));
        }
        else
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                        "AACS: Reading existing binding nonce\n"
                        ));
        }

        status = RequestValidateAacsBindingNonce(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_CDROM_ENABLE_STREAMING:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: [%p] IOCTL_CDROM_ENABLE_STREAMING\n", Request));

        status = RequestValidateEnableStreaming(Request, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_CDROM_SEND_OPC_INFORMATION:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: [%p] IOCTL_CDROM_SEND_OPC_INFORMATION\n", Request));

        status = RequestValidateSendOpcInformation(Request, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_CDROM_GET_PERFORMANCE:
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                    "RequestDispatchToSequentialQueue: [%p] IOCTL_CDROM_GET_PERFORMANCE\n", Request));

        status = RequestValidateGetPerformance(Request, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_STORAGE_MEDIA_REMOVAL:
    case IOCTL_STORAGE_EJECTION_CONTROL:
    {
        if(RequestParameters.Parameters.DeviceIoControl.InputBufferLength <
           sizeof(PREVENT_MEDIA_REMOVAL))
        {
            status = STATUS_INFO_LENGTH_MISMATCH;
        }
        else
        {
            status = STATUS_SUCCESS;
        }

        processed = TRUE;
        break; // complete the irp
    }

    case IOCTL_STORAGE_MCN_CONTROL:
    {
        if(RequestParameters.Parameters.DeviceIoControl.InputBufferLength <
            sizeof(PREVENT_MEDIA_REMOVAL))
        {
            status = STATUS_INFO_LENGTH_MISMATCH;
        }
        else
        {
            status = STATUS_SUCCESS;
        }

        processed = TRUE;
        break; // complete the irp
    }

    case IOCTL_STORAGE_RESERVE:
    case IOCTL_STORAGE_RELEASE:
    {
        // there is no validate check currently.
        status = STATUS_SUCCESS;
        processed = TRUE;
        break;
    }

    case IOCTL_STORAGE_PERSISTENT_RESERVE_IN:
    case IOCTL_STORAGE_PERSISTENT_RESERVE_OUT:
    {
        status = RequestValidatePersistentReserve(deviceExtension, Request, RequestParameters, &dataLength);

        processed = TRUE;
        break;
    }

    case IOCTL_STORAGE_EJECT_MEDIA:
    case IOCTL_STORAGE_LOAD_MEDIA:
    case IOCTL_STORAGE_LOAD_MEDIA2:
    {
        status = STATUS_SUCCESS;

        processed = TRUE;
        break; // complete the irp
    }

    case IOCTL_STORAGE_FIND_NEW_DEVICES:
    {
        // process it.
        IoInvalidateDeviceRelations(deviceExtension->LowerPdo, BusRelations);

        status = STATUS_SUCCESS;

        processed = TRUE;
        break; // complete the irp
    }

    case IOCTL_STORAGE_READ_CAPACITY:
    {
        if (RequestParameters.Parameters.DeviceIoControl.OutputBufferLength < sizeof(STORAGE_READ_CAPACITY))
        {
            dataLength = sizeof(STORAGE_READ_CAPACITY);
            status = STATUS_BUFFER_TOO_SMALL;
        }
        else if (inZeroPowerState != FALSE)
        {
            status = STATUS_NO_MEDIA_IN_DEVICE;
        }
        else
        {
            status = STATUS_SUCCESS;
        }

        processed = TRUE;
        break; // complete the irp
    }

    case IOCTL_STORAGE_CHECK_PRIORITY_HINT_SUPPORT:
    {
        // for disk.sys only in original classpnp
        status = STATUS_NOT_SUPPORTED;

        processed = TRUE;
        break; // complete the irp
    }

#if (NTDDI_VERSION >= NTDDI_WIN8)
    case IOCTL_DISK_ARE_VOLUMES_READY:
    {
        // this request doesn't access device at all, so seemingly it can be processed
        // directly; however, in case volume online is not received, we will need to
        // park these requests in a queue, and the only way a request can be queued is
        // if the request came out of another queue.
        status = STATUS_SUCCESS;

        processed = TRUE;
        break;
    }

    case IOCTL_VOLUME_ONLINE:
    case IOCTL_VOLUME_POST_ONLINE:
    {
        status = STATUS_SUCCESS;

        processed = TRUE;
        break;
    }
#endif

    default:
    {
        processed = FALSE;
        break;
    }
    } //end of switch (ioctlCode)

    if (processed)
    {
        UCHAR currentStackLocationFlags = 0;
        currentStackLocationFlags = RequestGetCurrentStackLocationFlags(Request);

        if ((status == STATUS_VERIFY_REQUIRED) &&
            (currentStackLocationFlags & SL_OVERRIDE_VERIFY_VOLUME))
        {
            // If the status is verified required and this request
            // should bypass verify required then retry the request.
            status = STATUS_IO_DEVICE_ERROR;
            UNREFERENCED_PARAMETER(status); // disables prefast warning; defensive coding...

            processed = RequestDispatchToSequentialQueue(Device, Request, RequestParameters);
        }
        else
        {
            if (NT_SUCCESS(status))
            {
                // Forward the request to serialized queue.
                status = WdfDeviceEnqueueRequest(Device, Request);
            }

            if (!NT_SUCCESS(status))
            {
                // Validation failed / forward failed, complete the request.
                RequestCompletion(deviceExtension, Request, status, dataLength);
            }
        }
    }

    return processed;
}


BOOLEAN
RequestDispatchSyncWithSequentialQueue(
    _In_ WDFDEVICE              Device,
    _In_ WDFREQUEST             Request,
    _In_ WDF_REQUEST_PARAMETERS RequestParameters
    )
/*++
Routine Description:

    These requests need to stay in caller's context and be processed in serialized manner.

Arguments:

    Device - handle to a WDF Device object

    Request - handle to the incoming WDF Request object

    RequestParameters - request parameters

Return Value:

    BOOLEAN - TRUE (request processed); FALSE (request is not processed in this function).

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    BOOLEAN                 processed = FALSE;
    size_t                  dataLength = 0;

    PCDROM_DEVICE_EXTENSION deviceExtension = DeviceGetExtension(Device);
    ULONG                   ioctlCode = RequestParameters.Parameters.DeviceIoControl.IoControlCode;

    switch (ioctlCode)
    {

    case IOCTL_CDROM_EXCLUSIVE_ACCESS:
    {
        //1. Validate
        status =  RequestValidateExclusiveAccess(Request, RequestParameters, &dataLength);

        //2. keep the request in serialized manner and stay in user's context.
        if (NT_SUCCESS(status))
        {
            PCDROM_EXCLUSIVE_ACCESS exclusiveAccess = NULL;

            status = WdfRequestRetrieveInputBuffer(Request,
                                                   RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                                   &exclusiveAccess,
                                                   NULL);

            if (NT_SUCCESS(status))
            {
                // do not need to check "status" as it passed validation and cannot fail in WdfRequestRetrieveInputBuffer()
                switch (exclusiveAccess->RequestType)
                {

                    case ExclusiveAccessQueryState:
                    {
                        status = RequestSetContextFields(Request, RequestHandleExclusiveAccessQueryLockState);
                        break;
                    }

                    case ExclusiveAccessLockDevice:
                    {
                        status = RequestSetContextFields(Request, RequestHandleExclusiveAccessLockDevice);
                        break;
                    }

                    case ExclusiveAccessUnlockDevice:
                    {
                        status = RequestSetContextFields(Request, RequestHandleExclusiveAccessUnlockDevice);
                        break;
                    }
                    default:
                    {
                        // already valicated in RequestValidateExclusiveAccess()
                        NT_ASSERT(FALSE);
                        break;
                    }
                }
            }

            if (NT_SUCCESS(status))
            {
                // now, put the special synchronization information into the context
                status = RequestSynchronizeProcessWithSerialQueue(Device, Request);

                // "status" is used for debugging in above statement, reset to success to avoid further work in this function.
                status = STATUS_SUCCESS;
            }
        }

        processed = TRUE;
        break; // complete the irp
    }

    default:
    {
        processed = FALSE;
        break;
    }
    } //end of switch (ioctlCode)

    // Following process is only valid if the request is not really processed. (failed in validation)
    if (processed && !NT_SUCCESS(status))
    {
        UCHAR currentStackLocationFlags = 0;
        currentStackLocationFlags = RequestGetCurrentStackLocationFlags(Request);

        if ((status == STATUS_VERIFY_REQUIRED) &&
            (currentStackLocationFlags & SL_OVERRIDE_VERIFY_VOLUME))
        {
            //
            // If the status is verified required and this request
            // should bypass verify required then retry the request.
            //
            status = STATUS_IO_DEVICE_ERROR;
            UNREFERENCED_PARAMETER(status); // disables prefast warning; defensive coding...

            processed = RequestDispatchSyncWithSequentialQueue(Device, Request, RequestParameters);
        }
        else
        {
            // Validation failed / forward failed, complete the request.
            RequestCompletion(deviceExtension, Request, status, dataLength);
        }
    }

    return processed;
}


BOOLEAN
RequestDispatchSpecialIoctls(
    _In_ WDFDEVICE              Device,
    _In_ WDFREQUEST             Request,
    _In_ WDF_REQUEST_PARAMETERS RequestParameters
    )
/*++
Routine Description:

    These requests need to be processed in different manner according to input parameters

Arguments:

    Device - handle to a WDF Device object

    Request - handle to the incoming WDF Request object

    RequestParameters - request parameters

Return Value:

    BOOLEAN - TRUE (request processed); FALSE (request is not processed in this function).

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    BOOLEAN                 processed = FALSE;
    size_t                  dataLength = 0;
    BOOLEAN                 requestCompleted = FALSE;

    PCDROM_DEVICE_EXTENSION deviceExtension = DeviceGetExtension(Device);
    PCDROM_DATA             cdData = &(deviceExtension->DeviceAdditionalData);
    ULONG                   ioctlCode = RequestParameters.Parameters.DeviceIoControl.IoControlCode;

    switch (ioctlCode)
    {
    case IOCTL_SCSI_PASS_THROUGH:
    case IOCTL_SCSI_PASS_THROUGH_DIRECT:
    case IOCTL_SCSI_PASS_THROUGH_EX:
    case IOCTL_SCSI_PASS_THROUGH_DIRECT_EX:
    {
        // SPTI is considered special case as we need to set the MinorFunction before pass to low level.

#if defined (_WIN64)
        if (WdfRequestIsFrom32BitProcess(Request))
        {
            if ((ioctlCode == IOCTL_SCSI_PASS_THROUGH) || (ioctlCode == IOCTL_SCSI_PASS_THROUGH_DIRECT))
            {
                if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength < sizeof(SCSI_PASS_THROUGH32))
                {
                    status = STATUS_INVALID_PARAMETER;
                }
            }
            else
            {
                if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength < sizeof(SCSI_PASS_THROUGH32_EX))
                {
                    status = STATUS_INVALID_PARAMETER;
                }
            }
        }
        else
#endif
        {
            if ((ioctlCode == IOCTL_SCSI_PASS_THROUGH) || (ioctlCode == IOCTL_SCSI_PASS_THROUGH_DIRECT))
            {
                if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength < sizeof(SCSI_PASS_THROUGH))
                {
                    status = STATUS_INVALID_PARAMETER;
                }
            }
            else
            {
                if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength < sizeof(SCSI_PASS_THROUGH_EX))
                {
                    status = STATUS_INVALID_PARAMETER;
                }
            }
        }

        if (!NT_SUCCESS(status))
        {
            // validation failed.
            RequestCompletion(deviceExtension, Request, status, dataLength);
        }
        else
        {
            // keep the request in serialized manner and stay in user's context.
            status = RequestSetContextFields(Request, RequestHandleScsiPassThrough);

            if (NT_SUCCESS(status))
            {
                status = RequestSynchronizeProcessWithSerialQueue(Device, Request);
            }
            else
            {
                RequestCompletion(deviceExtension, Request, status, 0);
            }
        }

        requestCompleted = TRUE;
        processed = TRUE;
        break;
    }

    case IOCTL_STORAGE_QUERY_PROPERTY:
    {
        if (RequestParameters.Parameters.DeviceIoControl.InputBufferLength < sizeof(STORAGE_PROPERTY_QUERY))
        {
            status = STATUS_INFO_LENGTH_MISMATCH;
        }
        else
        {
            PSTORAGE_PROPERTY_QUERY inputBuffer = NULL;

            status = WdfRequestRetrieveInputBuffer(Request,
                                                   RequestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                                   &inputBuffer,
                                                   NULL);

            if (NT_SUCCESS(status))
            {
                if (!EXCLUSIVE_MODE(cdData) ||                                     // not locked
                    EXCLUSIVE_OWNER(cdData, WdfRequestGetFileObject(Request)) ||   // request is from lock owner
                    (inputBuffer->QueryType == PropertyExistsQuery))               // request not access device
                {
                    if (inputBuffer->PropertyId == StorageDeviceUniqueIdProperty)
                    {
                        // previously handled in classpnp
                        // keep the request in serialized manner and stay in user's context.
                        status = RequestSetContextFields(Request, RequestHandleQueryPropertyDeviceUniqueId);

                        if (NT_SUCCESS(status))
                        {
                            status = RequestSynchronizeProcessWithSerialQueue(Device, Request);
                            // remeber that the request has been completed.
                            requestCompleted = TRUE;
                        }
                    }
                    else if (inputBuffer->PropertyId == StorageDeviceWriteCacheProperty)
                    {
                        // previously handled in classpnp
                        // keep the request in serialized manner and stay in user's context.
                        status = RequestSetContextFields(Request, RequestHandleQueryPropertyWriteCache);

                        if (NT_SUCCESS(status))
                        {
                            status = RequestSynchronizeProcessWithSerialQueue(Device, Request);
                            // remeber that the request has been completed.
                            requestCompleted = TRUE;
                        }
                    }
                    else
                    {
                        // Pass to port driver for handling
                        RequestDispatchUnknownRequests(Device, Request, RequestParameters);

                        // remeber that the request has been completed.
                        requestCompleted = TRUE;
                    }
                }
                else
                {
                    // If cached data exists, return cached data. Otherwise, fail the request.
                    if ((inputBuffer->QueryType == PropertyStandardQuery) &&
                        ((inputBuffer->PropertyId == StorageDeviceProperty) || (inputBuffer->PropertyId == StorageAdapterProperty)) )
                    {
                        status = RequestHandleQueryPropertyRetrieveCachedData(deviceExtension, Request, RequestParameters, &dataLength);
                    }
                    else
                    {
                        status = STATUS_ACCESS_DENIED;
                    }
                }
            }
        }

        processed = TRUE;
        break;
    }

    // this IOCTL is a fake one, used for MCN process sync-ed with serial queue.
    case IOCTL_MCN_SYNC_FAKE_IOCTL:
    {
        PIRP irp = WdfRequestWdmGetIrp(Request);

        if ((deviceExtension->MediaChangeDetectionInfo != NULL) &&
            (irp == deviceExtension->MediaChangeDetectionInfo->MediaChangeSyncIrp) &&
            (WdfRequestGetRequestorMode(Request) == KernelMode) &&
            (RequestParameters.Parameters.Others.Arg1 == RequestSetupMcnSyncIrp) &&
            (RequestParameters.Parameters.Others.Arg2 == RequestSetupMcnSyncIrp) &&
            (RequestParameters.Parameters.Others.Arg4 == RequestSetupMcnSyncIrp))
        {
            // This is the requset we use to sync Media Change Detection with sequential queue.
            status = WdfDeviceEnqueueRequest(Device, Request);

            if (!NT_SUCCESS(status))
            {
                RequestCompletion(deviceExtension, Request, status, dataLength);
            }

            requestCompleted = TRUE;
            processed = TRUE;
        }
        else
        {
            // process as an unknown request.
            processed = FALSE;
        }
        break;
    }

    default:
    {
        processed = FALSE;
        break;
    }
    } //end of switch (ioctlCode)

    if (processed && !requestCompleted)
    {
        UCHAR currentStackLocationFlags = 0;
        currentStackLocationFlags = RequestGetCurrentStackLocationFlags(Request);

        if ((status == STATUS_VERIFY_REQUIRED) &&
            (currentStackLocationFlags & SL_OVERRIDE_VERIFY_VOLUME))
        {
            // If the status is verified required and this request
            // should bypass verify required then retry the request.
            status = STATUS_IO_DEVICE_ERROR;
            UNREFERENCED_PARAMETER(status); // disables prefast warning; defensive coding...

            processed = RequestDispatchSpecialIoctls(Device, Request, RequestParameters);
        }
        else
        {
            RequestCompletion(deviceExtension, Request, status, dataLength);
        }
    }

    return processed;
}


BOOLEAN
RequestDispatchUnknownRequests(
    _In_ WDFDEVICE              Device,
    _In_ WDFREQUEST             Request,
    _In_ WDF_REQUEST_PARAMETERS RequestParameters
    )
/*++
Routine Description:

    All unknown requests will be pass to lower level driver.
    If IRQL is PASSIVE_LEVEL, the request will be serialized;
    Otherwise, it'll be sent and forget.

Arguments:

    Device - handle to a WDF Device object

    Request - handle to the incoming WDF Request object

    RequestParameters - request parameters

Return Value:

    BOOLEAN - TRUE (request processed); FALSE (request is not processed in this function).

--*/
{
    NTSTATUS                status = STATUS_UNSUCCESSFUL;
    PCDROM_DEVICE_EXTENSION deviceExtension = DeviceGetExtension(Device);

    ULONG       baseCode = DEVICE_TYPE_FROM_CTL_CODE(RequestParameters.Parameters.DeviceIoControl.IoControlCode);

    if ((KeGetCurrentIrql() != PASSIVE_LEVEL) ||
        (baseCode == FILE_DEVICE_ACPI))
    {
        // 1. When IRQL is higher than PASSIVE_LEVEL,
        // 2. ataport sends IOCTL_ACPI_ASYNC_EVAL_METHOD before queue starts,
        // send request directly to lower driver.
        status = RequestHandleUnknownIoctl(Device, Request);
    }
    else
    {
        // keep the request in serialized manner and stay in user's context.
        status = RequestSetContextFields(Request, RequestHandleUnknownIoctl);

        if (NT_SUCCESS(status))
        {
            status = RequestSynchronizeProcessWithSerialQueue(Device, Request);
        }
        else
        {
            RequestCompletion(deviceExtension, Request, status, 0);
        }
    }

    UNREFERENCED_PARAMETER(status); //defensive coding, avoid PREFAST warning.

    // All unknown IOCTLs are processed in this function.
    return TRUE; //processed
}

VOID
RequestProcessInternalDeviceControl(
    _In_ WDFREQUEST              Request,
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    )
/*++
Routine Description:

    all internal IOCTL will be send to lower driver asynchronously.

Arguments:

    Request - handle to the incoming WDF Request object
    DeviceExtension - device extension structure

Return Value:

    None

--*/
{
    NTSTATUS                 status = STATUS_SUCCESS;
    PIRP                     irp = NULL;
    PIO_STACK_LOCATION       irpStack = NULL;
    PIO_STACK_LOCATION       nextStack = NULL;
    BOOLEAN                  requestSent = FALSE;

    irp = WdfRequestWdmGetIrp(Request);
    irpStack = IoGetCurrentIrpStackLocation(irp);
    nextStack = IoGetNextIrpStackLocation(irp);

    // Set the parameters in the next stack location.
    nextStack->Parameters.Scsi.Srb = irpStack->Parameters.Scsi.Srb;
    nextStack->MajorFunction = IRP_MJ_SCSI;
    nextStack->MinorFunction = IRP_MN_SCSI_CLASS;

    WdfRequestSetCompletionRoutine(Request, RequestDummyCompletionRoutine, NULL);

    status = RequestSend(DeviceExtension,
                         Request,
                         DeviceExtension->IoTarget,
                         0,
                         &requestSent);

    // send the request straight down (asynchronously)
    if (!requestSent)
    {
        // fail the request
        RequestCompletion(DeviceExtension, Request, status, WdfRequestGetInformation(Request));
    }

    return;
}



//
// Serial I/O Queue Event callbacks
//

VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
SequentialQueueEvtIoReadWrite(
    _In_ WDFQUEUE    Queue,
    _In_ WDFREQUEST  Request,
    _In_ size_t      Length
    )
/*++
Routine Description:

    validate and process read/write request.

Arguments:

    Queue - parallel queue itself

    Request - handle to the incoming WDF Request object

    Length - read / write lenght

Return Value:

    None

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    WDFDEVICE               device = WdfIoQueueGetDevice(Queue);
    PCDROM_DEVICE_EXTENSION deviceExtension = DeviceGetExtension(device);
    WDF_REQUEST_PARAMETERS  requestParameters;
    PIRP                    wdmIrp = WdfRequestWdmGetIrp(Request);
    PIO_STACK_LOCATION      currentIrpStack = IoGetCurrentIrpStackLocation(wdmIrp);
    PCDROM_DATA             cdData = &(deviceExtension->DeviceAdditionalData);

    // Get the request parameters
    WDF_REQUEST_PARAMETERS_INIT(&requestParameters);
    WdfRequestGetParameters(Request, &requestParameters);

    if (requestParameters.Type ==  WdfRequestTypeRead)
    {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_GENERAL,
                    "Receiving READ, Length %Ix\n", (ULONG) Length));
    }
    else
    {
        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_GENERAL,
                    "Receiving WRITE, Length %Ix\n", (ULONG) Length));
    }

    // Check if a verify is required before a READ/WRITE
    if (TEST_FLAG(deviceExtension->DeviceObject->Flags, DO_VERIFY_VOLUME) &&
        (requestParameters.MinorFunction != CDROM_VOLUME_VERIFY_CHECKED) &&
        !TEST_FLAG(currentIrpStack->Flags, SL_OVERRIDE_VERIFY_VOLUME))
    {
        //  DO_VERIFY_VOLUME is set for the device object,
        //  but this request is not itself a verify request.
        //  So fail this request.
        RequestCompletion(deviceExtension, Request, STATUS_VERIFY_REQUIRED, 0);
    }
    else
    {
        //  Since we've bypassed the verify-required tests we don't need to repeat
        //  them with this IRP - in particular we don't want to worry about
        //  hitting them at the partition 0 level if the request has come through
        //  a non-zero partition.
        currentIrpStack->MinorFunction = CDROM_VOLUME_VERIFY_CHECKED;

        // Fail READ/WRITE requests when music is playing
        if (deviceExtension->DeviceAdditionalData.PlayActive)
        {
            RequestCompletion(deviceExtension, Request, STATUS_DEVICE_BUSY, 0);

            return;
        }

        // Fail READ/WRITE requests from non-owners if the drive is locked
        if (EXCLUSIVE_MODE(cdData) && !EXCLUSIVE_OWNER(cdData, WdfRequestGetFileObject(Request)))
        {
            RequestCompletion(deviceExtension, Request, STATUS_ACCESS_DENIED, 0);

            return;
        }

        // Succeed READ/WRITE requests of length 0
        if (Length == 0)
        {
            //  Several parts of the code turn 0 into 0xffffffff,
            //  so don't process a zero-length request any further.
            RequestCompletion(deviceExtension, Request, STATUS_SUCCESS, Length);

            return;
        }

        // If there is an unexpected write request, we want to rediscover MMC capabilities
        if (!deviceExtension->DeviceAdditionalData.Mmc.WriteAllowed &&
            (requestParameters.Type == WdfRequestTypeWrite))
        {
            // Schedule MMC capabilities update now, but perform it later in a work item
            deviceExtension->DeviceAdditionalData.Mmc.UpdateState = CdromMmcUpdateRequired;
        }

        // If MMC capabilities update is required, we create a separate work item to avoid blocking
        // the current thread; otherwise, we initiate an async read/write in the current thread.
        if (DeviceIsMmcUpdateRequired(deviceExtension->Device))
        {
            deviceExtension->ReadWriteWorkItemContext.OriginalRequest = Request;
            WdfWorkItemEnqueue(deviceExtension->ReadWriteWorkItem);

            status = STATUS_SUCCESS;
        }
        else
        {
            status = RequestValidateReadWrite(deviceExtension, Request, requestParameters);

            if (NT_SUCCESS(status))
            {
                status = RequestHandleReadWrite(deviceExtension, Request, requestParameters);
            }
        }

        if (!NT_SUCCESS(status))
        {
            RequestCompletion(deviceExtension, Request, status, 0);
        }
    }

    return;
}


VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ReadWriteWorkItemRoutine(
    _In_ WDFWORKITEM  WorkItem
    )
/*++

Routine Description:

    Work item routine for validating and initiating read and write requests.
    The reason why we do that from a work item is because we may need to update MMC
    capabilities before validating a read/write request and that is a sync operation.

Arguments:

    WorkItem - WDF work item

Return Value:

    none

--*/
{
    PCDROM_DEVICE_EXTENSION             deviceExtension = NULL;
    WDFREQUEST                          readWriteRequest = NULL;
    WDF_REQUEST_PARAMETERS              readWriteRequestParameters;
    NTSTATUS                            status = STATUS_SUCCESS;

    PAGED_CODE ();

    deviceExtension = WdfObjectGetTypedContext(WdfWorkItemGetParentObject(WorkItem), CDROM_DEVICE_EXTENSION);
    readWriteRequest = deviceExtension->ReadWriteWorkItemContext.OriginalRequest;
    deviceExtension->ReadWriteWorkItemContext.OriginalRequest = NULL;

    WDF_REQUEST_PARAMETERS_INIT(&readWriteRequestParameters);
    WdfRequestGetParameters(readWriteRequest, &readWriteRequestParameters);

    if (DeviceIsMmcUpdateRequired(deviceExtension->Device))
    {
        // Issue command to update the drive capabilities.
        // The failure of MMC update is not considered critical, so we'll
        // continue to process the request even if MMC update fails.
        (VOID) DeviceUpdateMmcCapabilities(deviceExtension->Device);
    }

    // Now verify and process the request
    if (NT_SUCCESS(status))
    {
        status = RequestValidateReadWrite(deviceExtension, readWriteRequest, readWriteRequestParameters);
    }
    if (NT_SUCCESS(status))
    {
        status = RequestHandleReadWrite(deviceExtension, readWriteRequest, readWriteRequestParameters);
    }

    // Complete the request immediately on failure
    if (!NT_SUCCESS(status))
    {
        RequestCompletion(deviceExtension, readWriteRequest, status, 0);
    }
}


VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
SequentialQueueEvtIoDeviceControl(
    _In_ WDFQUEUE   Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t     OutputBufferLength,
    _In_ size_t     InputBufferLength,
    _In_ ULONG      IoControlCode
    )
/*++
Routine Description:

    validate and process IOCTL request.

Arguments:

    Queue - sequential queue

    Request - handle to the incoming WDF Request object

Return Value:

    None

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    WDFDEVICE               device = WdfIoQueueGetDevice(Queue);
    PCDROM_DEVICE_EXTENSION deviceExtension = DeviceGetExtension(device);
    PCDROM_REQUEST_CONTEXT  requestContext = RequestGetContext(Request);
    PCDROM_DATA             cdData = &(deviceExtension->DeviceAdditionalData);
    WDF_REQUEST_PARAMETERS  requestParameters;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(IoControlCode);

    // get the request parameters
    WDF_REQUEST_PARAMETERS_INIT(&requestParameters);
    WdfRequestGetParameters(Request, &requestParameters);

    // If the device is in exclusive mode, check whether the request is from
    // the handle that locked the device
    if (EXCLUSIVE_MODE(cdData) && !EXCLUSIVE_OWNER(cdData, WdfRequestGetFileObject(Request)))
    {
        BOOLEAN isBlocked = FALSE;

        status = RequestIsIoctlBlockedByExclusiveAccess(Request, &isBlocked);
        if (NT_SUCCESS(status) && isBlocked)
        {
            if (requestContext->SyncRequired)
            {
                // set the following event, so RequestSynchronizeProcessWithSerialQueue() can contintue run to process the real request.
                // this function will wait for the request process finishes.
                KeSetEvent(requestContext->SyncEvent, IO_CD_ROM_INCREMENT, FALSE);
            }
            else
            {
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                            "DeviceEvtIoInCallerContext: Access Denied! Device in exclusive mode.Failing Ioctl %lx\n",
                            requestParameters.Parameters.DeviceIoControl.IoControlCode));
                RequestCompletion(deviceExtension, Request, STATUS_ACCESS_DENIED, 0);
            }

            return;
        }
    }

    if (!cdData->Mmc.WriteAllowed &&
        ((requestParameters.Parameters.DeviceIoControl.IoControlCode == IOCTL_DISK_IS_WRITABLE) ||
         (requestParameters.Parameters.DeviceIoControl.IoControlCode == IOCTL_DISK_VERIFY)))
    {
        cdData->Mmc.UpdateState = CdromMmcUpdateRequired;
    }

    // check if this is a synchronized ioctl
    if (requestContext->SyncRequired)
    {
        // set the following event, so RequestSynchronizeProcessWithSerialQueue() can contintue run to process the real request.
        // this function will wait for the request process finishes.
        KeSetEvent(requestContext->SyncEvent, IO_CD_ROM_INCREMENT, FALSE);
    }
    else
    {
        deviceExtension->IoctlWorkItemContext.OriginalRequest = Request;

        // all other IOCTL processing is currently processed via a
        // work item running at PASSIVE_LEVEL.
        WdfWorkItemEnqueue(deviceExtension->IoctlWorkItem);
    }

    return;
}


VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
IoctlWorkItemRoutine(
    _In_ WDFWORKITEM  WorkItem
    )
/*++

Routine Description:

    Work item routine for processing ioctl requests.
    This is needed because event callbacks are called at DISPATCH_LEVEL and ioctl
    requests are currently processed synchronously and not asynchronously.

Arguments:

    WorkItem - WDF work item

Return Value:

    none

--*/
{
    PCDROM_DEVICE_EXTENSION             deviceExtension = NULL;

    PAGED_CODE ();

    deviceExtension = WdfObjectGetTypedContext(WdfWorkItemGetParentObject(WorkItem), CDROM_DEVICE_EXTENSION);

    if (DeviceIsMmcUpdateRequired(deviceExtension->Device))
    {
        // Issue command to update the drive capabilities.
        // The failure of MMC update is not considered critical,
        // so that we'll continue to process I/O even MMC update fails.
        DeviceUpdateMmcCapabilities(deviceExtension->Device);
    }

    RequestProcessSerializedIoctl(deviceExtension, deviceExtension->IoctlWorkItemContext.OriginalRequest);
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
RequestProcessSerializedIoctl(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ WDFREQUEST               Request
    )
/*++
Routine Description:

    a dispatch routine for all functions to process IOCTLs.

Arguments:

    DeviceExtension - device context

    Request - handle to the incoming WDF Request object

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    size_t                  information = 0;
    WDF_REQUEST_PARAMETERS  requestParameters;
    BOOLEAN                 completeRequest = TRUE;

    PAGED_CODE ();

    // Get the Request parameters
    WDF_REQUEST_PARAMETERS_INIT(&requestParameters);
    WdfRequestGetParameters(Request, &requestParameters);

    NormalizeIoctl(&requestParameters);

    // process IOCTLs
    switch (requestParameters.Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_CDROM_READ_TOC:
    case IOCTL_CDROM_GET_LAST_SESSION:
        status = RequestHandleReadTOC(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_CDROM_READ_TOC_EX:
        status = RequestHandleReadTocEx(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_CDROM_GET_CONFIGURATION:
        status = RequestHandleGetConfiguration(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_CDROM_RAW_READ:
        status = DeviceHandleRawRead(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_DISK_GET_LENGTH_INFO:
    case IOCTL_DISK_GET_DRIVE_GEOMETRY:
    case IOCTL_DISK_GET_DRIVE_GEOMETRY_EX:
    case IOCTL_CDROM_GET_DRIVE_GEOMETRY:
    case IOCTL_CDROM_GET_DRIVE_GEOMETRY_EX:
    case IOCTL_STORAGE_READ_CAPACITY:
        status = RequestHandleGetDriveGeometry(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_DISK_VERIFY:
        status = RequestHandleDiskVerify(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_STORAGE_CHECK_VERIFY:
        // IOCTL_STORAGE_CHECK_VERIFY2 was processed including send a Test Unit Read
        // with srb flag SRB_CLASS_FLAGS_LOW_PRIORITY to port driver asynchronizelly.
        // The original request was completed after TUR finishes.
        // As CDROM.SYS serializes IOs need accessing device, it's not a big difference from above behavior to
        // just process it in serialized manner. So I put it here and treat it as same as IOCTL_STORAGE_CHECK_VERIFY.
    case IOCTL_STORAGE_CHECK_VERIFY2:
        status = RequestHandleCheckVerify(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_DISK_GET_DRIVE_LAYOUT:
    case IOCTL_DISK_GET_DRIVE_LAYOUT_EX:
    case IOCTL_DISK_GET_PARTITION_INFO:
    case IOCTL_DISK_GET_PARTITION_INFO_EX:
        status = RequestHandleFakePartitionInfo(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_DISK_IS_WRITABLE:
        //
        // Even though this media is writable, the requester of this IOCTL really
        // wants to know if thw media behaves like any other disk or not. This is
        // so if FeatureDefectManagement and FeatureRandomWritable are current on
        // the drive-represented by the FeatureDefectManagement validation schema
        //
        if (DeviceExtension->DeviceAdditionalData.Mmc.WriteAllowed &&
            (DeviceExtension->DeviceAdditionalData.Mmc.ValidationSchema == FeatureDefectManagement))
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_MEDIA_WRITE_PROTECTED;
        }
        information = 0;
        break;

    case IOCTL_CDROM_PLAY_AUDIO_MSF:
        status = DeviceHandlePlayAudioMsf(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_CDROM_READ_Q_CHANNEL:
        status = DeviceHandleReadQChannel(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_CDROM_PAUSE_AUDIO:
        status = DeviceHandlePauseAudio(DeviceExtension, Request, &information);
        break;

    case IOCTL_CDROM_RESUME_AUDIO:
        status = DeviceHandleResumeAudio(DeviceExtension, Request, &information);
        break;

    case IOCTL_CDROM_SEEK_AUDIO_MSF:
        status = DeviceHandleSeekAudioMsf(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_CDROM_STOP_AUDIO:
        status = DeviceHandleStopAudio(DeviceExtension, Request, &information);
        break;

    case IOCTL_CDROM_GET_VOLUME:
    case IOCTL_CDROM_SET_VOLUME:
        status = DeviceHandleGetSetVolume(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_DVD_GET_REGION:
        status = RequestHandleGetDvdRegion(DeviceExtension, Request, &information);
        break;

    case IOCTL_DVD_READ_STRUCTURE:
        status = DeviceHandleReadDvdStructure(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_DVD_END_SESSION:
        status = DeviceHandleDvdEndSession(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_DVD_START_SESSION:
    case IOCTL_DVD_READ_KEY:
        status = DeviceHandleDvdStartSessionReadKey(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_DVD_SEND_KEY:
    case IOCTL_DVD_SEND_KEY2:
        status = DeviceHandleDvdSendKey(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_STORAGE_SET_READ_AHEAD:
        status = DeviceHandleSetReadAhead(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_CDROM_SET_SPEED:
        status = DeviceHandleSetSpeed(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_AACS_READ_MEDIA_KEY_BLOCK_SIZE:
    case IOCTL_AACS_READ_MEDIA_KEY_BLOCK:
        status = DeviceHandleAacsReadMediaKeyBlock(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_AACS_START_SESSION:
        status = DeviceHandleAacsStartSession(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_AACS_END_SESSION:
        status = DeviceHandleAacsEndSession(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_AACS_SEND_CERTIFICATE:
        status = DeviceHandleAacsSendCertificate(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_AACS_GET_CERTIFICATE:
        status = DeviceHandleAacsGetCertificate(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_AACS_GET_CHALLENGE_KEY:
        status = DeviceHandleAacsGetChallengeKey(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_AACS_SEND_CHALLENGE_KEY:
        status = DeviceHandleSendChallengeKey(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_AACS_READ_VOLUME_ID:
        status = DeviceHandleReadVolumeId(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_AACS_READ_SERIAL_NUMBER:
        status = DeviceHandleAacsReadSerialNumber(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_AACS_READ_MEDIA_ID:
        status = DeviceHandleAacsReadMediaId(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_AACS_READ_BINDING_NONCE:
        status = DeviceHandleAacsReadBindingNonce(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_AACS_GENERATE_BINDING_NONCE:
        status = DeviceHandleAacsGenerateBindingNonce(DeviceExtension, Request, requestParameters, &information);
        break;

    case IOCTL_CDROM_ENABLE_STREAMING:
        status = RequestHandleEnableStreaming(DeviceExtension, Request, &information);
        break;

    case IOCTL_CDROM_SEND_OPC_INFORMATION:
        status = RequestHandleSendOpcInformation(DeviceExtension, Request, &information);
        break;

    case IOCTL_CDROM_GET_PERFORMANCE:
        status = RequestHandleGetPerformance(DeviceExtension, Request, requestParameters, &information);
        break;

    // This IOCTL is a fake one, used for MCN process sync-ed with serial queue.
    case IOCTL_MCN_SYNC_FAKE_IOCTL:
        status = RequestHandleMcnSyncFakeIoctl(DeviceExtension, &information);
        break;

    case IOCTL_STORAGE_MEDIA_REMOVAL:
    case IOCTL_STORAGE_EJECTION_CONTROL:
    {
        status = RequestHandleEjectionControl(DeviceExtension, Request, requestParameters, &information);

        break;
    }

    case IOCTL_STORAGE_EJECT_MEDIA:
    case IOCTL_STORAGE_LOAD_MEDIA:
    case IOCTL_STORAGE_LOAD_MEDIA2:
    {
        status = RequestHandleLoadEjectMedia(DeviceExtension, Request, requestParameters, &information);

        break;
    }

    case IOCTL_STORAGE_MCN_CONTROL:
    {
        status = RequestHandleMcnControl(DeviceExtension, Request, &information);

        break;
    }

    case IOCTL_STORAGE_RESERVE:
    case IOCTL_STORAGE_RELEASE:
    {
        status = RequestHandleReserveRelease(DeviceExtension, Request, requestParameters, &information);

        break;
    }

    case IOCTL_STORAGE_PERSISTENT_RESERVE_IN:
    case IOCTL_STORAGE_PERSISTENT_RESERVE_OUT:
    {
        status = RequestHandlePersistentReserve(DeviceExtension, Request, requestParameters, &information);

        break;
    }

#if (NTDDI_VERSION >= NTDDI_WIN8)
    case IOCTL_DISK_ARE_VOLUMES_READY:
    {
        status = RequestHandleAreVolumesReady(DeviceExtension, Request, requestParameters, &information);

        completeRequest = FALSE;

        break;
    }

    case IOCTL_VOLUME_ONLINE:
    case IOCTL_VOLUME_POST_ONLINE:
    {
        status = RequestHandleVolumeOnline(DeviceExtension, Request, requestParameters, &information);

        break;
    }
#endif

    default:
    {
        status = STATUS_ACCESS_DENIED;
        break;
    }
    } // end of switch(ioctl)

    if (completeRequest)
    {
        RequestCompletion(DeviceExtension, Request, status, information);
    }

    return status;
}

VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
SequentialQueueEvtCanceledOnQueue(
    _In_ WDFQUEUE   Queue,
    _In_ WDFREQUEST Request
    )
/*++
Routine Description:

    Perform cancellation when request is still in queue.

    If request is sych-ed in another thread, signal the event to let that thread be able to complete the request.
    Otherwise, complete the request.

Arguments:

    Queue - serial queue
    Request - handle to the incoming WDF Request object

Return Value:

    None

--*/
{
    PCDROM_REQUEST_CONTEXT  requestContext = RequestGetContext(Request);

    if (requestContext->SyncRequired)
    {
        KeSetEvent(requestContext->SyncEvent, IO_CD_ROM_INCREMENT, FALSE);
    }
    else
    {
        PCDROM_DEVICE_EXTENSION deviceExtension = NULL;
        WDFDEVICE               device = WdfIoQueueGetDevice(Queue);

        deviceExtension = DeviceGetExtension(device);

        RequestCompletion(deviceExtension, Request, STATUS_CANCELLED, 0);

    }

    return;
}


NTSTATUS
RequestSynchronizeProcessWithSerialQueue(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request
    )
/*++
Routine Description:

    This is the mechanism to sync a request process in original thread with serialize queue.
    initialize a EVENT and put the request inot serialize queue;
    waiting for serialize queue processes to this request and signal the EVENT;
    call the request handler to process this request.

Arguments:

    DeviceExtension - device context

    Request - handle to the incoming WDF Request object

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PCDROM_DEVICE_EXTENSION deviceExtension = DeviceGetExtension(Device);
    PCDROM_REQUEST_CONTEXT  requestContext = RequestGetContext(Request);
    PKEVENT                 bufferToFree = requestContext->SyncEvent;

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {
        // cannot block at or above DISPATCH_LEVEL
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
            "RequestSynchronousProcessWithSerialQueue called at DISPATCH_LEVEL or above"));
        NT_ASSERT(FALSE);
        RequestCompletion(deviceExtension, Request, STATUS_INVALID_LEVEL, 0);
        return STATUS_INVALID_LEVEL;
    }

    // init the synchronization event
    KeInitializeEvent(requestContext->SyncEvent, NotificationEvent, FALSE);

    // do we still need to do something like this?
    // SET_FLAG(nextStack->Flags, SL_OVERRIDE_VERIFY_VOLUME);

    // NOTE: this mechanism relies on that KMDF will not complete request by itself.
    // Doing that will cause the syncEvent not fired thus this thread will stuck.
    // This should not really happen: our EvtCanceledOnQueue callbacks should be
    // called even if queues are purged for some reason. The only case when these
    // callbacks are not called is when a request is owned by the driver (i.e. has
    // already been passed to one of the registered handlers). In this case, it is
    // our responsibility to cancel such requests properly.
    status = WdfDeviceEnqueueRequest(Device, Request);

    if (!NT_SUCCESS(status))
    {
        // Failed to forward request! Pretend the sync event already occured, otherwise we'll hit
        // an assert in RequestEvtCleanup.
        KeSetEvent(requestContext->SyncEvent, IO_CD_ROM_INCREMENT, FALSE);
        RequestCompletion(deviceExtension, Request, status, WdfRequestGetInformation(Request));
    }
    else
    {
        NTSTATUS                waitStatus = STATUS_UNSUCCESSFUL;
        PCDROM_DATA             cdData = &(deviceExtension->DeviceAdditionalData);
        BOOLEAN                 fCallSyncCallback = FALSE;
        PIRP                    irp = WdfRequestWdmGetIrp(Request);

        // ok, now wait on the event
        while (waitStatus != STATUS_SUCCESS)
        {
            waitStatus = KeWaitForSingleObject(requestContext->SyncEvent, Executive, KernelMode, TRUE, NULL);
            if (waitStatus == STATUS_SUCCESS) // must check equality -- STATUS_ALERTED is success code
            {
                // do nothing
            }
            else if (waitStatus != STATUS_ALERTED)
            {
                // do nothing
                TracePrint((TRACE_LEVEL_FATAL, TRACE_FLAG_IOCTL,
                            "Request %p on device object %p had a non-alert, non-success result from wait (%!HRESULT!)\n",
                            Request, Device, waitStatus));
                NT_ASSERT(FALSE);
            }
            else if (PsIsThreadTerminating(PsGetCurrentThread()))
            {
                // the thread was alerted and is terminating, so cancel the irp
                // this will cause EvtIoCanceledOnQueue to be called, which will signal the event,
                // so we will get out of the while loop and eventually complete the request.
                if (IoCancelIrp(irp))
                {
                    // cancellation routine was called
                    TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                                "Sychronize Ioctl: request %p cancelled from device %p\n",
                                Request, Device));
                }
                else
                {
                    TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                                "Sychronize Ioctl: request %p could not be cancelled from device %p\n",
                                Request, Device));
                }
            }
            else
            {
                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                            "SPURIOUS ALERT waiting for Request %p on device %p (%!STATUS!)\n",
                            Request, Device, status));
            }
        } // end of wait loop on the event

        // because we've waited an unknown amount of time, should check
        // the cancelled flag to immediately fail the irp as appropriate
        if (WdfRequestIsCanceled(Request))
        {
            // the request was cancelled, thus we should always stop
            // processing here if possible.
            status = STATUS_CANCELLED;
            RequestCompletion(deviceExtension, Request, status, 0);
        }
        else if (EXCLUSIVE_MODE(cdData) && !EXCLUSIVE_OWNER(cdData, WdfRequestGetFileObject(Request)))
        {
            WDF_REQUEST_PARAMETERS  requestParameters;
            BOOLEAN                 isBlocked = FALSE;

            // get the request parameters
            WDF_REQUEST_PARAMETERS_INIT(&requestParameters);
            WdfRequestGetParameters(Request, &requestParameters);

            status = RequestIsIoctlBlockedByExclusiveAccess(Request, &isBlocked);
            if (isBlocked)
            {
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                            "Access Denied! Device in exclusive mode.Failing Ioctl %lx\n",
                            requestParameters.Parameters.DeviceIoControl.IoControlCode));
                RequestCompletion(deviceExtension, Request, STATUS_ACCESS_DENIED, 0);
            }
            else
            {
                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                            "Ioctl %lx not blocked by cdrom being in exclusive mode\n",
                            requestParameters.Parameters.DeviceIoControl.IoControlCode));
                fCallSyncCallback = TRUE;
            }
        }
        else
        {
            fCallSyncCallback = TRUE;
        }

        if (fCallSyncCallback)
        {
            // Synchronization completed successfully.  Call the requested routine
            status = requestContext->SyncCallback(Device, Request);
        }
    }

    // The next SequentialQueue evt routine will not be triggered until the current request is completed.

    // clean up the request context setting.
    FREE_POOL(bufferToFree);

    return status;
}

NTSTATUS
RequestIsIoctlBlockedByExclusiveAccess(
    _In_  WDFREQUEST  Request,
    _Out_ PBOOLEAN    IsBlocked
    )
/*++
Routine Description:

    Check if the IOCTL request should be blocked or not according to
    the exclusive lock stat.

Arguments:

    Request - handle to the incoming WDF Request object

Return Value:

    NTSTATUS

    IsBlocked - TRUE (be blocked); FALSE (not blocked)

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    ULONG                   ioctlCode = 0;
    ULONG                   baseCode = 0;
    WDF_REQUEST_PARAMETERS  requestParameters;

    // Get the Request parameters
    WDF_REQUEST_PARAMETERS_INIT(&requestParameters);
    WdfRequestGetParameters(Request, &requestParameters);

    // check and initialize parameter
    if (IsBlocked == NULL)
    {
        //This is an internal function and this parameter must be supplied.
        NT_ASSERT(FALSE);

        return STATUS_INVALID_PARAMETER;
    }
    else
    {
        *IsBlocked = FALSE;
    }

    // check if this is an IOCTL
    if ((requestParameters.Type == WdfRequestTypeDeviceControl) ||
        (requestParameters.Type == WdfRequestTypeDeviceControlInternal))
    {
        //
        // Allow minimum set of commands that are required for the disk manager
        // to show the CD device, while in exclusive mode.
        // Note: These commands should not generate any requests to the device,
        //       and thus must be handled directly in StartIO during exclusive
        //       access (except for the exclusive owner, of course).
        //
        ioctlCode = requestParameters.Parameters.DeviceIoControl.IoControlCode;
        baseCode = DEVICE_TYPE_FROM_CTL_CODE(ioctlCode);

        if (ioctlCode == IOCTL_SCSI_GET_ADDRESS           ||
            ioctlCode == IOCTL_STORAGE_GET_HOTPLUG_INFO   ||
            ioctlCode == IOCTL_STORAGE_GET_DEVICE_NUMBER  ||
            ioctlCode == IOCTL_STORAGE_GET_MEDIA_TYPES_EX ||
            ioctlCode == IOCTL_CDROM_EXCLUSIVE_ACCESS     ||
            ioctlCode == IOCTL_CDROM_GET_INQUIRY_DATA
            )
        {
            *IsBlocked = FALSE;
        }

        //
        // Handle IOCTL_STORAGE_QUERY_PROPERTY special because:
        //  (1) PropertyExistsQuery should not generate device i/o
        //  (2) Queries for StorageDeviceProperty and StorageAdapterDescriptor
        //      will return cache'd data
    else if (ioctlCode == IOCTL_STORAGE_QUERY_PROPERTY)
        {
            PSTORAGE_PROPERTY_QUERY query = NULL;
            status = WdfRequestRetrieveInputBuffer(Request,
                                                   requestParameters.Parameters.DeviceIoControl.InputBufferLength,
                                                   (PVOID*)&query,
                                                   NULL);

            if (NT_SUCCESS(status))
            {
                if (query != NULL)
                {
                    if (query->QueryType == PropertyExistsQuery)
                    {
                        *IsBlocked = FALSE;
                    }
                    else if ((query->QueryType == PropertyStandardQuery) &&
                             ((query->PropertyId == StorageDeviceProperty) ||
                              (query->PropertyId == StorageAdapterProperty)))
                    {
                        *IsBlocked = FALSE;
                    }
                }
            }
        }

        // Return TRUE for unknown IOCTLs with STORAGE bases
    else if (baseCode == IOCTL_SCSI_BASE    ||
            baseCode == IOCTL_DISK_BASE    ||
            baseCode == IOCTL_CDROM_BASE   ||
            baseCode == IOCTL_STORAGE_BASE ||
            baseCode == IOCTL_DVD_BASE     )
        {
            *IsBlocked = TRUE;
        }
    }
    else
    {
        // this should only be called with an IOCTL
        NT_ASSERT(FALSE);

        status = STATUS_INVALID_PARAMETER;
    }

    return status;
}

BOOLEAN
DeviceIsMmcUpdateRequired(
    _In_ WDFDEVICE    Device
    )
/*++
Routine Description:

    Check if the device needs to update its MMC information.

Arguments:

    Device - device to be checked.

Return Value:

    TRUE (require update); FALSE (not require update)

--*/
{
    PCDROM_DEVICE_EXTENSION deviceExtension = DeviceGetExtension(Device);
    PCDROM_DATA             cdData = &(deviceExtension->DeviceAdditionalData);

    if ((cdData->Mmc.IsMmc) &&
        (cdData->Mmc.UpdateState == CdromMmcUpdateRequired))
    {
        return TRUE;
    }
    else
    {
        // no update required: just proceed
        return FALSE;
    }
}

VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
RequestEvtCleanup(
    _In_ WDFOBJECT Request
    )
/*++
Routine Description:

    Request cleanup callback.

Arguments:

    Request - request to clean up.

Return Value:

    None

--*/
{
    WDFREQUEST              request = (WDFREQUEST)Request;
    PCDROM_REQUEST_CONTEXT  requestContext = RequestGetContext(request);

    if (requestContext->SyncRequired)
    {
        // the event should have been signaled, just check that
        NT_ASSERT(KeReadStateEvent(requestContext->SyncEvent) != 0);
    }
}

