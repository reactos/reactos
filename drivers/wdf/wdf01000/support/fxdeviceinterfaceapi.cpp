#include "wdf.h"


extern "C" {

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfDeviceSetDeviceInterfaceState)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    CONST GUID *InterfaceClassGUID,
    __in_opt
    PCUNICODE_STRING RefString,
    __in
    BOOLEAN State
    )
{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceRetrieveDeviceInterfaceString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    CONST GUID* InterfaceClassGUID,
    __in_opt
    PCUNICODE_STRING RefString,
    __in
    WDFSTRING String
    )
/*++

Routine Description:
    Returns the symbolic link value of the registered device interface.

Arguments:


Return Value:


  --*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

} // extern "C"
