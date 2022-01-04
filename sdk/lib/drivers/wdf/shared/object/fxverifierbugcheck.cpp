/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxVerifierBugcheck.cpp

Abstract:

    This file contains definitions of verifier bugcheck functions
    These definitions are split from tracing.cpp in kmdf\src\core

Author:




Environment:

    Both kernel and user mode

Revision History:


--*/

#include "fxobjectpch.hpp"

// We use DoTraceMessage
extern "C" {
#if defined(EVENT_TRACING)
#include "FxVerifierBugcheck.tmh"
#endif
}

//=============================================================================
//
//=============================================================================


DECLSPEC_NORETURN
VOID
FxVerifierBugCheckWorker(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in WDF_BUGCHECK_CODES WdfBugCheckCode,
    __in_opt ULONG_PTR BugCheckParameter2,
    __in_opt ULONG_PTR BugCheckParameter3
    )
/*++

Routine Description:
    Wrapper for system BugCheck.

    Note this functions is marked "__declspec(noreturn)"

Arguments:

Returns:

--*/
{
    //
    // Indicate to the BugCheck callback filter which IFR to dump.
    //
    FxDriverGlobals->FxForceLogsInMiniDump = TRUE;

    Mx::MxBugCheckEx(WDF_VIOLATION,
                 WdfBugCheckCode,
                 BugCheckParameter2,
                 BugCheckParameter3,
                 (ULONG_PTR) FxDriverGlobals );
}

DECLSPEC_NORETURN
VOID
FxVerifierNullBugCheck(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PVOID ReturnAddress
    )
/*++

Routine Description:

    Calls KeBugCheckEx indicating a WDF DDI was passed a NULL parameter.

    Note this functions is marked "__declspec(noreturn)"

Arguments:

Returns:

--*/
{

    DoTraceLevelMessage( FxDriverGlobals, TRACE_LEVEL_FATAL, TRACINGERROR,
                         "NULL Required Parameter Passed to a DDI\n"
                         "FxDriverGlobals 0x%p",
                         FxDriverGlobals
                         );

    FxVerifierBugCheck(FxDriverGlobals,
                       WDF_REQUIRED_PARAMETER_IS_NULL,  // Bugcheck code.
                       0,                               // Parameter 2
                       (ULONG_PTR)ReturnAddress         // Parameter 3
                       );
}
