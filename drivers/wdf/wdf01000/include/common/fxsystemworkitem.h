#ifndef _FXSYSTEMWORKITEM_H
#define _FXSYSTEMWORKITEM_H

#include "fxglobals.h"
#include "fxnonpagedobject.h"
#include "fxwaitlock.h"
#include "primitives/mxworkitem.h"

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
NTAPI
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

    virtual
    ~FxSystemWorkItem(
       );

    __inline
    BOOLEAN
    TryToEnqueue(
        __in PFN_WDF_SYSTEMWORKITEM CallbackFunc,
        __in PVOID                  Parameter
        )
    {
        return EnqueueWorker(CallbackFunc, Parameter, FALSE);
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

        if (result == 0)
        {
            m_RemoveEvent.Set();
        }
    }

    static
    _Must_inspect_result_
    NTSTATUS
    NTAPI
    _Create(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in PVOID              WdmObject,
        __out FxSystemWorkItem** pObject
        );

    VOID
    WaitForExit(
        VOID
        );

    DECLARE_INTERNAL_NEW_OPERATOR();

private:

    FxSystemWorkItem(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    BOOLEAN
    EnqueueWorker(
        __in PFN_WDF_SYSTEMWORKITEM  Func,
        __in PVOID   Parameter,
        __in BOOLEAN AssertIfAlreadyQueued
        );

    static
    MX_WORKITEM_ROUTINE 
    NTAPI
    _WorkItemThunk;

    VOID
    WorkItemHandler(
        );

    _Must_inspect_result_
    NTSTATUS
    Initialize(
        __in PVOID WdmObject
        );

};

#endif //_FXSYSTEMWORKITEM_H
