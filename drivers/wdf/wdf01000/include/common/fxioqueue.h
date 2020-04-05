#ifndef _FXIOQUEUE_H_
#define _FXIOQUEUE_H_

#include "common/fxnonpagedobject.h"
#include "common/fxcxdeviceinfo.h"
#include "common/fxrequest.h"
#include "common/fxioqueuecallbacks.h"
#include "common/fxsystemworkitem.h"
#include "common/fxcallbackspinlock.h"
#include "common/fxcallbackmutexlock.h"


enum FxIoStopProcessingForPowerAction {
    FxIoStopProcessingForPowerHold = 1,
    FxIoStopProcessingForPowerPurgeManaged,
    FxIoStopProcessingForPowerPurgeNonManaged,
};

//
// These FxIoQueue public Enum and Struct are used to tie the queue 
// with FxPkgIo.
//
enum FxIoQueueNodeType {
    FxIoQueueNodeTypeInvalid = 0,
    FxIoQueueNodeTypeQueue,
    FxIoQueueNodeTypeBookmark,
    FxIoQueueNodeTypeLast,
};

struct FxIoQueueNode {
public:
    //
    // Data members.
    //
    LIST_ENTRY          m_ListEntry;
    FxIoQueueNodeType   m_Type;

public:
    //
    // Manager functions.
    //
    FxIoQueueNode(
        FxIoQueueNodeType NodeType
        ) :
        m_Type(NodeType)
    {
        ASSERT(_IsValidNodeType(m_Type));
        InitializeListHead(&m_ListEntry);
    }

    ~FxIoQueueNode()
    {
        ASSERT(IsListEmpty(&m_ListEntry) == TRUE);
    }

private:
    //
    // Turn off unsupported manager functions.
    //
    FxIoQueueNode();

    //
    // Block default behavior to shallow copy the object because it contains 
    // a double-link entry; shallow copying the object produces an invalid 
    // copy because the double-link entry is not properly re-initialized.
    //
    FxIoQueueNode(const FxIoQueueNode&);

    FxIoQueueNode& operator=(const FxIoQueueNode&);
    
public:
    //
    // Helper functions.
    //
    static
    FxIoQueueNode*
    _FromListEntry(
        __in PLIST_ENTRY Entry
        )
    {
        return CONTAINING_RECORD(Entry, FxIoQueueNode, m_ListEntry);
    }

    static
    BOOLEAN
    _IsValidNodeType(
        __in FxIoQueueNodeType NodeType
        )
    {
        return ((NodeType > FxIoQueueNodeTypeInvalid) &&
                (NodeType < FxIoQueueNodeTypeLast)) ? TRUE : FALSE;
    }
    
    __inline
    BOOLEAN 
    IsNodeType(
        __in FxIoQueueNodeType NodeType
        )
    {
        return (NodeType == m_Type) ? TRUE : FALSE;
    }
};

class FxPkgIo;

//
// These FxIoQueue private Enum's control the internal state machine
//

// begin_wpp enum
typedef enum FxIoQueuePowerState {
    FxIoQueuePowerInvalid = 0,
    FxIoQueuePowerOn,
    FxIoQueuePowerOff,
    FxIoQueuePowerStartingTransition,
    FxIoQueuePowerStopping,
    FxIoQueuePowerStoppingNotifyingDriver,
    FxIoQueuePowerStoppingDriverNotified,
    FxIoQueuePowerPurge,
    FxIoQueuePowerPurgeNotifyingDriver,
    FxIoQueuePowerPurgeDriverNotified,
    FxIoQueuePowerRestarting,
    FxIoQueuePowerRestartingNotifyingDriver,
    FxIoQueuePowerRestartingDriverNotified,
    FxIoQueuePowerLast,
} FXIOQUEUE_POWER_STATE;
// end_wpp

typedef struct _FXIO_FORWARD_PROGRESS_CONTEXT {
    //
    // Total Number of Reserved requests
    //
    ULONG m_NumberOfReservedRequests;
    //
    // Callback invoked to allocate resources for reserved requests at init time
    //
    //FxIoQueueForwardProgressAllocateResourcesReserved   m_IoReservedResourcesAllocate;
    //
    // Callback invoked to allocate resources for non-reserved requests at run time
    //
    FxIoQueueForwardProgressAllocateResources   m_IoResourcesAllocate;
    //
    // Callback invoked to Examine the IRP and decide whether to fail it or not
    //
    FxIoQueueForwardProgressExamineIrp       m_IoExamineIrp;   
    //
    // Policy configured for forward progress
    //
    WDF_IO_FORWARD_PROGRESS_RESERVED_POLICY  m_Policy;
    //
    // List of available reserved requests
    //
    LIST_ENTRY  m_ReservedRequestList;

    //
    // List of in use reserved requests
    //
    LIST_ENTRY  m_ReservedRequestInUseList;

    //
    // List of all pended IRPs
    //
    LIST_ENTRY  m_PendedIrpList;
    //
    // This lock is used to add new entreies to the pended IRP list
    // or the  add the request back to the Reserved List
    //
    MxLockNoDynam m_PendedReserveLock;
        
} FXIO_FORWARD_PROGRESS_CONTEXT, *PFXIO_FORWARD_PROGRESS_CONTEXT;

//
// This defines the valid arguments to the
// SetStatus call.
//
typedef enum _FX_IO_QUEUE_SET_STATE {
    FxIoQueueSetAcceptRequests     = 0x80000001,
    FxIoQueueClearAcceptRequests   = 0x80000002,
    FxIoQueueSetDispatchRequests   = 0x80000004,
    FxIoQueueClearDispatchRequests = 0x80000008,
    FxIoQueueSetShutdown           = 0x80010000,
    FxIoQueueClearShutdown         = 0x80020000,
} FX_IO_QUEUE_SET_STATE;

//
// This defines the internal queue state.
//
typedef enum _FX_IO_QUEUE_STATE {
    //
    // private == public values (low word).
    //
    FxIoQueueAcceptRequests     = WdfIoQueueAcceptRequests,     // 0x00000001
    FxIoQueueDispatchRequests   = WdfIoQueueDispatchRequests,   // 0x00000002
    FxIoQueueNoRequests         = WdfIoQueueNoRequests,         // 0x00000004
    FxIoQueueDriverNoRequests   = WdfIoQueueDriverNoRequests,   // 0x00000008
    FxIoQueuePnpHeld            = WdfIoQueuePnpHeld,            // 0x00000010
    //
    // private values only (high word).
    //
    // Queue is being shut down. Flag on means do not queue any request 
    // even if the accept request state is set. This flag prevents the 
    // following scenarios:
    // (1) a race condition where a dispatch queue handler changes the
    //     state of the queue to accept_requests while we are in the 
    //     middle of a power stopping (purge) operation (Win7 719587).
    // (2) another thread calling Stop or Start on a queue that is in the
    //     middle of a power stopping (purge) operation.
    //
    FxIoQueueShutdown           = 0x00010000
} FX_IO_QUEUE_STATE;

class FxIoQueue : public FxNonPagedObject {

    //
    // forward progress fields
    //
    PFXIO_FORWARD_PROGRESS_CONTEXT m_FwdProgContext;
    
    //
    // This is a true indicator whether the Queue is ready for forward progress
    //
    BOOLEAN m_SupportForwardProgress;

    //
    // Specifies that the queue has been configured
    // with driver callbacks and presentation serialization
    //
    BOOLEAN             m_Configured;

    //
    // TRUE if this is a power managed queue. If TRUE, it is reported
    // as a power managed queue, and will automatically start/resume
    // based on power management requests.
    //
    // If false, the device driver has manual control on start/resume.
    //
    BOOLEAN             m_PowerManaged;

    //
    // This is TRUE if we have an outstanding reference to the
    // Pnp package about having I/O in our queue.
    //
    volatile BOOLEAN    m_PowerReferenced;

    //
    // If TRUE, zero length read/writes are allowed to the driver.
    // Otherwise, they are completed automatically with STATUS_SUCCESS.
    //
    BOOLEAN             m_AllowZeroLengthRequests;

    //
    // True if callback operations to the driver occur at
    // PASSIVE_LEVEL. Also marked in FxObject, but this cache
    // avoids acquiring FxObject state lock.
    //
    BOOLEAN             m_PassiveLevel;

    //
    // This is set before m_FinishDisposing is signalled to
    // allow the Dispose thread to continue deletion of
    // queue resources. Once this is set, no thread is
    // allowed to run thru DispatchEvents.
    //
    volatile BOOLEAN    m_Deleted;

    //
    // This is set when the I/O package marks the queue
    // for deleting, but the I/O queue deferrs its final
    // dereference until all outstanding I/O's to the
    // device driver are completed.
    //
    volatile BOOLEAN    m_Disposing;
    MxEvent             m_FinishDisposing;

    //
    // Power State of the Queue
    //
    FXIOQUEUE_POWER_STATE m_PowerState;

    //
    // This is the type of queue, and is configured by the
    // user at initial queue creation time.
    //
    WDF_IO_QUEUE_DISPATCH_TYPE m_Type;

    //
    // Maximum number of driver presented Requests on a parallel Queue
    //
    ULONG  m_MaxParallelQueuePresentedRequests;

    //
    // This is the current processing status of the queue.
    //
    FX_IO_QUEUE_STATE m_QueueState;

    //
    // The FxIoQueue tracks a request from the time it arrives by being
    // enqueued, until it leaves by being completed, or forwarded.
    //
    // At any given time, a request may be in one the following five states:
    //
    // 1) Request is queued and cancelable
    //
    //    It is on the FxIrpQueue m_Queue using the LIST_ENTRY
    //    FxRequest::m_Irp->Tail.Overlay.ListEntry
    //
    // 2) Request has been passed to the driver, and is not cancelable
    //
    //    It is on the LIST_ENTRY m_DriverNonCancelable using the LIST_ENTRY
    //    FxRequest::m_OwnerListEntry
    //
    //    It is also on the LIST_ENTRY m_DriverInFlight using the LIST_ENTRY
    //    FxRequest::m_InFlightListEntry
    //
    // 3) Request has been passed to the driver, and is cancelable
    //
    //    It is on the FxIrpQueue m_DriverCancelable using the LIST_ENTRY
    //    FxRequest::m_Irp->Tail.Overlay.ListEntry
    //
    //    It is also on the LIST_ENTRY m_DriverInFlight using the LIST_ENTRY
    //    FxRequest::m_InFlightListEntry
    //
    // 4) Request has been cancelled, but the driver has not been notified
    //
    //    It is on the LIST_ENTRY m_Cancelled using the LIST_ENTRY
    //    FxRequest::m_OwnerListEntry
    //
    // 5) Request has been cancelled, driver has been notified, but has
    //    not completed it yet
    //
    //    It is on the LIST_ENTRY m_DriverNonCancelable using the LIST_ENTRY
    //    FxRequest::m_OwnerListEntry
    //
    //    It is also on the LIST_ENTRY m_DriverInFlight using the LIST_ENTRY
    //    FxRequest::m_InFlightListEntry
    //

    //
    // This is the Queue of current requests that have not
    // been presented to the driver
    //
    FxIrpQueue          m_Queue;

    //
    // This is the list of requests that driver has accepted
    // and is working on, but wants to be cancelable.
    //
    FxIrpQueue          m_DriverCancelable;

    //
    // This is a list of cancelled requests to be notified to the driver.
    //
    // FxListEntryQueueOwned of FxRequest is used for linkage
    //
    LIST_ENTRY          m_Cancelled;

    //
    // This is a list of request cancelled on queue waiting
    // to be notified to the driver.
    //
    // FxListEntryQueueOwned of FxRequest is used for linkage
    //
    LIST_ENTRY          m_CanceledOnQueueList;

    //
    // This is a list of requests that the driver has accepted
    // and is working on, but has not been completed.
    //
    // They may, or may not be cancelable.
    //
    // FxListEntryDriverOwned of FxRequest is used for linkage
    //
    LIST_ENTRY          m_DriverOwned;

    //
    // This is the list of power stop/start requests to be notified to
    // the driver.
    //
    // FxListEntryDriverOwned of FxRequest is used for linkage
    //
    LIST_ENTRY          m_PowerNotify;

    //
    // This is the list of power stop requests in which the driver
    // has been notified, and must be zero before power code can
    // resume the power thread.
    //
    // FxListEntryDriverOwned of FxRequest is used for linkage
    //
    LIST_ENTRY          m_PowerDriverNotified;

    //
    // Pointer to FxPkgIo object that contains this queue.
    // (No additional reference count)
    //
    FxPkgIo*            m_PkgIo;

    //
    // Weak pointer to the associated FxCxDeviceInfo struct.
    //
    FxCxDeviceInfo*     m_CxDeviceInfo;
    
    //
    // This is the count of currently executing callback
    // handlers into the device driver.
    //
    // It is used to control the dispatch state to prevent stack
    // recursions, as well as to handle notification when a queue
    // is idle and has no callbacks outstanding.
    //
    volatile ULONG      m_Dispatching;

    //
    // This is set when a queue goes from empty to
    // having requests and allows a callback to the driver
    // when a queue is ready.
    //
    volatile BOOLEAN    m_TransitionFromEmpty;
    
    //
    // This flag is set when the we return no_more_requests from 
    // WdfRequesdtGetNextRequest but the queue actually holds one or 
    // more requests in cancellation state (their cancellation routine 
    // are already running). 
    // This flag insures that we call the ReadyNotify callback when a new 
    // request is inserted in the queue.
    //
    volatile BOOLEAN    m_ForceTransitionFromEmptyWhenAddingNewRequest;

    //
    // This is set when the driver starts a WdfIoQueueStopAndPurge operation.
    // This is cleared in the following conditions:
    //  (a) there are no more driver owned requests.
    //  (b) driver re-enables the dispatch gate.
    // When set any requeued requests will be automatically deleted.
    //
    volatile BOOLEAN    m_CancelDispatchedRequests;

    BOOLEAN             m_IsDevicePowerPolicyOwner;

    //
    // This is the number of requests the driver
    // currently is processing both cancellable and noncancellable.
    //
    // For serial queue dispatch mode, this is used
    // to control when a request can be presented to the driver.
    // This is also used to implement counted Queues to make
    // sure the count doesn't exceed max. allowed on the 
    // parallel Queue.
    //
    volatile LONG       m_DriverIoCount;

    //
    // This is the number of requests that are about to be completed using two
    // phase completion technique (to support queued-by-driver requests).
    //
    volatile LONG       m_TwoPhaseCompletions;

    //
    // These are the driver configured callbacks to send
    // I/O events to the driver
    //
    FxIoQueueIoDefault         m_IoDefault;
    FxIoQueueIoStop            m_IoStop;
    FxIoQueueIoResume          m_IoResume;
    FxIoQueueIoRead            m_IoRead;
    FxIoQueueIoWrite           m_IoWrite;
    FxIoQueueIoDeviceControl   m_IoDeviceControl;
    FxIoQueueIoInternalDeviceControl m_IoInternalDeviceControl;
    FxIoQueueIoCanceledOnQueue m_IoCanceledOnQueue;

    FxCallbackLock*     m_IoCancelCallbackLockPtr;

    //
    // These are status events registered by the device driver
    //
    FxIoQueueIoState           m_IdleComplete;
    WDFCONTEXT                 m_IdleCompleteContext;

    FxIoQueueIoState           m_PurgeComplete;
    WDFCONTEXT                 m_PurgeCompleteContext;

    FxIoQueueIoState           m_ReadyNotify;
    WDFCONTEXT                 m_ReadyNotifyContext;

    //
    // The following items support the callback constraints
    // and handle locking and deferral
    //
    WDF_EXECUTION_LEVEL       m_ExecutionLevel;
    WDF_SYNCHRONIZATION_SCOPE m_SynchronizationScope;

    //
    // These are the passive and dispatch level presentation locks
    //
    FxCallbackSpinLock  m_CallbackSpinLock;
    FxCallbackMutexLock m_CallbackMutexLock;

    //
    // This pointer allows the proper lock to be acquired
    // based on the configuration with a minimal of runtime
    // checks. This is configured by ConfigureLocking()
    //
    FxCallbackLock*     m_CallbackLockPtr;
    FxObject*           m_CallbackLockObjectPtr;

    //
    // The IoQueue must sometimes defer event delivery to the
    // device driver due to interactions with current locks held,
    // and the device driver configured callback constraints.
    //
    // When a deferral occurs, a DPC or FxSystemWorkItem is used
    // to post an event to later deliver the event(s) to the device
    // driver. Whether a DPC or WorkItem is used depends
    // on the drivers configured execution level constraints.
    //
    // The IoQueue is designed to be robust in that multiple events
    // may occur while the queued DPC or WorkItem is outstanding,
    // and they will be properly processed without having to enqueue
    // one for each event. Basically, they are just a signal that
    // an IoQueue needs some attention that could result in device
    // driver notification.
    //
    // KDPC is only needed in kernel mode
    //

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    KDPC               m_Dpc;
#endif

    FxSystemWorkItem*  m_SystemWorkItem;

    //
    // These are true if the associated Dpc or Work Item is queued
    //
    BOOLEAN            m_DpcQueued;
    BOOLEAN            m_WorkItemQueued;

    //
    // Track whether the above DPC and WorkItem needs to be requeued.
    //
    BOOLEAN            m_RequeueDeferredDispatcher;
    
    // This is set when the Queue is power idle
    MxEvent            m_PowerIdle;

#if (FX_CORE_MODE==FX_CORE_USER_MODE)
    MxEvent            m_RequestWaitEventUm;
#endif

public:

    //
    // List node to tie the queue with FxPkgIo.
    //
    FxIoQueueNode      m_IoPkgListNode;
    //
    // List entry is used by FxPkgIo to iterate list all the queues without
    // holding a lock.
    //
    SINGLE_LIST_ENTRY  m_PowerSListEntry;


    // Factory function
    _Must_inspect_result_    
    static
    NTSTATUS
    _Create(
        __in PFX_DRIVER_GLOBALS         DriverGlobals,
        __in_opt PWDF_OBJECT_ATTRIBUTES Attributes,
        __in PWDF_IO_QUEUE_CONFIG       Config,
        __in_opt FxDriver*              Caller,
        __in FxPkgIo*                   PkgIo,
        __in BOOLEAN                    InitialPowerStateOn,
        __deref_out FxIoQueue**         Object
        );

    FxIoQueue(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in FxPkgIo* PkgIo
        );

    virtual
    ~FxIoQueue(
       );

    _Must_inspect_result_
    NTSTATUS
    Initialize(
        __in PWDF_IO_QUEUE_CONFIG       pConfig,
        __in_opt PWDF_OBJECT_ATTRIBUTES QueueAttributes,
        __in_opt FxDriver*              Caller,
        __in BOOLEAN                    InitialPowerStateOn
        );

    __inline
    FxCxDeviceInfo*
    GetCxDeviceInfo(
        VOID
        )
    {
        return m_CxDeviceInfo;
    }

    BOOLEAN
    IsIoEventHandlerRegistered(
        __in WDF_REQUEST_TYPE RequestType
        );

    __inline
    BOOLEAN
    IsForwardProgressQueue(
        VOID
        )
    {
        return m_SupportForwardProgress;
    }

    _Must_inspect_result_
    NTSTATUS
    GetReservedRequest(
        __in MdIrp Irp,
        __deref_out_opt FxRequest **ReservedRequest       
        );

    WDFQUEUE
    __inline
    GetHandle(
        VOID
        )
    {
        return (WDFQUEUE) GetObjectHandle();
    }

    //
    // Start the Queue
    //
    VOID
    QueueStart(
        VOID
        );

    //
    // Idle/Stop the Queue
    //
    _Must_inspect_result_
    NTSTATUS
    QueueIdle(
        __in BOOLEAN                    CancelQueueRequests,
        __in_opt PFN_WDF_IO_QUEUE_STATE IdleComplete,
        __in_opt WDFCONTEXT             Context
        );

    __inline
    NTSTATUS
    InvokeAllocateResourcesCallback(
    __in FxRequest *Request
    )
    /*++
    
    Routine Description:
        Give callback to allocate resources at runtime for a general request
        (not a reserved request).     
    
    --*/
    {
        NTSTATUS status;

        ASSERT(Request->IsReserved() == FALSE);
        
        status = STATUS_SUCCESS;
        if (m_FwdProgContext->m_IoResourcesAllocate.Method != NULL)
        {
            Request->SetPresented();
            status = m_FwdProgContext->m_IoResourcesAllocate.Invoke(
                GetHandle(), Request->GetHandle());
        }
        
        return status;
    }

    _Must_inspect_result_
    NTSTATUS
    QueueRequest(
        __in FxRequest* pRequest
        );

    __inline
    BOOLEAN
    IsState(
        __in WDF_IO_QUEUE_STATE State
        )
    {
        ASSERT(!(State & 0x80000000)); // Don't allow FX_IO_QUEUE_SET states
        return (((int)m_QueueState & (int) (State)) != 0);
    }

    _Releases_lock_(this->m_SpinLock.m_Lock)
    __drv_requiresIRQL(DISPATCH_LEVEL)
    BOOLEAN
    DispatchEvents(
        __in __drv_restoresIRQL KIRQL PreviousIrql,
        __in_opt FxRequest* NewRequest = NULL
        );

    _Must_inspect_result_
    BOOLEAN
    CanThreadDispatchEventsLocked(
        __in KIRQL PreviousIrql
        );

    VOID
    DispatchRequestToDriver(
        __in FxRequest* pRequest
        );

    __drv_requiresIRQL(DISPATCH_LEVEL)
    VOID
    ProcessAcknowledgedRequests(
        __in FxRequest* Request,
        __out PKIRQL PreviousIrql
        );

    __inline
    BOOLEAN
    IsPowerStateNotifyingDriver(
        VOID
        )
    {
        if (m_PowerState == FxIoQueuePowerStoppingNotifyingDriver ||
            m_PowerState == FxIoQueuePowerPurgeNotifyingDriver ||
            m_PowerState == FxIoQueuePowerRestartingNotifyingDriver)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    _Releases_lock_(this->m_SpinLock.m_Lock)
    VOID
    CancelForQueue(
        __in FxRequest* pRequest,
        __in __drv_restoresIRQL KIRQL PreviousIrql
        );

    _Releases_lock_(this->m_SpinLock.m_Lock)
    __drv_requiresIRQL(DISPATCH_LEVEL)
    __inline
    VOID
    DispatchInternalEvents(
        __in __drv_restoresIRQL KIRQL PreviousIrql
        )
    /*++

        Routine Description:

        Dispatch events and requests from the queue to the driver.

        The IoQueue object lock must be held on entry, but this
        routine returns to the caller with the lock released.

        To avoid recursion, this routine checks to see if this or another
        thread is already in the dispatch-event loop. If so, it
        doesn't re-enter the dispatch-even loop.

    --*/
    {
        if (m_Dispatching == 0)
        {
            //
            // Nobody is dispatching events so we must
            // call the main DispatchEvents function because
            // the caller of this function might have affected the
            // state of the queue.
            //
            (VOID) DispatchEvents(PreviousIrql);
        }
        else
        {
            Unlock(PreviousIrql);
        }
    }

    VOID
    InsertQueueDpc(
        VOID
    );

    VOID
    DeferredDispatchRequestsFromWorkerThread(
        VOID
        );

    __inline
    VOID
    SetCxDeviceInfo(
        __in FxCxDeviceInfo* CxDeviceInfo 
        )
    {
        m_CxDeviceInfo = CxDeviceInfo;
    }

    __inline
    VOID
    SetInterruptQueue(
        VOID
        )
    {
        MarkNoDeleteDDI(ObjectLock);
    }

    __inline
    static
    FxIoQueue*
    _FromIoPkgListEntry(
        __in PLIST_ENTRY Entry
        )
    {
        return CONTAINING_RECORD(Entry, FxIoQueue, m_IoPkgListNode.m_ListEntry);
    }

    __inline
    static
    FxIoQueue*
    _FromPowerSListEntry(
        __in PSINGLE_LIST_ENTRY Entry
        )
    {
        return CONTAINING_RECORD(Entry, FxIoQueue, m_PowerSListEntry);
    }

    __inline
    VOID
    SetPowerState(
        __in FXIOQUEUE_POWER_STATE PowerState
        )
    {
        if (m_PowerManaged)
        {
            m_PowerState = PowerState;
        }
    }

    VOID
    StartPowerTransitionOn(
        VOID
        );

    VOID
    StartPowerTransitionOff(
        VOID
        );

    VOID
    StopProcessingForPower(
        __in FxIoStopProcessingForPowerAction Action
        );

    VOID
    ResumeProcessingForPower(
        VOID
        );
    
    VOID
    SetStateForShutdown(
        VOID
        );

    VOID
    SetState(
        __in FX_IO_QUEUE_SET_STATE NewStatus
       );

    _Must_inspect_result_
    NTSTATUS
    ConfigureConstraints(
        __in_opt PWDF_OBJECT_ATTRIBUTES ObjectAttributes,
        __in_opt FxDriver*              Caller
        );

    VOID
    DeferredDispatchRequestsFromDpc(
        VOID
        );

    VOID
    CancelForDriver(
        __in FxRequest* pRequest
        );

    // Do not specify argument names
    FX_DECLARE_VF_FUNCTION_P1(
    VOID, 
    VerifyCancelForDriver, 
        _In_ FxRequest*
        );

    VOID
    FlushByFileObject(
        __in MdFileObject FileObject
        );

    _Must_inspect_result_
    NTSTATUS
    GetRequest(
        __in_opt  MdFileObject FileObject,
        __in_opt  FxRequest*   TagRequest,
        __deref_out FxRequest**  pOutRequest
        );

    // ForwardRequest Verifiers
    // Do not specify argument names
    FX_DECLARE_VF_FUNCTION_P2(
    NTSTATUS, 
    VerifyForwardRequest, 
        _In_ FxIoQueue*, 
        _In_ FxRequest* 
        );

    _Must_inspect_result_
    NTSTATUS
    ForwardRequest(
        __in FxIoQueue* pDestQueue,
        __in FxRequest* pRequest
       );

    _Must_inspect_result_
    NTSTATUS
    ForwardRequestWorker(
        __in FxRequest* Request,
        __in FxIoQueue* DestQueue
        );

    // ForwardRequestWorker verifiers
    // Do not specify argument names
    FX_DECLARE_VF_FUNCTION_P1_EX(
    ,
    SHORT,
    0,
    VerifyForwardRequestUpdateFlags, 
        _In_ FxRequest*
        );
        
    _Must_inspect_result_
    NTSTATUS
    QueueRequestFromForward(
        __in FxRequest* pRequest
        );

    // GetRequest Verifiers
    // Do not specify argument names
    FX_DECLARE_VF_FUNCTION_P1(
    NTSTATUS, 
    VerifyGetRequestUpdateFlags, 
        _In_ FxRequest*
        );

    // Do not specify argument names
    FX_DECLARE_VF_FUNCTION_P1(
    VOID, 
    VerifyGetRequestRestoreFlags, 
        _In_ FxRequest* 
        );

    // Do not specify argument names
    FX_DECLARE_VF_FUNCTION_P1(
    VOID, 
    VerifyValidateCompletedRequest, 
        _In_ FxRequest*
        );

    __inline
    VOID
    RequestCompletedCallback(
        __in FxRequest* Request
        )
    {
        //
        // This is called when a FxRequest object completes based
        // on the callback event registered by the I/O queue support
        // routines.
        //

        KIRQL irql;

#if FX_VERBOSE_TRACE
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
                            "Enter: WDFQUEUE 0x%p, WDFREQUEST 0x%p",
                            GetObjectHandle(),Request->GetHandle());
#endif
        VerifyValidateCompletedRequest(GetDriverGlobals(), Request);
      
        Lock(&irql);

        //
        // I/O has been completed by the driver
        //
        RemoveFromDriverOwnedList(Request);

        //
        // Don't run the event dispatcher if we come from a Request
        // complete callback in order to prevent stack recursion.
        //
        // Since some other thread (possibly this thread higher on
        // the stack) is running the dispatcher, no events will get lost.
        //
        DispatchInternalEvents(irql);
    }

    __inline
    VOID
    PostRequestCompletedCallback(
        __in FxRequest* Request
        )
    {
        // Do not acccess Request, at this point the object may have already been 
        // deleted or reused.
    
        //
        // This is called when a queued-by-driver request (driver created) is 
        // completed or sent to a lower driver with 'send-and-forget' option.
        // This callback allows the queue to schedule another request for delivery.
        //

        KIRQL irql;

#if FX_VERBOSE_TRACE
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
                            "Enter: WDFQUEUE 0x%p, WDFREQUEST 0x%p",
                            GetObjectHandle(), FxRequest::_ToHandle(Request));
#else
        UNREFERENCED_PARAMETER(Request);
#endif

        Lock(&irql);

        //
        // I/O has been completed by the driver
        //
        PostRemoveFromDriverOwnedList();
        
        //
        // Don't run the event dispatcher if we come from a Request
        // complete callback in order to prevent stack recursion.
        //
        // Since some other thread (possibly this thread higher on
        // the stack) is running the dispatcher, no events will get lost.
        //
        DispatchInternalEvents(irql);
    }

    __inline
    VOID
    PreRequestCompletedCallback(
        __in FxRequest* Request
        )
    {
        //
        // This is called when a driver created request is about to be completed.
        // This callback removes the FxRequest object from the internal queue linked
        // lists.  A call to PostRequestCompletedCallback must be made after irp is
        // completed.
        //

        KIRQL irql;

#if FX_VERBOSE_TRACE
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
                            "Enter: WDFQUEUE 0x%p, WDFREQUEST 0x%p",
                            GetObjectHandle(),Request->GetHandle());
#endif

        VerifyValidateCompletedRequest(GetDriverGlobals(), Request);

        Lock(&irql);

        //
        // I/O has been completed by the driver
        //
        PreRemoveFromDriverOwnedList(Request);

        Unlock(irql);
    }

    _Must_inspect_result_
    NTSTATUS
    RequestCancelable(
        __in FxRequest* pRequest,
        __in BOOLEAN    Cancelable,
        __in_opt PFN_WDF_REQUEST_CANCEL  EvtRequestCancel,
        __in BOOLEAN    FailIfIrpIsCancelled
       );

    // Do not specify argument names
    FX_DECLARE_VF_FUNCTION_P2(
    NTSTATUS, 
    VerifyRequestCancelable, 
        _In_ FxRequest*, 
        _In_ BOOLEAN
        );

    //
    // Purge the Queue
    //
    _Must_inspect_result_
    NTSTATUS
    QueuePurge(
        __in BOOLEAN                 CancelQueueRequests,
        __in BOOLEAN                 CancelDriverRequests,
        __in_opt PFN_WDF_IO_QUEUE_STATE PurgeComplete,
        __in_opt WDFCONTEXT              Context
        );

    //
    // Idle the Queue
    //
    _Must_inspect_result_
    NTSTATUS
    QueueIdleSynchronously(
        __in BOOLEAN    CancelRequests
        );

    __declspec(noreturn)
    VOID
    FatalError(
        __in NTSTATUS Status
        );

private:

    //
    // Helper functions for event processing loop DispatchEvents()
    //
    __drv_requiresIRQL(DISPATCH_LEVEL)
    VOID
    ProcessIdleComplete(
        __out PKIRQL PreviousIrql
        );

    __drv_requiresIRQL(DISPATCH_LEVEL)
    VOID
    ProcessPurgeComplete(
        __out PKIRQL PreviousIrql
        );

    //
    // This is called after inserting a new request in the IRP queue. 
    //
    // Must be called with the FxIoQueue lock held.
    //
    __inline
    VOID
    CheckTransitionFromEmpty(
        VOID
        )
    {
        if (m_Queue.GetRequestCount() == 1L ||
            m_ForceTransitionFromEmptyWhenAddingNewRequest)
        {            
            SetTransitionFromEmpty();
        }
    }

    //
    // Indicate that the queue went from empty to having one or more requests.
    //
    // Must be called with the FxIoQueue lock held.
    //
    __inline
    VOID
    SetTransitionFromEmpty(
        VOID
        );

    NTSTATUS
    InsertNewRequestLocked(
        __deref_in FxRequest** Request,
        __in KIRQL PreviousIrql
        );

    __inline
    NTSTATUS
    InsertNewRequest(
        __in FxRequest** Request,
        __in KIRQL PreviousIrql
        )
    {
        NTSTATUS status;
        
        if (*Request != NULL)
        {
            status = InsertNewRequestLocked(Request, PreviousIrql);
        }
        else
        {
            status = STATUS_SUCCESS; // nothing to do.
        }

        return status;
    }

    __drv_requiresIRQL(DISPATCH_LEVEL)
    BOOLEAN
    ProcessCancelledRequests(
        __out PKIRQL PreviousIrql
        );

    __drv_requiresIRQL(DISPATCH_LEVEL)
    BOOLEAN
    ProcessCancelledRequestsOnQueue(
        __out PKIRQL PreviousIrql
        );

    __drv_requiresIRQL(DISPATCH_LEVEL)
    BOOLEAN
    ProcessPowerEvents(
        __out PKIRQL PreviousIrql
        );

    __drv_requiresIRQL(DISPATCH_LEVEL)
    VOID
    ProcessReadyNotify(
        __out PKIRQL PreviousIrql
        );

    //
    // Insert in the list of requests that the driver is operating
    // on.
    //
    // Must be called with the FxIoQueue lock held.
    //
    __inline
    VOID
    InsertInDriverOwnedList(
        __in FxRequest* Request
        )
    {
        m_DriverIoCount++;

        InsertTailList(&m_DriverOwned, Request->GetListEntry(FxListEntryDriverOwned));
        return;
    }

    //
    // Remove the request from the list that the driver is operating
    // on.
    //
    // Must be called with the FxIoQueue lock held.
    //
    // Note: ForwardRequest and two phase completions (queued-by-driver) cases breaks 
    //          this up and manipulates the list entry and m_DriverIoCount separately.
    //
    __inline
    VOID
    RemoveFromDriverOwnedList(
        __in FxRequest* Request
        )
    {
        PLIST_ENTRY listEntry;

        listEntry = Request->GetListEntry(FxListEntryDriverOwned);
        
        RemoveEntryList(listEntry);
        InitializeListHead(listEntry);

        m_DriverIoCount--;
        ASSERT(m_DriverIoCount >= 0);

        return;
    }

    BOOLEAN
    IsPagingIo(
        __in MdIrp Irp
    );

    _Must_inspect_result_
    NTSTATUS
    QueueForwardProgressIrpLocked(
        __in MdIrp Irp
        );

    VOID 
    PurgeForwardProgressIrps(
        __in_opt MdFileObject FileObject
        );

    VOID 
    GetForwardProgressIrps(
        __in     PLIST_ENTRY    IrpListHead,
        __in_opt MdFileObject   FileObject
        );

    VOID 
    CancelIrps(
        __in PLIST_ENTRY    IrpListHead
        );

    _Must_inspect_result_
    MdIrp
    GetForwardProgressIrpLocked(
        __in_opt PFILE_OBJECT FileObject
        );

    //
    // Post-Remove the request from the list that the driver is operating on 
    // (the second of two phase completion).
    //
    // Must be called with the FxIoQueue lock held.
    //
    __inline
    VOID
    PostRemoveFromDriverOwnedList(
        VOID
        )
    {
        m_TwoPhaseCompletions--;
        ASSERT(m_TwoPhaseCompletions >= 0);

        m_DriverIoCount--;
        ASSERT(m_DriverIoCount >= 0);
        return;
    }

    //
    // Pre-Remove the request from the list that the driver is operating on 
    // (the first of two phase completion).
    //
    // Must be called with the FxIoQueue lock held.
    //
    __inline
    VOID
    PreRemoveFromDriverOwnedList(
        __in FxRequest* Request
        )
    {
        PLIST_ENTRY listEntry;

        listEntry = Request->GetListEntry(FxListEntryDriverOwned);
        
        RemoveEntryList(listEntry);
        InitializeListHead(listEntry);

        m_TwoPhaseCompletions++;
        ASSERT(m_TwoPhaseCompletions > 0);

        return;
    }

protected:

    static
    EVT_SYSTEMWORKITEM    
    _DeferredDispatchThreadThunk;

    static
    MdCancelRoutineType
    _WdmCancelRoutineForReservedIrp;

    static
    EVT_IRP_QUEUE_CANCEL 
    _IrpCancelForQueue;

    //
    // This is our Cancel Safe Queue Callback from
    // FxIrpQueue notifying us of an I/O cancellation
    // on a driver owned request (driver queue)
    //
    static
    EVT_IRP_QUEUE_CANCEL
    _IrpCancelForDriver;

    static
    MdDeferredRoutineType
    _DeferredDispatchDpcThunk;

    static
    EVT_WDF_IO_QUEUE_STATE 
    _IdleComplete;

};

#endif //_FXIOQUEUE_H_
