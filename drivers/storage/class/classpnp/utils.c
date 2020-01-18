/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

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

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, ClassGetDeviceParameter)
    #pragma alloc_text(PAGE, ClassScanForSpecial)
    #pragma alloc_text(PAGE, ClassSetDeviceParameter)
#endif

// custom string match -- careful!
BOOLEAN NTAPI ClasspMyStringMatches(IN PCSTR StringToMatch OPTIONAL, IN PCSTR TargetString)
{
    ULONG length;  // strlen returns an int, not size_t (!)
    PAGED_CODE();
    ASSERT(TargetString);
    // if no match requested, return TRUE
    if (StringToMatch == NULL) {
        return TRUE;
    }
    // cache the string length for efficiency
    length = strlen(StringToMatch);
    // ZERO-length strings may only match zero-length strings
    if (length == 0) {
        return (strlen(TargetString) == 0);
    }
    // strncmp returns zero if the strings match
    return (strncmp(StringToMatch, TargetString, length) == 0);
}

VOID NTAPI ClassGetDeviceParameter(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PWSTR SubkeyName OPTIONAL,
    IN PWSTR ParameterName,
    IN OUT PULONG ParameterValue  // also default value
    )
{
    NTSTATUS                 status;
    RTL_QUERY_REGISTRY_TABLE queryTable[2];
    HANDLE                   deviceParameterHandle;
    HANDLE                   deviceSubkeyHandle;
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
        OBJECT_ATTRIBUTES objectAttributes;

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

        RtlZeroMemory(queryTable, sizeof(queryTable));

        defaultParameterValue = *ParameterValue;

        queryTable->Flags         = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
        queryTable->Name          = ParameterName;
        queryTable->EntryContext  = ParameterValue;
        queryTable->DefaultType   = REG_DWORD;
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

    return;

} // end ClassGetDeviceParameter()

NTSTATUS NTAPI ClassSetDeviceParameter(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PWSTR SubkeyName OPTIONAL,
    IN PWSTR ParameterName,
    IN ULONG ParameterValue)
{
    NTSTATUS                 status;
    HANDLE                   deviceParameterHandle;
    HANDLE                   deviceSubkeyHandle;

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

VOID NTAPI ClassScanForSpecial(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN CLASSPNP_SCAN_FOR_SPECIAL_INFO DeviceList[],
    IN PCLASS_SCAN_FOR_SPECIAL_HANDLER Function)
{
    PSTORAGE_DEVICE_DESCRIPTOR deviceDescriptor;
    PCSTR vendorId;
    PCSTR productId;
    PCSTR productRevision;
    CHAR nullString[] = "";
    //ULONG j;

    PAGED_CODE();
    ASSERT(DeviceList);
    ASSERT(Function);

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
        vendorId = ((PCSTR)deviceDescriptor);
        vendorId += deviceDescriptor->VendorIdOffset;
    } else {
        vendorId = nullString;
    }
    if (deviceDescriptor->ProductIdOffset != 0 &&
        deviceDescriptor->ProductIdOffset != -1) {
        productId = ((PCSTR)deviceDescriptor);
        productId += deviceDescriptor->ProductIdOffset;
    } else {
        productId = nullString;
    }
    if (deviceDescriptor->VendorIdOffset != 0 &&
        deviceDescriptor->VendorIdOffset != -1) {
        productRevision = ((PCSTR)deviceDescriptor);
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

        if (ClasspMyStringMatches(DeviceList->VendorId,        vendorId) &&
            ClasspMyStringMatches(DeviceList->ProductId,       productId) &&
            ClasspMyStringMatches(DeviceList->ProductRevision, productRevision)
            ) {

            DebugPrint((1, "ClasspScanForSpecialByInquiry: Found matching "
                        "controller Ven: %s Prod: %s Rev: %s\n",
                        vendorId, productId, productRevision));

            //
            // pass the context to the call back routine and exit
            //

            (Function)(FdoExtension, DeviceList->Data);

            //
            // for CHK builds, try to prevent wierd stacks by having a debug
            // print here. it's a hack, but i know of no other way to prevent
            // the stack from being wrong.
            //

            DebugPrint((16, "ClasspScanForSpecialByInquiry: "
                        "completed callback\n"));
            return;

        } // else the strings did not match

    } // none of the devices matched.

    DebugPrint((1, "ClasspScanForSpecialByInquiry: no match found for %p\n",
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
// is also necessary because we are guaranteed to be modifying the
// SRB flags here, setting SuccessfulIO to zero, and incrementing the
// actual error count (which is always done within this spinlock).
//
// whenever there is no error, increment a counter.  if there have been
// errors on the device, and we've enabled dynamic perf, *and* we've
// just crossed the perf threshold, then grab the spin lock and
// double check that the threshold has, indeed been hit(*). then
// decrement the error count, and if it's dropped sufficiently, undo
// some of the safety changes made in the SRB flags due to the errors.
//
// * this works in all cases.  even if lots of ios occur after the
//   previous guy went in and cleared the successfulio counter, that
//   just means that we've hit the threshold again, and so it's proper
//   to run the inner loop again.
//

VOID
NTAPI
ClasspPerfIncrementErrorCount(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    PCLASS_PRIVATE_FDO_DATA fdoData = FdoExtension->PrivateFdoData;
    KIRQL oldIrql;
    ULONG errors;

    KeAcquireSpinLock(&fdoData->SpinLock, &oldIrql);

    fdoData->Perf.SuccessfulIO = 0; // implicit interlock
    errors = InterlockedIncrement((PLONG)&FdoExtension->ErrorCount);

    if (errors >= CLASS_ERROR_LEVEL_1) {

        //
        // If the error count has exceeded the error limit, then disable
        // any tagged queuing, multiple requests per lu queueing
        // and synchronous data transfers.
        //
        // Clearing the no queue freeze flag prevents the port driver
        // from sending multiple requests per logical unit.
        //

        CLEAR_FLAG(FdoExtension->SrbFlags, SRB_FLAGS_NO_QUEUE_FREEZE);
        CLEAR_FLAG(FdoExtension->SrbFlags, SRB_FLAGS_QUEUE_ACTION_ENABLE);

        SET_FLAG(FdoExtension->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);

        DebugPrint((ClassDebugError, "ClasspPerfIncrementErrorCount: "
                    "Too many errors; disabling tagged queuing and "
                    "synchronous data tranfers.\n"));

    }

    if (errors >= CLASS_ERROR_LEVEL_2) {

        //
        // If a second threshold is reached, disable disconnects.
        //

        SET_FLAG(FdoExtension->SrbFlags, SRB_FLAGS_DISABLE_DISCONNECT);
        DebugPrint((ClassDebugError, "ClasspPerfIncrementErrorCount: "
                    "Too many errors; disabling disconnects.\n"));
    }

    KeReleaseSpinLock(&fdoData->SpinLock, oldIrql);
    return;
}

VOID
NTAPI
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

    if (fdoData->Perf.ReEnableThreshold == 0) {
        return;
    }

    succeeded = InterlockedIncrement((PLONG)&fdoData->Perf.SuccessfulIO);
    if (succeeded < fdoData->Perf.ReEnableThreshold) {
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
    // for a single threshold being hit.  this keeps errorcount
    // somewhat useful.
    //

    succeeded = fdoData->Perf.SuccessfulIO;

    if ((FdoExtension->ErrorCount != 0) &&
        (fdoData->Perf.ReEnableThreshold <= succeeded)
        ) {

        fdoData->Perf.SuccessfulIO = 0; // implicit interlock

        ASSERT(FdoExtension->ErrorCount > 0);
        errors = InterlockedDecrement((PLONG)&FdoExtension->ErrorCount);

        //
        // note: do in reverse order of the sets "just in case"
        //

        if (errors < CLASS_ERROR_LEVEL_2) {
            if (errors == CLASS_ERROR_LEVEL_2 - 1) {
                DebugPrint((ClassDebugError, "ClasspPerfIncrementSuccessfulIo: "
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
                DebugPrint((ClassDebugError, "ClasspPerfIncrementSuccessfulIo: "
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
    } // end of threshold definitely being hit for first time

    KeReleaseSpinLock(&fdoData->SpinLock, oldIrql);
    return;
}

PMDL NTAPI BuildDeviceInputMdl(PVOID Buffer, ULONG BufferLen)
{
    PMDL mdl;

    mdl = IoAllocateMdl(Buffer, BufferLen, FALSE, FALSE, NULL);
    if (mdl){
        _SEH2_TRY {
            /*
             *  We are reading from the device.
             *  Therefore, the device is WRITING to the locked memory.
             *  So we request IoWriteAccess.
             */
            MmProbeAndLockPages(mdl, KernelMode, IoWriteAccess);

        } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
            NTSTATUS status = _SEH2_GetExceptionCode();

            DBGWARN(("BuildReadMdl: MmProbeAndLockPages failed with %xh.", status));
            IoFreeMdl(mdl);
            mdl = NULL;
        } _SEH2_END;
    }
    else {
        DBGWARN(("BuildReadMdl: IoAllocateMdl failed"));
    }

    return mdl;
}

VOID NTAPI FreeDeviceInputMdl(PMDL Mdl)
{
    MmUnlockPages(Mdl);
    IoFreeMdl(Mdl);
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
        DebugPrint((ClassDebugError, "ClasspPerfResetCounters: "
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
