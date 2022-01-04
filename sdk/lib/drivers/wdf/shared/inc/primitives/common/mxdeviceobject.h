/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxDeviceObject.h

Abstract:

    Mode agnostic definition of Device Object

    See MxDeviceObjectKm.h and MxDeviceObjectUm.h/cpp for mode
    specific implementations

--*/

#pragma once

class MxDeviceObject
{
private:
    //
    // MdDeviceObject is typedef'ed to appropriate type for the mode
    // in the mode specific file
    //
    MdDeviceObject m_DeviceObject;

public:
    __inline
    MxDeviceObject(
        __in MdDeviceObject DeviceObject
        ) :
        m_DeviceObject(DeviceObject)
    {
    }

    __inline
    MxDeviceObject(
        VOID
        ) :
        m_DeviceObject(NULL)
    {
    }

    __inline
    MdDeviceObject
    GetObject(
        VOID
        )
    {
        return m_DeviceObject;
    }

    __inline
    VOID
    SetObject(
        __in_opt MdDeviceObject DeviceObject
        )
    {
        m_DeviceObject = DeviceObject;
    }

    CCHAR
    GetStackSize(
        VOID
        );

    VOID
    SetStackSize(
        _In_ CCHAR Size
        );

    VOID
    ReferenceObject(
        );

    MdDeviceObject
    GetAttachedDeviceReference(
        VOID
        );

    VOID
    DereferenceObject(
        );

    ULONG
    GetFlags(
        VOID
        );

    VOID
    SetFlags(
        ULONG Flags
        );

    POWER_STATE
    SetPowerState(
        __in POWER_STATE_TYPE  Type,
        __in POWER_STATE  State
        );

    VOID
    InvalidateDeviceRelations(
        __in DEVICE_RELATION_TYPE Type
        );

    VOID
    InvalidateDeviceState(
        __in MdDeviceObject Fdo //used in UMDF
        );

    PVOID
    GetDeviceExtension(
        VOID
        );

    VOID
    SetDeviceExtension(
        PVOID Value
        );

    DEVICE_TYPE
    GetDeviceType(
        VOID
        );

    ULONG
    GetCharacteristics(
        VOID
        );

    VOID
    SetDeviceType(
        DEVICE_TYPE Value
        );

    VOID
    SetCharacteristics(
        ULONG Characteristics
        );

    VOID
    SetAlignmentRequirement(
        _In_ ULONG Value
        );

    ULONG
    GetAlignmentRequirement(
        VOID
        );
};
