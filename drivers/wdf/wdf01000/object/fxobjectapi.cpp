#include "common/fxglobals.h"
#include "common/fxobject.h"

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
    DDI_ENTRY();
    
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxObject* pObject;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Object,
                                   FX_TYPE_OBJECT,
                                   (PVOID*)&pObject,
                                   &pFxDriverGlobals);

    //
    // The object may not allow the Delete DDI
    //
    if (pObject->IsNoDeleteDDI())
    {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Attempt to Delete an Object Which does not allow WdfDeleteObject "
            "Handle 0x%p, %!STATUS!",
            Object, STATUS_CANNOT_DELETE);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
    }
    else
    {
        pObject->DeleteObject();
    }
}

} // extern "C"
