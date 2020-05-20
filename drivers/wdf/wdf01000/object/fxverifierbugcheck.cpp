/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Bugcheck functions
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */


#include "common/fxverifier.h"


#define WDF_VIOLATION                    ((ULONG)0x0000010DL)

VOID
#ifndef __GNUC__
DECLSPEC_NORETURN
#endif
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

VOID
#ifndef __GNUC__
DECLSPEC_NORETURN
#endif
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
