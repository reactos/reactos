//
//    Copyright (C) Microsoft.  All rights reserved.
//

extern "C" {
#include <ntddk.h>
}

//
// This will cause inclusion of VfWdfFunctions table implementation from header
//
#define  VF_FX_DYNAMICS_GENERATE_TABLE   1

//
// Compute the length based on the max. service name length and the rest of the
// error string as seen in ReportDdiFunctionCountMismatch
//
#define EVTLOG_DDI_COUNT_ERROR_MAX_LEN (53 + MAX_PATH)

#include "fx.hpp"
#include "fxldr.h"
#include "fxlibrarycommon.h"
#include "fxtelemetry.hpp"
#include "wdfversionlog.h"
#include "minwindef.h"

extern "C" {
//
// Global triage Info for dbgeng and 0x9F work
//
static WDFOBJECT_TRIAGE_INFO       _WdfObjectTriageInfo = {0};
static WDFCONTEXT_TRIAGE_INFO      _WdfContextTriageInfo = {0};
static WDFCONTEXTTYPE_TRIAGE_INFO  _WdfContextTypeTriageInfo = {0};
static WDFQUEUE_TRIAGE_INFO        _WdfQueueTriageInfo = {0};
static WDFIRPQUEUE_TRIAGE_INFO     _WdfIrpQueueTriageInfo = {0};
static WDFREQUEST_TRIAGE_INFO      _WdfRequestTriageInfo = {0};
static WDFDEVICE_TRIAGE_INFO       _WdfDeviceTriageInfo = {0};
static WDFIRP_TRIAGE_INFO          _WdfIrpTriageInfo = {0};
static WDFFWDPROGRESS_TRIAGE_INFO  _WdfFwdProgressTriageInfo = {0};

WDF_TRIAGE_INFO g_WdfTriageInfo = {
    //
    // KMDF Version.
    //
    __WDF_MAJOR_VERSION,
    __WDF_MINOR_VERSION,

    //
    // Table Version.
    //
    WDF_01_TRIAGE_INFO_MAJOR_VERSION,
    WDF_01_TRIAGE_INFO_MINOR_VERSION,

    //
    // Reserved ptr (set to NULL).
    //
    NULL,

    //
    // WDF objects triage info.
    //
    &_WdfObjectTriageInfo,
    &_WdfContextTriageInfo,
    &_WdfContextTypeTriageInfo,
    &_WdfQueueTriageInfo,
    &_WdfFwdProgressTriageInfo,
    &_WdfIrpQueueTriageInfo,
    &_WdfRequestTriageInfo,
    &_WdfDeviceTriageInfo,
    &_WdfIrpTriageInfo,
};
} // extern "C"

VOID
GetTriageInfo(
    VOID
    )
{
    // Object
    _WdfObjectTriageInfo.RawObjectSize = sizeof(FxObject);
    _WdfObjectTriageInfo.ObjectType = FIELD_OFFSET(FxObject, m_Type);
    _WdfObjectTriageInfo.TotalObjectSize = FIELD_OFFSET(FxObject, m_ObjectSize);
    _WdfObjectTriageInfo.ChildListHead = FIELD_OFFSET(FxObject, m_ChildListHead);
    _WdfObjectTriageInfo.ChildEntry = FIELD_OFFSET(FxObject, m_ChildEntry);
    _WdfObjectTriageInfo.Globals = FIELD_OFFSET(FxObject, m_Globals);
    _WdfObjectTriageInfo.ParentObject = FIELD_OFFSET(FxObject, m_ParentObject);

    // Context Triage Info
    _WdfContextTriageInfo.HeaderSize = sizeof(FxContextHeader);
    _WdfContextTriageInfo.NextHeader = FIELD_OFFSET(FxContextHeader, NextHeader);
    _WdfContextTriageInfo.Object = FIELD_OFFSET(FxContextHeader, Object);
    _WdfContextTriageInfo.TypeInfoPtr = FIELD_OFFSET(FxContextHeader, ContextTypeInfo);
    _WdfContextTriageInfo.Context = FIELD_OFFSET(FxContextHeader, Context);

    // Context type Triage info
    _WdfContextTypeTriageInfo.TypeInfoSize = sizeof(WDF_OBJECT_CONTEXT_TYPE_INFO);
    _WdfContextTypeTriageInfo.ContextSize = FIELD_OFFSET(WDF_OBJECT_CONTEXT_TYPE_INFO, ContextSize);
    _WdfContextTypeTriageInfo.ContextName = FIELD_OFFSET(WDF_OBJECT_CONTEXT_TYPE_INFO, ContextName);

    // WdfRequest Queue
    _WdfQueueTriageInfo.QueueSize = sizeof(FxIoQueue);
    _WdfQueueTriageInfo.IrpQueue1 = FIELD_OFFSET(FxIoQueue, m_Queue);
    _WdfQueueTriageInfo.IrpQueue2 = FIELD_OFFSET(FxIoQueue, m_DriverCancelable);
    _WdfQueueTriageInfo.RequestList1 = FIELD_OFFSET(FxIoQueue, m_Cancelled);
    _WdfQueueTriageInfo.RequestList2 = FIELD_OFFSET(FxIoQueue, m_CanceledOnQueueList);
    _WdfQueueTriageInfo.FwdProgressContext = FIELD_OFFSET(FxIoQueue, m_FwdProgContext);
    _WdfQueueTriageInfo.PkgIo = FIELD_OFFSET(FxIoQueue, m_PkgIo);

    // Forward Progress
    _WdfFwdProgressTriageInfo.ReservedRequestList =
        FIELD_OFFSET(FXIO_FORWARD_PROGRESS_CONTEXT, m_ReservedRequestList);
    _WdfFwdProgressTriageInfo.ReservedRequestInUseList =
        FIELD_OFFSET(FXIO_FORWARD_PROGRESS_CONTEXT, m_ReservedRequestInUseList);
    _WdfFwdProgressTriageInfo.PendedIrpList =
        FIELD_OFFSET(FXIO_FORWARD_PROGRESS_CONTEXT, m_PendedIrpList);

    // Irp Queue
    _WdfIrpQueueTriageInfo.IrpQueueSize = sizeof(FxIrpQueue);
    _WdfIrpQueueTriageInfo.IrpListHeader = FIELD_OFFSET(FxIrpQueue, m_Queue);
    _WdfIrpQueueTriageInfo.IrpListEntry = FIELD_OFFSET(IRP, Tail.Overlay.ListEntry);
    _WdfIrpQueueTriageInfo.IrpContext = FIELD_OFFSET(IRP,
        Tail.Overlay.DriverContext[FX_IRP_QUEUE_CSQ_CONTEXT_ENTRY]);

    // WdfRequest
    _WdfRequestTriageInfo.RequestSize = sizeof(FxRequest);
    _WdfRequestTriageInfo.CsqContext = FIELD_OFFSET(FxRequest, m_CsqContext);
    _WdfRequestTriageInfo.FxIrp = FIELD_OFFSET(FxRequest, m_Irp);
    _WdfRequestTriageInfo.ListEntryQueueOwned =
        FIELD_OFFSET(FxRequest, m_OwnerListEntry);
    _WdfRequestTriageInfo.ListEntryQueueOwned2 =
        FIELD_OFFSET(FxRequest, m_OwnerListEntry2);
    _WdfRequestTriageInfo.RequestListEntry =
        FIELD_OFFSET(FxRequest, m_ListEntry);
    _WdfRequestTriageInfo.FwdProgressList =
        FIELD_OFFSET(FxRequest, m_ForwardProgressList);

    // WdfDevice
    _WdfDeviceTriageInfo.DeviceInitSize = sizeof(WDFDEVICE_INIT);
    _WdfDeviceTriageInfo.DeviceDriver = FIELD_OFFSET(FxDevice, m_Driver);

    // FxIrp
    _WdfIrpTriageInfo.FxIrpSize = sizeof(FxIrp);
    _WdfIrpTriageInfo.IrpPtr = FIELD_OFFSET(FxIrp, m_Irp);
}

BOOLEAN
IsClientInfoValid(
    _In_ PCLIENT_INFO ClientInfo
    )
{
    if (ClientInfo == NULL ||
        ClientInfo->Size != sizeof(CLIENT_INFO) ||
        ClientInfo->RegistryPath == NULL ||
        ClientInfo->RegistryPath->Length == 0 ||
        ClientInfo->RegistryPath->Buffer == NULL) {
        return FALSE;
    }
    return TRUE;
}

VOID
ReportDdiFunctionCountMismatch(
    _In_ PCUNICODE_STRING ServiceName,
    _In_ ULONG ActualFunctionCount,
    _In_ ULONG ExpectedFunctionCount
    )
{
    WCHAR    insertString[EVTLOG_DDI_COUNT_ERROR_MAX_LEN] = { 0 };
    NTSTATUS status;

    //
    // NOTE: Any single call to DbgPrintEx will only transmit 512 bytes of
    // information.
    //
    DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL,
            "\n\n************************* \n"
            "* DDI function table mismatch detected in KMDF driver. The \n"
            "* driver will not load until it is re-compiled using a \n"
            "* newer version of the Windows Driver Kit (WDK). \n"
            );

    DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL,
            "* Service name                 : %wZ\n"
            "* Actual function table count  : %d \n"
            "* Expected function table count: %d \n"
            "*************************** \n\n",
            ServiceName,
            ActualFunctionCount,
            ExpectedFunctionCount
            );

    //
    // Report a warning level ETW event to the system event log. "Wdf01000" is
    // the listed event provider.
    //
    status = RtlStringCchPrintfW(insertString,
                            RTL_NUMBER_OF(insertString),
                            L"Service:%wZ Count:Actual %d Expected %d",
                            ServiceName,
                            ActualFunctionCount,
                            ExpectedFunctionCount);
    if (NT_SUCCESS(status)) {
        LibraryLogEvent(FxLibraryGlobals.DriverObject,
                    WDFVER_CLIENT_INVALID_DDI_COUNT,
                    STATUS_INVALID_PARAMETER,
                    insertString,
                    NULL,
                    0);
    }

    //
    // Report a telemetry event that can be used to proactively fix drivers
    //
    TraceLoggingWrite(g_TelemetryProvider,
                    "KmdfClientFunctionCountMismatch",
                    WDF_TELEMETRY_EVT_KEYWORDS,
                    TraceLoggingUnicodeString(ServiceName, "ServiceName"),
                    TraceLoggingUInt32(ActualFunctionCount, "FunctionCount"),
                    TraceLoggingUInt32(ExpectedFunctionCount, "ExpectedCount"));
}

_Must_inspect_result_
NTSTATUS
FxLibraryCommonCommission(
    VOID
    )
{
    DECLARE_CONST_UNICODE_STRING(usName, L"RtlGetVersion");
    PFN_RTL_GET_VERSION pRtlGetVersion = NULL;
    NTSTATUS   status;

    __Print((LITERAL(WDF_LIBRARY_COMMISSION) "\n"));

    //
    // Commission this version's DLL globals.
    //
    status = FxLibraryGlobalsCommission();

    if (!NT_SUCCESS(status)) {
        __Print(("FxLibraryGlobalsCommission failed %X\n", status));
        return status;
    }

    //
    // register telemetry provider.
    //
    RegisterTelemetryProvider();

    //
    // Initialize internal WPP tracing.
    //
    status = FxTraceInitialize();
    if (NT_SUCCESS(status)) {
        FxLibraryGlobals.InternalTracingInitialized = TRUE;
    }
    else {
        __Print(("Failed to initialize tracing for WDF\n"));

        //
        // Failure to initialize is not critical enough to fail driver load.
        //
        status = STATUS_SUCCESS;
    }

    //
    // Attempt to load RtlGetVersion (works for > w2k).
    //
    pRtlGetVersion = (PFN_RTL_GET_VERSION) MmGetSystemRoutineAddress(
        (PUNICODE_STRING) &usName
        );

    //
    // Now attempt to get this OS's version.
    //
    if (pRtlGetVersion != NULL) {
        pRtlGetVersion(&gOsVersion);
    }

    __Print(("OsVersion(%d.%d)\n",
             gOsVersion.dwMajorVersion,
             gOsVersion.dwMinorVersion ));

    //
    // Init triage info for 9f bugcheck analysis.
    //
    GetTriageInfo();

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxLibraryCommonDecommission(
    VOID
    )
{
    __Print((LITERAL(WDF_LIBRARY_DECOMMISSION) ": enter\n"));

    //
    // Uninitialize WPP tracing.
    //
    if (FxLibraryGlobals.InternalTracingInitialized) {
        TraceUninitialize();
        FxLibraryGlobals.InternalTracingInitialized = FALSE;
    }

    //
    // Unregister telemetry provider.
    //
    UnregisterTelemetryProvider();

    EventUnregisterMicrosoft_Windows_DriverFrameworks_KernelMode_Performance();

    //
    // Decommission this version's DLL globals.
    //
    FxLibraryGlobalsDecommission();

    //
    // Note: This is the absolute last action from WDF library (dynamic or static).
    //       The image is likely to be deleted after returning.
    //
    __Print((LITERAL(WDF_LIBRARY_DECOMMISSION) ": exit\n"));

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxLibraryCommonRegisterClient(
    __inout PWDF_BIND_INFO        Info,
    __deref_out PWDF_DRIVER_GLOBALS *WdfDriverGlobals,
    __in_opt PCLIENT_INFO          ClientInfo
    )
{
    NTSTATUS           status;
    UNICODE_STRING serviceName = { 0 };

    status = STATUS_INVALID_PARAMETER;

    __Print((LITERAL(WDF_LIBRARY_REGISTER_CLIENT) ": enter\n"));

    if (Info == NULL || WdfDriverGlobals == NULL || Info->FuncTable == NULL) {
        __Print((LITERAL(WDF_LIBRARY_REGISTER_CLIENT)
                 ": NULL parameter -- %s\n",
                 (Info == NULL)             ? "PWDF_BIND_INFO" :
                 (WdfDriverGlobals == NULL) ? "PWDF_DRIVER_GLOBALS *" :
                 (Info->FuncTable == NULL)  ? "PWDF_BIND_INFO->FuncTable" :
                                              "unknown" ));
        goto Done;
    }

    ASSERT(Info->FuncCount);


    *WdfDriverGlobals = NULL;

    //
    // WdfVersion.Count is initialized in FxDynamics.h and is never changed.
    // Prefast is unable to make that determination.
    //
    __assume(WdfVersion.FuncCount == sizeof(WDFFUNCTIONS)/sizeof(PVOID));

    if (Info->FuncCount > WdfVersion.FuncCount) {
        __Print((LITERAL(WDF_LIBRARY_REGISTER_CLIENT)
                 ": version mismatch detected in function table count: client"
                 "has 0x%x,  library has 0x%x\n",
                 Info->FuncCount, WdfVersion.FuncCount));
        goto Done;
    }

    if (Info->FuncCount <= WdfFunctionTableNumEntries_V1_15) {
        //
        // Make sure table count matches exactly with previously
        // released framework version table sizes.
        //
        switch (Info->FuncCount) {

        case WdfFunctionTableNumEntries_V1_15:
        case WdfFunctionTableNumEntries_V1_13:
        case WdfFunctionTableNumEntries_V1_11:
        case WdfFunctionTableNumEntries_V1_9:
     // case WdfFunctionTableNumEntries_V1_7:  // both 1.7 and 1.5 have 387 functions
        case WdfFunctionTableNumEntries_V1_5:
        case WdfFunctionTableNumEntries_V1_1:
        case WdfFunctionTableNumEntries_V1_0:
            break;

        default:
            __Print((LITERAL(WDF_LIBRARY_REGISTER_CLIENT)
                     ": Function table count 0x%x doesn't match any previously "
                     "released framework version table size\n",
                     Info->FuncCount));
            goto Done;
        }
    }
    else {








        // Client version is same as framework version. Make
        // sure table count is exact.
        if (Info->FuncCount != WdfFunctionTableNumEntries) {
            RtlZeroMemory(&serviceName, sizeof(UNICODE_STRING));

            if (IsClientInfoValid(ClientInfo)) {
                GetNameFromPath(ClientInfo->RegistryPath, &serviceName);
            }
            else {
                RtlInitUnicodeString(&serviceName, L"Unknown");
            }

            //
            // Report a DbgPrint message, telemetry event and an ETW event that
            // will serve as diagnostic aid.
            //
            ReportDdiFunctionCountMismatch((PCUNICODE_STRING)&serviceName,
                                        Info->FuncCount,
                                        WdfFunctionTableNumEntries);

            //
            // If loader diagnostics are enabled and KD is connected, break-in
            //
            if (WdfLdrDbgPrintOn && KD_DEBUGGER_ENABLED &&
                !KD_DEBUGGER_NOT_PRESENT) {
                DbgBreakPoint();
            }
            goto Done;
        }
    }

    //
    // Allocate an new FxDriverGlobals area for this driver.
    //
    *WdfDriverGlobals = FxAllocateDriverGlobals();

    if (*WdfDriverGlobals) {
        BOOLEAN isFunctinTableHookingOn  = FALSE;
        BOOLEAN isPerformanceAnalysisOn  = FALSE;
        PFX_DRIVER_GLOBALS fxDriverGlobals = NULL;

        //
        // Check the registry to see if Enhanced verifier is on for this driver.
        // if registry read fails, options value remains unchanged.
        // store enhanced verifier options in driver globals
        //
        fxDriverGlobals = GetFxDriverGlobals(*WdfDriverGlobals);
        GetEnhancedVerifierOptions(ClientInfo, &fxDriverGlobals->FxEnhancedVerifierOptions);
        isFunctinTableHookingOn = IsFxVerifierFunctionTableHooking(fxDriverGlobals);
        isPerformanceAnalysisOn = IsFxPerformanceAnalysis(fxDriverGlobals);

        //
        // Set-up the function table. Enhanced verifier and Performance analysis is off by default.
        //
        if (isFunctinTableHookingOn == FALSE && isPerformanceAnalysisOn == FALSE) {

            //
            // Starting in 1.15 we reference a copy of the DDI table in WDF01000,
            // prior to that we copy the entire table to local memory.
            //
            if (Info->FuncCount <= WdfFunctionTableNumEntries_V1_13) {
                RtlCopyMemory( Info->FuncTable,
                               &WdfVersion.Functions,
                               Info->FuncCount * sizeof(PVOID) );
            }
            else {
                //
                // FuncTable arrives with a ptr to &WdfFunctions, so we update
                // what WdfFunctions points to.
                //
                *((WDFFUNC**) Info->FuncTable) = (WDFFUNC*) &WdfVersion.Functions;
            }
        }
        else {
            __Print((LITERAL(WDF_LIBRARY_REGISTER_CLIENT)
                     ": Enhanced Verification is ON \n"));

            LockVerifierSection(fxDriverGlobals, ClientInfo->RegistryPath);

            if (Microsoft_Windows_DriverFrameworks_KernelMode_PerformanceHandle == NULL) {
                EventRegisterMicrosoft_Windows_DriverFrameworks_KernelMode_Performance();
            }

            //
            // Enhanced verification is on. Return verifier function table
            //
            // Starting in 1.15 we reference a copy of the DDI table in WDF01000,
            // prior to that we copy the entire table to local memory.
            //
            if (Info->FuncCount <= WdfFunctionTableNumEntries_V1_13) {
                RtlCopyMemory( Info->FuncTable,
                               &VfWdfVersion.Functions,
                               Info->FuncCount * sizeof(PVOID) );
            }
            else {
                //
                // FuncTable arrives with a ptr to &WdfFunctions, so we update
                // what WdfFunctions points to.
                //
                *((WDFFUNC**) Info->FuncTable) = (WDFFUNC*) &VfWdfVersion.Functions;
            }
        }

        status = STATUS_SUCCESS;

        __Print((LITERAL(WDF_LIBRARY_REGISTER_CLIENT)
                 ": WdfFunctions %p\n", Info->FuncTable));
    }

Done:
    __Print((LITERAL(WDF_LIBRARY_REGISTER_CLIENT)
             ": exit: status %X\n", status));

    return status;
}

_Must_inspect_result_
NTSTATUS
FxLibraryCommonUnregisterClient(
    __in PWDF_BIND_INFO        Info,
    __in PWDF_DRIVER_GLOBALS   WdfDriverGlobals
    )
{
    NTSTATUS status;

    __Print((LITERAL(WDF_LIBRARY_UNREGISTER_CLIENT) ": enter\n"));

    ASSERT(Info);
    ASSERT(WdfDriverGlobals);

    if (Info != NULL && WdfDriverGlobals != NULL) {
        PFX_DRIVER_GLOBALS pFxDriverGlobals;

        status = STATUS_SUCCESS;

        pFxDriverGlobals = GetFxDriverGlobals(WdfDriverGlobals);

        //
        // Destroy this FxDriver instance, if its still indicated.
        //
        if (pFxDriverGlobals->Driver != NULL) {
            //
            // Association support, we are a root with no parent
            //
            pFxDriverGlobals->Driver->DeleteObject();

            FxDestroy(pFxDriverGlobals);
        }

        //
        // Stop IFR logging
        //
        FxIFRStop(pFxDriverGlobals);

        //
        // unlock enhanced-verifier image sections
        //
        if (IsFxVerifierFunctionTableHooking(pFxDriverGlobals)) {
            UnlockVerifierSection(pFxDriverGlobals);
        }

        //
        // This will free the client's FxDriverGlobals area
        //
        FxFreeDriverGlobals(WdfDriverGlobals);
    }
    else {
        status = STATUS_UNSUCCESSFUL;
    }

    __Print((LITERAL(WDF_LIBRARY_UNREGISTER_CLIENT)
             ": exit: status %X\n", status));

    return status;
}

VOID
GetEnhancedVerifierOptions(
    __in PCLIENT_INFO ClientInfo,
    __out PULONG Options
    )
{
    NTSTATUS status;
    ULONG value;
    FxAutoRegKey hKey, hWdf;
    DECLARE_CONST_UNICODE_STRING(parametersPath, L"Parameters\\Wdf");
    DECLARE_CONST_UNICODE_STRING(valueName, WDF_ENHANCED_VERIFIER_OPTIONS_VALUE_NAME);

    *Options = 0;
    if (!IsClientInfoValid(ClientInfo) ||
        Options == NULL) {

        __Print((LITERAL(WDF_LIBRARY_REGISTER_CLIENT)
                 ": Invalid ClientInfo received from wdfldr \n"));
        return;
    }

    status = FxRegKey::_OpenKey(NULL,
                                ClientInfo->RegistryPath,
                                &hWdf.m_Key,
                                KEY_READ);
    if (!NT_SUCCESS(status)) {
        return;
    }

    status = FxRegKey::_OpenKey(hWdf.m_Key,
                                &parametersPath,
                                &hKey.m_Key,
                                KEY_READ);
    if (!NT_SUCCESS(status)) {
        return;
    }

    status = FxRegKey::_QueryULong(
        hKey.m_Key, &valueName, &value);

    //
    // Examine key values and set Options only on success.
    //
    if (NT_SUCCESS(status)) {
        if (value) {
            *Options = value;
        }
    }
}

VOID
LibraryLogEvent(
    __in PDRIVER_OBJECT DriverObject,
    __in NTSTATUS       ErrorCode,
    __in NTSTATUS       FinalStatus,
    __in PWSTR          ErrorInsertionString,
    __in_bcount(RawDataLen) PVOID    RawDataBuf,
    __in USHORT         RawDataLen
    )
/*++


Routine Description:

    Logs an error to the system event log.

    Arguments:

    DriverObject - Pointer to driver object reporting the error.

    ErrorCode    - Indicates the type of error, system or driver-defined.

    ErrorInsertionString - Null-terminated Unicode string inserted into error
    description, as defined by error code.

Return Value:

None.

--*/
{
    PIO_ERROR_LOG_PACKET errorLogEntry;
    size_t               errorLogEntrySize;                  // [including null]
    size_t               errorInsertionStringByteSize = 0;

    if (ErrorInsertionString) {
        errorInsertionStringByteSize = wcslen(ErrorInsertionString) * sizeof(WCHAR);
        errorInsertionStringByteSize += sizeof(UNICODE_NULL);
    }

    errorLogEntrySize = sizeof(IO_ERROR_LOG_PACKET) + RawDataLen + errorInsertionStringByteSize;

    //
    // Log an error.
    //
    //
    // prefast complains about comparison of constant with constant here
    //
#pragma prefast(suppress:__WARNING_CONST_CONST_COMP, "If ErrorInsertionString is not null then this is not a constant")
    if (errorLogEntrySize <= ERROR_LOG_MAXIMUM_SIZE) {

        errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(DriverObject,
            (UCHAR)errorLogEntrySize);

        if (errorLogEntry != NULL) {

            RtlZeroMemory(errorLogEntry, errorLogEntrySize);

            errorLogEntry->ErrorCode = ErrorCode;
            errorLogEntry->FinalStatus = FinalStatus;
            errorLogEntry->NumberOfStrings = (ErrorInsertionString) ? 1 : 0;
            errorLogEntry->DumpDataSize = RawDataLen;
            errorLogEntry->StringOffset = (FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData)) + errorLogEntry->DumpDataSize;

            //
            // Insertion strings follow dumpdata and since there is no dumpdata we place the
            // insertion string at the start offset of the dumpdata.
            //
            if (RawDataBuf) {
                RtlCopyMemory(errorLogEntry->DumpData,
                    RawDataBuf,
                    RawDataLen);
            }

            if (ErrorInsertionString) {
                RtlCopyMemory(((PCHAR)errorLogEntry->DumpData) + RawDataLen,
                    ErrorInsertionString,
                    errorInsertionStringByteSize);
            }

            IoWriteErrorLogEntry(errorLogEntry);
        }
    }

    return;
}
