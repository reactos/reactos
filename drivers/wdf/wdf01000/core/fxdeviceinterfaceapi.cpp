#include "wdf.h"

extern "C" {

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceCreateDeviceInterface)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    CONST GUID *InterfaceClassGUID,
    __in_opt
    PCUNICODE_STRING ReferenceString
    )
/*++

Routine Description:
    Creates a device interface associated with the passed in device object

Arguments:
    Device - Handle which represents the device exposing the interface

    InterfaceGUID - GUID describing the interface being exposed

    ReferenceString - OPTIONAL string which allows the driver writer to
        distinguish between different exposed interfaces

Return Value:
    STATUS_SUCCESS or appropriate NTSTATUS code

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_UNSUCCESSFUL;
}

}
