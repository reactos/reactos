#include "common/fxtrace.h"
#include "common/fxifr.h"

VOID
FxIFRStop(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
/*++

Routine Description:

    This routine stops the In-Flight Recorder (IFR).

    It should be called as late in the driver teardown as possible
    to allow for the capture of all significant events.

--*/
{
    if (FxDriverGlobals == NULL || FxDriverGlobals->WdfLogHeader == NULL)
    {
        return;
    }

    //
    // Under normal operation the ref count should usually drop to zero when 
    // FxIfrStop is called by FxLibraryCommonUnregisterClient, unless 
    // FxIfrReplay is in the process of making a copy of the IFR buffer. 
    // In which case that thread will call FxIfrStop.
    //
    //
    // Free the Log buffer.
    //
    ExFreePoolWithTag(FxDriverGlobals->WdfLogHeader, WDF_IFR_LOG_TAG);
    FxDriverGlobals->WdfLogHeader = NULL;
}
