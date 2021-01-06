/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxSpinLock.cpp

Abstract:

    This module implements FxSpinLock

Author:



Environment:

   Both kernel and user mode

Revision History:

--*/

#include "fxsupportpch.hpp"
#include "fxspinlock.hpp"

extern "C" {
// #include "FxSpinLock.tmh"
}

FxSpinLock::FxSpinLock(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in USHORT ExtraSize
    ) :
    FxObject(FX_TYPE_SPIN_LOCK, COMPUTE_OBJECT_SIZE(sizeof(FxSpinLock), ExtraSize), FxDriverGlobals)
{
    FX_SPIN_LOCK_HISTORY* pHistory;

    m_Irql = 0;
    m_InterruptLock = FALSE;

    pHistory = GetHistory();

    if (pHistory != NULL) {
        RtlZeroMemory(pHistory, sizeof(FX_SPIN_LOCK_HISTORY));

        pHistory->CurrentHistory = &pHistory->History[0];
    }
}


__drv_raisesIRQL(DISPATCH_LEVEL)
__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FxSpinLock::AcquireLock(
    __in PVOID CallersAddress
    )
{
    PFX_SPIN_LOCK_HISTORY pHistory;
    KIRQL irql;

    m_SpinLock.Acquire(&irql);

    m_Irql = irql;

    pHistory = GetHistory();

    if (pHistory != NULL) {
        PFX_SPIN_LOCK_HISTORY_ENTRY pCur;

        //
        // This assert should never fire here, but this helps track ownership
        // in the case of a release without an acquire.
        //
        ASSERT(pHistory->OwningThread == NULL);
        pHistory->OwningThread = Mx::MxGetCurrentThread();

        pCur = pHistory->CurrentHistory;

        Mx::MxQueryTickCount(&pCur->AcquiredAtTime);
        pCur->CallersAddress = CallersAddress;
    }
}

__drv_requiresIRQL(DISPATCH_LEVEL)
VOID
FxSpinLock::ReleaseLock(
    VOID
    )
{
    PFX_SPIN_LOCK_HISTORY pHistory;

    pHistory = GetHistory();

    if (pHistory != NULL) {
        LARGE_INTEGER now;
        PFX_SPIN_LOCK_HISTORY_ENTRY pCur;

        if (pHistory->OwningThread != Mx::MxGetCurrentThread()) {
            if (pHistory->OwningThread == NULL) {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGERROR,
                    "WDFSPINLOCK %p being released by thread 0x%p, but was "
                    "never acquired!", GetObjectHandle(), Mx::MxGetCurrentThread());
            }
            else {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGERROR,
                    "WDFSPINLOCK 0x%p not owned by thread 0x%p, owned by thread 0x%p",
                    GetObjectHandle(), Mx::MxGetCurrentThread(),
                    pHistory->OwningThread);
            }

            FxVerifierBugCheck(GetDriverGlobals(),
                               WDF_INVALID_LOCK_OPERATION,
                               (ULONG_PTR) GetObjectHandle(),
                               0x1);
            //
            // Will not get here
            //
            return;
        }

        ASSERT(pHistory->OwningThread != NULL);

        Mx::MxQueryTickCount(&now);

        pCur = pHistory->CurrentHistory;
        pCur->LockedDuraction = now.QuadPart - pCur->AcquiredAtTime.QuadPart;

        pHistory->CurrentHistory++;
        if (pHistory->CurrentHistory >=
            pHistory->History + FX_SPIN_LOCK_NUM_HISTORY_ENTRIES) {
            pHistory->CurrentHistory = pHistory->History;
        }

        pHistory->OwningThread = NULL;
    }

    m_SpinLock.Release(m_Irql);
}
