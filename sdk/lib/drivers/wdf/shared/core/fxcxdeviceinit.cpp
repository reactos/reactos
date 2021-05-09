/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxCxDeviceInit.cpp

Abstract:
    Internals for WDFCXDEVICE_INIT

Author:



Environment:

    Both kernel and user mode

Revision History:



--*/

#include "coreprivshared.hpp"

extern "C" {
// #include "FxCxDeviceInit.tmh"
}

WDFCXDEVICE_INIT::WDFCXDEVICE_INIT()
{
    InitializeListHead(&ListEntry);

    ClientDriverGlobals = NULL;
    CxDriverGlobals = NULL;
    PreprocessInfo = NULL;
    IoInCallerContextCallback = NULL;
    RtlZeroMemory(&RequestAttributes, sizeof(RequestAttributes));
    RtlZeroMemory(&FileObject, sizeof(FileObject));
    FileObject.AutoForwardCleanupClose = WdfUseDefault;
    CxDeviceInfo = NULL;
}

WDFCXDEVICE_INIT::~WDFCXDEVICE_INIT()
{
    ASSERT(IsListEmpty(&ListEntry));

    if (PreprocessInfo != NULL) {
        delete PreprocessInfo;
    }
}

_Must_inspect_result_
PWDFCXDEVICE_INIT
WDFCXDEVICE_INIT::_AllocateCxDeviceInit(
    __in PWDFDEVICE_INIT DeviceInit
    )
{
    PFX_DRIVER_GLOBALS  fxDriverGlobals;
    PWDFCXDEVICE_INIT   init;

    fxDriverGlobals = DeviceInit->DriverGlobals;

    init = new(fxDriverGlobals) WDFCXDEVICE_INIT();
    if (init == NULL) {
        DoTraceLevelMessage(fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                        "WDFDRIVER 0x%p  couldn't allocate WDFCXDEVICE_INIT",
                        DeviceInit->Driver);
        return NULL;
    }

    DeviceInit->AddCxDeviceInit(init);

    return init;
}

