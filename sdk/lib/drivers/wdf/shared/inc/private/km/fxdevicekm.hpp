/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDeviceKM.hpp

Abstract:

    This is the definition of the FxDevice object KM specific

Author:



Environment:

    Kernel mode only

Revision History:

--*/

#ifndef _FXDEVICEKM_H_
#define _FXDEVICEKM_H_

__inline
FxWdmDeviceExtension*
FxDevice::_GetFxWdmExtension(
    __in MdDeviceObject DeviceObject
    )
{
    //
    // DeviceObject->DeviceExtension points to our FxDevice allocation.  We
    // get the underlying DeviceExtension allocated as part of the
    // PDEVICE_OBJECT by adding the sizeof(DEVICE_OBJECT) to get the start
    // of the DeviceExtension.  This is documented behavior on how the
    // DeviceExtension can be found.
    //
    return (FxWdmDeviceExtension*) WDF_PTR_ADD_OFFSET(DeviceObject,
                                                      sizeof(*DeviceObject));
}

__inline
MdRemoveLock
FxDevice::GetRemoveLock(
    VOID
    )
{
    return &FxDevice::_GetFxWdmExtension(
        GetDeviceObject())->IoRemoveLock;
}

__inline
BOOLEAN
FxDevice::IsRemoveLockEnabledForIo(
    VOID
    )
{
    if (m_DeviceObject.GetObject() != NULL) {
        return (_GetFxWdmExtension(m_DeviceObject.GetObject())->RemoveLockOptionFlags &
            WDF_REMOVE_LOCK_OPTION_ACQUIRE_FOR_IO) ? TRUE : FALSE;
    }
    else {
        return FALSE;
    }
}

__inline
FxDevice*
FxDevice::GetFxDevice(
    __in MdDeviceObject DeviceObject
    )
{
    MxDeviceObject deviceObject((MdDeviceObject)DeviceObject);

    //
    // DeviceExtension points to the start of the first context assigned to
    // WDFDEVICE.  We walk backwards from the context to the FxDevice*.
    //
    return (FxDevice*) CONTAINING_RECORD(deviceObject.GetDeviceExtension(),
                                         FxContextHeader,
                                         Context)->Object;
}

__inline
VOID
FxDevice::DetachDevice(
    VOID
    )
{
    if (m_AttachedDevice.GetObject() != NULL) {
        Mx::MxDetachDevice(m_AttachedDevice.GetObject());
        m_AttachedDevice.SetObject(NULL);
    }
}

__inline
VOID
FxDevice::InvalidateDeviceState(
    VOID
    )
{
    PDEVICE_OBJECT pdo;

    //
    // If the PDO is not yet reported to pnp, do not call the API.  In this
    // case, the PDO will soon be reported and started and the state that
    // was just set will be queried for automatically by pnp as a part of
    // start.
    //
    pdo = GetSafePhysicalDevice();

    if (pdo != NULL) {
        IoInvalidateDeviceState(pdo);
    }
}

VOID
__inline
FxDevice::DeleteSymbolicLink(
    VOID
    )
{
    if (m_SymbolicLinkName.Buffer != NULL) {
        //
        // Must be at PASSIVE_LEVEL for this call
        //
        if (m_SymbolicLinkName.Length) {
            Mx::MxDeleteSymbolicLink(&m_SymbolicLinkName);
        }

        FxPoolFree(m_SymbolicLinkName.Buffer);
        RtlZeroMemory(&m_SymbolicLinkName, sizeof(m_SymbolicLinkName));
    }
}

__inline
NTSTATUS
FxDevice::_OpenDeviceRegistryKey(
    _In_ MdDeviceObject DeviceObject,
    _In_ ULONG DevInstKeyType,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE DevInstRegKey
    )
{
    return IoOpenDeviceRegistryKey(DeviceObject,
                                   DevInstKeyType,
                                   DesiredAccess,
                                   DevInstRegKey);
}

__inline
NTSTATUS
FxDevice::_GetDeviceProperty(
    _In_      MdDeviceObject DeviceObject,
    _In_      DEVICE_REGISTRY_PROPERTY DeviceProperty,
    _In_      ULONG BufferLength,
    _Out_opt_ PVOID PropertyBuffer,
    _Out_     PULONG ResultLength
    )
{
    return IoGetDeviceProperty(DeviceObject,
                               DeviceProperty,
                               BufferLength,
                               PropertyBuffer,
                               ResultLength);
}

#endif //_FXDEVICEKM_H_
