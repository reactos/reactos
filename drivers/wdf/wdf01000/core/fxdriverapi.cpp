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

    DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, __FUNCTION__" not implemented\r\n");
    return STATUS_UNSUCCESSFUL;
}

} // extern "C"
