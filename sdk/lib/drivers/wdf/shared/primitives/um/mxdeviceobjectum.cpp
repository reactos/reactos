/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxDeviceObjectUm.cpp

Abstract:

    User Mode implementation of Device Object defined in MxDeviceObject.h

--*/

#include "fxmin.hpp"

CCHAR
MxDeviceObject::GetStackSize(
    VOID
    )
{
    return (CCHAR) (static_cast<IWudfDevice2*>(m_DeviceObject))->GetStackSize2();
}

VOID
MxDeviceObject::ReferenceObject(
    )
{
    m_DeviceObject->AddRef();

    //
    // We take a reference on device stack object as it is the reference on
    // device stack object that keeps the driver loaded
    //
    m_DeviceObject->GetDeviceStackInterface()->AddRef();
}

MdDeviceObject
MxDeviceObject::GetAttachedDeviceReference(
    VOID
    )
{
    FX_VERIFY(INTERNAL, TRAPMSG("MxDeviceObject::GetAttachedDeviceReference "
                                "not implemented for UMDF"));
    return NULL;
}

VOID
MxDeviceObject::DereferenceObject(
    )
{
    m_DeviceObject->Release();

    //
    // We also take a device stack reference (see ReferenceObject)
    // release it now
    //
    m_DeviceObject->GetDeviceStackInterface()->Release();
}

ULONG
MxDeviceObject::GetFlags(
    VOID
    )
{
    return m_DeviceObject->GetDeviceObjectWdmFlags();
}

POWER_STATE
MxDeviceObject::SetPowerState(
    __in POWER_STATE_TYPE  /*Type*/,
    __in POWER_STATE  State
    )
{
    ULONG oldState;
    POWER_STATE oldStateEnum;

    m_DeviceObject->GetDeviceStackInterface()->SetPowerState(
        State.DeviceState,
        &oldState
        );

    oldStateEnum.DeviceState = (DEVICE_POWER_STATE) oldState;

    return oldStateEnum;
}

VOID
MxDeviceObject::InvalidateDeviceRelations(
    __in DEVICE_RELATION_TYPE Type
    )
{
    UNREFERENCED_PARAMETER(Type);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    //IoInvalidateDeviceRelations(DeviceObject, Type);
}

VOID
MxDeviceObject::InvalidateDeviceState(
    __in MdDeviceObject Fdo
    )
{
    //
    // DO NOT use m_DeviceObject. That specifies PDO in this case which is
    // currently NULL for UMDF. Use the passed in Fdo instead.
    //
    Fdo->GetDeviceStackInterface()->InvalidateDeviceState();
}

PVOID
MxDeviceObject::GetDeviceExtension(
    VOID
    )
{
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
    return NULL;
}

VOID
MxDeviceObject::SetDeviceExtension(
    PVOID Value
    )
{
    //
    // This will set the context
    //
    (static_cast<IWudfDevice2*>(m_DeviceObject))->SetContext(Value);
}

DEVICE_TYPE
MxDeviceObject::GetDeviceType(
    VOID
    )
{
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
    return (DEVICE_TYPE) 0;
}

ULONG
MxDeviceObject::GetCharacteristics(
    VOID
    )
{
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
    return 0;
}

VOID
MxDeviceObject::SetDeviceType(
    DEVICE_TYPE /* Value */
    )
{
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

VOID
MxDeviceObject::SetCharacteristics(
    ULONG /* Characteristics */
    )
{
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

VOID
MxDeviceObject::SetAlignmentRequirement(
    _In_ ULONG /* Value */
    )
{
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

ULONG
MxDeviceObject::GetAlignmentRequirement(
    VOID
    )
{
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
    return 0;
}

VOID
MxDeviceObject::SetStackSize(
    _In_ CCHAR Size
    )
{
    (static_cast<IWudfDevice2*>(m_DeviceObject))->SetStackSize2((ULONG)Size);
}

VOID
MxDeviceObject::SetFlags(
    ULONG Flags
    )
{
    UNREFERENCED_PARAMETER(Flags);








}

