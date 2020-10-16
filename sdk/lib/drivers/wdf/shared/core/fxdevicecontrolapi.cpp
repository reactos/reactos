/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    FxDeviceControlAPI.cpp

Abstract:

    This module implements external DDIs for control devices

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#include "coreprivshared.hpp"

extern "C" {
// #include "FxDeviceControlAPI.tmh"
}

extern "C" {

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfControlFinishInitializing)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice* pDevice;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID*) &pDevice,
                                   &pFxDriverGlobals);

    MxDeviceObject device(pDevice->GetDeviceObject());

    if (pDevice->IsLegacy()) {
        // pDevice->m_PkgWmi->Register(); __REACTOS__
        device.SetFlags(device.GetFlags() & ~DO_DEVICE_INITIALIZING);
    }
    else {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE %p not a control device", Device);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
    }
}

} // entire extern "C" for the file

