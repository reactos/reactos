#include "wdf.h"



extern "C" {

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfControlFinishInitializing)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    WDFNOTIMPLEMENTED();
}

} //extern "C"
