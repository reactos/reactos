/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxDeviceObjectUm.cpp

Abstract:

    User Mode implementation of Device Object defined in MxDeviceObject.h

--*/

#include "fxmin.hpp"
#include "fxldrum.h"

PVOID
MxDriverObject::GetDriverExtensionAddDevice(
    VOID
    )
{
    return m_DriverObject->AddDevice;
}

VOID
MxDriverObject::SetDriverExtensionAddDevice(
    _In_ MdDriverAddDevice Value
    )
{
    m_DriverObject->AddDevice = Value;
}

MdDriverUnload
MxDriverObject::GetDriverUnload(
    VOID
    )
{
    m_DriverObject->DriverUnload;
    return NULL;
}

VOID
MxDriverObject::SetDriverUnload(
    _In_ MdDriverUnload Value
    )
{
    m_DriverObject->DriverUnload = Value;
}

VOID
MxDriverObject::SetMajorFunction(
    _In_ UCHAR i,
    _In_ MdDriverDispatch Value
    )
{
    m_DriverObject->MajorFunction[i] = Value;
}

VOID
MxDriverObject::SetDriverObjectFlag(
    _In_ FxDriverObjectUmFlags Flag
    )
{
    m_DriverObject->Flags |= Flag;
}

BOOLEAN
MxDriverObject::IsDriverObjectFlagSet(
    _In_ FxDriverObjectUmFlags Flag
    )
{
    return (!!(m_DriverObject->Flags & Flag));
}

