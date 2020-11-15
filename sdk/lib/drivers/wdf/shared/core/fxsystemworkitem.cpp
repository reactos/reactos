/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxSystemWorkItem.hpp

Abstract:

    This module implements a frameworks managed WORKITEM that
    can synchrononize with driver frameworks object locks.

Author:



Environment:

    Both kernel and user mode

Revision History:


--*/

#include "coreprivshared.hpp"

#include "fxsystemworkitem.hpp"

// Tracing support
extern "C" {
// #include "FxSystemWorkItem.tmh"
}

//
// Public constructors
//
_Must_inspect_result_
NTSTATUS
FxSystemWorkItem::_Create(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PVOID              WdmObject,
    __out FxSystemWorkItem** pObject
    )
{
    NTSTATUS status;
    FxSystemWorkItem* wi;

    wi = new(FxDriverGlobals) FxSystemWorkItem(FxDriverGlobals);
    if (wi == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = wi->Initialize(WdmObject);
    if (!NT_SUCCESS(status)) {
        wi->Release();
        return status;
    }

    *pObject = wi;

    return status;
}

FxSystemWorkItem::FxSystemWorkItem(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    ) :
    FxNonPagedObject(FX_TYPE_SYSTEMWORKITEM, 0, FxDriverGlobals),
    m_WorkItemCompleted(NotificationEvent, TRUE),
    m_RemoveEvent(SynchronizationEvent, FALSE)
{
    m_RunningDown = FALSE;
    m_Enqueued = FALSE;
    m_Callback = NULL;
    m_CallbackArg = NULL;
    m_WorkItemRunningCount = 0;
    m_OutStandingWorkItem = 1;
}

FxSystemWorkItem::~FxSystemWorkItem()
{
    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();

    //
    // If this hits, it's because someone destroyed the WORKITEM by
    // removing too many references by mistake without calling WdfObjectDelete
    //
    if( !m_RunningDown && (m_WorkItem.GetWorkItem() != NULL)) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "WorkItem destroyed without calling "
                            "FxSystemWorkItem::Delete, or by Framework "
                            "processing DeviceRemove. "
                            "Possible reference count problem?");
        FxVerifierDbgBreakPoint(FxDriverGlobals);
    }

    // Free the workitem
    if( m_WorkItem.GetWorkItem() != NULL ) {
        m_WorkItem.Free();
    }

    ASSERT(m_Enqueued == FALSE);
    ASSERT(m_WorkItemRunningCount == 0L);

    return;
}

_Must_inspect_result_
NTSTATUS
FxSystemWorkItem::Initialize(
    __in PVOID WdmObject
    )
{
    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();
    NTSTATUS status;

    //
    // Mark this object as passive level to ensure that Dispose() is passive
    //
    MarkPassiveCallbacks(ObjectDoNotLock);

    MarkDisposeOverride(ObjectDoNotLock);

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    status = m_WorkItemCompleted.Initialize(NotificationEvent, TRUE);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Could not initialize m_WorkItemCompleted event "
                            "status %!status!", status);
        return status;
    }

    status = m_RemoveEvent.Initialize(SynchronizationEvent, FALSE);
    if(!NT_SUCCESS(status)) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Could not initialize m_RemoveEvent event "
                            "status %!status!", status);
        return status;
    }
#endif

    //
    // Allocate the PIO_WORKITEM we will re-use
    //
    status = m_WorkItem.Allocate((MdDeviceObject) WdmObject);
    if(!NT_SUCCESS(status)) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Could not allocate IoWorkItem, insufficient resources");
        return status;
    }

    ASSERT(m_WorkItem.GetWorkItem() != NULL);

    return STATUS_SUCCESS;
}

BOOLEAN
FxSystemWorkItem::EnqueueWorker(
    __in PFN_WDF_SYSTEMWORKITEM Func,
    __in PVOID                  Parameter,
    __in BOOLEAN                AssertIfAlreadyQueued
    )
/*++

Routine Description:

    Called to queue the workitem. This function, under verifier, will
    throw an assert if AssertIfAlreadyQueued parameter is set.

Arguments:
    Func - Callback function.

    Parameter - Callback's context.

    AssertIfAlreadyQueued - is used to make sure that caller doesn't
        miss any workitem callback.

Return Value:

    FALSE
        - if the previously queued workitem hasn't run to completion.
        - if the object is running down.

    TRUE - workitem is queued.
--*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    KIRQL irql;

    pFxDriverGlobals = GetDriverGlobals();

    Lock(&irql);

    if( m_Enqueued ) {
        if (AssertIfAlreadyQueued) {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                "WorkItem 0x%p already enqueued IoWorkItem 0x%p",
                                this, m_WorkItem.GetWorkItem());
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
        }

        Unlock(irql);
        return FALSE;
    }

    //
    // If running down, fail
    //
    if( m_RunningDown ) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WorkItem 0x%p is already deleted", this);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        Unlock(irql);
        return FALSE;
    }

    m_WorkItemCompleted.Clear();

    m_Callback = Func;
    m_CallbackArg = Parameter;

    m_Enqueued = TRUE;

    // Add a reference while outstanding
    IncrementWorkItemQueued();

    Unlock(irql);

    m_WorkItem.Enqueue(_WorkItemThunk, this);

    return TRUE;
}

VOID
FxSystemWorkItem::WorkItemHandler()
{
    PFN_WDF_SYSTEMWORKITEM Callback;
    PVOID                  CallbackArg;
    KIRQL                  irql;

    FX_TRACK_DRIVER(GetDriverGlobals());

    Lock(&irql);

    m_Enqueued  = FALSE;

    Callback    = m_Callback;
    CallbackArg = m_CallbackArg;

    m_Callback = NULL;

    //
    // We should only see this count rise to a small number (like 10 or so).
    //
    ASSERT(m_WorkItemRunningCount < 0xFF);

    m_WorkItemRunningCount++;

    Unlock(irql);

    Callback(CallbackArg);

    Lock(&irql);

    m_WorkItemRunningCount--;

    //
    // The driver could re-enqueue a new callback from within the
    // callback handler, which could make m_Enqueued no longer false.
    //
    // We only set the event specifying the callback as completed when
    // we are ensured that no more workitems are enqueued.
    //
    if (m_WorkItemRunningCount == 0L && m_Enqueued == FALSE) {
        m_WorkItemCompleted.Set();
    }

    Unlock(irql);

    return;
}

VOID
FxSystemWorkItem::_WorkItemThunk(
    __in MdDeviceObject DeviceObject,
    __in_opt PVOID      Context
    )

/*++

Routine Description:

    This is the static routine called by the kernel's PIO_WORKITEM handler.

    A reference count was taken by Enqueue, which this routine releases
    on return.

Arguments:

Return Value:

    Nothing.

--*/

{
    FxSystemWorkItem* pWorkItem = (FxSystemWorkItem*)Context;

    UNREFERENCED_PARAMETER(DeviceObject);

    pWorkItem->WorkItemHandler();

    // Release the reference taken when enqueued
    pWorkItem->DecrementWorkItemQueued();

    return;
}

//
// Invoked when DeleteObject is called on the object, or its parent.
//
BOOLEAN
FxSystemWorkItem::Dispose(
    )
{
    KIRQL irql;

    ASSERT(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);

    Lock(&irql);

    ASSERT(!m_RunningDown);

    m_RunningDown = TRUE;

    Unlock(irql);

    ReleaseWorkItemQueuedCountAndWait();

    return TRUE;
}

//
// Wait until any outstanding WorkItem has completed safely
//
// Can only be called after Delete
//
// The caller must call AddRef() and Release() around this call
// to ensure the FxSystemWorkItem remains valid while it is doing the
// wait.
//
VOID
FxSystemWorkItem::WaitForExit(
    )
{
    NTSTATUS Status;

    //
    // Wait for current workitem to complete processing
    //
    Status = m_WorkItemCompleted.EnterCRAndWaitAndLeave();

    ASSERT(NT_SUCCESS(Status));
    UNREFERENCED_PARAMETER(Status);

    //
    // This assert is not under a lock, but the code in WorkItemHandler()
    // clears m_Enqueued under the lock, thus releasing it with a memory
    // barrier. Since we have ensured above that no one can re-queue a request
    // due to m_RunningDown, we should not hit this assert unless there
    // is a code problem with wakeup occuring while an outstanding
    // workitem is running.
    //
    ASSERT(m_Enqueued == FALSE);

    return;
}

