/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    FxBugcheckCallback.cpp

Abstract:

    This module contains the bugcheck callback functions for determining whether
    a wdf driver caused a bugcheck and for capturing the IFR data to the mini-
    dump file.

Revision History:



--*/


#include "fxcorepch.hpp"
#include "fxifr.h"
#include "fxifrkm.h"     // kernel mode only IFR definitions
#include "fxldr.h"
#include "fxbugcheck.h"

// #include <aux_klib.h>

//
// Disable warnings of features used by the standard headers
//
// Disable warning C4115: named type definition in parentheses
// Disable warning C4200: nonstandard extension used : zero-sized array in struct/union
// Disable warning C4201: nonstandard extension used : nameless struct/union
// Disable warning C4214: nonstandard extension used : bit field types other than int
//
// #pragma warning(disable:4115 4200 4201 4214)
#include <ntimage.h>
// #pragma warning(default:4115 4200 4201 4214)


extern "C" {

//
// Private methods.
//
_Must_inspect_result_
BOOLEAN
FxpIsAddressKnownToWdf(
    __in PVOID Address,
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    );

_Must_inspect_result_
NTSTATUS
FxpGetImageBase(
    __in  PDRIVER_OBJECT DriverObject,
    __out PVOID* ImageBase,
    __out PULONG ImageSize
    );

_Must_inspect_result_
BOOLEAN
FxpBugCheckCallbackFilter(
    __inout PFX_DRIVER_GLOBALS FxDriverGlobals
    );

KBUGCHECK_REASON_CALLBACK_ROUTINE FxpBugCheckCallback;

KBUGCHECK_REASON_CALLBACK_ROUTINE FxpLibraryBugCheckCallback;

_Must_inspect_result_
BOOLEAN
FxpIsAddressKnownToWdf(
    __in PVOID Address,
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
/*++

Routine Description:

    This routine will check the address to determine if it falls within the
    image of a WDF component (Library, Client driver or Class Extension).
    This is accomplished by traversing the WdfLdrGlobals.LoadedModuleList
    and comparing each image start/end address with Address.

Arguments:

    PVOID Address - The address to be checked, and it need not be currently
        paged in. In other words, it is NOT used in this routine as an
        address, but more as a simple scaler into a 4GB (x86) array.

        NOTE - Do not attempt to validatate Address, say via MmIsAddressValid(Address).
               Address's memory could be paged out, but Address is still valid.

    PFX_DRIVER_GLOBALS FxDriverGlobals - Driver's globals.

Return Value:

    TRUE indicates something was found, either library, client or both.
    FALSE indicates either not found or invalid parameters.

--*/
{

    if (NULL == Address || NULL == FxDriverGlobals) {
        return FALSE;
    }

    if (Address >= FxDriverGlobals->ImageAddress &&
        Address < WDF_PTR_ADD_OFFSET(FxDriverGlobals->ImageAddress,
                                     FxDriverGlobals->ImageSize)) {
        return TRUE;
    }

    return FALSE;
}

_Must_inspect_result_
NTSTATUS
FxpGetImageBase(
    __in  PDRIVER_OBJECT DriverObject,
    __out PVOID* ImageBase,
    __out PULONG ImageSize
    )
{
//     NTSTATUS status = STATUS_UNSUCCESSFUL;
//     ULONG modulesSize = 0;
//     AUX_MODULE_EXTENDED_INFO* modules = NULL;
//     AUX_MODULE_EXTENDED_INFO* module;
//     PVOID addressInImage = NULL;
//     ULONG numberOfModules;
//     ULONG i;

//     //
//     // Basic validation.
//     //
//     if (NULL == DriverObject || NULL == ImageBase || NULL == ImageSize) {
//         status = STATUS_INVALID_PARAMETER;
//         goto exit;
//     }

//     //
//     // Get the address of a well known entry in the Image.
//     //
//     addressInImage = (PVOID) DriverObject->DriverStart;
//     ASSERT(addressInImage != NULL);

//     //
//     // Initialize the AUX Kernel Library.
//     //
//     status = AuxKlibInitialize();
//     if (!NT_SUCCESS(status)) {
//         goto exit;
//     }

//     //
//     // Get size of area needed for loaded modules.
//     //
//     status = AuxKlibQueryModuleInformation(&modulesSize,
//                                            sizeof(AUX_MODULE_EXTENDED_INFO),
//                                            NULL);

//     if (!NT_SUCCESS(status) || (0 == modulesSize)) {
//         goto exit;
//     }

//     numberOfModules = modulesSize / sizeof(AUX_MODULE_EXTENDED_INFO);

//     //
//     // Allocate returned-sized memory for the modules area.
//     //
//     modules = (AUX_MODULE_EXTENDED_INFO*) ExAllocatePoolWithTag(PagedPool,
//                                                                 modulesSize,
//                                                                 '30LW');
//     if (NULL == modules) {
//         status = STATUS_INSUFFICIENT_RESOURCES;
//         goto exit;
//     }

//     //
//     // Request the modules array be filled with module information.
//     //
//     status = AuxKlibQueryModuleInformation(&modulesSize,
//                                            sizeof(AUX_MODULE_EXTENDED_INFO),
//                                            modules);

//     if (!NT_SUCCESS(status)) {
//         goto exit;
//     }

//     //
//     // Traverse list, searching for the well known address in Image for which the
//     // module's Image Base Address is in its range.
//     //
//     module = modules;

//     for (i=0; i < numberOfModules; i++) {

//         if (addressInImage >= module->BasicInfo.ImageBase &&
//             addressInImage < WDF_PTR_ADD_OFFSET(module->BasicInfo.ImageBase,
//                                                 module->ImageSize)) {

//             *ImageBase = module->BasicInfo.ImageBase;
//             *ImageSize = module->ImageSize;

//             status = STATUS_SUCCESS;
//             goto exit;
//         }
//         module++;
//     }

//     status = STATUS_NOT_FOUND;

// exit:

//     if (modules != NULL) {
//         ExFreePool(modules);
//         modules = NULL;
//     }

//     return status;
    ROSWDFNOTIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
BOOLEAN
FxpBugCheckCallbackFilter(
    __inout PFX_DRIVER_GLOBALS FxDriverGlobals
    )
/*++

Routine Description:

    This routine evaluates whether the driver's IFR data has to be written to
    the mini-dump file.

Arguments:

    FxDriverGlobals - The driver globals of the wdf driver.

Return Value:

    TRUE  -  Indicates this driver's IFR log is to be captured in dump.
    FALSE -  Indicates this driver's log is (probably) not of interest.

--*/
{
    // PVOID codeAddr = NULL;
    // BOOLEAN found = FALSE;
    // KBUGCHECK_DATA bugCheckData = {0};

    // if (FxDriverGlobals->FxForceLogsInMiniDump) {
    //     return TRUE;
    // }

    // //
    // // Retrieve the bugcheck parameters.
    // //
    // bugCheckData.BugCheckDataSize = sizeof(KBUGCHECK_DATA);
    // AuxKlibGetBugCheckData(&bugCheckData);

    // //
    // // Check whether the code address that caused the bugcheck is from this wdf
    // // driver.
    // //
    // switch (bugCheckData.BugCheckCode) {

    // case KERNEL_APC_PENDING_DURING_EXIT:                    // 0x20
    //     codeAddr = (PVOID)bugCheckData.Parameter1;
    //     found = FxpIsAddressKnownToWdf(codeAddr, FxDriverGlobals);
    //     break;

    // case KMODE_EXCEPTION_NOT_HANDLED:                       // 0x1E
    // case SYSTEM_THREAD_EXCEPTION_NOT_HANDLED:               // 0x7E
    // case KERNEL_MODE_EXCEPTION_NOT_HANDLED:                 // 0x8E
    //     codeAddr = (PVOID)bugCheckData.Parameter2;
    //     found = FxpIsAddressKnownToWdf(codeAddr, FxDriverGlobals);
    //     break;

    // case PAGE_FAULT_IN_NONPAGED_AREA:                       // 0x50
    //     codeAddr = (PVOID)bugCheckData.Parameter3;
    //     found = FxpIsAddressKnownToWdf(codeAddr, FxDriverGlobals);
    //     break;

    // case IRQL_NOT_LESS_OR_EQUAL:                            // 0xA
    // case DRIVER_IRQL_NOT_LESS_OR_EQUAL:                     // 0xD1
    //     codeAddr = (PVOID)bugCheckData.Parameter4;
    //     found = FxpIsAddressKnownToWdf(codeAddr, FxDriverGlobals);
    //     break;
    // }

    // //
    // // If the code address was found in the wdf driver, then set the flag in the
    // // driver globals to indicate that the IFR data has to be written to the
    // // mini-dump.
    // //
    // if (found) {
    //     FxDriverGlobals->FxForceLogsInMiniDump = TRUE;
    // }
    // return found;
    ROSWDFNOTIMPLEMENTED;
    return FALSE;
}

VOID
STDCALL
FxpBugCheckCallback(
    __in    KBUGCHECK_CALLBACK_REASON Reason,
    __in    PKBUGCHECK_REASON_CALLBACK_RECORD Record,
    __inout PVOID ReasonSpecificData,
    __in    ULONG ReasonSpecificLength
    )

/*++

Routine Description:

    BugCheck callback routine for WDF

Arguments:

    Reason               - Must be KbCallbackSecondaryData
    Record               - Supplies the bugcheck record previously registered
    ReasonSpecificData   - Pointer to KBUGCHECK_SECONDARY_DUMP_DATA
    ReasonSpecificLength - Sizeof(ReasonSpecificData)

Return Value:

    None

Notes:
    When a bugcheck happens the kernel bugcheck processor will make two passes
    of all registered BugCheckCallbackRecord routines.  The first pass, called
    the "sizing pass" essentially queries all the callbacks to collect the
    total size of the secondary dump data. In the second pass the actual data
    is captured to the dump.

--*/

{
    PKBUGCHECK_SECONDARY_DUMP_DATA dumpData;
    PFX_DRIVER_GLOBALS fxDriverGlobals;
    ULONG logSize;
    BOOLEAN writeLog = FALSE;

    UNREFERENCED_PARAMETER(Reason);
    UNREFERENCED_PARAMETER(ReasonSpecificLength);

    ASSERT(ReasonSpecificLength >= sizeof(KBUGCHECK_SECONDARY_DUMP_DATA));
    ASSERT(Reason == KbCallbackSecondaryDumpData);

    dumpData = (PKBUGCHECK_SECONDARY_DUMP_DATA) ReasonSpecificData;

    //
    // See if the IFR's minimum amount of data can fit in the dump
    //
    if (dumpData->MaximumAllowed < FxIFRMinLogSize) {
        return;
    }

    //
    // Get the driver globals containing the bugcheck callback record.
    //
    fxDriverGlobals = CONTAINING_RECORD(Record,
                                        FX_DRIVER_GLOBALS,
                                        BugCheckCallbackRecord);

    //
    // If there is not IFR data, then this bugcheck callback does not have any
    // data to write to the minidump.
    //
    if (NULL == fxDriverGlobals->WdfLogHeader) {
        return;
    }

    //
    // Size is the length of the buffer which we log to. It should also include
    // the size of the header (which was subtracted out in FxIFRStart).
    //
    logSize = ((PWDF_IFR_HEADER) fxDriverGlobals->WdfLogHeader)->Size +
              sizeof(WDF_IFR_HEADER);

    ASSERT(FxIFRMinLogSize <= logSize && logSize <= FxIFRMaxLogSize);

    //
    // Check whether log can fit in the dump.
    //
    if (logSize > dumpData->MaximumAllowed) {
        return;
    }

    //
    // Check whether this log is the one to copy.
    //
    if (FxpBugCheckCallbackFilter(fxDriverGlobals)) {
        //
        // Let everyone know that we got the best match.
        //
        FxLibraryGlobals.BestDriverForDumpLog = fxDriverGlobals;

        //
        // Exact match driver.
        //
        writeLog = TRUE;
    }
    else if (NULL == FxLibraryGlobals.BestDriverForDumpLog) {
        //
        // So far we didn't get a perfect match. Make a best effort
        // to find a good candidate for the driver dump log.
        //
        if (fxDriverGlobals->FxTrackDriverForMiniDumpLog &&
            FxLibraryGlobals.DriverTracker.GetCurrentTrackedDriver() ==
                fxDriverGlobals) {
            //
            // Best effort match driver.
            //
            writeLog = TRUE;
        }
    }

    if (writeLog) {
        dumpData->OutBuffer = fxDriverGlobals->WdfLogHeader;
        dumpData->OutBufferLength  = logSize;
        dumpData->Guid = WdfDumpGuid;
    }
}

VOID
FxRegisterBugCheckCallback(
    __inout PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in    PDRIVER_OBJECT DriverObject
    )
{
    UNICODE_STRING funcName;
    PKBUGCHECK_REASON_CALLBACK_RECORD callbackRecord;
    PFN_KE_REGISTER_BUGCHECK_REASON_CALLBACK funcPtr;
    BOOLEAN enableDriverTracking;

    //
    // If any problem during this setup, disable driver tracking.
    //
    enableDriverTracking = FxDriverGlobals->FxTrackDriverForMiniDumpLog;
    FxDriverGlobals->FxTrackDriverForMiniDumpLog = FALSE;

    //
    // Zero out callback record.
    //
    callbackRecord = &FxDriverGlobals->BugCheckCallbackRecord;
    RtlZeroMemory(callbackRecord, sizeof(KBUGCHECK_REASON_CALLBACK_RECORD));

    //
    // Get the Image base address and size before registering the bugcheck
    // callbacks. If the image base address and size cannot be computed,
    // then the bugcheck callbacks depend on these values being properly
    // set.
    //
    FxDriverGlobals->ImageAddress = NULL;
    FxDriverGlobals->ImageSize = 0;

    if (!NT_SUCCESS(FxpGetImageBase(DriverObject,
                                   &FxDriverGlobals->ImageAddress,
                                   &FxDriverGlobals->ImageSize))) {
        goto Done;
    }










    //
    // The KeRegisterBugCheckReasonCallback exists for xp sp1 and above. So
    // check whether this function is defined on the current OS and register
    // for the bugcheck callback only if this function is defined.
    //
    RtlInitUnicodeString(&funcName, L"KeRegisterBugCheckReasonCallback");
    funcPtr = (PFN_KE_REGISTER_BUGCHECK_REASON_CALLBACK)
        MmGetSystemRoutineAddress(&funcName);

    if (NULL == funcPtr) {
        goto Done;
    }

    //
    // Register this driver with driver tracker.
    //
    if (enableDriverTracking) {
        if (NT_SUCCESS(FxLibraryGlobals.DriverTracker.Register(
                            FxDriverGlobals))) {
            FxDriverGlobals->FxTrackDriverForMiniDumpLog = TRUE;
        }
    }

    //
    // Initialize the callback record.
    //
    KeInitializeCallbackRecord(callbackRecord);

    //
    // Register the bugcheck callback.
    //
    funcPtr(callbackRecord,
            FxpBugCheckCallback,
            KbCallbackSecondaryDumpData,
            (PUCHAR)FxDriverGlobals->Public.DriverName);

    ASSERT(callbackRecord->CallbackRoutine != NULL);

Done:;
}

VOID
FxUnregisterBugCheckCallback(
    __inout PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    UNICODE_STRING funcName;
    PKBUGCHECK_REASON_CALLBACK_RECORD callbackRecord;
    PFN_KE_DEREGISTER_BUGCHECK_REASON_CALLBACK funcPtr;

    callbackRecord = &FxDriverGlobals->BugCheckCallbackRecord;
    if (NULL == callbackRecord->CallbackRoutine) {
        goto Done;
    }

    //
    // The KeDeregisterBugCheckReasonCallback exists for xp sp1 and above. So
    // check whether this function is defined on the current OS and deregister
    // from the bugcheck callback only if this function is defined.
    //
    RtlInitUnicodeString(&funcName, L"KeDeregisterBugCheckReasonCallback");
    funcPtr = (PFN_KE_DEREGISTER_BUGCHECK_REASON_CALLBACK)
        MmGetSystemRoutineAddress(&funcName);

    if (NULL == funcPtr) {
        goto Done;
    }

    funcPtr(callbackRecord);
    callbackRecord->CallbackRoutine = NULL;

    //
    // Deregister this driver with driver tracker.
    //
    if (FxDriverGlobals->FxTrackDriverForMiniDumpLog) {
        FxLibraryGlobals.DriverTracker.Deregister(FxDriverGlobals);
    }

Done:;
}

VOID
STDCALL
FxpLibraryBugCheckCallback(
    __in    KBUGCHECK_CALLBACK_REASON Reason,
    __in    PKBUGCHECK_REASON_CALLBACK_RECORD /* Record */,
    __inout PVOID ReasonSpecificData,
    __in    ULONG ReasonSpecificLength
    )

/*++

Routine Description:

    Global (framework-library) BugCheck callback routine for WDF

Arguments:

    Reason               - Must be KbCallbackSecondaryData
    Record               - Supplies the bugcheck record previously registered
    ReasonSpecificData   - Pointer to KBUGCHECK_SECONDARY_DUMP_DATA
    ReasonSpecificLength - Sizeof(ReasonSpecificData)

Return Value:

    None

Notes:
    When a bugcheck happens the kernel bugcheck processor will make two passes
    of all registered BugCheckCallbackRecord routines.  The first pass, called
    the "sizing pass" essentially queries all the callbacks to collect the
    total size of the secondary dump data. In the second pass the actual data
    is captured to the dump.

--*/

{
    PKBUGCHECK_SECONDARY_DUMP_DATA  dumpData;
    ULONG                           dumpSize;

    UNREFERENCED_PARAMETER(Reason);
    UNREFERENCED_PARAMETER(ReasonSpecificLength);

    ASSERT(ReasonSpecificLength >= sizeof(KBUGCHECK_SECONDARY_DUMP_DATA));
    ASSERT(Reason == KbCallbackSecondaryDumpData);

    dumpData = (PKBUGCHECK_SECONDARY_DUMP_DATA) ReasonSpecificData;
    dumpSize = FxLibraryGlobals.BugCheckDriverInfoIndex *
                sizeof(FX_DUMP_DRIVER_INFO_ENTRY);
    //
    // See if the bugcheck driver info is more than can fit in the dump
    //
    if (dumpData->MaximumAllowed < dumpSize) {
        dumpSize = EXP_ALIGN_DOWN_ON_BOUNDARY(
                        dumpData->MaximumAllowed,
                        sizeof(FX_DUMP_DRIVER_INFO_ENTRY));
    }

    if (0 == dumpSize) {
        goto Done;
    }

    //
    // Ok, provide the info about the bugcheck data.
    //
    dumpData->OutBuffer = FxLibraryGlobals.BugCheckDriverInfo;
    dumpData->OutBufferLength  = dumpSize;
    dumpData->Guid = WdfDumpGuid2;

Done:;
}

VOID
FxInitializeBugCheckDriverInfo()
{
    NTSTATUS                                 status;
    UNICODE_STRING                           funcName;
    PKBUGCHECK_REASON_CALLBACK_RECORD        callbackRecord;
    PFN_KE_REGISTER_BUGCHECK_REASON_CALLBACK funcPtr;
    SIZE_T                                   arraySize;
    ULONG                                    arrayCount;


    callbackRecord = &FxLibraryGlobals.BugCheckCallbackRecord;
    RtlZeroMemory(callbackRecord, sizeof(KBUGCHECK_REASON_CALLBACK_RECORD));

    FxLibraryGlobals.BugCheckDriverInfoCount = 0;
    FxLibraryGlobals.BugCheckDriverInfoIndex = 0;
    FxLibraryGlobals.BugCheckDriverInfo = NULL;










    //
    // The KeRegisterBugCheckReasonCallback exists for xp sp1 and above. So
    // check whether this function is defined on the current OS and register
    // for the bugcheck callback only if this function is defined.
    //
    RtlInitUnicodeString(&funcName, L"KeRegisterBugCheckReasonCallback");
    funcPtr = (PFN_KE_REGISTER_BUGCHECK_REASON_CALLBACK)
        MmGetSystemRoutineAddress(&funcName);

    if (NULL == funcPtr) {
        goto Done;
    }

    arraySize = sizeof(FX_DUMP_DRIVER_INFO_ENTRY) * FX_DUMP_DRIVER_INFO_INCREMENT;
    arrayCount = FX_DUMP_DRIVER_INFO_INCREMENT;

    FxLibraryGlobals.BugCheckDriverInfo =
        (PFX_DUMP_DRIVER_INFO_ENTRY)MxMemory::MxAllocatePoolWithTag(
                                    NonPagedPool,
                                    arraySize,
                                    FX_TAG);

    if (NULL == FxLibraryGlobals.BugCheckDriverInfo) {
        goto Done;
    }

    FxLibraryGlobals.BugCheckDriverInfoCount = arrayCount;

    //
    // Init first entry for the framework.
    //
    FxLibraryGlobals.BugCheckDriverInfo[0].FxDriverGlobals = NULL;
    FxLibraryGlobals.BugCheckDriverInfo[0].Version.Major = __WDF_MAJOR_VERSION;
    FxLibraryGlobals.BugCheckDriverInfo[0].Version.Minor = __WDF_MINOR_VERSION;
    FxLibraryGlobals.BugCheckDriverInfo[0].Version.Build = __WDF_BUILD_NUMBER;

    status = RtlStringCbCopyA(
                    FxLibraryGlobals.BugCheckDriverInfo[0].DriverName,
                    sizeof(FxLibraryGlobals.BugCheckDriverInfo[0].DriverName),
                    WdfLdrType);

    if (!NT_SUCCESS(status)) {
        ASSERT(FALSE);
        FxLibraryGlobals.BugCheckDriverInfo[0].DriverName[0] = '\0';
    }

    FxLibraryGlobals.BugCheckDriverInfoIndex++;

    //
    // Initialize the callback record.
    //
    KeInitializeCallbackRecord(callbackRecord);

    //
    // Register the bugcheck callback.
    //
    funcPtr(callbackRecord,
            FxpLibraryBugCheckCallback,
            KbCallbackSecondaryDumpData,
            (PUCHAR)WdfLdrType);

    ASSERT(callbackRecord->CallbackRoutine != NULL);

Done:;
}

VOID
FxUninitializeBugCheckDriverInfo()
{
    UNICODE_STRING                              funcName;
    PKBUGCHECK_REASON_CALLBACK_RECORD           callbackRecord;
    PFN_KE_DEREGISTER_BUGCHECK_REASON_CALLBACK  funcPtr;

    //
    // Deregister callback.
    //







    callbackRecord = &FxLibraryGlobals.BugCheckCallbackRecord;
    if (NULL == callbackRecord->CallbackRoutine) {
        goto Done;
    }

    //
    // The KeDeregisterBugCheckReasonCallback exists for xp sp1 and above. So
    // check whether this function is defined on the current OS and deregister
    // from the bugcheck callback only if this function is defined.
    //
    RtlInitUnicodeString(&funcName, L"KeDeregisterBugCheckReasonCallback");
    funcPtr = (PFN_KE_DEREGISTER_BUGCHECK_REASON_CALLBACK)
        MmGetSystemRoutineAddress(&funcName);

    if (NULL == funcPtr) {
        goto Done;
    }

    funcPtr(callbackRecord);
    callbackRecord->CallbackRoutine = NULL;

    //
    // Release memory.
    //
    if (NULL == FxLibraryGlobals.BugCheckDriverInfo) {
        goto Done;
    }

    //
    // Dynamic KMDF framework is unloading, make sure there is only one entry
    // left in the driver info array; the framework lib was using this entry
    // to store its version.
    //
    ASSERT(1 == FxLibraryGlobals.BugCheckDriverInfoIndex);

    FxLibraryGlobals.BugCheckDriverInfoIndex = 0;
    FxLibraryGlobals.BugCheckDriverInfoCount = 0;

    MxMemory::MxFreePool(FxLibraryGlobals.BugCheckDriverInfo);
    FxLibraryGlobals.BugCheckDriverInfo = NULL;

Done:;
}

VOID
FxCacheBugCheckDriverInfo(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    KIRQL                       irql;
    PFX_DUMP_DRIVER_INFO_ENTRY  driverInfo      = NULL;
    PFX_DUMP_DRIVER_INFO_ENTRY  oldDriverInfo   = NULL;
    ULONG                       newCount        = 0;

    //
    // Clear driver index (0-based). Drivers do not use index 0.
    //
    FxDriverGlobals->BugCheckDriverInfoIndex = 0;

    if (NULL == FxLibraryGlobals.BugCheckDriverInfo) {
        return;
    }

    FxLibraryGlobals.FxDriverGlobalsListLock.Acquire(&irql);

    //
    // Make sure we have enough space, else allocate some more memory.
    //
    if (FxLibraryGlobals.BugCheckDriverInfoIndex >=
            FxLibraryGlobals.BugCheckDriverInfoCount) {

        //
        // Just exit if no more space in dump buffer.
        //
        if ((FX_MAX_DUMP_DRIVER_INFO_COUNT - FX_DUMP_DRIVER_INFO_INCREMENT) <
             FxLibraryGlobals.BugCheckDriverInfoCount) {
             ASSERT(FALSE);
             goto Done;
        }

        newCount = FxLibraryGlobals.BugCheckDriverInfoCount +
                      FX_DUMP_DRIVER_INFO_INCREMENT;

        //
        // Allocate new buffer to hold driver info.
        //
        driverInfo = (PFX_DUMP_DRIVER_INFO_ENTRY)MxMemory::MxAllocatePoolWithTag(
                                NonPagedPool,
                                sizeof(FX_DUMP_DRIVER_INFO_ENTRY)* newCount,
                                FX_TAG);

        if (NULL == driverInfo) {
            goto Done;
        }

        //
        // Copy data from old to new buffer.
        //
        RtlCopyMemory(driverInfo,
                      FxLibraryGlobals.BugCheckDriverInfo,
                      FxLibraryGlobals.BugCheckDriverInfoCount *
                        sizeof(FX_DUMP_DRIVER_INFO_ENTRY));
        //
        // Ok, replace global pointer and its count.
        //
        oldDriverInfo = FxLibraryGlobals.BugCheckDriverInfo;
        FxLibraryGlobals.BugCheckDriverInfo = driverInfo;
        FxLibraryGlobals.BugCheckDriverInfoCount = newCount;
        driverInfo = NULL; // just in case.

        //
        // Free old memory.
        //
        MxMemory::MxFreePool(oldDriverInfo);
        oldDriverInfo = NULL;
    }

    ASSERT(FxLibraryGlobals.BugCheckDriverInfoIndex <
                FxLibraryGlobals.BugCheckDriverInfoCount);
    //
    // Compute ptr to free entry.
    //
    driverInfo = FxLibraryGlobals.BugCheckDriverInfo +
                    FxLibraryGlobals.BugCheckDriverInfoIndex;
    //
    // Cache some of this driver's info.
    //
    driverInfo->FxDriverGlobals = FxDriverGlobals;
    driverInfo->Version = FxDriverGlobals->WdfBindInfo->Version;

    C_ASSERT(sizeof(driverInfo->DriverName) ==
                sizeof(FxDriverGlobals->Public.DriverName));
    RtlCopyMemory(driverInfo->DriverName,
                  FxDriverGlobals->Public.DriverName,
                  sizeof(driverInfo->DriverName));
    driverInfo->DriverName[ARRAY_SIZE(driverInfo->DriverName) - 1] = '\0';

    //
    // Cache this index for later when the driver is removed.
    //
    FxDriverGlobals->BugCheckDriverInfoIndex =
        FxLibraryGlobals.BugCheckDriverInfoIndex;

    //
    // Update global index.
    //
    FxLibraryGlobals.BugCheckDriverInfoIndex++;

Done:

    FxLibraryGlobals.FxDriverGlobalsListLock.Release(irql);
}

VOID
FxPurgeBugCheckDriverInfo(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    KIRQL                       irql;
    PFX_DUMP_DRIVER_INFO_ENTRY  driverInfo  = NULL;
    ULONG                       driverIndex = 0;
    ULONG                       shiftCount  = 0;
    ULONG                       i           = 0;

    FxLibraryGlobals.FxDriverGlobalsListLock.Acquire(&irql);

    //
    // Zero means 'not used'.
    //
    driverIndex = FxDriverGlobals->BugCheckDriverInfoIndex;
    if (0 == driverIndex) {
        goto Done;
    }

    //
    // Check if feature is not supported.
    //
    if (NULL == FxLibraryGlobals.BugCheckDriverInfo) {
        ASSERT(FALSE); // driverIndex > 0 with NULL array?
        goto Done;
    }


    ASSERT(FxLibraryGlobals.BugCheckDriverInfoIndex <=
            FxLibraryGlobals.BugCheckDriverInfoCount);

    //
    // Index boundary validation.
    //
    if (driverIndex >= FxLibraryGlobals.BugCheckDriverInfoIndex) {
        ASSERT(FALSE);
        goto Done;
    }

    //
    // Compute ptr to driver info.
    //
    driverInfo = FxLibraryGlobals.BugCheckDriverInfo + driverIndex;

    //
    // Double-check that this is the same driver.
    //
    if (driverInfo->FxDriverGlobals != FxDriverGlobals) {
        ASSERT(FALSE);
        goto Done;
    }

    //
    // Shift memory to fill hole and update global free index.
    //
    shiftCount = FxLibraryGlobals.BugCheckDriverInfoIndex - driverIndex - 1;
    if (shiftCount > 0) {
        RtlMoveMemory(driverInfo,
                      driverInfo + 1,
                      shiftCount * sizeof(FX_DUMP_DRIVER_INFO_ENTRY));
    }

    FxLibraryGlobals.BugCheckDriverInfoIndex--;

    //
    // Update cached index for all 'shifted' drivers.
    //
    for (i = driverIndex; i < FxLibraryGlobals.BugCheckDriverInfoIndex; ++i) {
        FxLibraryGlobals.BugCheckDriverInfo[i].FxDriverGlobals->
                BugCheckDriverInfoIndex--;
    }

Done:

    FxLibraryGlobals.FxDriverGlobalsListLock.Release(irql);
}

//
// Driver usage tracker functions
//
NTSTATUS
FX_DRIVER_TRACKER_CACHE_AWARE::Initialize()
{
    //
    // Global initialization. It balances the global Uninitialize function.
    // The real init is actually done by the Register function the first time
    // a driver registers with the device tracker.
    //
    m_DriverUsage   = NULL;
    m_PoolToFree    = NULL;
    m_EntrySize     = 0;
    m_Number        = 0;

    return STATUS_SUCCESS;
}

VOID
FX_DRIVER_TRACKER_CACHE_AWARE::Uninitialize()
{
    if (m_PoolToFree != NULL) {

#ifdef DBG
    for (ULONG index = 0; index < m_Number; ++index) {
        PFX_DRIVER_TRACKER_ENTRY driverUsage = NULL;
        driverUsage = GetProcessorDriverEntryRef(index);
        ASSERT(NULL == driverUsage->FxDriverGlobals);
    }
#endif
        MxMemory::MxFreePool(m_PoolToFree);
        m_PoolToFree = NULL;
    }

    m_DriverUsage   = NULL;
    m_Number        = 0;
}

NTSTATUS
FX_DRIVER_TRACKER_CACHE_AWARE::Register(
    __in PFX_DRIVER_GLOBALS /* FxDriverGlobals */
    )
{
    NTSTATUS                    status      = STATUS_UNSUCCESSFUL;
    ULONG                       paddedSize  = 0;
    ULONG                       index       = 0;
    PFX_DRIVER_TRACKER_ENTRY    pool        = NULL;
    PFX_DRIVER_TRACKER_ENTRY    driverUsage = NULL;
    UNICODE_STRING              funcName;
    PVOID                       funcPtr     = NULL;

    //
    // Nothing to do if tracker is already initialized. No need for a lock
    // here since PnP already serializes driver initialization.
    //
    if (m_PoolToFree != NULL) {
        ASSERT(m_DriverUsage != NULL && m_Number != 0);
        status = STATUS_SUCCESS;
        goto Done;
    }

    //
    // Intialize the procgrp down level library.
    //
    // WdmlibProcgrpInitialize(); __REACTOS__ : haha we don't support ProcGrp

    //
    // Capture maximum number of processors.
    //
    RtlInitUnicodeString(&funcName, L"KeQueryMaximumProcessorCountEx");
    funcPtr = MmGetSystemRoutineAddress(&funcName);
    if (funcPtr != NULL) {
        //
        // Win 7 and forward.
        //
        m_Number = ((PFN_KE_QUERY_MAXIMUM_PROCESSOR_COUNT_EX)funcPtr)(
                            ALL_PROCESSOR_GROUPS);
    }
    else {
        RtlInitUnicodeString(&funcName, L"KeQueryMaximumProcessorCount");
        funcPtr = MmGetSystemRoutineAddress(&funcName);
        if (funcPtr != NULL) {
            //
            // Windows Server 2008.
            //
            m_Number = ((PFN_KE_QUERY_MAXIMUM_PROCESSOR_COUNT)funcPtr)();
        }
        else {
            if ((5 == FxLibraryGlobals.OsVersionInfo.dwMajorVersion &&
                 0 <  FxLibraryGlobals.OsVersionInfo.dwMinorVersion) ||
                (6 == FxLibraryGlobals.OsVersionInfo.dwMajorVersion &&
                 0 == FxLibraryGlobals.OsVersionInfo.dwMinorVersion)){
                //
                // XP (Major=5, Minor>0) and Vista (Major=6, Minor=0).
                //
                m_Number = (ULONG)(*((CCHAR *)&KeNumberProcessors));
            }
            else {
                //
                // This feature is not supported for Windows 2000.
                //
                ASSERT(FALSE);
                status = STATUS_NOT_SUPPORTED;
                goto Done;
            }
        }
    }

    //
    // Validate upper bound.
    //
    if (m_Number > FX_DRIVER_TRACKER_MAX_CPUS) {
        status = STATUS_NOT_SUPPORTED;
        goto Done;
    }

    //
    // Determine padded size of each tracking entry structure.
    //
    if (m_Number > 1 ) {
        RtlInitUnicodeString(&funcName, L"KeGetRecommendedSharedDataAlignment");
        funcPtr = MmGetSystemRoutineAddress(&funcName);

        if (funcPtr != NULL) {
            //
            //XP and forward
            //
            paddedSize = ((PFN_KE_GET_RECOMMENDED_SHARED_DATA_ALIGNMENT)funcPtr)();
            ASSERT ((paddedSize & (paddedSize - 1)) == 0);
        }
        else {
            //
            // This feature is not supported for Windows 2000.
            //
            status = STATUS_NOT_SUPPORTED;
            goto Done;
        }
    }
    else {
        paddedSize = sizeof(FX_DRIVER_TRACKER_ENTRY);
    }

    ASSERT(sizeof(FX_DRIVER_TRACKER_ENTRY) <= paddedSize);

    //
    // Remember the size.
    //
    m_EntrySize = paddedSize;

    pool = (PFX_DRIVER_TRACKER_ENTRY)MxMemory::MxAllocatePoolWithTag(
                                        NonPagedPool,
                                        paddedSize * m_Number,
                                        FX_TAG);
    if (NULL == pool) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    //
    // Check if pool is aligned if this is a multi-proc.
    //
    if ((m_Number > 1) && !EXP_IS_PTR_ALIGNED_ON_BOUNDARY(pool, paddedSize)) {
        //
        // We will allocate a padded size, free the old pool.
        //
        ExFreePool(pool);

        //
        // Allocate enough padding so we can start the refs on an aligned boundary
        //
        pool = (PFX_DRIVER_TRACKER_ENTRY)MxMemory::MxAllocatePoolWithTag(
                                        NonPagedPool,
                                        paddedSize * m_Number + paddedSize,
                                        FX_TAG);
        if (NULL == pool) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Done;
        }

        driverUsage = (PFX_DRIVER_TRACKER_ENTRY)
                            EXP_ALIGN_UP_PTR_ON_BOUNDARY(pool, paddedSize);

    }
    else {
        //
        // Already aligned pool.
        //
        driverUsage = pool;
    }

    //
    // Remember the pool block to free.
    //
    m_PoolToFree  = pool;
    m_DriverUsage = driverUsage;

    //
    // Init driver usage entries.
    //
    for (index = 0; index < m_Number; ++index) {
        driverUsage = GetProcessorDriverEntryRef(index);
        driverUsage->FxDriverGlobals = NULL;
    }

    //
    // All done.
    //
    status = STATUS_SUCCESS;

Done:
    return status;
}

VOID
FX_DRIVER_TRACKER_CACHE_AWARE::Deregister(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    ULONG                     index       = 0;
    PFX_DRIVER_TRACKER_ENTRY  driverUsage = NULL;

    //
    // Cleanup any refs to this driver's globals.
    //
    if (m_PoolToFree != NULL) {

        ASSERT(m_DriverUsage != NULL && m_Number != 0);

        for (index = 0; index < m_Number; ++index) {
            driverUsage = GetProcessorDriverEntryRef(index);
            if (driverUsage->FxDriverGlobals == FxDriverGlobals) {
                driverUsage->FxDriverGlobals = NULL;
            }
        }
    }
}

} // extern "C"
