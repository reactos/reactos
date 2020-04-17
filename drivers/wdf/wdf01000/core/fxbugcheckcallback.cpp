#include "fxbugcheck.h"
#include <ntstrsafe.h>
#include "common/fxglobals.h"
#include "common/fxifr.h"
#include "primitives/mxmemory.h"
#include "common/fxmacros.h"

extern "C" {

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
    if (m_PoolToFree != NULL)
    {
        
#ifdef DBG
    for (ULONG index = 0; index < m_Number; ++index)
    {
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

VOID
NTAPI
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
    if (dumpData->MaximumAllowed < dumpSize)
    {
        dumpSize = EXP_ALIGN_DOWN_ON_BOUNDARY( 
                        dumpData->MaximumAllowed,
                        sizeof(FX_DUMP_DRIVER_INFO_ENTRY));
    }
    
    //
    // Ok, provide the info about the bugcheck data.
    //
    dumpData->OutBuffer = FxLibraryGlobals.BugCheckDriverInfo;
    dumpData->OutBufferLength  = dumpSize;
    dumpData->Guid = WdfDumpGuid2;

//Done:;
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
    
    if (FxLibraryGlobals.StaticallyLinked)
    {
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

    if (NULL == funcPtr)
    {
        goto Done;
    }
    
    arraySize = sizeof(FX_DUMP_DRIVER_INFO_ENTRY) * FX_DUMP_DRIVER_INFO_INCREMENT;
    arrayCount = FX_DUMP_DRIVER_INFO_INCREMENT;

    FxLibraryGlobals.BugCheckDriverInfo = 
        (PFX_DUMP_DRIVER_INFO_ENTRY)MxMemory::MxAllocatePoolWithTag(
                                    NonPagedPool,
                                    arraySize,
                                    FX_TAG);
    
    if (NULL == FxLibraryGlobals.BugCheckDriverInfo)
    {
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
    
    if (!NT_SUCCESS(status))
    {
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
    if (NULL == callbackRecord->CallbackRoutine)
    {
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

    if (NULL == funcPtr)
    {
        goto Done;
    }

    funcPtr(callbackRecord);
    callbackRecord->CallbackRoutine = NULL;

    //
    // Release memory.
    //
    if (NULL == FxLibraryGlobals.BugCheckDriverInfo)
    {
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
FxUnregisterBugCheckCallback(
    __inout PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    UNICODE_STRING funcName;
    PKBUGCHECK_REASON_CALLBACK_RECORD callbackRecord;
    PFN_KE_DEREGISTER_BUGCHECK_REASON_CALLBACK funcPtr;

    callbackRecord = &FxDriverGlobals->BugCheckCallbackRecord;
    if (NULL == callbackRecord->CallbackRoutine)
    {
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

    if (NULL == funcPtr)
    {
        goto Done;
    }
    
    funcPtr(callbackRecord);
    callbackRecord->CallbackRoutine = NULL;

    //
    // Deregister this driver with driver tracker.
    //
    if (FxDriverGlobals->FxTrackDriverForMiniDumpLog)
    {
        FxLibraryGlobals.DriverTracker.Deregister(FxDriverGlobals);
    }

Done:;
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
    if (m_PoolToFree != NULL)
    {
        
        ASSERT(m_DriverUsage != NULL && m_Number != 0);
   
        for (index = 0; index < m_Number; ++index)
        {
            driverUsage = GetProcessorDriverEntryRef(index);
            if (driverUsage->FxDriverGlobals == FxDriverGlobals)
            {
                driverUsage->FxDriverGlobals = NULL;
            }
        }
    }
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
    if (0 == driverIndex)
    {
        goto Done;
    }

    //
    // Check if feature is not supported.
    //
    if (NULL == FxLibraryGlobals.BugCheckDriverInfo)
    {
        ASSERT(FALSE); // driverIndex > 0 with NULL array?
        goto Done;
    }
    
    
    ASSERT(FxLibraryGlobals.BugCheckDriverInfoIndex <= 
            FxLibraryGlobals.BugCheckDriverInfoCount);

    //
    // Index boundary validation. 
    //
    if (driverIndex >= FxLibraryGlobals.BugCheckDriverInfoIndex)
    {
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
    if (driverInfo->FxDriverGlobals != FxDriverGlobals)
    {
        ASSERT(FALSE);
        goto Done;
    }

    //
    // Shift memory to fill hole and update global free index.
    //
    shiftCount = FxLibraryGlobals.BugCheckDriverInfoIndex - driverIndex - 1;
    if (shiftCount > 0)
    {
        RtlMoveMemory(driverInfo, 
                      driverInfo + 1, 
                      shiftCount * sizeof(FX_DUMP_DRIVER_INFO_ENTRY));
    }

    FxLibraryGlobals.BugCheckDriverInfoIndex--;

    //
    // Update cached index for all 'shifted' drivers.
    //
    for (i = driverIndex; i < FxLibraryGlobals.BugCheckDriverInfoIndex; ++i)
    {
        FxLibraryGlobals.BugCheckDriverInfo[i].FxDriverGlobals->
                BugCheckDriverInfoIndex--;
    }
    
Done:
    
    FxLibraryGlobals.FxDriverGlobalsListLock.Release(irql);
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

    if (NULL == FxLibraryGlobals.BugCheckDriverInfo)
    {
        return;
    }

    FxLibraryGlobals.FxDriverGlobalsListLock.Acquire(&irql);

    //
    // Make sure we have enough space, else allocate some more memory.
    //
    if (FxLibraryGlobals.BugCheckDriverInfoIndex >=
            FxLibraryGlobals.BugCheckDriverInfoCount)
    {

        //
        // Just exit if no more space in dump buffer.
        //
        if ((FX_MAX_DUMP_DRIVER_INFO_COUNT - FX_DUMP_DRIVER_INFO_INCREMENT) <
             FxLibraryGlobals.BugCheckDriverInfoCount)
        {
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
        
        if (NULL == driverInfo)
        {
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

_Must_inspect_result_
NTSTATUS
FxpGetImageBase(
    __in  PDRIVER_OBJECT DriverObject,
    __out PVOID* ImageBase,
    __out PULONG ImageSize
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    //ULONG modulesSize = 0;
    //AUX_MODULE_EXTENDED_INFO* modules = NULL;
    //AUX_MODULE_EXTENDED_INFO* module;
    PVOID addressInImage = NULL;
    ULONG numberOfModules;
    ULONG i;

    //
    // Basic validation.
    //
    if (NULL == DriverObject || NULL == ImageBase || NULL == ImageSize)
    {
        status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    //
    // Get the address of a well known entry in the Image.
    //
    addressInImage = (PVOID) DriverObject->DriverStart;
    ASSERT(addressInImage != NULL);

    //
    // Initialize the AUX Kernel Library.
    //
    status = STATUS_UNSUCCESSFUL;

    //
    // TODO: Implement Auxiliary Kernel-Mode Library
    //

    //status = AuxKlibInitialize();
    if (!NT_SUCCESS(status))
    {
        goto exit;
    }

    //
    // Get size of area needed for loaded modules.
    //
    /*status = AuxKlibQueryModuleInformation(&modulesSize,
                                           sizeof(AUX_MODULE_EXTENDED_INFO),
                                           NULL);

    if (!NT_SUCCESS(status) || (0 == modulesSize))
    {
        goto exit;
    }

    numberOfModules = modulesSize / sizeof(AUX_MODULE_EXTENDED_INFO);

    //
    // Allocate returned-sized memory for the modules area.
    //
    modules = (AUX_MODULE_EXTENDED_INFO*) ExAllocatePoolWithTag(PagedPool,
                                                                modulesSize,
                                                                '30LW');
    if (NULL == modules)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    //
    // Request the modules array be filled with module information.
    //
    status = AuxKlibQueryModuleInformation(&modulesSize,
                                           sizeof(AUX_MODULE_EXTENDED_INFO),
                                           modules);

    if (!NT_SUCCESS(status))
    {
        goto exit;
    }

    //
    // Traverse list, searching for the well known address in Image for which the
    // module's Image Base Address is in its range.
    //
    module = modules;

    for (i=0; i < numberOfModules; i++)
    {

        if (addressInImage >= module->BasicInfo.ImageBase &&
            addressInImage < WDF_PTR_ADD_OFFSET(module->BasicInfo.ImageBase,
                                                module->ImageSize))
        {

            *ImageBase = module->BasicInfo.ImageBase;
            *ImageSize = module->ImageSize;

            status = STATUS_SUCCESS;
            goto exit;
        }
        module++;
    }

    status = STATUS_NOT_FOUND;*/

exit:

    /*if (modules != NULL)
    {
        ExFreePool(modules);
        modules = NULL;
    }*/

    return status;
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
                                   &FxDriverGlobals->ImageSize)))
    {
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

    if (NULL == funcPtr)
    {
        goto Done;
    }

    //
    // TODO: Need implement Processor Group (ProcGrp) compatibility library.
    //       Used in DriverTracker.Register
    //

    goto Done;

    //
    // Register this driver with driver tracker.
    //
    //if (enableDriverTracking)
    //{
    //    if (NT_SUCCESS(FxLibraryGlobals.DriverTracker.Register(
    //                        FxDriverGlobals)))
    //    {
    //        FxDriverGlobals->FxTrackDriverForMiniDumpLog = TRUE;
    //    }
    //}
    
    //
    // Initialize the callback record.
    //
    //KeInitializeCallbackRecord(callbackRecord);

    //
    // Register the bugcheck callback.
    //
    //funcPtr(callbackRecord,
    //        FxpBugCheckCallback,
    //        KbCallbackSecondaryDumpData,
    //        (PUCHAR)FxDriverGlobals->Public.DriverName);

    //ASSERT(callbackRecord->CallbackRoutine != NULL);

Done:;
}


}
