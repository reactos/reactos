//      TITLE("Fast Mutex Support")
//++
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
//    William K. Cheung (wcheung) 02-Oct-1995
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//    08-Feb-96    Updated to EAS2.1
//
//--

#include "ksia64.h"

        .file "fmutex.s"

        PublicFunction(KeWaitForSingleObject)
        PublicFunction(KeSetEventBoostPriority)
        PublicFunction(KiCheckForSoftwareInterrupt)

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
//    APC Level. Must be called at IRQL < DISPATCH_LEVEL.
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

//
// t0  -- Original value of Fmutex.FmCount
// t1  -- Address of Fmutex.FmCount
// t2  -- Address of Fmutex.FmOldIrql
// t3  -- Address of KiPcr
// t4  -- Address of Fmutex.FmContention
// t8  -- Address of KiPcr.PcCurrentThread
// r35 -- old IRQL
//
// t5  -- temp
//

        NESTED_ENTRY(ExAcquireFastMutex)

        .regstk     1, 3, 5, 0
        .prologue   0xC, savedpfs

        alloc       savedpfs = ar.pfs, 1, 3, 5, 0
        mov         savedbrp = brp
        GET_IRQL    (loc2)                      // Get old IRQL

        PROLOGUE_END

        ARGPTR(a0)

        SET_IRQL    (APC_LEVEL)                 // change IRQL to APC_LEVEL
        add         t1 = FmCount, a0
        add         t2 = FmOldIrql, a0
        ;;

        //
        // synchronize subsequent reads
        //

        fetchadd4.acq t0 = [t1], -1             // decrement mutex count
        add         t4 = FmContention, a0
        add         out0 = FmEvent, a0

        mov         out1 = Executive            // set reason for wait
        mov         out2 = KernelMode           // set mode of wait
        ;;

        cmp4.le     pt7, pt8 = t0, zero         // if le, mutex acq failed
        mov         out3 = FALSE                // set nonalertable wait
        mov         out4 = zero                 // set NULL timeout pointer
        ;;

  (pt7) ld4         t5 = [t4]                   // load contention count
  (pt8) st4         [t2] = loc2                 // save old IRQL
  (pt8) br.ret.sptk.clr brp                     // return
        ;;

Eafm10:

  (pt7) add         t5 = 1, t5                  // inc contention count
        ;;
        st4         [t4] = t5                   // save contention count
        br.call.sptk.many brp = KeWaitForSingleObject

        add         t2 = FmOldIrql, a0
        mov         ar.pfs = savedpfs           // restore pfs
        mov         brp = savedbrp              // restore brp
        ;;

        st4         [t2] = loc2                 // save old IRQL
        nop.m       0
        br.ret.sptk.clr brp                     // return

        NESTED_EXIT(ExAcquireFastMutex)

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

        NESTED_ENTRY(ExReleaseFastMutex)
  
        NESTED_SETUP(1, 3, 2, 0)
        ARGPTR(a0)
        add         t1 = FmCount, a0
        ;;

        PROLOGUE_END

//
// Increment mutex count and release waiter if contention.
//

        //
        // use release semantics to synchronize all previous writes
        //

        fetchadd4.rel t9 = [t1], 1              // increment mutex count
        add         t2 = FmOldIrql, a0
        add         out0 = FmEvent, a0
        ;;

        ld4         loc2 = [t2]                 // get old IRQL
        cmp4.eq     pt1, pt0 = zero, t9         // if eq, no waiter

        mov         out1 = zero
 (pt0)  br.call.spnt.many brp = KeSetEventBoostPriority
        ;;
        
        LOWER_IRQL  (loc2)
        mov         ar.pfs = savedpfs           // restore pfs
        mov         brp = savedbrp              // restore brp
        ;;

        br.ret.sptk.clr brp                     // return

        NESTED_EXIT(ExReleaseFastMutex)

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
//    If the fast mutex was successfully acquired, then a value of TRUE
//    is returned as the function value. Otherwise, a valye of FALSE is
//    returned.
//
//--

        LEAF_ENTRY(ExTryToAcquireFastMutex)

        GET_IRQL    (t0)                       // get old IRQL
        SET_IRQL    (APC_LEVEL)                // change IRQL to APC_LEVEL
        add         t1 = FmCount, a0
        add         t2 = FmOldIrql, a0
        ;;


        //
        // use acquire semantics to sychronize subsequent reads
        //

        fetchadd4.acq t4 = [t1], -1            // decrement mutex count
        ;;
        cmp4.le     pt8, pt7 = t4, zero        // if le, mutex acq failed
        mov         v0 = TRUE                  // return TRUE by default
        ;;
        
        PSET_IRQL   (pt8, t0)                  // restore IRQL        
  (pt7) st4         [t2] = t0                  // save old IRQL
  (pt8) mov         v0 = FALSE                 // return FALSE
        br.ret.sptk.clr brp                    // return

        LEAF_EXIT(ExTryToAcquireFastMutex)

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
//    IRQL to APC Level.
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

        NESTED_ENTRY(ExAcquireFastMutexUnsafe)

        NESTED_SETUP(1, 2, 5, 0)
        ARGPTR(a0)
        add         t1 = FmCount, a0
        ;;

        PROLOGUE_END

        //
        // Use acquire semantics to synchronize subsequent reads
        //

        fetchadd4.acq t0 = [t1], -1             // decrement mutex count
        add         out0 = FmEvent, a0

        add         t3 = FmContention, a0
        mov         out1 = Executive            // set reason for wait
        mov         out2 = KernelMode           // set mode of wait
        ;;

        cmp4.le     p0, pt8 = t0, zero          // if le, contention
        mov         out3 = FALSE                // set nonalertable wait
  (pt8) br.ret.sptk.clr brp                     // return

//
// Increment the contention count and then call KeWaitForSingleObject().
// The outgoing arguments have been set up ahead of time.
//

        ld4         t4 = [t3]                   // get contention count
        ;;
        add         t4 = 1, t4                  // increment contention count
        mov         out4 = 0                    // set NULL timeout pointer
        ;;

        st4         [t3] = t4                   // save contention count
        br.call.sptk.many brp = KeWaitForSingleObject
  
        mov         ar.pfs = savedpfs           // restore pfs
        mov         brp = savedbrp              // restore return link
        br.ret.sptk.clr brp                     // return

        NESTED_EXIT(ExAcquireFastMutexUnsafe)

//++
//
// VOID
// ExReleaseFastMutexUnsafe (
//    IN PFAST_MUTEX FastMutex
//    )
//
// Routine Description:
//
//    This function releases ownership to a fast mutex, and does not
//    restore IRQL to its previous level.
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

        NESTED_ENTRY(ExReleaseFastMutexUnsafe)
  
        NESTED_SETUP(1, 2, 2, 0)
        ARGPTR(a0)
        add         t1 = FmCount, a0
        ;;

        PROLOGUE_END

//
// Increment mutex count and release waiter if contention.
//

        //
        // Use release semantics to syncrhonize all previous writes
        // before the mutext is released.
        //

        fetchadd4.rel t9 = [t1], 1              // increment mutex count
        ;;
        add         out0 = FmEvent, a0
        cmp4.eq     pt7, pt8 = zero, t9         // if eq, no waiter
        ;;

        add         out1 = zero, zero
  (pt7) br.ret.sptk.clr brp                     // return
  (pt8) br.call.spnt.many brp = KeSetEventBoostPriority

        mov         ar.pfs = savedpfs
        mov         brp = savedbrp
        br.ret.sptk.clr brp                     // return

        NESTED_EXIT(ExReleaseFastMutexUnsafe)


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
//    If the fast mutex was successfully acquired, then a value of TRUE
//    is returned as the function value. Otherwise, a valye of FALSE is
//    returned.
//
//--

#if 0
         LEAF_ENTRY(ExTryToAcquireFastMutexUnsafe)


         LEAF_EXIT(ExTryToAcquireFastMutexUnsafe)
#endif
