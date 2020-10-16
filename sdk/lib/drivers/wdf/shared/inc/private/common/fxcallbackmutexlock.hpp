/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxCallbackMutexLock.hpp

Abstract:

    This is the C++ header for the FxCallbackMutexLock

    This represents a container for handling locking
    when we callback into the device driver at thread
    level.

Author:




Revision History:


--*/

#ifndef _FXCALLBACKMUTEXLOCK_H_
#define _FXCALLBACKMUTEXLOCK_H_

extern "C" {
// #include "FxCallbackMutexLock.hpp.tmh"
}

class FxCallbackMutexLock : public FxCallbackLock {

private:







    MxPagedLock m_Lock;

public:

    FxCallbackMutexLock(
        PFX_DRIVER_GLOBALS FxDriverGlobals
        ) :
        FxCallbackLock(FxDriverGlobals)
    {
        m_Lock.Initialize();
    }

    virtual
    ~FxCallbackMutexLock()
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
                                                       TRUE);
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
                                "Recursive acquire of callback lock 0x%p", this);

            FxVerifierBugCheck(fxDriverGlobals,
                               WDF_RECURSIVE_LOCK,
                               (ULONG_PTR)this);
        }
        else {
            if (m_Verifier != NULL) {
                m_Verifier->Lock(PreviousIrql, FALSE);
            }
            else {
                Mx::MxEnterCriticalRegion();
                m_Lock.AcquireUnsafe();
            }
            m_OwnerThread = cur;
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
            if (m_Verifier) {
                m_Verifier->Unlock(PreviousIrql, FALSE);
            }
            else {
                m_Lock.ReleaseUnsafe();
                Mx::MxLeaveCriticalRegion();
            }
        }
    }

    _Must_inspect_result_
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

#endif // _FXCALLBACKMUTEXLOCK_H_
