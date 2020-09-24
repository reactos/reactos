/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxSystemThread.cpp

Abstract:

    This is the implementation of the FxSystemThread object.


Author:



Environment:

    Kernel mode only

Revision History:


--*/

#include "fxcorepch.hpp"

extern "C" {
#include "FxSystemThread.tmh"
}


FxSystemThread::FxSystemThread(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    ) :
    FxNonPagedObject(FX_TYPE_SYSTEMTHREAD, 0, FxDriverGlobals)
{

    m_Initialized = FALSE;
    m_ThreadPtr = NULL;
    m_Exit = FALSE;
    m_PEThread = NULL;

    //m_Spinup.WorkerRoutine = NULL; // Async-Thread-Spinup
    m_Reaper.WorkerRoutine = NULL;

    InitializeListHead(&m_WorkList);
    m_InitEvent.Initialize(NotificationEvent, FALSE);
    m_WorkEvent.Initialize(NotificationEvent, FALSE);
}

FxSystemThread::~FxSystemThread() {

}

_Must_inspect_result_
NTSTATUS
FxSystemThread::_CreateAndInit(
    __deref_out FxSystemThread** SystemThread,
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in WDFDEVICE Device,
    __in PDEVICE_OBJECT DeviceObject
    )
{
    NTSTATUS status;
    FxSystemThread *pThread = NULL;

    *SystemThread = NULL;

    pThread = new(FxDriverGlobals) FxSystemThread(FxDriverGlobals);
    if (pThread == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFDEVICE 0x%p !devobj %p could not allocate a thread for handling "
            "power requests %!STATUS!",
            Device, DeviceObject, status);

        return status;
    }

    if (pThread->Initialize() == FALSE) {
        status = STATUS_UNSUCCESSFUL;

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFDEVICE 0x%p, !devobj %p, could not initialize power thread, "
            "%!STATUS!",
            Device, DeviceObject, status);

        pThread->DeleteFromFailedCreate();

        return status;
    }

    *SystemThread = pThread;

    status = STATUS_SUCCESS;

    return status;
}

//
// Create the system thread in order to be able to service work items
//
// It is recommended this is done from the system process context
// since the threads handle is available to the user mode process
// for a temporary window. XP and later supports OBJ_KERNELHANDLE, but
// DriverFrameworks must support W2K with the same binary.
//
// It is safe to call this at DriverEntry which is in the system process
// to create an initial driver thread, and this driver thread should be
// used for creating any child driver threads on demand.
//
BOOLEAN
FxSystemThread::Initialize()
{
    NTSTATUS status;
    KIRQL irql;

    Lock(&irql);

    // Frameworks bug if this is called with a thread already running
    ASSERT(m_ThreadPtr == NULL);

    ASSERT(m_Initialized == FALSE);

    m_Initialized = TRUE;

    Unlock(irql);

    status = CreateThread();

    return NT_SUCCESS(status) ? TRUE : FALSE;
}

_Must_inspect_result_
NTSTATUS
FxSystemThread::CreateThread(
    VOID
    )
{
    HANDLE threadHandle;
    NTSTATUS status;

    //
    // Take an extra object reference on ourselves to account
    // for the system thread referencing the object while it is running.
    //
    // The thread itself will release this reference in its exit routine
    //
    ADDREF(FxSystemThread::StaticThreadThunk);

    status = PsCreateSystemThread(
        &threadHandle,
        THREAD_ALL_ACCESS,
        NULL, // Obja
        NULL, // hProcess
        NULL, // CLIENT_ID,
        FxSystemThread::StaticThreadThunk,
        this
        );

    if (!NT_SUCCESS(status)) {
        m_Initialized = FALSE;

        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Could not create system thread %!STATUS!", status);
        //
        // Release the reference taken above due to failure
        //
        RELEASE(FxSystemThread::StaticThreadThunk);
    }
    else {
        status = ObReferenceObjectByHandle(
            threadHandle,
            THREAD_ALL_ACCESS,
            NULL,
            KernelMode,
            &m_ThreadPtr,
            NULL
            );

        ASSERT(NT_SUCCESS(status));

        // We can now close the thread handle since we have a pointer reference
        ZwClose(threadHandle);
    }

    return status;
}




































//
// This is called to tell the thread to exit.
//
// It must be called from thread context such as
// the driver unload routine since it will wait for the
// thread to exit.
//
BOOLEAN
FxSystemThread::ExitThread()
{
    NTSTATUS Status;
    KIRQL irql;

    Lock(&irql);

    if( !m_Initialized ) {
        Unlock(irql);
        return TRUE;
    }

    if( m_Exit ) {
        ASSERT(FALSE); // This is not race free, so don't allow it
        Unlock(irql);
        return TRUE;
    }

    // Tell the system thread to exit
    m_Exit = TRUE;

    //
    // The thread could still be spinning up, so we must handle this condition.
    //
    if( m_ThreadPtr == NULL ) {

        Unlock(irql);

        KeEnterCriticalRegion();

        // Wait for thread to start
        Status = m_InitEvent.WaitFor(Executive, KernelMode, FALSE, NULL);

        KeLeaveCriticalRegion();

        UNREFERENCED_PARAMETER(Status);
        ASSERT(NT_SUCCESS(Status));

        //
        // Now we have a thread, wait for it to go away
        //
        ASSERT(m_ThreadPtr != NULL);
    }
    else {
        Unlock(irql);
    }

    m_WorkEvent.Set();

    //
    // We can't be waiting in our own thread for the thread to exit.
    //
    ASSERT(IsCurrentThread() == FALSE);

    KeEnterCriticalRegion();

    // Wait for thread to exit
    Status = KeWaitForSingleObject(
                 m_ThreadPtr,
                 Executive,
                 KernelMode,
                 FALSE,
                 NULL
                 );

    KeLeaveCriticalRegion();

    UNREFERENCED_PARAMETER(Status);
    ASSERT(NT_SUCCESS(Status));

    ObDereferenceObject(m_ThreadPtr);

    //
    // Now safe to unload the driver or object
    // the thread worker is pointing to
    //

    return TRUE;
}

BOOLEAN
FxSystemThread::ExitThreadAsync(
    __inout FxSystemThread* Reaper
    )
{
    KIRQL irql;

    //
    // Without a top level reaper, the frameworks can not ensure
    // all worker threads have exited at driver unload in the async
    // case. If this is a top level thread, then DriverUnload should
    // call the synchronous ExitThread() method.
    //
    ASSERT(Reaper != NULL);

    Lock(&irql);

    if( !m_Initialized ) {
        Unlock(irql);
        return TRUE;
    }

    if( m_Exit ) {
        ASSERT(FALSE);
        Unlock(irql);
        return TRUE;
    }

    // Tell the system thread to exit
    m_Exit = TRUE;

    // Add a reference which will be released by the reaper
    ADDREF(FxSystemThread::StaticReaperThunk);

    Unlock(irql);

    m_WorkEvent.Set();

    ASSERT(m_Reaper.WorkerRoutine == NULL);

    // We are the only user of this field protected by setting m_Exit
    m_Reaper.WorkerRoutine = FxSystemThread::StaticReaperThunk;
    m_Reaper.Parameter = this;

    Reaper->QueueWorkItem(&m_Reaper);

    //
    // It is not safe to unload the driver until the reaper
    // thread has processed the work item and waited for this
    // thread to exit.
    //

    return TRUE;
}

BOOLEAN
FxSystemThread::QueueWorkItem(
    __inout PWORK_QUEUE_ITEM WorkItem
    )
{
    KIRQL irql;
    BOOLEAN result;

    Lock(&irql);

    if (m_Exit) {
        result = FALSE;
    }
    else {
        result = TRUE;
        InsertTailList(&m_WorkList, &WorkItem->List);

        m_WorkEvent.Set();
    }

    Unlock(irql);

    return result;
}

//
// Attempt to cancel the work item.
//
// Returns TRUE if success.
//
// If returns FALSE, the work item
// routine either has been called, is running,
// or is about to be called.
//
BOOLEAN
FxSystemThread::CancelWorkItem(
    __inout PWORK_QUEUE_ITEM WorkItem
    )
{
    PLIST_ENTRY Entry;
    KIRQL irql;

    Lock(&irql);

    Entry = m_WorkList.Flink;

    while( Entry != &this->m_WorkList ) {

        if( Entry == &WorkItem->List ) {
            RemoveEntryList(&WorkItem->List);
            Unlock(irql);
            return TRUE;
        }

        Entry = Entry->Flink;
    }

    // Not found
    Unlock(irql);

    return FALSE;
}

VOID
FxSystemThread::Thread()
{
    NTSTATUS status;
    LIST_ENTRY head;
    PLIST_ENTRY ple;
    PWORK_QUEUE_ITEM pItem;

    //DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
    //                    "Created, entering work loop");

    //
    // The worker thread will process all list entries before
    // checking for exit. This allows the top level FxDriver
    // thread to process all deferred cleanup work items at
    // driver unload.
    //
    // CancelWorkItem may be used to remove entries
    // before requesting a thread exit if the caller wants to
    // interrupt work queue processing.
    //

    InitializeListHead(&head);

    // Initialize for IsCurrentThread()
    m_PEThread = Mx::GetCurrentEThread();

    // Set the event that the thread now exists
    m_InitEvent.Set();

    for ( ; ; ) {
        KIRQL irql;

        //
        // Lock the list so we are in sync with QueueWorkItem
        //
        Lock(&irql);

        while(!IsListEmpty(&m_WorkList)) {
            //
            // Instead of popping each LIST_ENTRY off of the old list and
            // enqueueing it to the local list head, manipulate the first and
            // last entry in the list to point to our new head.
            //
            head.Flink = m_WorkList.Flink;
            head.Blink = m_WorkList.Blink;

            // First link in the list point backwrad to the new head
            m_WorkList.Flink->Blink = &head;

            // Last link in the list point fwd to the new head
            m_WorkList.Blink->Flink = &head;

            ASSERT(!IsListEmpty(&head));

            //
            // Reinitialize the work list head
            //
            InitializeListHead(&m_WorkList);

            //
            // Process the workitems while unlocked so that the work item can
            // requeue itself if needed and work at passive level.
            //
            Unlock(irql);

            while (!IsListEmpty(&head)) {
                ple = RemoveHeadList(&head);
                pItem = CONTAINING_RECORD(ple, WORK_QUEUE_ITEM, List);
                pItem->WorkerRoutine(pItem->Parameter);

                //
                // NOTE: pItem may have been pool released by the called worker
                //       routine, so it may not be accessed after the callback
                //
            }

            //
            // Relock the list and repeat until the work list is empty
            //
            Lock(&irql);
        }

        // No more items on list, check for exit
        if (m_Exit) {
            //DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            //                    "Terminating");

            Unlock(irql);

            // Release the object reference held by the thread
            RELEASE(FxSystemThread::StaticThreadThunk);

            status = PsTerminateSystemThread(STATUS_SUCCESS);
            UNREFERENCED_PARAMETER(status);
            ASSERT(NT_SUCCESS(status));

            // NOT REACHED
            return;
        }

        //
        // No work to do, clear event under lock to prevent a race with a work
        // enqueue
        //
        m_WorkEvent.Clear();

        Unlock(irql);

        // We are a system thread, so we do not need KeEnterCriticalRegion()
        status = m_WorkEvent.WaitFor(
                    Executive, KernelMode, FALSE, NULL);

        UNREFERENCED_PARAMETER(status);
        ASSERT(NT_SUCCESS(status));
    }

    // NOT REACHED
    return;
}



























VOID
FxSystemThread::Reaper()
{
    NTSTATUS Status;
    KIRQL irql;

    Lock(&irql);

    ASSERT(m_Initialized);
    ASSERT(m_Exit);

    //
    // The thread could still be spinning up, so we must handle this condition.
    //
    if( m_ThreadPtr == NULL ) {

        Unlock(irql);

        KeEnterCriticalRegion();

        // Wait for thread to start
        Status = m_InitEvent.WaitFor(Executive, KernelMode, FALSE, NULL);

        KeLeaveCriticalRegion();

        UNREFERENCED_PARAMETER(Status);
        ASSERT(NT_SUCCESS(Status));

        //
        // Now we have a thread, wait for it to go away
        //
        ASSERT(m_ThreadPtr != NULL);
    }
    else {
        Unlock(irql);
    }

    KeEnterCriticalRegion();

    // Wait for thread to exit
    Status = KeWaitForSingleObject(
                 m_ThreadPtr,
                 Executive,
                 KernelMode,
                 FALSE,
                 NULL
                 );

    KeLeaveCriticalRegion();

    UNREFERENCED_PARAMETER(Status);
    ASSERT(NT_SUCCESS(Status));

    ObDereferenceObject(m_ThreadPtr);

    RELEASE(FxSystemThread::StaticReaperThunk);

    return;
}

VOID
FxSystemThread::StaticThreadThunk(
    __inout PVOID Context
    )
{
    FxSystemThread* thread = (FxSystemThread*)Context;

    thread->Thread();
}















VOID
FxSystemThread::StaticReaperThunk(
    __inout PVOID Context
    )
{
    FxSystemThread* thread = (FxSystemThread*)Context;

    thread->Reaper();
}


