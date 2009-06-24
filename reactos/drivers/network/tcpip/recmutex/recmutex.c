#include <ntddk.h>
#include "recmutex.h"

VOID RecursiveMutexInit( PRECURSIVE_MUTEX RecMutex ) {
    RtlZeroMemory( RecMutex, sizeof(*RecMutex) );
    KeInitializeSpinLock( &RecMutex->SpinLock );
    ExInitializeFastMutex( &RecMutex->Mutex );
    KeInitializeEvent( &RecMutex->StateLockedEvent,
		       NotificationEvent, FALSE );
}

/* NOTE: When we leave, the FAST_MUTEX must have been released.  The result
 * is that we always exit in the same irql as entering */
SIZE_T RecursiveMutexEnter( PRECURSIVE_MUTEX RecMutex, BOOLEAN ToWrite ) {
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID CurrentThread = KeGetCurrentThread();
    KIRQL CurrentIrql;

    /* Wait for the previous user to unlock the RecMutex state.  There might be
     * multiple waiters waiting to change the state.  We need to check each
     * time we get the event whether somebody still has the state locked */

    if( !RecMutex ) return FALSE;

    if( CurrentThread == RecMutex->CurrentThread ||
	(!ToWrite && !RecMutex->Writer) ) {
	RecMutex->LockCount++;
	return TRUE;
    }

    CurrentIrql = KeGetCurrentIrql();

    ASSERT(CurrentIrql <= DISPATCH_LEVEL);

    if( CurrentIrql <= APC_LEVEL ) {
	ExAcquireFastMutex( &RecMutex->Mutex );
	RecMutex->OldIrql = CurrentIrql;
	while( RecMutex->Locked ) {
	    ExReleaseFastMutex( &RecMutex->Mutex );
	    Status = KeWaitForSingleObject( &RecMutex->StateLockedEvent,
					    UserRequest,
					    KernelMode,
					    FALSE,
					    NULL );
	    ExAcquireFastMutex( &RecMutex->Mutex );
	}
	RecMutex->Locked = TRUE;
	RecMutex->Writer = ToWrite;
	RecMutex->CurrentThread = CurrentThread;
	RecMutex->LockCount++;
	ExReleaseFastMutex( &RecMutex->Mutex );
    } else {
	KeAcquireSpinLockAtDpcLevel( &RecMutex->SpinLock );
        RecMutex->OldIrql = DISPATCH_LEVEL;
	RecMutex->Locked = TRUE;
	RecMutex->Writer = ToWrite;
	RecMutex->CurrentThread = CurrentThread;
	RecMutex->LockCount++;
    }

    return TRUE;
}

VOID RecursiveMutexLeave( PRECURSIVE_MUTEX RecMutex ) {

    ASSERT(RecMutex->LockCount > 0);
    RecMutex->LockCount--;

    if( !RecMutex->LockCount ) {
	RecMutex->CurrentThread = NULL;
	if( RecMutex->OldIrql <= APC_LEVEL ) {
	    ExAcquireFastMutex( &RecMutex->Mutex );
	    RecMutex->Locked = FALSE;
	    RecMutex->Writer = FALSE;
	    ExReleaseFastMutex( &RecMutex->Mutex );
	} else {
	    RecMutex->Locked = FALSE;
	    RecMutex->Writer = FALSE;
	    KeReleaseSpinLockFromDpcLevel( &RecMutex->SpinLock );
	}

	KePulseEvent( &RecMutex->StateLockedEvent, IO_NETWORK_INCREMENT,
		      FALSE );
    }
}

