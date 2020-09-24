/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    FxRelatedDevice.cpp

Abstract:

    This module implements the FxRelatedDevice class which is used in usage
    notification propagation

Author:



Environment:

    Kernel mode only

Revision History:

--*/

#include "FxSupportPch.hpp"

FxRelatedDevice::FxRelatedDevice(
    __in PDEVICE_OBJECT DeviceObject,
    __in PFX_DRIVER_GLOBALS Globals
    ) : FxObject(FX_TYPE_RELATED_DEVICE, 0, Globals),
        m_DeviceObject(DeviceObject),
        m_State(RelatedDeviceStateNeedsReportPresent)
{
    m_TransactionedEntry.SetTransactionedObject(this);
    ObReferenceObject(m_DeviceObject);
}

FxRelatedDevice::~FxRelatedDevice(
    VOID
    )
{
    ObDereferenceObject(m_DeviceObject);
}
