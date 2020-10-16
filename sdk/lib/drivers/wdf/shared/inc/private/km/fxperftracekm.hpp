/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxPerfTrace.hpp

Abstract:

    This is header file for perf trace methods.

Author:



Environment:

    Kernel mode only

Revision History:

Notes:

--*/

#pragma once

//
// Version starts from 2 to be in sync with ETW versioning
//
#define WDF_DPC_EVENT_VERSION_2         2
#define WDF_INTERRUPT_EVENT_VERSION_2   2
#define WDF_WORK_ITEM_EVENT_VERSION_2   2

// __REACTOS__ : functions are commented out

FORCEINLINE
VOID
FxPerfTraceDpc(
    _In_ PVOID DriverCallback
    )
{
    // PWMI_WDF_NOTIFY_ROUTINE perfTraceCallback = NULL;

    // //
    // // Trace driver's ISR using perf trace callback. If the perf trace callback
    // // is NULL, it means either perf tracing is not enabled, or this OS
    // // doesn't support perf tracing for WDF (note only win8+ supports WDF perf
    // // trace callbacks).
    // //
    // perfTraceCallback = FxLibraryGlobals.PerfTraceRoutines->DpcNotifyRoutine;
    // if (perfTraceCallback != NULL) {
    //     (perfTraceCallback) (DriverCallback,            // event data
    //                          sizeof(PVOID),             // sizeof event
    //                          PERF_WDF_DPC,              // group mask
    //                          PERFINFO_LOG_TYPE_WDF_DPC, // hook id
    //                          WDF_DPC_EVENT_VERSION_2    // version
    //                          );
    // }
}

FORCEINLINE
VOID
FxPerfTraceInterrupt(
    _In_ PVOID DriverCallback
    )
{
    // PWMI_WDF_NOTIFY_ROUTINE perfTraceCallback = NULL;

    // perfTraceCallback = FxLibraryGlobals.PerfTraceRoutines->InterruptNotifyRoutine;
    // if (perfTraceCallback != NULL) {
    //     (perfTraceCallback) (DriverCallback,           // event data
    //                          sizeof(PVOID),            // sizeof event
    //                          PERF_WDF_INTERRUPT,       // group mask
    //                          PERFINFO_LOG_TYPE_WDF_INTERRUPT, // hook id
    //                          WDF_INTERRUPT_EVENT_VERSION_2    // version
    //                          );
    // }
}

FORCEINLINE
VOID
FxPerfTracePassiveInterrupt(
    _In_ PVOID DriverCallback
    )
{
    // PWMI_WDF_NOTIFY_ROUTINE perfTraceCallback = NULL;

    // perfTraceCallback = FxLibraryGlobals.PerfTraceRoutines->InterruptNotifyRoutine;
    // if (perfTraceCallback != NULL) {
    //     (perfTraceCallback) (DriverCallback,
    //                          sizeof(PVOID),
    //                          PERF_WDF_INTERRUPT,
    //                          PERFINFO_LOG_TYPE_WDF_PASSIVE_INTERRUPT,
    //                          WDF_INTERRUPT_EVENT_VERSION_2
    //                          );
    // }
}

FORCEINLINE
VOID
FxPerfTraceWorkItem(
    _In_ PVOID DriverCallback
    )
{
    // PWMI_WDF_NOTIFY_ROUTINE perfTraceCallback = NULL;

    // perfTraceCallback = FxLibraryGlobals.PerfTraceRoutines->WorkItemNotifyRoutine;
    // if (perfTraceCallback != NULL) {
    //     (perfTraceCallback) (DriverCallback,
    //                          sizeof(PVOID),
    //                          PERF_WORKER_THREAD,
    //                          PERFINFO_LOG_TYPE_WDF_WORK_ITEM,
    //                          WDF_WORK_ITEM_EVENT_VERSION_2
    //                          );
    // }
}

