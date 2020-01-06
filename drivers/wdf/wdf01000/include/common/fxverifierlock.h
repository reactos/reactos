#ifndef _FXVERIFIERLOCK_H_
#define _FXVERIFIERLOCK_H_

#include "common/fxglobals.h"

/*
 * This lock performs the same actions as the base
 * FxLock, but provides additional tracking of lock
 * order to help detect and debug
 * deadlock and recursive lock situations.
 *
 * It does not inherit from FxLock since this would
 * cause a recursion chain.
 *
 * Since lock verification is already more costly than
 * basic lock use, this verifier lock supports both the
 * FAST_MUTEX and SpinLock modes in order to utilize common
 * code to support verification of FxCallback locks.
 *
 * FxVerifierLocks are only allocated when WdfVerifierLock
 * is turned on.
 */


//
// This must be a power of two for hash algorithm
//
// Table size is dependent on the number of active threads
// in the frameworks at a given time. This can not execeed
// the number of (virtual due to hyperthreading) CPU's in the system.
//
// Setting this number lower just increases collisions, but does
// not impede correct function.
//
#define VERIFIER_THREAD_HASHTABLE_SIZE 64

class FxVerifierLock {

public:

    static
    void
    FreeThreadTable(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    static
    void
    AllocateThreadTable(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

};

#endif //_FXVERIFIERLOCK_H_