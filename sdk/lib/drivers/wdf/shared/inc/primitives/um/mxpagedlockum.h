/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxPagedLockUm.h

Abstract:

    User mode implementation of paged lock defined in
    MxPagedLock.h

Author:



Revision History:



--*/

#pragma once

typedef struct {
    CRITICAL_SECTION Lock;
    bool Initialized;
    DWORD OwnerThreadId;
} MdPagedLock;

#include "MxPagedLock.h"

__inline
MxPagedLock::MxPagedLock(
    )
{
    m_Lock.Initialized = false;
    m_Lock.OwnerThreadId = 0;
}

_Must_inspect_result_
__inline
NTSTATUS
MxPagedLockNoDynam::Initialize(
    )
{
    if (InitializeCriticalSectionAndSpinCount(&m_Lock.Lock, 0)) {
        m_Lock.Initialized = true;

        return S_OK;
    }
    else {
        DWORD err = GetLastError();
        return WinErrorToNtStatus(err);
    }
}

__inline
VOID
#pragma prefast(suppress:__WARNING_UNMATCHED_DEFN, "Can't apply kernel mode annotations.");
MxPagedLockNoDynam::Acquire(
    )
{
    EnterCriticalSection(&m_Lock.Lock);

    DWORD threadId = GetCurrentThreadId();

    if (threadId == m_Lock.OwnerThreadId) {
        Mx::MxAssertMsg("Recursive acquision of the lock is not allowed", FALSE);
    }

    m_Lock.OwnerThreadId = GetCurrentThreadId();
}

__inline
VOID
MxPagedLockNoDynam::AcquireUnsafe(
    )
{
    MxPagedLockNoDynam::Acquire();
}

__inline
BOOLEAN
#pragma prefast(suppress:__WARNING_UNMATCHED_DEFN, "Can't apply kernel mode annotations.");
MxPagedLockNoDynam::TryToAcquire(
    )
{
    return TryEnterCriticalSection(&m_Lock.Lock) == TRUE ? TRUE : FALSE;
}


__inline
VOID
#pragma prefast(suppress:__WARNING_UNMATCHED_DEFN, "Can't apply kernel mode annotations.");
MxPagedLockNoDynam::Release(
    )
{
    m_Lock.OwnerThreadId = 0;

    LeaveCriticalSection(&m_Lock.Lock);
}

__inline
VOID
MxPagedLockNoDynam::ReleaseUnsafe(
    )
{
    MxPagedLockNoDynam::Release();
}

__inline
VOID
MxPagedLockNoDynam::Uninitialize(
    )
{
    DeleteCriticalSection(&m_Lock.Lock);
    m_Lock.Initialized = false;
}

__inline
MxPagedLock::~MxPagedLock(
    )
{
    if (m_Lock.Initialized) {
        this->Uninitialize();
    }
}
