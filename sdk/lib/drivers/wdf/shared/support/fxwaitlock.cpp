/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxWaitLock.cpp

Abstract:

    This module implements the FxWaitLock's factory method.

Author:


Revision History:


--*/

#include "FxSupportPch.hpp"

#if defined(EVENT_TRACING)
// Tracing support
extern "C" {
#include "FxWaitLock.tmh"
}
#endif

__checkReturn
NTSTATUS
FxWaitLock::_Create(
    __in PFX_DRIVER_GLOBALS         FxDriverGlobals,
    __in_opt PWDF_OBJECT_ATTRIBUTES Attributes,
    __in_opt FxObject*              ParentObject,
    __in BOOLEAN                    AssignDriverAsDefaultParent,
    __out WDFWAITLOCK*              LockHandle
    )
{
    FxWaitLock*     lock;
    NTSTATUS        status;

    *LockHandle = NULL;

    lock = new (FxDriverGlobals, Attributes) FxWaitLock(FxDriverGlobals);
    if (lock == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "Memory allocation failed: %!STATUS!", status);
        return status;
    }

    status = lock->Initialize();
    if (!NT_SUCCESS(status)) {
        lock->DeleteFromFailedCreate();
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "faield to initialize wait lock: %!STATUS!", status);
        return status;
    }

    status = lock->Commit(Attributes,
                          (WDFOBJECT*)LockHandle,
                          ParentObject,
                          AssignDriverAsDefaultParent);

    if (!NT_SUCCESS(status)) {
        lock->DeleteFromFailedCreate();
    }

    return status;
}



