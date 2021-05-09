/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxSystemWorkItem.hpp

Abstract:

    This implements an internal framework workitem that manages
    cleanup.

Author:



Environment:

    Both kernel and user mode

Revision History:


--*/

#ifndef _FXSYSTEMWORKITEM_H
#define _FXSYSTEMWORKITEM_H

//
// This class provides a common place for code to deal with
// cleanup and synchronization issues of workitems utilized
// internally by the framework
//

_Function_class_(EVT_SYSTEMWORKITEM)
__drv_maxIRQL(PASSIVE_LEVEL)
__drv_maxFunctionIRQL(DISPATCH_LEVEL)
__drv_sameIRQL
typedef
VOID
EVT_SYSTEMWORKITEM(
    __in PVOID Parameter
    );

typedef EVT_SYSTEMWORKITEM FN_WDF_SYSTEMWORKITEM,*PFN_WDF_SYSTEMWORKITEM;

class FxSystemWorkItem : public FxNonPagedObject {

private:

    // Ensures only one of either Delete or Cleanup runs down the object
    BOOLEAN            m_RunningDown;

    //
    // If this is set, a WorkItem has been enqueued
    //
    BOOLEAN            m_Enqueued;

    //
    // The workitem we use
    //
    MxWorkItem         m_WorkItem;

    //
    // The callback function
    //
    PFN_WDF_SYSTEMWORKITEM m_Callback;

    PVOID              m_CallbackArg;

    //
    // This event is signaled when the workitem is done processing
    // an Enqueue request.
    //
    FxCREvent          m_WorkItemCompleted;

    //
    // This count is used to prevent the object from being deleted if
    // one worker thread is preempted right after we drop the lock to call
    // the client callback and another workitem gets queued and runs
    // to completion and signals the event.
    //
    ULONG       m_WorkItemRunningCount;

    //
    // We will keep a count of workitems queued and wait for
    // all the workitems to run to completion before allowing the
    // dispose to complete. Since this object is also used in running
    // down the dispose list during driver unload, this run-down
    // protection is required to make sure that the unload after deleting
    // this object doesn't run ahead of the dispose worker thread.
    //
    LONG   m_OutStandingWorkItem;

    //
    // This event will be signed when the above count drops to zero.
    // The initial value of the count is biased to zero to provide
    // remlock semantics. This event is configured to be a synchronziation
    // event because we know for sure the only thread that's going to
    // wait on this event is the one that's going to call Dispose and
    // after that the object will be destroyed.
    //
    FxCREvent  m_RemoveEvent;

public:
    static
    _Must_inspect_result_
    NTSTATUS
    _Create(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in PVOID              WdmObject,
        __out FxSystemWorkItem** pObject
        );

    virtual
    ~FxSystemWorkItem(
       );

    virtual
    _Must_inspect_result_
    NTSTATUS
    QueryInterface(
        __inout FxQueryInterfaceParams* Params
        )
    {
        switch (Params->Type) {
        case FX_TYPE_SYSTEMWORKITEM:
             *Params->Object = (FxSystemWorkItem*) this;
             break;

        default:
             return FxNonPagedObject::QueryInterface(Params); // __super call
        }

        return STATUS_SUCCESS;
    }

    __inline
    MdWorkItem
    GetWorkItemPtr(
        VOID
        )
    {
        return m_WorkItem.GetWorkItem();
    }

    __inline
    BOOLEAN
    Enqueue(
        __in PFN_WDF_SYSTEMWORKITEM CallbackFunc,
        __in PVOID                  Parameter
        )
    {
        return EnqueueWorker(CallbackFunc, Parameter, TRUE);
    }

    __inline
    BOOLEAN
    TryToEnqueue(
        __in PFN_WDF_SYSTEMWORKITEM CallbackFunc,
        __in PVOID                  Parameter
        )
    {
        return EnqueueWorker(CallbackFunc, Parameter, FALSE);
    }

    VOID
    WaitForExit(
        VOID
        );

    __inline
    VOID
    IncrementWorkItemQueued(
        )
    {
        ASSERT(m_OutStandingWorkItem >= 1);

        InterlockedIncrement(&m_OutStandingWorkItem);
    }

    __inline
    VOID
    DecrementWorkItemQueued(
        )
    {
        LONG result;

        ASSERT(m_OutStandingWorkItem >= 1);

        result = InterlockedDecrement(&m_OutStandingWorkItem);

        if (result == 0) {
            m_RemoveEvent.Set();
        }
    }

    __inline
    VOID
    ReleaseWorkItemQueuedCountAndWait(
        )
    {
        NTSTATUS status;

        //
        // Drop the bias count to indicate the object is being removed.
        //
        DecrementWorkItemQueued();

        status = m_RemoveEvent.EnterCRAndWaitAndLeave();
        ASSERT(NT_SUCCESS(status));
        UNREFERENCED_PARAMETER(status);

        ASSERT(m_OutStandingWorkItem == 0);
    }

    DECLARE_INTERNAL_NEW_OPERATOR();

private:
    FxSystemWorkItem(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    virtual
    BOOLEAN
    Dispose(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    Initialize(
        __in PVOID WdmObject
        );

    VOID
    WorkItemHandler(
        );

    static
    MX_WORKITEM_ROUTINE
    _WorkItemThunk;

    BOOLEAN
    EnqueueWorker(
        __in PFN_WDF_SYSTEMWORKITEM  Func,
        __in PVOID   Parameter,
        __in BOOLEAN AssertIfAlreadyQueued
        );
};

#endif // _FXSYSTEMWORKITEM_H

