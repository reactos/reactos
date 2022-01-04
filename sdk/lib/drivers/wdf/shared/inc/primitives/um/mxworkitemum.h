/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxWorkItemUm.h

Abstract:

    User mode implementation of work item
    class defined in MxWorkItem.h

    ***********PLEASE NOTE*****************************
    A significant difference from kernel mode implementation of work item
    is that user mode version of MxWorkItem::_Free synchronously waits for
    callback to return.

    This implies that _Free cannot be invoked from within the callback otherwise
    it would lead to a deadlock.

    PLEASE NOTE that _Free cannot be made to return without waiting without
    significant changes.

    If Free is not made to wait synchronously there is a potential for binary
    unload while workitem is running - even with waiting for an event/reference
    count etc., the tail instructions may be running.
    The only way to resolve that is to move work-item code out of framework
    binary and into the host so that host can take a reference on framework
    binary around the work item callback invocation (similar to the way I/O
    manager keeps a reference on the device object around the invocation of

    workitem callback).
    ****************************************************

Author:



Revision History:






--*/

#pragma once

typedef
VOID
MX_WORKITEM_ROUTINE (
    __in MdDeviceObject DeviceObject,
    __in_opt PVOID Context
    );

typedef MX_WORKITEM_ROUTINE *PMX_WORKITEM_ROUTINE;

typedef struct {
    MdDeviceObject DeviceObject;

    //
    // threadpool wait block
    //
    PTP_WAIT WaitBlock;

    HANDLE WorkItemEvent;

    PMX_WORKITEM_ROUTINE Callback;

    PVOID Context;

    //
    // True if callbacks run in the default thread pool environment,
    // rather than in an environment explicitly owned by the driver.
    // This has implications in MxWorkItem::_Free.
    //
    BOOLEAN DefaultThreadpoolEnv;
} UmWorkItem;

typedef UmWorkItem*     MdWorkItem;

#include "MxWorkItem.h"

__inline
MxWorkItem::MxWorkItem(
    )
{
    m_WorkItem = NULL;
}

_Must_inspect_result_
__inline
NTSTATUS
MxWorkItem::Allocate(
    __in MdDeviceObject DeviceObject,
    __in_opt PVOID ThreadPoolEnv
    )
{
    DWORD err = 0;

    m_WorkItem = (MdWorkItem)::HeapAlloc(
        GetProcessHeap(),
        0,
        sizeof(UmWorkItem)
        );

    if (NULL == m_WorkItem) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ZeroMemory(m_WorkItem, sizeof(UmWorkItem));

    m_WorkItem->WorkItemEvent = CreateEvent(
        NULL,
        FALSE,
        FALSE,
        NULL);

    if (NULL == m_WorkItem->WorkItemEvent) {
        err = GetLastError();
        goto exit;
    }

    m_WorkItem->WaitBlock = CreateThreadpoolWait(
                        _WorkerThunk,
                        this->GetWorkItem(), // Context to callback function
                        (PTP_CALLBACK_ENVIRON)ThreadPoolEnv
                        );

    if (m_WorkItem->WaitBlock == NULL) {
        err = GetLastError();
        goto exit;
    }

    m_WorkItem->DefaultThreadpoolEnv = (NULL == ThreadPoolEnv);

    m_WorkItem->DeviceObject = DeviceObject;

exit:
    //
    // Cleanup in case of failure
    //
    if (0 != err) {
        if (NULL != m_WorkItem->WorkItemEvent) {
            CloseHandle(m_WorkItem->WorkItemEvent);
            m_WorkItem->WorkItemEvent = NULL;
        }

        ::HeapFree(GetProcessHeap(), 0, m_WorkItem);
        m_WorkItem = NULL;
    }

    return NTSTATUS_FROM_WIN32(err);
}

__inline
VOID
MxWorkItem::Enqueue(
    __in PMX_WORKITEM_ROUTINE Callback,
    __in PVOID Context
    )
{
    //
    // ASSUMPTION: This function assumes that another call to Enqueue
    // is made only after the callback has been invoked, altough it is OK
    // to make another call from within the callback.
    //
    // It is up to a higher layer/caller to ensure this.
    // For example: FxSystemWorkItem layered on top of MxWorkItem ensures this.
    //

    //
    // Since multiple calls to Enqueue cannot be made at the same time
    // as explained above, it is OK to store callback and context in
    // the workitem itself.
    //
    // This behavior is similar to that of IoQueueWorkItem which accepts
    // a callback and a context which are stored within the work-item.
    //

    m_WorkItem->Callback = Callback;
    m_WorkItem->Context = Context;

    //
    // We must register the event with the wait object before signaling it
    // to trigger the wait callback.
    //
    SetThreadpoolWait(m_WorkItem->WaitBlock,
                      m_WorkItem->WorkItemEvent,
                      NULL  // timeout
                      );

    SetEvent(m_WorkItem->WorkItemEvent);
}

__inline
MdWorkItem
MxWorkItem::GetWorkItem(
    )
{
    return m_WorkItem;
}

__inline
VOID
MxWorkItem::_Free(
    __in MdWorkItem Item
    )
{
    //
    // PLEASE NOTE that _Free waits for callback to return synchronously.
    //
    // DO NOT call _Free from work item callback otherwise it would cause a
    // deadlock.
    //
    // Please see comments on the top of the file.
    //

    if (NULL != Item) {
        //
        // Wait indefinitely for work item to complete
        //
        if (NULL != Item->WaitBlock) {
            //
            // this will prevent any new waits to be queued but callbacks
            // already queued will still occur.
            //
            SetThreadpoolWait(Item->WaitBlock, NULL, NULL);

            //
            // If the callbacks ran in the default thread pool environment,
            // wait for callbacks to finish.
            // If they ran in an environment explicitly owned by the driver,
            // then this wait will happen before the driver DLL is unloaded,
            // the host takes care of this.
            //
            if (Item->DefaultThreadpoolEnv) {
                WaitForThreadpoolWaitCallbacks(Item->WaitBlock,
                                               FALSE   // donot cancel pending waits
                                               );
            }

            //
            // Release the wait object.
            //
            CloseThreadpoolWait(Item->WaitBlock);
        }

        if (NULL != Item->WorkItemEvent) {
            CloseHandle(Item->WorkItemEvent);
            Item->WorkItemEvent = NULL;
        }

        ::HeapFree(
            GetProcessHeap(),
            0,
            Item
            );
    }
}

__inline
VOID
MxWorkItem::Free(
    )
{
    //
    // PLEASE NOTE that _Free waits for callback to return synchronously.
    //
    // DO NOT call Free from work item callback otherwise it would cause a
    // deadlock.
    //
    // Please see comments on the top of the file.
    //

    if (NULL != m_WorkItem) {
        MxWorkItem::_Free(m_WorkItem);
        m_WorkItem = NULL;
    }
}

//
// FxAutoWorkitem
//
__inline
MxAutoWorkItem::~MxAutoWorkItem(
    )
{
    this->Free();
}


