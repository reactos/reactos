#include "fxbugcheck.h"
#include <ntstrsafe.h>
#include "common/fxglobals.h"
#include "common/fxifr.h"
#include "common/mxmemory.h"
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
        
//#ifdef DBG
//    for (ULONG index = 0; index < m_Number; ++index)
//    {
//        PFX_DRIVER_TRACKER_ENTRY driverUsage = NULL;
//        driverUsage = GetProcessorDriverEntryRef(index);
//        ASSERT(NULL == driverUsage->FxDriverGlobals);
//    }
//#endif
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

    //                                                                      'void (*)(KBUGCHECK_CALLBACK_REASON, PKBUGCHECK_REASON_CALLBACK_RECORD, PVOID, ULONG)
    //                                                                 {aka void (*)(_KBUGCHECK_CALLBACK_REASON, _KBUGCHECK_REASON_CALLBACK_RECORD*, void*, long unsigned int)}'
    //'PKBUGCHECK_REASON_CALLBACK_ROUTINE {aka void (__attribute__((__stdcall__)) *)(_KBUGCHECK_CALLBACK_REASON, _KBUGCHECK_REASON_CALLBACK_RECORD*, void*, long unsigned int)}'

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


}
