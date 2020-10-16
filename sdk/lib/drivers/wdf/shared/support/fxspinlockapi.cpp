/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxSpinLockAPI.cpp

Abstract:

    This module implements external APIS to access FxSpinLock

Author:



Environment:

   Both kernel and user mode

Revision History:

--*/

#include "fxsupportpch.hpp"
#include "fxspinlock.hpp"

extern "C" {
// #include "FxSpinLockAPI.tmh"
}

//
// Extern the entire file
//
extern "C" {

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfSpinLockCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES SpinLockAttributes,
    __out
    WDFSPINLOCK* SpinLock
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxSpinLock* pLock;
    WDFSPINLOCK lock;
    USHORT extra;

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);

    //
    // Get the parent's globals if it is present
    //
    if (NT_SUCCESS(FxValidateObjectAttributesForParentHandle(
                        pFxDriverGlobals, SpinLockAttributes))) {
        FxObject* pParent;

        FxObjectHandleGetPtrAndGlobals(pFxDriverGlobals,
                                       SpinLockAttributes->ParentObject,
                                       FX_TYPE_OBJECT,
                                       (PVOID*)&pParent,
                                       &pFxDriverGlobals);
    }

    FxPointerNotNull(pFxDriverGlobals, SpinLock);

    status = FxValidateObjectAttributes(pFxDriverGlobals, SpinLockAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (pFxDriverGlobals->FxVerifierLock) {
        extra = sizeof(FX_SPIN_LOCK_HISTORY);
    }
    else {
        extra = 0;
    }

    *SpinLock = NULL;

    pLock = new(pFxDriverGlobals, SpinLockAttributes, extra)
        FxSpinLock(pFxDriverGlobals, extra);

    if (pLock == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = pLock->Commit(SpinLockAttributes, (WDFOBJECT*)&lock);

    if (NT_SUCCESS(status)) {
        *SpinLock = lock;
    }
    else {
        pLock->DeleteFromFailedCreate();
    }

    return status;
}

__drv_raisesIRQL(DISPATCH_LEVEL)
__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfSpinLockAcquire)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    __drv_savesIRQL
    _Requires_lock_not_held_(_Curr_)
    _Acquires_lock_(_Curr_)
    WDFSPINLOCK SpinLock
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxSpinLock* pLock;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   SpinLock,
                                   FX_TYPE_SPIN_LOCK,
                                   (PVOID*) &pLock,
                                   &pFxDriverGlobals);

    if (pLock->IsInterruptLock()) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                            "WDFSPINLOCK %p is associated with an interrupt, "
                            "cannot be used for normal sync operations",
                            SpinLock);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    pLock->AcquireLock(
        pFxDriverGlobals->FxVerifierLock ? _ReturnAddress() : NULL);
}

__drv_maxIRQL(DISPATCH_LEVEL)
__drv_minIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfSpinLockRelease)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    __drv_restoresIRQL
    _Requires_lock_held_(_Curr_)
    _Releases_lock_(_Curr_)
    WDFSPINLOCK SpinLock
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxSpinLock* pLock;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   SpinLock,
                                   FX_TYPE_SPIN_LOCK,
                                   (PVOID*) &pLock,
                                   &pFxDriverGlobals);

    if (pLock->IsInterruptLock()) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                            "WDFSPINLOCK %p is associated with an interrupt, "
                            "cannot be used for normal sync operations",
                            SpinLock);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    pLock->ReleaseLock();
}

} // extern "C"
