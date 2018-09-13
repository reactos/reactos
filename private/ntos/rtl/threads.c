/*++

Copyright (c) 1989-1998 Microsoft Corporation

Module Name:

    threads.c

Abstract:

    This module defines functions for thread pools. Thread pools can be used for
    one time execution of tasks, for waits and for one shot or periodic timers.

Author:

    Gurdeep Singh Pall (gurdeep) Nov 13, 1997

Environment:

    These routines are statically linked in the caller's executable and
    are callable in only from user mode. They make use of Nt system services.


Revision History:

    Aug-19 lokeshs - modifications to thread pool apis.

--*/


// There are 3 types of thread pool functions supported
//
// 1. Wait Thread Pool
// 2. Worker Thread Pool
// 3. Timer Thread Pool
//
// Wait Thread Pool
// ----------------
// Clients can submit a waitable object with an optional timeout to wait on. 
// One thread is created per 63 of such waitable objects. These threads are 
// never killed.
//
// Worker Thread Pool
// ------------------
// Clients can submit functions to be executed by a worker thread. Threads are 
// created if the work queue exceeds a threshold. Clients can request that the 
// function be invoked in the context of a I/O thread. I/O worker threads
// can be used for initiating asynchronous I/O requests. They are not terminated if
// there are pending IO requests. Worker threads terminate if inactivity exceeds a 
// threshold.
// Clients can also associate IO completion requests with the IO completion port
// waited upon by the non I/O worker threads. One should not post overlapped IO requests
// in worker threads.
//
// Timer Thread Pool
// -----------------
// Clients create one or more Timer Queues and insert one shot or periodic 
// timers in them. All timers in a queue are kept in a "Delta List" with each 
// timer's firing time relative to the timer before it. All Queues are also 
// kept in a "Delta List" with each Queue's firing time (set to the firing time 
// of the nearest firing timer) relative to the Queue before it. One NT Timer 
// is used to service all timers in all queues.


#include <ntos.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "ntrtlp.h"
#include "threads.h"


ULONG DPRN = 0x0000000;



NTSTATUS
RtlRegisterWait (
    OUT PHANDLE WaitHandle,
    IN  HANDLE  Handle,
    IN  WAITORTIMERCALLBACKFUNC Function,
    IN  PVOID Context,
    IN  ULONG  Milliseconds,
    IN  ULONG  Flags
    )

/*++

Routine Description:

    This routine adds a new wait request to the pool of objects being waited on.

Arguments:

    WaitHandle - Handle returned on successful completion of this routine.

    Handle - Handle to the object to be waited on

    Function - Routine that is called when the wait completes or a timeout occurs

    Context - Opaque pointer passed in as an argument to Function

    Milliseconds - Timeout for the wait in milliseconds. 0xffffffff means dont 
            timeout.

    Flags - Can be one of:

        WT_EXECUTEINWAITTHREAD - if WorkerProc should be invoked in the wait
                thread itself. This should only be used for small routines.
        WT_EXECUTEINIOTHREAD - use only if the WorkerProc should be invoked in
                an IO Worker thread. Avoid using it.

    If Flags is not WT_EXECUTEINWAITTHREAD, the following flag can also be set:
    
        WT_EXECUTELONGFUNCTION - indicates that the callback might be blocked
                for a long duration. Use only if the callback is being queued to a
                worker thread.
    
Return Value:

    NTSTATUS - Result code from call.  The following are returned

    STATUS_SUCCESS - The registration was successful.

    STATUS_NO_MEMORY - There was not sufficient heap to perform the requested 
                operation.
                
    or other NTSTATUS error code

--*/

{
    PRTLP_WAIT Wait ;
    NTSTATUS Status ;
    PRTLP_EVENT Event ;
    LARGE_INTEGER TimeOut ;
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB = NULL;

    *WaitHandle = NULL ;

    
    // Initialize thread pool if it isnt already done

    if ( CompletedWaitInitialization != 1) {

        Status = RtlpInitializeWaitThreadPool () ;

        if (! NT_SUCCESS( Status ) )
            return Status ;
    }


    // Initialize Wait request

    Wait = (PRTLP_WAIT) RtlpAllocateTPHeap ( sizeof (RTLP_WAIT),
                                            HEAP_ZERO_MEMORY) ;

    if (!Wait) {
        return STATUS_NO_MEMORY ;
    }
    
    Wait->WaitHandle = Handle ;
    Wait->Flags = Flags ;
    Wait->Function = Function ;
    Wait->Context = Context ;
    Wait->Timeout = Milliseconds ;
    SET_SIGNATURE(Wait) ;
        
    
    // timer part of wait is initialized by wait thread in RtlpAddWait

    
    // Get a wait thread that can accomodate another wait request.
    
    Status = RtlpFindWaitThread (&ThreadCB) ;

    if (Status != STATUS_SUCCESS) {
    
        RtlpFreeTPHeap( Wait ) ;
        
        return Status ;
    }

    Wait->ThreadCB = ThreadCB ;

    #if DBG1
    Wait->DbgId = ++NextWaitDbgId ;
    Wait->ThreadId = HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread) ;
    if (DPRN0)
    DbgPrint("<%d:%d> Wait %x created by thread:<%x:%x>\n\n", 
                Wait->DbgId, 1, (ULONG_PTR)Wait,
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)) ;
    #endif

    // Set the wait handle

    *WaitHandle = Wait ;


    // Queue an APC to the Wait Thread

    Status = NtQueueApcThread(
                    ThreadCB->ThreadHandle,
                    (PPS_APC_ROUTINE)RtlpAddWait,
                    (PVOID)Wait,
                    NULL,
                    NULL
                    );


    if ( NT_SUCCESS(Status) ) {

        Status = STATUS_SUCCESS ;

    } else {

        *WaitHandle = NULL ;
        RtlpFreeTPHeap( Wait ) ;
    }

    return Status ;

}



NTSTATUS
RtlDeregisterWait(
    IN HANDLE WaitHandle
    )
/*++

Routine Description:

    This routine removes the specified wait from the pool of objects being
    waited on. This routine is non-blocking. Once this call returns, no new
    Callbacks are invoked. However, Callbacks that might already have been queued
    to worker threads are not cancelled.

Arguments:

    WaitHandle - Handle indentifying the wait.

Return Value:

    STATUS_SUCCESS - The deregistration was successful.
    STATUS_PENDING - Some callbacks associated with this Wait, are still executing.
--*/

{
    return RtlDeregisterWaitEx( WaitHandle, NULL ) ;    
}


NTSTATUS
RtlDeregisterWaitEx(
    IN HANDLE WaitHandle,
    IN HANDLE Event
    )
/*++

Routine Description:

    This routine removes the specified wait from the pool of objects being
    waited on. Once this call returns, no new Callbacks will be invoked.
    Depending on the value of Event, the call can be blocking or non-blocking.
    Blocking calls MUST NOT be invoked inside the callback routines, except
    when a callback being executed in the Wait thread context deregisters
    its associated Wait (in this case there is no reason for making blocking calls),
    or when a callback queued to a worker thread is deregistering some other wait item
    (be careful of deadlocks here).

Arguments:

    WaitHandle - Handle indentifying the wait.

    Event - Event to wait upon.
            (HANDLE)-1: The function creates an event and waits on it.
            Event : The caller passes an Event. The function removes the wait handle,
                    but does not wait for all callbacks to complete. The Event is 
                    released after all callbacks have completed.
            NULL : The function is non-blocking. The function removes the wait handle,
                    but does not wait for all callbacks to complete.
            
Return Value:

    STATUS_SUCCESS - The deregistration was successful.
    STATUS_PENDING - Some callback is still pending.

--*/

{
    NTSTATUS Status, StatusAsync = STATUS_SUCCESS ;
    PRTLP_WAIT Wait = (PRTLP_WAIT) WaitHandle ;
    ULONG CurrentThreadId =  HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread) ;
    PRTLP_EVENT CompletionEvent = NULL ;
    HANDLE ThreadHandle ;
    ULONG NonBlocking = ( Event != (HANDLE) -1 ) ; //The call returns non-blocking


    if (!Wait) {
        ASSERT( FALSE ) ;
        return STATUS_INVALID_PARAMETER ;
    }
    
    ThreadHandle = Wait->ThreadCB->ThreadHandle ;

    
    CHECK_DEL_SIGNATURE( Wait ) ;
    SET_DEL_SIGNATURE( Wait ) ;
    
    #if DBG1
    Wait->ThreadId2 = CurrentThreadId ;
    #endif
    
    if (Event == (HANDLE)-1) {

        // Get an event from the event cache

        CompletionEvent = RtlpGetWaitEvent () ;

        if (!CompletionEvent) {

            return STATUS_NO_MEMORY ;

        }
    }

    
    Wait = (PRTLP_WAIT) WaitHandle ;

    #if DBG1
    if (DPRN0)
    DbgPrint("<%d:%d> Wait %x deregistering by thread:<%x:%x>\n\n", Wait->DbgId, 
                Wait->RefCount, (ULONG_PTR)Wait,
                CurrentThreadId,
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)) ;
    #endif


    Wait->CompletionEvent = CompletionEvent
                            ? CompletionEvent->Handle
                            : Event ;

    //
    // RtlDeregisterWaitEx is being called from within the Wait thread callback
    //
    
    if ( CurrentThreadId == Wait->ThreadCB->ThreadId ) {

        Status = RtlpDeregisterWait ( Wait, NULL, NULL ) ;


        // all callback functions run in the wait thread. So cannot return PENDING
        
        ASSERT ( Status != STATUS_PENDING ) ;
          
    
    } else {

        PRTLP_EVENT PartialCompletionEvent = NULL ;

        if (NonBlocking) {
        
            PartialCompletionEvent = RtlpGetWaitEvent () ;

            if (!PartialCompletionEvent) {

                return STATUS_NO_MEMORY ;
            }
        }
        
        // Queue an APC to the Wait Thread
        
        Status = NtQueueApcThread(
                        Wait->ThreadCB->ThreadHandle,
                        (PPS_APC_ROUTINE)RtlpDeregisterWait,
                        (PVOID) Wait,
                        NonBlocking ? PartialCompletionEvent->Handle : NULL ,
                        NonBlocking ? (PVOID)&StatusAsync : NULL
                        );
                        
        if (! NT_SUCCESS(Status)) {

            if (CompletionEvent) RtlpFreeWaitEvent( CompletionEvent ) ;
            if (PartialCompletionEvent) RtlpFreeWaitEvent( PartialCompletionEvent ) ;
    
            return Status ;
        }


        // block till the wait entry has been deactivated
        
        if (NonBlocking) {
        
            Status = RtlpWaitForEvent( PartialCompletionEvent->Handle, ThreadHandle ) ;
        }    


        if (PartialCompletionEvent) RtlpFreeWaitEvent( PartialCompletionEvent ) ;

    }

    if ( CompletionEvent ) {

        // wait for Event to be fired. Return if the thread has been killed.

        #if DBG1
        if (DPRN0)
        DbgPrint("Wait %x deregister waiting ThreadId<%x:%x>\n\n", 
                (ULONG_PTR)Wait,
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)) ;
        #endif
        
        Status = RtlpWaitForEvent( CompletionEvent->Handle, ThreadHandle ) ;

        #if DBG1
        if (DPRN0)
        DbgPrint("Wait %x deregister completed\n\n", (ULONG_PTR)Wait) ;
        #endif

        RtlpFreeWaitEvent( CompletionEvent ) ;

        return NT_SUCCESS( Status ) ? STATUS_SUCCESS : Status ;
        
    } else {

        return StatusAsync ;
    }
}




NTSTATUS
RtlQueueWorkItem(
    IN  WORKERCALLBACKFUNC Function,
    IN  PVOID Context,
    IN  ULONG  Flags
    )

/*++

Routine Description:

    This routine queues up the request to be executed in a worker thread.

Arguments:

    Function - Routine that is called by the worker thread

    Context - Opaque pointer passed in as an argument to WorkerProc

    Flags - Can be:

            WT_EXECUTEINIOTHREAD - Specifies that the WorkerProc should be invoked
            by a thread that is never destroyed when there are pending IO requests.
            This can be used by threads that invoke I/O and/or schedule APCs.

            The below flag can also be set:
            WT_EXECUTELONGFUNCTION - Specifies that the function might block for a
            long duration.

Return Value:

    STATUS_SUCCESS - Queued successfully.

    STATUS_NO_MEMORY - There was not sufficient heap to perform the
        requested operation.

--*/

{
    ULONG Threshold ;
    ULONG CurrentTickCount ;
    NTSTATUS Status = STATUS_SUCCESS ;

    
    // Make sure the worker thread pool is initialized

    if (CompletedWorkerInitialization != 1) {

        Status = RtlpInitializeWorkerThreadPool () ;
        
        if (! NT_SUCCESS(Status) )
            return Status ;
    }

       
    // Take lock for the global worker thread pool

    RtlEnterCriticalSection (&WorkerCriticalSection) ;


    if (Flags & (WT_EXECUTEINIOTHREAD |WT_EXECUTEINUITHREAD |WT_EXECUTEINPERSISTENTIOTHREAD) ){

        //
        // execute in IO Worker thread
        //

        ULONG NumEffIOWorkerThreads = NumIOWorkerThreads - NumLongIOWorkRequests ;
        ULONG ThreadCreationDampingTime = NumIOWorkerThreads < NEW_THREAD_THRESHOLD
                                            ? THREAD_CREATION_DAMPING_TIME1
                                            : THREAD_CREATION_DAMPING_TIME2 ;
                                            
        if (PersistentIOTCB && (Flags&WT_EXECUTELONGFUNCTION))
            NumEffIOWorkerThreads -- ;

        
        // Check if we need to grow I/O worker thread pool

        Threshold = (NumEffIOWorkerThreads < MAX_WORKER_THREADS 
                    ? NEW_THREAD_THRESHOLD * NumEffIOWorkerThreads 
                    : 0xffffffff) ;

        if (LastThreadCreationTickCount > NtGetTickCount())
            LastThreadCreationTickCount = NtGetTickCount() ;

        if (NumEffIOWorkerThreads == 0
            || ((NumIOWorkRequests - NumLongIOWorkRequests > Threshold)
                    && (LastThreadCreationTickCount + ThreadCreationDampingTime 
                        < NtGetTickCount()))) {

            // Grow the IO worker thread pool

            Status = RtlpStartIOWorkerThread () ;

        }

        if (Status == STATUS_SUCCESS) {

            // Queue the work request

            Status = RtlpQueueIOWorkerRequest (Function, Context, Flags) ;
        }


    } else {

        //
        // execute in regular worker thread
        //

        ULONG NumEffWorkerThreads = (NumWorkerThreads - NumLongWorkRequests) ;
        ULONG ThreadCreationDampingTime = NumWorkerThreads < NEW_THREAD_THRESHOLD
                                            ? THREAD_CREATION_DAMPING_TIME1
                                            : (NumWorkerThreads < 50
                                                ? THREAD_CREATION_DAMPING_TIME2
                                                : NumWorkerThreads << 7); // *100ms

        // if io completion set, then have 1 more thread
        
        if (NumMinWorkerThreads && NumEffWorkerThreads)
            NumEffWorkerThreads -- ;


        // Check if we need to grow worker thread pool

        Threshold = (NumWorkerThreads < MAX_WORKER_THREADS 
                    ? (NumEffWorkerThreads < 7 
                        ? NumEffWorkerThreads*NumEffWorkerThreads
                        : NEW_THREAD_THRESHOLD * NumEffWorkerThreads )
                    : 0xffffffff) ;

        if (LastThreadCreationTickCount > NtGetTickCount())
            LastThreadCreationTickCount = NtGetTickCount() ;

        if (NumEffWorkerThreads == 0 ||
            ( (NumWorkRequests - NumLongWorkRequests >= Threshold)
                    && (LastThreadCreationTickCount + ThreadCreationDampingTime 
                            < NtGetTickCount()))) 
        {

            // Grow the worker thread pool

            Status = RtlpStartWorkerThread () ;

        } 

        // Queue the work request

        if (Status == STATUS_SUCCESS) {

            Status = RtlpQueueWorkerRequest (Function, Context, Flags) ;
        }

    }

    // Release lock on the worker thread pool

    RtlLeaveCriticalSection (&WorkerCriticalSection) ;

    return Status ;
}



NTSTATUS
RtlSetIoCompletionCallback (
    IN  HANDLE  FileHandle,
    IN  APC_CALLBACK_FUNCTION  CompletionProc,
    IN  ULONG Flags
    )

/*++

Routine Description:

    This routine binds an Handle and an associated callback function to the
    IoCompletionPort which queues work items to worker threads.

Arguments:

    Handle - handle to be bound to the IO completion port
    
    CompletionProc - callback function to be executed when an IO request
        pending on the IO handle completes.

    Flags - Reserved. pass 0.

--*/

{
    IO_STATUS_BLOCK IoSb ;
    FILE_COMPLETION_INFORMATION CompletionInfo ;
    NTSTATUS Status;
    

    // Make sure that the worker thread pool is initialized as the file handle
    // is bound to IO completion port.

    if (CompletedWorkerInitialization != 1) {

        Status = RtlpInitializeWorkerThreadPool () ;
        
        if (! NT_SUCCESS(Status) )
            return Status ;

    }


    //
    // from now on NumMinWorkerThreads should be 1. If there is only 1 worker thread
    // create a new one.
    //
    
    if ( NumMinWorkerThreads == 0 ) {
    
        // Take lock for the global worker thread pool

        RtlEnterCriticalSection (&WorkerCriticalSection) ;

        if (NumWorkerThreads == 0) {

            Status = RtlpStartWorkerThread () ;

            if ( ! NT_SUCCESS(Status) ) {
            
                RtlLeaveCriticalSection (&WorkerCriticalSection) ;
                return Status ;
            }
        }

        // from now on, there will be at least 1 worker thread
        NumMinWorkerThreads = 1 ;
        
        RtlLeaveCriticalSection (&WorkerCriticalSection) ;

    }


    // bind to IoCompletionPort, which queues work items to worker threads

    CompletionInfo.Port = WorkerCompletionPort ;
    CompletionInfo.Key = (PVOID) CompletionProc ;

    Status = NtSetInformationFile (
                        FileHandle,
                        &IoSb, //not initialized
                        &CompletionInfo,
                        sizeof(CompletionInfo),
                        FileCompletionInformation //enum flag
                        ) ;
    return Status ;
}



NTSTATUS
RtlCreateTimerQueue(
    OUT PHANDLE TimerQueueHandle
    )

/*++

Routine Description:

    This routine creates a queue that can be used to queue time based tasks.

Arguments:

    TimerQueueHandle - Returns back the Handle identifying the timer queue created.

Return Value:

    NTSTATUS - Result code from call.  The following are returned

        STATUS_SUCCESS - Timer Queue created successfully.

        STATUS_NO_MEMORY - There was not sufficient heap to perform the
            requested operation.

--*/

{
    PRTLP_TIMER_QUEUE Queue ;
    NTSTATUS Status;


    // Initialize the timer component if it hasnt been done already

    if (CompletedTimerInitialization != 1) {

        Status = RtlpInitializeTimerThreadPool () ;

        if ( !NT_SUCCESS(Status) )
            return Status ;

    }


    InterlockedIncrement( &NumTimerQueues ) ;

    
    // Allocate a Queue structure

    Queue = (PRTLP_TIMER_QUEUE) RtlpAllocateTPHeap (
                                      sizeof (RTLP_TIMER_QUEUE),
                                      HEAP_ZERO_MEMORY
                                      ) ;

    if (Queue == NULL) {

        InterlockedDecrement( &NumTimerQueues ) ;
        
        return STATUS_NO_MEMORY ;
    }

    Queue->RefCount = 1 ;

    
    // Initialize the allocated queue

    InitializeListHead (&Queue->List) ;
    InitializeListHead (&Queue->TimerList) ;
    InitializeListHead (&Queue->UncancelledTimerList) ;
    SET_SIGNATURE( Queue ) ;

    Queue->DeltaFiringTime = 0 ;

    #if DBG1
    Queue->DbgId = ++NextTimerDbgId ;
    Queue->ThreadId = HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread) ;
    if (DPRN0)
    DbgPrint("<%d:%d> TimerQueue %x created by thread:<%x:%x>\n\n", 
                Queue->DbgId, 1, (ULONG_PTR)Queue,
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)) ;
    #endif
    
    *TimerQueueHandle = Queue ;
    
    return STATUS_SUCCESS ;
}


NTSTATUS
RtlDeleteTimerQueue(
    IN HANDLE TimerQueueHandle
    )

/*++

Routine Description:

    This routine deletes a previously created queue. This call is non-blocking and 
    can be made from Callbacks. Pending callbacks already queued to worker threads
    are not cancelled.

Arguments:

    TimerQueueHandle - Handle identifying the timer queue created.

Return Value:

    NTSTATUS - Result code from call.

        STATUS_PENDING - Timer Queue created successfully.
        
--*/

{
    return RtlDeleteTimerQueueEx( TimerQueueHandle, NULL ) ;
}


NTSTATUS
RtlDeleteTimerQueueEx (
    HANDLE QueueHandle,
    HANDLE Event
    )
/*++

Routine Description:

    This routine deletes the queue specified in the Request and frees all timers.
    This call is blocking or non-blocking depending on the value passed for Event.
    Blocking calls cannot be made from ANY Timer callbacks. After this call returns,
    no new Callbacks will be fired for any timer associated with the queue.

Arguments:

    QueueHandle - queue to delete

    Event - Event to wait upon.
            (HANDLE)-1: The function creates an event and waits on it.
            Event : The caller passes an event. The function marks the queue for deletion,
                    but does not wait for all callbacks to complete. The event is 
                    signalled after all callbacks have completed.
            NULL : The function is non-blocking. The function marks the queue for deletion,
                    but does not wait for all callbacks to complete.
            
Return Value:

    STATUS_SUCCESS - All timer callbacks have completed.
    STATUS_PENDING - Non-Blocking call. Some timer callbacks associated with timers
                    in this queue may not have completed.
    
--*/
{
    NTSTATUS Status;
    LARGE_INTEGER TimeOut ;
    PRTLP_EVENT CompletionEvent = NULL ;
    PRTLP_TIMER_QUEUE Queue = (PRTLP_TIMER_QUEUE)QueueHandle ;

    if (!Queue) {
        ASSERT( FALSE ) ;
        return STATUS_INVALID_PARAMETER ;
    }

    CHECK_DEL_SIGNATURE( Queue ) ;
    SET_DEL_SIGNATURE( Queue ) ;

    
    #if DBG1
    Queue->ThreadId2 = HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread) ;
    if (DPRN0)
    DbgPrint("\n<%d:%d> Queue Delete(Queue:%x Event:%x by Thread:<%x:%x>)\n\n", 
             Queue->DbgId, Queue->RefCount, (ULONG_PTR)Queue, (ULONG_PTR)Event,
             HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
             HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)) ;
    #endif


    if (Event == (HANDLE)-1 ) {

        // Get an event from the event cache

        CompletionEvent = RtlpGetWaitEvent () ;

        if (!CompletionEvent) {

            return STATUS_NO_MEMORY ;

        }
    }
    
    Queue->CompletionEvent = CompletionEvent
                             ? CompletionEvent->Handle 
                             : Event ;


    // once this flag is set, no timer will be fired
    
    ACQUIRE_GLOBAL_TIMER_LOCK();
    Queue->State |= STATE_DONTFIRE;
    RELEASE_GLOBAL_TIMER_LOCK();



    // queue an APC
    
    Status = NtQueueApcThread(
                    TimerThreadHandle,
                    (PPS_APC_ROUTINE)RtlpDeleteTimerQueue,
                    (PVOID) QueueHandle,
                    NULL,
                    NULL
                    );

    if (! NT_SUCCESS(Status)) {

        RtlpFreeWaitEvent( CompletionEvent ) ;

        return Status ;
    }
    
    if (CompletionEvent) {

        // wait for Event to be fired. Return if the thread has been killed.


        #if DBG1
        if (DPRN0)
        DbgPrint("<%x> Queue delete waiting Thread<%d:%d>\n\n",
                (ULONG_PTR)Queue,
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)) ;
        #endif


        Status = RtlpWaitForEvent( CompletionEvent->Handle, TimerThreadHandle ) ;


        #if DBG1
        if (DPRN0)
        DbgPrint("<%x> Queue delete completed\n\n", (ULONG_PTR) Queue) ;
        #endif

        RtlpFreeWaitEvent( CompletionEvent ) ;

        return NT_SUCCESS( Status ) ? STATUS_SUCCESS : Status ;

    } else {

        return STATUS_PENDING ;
    }
}



NTSTATUS
RtlCreateTimer(
    IN  HANDLE TimerQueueHandle,
    OUT HANDLE *Handle,
    IN  WAITORTIMERCALLBACKFUNC Function,
    IN  PVOID Context,
    IN  ULONG  DueTime,
    IN  ULONG  Period,
    IN  ULONG  Flags
    )
/*++

Routine Description:

    This routine puts a timer request in the queue identified in by TimerQueueHandle.
    The timer request can be one shot or periodic.

Arguments:

    TimerQueueHandle - Handle identifying the timer queue in which to insert the timer
                    request.

    Handle - Specifies a location to return a handle to this timer request

    Function - Routine that is called when the timer fires

    Context - Opaque pointer passed in as an argument to WorkerProc

    DueTime - Specifies the time in milliseconds after which the timer fires.

    Period - Specifies the period of the timer in milliseconds. This should be 0 for
    one shot requests.

    Flags - Can be one of:

            WT_EXECUTEINTIMERTHREAD - if WorkerProc should be invoked in the wait thread
            it this should only be used for small routines.

            WT_EXECUTELONGFUNCTION - if WorkerProc can possibly block for a long time.

            WT_EXECUTEINIOTHREAD - if WorkerProc should be invoked in IO worker thread
            
Return Value:

    NTSTATUS - Result code from call.  The following are returned

        STATUS_SUCCESS - Timer Queue created successfully.

        STATUS_NO_MEMORY - There was not sufficient heap to perform the
            requested operation.

--*/

{
    NTSTATUS Status;
    PRTLP_TIMER Timer ;

    Timer = (PRTLP_TIMER) RtlpAllocateTPHeap (
                                sizeof (RTLP_TIMER),
                                HEAP_ZERO_MEMORY
                                ) ;

    if (Timer == NULL) {

        return STATUS_NO_MEMORY ;

    }

    // Initialize the allocated timer

    Timer->DeltaFiringTime = DueTime ;
    Timer->Queue = (PRTLP_TIMER_QUEUE) TimerQueueHandle ;
    Timer->RefCount = 1 ;
    Timer->Flags = Flags ;
    Timer->Function = Function ;
    Timer->Context = Context ;
    //todo:remove below
    Timer->Period = (Period == -1) ? 0 : Period;
    InitializeListHead( &Timer->TimersToFireList ) ;
    SET_SIGNATURE( Timer ) ;
    
    
    #if DBG1
    Timer->DbgId = ++ Timer->Queue->NextDbgId ;
    Timer->ThreadId = HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread) ;
    if (DPRN1)
    DbgPrint("\n<%d:%d:%d> Timer: created by Thread:<%x:%x>\n\n", 
            Timer->Queue->DbgId, Timer->DbgId, 1,
            HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
            HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)) ;
    #endif

    *Handle = Timer ;


    // Increment the total number of timers in the queue

    InterlockedIncrement( &((PRTLP_TIMER_QUEUE)TimerQueueHandle)->RefCount ) ;


    // Queue APC to timer thread

    Status = NtQueueApcThread(
                    TimerThreadHandle,
                    (PPS_APC_ROUTINE)RtlpAddTimer,
                    (PVOID)Timer,
                    NULL,
                    NULL
                    ) ;

    return Status ;
}


NTSTATUS
RtlUpdateTimer(
    IN HANDLE TimerQueueHandle,
    IN HANDLE Timer,
    IN ULONG  DueTime,
    IN ULONG  Period
    )
/*++

Routine Description:

    This routine updates the timer

Arguments:

    TimerQueueHandle - Handle identifying the queue in which the timer to be updated exists

    Timer - Specifies a handle to the timer which needs to be updated

    DueTime - Specifies the time in milliseconds after which the timer fires.

    Period - Specifies the period of the timer in milliseconds. This should be 
            0 for one shot requests.

Return Value:

    NTSTATUS - Result code from call.  The following are returned

        STATUS_SUCCESS - Timer updated successfully.

--*/
{
    NTSTATUS Status;
    PRTLP_TIMER TmpTimer ;

    if (!TimerQueueHandle || !Timer) {
        ASSERT( FALSE ) ;
        return STATUS_INVALID_PARAMETER ;
    }

    CHECK_DEL_SIGNATURE( (PRTLP_TIMER)Timer ) ;

    
    TmpTimer = (PRTLP_TIMER) RtlpAllocateTPHeap (
                                        sizeof (RTLP_TIMER),
                                        0
                                        ) ;

    if (TmpTimer == NULL) {

        return STATUS_NO_MEMORY ;
    }

    TmpTimer->DeltaFiringTime = DueTime;
    //todo:remove below
    if (Period==-1) Period = 0;
    TmpTimer->Period = Period ;

    #if DBG1
    ((PRTLP_TIMER)Timer)->ThreadId2 = 
                    HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread) ;
    #endif
    #if DBG1
    if (DPRN1)
    DbgPrint("<%d:%d:%d> Timer: updated by Thread:<%x:%x>\n\n", 
                ((PRTLP_TIMER)Timer)->Queue->DbgId, 
                ((PRTLP_TIMER)Timer)->DbgId, ((PRTLP_TIMER)Timer)->RefCount,
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)) ;
    #endif


    // queue APC to update timer
    
    Status = NtQueueApcThread (
                    TimerThreadHandle,
                    (PPS_APC_ROUTINE)RtlpUpdateTimer,
                    (PVOID)Timer, //Actual timer
                    (PVOID)TmpTimer,
                    NULL
                    );


    return Status ;
}


NTSTATUS
RtlDeleteTimer (
    IN HANDLE TimerQueueHandle,
    IN HANDLE TimerToCancel,
    IN HANDLE Event
    )
/*++

Routine Description:

    This routine cancels the timer

Arguments:

    TimerQueueHandle - Handle identifying the queue from which to delete timer

    TimerToCancel - Handle identifying the timer to cancel

    Event - Event to be signalled when the timer is deleted
            (HANDLE)-1: The function creates an event and waits on it.
            Event : The caller passes an event. The function marks the timer for deletion,
                    but does not wait for all callbacks to complete. The event is 
                    signalled after all callbacks have completed.
            NULL : The function is non-blocking. The function marks the timer for deletion,
                    but does not wait for all callbacks to complete.

Return Value:

    NTSTATUS - Result code from call.  The following are returned

        STATUS_SUCCESS - Timer cancelled. No pending callbacks.
        STATUS_PENDING - Timer cancelled. Some callbacks still not completed.

--*/
{
    NTSTATUS Status;
    PRTLP_EVENT CompletionEvent = NULL ;
    PRTLP_TIMER Timer = (PRTLP_TIMER) TimerToCancel ;
    ULONG TimerRefCount ;
    #if DBG1
    ULONG QueueDbgId ;
    #endif


    if (!TimerQueueHandle || !TimerToCancel) {
        ASSERT( FALSE ) ;
        return STATUS_INVALID_PARAMETER ;
    }

    #if DBG1
    QueueDbgId = Timer->Queue->DbgId ;
    #endif


    CHECK_DEL_SIGNATURE( Timer ) ;
    SET_DEL_SIGNATURE( Timer ) ;
    CHECK_DEL_SIGNATURE( (PRTLP_TIMER_QUEUE)TimerQueueHandle ) ;

    
    if (Event == (HANDLE)-1 ) {

        // Get an event from the event cache

        CompletionEvent = RtlpGetWaitEvent () ;

        if (!CompletionEvent) {

            return STATUS_NO_MEMORY ;
        }
    }

    #if DBG1
    Timer->ThreadId2 = HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread) ;
    #endif
    #if DBG1
    if (DPRN0)
    DbgPrint("\n<%d:%d:%d> Timer: Cancel:(Timer:%x, Event:%x)\n\n", 
                Timer->Queue->DbgId, Timer->DbgId, Timer->RefCount, 
                (ULONG_PTR)Timer, (ULONG_PTR)Event) ;
    #endif

    Timer->CompletionEvent = CompletionEvent
                            ? CompletionEvent->Handle 
                            : Event ;


    ACQUIRE_GLOBAL_TIMER_LOCK();
    Timer->State |= STATE_DONTFIRE ;
    TimerRefCount = Timer->RefCount ;
    RELEASE_GLOBAL_TIMER_LOCK();

    
    Status = NtQueueApcThread(
                TimerThreadHandle,
                (PPS_APC_ROUTINE)RtlpCancelTimer,
                (PVOID)TimerToCancel,
                NULL,
                NULL
                );

    if (! NT_SUCCESS(Status)) {

        RtlpFreeWaitEvent( CompletionEvent ) ;

        return Status ;
    }


    
    if ( CompletionEvent ) {

        // wait for the event to be signalled 

        #if DBG1
        if (DPRN0)
        DbgPrint("<%d> Timer: %x: Cancel waiting Thread<%d:%d>\n\n", 
                QueueDbgId, (ULONG_PTR)Timer,
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)) ;
        #endif

        
        Status = RtlpWaitForEvent( CompletionEvent->Handle,  TimerThreadHandle ) ;

        
        #if DBG1
        if (DPRN0)
        DbgPrint("<%d> Timer: %x: Cancel waiting done\n\n", QueueDbgId, 
                (ULONG_PTR)Timer) ;
        #endif


        RtlpFreeWaitEvent( CompletionEvent ) ;

        return NT_SUCCESS(Status) ? STATUS_SUCCESS : Status ;

    } else {

        return (TimerRefCount > 1) ? STATUS_PENDING : STATUS_SUCCESS;
    }
}




NTSTATUS
NTAPI
RtlSetThreadPoolStartFunc(
    PRTLP_START_THREAD StartFunc,
    PRTLP_EXIT_THREAD ExitFunc
    )
/*++

Routine Description:

    This routine sets the thread pool's thread creation function.  This is not
    thread safe, because it is intended solely for kernel32 to call for processes
    that aren't csrss/smss.

Arguments:

    StartFunc - Function to create a new thread

Return Value:

--*/

{
    RtlpStartThreadFunc = StartFunc ;
    RtlpExitThreadFunc = ExitFunc ;
    return STATUS_SUCCESS ;
}



NTSTATUS
RtlThreadPoolCleanup (
    ULONG Flags
    )
/*++

Routine Description:
    This routine cleans up the thread pool.

Arguments:

    None
    
Return Value:

    STATUS_SUCCESS : if none of the components are in use.
    STATUS_UNSUCCESSFUL : if some components are still in use.

--*/
{
    BOOLEAN Cleanup ;
    PLIST_ENTRY Node ;
    ULONG i ;
    HANDLE TmpHandle ;
    
    return STATUS_UNSUCCESSFUL;
    
    // cleanup timer thread
    
    IS_COMPONENT_INITIALIZED(StartedTimerInitialization, 
                            CompletedTimerInitialization,
                            Cleanup ) ;

    if ( Cleanup ) {

        ACQUIRE_GLOBAL_TIMER_LOCK() ;
        
        if (NumTimerQueues != 0 ) {
        
            ASSERTMSG( FALSE,
                "Trying to deinitialize ThreadPool when timers exist\n" ) ;
            RELEASE_GLOBAL_TIMER_LOCK() ;

            return STATUS_UNSUCCESSFUL ;
        }
        
        NtQueueApcThread(
                TimerThreadHandle,
                (PPS_APC_ROUTINE)RtlpThreadCleanup,
                NULL,
                NULL,
                NULL
                );

        NtClose( TimerThreadHandle ) ;
        TimerThreadHandle = NULL ;

        RELEASE_GLOBAL_TIMER_LOCK() ;

    }


    //
    // cleanup wait threads
    //

    IS_COMPONENT_INITIALIZED(StartedWaitInitialization, 
                            CompletedWaitInitialization,
                            Cleanup ) ;

    if ( Cleanup ) {

        PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB ;

        ACQUIRE_GLOBAL_WAIT_LOCK() ;

        // Queue an APC to all Wait Threads
        
        for (Node = WaitThreads.Flink ; Node != &WaitThreads ; 
                Node = Node->Flink) 
        {

            ThreadCB = CONTAINING_RECORD(Node, 
                                RTLP_WAIT_THREAD_CONTROL_BLOCK,
                                WaitThreadsList) ;

            if ( ThreadCB->NumWaits != 0 ) {

                ASSERTMSG( FALSE,
                    "Cannot cleanup ThreadPool. Registered Wait events exist." ) ;
                RELEASE_GLOBAL_WAIT_LOCK( ) ;
                
                return STATUS_UNSUCCESSFUL ;
            }

            RemoveEntryList( &ThreadCB->WaitThreadsList ) ;
            TmpHandle = ThreadCB->ThreadHandle ;
            
            NtQueueApcThread(
                    ThreadCB->ThreadHandle,
                    (PPS_APC_ROUTINE)RtlpThreadCleanup,
                    NULL,
                    NULL,
                    NULL
                    );

            NtClose( TmpHandle ) ;
        }

        RELEASE_GLOBAL_WAIT_LOCK( ) ;

    }


    // cleanup worker threads

    IS_COMPONENT_INITIALIZED( StartedWorkerInitialization, 
                            CompletedWorkerInitialization,
                            Cleanup ) ;
                                
    if ( Cleanup ) {

        RtlEnterCriticalSection (&WorkerCriticalSection) ;

        if ( (NumWorkRequests != 0) || (NumIOWorkRequests != 0) ) {

            ASSERTMSG( FALSE,
                "Cannot cleanup ThreadPool. Work requests pending." ) ;

            RtlLeaveCriticalSection (&WorkerCriticalSection) ;
            
            return STATUS_UNSUCCESSFUL ;
        }
        
        // queue a cleanup for each worker thread
        
        for (i = 0 ;  i < NumWorkerThreads ; i ++ ) {

            NtSetIoCompletion (
                    WorkerCompletionPort,
                    RtlpThreadCleanup,
                    NULL,
                    STATUS_SUCCESS,
                    0
                    );
        }

        // queue an apc to cleanup all IO worker threads

        for (Node = IOWorkerThreads.Flink ; Node != &IOWorkerThreads ;
                Node = Node->Flink )
        {
            PRTLP_IOWORKER_TCB ThreadCB ;
            
            ThreadCB = CONTAINING_RECORD (Node, RTLP_IOWORKER_TCB, List) ;
            RemoveEntryList( &ThreadCB->List) ;
            TmpHandle = ThreadCB->ThreadHandle ;

            NtQueueApcThread(
                   ThreadCB->ThreadHandle,
                   (PPS_APC_ROUTINE)RtlpThreadCleanup,
                   NULL,
                   NULL,
                   NULL
                   );

            NtClose( TmpHandle ) ;
        }

        NumWorkerThreads = NumIOWorkerThreads = 0 ;

        RtlLeaveCriticalSection (&WorkerCriticalSection) ;

    }

    return STATUS_SUCCESS ;

}


// Private Functions


// Worker functions


NTSTATUS
RtlpQueueWorkerRequest (
    WORKERCALLBACKFUNC Function,
    PVOID Context,
    ULONG Flags
    )
/*++

Routine Description:

    This routine queues up the request to be executed in a worker thread.

Arguments:

    Function - Routine that is called by the worker thread

    Context - Opaque pointer passed in as an argument to WorkerProc

    Flags - flags passed to RtlQueueWorkItem

Return Value:

--*/

{
    NTSTATUS Status ;
    PRTLP_WORK WorkEntry ;
    
    // Increment the outstanding work request counter

    InterlockedIncrement (&NumWorkRequests) ;
    if (Flags & WT_EXECUTELONGFUNCTION) {
        InterlockedIncrement( & NumLongWorkRequests ) ;
    }

    WorkEntry = (PRTLP_WORK) RtlpForceAllocateTPHeap ( sizeof (RTLP_WORK),
                                                        HEAP_ZERO_MEMORY) ;
    WorkEntry->Function = Function ;
    WorkEntry->Flags = Flags ;

    if (Flags & WT_EXECUTEINPERSISTENTTHREAD) {

        // Queue APC to timer thread

        Status = NtQueueApcThread(
                        TimerThreadHandle,
                        (PPS_APC_ROUTINE)RtlpExecuteWorkerRequest,
                        (PVOID) STATUS_SUCCESS,
                        (PVOID) Context,
                        (PVOID) WorkEntry
                        ) ;

    } else {
    
        Status = NtSetIoCompletion (
                    WorkerCompletionPort,
                    RtlpExecuteWorkerRequest,
                    (PVOID) WorkEntry,
                    STATUS_SUCCESS,
                    (ULONG_PTR)Context
                    );
    }
    
    if ( ! NT_SUCCESS(Status) ) {
    
        InterlockedDecrement (&NumWorkRequests) ;
        if (Flags && WT_EXECUTELONGFUNCTION) {
            InterlockedDecrement( &NumLongWorkRequests ) ;
        }

        RtlpFreeTPHeap( WorkEntry ) ;

        ASSERT(FALSE) ;

        #if DBG
        DbgPrint("ERROR!! Thread Pool (RtlQeueuWorkItem): could not queue work item\n");
        #endif
    }

    return Status ;
}


VOID
RtlpExecuteWorkerRequest (
    NTSTATUS Status, //not  used
    PVOID Context,
    PVOID WorkContext
    )
/*++

Routine Description:

    This routine executes a work item.
    
Arguments:

    Context - contains context to be passed to the callback function.

    WorkContext - contains callback function ptr and flags
    
Return Value:

Notes:
    This function executes in a worker thread or a timer thread if
    WT_EXECUTEINTIMERTHREAD flag is set.

--*/

{
    PRTLP_WORK WorkEntry = (PRTLP_WORK) WorkContext;

    #if (DBG1)
    DBG_SET_FUNCTION( WorkEntry->Function, Context ) ;
    #endif

    try {
        ((WORKERCALLBACKFUNC) WorkEntry->Function) ( Context ) ;

    } except (EXCEPTION_EXECUTE_HANDLER) {
//        ASSERT(FALSE);
    }
    
    InterlockedDecrement( &NumWorkRequests ) ;
    if (WorkEntry->Flags & WT_EXECUTELONGFUNCTION) {
        InterlockedDecrement( &NumLongWorkRequests ) ;
    }

    RtlpFreeTPHeap( WorkEntry ) ;
}


NTSTATUS
RtlpQueueIOWorkerRequest (
    WORKERCALLBACKFUNC Function,
    PVOID Context,
    ULONG Flags
    )

/*++

Routine Description:

    This routine queues up the request to be executed in an IO worker thread.

Arguments:

    Function - Routine that is called by the worker thread

    Context - Opaque pointer passed in as an argument to WorkerProc

Return Value:

--*/

{
    NTSTATUS Status ;
    PRTLP_IOWORKER_TCB TCB ;
    BOOLEAN LongFunction = (Flags & WT_EXECUTELONGFUNCTION) ? TRUE : FALSE ;
    PLIST_ENTRY  ple ;
    
                
    if (Flags & WT_EXECUTEINPERSISTENTIOTHREAD) {

        if (!PersistentIOTCB) {
            for (ple=IOWorkerThreads.Flink;  ple!=&IOWorkerThreads;  ple=ple->Flink) {
                TCB = CONTAINING_RECORD (ple, RTLP_IOWORKER_TCB, List) ;
                if (! TCB->LongFunctionFlag)
                    break;
            }

            if (ple == &IOWorkerThreads) {
                // ASSERT(FALSE);
                return STATUS_NO_MEMORY;
            }

            
            PersistentIOTCB = TCB ;
            TCB->Flags |= WT_EXECUTEINPERSISTENTIOTHREAD ;
            
        } else {
            TCB = PersistentIOTCB ;
        }

    } else {
        for (ple=IOWorkerThreads.Flink;  ple!=&IOWorkerThreads;  ple=ple->Flink) {
        
            TCB = CONTAINING_RECORD (ple, RTLP_IOWORKER_TCB, List) ;

            // do not queue to the thread if it is executing a long function, or
            // if you are queueing a long function and the thread is a persistent thread
            
            if (! TCB->LongFunctionFlag
                && (! ((TCB->Flags&WT_EXECUTEINPERSISTENTIOTHREAD)
                        && (Flags&WT_EXECUTELONGFUNCTION)))) {
                break ;
            }            

        }

        if ((ple == &IOWorkerThreads) && (NumIOWorkerThreads<1)) {

            #if DBG
            DbgPrint("ThreadPool:ntdll.dll: Out of memory. "
                     "Could not execute IOWorkItem(%x)\n", (ULONG_PTR)Function);
            #endif
                     
            return STATUS_NO_MEMORY;
        }
        else {
            ple = IOWorkerThreads.Flink;
            TCB = CONTAINING_RECORD (ple, RTLP_IOWORKER_TCB, List) ;

            // treat it as a short function so that counters work fine.
            
            LongFunction = FALSE;            
        }

        // In order to implement "fair" assignment of work items between IO worker threads
        // each time remove the entry and reinsert at back.

        RemoveEntryList (&TCB->List) ;
        InsertTailList (&IOWorkerThreads, &TCB->List) ;
    }

             
    // Increment the outstanding work request counter

    InterlockedIncrement (&NumIOWorkRequests) ;
    if (LongFunction) {
        InterlockedIncrement( &NumLongIOWorkRequests ) ;
        TCB->LongFunctionFlag = TRUE ;
    }

    // Queue an APC to the IoWorker Thread

    Status = NtQueueApcThread(
                    TCB->ThreadHandle,
                    LongFunction? (PPS_APC_ROUTINE)RtlpExecuteLongIOWorkItem:
                                  (PPS_APC_ROUTINE)RtlpExecuteIOWorkItem,
                    (PVOID)Function,
                    Context,
                    TCB
                    );

    if (! NT_SUCCESS( Status ) ) {
        InterlockedDecrement( &NumIOWorkRequests ) ;
        if (LongFunction)
            InterlockedDecrement( &NumLongIOWorkRequests ) ;
    }
    
    return Status ;

}



NTSTATUS
RtlpStartWorkerThread (
    )
/*++

Routine Description:

    This routine starts a regular worker thread

Arguments:


Return Value:

    NTSTATUS error codes resulting from attempts to create a thread
    STATUS_SUCCESS

--*/
{
    HANDLE ThreadHandle ;
    ULONG CurrentTickCount ;
    NTSTATUS Status ;

    // Create worker thread
    
    Status = RtlpStartThreadFunc (RtlpWorkerThread, &ThreadHandle) ;

    if (Status == STATUS_SUCCESS ) {

        // Update the time at which the current thread was created

        LastThreadCreationTickCount = NtGetTickCount() ;

        // Increment the count of the thread type created

        InterlockedIncrement(&NumWorkerThreads) ;

        // Close Thread handle, we dont need it.

        NtClose (ThreadHandle) ;

    } else {

        // Thread creation failed. If there is even one thread present do not return
        // failure - else queue the request anyway.

        if (NumWorkerThreads == 0) {

            return Status ;
        }

    }

    return STATUS_SUCCESS ;
}


NTSTATUS
RtlpStartIOWorkerThread (
    )
/*++

Routine Description:

    This routine starts an I/O worker thread

Arguments:


Return Value:

    NTSTATUS error codes resulting from attempts to create a thread
    STATUS_SUCCESS

--*/
{
    HANDLE ThreadHandle ;
    ULONG CurrentTickCount ;
    NTSTATUS Status ;


    // Create worker thread

    Status = RtlpStartThreadFunc (RtlpIOWorkerThread, &ThreadHandle) ;

    if (Status == STATUS_SUCCESS ) {

        NtClose( ThreadHandle ) ;

        // Update the time at which the current thread was created

        LastThreadCreationTickCount = NtGetTickCount() ;

    } else {

        // Thread creation failed. If there is even one thread present do not return
        // failure since we can still service the work request.

        if (NumIOWorkerThreads == 0) {

            return Status ;

        }
    }

    return STATUS_SUCCESS ;
}


VOID
RtlpWorkerThreadTimerCallback(
    PVOID Context,
    BOOLEAN NotUsed
    )
/*++

Routine Description:

    This routine checks if new worker thread has to be created

Arguments:
    None

Return Value:
    None
    
--*/
{
    IO_COMPLETION_BASIC_INFORMATION Info ;
    BOOLEAN bCreateThread = FALSE ;
    NTSTATUS Status ;
    ULONG QueueLength, Threshold, ShortWorkRequests ;
    
    
    Status = NtQueryIoCompletion(
                WorkerCompletionPort,
                IoCompletionBasicInformation,
                &Info,
                sizeof(Info),
                NULL
                ) ;

    if (!NT_SUCCESS(Status))
        return ;

    QueueLength = Info.Depth ;

    if (!QueueLength) {
        OldTotalExecutedWorkRequests = TotalExecutedWorkRequests ;
        return ;
    }


    RtlEnterCriticalSection (&WorkerCriticalSection) ;

    
    // if there are queued work items and no new work items have been scheduled
    // in the last 30 seconds then create a new thread.
    // this will take care of deadlocks.

    // this will create a problem only if some thread is running for a long time
    
    if (TotalExecutedWorkRequests == OldTotalExecutedWorkRequests) {

        bCreateThread = TRUE ;
    }
        
    
    // if there are a lot of queued work items, then create a new thread
    {
        ULONG NumEffWorkerThreads = (NumWorkerThreads - NumLongWorkRequests) ;
        ULONG ShortWorkRequests ;
        
        Threshold = (NumWorkerThreads < MAX_WORKER_THREADS
                        ? (NumEffWorkerThreads < 7
                            ? NumEffWorkerThreads*NumEffWorkerThreads
                            : NEW_THREAD_THRESHOLD * NumEffWorkerThreads )
                        : 0xffffffff) ;

        ShortWorkRequests = QueueLength + NumExecutingWorkerThreads
                                - NumLongWorkRequests ;

        if (ShortWorkRequests  > Threshold)
        {
            bCreateThread = TRUE ;
        }
    }

    if (bCreateThread) {

        RtlpStartWorkerThread () ;
    }


    OldTotalExecutedWorkRequests = TotalExecutedWorkRequests ;
    
    RtlLeaveCriticalSection (&WorkerCriticalSection) ;

}



NTSTATUS
RtlpInitializeWorkerThreadPool (
    )
/*++

Routine Description:

    This routine initializes all aspects of the thread pool.

Arguments:

    None

Return Value:

    None

--*/
{
    NTSTATUS Status = STATUS_SUCCESS ;
    LARGE_INTEGER TimeOut ;


    // Initialize the timer component if it hasnt been done already

    if (CompletedTimerInitialization != 1) {

        Status = RtlpInitializeTimerThreadPool () ;

        if ( !NT_SUCCESS(Status) )
            return Status ;

    }

    
    // In order to avoid an explicit RtlInitialize() function to initialize the thread pool
    // we use StartedInitialization and CompletedInitialization to provide us the necessary
    // synchronization to avoid multiple threads from initializing the thread pool.
    // This scheme does not work if RtlInitializeCriticalSection() fails - but in this case the
    // caller has no choices left.

    if (!InterlockedExchange(&StartedWorkerInitialization, 1L)) {

        if (CompletedWorkerInitialization)
            InterlockedExchange( &CompletedWorkerInitialization, 0 ) ;

            
        do {

            // Initialize Critical Sections

            Status = RtlInitializeCriticalSection( &WorkerCriticalSection );
            if (!NT_SUCCESS(Status))
                break ;


            InitializeListHead (&IOWorkerThreads) ;

            {            
                SYSTEM_BASIC_INFORMATION BasicInfo;

                // get number of processors

                Status = NtQuerySystemInformation (
                                    SystemBasicInformation,
                                    &BasicInfo,
                                    sizeof(BasicInfo),
                                    NULL
                                    ) ;

                if ( !NT_SUCCESS(Status) ) {
                    BasicInfo.NumberOfProcessors = 1 ;
                }

                // Create completion port used by worker threads

                Status = NtCreateIoCompletion (
                                    &WorkerCompletionPort,
                                    IO_COMPLETION_ALL_ACCESS,
                                    NULL,
                                    BasicInfo.NumberOfProcessors
                                    );

                if (!NT_SUCCESS(Status))
                    break ;

            }

        } while ( FALSE ) ;

        if (!NT_SUCCESS(Status) ) {
        
            ASSERT ( Status == STATUS_SUCCESS ) ;
            StartedWorkerInitialization = 0 ;
            InterlockedExchange( &CompletedWorkerInitialization, ~0 ) ;
            return Status ;
        }
        
        // Signal that initialization has completed

        InterlockedExchange (&CompletedWorkerInitialization, 1L) ;

    } else {

        LARGE_INTEGER Timeout ;

        // Sleep 1 ms and see if the other thread has completed initialization

        ONE_MILLISECOND_TIMEOUT(TimeOut) ;

        while (!(volatile ULONG) CompletedWorkerInitialization) {

            NtDelayExecution (FALSE, &TimeOut) ;
        }

        if (CompletedWorkerInitialization != 1)
            return STATUS_NO_MEMORY ;
        
    }


    //
    // create timer for worker thread. it should be created outside any worker
    // lock and the above wait loop. It should not make the RtlpInitializeWorkerThread
    // call blocking on a timer thread. so queue a work item.
    //
    
    if (NT_SUCCESS(Status)
        && (InterlockedIncrement(&WorkerThreadTimerQueueInit) == 1) )
    {
        RtlQueueWorkItem(RtlpWorkerThreadInitializeTimers, NULL, 0);
    }
    

    return NT_SUCCESS(Status)  ? STATUS_SUCCESS : Status ;
}



VOID
RtlpWorkerThreadInitializeTimers(
    PVOID Context
    )
{
    NTSTATUS Status;
    

    // create a timer that will fire every 30 sec and check if new thread
    // should be created

    Status = RtlCreateTimerQueue(&WorkerThreadTimerQueue) ;
    if (!NT_SUCCESS(Status))
        return ;
    
    Status = RtlCreateTimer(
                        WorkerThreadTimerQueue,
                        &WorkerThreadTimer,
                        RtlpWorkerThreadTimerCallback,
                        NULL,
                        30000,
                        30000,
                        WT_EXECUTEINTIMERTHREAD
                        ) ;

   return;
}






LONG
RtlpWorkerThread (
    PVOID  Initialized
    )
/*++

Routine Description:

    All non I/O worker threads execute in this routine. Worker thread will try to
    terminate when it has not serviced a request for

        STARTING_WORKER_SLEEP_TIME +
        STARTING_WORKER_SLEEP_TIME << 1 +
        ...
        STARTING_WORKER_SLEEP_TIME << MAX_WORKER_SLEEP_TIME_EXPONENT

Arguments:

    Initialized - Set to 1 when we are initialized

Return Value:

--*/
{
    NTSTATUS Status ;
    PVOID WorkerProc ;
    PVOID Context ;
    IO_STATUS_BLOCK IoSb ;
    ULONG SleepTime ;
    LARGE_INTEGER TimeOut ;
    ULONG Terminate ;
    PVOID Overlapped ;


    // We are all initialized now. Notify the starter to queue the task.

    InterlockedExchange ((ULONG *)Initialized, 1L) ;


    // Set default sleep time for 40 seconds. This time is doubled each time a timeout
    // occurs after which the thread terminates

#define WORKER_IDLE_TIMEOUT     40000    // In Milliseconds
#define MAX_WORKER_SLEEP_TIME_EXPONENT 4

    SleepTime = WORKER_IDLE_TIMEOUT ;

    // Loop servicing I/O completion requests

    for ( ; ; ) {

        TimeOut.QuadPart = Int32x32To64( SleepTime, -10000 ) ;

        Status = NtRemoveIoCompletion(
                    WorkerCompletionPort,
                    (PVOID) &WorkerProc,
                    &Overlapped,
                    &IoSb,
                    &TimeOut
                    ) ;

        if (Status == STATUS_SUCCESS) {


            TotalExecutedWorkRequests ++ ;//interlocked op not req
            InterlockedIncrement(&NumExecutingWorkerThreads) ;
            
            // Call the work item. 
            // If IO APC, context1 contains number of IO bytes transferred, and context2
            // contains the overlapped structure.
            // If (IO)WorkerFunction, context1 contains the actual WorkerFunction to be
            // executed and context2 contains the actual context

            Context = (PVOID) IoSb.Information ;

            try {
                ((APC_CALLBACK_FUNCTION)WorkerProc) (
                                            IoSb.Status, 
                                            Context,        // Number of IO bytes transferred
                                            Overlapped      // Overlapped structure
                                            ) ;
            } except (EXCEPTION_EXECUTE_HANDLER) {
                ASSERT(FALSE);
            }

            SleepTime = WORKER_IDLE_TIMEOUT ;

            InterlockedDecrement(&NumExecutingWorkerThreads) ;

        } else if (Status == STATUS_TIMEOUT) {

            // NtRemoveIoCompletion timed out. Check to see if have hit our limit
            // on waiting. If so terminate.

            Terminate = FALSE ;

            RtlEnterCriticalSection (&WorkerCriticalSection) ;

            // The thread terminates if there are > 1 threads and the queue is small
            // OR if there is only 1 thread and there is no request pending

            if (NumWorkerThreads >  1) {

                ULONG NumEffWorkerThreads = (NumWorkerThreads - NumLongWorkRequests) ;

                if (NumEffWorkerThreads == 0) {

                    Terminate = FALSE ;

                } else if (SleepTime >= (WORKER_IDLE_TIMEOUT << MAX_WORKER_SLEEP_TIME_EXPONENT)) {

                    //
                    // have been idle for very long time. terminate irrespective of number of
                    // work items. (This is useful when the set of runnable threads is taking
                    // care of all the work items being queued). dont terminate if
                    // (NumEffWorkerThreads == 1)
                    //
                    
                    if (NumEffWorkerThreads > 1)
                        Terminate = TRUE ;

                } else {
                
                    ULONG Threshold ;
                    
                    // Check if we need to shrink worker thread pool

                    Threshold = NumEffWorkerThreads < 7
                                ? NumEffWorkerThreads*(NumEffWorkerThreads-1)
                                : NEW_THREAD_THRESHOLD * (NumEffWorkerThreads-1);


                    
                    if  (NumWorkRequests-NumLongWorkRequests < Threshold)  {

                        Terminate = TRUE ;

                    } else {

                        Terminate = FALSE ;
                        SleepTime <<= 1 ;
                    }
                }
                
            } else {

                if ( (NumMinWorkerThreads == 0) && (NumWorkRequests == 0) ) {

                    // delay termination of last thread
                    
                    if (SleepTime < (WORKER_IDLE_TIMEOUT << MAX_WORKER_SLEEP_TIME_EXPONENT)) {
                        SleepTime <<= 1 ;
                        Terminate = FALSE ;
                    }
                    else {
                        Terminate = TRUE ;
                    }
                    
                } else {

                    Terminate = FALSE ;

                }

            }

            if (Terminate) {

                THREAD_BASIC_INFORMATION ThreadInfo;
                ULONG IsIoPending ;
                HANDLE CurThreadHandle ;

                Status = NtDuplicateObject(
                            NtCurrentProcess(),
                            NtCurrentThread(),
                            NtCurrentProcess(),
                            &CurThreadHandle,
                            0,
                            FALSE,
                            DUPLICATE_SAME_ACCESS
                            ) ;

                ASSERT (Status == STATUS_SUCCESS) ;

                Terminate = FALSE ;

                Status = NtQueryInformationThread( CurThreadHandle,
                                                   ThreadIsIoPending,
                                                   &IsIoPending,
                                                   sizeof( IsIoPending ),
                                                   NULL
                                                 );
                if (NT_SUCCESS( Status )) {

                    if (! IsIoPending )
                        Terminate = TRUE ;
                }

                NtClose( CurThreadHandle ) ;
            }
            
            if (Terminate) {

                InterlockedDecrement (&NumWorkerThreads) ;

                RtlLeaveCriticalSection (&WorkerCriticalSection) ;

                RtlpExitThreadFunc( 0 );

            } else {

                // This is the condition where a request was queued *after* the
                // thread woke up - ready to terminate because of inactivity. In
                // this case dont terminate - service the completion port.

                RtlLeaveCriticalSection (&WorkerCriticalSection) ;

            }

        } else {

            ASSERT (FALSE) ;

        }

    }


    return 1 ;
}



LONG
RtlpIOWorkerThread (
    PVOID  Initialized
    )
/*++

Routine Description:

    All I/O worker threads execute in this routine. All the work requests execute as APCs
    in this thread.

Arguments:

    Initialized - set to 1 when the initialization has completed

Return Value:

--*/
{
    #define IOWORKER_IDLE_TIMEOUT     40000    // In Milliseconds
    
    LARGE_INTEGER TimeOut ;
    ULONG SleepTime = IOWORKER_IDLE_TIMEOUT ;
    RTLP_IOWORKER_TCB ThreadCB ;    // Control Block allocated on the stack
    NTSTATUS Status ;
    BOOLEAN Terminate ;

    
    //
    // Initialize thread control block
    // and insert it into list of IOWorker Threads
    //
    
    Status = NtDuplicateObject(
                NtCurrentProcess(),
                NtCurrentThread(),
                NtCurrentProcess(),
                &ThreadCB.ThreadHandle,
                0,
                FALSE,
                DUPLICATE_SAME_ACCESS
                ) ;

    if (!NT_SUCCESS(Status)) {
        ASSERT (FALSE) ;
        InterlockedExchange ((ULONG *)Initialized, (ULONG)~0) ;
        return STATUS_NO_MEMORY;
    }

    InsertHeadList (&IOWorkerThreads, &ThreadCB.List) ;
    ThreadCB.Flags = 0 ;
    ThreadCB.LongFunctionFlag = FALSE ;

    InterlockedIncrement(&NumIOWorkerThreads) ;

    
    // We are all initialized now. Notify the starter to queue the task.

    InterlockedExchange ((ULONG *)Initialized, 1L) ;



    // Sleep alertably so that all the activity can take place
    // in APCs

    for ( ; ; ) {

        // Set timeout for IdleTimeout

        TimeOut.QuadPart = Int32x32To64( SleepTime, -10000 ) ;


        Status = NtDelayExecution (TRUE, &TimeOut) ;


        // Status is STATUS_SUCCESS only when it has timed out
        
        if (Status != STATUS_SUCCESS) {
            continue ;
        } 


        //
        // idle timeout. check if you can terminate the thread
        //
        
        Terminate = FALSE ;

        RtlEnterCriticalSection (&WorkerCriticalSection) ;


        // dont terminate if it is a persistent thread
        
        if (ThreadCB.Flags & WT_EXECUTEINPERSISTENTIOTHREAD) {

            TimeOut.LowPart = 0x0;
            TimeOut.HighPart = 0x80000000;

            RtlLeaveCriticalSection (&WorkerCriticalSection) ;

            continue ;
        }

        
        // The thread terminates if there are > 1 threads and the queue is small
        // OR if there is only 1 thread and there is no request pending

        if (NumIOWorkerThreads >  1) {


            ULONG NumEffIOWorkerThreads = NumIOWorkerThreads - NumLongIOWorkRequests ;
            ULONG Threshold ;

            if (NumEffIOWorkerThreads == 0) {

                Terminate = FALSE ;

            } else {
            
                // Check if we need to shrink worker thread pool

                Threshold = NEW_THREAD_THRESHOLD * (NumEffIOWorkerThreads-1);



                if  (NumIOWorkRequests-NumLongIOWorkRequests < Threshold)  {

                    Terminate = TRUE ;

                } else {

                    Terminate = FALSE ;
                    SleepTime <<= 1 ;
                }
            }

        } else {

            if (NumIOWorkRequests == 0) {

                // delay termination of last thread

                if (SleepTime < 4*IOWORKER_IDLE_TIMEOUT) {
                
                    SleepTime <<= 1 ;
                    Terminate = FALSE ;

                } else {
                
                    Terminate = TRUE ;
                }

            } else {

                Terminate = FALSE ;

            }

        }

        //
        // terminate only if no io is pending
        //
        
        if (Terminate) {

            NTSTATUS Status;
            THREAD_BASIC_INFORMATION ThreadInfo;
            ULONG IsIoPending ;
            
            Terminate = FALSE ;
            
            Status = NtQueryInformationThread( ThreadCB.ThreadHandle,
                                               ThreadIsIoPending,
                                               &IsIoPending,
                                               sizeof( IsIoPending ),
                                               NULL
                                             );
            if (NT_SUCCESS( Status )) {

                if (! IsIoPending )
                    Terminate = TRUE ;
            }
        }

        if (Terminate) {

            InterlockedDecrement (&NumIOWorkerThreads) ;

            RemoveEntryList (&ThreadCB.List) ;
            NtClose( ThreadCB.ThreadHandle ) ;
            
            RtlLeaveCriticalSection (&WorkerCriticalSection) ;

            RtlpExitThreadFunc( 0 );

        } else {

            // This is the condition where a request was queued *after* the
            // thread woke up - ready to terminate because of inactivity. In
            // this case dont terminate - service the completion port.

            RtlLeaveCriticalSection (&WorkerCriticalSection) ;

        }
    }

    return 0 ;  // Keep compiler happy

}



VOID
RtlpExecuteLongIOWorkItem (
    PVOID Function,
    PVOID Context,
    PVOID ThreadCB
    )
/*++

Routine Description:

    Executes an IO Work function. RUNs in a APC in the IO Worker thread.

Arguments:

    Function - Worker function to call

    Context - Argument for the worker function.

    NotUsed - Argument is not used in this function.

Return Value:

--*/
{
    #if (DBG1)
    DBG_SET_FUNCTION( Function, Context ) ;
    #endif

    // Invoke the function

    try {
        ((WORKERCALLBACKFUNC) Function)((PVOID)Context) ;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        ASSERT(FALSE);
    }


    ((PRTLP_IOWORKER_TCB)ThreadCB)->LongFunctionFlag = FALSE ;

    // Decrement pending IO requests count

    InterlockedDecrement (&NumIOWorkRequests) ;

    // decrement pending long funcitons

    InterlockedDecrement (&NumLongIOWorkRequests ) ;
}


VOID
RtlpExecuteIOWorkItem (
    PVOID Function,
    PVOID Context,
    PVOID NotUsed
    )
/*++

Routine Description:

    Executes an IO Work function. RUNs in a APC in the IO Worker thread.

Arguments:

    Function - Worker function to call

    Context - Argument for the worker function.

    NotUsed - Argument is not used in this function.

Return Value:


--*/
{
    #if (DBG1)
    DBG_SET_FUNCTION( Function, Context ) ;
    #endif

    // Invoke the function

    try {
        ((WORKERCALLBACKFUNC) Function)((PVOID)Context) ;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        ASSERT(FALSE);
    }

    // Decrement pending IO requests count

    InterlockedDecrement (&NumIOWorkRequests) ;

}


NTSTATUS
NTAPI
RtlpStartThread (
    PUSER_THREAD_START_ROUTINE Function,
    HANDLE *ThreadHandle
    )
/*++

Routine Description:

    This routine is used start a new wait thread in the pool.
Arguments:

    None

Return Value:

    STATUS_SUCCESS - Timer Queue created successfully.

    STATUS_NO_MEMORY - There was not sufficient heap to perform the requested operation.

--*/
{
    NTSTATUS Status ;
    ULONG Initialized ;
    LARGE_INTEGER TimeOut ;

    Initialized = FALSE ;

    // Create the first thread. This thread never dies until the process exits

    Status = RtlCreateUserThread(
                   NtCurrentProcess(), // process handle
                   NULL,               // security descriptor
                   FALSE,              // Create suspended?
                   0L,                 // ZeroBits: default
                   0L,                 // Max stack size: default
                   0L,                 // Committed stack size: default
                   Function,           // Function to start in
                   &Initialized,       // Event the thread signals when the thread is ready
                   ThreadHandle,       // Thread handle
                   NULL                // Thread id
                   );

    if ( Status == STATUS_SUCCESS ) {

        // Sleep 1 ms and see if the other thread has completed initialization

        ONE_MILLISECOND_TIMEOUT(TimeOut) ;

        while (!(volatile ULONG) Initialized) {

            NtDelayExecution (FALSE, &TimeOut) ;

        }

    }


    return Status ;
}

NTSTATUS
RtlpExitThread(
    NTSTATUS Status
    )
{
    return NtTerminateThread( NtCurrentThread(), Status );
}



// Wait functions


NTSTATUS
RtlpInitializeWaitThreadPool (
    )
/*++

Routine Description:

    This routine initializes all aspects of the thread pool.

Arguments:

    None

Return Value:

    None

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    LARGE_INTEGER TimeOut ;

    // In order to avoid an explicit RtlInitialize() function to initialize the wait thread pool
    // we use StartedWaitInitialization and CompletedWait Initialization to provide us the
    // necessary synchronization to avoid multiple threads from initializing the thread pool.
    // This scheme does not work if RtlInitializeCriticalSection() or NtCreateEvent fails - but in this case the
    // caller has not choices left.

    if (!InterlockedExchange(&StartedWaitInitialization, 1L)) {

        if (CompletedWaitInitialization)
            InterlockedExchange (&CompletedWaitInitialization, 0L) ;

        // Initialize Critical Section

        Status = RtlInitializeCriticalSection( &WaitCriticalSection ) ;

        if (! NT_SUCCESS( Status ) ) {

            ASSERT ( NT_SUCCESS( Status ) ) ;

            StartedWaitInitialization = 0 ;
            InterlockedExchange (&CompletedWaitInitialization, ~0) ;
            
            return Status ;
        }
            
        InitializeListHead (&WaitThreads);  // Initialize global wait threads list

        InterlockedExchange (&CompletedWaitInitialization, 1L) ;

    } else {

        // Sleep 1 ms and see if the other thread has completed initialization

        ONE_MILLISECOND_TIMEOUT(TimeOut) ;

        while (!(volatile ULONG) CompletedWaitInitialization) {

            NtDelayExecution (FALSE, &TimeOut) ;

        }

        if (CompletedWaitInitialization != 1) {
            Status = STATUS_NO_MEMORY ;
        }
    }

    return Status ;
}



LONG
RtlpWaitThread (
    PVOID  Initialized
    )
/*++

Routine Description:

    This routine is used for all waits in the wait thread pool

Arguments:

    Initialized - This is set to 1 when the thread has initialized

Return Value:

    Nothing. The thread never terminates.

--*/
{
    ULONG  i ;                                   // Used as an index
    NTSTATUS Status ;
    LARGE_INTEGER TimeOut;                       // Timeout used for waits
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB ;   // Control Block allocated on the stack
#define WAIT_IDLE_TIMEOUT 400000


    try {
        ThreadCB = (PRTLP_WAIT_THREAD_CONTROL_BLOCK)
                        _alloca(sizeof(RTLP_WAIT_THREAD_CONTROL_BLOCK));

    } except (EXCEPTION_EXECUTE_HANDLER) {
    
        ASSERT(FALSE);
        InterlockedExchange ((ULONG *)Initialized, (ULONG)~0) ;
        return STATUS_NO_MEMORY;
    }
    

    
    // Initialize thread control block

    InitializeListHead (&ThreadCB->WaitThreadsList) ;

    Status = NtDuplicateObject(
                NtCurrentProcess(),
                NtCurrentThread(),
                NtCurrentProcess(),
                &ThreadCB->ThreadHandle,
                0,
                FALSE,
                DUPLICATE_SAME_ACCESS
                ) ;

    if (!NT_SUCCESS(Status)) {
        ASSERT (FALSE) ;
        InterlockedExchange ((ULONG *)Initialized, (ULONG)~0) ;
        return STATUS_NO_MEMORY;
    }

    ThreadCB->ThreadId =  HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread) ;

    RtlZeroMemory (&ThreadCB->ActiveWaitArray[0], sizeof (HANDLE) * 64) ;

    RtlZeroMemory (&ThreadCB->ActiveWaitPointers[0], sizeof (HANDLE) * 64) ;



    // Initialize the timer related fields.

    Status = NtCreateTimer(
                 &ThreadCB->TimerHandle,
                 TIMER_ALL_ACCESS,
                 NULL,
                 NotificationTimer
                 ) ;

    if (! NT_SUCCESS( Status )) {
        ASSERT (FALSE);
        NtClose(ThreadCB->ThreadHandle) ;
        InterlockedExchange ((ULONG *)Initialized, (ULONG)~0) ;
        return STATUS_NO_MEMORY;
    }
    

    ThreadCB->Firing64BitTickCount = 0 ;
    ThreadCB->Current64BitTickCount.QuadPart = NtGetTickCount() ;

    // Reset the NT Timer to never fire initially

    RtlpResetTimer (ThreadCB->TimerHandle, -1, ThreadCB) ;
    
    InitializeListHead (&ThreadCB->TimerQueue.TimerList) ;
    InitializeListHead (&ThreadCB->TimerQueue.UncancelledTimerList) ;

    
    // Initialize the timer blocks

    RtlZeroMemory (&ThreadCB->TimerBlocks[0], sizeof (RTLP_TIMER) * 63) ;

    InitializeListHead (&ThreadCB->FreeTimerBlocks) ;

    for (i = 0 ; i < 63 ; i++) {

        InitializeListHead (&(&ThreadCB->TimerBlocks[i])->List) ;
        InsertHeadList (&ThreadCB->FreeTimerBlocks, &(&ThreadCB->TimerBlocks[i])->List) ;

    }


    // Insert this new wait thread in the WaitThreads list. Insert at the head so that
    // the request that caused this thread to be created can find it right away.

    InsertHeadList (&WaitThreads, &ThreadCB->WaitThreadsList) ;


    // The first wait element is the timer object

    ThreadCB->ActiveWaitArray[0] = ThreadCB->TimerHandle ;

    ThreadCB->NumActiveWaits = ThreadCB->NumWaits = 1 ;


    // till here, the function is running under the global wait lock


    
    // We are all initialized now. Notify the starter to queue the task.

    InterlockedExchange ((ULONG *)Initialized, 1) ;


    // Loop forever - wait threads never, never die.

    for ( ; ; ) {

        if (ThreadCB->NumActiveWaits == 1)
            TimeOut.QuadPart = Int32x32To64( WAIT_IDLE_TIMEOUT, -10000 ) ;

        Status = NtWaitForMultipleObjects (
                     (CHAR) ThreadCB->NumActiveWaits,
                     ThreadCB->ActiveWaitArray,
                     WaitAny,
                     TRUE,      // Wait Alertably
                     ThreadCB->NumWaits!=1 ? NULL : &TimeOut       // Wait forever
                     ) ;

        if (Status == STATUS_ALERTED || Status == STATUS_USER_APC) {

            continue ;

        } else if (Status >= STATUS_WAIT_0 && Status <= STATUS_WAIT_63) {

            if (Status == STATUS_WAIT_0) {

                RtlpProcessTimeouts (ThreadCB) ;

            } else {

                // Wait completed call Callback function

                RtlpProcessWaitCompletion (
                        ThreadCB->ActiveWaitPointers[Status], Status) ;

            }

        } else if (Status >= STATUS_ABANDONED_WAIT_0 
                    && Status <= STATUS_ABANDONED_WAIT_63) {

            #if DBG
            DbgPrint ("RTL ThreadPool Wait Thread: Abandoned wait: %d\n",
                        Status - STATUS_ABANDONED_WAIT_0 ) ;
            #endif

            
            // Abandoned wait

            ASSERT (FALSE) ;

        } else if (Status == STATUS_TIMEOUT) {

            //
            // remove this thread from the wait list and terminate
            //

            {
                ULONG NumWaits;
                
                ACQUIRE_GLOBAL_WAIT_LOCK() ;

                NumWaits = ThreadCB->NumWaits;
                
                if (ThreadCB->NumWaits <= 1) {
                    RemoveEntryList(&ThreadCB->WaitThreadsList) ;
                    NtClose(ThreadCB->ThreadHandle) ;
                    NtClose(ThreadCB->TimerHandle) ;
                }

                RELEASE_GLOBAL_WAIT_LOCK() ;

                if (NumWaits <= 1) {

                    RtlpExitThreadFunc( 0 );
                }
            }
            
        } else {

            // Some other error: fatal condition
            ULONG i ;
            
//            ASSERTMSG( "Press 'i', and note the dbgprint\n", FALSE ) ;

            #if DBG
            DbgPrint ("RTL Thread Pool: Application closed an object handle "
                        "that the wait thread was waiting on: Code:%x ThreadId:<%x:%x>\n",
                        Status, HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
                        HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)) ;

            TimeOut.QuadPart = 0 ;

            for (i=0;  i<ThreadCB->NumActiveWaits;  i++) {

                Status = NtWaitForMultipleObjects(
                             (CHAR) 1,
                             &ThreadCB->ActiveWaitArray[i],
                             WaitAny,
                             TRUE,      // Wait Alertably
                             &TimeOut   // Dont 0
                             ) ;

                if (Status == STATUS_INVALID_HANDLE) {
                    DbgPrint("Bad Handle index:%d WaitEntry Ptr:%x\n",
                                i, ThreadCB->ActiveWaitPointers[i]) ;
                }
            }
            
            #endif

            ASSERT( FALSE ) ;

            
            // Set timeout for the largest timeout possible

            TimeOut.LowPart = 0 ;
            TimeOut.HighPart = 0x80000000 ;

            NtDelayExecution (TRUE, &TimeOut) ;
            
        }

    } // forever

    return 0 ; // Keep compiler happy

}


VOID
RtlpAsyncCallbackCompletion(
    PVOID Context
    )
/*++

Routine Description:

    This routine is called in a (IO)worker thread and is used to decrement the 
    RefCount at the end and call RtlpDelete(Wait/Timer) if required

Arguments:

    Context - AsyncCallback: containing pointer to Wait/Timer object, 

Return Value:

--*/
{
    PRTLP_ASYNC_CALLBACK AsyncCallback ;
    
    AsyncCallback = (PRTLP_ASYNC_CALLBACK) Context ;

    // callback queued by WaitThread (event or timer)
    
    if ( AsyncCallback->WaitThreadCallback ) {

        PRTLP_WAIT Wait = AsyncCallback->Wait ;

        //DPRN5
        if (DPRN4)
        DbgPrint("Calling WaitOrTimer: fn:%x  context:%x  bool:%d Thread<%d:%d>\n",
                (ULONG_PTR)Wait->Function, (ULONG_PTR)Wait->Context,
                (ULONG_PTR)AsyncCallback->TimerCondition,
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)
                ) ;

        #if (DBG1)
        DBG_SET_FUNCTION( Wait->Function, Wait->Context ) ;
        #endif
        
        ((WAITORTIMERCALLBACKFUNC) Wait->Function) 
                                ( Wait->Context, AsyncCallback->TimerCondition ) ;

        if ( InterlockedDecrement( &Wait->RefCount ) == 0 ) {

            RtlpDeleteWait( Wait ) ;            
        }

    }

    // callback queued by TimerThread
    
    else {

        PRTLP_TIMER Timer = AsyncCallback->Timer ;

        //DPRN5
        if (DPRN4)
        DbgPrint("Calling WaitOrTimer:Timer: fn:%x  context:%x  bool:%d Thread<%d:%d>\n",
                (ULONG_PTR)Timer->Function, (ULONG_PTR)Timer->Context,
                TRUE,
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)
                ) ;
                
        #if (DBG1)
        DBG_SET_FUNCTION( Timer->Function, Timer->Context ) ;
        #endif
  

        ((WAITORTIMERCALLBACKFUNC) Timer->Function) ( Timer->Context , TRUE) ;


        // decrement RefCount after function is executed so that the context is not deleted
        
        if ( InterlockedDecrement( &Timer->RefCount ) == 0 ) {
        
            RtlpDeleteTimer( Timer ) ;            
        }
        
    }

    
    RtlpFreeTPHeap( AsyncCallback );

}


VOID
RtlpProcessWaitCompletion (
    PRTLP_WAIT Wait,
    ULONG ArrayIndex
    )
/*++

Routine Description:

    This routine is used for processing a completed wait

Arguments:

    Wait - Wait that completed

Return Value:

--*/
{
    ULONG TimeRemaining ;
    ULONG NewFiringTime ;
    LARGE_INTEGER DueTime ;
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB ;
    PRTLP_ASYNC_CALLBACK AsyncCallback ;

    ThreadCB = Wait->ThreadCB ;

    // deactivate wait if it is meant for single execution
    
    if ( Wait->Flags & WT_EXECUTEONLYONCE ) {

        RtlpDeactivateWait (Wait) ;
    } 

    else {
        // if wait being reactivated, then reset the timer now itself as
        // it can be deleted in the callback function

        if  ( Wait->Timer ) {

            TimeRemaining = RtlpGetTimeRemaining (ThreadCB->TimerHandle) ;

            if (RtlpReOrderDeltaList (
                        &ThreadCB->TimerQueue.TimerList,
                        Wait->Timer,
                        TimeRemaining,
                        &NewFiringTime,
                        Wait->Timer->Period)) {

                // There is a new element at the head of the queue we need to reset the NT
                // timer to fire later

                RtlpResetTimer (ThreadCB->TimerHandle, NewFiringTime, ThreadCB) ;
            }

        }

        // move the wait entry to the end, and shift elements to its right one pos towards left
        {
            HANDLE HandlePtr = ThreadCB->ActiveWaitArray[ArrayIndex];
            PRTLP_WAIT WaitPtr = ThreadCB->ActiveWaitPointers[ArrayIndex];

            RtlpShiftWaitArray(ThreadCB, ArrayIndex+1, ArrayIndex,
                            ThreadCB->NumActiveWaits -1 - ArrayIndex)
            ThreadCB->ActiveWaitArray[ThreadCB->NumActiveWaits-1] = HandlePtr ;
            ThreadCB->ActiveWaitPointers[ThreadCB->NumActiveWaits-1] = WaitPtr ;
        }
    }
    
    // call callback function (FALSE since this isnt a timeout related callback)

    if ( Wait->Flags & WT_EXECUTEINWAITTHREAD ) {
    
        // executing callback after RtlpDeactivateWait allows the Callback to call
        // RtlDeregisterWait Wait->RefCount is not incremented so that RtlDeregisterWait 
        // will work on this Wait. Though Wait->RefCount is not incremented, others cannot
        // deregister this Wait as it has to be queued as an APC.

        if (DPRN4)
        DbgPrint("Calling WaitOrTimer(wait): fn:%x  context:%x  bool:%d Thread<%d:%d>\n",
                (ULONG_PTR)Wait->Function, (ULONG_PTR)Wait->Context,
                FALSE,
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)
                ) ;
                
        #if (DBG1)
        DBG_SET_FUNCTION( Wait->Function, Wait->Context ) ;
        #endif
        
        
        ((WAITORTIMERCALLBACKFUNC)(Wait->Function))(Wait->Context, FALSE) ;


        // Wait object could have been deleted in the above callback
        
        return ;

        
    } else {

        AsyncCallback = RtlpForceAllocateTPHeap( sizeof( RTLP_ASYNC_CALLBACK ), 0 );
        
        if ( AsyncCallback ) {

            NTSTATUS Status;
            
            AsyncCallback->Wait = Wait ;
            AsyncCallback->WaitThreadCallback = TRUE ;
            AsyncCallback->TimerCondition = FALSE ;

            InterlockedIncrement( &Wait->RefCount ) ;

            Status = RtlQueueWorkItem( RtlpAsyncCallbackCompletion, AsyncCallback,
                                Wait->Flags );


            if (!NT_SUCCESS(Status)) {

                RtlpFreeTPHeap( AsyncCallback );

                if ( InterlockedDecrement( &Wait->RefCount ) == 0 ) {
                    RtlpDeleteWait( Wait ) ;
                }
            }
        }
    }
}


VOID
RtlpAddWait (
    PRTLP_WAIT Wait
    )
/*++

Routine Description:

    This routine is used for adding waits to the wait thread. It is executed in
    an APC.

Arguments:

    Wait - The wait to add

Return Value:

--*/
{
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB = Wait->ThreadCB;


    // if the state is deleted, it implies that RtlDeregister was called in a
    // WaitThreadCallback for a Wait other than that which was fired. This is 
    // an application bug. I Assert, but also handle it.
    
    if ( Wait->State & STATE_DELETE ) {

        ASSERT(FALSE) ;

        InterlockedDecrement( &ThreadCB->NumWaits ) ;
        
        RtlpDeleteWait (Wait) ;

        return ;
    }

    
    // activate Wait
        
    ThreadCB->ActiveWaitArray [ThreadCB->NumActiveWaits] = Wait->WaitHandle ;
    ThreadCB->ActiveWaitPointers[ThreadCB->NumActiveWaits] = Wait ;
    ThreadCB->NumActiveWaits ++ ;
    Wait->State |= (STATE_REGISTERED | STATE_ACTIVE) ;
    Wait->RefCount = 1 ;


    // Fill in the wait timer

    if (Wait->Timeout != INFINITE_TIME) {

        ULONG TimeRemaining ;
        ULONG NewFiringTime ;

        // Initialize timer related fields and insert the timer in the timer queue for
        // this wait thread

        Wait->Timer = (PRTLP_TIMER) RemoveHeadList(&ThreadCB->FreeTimerBlocks);
        Wait->Timer->Function = Wait->Function ;
        Wait->Timer->Context = Wait->Context ;
        Wait->Timer->Flags = Wait->Flags ;
        Wait->Timer->DeltaFiringTime = Wait->Timeout ;
        Wait->Timer->Period = ( Wait->Flags & WT_EXECUTEONLYONCE )
                                ? 0 
                                : Wait->Timeout == INFINITE_TIME
                                ? 0 : Wait->Timeout ;

        Wait->Timer->State = ( STATE_REGISTERED | STATE_ACTIVE ) ; ;
        Wait->Timer->Wait = Wait ;
        Wait->Timer->RefCountPtr = &Wait->RefCount ;
        Wait->Timer->Queue = &ThreadCB->TimerQueue ;

        
        TimeRemaining = RtlpGetTimeRemaining (ThreadCB->TimerHandle) ;

        if (RtlpInsertInDeltaList (&ThreadCB->TimerQueue.TimerList, Wait->Timer,
                                    TimeRemaining, &NewFiringTime)) 
        {
            // If the element was inserted at head of list then reset the timers

            RtlpResetTimer (ThreadCB->TimerHandle, NewFiringTime, ThreadCB) ;
        }

    } else {

        // No timer with this wait

        Wait->Timer = NULL ;

    }

    return ;
}


NTSTATUS
RtlpDeregisterWait (
    PRTLP_WAIT Wait,
    HANDLE PartialCompletionEvent,
    PULONG RetStatusPtr
    )
/*++

Routine Description:

    This routine is used for deregistering the specified wait.

Arguments:

    Wait - The wait to deregister

Return Value:

--*/
{
    ULONG Status = STATUS_SUCCESS ;
    ULONG DontUse ;
    PULONG RetStatus = RetStatusPtr ? RetStatusPtr : &DontUse;
    
    CHECK_SIGNATURE(Wait) ;

    
    // RtlpDeregisterWait can be called on a wait that has not yet been
    // registered. This indicates that someone calls a RtlDeregisterWait
    // inside a WaitThreadCallback for a Wait other than that was fired.
    // Application bug!! I assert but handle it
    
    if ( ! (Wait->State & STATE_REGISTERED) ) {

        // set state to deleted, so that it does not get registered
        
        Wait->State |= STATE_DELETE ;
        
        InterlockedDecrement( &Wait->RefCount ) ;

        if ( PartialCompletionEvent ) {
        
            NtSetEvent( PartialCompletionEvent, NULL ) ;
        }

        *RetStatus = STATUS_SUCCESS ;
        return STATUS_SUCCESS ;
    }


    // deactivate wait.
    
    if ( Wait->State & STATE_ACTIVE ) {

        if ( ! NT_SUCCESS( RtlpDeactivateWait ( Wait ) ) ) {

            *RetStatus = STATUS_NOT_FOUND ;
            return STATUS_NOT_FOUND ;
        }
    }

    // delete wait if RefCount == 0

    Wait->State |= STATE_DELETE ;
    
    if ( InterlockedDecrement (&Wait->RefCount) == 0 ) {

        RtlpDeleteWait ( Wait ) ;

        Status = *RetStatus = STATUS_SUCCESS ;

    } else {

        Status = *RetStatus = STATUS_PENDING ;
    }

    if ( PartialCompletionEvent ) {
    
        NtSetEvent( PartialCompletionEvent, NULL ) ;
    }

        
    return Status ;
}


NTSTATUS
RtlpDeactivateWait (
    PRTLP_WAIT Wait
    )
/*++

Routine Description:

    This routine is used for deactivating the specified wait. It is executed in a APC.

Arguments:

    Wait - The wait to deactivate

Return Value:

--*/
{
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB = Wait->ThreadCB ;
    ULONG ArrayIndex ; //Index in ActiveWaitArray where the Wait object is placed
    ULONG EndIndex = ThreadCB->NumActiveWaits -1;

    // get the index in ActiveWaitArray
    
    for (ArrayIndex = 0;  ArrayIndex <= EndIndex; ArrayIndex++) {

        if (ThreadCB->ActiveWaitPointers[ArrayIndex] == Wait)
            break ;
    }

    if ( ArrayIndex > EndIndex ) {
    
        ASSERT (FALSE) ;
        return STATUS_NOT_FOUND;
    }


    // Move the remaining ActiveWaitArray left.

    RtlpShiftWaitArray( ThreadCB, ArrayIndex+1, ArrayIndex,
                    EndIndex - ArrayIndex ) ;


    //
    // delete timer if associated with this wait
    //
    // Though timer is being "freed" here, if it is in the timersToBeFired
    // list, some of its fields will be used later
    //
    
    if ( Wait->Timer ) {
    
        ULONG TimeRemaining ;
        ULONG NewFiringTime ;

        if (! (Wait->Timer->State & STATE_ACTIVE) ) {
        
            RemoveEntryList( &Wait->Timer->List ) ;

        } else {

            TimeRemaining = RtlpGetTimeRemaining (ThreadCB->TimerHandle) ;
            
            
            if (RtlpRemoveFromDeltaList (&ThreadCB->TimerQueue.TimerList, Wait->Timer, 
                                            TimeRemaining, &NewFiringTime)) 
            {

                RtlpResetTimer (ThreadCB->TimerHandle, NewFiringTime, ThreadCB) ;
            }
        }

        
        InsertTailList (&ThreadCB->FreeTimerBlocks, &Wait->Timer->List) ;

        Wait->Timer = NULL ;
    }

    // Decrement the (active) wait count

    ThreadCB->NumActiveWaits-- ;
    InterlockedDecrement( &ThreadCB->NumWaits ) ;
    
    Wait->State &= ~STATE_ACTIVE ;

    return STATUS_SUCCESS;

}


VOID
RtlpDeleteWait (
    PRTLP_WAIT Wait
    )
/*++

Routine Description:

    This routine is used for deleting the specified wait. It can be executed
    outside the context of the wait thread. So structure except the WaitEntry
    can be changed. It also sets the event.

Arguments:

    Wait - The wait to delete

Return Value:

--*/
{
    CHECK_SIGNATURE( Wait ) ;
    CLEAR_SIGNATURE( Wait ) ;

    #if DBG1
    if (DPRN1)
    DbgPrint("<%d> Wait %x deleted in thread:%d\n\n", Wait->DbgId, 
            (ULONG_PTR)Wait,
            HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread)) ;
    #endif

    
    if ( Wait->CompletionEvent ) {

        NtSetEvent( Wait->CompletionEvent, NULL ) ;
    }

    RtlpFreeTPHeap( Wait) ;

    return ;
}




VOID
RtlpDoNothing (
    PVOID NotUsed1,
    PVOID NotUsed2,
    PVOID NotUsed3
    )
/*++

Routine Description:

    This routine is used to see if the thread is alive

Arguments:

    NotUsed1, NotUsed2 and NotUsed 3 - not used

Return Value:

    None

--*/
{

}


__inline
LONGLONG
RtlpGet64BitTickCount(
    LARGE_INTEGER *Last64BitTickCount
    )
/*++

Routine Description:

    This routine is used for getting the latest 64bit tick count.

Arguments:

Return Value: 64bit tick count

--*/
{
    LARGE_INTEGER liCurTime ;

    liCurTime.QuadPart = NtGetTickCount() + Last64BitTickCount->HighPart ;

    // see if timer has wrapped.

    if (liCurTime.LowPart < Last64BitTickCount->LowPart) {
        liCurTime.HighPart++ ;
    }

    return (Last64BitTickCount->QuadPart = liCurTime.QuadPart) ;
}

__inline
LONGLONG
RtlpResync64BitTickCount(
    )
/*++

Routine Description:

    This routine is used for getting the latest 64bit tick count.

Arguments:

Return Value: 64bit tick count

Remarks: This call should be made in the first line of any APC queued
    to the timer thread and nowhere else. It is used to reduce the drift

--*/
{
    return Resync64BitTickCount.QuadPart = 
                    RtlpGet64BitTickCount(&Last64BitTickCount);
}


VOID
RtlpProcessTimeouts (
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB
    )
/*++

Routine Description:

    This routine processes timeouts for the wait thread

Arguments:

    ThreadCB - The wait thread to add the wait to

Return Value:

--*/
{
    ULONG NewFiringTime, TimeRemaining ;
    LIST_ENTRY TimersToFireList ;

    //
    // check if incorrect timer fired
    //
    if (ThreadCB->Firing64BitTickCount >
            RtlpGet64BitTickCount(&ThreadCB->Current64BitTickCount) + 200 )
    {
        RtlpResetTimer (ThreadCB->TimerHandle, 
                    RtlpGetTimeRemaining (ThreadCB->TimerHandle),
                    ThreadCB) ;

        return ;
    }
 
    InitializeListHead( &TimersToFireList ) ;

    
    // Walk thru the timer list and fire all waits with DeltaFiringTime == 0

    RtlpFireTimersAndReorder (&ThreadCB->TimerQueue, &NewFiringTime, &TimersToFireList) ;

    // Reset the NT timer

    RtlpResetTimer (ThreadCB->TimerHandle, NewFiringTime, ThreadCB) ;


    RtlpFireTimers( &TimersToFireList ) ;    
}


VOID
RtlpFireTimers (
    PLIST_ENTRY TimersToFireList
    )
/*++

Routine Description:

    Finally all the timers are fired here.

Arguments:

    TimersToFireList: List of timers to fire

--*/

{
    PLIST_ENTRY Node ;
    PRTLP_TIMER Timer ;
    NTSTATUS Status;

    
    for (Node = TimersToFireList->Flink;  Node != TimersToFireList; Node = TimersToFireList->Flink)
    {
        Timer = CONTAINING_RECORD (Node, RTLP_TIMER, TimersToFireList) ;

        RemoveEntryList( Node ) ;
        InitializeListHead( Node ) ;

        
        if ( (Timer->State & STATE_DONTFIRE) 
            || (Timer->Queue->State & STATE_DONTFIRE) )
        {
            ;

        } else if ( Timer->Flags & (WT_EXECUTEINTIMERTHREAD | WT_EXECUTEINWAITTHREAD ) ) {

            //DPRN5
            if (DPRN4)
            DbgPrint("Calling WaitOrTimer(Timer): fn:%x  context:%x  bool:%d Thread<%d:%d>\n",
                (ULONG_PTR)Timer->Function, (ULONG_PTR)Timer->Context,
                TRUE,
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)
                ) ;

            #if (DBG1)
            DBG_SET_FUNCTION( Timer->Function, Timer->Context ) ;
            #endif


            ((WAITORTIMERCALLBACKFUNC) Timer->Function) (Timer->Context, TRUE) ;

        } else {

            // create context for Callback and queue it appropriately
            
            PRTLP_ASYNC_CALLBACK  AsyncCallback ;
            
            AsyncCallback = RtlpForceAllocateTPHeap(
                                    sizeof( RTLP_ASYNC_CALLBACK ), 
                                    0 );
            AsyncCallback->TimerCondition = TRUE ;

            // timer associated with WaitEvents should be treated differently
            
            if ( Timer->Wait != NULL ) {

                AsyncCallback->Wait = Timer->Wait ;
                AsyncCallback->WaitThreadCallback = TRUE ;
                
                InterlockedIncrement( Timer->RefCountPtr ) ;

            } else {

                AsyncCallback->Timer = Timer ;
                AsyncCallback->WaitThreadCallback = FALSE ;

                InterlockedIncrement( &Timer->RefCount ) ;
            }


            // queue work item
            
            Status = RtlQueueWorkItem( RtlpAsyncCallbackCompletion, AsyncCallback, 
                                Timer->Flags );
                                
            if (!NT_SUCCESS(Status)) {

                RtlpFreeTPHeap( AsyncCallback );
                if ( Timer->Wait != NULL ) {
                    InterlockedDecrement( Timer->RefCountPtr ) ;
                } else {
                    InterlockedDecrement( &Timer->RefCount ) ;
                }
            }
            
        }

        
    }
}


NTSTATUS
RtlpFindWaitThread (
    PRTLP_WAIT_THREAD_CONTROL_BLOCK *ThreadCB
)
/*++

Routine Description:

    Walks thru the list of wait threads and finds one which can accomodate another wait.
    If one is not found then a new thread is created.

    This routine assumes that the caller has the GlobalWaitLock.

Arguments:

    ThreadCB: returns the ThreadCB of the wait thread that will service the wait request.

Return Value:

    STATUS_SUCCESS if a wait thread was allocated,

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLIST_ENTRY Node ;
    HANDLE ThreadHandle ;
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCBTmp;

    
    ACQUIRE_GLOBAL_WAIT_LOCK() ;

    do {

        // Walk thru the list of Wait Threads and find a Wait thread that can accomodate a
        // new wait request.

        // *Consider* finding a wait thread with least # of waits to facilitate better
        // load balancing of waits.


        for (Node = WaitThreads.Flink ; Node != &WaitThreads ; Node = Node->Flink) {

            ThreadCBTmp = CONTAINING_RECORD (Node, RTLP_WAIT_THREAD_CONTROL_BLOCK, 
                                            WaitThreadsList) ;


            // Wait Threads can accomodate upto 64 waits (NtWaitForMultipleObject limit)

            if ((ThreadCBTmp)->NumWaits < 64) {

                // Found a thread with some wait slots available.

                InterlockedIncrement ( &(ThreadCBTmp)->NumWaits) ;

                *ThreadCB = ThreadCBTmp;
                
                RELEASE_GLOBAL_WAIT_LOCK() ;
                
                return STATUS_SUCCESS ;
            }

        }


        // If we reach here, we dont have any more wait threads. so create a new wait thread.

        Status = RtlpStartThreadFunc (RtlpWaitThread, &ThreadHandle) ;


        // If thread creation fails then return the failure to caller

        if (Status != STATUS_SUCCESS ) {

            #if DBG
            DbgPrint("ERROR!! ThreadPool: could not create wait thread\n");
            #endif

            RELEASE_GLOBAL_WAIT_LOCK() ;
            
            return Status ;

        } else {

            // Close Thread handle, we dont need it.

            NtClose (ThreadHandle) ;
        }

        // Loop back now that we have created another thread

    } while (TRUE) ;    // Loop back to top and put new wait request in the newly created thread

    RELEASE_GLOBAL_WAIT_LOCK() ;
    
    return Status ;
}



// Timer Functions



VOID
RtlpAddTimer (
    PRTLP_TIMER Timer
    )
/*++

Routine Description:

    This routine runs as an APC into the Timer thread. It adds a new timer to the
    specified queue.

Arguments:

    Timer - Pointer to the timer to add

Return Value:


--*/
{
    PRTLP_TIMER_QUEUE Queue ;
    ULONG TimeRemaining, QueueRelTimeRemaining ;
    ULONG NewFiringTime ;

    RtlpResync64BitTickCount() ;

    
    // the timer was set to be deleted in a callback function.
    
    if (Timer->State & STATE_DELETE ) {
    
        RtlpDeleteTimer( Timer ) ;
        return ;
    }

    
    Queue = Timer->Queue ;

    
    // TimeRemaining is the time left in the current timer + the relative time of
    // the queue it is being inserted into

    TimeRemaining = RtlpGetTimeRemaining (TimerHandle) ;
    QueueRelTimeRemaining = TimeRemaining + RtlpGetQueueRelativeTime (Queue) ;


    if (RtlpInsertInDeltaList (&Queue->TimerList, Timer, QueueRelTimeRemaining, 
                                &NewFiringTime)) 
    {

        // If the Queue is not attached to TimerQueues since it had no timers 
        // previously then insert the queue into the TimerQueues list, else just 
        // reorder its existing position.

        if (IsListEmpty (&Queue->List)) {

            Queue->DeltaFiringTime = NewFiringTime ;

            if (RtlpInsertInDeltaList (&TimerQueues, Queue, TimeRemaining, 
                                        &NewFiringTime)) 
            {

                // There is a new element at the head of the queue we need to reset the NT
                // timer to fire sooner.

                RtlpResetTimer (TimerHandle, NewFiringTime, NULL) ;
            }

        } else {

            // If we insert at the head of the timer delta list we will need to
            // make sure the queue delta list is readjusted

            if (RtlpReOrderDeltaList(&TimerQueues, Queue, TimeRemaining, &NewFiringTime, NewFiringTime)){

                // There is a new element at the head of the queue we need to reset the NT
                // timer to fire sooner.

                RtlpResetTimer (TimerHandle, NewFiringTime, NULL) ;

            }
        }

    }

    Timer->State |= ( STATE_REGISTERED | STATE_ACTIVE ) ;
}



VOID
RtlpUpdateTimer (
    PRTLP_TIMER Timer,
    PRTLP_TIMER UpdatedTimer
    )
/*++

Routine Description:

    This routine executes in an APC and updates the specified timer if it exists

Arguments:

    Timer - Timer that is actually updated
    UpdatedTimer - Specifies pointer to a timer structure that contains Queue and
                Timer information

Return Value:


--*/
{
    PRTLP_TIMER_QUEUE Queue ;
    ULONG TimeRemaining, QueueRelTimeRemaining ;
    ULONG NewFiringTime ;


    RtlpResync64BitTickCount( ) ;

    CHECK_SIGNATURE(Timer) ;

    Queue = Timer->Queue ;

    // Update the periodic time on the timer

    Timer->Period = UpdatedTimer->Period ;


    // if timer is not in active state, then dont update it
    
    if ( ! ( Timer->State & STATE_ACTIVE ) ) {

        return ;
    }
        
    // Get the time remaining on the NT timer

    TimeRemaining = RtlpGetTimeRemaining (TimerHandle) ;
    QueueRelTimeRemaining = TimeRemaining + RtlpGetQueueRelativeTime (Queue) ;


    // Update the timer based on the due time
    
    if (RtlpReOrderDeltaList (&Queue->TimerList, Timer, QueueRelTimeRemaining, 
                                &NewFiringTime, 
                                UpdatedTimer->DeltaFiringTime)) 
    {

        // If this update caused the timer at the head of the queue to change, then reinsert
        // this queue in the list of queues.

        if (RtlpReOrderDeltaList (&TimerQueues, Queue, TimeRemaining, &NewFiringTime, NewFiringTime)) {

            // NT timer needs to be updated since the change caused the queue at the head of
            // the TimerQueues to change.

            RtlpResetTimer (TimerHandle, NewFiringTime, NULL) ;

        }

    }

    RtlpFreeTPHeap( UpdatedTimer ) ;
}


VOID
RtlpCancelTimer (
    PRTLP_TIMER Timer
    )
/*++

Routine Description:

    This routine executes in an APC and cancels the specified timer if it exists

Arguments:

    Timer - Specifies pointer to a timer structure that contains Queue and Timer information

Return Value:


--*/
{
    RtlpCancelTimerEx( Timer, FALSE ) ; // queue not being deleted
}

VOID
RtlpCancelTimerEx (
    PRTLP_TIMER Timer,
    BOOLEAN DeletingQueue
    )
/*++

Routine Description:

    This routine cancels the specified timer.

Arguments:

    Timer - Specifies pointer to a timer structure that contains Queue and Timer information
    DeletingQueue - FALSE: routine executing in an APC. Delete timer only.
                    TRUE : routine called by timer queue which is being deleted. So dont
                            reset the queue's position
Return Value:


--*/
{
    PRTLP_TIMER_QUEUE Queue ;

    RtlpResync64BitTickCount() ;
    CHECK_SIGNATURE( Timer ) ;
    
    Queue = Timer->Queue ;


    if ( Timer->State & STATE_ACTIVE ) {

        // if queue is being deleted, then the timer should not be reset
        
        if ( ! DeletingQueue )
            RtlpDeactivateTimer( Queue, Timer ) ;
        
    } else {

        // remove one shot Inactive timer from Queue->UncancelledTimerList
        // called only when the time queue is being deleted
        
        RemoveEntryList( &Timer->List ) ;

    }

    
    // Set the State to deleted
    
    Timer->State |= STATE_DELETE ;


    // delete timer if refcount == 0
    
    if ( InterlockedDecrement( &Timer->RefCount ) == 0 ) {
    
        RtlpDeleteTimer( Timer ) ;
    }
}

VOID
RtlpDeactivateTimer (
    PRTLP_TIMER_QUEUE Queue,
    PRTLP_TIMER Timer
    )
/*++

Routine Description:

    This routine executes in an APC and cancels the specified timer if it exists

Arguments:

    Timer - Specifies pointer to a timer structure that contains Queue and Timer information

Return Value:


--*/
{
    ULONG TimeRemaining, QueueRelTimeRemaining ;
    ULONG NewFiringTime ;

    
    // Remove the timer from the appropriate queue

    TimeRemaining = RtlpGetTimeRemaining (TimerHandle) ;
    QueueRelTimeRemaining = TimeRemaining + RtlpGetQueueRelativeTime (Queue) ;

    if (RtlpRemoveFromDeltaList (&Queue->TimerList, Timer, QueueRelTimeRemaining, &NewFiringTime)) {

        // If we removed the last timer from the queue then we should remove the queue
        // from TimerQueues, else we should readjust its position based on the delta time change

        if (IsListEmpty (&Queue->TimerList)) {

            // Remove the queue from TimerQueues

            if (RtlpRemoveFromDeltaList (&TimerQueues, Queue, TimeRemaining, &NewFiringTime)) {

                // There is a new element at the head of the queue we need to reset the NT
                // timer to fire later

                RtlpResetTimer (TimerHandle, NewFiringTime, NULL) ;

            }

            InitializeListHead (&Queue->List) ;

        } else {

            // If we remove from the head of the timer delta list we will need to
            // make sure the queue delta list is readjusted

            if (RtlpReOrderDeltaList (&TimerQueues, Queue, TimeRemaining, &NewFiringTime, NewFiringTime)) {

                // There is a new element at the head of the queue we need to reset the NT
                // timer to fire later

                RtlpResetTimer (TimerHandle, NewFiringTime, NULL) ;

            }

        }

    }
}


VOID
RtlpDeleteTimer (
    PRTLP_TIMER Timer
    )
/*++

Routine Description:

    This routine executes in worker or timer thread and deletes the timer 
    whose RefCount == 0. The function can be called outside timer thread,
    so no structure outside Timer can be touched (no list etc).

Arguments:

    Timer - Specifies pointer to a timer structure that contains Queue and Timer information

Return Value:


--*/
{
    PRTLP_TIMER_QUEUE Queue = Timer->Queue ;

    CHECK_SIGNATURE( Timer ) ;
    CLEAR_SIGNATURE( Timer ) ;

    #if DBG1
    if (DPRN1)
    DbgPrint("<%d> Timer: %x: deleted\n\n", Timer->Queue->DbgId, 
            (ULONG_PTR)Timer) ;
    #endif

    // safe to call this. Either the timer is in the TimersToFireList and
    // the function is being called in time context or else it is not in the
    // list

    RemoveEntryList( &Timer->TimersToFireList ) ;

    if ( Timer->CompletionEvent )
        NtSetEvent( Timer->CompletionEvent, NULL ) ;


    // decrement the total number of timers in the queue
    
    if ( InterlockedDecrement( &Queue->RefCount ) == 0 )

        RtlpDeleteTimerQueueComplete( Queue ) ;
        

    RtlpFreeTPHeap( Timer ) ;

}



ULONG
RtlpGetQueueRelativeTime (
    PRTLP_TIMER_QUEUE Queue
    )
/*++

Routine Description:

    Walks the list of queues and returns the relative firing time by adding all the
    DeltaFiringTimes for all queues before it.

Arguments:

    Queue - Queue for which to find the relative firing time

Return Value:

    Time in milliseconds

--*/
{
    PLIST_ENTRY Node ;
    ULONG RelativeTime ;
    PRTLP_TIMER_QUEUE CurrentQueue ;

    RelativeTime = 0 ;

    // It the Queue is not attached to TimerQueues List because it has no timer
    // associated with it simply returns 0 as the relative time. Else run thru
    // all queues before it in the list and compute the relative firing time

    if (!IsListEmpty (&Queue->List)) {

        for (Node = TimerQueues.Flink; Node != &Queue->List; Node=Node->Flink) {

            CurrentQueue = CONTAINING_RECORD (Node, RTLP_TIMER_QUEUE, List) ;

            RelativeTime += CurrentQueue->DeltaFiringTime ;

        }

        // Add the queue's delta firing time as well

        RelativeTime += Queue->DeltaFiringTime ;
        
    }

    return RelativeTime ;

}


ULONG
RtlpGetTimeRemaining (
    HANDLE TimerHandle
    )
/*++

Routine Description:

    Gets the time remaining on the specified NT timer

Arguments:

    TimerHandle - Handle to the NT timer

Return Value:

    Time remaining on the timer

--*/
{
    ULONG InfoLen ;
    TIMER_BASIC_INFORMATION Info ;

    NTSTATUS Status ;

    Status = NtQueryTimer (TimerHandle, TimerBasicInformation, &Info, sizeof(Info), &InfoLen) ;

    ASSERT (Status == STATUS_SUCCESS) ;

    // Return 0 if
    //
    // - the timer has already fired then return
    //   OR
    // - the timer is has more than 0x7f0000000 in its high part
    //   (which indicates that the timer was (probably) programmed for -1)

    
    if (Info.TimerState || ((ULONG)Info.RemainingTime.HighPart > 0x7f000000) ) {

        return 0 ;

    } else {

        return (ULONG) (Info.RemainingTime.QuadPart / 10000) ;

    }

}



VOID
RtlpResetTimer (
    HANDLE TimerHandle,
    ULONG DueTime,
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB
    )
/*++

Routine Description:

    This routine resets the timer object with the new due time.

Arguments:

    TimerHandle - Handle to the timer object

    DueTime - Relative timer due time in Milliseconds

Return Value:

--*/
{
    LARGE_INTEGER LongDueTime ;

    NtCancelTimer (TimerHandle, NULL) ;

    // If the DueTime is INFINITE_TIME then set the timer to the largest integer possible

    if (DueTime == INFINITE_TIME) {

        LongDueTime.LowPart = 0x0 ;

        LongDueTime.HighPart = 0x80000000 ;

    } else {

        //
        // set the absolute time when timer is to be fired
        //
        
        if (ThreadCB) {
        
            ThreadCB->Firing64BitTickCount = DueTime 
                                + RtlpGet64BitTickCount(&ThreadCB->Current64BitTickCount) ;

        } else {
            //
            // adjust for drift only if it is a global timer
            //
        
            ULONG Drift ;
            LONGLONG llCurrentTick ;
            
            llCurrentTick = RtlpGet64BitTickCount(&Last64BitTickCount) ;
            
            Drift = (ULONG) (llCurrentTick - RtlpGetResync64BitTickCount()) ;
            DueTime = (DueTime > Drift) ? DueTime-Drift : 1 ;
            RtlpSetFiring64BitTickCount(llCurrentTick + DueTime) ;
        }

        
        LongDueTime.QuadPart = Int32x32To64( DueTime, -10000 );

    }
    

    NtSetTimer (
        TimerHandle,
        &LongDueTime,
        ThreadCB ? NULL : RtlpServiceTimer,
        NULL,
        FALSE,
        0,
        NULL
        ) ;
}


BOOLEAN
RtlpInsertInDeltaList (
    PLIST_ENTRY DeltaList,
    PRTLP_GENERIC_TIMER NewTimer,
    ULONG TimeRemaining,
    ULONG *NewFiringTime
    )
/*++

Routine Description:

    Inserts the timer element in the appropriate place in the delta list.

Arguments:

    DeltaList - Delta list to insert into

    NewTimer - Timer element to insert into list

    TimeRemaining - This time must be added to the head of the list to get "real"
                    relative time.

    NewFiringTime - If the new element was inserted at the head of the list - this
                    will contain the new firing time in milliseconds. The caller
                    can use this time to re-program the NT timer. This MUST NOT be
                    changed if the function returns FALSE.

Return Value:

    TRUE - If the timer was inserted at head of delta list

    FALSE - otherwise

--*/
{
    PLIST_ENTRY Node ;
    PRTLP_GENERIC_TIMER Temp ;
    PRTLP_GENERIC_TIMER Head ;

    if (IsListEmpty (DeltaList)) {

        InsertHeadList (DeltaList, &NewTimer->List) ;

        *NewFiringTime = NewTimer->DeltaFiringTime ;
        
        NewTimer->DeltaFiringTime = 0 ;

        return TRUE ;

    }

    // Adjust the head of the list to reflect the time remaining on the NT timer

    Head = CONTAINING_RECORD (DeltaList->Flink, RTLP_GENERIC_TIMER, List) ;

    Head->DeltaFiringTime += TimeRemaining ;


    // Find the appropriate location to insert this element in

    for (Node = DeltaList->Flink ; Node != DeltaList ; Node = Node->Flink) {

        Temp = CONTAINING_RECORD (Node, RTLP_GENERIC_TIMER, List) ;


        if (Temp->DeltaFiringTime <= NewTimer->DeltaFiringTime) {

            NewTimer->DeltaFiringTime -= Temp->DeltaFiringTime ;

        } else {

            // found appropriate place to insert this timer

            break ;

        }

    }

    // Either we have found the appopriate node to insert before in terms of deltas.
    // OR we have come to the end of the list. Insert this timer here.

    InsertHeadList (Node->Blink, &NewTimer->List) ;


    // If this isnt the last element in the list - adjust the delta of the
    // next element

    if (Node != DeltaList) {

        Temp->DeltaFiringTime -= NewTimer->DeltaFiringTime ;

    }


    // Check if element was inserted at head of list

    if (DeltaList->Flink == &NewTimer->List) {

        // Set NewFiringTime to the time in milliseconds when the new head of list
        // should be serviced.

        *NewFiringTime = NewTimer->DeltaFiringTime ;

        // This means the timer must be programmed to service this request

        NewTimer->DeltaFiringTime = 0 ;

        return TRUE ;

    } else {

        // No change to the head of the list, set the delta time back

        Head->DeltaFiringTime -= TimeRemaining ;

        return FALSE ;

    }

}



BOOLEAN
RtlpRemoveFromDeltaList (
    PLIST_ENTRY DeltaList,
    PRTLP_GENERIC_TIMER Timer,
    ULONG TimeRemaining,
    ULONG* NewFiringTime
    )
/*++

Routine Description:

    Removes the specified timer from the delta list

Arguments:

    DeltaList - Delta list to insert into

    Timer - Timer element to insert into list

    TimerHandle - Handle of the NT Timer object

    TimeRemaining - This time must be added to the head of the list to get "real"
                    relative time.

Return Value:

    TRUE if the timer was removed from head of timer list
    FALSE otherwise

--*/
{
    PLIST_ENTRY Next ;
    PRTLP_GENERIC_TIMER Temp ;

    Next = Timer->List.Flink ;

    RemoveEntryList (&Timer->List) ;

    if (IsListEmpty (DeltaList)) {

        *NewFiringTime = INFINITE_TIME ;

        return TRUE ;

    }

    if (Next == DeltaList)  {

        // If we removed the last element in the list nothing to do either

        return FALSE ;

    } else {

        Temp = CONTAINING_RECORD ( Next, RTLP_GENERIC_TIMER, List) ;

        Temp->DeltaFiringTime += Timer->DeltaFiringTime ;
        
        // Check if element was removed from head of list

        if (DeltaList->Flink == Next) {

            *NewFiringTime = Temp->DeltaFiringTime + TimeRemaining ;

            Temp->DeltaFiringTime = 0 ;

            return TRUE ;

        } else {

            return FALSE ;

        }

    }

}



BOOLEAN
RtlpReOrderDeltaList (
    PLIST_ENTRY DeltaList,
    PRTLP_GENERIC_TIMER Timer,
    ULONG TimeRemaining,
    ULONG *NewFiringTime,
    ULONG ChangedFiringTime
    )
/*++

Routine Description:

    Called when a timer in the delta list needs to be re-inserted because the firing time
    has changed.

Arguments:

    DeltaList - List in which to re-insert

    Timer - Timer for which the firing time has changed

    TimeRemaining - Time before the head of the delta list is fired

    NewFiringTime - If the new element was inserted at the head of the list - this
                    will contain the new firing time in milliseconds. The caller
                    can use this time to re-program the NT timer.

    ChangedFiringTime - Changed Time for the specified timer.

Return Value:

    TRUE if the timer was removed from head of timer list
    FALSE otherwise

--*/
{
    ULONG NewTimeRemaining ;
    PRTLP_GENERIC_TIMER Temp ;

    // Remove the timer from the list

    if (RtlpRemoveFromDeltaList (DeltaList, Timer, TimeRemaining, NewFiringTime)) {

        // If element was removed from the head of the list we should record that

        NewTimeRemaining = *NewFiringTime ;


    } else {

        // Element was not removed from head of delta list, the current TimeRemaining is valid

        NewTimeRemaining = TimeRemaining ;

    }

    // Before inserting Timer, set its delta time to the ChangedFiringTime

    Timer->DeltaFiringTime = ChangedFiringTime ;

    // Reinsert this element back in the list

    if (!RtlpInsertInDeltaList (DeltaList, Timer, NewTimeRemaining, NewFiringTime)) {

        // If we did not add at the head of the list, then we should return TRUE if
        // RtlpRemoveFromDeltaList() had returned TRUE. We also update the NewFiringTime to
        // the reflect the new firing time returned by RtlpRemoveFromDeltaList()

        *NewFiringTime = NewTimeRemaining ;

        return (NewTimeRemaining != TimeRemaining) ;

    } else {

        // NewFiringTime contains the time the NT timer must be programmed for

        return TRUE ;

    }

}


VOID
RtlpAddTimerQueue (
    PVOID Queue
    )
/*++

Routine Description:

    This routine runs as an APC into the Timer thread. It does whatever necessary to
    create a new timer queue

Arguments:

    Queue - Pointer to the queue to add

Return Value:


--*/
{

    // We do nothing here. The newly created queue is free floating until a timer is
    // queued onto it.

}


VOID
RtlpServiceTimer (
    PVOID NotUsedArg,
    ULONG NotUsedLowTimer,
    LONG NotUsedHighTimer
    )
/*++

Routine Description:

    Services the timer. Runs in an APC.

Arguments:

    NotUsedArg - Argument is not used in this function.

    NotUsedLowTimer - Argument is not used in this function.

    NotUsedHighTimer - Argument is not used in this function.

Return Value:

Remarks:
    This APC is called only for timeouts of timer threads.
    
--*/
{
    PRTLP_TIMER Timer ;
    PRTLP_TIMER_QUEUE Queue ;
    PLIST_ENTRY TNode ;
    PLIST_ENTRY QNode ;
    PLIST_ENTRY Temp ;
    ULONG NewFiringTime ;
    LIST_ENTRY ReinsertTimerQueueList ;
    LIST_ENTRY TimersToFireList ;

    RtlpResync64BitTickCount() ;
    
    if (DPRN2) {
        DbgPrint("Before service timer ThreadId<%x:%x>\n",
                    HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
                    HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess));
        RtlDebugPrintTimes ();
    }

    ACQUIRE_GLOBAL_TIMER_LOCK();

    // fire it if it even 200ms ahead. else reset the timer
    
    if (Firing64BitTickCount.QuadPart > RtlpGet64BitTickCount(&Last64BitTickCount) + 200) {

        RtlpResetTimer (TimerHandle, RtlpGetTimeRemaining (TimerHandle), NULL) ;

        RELEASE_GLOBAL_TIMER_LOCK() ;
        return ;
    }
        
    InitializeListHead (&ReinsertTimerQueueList) ;

    InitializeListHead (&TimersToFireList) ;


    // We run thru all queues with DeltaFiringTime == 0 and fire all timers that
    // have DeltaFiringTime == 0. We remove the fired timers and either free them
    // (for one shot timers) or put them in aside list (for periodic timers).
    // After we have finished firing all timers in a queue we reinsert the timers
    // in the aside list back into the queue based on their new firing times.
    //
    // Similarly, we remove each fired Queue and put it in a aside list. After firing
    // all queues with DeltaFiringTime == 0, we reinsert the Queues in the aside list
    // and reprogram the NT timer to be the firing time of the first queue in the list


    for (QNode = TimerQueues.Flink ; QNode != &TimerQueues ; QNode = QNode->Flink) {

        Queue = CONTAINING_RECORD (QNode, RTLP_TIMER_QUEUE, List) ;

        // If the delta time in the timer queue is 0 - then this queue
        // has timers that are ready to fire. Walk the list and fire all timers with
        // Delta time of 0

        if (Queue->DeltaFiringTime == 0) {

            // Walk all timers with DeltaFiringTime == 0 and fire them. After that
            // reinsert the periodic timers in the appropriate place.

            RtlpFireTimersAndReorder (Queue, &NewFiringTime, &TimersToFireList) ;

            // detach this Queue from the list

            QNode = QNode->Blink ;

            RemoveEntryList (QNode->Flink) ;

            // If there are timers in the queue then prepare to reinsert the queue in
            // TimerQueues.

            if (NewFiringTime != INFINITE_TIME) {

                Queue->DeltaFiringTime = NewFiringTime ;

                // put the timer in list that we will process after we have
                // fired all elements in this queue

                InsertHeadList (&ReinsertTimerQueueList, &Queue->List) ;

            } else {

                // Queue has no more timers in it. Let the Queue float.

                InitializeListHead (&Queue->List) ;

            }


        } else {

            // No more Queues with DeltaFiringTime == 0

            break ;

        }

    }

    // At this point we have fired all the ready timers. We have two lists that need to be
    // merged - TimerQueues and ReinsertTimerQueueList. The following steps do this - at the
    // end of this we will reprogram the NT Timer.

    if (!IsListEmpty(&TimerQueues)) {

        Queue = CONTAINING_RECORD (TimerQueues.Flink, RTLP_TIMER_QUEUE, List) ;

        NewFiringTime = Queue->DeltaFiringTime ;

        Queue->DeltaFiringTime = 0 ;

        if (!IsListEmpty (&ReinsertTimerQueueList)) {

            // TimerQueues and ReinsertTimerQueueList are both non-empty. Merge them.

            RtlpInsertTimersIntoDeltaList (&ReinsertTimerQueueList, &TimerQueues, 
                                            NewFiringTime, &NewFiringTime) ;

        }

        // NewFiringTime contains the time the NT Timer should be programmed to.

    } else {

        if (!IsListEmpty (&ReinsertTimerQueueList)) {

            // TimerQueues is empty. ReinsertTimerQueueList is not.

            RtlpInsertTimersIntoDeltaList (&ReinsertTimerQueueList, &TimerQueues, 0, 
                                            &NewFiringTime) ;

        } else {

            NewFiringTime = INFINITE_TIME ;

        }

        // NewFiringTime contains the time the NT Timer should be programmed to.

    }


    // Reset the timer to reflect the Delta time associated with the first Queue

    RtlpResetTimer (TimerHandle, NewFiringTime, NULL) ;

    if (DPRN3) {
        DbgPrint("After service timer:ThreadId<%x:%x>\n",
                    HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
                    HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess));
        RtlDebugPrintTimes ();
    }

    
    // finally fire all the timers
    
    RtlpFireTimers( &TimersToFireList ) ;

    RELEASE_GLOBAL_TIMER_LOCK();

}


VOID
RtlpFireTimersAndReorder (
    PRTLP_TIMER_QUEUE Queue,
    ULONG *NewFiringTime,
    PLIST_ENTRY TimersToFireList
    )
/*++

Routine Description:

    Fires all timers in TimerList that have DeltaFiringTime == 0. After firing the timers
    it reorders the timers based on their periodic times OR frees the fired one shot timers.

Arguments:

    TimerList - Timer list to work thru.

    NewFiringTime - Location where the new firing time for the first timer in the delta list
                    is returned.


Return Value:


--*/
{
    PLIST_ENTRY TNode ;
    PRTLP_TIMER Timer ;
    LIST_ENTRY ReinsertTimerList ;
    PLIST_ENTRY TimerList = &Queue->TimerList ;
    
    InitializeListHead (&ReinsertTimerList) ;
    *NewFiringTime = 0 ;
    

    for (TNode = TimerList->Flink ; (TNode != TimerList) && (*NewFiringTime == 0); 
            TNode = TimerList->Flink) 
    {

        Timer = CONTAINING_RECORD (TNode, RTLP_TIMER, List) ;

        // Fire all timers with delta time of 0

        if (Timer->DeltaFiringTime == 0) {

            // detach this timer from the list

            RemoveEntryList (TNode) ;

            // get next firing time
            
            if (!IsListEmpty(TimerList)) {

                PRTLP_TIMER TmpTimer ;

                TmpTimer = CONTAINING_RECORD (TimerList->Flink, RTLP_TIMER, List) ;

                *NewFiringTime  = TmpTimer->DeltaFiringTime ;

                TmpTimer->DeltaFiringTime = 0 ;

            } else {

                *NewFiringTime = INFINITE_TIME ;
            }


            // if timer is not periodic then remove active state. Timer will be deleted
            // when cancel timer is called.

            if (Timer->Period == 0) {

                InsertHeadList( &Queue->UncancelledTimerList, &Timer->List ) ;

                // if one shot wait was timed out then, deactivate the wait
                
                if ( Timer->Wait ) {

                    //
                    // though timer is being "freed" in this call, it can still be
                    // used after this call
                    //
                    
                    RtlpDeactivateWait( Timer->Wait ) ;
                }

                else {
                    // should be set at the end
                
                    Timer->State &= ~STATE_ACTIVE ;
                }

                Timer->State |= STATE_ONE_SHOT_FIRED ;
                
            } else {

                // Set the DeltaFiringTime to be the next period

                Timer->DeltaFiringTime = Timer->Period ;

                // reinsert the timer in the list.
                
                RtlpInsertInDeltaList (TimerList, Timer, *NewFiringTime, NewFiringTime) ;
            }


            // Call the function associated with this timer. call it in the end
            // so that RtlTimer calls can be made in the timer function

            if ( (Timer->State & STATE_DONTFIRE) 
                || (Timer->Queue->State & STATE_DONTFIRE) )
            {
                ;

            } else {

                InsertTailList( TimersToFireList, &Timer->TimersToFireList ) ;

            }

        } else {

            // No more Timers with DeltaFiringTime == 0

            break ;

        }
    }


    if ( *NewFiringTime == 0 ) {
        *NewFiringTime = INFINITE_TIME ;
    }
}


VOID
RtlpInsertTimersIntoDeltaList (
    IN PLIST_ENTRY NewTimerList,
    IN PLIST_ENTRY DeltaTimerList,
    IN ULONG TimeRemaining,
    OUT ULONG *NewFiringTime
    )
/*++

Routine Description:

    This routine walks thru a list of timers in NewTimerList and inserts them into a delta
    timers list pointed to by DeltaTimerList. The timeout associated with the first element
    in the new list is returned in NewFiringTime.

Arguments:

    NewTimerList - List of timers that need to be inserted into the DeltaTimerList

    DeltaTimerList - Existing delta list of zero or more timers.

    TimeRemaining - Firing time of the first element in the DeltaTimerList

    NewFiringTime - Location where the new firing time will be returned

Return Value:


--*/
{
    PRTLP_GENERIC_TIMER Timer ;
    PLIST_ENTRY TNode ;
    PLIST_ENTRY Temp ;

    for (TNode = NewTimerList->Flink ; TNode != NewTimerList ; TNode = TNode->Flink) {

        Temp = TNode->Blink ;

        RemoveEntryList (Temp->Flink) ;

        Timer = CONTAINING_RECORD (TNode, RTLP_GENERIC_TIMER, List) ;

        if (RtlpInsertInDeltaList (DeltaTimerList, Timer, TimeRemaining, NewFiringTime)) {

            TimeRemaining = *NewFiringTime ;

        }

        TNode = Temp ;

    }

}


LONG
RtlpTimerThread (
    PVOID  Initialized
    )
/*++

Routine Description:

    All the timer activity takes place in APCs.

Arguments:

    Initialized - Used to notify the starter of the thread that thread initialization 
    has completed

Return Value:

--*/
{
    LARGE_INTEGER TimeOut ;

    // no structure initializations should be done here as new timer thread
    // may be created after threadPoolCleanup
    

    TimerThreadId = HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread) ;


    // Reset the NT Timer to never fire initially

    RtlpResetTimer (TimerHandle, -1, NULL) ;

    // Notify starter of this thread that it has initialized

    InterlockedExchange ((ULONG *) Initialized, 1) ;

    // Sleep alertably so that all the activity can take place
    // in APCs

    for ( ; ; ) {

        // Set timeout for the largest timeout possible

        TimeOut.LowPart = 0 ;
        TimeOut.HighPart = 0x80000000 ;

        NtDelayExecution (TRUE, &TimeOut) ;

    }

    return 0 ;  // Keep compiler happy

}


NTSTATUS
RtlpInitializeTimerThreadPool (
    )
/*++

Routine Description:

    This routine is used to initialize structures used for Timer Thread

Arguments:


Return Value:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    LARGE_INTEGER TimeOut ;

    // In order to avoid an explicit RtlInitialize() function to initialize the wait thread pool
    // we use StartedTimerInitialization and CompletedTimerInitialization to provide us the
    // necessary synchronization to avoid multiple threads from initializing the thread pool.
    // This scheme does not work if RtlInitializeCriticalSection() or NtCreateEvent fails - but in this case the
    // caller has not choices left.

    if (!InterlockedExchange(&StartedTimerInitialization, 1L)) {

        if (CompletedTimerInitialization)
            InterlockedExchange(&CompletedTimerInitialization, 0 ) ;

        do {

            // Initialize global timer lock

            Status = RtlInitializeCriticalSection( &TimerCriticalSection ) ;
            if (! NT_SUCCESS( Status )) {
                break ;
            }

            Status = NtCreateTimer(
                                &TimerHandle,
                                TIMER_ALL_ACCESS,
                                NULL,
                                NotificationTimer
                                ) ;

            if (!NT_SUCCESS(Status) )
                break ;
                
            InitializeListHead (&TimerQueues) ; // Initialize Timer Queue Structures


            // initialize tick count

            Resync64BitTickCount.QuadPart = NtGetTickCount()  ;
            Firing64BitTickCount.QuadPart = 0 ;

            
            Status = RtlpStartThreadFunc (RtlpTimerThread, &TimerThreadHandle) ;

            if (!NT_SUCCESS(Status) )
                break ;


        } while(FALSE ) ;

        if (!NT_SUCCESS(Status) ) {

            ASSERT (Status == STATUS_SUCCESS) ;
            
            StartedTimerInitialization = 0 ;
            InterlockedExchange (&CompletedTimerInitialization, ~0) ;
            
            return Status ;
        }
        
        InterlockedExchange (&CompletedTimerInitialization, 1L) ;

    } else {

        // Sleep 1 ms and see if the other thread has completed initialization

        ONE_MILLISECOND_TIMEOUT(TimeOut) ;

        while (!(volatile ULONG) CompletedTimerInitialization) {

            NtDelayExecution (FALSE, &TimeOut) ;

        }
        
        if (CompletedTimerInitialization != 1)
            Status = STATUS_NO_MEMORY ;
    }

    return NT_SUCCESS(Status) ? STATUS_SUCCESS : Status ;
}


NTSTATUS
RtlpDeleteTimerQueue (
    PRTLP_TIMER_QUEUE Queue
    )
/*++

Routine Description:

    This routine deletes the queue specified in the Request and frees all timers

Arguments:

    Queue - queue to delete

    Event - Event Handle used for signalling completion of request

Return Value:

--*/
{
    ULONG TimeRemaining ;
    ULONG NewFiringTime ;
    PLIST_ENTRY Node ;
    PRTLP_TIMER Timer ;

    RtlpResync64BitTickCount() ;

    SET_DEL_TIMERQ_SIGNATURE( Queue ) ;

    
    // If there are no timers in the queue then it is not attached to TimerQueues
    // In this case simply free the memory and return. Otherwise we have to first
    // remove the queue from the TimerQueues List, update the firing time if this
    // was the first queue in the list and then walk all the timers and free them
    // before freeing the Timer Queue.

    if (!IsListEmpty (&Queue->List)) {

        TimeRemaining = RtlpGetTimeRemaining (TimerHandle) 
                        + RtlpGetQueueRelativeTime (Queue) ;

        if (RtlpRemoveFromDeltaList (&TimerQueues, Queue, TimeRemaining, 
                                    &NewFiringTime)) 
        {

            // If removed from head of queue list, reset the timer

            RtlpResetTimer (TimerHandle, NewFiringTime, NULL) ;
        }


        // Free all the timers associated with this queue

        for (Node = Queue->TimerList.Flink ; Node != &Queue->TimerList ; ) {

            Timer =  CONTAINING_RECORD (Node, RTLP_TIMER, List) ;
            
            Node = Node->Flink ;

            RtlpCancelTimerEx( Timer ,TRUE ) ; // Queue being deleted
        }
    }


    // Free all the uncancelled one shot timers in this queue

    for (Node = Queue->UncancelledTimerList.Flink ; Node != &Queue->UncancelledTimerList ; ) {

        Timer =  CONTAINING_RECORD (Node, RTLP_TIMER, List) ;
        
        Node = Node->Flink ;

        RtlpCancelTimerEx( Timer ,TRUE ) ; // Queue being deleted
    }


    // delete the queue completely if the RefCount is 0
    
    if ( InterlockedDecrement( &Queue->RefCount ) == 0 ) {
    
        RtlpDeleteTimerQueueComplete( Queue ) ;

        return STATUS_SUCCESS ;
        
    } else {
    
        return STATUS_PENDING ;
    }

}



/*++

Routine Description:

    This routine frees the queue and sets the event.

Arguments:

    Queue - queue to delete

    Event - Event Handle used for signalling completion of request

Return Value:

--*/
VOID
RtlpDeleteTimerQueueComplete (
    PRTLP_TIMER_QUEUE Queue
    )
{
    #if DBG1
    if (DPRN1)
    DbgPrint("<%d> Queue: %x: deleted\n\n", Queue->DbgId, 
            (ULONG_PTR)Queue) ;
    #endif

    InterlockedDecrement( &NumTimerQueues ) ;    

    // Notify the thread issuing the cancel that the request is completed

    if ( Queue->CompletionEvent )
        NtSetEvent (Queue->CompletionEvent, NULL) ;
    
    RtlpFreeTPHeap( Queue ) ;
}


VOID
RtlpThreadCleanup (
    )
/*++

Routine Description:

    This routine is used for exiting timer, wait and IOworker threads.

Arguments:

Return Value:

--*/
{
    NtTerminateThread( NtCurrentThread(), 0) ;
}


NTSTATUS
RtlpWaitForEvent (
    HANDLE Event,
    HANDLE ThreadHandle
    )
/*++

Routine Description:

    Waits for the event to be signalled. If the event is not signalled within
    one second, then checks to see that the thread is alive

Arguments:

    Event : Event handle used for signalling completion of request

    ThreadHandle: Thread to check whether still alive

Return Value:

    STATUS_SUCCESS if event was signalled
    else return NTSTATUS

--*/
{
    NTSTATUS Status ;
    LARGE_INTEGER TimeOut ;
    
    // Timeout of 1 second for request to complete
    ONE_SECOND_TIMEOUT(TimeOut) ;

Wait:

    Status = NtWaitForSingleObject (Event, FALSE, &TimeOut) ;
    
    if (Status == STATUS_TIMEOUT) {

        // The wait timed out. Check to see if the wait thread is still alive.
        // This is done by trying to queue a dummy APC to it. There is no better
        // way known to determine if the thread has died unexpectedly.
        // If so then go back to waiting.

        Status = NtQueueApcThread(
            ThreadHandle,
            (PPS_APC_ROUTINE)RtlpDoNothing,
            NULL,
            NULL,
            NULL
            );

        if (NT_SUCCESS(Status) ) {

            // Wait thread is still alive. Go back to waiting.

            goto Wait ;

        } else {

            // The wait thread died between the time the APC was queued and the
            // the time we started waiting on NtQueryInformationThread()


            DbgPrint ("Thread died before event could be signalled") ;

        }

    }

    return NT_SUCCESS(Status) ? STATUS_SUCCESS : Status ;
}


PRTLP_EVENT
RtlpGetWaitEvent (
    VOID
    )
/*++

Routine Description:

    Returns an event from the event cache.

Arguments:

    None

Return Value:

    Pointer to event structure

--*/
{
    NTSTATUS Status;
    PRTLP_EVENT Event ;

    if (!CompletedEventCacheInitialization) {

        RtlpInitializeEventCache () ;

    }

    RtlEnterCriticalSection (&EventCacheCriticalSection) ;

    if (!IsListEmpty (&EventCache)) {

        Event = (PRTLP_EVENT) RemoveHeadList (&EventCache) ;

    } else {

        Event = RtlpForceAllocateTPHeap( sizeof( RTLP_EVENT ), 0 );

        if (!Event) {

            RtlLeaveCriticalSection (&EventCacheCriticalSection) ;

            return NULL ;

        }

        Status = NtCreateEvent(
                    &Event->Handle,
                    EVENT_ALL_ACCESS,
                    NULL,
                    SynchronizationEvent,
                    FALSE
                    );

        if (!NT_SUCCESS(Status) ) {

            RtlpFreeTPHeap( Event ) ;

            RtlLeaveCriticalSection (&EventCacheCriticalSection) ;

            return NULL ;

        }

    }

    RtlLeaveCriticalSection (&EventCacheCriticalSection) ;

    return Event ;
}


VOID
RtlpFreeWaitEvent (
    PRTLP_EVENT Event
    )
/*++

Routine Description:

    Frees the event to the event cache

Arguments:

    Event - the event struct to put back into the cache

Return Value:

    Nothing

--*/
{

    if ( Event == NULL )
        return ;
        
    InitializeListHead (&Event->List) ;

    RtlEnterCriticalSection (&EventCacheCriticalSection) ;

    if ( NumUnusedEvents > MAX_UNUSED_EVENTS ) {

        NtClose( Event->Handle ) ;
        
        RtlpFreeTPHeap( Event ) ;

    } else {
    
        InsertHeadList (&EventCache, &Event->List) ;
        NumUnusedEvents++ ;
    }

    
    RtlLeaveCriticalSection (&EventCacheCriticalSection) ;
}



VOID
RtlpInitializeEventCache (
    VOID
    )
/*++

Routine Description:

    Initializes the event cache

Arguments:

    None

Return Value:

    Nothing

--*/
{
    NTSTATUS Status;
    LARGE_INTEGER TimeOut ;

    if (!InterlockedExchange(&StartedEventCacheInitialization, 1L)) {

        InitializeListHead (&EventCache) ;

        Status = RtlInitializeCriticalSection(&EventCacheCriticalSection) ;

        ASSERT (Status == STATUS_SUCCESS) ;

        NumUnusedEvents = 0 ;
        
        InterlockedExchange (&CompletedEventCacheInitialization, 1L) ;

    } else {

        // sleep for 1 milliseconds and see if the initialization is complete

        ONE_MILLISECOND_TIMEOUT(TimeOut) ;

        while (!(volatile ULONG) CompletedEventCacheInitialization) {

            NtDelayExecution (FALSE, &TimeOut) ;

        }

    }
}


VOID
PrintTimerQueue(PLIST_ENTRY QNode, ULONG Delta, ULONG Count
    )
{
    PLIST_ENTRY Tnode ;
    PRTLP_TIMER Timer ;
    PRTLP_TIMER_QUEUE Queue ;
    
    Queue = CONTAINING_RECORD (QNode, RTLP_TIMER_QUEUE, List) ;
    DbgPrint("<%1d> Queue: %x FiringTime:%d\n", Count, (ULONG_PTR)Queue, 
                Queue->DeltaFiringTime);
    for (Tnode=Queue->TimerList.Flink; Tnode!=&Queue->TimerList; 
            Tnode=Tnode->Flink) 
    {
        Timer = CONTAINING_RECORD (Tnode, RTLP_TIMER, List) ;
        Delta += Timer->DeltaFiringTime ;
        DbgPrint("        Timer: %x Delta:%d Period:%d\n",(ULONG_PTR)Timer,
                    Delta, Timer->Period);
    }

}

VOID
RtlDebugPrintTimes (
    )
{    
    PLIST_ENTRY QNode ;
    ULONG Count = 0 ;
    ULONG Delta = RtlpGetTimeRemaining (TimerHandle) ;
    ULONG CurrentThreadId =  
                        HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread) ;

    RtlpResync64BitTickCount();

    if (CompletedTimerInitialization != 1) {

        DbgPrint("===========RtlTimerThread not yet initialized==========\n");
        return ;
    }

    if (CurrentThreadId == TimerThreadId)
    {
        PRTLP_TIMER_QUEUE Queue ;
        
        DbgPrint("================Printing timerqueues====================\n");
        DbgPrint("TimeRemaining: %d\n", Delta);
        for (QNode = TimerQueues.Flink; QNode != &TimerQueues; 
                QNode = QNode->Flink)
        {
            Queue = CONTAINING_RECORD (QNode, RTLP_TIMER_QUEUE, List) ;
            Delta += Queue->DeltaFiringTime ;
            
            PrintTimerQueue(QNode, Delta, ++Count);
            
        }
        DbgPrint("================Printed ================================\n");
    }

    else
    {
        NtQueueApcThread(
                    TimerThreadHandle,
                    (PPS_APC_ROUTINE)RtlDebugPrintTimes,
                    NULL,
                    NULL,
                    NULL
                    );
    }
}


/*DO NOT USE THIS FUNCTION: REPLACED BY RTLCREATETIMER*/

NTSTATUS
RtlSetTimer(
    IN  HANDLE TimerQueueHandle,
    OUT HANDLE *Handle,
    IN  WAITORTIMERCALLBACKFUNC Function,
    IN  PVOID Context,
    IN  ULONG  DueTime,
    IN  ULONG  Period,
    IN  ULONG  Flags
    )
{
    static ULONG Count = 0;
    if (Count++ ==0) {
        DbgPrint("Using obsolete function call: RtlSetTimer\n");
        DbgBreakPoint();
        DbgPrint("Using obsolete function call: RtlSetTimer\n");
    }
    
    return RtlCreateTimer(TimerQueueHandle,
                            Handle,
                            Function,
                            Context,
                            DueTime,
                            Period,
                            Flags
                            ) ;
}


PVOID
RtlpForceAllocateTPHeap(
    ULONG dwSize,
    ULONG dwFlags
    )
/*++
Routine Description:

    This routine goes into an infinite loop trying to allocate the memory.

Arguments:

    dwSize - size of memory to be allocated

    dwFlags - Flags for memory allocation

Return Value:

    ptr to memory

--*/
{
    PVOID ptr;
    ptr = RtlpAllocateTPHeap(dwSize, dwFlags);
    if (ptr)
        return ptr;

    {
        LARGE_INTEGER TimeOut ;
        do {

            ONE_SECOND_TIMEOUT(TimeOut) ;

            NtDelayExecution (FALSE, &TimeOut) ;

            ptr = RtlpAllocateTPHeap(dwSize, dwFlags);
            if (ptr)
                break;

        } while (TRUE) ;
    }
    return ptr;
}



/*DO NOT USE THIS FUNCTION: REPLACED BY RTLDeleteTimer*/

NTSTATUS
RtlCancelTimer(
    IN HANDLE TimerQueueHandle,
    IN HANDLE TimerToCancel
    )
/*++

Routine Description:

    This routine cancels the timer. This call is non-blocking. The timer Callback
    will not be executed after this call returns.

Arguments:

    TimerQueueHandle - Handle identifying the queue from which to delete timer

    TimerToCancel - Handle identifying the timer to cancel

Return Value:

    NTSTATUS - Result code from call.  The following are returned

        STATUS_SUCCESS - Timer cancelled. All callbacks completed.
        STATUS_PENDING - Timer cancelled. Some callbacks still not completed.

--*/
{
    static ULONG Count = 0;
    if (Count++ ==0) {
        DbgPrint("Using obsolete function call: RtlCancelTimer\n");
        DbgBreakPoint();
        DbgPrint("Using obsolete function call: RtlCancelTimer\n");
    }
    
    return RtlDeleteTimer( TimerQueueHandle, TimerToCancel, NULL ) ;
}

