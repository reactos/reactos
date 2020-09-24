/*++

Copyright (c) Microsoft Corporation

Module Name:

    framework.cpp

Abstract:

Environment:

    User mode only

Revision History:


--*/

#include <ntverp.h>
#include <strsafe.h>
#include <driverspecs.h>

#include "fxmin.hpp"
#include "FxFrameworkStubUm.h"

extern "C" {
#include "FxDynamics.h"
#include "..\librarycommon\FxLibraryCommon.h"
extern WDF_LIBRARY_INFO  WdfLibraryInfo;

#if !(NO_UMDF_VERSION_EXPORT)
__declspec(dllexport)

__declspec(selectany)

UMDF_VERSION_DATA Microsoft_WDF_UMDF_Version = {__WUDF_MAJOR_VERSION,

                                                __WUDF_MINOR_VERSION,

                                                __WUDF_SERVICE_VERSION};

#endif

//
// Pointer to the platform interface supplied by the host
//
IUMDFPlatform *g_IUMDFPlatform = NULL;
IWudfHost2 *g_IWudfHost2 = NULL;


// ***********************************************************************************
// DLL Entry Point
BOOL
WINAPI DllMain(
    __in HINSTANCE hInstance,
    __in DWORD dwReason,
    __in LPVOID lpReserved
    )
{
    UNREFERENCED_PARAMETER(lpReserved);

    hInstance;

    if (DLL_PROCESS_ATTACH == dwReason)
    {
        DisableThreadLibraryCalls(hInstance);
    }
    else if (DLL_PROCESS_DETACH == dwReason)
    {
        DO_NOTHING();
    }

    return TRUE;
}

__control_entrypoint(DllExport)
NTSTATUS
FxFrameworkEntryUm(
    __in PWUDF_LOADER_FX_INTERFACE LoaderInterface,
    __in PVOID Context
    )
{
    NTSTATUS status;

    //
    // Basic validation.
    //
    if (LoaderInterface == NULL ||
        LoaderInterface->Size < sizeof(WUDF_LOADER_FX_INTERFACE) ||
        LoaderInterface->pUMDFPlatform == NULL) {
        status = STATUS_INVALID_PARAMETER;
        __Print(("Failed to validate loader interface parameters, "
            "status 0x%x\n", status));
        goto Done;
    }

    //
    // Store platform interface.
    //
    g_IUMDFPlatform = LoaderInterface->pUMDFPlatform;

    //
    // Get the IWudfHost * from LoaderInterface.
    //
    FX_VERIFY(INTERNAL, CHECK_NOT_NULL(LoaderInterface->pIWudfHost));
    HRESULT hrQI = LoaderInterface->pIWudfHost->QueryInterface(
                                    IID_IWudfHost2,
                                    (PVOID*)&g_IWudfHost2
                                    );

    FX_VERIFY(INTERNAL, CHECK_QI(hrQI, g_IWudfHost2));
    g_IWudfHost2->Release();

    //
    // Do first time init of this v2.x framework module.
    // In framework v1.x this is done as a side effect of invoking the
    // IUMDFramework->Initialize method.
    //
    status = LoaderInterface->RegisterLibrary(Context, &WdfLibraryInfo);
    if (!NT_SUCCESS(status)) {
        __Print(("RegisterLibrary failed, status 0x%x\n", status));
        goto Done;
    }

    status = STATUS_SUCCESS;

Done:

    return status;
}

}
