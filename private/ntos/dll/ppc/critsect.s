//      TITLE("Eenter and Leave Critical Section")
//++
//
//  Copyright (c) 1993 IBM Corporation
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
//    Chuck Bauman 12-Aug-1993
//
//  Environment:
//
//    Any mode.
//
//  Revision History:
//
//    Port NT product1 source to PowerPC
//
//--

#include "ksppc.h"

        .extern ..RtlpWaitForCriticalSection
        .extern ..RtlpNotOwnerCriticalSection
        .extern ..RtlpUnWaitCriticalSection
        .extern ..DbgBreakPoint

//       SBTTL("Enter Critical Section")
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
// Arguments:
//
//    CriticalSection (r.3) - Supplies a pointer to a critical section.
//
// Return Value:
//
//    STATUS_SUCCESS is returned as the function value.
//
//--

          .struct 0
          .space  StackFrameHeaderLength
EcAddr:   .space  4                             // saved critical section address
EcClient: .space  4                             // saved client ID
          .space  4                             // room for LR save
          .align  3                             // ensure 8 byte alignment
EcFrameLength:                                  // frame length

//
// RtlEnterCriticalSection has been performance optimized for the fast path
// making it a leaf entry. When the slow path must be performed to acquire
// a critical section a branch to a NESTED_ENTRY for unwinding purposes is
// performed.
//

        LEAF_ENTRY(RtlEnterCriticalSection)

        lwz     r.7,CsSpinCount(r.3)            // get spin count
        li      r.10,CsLockCount                // constant for lwarx/stwcx. pairs
        cmpwi   r.7,0                           // is spin count zero?
        lwz     r.4,TeClientId + 4(r.13)        // get current thread client id
        bne     ecsSpinCountSpecified           // if ne, spin count specified

ecsEnter:

//
// Attempt to enter the critical section.
//

ecsStoreFailed1:
        lwarx   r.8,r.10,r.3                    // get lock count
        addi    r.8,r.8,1                       // increment lock count
        stwcx.  r.8,r.10,r.3                    // store new value conditionally
        bne-    ecsStoreFailed1                 // if eq, store failed

        cmpwi   r.8,0                           // check lock count
        bne-    ecsOwnedWait                    // if ne, lock already owned

//
// The critical section is not already owned. Set the critical section owner.
//
// N.B. Recursion count already initialized via RtlLeaveCriticalSection.
//

        stw     r.4,CsOwningThread(r.3)         // set critical section owner

#if DBG
        lwz     r.5,CsDebugInfo(r.3)            // increment entry count
        lwz     r.6,CsEntryCount(r.5)           //
        addi    r.6,r.6,1                       //
        stw     r.6,CsEntryCount(r.5)           //
        lwz     r.6,TeCountOfOwnedCriticalSections(r.13) // increment owned count
        addi    r.6,r.6,1                                //
        stw     r.6,TeCountOfOwnedCriticalSections(r.13) //
#endif

        li      r.3,STATUS_SUCCESS              // set return status
        blr                                     // return to caller

ecsOwnedWait:

//
// The critical section is already owned, but may be owned by the current thread.
//

        lwz     r.5,CsOwningThread(r.3)         // get owner client ID
        cmpw    r.5,r.4                         // check if current thread is owner
        bne-    ..RtlpEnterCriticalSection      // if ne, current thread not owner
                                                // NOTE: RtlEnterCriticalSection will
                                                //       NOT appear in a stack trace

ecsIncrementRecursionCount:

        lwz     r.5,CsRecursionCount(r.3)       // increment recursion count
        addi    r.5,r.5,1                       //
        stw     r.5,CsRecursionCount(r.3)       //

#if DBG
        lwz     r.5,CsDebugInfo(r.3)            // increment entry count
        lwz     r.6,CsEntryCount(r.5)           //
        addi    r.6,r.6,1                       //
        stw     r.6,CsEntryCount(r.5)           //
#endif

        li      r.3,STATUS_SUCCESS              // set return status
        blr                                     // return to caller

ecsSpinCountSpecified:

//
// A nonzero spin count is specified.
//

        lwz     r.5,CsOwningThread(r.3)         // get owner client ID
        cmpw    r.5,r.4                         // check if current thread is owner
        bne-    ecsOwnedSpin                    // if ne, current thread not owner

//
// The critical section is owned by the current thread. Increment the lock
// count and the recursion count.
//

ecsStoreFailed2:
        lwarx   r.8,r.10,r.3                    // get lock count
        addi    r.8,r.8,1                       // increment lock count
        stwcx.  r.8,r.10,r.3                    // store new value conditionally
        bne-    ecsStoreFailed2                 // if eq, store failed

        b       ecsIncrementRecursionCount      // join common code

ecsOwnedSpin:

//
// A nonzero spin count is specified and the current thread is not the owner.
//

        lwarx   r.8,r.10,r.3                    // get lock count
        addic.  r.8,r.8,1                       // increment lock count
        bne-    ecsSpin                         // if ne, lock not free
        stwcx.  r.8,r.10,r.3                    // store new value conditionally
        bne-    ecsOwnedSpin                    // if eq, store failed

//
// The critical section has been acquired. Set the owning thread.
//
// N.B. Recursion count already initialized via RtlLeaveCriticalSection.
//

        stw     r.4,CsOwningThread(r.3)         // set critical section owner

#if DBG
        lwz     r.5,CsDebugInfo(r.3)            // increment entry count
        lwz     r.6,CsEntryCount(r.5)           //
        addi    r.6,r.6,1                       //
        stw     r.6,CsEntryCount(r.5)           //
        lwz     r.6,TeCountOfOwnedCriticalSections(r.13) // increment owned count
        addi    r.6,r.6,1                                //
        stw     r.6,TeCountOfOwnedCriticalSections(r.13) //
#endif

        li      r.3,STATUS_SUCCESS              // set return status
        blr                                     // return to caller

ecsSpin:

//
// The critical section is currently owned. Spin until it is either unowned
// or the spin count has reached zero.
//

        lwz     r.8,CsLockCount(r.3)            // check if lock is owned
        cmpwi   r.8,-1                          //
        beq+    ecsOwnedSpin                    // if eq, lock is not owned
        subic.  r.7,r.7,1                       // decrement spin count
        bne+    ecsSpin                         // if ne, continue spinning

//
// Spin count exhausted. Jump back into normal path.
//

        b       ecsEnter

        DUMMY_EXIT(RtlEnterCriticalSection)

        NESTED_ENTRY(RtlpEnterCriticalSection,EcFrameLength,0,0)
        PROLOGUE_END(RtlpEnterCriticalSection)

//
// The critical section is owned by another thread and the current thread must
// wait for ownership.
//

        stw     r.4, EcClient(r.sp)             // save client id
        stw     r.3, EcAddr(r.sp)               // save critical section address
        bl      ..RtlpWaitForCriticalSection
        lwz     r.4, EcClient(r.sp)             // restore client id
        lwz     r.5, EcAddr(r.sp)               // restore critical section address
        li      r.3,STATUS_SUCCESS              // set return status
        stw     r.4, CsOwningThread(r.5)        // set critical section owner

#if DBG
        lwz     r.6,TeCountOfOwnedCriticalSections(r.13) // increment owned count
        addi    r.6,r.6,1                                //
        stw     r.6,TeCountOfOwnedCriticalSections(r.13) //
#endif

        NESTED_EXIT(RtlpEnterCriticalSection,EcFrameLength,0,0)


//       SBTTL("Leave Critical Section")
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
//    CriticalSection (r.3) - Supplies a pointer to a critical section.
//
// Return Value:
//
//   STATUS_SUCCESS is returned as the function value.
//
//--

//
// RtlLeaveCriticalSection has been performance optimized for the fast path
// making it a leaf entry. When the slow path must be performed to acquire
// a critical section a branch to a NESTED_ENTRY for unwinding purposes is
// performed.
//

#if DBG
        NESTED_ENTRY(RtlLeaveCriticalSection,EcFrameLength,0,0)
        PROLOGUE_END(RtlLeaveCriticalSection)
#else
        LEAF_ENTRY(RtlLeaveCriticalSection)
#endif

        li      r.10,CsLockCount                // Constant for lwarx/stwcx. pairs
        li      r.9,0
//
// If the current thread is not the owner of the critical section, then
// raise an exception.
//

#if DBG

        lwz     r.6,CsOwningThread(r.3)         // get owning thread unique id
        lwz     r.4,TeClientId + 4(r.13)        // get current thread unique id
        cmpw    r.4,r.6
        beq+    owner                           // if eq, current thread is owner

        bl      ..RtlpNotOwnerCriticalSection   // call RtlpNotOwnerCriticalSection
        LWI(r.3,STATUS_INVALID_OWNER)           // STATUS_INVALID_OWNER = 0xc000005a
        b       RtlLeaveCriticalSection.epi     // return error code

owner:

#endif

//
// Decrement the recursion count. If the result is zero, then the lock
// is no longer onwed.
//

        lwz     r.5,CsRecursionCount(r.3)       // decrement recursion count
        subic.  r.5,r.5,1                       //
        bge-    stillowned                      // if gez, lock still owned
                                                // predict branch not taken
        stw     r.9,CsOwningThread(r.3)         // clear owner thread id

#if DBG
        lwz     r.6,TeCountOfOwnedCriticalSections(r.13) // decrement owned count
        subi    r.6,r.6,1                                //
        stw     r.6,TeCountOfOwnedCriticalSections(r.13) //
#endif

//
// Decrement the lock count and check if a waiter should be continued.
//


res2failed:
        lwarx   r.8,r.10,r.3                    // get addend value
        subi    r.8,r.8,1                       // decrement addend value
        stwcx.  r.8,r.10,r.3                    // store conditionally
        bne-    res2failed                      // if eq, store failed

        cmpwi   cr.0,r.8,0

#if DBG
        blt+    nowaits                         // if ltz, no waiter present
                                                // predict branch taken
        bl      ..RtlpUnWaitCriticalSection
        b       nowaits
#else
        bge-    ..RtlpLeaveCriticalSection
                                                // predict branch not taken
                                                // NOTE: RtlLeaveCriticalSection will
                                                //       NOT appear in a stack trace
        li      r.3,STATUS_SUCCESS              // set completion status
        blr                                     // return
#endif

//
// Decrement the lock count and return a success status since the lock
// is still owned.
//

stillowned:
        stw     r.5,CsRecursionCount(r.3)

res3failed:
        lwarx   r.8,r.10,r.3                    // get addend value
        subi    r.8,r.8,1                       // decrement addend value
        stwcx.  r.8,r.10,r.3                    // store conditionally
        bne-    res3failed                      // if eq, store failed

nowaits:
        li      r.3,STATUS_SUCCESS              // set completion status

#if DBG
        NESTED_EXIT(RtlLeaveCriticalSection,EcFrameLength,0,0)
#else
        LEAF_EXIT(RtlLeaveCriticalSection)

//
//      r.3 - Pointer to the critical section
//
        NESTED_ENTRY(RtlpLeaveCriticalSection,EcFrameLength,0,0)
        PROLOGUE_END(RtlpLeaveCriticalSection)

        bl      ..RtlpUnWaitCriticalSection
        li      r.3,STATUS_SUCCESS              // set completion status

        NESTED_EXIT(RtlpLeaveCriticalSection,EcFrameLength,0,0)
#endif


//      SBTTL("Try to Enter Critical Section")
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
//    CriticalSection (r3) - Supplies a pointer to a critical section.
//
// Return Value:
//
//    If the critical section was successfully entered, then a value of TRUE
//    is returned as the function value. Otherwise, a value of FALSE is returned.
//
//--

        LEAF_ENTRY(RtlTryEnterCriticalSection)

        li      r6, CsLockCount                 // offset into critical section
        lwz     r5, TeClientId+4(r13)           // get current thread unique id
//
// Attempt to enter the critical section.
//

tecs10:
        lwarx   r7, r6, r3                      // get addend value - locked
        addic.  r8, r7, 1                       // increment addend value
        bne-    tecs20                          // jump if critical section owned
        stwcx.  r8, r6, r3                      // store conditionally
        bne-    tecs10                          // loop if store failed

//
// The critical section is now owned by this thread. Initialize the owner
// thread id and return a successful status.
//
        stw     r5, CsOwningThread(r3)          // set critical section owner

#if DBG
        lwz     r5, CsDebugInfo(r3)             // increment entry count
        lwz     r6, CsEntryCount(r5)            //
        addi    r6, r6, 1                       //
        stw     r6, CsEntryCount(r5)            //
        lwz     r6, TeCountOfOwnedCriticalSections(r13) // increment owned count
        addi    r6, r6, 1                               //
        stw     r6, TeCountOfOwnedCriticalSections(r13) //
#endif

        li      r3, TRUE                        // set success status
        blr                                     // return

tecs20:

//
// The critical section is already owned. If it is owned by another thread,
// return FALSE immediately. If it is owned by this thread, we must increment
// the lock count here.
//
        lwz     r7, CsOwningThread(r3)          // get current owner
        cmpw    r7, r5                          // same thread?
        beq     tecs30                          // if eq, this thread is already the owner

        li      r3, FALSE                       // set failure status
        blr                                     // return

tecs30:

        lwz     r4, CsRecursionCount(r3)

//
// This thread is already the owner of the critical section. Perform an atomic
// increment of the LockCount and a normal increment of the RecursionCount and
// return success.
//

tecs40:
        lwarx   r7, r6, r3                      // get addend value - locked
        addi    r8, r7, 1                       // increment addend value
        stwcx.  r8, r6, r3                      // store conditionally
        bne-    tecs40                          // loop if store failed

//
// Increment the recursion count
//
        addi    r5, r4, 1
        stw     r5, CsRecursionCount(r3)

#if DBG
        lwz     r5, CsDebugInfo(r3)             // increment entry count
        lwz     r6, CsEntryCount(r5)            //
        addi    r6, r6,1                        //
        stw     r6, CsEntryCount(r5)            //
#endif

        li      r3, TRUE                        // set success status
        LEAF_EXIT(RtlTryEnterCriticalSection)   // return

