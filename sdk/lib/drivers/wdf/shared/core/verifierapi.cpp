/*++

Copyright (c) Microsoft Corporation

Module Name:

    VerifierApi.c

Abstract:

    This module implements various global verifier worker routines

Author:



Environment:

    Both kernel and user mode

Revision History:


--*/

#include "coreprivshared.hpp"

//
// extern "C" all APIs
//
extern "C" {

//
// Global triage Info for dbgeng and 0x9F work
//
extern WDF_TRIAGE_INFO g_WdfTriageInfo;


VOID
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

    if (pFxDriverGlobals->FxVerifierDbgBreakOnError) {
        DbgBreakPoint();
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
    DDI_ENTRY_IMPERSONATION_OK();

    //
    // Indicate to the BugCheck callback filter which IFR to dump.
    //
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);
    pFxDriverGlobals->FxForceLogsInMiniDump = TRUE;

#pragma prefast(suppress:__WARNING_USE_OTHER_FUNCTION, "WDF wrapper to KeBugCheckEx.");
    Mx::MxBugCheckEx(BugCheckCode,
                 BugCheckParameter1,
                 BugCheckParameter2,
                 BugCheckParameter3,
                 BugCheckParameter4);
}

VOID
WDFEXPORT(WdfCxVerifierKeBugCheck)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    WDFOBJECT Object,
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

    Common Driver Frameworks KeBugCheckEx() function. Cx should use this
    function rather than the system one or WdfVerifierKeBugCheck. This routine
    will indicate to the bugcheck callbacks that the IFR data for this driver or its
    client driver needs to be copied to the minidump file.

Arguments:

    DriverGlobals -

    Object - WDF uses this object to select which logs to write in the minidump.
        Cx can pass an object from the client driver's hierarchy to force
        the client driver's logs in the minidump.

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
    DDI_ENTRY_IMPERSONATION_OK();

    FxObject* pObject;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    if (NULL == Object) {
        pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);
    }
    else {
        FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                       Object,
                                       FX_TYPE_OBJECT,
                                       (PVOID*)&pObject,
                                       &pFxDriverGlobals);
    }

    UNREFERENCED_PARAMETER(pObject);

    //
    // Indicate to the BugCheck callback filter which IFR to dump.
    //
    pFxDriverGlobals->FxForceLogsInMiniDump = TRUE;

#pragma prefast(suppress:__WARNING_USE_OTHER_FUNCTION, "WDF wrapper to KeBugCheckEx.");
    Mx::MxBugCheckEx(BugCheckCode,
                 BugCheckParameter1,
                 BugCheckParameter2,
                 BugCheckParameter3,
                 BugCheckParameter4);
}


PVOID
WDFEXPORT(WdfGetTriageInfo)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals
    )

/*++

Routine Description:

    Returns a pointer to the WDF triage info for dbgeng and 0x9F work.

Arguments:

    DriverGlobals -

Return Value:

    None.

--*/

{
    DDI_ENTRY();

    UNREFERENCED_PARAMETER(DriverGlobals);
    return &g_WdfTriageInfo;
}

} // extern "C"
