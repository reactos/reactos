/*--

Copyright (C) Microsoft Corporation. All rights reserved.

Module Name:

    init.c

Abstract:

    Initialization routines for CDROM

Environment:

    kernel mode only

Notes:


Revision History:

--*/


#include "ntddk.h"
#include "ntddstor.h"
#include "ntstrsafe.h"
#include "devpkey.h"

#include "cdrom.h"
#include "scratch.h"
#include "mmc.h"

#ifdef DEBUG_USE_WPP
#include "init.tmh"
#endif

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceInitAllocateBuffers(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceRetrieveScsiAddress(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_ PSCSI_ADDRESS           ScsiAddress
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceRetrieveDescriptorsAndTransferLength(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

_IRQL_requires_max_(APC_LEVEL)
VOID
DeviceScanSpecialDevices(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceInitMmcContext(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceGetMmcSupportInfo(
    _In_  PCDROM_DEVICE_EXTENSION   DeviceExtension,
    _Out_ PBOOLEAN                  IsMmcDevice
    );

#if (NTDDI_VERSION >= NTDDI_WIN8)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceIsPortable(
    _In_  PCDROM_DEVICE_EXTENSION   DeviceExtension,
    _Out_ PBOOLEAN                  IsPortable
    );
#endif


#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, DeviceClaimRelease)
#pragma alloc_text(PAGE, DeviceEvtSelfManagedIoInit)

#pragma alloc_text(PAGE, DeviceInitReleaseQueueContext)
#pragma alloc_text(PAGE, DeviceInitAllocateBuffers)
#pragma alloc_text(PAGE, DeviceInitPowerContext)
#pragma alloc_text(PAGE, DeviceCreateWellKnownName)
#pragma alloc_text(PAGE, DeviceRetrieveScsiAddress)
#pragma alloc_text(PAGE, DeviceRetrieveDescriptorsAndTransferLength)
#pragma alloc_text(PAGE, DeviceInitializeHotplugInfo)
#pragma alloc_text(PAGE, DeviceScanSpecialDevices)
#pragma alloc_text(PAGE, DeviceGetTimeOutValueFromRegistry)
#pragma alloc_text(PAGE, DeviceGetMmcSupportInfo)
#pragma alloc_text(PAGE, DeviceRetrieveDescriptor)
#pragma alloc_text(PAGE, DeviceRetrieveHackFlagsFromRegistry)
#pragma alloc_text(PAGE, DeviceScanForSpecial)
#pragma alloc_text(PAGE, DeviceHackFlagsScan)
#pragma alloc_text(PAGE, DeviceInitMmcContext)
#pragma alloc_text(PAGE, ScanForSpecialHandler)
#pragma alloc_text(PAGE, DeviceSetRawReadInfo)
#pragma alloc_text(PAGE, DeviceInitializeDvd)
#pragma alloc_text(PAGE, DeviceCacheDeviceInquiryData)

#if (NTDDI_VERSION >= NTDDI_WIN8)
#pragma alloc_text(PAGE, DeviceIsPortable)
#endif

#endif

#pragma warning(push)
#pragma warning(disable:4152) // nonstandard extension, function/data pointer conversion in expression
#pragma warning(disable:26000) // read overflow reported because of pointer type conversion

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceClaimRelease(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_ BOOLEAN                 Release
    )
/*++

Routine Description:

    This function claims a device in the port driver.  The port driver object
    is updated with the correct driver object if the device is successfully
    claimed.

Arguments:

    Device - The WDFDEVICE that needs to be claimed or released.

    Release - Indicates the logical unit should be released rather than claimed.

Return Value:

    Returns a status indicating success or failure of the operation.

--*/
{
    NTSTATUS                status;
    SCSI_REQUEST_BLOCK      srb = {0};
    WDF_MEMORY_DESCRIPTOR   descriptor;
    WDFREQUEST              request;
    WDF_OBJECT_ATTRIBUTES   attributes;

    PAGED_CODE();

    //Create a request
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes,
                                            CDROM_REQUEST_CONTEXT);

    status = WdfRequestCreate(&attributes,
                              DeviceExtension->IoTarget,
                              &request);

    if (NT_SUCCESS(status))
    {
        //fill up srb structure
        srb.OriginalRequest = WdfRequestWdmGetIrp(request);
        NT_ASSERT(srb.OriginalRequest != NULL);

        srb.Length = sizeof(SCSI_REQUEST_BLOCK);

        srb.Function = Release
                        ? SRB_FUNCTION_RELEASE_DEVICE
                        : SRB_FUNCTION_CLAIM_DEVICE;


        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&descriptor,
                                          &srb,
                                          sizeof(srb));

        status = WdfIoTargetSendInternalIoctlOthersSynchronously(DeviceExtension->IoTarget,
                                                                 request,
                                                                 IOCTL_SCSI_EXECUTE_NONE,
                                                                 &descriptor,
                                                                 NULL,
                                                                 NULL,
                                                                 NULL,
                                                                 NULL);

        NT_ASSERT(!TEST_FLAG(srb.SrbFlags, SRB_FLAGS_FREE_SENSE_BUFFER));

        // The request should be deleted.
        WdfObjectDelete(request);

        if (!NT_SUCCESS(status))
        {
            TracePrint((TRACE_LEVEL_FATAL, TRACE_FLAG_PNP,
                        "DeviceClaimRelease: Failed to %s device, status: 0x%X\n",
                        Release ? "Release" : "Claim",
                        status));
        }
    }
    else
    {
        TracePrint((TRACE_LEVEL_FATAL, TRACE_FLAG_PNP,
                    "DeviceClaimRelease: Failed to create request, status: 0x%X\n",
                    status));
    }

    if (Release)
    {
        // We only release the device when we don't want to manage it.
        // The failure status does not matter.
        status = STATUS_SUCCESS;
    }

    return status;
} // end DeviceClaimRelease()


NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DeviceEvtSelfManagedIoInit(
    _In_ WDFDEVICE      Device
    )
/*++

Routine Description:

    This routine is called only once after the device is added in system, so it's used to do
    hardware-dependent device initialization work and resource allocation.
    If this routine fails, DeviceEvtSelfManagedIoCleanup will be invoked by the framework.

Arguments:

    Device - Handle to device object

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PCDROM_DEVICE_EXTENSION deviceExtension = NULL;

    PAGED_CODE();

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP,
                "DeviceEvtSelfManagedIoInit: WDFDEVICE %p is being started.\n",
                Device));

    deviceExtension = DeviceGetExtension(Device);

    // 1. Set/retrieve basic information, some of the following operations may depend on it
    if (NT_SUCCESS(status))
    {
        // We do not care if this function fails, SCSI address is mainly for debugging/tracing purposes.
        (VOID) DeviceRetrieveScsiAddress(deviceExtension, &deviceExtension->ScsiAddress);
    }

    if (NT_SUCCESS(status))
    {
        status = DeviceRetrieveDescriptorsAndTransferLength(deviceExtension);
    }

    if (NT_SUCCESS(status))
    {
        // This function should be called after DeviceRetrieveDescriptorsAndTransferLength()
        // It depends on MaxTransferLenth fields.
        status = DeviceInitAllocateBuffers(deviceExtension);
    }

    // 2. The following functions depend on the allocated buffers.

    // perf re-enable after failing. Q: Is this one used by cdrom.sys?
    if (NT_SUCCESS(status))
    {
        // allow perf to be re-enabled after a given number of failed IOs
        // require this number to be at least CLASS_PERF_RESTORE_MINIMUM
        ULONG t = CLASS_PERF_RESTORE_MINIMUM;

        DeviceGetParameter(deviceExtension,
                           CLASSP_REG_SUBKEY_NAME,
                           CLASSP_REG_PERF_RESTORE_VALUE_NAME,
                           &t);
        if (t >= CLASS_PERF_RESTORE_MINIMUM)
        {
            deviceExtension->PrivateFdoData->Perf.ReEnableThreshhold = t;
        }
    }

    // 3. Retrieve information about special devices and hack flags.
    if (NT_SUCCESS(status))
    {
        DeviceRetrieveHackFlagsFromRegistry(deviceExtension);
        // scan for bad items.
        DeviceScanForSpecial(deviceExtension, CdRomBadItems, DeviceHackFlagsScan);
        // Check to see if it's a special device that needs special error process.
        DeviceScanSpecialDevices(deviceExtension);  // may send command to device
    }

    // 4. Initialize the hotplug information only after the ScanForSpecial routines,
    // as it relies upon the hack flags - deviceExtension->PrivateFdoData->HackFlags.
    if (NT_SUCCESS(status))
    {
        status = DeviceInitializeHotplugInfo(deviceExtension);
    }

    if (NT_SUCCESS(status))
    {
        // cache the device's inquiry data
        status = DeviceCacheDeviceInquiryData(deviceExtension);
        if (!NT_SUCCESS(status))
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                        "Failed to cache the device's inquiry data, failng %!STATUS!\n",
                        status
                        ));
        }
    }

    // 5. Initialize MMC context, media change notification stuff and read media capacity
    if (NT_SUCCESS(status))
    {
        status = DeviceInitializeMediaChangeDetection(deviceExtension);
    }
    if (NT_SUCCESS(status))
    {
        status = DeviceInitMmcContext(deviceExtension);
    }
    if (NT_SUCCESS(status))
    {
        status = DeviceInitializeZPODD(deviceExtension);
    }
    if (NT_SUCCESS(status))
    {
        // Do READ CAPACITY. This SCSI command returns the last sector address
        // on the device and the bytes per sector. These are used to calculate
        // the drive capacity in bytes.
        status = MediaReadCapacity(Device);

        // If READ CAPACITY succeeded, we can safely conclude that there is a media present
        if (NT_SUCCESS(status))
        {
            DeviceSetMediaChangeStateEx(deviceExtension,
                                        MediaPresent,
                                        NULL);
        }

        // READ CAPACITY is not critical for init, ignore all errors occuring during its execution
        status = STATUS_SUCCESS;
    }

    // 6. Perform DVD-specific initialization
    if (NT_SUCCESS(status))
    {
        status = DeviceInitializeDvd(Device);
    }

    // 7. Miscellaneous initialization actions
    if (NT_SUCCESS(status))
    {
        if (deviceExtension->PrivateFdoData != NULL)
        {
            deviceExtension->PrivateFdoData->Perf.OriginalSrbFlags = deviceExtension->SrbFlags;
        }

        if (deviceExtension->DeviceAdditionalData.Mmc.IsWriter)
        {
            // OPC can really take this long per IMAPIv1 timeout....
            deviceExtension->TimeOutValue = max(deviceExtension->TimeOutValue, SCSI_CDROM_OPC_TIMEOUT);
        }
    }

    // 8. Enable the main timer, create ARC name as needed
    if (NT_SUCCESS(status))
    {
        // Device successfully added and initialized, increase CdRomCount.
        IoGetConfigurationInformation()->CdRomCount++;

        deviceExtension->IsInitialized = TRUE;

        DeviceEnableMainTimer(deviceExtension);

    }

#if (NTDDI_VERSION >= NTDDI_WIN8)
    // 9. Set volume interface properties
    if (NT_SUCCESS(status))
    {
        BOOLEAN isCritical = FALSE;
        BOOLEAN isPortable = FALSE;
        BOOLEAN isRemovable = TEST_FLAG(deviceExtension->DeviceObject->Characteristics, FILE_REMOVABLE_MEDIA);
        DEVPROP_BOOLEAN propCritical = DEVPROP_FALSE;
        DEVPROP_BOOLEAN propPortable = DEVPROP_FALSE;
        DEVPROP_BOOLEAN propRemovable = DEVPROP_FALSE;

        status = DeviceIsPortable(deviceExtension, &isPortable);

        if (NT_SUCCESS(status))
        {
            if (isPortable) {
                SET_FLAG(deviceExtension->DeviceObject->Characteristics, FILE_PORTABLE_DEVICE);
            }

            propPortable = isPortable ? DEVPROP_TRUE : DEVPROP_FALSE;

            status = IoSetDeviceInterfacePropertyData(&deviceExtension->MountedDeviceInterfaceName,
                                                      &DEVPKEY_Storage_Portable,
                                                      0,
                                                      0,
                                                      DEVPROP_TYPE_BOOLEAN,
                                                      sizeof(DEVPROP_BOOLEAN),
                                                      &propPortable);
        }

        if (NT_SUCCESS(status))
        {
            propRemovable = isRemovable ? DEVPROP_TRUE : DEVPROP_FALSE;

            status = IoSetDeviceInterfacePropertyData(&deviceExtension->MountedDeviceInterfaceName,
                                                      &DEVPKEY_Storage_Removable_Media,
                                                      0,
                                                      0,
                                                      DEVPROP_TYPE_BOOLEAN,
                                                      sizeof(DEVPROP_BOOLEAN),
                                                      &propRemovable);
        }

        if (NT_SUCCESS(status))
        {
            isCritical = TEST_FLAG(deviceExtension->DeviceObject->Flags,
                                   (DO_SYSTEM_SYSTEM_PARTITION |
                                    DO_SYSTEM_BOOT_PARTITION   |
                                    DO_SYSTEM_CRITICAL_PARTITION));

            propCritical = isCritical ? DEVPROP_TRUE : DEVPROP_FALSE;

            status = IoSetDeviceInterfacePropertyData(&deviceExtension->MountedDeviceInterfaceName,
                                                      &DEVPKEY_Storage_System_Critical,
                                                      0,
                                                      0,
                                                      DEVPROP_TYPE_BOOLEAN,
                                                      sizeof(DEVPROP_BOOLEAN),
                                                      &propCritical);
        }

    }
#endif

    return status;
}


_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceInitReleaseQueueContext(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    Part of device initialize routine. Initialize ReleaseQueue related stuff.

Arguments:

    DeviceExtension - device extension of WDFDEVICE.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    WDF_OBJECT_ATTRIBUTES   attributes;

    PAGED_CODE();

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes,
                                            CDROM_REQUEST_CONTEXT);
    attributes.ParentObject = DeviceExtension->Device;

    status = WdfRequestCreate(&attributes,
                              DeviceExtension->IoTarget,
                              &(DeviceExtension->ReleaseQueueRequest));

    if (!NT_SUCCESS(status))
    {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_PNP,  "Cannot create the release queue request\n"));

        return status;
    }

    // Initialize ReleaseQueueInputMemory, a wrapper around ReleaseQueueSrb
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = DeviceExtension->ReleaseQueueRequest;

    status = WdfMemoryCreatePreallocated(&attributes,
                                         &DeviceExtension->ReleaseQueueSrb,
                                         sizeof(SCSI_REQUEST_BLOCK),
                                         &DeviceExtension->ReleaseQueueInputMemory);
    if (!NT_SUCCESS(status))
    {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_PNP,  "Failed to allocate ReleaseQueueSrb.\n"));

        return status;
    }

    // Preformat the release queue request here to ensure that this call will never
    // fail during an actual release of the queue.
    if (NT_SUCCESS(status))
    {
        status = WdfIoTargetFormatRequestForInternalIoctlOthers(DeviceExtension->IoTarget,
                                                                DeviceExtension->ReleaseQueueRequest,
                                                                IOCTL_SCSI_EXECUTE_NONE,
                                                                DeviceExtension->ReleaseQueueInputMemory,
                                                                NULL,
                                                                NULL,
                                                                NULL,
                                                                NULL,
                                                                NULL);
    }

    // Set a CompletionRoutine callback function for ReleaseQueueRequest.
    if (NT_SUCCESS(status))
    {
        WdfRequestSetCompletionRoutine(DeviceExtension->ReleaseQueueRequest,
                                       DeviceReleaseQueueCompletion,
                                       DeviceExtension->Device);
    }

    // Create a spinlock for ReleaseQueueRequest
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = DeviceExtension->Device;

    status = WdfSpinLockCreate(&attributes,
                               &(DeviceExtension->ReleaseQueueSpinLock));

    if (!NT_SUCCESS(status))
    {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_PNP,
                    "DeviceInitReleaseQueueContext: Cannot create the release queue spinlock\n"));

        return status;
    }

    // Initialize miscellaneous ReleaseQueue related fields
    DeviceExtension->ReleaseQueueNeeded = FALSE;
    DeviceExtension->ReleaseQueueInProgress = FALSE;
    DeviceExtension->ReleaseQueueSrb.Length = sizeof(SCSI_REQUEST_BLOCK);

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceInitPowerContext(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    Part of device initialize routine. Initialize PowerContext related stuff.

Arguments:

    DeviceExtension - device extension of WDFDEVICE.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    WDF_OBJECT_ATTRIBUTES   attributes;

    PAGED_CODE();

    // create request object for Power operations

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes,
                                            CDROM_REQUEST_CONTEXT);
    attributes.ParentObject = DeviceExtension->Device;

    status = WdfRequestCreate(&attributes,
                              DeviceExtension->IoTarget,
                              &(DeviceExtension->PowerContext.PowerRequest) );

    if (!NT_SUCCESS(status))
    {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_PNP,  "Cannot create the power request object.\n"));

        return status;
    }

    // Preformat the power request. With this being done, we never need to worry about
    // WdfIoTargetFormatRequestForInternalIoctlOthers ever failing later.
    status = WdfIoTargetFormatRequestForInternalIoctlOthers(DeviceExtension->IoTarget,
                                                            DeviceExtension->PowerContext.PowerRequest,
                                                            IOCTL_SCSI_EXECUTE_IN,
                                                            NULL, NULL,
                                                            NULL, NULL,
                                                            NULL, NULL);
    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceCreateWellKnownName(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine creates a symbolic link to the cdrom device object
    under \dosdevices.  The number of the cdrom device does not neccessarily
    match between \dosdevices and \device, but usually will be the same.

    Saves the buffer

Arguments:

    DeviceObject -

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    UNICODE_STRING  unicodeLinkName = {0};
    WCHAR           wideLinkName[64] = {0};
    PWCHAR          savedName;

    LONG            cdromNumber = DeviceExtension->DeviceNumber;

    PAGED_CODE();

    // if already linked, assert then return
    if (DeviceExtension->DeviceAdditionalData.WellKnownName.Buffer != NULL)
    {

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT,
                    "DeviceCreateWellKnownName: link already exists %p\n",
                    DeviceExtension->DeviceAdditionalData.WellKnownName.Buffer));

        NT_ASSERT(FALSE);

        return STATUS_UNSUCCESSFUL;
    }

    // find an unused CdRomNN to link to.
    // It's doing this way because the same might be used for other device in another driver.
    do
    {
        status = RtlStringCchPrintfW((NTSTRSAFE_PWSTR)wideLinkName,
                                     RTL_NUMBER_OF(wideLinkName),
                                     L"\\DosDevices\\CdRom%d",
                                     cdromNumber);
        if (!NT_SUCCESS(status))
        {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT,
                        "DeviceCreateWellKnownName: Format symbolic link failed with error: 0x%X\n", status));
            return status;
        }

        RtlInitUnicodeString(&unicodeLinkName, wideLinkName);

        status = WdfDeviceCreateSymbolicLink(DeviceExtension->Device,
                                             &unicodeLinkName);

        cdromNumber++;

    } while((status == STATUS_OBJECT_NAME_COLLISION) ||
            (status == STATUS_OBJECT_NAME_EXISTS));

    if (!NT_SUCCESS(status))
    {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT,
                    "DeviceCreateWellKnownName: Error %lx linking %wZ to "
                    "device %wZ\n",
                    status,
                    &unicodeLinkName,
                    &(DeviceExtension->DeviceName)));
        return status;
    }

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                "DeviceCreateWellKnownName: successfully linked %wZ "
                "to device %wZ\n",
                &unicodeLinkName,
                &(DeviceExtension->DeviceName)));

    // Save away the symbolic link name in the driver data block.  We need
    // it so we can delete the link when the device is removed.
    savedName = ExAllocatePoolWithTag(PagedPool,
                                      unicodeLinkName.MaximumLength,
                                      CDROM_TAG_STRINGS);

    if (savedName == NULL)
    {
        // Test Note: test path should excise here to see if the symbolic is deleted by framework.
        // IoDeleteSymbolicLink(&unicodeLinkName);
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                    "DeviceCreateWellKnownName: unable to allocate memory.\n"));

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(savedName, unicodeLinkName.MaximumLength);
    RtlCopyMemory(savedName, unicodeLinkName.Buffer, unicodeLinkName.MaximumLength);

    RtlInitUnicodeString(&(DeviceExtension->DeviceAdditionalData.WellKnownName), savedName);

    // the name was saved and the link created

    return STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceRetrieveScsiAddress(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_ PSCSI_ADDRESS           ScsiAddress
    )
/*++

Routine Description:

    retrieve SCSI address information and put into device extension

Arguments:

    DeviceExtension - device context.
    ScsiAddress - the buffer to put the scsi address info.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status;
    WDF_MEMORY_DESCRIPTOR   outputDescriptor;

    PAGED_CODE();

    if ((DeviceExtension == NULL) ||
        (ScsiAddress == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //Get IOTARGET for sending request to port driver.
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputDescriptor,
                                      (PVOID)ScsiAddress,
                                      sizeof(SCSI_ADDRESS));

    status = WdfIoTargetSendIoctlSynchronously(DeviceExtension->IoTarget,
                                               NULL,
                                               IOCTL_SCSI_GET_ADDRESS,
                                               NULL,
                                               &outputDescriptor,
                                               NULL,
                                               NULL);

    if (!NT_SUCCESS(status))
    {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                    "DeviceRetrieveScsiAddress: Get Address failed %lx\n",
                    status));
    }
    else
    {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                    "GetAddress: Port %x, Path %x, Target %x, Lun %x\n",
                    ScsiAddress->PortNumber,
                    ScsiAddress->PathId,
                    ScsiAddress->TargetId,
                    ScsiAddress->Lun));
    }

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceInitAllocateBuffers(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    Part of device initialize routine.
    Allocate all buffers in Device Extension.

Arguments:

    DeviceExtension - device extension of WDFDEVICE.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PVOID               senseData = NULL;

    PAGED_CODE();

    // allocate a private extension for class data
    if (DeviceExtension->PrivateFdoData == NULL)
    {
        DeviceExtension->PrivateFdoData = ExAllocatePoolWithTag(NonPagedPoolNx,
                                                                sizeof(CDROM_PRIVATE_FDO_DATA),
                                                                CDROM_TAG_PRIVATE_DATA);
    }

    if (DeviceExtension->PrivateFdoData == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        // initialize the struct's various fields.
        RtlZeroMemory(DeviceExtension->PrivateFdoData, sizeof(CDROM_PRIVATE_FDO_DATA));
    }

    // Allocate request sense buffer.
    senseData = ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned,
                                      SENSE_BUFFER_SIZE,
                                      CDROM_TAG_SENSE_INFO);

    if (senseData == NULL)
    {
        // The buffer cannot be allocated.
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        // Set the sense data pointer in the device extension.
        DeviceExtension->SenseData = senseData;
    }

    // Allocate scratch buffer -- Must occur after determining
    // max transfer size, but before other CD specific detection
    // (which relies upon this buffer).
    if (!ScratchBuffer_Allocate(DeviceExtension))
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                    "Failed to allocate scratch buffer, failing  %!STATUS!\n",
                    status
                    ));
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceRetrieveDescriptorsAndTransferLength(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    Part of device initialize routine.
    Retrieve Device Descriptor and Adaptor Descriptor.

Arguments:

    DeviceExtension - device extension of WDFDEVICE.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    STORAGE_PROPERTY_ID propertyId;

    PAGED_CODE();

    if (NT_SUCCESS(status))
    {
        // Call port driver to get adapter capabilities.
        propertyId = StorageAdapterProperty;

        status = DeviceRetrieveDescriptor(DeviceExtension->Device,
                                          &propertyId,
                                          (PSTORAGE_DESCRIPTOR_HEADER*)&DeviceExtension->AdapterDescriptor);
    }
    if (NT_SUCCESS(status))
    {
        // Call port driver to get device descriptor.
        propertyId = StorageDeviceProperty;

        status = DeviceRetrieveDescriptor(DeviceExtension->Device,
                                          &propertyId,
                                          (PSTORAGE_DESCRIPTOR_HEADER*)&DeviceExtension->DeviceDescriptor);
    }
    if (NT_SUCCESS(status))
    {
        // Call port driver to get device power property.
        // Not all port drivers support this property, and it's not fatal if this query fails.
        propertyId = StorageDevicePowerProperty;

        (void) DeviceRetrieveDescriptor(DeviceExtension->Device,
                                        &propertyId,
                                        (PSTORAGE_DESCRIPTOR_HEADER*)&DeviceExtension->PowerDescriptor);
    }

    if (NT_SUCCESS(status))
    {
        // Determine the maximum page-aligned and non-page-aligned transfer
        // lengths here, so we needn't do this in common READ/WRITE code paths

        // start with the number of pages the adapter can support
        ULONG maxAlignedTransfer = DeviceExtension->AdapterDescriptor->MaximumPhysicalPages;
        ULONG maxUnalignedTransfer = DeviceExtension->AdapterDescriptor->MaximumPhysicalPages;


        // Unaligned buffers could cross a page boundary.
        if (maxUnalignedTransfer > 1)
        {
            maxUnalignedTransfer--;
        }

        // if we'd overflow multiplying by page size, just max out the
        // transfer length allowed by the number of pages limit.
        if (maxAlignedTransfer >= (((ULONG)-1) / PAGE_SIZE))
        {
            maxAlignedTransfer = (ULONG)-1;
        }
        else
        {
            maxAlignedTransfer *= PAGE_SIZE;
        }

        if (maxUnalignedTransfer >= (((ULONG)-1) / PAGE_SIZE))
        {
            maxUnalignedTransfer = (ULONG)-1;
        }
        else
        {
            maxUnalignedTransfer *= PAGE_SIZE;
        }

        // finally, take the smaller of the above and the adapter's
        // reported maximum number of bytes per transfer.
        maxAlignedTransfer   = min(maxAlignedTransfer,   DeviceExtension->AdapterDescriptor->MaximumTransferLength);
        maxUnalignedTransfer = min(maxUnalignedTransfer, DeviceExtension->AdapterDescriptor->MaximumTransferLength);

        // Make sure the values are reasonable and not zero.
        maxAlignedTransfer   = max(maxAlignedTransfer,   PAGE_SIZE);
        maxUnalignedTransfer = max(maxUnalignedTransfer, PAGE_SIZE);

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                  "Device %p Max aligned/unaligned transfer size is %x/%x\n",
                  DeviceExtension->Device,
                  maxAlignedTransfer,
                  maxUnalignedTransfer
                  ));
        DeviceExtension->DeviceAdditionalData.MaxPageAlignedTransferBytes = maxAlignedTransfer;
        DeviceExtension->DeviceAdditionalData.MaxUnalignedTransferBytes = maxUnalignedTransfer;
    }
    else
    {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_PNP,  "DeviceRetrieveDescriptorsAndTransferLength failed %lx\n", status));
    }

    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceRetrieveDescriptor(
    _In_ WDFDEVICE                              Device,
    _In_ PSTORAGE_PROPERTY_ID                   PropertyId,
    _Outptr_ PSTORAGE_DESCRIPTOR_HEADER*        Descriptor
    )
/*++

Routine Description:

    This routine will perform a query for the specified property id and will
    allocate a non-paged buffer to store the data in.  It is the responsibility
    of the caller to ensure that this buffer is freed.

    This routine must be run at IRQL_PASSIVE_LEVEL

Arguments:

    Device - the device object
    PropertyId - type of property to retrieve
    Descriptor - buffer allocated in this function to hold the descriptor data

Return Value:

    status

--*/
{
    NTSTATUS                status;
    WDF_MEMORY_DESCRIPTOR   memoryDescriptor;

    STORAGE_PROPERTY_QUERY  query = {0};
    ULONG                   bufferLength = 0;

    PSTORAGE_DESCRIPTOR_HEADER  descriptor = NULL;
    PCDROM_DEVICE_EXTENSION     deviceExtension = DeviceGetExtension(Device);

    PAGED_CODE();

    // Set the passed-in descriptor pointer to NULL as default
    *Descriptor = NULL;

    // On the first pass we just want to get the first few
    // bytes of the descriptor so we can read it's size
    query.PropertyId = *PropertyId;
    query.QueryType = PropertyStandardQuery;

    descriptor = (PVOID)&query;

    NT_ASSERT(sizeof(STORAGE_PROPERTY_QUERY) >= (sizeof(ULONG)*2));

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memoryDescriptor,
                                      (PVOID)&query,
                                      sizeof(STORAGE_PROPERTY_QUERY));

    status = WdfIoTargetSendIoctlSynchronously(deviceExtension->IoTarget,
                                               NULL,
                                               IOCTL_STORAGE_QUERY_PROPERTY,
                                               &memoryDescriptor,
                                               &memoryDescriptor,
                                               NULL,
                                               NULL);

    if(!NT_SUCCESS(status))
    {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT,  "DeviceRetrieveDescriptor: error %lx trying to "
                       "query properties #1\n", status));
        return status;
    }

    if (descriptor->Size == 0)
    {
        // This DebugPrint is to help third-party driver writers
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT,  "DeviceRetrieveDescriptor: size returned was zero?! (status "
                    "%x\n", status));
        return STATUS_UNSUCCESSFUL;
    }

    // This time we know how much data there is so we can
    // allocate a buffer of the correct size
    bufferLength = descriptor->Size;
    NT_ASSERT(bufferLength >= sizeof(STORAGE_PROPERTY_QUERY));
    bufferLength = max(bufferLength, sizeof(STORAGE_PROPERTY_QUERY));

    descriptor = ExAllocatePoolWithTag(NonPagedPoolNx, bufferLength, CDROM_TAG_DESCRIPTOR);

    if(descriptor == NULL)
    {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT,  "DeviceRetrieveDescriptor: unable to memory for descriptor "
                    "(%d bytes)\n", bufferLength));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // setup the query again, as it was overwritten above
    RtlZeroMemory(&query, sizeof(STORAGE_PROPERTY_QUERY));
    query.PropertyId = *PropertyId;
    query.QueryType = PropertyStandardQuery;

    // copy the input to the new outputbuffer
    RtlCopyMemory(descriptor,
                  &query,
                  sizeof(STORAGE_PROPERTY_QUERY));

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memoryDescriptor,
                                      (PVOID)descriptor,
                                      bufferLength);

    status = WdfIoTargetSendIoctlSynchronously(deviceExtension->IoTarget,
                                               NULL,
                                               IOCTL_STORAGE_QUERY_PROPERTY,
                                               &memoryDescriptor,
                                               &memoryDescriptor,
                                               NULL,
                                               NULL);

    if(!NT_SUCCESS(status))
    {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT,  "DeviceRetrieveDescriptor: error %lx trying to "
                       "query properties #1\n", status));
        FREE_POOL(descriptor);

        return status;
    }

    // return the memory we've allocated to the caller
    *Descriptor = descriptor;

    return status;
} // end DeviceRetrieveDescriptor()

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
DeviceRetrieveHackFlagsFromRegistry(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    try to retrieve hack flages from registry and put the information in
    device extension.

Arguments:

    DeviceExtension - the device context

Return Value:

    none

--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    WDFKEY          hardwareKey = NULL;
    WDFKEY          subKey = NULL;
    ULONG           deviceHacks = 0;

    DECLARE_CONST_UNICODE_STRING(subKeyName, CLASSP_REG_SUBKEY_NAME);
    DECLARE_CONST_UNICODE_STRING(valueName, CLASSP_REG_HACK_VALUE_NAME);

    PAGED_CODE();

    status = WdfDeviceOpenRegistryKey(DeviceExtension->Device,
                                      PLUGPLAY_REGKEY_DEVICE,
                                      KEY_READ,
                                      WDF_NO_OBJECT_ATTRIBUTES,
                                      &hardwareKey);
    if (NT_SUCCESS(status))
    {
        status = WdfRegistryOpenKey(hardwareKey,
                                    &subKeyName,
                                    KEY_READ,
                                    WDF_NO_OBJECT_ATTRIBUTES,
                                    &subKey);

        if (NT_SUCCESS(status))
        {
            status = WdfRegistryQueryULong(subKey,
                                           &valueName,
                                           &deviceHacks);
            if (NT_SUCCESS(status))
            {
                // remove unknown values and save...
                CLEAR_FLAG(deviceHacks, FDO_HACK_INVALID_FLAGS);
                SET_FLAG(DeviceExtension->PrivateFdoData->HackFlags, deviceHacks);
            }

            WdfRegistryClose(subKey);
        }

        WdfRegistryClose(hardwareKey);
    }


    //
    // we should modify the system hive to include another key for us to grab
    // settings from.  in this case:  Classpnp\HackFlags
    //
    // the use of a DWORD value for the HackFlags allows 32 hacks w/o
    // significant use of the registry, and also reduces OEM exposure.
    //
    // definition of bit flags:
    //   0x00000001 -- Device succeeds PREVENT_MEDIUM_REMOVAL, but
    //                 cannot actually prevent removal.
    //   0x00000002 -- Device hard-hangs or times out for GESN requests.
    //   0x00000008 -- Device does not support RESERVE(6) and RELEASE(6).
    //   0x00000010 -- Device may incorrecly report operational changes in GESN.
    //   0x00000020 -- Device does not support streaming READ(12) / WRITE(12).
    //   0x00000040 -- Device does not support asynchronous notification.
    //   0xffffff80 -- Currently reserved, may be used later.
    //

    return;
}

_IRQL_requires_max_(APC_LEVEL)
VOID DeviceScanForSpecial(
    _In_ PCDROM_DEVICE_EXTENSION          DeviceExtension,
    _In_ CDROM_SCAN_FOR_SPECIAL_INFO      DeviceList[],
    _In_ PCDROM_SCAN_FOR_SPECIAL_HANDLER  Function)
/*++

Routine Description:

    scan the list of devices that should be hacked or not supported.

Arguments:

    DeviceExtension - the device context
    DeviceList - the device list
    Function - function used to scan from the list.

Return Value:

    none

--*/
{
    PSTORAGE_DEVICE_DESCRIPTOR  deviceDescriptor;
    PUCHAR                      vendorId;
    PUCHAR                      productId;
    PUCHAR                      productRevision;
    UCHAR                       nullString[] = "";

    PAGED_CODE();
    NT_ASSERT(DeviceList);
    NT_ASSERT(Function);

    if (DeviceList == NULL)
    {
        return;
    }
    if (Function == NULL)
    {
        return;
    }

    deviceDescriptor = DeviceExtension->DeviceDescriptor;

    // SCSI sets offsets to -1, ATAPI sets to 0.  check for both.
    if (deviceDescriptor->VendorIdOffset != 0 &&
        deviceDescriptor->VendorIdOffset != -1)
    {
        vendorId = ((PUCHAR)deviceDescriptor);
        vendorId += deviceDescriptor->VendorIdOffset;
    }
    else
    {
        vendorId = nullString;
    }

    if (deviceDescriptor->ProductIdOffset != 0 &&
        deviceDescriptor->ProductIdOffset != -1)
    {
        productId = ((PUCHAR)deviceDescriptor);
        productId += deviceDescriptor->ProductIdOffset;
    }
    else
    {
        productId = nullString;
    }

    if (deviceDescriptor->ProductRevisionOffset != 0 &&
        deviceDescriptor->ProductRevisionOffset != -1)
    {
        productRevision = ((PUCHAR)deviceDescriptor);
        productRevision += deviceDescriptor->ProductRevisionOffset;
    }
    else
    {
        productRevision = nullString;
    }

    // loop while the device list is valid (not null-filled)
    for (;(DeviceList->VendorId        != NULL ||
           DeviceList->ProductId       != NULL ||
           DeviceList->ProductRevision != NULL); DeviceList++)
    {
        if (StringsAreMatched(DeviceList->VendorId,        (LPSTR)vendorId) &&
            StringsAreMatched(DeviceList->ProductId,       (LPSTR)productId) &&
            StringsAreMatched(DeviceList->ProductRevision, (LPSTR)productRevision)
            )
        {

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT, "DeviceScanForSpecial: Found matching "
                        "controller Ven: %s Prod: %s Rev: %s\n",
                        (LPCSTR)vendorId, (LPCSTR)productId, (LPCSTR)productRevision));

            // pass the context to the call back routine and exit
            (Function)(DeviceExtension, DeviceList->Data);

            // for CHK builds, try to prevent wierd stacks by having a debug
            // print here. it's a hack, but i know of no other way to prevent
            // the stack from being wrong.
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT, "DeviceScanForSpecial: "
                        "completed callback\n"));
            return;

        } // else the strings did not match

    } // none of the devices matched.

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT, "DeviceScanForSpecial: no match found for %p\n",
                DeviceExtension->DeviceObject));
    return;

} // end DeviceScanForSpecial()

_IRQL_requires_max_(APC_LEVEL)
VOID
DeviceHackFlagsScan(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ ULONG_PTR                Data
    )
{
    PAGED_CODE();

    // remove invalid flags and save
    CLEAR_FLAG(Data, FDO_HACK_INVALID_FLAGS);
    SET_FLAG(DeviceExtension->PrivateFdoData->HackFlags, Data);

    return;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceInitializeHotplugInfo(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    Retrieve information into struc STORAGE_HOTPLUG_INFO in DeviceExtension
    initialize the hotplug information only after the ScanForSpecial routines,
    as it relies upon the hack flags - DeviceExtension->PrivateFdoData->HackFlags.

Arguments:

    DeviceExtension - the device context

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PCDROM_PRIVATE_FDO_DATA fdoData = DeviceExtension->PrivateFdoData;
    DEVICE_REMOVAL_POLICY   deviceRemovalPolicy = 0;
    ULONG                   resultLength = 0;
    ULONG                   writeCacheOverride;

    PAGED_CODE();

    // start with some default settings
    RtlZeroMemory(&(fdoData->HotplugInfo), sizeof(STORAGE_HOTPLUG_INFO));

    // set the size (aka version)
    fdoData->HotplugInfo.Size = sizeof(STORAGE_HOTPLUG_INFO);

    // set if the device has removable media
    if (DeviceExtension->DeviceDescriptor->RemovableMedia)
    {
        fdoData->HotplugInfo.MediaRemovable = TRUE;
    }
    else
    {
        fdoData->HotplugInfo.MediaRemovable = FALSE;
    }

    //
    // this refers to devices which, for reasons not yet understood,
    // do not fail PREVENT_MEDIA_REMOVAL requests even though they
    // have no way to lock the media into the drive.  this allows
    // the filesystems to turn off delayed-write caching for these
    // devices as well.
    //

    if (TEST_FLAG(DeviceExtension->PrivateFdoData->HackFlags, FDO_HACK_CANNOT_LOCK_MEDIA))
    {
        fdoData->HotplugInfo.MediaHotplug = TRUE;
    }
    else
    {
        fdoData->HotplugInfo.MediaHotplug = FALSE;
    }

    // Query the default removal policy from the kernel
    status = WdfDeviceQueryProperty(DeviceExtension->Device,
                                    DevicePropertyRemovalPolicy,
                                    sizeof(DEVICE_REMOVAL_POLICY),
                                    (PVOID)&deviceRemovalPolicy,
                                    &resultLength);
    if (NT_SUCCESS(status))
    {
        if (resultLength != sizeof(DEVICE_REMOVAL_POLICY))
        {
            status = STATUS_UNSUCCESSFUL;
        }
    }

    if (NT_SUCCESS(status))
    {
        // Look into the registry to see if the user has chosen
        // to override the default setting for the removal policy.
        // User can override only if the default removal policy is
        // orderly or suprise removal.

        if ((deviceRemovalPolicy == RemovalPolicyExpectOrderlyRemoval) ||
            (deviceRemovalPolicy == RemovalPolicyExpectSurpriseRemoval))
        {
            DEVICE_REMOVAL_POLICY userRemovalPolicy = 0;

            DeviceGetParameter(DeviceExtension,
                                CLASSP_REG_SUBKEY_NAME,
                                CLASSP_REG_REMOVAL_POLICY_VALUE_NAME,
                                (PULONG)&userRemovalPolicy);

            // Validate the override value and use it only if it is an
            // allowed value.
            if ((userRemovalPolicy == RemovalPolicyExpectOrderlyRemoval) ||
                (userRemovalPolicy == RemovalPolicyExpectSurpriseRemoval))
            {
                deviceRemovalPolicy = userRemovalPolicy;
            }
        }

        // use this info to set the DeviceHotplug setting
        // don't rely on DeviceCapabilities, since it can't properly
        // determine device relations, etc.  let the kernel figure this
        // stuff out instead.
        if (deviceRemovalPolicy == RemovalPolicyExpectSurpriseRemoval)
        {
            fdoData->HotplugInfo.DeviceHotplug = TRUE;
        }
        else
        {
            fdoData->HotplugInfo.DeviceHotplug = FALSE;
        }

        // this refers to the *filesystem* caching, but has to be included
        // here since it's a per-device setting.  this may change to be
        // stored by the system in the future.
        writeCacheOverride = FALSE;
        DeviceGetParameter(DeviceExtension,
                            CLASSP_REG_SUBKEY_NAME,
                            CLASSP_REG_WRITE_CACHE_VALUE_NAME,
                            &writeCacheOverride);

        if (writeCacheOverride)
        {
            fdoData->HotplugInfo.WriteCacheEnableOverride = TRUE;
        }
        else
        {
            fdoData->HotplugInfo.WriteCacheEnableOverride = FALSE;
        }
    }

    if (!NT_SUCCESS(status))
    {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT,  "Could not initialize hotplug information %lx\n", status));
    }

    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceInitMmcContext(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine initializes and populates the internal data structures that are
    used to discover various MMC-defined capabilities of the device.

    This routine will not clean up allocate resources if it fails - that
    is left for device stop/removal routines

Arguments:

    DeviceExtension - device extension

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;

    PAGED_CODE();

    DeviceExtension->DeviceAdditionalData.Mmc.IsMmc = FALSE;
    DeviceExtension->DeviceAdditionalData.Mmc.IsAACS = FALSE;
    DeviceExtension->DeviceAdditionalData.Mmc.IsWriter = FALSE;
    DeviceExtension->DeviceAdditionalData.Mmc.WriteAllowed = FALSE;
    DeviceExtension->DeviceAdditionalData.Mmc.IsCssDvd = FALSE;
    DeviceExtension->DeviceAdditionalData.DriveDeviceType = FILE_DEVICE_CD_ROM;

    // Determine if the drive is MMC-Capable
    if (NT_SUCCESS(status))
    {
        status = DeviceGetMmcSupportInfo(DeviceExtension,
                                         &DeviceExtension->DeviceAdditionalData.Mmc.IsMmc);

        if (!NT_SUCCESS(status))
        {
            //Currently, only low resource error comes here.
            //That is a success case for unsupporting this command.
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                        "DeviceInitMmcContext: Failed to get the support info for GET CONFIGURATION "
                        "command, failng %!STATUS!\n", status
                        ));

            DeviceExtension->DeviceAdditionalData.Mmc.IsMmc = FALSE;
            status = STATUS_SUCCESS;
        }
    }

    if (NT_SUCCESS(status) && DeviceExtension->DeviceAdditionalData.Mmc.IsMmc)
    {
        // the drive supports at least a subset of MMC commands
        // (and therefore supports READ_CD, etc...)

        // allocate a buffer for all the capabilities and such
        status = DeviceAllocateMmcResources(DeviceExtension->Device);
    }

    if (NT_SUCCESS(status) && DeviceExtension->DeviceAdditionalData.Mmc.IsMmc)
    {
        PFEATURE_HEADER header = NULL;
        FEATURE_NUMBER  validationSchema;
        ULONG           blockingFactor;

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                  "DeviceInitMmcContext: FDO %p GET CONFIGURATION buffer %p\n",
                  DeviceExtension->Device,
                  DeviceExtension->DeviceAdditionalData.Mmc.CapabilitiesBuffer
                  ));

        // Update several properties using the retrieved Configuration Data.

        // print all the feature pages (DBG only)
        DevicePrintAllFeaturePages(DeviceExtension->DeviceAdditionalData.Mmc.CapabilitiesBuffer,
                                   DeviceExtension->DeviceAdditionalData.Mmc.CapabilitiesBufferSize);

        // if AACS feature exists, enable AACS flag in the driver
        header = DeviceFindFeaturePage(DeviceExtension->DeviceAdditionalData.Mmc.CapabilitiesBuffer,
                                       DeviceExtension->DeviceAdditionalData.Mmc.CapabilitiesBufferSize,
                                       FeatureAACS);
        if (header)
        {
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                        "DeviceInitMmcContext: Reporting AACS support for device due to "
                        "GET CONFIGURATION showing support\n"
                        ));
            DeviceExtension->DeviceAdditionalData.Mmc.IsAACS = TRUE;
        }

#ifdef ENABLE_AACS_TESTING
        DeviceExtension->DeviceAdditionalData.Mmc.IsAACS = TRUE; // just force it true for testing
#endif // ENABLE_AACS_TESTING

        // Check if it's a DVD device
        header = DeviceFindFeaturePage(DeviceExtension->DeviceAdditionalData.Mmc.CapabilitiesBuffer,
                                       DeviceExtension->DeviceAdditionalData.Mmc.CapabilitiesBufferSize,
                                       FeatureDvdRead);
        if (header != NULL)
        {
            DeviceExtension->DeviceAdditionalData.DriveDeviceType = FILE_DEVICE_DVD;
        }

        // check if drive is writer
        DeviceUpdateMmcWriteCapability(DeviceExtension->DeviceAdditionalData.Mmc.CapabilitiesBuffer,
                                       DeviceExtension->DeviceAdditionalData.Mmc.CapabilitiesBufferSize,
                                       FALSE,                //Check if the drive has the ability to write.
                                       (PBOOLEAN)&(DeviceExtension->DeviceAdditionalData.Mmc.IsWriter),
                                       &validationSchema,
                                       &blockingFactor);

        // check if there is a CSS protected DVD or CPPM-protected DVDAudio media in drive.
        header = DeviceFindFeaturePage(DeviceExtension->DeviceAdditionalData.Mmc.CapabilitiesBuffer,
                                       DeviceExtension->DeviceAdditionalData.Mmc.CapabilitiesBufferSize,
                                       FeatureDvdCSS);

        DeviceExtension->DeviceAdditionalData.Mmc.IsCssDvd = (header != NULL) && (header->Current);

        // Flag the StartIo routine to update its state and hook in the error handler
        DeviceExtension->DeviceAdditionalData.Mmc.UpdateState = CdromMmcUpdateRequired;
        DeviceExtension->DeviceAdditionalData.ErrorHandler = DeviceErrorHandlerForMmc;

        SET_FLAG(DeviceExtension->DeviceFlags, DEV_SAFE_START_UNIT);

        // Read the CDROM mode sense page to get additional info for raw read requests.
        // only valid for MMC devices
        DeviceSetRawReadInfo(DeviceExtension);
    }

    // Set Read-Only device flag for non-MMC device.
    if (!(DeviceExtension->DeviceAdditionalData.Mmc.IsMmc))
    {
        ULONG  deviceCharacteristics = WdfDeviceGetCharacteristics(DeviceExtension->Device);

        deviceCharacteristics |= FILE_READ_ONLY_DEVICE;

        WdfDeviceSetCharacteristics(DeviceExtension->Device, deviceCharacteristics);

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                   "DeviceInitMmcContext: FDO %p Device is not an MMC compliant device, so setting "
                   "to read-only (legacy) mode",
                   DeviceExtension->Device
                   ));
    }

    // Set DEV_SAFE_START_UNIT flag for newer devices.
    if (DeviceExtension->DeviceAdditionalData.DriveDeviceType == FILE_DEVICE_DVD)
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                    "DeviceInitMmcContext: DVD Devices require START UNIT\n"));
        SET_FLAG(DeviceExtension->DeviceFlags, DEV_SAFE_START_UNIT);

    }
    else if ((DeviceExtension->DeviceDescriptor->BusType != BusTypeScsi)  &&
             (DeviceExtension->DeviceDescriptor->BusType != BusTypeAtapi) &&
             (DeviceExtension->DeviceDescriptor->BusType != BusTypeUnknown)
             )
    {
        // devices on the newer busses require START_UNIT
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                  "DeviceInitMmcContext: Devices for newer buses require START UNIT\n"));
        SET_FLAG(DeviceExtension->DeviceFlags, DEV_SAFE_START_UNIT);
    }

    return status;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
DeviceGetTimeOutValueFromRegistry()
/*++

Routine Description:

    get the device time out value from registry

Arguments:

    None

Return Value:

    ULONG - value of timeout

--*/
{
    NTSTATUS    status;
    WDFKEY      registryKey = NULL;
    ULONG        timeOutValue = 0;

    DECLARE_CONST_UNICODE_STRING(registryValueName, L"TimeOutValue");

    PAGED_CODE();

    // open the service key.
    status = WdfDriverOpenParametersRegistryKey(WdfGetDriver(),
                                                KEY_READ,
                                                WDF_NO_OBJECT_ATTRIBUTES,
                                                &registryKey);

    if (NT_SUCCESS(status))
    {
        status = WdfRegistryQueryULong(registryKey,
                                       &registryValueName,
                                       &timeOutValue);

        WdfRegistryClose(registryKey);
    }

    if (!NT_SUCCESS(status))
    {
        timeOutValue = 0;
    }

    return timeOutValue;

} // end DeviceGetTimeOutValueFromRegistry()


_IRQL_requires_max_(APC_LEVEL)
VOID
DeviceScanSpecialDevices(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This function checks to see if an SCSI logical unit requires an special
    initialization or error processing.

Arguments:

    Device - device object.

Return Value:

    None.

--*/
{

    PAGED_CODE();

    // set our hack flags
    DeviceScanForSpecial(DeviceExtension, CdromHackItems, ScanForSpecialHandler);

    //
    // All CDRom's can ignore the queue lock failure for power operations
    // and do not require handling the SpinUp case (unknown result of sending
    // a cdrom a START_UNIT command -- may eject disks?)
    //
    // We send the stop command mostly to stop outstanding asynch operations
    // (like audio playback) from running when the system is powered off.
    // Because of this and the unlikely chance that a PLAY command will be
    // sent in the window between the STOP and the time the machine powers down
    // we don't require queue locks.  This is important because without them
    // classpnp's power routines will send the START_STOP_UNIT command to the
    // device whether or not it supports locking (atapi does not support locking
    // and if we requested them we would end up not stopping audio on atapi
    // devices).
//    SET_FLAG(deviceExtension->ScanForSpecialFlags, CDROM_SPECIAL_DISABLE_SPIN_UP);
//    SET_FLAG(deviceExtension->ScanForSpecialFlags, CDROM_SPECIAL_NO_QUEUE_LOCK);

    if (TEST_FLAG(DeviceExtension->DeviceAdditionalData.HackFlags, CDROM_HACK_TOSHIBA_SD_W1101))
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                    "DeviceScanSpecialDevices: Found Toshiba SD-W1101 DVD-RAM "
                    "-- This drive will *NOT* support DVD-ROM playback.\n"));
    }
    else if (TEST_FLAG(DeviceExtension->DeviceAdditionalData.HackFlags, CDROM_HACK_HITACHI_GD_2000))
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                    "DeviceScanSpecialDevices: Found Hitachi GD-2000\n"));

        // Setup an error handler to spin up the drive when it idles out
        // since it seems to like to fail to spin itself back up on its
        // own for a REPORT_KEY command.  It may also lose the AGIDs that
        // it has given, which will result in DVD playback failures.
        // This routine will just do what it can...
        DeviceExtension->DeviceAdditionalData.ErrorHandler = DeviceErrorHandlerForHitachiGD2000;

        // this drive may require START_UNIT commands to spin
        // the drive up when it's spun itself down.
        SET_FLAG(DeviceExtension->DeviceFlags, DEV_SAFE_START_UNIT);
    }
    else if (TEST_FLAG(DeviceExtension->DeviceAdditionalData.HackFlags, CDROM_HACK_FUJITSU_FMCD_10x))
    {
        // When Read command is issued to FMCD-101 or FMCD-102 and there is a music
        // cd in it. It takes longer time than SCSI_CDROM_TIMEOUT before returning
        // error status.
        DeviceExtension->TimeOutValue = 20;
    }
    else if (TEST_FLAG(DeviceExtension->DeviceAdditionalData.HackFlags, CDROM_HACK_DEC_RRD))
    {
        NTSTATUS                   status;
        PMODE_PARM_READ_WRITE_DATA modeParameters;
        SCSI_REQUEST_BLOCK         srb = {0};
        PCDB                       cdb;

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
                    "DeviceScanSpecialDevices:  Found DEC RRD.\n"));

        DeviceExtension->DeviceAdditionalData.IsDecRrd = TRUE;

        // Setup an error handler to reinitialize the cd rom after it is reset?
        //
        //DeviceExtension->DevInfo->ClassError = DecRrdProcessError;

        // Found a DEC RRD cd-rom.  These devices do not pass MS HCT
        // multi-media tests because the DEC firmware modifieds the block
        // from the PC-standard 2K to 512.  Change the block transfer size
        // back to the PC-standard 2K by using a mode select command.

        modeParameters = ExAllocatePoolWithTag(NonPagedPoolNx,
                                               sizeof(MODE_PARM_READ_WRITE_DATA),
                                               CDROM_TAG_MODE_DATA);
        if (modeParameters == NULL)
        {
            return;
        }

        RtlZeroMemory(modeParameters, sizeof(MODE_PARM_READ_WRITE_DATA));
        RtlZeroMemory(&srb,           sizeof(SCSI_REQUEST_BLOCK));

        // Set the block length to 2K.
        modeParameters->ParameterListHeader.BlockDescriptorLength = sizeof(MODE_PARAMETER_BLOCK);

        // Set block length to 2K (0x0800) in Parameter Block.
        modeParameters->ParameterListBlock.BlockLength[0] = 0x00; //MSB
        modeParameters->ParameterListBlock.BlockLength[1] = 0x08;
        modeParameters->ParameterListBlock.BlockLength[2] = 0x00; //LSB

        // Build the mode select CDB.
        srb.CdbLength = 6;
        srb.TimeOutValue = DeviceExtension->TimeOutValue;

        cdb = (PCDB)srb.Cdb;
        cdb->MODE_SELECT.PFBit               = 1;
        cdb->MODE_SELECT.OperationCode       = SCSIOP_MODE_SELECT;
        cdb->MODE_SELECT.ParameterListLength = HITACHI_MODE_DATA_SIZE;

        // Send the request to the device.
        status = DeviceSendSrbSynchronously(DeviceExtension->Device,
                                            &srb,
                                            modeParameters,
                                            sizeof(MODE_PARM_READ_WRITE_DATA),
                                            TRUE,
                                            NULL);

        if (!NT_SUCCESS(status))
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                        "DeviceScanSpecialDevices: Setting DEC RRD to 2K block"
                        "size failed [%x]\n", status));
        }

        ExFreePool(modeParameters);
    }

    return;
}

_IRQL_requires_max_(APC_LEVEL)
VOID
ScanForSpecialHandler(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_ ULONG_PTR               HackFlags
    )
{
    PAGED_CODE();

    CLEAR_FLAG(HackFlags, CDROM_HACK_INVALID_FLAGS);

    DeviceExtension->DeviceAdditionalData.HackFlags = HackFlags;

    return;
}


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceCacheDeviceInquiryData(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    get inquiry data from device and cache it into device extension
    The first INQUIRY command sent is with 0x24 bytes required data,
    as ATAport driver always sends this to enumerate devices and 0x24
    bytes is the minimum data device should return by spec.

Arguments:

    DeviceExtension - device extension.

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    SCSI_REQUEST_BLOCK  srb = {0};
    PCDB                cdb = (PCDB)(&srb.Cdb);
    PINQUIRYDATA        tmpInquiry = NULL;

    // by spec, device should return at least 36 bytes.
    ULONG               requestedInquiryTransferBytes = MINIMUM_CDROM_INQUIRY_SIZE;
    BOOLEAN             needResendCommand = TRUE;
    BOOLEAN             portDriverHack = FALSE;

    // this ensures that the strings vendorID, productID, and firmwareRevision
    // are all available in the inquiry data.  In addition, MMC spec requires
    // all type 5 devices to have minimum 36 bytes of inquiry.
    static const UCHAR minInquiryAdditionalLength =
                                    MINIMUM_CDROM_INQUIRY_SIZE -
                                    RTL_SIZEOF_THROUGH_FIELD(INQUIRYDATA, AdditionalLength);

    C_ASSERT( RTL_SIZEOF_THROUGH_FIELD(INQUIRYDATA, AdditionalLength) <= 8 );
    C_ASSERT( RTL_SIZEOF_THROUGH_FIELD(INQUIRYDATA, ProductRevisionLevel) == MINIMUM_CDROM_INQUIRY_SIZE );

    PAGED_CODE();

    // short-circuit here for if already cached for this device
    // required to avoid use of scratch buffer after initialization
    // of MCN code.
    if (DeviceExtension->DeviceAdditionalData.CachedInquiryData != NULL)
    {
        NT_ASSERT(DeviceExtension->DeviceAdditionalData.CachedInquiryDataByteCount != 0);
        return STATUS_SUCCESS;
    }

    // 1. retrieve the inquiry data length

    // 1.1 allocate inquiry data buffer
    tmpInquiry = ExAllocatePoolWithTag(NonPagedPoolNx,
                                       requestedInquiryTransferBytes,
                                       CDROM_TAG_INQUIRY);
    if (tmpInquiry == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    // 1.2 send INQUIRY command
    if (NT_SUCCESS(status))
    {
        srb.CdbLength = 6;
        cdb->AsByte[0] = SCSIOP_INQUIRY;
        cdb->AsByte[3] = (UCHAR)( requestedInquiryTransferBytes >> (8*1) );
        cdb->AsByte[4] = (UCHAR)( requestedInquiryTransferBytes >> (8*0) );

        status = DeviceSendSrbSynchronously(DeviceExtension->Device,
                                            &srb,
                                            tmpInquiry,
                                            requestedInquiryTransferBytes,
                                            FALSE,
                                            NULL);
    }

    // 1.3 get required data length
    if (NT_SUCCESS(status))
    {
        if ((requestedInquiryTransferBytes == srb.DataTransferLength) &&
            (requestedInquiryTransferBytes == (tmpInquiry->AdditionalLength +
                                               RTL_SIZEOF_THROUGH_FIELD(INQUIRYDATA, AdditionalLength))) )
        {
            // device has only 36 bytes of INQUIRY data. do not need to resend the command.
            needResendCommand = FALSE;
        }
        else
        {
            // workaround an ATAPI.SYS bug where additional length field is set to zero
            if (tmpInquiry->AdditionalLength == 0)
            {
                tmpInquiry->AdditionalLength = minInquiryAdditionalLength;
                portDriverHack = TRUE;
            }

            requestedInquiryTransferBytes =
                                tmpInquiry->AdditionalLength +
                                RTL_SIZEOF_THROUGH_FIELD(INQUIRYDATA, AdditionalLength);

            if (requestedInquiryTransferBytes >= MINIMUM_CDROM_INQUIRY_SIZE)
            {
                needResendCommand = TRUE;
            }
            else
            {
                needResendCommand = FALSE;
                //Length is small than minimum length, error out.
                status = STATUS_DEVICE_PROTOCOL_ERROR;
            }
        }
    }

    // 2. retrieve the inquiry data if still needed.

    // 2.1 Clean up.
    if (NT_SUCCESS(status) && needResendCommand)
    {
        FREE_POOL(tmpInquiry);
        RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

        tmpInquiry = ExAllocatePoolWithTag(NonPagedPoolNx,
                                           requestedInquiryTransferBytes,
                                           CDROM_TAG_INQUIRY);
        if (tmpInquiry == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    // 2.2 resend INQUIRY command
    if (NT_SUCCESS(status) && needResendCommand)
    {
        srb.CdbLength = 6;
        cdb->AsByte[0] = SCSIOP_INQUIRY;
        cdb->AsByte[3] = (UCHAR)( requestedInquiryTransferBytes >> (8*1) );
        cdb->AsByte[4] = (UCHAR)( requestedInquiryTransferBytes >> (8*0) );

        status = DeviceSendSrbSynchronously( DeviceExtension->Device,
                                             &srb,
                                             tmpInquiry,
                                             requestedInquiryTransferBytes,
                                             FALSE,
                                             NULL);

        if (!NT_SUCCESS(status))
        {
            // Workaround for drive reports that it has more INQUIRY data than reality.
            if ((srb.SrbStatus == SRB_STATUS_DATA_OVERRUN) &&
                (srb.DataTransferLength < requestedInquiryTransferBytes) &&
                (srb.DataTransferLength >= MINIMUM_CDROM_INQUIRY_SIZE))
            {
                //Port driver says buffer size mismatch (buffer underrun),
                //retry with the real buffer size it could return.
                requestedInquiryTransferBytes = srb.DataTransferLength;

                FREE_POOL(tmpInquiry);
                RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

                tmpInquiry = ExAllocatePoolWithTag(NonPagedPoolNx,
                                                   requestedInquiryTransferBytes,
                                                   CDROM_TAG_INQUIRY);
                if (tmpInquiry == NULL)
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
                else
                {
                    srb.CdbLength = 6;
                    cdb->AsByte[0] = SCSIOP_INQUIRY;
                    cdb->AsByte[3] = (UCHAR)( requestedInquiryTransferBytes >> (8*1) );
                    cdb->AsByte[4] = (UCHAR)( requestedInquiryTransferBytes >> (8*0) );

                    status = DeviceSendSrbSynchronously(DeviceExtension->Device,
                                                        &srb,
                                                        tmpInquiry,
                                                        requestedInquiryTransferBytes,
                                                        FALSE,
                                                        NULL);
                }
            }
        }

        //Check the transferred data length for safe.
        if (NT_SUCCESS(status))
        {
            requestedInquiryTransferBytes = srb.DataTransferLength;

            if (requestedInquiryTransferBytes < MINIMUM_CDROM_INQUIRY_SIZE)
            {
                // should never occur
                status = STATUS_DEVICE_PROTOCOL_ERROR;
            }
        }

        // ensure we got some non-zero data....
        // This is done so we don't accidentally work around the
        // ATAPI.SYS bug when no data was transferred.
        if (NT_SUCCESS(status) && portDriverHack)
        {
            PULONG  tmp = (PULONG)tmpInquiry;
            ULONG   i = MINIMUM_CDROM_INQUIRY_SIZE / sizeof(ULONG);
            C_ASSERT( RTL_SIZEOF_THROUGH_FIELD(INQUIRYDATA, ProductRevisionLevel) % sizeof(ULONG) == 0 );

            // wouldn't you know it -- there is no RtlIsMemoryZero() function; Make one up.
            for ( ; i != 0; i--)
            {
                if (*tmp != 0)
                {
                    break; // out of this inner FOR loop -- guarantees 'i != 0'
                }
                tmp++;
            }

            if (i == 0) // all loop'd successfully
            {
                // should never occur to successfully get all zero'd data
                status = STATUS_DEVICE_PROTOCOL_ERROR;
            }
        }
    }

    // if everything succeeded, then (and only then) modify the device extension
    if (NT_SUCCESS(status))
    {
        DeviceExtension->DeviceAdditionalData.CachedInquiryData = tmpInquiry;
        DeviceExtension->DeviceAdditionalData.CachedInquiryDataByteCount = requestedInquiryTransferBytes;
    }
    else
    {
        FREE_POOL(tmpInquiry);
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceGetMmcSupportInfo(
    _In_  PCDROM_DEVICE_EXTENSION   DeviceExtension,
    _Out_ PBOOLEAN                  IsMmcDevice
    )
/*++

Routine Description:

    check if the device is MMC capable.

Arguments:

    DeviceExtension - device extension.

Return Value:

    NTSTATUS.
    IsMmcDevice - TRUE (MMC capable); FALSE (not MMC device)

--*/
{
    NTSTATUS    status;
    ULONG       size;
    ULONG       previouslyFailed;

    PAGED_CODE();

    *IsMmcDevice  = FALSE;

    // read the registry in case the drive failed previously,
    // and a timeout is occurring.
    previouslyFailed = FALSE;
    DeviceGetParameter(DeviceExtension,
                       CDROM_SUBKEY_NAME,
                       CDROM_NON_MMC_DRIVE_NAME,
                       &previouslyFailed);

    if (previouslyFailed)
    {
        SET_FLAG(DeviceExtension->DeviceAdditionalData.HackFlags, CDROM_HACK_BAD_GET_CONFIG_SUPPORT);
    }

    // read from the registry in case the drive reports bad profile lengths
    previouslyFailed = FALSE;
    DeviceGetParameter(DeviceExtension,
                       CDROM_SUBKEY_NAME,
                       CDROM_NON_MMC_VENDOR_SPECIFIC_PROFILE,
                       &previouslyFailed);

    if (previouslyFailed)
    {
        SET_FLAG(DeviceExtension->DeviceAdditionalData.HackFlags, CDROM_HACK_BAD_VENDOR_PROFILES);
    }

    // check for the ProfileList feature to determine if the drive is MMC compliant
    // and set the "size" local variable to total GetConfig data size available.
    // NOTE: This will exit this function in some error paths.
    {
        GET_CONFIGURATION_HEADER    localHeader = {0};
        ULONG                       usable = 0;

        status = DeviceGetConfiguration(DeviceExtension->Device,
                                        &localHeader,
                                        sizeof(GET_CONFIGURATION_HEADER),
                                        &usable,
                                        FeatureProfileList,
                                        SCSI_GET_CONFIGURATION_REQUEST_TYPE_ALL);

        if (status == STATUS_INVALID_DEVICE_REQUEST ||
            status == STATUS_NO_MEDIA_IN_DEVICE     ||
            status == STATUS_IO_DEVICE_ERROR        ||
            status == STATUS_IO_TIMEOUT)
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                       "GetConfiguration Failed (%x), device %p not mmc-compliant\n",
                       status, DeviceExtension->DeviceObject
                       ));

            previouslyFailed = TRUE;
            DeviceSetParameter( DeviceExtension,
                                CDROM_SUBKEY_NAME,
                                CDROM_NON_MMC_DRIVE_NAME,
                                previouslyFailed);

            return STATUS_SUCCESS;
        }
        else if (!NT_SUCCESS(status))
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                       "GetConfiguration Failed, status %x -- defaulting to -ROM\n",
                       status));

            return STATUS_SUCCESS;
        }
        else if (usable < sizeof(GET_CONFIGURATION_HEADER))
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                       "GetConfiguration Failed, returned only %x bytes!\n", usable));
            previouslyFailed = TRUE;
            DeviceSetParameter( DeviceExtension,
                                CDROM_SUBKEY_NAME,
                                CDROM_NON_MMC_DRIVE_NAME,
                                previouslyFailed);

            return STATUS_SUCCESS;
        }

        size = (localHeader.DataLength[0] << 24) |
               (localHeader.DataLength[1] << 16) |
               (localHeader.DataLength[2] <<  8) |
               (localHeader.DataLength[3] <<  0) ;


        if ((size <= 4) || (size + 4 < size))
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                       "GetConfiguration Failed, claims MMC support but doesn't "
                       "correctly return config length! (%x)\n",
                       size
                       ));
            previouslyFailed = TRUE;
            DeviceSetParameter( DeviceExtension,
                                CDROM_SUBKEY_NAME,
                                CDROM_NON_MMC_DRIVE_NAME,
                                previouslyFailed);

            return STATUS_SUCCESS;
        }
        else if ((size % 4) != 0)
        {
            if ((size % 2) != 0)
            {
                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                           "GetConfiguration Failed, returned odd number of bytes %x!\n",
                           size
                           ));
                previouslyFailed = TRUE;
                DeviceSetParameter( DeviceExtension,
                                    CDROM_SUBKEY_NAME,
                                    CDROM_NON_MMC_DRIVE_NAME,
                                    previouslyFailed);

                return STATUS_SUCCESS;
            }
            else
            {
                if (TEST_FLAG(DeviceExtension->DeviceAdditionalData.HackFlags, CDROM_HACK_BAD_VENDOR_PROFILES))
                {
                    // we've already caught this and ASSERT'd once, so don't do it again
                }
                else
                {
                    TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                               "GetConfiguration returned a size that is not per spec (%x bytes), this is probably because of a vendor specific data header with a size not divisible by 4.\n",
                               size
                               ));
                    previouslyFailed = TRUE;
                    DeviceSetParameter(DeviceExtension,
                                       CDROM_SUBKEY_NAME,
                                       CDROM_NON_MMC_VENDOR_SPECIFIC_PROFILE,
                                       previouslyFailed);
                }
            }
        }

        size += 4; // sizeof the datalength fields
    }

    *IsMmcDevice = TRUE;

    // This code doesn't handle total get config size over 64k
    NT_ASSERT( size <= MAXUSHORT );

    // Check for SCSI_GET_CONFIGURATION_REQUEST_TYPE_ONE support in the device.
    // NOTE: This will exit this function in some error paths.
    {
        ULONG   featureSize = sizeof(GET_CONFIGURATION_HEADER)+sizeof(FEATURE_HEADER);
        ULONG   usable = 0;

        PGET_CONFIGURATION_HEADER configBuffer = ExAllocatePoolWithTag(
                                                            NonPagedPoolNx,
                                                            featureSize,
                                                            CDROM_TAG_GET_CONFIG);

        if (configBuffer == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        // read the registry in case the drive failed previously,
        // and a timeout is occurring.
        previouslyFailed = FALSE;
        DeviceGetParameter( DeviceExtension,
                            CDROM_SUBKEY_NAME,
                            CDROM_TYPE_ONE_GET_CONFIG_NAME,
                            &previouslyFailed);

        if (previouslyFailed)
        {
            SET_FLAG(DeviceExtension->DeviceAdditionalData.HackFlags, CDROM_HACK_BAD_TYPE_ONE_GET_CONFIG);
            FREE_POOL(configBuffer);
            return STATUS_SUCCESS;
        }

        // Get only the config and feature header
        status = DeviceGetConfiguration(DeviceExtension->Device,
                                        configBuffer,
                                        featureSize,
                                        &usable,
                                        FeatureProfileList,
                                        SCSI_GET_CONFIGURATION_REQUEST_TYPE_ONE);

        if (!NT_SUCCESS(status) || (usable < featureSize))
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                       "Type One GetConfiguration Failed. Usable buffer size: %d\n", usable));
            previouslyFailed = TRUE;
        }
        else
        {
            PFEATURE_HEADER featureHeader;
            ULONG           totalAvailableBytes = 0;
            ULONG           expectedAvailableBytes = 0;

            REVERSE_BYTES(&totalAvailableBytes, configBuffer->DataLength);
            totalAvailableBytes += RTL_SIZEOF_THROUGH_FIELD(GET_CONFIGURATION_HEADER, DataLength);

            featureHeader = (PFEATURE_HEADER) ((PUCHAR)configBuffer + sizeof(GET_CONFIGURATION_HEADER));
            expectedAvailableBytes = sizeof(GET_CONFIGURATION_HEADER) +
                                     sizeof(FEATURE_HEADER) +
                                     featureHeader->AdditionalLength;

            if (totalAvailableBytes > expectedAvailableBytes)
            {
                // Device is returning more than required size. Most likely the device
                // is returning TYPE ALL data. Set the flag to use TYPE ALL for TYPE ONE
                // requets
                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                           "Type One GetConfiguration Failed. "
                           "Device returned %d bytes instead of %d bytes\n",
                           size, featureSize));

                previouslyFailed = TRUE;
            }
        }

        FREE_POOL(configBuffer);

        if (previouslyFailed == TRUE)
        {
            DeviceSetParameter( DeviceExtension,
                                CDROM_SUBKEY_NAME,
                                CDROM_TYPE_ONE_GET_CONFIG_NAME,
                                previouslyFailed);

            SET_FLAG(DeviceExtension->DeviceAdditionalData.HackFlags, CDROM_HACK_BAD_TYPE_ONE_GET_CONFIG);
        }
    }

    return status;
}


_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceSetRawReadInfo(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine reads the CDROM capabilities mode page and save information
    in the device extension needed for raw reads.
    NOTE: this function is only valid for MMC device

Arguments:

    DeviceExtension - device context

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    PUCHAR      buffer = NULL;
    ULONG       count = 0;

    PAGED_CODE();

    // Check whether the device can return C2 error flag bits and the block
    // error byte.  If so, save this info and fill in appropriate flag during
    // raw read requests.

    // Start by checking the GET_CONFIGURATION data
    {
        PFEATURE_DATA_CD_READ   cdReadHeader = NULL;
        ULONG                   additionalLength = sizeof(FEATURE_DATA_CD_READ) - sizeof(FEATURE_HEADER);

        cdReadHeader = DeviceFindFeaturePage(DeviceExtension->DeviceAdditionalData.Mmc.CapabilitiesBuffer,
                                            DeviceExtension->DeviceAdditionalData.Mmc.CapabilitiesBufferSize,
                                            FeatureCdRead);

        if ((cdReadHeader != NULL) &&
            (cdReadHeader->Header.AdditionalLength >= additionalLength) &&
            (cdReadHeader->C2ErrorData)
            )
        {
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                      "DeviceSetRawReadInfo: FDO %p GET_CONFIG shows ability to read C2 error bits\n",
                      DeviceExtension->DeviceObject
                      ));
            DeviceExtension->DeviceAdditionalData.Mmc.ReadCdC2Pointers = TRUE;    // Device returns C2 error info.
        }
    }

    // Unfortunately, the only way to check for the ability to read R-W subcode
    // information is via MODE_SENSE.  Do so here, and check the C2 bit as well
    // in case the drive has a firmware bug where it fails to report this ability
    // in GET_CONFIG (which some drives do).
    for (count = 0; count < 6; count++)
    {
        SCSI_REQUEST_BLOCK  srb = {0};
        PCDB                cdb = (PCDB)srb.Cdb;
        ULONG               bufferLength = 0;

        // Build the MODE SENSE CDB.  Try 10-byte CDB first.
        if ((count/3) == 0)
        {
            bufferLength = sizeof(CDVD_CAPABILITIES_PAGE)  +
                           sizeof(MODE_PARAMETER_HEADER10) +
                           sizeof(MODE_PARAMETER_BLOCK);

            cdb->MODE_SENSE10.OperationCode = SCSIOP_MODE_SENSE10;
            cdb->MODE_SENSE10.Dbd = 1;
            cdb->MODE_SENSE10.PageCode = MODE_PAGE_CAPABILITIES;
            cdb->MODE_SENSE10.AllocationLength[0] = (UCHAR)(bufferLength >> 8);
            cdb->MODE_SENSE10.AllocationLength[1] = (UCHAR)(bufferLength >> 0);
            srb.CdbLength = 10;
        }
        else
        {
            bufferLength = sizeof(CDVD_CAPABILITIES_PAGE) +
                           sizeof(MODE_PARAMETER_HEADER)  +
                           sizeof(MODE_PARAMETER_BLOCK);

            cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
            cdb->MODE_SENSE.Dbd = 1;
            cdb->MODE_SENSE.PageCode = MODE_PAGE_CAPABILITIES;
            cdb->MODE_SENSE.AllocationLength = (UCHAR)bufferLength;
            srb.CdbLength = 6;
        }

        // Set timeout value from device extension.
        srb.TimeOutValue = DeviceExtension->TimeOutValue;

        buffer = ExAllocatePoolWithTag(NonPagedPoolNx, bufferLength, CDROM_TAG_MODE_DATA);

        if (buffer == NULL)
        {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
                      "DeviceSetRawReadInfo: cannot allocate "
                      "buffer, so not setting raw read info for FDO %p\n",
                      DeviceExtension->DeviceObject
                      ));
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto FnExit;
        }

        RtlZeroMemory(buffer, bufferLength);

        status = DeviceSendSrbSynchronously(DeviceExtension->Device,
                                            &srb,
                                            buffer,
                                            bufferLength,
                                            FALSE,
                                            NULL);

        if (NT_SUCCESS(status) ||
            (status == STATUS_DATA_OVERRUN) ||
            (status == STATUS_BUFFER_OVERFLOW))
        {
            PCDVD_CAPABILITIES_PAGE capabilities = NULL;

            // determine where the capabilities page really is
            if ((count/3) == 0)
            {
                PMODE_PARAMETER_HEADER10 p = (PMODE_PARAMETER_HEADER10)buffer;
                capabilities = (PCDVD_CAPABILITIES_PAGE)(buffer +
                                                         sizeof(MODE_PARAMETER_HEADER10) +
                                                         (p->BlockDescriptorLength[0] * 256) +
                                                         p->BlockDescriptorLength[1]);
            }
            else
            {
                PMODE_PARAMETER_HEADER p = (PMODE_PARAMETER_HEADER)buffer;
                capabilities = (PCDVD_CAPABILITIES_PAGE)(buffer +
                                                         sizeof(MODE_PARAMETER_HEADER) +
                                                         p->BlockDescriptorLength);
            }

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                      "DeviceSetRawReadInfo: FDO %p CDVD Capabilities buffer %p\n",
                      DeviceExtension->DeviceObject,
                      buffer
                      ));

            if (capabilities->PageCode == MODE_PAGE_CAPABILITIES)
            {
                if (capabilities->C2Pointers)
                {
                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                              "DeviceSetRawReadInfo: FDO %p supports C2 error bits in READ_CD command\n",
                              DeviceExtension->DeviceObject
                              ));
                    DeviceExtension->DeviceAdditionalData.Mmc.ReadCdC2Pointers = TRUE;
                }

                if (capabilities->RWSupported)
                {
                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,
                              "DeviceSetRawReadInfo: FDO %p supports raw subcode in READ_CD command\n",
                              DeviceExtension->DeviceObject
                              ));
                    DeviceExtension->DeviceAdditionalData.Mmc.ReadCdSubCode = TRUE;
                }

                break;
            }
        }

        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                  "DeviceSetRawReadInfo: FDO %p failed %x byte mode sense, status %x\n",
                  DeviceExtension->DeviceObject,
                  (((count/3) == 0) ? 10 : 6),
                  status
                  ));

        FREE_POOL(buffer);
    }

    if (count == 6)
    {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,
                  "DeviceSetRawReadInfo: FDO %p couldn't get mode sense data\n",
                  DeviceExtension->DeviceObject
                  ));
    }

FnExit:

    if (buffer)
    {
        FREE_POOL(buffer);
    }

    return status;
}


_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceInitializeDvd(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

    This routine sets the region of DVD drive
    NOTE: this routine uses ScratchBuffer, it must be called after ScratchBuffer allocated.

Arguments:

    Device - device object

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                    status = STATUS_SUCCESS;
    PCDROM_DEVICE_EXTENSION     deviceExtension;
    PDVD_COPY_PROTECT_KEY       copyProtectKey = NULL;
    PDVD_RPC_KEY                rpcKey = NULL;
    ULONG                       bufferLen = 0;
    size_t                      bytesReturned;

    PAGED_CODE();

    deviceExtension = DeviceGetExtension(Device);

    // check to see if we have a DVD device
    if (deviceExtension->DeviceAdditionalData.DriveDeviceType != FILE_DEVICE_DVD)
    {
        return STATUS_SUCCESS;
    }

    // we got a DVD drive.
    bufferLen = DVD_RPC_KEY_LENGTH;
    copyProtectKey = (PDVD_COPY_PROTECT_KEY)ExAllocatePoolWithTag(PagedPool,
                                                                  bufferLen,
                                                                  DVD_TAG_RPC2_CHECK);

    if (copyProtectKey == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // get the device region
    RtlZeroMemory (copyProtectKey, bufferLen);
    copyProtectKey->KeyLength = DVD_RPC_KEY_LENGTH;
    copyProtectKey->KeyType = DvdGetRpcKey;

    // perform IOCTL_DVD_READ_KEY
    status = DvdStartSessionReadKey(deviceExtension,
                                    IOCTL_DVD_READ_KEY,
                                    NULL,
                                    copyProtectKey,
                                    DVD_RPC_KEY_LENGTH,
                                    copyProtectKey,
                                    DVD_RPC_KEY_LENGTH,
                                    &bytesReturned);

    if (NT_SUCCESS(status))
    {
        rpcKey = (PDVD_RPC_KEY)copyProtectKey->KeyData;

        // TypeCode of zero means that no region has been set.
        if (rpcKey->TypeCode == 0)
        {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_PNP,
                        "DVD Initialize (%p): must choose DVD region\n",
                        Device));
            deviceExtension->DeviceAdditionalData.PickDvdRegion = 1;

            // set the device region code to be the same as region code on media.
            if (deviceExtension->DeviceAdditionalData.Mmc.IsCssDvd)
            {
                DevicePickDvdRegion(Device);
            }
        }
    }

    FREE_POOL(copyProtectKey);

    // return status of IOCTL_DVD_READ_KEY will be ignored.
    return STATUS_SUCCESS;
}


#if (NTDDI_VERSION >= NTDDI_WIN8)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceIsPortable(
    _In_  PCDROM_DEVICE_EXTENSION   DeviceExtension,
    _Out_ PBOOLEAN                  IsPortable
    )
/*++

Routine Description:

    This routine checks if the volume is on a portable storage device.

Arguments:

    DeviceExtension - device context
    IsPortable - device is portable

Return Value:

    NTSTATUS.

--*/

{
    DEVPROP_BOOLEAN isInternal = DEVPROP_FALSE;
    BOOLEAN         isPortable = FALSE;
    ULONG           size       = 0;
    NTSTATUS        status     = STATUS_SUCCESS;
    DEVPROPTYPE     type       = DEVPROP_TYPE_EMPTY;

    PAGED_CODE();

    *IsPortable = FALSE;

    // Check to see if the underlying device object is in local machine container
    status = IoGetDevicePropertyData(DeviceExtension->LowerPdo,
                                     &DEVPKEY_Device_InLocalMachineContainer,
                                     0,
                                     0,
                                     sizeof(isInternal),
                                     &isInternal,
                                     &size,
                                     &type);

    if (!NT_SUCCESS(status))
    {
        goto Cleanup;
    }

    NT_ASSERT(size == sizeof(isInternal));
    NT_ASSERT(type == DEVPROP_TYPE_BOOLEAN);

    // Volume is hot-pluggable if the disk pdo container id differs from that of root device
    if (isInternal == DEVPROP_TRUE)
    {
        goto Cleanup;
    }

    isPortable = TRUE;

    // Examine the bus type to  ensure that this really is a fixed device
    if (DeviceExtension->DeviceDescriptor->BusType == BusTypeFibre ||
        DeviceExtension->DeviceDescriptor->BusType == BusTypeiScsi ||
        DeviceExtension->DeviceDescriptor->BusType == BusTypeRAID)
    {
        isPortable = FALSE;
    }

    *IsPortable = isPortable;

Cleanup:

    return status;
}
#endif


#pragma warning(pop) // un-sets any local warning changes

