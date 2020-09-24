/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxDeviceObjectKm.h

Abstract:

    Kernel Mode implementation of Device Object defined in MxDeviceObject.h

--*/

#pragma once

#include "MxDeviceObject.h"

__inline
CCHAR
MxDeviceObject::GetStackSize(
    VOID
    )
{
    return m_DeviceObject->StackSize;
}

__inline
VOID
MxDeviceObject::SetStackSize(
    _In_ CCHAR Size
    )
{
    m_DeviceObject->StackSize = Size;
}

__inline
VOID
MxDeviceObject::ReferenceObject(
    )
{
    ObReferenceObject(m_DeviceObject);
}

__inline
MdDeviceObject
MxDeviceObject::GetAttachedDeviceReference(
    VOID
    )
{
    return IoGetAttachedDeviceReference(m_DeviceObject);
}

__inline
VOID
MxDeviceObject::DereferenceObject(
    )
{
    ObDereferenceObject(m_DeviceObject);
}

__inline
ULONG
MxDeviceObject::GetFlags(
    VOID
    )
{
#pragma warning(disable:28129)
    return m_DeviceObject->Flags;
}

__inline
VOID
MxDeviceObject::SetFlags(
    ULONG Flags
    )
{
#pragma warning(disable:28129)
    m_DeviceObject->Flags = Flags;
}

__inline
POWER_STATE
MxDeviceObject::SetPowerState(
    __in POWER_STATE_TYPE  Type,
    __in POWER_STATE  State
    )
{
    return PoSetPowerState(m_DeviceObject, Type, State);
}

__inline
VOID
MxDeviceObject::InvalidateDeviceRelations(
    __in DEVICE_RELATION_TYPE Type
    )
{
    IoInvalidateDeviceRelations(m_DeviceObject, Type);
}

__inline
VOID
MxDeviceObject::InvalidateDeviceState(
    __in MdDeviceObject Fdo
    )
{
    //
    // UMDF currently needs Fdo for InvalidateDeviceState
    // FDO is not used in km.
    //
    // m_DeviceObject holds PDO that is what is used below.
    //

    UNREFERENCED_PARAMETER(Fdo);

    IoInvalidateDeviceState(m_DeviceObject);
}

__inline
PVOID
MxDeviceObject::GetDeviceExtension(
    VOID
    )
{
    return m_DeviceObject->DeviceExtension;
}

__inline
VOID
MxDeviceObject::SetDeviceExtension(
    PVOID Value
    )
{
    m_DeviceObject->DeviceExtension = Value;
}

__inline
DEVICE_TYPE
MxDeviceObject::GetDeviceType(
    VOID
    )
{
    return m_DeviceObject->DeviceType;
}

__inline
ULONG
MxDeviceObject::GetCharacteristics(
    VOID
    )
{
    return m_DeviceObject->Characteristics;
}

__inline
VOID
MxDeviceObject::SetDeviceType(
    DEVICE_TYPE Value
    )
{
    m_DeviceObject->DeviceType = Value;
}

__inline
VOID
MxDeviceObject::SetCharacteristics(
    ULONG Characteristics
    )
{
    m_DeviceObject->Characteristics = Characteristics;
}

__inline
VOID
MxDeviceObject::SetAlignmentRequirement(
    _In_ ULONG Value
    )
{
    m_DeviceObject->AlignmentRequirement = Value;
}

__inline
ULONG
MxDeviceObject::GetAlignmentRequirement(
    VOID
    )
{
    return m_DeviceObject->AlignmentRequirement;
}
