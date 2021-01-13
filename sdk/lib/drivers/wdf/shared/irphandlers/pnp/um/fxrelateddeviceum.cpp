/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    FxRelatedDeviceUm.cpp

Abstract:

    This module implements the FxRelatedDevice class which is used in usage
    notification propagation

Author:




Environment:

    User mode only

Revision History:

--*/

#include <fxmin.hpp>

#pragma warning(push)
#pragma warning(disable:4100) //unreferenced parameter

FxRelatedDevice::FxRelatedDevice(
    __in MdDeviceObject DeviceObject,
    __in PFX_DRIVER_GLOBALS Globals
    ) : FxObject(FX_TYPE_RELATED_DEVICE, 0, Globals),
        m_DeviceObject(NULL),
        m_State(RelatedDeviceStateNeedsReportPresent)
{
    UfxVerifierTrapNotImpl();
}

FxRelatedDevice::~FxRelatedDevice(
    VOID
    )
{
    UfxVerifierTrapNotImpl();
}

#pragma warning(pop)
