/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDeviceKm.cpp

Abstract:

    This is the KM specific class implementation for the base Device class.

Author:



Environment:

    Kernel mode only

Revision History:

--*/

#include "coreprivshared.hpp"

extern "C" {
#include "FxDeviceKm.tmh"
}

_Must_inspect_result_
NTSTATUS
FxDevice::FdoInitialize(
    __in PWDFDEVICE_INIT DeviceInit
    )
{
    PFX_DRIVER_GLOBALS pGlobals;
    NTSTATUS status;
    FxPkgFdo * pkgFdo;

    pGlobals = GetDriverGlobals();

    if (DeviceInit->Fdo.EventCallbacks.EvtDeviceFilterAddResourceRequirements != NULL &&
        DeviceInit->Fdo.EventCallbacks.EvtDeviceRemoveAddedResources == NULL) {
        //
        // Not allowed to add resources without filtering them out later
        //
        DoTraceLevelMessage(
            pGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "Must set EvtDeviceRemoveAddedResources if "
            "EvtDeviceFilterAddResourceRequirements (%p) is set",
            DeviceInit->Fdo.EventCallbacks.EvtDeviceFilterAddResourceRequirements);

        FxVerifierDbgBreakPoint(pGlobals);

        return STATUS_INVALID_DEVICE_STATE;
    }

    //
    // All FDOs cannot be deleted through the driver calling WdfObjectDelete
    //
    MarkNoDeleteDDI();

    m_PhysicalDevice.SetObject((MdDeviceObject)DeviceInit->Fdo.PhysicalDevice);

    //
    // The PDO is known because it was used to bring up this FDO.
    //
    m_PdoKnown = TRUE;

    //
    // Try to create and install the default packages that an FDO contains.
    //

    // PnP
    status = FxPkgFdo::_Create(pGlobals, (CfxDevice*)this, &pkgFdo);

    if (!NT_SUCCESS(status)) {
        return status;
    }
    else {
        m_PkgPnp = pkgFdo;
    }

    InstallPackage(m_PkgPnp);

    status = SetFilter(DeviceInit->Fdo.Filter);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = GetFdoPkg()->Initialize(DeviceInit);
    if (!NT_SUCCESS(status)) {
        return status;
    }


    //
    // Should be done after invoking Initialize so that FxPkgFdo::m_EnumInfo is
    // already allocated.
    //
    if (DeviceInit->Fdo.ListConfig.Size > 0) {
        status = GetFdoPkg()->CreateDefaultDeviceList(
            &DeviceInit->Fdo.ListConfig,
            DeviceInit->Fdo.ListConfigAttributes.Size > 0
                ? &DeviceInit->Fdo.ListConfigAttributes
                : NULL);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        SetDeviceTelemetryInfoFlags(DeviceInfoHasDynamicChildren);
    }

    //
    // If the Size is zero then the driver writer never set any callbacks so we
    // can skip this call.
    //
    if (DeviceInit->Fdo.EventCallbacks.Size != 0) {
        status = GetFdoPkg()->RegisterCallbacks(&DeviceInit->Fdo.EventCallbacks);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    status = CreateDevice(DeviceInit);
    if (NT_SUCCESS(status)) {
        //
        // If this is an FDO then the PhysicalDevice field will be initialized,
        // and we need to attach to the device stack.
        //
        m_AttachedDevice.SetObject(Mx::MxAttachDeviceToDeviceStack(
            m_DeviceObject.GetObject(), m_PhysicalDevice.GetObject()));

        if (m_AttachedDevice.GetObject() == NULL) {
            //
            // We couldn't attach the device for some reason.
            //
            Mx::MxDeleteDevice(m_DeviceObject.GetObject());
            m_DeviceObject = NULL;

            status = STATUS_NO_SUCH_DEVICE;
        }
        else {
            //
            // If we are a filter device, inherit some state from the
            // attached device.
            //
            if (m_Filter) {
                //
                // Set the IO type and power pageable status on our device based
                // on the attached device's settings.
                //
                SetFilterIoType();

                m_DeviceObject.SetFlags(m_DeviceObject.GetFlags() |
                    (m_AttachedDevice.GetFlags() & (DO_POWER_PAGABLE | DO_POWER_INRUSH)));

                m_DeviceObject.SetDeviceType(m_AttachedDevice.GetDeviceType());
                m_DeviceObject.SetCharacteristics(m_AttachedDevice.GetCharacteristics());

                //
                // For devices other then filters, m_PowerPageableCapable gets
                // set in CreateDevice, but since this is determined for filters
                // by the device they have attached to, we must set this value
                // later as a special case only for filters.
                //
                if (m_DeviceObject.GetFlags() & DO_POWER_PAGABLE) {
                    m_PowerPageableCapable = TRUE;
                }
            }
            else {
                //
                // We are not a filter, we dictate our own DO flags
                //

                //
                // Power pageable and inrush are mutually exclusive
                //
                if (DeviceInit->PowerPageable) {
                    m_DeviceObject.SetFlags(m_DeviceObject.GetFlags() | DO_POWER_PAGABLE);
                }
                else if (DeviceInit->Inrush) {
                    m_DeviceObject.SetFlags(m_DeviceObject.GetFlags() | DO_POWER_INRUSH);
                }
            }
        }
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = m_PkgWmi->PostCreateDeviceInitialize();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = m_PkgGeneral->PostCreateDeviceInitialize(DeviceInit);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = GetFdoPkg()->PostCreateDeviceInitialize();

    if (NT_SUCCESS(status)) {
        //
        // Do not clear the DO_DEVICE_INITIALIZING bit here.  Instead, either
        // let the caller do it in their AddDevice or do it after the AddDevice
        // callback returns.
        //
        // FinishInitializing();
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxDevice::PdoInitialize(
    __in PWDFDEVICE_INIT DeviceInit
    )
{
    PFX_DRIVER_GLOBALS pGlobals;
    FxPkgPdo* pPkgPdo;
    NTSTATUS status;

    pGlobals = GetDriverGlobals();

    //
    // If the child is static, the driver has the ability to delete the child
    // device handle in between WdfDeviceCreate succeededing and then adding
    // the child to the list of static children.
    //
    if (DeviceInit->Pdo.Static == FALSE) {
        MarkNoDeleteDDI();
    }

    //
    // Double check to make sure that the PDO has a name
    //
    if (DeviceInit->HasName() == FALSE) {
        return STATUS_OBJECT_NAME_INVALID;
    }

    m_ParentDevice = DeviceInit->Pdo.Parent;

    //
    // This reference will be removed when this object is destroyed
    //
    m_ParentDevice->ADDREF(this);

    pPkgPdo = new(pGlobals) FxPkgPdo(pGlobals, (CfxDevice*)this);

    m_PkgPnp = pPkgPdo;
    if (m_PkgPnp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    InstallPackage(m_PkgPnp);

    status = m_PkgPnp->Initialize(DeviceInit);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // If the Size is zero then the driver writer never set any callbacks so we
    // can skip this call.
    //
    if (DeviceInit->Pdo.EventCallbacks.Size != 0) {
        pPkgPdo->RegisterCallbacks(&DeviceInit->Pdo.EventCallbacks);
    }

    status = CreateDevice(DeviceInit);

    if (NT_SUCCESS(status)) {

        //
        // We are initializing the PDO, stash away the PDO's device object
        // under m_PhysicalDevice as well so that we don't have to check
        // for device type later.
        //
        m_PhysicalDevice = m_DeviceObject;

        if (DeviceInit->Pdo.Raw) {
            pPkgPdo->m_RawOK = TRUE;
        }

        //
        // Power pageable and inrush are mutually exclusive
        //
        if (DeviceInit->PowerPageable) {
            m_DeviceObject.SetFlags(m_DeviceObject.GetFlags() | DO_POWER_PAGABLE);
        }
        else if (DeviceInit->Inrush) {
            m_DeviceObject.SetFlags(m_DeviceObject.GetFlags() | DO_POWER_INRUSH);
        }

        if (DeviceInit->Pdo.ForwardRequestToParent) {
            m_DeviceObject.SetStackSize(m_DeviceObject.GetStackSize() + DeviceInit->Pdo.Parent->GetDeviceObject()->StackSize);
            pPkgPdo->m_AllowForwardRequestToParent = TRUE;
        }

        status = m_PkgWmi->PostCreateDeviceInitialize();
        if (!NT_SUCCESS(status)) {
            return status;
        }

        status = m_PkgGeneral->PostCreateDeviceInitialize(DeviceInit);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        status = pPkgPdo->PostCreateDeviceInitialize();

        if (NT_SUCCESS(status)) {
            //
            // Clear the DO_DEVICE_INITIALIZING bit.
            //
            FinishInitializing();
        }
    }

    return status;
}

VOID
FxDevice::Destroy(
    VOID
    )
{
    //
    // We must be at passive for IoDeleteDevice
    //
    ASSERT(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);

    if (m_DeviceObject.GetObject() != NULL) {
        //
        // The device object may not go away right away if there are pending
        // references on it.  But we can't look up our FxDevice anymore, so
        // lets clear the DeviceExtension pointer.
        //
        m_DeviceObject.SetDeviceExtension(NULL);
    }

    //
    // Since this can be called in the context of the destructor when the ref
    // count is zero, use GetObjectHandleUnchecked() to get the handle value.
    //
    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGDEVICE,
        "Deleting !devobj %p, WDFDEVICE %p, attached to !devobj %p",
        m_DeviceObject.GetObject(), GetObjectHandleUnchecked(), m_AttachedDevice.GetObject());

    DetachDevice();

    if (m_DeviceObject.GetObject() != NULL) {
        DeleteSymbolicLink();

        if (m_DeviceObjectDeleted) {
            //
            // The device already deleted previously, release the reference we
            // took at the time of delete.
            //
            Mx::MxDereferenceObject(m_DeviceObject.GetObject());
        }
        else {
            Mx::MxDeleteDevice(m_DeviceObject.GetObject());
        }

        m_DeviceObject.SetObject(NULL);
    }

    //
    // Clean up any referenced objects
    //
    if (m_DeviceName.Buffer != NULL) {
        FxPoolFree(m_DeviceName.Buffer);
        RtlZeroMemory(&m_DeviceName, sizeof(m_DeviceName));
    }

    if (m_MofResourceName.Buffer != NULL) {
        FxPoolFree(m_MofResourceName.Buffer);
        RtlZeroMemory(&m_MofResourceName, sizeof(m_DeviceName));
    }
}

VOID
FxDevice::DestructorInternal(
    VOID
    )
{
    // NOTHING TO DO
}

_Must_inspect_result_
NTSTATUS
FxDevice::ControlDeviceInitialize(
    __in PWDFDEVICE_INIT DeviceInit
    )
{
    NTSTATUS status;

    m_Legacy = TRUE;

    status = CreateDevice(DeviceInit);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = m_PkgWmi->PostCreateDeviceInitialize();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = m_PkgGeneral->PostCreateDeviceInitialize(DeviceInit);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // NOTE: The driver writer must clear the DO_DEVICE_INITIALIZING bit
    //       via WdfControlFinishInitializing
    //
    return status;
}

VOID
FxDevice::AddChildList(
    __inout FxChildList* List
    )
{
    if (IsPnp()) {
        m_PkgPnp->AddChildList(List);
    }
}

VOID
FxDevice::RemoveChildList(
    __inout FxChildList* List
    )
{
    if (IsPnp()) {
        m_PkgPnp->RemoveChildList(List);
    }
}

_Must_inspect_result_
NTSTATUS
FxDevice::AllocateDmaEnablerList(
    VOID
    )
{
    if (IsPnp()) {
        return m_PkgPnp->AllocateDmaEnablerList();
    }
    else {
        return STATUS_SUCCESS;
    }
}

VOID
FxDevice::AddDmaEnabler(
    __inout FxDmaEnabler* Enabler
    )
{
    if (IsPnp()) {
        m_PkgPnp->AddDmaEnabler(Enabler);
    }
}

VOID
FxDevice::RemoveDmaEnabler(
    __inout FxDmaEnabler* Enabler
    )
{
    if (IsPnp()) {
        m_PkgPnp->RemoveDmaEnabler(Enabler);
    }
}




NTSTATUS
FxDevice::WmiPkgRegister(
    VOID
    )
{
    return m_PkgWmi->Register();
}

VOID
FxDevice::WmiPkgDeregister(
    VOID
    )
{
    m_PkgWmi->Deregister();
}

VOID
FxDevice::WmiPkgCleanup(
    VOID
    )
{
    m_PkgWmi->Cleanup();
}

NTSTATUS
FxDevice::CreateSymbolicLink(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ PCUNICODE_STRING SymbolicLinkName
    )
{
    NTSTATUS status;
    PUNICODE_STRING pName;
    FxAutoString pdoName;

    //
    // Ensure m_DeviceName has been set or we can get the PDO's name
    //
    if (m_DeviceName.Buffer == NULL) {
        MdDeviceObject pPdo;
        PWSTR pBuffer;
        ULONG length;

        if (IsLegacy()) {
            //
            // No PDO on a legacy stack, can't call IoGetDeviceProperty
            //
            status = STATUS_INVALID_DEVICE_STATE;

            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WDFDEVICE %p has no device name (use WdfDeviceInitAssignName), "
                "%!STATUS!", GetHandle(), status);

            return status;
        }

        pPdo = GetSafePhysicalDevice();
        if (pPdo == NULL) {
            //
            // No PDO yet for this device, pnp doesn't know about it.
            //
            status = STATUS_INVALID_DEVICE_STATE;

            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WDFDEVICE %p has not yet been reported to pnp, cannot call "
                "IoGetDeviceProperty in this state, %!STATUS!", GetHandle(), status);

            return status;
        }

        length = 0;

        status = _GetDeviceProperty(pPdo,
                                    DevicePropertyPhysicalDeviceObjectName,
                                    0,
                                    NULL,
                                    &length);

        if (status != STATUS_BUFFER_TOO_SMALL && !NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WDFDEVICE %p IoGetDeviceProperty failed %!STATUS!",
                GetHandle(), status);
            return status;
        }
        else if (length > USHORT_MAX) {
            status = STATUS_INTEGER_OVERFLOW;

            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WDFDEVICE %p PDO name too long (%d, max is %d), %!STATUS!",
                GetHandle(), length, USHORT_MAX, status);

            return status;
        }
        else if (length == 0) {
            //
            // We can get zero back if the PDO is being deleted.
            //
            status = STATUS_INVALID_DEVICE_STATE;

            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WDFDEVICE %p PDO name length is zero, %!STATUS!",
                GetHandle(), status);

            return status;
        }

        pBuffer = (PWSTR) FxPoolAllocate(FxDriverGlobals, PagedPool, length);
        if (pBuffer == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;

            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WDFDEVICE %p could not allocate buffer for PDO name, %!STATUS!",
                GetHandle(), status);

            return status;
        }

        //
        // pdoName will free pBuffer when it is destructed
        //
        pdoName.m_UnicodeString.Buffer = pBuffer;

        status = _GetDeviceProperty(pPdo,
                                    DevicePropertyPhysicalDeviceObjectName,
                                    length,
                                    pBuffer,
                                    &length);

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WDFDEVICE %p IoGetDeviceProperty failed second time, %!STATUS!",
                GetHandle(), status);
            return status;
        }

        pdoName.m_UnicodeString.Length = (USHORT) length - sizeof(UNICODE_NULL);
        pdoName.m_UnicodeString.MaximumLength = (USHORT) length;

        pName = &pdoName.m_UnicodeString;
    }
    else {
        pName = &m_DeviceName;
    }

    status = FxDuplicateUnicodeString(FxDriverGlobals,
                                      SymbolicLinkName,
                                      &m_SymbolicLinkName);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE %p allocate buffer for symbolic name failed, %!STATUS!",
            GetHandle(), status);

        return status;
    }

    status = Mx::MxCreateSymbolicLink(&m_SymbolicLinkName, pName);

    if (!NT_SUCCESS(status)) {
        FxPoolFree(m_SymbolicLinkName.Buffer);

        RtlZeroMemory(&m_SymbolicLinkName,
                      sizeof(m_SymbolicLinkName));

        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "WDFDEVICE %p  create sybolic link failed, %!STATUS!",
                            GetHandle(), status);
    }

    return status;
}

NTSTATUS
FxDevice::AssignProperty (
    _In_ PVOID PropertyData,
    _In_ FxPropertyType FxPropertyType,
    _In_ DEVPROPTYPE Type,
    _In_ ULONG BufferLength,
    _In_opt_ PVOID PropertyBuffer
    )
{
    NTSTATUS status;
    const DEVPROPKEY * propertyKey;
    LCID lcid;
    ULONG flags;
    PWDF_DEVICE_PROPERTY_DATA deviceData;
    PDEVICE_OBJECT pdo;

    UNREFERENCED_PARAMETER(FxPropertyType);

    ASSERT(FxPropertyType == FxDeviceProperty);

    deviceData = (PWDF_DEVICE_PROPERTY_DATA) PropertyData;

    propertyKey = deviceData->PropertyKey;
    lcid = deviceData->Lcid;
    flags = deviceData->Flags;

    pdo = GetSafePhysicalDevice();

    if (pdo == NULL) {
        //
        // Pnp doesn't know about the PDO yet.
        //
        status = STATUS_INVALID_DEVICE_STATE;
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE %p is not yet known to PnP manager, cannot call "
            "WdfDeviceAssignPropertyEx in this state, %!STATUS!",
            GetHandle(), status);

        return status;
    }

    status = IoSetDevicePropertyData(
                pdo,
                propertyKey,
                lcid,
                flags,
                Type,
                BufferLength,
                PropertyBuffer
                );

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFDEVICE %p failed to assign device property, %!STATUS!",
            GetHandle(), status);
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxDevice::_OpenKey(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_opt_ PWDFDEVICE_INIT DeviceInit,
    _In_opt_ FxDevice* Device,
    _In_ ULONG DeviceInstanceKeyType,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES KeyAttributes,
    _Out_ WDFKEY* Key
    )
{
    FxRegKey* pKey;
    WDFKEY keyHandle;
    HANDLE hKey = NULL;
    NTSTATUS status;
    MdDeviceObject pdo;

    status = FxValidateObjectAttributes(FxDriverGlobals, KeyAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxVerifierCheckIrqlLevel(FxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = _ValidateOpenKeyParams(FxDriverGlobals, DeviceInit, Device);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    _Analysis_assume_(DeviceInit != NULL || Device != NULL);

    if (DeviceInit != NULL) {
        pdo = DeviceInit->Fdo.PhysicalDevice;
    }
    else {
        pdo = Device->GetSafePhysicalDevice();
        if (pdo == NULL) {
            //
            // Pnp doesn't know about the PDO yet.
            //
            status = STATUS_INVALID_DEVICE_STATE;
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WDFDEVICE %p is not yet known to PnP manager, cannot open "
                "PNP registry keys in this state, %!STATUS!", Device->GetHandle(), status);
            return status;
        }
    }

    pKey = new(FxDriverGlobals, KeyAttributes) FxRegKey(FxDriverGlobals);

    if (pKey == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Device != NULL) {
        pKey->SetDeviceBase(Device);
    }

    status = pKey->Commit(KeyAttributes, (WDFOBJECT*)&keyHandle);

    if (NT_SUCCESS(status)) {
        status = _OpenDeviceRegistryKey(pdo,
                                        DeviceInstanceKeyType,
                                        DesiredAccess,
                                        &hKey);
        if (NT_SUCCESS(status)) {
            pKey->SetHandle(hKey);
            *Key = keyHandle;
        }
    }

    if (!NT_SUCCESS(status)) {
        //
        // No object is being returned, make sure the destroy callback will not
        // be called.
        //
        pKey->DeleteFromFailedCreate();
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxDevice::OpenSettingsKey(
    __out HANDLE* Key,
    __in ACCESS_MASK DesiredAccess
    )
{
    NTSTATUS status;
    FxAutoRegKey parent;
    MdDeviceObject pdo;

    //
    // We need a PDO to open this reg key.  in the case of failure to create
    // a static PDO, we will go down this path in the pnp state machine, so we
    // must check for validity always.
    //
    pdo = GetSafePhysicalDevice();

    if (pdo == NULL) {
        return STATUS_INVALID_DEVICE_STATE;
    }

    status = _OpenDeviceRegistryKey(pdo,
                                    PLUGPLAY_REGKEY_DEVICE,
                                    DesiredAccess,
                                    &parent.m_Key);
    if (NT_SUCCESS(status)) {
        DECLARE_CONST_UNICODE_STRING(wdf, L"WDF");

        //
        // Create the key if it does not already exist
        //
        status = FxRegKey::_Create(parent.m_Key,
                                   &wdf,
                                   Key,
                                   DesiredAccess);
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxDevice::_QueryPropertyEx (
    _In_ PFX_DRIVER_GLOBALS DriverGlobals,
    _In_opt_ PWDFDEVICE_INIT DeviceInit,
    _In_opt_ FxDevice* Device,
    _In_ PVOID PropertyData,
    _In_ FxPropertyType FxPropertyType,
    _In_ ULONG BufferLength,
    _Out_ PVOID PropertyBuffer,
    _Out_ PULONG ResultLength,
    _Out_ PDEVPROPTYPE PropertyType
    )
{
    NTSTATUS status;
    DEVPROPTYPE propType;
    ULONG requiredLength = 0;
    const DEVPROPKEY * propertyKey;
    LCID lcid;
    PWDF_DEVICE_PROPERTY_DATA deviceData;
    MdDeviceObject pdo;

    UNREFERENCED_PARAMETER(FxPropertyType);
    ASSERT(FxPropertyType == FxDeviceProperty);

    *ResultLength = 0;
    *PropertyType = 0;

    status = FxDevice::_ValidateOpenKeyParams(DriverGlobals,
                                              DeviceInit,
                                              Device);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    _Analysis_assume_(DeviceInit != NULL || Device != NULL);

    if (DeviceInit != NULL) {
        pdo = DeviceInit->Fdo.PhysicalDevice;
    }
    else {
        pdo = Device->GetSafePhysicalDevice();
        if (pdo == NULL) {
            //
            // Pnp doesn't know about the PDO yet.
            //
            status = STATUS_INVALID_DEVICE_STATE;
            DoTraceLevelMessage(
                DriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WDFDEVICE %p is not yet known to PnP manager, cannot query "
                "device properties in this state, %!STATUS!", Device->GetHandle(), status);
            return status;
        }
    }

    deviceData = (PWDF_DEVICE_PROPERTY_DATA) PropertyData;
    propertyKey = deviceData->PropertyKey;
    lcid = deviceData->Lcid;

    status = IoGetDevicePropertyData(pdo,
                                     propertyKey,
                                     lcid,
                                     0,
                                     BufferLength,
                                     PropertyBuffer,
                                     &requiredLength,
                                     &propType);
    if (status == STATUS_BUFFER_TOO_SMALL || NT_SUCCESS(status)) {
        *ResultLength = requiredLength;
        *PropertyType = propType;
    }
    else {
        DoTraceLevelMessage(DriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Query for property buffer failed, %!STATUS!",
                            status);
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxDevice::_QueryProperty(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_opt_ PWDFDEVICE_INIT DeviceInit,
    _In_opt_ FxDevice* Device,
    _In_opt_ MdDeviceObject RemotePdo,
    _In_ DEVICE_REGISTRY_PROPERTY DeviceProperty,
    _In_ ULONG BufferLength,
    _Out_opt_ PVOID PropertyBuffer,
    _Out_opt_ PULONG ResultLength
    )
{
    NTSTATUS status;
    MdDeviceObject pdo;

    ASSERT(DeviceInit != NULL || Device != NULL || RemotePdo != NULL);

    if (RemotePdo != NULL) {
        pdo = RemotePdo;
    }
    else {
        status = FxDevice::_ValidateOpenKeyParams(FxDriverGlobals,
                                                  DeviceInit,
                                                  Device);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        if (DeviceInit != NULL) {
            pdo = DeviceInit->Fdo.PhysicalDevice;
        }
        else {
            pdo = Device->GetSafePhysicalDevice();
            if (pdo == NULL) {
                //
                // Pnp doesn't know about the PDO yet.
                //
                status = STATUS_INVALID_DEVICE_STATE;
                DoTraceLevelMessage(
                    FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                    "WDFDEVICE %p is not yet known to PnP manager, cannot query "
                    "device properties in this state, %!STATUS!", Device->GetHandle(), status);
                return status;
            }
        }
    }

    status = _GetDeviceProperty(pdo,
                                DeviceProperty,
                                BufferLength,
                                PropertyBuffer,
                                ResultLength);
    return status;
}

_Must_inspect_result_
NTSTATUS
FxDevice::OpenDevicemapKeyWorker(
    _In_ PFX_DRIVER_GLOBALS pFxDriverGlobals,
    _In_ PCUNICODE_STRING KeyName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ FxRegKey* pKey
    )
{

    NTSTATUS status;
    UNICODE_STRING registryKeyPath;
    wchar_t baseStringBuffer[256] = FX_DEVICEMAP_PATH;

    //
    // Unlike UMDF, KMDF can open any DEVICEMAP key directly. Create a fully qualified
    // DEVICEMAP path from the provided subkey and pass it to FxRegKey::_OpenKey
    //
    registryKeyPath.Buffer = baseStringBuffer;
    registryKeyPath.MaximumLength = sizeof(baseStringBuffer);
    registryKeyPath.Length = sizeof(FX_DEVICEMAP_PATH) - sizeof(UNICODE_NULL);

    status = RtlAppendUnicodeStringToString(&registryKeyPath, KeyName);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Unable to create a DEVICEMAP registry path for subkey %S, %!STATUS!",
            KeyName->Buffer, status);
    } else {

        status = pKey->Create(NULL,
                              &registryKeyPath,
                              DesiredAccess,
                              REG_OPTION_VOLATILE,
                              NULL);

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                "WDFKEY open failed, %!STATUS!", status);
        }
    }

    return status;
}

