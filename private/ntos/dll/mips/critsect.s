//      TITLE("Enter and Leave Critical Section")
//++
//
//  Copyright (c) 1991 Microsoft Corporation
//
//  Module Name:
//
//    critsect.s
//
//  Abstract:
//
//    This module implements functions to support user mode critical sections.
//
//  Author:
//
//    David N. Cutler 1-May-1992
//
//  Environment:
//
//    Any mode.
//
//  Revision History:
//
//--

#include "ksmips.h"

        SBTTL("Enter Critical Section")
//++
//
// NTSTATUS
// RtlEnterCriticalSection(
//    IN PRTL_CRITICAL_SECTION CriticalSection
//    )
//
// Routine Description:
//
//    This function enters a critical section.
//
//    N.B. This function is duplicated in the runtime library.
//
// Arguments:
//
//    CriticalSection (a0) - Supplies a pointer to a critical section.
//
// Return Value:
//
//    STATUS_SUCCESS is returned as the function value.
//
//--

        .struct 0
        .space  4 * 4                   // argument save area
        .space  3 * 4                   // fill
EcRa:   .space  4                       // saved return address
EcFrameLength:                          // length of stack frame
EcA0:   .space  4                       // saved critical section address
EcA1:   .space  4                       // saved unique thread id

        NESTED_ENTRY(RtlEnterCriticalSection, EcFrameLength, zero)

        subu    sp,sp,EcFrameLength     // allocate stack frame
        sw      ra,EcRa(sp)             // save return address

        PROLOGUE_END

//
// Attempt to enter the critical section.
//

10:     ll      v0,CsLockCount(a0)      // get addend value
        addu    v0,v0,1                 // increment addend value
        move    t0,v0                   // copy updated value
        sc      t0,CsLockCount(a0)      // store conditionally
        beq     zero,t0,10b             // if eq, store failed

//
// If the critical section is not already owned, then initialize the owner
// thread id, initialize the recursion count, and return a success status.
//

        li      t0,UsPcr                // get user PCR page address
        lw      t0,PcTeb(t0)            // get address of current TEB
        lw      a1,TeClientId + 4(t0)   // get current thread unique id
        bne     zero,v0,20f             // if ne, lock already owned

        sw      a1,CsOwningThread(a0)   // set critical section owner
        li      v0,STATUS_SUCCESS       // set return status
        lw      ra,EcRa(sp)             // restore return address
        addu    sp,sp,EcFrameLength     // deallocate stack frame
        j       ra                      // return

//
// The critical section is owned. If the current thread is the owner, then
// increment the recursion count, and return a success status. Otherwise,
/// wit for critical section ownership.
//

20:     lw      t0,CsOwningThread(a0)   // get unique id of owner thread
        bne     t0,a1,30f               // if ne, current thread not owner
        lw      t0,CsRecursionCount(a0) // increment the recursion count
        addu    t0,t0,1                 //
        sw      t0,CsRecursionCount(a0) //
        li      v0,STATUS_SUCCESS       // set return status
        lw      ra,EcRa(sp)             // restore return address
        addu    sp,sp,EcFrameLength     // deallocate stack frame
        j       ra                      // return

//
// The critical section is owned by a thread other than the current thread.
// Wait for ownership of the critical section.

30:     sw      a0,EcA0(sp)             // save address of critical section
        sw      a1,EcA1(sp)             // save unique thread id
        jal     RtlpWaitForCriticalSection // wait for critical section
        lw      a0,EcA0(sp)             // restore address of critical section
        lw      a1,EcA1(sp)             // restore unique thread id
        sw      a1,CsOwningThread(a0)   // set critical section owner
        li      v0,STATUS_SUCCESS       // set return status
        lw      ra,EcRa(sp)             // restore return address
        addu    sp,sp,EcFrameLength     // deallocate stack frame
        j       ra                      // return

        .end    RtlEnterCriticalSection

        SBTTL("Leave Critical Section")
//++
//
// NTSTATUS
// RtlLeaveCriticalSection(
//    IN PRTL_CRITICAL_SECTION CriticalSection
//    )
//
// Routine Description:
//
//    This function leaves a critical section.
//
//    N.B. This function is duplicated in the runtime library.
//
// Arguments:
//
//    CriticalSection (a0)- Supplies a pointer to a critical section.
//
// Return Value:
//
//   STATUS_SUCCESS is returned as the function value.
//
//--

        .struct 0
        .space  4 * 4                   // argument save area
        .space  3 * 4                   // fill
LcRa:   .space  4                       // saved return address
LcFrameLength:                          // length of stack frame
LcA0:   .space  4                       // saved critical section address

        NESTED_ENTRY(RtlLeaveCriticalSection, LcFrameLength, zero)

        subu    sp,sp,LcFrameLength     // allocate stack frame
        sw      ra,LcRa(sp)             // save return address

        PROLOGUE_END

//
// If the current thread is not the owner of the critical section, then
// raise an exception.
//

#if DBG

        li      t0,UsPcr                // get user PCR page address
        lw      t0,PcTeb(t0)            // get address of current TEB
        lw      t1,CsOwningThread(a0)   // get owning thread unique id
        lw      a1,TeClientId + 4(t0)   // get current thread unique id
        beq     a1,t1,10f               // if eq, current thread is owner
        jal     RtlpNotOwnerCriticalSection // raise exception
        li      v0,STATUS_INVALID_OWNER // set completion status
        lw      ra,LcRa(sp)             // restore return address
        addu    sp,sp,LcFrameLength     // deallocate stack frame
        j       ra                      // return

#endif

//
// Decrement the recursion count. If the result is zero, then the lock
// is no longer onwed.
//

10:     lw      t0,CsRecursionCount(a0) // decrement recursion count
        subu    t0,t0,1                 //
        bgez    t0,30f                  // if gez, lock still owned
        sw      zero,CsOwningThread(a0) // clear owner thread id

//
// Decrement the lock count and check if a waiter should be continued.
//

20:     ll      v0,CsLockCount(a0)      // get addend value
        subu    v0,v0,1                 // decrement addend value
        move    t0,v0                   // copy updated value
        sc      t0,CsLockCount(a0)      // store conditionally
        beq     zero,t0,20b             // if eq, store failed
        bltz    v0,50f                  // if ltz, no waiter present
        jal     RtlpUnWaitCriticalSection // unwait thread
        li      v0,STATUS_SUCCESS       // set completion status
        lw      ra,LcRa(sp)             // restore return address
        addu    sp,sp,LcFrameLength     // deallocate stack frame
        j       ra                      // return

//
// Decrement the lock count and return a success status since the lock
// is still owned.
//

30:     sw      t0,CsRecursionCount(a0) //
40:     ll      v0,CsLockCount(a0)      // get addend value
        subu    v0,v0,1                 // decrement addend value
        sc      v0,CsLockCount(a0)      // store conditionally
        beq     zero,v0,40b             // if eq, store failed
50:     li      v0,STATUS_SUCCESS       // set completion status
        lw      ra,LcRa(sp)             // restore return address
        addu    sp,sp,LcFrameLength     // deallocate stack frame
        j       ra                      // return

        .end    RtlLeaveCriticalSection


        SBTTL("Try to Enter Critical Section")
//++
//
// BOOL
// RtlTryEnterCriticalSection(
//    IN PRTL_CRITICAL_SECTION CriticalSection
//    )
//
// Routine Description:
//
//    This function attempts to enter a critical section without blocking.
//
// Arguments:
//
//    CriticalSection (a0) - Supplies a pointer to a critical section.
//
// Return Value:
//
//    If the critical section was successfully entered, then a value of TRUE
//    is returned as the function value. Otherwise, a value of FALSE is returned.
//
//--

        LEAF_ENTRY(RtlTryEnterCriticalSection)

        li      v0,UsPcr                // get user PCR page address
        lw      v0,PcTeb(v0)            // get address of current TEB
        lw      a1,TeClientId + 4(v0)   // get current thread unique id

//
// Attempt to enter the critical section.
//

10:     ll      t0,CsLockCount(a0)      // get addend value - locked
        addu    t1,t0,1                 // increment addend value
        bne     zero,t1,20f             // critical section owned
        sc      t1,CsLockCount(a0)      // store conditionally
        beq     zero,t1,10b             // if lock-flag eq zero, store failed

//
// The critical section is now owned by this thread. Initialize the owner
// thread id and return a successful status.
//
        sw      a1,CsOwningThread(a0)   // set critical section owner
        li      v0,TRUE                 // set success status
        j       ra                      // return

//
// The critical section is already owned. If it is owned by another thread,
// return FALSE immediately. If it is owned by this thread, we must increment
// the lock count here.
//

20:     lw      t2,CsOwningThread(a0)   // get current owner
        beq     t2,a1,30f               // if eq, this thread is already the owner
        li      v0,FALSE                // set failure status
        j       ra                      // return

//
// This thread is already the owner of the critical section. Perform an atomic
// increment of the LockCount and a normal increment of the RecursionCount and
// return success.
//

30:     ll      t0,CsLockCount(a0)      // get addend value - locked
        addu    t1,t0,1                 // increment addend value
        sc      t1,CsLockCount(a0)      // store conditionally
        beq     zero,t1,30b             // if eqz, store failed

//
// Increment the recursion count
//

        lw      t0,CsRecursionCount(a0) //
        addu    t1,t0,1                 //
        sw      t1,CsRecursionCount(a0) //
        li      v0,TRUE                 // set success status
        j       ra                      // return

        .end    RtlTryEnterCriticalSection
