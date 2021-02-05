/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxLock.hpp

Abstract:

    This is the C++ header for the FxLock

    This represents a container for handling locking

Author:




Revision History:


--*/

#ifndef _FXLOCK_H_
#define _FXLOCK_H_

/**
 * This is the base lock object implementation
 *
 * This is intended to be embedded in an FxNonPagedObject
 * rather than forcing a separate allocation for
 * non-verifier mode.
 *
 * In order to reduce the runtime memory cost of
 * building in verifier support for a retail version,
 * a single pointer it stored to FxVerifierLock
 * if verifier is on. If this pointer is != NULL,
 * lock calls are proxied to this lock function,
 * leaving our internal spinlock redundent.
 *
 * The decision is to minimize the non-verifier
 * memory footprint so we do not have to compile
 * it out for retail builds, but always have it
 * available through a driver debug setting.
 */

class FxLock {

private:
    MxLock m_lock;

    // For Verifier
    FxVerifierLock* m_Verifier;

public:
    FxLock(
        VOID
        )
    {




        m_lock.Initialize();
        m_Verifier = NULL;
    }

    VOID
    Initialize(
        __in FxObject * ParentObject
        );

    ~FxLock()
    {
        if (m_Verifier != NULL) {
            delete m_Verifier;
        }
    }

    _When_(this->m_Verifier == NULL, _Acquires_lock_(this->m_lock))
    inline
    VOID
    Lock(
        __out PKIRQL PreviousIrql
        )
    {
        if (m_Verifier != NULL) {
            m_Verifier->Lock(PreviousIrql, FALSE);
        }
        //  else if (PreviousIrql == NULL) {
         //     KeAcquireSpinLockAtDpcLevel(&m_lock);
        //  }
        else {
            m_lock.Acquire(PreviousIrql);
        }
    }

    _When_(this->m_Verifier == NULL, _Releases_lock_(this->m_lock))
    inline
    void
    Unlock(
        __in KIRQL PreviousIrql
        )
    {
        if (m_Verifier != NULL) {
            m_Verifier->Unlock(PreviousIrql, FALSE);
        }
        // else if (AtDpc) {
        //     KeReleaseSpinLockFromDpcLevel(&m_lock);
        // }
        else {
            m_lock.Release(PreviousIrql);
        }
    }

    _When_(this->m_Verifier == NULL, _Acquires_lock_(this->m_lock))
    inline
    VOID
    LockAtDispatch(
        VOID
        )
    {
        if (m_Verifier != NULL) {
            KIRQL previousIrql;

            m_Verifier->Lock(&previousIrql, TRUE);
        }
        else {
            m_lock.AcquireAtDpcLevel();
        }
    }

   _When_(this->m_Verifier == NULL, _Releases_lock_(this->m_lock))
   inline
    void
    UnlockFromDispatch(
        VOID
        )
    {
        if (m_Verifier != NULL) {
            m_Verifier->Unlock(DISPATCH_LEVEL, TRUE);
        }
        else {
            m_lock.ReleaseFromDpcLevel();
        }
    }
};

#endif // _FXLOCK_H_
