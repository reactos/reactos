/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    eballoc.c

Abstract:

    Process/Thread Environment Block allocation functions

Author:

    Steve Wood (stevewo) 10-May-1990

Revision History:

--*/

#include "ntrtlp.h"
#include <nturtl.h>

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,RtlAcquirePebLock)
#pragma alloc_text(PAGE,RtlReleasePebLock)
#endif

typedef VOID (*PEB_LOCK_ROUTINE)(PVOID FastLock);

VOID
RtlAcquirePebLock( VOID )
{
    PEB_LOCK_ROUTINE LockRoutine;
    PPEB Peb;

    RTL_PAGED_CODE();

    Peb = NtCurrentPeb();

    LockRoutine = (PEB_LOCK_ROUTINE)Peb->FastPebLockRoutine;
    ASSERT(LockRoutine);
    (LockRoutine)(Peb->FastPebLock);
}

VOID
RtlReleasePebLock( VOID )
{
    PEB_LOCK_ROUTINE LockRoutine;
    PPEB Peb;

    RTL_PAGED_CODE();

    Peb = NtCurrentPeb();

    LockRoutine = (PEB_LOCK_ROUTINE)Peb->FastPebUnlockRoutine;
    ASSERT(LockRoutine);
    (LockRoutine)(Peb->FastPebLock);
}
