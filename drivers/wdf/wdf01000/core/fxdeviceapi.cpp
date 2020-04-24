#include "common/fxglobals.h"
#include "common/fxdevice.h"
#include "common/fxdeviceinit.h"
#include "common/fxvalidatefunctions.h"

extern "C" {

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __inout
    PWDFDEVICE_INIT* DeviceInit,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES DeviceAttributes,
    __out
    WDFDEVICE* Device
    )
{
    DDI_ENTRY();
        
    FxDevice* pDevice;
    NTSTATUS status;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, DeviceInit);
    FxPointerNotNull(pFxDriverGlobals, *DeviceInit);
    FxPointerNotNull(pFxDriverGlobals, Device);

    //
    // Use the object's globals, not the caller's globals
    //
    pFxDriverGlobals = (*DeviceInit)->DriverGlobals;

    *Device = NULL;

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //
    // Make sure the device attributes is initialized properly if passed in
    //
    status = FxValidateObjectAttributes(pFxDriverGlobals,
                                        DeviceAttributes,
                                        (FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED |
                                        FX_VALIDATE_OPTION_EXECUTION_LEVEL_ALLOWED |
                                        FX_VALIDATE_OPTION_SYNCHRONIZATION_SCOPE_ALLOWED));

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if ((*DeviceInit)->CreatedDevice != NULL)
    {
        //
        // Already created the device!
        //
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "WDFDEVICE 0x%p   already created"
                            "STATUS_INVALID_DEVICE_STATE",
                            Device);

        return STATUS_INVALID_DEVICE_STATE;
    }

    //
    // If security is specified, then the device being created *must* have a
    // name to apply that security too.
    //
    if ((*DeviceInit)->Security.Sddl != NULL || (*DeviceInit)->Security.DeviceClassSet)
    {
        if ((*DeviceInit)->HasName())
        {
            //
            // Driver writer specified a name, all is good
            //
            DO_NOTHING();
        }
        else
        {
            status = STATUS_INVALID_SECURITY_DESCR;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "Device init: has device class or SDDL set, but does not have "
                "a name, %!STATUS!", status);

            return status;
        }
    }

    if ((*DeviceInit)->RequiresSelfIoTarget)
    {
        if ((*DeviceInit)->InitType != FxDeviceInitTypeFdo)
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "Client called WdfDeviceInitAllowSelfTarget. Self "
                "IO Targets are supported only for FDOs, %!STATUS!", status);
            return status;
        }
    }

    status = FxDevice::_Create(pFxDriverGlobals,
                               DeviceInit,
                               DeviceAttributes,
                               &pDevice);
    if (NT_SUCCESS(status))
    {
        *Device = pDevice->GetHandle();
    }

    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFQUEUE
WDFEXPORT(WdfDeviceGetDefaultQueue)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )

/*++

Routine Description:

    Return the handle to the default queue for the device.

Arguments:

    Device - Handle to the Device Object

Returns:

    WDFQUEUE

--*/

{
    DDI_ENTRY();
        
    FxPkgIo*   pPkgIo;;
    FxIoQueue* pFxIoQueue;;
    FxDevice * pFxDevice;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pPkgIo = NULL;
    pFxIoQueue = NULL;

    //
    // Validate the I/O Package handle, and get the FxPkgIo*
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pFxDevice,
                                   &pFxDriverGlobals);

    pPkgIo = (FxPkgIo *) pFxDevice->m_PkgIo;
    pFxIoQueue = pPkgIo->GetDefaultQueue();

    //
    // A default queue is optional
    //
    if (pFxIoQueue == NULL)
    {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGIO,
                            "No default Queue configured "
                            "for Device 0x%p", Device);
        return NULL;
    }

    return (WDFQUEUE)pFxIoQueue->GetObjectHandle();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceGetDeviceState)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __out
    PWDF_DEVICE_STATE DeviceState
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceSetDeviceState)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_DEVICE_STATE DeviceState
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
WDFEXPORT(WdfWdmDeviceGetWdfDeviceHandle)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PDEVICE_OBJECT DeviceObject
    )
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
PDEVICE_OBJECT
WDFEXPORT(WdfDeviceWdmGetDeviceObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
PDEVICE_OBJECT
WDFEXPORT(WdfDeviceWdmGetAttachedDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
PDEVICE_OBJECT
WDFEXPORT(WdfDeviceWdmGetPhysicalDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceWdmDispatchPreprocessedIrp)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    MdIrp Irp
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceAddDependentUsageDeviceObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PDEVICE_OBJECT DependentDevice
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceAddRemovalRelationsPhysicalDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PDEVICE_OBJECT PhysicalDevice
    )
/*++

Routine Description:
    Registers a PDO from another non descendant (not verifiable though) pnp
    stack to be reported as also requiring removal when this PDO is removed.

    The PDO could be another device enumerated by this driver.

Arguments:
    Device - this driver's PDO

    PhysicalDevice - PDO for another stack

Return Value:
    NTSTATUS

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceRemoveRemovalRelationsPhysicalDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PDEVICE_OBJECT PhysicalDevice
    )
/*++

Routine Description:
    Deregisters a PDO from another non descendant (not verifiable though) pnp
    stack to not be reported as also requiring removal when this PDO is removed.

    The PDO could be another device enumerated by this driver.

Arguments:
    Device - this driver's PDO

    PhysicalDevice - PDO for another stack

Return Value:
    None

  --*/
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceClearRemovalRelationsDevices)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
/*++

Routine Description:
    Deregisters all PDOs to not be reported as also requiring removal when this
    PDO is removed.

Arguments:
    Device - this driver's PDO

Return Value:
    None

  --*/
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFDRIVER
WDFEXPORT(WdfDeviceGetDriver)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
/*++

Routine Description:

    Given a Device Handle, return a Handle to the Driver object
    containing this device.

Arguments:
    Device - WDF Device handle.

Returns:
    WDF Driver handle.

--*/

{
    WDFNOTIMPLEMENTED();
    return NULL;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceRetrieveDeviceName)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    WDFSTRING String
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceAssignMofResourceName)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PCUNICODE_STRING MofResourceName
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFIOTARGET
WDFEXPORT(WdfDeviceGetIoTarget)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDF_DEVICE_PNP_STATE
WDFEXPORT(WdfDeviceGetDevicePnpState)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    WDFNOTIMPLEMENTED();
    return WDF_DEVICE_PNP_STATE::WdfDevStatePnpInvalid;
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDF_DEVICE_POWER_STATE
WDFEXPORT(WdfDeviceGetDevicePowerState)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    WDFNOTIMPLEMENTED();
    return WDF_DEVICE_POWER_STATE::WdfDevStatePowerInvalid;
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDF_DEVICE_POWER_POLICY_STATE
WDFEXPORT(WdfDeviceGetDevicePowerPolicyState)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    WDFNOTIMPLEMENTED();
    return WDF_DEVICE_POWER_POLICY_STATE::WdfDevStatePwrPolInvalid;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceAssignS0IdleSettings)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_DEVICE_POWER_POLICY_IDLE_SETTINGS Settings
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceAssignSxWakeSettings)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_DEVICE_POWER_POLICY_WAKE_SETTINGS Settings
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceOpenRegistryKey)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    ULONG DeviceInstanceKeyType,
    __in
    ACCESS_MASK DesiredAccess,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES KeyAttributes,
    __out
    WDFKEY* Key
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceSetCharacteristics)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    ULONG DeviceCharacteristics
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
ULONG
WDFEXPORT(WdfDeviceGetCharacteristics)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    WDFNOTIMPLEMENTED();
    return 0;
}

__drv_maxIRQL(DISPATCH_LEVEL)
ULONG
WDFEXPORT(WdfDeviceGetAlignmentRequirement)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    WDFNOTIMPLEMENTED();
    return 0;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceSetAlignmentRequirement)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    ULONG AlignmentRequirement
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceSetSpecialFileSupport)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    WDF_SPECIAL_FILE_TYPE FileType,
    __in
    BOOLEAN Supported
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceSetStaticStopRemove)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    BOOLEAN Stoppable
    )
{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceCreateSymbolicLink)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PCUNICODE_STRING SymbolicLinkName
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceQueryProperty)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    DEVICE_REGISTRY_PROPERTY  DeviceProperty,
    __in
    ULONG BufferLength,
     __out_bcount_full(BufferLength)
    PVOID PropertyBuffer,
    __out
    PULONG ResultLength
    )
/*++

Routine Description:
    Retrieves the requested device property for the given device

Arguments:
    Device - the device whose PDO whose will be queried

    DeviceProperty - the property being queried

    BufferLength - length of PropertyBuffer in bytes

    PropertyBuffer - Buffer which will receive the property being queried

    ResultLength - if STATUS_BUFFER_TOO_SMALL is returned, then this will contain
                   the required length

Return Value:
    NTSTATUS

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceAllocAndQueryProperty)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    DEVICE_REGISTRY_PROPERTY  DeviceProperty,
    __in
    __drv_strictTypeMatch(__drv_typeExpr)
    POOL_TYPE PoolType,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES PropertyMemoryAttributes,
    __out
    WDFMEMORY* PropertyMemory
    )
/*++

Routine Description:
    Allocates and retrieves the requested device property for the given device

Arguments:
    Device - the pnp device whose PDO whose will be queried

    DeviceProperty - the property being queried

    PoolType - what type of pool to allocate

    PropertyMemoryAttributes - attributes to associate with PropertyMemory

    PropertyMemory - handle which will receive the property buffer

Return Value:
    NTSTATUS

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceSetPnpCapabilities)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_DEVICE_PNP_CAPABILITIES PnpCapabilities
    )
/*++

Routine Description:
    Sets the pnp capabilities of the device.  This function is usually called
    during AddDevice or EvtDevicePrepareHardware since these values are queried
    by the stack and pnp during start device processing or immediately afterwards.

Arguments:
    Device - Device being set

    PnpCapabilities - Caps being set

Return Value:
    None

  --*/
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceSetPowerCapabilities)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_DEVICE_POWER_CAPABILITIES PowerCapabilities
    )
/*++

Routine Description:
    Sets the power capabilities of the device.  This function is usually called
    during AddDevice or EvtDevicePrepareHardware since these values are queried
    by the stack and power during start device processing or immediately afterwards.

Arguments:
    Device - Device being set

    PowerCapabilities - Caps being set

Return Value:
    None

  --*/
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceSetBusInformationForChildren)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PPNP_BUS_INFORMATION BusInformation
    )
{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceIndicateWakeStatus)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    NTSTATUS WaitWakeStatus
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceSetFailed)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    WDF_DEVICE_FAILED_ACTION FailedAction
    )
{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_when(WaitForD0 == 0, __drv_maxIRQL(DISPATCH_LEVEL))
__drv_when(WaitForD0 != 0, __drv_maxIRQL(PASSIVE_LEVEL))
NTSTATUS
WDFEXPORT(WdfDeviceStopIdle)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    BOOLEAN WaitForD0
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceResumeIdle)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFFILEOBJECT
WDFEXPORT(WdfDeviceGetFileObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    MdFileObject FileObject
    )
/*++

Routine Description:

    This functions returns the WDFFILEOBJECT corresponding to the WDM fileobject.

Arguments:

    Device - Handle to the device to which the WDM fileobject is related to.

    FileObject - WDM FILE_OBJECT structure.

Return Value:

--*/

{
    WDFNOTIMPLEMENTED();
    return NULL;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceEnqueueRequest)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE  Device,
    __in
    WDFREQUEST Request
    )

/*++

Routine Description:

    Inserts a request into the I/O Package processing pipeline.

    The given request is processed by the I/O package, forwarding
    it to its configured destination or the default queue.

    If an EvtIoInCallerContext is callback is registered, it is not called.

Arguments:

    Device  - Handle to the Device Object

    Request - Request to insert in the processing pipeline

Return Value:

    STATUS_SUCCESS - Request has been inserted into the I/O processing pipeline

    !NT_SUCCESS(status)
    STATUS_WDF_BUSY - Device is not accepting requests, the driver still owns the
                     the request and must complete it, or correct the conditions.

--*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceConfigureRequestDispatching)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    WDFQUEUE Queue,
    __in
    __drv_strictTypeMatch(__drv_typeCond)
    WDF_REQUEST_TYPE RequestType
    )

/*++

Routine Description:

    Configure which IRP_MJ_* requests are automatically
    forwarded by the I/O package to the specified Queue.

    By default the I/O package sends all requests to the
    devices default queue. This API allows certain requests
    to be specified to their own queues under driver control.

Arguments:

    Device    - The device which is handling the IO.

    Queue     - I/O Queue object to forward specified request to.

    RequestType - WDF Request type to be forwarded to the queue

Returns:

    NTSTATUS

--*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceRemoveDependentUsageDeviceObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PDEVICE_OBJECT DependentDevice
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
POWER_ACTION
WDFEXPORT(WdfDeviceGetSystemPowerAction)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )

/*++

Routine Description:

    This DDI returns the System power action.

    If the DDI is called in the power down path of a device, it will return the
    current system power transition type because of which the device is powering
    down.  If the DDI is called in the power up path of a device, it will return
    the system power transition type which initially put the device into a lower
    power state.  If the DDI is called during the normal functioning of a device
    that is not undergoing a power state transition, or if the device is powering
    up as part of the system start up, the DDI will return the PowerActionNone
    value.

Arguments:

    Device  - Handle to the Device Object

Return Value:

    Returns a POWER_ACTION enum value that represents the current system power
    action with regards to any system power transition underway.

--*/

{
    WDFNOTIMPLEMENTED();
    return POWER_ACTION::PowerActionNone;
}

} // extern "C"