#include "wdf.h"

extern "C" {

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI    
VOID
WDFEXPORT(WdfObjectDelete)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFOBJECT Object
    )
{
    WDFNOTIMPLEMENTED();
}

} // extern "C"
