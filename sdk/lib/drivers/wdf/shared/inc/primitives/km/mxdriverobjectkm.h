/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxDriverObjectKm.h

Abstract:

    Kernel Mode implementation of Driver Object defined in MxDriverObject.h

--*/

#pragma once

typedef DRIVER_ADD_DEVICE MdDriverAddDeviceType, *MdDriverAddDevice;
typedef DRIVER_UNLOAD MdDriverUnloadType, *MdDriverUnload;
typedef DRIVER_DISPATCH MdDriverDispatchType, *MdDriverDispatch;

#include "mxdriverobject.h"

__inline
PDRIVER_ADD_DEVICE
MxDriverObject::GetDriverExtensionAddDevice(
    VOID
    )
{
    return m_DriverObject->DriverExtension->AddDevice;
}

__inline
VOID
MxDriverObject::SetDriverExtensionAddDevice(
    _In_ MdDriverAddDevice Value
    )
{
    m_DriverObject->DriverExtension->AddDevice = Value;
}

__inline
MdDriverUnload
MxDriverObject::GetDriverUnload(
    VOID
    )
{
    return m_DriverObject->DriverUnload;
}

__inline
VOID
MxDriverObject::SetDriverUnload(
    _In_ MdDriverUnload Value
    )
{
    m_DriverObject->DriverUnload = Value;
}


__inline
VOID
MxDriverObject::SetMajorFunction(
    _In_ UCHAR i,
    _In_ MdDriverDispatch Value
    )
{
    m_DriverObject->MajorFunction[i] = Value;
}




