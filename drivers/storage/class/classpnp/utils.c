/*++

Copyright (C) Microsoft Corporation, 1991 - 2010

Module Name:

    utils.c

Abstract:

    SCSI class driver routines

Environment:

    kernel mode only

Notes:


Revision History:

--*/


#include "classp.h"
#include "debug.h"
#include <ntiologc.h>


#ifdef DEBUG_USE_WPP
#include "utils.tmh"
#endif

//
// Constant value used in firmware upgrade process.
//
#define FIRMWARE_ACTIVATE_TIMEOUT_VALUE        30


#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, ClassGetDeviceParameter)
    #pragma alloc_text(PAGE, ClassScanForSpecial)
    #pragma alloc_text(PAGE, ClassSetDeviceParameter)
    #pragma alloc_text(PAGE, ClasspMyStringMatches)
    #pragma alloc_text(PAGE, ClasspDeviceCopyOffloadProperty)
    #pragma alloc_text(PAGE, ClasspValidateOffloadSupported)
    #pragma alloc_text(PAGE, ClasspValidateOffloadInputParameters)
#endif

// custom string match -- careful!
BOOLEAN ClasspMyStringMatches(_In_opt_z_ PCHAR StringToMatch, _In_z_ PCHAR TargetString)
{
    ULONG length;  // strlen returns an int, not size_t (!)
    PAGED_CODE();
    NT_ASSERT(TargetString);
    // if no match requested, return TRUE
    if (StringToMatch == NULL) {
        return TRUE;
    }
    // cache the string length for efficiency
    length = (ULONG)strlen(StringToMatch);
    // ZERO-length strings may only match zero-length strings
    if (length == 0) {
        return (strlen(TargetString) == 0);
    }
    // strncmp returns zero if the strings match
    return (strncmp(StringToMatch, TargetString, length) == 0);
}


_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassGetDeviceParameter(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    _In_opt_ PWSTR SubkeyName,
    _In_ PWSTR ParameterName,
    _Inout_ PULONG ParameterValue  // also default value
    )
{
    NTSTATUS                 status;
    RTL_QUERY_REGISTRY_TABLE queryTable[2] = {0};
    HANDLE                   deviceParameterHandle = NULL;
    HANDLE                   deviceSubkeyHandle = NULL;
    ULONG                    defaultParameterValue;

    PAGED_CODE();

    //
    // open the given parameter
    //

    status = IoOpenDeviceRegistryKey(FdoExtension->LowerPdo,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_READ,
                                     &deviceParameterHandle);

    if (NT_SUCCESS(status) && (SubkeyName != NULL)) {

        UNICODE_STRING subkeyName;
        OBJECT_ATTRIBUTES objectAttributes = {0};

        RtlInitUnicodeString(&subkeyName, SubkeyName);
        InitializeObjectAttributes(&objectAttributes,
                                   &subkeyName,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   deviceParameterHandle,
                                   NULL);

        status = ZwOpenKey(&deviceSubkeyHandle,
                           KEY_READ,
                           &objectAttributes);
        if (!NT_SUCCESS(status)) {
            ZwClose(deviceParameterHandle);
        }

    }

    if (NT_SUCCESS(status)) {

        defaultParameterValue = *ParameterValue;

        queryTable->Flags         = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_TYPECHECK;
        queryTable->Name          = ParameterName;
        queryTable->EntryContext  = ParameterValue;
        queryTable->DefaultType   = (REG_DWORD << RTL_QUERY_REGISTRY_TYPECHECK_SHIFT) | REG_NONE;
        queryTable->DefaultData   = NULL;
        queryTable->DefaultLength = 0;

        status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                        (PWSTR)(SubkeyName ?
                                                deviceSubkeyHandle :
                                                deviceParameterHandle),
                                        queryTable,
                                        NULL,
                                        NULL);
        if (!NT_SUCCESS(status)) {
            *ParameterValue = defaultParameterValue; // use default value
        }

        //
        // close what we open
        //

        if (SubkeyName) {
            ZwClose(deviceSubkeyHandle);
        }

        ZwClose(deviceParameterHandle);
    }

    if (!NT_SUCCESS(status)) {

        //
        // Windows 2000 SP3 uses the driver-specific key, so look in there
        //

        status = IoOpenDeviceRegistryKey(FdoExtension->LowerPdo,
                                         PLUGPLAY_REGKEY_DRIVER,
                                         KEY_READ,
                                         &deviceParameterHandle);

        if (NT_SUCCESS(status) && (SubkeyName != NULL)) {

            UNICODE_STRING subkeyName;
            OBJECT_ATTRIBUTES objectAttributes = {0};

            RtlInitUnicodeString(&subkeyName, SubkeyName);
            InitializeObjectAttributes(&objectAttributes,
                                       &subkeyName,
                                       OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                       deviceParameterHandle,
                                       NULL);

            status = ZwOpenKey(&deviceSubkeyHandle, KEY_READ, &objectAttributes);

            if (!NT_SUCCESS(status)) {
                ZwClose(deviceParameterHandle);
            }
        }

        if (NT_SUCCESS(status)) {

            defaultParameterValue = *ParameterValue;

            queryTable->Flags         = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_TYPECHECK;
            queryTable->Name          = ParameterName;
            queryTable->EntryContext  = ParameterValue;
            queryTable->DefaultType   =  (REG_DWORD << RTL_QUERY_REGISTRY_TYPECHECK_SHIFT) | REG_NONE;
            queryTable->DefaultData   = NULL;
            queryTable->DefaultLength = 0;

            status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                            (PWSTR)(SubkeyName ?
                                                    deviceSubkeyHandle :
                                                    deviceParameterHandle),
                                            queryTable,
                                            NULL,
                                            NULL);
            if (NT_SUCCESS(status)) {

                //
                // Migrate the value over to the device-specific key
                //

                ClassSetDeviceParameter(FdoExtension, SubkeyName, ParameterName, *ParameterValue);

            } else {

                //
                // Use the default value
                //

                *ParameterValue = defaultParameterValue;
            }

            if (SubkeyName) {
                ZwClose(deviceSubkeyHandle);
            }

            ZwClose(deviceParameterHandle);
        }
    }

    return;

} // end ClassGetDeviceParameter()

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassSetDeviceParameter(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    _In_opt_ PWSTR SubkeyName,
    _In_ PWSTR ParameterName,
    _In_ ULONG ParameterValue)
{
    NTSTATUS                 status;
    HANDLE                   deviceParameterHandle = NULL;
    HANDLE                   deviceSubkeyHandle = NULL;

    PAGED_CODE();

    //
    // open the given parameter
    //

    status = IoOpenDeviceRegistryKey(FdoExtension->LowerPdo,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_READ | KEY_WRITE,
                                     &deviceParameterHandle);

    if (NT_SUCCESS(status) && (SubkeyName != NULL)) {

        UNICODE_STRING subkeyName;
        OBJECT_ATTRIBUTES objectAttributes;

        RtlInitUnicodeString(&subkeyName, SubkeyName);
        InitializeObjectAttributes(&objectAttributes,
                                   &subkeyName,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   deviceParameterHandle,
                                   NULL);

        status = ZwCreateKey(&deviceSubkeyHandle,
                             KEY_READ | KEY_WRITE,
                             &objectAttributes,
                             0, NULL, 0, NULL);
        if (!NT_SUCCESS(status)) {
            ZwClose(deviceParameterHandle);
        }

    }

    if (NT_SUCCESS(status)) {

        status = RtlWriteRegistryValue(
            RTL_REGISTRY_HANDLE,
            (PWSTR) (SubkeyName ?
                     deviceSubkeyHandle :
                     deviceParameterHandle),
            ParameterName,
            REG_DWORD,
            &ParameterValue,
            sizeof(ULONG));

        //
        // close what we open
        //

        if (SubkeyName) {
            ZwClose(deviceSubkeyHandle);
        }

        ZwClose(deviceParameterHandle);
    }

    return status;

} // end ClassSetDeviceParameter()


/*
 *  ClassScanForSpecial
 *
 *      This routine was written to simplify scanning for special
 *      hardware based upon id strings.  it does not check the registry.
 */

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassScanForSpecial(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    _In_ CLASSPNP_SCAN_FOR_SPECIAL_INFO DeviceList[],
    _In_ PCLASS_SCAN_FOR_SPECIAL_HANDLER Function)
{
    PSTORAGE_DEVICE_DESCRIPTOR deviceDescriptor;
    PUCHAR vendorId;
    PUCHAR productId;
    PUCHAR productRevision;
    UCHAR nullString[] = "";

    PAGED_CODE();
    NT_ASSERT(DeviceList);
    NT_ASSERT(Function);

    deviceDescriptor = FdoExtension->DeviceDescriptor;

    if (DeviceList == NULL) {
        return;
    }
    if (Function == NULL) {
        return;
    }

    //
    // SCSI sets offsets to -1, ATAPI sets to 0.  check for both.
    //

    if (deviceDescriptor->VendorIdOffset != 0 &&
        deviceDescriptor->VendorIdOffset != -1) {
        vendorId = ((PUCHAR)deviceDescriptor);
        vendorId += deviceDescriptor->VendorIdOffset;
    } else {
        vendorId = nullString;
    }
    if (deviceDescriptor->ProductIdOffset != 0 &&
        deviceDescriptor->ProductIdOffset != -1) {
        productId = ((PUCHAR)deviceDescriptor);
        productId += deviceDescriptor->ProductIdOffset;
    } else {
        productId = nullString;
    }
    if (deviceDescriptor->ProductRevisionOffset != 0 &&
        deviceDescriptor->ProductRevisionOffset != -1) {
        productRevision = ((PUCHAR)deviceDescriptor);
        productRevision += deviceDescriptor->ProductRevisionOffset;
    } else {
        productRevision = nullString;
    }

    //
    // loop while the device list is valid (not null-filled)
    //

    for (;(DeviceList->VendorId        != NULL ||
           DeviceList->ProductId       != NULL ||
           DeviceList->ProductRevision != NULL);DeviceList++) {

        if (ClasspMyStringMatches(DeviceList->VendorId,        (PCHAR)vendorId) &&
            ClasspMyStringMatches(DeviceList->ProductId,       (PCHAR)productId) &&
            ClasspMyStringMatches(DeviceList->ProductRevision, (PCHAR)productRevision)
            ) {

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT, "ClasspScanForSpecialByInquiry: Found matching "
                        "controller Ven: %s Prod: %s Rev: %s\n",
                        (PCSZ)vendorId, (PCSZ)productId, (PCSZ)productRevision));

            //
            // pass the context to the call back routine and exit
            //

            (Function)(FdoExtension, DeviceList->Data);

            //
            // for CHK builds, try to prevent wierd stacks by having a debug
            // print here. it's a hack, but i know of no other way to prevent
            // the stack from being wrong.
            //

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT, "ClasspScanForSpecialByInquiry: "
                        "completed callback\n"));
            return;

        } // else the strings did not match

    } // none of the devices matched.

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT, "ClasspScanForSpecialByInquiry: no match found for %p\n",
                FdoExtension->DeviceObject));
    return;

} // end ClasspScanForSpecialByInquiry()


//
// In order to provide better performance without the need to reboot,
// we need to implement a self-adjusting method to set and clear the
// srb flags based upon current performance.
//
// whenever there is an error, immediately grab the spin lock.  the
// MP perf hit here is acceptable, since we're in an error path.  this
// is also neccessary because we are guaranteed to be modifying the
// SRB flags here, setting SuccessfulIO to zero, and incrementing the
// actual error count (which is always done within this spinlock).
//
// whenever there is no error, increment a counter.  if there have been
// errors on the device, and we've enabled dynamic perf, *and* we've
// just crossed the perf threshhold, then grab the spin lock and
// double check that the threshhold has, indeed been hit(*). then
// decrement the error count, and if it's dropped sufficiently, undo
// some of the safety changes made in the SRB flags due to the errors.
//
// * this works in all cases.  even if lots of ios occur after the
//   previous guy went in and cleared the successfulio counter, that
//   just means that we've hit the threshhold again, and so it's proper
//   to run the inner loop again.
//

VOID
ClasspPerfIncrementErrorCount(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    PCLASS_PRIVATE_FDO_DATA fdoData = FdoExtension->PrivateFdoData;
    KIRQL oldIrql;
    ULONG errors;

    KeAcquireSpinLock(&fdoData->SpinLock, &oldIrql);

    fdoData->Perf.SuccessfulIO = 0; // implicit interlock
    errors = InterlockedIncrement((volatile LONG *)&FdoExtension->ErrorCount);

    if (!fdoData->DisableThrottling) {

        if (errors >= CLASS_ERROR_LEVEL_1) {

            //
            // If the error count has exceeded the error limit, then disable
            // any tagged queuing, multiple requests per lu queueing
            // and sychronous data transfers.
            //
            // Clearing the no queue freeze flag prevents the port driver
            // from sending multiple requests per logical unit.
            //

            CLEAR_FLAG(FdoExtension->SrbFlags, SRB_FLAGS_NO_QUEUE_FREEZE);
            CLEAR_FLAG(FdoExtension->SrbFlags, SRB_FLAGS_QUEUE_ACTION_ENABLE);

            SET_FLAG(FdoExtension->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);

            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL, "ClasspPerfIncrementErrorCount: "
                        "Too many errors; disabling tagged queuing and "
                        "synchronous data tranfers.\n"));

        }

        if (errors >= CLASS_ERROR_LEVEL_2) {

            //
            // If a second threshold is reached, disable disconnects.
            //

            SET_FLAG(FdoExtension->SrbFlags, SRB_FLAGS_DISABLE_DISCONNECT);
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL, "ClasspPerfIncrementErrorCount: "
                        "Too many errors; disabling disconnects.\n"));
        }
    }

    KeReleaseSpinLock(&fdoData->SpinLock, oldIrql);
    return;
}

VOID
ClasspPerfIncrementSuccessfulIo(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    PCLASS_PRIVATE_FDO_DATA fdoData = FdoExtension->PrivateFdoData;
    KIRQL oldIrql;
    ULONG errors;
    ULONG succeeded = 0;

    //
    // don't take a hit from the interlocked op unless we're in
    // a degraded state and we've got a threshold to hit.
    //

    if (FdoExtension->ErrorCount == 0) {
        return;
    }

    if (fdoData->Perf.ReEnableThreshhold == 0) {
        return;
    }

    succeeded = InterlockedIncrement((volatile LONG *)&fdoData->Perf.SuccessfulIO);
    if (succeeded < fdoData->Perf.ReEnableThreshhold) {
        return;
    }

    //
    // if we hit the threshold, grab the spinlock and verify we've
    // actually done so.  this allows us to ignore the spinlock 99%
    // of the time.
    //

    KeAcquireSpinLock(&fdoData->SpinLock, &oldIrql);

    //
    // re-read the value, so we don't run this multiple times
    // for a single threshhold being hit.  this keeps errorcount
    // somewhat useful.
    //

    succeeded = fdoData->Perf.SuccessfulIO;

    if ((FdoExtension->ErrorCount != 0) &&
        (fdoData->Perf.ReEnableThreshhold <= succeeded)
        ) {

        fdoData->Perf.SuccessfulIO = 0; // implicit interlock

        NT_ASSERT(FdoExtension->ErrorCount > 0);
        errors = InterlockedDecrement((volatile LONG *)&FdoExtension->ErrorCount);

        //
        // note: do in reverse order of the sets "just in case"
        //

        if (errors < CLASS_ERROR_LEVEL_2) {
            if (errors == CLASS_ERROR_LEVEL_2 - 1) {
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL, "ClasspPerfIncrementSuccessfulIo: "
                            "Error level 2 no longer required.\n"));
            }
            if (!TEST_FLAG(fdoData->Perf.OriginalSrbFlags,
                           SRB_FLAGS_DISABLE_DISCONNECT)) {
                CLEAR_FLAG(FdoExtension->SrbFlags,
                           SRB_FLAGS_DISABLE_DISCONNECT);
            }
        }

        if (errors < CLASS_ERROR_LEVEL_1) {
            if (errors == CLASS_ERROR_LEVEL_1 - 1) {
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL, "ClasspPerfIncrementSuccessfulIo: "
                            "Error level 1 no longer required.\n"));
            }
            if (!TEST_FLAG(fdoData->Perf.OriginalSrbFlags,
                           SRB_FLAGS_DISABLE_SYNCH_TRANSFER)) {
                CLEAR_FLAG(FdoExtension->SrbFlags,
                           SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
            }
            if (TEST_FLAG(fdoData->Perf.OriginalSrbFlags,
                          SRB_FLAGS_QUEUE_ACTION_ENABLE)) {
                SET_FLAG(FdoExtension->SrbFlags,
                         SRB_FLAGS_QUEUE_ACTION_ENABLE);
            }
            if (TEST_FLAG(fdoData->Perf.OriginalSrbFlags,
                          SRB_FLAGS_NO_QUEUE_FREEZE)) {
                SET_FLAG(FdoExtension->SrbFlags,
                         SRB_FLAGS_NO_QUEUE_FREEZE);
            }
        }
    } // end of threshhold definitely being hit for first time

    KeReleaseSpinLock(&fdoData->SpinLock, oldIrql);
    return;
}


PMDL ClasspBuildDeviceMdl(PVOID Buffer, ULONG BufferLen, BOOLEAN WriteToDevice)
{
    PMDL mdl;

    mdl = IoAllocateMdl(Buffer, BufferLen, FALSE, FALSE, NULL);
    if (mdl){
        _SEH2_TRY {
            MmProbeAndLockPages(mdl, KernelMode, WriteToDevice ? IoReadAccess : IoWriteAccess);
#ifdef _MSC_VER
        #pragma warning(suppress: 6320) // We want to handle any exception that MmProbeAndLockPages might throw
#endif
        } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
            NTSTATUS status = _SEH2_GetExceptionCode();

            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT, "ClasspBuildDeviceMdl: MmProbeAndLockPages failed with %xh.", status));
            IoFreeMdl(mdl);
            mdl = NULL;
        } _SEH2_END;
    }
    else {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT, "ClasspBuildDeviceMdl: IoAllocateMdl failed"));
    }

    return mdl;
}


PMDL BuildDeviceInputMdl(PVOID Buffer, ULONG BufferLen)
{
    return ClasspBuildDeviceMdl(Buffer, BufferLen, FALSE);
}


VOID ClasspFreeDeviceMdl(PMDL Mdl)
{
    MmUnlockPages(Mdl);
    IoFreeMdl(Mdl);
}


VOID FreeDeviceInputMdl(PMDL Mdl)
{
    ClasspFreeDeviceMdl(Mdl);
    return;
}


#if 0
    VOID
    ClasspPerfResetCounters(
        IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
        )
    {
        PCLASS_PRIVATE_FDO_DATA fdoData = FdoExtension->PrivateFdoData;
        KIRQL oldIrql;

        KeAcquireSpinLock(&fdoData->SpinLock, &oldIrql);
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL, "ClasspPerfResetCounters: "
                    "Resetting all perf counters.\n"));
        fdoData->Perf.SuccessfulIO = 0;
        FdoExtension->ErrorCount = 0;

        if (!TEST_FLAG(fdoData->Perf.OriginalSrbFlags,
                       SRB_FLAGS_DISABLE_DISCONNECT)) {
            CLEAR_FLAG(FdoExtension->SrbFlags,
                       SRB_FLAGS_DISABLE_DISCONNECT);
        }
        if (!TEST_FLAG(fdoData->Perf.OriginalSrbFlags,
                       SRB_FLAGS_DISABLE_SYNCH_TRANSFER)) {
            CLEAR_FLAG(FdoExtension->SrbFlags,
                       SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
        }
        if (TEST_FLAG(fdoData->Perf.OriginalSrbFlags,
                      SRB_FLAGS_QUEUE_ACTION_ENABLE)) {
            SET_FLAG(FdoExtension->SrbFlags,
                     SRB_FLAGS_QUEUE_ACTION_ENABLE);
        }
        if (TEST_FLAG(fdoData->Perf.OriginalSrbFlags,
                      SRB_FLAGS_NO_QUEUE_FREEZE)) {
            SET_FLAG(FdoExtension->SrbFlags,
                     SRB_FLAGS_NO_QUEUE_FREEZE);
        }
        KeReleaseSpinLock(&fdoData->SpinLock, oldIrql);
        return;
    }
#endif


/*++

ClasspDuidGetDeviceIdProperty

Routine Description:

    Add StorageDeviceIdProperty to the device unique ID structure.

Arguments:

    DeviceObject - a pointer to the device object
    Irp - a pointer to the I/O request packet

Return Value:

    Status Code

--*/
NTSTATUS
ClasspDuidGetDeviceIdProperty(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PSTORAGE_DEVICE_ID_DESCRIPTOR deviceIdDescriptor = NULL;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSTORAGE_DESCRIPTOR_HEADER descHeader;
    PSTORAGE_DEVICE_UNIQUE_IDENTIFIER storageDuid;
    PUCHAR dest;

    STORAGE_PROPERTY_ID propertyId = StorageDeviceIdProperty;

    NTSTATUS status;

    ULONG queryLength;
    ULONG offset;

    //
    // Get the VPD page 83h data.
    //

    status = ClassGetDescriptor(commonExtension->LowerDeviceObject,
                                &propertyId,
                                (PVOID *)&deviceIdDescriptor);

    if (!NT_SUCCESS(status) || !deviceIdDescriptor) {
        goto FnExit;
    }

    queryLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    descHeader = Irp->AssociatedIrp.SystemBuffer;

    //
    // Adjust required size and potential destination location.
    //

    offset = descHeader->Size;
    dest = (PUCHAR)descHeader + offset;

    descHeader->Size += deviceIdDescriptor->Size;

    if (queryLength < descHeader->Size) {

        //
        // Output buffer is too small.  Return error and make sure
        // the caller gets info about required buffer size.
        //

        Irp->IoStatus.Information = sizeof(STORAGE_DESCRIPTOR_HEADER);
        status = STATUS_BUFFER_OVERFLOW;
        goto FnExit;
    }

    storageDuid = Irp->AssociatedIrp.SystemBuffer;
    storageDuid->StorageDeviceIdOffset = offset;

    RtlCopyMemory(dest,
                  deviceIdDescriptor,
                  deviceIdDescriptor->Size);

    Irp->IoStatus.Information = storageDuid->Size;
    status = STATUS_SUCCESS;

FnExit:

    FREE_POOL(deviceIdDescriptor);

    return status;
}



/*++

ClasspDuidGetDeviceProperty

Routine Description:

    Add StorageDeviceProperty to the device unique ID structure.

Arguments:

    DeviceObject - a pointer to the device object
    Irp - a pointer to the I/O request packet

Return Value:

    Status Code

--*/
NTSTATUS
ClasspDuidGetDeviceProperty(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PSTORAGE_DEVICE_DESCRIPTOR deviceDescriptor = fdoExtension->DeviceDescriptor;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSTORAGE_DESCRIPTOR_HEADER descHeader;
    PSTORAGE_DEVICE_UNIQUE_IDENTIFIER storageDuid;
    PUCHAR dest;

    NTSTATUS status = STATUS_NOT_FOUND;

    ULONG queryLength;
    ULONG offset;

    //
    // Use the StorageDeviceProperty already cached in the device extension.
    //

    if (!deviceDescriptor) {
        goto FnExit;
    }

    queryLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    descHeader = Irp->AssociatedIrp.SystemBuffer;

    //
    // Use this info only if serial number is available.
    //

    if (deviceDescriptor->SerialNumberOffset == 0) {
        goto FnExit;
    }

    //
    // Adjust required size and potential destination location.
    //

    offset = descHeader->Size;
    dest = (PUCHAR)descHeader + offset;

    descHeader->Size += deviceDescriptor->Size;

    if (queryLength < descHeader->Size) {

        //
        // Output buffer is too small.  Return error and make sure
        // the caller get info about required buffer size.
        //

        Irp->IoStatus.Information = sizeof(STORAGE_DESCRIPTOR_HEADER);
        status = STATUS_BUFFER_OVERFLOW;
        goto FnExit;
    }

    storageDuid = Irp->AssociatedIrp.SystemBuffer;
    storageDuid->StorageDeviceOffset = offset;

    RtlCopyMemory(dest,
                  deviceDescriptor,
                  deviceDescriptor->Size);

    Irp->IoStatus.Information = storageDuid->Size;
    status = STATUS_SUCCESS;

FnExit:

    return status;
}


/*++

ClasspDuidGetDriveLayout

Routine Description:

    Add drive layout signature to the device unique ID structure.
    Layout signature is only added for disk-type devices.

Arguments:

    DeviceObject - a pointer to the device object
    Irp - a pointer to the I/O request packet

Return Value:

    Status Code

--*/
NTSTATUS
ClasspDuidGetDriveLayout(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
    PDRIVE_LAYOUT_INFORMATION_EX layoutEx = NULL;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSTORAGE_DESCRIPTOR_HEADER descHeader;
    PSTORAGE_DEVICE_UNIQUE_IDENTIFIER storageDuid;
    PSTORAGE_DEVICE_LAYOUT_SIGNATURE driveLayoutSignature;

    NTSTATUS status = STATUS_NOT_FOUND;

    ULONG queryLength;
    ULONG offset;

    //
    // Only process disk-type devices.
    //

    if (DeviceObject->DeviceType != FILE_DEVICE_DISK) {
        goto FnExit;
    }

    //
    // Get current partition table and process only if GPT
    // or MBR layout.
    //

    status = IoReadPartitionTableEx(DeviceObject, &layoutEx);

    if (!NT_SUCCESS(status)) {
        status = STATUS_NOT_FOUND;
        goto FnExit;
    }

    if (layoutEx->PartitionStyle != PARTITION_STYLE_GPT &&
        layoutEx->PartitionStyle != PARTITION_STYLE_MBR) {
        status = STATUS_NOT_FOUND;
        goto FnExit;
    }

    queryLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    descHeader = Irp->AssociatedIrp.SystemBuffer;

    //
    // Adjust required size and potential destination location.
    //

    offset = descHeader->Size;
    driveLayoutSignature = (PSTORAGE_DEVICE_LAYOUT_SIGNATURE)((PUCHAR)descHeader + offset);

    descHeader->Size += sizeof(STORAGE_DEVICE_LAYOUT_SIGNATURE);

    if (queryLength < descHeader->Size) {

        //
        // Output buffer is too small.  Return error and make sure
        // the caller get info about required buffer size.
        //

        Irp->IoStatus.Information = sizeof(STORAGE_DESCRIPTOR_HEADER);
        status = STATUS_BUFFER_OVERFLOW;
        goto FnExit;
    }

    storageDuid = Irp->AssociatedIrp.SystemBuffer;

    driveLayoutSignature->Size = sizeof(STORAGE_DEVICE_LAYOUT_SIGNATURE);
    driveLayoutSignature->Version = DUID_VERSION_1;

    if (layoutEx->PartitionStyle == PARTITION_STYLE_MBR) {

        driveLayoutSignature->Mbr = TRUE;

        RtlCopyMemory(&driveLayoutSignature->DeviceSpecific.MbrSignature,
                      &layoutEx->Mbr.Signature,
                      sizeof(layoutEx->Mbr.Signature));

    } else {

        driveLayoutSignature->Mbr = FALSE;

        RtlCopyMemory(&driveLayoutSignature->DeviceSpecific.GptDiskId,
                      &layoutEx->Gpt.DiskId,
                      sizeof(layoutEx->Gpt.DiskId));
    }

    storageDuid->DriveLayoutSignatureOffset = offset;

    Irp->IoStatus.Information = storageDuid->Size;
    status = STATUS_SUCCESS;


FnExit:

    FREE_POOL(layoutEx);

    return status;
}


/*++

ClasspDuidQueryProperty

Routine Description:

    Handles IOCTL_STORAGE_QUERY_PROPERTY for device unique ID requests
    (when PropertyId is StorageDeviceUniqueIdProperty).

Arguments:

    DeviceObject - a pointer to the device object
    Irp - a pointer to the I/O request packet

Return Value:

    Status Code

--*/
NTSTATUS
ClasspDuidQueryProperty(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
    PSTORAGE_PROPERTY_QUERY query =  Irp->AssociatedIrp.SystemBuffer;
    PSTORAGE_DESCRIPTOR_HEADER descHeader;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    NTSTATUS status;

    ULONG outLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    BOOLEAN includeOptionalIds;
    BOOLEAN overflow = FALSE;
    BOOLEAN infoFound = FALSE;
    BOOLEAN useStatus = TRUE;   // Use the status directly instead of relying on overflow and infoFound flags.

    //
    // Must run at less then dispatch.
    //

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {

        NT_ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
        status = STATUS_INVALID_LEVEL;
        goto FnExit;
    }

    //
    // Check proper query type.
    //

    if (query->QueryType == PropertyExistsQuery) {
        Irp->IoStatus.Information = 0;
        status = STATUS_SUCCESS;
        goto FnExit;
    }

    if (query->QueryType != PropertyStandardQuery) {
        status = STATUS_NOT_SUPPORTED;
        goto FnExit;
    }

    //
    // Check AdditionalParameters validity.
    //

    if (query->AdditionalParameters[0] == DUID_INCLUDE_SOFTWARE_IDS) {
        includeOptionalIds = TRUE;
    } else if (query->AdditionalParameters[0] == DUID_HARDWARE_IDS_ONLY) {
        includeOptionalIds = FALSE;
    } else {
        status = STATUS_INVALID_PARAMETER;
        goto FnExit;
    }

    //
    // Verify output parameters.
    //

    if (outLength < sizeof(STORAGE_DESCRIPTOR_HEADER)) {

        status = STATUS_INFO_LENGTH_MISMATCH;
        goto FnExit;
    }

    //
    // From this point forward the status depends on the overflow
    // and infoFound flags.
    //

    useStatus = FALSE;

    descHeader = Irp->AssociatedIrp.SystemBuffer;
    RtlZeroMemory(descHeader, outLength);

    descHeader->Version = DUID_VERSION_1;
    descHeader->Size = sizeof(STORAGE_DEVICE_UNIQUE_IDENTIFIER);

    //
    // Try to build device unique id from StorageDeviceIdProperty.
    //

    status = ClasspDuidGetDeviceIdProperty(DeviceObject,
                                           Irp);

    if (status == STATUS_BUFFER_OVERFLOW) {
        overflow = TRUE;
    }

    if (NT_SUCCESS(status)) {
        infoFound = TRUE;
    }

    //
    // Try to build device unique id from StorageDeviceProperty.
    //

    status = ClasspDuidGetDeviceProperty(DeviceObject,
                                         Irp);

    if (status == STATUS_BUFFER_OVERFLOW) {
        overflow = TRUE;
    }

    if (NT_SUCCESS(status)) {
        infoFound = TRUE;
    }

    //
    // The following portion is optional and only included if the
    // caller requested software IDs.
    //

    if (!includeOptionalIds) {
        goto FnExit;
    }

    //
    // Try to build device unique id from drive layout signature (disk
    // devices only).
    //

    status = ClasspDuidGetDriveLayout(DeviceObject,
                                      Irp);

    if (status == STATUS_BUFFER_OVERFLOW) {
        overflow = TRUE;
    }

    if (NT_SUCCESS(status)) {
        infoFound = TRUE;
    }

FnExit:

    if (!useStatus) {

        //
        // Return overflow, success, or a generic error.
        //

        if (overflow) {

            //
            // If output buffer is STORAGE_DESCRIPTOR_HEADER, then return
            // success to the user.  Otherwise, send an error so the user
            // knows a larger buffer is required.
            //

            if (outLength == sizeof(STORAGE_DESCRIPTOR_HEADER)) {
                status = STATUS_SUCCESS;
                Irp->IoStatus.Information = sizeof(STORAGE_DESCRIPTOR_HEADER);
            } else {
                status = STATUS_BUFFER_OVERFLOW;
            }

        } else if (infoFound) {
            status = STATUS_SUCCESS;

            //
            // Exercise the compare routine.  This should always succeed.
            //

            NT_ASSERT(DuidExactMatch == CompareStorageDuids(Irp->AssociatedIrp.SystemBuffer,
                                                         Irp->AssociatedIrp.SystemBuffer));

        } else {
            status = STATUS_NOT_FOUND;
        }
    }

    Irp->IoStatus.Status = status;

    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

    return status;
}

/*++////////////////////////////////////////////////////////////////////////////

ClasspWriteCacheProperty()

Routine Description:

    This routine reads the caching mode page from the device to
    build the Write Cache property page.

Arguments:

    DeviceObject - The device object to handle this irp

    Irp - The IRP for this request

    Srb - SRB allocated by the dispatch routine

Return Value:

--*/

NTSTATUS ClasspWriteCacheProperty(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PSTORAGE_WRITE_CACHE_PROPERTY writeCache;
    PSTORAGE_PROPERTY_QUERY query =  Irp->AssociatedIrp.SystemBuffer;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PMODE_PARAMETER_HEADER modeData = NULL;
    PMODE_CACHING_PAGE pageData = NULL;
    ULONG length, information = 0;
    NTSTATUS status;
    PCDB cdb;

    //
    // Must run at less then dispatch.
    //

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {

        NT_ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
        status = STATUS_INVALID_LEVEL;
        goto WriteCacheExit;
    }

    //
    // Check proper query type.
    //

    if (query->QueryType == PropertyExistsQuery) {
        status = STATUS_SUCCESS;
        goto WriteCacheExit;
    }

    if (query->QueryType != PropertyStandardQuery) {
        status = STATUS_NOT_SUPPORTED;
        goto WriteCacheExit;
    }

    length = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (length < sizeof(STORAGE_DESCRIPTOR_HEADER)) {
        status = STATUS_INFO_LENGTH_MISMATCH;
        goto WriteCacheExit;
    }

    writeCache = (PSTORAGE_WRITE_CACHE_PROPERTY) Irp->AssociatedIrp.SystemBuffer;
    RtlZeroMemory(writeCache, length);

    //
    // Set version and required size.
    //

    writeCache->Version = sizeof(STORAGE_WRITE_CACHE_PROPERTY);
    writeCache->Size = sizeof(STORAGE_WRITE_CACHE_PROPERTY);

    if (length < sizeof(STORAGE_WRITE_CACHE_PROPERTY)) {
        information = sizeof(STORAGE_DESCRIPTOR_HEADER);
        status = STATUS_SUCCESS;
        goto WriteCacheExit;
    }

    //
    // Set known values
    //

    writeCache->NVCacheEnabled = FALSE;
    writeCache->UserDefinedPowerProtection = TEST_FLAG(fdoExtension->DeviceFlags, DEV_POWER_PROTECTED);

    //
    // Check for flush cache support by sending a sync cache command
    // to the device.
    //

    //
    // Set timeout value and mark the request as not being a tagged request.
    //
    SrbSetTimeOutValue(Srb, fdoExtension->TimeOutValue * 4);
    SrbSetRequestTag(Srb, SP_UNTAGGED);
    SrbSetRequestAttribute(Srb, SRB_SIMPLE_TAG_REQUEST);
    SrbAssignSrbFlags(Srb, fdoExtension->SrbFlags);

    SrbSetCdbLength(Srb, 10);
    cdb = SrbGetCdb(Srb);
    cdb->CDB10.OperationCode = SCSIOP_SYNCHRONIZE_CACHE;

    status = ClassSendSrbSynchronous(DeviceObject,
                                     Srb,
                                     NULL,
                                     0,
                                     TRUE);
    if (NT_SUCCESS(status)) {
        writeCache->FlushCacheSupported = TRUE;
    } else {
        //
        // Device does not support sync cache
        //

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "ClasspWriteCacheProperty: Synchronize cache failed with status 0x%X\n", status));
        writeCache->FlushCacheSupported = FALSE;
        //
        // Reset the status if there was any failure
        //
        status = STATUS_SUCCESS;
    }

    modeData = ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned,
                                     MODE_PAGE_DATA_SIZE,
                                     CLASS_TAG_MODE_DATA);

    if (modeData == NULL) {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL, "ClasspWriteCacheProperty: Unable to allocate mode data buffer\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto WriteCacheExit;
    }

    RtlZeroMemory(modeData, MODE_PAGE_DATA_SIZE);

    length = ClassModeSense(DeviceObject,
                            (PCHAR) modeData,
                            MODE_PAGE_DATA_SIZE,
                            MODE_PAGE_CACHING);

    if (length < sizeof(MODE_PARAMETER_HEADER)) {

        //
        // Retry the request in case of a check condition.
        //

        length = ClassModeSense(DeviceObject,
                                (PCHAR) modeData,
                                MODE_PAGE_DATA_SIZE,
                                MODE_PAGE_CACHING);

        if (length < sizeof(MODE_PARAMETER_HEADER)) {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL, "ClasspWriteCacheProperty: Mode Sense failed\n"));
            status = STATUS_IO_DEVICE_ERROR;
            goto WriteCacheExit;
        }
    }

    //
    // If the length is greater than length indicated by the mode data reset
    // the data to the mode data.
    //

    if (length > (ULONG) (modeData->ModeDataLength + 1)) {
        length = modeData->ModeDataLength + 1;
    }

    //
    // Look for caching page in the returned mode page data.
    //

    pageData = ClassFindModePage((PCHAR) modeData,
                                 length,
                                 MODE_PAGE_CACHING,
                                 TRUE);

    //
    // Check if valid caching page exists.
    //

    if (pageData == NULL) {

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "ClasspWriteCacheProperty: Unable to find caching mode page.\n"));
        //
        // Set write cache value as unknown.
        //
        writeCache->WriteCacheEnabled = WriteCacheEnableUnknown;
        writeCache->WriteCacheType = WriteCacheTypeUnknown;
    } else {
        writeCache->WriteCacheEnabled = pageData->WriteCacheEnable ?
                                            WriteCacheEnabled : WriteCacheDisabled;

        writeCache->WriteCacheType = pageData->WriteCacheEnable ?
                                            WriteCacheTypeWriteBack : WriteCacheTypeUnknown;
    }

    //
    // Check write through support. If the device previously failed a write request
    // with FUA bit is set, then CLASS_SPECIAL_FUA_NOT_SUPPORTED will be set,
    // which means write through is not support by the device.
    //

    if ((modeData->DeviceSpecificParameter & MODE_DSP_FUA_SUPPORTED) &&
        (!TEST_FLAG(fdoExtension->ScanForSpecialFlags, CLASS_SPECIAL_FUA_NOT_SUPPORTED))) {
        writeCache->WriteThroughSupported = WriteThroughSupported;
    } else {
        writeCache->WriteThroughSupported = WriteThroughNotSupported;
    }

    //
    // Get the changeable caching mode page and check write cache is changeable.
    //

    RtlZeroMemory(modeData, MODE_PAGE_DATA_SIZE);

    length = ClasspModeSense(DeviceObject,
                            (PCHAR) modeData,
                            MODE_PAGE_DATA_SIZE,
                            MODE_PAGE_CACHING,
                            MODE_SENSE_CHANGEABLE_VALUES);

    if (length < sizeof(MODE_PARAMETER_HEADER)) {

        //
        // Retry the request in case of a check condition.
        //

        length = ClasspModeSense(DeviceObject,
                                (PCHAR) modeData,
                                MODE_PAGE_DATA_SIZE,
                                MODE_PAGE_CACHING,
                                MODE_SENSE_CHANGEABLE_VALUES);

        if (length < sizeof(MODE_PARAMETER_HEADER)) {

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "ClasspWriteCacheProperty: Mode Sense failed\n"));

            //
            // If the device fails to return changeable pages, then
            // set the write cache changeable value to unknown.
            //

            writeCache->WriteCacheChangeable = WriteCacheChangeUnknown;
            information = sizeof(STORAGE_WRITE_CACHE_PROPERTY);
            goto WriteCacheExit;
        }
    }

    //
    // If the length is greater than length indicated by the mode data reset
    // the data to the mode data.
    //

    if (length > (ULONG) (modeData->ModeDataLength + 1)) {
        length = modeData->ModeDataLength + 1;
    }

    //
    // Look for caching page in the returned mode page data.
    //

    pageData = ClassFindModePage((PCHAR) modeData,
                                 length,
                                 MODE_PAGE_CACHING,
                                 TRUE);
    //
    // Check if valid caching page exists.
    //

    if (pageData == NULL) {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "ClasspWriteCacheProperty: Unable to find caching mode page.\n"));
        //
        // Set write cache changeable value to unknown.
        //
        writeCache->WriteCacheChangeable = WriteCacheChangeUnknown;
    } else {
        writeCache->WriteCacheChangeable = pageData->WriteCacheEnable ?
                                            WriteCacheChangeable : WriteCacheNotChangeable;
    }

    information = sizeof(STORAGE_WRITE_CACHE_PROPERTY);

WriteCacheExit:

    FREE_POOL(modeData);

    //
    // Set the size and status in IRP
    //
    Irp->IoStatus.Information = information;;
    Irp->IoStatus.Status = status;

    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

    return status;
}

ULONG
ClasspCalculateLogicalSectorSize (
    _In_ PDEVICE_OBJECT Fdo,
    _In_ ULONG          BytesPerBlockInBigEndian
    )
/*++
    Convert the big-endian value.
    if it's 0, default to the standard 512 bytes.
    if it's not a power of 2 value, round down to power of 2.
--*/
{
    ULONG logicalSectorSize;

    REVERSE_BYTES(&logicalSectorSize, &BytesPerBlockInBigEndian);

    if (logicalSectorSize == 0) {
        logicalSectorSize = 512;
    } else {
        //
        //  Clear all but the highest set bit.
        //  That will give us a bytesPerSector value that is a power of 2.
        //
        if (logicalSectorSize & (logicalSectorSize-1)) {
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT, "FDO %ph has non-standard sector size 0x%x.", Fdo, logicalSectorSize));
            do {
                logicalSectorSize &= logicalSectorSize-1;
            }
            while (logicalSectorSize & (logicalSectorSize-1));
        }
    }

    return logicalSectorSize;
}

NTSTATUS
InterpretReadCapacity16Data (
    _Inout_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    _In_ PREAD_CAPACITY16_DATA ReadCapacity16Data
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT   lowestAlignedBlock;
    USHORT   logicalBlocksPerPhysicalBlock;
    PCLASS_READ_CAPACITY16_DATA cachedData = &(FdoExtension->FunctionSupportInfo->ReadCapacity16Data);

    // use Logical Sector Size from DiskGeometry to avoid duplicated calculation.
    FdoExtension->FunctionSupportInfo->ReadCapacity16Data.BytesPerLogicalSector = ClasspCalculateLogicalSectorSize(FdoExtension->DeviceObject, ReadCapacity16Data->BytesPerBlock);

    // FdoExtension->DiskGeometry.BytesPerSector might be 0 for class drivers that don't get READ CAPACITY info yet.
    NT_ASSERT( (FdoExtension->DiskGeometry.BytesPerSector == 0) ||
               (FdoExtension->DiskGeometry.BytesPerSector == FdoExtension->FunctionSupportInfo->ReadCapacity16Data.BytesPerLogicalSector) );

    logicalBlocksPerPhysicalBlock = 1 << ReadCapacity16Data->LogicalPerPhysicalExponent;
    lowestAlignedBlock = (ReadCapacity16Data->LowestAlignedBlock_MSB << 8) | ReadCapacity16Data->LowestAlignedBlock_LSB;

    if (lowestAlignedBlock > logicalBlocksPerPhysicalBlock) {
        // we get garbage data
        status = STATUS_UNSUCCESSFUL;
    } else {
        // value of lowestAlignedBlock (from T10 spec) needs to be converted.
        lowestAlignedBlock = (logicalBlocksPerPhysicalBlock - lowestAlignedBlock) % logicalBlocksPerPhysicalBlock;
    }

    if (NT_SUCCESS(status)) {
        // fill output buffer
        cachedData->BytesPerPhysicalSector = cachedData->BytesPerLogicalSector * logicalBlocksPerPhysicalBlock;
        cachedData->BytesOffsetForSectorAlignment = cachedData->BytesPerLogicalSector * lowestAlignedBlock;

        //
        // Fill in the Logical Block Provisioning info.  Note that we do not
        // use these fields; we use the Provisioning Type and LBPRZ fields from
        // the Logical Block Provisioning VPD page (0xB2).
        //
        cachedData->LBProvisioningEnabled = ReadCapacity16Data->LBPME;
        cachedData->LBProvisioningReadZeros = ReadCapacity16Data->LBPRZ;

        TracePrint((TRACE_LEVEL_INFORMATION,
                    TRACE_FLAG_INIT,
                    "InterpretReadCapacity16Data: Device\'s LBP enabled = %d\n",
                    cachedData->LBProvisioningEnabled));
    }

    return status;
}

NTSTATUS
ClassReadCapacity16 (
    _Inout_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    )
/*
    This routine may send down a READ CAPACITY 16 command to retrieve info.
    The info will be cached in FdoExtension->LowerLayerSupport->AccessAlignment.

    After info retrieving finished, this function sets following field:
        FdoExtension->LowerLayerSupport->AccessAlignment.LowerLayerSupported = Supported;
    to indicate that info has been cached.

    NOTE: some future processes may use this function to send the command anyway, it will be caller's decision
          on checking 'AccessAlignment.LowerLayerSupported' in case the cached info is good enough.
*/
{
    NTSTATUS              status = STATUS_SUCCESS;
    PREAD_CAPACITY16_DATA dataBuffer = NULL;
    UCHAR                 bufferLength = sizeof(READ_CAPACITY16_DATA);
    ULONG                 allocationBufferLength = bufferLength; //DMA buffer size for alignment
    PCDB                  cdb;
    ULONG                 dataTransferLength = 0;

    //
    // If the information retrieval has already been attempted, return the cached status.
    //
    if (FdoExtension->FunctionSupportInfo->ReadCapacity16Data.CommandStatus != -1) {
        // get cached NTSTATUS from previous call.
        return FdoExtension->FunctionSupportInfo->ReadCapacity16Data.CommandStatus;
    }

    if (ClasspIsObsoletePortDriver(FdoExtension)) {
        FdoExtension->FunctionSupportInfo->ReadCapacity16Data.CommandStatus = STATUS_NOT_IMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    }

#if defined(_ARM_) || defined(_ARM64_)
    //
    // ARM has specific alignment requirements, although this will not have a functional impact on x86 or amd64
    // based platforms. We are taking the conservative approach here.
    //
    allocationBufferLength = ALIGN_UP_BY(allocationBufferLength,KeGetRecommendedSharedDataAlignment());
    dataBuffer = (PREAD_CAPACITY16_DATA)ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned, allocationBufferLength, '4CcS');
#else
    dataBuffer = (PREAD_CAPACITY16_DATA)ExAllocatePoolWithTag(NonPagedPoolNx, bufferLength, '4CcS');
#endif

    if (dataBuffer == NULL) {
        // return without updating FdoExtension->FunctionSupportInfo->ReadCapacity16Data.CommandStatus
        // the field will remain value as "-1", so that the command will be attempted next time this function is called.
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(dataBuffer, allocationBufferLength);

    //
    // Initialize the SRB.
    //
    if (FdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        status = InitializeStorageRequestBlock((PSTORAGE_REQUEST_BLOCK)Srb,
                                                STORAGE_ADDRESS_TYPE_BTL8,
                                                CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE,
                                                1,
                                                SrbExDataTypeScsiCdb16);
        if (NT_SUCCESS(status)) {
            ((PSTORAGE_REQUEST_BLOCK)Srb)->SrbFunction = SRB_FUNCTION_EXECUTE_SCSI;
        } else {
            //
            // Should not occur.
            //
            NT_ASSERT(FALSE);
        }
    } else {
        RtlZeroMemory(Srb, sizeof(SCSI_REQUEST_BLOCK));
        Srb->Length = sizeof(SCSI_REQUEST_BLOCK);
        Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    }

    //prepare the Srb
    if (NT_SUCCESS(status))
    {

        SrbSetTimeOutValue(Srb, FdoExtension->TimeOutValue);
        SrbSetRequestTag(Srb, SP_UNTAGGED);
        SrbSetRequestAttribute(Srb, SRB_SIMPLE_TAG_REQUEST);
        SrbAssignSrbFlags(Srb, FdoExtension->SrbFlags);

        SrbSetCdbLength(Srb, 16);

        cdb = SrbGetCdb(Srb);
        cdb->READ_CAPACITY16.OperationCode = SCSIOP_READ_CAPACITY16;
        cdb->READ_CAPACITY16.ServiceAction = SERVICE_ACTION_READ_CAPACITY16;
        cdb->READ_CAPACITY16.AllocationLength[3] = bufferLength;

        status = ClassSendSrbSynchronous(FdoExtension->DeviceObject,
                                         Srb,
                                         dataBuffer,
                                         allocationBufferLength,
                                         FALSE);

        dataTransferLength = SrbGetDataTransferLength(Srb);
    }

    if (NT_SUCCESS(status) && (dataTransferLength < 16))
    {
        // the device should return at least 16 bytes of data for this command.
        status = STATUS_INFO_LENGTH_MISMATCH;
    }

    //
    // Handle the case where we get back STATUS_DATA_OVERRUN b/c the input
    // buffer was larger than necessary.
    //
    if (status == STATUS_DATA_OVERRUN && dataTransferLength < bufferLength)
    {
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status))
    {
        // cache data into FdoExtension
        status = InterpretReadCapacity16Data(FdoExtension, dataBuffer);
    }

    // cache the status indicates that this funciton has been called.
    FdoExtension->FunctionSupportInfo->ReadCapacity16Data.CommandStatus = status;

    ExFreePool(dataBuffer);

    return status;
}

NTSTATUS ClasspAccessAlignmentProperty(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    )
/*
    At first time of receiving the request, this function will forward it to lower stack to determine if it's supportted.
    If it's not supported, SCSIOP_READ_CAPACITY16 will be sent down to retrieve the information.
*/
{
    NTSTATUS    status = STATUS_UNSUCCESSFUL;

    PCOMMON_DEVICE_EXTENSION     commonExtension = (PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PSTORAGE_PROPERTY_QUERY      query = (PSTORAGE_PROPERTY_QUERY)Irp->AssociatedIrp.SystemBuffer;
    PIO_STACK_LOCATION           irpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG                        length = 0;
    ULONG                        information = 0;

    PSTORAGE_ACCESS_ALIGNMENT_DESCRIPTOR accessAlignment;

    //
    // check registry setting and fail the IOCTL if it's required.
    // this registry setting can be used to work around issues which upper layer doesn't support large physical sector size.
    //
    if (fdoExtension->FunctionSupportInfo->RegAccessAlignmentQueryNotSupported) {
        status = STATUS_NOT_SUPPORTED;
        goto Exit;
    }

    if ( (DeviceObject->DeviceType != FILE_DEVICE_DISK) ||
         (TEST_FLAG(DeviceObject->Characteristics, FILE_FLOPPY_DISKETTE)) ||
         (fdoExtension->FunctionSupportInfo->LowerLayerSupport.AccessAlignmentProperty == Supported) ) {
        // if it's not disk, forward the request to lower layer,
        // if the IOCTL is supported by lower stack, forward it down.
        IoCopyCurrentIrpStackLocationToNext(Irp);

        ClassReleaseRemoveLock(DeviceObject, Irp);
        status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
        return status;
    }

    //
    // Check proper query type.
    //

    if (query->QueryType == PropertyExistsQuery) {
        status = STATUS_SUCCESS;
        goto Exit;
    } else  if (query->QueryType != PropertyStandardQuery) {
        status = STATUS_NOT_SUPPORTED;
        goto Exit;
    }

    //
    // Request validation.
    // Note that InputBufferLength and IsFdo have been validated before entering this routine.
    //

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {
        NT_ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
        status = STATUS_INVALID_LEVEL;
        goto Exit;
    }

    // do not touch this buffer because it can still be used as input buffer for lower layer in 'SupportUnknown' case.
    accessAlignment = (PSTORAGE_ACCESS_ALIGNMENT_DESCRIPTOR)Irp->AssociatedIrp.SystemBuffer;

    length = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (length < sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR)) {

        if (length >= sizeof(STORAGE_DESCRIPTOR_HEADER)) {

            information = sizeof(STORAGE_DESCRIPTOR_HEADER);
            accessAlignment->Version = sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR);
            accessAlignment->Size = sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR);
            status = STATUS_SUCCESS;
            goto Exit;
        }

        status = STATUS_BUFFER_TOO_SMALL;
        goto Exit;
    }

    // not support Cache Line,
    // 'BytesPerCacheLine' and 'BytesOffsetForCacheAlignment' fields are zero-ed.

    //
    // note that 'Supported' case has been handled at the beginning of this function.
    //
    switch (fdoExtension->FunctionSupportInfo->LowerLayerSupport.AccessAlignmentProperty) {
    case SupportUnknown: {
        // send down request and wait for the request to complete.
        status = ClassForwardIrpSynchronous(commonExtension, Irp);

        if (ClasspLowerLayerNotSupport(status)) {
            // case 1: the request is not supported by lower layer, sends down command
            // some port drivers (or filter drivers) return STATUS_INVALID_DEVICE_REQUEST if a request is not supported.

            // ClassReadCapacity16() will either return status from cached data or send command to retrieve the information.
            if (ClasspIsObsoletePortDriver(fdoExtension) == FALSE) {
                status = ClassReadCapacity16(fdoExtension, Srb);
            } else {
                fdoExtension->FunctionSupportInfo->ReadCapacity16Data.CommandStatus = status;
            }

            // data is ready in fdoExtension
            // set the support status after the SCSI command is executed to avoid racing condition between multiple same type of requests.
            fdoExtension->FunctionSupportInfo->LowerLayerSupport.AccessAlignmentProperty = NotSupported;

            if (NT_SUCCESS(status)) {
                // fill output buffer
                RtlZeroMemory(accessAlignment, length);
                accessAlignment->Version = sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR);
                accessAlignment->Size = sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR);
                accessAlignment->BytesPerLogicalSector = fdoExtension->FunctionSupportInfo->ReadCapacity16Data.BytesPerLogicalSector;
                accessAlignment->BytesPerPhysicalSector = fdoExtension->FunctionSupportInfo->ReadCapacity16Data.BytesPerPhysicalSector;
                accessAlignment->BytesOffsetForSectorAlignment = fdoExtension->FunctionSupportInfo->ReadCapacity16Data.BytesOffsetForSectorAlignment;

                // set returned data length
                information = sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR);
            } else {
                information = 0;
            }

        } else {
            // case 2: the request is supported and it completes successfully
            // case 3: the request is supported by lower stack but other failure status is returned.
            // from now on, the same request will be send down to lower stack directly.
            fdoExtension->FunctionSupportInfo->LowerLayerSupport.AccessAlignmentProperty = Supported;
            information = (ULONG)Irp->IoStatus.Information;


        }


        goto Exit;

        break;
    }

    case NotSupported: {

        // ClassReadCapacity16() will either return status from cached data or send command to retrieve the information.
        status = ClassReadCapacity16(fdoExtension, Srb);

        if (NT_SUCCESS(status)) {
            RtlZeroMemory(accessAlignment, length);
            accessAlignment->Version = sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR);
            accessAlignment->Size = sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR);
            accessAlignment->BytesPerLogicalSector = fdoExtension->FunctionSupportInfo->ReadCapacity16Data.BytesPerLogicalSector;
            accessAlignment->BytesPerPhysicalSector = fdoExtension->FunctionSupportInfo->ReadCapacity16Data.BytesPerPhysicalSector;
            accessAlignment->BytesOffsetForSectorAlignment = fdoExtension->FunctionSupportInfo->ReadCapacity16Data.BytesOffsetForSectorAlignment;

            information = sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR);
        } else {
            information = 0;
        }
        goto Exit;

        break;
    }

    case Supported: {
        NT_ASSERT(FALSE); // this case is handled at the beginning of the function.
        status = STATUS_INTERNAL_ERROR;
        break;
    }

    }   // end of switch (fdoExtension->FunctionSupportInfo->LowerLayerSupport.AccessAlignmentProperty)

Exit:

    //
    // Set the size and status in IRP
    //
    Irp->IoStatus.Information = information;
    Irp->IoStatus.Status = status;

    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

    return status;
}

static
NTSTATUS
IncursSeekPenalty (
    _In_ USHORT     MediumRotationRate,
    _In_ PBOOLEAN   IncursSeekPenalty
    )
{
    NTSTATUS status;

    if (MediumRotationRate == 0x0001) {
        // Non-rotating media (e.g., solid state device)
        *IncursSeekPenalty = FALSE;
        status = STATUS_SUCCESS;
    } else if ( (MediumRotationRate >= 0x401) &&
                (MediumRotationRate <= 0xFFFE) ) {
        // Nominal media rotation rate in rotations per minute (rpm)
        *IncursSeekPenalty = TRUE;
        status = STATUS_SUCCESS;
    } else {
        // Unknown cases:
        //    0 - Rate not reported
        //    0002h-0400h - Reserved
        //    FFFFh - Reserved
        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}


NTSTATUS
ClasspDeviceMediaTypeProperty(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    )
/*++

Routine Description:

    This routine returns the medium product type reported by the device for the associated LU.

    This function must be called at IRQL < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Supplies the device object associated with this request
    Irp - The IRP to be processed
    Srb - The SRB associated with the request

Return Value:

    NTSTATUS code

--*/
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PSTORAGE_PROPERTY_QUERY query = (PSTORAGE_PROPERTY_QUERY)Irp->AssociatedIrp.SystemBuffer;
    PSTORAGE_MEDIUM_PRODUCT_TYPE_DESCRIPTOR pDesc = (PSTORAGE_MEDIUM_PRODUCT_TYPE_DESCRIPTOR)Irp->AssociatedIrp.SystemBuffer;
    PIO_STACK_LOCATION irpStack;
    ULONG length = 0;
    ULONG information = 0;

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspDeviceMediaTypeProperty (%p): Entering function.\n",
                DeviceObject));

    //
    // Check proper query type.
    //
    if (query->QueryType == PropertyExistsQuery) {

        //
        // In order to maintain consistency with the how the rest of the properties
        // are handled, always return success for PropertyExistsQuery.
        //
        status = STATUS_SUCCESS;
        goto __ClasspDeviceMediaTypeProperty_Exit;

    } else if (query->QueryType != PropertyStandardQuery) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspDeviceMediaTypeProperty (%p): Unsupported query type %x for media type property.\n",
                    DeviceObject,
                    query->QueryType));

        status = STATUS_NOT_SUPPORTED;
        goto __ClasspDeviceMediaTypeProperty_Exit;
    }

    //
    // Validate the request.
    // InputBufferLength and IsFdo have already been validated.
    //

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {

        NT_ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspDeviceMediaTypeProperty (%p): Query property for media type at incorrect IRQL.\n",
                    DeviceObject));

        status = STATUS_INVALID_LEVEL;
        goto __ClasspDeviceMediaTypeProperty_Exit;
    }

    length = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (length >= sizeof(STORAGE_DESCRIPTOR_HEADER)) {

        information = sizeof(STORAGE_MEDIUM_PRODUCT_TYPE_DESCRIPTOR);
        pDesc->Version = sizeof(STORAGE_MEDIUM_PRODUCT_TYPE_DESCRIPTOR);
        pDesc->Size = sizeof(STORAGE_MEDIUM_PRODUCT_TYPE_DESCRIPTOR);
    } else {

        status = STATUS_BUFFER_TOO_SMALL;
        goto __ClasspDeviceMediaTypeProperty_Exit;
    }

    if (length < sizeof(STORAGE_MEDIUM_PRODUCT_TYPE_DESCRIPTOR)) {

        status = STATUS_SUCCESS;
        goto __ClasspDeviceMediaTypeProperty_Exit;
    }

    //
    // Only query BlockDeviceCharacteristics VPD page if device support has been confirmed.
    //
    if (fdoExtension->FunctionSupportInfo->ValidInquiryPages.BlockDeviceCharacteristics == TRUE) {
        status = ClasspDeviceGetBlockDeviceCharacteristicsVPDPage(fdoExtension, Srb);
    } else {
        //
        // Otherwise device was previously found lacking support for this VPD page. Fail the request.
        //
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto __ClasspDeviceMediaTypeProperty_Exit;
    }

    if (!NT_SUCCESS(status)) {

        status = fdoExtension->FunctionSupportInfo->DeviceCharacteristicsData.CommandStatus;
        information = 0;

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspDeviceGetBlockDeviceCharacteristicsVPDPage (%p): VPD retrieval fails with %x.\n",
                    DeviceObject,
                    status));

        goto __ClasspDeviceMediaTypeProperty_Exit;
    }

    //
    // Fill in the output buffer.  All data is copied from the FDO extension, cached
    //  from device response to earlier VPD_BLOCK_DEVICE_CHARACTERISTICS query.
    //
    pDesc->MediumProductType = fdoExtension->FunctionSupportInfo->DeviceCharacteristicsData.MediumProductType;
    status = STATUS_SUCCESS;

__ClasspDeviceMediaTypeProperty_Exit:

    //
    // Set the size and status in IRP
    //
    Irp->IoStatus.Information = information;
    Irp->IoStatus.Status = status;

    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspDeviceMediaTypeProperty (%p): Exiting function with status %x.\n",
                DeviceObject,
                status));

    return status;
}

NTSTATUS ClasspDeviceGetBlockDeviceCharacteristicsVPDPage(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION fdoExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb
    )
/*
Routine Description:

    This function sends an INQUIRY command request for VPD_BLOCK_DEVICE_CHARACTERISTICS to
    the device. Relevant data from the response is cached in the FDO extension.

Arguments:
    FdoExtension: The FDO extension of the device to which the INQUIRY command will be sent.
    Srb: Allocated by the caller.
    SrbSize: The size of the Srb buffer in bytes.

Return Value:

    STATUS_INVALID_PARAMETER: May be returned if the LogPage buffer is NULL or
        not large enough.
    STATUS_SUCCESS: The log page was obtained and placed in the LogPage buffer.

    This function may return other NTSTATUS codes from internal function calls.
--*/
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PCDB cdb;
    UCHAR bufferLength = sizeof(VPD_BLOCK_DEVICE_CHARACTERISTICS_PAGE);  // data is 64 bytes
    ULONG allocationBufferLength = bufferLength;
    PVPD_BLOCK_DEVICE_CHARACTERISTICS_PAGE dataBuffer = NULL;


#if defined(_ARM_) || defined(_ARM64_)
    //
    // ARM has specific alignment requirements, although this will not have a functional impact on x86 or amd64
    // based platforms. We are taking the conservative approach here.
    //
    allocationBufferLength = ALIGN_UP_BY(allocationBufferLength,KeGetRecommendedSharedDataAlignment());
    dataBuffer = (PVPD_BLOCK_DEVICE_CHARACTERISTICS_PAGE)ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned,
                                                                                allocationBufferLength,
                                                                                '5CcS'
                                                                                );
#else

    dataBuffer = (PVPD_BLOCK_DEVICE_CHARACTERISTICS_PAGE)ExAllocatePoolWithTag(NonPagedPoolNx,
                                                                                bufferLength,
                                                                                '5CcS'
                                                                                );
#endif
    if (dataBuffer == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    RtlZeroMemory(dataBuffer, allocationBufferLength);

    // prepare the Srb
    SrbSetTimeOutValue(Srb, fdoExtension->TimeOutValue);
    SrbSetRequestTag(Srb, SP_UNTAGGED);
    SrbSetRequestAttribute(Srb, SRB_SIMPLE_TAG_REQUEST);
    SrbAssignSrbFlags(Srb, fdoExtension->SrbFlags);

    SrbSetCdbLength(Srb, 6);

    cdb = SrbGetCdb(Srb);
    cdb->CDB6INQUIRY3.OperationCode = SCSIOP_INQUIRY;
    cdb->CDB6INQUIRY3.EnableVitalProductData = 1;       //EVPD bit
    cdb->CDB6INQUIRY3.PageCode = VPD_BLOCK_DEVICE_CHARACTERISTICS;
    cdb->CDB6INQUIRY3.AllocationLength = bufferLength;  //AllocationLength field in CDB6INQUIRY3 is only one byte.

    status = ClassSendSrbSynchronous(fdoExtension->CommonExtension.DeviceObject,
                                        Srb,
                                        dataBuffer,
                                        allocationBufferLength,
                                        FALSE);
    if (NT_SUCCESS(status)) {
        if (SrbGetDataTransferLength(Srb) < 0x8) {
            // the device should return at least 8 bytes of data for use.
            status = STATUS_UNSUCCESSFUL;
        } else if ( (dataBuffer->PageLength != 0x3C) || (dataBuffer->PageCode != VPD_BLOCK_DEVICE_CHARACTERISTICS) ) {
            // 'PageLength' shall be 0x3C; and 'PageCode' shall match.
            status = STATUS_UNSUCCESSFUL;
        } else {
            // cache data into fdoExtension
            fdoExtension->FunctionSupportInfo->DeviceCharacteristicsData.MediumRotationRate = (dataBuffer->MediumRotationRateMsb << 8) |
                                                                                                dataBuffer->MediumRotationRateLsb;
            fdoExtension->FunctionSupportInfo->DeviceCharacteristicsData.MediumProductType = dataBuffer->MediumProductType;
            fdoExtension->FunctionSupportInfo->DeviceCharacteristicsData.NominalFormFactor = dataBuffer->NominalFormFactor;
        }
    } else {
        // the command failed, surface up the command error from 'status' variable. Nothing to do here.
    }

Exit:
    if (dataBuffer != NULL) {
        ExFreePool(dataBuffer);
    }

    return status;
}

NTSTATUS ClasspDeviceSeekPenaltyProperty(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    )
/*
    At first time of receiving the request, this function will forward it to lower stack to determine if it's supportted.
    If it's not supported, INQUIRY (Block Device Characteristics VPD page) will be sent down to retrieve the information.
*/
{
    NTSTATUS    status = STATUS_UNSUCCESSFUL;

    PCOMMON_DEVICE_EXTENSION     commonExtension = (PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PSTORAGE_PROPERTY_QUERY      query = (PSTORAGE_PROPERTY_QUERY)Irp->AssociatedIrp.SystemBuffer;
    PIO_STACK_LOCATION           irpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG                        length = 0;
    ULONG                        information = 0;
    BOOLEAN                      incursSeekPenalty = TRUE;
    PDEVICE_SEEK_PENALTY_DESCRIPTOR seekPenalty;

    if ( (DeviceObject->DeviceType != FILE_DEVICE_DISK) ||
         (TEST_FLAG(DeviceObject->Characteristics, FILE_FLOPPY_DISKETTE)) ||
         (fdoExtension->FunctionSupportInfo->LowerLayerSupport.SeekPenaltyProperty == Supported) ) {
        // if it's not disk, forward the request to lower layer,
        // if the IOCTL is supported by lower stack, forward it down.
        IoCopyCurrentIrpStackLocationToNext(Irp);

        ClassReleaseRemoveLock(DeviceObject, Irp);
        status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
        return status;
    }

    //
    // Check proper query type.
    //

    if (query->QueryType == PropertyExistsQuery) {
        status = STATUS_SUCCESS;
        goto Exit;
    } else  if (query->QueryType != PropertyStandardQuery) {
        status = STATUS_NOT_SUPPORTED;
        goto Exit;
    }

    //
    // Request validation.
    // Note that InputBufferLength and IsFdo have been validated beforing entering this routine.
    //

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {
        NT_ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
        status = STATUS_INVALID_LEVEL;
        goto Exit;
    }

    // do not touch this buffer because it can still be used as input buffer for lower layer in 'SupportUnknown' case.
    seekPenalty = (PDEVICE_SEEK_PENALTY_DESCRIPTOR)Irp->AssociatedIrp.SystemBuffer;

    length = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (length < sizeof(DEVICE_SEEK_PENALTY_DESCRIPTOR)) {

        if (length >= sizeof(STORAGE_DESCRIPTOR_HEADER)) {

            information = sizeof(STORAGE_DESCRIPTOR_HEADER);
            seekPenalty->Version = sizeof(DEVICE_SEEK_PENALTY_DESCRIPTOR);
            seekPenalty->Size = sizeof(DEVICE_SEEK_PENALTY_DESCRIPTOR);
            status = STATUS_SUCCESS;
            goto Exit;
        }

        status = STATUS_BUFFER_TOO_SMALL;
        goto Exit;
    }

    //
    // note that 'Supported' case has been handled at the beginning of this function.
    //
    switch (fdoExtension->FunctionSupportInfo->LowerLayerSupport.SeekPenaltyProperty) {
    case SupportUnknown: {
        // send down request and wait for the request to complete.
        status = ClassForwardIrpSynchronous(commonExtension, Irp);

        if (ClasspLowerLayerNotSupport(status)) {
            // case 1: the request is not supported by lower layer, sends down command
            // some port drivers (or filter drivers) return STATUS_INVALID_DEVICE_REQUEST if a request is not supported.

            // send INQUIRY command if the VPD page is supported.
            if (fdoExtension->FunctionSupportInfo->ValidInquiryPages.BlockDeviceCharacteristics == TRUE) {
                status = ClasspDeviceGetBlockDeviceCharacteristicsVPDPage(fdoExtension, Srb);
            } else {
                // the INQUIRY - VPD page command to discover the info is not supported, fail the request.
                status = STATUS_INVALID_DEVICE_REQUEST;
            }

            if (NT_SUCCESS(status)) {
                status = IncursSeekPenalty(fdoExtension->FunctionSupportInfo->DeviceCharacteristicsData.MediumRotationRate, &incursSeekPenalty);
            }

            fdoExtension->FunctionSupportInfo->DeviceCharacteristicsData.CommandStatus = status;

            // data is ready in fdoExtension
            // set the support status after the SCSI command is executed to avoid racing condition between multiple same type of requests.
            fdoExtension->FunctionSupportInfo->LowerLayerSupport.SeekPenaltyProperty = NotSupported;

            // fill output buffer
            if (NT_SUCCESS(status)) {
                RtlZeroMemory(seekPenalty, length);
                seekPenalty->Version = sizeof(DEVICE_SEEK_PENALTY_DESCRIPTOR);
                seekPenalty->Size = sizeof(DEVICE_SEEK_PENALTY_DESCRIPTOR);
                seekPenalty->IncursSeekPenalty = incursSeekPenalty;
                information = sizeof(DEVICE_SEEK_PENALTY_DESCRIPTOR);


            } else {
                information = 0;
            }

        } else {
            // case 2: the request is supported and it completes successfully
            // case 3: the request is supported by lower stack but other failure status is returned.
            // from now on, the same request will be send down to lower stack directly.
            fdoExtension->FunctionSupportInfo->LowerLayerSupport.SeekPenaltyProperty = Supported;
            information = (ULONG)Irp->IoStatus.Information;

        }


        goto Exit;

        break;
    }

    case NotSupported: {
        status = fdoExtension->FunctionSupportInfo->DeviceCharacteristicsData.CommandStatus;

        if (NT_SUCCESS(status)) {
            status = IncursSeekPenalty(fdoExtension->FunctionSupportInfo->DeviceCharacteristicsData.MediumRotationRate, &incursSeekPenalty);
        }

        if (NT_SUCCESS(status)) {
            RtlZeroMemory(seekPenalty, length);
            seekPenalty->Version = sizeof(DEVICE_SEEK_PENALTY_DESCRIPTOR);
            seekPenalty->Size = sizeof(DEVICE_SEEK_PENALTY_DESCRIPTOR);
            seekPenalty->IncursSeekPenalty = incursSeekPenalty;
            information = sizeof(DEVICE_SEEK_PENALTY_DESCRIPTOR);

        } else {
            information = 0;
        }

        goto Exit;

        break;
    }

    case Supported: {
        NT_ASSERT(FALSE); // this case is handled at the begining of the function.
        break;
    }

    }   // end of switch (fdoExtension->FunctionSupportInfo->LowerLayerSupport.SeekPenaltyProperty)

Exit:

    //
    // Set the size and status in IRP
    //
    Irp->IoStatus.Information = information;;
    Irp->IoStatus.Status = status;

    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS ClasspDeviceGetLBProvisioningVPDPage(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_opt_ PSCSI_REQUEST_BLOCK Srb
    )
{
    NTSTATUS                     status = STATUS_UNSUCCESSFUL;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    USHORT                       pageLength = 0;

    PVOID                        dataBuffer = NULL;
    UCHAR                        bufferLength = VPD_MAX_BUFFER_SIZE;  // use biggest buffer possible
    ULONG                        allocationBufferLength = bufferLength; // Since the CDB size may differ from the actual buffer allocation
    PCDB                         cdb;
    ULONG                        dataTransferLength = 0;
    PVPD_LOGICAL_BLOCK_PROVISIONING_PAGE  lbProvisioning = NULL;

    //
    // if the informaiton has been attempted to retrieve, return the cached status.
    //
    if (fdoExtension->FunctionSupportInfo->LBProvisioningData.CommandStatus != -1) {
        // get cached NTSTATUS from previous call.
        return fdoExtension->FunctionSupportInfo->LBProvisioningData.CommandStatus;
    }

    //
    // Initialize LBProvisioningData fields to 'unsupported' defaults.
    //
    fdoExtension->FunctionSupportInfo->LBProvisioningData.ProvisioningType = PROVISIONING_TYPE_UNKNOWN;
    fdoExtension->FunctionSupportInfo->LBProvisioningData.LBPRZ = FALSE;
    fdoExtension->FunctionSupportInfo->LBProvisioningData.LBPU = FALSE;
    fdoExtension->FunctionSupportInfo->LBProvisioningData.ANC_SUP = FALSE;
    fdoExtension->FunctionSupportInfo->LBProvisioningData.ThresholdExponent = 0;

    //
    // Try to get the Thin Provisioning VPD page (0xB2), if it is supported.
    //
    if (fdoExtension->FunctionSupportInfo->ValidInquiryPages.LBProvisioning == TRUE &&
        Srb != NULL)
    {
#if defined(_ARM_) || defined(_ARM64_)
        //
        // ARM has specific alignment requirements, although this will not have a functional impact on x86 or amd64
        // based platforms. We are taking the conservative approach here.
        //
        //
        allocationBufferLength = ALIGN_UP_BY(allocationBufferLength,KeGetRecommendedSharedDataAlignment());
        dataBuffer = ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned, allocationBufferLength,'0CcS');
#else
        dataBuffer = ExAllocatePoolWithTag(NonPagedPoolNx, bufferLength,'0CcS');
#endif
        if (dataBuffer == NULL) {
            // return without updating FdoExtension->FunctionSupportInfo->LBProvisioningData.CommandStatus
            // the field will remain value as "-1", so that the command will be attempted next time this function is called.
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        lbProvisioning = (PVPD_LOGICAL_BLOCK_PROVISIONING_PAGE)dataBuffer;

        RtlZeroMemory(dataBuffer, allocationBufferLength);

        if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
            status = InitializeStorageRequestBlock((PSTORAGE_REQUEST_BLOCK)Srb,
                                                   STORAGE_ADDRESS_TYPE_BTL8,
                                                   CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE,
                                                   1,
                                                   SrbExDataTypeScsiCdb16);
            if (NT_SUCCESS(status)) {
                ((PSTORAGE_REQUEST_BLOCK)Srb)->SrbFunction = SRB_FUNCTION_EXECUTE_SCSI;
            } else {
                //
                // Should not occur.
                //
                NT_ASSERT(FALSE);
            }
        } else {
            RtlZeroMemory(Srb, sizeof(SCSI_REQUEST_BLOCK));
            Srb->Length = sizeof(SCSI_REQUEST_BLOCK);
            Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
            status = STATUS_SUCCESS;
        }

        if (NT_SUCCESS(status)) {
            // prepare the Srb
            SrbSetTimeOutValue(Srb, fdoExtension->TimeOutValue);
            SrbSetRequestTag(Srb, SP_UNTAGGED);
            SrbSetRequestAttribute(Srb, SRB_SIMPLE_TAG_REQUEST);
            SrbAssignSrbFlags(Srb, fdoExtension->SrbFlags);

            SrbSetCdbLength(Srb, 6);

            cdb = SrbGetCdb(Srb);
            cdb->CDB6INQUIRY3.OperationCode = SCSIOP_INQUIRY;
            cdb->CDB6INQUIRY3.EnableVitalProductData = 1;       //EVPD bit
            cdb->CDB6INQUIRY3.PageCode = VPD_LOGICAL_BLOCK_PROVISIONING;
            cdb->CDB6INQUIRY3.AllocationLength = bufferLength;  //AllocationLength field in CDB6INQUIRY3 is only one byte.

            status = ClassSendSrbSynchronous(fdoExtension->DeviceObject,
                                             Srb,
                                             dataBuffer,
                                             allocationBufferLength,
                                             FALSE);

            dataTransferLength = SrbGetDataTransferLength(Srb);
        }

        //
        // Handle the case where we get back STATUS_DATA_OVERRUN b/c the input
        // buffer was larger than necessary.
        //
        if (status == STATUS_DATA_OVERRUN && dataTransferLength < bufferLength)
        {
            status = STATUS_SUCCESS;
        }

        if (NT_SUCCESS(status)) {
            REVERSE_BYTES_SHORT(&pageLength, &(lbProvisioning->PageLength));
        }

        if ( NT_SUCCESS(status) &&
                ((dataTransferLength < 0x08) ||
                (pageLength < (FIELD_OFFSET(VPD_LOGICAL_BLOCK_PROVISIONING_PAGE, Reserved2) - FIELD_OFFSET(VPD_LOGICAL_BLOCK_PROVISIONING_PAGE,ThresholdExponent))) ||
                (lbProvisioning->PageCode != VPD_LOGICAL_BLOCK_PROVISIONING)) ) {
            // the device should return at least 8 bytes of data for use.
            // 'PageCode' shall match and we need all the relevant data after the header.
            status = STATUS_INFO_LENGTH_MISMATCH;
        }

        //
        // Fill in the FDO extension with either the data from the VPD page, or
        // use defaults if there was an error.
        //
        if (NT_SUCCESS(status))
        {
            fdoExtension->FunctionSupportInfo->LBProvisioningData.ProvisioningType = lbProvisioning->ProvisioningType;
            fdoExtension->FunctionSupportInfo->LBProvisioningData.LBPRZ = lbProvisioning->LBPRZ;
            fdoExtension->FunctionSupportInfo->LBProvisioningData.LBPU = lbProvisioning->LBPU;
            fdoExtension->FunctionSupportInfo->LBProvisioningData.ANC_SUP = lbProvisioning->ANC_SUP;
            fdoExtension->FunctionSupportInfo->LBProvisioningData.ThresholdExponent = lbProvisioning->ThresholdExponent;

            TracePrint((TRACE_LEVEL_INFORMATION,
                        TRACE_FLAG_PNP,
                        "ClasspDeviceGetLBProvisioningVPDPage (%p): %s %s (rev %s) reported following parameters: \
                        \n\t\t\tProvisioningType: %u \
                        \n\t\t\tLBPRZ: %u \
                        \n\t\t\tLBPU: %u \
                        \n\t\t\tANC_SUP: %I64u \
                        \n\t\t\tThresholdExponent: %u\n",
                        DeviceObject,
                        (PCSZ)(((PUCHAR)fdoExtension->DeviceDescriptor) + fdoExtension->DeviceDescriptor->VendorIdOffset),
                        (PCSZ)(((PUCHAR)fdoExtension->DeviceDescriptor) + fdoExtension->DeviceDescriptor->ProductIdOffset),
                        (PCSZ)(((PUCHAR)fdoExtension->DeviceDescriptor) + fdoExtension->DeviceDescriptor->ProductRevisionOffset),
                        lbProvisioning->ProvisioningType,
                        lbProvisioning->LBPRZ,
                        lbProvisioning->LBPU,
                        lbProvisioning->ANC_SUP,
                        lbProvisioning->ThresholdExponent));
        }
    } else {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    fdoExtension->FunctionSupportInfo->LBProvisioningData.CommandStatus = status;

Exit:
    FREE_POOL(dataBuffer);

    return status;
}


NTSTATUS ClasspDeviceGetBlockLimitsVPDPage(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    _Inout_bytecount_(SrbSize) PSCSI_REQUEST_BLOCK Srb,
    _In_ ULONG SrbSize,
    _Out_ PCLASS_VPD_B0_DATA BlockLimitsData
    )
{
    NTSTATUS                     status = STATUS_UNSUCCESSFUL;
    PVOID                        dataBuffer = NULL;
    UCHAR                        bufferLength = VPD_MAX_BUFFER_SIZE;  // use biggest buffer possible
    ULONG                        allocationBufferLength = bufferLength;
    PCDB                         cdb;
    PVPD_BLOCK_LIMITS_PAGE       blockLimits = NULL;
    ULONG                        dataTransferLength = 0;

    //
    // Set default values for UNMAP parameters based upon UNMAP support or lack
    // thereof.
    //
    if (FdoExtension->FunctionSupportInfo->LBProvisioningData.LBPU) {
        //
        // If UNMAP is supported, we default to the maximum LBA count and
        // block descriptor count.  We also default the UNMAP granularity to
        // a single block and specify no granularity alignment.
        //
        BlockLimitsData->MaxUnmapLbaCount = (ULONG)-1;
        BlockLimitsData->MaxUnmapBlockDescrCount = (ULONG)-1;
        BlockLimitsData->OptimalUnmapGranularity = 1;
        BlockLimitsData->UnmapGranularityAlignment = 0;
        BlockLimitsData->UGAVALID = FALSE;
    } else {
        BlockLimitsData->MaxUnmapLbaCount = 0;
        BlockLimitsData->MaxUnmapBlockDescrCount = 0;
        BlockLimitsData->OptimalUnmapGranularity = 0;
        BlockLimitsData->UnmapGranularityAlignment = 0;
        BlockLimitsData->UGAVALID = FALSE;
    }

    //
    // Try to get the Block Limits VPD page (0xB0), if it is supported.
    //
    if (FdoExtension->FunctionSupportInfo->ValidInquiryPages.BlockLimits == TRUE)
    {
#if defined(_ARM_) || defined(_ARM64_)
        //
        // ARM has specific alignment requirements, although this will not have a functional impact on x86 or amd64
        // based platforms. We are taking the conservative approach here.
        //
        allocationBufferLength = ALIGN_UP_BY(allocationBufferLength, KeGetRecommendedSharedDataAlignment());
        dataBuffer = ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned, allocationBufferLength, '0CcS');
#else
        dataBuffer = ExAllocatePoolWithTag(NonPagedPoolNx, bufferLength, '0CcS');
#endif
        if (dataBuffer == NULL)
        {
            // return without updating FdoExtension->FunctionSupportInfo->BlockLimitsData.CommandStatus
            // the field will remain value as "-1", so that the command will be attempted next time this function is called.
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        blockLimits = (PVPD_BLOCK_LIMITS_PAGE)dataBuffer;

        RtlZeroMemory(dataBuffer, allocationBufferLength);

        if (FdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {

#ifdef _MSC_VER
            #pragma prefast(suppress:26015, "InitializeStorageRequestBlock ensures buffer access is bounded")
#endif
            status = InitializeStorageRequestBlock((PSTORAGE_REQUEST_BLOCK)Srb,
                                                   STORAGE_ADDRESS_TYPE_BTL8,
                                                   SrbSize,
                                                   1,
                                                   SrbExDataTypeScsiCdb16);

            if (NT_SUCCESS(status)) {
                ((PSTORAGE_REQUEST_BLOCK)Srb)->SrbFunction = SRB_FUNCTION_EXECUTE_SCSI;
            } else {
                //
                // Should not occur.
                //
                NT_ASSERT(FALSE);
            }
        } else {
            RtlZeroMemory(Srb, sizeof(SCSI_REQUEST_BLOCK));
            Srb->Length = sizeof(SCSI_REQUEST_BLOCK);
            Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
            status = STATUS_SUCCESS;
        }

        if (NT_SUCCESS(status)) {
            // prepare the Srb
            SrbSetTimeOutValue(Srb, FdoExtension->TimeOutValue);
            SrbSetRequestTag(Srb, SP_UNTAGGED);
            SrbSetRequestAttribute(Srb, SRB_SIMPLE_TAG_REQUEST);
            SrbAssignSrbFlags(Srb, FdoExtension->SrbFlags);

            SrbSetCdbLength(Srb, 6);

            cdb = SrbGetCdb(Srb);
            cdb->CDB6INQUIRY3.OperationCode = SCSIOP_INQUIRY;
            cdb->CDB6INQUIRY3.EnableVitalProductData = 1;       //EVPD bit
            cdb->CDB6INQUIRY3.PageCode = VPD_BLOCK_LIMITS;
            cdb->CDB6INQUIRY3.AllocationLength = bufferLength;  //AllocationLength field in CDB6INQUIRY3 is only one byte.

            status = ClassSendSrbSynchronous(FdoExtension->DeviceObject,
                                             Srb,
                                             dataBuffer,
                                             allocationBufferLength,
                                             FALSE);
            dataTransferLength = SrbGetDataTransferLength(Srb);
        }

        //
        // Handle the case where we get back STATUS_DATA_OVERRUN b/c the input
        // buffer was larger than necessary.
        //

        if (status == STATUS_DATA_OVERRUN && dataTransferLength < bufferLength)
        {
            status = STATUS_SUCCESS;
        }

        if (NT_SUCCESS(status))
        {
            USHORT pageLength;
            REVERSE_BYTES_SHORT(&pageLength, &(blockLimits->PageLength));

            //
            // Regardless of the device's support for unmap, cache away at least the basic block limits information
            //
            if (dataTransferLength >= 0x10 && blockLimits->PageCode == VPD_BLOCK_LIMITS) {

                // (6:7) OPTIMAL TRANSFER LENGTH GRANULARITY
                REVERSE_BYTES_SHORT(&BlockLimitsData->OptimalTransferLengthGranularity, &blockLimits->OptimalTransferLengthGranularity);
                // (8:11) MAXIMUM TRANSFER LENGTH
                REVERSE_BYTES(&BlockLimitsData->MaximumTransferLength, &blockLimits->MaximumTransferLength);
                // (12:15) OPTIMAL TRANSFER LENGTH
                REVERSE_BYTES(&BlockLimitsData->OptimalTransferLength, &blockLimits->OptimalTransferLength);
            }

            if ((dataTransferLength < 0x24) ||
                (pageLength < (FIELD_OFFSET(VPD_BLOCK_LIMITS_PAGE,Reserved1) - FIELD_OFFSET(VPD_BLOCK_LIMITS_PAGE,Reserved0))) ||
                (blockLimits->PageCode != VPD_BLOCK_LIMITS))
            {
                // the device should return at least 36 bytes of data for use.
                // 'PageCode' shall match and we need all the relevant data after the header.
                status = STATUS_INFO_LENGTH_MISMATCH;
            }
        }

        if (NT_SUCCESS(status))
        {
            // cache data into FdoExtension
            // (20:23) MAXIMUM UNMAP LBA COUNT
            REVERSE_BYTES(&BlockLimitsData->MaxUnmapLbaCount, &blockLimits->MaximumUnmapLBACount);
            // (24:27) MAXIMUM UNMAP BLOCK DESCRIPTOR COUNT
            REVERSE_BYTES(&BlockLimitsData->MaxUnmapBlockDescrCount, &blockLimits->MaximumUnmapBlockDescriptorCount);
            // (28:31) OPTIMAL UNMAP GRANULARITY
            REVERSE_BYTES(&BlockLimitsData->OptimalUnmapGranularity, &blockLimits->OptimalUnmapGranularity);

            // (32:35) UNMAP GRANULARITY ALIGNMENT; (32) bit7: UGAVALID
            BlockLimitsData->UGAVALID = blockLimits->UGAValid;
            if (BlockLimitsData->UGAVALID == TRUE) {
                REVERSE_BYTES(&BlockLimitsData->UnmapGranularityAlignment, &blockLimits->UnmapGranularityAlignment);
                BlockLimitsData->UnmapGranularityAlignment &= 0x7FFFFFFF; // remove value of UGAVALID bit.
            } else {
                BlockLimitsData->UnmapGranularityAlignment = 0;
            }

            TracePrint((TRACE_LEVEL_INFORMATION,
                        TRACE_FLAG_PNP,
                        "ClasspDeviceGetBlockLimitsVPDPage (%p): %s %s (rev %s) reported following parameters: \
                        \n\t\t\tOptimalTransferLengthGranularity: %u \
                        \n\t\t\tMaximumTransferLength: %u \
                        \n\t\t\tOptimalTransferLength: %u \
                        \n\t\t\tMaximumUnmapLBACount: %u \
                        \n\t\t\tMaximumUnmapBlockDescriptorCount: %u \
                        \n\t\t\tOptimalUnmapGranularity: %u \
                        \n\t\t\tUGAValid: %u \
                        \n\t\t\tUnmapGranularityAlignment: %u\n",
                        FdoExtension->DeviceObject,
                        (PCSZ)(((PUCHAR)FdoExtension->DeviceDescriptor) + FdoExtension->DeviceDescriptor->VendorIdOffset),
                        (PCSZ)(((PUCHAR)FdoExtension->DeviceDescriptor) + FdoExtension->DeviceDescriptor->ProductIdOffset),
                        (PCSZ)(((PUCHAR)FdoExtension->DeviceDescriptor) + FdoExtension->DeviceDescriptor->ProductRevisionOffset),
                        BlockLimitsData->OptimalTransferLengthGranularity,
                        BlockLimitsData->MaximumTransferLength,
                        BlockLimitsData->OptimalTransferLength,
                        BlockLimitsData->MaxUnmapLbaCount,
                        BlockLimitsData->MaxUnmapBlockDescrCount,
                        BlockLimitsData->OptimalUnmapGranularity,
                        BlockLimitsData->UGAVALID,
                        BlockLimitsData->UnmapGranularityAlignment));

        }
    } else {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    BlockLimitsData->CommandStatus = status;

Exit:
    FREE_POOL(dataBuffer);

    return status;
}


NTSTATUS ClasspDeviceTrimProperty(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    )
/*
    At first time of receiving the request, this function will forward it to lower stack to determine if it's supportted.
    If it's not supported, INQUIRY (Block Limits VPD page) will be sent down to retrieve the information.
*/
{
    NTSTATUS    status = STATUS_SUCCESS;

    PCOMMON_DEVICE_EXTENSION     commonExtension = (PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PSTORAGE_PROPERTY_QUERY      query = (PSTORAGE_PROPERTY_QUERY)Irp->AssociatedIrp.SystemBuffer;
    PIO_STACK_LOCATION           irpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG                        length = 0;
    ULONG                        information = 0;

    PDEVICE_TRIM_DESCRIPTOR      trimDescr;

    UNREFERENCED_PARAMETER(Srb);

    if ( (DeviceObject->DeviceType != FILE_DEVICE_DISK) ||
         (TEST_FLAG(DeviceObject->Characteristics, FILE_FLOPPY_DISKETTE)) ||
         (fdoExtension->FunctionSupportInfo->LowerLayerSupport.TrimProperty == Supported) ) {
        // if it's not disk, forward the request to lower layer,
        // if the IOCTL is supported by lower stack, forward it down.
        IoCopyCurrentIrpStackLocationToNext(Irp);

        ClassReleaseRemoveLock(DeviceObject, Irp);
        status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
        return status;
    }

    //
    // Check proper query type.
    //

    if (query->QueryType == PropertyExistsQuery) {
        status = STATUS_SUCCESS;
        goto Exit;
    } else  if (query->QueryType != PropertyStandardQuery) {
        status = STATUS_NOT_SUPPORTED;
        goto Exit;
    }

    //
    // Request validation.
    // Note that InputBufferLength and IsFdo have been validated beforing entering this routine.
    //

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {
        NT_ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
        status = STATUS_INVALID_LEVEL;
        goto Exit;
    }

    // do not touch this buffer because it can still be used as input buffer for lower layer in 'SupportUnknown' case.
    trimDescr = (PDEVICE_TRIM_DESCRIPTOR)Irp->AssociatedIrp.SystemBuffer;

    length = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (length < sizeof(DEVICE_TRIM_DESCRIPTOR)) {

        if (length >= sizeof(STORAGE_DESCRIPTOR_HEADER)) {

            information = sizeof(STORAGE_DESCRIPTOR_HEADER);
            trimDescr->Version = sizeof(DEVICE_TRIM_DESCRIPTOR);
            trimDescr->Size = sizeof(DEVICE_TRIM_DESCRIPTOR);
            status = STATUS_SUCCESS;
            goto Exit;
        }

        status = STATUS_BUFFER_TOO_SMALL;
        goto Exit;
    }

    //
    // note that 'Supported' case has been handled at the beginning of this function.
    //
    switch (fdoExtension->FunctionSupportInfo->LowerLayerSupport.TrimProperty) {
    case SupportUnknown: {
        // send down request and wait for the request to complete.
        status = ClassForwardIrpSynchronous(commonExtension, Irp);

        if ( (status == STATUS_NOT_SUPPORTED) ||
             (status == STATUS_NOT_IMPLEMENTED) ||
             (status == STATUS_INVALID_DEVICE_REQUEST) ||
             (status == STATUS_INVALID_PARAMETER_1) ) {
            // case 1: the request is not supported by lower layer, sends down command
            // some port drivers (or filter drivers) return STATUS_INVALID_DEVICE_REQUEST if a request is not supported.
            status = fdoExtension->FunctionSupportInfo->LBProvisioningData.CommandStatus;
            NT_ASSERT(status != -1);

            // data is ready in fdoExtension
            // set the support status after the SCSI command is executed to avoid racing condition between multiple same type of requests.
            fdoExtension->FunctionSupportInfo->LowerLayerSupport.TrimProperty = NotSupported;

            if (NT_SUCCESS(status)) {
                // fill output buffer
                RtlZeroMemory(trimDescr, length);
                trimDescr->Version = sizeof(DEVICE_TRIM_DESCRIPTOR);
                trimDescr->Size = sizeof(DEVICE_TRIM_DESCRIPTOR);
                trimDescr->TrimEnabled = ClasspSupportsUnmap(fdoExtension->FunctionSupportInfo);

                // set returned data length
                information = sizeof(DEVICE_TRIM_DESCRIPTOR);
            } else {
                // there was error retrieving TrimProperty. Surface the error up from 'status' variable.
                information = 0;
            }
            goto Exit;
        } else {
            // case 2: the request is supported and it completes successfully
            // case 3: the request is supported by lower stack but other failure status is returned.
            // from now on, the same request will be send down to lower stack directly.
            fdoExtension->FunctionSupportInfo->LowerLayerSupport.TrimProperty = Supported;
            information = (ULONG)Irp->IoStatus.Information;
            goto Exit;
        }
        break;
    }

    case NotSupported: {
        status = fdoExtension->FunctionSupportInfo->LBProvisioningData.CommandStatus;
        NT_ASSERT(status != -1);

        if (NT_SUCCESS(status)) {
            RtlZeroMemory(trimDescr, length);
            trimDescr->Version = sizeof(DEVICE_TRIM_DESCRIPTOR);
            trimDescr->Size = sizeof(DEVICE_TRIM_DESCRIPTOR);
            trimDescr->TrimEnabled = ClasspSupportsUnmap(fdoExtension->FunctionSupportInfo);

            information = sizeof(DEVICE_TRIM_DESCRIPTOR);
        } else {
            information = 0;
        }
        goto Exit;

        break;
    }

    case Supported: {
        NT_ASSERT(FALSE); // this case is handled at the begining of the function.
        break;
    }

    }   // end of switch (fdoExtension->FunctionSupportInfo->LowerLayerSupport.TrimProperty)

Exit:

    //
    // Set the size and status in IRP
    //
    Irp->IoStatus.Information = information;
    Irp->IoStatus.Status = status;

    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS ClasspDeviceLBProvisioningProperty(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    )
{
    NTSTATUS                     status = STATUS_SUCCESS;
    NTSTATUS                     blockLimitsStatus;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PSTORAGE_PROPERTY_QUERY      query = (PSTORAGE_PROPERTY_QUERY)Irp->AssociatedIrp.SystemBuffer;
    PIO_STACK_LOCATION           irpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG                        length = 0;
    ULONG                        information = 0;
    CLASS_VPD_B0_DATA            blockLimitsData;
    ULONG                        generationCount;

    PDEVICE_LB_PROVISIONING_DESCRIPTOR  lbpDescr;

    UNREFERENCED_PARAMETER(Srb);

    //
    // Check proper query type.
    //
    if (query->QueryType == PropertyExistsQuery) {
        status = STATUS_SUCCESS;
        goto Exit;
    } else  if (query->QueryType != PropertyStandardQuery) {
        status = STATUS_NOT_SUPPORTED;
        goto Exit;
    }

    //
    // Request validation.
    // Note that InputBufferLength and IsFdo have been validated beforing entering this routine.
    //

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {
        NT_ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
        status = STATUS_INVALID_LEVEL;
        goto Exit;
    }

    lbpDescr = (PDEVICE_LB_PROVISIONING_DESCRIPTOR)Irp->AssociatedIrp.SystemBuffer;

    length = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    RtlZeroMemory(lbpDescr, length);

    if (length < DEVICE_LB_PROVISIONING_DESCRIPTOR_V1_SIZE) {

        if (length >= sizeof(STORAGE_DESCRIPTOR_HEADER)) {

            information = sizeof(STORAGE_DESCRIPTOR_HEADER);
            lbpDescr->Version = sizeof(DEVICE_LB_PROVISIONING_DESCRIPTOR);
            lbpDescr->Size = sizeof(DEVICE_LB_PROVISIONING_DESCRIPTOR);
            status = STATUS_SUCCESS;
            goto Exit;
        }

        status = STATUS_BUFFER_TOO_SMALL;
        goto Exit;
    }

    //
    // Set the structure version/size based upon the size of the given output
    // buffer.  We may be working with an older component that was built with
    // the V1 structure definition.
    //
    if (length < sizeof(DEVICE_LB_PROVISIONING_DESCRIPTOR)) {
        lbpDescr->Version = DEVICE_LB_PROVISIONING_DESCRIPTOR_V1_SIZE;
        lbpDescr->Size = DEVICE_LB_PROVISIONING_DESCRIPTOR_V1_SIZE;
        information = DEVICE_LB_PROVISIONING_DESCRIPTOR_V1_SIZE;
    } else {
        lbpDescr->Version = sizeof(DEVICE_LB_PROVISIONING_DESCRIPTOR);
        lbpDescr->Size = sizeof(DEVICE_LB_PROVISIONING_DESCRIPTOR);
        information = sizeof(DEVICE_LB_PROVISIONING_DESCRIPTOR);
    }

    //
    // Take a snapshot of the block limits data since it can change.
    // If we failed to get the block limits data, we'll just set the Optimal
    // Unmap Granularity (and alignment) will default to 0.  We don't want to
    // fail the request outright since there is some non-block limits data that
    // we can return.
    //
    blockLimitsStatus = ClasspBlockLimitsDataSnapshot(fdoExtension,
                                                      TRUE,
                                                      &blockLimitsData,
                                                      &generationCount);

    //
    // Fill in the output buffer.  All data is copied from the FDO extension where we
    // cached Logical Block Provisioning info when the device was first initialized.
    //

    lbpDescr->ThinProvisioningEnabled = ClasspIsThinProvisioned(fdoExtension->FunctionSupportInfo);

    //
    // Make sure we have a non-zero value for the number of bytes per block.
    //
    if (fdoExtension->DiskGeometry.BytesPerSector == 0)
    {
        status = ClassReadDriveCapacity(fdoExtension->DeviceObject);
        if(!NT_SUCCESS(status) || fdoExtension->DiskGeometry.BytesPerSector == 0)
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            information = 0;
            goto Exit;
        }
    }

    lbpDescr->ThinProvisioningReadZeros = fdoExtension->FunctionSupportInfo->LBProvisioningData.LBPRZ;
    lbpDescr->AnchorSupported = fdoExtension->FunctionSupportInfo->LBProvisioningData.ANC_SUP;

    if (NT_SUCCESS(blockLimitsStatus)) {
        lbpDescr->UnmapGranularityAlignmentValid = blockLimitsData.UGAVALID;

        //
        // Granularity and Alignment are given to us in units of blocks,
        // but we convert and return them in bytes as it is more convenient
        // to the caller.
        //
        lbpDescr->OptimalUnmapGranularity = (ULONGLONG)blockLimitsData.OptimalUnmapGranularity * fdoExtension->DiskGeometry.BytesPerSector;
        lbpDescr->UnmapGranularityAlignment = (ULONGLONG)blockLimitsData.UnmapGranularityAlignment * fdoExtension->DiskGeometry.BytesPerSector;

#if (NTDDI_VERSION >= NTDDI_WINBLUE)
        //
        // If the output buffer is large enough (i.e. not a V1 structure) copy
        // over the max UNMAP LBA count and max UNMAP block descriptor count.
        //
        if (length >= sizeof(DEVICE_LB_PROVISIONING_DESCRIPTOR)) {
            lbpDescr->MaxUnmapLbaCount = blockLimitsData.MaxUnmapLbaCount;
            lbpDescr->MaxUnmapBlockDescriptorCount = blockLimitsData.MaxUnmapBlockDescrCount;
        }
#endif
    }

Exit:

    //
    // Set the size and status in IRP
    //
    Irp->IoStatus.Information = information;
    Irp->IoStatus.Status = status;

    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

    return status;
}


VOID
ConvertDataSetRangeToUnmapBlockDescr(
    _In_    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    _In_    PUNMAP_BLOCK_DESCRIPTOR      BlockDescr,
    _Inout_ PULONG                       CurrentBlockDescrIndex,
    _In_    ULONG                        MaxBlockDescrIndex,
    _Inout_ PULONGLONG                   CurrentLbaCount,
    _In_    ULONGLONG                    MaxLbaCount,
    _Inout_ PDEVICE_DATA_SET_RANGE       DataSetRange
    )
/*++

Routine Description:

    Convert DEVICE_DATA_SET_RANGE entry to be UNMAP_BLOCK_DESCRIPTOR entries.

    As LengthInBytes field in DEVICE_DATA_SET_RANGE structure is 64 bits (bytes)
    and LbaCount field in UNMAP_BLOCK_DESCRIPTOR structure is 32 bits (sectors),
    it's possible that one DEVICE_DATA_SET_RANGE entry needs multiple UNMAP_BLOCK_DESCRIPTOR entries.
    We must also take the unmap granularity into consideration and split up the
    the given ranges so that they are aligned with the specified granularity.

Arguments:
    All arguments must be validated by the caller.

    FdoExtension - The FDO extension of the device to which the unmap
        command that will use the resulting unmap block descriptors will be
        sent.
    BlockDescr - Pointer to a buffer that will contain the unmap block
        descriptors.  This buffer should be allocated by the caller and the
        caller should also ensure that it is large enough to contain all the
        requested descriptors.  Its size is implied by MaxBlockDescrIndex.
    CurrentBlockDescrIndex - This contains the next block descriptor index to
        be processed when this function returns.  This function should be called
        again with the same parameter to continue processing.
    MaxBlockDescrIndex - This is the index of the last unmap block descriptor,
        provided so that the function does not go off the end of BlockDescr.
    CurrentLbaCount - This contains the number of LBAs left to be processed
        when this function returns.  This function should be called again with
        the same parameter to continue processing.
    MaxLbaCount - This is the max number of LBAs that can be sent in a single
        unmap command.
    DataSetRange - This range will be modified to reflect the un-converted part.
        It must be valid (including being granularity-aligned) when it is first
        passed to this function.

Return Value:

    Count of UNMAP_BLOCK_DESCRIPTOR entries converted.

    NOTE: if LengthInBytes does not reach to 0, the conversion for DEVICE_DATA_SET_RANGE entry
          is not completed. Further conversion is needed by calling this function again.

--*/
{

    ULONGLONG startingSector;
    ULONGLONG sectorCount;

    TracePrint((TRACE_LEVEL_INFORMATION,
                    TRACE_FLAG_IOCTL,
                    "ConvertDataSetRangeToUnmapBlockDescr (%p): Generating UNMAP Block Descriptors from DataSetRange: \
                     \n\t\tStartingOffset = %I64u bytes \
                     \n\t\tLength = %I64u bytes\n",
                    FdoExtension->DeviceObject,
                    DataSetRange->StartingOffset,
                    DataSetRange->LengthInBytes));

    while ( (DataSetRange->LengthInBytes > 0) &&
            (*CurrentBlockDescrIndex < MaxBlockDescrIndex) &&
            (*CurrentLbaCount < MaxLbaCount) ) {

        //
        // Convert the starting offset and length from bytes to blocks.
        //
        startingSector = (ULONGLONG)(DataSetRange->StartingOffset / FdoExtension->DiskGeometry.BytesPerSector);
        sectorCount = (DataSetRange->LengthInBytes / FdoExtension->DiskGeometry.BytesPerSector);

        //
        // Make sure the sector count isn't more than can be specified with a
        // single descriptor.
        //
        if (sectorCount > MAXULONG) {
            sectorCount = MAXULONG;
        }

        //
        // The max LBA count is the max number of LBAs that can be unmapped with
        // a single UNMAP command.  Make sure we don't exceed this value.
        //
        if ((*CurrentLbaCount + sectorCount) > MaxLbaCount) {
            sectorCount = MaxLbaCount - *CurrentLbaCount;
        }

        REVERSE_BYTES_QUAD(BlockDescr[*CurrentBlockDescrIndex].StartingLba, &startingSector);
        REVERSE_BYTES(BlockDescr[*CurrentBlockDescrIndex].LbaCount, (PULONG)&sectorCount);

        DataSetRange->StartingOffset += sectorCount * FdoExtension->DiskGeometry.BytesPerSector;
        DataSetRange->LengthInBytes -= sectorCount * FdoExtension->DiskGeometry.BytesPerSector;

        *CurrentBlockDescrIndex += 1;
        *CurrentLbaCount += (ULONG)sectorCount;

        TracePrint((TRACE_LEVEL_INFORMATION,
                    TRACE_FLAG_IOCTL,
                    "ConvertDataSetRangeToUnmapBlockDescr (%p): Generated UNMAP Block Descriptor: \
                     \n\t\t\tStartingLBA = %I64u \
                     \n\t\t\tLBACount = %I64u\n",
                    FdoExtension->DeviceObject,
                    startingSector,
                    sectorCount));
    }

    return;
}


NTSTATUS
DeviceProcessDsmTrimRequest(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    _In_ PDEVICE_DATA_SET_RANGE       DataSetRanges,
    _In_ ULONG                        DataSetRangesCount,
    _In_ ULONG                        UnmapGranularity,
    _In_ ULONG                        SrbFlags,
    _In_ PIRP                         Irp,
    _In_ PGUID                        ActivityId,
    _Inout_ PSCSI_REQUEST_BLOCK       Srb
)
/*++

Routine Description:

    Process TRIM request that received from upper layer.

Arguments:

    FdoExtension
    DataSetRanges - this parameter must be already validated in caller.
    DataSetRangesCount - this parameter must be already validated in caller.
    UnmapGranularity - The unmap granularity in blocks.  This is used to split
        up the unmap command into chunks that are granularity-aligned.
    Srb - The SRB to use for the unmap command.  The caller must allocate it,
        but this function will take care of initialzing it.

Return Value:

    status of the operation

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;

    PUNMAP_LIST_HEADER      buffer = NULL;
    PUNMAP_BLOCK_DESCRIPTOR blockDescrPointer;
    ULONG                   bufferLength;
    ULONG                   maxBlockDescrCount;
    ULONG                   neededBlockDescrCount;
    ULONG                   i;

    BOOLEAN                 allDataSetRangeFullyConverted;
    BOOLEAN                 needToSendCommand;
    BOOLEAN                 tempDataSetRangeFullyConverted;

    ULONG                   dataSetRangeIndex;
    DEVICE_DATA_SET_RANGE   tempDataSetRange;

    ULONG                   blockDescrIndex;
    ULONGLONG               lbaCount;
    ULONGLONG               maxLbaCount;
    ULONGLONG               maxParameterListLength;


    UNREFERENCED_PARAMETER(UnmapGranularity);
    UNREFERENCED_PARAMETER(ActivityId);
    UNREFERENCED_PARAMETER(Irp);

    //
    // The given LBA ranges are in DEVICE_DATA_SET_RANGE format and need to be converted into UNMAP Block Descriptors.
    // The UNMAP command is able to carry 0xFFFF bytes (0xFFF8 in reality as there are 8 bytes of header plus n*16 bytes of Block Descriptors) of data.
    // The actual size will also be constrained by the Maximum LBA Count and Maximum Transfer Length.
    //

    //
    // 1.1 Calculate how many Block Descriptors are needed to complete this request.
    //
    neededBlockDescrCount = 0;
    for (i = 0; i < DataSetRangesCount; i++) {
        lbaCount = DataSetRanges[i].LengthInBytes / FdoExtension->DiskGeometry.BytesPerSector;

        //
        // 1.1.1 the UNMAP_BLOCK_DESCRIPTOR LbaCount is 32 bits, the max value is 0xFFFFFFFF
        //
        if (lbaCount > 0) {
            neededBlockDescrCount += (ULONG)((lbaCount - 1) / MAXULONG + 1);
        }
    }

    //
    // Honor Max Unmap Block Descriptor Count if it has been specified.  Otherwise,
    // use the maximum value that the Parameter List Length field will allow (0xFFFF).
    // If the count is 0xFFFFFFFF, then no maximum is specified.
    //
    if (FdoExtension->FunctionSupportInfo->BlockLimitsData.MaxUnmapBlockDescrCount != 0 &&
        FdoExtension->FunctionSupportInfo->BlockLimitsData.MaxUnmapBlockDescrCount != MAXULONG)
    {
        maxParameterListLength = (ULONGLONG)(FdoExtension->FunctionSupportInfo->BlockLimitsData.MaxUnmapBlockDescrCount * sizeof(UNMAP_BLOCK_DESCRIPTOR))
                                 + sizeof(UNMAP_LIST_HEADER);

        //
        // In the SBC-3, the Max Unmap Block Descriptor Count field in the 0xB0
        // page is 4 bytes and the Parameter List Length in the UNMAP command is
        // 2 bytes, therefore it is possible that the Max Unmap Block Descriptor
        // Count could imply more bytes than can be specified in the Parameter
        // List Length field.  Adjust for that here.
        //
        maxParameterListLength = min(maxParameterListLength, MAXUSHORT);
    }
    else
    {
        maxParameterListLength = MAXUSHORT;
    }

    //
    // 1.2 Calculate the buffer size needed, capped by the device's limitations.
    //
    bufferLength = min(FdoExtension->PrivateFdoData->HwMaxXferLen, (ULONG)maxParameterListLength);
    bufferLength = min(bufferLength, (neededBlockDescrCount * sizeof(UNMAP_BLOCK_DESCRIPTOR) + sizeof(UNMAP_LIST_HEADER)));

    maxBlockDescrCount = (bufferLength - sizeof(UNMAP_LIST_HEADER)) / sizeof(UNMAP_BLOCK_DESCRIPTOR);

    if (maxBlockDescrCount == 0) {
        //
        // This shouldn't happen since we've already done validation.
        //
        TracePrint((TRACE_LEVEL_INFORMATION,
                                TRACE_FLAG_IOCTL,
                                "DeviceProcessDsmTrimRequest (%p): Max Block Descriptor count is Zero\n",
                                FdoExtension->DeviceObject));

        NT_ASSERT(maxBlockDescrCount != 0);
        status = STATUS_DATA_ERROR;
        goto Exit;
    }

    //
    // The Maximum LBA Count is set during device initialization.
    //
    maxLbaCount = (ULONGLONG)FdoExtension->FunctionSupportInfo->BlockLimitsData.MaxUnmapLbaCount;
    if (maxLbaCount == 0) {
        //
        // This shouldn't happen since we've already done validation.
        //
        TracePrint((TRACE_LEVEL_INFORMATION,
                                TRACE_FLAG_IOCTL,
                                "DeviceProcessDsmTrimRequest (%p): Max LBA count is Zero\n",
                                FdoExtension->DeviceObject));

        NT_ASSERT(maxLbaCount != 0);
        status = STATUS_DATA_ERROR;
        goto Exit;
    }

    //
    // Finally, allocate the buffer we'll use to send the UNMAP command.
    //

#if defined(_ARM_) || defined(_ARM64_)
    //
    // ARM has specific alignment requirements, although this will not have a functional impact on x86 or amd64
    // based platforms. We are taking the conservative approach here.
    //
    bufferLength = ALIGN_UP_BY(bufferLength,KeGetRecommendedSharedDataAlignment());
    buffer = (PUNMAP_LIST_HEADER)ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned, bufferLength, CLASS_TAG_LB_PROVISIONING);
#else
    buffer = (PUNMAP_LIST_HEADER)ExAllocatePoolWithTag(NonPagedPoolNx, bufferLength, CLASS_TAG_LB_PROVISIONING);
#endif

    if (buffer == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    RtlZeroMemory(buffer, bufferLength);

    blockDescrPointer = &buffer->Descriptors[0];

    allDataSetRangeFullyConverted = FALSE;
    needToSendCommand = FALSE;
    tempDataSetRangeFullyConverted = TRUE;
    dataSetRangeIndex = 0;
    RtlZeroMemory(&tempDataSetRange, sizeof(tempDataSetRange));

    blockDescrIndex = 0;
    lbaCount = 0;


    while (!allDataSetRangeFullyConverted) {

        //
        // If the previous entry conversion completed, go on to the next one;
        // otherwise, continue processing the current entry.
        //
        if (tempDataSetRangeFullyConverted) {
            tempDataSetRange.StartingOffset = DataSetRanges[dataSetRangeIndex].StartingOffset;
            tempDataSetRange.LengthInBytes = DataSetRanges[dataSetRangeIndex].LengthInBytes;
            dataSetRangeIndex++;
        }

        ConvertDataSetRangeToUnmapBlockDescr(FdoExtension,
                                                blockDescrPointer,
                                                &blockDescrIndex,
                                                maxBlockDescrCount,
                                                &lbaCount,
                                                maxLbaCount,
                                                &tempDataSetRange
                                                );

        tempDataSetRangeFullyConverted = (tempDataSetRange.LengthInBytes == 0) ? TRUE : FALSE;

        allDataSetRangeFullyConverted = tempDataSetRangeFullyConverted && (dataSetRangeIndex == DataSetRangesCount);

        //
        // Send the UNMAP command when the buffer is full or when all input entries are converted.
        //
        if ((blockDescrIndex == maxBlockDescrCount) ||         // Buffer full or block descriptor count reached
            (lbaCount == maxLbaCount) ||                       // Block LBA count reached
             allDataSetRangeFullyConverted) {                   // All DataSetRanges have been converted

            USHORT transferSize;
            USHORT tempSize;
            PCDB   cdb;

            //
            // Get the transfer size, including the header.
            //
            transferSize = (USHORT)(blockDescrIndex * sizeof(UNMAP_BLOCK_DESCRIPTOR) + sizeof(UNMAP_LIST_HEADER));
            if (transferSize > bufferLength)
            {
                //
                // This should never happen.
                //
                NT_ASSERT(transferSize <= bufferLength);
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            tempSize = transferSize - (USHORT)FIELD_OFFSET(UNMAP_LIST_HEADER, BlockDescrDataLength);
            REVERSE_BYTES_SHORT(buffer->DataLength, &tempSize);
            tempSize = transferSize - (USHORT)FIELD_OFFSET(UNMAP_LIST_HEADER, Descriptors[0]);
            REVERSE_BYTES_SHORT(buffer->BlockDescrDataLength, &tempSize);

            //
            // Initialize the SRB.
            //
            if (FdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
                status = InitializeStorageRequestBlock((PSTORAGE_REQUEST_BLOCK)Srb,
                                                       STORAGE_ADDRESS_TYPE_BTL8,
                                                       CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE,
                                                       1,
                                                       SrbExDataTypeScsiCdb16);
                if (NT_SUCCESS(status)) {
                    ((PSTORAGE_REQUEST_BLOCK)Srb)->SrbFunction = SRB_FUNCTION_EXECUTE_SCSI;
                } else {
                    //
                    // Should not occur.
                    //
                    NT_ASSERT(FALSE);
                    break;
                }

            } else {
                RtlZeroMemory(Srb, sizeof(SCSI_REQUEST_BLOCK));
                Srb->Length = sizeof(SCSI_REQUEST_BLOCK);
                Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
            }

            //
            // Prepare the Srb
            //
            SrbSetTimeOutValue(Srb, FdoExtension->TimeOutValue);
            SrbSetRequestTag(Srb, SP_UNTAGGED);
            SrbSetRequestAttribute(Srb, SRB_SIMPLE_TAG_REQUEST);

            //
            // Set the SrbFlags to indicate that it's a data-out operation.
            // Also set any passed-in SrbFlags.
            //
            SrbAssignSrbFlags(Srb, FdoExtension->SrbFlags);
            SrbClearSrbFlags(Srb, SRB_FLAGS_DATA_IN);
            SrbSetSrbFlags(Srb, SRB_FLAGS_DATA_OUT);
            SrbSetSrbFlags(Srb, SrbFlags);

            SrbSetCdbLength(Srb, 10);

            cdb = SrbGetCdb(Srb);
            cdb->UNMAP.OperationCode = SCSIOP_UNMAP;
            cdb->UNMAP.Anchor = 0;
            cdb->UNMAP.GroupNumber = 0;
            cdb->UNMAP.AllocationLength[0] = (UCHAR)(transferSize >> 8);
            cdb->UNMAP.AllocationLength[1] = (UCHAR)transferSize;

            status = ClassSendSrbSynchronous(FdoExtension->DeviceObject,
                                             Srb,
                                             buffer,
                                             transferSize,
                                             TRUE);

            TracePrint((TRACE_LEVEL_INFORMATION,
                        TRACE_FLAG_IOCTL,
                        "DeviceProcessDsmTrimRequest (%p): UNMAP command issued. Returned NTSTATUS: %!STATUS!.\n",
                        FdoExtension->DeviceObject,
                        status
                        ));

            //
            // Clear the buffer so we can re-use it.
            //
            blockDescrIndex = 0;
            lbaCount = 0;
            RtlZeroMemory(buffer, bufferLength);
        }
    }

Exit:

    FREE_POOL(buffer);

    return status;
}

NTSTATUS ClasspDeviceTrimProcess(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PGUID ActivityId,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    )
/*
    This function is to process IOCTL_STORAGE_MANAGE_DATA_SET_ATTRIBUTES with DeviceDsmAction_Trim.
    At first time of receiving the request, this function will forward it to lower stack to determine if it's supportted.
    If it's not supported, UNMAP (with anchor attribute set) will be sent down to process the request.
*/
{
    NTSTATUS    status = STATUS_SUCCESS;

    PCOMMON_DEVICE_EXTENSION     commonExtension = (PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    PDEVICE_MANAGE_DATA_SET_ATTRIBUTES dsmAttributes = Irp->AssociatedIrp.SystemBuffer;

    PDEVICE_DATA_SET_RANGE      dataSetRanges;
    ULONG                       dataSetRangesCount;
    DEVICE_DATA_SET_RANGE       entireDataSetRange = {0};
    ULONG                       i;
    ULONGLONG                   granularityAlignmentInBytes;
    ULONG                       granularityInBlocks;
    ULONG                       srbFlags = 0;

    CLASS_VPD_B0_DATA           blockLimitsData;
    ULONG                       generationCount;


    if ( (DeviceObject->DeviceType != FILE_DEVICE_DISK) ||
         (TEST_FLAG(DeviceObject->Characteristics, FILE_FLOPPY_DISKETTE)) ||
         (fdoExtension->FunctionSupportInfo->LowerLayerSupport.TrimProcess == Supported) ) {
        // if it's not disk, forward the request to lower layer,
        // if the IOCTL is supported by lower stack, forward it down.
        IoCopyCurrentIrpStackLocationToNext(Irp);

        TracePrint((TRACE_LEVEL_INFORMATION,
                    TRACE_FLAG_GENERAL,
                    "ClasspDeviceTrimProcess (%p): Lower layer supports Trim DSM IOCTL, forwarding IOCTL.\n",
                    DeviceObject));

        ClassReleaseRemoveLock(DeviceObject, Irp);
        status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
        return status;
    }

    //
    // Request validation.
    // Note that InputBufferLength and IsFdo have been validated beforing entering this routine.
    //

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {
        NT_ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
        status = STATUS_INVALID_LEVEL;
        goto Exit;
    }


    //
    // If the caller has not set the "entire dataset range" flag then at least
    // one dataset range should be specified.  However, if the caller *has* set
    // the flag, then there should not be any dataset ranges specified.
    //
    if ((!TEST_FLAG(dsmAttributes->Flags, DEVICE_DSM_FLAG_ENTIRE_DATA_SET_RANGE) &&
         (dsmAttributes->DataSetRangesOffset == 0 ||
          dsmAttributes->DataSetRangesLength == 0)) ||
        (TEST_FLAG(dsmAttributes->Flags, DEVICE_DSM_FLAG_ENTIRE_DATA_SET_RANGE) &&
         (dsmAttributes->DataSetRangesOffset != 0 ||
          dsmAttributes->DataSetRangesLength != 0))) {

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    //
    // note that 'Supported' case has been handled at the beginning of this function.
    //
    switch (fdoExtension->FunctionSupportInfo->LowerLayerSupport.TrimProcess) {
        case SupportUnknown: {
            // send down request and wait for the request to complete.
            status = ClassForwardIrpSynchronous(commonExtension, Irp);

            TracePrint((TRACE_LEVEL_INFORMATION,
                    TRACE_FLAG_GENERAL,
                    "ClasspDeviceTrimProcess (%p): Trim DSM IOCTL support unknown.  Forwarded IOCTL and received NTSTATUS %!STATUS!.\n",
                    DeviceObject,
                    status));

            if (ClasspLowerLayerNotSupport(status)) {
                // case 1: the request is not supported by lower layer, sends down command
                // some port drivers (or filter drivers) return STATUS_INVALID_DEVICE_REQUEST if a request is not supported.
                // In this case we'll just fall through to the NotSupported case so that we can handle it ourselves.

                //
                // VPD pages 0xB2 and 0xB0 should have been cached in Start Device phase - ClassPnpStartDevice.
                // 0xB2 page: fdoExtension->FunctionSupportInfo->LBProvisioningData;
                // 0xB0 page: fdoExtension->FunctionSupportInfo->BlockLimitsData
                //
                if (fdoExtension->FunctionSupportInfo->ValidInquiryPages.LBProvisioning == TRUE) {
                    NT_ASSERT(fdoExtension->FunctionSupportInfo->LBProvisioningData.CommandStatus != -1);
                }

                if (fdoExtension->FunctionSupportInfo->ValidInquiryPages.BlockLimits == TRUE) {
                    NT_ASSERT(fdoExtension->FunctionSupportInfo->BlockLimitsData.CommandStatus != -1);
                }

            } else {

                // case 2: the request is supported and it completes successfully
                // case 3: the request is supported by lower stack but other failure status is returned.
                // from now on, the same request will be send down to lower stack directly.
                fdoExtension->FunctionSupportInfo->LowerLayerSupport.TrimProcess = Supported;
                goto Exit;
            }
        }

        case NotSupported: {

            // send UNMAP command if it is supported. don't need to check 'status' value.
            if (ClasspSupportsUnmap(fdoExtension->FunctionSupportInfo))
            {
                //
                // Make sure that we know the bytes per sector (logical block) as it's
                // necessary for calculations involving granularity and alignment.
                //
                if (fdoExtension->DiskGeometry.BytesPerSector == 0) {
                    status = ClassReadDriveCapacity(fdoExtension->DeviceObject);
                    if(!NT_SUCCESS(status) || fdoExtension->DiskGeometry.BytesPerSector == 0) {
                        status = STATUS_INVALID_DEVICE_REQUEST;
                        goto Exit;
                    }
                }

                //
                // Take a snapshot of the block limits data since it can change.
                // It's acceptable if the block limits data is outdated since
                // there isn't a hard requirement on the unmap granularity.
                //
                ClasspBlockLimitsDataSnapshot(fdoExtension,
                                              FALSE,
                                              &blockLimitsData,
                                              &generationCount);

                //
                // Check to see if the Optimal Unmap Granularity and Unmap Granularity
                // Alignment have been specified.  If not, default the granularity to
                // one block and the alignment to zero.
                //
                if (blockLimitsData.OptimalUnmapGranularity != 0)
                {
                    granularityInBlocks = blockLimitsData.OptimalUnmapGranularity;
                }
                else
                {
                    granularityInBlocks = 1;

                    TracePrint((TRACE_LEVEL_INFORMATION,
                                TRACE_FLAG_GENERAL,
                                "ClasspDeviceTrimProcess (%p): Optimal Unmap Granularity not provided, defaulted to 1.\n",
                                DeviceObject));
                }

                if (blockLimitsData.UGAVALID == TRUE)
                {
                    granularityAlignmentInBytes = (ULONGLONG)blockLimitsData.UnmapGranularityAlignment * fdoExtension->DiskGeometry.BytesPerSector;
                }
                else
                {
                    granularityAlignmentInBytes = 0;

                    TracePrint((TRACE_LEVEL_INFORMATION,
                                TRACE_FLAG_GENERAL,
                                "ClasspDeviceTrimProcess (%p): Unmap Granularity Alignment not provided, defaulted to 0.\n",
                                DeviceObject));
                }

                if (TEST_FLAG(dsmAttributes->Flags, DEVICE_DSM_FLAG_ENTIRE_DATA_SET_RANGE))
                {
                    //
                    // The caller wants to UNMAP the entire disk so we need to build a single
                    // dataset range that represents the entire disk.
                    //
                    entireDataSetRange.StartingOffset = granularityAlignmentInBytes;
                    entireDataSetRange.LengthInBytes = (ULONGLONG)fdoExtension->CommonExtension.PartitionLength.QuadPart - (ULONGLONG)entireDataSetRange.StartingOffset;

                    dataSetRanges = &entireDataSetRange;
                    dataSetRangesCount = 1;
                }
                else
                {

                    dataSetRanges = (PDEVICE_DATA_SET_RANGE)((PUCHAR)dsmAttributes + dsmAttributes->DataSetRangesOffset);
                    dataSetRangesCount = dsmAttributes->DataSetRangesLength / sizeof(DEVICE_DATA_SET_RANGE);

                    //
                    // Validate the data ranges.  Make sure the range is block-aligned,
                    // falls in a valid portion of the disk, and is non-zero.
                    //
                    for (i = 0; i < dataSetRangesCount; i++)
                    {
                        if ((dataSetRanges[i].StartingOffset % fdoExtension->DiskGeometry.BytesPerSector != 0) ||
                            (dataSetRanges[i].LengthInBytes % fdoExtension->DiskGeometry.BytesPerSector != 0) ||
                            (dataSetRanges[i].StartingOffset < (LONGLONG)granularityAlignmentInBytes) ||
                            (dataSetRanges[i].LengthInBytes == 0) ||
                            ((ULONGLONG)dataSetRanges[i].StartingOffset + dataSetRanges[i].LengthInBytes > (ULONGLONG)fdoExtension->CommonExtension.PartitionLength.QuadPart))
                        {
                            TracePrint((TRACE_LEVEL_ERROR,
                                        TRACE_FLAG_IOCTL,
                                        "ClasspDeviceTrimProcess (%p): Invalid dataset range.  StartingOffset = %I64x, LengthInBytes = %I64x\n",
                                        DeviceObject,
                                        dataSetRanges[i].StartingOffset,
                                        dataSetRanges[i].LengthInBytes));

                            status = STATUS_INVALID_PARAMETER;
                            goto Exit;
                        }
                    }
                }


                if (!TEST_FLAG(dsmAttributes->Flags, DEVICE_DSM_FLAG_TRIM_NOT_FS_ALLOCATED))
                {
                    {
                        //
                        // For security reasons, file-level TRIM must be forwarded on only
                        // if reading the unmapped blocks' contents will return back zeros.
                        // This is because if LBPRZ bit is not set, it indicates that a read
                        // of unmapped blocks may return "any" data thus potentially leaking
                        // in data (into the read buffer) from other blocks.
                        //
                        if (fdoExtension->FunctionSupportInfo->ValidInquiryPages.LBProvisioning &&
                            !fdoExtension->FunctionSupportInfo->LBProvisioningData.LBPRZ) {

                            TracePrint((TRACE_LEVEL_ERROR,
                                        TRACE_FLAG_IOCTL,
                                        "ClasspDeviceTrimProcess (%p): Device does not support file level TRIM.\n",
                                        DeviceObject));

                            status = STATUS_TRIM_READ_ZERO_NOT_SUPPORTED;
                            goto Exit;
                        }
                    }
                }

                // process DSM IOCTL
                status = DeviceProcessDsmTrimRequest(fdoExtension,
                                                     dataSetRanges,
                                                     dataSetRangesCount,
                                                     granularityInBlocks,
                                                     srbFlags,
                                                     Irp,
                                                     ActivityId,
                                                     Srb);
            } else {
                // DSM IOCTL should be completed as not supported

                TracePrint((TRACE_LEVEL_ERROR,
                            TRACE_FLAG_IOCTL,
                            "ClasspDeviceTrimProcess (%p): Device does not support UNMAP.\n",
                            DeviceObject));

                status = STATUS_NOT_SUPPORTED;
            }

            // set the support status after the SCSI command is executed to avoid racing condition between multiple same type of requests.
            fdoExtension->FunctionSupportInfo->LowerLayerSupport.TrimProcess = NotSupported;

            break;
        }

        case Supported: {
            NT_ASSERT(FALSE); // this case is handled at the begining of the function.
            break;
        }

    }   // end of switch (fdoExtension->FunctionSupportInfo->LowerLayerSupport.TrimProcess)

Exit:

    //
    // Set the size and status in IRP
    //
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;



    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
GetLBAStatus(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    _In_ PSCSI_REQUEST_BLOCK          Srb,
    _In_ ULONGLONG                    StartingLBA,
    _Inout_ PLBA_STATUS_LIST_HEADER   LBAStatusHeader,
    _In_ ULONG                        LBAStatusSize,
    _In_ BOOLEAN                      ConsolidateableBlocksOnly
    )
/*++

Routine Description:

    Send down a Get LBA Status command for the given range.

Arguments:
    FdoExtension: The FDO extension of the device to which Get LBA Status will
        be sent.
    Srb: This should be allocated and initialized before it's passed in.  It
        will be used for the Get LBA Status command.
    StartingLBA: The LBA that is at the beginning of the requested range.
    LBAStatusHeader: Caller-allocated output buffer.
    LBASTatusSize: Size of the caller-allocated output buffer.

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PCDB                    cdb;

    if (LBAStatusHeader == NULL || LBAStatusSize == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Build and send down the Get LBA Status command.
    //
    SrbSetTimeOutValue(Srb, FdoExtension->TimeOutValue);
    SrbSetRequestTag(Srb, SP_UNTAGGED);
    SrbSetRequestAttribute(Srb, SRB_SIMPLE_TAG_REQUEST);
    SrbAssignSrbFlags(Srb, FdoExtension->SrbFlags);
    SrbSetCdbLength(Srb, sizeof(cdb->GET_LBA_STATUS));


    cdb = SrbGetCdb(Srb);
    cdb->GET_LBA_STATUS.OperationCode = SCSIOP_GET_LBA_STATUS;
    cdb->GET_LBA_STATUS.ServiceAction = SERVICE_ACTION_GET_LBA_STATUS;
    REVERSE_BYTES_QUAD(&(cdb->GET_LBA_STATUS.StartingLBA), &StartingLBA);
    REVERSE_BYTES(&(cdb->GET_LBA_STATUS.AllocationLength), &LBAStatusSize);

    TracePrint((TRACE_LEVEL_INFORMATION,
                TRACE_FLAG_IOCTL,
                "GetLBAStatus (%p): sending command with StartingLBA = 0x%I64x, AllocationLength = 0x%I64x, ConsolidateableBlocksOnly = %u\n",
                FdoExtension->DeviceObject,
                StartingLBA,
                LBAStatusSize,
                ConsolidateableBlocksOnly));

    status = ClassSendSrbSynchronous(FdoExtension->DeviceObject,
                                        Srb,
                                        LBAStatusHeader,
                                        LBAStatusSize,
                                        FALSE);

    //
    // Handle the case where we get back STATUS_DATA_OVERRUN b/c the input
    // buffer was larger than necessary.
    //
    if (status == STATUS_DATA_OVERRUN &&
        SrbGetDataTransferLength(Srb) < LBAStatusSize)
    {
        status = STATUS_SUCCESS;
    }

    // log command.
    TracePrint((TRACE_LEVEL_INFORMATION,
                TRACE_FLAG_IOCTL,
                "GetLBAStatus (%p): command returned NT Status: %!STATUS!\n",
                FdoExtension->DeviceObject,
                status
                ));

    return status;
}


NTSTATUS ClasspDeviceGetLBAStatus(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    )
/*
Routine Description:

    This function is to process IOCTL_STORAGE_MANAGE_DATA_SET_ATTRIBUTES with DeviceDsmAction_Allocation.

    1. This function will only handle the first dataset range.
    2. This function will not handle dataset ranges whose LengthInBytes is greater than:
        ((MAXULONG - sizeof(LBA_STATUS_LIST_HEADER)) / sizeof(LBA_STATUS_DESCRIPTOR)) * BytesPerSlab

    The input buffer should consist of a DEVICE_MANAGE_DATA_SET_ATTRIBUTES followed
    in memory by a single DEVICE_DATA_SET_RANGE that specifies the requested range
    of slabs for which mapping status is desired.

    The output buffer will consist of a DEVICE_MANAGE_DATA_SET_ATTRIBUTES_OUTPUT
    followed in memory by a single DEVICE_DATA_SET_LB_PROVISIONING_STATE that
    contains a bitmap that represents the mapped status of the slabs in the requested
    range.  Note that the number of slabs returned may be less than the number
    requested.

    Thus function will automatically re-align the given range offset if it was
    not slab-aligned.  The delta between the given range offset and the properly
    aligned offset will be given in returned DEVICE_DATA_SET_LB_PROVISIONING_STATE.

Arguments:
    DeviceObject: The FDO of the device to which Get LBA Status will be sent.
    Irp: The IRP for the request.  This function will read the input buffer and
        write to the output buffer at the current IRP stack location.
    Srb: This should be allocated and initialized before it's passed in.  It
        will be used for the Get LBA Status command.

Return Value:

    STATUS_INVALID_PARAMETER: May be returned under the following conditions:
        - If the requested range was too large.  The caller should try again with a
            smaller range.  See above for how to calculate the maximum range.
        - If the given starting offset was not within the valid range of the device.
    STATUS_NOT_SUPPORTED: The storage did not report some information critical to
        the execution of this function (e.g. Optimal Unmap Granularity).
    STATUS_BUFFER_TOO_SMALL: The output buffer is not large enough to hold the max
        data that could be returned from this function.  If the output buffer is
        at least the size of a ULONG, we will write the required output buffer size
        to the first ULONG bytes of the output buffer.
    STATUS_UNSUCCESSFUL: The Get LBA Status command succeeded but did not
        return data as expected.
--*/
{
    PFUNCTIONAL_DEVICE_EXTENSION               fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION                         irpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_MANAGE_DATA_SET_ATTRIBUTES         dsmAttributes = (PDEVICE_MANAGE_DATA_SET_ATTRIBUTES)Irp->AssociatedIrp.SystemBuffer;
    PDEVICE_DATA_SET_RANGE                     dataSetRanges = NULL;
    PDEVICE_MANAGE_DATA_SET_ATTRIBUTES_OUTPUT  dsmOutput = (PDEVICE_MANAGE_DATA_SET_ATTRIBUTES_OUTPUT)Irp->AssociatedIrp.SystemBuffer;
    ULONG                                      dsmOutputLength;
    NTSTATUS                                   finalStatus;
    NTSTATUS                                   getLBAWorkerStatus;
    ULONG                                      retryCount;
    ULONG                                      retryCountMax;
    CLASS_VPD_B0_DATA                          blockLimitsData;
    ULONG                                      generationCount1;
    ULONG                                      generationCount2;
    BOOLEAN                                    blockLimitsDataMayHaveChanged;
    ULONG_PTR                                  information = 0;
    LONGLONG                                   startingOffset;
    ULONGLONG                                  lengthInBytes;
    BOOLEAN                                    consolidateableBlocksOnly = FALSE;
    ULONG                                      outputVersion;

    //
    // Basic parameter validation.
    // Note that InputBufferLength and IsFdo have been validated beforing entering this routine.
    //
    if (dsmOutput == NULL ||
        dsmAttributes == NULL)
    {
        finalStatus = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (TEST_FLAG(dsmAttributes->Flags, DEVICE_DSM_FLAG_ENTIRE_DATA_SET_RANGE)) {
        //
        // The caller wants the mapping status of the entire disk.
        //
        ULONG unmapGranularityAlignment = 0;
        if (fdoExtension->FunctionSupportInfo->BlockLimitsData.UGAVALID) {
            unmapGranularityAlignment = fdoExtension->FunctionSupportInfo->BlockLimitsData.UnmapGranularityAlignment;
        }
        startingOffset = unmapGranularityAlignment;
        lengthInBytes = (ULONGLONG)fdoExtension->CommonExtension.PartitionLength.QuadPart - (ULONGLONG)startingOffset;
    } else {
        if (dsmAttributes->DataSetRangesOffset == 0 ||
            dsmAttributes->DataSetRangesLength == 0) {
            finalStatus = STATUS_INVALID_PARAMETER;
            goto Exit;
        }

        //
        // We only service the first dataset range specified.
        //
        dataSetRanges = (PDEVICE_DATA_SET_RANGE)((PUCHAR)dsmAttributes + dsmAttributes->DataSetRangesOffset);
        startingOffset = dataSetRanges[0].StartingOffset;
        lengthInBytes = dataSetRanges[0].LengthInBytes;
    }


    //
    // See if the sender is requesting a specific version of the output data
    // structure.  Othwerwise, default to V1.
    //
    outputVersion = DEVICE_DATA_SET_LB_PROVISIONING_STATE_VERSION_V1;
#if (NTDDI_VERSION >= NTDDI_WINBLUE)
    if ((dsmAttributes->ParameterBlockOffset >= sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES)) &&
        (dsmAttributes->ParameterBlockLength >= sizeof(DEVICE_DATA_SET_LBP_STATE_PARAMETERS))) {
        PDEVICE_DATA_SET_LBP_STATE_PARAMETERS parameters = Add2Ptr(dsmAttributes, dsmAttributes->ParameterBlockOffset);
        if ((parameters->Version == DEVICE_DATA_SET_LBP_STATE_PARAMETERS_VERSION_V1) &&
            (parameters->Size >= sizeof(DEVICE_DATA_SET_LBP_STATE_PARAMETERS))) {

            outputVersion = parameters->OutputVersion;

            if ((outputVersion != DEVICE_DATA_SET_LB_PROVISIONING_STATE_VERSION_V1) &&
                (outputVersion != DEVICE_DATA_SET_LB_PROVISIONING_STATE_VERSION_V2)) {
                finalStatus = STATUS_INVALID_PARAMETER;
                goto Exit;
            }
        }
    }
#endif

    //
    // Take a snapshot of the block limits data for the worker function to use.
    // We need to fail the request if we fail to get updated block limits data
    // since we need an accurate Optimal Unmap Granularity value to properly
    // convert the returned mapping descriptors into a bitmap.
    //
    finalStatus = ClasspBlockLimitsDataSnapshot(fdoExtension,
                                                TRUE,
                                                &blockLimitsData,
                                                &generationCount1);

    if (!NT_SUCCESS(finalStatus)) {
        information = 0;
        goto Exit;
    }

    if (dsmAttributes->Flags & DEVICE_DSM_FLAG_ALLOCATION_CONSOLIDATEABLE_ONLY) {
        consolidateableBlocksOnly = TRUE;
    }

    //
    // The retry logic is to handle the case when block limits data changes during rare occasions
    // (e.g. diff-VHD fork or merge).
    //
    retryCountMax = GET_LBA_STATUS_RETRY_COUNT_MAX;
    for (retryCount = 0; retryCount < retryCountMax; retryCount++) {

        dsmOutputLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
        getLBAWorkerStatus = ClasspDeviceGetLBAStatusWorker(DeviceObject,
                                                            &blockLimitsData,
                                                            startingOffset,
                                                            lengthInBytes,
                                                            dsmOutput,
                                                            &dsmOutputLength,
                                                            Srb,
                                                            consolidateableBlocksOnly,
                                                            outputVersion,
                                                            &blockLimitsDataMayHaveChanged);

        if (!NT_SUCCESS(getLBAWorkerStatus) && !blockLimitsDataMayHaveChanged) {
            information = 0;
            finalStatus = getLBAWorkerStatus;
            break;
        }

        //
        // Again, we need to fail the request if we fail to get updated block
        // limits data since we need an accurate Optimal Unmap Granularity value.
        //
        finalStatus = ClasspBlockLimitsDataSnapshot(fdoExtension,
                                                    TRUE,
                                                    &blockLimitsData,
                                                    &generationCount2);
        if (!NT_SUCCESS(finalStatus)) {
            information = 0;
            goto Exit;
        }

        if (generationCount1 == generationCount2) {
            //
            // Block limits data stays the same during the call to ClasspDeviceGetLBAStatusWorker()
            // The result from ClasspDeviceGetLBAStatusWorker() is valid.
            //
            finalStatus = getLBAWorkerStatus;
            if (NT_SUCCESS(finalStatus)) {
                information = dsmOutputLength;
            }
            break;
        }

        //
        // Try again with the latest block limits data
        //
        generationCount1 = generationCount2;
        information = 0;
        finalStatus = STATUS_DEVICE_DATA_ERROR;
    }

Exit:
    Irp->IoStatus.Information = information;
    Irp->IoStatus.Status = finalStatus;
    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
    return finalStatus;
}

NTSTATUS
ClasspDeviceGetLBAStatusWorker(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PCLASS_VPD_B0_DATA BlockLimitsData,
    _In_ ULONGLONG StartingOffset,
    _In_ ULONGLONG LengthInBytes,
    _Out_ PDEVICE_MANAGE_DATA_SET_ATTRIBUTES_OUTPUT DsmOutput,
    _Inout_ PULONG DsmOutputLength,
    _Inout_ PSCSI_REQUEST_BLOCK Srb,
    _In_ BOOLEAN ConsolidateableBlocksOnly,
    _In_ ULONG OutputVersion,
    _Out_ PBOOLEAN BlockLimitsDataMayHaveChanged
    )
/*
Routine Description:

    This function is to process IOCTL_STORAGE_MANAGE_DATA_SET_ATTRIBUTES with DeviceDsmAction_Allocation.

    1. This function will only handle the first dataset range.
    2. This function will not handle dataset ranges whose LengthInBytes is greater than:
        ((MAXULONG - sizeof(LBA_STATUS_LIST_HEADER)) / sizeof(LBA_STATUS_DESCRIPTOR)) * BytesPerSlab

    The input buffer should consist of a DEVICE_MANAGE_DATA_SET_ATTRIBUTES followed
    in memory by a single DEVICE_DATA_SET_RANGE that specifies the requested range
    of slabs for which mapping status is desired.

    The output buffer will consist of a DEVICE_MANAGE_DATA_SET_ATTRIBUTES_OUTPUT
    followed in memory by a single DEVICE_DATA_SET_LB_PROVISIONING_STATE that
    contains a bitmap that represents the mapped status of the slabs in the requested
    range.  Note that the number of slabs returned may be less than the number
    requested.

    Thus function will automatically re-align the given range offset if it was
    not slab-aligned.  The delta between the given range offset and the properly
    aligned offset will be given in returned DEVICE_DATA_SET_LB_PROVISIONING_STATE.

Arguments:
    DeviceObject: The FDO of the device to which Get LBA Status will be sent.
    BlockLimitsData: Block limits data of the device
    StartingOffset: Starting byte offset of byte range to query LBA status (must be sector aligned)
    LengthInBytes: Length of byte range to query LBA status (multiple of sector size)
    DsmOutput: Output data buffer
    DsmOutputLength: output data buffer size.  It will be updated with actual bytes used.
    Srb: This should be allocated and initialized before it's passed in.  It
        will be used for the Get LBA Status command.
    ConsolidateableBlocksOnly: Only blocks that are eligible for consolidation
        should be returned.
    OutputVersion: The version of the DEVICE_DATA_SET_LB_PROVISIONING_STATE
        structure to return.  This should be one of:
            - DEVICE_DATA_SET_LB_PROVISIONING_STATE_VERSION_V1
            - DEVICE_DATA_SET_LB_PROVISIONING_STATE_VERSION_V2
    BlockLimitsDataMayHaveChanged: if this function fails, this flag indicates
        if the failure can be caused by changes in device's block limit data.

Return Value:

    STATUS_INVALID_PARAMETER: May be returned under the following conditions:
        - If the requested range was too large.  The caller should try again with a
            smaller range.  See above for how to calculate the maximum range.
        - If the given starting offset was not within the valid range of the device.
    STATUS_NOT_SUPPORTED: The storage did not report some information critical to
        the execution of this function (e.g. Optimal Unmap Granularity).
    STATUS_BUFFER_TOO_SMALL: The output buffer is not large enough to hold the max
        data that could be returned from this function.  If the output buffer is
        at least the size of a ULONG, we will write the required output buffer size
        to the first ULONG bytes of the output buffer.
    STATUS_DEVICE_DATA_ERROR: The Get LBA Status command succeeded but did not
        return data as expected.
--*/
{
    NTSTATUS    status = STATUS_SUCCESS;

    PFUNCTIONAL_DEVICE_EXTENSION               fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    PDEVICE_DATA_SET_LB_PROVISIONING_STATE     lbpState;
    ULONG                                      bitMapGranularityInBits = FIELD_SIZE(DEVICE_DATA_SET_LB_PROVISIONING_STATE,SlabAllocationBitMap[0]) * 8;
    ULONG                                      requiredOutputLength;
    ULONG                                      outputLength = *DsmOutputLength;

    ULONG                                      blocksPerSlab;
    ULONGLONG                                  bytesPerSlab;
    ULONGLONG                                  alignmentInBytes = 0;
    ULONG                                      alignmentInBlocks = 0;
    ULONG                                      maxBufferSize;
    ULONG                                      maxSlabs;
    ULONGLONG                                  requestedSlabs; // Total number of slabs requested by the caller.
    ULONGLONG                                  startingLBA;
    ULONGLONG                                  startingOffsetDelta;
    ULONG                                      totalProcessedSlabs = 0; // Total number of slabs we processed.
    ULONGLONG                                  slabsPerCommand; // Number of slabs we can ask for in one Get LBA Status command.
    BOOLEAN                                    doneProcessing = FALSE; // Indicates we should break out of the Get LBA Status loop.

    ULONG                                      lbaStatusSize;
    PLBA_STATUS_LIST_HEADER                    lbaStatusListHeader = NULL;

    //
    // This function can fail if the block limits data on the device changes.
    // This flag tells the caller if it should retry with a newer block limits data
    //
    *BlockLimitsDataMayHaveChanged = FALSE;

    //
    // Make sure we're running at PASSIVE_LEVEL
    //
    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
    {
        NT_ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
        status = STATUS_INVALID_LEVEL;
        goto Exit;
    }

    //
    // Don't send down a Get LBA Status command if UNMAP isn't supported.
    //
    if (!fdoExtension->FunctionSupportInfo->LBProvisioningData.LBPU)
    {
        return STATUS_NOT_SUPPORTED;
        goto Exit;
    }

    //
    // Make sure we have a non-zero value for the number of bytes per block.
    // Otherwise we will end up dividing by zero later on.
    //
    if (fdoExtension->DiskGeometry.BytesPerSector == 0)
    {
        status = ClassReadDriveCapacity(fdoExtension->DeviceObject);
        if(!NT_SUCCESS(status) || fdoExtension->DiskGeometry.BytesPerSector == 0)
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            goto Exit;
        }
    }

    //
    // We only service the first dataset range specified.
    //
    if (BlockLimitsData->UGAVALID == TRUE) {
        alignmentInBlocks = BlockLimitsData->UnmapGranularityAlignment;
        alignmentInBytes = (ULONGLONG)alignmentInBlocks * (ULONGLONG)fdoExtension->DiskGeometry.BytesPerSector;
    }

    //
    // Make sure the specified range is valid.  The Unmap Granularity Alignment
    // defines a region at the beginning of the disk that cannot be
    // mapped/unmapped so the specified range should not include any part of that
    // region.
    //
    if (LengthInBytes == 0 ||
        StartingOffset < alignmentInBytes ||
        StartingOffset + LengthInBytes > (ULONGLONG)fdoExtension->CommonExtension.PartitionLength.QuadPart)
    {
        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspDeviceGetLBAStatusWorker (%p): Invalid range, length is %I64u bytes, starting offset is %I64u bytes, Unmap alignment is %I64u bytes, and disk size is %I64u bytes\n",
                    DeviceObject,
                    LengthInBytes,
                    StartingOffset,
                    alignmentInBytes,
                    (ULONGLONG)fdoExtension->CommonExtension.PartitionLength.QuadPart));

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    //
    // Calculate the number of bytes per slab so that we can convert (and
    // possibly align) the given offset (given in bytes) to slabs.
    //
    blocksPerSlab = BlockLimitsData->OptimalUnmapGranularity;
    bytesPerSlab = (ULONGLONG)blocksPerSlab * (ULONGLONG)fdoExtension->DiskGeometry.BytesPerSector;

    //
    // If the starting offset is not slab-aligned, we need to adjust it to
    // be aligned with the next highest slab.  We also need to save the delta
    // to return to the user later.
    //
    if (((StartingOffset - alignmentInBytes) % bytesPerSlab) != 0)
    {
        startingLBA = (((StartingOffset - alignmentInBytes) / bytesPerSlab) + 1) * (ULONGLONG)blocksPerSlab + alignmentInBlocks;
        startingOffsetDelta = (startingLBA * fdoExtension->DiskGeometry.BytesPerSector) - StartingOffset;
    }
    else
    {
        startingLBA = ((StartingOffset - alignmentInBytes) / bytesPerSlab) * (ULONGLONG)blocksPerSlab + alignmentInBlocks;
        startingOffsetDelta = 0;
    }

    //
    // Caclulate the number of slabs the caller requested.
    //
    if ((LengthInBytes % bytesPerSlab) == 0) {
        requestedSlabs = (LengthInBytes / bytesPerSlab);
    } else {
        //
        // Round up the number of requested slabs if the length indicates a
        // partial slab.  This should cover the case where the user specifies
        // a dataset range for the whole disk, but the size of the disk is not
        // a slab-multiple.  Rounding up allows us to return the status of the
        // partial slab
        //
        requestedSlabs = (LengthInBytes / bytesPerSlab) + 1;
    }

    //
    // If the caller asked for no slabs then return STATUS_INVALID_PARAMETER.
    //
    if (requestedSlabs == 0)
    {
        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspDeviceGetLBAStatusWorker (%p): Invalid number (%I64u) of slabs requested\n",
                    DeviceObject,
                    requestedSlabs));

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    //
    // Cap requested slabs at MAXULONG since SlabAllocationBitMapBitCount
    // is a 4-byte field.  We may return less data than requested, but the
    // caller can simply re-query for the omitted portion(s).
    //
    requestedSlabs = min(requestedSlabs, MAXULONG);

    //
    // Calculate the required size of the output buffer based upon the desired
    // version of the output structure.
    // In the worst case, Get LBA Status returns a descriptor for each slab
    // requested, thus the required output buffer length is equal to:
    //  1. The size of DEVICE_MANAGE_DATA_SET_ATTRIBUTES_OUTPUT; plus
    //  2. The size of DEVICE_DATA_SET_LB_PROVISIONING_STATE(_V2); plus
    //  3. The size of a ULONG array large enough to hold a bit for each slab requested.
    //     (The first element is already allocated in DEVICE_DATA_SET_LB_PROVISIONING_STATE(_V2).)
    //
#if (NTDDI_VERSION >= NTDDI_WINBLUE)
    if (OutputVersion == DEVICE_DATA_SET_LB_PROVISIONING_STATE_VERSION_V2) {

        requiredOutputLength = (ULONG)(sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES_OUTPUT)
                                        + sizeof(DEVICE_DATA_SET_LB_PROVISIONING_STATE_V2)
                                        + (((requestedSlabs - 1) / bitMapGranularityInBits))
                                           * FIELD_SIZE(DEVICE_DATA_SET_LB_PROVISIONING_STATE_V2, SlabAllocationBitMap[0]));

    } else
#else
    UNREFERENCED_PARAMETER(OutputVersion);
#endif
    {

        requiredOutputLength = (ULONG)(sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES_OUTPUT)
                                        + sizeof(DEVICE_DATA_SET_LB_PROVISIONING_STATE)
                                        + (((requestedSlabs - 1) / bitMapGranularityInBits))
                                           * FIELD_SIZE(DEVICE_DATA_SET_LB_PROVISIONING_STATE, SlabAllocationBitMap[0]));
    }

    //
    // The output buffer is not big enough to hold the requested data.
    // Inform the caller of the correct buffer size.
    //
    if (outputLength < requiredOutputLength)
    {
        status = STATUS_BUFFER_TOO_SMALL;

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspDeviceGetLBAStatusWorker (%p): Given output buffer is %u bytes, needs to be %u bytes\n",
                    DeviceObject,
                    outputLength,
                    requiredOutputLength));

        //
        // If the output buffer is big enough, write the required buffer
        // length to the first ULONG bytes of the output buffer.
        //
        if (outputLength >= sizeof(ULONG))
        {
            *((PULONG)DsmOutput) = requiredOutputLength;
        }

        goto Exit;
    }

    //
    // Calculate the maximum number of slabs that could be returned by a single
    // Get LBA Status command.  The max buffer size could either be capped by
    // the Parameter Data Length field or the Max Transfer Length of the
    // adapter.
    // The number of slabs we actually ask for in a single command is the
    // smaller of the number of slabs requested by the user or the max number
    // of slabs we can theoretically ask for in a single command.
    //
    maxBufferSize = MIN(MAXULONG, fdoExtension->PrivateFdoData->HwMaxXferLen);
    maxSlabs = (maxBufferSize - sizeof(LBA_STATUS_LIST_HEADER)) / sizeof(LBA_STATUS_DESCRIPTOR);
    slabsPerCommand = min(requestedSlabs, maxSlabs);

    //
    // Allocate the buffer that will contain the returned LBA Status Descriptors.
    // Assume that in the worst case every other slab has a different mapping
    // status.  That means that there may be a descriptor for every slab requested.
    //
    lbaStatusSize = (ULONG)(sizeof(LBA_STATUS_LIST_HEADER) + (slabsPerCommand * sizeof(LBA_STATUS_DESCRIPTOR)));
#if defined(_ARM_) || defined(_ARM64_)
    //
    // ARM has specific alignment requirements, although this will not have a functional impact on x86 or amd64
    // based platforms. We are taking the conservative approach here.
    //
    lbaStatusSize = ALIGN_UP_BY(lbaStatusSize,KeGetRecommendedSharedDataAlignment());
    lbaStatusListHeader = (PLBA_STATUS_LIST_HEADER)ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned, lbaStatusSize, CLASS_TAG_LB_PROVISIONING);
#else
    lbaStatusListHeader = (PLBA_STATUS_LIST_HEADER)ExAllocatePoolWithTag(NonPagedPoolNx, lbaStatusSize, CLASS_TAG_LB_PROVISIONING);
#endif

    if (lbaStatusListHeader == NULL)
    {
        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspDeviceGetLBAStatusWorker (%p): Failed to allocate %u bytes for descriptors\n",
                    DeviceObject,
                    lbaStatusSize));

        NT_ASSERT(lbaStatusListHeader != NULL);
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    //
    // Set default values for the output buffer.
    // If we process at least one slab from the device we will update the
    // offset and lengths accordingly.
    //
    DsmOutput->Action = DeviceDsmAction_Allocation;
    DsmOutput->Size = sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES_OUTPUT);
    DsmOutput->OutputBlockOffset = 0;
    DsmOutput->OutputBlockLength = 0;
    *DsmOutputLength = DsmOutput->Size;

    //
    // The returned DEVICE_DATA_SET_LB_PROVISIONING_STATE is at the end of the
    // DSM output structure.  Zero it out before we start to fill it in.
    //
    lbpState = Add2Ptr(DsmOutput, sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES_OUTPUT));
    RtlZeroMemory(lbpState, requiredOutputLength - sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES_OUTPUT));

    do {
        //
        // Send down GetLBAStatus for the current range.
        //
        status = GetLBAStatus(fdoExtension,
                              Srb,
                              startingLBA,
                              lbaStatusListHeader,
                              lbaStatusSize,
                              ConsolidateableBlocksOnly);

        if (NT_SUCCESS(status))
        {
            ULONG descrIndex = 0;
            ULONG descrSize = 0;
            ULONG descrSizeOverhead;
            ULONG descrCount = 0;
            ULONGLONG expectedStartingLBA;
            BOOLEAN processCurrentDescriptor = TRUE;
            ULONG commandProcessedSlabs = 0; // Number of slabs processed for this command.

            descrSizeOverhead = FIELD_OFFSET(LBA_STATUS_LIST_HEADER, Descriptors[0]) -
                                RTL_SIZEOF_THROUGH_FIELD(LBA_STATUS_LIST_HEADER, ParameterLength);
            REVERSE_BYTES(&descrSize, &(lbaStatusListHeader->ParameterLength));

            //
            // If the returned Parameter Data Length field describes more
            // descriptors than we allocated space for then make sure we don't
            // try to process more descriptors than are actually present.
            //
            if (descrSize > (lbaStatusSize - RTL_SIZEOF_THROUGH_FIELD(LBA_STATUS_LIST_HEADER, ParameterLength))) {
                descrSize = (lbaStatusSize - RTL_SIZEOF_THROUGH_FIELD(LBA_STATUS_LIST_HEADER, ParameterLength));
            }

            if (descrSize >= descrSizeOverhead) {
                descrSize -= descrSizeOverhead;
                descrCount = descrSize / sizeof(LBA_STATUS_DESCRIPTOR);

                //
                // Make sure at least one descriptor was returned.
                //
                if (descrCount > 0) {
                    //
                    // We expect the first starting LBA returned by the device to be the
                    // same starting LBA we specified in the command.
                    //
                    expectedStartingLBA = startingLBA;

                    //
                    // Translate the returned LBA status descriptors into a bitmap where each bit represents
                    // a slab.  The slab size is represented by the Optimal Unmap Granularity.
                    // 1 = The slab is mapped.
                    // 0 = The slab is unmapped (deallocated or anchored).
                    //
                    for (descrIndex = 0; descrIndex < descrCount && totalProcessedSlabs < requestedSlabs && !doneProcessing; descrIndex++)
                    {
                        PLBA_STATUS_DESCRIPTOR lbaStatusDescr = &(lbaStatusListHeader->Descriptors[descrIndex]);
                        ULONGLONG returnedStartingLBA;
                        ULONG mapped = (lbaStatusDescr->ProvisioningStatus != LBA_STATUS_MAPPED) ? 0x0 : 0x1;
                        ULONG lbaCount = 0;

                        REVERSE_BYTES_QUAD(&returnedStartingLBA, &(lbaStatusDescr->StartingLBA));
                        REVERSE_BYTES(&lbaCount, &(lbaStatusDescr->LogicalBlockCount));

                        if (returnedStartingLBA != expectedStartingLBA)
                        {
                            //
                            // We expect the descriptors will express a contiguous range of LBAs.
                            // If the starting LBA is not contiguous with the LBA range from the
                            // previous descriptor then we should not process any more descriptors,
                            // including the current one.
                            //
                            TracePrint((TRACE_LEVEL_ERROR,
                                        TRACE_FLAG_IOCTL,
                                        "ClasspDeviceGetLBAStatusWorker (%p): Device returned starting LBA = %I64x when %I64x was expected.\n",
                                        DeviceObject,
                                        returnedStartingLBA,
                                        startingLBA));

                            doneProcessing = TRUE;
                            processCurrentDescriptor = FALSE;
                            *BlockLimitsDataMayHaveChanged = TRUE;
                        }
                        else if (lbaCount > 0 && lbaCount % blocksPerSlab != 0)
                        {
                            //
                            // If the device returned an LBA count with a partial slab, round
                            // the LBA count up to the nearest slab and set a flag to stop
                            // processing further descriptors.  This is mainly to handle the
                            // case where disk size may not be slab-aligned and thus the last
                            // "slab" is actually a partial slab.
                            //
                            TracePrint((TRACE_LEVEL_WARNING,
                                    TRACE_FLAG_IOCTL,
                                    "ClasspDeviceGetLBAStatusWorker (%p): Device returned an LBA count (%u) that is not a multiple of the slab size (%u)\n",
                                    DeviceObject,
                                    lbaCount,
                                    blocksPerSlab));

                            lbaCount = ((lbaCount / blocksPerSlab) + 1) * blocksPerSlab;

                            doneProcessing = TRUE;
                            processCurrentDescriptor = TRUE;
                        }
                        else if (lbaCount == 0)
                        {
                            //
                            // If the LBA count is 0, just skip this descriptor.
                            //
                            TracePrint((TRACE_LEVEL_WARNING,
                                    TRACE_FLAG_IOCTL,
                                    "ClasspDeviceGetLBAStatusWorker (%p): Device returned a zero LBA count\n",
                                    DeviceObject));

                            processCurrentDescriptor = FALSE;
                        }

                        //
                        // Generate bits for the slabs described in the current descriptor.
                        // It's possible the device may have returned more slabs than requested
                        // so we make sure to stop once we've processed all we need.
                        //
                        if (processCurrentDescriptor)
                        {
                            ULONG descrSlabs = lbaCount / blocksPerSlab; // Number of slabs in this descriptor.

                            for(; 0 < descrSlabs && totalProcessedSlabs < requestedSlabs; descrSlabs--, commandProcessedSlabs++, totalProcessedSlabs++)
                            {
                                ULONG bitMapIndex = totalProcessedSlabs / bitMapGranularityInBits;
                                ULONG bitPos = totalProcessedSlabs % bitMapGranularityInBits;

                                #if (NTDDI_VERSION >= NTDDI_WINBLUE)
                                if (OutputVersion == DEVICE_DATA_SET_LB_PROVISIONING_STATE_VERSION_V2) {
                                    ((PDEVICE_DATA_SET_LB_PROVISIONING_STATE_V2)lbpState)->SlabAllocationBitMap[bitMapIndex] |= (mapped << bitPos);
                                } else
                                #endif
                                {
                                    lbpState->SlabAllocationBitMap[bitMapIndex] |= (mapped << bitPos);
                                }
                            }
                        }

                        //
                        // Calculate the next expected starting LBA.
                        //
                        expectedStartingLBA = returnedStartingLBA + lbaCount;
                    }

                    if (commandProcessedSlabs > 0) {

                        //
                        // Calculate the starting LBA we'll use for the next command.
                        //
                        startingLBA += ((ULONGLONG)commandProcessedSlabs * (ULONGLONG)blocksPerSlab);

                    } else {
                        //
                        // This should never happen, but we should handle it gracefully anyway.
                        //
                        TracePrint((TRACE_LEVEL_ERROR,
                                    TRACE_FLAG_IOCTL,
                                    "ClasspDeviceGetLBAStatusWorker (%p): The slab allocation bitmap has zero length.\n",
                                    DeviceObject));

                        NT_ASSERT(commandProcessedSlabs != 0);
                        doneProcessing = TRUE;
                        status = STATUS_UNSUCCESSFUL;
                    }
                } else {
                    TracePrint((TRACE_LEVEL_ERROR,
                                TRACE_FLAG_IOCTL,
                                "ClasspDeviceGetLBAStatusWorker (%p): Device returned no LBA Status Descriptors.\n",
                                DeviceObject));

                    doneProcessing = TRUE;
                    status = STATUS_UNSUCCESSFUL;
                }
            } else {
                TracePrint((TRACE_LEVEL_ERROR,
                            TRACE_FLAG_IOCTL,
                            "ClasspDeviceGetLBAStatusWorker (%p): not enough bytes returned\n",
                            DeviceObject));

                doneProcessing = TRUE;
                status = STATUS_DEVICE_DATA_ERROR;
            }
        }

    //
    // Loop until we encounter some error or we've processed all the requested slabs.
    //
    } while (NT_SUCCESS(status) &&
             !doneProcessing &&
             (totalProcessedSlabs < requestedSlabs));

    //
    // At least one slab was returned by the device and processed, which we
    // consider success.  It's up to the caller to detect truncation.
    // Update the output buffer sizes, offsets, etc. accordingly.
    //
    if (totalProcessedSlabs > 0) {

        #if (NTDDI_VERSION >= NTDDI_WINBLUE)
        if (OutputVersion == DEVICE_DATA_SET_LB_PROVISIONING_STATE_VERSION_V2) {
            PDEVICE_DATA_SET_LB_PROVISIONING_STATE_V2 lbpStateV2 = (PDEVICE_DATA_SET_LB_PROVISIONING_STATE_V2)lbpState;

            lbpStateV2->SlabSizeInBytes = bytesPerSlab;
            lbpStateV2->SlabOffsetDeltaInBytes = startingOffsetDelta;
            lbpStateV2->SlabAllocationBitMapBitCount = totalProcessedSlabs;
            lbpStateV2->SlabAllocationBitMapLength = ((totalProcessedSlabs - 1) / (ULONGLONG)bitMapGranularityInBits) + 1;
            lbpStateV2->Version = DEVICE_DATA_SET_LB_PROVISIONING_STATE_VERSION_V2;

            //
            // Note that there is already one element of the bitmap array allocated
            // in the DEVICE_DATA_SET_LB_PROVISIONING_STATE_V2 structure itself, which
            // is why we subtract 1 from SlabAllocationBitMapLength.
            //
            lbpStateV2->Size = sizeof(DEVICE_DATA_SET_LB_PROVISIONING_STATE_V2)
                               + ((lbpStateV2->SlabAllocationBitMapLength - 1) * sizeof(lbpStateV2->SlabAllocationBitMap[0]));

        } else
        #endif
        {

            lbpState->SlabSizeInBytes = bytesPerSlab;
            lbpState->SlabOffsetDeltaInBytes = (ULONG)startingOffsetDelta;
            lbpState->SlabAllocationBitMapBitCount = totalProcessedSlabs;
            lbpState->SlabAllocationBitMapLength = ((totalProcessedSlabs - 1) / bitMapGranularityInBits) + 1;
            lbpState->Version = DEVICE_DATA_SET_LB_PROVISIONING_STATE_VERSION_V1;

            //
            // Note that there is already one element of the bitmap array allocated
            // in the DEVICE_DATA_SET_LB_PROVISIONING_STATE structure itself, which
            // is why we subtract 1 from SlabAllocationBitMapLength.
            //
            lbpState->Size = sizeof(DEVICE_DATA_SET_LB_PROVISIONING_STATE)
                             + ((lbpState->SlabAllocationBitMapLength - 1) * sizeof(lbpState->SlabAllocationBitMap[0]));
        }

        DsmOutput->OutputBlockLength = lbpState->Size; // Size is at the same offset in all versions of the structure.
        DsmOutput->OutputBlockOffset = sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES_OUTPUT);
        *DsmOutputLength = DsmOutput->Size + DsmOutput->OutputBlockLength;

        status = STATUS_SUCCESS;
    }

    TracePrint((TRACE_LEVEL_INFORMATION,
            TRACE_FLAG_IOCTL,
            "ClasspDeviceGetLBAStatusWorker (%p): Processed a total of %u slabs\n",
            DeviceObject,
            totalProcessedSlabs));
Exit:

    FREE_POOL(lbaStatusListHeader);
    return status;
}

NTSTATUS ClassGetLBProvisioningLogPage(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_ ULONG LogPageSize,
    _Inout_ PLOG_PAGE_LOGICAL_BLOCK_PROVISIONING LogPage
    )
/*
Routine Description:

    This function sends a LOG SENSE command to the given device and returns the
    Logical Block Provisioning Log Page, if available.

Arguments:
    DeviceObject: The FDO of the device to which the Log Sense command will be sent.
    Srb: This should be allocated before it is passed in, but it does not have
        to be initialized.  This function will initialize it.
    LogPageSize: The size of the LogPage buffer in bytes.
    LogPage: A pointer to an already allocated output buffer that may contain
        the LBP log page when this function returns.

Return Value:

    STATUS_INVALID_PARAMETER: May be returned if the LogPage buffer is NULL or
        not large enough.
    STATUS_SUCCESS: The log page was obtained and placed in the LogPage buffer.

    This function may return other NTSTATUS codes from internal function calls.
--*/
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    NTSTATUS status = STATUS_SUCCESS;
    PCDB cdb = NULL;

    //
    // Make sure the caller passed in an adequate output buffer.  The Allocation
    // Length field in the Log Sense command is only 2 bytes so we need to also
    // make sure that the given log page size isn't larger than MAXUSHORT.
    //
    if (LogPage == NULL ||
        LogPageSize < sizeof(LOG_PAGE_LOGICAL_BLOCK_PROVISIONING) ||
        LogPageSize > MAXUSHORT)
    {
        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_GENERAL,
                    "ClassGetLBProvisioningLogPage: DO (%p), Invalid parameter, LogPage = %p, LogPageSize = %u.\n",
                    DeviceObject,
                    LogPage,
                    LogPageSize));

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Initialize the SRB.
    //
    if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        status = InitializeStorageRequestBlock((PSTORAGE_REQUEST_BLOCK)Srb,
                                                STORAGE_ADDRESS_TYPE_BTL8,
                                                CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE,
                                                1,
                                                SrbExDataTypeScsiCdb16);
        if (NT_SUCCESS(status)) {
            ((PSTORAGE_REQUEST_BLOCK)Srb)->SrbFunction = SRB_FUNCTION_EXECUTE_SCSI;
        } else {
            //
            // Should not occur.
            //
            NT_ASSERT(FALSE);
        }
    } else {
        RtlZeroMemory(Srb, sizeof(SCSI_REQUEST_BLOCK));
        Srb->Length = sizeof(SCSI_REQUEST_BLOCK);
        Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    }

    //
    // Build and send down the Log Sense command.
    //
    SrbSetTimeOutValue(Srb, fdoExtension->TimeOutValue);
    SrbSetRequestTag(Srb, SP_UNTAGGED);
    SrbSetRequestAttribute(Srb, SRB_SIMPLE_TAG_REQUEST);
    SrbAssignSrbFlags(Srb, fdoExtension->SrbFlags);
    SrbSetCdbLength(Srb, sizeof(cdb->LOGSENSE));

    cdb = SrbGetCdb(Srb);
    cdb->LOGSENSE.OperationCode = SCSIOP_LOG_SENSE;
    cdb->LOGSENSE.PageCode = LOG_PAGE_CODE_LOGICAL_BLOCK_PROVISIONING;
    cdb->LOGSENSE.PCBit = 0;
    cdb->LOGSENSE.ParameterPointer[0] = 0;
    cdb->LOGSENSE.ParameterPointer[1] = 0;
    REVERSE_BYTES_SHORT(&(cdb->LOGSENSE.AllocationLength), &LogPageSize);

    status = ClassSendSrbSynchronous(fdoExtension->DeviceObject,
                                        Srb,
                                        LogPage,
                                        LogPageSize,
                                        FALSE);

    //
    // Handle the case where we get back STATUS_DATA_OVERRUN b/c the input
    // buffer was larger than necessary.
    //
    if (status == STATUS_DATA_OVERRUN &&
        SrbGetDataTransferLength(Srb) < LogPageSize)
    {
        status = STATUS_SUCCESS;
    }

    //
    // Log the command.
    //
    TracePrint((TRACE_LEVEL_INFORMATION,
                TRACE_FLAG_IOCTL,
                "ClassGetLBProvisioningLogPage: DO (%p), LogSense command issued for LBP log page. NT Status: %!STATUS!.\n",
                DeviceObject,
                status
                ));

    return status;
}

NTSTATUS ClassInterpretLBProvisioningLogPage(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG LogPageSize,
    _In_ PLOG_PAGE_LOGICAL_BLOCK_PROVISIONING LogPage,
    _In_ ULONG ResourcesSize,
    _Out_ PSTORAGE_LB_PROVISIONING_MAP_RESOURCES Resources
    )
/*
Routine Description:

    This function takes a Logical Block Provisioning log page (returned by
    ClassGetLBProvisioningLogPage(), for example), interprets its contents,
    and returns the interpreted data in a STORAGE_LB_PROVISIONING_MAP_RESOURCES
    structure.

    None, some, or all of the data in the output buffer may be valid.  The
    caller must look at the individual "Valid" fields to see which fields have
    valid data.

Arguments:
    DeviceObject: The FDO of the device from which the log page was obtained.
    LogPageSize: The size of the LogPage buffer in bytes.
    LogPage: A pointer to a valid LBP log page structure.
    ResourcesSize: The size of the Resources buffer in bytes.
    Resources: A pointer to an already allocated output buffer that may contain
        the interpreted log page data when this function returns.

Return Value:

    STATUS_NOT_SUPPORTED: May be returned if the threshold exponent from the
        0xB2 page is invalid.
    STATUS_INVALID_PARAMETER: May be returned if either the LogPage or Resources
        buffers are NULL or too small.
    STATUS_SUCCESS: The log page data was interpreted and the Resources output
        buffer has data in it.

    This function may return other NTSTATUS codes from internal function calls.
--*/
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    USHORT pageLength;
    PLOG_PARAMETER_HEADER parameter;
    PVOID endOfPage;
    USHORT parameterCode;
    ULONG resourceCount;
    UCHAR thresholdExponent = fdoExtension->FunctionSupportInfo->LBProvisioningData.ThresholdExponent;
    ULONGLONG thresholdSetSize;

    //
    // SBC-3 states that the threshold exponent (from the 0xB2 VPD page), must
    // be non-zero and less than or equal to 32.
    //
    if (thresholdExponent < 0 || thresholdExponent > 32)
    {
        TracePrint((TRACE_LEVEL_ERROR,
            TRACE_FLAG_GENERAL,
            "ClassInterpretLBProvisioningLogPage: DO (%p), Threshold Exponent (%u) is invalid.\n",
            DeviceObject,
            thresholdExponent));

        return STATUS_NOT_SUPPORTED;
    }

    if (Resources == NULL ||
        ResourcesSize < sizeof(STORAGE_LB_PROVISIONING_MAP_RESOURCES) ||
        LogPage == NULL ||
        LogPageSize < sizeof(LOG_PAGE_LOGICAL_BLOCK_PROVISIONING))
    {
        TracePrint((TRACE_LEVEL_ERROR,
            TRACE_FLAG_GENERAL,
            "ClassInterpretLBProvisioningLogPage: DO (%p), Invalid parameter, Resources = %p, ResourcesSize = %u, LogPage = %p, LogPageSize = %u.\n",
            DeviceObject,
            Resources,
            ResourcesSize,
            LogPage,
            LogPageSize));

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Calculate the threshold set size (in LBAs).
    //
    thresholdSetSize = 1ULL << thresholdExponent;

    REVERSE_BYTES_SHORT(&pageLength, &(LogPage->PageLength));

    //
    // Initialize the output buffer.
    //
    RtlZeroMemory(Resources, sizeof(STORAGE_LB_PROVISIONING_MAP_RESOURCES));
    Resources->Size = sizeof(STORAGE_LB_PROVISIONING_MAP_RESOURCES);
    Resources->Version = sizeof(STORAGE_LB_PROVISIONING_MAP_RESOURCES);

    //
    // Make sure we don't walk off the end of the log page buffer
    // if pageLength is somehow longer than the buffer itself.
    //
    pageLength = (USHORT)min(pageLength, (LogPageSize - FIELD_OFFSET(LOG_PAGE_LOGICAL_BLOCK_PROVISIONING, Parameters)));

    parameter = (PLOG_PARAMETER_HEADER)((PUCHAR)LogPage + FIELD_OFFSET(LOG_PAGE_LOGICAL_BLOCK_PROVISIONING, Parameters));
    endOfPage = (PVOID)((PUCHAR)parameter + pageLength);

    //
    // Walk the parameters.
    //
    while ((PVOID)parameter < endOfPage)
    {
        if (parameter->ParameterLength > 0)
        {
            REVERSE_BYTES_SHORT(&parameterCode, &(parameter->ParameterCode));
            switch(parameterCode)
            {
                case LOG_PAGE_LBP_PARAMETER_CODE_AVAILABLE:
                {
                    REVERSE_BYTES(&resourceCount, &(((PLOG_PARAMETER_THRESHOLD_RESOURCE_COUNT)parameter)->ResourceCount));
                    Resources->AvailableMappingResources = (ULONGLONG)resourceCount * thresholdSetSize * (ULONGLONG)fdoExtension->DiskGeometry.BytesPerSector;
                    Resources->AvailableMappingResourcesValid = TRUE;

                    //
                    // Devices that implement SBC-3 revisions older than r27 will not specify
                    // an LBP log page parameter that has fields beyond ResourceCount.
                    //
                    if (parameter->ParameterLength > FIELD_OFFSET(LOG_PARAMETER_THRESHOLD_RESOURCE_COUNT, ResourceCount[3])) {
                        Resources->AvailableMappingResourcesScope = ((PLOG_PARAMETER_THRESHOLD_RESOURCE_COUNT)parameter)->Scope;
                    }

                    break;
                }

                case LOG_PAGE_LBP_PARAMETER_CODE_USED:
                {
                    REVERSE_BYTES(&resourceCount, &(((PLOG_PARAMETER_THRESHOLD_RESOURCE_COUNT)parameter)->ResourceCount));
                    Resources->UsedMappingResources = (ULONGLONG)resourceCount * thresholdSetSize * (ULONGLONG)fdoExtension->DiskGeometry.BytesPerSector;
                    Resources->UsedMappingResourcesValid = TRUE;

                    //
                    // Devices that implement SBC-3 revisions older than r27 will not specify
                    // an LBP log page parameter that has fields beyond ResourceCount.
                    //
                    if (parameter->ParameterLength > FIELD_OFFSET(LOG_PARAMETER_THRESHOLD_RESOURCE_COUNT, ResourceCount[3])) {
                        Resources->UsedMappingResourcesScope = ((PLOG_PARAMETER_THRESHOLD_RESOURCE_COUNT)parameter)->Scope;
                    }

                    break;
                }
            }
        }

        //
        // Move to the next parameter.
        //
        parameter = (PLOG_PARAMETER_HEADER)((PUCHAR)parameter + sizeof(LOG_PARAMETER_HEADER) + parameter->ParameterLength);
    }

    return STATUS_SUCCESS;
}

NTSTATUS ClassGetLBProvisioningResources(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PSCSI_REQUEST_BLOCK Srb,
    _In_ ULONG ResourcesSize,
    _Inout_ PSTORAGE_LB_PROVISIONING_MAP_RESOURCES Resources
    )
/*
Routine Description:

    This function obtains the Logical Block Provisioning log page, interprets
    its contents, and returns the interpreted data in a
    STORAGE_LB_PROVISIONING_MAP_RESOURCES structure.

    None, some, or all of the data in the output buffer may be valid.  The
    caller must look at the individual "Valid" fields to see which fields have
    valid data.

Arguments:
    DeviceObject: The target FDO.
    Srb: This should be allocated before it is passed in, but it does not have
        to be initialized.
    ResourcesSize: The size of the Resources buffer in bytes.
    Resources: A pointer to an already allocated output buffer that may contain
        the interpreted log page data when this function returns.

Return Value:

    STATUS_NOT_SUPPORTED: May be returned if the device does not have LBP enabled.
    STATUS_INVALID_PARAMETER: May be returned if either the Resources buffer is
        NULL or too small.
    STATUS_INSUFFICIENT_RESOURCES: May be returned if a log page buffer could not
        be allocated.
    STATUS_SUCCESS: The log page data was obtained and the Resources output
        buffer has data in it.

    This function may return other NTSTATUS codes from internal function calls.
--*/
{
    NTSTATUS status;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ULONG logPageSize;
    PLOG_PAGE_LOGICAL_BLOCK_PROVISIONING logPage = NULL;

    //
    // This functionality is only supported for devices that support logical
    // block provisioning.
    //
    if (fdoExtension->FunctionSupportInfo->ValidInquiryPages.LBProvisioning == FALSE)
    {
        TracePrint((TRACE_LEVEL_ERROR,
            TRACE_FLAG_GENERAL,
            "ClassGetLBProvisioningResources: DO (%p), Device does not support logical block provisioning.\n",
            DeviceObject));

        return STATUS_NOT_SUPPORTED;
    }

    //
    // Validate the output buffer.
    //
    if (Resources == NULL ||
        ResourcesSize < sizeof(STORAGE_LB_PROVISIONING_MAP_RESOURCES))
    {
        TracePrint((TRACE_LEVEL_ERROR,
            TRACE_FLAG_GENERAL,
            "ClassGetLBProvisioningResources: DO (%p), Invalid parameter, Resources = %p, ResourcesSize = %u.\n",
            DeviceObject,
            Resources,
            ResourcesSize));

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Allocate a buffer for the log page.  Currently the log page contains:
    // 1. Log page header
    // 2. Log page parameter for used resources
    // 3. Log page parameter for available resources
    //
    logPageSize = sizeof(LOG_PAGE_LOGICAL_BLOCK_PROVISIONING) + (2 * sizeof(LOG_PARAMETER_THRESHOLD_RESOURCE_COUNT));

#if defined(_ARM_) || defined(_ARM64_)
    //
    // ARM has specific alignment requirements, although this will not have a functional impact on x86 or amd64
    // based platforms. We are taking the conservative approach here.
    //
    logPageSize = ALIGN_UP_BY(logPageSize, KeGetRecommendedSharedDataAlignment());
    logPage = (PLOG_PAGE_LOGICAL_BLOCK_PROVISIONING)ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned, logPageSize,  CLASS_TAG_LB_PROVISIONING);
#else
    logPage = (PLOG_PAGE_LOGICAL_BLOCK_PROVISIONING)ExAllocatePoolWithTag(NonPagedPoolNx, logPageSize,  CLASS_TAG_LB_PROVISIONING);
#endif
    if (logPage != NULL)
    {
        //
        // Get the LBP log page from the device.
        //
        status = ClassGetLBProvisioningLogPage(DeviceObject,
                                               Srb,
                                               logPageSize,
                                               logPage);

        if (NT_SUCCESS(status))
        {
            //
            // Interpret the log page and fill in the output buffer.
            //
            status = ClassInterpretLBProvisioningLogPage(DeviceObject,
                                                         logPageSize,
                                                         logPage,
                                                         ResourcesSize,
                                                         Resources);
        }

        ExFreePool(logPage);
    }
    else
    {
        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_GENERAL,
                    "ClassGetLBProvisioningResources: DO (%p), Failed to allocate memory for LBP log page.\n",
                    DeviceObject));

        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}

NTSTATUS
ClassDeviceGetLBProvisioningResources(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    )
/*
Routine Description:

    This function returns the LBP resource counts in a
    STORAGE_LB_PROVISIONING_MAP_RESOURCES structure in the IRP.

    None, some, or all of the data in the output buffer may be valid.  The
    caller must look at the individual "Valid" fields to see which fields have
    valid data.

Arguments:
    DeviceObject: The target FDO.
    Irp: The IRP which will contain the output buffer upon completion.
    Srb: This should be allocated before it is passed in, but it does not have
        to be initialized.

Return Value:

    Some NTSTATUS code.

--*/
{
    NTSTATUS status;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSTORAGE_LB_PROVISIONING_MAP_RESOURCES mapResources = (PSTORAGE_LB_PROVISIONING_MAP_RESOURCES)Irp->AssociatedIrp.SystemBuffer;

    status = ClassGetLBProvisioningResources(DeviceObject,
                                           Srb,
                                           irpStack->Parameters.DeviceIoControl.OutputBufferLength,
                                           mapResources);

    if (NT_SUCCESS(status)) {
        Irp->IoStatus.Information = mapResources->Size;
    } else {
        Irp->IoStatus.Information = 0;
    }

    Irp->IoStatus.Status = status;
    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

    return status;
}

_Function_class_(IO_WORKITEM_ROUTINE)
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassLogThresholdEvent(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context
    )
/*
    Routine Description:

    This function logs a logical block provisioning soft threshold event to the
    system event log.

Arguments:
    DeviceObject: The FDO that represents the device that reported the soft
        threshold.
    Context: A pointer to the IO_WORKITEM in which this function is running.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_WORKITEM workItem = (PIO_WORKITEM)Context;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PSCSI_REQUEST_BLOCK srb = NULL;
    STORAGE_LB_PROVISIONING_MAP_RESOURCES resources = {0};
    ULONG resourcesSize = sizeof(STORAGE_LB_PROVISIONING_MAP_RESOURCES);
    PIO_ERROR_LOG_PACKET errorLogEntry = NULL;
    ULONG logEntrySize = sizeof(IO_ERROR_LOG_PACKET);
    PWCHAR stringIndex = NULL;
    LONG stringSize = 0;
    ULONG srbSize;

    //
    // Allocate an SRB for getting the LBP log page.
    //
    if ((fdoExtension->AdapterDescriptor != NULL) &&
        (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK)) {
        srbSize = CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE;
    } else {
        srbSize = sizeof(SCSI_REQUEST_BLOCK);
    }

    srb = ExAllocatePoolWithTag(NonPagedPoolNx,
                                srbSize,
                                'ACcS');
    if (srb != NULL) {

        //
        // Try to get the LBP resources from the device so we can report them in
        // the system event log.
        //
        ClassGetLBProvisioningResources(DeviceObject,
                                        srb,
                                        resourcesSize,
                                        &resources);

        //
        // We need to allocate enough space for 3 insertion strings:
        // The first is a ULONG representing the disk number in decimal, which means
        // a max of 10 digits, plus one for the NULL character.
        // The second and third are ULONGLONGs representing the used and available
        // bytes, which means a max of 20 digits, plus one for the NULL character.
        // Make sure we do not exceed the max error log size or the max size of a
        // UCHAR since the size gets truncated to a UCHAR when we pass it to
        // IoAllocateErrorLogEntry().
        //
        logEntrySize = sizeof(IO_ERROR_LOG_PACKET) + (11 * sizeof(WCHAR)) + (2 * (21 * sizeof(WCHAR)));
        logEntrySize = min(logEntrySize, ERROR_LOG_MAXIMUM_SIZE);
        logEntrySize = min(logEntrySize, MAXUCHAR);

        errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(DeviceObject, (UCHAR)logEntrySize);
        if (errorLogEntry != NULL)
        {
            //
            // There are two event IDs we can use here.  Both use the disk number,
            // but one reports the available and used bytes while the other does not.
            // We fall back on the latter if we failed to obtain the available and
            // used byte counts from the LBP log page.
            //
            // The event insertion strings need to be in this order:
            // 1. The disk number. (Both event IDs use this.)
            // 2. Bytes used.
            // 3. Bytes available.
            //

            RtlZeroMemory(errorLogEntry, logEntrySize);
            errorLogEntry->StringOffset = sizeof(IO_ERROR_LOG_PACKET);

            stringIndex = (PWCHAR)((ULONG_PTR)errorLogEntry + sizeof(IO_ERROR_LOG_PACKET));
            stringSize = logEntrySize - sizeof(IO_ERROR_LOG_PACKET);

            //
            // Add the disk number to the insertion strings.
            //
            status = RtlStringCbPrintfW(stringIndex, stringSize, L"%d", fdoExtension->DeviceNumber);

            if (NT_SUCCESS(status) )
            {
                errorLogEntry->NumberOfStrings++;

                if (resources.UsedMappingResourcesValid &&
                    resources.AvailableMappingResourcesValid)
                {
                    //
                    // Add the used mapping resources to the insertion strings.
                    //
                    stringIndex += (wcslen(stringIndex) + 1);
                    stringSize -= (LONG)(wcslen(stringIndex) + 1) * sizeof(WCHAR);

                    status = RtlStringCbPrintfW(stringIndex, stringSize, L"%I64u", resources.UsedMappingResources);

                    if (NT_SUCCESS(status))
                    {
                        errorLogEntry->NumberOfStrings++;

                        //
                        // Add the available mapping resources to the insertion strings.
                        //
                        stringIndex += (wcslen(stringIndex) + 1);
                        stringSize -= (LONG)(wcslen(stringIndex) + 1) * sizeof(WCHAR);

                        status = RtlStringCbPrintfW(stringIndex, stringSize, L"%I64u", resources.AvailableMappingResources);

                        if (NT_SUCCESS(status))
                        {
                            errorLogEntry->NumberOfStrings++;
                        }
                    }
                }
                else
                {
                    TracePrint((TRACE_LEVEL_WARNING,
                                TRACE_FLAG_GENERAL,
                                "ClassLogThresholdEvent: DO (%p), Used and available mapping resources were unavailable.\n",
                                DeviceObject));
                }
            }

            //
            // If we were able to successfully assemble all 3 insertion strings,
            // then we can use one of the "extended" event IDs.  Otherwise, use the basic
            // event ID, which only requires the disk number.
            //
            if (errorLogEntry->NumberOfStrings == 3)
            {
                if (resources.UsedMappingResourcesScope == LOG_PAGE_LBP_RESOURCE_SCOPE_DEDICATED_TO_LUN &&
                    resources.AvailableMappingResourcesScope == LOG_PAGE_LBP_RESOURCE_SCOPE_DEDICATED_TO_LUN) {

                    errorLogEntry->ErrorCode = IO_WARNING_SOFT_THRESHOLD_REACHED_EX_LUN_LUN;

                } else if (resources.UsedMappingResourcesScope == LOG_PAGE_LBP_RESOURCE_SCOPE_DEDICATED_TO_LUN &&
                           resources.AvailableMappingResourcesScope == LOG_PAGE_LBP_RESOURCE_SCOPE_NOT_DEDICATED_TO_LUN) {

                    errorLogEntry->ErrorCode = IO_WARNING_SOFT_THRESHOLD_REACHED_EX_LUN_POOL;

                } else if (resources.UsedMappingResourcesScope == LOG_PAGE_LBP_RESOURCE_SCOPE_NOT_DEDICATED_TO_LUN &&
                           resources.AvailableMappingResourcesScope == LOG_PAGE_LBP_RESOURCE_SCOPE_DEDICATED_TO_LUN) {

                    errorLogEntry->ErrorCode = IO_WARNING_SOFT_THRESHOLD_REACHED_EX_POOL_LUN;

                } else if (resources.UsedMappingResourcesScope == LOG_PAGE_LBP_RESOURCE_SCOPE_NOT_DEDICATED_TO_LUN &&
                           resources.AvailableMappingResourcesScope == LOG_PAGE_LBP_RESOURCE_SCOPE_NOT_DEDICATED_TO_LUN) {

                    errorLogEntry->ErrorCode = IO_WARNING_SOFT_THRESHOLD_REACHED_EX_POOL_POOL;

                } else {

                    errorLogEntry->ErrorCode = IO_WARNING_SOFT_THRESHOLD_REACHED_EX;
                }
            }
            else
            {
                errorLogEntry->ErrorCode = IO_WARNING_SOFT_THRESHOLD_REACHED;
            }

            //
            // Write the error log packet to the system error logging thread.
            // It will be freed automatically.
            //
            IoWriteErrorLogEntry(errorLogEntry);

            TracePrint((TRACE_LEVEL_INFORMATION,
                        TRACE_FLAG_GENERAL,
                        "ClassLogThresholdEvent: DO (%p), Soft threshold notification logged.\n",
                        DeviceObject));
        }
        else
        {
            TracePrint((TRACE_LEVEL_ERROR,
                        TRACE_FLAG_GENERAL,
                        "ClassLogThresholdEvent: DO (%p), Failed to allocate memory for error log entry.\n",
                        DeviceObject));
        }
    } else {
        TracePrint((TRACE_LEVEL_ERROR,
                        TRACE_FLAG_GENERAL,
                        "ClassLogThresholdEvent: DO (%p), Failed to allocate memory for SRB.\n",
                        DeviceObject));
    }


    //
    // Clear the soft threshold event pending flag so that another can be queued.
    //
    InterlockedExchange((PLONG)&(fdoExtension->FunctionSupportInfo->LBProvisioningData.SoftThresholdEventPending), 0);

    ClassReleaseRemoveLock(DeviceObject, (PIRP)workItem);

    FREE_POOL(srb);

    if (workItem != NULL) {
        IoFreeWorkItem(workItem);
    }
}

NTSTATUS
ClasspLogSystemEventWithDeviceNumber(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ NTSTATUS IoErrorCode
    )
/*
    Routine Description:

    This function is a helper routine to log any system events that require
    the DeviceNumber (e.g. disk number). It is basically a wrapper for the
    IoWriteErrorLogEntry call.

Arguments:
    DeviceObject: The FDO that represents the device for which the event needs to be logged.
    IoErrorCode: The IO error code for the event.

Return Value:
    STATUS_SUCCESS - if the event was logged
    STATUS_INSUFFICIENT_RESOURCES - otherwise

--*/
{
    NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PIO_ERROR_LOG_PACKET errorLogEntry = NULL;
    ULONG logEntrySize = sizeof(IO_ERROR_LOG_PACKET);
    PWCHAR stringIndex = NULL;
    LONG stringSize = 0;

    //
    // We need to allocate enough space for one insertion string: a ULONG
    // representing the disk number in decimal, which means a max of 10 digits,
    // plus one for the NULL character.
    // Make sure we do not exceed the max error log size or the max size of a
    // UCHAR since the size gets truncated to a UCHAR when we pass it to
    // IoAllocateErrorLogEntry().
    //
    logEntrySize = sizeof(IO_ERROR_LOG_PACKET) + (11 * sizeof(WCHAR));
    logEntrySize = min(logEntrySize, ERROR_LOG_MAXIMUM_SIZE);
    logEntrySize = min(logEntrySize, MAXUCHAR);

    errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(DeviceObject, (UCHAR)logEntrySize);
    if (errorLogEntry) {

        RtlZeroMemory(errorLogEntry, logEntrySize);
        errorLogEntry->StringOffset = sizeof(IO_ERROR_LOG_PACKET);
        errorLogEntry->ErrorCode = IoErrorCode;

        stringIndex = (PWCHAR)((ULONG_PTR)errorLogEntry + sizeof(IO_ERROR_LOG_PACKET));
        stringSize = logEntrySize - sizeof(IO_ERROR_LOG_PACKET);

        //
        // Add the disk number to the insertion strings.
        //
        status = RtlStringCbPrintfW(stringIndex, stringSize, L"%d", fdoExtension->DeviceNumber);

        if (NT_SUCCESS(status)) {
            errorLogEntry->NumberOfStrings++;
        }

        //
        // Write the error log packet to the system error logging thread.
        // It will be freed automatically.
        //
        IoWriteErrorLogEntry(errorLogEntry);

        status = STATUS_SUCCESS;
    }

    return status;
}

_Function_class_(IO_WORKITEM_ROUTINE)
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassLogResourceExhaustionEvent(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context
    )
/*
    Routine Description:

    This function logs a logical block provisioning permanent resource exhaustion
    event to the system event log.

Arguments:
    DeviceObject: The FDO that represents the device that reported the permanent
        resource exhaustion.
    Context: A pointer to the IO_WORKITEM in which this function is running.

--*/
{
    PIO_WORKITEM workItem = (PIO_WORKITEM)Context;

    if (NT_SUCCESS(ClasspLogSystemEventWithDeviceNumber(DeviceObject, IO_ERROR_DISK_RESOURCES_EXHAUSTED))) {

        TracePrint((TRACE_LEVEL_INFORMATION,
                    TRACE_FLAG_GENERAL,
                    "ClassLogResourceExhaustionEvent: DO (%p), Permanent resource exhaustion logged.\n",
                    DeviceObject));
    } else {
        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_GENERAL,
                    "ClassLogResourceExhaustionEvent: DO (%p), Failed to allocate memory for error log entry.\n",
                    DeviceObject));
    }


    ClassReleaseRemoveLock(DeviceObject, (PIRP)workItem);

    if (workItem != NULL) {
        IoFreeWorkItem(workItem);
    }
}


VOID ClassQueueThresholdEventWorker(
    _In_ PDEVICE_OBJECT DeviceObject
    )
/*
Routine Description:

    This function queues a delayed work item that will eventually log a
    logical block provisioning soft threshold event to the system event log.

Arguments:
    DeviceObject: The FDO that represents the device that reported the soft
        threshold.

--*/
{
    PCOMMON_DEVICE_EXTENSION commonExtension = (PCOMMON_DEVICE_EXTENSION)(DeviceObject->DeviceExtension);
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)(DeviceObject->DeviceExtension);
    PIO_WORKITEM workItem = NULL;

    if (commonExtension->IsFdo &&
        InterlockedCompareExchange((PLONG)&(fdoExtension->FunctionSupportInfo->LBProvisioningData.SoftThresholdEventPending), 1, 0) == 0)
    {
        workItem = IoAllocateWorkItem(DeviceObject);

        if (workItem)
        {

            TracePrint((TRACE_LEVEL_INFORMATION,
                        TRACE_FLAG_GENERAL,
                        "ClassQueueThresholdEventWorker: DO (%p), Queueing soft threshold notification work item.\n",
                        DeviceObject));


            ClassAcquireRemoveLock(DeviceObject, (PIRP)(workItem));

            //
            // Queue a work item to write the threshold notification to the
            // system event log.
            //
            IoQueueWorkItem(workItem, ClassLogThresholdEvent, DelayedWorkQueue, workItem);
        }
        else
        {
            //
            // Clear the soft threshold event pending flag since this is normally
            // done when the work item completes.
            //
            InterlockedExchange((PLONG)&(fdoExtension->FunctionSupportInfo->LBProvisioningData.SoftThresholdEventPending), 0);

            TracePrint((TRACE_LEVEL_ERROR,
                        TRACE_FLAG_GENERAL,
                        "ClassQueueThresholdEventWorker: DO (%p), Failed to allocate memory for the work item.\n",
                        DeviceObject));
        }
    }
}

VOID ClassQueueResourceExhaustionEventWorker(
    _In_ PDEVICE_OBJECT DeviceObject
    )
/*
Routine Description:

    This function queues a delayed work item that will eventually log a
    logical block provisioning permanent resource exhaustion event to the
    system event log.

Arguments:
    DeviceObject: The FDO that represents the device that reported the resource
        exhaustion.

--*/
{
    PCOMMON_DEVICE_EXTENSION commonExtension = (PCOMMON_DEVICE_EXTENSION)(DeviceObject->DeviceExtension);
    PIO_WORKITEM workItem = NULL;

    if (commonExtension->IsFdo)
    {
        workItem = IoAllocateWorkItem(DeviceObject);

        if (workItem)
        {

            TracePrint((TRACE_LEVEL_INFORMATION,
                        TRACE_FLAG_GENERAL,
                        "ClassQueueResourceExhaustionEventWorker: DO (%p), Queueing permanent resource exhaustion event work item.\n",
                        DeviceObject));

            ClassAcquireRemoveLock(DeviceObject, (PIRP)(workItem));

            //
            // Queue a work item to write the threshold notification to the
            // system event log.
            //
            IoQueueWorkItem(workItem, ClassLogResourceExhaustionEvent, DelayedWorkQueue, workItem);
        }
        else
        {
            TracePrint((TRACE_LEVEL_ERROR,
                        TRACE_FLAG_GENERAL,
                        "ClassQueueResourceExhaustionEventWorker: DO (%p), Failed to allocate memory for the work item.\n",
                        DeviceObject));
        }
    }
}

_Function_class_(IO_WORKITEM_ROUTINE)
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassLogCapacityChangedProcess(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context
    )
/*
    Routine Description:

    This function logs a capacity changed event to the system event log.

Arguments:
    DeviceObject: The FDO that represents the device that reported the capacity change.
    Context: A pointer to the IO_WORKITEM in which this function is running.

--*/
{
    NTSTATUS     status;
    PIO_WORKITEM workItem = (PIO_WORKITEM)Context;

    status = ClasspLogSystemEventWithDeviceNumber(DeviceObject, IO_WARNING_DISK_CAPACITY_CHANGED);

    if (NT_SUCCESS(status)) {

        TracePrint((TRACE_LEVEL_INFORMATION,
                    TRACE_FLAG_GENERAL,
                    "ClassLogCapacityChangedEvent: DO (%p), Capacity changed logged.\n",
                    DeviceObject));
    } else {
        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_GENERAL,
                    "ClassLogCapacityChangedEvent: DO (%p), Failed to allocate memory for error log entry.\n",
                    DeviceObject));
    }

    //
    // Get disk capacity and notify upper layer if capacity is changed.
    //
    status = ClassReadDriveCapacity(DeviceObject);

    if (!NT_SUCCESS(status)) {
        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_GENERAL,
                    "ClassLogCapacityChangedEvent: DO (%p), ClassReadDriveCapacity returned %!STATUS!.\n",
                    DeviceObject,
                    status));
    }

    ClassReleaseRemoveLock(DeviceObject, (PIRP)workItem);

    if (workItem != NULL) {
        IoFreeWorkItem(workItem);
    }
}


VOID
ClassQueueCapacityChangedEventWorker(
    _In_ PDEVICE_OBJECT DeviceObject
    )
/*
Routine Description:

    This function queues a delayed work item that will eventually log a
    disk capacity changed event to the system event log.

Arguments:
    DeviceObject: The FDO that represents the device that reported the capacity change.

--*/
{
    PCOMMON_DEVICE_EXTENSION commonExtension = (PCOMMON_DEVICE_EXTENSION)(DeviceObject->DeviceExtension);
    PIO_WORKITEM workItem = NULL;

    if (commonExtension->IsFdo)
    {
        workItem = IoAllocateWorkItem(DeviceObject);

        if (workItem)
        {

            TracePrint((TRACE_LEVEL_INFORMATION,
                        TRACE_FLAG_GENERAL,
                        "ClassQueueCapacityChangedEventWorker: DO (%p), Queueing capacity changed event work item.\n",
                        DeviceObject));

            ClassAcquireRemoveLock(DeviceObject, (PIRP)(workItem));

            //
            // Queue a work item to write the threshold notification to the
            // system event log.
            //
            IoQueueWorkItem(workItem, ClassLogCapacityChangedProcess, DelayedWorkQueue, workItem);
        }
        else
        {
            TracePrint((TRACE_LEVEL_ERROR,
                        TRACE_FLAG_GENERAL,
                        "ClassQueueCapacityChangedEventWorker: DO (%p), Failed to allocate memory for the work item.\n",
                        DeviceObject));
        }
    }
}

_Function_class_(IO_WORKITEM_ROUTINE)
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassLogProvisioningTypeChangedEvent(
    PDEVICE_OBJECT DeviceObject,
    PVOID Context
    )
/*
    Routine Description:

    This function logs a provisioning type changed event to the system event log.

Arguments:
    DeviceObject: The FDO that represents the device that reported the provisioning type change.
    Context: A pointer to the IO_WORKITEM in which this function is running.

--*/
{
    PIO_WORKITEM workItem = (PIO_WORKITEM)Context;

    if (NT_SUCCESS(ClasspLogSystemEventWithDeviceNumber(DeviceObject, IO_WARNING_DISK_PROVISIONING_TYPE_CHANGED))) {

        TracePrint((TRACE_LEVEL_INFORMATION,
                    TRACE_FLAG_GENERAL,
                    "ClassLogProvisioningTypeChangedEvent: DO (%p), LB Provisioning Type changed logged.\n",
                    DeviceObject));
    } else {
        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_GENERAL,
                    "ClassLogProvisioningTypeChangedEvent: DO (%p), Failed to allocate memory for error log entry.\n",
                    DeviceObject));
    }

    ClassReleaseRemoveLock(DeviceObject, (PIRP)workItem);

    IoFreeWorkItem(workItem);
}


VOID
ClassQueueProvisioningTypeChangedEventWorker(
    _In_ PDEVICE_OBJECT DeviceObject
    )
/*
Routine Description:

    This function queues a delayed work item that will eventually log a
    provisioning type changed event to the system event log.

Arguments:
    DeviceObject: The FDO that represents the device that reported the provisioning type change.

--*/
{
    PCOMMON_DEVICE_EXTENSION commonExtension = (PCOMMON_DEVICE_EXTENSION)(DeviceObject->DeviceExtension);
    PIO_WORKITEM workItem = NULL;

    if (commonExtension->IsFdo)
    {
        workItem = IoAllocateWorkItem(DeviceObject);

        if (workItem)
        {

            TracePrint((TRACE_LEVEL_INFORMATION,
                        TRACE_FLAG_GENERAL,
                        "ClassQueueProvisioningTypeChangedEventWorker: DO (%p), Queueing LB provisioning type changed event work item.\n",
                        DeviceObject));

            ClassAcquireRemoveLock(DeviceObject, (PIRP)(workItem));

            //
            // Queue a work item to write the threshold notification to the
            // system event log.
            //
            IoQueueWorkItem(workItem, ClassLogProvisioningTypeChangedEvent, DelayedWorkQueue, workItem);
        }
        else
        {
            TracePrint((TRACE_LEVEL_ERROR,
                        TRACE_FLAG_GENERAL,
                        "ClassQueueProvisioningTypeChangedEventWorker: DO (%p), Failed to allocate memory for the work item.\n",
                        DeviceObject));
        }
    }
}

_Function_class_(IO_WORKITEM_ROUTINE)
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClasspLogIOEventWithContext(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context
    )
/*
    Routine Description:

    This function logs an event to the system event log with dumpdata containing opcode and
    sense data.

Arguments:
    DeviceObject: The FDO that represents the device that retried the IO.
    Context: A pointer to the OPCODE_SENSE_DATA_IO_LOG_MESSAGE_CONTEXT that has data to be logged as part of the message.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    POPCODE_SENSE_DATA_IO_LOG_MESSAGE_CONTEXT_HEADER ioLogMessageContextHeader = (POPCODE_SENSE_DATA_IO_LOG_MESSAGE_CONTEXT_HEADER)Context;
    PIO_WORKITEM workItem;
    PIO_ERROR_LOG_PACKET errorLogEntry = NULL;
    ULONG logEntrySize;
    PWCHAR stringIndex = NULL;
    LONG stringSize = 0;
    ULONG senseBufferSize;
    ULONG stringsBufferLength = 0;
    ULONG pdoNameLength = 0;

    NT_ASSERT(ioLogMessageContextHeader != NULL);
    _Analysis_assume_(ioLogMessageContextHeader != NULL);

    switch (ioLogMessageContextHeader->ErrorCode) {

        case IO_ERROR_IO_HARDWARE_ERROR:
        case IO_WARNING_IO_OPERATION_RETRIED: {

            //
            // We need to allocate enough space for 3 insertion strings:
            // 1. A ULONGLONG in Hex representing the LBA which means a max of 16 digits,
            // plus two for "0x" plus one for the NULL character.
            // 2. A ULONG representing the disk number in decimal, which means
            // a max of 10 digits, plus one for the NULL character.
            // 3. The PDO name, so that if the disk number is hidden from the
            // user for some reason, there is still a way to associate the
            // event with the correct device.
            //
            stringsBufferLength = (19 + 11) * sizeof(WCHAR);

            //
            // Query for the size of the PDO name.
            //
            status = IoGetDeviceProperty(fdoExtension->LowerPdo,
                                         DevicePropertyPhysicalDeviceObjectName,
                                         0,
                                         NULL,
                                         &pdoNameLength);

            if (status == STATUS_BUFFER_TOO_SMALL && pdoNameLength > 0) {
                stringsBufferLength += pdoNameLength;
            } else {
                pdoNameLength = 0;
            }

            break;
        }

    }

    workItem = ioLogMessageContextHeader->WorkItem;

    //
    // DumpData[0] which is of ULONG size and will contain opcode|srbstatus|scsistatus.
    // Then we will have sensebuffer, hence
    // DumpDataSize = senseBufferSize + sizeof(ULONG)
    // and DumpDataSize must be multiple of sizeof(ULONG)
    // which means senseBufferSize needs to ULONG aligned
    // Please note we will use original buffersize for padding later
    //
    senseBufferSize = ALIGN_UP_BY(ioLogMessageContextHeader->SenseDataSize, sizeof(ULONG));

    logEntrySize = FIELD_OFFSET( IO_ERROR_LOG_PACKET, DumpData ) + sizeof(ULONG) + senseBufferSize;

    //
    // We need to make sure the string offset is WCHAR-aligned (the insertion strings
    // come after the sense buffer in the dump data, if any).
    // But we don't need to do anything special for it,
    // since FIELD_OFFSET( IO_ERROR_LOG_PACKET, DumpData) is currently ULONG aligned
    // and SenseBufferSize is also ULONG aligned. This means buffer that precedes the insertion string is ULONG aligned
    // note stringoffset = FIELD_OFFSET( IO_ERROR_LOG_PACKET, DumpData ) + DumpDataSize
    // This leads us to fact that stringoffset will always be ULONG aligned and effectively WCHAR aligned
    //

    //
    // We need to allocate enough space for the insertion strings provided in the passed in Context
    // as well as the opcode and the sense data, while making sure we cap at max error log size.
    // The log packet is followed by the opcode, then the sense data, and then the
    // insertion strings.
    //
    logEntrySize = logEntrySize + stringsBufferLength;

    if (logEntrySize > ERROR_LOG_MAXIMUM_SIZE) {
        if (senseBufferSize) {
            if (logEntrySize - ERROR_LOG_MAXIMUM_SIZE < senseBufferSize) {
                //
                // In below steps, senseBufferSize will become same or less than as ioLogMessageContextHeader->SenseDataSize
                // it can't be more than that.
                //
                senseBufferSize -= logEntrySize - ERROR_LOG_MAXIMUM_SIZE;

                //
                // Decrease the sensebuffersize further, if needed, to keep senseBufferSize ULONG aligned
                //
                senseBufferSize = ALIGN_DOWN_BY(senseBufferSize, sizeof(ULONG));

            } else {
                senseBufferSize = 0;
            }
        }
        logEntrySize = ERROR_LOG_MAXIMUM_SIZE;
    }

    errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(DeviceObject, (UCHAR)logEntrySize);

    if (errorLogEntry) {

        RtlZeroMemory(errorLogEntry, logEntrySize);
        errorLogEntry->MajorFunctionCode = IRP_MJ_SCSI;
        errorLogEntry->RetryCount = 1;
        errorLogEntry->DumpDataSize = (USHORT)(sizeof(ULONG) + senseBufferSize);
        errorLogEntry->StringOffset = (USHORT)(FIELD_OFFSET( IO_ERROR_LOG_PACKET, DumpData ) + errorLogEntry->DumpDataSize);
        errorLogEntry->ErrorCode = ioLogMessageContextHeader->ErrorCode;
        errorLogEntry->DumpData[0] = (((ULONG)(ioLogMessageContextHeader->OpCode)) << 24) |
                                     (((ULONG)(ioLogMessageContextHeader->SrbStatus)) << 16) |
                                     (((ULONG)(ioLogMessageContextHeader->ScsiStatus)) << 8);

        //
        // Copy sense data and do padding for sense data if needed, with '-'
        //
        if (senseBufferSize  > ioLogMessageContextHeader->SenseDataSize) {
            RtlCopyMemory(&errorLogEntry->DumpData[1], ioLogMessageContextHeader->SenseData, ioLogMessageContextHeader->SenseDataSize);
            RtlFillMemory( (PCHAR)&errorLogEntry->DumpData[1] + ioLogMessageContextHeader->SenseDataSize , (senseBufferSize - ioLogMessageContextHeader->SenseDataSize) , '-' );
        } else  {
            RtlCopyMemory(&errorLogEntry->DumpData[1], ioLogMessageContextHeader->SenseData, senseBufferSize);
        }

        stringIndex = (PWCHAR)((PCHAR)errorLogEntry->DumpData + errorLogEntry->DumpDataSize);
        stringSize = logEntrySize - errorLogEntry->StringOffset;

        //
        // Add the strings
        //
        switch (ioLogMessageContextHeader->ErrorCode) {
            case IO_ERROR_IO_HARDWARE_ERROR:
            case IO_WARNING_IO_OPERATION_RETRIED: {

                PIO_RETRIED_LOG_MESSAGE_CONTEXT ioLogMessageContext = (PIO_RETRIED_LOG_MESSAGE_CONTEXT)Context;

                //
                // The first is a "0x" plus ULONGLONG in hex representing the LBA plus the NULL character.
                // The second is a ULONG representing the disk number plus the NULL character.
                //
                status = RtlStringCbPrintfW(stringIndex, stringSize, L"0x%I64x", ioLogMessageContext->Lba.QuadPart);
                if (NT_SUCCESS(status)) {
                    errorLogEntry->NumberOfStrings++;

                    //
                    // Add the disk number to the insertion strings.
                    //
                    stringSize -= (ULONG)(wcslen(stringIndex) + 1) * sizeof(WCHAR);
                    stringIndex += (wcslen(stringIndex) + 1);

                    if (stringSize > 0) {

                        status = RtlStringCbPrintfW(stringIndex, stringSize, L"%d", ioLogMessageContext->DeviceNumber);

                        if (NT_SUCCESS(status)) {

                            errorLogEntry->NumberOfStrings++;

                            stringSize -= (ULONG)(wcslen(stringIndex) + 1) * sizeof(WCHAR);
                            stringIndex += (wcslen(stringIndex) + 1);

                            if (stringSize >= (LONG)pdoNameLength && pdoNameLength > 0) {
                                ULONG resultLength;

                                //
                                // Get the PDO name and place it in the insertion string buffer.
                                //
                                status = IoGetDeviceProperty(fdoExtension->LowerPdo,
                                                             DevicePropertyPhysicalDeviceObjectName,
                                                             pdoNameLength,
                                                             stringIndex,
                                                             &resultLength);

                                if (NT_SUCCESS(status) && resultLength > 0) {
                                    errorLogEntry->NumberOfStrings++;
                                }
                            }
                        }
                    }
                }

                break;
            }

        }

        //
        // Write the error log packet to the system error logging thread.
        // It will be freed automatically.
        //
        IoWriteErrorLogEntry(errorLogEntry);

        TracePrint((TRACE_LEVEL_INFORMATION,
                    TRACE_FLAG_GENERAL,
                    "ClasspLogIORetriedEvent: DO (%p), Soft threshold notification logged.\n",
                    DeviceObject));
    }

    ClassReleaseRemoveLock(DeviceObject, (PIRP)workItem);

    if (ioLogMessageContextHeader->SenseData) {
        ExFreePool(ioLogMessageContextHeader->SenseData);
    }
    if (workItem) {
        IoFreeWorkItem(workItem);
    }
    ExFreePool(ioLogMessageContextHeader);
}


VOID
ClasspQueueLogIOEventWithContextWorker(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG SenseBufferSize,
    _In_ PVOID SenseData,
    _In_ UCHAR SrbStatus,
    _In_ UCHAR ScsiStatus,
    _In_ ULONG ErrorCode,
    _In_ ULONG CdbLength,
    _In_opt_ PCDB Cdb,
    _In_opt_ PTRANSFER_PACKET Pkt
    )
/*
Routine Description:

    Helper function that queues a delayed work item that will eventually
    log an event to the system event log corresponding to passed in ErrorCode.
    The dumpdata is fixed to include the opcode and the sense information.
    But the number of insertion strings varies based on the passed in ErrorCode.

Arguments:
    DeviceObject: The FDO that represents the device that was the target of the IO.
    SesneBufferSize: Size of the SenseData buffer.
    SenseData: Error information from the target (to be included in the dump data).
    SrbStatus: Srb status returned by the miniport.
    ScsiStatus: SCSI status associated with the request upon completion from lower layers.
    ErrorCode: Numerical value of the error code.
    CdbLength: Number of bytes of Cdb.
    Cdb: Pointer to the CDB.
    Pkt: The tranfer packet representing the IO of interest. This may be NULL.

--*/
{
    PCOMMON_DEVICE_EXTENSION commonExtension = (PCOMMON_DEVICE_EXTENSION)(DeviceObject->DeviceExtension);
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    POPCODE_SENSE_DATA_IO_LOG_MESSAGE_CONTEXT_HEADER ioLogMessageContextHeader = NULL;
    PVOID senseData = NULL;
    PIO_WORKITEM workItem = NULL;
    ULONG senseBufferSize = 0;
    LARGE_INTEGER lba = {0};

    if (!commonExtension->IsFdo) {
        return;
    }

    if (!Cdb) {
        return;
    }

    workItem = IoAllocateWorkItem(DeviceObject);
    if (!workItem) {
        goto __ClasspQueueLogIOEventWithContextWorker_ExitWithMessage;
    }

    if (SenseBufferSize) {
        senseData = ExAllocatePoolWithTag(NonPagedPoolNx, SenseBufferSize, CLASSPNP_POOL_TAG_LOG_MESSAGE);
        if (senseData) {
            senseBufferSize = SenseBufferSize;
        }
    }

    if (CdbLength == 16) {
        REVERSE_BYTES_QUAD(&lba, Cdb->CDB16.LogicalBlock);
    } else {
        ((PFOUR_BYTE)&lba.LowPart)->Byte3 = Cdb->CDB10.LogicalBlockByte0;
        ((PFOUR_BYTE)&lba.LowPart)->Byte2 = Cdb->CDB10.LogicalBlockByte1;
        ((PFOUR_BYTE)&lba.LowPart)->Byte1 = Cdb->CDB10.LogicalBlockByte2;
        ((PFOUR_BYTE)&lba.LowPart)->Byte0 = Cdb->CDB10.LogicalBlockByte3;
    }

    //
    // Calculate the amount of buffer required for the insertion strings.
    //
    switch (ErrorCode) {
        case IO_ERROR_IO_HARDWARE_ERROR:
        case IO_WARNING_IO_OPERATION_RETRIED: {

            PIO_RETRIED_LOG_MESSAGE_CONTEXT ioLogMessageContext = NULL;

            ioLogMessageContext = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(IO_RETRIED_LOG_MESSAGE_CONTEXT), CLASSPNP_POOL_TAG_LOG_MESSAGE);
            if (!ioLogMessageContext) {
                goto __ClasspQueueLogIOEventWithContextWorker_ExitWithMessage;
            }

            ioLogMessageContext->Lba.QuadPart = lba.QuadPart;
            ioLogMessageContext->DeviceNumber = fdoExtension->DeviceNumber;

            ioLogMessageContextHeader = (POPCODE_SENSE_DATA_IO_LOG_MESSAGE_CONTEXT_HEADER)ioLogMessageContext;

            break;
        }

        default: goto __ClasspQueueLogIOEventWithContextWorker_Exit;
    }

    TracePrint((TRACE_LEVEL_INFORMATION,
                TRACE_FLAG_GENERAL,
                "ClasspQueueLogIOEventWithContextWorker: DO (%p), Pkt (%p), Queueing IO retried event log message work item.\n",
                DeviceObject,
                Pkt));

    ioLogMessageContextHeader->WorkItem = workItem;
    if (senseData) {
        RtlCopyMemory(senseData, SenseData, SenseBufferSize);
    }
    ioLogMessageContextHeader->SenseData = senseData;
    ioLogMessageContextHeader->SenseDataSize = senseBufferSize;
    ioLogMessageContextHeader->SrbStatus = SrbStatus;
    ioLogMessageContextHeader->ScsiStatus = ScsiStatus;
    ioLogMessageContextHeader->OpCode = Cdb->CDB6GENERIC.OperationCode;
    ioLogMessageContextHeader->Reserved = 0;
    ioLogMessageContextHeader->ErrorCode = ErrorCode;

    ClassAcquireRemoveLock(DeviceObject, (PIRP)(workItem));

    //
    // Queue a work item to write the system event log.
    //
    IoQueueWorkItem(workItem, ClasspLogIOEventWithContext, DelayedWorkQueue, ioLogMessageContextHeader);

    return;

__ClasspQueueLogIOEventWithContextWorker_ExitWithMessage:

    TracePrint((TRACE_LEVEL_ERROR,
                TRACE_FLAG_GENERAL,
                "ClasspQueueLogIOEventWithContextWorker: DO (%p), Failed to allocate memory for the log message.\n",
                DeviceObject));

__ClasspQueueLogIOEventWithContextWorker_Exit:
    if (senseData) {
        ExFreePool(senseData);
    }
    if (workItem) {
        IoFreeWorkItem(workItem);
    }
    if (ioLogMessageContextHeader) {
        ExFreePool(ioLogMessageContextHeader);
    }
}

static
BOOLEAN
ValidPersistentReserveScope(
    UCHAR Scope)
{
    switch (Scope) {
    case RESERVATION_SCOPE_LU:
    case RESERVATION_SCOPE_ELEMENT:

        return TRUE;

    default:

        break;
    }

    return FALSE;
}

static
BOOLEAN
ValidPersistentReserveType(
    UCHAR Type)
{
    switch (Type) {
    case RESERVATION_TYPE_WRITE_EXCLUSIVE:
    case RESERVATION_TYPE_EXCLUSIVE:
    case RESERVATION_TYPE_WRITE_EXCLUSIVE_REGISTRANTS:
    case RESERVATION_TYPE_EXCLUSIVE_REGISTRANTS:

        return TRUE;

    default:

        break;
    }

    return FALSE;
}


/*++

ClasspPersistentReserve

Routine Description:

    Handles IOCTL_STORAGE_PERSISTENT_RESERVE_IN and IOCTL_STORAGE_PERSISTENT_RESERVE_OUT.

Arguments:

    DeviceObject - a pointer to the device object
    Irp - a pointer to the I/O request packet
    Srb - pointer to preallocated SCSI_REQUEST_BLOCK.

Return Value:

    Status Code

--*/
NTSTATUS
ClasspPersistentReserve(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    )
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PCDB cdb = NULL;
    PPERSISTENT_RESERVE_COMMAND prCommand = Irp->AssociatedIrp.SystemBuffer;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;

    NTSTATUS status;

    ULONG dataBufLen;
    ULONG controlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

    BOOLEAN writeToDevice;

    //
    // Check common input buffer parameters.
    //

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(PERSISTENT_RESERVE_COMMAND) ||
        prCommand->Size < sizeof(PERSISTENT_RESERVE_COMMAND)) {

        status = STATUS_INFO_LENGTH_MISMATCH;
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;

        FREE_POOL(Srb);

        ClassReleaseRemoveLock(DeviceObject, Irp);
        ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
        goto ClasspPersistentReserve_Exit;
    }

    //
    // Check buffer alignment. Only an issue if another kernel mode component
    // (not the I/O manager) allocates the buffer.
    //

    if ((ULONG_PTR)prCommand & fdoExtension->AdapterDescriptor->AlignmentMask) {

        status = STATUS_INVALID_USER_BUFFER;
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;

        FREE_POOL(Srb);

        ClassReleaseRemoveLock(DeviceObject, Irp);
        ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
        goto ClasspPersistentReserve_Exit;
    }

    //
    // Check additional parameters.
    //

    status = STATUS_SUCCESS;

    SrbSetCdbLength(Srb, 10);
    cdb = SrbGetCdb(Srb);

    if (controlCode == IOCTL_STORAGE_PERSISTENT_RESERVE_IN) {

        //
        // Check output buffer for PR In.
        //

        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
            prCommand->PR_IN.AllocationLength) {

            status = STATUS_INVALID_PARAMETER;
        }

        switch (prCommand->PR_IN.ServiceAction) {

        case RESERVATION_ACTION_READ_KEYS:

            if (prCommand->PR_IN.AllocationLength < sizeof(PRI_REGISTRATION_LIST)) {

                status = STATUS_INVALID_PARAMETER;
            }

            break;

        case RESERVATION_ACTION_READ_RESERVATIONS:

            if (prCommand->PR_IN.AllocationLength < sizeof(PRI_RESERVATION_LIST)) {

                status = STATUS_INVALID_PARAMETER;
            }

            break;

        default:

            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!NT_SUCCESS(status)) {

            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;

            FREE_POOL(Srb);

            ClassReleaseRemoveLock(DeviceObject, Irp);
            ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
            goto ClasspPersistentReserve_Exit;
        }

        //
        // Fill in the CDB.
        //

        cdb->PERSISTENT_RESERVE_IN.OperationCode    = SCSIOP_PERSISTENT_RESERVE_IN;
        cdb->PERSISTENT_RESERVE_IN.ServiceAction    = prCommand->PR_IN.ServiceAction;

        REVERSE_BYTES_SHORT(&(cdb->PERSISTENT_RESERVE_IN.AllocationLength),
                            &(prCommand->PR_IN.AllocationLength));

        dataBufLen = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
        writeToDevice = FALSE;


    } else {

        //
        // Verify ServiceAction, Scope, and Type
        //

        switch (prCommand->PR_OUT.ServiceAction) {

        case RESERVATION_ACTION_REGISTER:
        case RESERVATION_ACTION_REGISTER_IGNORE_EXISTING:
        case RESERVATION_ACTION_CLEAR:

            // Scope and type ignored.

            break;

        case RESERVATION_ACTION_RESERVE:
        case RESERVATION_ACTION_RELEASE:
        case RESERVATION_ACTION_PREEMPT:
        case RESERVATION_ACTION_PREEMPT_ABORT:

            if (!ValidPersistentReserveScope(prCommand->PR_OUT.Scope) ||
                !ValidPersistentReserveType(prCommand->PR_OUT.Type)) {

                status = STATUS_INVALID_PARAMETER;

            }

            break;

        default:

            status = STATUS_INVALID_PARAMETER;

            break;
        }

        //
        // Check input buffer for PR Out.
        // Caller must include the PR parameter list.
        //

        if (NT_SUCCESS(status)) {

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                    (sizeof(PERSISTENT_RESERVE_COMMAND) +
                     sizeof(PRO_PARAMETER_LIST)) ||
                prCommand->Size <
                    irpStack->Parameters.DeviceIoControl.InputBufferLength) {

            status = STATUS_INVALID_PARAMETER;

            }
        }


        if (!NT_SUCCESS(status)) {

            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;

            FREE_POOL(Srb);

            ClassReleaseRemoveLock(DeviceObject, Irp);
            ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
            goto ClasspPersistentReserve_Exit;
        }

        //
        // Fill in the CDB.
        //

        cdb->PERSISTENT_RESERVE_OUT.OperationCode   = SCSIOP_PERSISTENT_RESERVE_OUT;
        cdb->PERSISTENT_RESERVE_OUT.ServiceAction   = prCommand->PR_OUT.ServiceAction;
        cdb->PERSISTENT_RESERVE_OUT.Scope           = prCommand->PR_OUT.Scope;
        cdb->PERSISTENT_RESERVE_OUT.Type            = prCommand->PR_OUT.Type;

        cdb->PERSISTENT_RESERVE_OUT.ParameterListLength[1] = (UCHAR)sizeof(PRO_PARAMETER_LIST);

        //
        // Move the parameter list to the beginning of the data buffer (so it is aligned
        // correctly and that the MDL describes it correctly).
        //

        RtlMoveMemory(prCommand,
                      prCommand->PR_OUT.ParameterList,
                      sizeof(PRO_PARAMETER_LIST));

        dataBufLen = sizeof(PRO_PARAMETER_LIST);
        writeToDevice = TRUE;
    }

    //
    // Fill in the SRB
    //

    //
    // Set timeout value.
    //

    SrbSetTimeOutValue(Srb, fdoExtension->TimeOutValue);

    //
    // Send as a tagged request.
    //

    SrbSetRequestAttribute(Srb, SRB_HEAD_OF_QUEUE_TAG_REQUEST);
    SrbSetSrbFlags(Srb, SRB_FLAGS_NO_QUEUE_FREEZE | SRB_FLAGS_QUEUE_ACTION_ENABLE);

    status = ClassSendSrbAsynchronous(DeviceObject,
                                      Srb,
                                      Irp,
                                      prCommand,
                                      dataBufLen,
                                      writeToDevice);

ClasspPersistentReserve_Exit:

    return status;

}

/*++

ClasspPriorityHint

Routine Description:

    Handles IOCTL_STORAGE_CHECK_PRIORITY_HINT_SUPPORT.

Arguments:

    DeviceObject - a pointer to the device object
    Irp - a pointer to the I/O request packet

Return Value:

    Status Code

--*/
NTSTATUS
ClasspPriorityHint(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExtension->PrivateFdoData;
    PSTORAGE_PRIORITY_HINT_SUPPORT priSupport = Irp->AssociatedIrp.SystemBuffer;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;

    Irp->IoStatus.Information = 0;

    //
    // Check whether this device supports idle priority.
    //
    if (!fdoData->IdlePrioritySupported) {
        status = STATUS_NOT_SUPPORTED;
        goto PriorityHintExit;
    }

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(STORAGE_PRIORITY_HINT_SUPPORT)) {

        status = STATUS_BUFFER_TOO_SMALL;
        goto PriorityHintExit;
    }

    RtlZeroMemory(priSupport, sizeof(STORAGE_PRIORITY_HINT_SUPPORT));

    status = ClassForwardIrpSynchronous(commonExtension, Irp);
    if (!NT_SUCCESS(status)) {
        //
        // If I/O priority is not supported by lower drivers, just set the
        // priorities supported by class driver.
        //
        TracePrint((TRACE_LEVEL_FATAL, TRACE_FLAG_IOCTL, "ClasspPriorityHint: I/O priority not supported by port driver.\n"));
        priSupport->SupportFlags = 0;
        status = STATUS_SUCCESS;
    }

    TracePrint((TRACE_LEVEL_FATAL, TRACE_FLAG_IOCTL, "ClasspPriorityHint: I/O priorities supported by port driver: %X\n", priSupport->SupportFlags));

    priSupport->SupportFlags |= (1 << IoPriorityVeryLow) |
                                (1 << IoPriorityLow) |
                                (1 << IoPriorityNormal) ;

    TracePrint((TRACE_LEVEL_FATAL, TRACE_FLAG_IOCTL, "ClasspPriorityHint: I/O priorities supported: %X\n", priSupport->SupportFlags));
    Irp->IoStatus.Information = sizeof(STORAGE_PRIORITY_HINT_SUPPORT);

PriorityHintExit:

    Irp->IoStatus.Status = status;
    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
    return status;
}

/*++

ClasspConvertToScsiRequestBlock

Routine Description:

    Convert an extended SRB to a SCSI_REQUEST_BLOCK. This function handles only
    a single SRB and will not converted SRBs that are linked.

Arguments:

    Srb - a pointer to a SCSI_REQUEST_BLOCK
    SrbEx - a pointer to an extended SRB

Return Value:

    None

--*/
VOID
ClasspConvertToScsiRequestBlock(
    _Out_ PSCSI_REQUEST_BLOCK Srb,
    _In_ PSTORAGE_REQUEST_BLOCK SrbEx
    )
{
    PSTOR_ADDR_BTL8 storAddrBtl8;
    ULONG i;
    BOOLEAN foundEntry = FALSE;
    PSRBEX_DATA srbExData;

    if ((Srb == NULL) || (SrbEx == NULL)) {
        return;
    }

    RtlZeroMemory(Srb, sizeof(SCSI_REQUEST_BLOCK));

    Srb->Length = sizeof(SCSI_REQUEST_BLOCK);
    Srb->Function = (UCHAR)SrbEx->SrbFunction;
    Srb->SrbStatus = SrbEx->SrbStatus;
    Srb->QueueTag = (UCHAR)SrbEx->RequestTag;
    Srb->QueueAction = (UCHAR)SrbEx->RequestAttribute;
    Srb->SrbFlags = SrbEx->SrbFlags;
    Srb->DataTransferLength = SrbEx->DataTransferLength;
    Srb->TimeOutValue = SrbEx->TimeOutValue;
    Srb->DataBuffer = SrbEx->DataBuffer;
    Srb->OriginalRequest = SrbEx->OriginalRequest;
    Srb->SrbExtension = SrbEx->MiniportContext;
    Srb->InternalStatus = SrbEx->SystemStatus;

    //
    // Handle address fields
    //
    if (SrbEx->AddressOffset >= sizeof(STORAGE_REQUEST_BLOCK)) {
        storAddrBtl8 = (PSTOR_ADDR_BTL8)((PCHAR)SrbEx + SrbEx->AddressOffset);

        if (storAddrBtl8->Type == STOR_ADDRESS_TYPE_BTL8) {
            Srb->PathId = storAddrBtl8->Path;
            Srb->TargetId = storAddrBtl8->Target;
            Srb->Lun = storAddrBtl8->Lun;
        } else {
            // Catch unsupported address types
            NT_ASSERT(FALSE);
        }
    }

    //
    // Handle SRB function specific fields
    //
    if (SrbEx->NumSrbExData > 0) {

        for (i = 0; i < SrbEx->NumSrbExData; i++) {

            if ((SrbEx->SrbExDataOffset[i] == 0) ||
                (SrbEx->SrbExDataOffset[i] < sizeof(STORAGE_REQUEST_BLOCK))) {
                // Catch invalid offsets
                NT_ASSERT(FALSE);
                continue;
            }

            srbExData = (PSRBEX_DATA)((PCHAR)SrbEx + SrbEx->SrbExDataOffset[i]);

            switch (SrbEx->SrbFunction) {

                case SRB_FUNCTION_EXECUTE_SCSI:

                    switch (srbExData->Type) {

                        case SrbExDataTypeScsiCdb16:
                            Srb->ScsiStatus = ((PSRBEX_DATA_SCSI_CDB16)srbExData)->ScsiStatus;
                            Srb->CdbLength = ((PSRBEX_DATA_SCSI_CDB16)srbExData)->CdbLength;
                            Srb->SenseInfoBufferLength = ((PSRBEX_DATA_SCSI_CDB16)srbExData)->SenseInfoBufferLength;
                            Srb->SenseInfoBuffer = ((PSRBEX_DATA_SCSI_CDB16)srbExData)->SenseInfoBuffer;
                            RtlCopyMemory(Srb->Cdb, ((PSRBEX_DATA_SCSI_CDB16)srbExData)->Cdb, sizeof(Srb->Cdb));
                            foundEntry = TRUE;
                            break;

                        case SrbExDataTypeScsiCdb32:
                            Srb->ScsiStatus = ((PSRBEX_DATA_SCSI_CDB32)srbExData)->ScsiStatus;
                            Srb->CdbLength = ((PSRBEX_DATA_SCSI_CDB32)srbExData)->CdbLength;
                            Srb->SenseInfoBufferLength = ((PSRBEX_DATA_SCSI_CDB32)srbExData)->SenseInfoBufferLength;
                            Srb->SenseInfoBuffer = ((PSRBEX_DATA_SCSI_CDB32)srbExData)->SenseInfoBuffer;

                            // Copy only the first 16 bytes
                            RtlCopyMemory(Srb->Cdb, ((PSRBEX_DATA_SCSI_CDB32)srbExData)->Cdb, sizeof(Srb->Cdb));
                            foundEntry = TRUE;
                            break;

                        case SrbExDataTypeScsiCdbVar:
                            Srb->ScsiStatus = ((PSRBEX_DATA_SCSI_CDB_VAR)srbExData)->ScsiStatus;
                            Srb->CdbLength = (UCHAR)((PSRBEX_DATA_SCSI_CDB_VAR)srbExData)->CdbLength;
                            Srb->SenseInfoBufferLength = ((PSRBEX_DATA_SCSI_CDB_VAR)srbExData)->SenseInfoBufferLength;
                            Srb->SenseInfoBuffer = ((PSRBEX_DATA_SCSI_CDB_VAR)srbExData)->SenseInfoBuffer;

                            // Copy only the first 16 bytes
                            RtlCopyMemory(Srb->Cdb, ((PSRBEX_DATA_SCSI_CDB_VAR)srbExData)->Cdb, sizeof(Srb->Cdb));
                            foundEntry = TRUE;
                            break;

                        default:
                            break;

                    }
                    break;

                case SRB_FUNCTION_WMI:

                    if (srbExData->Type == SrbExDataTypeWmi) {
                        ((PSCSI_WMI_REQUEST_BLOCK)Srb)->WMISubFunction = ((PSRBEX_DATA_WMI)srbExData)->WMISubFunction;
                        ((PSCSI_WMI_REQUEST_BLOCK)Srb)->WMIFlags = ((PSRBEX_DATA_WMI)srbExData)->WMIFlags;
                        ((PSCSI_WMI_REQUEST_BLOCK)Srb)->DataPath = ((PSRBEX_DATA_WMI)srbExData)->DataPath;
                        foundEntry = TRUE;
                    }
                    break;

                case SRB_FUNCTION_PNP:

                    if (srbExData->Type == SrbExDataTypePnP) {
                        ((PSCSI_PNP_REQUEST_BLOCK)Srb)->PnPAction = ((PSRBEX_DATA_PNP)srbExData)->PnPAction;
                        ((PSCSI_PNP_REQUEST_BLOCK)Srb)->PnPSubFunction = ((PSRBEX_DATA_PNP)srbExData)->PnPSubFunction;
                        ((PSCSI_PNP_REQUEST_BLOCK)Srb)->SrbPnPFlags = ((PSRBEX_DATA_PNP)srbExData)->SrbPnPFlags;
                        foundEntry = TRUE;
                    }
                    break;

                case SRB_FUNCTION_POWER:

                    if (srbExData->Type == SrbExDataTypePower) {
                        ((PSCSI_POWER_REQUEST_BLOCK)Srb)->DevicePowerState = ((PSRBEX_DATA_POWER)srbExData)->DevicePowerState;
                        ((PSCSI_POWER_REQUEST_BLOCK)Srb)->PowerAction = ((PSRBEX_DATA_POWER)srbExData)->PowerAction;
                        ((PSCSI_POWER_REQUEST_BLOCK)Srb)->SrbPowerFlags = ((PSRBEX_DATA_POWER)srbExData)->SrbPowerFlags;
                        foundEntry = TRUE;
                    }
                    break;

                default:
                    break;

            }

            //
            // Quit on first match
            //
            if (foundEntry) {
                break;
            }
        }
    }

    return;
}



_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
ClasspGetMaximumTokenListIdentifier(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_z_ PWSTR RegistryPath,
    _Out_ PULONG MaximumListIdentifier
    )

/*++

Routine Description:

    This routine returns the maximum ListIdentifier (to be used when building TokenOperation
    requests) by querying the value MaximumListIdentifier under the key 'RegistryPath'.

Arguments:

    DeviceObject - The device handling the request.
    RegistryPath - The absolute registry path under which MaximumListIdentifier resides.
    MaximumListIdentifier - Returns the value being queried.

Return Value:

    STATUS_SUCCESS or appropriate error status returned by Registry API.

--*/

{
    RTL_QUERY_REGISTRY_TABLE queryTable[2];
    ULONG value = 0;
    NTSTATUS status;

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_PNP,
                "ClasspGetMaximumTokenListIdentifier (%p): Entering function.\n",
                DeviceObject));

    //
    // Zero the table entries.
    //
    RtlZeroMemory(queryTable, sizeof(queryTable));

    //
    // The query table has two entries. One for the MaximumListIdentifier and
    // the second which is the 'NULL' terminator.
    //
    // Indicate that there is NO call-back routine.
    //
    queryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK;

    //
    // The value to query.
    //
    queryTable[0].Name = REG_MAX_LIST_IDENTIFIER_VALUE;

    //
    // Where to put the value, the type of the value, default value and length.
    //
    queryTable[0].EntryContext = &value;
    queryTable[0].DefaultType = (REG_DWORD << RTL_QUERY_REGISTRY_TYPECHECK_SHIFT) | REG_DWORD;
    queryTable[0].DefaultData = &value;
    queryTable[0].DefaultLength = sizeof(value);

    //
    // Try to get the maximum listIdentifier.
    //
    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    RegistryPath,
                                    queryTable,
                                    NULL,
                                    NULL);

    if (NT_SUCCESS(status)) {
        *MaximumListIdentifier = value;
    } else {
    *MaximumListIdentifier = 0;
    }

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_PNP,
                "ClasspGetMaximumTokenListIdentifier (%p): Exiting function with status %x (maxListId %u).\n",
                DeviceObject,
                status,
                *MaximumListIdentifier));

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
ClasspGetCopyOffloadMaxDuration(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_z_ PWSTR RegistryPath,
    _Out_ PULONG MaxDuration
    )

    /*++

    Routine Description:

    This routine returns the maximum time (in seconds) that a Copy Offload
    operation should take to complete by a target.

    Arguments:

    DeviceObject - The device handling the request.
    RegistryPath - The absolute registry path under which MaxDuration resides.
    MaxDuration - Returns the value being queried, in seconds.

    Return Value:

    STATUS_SUCCESS or appropriate error status returned by Registry API.

    --*/

{
    RTL_QUERY_REGISTRY_TABLE queryTable[2];
    ULONG value = 0;
    NTSTATUS status;

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_PNP,
                "ClasspGetCopyOffloadMaxDuration (%p): Entering function.\n",
                DeviceObject));

    //
    // Zero the table entries.
    //
    RtlZeroMemory(queryTable, sizeof(queryTable));

    //
    // The query table has two entries. One for CopyOffloadMaxDuration and
    // the second which is the 'NULL' terminator.
    //
    // Indicate that there is NO call-back routine.
    //
    queryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK;

    //
    // The value to query.
    //
    queryTable[0].Name = CLASSP_REG_COPY_OFFLOAD_MAX_TARGET_DURATION;

    //
    // Where to put the value, the type of the value, default value and length.
    //
    queryTable[0].EntryContext = &value;
    queryTable[0].DefaultType = (REG_DWORD << RTL_QUERY_REGISTRY_TYPECHECK_SHIFT) | REG_NONE;
    queryTable[0].DefaultData = &value;
    queryTable[0].DefaultLength = sizeof(value);

    //
    // Try to get the max target duration.
    //
    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    RegistryPath,
                                    queryTable,
                                    NULL,
                                    NULL);

    //
    // Don't allow the user to set the value to lower than the default (4s) so
    // they don't break ODX functionality if they accidentally set it too low.
    //
    if (NT_SUCCESS(status) &&
        value > DEFAULT_MAX_TARGET_DURATION) {
        *MaxDuration = value;
    } else {
        *MaxDuration = DEFAULT_MAX_TARGET_DURATION;
    }

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_PNP,
                "ClasspGetCopyOffloadMaxDuration (%p): Exiting function with status %x (Max Duration %u seconds).\n",
                DeviceObject,
                status,
                *MaxDuration));

    return status;
}


_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
ClasspDeviceCopyOffloadProperty(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine returns the copy offload parameters associated with the device.

    This function must be called at IRQL < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Supplies the device object associated with this request
    Irp - The IRP to be processed
    Srb - The SRB associated with the request

Return Value:

    NTSTATUS code

--*/

{
    NTSTATUS status;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
    PSTORAGE_PROPERTY_QUERY query;
    PIO_STACK_LOCATION irpStack;
    ULONG length;
    ULONG information;
    PDEVICE_COPY_OFFLOAD_DESCRIPTOR copyOffloadDescr = (PDEVICE_COPY_OFFLOAD_DESCRIPTOR)Irp->AssociatedIrp.SystemBuffer;

    UNREFERENCED_PARAMETER(Srb);

    PAGED_CODE();

    fdoExtension = DeviceObject->DeviceExtension;
    query = (PSTORAGE_PROPERTY_QUERY)Irp->AssociatedIrp.SystemBuffer;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    length = 0;
    information = 0;

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspDeviceCopyOffloadProperty (%p): Entering function.\n",
                DeviceObject));

    //
    // Check proper query type.
    //
    if (query->QueryType == PropertyExistsQuery) {

        //
        // In order to maintain consistency with the how the rest of the properties
        // are handled, we shall always return success for PropertyExistsQuery.
        //
        status = STATUS_SUCCESS;
        goto __ClasspDeviceCopyOffloadProperty_Exit;

    } else if (query->QueryType != PropertyStandardQuery) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspDeviceCopyOffloadProperty (%p): Unsupported query type %x for Copy Offload property.\n",
                    DeviceObject,
                    query->QueryType));

        status = STATUS_NOT_SUPPORTED;
        goto __ClasspDeviceCopyOffloadProperty_Exit;
    }

    //
    // Request validation.
    // Note that InputBufferLength and IsFdo have been validated beforing entering this routine.
    //

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {

        NT_ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspDeviceCopyOffloadProperty (%p): Query property for Copy Offload called at incorrect IRQL.\n",
                    DeviceObject));

        status = STATUS_INVALID_LEVEL;
        goto __ClasspDeviceCopyOffloadProperty_Exit;
    }

    length = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (length < sizeof(DEVICE_COPY_OFFLOAD_DESCRIPTOR)) {

        if (length >= sizeof(STORAGE_DESCRIPTOR_HEADER)) {

            TracePrint((TRACE_LEVEL_WARNING,
                        TRACE_FLAG_IOCTL,
                        "ClasspDeviceCopyOffloadProperty (%p): Length %u specified for Copy Offload property enough only for header.\n",
                        DeviceObject,
                        length));

            information = sizeof(STORAGE_DESCRIPTOR_HEADER);
            copyOffloadDescr->Version = sizeof(DEVICE_COPY_OFFLOAD_DESCRIPTOR);
            copyOffloadDescr->Size = sizeof(DEVICE_COPY_OFFLOAD_DESCRIPTOR);

            status = STATUS_SUCCESS;
            goto __ClasspDeviceCopyOffloadProperty_Exit;
        }

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspDeviceCopyOffloadProperty (%p): Incorrect length %u specified for Copy Offload property.\n",
                    DeviceObject,
                    length));

        status = STATUS_BUFFER_TOO_SMALL;
        goto __ClasspDeviceCopyOffloadProperty_Exit;
    }

    if (!fdoExtension->FunctionSupportInfo->ValidInquiryPages.BlockDeviceRODLimits) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspDeviceCopyOffloadProperty (%p): Command not supported on this device.\n",
                    DeviceObject));

        status = STATUS_DEVICE_FEATURE_NOT_SUPPORTED;
        goto __ClasspDeviceCopyOffloadProperty_Exit;
    }

    if (!NT_SUCCESS(fdoExtension->FunctionSupportInfo->BlockDeviceRODLimitsData.CommandStatus)) {

        status = fdoExtension->FunctionSupportInfo->BlockDeviceRODLimitsData.CommandStatus;

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspDeviceCopyOffloadProperty (%p): VPD retrieval had failed with %x.\n",
                    DeviceObject,
                    status));

        goto __ClasspDeviceCopyOffloadProperty_Exit;
    }

    //
    // Fill in the output buffer.  All data is copied from the FDO extension where we
    // cached Block Limits and Block Device Token Limits info when the device was first initialized.
    //
    RtlZeroMemory(copyOffloadDescr, length);
    copyOffloadDescr->Version = 1;
    copyOffloadDescr->Size = sizeof(DEVICE_COPY_OFFLOAD_DESCRIPTOR);
    copyOffloadDescr->MaximumTokenLifetime = fdoExtension->FunctionSupportInfo->BlockDeviceRODLimitsData.MaximumInactivityTimer;
    copyOffloadDescr->DefaultTokenLifetime = fdoExtension->FunctionSupportInfo->BlockDeviceRODLimitsData.DefaultInactivityTimer;
    copyOffloadDescr->MaximumTransferSize = fdoExtension->FunctionSupportInfo->BlockDeviceRODLimitsData.MaximumTokenTransferSize;
    copyOffloadDescr->OptimalTransferCount = fdoExtension->FunctionSupportInfo->BlockDeviceRODLimitsData.OptimalTransferCount;
    copyOffloadDescr->MaximumDataDescriptors = fdoExtension->FunctionSupportInfo->BlockDeviceRODLimitsData.MaximumRangeDescriptors;

    if (NT_SUCCESS(fdoExtension->FunctionSupportInfo->BlockLimitsData.CommandStatus)) {

        copyOffloadDescr->MaximumTransferLengthPerDescriptor = fdoExtension->FunctionSupportInfo->BlockLimitsData.MaximumTransferLength;
        copyOffloadDescr->OptimalTransferLengthPerDescriptor = fdoExtension->FunctionSupportInfo->BlockLimitsData.OptimalTransferLength;
        copyOffloadDescr->OptimalTransferLengthGranularity = fdoExtension->FunctionSupportInfo->BlockLimitsData.OptimalTransferLengthGranularity;
    }

    information = sizeof(DEVICE_COPY_OFFLOAD_DESCRIPTOR);
    status = STATUS_SUCCESS;

__ClasspDeviceCopyOffloadProperty_Exit:

    //
    // Set the size and status in IRP
    //
    Irp->IoStatus.Information = information;
    Irp->IoStatus.Status = status;

    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspDeviceCopyOffloadProperty (%p): Exiting function with status %x.\n",
                DeviceObject,
                status));

    return status;
}


_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
ClasspValidateOffloadSupported(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp
    )

/*++

Routine Description:

    This routine validates if this device supports offload requests.

    This function must be called at IRQL < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Supplies the device object associated with this request
    Irp - The IRP to be processed

Return Value:

    NTSTATUS code

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt;
    NTSTATUS status;

    PAGED_CODE();

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspValidateOffloadSupported (%p): Entering function. Irp %p\n",
                DeviceObject,
                Irp));

    fdoExt = DeviceObject->DeviceExtension;
    status = STATUS_SUCCESS;

    //
    // For now this command is only supported by disk devices
    //
    if ((DeviceObject->DeviceType == FILE_DEVICE_DISK) &&
        (!TEST_FLAG(DeviceObject->Characteristics, FILE_FLOPPY_DISKETTE))) {

        if (!fdoExt->FunctionSupportInfo->ValidInquiryPages.BlockDeviceRODLimits) {

            TracePrint((TRACE_LEVEL_ERROR,
                        TRACE_FLAG_IOCTL,
                        "ClasspValidateOffloadSupported (%p): Command not supported on this disk device.\n",
                        DeviceObject));

            status = STATUS_DEVICE_FEATURE_NOT_SUPPORTED;
            goto __ClasspValidateOffloadSupported_Exit;
        }

        if (!NT_SUCCESS(fdoExt->FunctionSupportInfo->BlockDeviceRODLimitsData.CommandStatus)) {

            status = fdoExt->FunctionSupportInfo->BlockDeviceRODLimitsData.CommandStatus;

            TracePrint((TRACE_LEVEL_ERROR,
                        TRACE_FLAG_IOCTL,
                        "ClasspValidateOffloadSupported (%p): VPD retrieval failed with %x.\n",
                        DeviceObject,
                        status));

            goto __ClasspValidateOffloadSupported_Exit;
        }
    } else {

        TracePrint((TRACE_LEVEL_WARNING,
                    TRACE_FLAG_IOCTL,
                    "ClasspValidateOffloadSupported (%p): Suported only on Disk devices.\n",
                    DeviceObject));

        status = STATUS_DEVICE_FEATURE_NOT_SUPPORTED;
        goto __ClasspValidateOffloadSupported_Exit;
    }

__ClasspValidateOffloadSupported_Exit:
    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspValidateOffloadSupported (%p): Exiting function Irp %p with status %x.\n",
                DeviceObject,
                Irp,
                status));

    return status;
}


_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
ClasspValidateOffloadInputParameters(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp
    )

/*++

Routine Description:

    This routine does some basic validation of the input parameters of the offload request.

    This function must be called at IRQL < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Supplies the device object associated with this request
    Irp - The IRP to be processed

Return Value:

    NTSTATUS code

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_MANAGE_DATA_SET_ATTRIBUTES dsmAttributes;
    PDEVICE_DATA_SET_RANGE dataSetRanges;
    ULONG dataSetRangesCount;
    ULONG i;
    NTSTATUS status;

    PAGED_CODE();

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspValidateOffloadInputParameters (%p): Entering function Irp %p.\n",
                DeviceObject,
                Irp));

    fdoExtension = DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation (Irp);
    dsmAttributes = Irp->AssociatedIrp.SystemBuffer;
    status = STATUS_SUCCESS;

    if (!dsmAttributes) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspValidateOffloadInputParameters (%p): NULL DsmAttributes passed in.\n",
                    DeviceObject));

        status = STATUS_INVALID_PARAMETER;
        goto __ClasspValidateOffloadInputParameters_Exit;
    }

    if ((irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES)) ||
        (irpStack->Parameters.DeviceIoControl.InputBufferLength <
        (sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES) + dsmAttributes->ParameterBlockLength + dsmAttributes->DataSetRangesLength))) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspValidateOffloadInputParameters (%p): Input buffer size (%u) too small.\n",
                    DeviceObject,
                    irpStack->Parameters.DeviceIoControl.InputBufferLength));

        status = STATUS_INVALID_PARAMETER;
        goto __ClasspValidateOffloadInputParameters_Exit;
    }

    if ((dsmAttributes->DataSetRangesOffset == 0) ||
        (dsmAttributes->DataSetRangesLength == 0)) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspValidateOffloadInputParameters (%p): Incorrect DataSetRanges [offset %u, length %u].\n",
                    DeviceObject,
                    dsmAttributes->DataSetRangesOffset,
                    dsmAttributes->DataSetRangesLength));

        status = STATUS_INVALID_PARAMETER;
        goto __ClasspValidateOffloadInputParameters_Exit;
    }

    dataSetRanges = Add2Ptr(dsmAttributes, dsmAttributes->DataSetRangesOffset);
    dataSetRangesCount = dsmAttributes->DataSetRangesLength / sizeof(DEVICE_DATA_SET_RANGE);

    if (dataSetRangesCount == 0) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspValidateOffloadInputParameters (%p): DataSetRanges specifies no extents.\n",
                    DeviceObject));

        status = STATUS_INVALID_PARAMETER;
        goto __ClasspValidateOffloadInputParameters_Exit;
    }

    //
    // Some third party disk class drivers do not query the geometry at initialization time,
    // so this information may not be available at this time. If that is the case, we'll
    // first query that information before proceeding with the rest of our validations.
    //
    if (fdoExtension->DiskGeometry.BytesPerSector == 0) {
        status = ClassReadDriveCapacity(fdoExtension->DeviceObject);
        if ((!NT_SUCCESS(status)) || (fdoExtension->DiskGeometry.BytesPerSector == 0)) {
            TracePrint((TRACE_LEVEL_ERROR,
                        TRACE_FLAG_IOCTL,
                        "ClasspValidateOffloadInputParameters (%p): Couldn't retrieve disk geometry, status: %x, bytes/sector: %u.\n",
                        DeviceObject,
                        status,
                        fdoExtension->DiskGeometry.BytesPerSector));

            status = STATUS_INVALID_PARAMETER;
            goto __ClasspValidateOffloadInputParameters_Exit;
        }
    }

    //
    // Data must be aligned to sector boundary and
    // LengthInBytes must be > 0 for it to be a valid LBA entry
    //
    for (i = 0; i < dataSetRangesCount; i++) {
        if ((dataSetRanges[i].StartingOffset % fdoExtension->DiskGeometry.BytesPerSector != 0) ||
            (dataSetRanges[i].LengthInBytes % fdoExtension->DiskGeometry.BytesPerSector != 0) ||
            (dataSetRanges[i].LengthInBytes == 0) ) {
            TracePrint((TRACE_LEVEL_ERROR,
                        TRACE_FLAG_IOCTL,
                        "ClasspValidateOffloadInputParameters (%p): Incorrect DataSetRanges entry %u [offset %I64x, length %I64x].\n",
                        DeviceObject,
                        i,
                        dataSetRanges[i].StartingOffset,
                        dataSetRanges[i].LengthInBytes));

            status = STATUS_INVALID_PARAMETER;
            goto __ClasspValidateOffloadInputParameters_Exit;
        }

        if ((ULONGLONG)dataSetRanges[i].StartingOffset + dataSetRanges[i].LengthInBytes > (ULONGLONG)fdoExtension->CommonExtension.PartitionLength.QuadPart) {

            TracePrint((TRACE_LEVEL_ERROR,
                        TRACE_FLAG_IOCTL,
                        "ClasspValidateOffloadInputParameters (%p): Error! DataSetRange %u (starting LBA %I64x) specified length %I64x exceeds the medium's capacity (%I64x).\n",
                        DeviceObject,
                        i,
                        dataSetRanges[i].StartingOffset,
                        dataSetRanges[i].LengthInBytes,
                        fdoExtension->CommonExtension.PartitionLength.QuadPart));

            status = STATUS_NONEXISTENT_SECTOR;
            goto __ClasspValidateOffloadInputParameters_Exit;
        }
    }

__ClasspValidateOffloadInputParameters_Exit:
    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspValidateOffloadInputParameters (%p): Exiting function Irp %p with status %x.\n",
                DeviceObject,
                Irp,
                status));

    return status;
}


_IRQL_requires_same_
NTSTATUS
ClasspGetTokenOperationCommandBufferLength(
    _In_ PDEVICE_OBJECT Fdo,
    _In_ ULONG ServiceAction,
    _Inout_ PULONG CommandBufferLength,
    _Out_opt_ PULONG TokenOperationBufferLength,
    _Out_opt_ PULONG ReceiveTokenInformationBufferLength
    )

/*++

Routine description:

    This routine calculates the buffer length required to service a TokenOperation and its
    corresponding ReceiveTokenInformation command.

Arguments:

    Fdo - The functional device object processing the PopulateToken/WriteUsingToken request
    ServiceAction - Used to distinguish between a PopulateToken and a WriteUsingToken operation
    CommandBufferLength - Returns the length of the buffer needed to service the token request (i.e. TokenOperation and its corresponding ReceiveTokenInformation command)
    TokenOperationBufferLength - Optional parameter, which returns the length of the buffer needed to service just the TokenOperation command.
    ReceiveTokenInformationBufferLength - Optional parameter, which returns the length of the buffer needed to service just the ReceiveTokenInformation command.

Return Value:

    STATUS_SUCCESS

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;
    ULONG tokenOperationBufferLength;
    ULONG receiveTokenInformationBufferLength;
    PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;
    PSTORAGE_ADAPTER_DESCRIPTOR adapterDesc = commonExtension->PartitionZeroExtension->AdapterDescriptor;
    ULONG hwMaxXferLen;
    ULONG bufferLength = 0;
    ULONG tokenOperationHeaderSize;
    ULONG responseSize;

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspGetTokenOperationCommandBufferLengths (%p): Entering function.\n",
                Fdo));

    NT_ASSERT(fdoExt->FunctionSupportInfo->ValidInquiryPages.BlockDeviceRODLimits &&
              NT_SUCCESS(fdoExt->FunctionSupportInfo->BlockDeviceRODLimitsData.CommandStatus));

    if (ServiceAction == SERVICE_ACTION_POPULATE_TOKEN) {
        tokenOperationHeaderSize = FIELD_OFFSET(POPULATE_TOKEN_HEADER, BlockDeviceRangeDescriptor);
        responseSize = FIELD_OFFSET(RECEIVE_TOKEN_INFORMATION_RESPONSE_HEADER, TokenDescriptor) + sizeof(BLOCK_DEVICE_TOKEN_DESCRIPTOR);
    } else {
        tokenOperationHeaderSize = FIELD_OFFSET(WRITE_USING_TOKEN_HEADER, BlockDeviceRangeDescriptor);
        responseSize = 0;
    }

    //
    // The TokenOperation command can specify a parameter length of max 2^16.
    // If the device has a max limit on the number of range descriptors that can be specified in
    // the TokenOperation command, we are limited to the lesser of these two values.
    //
    if (fdoExt->FunctionSupportInfo->BlockDeviceRODLimitsData.MaximumRangeDescriptors == 0) {

        tokenOperationBufferLength = MAX_TOKEN_OPERATION_PARAMETER_DATA_LENGTH;

    } else {

        tokenOperationBufferLength = MIN(tokenOperationHeaderSize + fdoExt->FunctionSupportInfo->BlockDeviceRODLimitsData.MaximumRangeDescriptors * sizeof(BLOCK_DEVICE_RANGE_DESCRIPTOR),
                                         MAX_TOKEN_OPERATION_PARAMETER_DATA_LENGTH);
    }


    //
    // The ReceiveTokenInformation command can specify a parameter length of max 2 ^ 32
    // Also, since the sense data can be of variable size, we'll use MAX_SENSE_BUFFER_SIZE.
    //
    receiveTokenInformationBufferLength = MIN(FIELD_OFFSET(RECEIVE_TOKEN_INFORMATION_HEADER, SenseData) + MAX_SENSE_BUFFER_SIZE + responseSize,
                                              MAX_RECEIVE_TOKEN_INFORMATION_PARAMETER_DATA_LENGTH);

    //
    // Since we're going to reuse the buffer for both the TokenOperation and the ReceiveTokenInformation
    // commands, the buffer length needs to handle both operations.
    //
    bufferLength = MAX(tokenOperationBufferLength, receiveTokenInformationBufferLength);

    //
    // The buffer length needs to be further limited to the adapter's capability though.
    //
    hwMaxXferLen = MIN(fdoData->HwMaxXferLen, adapterDesc->MaximumTransferLength);
    bufferLength = MIN(bufferLength, hwMaxXferLen);

    *CommandBufferLength = bufferLength;

    if (TokenOperationBufferLength) {
        *TokenOperationBufferLength = tokenOperationBufferLength;
    }

    if (ReceiveTokenInformationBufferLength) {
        *ReceiveTokenInformationBufferLength = receiveTokenInformationBufferLength;
    }

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspGetTokenOperationCommandBufferLengths (%p): Exiting function with bufferLength %u (tokenOpBufLen %u, recTokenInfoBufLen %u).\n",
                Fdo,
                bufferLength,
                tokenOperationBufferLength,
                receiveTokenInformationBufferLength));

    return STATUS_SUCCESS;
}


_IRQL_requires_same_
NTSTATUS
ClasspGetTokenOperationDescriptorLimits(
    _In_ PDEVICE_OBJECT Fdo,
    _In_ ULONG ServiceAction,
    _In_ ULONG MaxParameterBufferLength,
    _Out_ PULONG MaxBlockDescriptorsCount,
    _Out_ PULONGLONG MaxBlockDescriptorsLength
    )

/*++

Routine description:

    This routine calculates the maximum block descriptors and the maximum token transfer size
    that can be accomodated in a single TokenOperation command.

Arguments:

    Fdo - The functional device object processing the PopulateToken/WriteUsingToken request
    ServiceAction - Used to distinguish between a PopulateToken and a WriteUsingToken operation
    MaxParameterBufferLength - The length constraint of the entire buffer for the parameter list based on other limitations (e.g. adapter max transfer length)
    MaxBlockDescriptorsCount - Returns the maximum number of the block range descriptors that can be passed in a single TokenOperation command.
    MaxBlockDescriptorsLength - Returns the maximum cumulative number of blocks across all the descriptors that must not be exceeded in a single TokenOperation command.

Return Value:

    STATUS_SUCCESS

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Fdo->DeviceExtension;
    ULONG tokenOperationHeaderSize = (ServiceAction == SERVICE_ACTION_POPULATE_TOKEN) ?
                                     FIELD_OFFSET(POPULATE_TOKEN_HEADER, BlockDeviceRangeDescriptor) :
                                     FIELD_OFFSET(WRITE_USING_TOKEN_HEADER, BlockDeviceRangeDescriptor);

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspGetTokenOperationDescriptorLimits (%p): Entering function.\n",
                Fdo));

    NT_ASSERT(fdoExt->FunctionSupportInfo->ValidInquiryPages.BlockDeviceRODLimits &&
              NT_SUCCESS(fdoExt->FunctionSupportInfo->BlockDeviceRODLimitsData.CommandStatus));

    *MaxBlockDescriptorsCount = (MaxParameterBufferLength - tokenOperationHeaderSize) / sizeof(BLOCK_DEVICE_RANGE_DESCRIPTOR);
    *MaxBlockDescriptorsLength = (fdoExt->FunctionSupportInfo->BlockDeviceRODLimitsData.MaximumTokenTransferSize == 0) ?
                                  MAX_TOKEN_TRANSFER_SIZE : fdoExt->FunctionSupportInfo->BlockDeviceRODLimitsData.MaximumTokenTransferSize;

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspGetTokenOperationDescriptorLimits (%p): Exiting function with MaxDescr %u, MaxXferBlocks %I64u.\n",
                Fdo,
                *MaxBlockDescriptorsCount,
                *MaxBlockDescriptorsLength));

    return STATUS_SUCCESS;
}



_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
ClasspConvertDataSetRangeToBlockDescr(
    _In_    PDEVICE_OBJECT Fdo,
    _In_    PVOID BlockDescr,
    _Inout_ PULONG CurrentBlockDescrIndex,
    _In_    ULONG MaxBlockDescrCount,
    _Inout_ PULONG CurrentLbaCount,
    _In_    ULONGLONG MaxLbaCount,
    _Inout_ PDEVICE_DATA_SET_RANGE DataSetRange,
    _Inout_ PULONGLONG TotalSectorsProcessed
    )

/*++

Routine Description:

    Convert DEVICE_DATA_SET_RANGE entry to be WINDOWS_BLOCK_DEVICE_RANGE_DESCRIPTOR entries.

    As LengthInBytes field in DEVICE_DATA_SET_RANGE structure is 64 bits (bytes)
    and LbaCount field in WINDOWS_BLOCK_DEVICE_RANGE_DESCRIPTOR structure is 32 bits (sectors),
    it's possible that one DEVICE_DATA_SET_RANGE entry needs multiple
    WINDOWS_BLOCK_DEVICE_RANGE_DESCRIPTOR entries. This routine handles the need for that
    potential split.

Arguments:

    Fdo - The functional device object
    BlockDescr - Pointer to the start of the Token Operation command's block descriptor
    CurrentBlockDescrIndex - Index into the block descriptor at which to update the DataSetRange info
                             It also gets updated to return the index to the next empty one.
    MaxBlockDescrCount - Maximum number of block descriptors that the device can handle in a single TokenOperation command
    CurrentLbaCount - Returns the LBA of the last successfully processed DataSetRange
    MaxLbaCount - Maximum transfer size that the device is capable of handling in a single TokenOperation command
    DataSetRange - Contains information about one range extent that needs to be converted into a block descriptor
    TotalSectorsProcessed - Returns the number of sectors corresponding to the DataSetRange that were succesfully mapped into block descriptors

Return Value:

    Nothing.

    NOTE: if LengthInBytes does not reach to 0, the conversion for DEVICE_DATA_SET_RANGE entry
          is not completed. Further conversion is needed by calling this function again.

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
    PBLOCK_DEVICE_RANGE_DESCRIPTOR blockDescr;
    ULONGLONG startingSector;
    ULONGLONG sectorCount;
    ULONGLONG totalSectorCount;
    ULONGLONG numberOfOptimalChunks;
    USHORT optimalLbaPerDescrGranularity;
    ULONG optimalLbaPerDescr;
    ULONG maxLbaPerDescr;

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspConvertDataSetRangeToBlockDescr (%p): Entering function. Starting offset %I64x.\n",
                Fdo,
                DataSetRange->StartingOffset));

    fdoExtension = Fdo->DeviceExtension;
    blockDescr = (PBLOCK_DEVICE_RANGE_DESCRIPTOR)BlockDescr;
    totalSectorCount = 0;


    //
    // Since the OptimalTransferLength and the MaximumTransferLength are overloaded parameters for
    // offloaded data transfers and regular read/write requests, it is not safe to use these values
    // as they may report back what is used by regular read/write, which will cause a perf degradation
    // in the offloaded case, since we may end up limiting the per block range descriptor length
    // specified as opposed to what the target can actually handle in a single request.
    // So until the SPC spec introduces these values specific to offloaded data transfers, we shall
    // ignore them completely. The expectation we have from the target is as follows:
    // 1. If the length specified in any of the block range descriptors is greater than the OTL that
    //    applies to ODX, the target will internally split into additional descriptors.
    // 2. If the above causes it to run out of descriptors, or if the length specified in any of the
    //    descriptors is greater than the MTL that applies to ODX, the target will operate on as much
    //    data as possible and truncate the request to that point.
    //
    optimalLbaPerDescrGranularity = 0;
    optimalLbaPerDescr = 0;
    maxLbaPerDescr = 0;

    if (optimalLbaPerDescr && maxLbaPerDescr) {

        NT_ASSERT(optimalLbaPerDescr <= maxLbaPerDescr);
    }

    while ((DataSetRange->LengthInBytes > 0) &&
           (*CurrentBlockDescrIndex < MaxBlockDescrCount) &&
           (*CurrentLbaCount < MaxLbaCount)) {

        startingSector = (ULONGLONG)(DataSetRange->StartingOffset / fdoExtension->DiskGeometry.BytesPerSector);

        //
        // Since the block descriptor has only 4 bytes for the number of logical blocks, we are
        // constrained by that theoretical maximum.
        //
        sectorCount = MIN(DataSetRange->LengthInBytes / fdoExtension->DiskGeometry.BytesPerSector,
                          MAX_NUMBER_BLOCKS_PER_BLOCK_DEVICE_RANGE_DESCRIPTOR);

        //
        // We are constrained by MaxLbaCount.
        //
        if (((ULONGLONG)*CurrentLbaCount + sectorCount) >= MaxLbaCount) {

            sectorCount = MaxLbaCount - *CurrentLbaCount;
        }

        //
        // For each descriptor, the block count should be lesser than the MaximumTransferSize
        //
        if (maxLbaPerDescr > 0) {

            //
            // Each block device range descriptor can specify a max number of LBAs
            //
            sectorCount = MIN(sectorCount, maxLbaPerDescr);
        }

        //
        // If the number of LBAs specified in the descriptor is greater than the OptimalTransferLength,
        // processing of this descriptor by the target may incur a significant delay.
        // So in order to allow the target to perform optimally, we'll further limit the number
        // of blocks specified in any descriptor to be maximum OptimalTranferLength.
        //
        if (optimalLbaPerDescr > 0) {

            sectorCount = MIN(sectorCount, optimalLbaPerDescr);
        }

        //
        // In addition, it should either be an exact multiple of the OptimalTransferLengthGranularity,
        // or be lesser than the OptimalTransferLengthGranularity (taken care of here).
        //
        if (optimalLbaPerDescrGranularity > 0) {

            numberOfOptimalChunks = sectorCount / optimalLbaPerDescrGranularity;

            if (numberOfOptimalChunks > 0) {
                sectorCount = numberOfOptimalChunks * optimalLbaPerDescrGranularity;
            }
        }

        NT_ASSERT(sectorCount <= MAX_NUMBER_BLOCKS_PER_BLOCK_DEVICE_RANGE_DESCRIPTOR);

        REVERSE_BYTES_QUAD(blockDescr[*CurrentBlockDescrIndex].LogicalBlockAddress, &startingSector);
        REVERSE_BYTES(blockDescr[*CurrentBlockDescrIndex].TransferLength, &sectorCount);

        totalSectorCount += sectorCount;

        DataSetRange->StartingOffset += sectorCount * fdoExtension->DiskGeometry.BytesPerSector;
        DataSetRange->LengthInBytes -= sectorCount * fdoExtension->DiskGeometry.BytesPerSector;

        *CurrentBlockDescrIndex += 1;
        *CurrentLbaCount += (ULONG)sectorCount;

        TracePrint((TRACE_LEVEL_INFORMATION,
                    TRACE_FLAG_IOCTL,
                    "ClasspConvertDataSetRangeToBlockDescr (%p): Descriptor: %u, starting LBA: %I64x, length: %I64x bytes, media size: %I64x.\n",
                    Fdo,
                    *CurrentBlockDescrIndex - 1,
                    startingSector,
                    sectorCount * fdoExtension->DiskGeometry.BytesPerSector,
                    (ULONGLONG)fdoExtension->CommonExtension.PartitionLength.QuadPart));
    }

    *TotalSectorsProcessed = totalSectorCount;

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspConvertDataSetRangeToBlockDescr (%p): Exiting function (starting offset %I64x). Total sectors processed %I64u.\n",
                Fdo,
                DataSetRange->StartingOffset,
                totalSectorCount));

    return;
}

_IRQL_requires_same_
PUCHAR
ClasspBinaryToAscii(
    _In_reads_(Length) PUCHAR HexBuffer,
    _In_ ULONG Length,
    _Inout_ PULONG UpdateLength
    )

/*++

Routine Description:

    This routine will convert HexBuffer into an ascii NULL-terminated string.

    Note: This routine will allocate memory for storing the ascii string. It is
          the responsibility of the caller to free this buffer.

Arguments:

    HexBuffer - Pointer to the binary data.
    Length - Length, in bytes, of HexBuffer.
    UpdateLength - Storage to place the actual length of the returned string.

Return Value:

    ASCII string equivalent of the hex buffer, or NULL if an error occurred.

--*/

{
    static const UCHAR integerTable[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    ULONG i;
    ULONG j;
    ULONG actualLength;
    PUCHAR buffer = NULL;
    UCHAR highWord;
    UCHAR lowWord;

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspBinaryToAscii (HexBuff %p): Entering function.\n",
                HexBuffer));

    if (!HexBuffer || Length == 0) {
        *UpdateLength = 0;
        goto __ClasspBinaryToAscii_Exit;
    }

    //
    // Each byte converts into 2 chars:
    // e.g. 0x05 => '0' '5'
    //      0x0C => '0' 'C'
    //      0x12 => '1' '2'
    // And we need a terminating NULL for the string.
    //
    actualLength = (Length * 2) + 1;

    //
    // Allocate the buffer.
    //
    buffer = ExAllocatePoolWithTag(NonPagedPoolNx, actualLength, CLASSPNP_POOL_TAG_TOKEN_OPERATION);
    if (!buffer) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspBinaryToAscii (HexBuff %p): Failed to allocate buffer for ASCII equivalent.\n",
                    HexBuffer));

        *UpdateLength = 0;
        goto __ClasspBinaryToAscii_Exit;
    }

    RtlZeroMemory(buffer, actualLength);

    for (i = 0, j = 0; i < Length; i++) {

        //
        // Split out each nibble from the binary byte.
        //
        highWord = HexBuffer[i] >> 4;
        lowWord = HexBuffer[i] & 0x0F;

        //
        // Using the lookup table, convert and stuff into
        // the ascii buffer.
        //
        buffer[j++] = integerTable[highWord];
#ifdef _MSC_VER
#pragma warning(suppress: 6386) // PREFast bug means it doesn't see that Length < actualLength
#endif
        buffer[j++] = integerTable[lowWord];
    }

    //
    // Update the caller's length field.
    //
    *UpdateLength = actualLength;

__ClasspBinaryToAscii_Exit:

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspBinaryToAscii (HexBuff %p): Exiting function with buffer %s.\n",
                HexBuffer,
                (buffer == NULL) ? "" : (const char*)buffer));

    return buffer;
}

_IRQL_requires_same_
NTSTATUS
ClasspStorageEventNotification(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp
    )

/*++

Routine Description:

    This routine handles an asynchronous event notification (most likely from
    port drivers). Currently, we only care about media status change events.

Arguments:

    DeviceObject - Supplies the device object associated with this request
    Irp - The IRP to be processed

Return Value:

    NTSTATUS code

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
    PIO_STACK_LOCATION irpStack;
    PSTORAGE_EVENT_NOTIFICATION storageEvents;
    NTSTATUS status;

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspStorageEventNotification (%p): Entering function Irp %p.\n",
                DeviceObject,
                Irp));

    fdoExtension = DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation (Irp);
    storageEvents = Irp->AssociatedIrp.SystemBuffer;
    status = STATUS_SUCCESS;

    if (!storageEvents) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspStorageEventNotification (%p): NULL storage events passed in.\n",
                    DeviceObject));

        status = STATUS_INVALID_PARAMETER;
        goto __ClasspStorageEventNotification_Exit;
    }

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(STORAGE_EVENT_NOTIFICATION)) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspStorageEventNotification (%p): Input buffer size (%u) too small.\n",
                    DeviceObject,
                    irpStack->Parameters.DeviceIoControl.InputBufferLength));

        status = STATUS_INFO_LENGTH_MISMATCH;
        goto __ClasspStorageEventNotification_Exit;
    }

    if ((storageEvents->Version != STORAGE_EVENT_NOTIFICATION_VERSION_V1) ||
        (storageEvents->Size != sizeof(STORAGE_EVENT_NOTIFICATION))) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspStorageEventNotification (%p): Invalid version/size [version %u, size %u].\n",
                    DeviceObject,
                    storageEvents->Version,
                    storageEvents->Size));

        status = STATUS_INVALID_PARAMETER;
        goto __ClasspStorageEventNotification_Exit;
    }

    //
    // Handle a media status event.
    //
    if (storageEvents->Events & STORAGE_EVENT_MEDIA_STATUS) {

        //
        // Only initiate operation if underlying port driver supports asynchronous notification
        // and this is the FDO.
        //
        if ((fdoExtension->CommonExtension.IsFdo == TRUE) &&
            (fdoExtension->FunctionSupportInfo->AsynchronousNotificationSupported)) {
            ClassCheckMediaState(fdoExtension);
        } else {
            status = STATUS_NOT_SUPPORTED;
        }

    }

__ClasspStorageEventNotification_Exit:
    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspStorageEventNotification (%p): Exiting function Irp %p with status %x.\n",
                DeviceObject,
                Irp,
                status));

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

    return status;
}

VOID
ClasspZeroQERR(
    _In_ PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This routine will attempt to set the QERR bit of the mode Control page to
    zero.

Arguments:

    DeviceObject - Supplies the device object associated with this request

Return Value:

    None

--*/
{
    PMODE_PARAMETER_HEADER modeData = NULL;
    PMODE_CONTROL_PAGE pageData = NULL;
    ULONG size = 0;

    modeData = ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned,
                                    MODE_PAGE_DATA_SIZE,
                                    CLASS_TAG_MODE_DATA);

    if (modeData == NULL) {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_SCSI, "ClasspZeroQERR: Unable to allocate mode data buffer\n"));
        goto ClasspZeroQERR_Exit;
    }

    RtlZeroMemory(modeData, MODE_PAGE_DATA_SIZE);

    size = ClassModeSense(DeviceObject,
                            (PCHAR) modeData,
                            MODE_PAGE_DATA_SIZE,
                            MODE_PAGE_CONTROL);

    if (size < sizeof(MODE_PARAMETER_HEADER)) {

        //
        // Retry the request in case of a check condition.
        //

        size = ClassModeSense(DeviceObject,
                                (PCHAR) modeData,
                                MODE_PAGE_DATA_SIZE,
                                MODE_PAGE_CONTROL);

        if (size < sizeof(MODE_PARAMETER_HEADER)) {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_SCSI, "ClasspZeroQERR: Mode Sense failed\n"));
            goto ClasspZeroQERR_Exit;
        }
    }

    //
    // If the size is greater than size indicated by the mode data reset
    // the data to the mode data.
    //

    if (size > (ULONG) (modeData->ModeDataLength + 1)) {
        size = modeData->ModeDataLength + 1;
    }

    //
    // Look for control page in the returned mode page data.
    //

    pageData = ClassFindModePage((PCHAR) modeData,
                                    size,
                                    MODE_PAGE_CONTROL,
                                    TRUE);

    if (pageData) {
        TracePrint((TRACE_LEVEL_VERBOSE,
                    TRACE_FLAG_SCSI,
                    "ClasspZeroQERR (%p): Current settings: QERR = %u, TST = %u, TAS = %u.\n",
                    DeviceObject,
                    pageData->QERR,
                    pageData->TST,
                    pageData->TAS));

        if (pageData->QERR != 0) {
            NTSTATUS status;
            UCHAR pageSavable = 0;

            //
            // Set QERR to 0 with a Mode Select command.  Re-use the modeData
            // and pageData structures.
            //
            pageData->QERR = 0;

            //
            // We use the original Page Savable (PS) value for the Save Pages
            // (SP) bit due to behavior described under the MODE SELECT(6)
            // section of SPC-4.
            //
            pageSavable = pageData->PageSavable;

            status = ClasspModeSelect(DeviceObject,
                                     (PCHAR)modeData,
                                     size,
                                     pageSavable);

            if (!NT_SUCCESS(status)) {
                TracePrint((TRACE_LEVEL_WARNING,
                    TRACE_FLAG_SCSI,
                    "ClasspZeroQERR (%p): Failed to set QERR = 0 with status %x\n",
                    DeviceObject,
                    status));
            }
        }
    }

ClasspZeroQERR_Exit:

    if (modeData != NULL) {
        ExFreePool(modeData);
    }
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
ClasspPowerActivateDevice(
    _In_ PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This routine synchronously sends an IOCTL_STORAGE_POWER_ACTIVE to the port
    PDO in order to take an active reference on the given device.  The device
    will remain powered up and active for as long as this active reference is
    taken.

    The caller should ensure idle power management is enabled for the device
    before calling this function.

    Call ClasspPowerIdleDevice to release the active reference.

Arguments:

    DeviceObject - Supplies the FDO associated with this request.

Return Value:

    STATUS_SUCCESS if the active reference was successfully taken.

--*/
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    NT_ASSERT(fdoExtension->CommonExtension.IsFdo);
    NT_ASSERT(fdoExtension->FunctionSupportInfo->IdlePower.IdlePowerEnabled);

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_POWER_ACTIVE,
                                        fdoExtension->LowerPdo,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        FALSE,
                                        &event,
                                        &ioStatus);

    if (irp != NULL) {
        status = IoCallDriver(fdoExtension->LowerPdo, irp);
        if (status == STATUS_PENDING) {
            (VOID)KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
ClasspPowerIdleDevice(
    _In_ PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This routine synchronously sends an IOCTL_STORAGE_POWER_IDLE to the port
    PDO in order to release an active reference on the given device.

    A call to ClasspPowerActivateDevice *must* have preceded a call to this
    function.

    The caller should ensure idle power management is enabled for the device
    before calling this function.

Arguments:

    DeviceObject - Supplies the FDO associated with this request.

Return Value:

    STATUS_SUCCESS if the active reference was successfully released.

--*/
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    NT_ASSERT(fdoExtension->CommonExtension.IsFdo);
    NT_ASSERT(fdoExtension->FunctionSupportInfo->IdlePower.IdlePowerEnabled);

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_POWER_IDLE,
                                        fdoExtension->LowerPdo,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        FALSE,
                                        &event,
                                        &ioStatus);

    if (irp != NULL) {
        status = IoCallDriver(fdoExtension->LowerPdo, irp);
        if (status == STATUS_PENDING) {
            (VOID)KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


#if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

NTSTATUS
ClasspGetHwFirmwareInfo(
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;

    PSTORAGE_HW_FIRMWARE_INFO firmwareInfo = NULL;
    PSTORAGE_HW_FIRMWARE_INFO_QUERY query = NULL;

    IO_STATUS_BLOCK ioStatus = {0};
    ULONG dataLength = sizeof(STORAGE_HW_FIRMWARE_INFO);
    ULONG iteration = 1;

    CLASS_FUNCTION_SUPPORT oldState;
    KLOCK_QUEUE_HANDLE  lockHandle;

    //
    // Try to get firmware information that contains only one slot.
    // We will retry the query if the required buffer size is bigger than that.
    //
retry:

    firmwareInfo = ExAllocatePoolWithTag(NonPagedPoolNx, dataLength, CLASSPNP_POOL_TAG_FIRMWARE);

    if (firmwareInfo == NULL) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT, "ClasspGetHwFirmwareInfo: cannot allocate memory to hold data. \n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(firmwareInfo, dataLength);

    //
    // Set up query data, making sure the "Flags" field indicating the request is for device itself.
    //
    query = (PSTORAGE_HW_FIRMWARE_INFO_QUERY)firmwareInfo;

    query->Version = sizeof(STORAGE_HW_FIRMWARE_INFO_QUERY);
    query->Size = sizeof(STORAGE_HW_FIRMWARE_INFO_QUERY);
    query->Flags = 0;

    //
    // On the first pass we just want to get the first few
    // bytes of the descriptor so we can read it's size
    //
    ClassSendDeviceIoControlSynchronous(IOCTL_STORAGE_FIRMWARE_GET_INFO,
                                        commonExtension->LowerDeviceObject,
                                        query,
                                        sizeof(STORAGE_HW_FIRMWARE_INFO_QUERY),
                                        dataLength,
                                        FALSE,
                                        &ioStatus
                                        );

    if (!NT_SUCCESS(ioStatus.Status) &&
        (ioStatus.Status != STATUS_BUFFER_OVERFLOW)) {
        if (ClasspLowerLayerNotSupport(ioStatus.Status)) {
            oldState = InterlockedCompareExchange((PLONG)(&fdoExtension->FunctionSupportInfo->HwFirmwareGetInfoSupport), (LONG)NotSupported, (ULONG)SupportUnknown);
        }

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT, "ClasspGetHwFirmwareInfo: error %lx trying to "
                                                        "query hardware firmware information #%d \n", ioStatus.Status, iteration));
        FREE_POOL(firmwareInfo);
        return ioStatus.Status;
    }

    //
    // Catch implementation issues from lower level driver.
    //
    if ((firmwareInfo->Version < sizeof(STORAGE_HW_FIRMWARE_INFO)) ||
        (firmwareInfo->Size < sizeof(STORAGE_HW_FIRMWARE_INFO)) ||
        (firmwareInfo->SlotCount == 0) ||
        (firmwareInfo->ImagePayloadMaxSize > fdoExtension->AdapterDescriptor->MaximumTransferLength)) {

        oldState = InterlockedCompareExchange((PLONG)(&fdoExtension->FunctionSupportInfo->HwFirmwareGetInfoSupport), (LONG)NotSupported, (ULONG)SupportUnknown);

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT, "ClasspGetHwFirmwareInfo: error in returned data! "
                                                        "Version: 0x%X, Size: 0x%X, SlotCount: 0x%X, ActiveSlot: 0x%X, PendingActiveSlot: 0x%X, ImagePayloadMaxSize: 0x%X \n",
                                                        firmwareInfo->Version,
                                                        firmwareInfo->Size,
                                                        firmwareInfo->SlotCount,
                                                        firmwareInfo->ActiveSlot,
                                                        firmwareInfo->PendingActivateSlot,
                                                        firmwareInfo->ImagePayloadMaxSize));

        FREE_POOL(firmwareInfo);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // If the data size is bigger than sizeof(STORAGE_HW_FIRMWARE_INFO), e.g. device has more than one firmware slot,
    // allocate a buffer to get all the data.
    //
    if ((firmwareInfo->Size > sizeof(STORAGE_HW_FIRMWARE_INFO)) &&
        (iteration < 2)) {

        dataLength = max(firmwareInfo->Size, sizeof(STORAGE_HW_FIRMWARE_INFO) + sizeof(STORAGE_HW_FIRMWARE_SLOT_INFO) * (firmwareInfo->SlotCount - 1));

        //
        // Retry the query with required buffer length.
        //
        FREE_POOL(firmwareInfo);
        iteration++;
        goto retry;
    }


    //
    // Set the support status and use the memory we've allocated as caching buffer.
    // In case of a competing thread already set the state, it will assign the caching buffer so release the current allocated one.
    //
    KeAcquireInStackQueuedSpinLock(&fdoExtension->FunctionSupportInfo->SyncLock, &lockHandle);

    oldState = InterlockedCompareExchange((PLONG)(&fdoExtension->FunctionSupportInfo->HwFirmwareGetInfoSupport), (LONG)Supported, (ULONG)SupportUnknown);

    if (oldState == SupportUnknown) {
        fdoExtension->FunctionSupportInfo->HwFirmwareInfo = firmwareInfo;
    } else if (oldState == Supported) {
        //
        // swap the buffers to keep the latest version.
        //
        PSTORAGE_HW_FIRMWARE_INFO cachedInfo = fdoExtension->FunctionSupportInfo->HwFirmwareInfo;

        fdoExtension->FunctionSupportInfo->HwFirmwareInfo = firmwareInfo;

        FREE_POOL(cachedInfo);
    } else {
        FREE_POOL(firmwareInfo);
    }

    KeReleaseInStackQueuedSpinLock(&lockHandle);

    return ioStatus.Status;
} // end ClasspGetHwFirmwareInfo()

#endif // #if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

#ifndef __REACTOS__ // the functions is not used
__inline
BOOLEAN
ClassDeviceHwFirmwareIsPortDriverSupported(
    _In_  PDEVICE_OBJECT DeviceObject
    )
/*
Routine Description:

    This function informs the caller whether the port driver supports hardware firmware requests.

Arguments:
    DeviceObject: The target object.

Return Value:

    TRUE if the port driver is supported.

--*/
{
    //
    // If the request is for a FDO, process the request for Storport, SDstor and Spaceport only.
    // Don't process it if we don't have a miniport descriptor.
    //
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;

    BOOLEAN isSupported = FALSE;
    if (commonExtension->IsFdo && (fdoExtension->MiniportDescriptor != NULL)) {
        isSupported = ((fdoExtension->MiniportDescriptor->Portdriver == StoragePortCodeSetStorport) ||
                       (fdoExtension->MiniportDescriptor->Portdriver == StoragePortCodeSetSpaceport) ||
                       (fdoExtension->MiniportDescriptor->Portdriver == StoragePortCodeSetSDport   ));
    }

    return isSupported;
}
#endif

NTSTATUS
ClassDeviceHwFirmwareGetInfoProcess(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
    )
/*
Routine Description:

    This function processes the Storage Hardware Firmware Get Information request.
    If the information is not cached yet, it gets from lower level driver.

Arguments:
    DeviceObject: The target FDO.
    Irp: The IRP which will contain the output buffer upon completion.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

#if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSTORAGE_HW_FIRMWARE_INFO_QUERY query = (PSTORAGE_HW_FIRMWARE_INFO_QUERY)Irp->AssociatedIrp.SystemBuffer;
    BOOLEAN passDown = FALSE;
    BOOLEAN copyData = FALSE;


    //
    // Input buffer is not big enough to contain required input information.
    //
    if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(STORAGE_HW_FIRMWARE_INFO_QUERY)) {

        status = STATUS_INFO_LENGTH_MISMATCH;
        goto Exit_Firmware_Get_Info;
    }

    //
    // Output buffer is too small to contain return data.
    //
    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(STORAGE_HW_FIRMWARE_INFO)) {

        status = STATUS_BUFFER_TOO_SMALL;
        goto Exit_Firmware_Get_Info;
    }

    //
    // Only process the request for a supported port driver.
    //
    if (!ClassDeviceHwFirmwareIsPortDriverSupported(DeviceObject)) {
        status = STATUS_NOT_IMPLEMENTED;
        goto Exit_Firmware_Get_Info;
    }

    //
    // Buffer "FunctionSupportInfo" is allocated during start device process. Following check defends against the situation
    // of receiving this IOCTL when the device is created but not started, or device start failed but did not get removed yet.
    //
    if (commonExtension->IsFdo && (fdoExtension->FunctionSupportInfo == NULL)) {

        status = STATUS_UNSUCCESSFUL;
        goto Exit_Firmware_Get_Info;
    }

    //
    // Process the situation that request should be forwarded to lower level.
    //
    if (!commonExtension->IsFdo) {
        passDown = TRUE;
    }

    if ((query->Flags & STORAGE_HW_FIRMWARE_REQUEST_FLAG_CONTROLLER) != 0) {
        passDown = TRUE;
    }

    if (passDown) {

        IoCopyCurrentIrpStackLocationToNext(Irp);

        ClassReleaseRemoveLock(DeviceObject, Irp);
        status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
        return status;
    }

    //
    // The request is for a FDO. Process the request.
    //
    if (fdoExtension->FunctionSupportInfo->HwFirmwareGetInfoSupport == NotSupported) {
        status = STATUS_NOT_IMPLEMENTED;
        goto Exit_Firmware_Get_Info;
    } else {
        //
        // Retrieve information from lower layer for the request. The cached information is not used
        // in case device firmware information changed.
        //
        status = ClasspGetHwFirmwareInfo(DeviceObject);
        copyData = NT_SUCCESS(status);
    }

Exit_Firmware_Get_Info:

    if (copyData) {
        //
        // Firmware information is already cached in classpnp. Return a copy.
        //
        KLOCK_QUEUE_HANDLE lockHandle;
        KeAcquireInStackQueuedSpinLock(&fdoExtension->FunctionSupportInfo->SyncLock, &lockHandle);

        ULONG dataLength = min(irpStack->Parameters.DeviceIoControl.OutputBufferLength, fdoExtension->FunctionSupportInfo->HwFirmwareInfo->Size);

        memcpy(Irp->AssociatedIrp.SystemBuffer, fdoExtension->FunctionSupportInfo->HwFirmwareInfo, dataLength);

        KeReleaseInStackQueuedSpinLock(&lockHandle);

        Irp->IoStatus.Information = dataLength;
    }

    Irp->IoStatus.Status = status;

#else
    status = STATUS_NOT_IMPLEMENTED;
    Irp->IoStatus.Status = status;
#endif // #if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

    return status;
}


#if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
ClassHwFirmwareDownloadComplete (
    _In_ PDEVICE_OBJECT Fdo,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context
    )
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    PIRP originalIrp;

    //
    // Free the allocated buffer for firmware image.
    //
    if (Context != NULL) {
        FREE_POOL(Context);
    }

    originalIrp = irpStack->Parameters.Others.Argument1;

    NT_ASSERT(originalIrp != NULL);

    originalIrp->IoStatus.Status = Irp->IoStatus.Status;
    originalIrp->IoStatus.Information = Irp->IoStatus.Information;

    ClassReleaseRemoveLock(Fdo, originalIrp);
    ClassCompleteRequest(Fdo, originalIrp, IO_DISK_INCREMENT);

    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;

} // end ClassHwFirmwareDownloadComplete()
#endif // #if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)


NTSTATUS
ClassDeviceHwFirmwareDownloadProcess(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    )
{
    NTSTATUS status = STATUS_SUCCESS;

#if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSTORAGE_HW_FIRMWARE_DOWNLOAD firmwareDownload = (PSTORAGE_HW_FIRMWARE_DOWNLOAD)Irp->AssociatedIrp.SystemBuffer;
    BOOLEAN passDown = FALSE;
    ULONG   i;
    ULONG   bufferSize = 0;
    PUCHAR  firmwareImageBuffer = NULL;
    PIRP irp2 = NULL;
    PIO_STACK_LOCATION newStack = NULL;
    PCDB cdb = NULL;
    BOOLEAN lockHeld = FALSE;
    KLOCK_QUEUE_HANDLE lockHandle;


    //
    // Input buffer is not big enough to contain required input information.
    //
    if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(STORAGE_HW_FIRMWARE_DOWNLOAD)) {

        status = STATUS_INFO_LENGTH_MISMATCH;
        goto Exit_Firmware_Download;
    }

    //
    // Input buffer basic validation.
    //
    if ((firmwareDownload->Version < sizeof(STORAGE_HW_FIRMWARE_DOWNLOAD)) ||
        (firmwareDownload->Size > irpStack->Parameters.DeviceIoControl.InputBufferLength) ||
        ((firmwareDownload->BufferSize + FIELD_OFFSET(STORAGE_HW_FIRMWARE_DOWNLOAD, ImageBuffer)) > firmwareDownload->Size)) {

        status = STATUS_INVALID_PARAMETER;
        goto Exit_Firmware_Download;
    }

    //
    // Only process the request for a supported port driver.
    //
    if (!ClassDeviceHwFirmwareIsPortDriverSupported(DeviceObject)) {
        status = STATUS_NOT_IMPLEMENTED;
        goto Exit_Firmware_Download;
    }

    //
    // Buffer "FunctionSupportInfo" is allocated during start device process. Following check defends against the situation
    // of receiving this IOCTL when the device is created but not started, or device start failed but did not get removed yet.
    //
    if (commonExtension->IsFdo && (fdoExtension->FunctionSupportInfo == NULL)) {

        status = STATUS_UNSUCCESSFUL;
        goto Exit_Firmware_Download;
    }

    //
    // Process the situation that request should be forwarded to lower level.
    //
    if (!commonExtension->IsFdo) {
        passDown = TRUE;
    }

    if ((firmwareDownload->Flags & STORAGE_HW_FIRMWARE_REQUEST_FLAG_CONTROLLER) != 0) {
        passDown = TRUE;
    }

    if (passDown) {

        IoCopyCurrentIrpStackLocationToNext(Irp);

        ClassReleaseRemoveLock(DeviceObject, Irp);
        status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
        FREE_POOL(Srb);
        return status;
    }

    //
    // If firmware information hasn't been cached in classpnp, retrieve it.
    //
    if (fdoExtension->FunctionSupportInfo->HwFirmwareInfo == NULL) {
        if (fdoExtension->FunctionSupportInfo->HwFirmwareGetInfoSupport == NotSupported) {
            status = STATUS_NOT_IMPLEMENTED;
            goto Exit_Firmware_Download;
        } else {
            //
            // If this is the first time of retrieving firmware information,
            // send request to lower level to get it.
            //
            status = ClasspGetHwFirmwareInfo(DeviceObject);

            if (!NT_SUCCESS(status)) {
                goto Exit_Firmware_Download;
            }
        }
    }

    //
    // Fail the request if the firmware information cannot be retrieved.
    //
    if (fdoExtension->FunctionSupportInfo->HwFirmwareInfo == NULL) {
        if (fdoExtension->FunctionSupportInfo->HwFirmwareGetInfoSupport == NotSupported) {
            status = STATUS_NOT_IMPLEMENTED;
        } else {
            status = STATUS_UNSUCCESSFUL;
        }

        goto Exit_Firmware_Download;
    }

    //
    // Acquire the SyncLock to ensure the HwFirmwareInfo pointer doesn't change
    // while we're dereferencing it.
    //
    lockHeld = TRUE;
    KeAcquireInStackQueuedSpinLock(&fdoExtension->FunctionSupportInfo->SyncLock, &lockHandle);

    //
    // Validate the device support
    //
    if ((fdoExtension->FunctionSupportInfo->HwFirmwareInfo->SupportUpgrade == FALSE) ||
        (fdoExtension->FunctionSupportInfo->HwFirmwareInfo->ImagePayloadAlignment == 0)) {
        status = STATUS_NOT_SUPPORTED;
        goto Exit_Firmware_Download;
    }

    //
    // Check if the slot can be used to hold firmware image.
    //
    for (i = 0; i < fdoExtension->FunctionSupportInfo->HwFirmwareInfo->SlotCount; i++) {
        if (fdoExtension->FunctionSupportInfo->HwFirmwareInfo->Slot[i].SlotNumber == firmwareDownload->Slot) {
            break;
        }
    }

    if ((i >= fdoExtension->FunctionSupportInfo->HwFirmwareInfo->SlotCount) ||
        (fdoExtension->FunctionSupportInfo->HwFirmwareInfo->Slot[i].ReadOnly == TRUE)) {
        //
        // Either the slot number is out of scope or the slot is read-only.
        //
        status = STATUS_INVALID_PARAMETER;
        goto Exit_Firmware_Download;
    }

    //
    // Buffer size and alignment validation.
    // Max Offset and Buffer Size can be represented by SCSI command is max value for 3 bytes.
    //
    if ((firmwareDownload->BufferSize == 0) ||
        ((firmwareDownload->BufferSize % fdoExtension->FunctionSupportInfo->HwFirmwareInfo->ImagePayloadAlignment) != 0) ||
        (firmwareDownload->BufferSize > fdoExtension->FunctionSupportInfo->HwFirmwareInfo->ImagePayloadMaxSize) ||
        (firmwareDownload->BufferSize > fdoExtension->AdapterDescriptor->MaximumTransferLength) ||
        ((firmwareDownload->Offset % fdoExtension->FunctionSupportInfo->HwFirmwareInfo->ImagePayloadAlignment) != 0) ||
        (firmwareDownload->Offset > 0xFFFFFF) ||
        (firmwareDownload->BufferSize > 0xFFFFFF)) {

        status = STATUS_INVALID_PARAMETER;
        goto Exit_Firmware_Download;
    }


    //
    // Process the request by translating it into WRITE BUFFER command.
    //
    if (((ULONG_PTR)firmwareDownload->ImageBuffer % fdoExtension->FunctionSupportInfo->HwFirmwareInfo->ImagePayloadAlignment) != 0) {
        //
        // Allocate buffer aligns to ImagePayloadAlignment to accommodate the alignment requirement.
        //
        bufferSize = ALIGN_UP_BY(firmwareDownload->BufferSize, fdoExtension->FunctionSupportInfo->HwFirmwareInfo->ImagePayloadAlignment);

        //
        // We're done accessing HwFirmwareInfo at this point so we can release
        // the SyncLock.
        //
        NT_ASSERT(lockHeld);
        KeReleaseInStackQueuedSpinLock(&lockHandle);
        lockHeld = FALSE;

#ifdef _MSC_VER
#pragma prefast(suppress:6014, "The allocated memory that firmwareImageBuffer points to will be freed in ClassHwFirmwareDownloadComplete().")
#endif
        firmwareImageBuffer = ExAllocatePoolWithTag(NonPagedPoolNx, bufferSize, CLASSPNP_POOL_TAG_FIRMWARE);

        if (firmwareImageBuffer == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_Firmware_Download;
        }

        RtlZeroMemory(firmwareImageBuffer, bufferSize);

        RtlCopyMemory(firmwareImageBuffer, firmwareDownload->ImageBuffer, (ULONG)firmwareDownload->BufferSize);

    } else {
        NT_ASSERT(lockHeld);
        KeReleaseInStackQueuedSpinLock(&lockHandle);
        lockHeld = FALSE;

        firmwareImageBuffer = firmwareDownload->ImageBuffer;
        bufferSize = (ULONG)firmwareDownload->BufferSize;
    }

    //
    // Allocate a new irp to send the WRITE BUFFER command down.
    // Similar process as IOCTL_STORAGE_CHECK_VERIFY.
    //
    irp2 = IoAllocateIrp((CCHAR)(DeviceObject->StackSize + 3), FALSE);

    if (irp2 == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        if (firmwareImageBuffer != firmwareDownload->ImageBuffer) {
            FREE_POOL(firmwareImageBuffer);
        }

        goto Exit_Firmware_Download;
    }

    //
    // Make sure to acquire the lock for the new irp.
    //
    ClassAcquireRemoveLock(DeviceObject, irp2);

    irp2->Tail.Overlay.Thread = Irp->Tail.Overlay.Thread;
    IoSetNextIrpStackLocation(irp2);

    //
    // Set the top stack location and shove the master Irp into the
    // top location
    //
    newStack = IoGetCurrentIrpStackLocation(irp2);
    newStack->Parameters.Others.Argument1 = Irp;
    newStack->DeviceObject = DeviceObject;

    //
    // Stick the firmware download completion routine onto the stack
    // and prepare the irp for the port driver
    //
    IoSetCompletionRoutine(irp2,
                           ClassHwFirmwareDownloadComplete,
                           (firmwareImageBuffer != firmwareDownload->ImageBuffer) ? firmwareImageBuffer : NULL,
                           TRUE,
                           TRUE,
                           TRUE);

    IoSetNextIrpStackLocation(irp2);
    newStack = IoGetCurrentIrpStackLocation(irp2);
    newStack->DeviceObject = DeviceObject;
    newStack->MajorFunction = irpStack->MajorFunction;
    newStack->MinorFunction = irpStack->MinorFunction;
    newStack->Flags = irpStack->Flags;


    //
    // Mark the master irp as pending - whether the lower level
    // driver completes it immediately or not this should allow it
    // to go all the way back up.
    //
    IoMarkIrpPending(Irp);

    //
    // Setup the CDB.
    //
    SrbSetCdbLength(Srb, CDB10GENERIC_LENGTH);
    cdb = SrbGetCdb(Srb);
    cdb->WRITE_BUFFER.OperationCode = SCSIOP_WRITE_DATA_BUFF;
    cdb->WRITE_BUFFER.Mode = SCSI_WRITE_BUFFER_MODE_DOWNLOAD_MICROCODE_WITH_OFFSETS_SAVE_DEFER_ACTIVATE;
    cdb->WRITE_BUFFER.ModeSpecific = 0;     //Reserved for Mode 0x0E
    cdb->WRITE_BUFFER.BufferID = firmwareDownload->Slot;

    cdb->WRITE_BUFFER.BufferOffset[0] = *((PCHAR)&firmwareDownload->Offset + 2);
    cdb->WRITE_BUFFER.BufferOffset[1] = *((PCHAR)&firmwareDownload->Offset + 1);
    cdb->WRITE_BUFFER.BufferOffset[2] = *((PCHAR)&firmwareDownload->Offset);

    cdb->WRITE_BUFFER.ParameterListLength[0] = *((PCHAR)&bufferSize + 2);
    cdb->WRITE_BUFFER.ParameterListLength[1] = *((PCHAR)&bufferSize + 1);
    cdb->WRITE_BUFFER.ParameterListLength[2] = *((PCHAR)&bufferSize);

    //
    // Send as a tagged command.
    //
    SrbSetRequestAttribute(Srb, SRB_HEAD_OF_QUEUE_TAG_REQUEST);
    SrbSetSrbFlags(Srb, SRB_FLAGS_NO_QUEUE_FREEZE | SRB_FLAGS_QUEUE_ACTION_ENABLE);

    //
    // Set timeout value.
    //
    SrbSetTimeOutValue(Srb, fdoExtension->TimeOutValue);

    //
    // This routine uses a completion routine so we don't want to release
    // the remove lock until then.
    //
    status = ClassSendSrbAsynchronous(DeviceObject,
                                      Srb,
                                      irp2,
                                      firmwareImageBuffer,
                                      bufferSize,
                                      TRUE);

    if (status != STATUS_PENDING) {
        //
        // If the new irp cannot be sent down, free allocated memory and bail out.
        //
        if (firmwareImageBuffer != firmwareDownload->ImageBuffer) {
            FREE_POOL(firmwareImageBuffer);
        }

        //
        // If the irp cannot be sent down, the Srb has been freed. NULL it to prevent from freeing it again.
        //
        Srb = NULL;

        ClassReleaseRemoveLock(DeviceObject, irp2);

        IoFreeIrp(irp2);

        goto Exit_Firmware_Download;
    }

    return status;

Exit_Firmware_Download:

    //
    // Release the SyncLock if it's still held.
    // This should only happen in the failure path.
    //
    if (lockHeld) {
        KeReleaseInStackQueuedSpinLock(&lockHandle);
        lockHeld = FALSE;
    }

    //
    // Firmware Download request will be failed.
    //
    NT_ASSERT(!NT_SUCCESS(status));

    Irp->IoStatus.Status = status;

#else
    status = STATUS_NOT_IMPLEMENTED;
    Irp->IoStatus.Status = status;
#endif // #if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

    FREE_POOL(Srb);

    return status;
}

NTSTATUS
ClassDeviceHwFirmwareActivateProcess(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    )
{
    NTSTATUS status = STATUS_SUCCESS;

#if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSTORAGE_HW_FIRMWARE_ACTIVATE firmwareActivate = (PSTORAGE_HW_FIRMWARE_ACTIVATE)Irp->AssociatedIrp.SystemBuffer;
    BOOLEAN passDown = FALSE;
    PCDB cdb = NULL;
    ULONG   i;
    BOOLEAN lockHeld = FALSE;
    KLOCK_QUEUE_HANDLE lockHandle;


    //
    // Input buffer is not big enough to contain required input information.
    //
    if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(STORAGE_HW_FIRMWARE_ACTIVATE)) {

        status = STATUS_INFO_LENGTH_MISMATCH;
        goto Exit_Firmware_Activate;
    }

    //
    // Input buffer basic validation.
    //
    if ((firmwareActivate->Version < sizeof(STORAGE_HW_FIRMWARE_ACTIVATE)) ||
        (firmwareActivate->Size > irpStack->Parameters.DeviceIoControl.InputBufferLength)) {

        status = STATUS_INVALID_PARAMETER;
        goto Exit_Firmware_Activate;
    }

    //
    // Only process the request for a supported port driver.
    //
    if (!ClassDeviceHwFirmwareIsPortDriverSupported(DeviceObject)) {
        status = STATUS_NOT_IMPLEMENTED;
        goto Exit_Firmware_Activate;
    }

    //
    // Buffer "FunctionSupportInfo" is allocated during start device process. Following check defends against the situation
    // of receiving this IOCTL when the device is created but not started, or device start failed but did not get removed yet.
    //
    if (commonExtension->IsFdo && (fdoExtension->FunctionSupportInfo == NULL)) {

        status = STATUS_UNSUCCESSFUL;
        goto Exit_Firmware_Activate;
    }

    //
    // Process the situation that request should be forwarded to lower level.
    //
    if (!commonExtension->IsFdo) {
        passDown = TRUE;
    }

    if ((firmwareActivate->Flags & STORAGE_HW_FIRMWARE_REQUEST_FLAG_CONTROLLER) != 0) {
        passDown = TRUE;
    }

    if (passDown) {

        IoCopyCurrentIrpStackLocationToNext(Irp);

        ClassReleaseRemoveLock(DeviceObject, Irp);
        status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
        FREE_POOL(Srb);
        return status;
    }

    //
    // If firmware information hasn't been cached in classpnp, retrieve it.
    //
    if (fdoExtension->FunctionSupportInfo->HwFirmwareInfo == NULL) {
        if (fdoExtension->FunctionSupportInfo->HwFirmwareGetInfoSupport == NotSupported) {
            status = STATUS_NOT_IMPLEMENTED;
            goto Exit_Firmware_Activate;
        } else {
            //
            // If this is the first time of retrieving firmware information,
            // send request to lower level to get it.
            //
            status = ClasspGetHwFirmwareInfo(DeviceObject);

            if (!NT_SUCCESS(status)) {
                goto Exit_Firmware_Activate;
            }
        }
    }

    //
    // Fail the request if the firmware information cannot be retrieved.
    //
    if (fdoExtension->FunctionSupportInfo->HwFirmwareInfo == NULL) {
        if (fdoExtension->FunctionSupportInfo->HwFirmwareGetInfoSupport == NotSupported) {
            status = STATUS_NOT_IMPLEMENTED;
        } else {
            status = STATUS_UNSUCCESSFUL;
        }

        goto Exit_Firmware_Activate;
    }

    //
    // Acquire the SyncLock to ensure the HwFirmwareInfo pointer doesn't change
    // while we're dereferencing it.
    //
    lockHeld = TRUE;
    KeAcquireInStackQueuedSpinLock(&fdoExtension->FunctionSupportInfo->SyncLock, &lockHandle);

    //
    // Validate the device support
    //
    if (fdoExtension->FunctionSupportInfo->HwFirmwareInfo->SupportUpgrade == FALSE) {
        status = STATUS_NOT_SUPPORTED;
        goto Exit_Firmware_Activate;
    }

    //
    // Check if the slot number is valid.
    //
    for (i = 0; i < fdoExtension->FunctionSupportInfo->HwFirmwareInfo->SlotCount; i++) {
        if (fdoExtension->FunctionSupportInfo->HwFirmwareInfo->Slot[i].SlotNumber == firmwareActivate->Slot) {
            break;
        }
    }

    if (i >= fdoExtension->FunctionSupportInfo->HwFirmwareInfo->SlotCount) {
        //
        // Either the slot number is out of scope or the slot is read-only.
        //
        status = STATUS_INVALID_PARAMETER;
        goto Exit_Firmware_Activate;
    }

    //
    // We're done accessing HwFirmwareInfo at this point so we can release
    // the SyncLock.
    //
    NT_ASSERT(lockHeld);
    KeReleaseInStackQueuedSpinLock(&lockHandle);
    lockHeld = FALSE;

    //
    // Process the request by translating it into WRITE BUFFER command.
    //
    //
    // Setup the CDB. This should be an untagged request.
    //
    SrbSetCdbLength(Srb, CDB10GENERIC_LENGTH);
    cdb = SrbGetCdb(Srb);
    cdb->WRITE_BUFFER.OperationCode = SCSIOP_WRITE_DATA_BUFF;
    cdb->WRITE_BUFFER.Mode = SCSI_WRITE_BUFFER_MODE_ACTIVATE_DEFERRED_MICROCODE;
    cdb->WRITE_BUFFER.ModeSpecific = 0;                     //Reserved for Mode 0x0F
    cdb->WRITE_BUFFER.BufferID = firmwareActivate->Slot;    //NOTE: this field will be ignored by SCSI device.

    //
    // Set timeout value.
    //
    SrbSetTimeOutValue(Srb, FIRMWARE_ACTIVATE_TIMEOUT_VALUE);

    //
    // This routine uses a completion routine - ClassIoComplete() so we don't want to release
    // the remove lock until then.
    //
    status = ClassSendSrbAsynchronous(DeviceObject,
                                      Srb,
                                      Irp,
                                      NULL,
                                      0,
                                      FALSE);

    if (status != STATUS_PENDING) {
        //
        // If the irp cannot be sent down, the Srb has been freed. NULL it to prevent from freeing it again.
        //
        Srb = NULL;

        goto Exit_Firmware_Activate;
    }

    return status;

Exit_Firmware_Activate:

    //
    // Release the SyncLock if it's still held.
    // This should only happen in the failure path.
    //
    if (lockHeld) {
        KeReleaseInStackQueuedSpinLock(&lockHandle);
        lockHeld = FALSE;
    }

    //
    // Firmware Activate request will be failed.
    //
    NT_ASSERT(!NT_SUCCESS(status));

    Irp->IoStatus.Status = status;

#else
    status = STATUS_NOT_IMPLEMENTED;
    Irp->IoStatus.Status = status;
#endif // #if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

    FREE_POOL(Srb);
    return status;
}


BOOLEAN
ClasspIsThinProvisioningError (
    _In_ PSCSI_REQUEST_BLOCK Srb
    )
/*++

Routine Description:

    This routine checks whether the completed SRB Srb was completed with a thin provisioning
    soft threshold error.

Arguments:

    Srb - the SRB to be checked.

Return Value:

    BOOLEAN

--*/
{
    if (TEST_FLAG(Srb->SrbStatus, SRB_STATUS_AUTOSENSE_VALID)) {
        PVOID senseBuffer = SrbGetSenseInfoBuffer(Srb);
        if (senseBuffer) {
            UCHAR senseKey = 0;
            UCHAR addlSenseCode = 0;
            UCHAR addlSenseCodeQual = 0;
            BOOLEAN validSense = ScsiGetSenseKeyAndCodes(senseBuffer,
                                                         SrbGetSenseInfoBufferLength(Srb),
                                                         SCSI_SENSE_OPTIONS_NONE,
                                                         &senseKey,
                                                         &addlSenseCode,
                                                         &addlSenseCodeQual);

            return (validSense
                    && (senseKey == SCSI_SENSE_UNIT_ATTENTION)
                    && (addlSenseCode == SCSI_ADSENSE_LB_PROVISIONING)
                    && (addlSenseCodeQual == SCSI_SENSEQ_SOFT_THRESHOLD_REACHED));
        }
    }
    return FALSE;
}
