/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxVerifier.cpp

Abstract:

    This is the main driver framework verifier

Environment:

    kernel/user mode

Revision History:


        Made it mode agnostic

--*/

#ifndef _FXVERIFIER_H_
#define _FXVERIFIER_H_

extern "C" {
#if defined(EVENT_TRACING)
#include "FxVerifier.h.tmh"
#endif
}


enum FxEnhancedVerifierBitFlags {
    //
    // low 2 bytes are used for function table Hooking
    //
    FxEnhancedVerifierCallbackIrqlAndCRCheck      = 0x00000001,
    //
    // Lower nibble of 3rd byte  for forward progress
    //
    FxEnhancedVerifierForwardProgressFailAll      = 0x00010000,
    FxEnhancedVerifierForwardProgressFailRandom   = 0x00020000,

    //
    // bit masks
    //
    FxEnhancedVerifierFunctionTableHookMask       = 0x0000ffff,
    FxEnhancedVerifierForwardProgressMask         = 0x000f0000,

    //
    // higher nibble of 3rd byte for performance analysis
    //
    FxEnhancedVerifierPerformanceAnalysisMask      = 0x00f00000,
};

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
#define FxVerifierBugCheck(FxDriverGlobals, Error, ...)              \
    FX_VERIFY_WITH_NAME(DRIVER(BadAction, Error),                    \
                        TRAPMSG("WDF Violation: Please check"        \
                        "tracelog for a description of this error"), \
                        FxDriverGlobals->Public.DriverName)
#else
#define FxVerifierBugCheck(FxDriverGlobals, ...)              \
        FxVerifierBugCheckWorker(FxDriverGlobals, __VA_ARGS__);
#endif

//
// FxVerifierDbgBreakPoint and FxVerifierBreakOnDeviceStateError are mapped
// to FX_VERIFY in UMDF and break regardless of any flags
//
__inline
VOID
FxVerifierDbgBreakPoint(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
#if FX_CORE_MODE == FX_CORE_KERNEL_MODE
    CHAR ext[] = "sys";
#else
    CHAR ext[] = "dll";
#endif

    Mx::MxDbgPrint("WDF detected potentially invalid operation by %s.%s "
             "Dump the driver log (!wdflogdump %s.%s) for more information.\n",
             FxDriverGlobals->Public.DriverName, ext,
             FxDriverGlobals->Public.DriverName, ext
             );

    if (FxDriverGlobals->FxVerifierDbgBreakOnError) {
        Mx::MxDbgBreakPoint();
    } else {
        Mx::MxDbgPrint("Turn on framework verifier for %s.%s to automatically "
            "break into the debugger next time it happens.\n",
            FxDriverGlobals->Public.DriverName, ext);
    }
}

__inline
VOID
FxVerifierBreakOnDeviceStateError(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
#if FX_CORE_MODE == FX_CORE_KERNEL_MODE
        CHAR ext[] = "sys";
#else
        CHAR ext[] = "dll";
#endif

    Mx::MxDbgPrint("WDF detected potentially invalid device state in %s.%s. "
             "Dump the driver log (!wdflogdump %s.$s) for more information.\n",
             FxDriverGlobals->Public.DriverName, ext,
             FxDriverGlobals->Public.DriverName, ext);

    if (FxDriverGlobals->FxVerifierDbgBreakOnDeviceStateError) {
        Mx::MxDbgBreakPoint();
    } else {
        Mx::MxDbgPrint("Turn on framework verifier for %s.%s to automatically "
            "break into the debugger next time it happens.\n",
            FxDriverGlobals->Public.DriverName, ext);
    }
}

__inline
BOOLEAN
IsFxVerifierFunctionTableHooking(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    if (FxDriverGlobals->FxEnhancedVerifierOptions &
            FxEnhancedVerifierFunctionTableHookMask) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

DECLSPEC_NORETURN
VOID
FxVerifierBugCheckWorker(
    __in     PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in     WDF_BUGCHECK_CODES WdfBugCheckCode,
    __in_opt ULONG_PTR BugCheckParameter2 = 0,
    __in_opt ULONG_PTR BugCheckParameter3 = 0
    );

DECLSPEC_NORETURN
VOID
FxVerifierNullBugCheck(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PVOID ReturnAddress
    );

__inline
NTSTATUS
FxVerifierCheckIrqlLevel(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in KIRQL    Irql
    )
/*++

Routine Description:
    Check that current IRQL matches expected IRQL.

Arguments:
    Irql  -  The expected IRQL

Return Value:
    STATUS_SUCCESS                if expected IRQL matches current IRQL.
    STATUS_INVALID_DEVICE_REQUEST if expected IRQL does not match current IRQL.

  --*/
{
    //
    // Full treatment only if VerifierOn is set.
    //
    if (FxDriverGlobals->FxVerifierOn) {

        KIRQL currentIrql = Mx::MxGetCurrentIrql();

        if (currentIrql <= Irql) {
            return STATUS_SUCCESS;
        }

        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Called at wrong IRQL; at level %d, should be "
                            "at level %d", currentIrql, Irql);

        FxVerifierDbgBreakPoint(FxDriverGlobals);

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // If Verifier is turned off, always return success.
    //
    return STATUS_SUCCESS;
}


__inline
BOOLEAN
IsFxVerifierTestForwardProgressFailAll(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    if (FxDriverGlobals->FxEnhancedVerifierOptions &
            FxEnhancedVerifierForwardProgressFailAll) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

__inline
BOOLEAN
IsFxVerifierTestForwardProgressFailRandom(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    if (FxDriverGlobals->FxEnhancedVerifierOptions &
            FxEnhancedVerifierForwardProgressFailRandom) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

__inline
BOOLEAN
IsFxVerifierTestForwardProgress(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    if (FxDriverGlobals->FxEnhancedVerifierOptions &
            FxEnhancedVerifierForwardProgressMask) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

__inline
BOOLEAN
IsFxPerformanceAnalysis(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    if (FxDriverGlobals->FxEnhancedVerifierOptions &
            FxEnhancedVerifierPerformanceAnalysisMask) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

#endif // _FXVERIFIER_H_
