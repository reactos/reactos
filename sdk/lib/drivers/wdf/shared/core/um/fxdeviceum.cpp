/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDeviceUm.cpp

Abstract:

    This is the KM specific class implementation for the base Device class.

Author:



Environment:

    User mode only

Revision History:

--*/

#include "coreprivshared.hpp"

extern "C" {
#include "FxDeviceUm.tmh"
}

VOID
FxDevice::DispatchUm(
    _In_ MdDeviceObject DeviceObject,
    _In_ MdIrp Irp,
    _In_opt_ IUnknown* Context
    )
{
    UNREFERENCED_PARAMETER(Context);





    (void) Dispatch(DeviceObject, Irp);
}

VOID
FxDevice::DispatchWithLockUm(
    _In_ MdDeviceObject DeviceObject,
    _In_ MdIrp Irp,
    _In_opt_ IUnknown* Context
    )
{
    UNREFERENCED_PARAMETER(Context);

    (void) DispatchWithLock(DeviceObject, Irp);
}

FxDevice*
FxDevice::GetFxDevice(
    __in MdDeviceObject DeviceObject
    )
{
    IWudfDevice2* device2;




    device2 = static_cast<IWudfDevice2*> (DeviceObject);

    return (FxDevice *)device2->GetContext();
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
    HRESULT hr;
    BOOLEAN bAttached = FALSE;

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
        MdDeviceObject attachedDevice = NULL;

        //
        // If this is an FDO then the PhysicalDevice field will be initialized,
        // and we need to attach to the device stack.
        //
        //
        // Attach the newly created host device object to existing stack.
        // Insert the host device in the device chain for this device stack.
        //
        hr = DeviceInit->DevStack->AttachDevice(
                                        m_DeviceObject.GetObject(),
                                        &attachedDevice
                                        );
        if (S_OK == hr) {
            status = STATUS_SUCCESS;
        }
        else {
            //
            // Catch if the host isn't obeying the COM contract that requires
            // a NULL interface when FAILED(hr).
            // Note that AttachDevice can return success with a NULL interface
            // for the attached device when this device is the first in the
            // stack.
            //
            FX_VERIFY(DRIVER(BadArgument, TODO), CHECK_NULL(attachedDevice));
            status = FxDevice::NtStatusFromHr(DeviceInit->DevStack, hr);
            goto exit;
        }

        m_AttachedDevice.SetObject(attachedDevice);
        bAttached = TRUE;

        if (m_AttachedDevice.GetObject() == NULL) {
            //
            // Note that AttachDevice can return success with a NULL interface
            // for the attached device when this device is the first in the
            // stack.
            //




            //
            DO_NOTHING();
        }

        if (NT_SUCCESS(status)) {
            //
            // If PPO, save newly created device as PPO in device stack
            //
            if (DeviceInit->IsPwrPolOwner()) {
                GetDeviceStack()->SetPPO(m_DeviceObject.GetObject());
            }

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

                if (m_AttachedDevice.GetObject() != NULL) {
                    m_DeviceObject.SetFlags(m_DeviceObject.GetFlags() |
                        (m_AttachedDevice.GetFlags() & (DO_POWER_PAGABLE | DO_POWER_INRUSH)));
                }

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

    //
    // Set buffer retrieval and I/O type values
    //
    m_RetrievalMode = UMINT::WdfDeviceIoBufferRetrievalDeferred;
    m_ReadWriteIoType = DeviceInit->ReadWriteIoType;
    m_IoctlIoType = DeviceInit->DeviceControlIoType;;
    m_DirectTransferThreshold = DeviceInit->DirectTransferThreshold;

    //
    // Read HwAccess settings from registry
    //
    RetrieveDeviceRegistrySettings();

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

    exit:

    //
    // Let the device stack object hold the only reference to
    // the host devices. The infrastructure guarantees that the device
    // stack's lifetime is greater than the this object's lifetime.
    // If the Attach failed, the object will be destroyed.
    //
    if (NULL != m_DeviceObject.GetObject()) {
        m_DeviceObject.GetObject()->Release();
    }

    if (NULL != m_AttachedDevice.GetObject()) {
        m_AttachedDevice.GetObject()->Release();
    }

    if (!NT_SUCCESS(status)) {
        if (bAttached) {
            DetachDevice();
            bAttached = FALSE;
        }

        //
        // NULL out m_pIWudfDevice so that d'tor doesn't try to detach again.
        //
        m_DeviceObject.SetObject(NULL); // weak ref.
        m_AttachedDevice.SetObject(NULL); // weak ref.
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxDevice::CreateDevice(
    __in PWDFDEVICE_INIT DeviceInit
    )
{
    MdDeviceObject  pNewDeviceObject = NULL;
    ULONG           characteristics;
    NTSTATUS        status;
    DEVICE_TYPE     devType;
    HRESULT hr;
    IWudfDevice2* pNewDeviceObject2;
    IWudfDeviceStack2* pDevStack2;

    status = m_PkgGeneral->Initialize(DeviceInit);
    if (!NT_SUCCESS(status)) {
        return status;
    }










    UNREFERENCED_PARAMETER(devType);

    characteristics = DeviceInit->Characteristics;

    status = FxMessageDispatch::_CreateAndInitialize(GetDriverGlobals(),
                                                     this,
                                                     &m_Dispatcher);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    hr = DeviceInit->DevStack->QueryInterface(IID_IWudfDeviceStack2,
                                             (PVOID*)&pDevStack2);
    FX_VERIFY(INTERNAL, CHECK_QI(hr, pDevStack2));
    if (FAILED(hr)) {
        status = FxDevice::NtStatusFromHr(DeviceInit->DevStack, hr);
        return status;
    }
    pDevStack2->Release(); // weak ref. is sufficient.

    //
    // Create IWudfDevice
    //
    hr = pDevStack2->CreateDevice2(DeviceInit->DriverID,
                               static_cast<IFxMessageDispatch*>(m_Dispatcher),
                               sizeof(FxWdmDeviceExtension),
                               &pNewDeviceObject2);
    if (FAILED(hr)) {
        status = FxDevice::NtStatusFromHr(DeviceInit->DevStack, hr);
        return status;
    }

    hr = pNewDeviceObject2->QueryInterface(IID_IWudfDevice,
                                          (PVOID*)&pNewDeviceObject);
    FX_VERIFY(INTERNAL, CHECK_QI(hr, pNewDeviceObject));
    if (FAILED(hr)) {
        status = FxDevice::NtStatusFromHr(DeviceInit->DevStack, hr);
        return status;
    }
    pNewDeviceObject->Release();  // weak ref. is sufficient.

    if (NT_SUCCESS(status)) {

        //
        // Initialize the remove lock and the event for use with the remove lock
        // The event is initiatalized via IWudfDevice so the host can close the
        // handle before destroying the allocated "WDM extension"
        // In the KMDF implementation the "WDM device extension" is allocated by
        // IoCreateDevice and destroyed when the WDM device object is deleted.
        // In the kernel implementation the allocated extension only needs to be freed,
        // where-as with UM the event handle needs to be closed as well.
        //
        FxWdmDeviceExtension* pWdmExt;
        pWdmExt = _GetFxWdmExtension(pNewDeviceObject);

        Mx::MxInitializeRemoveLock(&pWdmExt->IoRemoveLock,
                               GetDriverGlobals()->Tag,
                               0,          // max min
                               0);         // highwater mark

        status = pNewDeviceObject2->InitializeEventForRemoveLock(
                                    &(pWdmExt->IoRemoveLock.RemoveEvent));
        if (!NT_SUCCESS(status)) {
            return status;
        }
        ASSERT(pWdmExt->IoRemoveLock.RemoveEvent);

        m_DeviceObject.SetObject(pNewDeviceObject);

        //
        // Set the context
        //
        pNewDeviceObject2->SetContext(this);

        //
        // capture input info
        //
        m_DevStack = DeviceInit->DevStack;

        //
        // Hijack from deviceinit
        //
        m_PdoDevKey = DeviceInit->PdoKey;
        DeviceInit->PdoKey = NULL;

        m_DeviceKeyPath = DeviceInit->ConfigRegistryPath;
        DeviceInit->ConfigRegistryPath = NULL;

        m_KernelDeviceName = DeviceInit->KernelDeviceName;
        DeviceInit->KernelDeviceName = NULL;

        m_DeviceInstanceId = DeviceInit->DevInstanceID;
        DeviceInit->DevInstanceID = NULL;

        //
        // Set some device object flags based on properties of DeviceInit.
        //
        // If we are a filter, we will set these flags later
        // (in FxDevice::FdoInitialize) based on the device we are attached to.
        //
        if (m_Filter == FALSE) {
            if (DeviceInit->ReadWriteIoType == WdfDeviceIoBuffered) {
                m_DeviceObject.SetFlags(m_DeviceObject.GetFlags() | DO_BUFFERED_IO);
            }
            else if (DeviceInit->ReadWriteIoType == WdfDeviceIoDirect) {
                m_DeviceObject.SetFlags(m_DeviceObject.GetFlags() | DO_DIRECT_IO);
            }

            m_ReadWriteIoType = DeviceInit->ReadWriteIoType;
            m_PowerPageableCapable = DeviceInit->PowerPageable;
        }

        //
        // Note: In case of UMDF the StackSize of the MdDeviceObject must
        // be incremented prior to calling AttachDevice. It is because at the
        // time of attaching the device the CWudfDevStack caches the stack
        // size of devnode.
        // This is unlike KMDF where it must be done after AttachDevice.
        // Though the UMDF design can be modified to match KMDF solution, it
        // seems simpler to adjust the stack size of UMDF prior to AttachDevice
        //
        if (m_SelfIoTargetNeeded) {
            SetStackSize(GetStackSize()+1);
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
    // If there was a failure during create (AddDevice), we need to detach
    // from stack so that we don't receive remove irp from host (since fx sm
    // uses a simulated remove event to do remove related cleanup).
    //
    if (m_CleanupFromFailedCreate) {
        if (m_DeviceObject.GetObject() != NULL) {
            DeleteSymbolicLink();

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
            m_DeviceObject.GetObject(), GetObjectHandleUnchecked(),
            m_AttachedDevice.GetObject());

        //
        // This will detach the device from stack. Note that detach may cause
        // the last ref on device object (IWudfDevice) to be released and
        // therefore delete the device object.
        //
        DetachDevice();
    }
}

VOID
FxDevice::DestructorInternal(
    VOID
    )
{
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
        m_DeviceObject.GetObject(), GetObjectHandleUnchecked(),
        m_AttachedDevice.GetObject());

    //
    // This will detach the device from stack. Note that detach may cause
    // the last ref on device object (IWudfDevice) to be released and
    // therefore delete the device object.
    //
    DetachDevice();

    if (m_InteruptThreadpool) {
        delete m_InteruptThreadpool;
        m_InteruptThreadpool = NULL;
    }

    delete [] m_KernelDeviceName;
    m_KernelDeviceName = NULL;

    delete [] m_DeviceKeyPath;
    m_DeviceKeyPath = NULL;

    delete [] m_DeviceInstanceId;
    m_DeviceInstanceId = NULL;

    if (m_PdoDevKey) {
        RegCloseKey(m_PdoDevKey);
        this->m_PdoDevKey = NULL;
    }

    if (m_Dispatcher) {
        delete m_Dispatcher;
        m_Dispatcher = NULL;
    }
}

VOID
FxDevice::GetPreferredTransferMode(
    _In_ MdDeviceObject DeviceObject,
    _Out_ UMINT::WDF_DEVICE_IO_BUFFER_RETRIEVAL *RetrievalMode,
    _Out_ WDF_DEVICE_IO_TYPE *RWPreference,
    _Out_ WDF_DEVICE_IO_TYPE *IoctlPreference
    )
/*++

  Routine Description:

    This method returns the i/o type information for the device to the
    caller.

  Arguments:

    RetrievalMode - the retrival mode desired by this device

    RWPreference - the preferred r/w mode for the device

    IoctlPreference - the preferred ioctl mode for the device

  Return Value:

    None

--*/
{
    FxDevice* device;

    device = GetFxDevice(DeviceObject);

    *RetrievalMode = device->GetRetrievalMode();
    *RWPreference = device->GetPreferredRWTransferMode();
    *IoctlPreference = device->GetPreferredIoctlTransferMode();
}

_Must_inspect_result_
NTSTATUS
FxDevice::PdoInitialize(
    __in PWDFDEVICE_INIT DeviceInit
    )
{
    UNREFERENCED_PARAMETER(DeviceInit);
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
    return STATUS_NOT_IMPLEMENTED;
}


_Must_inspect_result_
NTSTATUS
FxDevice::ControlDeviceInitialize(
    __in PWDFDEVICE_INIT DeviceInit
    )
{
    UNREFERENCED_PARAMETER(DeviceInit);
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

VOID
FxDevice::AddChildList(
    __inout FxChildList* List
    )
{
    UNREFERENCED_PARAMETER(List);
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

VOID
FxDevice::RemoveChildList(
    __inout FxChildList* List
    )
{
    UNREFERENCED_PARAMETER(List);
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

_Must_inspect_result_
NTSTATUS
FxDevice::AllocateDmaEnablerList(
    VOID
    )
{
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

VOID
FxDevice::AddDmaEnabler(
    __inout FxDmaEnabler* Enabler
    )
{
    UNREFERENCED_PARAMETER(Enabler);
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

VOID
FxDevice::RemoveDmaEnabler(
    __inout FxDmaEnabler* Enabler
    )
{
    UNREFERENCED_PARAMETER(Enabler);
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

NTSTATUS
FxDevice::ProcessWmiPowerQueryOrSetData (
    _In_ RdWmiPowerAction   Action,
    _Out_ BOOLEAN *         QueryResult
    )
/*++

  Routine Description:

    This method is called to process WMI set/Query data received from
    reflector. Reflector sends this message when it receives a WMI Query or
    Set Irp for this device for power guids.

  Arguments:

    Action - Enumeration of Set and Query actions for S0Idle and SxWake

    *QueryResult - receives the query result.

  Return Value:

    STATUS_SUCCESS if successful

    STATUS_INVALID_PARAMETER if the parameter values are invalid

--*/
{
    if (Action == ActionInvalid) {
        return STATUS_INVALID_PARAMETER;
    }

    if ((Action == QueryS0Idle || Action == QuerySxWake) && QueryResult == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    switch(Action)
    {
    case SetS0IdleEnable:
        m_PkgPnp->PowerPolicySetS0IdleState(TRUE);
        break;
    case SetS0IdleDisable:
        m_PkgPnp->PowerPolicySetS0IdleState(FALSE);
        break;
    case SetSxWakeEnable:
        m_PkgPnp->PowerPolicySetSxWakeState(TRUE);
        break;
    case SetSxWakeDisable:
        m_PkgPnp->PowerPolicySetSxWakeState(FALSE);
        break;
    case QueryS0Idle:
        *QueryResult = m_PkgPnp->m_PowerPolicyMachine.m_Owner->m_IdleSettings.Enabled;
        break;
    case QuerySxWake:
        *QueryResult = m_PkgPnp->m_PowerPolicyMachine.m_Owner->m_WakeSettings.Enabled;
        break;
    default:
        ASSERT(FALSE);
        break;
    }

    return STATUS_SUCCESS;
}

WUDF_INTERFACE_CONTEXT
FxDevice::RemoteInterfaceArrival (
    _In_    IWudfDevice *   DeviceObject,
    _In_    LPCGUID         DeviceInterfaceGuid,
    _In_    PCWSTR          SymbolicLink
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(DeviceInterfaceGuid);
    UNREFERENCED_PARAMETER(SymbolicLink);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return NULL;
}

void
FxDevice::RemoteInterfaceRemoval (
    _In_    IWudfDevice *   DeviceObject,
    _In_    WUDF_INTERFACE_CONTEXT RemoteInterfaceID
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(RemoteInterfaceID);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

BOOL
FxDevice::TransportQueryId (
    _In_    IWudfDevice *   DeviceObject,
    _In_    DWORD           Id,
    _In_    PVOID           DataBuffer,
    _In_    SIZE_T          cbDataBufferSize
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Id);
    UNREFERENCED_PARAMETER(DataBuffer);
    UNREFERENCED_PARAMETER(cbDataBufferSize);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return FALSE;
}

void
FxDevice::PoFxDevicePowerRequired (
    _In_ MdDeviceObject DeviceObject
    )
{
    GetFxDevice(DeviceObject)->m_PkgPnp->m_PowerPolicyMachine.m_Owner->
         m_PoxInterface.PowerRequiredCallbackInvoked();
}

void
FxDevice::PoFxDevicePowerNotRequired (
    _In_ MdDeviceObject DeviceObject
    )
{
    GetFxDevice(DeviceObject)->m_PkgPnp->m_PowerPolicyMachine.m_Owner->
         m_PoxInterface.PowerNotRequiredCallbackInvoked();
}

NTSTATUS
FxDevice::NtStatusFromHr (
    _In_ IWudfDeviceStack * DevStack,
    _In_ HRESULT Hr
    )
{
    PUMDF_VERSION_DATA driverVersion = DevStack->GetMinDriverVersion();
    BOOL preserveCompat =
         DevStack->ShouldPreserveIrpCompletionStatusCompatibility();

    return CHostFxUtil::NtStatusFromHr(Hr,
                                       driverVersion->MajorNumber,
                                       driverVersion->MinorNumber,
                                       preserveCompat
                                       );
}

NTSTATUS
FxDevice::NtStatusFromHr (
    _In_ HRESULT Hr
    )
{
   return FxDevice::NtStatusFromHr(GetDeviceStack(), Hr);
}

VOID
FxDevice::RetrieveDeviceRegistrySettings(
    VOID
    )
{
    DWORD err;
    HKEY wudfKey = NULL;
    DWORD data;
    DWORD dataSize;

    err = RegOpenKeyEx(m_PdoDevKey,
                       WUDF_SUB_KEY,
                       0,
                       KEY_READ,
                       &wudfKey);
    if (ERROR_SUCCESS != err) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
             "Failed to open hw registry key to read hw access settings");
        goto clean0;
    }

    //
    // Read FX_DIRECT_HARDWARE_ACCESS value
    //
    dataSize = sizeof(data);
    data = 0;
    err = RegQueryValueEx(wudfKey,
                          FX_DIRECT_HARDWARE_ACCESS,
                          NULL,
                          NULL,
                          (BYTE*) &data,
                          &dataSize);

    if (ERROR_SUCCESS == err) {
        if (((WDF_DIRECT_HARDWARE_ACCESS_TYPE)data) < WdfDirectHardwareAccessMax) {
            //
            // save the setting only if it is valid
            //
            m_DirectHardwareAccess = (WDF_DIRECT_HARDWARE_ACCESS_TYPE)data;
        }
        else {
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                     "invalid direct hardware access value in registry %u",
                     (WDF_DIRECT_HARDWARE_ACCESS_TYPE)data);
        }
    }
    else if (ERROR_FILE_NOT_FOUND != err) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
             "Failed to read direct hardware access value in registry");
    }

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
        "DirectHardwareAccess = %u", m_DirectHardwareAccess);


    //
    // Read FX_REGISTER_ACCESS_MODE value
    //
    dataSize = sizeof(data);
    data = 0;
    err = RegQueryValueEx(wudfKey,
                          FX_REGISTER_ACCESS_MODE,
                          NULL,
                          NULL,
                          (BYTE*) &data,
                          &dataSize);
    if (ERROR_SUCCESS == err) {
        if (((WDF_REGISTER_ACCESS_MODE_TYPE)data) < WdfRegisterAccessMax) {
            //
            // save the setting only if it is valid
            //
            m_RegisterAccessMode = (WDF_REGISTER_ACCESS_MODE_TYPE)data;
        }
        else {
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                 "Invalid Register Access mode value in registry %u",
                 (WDF_REGISTER_ACCESS_MODE_TYPE)data);
        }
    }
    else if (ERROR_FILE_NOT_FOUND != err) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
             "Failed to read Register Access mode value in registry");
    }

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
        "RegisterAccessMode = %u", m_RegisterAccessMode);

    //
    // Read FX_FILE_OBJECT_POLICY
    //
    dataSize = sizeof(data);
    data = 0;
    err = RegQueryValueEx(wudfKey,
                          FX_FILE_OBJECT_POLICY,
                          NULL,
                          NULL,
                          (BYTE*) &data,
                          &dataSize);
    if (ERROR_SUCCESS == err) {
        if (((WDF_FILE_OBJECT_POLICY_TYPE)data) < WdfFileObjectPolicyMax) {
            //
            // save the setting only if it is valid
            //
            m_FileObjectPolicy = (WDF_FILE_OBJECT_POLICY_TYPE)data;
        }
        else {
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                 "Invalid File object Policy value in registry %u",
                 (WDF_FILE_OBJECT_POLICY_TYPE)data);
        }
    }
    else if (ERROR_FILE_NOT_FOUND != err) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
             "Failed to read File Object Policy value in registry");
    }

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
        "FileObjectPolicy = %u", m_FileObjectPolicy);

    //
    // Read FX_FS_CONTEXT_USE_POLICY
    //
    dataSize = sizeof(data);
    data = 0;
    err = RegQueryValueEx(wudfKey,
                          FX_FS_CONTEXT_USE_POLICY,
                          NULL,
                          NULL,
                          (BYTE*) &data,
                          &dataSize);
    if (ERROR_SUCCESS == err) {
        if (((WDF_FS_CONTEXT_USE_POLICY_TYPE)data) < WdfFsContextUsePolicyMax) {
            //
            // save the setting only if it is valid
            //
            m_FsContextUsePolicy = (WDF_FS_CONTEXT_USE_POLICY_TYPE)data;
        }
        else {
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                 "Invalid Fs Context Use Policy value in registry %u",
                 (WDF_FILE_OBJECT_POLICY_TYPE)data);
        }
    }
    else if (ERROR_FILE_NOT_FOUND != err) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
             "Failed to read Fs Context Use Policy value in registry");
    }

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
        "FsContextUsePolicy = %u", m_FsContextUsePolicy);


clean0:

    if (NULL != wudfKey) {
        RegCloseKey(wudfKey);
    }

    return;
}

NTSTATUS
FxDevice::_OpenDeviceRegistryKey(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ IWudfDeviceStack* DeviceStack,
    _In_ PWSTR DriverName,
    _In_ ULONG DevInstKeyType,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE DevInstRegKey
    )
{
    NTSTATUS status;
    HRESULT hr;

    UMINT::WDF_PROPERTY_STORE_ROOT root;
    UMINT::WDF_PROPERTY_STORE_RETRIEVE_FLAGS flags = UMINT::WdfPropertyStoreNormal;
    PWSTR subpath = NULL;




    #define WDF_REGKEY_DEVICE_SUBKEY 256
    #define WDF_REGKEY_DRIVER_SUBKEY 256

    root.LengthCb = sizeof(UMINT::WDF_PROPERTY_STORE_ROOT);

    if (DevInstKeyType & PLUGPLAY_REGKEY_CURRENT_HWPROFILE)
    {
        ASSERTMSG("Not available in UMDF\n", FALSE);
    }
    else if (DevInstKeyType & PLUGPLAY_REGKEY_DEVICE)
    {
        root.RootClass = UMINT::WdfPropertyStoreRootClassHardwareKey;

        if (DevInstKeyType & WDF_REGKEY_DEVICE_SUBKEY)
        {
            root.Qualifier.HardwareKey.ServiceName = DriverName;
            flags = UMINT::WdfPropertyStoreCreateIfMissing;
        }
        else
        {
            root.Qualifier.HardwareKey.ServiceName = WDF_PROPERTY_STORE_HARDWARE_KEY_ROOT;
        }
    }
    else if (DevInstKeyType & PLUGPLAY_REGKEY_DRIVER)
    {
        root.RootClass = UMINT::WdfPropertyStoreRootClassSoftwareKey;

        if (DevInstKeyType & WDF_REGKEY_DRIVER_SUBKEY)
        {
            subpath = DriverName;
            flags = UMINT::WdfPropertyStoreCreateIfMissing;
        }
        else
        {
            subpath = L"\\";
        }
    }
    else if (DevInstKeyType & FX_PLUGPLAY_REGKEY_DEVICEMAP)
    {
        root.RootClass = UMINT::WdfPropertyStoreRootClassLegacyHardwareKey;

        //
        // Legacy keys must always be opened volatile for UMDF
        //
        flags = UMINT::WdfPropertyStoreCreateVolatile;

        subpath = NULL;
        root.Qualifier.LegacyHardwareKey.LegacyMapName = DriverName;
    }

    hr = ((IWudfDeviceStack*)DeviceStack)->CreateRegistryEntry(&root,
                                                               flags,
                                                               DesiredAccess,
                                                               subpath,
                                                               (PHKEY)DevInstRegKey,
                                                               NULL);
    status = FxDevice::NtStatusFromHr(DeviceStack, hr);

    return status;
}

NTSTATUS
FxDevice::_GetDeviceProperty(
    _In_       PVOID DeviceStack,
    _In_       DEVICE_REGISTRY_PROPERTY DeviceProperty,
    _In_       ULONG BufferLength,
    _Out_opt_  PVOID PropertyBuffer,
    _Out_opt_  PULONG ResultLength
    )
{
    HRESULT hr;
    NTSTATUS status;

    DEVPROPTYPE propType;
    const DEVPROPKEY *propKey = NULL;

    //
    // {cccccccc-cccc-cccc-cccc-cccccccccccc}
    // 2 brackets + 4 dashes + 32 characters + UNICODE_NULL
    //
    GUID guidBuffer = {0};
    ULONG guidChLen = 2 + 4 + 32 + 1;
    ULONG guidCbLen = guidChLen * sizeof(WCHAR);
    BOOLEAN convertGuidToString = FALSE;

    PVOID buffer = PropertyBuffer;
    ULONG bufferLen = BufferLength;
    ULONG resultLen = 0;

    UMINT::WDF_PROPERTY_STORE_ROOT rootSpecifier;
    rootSpecifier.LengthCb = sizeof(UMINT::WDF_PROPERTY_STORE_ROOT);
    rootSpecifier.RootClass = UMINT::WdfPropertyStoreRootClassHardwareKey;
    rootSpecifier.Qualifier.HardwareKey.ServiceName = NULL;

    //
    // Map DEVICE_REGISTRY_PROPERTY enums to DEVPROPKEYs
    //
    switch (DeviceProperty)
    {
        case DevicePropertyDeviceDescription:
            propKey = &DEVPKEY_Device_DeviceDesc;
            break;
        case DevicePropertyHardwareID:
            propKey = &DEVPKEY_Device_HardwareIds;
            break;
        case DevicePropertyCompatibleIDs:
            propKey = &DEVPKEY_Device_CompatibleIds;
            break;
        case DevicePropertyBootConfiguration:
            ASSERTMSG("Not available in UMDF\n", FALSE);
            break;
        case DevicePropertyBootConfigurationTranslated:
            ASSERTMSG("Not available in UMDF\n", FALSE);
            break;
        case DevicePropertyClassName:
            propKey = &DEVPKEY_Device_Class;
            break;
        case DevicePropertyClassGuid:
            propKey = &DEVPKEY_Device_ClassGuid;
            convertGuidToString = TRUE;
            break;
        case DevicePropertyDriverKeyName:
            propKey = &DEVPKEY_NAME;
            break;
        case DevicePropertyManufacturer:
            propKey = &DEVPKEY_Device_Manufacturer;
            break;
        case DevicePropertyFriendlyName:
            propKey = &DEVPKEY_Device_FriendlyName;
            break;
        case DevicePropertyLocationInformation:
            propKey = &DEVPKEY_Device_LocationInfo;
            break;
        case DevicePropertyPhysicalDeviceObjectName:
            propKey = &DEVPKEY_Device_PDOName;
            break;
        case DevicePropertyBusTypeGuid:
            propKey = &DEVPKEY_Device_BusTypeGuid;
            break;
        case DevicePropertyLegacyBusType:
            propKey = &DEVPKEY_Device_LegacyBusType;
            break;
        case DevicePropertyBusNumber:
            propKey = &DEVPKEY_Device_BusNumber;
            break;
        case DevicePropertyEnumeratorName:
            propKey = &DEVPKEY_Device_EnumeratorName;
            break;
        case DevicePropertyAddress:
            propKey = &DEVPKEY_Device_Address;
            break;
        case DevicePropertyUINumber:
            propKey = &DEVPKEY_Device_UINumber;
            break;
        case DevicePropertyInstallState:
            propKey = &DEVPKEY_Device_InstallState;
            break;
        case DevicePropertyRemovalPolicy:
            propKey = &DEVPKEY_Device_RemovalPolicy;
            break;
        case DevicePropertyResourceRequirements:
            ASSERTMSG("Not available in UMDF\n", FALSE);
            break;
        case DevicePropertyAllocatedResources:
            ASSERTMSG("Not available in UMDF\n", FALSE);
            break;
        case DevicePropertyContainerID:
            propKey = &DEVPKEY_Device_ContainerId;
            convertGuidToString = TRUE;
            break;
    }

    FX_VERIFY(DRIVER(BadArgument, TODO), CHECK_NOT_NULL(propKey));

    if (convertGuidToString)
    {
        buffer = &guidBuffer;
        bufferLen = sizeof(GUID);
    }

    hr = ((IWudfDeviceStack*)DeviceStack)->GetUnifiedPropertyData(&rootSpecifier,
                                                                  propKey,
                                                                  0,
                                                                  0,
                                                                  bufferLen,
                                                                  &propType,
                                                                  &resultLen,
                                                                  buffer);
    if (S_OK == hr)
    {
        status = STATUS_SUCCESS;

        //
        // Some DEVICE_REGISTRY_PROPERTY values are GUID strings,
        // while their DEVPROPKEY equivalents are GUID structs. To preserve
        // KMDF-UMDF DDI parity, we convert select GUID structs to wchar strings.
        //
        if (convertGuidToString)
        {
            if (PropertyBuffer == NULL || BufferLength < guidCbLen)
            {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                hr = StringCchPrintf((PWSTR)PropertyBuffer,
                                     guidChLen,
                                     L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                                     guidBuffer.Data1, guidBuffer.Data2, guidBuffer.Data3,
                                     guidBuffer.Data4[0], guidBuffer.Data4[1],
                                     guidBuffer.Data4[2], guidBuffer.Data4[3],
                                     guidBuffer.Data4[4], guidBuffer.Data4[5],
                                     guidBuffer.Data4[6], guidBuffer.Data4[7]);
                if (hr != S_OK)
                {
                    status = STATUS_UNSUCCESSFUL;
                }
            }
        }
    }
    else if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == hr)
    {
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        status = STATUS_UNSUCCESSFUL;
    }

    if (ResultLength)
    {
        *ResultLength = convertGuidToString ? guidCbLen : resultLen;
    }

    return status;
}

VOID
FxDevice::DetachDevice(
    VOID
    )
{
    //
    // Note that UMDF host's DetachDevice has a different interface than
    // IoDetachDevice. DetachDevice takes the current device object as parameter
    // instead of target device.
    //
    if (m_DevStack != NULL &&  m_DeviceObject.GetObject() != NULL) {
        Mx::MxDetachDevice(m_DeviceObject.GetObject());
        m_AttachedDevice.SetObject(NULL);

        //
        // This was a weak ref. Set it to NULL. m_DeviceObject (IWudfDevice)'s
        // lifetime is managed by host through a ref taken during Attach.
        //
        m_DeviceObject.SetObject(NULL);
    }
}

VOID
FxDevice::InvalidateDeviceState(
    VOID
    )
{
    GetMxDeviceObject()->InvalidateDeviceState(GetDeviceObject());
}

NTSTATUS
FxDevice::CreateSymbolicLink(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ PCUNICODE_STRING SymbolicLinkName
    )
{
    HRESULT hr;
    NTSTATUS status;

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

    hr = GetDeviceStack()->CreateSymbolicLink(SymbolicLinkName->Buffer, NULL);
    if (SUCCEEDED(hr)) {
        status = STATUS_SUCCESS;
    }
    else {
        status = NtStatusFromHr(hr);
        ASSERT(NT_SUCCESS(status) == FALSE);

        FxPoolFree(m_SymbolicLinkName.Buffer);

        RtlZeroMemory(&m_SymbolicLinkName,
                      sizeof(m_SymbolicLinkName));

        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "WDFDEVICE %p  create symbolic link failed, %!STATUS!",
                            GetHandle(), status);
    }

    return status;
}

VOID
FxDevice::DeleteSymbolicLink(
    VOID
    )
{
    if (m_SymbolicLinkName.Buffer != NULL) {
        //
        // There is no IoDeleteSymbolicLink equivalent exposed by UMDF host.
        // Reflector takes care of deleteing the symbolic link on removal.
        // So just free the string now.
        //
        FxPoolFree(m_SymbolicLinkName.Buffer);
        RtlZeroMemory(&m_SymbolicLinkName, sizeof(m_SymbolicLinkName));
    }
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
    HRESULT hr;
    const DEVPROPKEY * propertyKey;
    LCID lcid;
    ULONG flags;

    //
    // call into host to assign the property
    //
    UMINT::WDF_PROPERTY_STORE_ROOT rootSpecifier = {0};

    if (FxPropertyType == FxInterfaceProperty) {
        PWDF_DEVICE_INTERFACE_PROPERTY_DATA interfaceData =
            (PWDF_DEVICE_INTERFACE_PROPERTY_DATA) PropertyData;

        rootSpecifier.LengthCb = sizeof(UMINT::WDF_PROPERTY_STORE_ROOT);
        rootSpecifier.RootClass = UMINT::WdfPropertyStoreRootClassDeviceInterfaceKey;
        rootSpecifier.Qualifier.DeviceInterfaceKey.InterfaceGUID =
            interfaceData->InterfaceClassGUID;
        if (interfaceData->ReferenceString != NULL) {
            rootSpecifier.Qualifier.DeviceInterfaceKey.ReferenceString =
                interfaceData->ReferenceString->Buffer;
        }
        propertyKey = interfaceData->PropertyKey;
        lcid = interfaceData->Lcid;
        flags = interfaceData->Flags;
    }
    else {
        PWDF_DEVICE_PROPERTY_DATA deviceData =
            (PWDF_DEVICE_PROPERTY_DATA) PropertyData;

        ASSERT(FxPropertyType == FxDeviceProperty);

        rootSpecifier.LengthCb = sizeof(UMINT::WDF_PROPERTY_STORE_ROOT);
        rootSpecifier.RootClass = UMINT::WdfPropertyStoreRootClassHardwareKey;
        propertyKey = deviceData->PropertyKey;
        lcid = deviceData->Lcid;
        flags = deviceData->Flags;
    }

    hr = GetDeviceStack()->SetUnifiedPropertyData(&rootSpecifier,
                                                  propertyKey,
                                                  lcid,
                                                  flags,
                                                  Type,
                                                  BufferLength,
                                                  PropertyBuffer);
    if (S_OK == hr) {
        status = STATUS_SUCCESS;
    }
    else {
        status = NtStatusFromHr(hr);
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFDEVICE %p failed to assign interface property, %!STATUS!",
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
    IWudfDeviceStack* deviceStack;
    PWSTR driverName;

    status = FxValidateObjectAttributes(FxDriverGlobals, KeyAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = _ValidateOpenKeyParams(FxDriverGlobals, DeviceInit, Device);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    _Analysis_assume_(DeviceInit != NULL || Device != NULL);

    if (DeviceInit != NULL) {
        deviceStack = DeviceInit->DevStack;
        driverName = DeviceInit->ConfigRegistryPath;
    }
    else {
        deviceStack = Device->m_DevStack;
        driverName = Device->m_DeviceKeyPath;
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
        status = _OpenDeviceRegistryKey(FxDriverGlobals,
                                        deviceStack,
                                        driverName,
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

    status = _OpenDeviceRegistryKey(GetDriverGlobals(),
                                    m_DevStack,
                                    m_DeviceKeyPath,
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
    HRESULT hr;
    DEVPROPTYPE propType;
    ULONG requiredLength = 0;
    const DEVPROPKEY * propertyKey;
    LCID lcid;
    ULONG flags;
    IWudfDeviceStack* devStack;

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
        devStack = DeviceInit->DevStack;
    }
    else {
        devStack = Device->m_DevStack;
    }

    UMINT::WDF_PROPERTY_STORE_ROOT rootSpecifier = {0};

    if (FxPropertyType == FxInterfaceProperty) {
        PWDF_DEVICE_INTERFACE_PROPERTY_DATA interfaceData =
            (PWDF_DEVICE_INTERFACE_PROPERTY_DATA) PropertyData;

        rootSpecifier.LengthCb = sizeof(UMINT::WDF_PROPERTY_STORE_ROOT);
        rootSpecifier.RootClass = UMINT::WdfPropertyStoreRootClassDeviceInterfaceKey;
        rootSpecifier.Qualifier.DeviceInterfaceKey.InterfaceGUID =
            interfaceData->InterfaceClassGUID;
        if (interfaceData->ReferenceString != NULL) {
            rootSpecifier.Qualifier.DeviceInterfaceKey.ReferenceString =
                interfaceData->ReferenceString->Buffer;
        }
        propertyKey = interfaceData->PropertyKey;
        lcid = interfaceData->Lcid;
        flags = interfaceData->Flags;
    }
    else {
        PWDF_DEVICE_PROPERTY_DATA deviceData =
            (PWDF_DEVICE_PROPERTY_DATA) PropertyData;

        ASSERT(FxPropertyType == FxDeviceProperty);

        rootSpecifier.LengthCb = sizeof(UMINT::WDF_PROPERTY_STORE_ROOT);
        rootSpecifier.RootClass = UMINT::WdfPropertyStoreRootClassHardwareKey;
        propertyKey = deviceData->PropertyKey;
        lcid = deviceData->Lcid;
        flags = deviceData->Flags;
    }

    hr = devStack->GetUnifiedPropertyData(&rootSpecifier,
                                               propertyKey,
                                               lcid,
                                               flags,
                                               BufferLength,
                                               &propType,
                                               &requiredLength,
                                               PropertyBuffer);

    if (hr == (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))) {
        status = STATUS_BUFFER_TOO_SMALL;
        *ResultLength = requiredLength;
        *PropertyType = propType;
    }
    else if (hr == S_OK) {
        status = STATUS_SUCCESS;
        *ResultLength = requiredLength;
        *PropertyType = propType;
    }
    else {
        status = NtStatusFromHr(devStack, hr);
        ASSERT(NT_SUCCESS(status) == FALSE);
        DoTraceLevelMessage(DriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Query for unified property buffer failed, %!STATUS!",
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
    IWudfDeviceStack* deviceStack;

    UNREFERENCED_PARAMETER(RemotePdo);

    status = FxDevice::_ValidateOpenKeyParams(FxDriverGlobals,
                                              DeviceInit,
                                              Device);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    _Analysis_assume_(DeviceInit != NULL || Device != NULL);

    if (DeviceInit != NULL) {
        deviceStack = DeviceInit->DevStack;
    }
    else {
        deviceStack = Device->m_DevStack;
    }

    status = _GetDeviceProperty(deviceStack,
                                DeviceProperty,
                                BufferLength,
                                PropertyBuffer,
                                ResultLength);
    return status;
}

_Must_inspect_result_
NTSTATUS
FxDevice::FxValidateInterfacePropertyData(
    _In_ PWDF_DEVICE_INTERFACE_PROPERTY_DATA PropertyData
    )
{
    NTSTATUS status;
    PFX_DRIVER_GLOBALS pFxDriverGlobals = GetDriverGlobals();

    if (PropertyData->Size != sizeof(WDF_DEVICE_INTERFACE_PROPERTY_DATA)) {
        status = STATUS_INFO_LENGTH_MISMATCH;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                     "PropertyData size (%d) incorrect, expected %d, %!STATUS!",
                     PropertyData->Size,
                     sizeof(WDF_DEVICE_INTERFACE_PROPERTY_DATA), status);
        return status;
    }

    FxPointerNotNull(pFxDriverGlobals, PropertyData->InterfaceClassGUID);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return status;
    }

    if (PropertyData->ReferenceString != NULL) {
        status = FxValidateUnicodeString(pFxDriverGlobals,
                                         PropertyData->ReferenceString);
        if (!NT_SUCCESS(status)) {
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
            return status;
        }
    }

    //
    // check if the interface has been registered with WDF
    //
    if (IsInterfaceRegistered(PropertyData->InterfaceClassGUID,
                                       PropertyData->ReferenceString) == FALSE) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFDEVICE %p cannot assign interface property for an interface not"
            " yet registered with WDF, %!STATUS!",
            GetHandle(), status);
        return status;
    }

    return status;
}

VOID
FxDevice::GetDeviceStackIoType (
    _Out_ WDF_DEVICE_IO_TYPE* ReadWriteIoType,
    _Out_ WDF_DEVICE_IO_TYPE* IoControlIoType
    )
{
    UMINT::WDF_DEVICE_IO_BUFFER_RETRIEVAL retrievalMode;

    GetDeviceStack()->GetDeviceStackPreferredTransferMode(
                    &retrievalMode,
                    (UMINT::WDF_DEVICE_IO_TYPE*)ReadWriteIoType,
                    (UMINT::WDF_DEVICE_IO_TYPE*)IoControlIoType
                    );
}

VOID
FxDevice::RetrieveDeviceInfoRegistrySettings(
    _Out_ PCWSTR* GroupId,
    _Out_ PUMDF_DRIVER_REGSITRY_INFO DeviceRegInfo
    )
{
    DWORD Err;
    DWORD Data;
    HKEY wudfKey = NULL;
    DWORD DataSize;
    DWORD type;
    PWSTR buffer;
    DWORD bufferSize;

    ASSERT(GroupId != NULL);
    ASSERT(DeviceRegInfo != NULL);

    ZeroMemory(DeviceRegInfo, sizeof(UMDF_DRIVER_REGSITRY_INFO));
    type = REG_NONE;

    if (m_PdoDevKey == NULL) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                     "Handle to hardware key not yet available");
        return;
    }

    Err = RegOpenKeyEx(m_PdoDevKey,
                       WUDF_SUB_KEY,
                       0,
                       KEY_READ,
                       &wudfKey);
    if (ERROR_SUCCESS != Err) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                     "Failed to open hw registry key to read hw access settings");
        goto clean0;
    }

    //
    // Read WDF_KERNEL_MODE_CLIENT_POLICY value
    //
    DataSize = sizeof(Data);
    Data = 0;
    Err = RegQueryValueEx(wudfKey,
                          FX_KERNEL_MODE_CLIENT_POLICY,
                          NULL,
                          NULL,
                          (BYTE*) &Data,
                          &DataSize);

    if (ERROR_SUCCESS == Err) {
        if (((WDF_KERNEL_MODE_CLIENT_POLICY_TYPE)Data) == WdfAllowKernelModeClients) {
            DeviceRegInfo->IsKernelModeClientAllowed = TRUE;
        }
    }
    else if (ERROR_FILE_NOT_FOUND != Err) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                     "Failed to read kernel mode client policy value in registry");
    }

    //
    // Read WDF_FILE_OBJECT_POLICY value
    //
    DataSize = sizeof(Data);
    Data = 0;
    Err = RegQueryValueEx(wudfKey,
                          FX_FILE_OBJECT_POLICY,
                          NULL,
                          NULL,
                          (BYTE*) &Data,
                          &DataSize);
    if (ERROR_SUCCESS == Err) {
        if (((WDF_FILE_OBJECT_POLICY_TYPE)Data) == WdfAllowNullAndUnknownFileObjects) {
            DeviceRegInfo->IsNullFileObjectAllowed = TRUE;
        }
    }
    else if (ERROR_FILE_NOT_FOUND != Err) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                     "Failed to read file object policy value in registry");
    }

    //
    // Read WDF_METHOD_NEITHER_ACTION value
    //
    DataSize = sizeof(Data);
    Data = 0;
    Err = RegQueryValueEx(wudfKey,
                          FX_METHOD_NEITHER_ACTION,
                          NULL,
                          NULL,
                          (BYTE*) &Data,
                          &DataSize);
    if (ERROR_SUCCESS == Err) {
        if (((WDF_METHOD_NEITHER_ACTION_TYPE)Data) == WdfMethodNeitherAction_Copy) {
            DeviceRegInfo->IsMethodNeitherActionCopy = TRUE;
        }
    }
    else if (ERROR_FILE_NOT_FOUND != Err) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                     "Failed to read method neither action value in registry");
    }

    //
    // Read WDF_PROCESS_SHARING_ENABLED value
    //
    DataSize = sizeof(Data);
    Data = 0;
    Err = RegQueryValueEx(wudfKey,
                          FX_PROCESS_SHARING_ENABLED,
                          NULL,
                          NULL,
                          (BYTE*) &Data,
                          &DataSize);
    if (ERROR_SUCCESS == Err) {
        if (((WDF_PROCESS_SHARING_TYPE)Data) == WdfProcessSharingDisabled) {
            DeviceRegInfo->IsHostProcessSharingDisabled = TRUE;
        }
    }
    else if (ERROR_FILE_NOT_FOUND != Err) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                     "Failed to read method neither action value in registry");
    }

    //
    // Read Group ID
    //
    buffer = NULL;
    bufferSize = 0;
    *GroupId = NULL;

    Err = RegQueryValueEx(wudfKey,
                          FX_DEVICE_GROUP_ID,
                          0,
                          &type,
                          (LPBYTE) buffer,
                          &bufferSize);
    if (ERROR_MORE_DATA == Err) {

        buffer = new WCHAR[bufferSize/sizeof(buffer[0])];
        if (buffer == NULL) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                         "Failed to allocate memory for string buffer");
        }
        else {

            buffer[0] = L'\0';
            Err = RegQueryValueEx(wudfKey,
                                  FX_DEVICE_GROUP_ID,
                                  0,
                                  &type,
                                  (LPBYTE) buffer,
                                  &bufferSize);
            if (Err == ERROR_SUCCESS) {
                if (type != REG_SZ) {
                    Err = ERROR_INVALID_PARAMETER;
                }
                else {
                    //
                    // according to the string data returned by RegQueryValueEx()
                    // is not always null terminated.
                    //
                    buffer[bufferSize/sizeof(buffer[0]) - 1] = L'\0';
                }
            }

            if (Err == ERROR_SUCCESS) {
                *GroupId = buffer;
            }
            else {
                delete [] buffer;
                buffer = NULL;
            }
        }
    }
    else if (ERROR_FILE_NOT_FOUND != Err) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                     "Failed to read Group id value in registry");
    }


clean0:

    if (NULL != wudfKey) {
        RegCloseKey(wudfKey);
    }

    return;
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
    HANDLE hKey = NULL;

    status = _OpenDeviceRegistryKey(pFxDriverGlobals,
                                    m_DevStack,
                                    KeyName->Buffer,
                                    FX_PLUGPLAY_REGKEY_DEVICEMAP,
                                    DesiredAccess,
                                    &hKey);
    if (NT_SUCCESS(status)) {
        pKey->SetHandle(hKey);
    }

    return status;
}

