#include "wdf.h"

extern "C" {

VOID
WDFEXPORT(WdfVerifierDbgBreakPoint)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals
    )

/*++

Routine Description:

    Common Driver Frameworks DbgBreakPoint() function.

    This will only break point if WdfVerifierDbgBreakOnError is defined, so
    it's safe to call for production systems.

Arguments:

    DriverGlobals -

Return Value:

    None.

--*/

{
    WDFNOTIMPLEMENTED();
}

} // extern "C"
