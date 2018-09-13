//      TITLE("Fast Mutex Support")
//++
//
// Copyright (c) 1994  Microsoft Corporation
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
//--

#include "ksmips.h"

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
//    APC level.
//
// Arguments:
//
//    FastMutex (a0) - Supplies a pointer to a fast mutex.
//
// Return Value:
//
//    None.
//
//--

        .struct 0
        .space  4 * 4                   // argment save area
FmTime: .space  4                       // wait timeout pointer
FmA0:   .space  4                       // saved fast mutex address
FmIrql: .space  4                       // old IRQL value
FmRa:   .space  4                       // saved return address
FastMutexFrameLength:                   // frame length

        NESTED_ENTRY(ExAcquireFastMutex, FastMutexFrameLength, zero)

        subu    sp,sp,FastMutexFrameLength // allocate stack frame
        sw      ra,FmRa(sp)             // save return address

        PROLOGUE_END

//
// Raise IRQL to APC_LEVEL.
//

        li      a1,APC_LEVEL            // set new IRQL level
        lbu     t0,KiPcr + PcIrqlTable(a1) // get translation table entry value
        li      t1,~(0xff << PSR_INTMASK) // get interrupt enable mask
        sll     t0,t0,PSR_INTMASK       // shift table entry into position
        lbu     t2,KiPcr + PcCurrentIrql(zero) // get current IRQL

        DISABLE_INTERRUPTS(t3)          // disable interrupts

        and     t3,t3,t1                // clear current interrupt enables
        or      t3,t3,t0                // set new interrupt enables
        sb      a1,KiPcr + PcCurrentIrql(zero) // set new IRQL

        ENABLE_INTERRUPTS(t3)           // enable interrupts

//
// Decrement ownership count.
//

        lw      t6,KiPcr + PcCurrentThread(zero) // get current thread address
10:     ll      t4,FmCount(a0)          // get ownership count
        subu    t5,t4,1                 // decrement ownership count
        sc      t5,FmCount(a0)          // conditionally store ownership count
        beq     zero,t5,10b             // if eq, store conditional failed
        blez    t4,20f                  // if lez, mutex is already owned
        sb      t2,FmOldIrql(a0)        // store old IRQL
        sw      t6,FmOwner(a0)          // set owner thread address
        addu    sp,sp,FastMutexFrameLength // deallocate stack frame
        j       ra                      // return

//
// Fast mutex is currently owned by another thread. Increment the contention
// count and wait for ownership.
//

20:     sb      t2,FmIrql(sp)           // save old IRQL
        sw      a0,FmA0(sp)             // save address of fast mutex
        lw      t0,FmContention(a0)     // increment contention count
        addu    t0,t0,1                 //
        sw      t0,FmContention(a0)     //

#if DBG

        lw      t0,FmOwner(a0)          // get owner thread address
        bne     t0,t6,30f               // if ne, thread not owner
        jal     DbgBreakPoint           // break

#endif

30:     sw      zero,FmTime(sp)         // set NULL timeout pointer
        li      a3,FALSE                // set nonalertable wait
        li      a2,KernelMode           // set mode of wait
        li      a1,Executive            // set reason for wait
        addu    a0,a0,FmEvent           // compute address of event
        jal     KeWaitForSingleObject   // wait for ownership
        lw      a0,FmA0(sp)             // get address of fast mutex
        lbu     a1,FmIrql(sp)           // get old IRQL value
        lw      a2,KiPcr + PcCurrentThread(zero) // get current thread address
        sb      a1,FmOldIrql(a0)        // save old IRQL in fast mutex
        sw      a2,FmOwner(a0)          // set owner thread address
        lw      ra,FmRa(sp)             // restore return address
        addu    sp,sp,FastMutexFrameLength // deallocate stack frame
        j       ra                      // return

        .end    ExAcquireFastMutex

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
//    FastMutex (a0) - Supplies a pointer to a fast mutex.
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY(ExReleaseFastMutex, FastMutexFrameLength, zero)

        subu    sp,sp,FastMutexFrameLength // allocate stack frame
        sw      ra,FmRa(sp)             // save return address

        PROLOGUE_END

//
// Increment ownership count and release waiter if contention.
//

        lbu     a1,FmOldIrql(a0)        // get old IRQL value
        sw      zero,FmOwner(a0)        // clear owner thread address
10:     ll      t0,FmCount(a0)          // get ownership count
        addu    t1,t0,1                 // increment ownership count
        sc      t1,FmCount(a0)          // conditionally store ownership count
        beq     zero,t1,10b             // if eq, store conditional failed
        beq     zero,t0,20f             // if eq, no waiter is present

//
// There is contention for the fast mutex. Wake up a waiting thread and
// boost its priority to the priority of the current thread.
//

        sw      a0,FmA0(sp)             // save address of fast mutex
        sb      a1,FmIrql(sp)           // save old IRQL value
        addu    a1,a0,FmOwner           // compute address to store owner
        addu    a0,a0,FmEvent           // compute address of event
        jal     KeSetEventBoostPriority // set event and boost priority
        lw      a0,FmA0(sp)             // restore fast mutex address
        lbu     a1,FmIrql(sp)           // restore old IRQL value
        lw      ra,FmRa(sp)             // restore return address

//
// Lower IRQL to its previous value.
//

20:     lbu     t0,KiPcr + PcIrqlTable(a1) // get translation table entry value
        li      t1,~(0xff << PSR_INTMASK) // get interrupt enable mask
        sll     t0,t0,PSR_INTMASK       // shift table entry into position

        DISABLE_INTERRUPTS(t2)          // disable interrupts

        and     t2,t2,t1                // clear current interrupt enables
        or      t2,t2,t0                // set new interrupt enables
        sb      a1,KiPcr + PcCurrentIrql(zero) // set new IRQL

        ENABLE_INTERRUPTS(t2)           // enable interrupts

        addu    sp,sp,FastMutexFrameLength // deallocate stack frame
        j       ra                      // return

        .end    ExReleaseFastMutex

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
//    FastMutex (a0) - Supplies a pointer to a fast mutex.
//
// Return Value:
//
//    If the fast mutex was successfuly acquired, then a value of TRUE
//    is returned as the function vlaue. Otherwise, a value of FALSE is
//    returned.
//
//--

        LEAF_ENTRY(ExTryToAcquireFastMutex)

        li      a1,APC_LEVEL            // set new IRQL level
        lbu     t0,KiPcr + PcIrqlTable(a1) // get translation table entry value
        li      t1,~(0xff << PSR_INTMASK) // get interrupt enable mask
        sll     t0,t0,PSR_INTMASK       // shift table entry into position
        lbu     t2,KiPcr + PcCurrentIrql(zero) // get current IRQL

        DISABLE_INTERRUPTS(t3)          // disable interrupts

//
// Decrement ownership count if and only if the fast mutex is not currently
// owned.
//

        lw      t5,KiPcr + PcCurrentThread(zero) // get current thread address
10:     ll      t4,FmCount(a0)          // get ownership count
        subu    v0,t4,1                 // decrement ownership count
        blez    t4,20f                  // if lez, mutex is already owned
        sc      v0,FmCount(a0)          // conditionally store ownership count
        beq     zero,v0,10b             // if eq, store conditional failed
        and     t3,t3,t1                // clear current interrupt enables
        or      t3,t3,t0                // set new interrupt enables
        sb      a1,KiPcr + PcCurrentIrql(zero) // set new IRQL

        ENABLE_INTERRUPTS(t3)           // enable interrupts

        sb      t2,FmOldIrql(a0)        // store old IRQL
        sw      t5,FmOwner(a0)          // set owner thread address
        j       ra                      // return

//
// Fast mutex is currently owned by another thread. Enable interrupts and
// return FALSE.
//

20:     ENABLE_INTERRUPTS(t3)           // enable interrupts

        li      v0,FALSE                // set return value
        j       ra                      // return

        .end    ExTryToAcquireFastMutex

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
//    FastMutex (a0) - Supplies a pointer to a fast mutex.
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY(ExAcquireFastMutexUnsafe, FastMutexFrameLength, zero)

        subu    sp,sp,FastMutexFrameLength // allocate stack frame
        sw      ra,FmRa(sp)             // save return address

        PROLOGUE_END

//
// Decrement ownership count.
//

        lw      t0,KiPcr + PcCurrentThread(zero) // get current thread address
10:     ll      t1,FmCount(a0)          // get ownership count
        subu    t2,t1,1                 // decrement ownership count
        sc      t2,FmCount(a0)          // conditionally store ownership count
        beq     zero,t2,10b             // if eq, store conditional failed
        blez    t1,20f                  // if lez, mutex is already owned
        sw      t0,FmOwner(a0)          // set owner thread address
        addu    sp,sp,FastMutexFrameLength // deallocate stack frame
        j       ra                      // return

//
// Fast mutex is currently owned by another thread. Increment the contention
// count and wait for ownership.
//

20:     sw      a0,FmA0(sp)             // save address of fast mutex
        lw      t1,FmContention(a0)     // increment contention count
        addu    t1,t1,1                 //
        sw      t1,FmContention(a0)     //

#if DBG

        lw      t1,FmOwner(a0)          // get owner thread address
        bne     t0,t1,30f               // if ne, thread not owner
        jal     DbgBreakPoint           // break

#endif

30:     sw      zero,FmTime(sp)         // set NULL timeout pointer
        li      a3,FALSE                // set nonalertable wait
        li      a2,KernelMode           // set mode of wait
        li      a1,Executive            // set reason for wait
        addu    a0,a0,FmEvent           // compute address of event
        jal     KeWaitForSingleObject   // wait for ownership
        lw      a0,FmA0(sp)             // get address of fast mutex
        lw      a1,KiPcr + PcCurrentThread(zero) // get current thread address
        sw      a1,FmOwner(a0)          // set owner thread address
        lw      ra,FmRa(sp)             // restore return address
        addu    sp,sp,FastMutexFrameLength // deallocate stack frame
        j       ra                      // return

        .end    ExAcquireFastMutexUnsafe

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
//    FastMutex (a0) - Supplies a pointer to a fast mutex.
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY(ExReleaseFastMutexUnsafe, FastMutexFrameLength, zero)

        subu    sp,sp,FastMutexFrameLength // allocate stack frame
        sw      ra,FmRa(sp)             // save return address

        PROLOGUE_END

//
// Increment ownership count and release waiter if contention.
//

        sw      zero,FmOwner(a0)        // clear owner thread address
10:     ll      t0,FmCount(a0)          // get ownership count
        addu    t1,t0,1                 // increment ownership count
        sc      t1,FmCount(a0)          // conditionally store ownership count
        beq     zero,t1,10b             // if eq, store conditional failed
        beq     zero,t0,20f             // if eq, no waiter is present

//
// There is contention for the fast mutex. Wake up a waiting thread and
// boost its priority to the priority of the current thread.
//

        addu    a1,a0,FmOwner           // compute address to store owner
        addu    a0,a0,FmEvent           // compute address of event
        jal     KeSetEventBoostPriority // set event and boost priority
        lw      ra,FmRa(sp)             // restore return address
20:     addu    sp,sp,FastMutexFrameLength // deallocate stack frame
        j       ra                      // return

        .end    ExReleaseFastMutexUnsafe

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
//    FastMutex (a0) - Supplies a pointer to a fast mutex.
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

        lw      t0,KiPcr + PcCurrentThread(zero) // get current thread address
10:     ll      t1,FmCount(a0)          // get ownership count
        subu    v0,t1,1                 // decrement ownership count
        blez    t1,20f                  // if lez, mutex is already owned
        sc      v0,FmCount(a0)          // conditionally store ownership count
        beq     zero,v0,10b             // if eq, store conditional failed
        sw      t0,FmOwner(a0)          // set owner thread address
        j       ra                      // return

//
// Fast mutex is currently owned by another thread.
//

20:     li      v0,FALSE                // set return value
        j       ra                      // return

        .end    ExTryToAcquireFastMutexUnsafe

#endif
