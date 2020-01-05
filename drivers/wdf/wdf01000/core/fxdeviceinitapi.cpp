#include "wdf.h"

extern "C" {

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceInitSetPnpPowerEventCallbacks)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PWDF_PNPPOWER_EVENT_CALLBACKS PnpPowerEventCallbacks
    )
{
    WDFNOTIMPLEMENTED();    
}

} // extern "C"