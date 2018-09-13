/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    locking.h

Abstract:

    Private header file for locking/synchronization functions
    within setup api dll.

Author:

    Ted Miller (tedm) 31-Mar-1995

Revision History:

--*/


//
// Locking functions. These functions are used to make various parts of
// the DLL multithread-safe. The basic idea is to have a mutex and an event.
// The mutex is used to synchronize access to the structure being guarded.
// The event is only signalled when the structure being guarded is destroyed.
// To gain access to the guarded structure, a routine waits on both the mutex
// and the event. If the event gets signalled, then the structure was destroyed.
// If the mutex gets signalled, then the thread has access to the structure.
//
typedef struct _MYLOCK {
    HANDLE Handles[2];
} MYLOCK, *PMYLOCK;

//
// Indices into Locks array in string table structure.
//
#define TABLE_DESTROYED_EVENT 0
#define TABLE_ACCESS_MUTEX    1

BOOL
__inline
BeginSynchronizedAccess(
    IN PMYLOCK Lock
    )
{
    DWORD d = WaitForMultipleObjects(2,Lock->Handles,FALSE,INFINITE);
    //
    // Success if the mutex object satisfied the wait;
    // Failure if the table destroyed event satisified the wait, or
    // the mutex was abandoned, etc.
    //
    return((d - WAIT_OBJECT_0) == TABLE_ACCESS_MUTEX);
}

VOID
__inline
EndSynchronizedAccess(
    IN PMYLOCK Lock
    )
{
    ReleaseMutex(Lock->Handles[TABLE_ACCESS_MUTEX]);
}

BOOL
InitializeSynchronizedAccess(
    OUT PMYLOCK Lock
    );

VOID
DestroySynchronizedAccess(
    IN OUT PMYLOCK Lock
    );
