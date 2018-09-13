//      TITLE("Spin Locks")
//++
//
// Copyright (c) 1990  Microsoft Corporation
// Copyright (c) 1992  Digital Equipment Corporation
//
// Module Name:
//
//    spinlock.s
//
// Abstract:
//
//    This module implements the routines for acquiring and releasing
//    spin locks.
//
// Author:
//
//    David N. Cutler (davec) 23-Mar-1990
//    Joe Notarangelo 06-Apr-1992
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksalpha.h"

//++
//
// VOID
// KeInitializeSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    )
//
// Routine Description:
//
//    This function initializes an executive spin lock.
//
// Argument:
//
//    SpinLock (a0) - Supplies a pointer to the executive spin lock.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY( KeInitializeSpinLock )

        STP     zero, 0(a0)             // set spin lock not owned
        ret     zero, (ra)              // return

        .end KeInitializeSpinLock

//++
//
// VOID
// KeAcquireSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    OUT PKIRQL OldIrql
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to DISPATCH_LEVEL and acquires
//    the specified executive spinlock.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to a executive spinlock.
//
//    OldIrql  (a1) - Supplies a pointer to a variable that receives the
//        the previous IRQL value.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KeAcquireSpinLock)

//
// Raise IRQL to DISPATCH_LEVEL and acquire the specified spinlock.
//
// N.B. The raise IRQL code is duplicated here is avoid any extra overhead
//      since this is such a common operation.
//
// N.B. The previous IRQL must not be stored until the lock is owned.
//
// N.B. The longword surrounding the previous IRQL must not be read
//      until the lock is owned.
//

        bis     a0, zero, t5            // t5 = address of spin lock
        ldil    a0, DISPATCH_LEVEL      // set new IRQL
        bis     a1, zero, t0            // t0 = a1, a1 may be destroyed

        SWAP_IRQL                       // swap irql, on return v0 = old irql

//
// Acquire the specified spinlock.
//
// N.B. code below branches forward if spinlock fails intentionally
//      because branch forwards are predicted to miss
//

#if !defined(NT_UP)

10:     LDP_L   t3, 0(t5)               // get current lock value
        bis     t5, zero, t4            // set ownership value
        bne     t3, 15f                 // if ne => lock owned
        STP_C   t4, 0(t5)               // set lock owned
        beq     t4, 15f                 // if eq => stx_c failed
        mb                              // synchronize memory access

#endif

//
// Save the old Irql at the address saved by the caller.
// Insure that the old Irql is updated with longword granularity.
//

        ldq_u   t1, 0(t0)               // read quadword surrounding KIRQL
        bic     t0, 3, t2               // get address of containing longword
        mskbl   t1, t0, t1              // clear KIRQL byte in quadword
        insbl   v0, t0, v0              // get new KIRQL to correct byte
        bis     t1, v0, t1              // merge KIRQL into quadword
        extll   t1, t2, t1              // get longword containg KIRQL
        stl     t1, 0(t2)               // store containing longword
        ret     zero, (ra)              // return

//
// Attempt to acquire spinlock failed.
//

#if !defined(NT_UP)

15:     LDP     t3, 0(t5)               // get current lock value
        beq     t3, 10b                 // retry acquire lock if unowned
        br      zero, 15b               // loop in cache until lock free

#endif

        .end    KeAcquireSpinLock

        SBTTL("Acquire SpinLock and Raise to Synch")
//++
//
// KIRQL
// KeAcquireSpinLockRaiseToSynch (
//    IN PKSPIN_LOCK SpinLock
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to synchronization level and
//    acquires the specified spinlock.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to the spinlock that is to be
//        acquired.
//
// Return Value:
//
//    The previous IRQL is returned as the function value.
//
//--

        LEAF_ENTRY(KeAcquireSpinLockRaiseToSynch)

#if !defined(NT_UP)

        bis         a0, zero, t5        // save spinlock address
        ldl         a0, KiSynchIrql     // get synch level IRQL

//
// Raise IRQL and attempt to acquire the specified spinlock.
//

10:     SWAP_IRQL                       // raise IRQL to synch level

        LDP_L       t3, 0(t5)           // get current lock value
        bis         t5, zero, t4        // set ownership value
        bne         t3, 25f             // if ne, lock owned
        STP_C       t4, 0(t5)           // set lock owned
        beq         t4, 25f             // if eq, conditional store failed
        mb                              // synchronize subsequent reads
        ret         zero, (ra)

//
// Spinlock is owned, lower IRQL and spin in cache until it looks free.
//

25:     bis         v0, zero, a0        // get previous IRQL value

        SWAP_IRQL                       // lower IRQL

        bis         v0, zero, a0        // save synch level IRQL
26:     LDP         t3, 0(t5)           // get current lock value
        beq         t3, 10b             // retry acquire if unowned
        br          zero, 26b           // loop in cache until free

#else

        ldl         a0, KiSynchIrql     // get synch level IRQL

        SWAP_IRQL                       // rasie IRQL to synch levcel

        ret         zero, (ra)          // return

        .end    KeAcquireSpinLockRaiseToSynch

#endif

//++
//
// KIRQL
// KeAcquireSpinLockRaiseToDpc (
//    IN PKSPIN_LOCK SpinLock
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to dispatcher level and acquires
//    the specified spinlock.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to the spinlock that is to be
//        acquired.
//
// Return Value:
//
//    The previous IRQL is returned as the function value.
//
//--

#if !defined(NT_UP)

        ALTERNATE_ENTRY(KeAcquireSpinLockRaiseToDpc)

        bis     a0, zero, t5            // save spinlock address
        ldil    a0, DISPATCH_LEVEL      // set IRQL level
        br      10b                     // finish in common code

        .end    KeAcquireSpinLockRaiseToSynch

#else

        LEAF_ENTRY(KeAcquireSpinLockRaiseToDpc)

        ldil    a0, DISPATCH_LEVEL      // set new IRQL

        SWAP_IRQL                       // old irql in v0

        ret     zero, (ra)

        .end    KeAcquireSpinLockRaiseToDpc

#endif

//++
//
// VOID
// KeReleaseSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    IN KIRQL OldIrql
//    )
//
// Routine Description:
//
//    This function releases an executive spin lock and lowers the IRQL
//    to its previous value.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to an executive spin lock.
//
//    OldIrql (a1) - Supplies the previous IRQL value.
//
// Return Value:
//
//    None.
//
//--
        LEAF_ENTRY(KeReleaseSpinLock)

//
// Release the specified spinlock.
//

#if !defined(NT_UP)

        mb                              // synchronize memory access
        STP     zero, 0(a0)             // set spin lock not owned

#endif

//
// Lower the IRQL to the specified level.
//
// N.B. The lower IRQL code is duplicated here is avoid any extra overhead
//      since this is such a common operation.
//

10:     bis     a1, zero, a0            // a0 = new irql

        SWAP_IRQL                       // change to new irql

        ret     zero, (ra)              // return

        .end    KeReleaseSpinLock

//++
//
// BOOLEAN
// KeTryToAcquireSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    OUT PKIRQL OldIrql
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to DISPATCH_LEVEL and attempts
//    to acquires the specified executive spinlock. If the spinlock can be
//    acquired, then TRUE is returned. Otherwise, the IRQL is restored to
//    its previous value and FALSE is returned.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to a executive spinlock.
//
//    OldIrql  (a1) - Supplies a pointer to a variable that receives the
//        the previous IRQL value.
//
// Return Value:
//
//    If the spin lock is acquired, then a value of TRUE is returned.
//    Otherwise, a value of FALSE is returned.
//
//--

        LEAF_ENTRY(KeTryToAcquireSpinLock)

//
// Raise IRQL to DISPATCH_LEVEL and try to acquire the specified spinlock.
//
// N.B. The raise IRQL code is duplicated here is avoid any extra overhead
//      since this is such a common operation.
//

        bis     a0, zero, t5            // t5 = address of spin lock
        ldil    a0, DISPATCH_LEVEL      // new irql
        bis     a1, zero, t11           // t11 = a1, a1 may be clobbered

        SWAP_IRQL                       // a0 = new, on return v0 = old irql

//
// Try to acquire the specified spinlock.
//
// N.B. A noninterlocked test is done before the interlocked attempt. This
//      allows spinning without interlocked cycles.
//

#if !defined(NT_UP)

        LDP     t0, 0(t5)               // get current lock value
        bne     t0, 20f                 // if ne, lock owned
10:     LDP_L   t0, 0(t5)               // get current lock value
        bis     t5, zero, t3            // t3 = ownership value
        bne     t0, 20f                 // if ne, spin lock owned
        STP_C   t3, 0(t5)               // set lock owned
        beq     t3, 15f                 // if eq, store conditional failure
        mb                              // synchronize memory access

#endif

//
// The attempt to acquire the specified spin lock succeeded.
//
// Save the old Irql at the address saved by the caller.
// Insure that the old Irql is updated with longword granularity.
//

        ldq_u   t1, 0(t11)              // read quadword containing KIRQL
        bic     t11, 3, t2              // get address of containing longword
        mskbl   t1, t11, t1             // clear byte position of KIRQL
        bis     v0, zero, a0            // save old irql
        insbl   v0, t11, v0             // get KIRQL to correct byte
        bis     t1, v0, t1              // merge KIRQL into quadword
        extll   t1, t2, t1              // extract containing longword
        stl     t1, 0(t2)               // store containing longword
        ldil    v0, TRUE                // set return value
        ret     zero, (ra)              // return

//
// The attempt to acquire the specified spin lock failed. Lower IRQL to its
// previous value and return FALSE.
//
// N.B. The lower IRQL code is duplicated here is avoid any extra overhead
//      since this is such a common operation.
//

#if !defined(NT_UP)

20:     bis     v0, zero, a0            // set old IRQL value

        SWAP_IRQL                       // change back to old irql(a0)

        ldil    v0, FALSE               // set return to failed
        ret     zero, (ra)              // return

//
// Attempt to acquire spinlock failed.
//

15:     br      zero, 10b               // retry spinlock

#endif

        .end    KeTryToAcquireSpinLock

//++
//
// KIRQL
// KiAcquireSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    )
//
// Routine Description:
//
//    This function acquires a kernel spin lock.
//
//    N.B. This function assumes that the current IRQL is set properly.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to a kernel spin lock.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiAcquireSpinLock)

        ALTERNATE_ENTRY(KeAcquireSpinLockAtDpcLevel)

#if !defined(NT_UP)

        GET_CURRENT_THREAD              // v0 = current thread address

10:     LDP_L   t2, 0(a0)               // get current lock value
        bis     v0, zero, t3            // set ownership value
        bne     t2, 15f                 // if ne, spin lock owned
        STP_C   t3, 0(a0)               // set spin lock owned
        beq     t3, 15f                 // if eq, store conditional failure
        mb                              // synchronize memory access
        ret     zero, (ra)              // return

//
// attempt to acquire spinlock failed.
//

15:     LDP     t2, 0(a0)               // get current lock value
        beq     t2, 10b                 // retry acquire lock if unowned
        br      zero, 15b               // loop in cache until lock free

#else

        ret     zero, (ra)              // return

#endif

        .end    KiAcquireSpinLock

//++
//
// VOID
// KiReleaseSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    )
//
// Routine Description:
//
//    This function releases a kernel spin lock.
//
//    N.B. This function assumes that the current IRQL is set properly.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to an executive spin lock.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiReleaseSpinLock)

        ALTERNATE_ENTRY(KeReleaseSpinLockFromDpcLevel)

#if !defined(NT_UP)

        mb                              // synchronize memory accesss
        STP    zero, 0(a0)             // set spin lock not owned

#endif

        ret     zero, (ra)              // return

        .end    KiReleaseSpinLock

//++
//
// KIRQL
// KiTryToAcquireSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    )
//
// Routine Description:
//
//    This function attempts to acquires the specified kernel spinlock. If
//    the spinlock can be acquired, then TRUE is returned. Otherwise, FALSE
//    is returned.
//
//    N.B. This function assumes that the current IRQL is set properly.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to a kernel spin lock.
//
// Return Value:
//
//    If the spin lock is acquired, then a value of TRUE is returned.
//    Otherwise, a value of FALSE is returned.
//
//--

        LEAF_ENTRY(KiTryToAcquireSpinLock)

#if !defined(NT_UP)

        GET_CURRENT_THREAD              // v0 = current thread address

10:     LDP_L   t2, 0(a0)               // get current lock value
        bis     v0, zero, t3            // set ownership value
        bne     t2, 20f                 // if ne, spin lock owned
        STP_C   t3, 0(a0)               // set spin lock owned
        beq     t3, 15f                 // if eq, conditional store failed
        mb                              // synchronize memory access
        ldil    v0, TRUE                // set success return value
        ret     zero, (ra)              // return

20:     ldil    v0, FALSE               // set failure return value
        ret     zero, (ra)              // return

//
// Attempt to acquire spinlock failed.
//

15:     br      zero, 10b               // retry

#else

        ldil    v0, TRUE                // set success return value
        ret     zero, (ra)              // return

#endif

        .end    KiTryToAcquireSpinLock

//++
//
// BOOLEAN
// KeTestSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    )
//
// Routine Description:
//
//    This function tests a kernel spin lock.  If the spinlock is
//    busy, FALSE is returned.  If not, TRUE is returned.  The spinlock
//    is never acquired.  This is provided to allow code to spin at low
//    IRQL, only raising the IRQL when there is a reasonable hope of
//    acquiring the lock.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to a kernel spin lock.
//
// Return Value:
//
//    TRUE  - Spinlock appears available
//    FALSE - SpinLock is busy
//--

#if !defined(NT_UP)

        LEAF_ENTRY(KeTestSpinLock)

        LDP     t0, (a0)                // get current spinlock value
        ldil    v0, 1                   // default TRUE
        cmovne  t0, zero, v0            // if t0 != 0, return FALSE
        ret     zero, (ra)              // return

        .end    KeTestSpinLock

#endif

        SBTTL("Acquire Queued SpinLock and Raise IRQL")
//++
//
// KIRQL
// KeAcquireQueuedSpinLock (
//    IN KSPIN_LOCK_QUEUE_NUMBER Number
//    )
//
// KIRQL
// KeAcquireQueuedSpinLockRaiseToSynch (
//    IN KSPIN_LOCK_QUEUE_NUMBER Number
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to synchronization level and
//    acquires the specified queued spinlock.
//
// Arguments:
//
//    Number (a0) - Supplies the queued spinlock number.
//
// Return Value:
//
//    The previous IRQL is returned as the function value.
//
//--

#if !defined(NT_UP)

        LEAF_ENTRY(KeAcquireQueuedSpinLock)

        addq    a0, a0, t5              // account for two addresses
        ldil    a0, DISPATCH_LEVEL      // get dispatch level IRQL
        br      zero, 5f                //

        ALTERNATE_ENTRY(KeAcquireQueuedSpinLockRaiseToSynch)

        addq    a0, a0, t5              // account for two addresses
        ldl     a0, KiSynchIrql         // get synch level IRQL

5:      SWAP_IRQL                       // raise Irql to specified level

        bis     v0, zero, t0            // save previous Irql

        GET_PROCESSOR_CONTROL_BLOCK_BASE // get current prcb address

        SPADDP  t5, v0, t5              // entry address
        lda     t5, PbLockQueue(t5)     //
        LDP     t4, LqLock(t5)          // get associated spinlock address
10:     LDP_L   t3, 0(t4)               // get current lock value
        bis     t5, zero, t2            // set lock ownership value
        STP_C   t2, 0(t4)               //
        beq     t2, 50f                 // if eq, conditional store failed
        mb                              // synchronize subsequent reads
        bne     t3, 30f                 // if ne, lock already owned
        bis     t4, LOCK_QUEUE_OWNER, t4 // set lock owner bit in lock entry
        STP     t4, LqLock(t5)          //
20:     bis     t0, zero, v0            // set old IRQL value
        ret     zero, (ra)              // return

//
// The lock is owned by another processor. Set the lock bit in the current
// processor lock queue entry, set the next link in the previous lock queue
// entry, and spin on the current processor's lock bit.
//

30:     bis     t4, LOCK_QUEUE_WAIT, t4 // set lock wait bit in lock entry
        STP     t4, LqLock(t5)          //
        mb                              // synchronize memory access
        STP     t5, LqNext(t3)          // set address of lock queue entry
40:     LDP     t4, LqLock(t5)          // get lock address and lock wait bit
        blbc    t4, 20b                 // if lbc (lock wait), ownership granted
        br      zero, 40b               // try again

//
// Conditional store failed.
//

50:     br      zero, 10b               // try again

        .end    KeAcquireQueuedSpinLock

#endif

        SBTTL("Release Queued SpinLock and Lower IRQL")
//++
//
// VOID
// KeReleaseQueuedSpinLock (
//    IN KSPIN_LOCK_QUEUE_NUMBER Number,
//    IN KIRQL OldIrql
//    )
//
// Routine Description:
//
//    This function releases a queued spinlock and lowers the IRQL to its
//    previous value.
//
// Arguments:
//
//    Number (a0) - Supplies the queued spinlock number.
//
//    OldIrql (a1) - Supplies the previous IRQL value.
//
// Return Value:
//
//    None.
//
//--

#if !defined(NT_UP)

        LEAF_ENTRY(KeReleaseQueuedSpinLock)

        bis     a1, zero, t0            // save old IRQL
        addq    a0, a0, t5              // account for two addresses

        GET_PROCESSOR_CONTROL_BLOCK_BASE // get current prcb address

        SPADDP  t5, v0, t5              // compute per processor lock queue
        lda     t5, PbLockQueue(t5)     // entry address
        mb                              // synchronize memory access
        LDP     t3, LqNext(t5)          // get next lock queue entry address
        LDP     t4, LqLock(t5)          // get associate spin lock address
        bic     t4, LOCK_QUEUE_OWNER, t4 // clear lock owner bit in lock entry
        STP     t4, LqLock(t5)          //
        bne     t3, 30f                 // if ne, another processor waiting
10:     LDP_L   t3, 0(t4)               // get current lock ownership value
        xor     t3, t5, t2              // set lock ownership value
        bne     t2, 20f                 // if ne, another processor waiting
        STP_C   t2, 0(t4)               // set new ownership value
        beq     t2, 40f                 // if eq, conditional store failed
        bis     t0, zero, a0            // set old IRQL value

        SWAP_IRQL                       // lower IRQL to previous level

        ret     zero, (ra)              // return

//
// Another processor has inserted its lock queue entry in the lock queue,
// but has not yet written its lock queue entry address in the current
// processor's next link. Spin until the lock queue address is written.
//

20:     LDP     t3, LqNext(t5)          // get next lock queue entry address
        beq     t3, 50f                 // if eq, address not written yet

//
// Grant the next process in the lock queue ownership of the spinlock.
//

30:     LDP     t2, LqLock(t3)          // get spinlock address and lock bit
        STP     zero, LqNext(t5)        // clear next lock queue entry address
        bic     t2, LOCK_QUEUE_WAIT, t2 // clear lock wait bit in lock entry
        bis     t2, LOCK_QUEUE_OWNER, t2 // set lock owner bit in lock entry
        STP     t2, LqLock(t3)          //
        mb                              // synchronize memory access
        bis     t0, zero, a0            // set old IRQL value

        SWAP_IRQL                       // lower IRQL to previous level

        ret     zero, (ra)              // return

//
// Store conditional failed.
//

40:     br      zero, 10b               // try again

//
// Next lock queue entry address NULL.
//

50:     br      zero, 20b               //

        .end    KeReleaseQueuedSpinLock

#endif

        SBTTL("Try to Acquire Queued SpinLock and Raise IRQL")
//++
//
// LOGICAL
// KeTryToAcquireQueuedSpinLock (
//    IN KSPIN_LOCK_QUEUE_NUMBER Number
//    OUT PKIRQL OldIrql
//    )
//
// LOGICAL
// KeTryToAcquireQueuedSpinLockRaiseToSynch (
//    IN KSPIN_LOCK_QUEUE_NUMBER Number
//    OUT PKIRQL OldIrql
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to synchronization level and
//    attempts to acquire the specified queued spinlock. If the spinlock
//    cannot be acquired, then IRQL is restored and FALSE is returned as
//    the function value. Otherwise, TRUE is returned as the function
//    value.
//
// Arguments:
//
//    Number (a0) - Supplies the queued spinlock number.
//
//    OldIrql  (a1) - Supplies a pointer to a variable that receives the
//        the previous IRQL value.
//
// Return Value:
//
//    If the spin lock is acquired, then a value of TRUE is returned.
//    Otherwise, a value of FALSE is returned.
//
//--

#if !defined(NT_UP)

        LEAF_ENTRY(KeTryToAcquireQueuedSpinLock)

        addq    a0, a0, t5              // account for two addresses
        ldil    a0, DISPATCH_LEVEL      // get dispatch level irql
        br      zero, 5f                //

        ALTERNATE_ENTRY(KeTryToAcquireQueuedSpinLockRaiseToSynch)

        addq    a0, a0, t5              // account for two addresses
        ldl     a0, KiSynchIrql         // get synch level irql
5:      bis     a1, zero, t0            // save previous irql address

        SWAP_IRQL                       // raise irql to specified level

        bis     v0, zero, t1            // save previous irql

        GET_PROCESSOR_CONTROL_BLOCK_BASE // get current prcb address

        SPADDP  t5, v0, t5              // compute per processor lock queue
        lda     t5, PbLockQueue(t5)     // entry address
        LDP     t4, LqLock(t5)          // get associated spinlock address

//
// Try to acquire the specified spinlock.
//
// N.B. A noninterlocked test is done before the interlocked attempt. This
//      allows spinning without interlocked cycles.
//

        LDP     t3, 0(t4)               // get current lock value
        bne     t3, 20f                 // if ne, lock owned
10:     LDP_L   t3, 0(t4)               // get current lock value
        bis     t5, zero, t2            // set lock ownership value
        bne     t3, 20f                 // if ne, spin lock owned
        STP_C   t2, 0(t4)               // set lock owned
        beq     t2, 30f                 // if eq, store conditional failure
        mb                              // synchronize memory access

//
// The attempt to acquire the specified spin lock succeeded. Set the spin
// lock owner bit and save the old irql at the address specified by the
// caller. Insure that the old Irql is updated with longword granularity.
//

        bis     t4, LOCK_QUEUE_OWNER, t4 // set lock owner bit in lock entry
        STP     t4, LqLock(t5)          //
        ldq_u   t2, 0(t0)               // get quadword containing irql
        bic     t0, 3, t3               // get containing longword address
        mskbl   t2, t0, t2              // clear byte position of Irql
        insbl   t1, t0, t1              // shift irql to correct byte
        bis     t2, t1, t2              // merge irql into quadword
        extll   t2, t3, t2              // extract containing longword
        stl     t2, 0(t3)               // store containing longword
        ldil    v0, TRUE                // set return value
        ret     zero, (ra)              // return

//
// The attempt to acquire the specified spin lock failed. Lower IRQL to its
// previous value and return FALSE.
//

20:     bis     t1, zero, a0            // set old irql value

        SWAP_IRQL                       // lower irql to previous level

        ldil    v0, FALSE               // set return value
        ret     zero, (ra)              // return

//
// Attempt to acquire spinlock failed.
//

30:     br      zero, 10b               // retry spinlock

        .end    KeTryToAcquireQueuedSpinLock

#endif

        SBTTL("Acquire Queued SpinLock at Current IRQL")
//++
//
// VOID
// KiAcquireQueuedSpinLock (
//    IN PKSPIN_LOCK_QUEUE LockQueue
//    )
//
// Routine Description:
//
//    This function acquires the specified queued spinlock at the current
//    IRQL.
//
// Arguments:
//
//    LockQueue (a0) - Supplies the address of the lock queue entry.
//
// Return Value:
//
//    None.
//
//--

#if !defined(NT_UP)

        LEAF_ENTRY(KiAcquireQueuedSpinLock)

        LDP     t4, LqLock(a0)          // get associated spinlock address
10:     LDP_L   t3, 0(t4)               // get current lock value
        bis     a0, zero, t2            // set lock ownership value
        STP_C   t2, 0(t4)               //
        beq     t2, 50f                 // if eq, conditional store failed
        mb                              // synchronize subsequent reads
        bne     t3, 30f                 // if ne, lock already owned
        bis     t4, LOCK_QUEUE_OWNER, t4 // set lock owner bit in lock entry
        STP     t4, LqLock(a0)          //
20:     ret     zero, (ra)              // return

//
// The lock is owned by another processor. Set the lock bit in the current
// processor lock queue entry, set the next link in the previous lock queue
// entry, and spin on the current processor's lock bit.
//

30:     bis     t4, LOCK_QUEUE_WAIT, t4 // set lock wait bit in lock entry
        STP     t4, LqLock(a0)          //
        mb                              // synchronize memory access
        STP     a0, LqNext(t3)          // set address of lock queue entry
40:     LDP     t4, LqLock(a0)          // get lock address and lock wait bit
        blbc    t4, 20b                 // if lbc (lock wait), ownership granted
        br      zero, 40b               // try again

//
// Conditional store failed.
//

50:     br      zero, 10b               // try again

        .end    KiAcquireQueuedSpinLock

#endif

        SBTTL("Release Queued SpinLock at Current IRQL")
//++
//
// VOID
// KiReleaseQueuedSpinLock (
//    IN PKSPIN_LOCK_QUEUE LockQueue
//    )
//
// Routine Description:
//
//    This function releases a queued spinlock and preserves the current
//    IRQL.
//
// Arguments:
//
//    LockQueue (a0) - Supplies the address of the lock queue entry.
//
// Return Value:
//
//    None.
//
//--

#if !defined(NT_UP)

        LEAF_ENTRY(KiReleaseQueuedSpinLock)

        mb                              // synchronize memory access
        LDP     t3, LqNext(a0)          // get next lock queue entry address
        LDP     t4, LqLock(a0)          // get associate spin lock address
        bic     t4, LOCK_QUEUE_OWNER, t4 // clear lock owner bit in lock entry
        STP     t4, LqLock(a0)          //
        bne     t3, 30f                 // if ne, another processor waiting
10:     LDP_L   t3, 0(t4)               // get current lock ownership value
        xor     t3, a0, t2              // set lock ownership value
        bne     t2, 20f                 // if ne, another processor waiting
        STP_C   t2, 0(t4)               // set new ownership value
        beq     t2, 40f                 // if eq, conditional store failed
        ret     zero, (ra)              // return

//
// Another processor has inserted its lock queue entry in the lock queue,
// but has not yet written its lock queue entry address in the current
// processor's next link. Spin until the lock queue address is written.
//

20:     LDP     t3, LqNext(a0)          // get next lock queue entry address
        beq     t3, 50f                 // if eq, address not written yet

//
// Grant the next process in the lock queue ownership of the spinlock.
//

30:     LDP     t2, LqLock(t3)          // get spinlock address and lock bit
        STP     zero, LqNext(a0)        // clear next lock queue entry address
        bic     t2, LOCK_QUEUE_WAIT, t2 // clear lock wait bit in lock entry
        bis     t2, LOCK_QUEUE_OWNER, t2 // set lock owner bit in lock entry
        STP     t2, LqLock(t3)          //
        mb                              // synchronize memory access
        ret     zero, (ra)              // return

//
// Store conditional failed.
//

40:     br      zero, 10b               // try again

//
// Next lock queue entry address NULL.
//

50:     br      zero, 20b               //

        .end    KiReleaseQueuedSpinLock

#endif
