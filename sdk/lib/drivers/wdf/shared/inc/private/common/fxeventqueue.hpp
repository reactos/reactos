//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef _FXEVENTQUEUE_H_
#define _FXEVENTQUEUE_H_

struct FxPostProcessInfo {
    FxPostProcessInfo(
        VOID
        )
    {
        m_Event = NULL;
        m_DeleteObject = FALSE;
        m_SetRemovedEvent = FALSE;
        m_FireAndForgetIrp = NULL;
    }

    BOOLEAN
    SomethingToDo(
        VOID
        )
    {
        return ((m_Event != NULL) || m_DeleteObject) ? TRUE : FALSE;
    }

    VOID
    Evaluate(
        __inout FxPkgPnp* PkgPnp
        );

    FxCREvent* m_Event;
    BOOLEAN m_DeleteObject;
    BOOLEAN m_SetRemovedEvent;
    MdIrp m_FireAndForgetIrp;
};

typedef
VOID
(*PFN_PNP_EVENT_WORKER)(
    __in FxPkgPnp* PkgPnp,
    __in FxPostProcessInfo* Info,
    __in PVOID Context
    );

enum FxEventQueueFlags {
    FxEventQueueFlagWorkItemQueued = 0x01,
    FxEventQueueFlagClosed = 0x02,
    FxEventQueueFlagDelayDeletion = 0x04,
};

struct FxEventQueue : public FxStump {
    FxEventQueue(
        __in UCHAR QueueDepth
        );

    _Must_inspect_result_
    NTSTATUS
    Initialize(
        __in PFX_DRIVER_GLOBALS DriverGlobals
        );

    _Acquires_lock_(this->m_QueueLock)
    __drv_maxIRQL(DISPATCH_LEVEL)
    __drv_setsIRQL(DISPATCH_LEVEL)
    VOID
    Lock(
        __out __drv_deref(__drv_savesIRQL)
        PKIRQL Irql
        )
    {
        m_QueueLock.Acquire(Irql);
    }

     _Releases_lock_(this->m_QueueLock)
    __drv_requiresIRQL(DISPATCH_LEVEL)
    VOID
    Unlock(
        __in __drv_restoresIRQL
        KIRQL Irql
        )
    {
        m_QueueLock.Release(Irql);
    }

    BOOLEAN
    IsFull(
        VOID
        )
    {
        return ((m_QueueHead + m_QueueDepth - 1) % m_QueueDepth) == (m_QueueTail % m_QueueDepth);
    }

    BOOLEAN
    IsEmpty(
        VOID
        )
    {
        return m_QueueHead == m_QueueTail;
    }

    VOID
    IncrementHead(
        VOID
        )
    {
        m_QueueHead = (m_QueueHead + 1) % m_QueueDepth;
    }

    UCHAR
    GetHead(
        VOID
        )
    {
        return m_QueueHead;
    }

    UCHAR
    InsertAtHead(
        VOID
        )
    {
        m_QueueHead = (m_QueueHead + m_QueueDepth - 1) % m_QueueDepth;
        return m_QueueHead;
    }

    UCHAR
    InsertAtTail(
        VOID
        )
    {
        UCHAR index;

        // Save the index which is the current tail
        index = m_QueueTail;

        // goto next slot
        m_QueueTail = (m_QueueTail + 1) % m_QueueDepth;

        // return the old tail as the slot to insert at
        return index;
    }

    UCHAR
    IncrementHistoryIndex(
        VOID
        )
    {
        UCHAR cur;

        cur = m_HistoryIndex;
        m_HistoryIndex = (m_HistoryIndex + 1) % m_QueueDepth;

        return cur;
    }

    BOOLEAN
    IsClosedLocked(
        VOID
        )
    {
        return (m_QueueFlags & FxEventQueueFlagClosed) ? TRUE : FALSE;
    }

    VOID
    GetFinishedState(
        __inout FxPostProcessInfo* Info
        )
    {
        if (IsIdleLocked()) {
            if (m_QueueFlags & FxEventQueueFlagDelayDeletion) {
                m_QueueFlags &= ~FxEventQueueFlagDelayDeletion;
                Info->m_DeleteObject = TRUE;
            }

            if (IsClosedLocked()) {
                Info->m_Event = m_WorkItemFinished;
                m_WorkItemFinished = NULL;
            }
        }
    }

    BOOLEAN
    SetFinished(
        __in FxCREvent* Event
        );

    VOID
    SetDelayedDeletion(
        VOID
        );

protected:
    VOID
    Configure(
        __in FxPkgPnp* Pnp,
        __in PFN_PNP_EVENT_WORKER WorkerRoutine,
        __in PVOID Context
        );

    BOOLEAN
    QueueToThreadWorker(
        VOID
        );

    VOID
    EventQueueWorker(
        VOID
        );

    BOOLEAN
    IsIdleLocked(
        VOID
        )
    {
        //
        // We are idle if there is no work item queued, no work item running,
        // and there are no events in the queue.  Since m_WorkItemQueued is
        // cleared before we enter the state machine, we must also track the
        // number of work items running.
        //
        if ((m_QueueFlags & FxEventQueueFlagWorkItemQueued) == 0x00 &&
            m_WorkItemRunningCount == 0x0 &&
            IsEmpty()) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }

protected:
    // index into the beginning of the circular event ring buffer
    UCHAR m_QueueHead;

    // index into the end of the circular event ring buffer
    UCHAR m_QueueTail;

    // circular event ring buffer size
    UCHAR m_QueueDepth;

    UCHAR m_HistoryIndex;

    FxPkgPnp* m_PkgPnp;

    //
    // Context that is passed back to the the state machine as
    // part of the event worker
    //
    PVOID m_EventWorkerContext;

    //
    // Lock for the queue
    //
    MxLock m_QueueLock;

public:
    FxWaitLockInternal m_StateMachineLock;

protected:
    PFN_PNP_EVENT_WORKER m_EventWorker;

    //
    // Guarded by m_QueueLock.  Will be set to a valid pointer value when pnp
    // wants to remove the device and we want to synchronize against the work
    // item running.
    //
    FxCREvent* m_WorkItemFinished;

    //
    //
    union {
        //
        // See FxEventQueueFlags for values
        //
        UCHAR m_QueueFlags;

        struct {
            UCHAR WorkItemQueued : 1;
            UCHAR Closed : 1;
            UCHAR DelayDeletion : 1;
        } m_QueueFlagsByName;
    };

    //
    // Count of times the work item is running.  Since m_WorkItemQueued is
    // cleared before we enter the state machine, we must also track the
    // number of instances to make sure we know when we are idle or not.
    //
    UCHAR m_WorkItemRunningCount;
};

struct FxWorkItemEventQueue : public FxEventQueue {
    FxWorkItemEventQueue(
        __in UCHAR QueueDepth
        );

    ~FxWorkItemEventQueue();

    _Must_inspect_result_
    NTSTATUS
    Init(
        __inout FxPkgPnp* Pnp,
        __in PFN_PNP_EVENT_WORKER WorkerRoutine,
        __in PVOID WorkerContext = NULL
        );

    VOID
    QueueToThread(
        VOID
        )
    {
        if (QueueToThreadWorker()) {
            QueueWorkItem();
        }
    }

protected:
    VOID
    QueueWorkItem(
        VOID
        );

    static
    MX_WORKITEM_ROUTINE
    _WorkItemCallback;

    MxWorkItem m_WorkItem;
};

//
// struct that encapsulates posting a work item to the dedicated power thread
// or work item depending on the power pagable status of the stack.
//
struct FxThreadedEventQueue : public FxEventQueue {
    FxThreadedEventQueue(
        __in UCHAR QueueDepth
        );

    ~FxThreadedEventQueue(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    Init(
        __inout FxPkgPnp* Pnp,
        __in PFN_PNP_EVENT_WORKER WorkerRoutine,
        __in PVOID WorkerContext = NULL
        );

    VOID
    QueueToThread(
        VOID
        )
    {
        if (QueueToThreadWorker()) {
            QueueWorkItem();
        }
    }

protected:
    static
    WORKER_THREAD_ROUTINE
    _WorkerThreadRoutine;

    static
    MX_WORKITEM_ROUTINE
    _WorkItemCallback;

    VOID
    QueueWorkItem(
        VOID
        );

    MxWorkItem m_WorkItem;

    // work item used to queue to the thread
    WORK_QUEUE_ITEM     m_EventWorkQueueItem;
};

#endif // _FXEVENTQUEUE_H_
