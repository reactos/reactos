/*++

Copyright (c) Microsoft Corporation

ModuleName:

    UMDFStubs.h

Abstract:

    Contains declarations for functions implemented by WdfStubUm.lib and
    WdfVersionUm.lib

    These are called by UMDF framework (in WUDF.cpp)

Author:



Revision History:



--*/

//
// Functions implemented by version\version.cpp
//
extern "C"
NTSTATUS
LibraryDriverEntry(
    __in PUNICODE_STRING  RegistryPath
    );

extern "C"
VOID
LibraryUnload(
    );

//
// Functions implemented by stub\stub.cpp
//
extern "C"
NTSTATUS
FxDriverEntry(
    __in PWDF_DRIVER_GLOBALS * WdfDriverGlobals
    );

extern "C"
VOID
DriverUnload(
    __in PWDF_DRIVER_GLOBALS WdfDriverGlobals
    );

//
// Functions implemented by version\FxLibraryCommon.cpp
//

VOID
FxIFRStop(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    );
