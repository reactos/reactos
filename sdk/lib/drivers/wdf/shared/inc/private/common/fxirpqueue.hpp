/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxIrpQueue.hpp

Abstract:

    This module implements a common queue structure for the
    driver frameworks built around the Cancel Safe Queues pattern

Author:






Environment:

    Both kernel and user mode

Revision History:


--*/

#ifndef _FXIRPQUEUE_H_
#define _FXIRPQUEUE_H_

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
#include "FxIrpKm.hpp"
#else
#include "FxIrpUm.hpp"
#endif


//
// IRP DriverContext[] entry used.
//
// We use the same entry that the CSQ package does
// which is OK since we can't use CSQ if we implement the
// cancel handler ourselves
//
#define FX_IRP_QUEUE_CSQ_CONTEXT_ENTRY 3

//
// FxIrpQueue entry identifier
//
#define FX_IRP_QUEUE_ENTRY_IDENTIFIER 1

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
#include "FxIrpKm.hpp"
#else if ((FX_CORE_MODE)==(FX_CORE_USER_MODE))
#include "FxIrpUm.hpp"
#endif


//
// This is the cancel function callback for the IrpQueue to its caller/parent
// This is called holding the lock of the object owning the IrpQueue, and it
// is responsible for completing the IRP.
//

extern "C" {
__drv_functionClass(EVT_IRP_QUEUE_CANCEL)
__drv_requiresIRQL(DISPATCH_LEVEL)
typedef
VOID
EVT_IRP_QUEUE_CANCEL (
    __in FxIrpQueue* Queue,
    __in MdIrp        Irp,
    __in PMdIoCsqIrpContext pCsqContext,
    __in KIRQL CallerIrql
    );

typedef EVT_IRP_QUEUE_CANCEL *PFN_IRP_QUEUE_CANCEL;
}

class FxIrpQueue {

    friend VOID GetTriageInfo(VOID);

private:

    //
    // The Queue
    //
    LIST_ENTRY        m_Queue;

    //
    // The object whose lock controls the queue.
    // Provided by the client object.
    //
    FxNonPagedObject* m_LockObject;

    //
    // Callers registered cancel callback
    //
    PFN_IRP_QUEUE_CANCEL m_CancelCallback;

    //
    // Count of requests in the Queue
    //
    LONG              m_RequestCount;

public:

    FxIrpQueue(
        VOID
        );

    ~FxIrpQueue(
        VOID
        );

    VOID
    Initialize(
        __in FxNonPagedObject* LockObject,
        __in PFN_IRP_QUEUE_CANCEL Callback
        );

    _Must_inspect_result_
    NTSTATUS
    InsertTailRequest(
        __inout MdIrp Irp,
        __in_opt PMdIoCsqIrpContext CsqContext,
        __out_opt ULONG* pRequestCount
        );

    _Must_inspect_result_
    NTSTATUS
    InsertHeadRequest(
        __inout MdIrp Irp,
        __in_opt PMdIoCsqIrpContext CsqContext,
        __out_opt ULONG* pRequestCount
        );

    MdIrp
    GetNextRequest(
        __out PMdIoCsqIrpContext* pCsqContext
        );

    _Must_inspect_result_
    NTSTATUS
    GetNextRequest(
        __in_opt  PMdIoCsqIrpContext  TagContext,
        __in_opt  MdFileObject         FileObject,
        __out FxRequest**          ppOutRequest
        );

    _Must_inspect_result_
    NTSTATUS
    PeekRequest(
        __in_opt PMdIoCsqIrpContext  TagContext,
        __in_opt MdFileObject         FileObject,
        __out FxRequest**          ppOutRequest
        );

    MdIrp
    RemoveRequest(
        __in PMdIoCsqIrpContext Context
        );

    //
    // Return whether the queue is empty
    // (for optimizing the non-pending case in the caller)
    //
    inline
    BOOLEAN
    IsQueueEmpty() {

        if( IsListEmpty(&m_Queue) ) {

            ASSERT(m_RequestCount == 0);

            return TRUE;
        }
        else {
            ASSERT(m_RequestCount != 0);

            return FALSE;
        }
    }

    //
    // Return count of queued and driver pending requests.
    //
    inline
    LONG
    GetRequestCount() {
        return m_RequestCount;
    }

    BOOLEAN
    IsIrpInQueue(
        __in PMdIoCsqIrpContext Context
        );


private:

    //
    // Insert an IRP on the cancel list
    //
    _Must_inspect_result_
    NTSTATUS
    InsertIrpInQueue(
        __inout   MdIrp                Irp,
        __in_opt  PMdIoCsqIrpContext Context,
        __in      BOOLEAN             InsertInHead,
        __out_opt ULONG*              pRequestCount
        );

    // Do not specify argument names
    FX_DECLARE_VF_FUNCTION_P1(
    VOID,
    VerifyRemoveIrpFromQueueByContext,
        __in PMdIoCsqIrpContext
        );

    //
    // Ask to remove an IRP from the cancel list by Context,
    // and return NULL if its been cancelled
    //
    MdIrp
    RemoveIrpFromQueueByContext(
        __in PMdIoCsqIrpContext Context
        );

    //
    // Remove a request from the head of the queue if PeekContext == NULL,
    // or the next request after PeekContext if != NULL
    //
    MdIrp
    RemoveNextIrpFromQueue(
        __in_opt  PVOID                PeekContext,
        __out_opt PMdIoCsqIrpContext* pCsqContext
        );

    //
    // Peek an IRP in the queue
    //
    MdIrp
    PeekNextIrpFromQueue(
        __in_opt MdIrp  Irp,
        __in_opt PVOID  PeekContext
        );

    //
    // Unconditionally remove the IRP from the list entry
    //
    inline
    VOID
    RemoveIrpFromListEntry(
        __inout FxIrp*   Irp
        )
    {
      PLIST_ENTRY entry = Irp->ListEntry();
      RemoveEntryList(entry);
      InitializeListHead(entry);
      m_RequestCount--;
      ASSERT(m_RequestCount >= 0);
    }

    //
    // WDM IRP cancel function
    //
    static
    MdCancelRoutineType
    _WdmCancelRoutineInternal;

    //
    // Lock functions accessed from WDM cancel callback
    //
    __inline
    void
    LockFromCancel(
        __out PKIRQL PreviousIrql
        )
    {
        m_LockObject->Lock(PreviousIrql);
    }

    __inline
    void
    UnlockFromCancel(
        __in KIRQL PreviousIrql
        )
    {
        m_LockObject->Unlock(PreviousIrql);
    }
};

#endif // _FXIRPQUEUE_H
