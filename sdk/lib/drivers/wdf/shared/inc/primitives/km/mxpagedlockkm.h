/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxPagedLockKm.h

Abstract:

    Kernel mode implementation of paged lock defined in
    MxPagedLock.h

Author:



Revision History:



--*/

#pragma once

#include "dbgmacros.h"

typedef FAST_MUTEX MdPagedLock;

#include "mxpagedlock.h"

__inline
MxPagedLock::MxPagedLock(
    )
{
    CLEAR_DBGFLAG_INITIALIZED;

    //
    // Temporarily call initialize from c'tor
    // so that we don't have to churn all of the KMDF code
    //
#ifndef MERGE_COMPLETE
    (VOID) MxPagedLock::Initialize();
#endif
}

__inline
NTSTATUS
#ifdef _MSC_VER
#pragma prefast(suppress:__WARNING_UNMATCHED_DEFN, "_Must_inspect_result_ not needed in kernel mode as the function always succeeds");
#endif
MxPagedLockNoDynam::Initialize(
    )
{
    ExInitializeFastMutex(&m_Lock);

    SET_DBGFLAG_INITIALIZED;

    return STATUS_SUCCESS;
}

__drv_maxIRQL(APC_LEVEL)
__drv_setsIRQL(APC_LEVEL)
__drv_savesIRQLGlobal(FastMutexObject, this->m_Lock)
_Acquires_lock_(this->m_Lock)
__inline
VOID
MxPagedLockNoDynam::Acquire(
    )
{
    ASSERT_DBGFLAG_INITIALIZED;

    ExAcquireFastMutex(&m_Lock);
}

__inline
VOID
MxPagedLockNoDynam::AcquireUnsafe(
    )
{
    ASSERT_DBGFLAG_INITIALIZED;

    ExAcquireFastMutexUnsafe(&m_Lock);
}

_Must_inspect_result_
__drv_maxIRQL(APC_LEVEL)
__drv_savesIRQLGlobal(FastMutexObject, this->m_Lock)
__drv_valueIs(==1;==0)
__drv_when(return==1, __drv_setsIRQL(APC_LEVEL))
_When_(return==1, _Acquires_lock_(this->m_Lock))
__inline
BOOLEAN
MxPagedLockNoDynam::TryToAcquire(
    )
{
    ASSERT_DBGFLAG_INITIALIZED;

    return ExTryToAcquireFastMutex(&m_Lock);
}

__drv_requiresIRQL(APC_LEVEL)
__drv_restoresIRQLGlobal(FastMutexObject, this->m_Lock)
_Releases_lock_(this->m_Lock)
__inline
VOID
MxPagedLockNoDynam::Release(
    )
{
    ASSERT_DBGFLAG_INITIALIZED;

    ExReleaseFastMutex(&m_Lock);
}

__inline
VOID
MxPagedLockNoDynam::ReleaseUnsafe(
    )
{
    ASSERT_DBGFLAG_INITIALIZED;

    ExReleaseFastMutexUnsafe(&m_Lock);
}


__inline
VOID
MxPagedLockNoDynam::Uninitialize(
    )
{
    CLEAR_DBGFLAG_INITIALIZED;
}

__inline
MxPagedLock::~MxPagedLock(
    )
{
    CLEAR_DBGFLAG_INITIALIZED;
}
