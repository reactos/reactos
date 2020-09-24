/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDeviceInitApiUm.cpp

Abstract:

    This module exposes the "C" interface to the FxDevice object.

Author:



Environment:

    User mode only

Revision History:

--*/

#include "coreprivshared.hpp"

extern "C" {
#include "FxDeviceInitApiUm.tmh"
}

//
// Extern "C" the entire file
//
extern "C" {

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfDeviceInitEnableHidInterface)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    )
{
    DDI_ENTRY();

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);

    DeviceInit->DevStack->SetHidInterfaceSupport();
}

} // extern "C"

