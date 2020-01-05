#include "wdf.h"

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
    WDFNOTIMPLEMENTED();
    return STATUS_UNSUCCESSFUL;
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
    WDFNOTIMPLEMENTED();
    return NULL;
}

} // extern "C"