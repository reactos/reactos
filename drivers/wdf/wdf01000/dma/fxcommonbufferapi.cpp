#include "wdf.h"


extern "C" {

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfCommonBufferCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler,
    __in
    __drv_when(Length == 0, __drv_reportError(Length cannot be zero))
    size_t Length,
    __in_opt
    WDF_OBJECT_ATTRIBUTES * Attributes,
    __out
    WDFCOMMONBUFFER * CommonBufferHandle
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfCommonBufferCreateWithConfig)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler,
    __in
    __drv_when(Length == 0, __drv_reportError(Length cannot be zero))
    size_t Length,
    __in
    PWDF_COMMON_BUFFER_CONFIG Config,
    __in_opt
    WDF_OBJECT_ATTRIBUTES * Attributes,
    __out
    WDFCOMMONBUFFER * CommonBufferHandle
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
PVOID
WDFEXPORT(WdfCommonBufferGetAlignedVirtualAddress)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOMMONBUFFER CommonBuffer
    )
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
PHYSICAL_ADDRESS
WDFEXPORT(WdfCommonBufferGetAlignedLogicalAddress)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOMMONBUFFER CommonBuffer
    )
{
    LARGE_INTEGER res = {0};
    WDFNOTIMPLEMENTED();
    return res;
}

__drv_maxIRQL(DISPATCH_LEVEL)
size_t
WDFEXPORT(WdfCommonBufferGetLength)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOMMONBUFFER CommonBuffer
    )
{
    WDFNOTIMPLEMENTED();
    return 0;
}

} // extern "C"
