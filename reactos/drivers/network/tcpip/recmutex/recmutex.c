#include <ntddk.h>
#include "recmutex.h"

VOID RecursiveMutexInit( PRECURSIVE_MUTEX RecMutex ) {
    RtlZeroMemory( RecMutex, sizeof(*RecMutex) );
    ExInitializeFastMutex( &RecMutex->Mutex );
    KeInitializeEvent( &RecMutex->StateLockedEvent,
        NotificationEvent, FALSE );
}

/* NOTE: When we leave, the FAST_MUTEX must have been released.  The result
* is that we always exit in the same irql as entering */
VOID RecursiveMutexEnter( PRECURSIVE_MUTEX RecMutex ) {
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID CurrentThread = KeGetCurrentThread();
    KIRQL CurrentIrql = KeGetCurrentIrql();

    ASSERT(RecMutex);
    ASSERT(CurrentIrql <= APC_LEVEL);

    /* Wait for the previous user to unlock the RecMutex state.  There might be
    * multiple waiters waiting to change the state.  We need to check each
    * time we get the event whether somebody still has the state locked */

    ExAcquireFastMutex( &RecMutex->Mutex );

    if( CurrentThread == RecMutex->CurrentThread ) {
            RecMutex->LockCount++;
            ExReleaseFastMutex( &RecMutex->Mutex );
            return;
    }

    while( RecMutex->LockCount != 0 ) {
         ExReleaseFastMutex( &RecMutex->Mutex );
         Status = KeWaitForSingleObject( &RecMutex->StateLockedEvent,
             UserRequest,
             KernelMode,
             FALSE,
             NULL );
         ExAcquireFastMutex( &RecMutex->Mutex );
    }
    RecMutex->CurrentThread = CurrentThread;
    RecMutex->LockCount++;
    ExReleaseFastMutex( &RecMutex->Mutex );
}

VOID RecursiveMutexLeave( PRECURSIVE_MUTEX RecMutex ) {
    ASSERT(RecMutex);

    ExAcquireFastMutex( &RecMutex->Mutex );

    ASSERT(RecMutex->LockCount > 0);
    RecMutex->LockCount--;

    if( !RecMutex->LockCount ) {
        KePulseEvent( &RecMutex->StateLockedEvent, IO_NETWORK_INCREMENT,
            FALSE );
    }

    ExReleaseFastMutex( &RecMutex->Mutex );
}

