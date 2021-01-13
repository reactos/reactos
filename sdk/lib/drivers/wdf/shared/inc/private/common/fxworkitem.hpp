/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxWorkItem.hpp

Abstract:

    This module implements a frameworks managed WorkItem that
    can synchrononize with driver frameworks object locks.

Author:




Environment:

    Both kernel and user mode

Revision History:


--*/

#ifndef _FXWORKITEM_H_
#define _FXWORKITEM_H_

//
// Driver Frameworks WorkItem Design:
//
// The driver frameworks provides an optional WorkItem wrapper object that allows
// a reference counted WorkItem object to be created that can synchronize
// automatically with certain frameworks objects.
//
// This provides automatic synchronization between the WorkItems execution, and the
// frameworks objects event callbacks into the device driver.
//
// The WDFWORKITEM object is designed to be re-useable, in which it can be re-linked
// into the WorkItem queue after firing.
//
// Calling GetWorkItemPtr returns the WDM IoWorkItem.
//

class FxWorkItem : public FxNonPagedObject {

private:
    //
    // WDM work item.
    //
    MxWorkItem         m_WorkItem;

    // Ensures only one of either Delete or Cleanup runsdown the object
    BOOLEAN            m_RunningDown;

    //
    // If this is set, a WorkItem has been enqueued
    //
    BOOLEAN            m_Enqueued;

    //
    // This count is used to prevent the object from being deleted if
    // one worker thread is preempted right after we drop the lock to call
    // the client callback and another workitem gets queued and runs
    // to completion and signals the event.
    //
    ULONG           m_WorkItemRunningCount;

    //
    // This is the Framework object who is associated with the work
    // item if supplied
    //
    FxObject*          m_Object;

    //
    // This is the callback lock for the object this WorkItem will
    // synchronize with
    //
    FxCallbackLock*    m_CallbackLock;

    //
    // This is the object whose reference count actually controls
    // the lifetime of the m_CallbackLock
    //
    FxObject*          m_CallbackLockObject;

    //
    // This is the user supplied callback function and context
    //
    PFN_WDF_WORKITEM   m_Callback;

    //
    // This event is signaled when the workitem is done processing
    // an Enqueue request.
    //
    FxCREvent          m_WorkItemCompleted;

    //
    // This is a pointer to thread object that invoked our workitem
    // callback. This value will be used to avoid deadlock when we try
    // to flush the workitem.
    //
    MxThread          m_WorkItemThread;

public:

    static
    _Must_inspect_result_
    NTSTATUS
    _Create(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in PWDF_WORKITEM_CONFIG Config,
        __in PWDF_OBJECT_ATTRIBUTES Attributes,
        __in FxObject* ParentObject,
        __out WDFWORKITEM* WorkItem
        );

/*++

Routine Description:

    Construct an FxWorkItem

Arguments:

Returns:

    NTSTATUS

--*/

    FxWorkItem(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    virtual
    ~FxWorkItem(
       );

    MdWorkItem
    GetWorkItemPtr() {
        return m_WorkItem.GetWorkItem();
    }

/*++

Routine Description:

    Initialize the WorkItem using either the caller supplied WorkItem
    struct, or if NULL, our own internally allocated one

Arguments:

Returns:

    NTSTATUS

--*/
    _Must_inspect_result_
    NTSTATUS
    Initialize(
        __in PWDF_OBJECT_ATTRIBUTES Attributes,
        __in PWDF_WORKITEM_CONFIG Config,
        __in FxObject* ParentObject,
        __out WDFWORKITEM* WorkItem
        );

    virtual
    BOOLEAN
    Dispose(
        VOID
        );

    VOID
    Enqueue(
        );

    WDFOBJECT
    GetAssociatedObject(
        )
    {
        if( m_Object != NULL ) {
            return m_Object->GetObjectHandle();
        }
        else {
            return NULL;
        }
    }

    WDFWORKITEM
    GetHandle(
        VOID
        )
    {
        return (WDFWORKITEM) GetObjectHandle();
    }

private:

    //
    // Called from Dispose, or cleanup list to perform final flushing of any
    // outstanding DPC's and dereferencing of objects.
    //
    VOID
    FlushAndRundown(
        );

    VOID
    WorkItemHandler(
        );

    static
    MX_WORKITEM_ROUTINE
    WorkItemThunk;

    VOID
    WaitForSignal(
        VOID
        );

public:
    VOID
    FlushAndWait(
        VOID
        );
};

#endif // _FXWORKITEM_H_

