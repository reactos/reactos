//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include "fxobjectpch.hpp"
#include "fxldrum.h"

extern "C" {

VOID
FxFreeAllocatedMdlsDebugInfo(
    __in FxDriverGlobalsDebugExtension* DebugExtension
    )
{
    UNREFERENCED_PARAMETER(DebugExtension);

    //DO_NOTHING()
}

_Must_inspect_result_
BOOLEAN
FX_DRIVER_GLOBALS::IsCorrectVersionRegistered(
    _In_ PCUNICODE_STRING /*ServiceKeyName*/
    )
{
    return TRUE;    //  Then it won't even call the next method
}

VOID
FX_DRIVER_GLOBALS::RegisterClientVersion(
    _In_ PCUNICODE_STRING ServiceKeyName
    )
{
    UNREFERENCED_PARAMETER(ServiceKeyName);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

_Must_inspect_result_
BOOLEAN
FX_DRIVER_GLOBALS::IsVersionGreaterThanOrEqualTo(
    __in ULONG  Major,
    __in ULONG  Minor
    )
{
    if ((WdfBindInfo->Version.Major > Major) ||
                (WdfBindInfo->Version.Major == Major &&
                  WdfBindInfo->Version.Minor >= Minor)) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

}

_Must_inspect_result_
BOOLEAN
FX_DRIVER_GLOBALS::IsDebuggerAttached(
    VOID
    )
{
    //
    // COnvert the returned BOOL into BOOLEAN
    //
    return (IsDebuggerPresent() != FALSE);
}
