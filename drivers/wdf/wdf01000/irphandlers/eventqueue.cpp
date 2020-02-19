#include "common/fxeventqueue.h"
#include "common/fxpkgpnp.h"
#include "common/fxdevice.h"
#include "common/fxdriver.h"
#include "common/fxglobals.h"


FxEventQueue::FxEventQueue(
    __in UCHAR QueueDepth
    )
{
    m_PkgPnp = NULL;
    m_EventWorker = NULL;

    m_HistoryIndex = 0;
    m_QueueHead = 0;
    m_QueueTail = 0;
    m_QueueDepth = QueueDepth;

    m_WorkItemFinished = NULL;
    m_QueueFlags = 0x0;
    m_WorkItemRunningCount = 0x0;
}

BOOLEAN
FxEventQueue::QueueToThreadWorker(
    VOID
    )
/*++

Routine Description:
    Generic worker function which encapsulates the logic of whether to enqueue
    onto a different thread if the thread has not already been queued to.

    NOTE: this function could have been virtual, or call a virtual worker function
          once we have determined that we need to queue to a thread.  But to save
          space on vtable storage (why have one unless you really need one?),
          we rearrange the code so that the derived class calls the worker function
          and this function indicates in its return value what the caller should
          do

Arguments:
    None

Return Value:
    TRUE if the caller should queue to a thread to do the work
    FALSE if the caller shoudl not queue to a thread b/c it has already been
          queued

  --*/
{
    KIRQL irql;
    BOOLEAN result;

    Lock(&irql);

    //
    // For one reason or another, we couldn't run the state machine on this
    // thread.  So queue a work item to do it.
    //
    if (IsEmpty())
    {
        //
        // There is no work to do.  This means that the caller inserted the
        // event into the queue, dropped the lock, and then another thread came
        // in and processed the event.
        //
        // This check also helps in the rundown case when the queue is closing
        // and the following happens between 2 thread:
        // #1                       #2
        // insert event
        // drop lock
        //                          process event queue
        //                          queue goes to empty, so event is set
        // try to queue work item
        //
        result = FALSE;

        DoTraceLevelMessage(
            m_PkgPnp->GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "WDFDEVICE 0x%p !devobj 0x%p not queueing work item to process "
            "event queue", m_PkgPnp->GetDevice()->GetHandle(),
            m_PkgPnp->GetDevice()->GetDeviceObject());
    }
    else if ((m_QueueFlags & FxEventQueueFlagWorkItemQueued) == 0x00)
    {
        m_QueueFlags |= FxEventQueueFlagWorkItemQueued;
        result = TRUE;
    }
    else
    {
        //
        // Somebody is already in the process of enqueuing the work item.
        //
        result = FALSE;
    }

    Unlock(irql);

    return result;
}

VOID
FxWorkItemEventQueue::QueueWorkItem(
    VOID
    )
{
    //
    // The work item will take a reference on KMDF itself.  This will keep KMDF's
    // image around (but not necessarily prevent DriverUnload from being called)
    // so that the code after we set the done event will be in memory and not
    // unloaded.  We must do this because there is no explicit reference between
    // the client driver and KMDF, so when the io manager calls the client's
    // DriverUnload, it has no way of managing KMDF's ref count to stay in memory
    // when the loader unloads KMDF explicitly in response to DriverUnload.
    //
    // We manually take a reference on the client so that we provide the same
    // functionality that IO workitems do.  The client driver's image will have
    // a reference on it after it has returned.
    //

    Mx::MxReferenceObject(m_PkgPnp->GetDriverGlobals()->Driver->GetDriverObject());

    //
    // Prevent FxDriverGlobals from being deleted until the workitem finishes 
    // its work. In case of a bus driver with a PDO in removed
    // state, if the bus is removed, the removal of PDO may happen in a workitem
    // and may result in unload routine being called before the PDO package is
    // deallocated. Since FxPool deallocation code touches FxDriverGlobals 
    // (a pointer of which is located in the header of each FxPool allocated 
    // object), taking this ref prevents the globals from going away until a 
    // corresponding deref at the end of workitem.
    //
    //m_PkgPnp->GetDriverGlobals()->ADDREF(_WorkItemCallback);
   
    m_WorkItem.Enqueue(
        (PMX_WORKITEM_ROUTINE) _WorkItemCallback,
        (FxEventQueue*) this);
}

VOID
FxWorkItemEventQueue::_WorkItemCallback(
    __in MdDeviceObject DeviceObject,
    __in PVOID Context
    )
/*++

Routine Description:
    This is the work item that attempts to run the machine on a thread
    separate from the one the caller was using.  It implements step 9 above.

--*/
{
    FxWorkItemEventQueue* This = (FxWorkItemEventQueue*) Context;
    PFX_DRIVER_GLOBALS  pFxDriverGlobals;

    UNREFERENCED_PARAMETER(DeviceObject);

    MdDriverObject pDriverObject;

    pFxDriverGlobals = This->m_PkgPnp->GetDriverGlobals();

    //
    // Capture the driver object before we call the EventQueueWoker() because
    // the time it returns, This could be freed.

    pDriverObject = pFxDriverGlobals->Driver->GetDriverObject();

    This->EventQueueWorker();

    //
    // Release the ref on FxDriverGlobals taken before queuing this workitem.
    //
    //pFxDriverGlobals->RELEASE(_WorkItemCallback);

    Mx::MxDereferenceObject(pDriverObject);
}

VOID
FxEventQueue::EventQueueWorker(
    VOID
    )
/*++

Routine Description:
    This is the work item that attempts to run the queue state machine on
    the special power thread. 


--*/
{
    FxPostProcessInfo info;
    KIRQL irql;
    FxPkgPnp* pPkgPnp;
    
#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    FX_TRACK_DRIVER(m_PkgPnp->GetDriverGlobals());
#endif

    //
    // Cache away m_PkgPnp while we know we still have a valid object.  Once
    // we Unlock() after the worker routine, the object could be gone until
    // the worker routine set a flag postponing deletion.
    //
    pPkgPnp = m_PkgPnp;

    Lock(&irql);

    ASSERT(m_QueueFlags & FxEventQueueFlagWorkItemQueued);

    //
    // Clear the queued flag, so that it's clear that the work item can
    // be safely re-enqueued.
    //
    m_QueueFlags &= ~FxEventQueueFlagWorkItemQueued;

    //
    // We should only see this count rise to a small number (like 10 or so).
    //
    ASSERT(m_WorkItemRunningCount < 0xFF);
    m_WorkItemRunningCount++;

    Unlock(irql);

    //
    // Call the function that will actually run the state machine.
    //
    m_EventWorker(m_PkgPnp, &info, m_EventWorkerContext);

    Lock(&irql);
    m_WorkItemRunningCount--;
    GetFinishedState(&info);
    Unlock(irql);

    //
    // NOTE:  There is no need to use a reference count to keep this event queue
    //        (and the containing state machine) alive.  Instead, the thread
    //        which wants to delete the state machine must wait for this work
    //        item to exit.  If there was a reference to release, we would have
    //        a race between Unlock()ing and releasing the reference if the state
    //        machine moved into the finished state and deletes the device after
    //        we dropped the lock, but before we released the reference.
    //
    //        This is important in that the device deletion can trigger
    //        DriverUnload to run before the release executes.  DriverUnload
    //        frees the IFR buffer.  If this potential release logs something to
    //        the IFR, you would bugcheck.  Since it is impossible to defensively
    //        prevent all destructors from logging to the IFR, we can't use a
    //        ref count here to keep the queue alive.
    //

    //
    // If Evaluate needs to use pPkgPnp, then the call to the worker routine
    // above made sure that pPkgPnp has not yet been freed.
    //
    info.Evaluate(pPkgPnp);
}

BOOLEAN
FxEventQueue::SetFinished(
    __in FxCREvent* Event
    )
/*++

Routine Description:
    Puts the queue into a closed state.  If the queue cannot be closed and
    finished in this context, the Event is stored and set when it moves into
    the finished state

Arguments:
    Event - the event to set when we move into the finished state

Return Value:
    TRUE if the queue is closed in this context, FALSE if the Event should be
    waited on.

  --*/
{
    KIRQL irql;
    BOOLEAN result;

    result = TRUE;

    Lock(&irql);
    ASSERT((m_QueueFlags & FxEventQueueFlagClosed) == 0x0);
    m_QueueFlags |= FxEventQueueFlagClosed;

    result = IsIdleLocked();

    if (result == FALSE)
    {
        m_WorkItemFinished = Event;
    }

    Unlock(irql);

    if (result)
    {
        Event->Set();
    }

    return result;
}

_Must_inspect_result_
NTSTATUS
FxEventQueue::Initialize(
    __in PFX_DRIVER_GLOBALS DriverGlobals
    )
{
    NTSTATUS status;
    
    //
    // For KM, lock initialize always succeeds. For UM, it might fail.
    //
    status = m_StateMachineLock.Initialize();
    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(DriverGlobals, 
            TRACE_LEVEL_ERROR, TRACINGPNP,
            "Initializing state machine lock failed for EventQueue 0x%p, "
            "status %!STATUS!",
            this, status);
    }

    return status;
}

VOID
FxEventQueue::Configure(
    __in FxPkgPnp* Pnp,
    __in PFN_PNP_EVENT_WORKER WorkerRoutine,
    __in PVOID Context
    )
{
    m_PkgPnp = Pnp;
    m_EventWorker = WorkerRoutine;
    m_EventWorkerContext = Context;

    return;
}

FxWorkItemEventQueue::FxWorkItemEventQueue(
    __in UCHAR QueueDepth
    ) : FxEventQueue(QueueDepth)
{
}

FxWorkItemEventQueue::~FxWorkItemEventQueue()
{
    m_WorkItem.Free();
}

_Must_inspect_result_
NTSTATUS
FxWorkItemEventQueue::Init(
    __inout FxPkgPnp* Pnp,
    __in PFN_PNP_EVENT_WORKER WorkerRoutine,
    __in PVOID WorkerContext
    )
{
    NTSTATUS status;

    Configure(Pnp, WorkerRoutine, WorkerContext);

    status = m_WorkItem.Allocate(
        (MdDeviceObject)FxLibraryGlobals.DriverObject
        //(MdDeviceObject)(GetIoMgrObjectForWorkItemAllocation())
        );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxThreadedEventQueue::Init(
    __inout FxPkgPnp* Pnp,
    __in PFN_PNP_EVENT_WORKER WorkerRoutine,
    __in PVOID WorkerContext
    )
{
    NTSTATUS status;

    Configure(Pnp, WorkerRoutine, WorkerContext);

    status = m_WorkItem.Allocate(Pnp->GetDevice()->GetDeviceObject());
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return STATUS_SUCCESS;
}

FxThreadedEventQueue::FxThreadedEventQueue(
    __in UCHAR QueueDepth
    ) : FxEventQueue(QueueDepth)
{
    ExInitializeWorkItem(&m_EventWorkQueueItem,
                         (PWORKER_THREAD_ROUTINE) _WorkerThreadRoutine,
                         this);
}

FxThreadedEventQueue::~FxThreadedEventQueue(
    VOID
    )
{
    m_WorkItem.Free();
}

VOID
FxThreadedEventQueue::_WorkerThreadRoutine(
    __in PVOID Context
    )
{
    FxThreadedEventQueue* This = (FxThreadedEventQueue *)Context;

    This->EventQueueWorker();
}
