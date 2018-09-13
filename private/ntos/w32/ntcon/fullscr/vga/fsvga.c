/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    fsvga.c

Abstract:

    This is the console fullscreen driver for the VGA card.

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "stdarg.h"
#include "stdio.h"
#include "ntddk.h"
#include "fsvga.h"
#include "fsvgalog.h"

//
// Use the alloc_text pragma to specify the driver initialization routines
// (they can be paged out).
//

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(INIT,FsVgaConfiguration)
#pragma alloc_text(INIT,FsVgaPeripheralCallout)
#pragma alloc_text(INIT,FsVgaServiceParameters)
#pragma alloc_text(INIT,FsVgaBuildResourceList)
#endif


//
// Declare the global debug flag for this driver.
//

#if DBG
ULONG FsVgaDebug = 0;
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - Pointer to driver object created by system.

    RegistryPath - Pointer to the Unicode name of the registry path
        for this driver.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    PDEVICE_OBJECT DeviceObject = NULL;
    PDEVICE_EXTENSION deviceExtension = NULL;
    DEVICE_EXTENSION tmpDeviceExtension;
    NTSTATUS status = STATUS_SUCCESS;
    PCM_RESOURCE_LIST resources = NULL;
    ULONG resourceListSize = 0;
    BOOLEAN overrideConflict;
    BOOLEAN conflictDetected;
    ULONG addressSpace;
    PHYSICAL_ADDRESS cardAddress;
    ULONG i;
    UNICODE_STRING fullFsVgaName;
    UNICODE_STRING baseFsVgaName;
    UNICODE_STRING deviceNameSuffix;
    UNICODE_STRING resourceDeviceClass;
    UNICODE_STRING registryPath;

#define NAME_MAX 256
    WCHAR FsVgaBuffer[NAME_MAX];

    ULONG uniqueErrorValue;
    NTSTATUS errorCode = STATUS_SUCCESS;
    ULONG dumpCount = 0;

#define DUMP_COUNT 4
    ULONG dumpData[DUMP_COUNT];

    FsVgaPrint((1,
                "\n\nFSVGA-FSVGAInitialize: enter\n"));

    //
    // Zero-initialize various structures.
    //
    RtlZeroMemory(&tmpDeviceExtension, sizeof(DEVICE_EXTENSION));

    fullFsVgaName.MaximumLength = 0;
    fullFsVgaName.Length = 0;
    deviceNameSuffix.MaximumLength = 0;
    deviceNameSuffix.Length = 0;
    resourceDeviceClass.MaximumLength = 0;
    resourceDeviceClass.Length = 0;
    registryPath.MaximumLength = 0;
    registryPath.Length = 0;

    RtlZeroMemory(FsVgaBuffer, NAME_MAX * sizeof(WCHAR));
    baseFsVgaName.Buffer = FsVgaBuffer;
    baseFsVgaName.Length = 0;
    baseFsVgaName.MaximumLength = NAME_MAX * sizeof(WCHAR);

    RtlZeroMemory(dumpData, sizeof(dumpData));

    //
    // Need to ensure that the registry path is null-terminated.
    // Allocate pool to hold a null-terminated copy of the path.
    //
    registryPath.Buffer = ExAllocatePool(PagedPool,
                                         RegistryPath->Length + sizeof(UNICODE_NULL));
    if (!registryPath.Buffer)
    {
        FsVgaPrint((1,
                    "FSVGA-FSVGAInitialize: Couldn't allocate pool for registry path\n"));
        status = STATUS_UNSUCCESSFUL;
        errorCode = FSVGA_INSUFFICIENT_RESOURCES;
        uniqueErrorValue = FSVGA_ERROR_VALUE_BASE + 2;
        dumpData[0] = (ULONG) RegistryPath->Length + sizeof(UNICODE_NULL);
        dumpCount = 1;
        goto FsVgaInitializeExit;
    }
    else
    {
        registryPath.Length = RegistryPath->Length + sizeof(UNICODE_NULL);
        registryPath.MaximumLength = registryPath.Length;

        RtlZeroMemory(registryPath.Buffer,
                      registryPath.Length);
        RtlMoveMemory(registryPath.Buffer,
                      RegistryPath->Buffer,
                      RegistryPath->Length);
    }

    //
    // Get the configuration information for this driver.
    //

    FsVgaConfiguration(&tmpDeviceExtension,
                       &registryPath,
                       &baseFsVgaName);

    if (!(tmpDeviceExtension.HardwarePresent & FSVGA_HARDWARE_PRESENT)) {

        //
        // There is neither a Full Screen Video attached.  Free
        // resources and return with unsuccessful status.
        //

        FsVgaPrint((1,
                    "FSVGA-FsVgaInitialize: No Full Screen Video attached.\n"));
        status = STATUS_NO_SUCH_DEVICE;
        errorCode = FSVGA_NO_SUCH_DEVICE;
        uniqueErrorValue = FSVGA_ERROR_VALUE_BASE + 4;
        goto FsVgaInitializeExit;

    }

    //
    // Set up space for the port's device object suffix.  Note that
    // we overallocate space for the suffix string because it is much
    // easier than figuring out exactly how much space is required.
    // The storage gets freed at the end of driver initialization, so
    // who cares...
    //

    RtlInitUnicodeString(&deviceNameSuffix,
                         NULL);

    deviceNameSuffix.MaximumLength = FULLSCREEN_VIDEO_SUFFIX_MAXIMUM * sizeof(WCHAR);
    deviceNameSuffix.MaximumLength += sizeof(UNICODE_NULL);

    deviceNameSuffix.Buffer = ExAllocatePool(PagedPool,
                                             deviceNameSuffix.MaximumLength);
    if (!deviceNameSuffix.Buffer) {
        FsVgaPrint((1,
                    "FSVGA-FsVgaInitialize: Couldn't allocate string for device object suffix\n"));

        status = STATUS_UNSUCCESSFUL;
        errorCode = FSVGA_INSUFFICIENT_RESOURCES;
        uniqueErrorValue = FSVGA_ERROR_VALUE_BASE + 8;
        dumpData[0] = (ULONG) deviceNameSuffix.MaximumLength;
        dumpCount = 1;
        goto FsVgaInitializeExit;
    }

    RtlZeroMemory(deviceNameSuffix.Buffer,
                  deviceNameSuffix.MaximumLength);

    //
    // Set up space for the port's Full Screen Video device object name.
    //

    RtlInitUnicodeString(&fullFsVgaName,
                         NULL);

    fullFsVgaName.MaximumLength = sizeof(L"\\Device\\") +
                                      baseFsVgaName.Length +
                                      deviceNameSuffix.MaximumLength;

    fullFsVgaName.Buffer = ExAllocatePool(PagedPool,
                                          fullFsVgaName.MaximumLength);
    if (!fullFsVgaName.Buffer) {
        FsVgaPrint((1,
                    "FSVGA-FsVgaInitialize: Couldn't allocate string for Full Screen Video device object name\n"));

        status = STATUS_UNSUCCESSFUL;
        errorCode = FSVGA_INSUFFICIENT_RESOURCES;
        uniqueErrorValue = FSVGA_ERROR_VALUE_BASE + 10;
        dumpData[0] = (ULONG) fullFsVgaName.MaximumLength;
        dumpCount = 1;
        goto FsVgaInitializeExit;

    }

    RtlZeroMemory(fullFsVgaName.Buffer,
                  fullFsVgaName.MaximumLength);
    RtlAppendUnicodeToString(&fullFsVgaName,
                             L"\\Device\\");
    RtlAppendUnicodeToString(&fullFsVgaName,
                             baseFsVgaName.Buffer);

    //
    // Append the suffix to the device object name string.  E.g., turn
    // \Device\FullScreenVideo into \Device\FullScreenVideo0.  Then we attempt
    // to create the device object.  If the device object already
    // exists (because it was already created by another port driver),
    // increment the suffix and try again.
    //

    status = RtlIntegerToUnicodeString( 0,  // suffix number = 0
                                       10,
                                       &deviceNameSuffix);
    if (!NT_SUCCESS(status))
    {
        FsVgaPrint((1,
                    "FSVGA-FsVgaInitialize: Could not create suffix number\n"));
        errorCode = FSVGA_INSUFFICIENT_RESOURCES;
        uniqueErrorValue = FSVGA_ERROR_VALUE_BASE + 12;
        dumpData[0] = (ULONG) 0; // suffix number = 0
        dumpCount = 1;
        goto FsVgaInitializeExit;
    }

    RtlAppendUnicodeStringToString(&fullFsVgaName,
                                   &deviceNameSuffix);

    FsVgaPrint((1,
                "FSVGA-FSVGAInitialize: Creating device object named %ws\n",
                fullFsVgaName.Buffer));

    //
    // Create device object for the FsVga device.
    // Note that we specify that this is a exclusive device.
    //

    status = IoCreateDevice(DriverObject,
                            sizeof(DEVICE_EXTENSION),
                            &fullFsVgaName,
                            FILE_DEVICE_FULLSCREEN_VIDEO,
                            0,
                            TRUE,
                            &DeviceObject);
    if (!NT_SUCCESS(status))
    {
        FsVgaPrint((1,
                    "FSVGA-FSVGAInitialize: Couldn't create device object = %ws\n"));
        status = STATUS_UNSUCCESSFUL;
        errorCode = FSVGA_INSUFFICIENT_RESOURCES;
        uniqueErrorValue = FSVGA_ERROR_VALUE_BASE + 2;
        dumpData[0] = (ULONG) RegistryPath->Length + sizeof(UNICODE_NULL);
        dumpCount = 1;
        goto FsVgaInitializeExit;
    }

    //
    // Set up the device extension.
    //

    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    *deviceExtension = tmpDeviceExtension;
    deviceExtension->DeviceObject = DeviceObject;

    //
    // Set up the resource list prior to reporting resource usage.
    //

    FsVgaBuildResourceList(deviceExtension, &resources, &resourceListSize);

    //
    // Set up space for the resource device class name.
    //

    RtlInitUnicodeString(&resourceDeviceClass,
                         NULL);

    resourceDeviceClass.MaximumLength = baseFsVgaName.Length;

    resourceDeviceClass.Buffer = ExAllocatePool(PagedPool,
                                                resourceDeviceClass.MaximumLength);

    if (!resourceDeviceClass.Buffer) {

        FsVgaPrint((1,
                    "FSVGA-FsVgaInitialize: Couldn't allocate string for resource device class name\n"));

        status = STATUS_UNSUCCESSFUL;
        errorCode = FSVGA_INSUFFICIENT_RESOURCES;
        uniqueErrorValue = FSVGA_ERROR_VALUE_BASE + 15;
        dumpData[0] = (ULONG) resourceDeviceClass.MaximumLength;
        dumpCount = 1;
        goto FsVgaInitializeExit;

    }

    //
    // Form the resource device class name from the Full Screen Video
    // device names.
    //

    RtlZeroMemory(resourceDeviceClass.Buffer,
                  resourceDeviceClass.MaximumLength);
    RtlAppendUnicodeStringToString(&resourceDeviceClass,
                                   &baseFsVgaName);

    //
    // Report resource usage for the registry.
    //

        //
        // If we are loading the VGA, do not generate an error if it conflicts
        // with another driver.
        //
        // overrideConflict = pOverrideConflict(DeviceExtension, TRUE);
        overrideConflict = TRUE;
#if DBG
        if (overrideConflict) {
            FsVgaPrint((2, "We are checking the vga driver resources\n"));
        } else {
            FsVgaPrint((2, "We are NOT checking vga driver resources\n"));
        }
#endif

    status = IoReportResourceUsage(&resourceDeviceClass,
                                   DriverObject,
                                   NULL,
                                   0,
                                   DeviceObject,
                                   resources,
                                   resourceListSize,
                                   overrideConflict,
                                   &conflictDetected
                                   );

        //
        // If we tried to override the conflict, let's take a look a what
        // we want to do with the result
        //

        if ((NT_SUCCESS(status)) &&
            overrideConflict &&
            conflictDetected) {

            //
            // For cases like DetectDisplay, a conflict is bad and we do
            // want to fail.
            //
            // In the case of Basevideo, a conflict is possible.  But we still
            // want to load the VGA anyways. Return success and reset the
            // conflict flag !
            //
            // pOverrideConflict with the FALSE flag will check that.
            //
            //
            // if (pOverrideConflict(DeviceExtension, FALSE)) {
            //     error
            // }
            // else {

                conflictDetected = FALSE;

            // }
        }


    if (conflictDetected) {

        //
        // Some other device already owns the Full Screen Video ports.
        // Fatal error.
        //

        FsVgaPrint((1,
                    "FSVGA-FsVgaInitialize: Resource usage conflict\n"));

        //
        // Set up error log info.
        //

        status = STATUS_INSUFFICIENT_RESOURCES;
        errorCode = FSVGA_RESOURCE_CONFLICT;
        uniqueErrorValue = FSVGA_ERROR_VALUE_BASE + 20;
        dumpData[0] =  (ULONG)
            resources->List[0].PartialResourceList.PartialDescriptors[0].u.Port.Start.LowPart;
        dumpData[1] =  (ULONG)
            resources->List[0].PartialResourceList.PartialDescriptors[1].u.Port.Start.LowPart;
        dumpData[2] =  (ULONG)
            resources->List[0].PartialResourceList.PartialDescriptors[2].u.Port.Start.LowPart;
        dumpCount = 3;

        goto FsVgaInitializeExit;

    }

    //
    // Map the VGA controller registers.
    //

    for (i = 0; i < deviceExtension->Configuration.PortListCount; i++) {

        addressSpace = (deviceExtension->Configuration.PortList[i].Flags
                           & CM_RESOURCE_PORT_IO) == CM_RESOURCE_PORT_IO? 1:0;

        if (!HalTranslateBusAddress(deviceExtension->Configuration.InterfaceType,
                                    deviceExtension->Configuration.BusNumber,
                                    deviceExtension->Configuration.PortList[i].u.Port.Start,
                                    &addressSpace,
                                    &cardAddress
                                   )) {

            addressSpace = 1;
            cardAddress.QuadPart = 0;
        }

        if (!addressSpace) {

            deviceExtension->UnmapRegistersRequired = TRUE;
            deviceExtension->DeviceRegisters[i] =
                MmMapIoSpace(
                    cardAddress,
                    deviceExtension->Configuration.PortList[i].u.Port.Length,
                    FALSE
                    );

        } else {

            deviceExtension->UnmapRegistersRequired = FALSE;
            deviceExtension->DeviceRegisters[i] = (PVOID)cardAddress.LowPart;

        }

        if (!deviceExtension->DeviceRegisters[i]) {

            FsVgaPrint((1,
                        "FSVGA-FsVgaInitialize: Couldn't map the device registers.\n"));
            status = STATUS_NONE_MAPPED;

            //
            // Set up error log info.
            //

            errorCode = FSVGA_REGISTERS_NOT_MAPPED;
            uniqueErrorValue = FSVGA_ERROR_VALUE_BASE + 30;
            dumpData[0] = cardAddress.LowPart;
            dumpCount = 1;

            goto FsVgaInitializeExit;

        }
    }



    //
    // Once initialization is finished, load the device map information
    // into the registry so that setup can determine which full screen
    // port are active.
    //

    if (deviceExtension->HardwarePresent & FSVGA_HARDWARE_PRESENT) {

        status = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP,
                                       baseFsVgaName.Buffer,
                                       fullFsVgaName.Buffer,
                                       REG_SZ,
                                       registryPath.Buffer,
                                       registryPath.Length);

        if (!NT_SUCCESS(status))
        {
            FsVgaPrint((1,
                       "FSVGA-FSVGAInitialize: Could not store keyboard name in DeviceMap\n"));
            errorCode = FSVGA_NO_DEVICEMAP_CREATED;
            uniqueErrorValue = FSVGA_ERROR_VALUE_BASE + 90;
            dumpCount = 0;
            goto FsVgaInitializeExit;
        }
        else
        {
            FsVgaPrint((1,
                       "FSVGA-FSVGAInitialize: Stored pointer name in DeviceMap\n"));
        }
    }

    ASSERT(status == STATUS_SUCCESS);

    //
    // Set up the device driver entry points.
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE] = FsVgaOpenCloseDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]  = FsVgaOpenCloseDispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = FsVgaDeviceControl;

FsVgaInitializeExit:

    if (errorCode != STATUS_SUCCESS)
    {
        PIO_ERROR_LOG_PACKET errorLogEntry;
        ULONG i;

        //
        // Log an error/warning message.
        //
        errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
                (DeviceObject == NULL) ? (PVOID) DriverObject : (PVOID) DeviceObject,
                (UCHAR) (sizeof(IO_ERROR_LOG_PACKET)
                         + (dumpCount * sizeof(ULONG)))
                );

        if (errorLogEntry != NULL)
        {
            errorLogEntry->ErrorCode = errorCode;
            errorLogEntry->DumpDataSize = (USHORT) dumpCount * sizeof(ULONG);
            errorLogEntry->SequenceNumber = 0;
            errorLogEntry->MajorFunctionCode = 0;
            errorLogEntry->IoControlCode = 0;
            errorLogEntry->RetryCount = 0;
            errorLogEntry->UniqueErrorValue = uniqueErrorValue;
            errorLogEntry->FinalStatus = status;
            for (i = 0; i < dumpCount; i++)
                errorLogEntry->DumpData[i] = dumpData[i];

            IoWriteErrorLogEntry(errorLogEntry);
        }
    }

    if (!NT_SUCCESS(status))
    {
        //
        // The initialization failed.  Cleanup resources before exiting.
        //

        if (resources) {

            //
            // Call IoReportResourceUsage to remove the resources from
            // the map.
            //

            resources->Count = 0;

            IoReportResourceUsage(&resourceDeviceClass,
                                  DriverObject,
                                  NULL,
                                  0,
                                  DeviceObject,
                                  resources,
                                  resourceListSize,
                                  FALSE,
                                  &conflictDetected
                                 );

        }

        if (deviceExtension) {

            if (deviceExtension->UnmapRegistersRequired) {
                for (i = 0;
                     i < deviceExtension->Configuration.PortListCount; i++){
                    if (deviceExtension->DeviceRegisters[i]) {
                        MmUnmapIoSpace(
                            deviceExtension->DeviceRegisters[i],
                            deviceExtension->Configuration.PortList[i].u.Port.Length);
                    }
                }
            }
        }

        if (DeviceObject) {
            IoDeleteDevice(DeviceObject);
        }
    }

    //
    // Free the resource list.
    //
    // N.B.  If we ever decide to hang on to the resource list instead,
    //       we need to allocate it from non-paged pool (it is now paged pool).
    //

    if (resources) {
        ExFreePool(resources);
    }

    //
    // Free the unicode strings for device names.
    //
    if (deviceNameSuffix.MaximumLength != 0)
        ExFreePool(deviceNameSuffix.Buffer);
    if (fullFsVgaName.MaximumLength != 0)
        ExFreePool(fullFsVgaName.Buffer);
    if (resourceDeviceClass.MaximumLength != 0)
        ExFreePool(resourceDeviceClass.Buffer);
    if (registryPath.MaximumLength != 0)
        ExFreePool(registryPath.Buffer);

    FsVgaPrint((1,
                "FSVGA-FsVgaInitialize: exit\n"));

    return(status);
}


VOID
FsVgaConfiguration(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING FsVgaDeviceName
    )

/*++

Routine Description:

    This routine retrieves the configuration information for the keyboard.

Arguments:

    DeviceExtension - Pointer to the device extension.

    RegistryPath - Pointer to the null-terminated Unicode name of the
        registry path for this driver.

    FsVgaDeviceName - Pointer to the Unicode string that will receive
        the Full Screen Video port device name.

Return Value:

    None.  As a side-effect, may set DeviceExtension->HardwarePresent.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PFSVGA_CONFIGURATION_INFORMATION configuration;
    INTERFACE_TYPE interfaceType;
    ULONG i;

    for (i = 0; i < MaximumInterfaceType; i++)
    {

        //
        // Get the registry information for this device.
        //

        interfaceType = i;
        status = IoQueryDeviceDescription(&interfaceType,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          FsVgaPeripheralCallout,
                                          (PVOID) DeviceExtension);

        if (DeviceExtension->HardwarePresent & FSVGA_HARDWARE_PRESENT)
        {
            //
            // Get the service parameters (e.g., user-configurable number
            // of resends, polling iterations, etc.).
            //

            FsVgaServiceParameters(DeviceExtension,
                                   RegistryPath,
                                   FsVgaDeviceName);

            break;
        }
        else
        {
            FsVgaPrint((1,
                        "FSVGA-FsVgaConfiguration: IoQueryDeviceDescription for bus type %d failed\n",
                        interfaceType));
        }
    }
}

NTSTATUS
FsVgaPeripheralCallout(
    IN PVOID Context,
    IN PUNICODE_STRING PathName,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE ControllerType,
    IN ULONG ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE PeripheralType,
    IN ULONG PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    )

/*++

Routine Description:

    This is the callout routine sent as a parameter to
    IoQueryDeviceDescription.  It grabs the Display controller
    configuration information.

Arguments:

    Context - Context parameter that was passed in by the routine
        that called IoQueryDeviceDescription.

    PathName - The full pathname for the registry key.

    BusType - Bus interface type (Isa, Eisa, Mca, etc.).

    BusNumber - The bus sub-key (0, 1, etc.).

    BusInformation - Pointer to the array of pointers to the full value
        information for the bus.

    ControllerType - The controller type (should be DisplayController).

    ControllerNumber - The controller sub-key (0, 1, etc.).

    ControllerInformation - Pointer to the array of pointers to the full
        value information for the controller key.

    PeripheralType - The peripheral type (should be MonitorPeripheral).

    PeripheralNumber - The peripheral sub-key.

    PeripheralInformation - Pointer to the array of pointers to the full
        value information for the peripheral key.


Return Value:

    None.  If successful, will have the following side-effects:

        - Sets DeviceObject->DeviceExtension->HardwarePresent.
        - Sets configuration fields in
          DeviceObject->DeviceExtension->Configuration.

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    PFSVGA_CONFIGURATION_INFORMATION configuration;
    NTSTATUS status = STATUS_SUCCESS;
    CM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDescriptor;

    FsVgaPrint((1,
                "FSVGA-FsVgaPeripheralCallout: Path @ 0x%x, Bus Type 0x%x, Bus Number 0x%x\n",
                PathName, BusType, BusNumber));
    FsVgaPrint((1,
                "    Controller Type 0x%x, Controller Number 0x%x, Controller info @ 0x%x\n",
                ControllerType, ControllerNumber, ControllerInformation));
    FsVgaPrint((1,
                "    Peripheral Type 0x%x, Peripheral Number 0x%x, Peripheral info @ 0x%x\n",
                PeripheralType, PeripheralNumber, PeripheralInformation));

    //
    // If we already have the configuration information for the
    // keyboard peripheral, or if the peripheral identifier is missing,
    // just return.
    //

    deviceExtension = (PDEVICE_EXTENSION) Context;
    if (deviceExtension->HardwarePresent & FSVGA_HARDWARE_PRESENT)
    {
        return (status);
    }

    configuration = &deviceExtension->Configuration;

    deviceExtension->HardwarePresent |= FSVGA_HARDWARE_PRESENT;

    //
    // Get the bus information.
    //

    configuration->InterfaceType = BusType;
    configuration->BusNumber = BusNumber;

    //
    // Get logical IO port addresses.
    //
    ResourceDescriptor.Type = CmResourceTypePort;
    ResourceDescriptor.ShareDisposition = CmResourceShareShared;
    ResourceDescriptor.Flags = CM_RESOURCE_PORT_IO;

    ResourceDescriptor.u.Port.Start.LowPart = VGA_BASE_IO_PORT+CRTC_ADDRESS_PORT_COLOR;
    ResourceDescriptor.u.Port.Start.HighPart = 0;
    ResourceDescriptor.u.Port.Length = 1;
    configuration->PortList[configuration->PortListCount] = ResourceDescriptor;
    configuration->PortListCount += 1;

    ResourceDescriptor.u.Port.Start.LowPart = VGA_BASE_IO_PORT+CRTC_DATA_PORT_COLOR;
    ResourceDescriptor.u.Port.Start.HighPart = 0;
    ResourceDescriptor.u.Port.Length = 1;
    configuration->PortList[configuration->PortListCount] = ResourceDescriptor;
    configuration->PortListCount += 1;

    ResourceDescriptor.u.Port.Start.LowPart = VGA_BASE_IO_PORT+GRAPH_ADDRESS_PORT;
    ResourceDescriptor.u.Port.Start.HighPart = 0;
    ResourceDescriptor.u.Port.Length = 2;
    configuration->PortList[configuration->PortListCount] = ResourceDescriptor;
    configuration->PortListCount += 1;

    ResourceDescriptor.u.Port.Start.LowPart = VGA_BASE_IO_PORT+SEQ_ADDRESS_PORT;
    ResourceDescriptor.u.Port.Start.HighPart = 0;
    ResourceDescriptor.u.Port.Length = 2;
    configuration->PortList[configuration->PortListCount] = ResourceDescriptor;
    configuration->PortListCount += 1;

    return(status);
}


VOID
FsVgaServiceParameters(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING FsVgaDeviceName
    )

/*++

Routine Description:

    This routine retrieves this driver's service parameters information
    from the registry.

Arguments:

    DeviceExtension - Pointer to the device extension.

    RegistryPath - Pointer to the null-terminated Unicode name of the
        registry path for this driver.

    FsVgaDeviceName - Pointer to the Unicode string that will receive
        the Full Screen Video port device name.

Return Value:

    None.  As a side-effect, sets fields in DeviceExtension->Configuration.

--*/

{
    PFSVGA_CONFIGURATION_INFORMATION configuration;
    UNICODE_STRING parametersPath;
    PWSTR path;
    PRTL_QUERY_REGISTRY_TABLE parameters = NULL;
    USHORT queriesPlusOne = 4;
    NTSTATUS status = STATUS_SUCCESS;
#define PARAMETER_MAX 256
    ULONG EmulationMode;
    ULONG HardwareCursor;
    ULONG HardwareScroll;
    USHORT defaultEmulationMode = 0;
    USHORT defaultHardwareCursor = NO_HARDWARE_CURSOR;
    USHORT defaultHardwareScroll = NO_HARDWARE_SCROLL;
    UNICODE_STRING defaultFsVgaName;

    configuration = &DeviceExtension->Configuration;
    parametersPath.Buffer = NULL;

    //
    // Registry path is already null-terminated, so just use it.
    //

    path = RegistryPath->Buffer;

    //
    // Allocate the Rtl query table.
    //

    parameters = ExAllocatePool(PagedPool,
                                sizeof(RTL_QUERY_REGISTRY_TABLE) * queriesPlusOne);
    if (!parameters)
    {
        FsVgaPrint((1,
                    "FSVGA-FsVgaServiceParameters: Couldn't allocate table for Rtl query to parameters for %ws\n",
                    path));
        status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        RtlZeroMemory(parameters,
                      sizeof(RTL_QUERY_REGISTRY_TABLE) * queriesPlusOne);

        //
        // Form a path to this driver's Parameters subkey.
        //

        RtlInitUnicodeString(&parametersPath,
                             NULL);

        parametersPath.MaximumLength = RegistryPath->Length +
                                       sizeof(L"\\Parameters");
        parametersPath.Buffer = ExAllocatePool(PagedPool,
                                               parametersPath.MaximumLength);
        if (!parametersPath.Buffer)
        {
            FsVgaPrint((1,
                        "FSVGA-FsVgaServiceParameters: Couldn't allocate string for path to parameters for %ws\n",
                        path));
            status = STATUS_UNSUCCESSFUL;
        }
    }

    if (NT_SUCCESS(status))
    {
        //
        // Form the parameters path.
        //
        RtlZeroMemory(parametersPath.Buffer,
                      parametersPath.MaximumLength);
        RtlAppendUnicodeToString(&parametersPath,
                                 path);
        RtlAppendUnicodeToString(&parametersPath,
                                 L"\\Parameters");

        FsVgaPrint((1,
                    "FsVga-FsVgaServiceParameters: parameters path is %ws\n",
                    parametersPath.Buffer));

        //
        // Gather all of the "user specified" information from
        // the registry.
        //
        parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[0].Name = L"ConsoleFullScreen.EmulationMode";
        parameters[0].EntryContext = &EmulationMode;
        parameters[0].DefaultType = REG_DWORD;
        parameters[0].DefaultData = &defaultEmulationMode;
        parameters[0].DefaultLength = sizeof(USHORT);

        parameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[1].Name = L"ConsoleFullScreen.HardwareCursor";
        parameters[1].EntryContext = &HardwareCursor;
        parameters[1].DefaultType = REG_DWORD;
        parameters[1].DefaultData = &defaultHardwareCursor;
        parameters[1].DefaultLength = sizeof(USHORT);

        parameters[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[2].Name = L"ConsoleFullScreen.HardwareScroll";
        parameters[2].EntryContext = &HardwareScroll;
        parameters[2].DefaultType = REG_DWORD;
        parameters[2].DefaultData = &defaultHardwareScroll;
        parameters[2].DefaultLength = sizeof(USHORT);

        status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                        parametersPath.Buffer,
                                        parameters,
                                        NULL,
                                        NULL);
        if (!NT_SUCCESS(status))
        {
            FsVgaPrint((1,
                        "FsVga-FsVgaServiceParameters: RtlQueryRegistryValues failed with 0x%x\n",
                        status));
        }
    }

    if (!NT_SUCCESS(status))
    {
        //
        // Go ahead and assign driver defaults.
        //
        configuration->EmulationMode = defaultEmulationMode;
        configuration->HardwareCursor = defaultHardwareCursor;
        configuration->HardwareScroll = defaultHardwareScroll;
    }
    else
    {
        configuration->EmulationMode = (USHORT)EmulationMode;
        configuration->HardwareCursor = (USHORT)HardwareCursor;
        configuration->HardwareScroll = (USHORT)HardwareScroll;
    }

    //
    // Form the default port device names, in case they are not
    // specified in the registry.
    //

    RtlInitUnicodeString(&defaultFsVgaName,
                         DD_FULLSCREEN_PORT_BASE_NAME_U);
    RtlCopyUnicodeString(FsVgaDeviceName, &defaultFsVgaName);

    FsVgaPrint((1,
                "FsVga-FsVgaServiceParameters: Full Screen Video port base name = %ws\n",
                FsVgaDeviceName->Buffer));

    FsVgaPrint((1,
                "FsVga-FsVgaServiceParameters: Emulation Mode = %d\n",
                configuration->EmulationMode));

    FsVgaPrint((1,
                "FsVga-FsVgaServiceParameters: Hardware Cursor = %d\n",
                configuration->HardwareCursor));

    FsVgaPrint((1,
                "FsVga-FsVgaServiceParameters: Hardware Scroll = %d\n",
                configuration->HardwareScroll));

    //
    // Free the allocated memory before returning.
    //

    if (parametersPath.Buffer)
        ExFreePool(parametersPath.Buffer);
    if (parameters)
        ExFreePool(parameters);
}

VOID
FsVgaBuildResourceList(
    IN PDEVICE_EXTENSION DeviceExtension,
    OUT PCM_RESOURCE_LIST *ResourceList,
    OUT PULONG ResourceListSize
    )

/*++

Routine Description:

    Creates a resource list that is used to query or report resource usage.

Arguments:

    DeviceExtension - Pointer to the port's device extension.

    ResourceList - Pointer to a pointer to the resource list to be allocated
        and filled.

    ResourceListSize - Pointer to the returned size of the resource
        list (in bytes).

Return Value:

    None.  If the call succeeded, *ResourceList points to the built
    resource list and *ResourceListSize is set to the size (in bytes)
    of the resource list; otherwise, *ResourceList is NULL.

Note:

    Memory may be allocated here for *ResourceList. It must be
    freed up by the caller, by calling ExFreePool();

--*/

{
    ULONG count = 0;
    ULONG i = 0;
    ULONG j = 0;
#define DUMP_COUNT 4
    ULONG dumpData[DUMP_COUNT];

    count += DeviceExtension->Configuration.PortListCount;

    *ResourceListSize = sizeof(CM_RESOURCE_LIST) +
                       ((count - 1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

    *ResourceList = (PCM_RESOURCE_LIST) ExAllocatePool(PagedPool,
                                                       *ResourceListSize);

    //
    // Return NULL if the structure could not be allocated.
    // Otherwise, fill in the resource list.
    //

    if (!*ResourceList) {

        //
        // Could not allocate memory for the resource list.
        //

        FsVgaPrint((1,
                    "FSVGA-FsVgaBuildResourceList: Could not allocate resource list\n"));

        //
        // Log an error.
        //

        dumpData[0] = *ResourceListSize;
        *ResourceListSize = 0;

        FsVgaLogError(DeviceExtension->DeviceObject,
                      FSVGA_INSUFFICIENT_RESOURCES,
                      FSVGA_ERROR_VALUE_BASE + 110,
                      STATUS_INSUFFICIENT_RESOURCES,
                      dumpData,
                      1
                     );

        return;
    }

    RtlZeroMemory(*ResourceList,
                  *ResourceListSize);

    //
    // Concoct one full resource descriptor.
    //

    (*ResourceList)->Count = 1;

    (*ResourceList)->List[0].InterfaceType =
        DeviceExtension->Configuration.InterfaceType;
    (*ResourceList)->List[0].BusNumber =
        DeviceExtension->Configuration.BusNumber;

    //
    // Build the partial resource descriptors for port
    // resources from the saved values.
    //

    (*ResourceList)->List[0].PartialResourceList.Count = count;

    for (j = 0; j < DeviceExtension->Configuration.PortListCount; j++) {
        (*ResourceList)->List[0].PartialResourceList.PartialDescriptors[i++] =
            DeviceExtension->Configuration.PortList[j];
    }

}

NTSTATUS
FsVgaOpenCloseDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the dispatch routine for create/open and close requests.
    These requests complete successfully.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    UNREFERENCED_PARAMETER(DeviceObject);

    FsVgaPrint((3,"FSVGA-FsVgaOpenCloseDispatch: enter\n"));

    PAGED_CODE();

    //
    // Complete the request with successful status.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    FsVgaPrint((3,"FSVGA-FsVgaOpenCloseDispatch: exit\n"));

    return(STATUS_SUCCESS);

}

NTSTATUS
FsVgaDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for device control requests.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION irpSp;
    PVOID ioBuffer;
    ULONG inputBufferLength;
    ULONG outputBufferLength;
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS status = STATUS_SUCCESS;

    FsVgaPrint((2,"FSVGA-FsVgaDeviceControl: enter\n"));

    PAGED_CODE();

    //
    // Get a pointer to the device extension.
    //

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Initialize the returned Information field.
    //

    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get the pointer to the input/output buffer and it's length
    //

    ioBuffer = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // Case on the device control subfunction that is being performed by the
    // requestor.
    //

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_FSVIDEO_COPY_FRAME_BUFFER:
            FsVgaPrint((2, "FsVgaDeviceControl - CopyFrameBuffer\n"));
            status = FsVgaCopyFrameBuffer(deviceExtension,
                                          (PFSVIDEO_COPY_FRAME_BUFFER) ioBuffer,
                                          inputBufferLength);
            break;

        case IOCTL_FSVIDEO_WRITE_TO_FRAME_BUFFER:
            FsVgaPrint((2, "FsVgaDeviceControl - WriteToFrameBuffer\n"));
            status = FsVgaWriteToFrameBuffer(deviceExtension,
                                             (PFSVIDEO_WRITE_TO_FRAME_BUFFER) ioBuffer,
                                             inputBufferLength);
            break;

        case IOCTL_FSVIDEO_REVERSE_MOUSE_POINTER:
            FsVgaPrint((2, "FsVgaDeviceControl - ReverseMousePointer\n"));
            status = FsVgaReverseMousePointer(deviceExtension,
                                              (PFSVIDEO_REVERSE_MOUSE_POINTER) ioBuffer,
                                              inputBufferLength);
            break;

        case IOCTL_FSVIDEO_SET_CURRENT_MODE:
            FsVgaPrint((2, "FsVgaDeviceControl - SetCurrentModes\n"));
            status = FsVgaSetMode(deviceExtension,
                                  (PFSVIDEO_MODE_INFORMATION) ioBuffer,
                                  inputBufferLength);
            break;

        case IOCTL_FSVIDEO_SET_SCREEN_INFORMATION:
            FsVgaPrint((2, "FsVgaDeviceControl - SetScreenInformation\n"));
            status = FsVgaSetScreenInformation(deviceExtension,
                                               (PFSVIDEO_SCREEN_INFORMATION) ioBuffer,
                                               inputBufferLength);
            break;

        case IOCTL_FSVIDEO_SET_CURSOR_POSITION:
            FsVgaPrint((2, "FsVgaDeviceControl - SetCursorPosition\n"));
            status = FsVgaSetCursorPosition(deviceExtension,
                                            (PFSVIDEO_CURSOR_POSITION) ioBuffer,
                                            inputBufferLength);
            break;

        case IOCTL_VIDEO_SET_CURSOR_ATTR:
            FsVgaPrint((2, "FsVgaDeviceControl - SetCursorAttribute\n"));
            status = FsVgaSetCursorAttribute(deviceExtension,
                                             (PVIDEO_CURSOR_ATTRIBUTES) ioBuffer,
                                             inputBufferLength);
            break;

        default:
            FsVgaPrint((1,
                        "FSVGA-FsVgaDeviceControl: INVALID REQUEST (0x%x)\n",
                        irpSp->Parameters.DeviceIoControl.IoControlCode));

            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    FsVgaPrint((2,"FSVGA-FsVgaDeviceControl: exit\n"));

    return(status);
}

NTSTATUS
FsVgaCopyFrameBuffer(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_COPY_FRAME_BUFFER CopyFrameBuffer,
    ULONG inputBufferLength
    )

/*++

Routine Description:

    This routine copy the frame buffer.

Arguments:

    DeviceExtension - Pointer to the miniport driver's device extension.

    CopyFrameBuffer - Pointer to the structure containing the information about the copy frame buffer.

    inputBufferLength - Length of the input buffer supplied by the user.

Return Value:

    STATUS_INSUFFICIENT_BUFFER if the input buffer was not large enough
        for the input data.

    STATUS_SUCCESS if the operation completed successfully.

--*/

{
    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if (inputBufferLength < sizeof(FSVIDEO_COPY_FRAME_BUFFER)) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    if (CopyFrameBuffer->SrcScreen.nNumberOfChars != CopyFrameBuffer->DestScreen.nNumberOfChars) {
        return STATUS_INVALID_PARAMETER;
    }

    if (! (DeviceExtension->CurrentMode.VideoMode.AttributeFlags & VIDEO_MODE_GRAPHICS))
    {
        /*
         * This is the TEXT frame buffer.
         */

        ULONG OffsSrc;
        ULONG OffsDest;
        PUCHAR pFrameBuf = DeviceExtension->CurrentMode.VideoMemory.FrameBufferBase;

        OffsSrc = SCREEN_BUFFER_POINTER(CopyFrameBuffer->SrcScreen.Position.X,
                                        CopyFrameBuffer->SrcScreen.Position.Y,
                                        CopyFrameBuffer->SrcScreen.ScreenSize.X,
                                        sizeof(VGA_CHAR));

        OffsDest = SCREEN_BUFFER_POINTER(CopyFrameBuffer->DestScreen.Position.X,
                                         CopyFrameBuffer->DestScreen.Position.Y,
                                         CopyFrameBuffer->DestScreen.ScreenSize.X,
                                         sizeof(VGA_CHAR));

        RtlMoveMemory(pFrameBuf + OffsDest,
                      pFrameBuf + OffsSrc,
                      CopyFrameBuffer->SrcScreen.nNumberOfChars * sizeof(VGA_CHAR));
    }
    else
    {
        /*
         * This is the GRAPHICS frame buffer.
         */
        return FsgCopyFrameBuffer(DeviceExtension,
                                  CopyFrameBuffer,
                                  inputBufferLength);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FsVgaWriteToFrameBuffer(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_WRITE_TO_FRAME_BUFFER WriteFrameBuffer,
    ULONG inputBufferLength
    )

/*++

Routine Description:

    This routine write the frame buffer.

Arguments:

    DeviceExtension - Pointer to the miniport driver's device extension.

    WriteFrameBuffer - Pointer to the structure containing the information about the write frame buffer.

    inputBufferLength - Length of the input buffer supplied by the user.

Return Value:

    STATUS_INSUFFICIENT_BUFFER if the input buffer was not large enough
        for the input data.

    STATUS_SUCCESS if the operation completed successfully.

--*/

{
    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if (inputBufferLength < sizeof(FSVIDEO_WRITE_TO_FRAME_BUFFER)) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    if (WriteFrameBuffer->DestScreen.Position.X < 0 ||
        WriteFrameBuffer->DestScreen.Position.X > DeviceExtension->ScreenAndFont.ScreenSize.X ||
        (SHORT)(WriteFrameBuffer->DestScreen.Position.X +
                WriteFrameBuffer->DestScreen.nNumberOfChars)
                                                > DeviceExtension->ScreenAndFont.ScreenSize.X ||
        WriteFrameBuffer->DestScreen.Position.Y < 0 ||
        WriteFrameBuffer->DestScreen.Position.Y > DeviceExtension->ScreenAndFont.ScreenSize.Y) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    if (! (DeviceExtension->CurrentMode.VideoMode.AttributeFlags & VIDEO_MODE_GRAPHICS))
    {
        /*
         * This is the TEXT frame buffer.
         */

        ULONG Offs;
        PUCHAR pFrameBuf = DeviceExtension->CurrentMode.VideoMemory.FrameBufferBase;
        PCHAR_IMAGE_INFO pCharInfoUni = WriteFrameBuffer->SrcBuffer;
        PCHAR_IMAGE_INFO pCharInfoAsc;
        DWORD Length = WriteFrameBuffer->DestScreen.nNumberOfChars;
        PVOID pCapBuffer = NULL;
        ULONG cCapBuffer = 0;

        Offs = SCREEN_BUFFER_POINTER(WriteFrameBuffer->DestScreen.Position.X,
                                     WriteFrameBuffer->DestScreen.Position.Y,
                                     WriteFrameBuffer->DestScreen.ScreenSize.X,
                                     sizeof(VGA_CHAR));

        cCapBuffer = Length * sizeof(CHAR_IMAGE_INFO);
        pCapBuffer = ExAllocatePool(PagedPool, cCapBuffer);

        if (!pCapBuffer) {
            #define DUMP_COUNT 4
            ULONG dumpData[DUMP_COUNT];

            FsVgaPrint((1,
                        "FSVGA-FsVgaWriteToFrameBuffer: Could not allocate resource list\n"));
            //
            // Log an error.
            //
            dumpData[0] = cCapBuffer;
            FsVgaLogError(DeviceExtension->DeviceObject,
                          FSVGA_INSUFFICIENT_RESOURCES,
                          FSVGA_ERROR_VALUE_BASE + 200,
                          STATUS_INSUFFICIENT_RESOURCES,
                          dumpData,
                          1
                         );
            return STATUS_UNSUCCESSFUL;
        }

        TranslateOutputToOem(pCapBuffer, pCharInfoUni, Length);

        pCharInfoAsc = pCapBuffer;
        pFrameBuf += Offs;
        while (Length--)
        {
            *pFrameBuf++ = pCharInfoAsc->CharInfo.Char.AsciiChar;
            *pFrameBuf++ = (UCHAR) (pCharInfoAsc->CharInfo.Attributes);
            pCharInfoAsc++;
        }

        ExFreePool(pCapBuffer);
    }
    else
    {
        /*
         * This is the GRAPHICS frame buffer.
         */
        return FsgWriteToFrameBuffer(DeviceExtension,
                                     WriteFrameBuffer,
                                     inputBufferLength);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FsVgaReverseMousePointer(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_REVERSE_MOUSE_POINTER MouseBuffer,
    ULONG inputBufferLength
    )

/*++

Routine Description:

    This routine reverse the frame buffer for mouse pointer.

Arguments:

    DeviceExtension - Pointer to the miniport driver's device extension.

    MouseBuffer - Pointer to the structure containing the information about the mouse frame buffer.

    inputBufferLength - Length of the input buffer supplied by the user.

Return Value:

    STATUS_INSUFFICIENT_BUFFER if the input buffer was not large enough
        for the input data.

    STATUS_SUCCESS if the operation completed successfully.

--*/

{
    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if (inputBufferLength < sizeof(FSVIDEO_REVERSE_MOUSE_POINTER)) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    if (! (DeviceExtension->CurrentMode.VideoMode.AttributeFlags & VIDEO_MODE_GRAPHICS))
    {
        /*
         * This is the TEXT frame buffer.
         */

        ULONG Offs;
        PUCHAR pFrameBuf = DeviceExtension->CurrentMode.VideoMemory.FrameBufferBase;
        BYTE Attribute;

        Offs = SCREEN_BUFFER_POINTER(MouseBuffer->Screen.Position.X,
                                     MouseBuffer->Screen.Position.Y,
                                     MouseBuffer->Screen.ScreenSize.X,
                                     sizeof(VGA_CHAR));
        pFrameBuf += Offs;

        Attribute =  (*(pFrameBuf + 1) & 0xF0) >> 4;
        Attribute |= (*(pFrameBuf + 1) & 0x0F) << 4;
        *(pFrameBuf + 1) = Attribute;
    }
    else
    {
        /*
         * This is the GRAPHICS frame buffer.
         */
        return FsgReverseMousePointer(DeviceExtension,
                                      MouseBuffer,
                                      inputBufferLength);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FsVgaSetMode(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_MODE_INFORMATION ModeInformation,
    ULONG inputBufferLength
    )

/*++

Routine Description:

    This routine sets the current video information.

Arguments:

    DeviceExtension - Pointer to the miniport driver's device extension.

    ModeInformation - Pointer to the structure containing the information about the
                      full screen video.

    inputBufferLength - Length of the input buffer supplied by the user.

Return Value:

    STATUS_INSUFFICIENT_BUFFER if the input buffer was not large enough
        for the input data.

    STATUS_SUCCESS if the operation completed successfully.

--*/

{
    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if (inputBufferLength < sizeof(FSVIDEO_MODE_INFORMATION)) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    DeviceExtension->CurrentMode = *ModeInformation;

    return STATUS_SUCCESS;
}

NTSTATUS
FsVgaSetScreenInformation(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_SCREEN_INFORMATION ScreenInformation,
    ULONG inputBufferLength
    )

/*++

Routine Description:

    This routine sets the screen and font information.

Arguments:

    DeviceExtension - Pointer to the miniport driver's device extension.

    ScreenInformation - Pointer to the structure containing the information about the
                        screen anf font.

    inputBufferLength - Length of the input buffer supplied by the user.

Return Value:

    STATUS_INSUFFICIENT_BUFFER if the input buffer was not large enough
        for the input data.

    STATUS_SUCCESS if the operation completed successfully.

--*/

{
    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if (inputBufferLength < sizeof(FSVIDEO_SCREEN_INFORMATION)) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    DeviceExtension->ScreenAndFont = *ScreenInformation;

    FsgVgaInitializeHWFlags(DeviceExtension);

    return STATUS_SUCCESS;
}

NTSTATUS
FsVgaSetCursorPosition(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_CURSOR_POSITION CursorPosition,
    ULONG inputBufferLength
    )

/*++

Routine Description:

    This routine sets the cursor position.

Arguments:

    DeviceExtension - Pointer to the miniport driver's device extension.

    CursorPosition - Pointer to the structure containing the information about the
                     cursor position.

    inputBufferLength - Length of the input buffer supplied by the user.

Return Value:

    STATUS_INSUFFICIENT_BUFFER if the input buffer was not large enough
        for the input data.

    STATUS_SUCCESS if the operation completed successfully.

--*/

{
    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if (inputBufferLength < sizeof(VIDEO_CURSOR_POSITION)) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    if (DeviceExtension->CurrentMode.VideoMode.AttributeFlags & VIDEO_MODE_GRAPHICS)
    {
        FsgInvertCursor(DeviceExtension,FALSE);
    }

    DeviceExtension->EmulateInfo.CursorPosition = *CursorPosition;

    if (DeviceExtension->CurrentMode.VideoMode.AttributeFlags & VIDEO_MODE_GRAPHICS)
    {
        FsgInvertCursor(DeviceExtension,TRUE);
        return STATUS_SUCCESS;
    }
    else
    {
        /*
         * If current video mode is a TEXT MODE.
         * FSVGA.SYS didn't handling hardware cursor
         * because I don't know device of VGA.SYS or others.
         *
         * In this case, by returns STATUS_UNSUCCESSFUL, caller 
         * do DeviceIoControl to VGA miniport driver.
         */
        return STATUS_UNSUCCESSFUL;
    }
}


NTSTATUS
FsVgaSetCursorAttribute(
    PDEVICE_EXTENSION DeviceExtension,
    PVIDEO_CURSOR_ATTRIBUTES CursorAttributes,
    ULONG inputBufferLength
    )

/*++

Routine Description:

    This routine sets the cursor attributes.

Arguments:

    DeviceExtension - Pointer to the miniport driver's device extension.

    CursorAttributes - Pointer to the structure containing the information about the
                       cursor attributes.

    inputBufferLength - Length of the input buffer supplied by the user.

Return Value:

    STATUS_INSUFFICIENT_BUFFER if the input buffer was not large enough
        for the input data.

    STATUS_SUCCESS if the operation completed successfully.

--*/

{
    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if (inputBufferLength < sizeof(VIDEO_CURSOR_ATTRIBUTES)) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    if (DeviceExtension->CurrentMode.VideoMode.AttributeFlags & VIDEO_MODE_GRAPHICS)
    {
        FsgInvertCursor(DeviceExtension,FALSE);
    }

    DeviceExtension->EmulateInfo.CursorAttributes = *CursorAttributes;

    if (DeviceExtension->CurrentMode.VideoMode.AttributeFlags & VIDEO_MODE_GRAPHICS)
    {
        FsgInvertCursor(DeviceExtension,TRUE);
        return STATUS_SUCCESS;
    }
    else
    {
        /*
         * If current video mode is a TEXT MODE.
         * FSVGA.SYS didn't handling hardware cursor
         * because I don't know device of VGA.SYS or others.
         *
         * In this case, by returns STATUS_UNSUCCESSFUL, caller 
         * do DeviceIoControl to VGA miniport driver.
         */
        return STATUS_UNSUCCESSFUL;
    }
}


VOID
FsVgaLogError(
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS ErrorCode,
    IN ULONG UniqueErrorValue,
    IN NTSTATUS FinalStatus,
    IN PULONG DumpData,
    IN ULONG DumpCount
    )

/*++

Routine Description:

    This routine contains common code to write an error log entry.  It is
    called from other routines, especially FsVgaInitialize, to avoid
    duplication of code.  Note that some routines continue to have their
    own error logging code (especially in the case where the error logging
    can be localized and/or the routine has more data because there is
    and IRP).

Arguments:

    DeviceObject - Pointer to the device object.

    ErrorCode - The error code for the error log packet.

    UniqueErrorValue - The unique error value for the error log packet.

    FinalStatus - The final status of the operation for the error log packet.

    DumpData - Pointer to an array of dump data for the error log packet.

    DumpCount - The number of entries in the dump data array.


Return Value:

    None.

--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;
    ULONG i;

    errorLogEntry = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry(
                                               DeviceObject,
                                               (UCHAR)
                                               (sizeof(IO_ERROR_LOG_PACKET)
                                               + (DumpCount * sizeof(ULONG)))
                                               );

    if (errorLogEntry != NULL) {

        errorLogEntry->ErrorCode = ErrorCode;
        errorLogEntry->DumpDataSize = (USHORT) (DumpCount * sizeof(ULONG));
        errorLogEntry->SequenceNumber = 0;
        errorLogEntry->MajorFunctionCode = 0;
        errorLogEntry->IoControlCode = 0;
        errorLogEntry->RetryCount = 0;
        errorLogEntry->UniqueErrorValue = UniqueErrorValue;
        errorLogEntry->FinalStatus = FinalStatus;
        for (i = 0; i < DumpCount; i++)
            errorLogEntry->DumpData[i] = DumpData[i];

        IoWriteErrorLogEntry(errorLogEntry);
    }
}


#if DBG
VOID
FsVgaDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print routine.

Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None.

--*/

{
    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= FsVgaDebug) {

        char buffer[128];

        (VOID) vsprintf(buffer, DebugMessage, ap);

        DbgPrint(buffer);
    }

    va_end(ap);

}
#endif

