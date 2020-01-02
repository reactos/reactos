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

class FxVerifierLock {

public:

    static
    void
    FreeThreadTable(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

};

#endif //_FXVERIFIERLOCK_H_