/*++

Copyright (c) Microsoft Corporation

Module Name:

    Stub.cpp

Abstract:

    This is the stub used by UMDF corresponding to the stub used by
    KMDF.

    It includes definitions of WdfFunctions, WdfBindInfo and WdfDriverGlobals

    On the UMDF side we don't need the thunk for DriverEntry because
    DriverEntry is invoked by UMDF host.

    For COM API UMDF framework will link the stub in
    For flat-C API UMDF driver will need to link the stub in
    (similar to the way KMDF drivers link the stub)

Environment:

    User mode only

Revision History:

--*/

#define  FX_DYNAMICS_GENERATE_TABLE   1

#include <ntverp.h>
extern "C" {
#include "mx.h"
}
#include "fxmin.hpp"
#include "fxldrUm.h"
#include "fxIFR.h"

#include <strsafe.h>
#include <driverspecs.h>

extern const WDFFUNC *WdfFunctions;

extern "C" {

#include "FxDynamics.h"

#include "..\librarycommon\FxLibraryCommon.h"

#define  KMDF_DEFAULT_NAME   "Wdf" ## \
                             LITERAL(__WDF_MAJOR_VERSION)   ## \
                             "000" //minor version



//-----------------------------------------------------------------------------
// local prototype definitions
//-----------------------------------------------------------------------------

ULONG    WdfLdrDbgPrintOn = 0;

PCHAR WdfLdrType = KMDF_DEFAULT_NAME;

}  // extern "C"

#include "umdfstub.h"

extern "C"
NTSTATUS
WDF_LIBRARY_COMMISSION(
    VOID
    );

extern "C"
NTSTATUS
WDF_LIBRARY_DECOMMISSION(
    VOID
    );

extern "C"
NTSTATUS
WDF_LIBRARY_REGISTER_CLIENT(
    PWDF_BIND_INFO        Info,
    PWDF_DRIVER_GLOBALS * WdfDriverGlobals,
    PVOID               * Context
    );

extern "C"
NTSTATUS
WDF_LIBRARY_UNREGISTER_CLIENT(
    PWDF_BIND_INFO        Info,
    PWDF_DRIVER_GLOBALS   WdfDriverGlobals
    );

extern "C" {

WDF_LIBRARY_INFO  WdfLibraryInfo = {
    sizeof(WDF_LIBRARY_INFO),
    (PFNLIBRARYCOMMISSION)        WDF_LIBRARY_COMMISSION,
    (PFNLIBRARYDECOMMISSION)      WDF_LIBRARY_DECOMMISSION,
    (PFNLIBRARYREGISTERCLIENT)    WDF_LIBRARY_REGISTER_CLIENT,
    (PFNLIBRARYUNREGISTERCLIENT)  WDF_LIBRARY_UNREGISTER_CLIENT,
    { __WUDF_MAJOR_VERSION, __WUDF_MINOR_VERSION, __WUDF_SERVICE_VERSION }
};

}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C"
NTSTATUS
WDF_LIBRARY_COMMISSION(
    VOID
    )
{
    return FxLibraryCommonCommission();
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C"
NTSTATUS
WDF_LIBRARY_DECOMMISSION(
    VOID
    )
{
    return FxLibraryCommonDecommission();
}

#define EVTLOG_MESSAGE_SIZE 70
#define RAW_DATA_SIZE 4

extern "C"
NTSTATUS
WDF_LIBRARY_REGISTER_CLIENT(
    PWDF_BIND_INFO        Info,
    PWDF_DRIVER_GLOBALS * WdfDriverGlobals,
    PVOID               * Context
    )
{
    NTSTATUS           status = STATUS_INVALID_PARAMETER;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    WCHAR              insertString[EVTLOG_MESSAGE_SIZE];
    ULONG              rawData[RAW_DATA_SIZE];
    PCLIENT_INFO       clientInfo = NULL;

    __Print((LITERAL(WDF_LIBRARY_REGISTER_CLIENT) ": enter\n"));

    clientInfo = (PCLIENT_INFO)*Context;
    *Context = NULL;

    ASSERT(Info->Version.Major == WdfLibraryInfo.Version.Major);

    //
    // NOTE: If the currently loaded  library < drivers minor version fail the load
    // instead of binding to a lower minor version. The reason for that if there
    // is a newer API or new contract change made the driver shouldn't be using older
    // API than it was compiled with.
    //

    if (Info->Version.Minor > WdfLibraryInfo.Version.Minor) {
        status = StringCchPrintfW(insertString,
                                     RTL_NUMBER_OF(insertString),
                                     L"Driver Version: %d.%d Umdf Lib. Version: %d.%d",
                                     Info->Version.Major,
                                     Info->Version.Minor,
                                     WdfLibraryInfo.Version.Major,
                                     WdfLibraryInfo.Version.Minor);
        if (!NT_SUCCESS(status)) {
            __Print(("ERROR: RtlStringCchPrintfW failed with Status 0x%x\n", status));
            return status;
        }
        rawData[0] = Info->Version.Major;
        rawData[1] = Info->Version.Minor;
        rawData[2] = WdfLibraryInfo.Version.Major;
        rawData[3] = WdfLibraryInfo.Version.Minor;











        //
        // this looks like the best status to return
        //
        return STATUS_OBJECT_TYPE_MISMATCH;

    }

    status = FxLibraryCommonRegisterClient(Info, WdfDriverGlobals, clientInfo);

    if (NT_SUCCESS(status)) {
        //
        // The context will be a pointer to FX_DRIVER_GLOBALS
        //
        *Context = GetFxDriverGlobals(*WdfDriverGlobals);

        //
        // Set the WDF_BIND_INFO structure pointer in FxDriverGlobals
        //
        pFxDriverGlobals = GetFxDriverGlobals(*WdfDriverGlobals);
        pFxDriverGlobals->WdfBindInfo = Info;
    }

    return status;
}

extern "C"
NTSTATUS
WDF_LIBRARY_UNREGISTER_CLIENT(
    PWDF_BIND_INFO        Info,
    PWDF_DRIVER_GLOBALS   WdfDriverGlobals
    )
{
    return FxLibraryCommonUnregisterClient(Info, WdfDriverGlobals);
}




















































extern "C" {

//-----------------------------------------------------------------------------
// These header files are referenced in order to make internal structures
// available in public symbols. Various WDFKD debug commands use these
// internal structures to provide information about WDF.
//-----------------------------------------------------------------------------

#ifndef DECLARE_TYPE
#define DECLARE_TYPE(Name) Name* _DECL_##Name
#endif

union {

    DECLARE_TYPE (WDF_IFR_HEADER);
    DECLARE_TYPE (WDF_IFR_RECORD);
    DECLARE_TYPE (WDF_IFR_OFFSET);
    DECLARE_TYPE (WDF_BIND_INFO);
    DECLARE_TYPE (WDF_OBJECT_CONTEXT_TYPE_INFO);
    DECLARE_TYPE (WDF_POWER_ROUTINE_TIMED_OUT_DATA);
    DECLARE_TYPE (WDF_BUGCHECK_CODES);
    DECLARE_TYPE (WDF_REQUEST_FATAL_ERROR_CODES);
    DECLARE_TYPE (FX_OBJECT_INFO);
    DECLARE_TYPE (FX_POOL_HEADER);
    DECLARE_TYPE (FX_POOL);
    DECLARE_TYPE (FxObject);
    DECLARE_TYPE (FxContextHeader);
//  DECLARE_TYPE (FX_DUMP_DRIVER_INFO_ENTRY);   // KMDF only
    DECLARE_TYPE (FxTargetSubmitSyncParams);

} uAllPublicTypes;

} // extern "C" end

//-----------------------------------------------------------------------------
