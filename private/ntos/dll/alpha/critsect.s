//      TITLE("Enter and Leave Critical Section")
//++
//
// Copyright (c) 1991  Microsoft Corporation
// Copyright (c) 1992  Digital Equipment Corporation
//
// Module Name:
//
//    critsect.s
//
// Abstract:
//
//    This module implements functions to support user mode critical sections.
//
// Author:
//
//    David N. Cutler 1-May-1992
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//    Thomas Van Baak (tvb) 21-May-1992
//
//        Adapted for Alpha AXP.
//
//--

#include "ksalpha.h"

        SBTTL("Enter Critical Section")
//++
//
// NTSTATUS
// RtlEnterCriticalSection (
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
EcRa:   .space  8                       // saved return address
EcA0:   .space  8                       // saved critical section address
EcA1:   .space  8                       // saved unique thread id
        .space  1 * 8                   // required for 16-byte stack alignment
EcFrameLength:                          // length of stack frame

        NESTED_ENTRY(RtlEnterCriticalSection, EcFrameLength, zero)

        lda     sp, -EcFrameLength(sp)  // allocate stack frame
        stq     ra, EcRa(sp)            // save return address

        PROLOGUE_END

        GET_THREAD_ENVIRONMENT_BLOCK    // (PALcode) get TEB address in v0

        ldl     t3, CsSpinCount(a0)     // check if spin count is zero
        LDP     a1, TeClientId + CidUniqueThread(v0) // get current thread id
        bne     t3, 50f                 // if ne, spin count specified

//
// Attempt to enter the critical section.
//

10:     ldl_l   t0, CsLockCount(a0)     // get addend value - locked
        addl    t0, 1, t0               // increment addend value
        mov     t0, t1                  //
        stl_c   t1, CsLockCount(a0)     // store conditionally
        beq     t1, 40f                 // if eq, conditional store failed
        mb                              // synchronize memory access

//
// If the critical section is not already owned, then initialize the owner
// thread id, initialize the recursion count, and return a success status.
// The updated lock value is now in t0.
//

        bne     t0, 20f                 // if ne, lock already owned
        STP     a1, CsOwningThread(a0)  // set critical section owner
        ldil    v0, STATUS_SUCCESS      // set return status
        lda     sp, EcFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

//
// The critical section is owned. If the current thread is the owner, then
// increment the recursion count, and return a success status. Otherwise,
// wait for critical section ownership.
// The current thread unique id is in a1.
//

20:     LDP     t0, CsOwningThread(a0)  // get unique id of owner thread
        cmpeq   a1, t0, t1              // check if current thread is owner
        beq     t1, 30f                 // if eq, current thread not owner
        ldl     t0, CsRecursionCount(a0) // increment the recursion count
        addl    t0, 1, t2               //
        stl     t2, CsRecursionCount(a0) //
        ldil    v0, STATUS_SUCCESS      // set return status
        lda     sp, EcFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

//
// The critical section is owned by a thread other than the current thread.
// Wait for ownership of the critical section.
// N.B. a1 is just a temp register below, not an argument to the function.
//

30:     stq     a0, EcA0(sp)            // save address of critical section
        stq     a1, EcA1(sp)            // save unique thread id
        bsr     ra, RtlpWaitForCriticalSection // wait for critical section
        ldq     a0, EcA0(sp)            // restore address of critical section
        ldq     a1, EcA1(sp)            // restore unique thread id
        STP     a1, CsOwningThread(a0)  // set critical section owner
        ldil    v0, STATUS_SUCCESS      // set return status
        ldq     ra, EcRa(sp)            // restore return address
        lda     sp, EcFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

40:     br      zero, 10b               // go try lock again

//
// A nonzero spin count is specified
//

50:     LDP     t4, CsOwningThread(a0)  // get owner thread id
        cmpeq   t4, a1, t5              // check if current thread is owner
        beq     t5, 60f                 // if eq, current thread not owner

//
// The critical section is owned by the current thread. Increment the lock
// count and the recursion count.
//

55:     ldl_l   t0, CsLockCount(a0)     // get addend value - locked
        addl    t0, 1, t1               // increment addend value
        stl_c   t1, CsLockCount(a0)     // store conditionally
        beq     t1, 59f                 // if lock-flag eq zero, store failed
        mb                              // synchronize memory access
        ldl     t3, CsRecursionCount(a0) // increment recursion count
        addl    t3, 1, t4               //
        stl     t4, CsRecursionCount(a0) //
        ldil    v0, STATUS_SUCCESS      // set return status
        lda     sp, EcFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

//
// Store conditional attempt failed.
//

59:     br      zero, 55b               // go try lock again

//
// A nonzero spin count is specified and the current thread is not the owner.
//

60:     ldl_l   t0, CsLockCount(a0)     // get addend value - locked
        addl    t0, 1, t1               // increment addend value
        bne     t1, 70f                 // if ne, critical section is owned
        stl_c   t1, CsLockCount(a0)     // set new lock count
        beq     t1, 69f                 // if eq, conditional store failed
        mb                              // synchronize memory access

//
// The critical section has been acquired. Set the owning thread and the initial
// recursion count.
//

        STP     a1, CsOwningThread(a0)  // set critical section owner
        ldil    v0, STATUS_SUCCESS      // set return status
        lda     sp, EcFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

//
// Store conditional attempt failed.
//

69:     br      zero, 60b               //

//
// The critical section is currently owned. Spin until it is either unowned
// or the spin count has reached zero. Spin count is in t3
//
// If waiters are present, don't spin on the lock since we will never see it
// go free.
//

70:     ldl     t0, CsLockCount(a0)     // check if lock is owned
        addl    t0, 1, t1               //
        bgt     t0, 10b                 // if >=, then do not spin
        beq     t1, 60b                 // if eq, lock is not owned
        subl    t3, 1, t3               // decrement spin count
        bne     t3, 70b                 // if nz, continue spinning
        br      10b                     // spin expired, go wait for lock

        .end    RtlEnterCriticalSection

        SBTTL("Leave Critical Section")
//++
//
// NTSTATUS
// RtlLeaveCriticalSection (
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
//    CriticalSection (a0) - Supplies a pointer to a critical section.
//
// Return Value:
//
//    STATUS_SUCCESS is returned as the function value.
//
//--

        .struct 0
LcRa:   .space  8                       // saved return address
        .space  1 * 8                   // required for 16-byte stack alignment
LcFrameLength:                          // length of stack frame

        NESTED_ENTRY(RtlLeaveCriticalSection, LcFrameLength, zero)

        lda     sp, -LcFrameLength(sp)  // allocate stack frame
        stq     ra, LcRa(sp)            // save return address

        PROLOGUE_END

//
// If the current thread is not the owner of the critical section, then
// raise an exception.
//

#if DBG

        GET_THREAD_ENVIRONMENT_BLOCK    // (PALcode) get TEB address in v0

        LDP     a1, TeClientId + CidUniqueThread(v0) // get current thread id
        LDP     t0, CsOwningThread(a0)  // get owner thread id
        cmpeq   a1, t0, t1              // check if current thread owner
        bne     t1, 10f                 // if ne, current thread is owner
        bsr     ra, RtlpNotOwnerCriticalSection // raise exception
        ldil    v0, STATUS_INVALID_OWNER // set completion status
        ldq     ra, LcRa(sp)            // restore return address
        lda     sp, LcFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

#endif

//
// Decrement the recursion count. If the result is zero, then the lock
// is no longer owned.
//

10:     ldl     t0, CsRecursionCount(a0) // decrement recursion count
        subl    t0, 1, t0                //
        bge     t0, 30f                  // if ge, lock still owned
        STP     zero, CsOwningThread(a0) // clear owner thread id

//
// Decrement the lock count and check if a waiter should be continued.
//
//

20:     mb                              // synchronize memory access
        ldl_l   t0, CsLockCount(a0)     // get addend value - locked
        subl    t0, 1, t0               // decrement addend value
        mov     t0, t1                  // copy updated value to t1 for store
        stl_c   t1, CsLockCount(a0)     // store conditionally
        beq     t1, 60f                 // if eq, conditional store failed
        blt     t0, 50f                 // if lt, no waiter present
        bsr     ra, RtlpUnWaitCriticalSection // unwait thread
        ldil    v0, STATUS_SUCCESS      // set completion status
        ldq     ra, LcRa(sp)            // restore return address
        lda     sp, LcFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

//
// Decrement the lock count and return a success status since the lock
// is still owned.
//

30:     stl     t0, CsRecursionCount(a0) // store updated recursion count
40:     ldl_l   t0, CsLockCount(a0)     // get addend value - locked
        subl    t0, 1, t0               // decrement addend value
        stl_c   t0, CsLockCount(a0)     // store conditionally
        beq     t0, 70f                 // if lock-flag eq zero, store failed
50:     ldil    v0, STATUS_SUCCESS      // set completion status
        lda     sp, LcFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

60:     br      zero, 20b               // go try lock again

70:     br      zero, 40b               // go try lock again

        .end    RtlLeaveCriticalSection


        SBTTL("Try to Enter Critical Section")
//++
//
// BOOLEAN
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

        .struct 0
EcRa:   .space  8                       // saved return address
EcA0:   .space  8                       // saved critical section address
EcA1:   .space  8                       // saved unique thread id
        .space  1 * 8                   // required for 16-byte stack alignment
EcFrameLength:                          // length of stack frame

        LEAF_ENTRY(RtlTryEnterCriticalSection)

        GET_THREAD_ENVIRONMENT_BLOCK    // (PALcode) get TEB address in v0

        LDP     a1, TeClientId + CidUniqueThread(v0) // get current thread unique id

//
// Attempt to enter the critical section.
//

10:     ldl_l   t0, CsLockCount(a0)     // get addend value - locked
        addl    t0, 1, t1               // increment addend value
        bne     t1, 20f                 // critical section owned
        stl_c   t1, CsLockCount(a0)     // store conditionally
        beq     t1, 40f                 // if eq, conditional store failed
        mb                              // synchronize memory access

//
// The critical section is now owned by this thread. Initialize the owner
// thread id and return a successful status.
//

        STP     a1, CsOwningThread(a0)  // set critical section owner
        ldil    v0, TRUE                // set success status
        ret     zero, (ra)

//
// The critical section is already owned. If it is owned by another thread,
// return FALSE immediately. If it is owned by this thread, we must increment
// the lock count here.
//

20:     LDP     t2, CsOwningThread(a0)  // get current owner
        cmpeq   t2, a1, t3              // check if current thread owner
        bne     t3, 30f                 // if ne, current thread owner
        bis     zero,zero,v0            // set failure status
        ret     zero, (ra)              // return

//
// This thread is already the owner of the critical section. Perform an atomic
// increment of the LockCount and a normal increment of the RecursionCount and
// return success.
//

30:     ldl_l   t0, CsLockCount(a0)     // get addend value - locked
        addl    t0, 1, t1               // increment addend value
        stl_c   t1, CsLockCount(a0)     // store conditionally
        beq     t1, 50f                 // if eq, conditional store failed
        ldl     t0, CsRecursionCount(a0) // increment recursion count
        addl    t0, 1, t1               //
        stl     t1, CsRecursionCount(a0) //
        ldil    v0, TRUE                // set success status
        ret     zero, (ra)              // return

//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

40:     br      zero, 10b               // go try lock again

50:     br      zero, 30b               // retry lock

        .end    RtlTryEnterCriticalSection
