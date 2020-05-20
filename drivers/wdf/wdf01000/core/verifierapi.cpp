/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Global verifier worker routines
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */



#include "common/fxglobals.h"
#include "common/dbgtrace.h"


extern "C" {

VOID
NTAPI
WDFEXPORT(WdfVerifierDbgBreakPoint)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals
    )

/*++

Routine Description:

    Common Driver Frameworks DbgBreakPoint() function.

    This will only break point if WdfVerifierDbgBreakOnError is defined, so
    it's safe to call for production systems.

Arguments:

    DriverGlobals -

Return Value:

    None.

--*/

{
    DDI_ENTRY_IMPERSONATION_OK();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);

    if (pFxDriverGlobals->FxVerifierDbgBreakOnError)
    {
        DbgBreakPoint();
    }
    else
    {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGDRIVER,
            "DbgBreakOnError registry value wasn't set, ignoring WdfVerifierDbgBreakPoint");
    }
}

VOID
WDFEXPORT(WdfVerifierKeBugCheck)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    ULONG BugCheckCode,
    __in
    ULONG_PTR BugCheckParameter1,
    __in
    ULONG_PTR BugCheckParameter2,
    __in
    ULONG_PTR BugCheckParameter3,
    __in
    ULONG_PTR BugCheckParameter4
    )

/*++

Routine Description:

    Common Driver Frameworks KeBugCheckEx() function. Use this function rather
    than the system one. This routine will indicate to the bugcheck callbacks
    that the IFR data for this driver needs to be copied to the minidump file.

Arguments:

    DriverGlobals -

    BugCheckCode - Specifies a value that indicates the reason for the bug check.

    BugCheckParameter1 - Supply additional information, such as the address and
        data where a memory-corruption error occurred, depending on the value of
        BugCheckCode.

    BugCheckParameter2 - Supply additional information, such as the address and
        data where a memory-corruption error occurred, depending on the value of
        BugCheckCode.

    BugCheckParameter3 - Supply additional information, such as the address and
        data where a memory-corruption error occurred, depending on the value of
        BugCheckCode.

    BugCheckParameter4 - Supply additional information, such as the address and
        data where a memory-corruption error occurred, depending on the value of
        BugCheckCode.

Return Value:

    None.

--*/

{
    WDFNOTIMPLEMENTED();
}

} // extern "C"
