//++
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
//    William K. Cheung (wcheung) 29-Sep-1995
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//    31-Dec-1998  wc   Updated to use xchg8
//
//    07-Jul-1997  bl   Updated to EAS2.3
//
//    08-Feb-1996       Updated to EAS2.1
//
//--

#include "ksia64.h"

        .file       "spinlock.s"

//
// Globals
//

        PublicFunction(KiCheckForSoftwareInterrupt)


//++
//
// VOID
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

        ACQUIRE_SPINLOCK(a0,a0,Kiasl10)

        br.ret.dptk brp
        ;;

#else

        LEAF_RETURN

#endif // !defined(NT_UP)

        LEAF_EXIT(KiAcquireSpinLock)


//++
//
// BOOLEAN
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
// N.B. The caller KeTryToAcquireSpinLock implicitly depends on the 
//      contents of predicate registers pt1 & pt2.
//
//--

        LEAF_ENTRY(KiTryToAcquireSpinLock)

#if !defined(NT_UP)

        ARGPTR      (a0)                        // swizzle spinlock pointer

        xchg8.nt1   t0 = [a0], a0
        ;;
        cmp.ne      pt0 = t0, zero              // if ne, lock acq failed
        mov         v0 = TRUE                   // acquire assumed succeed
        ;;

        nop.m       0
  (pt0) mov         v0 = FALSE                  // return FALSE

#else
        mov         v0 = TRUE
#endif

        LEAF_RETURN
        LEAF_EXIT(KiTryToAcquireSpinLock)


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
//    SpinLock (a0) - Supplies a pointer to a executive spinlock.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KeInitializeSpinLock)

        ARGPTR      (a0)                        // swizzle spinlock pointer
        st8.rel.nta [a0] = zero                 // clear spin lock value

        LEAF_RETURN
        LEAF_EXIT(KeInitializeSpinLock)

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
//    N.B. The Old IRQL MUST be stored after the lock is acquired.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KeAcquireSpinLock)

//
// Register aliases
//

        rOldIrql    = t2
        

//
// Get original IRQL, raise IRQL to DISPATCH_LEVEL
// and then acquire the specified spinlock.
//

        GET_IRQL    (rOldIrql)                  // get original IRQL
        SET_IRQL    (DISPATCH_LEVEL)            // raise to DISPATCH_LEVEL

#if !defined(NT_UP)

        ACQUIRE_SPINLOCK(a0,a0,Kasl10)

#endif  // !defined(NT_UP)

        st1         [a1] = rOldIrql             // save old IRQL
        LEAF_RETURN

        LEAF_EXIT(KeAcquireSpinLock)



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

//
// Register aliases
//

        pHeld       = pt0
        pFree       = pt1

        
#if !defined(NT_UP)

        ARGPTR      (a0)
        GET_IRQL    (v0)

KaslrtsRetry:

        SET_IRQL    (SYNCH_LEVEL)
        ;;

        srlz.d
        ;;
        xchg8.nt1   t0 = [a0], a0
        ;;
        cmp.eq      pFree, pHeld = 0, t0
        ;;
        PSET_IRQL   (pHeld, v0)
(pFree) LEAF_RETURN
        ;;

KaslrtsLoop:

        cmp.eq      pFree, pHeld = 0, t0
        ld8         t0 = [a0]
(pFree) br.cond.dpnt  KaslrtsRetry
(pHeld) br.cond.dptk  KaslrtsLoop


#else

        GET_IRQL    (v0)
        SET_IRQL    (SYNCH_LEVEL)                        // Raise IRQL
        LEAF_RETURN

#endif // !defined(NT_UP)

        LEAF_EXIT(KeAcquireSpinLockRaiseToSynch)


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


        LEAF_ENTRY(KeAcquireSpinLockRaiseToDpc)
        
        GET_IRQL    (v0)

#if !defined(NT_UP)

        rsm         1 << PSR_I                // disable interrupt
        cmp.eq      pt0, pt1 = zero, zero
        cmp.eq      pt2 = zero, zero
        ;;

Kaslrtp10:
.pred.rel "mutex",pt0,pt1
(pt0)   xchg8.nt1   t0 = [a0], a0
(pt1)   ld8.nt1     t0 = [a0]
        ;;
(pt0)   cmp.ne      pt2 = zero, t0
        cmp.eq      pt0, pt1 = zero, t0
        ;;
(pt1)   ssm         1 << PSR_I                // enable interrupt 
(pt0)   rsm         1 << PSR_I                // disable interrupt
(pt2)   br.dpnt     Kaslrtp10
        ;;

#endif // !defined(NT_UP)

        SET_IRQL    (DISPATCH_LEVEL)          // Raise IRQL
#if !defined(NT_UP)
        ;;
        ssm         1 << PSR_I                // enable interrupt 
#endif // !defined(NT_UP)
        LEAF_RETURN

        LEAF_EXIT(KeAcquireSpinLockRaiseToDpc)



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
        ARGPTR      (a0)                        // swizzle spinlock address
        st8.rel.nta [a0] = zero             // set spin lock not owned
#endif

        LEAF_RETURN
        LEAF_EXIT(KiReleaseSpinLock)


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
//    to its previous value.  Called at DPC_LEVEL.
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


        NESTED_ENTRY(KeReleaseSpinLock)
        NESTED_SETUP(2,2,1,0)
        
#if !defined(NT_UP)
//
// compute swizzled pointers to spinlock
//
        ARGPTR      (a0)
        st8.rel.nta [a0] = zero                 // set spinlock not owned
#endif

        zxt1        a1 = a1
        ;;
        LOWER_IRQL  (a1)                        // Lower IRQL
        
        NESTED_RETURN
        NESTED_EXIT(KeReleaseSpinLock)

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
//    its previous value and FALSE is returned. Called at IRQL <= DISPATCH_LEVEL.
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
// N.B. This routine assumes KiTryToAcquireSpinLock pt1 & pt2 will be set to reflect
//      the result of the attempt to acquire the spinlock.
//
//--

        LEAF_ENTRY(KeTryToAcquireSpinLock)

        rOldIrql    = t2
        
//
// Raise IRQL to DISPATCH_LEVEL and try to acquire the specified spinlock.
// Return FALSE if failed; otherwise, return TRUE.
//

        ARGPTR      (a1)                        // swizzle IRQL pointer
        GET_IRQL    (rOldIrql)                  // get original IRQL
        SET_IRQL    (DISPATCH_LEVEL)            // raise to dispatch level
        
#if !defined(NT_UP)

        xchg8.nt1   t0 = [a0], a0
        ;;
        cmp.ne      pt2 = t0, zero              // if ne, lock acq failed
        ;;


//
// If successfully acquired, pt1 is set to TRUE while pt2 is set to FALSE.
// Otherwise, pt2 is set to TRUE while pt1 is set to FALSE.
//

  (pt2) mov         v0 = FALSE                  // return FALSE
        PSET_IRQL   (pt2, rOldIrql)             // restore old IRQL
  (pt2) LEAF_RETURN
        ;;

#endif // !defined(NT_UP)

        st1         [a1] = rOldIrql             // save old IRQL
        mov         v0 = TRUE                   // successfully acquired
        LEAF_RETURN

        LEAF_EXIT(KeTryToAcquireSpinLock)


#if !defined(NT_UP)
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

        LEAF_ENTRY(KeTestSpinLock)

        ld8.nt1    t0 = [a0]
        ;;
        cmp.ne     pt0 = 0, t0
        mov        v0 = TRUE                        // default TRUE
        ;;

 (pt0)  mov        v0 = FALSE                       // if t0 != 0 return FALSE
        LEAF_RETURN

        LEAF_EXIT(KeTestSpinLock)
#endif // !defined(NT_UP)


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

        LEAF_EXIT(KeAcquireQueuedSpinLock)

#endif // !defined(NT_UP)
