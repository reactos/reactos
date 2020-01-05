#include "wdf.h"
#include "common/mxgeneral.h"

//
// extern the whole file
//
extern "C" {

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDriverCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    MdDriverObject DriverObject,
    __in
    PCUNICODE_STRING RegistryPath,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES DriverAttributes,
    __in
    PWDF_DRIVER_CONFIG DriverConfig,
    __out_opt
    WDFDRIVER* Driver
    )
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);
    UNREFERENCED_PARAMETER(DriverAttributes);
    UNREFERENCED_PARAMETER(DriverConfig);
    UNREFERENCED_PARAMETER(Driver);

    WDFNOTIMPLEMENTED();
    return STATUS_UNSUCCESSFUL;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDriverRetrieveVersionString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDRIVER Driver,
    __in
    WDFSTRING String
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_UNSUCCESSFUL;
}


_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
BOOLEAN
WDFEXPORT(WdfDriverIsVersionAvailable)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDRIVER Driver,
    __in
    PWDF_DRIVER_VERSION_AVAILABLE_PARAMS VersionAvailableParams
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_UNSUCCESSFUL;
}


} // extern "C"
