#include "common/fxglobals.h"
#include "common/dbgtrace.h"


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
    DDI_ENTRY_IMPERSONATION_OK();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);

    if (pFxDriverGlobals->FxVerifierDbgBreakOnError)
    {
        DbgBreakPoint();
    }
    else
    {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGDRIVER,
            "DbgBreakOnError registry value wasn't set, ignoring WdfVerifierDbgBreakPoint");
    }
}

} // extern "C"
