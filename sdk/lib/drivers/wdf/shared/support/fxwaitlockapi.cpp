/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxWaitLockAPI.cpp

Abstract:

    This module implements the external APIs for FxWaitLock

Author:



Environment:

   Both kernel and user mode

Revision History:

--*/

#include "FxSupportPch.hpp"

// extern the entire file
extern "C" {


_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfWaitLockCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
     __in_opt
    PWDF_OBJECT_ATTRIBUTES LockAttributes,
    __out
    WDFWAITLOCK* Lock
    )
/*++

Routine Description:
    Creates a lock object which can be acquired at PASSIVE_LEVEL and will return
    to the caller at PASSIVE_LEVEL once acquired.

Arguments:
    LockAttributes - generic attributes to be associated with the created lock

    Lock - pointer to receive the newly created lock

Return Value:
    NTSTATUS

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS  fxDriverGlobals;
    NTSTATUS            status;
    FxObject*           parent;

    fxDriverGlobals = GetFxDriverGlobals(DriverGlobals);
    parent = NULL;

    //
    // Get the parent's globals if it is present
    //
    if (NT_SUCCESS(FxValidateObjectAttributesForParentHandle(fxDriverGlobals,
                                                             LockAttributes))) {
        FxObjectHandleGetPtrAndGlobals(fxDriverGlobals,
                                       LockAttributes->ParentObject,
                                       FX_TYPE_OBJECT,
                                       (PVOID*)&parent,
                                       &fxDriverGlobals);
    }

    FxPointerNotNull(fxDriverGlobals, Lock);

    status = FxValidateObjectAttributes(fxDriverGlobals, LockAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    return FxWaitLock::_Create(fxDriverGlobals,
                               LockAttributes,
                               parent,
                               TRUE,
                               Lock);
}

__drv_when(Timeout != 0, _Must_inspect_result_)
__drv_when(Timeout == 0, __drv_maxIRQL(PASSIVE_LEVEL))
__drv_when(Timeout != 0 && *Timeout == 0, __drv_maxIRQL(DISPATCH_LEVEL))
__drv_when(Timeout != 0 && *Timeout != 0, __drv_maxIRQL(PASSIVE_LEVEL))
NTSTATUS
WDFAPI
WDFEXPORT(WdfWaitLockAcquire)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWAITLOCK Lock,
    __in_opt
    PLONGLONG Timeout
    )
/*++

Routine Description:
    Attempts to acquire the lock object.  If a non NULL timeout is provided, the
    attempt to acquire the lock may fail if it cannot be acquired in the
    specified time.

Arguments:
    Lock - the lock to acquire

    Timeout - optional timeout in acquiring the lock.  If calling at an IRQL >=
              DISPATCH_LEVEL, then this parameter is not NULL (and should more
              then likely be zero)

Return Value:
    STATUS_TIMEOUT if a timeout was provided and the lock could not be acquired
    in the specified time, otherwise STATUS_SUCCESS

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxWaitLock* pLock;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Lock,
                                   FX_TYPE_WAIT_LOCK,
                                   (PVOID*) &pLock,
                                   &pFxDriverGlobals);

    if (Timeout == NULL || *Timeout != 0) {
        status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
        if (!NT_SUCCESS(status)) {
            return status;
        }

    }

    return pLock->AcquireLock(pFxDriverGlobals, Timeout);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
WDFEXPORT(WdfWaitLockRelease)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWAITLOCK Lock
    )
/*++

Routine Description:
    Releases a previously acquired wait lock

Arguments:
    Lock - the lock to release

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxWaitLock* pLock;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Lock,
                                   FX_TYPE_WAIT_LOCK,
                                   (PVOID*) &pLock,
                                   &pFxDriverGlobals);

    pLock->ReleaseLock(pFxDriverGlobals);
}

} // extern "C"
