/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxWorkItem.hpp

Abstract:

    This module implements a frameworks managed WORKITEM that
    can synchrononize with driver frameworks object locks.

Author:



Environment:

    Both kernel and user mode

Revision History:


--*/

#include "coreprivshared.hpp"

#include "fxworkitem.hpp"

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
//
// For DRIVER_OBJECT_UM definition
//
#include "fxldrum.h"
#endif

// Tracing support
extern "C" {
// #include "FxWorkItem.tmh"
}

FxWorkItem::FxWorkItem(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    ) :
    FxNonPagedObject(FX_TYPE_WORKITEM, sizeof(FxWorkItem), FxDriverGlobals),
    m_WorkItemCompleted(NotificationEvent, TRUE)
{
   m_Object = NULL;
   m_Callback = NULL;
   m_CallbackLock = NULL;
   m_CallbackLockObject = NULL;
   m_RunningDown = FALSE;
   m_Enqueued = FALSE;
   m_WorkItemThread = NULL;
   m_WorkItemRunningCount = 0;

   //
   // All operations on a workitem are PASSIVE_LEVEL so ensure that any Dispose
   // and Destroy callbacks to the driver are as well.
   //
   MarkPassiveCallbacks(ObjectDoNotLock);

   MarkDisposeOverride(ObjectDoNotLock);
}


FxWorkItem::~FxWorkItem(
    VOID
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetDriverGlobals();

    //
    // If this hits, it's because someone destroyed the WORKITEM by
    // removing too many references by mistake without calling WdfObjectDelete
    //
    if (m_RunningDown == FALSE && m_Callback != NULL) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFWORKITEM %p destroyed without calling WdfObjectDelete, or by "
            "Framework processing DeviceRemove.  Possible reference count "
            "problem?", GetObjectHandleUnchecked());
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
    }

    // Release our parent object reference
    if (m_Object != NULL) {
        m_Object->RELEASE(this);
        m_Object = NULL;
    }

    // Free the workitem
    if (m_WorkItem.GetWorkItem() != NULL) {
        m_WorkItem.Free();
        //m_WorkItem = NULL;
    }

    ASSERT(m_Enqueued == FALSE);
    ASSERT(m_WorkItemRunningCount == 0L);

    return;
}

_Must_inspect_result_
NTSTATUS
FxWorkItem::_Create(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PWDF_WORKITEM_CONFIG Config,
    __in PWDF_OBJECT_ATTRIBUTES Attributes,
    __in FxObject* ParentObject,
    __out WDFWORKITEM* WorkItem
    )
{
    FxWorkItem* pFxWorkItem;
    NTSTATUS status;

    pFxWorkItem = new(FxDriverGlobals, Attributes) FxWorkItem(FxDriverGlobals);

    if (pFxWorkItem == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = pFxWorkItem->Initialize(
        Attributes,
        Config,
        ParentObject,
        WorkItem
        );

    if (!NT_SUCCESS(status)) {
        pFxWorkItem->DeleteFromFailedCreate();
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxWorkItem::Initialize(
    __in PWDF_OBJECT_ATTRIBUTES Attributes,
    __in PWDF_WORKITEM_CONFIG Config,
    __in FxObject* ParentObject,
    __out WDFWORKITEM* WorkItem
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    IFxHasCallbacks* pCallbacks;
    NTSTATUS status;

    pFxDriverGlobals = GetDriverGlobals();

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    status = m_WorkItemCompleted.Initialize(NotificationEvent, TRUE);
    if(!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Could not initialize m_WorkItemCompleted event "
                            "%!STATUS!", status);
        return status;
    }
#endif

    ASSERT(Config->EvtWorkItemFunc != NULL);

    // Set users callback function
    m_Callback = Config->EvtWorkItemFunc;

    //
    // As long as we are associated, the parent object holds a reference
    // count on the WDFWORKITEM.
    //
    // This reference must be taken early before we return any failure,
    // since Dispose() expects this extra reference, and Dispose() will
    // be called even if we return a failure status right now.
    //
    ADDREF(this);

    //
    // WorkItems can be parented by, and optionally serialize with an FxDevice or an FxQueue
    //
    m_DeviceBase = FxDeviceBase::_SearchForDevice(ParentObject, &pCallbacks);

    if (m_DeviceBase == NULL) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Determine if it's an FxDevice, or FxIoQueue and get the
    // CallbackSpinLock pointer for it.
    //
    status = _GetEffectiveLock(
        ParentObject,
        pCallbacks,
        Config->AutomaticSerialization,
        TRUE,
        &m_CallbackLock,
        &m_CallbackLockObject
        );

    if (!NT_SUCCESS(status)) {
        if (status == STATUS_WDF_INCOMPATIBLE_EXECUTION_LEVEL) {






            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "ParentObject %p cannot automatically synchronize callbacks "
                "with a WorkItem since it is not configured for passive level "
                "callback constraints.  Use a WDFDPC instead or set "
                "AutomaticSerialization to FALSE."
                "%!STATUS!", Attributes->ParentObject, status);
        }

        return status;
    }

    //
    // Allocate the PIO_WORKITEM we will re-use
    //
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    m_WorkItem.Allocate(m_Device->GetDeviceObject());
#elif (FX_CORE_MODE == FX_CORE_USER_MODE)
    m_WorkItem.Allocate(
        m_Device->GetDeviceObject(),
        (PVOID)&m_Device->GetDriver()->GetDriverObject()->ThreadPoolEnv);
#endif
    if (m_WorkItem.GetWorkItem() == NULL) {
        status =  STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Could not allocate IoWorkItem, %!STATUS!", status);
        return status;
    }

    //
    // We automatically synchronize with and reference count
    // the lifetime of the framework object to prevent any WORKITEM races
    // that can access the object while it is going away.
    //

    //
    // The caller supplied object is the object the caller wants the
    // WorkItem to be associated with, and the framework must ensure this
    // object remains live until the WorkItem object is destroyed. Otherwise,
    // it could access either object context memory, or an object API
    // on a freed object.
    //
    // Due to the locking model of the framework, the lock may actually
    // be owned by a higher level object as well. This is the lockObject
    // returned. As long as we are a child of this object, the lockObject
    // does not need to be dereferenced since it will notify us of Cleanup
    // before it goes away.
    //

    //
    // Associate the FxWorkItem with the object. When this object Cleans up, it
    // will notify our Cleanup function as well.
    //

    //
    // Add a reference to the parent object we are associated with.
    // We will be notified of Cleanup to release this reference.
    //
    ParentObject->ADDREF(this);

    // Save the ptr to the object the WorkItem is associated with
    m_Object = ParentObject;

    //
    // Attributes->ParentObject is the same as ParentObject. Since we already
    // converted it to an object, use that.
    //
    status = Commit(Attributes, (WDFOBJECT*)WorkItem, ParentObject);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    return status;
}

VOID
FxWorkItem::Enqueue(
    VOID
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    KIRQL irql;
    BOOLEAN enqueue;

    pFxDriverGlobals = GetDriverGlobals();
    enqueue = FALSE;

    Lock(&irql);

    if (m_Enqueued) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDEVICE,
            "Previously queued WDFWORKITEM 0x%p is already pending. "
            "Ignoring the request to queue again", GetHandle());
    }
    else if (m_RunningDown) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WDFWORKITEM 0x%p is already deleted", GetHandle());
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
    }
    else {
        m_WorkItemCompleted.Clear();

        m_Enqueued = TRUE;

        //
        // We are going to enqueue the work item. Reference this FxWorkItem
        // object and Globals while they are outstanding.
        // These will be released when the workitem completes.
        //
        ADDREF((PVOID)WorkItemThunk);
        pFxDriverGlobals->ADDREF((PVOID)WorkItemThunk);

        enqueue = TRUE;
    }

    Unlock(irql);

    if (enqueue) {
        m_WorkItem. Enqueue(FxWorkItem::WorkItemThunk, this);
    }

    return;
}

VOID
FxWorkItem::WorkItemHandler(
    VOID
    )
{
    KIRQL irql;

    FX_TRACK_DRIVER(GetDriverGlobals());

    Lock(&irql);

    //
    // Mark the workitem as no longer enqueued and completed
    //
    // The handler is allowed to re-enqueue, so mark it before the callback
    //
    m_Enqueued = FALSE;

    m_WorkItemRunningCount++;

    Unlock(irql);

    if (m_CallbackLock != NULL) {
        m_CallbackLock->Lock(&irql);
#if FX_IS_KERNEL_MODE
        FxPerfTraceWorkItem(&m_Callback);
#endif
        m_Callback(GetHandle());
        m_CallbackLock->Unlock(irql);
    }
    else {
#if FX_IS_KERNEL_MODE
        FxPerfTraceWorkItem(&m_Callback);
#endif
        m_Callback(GetHandle());
    }

    Lock(&irql);

    m_WorkItemRunningCount--;

    //
    // The workitem can be re-enqueued by the drivers
    // work item handler routine. We can't set the work
    // item completed event until we are sure there are
    // no outstanding work items.
    //

    if (m_WorkItemRunningCount == 0L && m_Enqueued == FALSE) {
        m_WorkItemCompleted.Set();
    }

    Unlock(irql);
}

VOID
FxWorkItem::WorkItemThunk(
    __in MdDeviceObject DeviceObject,
    __in_opt PVOID      Context
    )
/*++

Routine Description:

    This is the static routine called by the kernels PIO_WORKITEM handler.

    A reference count was taken by Enqueue, which this routine releases
    on return.

Arguments:
    DeviceObject  - the devobj we passed to IoCreateWorkItem

    Context - the internal Fx object

Return Value:
    None

--*/
{
    FxWorkItem*         pWorkItem;
    PFX_DRIVER_GLOBALS  pFxDriverGlobals;

    UNREFERENCED_PARAMETER(DeviceObject);

    pWorkItem = (FxWorkItem*)Context;
    pFxDriverGlobals = pWorkItem->GetDriverGlobals();

    //
    // Save the current worker thread object pointer. We will use this avoid
    // deadlock if the driver tries to delete or flush the workitem from within
    // the callback.
    //
    pWorkItem->m_WorkItemThread = Mx::MxGetCurrentThread();

    pWorkItem->WorkItemHandler();

    pWorkItem->m_WorkItemThread = NULL;

    //
    // Release the reference on the FxWorkItem and Globals taken when Enqueue
    // was done. This may release the FxWorkItem if it is running down.
    //
    pWorkItem->RELEASE((PVOID)WorkItemThunk);

    //
    // This may release the driver if it is running down.
    //
    pFxDriverGlobals->RELEASE((PVOID)WorkItemThunk);
}

VOID
FxWorkItem::FlushAndRundown(
    VOID
    )
{
    FxObject* pObject;

    //
    // Wait for any outstanding workitem to complete if the workitem is not
    // deleted from within the workitem callback to avoid deadlock.
    //
    if (m_WorkItemThread != Mx::MxGetCurrentThread()) {
        WaitForSignal();
    }

    //
    // Release our reference count to the associated parent object if present
    //
    if (m_Object != NULL) {
        pObject = m_Object;
        m_Object = NULL;

        pObject->RELEASE(this);
    }

    //
    // Perform our final release to ourselves, destroying the FxWorkItem
    //
    RELEASE(this);
}

BOOLEAN
FxWorkItem::Dispose(
    VOID
    )
/*++

Routine Description:
    Called when DeleteObject is called, or when the parent is being deleted or
    Disposed.

Arguments:
    None

Return Value:
    TRUE if the cleanup routines should be invoked, FALSE otherwise

  --*/
{
    KIRQL irql;

    Lock(&irql);
    m_RunningDown = TRUE;
    Unlock(irql);

    FlushAndRundown();

    return TRUE;
}

VOID
FxWorkItem::FlushAndWait()
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetDriverGlobals();

    if (m_WorkItemThread == Mx::MxGetCurrentThread()) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                        "Calling WdfWorkItemFlush from within the WDFWORKITEM "
                        "%p callback will lead to deadlock, PRKTHREAD %p",
                        GetHandle(), m_WorkItemThread);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    //
    // Wait for any outstanding workitem to complete.
    // The event is only set upon return from the callback
    // into the driver *and* the driver did not re-queue
    // the workitem. See similar comment in WorkItemHandler().
    //
    WaitForSignal();
    return;
}

VOID
FxWorkItem::WaitForSignal(
    VOID
    )
{
    LARGE_INTEGER timeOut;
    NTSTATUS status;

    ASSERT(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);

    timeOut.QuadPart = WDF_REL_TIMEOUT_IN_SEC(60);

    do {
        status = m_WorkItemCompleted.EnterCRAndWaitAndLeave(&timeOut.QuadPart);
        if (status == STATUS_TIMEOUT) {
            DbgPrint("Thread 0x%p is waiting on WDFWORKITEM 0x%p\n",
                      Mx::GetCurrentEThread(),
                      GetHandle());
        }
        else  {
            ASSERT(NT_SUCCESS(status));
            break;
        }
    } WHILE(TRUE);


    ASSERT(NT_SUCCESS(status));
    return;
}
