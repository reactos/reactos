/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxFrameworkStubUm.h

Abstract:

    This is the internal flat-api stub version.

--*/

// generic loader defines.
#include <fxldrum.h>

#pragma once

#ifdef __cplusplus
extern "C"{
#endif

//------------------------------------------------------------------------
// UMDF Loader interface for Flat-c framework stub.
//------------------------------------------------------------------------
struct IWudfHost;
struct IUMDFPlatform;

typedef
__checkReturn
NTSTATUS
(*PFN_WUDF_REGISTER_LIBRARY) (
    __in PVOID Context,
    __in PWDF_LIBRARY_INFO LibraryInfo
    );

typedef struct _WUDF_LOADER_FX_INTERFACE {
    ULONG                       Size;
    PFN_WUDF_REGISTER_LIBRARY   RegisterLibrary;
    IWudfHost *                 pIWudfHost;
    IUMDFPlatform *             pUMDFPlatform;
} WUDF_LOADER_FX_INTERFACE, *PWUDF_LOADER_FX_INTERFACE;

#ifdef __cplusplus
}
#endif

