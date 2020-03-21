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

} // extern "C"