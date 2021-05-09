/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxLockUm.h

Abstract:

    User mode implementation of lock
    class defined in MxLock.h

Author:



Revision History:



--*/

#pragma once

typedef struct {
    CRITICAL_SECTION Lock;
    bool Initialized;
    DWORD OwnerThreadId;
} MdLock;

#include "DbgMacros.h"
#include "MxLock.h"

__inline
MxLock::MxLock(
    )
{
    CLEAR_DBGFLAG_INITIALIZED;

    m_Lock.Initialized = false;
    m_Lock.OwnerThreadId = 0;

    MxLock::Initialize();
}

__inline
VOID
MxLockNoDynam::Initialize(
    )
{
    BOOL ret;

    ASSERT_DBGFLAG_NOT_INITIALIZED;

    ret = InitializeCriticalSectionAndSpinCount(&m_Lock.Lock, 0);

    //
    // InitializeCriticalSectionAndSpinCount always returns TRUE on Vista+
    // Assert this contract on checked builds using DBGFLAG macro.
    //
    if (ret) {
        m_Lock.Initialized = true;
        SET_DBGFLAG_INITIALIZED;
    }

    ASSERT_DBGFLAG_INITIALIZED;
}


__inline
VOID
#pragma prefast(suppress:__WARNING_UNMATCHED_DECL_ANNO, "Can't apply kernel mode annotations.");
MxLockNoDynam::Acquire(
    __out KIRQL * OldIrql
    )
{
    ASSERT_DBGFLAG_INITIALIZED;

    EnterCriticalSection(&m_Lock.Lock);

    DWORD threadId = GetCurrentThreadId();

    if (threadId == m_Lock.OwnerThreadId) {
        Mx::MxAssertMsg("Recursive acquision of the lock is not allowed", FALSE);
    }

    m_Lock.OwnerThreadId = threadId;

    *OldIrql = PASSIVE_LEVEL;
}

__inline
BOOLEAN
MxLockNoDynam::TryToAcquire(
    VOID
    )
{
    BOOLEAN acquired;

    ASSERT_DBGFLAG_INITIALIZED;

    acquired = (BOOLEAN) TryEnterCriticalSection(&m_Lock.Lock);

    if (acquired) {
        DWORD threadId = GetCurrentThreadId();

        if (threadId == m_Lock.OwnerThreadId) {
            Mx::MxAssertMsg("Recursive acquision of the lock is not allowed", FALSE);
        }

        m_Lock.OwnerThreadId = threadId;
    }

    return acquired;
}

__inline
VOID
#pragma prefast(suppress:__WARNING_UNMATCHED_DEFN, "Can't apply kernel mode annotations.");
MxLockNoDynam::AcquireAtDpcLevel(
    )
{
    ASSERT_DBGFLAG_INITIALIZED;

    KIRQL dontCare;

    Acquire(&dontCare);
}

__inline
VOID
#pragma prefast(suppress:__WARNING_UNMATCHED_DEFN, "Can't apply kernel mode annotations.");
MxLockNoDynam::Release(
    KIRQL NewIrql
    )
{
    ASSERT_DBGFLAG_INITIALIZED;

    Mx::MxAssert(NewIrql == PASSIVE_LEVEL);

    m_Lock.OwnerThreadId = 0;

    LeaveCriticalSection(&m_Lock.Lock);
}

__inline
VOID
#pragma prefast(suppress:__WARNING_UNMATCHED_DEFN, "Can't apply kernel mode annotations.");
MxLockNoDynam::ReleaseFromDpcLevel(
    )
{
    ASSERT_DBGFLAG_INITIALIZED;

    Release(PASSIVE_LEVEL);
}

__inline
VOID
MxLockNoDynam::Uninitialize(
    )
{
    ASSERT_DBGFLAG_INITIALIZED;

    DeleteCriticalSection(&m_Lock.Lock);
    m_Lock.Initialized = false;

    CLEAR_DBGFLAG_INITIALIZED;
}

__inline
MxLock::~MxLock(
    )
{
    //
    // PLEASE NOTE: shared code must not rely of d'tor uninitializing the
    // lock. d'tor may not be invoked if the event is used in a structure
    // which is allocated/deallocated using MxPoolAllocate/Free instead of
    // new/delete
    //

    if (m_Lock.Initialized) {
        this->Uninitialize();
    }
}
