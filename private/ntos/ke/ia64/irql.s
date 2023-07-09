//      TITLE("Manipulate Interrupt Request Level")
//++
//
// Module Name:
//
//    irql.s
//
// Abstract:
//
//    This module implements the code necessary to lower and raise the current
//    Interrupt Request Level (IRQL).
//
//
// Author:
//
//    William K. Cheung (wcheung) 05-Oct-1995
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//    08-Feb-1996    Updated to EAS2.1
//
//--

#include "ksia64.h"

         .file    "irql.s"

//
// Globals
//

        PublicFunction(KiCheckForSoftwareInterrupt)
       
//++
//
// VOID
// KiLowerIrqlSpecial (
//    KIRQL NewIrql
//    )
//
// Routine Description:
//
//    This function lowers the current IRQL to the specified value.
//    Does not check for software interrupts. For use within the software interupt
//    dispatch code.
//
// Arguments:
//
//    NewIrql (a0) - Supplies the new IRQL value.
//
// Return Value:
//
//    None.
//
// N.B. The IRQL is being lowered.  Therefore, it is not necessary to
//      do a data serialization after the TPR is updated unless it is
//      very critical to accept interrupts of lower priorities as soon
//      as possible.  The TPR change will take into effect eventually.
//
//--

        LEAF_ENTRY(KiLowerIrqlSpecial)

        SET_IRQL(a0)
        LEAF_RETURN

        LEAF_EXIT(KiLowerIrqlSpecial)
       
//++
//
// VOID
// KeLowerIrql (
//    KIRQL NewIrql
//    )
//
// Routine Description:
//
//    This function lowers the current IRQL to the specified value.
//
// Arguments:
//
//    NewIrql (a0) - Supplies the new IRQL value.
//
// Return Value:
//
//    None.
//
// N.B. The IRQL is being lowered.  Therefore, it is not necessary to
//      do a data serialization after the TPR is updated unless it is
//      very critical to accept interrupts of lower priorities as soon
//      as possible.  The TPR change will take into effect eventually.
//
//--

        NESTED_ENTRY(KeLowerIrql)
        NESTED_SETUP(1,2,1,0)

        rOldIrql    = t0
        rT1         = t1
        rpT1        = t2

        zxt1        a0 = a0
        ;;

        PROLOGUE_END

#if 0        
        br.call.sptk brp = KiCurrentKTraceEntry
        ;;
        
        ARGPTR      (v0)

        mov         rT1 = 0x2
        st4         [v0] = rT1,4                // module id
        
        mov         rT1 = 2                     
        st2         [v0] = rT1,2                // message info
        st2         [v0] = zero,2
        
        mov         rOldIrql = cr.tpr
        st4         [v0] = rOldIrql,4           // arg 1
        st4         [v0] = a0,4                 // arg 1

        st4         [v0] = savedbrp,4           // arg 2
#endif // 0

        LOWER_IRQL(a0)

        NESTED_RETURN
        NESTED_EXIT(KeLowerIrql)

//++
//
// VOID
// KeRaiseIrql (
//    KIRQL NewIrql,
//    PKIRQL OldIrql
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to the specified value and returns
//    the old IRQL value.
//
// Arguments:
//
//    NewIrql (a0) - Supplies the new IRQL value.
//
//    OldIrql (a1) - Supplies a pointer to a variable that recieves the old
//       IRQL value.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KeRaiseIrql)
        
//
// Register aliases
//

        rOldIrql    = t3
        
        GET_IRQL(rOldIrql)

        ARGPTR      (a1)                        

        SET_IRQL    (a0)                        // Raise IRQL
        ;;
        srlz.d
        ;;
        st1         [a1] = rOldIrql             // return old IRQL value

        LEAF_RETURN

        LEAF_EXIT(KeRaiseIrql)

//++
//
// KIRQL
// KeRaiseIrqlToDpcLevel (
//    VOID
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to DISPATCH_LEVEL and returns
//    the old IRQL value.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    Old IRQL value
//
//--

        LEAF_ENTRY(KeRaiseIrqlToDpcLevel)
//
// Register aliases
//

        rNewIrql = t0
        
        mov         rNewIrql = DISPATCH_LEVEL
        GET_IRQL(v0)
        ;;

        SET_IRQL    (rNewIrql)                  // Raise IRQL

        LEAF_RETURN
        LEAF_EXIT(KeRaiseIrqlToDpcLevel)

//++
//
// KIRQL
// KeRaiseIrqlToSynchLevel (
//    VOID
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to SYNCH_LEVEL and returns
//    the old IRQL value.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    Old IRQL value
//
//--

        LEAF_ENTRY(KeRaiseIrqlToSynchLevel)

//
// Register aliases
//

        rNewIrql = t0
        
        mov         rNewIrql = SYNCH_LEVEL
        GET_IRQL(v0)
        ;;

        SET_IRQL    (rNewIrql)                  // Raise IRQL
        LEAF_RETURN

        LEAF_EXIT(KeRaiseIrqlToSynchLevel)
