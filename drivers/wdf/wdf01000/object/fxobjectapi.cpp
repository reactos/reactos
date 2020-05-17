#include "common/fxglobals.h"
#include "common/fxobject.h"

extern "C" {

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI    
VOID
NTAPI
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

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI    
VOID
WDFEXPORT(WdfObjectReferenceActual)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFOBJECT Object,
    __in_opt
    PVOID Tag,
    __in
    LONG Line,
    __in
    PCSTR File
    )
/*++

Routine Description:
    Adds an explicit reference that was taken on an object.

Arguments:
    Object - the object to reference
    Tag - The tag used to track this reference.  If not matched on dereference,
          a breakpoint is hit.  Tags are only tracked when enabled via the
          registry or WDF verifier.
    Line - the caller's line number making the call
    File - the caller's file name making the call

Return Value:
    None.  We do not return the current reference count.

  --*/
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI    
VOID
WDFEXPORT(WdfObjectDereferenceActual)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFOBJECT Object,
    __in_opt
    PVOID Tag,
    __in
    LONG Line,
    __in
    PCSTR File
    )
/*++

Routine Description:
    Removes an explicit reference that was taken on an object.

Arguments:
    Object - the object to dereference
    Tag - The tag used when referencing the Object, if not matched, a breakpoint
          is hit.  Tags are only tracked when enabled via the registry or
          WDF verifier.
    Line - the caller's line number making the call
    File - the caller's file name making the call

Return Value:
    None.  We do not return the current reference count.

  --*/
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI    
VOID
WDFEXPORT(WdfObjectAcquireLock)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    _Requires_lock_not_held_(_Curr_)
    _Acquires_lock_(_Curr_)
    WDFOBJECT Object
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI    
VOID
WDFEXPORT(WdfObjectReleaseLock)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    _Requires_lock_held_(_Curr_)
    _Releases_lock_(_Curr_)
    WDFOBJECT Object
    )
{
    WDFNOTIMPLEMENTED();
}

} // extern "C"
