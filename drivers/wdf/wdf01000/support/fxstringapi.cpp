#include "wdf.h"

extern "C" {

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfStringCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    PCUNICODE_STRING UnicodeString,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES StringAttributes,
    __out
    WDFSTRING* String
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_UNSUCCESSFUL;
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfStringGetUnicodeString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFSTRING String,
    __out
    PUNICODE_STRING UnicodeString
    )
{
    WDFNOTIMPLEMENTED();
}

} // extern "C"
