//      TITLE("Fast Mutex Support")
//++
//
// Copyright (c) 1994  Microsoft Corporation
// Copyright (c) 1994  IBM Corporation
//
// Module Name:
//
//    fmutex.s
//
// Abstract:
//
//    This module implements the code necessary to acquire and release fast
//    mutxes.
//
//
// Author:
//
//    David N. Cutler (davec) 13-Apr-1994
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//    Peter L Johnston (plj@vnet.ibm.com) 12-Jun-1994
//
//        Ported to PowerPC architecture.
//
//--

	.extern	..DbgBreakPoint
	.extern ..KeWaitForSingleObject
	.extern ..KeSetEventBoostPriority
	.extern ..KiDispatchSoftwareInterrupt
	
#include "ksppc.h"

        SBTTL("Acquire Fast Mutex")
//++
//
// VOID
// ExAcquireFastMutex (
//    IN PFAST_MUTEX FastMutex
//    )
//
// Routine Description:
//
//    This function acquires ownership of a fast mutex and raises IRQL to
//    APC level.  This routine is coded as a leaf routine on the assumption
//    that it will acquire the mutex without waiting.  If we have to wait
//    for the mutex, we convert to a nested routine.
//
// Arguments:
//
//    FastMutex (r.3) - Supplies a pointer to a fast mutex.
//
// Return Value:
//
//    None.
//
//--

        .struct 0
	.space	StackFrameHeaderLength
FmAddr: .space  4                       // saved fast mutex address
FmIrql: .space  4                       // old IRQL value
Fm31:   .space  4                       // room for LR save
FmThr:  .space  4                       // save current thread
        .align  3                       // ensure 8 byte alignment
FastMutexFrameLength:                   // frame length

        LEAF_ENTRY(ExAcquireFastMutex)

//
// Raise IRQL to APC_LEVEL.
//

        li      r.4, APC_LEVEL		// set new IRQL level
	lbz	r.5, KiPcr+PcCurrentIrql(r.0) // get current IRQL
	stb	r.4, KiPcr+PcCurrentIrql(r.0) // set new IRQL

//
// Decrement ownership count.
//

        lwz     r.6, KiPcr+PcCurrentThread(r.0) // get current thread address
	addi	r.7, r.3, FmCount	// compute address of ownership count

afmDec:
	lwarx	r.8, 0, r.7		// get ownership count
	subi	r.9, r.8, 1		// decrement ownership count
	stwcx.	r.9, 0, r.7		// conditionally store ownership count
	bne-	afmDec			// if store conditional failed
	cmpwi	r.8, 0			// check if already owned
	ble	..ExxAcquireFastMutex	// jif already owned
	stb	r.5, FmOldIrql(r.3)	// save old IRQL in fast mutex
	stw	r.6, FmOwner(r.3)	// set owner thread address

	LEAF_EXIT(ExAcquireFastMutex)


//
// Fast mutex is currently owned by another thread. Increment the contention
// count and wait for ownership.
//

        SPECIAL_ENTRY(ExxAcquireFastMutex)

	lwz	r.8, FmContention(r.3)  // get contention count
	li	r.4, Executive		// set reason for wait
        stw     r.31, Fm31-FastMutexFrameLength(r.sp)
	li	r.7, 0			// set NULL timeout pointer
        stwu    r.sp, -FastMutexFrameLength(r.sp)
        mflr    r.31                    // save link register

        PROLOGUE_END(ExxAcquireFastMutex)

	stb	r.5, FmIrql(r.sp)	// save old IRQL
	stw	r.3, FmAddr(r.sp)	// save address of fast mutex
	li	r.5, KernelMode		// set mode of wait
	addi	r.8, r.8, 1		// increment contention count
	stw     r.6, FmThr(r.sp)        // save current thread address
	stw	r.8, FmContention(r.3)	//

#if DBG

        lwz     r.8, FmOwner(r.3)	// get owner thread address
	cmpw	r.8, r.6		// check thread isn't owner
	bne	afmWait
	bl	..DbgBreakPoint	// break
afmWait:

#endif

	li	r.6, FALSE		// set nonalertable wait
	addi	r.3, r.3, FmEvent	// compute address of event
	bl	..KeWaitForSingleObject // wait for ownership
	lwz	r.3, FmAddr(r.sp)	// get address of fast mutex
	lbz	r.5, FmIrql(r.sp)	// get old IRQL value
	lwz     r.6, FmThr(r.sp)        // get current thread address
	mtlr    r.31                    // set return address
	lwz     r.31, Fm31(r.sp)        // restore r.31
	stb	r.5, FmOldIrql(r.3)	// save old IRQL in fast mutex
	stw	r.6, FmOwner(r.3)	// set owner thread address

	addi	r.sp, r.sp, FastMutexFrameLength

        SPECIAL_EXIT(ExxAcquireFastMutex)

        SBTTL("Release Fast Mutex")
//++
//
// VOID
// ExReleaseFastMutex (
//    IN PFAST_MUTEX FastMutex
//    )
//
// Routine Description:
//
//    This function releases ownership to a fast mutex and lowers IRQL to
//    its previous level.
//
// Arguments:
//
//    FastMutex (r.3) - Supplies a pointer to a fast mutex.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(ExReleaseFastMutex)

//
// Increment ownership count and release waiter if contention.
//

	lbz	r.4, FmOldIrql(r.3)	// get old IRQL value
	li	r.0, 0
	stw	r.0, FmOwner(r.3)	// clear owner thread address
	addi	r.5, r.3, FmCount	// compute address of ownership count
afmInc:
	lwarx	r.8, 0, r.5		// get ownership count
	addi	r.9, r.8, 1		// increment ownership count
	stwcx.	r.9, 0, r.5		// conditionally store ownership count
	bne-	afmInc			// if store conditional failed
	cmpwi	r.8, 0			// check if another waiter
	bne	..ExxReleaseFastMutex	// jif there is a waiter

//
// Lower IRQL to its previous level.
//

        DISABLE_INTERRUPTS(r.8,r.9)

        lhz     r.7, KiPcr+PcSoftwareInterrupt(r.0)
        stb     r.4, KiPcr+PcCurrentIrql(r.0) // set new IRQL

        ENABLE_INTERRUPTS(r.8)

//
// Check to see if a software interrupt could be run at this level.
// We know we are at most at APC_LEVEL.
//
// IF IRQL <= APC LEVEL and IRQL < PcSoftwareInterrupt then a
// software interrupt could run at this time.
//

        cmpw    r.4,r.7
        bgelr+                          // jif no runnable interrupt
        b       ..KiDispatchSoftwareInterrupt

        DUMMY_EXIT(ExReleaseFastMutex)


//
// There is contention for the fast mutex. Wake up a waiting thread and
// boost its priority to the priority of the current thread.
//

        SPECIAL_ENTRY(ExxReleaseFastMutex)

        stwu    r.sp, -FastMutexFrameLength(r.sp)
        stw     r.31, Fm31(r.sp)        // save r.31
        mflr    r.31                    // save link register (in r.31)

        PROLOGUE_END(ExxReleaseFastMutex)

	stw	r.3, FmAddr(r.sp)	// save address of fast mutex
	stb	r.4, FmIrql(r.sp)	// save old IRQL value
	addi	r.4, r.3, FmOwner	// compute address to store owner
	addi	r.3, r.3, FmEvent	// compute address of event
	bl	..KeSetEventBoostPriority// set event and boost priority
	lbz	r.4, FmIrql(r.sp)	// restore old IRQL value

//
// Lower IRQL to its previous value.
//

        DISABLE_INTERRUPTS(r.8, r.9)

	cmpwi   cr.1, r.4, APC_LEVEL
	lhz     r.7, KiPcr+PcSoftwareInterrupt(r.0)
	stb	r.4, KiPcr+PcCurrentIrql(r.0) // set new IRQL

        ENABLE_INTERRUPTS(r.8)

	cmpw    r.4, r.7
	bge+    rfmexit                 // jif no runnable interrupt

	bl      ..KiDispatchSoftwareInterrupt

rfmexit:
        mtlr    r.31                    // set return address
        lwz     r.31, Fm31(r.sp)        // restore r.31
        addi    r.sp, r.sp, FastMutexFrameLength

        SPECIAL_EXIT(ExxReleaseFastMutex)

        SBTTL("Try To Acquire Fast Mutex")
//++
//
// BOOLEAN
// ExTryToAcquireFastMutex (
//    IN PFAST_MUTEX FastMutex
//    )
//
// Routine Description:
//
//    This function attempts to acquire ownership of a fast mutex, and if
//    successful, raises IRQL to APC level.
//
// Arguments:
//
//    FastMutex (r.3) - Supplies a pointer to a fast mutex.
//
// Return Value:
//
//    If the fast mutex was successfuly acquired, then a value of TRUE
//    is returned as the function vlaue. Otherwise, a value of FALSE is
//    returned.
//
//--

        LEAF_ENTRY(ExTryToAcquireFastMutex)

	DISABLE_INTERRUPTS(r.11,r.5)

	li	r.4, APC_LEVEL		// set new IRQL level
	lbz	r.7, KiPcr+PcCurrentIrql(r.0) // get current IRQL

	addi	r.5, r.3, FmCount	// compute address of ownership count
	lwz	r.6, KiPcr+PcCurrentThread(r.0) // get current thread address

//
// Decrement ownership count if and only if the fast mutex is not currently
// owned.
//

afmTry:
	lwarx	r.8, 0, r.5		// get ownership count
	subic.	r.9, r.8, 1		// decrement ownership count
	blt	afmFailed		// if result ltz, mutex already owned
	stwcx.	r.9, 0, r.5		// conditionally store ownership count
	bne-	afmTry			// if store conditional failed
	stb	r.4, KiPcr+PcCurrentIrql(r.0) // set new IRQL

	ENABLE_INTERRUPTS(r.11)		// re-enable interrupts

	stb	r.7, FmOldIrql(r.3)	// store old IRQL
	stw	r.6, FmOwner(r.3)	// set owner thread address
	li	r.3, TRUE		// return success
	blr

//
// Fast mutex is currently owned by another thread. Enable interrupts and
// return FALSE.
//

afmFailed:

	ENABLE_INTERRUPTS(r.11)

	li	r.3, FALSE		// return failure

        LEAF_EXIT(ExTryToAcquireFastMutex)

        SBTTL("Acquire Fast Mutex Unsafe")
//++
//
// VOID
// ExAcquireFastMutexUnsafe (
//    IN PFAST_MUTEX FastMutex
//    )
//
// Routine Description:
//
//    This function acquires ownership of a fast mutex, but does not raise
//    IRQL to APC level.
//
// Arguments:
//
//    FastMutex (r.3) - Supplies a pointer to a fast mutex.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(ExAcquireFastMutexUnsafe)

//
// Decrement ownership count.
//

	addi	r.7, r.3, FmCount	// compute address of ownership count
        lwz     r.6, KiPcr+PcCurrentThread(r.0) // get current thread address

afmDecUnsafe:
	lwarx	r.8, 0, r.7		// get ownership count
	subi	r.9, r.8, 1		// decrement ownership count
	stwcx.	r.9, 0, r.7		// conditionally store ownership count
	bne-	afmDecUnsafe		// if store conditional failed
	cmpwi	r.8, 0			// check if already owned
	ble	..ExxAcquireFastMutexUnsafe// jif mutex already owned
	stw	r.6, FmOwner(r.3)	// set owner thread address

	LEAF_EXIT(ExAcquireFastMutexUnsafe)

//
// Fast mutex is currently owned by another thread. Increment the contention
// count and wait for ownership.
//

        SPECIAL_ENTRY(ExxAcquireFastMutexUnsafe)

	lwz	r.8, FmContention(r.3)  // get contention count
	li	r.7, 0			// set NULL timeout pointer
	li	r.4, Executive		// set reason for wait
        stwu    r.sp, -FastMutexFrameLength(r.sp)
	li	r.5, KernelMode		// set mode of wait
        stw     r.31, Fm31(r.sp)        // save r.31
        mflr    r.31                    // save link register (in r.31)

        PROLOGUE_END(ExxAcquireFastMutexUnsafe)

	stw     r.6, FmThr(r.sp)        // save thread address
	addi	r.8, r.8, 1		// increment contention count
	stw	r.3, FmAddr(r.sp)	// save address of fast mutex
	stw	r.8, FmContention(r.3)	//

#if DBG

        lwz     r.8, FmOwner(r.3)	// get owner thread address
	cmpw	r.8, r.6		// check thread isn't owner
	bne	afmWaitUnsafe
	bl	..DbgBreakPoint	// break
afmWaitUnsafe:

#endif

	li	r.6, FALSE		// set nonalertable wait
	addi	r.3, r.3, FmEvent	// compute address of event
	bl	..KeWaitForSingleObject// wait for ownership
	lwz	r.3, FmAddr(r.sp)	// get address of fast mutex
	lwz	r.6, FmThr(r.sp)        // get current thread address
	mtlr    r.31                    // set return address
	lwz     r.31, Fm31(r.sp)        // restore r.31
	stw	r.6, FmOwner(r.3)	// set owner thread address

	addi	r.sp, r.sp, FastMutexFrameLength // deallocate stack frame

        SPECIAL_EXIT(ExxAcquireFastMutexUnsafe)

        SBTTL("Release Fast Mutex Unsafe")
//++
//
// VOID
// ExReleaseFastMutexUnsafe (
//    IN PFAST_MUTEX FastMutex
//    )
//
// Routine Description:
//
//    This function releases ownership of a fast mutex, and does not
//    restore IRQL to its previous value.
//
// Arguments:
//
//    FastMutex (r.3) - Supplies a pointer to a fast mutex.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(ExReleaseFastMutexUnsafe)

//
// Increment ownership count and release waiter if contention.
//

	li	r.0, 0
	stw	r.0, FmOwner(r.3)	// clear owner thread address
	addi	r.5, r.3, FmCount	// compute address of ownership count
afmIncUnsafe:
	lwarx	r.8, 0, r.5		// get ownership count
	addi	r.9, r.8, 1		// increment ownership count
	stwcx.	r.9, 0, r.5		// conditionally store ownership count
	bne-	afmIncUnsafe		// if store conditional failed
	cmpwi	r.8, 0			// check if another waiter
	beqlr+                          // retorn if no waiter

//
// There is contention for the fast mutex. Wake up a waiting thread and
// boost its priority to the priority of the current thread.
//
// N.B. KeSetEventBoostPriority returns directly to our caller.
//

	addi	r.4, r.3, FmOwner	// compute address to store owner
	addi	r.3, r.3, FmEvent	// compute address of event
	b 	..KeSetEventBoostPriority// set event and boost priority

        LEAF_EXIT(ExReleaseFastMutexUnsafe, FastMutexFrameLength, 0, 0)

        SBTTL("Try To Acquire Fast Mutex Unsafe")
//++
//
// BOOLEAN
// ExTryToAcquireFastMutexUnsafe (
//    IN PFAST_MUTEX FastMutex
//    )
//
// Routine Description:
//
//    This function attempts to acquire ownership of a fast mutex, and if
//    successful, does not raise IRQL to APC level.
//
// Arguments:
//
//    FastMutex (r.3) - Supplies a pointer to a fast mutex.
//
// Return Value:
//
//    If the fast mutex was successfuly acquired, then a value of TRUE
//    is returned as the function vlaue. Otherwise, a value of FALSE is
//    returned.
//
//--

#if 0

        LEAF_ENTRY(ExTryToAcquireFastMutexUnsafe)

//
// Decrement ownership count if and only if the fast mutex is not currently
// owned.
//

	addi	r.5, r.3, FmCount	// compute address of ownership count
	lwz	r.6, KiPcr+PcCurrentThread(r.0) // get current thread address

afmTryUnsafe:
	lwarx	r.8, 0, r.5		// get ownership count
	subic.	r.9, r.8, 1		// decrement ownership count
	blt	afmTryUnsafeFailed	// if result ltz, mutex already owned
	stwcx.	r.9, 0, r.5		// conditionally store ownership count
	bne-	afmTryUnsafe		// if store conditional failed
	stw	r.6, FmOwner(r.3)	// set owner thread address
	li	r.3, TRUE		// set return value
	blr				// return

//
// Fast mutex is currently owned by another thread.
//

afmTryUnsafeFailed:

	li      r.3, FALSE		// set return value

        LEAF_EXIT(ExTryToAcquireFastMutexUnsafe)

#endif
