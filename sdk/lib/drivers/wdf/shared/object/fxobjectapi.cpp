/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxObjectApi.cpp

Abstract:


Author:


Environment:

    kernel mode only

Revision History:

--*/

#include "fxobjectpch.hpp"

extern "C" {

#if defined(EVENT_TRACING)
#include "FxObjectAPI.tmh"
#endif

}


extern "C" {

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
    PSTR File
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
    DDI_ENTRY();

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), Object);

    FxObject::_ReferenceActual(
        Object,
        Tag,
        Line,
        File
        );
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
    PSTR File
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
    DDI_ENTRY();

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), Object);

    FxObject::_DereferenceActual(
        Object,
        Tag,
        Line,
        File
        );
}

FxCallbackLock*
FxGetCallbackLock(
    FxObject* Object
    )

/*++

Routine Description:

    This returns the proper FxCallbackLock pointer
    for the object.

    It returns NULL if an FxCallbackLock is not valid
    for the object.

Arguments:

    Object - Pointer to object to retrieve the callback lock pointer for

Returns:

    FxCallbackLock*

--*/

{
    NTSTATUS status;
    IFxHasCallbacks* ihcb;
    FxQueryInterfaceParams params = { (PVOID*) &ihcb, FX_TYPE_IHASCALLBACKS, 0 };

    ihcb = NULL;

    //
    // Query the object for the IFxHasCallbacks interface. Objects that
    // have callback locks must support this interface.
    //
    status = Object->QueryInterface(&params);
    if (!NT_SUCCESS(status)) {
        return NULL;
    }

    return ihcb->GetCallbackLockPtr(NULL);
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
    DDI_ENTRY();

    FxObject* pObject;
    FxCallbackLock* pLock;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    KIRQL irql;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Object,
                                   FX_TYPE_OBJECT,
                                   (PVOID*)&pObject,
                                   &pFxDriverGlobals);

    //
    // If Lock Verifier on, validate whether its correct to
    // make this call
    //
    if (pFxDriverGlobals->FxVerifierLock) {
        //
        // Check IRQL Level, etc.
        //
    }

    //
    // Get the CallbackLock for the object
    //
    pLock = FxGetCallbackLock(pObject);

    if (pLock == NULL) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Invalid to call on WDFOBJECT 0x%p", Object);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        FxVerifierBugCheck(pFxDriverGlobals,
                           WDF_INVALID_LOCK_OPERATION,
                           (ULONG_PTR)Object,
                           (ULONG_PTR)NULL
                           );
        return;
    }

    pLock->Lock(&irql);
    pLock->m_PreviousIrql = irql;

    return;
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
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxObject* pObject;
    FxCallbackLock* pLock;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Object,
                                   FX_TYPE_OBJECT,
                                   (PVOID*)&pObject,
                                   &pFxDriverGlobals);

    //
    // If Lock Verifier on, validate whether its correct to
    // make this call
    //
    if (pFxDriverGlobals->FxVerifierLock) {
        //
        // Check IRQL Level, etc.
        //
    }

    //
    // Get the CallbackLock for the object
    //
    pLock = FxGetCallbackLock(pObject);
    if (pLock == NULL) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Invalid to call on WDFOBJECT 0x%p",Object);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        FxVerifierBugCheck(pFxDriverGlobals,
                           WDF_INVALID_LOCK_OPERATION,
                           (ULONG_PTR)Object,
                           (ULONG_PTR)NULL
                           );
        return;
    }

    pLock->Unlock(pLock->m_PreviousIrql);

    return;
}

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
    if (pObject->IsNoDeleteDDI()) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Attempt to Delete an Object Which does not allow WdfDeleteObject "
            "Handle 0x%p, %!STATUS!",
            Object, STATUS_CANNOT_DELETE);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
    }
    else {
        pObject->DeleteObject();
    }
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfObjectQuery)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFOBJECT Object,
    __in
    CONST GUID* Guid,
    __in
    ULONG QueryBufferLength,
    __out_bcount(QueryBufferLength)
    PVOID QueryBuffer
    )

/*++

Routine Description:

    Query the object handle for specific information

    This allows dynamic extensions to DDI's.

    Currently, it is used to allow test hooks for verification
    which are not available in a production release.

Arguments:

    Object - Handle to object for the query

    Guid - GUID to represent the information/DDI to query for

    QueryBufferLength - Length of QueryBuffer to return data in

    QueryBuffer - Pointer to QueryBuffer

Returns:

    NTSTATUS

--*/

{
    DDI_ENTRY();

    FxObject* p;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Object,
                         FX_TYPE_OBJECT,
                         (PVOID*)&p);

    return FxObject::_ObjectQuery(p,
                                  Guid,
                                  QueryBufferLength,
                                  QueryBuffer);
}

} // extern "C" for all functions


