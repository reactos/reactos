/*++

Copyright (c) 1989-1998 Microsoft Corporation

Module Name:

    threads.h

Abstract:

    This module is the header file for thread pools. Thread pools can be used for
    one time execution of tasks, for waits and for one shot or periodic timers.

Author:

    Gurdeep Singh Pall (gurdeep) Nov 13, 1997

Environment:

    These routines are statically linked in the caller's executable and
    are callable in only from user mode. They make use of Nt system services.


Revision History:

    Aug-19 lokeshs - modifications to thread pool apis.

--*/

//todo remove below
#define DBG1 1



// Structures used by the Thread pool

// Timer structures

// Timer Queues and Timer entries both use RTLP_GENERIC_TIMER structure below.
// Timer Queues are linked using List.
// Timers are attached to the Timer Queue using TimerList
// Timers are linked to each other using List

#define RTLP_TIMER RTLP_GENERIC_TIMER
#define PRTLP_TIMER PRTLP_GENERIC_TIMER

#define RTLP_TIMER_QUEUE RTLP_GENERIC_TIMER
#define PRTLP_TIMER_QUEUE PRTLP_GENERIC_TIMER

struct _RTLP_WAIT ;

typedef struct _RTLP_GENERIC_TIMER {

    LIST_ENTRY List ;                   // All Timers and Queues are linked using this.
    ULONG DeltaFiringTime ;             // Time difference in Milliseconds from the TIMER entry
                                        // just before this entry
    union {
        ULONG RefCount ;        // Timer RefCount
        ULONG * RefCountPtr ;   // Pointer to Wait->Refcount
    } ;                         // keeps count of async callbacks still executing

    ULONG State ;               // State of timer: CREATED, DELETE, ACTIVE. DONT_FIRE

    union {

        // Used for Timer Queues

        struct  {

            LIST_ENTRY  TimerList ;     // Timers Hanging off of the queue
            LIST_ENTRY  UncancelledTimerList ;//List of one shot timers not cancelled
                                              //not used for wait timers
            #if DBG1
            ULONG NextDbgId; //ksl
            #endif
            
        } ;

        // Used for Timers

        struct  {
            struct _RTLP_GENERIC_TIMER *Queue ;// Queue to which this timer belongs
            struct _RTLP_WAIT *Wait ;  // Pointer to Wait event if timer is part of waits. else NULL
            ULONG Flags ;              // Flags indicating special treatment for this timer
            PVOID Function ;           // Function to call when timer fires
            PVOID Context ;            // Context to pass to function when timer fires
            ULONG Period ;             // In Milliseconds. Used for periodic timers.
            LIST_ENTRY TimersToFireList;//placed in this list if the timer is fired
        } ;
    } ;

    HANDLE CompletionEvent ;   // Event signalled when the timer is finally deleted

    #if DBG1
    ULONG DbgId; //ksl
    ULONG ThreadId ;
    ULONG ThreadId2 ;
    #endif

}  RTLP_GENERIC_TIMER, *PRTLP_GENERIC_TIMER ;

#if DBG1
ULONG NextTimerDbgId;
ULONG NextWaitDbgId;
#endif


// Structurs used by Wait Threads

// Wait structure

typedef struct _RTLP_WAIT {

    struct _RTLP_WAIT_THREAD_CONTROL_BLOCK *ThreadCB ;
    HANDLE WaitHandle ;         // Object to wait on
    ULONG State ;               // REGISTERED, ACTIVE,DELETE state flags
    ULONG RefCount ;            // initially set to 1. When 0, then ready to be deleted
    HANDLE CompletionEvent ;
    struct _RTLP_GENERIC_TIMER *Timer ; // For timeouts on the wait
    ULONG Flags ;               // Flags indicating special treatment for this wait
    PVOID Function ;            // Function to call when wait completes
    PVOID Context ;             // Context to pass to function
    ULONG Timeout ;             // In Milliseconds.
    #if DBG1
    ULONG DbgId ;
    ULONG ThreadId ;
    ULONG ThreadId2 ;
    #endif
    
} RTLP_WAIT, *PRTLP_WAIT ;


// Wait Thread Control Block

typedef struct _RTLP_WAIT_THREAD_CONTROL_BLOCK {

    LIST_ENTRY WaitThreadsList ;// List of all the thread control blocks

    HANDLE ThreadHandle ;       // Handle for this thread
    ULONG ThreadId ;            // Used to check if callback is in WaitThread

    ULONG NumWaits ;            // Number of active waits + handles not being waited upon
    ULONG NumActiveWaits ;      // Total number of waits.
    HANDLE ActiveWaitArray[64] ;// Array used for waiting
    PRTLP_WAIT ActiveWaitPointers[64] ;// Array of pointers to active Wait blocks.
    HANDLE TimerHandle ;        // Handle to the NT timer used for timeouts
    RTLP_TIMER_QUEUE TimerQueue;// Queue in which all timers are kept
    RTLP_TIMER TimerBlocks[63] ;// All the timers required for wait timeouts - max of 63 since wait[0]
                                // is used for NT timer object
    LIST_ENTRY FreeTimerBlocks ;// List of Free Blocks

    LARGE_INTEGER Current64BitTickCount ;
    LONGLONG Firing64BitTickCount ;
    
    RTL_CRITICAL_SECTION WaitThreadCriticalSection ;
                                // Used for addition and deletion of waits

} RTLP_WAIT_THREAD_CONTROL_BLOCK, *PRTLP_WAIT_THREAD_CONTROL_BLOCK ;


// Structure used for attaching all I/O worker threads

typedef struct _RTLP_IOWORKER_TCB {

    LIST_ENTRY List ;           // List of IO Worker threads
    HANDLE     ThreadHandle ;   // Handle of this thread
    ULONG      Flags ;          // WT_EXECUTEINPERSISTENTIOTHREAD
    BOOLEAN    LongFunctionFlag ;// Is the thread currently executing long fn
} RTLP_IOWORKER_TCB, *PRTLP_IOWORKER_TCB ;

typedef struct _RTLP_WAITWORKER {
    union {
        PRTLP_WAIT Wait ;
        PRTLP_TIMER Timer ;
    } ;
    BOOLEAN WaitThreadCallback ; //callback queued by Wait thread or Timer thread
    BOOLEAN TimerCondition ;//true if fired because wait timed out.
} RTLP_ASYNC_CALLBACK, * PRTLP_ASYNC_CALLBACK ;



// structure used for calling worker function

typedef struct _RTLP_WORK {

    WORKERCALLBACKFUNC Function ;
    ULONG Flags ;
    
} RTLP_WORK, *PRTLP_WORK ;



// Structure used for storing events

typedef struct _RTLP_EVENT {

    LIST_ENTRY List ;
    HANDLE Handle ;
    
} RTLP_EVENT, *PRTLP_EVENT ;



// Globals used by the thread pool

ULONG StartedTPInitialization ; // Used for Initializing ThreadPool
ULONG CompletedTPInitialization;// Used to check if ThreadPool is initialized

ULONG StartedWorkerInitialization ;     // Used for Worker thread startup synchronization
ULONG CompletedWorkerInitialization ;   // Used to check if Worker thread pool is initialized

ULONG StartedWaitInitialization ;       // Used for Wait thread startup synchronization
ULONG CompletedWaitInitialization ;     // Used to check if Wait thread pool is initialized

ULONG StartedTimerInitialization ;      // Used by Timer thread startup synchronization
ULONG CompletedTimerInitialization ;    // Used for to check if Timer thread is initialized

ULONG StartedEventCacheInitialization ; // Used for initializing event cache
ULONG CompletedEventCacheInitialization;// Used for initializing event cache

HANDLE TimerThreadHandle ;              // Holds the timer thread handle
ULONG TimerThreadId ;                   // Used to check if current thread is a timer thread

ULONG NumIOWorkerThreads ;              // Count of IO Worker Threads alive
ULONG NumWorkerThreads ;                // Count of Worker Threads alive
ULONG NumMinWorkerThreads ;             // Min worker threads should be alive: 1 if ioCompletion used, else 0
ULONG NumIOWorkRequests ;               // Count of IO Work Requests pending
ULONG NumLongIOWorkRequests ;           // IO Worker threads executing long worker functions
ULONG NumWorkRequests ;                 // Count of Work Requests pending.
ULONG NumLongWorkRequests ;             // Worker threads executing long worker functions
ULONG NumExecutingWorkerThreads ;       // Worker threads currently executing worker functions
ULONG TotalExecutedWorkRequests ;       // Total worker requests that were picked up
ULONG OldTotalExecutedWorkRequests ;    // Total worker requests since last timeout.
HANDLE WorkerThreadTimerQueue ;         // Timer queue used by worker threads
ULONG WorkerThreadTimerQueueInit ;      // Initialization of the above timer queue

HANDLE WorkerThreadTimer ;              // Timer used by worker threads

ULONG NumUnusedEvents ;                 // Count of Unused events in the cache

ULONG LastThreadCreationTickCount ;     // Tick count at which the last thread was created

LIST_ENTRY IOWorkerThreads ;            // List of IOWorkerThreads
PRTLP_IOWORKER_TCB PersistentIOTCB ;    // ptr to TCB of persistest IO worker thread
HANDLE WorkerCompletionPort ;           // Completion port used for queuing tasks to Worker threads

LIST_ENTRY WaitThreads ;                // List of all wait threads created
LIST_ENTRY EventCache ;                 // Events used for synchronization


LIST_ENTRY TimerQueues ;                // All timer queues are linked in this list
HANDLE     TimerHandle ;                // Holds handle of NT Timer used by the Timer Thread
ULONG      NumTimerQueues ;             // Number of timer queues

RTL_CRITICAL_SECTION WorkerCriticalSection ;    // Exclusion used by worker threads
RTL_CRITICAL_SECTION WaitCriticalSection ;      // Exclusion used by wait threads
RTL_CRITICAL_SECTION TimerCriticalSection ;     // Exclusion used by timer threads
RTL_CRITICAL_SECTION EventCacheCriticalSection ;// Exclusion used for handle allocation

RTLP_START_THREAD RtlpStartThread ;
PRTLP_START_THREAD RtlpStartThreadFunc = RtlpStartThread ;
RTLP_EXIT_THREAD RtlpExitThread ;
PRTLP_EXIT_THREAD RtlpExitThreadFunc = RtlpExitThread ;

#if DBG1
PVOID CallbackFn1, CallbackFn2, Context1, Context2 ;
#endif


// defines used in the thread pool

#define THREAD_CREATION_DAMPING_TIME1    1000    // In Milliseconds. Time between starting successive threads.
#define THREAD_CREATION_DAMPING_TIME2    5000    // In Milliseconds. Time between starting successive threads.
#define THREAD_TERMINATION_DAMPING_TIME 10000    // In Milliseconds. Time between stopping successive threads.
#define NEW_THREAD_THRESHOLD            7       // Number of requests outstanding before we start a new thread
#define MAX_WORKER_THREADS              1000    // Max effective worker threads
#define INFINITE_TIME                   (ULONG)~0   // In milliseconds
#define RTLP_MAX_TIMERS                 0x00080000  // 524288 timers per process
#define MAX_UNUSED_EVENTS               40


// Macros


#define ONE_MILLISECOND_TIMEOUT(TimeOut) {      \
        TimeOut.LowPart  = 0xffffd8f0 ;         \
        TimeOut.HighPart = 0xffffffff ;         \
        }

#define HUNDRED_MILLISECOND_TIMEOUT(TimeOut) {  \
        TimeOut.LowPart  = 0xfff0bdc0 ;         \
        TimeOut.HighPart = 0xffffffff ;         \
        }

#define ONE_SECOND_TIMEOUT(TimeOut) {           \
        TimeOut.LowPart  = 0xff676980 ;         \
        TimeOut.HighPart = 0xffffffff ;         \
        }

#define USE_PROCESS_HEAP 1

#define RtlpFreeTPHeap(Ptr) \
    RtlFreeHeap( RtlProcessHeap(), 0, (Ptr) )

#define RtlpAllocateTPHeap(Size, Flags) \
    RtlAllocateHeap( RtlProcessHeap(), (Flags), (Size) )

// used to allocate Wait thread

#define ACQUIRE_GLOBAL_WAIT_LOCK() \
    RtlEnterCriticalSection (&WaitCriticalSection)

#define RELEASE_GLOBAL_WAIT_LOCK() \
    RtlLeaveCriticalSection(&WaitCriticalSection)


// taken before a timer/queue is deleted and when the timers
// are being fired. Used to assure that no timers will be fired later.

#define ACQUIRE_GLOBAL_TIMER_LOCK() \
    RtlEnterCriticalSection (&TimerCriticalSection)

#define RELEASE_GLOBAL_TIMER_LOCK() \
    RtlLeaveCriticalSection(&TimerCriticalSection)

// used in RtlpThreadPoolCleanup to find if a component is initialized

#define IS_COMPONENT_INITIALIZED(StartedVariable, CompletedVariable, Flag) \
{\
    LARGE_INTEGER TimeOut ;     \
    Flag = FALSE ;              \
                                \
    if ( StartedVariable ) {    \
                                \
        if ( !CompletedVariable ) { \
                                    \
            ONE_MILLISECOND_TIMEOUT(TimeOut) ;  \
                                                \
            while (!(volatile ULONG) CompletedVariable) \
                NtDelayExecution (FALSE, &TimeOut) ;    \
                                                        \
            if (CompletedVariable == 1)                 \
                Flag = TRUE ;                           \
                                                        \
        } else {                                        \
            Flag = TRUE ;                               \
        }                                               \
    }                                                   \
}    


// macro used to set dbg function/context

#define DBG_SET_FUNCTION(Fn, Context) { \
    CallbackFn1 = CallbackFn2 ;         \
    CallbackFn2 = (Fn) ;                \
    Context1 = Context2 ;               \
    Context2 = (Context ) ;             \
}


// used to move the wait array
/*
VOID
RtlpShiftWaitArray(
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB ThreadCB,
    ULONG SrcIndex,
    ULONG DstIndex,
    ULONG Count
    )
*/
#define RtlpShiftWaitArray(ThreadCB, SrcIndex, DstIndex, Count) {  \
                                                            \
    RtlCopyMemory (&(ThreadCB)->ActiveWaitArray[DstIndex],  \
                    &(ThreadCB)->ActiveWaitArray[SrcIndex], \
                    sizeof (HANDLE) * (Count)) ;            \
                                                            \
    RtlCopyMemory (&(ThreadCB)->ActiveWaitPointers[DstIndex],\
                    &(ThreadCB)->ActiveWaitPointers[SrcIndex],\
                    sizeof (HANDLE) * (Count)) ;            \
}

LARGE_INTEGER   Last64BitTickCount ;
LARGE_INTEGER   Resync64BitTickCount ;
LARGE_INTEGER   Firing64BitTickCount ;

#define RtlpGetResync64BitTickCount()  Resync64BitTickCount.QuadPart
#define RtlpSetFiring64BitTickCount(Timeout) \
            Firing64BitTickCount.QuadPart = (Timeout)


    
// signature for timer and wait entries

#define SET_SIGNATURE(ptr)          (ptr)->State |= 0xfedc0000
#define CHECK_SIGNATURE(ptr)        ASSERT( ((ptr)->State & 0xffff0000) == 0xfedc0000 )
#define SET_DEL_SIGNATURE(ptr)      ((ptr)->State |= 0xfedcb000)
#define CHECK_DEL_SIGNATURE(ptr)    ASSERT( (((ptr)->State & 0xffff0000) == 0xfedc0000) \
                                        && ( ! ((ptr)->State & 0x0000f000)) )
#define CLEAR_SIGNATURE(ptr)        ((ptr)->State = ((ptr)->State & 0x0000ffff) | 0xcdef0000)
#define SET_DEL_TIMERQ_SIGNATURE(ptr)  ((ptr)->State |= 0x00000a00)


// debug prints

#define DPRN0 (DPRN & 0x1)
#define DPRN1 (DPRN & 0x2)
#define DPRN2 (DPRN & 0x4)
#define DPRN3 (DPRN & 0x8)
#define DPRN4 (DPRN & 0x10)



// Prototypes for thread pool private functions

NTSTATUS
RtlpInitializeWorkerThreadPool (
    ) ;

NTSTATUS
RtlpInitializeWaitThreadPool (
    ) ;

NTSTATUS
RtlpInitializeTimerThreadPool (
    ) ;

NTSTATUS
RtlpStartThread (
    IN  PUSER_THREAD_START_ROUTINE Function,
    OUT HANDLE *ThreadHandle
    ) ;

LONG
RtlpWaitThread (
    IN PVOID  WaitHandle
    ) ;

LONG
RtlpWorkerThread (
    PVOID  NotUsed
    ) ;

LONG
RtlpIOWorkerThread (
    PVOID  NotUsed
    ) ;

LONG
RtlpTimerThread (
    PVOID  NotUsed
    ) ;

VOID
RtlpAddTimerQueue (
    PVOID Queue
    ) ;

VOID
RtlpAddTimer (
    PRTLP_TIMER Timer
    ) ;

VOID
RtlpResetTimer (
    HANDLE TimerHandle,
    ULONG DueTime,
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB
    ) ;


VOID
RtlpFireTimersAndReorder (
    PRTLP_TIMER_QUEUE Queue,
    ULONG *NewFiringTime,
    PLIST_ENTRY TimersToFireList
    ) ;

VOID
RtlpFireTimers (
    PLIST_ENTRY TimersToFireList
    ) ;

VOID
RtlpInsertTimersIntoDeltaList (
    PLIST_ENTRY NewTimerList,
    PLIST_ENTRY DeltaTimerList,
    ULONG TimeRemaining,
    ULONG *NewFiringTime
    ) ;

BOOLEAN
RtlpInsertInDeltaList (
    PLIST_ENTRY DeltaList,
    PRTLP_GENERIC_TIMER NewTimer,
    ULONG TimeRemaining,
    ULONG *NewFiringTime
    ) ;

BOOLEAN
RtlpRemoveFromDeltaList (
    PLIST_ENTRY DeltaList,
    PRTLP_GENERIC_TIMER Timer,
    ULONG TimeRemaining,
    ULONG* NewFiringTime
    ) ;

BOOLEAN
RtlpReOrderDeltaList (
    PLIST_ENTRY DeltaList,
    PRTLP_GENERIC_TIMER Timer,
    ULONG TimeRemaining,
    ULONG* NewFiringTime,
    ULONG ChangedFiringTime
    ) ;


VOID
RtlpAddWait (
    PRTLP_WAIT Wait
    ) ;

NTSTATUS
RtlpDeregisterWait (
    PRTLP_WAIT Wait,
    HANDLE PartialCompletionEvent,
    PULONG StatusPtr
    ) ;

NTSTATUS
RtlpDeactivateWait (
    PRTLP_WAIT Wait
    ) ;

VOID
RtlpDeleteWait (
    PRTLP_WAIT Wait
    ) ;

VOID
RtlpProcessWaitCompletion (
    PRTLP_WAIT Wait,
    ULONG ArrayIndex
    ) ;

VOID
RtlpProcessTimeouts (
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB
    ) ;

ULONG
RtlpGetTimeRemaining (
    HANDLE TimerHandle
    ) ;


VOID
RtlpServiceTimer (
    PVOID NotUsedArg,
    ULONG NotUsedLowTimer,
    LONG NotUsedHighTimer
    ) ;

ULONG
RtlpGetQueueRelativeTime (
    PRTLP_TIMER_QUEUE Queue
    ) ;

VOID
RtlpCancelTimer (
    PRTLP_TIMER TimerToCancel
    ) ;

VOID
RtlpCancelTimerEx (
    PRTLP_TIMER Timer,
    BOOLEAN DeletingQueue
    ) ;

VOID
RtlpDeactivateTimer (
    PRTLP_TIMER_QUEUE Queue,
    PRTLP_TIMER Timer
    ) ;


NTSTATUS
RtlpDeleteTimerQueue (
    PRTLP_TIMER_QUEUE Queue
    ) ;

VOID
RtlpDeleteTimerQueueComplete (
    PRTLP_TIMER_QUEUE Queue
    ) ;

LONGLONG
Rtlp64BitTickCount(
    ) ;

NTSTATUS
RtlpFindWaitThread (
    PRTLP_WAIT_THREAD_CONTROL_BLOCK *ThreadCB
    ) ;

VOID
RtlpExecuteIOWorkItem (
    PVOID Function,
    PVOID Context,
    PVOID NotUsed
    ) ;

VOID
RtlpExecuteLongIOWorkItem (
    PVOID Function,
    PVOID Context,
    PVOID NotUsed
    ) ;

NTSTATUS
RtlpQueueIOWorkerRequest (
    WORKERCALLBACKFUNC Function,
    PVOID Context,
    ULONG Flags
    ) ;

NTSTATUS
RtlpStartWorkerThread (
    ) ;

NTSTATUS
RtlpStartIOWorkerThread (
    ) ;

VOID
RtlpWorkerThreadTimerCallback(
    PVOID Context,
    BOOLEAN NotUsed
    ) ;

NTSTATUS
RtlpQueueWorkerRequest (
    WORKERCALLBACKFUNC Function,
    PVOID Context,
    ULONG Flags
    ) ;


#define THREAD_TYPE_WORKER 1
#define THREAD_TYPE_IO_WORKER 2

BOOLEAN
RtlpDoWeNeedNewWorkerThread (
    ULONG ThreadType
    ) ;

VOID
RtlpUpdateTimer (
    PRTLP_TIMER Timer,
    PRTLP_TIMER UpdatedTimer
    ) ;

VOID
RtlpDeleteTimer (
    PRTLP_TIMER Timer
    ) ;

PRTLP_EVENT
RtlpGetWaitEvent (
    VOID
    ) ;

VOID
RtlpFreeWaitEvent (
    PRTLP_EVENT Event
    ) ;

VOID
RtlpInitializeEventCache (
    VOID
    ) ;

VOID
RtlpFreeWaitEvent (
    PRTLP_EVENT Event
    ) ;

PRTLP_EVENT
RtlpGetWaitEvent (
    VOID
    ) ;

VOID
RtlpDeleteWaitAPC (
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB,
    PRTLP_WAIT Wait,
    HANDLE Handle
    ) ;

VOID
RtlpDoNothing (
    PVOID NotUsed1,
    PVOID NotUsed2,
    PVOID NotUsed3
    ) ;

VOID
RtlpExecuteWorkerRequest (
    NTSTATUS Status,
    PVOID Context,
    PVOID ActualFunction
    ) ;

NTSTATUS
RtlpInitializeTPHeap (
    ) ;

NTSTATUS
RtlpWaitForEvent (
    HANDLE Event,
    HANDLE ThreadHandle
    ) ;

VOID
RtlpThreadCleanup (
    ) ;

VOID
RtlpWorkerThreadCleanup (
    NTSTATUS Status,
    PVOID NotUsed,
    PVOID NotUsed2
    ) ;

PVOID
RtlpForceAllocateTPHeap(
    ULONG dwSize,
    ULONG dwFlags
    );
    
VOID
RtlpWorkerThreadInitializeTimers(
    PVOID Context
    );


NTSYSAPI
NTSTATUS
NTAPI
RtlThreadPoolCleanup (
    ULONG Flags
    ) ;

//to make sure that a wait is not deleted before being registered
#define STATE_REGISTERED   0x0001

//set when wait registered. Removed when one shot wait fired.
//when deregisterWait called, tells whether to be removed from ActiveArray
//If timer active, then have to remove it from delta list and reset the timer.
#define STATE_ACTIVE       0x0002

//when deregister wait is called(RefCount may be >0)
#define STATE_DELETE       0x0004

//set when cancel timer called. The APC will clean it up.
#define STATE_DONTFIRE     0x0008

//set when one shot timer fired.
#define STATE_ONE_SHOT_FIRED 0x0010
