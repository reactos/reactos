//++
//
// Copyright (c) 1989  Microsoft Corporation
//
// Module Name:
//
//     critsect.s
//
// Abstract:
//
//    This module implements functions to support user mode critical sections.
//
// Author:
//
//    William K. Cheung (wcheung) 18-Sep-95
//
// Revision History:
//
//    07-Jul-97  bl    Updated to EAS2.3
//
//    08-Feb-96        Updated to EAS2.1
//
//--

#include "ksia64.h"

        .file    "critsect.s"

        //
        // intra-module functions to be called.
        //

        PublicFunction(RtlpWaitForCriticalSection)
        PublicFunction(RtlpUnWaitCriticalSection)


//++
//
// NTSTATUS
// RtlEnterCriticalSection(
//     IN PRTL_CRITICAL_SECTION CriticalSection
//     )
//
// Routine Description:
//
//     This function enters a critical section.
//
// Arguments:
//
//     CriticalSection (a0) - supplies a pointer to a critical section.
//
// Return Value:
//
//     STATUS_SUCCESS or raises an exception if an error occured.
//
// Algorithm in C:
//
// NTSTATUS
// RtlEnterCriticalSection(
//     IN PRTL_CRITICAL_SECTION CriticalSection
//     )
// {
//     PTEB     Teb;
//     LONG     OriginalValue;
//     HANDLE   CurrentThreadId;
//     DWORD    SpinCount;
//
//     Teb = NtCurrentTeb();
//     CurrentThreadId = Teb->ClientId.UniqueThread;
//     SpinCount = CriticalSection->SpinCount;
//
//     if (SpinCount == 0) {
// nospin:
//           OriginalValue = AtomicIncrement(CriticalSection->LockCount);
//      
//           if (OriginalValue != -1) {
//               if (CriticalSection->OwningThread != CurrentThreadId) {
//                   RtlpWaitForCriticalSection(CriticalSection);
//                   CriticalSection->OwningThread = CurrentThreadId;
//                   CriticalSection->RecursionCount = 0;
//               } else {
//                   CriticalSection->RecursionCount++;
//               }
//           } else {
//               CriticalSection->OwningThread = CurrentThreadId;
//               CriticalSection->RecursionCount = 0;
//           }
//       
//           return STATUS_SUCCESS;
//
//     } else { /* spin count not 0 */
//
//           if (CriticalSection->OwningThread == CurrentThread) {
//               AtomicIncrement(CriticalSection->LockCount);
//               CriticalSection->RecursionCount++;
//               return STATUS_SUCCESS;
//           } else {
//             do {
//               if (!CmpXchg(CriticalSection->LockCount,0,-1)) {
//                  Lock acquired
//                  CriticalSection->OwningThread = CurrentThreadId;
//                  CriticalSection->RecursionCount = 0;
//                  return STATUS_SUCCESS;
//               }
//               if (CriticalSection->LockCount >= 1) goto nospin;
//
//               while (CriticalSection->LockCount == 0) {
//                  if (!--SpinCount) goto nospin;
//               }
//             } while (1) // CriticalSection->LockCount == -1,  not owned
//           }
//     } 
// }
//
//--



        NESTED_ENTRY(RtlEnterCriticalSection)

        //
        // register aliases
        //

        rTeb=teb
        rpThreadId=t3
        rOldLockCount=t4
        rpLockCount=t5
        rRecursionCount=t6
        rOwnerId=t7
        rpSpinCount=t9
        rSpinCount=t10
        rT1=t11
        rT2=t12
        
        pSpin=pt0
        pNoSpin=pt1
        pNotOwn=pt2
        pNotAcq=pt3
        pFree=pt5
        pHeld=pt6
        pGo=pt7
        pWait=pt8

//
// alloc regs and save brp & pfs
//

        NESTED_SETUP(1,5,1,0)
        add         rpThreadId = TeClientId+CidUniqueThread, rTeb

        //
        // more register aliases
        //

        rThreadId   = loc2
        rpOwner     = loc3
        rpCsRecursion = loc4

        PROLOGUE_END

//
// Swizzle pointers to address the RecursionCount and
// LockCount fields in the critical section structure.
//

        add         rpSpinCount = CsSpinCount, a0
        add         rpLockCount = CsLockCount, a0
        add         rpCsRecursion = CsRecursionCount, a0
        ;;

//
// Load the id of the currently running thread, anticipating
// that it may be needed.
//

        LDPTR(rThreadId, rpThreadId)
        ld4         rSpinCount = [rpSpinCount]
        add         rpOwner = CsOwningThread, a0
        ;;
        
//
// Branch out if spin count is non-zero
//      
        cmp4.ne     pSpin, pNoSpin = rSpinCount, zero
        mov         v0 = STATUS_SUCCESS
(pSpin) br.spnt     RecsSpin

//
// Atomically increment the lock count at location (rpLockCount)
// and put the old value in register "rOldLockCount".
// Swizzle a pointer to address the thread id field in teb structure.
//

RecsNoSpin:
        fetchadd4.acq rOldLockCount = [rpLockCount], 1
        ld4.nt1     rRecursionCount = [rpCsRecursion]
        ;;
        
//
// Check the original value of lock count to determine if the
// lock is free.  If the value is -1, it is free.
//

        cmp4.eq     pFree, pHeld = -1, rOldLockCount
        ;;

//
// if lock is not free, get the thread id of its current owner
// otherwise, save the currently running thread's id.
//

(pHeld) ld8         rOwnerId = [rpOwner]
(pFree) st8         [rpOwner] = rThreadId
(pFree) st4         [rpCsRecursion] = zero
(pFree) br.ret.sptk.clr brp                     // return
        ;;

//
// if lock is not free, compare the owner id of the critical section against 
// that of the thread to determine if this thread is the owner.
// otherwise, return to caller.
//

        cmp.eq      pGo, pWait = rThreadId, rOwnerId
        mov         out0 = a0
        add         rRecursionCount = 1, rRecursionCount
        ;;

//
// if the thread has already owned the lock, save the updated 
// recursion count and return.
// otherwise, wait for the critical section to be released.
//

(pGo)   st4         [rpCsRecursion] = rRecursionCount  
(pGo)   br.ret.sptk brp
(pWait) br.call.spnt.many brp = RtlpWaitForCriticalSection
        ;;
        mov         v0 = STATUS_SUCCESS
        st8.rel     [rpOwner] = rThreadId
        st4         [rpCsRecursion] = zero

        NESTED_RETURN

// A nonzero spin count is specified
//

RecsSpin:

        LDPTR(rOwnerId, rpOwner)
        ;;
        mov         rT1 = -1
        ;;
        zxt4        rT1 = rT1                   // zero extend for compare with
        ;;                                      // 4 byte lock value
        mov         ar.ccv = rT1                // compare value
        cmp.ne      pNotOwn = rOwnerId, rThreadId        
(pNotOwn) br.spnt   RecsNotOwn

//
// The critical section is owned by the current thread. Increment the lock
// count and the recursion count.
//
        
        fetchadd4.acq rOldLockCount = [rpLockCount], 1
        ld4         rRecursionCount = [rpCsRecursion]
        ;;
        add         rRecursionCount = 1, rRecursionCount
        ;;

        st4         [rpCsRecursion] = rRecursionCount
        br.ret.sptk.clr brp                     // return

//
// A nonzero spin count is specified and the current thread is not the owner.
//

RecsNotOwn:
        cmpxchg4.acq rT1 = [rpLockCount], zero  // try to acquire lock
        ;;
        
        cmp4.ne     pNotAcq = -1, rT1
(pNotAcq) br.spnt   RecsNotAcq

        
//
// The critical section has been acquired. Set the owning thread and the initial
// recursion count and return success.
//

        STPTR(rpOwner, rThreadId)
        st4         [rpCsRecursion] = zero
        br.ret.sptk.clr brp                     // return

//
// The critical section is currently owned. Spin until it is either unowned
// or the spin count has reached zero. 
//
// If LockCount > 0, then there are waiters. Don't spin because
// the lock will not free.
//

RecsNotAcq:
        ld4         rOldLockCount = [rpLockCount]
        ;;

        cmp4.eq     pNotOwn = -1, rOldLockCount
        cmp4.gt     pNoSpin = rOldLockCount, zero
(pNoSpin) br.spnt   RecsNoSpin
(pNotOwn) br.spnt   RecsNotOwn

        add         rSpinCount = -1, rSpinCount
        ;;

        cmp4.eq     pNoSpin, pSpin = zero, rSpinCount
(pNoSpin) br.spnt   RecsNoSpin
(pSpin) br.sptk     RecsNotAcq
        ;;

        NESTED_EXIT(RtlEnterCriticalSection)

//++
//
// NTSTATUS
// RtlLeaveCriticalSection(
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
// Algorithm in C:
//
// NTSTATUS
// RtlLeaveCriticalSection(
//     IN PRTL_CRITICAL_SECTION CriticalSection
//     )
// {
//     LONG NewRecursionCount;
//     LONG OldLockCount;
//     BOOL ToRelease;
// 
//     ASSERT(CriticalSection->RecursionCount >= 0)
//
//     if (CriticalSection->RecursionCount != 0) {
//         CriticalSection->RecursionCount -= 1;
//         AtomicDecrement(CriticalSection->LockCount);
//         return STATUS_SUCCESS;
//     }
// 
//     CriticalSection->OwningThread = 0; 
//     OldLockCount = AtomicDecrement(CriticalSection->LockCount);
// 
//     if ( OldLockCount != 0 ) {
//         RtlpUnWaitCriticalSection(CriticalSection);
//     }
// 
//     return STATUS_SUCCESS;
// }
// 
//--

//
// register aliases
//


        NESTED_ENTRY(RtlLeaveCriticalSection)

        rpOwner=t0
        rOldLockCount=t1
        rRecursionCount=t2
        rpLockCount=t5
        rpCsRecursion=t9

        pHold=pt0
        pGo=pt7
        pWait=pt8

        NESTED_SETUP(1,2,1,0)
        add         rpCsRecursion = CsRecursionCount, a0
        ;;

        PROLOGUE_END

//
// load recursion count
// swizzle pointers to address the LockCount and OwningThread
// fields in the critical section structure.
//

        ld4         rRecursionCount = [rpCsRecursion]
        add         rpOwner = CsOwningThread, a0
        add         rpLockCount = CsLockCount, a0
        ;;

//
// check if the original value of the recursion count to determine
// if the lock is to be released.
//
// decrement the register copy of recursion count by 1 and save
// the new value in temp register
//

        cmp.ne      pHold = zero, rRecursionCount
        add         rRecursionCount = -1, rRecursionCount
        add         v0 = STATUS_SUCCESS, zero   // return STATUS_SUCCESS
        ;;

//
// save the updated recursion count into the critical section structure.
//
// atomically decrement the lock count.
//
// if lock is still held, return to caller.
//
// Note: An atomic fetch & add with release form is used here
//       all previous memory accesses are visible at this point.
//

(pHold) st4         [rpCsRecursion] = rRecursionCount
        fetchadd4.rel rOldLockCount = [rpLockCount], -1
(pHold) br.ret.sptk.clr brp                     // return to caller
        ;;

//
// the lock is now free, clear the OwnerThread field in the critical 
// section structure,
// also check the original value of the lock count to determine if
// any other thread is waiting for this critical section.
// if no thread is waiting, return to caller immediately.
// 

        st8.rel     [rpOwner] = zero            // clear the owner field

        cmp4.ge     pGo, pWait = zero, rOldLockCount
  (pGo) br.ret.sptk.clr brp                     // return to caller

        mov         out0 = a0
(pWait) br.call.spnt.many brp = RtlpUnWaitCriticalSection
 
        mov         v0 = STATUS_SUCCESS         // return STATUS_SUCCESS

        NESTED_RETURN
        ;;

        NESTED_EXIT(RtlLeaveCriticalSection)


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
//    is returned as the function value. Otherwise, a value of FALSE is returned
//
//--

        LEAF_ENTRY(RtlTryEnterCriticalSection)

        //
        // register aliases
        //

        rT0=t0
        rT1=t1
        rTeb=t2
        rpThreadId=t3
        rOldLockCount=t4
        rpLockCount=t5
        rRecursionCount=t6
        rOwnerId=t7
        rpCsRecursion=t8
        rThreadId=t9
        rpOwner=t10
 
        pFree=pt5
        pHeld=pt6
        pOwn=pt7
        pFail=pt8

        alloc       rT0 = ar.pfs, 1, 0, 0, 0
        add         rpLockCount = CsLockCount, a0
        movl        rT1 = 0xffffffff
        ;;

        mov         ar.ccv = rT1
        mov         rTeb = teb
        add         rpOwner = CsOwningThread, a0
        ;;

        cmpxchg4.acq rOldLockCount = [rpLockCount], r0, ar.ccv
        add         rpCsRecursion = CsRecursionCount, a0
        nop.i       0
        ;;
 
        LDPTR(rOwnerId, rpOwner)
        cmp4.eq     pFree, pHeld = rT1, rOldLockCount

        add         rpThreadId = TeClientId+CidUniqueThread, rTeb
        ;;


        ld8.nt1     rThreadId = [rpThreadId]
(pHeld) ld4.nt1     rRecursionCount = [rpCsRecursion]
        mov         v0 = TRUE
        ;;

(pFree) st4         [rpCsRecursion] = zero
(pFree) st8         [rpOwner] = rThreadId
(pHeld) cmp.eq      pOwn, pFail = rThreadId, rOwnerId
(pFree) br.ret.sptk.clr brp
        ;;

(pOwn)  fetchadd4.acq rT0 = [rpLockCount], 1
(pOwn)  add         rRecursionCount = 1, rRecursionCount
        nop.i       0
        ;;

(pOwn)  st4.rel     [rpCsRecursion] = rRecursionCount  
(pFail) mov         v0 = FALSE
        br.ret.sptk.clr brp

        LEAF_EXIT(RtlTryEnterCriticalSection)
