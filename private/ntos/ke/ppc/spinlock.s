//      TITLE("Spin Locks")
//++
//
// Copyright (c) 1993  IBM Corporation
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
//    Peter L. Johnston (plj@vnet.ibm.com) 28-Jun-93
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksppc.h"

        .extern ..KxReleaseSpinLock


        SBTTL("Initialize Executive Spin Lock")
//++
//
// VOID
// KeInitializeSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    )
//
// Routine Description:
//
//    This function initialzies an executive spin lock.
//
// Arguments:
//
//    SpinLock (r3) - Supplies a pointer to a executive spinlock.
//
// Return Value:
//
//    None.
//
// Remarks:
//
//    Equivalent C code for body of function.
//      {
//              *spinlock = 0;
//      }
//
//    Why is this in assembly code?  I suspect simply so it is bundled
//    with the rest of the spin lock code.
//--

        LEAF_ENTRY(KeInitializeSpinLock)

        li      r.0, 0                  // clear spin lock value by storing
        stw     r.0, 0(r.3)             // zero at address from parameter.

        LEAF_EXIT(KeInitializeSpinLock) // return

        SBTTL("Acquire Executive Spin Lock")
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
//    SpinLock (r3) - Supplies a pointer to an executive spinlock.
//
//    OldIrql  (r4) - Supplies a pointer to a variable that receives the
//        the previous IRQL value.
//
// Return Value:
//
//    None.
//
// Remarks:
//
//    In the UniProcessor version, we just raise IRQL, the lock address
//    is never touched.
//
//    In the MultiProcessor version the spinlock is taken after raising
//    IRQL.
//
//    N.B.  The old IRQL must be stored AFTER the lock is acquired.
//
//--

        LEAF_ENTRY_S(KeAcquireSpinLock,_TEXT$01)

#if !defined(NT_UP)

        DISABLE_INTERRUPTS(r.5, r.6)

        lwz     r.12, KiPcr+PcCurrentThread(r.0) // addr of current thread

        ACQUIRE_SPIN_LOCK(r.3, r.12, r.11, kas.10, kas.15)

#endif

        li      r.0, DISPATCH_LEVEL             // new IRQL = DISPATCH_LEVEL
        lbz     r.10,KiPcr+PcCurrentIrql(r.0)   // get current IRQL
        stb     r.0, KiPcr+PcCurrentIrql(r.0)   // set new IRQL

#if !defined(NT_UP)
        ENABLE_INTERRUPTS(r.5)
#endif

        stb     r.10, 0(r.4)                    // return old IRQL
        blr

#if !defined(NT_UP)
        SPIN_ON_SPIN_LOCK_ENABLED(r.3, r.11, kas.10, kas.15, kas.17, r.5, r.6)
#endif

        DUMMY_EXIT(KeAcquireSpinLock)

        SBTTL("Acquire SpinLock and Raise to DPC")
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
//    SpinLock (r3) - Supplies a pointer to the spinlock that is to be
//        acquired.
//
// Return Value:
//
//    The previous IRQL is returned at the function value.
//
// Remarks:
//
//    In the UniProcessor version, we just raise IRQL, the lock address
//    is never touched.
//
//    In the MultiProcessor version the spinlock is taken after raising
//    IRQL.
//
//    N.B.  The old IRQL must be stored AFTER the lock is acquired.
//
//--

        LEAF_ENTRY_S(KeAcquireSpinLockRaiseToDpc,_TEXT$01)

//
// On PPC, synchronization level is the same as dispatch level.
//

        ALTERNATE_ENTRY(KeAcquireSpinLockRaiseToSynch)

#if !defined(NT_UP)

        DISABLE_INTERRUPTS(r.5, r.6)

        lwz     r.12, KiPcr+PcCurrentThread(r.0) // addr of current thread

        ACQUIRE_SPIN_LOCK(r.3, r.12, r.11, kasrtd.10, kasrtd.15)

#endif

        li      r.0, DISPATCH_LEVEL             // new IRQL = DISPATCH_LEVEL
        lbz     r.10,KiPcr+PcCurrentIrql(r.0)   // get current IRQL
        stb     r.0, KiPcr+PcCurrentIrql(r.0)   // set new IRQL

#if !defined(NT_UP)
        ENABLE_INTERRUPTS(r.5)
#endif

        ori     r.3, r.10, 0                    // return old IRQL
        blr

#if !defined(NT_UP)
        SPIN_ON_SPIN_LOCK_ENABLED(r.3, r.11, kasrtd.10, kasrtd.15, kasrtd.20, r.5, r.6)
#endif

        DUMMY_EXIT(KeAcquireSpinLockRaiseToDpc)

        SBTTL("Release Executive Spin Lock")
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
//    N.B. This routine is entered at DISPATCH_LEVEL.
//
// Arguments:
//
//    SpinLock (r.3) - Supplies a pointer to an executive spin lock.
//
//    OldIrql  (r.4) - Supplies the previous IRQL value.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY_S(KeReleaseSpinLock, _TEXT$01)

        cmpwi   r.4, DISPATCH_LEVEL             // check if new IRQL < DISPATCH

#if !defined(NT_UP)
        li      r.0, 0
        RELEASE_SPIN_LOCK(r.3, r.0)
#endif

        bgelr                                   // if target IRQL >= DISPATCH
                                                // just return
        DISABLE_INTERRUPTS(r.7,r.10)
        lhz     r.10, KiPcr+PcSoftwareInterrupt(r.0) // s/w interrupt pending?
        stb     r.4, KiPcr+PcCurrentIrql(r.0)   // set target IRQL
        cmpw    cr.6, r.10, r.4
        srwi.   r.5, r.10, 8                    // isolate DPC pending
        cmpwi   cr.7, r.4, APC_LEVEL            // compare IRQL to APC_LEVEL

//
// Possible values for SoftwareInterrupt (r.10) are
//
//   0x0101     DPC and APC interrupt pending
//   0x0100     DPC interrupt pending
//   0x0001     APC interrupt pending
//   0x0000     No software interrupt pending (unlikely but possible)
//
// Possible values for current IRQL are zero or one.  By comparing
// SoftwareInterrupt against current IQRL (above) we can quickly see
// if any software interrupts are valid at this time.
//
// Calculate correct IRQL for the interrupt we are processing.  If DPC
// then we need to be at DISPATCH_LEVEL which is one greater than APC_
// LEVEL.  r.5 contains one if we are going to run a DPC, so we add
// APC_LEVEL to r.5 to get the desired IRQL.
//

        addi    r.4, r.5, APC_LEVEL             // calculate new IRQL

        ble     cr.6,Enable                     // jif no valid interrupt

//
// A software interrupt is pending and the new IRQL allows it to be
// taken at this time.  Branch directly to KxReleaseSpinLock (ctxswap.s)
// to dispatch the interrupt.  KxReleaseSpinLock returns directly to
// KeReleaseSpinLock's caller.
//
// WARNING: KxReleaseSpinLock is dependent on the values in r.4, r.7, r.12
//          and condition register fields cr.0 and cr.7.
//

        b       ..KxReleaseSpinLock

Enable: ENABLE_INTERRUPTS(r.7)

        LEAF_EXIT(KeReleaseSpinLock)            // return

        SBTTL("Try To Acquire Executive Spin Lock")
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
//    SpinLock (r.3) - Supplies a pointer to a executive spinlock.
//
//    OldIrql  (r.4) - Supplies a pointer to a variable that receives the
//        the previous IRQL value.
//
// Return Value:
//
//    If the spin lock is acquired, then a value of TRUE is returned.
//    Otherwise, a value of FALSE is returned.   UP systems always succeed.
//    On MP systems we test if the lock is taken with interrupts disabled,
//    so that we don't have to check for any DPCs/APCs that could be queued
//    while we have priority at dispatch level.
//
//    N.B.  The old IRQL must be stored AFTER the lock is acquired.
//
//--

        LEAF_ENTRY_S(KeTryToAcquireSpinLock,_TEXT$01)

#if !defined(NT_UP)

//
// Try to acquire the specified spinlock.
//

        DISABLE_INTERRUPTS(r.9, r.8)

        lwz     r.12, KiPcr+PcCurrentThread(r.0) // addr of current thread

        TRY_TO_ACQUIRE_SPIN_LOCK(r.3, r.12, r.11, ktas.10, ktas.20)

#endif

//
// Raise IRQL and indicate success.
//

        lbz     r.10,KiPcr+PcCurrentIrql(r.0)   // get current IRQL
        li      r.0, DISPATCH_LEVEL             // new IRQL = DISPATCH_LEVEL
        li      r.3, TRUE                       // set return value (success)
        stb     r.10, 0(r.4)                    // return old IRQL
        stb     r.0, KiPcr+PcCurrentIrql(r.0)   // set new IRQL

#if !defined(NT_UP)

        ENABLE_INTERRUPTS(r.9)

        blr                                     // return

//
// The attempt to acquire the specified spin lock failed.  Indicate failure.
//

ktas.20:
        ENABLE_INTERRUPTS(r.9)

        li      r.3, FALSE                      // set return value (failure)

#endif

        LEAF_EXIT(KeTryToAcquireSpinLock)

        SBTTL("Acquire Kernel Spin Lock")
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

        LEAF_ENTRY_S(KiAcquireSpinLock,_TEXT$01)

        ALTERNATE_ENTRY(KeAcquireSpinLockAtDpcLevel)

#if !defined(NT_UP)
        lwz     r.12, KiPcr+PcCurrentThread(r.0) // addr of current thread

        ACQUIRE_SPIN_LOCK(r.3, r.12, r.11, kas.20, kas.30)
#endif

        blr

#if !defined(NT_UP)
        SPIN_ON_SPIN_LOCK(r.3, r.11, kas.20, kas.30)
#endif

        DUMMY_EXIT(KiAcquireSpinLock)

        SBTTL("Release Kernel Spin Lock")
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

        LEAF_ENTRY_S(KiReleaseSpinLock,_TEXT$01)

        ALTERNATE_ENTRY(KeReleaseSpinLockFromDpcLevel)

#if !defined(NT_UP)
        li      r.0, 0
        RELEASE_SPIN_LOCK(r.3, r.0)
#endif

        LEAF_EXIT(KiReleaseSpinLock)            // return

        SBTTL("Try To Acquire Kernel Spin Lock")
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
//    SpinLock (r.3) - Supplies a pointer to a kernel spin lock.
//
// Return Value:
//
//    If the spin lock is acquired, then a value of TRUE is returned.
//    Otherwise, a value of FALSE is returned.
//
//--

        LEAF_ENTRY_S(KiTryToAcquireSpinLock,_TEXT$01)

#if defined(NT_UP)

        li      r.3, TRUE                       // set return value (success)

#else

        lwz     r.12, KiPcr+PcCurrentThread(r.0) // addr of current thread

        TRY_TO_ACQUIRE_SPIN_LOCK(r.3, r.12, r.11, kitas.10, kitas.20)
        li      r.3, TRUE                       // set return value (success)

        blr                                     // return

kitas.20:
        li      r.3, FALSE                      // set return value (failure)
#endif
        LEAF_EXIT(KiTryToAcquireSpinLock)


#if !defined(NT_UP)
#if SPINDBG
        .struct 0
        .space  StackFrameHeaderLength
kasdR5: .space  4                               // saved R5
kasdR6: .space  4                               // saved R6
kasdR7: .space  4                               // saved R7
kasdR8: .space  4                               // saved R8
        .align  3                               // 8 byte align
kasdFrameLength:

        SPECIAL_ENTRY_S(KiAcquireSpinLockDbg,_TEXT$01)
        stwu    sp,-kasdFrameLength(sp)
        stw     r5,kasdR5(sp)                   // save r5
        stw     r6,kasdR6(sp)                   // save r6
        stw     r7,kasdR7(sp)                   // save r7
        stw     r8,kasdR8(sp)                   // save r8
        PROLOGUE_END(KiAcquireSpinLockDbg)
        lwz     r5,[toc]KiSpinLockLimit(r.toc)
        lwz     r5,0(r5)
        mflr    r7
        CHKBRK(r6,kasd.05)
        CHKLOCK(r6,r3,kasd.10)
        DBGSTORE_IRR(r6,r8,0x7711,r4,r7)
kasd.10:
        lwarx   r6,0,r3                         // load word at (r.3) and reserve
        cmpwi   r6,0                            // check if locked
        bne-    kasd.20                         // jif locked (predict NOT taken)
        stwcx.  r4,0,r3                         // set lock owned
        bne-    kasd.20                         // try again if store failed
        isync                                   // allow no readahead
        CHKLOCK(r6,r3,kasd.11)
        DBGSTORE_IRR(r6,r8,0x7712,r4,r7)
kasd.11:
        lwz     r8,kasdR8(sp)                   // restore r8
        lwz     r7,kasdR7(sp)                   // restore r7
        lwz     r6,kasdR6(sp)                   // restore r6
        lwz     r5,kasdR5(sp)                   // restore r5
        addi    sp,sp,kasdFrameLength           // deallocate stack frame
        blr                                     // return
kasd.20:
        subi    r5,r5,1
        cmpwi   r5,0
        beq-    kasd.30
kasd.25:
        lwz     r6,0(r3)
        cmpwi   r6,0
        beq+    kasd.10
        b       kasd.20
kasd.30:
        //DBGSTORE_IRRR(r6,r8,0x7713,r3,r4,r7)
        twi     31,0,0x16
        lwz     r5,[toc]KiSpinLockLimit(r.toc)
        lwz     r5,0(r5)
        b       kasd.25
        DUMMY_EXIT(KiAcquireSpinLockDbg)

        SPECIAL_ENTRY_S(KiTryToAcquireSpinLockDbg,_TEXT$01)
        stwu    sp,-kasdFrameLength(sp)
        stw     r5,kasdR5(sp)                   // save r5
        stw     r6,kasdR6(sp)                   // save r6
        stw     r7,kasdR7(sp)                   // save r7
        stw     r8,kasdR8(sp)                   // save r8
        PROLOGUE_END(KiTryToAcquireSpinLockDbg)
        lwz     r5,[toc]KiSpinLockLimit(r.toc)
        lwz     r5,0(r5)
        ori     r6,r3,0
        mflr    r7
        CHKBRK(r3,ktasd.05)
        CHKLOCK(r3,r6,ktasd.10)
        DBGSTORE_IRR(r3,r8,0x7721,r4,r7)
ktasd.10:
        lwarx   r3,0,r6                         // load word at (r.3) and reserve
        cmpwi   r3,0                            // check if locked
        bne-    ktasd.40                        // jif locked (predict NOT taken)
        stwcx.  r4,0,r6                         // set lock owned
        bne-    ktasd.20                        // try again if store failed
        isync                                   // allow no readahead
        li      r3,TRUE
ktasd.15:
        CHKLOCK(r8,r6,ktasd.16)
        DBGSTORE_IRR(r6,r8,0x7722,r4,r7)
ktasd.16:
        lwz     r8,kasdR8(sp)                   // restore r8
        lwz     r7,kasdR7(sp)                   // restore r7
        lwz     r6,kasdR6(sp)                   // restore r6
        lwz     r5,kasdR5(sp)                   // restore r5
        addi    sp,sp,kasdFrameLength           // deallocate stack frame
        blr                                     // return
ktasd.20:
        subi    r5,r5,1
        cmpwi   r5,0
        bne+    ktasd.10
ktasd.30:
        //DBGSTORE_IRRR(r3,r8,0x7723,r3,r4,r7)
        twi     31,0,0x16
        lwz     r5,[toc]KiSpinLockLimit(r.toc)
        lwz     r5,0(r5)
        b       ktasd.10
ktasd.40:
        li      r3,FALSE
        b       ktasd.15
        DUMMY_EXIT(KiTryToAcquireSpinLockDbg)
#endif
#endif
