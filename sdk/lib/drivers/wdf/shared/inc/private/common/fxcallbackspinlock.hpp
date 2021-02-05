/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxCallbackSpinLock.hpp

Abstract:

    This is the C++ header for the FxCallbackSpinLock

    This is the spinlock based driver callback lock


Author:




Revision History:


--*/

#ifndef _FXCALLBACKSPINLOCK_H_
#define _FXCALLBACKSPINLOCK_H_

extern "C" {

#if defined(EVENT_TRACING)
#include "FxCallbackSpinLock.hpp.tmh"
#endif

}

class FxCallbackSpinLock : public FxCallbackLock {

private:
    MxLock m_Lock;

public:

    FxCallbackSpinLock(
        PFX_DRIVER_GLOBALS FxDriverGlobals
        ) :
        FxCallbackLock(FxDriverGlobals)
    {
    }

    virtual
    ~FxCallbackSpinLock(
        VOID
        )
    {
        if (m_Verifier) {
            delete m_Verifier;
        }
    }

    virtual
    void
    Initialize(
        FxObject* ParentObject
        )
    {
        PFX_DRIVER_GLOBALS fxDriverGlobals;

        m_Verifier = NULL;
        fxDriverGlobals = GetDriverGlobals();

        if (fxDriverGlobals->FxVerifierLock) {

            //
            // VerifierLock CreateAndInitialize failure is not fatal,
            // we just won't track anything
            //
            (void) FxVerifierLock::CreateAndInitialize(&m_Verifier,
                                                       fxDriverGlobals,
                                                       ParentObject,
                                                       FALSE);

        }
    }

    virtual
    void
    Lock(
        __out PKIRQL PreviousIrql
        )
    {
        MxThread cur = Mx::MxGetCurrentThread();

        if (m_OwnerThread == cur) {
            PFX_DRIVER_GLOBALS fxDriverGlobals;

            fxDriverGlobals = GetDriverGlobals();
            m_RecursionCount++;

            //
            // For now we have decided not to allow this.
            //





            //
            DoTraceLevelMessage(fxDriverGlobals, TRACE_LEVEL_FATAL, TRACINGDEVICE,
                                "Recursive acquire of callback lock! 0x%p", this);

            FxVerifierBugCheck(fxDriverGlobals,
                               WDF_RECURSIVE_LOCK,
                               (ULONG_PTR) this);
            return;
        }
        else {
            if (m_Verifier != NULL) {
                m_Verifier->Lock(PreviousIrql, FALSE);
            }
            else {
                m_Lock.Acquire(PreviousIrql);
            }

            m_OwnerThread = cur;
            return;
        }
    }

    virtual
    void
    Unlock(
        __in KIRQL PreviousIrql
        )
    {
        CheckOwnership();

        if (m_RecursionCount > 0) {
            m_RecursionCount--;
        }
        else {
            m_OwnerThread = NULL;
            if (m_Verifier != NULL) {
                m_Verifier->Unlock(PreviousIrql, FALSE);
            }
            else {
                m_Lock.Release(PreviousIrql);
            }
        }
    }

    _Must_inspect_result_
    inline
    virtual
    BOOLEAN
    IsOwner(
        VOID
        )
    {
        MxThread cur = Mx::MxGetCurrentThread();

        if (m_OwnerThread == cur) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }
};

#endif // _FXCALLBACKSPINLOCK_H_
